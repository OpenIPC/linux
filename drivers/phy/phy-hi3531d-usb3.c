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

#define SYS_CTRL		0x12050000
#define USB2_PHY1		0x16c
#define USB2_PHY1_TEST_SRST_REQ	BIT(10)
#define USB2_PHY1_SRST_TREQ	BIT(9)
#define USB2_PHY1_SRST_REQ	BIT(8)
#define USB2_PHY1_REF_CLKEN	BIT(0)

#define HOST_U3_DISABLE        (1 << 3)
#define USB3_U2_PHY		0x12c

#define USB3_CTRL		0x12c
#define USB3_VCC_SRST_REQ		BIT(13)
#define USB3_UTMI_CLKEN			BIT(12)
#define USB3_PIPE_CLKEN			BIT(11)
#define USB3_SUSPEND_CLKEN		BIT(10)
#define USB3_REF_CLKEN			BIT(9)
#define USB3_BUS_CLKEN			BIT(8)

#define USB3_COMBPHY		0x120
#define COMBPHY1_LANE0_REQ		BIT(2)

#define GTXTHRCFG		0xc108
#define GRXTHRCFG		0xc10c
#define REG_GCTL		0xc110
#define U2RSTECN		(0x1 << 16)

#define REG_GUSB2PHYCFG0	0xC200
#define BIT_UTMI_ULPI		(0x1 << 4)
#define BIT_UTMI_8_16		(0x1 << 3)

#define REG_GUSB3PIPECTL0	0xc2c0
#define PCS_SSP_SOFT_RESET	(0x1 << 31)
#define TX_MARGIN_MASK		(0x7 << 3)
#define TX_MARGIN_VAL		(0x2 << 3)

#define USB2_PHY0_CTLL		0x80

#define USB3_PHY		0x88
#define COMBO_PHY_TX_DEEMP_MASK		(0x7 << 12)
#define COMBO_PHY_TX_DEEMP_VAL		(0x1 << 12)

#define USB2_PHY_TEST_REG_ACCESS        (1 << 20)

struct hisi_priv {
	void __iomem	*base;
	void __iomem	*dwc3_ctrl; /* 0x11000000 */
	void __iomem	*peri_ctrl; /* 0x12040000 */
	void __iomem	*misc_ctrl; /* 0x12120000 */
};

static void hisi_usb3_phy_eye(struct phy *phy)
{
	int reg;
	struct hisi_priv *priv = phy_get_drvdata(phy);

	/* configuration method */
	reg = readl(priv->misc_ctrl + 0x10);
	reg |= USB2_PHY_TEST_REG_ACCESS;
	writel(reg, priv->misc_ctrl + 0x10);
	udelay(100);
	/* slew rate */
	writel(0xc4, priv->misc_ctrl + 0x8020);
	udelay(20);
	/* Calibration mode */
	writel(0xc1, priv->misc_ctrl + 0x8044);
	udelay(20);
	/* disconnect threshold value */
	writel(0x1b, priv->misc_ctrl + 0x8028);
	udelay(20);
	/* turn on pre-emphasis */
	writel(0x1c, priv->misc_ctrl + 0x8000);
	udelay(20);
	writel(0x92, priv->misc_ctrl + 0x8014);
	udelay(20);
	writel(0xd, priv->misc_ctrl + 0x8018);
	udelay(20);
	/* down 100mv 2017/2/13 by h292880*/
	writel(0x103, priv->misc_ctrl + 0x134);
	udelay(20);
	writel(0x143, priv->misc_ctrl + 0x134);
	udelay(20);
}

static int hisi_usb3_ctrl_phy_config(struct phy *phy)
{
	int reg;
	struct hisi_priv *priv = phy_get_drvdata(phy);

	reg = readl(priv->dwc3_ctrl + REG_GUSB3PIPECTL0);
	reg |= PCS_SSP_SOFT_RESET;
	writel(reg, priv->dwc3_ctrl + REG_GUSB3PIPECTL0);

	/*step 3: USB2 PHY chose ulpi 8bit interface */
	reg = readl(priv->dwc3_ctrl + REG_GUSB2PHYCFG0);
	reg &= ~BIT_UTMI_ULPI;
	reg &= ~(BIT_UTMI_8_16);
	writel(reg, priv->dwc3_ctrl + REG_GUSB2PHYCFG0);
	mdelay(20);
	reg = readl(priv->dwc3_ctrl + REG_GCTL);
	reg &= ~(0x3<<12);
	reg |= (0x1<<12); /*[13:12] 01: Host; 10: Device; 11: OTG*/
	reg &= ~U2RSTECN;
	writel(reg, priv->dwc3_ctrl + REG_GCTL);
	mdelay(20);

	reg = readl(priv->dwc3_ctrl + REG_GUSB3PIPECTL0);
	reg &= ~PCS_SSP_SOFT_RESET;
	reg &= ~(1<<17);       /* disable suspend */
	writel(reg, priv->dwc3_ctrl + REG_GUSB3PIPECTL0);
	mdelay(100);

	writel(0x23100000, priv->dwc3_ctrl + GTXTHRCFG);
	writel(0x23180000, priv->dwc3_ctrl + GRXTHRCFG);
	mdelay(20);

	hisi_usb3_phy_eye(phy);

	return 0;
}

static int hisi_usb3_phy_on(struct phy *phy)
{
	int reg;
	void __iomem *sys_base;
	struct hisi_priv *priv = phy_get_drvdata(phy);

	sys_base = ioremap(SYS_CTRL, 0x1000);
	reg = readl(sys_base + 0x8c);
	if ((reg & (0x1 << 15)) != 0) {
		reg = readl(priv->peri_ctrl + 0x120);
		reg |= COMBPHY1_LANE0_REQ;
		writel(reg, priv->peri_ctrl + 0x120);
		mdelay(10);

		reg = readl_relaxed(priv->peri_ctrl + USB3_COMBPHY);
		reg &= ~COMBPHY1_LANE0_REQ;
		writel_relaxed(reg, priv->peri_ctrl + USB3_COMBPHY);
		mdelay(10);
	} else {
		reg = readl(priv->misc_ctrl + 0x128);
		reg |= HOST_U3_DISABLE;
		writel(reg, priv->misc_ctrl + 0x128);
		mdelay(1);

		pr_info("COMBPHY IS NOT USB\n");
	}
	iounmap(sys_base);

	reg = readl_relaxed(priv->peri_ctrl + USB2_PHY1);
	reg &= ~USB2_PHY1_TEST_SRST_REQ;
	reg &= ~USB2_PHY1_SRST_TREQ;
	reg &= ~USB2_PHY1_SRST_REQ;
	reg |= USB2_PHY1_REF_CLKEN;
	writel_relaxed(reg, priv->peri_ctrl + USB2_PHY1);
	mdelay(10);

	reg = readl_relaxed(priv->peri_ctrl + USB3_CTRL);
	reg |= USB3_UTMI_CLKEN;
	reg |= USB3_PIPE_CLKEN;
	reg |= USB3_SUSPEND_CLKEN;
	reg |= USB3_REF_CLKEN;
	reg |= USB3_BUS_CLKEN;
	reg &= ~USB3_VCC_SRST_REQ;
	writel_relaxed(reg, priv->peri_ctrl + USB3_CTRL);
	mdelay(10);

	hisi_usb3_ctrl_phy_config(phy);

	return 0;
}

static int hisi_usb3_phy_power_off(struct device *dev)
{
	int reg;
	struct phy *phy = dev_get_drvdata(dev);
	struct hisi_priv *priv = phy_get_drvdata(phy);

	reg = readl_relaxed(priv->peri_ctrl + USB3_COMBPHY);
	reg |= COMBPHY1_LANE0_REQ;
	writel_relaxed(reg, priv->peri_ctrl + USB3_COMBPHY);
	mdelay(100);

	reg = readl_relaxed(priv->peri_ctrl + USB3_CTRL);
	reg |= USB3_VCC_SRST_REQ;
	writel_relaxed(reg, priv->peri_ctrl + USB3_CTRL);
	mdelay(10);

	return 0;
}

static int hisi_usb3_phy_power_on(struct device *dev)
{
	struct phy *phy = dev_get_drvdata(dev);

	hisi_usb3_phy_on(phy);

	return 0;
}

static int hisi_usb3_phy_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct phy *phy;
	struct hisi_priv *priv;
	struct device_node *np = pdev->dev.of_node;

	priv = devm_kzalloc(dev, sizeof(*priv), GFP_KERNEL);
	if (!priv)
		return -ENOMEM;

	priv->dwc3_ctrl = of_iomap(np, 0); /* 0x11000000 */
	if (IS_ERR(priv->dwc3_ctrl))
		priv->dwc3_ctrl = NULL;

	priv->peri_ctrl = of_iomap(np, 1); /* 0x12040000 */
	if (IS_ERR(priv->peri_ctrl))
		priv->peri_ctrl = NULL;

	priv->misc_ctrl = of_iomap(np, 2); /* 0x12120000 */
	if (IS_ERR(priv->misc_ctrl))
		priv->misc_ctrl = NULL;

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
	int reg;
	struct phy *phy = platform_get_drvdata(pdev);
	struct hisi_priv *priv = phy_get_drvdata(phy);

	reg = readl_relaxed(priv->peri_ctrl + USB3_CTRL);
	reg &= ~USB3_UTMI_CLKEN;
	reg &= ~USB3_PIPE_CLKEN;
	reg &= ~USB3_SUSPEND_CLKEN;
	reg &= ~USB3_REF_CLKEN;
	reg &= ~USB3_BUS_CLKEN;
	reg |= USB3_VCC_SRST_REQ;
	writel_relaxed(reg, priv->peri_ctrl + USB3_CTRL);
	mdelay(10);

	return 0;
}

static const struct dev_pm_ops hisi_usb3_pmops = {
	.suspend = hisi_usb3_phy_power_off,
	.resume  = hisi_usb3_phy_power_on,
#if defined(CONFIG_PM_HIBERNATE) || defined(CONFIG_HISI_SNAPSHOT_BOOT)
	.freeze = hisi_usb3_phy_power_off,
	.thaw = hisi_usb3_phy_power_on,
	.poweroff = hisi_usb3_phy_power_off,
	.restore = hisi_usb3_phy_power_on,
#endif
};

static const struct of_device_id hisi_usb3_phy_of_match[] = {
	{.compatible = "hisilicon,hi3531d-usb3-phy",},
	{ },
};
MODULE_DEVICE_TABLE(of, hisi_usb3_phy_of_match);

static struct platform_driver hisi_usb3_phy_driver = {
	.probe	= hisi_usb3_phy_probe,
	.remove = hisi_usb3_phy_remove,
	.driver = {
		.name	= "hisi-usb3-phy",
		.of_match_table	= hisi_usb3_phy_of_match,
		.pm    = &hisi_usb3_pmops,
	}
};
module_platform_driver(hisi_usb3_phy_driver);

MODULE_AUTHOR("Pengcheng Li <lpc.li@hisilicon.com>");
MODULE_DESCRIPTION("HISILICON USB PHY driver");
MODULE_ALIAS("platform:hisi-usb3-phy");
MODULE_LICENSE("GPL v2");
