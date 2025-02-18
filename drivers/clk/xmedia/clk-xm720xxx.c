/*
 * Copyright (c) XMEDIA. All rights reserved.
 */

#include <dt-bindings/clock/xm720xxx-clock.h>
#include <linux/clk-provider.h>
#include <linux/module.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include "clk.h"
#include "reset.h"

static struct xmedia_fixed_rate_clock xm720xxx_fixed_rate_clks[] __initdata = {
	{ XM720XXX_FIXED_100K,		"100k",	NULL, 0, 100000, },
	{ XM720XXX_FIXED_400K,		"400k",	NULL, 0, 400000, },
	{ XM720XXX_FIXED_3M,		"3m",	NULL, 0, 3000000, },
	{ XM720XXX_FIXED_6M,		"6m",	NULL, 0, 6000000, },
	{ XM720XXX_FIXED_12M,	"12m",	NULL, 0, 12000000, },
	{ XM720XXX_FIXED_24M,	"24m",	NULL, 0, 24000000, },
	{ XM720XXX_FIXED_25M,	"25m",	NULL, 0, 25000000, },
	{ XM720XXX_FIXED_50M,	"50m",	NULL, 0, 50000000, },
	{ XM720XXX_FIXED_83P3M,	"83.3m",NULL, 0, 83300000, },
	{ XM720XXX_FIXED_90M,	"90m", NULL, 0, 90000000, },
	{ XM720XXX_FIXED_100M,	"100m", NULL, 0, 100000000, },
	{ XM720XXX_FIXED_112M,	"112m", NULL, 0, 112000000, },
	{ XM720XXX_FIXED_125M,	"125m", NULL, 0, 125000000, },
	{ XM720XXX_FIXED_150M,	"150m", NULL, 0, 150000000, },
	{ XM720XXX_FIXED_200M,	"200m", NULL, 0, 200000000, },
	{ XM720XXX_FIXED_250M,	"250m", NULL, 0, 250000000, },
	{ XM720XXX_FIXED_300M,	"300m", NULL, 0, 300000000, },
	{ XM720XXX_FIXED_324M,	"324m", NULL, 0, 324000000, },
	{ XM720XXX_FIXED_342M,	"342m", NULL, 0, 342000000, },
	{ XM720XXX_FIXED_342M,	"375m", NULL, 0, 375000000, },
	{ XM720XXX_FIXED_400M,	"400m", NULL, 0, 400000000, },
	{ XM720XXX_FIXED_448M,	"448m", NULL, 0, 448000000, },
	{ XM720XXX_FIXED_500M,	"500m", NULL, 0, 500000000, },
	{ XM720XXX_FIXED_540M,	"540m", NULL, 0, 540000000, },
	{ XM720XXX_FIXED_600M,	"600m", NULL, 0, 600000000, },
	{ XM720XXX_FIXED_750M,	"750m",	NULL, 0, 750000000, },
	{ XM720XXX_FIXED_1000M,	"1000m",NULL, 0, 1000000000, },
	{ XM720XXX_FIXED_1500M,	"1500m",NULL, 0, 1500000000UL, },
};

static const char *sysaxi_mux_p[] __initconst = {
	"24m", "200m"
};
static const char *sysapb_mux_p[] __initconst = {"24m", "50m"};
static const char *uart_mux_p[] __initconst = {"24m", "6m"};
static const char *fmc_mux_p[] __initconst = {"24m", "100m", "150m", "200m", "300m", "360m"};
static const char *mmc_mux_p[] __initdata = {
	"100k", "400k", "25m", "50m", "90m", "112m", "150m"
};
static const char *eth_mux_p[] __initconst = {"100m", "54m"};
static const char *usb_mux_p[] __initdata = {"phy", "crg",};

static u32 sysaxi_mux_table[] = {0, 1};
static u32 sysapb_mux_table[] = {0, 1};
static u32 uart_mux_table[] = {0, 1};
static u32 fmc_mux_table[] = {0, 1, 2, 3, 4, 5, 6, 7};
static u32 mmc_mux_table[] = {0, 1, 2, 3, 4, 5, 6};
static u32 eth_mux_table[] = {0, 1};
static u32 usb_mux_table[] = {0, 1};

static struct xmedia_mux_clock xm720xxx_mux_clks[] __initdata = {
	{
		XM720XXX_SYSAXI_CLK, "sysaxi_mux", sysaxi_mux_p,
		ARRAY_SIZE(sysaxi_mux_p),
		CLK_SET_RATE_PARENT, 0x80, 6, 1, 0, sysaxi_mux_table,
	},
	{
		XM720XXX_SYSAPB_CLK, "sysapb_mux", sysapb_mux_p,
		ARRAY_SIZE(sysapb_mux_p),
		CLK_SET_RATE_PARENT, 0x80, 10, 1, 0, sysapb_mux_table,
	},
	{
		XM720XXX_FMC_MUX, "fmc_mux", fmc_mux_p, ARRAY_SIZE(fmc_mux_p),
		CLK_SET_RATE_PARENT, 0x144, 2, 3, 0, fmc_mux_table,
	},
	{
		XM720XXX_MMC0_MUX, "mmc0_mux", mmc_mux_p, ARRAY_SIZE(mmc_mux_p),
		CLK_SET_RATE_PARENT, 0x1f4, 24, 3, 0, mmc_mux_table,
	},
	{
		XM720XXX_MMC1_MUX, "mmc1_mux", mmc_mux_p, ARRAY_SIZE(mmc_mux_p),
		CLK_SET_RATE_PARENT, 0x22c, 24, 3, 0, mmc_mux_table,
	},
	{
		XM720XXX_UART_MUX, "uart_mux0", uart_mux_p,
		ARRAY_SIZE(uart_mux_p),
		CLK_SET_RATE_PARENT, 0x1bc, 18, 1, 0, uart_mux_table,
	},
	{
		XM720XXX_UART_MUX, "uart_mux1", uart_mux_p,
		ARRAY_SIZE(uart_mux_p),
		CLK_SET_RATE_PARENT, 0x1bc, 19, 1, 0, uart_mux_table,
	},
	{
		XM720XXX_UART_MUX, "uart_mux2", uart_mux_p,
		ARRAY_SIZE(uart_mux_p),
		CLK_SET_RATE_PARENT, 0x1bc, 20, 1, 0, uart_mux_table,
	},
	{
		XM720XXX_ETH_MUX, "eth_mux", eth_mux_p, ARRAY_SIZE(eth_mux_p),
		CLK_SET_RATE_PARENT, 0x16c, 7, 1, 0, eth_mux_table,
	},
	{
		XM720XXX_USB2_MUX, "usb2_mux", usb_mux_p, ARRAY_SIZE(usb_mux_p),
		CLK_SET_RATE_PARENT, 0x140, 13, 0, 0, usb_mux_table
	},
};

#if 1
static struct xmedia_fixed_factor_clock xm720xxx_fixed_factor_clks[] __initdata = {
	{
		XM720XXX_SYSAXI_CLK, "clk_sysaxi", "sysaxi_mux", 1, 4,
		CLK_SET_RATE_PARENT
	},
};
#endif

static struct xmedia_gate_clock xm720xxx_gate_clks[] __initdata = {
	/* fmc */
	{
		XM720XXX_FMC_CLK, "clk_fmc", "fmc_mux",
		CLK_SET_RATE_PARENT, 0x144, 1, 0,
	},
	/* mmc */
	{
		XM720XXX_MMC0_CLK, "clk_mmc0", "mmc0_mux",
		CLK_SET_RATE_PARENT, 0x1f4, 28, 0,
	},
	{
		XM720XXX_MMC1_CLK, "clk_mmc1", "mmc1_mux",
		CLK_SET_RATE_PARENT, 0x22c, 28, 0,
	},
	/* uart */
	{
		XM720XXX_UART0_CLK, "clk_uart0", "24m",
		CLK_SET_RATE_PARENT, 0x1b8, 0, 0,
	},
	{
		XM720XXX_UART1_CLK, "clk_uart1", "uart_mux1",
		CLK_SET_RATE_PARENT, 0x1b8, 1, 0,
	},
	{
		XM720XXX_UART2_CLK, "clk_uart2", "uart_mux2",
		CLK_SET_RATE_PARENT, 0x1b8, 2, 0,
	},
	/* spi */
	{
		XM720XXX_SPI0_CLK, "clk_spi0", "100m",
		CLK_SET_RATE_PARENT, 0x1bc, 12, 0,
	},
	{
		XM720XXX_SPI1_CLK, "clk_spi1", "100m",
		CLK_SET_RATE_PARENT, 0x1bc, 13, 0,
	},
	/* i2c */
	{
		XM720XXX_I2C0_CLK, "clk_i2c0", "50m",
		CLK_SET_RATE_PARENT, 0x1b8, 11, 0,
	},
	{
		XM720XXX_I2C1_CLK, "clk_i2c1", "50m",
		CLK_SET_RATE_PARENT, 0x1b8, 12, 0,
	},
	{
		XM720XXX_I2C2_CLK, "clk_i2c2", "50m",
		CLK_SET_RATE_PARENT, 0x1b8, 13, 0,
	},
	/* ethernet mac */
	{
		XM720XXX_ETH0_CLK, "clk_eth0", "eth_mux",
		CLK_SET_RATE_PARENT, 0x16c, 1, 0,
	},
	/* edmac */
	{
		XM720XXX_EDMAC_AXICLK, "axi_clk_edmac", NULL,
		CLK_SET_RATE_PARENT, 0x194, 2, 0,
	},
	{
		XM720XXX_EDMAC_CLK, "clk_edmac", NULL,
		CLK_SET_RATE_PARENT, 0x194, 1, 0,
	},
	/* usb */
	{
		XM720XXX_USB2_BUS_CLK, "clk_usb2_bus", "usb2_mux",
		CLK_SET_RATE_PARENT, 0x140, 8, 0,
	},
	{
		XM720XXX_USB2_REF_CLK, "clk_usb2_ref", "usb2_mux",
		CLK_SET_RATE_PARENT, 0x140, 9, 0,
	},
	{
		XM720XXX_USB2_UTMI_CLK, "clk_usb2_utmi", "usb2_mux",
		CLK_SET_RATE_PARENT, 0x140, 12, 0,
	},
	{
		XM720XXX_USB2_PHY_APB_CLK, "clk_u2phy_apb_ref",NULL,
		CLK_SET_RATE_PARENT, 0x140, 11, 0,
	},
	{
		XM720XXX_USB2_PHY_PLL_CLK, "clk_u2phy_pll_ref",NULL,
		CLK_SET_RATE_PARENT, 0x140, 4, 0,
	},
	{
		XM720XXX_USB2_PHY_XO_CLK, "clk_u2phy_xo_ref",NULL,
		CLK_SET_RATE_PARENT, 0x140, 2, 0,
	},
};

static void __init xm720xxx_clk_init(struct device_node *np)
{
	struct xmedia_clock_data *clk_data;

	clk_data = xmedia_clk_init(np, XM720XXX_NR_CLKS);
	if (!clk_data)
		return;
	if (IS_ENABLED(CONFIG_RESET_CONTROLLER))
		xmedia_reset_init(np, XM720XXX_NR_RSTS);

	xmedia_clk_register_fixed_rate(xm720xxx_fixed_rate_clks,
				     ARRAY_SIZE(xm720xxx_fixed_rate_clks),
				     clk_data);
	xmedia_clk_register_mux(xm720xxx_mux_clks, ARRAY_SIZE(xm720xxx_mux_clks),
			      clk_data);
	xmedia_clk_register_fixed_factor(xm720xxx_fixed_factor_clks,
				       ARRAY_SIZE(xm720xxx_fixed_factor_clks), clk_data);
	xmedia_clk_register_gate(xm720xxx_gate_clks,
			       ARRAY_SIZE(xm720xxx_gate_clks), clk_data);

}

CLK_OF_DECLARE(xm720xxx_clk, "xmedia,xm720xxx-clock", xm720xxx_clk_init);

