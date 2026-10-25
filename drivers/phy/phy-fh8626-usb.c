// SPDX-License-Identifier: GPL-2.0
/* Fullhan FH8626V100 integrated UTMI PHY and host power sequence. */
#include <linux/bitops.h>
#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/phy/phy.h>
#include <linux/platform_device.h>
#include <linux/regulator/consumer.h>
#include <linux/slab.h>
#include <linux/soc/fullhan/fh8626-pmu.h>

#define FH_USB_RESET		0x04c
#define FH_USB_RESET_N		BIT(19)
#define FH_USB_CTRL		0x210
#define FH_USB_CTRL_MASK		(BIT(31) | BIT(24) | BIT(4) | BIT(0))
#define FH_USB_TUNE		0x218
#define FH_USB_TUNE_MASK		GENMASK(2, 0)

struct fh8626_usb_phy {
	struct device *dev;
	struct phy *phy;
	struct clk *hclk;
	struct regulator *vbus;
	bool powered;
};

static int fh8626_usb_power_on(struct phy *phy)
{
	struct fh8626_usb_phy *priv = phy_get_drvdata(phy);
	u32 ctrl, tune, reset, value;
	int ret, err;

	ret = fh8626_pmu_read(FH_USB_CTRL, &ctrl);
	if (ret)
		return ret;
	ret = fh8626_pmu_read(FH_USB_TUNE, &tune);
	if (ret)
		return ret;
	ret = fh8626_pmu_read(FH_USB_RESET, &reset);
	if (ret)
		return ret;
	ret = clk_prepare_enable(priv->hclk);
	if (ret)
		return ret;
	ret = fh8626_pmu_update_bits(FH_USB_CTRL, BIT(31), 0);
	if (ret)
		goto restore;
	udelay(1000);
	ret = fh8626_pmu_update_bits(FH_USB_CTRL, BIT(0) | BIT(4),
				      BIT(0) | BIT(4));
	if (ret)
		goto restore;
	udelay(1000);
	ret = fh8626_pmu_update_bits(FH_USB_CTRL, BIT(0) | BIT(4), 0);
	if (ret)
		goto restore;
	ret = fh8626_pmu_update_bits(FH_USB_TUNE, FH_USB_TUNE_MASK,
				      FH_USB_TUNE_MASK);
	if (ret)
		goto restore;
	ret = fh8626_pmu_update_bits(FH_USB_RESET, FH_USB_RESET_N, 0);
	if (ret)
		goto restore;
	ret = fh8626_pmu_read(FH_USB_RESET, &value);
	if (ret)
		goto restore;
	udelay(1000);
	ret = fh8626_pmu_update_bits(FH_USB_RESET, FH_USB_RESET_N,
				      FH_USB_RESET_N);
	if (ret)
		goto restore;
	ret = fh8626_pmu_read(FH_USB_RESET, &value);
	if (ret)
		goto restore;
	msleep(20);
	ret = fh8626_pmu_update_bits(FH_USB_CTRL, BIT(24), BIT(24));
	if (ret)
		goto restore;
	udelay(1000);
	/* VBUS follows the PHY sequence and precedes the DWC2 core reset. */
	ret = regulator_enable(priv->vbus);
	if (ret)
		goto restore;
	priv->powered = true;
	return 0;

restore:
	err = fh8626_pmu_update_bits(FH_USB_RESET, FH_USB_RESET_N, reset);
	if (err)
		dev_err(priv->dev, "failed to restore USB reset: %d\n", err);
	err = fh8626_pmu_update_bits(FH_USB_TUNE, FH_USB_TUNE_MASK, tune);
	if (err)
		dev_err(priv->dev, "failed to restore PHY tuning: %d\n", err);
	err = fh8626_pmu_update_bits(FH_USB_CTRL, FH_USB_CTRL_MASK, ctrl);
	if (err)
		dev_err(priv->dev, "failed to restore PHY control: %d\n", err);
	clk_disable_unprepare(priv->hclk);
	return ret;
}

static int fh8626_usb_power_off(struct phy *phy)
{
	struct fh8626_usb_phy *priv = phy_get_drvdata(phy);
	int ret;

	ret = regulator_disable(priv->vbus);
	if (ret)
		return ret;
	clk_disable_unprepare(priv->hclk);
	priv->powered = false;
	return 0;
}

static const struct phy_ops fh8626_usb_ops = {
	.power_on = fh8626_usb_power_on,
	.power_off = fh8626_usb_power_off,
	.owner = THIS_MODULE,
};

static int fh8626_usb_prepare(struct device *dev)
{
	struct fh8626_usb_phy *priv = dev_get_drvdata(dev);
	int ret;

	/* The host cannot recover its live context after this PHY reset. */
	mutex_lock(&priv->phy->mutex);
	ret = priv->powered ? -EOPNOTSUPP : 0;
	mutex_unlock(&priv->phy->mutex);
	return ret;
}

static const struct dev_pm_ops fh8626_usb_pm_ops = {
	.prepare = fh8626_usb_prepare,
};

static int fh8626_usb_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct fh8626_usb_phy *priv;
	struct phy_provider *provider;
	struct device_node *parent;
	u32 value;
	int ret;

	parent = of_get_parent(dev->of_node);
	ret = of_device_is_compatible(parent, "fullhan,fh8626v100-pmu");
	of_node_put(parent);
	if (!ret)
		return -EINVAL;
	ret = fh8626_pmu_read(FH_USB_CTRL, &value);
	if (ret)
		return ret;
	priv = devm_kzalloc(dev, sizeof(*priv), GFP_KERNEL);
	if (!priv)
		return -ENOMEM;
	priv->dev = dev;
	priv->hclk = devm_clk_get(dev, "hclk");
	if (IS_ERR(priv->hclk))
		return PTR_ERR(priv->hclk);
	/* Require a real board supply instead of a dummy regulator. */
	if (!of_find_property(dev->of_node, "vbus-supply", NULL))
		return -EINVAL;
	priv->vbus = devm_regulator_get_optional(dev, "vbus");
	if (IS_ERR(priv->vbus))
		return PTR_ERR(priv->vbus);
	priv->phy = devm_phy_create(dev, NULL, &fh8626_usb_ops);
	if (IS_ERR(priv->phy))
		return PTR_ERR(priv->phy);
	phy_set_drvdata(priv->phy, priv);
	phy_set_bus_width(priv->phy, 16);
	platform_set_drvdata(pdev, priv);
	provider = devm_of_phy_provider_register(dev, of_phy_simple_xlate);
	return PTR_ERR_OR_ZERO(provider);
}

static const struct of_device_id fh8626_usb_match[] = {
	{ .compatible = "fullhan,fh8626v100-usb-phy" },
	{ }
};
MODULE_DEVICE_TABLE(of, fh8626_usb_match);

static struct platform_driver fh8626_usb_driver = {
	.probe = fh8626_usb_probe,
	.driver = {
		.name = "fh8626-usb-phy",
		.of_match_table = fh8626_usb_match,
		.pm = &fh8626_usb_pm_ops,
		.suppress_bind_attrs = true,
	},
};
module_platform_driver(fh8626_usb_driver);

MODULE_DESCRIPTION("Fullhan FH8626V100 integrated USB2 PHY");
MODULE_LICENSE("GPL");
