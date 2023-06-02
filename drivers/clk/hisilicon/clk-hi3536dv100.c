/*
 * Hi3536DV100 Clock Driver
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

#include <dt-bindings/clock/hi3536dv100-clock.h>
#include <linux/clk-provider.h>
#include <linux/module.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include "clk.h"
#include "crg.h"
#include "reset.h"

static const struct
hisi_fixed_rate_clock hi3536dv100_fixed_rate_clks_crg[] = {
	{ HI3536DV100_FIXED_3M,		"3m",	NULL, 0, 3000000, },
	{ HI3536DV100_FIXED_6M,		"6m",	NULL, 0, 6000000, },
	{ HI3536DV100_FIXED_12M,	"12m",	NULL, 0, 12000000, },
	{ HI3536DV100_FIXED_24M,	"24m",	NULL, 0, 24000000, },
	{ HI3536DV100_FIXED_50M,	"50m",	NULL, 0, 50000000, },
	{ HI3536DV100_FIXED_83P3M,	"83.3m",NULL, 0, 83300000, },
	{ HI3536DV100_FIXED_100M,	"100m", NULL, 0, 100000000, },
	{ HI3536DV100_FIXED_125M,	"125m", NULL, 0, 125000000, },
	{ HI3536DV100_FIXED_150M,	"150m", NULL, 0, 150000000, },
	{ HI3536DV100_FIXED_200M,	"200m", NULL, 0, 200000000, },
	{ HI3536DV100_FIXED_250M,	"250m", NULL, 0, 250000000, },
	{ HI3536DV100_FIXED_300M,	"300m", NULL, 0, 300000000, },
	{ HI3536DV100_FIXED_324M,	"324m", NULL, 0, 324000000, },
	{ HI3536DV100_FIXED_342M,	"342m", NULL, 0, 342000000, },
	{ HI3536DV100_FIXED_342M,	"375m", NULL, 0, 375000000, },
	{ HI3536DV100_FIXED_400M,	"400m", NULL, 0, 400000000, },
	{ HI3536DV100_FIXED_448M,	"448m", NULL, 0, 448000000, },
	{ HI3536DV100_FIXED_500M,	"500m", NULL, 0, 500000000, },
	{ HI3536DV100_FIXED_540M,	"540m", NULL, 0, 540000000, },
	{ HI3536DV100_FIXED_600M,	"600m", NULL, 0, 600000000, },
	{ HI3536DV100_FIXED_750M,	"750m",	NULL, 0, 750000000, },
	{ HI3536DV100_FIXED_1500M,	"1500m",NULL, 0, 1500000000UL, },
};

static const char *sysaxi_mux_p[] __initconst = {
	"24m", "250m", "200m", "300m"};
static const char *sysapb_mux_p[] __initconst = {"24m", "50m"};
static const char *uart_mux_p[] __initconst = {"sysapb_mux", "24m", "2m"};
static const char *fmc_mux_p[] __initconst = {"24m", "83.3m", "150m"};

static u32 sysaxi_mux_table[] = {0, 1, 2, 3};
static u32 sysapb_mux_table[] = {0, 1};
static u32 uart_mux_table[] = {0, 1, 2};
static u32 fmc_mux_table[] = {0, 1, 2};

static struct hisi_mux_clock hi3536dv100_mux_clks_crg[] __initdata = {
	{ HI3536DV100_SYSAXI_CLK, "sysaxi_mux", sysaxi_mux_p,
		ARRAY_SIZE(sysaxi_mux_p),
		CLK_SET_RATE_PARENT, 0x50, 2, 2, 0, sysaxi_mux_table, },
	{ HI3536DV100_SYSAPB_CLK, "sysapb_mux", sysapb_mux_p,
		ARRAY_SIZE(sysapb_mux_p),
		CLK_SET_RATE_PARENT, 0x50, 0, 1, 0, sysapb_mux_table, },
	{ HI3536DV100_FMC_MUX, "fmc_mux", fmc_mux_p, ARRAY_SIZE(fmc_mux_p),
		CLK_SET_RATE_PARENT, 0xc0, 2, 2, 0, fmc_mux_table, },
	{ HI3536DV100_UART_MUX, "uart_mux", uart_mux_p,
		ARRAY_SIZE(uart_mux_p),
		CLK_SET_RATE_PARENT, 0xcc, 18, 2, 0, uart_mux_table, },
};

static struct hisi_fixed_factor_clock
				hi3536dv100_fixed_factor_clks[] __initdata = {
	{ HI3536DV100_SYSAXI_CLK, "clk_sysaxi", "sysaxi_mux", 1, 4,
		CLK_SET_RATE_PARENT},
};

static struct hisi_gate_clock hi3536dv100_gate_clks[] __initdata = {
	/* fmc */
	{ HI3536DV100_FMC_CLK, "clk_fmc", "fmc_mux",
		CLK_SET_RATE_PARENT, 0xc0, 1, 0, },
	/* uart */
	{ HI3536DV100_UART0_CLK, "clk_uart0", "24m",
		CLK_SET_RATE_PARENT, 0xcc, 15, 0, },
	{ HI3536DV100_UART1_CLK, "clk_uart1", "24m",
		CLK_SET_RATE_PARENT, 0xcc, 16, 0, },
	{ HI3536DV100_UART2_CLK, "clk_uart2", "24m",
		CLK_SET_RATE_PARENT, 0xcc, 17, 0, },
	/* ethernet mac */
	{ HI3536DV100_ETH0_CLK, "clk_eth0", NULL,
		CLK_SET_RATE_PARENT, 0xc4, 1, 0, },
	{ HI3536DV100_ETH0_PHY_CLK, "clk_eth0_phy", NULL,
		CLK_SET_RATE_PARENT, 0xc4, 10, 0, },
	{ HI3536DV100_DMAC_CLK, "clk_dmac", "50m",
		CLK_SET_RATE_PARENT, 0xc8, 5, 0, },
};

static struct hisi_clock_data *hi3536dv100_clk_register(
		struct platform_device *pdev)
{
	struct hisi_clock_data *clk_data;
	int ret;

	clk_data = hisi_clk_alloc(pdev, HI3536DV100_CRG_NR_CLKS);
	if (!clk_data)
		return ERR_PTR(-ENOMEM);

	ret = hisi_clk_register_fixed_rate(hi3536dv100_fixed_rate_clks_crg,
			ARRAY_SIZE(hi3536dv100_fixed_rate_clks_crg), clk_data);
	if (ret)
		return ERR_PTR(ret);

	ret = hisi_clk_register_mux(hi3536dv100_mux_clks_crg,
			ARRAY_SIZE(hi3536dv100_mux_clks_crg), clk_data);
	if (ret)
		goto unregister_fixed_rate;

	ret = hisi_clk_register_fixed_factor(hi3536dv100_fixed_factor_clks,
			ARRAY_SIZE(hi3536dv100_fixed_factor_clks), clk_data);
	if (ret)
		goto unregister_mux;

	ret = hisi_clk_register_gate(hi3536dv100_gate_clks,
			ARRAY_SIZE(hi3536dv100_gate_clks), clk_data);
	if (ret)
		goto unregister_factor;

	ret = of_clk_add_provider(pdev->dev.of_node,
			of_clk_src_onecell_get, &clk_data->clk_data);
	if (ret)
		goto unregister_gate;

	return clk_data;

unregister_gate:
	hisi_clk_unregister_gate(hi3536dv100_gate_clks,
			ARRAY_SIZE(hi3536dv100_gate_clks), clk_data);
unregister_factor:
	hisi_clk_unregister_fixed_factor(hi3536dv100_fixed_factor_clks,
			ARRAY_SIZE(hi3536dv100_fixed_factor_clks), clk_data);
unregister_mux:
	hisi_clk_unregister_mux(hi3536dv100_mux_clks_crg,
			ARRAY_SIZE(hi3536dv100_mux_clks_crg), clk_data);
unregister_fixed_rate:
	hisi_clk_unregister_fixed_rate(hi3536dv100_fixed_rate_clks_crg,
			ARRAY_SIZE(hi3536dv100_fixed_rate_clks_crg), clk_data);
	return ERR_PTR(ret);
}

static void hi3536dv100_clk_unregister(struct platform_device *pdev)
{
	struct hisi_crg_dev *crg = platform_get_drvdata(pdev);

	of_clk_del_provider(pdev->dev.of_node);

	hisi_clk_unregister_gate(hi3536dv100_gate_clks,
			ARRAY_SIZE(hi3536dv100_gate_clks), crg->clk_data);
	hisi_clk_unregister_mux(hi3536dv100_mux_clks_crg,
			ARRAY_SIZE(hi3536dv100_mux_clks_crg), crg->clk_data);
	hisi_clk_unregister_fixed_factor(hi3536dv100_fixed_factor_clks,
			ARRAY_SIZE(hi3536dv100_fixed_factor_clks), crg->clk_data);
	hisi_clk_unregister_fixed_rate(hi3536dv100_fixed_rate_clks_crg,
			ARRAY_SIZE(hi3536dv100_fixed_rate_clks_crg), crg->clk_data);
}

static const struct hisi_crg_funcs hi3536dv100_crg_funcs = {
	.register_clks = hi3536dv100_clk_register,
	.unregister_clks = hi3536dv100_clk_unregister,
};


static const struct of_device_id hi3536dv100_crg_match_table[] = {
	{
		.compatible = "hisilicon,hi3536dv100-clock",
		.data = &hi3536dv100_crg_funcs
	},
	{ }
};
MODULE_DEVICE_TABLE(of, hi3536dv100_crg_match_table);

static int hi3536dv100_crg_probe(struct platform_device *pdev)
{
	struct hisi_crg_dev *crg;

	crg = devm_kmalloc(&pdev->dev, sizeof(*crg), GFP_KERNEL);
	if (!crg)
		return -ENOMEM;

	crg->funcs = of_device_get_match_data(&pdev->dev);
	if (!crg->funcs)
		return -ENOENT;

	crg->rstc = hisi_reset_init(pdev);
	if (!crg->rstc)
		return -ENOMEM;

	crg->clk_data = crg->funcs->register_clks(pdev);
	if (IS_ERR(crg->clk_data)) {
		hisi_reset_exit(crg->rstc);
		return PTR_ERR(crg->clk_data);
	}

	platform_set_drvdata(pdev, crg);
	return 0;
}

static int hi3536dv100_crg_remove(struct platform_device *pdev)
{
	struct hisi_crg_dev *crg = platform_get_drvdata(pdev);

	hisi_reset_exit(crg->rstc);
	crg->funcs->unregister_clks(pdev);
	return 0;
}

static struct platform_driver hi3536dv100_crg_driver = {
	.probe          = hi3536dv100_crg_probe,
	.remove     = hi3536dv100_crg_remove,
	.driver         = {
		.name   = "hi3536dv100-clock",
		.of_match_table = hi3536dv100_crg_match_table,
	},
};

static int __init hi3536dv100_crg_init(void)
{
	return platform_driver_register(&hi3536dv100_crg_driver);
}
core_initcall(hi3536dv100_crg_init);

static void __exit hi3536dv100_crg_exit(void)
{
	platform_driver_unregister(&hi3536dv100_crg_driver);
}
module_exit(hi3536dv100_crg_exit);

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("HiSilicon Hi3536DV100 CRG Driver");
