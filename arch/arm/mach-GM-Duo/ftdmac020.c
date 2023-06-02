#include <linux/dma-mapping.h>
#include <linux/dmaengine.h>
#include <linux/dmapool.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
//#include <asm/io.h>
#include <mach/ftdmac020.h>

#define CHANNEL_NR	8
#define MAX_CYCLE_PER_BLOCK	0x3FF000    //3.xM, TOT_SIZE[21:0]
//#define DEBUG
//#define REG_DUMP

#define USE_IRQ_LOCK    /* SSP ALSA will call these functions while disabling irq */

#ifdef USE_IRQ_LOCK
#define DMAC020_LOCK(x)     spin_lock_irqsave(x, flags)
#define DMAC020_UNLOCK(x)   spin_unlock_irqrestore(x, flags)
#else
#define DMAC020_LOCK(x)     spin_lock_bh(x)
#define DMAC020_UNLOCK(x)   spin_unlock_bh(x)
#endif

/* platform dependent */
static int (*platform_chan_filter)(int chan_id) = NULL;

/*
 * Initial number of descriptors to allocate for each channel. This could
 * be increased during dma usage.
 */
static unsigned int init_nr_desc_per_channel = 24;

/*
 * driver/dma/at_hdmac.c can be the template
 */
/**
 * struct ftdmac020_desc - async transaction descriptor.
 * @lld: hardware descriptor, MUST be the first member
 * @value: value for MEMSET (Must be fixed here)
 * @ccr: value for channel control register
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
 *
 * For a chained transfer, software has to set channel-specific hardware
 * registers to describe the first block and link list descriptors to describe
 * other blocks. Because of this unbelievable brain-damaged design, the
 * software descriptor and driver became unnecessarily complex.
 *
 * Sure, we can reuse src, dst, next and cycle in lld to save some space.
 * The reason I do not do that is not to make you confused.
 */
struct ftdmac020_desc {
	/* not used by the first block */
	struct ftdmac020_lld lld;

	/* used only by the first block. This is per channel configuration */
	unsigned int value; /* for MEMSET */
	unsigned int ccr;   /* Cn_CSR */
	unsigned int cfg;   /* Cn_CFG */
	dma_addr_t src;     /* Cn_SrcAddr */
	dma_addr_t dst;     /* Cn_DstAddr */
	dma_addr_t next;    /* Cn_LLP */
	unsigned int cycle; /* Cn_SIZE */

	struct dma_async_tx_descriptor txd;
	struct list_head child_list;

	/* used by all blocks */
	struct ftdmac020_chan *ftchan;
	struct list_head node;
	size_t len;
};

/**
 * struct ftdmac020_chan - internal representation of an ftdmac020 channel.
 * @common: common dmaengine channel object
 * @active_list: list of descriptors dmaengine is being running on
 * @free_list: list of descriptors usable by the channel
 * @tasklet: bottom half to finish transaction work
 * @lock: serializes enqueue/dequeue operations to descriptors lists
 * @ftdmac020: parent device
 * @completed_cookie: identifier for the most recently completed operation
 * @descs_allocated: number of allocated descriptors
 */
struct ftdmac020_chan {
	struct dma_chan common;
	struct list_head active_list;
	struct list_head free_list;
	struct tasklet_struct tasklet;
	spinlock_t lock;
	struct ftdmac020 *ftdmac020;
	dma_cookie_t completed_cookie;
	int descs_allocated;
	enum dma_transaction_type transaction_type;
};

/**
 * struct ftdmac020 - internal representation of an ftdmac020 device
 * @dma: common dmaengine dma_device object
 * @res: io memory resource of hardware registers
 * @base: virtual address of hardware register base
 * @id: id for each device
 * @tc_irq: terminal count irq number
 * @ea_irq: error and abort irq number
 * @channel: channel table
 */
struct ftdmac020 {
	struct dma_device dma;
	struct resource *res;
	void __iomem *base;
	int id;
	unsigned int irq;
	struct dma_pool *dma_desc_pool;
	struct ftdmac020_chan channel[CHANNEL_NR];
};

static const char *ftdmac020_name[] = {"ftdmac020", "ftdmac020"};
static dma_cookie_t ftdmac020_tx_submit(struct dma_async_tx_descriptor *);

/******************************************************************************
 * internal functions
 *****************************************************************************/
static inline struct device *chan2dev(struct dma_chan *chan)
{
	return &chan->dev->device;
}

static void ftdmac020_stop_channel(struct ftdmac020_chan *ftchan)
{
	int chan_id = ftchan->common.chan_id;
	void __iomem *base = ftchan->ftdmac020->base;
    u32 value;

    /* abort first */
    value = ioread32(base + FTDMAC020_OFFSET_CCR_CH(chan_id));
    value |= (0x1 << 15);   /* ABT */
    iowrite32(value, base + FTDMAC020_OFFSET_CCR_CH(chan_id));
    udelay(100);
#if 0
    // clear ABT interrupt will do at ISR
    /* clear ABT interrupt, then this channel can be enabled again */
    value = (0x1 << chan_id) | (0x1 << (16 + chan_id));
    iowrite32(value, base + FTDMAC020_OFFSET_EAICR);
    // we don't need to disable dmach, because when ERR/ABT status register(0x18) was assert to 1,
    // dma020 controller will set dmach CH_EN & ABT(Cn_CSR)to 0
    /* disable dmach */
	iowrite32(0, base + FTDMAC020_OFFSET_CCR_CH(chan_id));
#endif
}

static void ftdmac020_enable(struct ftdmac020 *ftdmac020)
{
	void __iomem *base = ftdmac020->base;

	iowrite32(FTDMAC020_CR_ENABLE, base + FTDMAC020_OFFSET_CR);
}

static void ftdmac020_disable(struct ftdmac020 *ftdmac020)
{
	void __iomem *base = ftdmac020->base;

	iowrite32(0, base + FTDMAC020_OFFSET_CR);
}


static int ftdmac020_chan_is_enabled(struct ftdmac020_chan *ftchan)
{
	int chan_id = ftchan->common.chan_id;
	void __iomem *base = ftchan->ftdmac020->base;
	unsigned int enabled;

	enabled = ioread32(base + FTDMAC020_OFFSET_CH_ENABLED);
	return enabled & (1 << chan_id);
}

/**
 * ftdmac020_alloc_desc - allocate and initialize descriptor
 * @ftchan: the channel to allocate descriptor for
 * @gfp_flags: GFP allocation flags
 */
static struct ftdmac020_desc *ftdmac020_alloc_desc(
		struct ftdmac020_chan *ftchan, gfp_t gfp_flags)
{
	struct dma_chan *chan = &ftchan->common;
	struct ftdmac020 *ftdmac020 = ftchan->ftdmac020;
	struct ftdmac020_desc *desc;
	dma_addr_t phys;

	dev_dbg(chan2dev(chan), "%s\n", __func__);
	desc = dma_pool_alloc(ftdmac020->dma_desc_pool, gfp_flags, &phys);
	if (desc) {
		memset(desc, 0, sizeof(*desc));

		/* initialize dma_async_tx_descriptor fields */
		dma_async_tx_descriptor_init(&desc->txd, &ftchan->common);
		desc->txd.tx_submit = ftdmac020_tx_submit;
		desc->txd.phys = phys;

		INIT_LIST_HEAD(&desc->child_list);
		desc->ftchan = ftchan;
	}

	return desc;
}

static void ftdmac020_free_desc(struct ftdmac020_desc *desc)
{
	struct ftdmac020_chan *ftchan = desc->ftchan;
	struct dma_chan *chan = &desc->ftchan->common;
	struct ftdmac020 *ftdmac020 = ftchan->ftdmac020;

	dev_dbg(chan2dev(chan), "%s(%p)\n", __func__, desc);
	dma_pool_free(ftdmac020->dma_desc_pool, desc, desc->txd.phys);
}

/**
 * ftdmac020_desc_get - get an unused descriptor from free list
 * @ftchan: channel we want a new descriptor for
 */
static struct ftdmac020_desc *ftdmac020_desc_get(struct ftdmac020_chan *ftchan)
{
	struct dma_chan *chan = &ftchan->common;
	struct ftdmac020_desc *desc = NULL;
    unsigned long       flags;

    if (flags)  {}

	DMAC020_LOCK(&ftchan->lock);
	if (!list_empty(&ftchan->free_list)) {
		desc = list_first_entry(&ftchan->free_list,
			struct ftdmac020_desc, node);

		list_del(&desc->node);
	}
	DMAC020_UNLOCK(&ftchan->lock);

	/* no more descriptor available in initial pool: create one more */
	if (!desc) {
		desc = ftdmac020_alloc_desc(ftchan, GFP_ATOMIC);
		if (desc) {
			DMAC020_LOCK(&ftchan->lock);
			ftchan->descs_allocated++;
			DMAC020_UNLOCK(&ftchan->lock);
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
 * ftdmac020_desc_put - move a descriptor, including any children, to the free list
 * @ftchan: channel we work on
 * @desc: descriptor, at the head of a chain, to move to free list
 */
static void ftdmac020_desc_put(struct ftdmac020_desc *desc)
{
	struct ftdmac020_chan *ftchan = desc->ftchan;
	struct dma_chan *chan = &ftchan->common;
	struct ftdmac020_desc *child;
    unsigned long       flags;

    if (flags)  {}

	DMAC020_LOCK(&ftchan->lock);

	if (!list_empty(&desc->child_list)) {
		list_for_each_entry(child, &desc->child_list, node) {
			dev_dbg(chan2dev(chan),
				"moving child desc %p to freelist\n", child);
		}

		list_splice_init(&desc->child_list, &ftchan->free_list);
	}

	dev_dbg(chan2dev(chan), "moving desc %p to freelist\n", desc);
	list_add(&desc->node, &ftchan->free_list);
	DMAC020_UNLOCK(&ftchan->lock);
}

static void ftdmac020_unmap_desc(struct ftdmac020_desc *desc)
{
	struct ftdmac020_chan *ftchan = desc->ftchan;
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

static void ftdmac020_remove_chain(struct ftdmac020_desc *desc)
{
	struct ftdmac020_chan *ftchan = desc->ftchan;
	struct dma_chan *chan = &ftchan->common;
	struct ftdmac020_desc *child;
	struct ftdmac020_desc *tmp;

	dev_dbg(chan2dev(chan), "%s(%p)\n", __func__, desc);
	list_for_each_entry_safe(child, tmp, &desc->child_list, node) {
		dev_dbg(chan2dev(chan), "removing child desc %p\n", child);
		list_del(&child->node);
		ftdmac020_unmap_desc(child);
		ftdmac020_desc_put(child);
	}

	ftdmac020_unmap_desc(desc);
	ftdmac020_desc_put(desc);
}

/**
 * ftdmac020_create_chain - create a DMA transfer chain
 * @ftchan: the channel to allocate descriptor for
 * @first: first descriptor of a transfer chain if any
 * @src: physical source address
 * @dest: phyical destination address
 * @len: length in bytes
 * @src_shift: shift value for width (0: byte, 1: halfword, 2: word)
 * @dst_shift: shift value for width (0: byte, 1: halfword, 2: word)
 * @fixed_src: source address is fixed (device register)
 * @fixed_dest: destination address is fixed (device register)
 * 		when MEMSET, it's the set value
 */
static struct ftdmac020_desc *ftdmac020_create_chain(
		struct ftdmac020_chan *ftchan,
		struct ftdmac020_desc *first,
		dma_addr_t src, dma_addr_t dest, size_t len,
		unsigned int src_shift, unsigned int dst_shift, int fixed_src, int fixed_dest)
{
	struct dma_chan *chan = &ftchan->common;
	struct ftdmac020_dma_slave *slave = chan->private;
	struct ftdmac020_desc *prev = NULL;
	unsigned int ccr;
	unsigned int lld_ctrl;
	size_t offset;
	unsigned int cycle;
	bool is_memset = false;

	dev_dbg(chan2dev(chan), "%s(src %x, dest %x, len %x, src_shift %d, dst_shift %d)\n",
		__func__, src, dest, len, src_shift, dst_shift);
	if ((src_shift == 2 && ((src | len) & 3)) ||
	    (src_shift == 1 && ((src | len) & 1))) {
		dev_err(chan2dev(chan), "%s: register or src data misaligned\n",
			__func__);
		return NULL;
	}

    if ((dst_shift == 2 && ((dest | len) & 3)) ||
	    (dst_shift == 1 && ((dest | len) & 1))) {
		dev_err(chan2dev(chan), "%s: register or dst data misaligned\n",
			__func__);
		return NULL;
	}

	/* When using memset, src is set to zero */
	if (src == 0 && fixed_src)
		is_memset = true;

	if (first) {
		if (list_empty(&first->child_list))
			prev = first;
		else
			prev = list_entry(first->child_list.prev,
				struct ftdmac020_desc, node);
	}

	ccr = FTDMAC020_CCR_ENABLE
	    | ((slave->src_size & 0x7) << 16)
	    | FTDMAC020_CCR_PRIO_0
	    | FTDMAC020_CCR_FIFOTH_1;

	lld_ctrl = FTDMAC020_LLD_CTRL_FIFOTH_1;

    switch (src_shift) {
	case 2:
		ccr |= FTDMAC020_CCR_SRC_WIDTH_32;
		lld_ctrl |= FTDMAC020_LLD_CTRL_SRC_WIDTH_32;
		break;
	case 1:
		ccr |= FTDMAC020_CCR_SRC_WIDTH_16;
		lld_ctrl |= FTDMAC020_LLD_CTRL_SRC_WIDTH_16;
		break;
	case 0:
		ccr |= FTDMAC020_CCR_SRC_WIDTH_8;
		lld_ctrl |= FTDMAC020_LLD_CTRL_SRC_WIDTH_8;
		break;
	default:
		dev_err(chan2dev(chan), "%s: incorrect data width\n",
			__func__);
		BUG();
		break;
	}

	switch (dst_shift) {
	case 2:
		ccr |= FTDMAC020_CCR_DST_WIDTH_32;
		lld_ctrl |= FTDMAC020_LLD_CTRL_DST_WIDTH_32;
		break;
	case 1:
		ccr |= FTDMAC020_CCR_DST_WIDTH_16;
		lld_ctrl |= FTDMAC020_LLD_CTRL_DST_WIDTH_16;
		break;
	case 0:
		ccr |= FTDMAC020_CCR_DST_WIDTH_8;
		lld_ctrl |= FTDMAC020_LLD_CTRL_DST_WIDTH_8;
		break;
	default:
		dev_err(chan2dev(chan), "%s: incorrect data width\n",
			__func__);
		BUG();
		break;
	}

	if (slave && slave->handshake >= 0)
		ccr |= FTDMAC020_CCR_HANDSHAKE;

	if (fixed_src) {
		ccr |= FTDMAC020_CCR_SRC_FIXED;
		lld_ctrl |= FTDMAC020_LLD_CTRL_SRC_FIXED;
	} else {
		ccr |= FTDMAC020_CCR_SRC_INC;
		lld_ctrl |= FTDMAC020_LLD_CTRL_SRC_INC;
	}

	if (fixed_dest && !is_memset) {
		ccr |= FTDMAC020_CCR_DST_FIXED;
		lld_ctrl |= FTDMAC020_LLD_CTRL_DST_FIXED;
	} else {
		ccr |= FTDMAC020_CCR_DST_INC;
		lld_ctrl |= FTDMAC020_LLD_CTRL_DST_INC;
	}

	ccr |= ((slave->src_sel & 0x1) << 2);  /* SRC_SEL */
	ccr |= ((slave->dst_sel & 0x1) << 1);  /* DST_SEL */
	lld_ctrl |= ((slave->src_sel & 0x1) << 17);  /* SRC_SEL */
	lld_ctrl |= ((slave->dst_sel & 0x1) << 16);  /* DST_SEL */

	/* we must seperate the len into multiple descriptors. Because len may be a bigger one */
	for (offset = 0; offset < len; offset += cycle << src_shift) {
		struct ftdmac020_desc *desc;

		cycle = min_t(size_t, (len - offset) >> src_shift,
				MAX_CYCLE_PER_BLOCK);

		desc = ftdmac020_desc_get(ftchan);
		if (!desc)
			goto err;

		/*
		 * Importatnt: 1. fixed_dest is viewed as memset value
		 *             2. src is defined as the place of "int value" in ftdmac020_desc
		 */
		if (is_memset) {
			desc->value = fixed_dest;
			src = desc->txd.phys + sizeof(struct ftdmac020_lld);
		}

		if (fixed_src) {
			desc->src = desc->lld.src = src;
		} else {
			desc->src = desc->lld.src = src + offset;
		}

		if (fixed_dest && !is_memset) {
			desc->dst = desc->lld.dst = dest;
		} else {
			desc->dst = desc->lld.dst = dest + offset;
		}

		desc->next	= 0;
		desc->cycle	= cycle & FTDMAC020_CYC_MASK;
		desc->ccr	= ccr;
		desc->cfg	= 0;
		desc->len	= cycle << src_shift;

		desc->lld.next	= 0;
		desc->lld.cycle	= cycle & FTDMAC020_LLD_CYCLE_MASK;
		desc->lld.ctrl	= lld_ctrl;

		if (!first) {
			first = desc;
		} else {
			/*
			 * Mask terminal count interrupt for this descriptor.
			 * What an inconvenient stupid design.
			 */
			prev->ccr	|= FTDMAC020_CCR_MASK_TC;
			prev->lld.ctrl	|= FTDMAC020_LLD_CTRL_MASK_TC;

			/* hardware link list pointer */
			prev->next	= FTDMAC020_LLP_ADDR(desc->txd.phys);
			prev->lld.next	= desc->txd.phys;

			/* insert the link descriptor to the transfer chain */
			list_add_tail(&desc->node, &first->child_list);
		}
		prev = desc;
	}

	/*
	 * Here just create the chain only. When submit function is called, this desc will be added to
	 * ftchan->active_list.
	 */
	return first;
err:
	if (first)
		ftdmac020_remove_chain(first);
	return NULL;
}

static void ftdmac020_start_chain(struct ftdmac020_desc *desc)
{
	struct ftdmac020_chan *ftchan = desc->ftchan;
	struct dma_chan *chan = &ftchan->common;
	void __iomem *base = ftchan->ftdmac020->base;
	int chan_id = ftchan->common.chan_id;

	dev_dbg(chan2dev(chan), "%s(%p)\n", __func__, desc);

	/*
	 * The first transfer block is not described by hardware lld.
	 * Instead, we should fill some hardware registers.
	 * What a stupid weird design.
	 */
	dev_dbg(chan2dev(chan), "\t[SRC %d] = %x\n", chan_id, desc->src);
	iowrite32(desc->src, base + FTDMAC020_OFFSET_SRC_CH(chan_id));
	dev_dbg(chan2dev(chan), "\t[DST %d] = %x\n", chan_id, desc->dst);
	iowrite32(desc->dst, base + FTDMAC020_OFFSET_DST_CH(chan_id));
	dev_dbg(chan2dev(chan), "\t[LLP %d] = %x\n", chan_id, desc->next);
	iowrite32(desc->next, base + FTDMAC020_OFFSET_LLP_CH(chan_id));
	dev_dbg(chan2dev(chan), "\t[CYC %d] = %x\n", chan_id, desc->cycle);
	iowrite32(desc->cycle, base + FTDMAC020_OFFSET_CYC_CH(chan_id));

	/* go */
	dev_dbg(chan2dev(chan), "\t[CFG %d] = %x\n", chan_id, desc->cfg);
	iowrite32(desc->cfg, base + FTDMAC020_OFFSET_CFG_CH(chan_id));
	dev_dbg(chan2dev(chan), "\t[CCR %d] = %x\n", chan_id, desc->ccr);
	iowrite32(desc->ccr, base + FTDMAC020_OFFSET_CCR_CH(chan_id));

#ifdef REG_DUMP
	printk("\t[SRC %d] = %x\n", chan_id, desc->src);
	printk("\t[DST %d] = %x\n", chan_id, desc->dst);
	printk("\t[LLP %d] = %x\n", chan_id, desc->next);
	printk("\t[CYC %d] = %x\n", chan_id, desc->cycle);
	printk("\t[CFG %d] = %x\n", chan_id, desc->cfg);
	printk("\t[CCR %d] = %x\n", chan_id, desc->ccr);
#endif
}

static void ftdmac020_start_new_chain(struct ftdmac020_chan *ftchan)
{
	struct dma_chan *chan = &ftchan->common;
	struct ftdmac020_desc *desc;

	dev_dbg(chan2dev(chan), "%s\n", __func__);
	if (list_empty(&ftchan->active_list))
		return;

	desc = list_first_entry(&ftchan->active_list, struct ftdmac020_desc, node);
	ftdmac020_start_chain(desc);
}

static void ftdmac020_finish_chain(struct ftdmac020_desc *desc)
{
	struct ftdmac020_chan *ftchan = desc->ftchan;
	struct dma_chan *chan = &ftchan->common;
    unsigned long       flags;

    if (flags)  {}

	dev_dbg(chan2dev(chan), "%s(%p)\n", __func__, desc);
	DMAC020_LOCK(&ftchan->lock);
	ftchan->completed_cookie = desc->txd.cookie;
	DMAC020_UNLOCK(&ftchan->lock);

	/*
	 * The API requires that no submissions are done from a
	 * callback, so we don't need to drop the lock here
	 */
	if (desc->txd.callback) {
	    /* If this channel is terminated by manual, we dont need to callback */
	    if (ftchan->transaction_type != DMA_TX_TYPE_END)
		    desc->txd.callback(desc->txd.callback_param);
    }

	ftdmac020_remove_chain(desc);
}

static void ftdmac020_finish_all_chains(struct ftdmac020_chan *ftchan)
{
	struct dma_chan *chan = &ftchan->common;
	struct ftdmac020_desc *desc;
	struct ftdmac020_desc *tmp;

	dev_dbg(chan2dev(chan), "%s\n", __func__);
	list_for_each_entry_safe(desc, tmp, &ftchan->active_list, node) {
		list_del(&desc->node);
		ftdmac020_finish_chain(desc);
	}
}

static dma_cookie_t ftdmac020_new_cookie(struct ftdmac020_chan *ftchan)
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
bool ftdmac020_chan_filter(struct dma_chan *chan, void *data)
{
	const char *drv_name = dev_driver_string(chan->device->dev);
	struct ftdmac020_dma_slave *slave = data;
	const char *ftdmac020_drv_name = *(ftdmac020_name + slave->id);
	struct ftdmac020_chan *ftchan;
	struct ftdmac020 *ftdmac020;

	ftchan = container_of(chan, struct ftdmac020_chan, common);
	ftdmac020 = ftchan->ftdmac020;

	if (strncmp(ftdmac020_drv_name, drv_name, sizeof("ftdmac020")))
		return false;

	/* When you have more than 2 dma controller's, it checks id */
	if (slave->id >= 0 && slave->id != ftdmac020->id)
		return false;

    /* this is platform dependent. Some platform only can use some channels instead of all. */
    if (platform_chan_filter && platform_chan_filter((int)chan->chan_id))
        return false;

	chan->private = slave;
	return true;
}
EXPORT_SYMBOL_GPL(ftdmac020_chan_filter);

/**
 * atc_handle_cyclic - at the end of a period, run callback function
 * @atchan: channel used for cyclic operations
 *
 * Called with atchan->lock held and bh disabled
 */
static void ftdmac020_handle_cyclic(struct ftdmac020_chan *ftchan)
{
    struct ftdmac020_desc	*first = list_first_entry(&ftchan->active_list, struct ftdmac020_desc, node);
	struct dma_async_tx_descriptor	*txd = &first->txd;
	dma_async_tx_callback   callback = txd->callback;
	void			*param = txd->callback_param;
    struct dma_chan *chan = &ftchan->common;

    dev_dbg(chan2dev(chan), "%s\n", __func__);

	if (callback)
		callback(param);
}

/******************************************************************************
 * tasklet - called after we finished one chain
 *****************************************************************************/
static void ftdmac020_tasklet(unsigned long data)
{
	struct ftdmac020_chan *ftchan = (struct ftdmac020_chan *)data;
	struct dma_chan *chan = &ftchan->common;
	struct ftdmac020_desc *desc;
    unsigned long       flags;

    if (flags)  {}

    if (ftchan->transaction_type == DMA_TX_TYPE_END) {
        if (!list_empty(&ftchan->active_list))
            panic("%s, active_list should be empty due to terminate_all! \n", __func__);

        return;
    }

    if (ftchan->transaction_type == DMA_CYCLIC) {
        dev_dbg(chan2dev(chan), "%s, DMA_CYCLIC \n", __func__);
	    BUG_ON(list_empty(&ftchan->active_list));

        DMAC020_LOCK(&ftchan->lock);
        ftdmac020_handle_cyclic(ftchan);
        DMAC020_UNLOCK(&ftchan->lock);
    } else {
        /*
         * DMA_SLAVE, DMA_MEMCPY
         */
        dev_dbg(chan2dev(chan), "%s, DMA_SLAVE / MEMCPY\n", __func__);
	    BUG_ON(list_empty(&ftchan->active_list));

        DMAC020_LOCK(&ftchan->lock);

    	/* remove already finished descriptor */
    	desc = list_first_entry(&ftchan->active_list, struct ftdmac020_desc, node);
    	list_del(&desc->node);

    	/* check if there were another transfer to do */
    	ftdmac020_start_new_chain(ftchan);

    	DMAC020_UNLOCK(&ftchan->lock);
    	ftdmac020_finish_chain(desc);
    }
}

/******************************************************************************
 * interrupt handler
 *****************************************************************************/
static irqreturn_t ftdmac020_interrupt(int irq, void *dev_id)
{
	struct ftdmac020 *ftdmac020 = dev_id;
	unsigned int status;
	int i, count = 0;

	status = ioread32(ftdmac020->base + FTDMAC020_OFFSET_TCISR);
	if (!status) {
	    /* read error status */
	    status = ioread32(ftdmac020->base + FTDMAC020_OFFSET_EAISR);
    	if (!status)
    		return IRQ_NONE;

        /* check channel abort */
        if (status & 0x00ff0000) {  // if channel abort
            while(1) {
                unsigned int abort_status;
                count++;
                // read ABT status register to make sure channel has abort status
                abort_status = ioread32(ftdmac020->base + FTDMAC020_OFFSET_EARAW);
                // wait channel abort status is 1, exit
                if (status & abort_status)
                    break;

                if (count > 1000) {
                    printk("Error!! FTDMAC020 abort but abort sts bit not assert to 1\n");
                    break;
                }
            }
        }

        /* clear status */
	    iowrite32(status, ftdmac020->base + FTDMAC020_OFFSET_EAICR);

	    for (i = 0; i < CHANNEL_NR; i++) {
		struct ftdmac020_chan *ftchan = &ftdmac020->channel[i];
		struct dma_chan *chan = &ftchan->common;

		if (status & FTDMAC020_EA_ERR_CH(i))
			dev_info(chan2dev(chan), "error happened\n");

#if 0	/* treat this case as normal due to ABT action for stop */
		if (status & FTDMAC020_EA_ABT_CH(i))
			dev_dbg(chan2dev(chan), "transfer aborted\n");
#endif
	    }

	    return IRQ_HANDLED;
	}

	/* clear status */
	iowrite32(status, ftdmac020->base + FTDMAC020_OFFSET_TCICR);

	for (i = 0; i < CHANNEL_NR; i++) {
		struct ftdmac020_chan *ftchan = &ftdmac020->channel[i];
		struct dma_chan *chan = &ftchan->common;

		if (status & (1 << i)) {
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
 * ftdmac020_tx_submit - set the prepared descriptor(s) to be executed by the engine
 * @txd: async transaction descriptor
 */
static dma_cookie_t ftdmac020_tx_submit(struct dma_async_tx_descriptor *txd)
{
	struct ftdmac020_desc *desc;
	struct ftdmac020_chan *ftchan;
	struct dma_chan *chan;
	dma_cookie_t cookie;
	bool busy;
    unsigned long       flags;

    if (flags)  {}

	desc = container_of(txd, struct ftdmac020_desc, txd);
	ftchan = desc->ftchan;
	chan = &ftchan->common;

	dev_dbg(chan2dev(chan), "%s: submit desc %p\n", __func__, desc);
	/* we support simple situation only */
	BUG_ON(!async_tx_test_ack(txd));

	DMAC020_LOCK(&ftchan->lock);

	cookie = ftdmac020_new_cookie(ftchan);
	ftchan->common.cookie = cookie;
	txd->cookie = cookie;

	if (list_empty(&ftchan->active_list))
		busy = false;
	else
		busy = true;

	list_add_tail(&desc->node, &ftchan->active_list);

#if 0   /* Harry, move to issue pending. If we start the transaction here, it will have potential
         * chance to lose the LLP due to race condition between CPU and hardware.
         */
	/* start ASAP */
	if (!busy)
		ftdmac020_start_chain(desc);
#endif

	DMAC020_UNLOCK(&ftchan->lock);

	return cookie;
}

/******************************************************************************
 * struct dma_device functions
 *****************************************************************************/

/**
 * ftdmac020_alloc_chan_resources - allocate resources for DMA channel
 * @chan: DMA channel
 */
static int ftdmac020_alloc_chan_resources(struct dma_chan *chan)
{
	struct ftdmac020_chan *ftchan;
	LIST_HEAD(tmp_list);
	int i;
	unsigned long       flags;

    if (flags)  {}

	ftchan = container_of(chan, struct ftdmac020_chan, common);

	/* have we already been set up?
	 * reconfigure channel but no need to reallocate descriptors */
	if (!list_empty(&ftchan->free_list))
		return ftchan->descs_allocated;

	/* Allocate initial pool of descriptors */
	for (i = 0; i < init_nr_desc_per_channel; i++) {
		struct ftdmac020_desc *desc;

		desc = ftdmac020_alloc_desc(ftchan, GFP_KERNEL);
		if (!desc) {
			dev_err(chan2dev(chan),
				"Only %d initial descriptors\n", i);
			break;
		}
		list_add_tail(&desc->node, &tmp_list);
	}

	DMAC020_LOCK(&ftchan->lock);
	ftchan->descs_allocated = i;
	list_splice(&tmp_list, &ftchan->free_list); /* add tmp_list to free_list */
	ftchan->completed_cookie = chan->cookie = 1;
	DMAC020_UNLOCK(&ftchan->lock);

	dev_dbg(chan2dev(chan), "%s: allocated %d descriptors\n",
		__func__, ftchan->descs_allocated);

	return ftchan->descs_allocated;
}

/**
 * ftdmac020_free_chan_resources - free all channel resources
 * @chan: DMA channel
 */
static void ftdmac020_free_chan_resources(struct dma_chan *chan)
{
	struct ftdmac020_chan *ftchan;
	struct ftdmac020_desc *desc;
	struct ftdmac020_desc *tmp;
	struct ftdmac020 *ftdmac020;
    unsigned long       flags;

    if (flags)  {}

	dev_dbg(chan2dev(chan), "%s\n", __func__);
	ftchan = container_of(chan, struct ftdmac020_chan, common);
	ftdmac020 = ftchan->ftdmac020;

	/* channel must be idle */
	BUG_ON(!list_empty(&ftchan->active_list));
	BUG_ON(ftdmac020_chan_is_enabled(ftchan));

	DMAC020_LOCK(&ftchan->lock);
	ftdmac020_stop_channel(ftchan);
	ftdmac020_finish_all_chains(ftchan);
	list_for_each_entry_safe(desc, tmp, &ftchan->free_list, node) {
		dev_dbg(chan2dev(chan), "  freeing descriptor %p\n", desc);
		list_del(&desc->node);
		/* free link descriptor */
		ftdmac020_free_desc(desc);
	}

	ftchan->descs_allocated = 0;
	DMAC020_UNLOCK(&ftchan->lock);
}

/**
 * ftdmac020_prep_dma_memcpy - prepare a memcpy operation
 * @chan: the channel to prepare operation on
 * @dest: operation virtual destination address
 * @src: operation virtual source address
 * @len: operation length
 * @flags: tx descriptor status flags
 */
static struct dma_async_tx_descriptor *
ftdmac020_prep_dma_memcpy(struct dma_chan *chan, dma_addr_t dest,
		dma_addr_t src, size_t len, unsigned long flags)
{
	struct ftdmac020_chan *ftchan;
	struct ftdmac020_desc *desc;
	unsigned int shift;

	dev_dbg(chan2dev(chan), "%s(src %x, dest %x, len %x)\n",
		__func__, src, dest, len);
	ftchan = container_of(chan, struct ftdmac020_chan, common);

	if (unlikely(!len)) {
		dev_info(chan2dev(chan), "%s: length is zero!\n", __func__);
		return NULL;
	}

	if (unlikely(!pfn_valid((src >> PAGE_SHIFT)))) {
		dev_err(chan2dev(chan), "%s: incorrect src physical address 0x%x \n", __func__, src);
		return NULL;
	}

	if (unlikely(!pfn_valid((dest >> PAGE_SHIFT)))) {
		dev_err(chan2dev(chan), "%s: incorrect dest physical address 0x%x \n", __func__, dest);
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

	desc = ftdmac020_create_chain(ftchan, NULL, src, dest, len,
		shift, shift, 0, 0);
	if (!desc)
		goto err;

	desc->txd.flags = flags;
	ftchan->transaction_type = DMA_MEMCPY;

	return &desc->txd;

err:
	return NULL;
}

/**
 * ftdmac020_prep_dma_memset - prepare a memset operation
 * @chan: the channel to prepare operation on
 * @dest: operation virtual destination address
 * @value: the given value for memset used
 * @len: operation length
 * @flags: tx descriptor status flags
 */
static struct dma_async_tx_descriptor *
ftdmac020_prep_dma_memset(struct dma_chan *chan, dma_addr_t dest,
		int value, size_t len, unsigned long flags)
{
	struct ftdmac020_chan *ftchan;
	struct ftdmac020_desc *desc;
	unsigned int shift;

	dev_dbg(chan2dev(chan), "%s(dest %x, value %x, len %x)\n",
		__func__, dest, value, len);
	ftchan = container_of(chan, struct ftdmac020_chan, common);

	if (unlikely(!len)) {
		dev_info(chan2dev(chan), "%s: length is zero!\n", __func__);
		return NULL;
	}

	if (unlikely(len & 3)) {
		dev_info(chan2dev(chan), "%s: length isn't word-align!\n",
			 __func__);
		return NULL;
	}

	if (unlikely(dest & 3)) {
		dev_info(chan2dev(chan), "%s: dest addr isn't word-align!\n",
			 __func__);
		return NULL;
	}

	if (unlikely(!pfn_valid((dest >> PAGE_SHIFT)))) {
		dev_err(chan2dev(chan), "%s: incorrect dest physical address 0x%x \n", __func__, dest);
		return NULL;
	}

	shift = 2;

	desc = ftdmac020_create_chain(ftchan, NULL, 0, dest, len,
		shift, shift, 1, value);
	if (!desc)
		goto err;

	desc->txd.flags = flags;
	ftchan->transaction_type = DMA_MEMSET;

	return &desc->txd;

err:
	return NULL;
}

/**
 * atc_prep_dma_cyclic - prepare the cyclic DMA transfer
 * @chan: the DMA channel to prepare
 * @buf_addr: physical DMA address where the buffer starts
 * @buf_len: total number of bytes for the entire buffer
 * @period_len: number of bytes for each period
 * @direction: transfer direction, to or from device
 */
static struct dma_async_tx_descriptor *
ftdmac020_prep_dma_cyclic(struct dma_chan *chan, dma_addr_t buf_addr, size_t buf_len,
		size_t period_len, enum dma_transfer_direction direction)
{
	struct ftdmac020_dma_slave  *slave = chan->private;
	struct ftdmac020_chan *ftchan;
	struct ftdmac020_desc *desc = NULL, *child, *last;
	dma_addr_t  src_addr, dst_addr;
	int i, src_shift, dst_shift, periods = buf_len / period_len;

	dev_dbg(chan2dev(chan), "%s\n", __func__);
	ftchan = container_of(chan, struct ftdmac020_chan, common);

	if (unlikely(!slave)) {
		dev_err(chan2dev(chan), "%s, please call dmaengine_slave_config() first! \n", __func__);
		return NULL;
	}

	if (unlikely(!list_empty(&ftchan->active_list))) {
		dev_err(chan2dev(chan), "%s, active list is not empty! \n", __func__);
		return NULL;
	}

	if (unlikely(!buf_len || !period_len)) {
		dev_dbg(chan2dev(chan), "prep_dma_cyclic: length is zero!\n");
		return NULL;
	}

	if ((direction != DMA_MEM_TO_DEV) && (direction != DMA_DEV_TO_MEM)) {
		dev_err(chan2dev(chan), "%s: incorrect direction %d \n", __func__, direction);
		return NULL;
	}

	if (buf_len % period_len) {
		dev_err(chan2dev(chan), "%s: incorrect length %d \n", __func__, buf_len);
		return NULL;
	}

	switch (slave->common.src_addr_width) {
		case DMA_SLAVE_BUSWIDTH_1_BYTE:
			src_shift = 0;
			break;
		case DMA_SLAVE_BUSWIDTH_2_BYTES:
			src_shift = 1;
			break;
		case DMA_SLAVE_BUSWIDTH_4_BYTES:
			src_shift = 2;
			break;
		default:
			dev_err(chan2dev(chan), "%s: incorrect data width\n",
					__func__);
			return NULL;
	}

    switch (slave->common.dst_addr_width) {
		case DMA_SLAVE_BUSWIDTH_1_BYTE:
			dst_shift = 0;
			break;
		case DMA_SLAVE_BUSWIDTH_2_BYTES:
			dst_shift = 1;
			break;
		case DMA_SLAVE_BUSWIDTH_4_BYTES:
			dst_shift = 2;
			break;
		default:
			dev_err(chan2dev(chan), "%s: incorrect data width\n",
					__func__);
			return NULL;
	}

#ifdef REG_DUMP
    printk("%s, src_shift: %d, dst_shift: %d, slave->src_size: %d\n", __func__,
                    src_shift, dst_shift, slave->src_size);
#endif

	if (slave->src_size > FTDMAC020_BURST_SZ_256 || slave->src_size < FTDMAC020_BURST_SZ_1) {
		dev_err(chan2dev(chan), "%s: incorrect src_size %d\n", __func__, slave->src_size);
		return NULL;
	}

    if (direction == DMA_MEM_TO_DEV) {
        if (buf_addr != slave->common.src_addr) {
            dev_err(chan2dev(chan), "%s: inconsistent src address %x \n", __func__, buf_addr);
                      return NULL;
        }
    } else {
        if (buf_addr != slave->common.dst_addr) {
            dev_err(chan2dev(chan), "%s: inconsistent dst address %x \n", __func__, buf_addr);
                      return NULL;
        }
    }

	if (unlikely(!pfn_valid((buf_addr >> PAGE_SHIFT)))) {
		dev_err(chan2dev(chan), "%s: incorrect physical address 0x%x \n", __func__, buf_addr);
		return NULL;
	}

	if (direction != slave->common.direction) {
		dev_err(chan2dev(chan), "%s: inconsistent direction %d\n", __func__, direction);
		return NULL;
	}

	src_addr = slave->common.src_addr;
	dst_addr = slave->common.dst_addr;

	/* create chain */
	for (i = 0; i < periods; i ++) {
		struct ftdmac020_desc *tmp;
		int fixed_src, fixed_dst;

		if (direction == DMA_MEM_TO_DEV) {
			fixed_src = 0;  /* INC */
			fixed_dst = 1;
		} else {
			fixed_src = 1;
			fixed_dst = 0;
		}

		tmp = ftdmac020_create_chain(ftchan, desc,
				src_addr, dst_addr, period_len,
				src_shift, dst_shift, fixed_src, fixed_dst);

		/* increase the address */
		if (fixed_src == 0)
			src_addr += period_len;
		if (fixed_dst == 0)
			dst_addr += period_len;

		if (!tmp)
			return NULL;
		if (!desc)  /* all childs are chained to the first active node */
			desc = tmp;

		if (direction == DMA_MEM_TO_DEV) {
			if (slave->handshake >= 0)
				tmp->cfg = FTDMAC020_CFG_DST_HANDSHAKE_EN
					| FTDMAC020_CFG_DST_HANDSHAKE(slave->handshake);
		} else {
			if (slave->handshake >= 0)
				tmp->cfg = FTDMAC020_CFG_SRC_HANDSHAKE_EN
					| FTDMAC020_CFG_SRC_HANDSHAKE(slave->handshake);
		}
	}

	/* walk through the whole list and update TC_MASK to issue interrupt
	*/
	list_for_each_entry(child, &desc->child_list, node) {
		child->ccr &= ~FTDMAC020_CCR_MASK_TC;
		child->ccr |= FTDMAC020_CCR_PRIO_3; /* audio needs high priority */
		child->lld.ctrl &= ~FTDMAC020_LLD_CTRL_MASK_TC;
	}

	/* form hardware link list pointer ring */
	last = list_entry(desc->child_list.prev, struct ftdmac020_desc, node);
	last->next	= FTDMAC020_LLP_ADDR(desc->txd.phys);
	last->lld.next	= desc->txd.phys;

#ifdef REG_DUMP   //dump register
	child = desc;
	printk("(0x%x)child->ccr: 0x%x, cfg: 0x%x, src:0x%x, lld.src:0x%x, dst:0x%x, lld.dst:0x%x, lld.next:0x%x, lld.cycle:0x%x, lld.ctrl:0x%x \n",
			child->txd.phys, child->ccr, child->cfg, child->src, child->dst,
			child->lld.src, child->lld.dst, child->lld.next, child->lld.cycle, child->lld.ctrl);

	list_for_each_entry(child, &desc->child_list, node) {
		printk("(0x%x)child->ccr:0x%x, cfg:0x%x, src:0x%x, dst:0x%x, lld.src:0x%x, lld.dst:0x%x, lld.next:0x%x, lld.cycle:0x%x, lld.ctrl:0x%x \n",
				child->txd.phys, child->ccr, child->cfg, child->src, child->dst,
				child->lld.src, child->lld.dst, child->lld.next, child->lld.cycle, child->lld.ctrl);
	}
#endif

	/* Update the tc_mask to issue interrupt per element in the chain */
	desc->txd.cookie = -EBUSY;
	desc->txd.flags = DMA_PREP_INTERRUPT | DMA_CTRL_ACK | DMA_COMPL_SKIP_SRC_UNMAP | DMA_COMPL_SKIP_DEST_UNMAP;
	ftchan->transaction_type = DMA_CYCLIC;
	chan->private = NULL;   /* got the parameter already, so free it */

	return &desc->txd;
}


/**
 * ftdmac020_prep_slave_sg - prepare descriptors for a DMA_SLAVE transaction
 * @chan: DMA channel
 * @sgl: scatterlist to transfer to/from
 * @sg_len: number of entries in @scatterlist
 * @direction: DMA direction
 * @flags: tx descriptor status flags
 */
static struct dma_async_tx_descriptor *
ftdmac020_prep_slave_sg(struct dma_chan *chan, struct scatterlist *sgl,
		unsigned int sg_len, enum dma_transfer_direction direction,
		unsigned long flags)
{
	struct ftdmac020_dma_slave *slave = chan->private;
	struct ftdmac020_chan *ftchan;
	struct ftdmac020_desc *desc = NULL;
	struct scatterlist *sg;
	unsigned int src_shift, dst_shift;
	unsigned int i;

	if (unlikely(!slave)) {
		dev_err(chan2dev(chan), "%s, please call dmaengine_slave_config() first! \n", __func__);
		return NULL;
	}

	dev_dbg(chan2dev(chan), "%s\n", __func__);
	ftchan = container_of(chan, struct ftdmac020_chan, common);

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
		src_shift = 0;
		break;
	case DMA_SLAVE_BUSWIDTH_2_BYTES:
		src_shift = 1;
		break;
	case DMA_SLAVE_BUSWIDTH_4_BYTES:
		src_shift = 2;
		break;
	default:
		dev_err(chan2dev(chan), "%s: incorrect data width\n",
			__func__);
		BUG();
	}

    switch (slave->common.dst_addr_width) {
	case DMA_SLAVE_BUSWIDTH_1_BYTE:
		dst_shift = 0;
		break;
	case DMA_SLAVE_BUSWIDTH_2_BYTES:
		dst_shift = 1;
		break;
	case DMA_SLAVE_BUSWIDTH_4_BYTES:
		dst_shift = 2;
		break;
	default:
		dev_err(chan2dev(chan), "%s: incorrect data width\n",
			__func__);
		BUG();
	}

	if (slave->src_size > FTDMAC020_BURST_SZ_256 || slave->src_size < FTDMAC020_BURST_SZ_1) {
		dev_err(chan2dev(chan), "%s: incorrect src_size %d\n", __func__, slave->src_size);
		BUG();
	}

	switch (direction) {
	case DMA_MEM_TO_DEV:
		for_each_sg(sgl, sg, sg_len, i) {
			struct ftdmac020_desc *tmp;
			unsigned int len;
			dma_addr_t mem;

			mem = sg_phys(sg);
			len = sg_dma_len(sg);

            if (unlikely(!pfn_valid((mem >> PAGE_SHIFT)))) {
                dev_err(chan2dev(chan), "%s: incorrect physical address 0x%x \n", __func__, mem);
                goto err;
            }
			tmp = ftdmac020_create_chain(ftchan, desc,
				mem, slave->common.dst_addr, len,
				src_shift, dst_shift, 0, 1);
			if (!tmp)
				goto err;

			if (!desc)
				desc = tmp;
		}

		if (slave->handshake >= 0)
			desc->cfg = FTDMAC020_CFG_DST_HANDSHAKE_EN
				| FTDMAC020_CFG_DST_HANDSHAKE(slave->handshake);
		break;
	case DMA_DEV_TO_MEM:
		for_each_sg(sgl, sg, sg_len, i) {
			struct ftdmac020_desc *tmp;
			unsigned int len;
			dma_addr_t mem;

			mem = sg_phys(sg);
			len = sg_dma_len(sg);

            if (unlikely(!pfn_valid((mem >> PAGE_SHIFT)))) {
                dev_err(chan2dev(chan), "%s: incorrect physical address 0x%x \n", __func__, mem);
                goto err;
            }

			tmp = ftdmac020_create_chain(ftchan, desc,
				slave->common.src_addr, mem, len,
				src_shift, dst_shift, 1, 0);
			if (!tmp)
				goto err;

			if (!desc)
				desc = tmp;
		}

		if (slave->handshake >= 0)
			desc->cfg = FTDMAC020_CFG_SRC_HANDSHAKE_EN
				| FTDMAC020_CFG_SRC_HANDSHAKE(slave->handshake);
		break;
	default:
		dev_err(chan2dev(chan), "%s: incorrect direction\n",
			__func__);
		goto err;
	}

	desc->txd.flags = flags;
	chan->private = NULL;   /* got the parameter already, so free it */
	ftchan->transaction_type = DMA_SLAVE;

	return &desc->txd;

err:
	return NULL;
}

static int ftdmac020_control(struct dma_chan *chan, enum dma_ctrl_cmd cmd,
		unsigned long arg)
{
	struct ftdmac020_chan *ftchan;
	unsigned long       flags;
	int ret = -ENXIO;

    if (flags)  {}

	dev_dbg(chan2dev(chan), "%s\n", __func__);
	ftchan = container_of(chan, struct ftdmac020_chan, common);

	if (cmd == DMA_SLAVE_CONFIG) {
		struct dma_slave_config *slave_config = (struct dma_slave_config *)arg;
		struct ftdmac020_dma_slave  *dma_slave;

		dma_slave = container_of(slave_config, struct ftdmac020_dma_slave, common);
		chan->private = dma_slave;
		ret = 0;
	}

	if (cmd == DMA_TERMINATE_ALL) {
		/*
		 * This is only called when something went wrong elsewhere, so
		 * we don't really care about the data. Just disable the channel.
		 */
		DMAC020_LOCK(&ftchan->lock);
		ftdmac020_stop_channel(ftchan);
		ftchan->transaction_type = DMA_TX_TYPE_END;
		ftdmac020_finish_all_chains(ftchan);
		DMAC020_UNLOCK(&ftchan->lock);
	}

	return ret;
}

/**
 * ftdmac020_tx_status - poll for transaction completion
 * @chan: DMA channel
 * @cookie: transaction identifier to check status of
 * @txstate: if not %NULL updated with transaction state
 *
 * If @txstate is passed in, upon return it reflect the driver
 * internal state and can be used with dma_async_is_complete() to check
 * the status of multiple cookies without re-checking hardware state.
 */
static enum dma_status
ftdmac020_tx_status(struct dma_chan *chan, dma_cookie_t cookie,
		struct dma_tx_state *txstate)
{
	struct ftdmac020_chan *ftchan;
	dma_cookie_t last_used;
	dma_cookie_t last_complete;
	enum dma_status ret;

	dev_dbg(chan2dev(chan), "%s\n", __func__);
	ftchan = container_of(chan, struct ftdmac020_chan, common);

	last_complete = ftchan->completed_cookie;
	last_used = chan->cookie;

	ret = dma_async_is_complete(cookie, last_complete, last_used);
	dma_set_tx_state(txstate, last_complete, last_used, 0);

	return ret;
}

/**
 * ftdmac020_issue_pending - try to active work
 * @chan: target DMA channel
 */
static void ftdmac020_issue_pending(struct dma_chan *chan)
{
	struct ftdmac020_chan *ftchan;
    unsigned long       flags;

    if (flags)  {}

	dev_dbg(chan2dev(chan), "%s\n", __func__);
	ftchan = container_of(chan, struct ftdmac020_chan, common);

	DMAC020_LOCK(&ftchan->lock);
	BUG_ON(list_empty(&ftchan->active_list));
	/* get first descriptor and go */
	ftdmac020_start_new_chain(ftchan);
	DMAC020_UNLOCK(&ftchan->lock);
}

/******************************************************************************
 * struct platform_driver functions
 *****************************************************************************/
static int ftdmac020_probe(struct platform_device *pdev)
{
	struct resource *res;
	struct ftdmac020 *ftdmac020;
	struct dma_device *dma;
	int irq, i , err;

	if (pdev == NULL)
		return -ENODEV;

	if ((res = platform_get_resource(pdev, IORESOURCE_MEM, 0)) == 0) {
		return -ENXIO;
	}

	if ((irq = platform_get_irq(pdev, 0)) < 0) {
		return irq;
	}

	ftdmac020 = kzalloc(sizeof(*ftdmac020), GFP_KERNEL);
	if (!ftdmac020) {
		return -ENOMEM;
	}

	dma = &ftdmac020->dma;

	INIT_LIST_HEAD(&dma->channels);
	dma->dev = &pdev->dev;

	platform_set_drvdata(pdev, ftdmac020);
	ftdmac020->id = pdev->id;

	/* map io memory */
	ftdmac020->res = request_mem_region(res->start, res->end - res->start + 1,
			dev_name(&pdev->dev));
	if (ftdmac020->res == NULL) {
		dev_err(&pdev->dev, "Could not reserve memory region\n");
		err = -ENOMEM;
		goto err_req_mem;
	}

	ftdmac020->base = ioremap(res->start, res->end - res->start + 1);
	if (ftdmac020->base == NULL) {
		dev_err(&pdev->dev, "Failed to ioremap ethernet registers\n");
		err = -EIO;
		goto err_ioremap;
	}

	/* create a pool of consistent memory blocks for hardware descriptors */
	ftdmac020->dma_desc_pool = dma_pool_create("ftdmac020_desc_pool",
			&pdev->dev, sizeof(struct ftdmac020_desc),
			4 /* word alignment */, 0);
	if (!ftdmac020->dma_desc_pool) {
		dev_err(&pdev->dev, "No memory for descriptor pool\n");
		err = -ENOMEM;
		goto err_pool_create;
	}

	/* force dma off, just in case */
	ftdmac020_disable(ftdmac020);

	/* initialize channels and create tasklet per channel */
	for (i = 0; i < CHANNEL_NR; i++) {
		struct ftdmac020_chan *ftchan = &ftdmac020->channel[i];

		INIT_LIST_HEAD(&ftchan->active_list);
		INIT_LIST_HEAD(&ftchan->free_list);
		ftchan->common.device	= dma;
		ftchan->common.cookie	= DMA_MIN_COOKIE;

		spin_lock_init(&ftchan->lock);
		ftchan->ftdmac020		= ftdmac020;
		ftchan->completed_cookie	= DMA_MIN_COOKIE;

		tasklet_init(&ftchan->tasklet, ftdmac020_tasklet, (unsigned long)ftchan);
		list_add_tail(&ftchan->common.device_node /*new*/, &dma->channels /*head*/);
	}

	/* clear all pending interrupts */
	iowrite32(0xFF, ftdmac020->base + FTDMAC020_OFFSET_TCICR);
	iowrite32(0xFF, ftdmac020->base + FTDMAC020_OFFSET_EAICR);

    /* sync dma_req */
    iowrite32(0xFF, ftdmac020->base + FTDMAC020_OFFSET_SYNC);	

	/* initialize dma_device */
	dma->device_alloc_chan_resources = ftdmac020_alloc_chan_resources;
	dma->device_free_chan_resources	= ftdmac020_free_chan_resources;
	dma->device_prep_dma_memcpy	= ftdmac020_prep_dma_memcpy;
	dma->device_prep_dma_memset 	= ftdmac020_prep_dma_memset;
	dma->device_prep_slave_sg	= ftdmac020_prep_slave_sg;
	dma->device_prep_dma_cyclic = ftdmac020_prep_dma_cyclic;
	dma->device_control		    = ftdmac020_control;
	dma->device_tx_status		= ftdmac020_tx_status;
	dma->device_issue_pending	= ftdmac020_issue_pending;

	/* set DMA capability */
	dma_cap_set(DMA_MEMCPY, dma->cap_mask); //for replacing memcpy by DMA
	dma_cap_set(DMA_MEMSET, dma->cap_mask); //for replacing memset by DMA
	dma_cap_set(DMA_SLAVE, dma->cap_mask);  //for normal DMA IO
	dma_cap_set(DMA_CYCLIC, dma->cap_mask); //for audio

	err = request_irq(irq, ftdmac020_interrupt, 0, pdev->name, ftdmac020);
	if (err) {
		dev_err(&pdev->dev, "failed to request irq %d\n", irq);
		goto err_irq;
	}

	ftdmac020->irq = irq;
	err = dma_async_device_register(dma);
	if (err) {
		dev_err(&pdev->dev, "failed to register dma device\n");
		goto err_register;
	}

	ftdmac020_enable(ftdmac020);
	dev_info(&pdev->dev,
		"DMA engine driver: irq %d, mapped at 0x%p\n",
		irq, ftdmac020->base);

	return 0;

err_register:
	free_irq(irq, ftdmac020);
err_irq:
	dma_pool_destroy(ftdmac020->dma_desc_pool);
err_pool_create:
	for (i = 0; i < CHANNEL_NR; i++) {
		struct ftdmac020_chan *ftchan = &ftdmac020->channel[i];

		list_del(&ftchan->common.device_node);
		tasklet_kill(&ftchan->tasklet);
	}

	iounmap(ftdmac020->base);
err_ioremap:
	release_resource(ftdmac020->res);
err_req_mem:
	platform_set_drvdata(pdev, NULL);
	kfree(ftdmac020);
	return err;
}

static int __exit ftdmac020_remove(struct platform_device *pdev)
{
	struct ftdmac020 *ftdmac020;
	int i;

	ftdmac020 = platform_get_drvdata(pdev);

	ftdmac020_disable(ftdmac020);
	dma_async_device_unregister(&ftdmac020->dma);

	free_irq(ftdmac020->irq, ftdmac020);
	dma_pool_destroy(ftdmac020->dma_desc_pool);

	for (i = 0; i < CHANNEL_NR; i++) {
		struct ftdmac020_chan *ftchan = &ftdmac020->channel[i];

		tasklet_disable(&ftchan->tasklet);
		tasklet_kill(&ftchan->tasklet);
		list_del(&ftchan->common.device_node);
	}

	iounmap(ftdmac020->base);
	release_resource(ftdmac020->res);

	platform_set_drvdata(pdev, NULL);
	kfree(ftdmac020);

	return 0;
}

/**
 * ftdmac020_set_platform_chanfilter() - filter function for platform dependent.
 * @chan_id: DMA channel
 * @chan_filter_fn: filter function used in platform. When ftdmac020_chan_filter is called, it will
 *                  call chan_filter_fn as well.
 * @return value: 0 for success, -1 for fail
 */
int ftdmac020_set_platform_chanfilter(int (*chan_filter_fn)(int chan_id))
{
    if (platform_chan_filter != NULL) {
        printk("%s, platform_chan_filter was registered already! \n", __func__);
        return -1;
    }

    platform_chan_filter = chan_filter_fn;

    return 0;
}

static struct platform_driver ftdmac020_driver = {
	.probe		= ftdmac020_probe,
	.remove		= __exit_p(ftdmac020_remove),
	.driver		= {
		.name	= "ftdmac020",
		.owner	= THIS_MODULE,
	},
};

/******************************************************************************
 * initialization / finalization
 *****************************************************************************/
static int __init ftdmac020_init(void)
{
	int ret = 0;
	if (MAX_CYCLE_PER_BLOCK & ~FTDMAC020_CYC_MASK)
		panic("%s, invalid MAX_CYCLE_PER_BLOCK: 0x%x \n", __func__, MAX_CYCLE_PER_BLOCK);

	if ((ret = platform_driver_register(&ftdmac020_driver))) {
		printk("failed to register DMAC020 driver\n");
		return ret;
	}
	return ret;
}

static void __exit ftdmac020_exit(void)
{
	platform_driver_unregister(&ftdmac020_driver);
}

module_init(ftdmac020_init);
module_exit(ftdmac020_exit);

MODULE_AUTHOR("Po-Yu Chuang <ratbert@faraday-tech.com>");
MODULE_DESCRIPTION("FTDMAC020 DMA engine driver");
MODULE_LICENSE("GPL");
