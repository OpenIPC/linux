
/*
 * Axera AX620X clock driver
 *
 * Copyright (c) 2019-2020 Axera Technology Co., Ltd.
 *
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <linux/kernel.h>
#include <linux/clk-provider.h>
#include <linux/clkdev.h>
#include <linux/io.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_device.h>
#include <linux/slab.h>
#include <linux/clk.h>
#include <dt-bindings/clock/ax620e-clock.h>
#include "clk-axera.h"


/*pll*/

#define GRP_PLL_RE_OPEN 0x230
#define GRP_PLL_RDY_STS 0x1C0
/*
#define HPLL_ON_CFG 0x20C
#define HPLL_CFG0 0x184
#define HPLL_CFG1 0x190
#define HPLL_STS 0x180
#define HPLL_REF_FREQ 12000000
#define HPLL_MIN_RATE 1000000000
#define HPLL_MAX_RATE 1228000000
*/


#define CPUPLL_ON_CFG 0x1E8
#define CPUPLL_CFG0 0xC4
#define CPUPLL_CFG1 0xD0
#define CPUPLL_STS 0xC0
#define CPUPLL_REF_FREQ 12000000 //cpll_12m
#define CPUPLL_MIN_RATE 1500000000
#define CPUPLL_MAX_RATE 1500000000

#define AX_VPLL0_BASE 0x02210050
#define AX_VPLL0_MASK 0x1F
#define AX_VPLL0_TAG  0x11B
#define AX_VPLL0_TAG_1  0x13C
#define AX_VPLL0_VAL  0x6
#define AX_DEC_SZ_1M  1000000


struct axera_pll_cfg_reg ax620x_pll_cfg_array[] = {
//	{AX620X_PLL_CPLL,  "hpll","cpll_12m", GRP_PLL_RE_OPEN, HPLL_ON_CFG, HPLL_CFG0, HPLL_CFG1, HPLL_STS,GRP_PLL_RDY_STS, 6, HPLL_REF_FREQ, HPLL_MIN_RATE, HPLL_MAX_RATE},
	{AX620X_PLL_CPULL, "cpupll","cpll_12m", GRP_PLL_RE_OPEN, CPUPLL_ON_CFG, CPUPLL_CFG0, CPUPLL_CFG1, CPUPLL_STS,GRP_PLL_RDY_STS, 3, CPUPLL_REF_FREQ, CPUPLL_MIN_RATE, CPUPLL_MAX_RATE},
/*	{AX620X_PLL_HPLL,  "hpll","cpll_12m", GRP_PLL_RE_OPEN, HPLL_ON_CFG, HPLL_CFG0, HPLL_CFG1, HPLL_STS,GRP_PLL_RDY_STS, 6, HPLL_REF_FREQ, HPLL_MIN_RATE, HPLL_MAX_RATE},
	{AX620X_PLL_NPLL,  "npll", "cpll_12m", GRP_PLL_RE_OPEN, HPLL_ON_CFG, HPLL_CFG0, HPLL_CFG1, HPLL_STS,GRP_PLL_RDY_STS, 6, HPLL_REF_FREQ, HPLL_MIN_RATE, HPLL_MAX_RATE},
	{AX620X_PLL_VPLL0, "vpll0", "cpll_12m", GRP_PLL_RE_OPEN, HPLL_ON_CFG, HPLL_CFG0, HPLL_CFG1, HPLL_STS,GRP_PLL_RDY_STS, 6, HPLL_REF_FREQ, HPLL_MIN_RATE, HPLL_MAX_RATE},
	{AX620X_PLL_VPLL1, "vpll1", "cpll_12m", GRP_PLL_RE_OPEN, HPLL_ON_CFG, HPLL_CFG0, HPLL_CFG1, HPLL_STS,GRP_PLL_RDY_STS, 6, HPLL_REF_FREQ, HPLL_MIN_RATE, HPLL_MAX_RATE},
	{AX620X_PLL_EPLL,  "epll", "cpll_12m", GRP_PLL_RE_OPEN, HPLL_ON_CFG, HPLL_CFG0, HPLL_CFG1, HPLL_STS,GRP_PLL_RDY_STS, 6, HPLL_REF_FREQ, HPLL_MIN_RATE, HPLL_MAX_RATE},
	{AX620X_PLL_DPLL,  "dpll", "cpll_12m", GRP_PLL_RE_OPEN, HPLL_ON_CFG, HPLL_CFG0, HPLL_CFG1, HPLL_STS,GRP_PLL_RDY_STS, 6, HPLL_REF_FREQ, HPLL_MIN_RATE, HPLL_MAX_RATE},*/
};

static void __init ax620x_pllc_clk_init(struct device_node *np)
{
	struct axera_clock_data *clk_data_top;

	clk_data_top = axera_clk_init(np, AX620X_PLL_NR);
	if (!clk_data_top)
		return;

	axera_clk_register_pll(ax620x_pll_cfg_array, ARRAY_SIZE(ax620x_pll_cfg_array), clk_data_top);
}
CLK_OF_DECLARE(ax650x_clk_pllc_glb, "axera,ax620x-pllc-clk", ax620x_pllc_clk_init);


/*cpu*/
static const char *clk_h_ssi_sel[] __initdata = { "cpll_24m", "epll_125m", "cpll_208m", "cpll_312m", "npll_400m", "cpll_416m" };
static const char *clk_cpu_sel[] __initdata = { "cpll_24m", "cpll_208m", "epll_500m", "npll_800m", "cpupll_1200m" };
static const char *clk_bus_flash_sel[] __initdata = { "cpll_24m", "epll_125m", "cpll_208m", "cpll_312m" };

static struct axera_mux_clock ax620x_mux_clks_cpu[] __initdata = {
	{AX620X_CLK_H_SSI_SEL, "clk_h_ssi_sel", clk_h_ssi_sel, ARRAY_SIZE(clk_h_ssi_sel), CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x0, 7, 3, 0, NULL,},
	{AX620X_CLK_CPU_SEL, "clk_cpu_sel", clk_cpu_sel, ARRAY_SIZE(clk_cpu_sel), CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x0, 2, 3, 0, NULL,},
	{AX620X_CLK_BUS_FLASH_SEL, "clk_bus_flash_sel", clk_bus_flash_sel, ARRAY_SIZE(clk_bus_flash_sel), CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x0, 0, 2, 0, NULL,},
};

static struct axera_gate_clock ax620x_gate_clks_cpu[] __initdata = {
	{AX620X_CLK_H_SSI_EB, "clk_h_ssi_eb", "clk_h_ssi_divn", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x4, 3, 1, 0, 0, 0, 0,},
	{AX620X_CLK_CPU_24M_EB, "clk_cpu_24m_eb", "cpll_24m", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x4, 1, 1, 0, 0, 0, 0,},
	{AX620X_CLK_CS_APB_EB, "clk_cs_apb_eb", "cpll_24m", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x8, 3, 1, 0, 0, 0, 0,},
};

static struct axera_divider_clock ax620x_div_clks_cpu[] __initdata = {
	{AX620X_CLK_H_SSI_DIVN, "clk_h_ssi_divn", "clk_h_ssi_sel", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0xC, 0xC, 11, 7, 4, 0, NULL,},
};

static void __init ax620x_clk_cpu_clk_init(struct device_node *np)
{

	struct axera_clock_data *clk_data_top;
	clk_data_top = axera_clk_init(np, AX620X_CPU_NR_CLKS);
	if (!clk_data_top)
		return;

	axera_clk_register_mux(ax620x_mux_clks_cpu, ARRAY_SIZE(ax620x_mux_clks_cpu), clk_data_top);
	axera_clk_register_gate(ax620x_gate_clks_cpu, ARRAY_SIZE(ax620x_gate_clks_cpu), clk_data_top);
	axera_clk_register_divider(ax620x_div_clks_cpu, ARRAY_SIZE(ax620x_div_clks_cpu), clk_data_top);
}

CLK_OF_DECLARE(ax620x_clk_cpu_glb, "axera,ax620x-cpu-clk", ax620x_clk_cpu_clk_init);

/*common*/
/* clocks in AO (always on) controller */
static struct axera_fixed_rate_clock AX620X_fixed_rate_clks[] __initdata = {
	{AX620X_CLK_RTC_OUT_32K, "rtc_out_32k", NULL, 0, 32768,},
	{AX620X_REF24M, "xtal_24m", NULL, 0, 24000000,},
	{AX620X_CPLL, "cpll", NULL, 0, 2496000000,},
//	{AX620X_CPUPLL, "cpupll", NULL, 0, 1200000000,},
	{AX620X_HPLL, "hpll", NULL, 0, 1228800000,},
	{AX620X_NPLL, "npll", NULL, 0, 1600000000,},
	{AX620X_VPLL0, "vpll0", NULL, 0, 1188000000,},//VPLL0
	{AX620X_VPLL1, "vpll1", NULL, 0, 1188000000,},//VPLL1
	{AX620X_EPLL, "epll", NULL, 0, 1500000000,},
	{AX620X_DPLL, "dpll", NULL, 0, 3200000000,},
};

static struct axera_fixed_factor_clock AX620X_fixed_factor_clks[] __initdata = {
	{AX620X_CPLL_2496M, "cpll_2496m", "cpll", 1, 1, 0,},
	{AX620X_CPLL_1248M, "cpll_1248m", "cpll_2496m", 1, 2, 0,},
	{AX620X_CPLL_12M, "cpll_12m", "cpll_2496m", 1, 208, 0,},
	{AX620X_CPLL_624M, "cpll_624m", "cpll_1248m", 1, 2, 0,},
	{AX620X_CPLL_416M, "cpll_416m", "cpll_1248m", 1, 3, 0,},
	{AX620X_CPLL_249P6M, "cpll_249p6m", "cpll_1248m", 1, 5, 0,},
	{AX620X_CPLL_312M, "cpll_312m", "cpll_624m", 1, 2, 0,},
	{AX620X_CPLL_208M, "cpll_208m", "cpll_624m", 1, 3, 0,},
	{AX620X_CPLL_19P2M, "cpll_19p2m", "cpll_249p6m", 1, 5, 0,},
	{AX620X_CPLL_156M, "cpll_156m", "cpll_312m", 1, 2, 0,},
	{AX620X_CPLL_24M, "cpll_24m", "cpll_312m", 1, 13, 0,},
	{AX620X_CPLL_78M, "cpll_78m", "cpll_156m", 1, 2, 0,},
	{AX620X_CPLL_39M, "cpll_39m", "cpll_78m", 1, 2, 0,},
	{AX620X_CPLL_26M, "cpll_26m", "cpll_78m", 1, 3, 0,},
	{AX620X_HPLL_1228P8M, "hpll_1228p8m", "hpll", 1, 1, 0,},
	{AX620X_HPLL_245P76M, "hpll_245p76m", "hpll_1228p8m", 1, 5, 0,},
	{AX620X_HPLL_49p152M, "hpll_49p152m", "hpll_245p76m", 1, 5, 0,},
	{AX620X_HPLL_81P92M, "hpll_81p92m", "hpll_245p76m", 1, 3, 0,},
	{AX620X_HPLL_24P576M, "hpll_24p576m", "hpll_49p152m", 1, 2, 0,},
	{AX620X_HPLL_16P384M, "hpll_16p384m", "hpll_49p152m", 1, 3, 0,},
	{AX620X_HPLL_40P96M, "hpll_40p96m", "hpll_81p92m", 1, 2, 0,},
	{AX620X_HPLL_12P288M, "hpll_12p288m", "hpll_24p576m", 1, 2, 0,},
	{AX620X_HPLL_20P48M, "hpll_20p48m", "hpll_40p96m", 1, 2, 0,},
	{AX620X_HPLL_4096K, "hpll_4096k", "hpll_20p48m", 1, 5, 0,},
	{AX620X_NPLL_1600M, "npll_1600m", "npll", 1, 1, 0,},
	{AX620X_NPLL_800M, "npll_800m", "npll_1600m", 1, 2, 0,},
	{AX620X_NPLL_400M, "npll_400m", "npll_800m", 1, 2, 0,},
	{AX620X_NPLL_200M, "npll_200m", "npll_400m", 1, 2, 0,},
	{AX620X_NPLL_100M, "npll_100m", "npll_200m", 1, 2, 0,},
	{AX620X_NPLL_50M, "npll_50m", "npll_100m", 1, 2, 0,},
	{AX620X_NPLL_25M, "npll_25m", "npll_50m", 1, 2, 0,},
	{AX620X_NPLL_533M, "npll_533m", "npll_1600m", 1, 3, 0,},
	{AX620X_VPLL0_1188M, "vpll0_1188m", "vpll0", 1, 1, 0,},
	{AX620X_VPLL0_108M, "vpll0_108m", "vpll0_1188m", 1, 11, 0,},
	{AX620X_VPLL0_594M, "vpll0_594m", "vpll0_1188m", 1, 2, 0,},
	{AX620X_VPLL0_198M, "vpll0_198m", "vpll0_594m", 1, 3, 0,},
	{AX620X_VPLL0_118P8M, "vpll0_118p8m", "vpll0_594m", 1, 5, 0,},
	{AX620X_VPLL0_297M, "vpll0_297m", "vpll0_594m", 1, 2, 0,},
	{AX620X_VPLL0_148P5M, "vpll0_148p5m", "vpll0_297m", 1, 2, 0,},
	{AX620X_VPLL0_27M, "vpll0_27m", "vpll0_297m", 1, 11, 0,},
	{AX620X_VPLL0_74P25M, "vpll0_74p25m", "vpll0_148p5m", 1, 2, 0,},
	{AX620X_VPLL0_37P125M, "vpll0_37p125m", "vpll0_74p25m", 1, 2, 0,},
	{AX620X_VPLL1_1188M, "vpll1_1188m", "vpll1", 1, 1, 0,},
	{AX620X_VPLL1_108M, "vpll1_108m", "vpll1_1188m", 1, 11, 0,},
	{AX620X_VPLL1_594M, "vpll1_594m", "vpll1_1188m", 1, 2, 0,},
	{AX620X_VPLL1_198M, "vpll1_198m", "vpll1_594m", 1, 3, 0,},
	{AX620X_VPLL1_118P8M, "vpll1_118p8m", "vpll1_594m", 1, 5, 0,},
	{AX620X_VPLL1_297M, "vpll1_297m", "vpll1_594m", 1, 2, 0,},
	{AX620X_VPLL1_148M, "vpll1_148m", "vpll1_297m", 1, 2, 0,},
	{AX620X_VPLL1_27M, "vpll1_27m", "vpll1_297m", 1, 11, 0,},
	{AX620X_VPLL1_74P25M, "vpll1_74p25m", "vpll1_148m", 1, 2, 0,},
	{AX620X_VPLL1_37P125M, "vpll1_37p125m", "vpll1_74p25m", 1, 2, 0,},
	{AX620X_EPLL_1500M, "epll_1500m", "epll", 1, 1, 0,},
	{AX620X_EPLL_500M, "epll_500m", "epll_1500m", 1, 3, 0,},
	{AX620X_EPLL_750M, "epll_750m", "epll_1500m", 1, 2, 0,},
	{AX620X_EPLL_250M, "epll_250m", "epll_500m", 1, 2, 0,},
	{AX620X_EPLL_375M, "epll_375m", "epll_750m", 1, 2, 0,},
	{AX620X_EPLL_100M, "epll_100m", "epll_500m", 1, 5, 0,},
	{AX620X_EPLL_125M, "epll_125m", "epll_250m", 1, 2, 0,},
	{AX620X_EPLL_20M, "epll_20m", "epll_100m", 1, 5, 0,},
	{AX620X_EPLL_50M, "epll_50m", "epll_100m", 1, 2, 0,},
	{AX620X_EPLL_62P5M, "epll_62p5m", "epll_125m", 1, 2, 0,},
	{AX620X_EPLL_10M, "epll_10m", "epll_20m", 1, 2, 0,},
	{AX620X_EPLL_25M, "epll_25m", "epll_50m", 1, 2, 0,},
	{AX620X_EPLL_31P25M, "epll_31p25m", "epll_62p5m", 1, 2, 0,},
	{AX620X_EPLL_5M, "epll_5m", "epll_25m", 1, 5, 0,},
	{AX620X_DPLL_3200M, "dpll_3200m", "dpll", 1, 1, 0,},
	{AX620X_DPLL_1600M, "dpll_1600m", "dpll_3200m", 1, 2, 0,},
	{AX620X_DPLL_800M, "dpll_800m", "dpll_1600m", 1, 2, 0,},
	{AX620X_DPLL_400M, "dpll_400m", "dpll_800m", 1, 2, 0,},
	{AX620X_DPLL_200M, "dpll_200m", "dpll_400m", 1, 2, 0,},
	{AX620X_DPLL_100M, "dpll_100m", "dpll_200m", 1, 2, 0,},
	{AX620X_DPLL_50M, "dpll_50m", "dpll_100m", 1, 2, 0,},
	{AX620X_DPLL_25M, "dpll_25m", "dpll_50m", 1, 2, 0,},
	{AX620X_CPUPLL_1200M, "cpupll_1200m", "cpupll", 1, 1, CLK_SET_RATE_PARENT,},
	{AX620X_CPUPLL_600M, "cpupll_600m", "cpupll", 1, 2, CLK_SET_RATE_PARENT,},
	{AX620X_CPUPLL_300M, "cpupll_300m", "cpupll_600m", 1, 2, CLK_SET_RATE_PARENT,},
	{AX620X_CPUPLL_150M, "cpupll_150m", "cpupll_300m", 1, 2, CLK_SET_RATE_PARENT,},
	{AX620X_CPUPLL_75M, "cpupll_75m", "cpupll_150m", 1, 2, CLK_SET_RATE_PARENT,},
	{AX620X_CPUPLL_25M, "cpupll_25m", "cpupll_75m", 1, 3, CLK_SET_RATE_PARENT,},
};

static const char *aclk_isp_top_sel[] __initdata = { "cpll_24m", "cpll_208m", "cpll_312m", "cpll_416m", "epll_500m", "npll_533m" };
static const char *aclk_cpu_top_sel[] __initdata = { "cpll_24m", "cpll_208m", "cpll_312m", "cpll_416m", "epll_500m", "npll_533m" };
static const char *MCLK5_sel[] __initdata = { "cpll_12m", "cpll_19p2m", "hpll_20p48m", "cpll_24m", "hpll_24p576m", "epll_25m", "cpll_26m", "vpll0_27m", "vpll1_27m", "epll_50m", "vpll0_74p25m", "vpll1_74p25m", "epll_125m" };
static const char *MCLK4_sel[] __initdata = { "cpll_12m", "cpll_19p2m", "hpll_20p48m", "cpll_24m", "hpll_24p576m", "epll_25m", "cpll_26m", "vpll0_27m", "vpll1_27m", "epll_50m", "vpll0_74p25m", "vpll1_74p25m", "epll_125m" };
static const char *MCLK3_sel[] __initdata = { "cpll_12m", "cpll_19p2m", "hpll_20p48m", "cpll_24m", "hpll_24p576m", "epll_25m", "cpll_26m", "vpll0_27m", "vpll1_27m", "epll_50m", "vpll0_74p25m", "vpll1_74p25m", "epll_125m" };
static const char *MCLK2_sel[] __initdata = { "cpll_12m", "cpll_19p2m", "hpll_20p48m", "cpll_24m", "hpll_24p576m", "epll_25m", "cpll_26m", "vpll0_27m", "vpll1_27m", "epll_50m", "vpll0_74p25m", "vpll1_74p25m", "epll_125m" };
static const char *MCLK1_sel[] __initdata = { "cpll_12m", "cpll_19p2m", "hpll_20p48m", "cpll_24m", "hpll_24p576m", "epll_25m", "cpll_26m", "vpll0_27m", "vpll1_27m", "epll_50m", "vpll0_74p25m", "vpll1_74p25m", "epll_125m" };
static const char *MCLK0_sel[] __initdata = { "cpll_12m", "cpll_19p2m", "hpll_20p48m", "cpll_24m", "hpll_24p576m", "epll_25m", "cpll_26m", "vpll0_27m", "vpll1_27m", "epll_50m", "vpll0_74p25m", "vpll1_74p25m", "epll_125m" };
static const char *clk_vi_sel[] __initdata = { "epll_100m", "cpll_208m", "cpll_312m" };
static const char *clk_nx_vo1_comm_sel[] __initdata = { "vpll1_108m", "vpll1_118p8m", "vpll1_198m", "vpll1_297m" };
static const char *clk_nx_vo0_comm_sel[] __initdata = { "vpll0_108m", "vpll0_118p8m", "vpll0_198m", "vpll0_297m" };
static const char *clk_isp_mm_sel[] __initdata = { "epll_100m", "cpll_156m", "epll_250m", "cpll_312m", "cpll_416m" };
static const char *clk_dbc_gpio_sel[] __initdata = { "rtc_out_32k", "xtal_24m" };
static const char *clk_1x_vo1_comm_sel[] __initdata = { "vpll1_108m", "vpll1_118p8m", "vpll1_198m", "vpll1_297m" };
static const char *clk_1x_vo0_comm_sel[] __initdata = { "vpll0_108m", "vpll0_118p8m", "vpll0_198m", "vpll0_297m" };
static const char *aclk_vpu_top_sel[] __initdata = { "cpll_24m", "cpll_208m", "cpll_312m", "cpll_416m", "epll_500m", "npll_533m" };
static const char *aclk_ocm_top_sel[] __initdata = { "cpll_24m", "cpll_208m", "cpll_312m", "cpll_416m", "epll_500m", "npll_533m" };
static const char *aclk_nn_top_sel[] __initdata = { "cpll_24m", "cpll_208m", "cpll_312m", "cpll_416m", "epll_500m", "npll_533m" };
static const char *aclk_mm_top_sel[] __initdata = { "cpll_24m", "cpll_208m", "cpll_312m", "cpll_416m", "epll_500m", "npll_533m" };
static const char *pclk_top_sel[] __initdata = { "cpll_24m", "epll_100m", "cpll_156m", "cpll_208m" };

static struct axera_mux_clock ax620x_mux_clks_common[] __initdata = {
	{AX620X_ACLK_ISP_TOP_SEL, "aclk_isp_top_sel", aclk_isp_top_sel, ARRAY_SIZE(aclk_isp_top_sel), CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x0, 27, 3, 0, NULL,},
	{AX620X_ACLK_CPU_TOP_SEL, "aclk_cpu_top_sel", aclk_cpu_top_sel, ARRAY_SIZE(aclk_cpu_top_sel), CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x0, 24, 3, 0, NULL,},
	{AX620X_MCLK5_SEL, "MCLK5_sel", MCLK5_sel, ARRAY_SIZE(MCLK5_sel), CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x0, 20, 4, 0, NULL,},
	{AX620X_MCLK4_SEL, "MCLK4_sel", MCLK4_sel, ARRAY_SIZE(MCLK4_sel), CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x0, 16, 4, 0, NULL,},
	{AX620X_MCLK3_SEL, "MCLK3_sel", MCLK3_sel, ARRAY_SIZE(MCLK3_sel), CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x0, 12, 4, 0, NULL,},
	{AX620X_MCLK2_SEL, "MCLK2_sel", MCLK2_sel, ARRAY_SIZE(MCLK2_sel), CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x0, 8, 4, 0, NULL,},
	{AX620X_MCLK1_SEL, "MCLK1_sel", MCLK1_sel, ARRAY_SIZE(MCLK1_sel), CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x0, 4, 4, 0, NULL,},
	{AX620X_MCLK0_SEL, "MCLK0_sel", MCLK0_sel, ARRAY_SIZE(MCLK0_sel), CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x0, 0, 4, 0, NULL,},
	{AX620X_CLK_VI_SEL, "clk_vi_sel", clk_vi_sel, ARRAY_SIZE(clk_vi_sel), CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0xC, 28, 2, 0, NULL,},
	{AX620X_CLK_NX_VO1_COMM_SEL, "clk_nx_vo1_comm_sel", clk_nx_vo1_comm_sel, ARRAY_SIZE(clk_nx_vo1_comm_sel), CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0xC, 22, 2, 0, NULL,},
	{AX620X_CLK_NX_VO0_COMM_SEL, "clk_nx_vo0_comm_sel", clk_nx_vo0_comm_sel, ARRAY_SIZE(clk_nx_vo0_comm_sel), CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0xC, 20, 2, 0, NULL,},
	{AX620X_CLK_ISP_MM_SEL, "clk_isp_mm_sel", clk_isp_mm_sel, ARRAY_SIZE(clk_isp_mm_sel), CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0xC, 17, 3, 0, NULL,},
	{AX620X_CLK_DBC_GPIO_SEL, "clk_dbc_gpio_sel", clk_dbc_gpio_sel, ARRAY_SIZE(clk_dbc_gpio_sel), CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0xC, 16, 1, 0, NULL,},
	{AX620X_CLK_1X_VO1_COMM_SEL, "clk_1x_vo1_comm_sel", clk_1x_vo1_comm_sel, ARRAY_SIZE(clk_1x_vo1_comm_sel), CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0xC, 14, 2, 0, NULL,},
	{AX620X_CLK_1X_VO0_COMM_SEL, "clk_1x_vo0_comm_sel", clk_1x_vo0_comm_sel, ARRAY_SIZE(clk_1x_vo1_comm_sel), CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0xC, 12, 2, 0, NULL,},
	{AX620X_ACLK_VPU_TOP_SEL, "aclk_vpu_top_sel", aclk_vpu_top_sel, ARRAY_SIZE(aclk_vpu_top_sel), CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0xC, 9, 3, 0, NULL,},
	{AX620X_ACLK_OCM_TOP_SEL, "aclk_ocm_top_sel", aclk_ocm_top_sel, ARRAY_SIZE(aclk_ocm_top_sel), CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0xC, 6, 3, 0, NULL,},
	{AX620X_ACLK_NN_TOP_SEL, "aclk_nn_top_sel", aclk_nn_top_sel, ARRAY_SIZE(aclk_nn_top_sel), CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0xC, 3, 3, 0, NULL,},
	{AX620X_ACLK_MM_TOP_SEL, "aclk_mm_top_sel", aclk_mm_top_sel, ARRAY_SIZE(aclk_mm_top_sel), CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0xC, 0, 3, 0, NULL,},
	{AX620X_PCLK_TOP_SEL, "pclk_top_sel", pclk_top_sel, ARRAY_SIZE(pclk_top_sel), CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x18, 0, 2, 0, NULL,},
};

static struct axera_gate_clock ax620x_gate_clks_common[] __initdata = {
	{AX620X_CLK_VI_EB, "clk_vi_eb", "clk_vi", CLK_SET_RATE_PARENT, 0x24, 17, 1, 0, 0, 0, 0,},
	{AX620X_CLK_TMR_SYNC_EB, "clk_tmr_sync_eb", "clk_tmr_sync", CLK_SET_RATE_PARENT, 0x24, 16, 1, 0, 0, 0, 0,},
	{AX620X_CLK_NX_VO1_COMM_EB, "clk_nx_vo1_comm_eb", "clk_nx_vo1_comm_divn", CLK_SET_RATE_PARENT, 0x24, 13, 1, 0, 0, 0, 0,},
	{AX620X_CLK_NX_VO0_COMM_EB, "clk_nx_vo0_comm_eb", "clk_nx_vo0_comm_divn", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x24, 12, 1, 0, 0, 0, 0,},
	{AX620X_CLK_ISP_MM_EB, "clk_isp_mm_eb", "clk_isp_mm_sel", CLK_SET_RATE_PARENT, 0x24, 11, 1, 0, 0, 0, 0,},
	{AX620X_CLK_DPHYTX_TLB_EB, "clk_dphytx_tlb_eb", "cpll_24m", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x24, 10, 1, 0, 0, 0, 0,},
	{AX620X_CLK_DPHYRX_TLB_EB, "clk_dphyrx_tlb_eb", "cpll_24m", CLK_SET_RATE_PARENT, 0x24, 9, 1, 0, 0, 0, 0,},
	{AX620X_CLK_AUDIO_TLB_EB, "clk_audio_tlb_eb", "cpll_24m", CLK_SET_RATE_PARENT, 0x24, 8, 1, 0, 0, 0, 0,},
	{AX620X_CLK_1X_VO1_COMM_EB, "clk_1x_vo1_comm_eb", "clk_1x_vo1_comm_divn", CLK_SET_RATE_PARENT, 0x24, 7, 1, 0, 0, 0, 0,},
	{AX620X_CLK_1X_VO0_COMM_EB, "clk_1x_vo0_comm_eb", "clk_1x_vo1_comm_divn", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x24, 6, 1, 0, 0, 0, 0,},
	{AX620X_MCLK5_EB, "MCLK5_eb", "MCLK5_divn", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x24, 5, 1, 0, 0, 0, 0,},
	{AX620X_MCLK4_EB, "MCLK4_eb", "MCLK4_divn", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x24, 4, 1, 0, 0, 0, 0,},
	{AX620X_MCLK3_EB, "MCLK3_eb", "MCLK3_divn", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x24, 3, 1, 0, 0, 0, 0,},
	{AX620X_MCLK2_EB, "MCLK2_eb", "MCLK2_divn", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x24, 2, 1, 0, 0, 0, 0,},
	{AX620X_MCLK1_EB, "MCLK1_eb", "MCLK1_divn", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x24, 1, 1, 0, 0, 0, 0,},
	{AX620X_MCLK0_EB, "MCLK0_eb", "MCLK0_divn", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x24, 0, 1, 0, 0, 0, 0,},
};

static struct axera_divider_clock ax620x_div_clks_common[] __initdata = {
	{AX620X_MCLK5_DIVN, "MCLK5_divn", "MCLK5_sel", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x3C, 0x3C, 29, 25, 4, 0, NULL,},
	{AX620X_MCLK4_DIVN, "MCLK4_divn", "MCLK4_sel", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x3C, 0x3C, 24, 20, 4, 0, NULL,},
	{AX620X_MCLK3_DIVN, "MCLK3_divn", "MCLK3_sel", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x3C, 0x3C, 19, 15, 4, 0, NULL,},
	{AX620X_MCLK2_DIVN, "MCLK2_divn", "MCLK2_sel", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x3C, 0x3C, 14, 10, 4, 0, NULL,},
	{AX620X_MCLK1_DIVN, "MCLK1_divn", "MCLK1_sel", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x3C, 0x3C, 9, 5, 4, 0, NULL,},
	{AX620X_MCLK0_DIVN, "MCLK0_divn", "MCLK0_sel", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x3C, 0x3C, 4, 0, 4, 0, NULL,},
	{AX620X_CLK_NX_VO1_DIVN, "clk_nx_vo1_comm_divn", "clk_nx_vo1_comm_sel", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x48, 0x48, 19, 15, 4, 0, NULL,},
	{AX620X_CLK_NX_VO0_DIVN, "clk_nx_vo0_comm_divn", "clk_nx_vo0_comm_sel", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x48, 0x48, 14, 10, 4, 0, NULL,},
	{AX620X_CLK_1X_VO1_DIVN, "clk_1x_vo1_comm_divn", "clk_1x_vo1_comm_sel", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x48, 0x48, 9, 5, 4, 0, NULL,},
	{AX620X_CLK_1X_VO0_DIVN, "clk_1x_vo0_comm_divn", "clk_1x_vo0_comm_sel", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x48, 0x48, 4, 0, 4, 0, NULL,},
};

static void __init ax620x_clk_common_clk_init(struct device_node *np)
{
	u64 vclk;
	struct axera_clock_data *clk_data_top;
	void __iomem *vpll0_base;
	clk_data_top = axera_clk_init(np, AX620X_COMMON_NR_CLKS);
	if (!clk_data_top)
		return;

	axera_clk_register_fixed_rate(AX620X_fixed_rate_clks, ARRAY_SIZE(AX620X_fixed_rate_clks), clk_data_top);
	axera_clk_register_fixed_factor(AX620X_fixed_factor_clks, ARRAY_SIZE(AX620X_fixed_factor_clks), clk_data_top);

	axera_clk_register_mux(ax620x_mux_clks_common, ARRAY_SIZE(ax620x_mux_clks_common), clk_data_top);
	axera_clk_register_gate(ax620x_gate_clks_common, ARRAY_SIZE(ax620x_gate_clks_common), clk_data_top);
	axera_clk_register_divider(ax620x_div_clks_common, ARRAY_SIZE(ax620x_div_clks_common), clk_data_top);

	vpll0_base = ioremap(AX_VPLL0_BASE, SZ_4K);
	if (vpll0_base == NULL) {
		printk("AX_VPLL0_BASE MAP ERROR\n");
	}

	if (vpll0_base != NULL && ((readl(vpll0_base) & AX_VPLL0_MASK) == AX_VPLL0_TAG)) {
		vclk = AX_VPLL0_VAL*AX_VPLL0_TAG*AX_DEC_SZ_1M;
		AX620X_fixed_rate_clks[5].fixed_rate = vclk;
	}

	if (vpll0_base != NULL && ((readl(vpll0_base) & AX_VPLL0_MASK) == AX_VPLL0_TAG_1)) {
		vclk = AX_VPLL0_VAL*AX_VPLL0_TAG_1*AX_DEC_SZ_1M;
		AX620X_fixed_rate_clks[5].fixed_rate = vclk;
	}

	if (vpll0_base != NULL)
		iounmap(vpll0_base);
}

CLK_OF_DECLARE(ax620x_clk_common_glb, "axera,ax620x-common-clk", ax620x_clk_common_clk_init);

/*dispc*/
static const char *clk_dphy_tx_esc_sel[] __initdata = { "epll_10m", "epll_20m" };
static const char *clk_dispc_glb_sel[] __initdata = { "cpll_24m", "epll_100m", "cpll_208m", "cpll_312m", "cpll_416m" };

static struct axera_mux_clock ax620x_mux_clks_dispc[] __initdata = {
	{AX620X_CLK_DPHY_TX_ESC_SEL, "clk_dphy_tx_esc_sel", clk_dphy_tx_esc_sel, ARRAY_SIZE(clk_dphy_tx_esc_sel), CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x0, 3, 1, 0, NULL,},
	{AX620X_CLK_DISPC_GLB_SEL, "clk_dispc_glb_sel", clk_dispc_glb_sel, ARRAY_SIZE(clk_dispc_glb_sel), CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x0, 0, 3, 0, NULL,},
};

static struct axera_gate_clock ax620x_gate_clks_dispc[] __initdata = {
	{AX620X_CLK_DPHY_TX_REF_EB, "clk_dphy_tx_ref_eb", "cpll_12m", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x4, 1, 1, 0, 0, 0, 0,},
	{AX620X_CLK_DPHY_TX_ESC_EB, "clk_dphy_tx_esc_eb", "clk_dphy_tx_esc_sel", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x4, 0, 1, 0, 0, 0, 0,},
	{AX620X_PCLK_LVDS_TX_EB, "pclk_lvds_tx_eb", "clk_dispc_glb_sel", CLK_SET_RATE_PARENT, 0x8, 9, 1, 0, 0, 0, 0,},
	{AX620X_PCLK_DSI_EB, "pclk_dsi_eb", "clk_dispc_glb_sel", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x8, 8, 1, 0, 0, 0, 0,},
	{AX620X_PCLK_CSI_EB, "pclk_csi_eb", "clk_dispc_glb_sel", CLK_SET_RATE_PARENT, 0x8, 7, 1, 0, 0, 0, 0,},
	{AX620X_CLK_DSI_TX_ESC_EB, "clk_dsi_tx_esc_eb", "clk_dphy_tx_esc_sel", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x8, 6, 1, 0, 0, 0, 0,},
	{AX620X_CLK_DSI_SYS_EB, "clk_dsi_sys_eb", "clk_dispc_glb_sel", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x8, 5, 1, 0, 0, 0, 0,},
	{AX620X_CLK_DPHY_TX_PLL_DIV7_CG_EB, "clk_dphy_tx_pll_div7_cg_eb", "need_modify", CLK_SET_RATE_PARENT, 0x8, 4, 1, 0, 0, 0, 0,},
	{AX620X_CLK_DPHY_TX_PLL_CG_EB, "clk_dphy_tx_pll_cg_eb", "need_modify", CLK_SET_RATE_PARENT, 0x8, 3, 1, 0, 0, 0, 0,},
	{AX620X_CLK_DPHY2DSI_HS_EB, "clk_dphy2dsi_hs_eb", "need_modify", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x8, 2, 1, 0, 0, 0, 0,},
	{AX620X_CLK_DPHY2CSI_HS_EB, "clk_dphy2csi_hs_eb", "need_modify", CLK_SET_RATE_PARENT, 0x8, 1, 1, 0, 0, 0, 0,},
	{AX620X_CLK_CSI_TX_ESC_EB, "clk_csi_tx_esc_eb", "need_modify", CLK_SET_RATE_PARENT, 0x8, 0, 1, 0, 0, 0, 0,},
};

static void __init ax620x_clk_dispc_clk_init(struct device_node *np)
{
	struct axera_clock_data *clk_data_top;
	clk_data_top = axera_clk_init(np, AX620X_DISPC_NR_CLKS);
	if (!clk_data_top)
		return;

	axera_clk_register_mux(ax620x_mux_clks_dispc, ARRAY_SIZE(ax620x_mux_clks_dispc), clk_data_top);
	axera_clk_register_gate(ax620x_gate_clks_dispc, ARRAY_SIZE(ax620x_gate_clks_dispc), clk_data_top);
}

CLK_OF_DECLARE(ax620x_clk_dispc_glb, "axera,ax620x-dispc-clk", ax620x_clk_dispc_clk_init);
/*flash*/
static const char *rgmii_ephy_clk_sel[] __initdata = { "epll_25m", "epll_50m", "epll_125m" };
static const char *clk_nx_vo1_sel[] __initdata = { "vpll1_108m", "vpll1_118p8m", "vpll1_198m", "vpll1_297m" };
static const char *clk_nx_vo0_sel[] __initdata = { "vpll0_108m", "vpll0_118p8m", "vpll0_198m", "vpll0_297m" };
static const char *clk_flash_glb_sel[] __initdata = { "cpll_24m", "epll_100m", "cpll_156m", "cpll_208m", "epll_250m", "cpll_312m" };
static const char *clk_emac_rgmii_tx_sel[] __initdata = { "epll_5m", "epll_50m", "epll_250m" };
static const char *clk_1x_vo1_sel[] __initdata = { "vpll1_108m", "vpll1_118p8m", "vpll1_198m", "vpll1_297m" };
static const char *clk_1x_vo0_sel[] __initdata = { "vpll0_108m", "vpll0_118p8m", "vpll0_198m", "vpll0_297m" };

static struct axera_mux_clock ax620x_mux_clks_flash[] __initdata = {
	{AX620X_RGMII_EPHY_CLK_SEL, "rgmii_ephy_clk_sel", rgmii_ephy_clk_sel, ARRAY_SIZE(rgmii_ephy_clk_sel), CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x0, 22, 2, 0, NULL,},
	{AX620X_CLK_NX_VO1_SEL, "clk_nx_vo1_sel", clk_nx_vo1_sel, ARRAY_SIZE(clk_nx_vo1_sel), CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x0, 14, 2, 0, NULL,},
	{AX620X_CLK_NX_VO0_SEL, "clk_nx_vo0_sel", clk_nx_vo0_sel, ARRAY_SIZE(clk_nx_vo0_sel), CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x0, 12, 2, 0, NULL,},
	{AX620X_CLK_FLASH_GLB_SEL, "clk_flash_glb_sel", clk_flash_glb_sel, ARRAY_SIZE(clk_flash_glb_sel), CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x0, 6, 3, 0, NULL,},
	{AX620X_CLK_EMAC_RGMII_TX_SEL, "clk_emac_rgmii_tx_sel", clk_emac_rgmii_tx_sel, ARRAY_SIZE(clk_emac_rgmii_tx_sel), CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x0, 4, 2, 0,
	 NULL,},
	{AX620X_CLK_1X_VO1_SEL, "clk_1x_vo1_sel", clk_1x_vo1_sel, ARRAY_SIZE(clk_1x_vo1_sel), CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x0, 2, 2, 0, NULL,},
	{AX620X_CLK_1X_VO0_SEL, "clk_1x_vo0_sel", clk_1x_vo0_sel, ARRAY_SIZE(clk_1x_vo0_sel), CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x0, 0, 2, 0, NULL,},
};

static struct axera_gate_clock ax620x_gate_clks_flash[] __initdata = {
	{AX620X_EPHY_CLK_EB, "ephy_clk_eb", "rgmii_ephy_clk_sel", CLK_SET_RATE_PARENT, 0x4, 13, 1, 0, 0, 0, 0,},
	{AX620X_CLK_NX_VO1_EB, "clk_nx_vo1_eb", "clk_nx_vo1_divn_flash", CLK_SET_RATE_PARENT, 0x4, 8, 1, 0, 0, 0, 0,},
	{AX620X_CLK_NX_VO0_EB, "clk_nx_vo0_eb", "clk_nx_vo0_divn_flash", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x4, 7, 1, 0, 0, 0, 0,},
	{AX620X_CLK_EMAC_RMII_PHY_EB, "clk_emac_rmii_phy_eb", "epll_50m", CLK_SET_RATE_PARENT, 0x4, 4, 1, 0, 0, 0, 0,},
	{AX620X_CLK_EMAC_RGMII_TX_EB, "clk_emac_rgmii_tx_eb", "clk_emac_rgmii_tx_sel", CLK_SET_RATE_PARENT, 0x4, 3, 1, 0, 0, 0, 0,},
	{AX620X_CLK_EMAC_PTP_REF_EB, "clk_emac_ptp_ref_eb", "epll_50m", CLK_SET_RATE_PARENT, 0x4, 2, 1, 0, 0, 0, 0,},
	{AX620X_CLK_1X_VO1_EB, "clk_1x_vo1_eb", "clk_1x_vo1_divn_flash", CLK_SET_RATE_PARENT, 0x4, 1, 1, 0, 0, 0, 0,},
	{AX620X_CLK_1X_VO0_EB, "clk_1x_vo0_eb", "clk_1x_vo0_divn_flash", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x4, 0, 1, 0, 0, 0, 0,},
	{AX620X_CLK_LPC_FLASH_EB, "clk_lpc_flash_eb", "cpll_24m", CLK_SET_RATE_PARENT, 0x8, 11, 1, 0, 0, 0, 0,},
	{AX620X_ACLK_EMAC_EB, "aclk_emac_eb", "clk_flash_glb_sel", CLK_SET_RATE_PARENT, 0x8, 2, 1, 0, 0, 0, 0,},
};

static struct axera_divider_clock ax620x_div_clks_flash[] __initdata = {
	{AX620X_CLK_NX_VO1_DIVN_FLASH, "clk_nx_vo1_divn_flash", "clk_nx_vo1_sel", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0xC, 0xC, 19, 15, 4, 0, NULL,},
	{AX620X_CLK_NX_VO0_DIVN_FLASH, "clk_nx_vo0_divn_flash", "clk_nx_vo0_sel", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0xC, 0xC, 14, 10, 4, 0, NULL,},
	{AX620X_CLK_1X_VO1_DIVN_FLASH, "clk_1x_vo1_divn_flash", "clk_1x_vo1_sel", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0xC, 0xC, 9, 5, 4, 0, NULL,},
	{AX620X_CLK_1X_VO0_DIVN_FLASH, "clk_1x_vo0_divn_flash", "clk_1x_vo0_sel", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0xC, 0xC, 4, 0, 4, 0, NULL,},
};

static void __init ax620x_clk_flash_clk_init(struct device_node *np)
{
	struct axera_clock_data *clk_data_top;
	clk_data_top = axera_clk_init(np, AX620X_FLASH_NR_CLKS);
	if (!clk_data_top)
		return;

	axera_clk_register_mux(ax620x_mux_clks_flash, ARRAY_SIZE(ax620x_mux_clks_flash), clk_data_top);
	axera_clk_register_gate(ax620x_gate_clks_flash, ARRAY_SIZE(ax620x_gate_clks_flash), clk_data_top);
	axera_clk_register_divider(ax620x_div_clks_flash, ARRAY_SIZE(ax620x_div_clks_flash), clk_data_top);
}

CLK_OF_DECLARE(ax620x_clk_flash_glb, "axera,ax620x-flash-clk", ax620x_clk_flash_clk_init);

/*mm*/
static const char *clk_vpp_src_sel[] __initdata = { "cpll_24m", "cpll_208m", "cpll_312m", "cpll_416m", "epll_500m", "npll_533m" };
static const char *clk_tdp_src_sel[] __initdata = { "cpll_24m", "cpll_208m", "cpll_312m", "cpll_416m", "epll_500m", "npll_533m" };
static const char *clk_mm_glb_sel[] __initdata = { "cpll_24m", "cpll_208m", "cpll_312m", "cpll_416m", "epll_500m", "npll_533m" };
static const char *clk_ive_src_sel[] __initdata = { "cpll_24m", "cpll_208m", "cpll_312m", "cpll_416m", "epll_500m", "npll_533m" };
static const char *clk_gdc_src_sel[] __initdata = { "cpll_24m", "cpll_208m", "cpll_312m", "cpll_416m", "epll_500m", "npll_533m" };
static const char *clk_dpu_src_sel[] __initdata = { "cpll_24m", "cpll_208m", "cpll_312m", "cpll_416m", "epll_500m", "npll_533m" };
static const char *clk_dpu_out_sel[] __initdata = { "vpll0_108m", "vpll0_118p8m", "vpll0_198m", "vpll0_297m" };
static const char *clk_dpu_lite_src_sel[] __initdata = { "cpll_24m", "cpll_208m", "cpll_312m", "cpll_416m", "epll_500m", "npll_533m" };
static const char *clk_dpu_lite_out_sel[] __initdata = { "vpll1_108m", "vpll1_118p8m", "vpll1_198m", "vpll1_297m" };
static const char *clk_csi_async_sel[] __initdata = { "cpll_312m", "cpll_416m", "epll_500m", "npll_533m" };
static const char *clk_axi2csi_src_sel[] __initdata = { "cpll_24m", "cpll_208m", "cpll_312m", "cpll_416m", "epll_500m", "npll_533m" };

static struct axera_mux_clock ax620x_mux_clks_mm[] __initdata = {
	{AX620X_CLK_VPP_SRC_SEL, "clk_vpp_src_sel", clk_vpp_src_sel, ARRAY_SIZE(clk_vpp_src_sel), CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x0, 27, 3, 0, NULL,},
	{AX620X_CLK_TDP_SRC_SEL, "clk_tdp_src_sel", clk_tdp_src_sel, ARRAY_SIZE(clk_tdp_src_sel), CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x0, 24, 3, 0, NULL,},
	{AX620X_CLK_MM_GLB_SEL, "clk_mm_glb_sel", clk_mm_glb_sel, ARRAY_SIZE(clk_mm_glb_sel), CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x0, 21, 3, 0, NULL,},
	{AX620X_CLK_IVE_SRC_SEL, "clk_ive_src_sel", clk_ive_src_sel, ARRAY_SIZE(clk_ive_src_sel), CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x0, 18, 3, 0, NULL,},
	{AX620X_CLK_GDC_SRC_SEL, "clk_gdc_src_sel", clk_gdc_src_sel, ARRAY_SIZE(clk_gdc_src_sel), CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x0, 15, 3, 0, NULL,},
	{AX620X_CLK_DPU_SRC_SEL, "clk_dpu_src_sel", clk_dpu_src_sel, ARRAY_SIZE(clk_dpu_src_sel), CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x0, 12, 3, 0, NULL,},
	{AX620X_CLK_DPU_OUT_SEL, "clk_dpu_out_sel", clk_dpu_out_sel, ARRAY_SIZE(clk_dpu_out_sel), CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x0, 10, 2, 0, NULL,},
	{AX620X_CLK_DPU_LITE_SRC_SEL, "clk_dpu_lite_src_sel", clk_dpu_lite_src_sel, ARRAY_SIZE(clk_dpu_lite_src_sel), CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x0, 7, 3, 0, NULL,},
	{AX620X_CLK_DPU_LITE_OUT_SEL, "clk_dpu_lite_out_sel", clk_dpu_lite_out_sel, ARRAY_SIZE(clk_dpu_lite_out_sel), CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x0, 5, 2, 0, NULL,},
	{AX620X_CLK_CSI_ASYNC_SEL, "clk_csi_async_sel", clk_csi_async_sel, ARRAY_SIZE(clk_csi_async_sel), CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x0, 3, 2, 0, NULL,},
	{AX620X_CLK_AXI2CSI_SRC_SEL, "clk_axi2csi_src_sel", clk_axi2csi_src_sel, ARRAY_SIZE(clk_axi2csi_src_sel), CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x0, 0, 3, 0, NULL,},
};

static struct axera_gate_clock ax620x_gate_clks_mm[] __initdata = {
	{AX620X_CLK_DPU_OUT_EB, "clk_dpu_out_eb", "clk_dpu_out_divn", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x4, 2, 1, 0, 0, 0, 0,},
	{AX620X_CLK_DPU_LITE_OUT_EB, "clk_dpu_lite_out_eb", "clk_dpu_lite_out_divn", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x4, 1, 1, 0, 0, 0, 0,},
	{AX620X_CLK_CSI_ASYNC_EB, "clk_csi_async_eb", "clk_csi_async_sel", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x4, 0, 1, 0, 0, 0, 0,},
	{AX620X_PCLK_VPP_EB, "pclk_vpp_eb", "clk_vpp_src_sel", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x8, 25, 1, 0, 0, 0, 0,},
	{AX620X_PCLK_TDP_EB, "pclk_tdp_eb", "clk_tdp_src_sel", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x8, 24, 1, 0, 0, 0, 0,},
	{AX620X_PCLK_IVE_EB, "pclk_ive_eb", "clk_ive_src_sel", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x8, 23, 1, 0, 0, 0, 0,},
	{AX620X_PCLK_GDC_EB, "pclk_gdc_eb", "clk_gdc_src_sel", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x8, 22, 1, 0, 0, 0, 0,},
	{AX620X_PCLK_DPU_LITE_EB, "pclk_dpu_lite_eb", "clk_dpu_lite_src_sel", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x8, 21, 1, 0, 0, 0, 0,},
	{AX620X_PCLK_DPU_EB, "pclk_dpu_eb", "clk_dpu_src_sel", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x8, 20, 1, 0, 0, 0, 0,},
	{AX620X_PCLK_CMD_EB, "pclk_cmd_eb", "clk_mm_glb_sel", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x8, 19, 1, 0, 0, 0, 0,},
	{AX620X_PCLK_AXI2CSI_EB, "pclk_axi2csi_eb", "clk_axi2csi_src_sel", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x8, 18, 1, 0, 0, 0, 0,},
	{AX620X_CLK_VPP_SCL4_EB, "clk_vpp_scl4_eb", "clk_vpp_src_sel", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x8, 17, 1, 0, 0, 0, 0,},
	{AX620X_CLK_VPP_SCL3_EB, "clk_vpp_scl3_eb", "clk_vpp_src_sel", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x8, 16, 1, 0, 0, 0, 0,},
	{AX620X_CLK_VPP_SCL2_EB, "clk_vpp_scl2_eb", "clk_vpp_src_sel", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x8, 15, 1, 0, 0, 0, 0,},
	{AX620X_CLK_VPP_SCL1_EB, "clk_vpp_scl1_eb", "clk_vpp_src_sel", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x8, 14, 1, 0, 0, 0, 0,},
	{AX620X_CLK_VPP_SCL0_EB, "clk_vpp_scl0_eb", "clk_vpp_src_sel", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x8, 13, 1, 0, 0, 0, 0,},
	{AX620X_CLK_VPP_EB, "clk_vpp_eb", "clk_vpp_src_sel", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x8, 12, 1, 0, 0, 0, 0,},
	{AX620X_CLK_TDP_EB, "clk_tdp_eb", "clk_tdp_src_sel", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x8, 11, 1, 0, 0, 0, 0,},
	{AX620X_CLK_LPC_MM_EB, "clk_lpc_mm_eb", "cpll_24m", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x8, 10, 1, 0, 0, 0, 0,},
	{AX620X_CLK_IVE_EB, "clk_ive_eb", "clk_ive_src_sel", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x8, 9, 1, 0, 0, 0, 0,},
	{AX620X_CLK_GDC_EB, "clk_gdc_eb", "clk_gdc_src_sel", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x8, 8, 1, 0, 0, 0, 0,},
	{AX620X_CLK_FBCD_EB, "clk_fbcd_eb", "clk_mm_glb_sel", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x8, 7, 1, 0, 0, 0, 0,},
	{AX620X_CLK_FBC_EB, "clk_fbc_eb", "clk_mm_glb_sel", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x8, 6, 1, 0, 0, 0, 0,},
	{AX620X_CLK_DPU_LITE_EB, "clk_dpu_lite_eb", "clk_dpu_lite_src_sel", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x8, 5, 1, 0, 0, 0, 0,},
	{AX620X_CLK_DPU_EB, "clk_dpu_eb", "clk_dpu_src_sel", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x8, 4, 1, 0, 0, 0, 0,},
	{AX620X_CLK_CMD_EB, "clk_cmd_eb", "clk_mm_glb_sel", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x8, 3, 1, 0, 0, 0, 0,},
	{AX620X_CLK_AXI2CSI_EB, "clk_axi2csi_eb", "clk_axi2csi_src_sel", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x8, 0, 1, 0, 0, 0, 0,},
};

static struct axera_divider_clock ax620x_div_clks_mm[] __initdata = {
	{AX620X_CLK_DPU_OUT_DIVN, "clk_dpu_out_divn", "clk_dpu_out_sel", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0xC, 0xC, 9, 5, 4, 0, NULL,},
	{AX620X_CLK_DPU_LITE_OUT_DIVN, "clk_dpu_lite_out_divn", "clk_dpu_lite_out_sel", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0xC, 0xC, 4, 0, 4, 0, NULL,},
};

static void __init ax620x_clk_mm_clk_init(struct device_node *np)
{
	struct axera_clock_data *clk_data_top;
	clk_data_top = axera_clk_init(np, AX620X_MM_NR_CLKS);
	if (!clk_data_top)
		return;

	axera_clk_register_mux(ax620x_mux_clks_mm, ARRAY_SIZE(ax620x_mux_clks_mm), clk_data_top);
	axera_clk_register_gate(ax620x_gate_clks_mm, ARRAY_SIZE(ax620x_gate_clks_mm), clk_data_top);
	axera_clk_register_divider(ax620x_div_clks_mm, ARRAY_SIZE(ax620x_div_clks_mm), clk_data_top);
}

CLK_OF_DECLARE(ax620x_clk_mm_glb, "axera,ax620x-mm-clk", ax620x_clk_mm_clk_init);

/*periph*/
static const char *sclk_i2s_tdm_sel[] __initdata = { "hpll_16p384m", "hpll_20p48m", "hpll_24p576m" };
static const char *sclk_i2s_m_sel[] __initdata = { "hpll_16p384m", "hpll_20p48m", "hpll_24p576m" };
static const char *clk_timer_sel[] __initdata = { "rtc_out_32k", "cpll_24m" };
static const char *clk_i2s_ref0_sel[] __initdata = { "cpll_12m", "hpll_16p384m", "hpll_24p576m", "epll_25m" };

static struct axera_mux_clock ax620x_mux_clks_periph[] __initdata = {
	{AX620X_SCLK_I2S_TDM_SEL, "sclk_i2s_tdm_sel", sclk_i2s_tdm_sel, ARRAY_SIZE(sclk_i2s_tdm_sel), CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x0, 23, 2, 0, NULL,},
	{AX620X_SCLK_I2S_M_SEL, "sclk_i2s_m_sel", sclk_i2s_m_sel, ARRAY_SIZE(sclk_i2s_m_sel), CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x0, 21, 2, 0, NULL,},
	{AX620X_CLK_TIMER_SEL, "clk_timer_sel", clk_timer_sel, ARRAY_SIZE(clk_timer_sel), CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x0, 13, 1, 0, NULL,},
	{AX620X_CLK_I2S_REF0_SEL, "clk_i2s_ref0_sel", clk_i2s_ref0_sel, ARRAY_SIZE(clk_i2s_ref0_sel), CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x0, 5, 2, 0, NULL,},
};

static struct axera_gate_clock ax620x_gate_clks_periph[] __initdata = {
	{AX620X_SCLK_I2S_TDM_EB, "sclk_i2s_tdm_eb", "sclk_i2s_tdm_divn", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x4, 17, 1, 0, 0, 0, 0,},
	{AX620X_SCLK_I2S_M_EB, "sclk_i2s_m_eb", "sclk_i2s_m_divn", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x4, 16, 1, 0, 0, 0, 0,},
	{AX620X_CLK_TIMER_EB, "clk_timer_eb", "clk_timer_sel", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x4, 9, 1, 0, 0, 0, 0,},
	{AX620X_CLK_I2S_REF0_EB, "clk_i2s_ref0_eb", "clk_i2s_ref0_divn", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x4, 4, 1, 0, 0, 0, 0,},
	{AX620X_CLK_I2S_AUDIO_REF_EB, "clk_i2s_audio_ref_eb", "hpll_12p288m", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x4, 3, 1, 0, 0, 0, 0,},
	{AX620X_CLK_TIMER0_EB, "clk_timer0_eb", "clk_timer_sel", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x8, 31, 1, 0, 0, 0, 0,},
	{AX620X_CLK_LPC_PERI_EB, "clk_lpc_peri_eb", "cpll_24m", CLK_SET_RATE_PARENT, 0x8, 18, 1, 0, 0, 0, 0,},
	{AX620X_ACLK_AX_DMA_PER_EB, "aclk_ax_dma_per_eb", "pclk_top_sel", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x8, 0, 1, 0, 0, 0, 0,},
	{AX620X_PCLK_I2S_TDM_S_EB, "pclk_i2s_tdm_s_eb", "pclk_top_sel", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0xC, 30, 1, 0, 0, 0, 0,},
	{AX620X_PCLK_I2S_TDM_M_EB, "pclk_i2s_tdm_m_eb", "pclk_top_sel", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0xC, 29, 1, 0, 0, 0, 0,},
	{AX620X_PCLK_I2S_S_EB, "pclk_i2s_s_eb", "pclk_top_sel", CLK_SET_RATE_PARENT, 0xC, 28, 1, 0, 0, 0, 0,},
	{AX620X_PCLK_I2S_M_EB, "pclk_i2s_m_eb", "pclk_top_sel", CLK_SET_RATE_PARENT , 0xC, 27, 1, 0, 0, 0, 0,},
	{AX620X_PCLK_AX_DMA_PER_EB, "pclk_ax_dma_per_eb", "pclk_top_sel", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0xC, 11, 1, 0, 0, 0, 0,},
	{AX620X_PCLK_TIMER0_EB, "pclk_timer0_eb", "pclk_top_sel", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x10, 5, 1, 0, 0, 0, 0,},
};

static struct axera_divider_clock ax620x_div_clks_periph[] __initdata = {
	{AX620X_SCLK_I2S_TDM_DIVN, "sclk_i2s_tdm_divn", "sclk_i2s_tdm_sel", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x14, 0x14, 20, 14, 6, 0, NULL,},
	{AX620X_SCLK_I2S_M_DIVN, "sclk_i2s_m_divn", "sclk_i2s_m_sel", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x14, 0x14, 13, 7, 6, 0, NULL,},
	{AX620X_CLK_I2S_REF0_DIVN, "clk_i2s_ref0_divn", "clk_i2s_ref0_sel", CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x14, 0x14, 6, 0, 6, 0, NULL,},
};

static void __init ax620x_clk_periph_clk_init(struct device_node *np)
{
	struct axera_clock_data *clk_data_top;
	clk_data_top = axera_clk_init(np, AX620X_PERIPH_NR_CLKS);
	if (!clk_data_top)
		return;

	axera_clk_register_mux(ax620x_mux_clks_periph, ARRAY_SIZE(ax620x_mux_clks_periph), clk_data_top);
	axera_clk_register_gate(ax620x_gate_clks_periph, ARRAY_SIZE(ax620x_gate_clks_periph), clk_data_top);
	axera_clk_register_divider(ax620x_div_clks_periph, ARRAY_SIZE(ax620x_div_clks_periph), clk_data_top);
}

CLK_OF_DECLARE(ax620x_clk_periph_glb, "axera,ax620x-periph-clk", ax620x_clk_periph_clk_init);

/*vpu*/
static const char *clk_vpu_glb_sel[] __initdata = { "cpll_208m", "cpll_312m", "epll_375m", "cpll_416m", "epll_500m", "npll_533m" };
static const char *clk_vdec_src_sel[] __initdata = { "cpll_208m", "cpll_312m", "epll_375m", "cpll_416m", "epll_500m", "npll_533m" };
static const char *clk_jenc_src_sel[] __initdata = { "cpll_208m", "cpll_312m", "epll_375m", "cpll_416m", "epll_500m", "npll_533m" };

static struct axera_mux_clock ax620x_mux_clks_vpu[] __initdata = {
	{AX620X_CLK_VPU_GLB_SEL, "clk_vpu_glb_sel", clk_vpu_glb_sel, ARRAY_SIZE(clk_vpu_glb_sel), CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x0, 6, 3, 0, NULL,},
	{AX620X_CLK_VDEC_SRC_SEL, "clk_vdec_src_sel", clk_vdec_src_sel, ARRAY_SIZE(clk_vdec_src_sel), CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x0, 3, 3, 0, NULL,},
	{AX620X_CLK_JENC_SRC_SEL, "clk_jenc_src_sel", clk_jenc_src_sel, ARRAY_SIZE(clk_jenc_src_sel), CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x0, 0, 3, 0, NULL,},
};

static struct axera_gate_clock ax620x_gate_clks_vpu[] __initdata = {
	{AX620X_CLK_VENC_EB, "clk_venc_eb", "clk_vpu_glb_sel", CLK_SET_RATE_PARENT, 0x8, 7, 1, 0, 0, 0, 0,},
	{AX620X_CLK_VDEC_EB, "clk_vdec_eb", "clk_vdec_src_sel", CLK_SET_RATE_PARENT, 0x8, 6, 1, 0, 0, 0, 0,},
	{AX620X_CLK_LPC_VPU_EB, "clk_lpc_vpu_eb", "cpll_24m", CLK_SET_RATE_PARENT, 0x8, 5, 1, 0, 0, 0, 0,},
	{AX620X_CLK_JENC_EB, "clk_jenc_eb", "clk_jenc_src_sel", CLK_SET_RATE_PARENT, 0x8, 4, 1, 0, 0, 0, 0,},
};

static void __init ax620x_clk_vpu_clk_init(struct device_node *np)
{
	struct axera_clock_data *clk_data_top;
	clk_data_top = axera_clk_init(np, AX620X_VPU_NR_CLKS);
	if (!clk_data_top)
		return;

	axera_clk_register_mux(ax620x_mux_clks_vpu, ARRAY_SIZE(ax620x_mux_clks_vpu), clk_data_top);
	axera_clk_register_gate(ax620x_gate_clks_vpu, ARRAY_SIZE(ax620x_gate_clks_vpu), clk_data_top);
}

CLK_OF_DECLARE(ax620x_clk_vpu_glb, "axera,ax620x-vpu-clk", ax620x_clk_vpu_clk_init);
