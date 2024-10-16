/*
 * PMC driver for the X-Powers' Power Management ICs
 *
 * SUNXI_PMC typically comprises an adaptive USB-Compatible PWM charger, BUCK DC-DC
 * converters, LDOs, multiple 12-bit ADCs of voltage, current and temperature
 * as well as configurable GPIOs.
 *
 * This driver supports the PMC variants.
 *
 * Copyright (C) 2015 Chen-Yu Tsai
 *
 * Author: Chen-Yu Tsai <wens@csie.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/acpi.h>
#include <linux/err.h>
#include <linux/mfd/sunxi_pmc.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/regmap.h>
#include <linux/slab.h>
#include <linux/platform_device.h>
#include <linux/of_irq.h>

static int sunxi_pmc_pmc_probe(struct platform_device *rdev)
{
	struct sunxi_pmc_dev *sunxi_pmc;
	struct device *dev = &rdev->dev;
	struct platform_device *pdev = rdev;
	struct resource *mem;
	void __iomem *base;
	int ret;

	mem = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	base = devm_ioremap_resource(dev, mem);
	if (IS_ERR(base)) {
		return PTR_ERR(base);
	}

	sunxi_pmc = devm_kzalloc(&rdev->dev, sizeof(*sunxi_pmc), GFP_KERNEL);
	if (!sunxi_pmc)
		return -ENOMEM;

	sunxi_pmc->dev = &rdev->dev;
	if (rdev->dev.of_node) {
		sunxi_pmc->irq = of_irq_get(rdev->dev.of_node, 0);
	}

	dev_set_drvdata(&rdev->dev, sunxi_pmc);

	ret = sunxi_pmc_match_device(sunxi_pmc);
	if (ret)
		return ret;

	sunxi_pmc->regmap = devm_regmap_init_mmio(dev, base, sunxi_pmc->regmap_cfg);
	if (IS_ERR(sunxi_pmc->regmap)) {
		ret = PTR_ERR(sunxi_pmc->regmap);
		dev_err(&rdev->dev, "regmap init failed: %d\n", ret);
		return ret;
	}

	return sunxi_pmc_device_probe(sunxi_pmc);
}

static int sunxi_pmc_pmc_remove(struct platform_device *rdev)
{
	struct sunxi_pmc_dev *sunxi_pmc = dev_get_drvdata(&rdev->dev);

	return sunxi_pmc_device_remove(sunxi_pmc);
}

static const struct of_device_id sunxi_pmc_pmc_of_match[] = {
	{ .compatible = "x-powers,pmc", .data = (void *)PMC_V1_ID },
	{ },
};
MODULE_DEVICE_TABLE(of, sunxi_pmc_pmc_of_match);

static struct platform_driver sunxi_pmc_pmc_driver = {
	.driver = {
		.name	= "sunxi_pmc",
		.of_match_table	= of_match_ptr(sunxi_pmc_pmc_of_match),
	},
	.probe	= sunxi_pmc_pmc_probe,
	.remove	= sunxi_pmc_pmc_remove,
};

static int __init sunxi_pmc_pmc_init(void)
{
	return platform_driver_register(&sunxi_pmc_pmc_driver);
}

static void __exit sunxi_pmc_pmc_exit(void)
{
	platform_driver_unregister(&sunxi_pmc_pmc_driver);
}
subsys_initcall(sunxi_pmc_pmc_init);
module_exit(sunxi_pmc_pmc_exit);

MODULE_DESCRIPTION("PMIC MFD SUNXI PMC driver for SUNXI_PMC");
MODULE_AUTHOR("Chen-Yu Tsai <wens@csie.org>");
MODULE_LICENSE("GPL v2");
