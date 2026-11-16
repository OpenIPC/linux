// SPDX-License-Identifier: GPL-2.0
/*
 * Ingenic T23 SoC CGU driver
 *
 * Copyright (C) 2019 Ingenic Semiconductor Co., Ltd.
 * Copyright (C) 2026 Difan Zhang
 */

#include <linux/clk-provider.h>
#include <linux/delay.h>
#include <linux/io.h>
#include <linux/math64.h>
#include <linux/of.h>

#include <dt-bindings/clock/ingenic,t23-cgu.h>

#include "cgu.h"
#include "pm.h"

/* CGU CPM register offsets */
#define CPM_CPCCR		0x00
#define CPM_CPAPCR		0x10
#define CPM_CPMPCR		0x14
#define CPM_CLKGR		0x20
#define CPM_CLKGR1		0x28
#define CPM_DDRCDR		0x2c
#define CPM_VPUCDR		0x30
#define CPM_RSACDR		0x4c
#define CPM_MACCDR		0x54
#define CPM_SSICDR		0x60
#define CPM_LPCDR		0x64
#define CPM_MSC0CDR		0x68
#define CPM_I2SSPKCDR		0x70
#define CPM_SFCCDR		0x74
#define CPM_CIMCDR		0x7c
#define CPM_ISPCDR		0x80
#define CPM_I2SMICCDR		0x84
#define CPM_MSC1CDR		0xa4
#define CPM_CPCSR		0xd4

static struct ingenic_cgu *cgu;

static u32 t23_frac_to_value(u32 frac)
{
	u64 t = (u64)(frac & 0xffffff) * 125;

	return (u32)((t + 0x100000) >> 21);
}

static unsigned long
t23_pll_recalc_rate(struct clk_hw *hw, unsigned long parent_rate)
{
	struct ingenic_clk *ingenic_clk = to_ingenic_clk(hw);
	struct ingenic_cgu *cgu_ptr = ingenic_clk->cgu;
	unsigned int reg = ingenic_clk->idx == CLK_APLL ?
			   CPM_CPAPCR : CPM_CPMPCR;
	u32 cpxpcr = readl(cgu_ptr->base + reg);
	unsigned long m, n, od1, od0, frac, frac_value;
	unsigned long rate;

	if (!(cpxpcr & BIT(0)))
		return 0;

	m = (cpxpcr >> 20) & 0xfff;
	n = (cpxpcr >> 14) & 0x3f;
	od1 = (cpxpcr >> 11) & 0x07;
	od0 = (cpxpcr >> 8) & 0x07;

	if (!parent_rate || !n || !od1 || !od0)
		return 0;

	frac = readl(cgu_ptr->base + reg + 8);
	frac_value = t23_frac_to_value(frac);

	rate = ((parent_rate / 4000) * m / n / od1 / od0) * 4000
		+ ((parent_rate / 4000) * frac_value / n / od1 / od0) * 4;

	return rate;
}

static int t23_pll_is_enabled(struct clk_hw *hw)
{
	struct ingenic_clk *ingenic_clk = to_ingenic_clk(hw);
	struct ingenic_cgu *cgu_ptr = ingenic_clk->cgu;
	unsigned int reg = ingenic_clk->idx == CLK_APLL ?
			   CPM_CPAPCR : CPM_CPMPCR;

	return !!(readl(cgu_ptr->base + reg) & BIT(0));
}

static int t23_pll_enable(struct clk_hw *hw)
{
	return 0;
}

static void t23_pll_disable(struct clk_hw *hw)
{
}

static const struct clk_ops t23_pll_ops = {
	.recalc_rate = t23_pll_recalc_rate,
	.is_enabled = t23_pll_is_enabled,
	.enable = t23_pll_enable,
	.disable = t23_pll_disable,
};

static unsigned long t23_i2s_recalc_rate(struct clk_hw *hw,
					 unsigned long parent_rate)
{
	struct ingenic_clk *ingenic_clk = to_ingenic_clk(hw);
	struct ingenic_cgu *cgu_ptr = ingenic_clk->cgu;
	unsigned int reg = (ingenic_clk->idx == CLK_CGU_I2S_SPK) ?
			   CPM_I2SSPKCDR : CPM_I2SMICCDR;
	u32 val = readl(cgu_ptr->base + reg);
	u32 m = (val >> 20) & 0x1ff;
	u32 n = val & 0xfffff;

	if (!n || !parent_rate)
		return 0;

	return div_u64((u64)parent_rate * m, n);
}

static u8 t23_i2s_get_parent(struct clk_hw *hw)
{
	struct ingenic_clk *ingenic_clk = to_ingenic_clk(hw);
	struct ingenic_cgu *cgu_ptr = ingenic_clk->cgu;
	unsigned int reg = (ingenic_clk->idx == CLK_CGU_I2S_SPK) ?
			   CPM_I2SSPKCDR : CPM_I2SMICCDR;
	u32 val = readl(cgu_ptr->base + reg);

	return (val >> 30) & 1;
}

static int t23_i2s_set_parent(struct clk_hw *hw, u8 index)
{
	struct ingenic_clk *ingenic_clk = to_ingenic_clk(hw);
	struct ingenic_cgu *cgu_ptr = ingenic_clk->cgu;
	unsigned int reg = (ingenic_clk->idx == CLK_CGU_I2S_SPK) ?
			   CPM_I2SSPKCDR : CPM_I2SMICCDR;
	unsigned long flags;
	u32 val;

	if (index > 1)
		return -EINVAL;

	spin_lock_irqsave(&cgu_ptr->lock, flags);
	val = readl(cgu_ptr->base + reg) & ~GENMASK(31, 30);
	val |= (u32)index << 30;
	writel(val, cgu_ptr->base + reg);
	spin_unlock_irqrestore(&cgu_ptr->lock, flags);

	return 0;
}

static int t23_i2s_enable(struct clk_hw *hw)
{
	struct ingenic_clk *ingenic_clk = to_ingenic_clk(hw);
	struct ingenic_cgu *cgu_ptr = ingenic_clk->cgu;
	unsigned int reg = (ingenic_clk->idx == CLK_CGU_I2S_SPK) ?
			   CPM_I2SSPKCDR : CPM_I2SMICCDR;
	unsigned long flags;
	u32 val;

	spin_lock_irqsave(&cgu_ptr->lock, flags);
	val = readl(cgu_ptr->base + reg);
	val |= BIT(29);
	writel(val, cgu_ptr->base + reg);
	spin_unlock_irqrestore(&cgu_ptr->lock, flags);

	return 0;
}

static void t23_i2s_disable(struct clk_hw *hw)
{
	struct ingenic_clk *ingenic_clk = to_ingenic_clk(hw);
	struct ingenic_cgu *cgu_ptr = ingenic_clk->cgu;
	unsigned int reg = (ingenic_clk->idx == CLK_CGU_I2S_SPK) ?
			   CPM_I2SSPKCDR : CPM_I2SMICCDR;
	unsigned long flags;
	u32 val;

	spin_lock_irqsave(&cgu_ptr->lock, flags);
	val = readl(cgu_ptr->base + reg);
	val &= ~BIT(29);
	writel(val, cgu_ptr->base + reg);
	spin_unlock_irqrestore(&cgu_ptr->lock, flags);
}

static int t23_i2s_is_enabled(struct clk_hw *hw)
{
	struct ingenic_clk *ingenic_clk = to_ingenic_clk(hw);
	struct ingenic_cgu *cgu_ptr = ingenic_clk->cgu;
	unsigned int reg = (ingenic_clk->idx == CLK_CGU_I2S_SPK) ?
			   CPM_I2SSPKCDR : CPM_I2SMICCDR;

	return !!(readl(cgu_ptr->base + reg) & BIT(29));
}

static int t23_i2s_determine_rate(struct clk_hw *hw,
				  struct clk_rate_request *req)
{
	if (!req->best_parent_rate || !req->rate)
		req->rate = 0;
	return 0;
}

static int t23_i2s_set_rate(struct clk_hw *hw, unsigned long req_rate,
			    unsigned long parent_rate)
{
	struct ingenic_clk *ingenic_clk = to_ingenic_clk(hw);
	struct ingenic_cgu *cgu_ptr = ingenic_clk->cgu;
	unsigned int reg = (ingenic_clk->idx == CLK_CGU_I2S_SPK) ?
			   CPM_I2SSPKCDR : CPM_I2SMICCDR;
	unsigned long flags;
	u64 m, n, m_mul, tmp_val, diff, min_diff = ~0ULL;
	u32 best_m = 1, best_n = 1;
	u32 val;

	if (!parent_rate || !req_rate)
		return -EINVAL;

	for (m = 1; m <= 0x1ff; m++) {
		m_mul = (u64)parent_rate * m;
		n = div64_u64(m_mul + (req_rate / 2), req_rate);
		if (!n)
			n = 1;
		if (n > 0xfffff)
			continue;
		tmp_val = div64_u64(m_mul, n);
		if (tmp_val == req_rate) {
			best_m = m;
			best_n = n;
			break;
		}
		diff = tmp_val > req_rate ? tmp_val - req_rate :
					    req_rate - tmp_val;
		if (diff < min_diff) {
			min_diff = diff;
			best_m = m;
			best_n = n;
		}
	}

	spin_lock_irqsave(&cgu_ptr->lock, flags);
	val = readl(cgu_ptr->base + reg) & 0xe0000000;
	val |= (best_m << 20) | (best_n & 0xfffff);
	writel(val, cgu_ptr->base + reg);
	spin_unlock_irqrestore(&cgu_ptr->lock, flags);

	return 0;
}

static const struct clk_ops t23_i2s_ops = {
	.get_parent = t23_i2s_get_parent,
	.set_parent = t23_i2s_set_parent,
	.recalc_rate = t23_i2s_recalc_rate,
	.determine_rate = t23_i2s_determine_rate,
	.set_rate = t23_i2s_set_rate,
	.enable = t23_i2s_enable,
	.disable = t23_i2s_disable,
	.is_enabled = t23_i2s_is_enabled,
};

static const struct ingenic_cgu_clk_info t23_cgu_clocks[] = {
	/* External root clocks */
	[CLK_EXT] = { "ext", CGU_CLK_EXT },
	[CLK_RTC] = { "rtc", CGU_CLK_EXT },

	/* PLLs (read-only from bootloader) */
	[CLK_APLL] = {
		"apll", CGU_CLK_CUSTOM,
		.parents = { CLK_EXT, -1, -1, -1 },
		.custom = { &t23_pll_ops },
	},
	[CLK_MPLL] = {
		"mpll", CGU_CLK_CUSTOM,
		.parents = { CLK_EXT, -1, -1, -1 },
		.custom = { &t23_pll_ops },
	},

	/* Core Muxes */
	[CLK_SCLKA] = {
		"sclka", CGU_CLK_MUX,
		.parents = { -1, CLK_EXT, CLK_APLL, -1 },
		.mux = { CPM_CPCCR, 30, 2 },
	},
	[CLK_CPUMUX] = {
		"cpu_mux", CGU_CLK_MUX,
		.parents = { -1, CLK_SCLKA, CLK_MPLL, -1 },
		.mux = { CPM_CPCCR, 28, 2 },
		.flags = CLK_IS_CRITICAL | CLK_IGNORE_UNUSED,
	},
	[CLK_AHB2PMUX] = {
		"ahb2_apb_mux", CGU_CLK_MUX,
		.parents = { -1, CLK_SCLKA, CLK_MPLL, -1 },
		.mux = { CPM_CPCCR, 24, 2 },
		.flags = CLK_IS_CRITICAL | CLK_IGNORE_UNUSED,
	},

	/* Core Dividers (CPCCR) */
	[CLK_CCLK] = {
		"cclk", CGU_CLK_DIV,
		.parents = { CLK_CPUMUX, -1, -1, -1 },
		.div = { CPM_CPCCR, 0, 1, 4, 22, -1, -1 },
		.flags = CLK_IS_CRITICAL | CLK_IGNORE_UNUSED,
	},
	[CLK_L2CLK] = {
		"l2clk", CGU_CLK_DIV,
		.parents = { CLK_CPUMUX, -1, -1, -1 },
		.div = { CPM_CPCCR, 4, 1, 4, 22, -1, -1 },
		.flags = CLK_IS_CRITICAL | CLK_IGNORE_UNUSED,
	},
	[CLK_H0CLK] = {
		"h0clk", CGU_CLK_MUX | CGU_CLK_DIV,
		.parents = { -1, CLK_SCLKA, CLK_MPLL, -1 },
		.mux = { CPM_CPCCR, 26, 2 },
		.div = { CPM_CPCCR, 8, 1, 4, 21, -1, -1 },
		.flags = CLK_IS_CRITICAL | CLK_IGNORE_UNUSED,
	},
	[CLK_H2CLK] = {
		"h2clk", CGU_CLK_DIV,
		.parents = { CLK_AHB2PMUX, -1, -1, -1 },
		.div = { CPM_CPCCR, 12, 1, 4, 20, -1, -1 },
		.flags = CLK_IS_CRITICAL | CLK_IGNORE_UNUSED,
	},
	[CLK_PCLK] = {
		"pclk", CGU_CLK_DIV,
		.parents = { CLK_AHB2PMUX, -1, -1, -1 },
		.div = { CPM_CPCCR, 16, 1, 4, 20, -1, -1 },
		.flags = CLK_IS_CRITICAL | CLK_IGNORE_UNUSED,
	},

	/* Peripheral Gates (CLKGR - Bank 0) */
	[CLK_EFUSE] = {
		"efuse", CGU_CLK_GATE,
		.parents = { CLK_H2CLK, -1, -1, -1 },
		.gate = { CPM_CLKGR, 1 },
	},
	[CLK_OTG] = {
		"otg", CGU_CLK_GATE,
		.parents = { CLK_H2CLK, -1, -1, -1 },
		.gate = { CPM_CLKGR, 3 },
	},
	[CLK_MSC0] = {
		"msc0", CGU_CLK_GATE,
		.parents = { CLK_H2CLK, -1, -1, -1 },
		.gate = { CPM_CLKGR, 4 },
	},
	[CLK_MSC1] = {
		"msc1", CGU_CLK_GATE,
		.parents = { CLK_H2CLK, -1, -1, -1 },
		.gate = { CPM_CLKGR, 5 },
	},
	[CLK_SSI0] = {
		"ssi0", CGU_CLK_GATE,
		.parents = { CLK_CGU_SSI, -1, -1, -1 },
		.gate = { CPM_CLKGR, 6 },
	},
	[CLK_I2C0] = {
		"i2c0", CGU_CLK_GATE,
		.parents = { CLK_PCLK, -1, -1, -1 },
		.gate = { CPM_CLKGR, 7 },
	},
	[CLK_I2C1] = {
		"i2c1", CGU_CLK_GATE,
		.parents = { CLK_PCLK, -1, -1, -1 },
		.gate = { CPM_CLKGR, 8 },
	},
	[CLK_I2C2] = {
		"i2c2", CGU_CLK_GATE,
		.parents = { CLK_PCLK, -1, -1, -1 },
		.gate = { CPM_CLKGR, 9 },
	},
	[CLK_AIC] = {
		"aic", CGU_CLK_GATE,
		.parents = { CLK_PCLK, -1, -1, -1 },
		.gate = { CPM_CLKGR, 11 },
	},
	[CLK_SADC] = {
		"sadc", CGU_CLK_GATE,
		.parents = { CLK_PCLK, -1, -1, -1 },
		.gate = { CPM_CLKGR, 13 },
	},
	[CLK_UART0] = {
		"uart0", CGU_CLK_GATE,
		.parents = { CLK_EXT, -1, -1, -1 },
		.gate = { CPM_CLKGR, 14 },
	},
	[CLK_UART1] = {
		"uart1", CGU_CLK_GATE,
		.parents = { CLK_EXT, -1, -1, -1 },
		.gate = { CPM_CLKGR, 15 },
		.flags = CLK_IS_CRITICAL | CLK_IGNORE_UNUSED,
	},
	[CLK_UART2] = {
		"uart2", CGU_CLK_GATE,
		.parents = { CLK_EXT, -1, -1, -1 },
		.gate = { CPM_CLKGR, 16 },
	},
	[CLK_HASH] = {
		"hash", CGU_CLK_GATE,
		.parents = { CLK_H2CLK, -1, -1, -1 },
		.gate = { CPM_CLKGR, 18 },
	},
	[CLK_SFC] = {
		"sfc", CGU_CLK_GATE,
		.parents = { CLK_CGU_SFC, -1, -1, -1 },
		.gate = { CPM_CLKGR, 20 },
	},
	[CLK_PDMA] = {
		"pdma", CGU_CLK_GATE,
		.parents = { CLK_H2CLK, -1, -1, -1 },
		.gate = { CPM_CLKGR, 21 },
	},
	[CLK_ISP] = {
		"isp", CGU_CLK_GATE,
		.parents = { CLK_CGU_ISP, -1, -1, -1 },
		.gate = { CPM_CLKGR, 23 },
	},
	[CLK_LCD] = {
		"lcd", CGU_CLK_GATE,
		.parents = { CLK_H0CLK, -1, -1, -1 },
		.gate = { CPM_CLKGR, 24 },
	},
	[CLK_CSI] = {
		"csi", CGU_CLK_GATE,
		.parents = { CLK_PCLK, -1, -1, -1 },
		.gate = { CPM_CLKGR, 25 },
	},
	[CLK_RISCV] = {
		"riscv", CGU_CLK_GATE,
		.parents = { CLK_L2CLK, -1, -1, -1 },
		.gate = { CPM_CLKGR, 26 },
	},
	[CLK_RSA] = {
		"rsa", CGU_CLK_GATE,
		.parents = { CLK_CGU_RSA, -1, -1, -1 },
		.gate = { CPM_CLKGR, 27 },
	},
	[CLK_DES] = {
		"des", CGU_CLK_GATE,
		.parents = { CLK_PCLK, -1, -1, -1 },
		.gate = { CPM_CLKGR, 28 },
	},
	[CLK_TCU] = {
		"tcu", CGU_CLK_GATE,
		.parents = { CLK_PCLK, -1, -1, -1 },
		.gate = { CPM_CLKGR, 30 },
		.flags = CLK_IS_CRITICAL | CLK_IGNORE_UNUSED,
	},
	[CLK_DDR] = {
		"ddr", CGU_CLK_GATE,
		.parents = { CLK_CGU_DDR, -1, -1, -1 },
		.gate = { CPM_CLKGR, 31 },
		.flags = CLK_IS_CRITICAL | CLK_IGNORE_UNUSED,
	},

	/* Peripheral Gates (CLKGR1 - Bank 1) */
	[CLK_AVPU] = {
		"avpu", CGU_CLK_GATE,
		.parents = { CLK_CGU_VPU, -1, -1, -1 },
		.gate = { CPM_CLKGR1, 0 },
	},
	[CLK_DTRNG] = {
		"dtrng", CGU_CLK_GATE,
		.parents = { CLK_PCLK, -1, -1, -1 },
		.gate = { CPM_CLKGR1, 1 },
	},
	[CLK_IPU] = {
		"ipu", CGU_CLK_GATE,
		.parents = { CLK_H0CLK, -1, -1, -1 },
		.gate = { CPM_CLKGR1, 2 },
	},
	[CLK_IVDC] = {
		"ivdc", CGU_CLK_GATE,
		.parents = { CLK_CGU_VPU, -1, -1, -1 },
		.gate = { CPM_CLKGR1, 3 },
	},
	[CLK_GMAC] = {
		"gmac", CGU_CLK_GATE,
		.parents = { CLK_CGU_MACPHY, -1, -1, -1 },
		.gate = { CPM_CLKGR1, 4 },
	},
	[CLK_AES] = {
		"aes", CGU_CLK_GATE,
		.parents = { CLK_H2CLK, -1, -1, -1 },
		.gate = { CPM_CLKGR1, 5 },
	},
	[CLK_AHB0] = {
		"ahb0_gate", CGU_CLK_GATE,
		.parents = { CLK_H0CLK, -1, -1, -1 },
		.gate = { CPM_CLKGR1, 10 },
		.flags = CLK_IS_CRITICAL | CLK_IGNORE_UNUSED,
	},
	[CLK_SYS_OST] = {
		"sys_ost", CGU_CLK_GATE,
		.parents = { CLK_EXT, -1, -1, -1 },
		.gate = { CPM_CLKGR1, 11 },
		.flags = CLK_IS_CRITICAL | CLK_IGNORE_UNUSED,
	},
	[CLK_APB0] = {
		"apb0_gate", CGU_CLK_GATE,
		.parents = { CLK_PCLK, -1, -1, -1 },
		.gate = { CPM_CLKGR1, 14 },
		.flags = CLK_IS_CRITICAL | CLK_IGNORE_UNUSED,
	},
	[CLK_CPU] = {
		"cpu_gate", CGU_CLK_GATE,
		.parents = { CLK_CCLK, -1, -1, -1 },
		.gate = { CPM_CLKGR1, 15 },
		.flags = CLK_IS_CRITICAL | CLK_IGNORE_UNUSED,
	},
	[CLK_MSC] = {
		"msc", CGU_CLK_GATE,
		.parents = { CLK_H2CLK, -1, -1, -1 },
		.gate = { CPM_CLKGR1, 16 },
	},

	/* Composite Peripheral Clocks (CDRs) */
	[CLK_CGU_ISP] = {
		"cgu_isp", CGU_CLK_MUX | CGU_CLK_DIV,
		.parents = { CLK_SCLKA, CLK_MPLL, -1, -1 },
		.mux = { CPM_ISPCDR, 30, 2 },
		.div = { CPM_ISPCDR, 0, 1, 4, 29, 28, 27 },
	},
	[CLK_CGU_CIM] = {
		"cgu_cim", CGU_CLK_MUX | CGU_CLK_DIV,
		.parents = { CLK_SCLKA, CLK_MPLL, -1, -1 },
		.mux = { CPM_CIMCDR, 30, 2 },
		.div = { CPM_CIMCDR, 0, 1, 8, 29, 28, 27 },
	},
	[CLK_CGU_SSI] = {
		"cgu_ssi", CGU_CLK_MUX | CGU_CLK_DIV,
		.parents = { CLK_SCLKA, CLK_MPLL, -1, -1 },
		.mux = { CPM_SSICDR, 30, 2 },
		.div = { CPM_SSICDR, 0, 1, 8, 28, 27, 26 },
	},
	[CLK_CGU_SFC] = {
		"cgu_sfc", CGU_CLK_MUX | CGU_CLK_DIV,
		.parents = { CLK_SCLKA, CLK_MPLL, -1, -1 },
		.mux = { CPM_SFCCDR, 30, 2 },
		.div = { CPM_SFCCDR, 0, 1, 8, 28, 27, 26 },
	},
	[CLK_CGU_I2S_SPK] = {
		"cgu_i2s_spk", CGU_CLK_CUSTOM,
		.parents = { CLK_SCLKA, CLK_MPLL, -1, -1 },
		.custom = { &t23_i2s_ops },
	},
	[CLK_CGU_I2S_MIC] = {
		"cgu_i2s_mic", CGU_CLK_CUSTOM,
		.parents = { CLK_SCLKA, CLK_MPLL, -1, -1 },
		.custom = { &t23_i2s_ops },
	},
	[CLK_CGU_MSC1] = {
		"cgu_msc1", CGU_CLK_MUX | CGU_CLK_DIV,
		.parents = { CLK_SCLKA, CLK_MPLL, -1, -1 },
		.mux = { CPM_MSC1CDR, 30, 2 },
		.div = { CPM_MSC1CDR, 0, 2, 8, 29, 28, 27 },
	},
	[CLK_CGU_MSC0] = {
		"cgu_msc0", CGU_CLK_MUX | CGU_CLK_DIV,
		.parents = { CLK_SCLKA, CLK_MPLL, -1, -1 },
		.mux = { CPM_MSC0CDR, 30, 2 },
		.div = { CPM_MSC0CDR, 0, 2, 8, 29, 28, 27 },
	},
	[CLK_CGU_LPC] = {
		"cgu_lpc", CGU_CLK_MUX | CGU_CLK_DIV,
		.parents = { CLK_SCLKA, CLK_MPLL, -1, -1 },
		.mux = { CPM_LPCDR, 30, 2 },
		.div = { CPM_LPCDR, 0, 1, 8, 28, 27, 26 },
	},
	[CLK_CGU_VPU] = {
		"cgu_vpu", CGU_CLK_MUX | CGU_CLK_DIV,
		.parents = { CLK_SCLKA, CLK_MPLL, -1, -1 },
		.mux = { CPM_VPUCDR, 30, 2 },
		.div = { CPM_VPUCDR, 0, 1, 4, 29, 28, 27 },
	},
	[CLK_CGU_DDR] = {
		"cgu_ddr", CGU_CLK_MUX | CGU_CLK_DIV,
		.parents = { -1, CLK_SCLKA, CLK_MPLL, -1 },
		.mux = { CPM_DDRCDR, 30, 2 },
		.div = { CPM_DDRCDR, 0, 1, 4, 29, 28, 27 },
		.flags = CLK_IS_CRITICAL | CLK_IGNORE_UNUSED,
	},
	[CLK_CGU_MACPHY] = {
		"cgu_macphy", CGU_CLK_MUX | CGU_CLK_DIV,
		.parents = { CLK_SCLKA, CLK_MPLL, -1, -1 },
		.mux = { CPM_MACCDR, 30, 2 },
		.div = { CPM_MACCDR, 0, 1, 8, 29, 28, 27 },
	},
	[CLK_CGU_RSA] = {
		"cgu_rsa", CGU_CLK_MUX | CGU_CLK_DIV,
		.parents = { CLK_SCLKA, CLK_MPLL, -1, -1 },
		.mux = { CPM_RSACDR, 30, 2 },
		.div = { CPM_RSACDR, 0, 1, 4, 29, 28, 27 },
	},
};

static void __init t23_cgu_init(struct device_node *np)
{
	int retval;

	cgu = ingenic_cgu_new(t23_cgu_clocks,
			      ARRAY_SIZE(t23_cgu_clocks), np);
	if (!cgu) {
		pr_err("%s: failed to initialise CGU\n", __func__);
		return;
	}

	cgu->stop_div_on_disable = true;

	retval = ingenic_cgu_register_clocks(cgu);
	if (retval) {
		pr_err("%s: failed to register CGU clocks\n", __func__);
		return;
	}

	ingenic_cgu_register_syscore(cgu);
}

CLK_OF_DECLARE_DRIVER(t23_cgu, "ingenic,t23-cgu", t23_cgu_init);
