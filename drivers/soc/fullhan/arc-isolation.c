// SPDX-License-Identifier: GPL-2.0
/*
 * Fullhan FH8626 ARC coprocessor isolation
 *
 * Copyright (C) 2026 Difan Zhang
 *
 * Holds the ARC companion in reset and gates its clock. Reset is active
 * low. Setting the clock gate stops the clock.
 */

#include <linux/bitops.h>
#include <linux/device.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/soc/fullhan/fh8626-pmu.h>

#define FH8626_PMU_REG_CLK_GATE		0x01c
#define FH8626_PMU_BIT_ARC_CLK_GATE	BIT(2)

#define FH8626_PMU_REG_ARC_RESET	0x1c4
#define FH8626_PMU_BIT_ARC_RESET	BIT(18)

static int fh8626_arc_isolation_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	int ret;

	/* Holding the ARC in reset stops all of its memory bus activity. */
	ret = fh8626_pmu_update_bits(FH8626_PMU_REG_ARC_RESET,
				     FH8626_PMU_BIT_ARC_RESET, 0);
	if (ret) {
		if (ret != -EPROBE_DEFER)
			dev_err(dev, "failed to assert ARC reset: %d\n", ret);
		return ret;
	}

	/*
	 * arc_clk is registered with CLK_IGNORE_UNUSED, so the clock framework
	 * never gates it; gate it here directly.
	 */
	ret = fh8626_pmu_update_bits(FH8626_PMU_REG_CLK_GATE,
				     FH8626_PMU_BIT_ARC_CLK_GATE,
				     FH8626_PMU_BIT_ARC_CLK_GATE);
	if (ret) {
		dev_err(dev, "failed to gate ARC clock: %d\n", ret);
		return ret;
	}

	return 0;
}

static const struct of_device_id fh8626_arc_of_match[] = {
	{ .compatible = "fullhan,fh8626-arc-isolation" },
	{ .compatible = "fullhan,fh8626v100-arc" },
	{ }
};
MODULE_DEVICE_TABLE(of, fh8626_arc_of_match);

static struct platform_driver fh8626_arc_isolation_driver = {
	.probe = fh8626_arc_isolation_probe,
	.driver = {
		.name = "fh8626-arc-isolation",
		.of_match_table = fh8626_arc_of_match,
	},
};
module_platform_driver(fh8626_arc_isolation_driver);

MODULE_DESCRIPTION("Fullhan FH8626 ARC hardware isolation driver");
MODULE_AUTHOR("Difan Zhang");
MODULE_LICENSE("GPL v2");
