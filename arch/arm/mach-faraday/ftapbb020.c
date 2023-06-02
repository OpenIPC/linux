/*
 * Faraday FTAPBB020 DMA engine driver
 *
 * (C) Copyright 2010 Faraday Technology
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
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/slab.h>

#include <asm/io.h>

#include <mach/ftapbb020.h>

#define DRV_NAME	"ftapbb020"
#define CHANNEL_NR	4

/*
 * This value must be multiple of 32-bits to prevent from changing the
 * alignment of child descriptors.
 */
#define MAX_CYCLE_PER_BLOCK		0x00800000
#if (MAX_CYCLE_PER_BLOCK > FTAPBB020_CYC_MASK) || (MAX_CYCLE_PER_BLOCK & 0x3)
#error invalid MAX_CYCLE_PER_BLOCK
#endif

/*
 * Initial number of descriptors to allocate for each channel. This could
 * be increased during dma usage.
 */
static unsigned int init_nr_desc_per_channel = 64;
module_param(init_nr_desc_per_channel, uint, 0644);
MODULE_PARM_DESC(init_nr_desc_per_channel,
		 "initial descriptors per channel (default: 64)");

/**
 * struct ftapbb020_desc - async transaction descriptor.
 * @txd: support for the async_tx api
 * @node: node on the descriptors list
 * @child_list: list for transfer chain
 * @ftchan: the channel which this descriptor belongs to
 * @cmd: command register content
 * @src: source physical address
 * @dst: destination physical addr
 * @len: length in bytes
 */
struct ftapbb020_desc {
	struct dma_async_tx_descriptor txd;
	struct list_head node;
	struct list_head child_list;
	struct ftapbb020_chan *ftchan;
	unsigned int cmd;
	dma_addr_t src;
	dma_addr_t dst;
	unsigned int cycle;
	size_t len;
};

/**
 * struct ftapbb020_chan - internal representation of an ftapbb020 channel.
 * @common: common dmaengine channel object
 * @active_list: list of descriptors dmaengine is being running on
 * @pending_list: list of descriptors dmaengine which is wating to run
 * @free_list: list of descriptors usable by the channel
 * @tasklet: bottom half to finish transaction work
 * @lock: serializes enqueue/dequeue operations to descriptors lists
 * @ftapbb020: parent device
 * @completed_cookie: identifier for the most recently completed operation
 * @descs_allocated: number of allocated descriptors
 */
struct ftapbb020_chan {
	struct dma_chan	common;
	struct list_head active_list;
	struct list_head pending_list;
	struct list_head free_list;
	struct tasklet_struct tasklet;
	spinlock_t lock;
	struct ftapbb020 *ftapbb020;
	dma_cookie_t completed_cookie;
	int descs_allocated;
};

/**
 * struct ftapbb020 - internal representation of an ftapbb020 device
 * @dma: common dmaengine dma_device object
 * @res: io memory resource of hardware registers
 * @base: virtual address of hardware register base
 * @irq: irq number
 * @channel: channel table
 */
struct ftapbb020 {
	struct dma_device dma;
	struct resource *res;
	void __iomem *base;
	unsigned int irq;
	struct ftapbb020_chan channel[CHANNEL_NR];
};

static dma_cookie_t ftapbb020_tx_submit(struct dma_async_tx_descriptor *);

/******************************************************************************
 * internal functions
 *****************************************************************************/
static inline struct device *chan2dev(struct dma_chan *chan)
{
	return &chan->dev->device;
}

static void ftapbb020_stop_channel(struct ftapbb020_chan *ftchan)
{
	void __iomem *base = ftchan->ftapbb020->base;
	int chan_id = ftchan->common.chan_id;

	iowrite32(0, base + FTAPBB020_OFFSET_CMD(chan_id));
}

static void ftapbb020_stop_all_channels(struct ftapbb020 *ftapbb020)
{
	void __iomem *base = ftapbb020->base;
	int i;

	for (i = 0; i < CHANNEL_NR; i++)
		iowrite32(0, base + FTAPBB020_OFFSET_CMD(i));
}

static int ftapbb020_chan_is_enabled(struct ftapbb020_chan *ftchan)
{
	void __iomem *base = ftchan->ftapbb020->base;
	int chan_id = ftchan->common.chan_id;
	unsigned int cmd;

	cmd = ioread32(base + FTAPBB020_OFFSET_CMD(chan_id));
	return cmd & FTAPBB020_CMD_ENABLE;
}

/**
 * ftapbb020_alloc_desc - allocate and initialize descriptor
 * @ftchan: the channel to allocate descriptor for
 * @gfp_flags: GFP allocation flags
 */
static struct ftapbb020_desc *ftapbb020_alloc_desc(
		struct ftapbb020_chan *ftchan, gfp_t gfp_flags)
{
	struct dma_chan *chan = &ftchan->common;
	struct ftapbb020_desc *desc;

	dev_dbg(chan2dev(chan), "%s\n", __func__);
	desc = kzalloc(sizeof(*desc), gfp_flags);
	if (desc) {
		/* initialize dma_async_tx_descriptor fields */
		dma_async_tx_descriptor_init(&desc->txd, &ftchan->common);
		desc->txd.tx_submit = ftapbb020_tx_submit;

		INIT_LIST_HEAD(&desc->child_list);
		desc->ftchan = ftchan;
	}

	return desc;
}

static void ftapbb020_free_desc(struct ftapbb020_desc *desc)
{
	struct dma_chan *chan = &desc->ftchan->common;

	dev_dbg(chan2dev(chan), "%s(%p)\n", __func__, desc);
	kfree(desc);
}

/**
 * ftapbb020_desc_get - get an unused descriptor from free list
 * @ftchan: channel we want a new descriptor for
 */
static struct ftapbb020_desc *ftapbb020_desc_get(struct ftapbb020_chan *ftchan)
{
	struct dma_chan *chan = &ftchan->common;
	struct ftapbb020_desc *desc = NULL;

	spin_lock_bh(&ftchan->lock);
	if (!list_empty(&ftchan->free_list)) {
		desc = list_first_entry(&ftchan->free_list,
			struct ftapbb020_desc, node);

		list_del(&desc->node);
	}
	spin_unlock_bh(&ftchan->lock);

	/* no more descriptor available in initial pool: create one more */
	if (!desc) {
		desc = ftapbb020_alloc_desc(ftchan, GFP_ATOMIC);
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
 * ftapbb020_desc_put - move a descriptor, including any children, to the free list
 * @ftchan: channel we work on
 * @desc: descriptor, at the head of a chain, to move to free list
 */
static void ftapbb020_desc_put(struct ftapbb020_desc *desc)
{
	struct ftapbb020_chan *ftchan = desc->ftchan;
	struct dma_chan *chan = &ftchan->common;
	struct ftapbb020_desc *child;

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

static void ftapbb020_unmap_desc(struct ftapbb020_desc *desc)
{
	struct ftapbb020_chan *ftchan = desc->ftchan;
	struct dma_chan *chan = &ftchan->common;
	enum dma_ctrl_flags flags = desc->txd.flags;
	struct device *parent = ftchan->common.dev->device.parent;

	dev_dbg(chan2dev(chan), "%s(%p)\n", __func__, desc);
	if (ftchan->common.private)
		return;

	if (!(flags & DMA_COMPL_SKIP_DEST_UNMAP)) {
		if (flags & DMA_COMPL_DEST_UNMAP_SINGLE) {
			dma_unmap_single(parent, desc->dst, desc->len,
				DMA_FROM_DEVICE);
		} else {
			dma_unmap_page(parent, desc->dst, desc->len,
				DMA_FROM_DEVICE);
		}
	}

	if (!(flags & DMA_COMPL_SKIP_SRC_UNMAP)) {
		if (flags & DMA_COMPL_SRC_UNMAP_SINGLE) {
			dma_unmap_single(parent, desc->src, desc->len,
				DMA_TO_DEVICE);
		} else {
			dma_unmap_page(parent, desc->src, desc->len,
				DMA_TO_DEVICE);
		}
	}
}

static void ftapbb020_remove_chain(struct ftapbb020_desc *desc)
{
	struct ftapbb020_chan *ftchan = desc->ftchan;
	struct dma_chan *chan = &ftchan->common;
	struct ftapbb020_desc *child;
	struct ftapbb020_desc *tmp;

	dev_dbg(chan2dev(chan), "%s(%p)\n", __func__, desc);
	list_for_each_entry_safe(child, tmp, &desc->node, node) {
		dev_dbg(chan2dev(chan), "removing child desc %p\n", child);
		list_del(&child->node);
		ftapbb020_unmap_desc(child);
		ftapbb020_desc_put(child);
	}

	ftapbb020_unmap_desc(desc);
	ftapbb020_desc_put(desc);
}

/**
 * ftapbb020_create_chain - create a DMA transfer chain
 * @ftchan: the channel to allocate descriptor for
 * @first: first descriptor of a transfer chain if any
 * @src: physical source address
 * @dest: phyical destination address
 * @len: length in bytes
 * @shift: shift value for width (0: byte, 1: halfword, 2: word)
 * @fixed_src: source address is fixed (device register)
 * @fixed_dest: destination address is fixed (device register)
 * @cmd: command register content
 */
static struct ftapbb020_desc *ftapbb020_create_chain(
		struct ftapbb020_chan *ftchan,
		struct ftapbb020_desc *first,
		dma_addr_t src, dma_addr_t dest, size_t len,
		unsigned int shift, int fixed_src, int fixed_dest,
		unsigned int cmd)
{
	struct dma_chan *chan = &ftchan->common;
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

	for (offset = 0; offset < len; offset += cycle << shift) {
		struct ftapbb020_desc *desc;

		cycle = min_t(size_t, (len - offset) >> shift,
				MAX_CYCLE_PER_BLOCK);

		cycle &= FTAPBB020_CYC_MASK;

		desc = ftapbb020_desc_get(ftchan);
		if (!desc)
			goto err;

		if (fixed_src)
			desc->src = src;
		else
			desc->src = src + offset;

		if (fixed_dest)
			desc->dst = dest;
		else
			desc->dst = dest + offset;

		desc->cmd = cmd;
		desc->len = cycle << shift;
		desc->cycle = cycle;

		if (!first) {
			first = desc;
		} else {
			/* insert the link descriptor to the transfer chain */
			list_add_tail(&desc->node, &first->child_list);
		}
	}

	return first;
err:
	if (first)
		ftapbb020_remove_chain(first);
	return NULL;
}

static void ftapbb020_start_desc(struct ftapbb020_desc *desc)
{
	struct ftapbb020_chan *ftchan = desc->ftchan;
	struct dma_chan *chan = &ftchan->common;
	unsigned int cmd = desc->cmd;
	void __iomem *base = ftchan->ftapbb020->base;
	int chan_id = ftchan->common.chan_id;

	dev_dbg(chan2dev(chan), "%s(%p)\n", __func__, desc);
	cmd |= FTAPBB020_CMD_ENABLE
	    |  FTAPBB020_CMD_FININT_E
	    |  FTAPBB020_CMD_ERRINT_E;

	dev_dbg(chan2dev(chan), "\t[SAR %d] = %x\n", chan_id, desc->src);
	iowrite32(desc->src, base + FTAPBB020_OFFSET_SAR(chan_id));
	dev_dbg(chan2dev(chan), "\t[DAR %d] = %x\n", chan_id, desc->dst);
	iowrite32(desc->dst, base + FTAPBB020_OFFSET_DAR(chan_id));
	dev_dbg(chan2dev(chan), "\t[CYC %d] = %x\n", chan_id, desc->cycle);
	iowrite32(desc->cycle, base + FTAPBB020_OFFSET_CYC(chan_id));
	dev_dbg(chan2dev(chan), "\t[CMD %d] = %x\n", chan_id, cmd);
	iowrite32(cmd, base + FTAPBB020_OFFSET_CMD(chan_id));
}

static void ftapbb020_activate_chain(struct ftapbb020_desc *desc)
{
	struct ftapbb020_chan *ftchan = desc->ftchan;
	struct dma_chan *chan = &ftchan->common;

	dev_dbg(chan2dev(chan), "%s\n", __func__);
	/* move new transfer chain to active list */
	list_add_tail(&desc->node, &ftchan->active_list);
	list_splice_tail_init(&desc->child_list, &ftchan->active_list);

	/* start first descriptor */
	ftapbb020_start_desc(desc);
}

static void ftapbb020_start_next_chain(struct ftapbb020_chan *ftchan)
{
	struct dma_chan *chan = &ftchan->common;
	struct ftapbb020_desc *new;

	dev_dbg(chan2dev(chan), "%s\n", __func__);
	if (list_empty(&ftchan->pending_list))
		return;

	new = list_first_entry(&ftchan->pending_list, struct ftapbb020_desc,
		node);
	list_del(&new->node);

	ftapbb020_activate_chain(new);
}

static void ftapbb020_invoke_callback(struct ftapbb020_desc *desc)
{
	struct ftapbb020_chan *ftchan = desc->ftchan;
	struct dma_chan *chan = &ftchan->common;

	dev_dbg(chan2dev(chan), "%s(%p)\n", __func__, desc);
	ftchan->completed_cookie = desc->txd.cookie;

	/*
	 * The API requires that no submissions are done from a
	 * callback, so we don't need to drop the lock here
	 */
	if (desc->txd.callback)
		desc->txd.callback(desc->txd.callback_param);
}

static void ftapbb020_finish_all_pending_chains(struct ftapbb020_chan *ftchan)
{
	struct dma_chan *chan = &ftchan->common;
	struct ftapbb020_desc *desc;
	struct ftapbb020_desc *tmp;

	dev_dbg(chan2dev(chan), "%s\n", __func__);
	list_for_each_entry_safe(desc, tmp, &ftchan->pending_list, node) {
		list_del(&desc->node);
		ftapbb020_invoke_callback(desc);
		ftapbb020_remove_chain(desc);
	}
}

static void ftapbb020_finish_active_chain(struct ftapbb020_chan *ftchan)
{
	struct dma_chan *chan = &ftchan->common;
	struct ftapbb020_desc *desc;
	struct ftapbb020_desc *tmp;

	dev_dbg(chan2dev(chan), "%s\n", __func__);
	if (list_empty(&ftchan->active_list))
		return;

	desc = list_first_entry(&ftchan->active_list, struct ftapbb020_desc,
		node);
	/* first descriptor in the active list has callback fields */
	ftapbb020_invoke_callback(desc);

	list_for_each_entry_safe(desc, tmp, &ftchan->active_list, node) {
		list_del(&desc->node);
		ftapbb020_unmap_desc(desc);
		ftapbb020_desc_put(desc);
	}
}

static void ftapbb020_finish_all_chains(struct ftapbb020_chan *ftchan)
{
	struct dma_chan *chan = &ftchan->common;

	dev_dbg(chan2dev(chan), "%s\n", __func__);
	ftapbb020_finish_active_chain(ftchan);
	ftapbb020_finish_all_pending_chains(ftchan);
}

static dma_cookie_t ftapbb020_new_cookie(struct ftapbb020_chan *ftchan)
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
bool ftapbb020_chan_filter(struct dma_chan *chan, void *data)
{
	const char *drv_name = dev_driver_string(chan->device->dev);
	struct ftapbb020_dma_slave *slave = data;

	if (strncmp(DRV_NAME, drv_name, sizeof(DRV_NAME)))
		return false;

	if ((slave->channels & (1 << chan->chan_id)) == 0)
		return false;

	chan->private = slave;
	return true;
}
EXPORT_SYMBOL_GPL(ftapbb020_chan_filter);

/******************************************************************************
 * tasklet - called after we finished one chunk
 *****************************************************************************/
static void ftapbb020_tasklet(unsigned long data)
{
	struct ftapbb020_chan *ftchan = (struct ftapbb020_chan *)data;
	struct dma_chan *chan = &ftchan->common;
	struct ftapbb020_desc *desc;

	dev_dbg(chan2dev(chan), "%s\n", __func__);
	BUG_ON(list_empty(&ftchan->active_list));

	spin_lock_bh(&ftchan->lock);

	/* remove already finished descriptor */
	desc = list_first_entry(&ftchan->active_list, struct ftapbb020_desc,
		node);
	list_del(&desc->node);

	if (list_empty(&ftchan->active_list)) {
		ftapbb020_invoke_callback(desc);
		ftapbb020_start_next_chain(ftchan);
	} else {
		struct ftapbb020_desc *next;

		/* Do next descriptor in the trasnfer chain */
		next = list_first_entry(&ftchan->active_list,
			struct ftapbb020_desc, node);

		/*
		 * Always keep the txd fields in the first descriptor in the
		 * active list.
		 */
		next->txd.cookie = desc->txd.cookie;
		next->txd.flags = desc->txd.flags;
		next->txd.callback = desc->txd.callback;
		next->txd.callback_param = desc->txd.callback_param;

		ftapbb020_start_desc(next);
	}

	ftapbb020_unmap_desc(desc);
	spin_unlock_bh(&ftchan->lock);
	ftapbb020_desc_put(desc);
}

/******************************************************************************
 * interrupt handler
 *****************************************************************************/
static irqreturn_t ftapbb020_interrupt(int irq, void *dev_id)
{
	struct ftapbb020 *ftapbb020 = dev_id;
	struct device *dev = ftapbb020->dma.dev;
	irqreturn_t ret = IRQ_NONE;
	unsigned int sr;
	int i;

	sr = ioread32(ftapbb020->base + FTAPBB020_OFFSET_SR);
	if (sr & FTAPBB020_SR_BWERRINT) {
		dev_info(dev, "bufferable write error\n");
		ret = IRQ_HANDLED;
	}

	/* clear SR */
	iowrite32(sr, ftapbb020->base + FTAPBB020_OFFSET_SR);

	for (i = 0; i < CHANNEL_NR; i++) {
		struct ftapbb020_chan *ftchan = &ftapbb020->channel[i];
		struct dma_chan *chan = &ftchan->common;
		void __iomem *base = ftchan->ftapbb020->base;
		int chan_id = ftchan->common.chan_id;
		unsigned int cmd;

		/*
		 * status bits in command register?
		 * what a brain-damaged design.
		 */
		cmd = ioread32(base + FTAPBB020_OFFSET_CMD(chan_id));

		if (cmd & FTAPBB020_CMD_FININT_S) {
			tasklet_schedule(&ftchan->tasklet);
			ret = IRQ_HANDLED;
		} else if (cmd & FTAPBB020_CMD_ERRINT_S) {
			dev_err(chan2dev(chan), "error happened\n");
			ret = IRQ_HANDLED;
		}

		if (cmd & (FTAPBB020_CMD_FININT_S | FTAPBB020_CMD_ERRINT_S)) {
			/* clear interrupt status */
			cmd &= ~(FTAPBB020_CMD_FININT_S | FTAPBB020_CMD_ERRINT_S);
			iowrite32(cmd, base + FTAPBB020_OFFSET_CMD(chan_id));
		}
	}

	return ret;
}

/******************************************************************************
 * struct dma_async_tx_descriptor function
 *****************************************************************************/

/**
 * ftapbb020_tx_submit - set the prepared descriptor(s) to be executed by the engine
 * @txd: async transaction descriptor
 */
static dma_cookie_t ftapbb020_tx_submit(struct dma_async_tx_descriptor *txd)
{
	struct ftapbb020_desc *desc;
	struct ftapbb020_chan *ftchan;
	struct dma_chan *chan;
	dma_cookie_t cookie;

	desc = container_of(txd, struct ftapbb020_desc, txd);
	ftchan = desc->ftchan;
	chan = &ftchan->common;

	dev_dbg(chan2dev(chan), "%s: submit desc %p\n", __func__, desc);
	/* we support simple situation only */
	BUG_ON(!async_tx_test_ack(txd));

	spin_lock_bh(&ftchan->lock);

	cookie = ftapbb020_new_cookie(ftchan);
	ftchan->common.cookie = cookie;
	txd->cookie = cookie;

	if (list_empty(&ftchan->active_list)) {
		ftapbb020_activate_chain(desc);
	} else {
		list_add_tail(&desc->node, &ftchan->pending_list);
	}

	spin_unlock_bh(&ftchan->lock);

	return cookie;
}

/******************************************************************************
 * struct dma_device functions
 *****************************************************************************/

/**
 * ftapbb020_alloc_chan_resources - allocate resources for DMA channel
 * @chan: DMA channel
 */
static int ftapbb020_alloc_chan_resources(struct dma_chan *chan)
{
	struct ftapbb020_chan *ftchan;
	LIST_HEAD(tmp_list);
	int i;

	ftchan = container_of(chan, struct ftapbb020_chan, common);

	/* have we already been set up?
	 * reconfigure channel but no need to reallocate descriptors */
	if (!list_empty(&ftchan->free_list))
		return ftchan->descs_allocated;

	/* Allocate initial pool of descriptors */
	for (i = 0; i < init_nr_desc_per_channel; i++) {
		struct ftapbb020_desc *desc;

		desc = ftapbb020_alloc_desc(ftchan, GFP_KERNEL);
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
 * ftapbb020_free_chan_resources - free all channel resources
 * @chan: DMA channel
 */
static void ftapbb020_free_chan_resources(struct dma_chan *chan)
{
	struct ftapbb020_chan *ftchan;
	struct ftapbb020_desc *desc;
	struct ftapbb020_desc *tmp;
	struct ftapbb020 *ftapbb020;

	dev_dbg(chan2dev(chan), "%s\n", __func__);
	ftchan = container_of(chan, struct ftapbb020_chan, common);
	ftapbb020 = ftchan->ftapbb020;

	/* channel must be idle */
	BUG_ON(!list_empty(&ftchan->active_list));
	BUG_ON(!list_empty(&ftchan->pending_list));
	BUG_ON(ftapbb020_chan_is_enabled(ftchan));

	spin_lock_bh(&ftchan->lock);
	ftapbb020_stop_channel(ftchan);
	ftapbb020_finish_all_chains(ftchan);
	list_for_each_entry_safe(desc, tmp, &ftchan->free_list, node) {
		dev_dbg(chan2dev(chan), "  freeing descriptor %p\n", desc);
		list_del(&desc->node);
		/* free link descriptor */
		ftapbb020_free_desc(desc);
	}

	ftchan->descs_allocated = 0;
	spin_unlock_bh(&ftchan->lock);
}

/**
 * ftapbb020_prep_dma_memcpy - prepare a memcpy operation
 * @chan: the channel to prepare operation on
 * @dest: operation virtual destination address
 * @src: operation virtual source address
 * @len: operation length
 * @flags: tx descriptor status flags
 */
static struct dma_async_tx_descriptor *
ftapbb020_prep_dma_memcpy(struct dma_chan *chan, dma_addr_t dest,
		dma_addr_t src, size_t len, unsigned long flags)
{
	struct ftapbb020_chan *ftchan;
	struct ftapbb020_desc *desc;
	unsigned int shift;
	unsigned int cmd;

	dev_dbg(chan2dev(chan), "%s(src %x, dest %x, len %x)\n",
		__func__, src, dest, len);
	ftchan = container_of(chan, struct ftapbb020_chan, common);

	if (unlikely(!len)) {
		dev_info(chan2dev(chan), "%s: length is zero!\n", __func__);
		return NULL;
	}

	/* Assume RAM is on AHB bus. */
	cmd = FTAPBB020_CMD_DST_TYPE_AHB | FTAPBB020_CMD_SRC_TYPE_AHB;

	/*
	 * We can be a lot more clever here, but this should take care
	 * of the most common optimization.
	 */
	if (!((src | dest | len) & 3)) {
		shift = 2;
		cmd |= FTAPBB020_CMD_WIDTH_WORD
		    |  FTAPBB020_CMD_DST_MODE_WORD_INC
		    |  FTAPBB020_CMD_SRC_MODE_WORD_INC;
	} else if (!((src | dest | len) & 1)) {
		shift = 1;
		cmd |= FTAPBB020_CMD_WIDTH_HALF
		    |  FTAPBB020_CMD_DST_MODE_HALF_INC
		    |  FTAPBB020_CMD_SRC_MODE_HALF_INC;
	} else {
		shift = 0;
		cmd |= FTAPBB020_CMD_WIDTH_BYTE
		    |  FTAPBB020_CMD_DST_MODE_BYTE_INC
		    |  FTAPBB020_CMD_SRC_MODE_BYTE_INC;
	}

	desc = ftapbb020_create_chain(ftchan, NULL, src, dest, len, shift, 0, 0, cmd);
	if (!desc)
		goto err;

	desc->txd.flags = flags;
	desc->cmd = cmd;

	return &desc->txd;

err:
	return NULL;
}

/**
 * ftapbb020_prep_slave_sg - prepare descriptors for a DMA_SLAVE transaction
 * @chan: DMA channel
 * @sgl: scatterlist to transfer to/from
 * @sg_len: number of entries in @scatterlist
 * @direction: DMA direction
 * @flags: tx descriptor status flags
 */
static struct dma_async_tx_descriptor *
ftapbb020_prep_slave_sg(struct dma_chan *chan, struct scatterlist *sgl,
		unsigned int sg_len, enum dma_data_direction direction,
		unsigned long flags)
{
	struct ftapbb020_dma_slave *slave = chan->private;
	struct ftapbb020_chan *ftchan;
	struct ftapbb020_desc *desc = NULL;
	struct scatterlist *sg;
	unsigned int cmd;
	unsigned int shift;
	unsigned int i;

	dev_dbg(chan2dev(chan), "%s\n", __func__);
	ftchan = container_of(chan, struct ftapbb020_chan, common);

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

	switch (direction) {
	case DMA_TO_DEVICE:
		cmd = FTAPBB020_CMD_DST_MODE_FIXED
		    | FTAPBB020_CMD_DST_HANDSHAKE(slave->handshake);

		/* Assume RAM is on AHB bus. */
		cmd |= FTAPBB020_CMD_SRC_TYPE_AHB;

		switch (slave->type) {
		case FTAPBB020_BUS_TYPE_AHB:
			cmd |= FTAPBB020_CMD_DST_TYPE_AHB;
			break;
		case FTAPBB020_BUS_TYPE_APB:
			break;
		default:
			dev_err(chan2dev(chan), "%s: incorrect bus type\n",
				__func__);
			BUG();
		}

		switch (slave->common.src_addr_width) {
		case DMA_SLAVE_BUSWIDTH_1_BYTE:
			shift = 0;
			cmd |= FTAPBB020_CMD_WIDTH_BYTE
			    |  FTAPBB020_CMD_SRC_MODE_BYTE_INC;
			break;
		case DMA_SLAVE_BUSWIDTH_2_BYTES:
			shift = 1;
			cmd |= FTAPBB020_CMD_WIDTH_HALF
			    |  FTAPBB020_CMD_SRC_MODE_HALF_INC;
			break;
		case DMA_SLAVE_BUSWIDTH_4_BYTES:
			shift = 2;
			cmd |= FTAPBB020_CMD_WIDTH_WORD
			    |  FTAPBB020_CMD_SRC_MODE_WORD_INC;
			break;
		default:
			dev_err(chan2dev(chan), "%s: incorrect data width\n",
				__func__);
			BUG();
		}

		for_each_sg(sgl, sg, sg_len, i) {
			struct ftapbb020_desc *tmp;
			unsigned int len;
			dma_addr_t mem;

			mem = sg_phys(sg);
			len = sg_dma_len(sg);

			tmp = ftapbb020_create_chain(ftchan, desc,
				mem, slave->common.dst_addr, len,
				shift, 0, 1, cmd);
			if (!tmp)
				goto err;

			if (!desc)
				desc = tmp;
		}
		break;
	case DMA_FROM_DEVICE:
		cmd = FTAPBB020_CMD_SRC_MODE_FIXED
		    | FTAPBB020_CMD_SRC_HANDSHAKE(slave->handshake);

		/* Assume RAM is on AHB bus. */
		cmd |= FTAPBB020_CMD_DST_TYPE_AHB;

		switch (slave->type) {
		case FTAPBB020_BUS_TYPE_AHB:
			cmd |= FTAPBB020_CMD_SRC_TYPE_AHB;
			break;
		case FTAPBB020_BUS_TYPE_APB:
			break;
		default:
			dev_err(chan2dev(chan), "%s: incorrect bus type\n",
				__func__);
			BUG();
		}

		switch (slave->common.src_addr_width) {
		case DMA_SLAVE_BUSWIDTH_1_BYTE:
			shift = 0;
			cmd |= FTAPBB020_CMD_WIDTH_BYTE
			    |  FTAPBB020_CMD_DST_MODE_BYTE_INC;
			break;
		case DMA_SLAVE_BUSWIDTH_2_BYTES:
			shift = 1;
			cmd |= FTAPBB020_CMD_WIDTH_HALF
			    |  FTAPBB020_CMD_DST_MODE_HALF_INC;
			break;
		case DMA_SLAVE_BUSWIDTH_4_BYTES:
			shift = 2;
			cmd |= FTAPBB020_CMD_WIDTH_WORD
			    |  FTAPBB020_CMD_DST_MODE_WORD_INC;
			break;
		default:
			dev_err(chan2dev(chan), "%s: incorrect data width\n",
				__func__);
			BUG();
		}

		for_each_sg(sgl, sg, sg_len, i) {
			struct ftapbb020_desc *tmp;
			unsigned int len;
			dma_addr_t mem;

			mem = sg_phys(sg);
			len = sg_dma_len(sg);

			tmp = ftapbb020_create_chain(ftchan, desc,
				slave->common.src_addr, mem, len,
				shift, 1, 0, cmd);
			if (!tmp)
				goto err;

			if (!desc)
				desc = tmp;
		}
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

static int ftapbb020_control(struct dma_chan *chan, enum dma_ctrl_cmd cmd,
		unsigned long arg)
{
	struct ftapbb020_chan *ftchan;

	dev_dbg(chan2dev(chan), "%s\n", __func__);
	ftchan = container_of(chan, struct ftapbb020_chan, common);

	/* Only supports DMA_TERMINATE_ALL */
	if (cmd != DMA_TERMINATE_ALL)
		return -ENXIO;

	/*
	 * This is only called when something went wrong elsewhere, so
	 * we don't really care about the data. Just disable the channel.
	 */
	spin_lock_bh(&ftchan->lock);
	ftapbb020_stop_channel(ftchan);
	ftapbb020_finish_all_chains(ftchan);
	spin_unlock_bh(&ftchan->lock);
	return 0;
}

/**
 * ftapbb020_tx_status - poll for transaction completion
 * @chan: DMA channel
 * @cookie: transaction identifier to check status of
 * @txstate: if not %NULL updated with transaction state
 *
 * If @txstate is passed in, upon return it reflect the driver
 * internal state and can be used with dma_async_is_complete() to check
 * the status of multiple cookies without re-checking hardware state.
 */
static enum dma_status
ftapbb020_tx_status(struct dma_chan *chan, dma_cookie_t cookie,
		struct dma_tx_state *txstate)
{
	struct ftapbb020_chan *ftchan;
	dma_cookie_t last_used;
	dma_cookie_t last_complete;
	enum dma_status ret;

	dev_dbg(chan2dev(chan), "%s\n", __func__);
	ftchan = container_of(chan, struct ftapbb020_chan, common);

	last_complete = ftchan->completed_cookie;
	last_used = chan->cookie;

	ret = dma_async_is_complete(cookie, last_complete, last_used);
	dma_set_tx_state(txstate, last_complete, last_used, 0);

	return ret;
}

/**
 * ftapbb020_issue_pending - try to finish work
 * @chan: target DMA channel
 */
static void ftapbb020_issue_pending(struct dma_chan *chan)
{
	struct ftapbb020_chan *ftchan;

	dev_dbg(chan2dev(chan), "%s\n", __func__);
	ftchan = container_of(chan, struct ftapbb020_chan, common);

	if (list_empty(&ftchan->active_list)) {
		spin_lock_bh(&ftchan->lock);
		ftapbb020_start_next_chain(ftchan);
		spin_unlock_bh(&ftchan->lock);
	}
}

/******************************************************************************
 * struct platform_driver functions
 *****************************************************************************/
static int ftapbb020_probe(struct platform_device *pdev)
{
	struct resource *res;
	struct ftapbb020 *ftapbb020;
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

	ftapbb020 = kzalloc(sizeof(*ftapbb020), GFP_KERNEL);
	if (!ftapbb020) {
		return -ENOMEM;
	}

	dma = &ftapbb020->dma;

	INIT_LIST_HEAD(&dma->channels);
	dma->dev = &pdev->dev;

	platform_set_drvdata(pdev, ftapbb020);

	/* map io memory */

	ftapbb020->res = request_mem_region(res->start, res->end - res->start + 1,
			dev_name(&pdev->dev));
	if (ftapbb020->res == NULL) {
		dev_err(&pdev->dev, "Could not reserve memory region\n");
		err = -ENOMEM;
		goto err_req_mem;
	}

	ftapbb020->base = ioremap(res->start, res->end - res->start + 1);
	if (ftapbb020->base == NULL) {
		dev_err(&pdev->dev, "Failed to ioremap ethernet registers\n");
		err = -EIO;
		goto err_ioremap;
	}

	/* force dma off, just in case */
	ftapbb020_stop_all_channels(ftapbb020);

	/* initialize channels */
	for (i = 0; i < CHANNEL_NR; i++) {
		struct ftapbb020_chan *ftchan = &ftapbb020->channel[i];

		INIT_LIST_HEAD(&ftchan->active_list);
		INIT_LIST_HEAD(&ftchan->pending_list);
		INIT_LIST_HEAD(&ftchan->free_list);
		ftchan->common.device	= dma;
		ftchan->common.cookie	= DMA_MIN_COOKIE;

		spin_lock_init(&ftchan->lock);
		ftchan->ftapbb020		= ftapbb020;
		ftchan->completed_cookie	= DMA_MIN_COOKIE;

		tasklet_init(&ftchan->tasklet, ftapbb020_tasklet,
				(unsigned long)ftchan);

		list_add_tail(&ftchan->common.device_node, &dma->channels);
	}

	/* initialize dma_device */
	dma->device_alloc_chan_resources	= ftapbb020_alloc_chan_resources;
	dma->device_free_chan_resources	= ftapbb020_free_chan_resources;
	dma->device_prep_dma_memcpy	= ftapbb020_prep_dma_memcpy;
	dma->device_prep_slave_sg	= ftapbb020_prep_slave_sg;
	dma->device_control		= ftapbb020_control;
	dma->device_tx_status		= ftapbb020_tx_status;
	dma->device_issue_pending	= ftapbb020_issue_pending;

	/* set DMA capability */
	dma_cap_set(DMA_MEMCPY, dma->cap_mask);
	dma_cap_set(DMA_SLAVE, dma->cap_mask);

	err = request_irq(irq, ftapbb020_interrupt, IRQF_SHARED, pdev->name,
		ftapbb020);
	if (err) {
		dev_err(&pdev->dev, "failed to request irq %d\n", irq);
		goto err_irq;
	}

	ftapbb020->irq = irq;

	err = dma_async_device_register(dma);
	if (err) {
		dev_err(&pdev->dev, "failed to register dma device\n");
		goto err_register;
	}

	dev_info(&pdev->dev, "DMA engine driver: irq %d, mapped at %p\n",
		irq, ftapbb020->base);

	return 0;

err_register:
	free_irq(irq, ftapbb020);
err_irq:
	for (i = 0; i < CHANNEL_NR; i++) {
		struct ftapbb020_chan *ftchan = &ftapbb020->channel[i];

		list_del(&ftchan->common.device_node);
		tasklet_kill(&ftchan->tasklet);
	}

	iounmap(ftapbb020->base);
err_ioremap:
	release_resource(ftapbb020->res);
err_req_mem:
	platform_set_drvdata(pdev, NULL);
	kfree(ftapbb020);
	return err;
}

static int __exit ftapbb020_remove(struct platform_device *pdev)
{
	struct ftapbb020 *ftapbb020;
	int i;

	ftapbb020 = platform_get_drvdata(pdev);

	dma_async_device_unregister(&ftapbb020->dma);

	free_irq(ftapbb020->irq, ftapbb020);
	for (i = 0; i < CHANNEL_NR; i++) {
		struct ftapbb020_chan *ftchan = &ftapbb020->channel[i];

		tasklet_disable(&ftchan->tasklet);
		tasklet_kill(&ftchan->tasklet);
		list_del(&ftchan->common.device_node);
	}

	iounmap(ftapbb020->base);
	release_resource(ftapbb020->res);

	platform_set_drvdata(pdev, NULL);
	kfree(ftapbb020);
	return 0;
}

static struct platform_driver ftapbb020_driver = {
	.probe		= ftapbb020_probe,
	.remove		= __exit_p(ftapbb020_remove),
	.driver		= {
		.name	= DRV_NAME,
		.owner	= THIS_MODULE,
	},
};

/******************************************************************************
 * initialization / finalization
 *****************************************************************************/
static int __init ftapbb020_init(void)
{
	return platform_driver_register(&ftapbb020_driver);
}

static void __exit ftapbb020_exit(void)
{
	platform_driver_unregister(&ftapbb020_driver);
}

module_init(ftapbb020_init);
module_exit(ftapbb020_exit);

MODULE_AUTHOR("Po-Yu Chuang <ratbert@faraday-tech.com>");
MODULE_DESCRIPTION("FTAPBB020 DMA engine driver");
MODULE_LICENSE("GPL");
