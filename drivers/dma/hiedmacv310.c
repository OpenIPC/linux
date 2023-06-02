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

#include "hiedmacv310.h"
#include "dmaengine.h"
#include "virt-dma.h"

#define DRIVER_NAME "hiedmacv310"

int hiedmacv310_trace_level = HIEDMACV310_TRACE_LEVEL;

typedef struct hiedmac_lli {
	u64 next_lli;
	u32 reserved[5];
	u32 count;
	u64 src_addr;
	u64 dest_addr;
	u32 config;
	u32 pad[3];
} hiedmac_lli;

struct hiedmac_sg {
	dma_addr_t src_addr;
	dma_addr_t dst_addr;
	size_t len;
	struct list_head node;
};

struct transfer_desc {
	struct virt_dma_desc virt_desc;

	dma_addr_t llis_busaddr;
	u32 *llis_vaddr;
	u32 ccfg;
	size_t size;
	bool done;
	bool cyclic;
};

enum edmac_dma_chan_state {
	HIEDMAC_CHAN_IDLE,
	HIEDMAC_CHAN_RUNNING,
	HIEDMAC_CHAN_PAUSED,
	HIEDMAC_CHAN_WAITING,
};

struct hiedmacv310_dma_chan {
	bool slave;
	int signal;
	int id;
	struct virt_dma_chan virt_chan;
	struct hiedmacv310_phy_chan *phychan;
	struct dma_slave_config cfg;
	struct transfer_desc *at;
	struct hiedmacv310_driver_data *host;
	enum edmac_dma_chan_state state;
};

struct hiedmacv310_phy_chan {
	unsigned int id;
	void __iomem *base;
	spinlock_t lock;
	struct hiedmacv310_dma_chan *serving;
};

struct hiedmacv310_driver_data {
	struct platform_device *dev;
	struct dma_device slave;
	struct dma_device memcpy;
	void __iomem *base;
	struct regmap *misc_regmap;
	void __iomem *crg_ctrl;
	struct hiedmacv310_phy_chan *phy_chans;
	struct dma_pool *pool;
	unsigned int misc_ctrl_base;
	unsigned int irq;
	unsigned int id;
	struct clk *clk;
	struct clk *axi_clk;
	struct reset_control *rstc;
	unsigned int channels;
	unsigned int slave_requests;
	unsigned int max_transfer_size;
};

#ifdef DEBUG_HIEDMAC
void dump_lli(u32 *llis_vaddr, unsigned int num)
{

	hiedmac_lli *plli = (hiedmac_lli *)llis_vaddr;
	unsigned int i;

	hiedmacv310_trace(3, "lli num = 0%d\n", num);
	for (i = 0;i < num; i++)
	{
		printk("lli%d:lli_L:      0x%llx\n", i, plli[i].next_lli & 0xffffffff);
		printk("lli%d:lli_H:      0x%llx\n", i, plli[i].next_lli >> 32 & 0xffffffff);
		printk("lli%d:count:      0x%llx\n", i, plli[i].count);
		printk("lli%d:src_addr_L: 0x%llx\n", i, plli[i].src_addr & 0xffffffff);
		printk("lli%d:src_addr_H: 0x%llx\n", i, plli[i].src_addr >> 32 & 0xffffffff);
		printk("lli%d:dst_addr_L: 0x%llx\n", i, plli[i].dest_addr & 0xffffffff);
		printk("lli%d:dst_addr_H: 0x%llx\n", i, plli[i].dest_addr >> 32 & 0xffffffff);
		printk("lli%d:CONFIG:	  0x%llx\n", i, plli[i].config);
	}
}

#else
void dump_lli(u32 *llis_vaddr, unsigned int num)
{
}
#endif

static inline struct hiedmacv310_dma_chan *to_edamc_chan(struct dma_chan *chan)
{
	return container_of(chan, struct hiedmacv310_dma_chan, virt_chan.chan);
}

static inline struct transfer_desc *to_edmac_transfer_desc(struct dma_async_tx_descriptor *tx)
{
	return container_of(tx, struct transfer_desc, virt_desc.tx);
}

static struct dma_chan *hiedmac_find_chan_id(struct hiedmacv310_driver_data *hiedmac,
		int request_num)
{
	struct hiedmacv310_dma_chan *edmac_dma_chan = NULL;

	list_for_each_entry(edmac_dma_chan, &hiedmac->slave.channels, virt_chan.chan.device_node) {
		if (edmac_dma_chan->id == request_num)
			return &edmac_dma_chan->virt_chan.chan;
	}
	return NULL;
}

static struct dma_chan *hiedma_of_xlate(struct of_phandle_args *dma_spec,
		struct of_dma *ofdma)
{
	struct hiedmacv310_driver_data *hiedmac = ofdma->of_dma_data;
	struct hiedmacv310_dma_chan *edmac_dma_chan = NULL;
	struct dma_chan *dma_chan;
	struct regmap *misc = hiedmac->misc_regmap;
	unsigned int signal = 0, request_num = 0;
	unsigned int reg = 0, offset = 0;

	if (!hiedmac)
		return NULL;

	if (dma_spec->args_count != 2) {
		hiedmacv310_error("args count not true!\n");
		return NULL;
	}

	request_num = dma_spec->args[0];
	signal = dma_spec->args[1];

	hiedmacv310_trace(3, "host->id = %d,signal = %d, request_num = %d\n",hiedmac->id, signal, request_num);

	offset = hiedmac->misc_ctrl_base + (request_num & (~0x3));
	regmap_read(misc, offset , &reg);
	reg &= ~(0x3f << ((request_num & 0x3) << 3));
	reg |= signal << ((request_num & 0x3) << 3);
	regmap_write(misc, offset, reg);

	hiedmacv310_trace(3, "offset = 0x%x, reg = 0x%x\n", offset, reg);

	dma_chan = hiedmac_find_chan_id(hiedmac, request_num);
	if (!dma_chan) {
		hiedmacv310_error("DMA slave channel is not found!\n");
		return NULL;
	}

	edmac_dma_chan = to_edamc_chan(dma_chan);
	edmac_dma_chan->signal = request_num;

	return dma_get_slave_channel(dma_chan);
}

static int get_of_probe(struct hiedmacv310_driver_data *hiedmac)
{
	struct resource *res;
	struct platform_device *platdev = hiedmac->dev;
	struct device_node *np = platdev->dev.of_node;
	int ret;

	ret = of_property_read_u32((&platdev->dev)->of_node,
			"devid",&(hiedmac->id));
	if (ret) {
		hiedmacv310_error("get hiedmac id fail\n");
		return -ENODEV;
	}

	hiedmac->clk = devm_clk_get(&(platdev->dev), "apb_pclk");
	if (IS_ERR(hiedmac->clk)) {
		return PTR_ERR(hiedmac->clk);
	}

	hiedmac->axi_clk = devm_clk_get(&(platdev->dev), "axi_aclk");
	if (IS_ERR(hiedmac->axi_clk)) {
		return PTR_ERR(hiedmac->axi_clk);
	}

	hiedmac->rstc = devm_reset_control_get(&(platdev->dev), "dma-reset");
	if (IS_ERR(hiedmac->rstc))
		return PTR_ERR(hiedmac->rstc);

	res = platform_get_resource(platdev, IORESOURCE_MEM, 0);
	if (!res) {
		hiedmacv310_error("no reg resource\n");
		return -ENODEV;
	}

	hiedmac->base = devm_ioremap_resource(&(platdev->dev), res);
	if (IS_ERR(hiedmac->base))
		return PTR_ERR(hiedmac->base);

	hiedmac->misc_regmap = syscon_regmap_lookup_by_phandle(np, "misc_regmap");
	if (IS_ERR(hiedmac->misc_regmap))
		return PTR_ERR(hiedmac->misc_regmap);

	ret = of_property_read_u32((&platdev->dev)->of_node,
			"misc_ctrl_base",&(hiedmac->misc_ctrl_base));
	if (ret) {
		hiedmacv310_error( "get dma-misc_ctrl_base fail\n");
		return -ENODEV;
	}

	hiedmac->irq = platform_get_irq(platdev, 0);
	if (unlikely(hiedmac->irq < 0))
		return -ENODEV;

	ret = of_property_read_u32((&platdev->dev)->of_node,
			"dma-channels",&(hiedmac->channels));
	if (ret) {
		hiedmacv310_error( "get dma-channels fail\n");
		return -ENODEV;
	}
	ret = of_property_read_u32((&platdev->dev)->of_node,
			"dma-requests",&(hiedmac->slave_requests));
	if (ret) {
		hiedmacv310_error( "get dma-requests fail\n");
		return -ENODEV;
	}
	hiedmacv310_trace(2,"dma-channels = %d, dma-requests = %d\n",
			hiedmac->channels, hiedmac->slave_requests);
	return of_dma_controller_register(platdev->dev.of_node, hiedma_of_xlate, hiedmac);
}

static void hiedmac_free_chan_resources(struct dma_chan *chan)
{
	vchan_free_chan_resources(to_virt_chan(chan));
}

static enum dma_status hiedmac_tx_status(struct dma_chan *chan,
		dma_cookie_t cookie, struct dma_tx_state *txstate)
{
	enum dma_status ret = DMA_COMPLETE;
	struct hiedmacv310_dma_chan *edmac_dma_chan = to_edamc_chan(chan);
	struct hiedmacv310_phy_chan *phychan = edmac_dma_chan->phychan;
	struct hiedmacv310_driver_data *hiedmac = edmac_dma_chan->host;
	struct virt_dma_desc *vd;
	struct transfer_desc *tsf_desc;
	unsigned long flags;
	size_t bytes = 0;
	u64 curr_lli = 0, curr_residue_bytes = 0, temp = 0;
	hiedmac_lli *plli;
	unsigned int i  = 0, index = 0;

	ret = dma_cookie_status(chan, cookie, txstate);
	if (ret == DMA_COMPLETE) {
		return ret;
	}

	spin_lock_irqsave(&edmac_dma_chan->virt_chan.lock, flags);
	vd = vchan_find_desc(&edmac_dma_chan->virt_chan, cookie);
	if (vd) {
		/* no been trasfer */
		tsf_desc = to_edmac_transfer_desc(&vd->tx);
		bytes = tsf_desc->size;
	} else {
		/* trasfering */
		tsf_desc = edmac_dma_chan->at;

		if (!phychan || !tsf_desc) {
			bytes = 0;
			spin_unlock_irqrestore(&edmac_dma_chan->virt_chan.lock, flags);
			goto out;
		}
		curr_lli = (hiedmacv310_readl(hiedmac->base + HIEDMAC_Cx_LLI_L(phychan->id)) & (~(HIEDMAC_LLI_ALIGN-1)));
		curr_lli |= (hiedmacv310_readl(hiedmac->base + HIEDMAC_Cx_LLI_H(phychan->id)) & 0xffffffff) << 32;
		curr_residue_bytes = hiedmacv310_readl(hiedmac->base + HIEDMAC_Cx_CURR_CNT0(phychan->id));
		if (curr_lli == 0) {
			/* It means non-lli mode */
			bytes = curr_residue_bytes;
		} else {
			/* It means lli mode */
			index = (curr_lli - tsf_desc->llis_busaddr) / sizeof(hiedmac_lli) - 1;
			plli = (hiedmac_lli *)(tsf_desc->llis_vaddr);
			for (i = 0; i < index; i++) {
				temp += plli[i].count;
			}
			temp += plli[i].count - curr_residue_bytes;
			bytes = tsf_desc->size - temp;
		}
	}
	spin_unlock_irqrestore(&edmac_dma_chan->virt_chan.lock, flags);

	dma_set_residue(txstate, bytes);

	if (edmac_dma_chan->state == HIEDMAC_CHAN_PAUSED && ret == DMA_IN_PROGRESS) {
		ret = DMA_PAUSED;
		return ret;
	}

out:
	return ret;
}

static struct hiedmacv310_phy_chan *hiedmac_get_phy_channel(
		struct hiedmacv310_driver_data *hiedmac,
		struct hiedmacv310_dma_chan *edmac_dma_chan)
{
	struct hiedmacv310_phy_chan *ch = NULL;
	unsigned long flags;
	int i;

	for (i = 0; i < hiedmac->channels; i++) {
		ch = &hiedmac->phy_chans[i];

		spin_lock_irqsave(&ch->lock, flags);

		if (!ch->serving) {
			ch->serving = edmac_dma_chan;
			spin_unlock_irqrestore(&ch->lock, flags);
			break;
		}
		spin_unlock_irqrestore(&ch->lock, flags);
	}

	if (i == hiedmac->channels) {
		return NULL;
	}

	return ch;
}

static void hiedmac_write_lli(struct hiedmacv310_driver_data *hiedmac,
		struct hiedmacv310_phy_chan *phychan,
		struct transfer_desc *tsf_desc)
{

	hiedmac_lli *plli = (hiedmac_lli *)tsf_desc->llis_vaddr;

	if (plli->next_lli != 0x0)
		hiedmacv310_writel((plli->next_lli & 0xffffffff) | HIEDMAC_LLI_ENABLE , hiedmac->base + HIEDMAC_Cx_LLI_L(phychan->id));

	else
		hiedmacv310_writel((plli->next_lli & 0xffffffff) , hiedmac->base + HIEDMAC_Cx_LLI_L(phychan->id));

	hiedmacv310_writel(((plli->next_lli >> 32) & 0xffffffff) , hiedmac->base + HIEDMAC_Cx_LLI_H(phychan->id));
	hiedmacv310_writel(plli->count, hiedmac->base + HIEDMAC_Cx_CNT0(phychan->id));
	hiedmacv310_writel(plli->src_addr & 0xffffffff , hiedmac->base + HIEDMAC_Cx_SRC_ADDR_L(phychan->id));
	hiedmacv310_writel((plli->src_addr >> 32) & 0xffffffff , hiedmac->base + HIEDMAC_Cx_SRC_ADDR_H(phychan->id));
	hiedmacv310_writel(plli->dest_addr & 0xffffffff , hiedmac->base + HIEDMAC_Cx_DEST_ADDR_L(phychan->id));
	hiedmacv310_writel((plli->dest_addr >> 32) & 0xffffffff , hiedmac->base + HIEDMAC_Cx_DEST_ADDR_H(phychan->id));
	hiedmacv310_writel(plli->config, hiedmac->base + HIEDMAC_Cx_CONFIG(phychan->id));
}

static void hiedmac_start_next_txd(struct hiedmacv310_dma_chan *edmac_dma_chan)
{
	struct hiedmacv310_driver_data *hiedmac = edmac_dma_chan->host;
	struct hiedmacv310_phy_chan *phychan = edmac_dma_chan->phychan;
	struct virt_dma_desc *vd = vchan_next_desc(&edmac_dma_chan->virt_chan);
	struct transfer_desc *tsf_desc = to_edmac_transfer_desc(&vd->tx);
	unsigned int val = 0;

	list_del(&tsf_desc->virt_desc.node);

	edmac_dma_chan->at = tsf_desc;

	hiedmac_write_lli(hiedmac, phychan, tsf_desc);

	val = hiedmacv310_readl(hiedmac->base + HIEDMAC_Cx_CONFIG(phychan->id));

	hiedmacv310_trace(2," HIEDMAC_Cx_CONFIG  = 0x%x\n", val);
	hiedmacv310_writel(val | HIEDMAC_CxCONFIG_LLI_START, hiedmac->base + HIEDMAC_Cx_CONFIG(phychan->id));
}

static void hiedmac_start(struct hiedmacv310_dma_chan * edmac_dma_chan)
{
	struct hiedmacv310_driver_data *hiedmac = edmac_dma_chan->host;
	struct hiedmacv310_phy_chan *ch;

	ch = hiedmac_get_phy_channel(hiedmac, edmac_dma_chan);
	if (!ch) {
		hiedmacv310_error("no phy channel available !\n");
		edmac_dma_chan->state = HIEDMAC_CHAN_WAITING;
		return;
	}

	edmac_dma_chan->phychan = ch;
	edmac_dma_chan->state = HIEDMAC_CHAN_RUNNING;

	hiedmac_start_next_txd(edmac_dma_chan);
}

static void hiedmac_issue_pending(struct dma_chan *chan)
{
	struct hiedmacv310_dma_chan *edmac_dma_chan = to_edamc_chan(chan);
	unsigned long flags;

	spin_lock_irqsave(&edmac_dma_chan->virt_chan.lock, flags);
	if (vchan_issue_pending(&edmac_dma_chan->virt_chan)) {
		if (!edmac_dma_chan->phychan && edmac_dma_chan->state != HIEDMAC_CHAN_WAITING) {
			hiedmac_start(edmac_dma_chan);
		}
	}
	spin_unlock_irqrestore(&edmac_dma_chan->virt_chan.lock, flags);
}

static void hiedmac_free_txd_list(struct hiedmacv310_dma_chan *edmac_dma_chan)
{
	LIST_HEAD(head);

	vchan_get_all_descriptors(&edmac_dma_chan->virt_chan, &head);
	vchan_dma_desc_free_list(&edmac_dma_chan->virt_chan, &head);
}

static int hiedmac_config(struct dma_chan *chan,
		struct dma_slave_config *config)
{
	struct hiedmacv310_dma_chan *edmac_dma_chan = to_edamc_chan(chan);

	if (!edmac_dma_chan->slave) {
		hiedmacv310_error("slave is null!");
		return -EINVAL;
	}

	edmac_dma_chan->cfg = *config;

	return 0;
}

static void hiedmac_pause_phy_chan(struct hiedmacv310_dma_chan *edmac_dma_chan)
{
	struct hiedmacv310_driver_data *hiedmac = edmac_dma_chan->host;
	struct hiedmacv310_phy_chan *phychan = edmac_dma_chan->phychan;
	unsigned int val;
	int timeout;


	val = hiedmacv310_readl(hiedmac->base + HIEDMAC_Cx_CONFIG(phychan->id));
	val &= ~CCFG_EN;
	hiedmacv310_writel(val, hiedmac->base + HIEDMAC_Cx_CONFIG(phychan->id));

	/* Wait for channel inactive */
	for (timeout = 2000; timeout > 0; timeout--) {
		if (!(0x1 << phychan->id & hiedmacv310_readl(hiedmac->base + HIEDMAC_CH_STAT)))
			break;
		hiedmacv310_writel(val, hiedmac->base + HIEDMAC_Cx_CONFIG(phychan->id));
		udelay(1);
	}

	if (timeout == 0) {
		hiedmacv310_error(":channel%u timeout waiting for pause, timeout:%d\n",
				phychan->id, timeout);
	}
	return 0;
}

static int hiedmac_pause(struct dma_chan *chan)
{
	struct hiedmacv310_dma_chan *edmac_dma_chan = to_edamc_chan(chan);
	unsigned long flags;

	spin_lock_irqsave(&edmac_dma_chan->virt_chan.lock, flags);

	if (!edmac_dma_chan->phychan && !edmac_dma_chan->at) {
		spin_unlock_irqrestore(&edmac_dma_chan->virt_chan.lock, flags);
		return 0;
	}

	hiedmac_pause_phy_chan(edmac_dma_chan);
	edmac_dma_chan->state = HIEDMAC_CHAN_PAUSED;
	spin_unlock_irqrestore(&edmac_dma_chan->virt_chan.lock, flags);

	return 0;
}

static void hiedmac_resume_phy_chan(struct hiedmacv310_dma_chan *edmac_dma_chan)
{
	struct hiedmacv310_driver_data *hiedmac = edmac_dma_chan->host;
	struct hiedmacv310_phy_chan *phychan = edmac_dma_chan->phychan;
	unsigned int val;

	val = hiedmacv310_readl(hiedmac->base + HIEDMAC_Cx_CONFIG(phychan->id));
	val |= CCFG_EN;
	hiedmacv310_writel(val, hiedmac->base + HIEDMAC_Cx_CONFIG(phychan->id));

	return 0;
}

static int hiedmac_resume(struct dma_chan *chan)
{
	struct hiedmacv310_dma_chan *edmac_dma_chan = to_edamc_chan(chan);
	unsigned long flags;

	spin_lock_irqsave(&edmac_dma_chan->virt_chan.lock, flags);

	if (!edmac_dma_chan->phychan && !edmac_dma_chan->at) {
		spin_unlock_irqrestore(&edmac_dma_chan->virt_chan.lock, flags);
		return 0;
	}

	hiedmac_resume_phy_chan(edmac_dma_chan);
	edmac_dma_chan->state = HIEDMAC_CHAN_RUNNING;
	spin_unlock_irqrestore(&edmac_dma_chan->virt_chan.lock, flags);

	return 0;
}

void hiedmac_phy_free(struct hiedmacv310_dma_chan *chan);
static void hiedmac_desc_free(struct virt_dma_desc *vd);
static int hiedmac_terminate_all(struct dma_chan *chan)
{
	struct hiedmacv310_dma_chan *edmac_dma_chan = to_edamc_chan(chan);
	unsigned long flags;

	spin_lock_irqsave(&edmac_dma_chan->virt_chan.lock, flags);
	if (!edmac_dma_chan->phychan && !edmac_dma_chan->at) {
		spin_unlock_irqrestore(&edmac_dma_chan->virt_chan.lock, flags);
		return 0;
	}

	edmac_dma_chan->state = HIEDMAC_CHAN_IDLE;

	if (edmac_dma_chan->phychan) {
		hiedmac_phy_free(edmac_dma_chan);
	}

	if (edmac_dma_chan->at) {
		hiedmac_desc_free(&edmac_dma_chan->at->virt_desc);
		edmac_dma_chan->at = NULL;
	}
	hiedmac_free_txd_list(edmac_dma_chan);

	spin_unlock_irqrestore(&edmac_dma_chan->virt_chan.lock, flags);

	return 0;
}

static struct transfer_desc *hiedmac_get_tsf_desc(struct hiedmacv310_driver_data *plchan)
{
	struct transfer_desc *tsf_desc = kzalloc(sizeof(struct transfer_desc), GFP_NOWAIT);

	if (tsf_desc) {
		tsf_desc->ccfg = 0;
	}

	return tsf_desc;
}

static void hiedmac_free_tsf_desc(struct hiedmacv310_driver_data *hiedmac,
		struct transfer_desc *tsf_desc)
{
	if (tsf_desc->llis_vaddr) {
		dma_pool_free(hiedmac->pool, tsf_desc->llis_vaddr, tsf_desc->llis_busaddr);
	}

	kfree(tsf_desc);
}

static u32 get_width(enum dma_slave_buswidth width)
{
	switch (width) {
		case DMA_SLAVE_BUSWIDTH_1_BYTE:
			return HIEDMAC_WIDTH_8BIT;
		case DMA_SLAVE_BUSWIDTH_2_BYTES:
			return HIEDMAC_WIDTH_16BIT;
		case DMA_SLAVE_BUSWIDTH_4_BYTES:
			return HIEDMAC_WIDTH_32BIT;
		case DMA_SLAVE_BUSWIDTH_8_BYTES:
			return HIEDMAC_WIDTH_64BIT;
		default: 
			hiedmacv310_error("check here, width warning!\n");
			return ~0;
	}
}

struct transfer_desc *hiedmac_init_tsf_desc (
		struct dma_chan *chan,
		enum dma_transfer_direction direction,
		dma_addr_t *slave_addr)
{
	struct hiedmacv310_dma_chan *edmac_dma_chan = to_edamc_chan(chan);
	struct hiedmacv310_driver_data *hiedmac = edmac_dma_chan->host;
	struct transfer_desc *tsf_desc;
	unsigned int config = 0, burst = 0;
	unsigned int addr_width = 0, maxburst = 0;
	unsigned int width = 0;

	tsf_desc = hiedmac_get_tsf_desc(hiedmac);
	if (!tsf_desc) {
		hiedmacv310_error("get tsf desc fail!\n");
		return NULL;
	}

	if (direction == DMA_MEM_TO_DEV) {
		config = HIEDMAC_CONFIG_SRC_INC;
		*slave_addr = edmac_dma_chan->cfg.dst_addr;
		addr_width = edmac_dma_chan->cfg.dst_addr_width;
		maxburst = edmac_dma_chan->cfg.dst_maxburst;
	} else if (direction == DMA_DEV_TO_MEM){
		config = HIEDMAC_CONFIG_DST_INC;
		*slave_addr = edmac_dma_chan->cfg.src_addr;
		addr_width = edmac_dma_chan->cfg.src_addr_width;
		maxburst = edmac_dma_chan->cfg.src_maxburst;
	} else {
		hiedmac_free_tsf_desc(hiedmac, tsf_desc);
		hiedmacv310_error("direction unsupported!\n");
		return NULL;
	}

	hiedmacv310_trace(3, "addr_width = 0x%x\n",addr_width);
	width = get_width(addr_width);
	hiedmacv310_trace(3, "width = 0x%x\n",width);
	config |= width << HIEDMAC_CONFIG_SRC_WIDTH_SHIFT;
	config |= width << HIEDMAC_CONFIG_DST_WIDTH_SHIFT;
	hiedmacv310_trace(2, "tsf_desc->ccfg = 0x%x\n",config);

	hiedmacv310_trace(3, "maxburst = 0x%x\n", maxburst);
	if (maxburst > (HIEDMAC_MAX_BURST_WIDTH))
		burst |= (HIEDMAC_MAX_BURST_WIDTH - 1);
	else if (maxburst == 0)
		burst |= HIEDMAC_MIN_BURST_WIDTH;
	else
		burst |= (maxburst - 1);
	hiedmacv310_trace(3, "burst = 0x%x\n", burst);
	config |= burst << HIEDMAC_CONFIG_SRC_BURST_SHIFT;
	config |= burst << HIEDMAC_CONFIG_DST_BURST_SHIFT;

	if (edmac_dma_chan->signal >= 0) {
		hiedmacv310_trace(2, "edmac_dma_chan->signal = %d\n", edmac_dma_chan->signal);
		config |= edmac_dma_chan->signal << HIEDMAC_CXCONFIG_SIGNAL_SHIFT;
	}

	config |= HIEDMAC_CXCONFIG_DEV_MEM_TYPE << HIEDMAC_CXCONFIG_TSF_TYPE_SHIFT;
	tsf_desc->ccfg = config;
	hiedmacv310_trace(2, "tsf_desc->ccfg = 0x%x\n",tsf_desc->ccfg);

	return tsf_desc;
}

static void hiedmac_fill_desc(struct transfer_desc *tsf_desc, dma_addr_t src, 
		dma_addr_t dst, unsigned int length, unsigned int num)
{
	hiedmac_lli *plli;

	plli = (hiedmac_lli *)(tsf_desc->llis_vaddr);
	memset(&plli[num], 0x0, sizeof(hiedmac_lli));

	plli[num].src_addr = src;
	plli[num].dest_addr = dst;
	plli[num].config = tsf_desc->ccfg;
	plli[num].count = length;
	tsf_desc->size += length;

	if (num > 0)
	{
		plli[num - 1].next_lli = (tsf_desc->llis_busaddr + (num)*sizeof(hiedmac_lli)) & (~(HIEDMAC_LLI_ALIGN-1));
		plli[num - 1].next_lli |= HIEDMAC_LLI_ENABLE;
	}
}

static struct dma_async_tx_descriptor *hiedmac_perp_slave_sg(
		struct dma_chan *chan, struct scatterlist *sgl,
		unsigned int sg_len, enum dma_transfer_direction direction,
		unsigned long flags, void *context)
{
	struct hiedmacv310_dma_chan *edmac_dma_chan = to_edamc_chan(chan); 
	struct hiedmacv310_driver_data *hiedmac = edmac_dma_chan->host;
	struct transfer_desc *tsf_desc;
	struct scatterlist *sg;
	int tmp;
	dma_addr_t  src = 0, dst = 0, addr = 0, slave_addr = 0;
	unsigned int length = 0, num = 0;

	hiedmac_lli *last_plli = NULL;

	if (sgl == NULL) {
		hiedmacv310_error("sgl is null!\n");
		return NULL;
	}

	tsf_desc = hiedmac_init_tsf_desc(chan, direction, &slave_addr);
	if (!tsf_desc) {
		hiedmacv310_error("desc init fail\n");
		return NULL;
	}

	tsf_desc->llis_vaddr = dma_pool_alloc(hiedmac->pool, GFP_NOWAIT, &tsf_desc->llis_busaddr);
	if (!tsf_desc->llis_vaddr) {
		hiedmac_free_tsf_desc(hiedmac, tsf_desc);
		hiedmacv310_error("malloc memory from pool fail !\n");
		return 0;
	}

	for_each_sg(sgl, sg, sg_len, tmp) {
		addr = sg_dma_address(sg);
		length = sg_dma_len(sg);
		if (direction == DMA_MEM_TO_DEV) {
			src = addr;
			dst = slave_addr;
		} else if (direction == DMA_DEV_TO_MEM) {
			src = slave_addr;
			dst = addr;
		}
		hiedmac_fill_desc(tsf_desc, src, dst, length, num);
		num++;
	}

	last_plli = (hiedmac_lli *)(tsf_desc->llis_vaddr + (num - 1)*sizeof(hiedmac_lli));
	last_plli->next_lli |= HIEDMAC_LLI_DISABLE;
	dump_lli(tsf_desc->llis_vaddr, num);

	return vchan_tx_prep(&edmac_dma_chan->virt_chan, &tsf_desc->virt_desc, flags);
}

static struct dma_async_tx_descriptor *hiedmac_prep_dma_memcpy(
		struct dma_chan *chan, dma_addr_t dest, dma_addr_t src,
		size_t len, unsigned long flags)
{
	struct hiedmacv310_dma_chan *edmac_dma_chan = to_edamc_chan(chan);
	struct hiedmacv310_driver_data *hiedmac = edmac_dma_chan->host;
	struct transfer_desc *tsf_desc;
	u32 config = 0;
	size_t num = 0;
	size_t length = 0;
	hiedmac_lli *last_plli = NULL;

	if (!len)
		return NULL;

	tsf_desc = hiedmac_get_tsf_desc(hiedmac);
	if (!tsf_desc) {
		hiedmacv310_error("get tsf desc fail!\n");
		return NULL;
	}

	tsf_desc->llis_vaddr = dma_pool_alloc(hiedmac->pool, GFP_NOWAIT, &tsf_desc->llis_busaddr);
	if (!tsf_desc->llis_vaddr) {
		hiedmac_free_tsf_desc(hiedmac, tsf_desc);
		hiedmacv310_error("malloc memory from pool fail !\n");
		return 0;
	}

	config |= HIEDMAC_CONFIG_SRC_INC | HIEDMAC_CONFIG_DST_INC;
	config |= HIEDMAC_CXCONFIG_MEM_TYPE << HIEDMAC_CXCONFIG_TSF_TYPE_SHIFT;

	/*  max burst width is 16 ,but reg value set 0xf */
	config |= (HIEDMAC_MAX_BURST_WIDTH - 1) << HIEDMAC_CONFIG_SRC_BURST_SHIFT;
	config |= (HIEDMAC_MAX_BURST_WIDTH - 1)<< HIEDMAC_CONFIG_DST_BURST_SHIFT;

	tsf_desc->ccfg = config;

	do {
		length = min_t(size_t, len, MAX_TRANSFER_BYTES);
		hiedmac_fill_desc(tsf_desc, src, dest, length, num);

		src += length;
		dest += length;
		len -= length;
		num++;
	} while(len);

	last_plli = (hiedmac_lli *)(tsf_desc->llis_vaddr + (num - 1)*sizeof(hiedmac_lli));
	last_plli->next_lli |= HIEDMAC_LLI_DISABLE;
	dump_lli(tsf_desc->llis_vaddr, num);

	return vchan_tx_prep(&edmac_dma_chan->virt_chan, &tsf_desc->virt_desc, flags);
}



static struct dma_async_tx_descriptor *hiemdac_prep_dma_cyclic(
		struct dma_chan *chan, dma_addr_t buf_addr, size_t buf_len,
		size_t period_len, enum dma_transfer_direction direction,
		unsigned long flags)
{
	struct hiedmacv310_dma_chan *edmac_dma_chan = to_edamc_chan(chan);
	struct hiedmacv310_driver_data *hiedmac = edmac_dma_chan->host;
	struct transfer_desc *tsf_desc;
	dma_addr_t  src = 0, dst = 0, addr = 0, slave_addr = 0;
	size_t length = 0, since = 0, total = 0, num = 0, len = 0;
	hiedmac_lli *last_plli = NULL;
	hiedmac_lli *plli = NULL;

	tsf_desc = hiedmac_init_tsf_desc(chan, direction, &slave_addr);
	if (!tsf_desc) {
		hiedmacv310_error("desc init fail\n");
		return NULL;
	}

	tsf_desc->llis_vaddr = dma_pool_alloc(hiedmac->pool, GFP_NOWAIT, &tsf_desc->llis_busaddr);
	if (!tsf_desc->llis_vaddr) {
		hiedmacv310_error("malloc memory from pool fail !\n");
		return 0;
	}

	tsf_desc->cyclic = true;
	addr = buf_addr;
	total = buf_len;

	if (period_len < MAX_TRANSFER_BYTES)
		len = period_len;
	do
	{
		length = min_t(size_t, total, len);

		if (direction == DMA_MEM_TO_DEV) {
			src = addr;
			dst = slave_addr;
		} else if (direction == DMA_DEV_TO_MEM) {
			src = slave_addr;
			dst = addr;
		}

		hiedmac_fill_desc(tsf_desc, src, dst, length, num);

		since += length;
		if (since >= period_len) {
			plli = (hiedmac_lli *)(tsf_desc->llis_vaddr + (num)*sizeof(hiedmac_lli));
			plli->config |= HIEDMAC_CXCONFIG_ITC_EN << HIEDMAC_CXCONFIG_ITC_EN_SHIFT;
			since -= period_len;
		}
		addr += length;
		total -= length;
		num++;
	} while(total);

	last_plli = (hiedmac_lli *)(tsf_desc->llis_vaddr + (num - 1)*sizeof(hiedmac_lli));

	last_plli->next_lli = (u64)(tsf_desc->llis_vaddr);

	dump_lli(tsf_desc->llis_vaddr, num);

	return vchan_tx_prep(&edmac_dma_chan->virt_chan, &tsf_desc->virt_desc, flags);
}


static void  hiedmac_phy_reassign(struct hiedmacv310_phy_chan *phy_chan,
		struct hiedmacv310_dma_chan *chan)
{
	phy_chan->serving = chan;
	chan->phychan = phy_chan;
	chan->state = HIEDMAC_CHAN_RUNNING;

	hiedmac_start_next_txd(chan);
}

static void hiedmac_terminate_phy_chan(struct hiedmacv310_driver_data *hiedmac,
		struct hiedmacv310_dma_chan *edmac_dma_chan)
{
	unsigned int val;
	struct hiedmacv310_phy_chan *phychan = edmac_dma_chan->phychan;

	hiedmac_pause_phy_chan(edmac_dma_chan);

	val = 0x1 << phychan->id;

	hiedmacv310_writel(val, hiedmac->base + HIEDMAC_INT_TC1_RAW);
	hiedmacv310_writel(val, hiedmac->base + HIEDMAC_INT_ERR1_RAW);
	hiedmacv310_writel(val, hiedmac->base + HIEDMAC_INT_ERR2_RAW);
}

void hiedmac_phy_free(struct hiedmacv310_dma_chan *chan)
{
	struct hiedmacv310_driver_data *hiedmac = chan->host;
	struct hiedmacv310_dma_chan *p, *next = NULL;

	list_for_each_entry(p, &hiedmac->memcpy.channels, virt_chan.chan.device_node) {
		if (p->state == HIEDMAC_CHAN_WAITING) {
			next = p;
			break;
		}
	}

	if (!next) {
		list_for_each_entry(p, &hiedmac->slave.channels, virt_chan.chan.device_node) {
			if (p->state == HIEDMAC_CHAN_WAITING) {
				next = p;
				break;
			}
		}
	}
	hiedmac_terminate_phy_chan(hiedmac, chan);

	if (next) {
		spin_lock(&next->virt_chan.lock);
		hiedmac_phy_reassign(chan->phychan, next);
		spin_unlock(&next->virt_chan.lock);
	} else {
		chan->phychan->serving = NULL;
	}

	chan->phychan = NULL;
	chan->state = HIEDMAC_CHAN_IDLE;
}

static irqreturn_t hiemdacv310_irq(int irq, void *dev)
{
	struct hiedmacv310_driver_data *hiedmac = (struct hiedmacv310_driver_data *)dev;
	struct hiedmacv310_dma_chan *chan = NULL;
	struct hiedmacv310_phy_chan *phy_chan = NULL;
	struct transfer_desc * tsf_desc = NULL;

	u32 mask = 0;
	unsigned int channel_err_status[3];
	unsigned int channel_status = 0;
	unsigned int temp = 0;
	unsigned int channel_tc_status = -1;
	int i = 0;

	channel_status = hiedmacv310_readl(hiedmac->base + HIEDMAC_INT_STAT);

	if (!channel_status) {
		hiedmacv310_error("channel_status = 0x%x\n", channel_status);
		return IRQ_NONE;
	}

	for (i = 0; i < hiedmac->channels; i++)
	{
		temp = (channel_status >> i) & 0x1;
		if (temp) {
			phy_chan = &hiedmac->phy_chans[i];
			chan = phy_chan->serving;
			tsf_desc = chan->at;

			channel_tc_status = hiedmacv310_readl(hiedmac->base + HIEDMAC_INT_TC1_RAW);
			channel_tc_status = (channel_tc_status >> i) &0x01;
			if (channel_tc_status)
				hiedmacv310_writel(channel_tc_status << i, hiedmac->base + HIEDMAC_INT_TC1_RAW);

			channel_tc_status = hiedmacv310_readl(hiedmac->base + HIEDMAC_INT_TC2);
			channel_tc_status = (channel_tc_status >> i) &0x01;
			if (channel_tc_status)
				hiedmacv310_writel(channel_tc_status << i, hiedmac->base + HIEDMAC_INT_TC2_RAW);

			if (!chan) {
				hiedmacv310_error("error interrupt on chan: %d!\n",i);
				continue;
			}

			channel_err_status[0] = hiedmacv310_readl(hiedmac->base + HIEDMAC_INT_ERR1);
			channel_err_status[0] = (channel_err_status[0] >> i) & 0x01;
			channel_err_status[1] = hiedmacv310_readl(hiedmac->base + HIEDMAC_INT_ERR2);
			channel_err_status[1] = (channel_err_status[1] >> i) & 0x01;
			channel_err_status[2] = hiedmacv310_readl(hiedmac->base + HIEDMAC_INT_ERR3);
			channel_err_status[2] = (channel_err_status[2] >> i) & 0x01;
			if (channel_err_status[0] | channel_err_status[1] | channel_err_status[2]) {
				hiedmacv310_error("Error in hiedmac %d finish!,ERR1 = 0x%x,ERR2 = 0x%x,ERR3 = 0x%x\n",
						i, channel_err_status[0],channel_err_status[1],channel_err_status[2]);
				hiedmacv310_writel(1<<i, hiedmac->base + HIEDMAC_INT_ERR1_RAW);
				hiedmacv310_writel(1<<i, hiedmac->base + HIEDMAC_INT_ERR2_RAW);
				hiedmacv310_writel(1<<i, hiedmac->base + HIEDMAC_INT_ERR3_RAW);
			}

			spin_lock(&chan->virt_chan.lock);

			if (tsf_desc->cyclic) {
				vchan_cyclic_callback(&tsf_desc->virt_desc);
				spin_unlock(&chan->virt_chan.lock);
				continue;
			}
			chan->at = NULL;
			tsf_desc->done = true;
			vchan_cookie_complete(&tsf_desc->virt_desc);

			if (vchan_next_desc(&chan->virt_chan))
				hiedmac_start_next_txd(chan);
			else
				hiedmac_phy_free(chan);

			spin_unlock(&chan->virt_chan.lock);
			mask |= (1 << i);
		}
	}

	return mask ? IRQ_HANDLED : IRQ_NONE;
}

static void hiedmac_dma_slave_init(struct hiedmacv310_dma_chan *chan)
{
	chan->slave = true;
}

static void hiedmac_desc_free(struct virt_dma_desc *vd)
{
	struct transfer_desc *tsf_desc = to_edmac_transfer_desc(&vd->tx);
	struct hiedmacv310_dma_chan * edmac_dma_chan = to_edamc_chan(vd->tx.chan);

	dma_descriptor_unmap(&vd->tx);
	hiedmac_free_tsf_desc(edmac_dma_chan->host, tsf_desc);
}

static int hiedmac_init_virt_channels(struct hiedmacv310_driver_data *hiedmac,
		struct dma_device *dmadev, unsigned int channels, bool slave)
{
	struct hiedmacv310_dma_chan *chan;
	int i;
	INIT_LIST_HEAD(&dmadev->channels);

	for (i = 0; i < channels; i++)
	{
		chan = kzalloc(sizeof(struct hiedmacv310_dma_chan), GFP_KERNEL);
		chan->host = hiedmac;
		chan->state = HIEDMAC_CHAN_IDLE;
		chan->signal = -1;

		if (slave) {
			chan->id = i;
			hiedmac_dma_slave_init(chan);
		}
		chan->virt_chan.desc_free = hiedmac_desc_free;
		vchan_init(&chan->virt_chan, dmadev);
	}
	return 0;
}

void hiedmac_free_virt_channels(struct dma_device *dmadev)
{
	struct hiedmacv310_dma_chan *chan = NULL;
	struct hiedmacv310_dma_chan *next;

	list_for_each_entry_safe(chan,
			next, &dmadev->channels, virt_chan.chan.device_node) {
		list_del(&chan->virt_chan.chan.device_node);
		kfree(chan);
	}
}


#define MAX_TSFR_LLIS           512
#define EDMACV300_LLI_WORDS     64
#define EDMACV300_POOL_ALIGN    64

static int __init hiedmacv310_probe(struct platform_device *pdev)
{

	int ret = 0, i = 0;
	struct hiedmacv310_driver_data *hiedmac = NULL;
	size_t trasfer_size = 0;

	ret = dma_set_mask_and_coherent(&(pdev->dev), DMA_BIT_MASK(64));
	if (ret)
		return ret;

	hiedmac = kzalloc(sizeof(*hiedmac), GFP_KERNEL);
	if (!hiedmac) {
		hiedmacv310_error("malloc for hiedmac fail!");
		ret = -ENOMEM;
		return ret;
	}
	hiedmac->dev = pdev;

	ret = get_of_probe(hiedmac);
	if (ret) {
		hiedmacv310_error("get dts info fail!");
		goto free_hiedmac;
	}


	clk_prepare_enable(hiedmac->clk);
	clk_prepare_enable(hiedmac->axi_clk);

	reset_control_deassert(hiedmac->rstc);

	dma_cap_set(DMA_MEMCPY, hiedmac->memcpy.cap_mask);
	hiedmac->memcpy.dev = &pdev->dev;
	hiedmac->memcpy.device_free_chan_resources = hiedmac_free_chan_resources;
	hiedmac->memcpy.device_prep_dma_memcpy = hiedmac_prep_dma_memcpy;
	hiedmac->memcpy.device_tx_status = hiedmac_tx_status;
	hiedmac->memcpy.device_issue_pending = hiedmac_issue_pending;
	hiedmac->memcpy.device_config = hiedmac_config;
	hiedmac->memcpy.device_pause = hiedmac_pause;
	hiedmac->memcpy.device_resume = hiedmac_resume;
	hiedmac->memcpy.device_terminate_all = hiedmac_terminate_all;
	hiedmac->memcpy.directions = BIT(DMA_MEM_TO_MEM);
	hiedmac->memcpy.residue_granularity = DMA_RESIDUE_GRANULARITY_SEGMENT;

	dma_cap_set(DMA_SLAVE, hiedmac->slave.cap_mask);
	dma_cap_set(DMA_CYCLIC, hiedmac->slave.cap_mask);
	hiedmac->slave.dev = &pdev->dev;
	hiedmac->slave.device_free_chan_resources = hiedmac_free_chan_resources;
	hiedmac->slave.device_tx_status = hiedmac_tx_status;
	hiedmac->slave.device_issue_pending = hiedmac_issue_pending;
	hiedmac->slave.device_prep_slave_sg = hiedmac_perp_slave_sg;
	hiedmac->slave.device_prep_dma_cyclic = hiemdac_prep_dma_cyclic;
	hiedmac->slave.device_config = hiedmac_config;
	hiedmac->slave.device_resume = hiedmac_resume;
	hiedmac->slave.device_pause = hiedmac_pause;
	hiedmac->slave.device_terminate_all = hiedmac_terminate_all;
	hiedmac->slave.directions = BIT(DMA_DEV_TO_MEM) | BIT(DMA_MEM_TO_DEV);
	hiedmac->slave.residue_granularity = DMA_RESIDUE_GRANULARITY_SEGMENT;

	hiedmac->max_transfer_size = MAX_TRANSFER_BYTES;
	trasfer_size = MAX_TSFR_LLIS * EDMACV300_LLI_WORDS * sizeof(u32);

	hiedmac->pool = dma_pool_create(DRIVER_NAME, &(pdev->dev),
			trasfer_size,  EDMACV300_POOL_ALIGN, 0);
	if (!hiedmac->pool) {
		hiedmacv310_error("create pool fail!");
		ret = -ENOMEM;
		goto free_hiedmac;
	}

	hiedmacv310_writel(HIEDMAC_ALL_CHAN_CLR, hiedmac->base + HIEDMAC_INT_TC1_RAW);
	hiedmacv310_writel(HIEDMAC_ALL_CHAN_CLR, hiedmac->base + HIEDMAC_INT_TC2_RAW);
	hiedmacv310_writel(HIEDMAC_ALL_CHAN_CLR, hiedmac->base + HIEDMAC_INT_ERR1_RAW);
	hiedmacv310_writel(HIEDMAC_ALL_CHAN_CLR, hiedmac->base + HIEDMAC_INT_ERR2_RAW);
	hiedmacv310_writel(HIEDMAC_ALL_CHAN_CLR, hiedmac->base + HIEDMAC_INT_ERR3_RAW);

	hiedmacv310_writel(HIEDMAC_INT_ENABLE_ALL_CHAN , hiedmac->base + HIEDMAC_INT_TC1_MASK);
	hiedmacv310_writel(HIEDMAC_INT_ENABLE_ALL_CHAN , hiedmac->base + HIEDMAC_INT_TC2_MASK);
	hiedmacv310_writel(HIEDMAC_INT_ENABLE_ALL_CHAN , hiedmac->base + HIEDMAC_INT_ERR1_MASK);
	hiedmacv310_writel(HIEDMAC_INT_ENABLE_ALL_CHAN , hiedmac->base + HIEDMAC_INT_ERR2_MASK);
	hiedmacv310_writel(HIEDMAC_INT_ENABLE_ALL_CHAN , hiedmac->base + HIEDMAC_INT_ERR3_MASK);

	ret = request_irq(hiedmac->irq, hiemdacv310_irq, 0, DRIVER_NAME, hiedmac);
	if (ret) {
		hiedmacv310_error("fail to request irq");
		goto free_pool;
	}

	hiedmac->phy_chans = kzalloc((hiedmac->channels * sizeof(struct hiedmacv310_phy_chan)),
			GFP_KERNEL);
	if (!hiedmac->phy_chans) {
		hiedmacv310_error("malloc for phy chans fail!");
		ret = -ENOMEM;
		goto free_irq_res;
	}

	/* initialize  the phy chan */
	for (i = 0; i < hiedmac->channels; i++) {
		struct hiedmacv310_phy_chan* phy_ch = &hiedmac->phy_chans[i];
		phy_ch->id = i;
		phy_ch->base = hiedmac->base + HIEDMAC_Cx_BASE(i);
		spin_lock_init(&phy_ch->lock);
		phy_ch->serving = NULL;
	}

	/* initialize the memory virt chan */
	ret = hiedmac_init_virt_channels(hiedmac, &hiedmac->memcpy, hiedmac->channels, false);
	if (ret) {
		hiedmacv310_error("fail to init memory virt channels!");
		goto  free_phychans;
	}

	/* initialize the slave virt chan */
	ret = hiedmac_init_virt_channels(hiedmac, &hiedmac->slave,  hiedmac->slave_requests, true);
	if (ret) {
		hiedmacv310_error("fail to init slave virt channels!");
		goto  free_memory_virt_channels;

	}

	ret = dma_async_device_register(&hiedmac->memcpy);
	if (ret) {
		hiedmacv310_error(
				"%s failed to register memcpy as an async device - %d\n",
				__func__, ret);
		goto free_slave_virt_channels;
	}

	ret = dma_async_device_register(&hiedmac->slave);
	if (ret) {
		hiedmacv310_error(
				"%s failed to register slave as an async device - %d\n",
				__func__, ret);
		goto free_memcpy_device;
	}

	return 0;

free_memcpy_device:
	dma_async_device_unregister(&hiedmac->memcpy);
free_slave_virt_channels:
	hiedmac_free_virt_channels(&hiedmac->slave);
free_memory_virt_channels:
	hiedmac_free_virt_channels(&hiedmac->memcpy);
free_phychans:
	kfree(hiedmac->phy_chans);
free_irq_res:
	free_irq(hiedmac->irq,hiedmac);
free_pool:
	dma_pool_destroy(hiedmac->pool);
free_hiedmac:
	kfree(hiedmac);

	return ret;
}


static int hiemda_remove(struct platform_device *pdev)
{
	int err = 0;
	return err;
}


static const struct of_device_id hiedmacv310_match[] = {
	{ .compatible = "hisilicon,hiedmacv310" },
	{},
};


static struct platform_driver hiedmacv310_driver = {
	.remove = hiemda_remove,
	.driver = {
		.name   = "hiedmacv310",
		.of_match_table = hiedmacv310_match,
	},
};

static int __init hiedmacv310_init(void)
{
	return platform_driver_probe(&hiedmacv310_driver, hiedmacv310_probe);
}
subsys_initcall(hiedmacv310_init);

static void __exit hiedmacv310_exit(void)
{
	platform_driver_unregister(&hiedmacv310_driver);
}
module_exit(hiedmacv310_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Hisilicon");
