// SPDX-License-Identifier: GPL-2.0

#include <dt-bindings/clock/xmfalcon-clock.h>
#include <linux/clk-provider.h>
#include <linux/module.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include "clk.h"
#include "reset.h"

static struct lotus_fixed_rate_clock xmfalcon_fixed_rate_clks[] __initdata = {
	{ XMFALCON_FIXED_400K,		"400k",		NULL, 0, 400000, },
	{ XMFALCON_FIXED_1600K,		"1600k",	NULL, 0, 1600000, },
	{ XMFALCON_FIXED_3M,		"3m",		NULL, 0, 3000000, },
	{ XMFALCON_FIXED_24M,		"24m",		NULL, 0, 24000000, },
	{ XMFALCON_FIXED_25M,		"25m",		NULL, 0, 25000000, },
	{ XMFALCON_FIXED_50M,		"50m",		NULL, 0, 50000000, },
	{ XMFALCON_FIXED_90M,		"90m",		NULL, 0, 90000000, },
	{ XMFALCON_FIXED_99M,		"99m",		NULL, 0, 99000000, },
	{ XMFALCON_FIXED_100M,		"100m",		NULL, 0, 100000000, },
	{ XMFALCON_FIXED_112M,		"112m",		NULL, 0, 112000000, },
	{ XMFALCON_FIXED_148P5M,	"148.5m",	NULL, 0, 148500000, },
	{ XMFALCON_FIXED_150M,		"150m",		NULL, 0, 150000000, },
	{ XMFALCON_FIXED_198M,		"198m",		NULL, 0, 198000000, },
	{ XMFALCON_FIXED_200M,		"200m",		NULL, 0, 200000000, },
	{ XMFALCON_FIXED_300M,		"300m",		NULL, 0, 300000000, },
	{ XMFALCON_FIXED_396M,		"396m",		NULL, 0, 396000000, },
	{ XMFALCON_FIXED_458M,		"458m",		NULL, 0, 458000000, },
	{ XMFALCON_FIXED_594M,		"594m",		NULL, 0, 594000000, },
	{ XMFALCON_FIXED_792M,		"792m",		NULL, 0, 792000000, },
	{ XMFALCON_FIXED_917M,		"917m",		NULL, 0, 917000000, },
	{ XMFALCON_FIXED_1188M,		"1188m",	NULL, 0, 1188000000UL, },
	{ XMFALCON_FIXED_1375M,		"1375m",	NULL, 0, 1375000000UL, },
	{ XMFALCON_FIXED_1500M,		"1500m",	NULL, 0, 1500000000UL, },
	{ XMFALCON_FIXED_1000M,		"1000m",	NULL, 0, 1000000000UL, },
};

/* Sys APB mux  */
static const char *sysapb_mux_p[] __initconst = {"24m", "50m"};
static u32 sysapb_mux_table[] = {0, 1};

/* CPU mux */
static const char *cpu_mux_p[] __initconst = {"396m", "594m", "792m", "917m", "1188m", "1375m", "1000m"};
static u32 cpu_mux_table[] = {7, 6, 5, 4, 3, 2, 1};

/* UART mux */
static const char *uart_mux_p[] __initconst = {"24m", "3m"};
static u32 uart_mux_table[] = {0, 1};

/* FMC mux */
static const char *fmc_mux_p[] __initconst = {"24m", "99m", "148.5m", "198m", "297m", "396m"};
static u32 fmc_mux_table[] = {0, 1, 2, 3, 4, 5};

/* MMC mux */
static const char *mmc_mux_p[] __initconst = {
	"100k", "400k", "25m", "50m", "90m", "112m", "150m", "200m"
};
static u32 mmc_mux_table[] = {0, 1, 2, 3, 4, 5, 6, 7};

static struct lotus_mux_clock xmfalcon_mux_clks[] __initdata = {
	{
		XMFALCON_SYSAPB_CLK, "sysapb_mux", sysapb_mux_p,
		ARRAY_SIZE(sysapb_mux_p),
		CLK_SET_RATE_PARENT, 0x80, 16, 1, 0, sysapb_mux_table,
	},
	{
		XMFALCON_CPU_MUX, "cpu_mux", cpu_mux_p, ARRAY_SIZE(cpu_mux_p),
		CLK_SET_RATE_PARENT, 0x80, 0, 3, 0, cpu_mux_table,
	},
	{
		XMFALCON_FMC_MUX, "fmc_mux", fmc_mux_p, ARRAY_SIZE(fmc_mux_p),
		CLK_SET_RATE_PARENT, 0x144, 2, 3, 0, fmc_mux_table,
	},
	{
		XMFALCON_MMC0_MUX, "mmc0_mux", mmc_mux_p, ARRAY_SIZE(mmc_mux_p),
		CLK_SET_RATE_PARENT, 0x1f4, 24, 3, 0, mmc_mux_table,
	},
	{
		XMFALCON_MMC1_MUX, "mmc1_mux", mmc_mux_p, ARRAY_SIZE(mmc_mux_p),
		CLK_SET_RATE_PARENT, 0x22c, 24, 3, 0, mmc_mux_table,
	},
	{
		XMFALCON_MMC2_MUX, "mmc2_mux", mmc_mux_p, ARRAY_SIZE(mmc_mux_p),
		CLK_SET_RATE_PARENT, 0x240, 24, 3, 0, mmc_mux_table,
	},
	{
		XMFALCON_UART0_MUX, "uart_mux0", uart_mux_p,
		ARRAY_SIZE(uart_mux_p),
		CLK_SET_RATE_PARENT, 0x1bc, 20, 1, 0, uart_mux_table,
	},
	{
		XMFALCON_UART1_MUX, "uart_mux1", uart_mux_p,
		ARRAY_SIZE(uart_mux_p),
		CLK_SET_RATE_PARENT, 0x1bc, 21, 1, 0, uart_mux_table,
	},
	{
		XMFALCON_UART2_MUX, "uart_mux2", uart_mux_p,
		ARRAY_SIZE(uart_mux_p),
		CLK_SET_RATE_PARENT, 0x1bc, 22, 1, 0, uart_mux_table,
	},
	{
		XMFALCON_UART3_MUX, "uart_mux3", uart_mux_p,
		ARRAY_SIZE(uart_mux_p),
		CLK_SET_RATE_PARENT, 0x1bc, 23, 1, 0, uart_mux_table,
	},
	{
		XMFALCON_UART4_MUX, "uart_mux4", uart_mux_p,
		ARRAY_SIZE(uart_mux_p),
		CLK_SET_RATE_PARENT, 0x1bc, 24, 1, 0, uart_mux_table,
	},
};

static struct lotus_fixed_factor_clock xmfalcon_fixed_factor_clks[] __initdata = {
};

static struct lotus_gate_clock xmfalcon_gate_clks[] __initdata = {
	/* fmc */
	{
		XMFALCON_FMC_CLK, "clk_fmc", "fmc_mux",
		CLK_SET_RATE_PARENT, 0x144, 1, 0,
	},
	/* mmc */
	{
		XMFALCON_MMC0_CLK, "clk_mmc0", "mmc0_mux",
		CLK_SET_RATE_PARENT, 0x1f4, 28, 0,
	},
	{
		XMFALCON_MMC1_CLK, "clk_mmc1", "mmc1_mux",
		CLK_SET_RATE_PARENT, 0x22c, 28, 0,
	},
	{
		XMFALCON_MMC2_CLK, "clk_mmc2", "mmc2_mux",
		CLK_SET_RATE_PARENT, 0x240, 28, 0,
	},
	/* uart */
	{
		XMFALCON_UART0_CLK, "clk_uart0", "24m",
		CLK_SET_RATE_PARENT, 0x1b8, 0, 0,
	},
	{
		XMFALCON_UART1_CLK, "clk_uart1", "uart_mux1",
		CLK_SET_RATE_PARENT, 0x1b8, 1, 0,
	},
	{
		XMFALCON_UART2_CLK, "clk_uart2", "uart_mux2",
		CLK_SET_RATE_PARENT, 0x1b8, 2, 0,
	},
	{
		XMFALCON_UART3_CLK, "clk_uart3", "uart_mux3",
		CLK_SET_RATE_PARENT, 0x1b8, 3, 0,
	},
	{
		XMFALCON_UART4_CLK, "clk_uart4", "uart_mux4",
		CLK_SET_RATE_PARENT, 0x1b8, 4, 0,
	},
	/* spi */
	{
		XMFALCON_SPI0_CLK, "clk_spi0", "100m",
		CLK_SET_RATE_PARENT, 0x1bc, 12, 0,
	},
	{
		XMFALCON_SPI1_CLK, "clk_spi1", "100m",
		CLK_SET_RATE_PARENT, 0x1bc, 13, 0,
	},
	{
		XMFALCON_SPI2_CLK, "clk_spi2", "100m",
		CLK_SET_RATE_PARENT, 0x1bc, 14, 0,
	},
	{
		XMFALCON_SPI3_CLK, "clk_spi3", "100m",
		CLK_SET_RATE_PARENT, 0x1bc, 15, 0,
	},
	/* i2c */
	{
		XMFALCON_I2C0_CLK, "clk_i2c0", "50m",
		CLK_SET_RATE_PARENT, 0x1b8, 16, 0,
	},
	{
		XMFALCON_I2C1_CLK, "clk_i2c1", "50m",
		CLK_SET_RATE_PARENT, 0x1b8, 17, 0,
	},
	{
		XMFALCON_I2C2_CLK, "clk_i2c2", "50m",
		CLK_SET_RATE_PARENT, 0x1b8, 18, 0,
	},
	{
		XMFALCON_I2C3_CLK, "clk_i2c3", "50m",
		CLK_SET_RATE_PARENT, 0x1b8, 19, 0,
	},
	{
		XMFALCON_I2C4_CLK, "clk_i2c4", "50m",
		CLK_SET_RATE_PARENT, 0x1b8, 20, 0,
	},
	{
		XMFALCON_I2C5_CLK, "clk_i2c5", "50m",
		CLK_SET_RATE_PARENT, 0x1b8, 21, 0,
	},
	{
		XMFALCON_I2C6_CLK, "clk_i2c6", "50m",
		CLK_SET_RATE_PARENT, 0x1b8, 22, 0,
	},
	{
		XMFALCON_I2C7_CLK, "clk_i2c7", "50m",
		CLK_SET_RATE_PARENT, 0x1b8, 23, 0,
	},
	/* edmac */
	{
		XMFALCON_EDMAC_AXICLK, "axi_clk_edmac", NULL,
		CLK_SET_RATE_PARENT, 0x194, 2, 0,
	},
	{
		XMFALCON_EDMAC_CLK, "clk_edmac", NULL,
		CLK_SET_RATE_PARENT, 0x194, 1, 0,
	},
};

static void __init xmfalcon_clk_init(struct device_node *np)
{
	struct lotus_clock_data *clk_data;

	clk_data = lotus_clk_init(np, XMFALCON_NR_CLKS);
	if (!clk_data)
		return;
	if (IS_ENABLED(CONFIG_RESET_CONTROLLER))
		lotus_reset_init(np, XMFALCON_NR_RSTS);

	lotus_clk_register_fixed_rate(xmfalcon_fixed_rate_clks,
				     ARRAY_SIZE(xmfalcon_fixed_rate_clks),
				     clk_data);
	lotus_clk_register_mux(xmfalcon_mux_clks, ARRAY_SIZE(xmfalcon_mux_clks),
			      clk_data);
	lotus_clk_register_fixed_factor(xmfalcon_fixed_factor_clks,
				       ARRAY_SIZE(xmfalcon_fixed_factor_clks), clk_data);
	lotus_clk_register_gate(xmfalcon_gate_clks,
			       ARRAY_SIZE(xmfalcon_gate_clks), clk_data);

}

CLK_OF_DECLARE(xmfalcon_clk, "lotus,xmfalcon-clock", xmfalcon_clk_init);

