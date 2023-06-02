/*
 * Copyright (c) 2015 HiSilicon Technologies Co., Ltd.
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

#include <linux/delay.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/phy/phy.h>
#include <linux/platform_device.h>
#include <linux/of_address.h>
#include <mach/io.h>
#include <linux/kthread.h>
#include <linux/export.h>

#define USB2_CTRL		0x130
#define USB2_BUS_CKEN		(1 << 0)
#define USB2_OHCI48M_CKEN	(1 << 1)
#define USB2_OHCI12M_CKEN	(1 << 2)
#define USB2_HST_PHY_CKEN	(1 << 4)
#define USB2_UTMI0_CKEN		(1 << 5)
#define USB2_UTMI1_CKEN		(1 << 6)
#define USB2_BUS_SRST_REQ	(1 << 12)
#define USB2_UTMI0_SRST_REQ	(1 << 13)
#define USB2_UTMI1_SRST_REQ	(1 << 14)
#define USB2_HST_PHY_SYST_REQ	(1 << 16)

#define REG_USB2_PHY0		0x134
#define USB_PHY0_REF_CKEN	(1 << 0)
#define USB_PHY0_SRST_REQ	(1 << 8)
#define USB_PHY0_SRST_TREQ	(1 << 9)
#define USB_PHY1_SRST_TREQ	(1 << 10)
#define USB_PHY0_TEST_SRST_REQ	(1 << 11)
#define USB_PHY0_REFCLK_SEL	(1 << 16)
#ifdef CONFIG_ARCH_HI3531D
#define USB1_CTRL0		0x90
#endif
#if defined(CONFIG_ARCH_HI3521D) || defined(CONFIG_ARCH_HI3536C)
#define USB1_CTRL0		0x50
#endif
#define WORDINTERFACE		(1 << 0)
#define SS_BURST4_EN		(1 << 7)
#define SS_BURST8_EN		(1 << 8)
#define SS_BURST16_EN		(1 << 9)

#define USB2_PHY_TEST_REG_ACCESS	(1 << 20)

#define USB2_CTRL1		0x94
/* write(0x1 << 5) 0x6 to addr 0x4 */
#define CONFIG_CLK	((0x1 << 21) | (0x6 << 8) | (0x4 << 0))

extern int otg_usbdev_stat;
int otg_usbhost_stat;
EXPORT_SYMBOL(otg_usbhost_stat);

struct hisi_priv {
	void __iomem	*base;
	void __iomem	*peri_ctrl; /* 0x12040000 */
	void __iomem	*misc_ctrl; /* 0x12120000 */
};

static int hisi_usb_phy_on(struct phy *phy)
{
	int reg;
	struct hisi_priv *priv = phy_get_drvdata(phy);

	/* reset enable */
	reg = readl(priv->peri_ctrl + USB2_CTRL);
	reg |= (USB2_BUS_SRST_REQ
		| USB2_UTMI0_SRST_REQ
		| USB2_HST_PHY_SYST_REQ);
#if defined(CONFIG_ARCH_HI3521D) || defined(CONFIG_ARCH_HI3536C)
	reg |= USB2_UTMI1_SRST_REQ;
#endif

	writel(reg, priv->peri_ctrl + USB2_CTRL);
	udelay(200);

	reg = readl(priv->peri_ctrl + REG_USB2_PHY0);
	reg |= (USB_PHY0_SRST_REQ
		| USB_PHY0_SRST_TREQ);
#if defined(CONFIG_ARCH_HI3521D) || defined(CONFIG_ARCH_HI3536C)
	reg |= USB_PHY1_SRST_TREQ;
#endif
	writel(reg, priv->peri_ctrl + REG_USB2_PHY0);
	udelay(200);
	reg = readl(priv->misc_ctrl + USB1_CTRL0);
	reg &= ~(WORDINTERFACE); /* 8bit */
	reg &= ~(SS_BURST16_EN); /* 16 bit burst disable */
	writel(reg, priv->misc_ctrl + USB1_CTRL0);
	udelay(100);
	/* for ssk usb storage ok */
	msleep(20);

	/* open ref clock */
	reg = readl(priv->peri_ctrl + REG_USB2_PHY0);
	reg |= (USB_PHY0_REF_CKEN);
	writel(reg, priv->peri_ctrl + REG_USB2_PHY0);
	udelay(100);

	/* cancel power on reset */
	reg = readl(priv->peri_ctrl + REG_USB2_PHY0);
	reg &= ~(USB_PHY0_SRST_REQ);
	reg &= ~(USB_PHY0_TEST_SRST_REQ);
	writel(reg , priv->peri_ctrl + REG_USB2_PHY0);
	udelay(300);

	/* config type */
	reg = readl(priv->misc_ctrl + 0x10);
	reg |= USB2_PHY_TEST_REG_ACCESS;
	writel(reg, priv->misc_ctrl + 0x10);
	udelay(2);
#if defined(CONFIG_ARCH_HI3521D) || defined(CONFIG_ARCH_HI3536C)
	/* config clock */
	writel(0xc, priv->misc_ctrl + 0x8018);
	mdelay(2);
	/* port0 IComp default 400 and disconnect 625mv */
	writel(0x1, priv->misc_ctrl + 0x8028);
	udelay(20);
	/* port1 IComp default400 and disconnect 625mv */
	writel(0x1, priv->misc_ctrl + 0x8428);
	udelay(20);
	/* comp 262mv */
	writel(0xc, priv->misc_ctrl + 0x8018);
	udelay(20);
	writel(0x92, priv->misc_ctrl + 0x8014);
	udelay(20);
	/* port0 pre-emphasis to adjust for 000 0x9[5:3] */
	writel(0x4, priv->misc_ctrl + 0x8024);
	udelay(20);
	/* port1 pre-emphasis to adjust for 000 0x9[5:3] */
	writel(0x4, priv->misc_ctrl + 0x8424);
	udelay(20);
	/* port 1 pre driver  011 */
	writel(0xc4, priv->misc_ctrl + 0x8420);
	udelay(20);
	/* port0 pre-emphasis to adjust for 000 0x9[5:3] */
	writel(0xc4, priv->misc_ctrl + 0x8020);
	udelay(20);
#endif
#ifdef CONFIG_ARCH_HI3531D
	/* config clock */
	writel(0xc, priv->misc_ctrl + 0x4018);
	mdelay(2);
	/* slew rate */
	writel(0xc4, priv->misc_ctrl + 0x4020);
	udelay(20);
	writel(0xc1, priv->misc_ctrl + 0x4044);
	udelay(20);
	/* disconnect */
	writel(0x1b, priv->misc_ctrl + 0x4028);
	udelay(20);
	writel(0x1c, priv->misc_ctrl + 0x4000);
	udelay(20);
	writel(0x92, priv->misc_ctrl + 0x4014);
	udelay(20);
	writel(0xe, priv->misc_ctrl + 0x4018);
	udelay(20);
	writel(0x4, priv->misc_ctrl + 0x4024);
	udelay(20);
#endif
	/* cancel port reset */
	reg = readl(priv->peri_ctrl + REG_USB2_PHY0);
	reg &= ~(USB_PHY0_SRST_TREQ);
#if defined(CONFIG_ARCH_HI3521D) || defined(CONFIG_ARCH_HI3536C)
	reg &= ~(USB_PHY1_SRST_TREQ);
#endif
	writel(reg, priv->peri_ctrl + REG_USB2_PHY0);
	udelay(300);

	/* cancel control reset */
	reg = readl(priv->peri_ctrl + USB2_CTRL);
	reg &= ~(USB2_BUS_SRST_REQ
		| USB2_UTMI0_SRST_REQ
		| USB2_HST_PHY_SYST_REQ);
#if defined(CONFIG_ARCH_HI3521D) || defined(CONFIG_ARCH_HI3536C)
	reg &= ~USB2_UTMI1_SRST_REQ;
#endif

	reg |= (USB2_BUS_CKEN
			| USB2_OHCI48M_CKEN
			| USB2_OHCI12M_CKEN
			| USB2_HST_PHY_CKEN
			| USB2_UTMI0_CKEN);
#if defined(CONFIG_ARCH_HI3521D) || defined(CONFIG_ARCH_HI3536C)
	reg |= USB2_UTMI1_CKEN;
#endif
	writel(reg, priv->peri_ctrl + USB2_CTRL);
	udelay(200);

	return 0;
}

static int hisi_usb_phy_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct phy *phy;
	struct hisi_priv *priv;
	struct device_node *np = pdev->dev.of_node;

	priv = devm_kzalloc(dev, sizeof(*priv), GFP_KERNEL);
	if (!priv)
		return -ENOMEM;

	priv->peri_ctrl = of_iomap(np, 0); /* 0x12040000 */
	if (IS_ERR(priv->peri_ctrl))
		priv->peri_ctrl = NULL;

	priv->misc_ctrl = of_iomap(np, 1); /* 0x12120000 */
	if (IS_ERR(priv->misc_ctrl))
		priv->misc_ctrl = NULL;


	phy = devm_kzalloc(dev, sizeof(*phy), GFP_KERNEL);
	if (!phy)
		return -ENOMEM;
	platform_set_drvdata(pdev, phy);
	phy_set_drvdata(phy, priv);
	hisi_usb_phy_on(phy);

	return 0;
}

static int hisi_usb_phy_remove(struct platform_device *pdev)
{
#if 1
	int reg;
	struct phy *phy = platform_get_drvdata(pdev);
	struct hisi_priv *priv = phy_get_drvdata(phy);

	reg = readl(priv->peri_ctrl + REG_USB2_PHY0);
	reg |= (USB_PHY0_SRST_REQ
		| USB_PHY0_SRST_TREQ);
#if defined(CONFIG_ARCH_HI3521D) || defined(CONFIG_ARCH_HI3536C)
	reg |= USB_PHY1_SRST_TREQ;
#endif
	writel(reg, priv->peri_ctrl + REG_USB2_PHY0);
	udelay(100);

	/* close clock */
	reg = readl(priv->peri_ctrl + REG_USB2_PHY0);
	reg &= ~(USB_PHY0_REFCLK_SEL
		| USB_PHY0_REF_CKEN);
	writel(reg, priv->peri_ctrl + REG_USB2_PHY0);
	udelay(300);

	/* close clock */
	reg = readl(priv->peri_ctrl + USB2_CTRL);
	reg &= ~(USB2_BUS_CKEN
		| USB2_OHCI48M_CKEN
		| USB2_OHCI12M_CKEN
		| USB2_HST_PHY_CKEN
		| USB2_UTMI0_CKEN);
#if defined(CONFIG_ARCH_HI3521D) || defined(CONFIG_ARCH_HI3536C)
	reg &= ~USB2_UTMI1_CKEN;
#endif
	writel(reg, priv->peri_ctrl + USB2_CTRL);
	udelay(200);
#endif
	return 0;
}

static const struct of_device_id hisi_usb_phy_of_match[] = {
	{.compatible = "hisilicon,hi3531d-usb2-phy",},
	{.compatible = "hisilicon,hi3521d-usb2-phy",},
	{.compatible = "hisilicon,hi3536c-usb2-phy",},
	{ },
};
MODULE_DEVICE_TABLE(of, hisi_usb_phy_of_match);

#ifdef CONFIG_PM_SLEEP

static int hisi_usb_phy_suspend(struct device *dev)
{
	int reg;
	struct phy *phy = dev_get_drvdata(dev);
	struct hisi_priv *priv = phy_get_drvdata(phy);

	reg = readl(priv->peri_ctrl + REG_USB2_PHY0);
	reg |= (USB_PHY0_SRST_REQ
		| USB_PHY0_SRST_TREQ);
#if defined(CONFIG_ARCH_HI3521D) || defined(CONFIG_ARCH_HI3536C)
	reg |= USB_PHY1_SRST_TREQ;
#endif
	writel(reg, priv->peri_ctrl + REG_USB2_PHY0);
	udelay(100);

	/* close clock */
	reg = readl(priv->peri_ctrl + REG_USB2_PHY0);
	reg &= ~(USB_PHY0_REFCLK_SEL
		| USB_PHY0_REF_CKEN);
	writel(reg, priv->peri_ctrl + REG_USB2_PHY0);
	udelay(300);

	/* close clock */
	reg = readl(priv->peri_ctrl + USB2_CTRL);
	reg &= ~(USB2_BUS_CKEN
		| USB2_OHCI48M_CKEN
		| USB2_OHCI12M_CKEN
		| USB2_HST_PHY_CKEN
		| USB2_UTMI0_CKEN);
#if defined(CONFIG_ARCH_HI3521D) || defined(CONFIG_ARCH_HI3536C)
	reg &= ~USB2_UTMI1_CKEN;
#endif
	writel(reg, priv->peri_ctrl + USB2_CTRL);
	udelay(200);

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
	.probe	= hisi_usb_phy_probe,
	.remove = hisi_usb_phy_remove,
	.driver = {
		.name	= "hisi-usb-phy",
		.pm     = &hisi_usb2_pm_ops,
		.of_match_table	= hisi_usb_phy_of_match,
	}
};
module_platform_driver(hisi_usb_phy_driver);

MODULE_AUTHOR("Pengcheng Li <lpc.li@hisilicon.com>");
MODULE_DESCRIPTION("HISILICON USB PHY driver");
MODULE_ALIAS("platform:hisi-usb-phy");
MODULE_LICENSE("GPL v2");
