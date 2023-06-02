/*
 * Core driver for the Synopsys DesignWare DMA Controller
 *
 * Copyright (C) 2007-2008 Atmel Corporation
 * Copyright (C) 2010-2011 ST Microelectronics
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/bitops.h>
#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/dmaengine.h>
#include <linux/dma-mapping.h>
#include <linux/dmapool.h>
#include <linux/err.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/of.h>
#include <linux/mm.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/dmaengine.h>
#include "dmaengine.h"
#include <mach/fh_dmac_regs.h>

/*
 * This supports the Synopsys "DesignWare AHB Central DMA Controller",
 * (FH_ahb_dmac) which is used with various AMBA 2.0 systems (not all
 * of which use ARM any more).  See the "Databook" from Synopsys for
 * information beyond what licensees probably provide.
 *
 * The driver has currently been tested only with the Atmel AT32AP7000,
 * which does not support descriptor writeback.
 */

static inline unsigned int fhc_get_dms(struct fh_dma_slave *slave)
{
	return slave ? slave->dst_master : 0;
}

static inline unsigned int fhc_get_sms(struct fh_dma_slave *slave)
{
	return slave ? slave->src_master : 1;
}

static inline void fhc_set_masters(struct fh_dma_chan *fhc)
{
	struct fh_dma *fhd = to_fh_dma(fhc->chan.device);
	struct fh_dma_slave *dms = fhc->chan.private;
	unsigned char mmax = fhd->nr_masters - 1;

	if (fhc->request_line == ~0) {
		fhc->src_master = min_t(unsigned char, mmax, fhc_get_sms(dms));
		fhc->dst_master = min_t(unsigned char, mmax, fhc_get_dms(dms));
	}
}

#define FHC_DEFAULT_CTLLO(_chan) ({				\
		struct fh_dma_chan *_fhc = to_fh_dma_chan(_chan);	\
		struct dma_slave_config	*_sconfig = &_fhc->dma_sconfig;	\
		bool _is_slave = is_slave_direction(_fhc->direction);	\
		u8 _smsize = _is_slave ? _sconfig->src_maxburst :	\
			FH_DMA_MSIZE_16;			\
		u8 _dmsize = _is_slave ? _sconfig->dst_maxburst :	\
			FH_DMA_MSIZE_16;			\
								\
		(FHC_CTLL_DST_MSIZE(_dmsize)			\
		 | FHC_CTLL_SRC_MSIZE(_smsize)			\
		 | FHC_CTLL_LLP_D_EN				\
		 | FHC_CTLL_LLP_S_EN				\
		 | FHC_CTLL_DMS(_fhc->dst_master)		\
		 | FHC_CTLL_SMS(_fhc->src_master));		\
	})

#define FHC_DEFAULT_CTLLO_OLD(private) ({				\
		struct fh_dma_slave *__slave = (private);	\
		int dms = __slave ? __slave->dst_master : 0;	\
		int sms = __slave ? __slave->src_master : 1;	\
		u8 smsize = __slave ? __slave->src_msize : FH_DMA_MSIZE_16; \
		u8 dmsize = __slave ? __slave->dst_msize : FH_DMA_MSIZE_16; \
								\
		(FHC_CTLL_DST_MSIZE(dmsize)			\
		 | FHC_CTLL_SRC_MSIZE(smsize)			\
		 | FHC_CTLL_LLP_D_EN				\
		 | FHC_CTLL_LLP_S_EN				\
		 | FHC_CTLL_DMS(dms)				\
		 | FHC_CTLL_SMS(sms));				\
	})

/*
 * Number of descriptors to allocate for each channel. This should be
 * made configurable somehow; preferably, the clients (at least the
 * ones using slave transfers) should be able to give us a hint.
 */
#define NR_DESCS_PER_CHANNEL	4096

/*----------------------------------------------------------------------*/

static struct device *chan2dev(struct dma_chan *chan)
{
	return &chan->dev->device;
}
static struct device *chan2parent(struct dma_chan *chan)
{
	return chan->dev->device.parent;
}

static struct fh_desc *fhc_first_active(struct fh_dma_chan *fhc)
{
	return to_fh_desc(fhc->active_list.next);
}

static struct fh_desc *fhc_desc_get(struct fh_dma_chan *fhc)
{
	struct fh_desc *desc, *_desc;
	struct fh_desc *ret = NULL;
	unsigned int i = 0;
	unsigned long flags;

	spin_lock_irqsave(&fhc->lock, flags);
	list_for_each_entry_safe(desc, _desc, &fhc->free_list, desc_node) {
		i++;
		if (async_tx_test_ack(&desc->txd)) {
			list_del(&desc->desc_node);
			ret = desc;
			break;
		}
		dev_dbg(chan2dev(&fhc->chan), "desc %p not ACKed\n", desc);
	}
	spin_unlock_irqrestore(&fhc->lock, flags);

	dev_vdbg(chan2dev(&fhc->chan), "scanned %u descriptors on freelist\n", i);

	return ret;
}

/*
 * Move a descriptor, including any children, to the free list.
 * `desc' must not be on any lists.
 */
static void fhc_desc_put(struct fh_dma_chan *fhc, struct fh_desc *desc)
{
	unsigned long flags;

	if (desc) {
		struct fh_desc *child;

		spin_lock_irqsave(&fhc->lock, flags);
		list_for_each_entry(child, &desc->tx_list, desc_node)
			dev_vdbg(chan2dev(&fhc->chan),
					"moving child desc %p to freelist\n",
					child);
		list_splice_init(&desc->tx_list, &fhc->free_list);
		dev_vdbg(chan2dev(&fhc->chan), "moving desc %p to freelist\n", desc);
		list_add(&desc->desc_node, &fhc->free_list);
		spin_unlock_irqrestore(&fhc->lock, flags);
	}
}

static void fhc_initialize(struct fh_dma_chan *fhc)
{
	struct fh_dma *fhd = to_fh_dma(fhc->chan.device);
	struct fh_dma_slave *dms = fhc->chan.private;
	u32 cfghi = FHC_CFGH_FIFO_MODE;
	u32 cfglo = FHC_CFGL_CH_PRIOR(fhc->priority);

	if (fhc->initialized == true)
		return;

	if (dms) {
		cfghi = dms->cfg_hi;
		cfglo |= dms->cfg_lo & ~FHC_CFGL_CH_PRIOR_MASK;
	} else {
		if (fhc->direction == DMA_MEM_TO_DEV)
			cfghi = FHC_CFGH_DST_PER(fhc->request_line);
		else if (fhc->direction == DMA_DEV_TO_MEM)
			cfghi = FHC_CFGH_SRC_PER(fhc->request_line);
	}

	channel_writel(fhc, CFG_LO, cfglo);
	channel_writel(fhc, CFG_HI, cfghi);

	/* Enable interrupts */
	channel_set_bit(fhd, MASK.XFER, fhc->mask);
	channel_set_bit(fhd, MASK.BLOCK, fhc->mask);
	channel_set_bit(fhd, MASK.ERROR, fhc->mask);

	fhc->initialized = true;
}

/*----------------------------------------------------------------------*/

static inline unsigned int fhc_fast_fls(unsigned long long v)
{
	/*
	 * We can be a lot more clever here, but this should take care
	 * of the most common optimization.
	 */
	if (!(v & 7))
		return 3;
	else if (!(v & 3))
		return 2;
	else if (!(v & 1))
		return 1;
	return 0;
}

static inline void fhc_dump_chan_regs(struct fh_dma_chan *fhc)
{
	dev_err(chan2dev(&fhc->chan),
		"  SAR: 0x%x DAR: 0x%x LLP: 0x%x CTL: 0x%x:%08x\n",
		channel_readl(fhc, SAR),
		channel_readl(fhc, DAR),
		channel_readl(fhc, LLP),
		channel_readl(fhc, CTL_HI),
		channel_readl(fhc, CTL_LO));
}

static inline void fhc_chan_disable(struct fh_dma *fhd, struct fh_dma_chan *fhc)
{
	channel_clear_bit(fhd, CH_EN, fhc->mask);
	while (dma_readl(fhd, CH_EN) & fhc->mask)
		cpu_relax();
}

/*----------------------------------------------------------------------*/

/* Perform single block transfer */
static inline void fhc_do_single_block(struct fh_dma_chan *fhc,
				       struct fh_desc *desc)
{
	struct fh_dma	*fhd = to_fh_dma(fhc->chan.device);
	u32		ctllo;

	/* Software emulation of LLP mode relies on interrupts to continue
	 * multi block transfer. */
	ctllo = desc->lli.ctllo | FHC_CTLL_INT_EN;

	channel_writel(fhc, SAR, desc->lli.sar);
	channel_writel(fhc, DAR, desc->lli.dar);
	channel_writel(fhc, CTL_LO, ctllo);
	channel_writel(fhc, CTL_HI, desc->lli.ctlhi);
	channel_set_bit(fhd, CH_EN, fhc->mask);

	/* Move pointer to next descriptor */
	fhc->tx_node_active = fhc->tx_node_active->next;
}

/* Called with fhc->lock held and bh disabled */
static void fhc_dostart(struct fh_dma_chan *fhc, struct fh_desc *first)
{
	struct fh_dma	*fhd = to_fh_dma(fhc->chan.device);
	unsigned long	was_soft_llp;

	/* ASSERT:  channel is idle */
	if (dma_readl(fhd, CH_EN) & fhc->mask) {
		dev_err(chan2dev(&fhc->chan),
			"BUG: Attempted to start non-idle channel\n");
		fhc_dump_chan_regs(fhc);

		/* The tasklet will hopefully advance the queue... */
		return;
	}

	if (fhc->nollp) {
		was_soft_llp = test_and_set_bit(FH_DMA_IS_SOFT_LLP,
						&fhc->flags);
		if (was_soft_llp) {
			dev_err(chan2dev(&fhc->chan),
				"BUG: Attempted to start new LLP transfer "
				"inside ongoing one\n");
			return;
		}

		fhc_initialize(fhc);

		fhc->residue = first->total_len;
		fhc->tx_node_active = &first->tx_list;

		/* Submit first block */
		fhc_do_single_block(fhc, first);

		return;
	}

	fhc_initialize(fhc);

	channel_writel(fhc, LLP, first->txd.phys);
	channel_writel(fhc, CTL_LO,
			FHC_CTLL_LLP_D_EN | FHC_CTLL_LLP_S_EN);
	channel_writel(fhc, CTL_HI, 0);
	channel_set_bit(fhd, CH_EN, fhc->mask);
}

/*----------------------------------------------------------------------*/

static void
fhc_descriptor_complete(struct fh_dma_chan *fhc, struct fh_desc *desc,
		bool callback_required)
{
	dma_async_tx_callback		callback = NULL;
	void				*param = NULL;
	struct dma_async_tx_descriptor	*txd = &desc->txd;
	struct fh_desc			*child;
	unsigned long			flags;

	dev_vdbg(chan2dev(&fhc->chan), "descriptor %u complete\n", txd->cookie);

	spin_lock_irqsave(&fhc->lock, flags);
	dma_cookie_complete(txd);
	if (callback_required) {
		callback = txd->callback;
		param = txd->callback_param;
	}

	/* async_tx_ack */
	list_for_each_entry(child, &desc->tx_list, desc_node)
		async_tx_ack(&child->txd);
	async_tx_ack(&desc->txd);

	list_splice_init(&desc->tx_list, &fhc->free_list);
	list_move(&desc->desc_node, &fhc->free_list);

	if (!is_slave_direction(fhc->direction)) {
		struct device *parent = chan2parent(&fhc->chan);
		if (!(txd->flags & DMA_COMPL_SKIP_DEST_UNMAP)) {
			if (txd->flags & DMA_COMPL_DEST_UNMAP_SINGLE)
				dma_unmap_single(parent, desc->lli.dar,
					desc->total_len, DMA_FROM_DEVICE);
			else
				dma_unmap_page(parent, desc->lli.dar,
					desc->total_len, DMA_FROM_DEVICE);
		}
		if (!(txd->flags & DMA_COMPL_SKIP_SRC_UNMAP)) {
			if (txd->flags & DMA_COMPL_SRC_UNMAP_SINGLE)
				dma_unmap_single(parent, desc->lli.sar,
					desc->total_len, DMA_TO_DEVICE);
			else
				dma_unmap_page(parent, desc->lli.sar,
					desc->total_len, DMA_TO_DEVICE);
		}
	}

	spin_unlock_irqrestore(&fhc->lock, flags);

	if (callback)
		callback(param);
}

static void fhc_complete_all(struct fh_dma *fhd, struct fh_dma_chan *fhc)
{
	struct fh_desc *desc, *_desc;
	LIST_HEAD(list);
	unsigned long flags;

	spin_lock_irqsave(&fhc->lock, flags);
	if (dma_readl(fhd, CH_EN) & fhc->mask) {
		dev_err(chan2dev(&fhc->chan),
			"BUG: XFER bit set, but channel not idle!\n");

		/* Try to continue after resetting the channel... */
		fhc_chan_disable(fhd, fhc);
	}

	/*
	 * Submit queued descriptors ASAP, i.e. before we go through
	 * the completed ones.
	 */
	list_splice_init(&fhc->active_list, &list);
	if (!list_empty(&fhc->queue)) {
		list_move(fhc->queue.next, &fhc->active_list);
		fhc_dostart(fhc, fhc_first_active(fhc));
	}

	spin_unlock_irqrestore(&fhc->lock, flags);

	list_for_each_entry_safe(desc, _desc, &list, desc_node)
		fhc_descriptor_complete(fhc, desc, true);
}

/* Returns how many bytes were already received from source */
static inline u32 fhc_get_sent(struct fh_dma_chan *fhc)
{
	u32 ctlhi = channel_readl(fhc, CTL_HI);
	u32 ctllo = channel_readl(fhc, CTL_LO);

	return (ctlhi & FHC_CTLH_BLOCK_TS_MASK) * (1 << (ctllo >> 4 & 7));
}

static void fhc_scan_descriptors(struct fh_dma *fhd, struct fh_dma_chan *fhc)
{
	dma_addr_t llp;
	struct fh_desc *desc, *_desc;
	struct fh_desc *child;
	u32 status_xfer;
	unsigned long flags;

	spin_lock_irqsave(&fhc->lock, flags);
	/*
	 * Clear block interrupt flag before scanning so that we don't
	 * miss any, and read LLP before RAW_XFER to ensure it is
	 * valid if we decide to scan the list.
	 */
	dma_writel(fhd, CLEAR.BLOCK, fhc->mask);
	llp = channel_readl(fhc, LLP);
	status_xfer = dma_readl(fhd, RAW.XFER);

	if (status_xfer & fhc->mask) {
		/* Everything we've submitted is done */
		dma_writel(fhd, CLEAR.XFER, fhc->mask);
		if (test_bit(FH_DMA_IS_SOFT_LLP, &fhc->flags)) {
			struct list_head *head, *active = fhc->tx_node_active;

			/*
			 * We are inside first active descriptor.
			 * Otherwise something is really wrong.
			 */
			desc = fhc_first_active(fhc);

			head = &desc->tx_list;
			if (active != head) {
				/* Update desc to reflect last sent one */
				if (active != head->next)
					desc = to_fh_desc(active->prev);

				fhc->residue -= desc->len;

				child = to_fh_desc(active);

				/* Submit next block */
				fhc_do_single_block(fhc, child);

				spin_unlock_irqrestore(&fhc->lock, flags);
				return;
			}

			/* We are done here */
			clear_bit(FH_DMA_IS_SOFT_LLP, &fhc->flags);
		}
		fhc->residue = 0;

		spin_unlock_irqrestore(&fhc->lock, flags);

		fhc_complete_all(fhd, fhc);
		return;
	}

	if (list_empty(&fhc->active_list)) {
		fhc->residue = 0;
		spin_unlock_irqrestore(&fhc->lock, flags);
		return;
	}

	if (test_bit(FH_DMA_IS_SOFT_LLP, &fhc->flags)) {
		dev_vdbg(chan2dev(&fhc->chan), "%s: soft LLP mode\n", __func__);
		spin_unlock_irqrestore(&fhc->lock, flags);
		return;
	}

	dev_vdbg(chan2dev(&fhc->chan), "%s: llp=0x%llx\n", __func__,
			(unsigned long long)llp);

	list_for_each_entry_safe(desc, _desc, &fhc->active_list, desc_node) {
		/* Initial residue value */
		fhc->residue = desc->total_len;

		/* Check first descriptors addr */
		if (desc->txd.phys == llp) {
			spin_unlock_irqrestore(&fhc->lock, flags);
			return;
		}

		/* Check first descriptors llp */
		if (desc->lli.llp == llp) {
			/* This one is currently in progress */
			fhc->residue -= fhc_get_sent(fhc);
			spin_unlock_irqrestore(&fhc->lock, flags);
			return;
		}

		fhc->residue -= desc->len;
		list_for_each_entry(child, &desc->tx_list, desc_node) {
			if (child->lli.llp == llp) {
				/* Currently in progress */
				fhc->residue -= fhc_get_sent(fhc);
				spin_unlock_irqrestore(&fhc->lock, flags);
				return;
			}
			fhc->residue -= child->len;
		}

		/*
		 * No descriptors so far seem to be in progress, i.e.
		 * this one must be done.
		 */
		spin_unlock_irqrestore(&fhc->lock, flags);
		fhc_descriptor_complete(fhc, desc, true);
		spin_lock_irqsave(&fhc->lock, flags);
	}

	dev_err(chan2dev(&fhc->chan),
		"BUG: All descriptors done, but channel not idle!\n");

	/* Try to continue after resetting the channel... */
	fhc_chan_disable(fhd, fhc);

	if (!list_empty(&fhc->queue)) {
		list_move(fhc->queue.next, &fhc->active_list);
		fhc_dostart(fhc, fhc_first_active(fhc));
	}
	spin_unlock_irqrestore(&fhc->lock, flags);
}

static inline void fhc_dump_lli(struct fh_dma_chan *fhc, struct fh_lli *lli)
{
	dev_crit(chan2dev(&fhc->chan), "  desc: s0x%x d0x%x l0x%x c0x%x:%x\n",
		 lli->sar, lli->dar, lli->llp, lli->ctlhi, lli->ctllo);

}

static void fhc_handle_error(struct fh_dma *fhd, struct fh_dma_chan *fhc)
{
	struct fh_desc *bad_desc;
	struct fh_desc *child;
	unsigned long flags;

	fhc_scan_descriptors(fhd, fhc);

	spin_lock_irqsave(&fhc->lock, flags);

	/*
	 * The descriptor currently at the head of the active list is
	 * borked. Since we don't have any way to report errors, we'll
	 * just have to scream loudly and try to carry on.
	 */
	bad_desc = fhc_first_active(fhc);
	list_del_init(&bad_desc->desc_node);
	list_move(fhc->queue.next, fhc->active_list.prev);

	/* Clear the error flag and try to restart the controller */
	dma_writel(fhd, CLEAR.ERROR, fhc->mask);
	if (!list_empty(&fhc->active_list))
		fhc_dostart(fhc, fhc_first_active(fhc));

	/*
	 * WARN may seem harsh, but since this only happens
	 * when someone submits a bad physical address in a
	 * descriptor, we should consider ourselves lucky that the
	 * controller flagged an error instead of scribbling over
	 * random memory locations.
	 */
	dev_WARN(chan2dev(&fhc->chan), "Bad descriptor submitted for DMA!\n"
				       "  cookie: %d\n", bad_desc->txd.cookie);
	fhc_dump_lli(fhc, &bad_desc->lli);
	list_for_each_entry(child, &bad_desc->tx_list, desc_node)
		fhc_dump_lli(fhc, &child->lli);

	spin_unlock_irqrestore(&fhc->lock, flags);

	/* Pretend the descriptor completed successfully */
	fhc_descriptor_complete(fhc, bad_desc, true);
}

/* --------------------- Cyclic DMA API extensions -------------------- */

inline dma_addr_t fh_dma_get_src_addr(struct dma_chan *chan)
{
	struct fh_dma_chan *fhc = to_fh_dma_chan(chan);
	return channel_readl(fhc, SAR);
}
EXPORT_SYMBOL(fh_dma_get_src_addr);

inline dma_addr_t fh_dma_get_dst_addr(struct dma_chan *chan)
{
	struct fh_dma_chan *fhc = to_fh_dma_chan(chan);
	return channel_readl(fhc, DAR);
}
EXPORT_SYMBOL(fh_dma_get_dst_addr);

/* Called with fhc->lock held and all DMAC interrupts disabled */
static void fhc_handle_cyclic(struct fh_dma *fhd, struct fh_dma_chan *fhc,
		u32 status_err, u32 status_xfer, u32 status_block)
{
	unsigned long flags;

	if (status_block & fhc->mask) {
		void (*callback) (void *param);
		void *callback_param;

		dev_vdbg(chan2dev(&fhc->chan), "new cyclic period llp 0x%08x\n",
			 channel_readl(fhc, LLP));
		dma_writel(fhd, CLEAR.BLOCK, fhc->mask);

		callback = fhc->cdesc->period_callback;
		callback_param = fhc->cdesc->period_callback_param;

		if (callback)
			callback(callback_param);
	}

	/*
	 * Error and transfer complete are highly unlikely, and will most
	 * likely be due to a configuration error by the user.
	 */
	if (unlikely(status_err & fhc->mask) ||
			unlikely(status_xfer & fhc->mask)) {
		int i;

		dev_err(chan2dev(&fhc->chan), "cyclic DMA unexpected %s "
				"interrupt, stopping DMA transfer\n",
				status_xfer ? "xfer" : "error");

		spin_lock_irqsave(&fhc->lock, flags);

		fhc_dump_chan_regs(fhc);

		fhc_chan_disable(fhd, fhc);

		/* Make sure DMA does not restart by loading a new list */
		channel_writel(fhc, LLP, 0);
		channel_writel(fhc, CTL_LO, 0);
		channel_writel(fhc, CTL_HI, 0);

		dma_writel(fhd, CLEAR.ERROR, fhc->mask);
		dma_writel(fhd, CLEAR.XFER, fhc->mask);
		dma_writel(fhd, CLEAR.BLOCK, fhc->mask);

		for (i = 0; i < fhc->cdesc->periods; i++)
			fhc_dump_lli(fhc, &fhc->cdesc->desc[i]->lli);

		spin_unlock_irqrestore(&fhc->lock, flags);
	}
}

/* ------------------------------------------------------------------------- */

static void fh_dma_tasklet(unsigned long data)
{
	struct fh_dma *fhd = (struct fh_dma *)data;
	struct fh_dma_chan *fhc;
	u32 status_xfer;
	u32 status_err;
	u32 status_block;
	int i;

	status_xfer = dma_readl(fhd, RAW.XFER);
	status_block = dma_readl(fhd, RAW.BLOCK);
	status_err = dma_readl(fhd, RAW.ERROR);

	dev_vdbg(fhd->dma.dev, "%s: status_err=%x\n", __func__, status_err);

	for (i = 0; i < fhd->dma.chancnt; i++) {
		fhc = &fhd->chan[i];
		if (test_bit(FH_DMA_IS_CYCLIC, &fhc->flags))
			fhc_handle_cyclic(fhd, fhc, status_err,
					status_xfer, status_block);
		else if (status_err & (1 << i))
			fhc_handle_error(fhd, fhc);
		else if (status_xfer & (1 << i))
			fhc_scan_descriptors(fhd, fhc);
	}

	/*
	 * Re-enable interrupts.
	 */
	channel_set_bit(fhd, MASK.XFER, fhd->all_chan_mask);
	channel_set_bit(fhd, MASK.BLOCK, fhd->all_chan_mask);
	channel_set_bit(fhd, MASK.ERROR, fhd->all_chan_mask);
}

static irqreturn_t fh_dma_interrupt(int irq, void *dev_id)
{
	struct fh_dma *fhd = dev_id;
	u32 status;

	dev_vdbg(fhd->dma.dev, "%s: status=0x%x\n", __func__,
			dma_readl(fhd, STATUS_INT));

	/*
	 * Just disable the interrupts. We'll turn them back on in the
	 * softirq handler.
	 */
	channel_clear_bit(fhd, MASK.XFER, fhd->all_chan_mask);
	channel_clear_bit(fhd, MASK.BLOCK, fhd->all_chan_mask);
	channel_clear_bit(fhd, MASK.ERROR, fhd->all_chan_mask);

	status = dma_readl(fhd, STATUS_INT);
	if (status) {
		dev_err(fhd->dma.dev,
			"BUG: Unexpected interrupts pending: 0x%x\n",
			status);

		/* Try to recover */
		channel_clear_bit(fhd, MASK.XFER, (1 << 8) - 1);
		channel_clear_bit(fhd, MASK.BLOCK, (1 << 8) - 1);
		channel_clear_bit(fhd, MASK.SRC_TRAN, (1 << 8) - 1);
		channel_clear_bit(fhd, MASK.DST_TRAN, (1 << 8) - 1);
		channel_clear_bit(fhd, MASK.ERROR, (1 << 8) - 1);
	}

	tasklet_schedule(&fhd->tasklet);

	return IRQ_HANDLED;
}

/*----------------------------------------------------------------------*/

static dma_cookie_t fhc_tx_submit(struct dma_async_tx_descriptor *tx)
{
	struct fh_desc		*desc = txd_to_fh_desc(tx);
	struct fh_dma_chan	*fhc = to_fh_dma_chan(tx->chan);
	dma_cookie_t		cookie;
	unsigned long		flags;

	spin_lock_irqsave(&fhc->lock, flags);
	cookie = dma_cookie_assign(tx);

	/*
	 * REVISIT: We should attempt to chain as many descriptors as
	 * possible, perhaps even appending to those already submitted
	 * for DMA. But this is hard to do in a race-free manner.
	 */
	if (list_empty(&fhc->active_list)) {
		dev_vdbg(chan2dev(tx->chan), "%s: started %u\n", __func__,
				desc->txd.cookie);
		list_add_tail(&desc->desc_node, &fhc->active_list);
		fhc_dostart(fhc, fhc_first_active(fhc));
	} else {
		dev_vdbg(chan2dev(tx->chan), "%s: queued %u\n", __func__,
				desc->txd.cookie);

		list_add_tail(&desc->desc_node, &fhc->queue);
	}

	spin_unlock_irqrestore(&fhc->lock, flags);

	return cookie;
}

static struct dma_async_tx_descriptor *
fhc_prep_dma_memcpy(struct dma_chan *chan, dma_addr_t dest, dma_addr_t src,
		size_t len, unsigned long flags)
{
	struct fh_dma_chan	*fhc = to_fh_dma_chan(chan);
	struct fh_dma		*fhd = to_fh_dma(chan->device);
	struct fh_desc		*desc;
	struct fh_desc		*first;
	struct fh_desc		*prev;
	size_t			xfer_count;
	size_t			offset;
	unsigned int		src_width;
	unsigned int		dst_width;
	unsigned int		data_width;
	u32			ctllo;

	dev_vdbg(chan2dev(chan),
			"%s: d0x%llx s0x%llx l0x%zx f0x%lx\n", __func__,
			(unsigned long long)dest, (unsigned long long)src,
			len, flags);

	if (unlikely(!len)) {
		dev_dbg(chan2dev(chan), "%s: length is zero!\n", __func__);
		return NULL;
	}

	fhc->direction = DMA_MEM_TO_MEM;

	data_width = min_t(unsigned int, fhd->data_width[fhc->src_master],
			   fhd->data_width[fhc->dst_master]);

	src_width = dst_width = min_t(unsigned int, data_width,
				      fhc_fast_fls(src | dest | len));

	ctllo = FHC_DEFAULT_CTLLO(chan)
			| FHC_CTLL_DST_WIDTH(dst_width)
			| FHC_CTLL_SRC_WIDTH(src_width)
			| FHC_CTLL_DST_INC
			| FHC_CTLL_SRC_INC
			| FHC_CTLL_FC_M2M;
	prev = first = NULL;

	for (offset = 0; offset < len; offset += xfer_count << src_width) {
		xfer_count = min_t(size_t, (len - offset) >> src_width,
					   fhc->block_size);

		desc = fhc_desc_get(fhc);
		if (!desc)
			goto err_desc_get;

		desc->lli.sar = src + offset;
		desc->lli.dar = dest + offset;
		desc->lli.ctllo = ctllo;
		desc->lli.ctlhi = xfer_count;
		desc->len = xfer_count << src_width;

		if (!first) {
			first = desc;
		} else {
			prev->lli.llp = desc->txd.phys;
			list_add_tail(&desc->desc_node,
					&first->tx_list);
		}
		prev = desc;
	}

	if (flags & DMA_PREP_INTERRUPT)
		/* Trigger interrupt after last block */
		prev->lli.ctllo |= FHC_CTLL_INT_EN;

	prev->lli.llp = 0;
	first->txd.flags = flags;
	first->total_len = len;

	return &first->txd;

err_desc_get:
	fhc_desc_put(fhc, first);
	return NULL;
}

static struct dma_async_tx_descriptor *
fhc_prep_slave_sg(struct dma_chan *chan, struct scatterlist *sgl,
		unsigned int sg_len, enum dma_transfer_direction direction,
		unsigned long flags, void *context)
{
	struct fh_dma_chan	*fhc = to_fh_dma_chan(chan);
	struct fh_dma		*fhd = to_fh_dma(chan->device);
	struct dma_slave_config	*sconfig = &fhc->dma_sconfig;
	struct fh_desc		*prev;
	struct fh_desc		*first;
	u32			ctllo;
	dma_addr_t		reg;
	unsigned int		reg_width;
	unsigned int		mem_width;
	unsigned int		data_width;
	unsigned int		i;
	struct scatterlist	*sg;
	size_t			total_len = 0;
	struct fh_dma_pri *fh_pri = (struct fh_dma_pri *)context;
	dev_vdbg(chan2dev(chan), "%s\n", __func__);

	if (unlikely(!is_slave_direction(direction) || !sg_len))
		return NULL;

	fhc->direction = direction;

	prev = first = NULL;

	switch (direction) {
	case DMA_MEM_TO_DEV:
		reg_width = __fls(sconfig->dst_addr_width);
		reg = sconfig->dst_addr;
        if(!fh_pri){
            ctllo = (FHC_DEFAULT_CTLLO(chan)
                | FHC_CTLL_DST_WIDTH(reg_width)
                | FHC_CTLL_DST_FIX
                | FHC_CTLL_SRC_INC);
        }
        else{
            ctllo = (FHC_DEFAULT_CTLLO(chan) | FHC_CTLL_DST_WIDTH(reg_width));
            ctllo |= fh_pri->sinc << 9;
            ctllo |= fh_pri->dinc << 7;
        }

		ctllo |= sconfig->device_fc ? FHC_CTLL_FC(FH_DMA_FC_P_M2P) :
			FHC_CTLL_FC(FH_DMA_FC_D_M2P);

		data_width = fhd->data_width[fhc->src_master];

		for_each_sg(sgl, sg, sg_len, i) {
			struct fh_desc	*desc;
			u32		len, dlen, mem;

			mem = sg_dma_address(sg);
			len = sg_dma_len(sg);

			mem_width = min_t(unsigned int,
					  data_width, fhc_fast_fls(mem | len));

slave_sg_todev_fill_desc:
			desc = fhc_desc_get(fhc);
			if (!desc) {
				dev_err(chan2dev(chan),
					"not enough descriptors available\n");
				goto err_desc_get;
			}

			desc->lli.sar = mem;
			desc->lli.dar = reg;
			desc->lli.ctllo = ctllo | FHC_CTLL_SRC_WIDTH(mem_width);
			if ((len >> mem_width) > fhc->block_size) {
				dlen = fhc->block_size << mem_width;
				mem += dlen;
				len -= dlen;
			} else {
				dlen = len;
				len = 0;
			}

			desc->lli.ctlhi = dlen >> mem_width;
			desc->len = dlen;

			if (!first) {
				first = desc;
			} else {
				prev->lli.llp = desc->txd.phys;
				list_add_tail(&desc->desc_node,
						&first->tx_list);
			}
			prev = desc;
			total_len += dlen;

			if (len)
				goto slave_sg_todev_fill_desc;
		}
		break;
	case DMA_DEV_TO_MEM:
		reg_width = __fls(sconfig->src_addr_width);
		reg = sconfig->src_addr;

        if(!fh_pri){
            ctllo = (FHC_DEFAULT_CTLLO(chan)
                | FHC_CTLL_SRC_WIDTH(reg_width)
                | FHC_CTLL_DST_INC
                | FHC_CTLL_SRC_FIX);
        }
        else{
            ctllo = (FHC_DEFAULT_CTLLO(chan) | FHC_CTLL_SRC_WIDTH(reg_width));
            ctllo |= fh_pri->sinc << 9;
            ctllo |= fh_pri->dinc << 7;
        }


		ctllo |= sconfig->device_fc ? FHC_CTLL_FC(FH_DMA_FC_P_P2M) :
			FHC_CTLL_FC(FH_DMA_FC_D_P2M);

		data_width = fhd->data_width[fhc->dst_master];

		for_each_sg(sgl, sg, sg_len, i) {
			struct fh_desc	*desc;
			u32		len, dlen, mem;

			mem = sg_dma_address(sg);
			len = sg_dma_len(sg);

			mem_width = min_t(unsigned int,
					  data_width, fhc_fast_fls(mem | len));

slave_sg_fromdev_fill_desc:
			desc = fhc_desc_get(fhc);
			if (!desc) {
				dev_err(chan2dev(chan),
						"not enough descriptors available\n");
				goto err_desc_get;
			}

			desc->lli.sar = reg;
			desc->lli.dar = mem;
			desc->lli.ctllo = ctllo | FHC_CTLL_DST_WIDTH(mem_width);
			if ((len >> reg_width) > fhc->block_size) {
				dlen = fhc->block_size << reg_width;
				mem += dlen;
				len -= dlen;
			} else {
				dlen = len;
				len = 0;
			}
			desc->lli.ctlhi = dlen >> reg_width;
			desc->len = dlen;

			if (!first) {
				first = desc;
			} else {
				prev->lli.llp = desc->txd.phys;
				list_add_tail(&desc->desc_node,
						&first->tx_list);
			}
			prev = desc;
			total_len += dlen;

			if (len)
				goto slave_sg_fromdev_fill_desc;
		}
		break;
	default:
		return NULL;
	}

	if (flags & DMA_PREP_INTERRUPT)
		/* Trigger interrupt after last block */
		prev->lli.ctllo |= FHC_CTLL_INT_EN;

	prev->lli.llp = 0;
	first->total_len = total_len;

	return &first->txd;

err_desc_get:
	fhc_desc_put(fhc, first);
	return NULL;
}

/*
 * Fix sconfig's burst size according to fh_dmac. We need to convert them as:
 * 1 -> 0, 4 -> 1, 8 -> 2, 16 -> 3.
 *
 * NOTE: burst size 2 is not supported by controller.
 *
 * This can be done by finding least significant bit set: n & (n - 1)
 */
static inline void convert_burst(u32 *maxburst)
{
	if (*maxburst > 1)
		*maxburst = fls(*maxburst) - 2;
	else
		*maxburst = 0;
}

static int
set_runtime_config(struct dma_chan *chan, struct dma_slave_config *sconfig)
{
	struct fh_dma_chan *fhc = to_fh_dma_chan(chan);

	/* Check if chan will be configured for slave transfers */
	if (!is_slave_direction(sconfig->direction))
		return -EINVAL;

	memcpy(&fhc->dma_sconfig, sconfig, sizeof(*sconfig));
	fhc->direction = sconfig->direction;

	/* Take the request line from slave_id member */
	if (fhc->request_line == ~0)
		fhc->request_line = sconfig->slave_id;

	convert_burst(&fhc->dma_sconfig.src_maxburst);
	convert_burst(&fhc->dma_sconfig.dst_maxburst);

	return 0;
}

static inline void fhc_chan_pause(struct fh_dma_chan *fhc)
{
	u32 cfglo = channel_readl(fhc, CFG_LO);
	unsigned int count = 20;	/* timeout iterations */

	channel_writel(fhc, CFG_LO, cfglo | FHC_CFGL_CH_SUSP);
	while (!(channel_readl(fhc, CFG_LO) & FHC_CFGL_FIFO_EMPTY) && count--)
		udelay(2);

	fhc->paused = true;
}

static inline void fhc_chan_resume(struct fh_dma_chan *fhc)
{
	u32 cfglo = channel_readl(fhc, CFG_LO);

	channel_writel(fhc, CFG_LO, cfglo & ~FHC_CFGL_CH_SUSP);

	fhc->paused = false;
}

static int fhc_control(struct dma_chan *chan, enum dma_ctrl_cmd cmd,
		       unsigned long arg)
{
	struct fh_dma_chan	*fhc = to_fh_dma_chan(chan);
	struct fh_dma		*fhd = to_fh_dma(chan->device);
	struct fh_desc		*desc, *_desc;
	unsigned long		flags;
	LIST_HEAD(list);

	if (cmd == DMA_PAUSE) {
		spin_lock_irqsave(&fhc->lock, flags);

		fhc_chan_pause(fhc);

		spin_unlock_irqrestore(&fhc->lock, flags);
	} else if (cmd == DMA_RESUME) {
		if (!fhc->paused)
			return 0;

		spin_lock_irqsave(&fhc->lock, flags);

		fhc_chan_resume(fhc);

		spin_unlock_irqrestore(&fhc->lock, flags);
	} else if (cmd == DMA_TERMINATE_ALL) {
		spin_lock_irqsave(&fhc->lock, flags);

		clear_bit(FH_DMA_IS_SOFT_LLP, &fhc->flags);

		fhc_chan_disable(fhd, fhc);

		fhc_chan_resume(fhc);

		/* active_list entries will end up before queued entries */
		list_splice_init(&fhc->queue, &list);
		list_splice_init(&fhc->active_list, &list);

		spin_unlock_irqrestore(&fhc->lock, flags);

		/* Flush all pending and queued descriptors */
		list_for_each_entry_safe(desc, _desc, &list, desc_node)
			fhc_descriptor_complete(fhc, desc, false);
	} else if (cmd == DMA_SLAVE_CONFIG) {
		return set_runtime_config(chan, (struct dma_slave_config *)arg);
	} else {
		return -ENXIO;
	}

	return 0;
}

static inline u32 fhc_get_residue(struct fh_dma_chan *fhc)
{
	unsigned long flags;
	u32 residue;

	spin_lock_irqsave(&fhc->lock, flags);

	residue = fhc->residue;
	if (test_bit(FH_DMA_IS_SOFT_LLP, &fhc->flags) && residue)
		residue -= fhc_get_sent(fhc);

	spin_unlock_irqrestore(&fhc->lock, flags);
	return residue;
}

static enum dma_status
fhc_tx_status(struct dma_chan *chan,
	      dma_cookie_t cookie,
	      struct dma_tx_state *txstate)
{
	struct fh_dma_chan	*fhc = to_fh_dma_chan(chan);
	enum dma_status		ret;

	ret = dma_cookie_status(chan, cookie, txstate);
	if (ret != DMA_SUCCESS) {
		fhc_scan_descriptors(to_fh_dma(chan->device), fhc);

		ret = dma_cookie_status(chan, cookie, txstate);
	}

	if (ret != DMA_SUCCESS)
		dma_set_residue(txstate, fhc_get_residue(fhc));

	if (fhc->paused)
		return DMA_PAUSED;

	return ret;
}

static void fhc_issue_pending(struct dma_chan *chan)
{
	struct fh_dma_chan	*fhc = to_fh_dma_chan(chan);

	if (!list_empty(&fhc->queue))
		fhc_scan_descriptors(to_fh_dma(chan->device), fhc);
}

static int fhc_alloc_chan_resources(struct dma_chan *chan)
{
	struct fh_dma_chan	*fhc = to_fh_dma_chan(chan);
	struct fh_dma		*fhd = to_fh_dma(chan->device);
	struct fh_desc		*desc;
	int			i;
	unsigned long		flags;

	dev_vdbg(chan2dev(chan), "%s\n", __func__);

	/* ASSERT:  channel is idle */
	if (dma_readl(fhd, CH_EN) & fhc->mask) {
		dev_dbg(chan2dev(chan), "DMA channel not idle?\n");
		return -EIO;
	}

	dma_cookie_init(chan);

	/*
	 * NOTE: some controllers may have additional features that we
	 * need to initialize here, like "scatter-gather" (which
	 * doesn't mean what you think it means), and status writeback.
	 */

	fhc_set_masters(fhc);

	spin_lock_irqsave(&fhc->lock, flags);
	i = fhc->descs_allocated;
	while (fhc->descs_allocated < NR_DESCS_PER_CHANNEL) {
		dma_addr_t phys;

		spin_unlock_irqrestore(&fhc->lock, flags);

		desc = dma_pool_alloc(fhd->desc_pool, GFP_ATOMIC, &phys);
		if (!desc)
			goto err_desc_alloc;

		memset(desc, 0, sizeof(struct fh_desc));

		INIT_LIST_HEAD(&desc->tx_list);
		dma_async_tx_descriptor_init(&desc->txd, chan);
		desc->txd.tx_submit = fhc_tx_submit;
		desc->txd.flags = DMA_CTRL_ACK;
		desc->txd.phys = phys;

		fhc_desc_put(fhc, desc);

		spin_lock_irqsave(&fhc->lock, flags);
		i = ++fhc->descs_allocated;
	}

	spin_unlock_irqrestore(&fhc->lock, flags);

	dev_dbg(chan2dev(chan), "%s: allocated %d descriptors\n", __func__, i);

	return i;

err_desc_alloc:
	dev_info(chan2dev(chan), "only allocated %d descriptors\n", i);

	return i;
}

static void fhc_free_chan_resources(struct dma_chan *chan)
{
	struct fh_dma_chan	*fhc = to_fh_dma_chan(chan);
	struct fh_dma		*fhd = to_fh_dma(chan->device);
	struct fh_desc		*desc, *_desc;
	unsigned long		flags;
	LIST_HEAD(list);

	dev_dbg(chan2dev(chan), "%s: descs allocated=%u\n", __func__,
			fhc->descs_allocated);

	/* ASSERT:  channel is idle */
	BUG_ON(!list_empty(&fhc->active_list));
	BUG_ON(!list_empty(&fhc->queue));
	BUG_ON(dma_readl(to_fh_dma(chan->device), CH_EN) & fhc->mask);

	spin_lock_irqsave(&fhc->lock, flags);
	list_splice_init(&fhc->free_list, &list);
	fhc->descs_allocated = 0;
	fhc->initialized = false;
	fhc->request_line = ~0;

	/* Disable interrupts */
	channel_clear_bit(fhd, MASK.XFER, fhc->mask);
	channel_clear_bit(fhd, MASK.BLOCK, fhc->mask);
	channel_clear_bit(fhd, MASK.ERROR, fhc->mask);

	spin_unlock_irqrestore(&fhc->lock, flags);

	list_for_each_entry_safe(desc, _desc, &list, desc_node) {
		dev_vdbg(chan2dev(chan), "  freeing descriptor %p\n", desc);
		dma_pool_free(fhd->desc_pool, desc, desc->txd.phys);
	}

	dev_vdbg(chan2dev(chan), "%s: done\n", __func__);
}


/* --------------------- Cyclic DMA API extensions -------------------- */

/**
 * fh_dma_cyclic_start - start the cyclic DMA transfer
 * @chan: the DMA channel to start
 *
 * Must be called with soft interrupts disabled. Returns zero on success or
 * -errno on failure.
 */
int fh_dma_cyclic_start(struct dma_chan *chan)
{
	struct fh_dma_chan	*fhc = to_fh_dma_chan(chan);
	struct fh_dma		*fhd = to_fh_dma(fhc->chan.device);
	unsigned long		flags;

	if (!test_bit(FH_DMA_IS_CYCLIC, &fhc->flags)) {
		dev_err(chan2dev(&fhc->chan), "missing prep for cyclic DMA\n");
		return -ENODEV;
	}

	spin_lock_irqsave(&fhc->lock, flags);

	/* Assert channel is idle */
	if (dma_readl(fhd, CH_EN) & fhc->mask) {
		dev_err(chan2dev(&fhc->chan),
			"BUG: Attempted to start non-idle channel\n");
		fhc_dump_chan_regs(fhc);
		spin_unlock_irqrestore(&fhc->lock, flags);
		return -EBUSY;
	}

	dma_writel(fhd, CLEAR.ERROR, fhc->mask);
	dma_writel(fhd, CLEAR.XFER, fhc->mask);
	dma_writel(fhd, CLEAR.BLOCK, fhc->mask);

	fhc_initialize(fhc);

	/* Setup DMAC channel registers */
	channel_writel(fhc, LLP, fhc->cdesc->desc[0]->txd.phys);
	channel_writel(fhc, CTL_LO, FHC_CTLL_LLP_D_EN | FHC_CTLL_LLP_S_EN);
	channel_writel(fhc, CTL_HI, 0);

	channel_set_bit(fhd, CH_EN, fhc->mask);

	spin_unlock_irqrestore(&fhc->lock, flags);

	return 0;
}
EXPORT_SYMBOL(fh_dma_cyclic_start);

/**
 * fh_dma_cyclic_stop - stop the cyclic DMA transfer
 * @chan: the DMA channel to stop
 *
 * Must be called with soft interrupts disabled.
 */
void fh_dma_cyclic_stop(struct dma_chan *chan)
{
	struct fh_dma_chan	*fhc = to_fh_dma_chan(chan);
	struct fh_dma		*fhd = to_fh_dma(fhc->chan.device);
	unsigned long		flags;

	spin_lock_irqsave(&fhc->lock, flags);

	fhc_chan_disable(fhd, fhc);

	spin_unlock_irqrestore(&fhc->lock, flags);
}
EXPORT_SYMBOL(fh_dma_cyclic_stop);

/**
 * fh_dma_cyclic_prep - prepare the cyclic DMA transfer
 * @chan: the DMA channel to prepare
 * @buf_addr: physical DMA address where the buffer starts
 * @buf_len: total number of bytes for the entire buffer
 * @period_len: number of bytes for each period
 * @direction: transfer direction, to or from device
 *
 * Must be called before trying to start the transfer. Returns a valid struct
 * fh_cyclic_desc if successful or an ERR_PTR(-errno) if not successful.
 */
struct fh_cyclic_desc *fh_dma_cyclic_prep(struct dma_chan *chan,
		dma_addr_t buf_addr, size_t buf_len, size_t period_len,
		enum dma_transfer_direction direction)
{
	struct fh_dma_chan		*fhc = to_fh_dma_chan(chan);
	struct fh_dma_slave 		*fhs = chan->private;
	struct fh_cyclic_desc		*cdesc;
	struct fh_cyclic_desc		*retval = NULL;
	struct fh_desc			*desc;
	struct fh_desc			*last = NULL;
	unsigned long			was_cyclic;
	unsigned int			reg_width;
	unsigned int			periods;
	unsigned int			i;
	unsigned long			flags;

	spin_lock_irqsave(&fhc->lock, flags);
	if (fhc->nollp) {
		spin_unlock_irqrestore(&fhc->lock, flags);
		dev_dbg(chan2dev(&fhc->chan),
				"channel doesn't support LLP transfers\n");
		return ERR_PTR(-EINVAL);
	}

	if (!list_empty(&fhc->queue) || !list_empty(&fhc->active_list)) {
		spin_unlock_irqrestore(&fhc->lock, flags);
		dev_dbg(chan2dev(&fhc->chan),
				"queue and/or active list are not empty\n");
		return ERR_PTR(-EBUSY);
	}

	was_cyclic = test_and_set_bit(FH_DMA_IS_CYCLIC, &fhc->flags);
	spin_unlock_irqrestore(&fhc->lock, flags);
	if (was_cyclic) {
		dev_dbg(chan2dev(&fhc->chan),
				"channel already prepared for cyclic DMA\n");
		return ERR_PTR(-EBUSY);
	}

	retval = ERR_PTR(-EINVAL);

	reg_width = fhs->reg_width;

	if (unlikely(!is_slave_direction(direction)))
		goto out_err;

	fhc->direction = direction;

	periods = buf_len / period_len;

	/* Check for too big/unaligned periods and unaligned DMA buffer. */
	if (period_len > (fhc->block_size << reg_width))
		goto out_err;
	if (unlikely(period_len & ((1 << reg_width) - 1)))
		goto out_err;
	if (unlikely(buf_addr & ((1 << reg_width) - 1)))
		goto out_err;

	retval = ERR_PTR(-ENOMEM);

	if (periods > NR_DESCS_PER_CHANNEL)
		goto out_err;

	cdesc = kzalloc(sizeof(struct fh_cyclic_desc), GFP_KERNEL);
	if (!cdesc)
		goto out_err;

	cdesc->desc = kzalloc(sizeof(struct fh_desc *) * periods, GFP_KERNEL);
	if (!cdesc->desc)
		goto out_err_alloc;

	for (i = 0; i < periods; i++) {
		desc = fhc_desc_get(fhc);
		if (!desc)
			goto out_err_desc_get;

		switch (direction) {
		case DMA_MEM_TO_DEV:
			desc->lli.dar = fhs->tx_reg;
			desc->lli.sar = buf_addr + (period_len * i);
			desc->lli.ctllo = (FHC_DEFAULT_CTLLO_OLD(chan->private)
					| FHC_CTLL_DST_WIDTH(reg_width)
					| FHC_CTLL_SRC_WIDTH(reg_width)
					| FHC_CTLL_DST_FIX
					| FHC_CTLL_SRC_INC
					| FHC_CTLL_FC(fhs->fc)
					| FHC_CTLL_INT_EN);

			break;
		case DMA_DEV_TO_MEM:
			desc->lli.dar = buf_addr + (period_len * i);
			desc->lli.sar = fhs->rx_reg;
			desc->lli.ctllo = (FHC_DEFAULT_CTLLO_OLD(chan->private)
					| FHC_CTLL_SRC_WIDTH(reg_width)
					| FHC_CTLL_DST_WIDTH(reg_width)
					| FHC_CTLL_DST_INC
					| FHC_CTLL_SRC_FIX
					| FHC_CTLL_FC(fhs->fc)
					| FHC_CTLL_INT_EN);


			break;
		default:
			break;
		}

		desc->lli.ctlhi = (period_len >> reg_width);
		cdesc->desc[i] = desc;

		if (last)
		{
			last->lli.llp = desc->txd.phys;
			dma_sync_single_for_device(chan2parent(chan),
						   last->txd.phys,
						   sizeof(last->lli),
						   DMA_TO_DEVICE);
		}

		last = desc;
	}

	/* Let's make a cyclic list */
	last->lli.llp = cdesc->desc[0]->txd.phys;
	dma_sync_single_for_device(chan2parent(chan), last->txd.phys,
				   sizeof(last->lli), DMA_TO_DEVICE);

	dev_dbg(chan2dev(&fhc->chan), "cyclic prepared buf 0x%llx len %zu "
			"period %zu periods %d\n", (unsigned long long)buf_addr,
			buf_len, period_len, periods);

	cdesc->periods = periods;
	fhc->cdesc = cdesc;

	return cdesc;

out_err_desc_get:
	while (i--)
		fhc_desc_put(fhc, cdesc->desc[i]);
out_err_alloc:
	kfree(cdesc);
out_err:
	clear_bit(FH_DMA_IS_CYCLIC, &fhc->flags);
	return (struct fh_cyclic_desc *)retval;
}
EXPORT_SYMBOL(fh_dma_cyclic_prep);

/**
 * fh_dma_cyclic_free - free a prepared cyclic DMA transfer
 * @chan: the DMA channel to free
 */
void fh_dma_cyclic_free(struct dma_chan *chan)
{
	struct fh_dma_chan	*fhc = to_fh_dma_chan(chan);
	struct fh_dma		*fhd = to_fh_dma(fhc->chan.device);
	struct fh_cyclic_desc	*cdesc = fhc->cdesc;
	int			i;
	unsigned long		flags;

	dev_dbg(chan2dev(&fhc->chan), "%s\n", __func__);

	if (!cdesc)
		return;

	spin_lock_irqsave(&fhc->lock, flags);

	fhc_chan_disable(fhd, fhc);

	dma_writel(fhd, CLEAR.ERROR, fhc->mask);
	dma_writel(fhd, CLEAR.XFER, fhc->mask);
	dma_writel(fhd, CLEAR.BLOCK, fhc->mask);

	spin_unlock_irqrestore(&fhc->lock, flags);

	for (i = 0; i < cdesc->periods; i++)
		fhc_desc_put(fhc, cdesc->desc[i]);

	kfree(cdesc->desc);
	kfree(cdesc);

	clear_bit(FH_DMA_IS_CYCLIC, &fhc->flags);
}
EXPORT_SYMBOL(fh_dma_cyclic_free);

/*----------------------------------------------------------------------*/

static void fh_dma_off(struct fh_dma *fhd)
{
	int i;

	dma_writel(fhd, CFG, 0);

	channel_clear_bit(fhd, MASK.XFER, fhd->all_chan_mask);
	channel_clear_bit(fhd, MASK.BLOCK, fhd->all_chan_mask);
	channel_clear_bit(fhd, MASK.SRC_TRAN, fhd->all_chan_mask);
	channel_clear_bit(fhd, MASK.DST_TRAN, fhd->all_chan_mask);
	channel_clear_bit(fhd, MASK.ERROR, fhd->all_chan_mask);

	while (dma_readl(fhd, CFG) & FH_CFG_DMA_EN)
		cpu_relax();

	for (i = 0; i < fhd->dma.chancnt; i++)
		fhd->chan[i].initialized = false;
}

static int fh_dma_probe(struct platform_device *pdev)
{
	struct fh_dma_platform_data *pdata;
	struct resource		*io;
	struct fh_dma		*fhd;
	size_t			size;
	void __iomem		*regs;
	bool			autocfg;
	unsigned int		fh_params;
	unsigned int		nr_channels;
	unsigned int		max_blk_size = 0;
	int			irq;
	int			err;
	int			i;

	io = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!io)
		return -EINVAL;

	irq = platform_get_irq(pdev, 0);
	if (irq < 0)
		return irq;

    if (!request_mem_region(io->start, FH_REGLEN, pdev->dev.driver->name))
        return -EBUSY;

    regs = ioremap(io->start, FH_REGLEN);
    if (!regs) {
        err = -ENOMEM;
        goto err_release_r;
    }

	/* Apply default dma_mask if needed */
	if (!pdev->dev.dma_mask) {
		pdev->dev.dma_mask = &pdev->dev.coherent_dma_mask;
		pdev->dev.coherent_dma_mask = DMA_BIT_MASK(32);
	}

	fh_params = dma_read_byaddr(regs, FH_PARAMS);
	autocfg = fh_params >> FH_PARAMS_EN & 0x1;

	dev_dbg(&pdev->dev, "FH_PARAMS: 0x%08x\n", fh_params);

	pdata = dev_get_platdata(&pdev->dev);

	if (!pdata && autocfg) {
		pdata = devm_kzalloc(&pdev->dev, sizeof(*pdata), GFP_KERNEL);
		if (!pdata)
			return -ENOMEM;

		/* Fill platform data with the default values */
		pdata->is_private = true;
		pdata->chan_allocation_order = CHAN_ALLOCATION_ASCENDING;
		pdata->chan_priority = CHAN_PRIORITY_ASCENDING;
	} else if (!pdata || pdata->nr_channels > FH_DMA_MAX_NR_CHANNELS)
		return -EINVAL;

	if (autocfg)
		nr_channels = (fh_params >> FH_PARAMS_NR_CHAN & 0x7) + 1;
	else
		nr_channels = pdata->nr_channels;

	size = sizeof(struct fh_dma) + nr_channels * sizeof(struct fh_dma_chan);
	fhd = devm_kzalloc(&pdev->dev, size, GFP_KERNEL);
	if (!fhd)
		return -ENOMEM;

	fhd->clk = clk_get(&pdev->dev, "ahb_clk");
	if (IS_ERR(fhd->clk))
		return PTR_ERR(fhd->clk);
	clk_enable(fhd->clk);

	fhd->regs = regs;

	/* Get hardware configuration parameters */
	if (autocfg) {
		max_blk_size = dma_readl(fhd, MAX_BLK_SIZE);

		fhd->nr_masters = (fh_params >> FH_PARAMS_NR_MASTER & 3) + 1;
		for (i = 0; i < fhd->nr_masters; i++) {
			fhd->data_width[i] =
				(fh_params >> FH_PARAMS_DATA_WIDTH(i) & 3) + 2;
		}
	} else {
		fhd->nr_masters = pdata->nr_masters;
		memcpy(fhd->data_width, pdata->data_width, 4);
	}

	/* Calculate all channel mask before DMA setup */
	fhd->all_chan_mask = (1 << nr_channels) - 1;

	/* Force dma off, just in case */
	fh_dma_off(fhd);

	/* Disable BLOCK interrupts as well */
	channel_clear_bit(fhd, MASK.BLOCK, fhd->all_chan_mask);

	err = devm_request_irq(&pdev->dev, irq, fh_dma_interrupt, 0,
			       "fh_dmac", fhd);
	if (err)
		return err;

	platform_set_drvdata(pdev, fhd);

	/* Create a pool of consistent memory blocks for hardware descriptors */
	fhd->desc_pool = dmam_pool_create("fh_dmac_desc_pool", &pdev->dev,
					 sizeof(struct fh_desc), 4, 0);
	if (!fhd->desc_pool) {
		dev_err(&pdev->dev, "No memory for descriptors dma pool\n");
		return -ENOMEM;
	}

	tasklet_init(&fhd->tasklet, fh_dma_tasklet, (unsigned long)fhd);

	INIT_LIST_HEAD(&fhd->dma.channels);
	for (i = 0; i < nr_channels; i++) {
		struct fh_dma_chan	*fhc = &fhd->chan[i];
		int			r = nr_channels - i - 1;

		fhc->chan.device = &fhd->dma;
		dma_cookie_init(&fhc->chan);
		if (pdata->chan_allocation_order == CHAN_ALLOCATION_ASCENDING)
			list_add_tail(&fhc->chan.device_node,
					&fhd->dma.channels);
		else
			list_add(&fhc->chan.device_node, &fhd->dma.channels);

		/* 7 is highest priority & 0 is lowest. */
		if (pdata->chan_priority == CHAN_PRIORITY_ASCENDING)
			fhc->priority = r;
		else
			fhc->priority = i;

		fhc->ch_regs = &__fh_regs(fhd)->CHAN[i];
		spin_lock_init(&fhc->lock);
		fhc->mask = 1 << i;

		INIT_LIST_HEAD(&fhc->active_list);
		INIT_LIST_HEAD(&fhc->queue);
		INIT_LIST_HEAD(&fhc->free_list);

		channel_clear_bit(fhd, CH_EN, fhc->mask);

		fhc->direction = DMA_TRANS_NONE;
		fhc->request_line = ~0;

		/* Hardware configuration */
		if (autocfg) {
			unsigned int fhc_params;

			fhc_params = dma_read_byaddr(regs + r * sizeof(u32),
						     FHC_PARAMS);

			dev_dbg(&pdev->dev, "FHC_PARAMS[%d]: 0x%08x\n", i,
					    fhc_params);

			/* Decode maximum block size for given channel. The
			 * stored 4 bit value represents blocks from 0x00 for 3
			 * up to 0x0a for 4095. */
			fhc->block_size =
				(4 << ((max_blk_size >> 4 * i) & 0xf)) - 1;
			fhc->nollp =
				(fhc_params >> FHC_PARAMS_MBLK_EN & 0x1) == 0;
		} else {
			fhc->block_size = pdata->block_size;

			/* Check if channel supports multi block transfer */
			channel_writel(fhc, LLP, 0xfffffffc);
			fhc->nollp =
				(channel_readl(fhc, LLP) & 0xfffffffc) == 0;
			channel_writel(fhc, LLP, 0);
		}
	}

	/* Clear all interrupts on all channels. */
	dma_writel(fhd, CLEAR.XFER, fhd->all_chan_mask);
	dma_writel(fhd, CLEAR.BLOCK, fhd->all_chan_mask);
	dma_writel(fhd, CLEAR.SRC_TRAN, fhd->all_chan_mask);
	dma_writel(fhd, CLEAR.DST_TRAN, fhd->all_chan_mask);
	dma_writel(fhd, CLEAR.ERROR, fhd->all_chan_mask);

	dma_cap_set(DMA_MEMCPY, fhd->dma.cap_mask);
	dma_cap_set(DMA_SLAVE, fhd->dma.cap_mask);
	if (pdata->is_private)
		dma_cap_set(DMA_PRIVATE, fhd->dma.cap_mask);
	fhd->dma.dev = &pdev->dev;
	fhd->dma.device_alloc_chan_resources = fhc_alloc_chan_resources;
	fhd->dma.device_free_chan_resources = fhc_free_chan_resources;

	fhd->dma.device_prep_dma_memcpy = fhc_prep_dma_memcpy;

	fhd->dma.device_prep_slave_sg = fhc_prep_slave_sg;
	fhd->dma.device_control = fhc_control;

	fhd->dma.device_tx_status = fhc_tx_status;
	fhd->dma.device_issue_pending = fhc_issue_pending;

	dma_writel(fhd, CFG, FH_CFG_DMA_EN);

	err = dma_async_device_register(&fhd->dma);

	if(err)
	    pr_err("dma register failed, ret %d\n", err);

    dev_info(&pdev->dev, "FH DMA Controller, %d channels\n",
         nr_channels);

	return 0;

err_release_r:
    release_resource(io);
    return err;
}

static int fh_dma_remove(struct platform_device *pdev)
{
	struct fh_dma		*fhd = platform_get_drvdata(pdev);
	struct fh_dma_chan	*fhc, *_fhc;

	fh_dma_off(fhd);
	dma_async_device_unregister(&fhd->dma);

	tasklet_kill(&fhd->tasklet);

	list_for_each_entry_safe(fhc, _fhc, &fhd->dma.channels,
			chan.device_node) {
		list_del(&fhc->chan.device_node);
		channel_clear_bit(fhd, CH_EN, fhc->mask);
	}

	return 0;
}

static void fh_dma_shutdown(struct platform_device *pdev)
{
	struct fh_dma	*fhd = platform_get_drvdata(pdev);

	fh_dma_off(fhd);
	clk_disable(fhd->clk);
}

static int fh_dma_suspend_noirq(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct fh_dma	*fhd = platform_get_drvdata(pdev);

	fh_dma_off(fhd);
	clk_disable(fhd->clk);

	return 0;
}

static int fh_dma_resume_noirq(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct fh_dma	*fhd = platform_get_drvdata(pdev);

	clk_enable(fhd->clk);
	dma_writel(fhd, CFG, FH_CFG_DMA_EN);

	return 0;
}

static const struct dev_pm_ops fh_dma_dev_pm_ops = {
	.suspend_noirq = fh_dma_suspend_noirq,
	.resume_noirq = fh_dma_resume_noirq,
	.freeze_noirq = fh_dma_suspend_noirq,
	.thaw_noirq = fh_dma_resume_noirq,
	.restore_noirq = fh_dma_resume_noirq,
	.poweroff_noirq = fh_dma_suspend_noirq,
};

static struct platform_driver fh_dma_driver = {
	.probe		= fh_dma_probe,
	.remove		= fh_dma_remove,
	.shutdown	= fh_dma_shutdown,
	.driver = {
		.name	= "fh_dmac",
		.pm	= &fh_dma_dev_pm_ops,
	},
};

static int __init fh_dma_init(void)
{
	return platform_driver_register(&fh_dma_driver);
}
subsys_initcall(fh_dma_init);

static void __exit fh_dma_exit(void)
{
	platform_driver_unregister(&fh_dma_driver);
}
module_exit(fh_dma_exit);

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("FH DMA Controller driver");
