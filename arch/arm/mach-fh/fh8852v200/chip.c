#include <linux/init.h>
#include <linux/clk.h>
#include <linux/platform_device.h>
#include <linux/dma-mapping.h>
#include <linux/gpio.h>
#include <linux/spi/spi.h>
#include <asm/mach/map.h>
#include <mach/pmu.h>
#include <mach/clock.h>
#include <mach/board_config.h>

/*
 * external oscillator
 * fixed to 24M
 */
static struct fh_clk osc_clk = {
	.name               = "osc_clk",
	.frequency          = OSC_FREQUENCY,
	.flag               = CLOCK_FIXED,
};

/*
 * phase-locked-loop device,
 * generates a higher frequency clock
 * from the external oscillator reference
 *PLL_DDR
 */

static struct fh_clk pll_ddr_rclk = {
	.name               = "pll_ddr_rclk",
	.flag               = CLOCK_PLL_R|CLOCK_NOGATE,
	.parent             = {&osc_clk},
	.div_reg_offset     = REG_PMU_PLL0,
	.en_reg_offset      = REG_PMU_PLL0_CTRL,
	.en_reg_mask        = 0xf000,
};

/*PLL_CPU*/
static struct fh_clk pll_cpu_pclk = {
	.name               = "pll_cpu_pclk",
	.flag               = CLOCK_PLL_P|CLOCK_NOGATE,
	.parent             = {&osc_clk},
	.div_reg_offset     = REG_PMU_PLL1,
	.en_reg_offset      = REG_PMU_PLL1_CTRL,
	.en_reg_mask        = 0xf00,
};

static struct fh_clk pll_cpu_rclk = {
	.name               = "pll_cpu_rclk",
	.flag               = CLOCK_PLL_R|CLOCK_NOGATE,
	.parent             = {&osc_clk},
	.div_reg_offset     = REG_PMU_PLL1,
	.en_reg_offset      = REG_PMU_PLL1_CTRL,
	.en_reg_mask        = 0xf000,
};

/*PLL_SYS*/
static struct fh_clk pll_sys_pclk = {
	.name               = "pll_sys_pclk",
	.flag               = CLOCK_PLL_P|CLOCK_NOGATE,
	.parent             = {&osc_clk},
	.div_reg_offset     = REG_PMU_PLL2,
	.en_reg_offset      = REG_PMU_PLL2_CTRL,
	.en_reg_mask        = 0xf00,
};


static struct fh_clk pll_sys_rclk = {
	.name               = "pll_sys_rclk",
	.flag               = CLOCK_PLL_R|CLOCK_NOGATE,
	.parent             = {&osc_clk},
	.div_reg_offset     = REG_PMU_PLL2,
	.en_reg_offset      = REG_PMU_PLL2_CTRL,
	.en_reg_mask        = 0xf000,
};

static struct fh_clk pllsysp_div12_clk = {
	.name               = "pllsysp_div12_clk",
	.flag               = CLOCK_NORESET|CLOCK_NOGATE,
	.parent             = {&pll_sys_pclk},
	.prediv             = 1,
	.div_reg_offset     = REG_PMU_CLK_DIV5,
	.div_reg_mask       = 0xf000000,
};

static struct fh_clk ddr_clk = {
	.name               = "ddr_clk",
	.flag               = CLOCK_NODIV,
	.parent             = {&pll_ddr_rclk},
	.prediv             = 1,
	.en_reg_offset      = REG_PMU_CLK_GATE1,
	.en_reg_mask        = 0x4000000,
	.rst_reg_offset     = REG_PMU_SWRST_MAIN_CTRL,
	.rst_reg_mask       = 0x8,
};
static struct fh_clk arm_clk = {
	.name               = "arm_clk",
	.flag               = CLOCK_MULTI_PARENT|CLOCK_NOGATE|CLOCK_NODIV,
	.parent             = {&osc_clk, &pll_cpu_pclk},
	.prediv             = 1,
	.sel_reg_offset     = REG_PMU_SYS_CTRL,
	.sel_reg_mask       = 0x1,
	.rst_reg_offset     = REG_PMU_SWRST_MAIN_CTRL,
	.rst_reg_mask       = 0x1,
};
static struct fh_clk arc_clk = {
	.name               = "arc_clk",
	.flag               = CLOCK_MULTI_PARENT|CLOCK_NODIV,
	.parent             = {&osc_clk, &pll_cpu_rclk},
	.prediv             = 1,
	.sel_reg_offset     = REG_PMU_SYS_CTRL,
	.sel_reg_mask       = 0x1,
	.en_reg_offset      = REG_PMU_CLK_GATE,
	.en_reg_mask        = 0x400000,
	.rst_reg_offset     = REG_PMU_SWRST_MAIN_CTRL,
	.rst_reg_mask       = 0x400000,
};
static struct fh_clk ahb_clk = {
	.name               = "ahb_clk",
	.flag               = CLOCK_MULTI_PARENT|CLOCK_NORESET|CLOCK_NOGATE,
	.parent             = {&osc_clk, &pll_sys_pclk},
	.prediv             = 1,
	.sel_reg_offset     = REG_PMU_SYS_CTRL,
	.sel_reg_mask       = 0x1,
	.div_reg_offset     = REG_PMU_CLK_DIV0,
	.div_reg_mask       = 0xf0000,
};

static struct fh_clk isp_aclk = {
	.name               = "isp_aclk",
	.flag               = CLOCK_NORESET,
	.parent             = {&pll_sys_pclk},
	.prediv             = 1,
	.div_reg_offset     = REG_PMU_CLK_DIV0,
	.div_reg_mask       = 0xf00,
	.en_reg_offset      = REG_PMU_CLK_GATE,
	.en_reg_mask        = 0x1,
	.def_rate           = CONFIG_ISP_CLK_RATE,
};
static struct fh_clk ispb_aclk = {
	.name               = "ispb_aclk",
	.flag               = CLOCK_NORESET|CLOCK_NODIV,
	.parent             = {&isp_aclk},
	.prediv             = 1,
	.en_reg_offset      = REG_PMU_CLK_GATE,
	.en_reg_mask        = 0x4,
};

static struct fh_clk vpu_clk = {
	.name               = "vpu_clk",
	.flag               = CLOCK_NORESET|CLOCK_NODIV,
	.parent             = {&isp_aclk},
	.prediv             = 1,
	.en_reg_offset	    = REG_PMU_CLK_GATE1,
	.en_reg_mask	    = 0x80000000,
};

static struct fh_clk pix_clk = {
	.name               = "pix_clk",
	.flag               = CLOCK_NORESET|CLOCK_NOGATE,
	.parent             = {&pll_sys_pclk},
	.prediv             = 1,
	.div_reg_offset     = REG_PMU_CLK_DIV2,
	.div_reg_mask       = 0xf000000,
};

static struct fh_clk jpeg_clk = {
	.name               = "jpeg_clk",
	.flag               = CLOCK_NORESET,
	.parent             = {&pll_sys_pclk},
	.prediv             = 1,
	.div_reg_offset     = REG_PMU_CLK_DIV7,
	.div_reg_mask       = 0xf00000,
	.en_reg_offset      = REG_PMU_CLK_GATE1,
	.en_reg_mask        = 0x40000000,
	.def_rate           = CONFIG_JPEG_CLK_RATE,
};

static struct fh_clk bgm_clk = {
	.name               = "bgm_clk",
	.flag               = CLOCK_NORESET,
	.parent             = {&pll_sys_pclk},
	.prediv				= 1,
	.div_reg_offset		= REG_PMU_CLK_DIV7,
	.div_reg_mask		= 0xf00000,
	.en_reg_offset      = REG_PMU_CLK_GATE,
	.en_reg_mask        = 0x40000,
};

static struct fh_clk jpeg_adapt_clk = {
	.name               = "jpeg_adapt_clk",
	.flag               = CLOCK_NORESET|CLOCK_NODIV,
	.parent             = {&jpeg_clk},
	.prediv             = 1,
	.en_reg_offset      = REG_PMU_CLK_GATE1,
	.en_reg_mask        = 0x2,
};
static struct fh_clk spi0_clk = {
	.name               = "spi0_clk",
	.parent             = {&pll_sys_pclk},
	.prediv             = 1,
	.div_reg_offset     = REG_PMU_CLK_DIV3,
	.div_reg_mask       = 0xff,
	.en_reg_offset      = REG_PMU_CLK_GATE,
	.en_reg_mask        = 0x80,
	.rst_reg_offset     = REG_PMU_SWRST_MAIN_CTRL,
	.rst_reg_mask       = 0x100,
};
static struct fh_clk sdc0_clk = {
	.name               = "sdc0_clk",
	.parent             = {&pll_sys_pclk},
	.prediv             = 8,
	.div_reg_offset     = REG_PMU_CLK_DIV3,
	.div_reg_mask       = 0xf00,
	.en_reg_offset      = REG_PMU_CLK_GATE,
	.en_reg_mask        = 0x200,
	.rst_reg_offset     = REG_PMU_SWRST_AHB_CTRL,
	.rst_reg_mask       = 0x4,
};
static struct fh_clk spi2_clk = {
	.name               = "spi2_clk",
	.parent             = {&pll_sys_pclk},
	.prediv             = 1,
	.div_reg_offset     = REG_PMU_CLK_DIV3,
	.div_reg_mask       = 0xf000,
	.en_reg_offset      = REG_PMU_CLK_GATE,
	.en_reg_mask        = 0x2,
	.rst_reg_offset     = REG_PMU_SWRST_MAIN_CTRL,
	.rst_reg_mask       = 0x100000,
};
static struct fh_clk spi1_clk = {
	.name               = "spi1_clk",
	.parent             = {&pll_sys_pclk},
	.prediv             = 1,
	.div_reg_offset     = REG_PMU_CLK_DIV3,
	.div_reg_mask       = 0xff0000,
	.en_reg_offset      = REG_PMU_CLK_GATE,
	.en_reg_mask        = 0x100,
	.rst_reg_offset     = REG_PMU_SWRST_MAIN_CTRL,
	.rst_reg_mask       = 0x200,
};
static struct fh_clk sdc1_clk = {
	.name               = "sdc1_clk",
	.parent             = {&pll_sys_pclk},
	.prediv             = 8,
	.div_reg_offset     = REG_PMU_CLK_DIV3,
	.div_reg_mask       = 0xf000000,
	.en_reg_offset      = REG_PMU_CLK_GATE,
	.en_reg_mask        = 0x400,
	.rst_reg_offset     = REG_PMU_SWRST_AHB_CTRL,
	.rst_reg_mask       = 0x2,
};

static struct fh_clk veu_clk = {
	.name               = "veu_clk",
	.flag               = CLOCK_MULTI_PARENT,
	.parent             = {&pll_sys_pclk, &pll_sys_rclk},
	.prediv             = 1,
	.sel_reg_offset		= REG_PMU_SYS_CTRL,
	.sel_reg_mask		= 0x4,
	.div_reg_offset     = REG_PMU_CLK_DIV0,
	.div_reg_mask       = 0x7000000,
	.en_reg_offset      = REG_PMU_CLK_GATE,
	.en_reg_mask        = 0x10,
	.rst_reg_offset		= REG_PMU_SWRST_MAIN_CTRL,
	.rst_reg_mask		= 0x2000000,
	.def_rate           = CONFIG_VEU_CLK_RATE,

};

static struct fh_clk veu_adapt_clk = {
	.name               = "veu_adapt_clk",
	.flag               = CLOCK_NORESET|CLOCK_NODIV,
	.parent             = {&veu_clk},
	.prediv             = 1,
	.en_reg_offset      = REG_PMU_CLK_GATE,
	.en_reg_mask        = 0x20000000,

};

static struct fh_clk cis_clk_out = {
	.name               = "cis_clk_out",
	.flag               = CLOCK_NORESET,
	.parent             = {&pll_sys_pclk},
	.prediv             = 1,
	.div_reg_offset     = REG_PMU_CLK_DIV1,
	.div_reg_mask       = 0xff0000,
	.en_reg_offset      = REG_PMU_CLK_GATE,
	.en_reg_mask        = 0x800000,
};

static struct fh_clk eth_clk = {
	.name               = "eth_clk",
	.parent             = {&pllsysp_div12_clk},
	.prediv             = 1,
	.div_reg_offset     = REG_PMU_CLK_DIV6,
	.div_reg_mask       = 0xf000000,
	.en_reg_offset      = REG_PMU_CLK_GATE,
	.en_reg_mask        = 0x12000000,
	.rst_reg_offset     = REG_PMU_SWRST_AHB_CTRL,
	.rst_reg_mask       = 0x20000,
};
static struct fh_clk i2c0_clk = {
	.name               = "i2c0_clk",
	.parent             = {&pllsysp_div12_clk},
	.prediv             = 1,
	.div_reg_offset     = REG_PMU_CLK_DIV4,
	.div_reg_mask       = 0x3f0000,
	.en_reg_offset      = REG_PMU_CLK_GATE,
	.en_reg_mask        = 0x1000,
	.rst_reg_offset     = REG_PMU_SWRST_MAIN_CTRL,
	.rst_reg_mask       = 0x400,
};

static struct fh_clk i2c1_clk = {
	.name               = "i2c1_clk",
	.parent             = {&pllsysp_div12_clk},
	.prediv             = 1,
	.div_reg_offset     = REG_PMU_CLK_DIV4,
	.div_reg_mask       = 0x3f000000,
	.en_reg_offset      = REG_PMU_CLK_GATE,
	.en_reg_mask        = 0x8000000,
	.rst_reg_offset     = REG_PMU_SWRST_MAIN_CTRL,
	.rst_reg_mask       = 0x800,
};

static struct fh_clk i2c2_clk = {
	.name               = "i2c2_clk",
	.parent             = {&pllsysp_div12_clk},
	.prediv             = 1,
	.div_reg_offset     = REG_PMU_CLK_DIV7,
	.div_reg_mask       = 0x00003f00,
	.en_reg_offset      = REG_PMU_CLK_GATE,
	.en_reg_mask        = 0x00000008,
	.rst_reg_offset     = REG_PMU_SWRST_MAIN_CTRL,
	.rst_reg_mask       = 0x20000000,
};

static struct fh_clk pwm_clk = {
	.name               = "pwm_clk",
	.parent             = {&pllsysp_div12_clk},
	.prediv             = 1,
	.div_reg_offset     = REG_PMU_CLK_DIV5,
	.div_reg_mask       = 0xff,
	.en_reg_offset      = REG_PMU_CLK_GATE,
	.en_reg_mask        = 0x10000,
	.rst_reg_offset     = REG_PMU_SWRST_MAIN_CTRL,
	.rst_reg_mask       = 0x80,
	.def_rate       = 50000000,
};

static struct fh_clk uart0_clk = {
	.name               = "uart0_clk",
	.parent             = {&pllsysp_div12_clk},
	.prediv             = 1,
	.div_reg_offset     = REG_PMU_CLK_DIV4,
	.div_reg_mask       = 0x1f,
	.en_reg_offset      = REG_PMU_CLK_GATE,
	.en_reg_mask        = 0x2000,
	.rst_reg_offset     = REG_PMU_SWRST_MAIN_CTRL,
	.rst_reg_mask       = 0x4000,
	.def_rate           = 16666666,
};

static struct fh_clk uart1_clk = {
	.name               = "uart1_clk",
	.parent             = {&pllsysp_div12_clk},
	.prediv             = 1,
	.div_reg_offset     = REG_PMU_CLK_DIV4,
	.div_reg_mask       = 0x1f00,
	.en_reg_offset      = REG_PMU_CLK_GATE,
	.en_reg_mask        = 0x4000,
	.rst_reg_offset     = REG_PMU_SWRST_MAIN_CTRL,
	.rst_reg_mask       = 0x8000,
	.def_rate           = 16666666,
};
static struct fh_clk uart2_clk = {
	.name               = "uart2_clk",
	.parent             = {&pllsysp_div12_clk},
	.flag               = 0,
	.prediv             = 1,
	.div_reg_offset     = REG_PMU_CLK_DIV7,
	.div_reg_mask       = 0x7f,
	.en_reg_offset      = REG_PMU_CLK_GATE,
	.en_reg_mask        = 0x8000,
	.rst_reg_offset     = REG_PMU_SWRST_MAIN_CTRL,
	.rst_reg_mask       = 0x8000000,
	.def_rate           = 16666666,
};

static struct fh_clk efuse_clk = {
	.name               = "efuse_clk",
	.parent             = {&pllsysp_div12_clk},
	.prediv             = 1,
	.div_reg_offset     = REG_PMU_CLK_DIV1,
	.div_reg_mask       = 0x3f000000,
	.en_reg_offset      = REG_PMU_CLK_GATE,
	.en_reg_mask        = 0x200000,
	.rst_reg_offset     = REG_PMU_SWRST_MAIN_CTRL,
	.rst_reg_mask       = 0x800000,
};

static struct fh_clk pts_clk = {
	.name               = "pts_clk",
	.parent             = {&pllsysp_div12_clk},
	.flag               = CLOCK_NORESET,
	.prediv             = 1,
	.div_reg_offset     = REG_PMU_CLK_DIV2,
	.div_reg_mask       = 0x1ff,
	.en_reg_offset      = REG_PMU_CLK_GATE,
	.en_reg_mask        = 0x80000,
	.rst_reg_offset     = REG_PMU_SWRST_MAIN_CTRL2,
	.rst_reg_mask       = 0x1,
	.def_rate           = 1000000,
};

static struct fh_clk tmr0_clk = {
	.name               = "tmr0_clk",
	.parent             = {&pllsysp_div12_clk},
	.prediv             = 1,
	.div_reg_offset     = REG_PMU_CLK_DIV5,
	.div_reg_mask       = 0xff0000,
	.en_reg_offset      = REG_PMU_CLK_GATE,
	.en_reg_mask        = 0x20000,
	.rst_reg_offset     = REG_PMU_SWRST_MAIN_CTRL,
	.rst_reg_mask       = 0x40000,
};

static struct fh_clk sadc_clk = {
	.name               = "sadc_clk",
	.parent             = {&pllsysp_div12_clk},
	.prediv             = 1,
	.div_reg_offset     = REG_PMU_CLK_DIV6,
	.div_reg_mask       = 0x7f0000,
	.en_reg_offset      = REG_PMU_CLK_GATE,
	.en_reg_mask        = 0x4000000,
	.rst_reg_offset     = REG_PMU_SWRST_MAIN_CTRL,
	.rst_reg_mask       = 0x10000,
};

static struct fh_clk ac_clk = {
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

static struct fh_clk i2s_clk = {
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

static struct fh_clk wdt_clk = {
	.name               = "wdt_clk",
	.flag               = 0,
	.parent             = {&ahb_clk},
	.prediv             = 1,
	.div_reg_offset     = REG_PMU_CLK_DIV5,
	.div_reg_mask       = 0xff00,
	.en_reg_offset      = REG_PMU_CLK_GATE1,
	.en_reg_mask        = 0x8000000,
	.rst_reg_offset     = REG_PMU_SWRST_APB_CTRL,
	.rst_reg_mask       = 0x100000,
	.def_rate           = 1000000,
};

static struct fh_clk gpio0_db_clk = {
	.name               = "gpio0_db_clk",
	.flag               = 0,
	.parent             = {&pllsysp_div12_clk},
	.prediv             = 100,
	.div_reg_offset     = REG_PMU_CLK_DIV8,
	.div_reg_mask       = 0x7fff,
	.en_reg_offset      = REG_PMU_CLK_DIV8,
	.en_reg_mask        = 0x8000,
	.rst_reg_offset     = REG_PMU_SWRST_MAIN_CTRL,
	.rst_reg_mask       = 0x10,
};

static struct fh_clk gpio1_db_clk = {
	.name               = "gpio1_db_clk",
	.flag               = 0,
	.parent             = {&pllsysp_div12_clk},
	.prediv             = 100,
	.div_reg_offset     = REG_PMU_CLK_DIV8,
	.div_reg_mask       = 0x7fff0000,
	.en_reg_offset      = REG_PMU_CLK_DIV8,
	.en_reg_mask        = 0x80000000,
	.rst_reg_offset     = REG_PMU_SWRST_MAIN_CTRL,
	.rst_reg_mask       = 0x20,
};


static struct fh_clk mipi_dphy_clk = {
	.name               = "mipi_dphy_clk",
	.flag               = CLOCK_NORESET|CLOCK_NODIV,
	.parent             = {&osc_clk},
	.prediv             = 1,
	.en_reg_offset      = REG_PMU_CLK_GATE,
	.en_reg_mask        = 0x100000,
};
static struct fh_clk mipi_wrap_gate = {
	.name               = "mipi_wrap_gate",
	.flag               = CLOCK_NORESET|CLOCK_NODIV,
	.prediv             = 1,
	.en_reg_offset      = REG_PMU_CLK_GATE1,
	.en_reg_mask        = 0x20000000,
};
static struct fh_clk rtc_hclk_gate = {
	.name               = "rtc_hclk_gate",
	.flag               = CLOCK_NORESET|CLOCK_NODIV,
	.prediv             = 1,
	.en_reg_offset      = REG_PMU_CLK_GATE1,
	.en_reg_mask        = 0x10000000,
};
static struct fh_clk emac_hclk_gate = {
	.name               = "emac_hclk_gate",
	.flag               = CLOCK_NORESET|CLOCK_NODIV,
	.prediv             = 1,
	.en_reg_offset      = REG_PMU_CLK_GATE1,
	.en_reg_mask        = 0x2000000,
};
static struct fh_clk usb_clk = {
	.name               = "usb_clk",
	.flag               = CLOCK_NORESET|CLOCK_NODIV,
	.prediv             = 1,
	.en_reg_offset      = REG_PMU_CLK_GATE1,
	.en_reg_mask        = 0x1000000,
};
static struct fh_clk aes_hclk_gate = {
	.name               = "aes_hclk_gate",
	.flag               = CLOCK_NORESET|CLOCK_NODIV,
	.prediv             = 1,
	.en_reg_offset      = REG_PMU_CLK_GATE1,
	.en_reg_mask        = 0x80,
};
static struct fh_clk ephy_clk_gate = {
	.name               = "ephy_clk_gate",
	.flag               = CLOCK_NORESET|CLOCK_NODIV,
	.prediv             = 1,
	.en_reg_offset      = REG_PMU_CLK_GATE1,
	.en_reg_mask        = 0x1,
};
static struct fh_clk sdc0_clk8x_gate = {
	.name               = "sdc0_clk8x_gate",
	.flag               = CLOCK_NORESET|CLOCK_NODIV,
	.prediv             = 1,
	.en_reg_offset      = REG_PMU_CLK_GATE1,
	.en_reg_mask        = 0x4,
};
static struct fh_clk sdc1_clk8x_gate = {
	.name               = "sdc1_clk8x_gate",
	.flag               = CLOCK_NORESET|CLOCK_NODIV,
	.prediv             = 1,
	.en_reg_offset      = REG_PMU_CLK_GATE1,
	.en_reg_mask        = 0x8,
};
static struct fh_clk mipic_pclk_gate = {
	.name               = "mipic_pclk_gate",
	.flag               = CLOCK_NORESET|CLOCK_NODIV,
	.prediv             = 1,
	.en_reg_offset      = REG_PMU_CLK_GATE1,
	.en_reg_mask        = 0x10,
};

static struct fh_clk gpio0_pclk_gate = {
	.name               = "gpio0_pclk_gate",
	.flag               = CLOCK_NORESET|CLOCK_NODIV,
	.prediv             = 1,
	.en_reg_offset      = REG_PMU_CLK_DIV8,
	.en_reg_mask        = 0x4000,
};
static struct fh_clk gpio1_pclk_gate = {
	.name               = "gpio1_pclk_gate",
	.flag               = CLOCK_NORESET|CLOCK_NODIV,
	.prediv             = 1,
	.en_reg_offset      = REG_PMU_CLK_DIV8,
	.en_reg_mask        = 0x40000000,
};
static struct fh_clk isp_hclk_gate = {
	.name               = "isp_hclk_gate",
	.flag               = CLOCK_NORESET|CLOCK_NODIV,
	.prediv             = 1,
	.en_reg_offset      = REG_PMU_CLK_DIV7,
	.en_reg_mask        = 0x1000000,
};
static struct fh_clk veu_hclk_gate = {
	.name               = "veu_hclk_gate",
	.flag               = CLOCK_NORESET|CLOCK_NODIV,
	.prediv             = 1,
	.en_reg_offset      = REG_PMU_CLK_DIV7,
	.en_reg_mask        = 0x2000000,
};
static struct fh_clk bgm_hclk_gate = {
	.name               = "bgm_hclk_gate",
	.flag               = CLOCK_NORESET|CLOCK_NODIV,
	.prediv             = 1,
	.en_reg_offset      = REG_PMU_CLK_DIV7,
	.en_reg_mask        = 0x4000000,
};
static struct fh_clk adapt_hclk_gate = {
	.name               = "adapt_hclk_gate",
	.flag               = CLOCK_NORESET|CLOCK_NODIV,
	.prediv             = 1,
	.en_reg_offset      = REG_PMU_CLK_DIV7,
	.en_reg_mask        = 0x8000000,
};
static struct fh_clk jpg_hclk_gate = {
	.name               = "jpg_hclk_gate",
	.flag               = CLOCK_NORESET|CLOCK_NODIV,
	.prediv             = 1,
	.en_reg_offset      = REG_PMU_CLK_DIV7,
	.en_reg_mask        = 0x10000000,
};
static struct fh_clk jpg_adapt_gate = {
	.name               = "jpg_adapt_gate",
	.flag               = CLOCK_NORESET|CLOCK_NODIV,
	.prediv             = 1,
	.en_reg_offset      = REG_PMU_CLK_DIV7,
	.en_reg_mask        = 0x20000000,
};
static struct fh_clk vpu_hclk_gate = {
	.name               = "vpu_hclk_gate",
	.flag               = CLOCK_NORESET|CLOCK_NODIV,
	.prediv             = 1,
	.en_reg_offset      = REG_PMU_CLK_DIV7,
	.en_reg_mask        = 0x40000000,
};

static struct fh_clk sdc0_clk_sample = {
	.name               = "sdc0_clk_sample",
	.parent             = {&sdc0_clk},
	.flag				= CLOCK_NOGATE | CLOCK_PHASE,
	.prediv             = 1,
	.sel_reg_offset     = REG_PMU_CLK_GATE1,
	.sel_reg_mask       = 0xf0000,
};

static struct fh_clk sdc0_clk_drv = {
	.name               = "sdc0_clk_drv",
	.parent             = {&sdc0_clk},
	.flag				= CLOCK_NOGATE | CLOCK_PHASE,
	.prediv             = 1,
	.sel_reg_offset     = REG_PMU_CLK_GATE1,
	.sel_reg_mask       = 0xf00000,
};

static struct fh_clk sdc1_clk_sample = {
	.name               = "sdc1_clk_sample",
	.parent             = {&sdc0_clk},
	.flag				= CLOCK_NOGATE | CLOCK_PHASE,
	.prediv             = 1,
	.sel_reg_offset     = REG_PMU_CLK_GATE1,
	.sel_reg_mask       = 0xf00,
};

static struct fh_clk sdc1_clk_drv = {
	.name               = "sdc1_clk_drv",
	.parent             = {&sdc0_clk},
	.flag				= CLOCK_NOGATE | CLOCK_PHASE,
	.prediv             = 1,
	.sel_reg_offset     = REG_PMU_CLK_GATE1,
	.sel_reg_mask       = 0xf000,
};

struct fh_clk *fh_clks[] = {
	&osc_clk,
	&pll_ddr_rclk,
	&pll_cpu_pclk,
	&pll_cpu_rclk,
	&pll_sys_pclk,
	&pll_sys_rclk,
	&arm_clk,
	&arc_clk,
	&ahb_clk,
	&ddr_clk,
	&isp_aclk,
	&ispb_aclk,
	&jpeg_clk,
	&jpeg_adapt_clk,
	&vpu_clk,
	&veu_clk,
	&veu_adapt_clk,
	&bgm_clk,
	&mipi_dphy_clk,
	&pllsysp_div12_clk,
	&cis_clk_out,
	&pix_clk,
	&pts_clk,
	&spi0_clk,
	&spi1_clk,
	&spi2_clk,
	&sdc0_clk,
	&sdc1_clk,
	&uart0_clk,
	&uart1_clk,
	&uart2_clk,
	&i2c0_clk,
	&i2c1_clk,
	&i2c2_clk,
	&pwm_clk,
	&wdt_clk,
	&tmr0_clk,
	&ac_clk,
	&i2s_clk,
	&sadc_clk,
	&eth_clk,
	&efuse_clk,
	&gpio0_db_clk,
	&gpio1_db_clk,
	&mipi_wrap_gate,
	&rtc_hclk_gate,
	&emac_hclk_gate,
	&usb_clk,
	&aes_hclk_gate,
	&ephy_clk_gate,
	&sdc0_clk8x_gate,
	&sdc1_clk8x_gate,
	&gpio0_pclk_gate,
	&gpio1_pclk_gate,
	&mipic_pclk_gate,
	&sdc0_clk_sample,
	&sdc0_clk_drv,
	&sdc1_clk_sample,
	&sdc1_clk_drv,
	&isp_hclk_gate,
	&veu_hclk_gate,
	&bgm_hclk_gate,
	&adapt_hclk_gate,
	&jpg_hclk_gate,
	&jpg_adapt_gate,
	&vpu_hclk_gate,
	NULL,
};
EXPORT_SYMBOL(fh_clks);
