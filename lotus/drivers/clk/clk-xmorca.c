// SPDX-License-Identifier: GPL-2.0

#include <dt-bindings/clock/xmorca-clock.h>
#include <linux/clk-provider.h>
#include <linux/module.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include "clk.h"
#include "reset.h"

static struct lotus_fixed_rate_clock xmorca_fixed_rate_clks[] __initdata = {
	{ XMORCA_FIXED_400K,		"400k",		NULL, 0, 400000, },
	{ XMORCA_FIXED_1600K,		"1600k",	NULL, 0, 1600000, },
	{ XMORCA_FIXED_3M,		"3m",		NULL, 0, 3000000, },
	{ XMORCA_FIXED_24M,		"24m",		NULL, 0, 24000000, },
	{ XMORCA_FIXED_25M,		"25m",		NULL, 0, 25000000, },
	{ XMORCA_FIXED_50M,		"50m",		NULL, 0, 50000000, },
	{ XMORCA_FIXED_74M,		"74.25m",	NULL, 0, 74250000, },
	{ XMORCA_FIXED_90M,		"90m",		NULL, 0, 90000000, },
	{ XMORCA_FIXED_99M,		"99m",		NULL, 0, 99000000, },
	{ XMORCA_FIXED_100M,		"100m",		NULL, 0, 100000000, },
	{ XMORCA_FIXED_112M,		"112m",		NULL, 0, 112000000, },
	{ XMORCA_FIXED_148P5M,	"148.5m",	NULL, 0, 148500000, },
	{ XMORCA_FIXED_150M,		"150m",		NULL, 0, 150000000, },
	{ XMORCA_FIXED_198M,		"198m",		NULL, 0, 198000000, },
	{ XMORCA_FIXED_200M,		"200m",		NULL, 0, 200000000, },
	{ XMORCA_FIXED_300M,		"300m",		NULL, 0, 300000000, },
	{ XMORCA_FIXED_396M,		"396m",		NULL, 0, 396000000, },
	{ XMORCA_FIXED_458M,		"458m",		NULL, 0, 458000000, },
	{ XMORCA_FIXED_594M,		"594m",		NULL, 0, 594000000, },
	{ XMORCA_FIXED_792M,		"792m",		NULL, 0, 792000000, },
	{ XMORCA_FIXED_917M,		"917m",		NULL, 0, 917000000, },
	{ XMORCA_FIXED_1188M,		"1188m",	NULL, 0, 1188000000UL, },
	{ XMORCA_FIXED_1375M,		"1375m",	NULL, 0, 1375000000UL, },
	{ XMORCA_FIXED_1500M,		"1500m",	NULL, 0, 1500000000UL, },
};

/* Sys APB mux  */
static const char *sysapb_mux_p[] __initconst = {"24m", "50m"};
static u32 sysapb_mux_table[] = {0, 1};

/* CPU mux */
static const char *cpu_mux_p[] __initconst = {"396m", "594m", "792m", "917m", "1188m", "1375m", "1500m"};
static u32 cpu_mux_table[] = {7, 6, 5, 4, 3, 2, 1};

/* UART mux */
static const char *uart_mux_p[] __initconst = {"24m", "3m", "74.25m"};
static u32 uart_mux_table[] = {0, 1, 2};

/* FMC mux */
static const char *fmc_mux_p[] __initconst = {"24m", "99m", "148.5m", "198m", "297m", "396m"};
static u32 fmc_mux_table[] = {0, 5, 4, 3, 2, 1};

/* MMC mux */
static const char *mmc_mux_p[] __initconst = {
	"100k", "400k", "25m", "50m", "90m"
};
static u32 mmc_mux_table[] = {0, 1, 2, 3, 4};

/* SSP mux */
static const char *ssp_mux_p[] __initconst = {"594m", "396m"};
static u32 ssp_mux_table[] = {0, 1};

static struct lotus_mux_clock xmorca_mux_clks[] __initdata = {
	{
		XMORCA_SYSAPB_CLK, "sysapb_mux", sysapb_mux_p,
		ARRAY_SIZE(sysapb_mux_p),
		CLK_SET_RATE_PARENT, 0x80, 16, 1, 0, sysapb_mux_table,
	},
	{
		XMORCA_CPU_MUX, "cpu_mux", cpu_mux_p, ARRAY_SIZE(cpu_mux_p),
		CLK_SET_RATE_PARENT, 0x80, 0, 3, 0, cpu_mux_table,
	},
	{
		XMORCA_FMC_MUX, "fmc_mux", fmc_mux_p, ARRAY_SIZE(fmc_mux_p),
		CLK_SET_RATE_PARENT, 0x144, 2, 3, 0, fmc_mux_table,
	},
	{
		XMORCA_MMC0_MUX, "mmc0_mux", mmc_mux_p, ARRAY_SIZE(mmc_mux_p),
		CLK_SET_RATE_PARENT, 0x1f4, 24, 3, 0, mmc_mux_table,
	},
	{
		XMORCA_MMC1_MUX, "mmc1_mux", mmc_mux_p, ARRAY_SIZE(mmc_mux_p),
		CLK_SET_RATE_PARENT, 0x22c, 24, 3, 0, mmc_mux_table,
	},
	{
		XMORCA_MMC2_MUX, "mmc2_mux", mmc_mux_p, ARRAY_SIZE(mmc_mux_p),
		CLK_SET_RATE_PARENT, 0x240, 24, 3, 0, mmc_mux_table,
	},
	{
		XMORCA_UART0_MUX, "uart_mux0", uart_mux_p,
		ARRAY_SIZE(uart_mux_p),
		CLK_SET_RATE_PARENT, 0x1bc, 20, 2, 0, uart_mux_table,
	},
	{
		XMORCA_UART1_MUX, "uart_mux1", uart_mux_p,
		ARRAY_SIZE(uart_mux_p),
		CLK_SET_RATE_PARENT, 0x1bc, 22, 2, 0, uart_mux_table,
	},
	{
		XMORCA_UART2_MUX, "uart_mux2", uart_mux_p,
		ARRAY_SIZE(uart_mux_p),
		CLK_SET_RATE_PARENT, 0x1bc, 24, 2, 0, uart_mux_table,
	},
	{
		XMORCA_UART3_MUX, "uart_mux3", uart_mux_p,
		ARRAY_SIZE(uart_mux_p),
		CLK_SET_RATE_PARENT, 0x1bc, 26, 2, 0, uart_mux_table,
	},
	{
		XMORCA_SSP_MUX, "ssp_mux", ssp_mux_p,
		ARRAY_SIZE(ssp_mux_p),
		CLK_SET_RATE_PARENT, 0x1bc, 15, 1, 0, ssp_mux_table,
	},
};

static struct lotus_fixed_factor_clock xmorca_fixed_factor_clks[] __initdata = {
};

static struct lotus_gate_clock xmorca_gate_clks[] __initdata = {
	/* fmc */
	{
		XMORCA_FMC_CLK, "clk_fmc", "fmc_mux",
		CLK_SET_RATE_PARENT, 0x144, 1, 0,
	},
	/* mmc */
	{
		XMORCA_MMC0_CLK, "clk_mmc0", "mmc0_mux",
		CLK_SET_RATE_PARENT, 0x1f4, 28, 0,
	},
	{
		XMORCA_MMC1_CLK, "clk_mmc1", "mmc1_mux",
		CLK_SET_RATE_PARENT, 0x22c, 28, 0,
	},
	{
		XMORCA_MMC2_CLK, "clk_mmc2", "mmc2_mux",
		CLK_SET_RATE_PARENT, 0x240, 28, 0,
	},
	/* uart */
	{
		XMORCA_UART0_CLK, "clk_uart0", "24m",
		CLK_SET_RATE_PARENT, 0x1b8, 0, 0,
	},
	{
		XMORCA_UART1_CLK, "clk_uart1", "uart_mux1",
		CLK_SET_RATE_PARENT, 0x1b8, 1, 0,
	},
	{
		XMORCA_UART2_CLK, "clk_uart2", "uart_mux2",
		CLK_SET_RATE_PARENT, 0x1b8, 2, 0,
	},
	{
		XMORCA_UART3_CLK, "clk_uart3", "uart_mux3",
		CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED, 0x1b8, 3, 0,
	},
	/* spi */
	{
		XMORCA_SPI0_CLK, "clk_spi0", "ssp_mux",
		CLK_SET_RATE_PARENT, 0x1bc, 12, 0,
	},
	{
		XMORCA_SPI1_CLK, "clk_spi1", "ssp_mux",
		CLK_SET_RATE_PARENT, 0x1bc, 13, 0,
	},
	{
		XMORCA_SPI2_CLK, "clk_spi2", "ssp_mux",
		CLK_SET_RATE_PARENT, 0x1bc, 14, 0,
	},
	/* i2c */
	{
		XMORCA_I2C0_CLK, "clk_i2c0", "50m",
		CLK_SET_RATE_PARENT, 0x1b8, 16, 0,
	},
	{
		XMORCA_I2C1_CLK, "clk_i2c1", "50m",
		CLK_SET_RATE_PARENT, 0x1b8, 17, 0,
	},
	{
		XMORCA_I2C2_CLK, "clk_i2c2", "50m",
		CLK_SET_RATE_PARENT, 0x1b8, 18, 0,
	},
	{
		XMORCA_I2C3_CLK, "clk_i2c3", "50m",
		CLK_SET_RATE_PARENT, 0x1b8, 19, 0,
	},
	/* edmac */
	{
		XMORCA_EDMAC_AXICLK, "axi_clk_edmac", NULL,
		CLK_SET_RATE_PARENT, 0x194, 2, 0,
	},
	{
		XMORCA_EDMAC_CLK, "clk_edmac", NULL,
		CLK_SET_RATE_PARENT, 0x194, 1, 0,
	},
};

static void __init xmorca_clk_init(struct device_node *np)
{
	struct lotus_clock_data *clk_data;

	clk_data = lotus_clk_init(np, XMORCA_NR_CLKS);
	if (!clk_data)
		return;
	if (IS_ENABLED(CONFIG_RESET_CONTROLLER))
		lotus_reset_init(np, XMORCA_NR_RSTS);

	lotus_clk_register_fixed_rate(xmorca_fixed_rate_clks,
				     ARRAY_SIZE(xmorca_fixed_rate_clks),
				     clk_data);
	lotus_clk_register_mux(xmorca_mux_clks, ARRAY_SIZE(xmorca_mux_clks),
			      clk_data);
	lotus_clk_register_fixed_factor(xmorca_fixed_factor_clks,
				       ARRAY_SIZE(xmorca_fixed_factor_clks), clk_data);
	lotus_clk_register_gate(xmorca_gate_clks,
			       ARRAY_SIZE(xmorca_gate_clks), clk_data);

}

CLK_OF_DECLARE(xmorca_clk, "lotus,xmorca-clock", xmorca_clk_init);

