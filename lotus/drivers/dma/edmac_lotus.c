// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) LOTUS. All rights reserved.
 */

#include <linux/debugfs.h>
#include <linux/delay.h>
#include <linux/clk.h>
#include <linux/reset.h>
#include <linux/platform_device.h>
#include <linux/device.h>
#include <linux/dmaengine.h>
#include <linux/dmapool.h>
#include <linux/dma-mapping.h>
#include <linux/export.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_dma.h>
#include <linux/pm_runtime.h>
#include <linux/seq_file.h>
#include <linux/slab.h>
#include <linux/io.h>
#include <linux/regmap.h>
#include <linux/mfd/syscon.h>

#include "edmac_lotus.h"
#include "../../../drivers/dma/dmaengine.h"
#include "../../../drivers/dma/virt-dma.h"

#define DRIVER_NAME "edmac-lotus"


enum dma_burst_beats {
	DMA_BURST_1_BEATS = 1,
	DMA_BURST_2_BEATS = 2,
	DMA_BURST_4_BEATS = 4,
	DMA_BURST_8_BEATS = 8,
	DMA_BURST_16_BEATS = 16,
	DMA_BURST_32_BEATS = 32,
	DMA_BURST_64_BEATS = 64,
	DMA_BURST_128_BEATS = 128,
};

struct edmac_sg {
	dma_addr_t src_addr;
	dma_addr_t dst_addr;
	size_t len;
	struct list_head node;
};

// feature register
union dma_feature_t {
	struct {
	   unsigned int ch_num     :3; /* [2:0] */
	   unsigned int reserved3  :1; /* [3] */
	   unsigned int d_width    :2; /* [5:4] */
	   unsigned int reserved6  :2; /* [7:6] */
	   unsigned int dfifo_depth:3; /* [10:8] */
	   unsigned int reserved11 :1; /* [11] */
	   unsigned int pri_on     :1; /* [12] */
	   unsigned int reserved13 :3; /* [15:13] */
	   unsigned int pri_num    :4; /* [19:16] */
	   unsigned int ldm_on     :1; /* [20] */
	   unsigned int reserved21 :3; /* [23:21] */
	   unsigned int ldm_depth  :2; /* [25:24] */
	   unsigned int reserved26 :2; /* [27:26] */
	   unsigned int cmd_depth  :2; /* [29:28] */
	   unsigned int reserved30 :2; /* [31:30] */
	} bits;
	unsigned int u32;
};

//control register
union dma_ch_csr_t {
	struct {
	   unsigned int ch_wevent  :8; /* [7:0] */
	   unsigned int wsync      :1; /* [8] */
	   unsigned int ch_sevent  :3; /* [11:9] */
	   unsigned int sevent_en  :1; /* [12] */
	   unsigned int wevent_en  :1; /* [13] */
	   unsigned int twod_en    :1; /* [14] */
	   unsigned int exp_en     :1; /* [15] */
	   unsigned int ch_en      :1; /* [16] */
	   unsigned int wdt_en     :1; /* [17] */
	   unsigned int dst_ctrl   :2; /* [19:18] */
	   unsigned int src_ctrl   :2; /* [21:20] */
	   unsigned int dst_width  :3; /* [24:22] */
	   unsigned int src_width  :3; /* [27:25] */
	   unsigned int tc_msk     :1; /* [28] */
	   unsigned int src_tcnt   :3; /* [31:29] */
	} bits;
	unsigned int u32;
};

//config register
union dma_ch_crg_t {
	struct {
	   unsigned int int_tc_msk :1; /* [0] */
	   unsigned int int_err_msk:1; /* [1] */
	   unsigned int int_abt_msk:1; /* [2] */
	   unsigned int src_rs     :4; /* [6:3] */
	   unsigned int src_hen    :1; /* [7] */
	   unsigned int reserved   :1; /* [8] */
	   unsigned int dst_rs     :4; /* [12:9] */
	   unsigned int dst_hen    :1; /* [13] */
	   unsigned int reserved1  :2; /* [15:14] */
	   unsigned int llp_cnt    :4; /* [29:16] */
	   unsigned int ch_gntwin  :8; /* [27:20] */
	   unsigned int ch_pri     :1; /* [28] */
	   unsigned int reserved29 :1; /* [29] */
	   unsigned int wo_mode    :1; /* [30] */
	   unsigned int Unalign_Mode  :1; /* [31] */
	} bits;
	unsigned int u32;
};

struct transfer_desc {
	struct virt_dma_desc virt_desc;

	union dma_ch_csr_t csr;
	union dma_ch_crg_t cfg;
	dma_addr_t src_addr;
	dma_addr_t dst_addr;
	unsigned int llp;
	unsigned int size;
	size_t residue;
	unsigned int stride;
	unsigned int dummy;
	bool done;
	bool cyclic;
	struct list_head node;
	struct transfer_desc *parent;
};

enum edmac_dma_chan_state {
	EDMAC_CHAN_IDLE,
	EDMAC_CHAN_RUNNING,
	EDMAC_CHAN_PAUSED,
	EDMAC_CHAN_WAITING,
};

struct edmac_dma_chan {
	bool slave;
	int signal;
	int id;
	struct virt_dma_chan virt_chan;
	struct edmac_phy_chan *phychan;
	struct dma_slave_config cfg;
	struct transfer_desc *at;
	struct edmac_driver_data *host;
	enum edmac_dma_chan_state state;
};

struct edmac_phy_chan {
	unsigned int id;
	void __iomem *base;
	spinlock_t lock;
	struct edmac_dma_chan *serving;
};

struct edmac_driver_data {
	struct platform_device *dev;
	struct dma_device slave;
	struct dma_device memcpy;
	void __iomem *base;
	struct regmap *misc_regmap;
	void __iomem *crg_ctrl;
	struct edmac_phy_chan *phy_chans;
	struct dma_pool *pool;
	unsigned int misc_ctrl_base;
	int irq;
	unsigned int id;
	struct clk *clk;
	struct clk *axi_clk;
	struct reset_control *rstc;
	unsigned int channels;
	unsigned int slave_requests;
	unsigned int max_transfer_size;
};

#ifdef DEBUG_EDMAC
static void dma_slave_config(struct dma_slave_config *config)
{
	if (config->direction == DMA_MEM_TO_DEV) {
		pr_info("DMA_MEM_TO_DEV slave config");
		pr_info("config->dst_addr==0x%x", config->dst_addr);
		pr_info("config->src_maxburst==0x%x", config->src_maxburst);
		pr_info("config->dst_maxburst==0x%x", config->dst_maxburst);
		pr_info("config->src_addr_width==0x%x", config->src_addr_width);
		pr_info("config->dst_addr_width==0x%x", config->dst_addr_width);
	} else if (config->direction == DMA_DEV_TO_MEM) {
		pr_info("DMA_DEV_TO_MEM slave config");
		pr_info("config->src_addr==0x%x\n", config->src_addr);
		pr_info("config->src_maxburst==0x%x\n", config->src_maxburst);
		pr_info("config->dst_maxburst==0x%x\n", config->dst_maxburst);
		pr_info("config->src_addr_width==0x%x\n", config->src_addr_width);
		pr_info("config->dst_addr_width==0x%x\n", config->dst_addr_width);
	} else {
		pr_err("slave config direction is not support!\n");
	}

}

void dump_desc(struct transfer_desc *tsf_desc)
{
	if (tsf_desc != NULL) {
		pr_info("transfer desc csr: 0x%x\n", tsf_desc->csr.u32 & 0xffffffff);
		pr_info("transfer desc cfg: 0x%x\n", tsf_desc->cfg.u32 & 0xffffffff);
		pr_info("transfer desc src_addr: 0x%x\n", tsf_desc->src_addr & 0xffffffff);
		pr_info("transfer desc dst_addr: 0x%x\n", tsf_desc->dst_addr & 0xffffffff);
		pr_info("transfer desc llp: 0x%x\n", tsf_desc->llp & 0xffffffff);
		pr_info("transfer desc size: 0x%x\n", tsf_desc->size & 0xffffffff);
		pr_info("transfer desc stride: 0x%x\n", tsf_desc->stride & 0xffffffff);
		pr_info("transfer desc dummy: 0x%x\n", tsf_desc->dummy & 0xffffffff);
	} else {
		pr_err("stsf_desc == NULL !\n");
	}
}
#else
void dump_desc(struct transfer_desc *tsf_desc)
{

}
static void dma_slave_config(struct dma_slave_config *config)
{

}
#endif

static inline struct edmac_dma_chan *to_edamc_chan(struct dma_chan *chan)
{
	return container_of(chan, struct edmac_dma_chan, virt_chan.chan);
}

static inline struct transfer_desc *to_edmac_transfer_desc(struct dma_async_tx_descriptor *tx)
{
	return container_of(tx, struct transfer_desc, virt_desc.tx);
}

static struct dma_chan *edmac_find_chan_id(struct edmac_driver_data *edmac,
		int request_num)
{
	struct edmac_dma_chan *edmac_dma_chan = NULL;

	list_for_each_entry(edmac_dma_chan, &edmac->slave.channels, virt_chan.chan.device_node) {
		if (edmac_dma_chan->id == request_num)
			return &edmac_dma_chan->virt_chan.chan;
	}
	return NULL;
}

static struct dma_chan *edma_of_xlate(struct of_phandle_args *dma_spec,
					struct of_dma *ofdma)
{
	struct edmac_driver_data *edmac = ofdma->of_dma_data;
	struct edmac_dma_chan *edmac_dma_chan = NULL;
	struct dma_chan *dma_chan = NULL;
	struct regmap *misc = NULL;
	unsigned int signal = 0, request_num = 0;
	unsigned int reg = 0, offset = 0;

	if (edmac == NULL) {
		pr_err("edmac is NULL\n");
		return NULL;
	}
	misc = edmac->misc_regmap;

	if (dma_spec->args_count != 2) {
		pr_err("args count not true!\n");
		return NULL;
	}

	request_num = dma_spec->args[0];
	signal = dma_spec->args[1];

	if (misc != NULL) {
#ifdef CONFIG_ACCESS_M7_DEV
		offset = edmac->misc_ctrl_base;
		reg = 0xc0;
		regmap_write(misc, offset, reg);
#else
		offset = edmac->misc_ctrl_base + (request_num & (~0x3));
		regmap_read(misc, offset, &reg);
		reg &= ~(0x3f << ((request_num & 0x3) << 3));
		reg |= signal << ((request_num & 0x3) << 3);
		regmap_write(misc, offset, reg);
#endif
	}

	dma_chan = edmac_find_chan_id(edmac, request_num);
	if (!dma_chan) {
		pr_err("DMA slave channel is not found!\n");
		return NULL;
	}

	edmac_dma_chan = to_edamc_chan(dma_chan);
	edmac_dma_chan->signal = request_num;

	return dma_get_slave_channel(dma_chan);
}

static int get_of_probe(struct edmac_driver_data *edmac)
{
	struct resource *res = NULL;
	struct platform_device *platdev = edmac->dev;
	struct device_node *np = platdev->dev.of_node;
	int ret;

	ret = of_property_read_u32((&platdev->dev)->of_node,
				   "devid", &(edmac->id));
	if (ret) {
		pr_err("get edmac id fail\n");
		return -ENODEV;
	}

	edmac->clk = devm_clk_get(&(platdev->dev), "apb_pclk");
	if (IS_ERR(edmac->clk))
		return PTR_ERR(edmac->clk);

	edmac->axi_clk = devm_clk_get(&(platdev->dev), "axi_aclk");
	if (IS_ERR(edmac->axi_clk))
		return PTR_ERR(edmac->axi_clk);

	edmac->rstc = devm_reset_control_get(&(platdev->dev), "dma-reset");
	if (IS_ERR(edmac->rstc))
		return PTR_ERR(edmac->rstc);

	res = platform_get_resource(platdev, IORESOURCE_MEM, 0);
	if (!res) {
		pr_err("no reg resource\n");
		return -ENODEV;
	}

	edmac->base = devm_ioremap_resource(&(platdev->dev), res);
	if (IS_ERR(edmac->base))
		return PTR_ERR(edmac->base);

	edmac->misc_regmap = 0;
	(void)np;
	edmac->irq = platform_get_irq(platdev, 0);
	if (unlikely(edmac->irq < 0))
		return -ENODEV;

	ret = of_property_read_u32((&platdev->dev)->of_node,
				   "dma-channels", &(edmac->channels));
	if (ret) {
		pr_err("get dma-channels fail\n");
		return -ENODEV;
	}
	ret = of_property_read_u32((&platdev->dev)->of_node,
				   "dma-requests", &(edmac->slave_requests));
	if (ret) {
		pr_err("get dma-requests fail\n");
		return -ENODEV;
	}

	return of_dma_controller_register(platdev->dev.of_node, edma_of_xlate, edmac);
}

static void edmac_free_chan_resources(struct dma_chan *chan)
{
	vchan_free_chan_resources(to_virt_chan(chan));
}

static enum dma_status edmac_tx_status(struct dma_chan *chan,
		dma_cookie_t cookie, struct dma_tx_state *txstate)
{
	enum dma_status ret = DMA_COMPLETE;
	struct edmac_dma_chan *edmac_dma_chan = to_edamc_chan(chan);
	struct edmac_phy_chan *phychan = edmac_dma_chan->phychan;
	struct edmac_driver_data *edmac = edmac_dma_chan->host;
	struct virt_dma_desc *vd = NULL;
	struct transfer_desc *tsf_desc = NULL;
	u32 bytes = 0, residue_size;
	unsigned long flags;

	ret = dma_cookie_status(chan, cookie, txstate);
	if (ret == DMA_COMPLETE)
		return ret;
	spin_lock_irqsave(&edmac_dma_chan->virt_chan.lock, flags);
	vd = vchan_find_desc(&edmac_dma_chan->virt_chan, cookie);
	if (vd) {
		/* no been transfer */
		tsf_desc = to_edmac_transfer_desc(&vd->tx);
		residue_size = tsf_desc->size;
	} else {
		/* trasfering */
		tsf_desc = edmac_dma_chan->at;
		if (tsf_desc != NULL)
			dump_desc(tsf_desc);
		if (!phychan || !tsf_desc) {
			spin_unlock_irqrestore(&edmac_dma_chan->virt_chan.lock, flags);
			goto out;
		}
		residue_size = readl(edmac->base + DMA_Cx_BASE(phychan->id) + DMA_CH_SIZE);

		/* non-lli mode */
	}

	if (tsf_desc->cfg.bits.Unalign_Mode)
		bytes = residue_size;
	else
		bytes = residue_size << tsf_desc->csr.bits.src_width;

	spin_unlock_irqrestore(&edmac_dma_chan->virt_chan.lock, flags);

	dma_set_residue(txstate, bytes);

	if (edmac_dma_chan->state == EDMAC_CHAN_PAUSED && ret == DMA_IN_PROGRESS) {
		ret = DMA_PAUSED;
		return ret;
	}

out:
	return ret;
}

static struct edmac_phy_chan *edmac_get_phy_channel(
	struct edmac_driver_data *edmac,
	struct edmac_dma_chan *edmac_dma_chan)
{
	struct edmac_phy_chan *ch = NULL;
	unsigned long flags;
	int i;

	for (i = 0; i < edmac->channels; i++) {
		ch = &edmac->phy_chans[i];

		spin_lock_irqsave(&ch->lock, flags);
		/* get idle phy channel */
		if (!ch->serving) {
			ch->serving = edmac_dma_chan;
			spin_unlock_irqrestore(&ch->lock, flags);
			break;
		}
		spin_unlock_irqrestore(&ch->lock, flags);
	}

	if (i == edmac->channels) {
		pr_err("not found idle channel ,maybe all channel is busy!!\n");
		return NULL;
	}

	return ch;
}

static void edmac_write_reg(struct edmac_driver_data *edmac,
			      struct edmac_phy_chan *phychan,
			      struct transfer_desc *tsf_desc)
{
	/* channel clear tc & err */
	writel(BIT(phychan->id), (edmac->base + DMA_TC_CLEAR));
	writel((BIT(phychan->id) | BIT(phychan->id + 16)), edmac->base + DMA_ERR_CLEAR);

	/* close llp logic not support */
	if (tsf_desc->llp == 0x0)
		writel(tsf_desc->llp, (edmac->base + DMA_Cx_BASE(phychan->id) + DMA_CH_LLP));

	/* write control register */
	writel(tsf_desc->csr.u32, (edmac->base + DMA_Cx_BASE(phychan->id) + DMA_CH_CTL));

	/* write configure register */
	writel(tsf_desc->cfg.u32, (edmac->base + DMA_Cx_BASE(phychan->id) + DMA_CH_CFG));

	/* write src address register */
	writel(tsf_desc->src_addr, (edmac->base + DMA_Cx_BASE(phychan->id) + DMA_CH_SRC_ADDR));

	/* write dst address register */
	writel(tsf_desc->dst_addr, (edmac->base + DMA_Cx_BASE(phychan->id) + DMA_CH_DST_ADDR));

	/* write transfer size register */
	writel(tsf_desc->size, (edmac->base + DMA_Cx_BASE(phychan->id) + DMA_CH_SIZE));
}

static void edmac_start_next_txd(struct edmac_dma_chan *edmac_dma_chan)
{
	struct edmac_driver_data *edmac = edmac_dma_chan->host;
	struct edmac_phy_chan *phychan = edmac_dma_chan->phychan;
	struct virt_dma_desc *vd = vchan_next_desc(&edmac_dma_chan->virt_chan);
	struct transfer_desc *tsf_desc = to_edmac_transfer_desc(&vd->tx);
	unsigned int val = 0;

	list_del(&tsf_desc->virt_desc.node);

	edmac_dma_chan->at = tsf_desc;

	dump_desc(tsf_desc);

	edmac_write_reg(edmac, phychan, tsf_desc);

	/* enable phy channel */
	val = readl(edmac->base + DMA_Cx_BASE(phychan->id) + DMA_CH_CTL);
	writel(val | DMA_CSR_CH_EN, edmac->base + DMA_Cx_BASE(phychan->id) + DMA_CH_CTL);
}

static void edmac_start(struct edmac_dma_chan *edmac_dma_chan)
{
	struct edmac_driver_data *edmac = edmac_dma_chan->host;
	struct edmac_phy_chan *ch;

	ch = edmac_get_phy_channel(edmac, edmac_dma_chan);
	if (!ch) {
		pr_err("no phy channel available !\n");
		edmac_dma_chan->state = EDMAC_CHAN_WAITING;
		return;
	}

	edmac_dma_chan->phychan = ch;
	edmac_dma_chan->state = EDMAC_CHAN_RUNNING;

	edmac_start_next_txd(edmac_dma_chan);
}

static void edmac_issue_pending(struct dma_chan *chan)
{
	struct edmac_dma_chan *edmac_dma_chan = to_edamc_chan(chan);
	unsigned long flags;

	spin_lock_irqsave(&edmac_dma_chan->virt_chan.lock, flags);
	if (vchan_issue_pending(&edmac_dma_chan->virt_chan)) {
		if (!edmac_dma_chan->phychan && edmac_dma_chan->state != EDMAC_CHAN_WAITING)
			edmac_start(edmac_dma_chan);
	}
	spin_unlock_irqrestore(&edmac_dma_chan->virt_chan.lock, flags);
}

static void edmac_free_txd_list(struct edmac_dma_chan *edmac_dma_chan)
{
	LIST_HEAD(head);

	vchan_get_all_descriptors(&edmac_dma_chan->virt_chan, &head);
	vchan_dma_desc_free_list(&edmac_dma_chan->virt_chan, &head);
}

static int edmac_config(struct dma_chan *chan,
			  struct dma_slave_config *config)
{
	struct edmac_dma_chan *edmac_dma_chan = to_edamc_chan(chan);

	if (!edmac_dma_chan->slave) {
		pr_err("edmac chan slave is not setting !\n");
		return -EINVAL;
	}

	edmac_dma_chan->cfg = *config;
	dma_slave_config(&(edmac_dma_chan->cfg));

	return 0;
}

static void edmac_pause_phy_chan(struct edmac_dma_chan *edmac_dma_chan)
{
	struct edmac_driver_data *edmac = edmac_dma_chan->host;
	struct edmac_phy_chan *phychan = edmac_dma_chan->phychan;
	unsigned int val;

	/* Use Channel Enable Register on the FTDMAC030 */
	val = ~(BIT(phychan->id));
	writel(val, edmac->base + DMA_EN_CHAN);
}

static int edmac_pause(struct dma_chan *chan)
{
	struct edmac_dma_chan *edmac_dma_chan = to_edamc_chan(chan);
	struct edmac_driver_data *edmac = edmac_dma_chan->host;
	struct edmac_phy_chan *phychan = edmac_dma_chan->phychan;
	unsigned long flags;
	u32 val;

	spin_lock_irqsave(&edmac_dma_chan->virt_chan.lock, flags);

	if (!edmac_dma_chan->phychan) {
		spin_unlock_irqrestore(&edmac_dma_chan->virt_chan.lock, flags);
		return 0;
	}

	/*
	 * When actively pause dma transmission, we mask the abort interrupt.
	 * then never receive the abort interrupt on this dma channel.
	 * reduce entering interrupt times and improve performance.
	 */
	val = readl(edmac->base + DMA_Cx_BASE(phychan->id) + DMA_CH_CFG);
	val |= (DMA_CH_CFG_INT_ABT_MASK);
	writel(val, edmac->base + DMA_Cx_BASE(phychan->id) + DMA_CH_CFG);

	edmac_pause_phy_chan(edmac_dma_chan);
	edmac_dma_chan->state = EDMAC_CHAN_PAUSED;
	spin_unlock_irqrestore(&edmac_dma_chan->virt_chan.lock, flags);

	return 0;
}

static void edmac_resume_phy_chan(struct edmac_dma_chan *edmac_dma_chan)
{
	struct edmac_driver_data *edmac = edmac_dma_chan->host;
	struct edmac_phy_chan *phychan = edmac_dma_chan->phychan;
	unsigned int val;

	val = readl(edmac->base + DMA_Cx_BASE(phychan->id) + DMA_CH_CTL);
	val |= DMA_CSR_CH_EN;
	writel(val, edmac->base + DMA_Cx_BASE(phychan->id) + DMA_CH_CTL);
}

static int edmac_resume(struct dma_chan *chan)
{
	struct edmac_dma_chan *edmac_dma_chan = to_edamc_chan(chan);
	struct edmac_driver_data *edmac = edmac_dma_chan->host;
	struct edmac_phy_chan *phychan = edmac_dma_chan->phychan;
	struct transfer_desc *tsf_desc = edmac_dma_chan->at;
	unsigned long flags;
	u32 val;

	spin_lock_irqsave(&edmac_dma_chan->virt_chan.lock, flags);

	if (!edmac_dma_chan->phychan) {
		spin_unlock_irqrestore(&edmac_dma_chan->virt_chan.lock, flags);
		return 0;
	}

	/*
	 * Masked the abort interrupt when pause dma channel before, so recovery
	 * the abort interrupt msk bit according to origin config.
	 */
	val = readl(edmac->base + DMA_Cx_BASE(phychan->id) + DMA_CH_CFG);
	val &= ~(DMA_CH_CFG_INT_ABT_MASK);
	val |= tsf_desc->cfg.u32 & DMA_CH_CFG_INT_ABT_MASK;
	writel(val, edmac->base + DMA_Cx_BASE(phychan->id) + DMA_CH_CFG);

	edmac_resume_phy_chan(edmac_dma_chan);
	edmac_dma_chan->state = EDMAC_CHAN_RUNNING;
	spin_unlock_irqrestore(&edmac_dma_chan->virt_chan.lock, flags);

	return 0;
}

void edmac_phy_free(struct edmac_dma_chan *chan);
static void edmac_desc_free(struct virt_dma_desc *vd);
static int edmac_terminate_all(struct dma_chan *chan)
{
	struct edmac_dma_chan *edmac_dma_chan = to_edamc_chan(chan);
	unsigned long flags;

	spin_lock_irqsave(&edmac_dma_chan->virt_chan.lock, flags);
	if (!edmac_dma_chan->phychan && !edmac_dma_chan->at) {
		spin_unlock_irqrestore(&edmac_dma_chan->virt_chan.lock, flags);
		return 0;
	}

	edmac_dma_chan->state = EDMAC_CHAN_IDLE;

	if (edmac_dma_chan->phychan)
		edmac_phy_free(edmac_dma_chan);

	if (edmac_dma_chan->at) {
		edmac_desc_free(&edmac_dma_chan->at->virt_desc);
		edmac_dma_chan->at = NULL;
	}
	edmac_free_txd_list(edmac_dma_chan);

	spin_unlock_irqrestore(&edmac_dma_chan->virt_chan.lock, flags);

	return 0;
}

static struct transfer_desc *edmac_get_tsf_desc(struct edmac_driver_data *plchan)
{
	struct transfer_desc *tsf_desc = kzalloc(sizeof(struct transfer_desc), GFP_NOWAIT);

	if (tsf_desc) {
		tsf_desc->csr.u32 = 0;
		tsf_desc->cfg.u32 = 0;
		tsf_desc->src_addr = 0;
		tsf_desc->dst_addr = 0;
		tsf_desc->llp = 0;
		tsf_desc->size = 0;
		tsf_desc->stride = 0;
		tsf_desc->dummy = 0;
	}

	return tsf_desc;
}

static void edmac_free_tsf_desc(struct edmac_driver_data *edmac,
				  struct transfer_desc *tsf_desc)
{
	kfree(tsf_desc);
}

static u32 get_width(enum dma_slave_buswidth width)
{
	switch (width) {
	case DMA_SLAVE_BUSWIDTH_1_BYTE:
		return DMA_WIDTH_8BIT;
	case DMA_SLAVE_BUSWIDTH_2_BYTES:
		return DMA_WIDTH_16BIT;
	case DMA_SLAVE_BUSWIDTH_4_BYTES:
		return DMA_WIDTH_32BIT;
	case DMA_SLAVE_BUSWIDTH_8_BYTES:
		return DMA_WIDTH_64BIT;
	case DMA_SLAVE_BUSWIDTH_16_BYTES:
		return DMA_WIDTH_128BIT;
	default:
		pr_err("check here, width warning!\n");
		return ~0;
	}
}

static u32 get_burst_beats(enum dma_burst_beats beats)
{
	switch (beats) {
	case DMA_BURST_1_BEATS:
		return AHBDMA_Burst1;
	case DMA_BURST_2_BEATS:
		return AHBDMA_Burst2;
	case DMA_BURST_4_BEATS:
		return AHBDMA_Burst4;
	case DMA_BURST_8_BEATS:
		return AHBDMA_Burst8;
	case DMA_BURST_16_BEATS:
		return AHBDMA_Burst16;
	case DMA_BURST_32_BEATS:
		return AHBDMA_Burst32;
	case DMA_BURST_64_BEATS:
		return AHBDMA_Burst64;
	case DMA_BURST_128_BEATS:
		return AHBDMA_Burst128;
	default:
		pr_err("check here, burst beats warning!\n");
		return ~0;
	}
}

struct transfer_desc *edmac_init_tsf_desc(struct dma_chan *chan,
	enum dma_transfer_direction direction,
	dma_addr_t *slave_addr)
{
	struct edmac_dma_chan *edmac_dma_chan = to_edamc_chan(chan);
	struct edmac_driver_data *edmac = edmac_dma_chan->host;
	struct transfer_desc *tsf_desc;
	unsigned int addr_width = 0;
	unsigned char width = 0, maxburst = 0, burst = 0;

	tsf_desc = edmac_get_tsf_desc(edmac);
	if (!tsf_desc) {
		pr_err("get tsf desc fail!\n");
		return NULL;
	}

	if (direction == DMA_MEM_TO_DEV) {
		*slave_addr = edmac_dma_chan->cfg.dst_addr;
		addr_width = edmac_dma_chan->cfg.dst_addr_width;
		maxburst = edmac_dma_chan->cfg.dst_maxburst;
	} else if (direction == DMA_DEV_TO_MEM) {
		*slave_addr = edmac_dma_chan->cfg.src_addr;
		addr_width = edmac_dma_chan->cfg.src_addr_width;
		maxburst = edmac_dma_chan->cfg.src_maxburst;
	} else {
		edmac_free_tsf_desc(edmac, tsf_desc);
		pr_err("direction unsupported!\n");
		return NULL;
	}

	width = get_width(addr_width);
	tsf_desc->csr.bits.src_width = width;
	tsf_desc->csr.bits.dst_width = width;

	burst = get_burst_beats(maxburst);
	tsf_desc->csr.bits.src_tcnt = burst;

	if (edmac_dma_chan->signal >= 0) {
		/* Assign the flow control signal to this channel, Mode is AHBDMA_NormalMode */
		if (direction == DMA_MEM_TO_DEV) {
			tsf_desc->cfg.bits.dst_hen = 1;
			tsf_desc->cfg.bits.dst_rs = edmac_dma_chan->signal;
			/* destination increment, decrement or fix */
			tsf_desc->csr.bits.dst_ctrl = AHBDMA_DstFix;
			tsf_desc->csr.bits.src_ctrl = AHBDMA_SrcInc;
		} else if (direction == DMA_DEV_TO_MEM) {
			tsf_desc->cfg.bits.src_hen = 1;
			tsf_desc->cfg.bits.src_rs = edmac_dma_chan->signal;
			/* source increment, decrement or fix */
			tsf_desc->csr.bits.src_ctrl = AHBDMA_SrcFix;
			tsf_desc->csr.bits.dst_ctrl = AHBDMA_DstInc;
		}
	}

	return tsf_desc;
}

static void edmac_fill_desc(struct transfer_desc *tsf_desc, dma_addr_t src,
				  dma_addr_t dst, unsigned int length)
{
	tsf_desc->src_addr = src;
	tsf_desc->dst_addr = dst;
	if (tsf_desc->cfg.bits.Unalign_Mode)
		tsf_desc->size = length;
	else
		tsf_desc->size = (length >> tsf_desc->csr.bits.src_width);
}

static struct dma_async_tx_descriptor *edmac_perp_slave_sg(
	struct dma_chan *chan, struct scatterlist *sgl,
	unsigned int sg_len, enum dma_transfer_direction direction,
	unsigned long flags, void *context)
{
	struct edmac_dma_chan *edmac_dma_chan = to_edamc_chan(chan);
	struct transfer_desc *tsf_desc = NULL;
	struct scatterlist *sg = NULL;
	int tmp = 0;
	dma_addr_t  src = 0, dst = 0, addr = 0, slave_addr = 0;
	unsigned int length = 0, num = 0;

	if (sgl == NULL) {
		pr_err("sgl is null!\n");
		return NULL;
	}

	tsf_desc = edmac_init_tsf_desc(chan, direction, &slave_addr);
	if (!tsf_desc) {
		pr_err("desc init fail\n");
		return NULL;
	}
	tsf_desc->csr.bits.tc_msk = 0;
	tsf_desc->csr.bits.exp_en = 1;
	tsf_desc->csr.bits.wsync = 1;

	/* program channel CFG */
	tsf_desc->cfg.bits.int_tc_msk = 0;
	tsf_desc->cfg.bits.int_err_msk = 0;
	tsf_desc->cfg.bits.int_abt_msk = 0;

	tsf_desc->cfg.bits.ch_gntwin = 0;
	tsf_desc->cfg.bits.reserved = 0;
	tsf_desc->cfg.bits.reserved1 = 0;
	tsf_desc->cfg.bits.llp_cnt = 0;
	tsf_desc->cfg.bits.ch_pri = 1;
	tsf_desc->cfg.bits.Unalign_Mode = 0;
	/* program channel llp */
	tsf_desc->llp = 0;

	for_each_sg(sgl, sg, sg_len, tmp) {
		if (num == 0) {
			addr = sg_dma_address(sg);
			length = sg_dma_len(sg);
			if (direction == DMA_MEM_TO_DEV) {
				src = addr;
				dst = slave_addr;
			} else if (direction == DMA_DEV_TO_MEM) {
				src = slave_addr;
				dst = addr;
			}
			edmac_fill_desc(tsf_desc, src, dst, length);
		} else {
			addr = sg_dma_address(sg);
			length = sg_dma_len(sg);
		}
		num++;
	}

	dump_desc(tsf_desc);

	return vchan_tx_prep(&edmac_dma_chan->virt_chan, &tsf_desc->virt_desc, flags);
}

static struct dma_async_tx_descriptor *edmac_prep_dma_memcpy(
	struct dma_chan *chan, dma_addr_t dest, dma_addr_t src,
	size_t len, unsigned long flags)
{

	struct edmac_dma_chan *edmac_dma_chan = to_edamc_chan(chan);
	struct edmac_driver_data *edmac = edmac_dma_chan->host;
	struct transfer_desc *tsf_desc = NULL;

	if (chan == NULL) {
		pr_err("dma chan == NULL!\n");
		return NULL;
	}
	if (edmac_dma_chan->host == NULL) {
		pr_err("edmac_dma_chan->host == NULL!\n");
		return NULL;
	}
	if (!len) {
		pr_err("prep dma memcpy transfer length <= 0\n");
		return NULL;
	}
	if (len >= MAX_TRANSFER_BYTES) {
		pr_err("transfer length > (%d)\n", MAX_TRANSFER_BYTES);
		return NULL;
	}

	tsf_desc = edmac_get_tsf_desc(edmac);
	if (!tsf_desc) {
		pr_err("get tsf desc fail!\n");
		return NULL;
	}

	/* program channel CSR */
	tsf_desc->csr.bits.tc_msk = 0;
	tsf_desc->csr.bits.src_tcnt = 0;
	tsf_desc->csr.bits.src_width = AHBDMA_SrcWidth_DDWord;
	tsf_desc->csr.bits.dst_width = AHBDMA_DstWidth_DDWord;
	tsf_desc->csr.bits.src_ctrl = AHBDMA_SrcInc;
	tsf_desc->csr.bits.dst_ctrl = AHBDMA_DstInc;
	tsf_desc->csr.bits.exp_en = 0;
	tsf_desc->csr.bits.wsync = 0;

	/* program channel CFG */
	tsf_desc->cfg.bits.int_tc_msk = 0;
	tsf_desc->cfg.bits.int_err_msk = 0;
	tsf_desc->cfg.bits.int_abt_msk = 0;

	/* Mode is AHBDMA_NormalMode */
	tsf_desc->cfg.bits.src_hen = 0;
	tsf_desc->cfg.bits.src_rs = 0;
	tsf_desc->cfg.bits.dst_hen = 0;
	tsf_desc->cfg.bits.dst_rs = 0;

	tsf_desc->cfg.bits.ch_gntwin = 0;
	tsf_desc->cfg.bits.reserved = 0;
	tsf_desc->cfg.bits.reserved1 = 0;
	tsf_desc->cfg.bits.llp_cnt = 0;
	tsf_desc->cfg.bits.ch_pri = 0;
	tsf_desc->cfg.bits.Unalign_Mode = 1;

	/* program channel llp */
	tsf_desc->llp = 0;
	edmac_fill_desc(tsf_desc, src, dest, len);

	dump_desc(tsf_desc);

	return vchan_tx_prep(&edmac_dma_chan->virt_chan, &tsf_desc->virt_desc, flags);
}

static void  edmac_phy_reassign(struct edmac_phy_chan *phy_chan,
				  struct edmac_dma_chan *chan)
{
	phy_chan->serving = chan;
	chan->phychan = phy_chan;
	chan->state = EDMAC_CHAN_RUNNING;

	edmac_start_next_txd(chan);
}

static void edmac_terminate_phy_chan(struct edmac_driver_data *edmac,
				       struct edmac_dma_chan *edmac_dma_chan)
{
	unsigned int val;
	struct edmac_phy_chan *phychan = edmac_dma_chan->phychan;

	/* Disable all interrupts */
	val = readl(edmac->base + DMA_Cx_BASE(phychan->id) + DMA_CH_CFG);
	val |= (DMA_CH_CFG_INT_ABT_MASK | DMA_CH_CFG_INT_ERR_MASK | DMA_CH_CFG_INT_TC_MASK);
	writel(val, edmac->base + DMA_Cx_BASE(phychan->id) + DMA_CH_CFG);

	edmac_pause_phy_chan(edmac_dma_chan);

	/* disable channel */
	val = readl(edmac->base + DMA_Cx_BASE(phychan->id) + DMA_CH_CTL);
	val &= ~DMA_CSR_CH_EN;
	writel(val, edmac->base + DMA_Cx_BASE(phychan->id) + DMA_CH_CTL);

	/* clear channel tc & err */
	writel(BIT(phychan->id), (edmac->base + DMA_TC_CLEAR));
	val = BIT(phychan->id) | BIT(phychan->id + 16);
	writel(val, (edmac->base + DMA_ERR_CLEAR));
}

void edmac_phy_free(struct edmac_dma_chan *chan)
{
	struct edmac_driver_data *edmac = chan->host;
	struct edmac_dma_chan *p = NULL;
	struct edmac_dma_chan *next = NULL;

	list_for_each_entry(p, &edmac->memcpy.channels, virt_chan.chan.device_node) {
		if (p->state == EDMAC_CHAN_WAITING) {
			next = p;
			break;
		}
	}

	if (!next) {
		list_for_each_entry(p, &edmac->slave.channels, virt_chan.chan.device_node) {
			if (p->state == EDMAC_CHAN_WAITING) {
				next = p;
				break;
			}
		}
	}
	edmac_terminate_phy_chan(edmac, chan);

	if (next) {
		spin_lock(&next->virt_chan.lock);
		edmac_phy_reassign(chan->phychan, next);
		spin_unlock(&next->virt_chan.lock);
	} else {
		chan->phychan->serving = NULL;
	}

	chan->phychan = NULL;
	chan->state = EDMAC_CHAN_IDLE;
}

static irqreturn_t edmac_irq(int irq, void *dev)
{
	struct edmac_driver_data *edmac = (struct edmac_driver_data *)dev;
	struct edmac_dma_chan *chan = NULL;
	struct edmac_phy_chan *phy_chan = NULL;
	struct transfer_desc *tsf_desc = NULL;
	u32 mask = 0;
	unsigned int i = 0;
	unsigned int channel_status = 0;
	unsigned int tc = 0, err = 0;

	channel_status = readl(edmac->base + DMA_INT_STATUS);

	if (!channel_status)
		return IRQ_NONE;

	/* check & clear - ERR & TC interrupts */
	err = readl(edmac->base + DMA_ERR_STATUS);
	if (err) {
		dev_err(&edmac->dev->dev, "%s error interrupt, register value 0x%08x\n",
			__func__, err);
		writel(err, edmac->base + DMA_ERR_CLEAR);
	}
	tc = readl(edmac->base + DMA_TC_STATUS);
	if (tc)
		writel(tc, edmac->base + DMA_TC_CLEAR);

	if (!err && !tc)
		return IRQ_NONE;

	for (i = 0; i < edmac->channels; i++) {
		if ((BIT(i) & err) || (BIT(i) & tc)) {
			phy_chan = &edmac->phy_chans[i];
			chan = phy_chan->serving;
			if (!chan) {
				dev_err(&edmac->dev->dev,
					"%s Error TC interrupt on unused channel: %d\n",
					__func__, i);

				continue;
			}
			spin_lock(&chan->virt_chan.lock);
			tsf_desc = chan->at;
			if (tsf_desc && tsf_desc->cyclic) {
				vchan_cyclic_callback(&tsf_desc->virt_desc);
				spin_unlock(&chan->virt_chan.lock);
				continue;
			} else if (tsf_desc) {
				chan->at = NULL;
				tsf_desc->done = true;
				vchan_cookie_complete(&tsf_desc->virt_desc);

				if (vchan_next_desc(&chan->virt_chan))
					edmac_start_next_txd(chan);
				else
					edmac_phy_free(chan);
			}
			spin_unlock(&chan->virt_chan.lock);
			mask |= BIT(i);
		}
	}

	return mask ? IRQ_HANDLED : IRQ_NONE;
}

static void edmac_dma_slave_init(struct edmac_dma_chan *chan)
{
	chan->slave = true;
}

static void edmac_desc_free(struct virt_dma_desc *vd)
{
	struct transfer_desc *tsf_desc = to_edmac_transfer_desc(&vd->tx);
	struct edmac_dma_chan *edmac_dma_chan = to_edamc_chan(vd->tx.chan);

	dma_descriptor_unmap(&vd->tx);
	edmac_free_tsf_desc(edmac_dma_chan->host, tsf_desc);
}

static int edmac_init_virt_channels(struct edmac_driver_data *edmac,
				      struct dma_device *dmadev, unsigned int channels, bool slave)
{
	struct edmac_dma_chan *chan = NULL;
	int i;

	INIT_LIST_HEAD(&dmadev->channels);

	for (i = 0; i < channels; i++) {
		chan = kzalloc(sizeof(struct edmac_dma_chan), GFP_KERNEL);
		chan->host = edmac;
		chan->state = EDMAC_CHAN_IDLE;
		chan->signal = -1;

		if (slave) {
			chan->id = i;
			edmac_dma_slave_init(chan);
		}
		chan->virt_chan.desc_free = edmac_desc_free;
		vchan_init(&chan->virt_chan, dmadev);
	}

	return 0;
}

void edmac_free_virt_channels(struct dma_device *dmadev)
{
	struct edmac_dma_chan *chan = NULL;
	struct edmac_dma_chan *next = NULL;

	list_for_each_entry_safe(chan,
				 next, &dmadev->channels, virt_chan.chan.device_node) {
		list_del(&chan->virt_chan.chan.device_node);
		kfree(chan);
	}
}

#ifdef CONFIG_PM_SLEEP
static int edmac_dma_suspend(struct device *dev)
{
	struct edmac_driver_data *edmac = dev_get_drvdata(dev);

	pr_debug("%s: enter!\n", __func__);

	clk_disable_unprepare(edmac->clk);
	clk_disable_unprepare(edmac->axi_clk);

	pr_debug("%s: ok!\n", __func__);

	return 0;
}

static int edmac_dma_resume(struct device *dev)
{
	struct edmac_driver_data *edmac = dev_get_drvdata(dev);
	int ret;

	pr_debug("%s: enter!\n", __func__);

	ret = clk_prepare_enable(edmac->clk);
	if (ret) {
		dev_err(dev, "failed to prepare_enable clock\n");
		return ret;
	}

	ret = clk_prepare_enable(edmac->axi_clk);
	if (ret) {
		dev_err(dev, "failed to prepare_enable axi_clock\n");
		return ret;
	}

	pr_debug("%s: ok!\n", __func__);

	return ret;
}

static SIMPLE_DEV_PM_OPS(edmac_dma_pm_ops, edmac_dma_suspend,
			 edmac_dma_resume);

#endif
static int __init edmac_probe(struct platform_device *pdev)
{
	u32 val = 0;
	int ret = 0, i = 0;
	union dma_feature_t fea;
	struct edmac_driver_data *edmac = NULL;

	ret = dma_set_mask_and_coherent(&(pdev->dev), DMA_BIT_MASK(64));
	if (ret)
		return ret;

	edmac = kzalloc(sizeof(*edmac), GFP_KERNEL);
	edmac->dev = pdev;

	ret = get_of_probe(edmac);
	if (ret) {
		pr_err("get dts info fail!\n");
		goto free_edmac;
	}

	clk_prepare_enable(edmac->clk);
	clk_prepare_enable(edmac->axi_clk);
	reset_control_deassert(edmac->rstc);

	val = readl(edmac->base + DMA_VER);
	pr_info("FTDMAC030 Revision number: 0x%x\n", val);

	fea.u32 = readl(edmac->base + DMA_FEA);
	pr_info("FTDMAC030 feature: 0x%x\n", fea.u32);
	edmac->channels = fea.bits.ch_num + 1;

	dma_cap_set(DMA_MEMCPY, edmac->memcpy.cap_mask);
	edmac->memcpy.dev = &pdev->dev;
	edmac->memcpy.device_free_chan_resources = edmac_free_chan_resources;
	edmac->memcpy.device_prep_dma_memcpy = edmac_prep_dma_memcpy;
	edmac->memcpy.device_tx_status = edmac_tx_status;
	edmac->memcpy.device_issue_pending = edmac_issue_pending;
	edmac->memcpy.device_config = edmac_config;
	edmac->memcpy.device_pause = edmac_pause;
	edmac->memcpy.device_resume = edmac_resume;
	edmac->memcpy.device_terminate_all = edmac_terminate_all;
	edmac->memcpy.directions = BIT(DMA_MEM_TO_MEM);
	edmac->memcpy.residue_granularity = DMA_RESIDUE_GRANULARITY_SEGMENT;

	dma_cap_set(DMA_SLAVE, edmac->slave.cap_mask);
	edmac->slave.dev = &pdev->dev;
	edmac->slave.device_free_chan_resources = edmac_free_chan_resources;
	edmac->slave.device_tx_status = edmac_tx_status;
	edmac->slave.device_issue_pending = edmac_issue_pending;
	edmac->slave.device_prep_slave_sg = edmac_perp_slave_sg;
	edmac->slave.device_config = edmac_config;
	edmac->slave.device_resume = edmac_resume;
	edmac->slave.device_pause = edmac_pause;
	edmac->slave.device_terminate_all = edmac_terminate_all;
	edmac->slave.directions = BIT(DMA_DEV_TO_MEM) | BIT(DMA_MEM_TO_DEV);
	edmac->slave.residue_granularity = DMA_RESIDUE_GRANULARITY_SEGMENT;
	edmac->max_transfer_size = MAX_TRANSFER_BYTES;

	/* Clear any pending interrupts */
	writel(0x0000FFFF, (edmac->base + DMA_ERR_CLEAR));
	writel(0x000000FF, (edmac->base + DMA_TC_CLEAR));

	ret = request_irq(edmac->irq, edmac_irq, 0, DRIVER_NAME, edmac);
	if (ret) {
		pr_err("fail to request irq\n");
		goto free_edmac;
	}

	pr_info("channel number==%d\n ", edmac->channels);
	edmac->phy_chans = kzalloc((edmac->channels * sizeof(struct edmac_phy_chan)), GFP_KERNEL);
	if (!edmac->phy_chans) {
		pr_err("malloc for phy chans fail!\n");
		ret = -ENOMEM;
		goto free_irq_res;
	}

	/* initialize  the phy chan */
	for (i = 0; i < edmac->channels; i++) {
		struct edmac_phy_chan *phy_ch = NULL;

		phy_ch = &edmac->phy_chans[i];
		phy_ch->id = i;
		phy_ch->base = edmac->base + DMA_Cx_BASE(i);
		spin_lock_init(&phy_ch->lock);
		phy_ch->serving = NULL;
	}

	/* initialize the memory virt chan */
	ret = edmac_init_virt_channels(edmac, &edmac->memcpy, edmac->channels, false);
	if (ret) {
		pr_err("edmac init virt channels fail!\n");
		goto  free_phychans;
	}

	/* initialize the slave virt chan */
	ret = edmac_init_virt_channels(edmac, &edmac->slave,  edmac->slave_requests, true);
	if (ret) {
		pr_err("edmac init virt channels fail!\n");
		goto  free_memory_virt_channels;

	}

	ret = dma_async_device_register(&edmac->memcpy);
	if (ret) {
		pr_err("edma memcpy async device register fail\n");
		goto free_slave_virt_channels;
	}

	ret = dma_async_device_register(&edmac->slave);
	if (ret) {
		pr_err("edmac slave async device register fail!\n");
		goto free_memcpy_device;
	}

	platform_set_drvdata(pdev, edmac);

	return 0;

free_memcpy_device:
	dma_async_device_unregister(&edmac->memcpy);
free_slave_virt_channels:
	edmac_free_virt_channels(&edmac->slave);
free_memory_virt_channels:
	edmac_free_virt_channels(&edmac->memcpy);
free_phychans:
	kfree(edmac->phy_chans);
free_irq_res:
	free_irq(edmac->irq, edmac);
free_edmac:
	kfree(edmac);

	return ret;
}

static int edmac_remove(struct platform_device *pdev)
{
	int err = 0;
	return err;
}

static const struct of_device_id edmac_match[] = {
	{ .compatible = "lotus,edmac" },
	{},
};

static struct platform_driver edmac_driver = {
	.remove = edmac_remove,
	.driver = {
		.name   = "edmac",
#ifdef CONFIG_PM_SLEEP
		.pm = &edmac_dma_pm_ops,
#endif
		.of_match_table = edmac_match,
	},
};

static int __init edmac_init(void)
{
	return platform_driver_probe(&edmac_driver, edmac_probe);
}
subsys_initcall(edmac_init);

static void __exit edmac_exit(void)
{
	platform_driver_unregister(&edmac_driver);
}
module_exit(edmac_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Lotus");
