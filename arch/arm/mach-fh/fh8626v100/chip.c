#include <linux/init.h>
#include <linux/module.h>

#include <mach/clock.h>

#include "chip.h"

static struct fh_clk osc_clk = {
	.name			= "osc_clk",
	.frequency		= OSC_FREQUENCY,
	.flag			= CLOCK_FIXED,
};

static struct fh_clk pll0_clk = {
	.name			= "pll0_clk",
	.flag			= CLOCK_PLL,
	.parent			= { &osc_clk },
	.div_reg_offset		= REG_PMU_PLL0,
};

static struct fh_clk pll1_clk = {
	.name			= "pll1_clk",
	.flag			= CLOCK_PLL,
	.parent			= { &osc_clk },
	.div_reg_offset		= REG_PMU_PLL1,
};

static struct fh_clk cis_pix_clk = {
	.name			= "cis_pix_clk",
	.frequency		= 75000000,
	.flag			= CLOCK_FIXED,
};

static struct fh_clk cis_pix_clk_rt = {
	.name			= "cis_pix_clk_rt",
	.frequency		= 75000000,
	.flag			= CLOCK_FIXED,
};

static struct fh_clk mipi_pix_clk = {
	.name			= "mipi_pix_clk",
	.flag			= CLOCK_MULTI_PARENT | CLOCK_NOGATE |
				  CLOCK_NORESET,
	.parent			= { &pll0_clk, &pll1_clk },
	.prediv			= 1,
	.div_reg_offset		= 0x0000003c,
	.div_reg_mask		= 0x001f0000,
	.sel_reg_offset		= 0x00000028,
	.sel_reg_mask		= 0x00000020,
};

static struct fh_clk pll0_cis_clk = {
	.name			= "pll0_cis_clk",
	.flag			= CLOCK_NODIV | CLOCK_NORESET,
	.en_reg_offset		= 0x00000024,
	.en_reg_mask		= 0x00000010,
};

static struct fh_clk pix_clk = {
	.name			= "pix_clk",
	.flag			= CLOCK_MULTI_PARENT | CLOCK_NODIV |
				  CLOCK_NORESET,
	.parent			= { &cis_pix_clk, &cis_pix_clk_rt,
				    &mipi_pix_clk, &mipi_pix_clk },
	.prediv			= 1,
	.en_reg_offset		= 0x00000024,
	.en_reg_mask		= 0x00020000,
	.sel_reg_offset		= 0x00000028,
	.sel_reg_mask		= 0x00000060,
};

static struct fh_clk cis_clk_out = {
	.name			= "cis_clk_out",
	.flag			= CLOCK_MULTI_PARENT | CLOCK_NODIV,
	.parent			= { &osc_clk, &pll0_clk },
	.prediv			= 1,
	.en_reg_offset		= 0x0000001c,
	.en_reg_mask		= 0x00800000,
	.sel_reg_offset		= 0x00000028,
	.sel_reg_mask		= 0x00000008,
};

static struct fh_clk arc_clk = {
	.name			= "arc_clk",
	.flag			= CLOCK_MULTI_PARENT,
	.parent			= { &osc_clk, &pll0_clk },
	.prediv			= 1,
	.div_reg_offset		= REG_PMU_CLK_DIV2,
	.div_reg_mask		= 0x000000f0,
	.en_reg_offset		= REG_PMU_CLK_GATE,
	.en_reg_mask		= 0x00000004,
	.rst_reg_offset		= REG_PMU_SWRST_APB_CTRL,
	.rst_reg_mask		= 0x00400000,
	.sel_reg_offset		= REG_PMU_SYS_CTRL,
	.sel_reg_mask		= 0x00000001,
};

static struct fh_clk ahb_clk = {
	.name			= "ahb_clk",
	.flag			= CLOCK_MULTI_PARENT | CLOCK_NORESET |
				  CLOCK_NOGATE,
	.parent			= { &osc_clk, &pll0_clk },
	.prediv			= 2,
	.div_reg_offset		= REG_PMU_CLK_DIV2,
	.div_reg_mask		= 0x000000f0,
	.sel_reg_offset		= REG_PMU_SYS_CTRL,
	.sel_reg_mask		= 0x00000001,
};

static struct fh_clk dw_100m_clk = {
	.name			= "dw_100m_clk",
	.flag			= CLOCK_NOGATE | CLOCK_NODIV | CLOCK_NORESET,
	.parent			= { &pll1_clk },
	.prediv			= 6,
};

/* Exact stock FH8626V100 SDC clock and phase controls. */
static struct fh_clk sdc0_clk = {
	.name			= "sdc0_clk",
	.parent			= { &dw_100m_clk },
	.prediv			= 2,
	.div_reg_offset		= REG_PMU_CLK_DIV2,
	.div_reg_mask		= 0x000f0000,
	.en_reg_offset		= REG_PMU_CLK_GATE,
	.en_reg_mask		= 0x00000200,
	.rst_reg_offset		= REG_PMU_SWRST_AHB_CTRL,
	.rst_reg_mask		= FH8626V100_SDC0_RESET_MASK,
};

static struct fh_clk sdc0_clk_drv = {
	.name			= "sdc0_clk_drv",
	.flag			= CLOCK_NOGATE | CLOCK_PHASE,
	.parent			= { &sdc0_clk },
	.prediv			= 1,
	.sel_reg_offset		= REG_PMU_ETH_CTRL,
	.sel_reg_mask		= 0x00000180,
};

static struct fh_clk sdc0_clk_sample = {
	.name			= "sdc0_clk_sample",
	.flag			= CLOCK_NOGATE | CLOCK_PHASE,
	.parent			= { &sdc0_clk },
	.prediv			= 1,
	.sel_reg_offset		= REG_PMU_ETH_CTRL,
	.sel_reg_mask		= 0x00000600,
};

static struct fh_clk sdc1_clk = {
	.name			= "sdc1_clk",
	.parent			= { &dw_100m_clk },
	.prediv			= 2,
	.div_reg_offset		= REG_PMU_CLK_DIV2,
	.div_reg_mask		= 0x00f00000,
	.en_reg_offset		= REG_PMU_CLK_GATE,
	.en_reg_mask		= 0x00000400,
	.rst_reg_offset		= REG_PMU_SWRST_AHB_CTRL,
	.rst_reg_mask		= FH8626V100_SDC1_RESET_MASK,
};

static struct fh_clk sdc1_clk_drv = {
	.name			= "sdc1_clk_drv",
	.flag			= CLOCK_NOGATE | CLOCK_PHASE,
	.parent			= { &sdc1_clk },
	.prediv			= 1,
	.sel_reg_offset		= REG_PMU_ETH_CTRL,
	.sel_reg_mask		= 0x00001800,
};

static struct fh_clk sdc1_clk_sample = {
	.name			= "sdc1_clk_sample",
	.flag			= CLOCK_NOGATE | CLOCK_PHASE,
	.parent			= { &sdc1_clk },
	.prediv			= 1,
	.sel_reg_offset		= REG_PMU_ETH_CTRL,
	.sel_reg_mask		= 0x00006000,
};

static struct fh_clk pts_clk = {
	.name			= "pts_clk",
	.parent			= { &dw_100m_clk },
	.prediv			= 1,
	.div_reg_offset		= 0x00000038,
	.div_reg_mask		= 0x01ff0000,
	.en_reg_offset		= 0x0000001c,
	.en_reg_mask		= 0x00080000,
	.rst_reg_offset		= 0x0000004c,
	.rst_reg_mask		= 0x00000001,
};

static struct fh_clk isp_hclk = {
	.name			= "isp_hclk",
	.flag			= CLOCK_NODIV | CLOCK_NORESET,
	.prediv			= 1,
	.en_reg_offset		= 0x0000001c,
	.en_reg_mask		= 0x00000002,
};

static struct fh_clk isp_aclk = {
	.name			= "isp_aclk",
	.flag			= CLOCK_MULTI_PARENT,
	.parent			= { &pll0_clk, &pll1_clk },
	.prediv			= 1,
	.div_reg_offset		= 0x0000003c,
	.div_reg_mask		= 0x00001f00,
	.en_reg_offset		= 0x0000001c,
	.en_reg_mask		= 0x00000001,
	.rst_reg_offset		= 0x0000004c,
	.rst_reg_mask		= 0x00400000,
	.sel_reg_offset		= 0x00000028,
	.sel_reg_mask		= 0x00000010,
};

static struct fh_clk pae_clk = {
	.name			= "pae_clk",
	.parent			= { &pll0_clk },
	.prediv			= 1,
	.div_reg_offset		= 0x0000003c,
	.div_reg_mask		= 0x07000000,
	.en_reg_offset		= 0x00000024,
	.en_reg_mask		= 0x00400000,
	.rst_reg_offset		= 0x0000004c,
	.rst_reg_mask		= 0x00800000,
};

static struct fh_clk pae_adpt_clk = {
	.name			= "pae_adpt_clk",
	.flag			= CLOCK_NODIV | CLOCK_NORESET,
	.en_reg_offset		= 0x00000024,
	.en_reg_mask		= 0x01000000,
};

static struct fh_clk multi_pae_clk = {
	.name			= "multi_pae_clk",
	.flag			= CLOCK_NODIV | CLOCK_NORESET,
	.en_reg_offset		= 0x00000024,
	.en_reg_mask		= 0x00000200,
};

static struct fh_clk pae_hclk_gate = {
	.name			= "pae_hclk_gate",
	.flag			= CLOCK_NODIV | CLOCK_NORESET,
	.en_reg_offset		= 0x00000024,
	.en_reg_mask		= 0x00800000,
};

static struct fh_clk jpeg_hclk = {
	.name			= "jpeg_hclk",
	.flag			= CLOCK_NODIV | CLOCK_NORESET,
	.prediv			= 1,
	.en_reg_offset		= 0x00000024,
	.en_reg_mask		= 0x00080000,
};

static struct fh_clk jpeg_clk = {
	.name			= "jpeg_clk",
	.flag			= CLOCK_MULTI_PARENT,
	.parent			= { &pll0_clk, &pll1_clk },
	.prediv			= 1,
	.div_reg_offset		= 0x0000003c,
	.div_reg_mask		= 0x00001f00,
	.en_reg_offset		= 0x00000024,
	.en_reg_mask		= 0x00040000,
	.rst_reg_offset		= 0x0000004c,
	.rst_reg_mask		= 0x01000000,
	.sel_reg_offset		= 0x00000028,
	.sel_reg_mask		= 0x00000010,
};

static struct fh_clk bgm_hclk = {
	.name			= "bgm_hclk",
	.flag			= CLOCK_NODIV | CLOCK_NORESET,
	.prediv			= 1,
	.en_reg_offset		= 0x00000024,
	.en_reg_mask		= 0x00200000,
};

static struct fh_clk bgm_clk = {
	.name			= "bgm_clk",
	.flag			= CLOCK_MULTI_PARENT,
	.parent			= { &pll0_clk, &pll1_clk },
	.prediv			= 1,
	.div_reg_offset		= 0x0000003c,
	.div_reg_mask		= 0x00001f00,
	.en_reg_offset		= 0x00000024,
	.en_reg_mask		= 0x00100000,
	.rst_reg_offset		= 0x0000004c,
	.rst_reg_mask		= 0x02000000,
	.sel_reg_offset		= 0x00000028,
	.sel_reg_mask		= 0x00000010,
};

static struct fh_clk spi0_clk = {
	.name			= "spi0_clk",
	.parent			= { &dw_100m_clk },
	.prediv			= 1,
	.div_reg_offset		= REG_PMU_CLK_DIV2,
	.div_reg_mask		= 0x00000f00,
	.en_reg_offset		= REG_PMU_CLK_GATE,
	.en_reg_mask		= 0x00000080,
	.rst_reg_offset		= REG_PMU_SWRST_APB_CTRL,
	.rst_reg_mask		= 0x00002000,
};

static struct fh_clk wdt_clk = {
	.name			= "wdt_clk",
	.parent			= { &ahb_clk },
	.prediv			= 1,
	.div_reg_offset		= REG_PMU_CLK_DIV5,
	.div_reg_mask		= 0x0000ff00,
	.en_reg_offset		= REG_PMU_CLK_GATE1,
	.en_reg_mask		= 0x00000004,
	.rst_reg_offset		= REG_PMU_WDT_CLK_RST,
	.rst_reg_mask		= 0x00080000,
};

static struct fh_clk pll1_clk_div_2 = {
	.name			= "pll1_clk_div_2",
	.flag			= CLOCK_NOGATE | CLOCK_NODIV,
	.parent			= { &pll1_clk },
	.prediv			= 2,
};

static struct fh_clk dw_50m_clk = {
	.name			= "dw_50m_clk",
	.flag			= CLOCK_NOGATE | CLOCK_NODIV | CLOCK_NORESET,
	.parent			= { &dw_100m_clk },
	.prediv			= 2,
};

static struct fh_clk i2c0_clk = {
	.name			= "i2c0_clk",
	.parent			= { &dw_100m_clk },
	.prediv			= 1,
	.div_reg_offset		= 0x00000034,
	.div_reg_mask		= 0x0000003f,
	.en_reg_offset		= 0x0000001c,
	.en_reg_mask		= 0x00001000,
	.rst_reg_offset		= 0x0000004c,
	.rst_reg_mask		= 0x00000200,
};

static struct fh_clk i2c1_clk = {
	.name			= "i2c1_clk",
	.parent			= { &dw_100m_clk },
	.prediv			= 1,
	.div_reg_offset		= 0x00000034,
	.div_reg_mask		= 0x00003f00,
	.en_reg_offset		= 0x0000001c,
	.en_reg_mask		= 0x08000000,
	.rst_reg_offset		= 0x0000004c,
	.rst_reg_mask		= 0x00000100,
};

static struct fh_clk i2c2_clk = {
	.name			= "i2c2_clk",
	.parent			= { &dw_100m_clk },
	.prediv			= 1,
	.div_reg_offset		= 0x00000034,
	.div_reg_mask		= 0x003f0000,
	.en_reg_offset		= 0x00000020,
	.en_reg_mask		= 0x00000200,
	.rst_reg_offset		= 0x0000004c,
	.rst_reg_mask		= 0x00000080,
};

static struct fh_clk eth_clk = {
	.name			= "eth_clk",
	.parent			= { &dw_100m_clk },
	.prediv			= 1,
	.div_reg_offset		= 0x0000002c,
	.div_reg_mask		= 0x0f000000,
	.en_reg_offset		= 0x0000001c,
	.en_reg_mask		= 0x02000000,
	.rst_reg_offset		= 0x00000054,
	.rst_reg_mask		= 0x00000400,
};

static struct fh_clk eth_rmii_clk = {
	.name			= "eth_rmii_clk",
	.flag			= CLOCK_NODIV | CLOCK_NORESET,
	.prediv			= 1,
	.en_reg_offset		= 0x0000001c,
	.en_reg_mask		= 0x10000000,
};

/*
 * Internal audio clock tree recovered field-for-field from the stock
 * FH8626V100 kernel.  The fixed 98.304 MHz source supports the codec's audio
 * sample-rate family without depending on external I2S pin multiplexing.
 * Keep the gate masks exact: xbus_rpc's RTX audio reset ioctl enables these
 * clocks before the ARC firmware starts capture.  A shifted mask lets the
 * command path start but leaves the PCM DMA ring permanently empty.
 */
static struct fh_clk acodec_pll_clk = {
	.name			= "acodec_pll_clk",
	.frequency		= 98304000,
	.flag			= CLOCK_FIXED,
};

static struct fh_clk ac_clk = {
	.name			= "ac_clk",
	.flag			= CLOCK_MULTI_PARENT,
	.parent			= { &osc_clk, &acodec_pll_clk },
	.prediv			= 1,
	.div_reg_offset		= REG_PMU_AUDIO_CTRL,
	.div_reg_mask		= 0x000003f0,
	.en_reg_offset		= REG_PMU_AUDIO_CTRL,
	.en_reg_mask		= 0x00100000,
	.rst_reg_offset		= REG_PMU_SWRST_APB_CTRL,
	.rst_reg_mask		= 0x00000040,
	.sel_reg_offset		= REG_PMU_AUDIO_CTRL,
	.sel_reg_mask		= 0x00000001,
};

static struct fh_clk i2s_clk = {
	.name			= "i2s_clk",
	.parent			= { &ac_clk },
	.prediv			= 1,
	.div_reg_offset		= REG_PMU_AUDIO_CTRL,
	.div_reg_mask		= 0x0003f000,
	.en_reg_offset		= REG_PMU_AUDIO_CTRL,
	.en_reg_mask		= 0x01000000,
	.rst_reg_offset		= REG_PMU_SWRST_APB_CTRL,
	.rst_reg_mask		= 0x00001000,
};

static struct fh_clk acodec_pclk = {
	.name			= "acodec_pclk",
	.flag			= CLOCK_NODIV | CLOCK_NORESET,
	.en_reg_offset		= REG_PMU_CLK_GATE2,
	.en_reg_mask		= 0x00010000,
};

static struct fh_clk acodec_mclk = {
	.name			= "acodec_mclk",
	.flag			= CLOCK_NODIV | CLOCK_NORESET,
	.en_reg_offset		= REG_PMU_CLK_GATE2,
	.en_reg_mask		= 0x00008000,
};

static struct fh_clk rtc_pclk_gate = {
	.name			= "rtc_pclk_gate",
	.flag			= CLOCK_NODIV | CLOCK_NORESET,
	.en_reg_offset		= 0x00000020,
	.en_reg_mask		= 0x00000002,
};

static struct fh_clk efuse_clk = {
	.name			= "efuse_clk",
	.parent			= { &dw_100m_clk },
	.prediv			= 1,
	.div_reg_offset		= 0x0000002c,
	.div_reg_mask		= 0xf0000000,
	.en_reg_offset		= 0x0000001c,
	.en_reg_mask		= 0x00002000,
	.rst_reg_offset		= 0x0000004c,
	.rst_reg_mask		= 0x00000020,
};

static struct fh_clk pwm_clk = {
	.name			= "pwm_clk",
	.parent			= { &dw_100m_clk },
	.prediv			= 1,
	.div_reg_offset		= 0x00000034,
	.div_reg_mask		= 0xff000000,
	.en_reg_offset		= 0x0000001c,
	.en_reg_mask		= 0x00010000,
	.rst_reg_offset		= 0x0000004c,
	.rst_reg_mask		= 0x00000010,
	/* Stock FH8626V100 programs PWM to 25 MHz during clock init. */
	.def_rate		= 25000000,
};

static struct fh_clk sadc_clk = {
	.name			= "sadc_clk",
	.parent			= { &dw_100m_clk },
	.prediv			= 1,
	.div_reg_offset		= 0x0000003c,
	.div_reg_mask		= 0x0000007f,
	.en_reg_offset		= 0x0000001c,
	.en_reg_mask		= 0x04000000,
	.rst_reg_offset		= 0x0000004c,
	.rst_reg_mask		= 0x00000008,
};

static struct fh_clk usb_clk = {
	.name			= "usb_clk",
	.flag			= CLOCK_MULTI_PARENT | CLOCK_NODIV,
	.parent			= { &osc_clk, &dw_50m_clk },
	.prediv			= 1,
	.en_reg_offset		= REG_PMU_CLK_GATE2,
	.en_reg_mask		= 0x00002000,
	.sel_reg_offset		= REG_PMU_ETH_CTRL,
	.sel_reg_mask		= 0x00008000,
};

static struct fh_clk usb_hclk = {
	.name			= "usb_hclk",
	.flag			= CLOCK_NODIV | CLOCK_NORESET,
	.prediv			= 1,
	.en_reg_offset		= REG_PMU_CLK_GATE2,
	.en_reg_mask		= 0x00004000,
};

static struct fh_clk tmr0_clk = {
	.name			= "tmr0_clk",
	.parent			= { &dw_100m_clk },
	.prediv			= 1,
	.div_reg_offset		= REG_PMU_CLK_DIV5,
	.div_reg_mask		= 0x000000ff,
	.en_reg_offset		= REG_PMU_CLK_GATE,
	.en_reg_mask		= 0x00020000,
	.rst_reg_offset		= REG_PMU_SWRST_APB_CTRL,
	.rst_reg_mask		= 0x00000002,
};

struct fh_clk *fh_clks[] = {
	&osc_clk,
	&pll0_clk,
	&pll1_clk,
	&cis_pix_clk,
	&cis_pix_clk_rt,
	&mipi_pix_clk,
	&pll0_cis_clk,
	&pix_clk,
	&cis_clk_out,
	&arc_clk,
	&ahb_clk,
	&dw_100m_clk,
	&sdc0_clk,
	&sdc0_clk_drv,
	&sdc0_clk_sample,
	&sdc1_clk,
	&sdc1_clk_drv,
	&sdc1_clk_sample,
	&pts_clk,
	&isp_hclk,
	&isp_aclk,
	&pae_clk,
	&pae_adpt_clk,
	&multi_pae_clk,
	&pae_hclk_gate,
	&jpeg_hclk,
	&jpeg_clk,
	&bgm_hclk,
	&bgm_clk,
	&spi0_clk,
	&wdt_clk,
	&pll1_clk_div_2,
	&dw_50m_clk,
	&i2c0_clk,
	&i2c1_clk,
	&i2c2_clk,
	&eth_clk,
	&eth_rmii_clk,
	&acodec_pll_clk,
	&ac_clk,
	&i2s_clk,
	&acodec_pclk,
	&acodec_mclk,
	&rtc_pclk_gate,
	&efuse_clk,
	&pwm_clk,
	&sadc_clk,
	&usb_clk,
	&usb_hclk,
	&tmr0_clk,
	NULL,
};
EXPORT_SYMBOL(fh_clks);
