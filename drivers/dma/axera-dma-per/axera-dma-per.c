/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/

#include "axera-dma-per.h"

// #define AX_DMA_PERIPHERAL_DEBUG

static ulong dmachan_start_cnt[AX_DMA_PER_MAX_CHANNELS] = {0};
static ulong dmachan_completed_cnt[AX_DMA_PER_MAX_CHANNELS] = {0};
static ulong dmachan_cyclic[AX_DMA_PER_MAX_CHANNELS] = {0};
static ulong dmachan_start_callback[AX_DMA_PER_MAX_CHANNELS] = {0};
static ulong dmachan_intr_end[AX_DMA_PER_MAX_CHANNELS] = {0};

void axi_chan_status_print(void)
{
	int i;

	for (i = 0; i < AX_DMA_PER_MAX_CHANNELS; i++) {
		printk("ch%d: start %ld times, completed %ld times, cyclic %ld times, dmachan_start_callback %ld times, dmachan_intr_end %ld times\n",
			i, dmachan_start_cnt[i], dmachan_completed_cnt[i], dmachan_cyclic[i], dmachan_start_callback[i], dmachan_intr_end[i]);
	}
}

static int ax_dma_per_chan_clk_en(struct axi_dma_chan *chan, char en)
{
	int ret = 0;
	static unsigned int using;

	if (en) {
		if (!using) {
			ret |= clk_prepare_enable(chan->chip->clk);
		}
		using |= BIT(chan->id);
	} else {
		if (using) {
			using &= ~BIT(chan->id);
			if (!using) {
				clk_disable_unprepare(chan->chip->clk);
			}
		}
	}
	return ret;
}

static int ax_dma_per_pclk_en(struct ax_dma_per_chip *chip, char en)
{
	int ret = 0;

	if (en) {
		ret |= clk_prepare_enable(chip->pclk);
	} else {
		clk_disable_unprepare(chip->pclk);
	}
	return ret;
}

static inline void
axi_dma_iowrite32(struct ax_dma_per_chip *chip, u32 reg, u32 val)
{
	iowrite32(val, chip->regs + reg);
	mb();
}

static inline u32 axi_dma_ioread32(struct ax_dma_per_chip *chip, u32 reg)
{
	u32 ret;
	ret = ioread32(chip->regs + reg);
	mb();
	return ret;
}

static u64 axi_dma_ioread64(struct ax_dma_per_chip *chip, u32 haddr)
{
	u64 val = 0;

	val = (u64)axi_dma_ioread32(chip, haddr);
	mb();
	val = (val << 32) | axi_dma_ioread32(chip, haddr + 4);
	mb();
	return val;
}

static inline void axi_dma_reg_set(struct ax_dma_per_chip *chip,
				   dma_addr_t addr, u32 val, u32 shift,
				   u32 mask)
{
	u32 tmp;
	unsigned long flags;

	spin_lock_irqsave(&chip->lock, flags);
	tmp = axi_dma_ioread32(chip, addr);
	tmp &= ~(mask << shift);
	axi_dma_iowrite32(chip, addr, tmp | (val << shift));
	spin_unlock_irqrestore(&chip->lock, flags);
}

static inline const char *axi_chan_name(struct axi_dma_chan *chan)
{
	return dma_chan_name(&chan->vc.chan);
}

static inline void axi_dma_irq_set(struct ax_dma_per_chip *chip, u32 irq_mask)
{
	axi_dma_iowrite32(chip, AX_DMAPER_INT_GLB_MASK, irq_mask);
}

static inline u32 axi_dma_irq_read(struct ax_dma_per_chip *chip)
{
	return axi_dma_ioread32(chip, AX_DMAPER_INT_GLB_STA);
}

static inline void axi_chan_irq_enable(struct axi_dma_chan *chan)
{
	axi_dma_reg_set(chan->chip, AX_DMAPER_INT_GLB_MASK, 1, chan->id, 1);
}

static inline void axi_chan_irq_disable(struct axi_dma_chan *chan)
{
	axi_dma_reg_set(chan->chip, AX_DMAPER_INT_GLB_MASK, 0, chan->id, 1);
}

static inline void axi_chan_set_high_perf(struct axi_dma_chan *chan, int en)
{
	axi_dma_reg_set(chan->chip, AX_DMAPER_HIGH_PERF, !!en, chan->id, 1);
}

static inline void axi_chan_irq_mask_set(struct axi_dma_chan *chan,
					 u32 irq_mask)
{
	axi_dma_iowrite32(chan->chip, AX_DMAPER_INT_MASK(chan->id), irq_mask);
}

static inline void axi_chan_irq_clear_all(struct axi_dma_chan *chan)
{
	axi_dma_iowrite32(chan->chip, AX_DMAPER_INT_CLR(chan->id),
			  CHN_INTR_ALL);
}

static inline void axi_chan_irq_clear(struct axi_dma_chan *chan, u32 irq_mask)
{
	axi_dma_iowrite32(chan->chip, AX_DMAPER_INT_CLR(chan->id), irq_mask);
}

static inline u32 axi_chan_irq_read(struct axi_dma_chan *chan)
{
	return axi_dma_ioread32(chan->chip, AX_DMAPER_INT_STA(chan->id));
}

static inline void axi_chan_enable(struct axi_dma_chan *chan)
{
	axi_dma_reg_set(chan->chip, AX_DMAPER_CHN_EN, 1, chan->id, 1);
}

static inline void axi_chan_disable(struct axi_dma_chan *chan)
{
	axi_dma_reg_set(chan->chip, AX_DMAPER_CHN_EN, 0, chan->id, 1);
}

static inline void axi_chan_start(struct axi_dma_chan *chan)
{
	axi_dma_iowrite32(chan->chip, AX_DMAPER_START, BIT(chan->id));
}

static void write_chan_llp(struct axi_dma_chan *chan, u64 addr)
{
	axi_dma_iowrite32(chan->chip, AX_DMAPER_CHN_LLI_H(chan->id),
			  upper_32_bits(addr));
	axi_dma_iowrite32(chan->chip, AX_DMAPER_CHN_LLI_L(chan->id),
			  lower_32_bits(addr));
}

static inline bool axi_chan_is_hw_enable(struct axi_dma_chan *chan)
{
	u32 val;

	val = axi_dma_ioread32(chan->chip, AX_DMAPER_CHN_EN);

	return !!(val & BIT(chan->id));
}

static u64 read_chan_llp(struct axi_dma_chan *chan)
{
	return axi_dma_ioread64(chan->chip, AX_DMAPER_CHN_LLI_H(chan->id));
}

static u64 read_chan_sar(struct axi_dma_chan *chan)
{
	return axi_dma_ioread64(chan->chip, AX_DMAPER_SRC_ADDR_H(chan->id));
}

static u64 read_chan_dst(struct axi_dma_chan *chan)
{
	return axi_dma_ioread64(chan->chip, AX_DMAPER_DST_ADDR_H(chan->id));
}

#ifndef CONFIG_AX_RISCV_LOAD_ROOTFS
static void ax_dma_per_set_def_req_id(struct axi_dma_chan *chan)
{
	u32 ch = chan->id;
	u32 req_id = 0x3f;

	if (req_id > 31)
		iowrite32(BIT(req_id - 32),
			  chan->chip->req_regs + AX_DMA_REQ_SEL_H_SET);
	else
		iowrite32(BIT(req_id),
			  chan->chip->req_regs + AX_DMA_REQ_SEL_L_SET);

	iowrite32(GENMASK(5, 0) << (ch % 5) * 6,
		  chan->chip->req_regs + AX_DMA_REQ_HS_SEL_CLR(ch / 5));
	iowrite32(req_id << (ch % 5) * 6,
		  chan->chip->req_regs + AX_DMA_REQ_HS_SEL_SET(ch / 5));
}
#endif

static u8 ax_dma_get_handshake_num(struct axi_dma_chan *chan)
{
	if (chan->use_of_dma)
		return chan->of_dma_handshake_num;
	else
		return chan->config.slave_id & 0xff;
}

static void ax_dma_per_set_req_id(struct axi_dma_chan *chan)
{
	u32 ch = chan->id;
	u32 req_id = ax_dma_get_handshake_num(chan);

	if (req_id > 31)
		iowrite32(BIT(req_id - 32),
			  chan->chip->req_regs + AX_DMA_REQ_SEL_H_SET);
	else
		iowrite32(BIT(req_id),
			  chan->chip->req_regs + AX_DMA_REQ_SEL_L_SET);

	iowrite32(GENMASK(5, 0) << (ch % 5) * 6,
		  chan->chip->req_regs + AX_DMA_REQ_HS_SEL_CLR(ch / 5));
	iowrite32(req_id << (ch % 5) * 6,
		  chan->chip->req_regs + AX_DMA_REQ_HS_SEL_SET(ch / 5));
}

static void ax_dma_per_clr_req_id(struct axi_dma_chan *chan)
{
	u32 ch = chan->id;
	u32 req_id = ax_dma_get_handshake_num(chan);

	if (req_id > 31)
		iowrite32(BIT(req_id - 32),
			  chan->chip->req_regs + AX_DMA_REQ_SEL_H_CLR);
	else
		iowrite32(BIT(req_id),
			  chan->chip->req_regs + AX_DMA_REQ_SEL_L_CLR);

	iowrite32(GENMASK(5, 0) << (ch % 5) * 6,
		  chan->chip->req_regs + AX_DMA_REQ_HS_SEL_SET(ch / 5));
}

#ifdef AX_DMA_PERIPHERAL_DEBUG
static void axi_chan_dump_lli(struct axi_dma_chan *chan,
			      struct axi_dma_hw_desc *desc, int count)
{
	if (!desc->lli) {
		dev_err(dchan2dev(&chan->vc.chan), "NULL LLI\n");
		return;
	}
	dev_info(dchan2dev(&chan->vc.chan), "count %d\
		 SAR: 0x%llx DAR: 0x%llx LLP: 0x%llx BTS 0x%llx CTL: 0x%llx\n",\
		 count, le64_to_cpu(desc->lli->sar),\
		 le64_to_cpu(desc->lli->dar),\
		 le64_to_cpu(desc->lli->data.llp),\
		 le64_to_cpu(desc->lli->data.block_ts),\
		 le64_to_cpu(*(u64 *)(&desc->lli->ctrl)));
}

static void axi_chan_list_dump_lli(struct axi_dma_chan *chan,
				   struct axi_dma_desc *desc_head)
{
	int count = atomic_read(&desc_head->descs_allocated);
	int i;

	for (i = 0; i < count; i++)
		axi_chan_dump_lli(chan, &desc_head->hw_desc[i], count);
}
#endif

static int dma_chan_pause(struct dma_chan *dchan)
{
	struct axi_dma_chan *chan = dchan_to_axi_dma_chan(dchan);
	unsigned long flags;
	unsigned int timeout = 20;	/* timeout iterations */

	spin_lock_irqsave(&chan->vc.lock, flags);
	axi_dma_reg_set(chan->chip, AX_DMAPER_CTRL, 1, chan->id,
			AX_DMAPER_CHN_SUSPEND_EN_MASK);
	do {
		if (axi_dma_ioread32(chan->chip, AX_DMAPER_STA) & BIT(chan->id))
			break;
		udelay(1);
	} while (--timeout);

	chan->is_paused = true;
	spin_unlock_irqrestore(&chan->vc.lock, flags);

	return timeout ? 0 : -EAGAIN;
}

static int dma_chan_resume(struct dma_chan *dchan)
{
	struct axi_dma_chan *chan = dchan_to_axi_dma_chan(dchan);
	unsigned long flags;

	spin_lock_irqsave(&chan->vc.lock, flags);
	if (chan->is_paused) {
		axi_dma_reg_set(chan->chip, AX_DMAPER_CTRL, 0, chan->id,
				AX_DMAPER_CHN_SUSPEND_EN_MASK);
		chan->is_paused = false;
	}
	spin_unlock_irqrestore(&chan->vc.lock, flags);

	return 0;
}

static int ax_dma_per_suspend(struct ax_dma_per_chip *chip)
{
	int ret;
	u32 val;
	u32 is_suspend =
	    AX_DMAPER_ALL_SUSPEND_STA_MASK << AX_DMAPER_ALL_SUSPEND_STA_SHIFT;

	axi_dma_reg_set(chip, AX_DMAPER_CTRL, 1, AX_DMAPER_ALL_SUSPEND_EN_SHIFT,
			AX_DMAPER_ALL_SUSPEND_EN_MASK);
	ret =
	    readl_poll_timeout_atomic(chip->regs + AX_DMAPER_STA, val,
				      (val & is_suspend), 1000, 10000);
	if (ret == -ETIMEDOUT)
		dev_warn(chip->dev, "failed to suspend all\n");

	return 0;
}

static int ax_dma_per_resume(struct ax_dma_per_chip *chip)
{
	int ret;
	u32 val;
	u32 is_suspend =
	    AX_DMAPER_ALL_SUSPEND_STA_MASK << AX_DMAPER_ALL_SUSPEND_STA_SHIFT;

	/*use clk/reset driver API when it is ready */
	/* lli prefetch disable */
	axi_dma_reg_set(chip, AX_DMAPER_CTRL, 0,
			AX_DMAPER_LLI_RD_PREFETCH_SHIFT,
			AX_DMAPER_LLI_RD_PREFETCH_MASK);
	/* globle irq en */
	axi_dma_irq_set(chip, INT_RESP_ERR);

	axi_dma_reg_set(chip, AX_DMAPER_CTRL, 0, AX_DMAPER_ALL_SUSPEND_EN_SHIFT,
			AX_DMAPER_ALL_SUSPEND_EN_MASK);
	ret =
	    readl_poll_timeout_atomic(chip->regs + AX_DMAPER_STA, val,
				      !(val & is_suspend), 1000, 10000);
	if (ret == -ETIMEDOUT)
		dev_warn(chip->dev, "failed to resume all\n");
	return 0;
}

static struct axi_dma_desc *axi_desc_alloc(struct ax_dma_per_chip *chip, u32 num)
{
	struct axi_dma_desc *desc;

	desc = kmem_cache_alloc(chip->desc_kmem, GFP_NOWAIT | __GFP_ZERO);
	if (!desc)
		return NULL;
	atomic_set(&desc->descs_allocated, 0);
	desc->hw_desc = kcalloc(num, sizeof(*desc->hw_desc), GFP_NOWAIT);
	if (!desc->hw_desc) {
		kmem_cache_free(chip->desc_kmem, desc);
		return NULL;
	}

	return desc;
}

static struct axi_dma_lli *axi_desc_get(struct axi_dma_chan *chan,
					struct axi_dma_desc *desc,
					dma_addr_t *addr)
{
	struct axi_dma_lli *lli;
	dma_addr_t phys;

	lli = dma_pool_zalloc(chan->chip->desc_pool, GFP_NOWAIT, &phys);
	if (unlikely(!lli)) {
		dev_err(chan2dev(chan),
			"%s: not enough descriptors available\n",
			axi_chan_name(chan));
		return NULL;
	}

	*addr = phys;
	atomic_inc(&desc->descs_allocated);
	return lli;
}

static void axi_desc_put(struct axi_dma_desc *desc)
{
	struct axi_dma_chan *chan = desc->chan;
	struct axi_dma_hw_desc *hw_desc;
	int count = atomic_read(&desc->descs_allocated);
	int descs_put;

	for (descs_put = 0; descs_put < count; descs_put++) {
		hw_desc = &desc->hw_desc[descs_put];
		dma_pool_free(chan->chip->desc_pool, hw_desc->lli, hw_desc->llp);
	}

	kfree(desc->hw_desc);
	atomic_sub(descs_put, &desc->descs_allocated);
	kmem_cache_free(chan->chip->desc_kmem, desc);
}

static void vchan_desc_put(struct virt_dma_desc *vdesc)
{
	axi_desc_put(vd_to_axi_desc(vdesc));
}

static size_t dma_get_residue(struct axi_dma_chan *chan,
			      struct virt_dma_desc *vdesc)
{
	unsigned long flags;
	struct axi_dma_hw_desc *hw_desc;
	struct axi_dma_desc *desc = vd_to_axi_desc(vdesc);
	u64 completed_length, len = 0, llp, i;
	int count = atomic_read(&desc->descs_allocated);

	if (chan->cyclic) {
		spin_lock_irqsave(&chan->vc.lock, flags);
		llp = desc->hw_desc[desc->cur_llp_index].llp;
		spin_unlock_irqrestore(&chan->vc.lock, flags);
	} else {
		llp = read_chan_llp(chan);
	}

	for (i = 0; i < count; i++) {
		hw_desc = &desc->hw_desc[i];
		if (hw_desc->llp == llp) {
			if (chan->direction == DMA_MEM_TO_DEV) {
				completed_length =
				    len + read_chan_sar(chan) -
				    hw_desc->lli->sar;
				return desc->length - completed_length;
			} else if (chan->direction == DMA_DEV_TO_MEM) {
				completed_length =
				    len + read_chan_dst(chan) -
				    hw_desc->lli->dar;
				return desc->length - completed_length;
			}
		}
		len += desc->hw_desc[i].len;
	}

	return 0;
}

static enum dma_status
dma_chan_tx_status(struct dma_chan *dchan, dma_cookie_t cookie,
		   struct dma_tx_state *txstate)
{
	struct axi_dma_chan *chan = dchan_to_axi_dma_chan(dchan);
	struct virt_dma_desc *vdesc;
	enum dma_status status;
	u32 i, length, completed_length = 0;
	unsigned long flags;
	u32 completed_blocks;
	size_t bytes = 0;

	status = dma_cookie_status(dchan, cookie, txstate);
	if (status == DMA_COMPLETE || !txstate)
		return status;

	spin_lock_irqsave(&chan->vc.lock, flags);

	vdesc = vchan_find_desc(&chan->vc, cookie);
	if (vdesc) {
		if (chan->cyclic) {
			length = vd_to_axi_desc(vdesc)->length;
			completed_blocks =
			    vd_to_axi_desc(vdesc)->completed_blocks;

			for (i = 0; i < completed_blocks; i++)
				completed_length +=
				    vd_to_axi_desc(vdesc)->hw_desc[i].len;
			bytes = length - completed_length;
		} else {
			bytes = dma_get_residue(chan, vdesc);
		}
	}

	spin_unlock_irqrestore(&chan->vc.lock, flags);
	dma_set_residue(txstate, bytes);

	return status;
}

/* Called in chan locked context */
static void axi_chan_block_xfer_start(struct axi_dma_chan *chan,
				      struct axi_dma_desc *first)
{
	if (chan->id < AX_DMA_PER_MAX_CHANNELS)
		dmachan_start_cnt[chan->id]++;
	if (unlikely(axi_chan_is_hw_enable(chan))) {
		dev_err(chan2dev(chan), "(%d) %s is non-idle!\n",
			__LINE__, axi_chan_name(chan));
		return;
	}
	axi_chan_set_high_perf(chan, first->high_perf);
	ax_dma_per_set_req_id(chan);
	write_chan_llp(chan, first->hw_desc[0].llp);
	axi_chan_enable(chan);
	axi_chan_irq_mask_set(chan, CHN_INTR_ALL);
	axi_chan_irq_enable(chan);
	axi_chan_start(chan);
	return;
}

static void axi_chan_start_first_queued(struct axi_dma_chan *chan)
{
	struct axi_dma_desc *desc;
	struct virt_dma_desc *vd;

	vd = vchan_next_desc(&chan->vc);
	if (!vd)
		return;

	desc = vd_to_axi_desc(vd);
	dev_dbg(chan2dev(chan), "%s: started %u\n", axi_chan_name(chan),
		vd->tx.cookie);
	axi_chan_block_xfer_start(chan, desc);
}

static void dma_chan_issue_pending(struct dma_chan *dchan)
{
	struct axi_dma_chan *chan = dchan_to_axi_dma_chan(dchan);
	unsigned long flags;

	spin_lock_irqsave(&chan->vc.lock, flags);
#ifdef CONFIG_AX_RISCV_LOAD_ROOTFS
	if (!chan->chip->en) {
		chan->chip->en = 1;
		/* lli prefetch disable */
		axi_dma_reg_set(chan->chip, AX_DMAPER_CTRL, 0,
			AX_DMAPER_LLI_RD_PREFETCH_SHIFT,
			AX_DMAPER_LLI_RD_PREFETCH_MASK);
		/* globle irq en */
		axi_dma_irq_set(chan->chip, INT_RESP_ERR);
		enable_irq(chan->chip->irq);
	}
#endif
	if (vchan_issue_pending(&chan->vc))
		axi_chan_start_first_queued(chan);
	spin_unlock_irqrestore(&chan->vc.lock, flags);
}

static void set_desc_last(struct axi_dma_hw_desc *hw_desc)
{
	hw_desc->lli->ctrl.last = 1;
}

static void write_desc_llp(struct axi_dma_hw_desc *hw_desc, u64 addr)
{
	hw_desc->lli->data.llp = cpu_to_le64(addr);
}

static int axi_dma_set_hw_desc(struct axi_dma_chan *chan,
			       struct axi_dma_desc *desc,
			       struct axi_dma_hw_desc *hw_desc,
			       dma_addr_t mem_addr, size_t len)
{
	u32 burst_size_byte;

	hw_desc->lli = axi_desc_get(chan, desc, &hw_desc->llp);
	if (unlikely(!hw_desc->lli))
		return -ENOMEM;
	if (chan->cyclic)
		hw_desc->lli->ctrl.ioc = 1;
	else
		hw_desc->lli->ctrl.ioc = 0;
	hw_desc->lli->ctrl.endian = 0;
	hw_desc->lli->ctrl.dst_osd = 0xf;
	hw_desc->lli->ctrl.src_osd = 0xf;
	/*in dma_chan_prep_slave_sg/dma_chan_prep_cyclic set
	 dst_addr_width = src_addr_width = dev addr wridth*/
	hw_desc->lli->ctrl.dst_tr_wridth = __ffs(chan->config.dst_addr_width);
	hw_desc->lli->ctrl.src_tr_wridth = hw_desc->lli->ctrl.dst_tr_wridth;

	switch (chan->direction) {
	case DMA_MEM_TO_DEV:
		hw_desc->lli->sar = cpu_to_le64((u64)mem_addr);
		hw_desc->lli->dar = cpu_to_le64((u64)chan->config.dst_addr);
		hw_desc->lli->ctrl.src_per = 0;
		hw_desc->lli->ctrl.sinc = 1;
		hw_desc->lli->ctrl.dinc = 0;
		hw_desc->lli->ctrl.arlen = 2;
		hw_desc->lli->ctrl.awlen = 1;
		//Dynamic change tr width for mem
		burst_size_byte = chan->config.dst_maxburst << hw_desc->lli->ctrl.dst_tr_wridth;
		hw_desc->lli->ctrl.src_tr_wridth = __ffs(len | 0x8 | burst_size_byte);
		hw_desc->lli->ctrl.dst_msize = chan->config.dst_maxburst;
		hw_desc->lli->ctrl.src_msize = burst_size_byte >> hw_desc->lli->ctrl.src_tr_wridth;
		break;
	case DMA_DEV_TO_MEM:
		hw_desc->lli->sar = cpu_to_le64((u64)chan->config.src_addr);
		hw_desc->lli->dar = cpu_to_le64((u64)mem_addr);
		hw_desc->lli->ctrl.src_per = 1;
		hw_desc->lli->ctrl.sinc = 0;
		hw_desc->lli->ctrl.dinc = 1;
		hw_desc->lli->ctrl.arlen = 1;
		hw_desc->lli->ctrl.awlen = 2;
		burst_size_byte = chan->config.src_maxburst << hw_desc->lli->ctrl.src_tr_wridth;
		hw_desc->lli->ctrl.dst_tr_wridth = __ffs(len | 0x8 | burst_size_byte);
		hw_desc->lli->ctrl.src_msize = chan->config.src_maxburst;
		hw_desc->lli->ctrl.dst_msize = burst_size_byte >> hw_desc->lli->ctrl.dst_tr_wridth;
		break;
	default:
		return -EINVAL;
	}
	if (burst_size_byte & 0x7)
		desc->high_perf = 0;
	hw_desc->lli->data.block_ts = len >> hw_desc->lli->ctrl.src_tr_wridth;
	hw_desc->len = len;
	if (chan->config.slave_id)
		if (ax_dma_get_handshake_num(chan) == 0 || ax_dma_get_handshake_num(chan) == 1)//add ssi spi endian
			hw_desc->lli->ctrl.endian = (chan->config.slave_id >> 30) & 0x3;

	return 0;
}

static struct dma_async_tx_descriptor *dma_chan_prep_cyclic(struct dma_chan *dchan,
							    dma_addr_t dma_addr,
							    size_t buf_len,
							    size_t period_len,
							    enum
							    dma_transfer_direction
							    direction,
							    unsigned long flags)
{
	struct axi_dma_chan *chan = dchan_to_axi_dma_chan(dchan);
	struct axi_dma_hw_desc *hw_desc = NULL;
	struct axi_dma_desc *desc = NULL;
	dma_addr_t src_addr = dma_addr;
	u32 num_periods, num_segments;
	size_t axi_block_len;
	u32 total_segments;
	u32 len, xfer_len;
	unsigned int i, loop;
	int status;
	u64 llp = 0;

	num_periods = buf_len / period_len;

	if (direction == DMA_MEM_TO_DEV) {
		chan->config.src_addr_width = chan->config.dst_addr_width;
	} else {
		chan->config.dst_addr_width = chan->config.src_addr_width;
	}
	axi_block_len =
	    chan->chip->axdma->hinfo->block_size * chan->config.src_addr_width;
	if (axi_block_len == 0)
		return NULL;

	num_segments = DIV_ROUND_UP(period_len, axi_block_len);
	total_segments = num_periods * num_segments;

	desc = axi_desc_alloc(chan->chip, total_segments);
	if (unlikely(!desc))
		goto err_desc_get;

	chan->direction = direction;
	desc->chan = chan;
	chan->cyclic = true;
	desc->length = 0;
	desc->period_len = period_len;

	for (i = 0, loop = 0; i < num_periods; i++) {
		len = period_len;
		do {
			if (len > axi_block_len)
				xfer_len = axi_block_len;
			else
				xfer_len = len;
			hw_desc = &desc->hw_desc[loop++];
			status =
			    axi_dma_set_hw_desc(chan, desc, hw_desc, src_addr,
						xfer_len);
			if (status < 0)
				goto err_desc_get;

			desc->length += hw_desc->len;
			len -= xfer_len;
			src_addr += xfer_len;
		} while (len);
	}

	llp = desc->hw_desc[0].llp;

	/* Managed transfer list */
	do {
		hw_desc = &desc->hw_desc[--total_segments];
		write_desc_llp(hw_desc, llp);
		llp = hw_desc->llp;
	} while (total_segments);
	desc->cur_llp_index = 0;
#ifdef AX_DMA_PERIPHERAL_DEBUG
	axi_chan_list_dump_lli(chan, desc);
#endif

	return vchan_tx_prep(&chan->vc, &desc->vd, flags);

err_desc_get:
	if (!IS_ERR_OR_NULL(desc))
		axi_desc_put(desc);
	return NULL;
}

static struct dma_async_tx_descriptor *dma_chan_prep_slave_sg(struct dma_chan *dchan,
							      struct scatterlist *sgl,
							      unsigned int sg_len,
							      enum dma_transfer_direction direction,
							      unsigned long flags,
							      void *context)
{
	struct axi_dma_chan *chan = dchan_to_axi_dma_chan(dchan);
	struct axi_dma_hw_desc *hw_desc = NULL;
	struct axi_dma_desc *desc = NULL;
	unsigned int loop = 0;
	struct scatterlist *sg;
	size_t axi_block_len, xfer_len;
	u32 len, num_sgs = 0;
	unsigned int i;
	unsigned long mem;
	int status;
	u64 llp = 0;

	if (unlikely(!is_slave_direction(direction) || !sg_len))
		return NULL;

	mem = sg_dma_address(sgl);
	len = sg_dma_len(sgl);

	dev_dbg(chan2dev(chan),
		"%s: slave_sg: mem: 0x%lx len: %d direction %d\n",
		axi_chan_name(chan), mem, len, direction);

	if (direction == DMA_MEM_TO_DEV) {
		chan->config.src_addr_width = chan->config.dst_addr_width;
	} else {
		chan->config.dst_addr_width = chan->config.src_addr_width;
	}
	axi_block_len =
	    chan->chip->axdma->hinfo->block_size * chan->config.src_addr_width;

	if (axi_block_len == 0)
		return NULL;

	for_each_sg(sgl, sg, sg_len, i)
	    num_sgs += DIV_ROUND_UP(sg_dma_len(sg), axi_block_len);

	desc = axi_desc_alloc(chan->chip, num_sgs);
	if (unlikely(!desc))
		goto err_desc_get;

	desc->chan = chan;
	desc->length = 0;
	chan->direction = direction;
	desc->high_perf = 0;
	for_each_sg(sgl, sg, sg_len, i) {
		mem = sg_dma_address(sg);
		len = sg_dma_len(sg);
		do {
			if (len > axi_block_len)
				xfer_len = axi_block_len;
			else
				xfer_len = len;
			hw_desc = &desc->hw_desc[loop++];
			if (xfer_len & 0x7)
				desc->high_perf = 0;
			status =
			    axi_dma_set_hw_desc(chan, desc, hw_desc, mem,
						xfer_len);
			if (status < 0)
				goto err_desc_get;

			desc->length += hw_desc->len;
			len -= xfer_len;
			mem += xfer_len;
		} while (len);
	}

	/* Set end-of-link to the last link descriptor of list */
	set_desc_last(&desc->hw_desc[num_sgs - 1]);

	/* Managed transfer list */
	do {
		hw_desc = &desc->hw_desc[--num_sgs];
		write_desc_llp(hw_desc, llp);
		llp = hw_desc->llp;
	} while (num_sgs);
#ifdef AX_DMA_PERIPHERAL_DEBUG
	axi_chan_list_dump_lli(chan, desc);
#endif

	return vchan_tx_prep(&chan->vc, &desc->vd, flags);

err_desc_get:
	if (!IS_ERR_OR_NULL(desc))
		axi_desc_put(desc);

	return NULL;
}

static int dma_chan_alloc_chan_resources(struct dma_chan *dchan)
{
#ifndef CONFIG_AX_RISCV_LOAD_ROOTFS
	struct axi_dma_chan *chan = dchan_to_axi_dma_chan(dchan);

	if (unlikely(axi_chan_is_hw_enable(chan))) {
		dev_err(chan2dev(chan), "(%d) %s is non-idle!\n",
			__LINE__, axi_chan_name(chan));
		return -EBUSY;
	}
#endif
	return 0;
}

static void dma_chan_free_chan_resources(struct dma_chan *dchan)
{
	struct axi_dma_chan *chan = dchan_to_axi_dma_chan(dchan);

	/* ASSERT: channel is idle */
	if (unlikely(axi_chan_is_hw_enable(chan)))
		dev_vdbg(dchan2dev(dchan), "(%d) %s is non-idle!\n",
			__LINE__, axi_chan_name(chan));

	axi_chan_disable(chan);
	axi_chan_irq_disable(chan);
	axi_chan_irq_mask_set(chan, 0);
	ax_dma_per_clr_req_id(chan);
	vchan_free_chan_resources(&chan->vc);

}

static struct dma_chan *dma_of_xlate(struct of_phandle_args *dma_spec,
				     struct of_dma *ofdma)
{
	struct ax_dma_per_info *axdma = ofdma->of_dma_data;
	struct axi_dma_chan *chan;
	struct dma_chan *dchan;

	dchan = dma_get_any_slave_channel(&axdma->dma);
	if (!dchan)
		return NULL;
	chan = dchan_to_axi_dma_chan(dchan);
	chan->use_of_dma = 1;
	chan->of_dma_handshake_num = dma_spec->args[0];
	return dchan;
}

static int dma_chan_slave_config(struct dma_chan *dchan,
				 struct dma_slave_config *config)
{
	struct axi_dma_chan *chan = dchan_to_axi_dma_chan(dchan);
	u32 size;

	memcpy(&chan->config, config, sizeof(*config));
	if (chan->config.src_addr_width > AXIDMA_TRANS_WIDTH_MAX) {
		size = chan->config.src_maxburst * chan->config.src_addr_width;
		chan->config.src_addr_width = AXIDMA_TRANS_WIDTH_MAX;
		chan->config.src_maxburst = size / chan->config.src_addr_width;
	}

	if (chan->config.dst_addr_width > AXIDMA_TRANS_WIDTH_MAX) {
		size = chan->config.dst_maxburst * chan->config.dst_addr_width;
		chan->config.dst_addr_width = AXIDMA_TRANS_WIDTH_MAX;
		chan->config.dst_maxburst = size / chan->config.dst_addr_width;
	}

	return 0;
}

static int dma_chan_terminate_all(struct dma_chan *dchan)
{
	struct axi_dma_chan *chan = dchan_to_axi_dma_chan(dchan);
	unsigned long flags;
	int ret;
	LIST_HEAD(head);

	ret = dma_chan_pause(dchan);
	spin_lock_irqsave(&chan->vc.lock, flags);
	axi_dma_reg_set(chan->chip, AX_DMAPER_CLEAR, 1, chan->id,
			AX_DMAPER_CHN_CLR_EN_MASK);
	axi_dma_reg_set(chan->chip, AX_DMAPER_CTRL, 0, chan->id,
			AX_DMAPER_CHN_SUSPEND_EN_MASK);
	axi_chan_disable(chan);
	chan->is_paused = false;
	axi_chan_irq_disable(chan);
	axi_chan_irq_mask_set(chan, 0);
	chan->vc.cyclic = NULL;
	vchan_get_all_descriptors(&chan->vc, &head);
	chan->cyclic = false;
	spin_unlock_irqrestore(&chan->vc.lock, flags);
	vchan_dma_desc_free_list(&chan->vc, &head);

	dev_dbg(dchan2dev(dchan), "terminated: %s\n", axi_chan_name(chan));

	return ret;
}

static void axi_chan_block_xfer_complete(struct axi_dma_chan *chan, u32 status)
{
	struct axi_dma_hw_desc *hw_desc;
	struct axi_dma_desc *desc;
	struct virt_dma_desc *vd;
	unsigned long flags;
	int count;
	u64 llp;
	u32 len;
	int i;

	if (chan->id < AX_DMA_PER_MAX_CHANNELS)
		dmachan_completed_cnt[chan->id]++;
	spin_lock_irqsave(&chan->vc.lock, flags);
	if (chan->cyclic) {
		if (chan->id < AX_DMA_PER_MAX_CHANNELS)
			dmachan_cyclic[chan->id]++;
		vd = vchan_next_desc(&chan->vc);
		if (unlikely(!vd)) {
			spin_unlock_irqrestore(&chan->vc.lock, flags);
			return;
		}
		desc = vd_to_axi_desc(vd);
		if (desc) {
			len = 0;
			count = atomic_read(&desc->descs_allocated);
			llp = desc->hw_desc[desc->cur_llp_index++].llp;
			if (desc->cur_llp_index == count)
				desc->cur_llp_index = 0;
			for (i = 0; i < count; i++) {
				hw_desc = &desc->hw_desc[i];
				len += hw_desc->len;
				if (hw_desc->llp == llp) {
					desc->completed_blocks = i + 1;
					if ((len % desc->period_len) == 0)
						vchan_cyclic_callback(vd);
					break;
				}
			}
		}
	} else {
		axi_chan_disable(chan);
		/* The completed descriptor currently is in the head of vc list */
		vd = vchan_next_desc(&chan->vc);
		if (unlikely(!vd)) {
			spin_unlock_irqrestore(&chan->vc.lock, flags);
			return;
		}
		if (chan->id < AX_DMA_PER_MAX_CHANNELS)
			dmachan_start_callback[chan->id]++;
		/* Remove the completed descriptor from issued list before completing */
		list_del(&vd->node);
		vchan_cookie_complete(vd);

		/* Submit queued descriptors after processing the completed ones */
		axi_chan_start_first_queued(chan);
	}

	if (chan->id < AX_DMA_PER_MAX_CHANNELS)
		dmachan_intr_end[chan->id]++;
	spin_unlock_irqrestore(&chan->vc.lock, flags);
}

static void ax_dma_tasklet(struct ax_dma_per_chip *chip)
{
	struct ax_dma_per_info *axdma = chip->axdma;
	u32 status, chn_status, i;

	status = axi_dma_irq_read(chip);
	for (i = 0; i < AX_DMA_PER_MAX_CHANNELS; i++) {
		if (status & (1 << i)) {
			chn_status = axi_chan_irq_read(&axdma->chans[i]);
			axi_chan_irq_clear(&axdma->chans[i], chn_status);
			axi_chan_block_xfer_complete(&axdma->chans[i], chn_status);
		}
	}
}

static irqreturn_t ax_dma_per_interrupt(int irq, void *dev_id)
{
	struct ax_dma_per_chip *chip = dev_id;
#ifdef CONFIG_AX_RISCV_LOAD_ROOTFS
	if (!chip->en)
		return IRQ_HANDLED;
#endif
	dev_dbg(chip->dev, "dma per glb irq status %d\n",
		axi_dma_irq_read(chip));
	disable_irq_nosync(irq);
	ax_dma_tasklet(chip);
	enable_irq(irq);
	return IRQ_HANDLED;
}

static int parse_device_properties(struct ax_dma_per_chip *chip)
{
	struct device *dev = chip->dev;
	u32 tmp;
	int ret;

	ret = device_property_read_u32(dev, "dma-channels", &tmp);
	if (ret)
		return ret;
	if (tmp == 0 || tmp > AX_DMA_PER_MAX_CHANNELS)
		return -EINVAL;

	chip->axdma->hinfo->nr_channels = tmp;

	ret = device_property_read_u32(dev, "data-width", &tmp);
	if (ret)
		return ret;
	if (tmp > AXIDMA_TRANS_WIDTH_MAX * 8)
		return -EINVAL;

	chip->axdma->hinfo->data_width = tmp;

	ret = device_property_read_u32(dev, "block-size", &tmp);
	if (ret)
		return ret;

	if (tmp == 0 || tmp > AX_DMA_PER_MAX_BLK_SIZE)
		return -EINVAL;
	chip->axdma->hinfo->block_size = tmp;

	/* axi-max-burst-len is optional property */
	ret = device_property_read_u32(dev, "axi-max-burst-len", &tmp);
	if (!ret) {
		if (tmp > AXIDMA_ARWLEN_MAX)
			return -EINVAL;
		if (tmp < AXIDMA_ARWLEN_MIN)
			return -EINVAL;

		chip->axdma->hinfo->restrict_axi_burst_len = true;
		chip->axdma->hinfo->axi_rw_burst_len = tmp;
	}

	return 0;
}

static int ax_dma_per_probe(struct platform_device *pdev)
{
	struct ax_dma_per_chip *chip;
	struct resource *mem;
	struct ax_dma_per_info *axdma;
	struct axi_dma_hw_info *hinfo;
	u32 i;
	int ret;

	chip = devm_kzalloc(&pdev->dev, sizeof(*chip), GFP_KERNEL);
	if (!chip)
		return -ENOMEM;

	axdma = devm_kzalloc(&pdev->dev, sizeof(*axdma), GFP_KERNEL);
	if (!axdma)
		return -ENOMEM;

	hinfo = devm_kzalloc(&pdev->dev, sizeof(*hinfo), GFP_KERNEL);
	if (!hinfo)
		return -ENOMEM;

	spin_lock_init(&chip->lock);
	chip->axdma = axdma;
	chip->dev = &pdev->dev;
	chip->axdma->hinfo = hinfo;

	of_dma_configure(chip->dev, chip->dev->of_node, true);
	dma_set_mask_and_coherent(&pdev->dev, DMA_BIT_MASK(37));

	chip->irq = platform_get_irq(pdev, 0);
	if (chip->irq < 0)
		return chip->irq;
	mem = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	chip->regs = devm_ioremap_resource(chip->dev, mem);
	if (IS_ERR_OR_NULL(chip->regs)) {
		dev_err(chip->dev, "devm_ioremap_resource regs\n");
		return -ENOMEM;
	}
	chip->paddr = mem->start;

	chip->rst = devm_reset_control_get_optional(&pdev->dev, "dmaper-arst");
	if (IS_ERR_OR_NULL(chip->rst)) {
		dev_err(chip->dev, "get rst fail\n");
		return -EINVAL;
	}

	chip->prst = devm_reset_control_get_optional(&pdev->dev, "dmaper-prst");
	if (IS_ERR_OR_NULL(chip->prst)) {
		dev_err(chip->dev, "get prst fail\n");
		return -EINVAL;
	}

	chip->clk = devm_clk_get(chip->dev, "dmaper-aclk");
	if (IS_ERR_OR_NULL(chip->clk)) {
		dev_err(chip->dev, "get clk fail\n");
		return -EINVAL;
	}

	chip->pclk = devm_clk_get(chip->dev, "dmaper-pclk");
	if (IS_ERR_OR_NULL(chip->pclk)) {
		dev_err(chip->dev, "get pclk fail\n");
		return -EINVAL;
	}

	ret = parse_device_properties(chip);
	if (ret) {
		dev_err(chip->dev, "parse_device_properties failed\n");
		return ret;
	}

	mem = platform_get_resource(pdev, IORESOURCE_MEM, 1);
	chip->req_regs = ioremap(mem->start, resource_size(mem));
	if (IS_ERR_OR_NULL(chip->req_regs)) {
		dev_err(chip->dev, "devm_ioremap_resource req_regs\n");
		return -ENOMEM;
	}
	chip->req_paddr = mem->start;

	axdma->chans = devm_kcalloc(chip->dev, hinfo->nr_channels,
				    sizeof(*axdma->chans), GFP_KERNEL);
	if (!axdma->chans) {
		dev_err(chip->dev, "devm_kcalloc failed\n");
		ret = -ENOMEM;
		goto err_ioremap;
	}

	INIT_LIST_HEAD(&axdma->dma.channels);
	for (i = 0; i < hinfo->nr_channels; i++) {
		struct axi_dma_chan *chan = &axdma->chans[i];
		chan->chip = chip;
		chan->id = i;
#ifndef CONFIG_AX_RISCV_LOAD_ROOTFS
		/* set default req id */
		ax_dma_per_set_def_req_id(chan);
#endif
		chan->vc.desc_free = vchan_desc_put;
		vchan_init(&chan->vc, &axdma->dma);
	}

	/* Set capabilities */
	dma_cap_set(DMA_SLAVE, axdma->dma.cap_mask);
	dma_cap_set(DMA_CYCLIC, axdma->dma.cap_mask);

	/* DMA capabilities */
	axdma->dma.chancnt = hinfo->nr_channels;
	axdma->dma.max_burst = hinfo->axi_rw_burst_len;
	axdma->dma.src_addr_widths = AXI_DMA_BUSWIDTHS;
	axdma->dma.dst_addr_widths = AXI_DMA_BUSWIDTHS;
	axdma->dma.directions = BIT(DMA_MEM_TO_DEV) | BIT(DMA_DEV_TO_MEM);
	axdma->dma.residue_granularity = DMA_RESIDUE_GRANULARITY_BURST;

	axdma->dma.dev = chip->dev;
	axdma->dma.device_tx_status = dma_chan_tx_status;
	axdma->dma.device_issue_pending = dma_chan_issue_pending;
	axdma->dma.device_terminate_all = dma_chan_terminate_all;
	axdma->dma.device_pause = dma_chan_pause;
	axdma->dma.device_resume = dma_chan_resume;

	axdma->dma.device_alloc_chan_resources = dma_chan_alloc_chan_resources;
	axdma->dma.device_free_chan_resources = dma_chan_free_chan_resources;

	axdma->dma.device_config = dma_chan_slave_config;
	axdma->dma.device_prep_slave_sg = dma_chan_prep_slave_sg;
	axdma->dma.device_prep_dma_cyclic = dma_chan_prep_cyclic;

	axdma->dma.dev->dma_parms = &axdma->dma_parms;
	platform_set_drvdata(pdev, chip);

	ax_dma_per_pclk_en(chip, 1);
	ax_dma_per_chan_clk_en(&axdma->chans[0], 1);
#ifndef CONFIG_AX_RISCV_LOAD_ROOTFS
	ret = ax_dma_per_resume(chip);
	if (ret < 0) {
		dev_err(chip->dev, "ax_dma_per_resume failed\n");
		goto err_disable;
	}
#endif
#ifdef CONFIG_AX_RISCV_LOAD_ROOTFS
	chip->en = 0;
#endif
	ret = devm_request_irq(chip->dev, chip->irq, ax_dma_per_interrupt,
			       IRQF_SHARED, KBUILD_MODNAME, chip);
	if (ret) {
		dev_err(chip->dev, "devm_request_irq failed\n");
		goto err_disable;
	}
#ifdef CONFIG_AX_RISCV_LOAD_ROOTFS
	disable_irq_nosync(chip->irq);
#endif
	/* LLI address must be aligned to a 64-byte boundary */
	chip->desc_pool = dma_pool_create(dev_name(chip->dev), chip->dev,
					  sizeof(struct axi_dma_lli), 64, 0);
	if (!chip->desc_pool) {
		dev_err(chip->dev, "No memory for descriptors\n");
		goto err_disable;
	}

	ret = dmaenginem_async_device_register(&axdma->dma);
	if (ret) {
		dev_err(chip->dev, "dmaenginem_async_device_register failed\n");
		goto err_disable;
	}

	/* Register with OF helpers for DMA lookups */
	ret = of_dma_controller_register(pdev->dev.of_node,
					 dma_of_xlate, axdma);
	if (ret < 0) {
		dev_err(&pdev->dev, "Failed to register OF DMA controller\n");
		goto err_disable;
	}

	chip->desc_kmem = kmem_cache_create(dev_name(chip->dev), sizeof(struct axi_dma_desc), 0, 0, NULL);
	if (!chip->desc_kmem)
		goto err_disable;
	dev_info(chip->dev, "Axera DMA Peripherals Controller, %d channels\n",
		 axdma->hinfo->nr_channels);
	return 0;

err_disable:
err_ioremap:
	iounmap(chip->req_regs);
	return ret;
}

static int ax_dma_per_remove(struct platform_device *pdev)
{
	struct ax_dma_per_chip *chip = platform_get_drvdata(pdev);
	struct ax_dma_per_info *axdma = chip->axdma;
	struct axi_dma_chan *chan, *_chan;

	ax_dma_per_pclk_en(chip, 0);
	ax_dma_per_chan_clk_en(&axdma->chans[0], 0);
	devm_free_irq(chip->dev, chip->irq, chip);
	of_dma_controller_free(chip->dev->of_node);

	list_for_each_entry_safe(chan, _chan, &axdma->dma.channels,
				 vc.chan.device_node) {
		list_del(&chan->vc.chan.device_node);
		tasklet_kill(&chan->vc.task);
	}
	kmem_cache_destroy(chip->desc_kmem);
	return 0;
}

static int __maybe_unused ax_dma_per_runtime_suspend(struct device *dev)
{
	struct ax_dma_per_chip *chip = dev_get_drvdata(dev);

	return ax_dma_per_suspend(chip);
}

static int __maybe_unused ax_dma_per_runtime_resume(struct device *dev)
{
	struct ax_dma_per_chip *chip = dev_get_drvdata(dev);

	return ax_dma_per_resume(chip);
}

static const struct dev_pm_ops ax_dma_per_pm_ops = {
	SET_SYSTEM_SLEEP_PM_OPS(ax_dma_per_runtime_suspend,
				ax_dma_per_runtime_resume)
};

static const struct of_device_id ax_dma_per_of_id_table[] = {
	{.compatible = "axera,dma-per"},
	{}
};

MODULE_DEVICE_TABLE(of, ax_dma_per_of_id_table);

static struct platform_driver ax_dma_per_driver = {
	.probe = ax_dma_per_probe,
	.remove = ax_dma_per_remove,
	.driver = {
		   .name = AX_DMA_PER_DRV,
		   .of_match_table = of_match_ptr(ax_dma_per_of_id_table),
		   .pm = &ax_dma_per_pm_ops,
		   },
};

static int __init ax_dma_per_init(void)
{
	return platform_driver_register(&ax_dma_per_driver);
}

subsys_initcall_sync(ax_dma_per_init);

static void __exit ax_dma_per_exit(void)
{
	platform_driver_unregister(&ax_dma_per_driver);
}

module_exit(ax_dma_per_exit);

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("AXERA Peripherals DMA Controller driver");
MODULE_AUTHOR("AXERA");
