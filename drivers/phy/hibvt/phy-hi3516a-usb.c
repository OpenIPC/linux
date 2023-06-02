#include <linux/delay.h>
#include <linux/io.h>
#include <linux/phy/phy.h>
#include "phy-hisi-usb.h"

#define USB2_SWITCH_OFFSET	0x130
#define PERI_CRG46		0xb8
#define USB_CKEN		(1 << 7)
#define USB_CTRL_UTMI0_REG	(1 << 5)
#define USB_CTRL_HUB_REG	(1 << 4)
#define USBPHY_PORT0_TREQ	(1 << 2)
#define USBPHY_REQ		(1 << 1)
#define USB_AHB_SRST_REQ	(1 << 0)

#define PERI_USB                0x78
#define WORDINTERFACE           (1 << 0)
#define SS_BURST4_EN            (1 << 7)
#define SS_BURST8_EN            (1 << 8)
#define SS_BURST16_EN           (1 << 9)
#define USBOVR_P_CTRL           (1 << 17)
#define MISC_USB                0x80

static int *usb2_switch_base;

void hisi_switch_func(int otg)
{
	int reg;

	reg = readl(usb2_switch_base);
	if (otg) {
		reg |= 0x1;
		writel(reg, usb2_switch_base);
	} else {
		reg &= ~(0x1);
		writel(reg, usb2_switch_base);
	}
}
EXPORT_SYMBOL(hisi_switch_func);

void hisi_usb_phy_on(struct phy *phy)
{
	int reg;
	struct hisi_priv *priv = phy_get_drvdata(phy);

	usb2_switch_base = priv->switch_base + USB2_SWITCH_OFFSET;
	/* enable phy ref clk to enable phy */
	reg = readl(priv->peri_ctrl + PERI_CRG46);
	reg |= USB_CKEN;
	writel(reg, priv->peri_ctrl + PERI_CRG46);
	udelay(100);

	/* config controller */
	reg = readl(priv->misc_ctrl + PERI_USB);
	reg &= ~(WORDINTERFACE); /* 8bit */
	/* disable ehci burst16 mode*/
	reg &= ~(SS_BURST16_EN);
	reg |= USBOVR_P_CTRL;
	writel(reg, priv->misc_ctrl + PERI_USB);
	udelay(100);

	/* de-assert phy port */
	reg = readl(priv->peri_ctrl + PERI_CRG46);
	reg &= ~(USBPHY_REQ);
	writel(reg, priv->peri_ctrl + PERI_CRG46);
	udelay(100);

	writel(0x908, priv->misc_ctrl + MISC_USB);
	udelay(10);
	writel(0x928, priv->misc_ctrl + MISC_USB);
	mdelay(1);

	/* open phy clk */
	writel(0xc06, priv->misc_ctrl + MISC_USB);
	udelay(10);
	writel(0xc26, priv->misc_ctrl + MISC_USB);
	mdelay(5);

	writel(0x108, priv->misc_ctrl + MISC_USB);
	udelay(10);
	writel(0x128, priv->misc_ctrl + MISC_USB);
	mdelay(2);

	/* usb2.0 phy eye pattern */
	writel(0x1c00, priv->misc_ctrl + MISC_USB);
	udelay(10);
	writel(0x1c20, priv->misc_ctrl + MISC_USB);
	mdelay(5);

	writel(0x0c09, priv->misc_ctrl + MISC_USB);
	udelay(10);
	writel(0x0c29, priv->misc_ctrl + MISC_USB);
	mdelay(5);

	writel(0x1a0a, priv->misc_ctrl + MISC_USB);
	udelay(10);
	writel(0x1a2a, priv->misc_ctrl + MISC_USB);
	mdelay(5);

	/* cancel phy utmi reset */
	reg = readl(priv->peri_ctrl + PERI_CRG46);
	reg &= ~(USBPHY_PORT0_TREQ);
	writel(reg, priv->peri_ctrl + PERI_CRG46);
	udelay(300);

	/* de-assert all the rsts of ctrl */
	reg = readl(priv->peri_ctrl + PERI_CRG46);
	reg &= ~(USB_CTRL_UTMI0_REG);
	reg &= ~(USB_CTRL_HUB_REG);
	reg &= ~(USB_AHB_SRST_REQ);
	writel(reg, priv->peri_ctrl + PERI_CRG46);
	udelay(200);

	/* decrease the threshold value from 650 to 550*/
	writel(0xa, priv->misc_ctrl + MISC_USB);
	udelay(10);
	writel(0x092a, priv->misc_ctrl + MISC_USB);
	mdelay(5);
}
EXPORT_SYMBOL(hisi_usb_phy_on);

void hisi_usb_phy_off(struct phy *phy)
{
	int reg;
	struct hisi_priv *priv = phy_get_drvdata(phy);

	reg = readl(priv->peri_ctrl + PERI_CRG46);
	reg &= ~(USB_CKEN);
	reg |= (USB_CTRL_UTMI0_REG);
	reg |= (USB_CTRL_HUB_REG);
	reg |= (USBPHY_PORT0_TREQ);
	reg |= (USBPHY_REQ);
	reg |= (USB_AHB_SRST_REQ);
	writel(reg, priv->peri_ctrl + PERI_CRG46);
	udelay(100);

	/* enable phy */
	reg = readl(priv->misc_ctrl + PERI_USB);
	reg |= (WORDINTERFACE);
	reg |= (SS_BURST16_EN);
	reg |= (USBOVR_P_CTRL);
	writel(reg, priv->misc_ctrl + PERI_USB);
}
EXPORT_SYMBOL(hisi_usb_phy_off);
