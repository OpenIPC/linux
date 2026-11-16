// SPDX-License-Identifier: GPL-2.0
/*
 * Ingenic T23 DDR windows.
 *
 * Boot has already programmed timing. This driver only maps the
 * controller, PHY, and APB ranges from the device tree.
 * Copyright (C) 2026 Difan Zhang
 */

#include <linux/io.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/platform_device.h>

static const char * const ingenic_t23_ddr_windows[] = {
	"controller",
	"phy",
	"apb",
};

struct ingenic_t23_ddr {
	void __iomem *base[ARRAY_SIZE(ingenic_t23_ddr_windows)];
};

static int ingenic_t23_ddr_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct ingenic_t23_ddr *ddr;
	int i;

	ddr = devm_kzalloc(dev, sizeof(*ddr), GFP_KERNEL);
	if (!ddr)
		return -ENOMEM;

	for (i = 0; i < ARRAY_SIZE(ingenic_t23_ddr_windows); i++) {
		struct resource *res;

		res = platform_get_resource_byname(pdev, IORESOURCE_MEM,
						   ingenic_t23_ddr_windows[i]);
		ddr->base[i] = devm_ioremap_resource(dev, res);
		if (IS_ERR(ddr->base[i]))
			return PTR_ERR(ddr->base[i]);
	}

	platform_set_drvdata(pdev, ddr);
	dev_info(dev, "mapped controller, phy and apb\n");
	return 0;
}

static const struct of_device_id ingenic_t23_ddr_of_match[] = {
	{ .compatible = "ingenic,t23-ddr" },
	{ }
};
MODULE_DEVICE_TABLE(of, ingenic_t23_ddr_of_match);

static struct platform_driver ingenic_t23_ddr_driver = {
	.probe = ingenic_t23_ddr_probe,
	.driver = {
		.name = "ingenic-t23-ddr",
		.of_match_table = ingenic_t23_ddr_of_match,
	},
};
module_platform_driver(ingenic_t23_ddr_driver);

MODULE_AUTHOR("Difan Zhang");
MODULE_DESCRIPTION("Ingenic T23 DDR window map");
MODULE_LICENSE("GPL");
