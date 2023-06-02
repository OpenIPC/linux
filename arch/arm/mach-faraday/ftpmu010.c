/*
 *  linux/arch/arm/mach-faraday/ftpmu010.c
 *
 *  Faraday FTPMU010 Power Management Unit
 *
 *  Copyright (C) 2009 Faraday Technology
 *  Copyright (C) 2009 Po-Yu Chuang <ratbert@faraday-tech.com>
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/kernel.h>
#include <linux/list.h>

#include <linux/clkdev.h>
#include <asm/io.h>

#include <mach/ftpmu010.h>

#include "clock.h"

struct ftpmu010_clk_params {
	int	mult;
	int	div;
};

/******************************************************************************
 * internal functions
 *****************************************************************************/
static unsigned int __ftpmu010_in_turbo_mode(void __iomem *base)
{
	unsigned int pmode = readl(base + FTPMU010_OFFSET_PMODE);

	return pmode & FTPMU010_PMODE_TURBO;
}

static unsigned int __ftpmu010_get_divahbclk(void __iomem *base)
{
	static const unsigned int bits2div[] = { 2, 3, 4, 6, 8, };
	unsigned int pmode = readl(base + FTPMU010_OFFSET_PMODE);
	unsigned int divbits = FTPMU010_PMODE_DIVAHBCLK(pmode);

	if (divbits > 4) {
		printk(KERN_ERR "unknown DIVAHBCLK %d\n", divbits);
		return 8;
	}

	return bits2div[divbits];
}

static unsigned long __ftpmu010_get_pll1_freq(void __iomem *base, unsigned long rate)
{
	unsigned int pdllcr0 = readl(base + FTPMU010_OFFSET_PDLLCR0);

	if (!(pdllcr0 & FTPMU010_PDLLCR0_PLL1DIS)) {
		unsigned long mul = FTPMU010_PDLLCR0_PLL1NS(pdllcr0);

		rate *= mul;
	}

	return rate;
}

static unsigned long __ftpmu010_get_hclk(void __iomem *base, unsigned long rate)
{
	unsigned int div = __ftpmu010_get_divahbclk(base);

	return __ftpmu010_get_pll1_freq(base, rate) / div;
}

static unsigned long __ftpmu010_get_cpuclk(void __iomem *base, unsigned long rate)
{
	if (__ftpmu010_in_turbo_mode(base))
		return __ftpmu010_get_pll1_freq(base, rate);

	return __ftpmu010_get_hclk(base, rate);
}

static int __ftpmu010_enable_32768hz_osc(void __iomem *base)
{
	unsigned int oscc;

	/* enable the 32768Hz oscillator */
	oscc = readl(base + FTPMU010_OFFSET_OSCC);
	oscc &= ~(FTPMU010_OSCC_OSCL_OFF | FTPMU010_OSCC_OSCL_TRI);
	writel(oscc, base + FTPMU010_OFFSET_OSCC);

	/* wait until ready */
	oscc = readl(base + FTPMU010_OFFSET_OSCC);
	while (!(oscc & FTPMU010_OSCC_OSCL_STABLE)) {
		udelay(1);
		oscc = readl(base + FTPMU010_OFFSET_OSCC);
	}

	/* select 32768Hz oscillator */
	oscc |= FTPMU010_OSCC_OSCL_RTCLSEL;
	writel(oscc, base + FTPMU010_OFFSET_OSCC);

	return 0;
}

static int __ftpmu010_disable_32768hz_osc(void __iomem *base)
{
	unsigned int oscc;

	/* deselect 32768Hz oscillator */
	oscc = readl(base + FTPMU010_OFFSET_OSCC);
	oscc &= ~FTPMU010_OSCC_OSCL_RTCLSEL;
	writel(oscc, base + FTPMU010_OFFSET_OSCC);

	/* disable the 32768Hz oscillator */
	oscc = readl(base + FTPMU010_OFFSET_OSCC);
	oscc |= FTPMU010_OSCC_OSCL_OFF | FTPMU010_OSCC_OSCL_TRI;
	writel(oscc, base + FTPMU010_OFFSET_OSCC);

	return 0;
}

/******************************************************************************
 * struct clk functions
 *****************************************************************************/

static unsigned long ftpmu010_get_scaled_clk(struct clk *clk)
{
	struct ftpmu010_clk_params *params = clk->params;
	int mult = params->mult;
	int div = params->div;
	unsigned long rate;

	rate = __clk_get_rate(clk->parent);
	clk->rate = rate * mult / div;
	return clk->rate;
}

static unsigned long ftpmu010_get_cpuclk(struct clk *clk)
{
	unsigned long rate;

	rate = __clk_get_rate(clk->parent);
	clk->rate = __ftpmu010_get_cpuclk(clk->base, rate);
	return clk->rate;
}

static unsigned long ftpmu010_get_hclk(struct clk *clk)
{
	unsigned long rate;

	rate = __clk_get_rate(clk->parent);
	clk->rate = __ftpmu010_get_hclk(clk->base, rate);
	return clk->rate;
}

static int ftpmu010_32768hz_osc_mode(struct clk *clk, int on)
{
	if (on)
		return __ftpmu010_enable_32768hz_osc(clk->base);
	else
		return __ftpmu010_disable_32768hz_osc(clk->base);
}

/******************************************************************************
 * clocks
 *****************************************************************************/
struct clk ftpmu010_main_clk;

struct clk ftpmu010_cpuclk = {
	.parent		= &ftpmu010_main_clk,
	.get_rate	= ftpmu010_get_cpuclk,
};

struct clk ftpmu010_hclk = {
	.parent		= &ftpmu010_main_clk,
	.get_rate	= ftpmu010_get_hclk,
};

static struct ftpmu010_clk_params ftpmu010_pclk_params = {
	.mult	= 1,
	.div	= 2,
};

struct clk ftpmu010_pclk = {
	.parent		= &ftpmu010_hclk,
	.get_rate	= ftpmu010_get_scaled_clk,
	.params		= &ftpmu010_pclk_params,
};

static struct ftpmu010_clk_params ftpmu010_pll2_params = {
	.mult	= 13,
	.div	= 1,
};

struct clk ftpmu010_pll2_clk = {
	.parent		= &ftpmu010_main_clk,
	.get_rate	= ftpmu010_get_scaled_clk,
	.params		= &ftpmu010_pll2_params,
};

struct clk ftpmu010_irda_clk = {
	.parent		= &ftpmu010_pll2_clk,
};

static struct ftpmu010_clk_params ftpmu010_pll3_params = {
	.mult	= 40,
	.div	= 1,
};

struct clk ftpmu010_pll3_clk = {
	.parent		= &ftpmu010_main_clk,
	.get_rate	= ftpmu010_get_scaled_clk,
	.params		= &ftpmu010_pll3_params,
};

static struct ftpmu010_clk_params ftpmu010_ssp_params = {
	.mult	= 1,
	.div	= 6,
};

struct clk ftpmu010_ssp_clk = {
	.parent		= &ftpmu010_pll3_clk,
	.get_rate	= ftpmu010_get_scaled_clk,
	.params		= &ftpmu010_ssp_params,
};

static struct ftpmu010_clk_params ftpmu010_i2s_params = {
	.mult	= 1,
	.div	= 12,
};

struct clk ftpmu010_i2s_clk = {
	.parent		= &ftpmu010_pll3_clk,
	.get_rate	= ftpmu010_get_scaled_clk,
	.params		= &ftpmu010_i2s_params,
};

static struct ftpmu010_clk_params ftpmu010_ac97_params1 = {
	.mult	= 1,
	.div	= 3,
};

struct clk ftpmu010_ac97_clk1 = {
	.parent		= &ftpmu010_pll3_clk,
	.get_rate	= ftpmu010_get_scaled_clk,
	.params		= &ftpmu010_ac97_params1,
};

static struct ftpmu010_clk_params ftpmu010_ac97_params2 = {
	.mult	= 1,
	.div	= 6,
};

struct clk ftpmu010_ac97_clk2 = {
	.parent		= &ftpmu010_pll3_clk,
	.get_rate	= ftpmu010_get_scaled_clk,
	.params		= &ftpmu010_ac97_params2,
};

static struct ftpmu010_clk_params ftpmu010_uart_params = {
	.mult	= 1,
	.div	= 8,
};

struct clk ftpmu010_uart_clk = {
	.parent		= &ftpmu010_pll3_clk,
	.get_rate	= ftpmu010_get_scaled_clk,
	.params		= &ftpmu010_uart_params,
};

struct clk ftpmu010_32768hz_clk = {
	.rate	= 32768,
	.mode	= ftpmu010_32768hz_osc_mode,
};

/******************************************************************************
 * initial functions
 *****************************************************************************/
void ftpmu010_init(void __iomem *base)
{
	ftpmu010_cpuclk.base		= base;
	ftpmu010_hclk.base		= base;
	ftpmu010_pclk.base		= base;
	ftpmu010_32768hz_clk.base	= base;
}
