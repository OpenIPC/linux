/*
 * phy-hisi-usb.c
 *
 * USB phy driver.
 *
 * Copyright (C) Hisilicon Technologies Co., Ltd. 2017-2019. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
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
#include <linux/of_address.h>
#include <linux/phy/phy.h>
#include <linux/platform_device.h>

#include "phy-hisi-usb.h"

static int hisi_usb_iomap(struct device_node *np, struct hisi_priv *priv)
{
	if ((np == NULL) || (priv == NULL))
		return -EINVAL;

	priv->peri_ctrl = of_iomap(np, CRG_REGBASE_NODE_IDX);
	if (IS_ERR(priv->peri_ctrl))
		return PTR_ERR(priv->peri_ctrl);

	priv->misc_ctrl = of_iomap(np, MISC_REGBASE_NODE_IDX);
	if (IS_ERR(priv->misc_ctrl)) {
		iounmap(priv->peri_ctrl);
		return PTR_ERR(priv->misc_ctrl);
	}

	return 0;
}

static int hisi_usb_phy_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct phy *phy = NULL;
	struct hisi_priv *priv = NULL;
	struct device_node *np = pdev->dev.of_node;
	int ret;

	priv = devm_kzalloc(dev, sizeof(*priv), GFP_KERNEL);
	if (priv == NULL)
		return -ENOMEM;

	phy = devm_kzalloc(dev, sizeof(*phy), GFP_KERNEL);
	if (phy == NULL) {
		devm_kfree(dev, priv);
		priv = NULL;
		return -ENOMEM;
	}

	ret = hisi_usb_iomap(np, priv);
	if (ret)
		goto usb2_unmap;

	platform_set_drvdata(pdev, phy);
	phy_set_drvdata(phy, priv);
	hisi_usb_phy_on(phy);

	iounmap(priv->misc_ctrl);
	iounmap(priv->peri_ctrl);

	return 0;
usb2_unmap:
	devm_kfree(dev, priv);
	priv = NULL;

	devm_kfree(dev, phy);
	phy = NULL;

	return ret;
}

static int hisi_usb_phy_remove(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct phy *phy = dev_get_drvdata(&pdev->dev);
	struct hisi_priv *priv = phy_get_drvdata(phy);

	hisi_usb_phy_off(phy);

	devm_kfree(dev, priv);
	priv = NULL;

	devm_kfree(dev, phy);
	phy = NULL;
	return 0;
}

static const struct of_device_id hisi_usb_phy_of_match[] = {
	{
		.compatible = "hisilicon,hisi-usb-phy",
	},
	{},
};
MODULE_DEVICE_TABLE(of, hisi_usb_phy_of_match);

#ifdef CONFIG_PM_SLEEP
static int hisi_usb_phy_suspend(struct device *dev)
{
	struct phy *phy = dev_get_drvdata(dev);

	hisi_usb_phy_off(phy);
	return 0;
}

static int hisi_usb_phy_resume(struct device *dev)
{
	struct phy *phy = dev_get_drvdata(dev);

	hisi_usb_phy_on(phy);
	return 0;
}
#endif /* CONFIG_PM_SLEEP */

static SIMPLE_DEV_PM_OPS(hisi_usb2_pm_ops, hisi_usb_phy_suspend,
				hisi_usb_phy_resume);

static struct platform_driver hisi_usb_phy_driver = {
	.probe = hisi_usb_phy_probe,
	.remove = hisi_usb_phy_remove,
	.driver = {
		.name = "hisi-usb-phy",
		.pm = &hisi_usb2_pm_ops,
		.of_match_table = hisi_usb_phy_of_match,
	}
};
module_platform_driver(hisi_usb_phy_driver);

MODULE_DESCRIPTION("HISILICON USB PHY driver");
MODULE_ALIAS("platform:hisi-usb-phy");
MODULE_LICENSE("GPL v2");
