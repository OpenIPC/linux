// SPDX-License-Identifier: GPL-2.0
/*
 * Fullhan FH8626V100 MIPI input routing and PHY power control.
 * Copyright (C) 2026 Difan Zhang
 * Licensed under the GNU General Public License version 2.
 */
#include <linux/bitops.h>
#include <linux/err.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/phy/phy.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/soc/fullhan/fh8626-pmu.h>

#define FH_MIPI_SYS_CTRL		0x000c
#define FH_MIPI_SYS_ENABLE	BIT(16)
#define FH_MIPI_CLK_GATE		0x0024
#define FH_MIPI_PIX_GATE		BIT(17)
#define FH_MIPI_CLK_SELECT	0x0028
#define FH_MIPI_ROUTE		GENMASK(6, 5)
#define FH_MIPI_ENABLE		0x2040
#define FH_MIPI_ENABLE_BIT	BIT(0)

struct fh8626_mipi_phy {
	u32 sys_ctrl;
	u32 clk_gate;
	u32 clk_select;
	u32 enable;
};

static int fh8626_mipi_init(struct phy *phy)
{
	struct fh8626_mipi_phy *priv = phy_get_drvdata(phy);
	int ret;

	ret = fh8626_pmu_read(FH_MIPI_SYS_CTRL, &priv->sys_ctrl);
	if (ret)
		return ret;
	ret = fh8626_pmu_read(FH_MIPI_CLK_GATE, &priv->clk_gate);
	if (ret)
		return ret;
	ret = fh8626_pmu_read(FH_MIPI_CLK_SELECT, &priv->clk_select);
	if (ret)
		return ret;
	ret = fh8626_pmu_read(FH_MIPI_ENABLE, &priv->enable);
	if (ret)
		return ret;

	return fh8626_pmu_update_bits(FH_MIPI_SYS_CTRL, FH_MIPI_SYS_ENABLE,
				      FH_MIPI_SYS_ENABLE);
}

static int fh8626_mipi_power_off(struct phy *phy)
{
	struct fh8626_mipi_phy *priv = phy_get_drvdata(phy);
	int ret, err;

	ret = fh8626_pmu_update_bits(FH_MIPI_ENABLE, FH_MIPI_ENABLE_BIT,
				     priv->enable);
	err = fh8626_pmu_update_bits(FH_MIPI_CLK_GATE, FH_MIPI_PIX_GATE,
				     priv->clk_gate);
	if (!ret)
		ret = err;
	err = fh8626_pmu_update_bits(FH_MIPI_CLK_SELECT, FH_MIPI_ROUTE,
				     priv->clk_select);
	return ret ? ret : err;
}

static int fh8626_mipi_power_on(struct phy *phy)
{
	int ret;

	ret = fh8626_pmu_update_bits(FH_MIPI_CLK_SELECT, FH_MIPI_ROUTE,
				     FH_MIPI_ROUTE);
	if (ret)
		return ret;
	/* The ISP's pixel input gate is active low. */
	ret = fh8626_pmu_update_bits(FH_MIPI_CLK_GATE, FH_MIPI_PIX_GATE, 0);
	if (ret)
		goto restore;
	ret = fh8626_pmu_update_bits(FH_MIPI_ENABLE, FH_MIPI_ENABLE_BIT,
				     FH_MIPI_ENABLE_BIT);
	if (!ret)
		return 0;
restore:
	fh8626_mipi_power_off(phy);
	return ret;
}

static int fh8626_mipi_exit(struct phy *phy)
{
	struct fh8626_mipi_phy *priv = phy_get_drvdata(phy);

	return fh8626_pmu_update_bits(FH_MIPI_SYS_CTRL, FH_MIPI_SYS_ENABLE,
				      priv->sys_ctrl);
}

static const struct phy_ops fh8626_mipi_ops = {
	.init = fh8626_mipi_init,
	.exit = fh8626_mipi_exit,
	.power_on = fh8626_mipi_power_on,
	.power_off = fh8626_mipi_power_off,
	.owner = THIS_MODULE,
};

static int fh8626_mipi_probe(struct platform_device *pdev)
{
	struct phy_provider *provider;
	struct fh8626_mipi_phy *priv;
	struct device_node *parent;
	struct phy *phy;
	u32 value;
	int ret;

	parent = of_get_parent(pdev->dev.of_node);
	ret = of_device_is_compatible(parent, "fullhan,fh8626v100-pmu");
	of_node_put(parent);
	if (!ret)
		return -EINVAL;
	ret = fh8626_pmu_read(FH_MIPI_ENABLE, &value);
	if (ret)
		return ret;
	priv = devm_kzalloc(&pdev->dev, sizeof(*priv), GFP_KERNEL);
	if (!priv)
		return -ENOMEM;
	phy = devm_phy_create(&pdev->dev, NULL, &fh8626_mipi_ops);
	if (IS_ERR(phy))
		return PTR_ERR(phy);
	phy_set_drvdata(phy, priv);
	provider = devm_of_phy_provider_register(&pdev->dev,
						 of_phy_simple_xlate);
	return PTR_ERR_OR_ZERO(provider);
}

static const struct of_device_id fh8626_mipi_match[] = {
	{ .compatible = "fullhan,fh8626v100-mipi-phy" },
	{ }
};
MODULE_DEVICE_TABLE(of, fh8626_mipi_match);

static struct platform_driver fh8626_mipi_driver = {
	.probe = fh8626_mipi_probe,
	.driver = {
		.name = "fh8626-mipi-phy",
		.of_match_table = fh8626_mipi_match,
		.suppress_bind_attrs = true,
	},
};
module_platform_driver(fh8626_mipi_driver);

MODULE_DESCRIPTION("Fullhan FH8626V100 MIPI input PHY");
MODULE_LICENSE("GPL v2");
