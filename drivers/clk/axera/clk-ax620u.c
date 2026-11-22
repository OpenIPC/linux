// SPDX-License-Identifier: GPL-2.0
/*
 * Axera AX620U ("axera,ax620x") clock driver
 *
 * The clock tables of the stock AX620U camera kernel (CONFIG_CLK_AX620U /
 * COMMON_CLK_AX620X, whose source was never released), recovered from
 * its image: same clock names, parents, register fields and IDs, built on
 * the SDK's axera_clk_* helpers (clk.c) as the stock driver is.
 *
 * Eight syscon blocks, each its own ID space (dt-bindings/clock/
 * ax620u-clock.h):
 *
 *   common_clk @0x02220000  PLL outputs, bus muxes (+0x60), always-on gates
 *   cpu_clk    @0x01900000  CPU and DMA fabric muxes, the DMA gate
 *   ap_clk     @0x049ba000  peripheral muxes (+0x0/+0x4), gates (+0x8..+0x14),
 *                           dividers (+0x18/+0x1c)
 *   flash_clk  @0x10030000  SPI-NAND (h_ssi) and eMMC clocks
 *   isp_clk    @0x02500000  ISP, MIPI RX and sensor reference clocks
 *   mm_clk     @0x04430000  display/TDP block
 *   vpu_clk    @0x04030000  VENC/VDEC/JENC
 *   dispc_clk  @0x044b0000  MIPI TX/DSI/D-PHY
 *
 * A gate's enable bit is read at its own register and set or cleared
 * through a set/clear pair of registers (gate_flags 0); gate_flags 1 is a
 * read-modify-write of the enable register itself. A divider's value N
 * divides by 2N (N = 0: by 1) and takes effect on a pulse of its update
 * bit (see axera_register_clkdiv()).
 *
 * The PLLs are fixed-rate clocks at the rates the first stage programs,
 * as in the stock driver, which does not read or reprogram them; nor does
 * it provide a reset controller. Its fixed-factor names are kept as they
 * are even where the name's rate is not the clock's (npll_800m is npll/2,
 * 600 MHz): the vendor code knows the clocks by these names.
 *
 * Not carried over: the stock driver also records every mux, gate and
 * divider register for axera_clk_suspend()/axera_clk_resume(), which only
 * its own platform suspend code calls.
 */
#include <linux/kernel.h>
#include <linux/clk-provider.h>
#include <linux/of.h>
#include <dt-bindings/clock/ax620u-clock.h>
#include "clk-axera.h"

/* common_clk */
static const char *pclk_top_sel_p[] __initdata = { "xtal_24m", "cpll_208m", "epll_250m", "cpll_312m" };
static const char *clk_csi_tx_sel_p[] __initdata = { "cpll_208m", "epll_333m", "npll_400m", "epll_500m", "npll_533m" };
static const char *clk_com_glb_sel_p[] __initdata = { "xtal_32k", "xtal_24m" };
static const char *aclk_vpu_top_sel_p[] __initdata = { "xtal_24m", "epll_500m", "npll_533m", "cpll_624m" };
static const char *aclk_nn_top_sel_p[] __initdata = { "xtal_24m", "epll_500m", "npll_533m", "cpll_624m" };
static const char *aclk_isp_top_sel_p[] __initdata = { "xtal_24m", "epll_500m", "npll_533m", "cpll_624m" };
static const char *aclk_cpu_top_sel_p[] __initdata = { "xtal_24m", "epll_500m", "npll_533m", "cpll_624m" };
static const char *debug_sel_p[] __initdata = { "xtal_24m", "xtal_32k", "rtc_32hz" };

static struct axera_fixed_rate_clock ax620u_fixed_rate_clks_common[] __initdata = {
	{AX620U_RTC_32HZ, "rtc_32hz", NULL, 0, 32,},
	{AX620U_XTAL_32K, "xtal_32k", NULL, 0, 32764,},
	{AX620U_XTAL_24M, "xtal_24m", NULL, 0, 24000000,},
	{AX620U_CPLL, "cpll", NULL, 0, 1248000000,},
	{AX620U_HPLL, "hpll", NULL, 0, 614400000,},
	{AX620U_NPLL, "npll", NULL, 0, 1200000000,},
	{AX620U_VPLL, "vpll", NULL, 0, 1188000000,},
	{AX620U_EPLL, "epll", NULL, 0, 1000000000,},
	{AX620U_DPLL, "dpll", NULL, 0, 1066000000,},
};

static struct axera_fixed_factor_clock ax620u_fixed_factor_clks_common[] __initdata = {
	{AX620U_CPLL_1248M, "cpll_1248m", "cpll", 1, 1, 0,},
	{AX620U_CPLL_624M, "cpll_624m", "cpll", 1, 2, 0,},
	{AX620U_CPLL_208M, "cpll_208m", "cpll", 1, 6, 0,},
	{AX620U_CPLL_312M, "cpll_312m", "cpll", 1, 4, 0,},
	{AX620U_CPLL_156M, "cpll_156m", "cpll", 1, 8, 0,},
	{AX620U_CPLL_DIV_16, "cpll_div_16", "cpll", 1, 16, 0,},
	{AX620U_CPLL_26M, "cpll_26m", "cpll", 1, 48, 0,},
	{AX620U_CPLL_39M, "cpll_39m", "cpll", 1, 32, 0,},
	{AX620U_CPLL_DIV_5, "cpll_div_5", "cpll", 1, 5, 0,},
	{AX620U_CPLL_19P2M, "cpll_19p2m", "cpll", 1, 65, 0,},
	{AX620U_CPLL_24M, "cpll_24m", "cpll", 1, 52, 0,},
	{AX620U_CPLL_12M, "cpll_12m", "cpll", 1, 104, 0,},
	{AX620U_HPLL_DIV_2, "hpll_div_2", "hpll", 1, 2, 0,},
	{AX620U_HPLL_DIV_4, "hpll_div_4", "hpll", 1, 4, 0,},
	{AX620U_HPLL_DIV_8, "hpll_div_8", "hpll", 1, 8, 0,},
	{AX620U_HPLL_DIV_16, "hpll_div_16", "hpll", 1, 16, 0,},
	{AX620U_HPLL_38P4M, "hpll_38p4m", "hpll", 1, 32, 0,},
	{AX620U_HPLL_DIV_3, "hpll_div_3", "hpll", 1, 3, 0,},
	{AX620U_HPLL_DIV_15, "hpll_div_15", "hpll", 1, 15, 0,},
	{AX620U_HPLL_40P96M, "hpll_40p96m", "hpll", 1, 30, 0,},
	{AX620U_HPLL_DIV_5, "hpll_div_5", "hpll", 1, 5, 0,},
	{AX620U_HPLL_49P152M, "hpll_49p152m", "hpll", 1, 25, 0,},
	{AX620U_HPLL_24P576M, "hpll_24p576m", "hpll", 1, 50, 0,},
	{AX620U_HPLL_16P384M, "hpll_16p384m", "hpll", 1, 75, 0,},
	{AX620U_NPLL_1600M, "npll_1600m", "npll", 1, 1, 0,},
	{AX620U_NPLL_800M, "npll_800m", "npll", 1, 2, 0,},
	{AX620U_NPLL_400M, "npll_400m", "npll", 1, 4, 0,},
	{AX620U_NPLL_200M, "npll_200m", "npll", 1, 8, 0,},
	{AX620U_NPLL_DIV_16, "npll_div_16", "npll", 1, 16, 0,},
	{AX620U_NPLL_DIV_32, "npll_div_32", "npll", 1, 32, 0,},
	{AX620U_NPLL_25M, "npll_25m", "npll", 1, 64, 0,},
	{AX620U_NPLL_533M, "npll_533m", "npll", 1, 3, 0,},
	{AX620U_VPLL_594M, "vpll_594m", "vpll", 1, 2, 0,},
	{AX620U_VPLL_DIV_4, "vpll_div_4", "vpll", 1, 4, 0,},
	{AX620U_VPLL_148P5M, "vpll_148p5m", "vpll", 1, 8, 0,},
	{AX620U_VPLL_74P25M, "vpll_74p25m", "vpll", 1, 16, 0,},
	{AX620U_VPLL_37P125M, "vpll_37p125m", "vpll", 1, 32, 0,},
	{AX620U_VPLL_27M, "vpll_27m", "vpll", 1, 44, 0,},
	{AX620U_EPLL_1000M, "epll_1000m", "epll", 1, 1, 0,},
	{AX620U_EPLL_500M, "epll_500m", "epll", 1, 2, 0,},
	{AX620U_EPLL_250M, "epll_250m", "epll", 1, 4, 0,},
	{AX620U_EPLL_125M, "epll_125m", "epll", 1, 8, 0,},
	{AX620U_EPLL_DIV_16, "epll_div_16", "epll", 1, 16, 0,},
	{AX620U_EPLL_31P25M, "epll_31p25m", "epll", 1, 32, 0,},
	{AX620U_EPLL_200M, "epll_200m", "epll", 1, 5, 0,},
	{AX620U_EPLL_100M, "epll_100m", "epll", 1, 10, 0,},
	{AX620U_EPLL_20M, "epll_20m", "epll", 1, 50, 0,},
	{AX620U_EPLL_10M, "epll_10m", "epll", 1, 100, 0,},
	{AX620U_EPLL_50M, "epll_50m", "epll", 1, 20, 0,},
	{AX620U_EPLL_25M, "epll_25m", "epll", 1, 40, 0,},
	{AX620U_EPLL_333M, "epll_333m", "epll", 1, 3, 0,},
	{AX620U_DPLL_1066M, "dpll_1066m", "dpll", 1, 1, 0,},
	{AX620U_DPLL_533M, "dpll_533m", "dpll", 1, 2, 0,},
	{AX620U_DPLL_DIV_4, "dpll_div_4", "dpll", 1, 4, 0,},
	{AX620U_DPLL_DIV_8, "dpll_div_8", "dpll", 1, 8, 0,},
	{AX620U_DPLL_DIV_16, "dpll_div_16", "dpll", 1, 16, 0,},
	{AX620U_DPLL_33M, "dpll_33m", "dpll", 1, 32, 0,},
};

static struct axera_mux_clock ax620u_mux_clks_common[] __initdata = {
	{AX620U_PCLK_TOP_SEL, "pclk_top_sel", pclk_top_sel_p, ARRAY_SIZE(pclk_top_sel_p), CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x060, 12, 2, 0, NULL,},
	{AX620U_CLK_CSI_TX_SEL, "clk_csi_tx_sel", clk_csi_tx_sel_p, ARRAY_SIZE(clk_csi_tx_sel_p), CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x060, 9, 3, 0, NULL,},
	{AX620U_CLK_COM_GLB_SEL, "clk_com_glb_sel", clk_com_glb_sel_p, ARRAY_SIZE(clk_com_glb_sel_p), CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x060, 8, 1, 0, NULL,},
	{AX620U_ACLK_VPU_TOP_SEL, "aclk_vpu_top_sel", aclk_vpu_top_sel_p, ARRAY_SIZE(aclk_vpu_top_sel_p), CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x060, 6, 2, 0, NULL,},
	{AX620U_ACLK_NN_TOP_SEL, "aclk_nn_top_sel", aclk_nn_top_sel_p, ARRAY_SIZE(aclk_nn_top_sel_p), CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x060, 4, 2, 0, NULL,},
	{AX620U_ACLK_ISP_TOP_SEL, "aclk_isp_top_sel", aclk_isp_top_sel_p, ARRAY_SIZE(aclk_isp_top_sel_p), CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x060, 2, 2, 0, NULL,},
	{AX620U_ACLK_CPU_TOP_SEL, "aclk_cpu_top_sel", aclk_cpu_top_sel_p, ARRAY_SIZE(aclk_cpu_top_sel_p), CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x060, 0, 2, 0, NULL,},
	{AX620U_DEBUG_SEL, "debug_sel", debug_sel_p, ARRAY_SIZE(debug_sel_p), CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x30c, 0, 3, 0, NULL,},
};

static struct axera_gate_clock ax620u_gate_clks_common[] __initdata = {
	{AX620U_CLK_CSI_TX, "clk_csi_tx", "clk_csi_tx_sel", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x06c, 2, 0, 0x070, 2, 0x074, 2,},
	{AX620U_CLK_COM_DIG, "clk_com_dig", "rtc_32hz", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x06c, 1, 0, 0x070, 1, 0x074, 1,},
	{AX620U_CLK_COM_ANA, "clk_com_ana", "xtal_24m", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED | CLK_IS_CRITICAL, 0x06c, 0, 0, 0x070, 0, 0x074, 0,},
	{AX620U_PCLK_WDT_EB_COMMON, "pclk_wdt_eb_common", "clk_com_glb_sel", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x078, 12, 0, 0x07c, 12, 0x080, 12,},
	{AX620U_PCLK_TMR32, "pclk_tmr32", "clk_com_glb_sel", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x078, 11, 0, 0x07c, 11, 0x080, 11,},
	{AX620U_PCLK_THM, "pclk_thm", "clk_com_glb_sel", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x078, 10, 0, 0x07c, 10, 0x080, 10,},
	{AX620U_PCLK_EFC, "pclk_efc", "clk_com_ana", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x078, 9, 0, 0x07c, 9, 0x080, 9,},
	{AX620U_PCLK_DBC_GPIO, "pclk_dbc_gpio", "clk_com_glb_sel", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x078, 8, 0, 0x07c, 8, 0x080, 8,},
	{AX620U_CLK_WDT_EB_COMMON, "clk_wdt_eb_common", "clk_com_dig", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x078, 7, 0, 0x07c, 7, 0x080, 7,},
	{AX620U_CLK_TMR32, "clk_tmr32", "clk_com_dig", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x078, 6, 0, 0x07c, 6, 0x080, 6,},
	{AX620U_CLK_THM, "clk_thm", "clk_com_ana", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x078, 5, 0, 0x07c, 5, 0x080, 5,},
	{AX620U_CLK_PMU, "clk_pmu", "clk_com_glb_sel", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x078, 4, 0, 0x07c, 4, 0x080, 4,},
	{AX620U_CLK_PLLC, "clk_pllc", "clk_com_glb_sel", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x078, 3, 0, 0x07c, 3, 0x080, 3,},
	{AX620U_CLK_PINMUX, "clk_pinmux", "clk_com_ana", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED | CLK_IS_CRITICAL, 0x078, 2, 0, 0x07c, 2, 0x080, 2,},
	{AX620U_CLK_IRAM, "clk_iram", "pclk_top_sel", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x078, 1, 0, 0x07c, 1, 0x080, 1,},
	{AX620U_CLK_DBC_GPIO, "clk_dbc_gpio", "debug_sel", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x078, 0, 0, 0x07c, 0, 0x080, 0,},
};

static void __init ax620u_clk_common_clk_init(struct device_node *np)
{
	struct axera_clock_data *clk_data;

	clk_data = axera_clk_init(np, AX620U_COMMON_NR_CLKS);
	if (!clk_data)
		return;

	axera_clk_register_fixed_rate(ax620u_fixed_rate_clks_common, ARRAY_SIZE(ax620u_fixed_rate_clks_common), clk_data);
	axera_clk_register_fixed_factor(ax620u_fixed_factor_clks_common, ARRAY_SIZE(ax620u_fixed_factor_clks_common), clk_data);
	axera_clk_register_mux(ax620u_mux_clks_common, ARRAY_SIZE(ax620u_mux_clks_common), clk_data);
	axera_clk_register_gate(ax620u_gate_clks_common, ARRAY_SIZE(ax620u_gate_clks_common), clk_data);
}
CLK_OF_DECLARE(ax620u_clk_common, "axera,ax620x-common-clk", ax620u_clk_common_clk_init);

/* dispc_clk */
static const char *clk_dphy_tx_esc2_sel_p[] __initdata = { "epll_10m", "epll_20m" };
static const char *clk_dphy_tx_esc1_sel_p[] __initdata = { "epll_10m", "epll_20m" };
static const char *clk_dphy_tx_esc0_sel_p[] __initdata = { "epll_10m", "epll_20m" };
static const char *clk_csi_tx_pix_sel_p[] __initdata = { "cpll_208m", "epll_333m", "npll_400m", "epll_500m", "npll_533m" };

static struct axera_mux_clock ax620u_mux_clks_dispc[] __initdata = {
	{AX620U_CLK_DPHY_TX_ESC2_SEL, "clk_dphy_tx_esc2_sel", clk_dphy_tx_esc2_sel_p, ARRAY_SIZE(clk_dphy_tx_esc2_sel_p), CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x000, 5, 1, 0, NULL,},
	{AX620U_CLK_DPHY_TX_ESC1_SEL, "clk_dphy_tx_esc1_sel", clk_dphy_tx_esc1_sel_p, ARRAY_SIZE(clk_dphy_tx_esc1_sel_p), CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x000, 4, 1, 0, NULL,},
	{AX620U_CLK_DPHY_TX_ESC0_SEL, "clk_dphy_tx_esc0_sel", clk_dphy_tx_esc0_sel_p, ARRAY_SIZE(clk_dphy_tx_esc0_sel_p), CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x000, 3, 1, 0, NULL,},
	{AX620U_CLK_CSI_TX_PIX_SEL, "clk_csi_tx_pix_sel", clk_csi_tx_pix_sel_p, ARRAY_SIZE(clk_csi_tx_pix_sel_p), CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x000, 0, 3, 0, NULL,},
};

static struct axera_gate_clock ax620u_gate_clks_dispc[] __initdata = {
	{AX620U_CLK_DPHY_TX_REF, "clk_dphy_tx_ref", "cpll_12m", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x004, 4, 0, 0x3b4, 4, 0x3b8, 4,},
	{AX620U_CLK_DPHY_TX_ESC2, "clk_dphy_tx_esc2", "clk_dphy_tx_esc2_sel", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x004, 3, 0, 0x3b4, 3, 0x3b8, 3,},
	{AX620U_CLK_DPHY_TX_ESC1, "clk_dphy_tx_esc1", "clk_dphy_tx_esc1_sel", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x004, 2, 0, 0x3b4, 2, 0x3b8, 2,},
	{AX620U_CLK_DPHY_TX_ESC0, "clk_dphy_tx_esc0", "clk_dphy_tx_esc0_sel", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x004, 1, 0, 0x3b4, 1, 0x3b8, 1,},
	{AX620U_CLK_CSI_TX_PIX, "clk_csi_tx_pix", "clk_csi_tx_pix_sel", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x004, 0, 0, 0x3b4, 0, 0x3b8, 0,},
	{AX620U_PCLK_DSI1, "pclk_dsi1", "pclk_dispc_divn", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x008, 25, 0, 0x3bc, 25, 0x3c0, 25,},
	{AX620U_PCLK_DSI0, "pclk_dsi0", "pclk_dispc_divn", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x008, 24, 0, 0x3bc, 24, 0x3c0, 24,},
	{AX620U_PCLK_CSI2, "pclk_csi2", "pclk_dispc_divn", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x008, 23, 0, 0x3bc, 23, 0x3c0, 23,},
	{AX620U_PCLK_CSI1, "pclk_csi1", "pclk_dispc_divn", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x008, 22, 0, 0x3bc, 22, 0x3c0, 22,},
	{AX620U_PCLK_CSI0, "pclk_csi0", "pclk_dispc_divn", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x008, 21, 0, 0x3bc, 21, 0x3c0, 21,},
	{AX620U_CLK_DSI_TX_ESC1, "clk_dsi_tx_esc1", "clk_dphy_tx_esc1_sel", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x008, 20, 0, 0x3bc, 20, 0x3c0, 20,},
	{AX620U_CLK_DSI_TX_ESC0, "clk_dsi_tx_esc0", "clk_dphy_tx_esc0_sel", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x008, 19, 0, 0x3bc, 19, 0x3c0, 19,},
	{AX620U_CLK_DSI_SYS1, "clk_dsi_sys1", "pclk_dispc_divn", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x008, 18, 0, 0x3bc, 18, 0x3c0, 18,},
	{AX620U_CLK_DSI_SYS0, "clk_dsi_sys0", "pclk_dispc_divn", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x008, 17, 0, 0x3bc, 17, 0x3c0, 17,},
	{AX620U_CLK_DPHY_REF2, "clk_dphy_ref2", "cpll_12m", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x008, 16, 0, 0x3bc, 16, 0x3c0, 16,},
	{AX620U_CLK_DPHY_REF1, "clk_dphy_ref1", "cpll_12m", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x008, 15, 0, 0x3bc, 15, 0x3c0, 15,},
	{AX620U_CLK_DPHY_REF0, "clk_dphy_ref0", "cpll_12m", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x008, 14, 0, 0x3bc, 14, 0x3c0, 14,},
	{AX620U_CLK_DPHY2DSI_HS1, "clk_dphy2dsi_hs1", "epll_333m", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x008, 13, 0, 0x3bc, 13, 0x3c0, 13,},
	{AX620U_CLK_DPHY2DSI_HS0, "clk_dphy2dsi_hs0", "epll_333m", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x008, 12, 0, 0x3bc, 12, 0x3c0, 12,},
	{AX620U_CLK_DPHY2CSI_HS2, "clk_dphy2csi_hs2", "epll_333m", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x008, 11, 0, 0x3bc, 11, 0x3c0, 11,},
	{AX620U_CLK_DPHY2CSI_HS1, "clk_dphy2csi_hs1", "epll_333m", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x008, 10, 0, 0x3bc, 10, 0x3c0, 10,},
	{AX620U_CLK_DPHY2CSI_HS0, "clk_dphy2csi_hs0", "epll_333m", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x008, 9, 0, 0x3bc, 9, 0x3c0, 9,},
	{AX620U_CLK_CSI_TX_PIX2, "clk_csi_tx_pix2", "clk_csi_tx_pix_sel", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x008, 8, 0, 0x3bc, 8, 0x3c0, 8,},
	{AX620U_CLK_CSI_TX_PIX1, "clk_csi_tx_pix1", "clk_csi_tx_pix_sel", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x008, 7, 0, 0x3bc, 7, 0x3c0, 7,},
	{AX620U_CLK_CSI_TX_PIX0, "clk_csi_tx_pix0", "clk_csi_tx_pix_sel", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x008, 6, 0, 0x3bc, 6, 0x3c0, 6,},
	{AX620U_CLK_CSI_TX_ESC2, "clk_csi_tx_esc2", "clk_dphy_tx_esc2_sel", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x008, 5, 0, 0x3bc, 5, 0x3c0, 5,},
	{AX620U_CLK_CSI_TX_ESC1, "clk_csi_tx_esc1", "clk_dphy_tx_esc1_sel", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x008, 4, 0, 0x3bc, 4, 0x3c0, 4,},
	{AX620U_CLK_CSI_TX_ESC0, "clk_csi_tx_esc0", "clk_dphy_tx_esc0_sel", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x008, 3, 0, 0x3bc, 3, 0x3c0, 3,},
	{AX620U_CLK_CSI_TX2, "clk_csi_tx2", "clk_csi_tx", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x008, 2, 0, 0x3bc, 2, 0x3c0, 2,},
	{AX620U_CLK_CSI_TX1, "clk_csi_tx1", "clk_csi_tx", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x008, 1, 0, 0x3bc, 1, 0x3c0, 1,},
	{AX620U_CLK_CSI_TX0, "clk_csi_tx0", "clk_csi_tx", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x008, 0, 0, 0x3bc, 0, 0x3c0, 0,},
};

static void __init ax620u_clk_dispc_clk_init(struct device_node *np)
{
	struct axera_clock_data *clk_data;

	clk_data = axera_clk_init(np, AX620U_DISPC_NR_CLKS);
	if (!clk_data)
		return;

	axera_clk_register_mux(ax620u_mux_clks_dispc, ARRAY_SIZE(ax620u_mux_clks_dispc), clk_data);
	axera_clk_register_gate(ax620u_gate_clks_dispc, ARRAY_SIZE(ax620u_gate_clks_dispc), clk_data);
}
CLK_OF_DECLARE(ax620u_clk_dispc, "axera,ax620x-dispc-clk", ax620u_clk_dispc_clk_init);

/* vpu_clk */
static const char *clk_vpu_glb_sel_p[] __initdata = { "epll_250m", "cpll_312m", "epll_500m", "cpll_624m" };

static struct axera_mux_clock ax620u_mux_clks_vpu[] __initdata = {
	{AX620U_CLK_VPU_GLB_SEL, "clk_vpu_glb_sel", clk_vpu_glb_sel_p, ARRAY_SIZE(clk_vpu_glb_sel_p), CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x000, 0, 2, 0, NULL,},
};

static struct axera_gate_clock ax620u_gate_clks_vpu[] __initdata = {
	{AX620U_CLK_VENC, "clk_venc", "clk_vpu_glb_sel", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x004, 2, 0, 0x088, 2, 0x08c, 2,},
	{AX620U_CLK_VDEC, "clk_vdec", "clk_vpu_glb_sel", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x004, 1, 0, 0x088, 1, 0x08c, 1,},
	{AX620U_CLK_JENC, "clk_jenc", "clk_vpu_glb_sel", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x004, 0, 0, 0x088, 0, 0x08c, 0,},
};

static void __init ax620u_clk_vpu_clk_init(struct device_node *np)
{
	struct axera_clock_data *clk_data;

	clk_data = axera_clk_init(np, AX620U_VPU_NR_CLKS);
	if (!clk_data)
		return;

	axera_clk_register_mux(ax620u_mux_clks_vpu, ARRAY_SIZE(ax620u_mux_clks_vpu), clk_data);
	axera_clk_register_gate(ax620u_gate_clks_vpu, ARRAY_SIZE(ax620u_gate_clks_vpu), clk_data);
}
CLK_OF_DECLARE(ax620u_clk_vpu, "axera,ax620x-vpu-clk", ax620u_clk_vpu_clk_init);

/* cpu_clk */
static const char *clk_dma_fab_sel_p[] __initdata = { "xtal_24m", "cpll_156m", "epll_250m", "epll_500m" };
static const char *clk_cpu_sel_p[] __initdata = { "xtal_24m", "npll_800m", "epll_1000m", "vpll", "cpll_1248m" };

static struct axera_mux_clock ax620u_mux_clks_cpu[] __initdata = {
	{AX620U_CLK_DMA_FAB_SEL, "clk_dma_fab_sel", clk_dma_fab_sel_p, ARRAY_SIZE(clk_dma_fab_sel_p), CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x000, 4, 2, 0, NULL,},
	{AX620U_CLK_CPU_SEL, "clk_cpu_sel", clk_cpu_sel_p, ARRAY_SIZE(clk_cpu_sel_p), CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x000, 1, 3, 0, NULL,},
};

static struct axera_gate_clock ax620u_gate_clks_cpu[] __initdata = {
	{AX620U_CLK_DMA, "clk_dma", "clk_dma_fab_sel", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x008, 0, 0, 0x1008, 0, 0x2008, 0,},
};

static void __init ax620u_clk_cpu_clk_init(struct device_node *np)
{
	struct axera_clock_data *clk_data;

	clk_data = axera_clk_init(np, AX620U_CPU_NR_CLKS);
	if (!clk_data)
		return;

	axera_clk_register_mux(ax620u_mux_clks_cpu, ARRAY_SIZE(ax620u_mux_clks_cpu), clk_data);
	axera_clk_register_gate(ax620u_gate_clks_cpu, ARRAY_SIZE(ax620u_gate_clks_cpu), clk_data);
}
CLK_OF_DECLARE(ax620u_clk_cpu, "axera,ax620x-cpu-clk", ax620u_clk_cpu_clk_init);

/* ap_clk */
static const char *clk_wdt_sel_p[] __initdata = { "xtal_32k", "xtal_24m" };
static const char *clk_stb_predft_sel_p[] __initdata = { "xtal_32k", "cpll_12m" };
static const char *clk_sdio_s_pad_sel_p[] __initdata = { "clk_pad_sdio_s", "epll_200m" };
static const char *clk_sdio_m_card_sel_p[] __initdata = { "epll_5m", "xtal_24m", "cpll_156m", "epll_200m" };
static const char *clk_otg_fast_sel_p[] __initdata = { "xtal_24m", "epll_50m", "epll_250m" };
static const char *clk_ephy_ref_sel_p[] __initdata = { "epll_25m", "epll_50m", "epll_125m" };
static const char *clk_emac_rgmii_tx_sel_p[] __initdata = { "epll_10m", "epll_100m", "epll_500m" };
static const char *clk_emac_rgmii_rx_pad_sel_p[] __initdata = { "clk_pad_emac_rx", "epll_125m" };
static const char *clk_ce_bus_sel_p[] __initdata = { "xtal_24m", "epll_500m", "cpll_624m" };
static const char *clk_ap_timer_sel_p[] __initdata = { "xtal_32k", "xtal_24m" };
static const char *clk_ap_spi_s_pad_sel_p[] __initdata = { "clk_pad_spi_s", "epll_62p5m" };
static const char *clk_ap_spi1_sel_p[] __initdata = { "xtal_24m", "cpll_156m", "epll_200m", "cpll_312m" };
static const char *clk_ap_spi0_sel_p[] __initdata = { "xtal_24m", "cpll_156m", "epll_200m", "cpll_312m" };
static const char *clk_ap_sil_sel_p[] __initdata = { "xtal_24m", "epll_50m", "cpll_156m", "epll_200m" };
static const char *clk_ap_sih_sel_p[] __initdata = { "xtal_24m", "epll_50m", "cpll_156m", "epll_200m" };
static const char *clk_ap_rtc_sel_p[] __initdata = { "xtal_32k", "xtal_24m" };
static const char *clk_ap_glb_sel_p[] __initdata = { "xtal_24m", "epll_100m", "epll_200m", "cpll_312m" };
static const char *clk_i2s_ref2_sel_p[] __initdata = { "cpll_12m", "hpll_16p384m", "hpll_24p576m", "epll_25m", "hpll_40p96m", "hpll_49p152m" };
static const char *clk_i2s_ref1_sel_p[] __initdata = { "cpll_12m", "hpll_16p384m", "hpll_24p576m", "epll_25m", "hpll_40p96m", "hpll_49p152m" };
static const char *clk_i2s_ref0_sel_p[] __initdata = { "cpll_12m", "hpll_16p384m", "hpll_24p576m", "epll_25m", "hpll_40p96m", "hpll_49p152m" };
static const char *sclk_i2s_tdm_sel_p[] __initdata = { "hpll_16p384m", "hpll_24p576m", "hpll_40p96m", "hpll_49p152m" };
static const char *sclk_i2s_sel_p[] __initdata = { "hpll_16p384m", "hpll_24p576m", "hpll_40p96m", "hpll_49p152m" };

static struct axera_mux_clock ax620u_mux_clks_ap[] __initdata = {
	{AX620U_CLK_WDT_SEL, "clk_wdt_sel", clk_wdt_sel_p, ARRAY_SIZE(clk_wdt_sel_p), CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x000, 30, 1, 0, NULL,},
	{AX620U_CLK_STB_PREDFT_SEL, "clk_stb_predft_sel", clk_stb_predft_sel_p, ARRAY_SIZE(clk_stb_predft_sel_p), CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x000, 28, 1, 0, NULL,},
	{AX620U_CLK_SDIO_S_PAD_SEL, "clk_sdio_s_pad_sel", clk_sdio_s_pad_sel_p, ARRAY_SIZE(clk_sdio_s_pad_sel_p), CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x000, 27, 1, 0, NULL,},
	{AX620U_CLK_SDIO_M_CARD_SEL, "clk_sdio_m_card_sel", clk_sdio_m_card_sel_p, ARRAY_SIZE(clk_sdio_m_card_sel_p), CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x000, 25, 2, 0, NULL,},
	{AX620U_CLK_OTG_FAST_SEL, "clk_otg_fast_sel", clk_otg_fast_sel_p, ARRAY_SIZE(clk_otg_fast_sel_p), CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x000, 23, 2, 0, NULL,},
	{AX620U_CLK_EPHY_REF_SEL, "clk_ephy_ref_sel", clk_ephy_ref_sel_p, ARRAY_SIZE(clk_ephy_ref_sel_p), CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x000, 18, 2, 0, NULL,},
	{AX620U_CLK_EMAC_RGMII_TX_SEL, "clk_emac_rgmii_tx_sel", clk_emac_rgmii_tx_sel_p, ARRAY_SIZE(clk_emac_rgmii_tx_sel_p), CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x000, 16, 2, 0, NULL,},
	{AX620U_CLK_EMAC_RGMII_RX_PAD_SEL, "clk_emac_rgmii_rx_pad_sel", clk_emac_rgmii_rx_pad_sel_p, ARRAY_SIZE(clk_emac_rgmii_rx_pad_sel_p), CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x000, 15, 1, 0, NULL,},
	{AX620U_CLK_CE_BUS_SEL, "clk_ce_bus_sel", clk_ce_bus_sel_p, ARRAY_SIZE(clk_ce_bus_sel_p), CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x000, 13, 2, 0, NULL,},
	{AX620U_CLK_AP_TIMER_SEL, "clk_ap_timer_sel", clk_ap_timer_sel_p, ARRAY_SIZE(clk_ap_timer_sel_p), CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x000, 12, 1, 0, NULL,},
	{AX620U_CLK_AP_SPI_S_PAD_SEL, "clk_ap_spi_s_pad_sel", clk_ap_spi_s_pad_sel_p, ARRAY_SIZE(clk_ap_spi_s_pad_sel_p), CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x000, 11, 1, 0, NULL,},
	{AX620U_CLK_AP_SPI1_SEL, "clk_ap_spi1_sel", clk_ap_spi1_sel_p, ARRAY_SIZE(clk_ap_spi1_sel_p), CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x000, 9, 2, 0, NULL,},
	{AX620U_CLK_AP_SPI0_SEL, "clk_ap_spi0_sel", clk_ap_spi0_sel_p, ARRAY_SIZE(clk_ap_spi0_sel_p), CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x000, 7, 2, 0, NULL,},
	{AX620U_CLK_AP_SIL_SEL, "clk_ap_sil_sel", clk_ap_sil_sel_p, ARRAY_SIZE(clk_ap_sil_sel_p), CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x000, 5, 2, 0, NULL,},
	{AX620U_CLK_AP_SIH_SEL, "clk_ap_sih_sel", clk_ap_sih_sel_p, ARRAY_SIZE(clk_ap_sih_sel_p), CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x000, 3, 2, 0, NULL,},
	{AX620U_CLK_AP_RTC_SEL, "clk_ap_rtc_sel", clk_ap_rtc_sel_p, ARRAY_SIZE(clk_ap_rtc_sel_p), CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x000, 2, 1, 0, NULL,},
	{AX620U_CLK_AP_GLB_SEL, "clk_ap_glb_sel", clk_ap_glb_sel_p, ARRAY_SIZE(clk_ap_glb_sel_p), CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x000, 0, 2, 0, NULL,},
	{AX620U_CLK_I2S_REF2_SEL, "clk_i2s_ref2_sel", clk_i2s_ref2_sel_p, ARRAY_SIZE(clk_i2s_ref2_sel_p), CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x004, 13, 3, 0, NULL,},
	{AX620U_CLK_I2S_REF1_SEL, "clk_i2s_ref1_sel", clk_i2s_ref1_sel_p, ARRAY_SIZE(clk_i2s_ref1_sel_p), CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x004, 10, 3, 0, NULL,},
	{AX620U_CLK_I2S_REF0_SEL, "clk_i2s_ref0_sel", clk_i2s_ref0_sel_p, ARRAY_SIZE(clk_i2s_ref0_sel_p), CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x004, 7, 3, 0, NULL,},
	{AX620U_SCLK_I2S_TDM_SEL, "sclk_i2s_tdm_sel", sclk_i2s_tdm_sel_p, ARRAY_SIZE(sclk_i2s_tdm_sel_p), CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x004, 2, 2, 0, NULL,},
	{AX620U_SCLK_I2S_SEL, "sclk_i2s_sel", sclk_i2s_sel_p, ARRAY_SIZE(sclk_i2s_sel_p), CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x004, 0, 2, 0, NULL,},
};

static struct axera_gate_clock ax620u_gate_clks_ap[] __initdata = {
	{AX620U_SCLK_I2S_TDM, "sclk_i2s_tdm", "sclk_i2s_tdm_divn", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x008, 21, 0, 0x1fc, 21, 0x200, 21,},
	{AX620U_SCLK_I2S, "sclk_i2s", "sclk_i2s_divn", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x008, 20, 0, 0x1fc, 20, 0x200, 20,},
	{AX620U_CLK_WDT, "clk_wdt", "clk_wdt_sel", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x008, 19, 0, 0x1fc, 19, 0x200, 19,},
	{AX620U_CLK_USB_REF, "clk_usb_ref", "epll_25m", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x008, 18, 0, 0x1fc, 18, 0x200, 18,},
	{AX620U_CLK_USB2PHY, "clk_usb2phy", "cpll_12m", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x008, 17, 0, 0x1fc, 17, 0x200, 17,},
	{AX620U_CLK_STB_PREDFT, "clk_stb_predft", "clk_stb_predft_sel", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x008, 16, 0, 0x1fc, 16, 0x200, 16,},
	{AX620U_CLK_SDIO_M_CARD, "clk_sdio_m_card", "clk_sdio_m_card_divn", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x008, 15, 0, 0x1fc, 15, 0x200, 15,},
	{AX620U_CLK_OTG_FAST, "clk_otg_fast", "clk_otg_fast_divn", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x008, 14, 0, 0x1fc, 14, 0x200, 14,},
	{AX620U_CLK_LPM_PREDFT, "clk_lpm_predft", "cpll_12m", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x008, 13, 0, 0x1fc, 13, 0x200, 13,},
	{AX620U_CLK_EPHY_REF, "clk_ephy_ref", "clk_ephy_ref_sel", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x008, 11, 0, 0x1fc, 11, 0x200, 11,},
	{AX620U_CLK_EMAC_RMII_TX, "clk_emac_rmii_tx", "epll_50m", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x008, 10, 0, 0x1fc, 10, 0x200, 10,},
	{AX620U_CLK_EMAC_RMII_RX_PAD_DIV, "clk_emac_rmii_rx_pad_div", "clk_emac_rmii_rx_pad_divn", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x008, 9, 0, 0x1fc, 9, 0x200, 9,},
	{AX620U_CLK_EMAC_RGMII_TX, "clk_emac_rgmii_tx", "clk_emac_rgmii_tx_sel", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x008, 8, 0, 0x1fc, 8, 0x200, 8,},
	{AX620U_CLK_CE_CNT, "clk_ce_cnt", "clk_ce_bus_sel", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x008, 7, 0, 0x1fc, 7, 0x200, 7,},
	{AX620U_CLK_APP_125_PREDFT, "clk_app_125_predft", "need_modify" /* as in the stock table: no such clock, an orphan */, CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x008, 6, 0, 0x1fc, 6, 0x200, 6,},
	{AX620U_CLK_AP_TIMER, "clk_ap_timer", "clk_ap_timer_sel", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x008, 5, 0, 0x1fc, 5, 0x200, 5,},
	{AX620U_CLK_AP_SPI1, "clk_ap_spi1", "clk_ap_spi1_sel", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x008, 4, 0, 0x1fc, 4, 0x200, 4,},
	{AX620U_CLK_AP_SPI0, "clk_ap_spi0", "clk_ap_spi0_sel", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x008, 3, 0, 0x1fc, 3, 0x200, 3,},
	{AX620U_CLK_AP_SIL, "clk_ap_sil", "clk_ap_sil_sel", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x008, 2, 0, 0x1fc, 2, 0x200, 2,},
	{AX620U_CLK_AP_SIH, "clk_ap_sih", "clk_ap_sih_sel", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x008, 1, 0, 0x1fc, 1, 0x200, 1,},
	{AX620U_CLK_AP_RTC, "clk_ap_rtc", "clk_ap_rtc_sel", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x008, 0, 0, 0x1fc, 0, 0x200, 0,},
	{AX620U_CLK_PWM23, "clk_pwm23", "clk_ap_rtc_sel", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x00c, 31, 0, 0x204, 31, 0x208, 31,},
	{AX620U_CLK_PWM22, "clk_pwm22", "clk_ap_rtc_sel", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x00c, 30, 0, 0x204, 30, 0x208, 30,},
	{AX620U_CLK_PWM21, "clk_pwm21", "clk_ap_timer_sel", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x00c, 29, 0, 0x204, 29, 0x208, 29,},
	{AX620U_CLK_PWM20, "clk_pwm20", "clk_ap_timer_sel", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x00c, 28, 0, 0x204, 28, 0x208, 28,},
	{AX620U_CLK_PWM13, "clk_pwm13", "clk_ap_rtc_sel", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x00c, 27, 0, 0x204, 27, 0x208, 27,},
	{AX620U_CLK_PWM12, "clk_pwm12", "clk_ap_rtc_sel", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x00c, 26, 0, 0x204, 26, 0x208, 26,},
	{AX620U_CLK_PWM11, "clk_pwm11", "clk_ap_timer_sel", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x00c, 25, 0, 0x204, 25, 0x208, 25,},
	{AX620U_CLK_PWM10, "clk_pwm10", "clk_ap_timer_sel", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x00c, 24, 0, 0x204, 24, 0x208, 24,},
	{AX620U_CLK_PWM03, "clk_pwm03", "clk_ap_rtc_sel", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x00c, 23, 0, 0x204, 23, 0x208, 23,},
	{AX620U_CLK_PWM02, "clk_pwm02", "clk_ap_rtc_sel", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x00c, 22, 0, 0x204, 22, 0x208, 22,},
	{AX620U_CLK_PWM01, "clk_pwm01", "clk_ap_timer_sel", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x00c, 21, 0, 0x204, 21, 0x208, 21,},
	{AX620U_CLK_PWM00, "clk_pwm00", "clk_ap_timer_sel", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x00c, 20, 0, 0x204, 20, 0x208, 20,},
	{AX620U_CLK_I2S_REF2, "clk_i2s_ref2", "clk_i2s_ref2_divn", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x00c, 19, 0, 0x204, 19, 0x208, 19,},
	{AX620U_CLK_I2S_REF1, "clk_i2s_ref1", "clk_i2s_ref1_divn", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x00c, 18, 0, 0x204, 18, 0x208, 18,},
	{AX620U_CLK_I2S_REF0, "clk_i2s_ref0", "clk_i2s_ref0_divn", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x00c, 17, 0, 0x204, 17, 0x208, 17,},
	{AX620U_CLK_I2C_HS3, "clk_i2c_hs3", "clk_ap_sih_sel", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x00c, 16, 0, 0x204, 16, 0x208, 16,},
	{AX620U_CLK_I2C_HS2, "clk_i2c_hs2", "clk_ap_sih_sel", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x00c, 15, 0, 0x204, 15, 0x208, 15,},
	{AX620U_CLK_I2C_HS1, "clk_i2c_hs1", "clk_ap_sih_sel", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x00c, 14, 0, 0x204, 14, 0x208, 14,},
	{AX620U_CLK_I2C_HS0, "clk_i2c_hs0", "clk_ap_sih_sel", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x00c, 13, 0, 0x204, 13, 0x208, 13,},
	{AX620U_CLK_I2C5, "clk_i2c5", "clk_ap_sil_sel", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x00c, 12, 0, 0x204, 12, 0x208, 12,},
	{AX620U_CLK_I2C4, "clk_i2c4", "clk_ap_sil_sel", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x00c, 11, 0, 0x204, 11, 0x208, 11,},
	{AX620U_CLK_I2C3, "clk_i2c3", "clk_ap_sil_sel", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x00c, 10, 0, 0x204, 10, 0x208, 10,},
	{AX620U_CLK_I2C2, "clk_i2c2", "clk_ap_sil_sel", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x00c, 9, 0, 0x204, 9, 0x208, 9,},
	{AX620U_CLK_I2C1, "clk_i2c1", "clk_ap_sil_sel", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x00c, 8, 0, 0x204, 8, 0x208, 8,},
	{AX620U_CLK_I2C0, "clk_i2c0", "clk_ap_sil_sel", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x00c, 7, 0, 0x204, 7, 0x208, 7,},
	{AX620U_CLK_GPIO2, "clk_gpio2", "clk_ap_rtc_sel", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x00c, 6, 0, 0x204, 6, 0x208, 6,},
	{AX620U_CLK_GPIO0, "clk_gpio0", "clk_ap_rtc_sel", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x00c, 5, 0, 0x204, 5, 0x208, 5,},
	{AX620U_CLK_CE, "clk_ce", "clk_ce_bus_sel", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x00c, 4, 0, 0x204, 4, 0x208, 4,},
	{AX620U_ACLK_USB, "aclk_usb", "clk_ap_glb_sel", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x00c, 3, 0, 0x204, 3, 0x208, 3,},
	{AX620U_ACLK_SDIO_M1, "aclk_sdio_m1", "clk_ap_glb_sel", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x00c, 2, 0, 0x204, 2, 0x208, 2,},
	{AX620U_ACLK_SDIO_M0, "aclk_sdio_m0", "clk_ap_glb_sel", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x00c, 1, 0, 0x204, 1, 0x208, 1,},
	{AX620U_ACLK_EMAC, "aclk_emac", "clk_ap_glb_sel", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x00c, 0, 0, 0x204, 0, 0x208, 0,},
	{AX620U_PCLK_I2C_HS2, "pclk_i2c_hs2", "clk_ap_glb_sel", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x010, 31, 0, 0x20c, 31, 0x210, 31,},
	{AX620U_PCLK_I2C_HS1, "pclk_i2c_hs1", "clk_ap_glb_sel", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x010, 30, 0, 0x20c, 30, 0x210, 30,},
	{AX620U_PCLK_I2C_HS0, "pclk_i2c_hs0", "clk_ap_glb_sel", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x010, 29, 0, 0x20c, 29, 0x210, 29,},
	{AX620U_PCLK_I2C5, "pclk_i2c5", "clk_ap_glb_sel", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x010, 28, 0, 0x20c, 28, 0x210, 28,},
	{AX620U_PCLK_I2C4, "pclk_i2c4", "clk_ap_glb_sel", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x010, 27, 0, 0x20c, 27, 0x210, 27,},
	{AX620U_PCLK_I2C3, "pclk_i2c3", "clk_ap_glb_sel", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x010, 26, 0, 0x20c, 26, 0x210, 26,},
	{AX620U_PCLK_I2C2, "pclk_i2c2", "clk_ap_glb_sel", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x010, 25, 0, 0x20c, 25, 0x210, 25,},
	{AX620U_PCLK_I2C1, "pclk_i2c1", "clk_ap_glb_sel", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x010, 24, 0, 0x20c, 24, 0x210, 24,},
	{AX620U_PCLK_I2C0, "pclk_i2c0", "clk_ap_glb_sel", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x010, 23, 0, 0x20c, 23, 0x210, 23,},
	{AX620U_PCLK_GPIO2, "pclk_gpio2", "clk_ap_glb_sel", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x010, 22, 0, 0x20c, 22, 0x210, 22,},
	{AX620U_PCLK_GPIO0, "pclk_gpio0", "clk_ap_glb_sel", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x010, 21, 0, 0x20c, 21, 0x210, 21,},
	{AX620U_PCLK_EMAC, "pclk_emac", "clk_ap_glb_sel", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x010, 20, 0, 0x20c, 20, 0x210, 20,},
	{AX620U_PCLK_CE, "pclk_ce", "clk_ap_glb_sel", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x010, 19, 0, 0x20c, 19, 0x210, 19,},
	{AX620U_HCLK_SPI_S, "hclk_spi_s", "clk_ap_glb_sel", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x010, 18, 0, 0x20c, 18, 0x210, 18,},
	{AX620U_HCLK_SDIO_S, "hclk_sdio_s", "clk_ap_glb_sel", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x010, 17, 0, 0x20c, 17, 0x210, 17,},
	{AX620U_CLK_UART3, "clk_uart3", "clk_ap_sil_sel", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x010, 16, 0, 0x20c, 16, 0x210, 16,},
	{AX620U_CLK_UART2, "clk_uart2", "clk_ap_sil_sel", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x010, 15, 0, 0x20c, 15, 0x210, 15,},
	{AX620U_CLK_UART1, "clk_uart1", "clk_ap_sil_sel", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x010, 14, 0, 0x20c, 14, 0x210, 14,},
	{AX620U_CLK_UART0, "clk_uart0", "clk_ap_sil_sel", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x010, 13, 0, 0x20c, 13, 0x210, 13,},
	{AX620U_CLK_TMR64, "clk_tmr64", "clk_ap_rtc_sel", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x010, 12, 0, 0x20c, 12, 0x210, 12, "clk_tmr64",},
	{AX620U_CLK_TIMER3, "clk_timer3", "clk_ap_rtc_sel", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x010, 11, 0, 0x20c, 11, 0x210, 11,},
	{AX620U_CLK_TIMER2, "clk_timer2", "clk_ap_rtc_sel", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x010, 10, 0, 0x20c, 10, 0x210, 10,},
	{AX620U_CLK_TIMER1, "clk_timer1", "clk_ap_timer_sel", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x010, 9, 0, 0x20c, 9, 0x210, 9,},
	{AX620U_CLK_TIMER0, "clk_timer0", "clk_ap_timer_sel", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x010, 8, 0, 0x20c, 8, 0x210, 8,},
	{AX620U_CLK_SPI_S, "clk_spi_s", "clk_ap_spi1_sel", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x010, 7, 0, 0x20c, 7, 0x210, 7,},
	{AX620U_CLK_SPI_M2, "clk_spi_m2", "clk_ap_spi1_sel", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x010, 6, 0, 0x20c, 6, 0x210, 6,},
	{AX620U_CLK_SPI_M1, "clk_spi_m1", "clk_ap_spi0_sel", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x010, 5, 0, 0x20c, 5, 0x210, 5,},
	{AX620U_CLK_SPI_M0, "clk_spi_m0", "clk_ap_spi0_sel", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x010, 4, 0, 0x20c, 4, 0x210, 4,},
	{AX620U_CLK_SDIO_M1_CARD, "clk_sdio_m1_card", "clk_sdio_m_card_divn", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x010, 3, 0, 0x20c, 3, 0x210, 3,},
	{AX620U_CLK_SDIO_M0_CARD, "clk_sdio_m0_card", "clk_sdio_m_card_divn", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x010, 2, 0, 0x20c, 2, 0x210, 2,},
	{AX620U_CLK_RTC1, "clk_rtc1", "clk_ap_rtc_sel", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x010, 1, 0, 0x20c, 1, 0x210, 1,},
	{AX620U_CLK_RTC0, "clk_rtc0", "clk_ap_timer_sel", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x010, 0, 0, 0x20c, 0, 0x210, 0,},
	{AX620U_PCLK_WDT, "pclk_wdt", "clk_ap_glb_sel", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x014, 22, 0, 0x214, 22, 0x218, 22,},
	{AX620U_PCLK_USB, "pclk_usb", "clk_ap_glb_sel", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x014, 21, 0, 0x214, 21, 0x218, 21,},
	{AX620U_PCLK_UART3, "pclk_uart3", "clk_ap_glb_sel", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x014, 20, 0, 0x214, 20, 0x218, 20,},
	{AX620U_PCLK_UART2, "pclk_uart2", "clk_ap_glb_sel", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x014, 19, 0, 0x214, 19, 0x218, 19,},
	{AX620U_PCLK_UART1, "pclk_uart1", "clk_ap_glb_sel", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x014, 18, 0, 0x214, 18, 0x218, 18,},
	{AX620U_PCLK_UART0, "pclk_uart0", "clk_ap_glb_sel", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x014, 17, 0, 0x214, 17, 0x218, 17,},
	{AX620U_PCLK_TMR64, "pclk_tmr64", "clk_ap_glb_sel", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x014, 16, 0, 0x214, 16, 0x218, 16,},
	{AX620U_PCLK_TIMER, "pclk_timer", "clk_ap_glb_sel", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x014, 15, 0, 0x214, 15, 0x218, 15,},
	{AX620U_PCLK_SPI_M2, "pclk_spi_m2", "clk_ap_glb_sel", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x014, 14, 0, 0x214, 14, 0x218, 14,},
	{AX620U_PCLK_SPI_M1, "pclk_spi_m1", "clk_ap_glb_sel", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x014, 13, 0, 0x214, 13, 0x218, 13,},
	{AX620U_PCLK_SPI_M0, "pclk_spi_m0", "clk_ap_glb_sel", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x014, 12, 0, 0x214, 12, 0x218, 12,},
	{AX620U_PCLK_SDIO_M1, "pclk_sdio_m1", "clk_ap_glb_sel", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x014, 11, 0, 0x214, 11, 0x218, 11,},
	{AX620U_PCLK_SDIO_M0, "pclk_sdio_m0", "clk_ap_glb_sel", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x014, 10, 0, 0x214, 10, 0x218, 10,},
	{AX620U_PCLK_RTC1, "pclk_rtc1", "clk_ap_glb_sel", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x014, 9, 0, 0x214, 9, 0x218, 9,},
	{AX620U_PCLK_RTC0, "pclk_rtc0", "clk_ap_glb_sel", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x014, 8, 0, 0x214, 8, 0x218, 8,},
	{AX620U_PCLK_PWM2, "pclk_pwm2", "clk_ap_glb_sel", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x014, 7, 0, 0x214, 7, 0x218, 7,},
	{AX620U_PCLK_PWM1, "pclk_pwm1", "clk_ap_glb_sel", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x014, 6, 0, 0x214, 6, 0x218, 6,},
	{AX620U_PCLK_PWM0, "pclk_pwm0", "clk_ap_glb_sel", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x014, 5, 0, 0x214, 5, 0x218, 5,},
	{AX620U_PCLK_I2S_TDM_S, "pclk_i2s_tdm_s", "clk_ap_glb_sel", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x014, 4, 0, 0x214, 4, 0x218, 4,},
	{AX620U_PCLK_I2S_TDM_M, "pclk_i2s_tdm_m", "clk_ap_glb_sel", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x014, 3, 0, 0x214, 3, 0x218, 3,},
	{AX620U_PCLK_I2S_S, "pclk_i2s_s", "clk_ap_glb_sel", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x014, 2, 0, 0x214, 2, 0x218, 2,},
	{AX620U_PCLK_I2S_M, "pclk_i2s_m", "clk_ap_glb_sel", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x014, 1, 0, 0x214, 1, 0x218, 1,},
	{AX620U_PCLK_I2C_HS3, "pclk_i2c_hs3", "clk_ap_glb_sel", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x014, 0, 0, 0x214, 0, 0x218, 0,},
};

static struct axera_divider_clock ax620u_divider_clks_ap[] __initdata = {
	{AX620U_SCLK_I2S_TDM_DIVN, "sclk_i2s_tdm_divn", "sclk_i2s_tdm_sel", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x018, 0x01c, 0, 27, 5, 0, NULL,},
	{AX620U_SCLK_I2S_DIVN, "sclk_i2s_divn", "sclk_i2s_sel", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x018, 0x018, 26, 21, 5, 0, NULL,},
	{AX620U_CLK_SDIO_M_CARD_DIVN, "clk_sdio_m_card_divn", "clk_sdio_m_card_sel", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x018, 0x018, 20, 16, 4, 0, NULL,},
	{AX620U_CLK_OTG_FAST_DIVN, "clk_otg_fast_divn", "clk_otg_fast_sel", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x018, 0x018, 15, 11, 4, 0, NULL,},
	{AX620U_CLK_EMAC_RMII_RX_PAD_DIVN, "clk_emac_rmii_rx_pad_divn", "clk_emac_rmii_tx", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x018, 0x018, 4, 0, 4, 0, NULL,},
	{AX620U_CLK_I2S_REF2_DIVN, "clk_i2s_ref2_divn", "clk_i2s_ref2_sel", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x01c, 0x01c, 18, 13, 5, 0, NULL,},
	{AX620U_CLK_I2S_REF1_DIVN, "clk_i2s_ref1_divn", "clk_i2s_ref1_sel", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x01c, 0x01c, 12, 7, 5, 0, NULL,},
	{AX620U_CLK_I2S_REF0_DIVN, "clk_i2s_ref0_divn", "clk_i2s_ref0_sel", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x01c, 0x01c, 6, 1, 5, 0, NULL,},
};

static void __init ax620u_clk_ap_clk_init(struct device_node *np)
{
	struct axera_clock_data *clk_data;

	clk_data = axera_clk_init(np, AX620U_AP_NR_CLKS);
	if (!clk_data)
		return;

	axera_clk_register_mux(ax620u_mux_clks_ap, ARRAY_SIZE(ax620u_mux_clks_ap), clk_data);
	axera_clk_register_gate(ax620u_gate_clks_ap, ARRAY_SIZE(ax620u_gate_clks_ap), clk_data);
	axera_clk_register_divider(ax620u_divider_clks_ap, ARRAY_SIZE(ax620u_divider_clks_ap), clk_data);
}
CLK_OF_DECLARE(ax620u_clk_ap, "axera,ax620x-ap-clk", ax620u_clk_ap_clk_init);

/* flash_clk */
static const char *clk_h_ssi_sel_p[] __initdata = { "xtal_24m", "epll_250m", "epll_333m" };
static const char *clk_flash_gpio_sel_p[] __initdata = { "xtal_32k", "xtal_24m" };
static const char *clk_flash_glb_sel_p[] __initdata = { "xtal_24m", "cpll_156m", "cpll_208m", "epll_250m" };
static const char *clk_emmc_card_sel_p[] __initdata = { "epll_5m", "xtal_24m", "cpll_156m", "epll_200m" };

static struct axera_mux_clock ax620u_mux_clks_flash[] __initdata = {
	{AX620U_CLK_H_SSI_SEL, "clk_h_ssi_sel", clk_h_ssi_sel_p, ARRAY_SIZE(clk_h_ssi_sel_p), CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x000, 5, 2, 0, NULL,},
	{AX620U_CLK_FLASH_GPIO_SEL, "clk_flash_gpio_sel", clk_flash_gpio_sel_p, ARRAY_SIZE(clk_flash_gpio_sel_p), CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x000, 4, 1, 0, NULL,},
	{AX620U_CLK_FLASH_GLB_SEL, "clk_flash_glb_sel", clk_flash_glb_sel_p, ARRAY_SIZE(clk_flash_glb_sel_p), CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x000, 2, 2, 0, NULL,},
	{AX620U_CLK_EMMC_CARD_SEL, "clk_emmc_card_sel", clk_emmc_card_sel_p, ARRAY_SIZE(clk_emmc_card_sel_p), CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x000, 0, 2, 0, NULL,},
};

static struct axera_gate_clock ax620u_gate_clks_flash[] __initdata = {
	{AX620U_CLK_TLB_RTC, "clk_tlb_rtc", "xtal_32k", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x004, 6, 0, 0x0f0, 6, 0x0f4, 6,},
	{AX620U_CLK_TLB_PIN_FLASH, "clk_tlb_pin_flash", "xtal_24m", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x004, 5, 0, 0x0f0, 5, 0x0f4, 5,},
	{AX620U_CLK_H_SSI, "clk_h_ssi", "clk_h_ssi_divn", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x004, 3, 0, 0x0f0, 3, 0x0f4, 3,},
	{AX620U_CLK_FLASH_GPIO, "clk_flash_gpio", "clk_flash_gpio_sel", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x004, 2, 0, 0x0f0, 2, 0x0f4, 2,},
	{AX620U_CLK_FLASH_GLB, "clk_flash_glb", "clk_flash_glb_sel", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x004, 1, 0, 0x0f0, 1, 0x0f4, 1,},
	{AX620U_CLK_EMMC_CARD, "clk_emmc_card", "clk_emmc_card_divn", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x004, 0, 0, 0x0f0, 0, 0x0f4, 0,},
	{AX620U_PCLK_GPIO, "pclk_gpio", "clk_flash_glb", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x008, 3, 0, 0x0f8, 3, 0x0fc, 3,},
	{AX620U_PCLK_EMMC, "pclk_emmc", "clk_flash_glb", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x008, 2, 0, 0x0f8, 2, 0x0fc, 2,},
	{AX620U_HCLK_SPI, "hclk_spi", "clk_flash_glb", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x008, 1, 0, 0x0f8, 1, 0x0fc, 1,},
	{AX620U_ACLK_EMMC, "aclk_emmc", "clk_flash_glb", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x008, 0, 0, 0x0f8, 0, 0x0fc, 0,},
};

static struct axera_divider_clock ax620u_divider_clks_flash[] __initdata = {
	{AX620U_CLK_H_SSI_DIVN, "clk_h_ssi_divn", "clk_h_ssi_sel", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x00c, 0x00c, 9, 5, 4, 0, NULL,},
	{AX620U_CLK_EMMC_CARD_DIVN, "clk_emmc_card_divn", "clk_emmc_card_sel", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x00c, 0x00c, 4, 0, 4, 0, NULL,},
};

static void __init ax620u_clk_flash_clk_init(struct device_node *np)
{
	struct axera_clock_data *clk_data;

	clk_data = axera_clk_init(np, AX620U_FLASH_NR_CLKS);
	if (!clk_data)
		return;

	axera_clk_register_mux(ax620u_mux_clks_flash, ARRAY_SIZE(ax620u_mux_clks_flash), clk_data);
	axera_clk_register_gate(ax620u_gate_clks_flash, ARRAY_SIZE(ax620u_gate_clks_flash), clk_data);
	axera_clk_register_divider(ax620u_divider_clks_flash, ARRAY_SIZE(ax620u_divider_clks_flash), clk_data);
}
CLK_OF_DECLARE(ax620u_clk_flash, "axera,ax620x-flash-clk", ax620u_clk_flash_clk_init);

/* isp_clk */
static const char *clk_sen5_ref_sel_p[] __initdata = { "xtal_24m", "vpll_27m", "vpll_37p125m", "vpll_74p25m" };
static const char *clk_sen4_ref_sel_p[] __initdata = { "xtal_24m", "vpll_27m", "vpll_37p125m", "vpll_74p25m" };
static const char *clk_sen3_ref_sel_p[] __initdata = { "xtal_24m", "vpll_27m", "vpll_37p125m", "vpll_74p25m" };
static const char *clk_sen2_ref_sel_p[] __initdata = { "xtal_24m", "vpll_27m", "vpll_37p125m", "vpll_74p25m" };
static const char *clk_sen1_ref_sel_p[] __initdata = { "xtal_24m", "vpll_27m", "vpll_37p125m", "vpll_74p25m" };
static const char *clk_sen0_ref_sel_p[] __initdata = { "xtal_24m", "vpll_27m", "vpll_37p125m", "vpll_74p25m" };
static const char *clk_isp_glb_sel_p[] __initdata = { "cpll_208m", "epll_333m", "npll_400m", "epll_500m", "npll_533m" };
static const char *clk_isp_csi_sel_p[] __initdata = { "vpll_148p5m", "cpll_208m" };

static struct axera_mux_clock ax620u_mux_clks_isp[] __initdata = {
	{AX620U_CLK_SEN5_REF_SEL, "clk_sen5_ref_sel", clk_sen5_ref_sel_p, ARRAY_SIZE(clk_sen5_ref_sel_p), CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x330, 16, 2, 0, NULL,},
	{AX620U_CLK_SEN4_REF_SEL, "clk_sen4_ref_sel", clk_sen4_ref_sel_p, ARRAY_SIZE(clk_sen4_ref_sel_p), CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x330, 8, 2, 0, NULL,},
	{AX620U_CLK_SEN3_REF_SEL, "clk_sen3_ref_sel", clk_sen3_ref_sel_p, ARRAY_SIZE(clk_sen3_ref_sel_p), CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x330, 0, 2, 0, NULL,},
	{AX620U_CLK_SEN2_REF_SEL, "clk_sen2_ref_sel", clk_sen2_ref_sel_p, ARRAY_SIZE(clk_sen2_ref_sel_p), CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x000, 15, 2, 0, NULL,},
	{AX620U_CLK_SEN1_REF_SEL, "clk_sen1_ref_sel", clk_sen1_ref_sel_p, ARRAY_SIZE(clk_sen1_ref_sel_p), CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x000, 13, 2, 0, NULL,},
	{AX620U_CLK_SEN0_REF_SEL, "clk_sen0_ref_sel", clk_sen0_ref_sel_p, ARRAY_SIZE(clk_sen0_ref_sel_p), CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x000, 11, 2, 0, NULL,},
	{AX620U_CLK_ISP_GLB_SEL, "clk_isp_glb_sel", clk_isp_glb_sel_p, ARRAY_SIZE(clk_isp_glb_sel_p), CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x000, 2, 3, 0, NULL,},
	{AX620U_CLK_ISP_CSI_SEL, "clk_isp_csi_sel", clk_isp_csi_sel_p, ARRAY_SIZE(clk_isp_csi_sel_p), CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x000, 1, 1, 0, NULL,},
};

static struct axera_gate_clock ax620u_gate_clks_isp[] __initdata = {
	{AX620U_CLK_SEN5_REF, "clk_sen5_ref", "clk_sen5_ref_divn", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x330, 23, 1, 0x000, 0, 0x000, 0,},
	{AX620U_CLK_SEN4_REF, "clk_sen4_ref", "clk_sen4_ref_divn", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x330, 15, 1, 0x000, 0, 0x000, 0,},
	{AX620U_CLK_SEN3_REF, "clk_sen3_ref", "clk_sen3_ref_divn", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x330, 7, 1, 0x000, 0, 0x000, 0,},
	{AX620U_CLK_SEN2_REF, "clk_sen2_ref", "clk_sen2_ref_divn", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x004, 6, 0, 0x194, 6, 0x198, 6,},
	{AX620U_CLK_SEN1_REF, "clk_sen1_ref", "clk_sen1_ref_divn", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x004, 5, 0, 0x194, 5, 0x198, 5,},
	{AX620U_CLK_SEN0_REF, "clk_sen0_ref", "clk_sen0_ref_divn", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x004, 4, 0, 0x194, 4, 0x198, 4,},
	{AX620U_CLK_PPI_TXCLKESC_ALL, "clk_ppi_txclkesc_all", "epll_10m", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x004, 3, 0, 0x194, 3, 0x198, 3,},
	{AX620U_CLK_ISP_CSI, "clk_isp_csi", "clk_isp_csi_sel", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x004, 2, 0, 0x194, 2, 0x198, 2,},
	{AX620U_CLK_DPHY_RX_REF, "clk_dphy_rx_ref", "epll_100m", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x004, 1, 0, 0x194, 1, 0x198, 1,},
	{AX620U_CLK_CFG_PHY, "clk_cfg_phy", "epll_100m", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x004, 0, 0, 0x194, 0, 0x198, 0,},
	{AX620U_PPI2_TXCLKESC_ALL, "ppi2_txclkesc_all", "clk_ppi_txclkesc_all", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x008, 16, 0, 0x19c, 16, 0x1a0, 16,},
	{AX620U_PPI1_TXCLKESC_ALL, "ppi1_txclkesc_all", "clk_ppi_txclkesc_all", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x008, 15, 0, 0x19c, 15, 0x1a0, 15,},
	{AX620U_PPI0_TXCLKESC_ALL, "ppi0_txclkesc_all", "clk_ppi_txclkesc_all", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x008, 14, 0, 0x19c, 14, 0x1a0, 14,},
	{AX620U_PCLK_CSI3_ISP, "pclk_csi3_isp", "clk_isp_csi", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x008, 13, 0, 0x19c, 13, 0x1a0, 13,},
	{AX620U_PCLK_CSI2_ISP, "pclk_csi2_isp", "clk_isp_csi", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x008, 12, 0, 0x19c, 12, 0x1a0, 12,},
	{AX620U_PCLK_CSI1_ISP, "pclk_csi1_isp", "clk_isp_csi", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x008, 11, 0, 0x19c, 11, 0x1a0, 11,},
	{AX620U_PCLK_CSI0_ISP, "pclk_csi0_isp", "clk_isp_csi", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x008, 10, 0, 0x19c, 10, 0x1a0, 10,},
	{AX620U_CLK_PHY_REF2, "clk_phy_ref2", "clk_dphy_rx_ref", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x008, 9, 0, 0x19c, 9, 0x1a0, 9,},
	{AX620U_CLK_PHY_REF1, "clk_phy_ref1", "clk_dphy_rx_ref", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x008, 8, 0, 0x19c, 8, 0x1a0, 8,},
	{AX620U_CLK_PHY_REF0, "clk_phy_ref0", "clk_dphy_rx_ref", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x008, 7, 0, 0x19c, 7, 0x1a0, 7,},
	{AX620U_CLK_ISP_ITP, "clk_isp_itp", "clk_isp_glb_sel", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x008, 6, 0, 0x19c, 6, 0x1a0, 6,},
	{AX620U_CLK_ISP_IFE, "clk_isp_ife", "clk_isp_glb_sel", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x008, 5, 0, 0x19c, 5, 0x1a0, 5,},
	{AX620U_CLK_ISP_GDC, "clk_isp_gdc", "clk_isp_glb_sel", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x008, 4, 0, 0x19c, 4, 0x1a0, 4,},
	{AX620U_CLK_ISP_AXIM, "clk_isp_axim", "clk_isp_glb_sel", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x008, 3, 0, 0x19c, 3, 0x1a0, 3,},
	{AX620U_CLK_CFG_PHY2, "clk_cfg_phy2", "clk_cfg_phy", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x008, 2, 0, 0x19c, 2, 0x1a0, 2,},
	{AX620U_CLK_CFG_PHY1, "clk_cfg_phy1", "clk_cfg_phy", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x008, 1, 0, 0x19c, 1, 0x1a0, 1,},
	{AX620U_CLK_CFG_PHY0, "clk_cfg_phy0", "clk_cfg_phy", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x008, 0, 0, 0x19c, 0, 0x1a0, 0,},
};

static struct axera_divider_clock ax620u_divider_clks_isp[] __initdata = {
	{AX620U_CLK_SEN5_REF_DIVN, "clk_sen5_ref_divn", "clk_sen5_ref_sel", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x330, 0x330, 22, 18, 4, 0, NULL,},
	{AX620U_CLK_SEN4_REF_DIVN, "clk_sen4_ref_divn", "clk_sen4_ref_sel", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x330, 0x330, 14, 10, 4, 0, NULL,},
	{AX620U_CLK_SEN3_REF_DIVN, "clk_sen3_ref_divn", "clk_sen3_ref_sel", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x330, 0x330, 6, 2, 4, 0, NULL,},
	{AX620U_CLK_SEN2_REF_DIVN, "clk_sen2_ref_divn", "clk_sen2_ref_sel", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x00c, 0x00c, 14, 10, 4, 0, NULL,},
	{AX620U_CLK_SEN1_REF_DIVN, "clk_sen1_ref_divn", "clk_sen1_ref_sel", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x00c, 0x00c, 9, 5, 4, 0, NULL,},
	{AX620U_CLK_SEN0_REF_DIVN, "clk_sen0_ref_divn", "clk_sen0_ref_sel", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x00c, 0x00c, 4, 0, 4, 0, NULL,},
};

static void __init ax620u_clk_isp_clk_init(struct device_node *np)
{
	struct axera_clock_data *clk_data;

	clk_data = axera_clk_init(np, AX620U_ISP_NR_CLKS);
	if (!clk_data)
		return;

	axera_clk_register_mux(ax620u_mux_clks_isp, ARRAY_SIZE(ax620u_mux_clks_isp), clk_data);
	axera_clk_register_gate(ax620u_gate_clks_isp, ARRAY_SIZE(ax620u_gate_clks_isp), clk_data);
	axera_clk_register_divider(ax620u_divider_clks_isp, ARRAY_SIZE(ax620u_divider_clks_isp), clk_data);
}
CLK_OF_DECLARE(ax620u_clk_isp, "axera,ax620x-isp-clk", ax620u_clk_isp_clk_init);

/* mm_clk */
static const char *pclk_dispc_sel_p[] __initdata = { "xtal_24m", "vpll_27m", "cpll_208m", "epll_333m", "vpll_594m" };
static const char *clk_mm_glb_sel_p[] __initdata = { "xtal_24m", "cpll_312m", "npll_400m", "epll_500m", "npll_533m", "vpll_594m" };

static struct axera_mux_clock ax620u_mux_clks_mm[] __initdata = {
	{AX620U_PCLK_DISPC_SEL, "pclk_dispc_sel", pclk_dispc_sel_p, ARRAY_SIZE(pclk_dispc_sel_p), CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x000, 3, 3, 0, NULL,},
	{AX620U_CLK_MM_GLB_SEL, "clk_mm_glb_sel", clk_mm_glb_sel_p, ARRAY_SIZE(clk_mm_glb_sel_p), CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x000, 0, 3, 0, NULL,},
};

static struct axera_gate_clock ax620u_gate_clks_mm[] __initdata = {
	{AX620U_CLK_TDP, "clk_tdp", "clk_mm_glb_sel", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x004, 5, 0, 0x084, 5, 0x088, 5,},
	{AX620U_CLK_FBCE, "clk_fbce", "clk_mm_glb_sel", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x004, 4, 0, 0x084, 4, 0x088, 4,},
	{AX620U_CLK_FBCD, "clk_fbcd", "clk_mm_glb_sel", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x004, 3, 0, 0x084, 3, 0x088, 3,},
	{AX620U_CLK_DPU, "clk_dpu", "clk_mm_glb_sel", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x004, 2, 0, 0x084, 2, 0x088, 2,},
	{AX620U_CLK_DPI, "clk_dpi", "pclk_dispc_divn", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x004, 1, 0, 0x084, 1, 0x088, 1,},
	{AX620U_CLK_CMD, "clk_cmd", "clk_mm_glb_sel", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x004, 0, 0, 0x084, 0, 0x088, 0,},
};

static struct axera_divider_clock ax620u_divider_clks_mm[] __initdata = {
	{AX620U_PCLK_DISPC_DIVN, "pclk_dispc_divn", "pclk_dispc_sel", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x008, 0x008, 4, 0, 4, 0, NULL,},
};

static void __init ax620u_clk_mm_clk_init(struct device_node *np)
{
	struct axera_clock_data *clk_data;

	clk_data = axera_clk_init(np, AX620U_MM_NR_CLKS);
	if (!clk_data)
		return;

	axera_clk_register_mux(ax620u_mux_clks_mm, ARRAY_SIZE(ax620u_mux_clks_mm), clk_data);
	axera_clk_register_gate(ax620u_gate_clks_mm, ARRAY_SIZE(ax620u_gate_clks_mm), clk_data);
	axera_clk_register_divider(ax620u_divider_clks_mm, ARRAY_SIZE(ax620u_divider_clks_mm), clk_data);
}
CLK_OF_DECLARE(ax620u_clk_mm, "axera,ax620x-mm-clk", ax620u_clk_mm_clk_init);
