// SPDX-License-Identifier: GPL-2.0
/* FH8626 hardware linked-list DMA for cyclic peripheral streams. */
#include <linux/clk.h>
#include <linux/dma-mapping.h>
#include <linux/dmaengine.h>
#include <linux/dmapool.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/iopoll.h>
#include <linux/module.h>
#include <linux/of_dma.h>
#include <linux/platform_device.h>
#include <linux/slab.h>

#include "dmaengine.h"
/* Register/LLI definitions only; no dependency on the DW driver. */
#include "dw/internal.h"

struct fh_dma_lli {
	struct dw_lli hw;
	dma_addr_t phys;
	struct fh_dma_lli *next;
};

struct fh_dma_desc {
	struct dma_async_tx_descriptor tx;
	struct fh_dma_lli *first;
	dma_addr_t buffer;
	size_t bytes;
	enum dma_transfer_direction direction;
};

struct fh_dma;

struct fh_dma_chan {
	struct dma_chan chan;
	struct fh_dma *dma;
	struct dw_dma_chan_regs __iomem *regs;
	spinlock_t lock;
	struct tasklet_struct tasklet;
	struct dma_slave_config config;
	struct fh_dma_desc *desc;
	u32 mask;
	u32 block_size;
	u8 request;
	u8 mem_master;
	u8 per_master;
	bool nollp;
	bool running;
	bool paused;
	bool stopping;
	bool error;
};

struct fh_dma {
	struct dma_device engine;
	struct device *dev;
	struct dw_dma_regs __iomem *regs;
	struct clk *clk;
	struct dma_pool *pool;
	struct resource *resource;
	int irq;
	u8 channels;
	u8 masters;
	u8 width[4];
	bool removing;
	struct fh_dma_chan chan[DW_DMA_MAX_NR_CHANNELS];
};

static struct fh_dma_chan *fh_dma_chan(struct dma_chan *chan)
{
	return container_of(chan, struct fh_dma_chan, chan);
}

static void fh_dma_mask(struct fh_dma_chan *c)
{
	writel(c->mask << 8, &c->dma->regs->MASK.BLOCK);
	writel(c->mask << 8, &c->dma->regs->MASK.XFER);
	writel(c->mask << 8, &c->dma->regs->MASK.ERROR);
}

static void fh_dma_clear(struct fh_dma_chan *c)
{
	writel(c->mask, &c->dma->regs->CLEAR.BLOCK);
	writel(c->mask, &c->dma->regs->CLEAR.XFER);
	writel(c->mask, &c->dma->regs->CLEAR.ERROR);
}

static void fh_dma_unmask(struct fh_dma_chan *c)
{
	writel(c->mask << 8 | c->mask, &c->dma->regs->MASK.BLOCK);
	writel(c->mask << 8 | c->mask, &c->dma->regs->MASK.XFER);
	writel(c->mask << 8 | c->mask, &c->dma->regs->MASK.ERROR);
}

static void fh_dma_free_desc(struct fh_dma_chan *c, struct fh_dma_desc *d)
{
	struct fh_dma_lli *lli, *next;

	if (!d)
		return;
	for (lli = d->first; lli; lli = next) {
		next = lli->next;
		dma_pool_free(c->dma->pool, lli, lli->phys);
	}
	kfree(d);
}

/* The caller owns the channel; a stopping channel cannot be restarted. */
static int fh_dma_stop(struct fh_dma_chan *c, bool atomic)
{
	u32 val;

	fh_dma_mask(c);
	writel(readl(&c->regs->CFG_LO) | DWC_CFGL_CH_SUSP,
	       &c->regs->CFG_LO);
	writel(c->mask << 8, &c->dma->regs->CH_EN);
	if (atomic)
		return readl_poll_timeout_atomic(&c->dma->regs->CH_EN, val,
						!(val & c->mask), 1, 1000);
	return readl_poll_timeout(&c->dma->regs->CH_EN, val,
				  !(val & c->mask), 10, 10000);
}

/* Only detach after CH_EN clears. Cookie completion means memory is safe. */
static struct fh_dma_desc *fh_dma_retire(struct fh_dma_chan *c)
{
	struct fh_dma_desc *d = c->desc;

	fh_dma_clear(c);
	c->desc = NULL;
	c->running = false;
	c->paused = false;
	c->stopping = false;
	c->error = false;
	if (d && d->tx.cookie > 0)
		dma_cookie_complete(&d->tx);
	return d;
}

static int fh_dma_terminate(struct dma_chan *chan)
{
	struct fh_dma_chan *c = fh_dma_chan(chan);
	struct fh_dma_desc *d = NULL;
	unsigned long flags;
	int ret;

	spin_lock_irqsave(&c->lock, flags);
	c->stopping = true;
	ret = fh_dma_stop(c, true);
	if (ret)
		c->error = true;
	else
		d = fh_dma_retire(c);
	spin_unlock_irqrestore(&c->lock, flags);
	fh_dma_free_desc(c, d);
	return ret;
}

static void fh_dma_synchronize(struct dma_chan *chan)
{
	struct fh_dma_chan *c = fh_dma_chan(chan);
	struct fh_dma_desc *d = NULL;
	unsigned long flags;
	bool retry;

	/* Drain any callback already copied before terminate acquired lock. */
	disable_irq(c->dma->irq);
	tasklet_kill(&c->tasklet);
	enable_irq(c->dma->irq);
	spin_lock_irqsave(&c->lock, flags);
	retry = c->stopping;
	spin_unlock_irqrestore(&c->lock, flags);
	if (!retry)
		return;
	if (fh_dma_stop(c, false)) {
		dev_err(c->dma->dev, "channel %u remains active; retaining ring\n",
			chan->chan_id);
		return;
	}
	spin_lock_irqsave(&c->lock, flags);
	d = fh_dma_retire(c);
	spin_unlock_irqrestore(&c->lock, flags);
	fh_dma_free_desc(c, d);
}

static dma_cookie_t fh_dma_submit(struct dma_async_tx_descriptor *tx)
{
	struct fh_dma_chan *c = fh_dma_chan(tx->chan);
	unsigned long flags;
	dma_cookie_t cookie;

	spin_lock_irqsave(&c->lock, flags);
	if (!c->desc || &c->desc->tx != tx || c->stopping || tx->cookie > 0)
		cookie = -EBUSY;
	else
		cookie = dma_cookie_assign(tx);
	spin_unlock_irqrestore(&c->lock, flags);
	return cookie;
}

static int fh_dma_config(struct dma_chan *chan, struct dma_slave_config *cfg)
{
	struct fh_dma_chan *c = fh_dma_chan(chan);
	unsigned long flags;
	int ret = 0;

	if (cfg->device_fc)
		return -EINVAL;
	spin_lock_irqsave(&c->lock, flags);
	if (c->desc || c->stopping || READ_ONCE(c->dma->removing))
		ret = -EBUSY;
	else
		c->config = *cfg;
	spin_unlock_irqrestore(&c->lock, flags);
	return ret;
}

static struct dma_async_tx_descriptor *
fh_dma_prep_cyclic(struct dma_chan *chan, dma_addr_t buffer, size_t bytes,
		   size_t period, enum dma_transfer_direction direction,
		   unsigned long txflags)
{
	struct fh_dma_chan *c = fh_dma_chan(chan);
	struct fh_dma_desc *d = NULL;
	struct fh_dma_lli *prev = NULL;
	struct dma_slave_config *cfg = &c->config;
	dma_addr_t peripheral;
	unsigned long flags;
	u32 width, burst, ctl;
	size_t count, i;

	if ((direction != DMA_MEM_TO_DEV && direction != DMA_DEV_TO_MEM) ||
	    !bytes || !period || bytes % period || upper_32_bits(buffer) ||
	    bytes - 1 > U32_MAX - (u32)buffer)
		return NULL;
	count = bytes / period;
	if (count < 2 || count > 1024)
		return NULL;

	spin_lock_irqsave(&c->lock, flags);
	if (c->desc || c->stopping || c->nollp || READ_ONCE(c->dma->removing))
		goto out;
	width = direction == DMA_MEM_TO_DEV ? cfg->dst_addr_width :
		cfg->src_addr_width;
	peripheral = direction == DMA_MEM_TO_DEV ? cfg->dst_addr : cfg->src_addr;
	burst = direction == DMA_MEM_TO_DEV ? cfg->dst_maxburst : cfg->src_maxburst;
	if ((width != 1 && width != 2 && width != 4) ||
	    upper_32_bits(peripheral) ||
	    ((buffer | peripheral | period) & (width - 1)) ||
	    width > c->dma->width[c->mem_master] ||
	    width > c->dma->width[c->per_master] ||
	    (burst != 1 && burst != 4 && burst != 8 && burst != 16 &&
	     burst != 32 && burst != 64 && burst != 128 && burst != 256))
		goto out;
	width = __ffs(width);
	if ((period >> width) > c->block_size)
		goto out;
	d = kzalloc(sizeof(*d), GFP_ATOMIC);
	if (!d)
		goto out;
	d->buffer = buffer;
	d->bytes = bytes;
	d->direction = direction;
	burst = burst > 1 ? fls(burst) - 2 : 0;
	ctl = DWC_CTLL_LLP_D_EN | DWC_CTLL_LLP_S_EN | DWC_CTLL_INT_EN |
		DWC_CTLL_SRC_WIDTH(width) | DWC_CTLL_DST_WIDTH(width) |
		DWC_CTLL_SRC_MSIZE(burst) | DWC_CTLL_DST_MSIZE(burst);
	if (direction == DMA_MEM_TO_DEV)
		ctl |= DWC_CTLL_DST_FIX | DWC_CTLL_SRC_INC | DWC_CTLL_FC_M2P |
			DWC_CTLL_SMS(c->mem_master) | DWC_CTLL_DMS(c->per_master);
	else
		ctl |= DWC_CTLL_SRC_FIX | DWC_CTLL_DST_INC | DWC_CTLL_FC_P2M |
			DWC_CTLL_SMS(c->per_master) | DWC_CTLL_DMS(c->mem_master);
	for (i = 0; i < count; i++) {
		dma_addr_t mem = buffer + i * period;
		dma_addr_t phys;
		struct fh_dma_lli *lli;

		lli = dma_pool_zalloc(c->dma->pool, GFP_ATOMIC, &phys);
		if (!lli) {
			fh_dma_free_desc(c, d);
			d = NULL;
			goto out;
		}
		lli->phys = phys;
		lli->hw.sar = cpu_to_le32(direction == DMA_MEM_TO_DEV ? mem : peripheral);
		lli->hw.dar = cpu_to_le32(direction == DMA_DEV_TO_MEM ? mem : peripheral);
		lli->hw.ctllo = cpu_to_le32(ctl);
		lli->hw.ctlhi = cpu_to_le32(period >> width);
		if (prev) {
			prev->next = lli;
			prev->hw.llp = cpu_to_le32(phys | DWC_LLP_LMS(c->mem_master));
		} else {
			d->first = lli;
		}
		prev = lli;
	}
	prev->hw.llp = cpu_to_le32(d->first->phys | DWC_LLP_LMS(c->mem_master));
	dma_async_tx_descriptor_init(&d->tx, chan);
	d->tx.tx_submit = fh_dma_submit;
	d->tx.flags = txflags;
	d->tx.phys = d->first->phys;
	c->desc = d;
out:
	spin_unlock_irqrestore(&c->lock, flags);
	return d ? &d->tx : NULL;
}

static void fh_dma_issue_pending(struct dma_chan *chan)
{
	struct fh_dma_chan *c = fh_dma_chan(chan);
	unsigned long flags;

	spin_lock_irqsave(&c->lock, flags);
	if (!c->desc || c->desc->tx.cookie <= 0 || c->running || c->stopping ||
	    READ_ONCE(c->dma->removing))
		goto out;
	fh_dma_clear(c);
	writel(DWC_CFGL_CH_PRIOR(c->dma->channels - chan->chan_id - 1),
	       &c->regs->CFG_LO);
	writel(DWC_CFGH_FIFO_MODE | DWC_CFGH_SRC_PER(c->request) |
	       DWC_CFGH_DST_PER(c->request), &c->regs->CFG_HI);
	dma_wmb();
	writel(c->desc->first->phys | DWC_LLP_LMS(c->mem_master), &c->regs->LLP);
	writel(DWC_CTLL_LLP_D_EN | DWC_CTLL_LLP_S_EN, &c->regs->CTL_LO);
	writel(0, &c->regs->CTL_HI);
	c->running = true;
	fh_dma_unmask(c);
	writel(c->mask << 8 | c->mask, &c->dma->regs->CH_EN);
out:
	spin_unlock_irqrestore(&c->lock, flags);
}

static enum dma_status fh_dma_status(struct dma_chan *chan, dma_cookie_t cookie,
				    struct dma_tx_state *state)
{
	struct fh_dma_chan *c = fh_dma_chan(chan);
	unsigned long flags;
	enum dma_status status;
	u32 residue = 0;

	spin_lock_irqsave(&c->lock, flags);
	status = dma_cookie_status(chan, cookie, state);
	if (c->desc && c->desc->tx.cookie == cookie) {
		struct fh_dma_desc *d = c->desc;
		u32 pos = d->direction == DMA_MEM_TO_DEV ? readl(&c->regs->SAR) :
			readl(&c->regs->DAR);
		u32 offset = pos - d->buffer;

		residue = c->running && offset < d->bytes ? d->bytes - offset : d->bytes;
		if (c->error || c->stopping)
			status = DMA_ERROR;
		else if (c->paused)
			status = DMA_PAUSED;
	}
	dma_set_residue(state, residue);
	spin_unlock_irqrestore(&c->lock, flags);
	return status;
}

static int fh_dma_pause(struct dma_chan *chan)
{
	struct fh_dma_chan *c = fh_dma_chan(chan);
	unsigned long flags;
	u32 val;
	int ret;

	spin_lock_irqsave(&c->lock, flags);
	if (!c->running || c->stopping || c->error) {
		ret = -EINVAL;
		goto out;
	}
	writel(readl(&c->regs->CFG_LO) | DWC_CFGL_CH_SUSP, &c->regs->CFG_LO);
	ret = readl_poll_timeout_atomic(&c->regs->CFG_LO, val,
					val & DWC_CFGL_FIFO_EMPTY, 2, 40);
	if (ret)
		writel(readl(&c->regs->CFG_LO) & ~DWC_CFGL_CH_SUSP, &c->regs->CFG_LO);
	else
		c->paused = true;
out:
	spin_unlock_irqrestore(&c->lock, flags);
	return ret;
}

static int fh_dma_resume(struct dma_chan *chan)
{
	struct fh_dma_chan *c = fh_dma_chan(chan);
	unsigned long flags;
	int ret = 0;

	spin_lock_irqsave(&c->lock, flags);
	if (!c->running || c->stopping || c->error)
		ret = -EINVAL;
	else {
		writel(readl(&c->regs->CFG_LO) & ~DWC_CFGL_CH_SUSP, &c->regs->CFG_LO);
		c->paused = false;
	}
	spin_unlock_irqrestore(&c->lock, flags);
	return ret;
}

static void fh_dma_tasklet(unsigned long data)
{
	struct fh_dma_chan *c = (struct fh_dma_chan *)data;
	struct dmaengine_desc_callback cb = { };
	struct dmaengine_result result = { .result = DMA_TRANS_NOERROR };
	unsigned long flags;
	u32 error, block;

	spin_lock_irqsave(&c->lock, flags);
	error = (readl(&c->dma->regs->RAW.ERROR) |
		 readl(&c->dma->regs->RAW.XFER)) & c->mask;
	block = readl(&c->dma->regs->RAW.BLOCK) & c->mask;
	fh_dma_clear(c);
	if (!c->desc || !c->running || c->stopping || c->error)
		goto out;
	if (error) {
		c->error = true;
		fh_dma_stop(c, true);
		result.result = DMA_TRANS_ABORTED;
		result.residue = c->desc->bytes;
		dmaengine_desc_get_callback(&c->desc->tx, &cb);
	} else {
		if (block && (c->desc->tx.flags & DMA_PREP_INTERRUPT))
			dmaengine_desc_get_callback(&c->desc->tx, &cb);
		fh_dma_unmask(c);
	}
out:
	spin_unlock_irqrestore(&c->lock, flags);
	dmaengine_desc_callback_invoke(&cb, &result);
}

static irqreturn_t fh_dma_irq(int irq, void *data)
{
	struct fh_dma *d = data;
	u32 pending = readl(&d->regs->STATUS.BLOCK) |
		readl(&d->regs->STATUS.ERROR) | readl(&d->regs->STATUS.XFER);
	unsigned int i;

	if (!pending)
		return IRQ_NONE;
	for (i = 0; i < d->channels; i++) {
		struct fh_dma_chan *c = &d->chan[i];

		if (!(pending & c->mask))
			continue;
		fh_dma_mask(c);
		tasklet_schedule(&c->tasklet);
	}
	return IRQ_HANDLED;
}

static int fh_dma_alloc_chan(struct dma_chan *chan)
{
	struct fh_dma_chan *c = fh_dma_chan(chan);

	if (c->desc || c->stopping || READ_ONCE(c->dma->removing) ||
	    (readl(&c->dma->regs->CH_EN) & c->mask))
		return -EBUSY;
	dma_cookie_init(chan);
	return 0;
}

static void fh_dma_free_chan(struct dma_chan *chan)
{
	fh_dma_terminate(chan);
	fh_dma_synchronize(chan);
}

static struct dma_chan *fh_dma_xlate(struct of_phandle_args *args,
				    struct of_dma *ofdma)
{
	struct fh_dma *d = ofdma->of_dma_data;
	unsigned int i;

	if (args->args_count != 3 || args->args[0] >= DW_DMA_MAX_NR_REQUESTS ||
	    args->args[1] >= d->masters || args->args[2] >= d->masters)
		return NULL;
	for (i = 0; i < d->channels; i++) {
		struct fh_dma_chan *c = &d->chan[i];
		struct dma_chan *chan;

		if (c->nollp)
			continue;
		chan = dma_get_slave_channel(&c->chan);
		if (!chan)
			continue;
		c->request = args->args[0];
		c->mem_master = args->args[1];
		c->per_master = args->args[2];
		return chan;
	}
	return NULL;
}

static int fh_dma_off(struct fh_dma *d)
{
	u32 val, mask = ((1 << d->channels) - 1) << 8;

	writel(mask, &d->regs->MASK.XFER);
	writel(mask, &d->regs->MASK.BLOCK);
	writel(mask, &d->regs->MASK.SRC_TRAN);
	writel(mask, &d->regs->MASK.DST_TRAN);
	writel(mask, &d->regs->MASK.ERROR);
	writel(0, &d->regs->CFG);
	return readl_poll_timeout(&d->regs->CFG, val, !(val & DW_CFG_DMA_EN),
				  10, 10000);
}

static int fh_dma_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct fh_dma *d;
	struct resource *res;
	u32 params, blocks;
	unsigned int i;
	int ret;

	ret = dma_set_mask_and_coherent(dev, DMA_BIT_MASK(32));
	if (ret)
		return ret;
	d = kzalloc(sizeof(*d), GFP_KERNEL);
	if (!d)
		return -ENOMEM;
	d->dev = get_device(dev);
	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res) {
		ret = -EINVAL;
		goto free_dma;
	}
	d->resource = request_mem_region(res->start, resource_size(res), dev_name(dev));
	if (!d->resource) {
		ret = -EBUSY;
		goto free_dma;
	}
	d->regs = ioremap(res->start, resource_size(res));
	if (!d->regs) {
		ret = -ENOMEM;
		goto release_region;
	}
	d->clk = clk_get(dev, "hclk");
	if (IS_ERR(d->clk)) {
		ret = PTR_ERR(d->clk);
		goto unmap;
	}
	ret = clk_prepare_enable(d->clk);
	if (ret)
		goto put_clk;
	params = readl(&d->regs->DW_PARAMS);
	if (!(params & BIT(DW_PARAMS_EN))) {
		ret = -EINVAL;
		goto disable_clk;
	}
	d->channels = ((params >> DW_PARAMS_NR_CHAN) & 7) + 1;
	d->masters = ((params >> DW_PARAMS_NR_MASTER) & 3) + 1;
	for (i = 0; i < d->masters; i++)
		d->width[i] = 4 << ((params >> DW_PARAMS_DATA_WIDTH(i)) & 3);
	ret = fh_dma_off(d);
	if (ret)
		goto disable_clk;
	d->pool = dma_pool_create("fh8626-lli", dev, sizeof(struct fh_dma_lli), 4, 0);
	if (!d->pool) {
		ret = -ENOMEM;
		goto disable_clk;
	}
	blocks = readl(&d->regs->MAX_BLK_SIZE);
	INIT_LIST_HEAD(&d->engine.channels);
	for (i = 0; i < d->channels; i++) {
		struct fh_dma_chan *c = &d->chan[i];
		u32 cp = readl(&d->regs->DWC_PARAMS[d->channels - i - 1]);
		u32 block_shift = (blocks >> (4 * i)) & 0xf;

		c->dma = d;
		c->regs = &d->regs->CHAN[i];
		c->mask = BIT(i);
		c->block_size = (4 << block_shift) - 1;
		c->nollp = !(cp & BIT(DWC_PARAMS_MBLK_EN)) || block_shift > 10;
		spin_lock_init(&c->lock);
		tasklet_init(&c->tasklet, fh_dma_tasklet, (unsigned long)c);
		c->chan.device = &d->engine;
		dma_cookie_init(&c->chan);
		list_add_tail(&c->chan.device_node, &d->engine.channels);
		fh_dma_clear(c);
	}
	d->irq = platform_get_irq(pdev, 0);
	if (d->irq < 0) {
		ret = d->irq;
		goto destroy_pool;
	}
	ret = request_irq(d->irq, fh_dma_irq, 0, dev_name(dev), d);
	if (ret)
		goto destroy_pool;
	d->engine.dev = dev;
	dma_cap_set(DMA_CYCLIC, d->engine.cap_mask);
	dma_cap_set(DMA_PRIVATE, d->engine.cap_mask);
	d->engine.device_alloc_chan_resources = fh_dma_alloc_chan;
	d->engine.device_free_chan_resources = fh_dma_free_chan;
	d->engine.device_config = fh_dma_config;
	d->engine.device_prep_dma_cyclic = fh_dma_prep_cyclic;
	d->engine.device_issue_pending = fh_dma_issue_pending;
	d->engine.device_tx_status = fh_dma_status;
	d->engine.device_terminate_all = fh_dma_terminate;
	d->engine.device_synchronize = fh_dma_synchronize;
	d->engine.device_pause = fh_dma_pause;
	d->engine.device_resume = fh_dma_resume;
	d->engine.src_addr_widths = BIT(DMA_SLAVE_BUSWIDTH_1_BYTE) |
		BIT(DMA_SLAVE_BUSWIDTH_2_BYTES) | BIT(DMA_SLAVE_BUSWIDTH_4_BYTES);
	d->engine.dst_addr_widths = d->engine.src_addr_widths;
	d->engine.directions = BIT(DMA_MEM_TO_DEV) | BIT(DMA_DEV_TO_MEM);
	d->engine.residue_granularity = DMA_RESIDUE_GRANULARITY_BURST;
	d->engine.max_burst = 256;
	ret = dma_async_device_register(&d->engine);
	if (ret)
		goto free_irq;
	writel(DW_CFG_DMA_EN, &d->regs->CFG);
	ret = of_dma_controller_register(dev->of_node, fh_dma_xlate, d);
	if (ret)
		goto unregister_dma;
	platform_set_drvdata(pdev, d);
	return 0;

unregister_dma:
	fh_dma_off(d);
	dma_async_device_unregister(&d->engine);
free_irq:
	free_irq(d->irq, d);
	for (i = 0; i < d->channels; i++)
		tasklet_kill(&d->chan[i].tasklet);
destroy_pool:
	dma_pool_destroy(d->pool);
disable_clk:
	clk_disable_unprepare(d->clk);
put_clk:
	clk_put(d->clk);
unmap:
	iounmap(d->regs);
release_region:
	release_mem_region(res->start, resource_size(res));
free_dma:
	put_device(d->dev);
	kfree(d);
	return ret;
}

static int fh_dma_remove(struct platform_device *pdev)
{
	struct fh_dma *d = platform_get_drvdata(pdev);
	unsigned int i;
	bool held = false;
	int ret;

	WRITE_ONCE(d->removing, true);
	of_dma_controller_free(pdev->dev.of_node);
	for (i = 0; i < d->channels; i++) {
		fh_dma_terminate(&d->chan[i].chan);
		fh_dma_synchronize(&d->chan[i].chan);
		held |= !!d->chan[i].chan.client_count;
	}
	ret = fh_dma_off(d);
	/* No devres may reclaim an outstanding ring or a client's channel. */
	if (ret || held) {
		dev_err(d->dev, "DMA shutdown incomplete; retaining hardware resources\n");
		return ret ? ret : -EBUSY;
	}
	dma_async_device_unregister(&d->engine);
	free_irq(d->irq, d);
	for (i = 0; i < d->channels; i++)
		tasklet_kill(&d->chan[i].tasklet);
	for (i = 0; i < d->channels; i++)
		fh_dma_free_desc(&d->chan[i], d->chan[i].desc);
	dma_pool_destroy(d->pool);
	clk_disable_unprepare(d->clk);
	clk_put(d->clk);
	iounmap(d->regs);
	release_mem_region(d->resource->start, resource_size(d->resource));
	put_device(d->dev);
	kfree(d);
	return 0;
}

static void fh_dma_shutdown(struct platform_device *pdev)
{
	struct fh_dma *d = platform_get_drvdata(pdev);
	unsigned int i;

	for (i = 0; i < d->channels; i++) {
		fh_dma_terminate(&d->chan[i].chan);
		fh_dma_synchronize(&d->chan[i].chan);
	}
	if (fh_dma_off(d))
		dev_err(d->dev, "DMA still active at shutdown\n");
}

static int __maybe_unused fh_dma_suspend(struct device *dev)
{
	struct fh_dma *d = dev_get_drvdata(dev);
	unsigned int i;
	int ret;

	for (i = 0; i < d->channels; i++)
		if (d->chan[i].desc || d->chan[i].stopping)
			return -EBUSY;
	ret = fh_dma_off(d);
	if (ret)
		return ret;
	disable_irq(d->irq);
	for (i = 0; i < d->channels; i++)
		tasklet_kill(&d->chan[i].tasklet);
	clk_disable_unprepare(d->clk);
	return 0;
}

static int __maybe_unused fh_dma_resume_device(struct device *dev)
{
	struct fh_dma *d = dev_get_drvdata(dev);
	int ret = clk_prepare_enable(d->clk);

	if (!ret) {
		writel(DW_CFG_DMA_EN, &d->regs->CFG);
		enable_irq(d->irq);
	}
	return ret;
}

static const struct dev_pm_ops fh_dma_pm = {
	SET_LATE_SYSTEM_SLEEP_PM_OPS(fh_dma_suspend, fh_dma_resume_device)
};

static const struct of_device_id fh_dma_match[] = {
	{ .compatible = "fullhan,fh8626v100-dma" },
	{ }
};
MODULE_DEVICE_TABLE(of, fh_dma_match);

static struct platform_driver fh_dma_driver = {
	.probe = fh_dma_probe,
	.remove = fh_dma_remove,
	.shutdown = fh_dma_shutdown,
	.driver = {
		.name = "fh8626-dma",
		.of_match_table = fh_dma_match,
		.pm = &fh_dma_pm,
		.suppress_bind_attrs = true,
	},
};
builtin_platform_driver(fh_dma_driver);

MODULE_DESCRIPTION("Fullhan FH8626 cyclic DMA controller");
MODULE_LICENSE("GPL");
