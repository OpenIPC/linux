/*
 * Fullhan FH8833 board support
 *
 * Copyright (C) 2014 Fullhan Microelectronics Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation version 2.
 *
 * This program is distributed "as is" WITHOUT ANY WARRANTY of any
 * kind, whether express or implied; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */
#include <linux/init.h>
#include <linux/clk.h>
#include <linux/serial_8250.h>
#include <linux/platform_device.h>
#include <linux/dma-mapping.h>
#include <linux/gpio.h>
#include <linux/spi/spi.h>

#include <asm/mach/map.h>

#include <mach/chip.h>
#include <mach/irqs.h>
#include <mach/timex.h>
#include <mach/pmu.h>
#include <mach/clock.h>
#include <mach/board_config.h>

/*
 * external oscillator
 * fixed to 24M
 */
static struct clk osc_clk = {
	.name               = "osc_clk",
	.frequency          = OSC_FREQUENCY,
	.flag               = CLOCK_FIXED,
};

/*
 * phase-locked-loop device,
 * generates a higher frequency clock
 * from the external oscillator reference
 */
static struct clk pll0p_clk = {
	.name               = "pll0p_clk",
	.flag               = CLOCK_PLL_P,
	.parent             = {&osc_clk},
	.div_reg_offset     = REG_PMU_PLL0,
	.en_reg_offset      = REG_PMU_PLL_CTRL,
	.en_reg_mask        = 0xf00,
};

static struct clk pll0r_clk = {
	.name               = "pll0r_clk",
	.flag               = CLOCK_PLL_R,
	.parent             = {&osc_clk},
	.div_reg_offset     = REG_PMU_PLL0,
	.en_reg_offset      = REG_PMU_PLL_CTRL,
	.en_reg_mask        = 0xf000,
};

#if 0
static struct clk pll1p_clk = {
	.name               = "pll1p_clk",
	.flag               = CLOCK_PLL_P,
	.parent             = {&osc_clk},
	.div_reg_offset     = REG_PMU_PLL1,
	.en_reg_offset      = REG_PMU_PLL_CTRL,
	.en_reg_mask        = 0xf000000,
};
#endif

static struct clk pll1r_clk = {
	.name               = "pll1r_clk",
	.flag               = CLOCK_PLL_R,
	.parent             = {&osc_clk},
	.div_reg_offset     = REG_PMU_PLL1,
	.en_reg_offset      = REG_PMU_PLL_CTRL,
	.en_reg_mask        = 0xf0000000,
};

static struct clk pll2p_clk = {
	.name               = "pll2p_clk",
	.flag               = CLOCK_PLL_P,
	.parent             = {&osc_clk},
	.div_reg_offset     = REG_PMU_PLL2,
	.en_reg_offset      = REG_PMU_PLL2_CTRL,
	.en_reg_mask        = 0xf00,
};

#if 0
static struct clk pll2r_clk = {
	.name               = "pll2r_clk",
	.flag               = CLOCK_PLL_R,
	.parent             = {&osc_clk},
	.div_reg_offset     = REG_PMU_PLL2,
	.en_reg_offset      = REG_PMU_PLL2_CTRL,
	.en_reg_mask        = 0xf000,
};
#endif
/*
 * pll0r
 */
static struct clk arm_clk = {
	.name               = "arm_clk",
	.flag               = CLOCK_NOGATE,
	.parent             = {&pll0r_clk},
	.prediv             = 1,
	.div_reg_offset     = REG_PMU_CLK_DIV0,
	.div_reg_mask       = 0xf,
	.rst_reg_offset     = REG_PMU_SWRST_MAIN_CTRL,
	.rst_reg_mask       = 0x1,
};

static struct clk ahb_clk = {
	.name               = "ahb_clk",
	.flag               = CLOCK_NORESET|CLOCK_NOGATE,
	.parent             = {&pll0r_clk},
	.prediv             = 2,
	.div_reg_offset     = REG_PMU_CLK_DIV0,
	.div_reg_mask       = 0xf0000,
};
static struct clk arc_clk = {
	.name               = "arc_clk",
	.flag               = 0,
	.parent             = {&pll0r_clk},
	.prediv             = 1,
	.div_reg_offset     = REG_PMU_CLK_DIV0,
	.div_reg_mask       = 0xf0000,
	.en_reg_offset      = REG_PMU_CLK_GATE,
	.en_reg_mask        = 0x4,
	.rst_reg_offset     = REG_PMU_SWRST_MAIN_CTRL,
	.rst_reg_mask       = 0x400000,
};

static struct clk pae_arc_clk = {
	.name               = "pae_arc_clk",
	.flag               = 0,
	.parent             = {&pll0r_clk},
	.prediv             = 1,
	.div_reg_offset     = REG_PMU_CLK_DIV0,
	.div_reg_mask       = 0xf0000,
	.en_reg_offset      = REG_PMU_CLK_GATE,
	.en_reg_mask        = 0x8,
	.rst_reg_offset     = REG_PMU_SWRST_MAIN_CTRL,
	.rst_reg_mask       = 0x400000,
};

static struct clk cis_clk_out = {
	.name               = "cis_clk_out",
	.flag               = CLOCK_NORESET,
	.parent             = {&pll0r_clk},
	.prediv             = 1,
	.div_reg_offset     = REG_PMU_CLK_DIV1,
	.div_reg_mask       = 0xff0000,
	.en_reg_offset      = REG_PMU_CLK_GATE,
	.en_reg_mask        = 0x800000,
};

/*
 * pll1r
 */
static struct clk ddr_clk = {
	.name               = "ddr_clk",
	.parent             = {&pll1r_clk},
	.prediv             = 1,
	.div_reg_offset     = REG_PMU_CLK_DIV1,
	.div_reg_mask       = 0xf,
	.en_reg_offset      = REG_PMU_CLK_GATE,
	.en_reg_mask        = 0x40,
	.rst_reg_offset     = REG_PMU_SWRST_MAIN_CTRL,
	.rst_reg_mask       = 0x8,
};
/*
 * pll0p
 */

static struct clk isp_aclk = {
	.name               = "isp_aclk",
	.flag               = CLOCK_NORESET,
	.parent             = {&pll0p_clk},
	.prediv             = 1,
	.div_reg_offset     = REG_PMU_CLK_DIV0,
	.div_reg_mask       = 0xf00,
	.en_reg_offset      = REG_PMU_CLK_GATE,
	.en_reg_mask        = 0x1,
	.def_rate           = CONFIG_ISP_CLK_RATE,
};

static struct clk jpeg_clk = {
	.name               = "jpeg_clk",
	.flag               = CLOCK_NORESET|CLOCK_NODIV,
	.parent             = {&isp_aclk},
	.prediv             = 1,
	.en_reg_offset      = REG_PMU_CLK_GATE,
	.en_reg_mask        = 0x1,
};

static struct clk vpu_clk = {
	.name               = "vpu_clk",
	.flag               = CLOCK_NORESET|CLOCK_NODIV,
	.parent             = {&isp_aclk},
	.prediv             = 1,
	.en_reg_offset	    = REG_PMU_CLK_GATE,
	.en_reg_mask	    = 0x1,
};

static struct clk bgm_clk = {
	.name               = "bgm_clk",
	.flag               = CLOCK_NORESET,
	.parent             = {&pll0p_clk},
	.prediv             = 1,
	.div_reg_offset     = REG_PMU_CLK_DIV0,
	.div_reg_mask       = 0xf00,
	.en_reg_offset      = REG_PMU_CLK_GATE,
	.en_reg_mask        = 0x40000,
};

static struct clk mipi_dphy_clk = {
	.name               = "mipi_dphy_clk",
	.flag               = CLOCK_NORESET|CLOCK_NODIV,
	.parent             = {&osc_clk},
	.prediv             = 1,
	.en_reg_offset      = REG_PMU_CLK_GATE,
	.en_reg_mask        = 0x100000,
};

static struct clk pix_clk = {
	.name               = "pix_clk",
	.flag               = CLOCK_NORESET,
	.parent             = {&pll0p_clk},
	.prediv             = 1,
	.div_reg_offset     = REG_PMU_CLK_DIV2,
	.div_reg_mask       = 0xf000000,
	.en_reg_offset      = REG_PMU_CLK_GATE,
	.en_reg_mask        = 0x400000,
};

static struct clk pae_clk = {
	.name               = "pae_clk",
	.flag               = CLOCK_NORESET|CLOCK_MULTI_PARENT|CLOCK_HIDE,
	.parent             = {&pll0p_clk, &pll2p_clk},
	.prediv             = 1,
	.div_reg_offset     = REG_PMU_CLK_DIV0,
	.div_reg_mask       = 0x7000000,
	.sel_reg_offset     = REG_PMU_CLK_SEL,
	.sel_reg_mask       = 0x2,
	.en_reg_offset      = REG_PMU_CLK_GATE,
	.en_reg_mask        = 0x10,
	.def_rate           = CONFIG_PAE_CLK_RATE,
};

static struct clk ddrc_a1clk = {
	.name               = "ddrc_a1clk",
	.flag               = CLOCK_NORESET|CLOCK_NODIV,
	.parent             = {&pae_clk},
	.prediv             = 1,
	.en_reg_offset      = REG_PMU_CLK_GATE,
	.en_reg_mask        = 0x20,
};

/*
 * pll2p
 */

static struct clk hevc_aclk = {
	.name               = "hevc_aclk",
	.flag               = CLOCK_MULTI_PARENT|CLOCK_HIDE,
	.parent             = {&pll0p_clk, &pll2p_clk},
	.prediv             = 1,
	.div_reg_offset     = REG_PMU_CLK_DIV7,
	.div_reg_mask       = 0x700,
	.sel_reg_offset     = REG_PMU_CLK_SEL,
	.sel_reg_mask       = 0x1,
	.en_reg_offset      = REG_PMU_CLK_GATE,
	.en_reg_mask        = 0x20000000,
	.rst_reg_offset     = REG_PMU_SWRST_MAIN_CTRL,
	.rst_reg_mask       = 0x1000000,
	.def_rate           = CONFIG_HEVC_CLK_RATE,
};

static struct clk hevc_bclk = {
	.name               = "hevc_bclk",
	.flag               = CLOCK_MULTI_PARENT|CLOCK_HIDE,
	.parent             = {&pll0p_clk , &pll2p_clk},
	.prediv             = 1,
	.div_reg_offset     = REG_PMU_CLK_DIV7,
	.div_reg_mask       = 0x70000,
	.sel_reg_offset     = REG_PMU_CLK_SEL,
	.sel_reg_mask       = 0x1,
	.en_reg_offset      = REG_PMU_CLK_GATE,
	.en_reg_mask        = 0x40000000,
	.rst_reg_offset     = REG_PMU_SWRST_MAIN_CTRL,
	.rst_reg_mask       = 0x2000000,
	.def_rate           = CONFIG_HEVC_CLK_RATE,
};

static struct clk hevc_cclk = {
	.name               = "hevc_cclk",
	.flag               = CLOCK_MULTI_PARENT|CLOCK_HIDE,
	.parent             = {&pll0p_clk, &pll2p_clk},
	.prediv             = 1,
	.div_reg_offset     = REG_PMU_CLK_DIV7,
	.div_reg_mask       = 0x7000000,
	.sel_reg_offset     = REG_PMU_CLK_SEL,
	.sel_reg_mask       = 0x1,
	.en_reg_offset      = REG_PMU_CLK_GATE,
	.en_reg_mask        = 0x80000000,
	.rst_reg_offset     = REG_PMU_SWRST_MAIN_CTRL,
	.rst_reg_mask       = 0x4000000,
	.def_rate           = CONFIG_HEVC_CLK_RATE,
};


static struct clk pll0_div12_dw_clk = {
	.name               = "pll0_div12_dw_clk",
	.flag               = CLOCK_NORESET|CLOCK_NOGATE,
	.parent             = {&pll0p_clk},
	.prediv             = 1,
	.div_reg_offset     = REG_PMU_CLK_DIV5,
	.div_reg_mask       = 0xf000000,
};

static struct clk sdc0_clk = {
	.name               = "sdc0_clk",
	.parent             = {&pll0_div12_dw_clk},
	.prediv             = 2,
	.div_reg_offset     = REG_PMU_CLK_DIV3,
	.div_reg_mask       = 0xf00,
	.en_reg_offset      = REG_PMU_CLK_GATE,
	.en_reg_mask        = 0x200,
	.rst_reg_offset     = REG_PMU_SWRST_AHB_CTRL,
	.rst_reg_mask       = 0x4,
};

static struct clk sdc1_clk = {
	.name               = "sdc1_clk",
	.parent             = {&pll0_div12_dw_clk},
	.prediv             = 2,
	.div_reg_offset     = REG_PMU_CLK_DIV3,
	.div_reg_mask       = 0xf000000,
	.en_reg_offset      = REG_PMU_CLK_GATE,
	.en_reg_mask        = 0x400,
	.rst_reg_offset     = REG_PMU_SWRST_AHB_CTRL,
	.rst_reg_mask       = 0x2,
};

static struct clk spi0_clk = {
	.name               = "spi0_clk",
	.parent             = {&pll0_div12_dw_clk},
	.prediv             = 1,
	.div_reg_offset     = REG_PMU_CLK_DIV3,
	.div_reg_mask       = 0xff,
	.en_reg_offset      = REG_PMU_CLK_GATE,
	.en_reg_mask        = 0x80,
	.rst_reg_offset     = REG_PMU_SWRST_MAIN_CTRL,
	.rst_reg_mask       = 0x100,
};

static struct clk spi1_clk = {
	.name               = "spi1_clk",
	.parent             = {&pll0_div12_dw_clk},
	.prediv             = 1,
	.div_reg_offset     = REG_PMU_CLK_DIV3,
	.div_reg_mask       = 0xff0000,
	.en_reg_offset      = REG_PMU_CLK_GATE,
	.en_reg_mask        = 0x100,
	.rst_reg_offset     = REG_PMU_SWRST_MAIN_CTRL,
	.rst_reg_mask       = 0x200,
};

static struct clk spi2_clk = {
	.name               = "spi2_clk",
	.parent             = {&pll0_div12_dw_clk},
	.prediv             = 1,
	.div_reg_offset     = REG_PMU_CLK_DIV3,
	.div_reg_mask       = 0xf000,
	.en_reg_offset      = REG_PMU_CLK_GATE,
	.en_reg_mask        = 0x2,
	.rst_reg_offset     = REG_PMU_SWRST_MAIN_CTRL,
	.rst_reg_mask       = 0x100000,
};

static struct clk eth_clk = {
	.name               = "eth_clk",
	.parent             = {&pll0_div12_dw_clk},
	.prediv             = 1,
	.div_reg_offset     = REG_PMU_CLK_DIV6,
	.div_reg_mask       = 0xf000000,
	.en_reg_offset      = REG_PMU_CLK_GATE,
	.en_reg_mask        = 0x12000000,
	.rst_reg_offset     = REG_PMU_SWRST_AHB_CTRL,
	.rst_reg_mask       = 0x20000,
};

static struct clk i2c0_clk = {
	.name               = "i2c0_clk",
	.parent             = {&pll0_div12_dw_clk},
	.prediv             = 1,
	.div_reg_offset     = REG_PMU_CLK_DIV4,
	.div_reg_mask       = 0x3f0000,
	.en_reg_offset      = REG_PMU_CLK_GATE,
	.en_reg_mask        = 0x1000,
	.rst_reg_offset     = REG_PMU_SWRST_MAIN_CTRL,
	.rst_reg_mask       = 0x400,
};

static struct clk i2c1_clk = {
	.name               = "i2c1_clk",
	.parent             = {&pll0_div12_dw_clk},
	.prediv             = 1,
	.div_reg_offset     = REG_PMU_CLK_DIV4,
	.div_reg_mask       = 0x3f000000,
	.en_reg_offset      = REG_PMU_CLK_GATE,
	.en_reg_mask        = 0x8000000,
	.rst_reg_offset     = REG_PMU_SWRST_MAIN_CTRL,
	.rst_reg_mask       = 0x800,
};

static struct clk pwm_clk = {
	.name               = "pwm_clk",
	.parent             = {&pll0_div12_dw_clk},
	.prediv             = 1,
	.div_reg_offset     = REG_PMU_CLK_DIV5,
	.div_reg_mask       = 0xff,
	.en_reg_offset      = REG_PMU_CLK_GATE,
	.en_reg_mask        = 0x10000,
	.rst_reg_offset     = REG_PMU_SWRST_MAIN_CTRL,
	.rst_reg_mask       = 0x80,
	.def_rate       = 50000000,
};

static struct clk uart0_clk = {
	.name               = "uart0_clk",
	.parent             = {&pll0_div12_dw_clk},
	.prediv             = 1,
	.div_reg_offset     = REG_PMU_CLK_DIV4,
	.div_reg_mask       = 0x1f,
	.en_reg_offset      = REG_PMU_CLK_GATE,
	.en_reg_mask        = 0x2000,
	.rst_reg_offset     = REG_PMU_SWRST_MAIN_CTRL,
	.rst_reg_mask       = 0x4000,
	.def_rate           = 16666666,
};

static struct clk uart1_clk = {
	.name               = "uart1_clk",
	.parent             = {&pll0_div12_dw_clk},
	.prediv             = 1,
	.div_reg_offset     = REG_PMU_CLK_DIV4,
	.div_reg_mask       = 0x1f00,
	.en_reg_offset      = REG_PMU_CLK_GATE,
	.en_reg_mask        = 0x4000,
	.rst_reg_offset     = REG_PMU_SWRST_MAIN_CTRL,
	.rst_reg_mask       = 0x8000,
	.def_rate           = 16666666,
};

static struct clk pts_clk = {
	.name               = "pts_clk",
	.parent             = {&pll0_div12_dw_clk},
	.prediv             = 1,
	.div_reg_offset     = REG_PMU_CLK_DIV2,
	.div_reg_mask       = 0x1ff,
	.en_reg_offset      = REG_PMU_CLK_GATE,
	.en_reg_mask        = 0x80000,
	.rst_reg_offset     = REG_PMU_SWRST_MAIN_CTRL,
	.rst_reg_mask       = 0x20000,
	.def_rate           = 1000000,
};

static struct clk efuse_clk = {
	.name               = "efuse_clk",
	.parent             = {&pll0_div12_dw_clk},
	.prediv             = 1,
	.div_reg_offset     = REG_PMU_CLK_DIV1,
	.div_reg_mask       = 0x3f000000,
	.en_reg_offset      = REG_PMU_CLK_GATE,
	.en_reg_mask        = 0x200000,
	.rst_reg_offset     = REG_PMU_SWRST_MAIN_CTRL,
	.rst_reg_mask       = 0x800000,
};

static struct clk tmr0_clk = {
	.name               = "tmr0_clk",
	.parent             = {&pll0_div12_dw_clk},
	.prediv             = 1,
	.div_reg_offset     = REG_PMU_CLK_DIV5,
	.div_reg_mask       = 0xff0000,
	.en_reg_offset      = REG_PMU_CLK_GATE,
	.en_reg_mask        = 0x20000,
	.rst_reg_offset     = REG_PMU_SWRST_MAIN_CTRL,
	.rst_reg_mask       = 0x40000,
};

static struct clk sadc_clk = {
	.name               = "sadc_clk",
	.parent             = {&pll0_div12_dw_clk},
	.prediv             = 1,
	.div_reg_offset     = REG_PMU_CLK_DIV6,
	.div_reg_mask       = 0x7f0000,
	.en_reg_offset      = REG_PMU_CLK_GATE,
	.en_reg_mask        = 0x4000000,
	.rst_reg_offset     = REG_PMU_SWRST_MAIN_CTRL,
	.rst_reg_mask       = 0x10000,
};

static struct clk ac_clk = {
	.name               = "ac_clk",
	.parent             = {&osc_clk},
	.prediv             = 1,
	.div_reg_offset     = REG_PMU_CLK_DIV6,
	.div_reg_mask       = 0x3f,
	.en_reg_offset      = REG_PMU_CLK_GATE,
	.en_reg_mask        = 0x800,
	.rst_reg_offset     = REG_PMU_SWRST_MAIN_CTRL,
	.rst_reg_mask       = 0x1000,
};

static struct clk i2s_clk = {
	.name               = "i2s_clk",
	.parent             = {&ac_clk},
	.prediv             = 1,
	.div_reg_offset     = REG_PMU_CLK_DIV6,
	.div_reg_mask       = 0x3f00,
	.en_reg_offset      = REG_PMU_CLK_GATE,
	.en_reg_mask        = 0x1000000,
	.rst_reg_offset     = REG_PMU_SWRST_MAIN_CTRL,
	.rst_reg_mask       = 0x2000,
};

static struct clk wdt_clk = {
	.name               = "wdt_clk",
	.flag               = CLOCK_NOGATE,
	.parent             = {&ahb_clk},
	.prediv             = 1,
	.div_reg_offset     = REG_PMU_CLK_DIV5,
	.div_reg_mask       = 0xff00,
	.rst_reg_offset     = REG_PMU_SWRST_APB_CTRL,
	.rst_reg_mask       = 0x100000,
	.def_rate           = 1000000,
};

struct clk_lookup fh_clks[] = {
	CLK(NULL, "osc_clk", &osc_clk),
	CLK(NULL, "pll0p_clk", &pll0p_clk),
	CLK(NULL, "pll0r_clk", &pll0r_clk),
	CLK(NULL, "pll1r_clk", &pll1r_clk),
	CLK(NULL, "pll2p_clk", &pll2p_clk),

	CLK(NULL, "arm_clk", &arm_clk),
	CLK(NULL, "arc_clk", &arc_clk),
	CLK(NULL, "pae_arc_clk", &pae_arc_clk),
	CLK(NULL, "ahb_clk", &ahb_clk),

	CLK(NULL, "ddr_clk", &ddr_clk),
	CLK(NULL, "isp_aclk", &isp_aclk),
	CLK(NULL, "jpeg_clk", &jpeg_clk),
	CLK(NULL, "vpu_clk", &vpu_clk),
	CLK(NULL, "pae_clk", &pae_clk),
	CLK(NULL, "bgm_clk", &bgm_clk),
	CLK(NULL, "mipi_dphy_clk", &mipi_dphy_clk),
	CLK(NULL, "ddrc_a1clk", &ddrc_a1clk),

	CLK(NULL, "hevc_aclk", &hevc_aclk),
	CLK(NULL, "hevc_bclk", &hevc_bclk),
	CLK(NULL, "hevc_cclk", &hevc_cclk),
	CLK(NULL, "pll0_div12_dw_clk", &pll0_div12_dw_clk),

	CLK(NULL, "cis_clk_out", &cis_clk_out),
	CLK(NULL, "pix_clk", &pix_clk),
	CLK(NULL, "pts_clk", &pts_clk),

	CLK(NULL, "spi0_clk", &spi0_clk),
	CLK(NULL, "spi1_clk", &spi1_clk),
	CLK(NULL, "spi2_clk", &spi2_clk),
	CLK(NULL, "sdc0_clk", &sdc0_clk),
	CLK(NULL, "sdc1_clk", &sdc1_clk),
	CLK(NULL, "uart0_clk", &uart0_clk),
	CLK(NULL, "uart1_clk", &uart1_clk),
	CLK(NULL, "i2c0_clk", &i2c0_clk),
	CLK(NULL, "i2c1_clk", &i2c1_clk),
	CLK(NULL, "pwm_clk", &pwm_clk),
	CLK(NULL, "wdt_clk", &wdt_clk),
	CLK(NULL, "tmr0_clk", &tmr0_clk),
	CLK(NULL, "ac_clk", &ac_clk),
	CLK(NULL, "i2s_clk", &i2s_clk),
	CLK(NULL, "sadc_clk", &sadc_clk),
	CLK(NULL, "eth_clk", &eth_clk),
	CLK(NULL, "efuse_clk", &efuse_clk),

	CLK(NULL, NULL, NULL),
};
EXPORT_SYMBOL(fh_clks);
