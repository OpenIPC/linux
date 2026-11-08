// SPDX-License-Identifier: GPL-2.0
/*
 * Fullhan FH8626V100 PMU clock provider
 *
 * Copyright (C) 2026 Difan Zhang
 *
 * The PMU block holds the PLLs, clock dividers, gates and muxes, the
 * peripheral reset register and the pad control registers. This driver
 * maps it and provides the locked register accessors used by the other
 * PMU consumers.
 */

#define pr_fmt(fmt) "fh8626-clk: " fmt

#include <linux/bitops.h>
#include <linux/clk.h>
#include <linux/clk-provider.h>
#include <linux/clkdev.h>
#include <linux/delay.h>
#include <linux/export.h>
#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/mutex.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/reset-controller.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/soc/fullhan/fh8626-pmu.h>
#include <asm/div64.h>
#include <dt-bindings/clock/fh8626v100-clk.h>
#include <dt-bindings/reset/fh8626v100-reset.h>

#define FH_PMU_REG_SWRST	0x54
#define FH_PMU_SWRST_GMAC_BIT	10
#define FH_PMU_SWRST_SDC0_BIT	12
#define FH_PMU_REG_ARC_RST	0x1c4
#define FH_PMU_ARC_RST_BIT	18

static DEFINE_SPINLOCK(fh_clk_lock);
static DEFINE_MUTEX(fh_reset_lock);
static void __iomem *fh_pmu_base;
static resource_size_t fh_pmu_size;

static int fh_pmu_offset(unsigned int reg)
{
	if (!fh_pmu_base)
		return -EPROBE_DEFER;
	if ((reg & 3) || fh_pmu_size < 4 || reg > fh_pmu_size - 4)
		return -EINVAL;
	return 0;
}

int fh8626_pmu_read(unsigned int reg, u32 *value)
{
	unsigned long flags;
	int ret = fh_pmu_offset(reg);

	if (ret)
		return ret;
	if (!value)
		return -EINVAL;
	spin_lock_irqsave(&fh_clk_lock, flags);
	*value = readl(fh_pmu_base + reg);
	spin_unlock_irqrestore(&fh_clk_lock, flags);
	return 0;
}
EXPORT_SYMBOL_GPL(fh8626_pmu_read);

int fh8626_pmu_write(unsigned int reg, u32 value)
{
	unsigned long flags;
	int ret = fh_pmu_offset(reg);

	if (ret)
		return ret;
	spin_lock_irqsave(&fh_clk_lock, flags);
	writel(value, fh_pmu_base + reg);
	spin_unlock_irqrestore(&fh_clk_lock, flags);
	return 0;
}
EXPORT_SYMBOL_GPL(fh8626_pmu_write);

int fh8626_pmu_update_bits(unsigned int reg, u32 mask, u32 value)
{
	unsigned long flags;
	u32 old;
	int ret = fh_pmu_offset(reg);

	if (ret)
		return ret;
	spin_lock_irqsave(&fh_clk_lock, flags);
	old = readl(fh_pmu_base + reg);
	writel((old & ~mask) | (value & mask), fh_pmu_base + reg);
	spin_unlock_irqrestore(&fh_clk_lock, flags);
	return 0;
}
EXPORT_SYMBOL_GPL(fh8626_pmu_update_bits);

/* Sleepable API; serialize the complete GMAC/SDC0 reset transaction. */
int fh8626_pmu_reset(unsigned int bit)
{
	u32 value;
	int ret, count;

	if (bit != FH_PMU_SWRST_GMAC_BIT && bit != FH_PMU_SWRST_SDC0_BIT)
		return -EINVAL;
	mutex_lock(&fh_reset_lock);
	ret = fh8626_pmu_write(FH_PMU_REG_SWRST, (u32)~BIT(bit));
	if (ret)
		goto out;
	for (count = 0; count < 10000; count++) {
		ret = fh8626_pmu_read(FH_PMU_REG_SWRST, &value);
		/*
		 * The reset is complete once the whole command word reads
		 * idle.
		 */
		if (ret || value == ~0U)
			goto out;
		udelay(1);
	}
	ret = -ETIMEDOUT;
out:
	mutex_unlock(&fh_reset_lock);
	return ret;
}
EXPORT_SYMBOL_GPL(fh8626_pmu_reset);

/* Level resets, active low */
static const struct {
	u16 reg;
	u8 bit;
} fh_resets[] = {
	[FH8626_RST_ARC] = { FH_PMU_REG_ARC_RST, FH_PMU_ARC_RST_BIT },
};

static int fh_reset_update(unsigned long id, bool assert)
{
	u32 mask = BIT(fh_resets[id].bit);

	return fh8626_pmu_update_bits(fh_resets[id].reg, mask,
				      assert ? 0 : mask);
}

static int fh_reset_assert(struct reset_controller_dev *rcdev,
			   unsigned long id)
{
	return fh_reset_update(id, true);
}

static int fh_reset_deassert(struct reset_controller_dev *rcdev,
			     unsigned long id)
{
	return fh_reset_update(id, false);
}

static int fh_reset_status(struct reset_controller_dev *rcdev,
			   unsigned long id)
{
	u32 value;
	int ret;

	ret = fh8626_pmu_read(fh_resets[id].reg, &value);
	if (ret)
		return ret;
	return !(value & BIT(fh_resets[id].bit));
}

static const struct reset_control_ops fh_reset_ops = {
	.assert = fh_reset_assert,
	.deassert = fh_reset_deassert,
	.status = fh_reset_status,
};

static struct reset_controller_dev fh_reset_dev = {
	.ops = &fh_reset_ops,
	.owner = THIS_MODULE,
	.nr_resets = ARRAY_SIZE(fh_resets),
};

struct clk_fh_pll {
	struct clk_hw	hw;
	void __iomem	*reg;
	spinlock_t	*lock;
};

static unsigned long clk_fh_pll_recalc_rate(struct clk_hw *hw,
					    unsigned long parent_rate)
{
	struct clk_fh_pll *pll = container_of(hw, struct clk_fh_pll, hw);
	u32 val = readl_relaxed(pll->reg);
	u32 mult = val & 0xff;
	u32 div = (val >> 8) & 0x0f;
	u32 post_shift = (val >> 16) & 0x03;
	u64 rate;

	if (!div)
		return 0;

	rate = (u64)parent_rate * mult;
	do_div(rate, div);
	return (unsigned long)(rate >> post_shift);
}

static long clk_fh_pll_round_rate(struct clk_hw *hw, unsigned long rate,
				  unsigned long *parent_rate)
{
	return clk_fh_pll_recalc_rate(hw, *parent_rate);
}

static const struct clk_ops clk_fh_pll_ops = {
	.recalc_rate = clk_fh_pll_recalc_rate,
	.round_rate  = clk_fh_pll_round_rate,
};

static struct clk *fh_register_pll(const char *name, const char *parent_name,
				   void __iomem *reg, spinlock_t *lock)
{
	struct clk_fh_pll *pll;
	struct clk_init_data init;

	pll = kzalloc(sizeof(*pll), GFP_KERNEL);
	if (!pll)
		return ERR_PTR(-ENOMEM);

	init.name = name;
	init.ops = &clk_fh_pll_ops;
	init.flags = CLK_IGNORE_UNUSED;
	init.parent_names = parent_name ? &parent_name : NULL;
	init.num_parents = parent_name ? 1 : 0;

	pll->reg = reg;
	pll->lock = lock;
	pll->hw.init = &init;

	return clk_register(NULL, &pll->hw);
}

struct clk_fh_divider {
	struct clk_hw	hw;
	void __iomem	*reg;
	u8		shift;
	u8		width;
	u32		k;
	bool		strict_range;
	spinlock_t	*lock;
};

static unsigned long clk_fh_divider_recalc_rate(struct clk_hw *hw,
						unsigned long parent_rate)
{
	struct clk_fh_divider *div =
		container_of(hw, struct clk_fh_divider, hw);
	u32 val, field;
	unsigned long rate;

	if (!div->reg)
		return div->k ? (parent_rate / div->k) : parent_rate;

	val = readl_relaxed(div->reg);
	field = (val >> div->shift) & ((1 << div->width) - 1);
	rate = parent_rate / (field + 1);
	if (div->k > 1)
		rate /= div->k;

	return rate;
}

static long clk_fh_divider_round_rate(struct clk_hw *hw, unsigned long rate,
				      unsigned long *parent_rate)
{
	struct clk_fh_divider *div =
		container_of(hw, struct clk_fh_divider, hw);
	unsigned int val;
	unsigned long target;

	if (!rate || !*parent_rate)
		return -EINVAL;

	target = *parent_rate;
	if (div->k > 1)
		target /= div->k;

	if (div->strict_range &&
	    (rate < target / (1U << div->width) || rate > target))
		return -EINVAL;

	val = DIV_ROUND_CLOSEST(target, rate);
	if (val < 1)
		val = 1;
	if (div->reg && val > (1 << div->width))
		val = 1 << div->width;

	return (target / val);
}

static int clk_fh_divider_set_rate(struct clk_hw *hw, unsigned long rate,
				   unsigned long parent_rate)
{
	struct clk_fh_divider *div =
		container_of(hw, struct clk_fh_divider, hw);
	unsigned int val;
	unsigned long flags = 0;
	u32 reg_val;

	if (!div->reg || !rate || !parent_rate)
		return -EINVAL;

	if (div->k > 1)
		parent_rate /= div->k;

	if (div->strict_range &&
	    (rate < parent_rate / (1U << div->width) || rate > parent_rate))
		return -EINVAL;

	val = DIV_ROUND_CLOSEST(parent_rate, rate);
	if (val < 1)
		val = 1;
	if (val > (1 << div->width))
		val = 1 << div->width;
	val -= 1;

	if (div->lock)
		spin_lock_irqsave(div->lock, flags);

	reg_val = readl_relaxed(div->reg);
	reg_val &= ~(((1 << div->width) - 1) << div->shift);
	reg_val |= (val << div->shift);
	writel_relaxed(reg_val, div->reg);

	if (div->lock)
		spin_unlock_irqrestore(div->lock, flags);

	return 0;
}

static const struct clk_ops clk_fh_divider_ops = {
	.recalc_rate = clk_fh_divider_recalc_rate,
	.round_rate  = clk_fh_divider_round_rate,
	.set_rate    = clk_fh_divider_set_rate,
};

enum fh_clk_support {
	FH_CLK_UNKNOWN,
	FH_CLK_RATE,
	FH_CLK_GATE,
};

struct fh_clk_desc {
	const char	*name;
	unsigned long	fixed_rate;
	u32		flags;
	const char	*parent_names[4];
	int		num_parents;
	u32		div_k;
	u32		div_reg;
	u32		div_mask;
	u32		gate_reg;
	u32		gate_mask;
	u32		mux_reg;
	u32		mux_mask;
	enum fh_clk_support support;
};

static const struct fh_clk_desc fh_clk_table[FH8626_NR_CLKS] = {
	/*  0 */
	{ "osc_clk", 24000000UL, 0x1, {  }, 0,
	  0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, FH_CLK_RATE },
	/*  1 */
	{ "cis_pix_clk", 75000000UL, 0x1, {  }, 0,
	  0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, FH_CLK_RATE },
	/*  2 */
	{ "acodec_pll_clk", 98304000UL, 0x1, {  }, 0,
	  0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, FH_CLK_RATE },
	/*  3 */
	{ "pll0_clk", 0UL, 0x20, { "osc_clk" }, 1,
	  0, 0x10, 0x0, 0x0, 0x0, 0x0, 0x0, FH_CLK_RATE },
	/*  4 */
	{ "pll1_clk", 0UL, 0x20, { "osc_clk" }, 1,
	  0, 0x14, 0x0, 0x0, 0x0, 0x0, 0x0, FH_CLK_RATE },
	/*  5 */
	{ "arm_clk", 0UL, 0x12,
	  { "osc_clk", "pll1_clk", "osc_clk", "pll0_clk" }, 4,
	  1, 0x2c, 0xf, 0x0, 0x0, 0xc, 0x3, FH_CLK_RATE },
	/*  6 */
	{ "arc_clk", 0UL, 0x10, { "osc_clk", "pll0_clk" }, 2,
	  1, 0x2c, 0xf0, 0x1c, 0x4, 0xc, 0x1, FH_CLK_RATE },
	/*  7 */
	{ "ahb_clk", 0UL, 0x1a, { "osc_clk", "pll0_clk" }, 2,
	  2, 0x2c, 0xf0, 0x0, 0x0, 0xc, 0x1, FH_CLK_RATE },
	/*  8 */
	{ "pll1_clk_div_2", 0UL, 0x6, { "pll1_clk" }, 1,
	  2, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, FH_CLK_RATE },
	/*  9 */
	{ "ddr_clk", 0UL, 0x14, { "pll0_clk", "pll1_clk_div_2" }, 2,
	  1, 0x0, 0x0, 0x1c, 0x40, 0x28, 0x1, FH_CLK_RATE },
	/* 10 */
	{ "isp_aclk", 0UL, 0x10, { "pll0_clk", "pll1_clk" }, 2,
	  1, 0x3c, 0x1f00, 0x1c, 0x1, 0x28, 0x10, FH_CLK_RATE },
	/* 11 */
	{ "isp_hclk", 0UL, 0xc, {  }, 0,
	  1, 0x0, 0x0, 0x1c, 0x2, 0x0, 0x0, FH_CLK_GATE },
	/* 12 */
	{ "pae_clk", 0UL, 0x0, { "pll0_clk" }, 1,
	  1, 0x3c, 0x7000000, 0x24, 0x400000, 0x0, 0x0, FH_CLK_RATE },
	/*
	 * BGM shares the ISP mux/divider: register it without changing
	 * the rate.
	 */
	/* 13 */
	{ "bgm_clk", 0UL, 0x10, { "pll0_clk", "pll1_clk" }, 2,
	  1, 0x3c, 0x1f00, 0x24, 0x100000, 0x28, 0x10, FH_CLK_RATE },
	/* 14 */
	{ "bgm_hclk", 0UL, 0xc, {  }, 0,
	  1, 0x0, 0x0, 0x24, 0x200000, 0x0, 0x0, FH_CLK_GATE },
	/* JPEG also shares the ISP/BGM mux and divider. */
	/* 15 */
	{ "jpeg_clk", 0UL, 0x10, { "pll0_clk", "pll1_clk" }, 2,
	  1, 0x3c, 0x1f00, 0x24, 0x40000, 0x28, 0x10, FH_CLK_RATE },
	/* 16 */
	{ "jpeg_hclk", 0UL, 0xc, {  }, 0,
	  1, 0x0, 0x0, 0x24, 0x80000, 0x0, 0x0, FH_CLK_GATE },
	/*
	 * Keep the boot-selected CIS parent/rate until a consumer changes
	 * it.
	 */
	/* 17 */
	{ "cis_clk_div", 0UL, 0x202, { "pll0_clk" }, 1,
	  1, 0x30, 0x1f000000, 0x0, 0x0, 0x0, 0x0, FH_CLK_RATE },
	/* 18 */
	{ "cis_clk_out", 0UL, 0x14, { "osc_clk", "pll0_clk" }, 2,
	  1, 0x0, 0x0, 0x1c, 0x800000, 0x28, 0x8, FH_CLK_RATE },
	/* 19 */
	{ "cis_pix_clk_rt", 75000000UL, 0x1, {  }, 0,
	  0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, FH_CLK_RATE },
	/*
	 * These two muxes share the MIPI pixel route with the PHY driver,
	 * so neither is registered.
	 */
	/* 20 */
	{ "mipi_pix_clk", 0UL, 0x1a, { "pll0_clk", "pll1_clk" }, 2,
	  1, 0x3c, 0x1f0000, 0x0, 0x0, 0x28, 0x20 },
	/* 21 */
	{ "pix_clk", 0UL, 0x1c,
	  { "cis_pix_clk", "cis_pix_clk_rt", "mipi_pix_clk", "mipi_pix_clk" },
	  4, 1, 0x0, 0x0, 0x24, 0x20000, 0x28, 0x60 },
	/* 22 */
	{ "dw_100m_clk", 0UL, 0xe, { "pll1_clk" }, 1,
	  6, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, FH_CLK_RATE },
	/* 23 */
	{ "dw_50m_clk", 0UL, 0xe, { "dw_100m_clk" }, 1,
	  2, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, FH_CLK_RATE },
	/* 24 */
	{ "pts_clk", 0UL, 0x0, { "dw_100m_clk" }, 1,
	  1, 0x38, 0x1ff0000, 0x1c, 0x80000, 0x0, 0x0 },
	/* 25 */
	{ "spi0_clk", 0UL, 0x0, { "dw_100m_clk" }, 1,
	  1, 0x2c, 0xf00, 0x1c, 0x80, 0x0, 0x0, FH_CLK_RATE },
	/* 26 */
	{ "spi1_clk", 0UL, 0x0, { "dw_100m_clk" }, 1,
	  1, 0x2c, 0xf000, 0x1c, 0x100, 0x0, 0x0, FH_CLK_RATE },
	/* 27 */
	{ "sdc0_clk", 0UL, 0x0, { "dw_100m_clk" }, 1,
	  2, 0x2c, 0xf0000, 0x1c, 0x200, 0x0, 0x0, FH_CLK_RATE },
	/* 28 */
	{ "sdc1_clk", 0UL, 0x0, { "dw_100m_clk" }, 1,
	  2, 0x2c, 0xf00000, 0x1c, 0x400, 0x0, 0x0 },
	/* 29 */
	{ "uart0_clk", 0UL, 0x0, { "dw_100m_clk" }, 1,
	  1, 0x30, 0x1f, 0x1c, 0x2000, 0x0, 0x0, FH_CLK_RATE },
	/* 30 */
	{ "uart1_clk", 0UL, 0x0, { "dw_100m_clk" }, 1,
	  1, 0x30, 0x1f00, 0x1c, 0x4000, 0x0, 0x0, FH_CLK_RATE },
	/* 31 */
	{ "uart2_clk", 0UL, 0x0, { "dw_100m_clk" }, 1,
	  1, 0x30, 0x1f0000, 0x1c, 0x8000, 0x0, 0x0, FH_CLK_RATE },
	/* 32 */
	{ "i2c0_clk", 0UL, 0x0, { "dw_100m_clk" }, 1,
	  1, 0x34, 0x3f, 0x1c, 0x1000, 0x0, 0x0, FH_CLK_RATE },
	/* 33 */
	{ "i2c1_clk", 0UL, 0x0, { "dw_100m_clk" }, 1,
	  1, 0x34, 0x3f00, 0x1c, 0x8000000, 0x0, 0x0, FH_CLK_RATE },
	/* 34 */
	{ "i2c2_clk", 0UL, 0x0, { "dw_100m_clk" }, 1,
	  1, 0x34, 0x3f0000, 0x20, 0x200, 0x0, 0x0, FH_CLK_RATE },
	/* 35 */
	{ "pwm_clk", 0UL, 0x0, { "dw_100m_clk" }, 1,
	  1, 0x34, 0xff000000, 0x1c, 0x10000, 0x0, 0x0, FH_CLK_RATE },
	/* 36 */
	{ "wdt_clk", 0UL, 0x0, { "ahb_clk" }, 1,
	  1, 0x38, 0xff00, 0x20, 0x4, 0x0, 0x0, FH_CLK_RATE },
	/* 37 */
	{ "usb_clk", 0UL, 0x14, { "osc_clk", "dw_50m_clk" }, 2,
	  1, 0x0, 0x0, 0x24, 0x2000, 0x28, 0x8000, FH_CLK_RATE },
	/* 38 */
	{ "tmr0_clk", 0UL, 0x0, { "dw_100m_clk" }, 1,
	  1, 0x38, 0xff, 0x1c, 0x20000, 0x0, 0x0, FH_CLK_RATE },
	/* 39 */
	{ "gpio0_dbclk", 0UL, 0x0, { "dw_100m_clk" }, 1,
	  100, 0x40, 0x7fff, 0x20, 0x800, 0x0, 0x0 },
	/* 40 */
	{ "gpio1_dbclk", 0UL, 0x0, { "dw_100m_clk" }, 1,
	  100, 0x40, 0x7fff0000, 0x20, 0x2000, 0x0, 0x0 },
	/* 41 */
	{ "ac_clk", 0UL, 0x10, { "osc_clk", "acodec_pll_clk" }, 2,
	  1, 0x48, 0x3f0, 0x48, 0x100000, 0x48, 0x1, FH_CLK_RATE },
	/* 42 */
	{ "i2s_clk", 0UL, 0x0, { "ac_clk" }, 1,
	  1, 0x48, 0x3f000, 0x48, 0x1000000, 0x0, 0x0, FH_CLK_RATE },
	/* 43 */
	{ "sadc_clk", 0UL, 0x0, { "dw_100m_clk" }, 1,
	  1, 0x3c, 0x7f, 0x1c, 0x4000000, 0x0, 0x0, FH_CLK_RATE },
	/* 44 */
	{ "eth_clk", 0UL, 0x0, { "dw_100m_clk" }, 1,
	  1, 0x2c, 0xf000000, 0x1c, 0x2000000, 0x0, 0x0, FH_CLK_RATE },
	/* 45 */
	{ "eth_rmii_clk", 0UL, 0xc, {  }, 0,
	  1, 0x0, 0x0, 0x1c, 0x10000000, 0x0, 0x0, FH_CLK_GATE },
	/* 46 */
	{ "efuse_clk", 0UL, 0x0, { "dw_100m_clk" }, 1,
	  1, 0x2c, 0xf0000000, 0x1c, 0x200000, 0x0, 0x0, FH_CLK_RATE },
	/* 47 */
	{ "pae_hclk_gate", 0UL, 0xc, {  }, 0,
	  0, 0x0, 0x0, 0x24, 0x800000, 0x0, 0x0, FH_CLK_GATE },
	/* 48 */
	{ "multi_pae_clk", 0UL, 0xc, {  }, 0,
	  0, 0x0, 0x0, 0x24, 0x200, 0x0, 0x0, FH_CLK_GATE },
	/* 49 */
	{ "pae_adpt_clk", 0UL, 0xc, {  }, 0,
	  0, 0x0, 0x0, 0x24, 0x1000000, 0x0, 0x0, FH_CLK_GATE },
	/* 50 */
	{ "pll0_cis_clk", 0UL, 0xc, {  }, 0,
	  0, 0x0, 0x0, 0x24, 0x10, 0x0, 0x0, FH_CLK_GATE },
	/* 51 */
	{ "emc_hclk_gate", 0UL, 0xc, {  }, 0,
	  0, 0x0, 0x0, 0x20, 0x1, 0x0, 0x0, FH_CLK_GATE },
	/* 52 */
	{ "rtc_pclk_gate", 0UL, 0xc, {  }, 0,
	  0, 0x0, 0x0, 0x20, 0x2, 0x0, 0x0 },
	/* 53 */
	{ "aes_hclk_gate", 0UL, 0xc, {  }, 0,
	  0, 0x0, 0x0, 0x20, 0x20, 0x0, 0x0, FH_CLK_GATE },
	/* 54 */
	{ "dmac0_hclk_gate", 0UL, 0xc, {  }, 0,
	  0, 0x0, 0x0, 0x20, 0x40, 0x0, 0x0, FH_CLK_GATE },
	/* 55 */
	{ "dmac1_hclk_gate", 0UL, 0xc, {  }, 0,
	  0, 0x0, 0x0, 0x20, 0x80, 0x0, 0x0 },
	/* 56 */
	{ "intc_hclk_gate", 0UL, 0xc, {  }, 0,
	  0, 0x0, 0x0, 0x20, 0x100, 0x0, 0x0 },
	/* 57 */
	{ "ddrc_hclk_gate", 0UL, 0xc, {  }, 0,
	  0, 0x0, 0x0, 0x20, 0x8000, 0x0, 0x0 },
	/* 58 */
	{ "mipi_dphy_clk", 0UL, 0xc, {  }, 0,
	  0, 0x0, 0x0, 0x24, 0x400, 0x0, 0x0, FH_CLK_GATE },
	/* 59 */
	{ "mipi_wrap_pclk", 0UL, 0xc, {  }, 0,
	  0, 0x0, 0x0, 0x24, 0x800, 0x0, 0x0, FH_CLK_GATE },
	/* 60 */
	{ "mipic_pclk", 0UL, 0xc, {  }, 0,
	  0, 0x0, 0x0, 0x24, 0x1000, 0x0, 0x0, FH_CLK_GATE },
	/* 61 */
	{ "usb_hclk", 0UL, 0xc, {  }, 0,
	  0, 0x0, 0x0, 0x24, 0x4000, 0x0, 0x0, FH_CLK_GATE },
	/* 62 */
	{ "acodec_mclk", 0UL, 0xc, {  }, 0,
	  0, 0x0, 0x0, 0x24, 0x8000, 0x0, 0x0, FH_CLK_GATE },
	/* 63 */
	{ "acodec_pclk", 0UL, 0xc, {  }, 0,
	  0, 0x0, 0x0, 0x24, 0x10000, 0x0, 0x0, FH_CLK_GATE },
	/* 64 */
	{ "sdc0_clk_sample", 0UL, 0x402, { "sdc0_clk" }, 1,
	  1, 0x0, 0x0, 0x0, 0x0, 0x28, 0x600 },
	/* 65 */
	{ "sdc0_clk_drv", 0UL, 0x402, { "sdc0_clk" }, 1,
	  1, 0x0, 0x0, 0x0, 0x0, 0x28, 0x180 },
	/* 66 */
	{ "sdc1_clk_sample", 0UL, 0x402, { "sdc0_clk" }, 1,
	  1, 0x0, 0x0, 0x0, 0x0, 0x28, 0x6000 },
	/* 67 */
	{ "sdc1_clk_drv", 0UL, 0x402, { "sdc0_clk" }, 1,
	  1, 0x0, 0x0, 0x0, 0x0, 0x28, 0x1800 },
};

static struct clk *clks[FH8626_NR_CLKS];
static struct clk_onecell_data clk_data;

static void __init fh8626_clk_init(struct device_node *np)
{
	struct device_node *pmu;
	void __iomem *base;
	struct resource res;
	struct clk *osc;
	int i, ret;

	osc = of_clk_get_by_name(np, "osc");
	if (IS_ERR(osc)) {
		pr_err("failed to get oscillator clock: %ld\n", PTR_ERR(osc));
		return;
	}
	pmu = of_get_parent(np);
	if (!pmu || !of_device_is_compatible(pmu, "fullhan,fh8626v100-pmu")) {
		pr_err("clock controller must be a child of the PMU\n");
		of_node_put(pmu);
		goto put_osc;
	}
	if (of_address_to_resource(pmu, 0, &res)) {
		of_node_put(pmu);
		goto put_osc;
	}
	/* Map the parent window early for all shared PMU register users. */
	base = of_iomap(pmu, 0);
	of_node_put(pmu);
	if (!base) {
		pr_err("failed to map PMU registers\n");
		goto put_osc;
	}
	fh_pmu_size = resource_size(&res);
	fh_pmu_base = base;

	for (i = 0; i < FH8626_NR_CLKS; i++) {
		const struct fh_clk_desc *desc = &fh_clk_table[i];
		struct clk *clk = ERR_PTR(-EINVAL);

		clks[i] = ERR_PTR(-ENOENT);

		if (i == FH8626_CLK_OSC) {
			clks[i] = osc;
			continue;
		}
		if (desc->support == FH_CLK_UNKNOWN) {
			continue;
		} else if (desc->support == FH_CLK_GATE) {
			clk = clk_register_gate(NULL, desc->name, NULL,
						CLK_IGNORE_UNUSED,
						base + desc->gate_reg,
						__ffs(desc->gate_mask),
						CLK_GATE_SET_TO_DISABLE,
						&fh_clk_lock);
		} else if (desc->flags & 0x20) {
			const char *pname = desc->num_parents ?
					    desc->parent_names[0] : "osc_clk";

			clk = fh_register_pll(desc->name, pname,
					      base + desc->div_reg,
					      &fh_clk_lock);
		} else if (desc->flags & 0x1) {
			clk = clk_register_fixed_rate(NULL, desc->name, NULL, 0,
						      desc->fixed_rate);
		} else if (desc->div_reg == 0 && desc->div_k > 1 &&
			   desc->gate_mask == 0 && desc->mux_mask == 0) {
			const char *pname = desc->num_parents ?
					    desc->parent_names[0] : NULL;

			clk = clk_register_fixed_factor(NULL, desc->name,
							pname, 0, 1,
							desc->div_k);
		} else {
			struct clk_mux *mux = NULL;
			struct clk_fh_divider *div = NULL;
			struct clk_gate *gate = NULL;

			if ((desc->flags & 0x10) && desc->mux_mask) {
				mux = kzalloc(sizeof(*mux), GFP_KERNEL);
				if (!mux) {
					clk = ERR_PTR(-ENOMEM);
					goto free_components;
				}
				mux->reg = base + desc->mux_reg;
				mux->shift = __ffs(desc->mux_mask);
				mux->mask = desc->mux_mask >> mux->shift;
				mux->lock = &fh_clk_lock;
			}

			if (desc->div_reg != 0 || desc->div_k > 1) {
				div = kzalloc(sizeof(*div), GFP_KERNEL);
				if (!div) {
					clk = ERR_PTR(-ENOMEM);
					goto free_components;
				}
				if (desc->div_reg != 0) {
					div->reg = base + desc->div_reg;
					div->shift = __ffs(desc->div_mask);
					div->width = fls(desc->div_mask >>
							 div->shift);
				}
				div->k = desc->div_k ? desc->div_k : 1;
				div->strict_range = i == FH8626_CLK_UART0 ||
						    i == FH8626_CLK_UART1 ||
						    i == FH8626_CLK_UART2;
				div->lock = &fh_clk_lock;
			}

			if (desc->gate_mask != 0) {
				gate = kzalloc(sizeof(*gate), GFP_KERNEL);
				if (!gate) {
					clk = ERR_PTR(-ENOMEM);
					goto free_components;
				}
				gate->reg = base + desc->gate_reg;
				gate->bit_idx = __ffs(desc->gate_mask);
				gate->flags = CLK_GATE_SET_TO_DISABLE;
				gate->lock = &fh_clk_lock;
			}

			clk = clk_register_composite(NULL, desc->name,
						     desc->num_parents ?
						     desc->parent_names : NULL,
						     desc->num_parents,
						     mux ? &mux->hw : NULL,
						     mux ? &clk_mux_ops : NULL,
						     div ? &div->hw : NULL,
						     div ? &clk_fh_divider_ops :
						     NULL,
						     gate ? &gate->hw : NULL,
						     gate ? &clk_gate_ops :
						     NULL,
						     CLK_IGNORE_UNUSED);
			if (IS_ERR(clk)) {
free_components:
				kfree(gate);
				kfree(div);
				kfree(mux);
			}
		}

		if (IS_ERR(clk)) {
			clks[i] = clk;
			if (PTR_ERR(clk) != -ENOMEM)
				pr_warn("failed to register clock %s: %ld\n",
					desc->name, PTR_ERR(clk));
			continue;
		}

		clks[i] = clk;
		clk_register_clkdev(clk, desc->name, NULL);
	}

	clk_data.clks = clks;
	clk_data.clk_num = FH8626_NR_CLKS;
	ret = of_clk_add_provider(np, of_clk_src_onecell_get, &clk_data);
	if (ret)
		pr_err("failed to register clock provider: %d\n", ret);

	fh_reset_dev.of_node = np;
	ret = reset_controller_register(&fh_reset_dev);
	if (ret)
		pr_err("failed to register reset controller: %d\n", ret);
	return;

put_osc:
	clk_put(osc);
}

CLK_OF_DECLARE(fh8626v100_clk, "fullhan,fh8626v100-clk", fh8626_clk_init);
CLK_OF_DECLARE(fh8626_clk, "fullhan,fh8626-clk", fh8626_clk_init);
