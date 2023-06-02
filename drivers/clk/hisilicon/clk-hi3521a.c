/*
 * Hi3521a Clock Driver
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

#include <dt-bindings/clock/hi3521a-clock.h>
#include <linux/clk-provider.h>
#include <linux/module.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <linux/slab.h>

#include "clk.h"
#include "crg.h"
#include "reset.h"

static const struct
hisi_fixed_rate_clock hi3521a_fixed_rate_clks_crg[] __initconst = {
	{ HI3521A_FIXED_2M, "2m", NULL, 0, 2000000, },
	{ HI3521A_FIXED_2P2M, "2.2m", NULL, 0, 2200000, },
	{ HI3521A_FIXED_2P5M, "2.5m", NULL, 0, 2500000, },
	{ HI3521A_FIXED_2P5M, "3m", NULL, 0, 3000000, },
	{ HI3521A_FIXED_24M, "24m", NULL, 0, 24000000, },
	{ HI3521A_FIXED_25M, "25m", NULL, 0, 25000000, },
	{ HI3521A_FIXED_27M, "27m", NULL, 0, 27000000, },
	{ HI3521A_FIXED_50M, "50m", NULL, 0, 50000000, },
	{ HI3521A_FIXED_54M, "54m", NULL, 0, 54000000, },
	{ HI3521A_FIXED_60M, "60m", NULL, 0, 60000000, },
	{ HI3521A_FIXED_62P5M, "62.5m", NULL, 0, 62500000, },
	{ HI3521A_FIXED_75M, "75m", NULL, 0, 75000000, },
	{ HI3521A_FIXED_83M, "83m", NULL, 0, 83000000, },
	{ HI3521A_FIXED_100M, "100m", NULL, 0, 100000000, },
	{ HI3521A_FIXED_125M, "125m", NULL, 0, 125000000, },
	{ HI3521A_FIXED_150M, "150m", NULL, 0, 150000000, },
	{ HI3521A_FIXED_162M, "162m", NULL, 0, 162000000, },
	{ HI3521A_FIXED_187M, "187m", NULL, 0, 187000000, },
	{ HI3521A_FIXED_187P5M, "187.5m", NULL, 0, 187500000, },
	{ HI3521A_FIXED_202P5M, "202.5m", NULL, 0, 202500000, },
	{ HI3521A_FIXED_250M, "250m", NULL, 0, 250000000, },
	{ HI3521A_FIXED_270M, "270m", NULL, 0, 270000000, },
	{ HI3521A_FIXED_300M, "300m", NULL, 0, 300000000, },
	{ HI3521A_FIXED_324M, "324m", NULL, 0, 324000000, },
	{ HI3521A_FIXED_375M, "375m", NULL, 0, 375000000, },
	{ HI3521A_FIXED_400M, "400m", NULL, 0, 400000000, },
	{ HI3521A_FIXED_405M, "405m", NULL, 0, 405000000, },
	{ HI3521A_FIXED_500M, "500m", NULL, 0, 500000000, },
	{ HI3521A_FIXED_750M, "750m", NULL, 0, 750000000, },
	{ HI3521A_FIXED_800M, "800m", NULL, 0, 800000000, },
	{ HI3521A_FIXED_810M, "810m", NULL, 0, 810000000, },
	{ HI3521A_FIXED_1500M, "1500m", NULL, 0, 1500000000UL, },
	{ HI3521A_FIXED_1620M, "1620m", NULL, 0, 1620000000UL, },
};

static const char *const sysaxi_mux_p[] __initconst = {"24m", "250m", "202.5m"};
static const char *const uart_mux_p[] __initconst = {"clk_sysapb", "2m", "24m"};
static const char *const fmc_mux_p[] __initconst = {"24m", "83m", "150m"};
static const char *const eth_phy_mux_p[] __initconst = {"50m", "25m"};

static u32 sysaxi_mux_table[] __initdata = {0, 1, 2};
static u32 uart_mux_table[] __initdata = {0, 1, 2};
static u32 fmc_mux_table[] __initdata = {0, 1, 2};
static u32 eth_phy_mux_table[] __initdata = {0, 1};

static const struct hisi_mux_clock hi3521a_mux_clks_crg[] __initconst = {
	{ HI3521A_SYSAPB_CLK, "sysaxi_mux", sysaxi_mux_p,
		ARRAY_SIZE(sysaxi_mux_p),
		CLK_SET_RATE_PARENT, 0x34, 12, 2, 0, sysaxi_mux_table, },
	{ HI3521A_FMC_MUX, "fmc_mux", fmc_mux_p, ARRAY_SIZE(fmc_mux_p),
		CLK_SET_RATE_PARENT, 0x74, 2, 2, 0, fmc_mux_table, },
	{ HI3521A_UART_MUX, "uart_mux", uart_mux_p, ARRAY_SIZE(uart_mux_p),
		CLK_SET_RATE_PARENT, 0x84, 18, 2, 0, uart_mux_table, },
	{ HI3521A_ETH_PHY_MUX, "eth_phy_mux", eth_phy_mux_p,
		ARRAY_SIZE(eth_phy_mux_p),
		CLK_SET_RATE_PARENT, 0x78, 6, 1, 0, eth_phy_mux_table, },
};

/* fixed factor clocks */
static struct hisi_fixed_factor_clock
				hi3521a_fixed_factor_clks[] __initdata = {
	{ HI3521A_SYSAPB_CLK, "clk_sysapb", "sysaxi_mux", 1, 4,
		CLK_SET_RATE_PARENT, },
};

static const struct hisi_gate_clock hi3521a_gate_clks[] __initconst = {
	/* fmc */
	{ HI3521A_FMC_CLK, "clk_fmc", "fmc_mux",
		CLK_SET_RATE_PARENT, 0x74, 1, 0, },

	/* uart */
	{ HI3521A_UART0_CLK, "clk_uart0", "uart_mux",
		CLK_SET_RATE_PARENT, 0x84, 15, 0, },
	{ HI3521A_UART1_CLK, "clk_uart1", "uart_mux",
		CLK_SET_RATE_PARENT, 0x84, 16, 0, },
	{ HI3521A_UART2_CLK, "clk_uart2", "uart_mux",
		CLK_SET_RATE_PARENT, 0x84, 17, 0, },
	/* ethernet */
	{ HI3521A_ETH_CLK, "clk_eth", NULL,
		CLK_SET_RATE_PARENT, 0x78, 1, 0, },
	{ HI3521A_ETH_MACIF_CLK, "clk_eth_macif", NULL,
		CLK_SET_RATE_PARENT, 0x78, 3, 0, },
	/* spi */
	{ HI3521A_SPI0_CLK, "clk_spi0", "sysaxi_mux",
		CLK_SET_RATE_PARENT, 0x84, 13, 0, },
	/* dmac */
	{ HI3521A_DMAC_CLK, "clk_dmac", "50m",
		CLK_SET_RATE_PARENT, 0x80, 5, 0, },
};

static void __init hi3521a_clk_crg_init(struct device_node *np)
{
	struct hisi_clock_data *clk_data;
	unsigned int count = 0;
	int ret;

	clk_data = hisi_clk_init(np, HI3521A_CRG_NR_CLKS);
	if (!clk_data) {
		pr_err("%s: failed to allocate CRG clock data\n", __func__);
		return;
	}

	ret = hisi_clk_register_fixed_rate(hi3521a_fixed_rate_clks_crg,
			ARRAY_SIZE(hi3521a_fixed_rate_clks_crg), clk_data);
	if (ret)
		goto err;

	ret = hisi_clk_register_mux(hi3521a_mux_clks_crg,
			ARRAY_SIZE(hi3521a_mux_clks_crg), clk_data);
	if (ret)
		goto unregister_fixed_rate;

	ret = hisi_clk_register_fixed_factor(hi3521a_fixed_factor_clks,
			ARRAY_SIZE(hi3521a_fixed_factor_clks), clk_data);
	if (ret)
		goto unregister_mux;

	ret = hisi_clk_register_gate(hi3521a_gate_clks,
			ARRAY_SIZE(hi3521a_gate_clks), clk_data);
	if (ret)
		goto unregister_factor;

	ret = of_clk_add_provider(np,
			of_clk_src_onecell_get, &clk_data->clk_data);
	if (ret)
		goto unregister_gate;

	if (!of_property_read_u32(np, "#reset-cells", &count) && (count == 2))
		if (hibvt_reset_init(np, HI3521A_CRG_NR_RSTS))
			goto err;

	return;

unregister_gate:
	hisi_clk_unregister_gate(hi3521a_gate_clks,
			ARRAY_SIZE(hi3521a_gate_clks), clk_data);
unregister_factor:
	hisi_clk_unregister_fixed_factor(hi3521a_fixed_factor_clks,
			ARRAY_SIZE(hi3521a_fixed_factor_clks), clk_data);
unregister_mux:
	hisi_clk_unregister_mux(hi3521a_mux_clks_crg,
			ARRAY_SIZE(hi3521a_mux_clks_crg), clk_data);
unregister_fixed_rate:
	hisi_clk_unregister_fixed_rate(hi3521a_fixed_rate_clks_crg,
			ARRAY_SIZE(hi3521a_fixed_rate_clks_crg), clk_data);
err:
	pr_err("%s: failed to init CRG clock\n", __func__);
	return;
}
CLK_OF_DECLARE(hi3521a_clk_crg, "hisilicon,hi3521a-clock",
					hi3521a_clk_crg_init);

/* clock in system control */
static const char *const timer_mux_p[] __initconst = {"3m", "clk_sysapb"};
static u32 timer_mux_table[] __initdata = {0, 1};

static const struct hisi_mux_clock hi3521a_mux_clks_sc_clk[] __initconst = {
	{ HI3521A_TIME0_0_CLK, "timer00", timer_mux_p,
		    ARRAY_SIZE(timer_mux_p), CLK_SET_RATE_PARENT,
			    0x0, 16, 1, 0, timer_mux_table, },

	{ HI3521A_TIME0_1_CLK, "timer01", timer_mux_p,
		    ARRAY_SIZE(timer_mux_p), CLK_SET_RATE_PARENT,
			    0x0, 18, 1, 0, timer_mux_table, },

	{ HI3521A_TIME1_2_CLK, "timer12", timer_mux_p,
		    ARRAY_SIZE(timer_mux_p), CLK_SET_RATE_PARENT,
			    0x0, 20, 1, 0, timer_mux_table, },

	{ HI3521A_TIME1_3_CLK, "timer13", timer_mux_p,
		    ARRAY_SIZE(timer_mux_p), CLK_SET_RATE_PARENT,
			    0x0, 22, 1, 0, timer_mux_table, },

	{ HI3521A_TIME2_4_CLK, "timer24", timer_mux_p,
		    ARRAY_SIZE(timer_mux_p), CLK_SET_RATE_PARENT,
			    0x0, 25, 1, 0, timer_mux_table, },

	{ HI3521A_TIME2_5_CLK, "timer25", timer_mux_p,
		    ARRAY_SIZE(timer_mux_p), CLK_SET_RATE_PARENT,
			    0x0, 27, 1, 0, timer_mux_table, },

	{ HI3521A_TIME3_6_CLK, "timer36", timer_mux_p,
		    ARRAY_SIZE(timer_mux_p), CLK_SET_RATE_PARENT,
			    0x0, 29, 1, 0, timer_mux_table, },

	{ HI3521A_TIME3_7_CLK, "timer37", timer_mux_p,
		    ARRAY_SIZE(timer_mux_p), CLK_SET_RATE_PARENT,
			    0x0, 31, 1, 0, timer_mux_table, },
};

static void __init hi3521a_sc_clk_init(struct device_node *np)
{
	struct hisi_clock_data *clk_data;
	int ret;

	clk_data = hisi_clk_init(np, HI3521A_SC_NR_CLKS);
	if (!clk_data) {
		pr_err("%s: failed to allocate %s clock data\n",
				__func__, np->name);
		return;
	}

	ret = hisi_clk_register_mux(hi3521a_mux_clks_sc_clk,
			ARRAY_SIZE(hi3521a_mux_clks_sc_clk), clk_data);
	if (ret) {
		pr_err("%s: failed to register %s mux clock\n",
				__func__, np->name);
		return;
	}
}

CLK_OF_DECLARE(hi3521a_clk_sysctrl, "hisilicon,sysctrl",
		hi3521a_sc_clk_init);
