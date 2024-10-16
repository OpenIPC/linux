/* SPDX-License-Identifier: GPL-2.0 */
#include <linux/clk.h>
#include <linux/clk/sunxi.h>
#include <linux/clk-provider.h>
#include <linux/dma-mapping.h>
#include <linux/err.h>
#include <linux/kernel.h>
#include <linux/mfd/syscon.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/regmap.h>
#include <linux/reset.h>
#include <asm/io.h>

#include "sunxi_rproc_boot.h"


/* hardware related operations for sun8iw21-e907 core */
struct sunxi_e907_info {
	void *gating;
	void *cfg;
	void *addr;
	void *clk;
	void *msg_rst;

	int freq;
	u32 boot_addr;
};
static struct sunxi_e907_info e907_info;

static int sunxi_e907_is_running(struct sunxi_e907_info *info)
{
	uint32_t reg_val;

	reg_val = readl(info->cfg) & 0x1;

	return reg_val;
}

static void sunxi_e907_rst_assert(struct sunxi_e907_info *info, int assert)
{
	u32 reg_val = 0;

	if (assert)
		reg_val = (1 << 16) | (1 << 0);

	writel(reg_val, info->cfg);
}

static void sunxi_e907_clk_assert(struct sunxi_e907_info *info, int assert)
{
	u32 reg_val = 0x16aa << 16;

	if (!assert)
		reg_val |= 0x7;

	writel(reg_val, info->gating);
}

static void sunxi_e907_msgbox_assert(struct sunxi_e907_info *info, int assert)
{
	u32 reg_val;

	reg_val = readl(info->msg_rst);
	/* Rest e907 msgbox */
	if (assert)
		reg_val &= ~(1 << 17 | 1 << 1);
	else
		reg_val |= (1 << 17 | 1 << 1);
	writel(reg_val, info->msg_rst);
}

static void sunxi_arch_e907_set_freq(struct sunxi_e907_info *info, u32 freq)
{
	u32 reg_val;

	switch (freq) {
	case 600000000:
		/*  cpu freq = 600M axi = 300M */
		reg_val = (3 << 24) | (1 << 8);
		break;
	case 480000000:
		/*  cpu freq = 480M axi = 140M */
		reg_val = (4 << 24) | (1 << 8);
		break;
	case 16000000:
		/*  cpu freq = 16M axi = 8M */
		reg_val = (2 << 24) | (1 << 8);
		break;
	case 32000:
		/*  cpu freq = 32K axi = 16K */
		reg_val = (1 << 24) | (1 << 8);
		break;
	default:
		reg_val = (3 << 24) | (1 << 8);
		break;
	}
	writel(reg_val, info->clk);
}


static int sunxi_e907_init(struct sunxi_core *core)
{

	e907_info.gating = ioremap(0x02001d04, 0x04);
	if (!PTR_ERR(e907_info.gating))
		goto out_gating;

	e907_info.cfg = ioremap(0x02001d0c, 0x04);
	if (!PTR_ERR(e907_info.cfg))
		goto out_cfg;

	e907_info.addr = ioremap(0x06010204, 0x04);
	if (!PTR_ERR(e907_info.addr))
		goto out_addr;

	e907_info.clk = ioremap(0x02001d00, 0x04);
	if (!PTR_ERR(e907_info.clk))
		goto out_clk;

	e907_info.msg_rst = ioremap(0x0200171c, 0x04);
	if (!PTR_ERR(e907_info.msg_rst))
		goto out_msg;
	return 0;

out_msg:
	iounmap(e907_info.clk);
out_clk:
	iounmap(e907_info.addr);
out_addr:
	iounmap(e907_info.cfg);
out_cfg:
	iounmap(e907_info.gating);
out_gating:
	return -ENOMEM;
}

static void sunxi_e907_deinit(struct sunxi_core *core)
{
	 iounmap(e907_info.gating);
	 iounmap(e907_info.cfg);
	 iounmap(e907_info.addr);
	 iounmap(e907_info.clk);
}

static int sunxi_e907_start(struct sunxi_core *core)
{
	struct sunxi_e907_info *info = core->priv;

	/* turn off clk and asserd cpu */
	sunxi_e907_clk_assert(info, 1);
	/* Reset Cpu */
	sunxi_e907_rst_assert(info, 1);
	/* Reset e907 msgbox */
	sunxi_e907_msgbox_assert(info, 1);

	/* Set Cpu boot addr */
	writel(info->boot_addr, info->addr);

	sunxi_e907_clk_assert(info, 1);
	sunxi_arch_e907_set_freq(info, info->freq);

	/* turn on clk and deassert cpu */
	sunxi_e907_clk_assert(info, 0);
	sunxi_e907_msgbox_assert(info, 0);

	return 0;

}

static int sunxi_e907_is_start(struct sunxi_core *core)
{
	struct sunxi_e907_info *info = core->priv;

	return sunxi_e907_is_running(info);
}

static int sunxi_e907_stop(struct sunxi_core *core)
{
	struct sunxi_e907_info *info = core->priv;
	/* turn off clk and asserd cpu */
	sunxi_e907_clk_assert(info, 1);
	sunxi_e907_rst_assert(info, 0);

	return 0;
}

static void sunxi_e907_set_start_addr(struct sunxi_core *core, u32 addr)
{
	struct sunxi_e907_info *info = core->priv;

	info->boot_addr = addr;
}

static void sunxi_e907_set_freq(struct sunxi_core *core, u32 freq)
{
	struct sunxi_e907_info *info = core->priv;

	info->freq = freq;
}

static struct sunxi_remote_core_ops sun8iw21_e907 = {
	.init = sunxi_e907_init,
	.deinit = sunxi_e907_deinit,
	.start = sunxi_e907_start,
	.is_start = sunxi_e907_is_start,
	.stop = sunxi_e907_stop,
	.set_start_addr = sunxi_e907_set_start_addr,
	.set_freq = sunxi_e907_set_freq,
};

static struct sunxi_core cores[] = {
	{
		.name = "sun8iw21p1-e907",
		.ops = &sun8iw21_e907,
		.priv = &e907_info
	},
};

struct sunxi_core *
sunxi_remote_core_find(const char *name)
{
	int i;
	int core_cnt = ARRAY_SIZE(cores);

	for (i = 0; i < core_cnt; i++) {
		if (!strcmp(cores[i].name, name))
			return &cores[i];
	}

	return NULL;
}
EXPORT_SYMBOL(sunxi_remote_core_find);

int sunxi_core_init(struct sunxi_core *core)
{
	return core->ops->init(core);
}
EXPORT_SYMBOL(sunxi_core_init);

void sunxi_core_deinit(struct sunxi_core *core)
{
	core->ops->deinit(core);
}
EXPORT_SYMBOL(sunxi_core_deinit);

int sunxi_core_start(struct sunxi_core *core)
{
	return core->ops->start(core);
}
EXPORT_SYMBOL(sunxi_core_start);

int sunxi_core_is_start(struct sunxi_core *core)
{
	return core->ops->is_start(core);
}
EXPORT_SYMBOL(sunxi_core_is_start);

int sunxi_core_stop(struct sunxi_core *core)
{
	return core->ops->stop(core);
}
EXPORT_SYMBOL(sunxi_core_stop);

void sunxi_core_set_start_addr(struct sunxi_core *core, u32 addr)
{
	core->ops->set_start_addr(core, addr);
}
EXPORT_SYMBOL(sunxi_core_set_start_addr);

void sunxi_core_set_freq(struct sunxi_core *core, u32 freq)
{
	core->ops->set_freq(core, freq);
}
EXPORT_SYMBOL(sunxi_core_set_freq);

MODULE_LICENSE("GPL v2");

