/*
 * Hi3518ev20x Clock Driver
 *
 * Copyright (c) 2016-2017 HiSilicon Technologies Co., Ltd.
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include <dt-bindings/clock/hi3518ev20x-clock.h>
#include <linux/clk-provider.h>
#include <linux/module.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <linux/slab.h>

#include "clk.h"
#include "crg.h"
#include "reset.h"

static const struct
hisi_fixed_rate_clock hi3518ev20x_fixed_rate_clks_crg[] __initconst = {
	{ HI3518EV20X_FIXED_3M, "3m", NULL, 0, 3000000, },
	{ HI3518EV20X_FIXED_6M, "6m", NULL, 0, 6000000, },
	{ HI3518EV20X_FIXED_24M, "24m", NULL, 0, 24000000, },
	{ HI3518EV20X_FIXED_25M, "25m", NULL, 0, 25000000, },
	{ HI3518EV20X_FIXED_27M, "27m", NULL, 0, 27000000, },
	{ HI3518EV20X_FIXED_37P125M, "37.125m", NULL, 0, 37125000, },
	{ HI3518EV20X_FIXED_49P5M, "49.5m", NULL, 0, 49500000, },
	{ HI3518EV20X_FIXED_50M, "50m", NULL, 0, 50000000, },
	{ HI3518EV20X_FIXED_54M, "54m", NULL, 0, 54000000, },
	{ HI3518EV20X_FIXED_74P25M, "74.25m", NULL, 0, 74250000, },
	{ HI3518EV20X_FIXED_99M, "99m", NULL, 0, 99000000, },
	{ HI3518EV20X_FIXED_125M, "125m", NULL, 0, 125000000, },
	{ HI3518EV20X_FIXED_148P5M, "148.5m", NULL, 0, 148500000, },
	{ HI3518EV20X_FIXED_198M, "198m", NULL, 0, 198000000, },
	{ HI3518EV20X_FIXED_200M, "200m", NULL, 0, 200000000, },
	{ HI3518EV20X_FIXED_250M, "250m", NULL, 0, 250000000, },
	{ HI3518EV20X_FIXED_297M, "297m", NULL, 0, 297000000, },
	{ HI3518EV20X_FIXED_300M, "300m", NULL, 0, 300000000, },
	{ HI3518EV20X_FIXED_396M, "396m", NULL, 0, 396000000, },
	{ HI3518EV20X_FIXED_540M, "540m", NULL, 0, 540000000, },
	{ HI3518EV20X_FIXED_594M, "594m", NULL, 0, 594000000, },
	{ HI3518EV20X_FIXED_600M, "600m", NULL, 0, 600000000, },
	{ HI3518EV20X_FIXED_650M, "660m", NULL, 0, 660000000, },
	{ HI3518EV20X_FIXED_750M, "750m", NULL, 0, 750000000, },
	{ HI3518EV20X_FIXED_1188M, "1188m", NULL, 0, 1188000000UL, },
};

static const char *const sysapb_mux_p[] __initconst = {"24m", "198m"};
static const char *const uart_mux_p[] __initconst = {"clk_sysapb", "24m", "6m"};
static const char *const fmc_mux_p[] __initconst = {"24m", "148.5m", "125m", "198m"};
static const char *const eth_mux_p[] __initconst = {"99m", "54m"};
static const char *const mmc0_mux_p[] __initconst = {"99m", "74.25m", "49.5m", "148.5m"};
static const char *const mmc1_mux_p[] __initconst = {"49.5m"};

static u32 sysapb_mux_table[] = {0, 1};
static u32 uart_mux_table[] = {0, 1, 2};
static u32 fmc_mux_table[] = {0, 1, 2, 3};
static u32 eth_mux_table[] = {0, 1};
static u32 mmc0_mux_table[] = {0, 1, 2, 3};
static u32 mmc1_mux_table[] = {0};

static const struct hisi_mux_clock hi3518ev20x_mux_clks_crg[] __initconst = {
	{ HI3518EV20X_SYSAPB_CLK, "sysapb_mux", sysapb_mux_p,
		ARRAY_SIZE(sysapb_mux_p),
		CLK_SET_RATE_PARENT, 0x30, 0, 1, 0, sysapb_mux_table, },
	{ HI3518EV20X_FMC_MUX, "fmc_mux", fmc_mux_p, ARRAY_SIZE(fmc_mux_p),
		    CLK_SET_RATE_PARENT, 0xc0, 2, 2, 0, fmc_mux_table, },
	{ HI3518EV20X_UART_MUX, "uart_mux", uart_mux_p,
		ARRAY_SIZE(uart_mux_p),
		CLK_SET_RATE_PARENT, 0xe4, 19, 1, 0, uart_mux_table, },
	/* ethernet clock select */
	{ HI3518EV20X_ETH_MUX, "eth_mux", eth_mux_p, ARRAY_SIZE(eth_mux_p),
		    CLK_SET_RATE_PARENT, 0xec, 7, 0, 0, eth_mux_table, },
	/* MMC0 clock select */
	{ HI3518EV20X_MMC0_MUX, "mmc0_mux", mmc0_mux_p, ARRAY_SIZE(mmc0_mux_p),
		CLK_SET_RATE_PARENT, 0xc4, 10, 2, 0, mmc0_mux_table, },
	/* MMC1 clock select */
	{ HI3518EV20X_MMC1_MUX, "mmc1_mux", mmc1_mux_p, ARRAY_SIZE(mmc1_mux_p),
		CLK_SET_RATE_PARENT, 0xc4, 2, 2, 0, mmc1_mux_table, },
};

/* fixed factor clocks */
static struct hisi_fixed_factor_clock
				hi3518ev20x_fixed_factor_clks[] __initdata = {
	{ HI3518EV20X_SYSAPB_CLK, "sysapb", "sysapb_mux", 1, 4,
		CLK_SET_RATE_PARENT, },
};

static const struct hisi_gate_clock hi3518ev20x_gate_clks[] __initconst = {
	/* fmc */
	{ HI3518EV20X_FMC_CLK, "clk_fmc", "fmc_mux",
		CLK_SET_RATE_PARENT, 0xc0, 1, 0, },

	/* usb ctrl */
	{ HI3518EV20X_USB2_CTRL_UTMI0_REQ, "usb2_cttl_utmi0_req", NULL,
		CLK_SET_RATE_PARENT, 0xb4, 5, 1, },
	{ HI3518EV20X_USB2_HRST_REQ, "usb2_hrst_req", NULL,
		CLK_SET_RATE_PARENT, 0xb4, 0, 1, },

	/* uart */
	{ HI3518EV20X_UART0_CLK, "clk_uart0", "uart_mux",
		CLK_SET_RATE_PARENT, 0xe4, 15, 0, },
	{ HI3518EV20X_UART1_CLK, "clk_uart1", "uart_mux",
		CLK_SET_RATE_PARENT, 0xe4, 16, 0, },
	{ HI3518EV20X_UART2_CLK, "clk_uart2", "uart_mux",
		CLK_SET_RATE_PARENT, 0xe4, 17, 0, },
	/* ethernet */
	{ HI3518EV20X_ETH_CLK, "clk_eth", "eth_mux",
		CLK_SET_RATE_PARENT, 0xec, 1, 0, },
	/* mmc0 */
	{ HI3518EV20X_MMC0_CLK, "clk_mmc0", "mmc0_mux",
		CLK_SET_RATE_PARENT, 0xc4, 9, 0, },
	/* mmc1 */
	{ HI3518EV20X_MMC1_CLK, "clk_mmc1", "mmc1_mux",
		CLK_SET_RATE_PARENT, 0xc4, 1, 0, },
	/* spi */
	{ HI3518EV20X_SPI0_CLK, "clk_spi0", "sysapb_mux",
		CLK_SET_RATE_PARENT, 0xe4, 13, 0, },
	{ HI3518EV20X_SPI1_CLK, "clk_spi1", "sysapb_mux",
		CLK_SET_RATE_PARENT, 0xe4, 14, 0, },
	/* dmac */
	{ HI3518EV20X_DMAC_CLK, "clk_dmac", "50m",
		CLK_SET_RATE_PARENT, 0xd8, 5, 0, },
};

static void __init hi3518ev20x_clk_crg_init(struct device_node *np)
{
	struct hisi_clock_data *clk_data;
	unsigned int count = 0;
	int ret;

	clk_data = hisi_clk_init(np, HI3518EV20X_CRG_NR_CLKS);
	if (!clk_data) {
		pr_err("%s: failed to allocate CRG clock data\n", __func__);
		return;
	}

	ret = hisi_clk_register_fixed_rate(hi3518ev20x_fixed_rate_clks_crg,
			ARRAY_SIZE(hi3518ev20x_fixed_rate_clks_crg), clk_data);
	if (ret)
		goto err;

	ret = hisi_clk_register_mux(hi3518ev20x_mux_clks_crg,
			ARRAY_SIZE(hi3518ev20x_mux_clks_crg), clk_data);
	if (ret)
		goto unregister_fixed_rate;

	ret = hisi_clk_register_fixed_factor(hi3518ev20x_fixed_factor_clks,
			ARRAY_SIZE(hi3518ev20x_fixed_factor_clks), clk_data);
	if (ret)
		goto unregister_mux;

	ret = hisi_clk_register_gate(hi3518ev20x_gate_clks,
			ARRAY_SIZE(hi3518ev20x_gate_clks), clk_data);
	if (ret)
		goto unregister_factor;

	ret = of_clk_add_provider(np,
			of_clk_src_onecell_get, &clk_data->clk_data);
	if (ret)
		goto unregister_gate;

	if (!of_property_read_u32(np, "#reset-cells", &count) && (count == 2))
		if (hibvt_reset_init(np, HI3518EV20X_CRG_NR_RSTS))
			goto err;

	return;

unregister_gate:
	hisi_clk_unregister_gate(hi3518ev20x_gate_clks,
			ARRAY_SIZE(hi3518ev20x_gate_clks), clk_data);
unregister_factor:
	hisi_clk_unregister_fixed_factor(hi3518ev20x_fixed_factor_clks,
			ARRAY_SIZE(hi3518ev20x_fixed_factor_clks), clk_data);
unregister_mux:
	hisi_clk_unregister_mux(hi3518ev20x_mux_clks_crg,
			ARRAY_SIZE(hi3518ev20x_mux_clks_crg), clk_data);
unregister_fixed_rate:
	hisi_clk_unregister_fixed_rate(hi3518ev20x_fixed_rate_clks_crg,
			ARRAY_SIZE(hi3518ev20x_fixed_rate_clks_crg), clk_data);
err:
	pr_err("%s: failed to init CRG clock\n", __func__);
	return;
}
CLK_OF_DECLARE(hi3518ev20x_clk_crg, "hisilicon,hi3518ev20x-clock",
					hi3518ev20x_clk_crg_init);

/* clock in system control */
static const char *const timer_mux_p[] __initconst = {"3m", "sysapb"};
static u32 timer_mux_table[] __initdata = {0, 1};

static const struct hisi_mux_clock hi3518ev20x_mux_clks_sc_clk[] __initconst = {
	{ HI3518EV20X_TIME0_0_CLK, "timer00", timer_mux_p,
		    ARRAY_SIZE(timer_mux_p), CLK_SET_RATE_PARENT,
			    0x0, 16, 1, 0, timer_mux_table, },

	{ HI3518EV20X_TIME0_1_CLK, "timer01", timer_mux_p,
		    ARRAY_SIZE(timer_mux_p), CLK_SET_RATE_PARENT,
			    0x0, 18, 1, 0, timer_mux_table, },

	{ HI3518EV20X_TIME1_2_CLK, "timer12", timer_mux_p,
		    ARRAY_SIZE(timer_mux_p), CLK_SET_RATE_PARENT,
			    0x0, 20, 1, 0, timer_mux_table, },

	{ HI3518EV20X_TIME1_3_CLK, "timer13", timer_mux_p,
		    ARRAY_SIZE(timer_mux_p), CLK_SET_RATE_PARENT,
			    0x0, 22, 1, 0, timer_mux_table, },
};

static void __init hi3518ev20x_sc_clk_init(struct device_node *np)
{
	struct hisi_clock_data *clk_data;
	int ret;

	clk_data = hisi_clk_init(np, HI3518EV20X_SC_NR_CLKS);
	if (!clk_data) {
		pr_err("%s: failed to allocate %s clock data\n",
				__func__, np->name);
		return;
	}

	ret = hisi_clk_register_mux(hi3518ev20x_mux_clks_sc_clk,
			ARRAY_SIZE(hi3518ev20x_mux_clks_sc_clk), clk_data);
	if (ret) {
		pr_err("%s: failed to register %s mux clock\n",
				__func__, np->name);
		return;
	}
}

CLK_OF_DECLARE(hi3518ev20x_clk_sysctrl, "hisilicon,sysctrl",
		hi3518ev20x_sc_clk_init);
