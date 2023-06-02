/*
 * phy-fullhan-usb2.c - Fullhan USB2 Phy Driver
 *
 * Copyright (C) 2013 Linaro Limited
 * Fullhan <fh@fullhan.com>
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/phy/phy.h>
#include <linux/platform_device.h>

struct fullhan_usb {
	void __iomem *regs;
};

static void fullhan_usb_phy_power(struct fullhan_usb *phy, int on)
{

}

static int fullhan_usb_phy_init(struct phy *gphy)
{
	/*struct fullhan_usb *phy = phy_get_drvdata(gphy);*/

	return 0;
}

static int fullhan_usb_phy_power_on(struct phy *gphy)
{
	struct fullhan_usb *phy = phy_get_drvdata(gphy);

	fullhan_usb_phy_power(phy, 1);

	return 0;
}

static int fullhan_usb_phy_power_off(struct phy *gphy)
{
	struct fullhan_usb *phy = phy_get_drvdata(gphy);

	fullhan_usb_phy_power(phy, 0);

	return 0;
}

static struct phy_ops ops = {
	.init		= fullhan_usb_phy_init,
	.power_on	= fullhan_usb_phy_power_on,
	.power_off	= fullhan_usb_phy_power_off,
	.owner		= THIS_MODULE,
};

static int fullhan_usb2_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct fullhan_usb *phy;
	/*struct resource *res;*/
	struct phy *gphy;
	struct phy_provider *phy_provider;

	phy = devm_kzalloc(dev, sizeof(*phy), GFP_KERNEL);
	if (!phy)
		return -ENOMEM;

	platform_set_drvdata(pdev, phy);

	gphy = devm_phy_create(dev, NULL, &ops);
	if (IS_ERR(gphy))
		return PTR_ERR(gphy);

	/* The PHY supports an 8-bit wide UTMI interface */
	phy_set_bus_width(gphy, 8);

	phy_set_drvdata(gphy, phy);

	phy_provider = devm_of_phy_provider_register(dev,
			of_phy_simple_xlate);

	return PTR_ERR_OR_ZERO(phy_provider);
}

static const struct of_device_id fullhan_usb2_dt_ids[] = {
	{ .compatible = "fh,fh-usb2-phy" },
	{ /* sentinel */ }
};

MODULE_DEVICE_TABLE(of, fullhan_usb2_dt_ids);

static struct platform_driver fullhan_usb2_driver = {
	.probe		= fullhan_usb2_probe,
	.driver		= {
		.name	= "fullhan-usb2",
		.of_match_table = fullhan_usb2_dt_ids,
	},
};

module_platform_driver(fullhan_usb2_driver);

MODULE_ALIAS("platform:fullhan-usb2");
MODULE_AUTHOR("Fullhan <fh@fullhan.com>");
MODULE_DESCRIPTION("Fullhan USB 2.0 PHY driver");
MODULE_LICENSE("GPL v2");
