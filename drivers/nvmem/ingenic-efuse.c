// SPDX-License-Identifier: GPL-2.0
/*
 * Ingenic T23 EFUSE nvmem.
 *
 * CLKGR bit 1 gates this block. A read while gated can hang the SoC, so
 * the clock is enabled only around the access. The registers are words;
 * each requested byte is taken from the little-endian word.
 * Copyright (C) 2026 Difan Zhang
 */

#include <linux/clk.h>
#include <linux/device.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/nvmem-provider.h>
#include <linux/of.h>
#include <linux/platform_device.h>

struct ingenic_efuse {
	void __iomem *base;
	struct clk *clk;
};

static int ingenic_efuse_read(void *context, unsigned int offset, void *val,
			      size_t bytes)
{
	struct ingenic_efuse *priv = context;
	u8 *out = val;
	int ret;

	ret = clk_prepare_enable(priv->clk);
	if (ret)
		return ret;

	while (bytes--) {
		u32 word = readl(priv->base + (offset & ~3U));

		*out++ = word >> ((offset & 3) * 8);
		offset++;
	}

	clk_disable_unprepare(priv->clk);
	return 0;
}

static int ingenic_efuse_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct nvmem_config config = { };
	struct ingenic_efuse *priv;
	struct nvmem_device *nvmem;
	struct resource *res;

	priv = devm_kzalloc(dev, sizeof(*priv), GFP_KERNEL);
	if (!priv)
		return -ENOMEM;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	priv->base = devm_ioremap_resource(dev, res);
	if (IS_ERR(priv->base))
		return PTR_ERR(priv->base);

	priv->clk = devm_clk_get(dev, "efuse");
	if (IS_ERR(priv->clk))
		return PTR_ERR(priv->clk);

	config.dev = dev;
	config.priv = priv;
	config.size = resource_size(res);
	config.stride = 1;
	config.word_size = 1;
	config.read_only = true;
	config.root_only = true;
	config.reg_read = ingenic_efuse_read;
	config.owner = THIS_MODULE;

	nvmem = devm_nvmem_register(dev, &config);
	return PTR_ERR_OR_ZERO(nvmem);
}

static const struct of_device_id ingenic_efuse_of_match[] = {
	{ .compatible = "ingenic,t23-efuse" },
	{ }
};
MODULE_DEVICE_TABLE(of, ingenic_efuse_of_match);

static struct platform_driver ingenic_efuse_driver = {
	.probe = ingenic_efuse_probe,
	.driver = {
		.name = "ingenic-efuse",
		.of_match_table = ingenic_efuse_of_match,
	},
};
module_platform_driver(ingenic_efuse_driver);

MODULE_AUTHOR("Difan Zhang");
MODULE_DESCRIPTION("Ingenic T23 EFUSE nvmem");
MODULE_LICENSE("GPL");
