// SPDX-License-Identifier: GPL-2.0-only
/*
 * HiSilicon Hi3520D V100 USB 2.0 PHY driver
 *
 * Copyright (c) 2026 Dylan Corrales
 *
 * The start and stop sequence and the control word values follow HiSilicon's
 * GPL vendor USB driver (hiusb_start_hcd() and hiusb_stop_hcd()); the bit
 * meanings are from the data sheet.
 *
 * The two-port PHY has no register window of its own. It is released from
 * reset through CRG reset lines (PERI_CRG46) behind the USB clock gate and is
 * configured through the USB control word PERIPHCTRL20 (PERI_USB) of the
 * system controller. The PHY node is a child of the system controller node
 * and uses its regmap.
 */

#include <linux/array_size.h>
#include <linux/bits.h>
#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <linux/mfd/syscon.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/phy/phy.h>
#include <linux/platform_device.h>
#include <linux/regmap.h>
#include <linux/reset.h>
#include <linux/types.h>

/* PERIPHCTRL20 (PERI_USB), the USB control word */
#define USB_CTRL_REG			0x80
#define USB_CTRL_WORDINTERFACE		BIT(0)	/* 0: 8-bit, 1: 16-bit UTMI */
#define USB_CTRL_ULPI_BYPASS_EN		BIT(3)	/* 1: PHY wired as UTMI */
#define USB_CTRL_SS_ENA_INCR16		BIT(9)	/* 1: AHB INCR16 bursts on */
#define USB_CTRL_USBOVR_P_CTRL		BIT(17)	/* over-current: 0 low, 1 high active */

#define USB_CTRL_MASK			(USB_CTRL_WORDINTERFACE | \
					 USB_CTRL_ULPI_BYPASS_EN | \
					 USB_CTRL_SS_ENA_INCR16 | \
					 USB_CTRL_USBOVR_P_CTRL)

/*
 * While the PHY is in use: UTMI through the ULPI bypass, 8-bit interface,
 * INCR16 bursts off ("disable ehci burst16 mode" in the vendor start
 * sequence) and the over-current polarity override off.
 */
#define USB_CTRL_ACTIVE			USB_CTRL_ULPI_BYPASS_EN

/*
 * After exit(): the state the vendor stop sequence leaves (16-bit interface,
 * ULPI bypass off, INCR16 on, over-current high active). The PHY is held in
 * reset then and init() writes all four bits again.
 */
#define USB_CTRL_PARKED			(USB_CTRL_WORDINTERFACE | \
					 USB_CTRL_SS_ENA_INCR16 | \
					 USB_CTRL_USBOVR_P_CTRL)

/*
 * Settle time after the resets are released and after the control word write.
 * The vendor start/stop sequence (hiusb_start_hcd() and hiusb_stop_hcd() of
 * its hiusb-godeyes.c) waits 100 us in the same places. No data sheet minimum
 * is known.
 */
#define USB_SETTLE_DELAY_US		100

static const char * const hi3520d_usb2_phy_resets[] = {
	"phy", "port0-treq", "port1-treq", "utmi0", "utmi1", "hub",
};

struct hi3520d_usb2_phy {
	struct regmap *sysctrl;
	struct clk *clk;
	struct reset_control_bulk_data rsts[ARRAY_SIZE(hi3520d_usb2_phy_resets)];
};

static int hi3520d_usb2_phy_init(struct phy *phy)
{
	struct hi3520d_usb2_phy *priv = phy_get_drvdata(phy);
	int ret;

	ret = clk_prepare_enable(priv->clk);
	if (ret)
		return ret;

	/* Release the PHY, both UTMI ports and the hub from reset */
	ret = reset_control_bulk_deassert(ARRAY_SIZE(priv->rsts), priv->rsts);
	if (ret)
		goto err_clk;
	fsleep(USB_SETTLE_DELAY_US);

	ret = regmap_update_bits(priv->sysctrl, USB_CTRL_REG, USB_CTRL_MASK,
				 USB_CTRL_ACTIVE);
	if (ret)
		goto err_rst;
	fsleep(USB_SETTLE_DELAY_US);

	return 0;

err_rst:
	reset_control_bulk_assert(ARRAY_SIZE(priv->rsts), priv->rsts);
err_clk:
	clk_disable_unprepare(priv->clk);

	return ret;
}

static int hi3520d_usb2_phy_exit(struct phy *phy)
{
	struct hi3520d_usb2_phy *priv = phy_get_drvdata(phy);
	int ret, ret2;

	ret = regmap_update_bits(priv->sysctrl, USB_CTRL_REG, USB_CTRL_MASK,
				 USB_CTRL_PARKED);
	/*
	 * Same order as the deassert. The vendor stop sequence asserts all
	 * resets in one register write, so no order is known to matter; the
	 * reverse order has not been verified on hardware.
	 */
	ret2 = reset_control_bulk_assert(ARRAY_SIZE(priv->rsts), priv->rsts);
	clk_disable_unprepare(priv->clk);

	return ret ?: ret2;
}

static const struct phy_ops hi3520d_usb2_phy_ops = {
	.init	= hi3520d_usb2_phy_init,
	.exit	= hi3520d_usb2_phy_exit,
	.owner	= THIS_MODULE,
};

static int hi3520d_usb2_phy_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct device_node *parent;
	struct phy_provider *provider;
	struct hi3520d_usb2_phy *priv;
	struct phy *phy;
	int i, ret;

	priv = devm_kzalloc(dev, sizeof(*priv), GFP_KERNEL);
	if (!priv)
		return -ENOMEM;

	/* The PHY node is a child of the system controller (syscon) node */
	parent = of_get_parent(dev->of_node);
	priv->sysctrl = syscon_node_to_regmap(parent);
	of_node_put(parent);
	if (IS_ERR(priv->sysctrl))
		return dev_err_probe(dev, PTR_ERR(priv->sysctrl),
				     "cannot get the system controller regmap\n");

	priv->clk = devm_clk_get(dev, NULL);
	if (IS_ERR(priv->clk))
		return dev_err_probe(dev, PTR_ERR(priv->clk), "cannot get the clock\n");

	for (i = 0; i < ARRAY_SIZE(priv->rsts); i++)
		priv->rsts[i].id = hi3520d_usb2_phy_resets[i];
	ret = devm_reset_control_bulk_get_exclusive(dev, ARRAY_SIZE(priv->rsts),
						    priv->rsts);
	if (ret)
		return dev_err_probe(dev, ret, "cannot get the resets\n");

	phy = devm_phy_create(dev, NULL, &hi3520d_usb2_phy_ops);
	if (IS_ERR(phy))
		return dev_err_probe(dev, PTR_ERR(phy), "cannot create the PHY\n");

	phy_set_drvdata(phy, priv);
	provider = devm_of_phy_provider_register(dev, of_phy_simple_xlate);

	return PTR_ERR_OR_ZERO(provider);
}

static const struct of_device_id hi3520d_usb2_phy_of_match[] = {
	{ .compatible = "hisilicon,hi3520dv100-usb2-phy" },
	{ }
};
MODULE_DEVICE_TABLE(of, hi3520d_usb2_phy_of_match);

static struct platform_driver hi3520d_usb2_phy_driver = {
	.probe	= hi3520d_usb2_phy_probe,
	.driver = {
		.name = "hi3520d-usb2-phy",
		.of_match_table = hi3520d_usb2_phy_of_match,
		/* Unbinding would free the PHY under live EHCI/OHCI users */
		.suppress_bind_attrs = true,
	},
};
module_platform_driver(hi3520d_usb2_phy_driver);

MODULE_DESCRIPTION("HiSilicon Hi3520D V100 USB 2.0 PHY driver");
MODULE_AUTHOR("Dylan Corrales <deathcamel58@gmail.com>");
MODULE_LICENSE("GPL");
