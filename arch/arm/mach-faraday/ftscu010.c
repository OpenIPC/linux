/*
 *  linux/arch/arm/mach-faraday/ftscu010.c
 *
 *  Faraday FTSCU010 System Control Unit
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
#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/list.h>
#include <linux/string.h>

#include <linux/clkdev.h>
#include <asm/io.h>

#include <mach/ftscu010.h>

#include "clock.h"

struct ftscu010_clk_params {
	int	mult;
	int	div;
};

static void __iomem *ftscu010_base;

/******************************************************************************
 * internal functions
 *****************************************************************************/
static unsigned long __ftscu010_get_pll_freq(void __iomem *base, unsigned long rate)
{
	unsigned int pll1cr = readl(base + FTSCU010_OFFSET_PLL1CR);
	unsigned long ns = FTSCU010_PLL1CR_PLL_NS(pll1cr);

	return rate * ns / 2;
}

static unsigned long __ftscu010_get_mcpu_clk(void __iomem *base, unsigned long rate)
{
	unsigned int csr = readl(base + FTSCU010_OFFSET_CSR);
	int select = FTSCU010_CSR_CPU_M_FCLK(csr);

	switch(select) {
	case 0:
		return rate / 4;

	case 1:
		return rate / 2;

	default:
		return rate;
	}
}

static unsigned int __ftscu010_get_extahb_clkdiv(void __iomem *base)
{
	unsigned int div = readl(base + FTSCU010_OFFSET_SCLK_CFG0);

	div &= FTSCU010_SCLK_CFG0_EXTAHB_FQ_MASK;
	div >>= FTSCU010_SCLK_CFG0_EXTAHB_FQ_SHIFT;
	return div + 1;
}

static void __ftscu010_set_extahb_clkdiv(void __iomem *base, unsigned int div)
{
	unsigned int cfg0 = readl(base + FTSCU010_OFFSET_SCLK_CFG0);

	div = div - 1;
	div <<= FTSCU010_SCLK_CFG0_EXTAHB_FQ_SHIFT;
	div &= FTSCU010_SCLK_CFG0_EXTAHB_FQ_MASK;

	cfg0 &= ~FTSCU010_SCLK_CFG0_EXTAHB_FQ_MASK;
	cfg0 |= div;
	writel(cfg0, base + FTSCU010_OFFSET_SCLK_CFG0);
}

static int __ftscu010_calc_extahb_clkdiv(unsigned long pll_rate, unsigned long rate)
{
	unsigned int div;

	div = pll_rate / 2 / rate;
	if (div == 0)
		return -EINVAL;

	if (pll_rate / 2 / div > rate)
		div++;

	if (div > 0xf + 1)
		return -EINVAL;

	return div;
}

static unsigned long __ftscu010_calc_extahb_rate(unsigned long pll_rate, unsigned int div)
{
	return pll_rate / 2 / div;
}

	/*
	 * Select external clock source (X_SSP_CLK) as SSP0 clock input, or it
	 * will be (AHB clock * 2) by default.
	 */
static void __ftscu010_select_ssp0_extclk(void __iomem *base, int on)
{
	unsigned int sclk_cfg1;

	sclk_cfg1 = ioread32(base + FTSCU010_OFFSET_SCLK_CFG1);
	if (on)
		sclk_cfg1 |= FTSCU010_SCLK_CFG1_SSP0_SEL;
	else
		sclk_cfg1 &= ~FTSCU010_SCLK_CFG1_SSP0_SEL;

	iowrite32(sclk_cfg1, base+ FTSCU010_OFFSET_SCLK_CFG1);
}

/******************************************************************************
 * struct clk functions
 *****************************************************************************/

static unsigned long ftscu010_get_scaled_clk(struct clk *clk)
{
	struct ftscu010_clk_params *params = clk->params;
	int mult = params->mult;
	int div = params->div;
	unsigned long rate;

	rate = __clk_get_rate(clk->parent);
	clk->rate = rate * mult / div;
	return clk->rate;
}

static unsigned long ftscu010_get_pll_clk(struct clk *clk)
{
	unsigned long rate;

	rate = __clk_get_rate(clk->parent);
	clk->rate = __ftscu010_get_pll_freq(clk->base, rate);
	return clk->rate;
}

static unsigned long ftscu010_get_mcpu_clk(struct clk *clk)
{
	unsigned long rate;

	rate = __clk_get_rate(clk->parent);
	clk->rate = __ftscu010_get_mcpu_clk(clk->base, rate);
	return clk->rate;
}

static int ftscu010_scpu_clk_mode(struct clk *clk, int on)
{
	unsigned int hclkgate = readl(clk->base + FTSCU010_OFFSET_HCLKGATE);

	if (on)
		hclkgate &= ~FTSCU010_HCLKGATE_CPU_S;
	else
		hclkgate |= FTSCU010_HCLKGATE_CPU_S;

	writel(hclkgate, clk->base + FTSCU010_OFFSET_HCLKGATE);
	return 0;
}

static unsigned long ftscu010_get_extahb_clk(struct clk *clk)
{
	unsigned long pll_rate = __clk_get_rate(clk->parent);
	unsigned int div = __ftscu010_get_extahb_clkdiv(clk->base);

	clk->rate = __ftscu010_calc_extahb_rate(pll_rate, div);
	return clk->rate;
}

static int ftscu010_set_extahb_clk(struct clk *clk, unsigned long rate)
{
	unsigned long pll_rate = __clk_get_rate(clk->parent);
	int div = __ftscu010_calc_extahb_clkdiv(pll_rate, rate);

	if (div < 0)
		return -EINVAL;

	__ftscu010_set_extahb_clkdiv(clk->base, div);
	clk->rate = __ftscu010_calc_extahb_rate(pll_rate, div);
	printk(KERN_INFO "ftscu010: set extahb clk rate to %ld\n",
		clk->rate);
	return 0;
}

static int ftscu010_ssp0_extclk_mode(struct clk *clk, int on)
{
	__ftscu010_select_ssp0_extclk(clk->base, on);
	return 0;
}

/******************************************************************************
 * clocks
 *****************************************************************************/
struct clk ftscu010_main_clk;

struct clk ftscu010_pll_clk = {
	.parent		= &ftscu010_main_clk,
	.get_rate	= ftscu010_get_pll_clk,
};

static struct ftscu010_clk_params ftscu010_dclk_params = {
	.mult	= 1,
	.div	= 2,
};

struct clk ftscu010_dclk = {
	.parent		= &ftscu010_pll_clk,
	.get_rate	= ftscu010_get_scaled_clk,
	.params		= &ftscu010_dclk_params,
};

static struct ftscu010_clk_params ftscu010_mclk_params = {
	.mult	= 1,
	.div	= 3,
};

struct clk ftscu010_mclk = {
	.parent		= &ftscu010_pll_clk,
	.get_rate	= ftscu010_get_scaled_clk,
	.params		= &ftscu010_mclk_params,
};

static struct ftscu010_clk_params ftscu010_hclk_params = {
	.mult	= 1,
	.div	= 4,
};

struct clk ftscu010_hclk = {
	.parent		= &ftscu010_pll_clk,
	.get_rate	= ftscu010_get_scaled_clk,
	.params		= &ftscu010_hclk_params,
};

static struct ftscu010_clk_params ftscu010_pclk_params = {
	.mult	= 1,
	.div	= 8,
};

struct clk ftscu010_pclk = {
	.parent		= &ftscu010_pll_clk,
	.get_rate	= ftscu010_get_scaled_clk,
	.params		= &ftscu010_pclk_params,
};

struct clk ftscu010_extahb_clk = {
	.parent		= &ftscu010_pll_clk,
	.get_rate	= ftscu010_get_extahb_clk,
	.set_rate	= ftscu010_set_extahb_clk,
};

struct clk ftscu010_mcpu_clk = {
	.parent		= &ftscu010_pll_clk,
	.get_rate	= ftscu010_get_mcpu_clk,
};

struct clk ftscu010_scpu_clk = {
	.parent		= &ftscu010_hclk,
	.mode		= ftscu010_scpu_clk_mode,
};

struct clk ftscu010_ssp0_extclk = {
	.rate	= 12288000,
	.mode	= ftscu010_ssp0_extclk_mode,
};

struct clk ftscu010_tsc_clk = {
	.parent		= &ftscu010_main_clk,
};

/******************************************************************************
 * pin mux setup
 *
 * This shit is used to handle the brain-damaged pin mux design.
 *****************************************************************************/
struct ftscu010_pinmux_conf {
	const char *name;
	unsigned int set;
};

struct ftscu010_pinmux_fg {
	const char *name;
	unsigned int offset;
	unsigned int mask;
	struct ftscu010_pinmux_conf conf[4];
};

static struct ftscu010_pinmux_fg ftscu010_pinmux_fgs[] = {
	{
		.name	= "extahb",
		.offset	= FTSCU010_OFFSET_MFPSR0,
		.mask	= FTSCU010_MFPSR0_FGEXTAHB_MASK,
		.conf	= {
			{
				.name	= "extahb",
				.set	= FTSCU010_MFPSR0_FGEXTAHB_EXTAHB,
			}, {
				.name	= "peripherals",
				.set	= FTSCU010_MFPSR0_FGEXTAHB_PERIPHERALS,
			},
		},
	}, {
		.name	= "sata",
		.offset	= FTSCU010_OFFSET_MFPSR0,
		.mask	= FTSCU010_MFPSR0_FGSATA_MASK,
		.conf	= {
			{
				.name	= "extahb",
				.set	= FTSCU010_MFPSR0_FGSATA_EXTAHB,
			}, {
				.name	= "sata",
				.set	= FTSCU010_MFPSR0_FGSATA_SATA,
			}, {
				.name	= "extdma",
				.set	= FTSCU010_MFPSR0_FGSATA_EXTDMA,
			},
		},
	}, {
		.name	= "isp",
		.offset	= FTSCU010_OFFSET_MFPSR0,
		.mask	= FTSCU010_MFPSR0_FGISP_MASK,
		.conf	= {
			{
				.name	= "extahb",
				.set	= FTSCU010_MFPSR0_FGISP_EXTAHB,
			}, {
				.name	= "gpio1",
				.set	= FTSCU010_MFPSR0_FGISP_GPIO1,
			}, {
				.name	= "isp",
				.set	= FTSCU010_MFPSR0_FGISP_ISP,
			},
		},
	}, {
		.name	= "ts",
		.offset	= FTSCU010_OFFSET_MFPSR0,
		.mask	= FTSCU010_MFPSR0_FGTS_MASK,
		.conf	= {
			{
				.name	= "extahb",
				.set	= FTSCU010_MFPSR0_FGTS_EXTAHB,
			}, {
				.name	= "gpio0",
				.set	= FTSCU010_MFPSR0_FGTS_GPIO0,
			}, {
				.name	= "ts",
				.set	= FTSCU010_MFPSR0_FGTS_TS,
			},
		}
	}, {
		.name	= "lcd",
		.offset	= FTSCU010_OFFSET_MFPSR0,
		.mask	= FTSCU010_MFPSR0_FGLCD_MASK,
		.conf	= {
			{
				.name	= "lcd",
				.set	= FTSCU010_MFPSR0_FGLCD_LCD,
			}, {
				.name	= "tv_mice",
				.set	= FTSCU010_MFPSR0_FGLCD_TV_MICE,
			}, {
				.name	= "lcd_mice",
				.set	= FTSCU010_MFPSR0_FGLCD_LCD_MICE,
			},
		},
	}, {
		.name	= "ebi",
		.offset	= FTSCU010_OFFSET_MFPSR0,
		.mask	= FTSCU010_MFPSR0_FGEBI_MASK,
		.conf	= {
			{
				.name	= "ebi",
				.set	= FTSCU010_MFPSR0_FGEBI_EBI,
			}, {
				.name	= "ide",
				.set	= FTSCU010_MFPSR0_FGEBI_IDE,
			},
		},
	}, {
		.name	= "uart1",
		.offset	= FTSCU010_OFFSET_MFPSR1,
		.mask	= FTSCU010_MFPSR1_FGUART1_MASK,
		.conf	= {
			{
				.name	= "extahb",
				.set	= FTSCU010_MFPSR1_FGUART1_EXTAHB,
			}, {
				.name	= "gpio0",
				.set	= FTSCU010_MFPSR1_FGUART1_GPIO0,
			}, {
				.name	= "uart1",
				.set	= FTSCU010_MFPSR1_FGUART1_UART1,
			},
		},
	}, {
		.name	= "uart2",
		.offset	= FTSCU010_OFFSET_MFPSR1,
		.mask	= FTSCU010_MFPSR1_FGUART2_MASK,
		.conf	= {
			{
				.name	= "gpio1",
				.set	= FTSCU010_MFPSR1_FGUART2_GPIO1,
			}, {
				.name	= "pwm",
				.set	= FTSCU010_MFPSR1_FGUART2_PWM,
			}, {
				.name	= "uart2",
				.set	= FTSCU010_MFPSR1_FGUART2_UART2,
			},
		},
	}, {
		.name	= "uart3",
		.offset	= FTSCU010_OFFSET_MFPSR1,
		.mask	= FTSCU010_MFPSR1_FGUART3_MASK,
		.conf	= {
			{
				.name	= "gpio1",
				.set	= FTSCU010_MFPSR1_FGUART3_GPIO1,
			}, {
				.name	= "uart3",
				.set	= FTSCU010_MFPSR1_FGUART3_UART3,
			},
		},
	}, {
		.name	= "ssp0",
		.offset	= FTSCU010_OFFSET_MFPSR1,
		.mask	= FTSCU010_MFPSR1_FGSSP0_MASK,
		.conf	= {
			{
				.name	= "ssp0",
				.set	= FTSCU010_MFPSR1_FGSSP0_SSP0,
			}, {
				.name	= "sata",
				.set	= FTSCU010_MFPSR1_FGSSP0_SATA,
			}, {
				.name	= "sice",
				.set	= FTSCU010_MFPSR1_FGSSP0_SICE,
			},
		},
	}, {
		.name	= "ssp1",
		.offset	= FTSCU010_OFFSET_MFPSR1,
		.mask	= FTSCU010_MFPSR1_FGSSP1_MASK,
		.conf	= {
			{
				.name	= "gpio1",
				.set	= FTSCU010_MFPSR1_FGSSP1_GPIO1,
			}, {
				.name	= "i2c1",
				.set	= FTSCU010_MFPSR1_FGSSP1_I2C1,
			}, {
				.name	= "ssp1",
				.set	= FTSCU010_MFPSR1_FGSSP1_SSP1,
			},
		},
	}, {
		.name	= "gmac",
		.offset	= FTSCU010_OFFSET_MFPSR1,
		.mask	= FTSCU010_MFPSR1_FGGMAC_MASK,
		.conf	= {
			{
				.name	= "gmac",
				.set	= FTSCU010_MFPSR1_FGGMAC_GMAC,
			}, {
				.name	= "mcp_ice",
				.set	= FTSCU010_MFPSR1_FGGMAC_MCP_ICE,
			},
		},
	}, {
		.name	= "pwm0",
		.offset	= FTSCU010_OFFSET_MFPSR1,
		.mask	= FTSCU010_MFPSR1_FGPWM0_MASK,
		.conf	= {
			{
				.name	= "extahb",
				.set	= FTSCU010_MFPSR1_FGPWM0_EXTAHB,
			}, {
				.name	= "pwm",
				.set	= FTSCU010_MFPSR1_FGPWM0_PWM,
			}, {
				.name	= "extdma",
				.set	= FTSCU010_MFPSR1_FGPWM0_EXTDMA,
			},
		},
	}, {
		.name	= "pwm1",
		.offset	= FTSCU010_OFFSET_MFPSR1,
		.mask	= FTSCU010_MFPSR1_FGPWM1_MASK,
		.conf	= {
			{
				.name	= "extahb",
				.set	= FTSCU010_MFPSR1_FGPWM1_EXTAHB,
			}, {
				.name	= "gpio1",
				.set	= FTSCU010_MFPSR1_FGPWM1_GPIO1,
			}, {
				.name	= "pwm",
				.set	= FTSCU010_MFPSR1_FGPWM1_PWM,
			},
		},
	}, {
		.name	= "i2c1",
		.offset	= FTSCU010_OFFSET_MFPSR1,
		.mask	= FTSCU010_MFPSR1_FGI2C1_MASK,
		.conf	= {
			{
				.name	= "extahb",
				.set	= FTSCU010_MFPSR1_FGI2C1_EXTAHB,
			}, {
				.name	= "gpio1",
				.set	= FTSCU010_MFPSR1_FGI2C1_GPIO1,
			}, {
				.name	= "i2c1",
				.set	= FTSCU010_MFPSR1_FGI2C1_I2C1,
			},
		},
	}, {
		.name	= "gpio0",
		.offset	= FTSCU010_OFFSET_MFPSR1,
		.mask	= FTSCU010_MFPSR1_FGGPIO0_MASK,
		.conf	= {
			{
				.name	= "extahb",
				.set	= FTSCU010_MFPSR1_FGGPIO0_EXTAHB,
			}, {
				.name	= "gpio0",
				.set	= FTSCU010_MFPSR1_FGGPIO0_GPIO0,
			}, {
				.name	= "extdma",
				.set	= FTSCU010_MFPSR1_FGGPIO0_EXTDMA,
			},
		},
	}, {
		.name	= "kbc",
		.offset	= FTSCU010_OFFSET_MFPSR1,
		.mask	= FTSCU010_MFPSR1_FGKBC_MASK,
		.conf	= {
			{
				.name	= "extahb",
				.set	= FTSCU010_MFPSR1_FGKBC_EXTAHB,
			}, {
				.name	= "gpio0",
				.set	= FTSCU010_MFPSR1_FGKBC_GPIO0,
			}, {
				.name	= "kbc",
				.set	= FTSCU010_MFPSR1_FGKBC_KBC,
			},
		},
	},
};

static struct ftscu010_pinmux_fg *ftscu010_pinmux_find_fg(const char *name)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(ftscu010_pinmux_fgs); i++) {
		struct ftscu010_pinmux_fg *p;

		p = &ftscu010_pinmux_fgs[i];
		if (!p->name)
			break;
		if (!strcmp(p->name, name))
			return p;
	}

	return NULL;
}

static struct ftscu010_pinmux_conf *ftscu010_pinmux_find_conf(
		struct ftscu010_pinmux_fg *fg, const char *name)
{
	int i;

	for (i = 0; i < 4; i++) {
		struct ftscu010_pinmux_conf *p;

		p = &fg->conf[i];
		if (!p->name)
			break;
		if (!strcmp(p->name, name))
			return p;
	}

	return NULL;
}

static void ftscu010_pinmux_set(struct ftscu010_pinmux_fg *fg,
		struct ftscu010_pinmux_conf *conf)
{
	unsigned int reg;

	reg = readl(ftscu010_base + fg->offset);
	reg &= ~fg->mask;
	reg |= conf->set;
	writel(reg, ftscu010_base + fg->offset);
}

int ftscu010_pinmux_setup(const char *fgname, const char *select)
{
	struct ftscu010_pinmux_fg *fg;
	struct ftscu010_pinmux_conf *conf;

	fg = ftscu010_pinmux_find_fg(fgname);
	if (!fg) {
		printk(KERN_ERR "%s: invalid function group %s\n",
			__func__, fgname);
		return -EINVAL;
	}

	conf = ftscu010_pinmux_find_conf(fg, select);
	if (!conf) {
		printk(KERN_ERR
			"%s: invalid selection %s for function group %s\n",
			__func__, select, fgname);
		return -EINVAL;
	}

	ftscu010_pinmux_set(fg, conf);
	return 0;
}

/******************************************************************************
 * DMA configuation
 *****************************************************************************/
struct ftscu010_dma_conf_field {
	unsigned int offset;
	unsigned int shift;
	unsigned int mask;
};

struct ftscu010_dma_device_conf {
	const char *name;
	int req_id;
	struct ftscu010_dma_conf_field handshake;
	struct ftscu010_dma_conf_field select;
};

static struct ftscu010_dma_device_conf ftscu010_dma_device_conf[] = {
	{
		.name	= "uart0tx",
		.req_id	= FTSCU010_DMA_REQ_CFG_UART0TX,
		.handshake	= {
			.offset = FTSCU010_OFFSET_DMA_ACK_CFG0,
			.mask	= FTSCU010_DMA_ACK_CFG0_UART0TX_DMAC_HS_MASK,
			.shift	= FTSCU010_DMA_ACK_CFG0_UART0TX_DMAC_HS_SHIFT,
		},
		.select	= {
			.offset	= FTSCU010_OFFSET_DMA_ACK_CFG0,
			.mask	= FTSCU010_DMA_ACK_CFG0_UART0TX_SELECT_MASK,
			.shift	= FTSCU010_DMA_ACK_CFG0_UART0TX_SELECT_SHIFT,
		},
	}, {
		.name	= "uart0rx",
		.req_id	= FTSCU010_DMA_REQ_CFG_UART0RX,
		.handshake	= {
			.offset = FTSCU010_OFFSET_DMA_ACK_CFG0,
			.mask	= FTSCU010_DMA_ACK_CFG0_UART0RX_DMAC_HS_MASK,
			.shift	= FTSCU010_DMA_ACK_CFG0_UART0RX_DMAC_HS_SHIFT,
		},
		.select	= {
			.offset	= FTSCU010_OFFSET_DMA_ACK_CFG0,
			.mask	= FTSCU010_DMA_ACK_CFG0_UART0RX_SELECT_MASK,
			.shift	= FTSCU010_DMA_ACK_CFG0_UART0RX_SELECT_SHIFT,
		},
	}, {
		.name	= "uart1tx",
		.req_id	= FTSCU010_DMA_REQ_CFG_UART1TX,
		.handshake	= {
			.offset = FTSCU010_OFFSET_DMA_ACK_CFG0,
			.mask	= FTSCU010_DMA_ACK_CFG0_UART1TX_DMAC_HS_MASK,
			.shift	= FTSCU010_DMA_ACK_CFG0_UART1TX_DMAC_HS_SHIFT,
		},
		.select	= {
			.offset	= FTSCU010_OFFSET_DMA_ACK_CFG0,
			.mask	= FTSCU010_DMA_ACK_CFG0_UART1TX_SELECT_MASK,
			.shift	= FTSCU010_DMA_ACK_CFG0_UART1TX_SELECT_SHIFT,
		},
	}, {
		.name	= "uart1rx",
		.req_id	= FTSCU010_DMA_REQ_CFG_UART1RX,
		.handshake	= {
			.offset = FTSCU010_OFFSET_DMA_ACK_CFG0,
			.mask	= FTSCU010_DMA_ACK_CFG0_UART1RX_DMAC_HS_MASK,
			.shift	= FTSCU010_DMA_ACK_CFG0_UART1RX_DMAC_HS_SHIFT,
		},
		.select	= {
			.offset	= FTSCU010_OFFSET_DMA_ACK_CFG0,
			.mask	= FTSCU010_DMA_ACK_CFG0_UART1RX_SELECT_MASK,
			.shift	= FTSCU010_DMA_ACK_CFG0_UART1RX_SELECT_SHIFT,
		},
	}, {
		.name	= "uart2tx",
		.req_id	= FTSCU010_DMA_REQ_CFG_UART2TX,
		.handshake	= {
			.offset = FTSCU010_OFFSET_DMA_ACK_CFG0,
			.mask	= FTSCU010_DMA_ACK_CFG0_UART2TX_DMAC_HS_MASK,
			.shift	= FTSCU010_DMA_ACK_CFG0_UART2TX_DMAC_HS_SHIFT,
		},
		.select	= {
			.offset	= FTSCU010_OFFSET_DMA_ACK_CFG0,
			.mask	= FTSCU010_DMA_ACK_CFG0_UART2TX_SELECT_MASK,
			.shift	= FTSCU010_DMA_ACK_CFG0_UART2TX_SELECT_SHIFT,
		},
	}, {
		.name	= "uart2rx",
		.req_id	= FTSCU010_DMA_REQ_CFG_UART2RX,
		.handshake	= {
			.offset = FTSCU010_OFFSET_DMA_ACK_CFG0,
			.mask	= FTSCU010_DMA_ACK_CFG0_UART2RX_DMAC_HS_MASK,
			.shift	= FTSCU010_DMA_ACK_CFG0_UART2RX_DMAC_HS_SHIFT,
		},
		.select	= {
			.offset	= FTSCU010_OFFSET_DMA_ACK_CFG0,
			.mask	= FTSCU010_DMA_ACK_CFG0_UART2RX_SELECT_MASK,
			.shift	= FTSCU010_DMA_ACK_CFG0_UART2RX_SELECT_SHIFT,
		},
	}, {
		.name	= "uart3tx",
		.req_id	= FTSCU010_DMA_REQ_CFG_UART3TX,
		.handshake	= {
			.offset = FTSCU010_OFFSET_DMA_ACK_CFG1,
			.mask	= FTSCU010_DMA_ACK_CFG1_UART3TX_DMAC_HS_MASK,
			.shift	= FTSCU010_DMA_ACK_CFG1_UART3TX_DMAC_HS_SHIFT,
		},
		.select	= {
			.offset	= FTSCU010_OFFSET_DMA_ACK_CFG1,
			.mask	= FTSCU010_DMA_ACK_CFG1_UART3TX_SELECT_MASK,
			.shift	= FTSCU010_DMA_ACK_CFG1_UART3TX_SELECT_SHIFT,
		},
	}, {
		.name	= "uart3rx",
		.req_id	= FTSCU010_DMA_REQ_CFG_UART3RX,
		.handshake	= {
			.offset = FTSCU010_OFFSET_DMA_ACK_CFG1,
			.mask	= FTSCU010_DMA_ACK_CFG1_UART3RX_DMAC_HS_MASK,
			.shift	= FTSCU010_DMA_ACK_CFG1_UART3RX_DMAC_HS_SHIFT,
		},
		.select	= {
			.offset	= FTSCU010_OFFSET_DMA_ACK_CFG1,
			.mask	= FTSCU010_DMA_ACK_CFG1_UART3RX_SELECT_MASK,
			.shift	= FTSCU010_DMA_ACK_CFG1_UART3RX_SELECT_SHIFT,
		},
	}, {
		.name	= "irda",
		.req_id	= FTSCU010_DMA_REQ_CFG_IRDA,
		.handshake	= {
			.offset = FTSCU010_OFFSET_DMA_ACK_CFG1,
			.mask	= FTSCU010_DMA_ACK_CFG1_IRDA_DMAC_HS_MASK,
			.shift	= FTSCU010_DMA_ACK_CFG1_IRDA_DMAC_HS_SHIFT,
		},
		.select	= {
			.offset	= FTSCU010_OFFSET_DMA_ACK_CFG1,
			.mask	= FTSCU010_DMA_ACK_CFG1_IRDA_SELECT_MASK,
			.shift	= FTSCU010_DMA_ACK_CFG1_IRDA_SELECT_SHIFT,
		},
	}, {
		.name	= "ssp0tx",
		.req_id	= FTSCU010_DMA_REQ_CFG_SSP0TX,
		.handshake	= {
			.offset = FTSCU010_OFFSET_DMA_ACK_CFG2,
			.mask	= FTSCU010_DMA_ACK_CFG2_SSP0TX_DMAC_HS_MASK,
			.shift	= FTSCU010_DMA_ACK_CFG2_SSP0TX_DMAC_HS_SHIFT,
		},
		.select	= {
			.offset	= FTSCU010_OFFSET_DMA_ACK_CFG2,
			.mask	= FTSCU010_DMA_ACK_CFG2_SSP0TX_SELECT_MASK,
			.shift	= FTSCU010_DMA_ACK_CFG2_SSP0TX_SELECT_SHIFT,
		},
	}, {
		.name	= "ssp0rx",
		.req_id	= FTSCU010_DMA_REQ_CFG_SSP0RX,
		.handshake	= {
			.offset = FTSCU010_OFFSET_DMA_ACK_CFG2,
			.mask	= FTSCU010_DMA_ACK_CFG2_SSP0RX_DMAC_HS_MASK,
			.shift	= FTSCU010_DMA_ACK_CFG2_SSP0RX_DMAC_HS_SHIFT,
		},
		.select	= {
			.offset	= FTSCU010_OFFSET_DMA_ACK_CFG2,
			.mask	= FTSCU010_DMA_ACK_CFG2_SSP0RX_SELECT_MASK,
			.shift	= FTSCU010_DMA_ACK_CFG2_SSP0RX_SELECT_SHIFT,
		},
	}, {
		.name	= "ssp1tx",
		.req_id	= FTSCU010_DMA_REQ_CFG_SSP1TX,
		.handshake	= {
			.offset = FTSCU010_OFFSET_DMA_ACK_CFG2,
			.mask	= FTSCU010_DMA_ACK_CFG2_SSP1TX_DMAC_HS_MASK,
			.shift	= FTSCU010_DMA_ACK_CFG2_SSP1TX_DMAC_HS_SHIFT,
		},
		.select	= {
			.offset	= FTSCU010_OFFSET_DMA_ACK_CFG2,
			.mask	= FTSCU010_DMA_ACK_CFG2_SSP1TX_SELECT_MASK,
			.shift	= FTSCU010_DMA_ACK_CFG2_SSP1TX_SELECT_SHIFT,
		},
	}, {
		.name	= "ssp1rx",
		.req_id	= FTSCU010_DMA_REQ_CFG_SSP1RX,
		.handshake	= {
			.offset = FTSCU010_OFFSET_DMA_ACK_CFG2,
			.mask	= FTSCU010_DMA_ACK_CFG2_SSP1RX_DMAC_HS_MASK,
			.shift	= FTSCU010_DMA_ACK_CFG2_SSP1RX_DMAC_HS_SHIFT,
		},
		.select	= {
			.offset	= FTSCU010_OFFSET_DMA_ACK_CFG2,
			.mask	= FTSCU010_DMA_ACK_CFG2_SSP1RX_SELECT_MASK,
			.shift	= FTSCU010_DMA_ACK_CFG2_SSP1RX_SELECT_SHIFT,
		},
	}, {
		.name	= "tsc",
		.req_id	= FTSCU010_DMA_REQ_CFG_TSC,
		.handshake	= {
			.offset = FTSCU010_OFFSET_DMA_ACK_CFG2,
			.mask	= FTSCU010_DMA_ACK_CFG2_TSC_DMAC_HS_MASK,
			.shift	= FTSCU010_DMA_ACK_CFG2_TSC_DMAC_HS_SHIFT,
		},
		.select	= {
			.offset	= FTSCU010_OFFSET_DMA_ACK_CFG2,
			.mask	= FTSCU010_DMA_ACK_CFG2_TSC_SELECT_MASK,
			.shift	= FTSCU010_DMA_ACK_CFG2_TSC_SELECT_SHIFT,
		},
	}, {
		.name	= "ts",
		.req_id	= FTSCU010_DMA_REQ_CFG_TS,
		.handshake	= {
			.offset = FTSCU010_OFFSET_DMA_ACK_CFG2,
			.mask	= FTSCU010_DMA_ACK_CFG2_TS_DMAC_HS_MASK,
			.shift	= FTSCU010_DMA_ACK_CFG2_TS_DMAC_HS_SHIFT,
		},
		.select	= {
			.offset	= FTSCU010_OFFSET_DMA_ACK_CFG2,
			.mask	= FTSCU010_DMA_ACK_CFG2_TS_SELECT_MASK,
			.shift	= FTSCU010_DMA_ACK_CFG2_TS_SELECT_SHIFT,
		},
	}, {
		.name	= "tmr1",
		.req_id	= FTSCU010_DMA_REQ_CFG_TMR1,
		.handshake	= {
			.offset = FTSCU010_OFFSET_DMA_ACK_CFG3,
			.mask	= FTSCU010_DMA_ACK_CFG3_TMR1_DMAC_HS_MASK,
			.shift	= FTSCU010_DMA_ACK_CFG3_TMR1_DMAC_HS_SHIFT,
		},
		.select	= {
			.offset	= FTSCU010_OFFSET_DMA_ACK_CFG3,
			.mask	= FTSCU010_DMA_ACK_CFG3_TMR1_SELECT_MASK,
			.shift	= FTSCU010_DMA_ACK_CFG3_TMR1_SELECT_SHIFT,
		},
	}, {
		.name	= "tmr2",
		.req_id	= FTSCU010_DMA_REQ_CFG_TMR2,
		.handshake	= {
			.offset = FTSCU010_OFFSET_DMA_ACK_CFG3,
			.mask	= FTSCU010_DMA_ACK_CFG3_TMR2_DMAC_HS_MASK,
			.shift	= FTSCU010_DMA_ACK_CFG3_TMR2_DMAC_HS_SHIFT,
		},
		.select	= {
			.offset	= FTSCU010_OFFSET_DMA_ACK_CFG3,
			.mask	= FTSCU010_DMA_ACK_CFG3_TMR2_SELECT_MASK,
			.shift	= FTSCU010_DMA_ACK_CFG3_TMR2_SELECT_SHIFT,
		},
	}, {
		.name	= "tmr3",
		.req_id	= FTSCU010_DMA_REQ_CFG_TMR3,
		.handshake	= {
			.offset = FTSCU010_OFFSET_DMA_ACK_CFG3,
			.mask	= FTSCU010_DMA_ACK_CFG3_TMR3_DMAC_HS_MASK,
			.shift	= FTSCU010_DMA_ACK_CFG3_TMR3_DMAC_HS_SHIFT,
		},
		.select	= {
			.offset	= FTSCU010_OFFSET_DMA_ACK_CFG3,
			.mask	= FTSCU010_DMA_ACK_CFG3_TMR3_SELECT_MASK,
			.shift	= FTSCU010_DMA_ACK_CFG3_TMR3_SELECT_SHIFT,
		},
	}, {
		.name	= "tmr5",
		.req_id	= FTSCU010_DMA_REQ_CFG_TMR5,
		.handshake	= {
			.offset = FTSCU010_OFFSET_DMA_ACK_CFG3,
			.mask	= FTSCU010_DMA_ACK_CFG3_TMR5_DMAC_HS_MASK,
			.shift	= FTSCU010_DMA_ACK_CFG3_TMR5_DMAC_HS_SHIFT,
		},
		.select	= {
			.offset	= FTSCU010_OFFSET_DMA_ACK_CFG3,
			.mask	= FTSCU010_DMA_ACK_CFG3_TMR5_SELECT_MASK,
			.shift	= FTSCU010_DMA_ACK_CFG3_TMR5_SELECT_SHIFT,
		},
	}, {
		.name	= "tmr6",
		.req_id	= FTSCU010_DMA_REQ_CFG_TMR6,
		.handshake	= {
			.offset = FTSCU010_OFFSET_DMA_ACK_CFG3,
			.mask	= FTSCU010_DMA_ACK_CFG3_TMR6_DMAC_HS_MASK,
			.shift	= FTSCU010_DMA_ACK_CFG3_TMR6_DMAC_HS_SHIFT,
		},
		.select	= {
			.offset	= FTSCU010_OFFSET_DMA_ACK_CFG3,
			.mask	= FTSCU010_DMA_ACK_CFG3_TMR6_SELECT_MASK,
			.shift	= FTSCU010_DMA_ACK_CFG3_TMR6_SELECT_SHIFT,
		},
	}, {
		.name	= "tmr7",
		.req_id	= FTSCU010_DMA_REQ_CFG_TMR7,
		.handshake	= {
			.offset = FTSCU010_OFFSET_DMA_ACK_CFG3,
			.mask	= FTSCU010_DMA_ACK_CFG3_TMR7_DMAC_HS_MASK,
			.shift	= FTSCU010_DMA_ACK_CFG3_TMR7_DMAC_HS_SHIFT,
		},
		.select	= {
			.offset	= FTSCU010_OFFSET_DMA_ACK_CFG3,
			.mask	= FTSCU010_DMA_ACK_CFG3_TMR7_SELECT_MASK,
			.shift	= FTSCU010_DMA_ACK_CFG3_TMR7_SELECT_SHIFT,
		},
	},
};

static struct ftscu010_dma_conf_field ftscu010_req_conf_dmac0[7] = {
	{
		.offset	= FTSCU010_OFFSET_DMAC0_REQ_CFG1,
		.mask	= FTSCU010_DMAC0_REQ_CFG1_HS0_MASK,
		.shift	= FTSCU010_DMAC0_REQ_CFG1_HS0_SHIFT,
	}, {
		.offset	= FTSCU010_OFFSET_DMAC0_REQ_CFG1,
		.mask	= FTSCU010_DMAC0_REQ_CFG1_HS1_MASK,
		.shift	= FTSCU010_DMAC0_REQ_CFG1_HS1_SHIFT,
	}, {
		.offset	= FTSCU010_OFFSET_DMAC0_REQ_CFG1,
		.mask	= FTSCU010_DMAC0_REQ_CFG1_HS2_MASK,
		.shift	= FTSCU010_DMAC0_REQ_CFG1_HS2_SHIFT,
	}, {
		.offset	= FTSCU010_OFFSET_DMAC0_REQ_CFG0,
		.mask	= FTSCU010_DMAC0_REQ_CFG0_HS3_MASK,
		.shift	= FTSCU010_DMAC0_REQ_CFG0_HS3_SHIFT,
	}, {
		.offset	= FTSCU010_OFFSET_DMAC0_REQ_CFG0,
		.mask	= FTSCU010_DMAC0_REQ_CFG0_HS4_MASK,
		.shift	= FTSCU010_DMAC0_REQ_CFG0_HS4_SHIFT,
	}, {
		.offset	= FTSCU010_OFFSET_DMAC0_REQ_CFG0,
		.mask	= FTSCU010_DMAC0_REQ_CFG0_HS5_MASK,
		.shift	= FTSCU010_DMAC0_REQ_CFG0_HS5_SHIFT,
	}, {
		.offset	= FTSCU010_OFFSET_DMAC0_REQ_CFG0,
		.mask	= FTSCU010_DMAC0_REQ_CFG0_HS6_MASK,
		.shift	= FTSCU010_DMAC0_REQ_CFG0_HS6_SHIFT,
	},
};

static struct ftscu010_dma_conf_field ftscu010_req_conf_dmac1[7] = {
	{
		.offset	= FTSCU010_OFFSET_DMAC1_REQ_CFG1,
		.mask	= FTSCU010_DMAC1_REQ_CFG1_HS0_MASK,
		.shift	= FTSCU010_DMAC1_REQ_CFG1_HS0_SHIFT,
	}, {
		.offset	= FTSCU010_OFFSET_DMAC1_REQ_CFG1,
		.mask	= FTSCU010_DMAC1_REQ_CFG1_HS1_MASK,
		.shift	= FTSCU010_DMAC1_REQ_CFG1_HS1_SHIFT,
	}, {
		.offset	= FTSCU010_OFFSET_DMAC1_REQ_CFG1,
		.mask	= FTSCU010_DMAC1_REQ_CFG1_HS2_MASK,
		.shift	= FTSCU010_DMAC1_REQ_CFG1_HS2_SHIFT,
	}, {
		.offset	= FTSCU010_OFFSET_DMAC1_REQ_CFG0,
		.mask	= FTSCU010_DMAC1_REQ_CFG0_HS3_MASK,
		.shift	= FTSCU010_DMAC1_REQ_CFG0_HS3_SHIFT,
	}, {
		.offset	= FTSCU010_OFFSET_DMAC1_REQ_CFG0,
		.mask	= FTSCU010_DMAC1_REQ_CFG0_HS4_MASK,
		.shift	= FTSCU010_DMAC1_REQ_CFG0_HS4_SHIFT,
	}, {
		.offset	= FTSCU010_OFFSET_DMAC1_REQ_CFG0,
		.mask	= FTSCU010_DMAC1_REQ_CFG0_HS5_MASK,
		.shift	= FTSCU010_DMAC1_REQ_CFG0_HS5_SHIFT,
	}, {
		.offset	= FTSCU010_OFFSET_DMAC1_REQ_CFG0,
		.mask	= FTSCU010_DMAC1_REQ_CFG0_HS6_MASK,
		.shift	= FTSCU010_DMAC1_REQ_CFG0_HS6_SHIFT,
	},
};

static struct ftscu010_dma_device_conf *ftscu010_dma_device_conf_find(
		const char *name)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(ftscu010_dma_device_conf); i++) {
		struct ftscu010_dma_device_conf *p;

		p = &ftscu010_dma_device_conf[i];
		if (!strcmp(p->name, name))
			return p;
	}

	return NULL;
}

static void ftscu010_dma_conf_field_set(void __iomem *base,
		struct ftscu010_dma_conf_field *field, unsigned int val)
{
	unsigned int reg;

	reg = readl(base + field->offset);
	reg &= ~field->mask;
	reg |= (val << field->shift) & field->mask;
	writel(reg, base + field->offset);
}

static void ftscu010_dma_config_ack(void __iomem *base,
		struct ftscu010_dma_device_conf *devconf, unsigned int select,
		unsigned int handshake)
{
	ftscu010_dma_conf_field_set(base, &devconf->handshake, handshake);
	ftscu010_dma_conf_field_set(base, &devconf->select, select);
}


static void ftscu010_dma_config_req_dmac0(void __iomem *base,
		unsigned int req_id, unsigned int handshake)
{
	ftscu010_dma_conf_field_set(base, &ftscu010_req_conf_dmac0[handshake],
		req_id);
}

static void ftscu010_dma_config_req_dmac1(void __iomem *base,
		unsigned int req_id, unsigned int handshake)
{
	ftscu010_dma_conf_field_set(base, &ftscu010_req_conf_dmac1[handshake],
		req_id);
}
static void ftscu010_dma_config_apbb(struct ftscu010_dma_device_conf *devconf)
{
	ftscu010_dma_config_ack(ftscu010_base, devconf,
		FTSCU010_DMA_ACK_CFG_SELECT_APBB, 0);
}

static void ftscu010_dma_config_dmac0(struct ftscu010_dma_device_conf *devconf,
		unsigned int handshake)
{
	ftscu010_dma_config_req_dmac0(ftscu010_base, devconf->req_id, handshake);
	ftscu010_dma_config_ack(ftscu010_base, devconf,
		FTSCU010_DMA_ACK_CFG_SELECT_DMAC0, handshake);
}

static void ftscu010_dma_config_dmac1(struct ftscu010_dma_device_conf *devconf,
		unsigned int handshake)
{
	ftscu010_dma_config_req_dmac1(ftscu010_base, devconf->req_id, handshake);
	ftscu010_dma_config_ack(ftscu010_base, devconf,
		FTSCU010_DMA_ACK_CFG_SELECT_DMAC1, handshake);
}

static struct ftscu010_dma_device_conf *handshake_to_devconf[7];

/*
 * ftscu010_handshake_alloc - allocate DMAC handshake number for APB devices
 * @name: connection name
 *
 * Returns handshake number or error code.
 *
 * Usually, APB devices use the internal DMA engine of APB bridge, but you
 * can use DMA controller for performance critical situation.
 */
int ftscu010_handshake_alloc(const char *name)
{
	struct ftscu010_dma_device_conf *devconf;
	int i;

	devconf = ftscu010_dma_device_conf_find(name);
	if (!devconf) {
		printk(KERN_ERR "%s: %s not found\n", __func__, name);
		return -EINVAL;
	}

	for (i = 0; i < 7; i++) {
		if (handshake_to_devconf[i] == NULL)
			break;
	}

	if (i == 7) {
		printk(KERN_ERR "%s: no free handshake number available\n",
			__func__);
		return -ENODEV;
	}

	handshake_to_devconf[i] = devconf;
	return i;
}

/**
 * ftscu010_handshake_setup - setup allocated handshake number for a dmac
 * @handshake: previously allocated handshake number
 * @which: 0 for DMAC0, 1 for DMAC1
 */
int ftscu010_handshake_setup(unsigned int handshake, int which)
{
	struct ftscu010_dma_device_conf *devconf;

	if (handshake >= 7) {
		printk(KERN_ERR "%s: invalid handshake number %d\n",
			__func__, handshake);
		return -EINVAL;
	}

	devconf = handshake_to_devconf[handshake];
	if (!devconf) {
		printk(KERN_ERR "%s: handshake number %d was not allocated\n",
			__func__, handshake);
		return -EINVAL;
	}

	switch (which) {
	case 0:
		ftscu010_dma_config_dmac0(devconf, handshake);
		return 0;
	case 1:
		ftscu010_dma_config_dmac1(devconf, handshake);
		return 0;
	default:
		printk(KERN_ERR "%s: there are only 2 dmac on a369\n",
			__func__);
		return -EINVAL;
	}
}

/**
 * ftscu010_handshake_free - free DMAC handshake number
 * @handshake: previously allocated handshake number
 */
int ftscu010_handshake_free(unsigned int handshake)
{
	struct ftscu010_dma_device_conf *devconf;

	if (handshake >= 7) {
		printk(KERN_ERR "%s: invalid handshake number %d\n",
			__func__, handshake);
		return -EINVAL;
	}

	devconf = handshake_to_devconf[handshake];
	if (!devconf) {
		printk(KERN_ERR "%s: handshake number %d is already free\n",
			__func__, handshake);
		return -EINVAL;
	}

	ftscu010_dma_config_apbb(devconf);
	handshake_to_devconf[handshake] = NULL;
	return 0;
}

/******************************************************************************
 * initial functions
 *****************************************************************************/
void ftscu010_init(void __iomem *base)
{
	unsigned int reg;
	unsigned int extcpu;

	ftscu010_base	= base;

	ftscu010_pll_clk.base		= base;
	ftscu010_extahb_clk.base	= base;
	ftscu010_mcpu_clk.base		= base;
	ftscu010_scpu_clk.base		= base;
	ftscu010_ssp0_extclk.base	= base;

	/*
	 * Enable external AHB
	 *
	 * By default, reset signal of external AHB is hold.
	 * To use external AHB, we have to release it. But be careful!
	 * The external AHB reset and slave CPU reset are connected together.
	 * Since we are not using slave CPU and did not prepare program for it,
	 * we must gate the slave CPU clock before doing reset.
	 *
	 * What the fuck!
	 */
	reg = readl(base + FTSCU010_OFFSET_HCLKGATE);
	reg |= FTSCU010_HCLKGATE_CPU_S;
	writel(reg, base + FTSCU010_OFFSET_HCLKGATE);

	reg = readl(base + FTSCU010_OFFSET_GC);
	reg |= FTSCU010_GC_CPU_S_EXTAHB_RSTN;
	writel(reg, base + FTSCU010_OFFSET_GC);

	/*
	 * Setup external IRQ source
	 *
	 * If the external CPU is selected, enable the external IRQ source
	 * or we cannot receive any interrupt.
	 */
	reg = readl(base + FTSCU010_OFFSET_EXT_CS);
	extcpu = reg & FTSCU010_EXT_CS_CPU_MODE;
	reg = readl(base + FTSCU010_OFFSET_GC);
	reg &= ~FTSCU010_GC_EXT_IRQ_SRC;
	reg |= (extcpu ? FTSCU010_GC_EXT_IRQ_SRC : 0);
	writel(reg, base + FTSCU010_OFFSET_GC);
}
