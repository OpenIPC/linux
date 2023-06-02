/**
    NVT clock management module for NA51068 SoC
    @file na51068-clk.c

    Copyright Novatek Microelectronics Corp. 2019. All rights reserved.

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2 as
    published by the Free Software Foundation.
*/

#include <linux/clk-provider.h>
#include <linux/kernel.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/gcd.h>
#include <linux/clkdev.h>

#include "plat/hardware.h"
#include "plat/cg-reg.h"
#include "nvt-im-clk.h"

#define IOADDR_CG_REG_SIZE 0x1A4

#define DEFAULT_OSC_IN_CLK_FREQ  12000000
#define DEFAULT_PLL_DIV_VALUE    131072
#define DEFAULT_PLL0_RATIO       4369066      /* (400MHz for AXI0) */
#define DEFAULT_PLL1_RATIO       4369066      /* (400MHz for AXI1) */
#define DEFAULT_PLL2_RATIO       5461333      /* (500MHz for CNN) */
#define DEFAULT_PLL3_RATIO       2184533      /* (200MHz for DRAM) */
#define DEFAULT_PLL4_RATIO       3276800      /* (300MHz for AHB) */
#define DEFAULT_PLL5_RATIO       1092266      /* (100MHz for SATAPHY) */
#define DEFAULT_PLL6_RATIO       2184533      /* (200MHz for SDC) */
#define DEFAULT_PLL7_RATIO       2730666      /* (250MHz for ETH) */
#define DEFAULT_PLL8_RATIO       2796202      /* (256MHz for UART) */
#define DEFAULT_PLL9_RATIO       3244032      /* (297MHz for DISP0) */
#define DEFAULT_PLL10_RATIO      2730666      /* (250MHz for DISP1) */
#define DEFAULT_PLL11_RATIO      589824       /* (54MHz for DISP2) */
#define DEFAULT_PLL12_RATIO      3758096      /* (344.0640MHz for SSP) */
#define DEFAULT_PLL13_RATIO      3822933      /* (350MHz for AXI2) */
#define DEFAULT_PLL14_RATIO      4150613      /* (380MHz for H264/H265) */
#define DEFAULT_PLL15_RATIO      6553600      /* (600MHz for DSP) */
#define DEFAULT_PLL16_RATIO      1911466      /* (175MHz for CPU) */

static void __iomem *remap_base = NULL;
static spinlock_t cg_lock;

#define CPU_CLK_SEL_CPU_APLL  0x80000000

/**
 * Get frequency of peripheral clock
 */
unsigned long get_periph_clk(void)
{
	unsigned long long cpu_clk_sel;
	u32 cpu_clk;

	cpu_clk_sel = readl(remap_base + CG_CPU_CKCTRL_REG_OFFSET) & 0x80000000;

	switch (cpu_clk_sel) {
	case CPU_CLK_SEL_CPU_APLL: {
		struct clk *pll16_clk = clk_get(NULL, "pll16");

		cpu_clk = clk_get_rate(pll16_clk) * 8;
		break;
	}
	default:
#ifdef CONFIG_NVT_FPGA_EMULATION
		cpu_clk = 24000000;
#else
		cpu_clk = 80000000;
#endif
		break;
	}

	return (cpu_clk/8);
}

static void __init nvt_periph_clk_init(struct device_node *node)
{
	struct clk *periph_clk;

	periph_clk = clk_register_fixed_rate(NULL,
			"periph_clk",
			NULL,
			0,
			get_periph_clk());

	clk_register_clkdev(periph_clk, NULL, "periph_clk_dev");
	of_clk_add_provider(node, of_clk_src_simple_get, periph_clk);
	pr_err("***%s\n", __func__);
}

CLK_OF_DECLARE(nvt_periph_clk, "nvt,periph_clk", nvt_periph_clk_init);

static struct nvt_fixed_rate_clk novatek_fixed_rate_clk[] __initdata = {
	FIXED_RATE_CONF("osc_in", 12000000),
	FIXED_RATE_CONF("osc_rtc", 32768),
#ifdef CONFIG_NVT_FPGA_EMULATION
	FIXED_RATE_CONF("fix48m", 48000000),
	FIXED_RATE_CONF("fix27m", 27000000),
	FIXED_RATE_CONF("fix25m", 25000000),
	FIXED_RATE_CONF("fix24m", 24000000),
	FIXED_RATE_CONF("fix20m", 20000000),
	FIXED_RATE_CONF("fix16m", 16000000),
	FIXED_RATE_CONF("fix12m", 12000000),
#else
	FIXED_RATE_CONF("fix480m", 480000000),
	FIXED_RATE_CONF("fix192m", 192000000),
	FIXED_RATE_CONF("fix12m", 12000000),
#endif
};

static struct nvt_pll_clk novatek_pll[] __initdata = {
	/*
	 * The format of PLL_CONF as shown below.
	 * ( name       , pll_ratio, pll_ratio_reg
	 *   pll_en_reg , bit_idx                  )
	 */
	PLL_CONF("pll0", DEFAULT_PLL0_RATIO, CG_PLL0_CLK_RATE0_REG_OFFSET,
	     CG_PLL_EN_REG_OFFSET, BIT0),
	PLL_CONF("pll1", DEFAULT_PLL1_RATIO, CG_PLL1_CLK_RATE0_REG_OFFSET,
	     CG_PLL_EN_REG_OFFSET, BIT1),
	PLL_CONF("pll2", DEFAULT_PLL2_RATIO, CG_PLL2_CLK_RATE0_REG_OFFSET,
	     CG_PLL_EN_REG_OFFSET, BIT2),
	PLL_CONF("pll3", DEFAULT_PLL3_RATIO, CG_PLL3_CLK_RATE0_REG_OFFSET,
	     CG_PLL_EN_REG_OFFSET, BIT3),
	PLL_CONF("pll4", DEFAULT_PLL4_RATIO, CG_PLL4_CLK_RATE0_REG_OFFSET,
	     CG_PLL_EN_REG_OFFSET, BIT4),
	PLL_CONF("pll5", DEFAULT_PLL5_RATIO, CG_PLL5_CLK_RATE0_REG_OFFSET,
	     CG_PLL_EN_REG_OFFSET, BIT5),
	PLL_CONF("pll6", DEFAULT_PLL6_RATIO, CG_PLL6_CLK_RATE0_REG_OFFSET,
	     CG_PLL_EN_REG_OFFSET, BIT6),
	PLL_CONF("pll7", DEFAULT_PLL7_RATIO, CG_PLL7_CLK_RATE0_REG_OFFSET,
	     CG_PLL_EN_REG_OFFSET, BIT7),
	PLL_CONF("pll8", DEFAULT_PLL8_RATIO, CG_PLL8_CLK_RATE0_REG_OFFSET,
	     CG_PLL_EN_REG_OFFSET, BIT8),
	PLL_CONF("pll9", DEFAULT_PLL9_RATIO, CG_PLL9_CLK_RATE0_REG_OFFSET,
	     CG_PLL_EN_REG_OFFSET, BIT9),
	PLL_CONF("pll10", DEFAULT_PLL10_RATIO, CG_PLL10_CLK_RATE0_REG_OFFSET,
	     CG_PLL_EN_REG_OFFSET, BIT10),
	PLL_CONF("pll11", DEFAULT_PLL11_RATIO, CG_PLL11_CLK_RATE0_REG_OFFSET,
	     CG_PLL_EN_REG_OFFSET, BIT11),
	PLL_CONF("pll12", DEFAULT_PLL12_RATIO, CG_PLL12_CLK_RATE0_REG_OFFSET,
	     CG_PLL_EN_REG_OFFSET, BIT12),
	PLL_CONF("pll13", DEFAULT_PLL13_RATIO, CG_PLL13_CLK_RATE0_REG_OFFSET,
	     CG_PLL_EN_REG_OFFSET, BIT13),
	PLL_CONF("pll14", DEFAULT_PLL14_RATIO, CG_PLL14_CLK_RATE0_REG_OFFSET,
	     CG_PLL_EN_REG_OFFSET, BIT14),
	PLL_CONF("pll15", DEFAULT_PLL15_RATIO, CG_PLL15_CLK_RATE0_REG_OFFSET,
	     CG_PLL_EN_REG_OFFSET, BIT15),
	PLL_CONF("pll16", DEFAULT_PLL16_RATIO, CG_PLL16_CLK_RATE0_REG_OFFSET,
	     CG_PLL_EN_REG_OFFSET, BIT16),
};

static const char *root_clk_parent[] __initconst = {
	"osc_in",
};
static const char *rtc_clk_parent[] __initconst = {
	"osc_rtc",
};
static const char *eth_root_parent[] __initconst = {
	"pll7",
};
static const char *eth0_parent[] __initconst = {
	"eth0_250m",
};
static const char *eth1_parent[] __initconst = {
	"eth1_250m",
};

#ifdef CONFIG_NVT_FPGA_EMULATION
static const char *i2c_clk_parent[] __initconst = {
	"osc_in",
};
static const char *uart_clk_parent[] __initconst = {
	"fix48m",
};
static const char *mmc_clk_parent[] __initconst = {
	"fix24m",
};
static const char *vcap_aclk_m_parent[] __initconst = {
	"fix20m",
};
static const char *vcap_aclk_s_parent[] __initconst = {
	"fix12m",
};
static const char *vcap_mclk_parent[] __initconst = {
	"fix20m", "fix48m"
};
static const char *vcap_patclk_parent[] __initconst = {
	"fix24m", "fix48m", "fix48m", "fix16m"
};
static const char *osg_clk_parent[] __initconst = {
	"pll0",	"pll1",
};
static const char *ext0_clk_parent[] __initconst = {
	"fix27m",
};
static const char *ext1_clk_parent[] __initconst = {
	"fix27m",
};
static const char *gm2d_clk_parent[] __initconst = {
	"fix24m",
};
static const char *ssca_clk_parent[] __initconst = {
	"fix24m",
};
static const char *vpe_clk_parent[] __initconst = {
	"pll0",	"pll1",
};
static const char *dei_aclk_parent[] __initconst = {
	"pll0",	"pll1",
};
static const char *dei_mclk_parent[] __initconst = {
	"pll0",	"pll1",
};
static const char *jpg_clk_parent[] __initconst = {
	"pll4",
};
static const char *venc_aclk_parent[] __initconst = {
    "pll4",
};
static const char *venc_mclk_parent[] __initconst = {
    "pll4", "pll14",
};
static const char *venc_pclk_parent[] __initconst = {
    "pll4",
};

static const char *sspmain_clk_parent[] __initconst = {
	"fix12m",
};
static const char *ssp_clk_parent[] __initconst = {
	"ssp_main_clk","fix12m","pll7","pll6"
};
static const char *ssph_clk_parent[] __initconst = {
	"ssph_main_clk","fix12m","pll7","pll6"
};
static const char *h264d_mclk_parent[] __initconst = {
	"pll0", "pll1",
};
static const char *h265d_mclk_parent[] __initconst = {
	"pll13", "pll14",
};
static const char *h26xd_pclk_parent[] __initconst = {
	"pll4",
};
static const char *h264d_aclk_parent[] __initconst = {
	"pll0",
};
static const char *h265d_aclk_parent[] __initconst = {
	"pll13", "pll14",
};

static const char *spi_clk_parent[] __initconst = {
	"osc_in",
};

static const char *lcd210_clk_parent[] __initconst = {
	"pll9","pll10","pll11",
};

static const char *lcd310_clk_parent[] __initconst = {
	"pll9","pll10","pll11",
};

/*static const char *lcd310_mclk_parent[] __initconst = {
	"mpll0","mpll1",
};*/

#else
static const char *pll4div2_clk_parent[] __initconst = {
	"pll4"
};
static const char *i2c_clk_parent[] __initconst = {
	"pll4div2",
};
static const char *uart_clk_parent[] __initconst = {
	"fix480m", "osc_in", "ssp_clk_parent", "pll7",
};
static const char *pll6div2_clk_parent[] __initconst = {
	"pll6"
};
static const char *mmc_clk_parent[] __initconst = {
	"pll6div2",
};
static const char *vcap_aclk_m_parent[] __initconst = {
	"pll0",
};
static const char *vcap_aclk_s_parent[] __initconst = {
	"pll4",
};
static const char *vcap_mclk_parent[] __initconst = {
	"pll0", "pll8"
};
static const char *vcap_patclk_parent[] __initconst = {
	"pll9", "pll6", "pll8", "pll13"
};
static const char *osg_clk_parent[] __initconst = {
	"pll0",	"pll1",
};
static const char *ext0_clk_parent[] __initconst = {
	"pll10", "pll7", "pll12", "osc_in"
};
static const char *ext1_clk_parent[] __initconst = {
	"pll10", "pll7", "pll12", "osc_in"
};
static const char *gm2d_clk_parent[] __initconst = {
	"pll4",
};
static const char *ssca_clk_parent[] __initconst = {
	"pll4",
};
static const char *vpe_clk_parent[] __initconst = {
	"pll0",	"pll1",
};
static const char *dei_aclk_parent[] __initconst = {
	"pll0",	"pll1",
};
static const char *dei_mclk_parent[] __initconst = {
	"pll0",	"pll1",
};
static const char *jpg_clk_parent[] __initconst = {
	"pll4",
};
static const char *venc_aclk_parent[] __initconst = {
    "pll4",
};
static const char *venc_mclk_parent[] __initconst = {
    "pll14", "pll4",
};
static const char *venc_pclk_parent[] __initconst = {
    "pll4",
};
static const char *sspmain_clk_parent[] __initconst = {
	"pll12",
};
static const char *ssp_clk_parent[] __initconst = {
	"ssp_main_clk","fix12m","pll7","pll6"
};
static const char *ssph_clk_parent[] __initconst = {
	"ssph_main_clk","fix12m","pll7","pll6"
};
static const char *h264d_mclk_parent[] __initconst = {
	"pll0", "pll1",
};
static const char *h265d_mclk_parent[] __initconst = {
	"pll13", "pll14",
};
static const char *h26xd_pclk_parent[] __initconst = {
	"pll4",
};
static const char *h264d_aclk_parent[] __initconst = {
	"pll0",
};
static const char *h265d_aclk_parent[] __initconst = {
	"pll13", "pll14",
};
static const char *spi_clk_parent[] __initconst = {
	"pll4div2", "pll6", "pll8"
};
static const char *lcd210_clk_parent[] __initconst = {
	"pll9","pll10","pll11",
};
static const char *lcd310_clk_parent[] __initconst = {
	"pll9","pll10","pll11",
};
static const char *sata_aclk_parent[] __initconst = {
	"pll4",
};

/*static const char *lcd310_mclk_parent[] __initconst = {
	"mpll0","mpll1",
};*/
static const char *sce_clk_parent[] __initconst = {
	"pll1", "pll13",
};
#endif

static struct nvt_composite_mux_clk novatek_cmux_clk[] __initdata = {
	/*
	 * If the parent is not assigned, use root_clk_parent as parent_names,
	 * and fill 0 to parent_idx and mux_reg.
	 *
	 * If the divider is not assigned, fill 0 to div_reg.
	 *
	 * Fill SWDIVX to correct software value, see pll4div2 and pll6div2.
	 *
	 * The format of COMP_MUX_CONF as shown below.
	 * ( name           , parent_names , current_rate , parent_idx
	 *   mux_reg        , bit_idx      , bit_width
	 *   div_reg        , bit_idx      , bit_width/SWDIVX
	 *   bus_clk_reg    , bit_idx      , apb_clk_reg  , bit_idx , do_enable
	 *   module_rst_reg , bit_idx      , apb_rst_reg  , bit_idx , do_reset  )
	 */
	COMP_MUX_CONF("fe6c0000.pwm", root_clk_parent, 12000000, 0,
	          0, 0, 0,
	          0, 0, 0,
	          0, 0, CG_APB_CLK_GATING_REG1_OFFSET, BIT23, NOT_ENABLE,
	          0, 0, CG_APB_RESET_REG1_OFFSET, BIT29, NOT_RESET),
	COMP_MUX_CONF("fca00000.dma030", root_clk_parent, 12000000, 0,
	          0, 0, 0,
	          0, 0, 0,
	          CG_BUS_CLK_GATING_REG0_OFFSET, BIT31, CG_APB_CLK_GATING_REG0_OFFSET, BIT22, NOT_ENABLE,
	          CG_MODULE_RESET_REG0_OFFSET, BIT16, CG_APB_RESET_REG0_OFFSET, BIT22, DO_RESET),
	COMP_MUX_CONF("u2host_phy_clk0", root_clk_parent, 12000000, 0,
	          0, 0, 0,
	          0, 0, 0,
	          0, 0, CG_APB_CLK_GATING_REG0_OFFSET, BIT12, NOT_ENABLE,
	          0, 0, CG_APB_RESET_REG0_OFFSET, BIT12, DO_RESET),
	COMP_MUX_CONF("u2host_phy_clk1", root_clk_parent, 12000000, 0,
	          0, 0, 0,
	          0, 0, 0,
	          0, 0, CG_APB_CLK_GATING_REG0_OFFSET, BIT13, NOT_ENABLE,
	          0, 0, CG_APB_RESET_REG0_OFFSET, BIT13, DO_RESET),
	COMP_MUX_CONF("f9100000.u2host", root_clk_parent, 12000000, 0,
	          0, 0, 0,
	          0, 0, 0,
	          CG_BUS_CLK_GATING_REG2_OFFSET, BIT4, 0, 0, NOT_ENABLE,
	          CG_MODULE_RESET_REG1_OFFSET, BIT4, 0, 0, DO_RESET),
	COMP_MUX_CONF("f9200000.u2host", root_clk_parent, 12000000, 0,
	          0, 0, 0,
	          0, 0, 0,
	          CG_BUS_CLK_GATING_REG2_OFFSET, BIT5, 0, 0, NOT_ENABLE,
	          CG_MODULE_RESET_REG1_OFFSET, BIT5, 0, 0, DO_RESET),
	COMP_MUX_CONF("f9e00000.sata0", sata_aclk_parent, 300000000, 0,
	          0, 0, 0,
	          0, 0, 0,
	          CG_BUS_CLK_GATING_REG2_OFFSET, BIT6, 0, 0, NOT_ENABLE,
	          CG_MODULE_RESET_REG1_OFFSET, BIT8, 0, 0, DO_RESET),
	COMP_MUX_CONF("f9f00000.sata1", sata_aclk_parent, 300000000, 0,
	          0, 0, 0,
	          0, 0, 0,
	          CG_BUS_CLK_GATING_REG2_OFFSET, BIT7, 0, 0, NOT_ENABLE,
	          CG_MODULE_RESET_REG1_OFFSET, BIT9, 0, 0, DO_RESET),
	COMP_MUX_CONF("sata_phy_clk0", root_clk_parent, 12000000, 0,
	          0, 0, 0,
	          0, 0, 0,
	          0, 0, CG_APB_CLK_GATING_REG0_OFFSET, BIT26, NOT_ENABLE,
	          0, 0, CG_APB_RESET_REG0_OFFSET, BIT20, DO_RESET),
	COMP_MUX_CONF("sata_phy_clk1", root_clk_parent, 12000000, 0,
	          0, 0, 0,
	          0, 0, 0,
	          0, 0, CG_APB_CLK_GATING_REG0_OFFSET, BIT27, NOT_ENABLE,
	          0, 0, CG_APB_RESET_REG0_OFFSET, BIT21, DO_RESET),
	COMP_MUX_CONF("fd900000.hdmi20", root_clk_parent, DEFAULT_PLL9_RATIO, 0,
	          0, 0, 0,
	          0, 0, 0,
	          CG_BUS_CLK_GATING_REG3_OFFSET, BIT2, CG_APB_CLK_GATING_REG0_OFFSET, BIT6, NOT_ENABLE,
	          0, 0, CG_APB_RESET_REG0_OFFSET, BIT30, DO_RESET),
	COMP_MUX_CONF("fad00000.tve100", root_clk_parent, DEFAULT_PLL11_RATIO, 0,
	          0, 0, 0,
	          0, 0, 0,
	          CG_BUS_CLK_GATING_REG2_OFFSET, BIT13, 0, 0, NOT_ENABLE,
	          CG_MODULE_RESET_REG1_OFFSET, BIT13, 0, 0, DO_RESET),
	COMP_MUX_CONF("eth0_250m", eth_root_parent, 250000000, 0,
                        0, 0, 0,                                // clk mux
                        0, 0, 0,                                // clk div
                        CG_BUS_CLK_GATING_REG3_OFFSET, BIT4,	// module clk gating
                        0, 0,					// module apb gating
                        NOT_ENABLE,
                        0, 0,					// module reset
                        0, 0,					// module apb reset
                        NOT_RESET),
	COMP_MUX_CONF("eth1_250m", eth_root_parent, 250000000, 0,
                        0, 0, 0,                                // clk mux
                        0, 0, 0,                                // clk div
                        CG_BUS_CLK_GATING_REG3_OFFSET, BIT5,	// module clk gating
                        0, 0,					// module apb gating
                        NOT_ENABLE,
                        0, 0,					// module reset
                        0, 0,					// module apb reset
                        NOT_RESET),
	COMP_MUX_CONF("fcc00000.eth0", eth0_parent, 250000000, 0,
                        0, 0, 0,                                // clk mux
                        0, 0, 0,                                // clk div
                        CG_BUS_CLK_GATING_REG0_OFFSET, BIT16,	// module clk gating
                        CG_APB_CLK_GATING_REG0_OFFSET, BIT9,	// module apb gating
                        NOT_ENABLE,
                        CG_MODULE_RESET_REG1_OFFSET, BIT1,	// module reset
                        CG_APB_RESET_REG0_OFFSET, BIT9,		// module apb reset
                        NOT_RESET),
	COMP_MUX_CONF("rmii0_refclk_i", eth0_parent, 50000000, 0,
                        0, 0, 0,                                // clk mux
                        0, 0, 0,                                // clk div
                        CG_MAC_CONFIG_REG_OFFSET, BIT4,		// module clk gating
                        0, 0,					// module apb gating
                        NOT_ENABLE,
                        0, 0,					// module reset
                        0, 0,					// module apb reset
                        NOT_RESET),
	COMP_MUX_CONF("eth0_rxclk_i", eth0_parent, 125000000, 0,
                        0, 0, 0,                                // clk mux
                        0, 0, 0,                                // clk div
                        CG_MAC_CONFIG_REG_OFFSET, BIT5,		// module clk gating
                        0, 0,					// module apb gating
                        NOT_ENABLE,
                        0, 0,					// module reset
                        0, 0,					// module apb reset
                        NOT_RESET),
	COMP_MUX_CONF("eth0_txclk_i", eth0_parent, 125000000, 0,
                        0, 0, 0,                                // clk mux
                        0, 0, 0,                                // clk div
                        CG_MAC_CONFIG_REG_OFFSET, BIT6,		// module clk gating
                        0, 0,					// module apb gating
                        NOT_ENABLE,
                        0, 0,					// module reset
                        0, 0,					// module apb reset
                        NOT_RESET),
	COMP_MUX_CONF("eth0_extphy_clk", eth0_parent, 25000000, 0,
                        0, 0, 0,                                // clk mux
                        0, 0, 0,                                // clk div
                        CG_MAC_CONFIG_REG_OFFSET, BIT7,		// module clk gating
                        0, 0,					// module apb gating
                        NOT_ENABLE,
                        0, 0,					// module reset
                        0, 0,					// module apb reset
                        NOT_RESET),
	COMP_MUX_CONF("fcd00000.eth1", eth1_parent, 250000000, 0,
                        0, 0, 0,                                // clk mux
                        0, 0, 0,                                // clk div
                        CG_BUS_CLK_GATING_REG0_OFFSET, BIT17,	// module clk gating
                        CG_APB_CLK_GATING_REG0_OFFSET, BIT10,	// module apb gating
                        NOT_ENABLE,
                        CG_MODULE_RESET_REG1_OFFSET, BIT2,	// module reset
                        CG_APB_RESET_REG0_OFFSET, BIT10,	// module apb reset
                        NOT_RESET),
	COMP_MUX_CONF("rmii1_refclk_i", eth1_parent, 50000000, 0,
                        0, 0, 0,                                // clk mux
                        0, 0, 0,                                // clk div
                        CG_MAC_CONFIG_REG_OFFSET, BIT20,	// module clk gating
                        0, 0,					// module apb gating
                        NOT_ENABLE,
                        0, 0,					// module reset
                        0, 0,					// module apb reset
                        NOT_RESET),
	COMP_MUX_CONF("eth1_rxclk_i", eth1_parent, 125000000, 0,
                        0, 0, 0,                                // clk mux
                        0, 0, 0,                                // clk div
                        CG_MAC_CONFIG_REG_OFFSET, BIT21,	// module clk gating
                        0, 0,					// module apb gating
                        NOT_ENABLE,
                        0, 0,					// module reset
                        0, 0,					// module apb reset
                        NOT_RESET),
	COMP_MUX_CONF("eth1_txclk_i", eth1_parent, 125000000, 0,
                        0, 0, 0,                                // clk mux
                        0, 0, 0,                                // clk div
                        CG_MAC_CONFIG_REG_OFFSET, BIT22,	// module clk gating
                        0, 0,					// module apb gating
                        NOT_ENABLE,
                        0, 0,					// module reset
                        0, 0,					// module apb reset
                        NOT_RESET),
	COMP_MUX_CONF("eth1_extphy_clk", eth1_parent, 25000000, 0,
                        0, 0, 0,                                // clk mux
                        0, 0, 0,                                // clk div
                        CG_MAC_CONFIG_REG_OFFSET, BIT23,	// module clk gating
                        0, 0,					// module apb gating
                        NOT_ENABLE,
                        0, 0,					// module reset
                        0, 0,					// module apb reset
                        NOT_RESET),
	COMP_MUX_CONF("fe840000.ahbc", root_clk_parent, 12000000, 0,
	          0, 0, 0,
	          0, 0, 0,
	          0, 0, 0, 0, 0,
	          CG_MODULE_RESET_REG1_OFFSET, BIT14, 0, 0, NOT_RESET),
	COMP_MUX_CONF("fe880000.rtc", rtc_clk_parent, 32768, 0,
			0, 0, 0,				// clk mux
			0, 0, 0,				// clk div
			0, 0,					// module clk gating
			CG_APB_CLK_GATING_REG1_OFFSET, BIT19,	// module apb gating
			NOT_ENABLE,
			0, 0,					// module reset
			CG_APB_RESET_REG1_OFFSET, BIT10,	// module apb reset
			NOT_RESET),
	COMP_MUX_CONF("fe8a0000.wdt", root_clk_parent, 12000000, 0,
			0, 0, 0,				// clk mux
			0, 0, 0,				// clk div
			0, 0,					// module clk gating
			CG_APB_CLK_GATING_REG2_OFFSET, BIT15,	// module apb gating
			NOT_ENABLE,
			0, 0,					// module reset
			CG_APB_RESET_REG1_OFFSET, BIT28,	// module apb reset
			NOT_RESET),
	COMP_MUX_CONF("f8900000.cc", root_clk_parent, 12000000, 0,
	          0, 0, 0,
	          0, 0, 0,
	          0, 0, 0, 0, NOT_ENABLE,
	          CG_CPU_DSP_RESET_REG_OFFSET, BIT2, 0, 0, NOT_RESET),

	COMP_MUX_CONF("ssp_main_clk", sspmain_clk_parent, 12288000, 0,
	          0, 0, 0,
	          CG_SDC_UART_SSP_CKCTRL_REG_OFFSET, BIT24, WID6,
	          0, 0, 0, 0, 0,
	          0, 0, 0, 0, 0),

	COMP_MUX_CONF("ssph_main_clk", sspmain_clk_parent, 12288000, 0,
	          0, 0, 0,
	          CG_HDMI_SPI_CKCTRL_REG_OFFSET, BIT0, WID6,
	          0, 0, 0, 0, 0,
	          0, 0, 0, 0, 0),

	COMP_MUX_CONF("fa300000.ssp0",ssp_clk_parent, 12288000, 0,
	          CG_SSP_CKCTRL_REG0_OFFSET, BIT0, WID2,
	          CG_SSP_CKCTRL_REG0_OFFSET, BIT2, WID6,
	          CG_BUS_CLK_GATING_REG3_OFFSET, BIT8, CG_APB_CLK_GATING_REG2_OFFSET, BIT0, NOT_ENABLE,
	          0, 0, CG_APB_RESET_REG1_OFFSET, BIT13, DO_RESET),

	COMP_MUX_CONF("fa320000.ssp1",ssp_clk_parent, 12288000, 0,
	          CG_SSP_CKCTRL_REG0_OFFSET, BIT8, WID2,
	          CG_SSP_CKCTRL_REG0_OFFSET, BIT10, WID6,
	          CG_BUS_CLK_GATING_REG3_OFFSET, BIT9, CG_APB_CLK_GATING_REG2_OFFSET, BIT1, NOT_ENABLE,
	          0, 0, CG_APB_RESET_REG1_OFFSET, BIT14, DO_RESET),

	COMP_MUX_CONF("fe500000.ssp2",ssp_clk_parent, 12288000, 0,
	          CG_SSP_CKCTRL_REG1_OFFSET, BIT16, WID2,
	          CG_SSP_CKCTRL_REG1_OFFSET, BIT18, WID6,
	          CG_BUS_CLK_GATING_REG3_OFFSET, BIT15, CG_APB_CLK_GATING_REG2_OFFSET, BIT18, NOT_ENABLE,
	          0, 0, CG_APB_RESET_REG1_OFFSET, BIT11, DO_RESET),

	COMP_MUX_CONF("fe4c0000.ssph",ssph_clk_parent, 12288000, 0,
	          CG_SSP_CKCTRL_REG1_OFFSET, BIT0, WID2,
	          CG_SSP_CKCTRL_REG1_OFFSET, BIT2, WID6,
	          CG_BUS_CLK_GATING_REG3_OFFSET, BIT12, CG_APB_CLK_GATING_REG2_OFFSET, BIT4, NOT_ENABLE,
	          0, 0, CG_APB_RESET_REG1_OFFSET, BIT17, DO_RESET),

	COMP_MUX_CONF("fe4c0000.ssphm",ssph_clk_parent, 12288000, 0,
	          0, 0, 0,
	          0, 0, 0,
	          0, 0, 0, 0, 0,
	          0, 0, 0, 0, 0),

	COMP_MUX_CONF("fa100000.dma200", root_clk_parent, 12000000, 0,
	          0, 0, 0,
	          0, 0, 0,
	          CG_BUS_CLK_GATING_REG2_OFFSET, BIT15, 0, 0, NOT_ENABLE,
	          CG_MODULE_RESET_REG1_OFFSET, BIT15, 0, 0, DO_RESET),

	COMP_MUX_CONF("fa200000.dma201", root_clk_parent, 12000000, 0,
	          0, 0, 0,
	          0, 0, 0,
	          CG_BUS_CLK_GATING_REG2_OFFSET, BIT16, 0, 0, NOT_ENABLE,
	          CG_MODULE_RESET_REG1_OFFSET, BIT16, 0, 0, DO_RESET),

	COMP_MUX_CONF("fe540000.irdet", root_clk_parent, 12000000, 0,
	          0, 0, 0,
	          0, 0, 0,
	          0, 0, CG_APB_CLK_GATING_REG1_OFFSET, BIT16, DO_ENABLE,
	          0, 0, CG_APB_RESET_REG1_OFFSET, BIT8, DO_RESET),

	COMP_MUX_CONF("fdb00000.lcd210", lcd210_clk_parent, 27000000, 0,
		      CG_LCD_CKCTRL_REG_OFFSET, BIT8, WID2,
	          CG_LCD_CKCTRL_REG_OFFSET, BIT10, WID6,
	          CG_BUS_CLK_GATING_REG0_OFFSET, BIT14, CG_APB_CLK_GATING_REG0_OFFSET, BIT7, DO_ENABLE,
	          CG_MODULE_RESET_REG0_OFFSET, BIT5, CG_APB_RESET_REG0_OFFSET, BIT7, NOT_RESET),

	COMP_MUX_CONF("fd700000.GPD", root_clk_parent, 12000000, 0,
	          0, 0, 0,
	          0, 0, 0,
	          0, 0, CG_APB_CLK_GATING_REG0_OFFSET, BIT15, DO_ENABLE,
	          CG_MODULE_RESET_REG0_OFFSET, BIT10, CG_APB_RESET_REG0_OFFSET, BIT15, NOT_RESET),

	COMP_MUX_CONF("fda00000.lcd310", lcd310_clk_parent, 148500000, 0,
		      CG_LCD_CKCTRL_REG_OFFSET, BIT16, WID2,
	          CG_LCD_CKCTRL_REG_OFFSET, BIT18, WID6,
	          CG_BUS_CLK_GATING_REG0_OFFSET, BIT15, CG_APB_CLK_GATING_REG0_OFFSET, BIT8, DO_ENABLE,
	          CG_MODULE_RESET_REG0_OFFSET, BIT6, CG_APB_RESET_REG0_OFFSET, BIT8, NOT_RESET),

#ifdef CONFIG_NVT_FPGA_EMULATION
	COMP_MUX_CONF("fe300000.i2c0", i2c_clk_parent, 24000000, 0,
	          0, 0, 0,
	          0, 0, 0,
	          0, 0, CG_APB_CLK_GATING_REG1_OFFSET, BIT8, NOT_ENABLE,
	          0, 0, CG_APB_RESET_REG1_OFFSET, BIT0, DO_RESET),
	COMP_MUX_CONF("fe320000.i2c1", i2c_clk_parent, 24000000, 0,
	          0, 0, 0,
	          0, 0, 0,
	          0, 0, CG_APB_CLK_GATING_REG1_OFFSET, BIT9, NOT_ENABLE,
	          0, 0, CG_APB_RESET_REG1_OFFSET, BIT1, DO_RESET),
	COMP_MUX_CONF("fe340000.i2c2", i2c_clk_parent, 24000000, 0,
	          0, 0, 0,
	          0, 0, 0,
	          0, 0, CG_APB_CLK_GATING_REG1_OFFSET, BIT10, NOT_ENABLE,
	          0, 0, CG_APB_RESET_REG1_OFFSET, BIT2, DO_RESET),
	COMP_MUX_CONF("fe360000.i2c3", i2c_clk_parent, 24000000, 0,
	          0, 0, 0,
	          0, 0, 0,
	          0, 0, CG_APB_CLK_GATING_REG1_OFFSET, BIT11, NOT_ENABLE,
	          0, 0, CG_APB_RESET_REG1_OFFSET, BIT3, DO_RESET),
	COMP_MUX_CONF("fe380000.i2c4", i2c_clk_parent, 24000000, 0,
	          0, 0, 0,
	          0, 0, 0,
	          0, 0, CG_APB_CLK_GATING_REG1_OFFSET, BIT12, NOT_ENABLE,
	          0, 0, CG_APB_RESET_REG1_OFFSET, BIT4, DO_RESET),
	COMP_MUX_CONF("fe3a0000.i2c5", i2c_clk_parent, 24000000, 0,
	          0, 0, 0,
	          0, 0, 0,
	          0, 0, CG_APB_CLK_GATING_REG1_OFFSET, BIT13, NOT_ENABLE,
	          0, 0, CG_APB_RESET_REG1_OFFSET, BIT5, DO_RESET),
	COMP_MUX_CONF("fe200000.uart", uart_clk_parent, 48000000, 0,
	          CG_SDC_UART_SSP_CKCTRL_REG_OFFSET, BIT6, WID2,
	          CG_SDC_UART_SSP_CKCTRL_REG_OFFSET, BIT0, WID6,
	          0, 0, CG_APB_CLK_GATING_REG2_OFFSET, BIT9, DO_ENABLE,
	          0, 0, CG_APB_RESET_REG1_OFFSET, BIT22, NOT_RESET),
	COMP_MUX_CONF("fe220000.uart", uart_clk_parent, 48000000, 0,
	          CG_SDC_UART_SSP_CKCTRL_REG_OFFSET, BIT6, WID2,
	          CG_SDC_UART_SSP_CKCTRL_REG_OFFSET, BIT0, WID6,
	          0, 0, CG_APB_CLK_GATING_REG2_OFFSET, BIT10, DO_ENABLE,
	          0, 0, CG_APB_RESET_REG1_OFFSET, BIT23, DO_RESET),
	COMP_MUX_CONF("fe240000.uart", uart_clk_parent, 48000000, 0,
	          CG_SDC_UART_SSP_CKCTRL_REG_OFFSET, BIT6, WID2,
	          CG_SDC_UART_SSP_CKCTRL_REG_OFFSET, BIT0, WID6,
	          0, 0, CG_APB_CLK_GATING_REG2_OFFSET, BIT11, DO_ENABLE,
	          0, 0, CG_APB_RESET_REG1_OFFSET, BIT24, DO_RESET),
	COMP_MUX_CONF("fe260000.uart", uart_clk_parent, 48000000, 0,
	          CG_SDC_UART_SSP_CKCTRL_REG_OFFSET, BIT6, WID2,
	          CG_SDC_UART_SSP_CKCTRL_REG_OFFSET, BIT0, WID6,
	          0, 0, CG_APB_CLK_GATING_REG2_OFFSET, BIT12, DO_ENABLE,
	          0, 0, CG_APB_RESET_REG1_OFFSET, BIT25, DO_RESET),
	COMP_MUX_CONF("fe400000.gpio", root_clk_parent, 24000000, 0,
	          0, 0, 0,
	          0, 0, 0,
	          0, 0, CG_APB_CLK_GATING_REG1_OFFSET, BIT3, DO_ENABLE,
	          0, 0, CG_APB_RESET_REG0_OFFSET, BIT27, DO_RESET),
	COMP_MUX_CONF("fe420000.gpio", root_clk_parent, 24000000, 0,
	          0, 0, 0,
	          0, 0, 0,
	          0, 0, CG_APB_CLK_GATING_REG1_OFFSET, BIT4, DO_ENABLE,
	          0, 0, CG_APB_RESET_REG0_OFFSET, BIT28, DO_RESET),
	COMP_MUX_CONF("fe440000.gpio", root_clk_parent, 24000000, 0,
	          0, 0, 0,
	          0, 0, 0,
	          0, 0, CG_APB_CLK_GATING_REG1_OFFSET, BIT5, DO_ENABLE,
	          0, 0, CG_APB_RESET_REG0_OFFSET, BIT29, DO_RESET),
	COMP_MUX_CONF("fe640000.gpio", root_clk_parent, 24000000, 0,
	          0, 0, 0,
	          0, 0, 0,
	          0, 0, CG_APB_CLK_GATING_REG1_OFFSET, BIT24, DO_ENABLE,
	          0, 0, CG_APB_RESET_REG0_OFFSET, BIT31, DO_RESET),
	COMP_MUX_CONF("fe8c0000.timer", root_clk_parent, 24000000, 0,
	          0, 0, 0,
	          0, 0, 0,
	          0, 0, CG_APB_CLK_GATING_REG2_OFFSET, BIT7, DO_ENABLE,
	          0, 0, CG_APB_RESET_REG1_OFFSET, BIT20, DO_RESET),
	COMP_MUX_CONF("fa600000.mmc", mmc_clk_parent, 24000000, 0,
	          0, 0, 0,
	          CG_SDC_UART_SSP_CKCTRL_REG_OFFSET, BIT8, WID6,
	          CG_BUS_CLK_GATING_REG2_OFFSET, BIT10, 0, 0, NOT_ENABLE,
	          CG_MODULE_RESET_REG1_OFFSET, BIT10, CG_APB_RESET_REG1_OFFSET, BIT25, DO_RESET),
	COMP_MUX_CONF("fa900000.nand", spi_clk_parent, 12000000, 0,
	          0, 0, 0,
	          0, 0, 0,
	          CG_BUS_CLK_GATING_REG2_OFFSET, BIT9, 0, 0, NOT_ENABLE,
	          CG_MODULE_RESET_REG1_OFFSET, BIT11, 0, 0, DO_RESET),
	COMP_MUX_CONF("vcap_aclk_m0", vcap_aclk_m_parent, 20000000, 0,
	          0, 0, 0,
	          0, 0, 0,
	          CG_BUS_CLK_GATING_REG1_OFFSET, BIT6, 0, 0, NOT_ENABLE,
	          0, 0, 0, 0, 0),
	COMP_MUX_CONF("vcap_aclk_m1", vcap_aclk_m_parent, 20000000, 0,
	          0, 0, 0,
	          0, 0, 0,
	          CG_BUS_CLK_GATING_REG1_OFFSET, BIT7, 0, 0, NOT_ENABLE,
	          0, 0, 0, 0, 0),
	COMP_MUX_CONF("vcap_aclk_m2", vcap_aclk_m_parent, 20000000, 0,
	          0, 0, 0,
	          0, 0, 0,
	          CG_BUS_CLK_GATING_REG1_OFFSET, BIT8, 0, 0, NOT_ENABLE,
	          0, 0, 0, 0, 0),
	COMP_MUX_CONF("vcap_aclk_s", vcap_aclk_s_parent, 12000000, 0,
	          0, 0, 0,
	          0, 0, 0,
	          CG_BUS_CLK_GATING_REG1_OFFSET, BIT10, 0, 0, NOT_ENABLE,
	          0, 0, 0, 0, 0),
	COMP_MUX_CONF("vcap_mclk", vcap_mclk_parent, 20000000, 0,
	          CG_VCAP_CKCTRL_REG_OFFSET, BIT31, WID1,
	          0, 0, 0,
	          CG_BUS_CLK_GATING_REG1_OFFSET, BIT9, 0, 0, NOT_ENABLE,
	          CG_MODULE_RESET_REG0_OFFSET, BIT26, 0, 0, DO_RESET),
	COMP_MUX_CONF("vcap_patclk", vcap_patclk_parent, 24000000, 0,
	          CG_VCAP_CKCTRL_REG_OFFSET, BIT28, WID2,
	          CG_VCAP_CKCTRL_REG_OFFSET, BIT20, WID6,
	          CG_BUS_CLK_GATING_REG1_OFFSET, BIT13, 0, 0, NOT_ENABLE,
	          0, 0, 0, 0, 0),
	COMP_MUX_CONF("fd100000.osg", osg_clk_parent, 16000000, 0, /* mclk default is pll0 with 16M */
           	  CG_OSG_CKCTRL_REG_OFFSET, BIT31, WID1,	/* mux_reg */
         	  0, 0, 0,	/* div_reg */
         	  CG_BUS_CLK_GATING_REG0_OFFSET/*AXI*/, BIT21, CG_APB_CLK_GATING_REG0_OFFSET/*APB*/, BIT11, NOT_ENABLE,	/* bus_clk_regj. AXI + APB */
         	  CG_MODULE_RESET_REG0_OFFSET/*0x50*/, BIT9, CG_APB_RESET_REG0_OFFSET/*0x58*/, BIT11, DO_RESET), /* module_rst_reg */
	COMP_MUX_CONF("ext0_clk", ext0_clk_parent, 27000000, 0,
	          CG_EXT_CKCTRL_REG_OFFSET, BIT1, WID2,
	          CG_EXT_CKCTRL_REG_OFFSET, BIT3, WID5,
	          CG_EXT_CKCTRL_REG_OFFSET, BIT0, 0, 0, 0,
	          0, 0, 0, 0, 0),
	COMP_MUX_CONF("ext1_clk", ext1_clk_parent, 27000000, 0,
	          CG_EXT_CKCTRL_REG_OFFSET, BIT9, WID2,
	          CG_EXT_CKCTRL_REG_OFFSET, BIT11, WID5,
	          CG_EXT_CKCTRL_REG_OFFSET, BIT8, 0, 0, 0,
	          0, 0, 0, 0, 0),
	COMP_MUX_CONF("faa00000.gm2d", gm2d_clk_parent, 20000000, 0, /* hclk default is 12MHz */
                  0, 0, 0,	/* mux_reg */
                  0, 0, 0,	/* div_reg */
                  CG_BUS_CLK_GATING_REG2_OFFSET/*0x68*/, BIT12, 0, 0, NOT_ENABLE,	/* bus_clk_reg.*/
                  CG_MODULE_RESET_REG1_OFFSET  /*0x54*/, BIT12, 0, 0, DO_RESET), /* module_rst_reg */
	COMP_MUX_CONF("fd500000.ssca", ssca_clk_parent, 20000000, 0, /* hclk default is 12MHz */
                  CG_SSCA_CKCTRL_REG_OFFSET/*0x160*/, BIT31, WID1,	/* mux_reg */
                  0, 0, 0,	/* div_reg */
                  CG_APB_CLK_GATING_REG0_OFFSET/*0x60*/, BIT27, CG_APB_CLK_GATING_REG0_OFFSET /*0x70*/, BIT16, NOT_ENABLE,	/* bus_clk_reg.*/
                  CG_MODULE_RESET_REG0_OFFSET  /*0x50*/, BIT12, CG_APB_RESET_REG0_OFFSET /*0x58*/, BIT16, DO_RESET), /* module_rst_reg */
	COMP_MUX_CONF("vpe_clk", vpe_clk_parent, 16000000, 0, /* mclk default is pll0 with 16M */
           	  CG_VPE536_CKCTRL_REG_OFFSET, BIT31, WID1,	/* mux_reg */
			  0, 0, 0,
         	  CG_BUS_CLK_GATING_REG0_OFFSET, BIT30, CG_APB_CLK_GATING_REG2_OFFSET, BIT17, NOT_ENABLE,
			  CG_MODULE_RESET_REG0_OFFSET, BIT15, CG_APB_RESET_REG0_OFFSET, BIT26, DO_RESET),
	COMP_MUX_CONF("dei_aclk", dei_aclk_parent, 16000000, 0, /* mclk default is pll0 with 16M */
           	  0, 0, 0,	/* mux_reg */
			  0, 0, 0,
			  CG_BUS_CLK_GATING_REG1_OFFSET, BIT11, 0, 0, NOT_ENABLE,
			  0, 0, 0, 0, 0),
	COMP_MUX_CONF("dei_mclk", dei_mclk_parent, 16000000, 0, /* mclk default is pll0 with 16M */
           	  CG_VPE316_CKCTRL_REG_OFFSET, BIT31, WID1,	/* mux_reg */
			  0, 0, 0,
			  CG_BUS_CLK_GATING_REG1_OFFSET, BIT12, CG_APB_CLK_GATING_REG0_OFFSET, BIT19, NOT_ENABLE,
			  CG_MODULE_RESET_REG0_OFFSET, BIT28, CG_APB_RESET_REG0_OFFSET, BIT19, DO_RESET),
	COMP_MUX_CONF("fac00000.jpg", jpg_clk_parent, 300000000, 0,   /* name, parent_names, current_rate, parent_idx */
		      0, 0, 0,                                        /* mux_reg, bit, bit_width */
		      0, 0, 0, 				              /* div_reg, bit, bit_width */
		      CG_BUS_CLK_GATING_REG2_OFFSET, BIT1, 0, 0, NOT_ENABLE, /* bus_clk_reg   , bit, apb_clk_reg, bit, do_enable */
		      CG_MODULE_RESET_REG1_OFFSET, BIT3, 0, 0, DO_RESET),
	COMP_MUX_CONF("venc_aclk", venc_aclk_parent, 12000000, 0,   /* name, parent_names, current_rate, parent_idx */
		      0, 0, 0,                            /* mux_reg, bit, bit_width */
		      0, 0, 0, 				              /* div_reg, bit, bit_width */
	          CG_BUS_CLK_GATING_REG1_OFFSET, BIT3, 0, 0, NOT_ENABLE,
	          CG_MODULE_RESET_REG0_OFFSET, BIT27, 0, 0, DO_RESET),
	COMP_MUX_CONF("venc_mclk", venc_mclk_parent, 20000000, 0,   /* name, parent_names, current_rate, parent_idx */
		      CG_VENC_CKCTRL_REG_OFFSET, BIT31, WID1,           /* mux_reg, bit, bit_width */
		      0, 0, 0, 				              /* div_reg, bit, bit_width */
	          0, 0, 0, 0, 0,
	          CG_MODULE_RESET_REG1_OFFSET, BIT19, 0, 0, DO_RESET),
	COMP_MUX_CONF("venc_pclk", venc_pclk_parent, 12000000, 0,   /* name, parent_names, current_rate, parent_idx */
		      0, 0, 0,                            /* mux_reg, bit, bit_width */
		      0, 0, 0, 				              /* div_reg, bit, bit_width */
	          CG_APB_CLK_GATING_REG0_OFFSET, BIT28, 0, 0, NOT_ENABLE,
	          CG_APB_RESET_REG0_OFFSET, BIT17, 0, 0, DO_RESET),
	COMP_MUX_CONF("h264d_mclk", h264d_mclk_parent, 16000000, 0,
			  CG_H264D_CKCTRL_REG_OFFSET, BIT31, WID1,
			  0, 0, 0,
			  0, 0, 0, 0, 0,
			  CG_MODULE_RESET_REG0_OFFSET, BIT7, 0, 0, DO_RESET),
	COMP_MUX_CONF("h265d_mclk", h265d_mclk_parent, 16000000, 0,
			  CG_H265D_CKCTRL_REG_OFFSET, BIT30, WID1,
			  0, 0, 0,
			  0, 0, 0, 0, 0,
			  CG_MODULE_RESET_REG0_OFFSET, BIT8, 0, 0, DO_RESET),
	COMP_MUX_CONF("h26xd_pclk", h26xd_pclk_parent, 12000000, 0,
			  0, 0, 0,
			  0, 0, 0,
			  CG_APB_CLK_GATING_REG0_OFFSET, BIT18, 0, 0, NOT_ENABLE,
			  CG_APB_RESET_REG0_OFFSET, BIT18, 0, 0, DO_RESET),
	COMP_MUX_CONF("h264d_aclk", h264d_aclk_parent, 16000000, 0,
			  0, 0, 0,
			  0, 0, 0,
			  CG_BUS_CLK_GATING_REG0_OFFSET, BIT20, 0, 0, NOT_ENABLE,
			  0, 0, 0, 0, 0),
	COMP_MUX_CONF("h265d_aclk", h265d_aclk_parent, 16000000, 0,
			  CG_H265D_CKCTRL_REG_OFFSET, BIT30, WID1,
			  0, 0, 0,
			  CG_BUS_CLK_GATING_REG0_OFFSET, BIT29, 0, 0, NOT_ENABLE,
			  0, 0, 0, 0, 0),
#else
	COMP_MUX_CONF("pll4div2", pll4div2_clk_parent, 150000000, 0,
	          0, 0, 0,
	          0, 0, SWDIV2,
	          0, 0, 0, 0, 0,
	          0, 0, 0, 0, 0),
	COMP_MUX_CONF("fe300000.i2c0", i2c_clk_parent, 150000000, 0,
	          0, 0, 0,
	          0, 0, 0,
	          0, 0, CG_APB_CLK_GATING_REG1_OFFSET, BIT8, NOT_ENABLE,
	          0, 0, CG_APB_RESET_REG1_OFFSET, BIT0, DO_RESET),
	COMP_MUX_CONF("fe320000.i2c1", i2c_clk_parent, 150000000, 0,
	          0, 0, 0,
	          0, 0, 0,
	          0, 0, CG_APB_CLK_GATING_REG1_OFFSET, BIT9, NOT_ENABLE,
	          0, 0, CG_APB_RESET_REG1_OFFSET, BIT1, DO_RESET),
	COMP_MUX_CONF("fe340000.i2c2", i2c_clk_parent, 150000000, 0,
	          0, 0, 0,
	          0, 0, 0,
	          0, 0, CG_APB_CLK_GATING_REG1_OFFSET, BIT10, NOT_ENABLE,
	          0, 0, CG_APB_RESET_REG1_OFFSET, BIT2, DO_RESET),
	COMP_MUX_CONF("fe360000.i2c3", i2c_clk_parent, 150000000, 0,
	          0, 0, 0,
	          0, 0, 0,
	          0, 0, CG_APB_CLK_GATING_REG1_OFFSET, BIT11, NOT_ENABLE,
	          0, 0, CG_APB_RESET_REG1_OFFSET, BIT3, DO_RESET),
	COMP_MUX_CONF("fe380000.i2c4", i2c_clk_parent, 150000000, 0,
	          0, 0, 0,
	          0, 0, 0,
	          0, 0, CG_APB_CLK_GATING_REG1_OFFSET, BIT12, NOT_ENABLE,
	          0, 0, CG_APB_RESET_REG1_OFFSET, BIT4, DO_RESET),
	COMP_MUX_CONF("fe3a0000.i2c5", i2c_clk_parent, 150000000, 0,
	          0, 0, 0,
	          0, 0, 0,
	          0, 0, CG_APB_CLK_GATING_REG1_OFFSET, BIT13, NOT_ENABLE,
	          0, 0, CG_APB_RESET_REG1_OFFSET, BIT5, DO_RESET),
	COMP_MUX_CONF("fe200000.uart", uart_clk_parent, 48000000, 0,
	          CG_SDC_UART_SSP_CKCTRL_REG_OFFSET, BIT6, WID2,
	          CG_SDC_UART_SSP_CKCTRL_REG_OFFSET, BIT0, WID6,
	          0, 0, CG_APB_CLK_GATING_REG2_OFFSET, BIT9, DO_ENABLE,
	          0, 0, CG_APB_RESET_REG1_OFFSET, BIT22, NOT_RESET),
	COMP_MUX_CONF("fe220000.uart", uart_clk_parent, 48000000, 0,
	          CG_SDC_UART_SSP_CKCTRL_REG_OFFSET, BIT6, WID2,
	          CG_SDC_UART_SSP_CKCTRL_REG_OFFSET, BIT0, WID6,
	          0, 0, CG_APB_CLK_GATING_REG2_OFFSET, BIT10, DO_ENABLE,
	          0, 0, CG_APB_RESET_REG1_OFFSET, BIT23, DO_RESET),
	COMP_MUX_CONF("fe240000.uart", uart_clk_parent, 48000000, 0,
	          CG_SDC_UART_SSP_CKCTRL_REG_OFFSET, BIT6, WID2,
	          CG_SDC_UART_SSP_CKCTRL_REG_OFFSET, BIT0, WID6,
	          0, 0, CG_APB_CLK_GATING_REG2_OFFSET, BIT11, DO_ENABLE,
	          0, 0, CG_APB_RESET_REG1_OFFSET, BIT24, DO_RESET),
	COMP_MUX_CONF("fe260000.uart", uart_clk_parent, 48000000, 0,
	          CG_SDC_UART_SSP_CKCTRL_REG_OFFSET, BIT6, WID2,
	          CG_SDC_UART_SSP_CKCTRL_REG_OFFSET, BIT0, WID6,
	          0, 0, CG_APB_CLK_GATING_REG2_OFFSET, BIT12, DO_ENABLE,
	          0, 0, CG_APB_RESET_REG1_OFFSET, BIT25, DO_RESET),
	COMP_MUX_CONF("fe400000.gpio", root_clk_parent, 150000000, 0,
	          0, 0, 0,
	          0, 0, 0,
	          0, 0, CG_APB_CLK_GATING_REG1_OFFSET, BIT3, DO_ENABLE,
	          0, 0, CG_APB_RESET_REG0_OFFSET, BIT27, DO_RESET),
	COMP_MUX_CONF("fe420000.gpio", root_clk_parent, 150000000, 0,
	          0, 0, 0,
	          0, 0, 0,
	          0, 0, CG_APB_CLK_GATING_REG1_OFFSET, BIT4, DO_ENABLE,
	          0, 0, CG_APB_RESET_REG0_OFFSET, BIT28, DO_RESET),
	COMP_MUX_CONF("fe440000.gpio", root_clk_parent, 150000000, 0,
	          0, 0, 0,
	          0, 0, 0,
	          0, 0, CG_APB_CLK_GATING_REG1_OFFSET, BIT5, DO_ENABLE,
	          0, 0, CG_APB_RESET_REG0_OFFSET, BIT29, DO_RESET),
	COMP_MUX_CONF("fe640000.gpio", root_clk_parent, 150000000, 0,
	          0, 0, 0,
	          0, 0, 0,
	          0, 0, CG_APB_CLK_GATING_REG1_OFFSET, BIT24, DO_ENABLE,
	          0, 0, CG_APB_RESET_REG0_OFFSET, BIT31, DO_RESET),
	COMP_MUX_CONF("fe8c0000.timer", root_clk_parent, 150000000, 0,
	          0, 0, 0,
	          0, 0, 0,
	          0, 0, CG_APB_CLK_GATING_REG2_OFFSET, BIT7, DO_ENABLE,
	          0, 0, CG_APB_RESET_REG1_OFFSET, BIT20, DO_RESET),
	COMP_MUX_CONF("pll6div2", pll6div2_clk_parent, 100000000, 0,
	          0, 0, 0,
	          0, 0, SWDIV2,
	          0, 0, 0, 0, 0,
	          0, 0, 0, 0, 0),
	COMP_MUX_CONF("fa600000.mmc", mmc_clk_parent, 100000000, 0,
	          0, 0, 0,
	          CG_SDC_UART_SSP_CKCTRL_REG_OFFSET, BIT8, WID6,
	          CG_BUS_CLK_GATING_REG2_OFFSET, BIT10, 0, 0, NOT_ENABLE,
	          CG_MODULE_RESET_REG1_OFFSET, BIT10, CG_APB_RESET_REG1_OFFSET, BIT25, DO_RESET),
	COMP_MUX_CONF("fa900000.nand", spi_clk_parent, 150000000, 0,
	          CG_HDMI_SPI_CKCTRL_REG_OFFSET, BIT28, WID2,
	          0, 0, 0,
	          CG_BUS_CLK_GATING_REG2_OFFSET, BIT9, 0, 0, NOT_ENABLE,
	          CG_MODULE_RESET_REG1_OFFSET, BIT11, 0, 0, DO_RESET),
	COMP_MUX_CONF("vcap_aclk_m0", vcap_aclk_m_parent, 400000000, 0,
	          0, 0, 0,
	          0, 0, 0,
	          CG_BUS_CLK_GATING_REG1_OFFSET, BIT6, 0, 0, NOT_ENABLE,
	          0, 0, 0, 0, 0),
	COMP_MUX_CONF("vcap_aclk_m1", vcap_aclk_m_parent, 400000000, 0,
	          0, 0, 0,
	          0, 0, 0,
	          CG_BUS_CLK_GATING_REG1_OFFSET, BIT7, 0, 0, NOT_ENABLE,
	          0, 0, 0, 0, 0),
	COMP_MUX_CONF("vcap_aclk_m2", vcap_aclk_m_parent, 400000000, 0,
	          0, 0, 0,
	          0, 0, 0,
	          CG_BUS_CLK_GATING_REG1_OFFSET, BIT8, 0, 0, NOT_ENABLE,
	          0, 0, 0, 0, 0),
	COMP_MUX_CONF("vcap_aclk_s", vcap_aclk_s_parent, 300000000, 0,
	          0, 0, 0,
	          0, 0, 0,
	          CG_BUS_CLK_GATING_REG1_OFFSET, BIT10, 0, 0, NOT_ENABLE,
	          0, 0, 0, 0, 0),
	COMP_MUX_CONF("vcap_mclk", vcap_mclk_parent, 400000000, 0,
	          CG_VCAP_CKCTRL_REG_OFFSET, BIT31, WID1,
	          0, 0, 0,
	          CG_BUS_CLK_GATING_REG1_OFFSET, BIT9, 0, 0, NOT_ENABLE,
	          CG_MODULE_RESET_REG0_OFFSET, BIT26, 0, 0, DO_RESET),
	COMP_MUX_CONF("vcap_patclk", vcap_patclk_parent, 74250000, 0,
	          CG_VCAP_CKCTRL_REG_OFFSET, BIT28, WID2,
	          CG_VCAP_CKCTRL_REG_OFFSET, BIT20, WID6,
	          CG_BUS_CLK_GATING_REG1_OFFSET, BIT13, 0, 0, NOT_ENABLE,
	          0, 0, 0, 0, 0),
	COMP_MUX_CONF("fd100000.osg", osg_clk_parent, 40000000, 1, /* mclk default is pll1 with 400M */
                  CG_OSG_CKCTRL_REG_OFFSET, BIT31, WID1,	/* mux_reg */
                  0, 0, 0,	/* div_reg */
                  CG_BUS_CLK_GATING_REG0_OFFSET/*AXI*/, BIT21, CG_APB_CLK_GATING_REG0_OFFSET/*APB*/, BIT11, NOT_ENABLE,	/* bus_clk_regj. AXI + APB */
                  CG_MODULE_RESET_REG0_OFFSET/*0x50*/, BIT9, CG_APB_RESET_REG0_OFFSET/*0x58*/, BIT11, DO_RESET), /* module_rst_reg */
	COMP_MUX_CONF("ext0_clk", ext0_clk_parent, 27000000, 0,
	          CG_EXT_CKCTRL_REG_OFFSET, BIT1, WID2,
	          CG_EXT_CKCTRL_REG_OFFSET, BIT3, WID5,
	          CG_EXT_CKCTRL_REG_OFFSET, BIT0, 0, 0, 0,
	          0, 0, 0, 0, 0),
	COMP_MUX_CONF("ext1_clk", ext1_clk_parent, 27000000, 0,
	          CG_EXT_CKCTRL_REG_OFFSET, BIT9, WID2,
	          CG_EXT_CKCTRL_REG_OFFSET, BIT11, WID5,
	          CG_EXT_CKCTRL_REG_OFFSET, BIT8, 0, 0, 0,
	          0, 0, 0, 0, 0),
	COMP_MUX_CONF("faa00000.gm2d", gm2d_clk_parent, 300000000, 0, /* hclk default is 370MHz */
                  0, 0, 0,	/* mux_reg */
                  0, 0, 0,	/* div_reg */
                  CG_BUS_CLK_GATING_REG2_OFFSET/*0x68*/, BIT12, 0, 0, NOT_ENABLE,	/* bus_clk_reg.*/
                  CG_MODULE_RESET_REG1_OFFSET  /*0x54*/, BIT12, 0, 0, DO_RESET), /* module_rst_reg */
	COMP_MUX_CONF("fd500000.ssca", ssca_clk_parent, 40000000, 0, /* hclk default is 12MHz */
                  CG_SSCA_CKCTRL_REG_OFFSET/*0x160*/, BIT31, WID1,	/* mux_reg */
                  0, 0, 0,	/* div_reg */
                  CG_APB_CLK_GATING_REG0_OFFSET/*0x60*/, BIT27, CG_APB_CLK_GATING_REG0_OFFSET /*0x70*/, BIT16, NOT_ENABLE,	/* bus_clk_reg.*/
                  CG_MODULE_RESET_REG0_OFFSET  /*0x50*/, BIT12, CG_APB_RESET_REG0_OFFSET /*0x58*/, BIT16, DO_RESET), /* module_rst_reg */
	COMP_MUX_CONF("vpe_clk", vpe_clk_parent, 16000000, 0, /* mclk default is pll0 with 16M */
           	  CG_VPE536_CKCTRL_REG_OFFSET, BIT31, WID1,	/* mux_reg */
			  0, 0, 0,
         	  CG_BUS_CLK_GATING_REG0_OFFSET, BIT30, CG_APB_CLK_GATING_REG2_OFFSET, BIT17, NOT_ENABLE,
			  CG_MODULE_RESET_REG0_OFFSET, BIT15, CG_APB_RESET_REG0_OFFSET, BIT26, DO_RESET),
	COMP_MUX_CONF("dei_aclk", dei_aclk_parent, 16000000, 0, /* mclk default is pll0 with 16M */
           	  0, 0, 0,	/* mux_reg */
			  0, 0, 0,
			  CG_BUS_CLK_GATING_REG1_OFFSET, BIT11, 0, 0, NOT_ENABLE,
			  0, 0, 0, 0, 0),
	COMP_MUX_CONF("dei_mclk", dei_mclk_parent, 16000000, 0, /* mclk default is pll0 with 16M */
           	  CG_VPE316_CKCTRL_REG_OFFSET, BIT31, WID1,	/* mux_reg */
			  0, 0, 0,
			  CG_BUS_CLK_GATING_REG1_OFFSET, BIT12, CG_APB_CLK_GATING_REG0_OFFSET, BIT19, NOT_ENABLE,
			  CG_MODULE_RESET_REG0_OFFSET, BIT28, CG_APB_RESET_REG0_OFFSET, BIT19, DO_RESET),
	COMP_MUX_CONF("fac00000.jpg", jpg_clk_parent, 300000000, 0,   /* name, parent_names, current rate, parent_idx */
		      0, 0, 0,                                        /* mux_reg, bit, bit_width */
		      0, 0, 0, 				              /* div_reg, bit, bit_width */
		      CG_BUS_CLK_GATING_REG2_OFFSET, BIT1, 0, 0, NOT_ENABLE, /* bus_clk_reg   , bit, apb_clk_reg, bit, do_enable */
		      CG_MODULE_RESET_REG1_OFFSET, BIT3, 0, 0, DO_RESET),
	COMP_MUX_CONF("venc_aclk", venc_aclk_parent, 300000000, 0,   /* name, parent_names, current_rate, parent_idx */
		      0, 0, 0,                            /* mux_reg, bit, bit_width */
		      0, 0, 0, 				              /* div_reg, bit, bit_width */
	          CG_BUS_CLK_GATING_REG1_OFFSET, BIT3, 0, 0, NOT_ENABLE,
	          CG_MODULE_RESET_REG0_OFFSET, BIT27, 0, 0, DO_RESET),
	COMP_MUX_CONF("venc_mclk", venc_mclk_parent, 380000000, 0,   /* name, parent_names, current_rate, parent_idx */
		      CG_VENC_CKCTRL_REG_OFFSET, BIT31, WID1,            /* mux_reg, bit, bit_width */
		      0, 0, 0, 				              /* div_reg, bit, bit_width */
	          0, 0, 0, 0, 0,
	          CG_MODULE_RESET_REG1_OFFSET, BIT19, 0, 0, DO_RESET),
	COMP_MUX_CONF("venc_pclk", venc_pclk_parent, 300000000, 0,   /* name, parent_names, current_rate, parent_idx */
		      0, 0, 0,                            /* mux_reg, bit, bit_width */
		      0, 0, 0, 				              /* div_reg, bit, bit_width */
	          CG_APB_CLK_GATING_REG0_OFFSET, BIT28, 0, 0, NOT_ENABLE,
	          CG_APB_RESET_REG0_OFFSET, BIT17, 0, 0, DO_RESET),
	COMP_MUX_CONF("h264d_mclk", h264d_mclk_parent, 500000000, 0,
			  CG_H264D_CKCTRL_REG_OFFSET, BIT31, WID1,
			  0, 0, 0,
			  0, 0, 0, 0, 0,
			  CG_MODULE_RESET_REG0_OFFSET, BIT7, 0, 0, DO_RESET),
	COMP_MUX_CONF("h265d_mclk", h265d_mclk_parent, 350000000, 0,
			  CG_H265D_CKCTRL_REG_OFFSET, BIT31, WID1,
			  0, 0, 0,
			  0, 0, 0, 0, 0,
			  CG_MODULE_RESET_REG0_OFFSET, BIT8, 0, 0, DO_RESET),
	COMP_MUX_CONF("h26xd_pclk", h26xd_pclk_parent, 150000000, 0,
			  0, 0, 0,
			  0, 0, 0,
			  CG_APB_CLK_GATING_REG0_OFFSET, BIT18, 0, 0, NOT_ENABLE,
			  CG_APB_RESET_REG0_OFFSET, BIT18, 0, 0, DO_RESET),
	COMP_MUX_CONF("h264d_aclk", h264d_aclk_parent, 500000000, 0,
			  0, 0, 0,
			  0, 0, 0,
			  CG_BUS_CLK_GATING_REG0_OFFSET, BIT20, 0, 0, NOT_ENABLE,
			  0, 0, 0, 0, 0),
	COMP_MUX_CONF("h265d_aclk", h265d_aclk_parent, 350000000, 0,
			  CG_H265D_CKCTRL_REG_OFFSET, BIT30, WID1,
			  0, 0, 0,
			  CG_BUS_CLK_GATING_REG0_OFFSET, BIT29, 0, 0, NOT_ENABLE,
			  0, 0, 0, 0, 0),
	COMP_MUX_CONF("sce_aclk", sce_clk_parent, 400000000, 0,
		      CG_CRYPTO_CKCTRL_REG_OFFSET, BIT31, WID1,
		      0, 0, 0,
		      CG_BUS_CLK_GATING_REG2_OFFSET, BIT0, 0, 0, NOT_ENABLE,
		      CG_MODULE_RESET_REG1_OFFSET, BIT0, 0, 0, NOT_RESET),
	COMP_MUX_CONF("fec60000.crypto", sce_clk_parent, 400000000, 0,
		      0, 0, 0,
		      0, 0, 0,
		      CG_BUS_CLK_GATING_REG1_OFFSET, BIT2, CG_APB_CLK_GATING_REG1_OFFSET, BIT21, NOT_ENABLE,
		      CG_MODULE_RESET_REG0_OFFSET, BIT17, 0, 0, NOT_RESET),
	COMP_MUX_CONF("fec80000.hash", sce_clk_parent, 400000000, 0,
		      0, 0, 0,
		      0, 0, 0,
		      CG_BUS_CLK_GATING_REG1_OFFSET, BIT0, CG_APB_CLK_GATING_REG1_OFFSET, BIT2, NOT_ENABLE,
		      CG_MODULE_RESET_REG0_OFFSET, BIT18, 0, 0, NOT_RESET),
	COMP_MUX_CONF("feca0000.rsa", sce_clk_parent, 400000000, 0,
		      0, 0, 0,
		      0, 0, 0,
		      CG_BUS_CLK_GATING_REG1_OFFSET, BIT1, CG_APB_CLK_GATING_REG1_OFFSET, BIT20, NOT_ENABLE,
		      CG_MODULE_RESET_REG0_OFFSET, BIT19, 0, 0, NOT_RESET),
#endif
};

static void __init novatek_clock_init(struct device_node *node)
{
	int ret;

	pr_info("novatek_clock_init\n");

	spin_lock_init(&cg_lock);

	remap_base = of_iomap(node, 0);
	if (IS_ERR((void *)remap_base))
		pr_err("Failed to remap CKG registers!\n");
	else
		nvt_cg_base_remap(remap_base);

	ret = nvt_fixed_rate_clk_register(novatek_fixed_rate_clk, ARRAY_SIZE(novatek_fixed_rate_clk));
	if (ret < 0)
		pr_err("Failed to register fixed rate clk!\n");

	ret = nvt_pll_clk_register(novatek_pll, ARRAY_SIZE(novatek_pll), DEFAULT_PLL_DIV_VALUE, &cg_lock);
	if (ret < 0)
		pr_err("Failed to register pll clk!\n");

	ret = nvt_composite_mux_clk_register(novatek_cmux_clk, ARRAY_SIZE(novatek_cmux_clk), &cg_lock);
	if (ret < 0)
		pr_err("Failed to register composite mux clk!\n");

}

CLK_OF_DECLARE(nvt_core_clk, "nvt,core_clk", novatek_clock_init);