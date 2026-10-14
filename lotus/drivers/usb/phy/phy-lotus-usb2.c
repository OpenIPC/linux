// SPDX-License-Identifier: GPL-2.0
#include <linux/clk-provider.h>
#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/module.h>
#include <linux/of_address.h>
#include <linux/phy/phy.h>
#include <linux/platform_device.h>
#include <linux/reset.h>
#include <linux/usb/ch9.h>
#include <linux/printk.h>

#include "phy-lotus-usb.h"

static int lotus_usb_iomap(struct device_node *np,
		  struct lotus_usb_priv *priv)
{
	if ((np == NULL) || (priv == NULL))
		return -EINVAL;

	priv->crg_base = of_iomap(np, 0);
	if (IS_ERR(priv->crg_base)) {
		return -ENOMEM;
	}

	priv->misc_base = of_iomap(np, 1);
	if (IS_ERR(priv->misc_base)) {
		iounmap(priv->crg_base);
		return -ENOMEM;
	}

	priv->phy_base = of_iomap(np, 2);
	if (IS_ERR(priv->phy_base)) {
		iounmap(priv->crg_base);
		iounmap(priv->misc_base);
		return -ENOMEM;
	}

	return 0;
}

static const struct phy_ops lotus_usb2_phy_ops = {
	.owner = THIS_MODULE,
};

static int lotus_usb_phy_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct phy *phy = NULL;
	struct lotus_usb_priv *priv = NULL;
	struct device_node *np = pdev->dev.of_node;
	struct phy_provider *phy_provider = NULL;
	unsigned int ret;

	pr_debug("%s: enter\n", __func__);

	priv = devm_kzalloc(dev, sizeof(*priv), GFP_KERNEL);
	if (priv == NULL)
		return -ENOMEM;

	phy = devm_phy_create(dev, dev->of_node, &lotus_usb2_phy_ops);
	if (IS_ERR(phy))
		return PTR_ERR(phy);

	ret = lotus_usb_iomap(np, priv);
	if (ret) {
		devm_kfree(dev, priv);
		priv = NULL;

		return -ENOMEM;
	}

	platform_set_drvdata(pdev, priv);
	priv->dev = dev;

	phy_set_drvdata(phy, priv);

	lotus_chip_u2_phy_para_get(dev, priv);
	lotus_chip_u2_phy_config(priv);
	ret = lotus_u2_phy_clk_init(priv);
	if (ret) {
		ret = -EINVAL;
		goto xvp_unmap;
	}
	lotus_u2_phy_eye_config(priv);

	phy_provider = devm_of_phy_provider_register(dev, of_phy_simple_xlate);
	if (IS_ERR(phy_provider)) {
		ret = PTR_ERR(phy_provider);
		goto xvp_unmap;
	}
	pr_debug("%s: ok\n", __func__);
	return 0;

xvp_unmap:
	pr_err("%s err, ret %d\n", __func__, ret);

	iounmap(priv->crg_base);
	iounmap(priv->misc_base);
	iounmap(priv->phy_base);

	devm_kfree(dev, priv);
	priv = NULL;

	return ret;
}

static int lotus_usb_phy_remove(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct lotus_usb_priv *priv = platform_get_drvdata(pdev);

	lotus_u2_phy_clk_exit(priv);

	iounmap(priv->crg_base);
	iounmap(priv->misc_base);
	iounmap(priv->phy_base);

	devm_kfree(dev, priv);
	priv = NULL;

	return 0;
}

#ifdef CONFIG_PM_SLEEP
static int lotus_usb_phy_suspend(struct device *dev)
{
	struct lotus_usb_priv *priv = dev_get_drvdata(dev);
	int ret = 0;

	ret = lotus_u2_phy_clk_exit(priv);
	if (ret) {
		pr_err("%s: usb phy clk exit fail!\n", __func__);
		return ret;
	}

	return 0;
}

static int lotus_usb_phy_resume(struct device *dev)
{
	struct lotus_usb_priv *priv = dev_get_drvdata(dev);
	int ret = 0;

	lotus_chip_u2_phy_config(priv);
	ret = lotus_u2_phy_clk_init(priv);
	if (ret) {
		pr_err("%s: usb phy clk init fail!\n", __func__);
		return -1;
	}
	lotus_u2_phy_eye_config(priv);

	return 0;
}
#endif /* CONFIG_PM_SLEEP */

static SIMPLE_DEV_PM_OPS(lotus_usb_phy_pm_ops, lotus_usb_phy_suspend,
			lotus_usb_phy_resume);

static const struct of_device_id lotus_usb_phy_of_match[] = {
	{ .compatible = "lotus,usb2-phy" },
	{},
};

static struct platform_driver lotus_usb_phy_driver = {
	.probe = lotus_usb_phy_probe,
	.remove = lotus_usb_phy_remove,
	.driver = {
		.name = "lotus-usb-phy",
		.pm = &lotus_usb_phy_pm_ops,
		.of_match_table = lotus_usb_phy_of_match,
	}
};
module_platform_driver(lotus_usb_phy_driver);
MODULE_DESCRIPTION("LOTUS USB PHY driver");
MODULE_ALIAS("platform:usb-phy");
MODULE_LICENSE("GPL v2");
