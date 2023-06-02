/**
    NVT pll preset function
    This file will preset PLL value
    @file       pll_preset.c
    @ingroup
    @note
    Copyright   Novatek Microelectronics Corp. 2016.  All rights reserved.

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2 as
    published by the Free Software Foundation.
*/
#include <linux/clk.h>
#include <linux/of.h>
#include <linux/clk-provider.h>
#include <plat/nvt-sramctl.h>

static void disable_cnn(void)
{
	struct clk* turnoff_clk;

	turnoff_clk = clk_get(NULL, "f0d60000.ai");
	if (!IS_ERR(turnoff_clk)) {
		clk_prepare_enable(turnoff_clk);
		clk_disable_unprepare(turnoff_clk);
		clk_set_phase(turnoff_clk, 1);
		nvt_enable_sram_shutdown(CNN2_SD);
		clk_put(turnoff_clk);
	} else
		pr_err("*** %s Get CNN2 Clock error\n", __func__);

	turnoff_clk = clk_get(NULL, "f0cb0000.ai");
	if (!IS_ERR(turnoff_clk)) {
		clk_prepare_enable(turnoff_clk);
		clk_disable_unprepare(turnoff_clk);
		nvt_enable_sram_shutdown(CNN_SD);
		clk_put(turnoff_clk);
	} else
		pr_err("*** %s Get CNN Clock error\n", __func__);

	turnoff_clk = clk_get(NULL, "f0d50000.ai");
	if (!IS_ERR(turnoff_clk)) {
		clk_prepare_enable(turnoff_clk);
		clk_disable_unprepare(turnoff_clk);
		nvt_enable_sram_shutdown(NUE2_SD);
		clk_put(turnoff_clk);
	} else
		pr_err("*** %s Get NUE2 Clock error\n", __func__);

	turnoff_clk = clk_get(NULL, "f0c60000.ai");
	if (!IS_ERR(turnoff_clk)) {
		clk_prepare_enable(turnoff_clk);
		clk_disable_unprepare(turnoff_clk);
		nvt_enable_sram_shutdown(NUE_SD);
		clk_put(turnoff_clk);
	} else
		pr_err("*** %s Get NUE Clock error\n", __func__);
}

static int __init nvt_preset_pll(void)
{
	struct device_node *node, *pll;
	struct clk* pll_clk;
	char pll_name[10] = {};
	u32 value[3] = {};

	node = of_find_node_by_path("/pll_preset@0");
	if (node) {
		for_each_child_of_node(node, pll) {
			if (!of_property_read_u32_array(pll, "pll_config", value, 3)) {
				if (value[1]) {
					if (value[0] == 24)
						snprintf(pll_name, 10, "pllf320");
					else
						snprintf(pll_name, 10, "pll%d", value[0]);

					pll_clk = clk_get(NULL, pll_name);
					if (!IS_ERR(pll_clk))
						clk_set_rate(pll_clk, value[1]);
					else
						pr_err("*** %s Get PLL%d Clock error\n", __func__, value[0]);
				}

				if (value[2]) {
					if (value[0] == 24)
						snprintf(pll_name, 10, "pllf320");
					else
						snprintf(pll_name, 10, "pll%d", value[0]);

					pll_clk = clk_get(NULL, pll_name);
					if (!IS_ERR(pll_clk)) {
						if (!__clk_is_enabled(pll_clk))
							clk_prepare_enable(pll_clk);
					} else
						pr_err("*** %s Get PLL%d Clock error\n", __func__, value[0]);
				}
			}
		}
	} else
		pr_err("*** %s not get dts node ***\n", __func__);

	disable_cnn();

	return 0;
}

arch_initcall_sync(nvt_preset_pll);
