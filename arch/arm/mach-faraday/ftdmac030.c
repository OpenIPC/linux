/*
 * Faraday FTDMAC030 DMA engine driver
 *
 * (C) Copyright 2011 Faraday Technology
 * Po-Yu Chuang <ratbert@faraday-tech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <linux/dma-mapping.h>
#include <linux/dmaengine.h>
#include <linux/dmapool.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/slab.h>

#include <asm/io.h>

#include <mach/ftdmac030.h>

#define DRV_NAME	"ftdmac030"
#define CHANNEL_NR	8

/*
 * This value must be multiple of 32-bits to prevent from changing the
 * alignment of child descriptors.
 */
#define MAX_CYCLE_PER_BLOCK	0x200000
#if (MAX_CYCLE_PER_BLOCK > FTDMAC030_CYC_MASK) || \
	(MAX_CYCLE_PER_BLOCK & 0x3)
#error invalid MAX_CYCLE_PER_BLOCK
#endif

#define GRANT_WINDOW	64

/*
 * Initial number of descriptors to allocate for each channel. This could
 * be increased during dma usage.
 */
static unsigned int init_nr_desc_per_channel = 64;
module_param(init_nr_desc_per_channel, uint, 0644);
MODULE_PARM_DESC(init_nr_desc_per_channel,
		 "initial descriptors per channel (default: 64)");

/**
 * struct ftdmac030_desc - async transaction descriptor.
 * @lld: hardware descriptor, MUST be the first member
 * @ctrl: value for channel control register
 * @cfg: value for channel configuration register
 * @src: source physical address
 * @dst: destination physical addr
 * @next: phsical address to the first link list descriptor (2nd block)
 * @cycle: transfer size
 * @txd: support for the async_tx api
 * @child_list: list for transfer chain
 * @ftchan: the channel which this descriptor belongs to
 * @node: node on the descriptors list
 * @len: length in bytes
 */
struct ftdmac030_desc {
	struct ftdmac030_lld lld;

	/* used only by the first block */
	unsigned int cfg;

	struct dma_async_tx_descriptor txd;
	struct list_head child_list;

	/* used by all blocks */
	struct ftdmac030_chan *ftchan;
	struct list_head node;
	size_t len;
};

/**
 * struct ftdmac030_chan - internal representation of an ftdmac030 channel.
 * @common: common dmaengine channel object
 * @active_list: list of descriptors dmaengine is being running on
 * @free_list: list of descriptors usable by the channel
 * @tasklet: bottom half to finish transaction work
 * @lock: serializes enqueue/dequeue operations to descriptors lists
 * @ftdmac030: parent device
 * @completed_cookie: identifier for the most recently completed operation
 * @descs_allocated: number of allocated descriptors
 */
struct ftdmac030_chan {
	struct dma_chan common;
	struct list_head active_list;
	struct list_head free_list;
	struct tasklet_struct tasklet;
	spinlock_t lock;
	struct ftdmac030 *ftdmac030;
	dma_cookie_t completed_cookie;
	int descs_allocated;
};

/**
 * struct ftdmac030 - internal representation of an ftdmac030 device
 * @dma: common dmaengine dma_device object
 * @res: io memory resource of hardware registers
 * @base: virtual address of hardware register base
 * @id: id for each device
 * @irq: irq number
 * @channel: channel table
 */
struct ftdmac030 {
	struct dma_device dma;
	struct resource *res;
	void __iomem *base;
	int id;
	unsigned int irq;
	struct dma_pool *dma_desc_pool;
	struct ftdmac030_chan channel[CHANNEL_NR];
};

static dma_cookie_t ftdmac030_tx_submit(struct dma_async_tx_descriptor *);

/******************************************************************************
 * internal functions
 *****************************************************************************/
static inline struct device *chan2dev(struct dma_chan *chan)
{
	return &chan->dev->device;
}

static void ftdmac030_stop_channel(struct ftdmac030_chan *ftchan)
{
	int chan_id = ftchan->common.chan_id;
	void __iomem *base = ftchan->ftdmac030->base;

	iowrite32(0, base + FTDMAC030_OFFSET_CTRL_CH(chan_id));
}

static int ftdmac030_chan_is_enabled(struct ftdmac030_chan *ftchan)
{
	int chan_id = ftchan->common.chan_id;
	void __iomem *base = ftchan->ftdmac030->base;
	unsigned int enabled;

	enabled = ioread32(base + FTDMAC030_OFFSET_CH_ENABLED);
	return enabled & (1 << chan_id);
}

/**
 * ftdmac030_alloc_desc - allocate and initialize descriptor
 * @ftchan: the channel to allocate descriptor for
 * @gfp_flags: GFP allocation flags
 */
static struct ftdmac030_desc *ftdmac030_alloc_desc(
		struct ftdmac030_chan *ftchan, gfp_t gfp_flags)
{
	struct dma_chan *chan = &ftchan->common;
	struct ftdmac030 *ftdmac030 = ftchan->ftdmac030;
	struct ftdmac030_desc *desc;
	dma_addr_t phys;

	dev_dbg(chan2dev(chan), "%s\n", __func__);
	desc = dma_pool_alloc(ftdmac030->dma_desc_pool, gfp_flags, &phys);
	if (desc) {
		memset(desc, 0, sizeof(*desc));

		/* initialize dma_async_tx_descriptor fields */
		dma_async_tx_descriptor_init(&desc->txd, &ftchan->common);
		desc->txd.tx_submit = ftdmac030_tx_submit;
		desc->txd.phys = phys;

		INIT_LIST_HEAD(&desc->child_list);
		desc->ftchan = ftchan;
	}

	return desc;
}

static void ftdmac030_free_desc(struct ftdmac030_desc *desc)
{
	struct ftdmac030_chan *ftchan = desc->ftchan;
	struct dma_chan *chan = &desc->ftchan->common;
	struct ftdmac030 *ftdmac030 = ftchan->ftdmac030;

	dev_dbg(chan2dev(chan), "%s(%p)\n", __func__, desc);
	dma_pool_free(ftdmac030->dma_desc_pool, desc, desc->txd.phys);
}

/**
 * ftdmac030_desc_get - get an unused descriptor from free list
 * @ftchan: channel we want a new descriptor for
 */
static struct ftdmac030_desc *ftdmac030_desc_get(struct ftdmac030_chan *ftchan)
{
	struct dma_chan *chan = &ftchan->common;
	struct ftdmac030_desc *desc = NULL;

	spin_lock_bh(&ftchan->lock);
	if (!list_empty(&ftchan->free_list)) {
		desc = list_first_entry(&ftchan->free_list,
			struct ftdmac030_desc, node);

		list_del(&desc->node);
	}
	spin_unlock_bh(&ftchan->lock);

	/* no more descriptor available in initial pool: create one more */
	if (!desc) {
		desc = ftdmac030_alloc_desc(ftchan, GFP_ATOMIC);
		if (desc) {
			spin_lock_bh(&ftchan->lock);
			ftchan->descs_allocated++;
			spin_unlock_bh(&ftchan->lock);
		} else {
			dev_err(chan2dev(chan),
				"not enough descriptors available\n");
		}
	} else {
		dev_dbg(chan2dev(chan), "%s got desc %p\n", __func__, desc);
	}

	return desc;
}

/**
 * ftdmac030_desc_put - move a descriptor, including any children, to the free list
 * @ftchan: channel we work on
 * @desc: descriptor, at the head of a chain, to move to free list
 */
static void ftdmac030_desc_put(struct ftdmac030_desc *desc)
{
	struct ftdmac030_chan *ftchan = desc->ftchan;
	struct dma_chan *chan = &ftchan->common;
	struct ftdmac030_desc *child;

	spin_lock_bh(&ftchan->lock);

	if (!list_empty(&desc->child_list)) {
		list_for_each_entry(child, &desc->child_list, node) {
			dev_dbg(chan2dev(chan),
				"moving child desc %p to freelist\n", child);
		}

		list_splice_init(&desc->child_list, &ftchan->free_list);
	}

	dev_dbg(chan2dev(chan), "moving desc %p to freelist\n", desc);
	list_add(&desc->node, &ftchan->free_list);
	spin_unlock_bh(&ftchan->lock);
}

static void ftdmac030_unmap_desc(struct ftdmac030_desc *desc)
{
	struct ftdmac030_chan *ftchan = desc->ftchan;
	struct dma_chan *chan = &ftchan->common;
	enum dma_ctrl_flags flags = desc->txd.flags;
	struct device *parent = ftchan->common.dev->device.parent;

	dev_dbg(chan2dev(chan), "%s(%p)\n", __func__, desc);
	if (ftchan->common.private)
		return;

	if (!(flags & DMA_COMPL_SKIP_DEST_UNMAP)) {
		if (flags & DMA_COMPL_DEST_UNMAP_SINGLE) {
			dma_unmap_single(parent, desc->lld.dst, desc->len,
				DMA_FROM_DEVICE);
		} else {
			dma_unmap_page(parent, desc->lld.dst, desc->len,
				DMA_FROM_DEVICE);
		}
	}

	if (!(flags & DMA_COMPL_SKIP_SRC_UNMAP)) {
		if (flags & DMA_COMPL_SRC_UNMAP_SINGLE) {
			dma_unmap_single(parent, desc->lld.src, desc->len,
				DMA_TO_DEVICE);
		} else {
			dma_unmap_page(parent, desc->lld.src, desc->len,
				DMA_TO_DEVICE);
		}
	}
}

static void ftdmac030_remove_chain(struct ftdmac030_desc *desc)
{
	struct ftdmac030_chan *ftchan = desc->ftchan;
	struct dma_chan *chan = &ftchan->common;
	struct ftdmac030_desc *child;
	struct ftdmac030_desc *tmp;

	dev_dbg(chan2dev(chan), "%s(%p)\n", __func__, desc);
	list_for_each_entry_safe(child, tmp, &desc->child_list, node) {
		dev_dbg(chan2dev(chan), "removing child desc %p\n", child);
		list_del(&child->node);
		ftdmac030_unmap_desc(child);
		ftdmac030_desc_put(child);
	}

	ftdmac030_unmap_desc(desc);
	ftdmac030_desc_put(desc);
}

/**
 * ftdmac030_create_chain - create a DMA transfer chain
 * @ftchan: the channel to allocate descriptor for
 * @first: first descriptor of a transfer chain if any
 * @src: physical source address
 * @dest: phyical destination address
 * @len: length in bytes
 * @shift: shift value for width (0: byte, 1: halfword, 2: word)
 * @fixed_src: source address is fixed (device register)
 * @fixed_dest: destination address is fixed (device register)
 */
static struct ftdmac030_desc *ftdmac030_create_chain(
		struct ftdmac030_chan *ftchan,
		struct ftdmac030_desc *first,
		dma_addr_t src, dma_addr_t dest, size_t len,
		unsigned int shift, int fixed_src, int fixed_dest)
{
	struct dma_chan *chan = &ftchan->common;
	struct ftdmac030_desc *prev = NULL;
	unsigned int ctrl;
	size_t offset;
	unsigned int cycle;

	dev_dbg(chan2dev(chan), "%s(src %x, dest %x, len %x, shift %d)\n",
		__func__, src, dest, len, shift);
	if ((shift == 2 && ((src | dest | len) & 3)) ||
	    (shift == 1 && ((src | dest | len) & 1))) {
		dev_err(chan2dev(chan), "%s: register or data misaligned\n",
			__func__);
		return NULL;
	}

	if (first) {
		if (list_empty(&first->child_list))
			prev = first;
		else
			prev = list_entry(first->child_list.prev,
				struct ftdmac030_desc, node);
	}

	ctrl = FTDMAC030_CTRL_ENABLE | FTDMAC030_CTRL_1BEAT;

	switch (shift) {
	case 2:
		ctrl |= FTDMAC030_CTRL_DST_WIDTH_32
		     |  FTDMAC030_CTRL_SRC_WIDTH_32;
		break;
	case 1:
		ctrl |= FTDMAC030_CTRL_DST_WIDTH_16
		     |  FTDMAC030_CTRL_SRC_WIDTH_16;
		break;
	case 0:
		ctrl |= FTDMAC030_CTRL_DST_WIDTH_8
		     |  FTDMAC030_CTRL_SRC_WIDTH_8;
		break;
	default:
		dev_err(chan2dev(chan), "%s: incorrect data width\n",
			__func__);
		BUG();
		break;
	}

	if (fixed_src)
		ctrl |= FTDMAC030_CTRL_SRC_FIXED;
	else
		ctrl |= FTDMAC030_CTRL_SRC_INC;

	if (fixed_dest)
		ctrl |= FTDMAC030_CTRL_DST_FIXED;
	else
		ctrl |= FTDMAC030_CTRL_DST_INC;

	for (offset = 0; offset < len; offset += cycle << shift) {
		struct ftdmac030_desc *desc;

		cycle = min_t(size_t, (len - offset) >> shift,
				MAX_CYCLE_PER_BLOCK);

		desc = ftdmac030_desc_get(ftchan);
		if (!desc)
			goto err;

		if (fixed_src)
			desc->lld.src = src;
		else
			desc->lld.src = src + offset;

		if (fixed_dest)
			desc->lld.dst = dest;
		else
			desc->lld.dst = dest + offset;

		desc->cfg = FTDMAC030_CFG_GW(GRANT_WINDOW)
			  | FTDMAC030_CFG_HIGH_PRIO;
		desc->len = cycle << shift;

		desc->lld.next	= 0;
		desc->lld.cycle	= FTDMAC030_CYC_TOTAL(cycle);
		desc->lld.ctrl	= ctrl;

		if (!first) {
			first = desc;
		} else {
			/*
			 * Mask terminal count interrupt for this descriptor.
			 * What an inconvenient stupid design.
			 */
			prev->lld.ctrl |= FTDMAC030_CTRL_MASK_TC;

			/* hardware link list pointer */
			prev->lld.next = desc->txd.phys;

			/* insert the link descriptor to the transfer chain */
			list_add_tail(&desc->node, &first->child_list);
		}

		prev = desc;
	}

	return first;
err:
	if (first)
		ftdmac030_remove_chain(first);
	return NULL;
}

static void ftdmac030_start_chain(struct ftdmac030_desc *desc)
{
	struct ftdmac030_chan *ftchan = desc->ftchan;
	struct dma_chan *chan = &ftchan->common;
	void __iomem *base = ftchan->ftdmac030->base;
	int chan_id = ftchan->common.chan_id;

	dev_dbg(chan2dev(chan), "%s(%p)\n", __func__, desc);

	/*
	 * The first transfer block is not described by hardware lld.
	 * Instead, we should fill some hardware registers.
	 * What a stupid weird design.
	 */
	dev_dbg(chan2dev(chan), "\t[SRC %d] = %x\n", chan_id, desc->lld.src);
	iowrite32(desc->lld.src, base + FTDMAC030_OFFSET_SRC_CH(chan_id));
	dev_dbg(chan2dev(chan), "\t[DST %d] = %x\n", chan_id, desc->lld.dst);
	iowrite32(desc->lld.dst, base + FTDMAC030_OFFSET_DST_CH(chan_id));
	dev_dbg(chan2dev(chan), "\t[LLP %d] = %x\n", chan_id, desc->lld.next);
	iowrite32(desc->lld.next, base + FTDMAC030_OFFSET_LLP_CH(chan_id));
	dev_dbg(chan2dev(chan), "\t[CYC %d] = %x\n", chan_id, desc->lld.cycle);
	iowrite32(desc->lld.cycle, base + FTDMAC030_OFFSET_CYC_CH(chan_id));

	/* go */
	dev_dbg(chan2dev(chan), "\t[CFG %d] = %x\n", chan_id, desc->cfg);
	iowrite32(desc->cfg, base + FTDMAC030_OFFSET_CFG_CH(chan_id));
	dev_dbg(chan2dev(chan), "\t[CTRL %d] = %x\n", chan_id, desc->lld.ctrl);
	iowrite32(desc->lld.ctrl, base + FTDMAC030_OFFSET_CTRL_CH(chan_id));
}

static void ftdmac030_start_new_chain(struct ftdmac030_chan *ftchan)
{
	struct dma_chan *chan = &ftchan->common;
	struct ftdmac030_desc *desc;

	dev_dbg(chan2dev(chan), "%s\n", __func__);
	if (list_empty(&ftchan->active_list))
		return;

	desc = list_first_entry(&ftchan->active_list, struct ftdmac030_desc, node);
	ftdmac030_start_chain(desc);
}

static void ftdmac030_finish_chain(struct ftdmac030_desc *desc)
{
	struct ftdmac030_chan *ftchan = desc->ftchan;
	struct dma_chan *chan = &ftchan->common;

	dev_dbg(chan2dev(chan), "%s(%p)\n", __func__, desc);
	spin_lock_bh(&ftchan->lock);
	ftchan->completed_cookie = desc->txd.cookie;
	spin_unlock_bh(&ftchan->lock);

	/*
	 * The API requires that no submissions are done from a
	 * callback, so we don't need to drop the lock here
	 */
	if (desc->txd.callback)
		desc->txd.callback(desc->txd.callback_param);

	ftdmac030_remove_chain(desc);
}

static void ftdmac030_finish_all_chains(struct ftdmac030_chan *ftchan)
{
	struct dma_chan *chan = &ftchan->common;
	struct ftdmac030_desc *desc;
	struct ftdmac030_desc *tmp;

	dev_dbg(chan2dev(chan), "%s\n", __func__);
	list_for_each_entry_safe(desc, tmp, &ftchan->active_list, node) {
		list_del(&desc->node);
		ftdmac030_finish_chain(desc);
	}
}

static dma_cookie_t ftdmac030_new_cookie(struct ftdmac030_chan *ftchan)
{
	struct dma_chan *chan = &ftchan->common;
	dma_cookie_t cookie = ftchan->common.cookie;

	dev_dbg(chan2dev(chan), "%s\n", __func__);
	if (++cookie < 0)
		cookie = DMA_MIN_COOKIE;

	return cookie;
}

/******************************************************************************
 * filter function for dma_request_channel()
 *****************************************************************************/
bool ftdmac030_chan_filter(struct dma_chan *chan, void *data)
{
	const char *drv_name = dev_driver_string(chan->device->dev);
	struct ftdmac030_dma_slave *slave = data;
	struct ftdmac030_chan *ftchan;
	struct ftdmac030 *ftdmac030;

	ftchan = container_of(chan, struct ftdmac030_chan, common);
	ftdmac030 = ftchan->ftdmac030;

	if (strncmp(DRV_NAME, drv_name, sizeof(DRV_NAME)))
		return false;

	if (slave->id >= 0 && slave->id != ftdmac030->id)
		return false;

	if ((slave->channels & (1 << chan->chan_id)) == 0)
		return false;

	chan->private = slave;
	return true;
}
EXPORT_SYMBOL_GPL(ftdmac030_chan_filter);

/******************************************************************************
 * tasklet - called after we finished one chain
 *****************************************************************************/
static void ftdmac030_tasklet(unsigned long data)
{
	struct ftdmac030_chan *ftchan = (struct ftdmac030_chan *)data;
	struct dma_chan *chan = &ftchan->common;
	struct ftdmac030_desc *desc;

	dev_dbg(chan2dev(chan), "%s\n", __func__);
	BUG_ON(list_empty(&ftchan->active_list));

	spin_lock_bh(&ftchan->lock);

	/* remove already finished descriptor */
	desc = list_first_entry(&ftchan->active_list, struct ftdmac030_desc,
		node);
	list_del(&desc->node);

	/* check if there were another transfer to do */
	ftdmac030_start_new_chain(ftchan);

	spin_unlock_bh(&ftchan->lock);
	ftdmac030_finish_chain(desc);
}

/******************************************************************************
 * interrupt handler
 *****************************************************************************/
static irqreturn_t ftdmac030_interrupt(int irq, void *dev_id)
{
	struct ftdmac030 *ftdmac030 = dev_id;
	unsigned int tcs;
	unsigned int eas;
	int i;

	tcs = ioread32(ftdmac030->base + FTDMAC030_OFFSET_TCISR);
	eas = ioread32(ftdmac030->base + FTDMAC030_OFFSET_EAISR);
	if (!tcs && !eas)
		return IRQ_NONE;

	/* clear status */
	if (tcs)
		iowrite32(tcs, ftdmac030->base + FTDMAC030_OFFSET_TCICR);
	if (eas)
		iowrite32(eas, ftdmac030->base + FTDMAC030_OFFSET_EAICR);

	for (i = 0; i < CHANNEL_NR; i++) {
		struct ftdmac030_chan *ftchan = &ftdmac030->channel[i];
		struct dma_chan *chan = &ftchan->common;

		if (eas & FTDMAC030_EA_ERR_CH(i))
			dev_info(chan2dev(chan), "error happened\n");

		if (eas & FTDMAC030_EA_ABT_CH(i))
			dev_info(chan2dev(chan), "transfer aborted\n");

		if (tcs & (1 << i)) {
			dev_dbg(chan2dev(chan), "terminal count\n");
			tasklet_schedule(&ftchan->tasklet);
		}
	}

	return IRQ_HANDLED;
}

/******************************************************************************
 * struct dma_async_tx_descriptor function
 *****************************************************************************/

/**
 * ftdmac030_tx_submit - set the prepared descriptor(s) to be executed by the engine
 * @txd: async transaction descriptor
 */
static dma_cookie_t ftdmac030_tx_submit(struct dma_async_tx_descriptor *txd)
{
	struct ftdmac030_desc *desc;
	struct ftdmac030_chan *ftchan;
	struct dma_chan *chan;
	dma_cookie_t cookie;
	bool busy;

	desc = container_of(txd, struct ftdmac030_desc, txd);
	ftchan = desc->ftchan;
	chan = &ftchan->common;

	dev_dbg(chan2dev(chan), "%s: submit desc %p\n", __func__, desc);
	/* we support simple situation only */
	BUG_ON(!async_tx_test_ack(txd));

	spin_lock_bh(&ftchan->lock);

	cookie = ftdmac030_new_cookie(ftchan);
	ftchan->common.cookie = cookie;
	txd->cookie = cookie;

	if (list_empty(&ftchan->active_list))
		busy = false;
	else
		busy = true;

	list_add_tail(&desc->node, &ftchan->active_list);

	/* start ASAP */
	if (!busy)
		ftdmac030_start_chain(desc);

	spin_unlock_bh(&ftchan->lock);

	return cookie;
}

/******************************************************************************
 * struct dma_device functions
 *****************************************************************************/

/**
 * ftdmac030_alloc_chan_resources - allocate resources for DMA channel
 * @chan: DMA channel
 */
static int ftdmac030_alloc_chan_resources(struct dma_chan *chan)
{
	struct ftdmac030_chan *ftchan;
	LIST_HEAD(tmp_list);
	int i;

	ftchan = container_of(chan, struct ftdmac030_chan, common);

	/* have we already been set up?
	 * reconfigure channel but no need to reallocate descriptors */
	if (!list_empty(&ftchan->free_list))
		return ftchan->descs_allocated;

	/* Allocate initial pool of descriptors */
	for (i = 0; i < init_nr_desc_per_channel; i++) {
		struct ftdmac030_desc *desc;

		desc = ftdmac030_alloc_desc(ftchan, GFP_KERNEL);
		if (!desc) {
			dev_err(chan2dev(chan),
				"Only %d initial descriptors\n", i);
			break;
		}
		list_add_tail(&desc->node, &tmp_list);
	}

	spin_lock_bh(&ftchan->lock);
	ftchan->descs_allocated = i;
	list_splice(&tmp_list, &ftchan->free_list);
	ftchan->completed_cookie = chan->cookie = 1;
	spin_unlock_bh(&ftchan->lock);

	dev_dbg(chan2dev(chan), "%s: allocated %d descriptors\n",
		__func__, ftchan->descs_allocated);

	return ftchan->descs_allocated;
}

/**
 * ftdmac030_free_chan_resources - free all channel resources
 * @chan: DMA channel
 */
static void ftdmac030_free_chan_resources(struct dma_chan *chan)
{
	struct ftdmac030_chan *ftchan;
	struct ftdmac030_desc *desc;
	struct ftdmac030_desc *tmp;
	struct ftdmac030 *ftdmac030;

	dev_dbg(chan2dev(chan), "%s\n", __func__);
	ftchan = container_of(chan, struct ftdmac030_chan, common);
	ftdmac030 = ftchan->ftdmac030;

	/* channel must be idle */
	BUG_ON(!list_empty(&ftchan->active_list));
	BUG_ON(ftdmac030_chan_is_enabled(ftchan));

	spin_lock_bh(&ftchan->lock);
	ftdmac030_stop_channel(ftchan);
	ftdmac030_finish_all_chains(ftchan);
	list_for_each_entry_safe(desc, tmp, &ftchan->free_list, node) {
		dev_dbg(chan2dev(chan), "  freeing descriptor %p\n", desc);
		list_del(&desc->node);
		/* free link descriptor */
		ftdmac030_free_desc(desc);
	}

	ftchan->descs_allocated = 0;
	spin_unlock_bh(&ftchan->lock);
}

/**
 * ftdmac030_prep_dma_memcpy - prepare a memcpy operation
 * @chan: the channel to prepare operation on
 * @dest: operation virtual destination address
 * @src: operation virtual source address
 * @len: operation length
 * @flags: tx descriptor status flags
 */
static struct dma_async_tx_descriptor *
ftdmac030_prep_dma_memcpy(struct dma_chan *chan, dma_addr_t dest,
		dma_addr_t src, size_t len, unsigned long flags)
{
	struct ftdmac030_chan *ftchan;
	struct ftdmac030_desc *desc;
	unsigned int shift;

	dev_dbg(chan2dev(chan), "%s(src %x, dest %x, len %x)\n",
		__func__, src, dest, len);
	ftchan = container_of(chan, struct ftdmac030_chan, common);

	if (unlikely(!len)) {
		dev_info(chan2dev(chan), "%s: length is zero!\n", __func__);
		return NULL;
	}

	/*
	 * We can be a lot more clever here, but this should take care
	 * of the most common optimization.
	 */
	if (!((src | dest | len) & 3)) {
		shift = 2;
	} else if (!((src | dest | len) & 1)) {
		shift = 1;
	} else {
		shift = 0;
	}

	desc = ftdmac030_create_chain(ftchan, NULL, src, dest, len,
		shift, 0, 0);
	if (!desc)
		goto err;

	desc->txd.flags = flags;

	return &desc->txd;

err:
	return NULL;
}

/**
 * ftdmac030_prep_slave_sg - prepare descriptors for a DMA_SLAVE transaction
 * @chan: DMA channel
 * @sgl: scatterlist to transfer to/from
 * @sg_len: number of entries in @scatterlist
 * @direction: DMA direction
 * @flags: tx descriptor status flags
 */
static struct dma_async_tx_descriptor *
ftdmac030_prep_slave_sg(struct dma_chan *chan, struct scatterlist *sgl,
		unsigned int sg_len, enum dma_data_direction direction,
		unsigned long flags)
{
	struct ftdmac030_dma_slave *slave = chan->private;
	struct ftdmac030_chan *ftchan;
	struct ftdmac030_desc *desc = NULL;
	struct scatterlist *sg;
	unsigned int shift;
	unsigned int i;

	dev_dbg(chan2dev(chan), "%s\n", __func__);
	ftchan = container_of(chan, struct ftdmac030_chan, common);

	if (unlikely(!slave)) {
		dev_err(chan2dev(chan), "%s: no slave data\n", __func__);
		return NULL;
	}

	if (unlikely(!sg_len)) {
		dev_err(chan2dev(chan), "%s: scatter list length is 0\n",
			__func__);
		return NULL;
	}

	if (unlikely(slave->common.src_addr_width !=
		     slave->common.dst_addr_width)) {
		dev_err(chan2dev(chan), "%s: data width mismatched\n",
			__func__);
		return NULL;
	}

	switch (slave->common.src_addr_width) {
	case DMA_SLAVE_BUSWIDTH_1_BYTE:
		shift = 0;
		break;
	case DMA_SLAVE_BUSWIDTH_2_BYTES:
		shift = 1;
		break;
	case DMA_SLAVE_BUSWIDTH_4_BYTES:
		shift = 2;
		break;
	default:
		dev_err(chan2dev(chan), "%s: incorrect data width\n",
			__func__);
		BUG();
	}

	switch (direction) {
	case DMA_TO_DEVICE:
		for_each_sg(sgl, sg, sg_len, i) {
			struct ftdmac030_desc *tmp;
			unsigned int len;
			dma_addr_t mem;

			mem = sg_phys(sg);
			len = sg_dma_len(sg);

			tmp = ftdmac030_create_chain(ftchan, desc,
				mem, slave->common.dst_addr, len,
				shift, 0, 1);
			if (!tmp)
				goto err;

			if (!desc)
				desc = tmp;
		}

		if (slave->handshake >= 0)
			desc->cfg = FTDMAC030_CFG_DST_HANDSHAKE_EN
				| FTDMAC030_CFG_DST_HANDSHAKE(slave->handshake);
		break;
	case DMA_FROM_DEVICE:
		for_each_sg(sgl, sg, sg_len, i) {
			struct ftdmac030_desc *tmp;
			unsigned int len;
			dma_addr_t mem;

			mem = sg_phys(sg);
			len = sg_dma_len(sg);

			tmp = ftdmac030_create_chain(ftchan, desc,
				slave->common.src_addr, mem, len,
				shift, 1, 0);
			if (!tmp)
				goto err;

			if (!desc)
				desc = tmp;
		}

		if (slave->handshake >= 0)
			desc->cfg = FTDMAC030_CFG_SRC_HANDSHAKE_EN
				| FTDMAC030_CFG_SRC_HANDSHAKE(slave->handshake);
		break;
	default:
		dev_err(chan2dev(chan), "%s: incorrect direction\n",
			__func__);
		goto err;
	}

	desc->txd.flags = flags;

	return &desc->txd;

err:
	return NULL;
}

static int ftdmac030_control(struct dma_chan *chan, enum dma_ctrl_cmd cmd,
		unsigned long arg)
{
	struct ftdmac030_chan *ftchan;

	dev_dbg(chan2dev(chan), "%s\n", __func__);
	ftchan = container_of(chan, struct ftdmac030_chan, common);

	/* Only supports DMA_TERMINATE_ALL */
	if (cmd != DMA_TERMINATE_ALL)
		return -ENXIO;

	/*
	 * This is only called when something went wrong elsewhere, so
	 * we don't really care about the data. Just disable the channel.
	 */
	spin_lock_bh(&ftchan->lock);
	ftdmac030_stop_channel(ftchan);
	ftdmac030_finish_all_chains(ftchan);
	spin_unlock_bh(&ftchan->lock);
	return 0;
}

/**
 * ftdmac030_tx_status - poll for transaction completion
 * @chan: DMA channel
 * @cookie: transaction identifier to check status of
 * @txstate: if not %NULL updated with transaction state
 *
 * If @txstate is passed in, upon return it reflect the driver
 * internal state and can be used with dma_async_is_complete() to check
 * the status of multiple cookies without re-checking hardware state.
 */
static enum dma_status
ftdmac030_tx_status(struct dma_chan *chan, dma_cookie_t cookie,
		struct dma_tx_state *txstate)
{
	struct ftdmac030_chan *ftchan;
	dma_cookie_t last_used;
	dma_cookie_t last_complete;
	enum dma_status ret;

	dev_dbg(chan2dev(chan), "%s\n", __func__);
	ftchan = container_of(chan, struct ftdmac030_chan, common);

	last_complete = ftchan->completed_cookie;
	last_used = chan->cookie;

	ret = dma_async_is_complete(cookie, last_complete, last_used);
	dma_set_tx_state(txstate, last_complete, last_used, 0);

	return ret;
}

/**
 * ftdmac030_issue_pending - try to finish work
 * @chan: target DMA channel
 */
static void ftdmac030_issue_pending(struct dma_chan *chan)
{
	/*
	 * We are posting descriptors to the hardware as soon as
	 * they are ready, so this function does nothing.
	 */
}

/******************************************************************************
 * struct platform_driver functions
 *****************************************************************************/
static int ftdmac030_probe(struct platform_device *pdev)
{
	struct resource *res;
	struct ftdmac030 *ftdmac030;
	struct dma_device *dma;
	int irq;
	int i;
	int err;

	if (pdev == NULL)
		return -ENODEV;

	if ((res = platform_get_resource(pdev, IORESOURCE_MEM, 0)) == 0) {
		return -ENXIO;
	}

	if ((irq = platform_get_irq(pdev, 0)) < 0) {
		return irq;
	}

	ftdmac030 = kzalloc(sizeof(*ftdmac030), GFP_KERNEL);
	if (!ftdmac030) {
		return -ENOMEM;
	}

	dma = &ftdmac030->dma;

	INIT_LIST_HEAD(&dma->channels);
	dma->dev = &pdev->dev;

	platform_set_drvdata(pdev, ftdmac030);
	ftdmac030->id = pdev->id;

	/* map io memory */

	ftdmac030->res = request_mem_region(res->start, res->end - res->start + 1,
			dev_name(&pdev->dev));
	if (ftdmac030->res == NULL) {
		dev_err(&pdev->dev, "Could not reserve memory region\n");
		err = -ENOMEM;
		goto err_req_mem;
	}

	ftdmac030->base = ioremap(res->start, res->end - res->start + 1);
	if (ftdmac030->base == NULL) {
		dev_err(&pdev->dev, "Failed to ioremap ethernet registers\n");
		err = -EIO;
		goto err_ioremap;
	}

	/* create a pool of consistent memory blocks for hardware descriptors */
	ftdmac030->dma_desc_pool = dma_pool_create("ftdmac030_desc_pool",
			&pdev->dev, sizeof(struct ftdmac030_desc),
			4 /* word alignment */, 0);
	if (!ftdmac030->dma_desc_pool) {
		dev_err(&pdev->dev, "No memory for descriptor pool\n");
		err = -ENOMEM;
		goto err_pool_create;
	}

	/* initialize channels */
	for (i = 0; i < CHANNEL_NR; i++) {
		struct ftdmac030_chan *ftchan = &ftdmac030->channel[i];

		INIT_LIST_HEAD(&ftchan->active_list);
		INIT_LIST_HEAD(&ftchan->free_list);
		ftchan->common.device	= dma;
		ftchan->common.cookie	= DMA_MIN_COOKIE;

		spin_lock_init(&ftchan->lock);
		ftchan->ftdmac030		= ftdmac030;
		ftchan->completed_cookie	= DMA_MIN_COOKIE;

		tasklet_init(&ftchan->tasklet, ftdmac030_tasklet,
				(unsigned long)ftchan);
		list_add_tail(&ftchan->common.device_node, &dma->channels);
	}

	/* initialize dma_device */
	dma->device_alloc_chan_resources = ftdmac030_alloc_chan_resources;
	dma->device_free_chan_resources	= ftdmac030_free_chan_resources;
	dma->device_prep_dma_memcpy	= ftdmac030_prep_dma_memcpy;
	dma->device_prep_slave_sg	= ftdmac030_prep_slave_sg;
	dma->device_control		= ftdmac030_control;
	dma->device_tx_status		= ftdmac030_tx_status;
	dma->device_issue_pending	= ftdmac030_issue_pending;

	/* set DMA capability */
	dma_cap_set(DMA_MEMCPY, dma->cap_mask);
	dma_cap_set(DMA_SLAVE, dma->cap_mask);

	err = request_irq(irq, ftdmac030_interrupt, IRQF_SHARED, pdev->name,
		ftdmac030);
	if (err) {
		dev_err(&pdev->dev, "failed to request irq %d\n", irq);
		goto err_req_irq;
	}

	ftdmac030->irq = irq;

	err = dma_async_device_register(dma);
	if (err) {
		dev_err(&pdev->dev, "failed to register dma device\n");
		goto err_register;
	}

	dev_info(&pdev->dev,
		"DMA engine driver: irq %d, mapped at %p\n",
		irq, ftdmac030->base);

	return 0;

err_register:
	free_irq(irq, ftdmac030);
err_req_irq:
	dma_pool_destroy(ftdmac030->dma_desc_pool);
err_pool_create:
	for (i = 0; i < CHANNEL_NR; i++) {
		struct ftdmac030_chan *ftchan = &ftdmac030->channel[i];

		list_del(&ftchan->common.device_node);
		tasklet_kill(&ftchan->tasklet);
	}

	iounmap(ftdmac030->base);
err_ioremap:
	release_resource(ftdmac030->res);
err_req_mem:
	platform_set_drvdata(pdev, NULL);
	kfree(ftdmac030);
	return err;
}

static int __exit ftdmac030_remove(struct platform_device *pdev)
{
	struct ftdmac030 *ftdmac030;
	int i;

	ftdmac030 = platform_get_drvdata(pdev);

	dma_async_device_unregister(&ftdmac030->dma);

	free_irq(ftdmac030->irq, ftdmac030);
	dma_pool_destroy(ftdmac030->dma_desc_pool);

	for (i = 0; i < CHANNEL_NR; i++) {
		struct ftdmac030_chan *ftchan = &ftdmac030->channel[i];

		tasklet_disable(&ftchan->tasklet);
		tasklet_kill(&ftchan->tasklet);
		list_del(&ftchan->common.device_node);
	}

	iounmap(ftdmac030->base);
	release_resource(ftdmac030->res);

	platform_set_drvdata(pdev, NULL);
	kfree(ftdmac030);
	return 0;
}

static struct platform_driver ftdmac030_driver = {
	.probe		= ftdmac030_probe,
	.remove		= __exit_p(ftdmac030_remove),
	.driver		= {
		.name	= DRV_NAME,
		.owner	= THIS_MODULE,
	},
};

/******************************************************************************
 * initialization / finalization
 *****************************************************************************/
static int __init ftdmac030_init(void)
{
	return platform_driver_register(&ftdmac030_driver);
}

static void __exit ftdmac030_exit(void)
{
	platform_driver_unregister(&ftdmac030_driver);
}

module_init(ftdmac030_init);
module_exit(ftdmac030_exit);

MODULE_AUTHOR("Po-Yu Chuang <ratbert@faraday-tech.com>");
MODULE_DESCRIPTION("FTDMAC030 DMA engine driver");
MODULE_LICENSE("GPL");
