/*
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 */

#include <linux/init.h>
#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/regmap.h>
#include <linux/suspend.h>
#include <linux/mfd/syscon.h>
#include <asm/cacheflush.h>
#include <asm/tlbflush.h>
#include <asm/suspend.h>

#include "ax620e_pm.h"

struct axera_chip_pm_data {
	const struct platform_suspend_ops *ops;
	int (*init)(struct device_node *np);
};

static const struct platform_suspend_ops ax620e_suspend_ops = {
	.enter   = ax620e_suspend_enter,
	.valid   = suspend_valid_only_mem,
	.prepare = ax620e_suspend_prepare,
	.finish  = ax620e_suspend_finish,
};

static const struct axera_chip_pm_data ax620e_pm_data = {
	.ops = &ax620e_suspend_ops,
	.init = ax620e_suspend_init,
};

static const struct of_device_id axera_chip_pmu_of_device_ids[] __initconst = {
	{
		.compatible = "axera,ax620e-pmu",
		.data = &ax620e_pm_data,
	},
	{ /* sentinel */ },
};

void __init axera_chip_pm_init(void)
{
	const struct axera_chip_pm_data *pm_data;
	const struct of_device_id *match;
	struct device_node *np;
	int ret;

	np = of_find_matching_node_and_match(NULL, axera_chip_pmu_of_device_ids,
					     &match);
	if (!match) {
		pr_err("Failed to find PMU node\n");
		return;
	}
	pm_data = (struct axera_chip_pm_data *) match->data;

	if (pm_data->init) {
		ret = pm_data->init(np);

		if (ret) {
			pr_err("%s: matches init error %d\n", __func__, ret);
			return;
		}
	}

	suspend_set_ops(pm_data->ops);
}
