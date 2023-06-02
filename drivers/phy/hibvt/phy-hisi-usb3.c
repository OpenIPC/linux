/*
 * Copyright (c) 2016-2017 HiSilicon Technologies Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <linux/module.h>
#include <linux/phy/phy.h>
#include <linux/platform_device.h>
#include <linux/of_address.h>

#include "phy-hisi-usb.h"

static int hisi_usb3_phy_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct phy *phy;
	struct hisi_priv *priv;
	struct device_node *np = pdev->dev.of_node;

	priv = devm_kzalloc(dev, sizeof(*priv), GFP_KERNEL);
	if (!priv)
		return -ENOMEM;

	priv->peri_ctrl = of_iomap(np, 0);
	if (IS_ERR(priv->peri_ctrl))
		priv->peri_ctrl = NULL;

	priv->misc_ctrl = of_iomap(np, 1);
	if (IS_ERR(priv->misc_ctrl))
		priv->misc_ctrl = NULL;

	priv->switch_base = of_iomap(np, 2);
	if (IS_ERR(priv->switch_base))
		priv->switch_base = NULL;

	phy = devm_kzalloc(dev, sizeof(*phy), GFP_KERNEL);
	if (!phy)
		return -ENOMEM;

	platform_set_drvdata(pdev, phy);
	phy_set_drvdata(phy, priv);
	hisi_usb3_phy_on(phy);

	return 0;
}

static int hisi_usb3_phy_remove(struct platform_device *pdev)
{
	struct phy *phy = dev_get_drvdata(&pdev->dev);

	hisi_usb3_phy_off(phy);
	return 0;
}

static const struct of_device_id hisi_usb3_phy_of_match[] = {
	{.compatible = "hisilicon,hisi-usb3-phy",},
	{ },
};
MODULE_DEVICE_TABLE(of, hisi_usb3_phy_of_match);

#ifdef CONFIG_PM_SLEEP
static int hisi_usb3_phy_suspend(struct device *dev)
{
	struct phy *phy = dev_get_drvdata(dev);

	hisi_usb3_phy_off(phy);
	return 0;
}

static int hisi_usb3_phy_resume(struct device *dev)
{
	struct phy *phy = dev_get_drvdata(dev);

	hisi_usb3_phy_on(phy);
	return 0;
}
#endif /* CONFIG_PM_SLEEP */

static SIMPLE_DEV_PM_OPS(hisi_usb3_pm_ops, hisi_usb3_phy_suspend,
		       hisi_usb3_phy_resume);

static struct platform_driver hisi_usb3_phy_driver = {
	.probe	= hisi_usb3_phy_probe,
	.remove = hisi_usb3_phy_remove,
	.driver = {
		.name	= "hisi-usb3-phy",
		.pm	= &hisi_usb3_pm_ops,
		.of_match_table	= hisi_usb3_phy_of_match,
	}
};

module_platform_driver(hisi_usb3_phy_driver);

MODULE_AUTHOR("Chen zequn <chenzequn2@hisilicon.com>");
MODULE_DESCRIPTION("HISILICON USB PHY driver");
MODULE_ALIAS("platform:hisi-usb3-phy");
MODULE_LICENSE("GPL v2");

