/*
 * Hi3516A Clock Driver
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

#include <dt-bindings/clock/hi3516a-clock.h>
#include <linux/clk-provider.h>
#include <linux/module.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <linux/slab.h>

#include "clk.h"
#include "crg.h"
#include "reset.h"

struct hi3516a_pll_clock {
	u32     id;
	const char  *name;
	const char  *parent_name;
	u32     ctrl_reg1;
	u8      frac_shift;
	u8      frac_width;
	u8      postdiv1_shift;
	u8      postdiv1_width;
	u8      postdiv2_shift;
	u8      postdiv2_width;
	u32     ctrl_reg2;
	u8      fbdiv_shift;
	u8      fbdiv_width;
	u8      refdiv_shift;
	u8      refdiv_width;
};

struct hi3516a_clk_pll {
	struct clk_hw   hw;
	u32     id;
	void __iomem    *ctrl_reg1;
	u8      frac_shift;
	u8      frac_width;
	u8      postdiv1_shift;
	u8      postdiv1_width;
	u8      postdiv2_shift;
	u8      postdiv2_width;
	void __iomem    *ctrl_reg2;
	u8      fbdiv_shift;
	u8      fbdiv_width;
	u8      refdiv_shift;
	u8      refdiv_width;
};

static const struct
hisi_fixed_rate_clock hi3516a_fixed_rate_clks_crg[] __initconst = {
	{ HI3516A_FIXED_3M, "3m", NULL, 0, 3000000, },
	{ HI3516A_FIXED_6M, "6m", NULL, 0, 6000000, },
	{ HI3516A_FIXED_13P5M, "13.5m", NULL, 0, 13500000, },
	{ HI3516A_FIXED_24M, "24m", NULL, 0, 24000000, },
	{ HI3516A_FIXED_25M, "25m", NULL, 0, 25000000, },
	{ HI3516A_FIXED_27M, "27m", NULL, 0, 27000000, },
	{ HI3516A_FIXED_37P125M, "37.125m", NULL, 0, 37125000, },
	{ HI3516A_FIXED_50M, "50m", NULL, 0, 50000000, },
	{ HI3516A_FIXED_74P25M, "74.25m", NULL, 0, 74250000, },
	{ HI3516A_FIXED_75M, "75m", NULL, 0, 75000000, },
	{ HI3516A_FIXED_99M, "99m", NULL, 0, 99000000, },
	{ HI3516A_FIXED_100M, "100m", NULL, 0, 100000000, },
	{ HI3516A_FIXED_125M, "125m", NULL, 0, 125000000, },
	{ HI3516A_FIXED_145M, "145m", NULL, 0, 145000000, },
	{ HI3516A_FIXED_148P5M, "148.5m", NULL, 0, 148500000, },
	{ HI3516A_FIXED_150M, "150m", NULL, 0, 150000000, },
	{ HI3516A_FIXED_194M, "194m", NULL, 0, 194000000, },
	{ HI3516A_FIXED_198M, "198m", NULL, 0, 198000000, },
	{ HI3516A_FIXED_200M, "200m", NULL, 0, 200000000, },
	{ HI3516A_FIXED_229M, "229m", NULL, 0, 229000000, },
	{ HI3516A_FIXED_237M, "237m", NULL, 0, 237000000, },
	{ HI3516A_FIXED_242M, "242m", NULL, 0, 242000000, },
	{ HI3516A_FIXED_250M, "250m", NULL, 0, 250000000, },
	{ HI3516A_FIXED_297M, "297m", NULL, 0, 297000000, },
	{ HI3516A_FIXED_300M, "300m", NULL, 0, 300000000, },
	{ HI3516A_FIXED_333M, "333m", NULL, 0, 333000000, },
	{ HI3516A_FIXED_400M, "400m", NULL, 0, 400000000, },
	{ HI3516A_FIXED_500M, "500m", NULL, 0, 500000000, },
	{ HI3516A_FIXED_594M, "594m", NULL, 0, 594000000, },
	{ HI3516A_FIXED_600M, "600m", NULL, 0, 600000000, },
	{ HI3516A_FIXED_726P25M, "725.25m", NULL, 0, 726250000, },
	{ HI3516A_FIXED_750M, "750m", NULL, 0, 750000000, },
	{ HI3516A_FIXED_900M, "900m", NULL, 0, 900000000, },
	{ HI3516A_FIXED_1000M, "1000m", NULL, 0, 1000000000UL, },
	{ HI3516A_FIXED_1188M, "1188m", NULL, 0, 1188000000UL, },
};

static const char *const sysaxi_mux_p[] __initconst = {"198m", "148.5m"};
static const char *const uart_mux_p[] __initconst = {"clk_sysapb", "6m"};
static const char *const snor_mux_p[] __initconst = {"24m", "75m", "125m"};
static const char *const snand_mux_p[] __initconst = {"24m", "75m", "125m"};
static const char *const nand_mux_p[] __initconst = {"24m", "198m"};
static const char *const eth_phy_mux_p[] __initconst = {"50m", "25m"};
static const char *const a7_mux_p[] __initconst = {"400m", "500m", "apll"};
static const char *const mmc_mux_p[] __initconst = {"50m", "100m", "25m", "75m"};

static u32 sysaxi_mux_table[] __initdata = {0, 1};
static u32 uart_mux_table[] __initdata = {0, 1};
static u32 snor_mux_table[] __initdata = {0, 1, 2};
static u32 snand_mux_table[] __initdata = {0, 1, 2};
static u32 nand_mux_table[] __initdata = {0, 1};
static u32 eth_phy_mux_table[] __initdata = {0, 1};
static u32 a7_mux_table[] __initdata = {2, 1, 0};
static u32 mmc_mux_table[] __initdata = {0, 1, 2, 3};

static const struct hisi_mux_clock hi3516a_mux_clks_crg[] __initconst = {
	{ HI3516A_SYSAXI_CLK, "sysaxi_mux", sysaxi_mux_p,
		ARRAY_SIZE(sysaxi_mux_p),
		CLK_SET_RATE_PARENT, 0x30, 3, 1, 0, sysaxi_mux_table, },
	{ HI3516A_SNOR_MUX, "snor_mux", snor_mux_p, ARRAY_SIZE(snor_mux_p),
		CLK_SET_RATE_PARENT, 0xc0, 2, 2, 0, snor_mux_table, },
	{ HI3516A_SNAND_MUX, "snand_mux", snand_mux_p, ARRAY_SIZE(snand_mux_p),
		CLK_SET_RATE_PARENT, 0xc0, 6, 2, 0, snand_mux_table, },
	{ HI3516A_NAND_MUX, "nand_mux", nand_mux_p, ARRAY_SIZE(nand_mux_p),
		CLK_SET_RATE_PARENT, 0xd0, 2, 1, 0, nand_mux_table, },
	{ HI3516A_MMC0_MUX, "mmc0_mux", mmc_mux_p, ARRAY_SIZE(mmc_mux_p),
		CLK_SET_RATE_PARENT, 0xc4, 2, 2, 0, mmc_mux_table, },
	{ HI3516A_MMC1_MUX, "mmc1_mux", mmc_mux_p, ARRAY_SIZE(mmc_mux_p),
		CLK_SET_RATE_PARENT, 0xc4, 10, 2, 0, mmc_mux_table, },
	{ HI3516A_UART_MUX, "uart_mux", uart_mux_p,
		ARRAY_SIZE(uart_mux_p),
		CLK_SET_RATE_PARENT, 0xe4, 19, 1, 0, uart_mux_table, },
	{ HI3516A_ETH_PHY_MUX, "eth_phy_mux", eth_phy_mux_p,
		ARRAY_SIZE(eth_phy_mux_p), CLK_SET_RATE_PARENT,
		0xcc, 6, 1, 0, eth_phy_mux_table, },
	{ HI3516A_A7_MUX, "a7_mux", a7_mux_p, ARRAY_SIZE(a7_mux_p),
		CLK_SET_RATE_PARENT, 0x30, 8, 2, 0, a7_mux_table, },
};

/* fixed factor clocks */
static struct hisi_fixed_factor_clock
				hi3516a_fixed_factor_clks[] __initdata = {
	{ HI3516A_SYSAXI_CLK, "clk_sysapb", "sysaxi_mux", 1, 4,
		CLK_SET_RATE_PARENT, },
};

static const struct hisi_gate_clock hi3516a_gate_clks[] __initconst = {
	/* spi nor */
	{ HI3516A_SNOR_CLK, "clk_snor", "snor_mux",
		CLK_SET_RATE_PARENT, 0xc0, 1, 0, },
	/* spi nand */
	{ HI3516A_SNAND_CLK, "clk_snand", "snand_mux",
		CLK_SET_RATE_PARENT, 0xc0, 5, 0, },
	/* nand */
	{ HI3516A_NAND_CLK, "clk_nand", "nand_mux",
		CLK_SET_RATE_PARENT, 0xd8, 1, 0, },
	/* mmc */
	{ HI3516A_MMC0_CLK, "clk_mmc0", "mmc0_mux",
		CLK_SET_RATE_PARENT, 0xc4, 1, 0, },
	{ HI3516A_MMC1_CLK, "clk_mmc1", "mmc1_mux",
		CLK_SET_RATE_PARENT, 0xc4, 9, 0, },

	/* usb ctrl */
	{ HI3516A_USB2_CTRL_UTMI0_REQ, "usb2_cttl_utmi0_req", NULL,
                CLK_SET_RATE_PARENT, 0xb4, 5, 1, },
        { HI3516A_USB2_HRST_REQ, "usb2_hrst_req", NULL,
                CLK_SET_RATE_PARENT, 0xb4, 0, 1, },

	/* uart */
	{ HI3516A_UART0_CLK, "clk_uart0", "50m",
		CLK_SET_RATE_PARENT, 0xe4, 15, 0, },
	{ HI3516A_UART1_CLK, "clk_uart1", "50m",
		CLK_SET_RATE_PARENT, 0xe4, 16, 0, },
	{ HI3516A_UART2_CLK, "clk_uart2", "50m",
		CLK_SET_RATE_PARENT, 0xe4, 17, 0, },
	{ HI3516A_UART3_CLK, "clk_uart3", "50m",
		CLK_SET_RATE_PARENT, 0xe4, 18, 0, },
	/* ethernet mac */
	{ HI3516A_ETH_CLK, "clk_eth", NULL,
		CLK_SET_RATE_PARENT, 0xcc, 1, 0, },
	{ HI3516A_ETH_MACIF_CLK, "clk_eth_macif", NULL,
		CLK_SET_RATE_PARENT, 0xcc, 3, 0, },
	/* spi */
	{ HI3516A_SPI0_CLK, "clk_spi0", "clk_sysapb",
		CLK_SET_RATE_PARENT, 0xe4, 13, 0, },
	{ HI3516A_SPI1_CLK, "clk_spi1", "clk_sysapb",
		CLK_SET_RATE_PARENT, 0xe4, 14, 0, },
	/* dmac */
	{ HI3516A_DMAC_CLK, "clk_dmac", "50m",
		CLK_SET_RATE_PARENT, 0xd8, 5, 0, },
};

static struct hi3516a_pll_clock hi3516a_pll_clks[] __initdata = {
	{ HI3516A_APLL_CLK, "apll", NULL, 0x0, 0, 24, 24, 3, 28, 3,
		0x4, 0, 12, 12, 6},
};

#define to_pll_clk(_hw) container_of(_hw, struct hi3516a_clk_pll, hw)

static void __init hi3516a_calc_pll(u32 *frac_val, u32 *postdiv1_val, u32 *postdiv2_val,
		u32 *fbdiv_val, u32 *refdiv_val, u64 rate)
{
	u64 rem;
	*frac_val = 0;
	rem = do_div(rate, 1000000);
	*fbdiv_val = rate;
	*refdiv_val = 24;
	rem = rem * (1 << 24);
	do_div(rem, 1000000);
	*frac_val = rem;
}

static int __init clk_pll_set_rate(struct clk_hw *hw,
		unsigned long rate,
		unsigned long parent_rate)
{
	struct hi3516a_clk_pll *clk = to_pll_clk(hw);
	u32 frac_val, postdiv1_val, postdiv2_val, fbdiv_val, refdiv_val;
	u32 val;

	/*Fixme  ignore postdives now because apll don't use them*/
	postdiv1_val = postdiv2_val = 0;

	hi3516a_calc_pll(&frac_val, &postdiv1_val, &postdiv2_val,
			&fbdiv_val, &refdiv_val, (u64)rate);

	val = readl_relaxed(clk->ctrl_reg1);
	val &= ~(((1 << clk->frac_width) - 1) << clk->frac_shift);
	val &= ~(((1 << clk->postdiv1_width) - 1) << clk->postdiv1_shift);
	val &= ~(((1 << clk->postdiv2_width) - 1) << clk->postdiv2_shift);

	val |= frac_val << clk->frac_shift;
	val |= postdiv1_val << clk->postdiv1_shift;
	val |= postdiv2_val << clk->postdiv2_shift;
	writel_relaxed(val, clk->ctrl_reg1);

	val = readl_relaxed(clk->ctrl_reg2);
	val &= ~(((1 << clk->fbdiv_width) - 1) << clk->fbdiv_shift);
	val &= ~(((1 << clk->refdiv_width) - 1) << clk->refdiv_shift);

	val |= fbdiv_val << clk->fbdiv_shift;
	val |= refdiv_val << clk->refdiv_shift;
	writel_relaxed(val, clk->ctrl_reg2);

	return 0;
}

static unsigned long __init clk_pll_recalc_rate(struct clk_hw *hw,
		unsigned long parent_rate)
{
	struct hi3516a_clk_pll *clk = to_pll_clk(hw);
	u64 frac_val, fbdiv_val, refdiv_val;
	u32 val;
	u64 tmp, rate;

	val = readl_relaxed(clk->ctrl_reg1);
	val = val >> clk->frac_shift;
	val &= ((1 << clk->frac_width) - 1);
	frac_val = val;

	val = readl_relaxed(clk->ctrl_reg2);
	val = val >> clk->fbdiv_shift;
	val &= ((1 << clk->fbdiv_width) - 1);
	fbdiv_val = val;

	val = readl_relaxed(clk->ctrl_reg2);
	val = val >> clk->refdiv_shift;
	val &= ((1 << clk->refdiv_width) - 1) ;
	refdiv_val = val;

	/* rate = 24000000 * (fbdiv + frac / (1<<24) ) / refdiv  */
	rate = 0;
	tmp = 24000000 * fbdiv_val;
	rate += tmp;
	do_div(rate, refdiv_val);

	return rate;
}

static int __init clk_pll_determine_rate(struct clk_hw *hw,
		struct clk_rate_request *req)
{
	return req->rate;
}

static struct clk_ops clk_pll_ops __initdata = {
	.set_rate = clk_pll_set_rate,
	.determine_rate = clk_pll_determine_rate,
	.recalc_rate = clk_pll_recalc_rate,
};

static int __init hi3516a_clk_register_pll(struct hi3516a_pll_clock *clks,
		int nums, struct hisi_clock_data *data)
{
	void __iomem *base = data->base;
	int i;

	for (i = 0; i < nums; i++) {
		struct hi3516a_clk_pll *p_clk;
		struct clk *clk;
		struct clk_init_data init;

		p_clk = kzalloc(sizeof(*p_clk), GFP_KERNEL);
		if (!p_clk)
			return -1;

		init.name = clks[i].name;
		init.flags = CLK_IS_BASIC;
		init.parent_names =
			(clks[i].parent_name ? &clks[i].parent_name : NULL);
		init.num_parents = (clks[i].parent_name ? 1 : 0);
		init.ops = &clk_pll_ops;

		p_clk->ctrl_reg1 = base + clks[i].ctrl_reg1;
		p_clk->frac_shift = clks[i].frac_shift;
		p_clk->frac_width = clks[i].frac_width;
		p_clk->postdiv1_shift = clks[i].postdiv1_shift;
		p_clk->postdiv1_width = clks[i].postdiv1_width;
		p_clk->postdiv2_shift = clks[i].postdiv2_shift;
		p_clk->postdiv2_width = clks[i].postdiv2_width;

		p_clk->ctrl_reg2 = base + clks[i].ctrl_reg2;
		p_clk->fbdiv_shift = clks[i].fbdiv_shift;
		p_clk->fbdiv_width = clks[i].fbdiv_width;
		p_clk->refdiv_shift = clks[i].refdiv_shift;
		p_clk->refdiv_width = clks[i].refdiv_width;
		p_clk->hw.init = &init;

		clk = clk_register(NULL, &p_clk->hw);
		if (IS_ERR(clk)) {
			kfree(p_clk);
			pr_err("%s: failed to register clock %s\n",
					__func__, clks[i].name);
			continue;
		}

		data->clk_data.clks[clks[i].id] = clk;
	}

	return 0;
}

static void __init hi3516a_clk_crg_init(struct device_node *np)
{
	struct hisi_clock_data *clk_data;
	unsigned int count = 0;
	int ret;

	clk_data = hisi_clk_init(np, HI3516A_CRG_NR_CLKS);
	if (!clk_data) {
		pr_err("%s: failed to allocate CRG clock data\n", __func__);
		return;
	}

	ret = hisi_clk_register_fixed_rate(hi3516a_fixed_rate_clks_crg,
			ARRAY_SIZE(hi3516a_fixed_rate_clks_crg), clk_data);
	if (ret)
		goto err;

	ret = hi3516a_clk_register_pll(hi3516a_pll_clks,
			ARRAY_SIZE(hi3516a_pll_clks), clk_data);
	if (ret)
		goto unregister_fixed_rate;

	ret = hisi_clk_register_mux(hi3516a_mux_clks_crg,
			ARRAY_SIZE(hi3516a_mux_clks_crg), clk_data);
	if (ret)
		goto unregister_fixed_rate;

	ret = hisi_clk_register_fixed_factor(hi3516a_fixed_factor_clks,
			ARRAY_SIZE(hi3516a_fixed_factor_clks), clk_data);
	if (ret)
		goto unregister_mux;

	ret = hisi_clk_register_gate(hi3516a_gate_clks,
			ARRAY_SIZE(hi3516a_gate_clks), clk_data);
	if (ret)
		goto unregister_factor;

	ret = of_clk_add_provider(np,
			of_clk_src_onecell_get, &clk_data->clk_data);
	if (ret)
		goto unregister_gate;

	if (!of_property_read_u32(np, "#reset-cells", &count) && (count == 2))
		if (hibvt_reset_init(np, HI3516A_CRG_NR_RSTS))
			goto err;

	return;

unregister_gate:
	hisi_clk_unregister_gate(hi3516a_gate_clks,
			ARRAY_SIZE(hi3516a_gate_clks), clk_data);
unregister_factor:
	hisi_clk_unregister_fixed_factor(hi3516a_fixed_factor_clks,
			ARRAY_SIZE(hi3516a_fixed_factor_clks), clk_data);
unregister_mux:
	hisi_clk_unregister_mux(hi3516a_mux_clks_crg,
			ARRAY_SIZE(hi3516a_mux_clks_crg), clk_data);
unregister_fixed_rate:
	hisi_clk_unregister_fixed_rate(hi3516a_fixed_rate_clks_crg,
			ARRAY_SIZE(hi3516a_fixed_rate_clks_crg), clk_data);
err:
	pr_err("%s: failed to init CRG clock\n", __func__);
	return;
}
CLK_OF_DECLARE(hi3516a_clk_crg, "hisilicon,hi3516a-clock",
					hi3516a_clk_crg_init);

/* clock in system control */
static const char *const timer_mux_p[] __initconst = {"3m", "clk_sysapb"};
static u32 timer_mux_table[] __initdata = {0, 1};

static const struct hisi_mux_clock hi3516a_mux_clks_sc_clk[] __initconst = {
	{ HI3516A_TIME0_0_CLK, "timer00", timer_mux_p,
		    ARRAY_SIZE(timer_mux_p), CLK_SET_RATE_PARENT,
			    0x0, 16, 1, 0, timer_mux_table, },

	{ HI3516A_TIME0_1_CLK, "timer01", timer_mux_p,
		    ARRAY_SIZE(timer_mux_p), CLK_SET_RATE_PARENT,
			    0x0, 18, 1, 0, timer_mux_table, },

	{ HI3516A_TIME1_2_CLK, "timer12", timer_mux_p,
		    ARRAY_SIZE(timer_mux_p), CLK_SET_RATE_PARENT,
			    0x0, 20, 1, 0, timer_mux_table, },

	{ HI3516A_TIME1_3_CLK, "timer13", timer_mux_p,
		    ARRAY_SIZE(timer_mux_p), CLK_SET_RATE_PARENT,
			    0x0, 22, 1, 0, timer_mux_table, },
};

static void __init hi3516a_sc_clk_init(struct device_node *np)
{
	struct hisi_clock_data *clk_data;
	int ret;

	clk_data = hisi_clk_init(np, HI3516A_SYS_NR_CLKS);
	if (!clk_data) {
		pr_err("%s: failed to allocate %s clock data\n",
				__func__, np->name);
		return;
	}

	ret = hisi_clk_register_mux(hi3516a_mux_clks_sc_clk,
			ARRAY_SIZE(hi3516a_mux_clks_sc_clk), clk_data);
	if (ret) {
		pr_err("%s: failed to register %s mux clock\n",
				__func__, np->name);
		return;
	}
}

CLK_OF_DECLARE(hi3516a_clk_sysctrl, "hisilicon,sysctrl",
		hi3516a_sc_clk_init);

