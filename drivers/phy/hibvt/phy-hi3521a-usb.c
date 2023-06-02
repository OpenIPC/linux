#include <linux/delay.h>
#include <linux/io.h>
#include <linux/phy/phy.h>
#include "phy-hisi-usb.h"

#define PERI_CRG28			0x70
#define USB_CKEN			(1 << 7)
#define USB_CTRL_UTMI1_REG	(1 << 6)
#define USB_CTRL_UTMI0_REG	(1 << 5)
#define USB_CTRL_HUB_REG	(1 << 4)
#define USBPHY_PORT1_TREQ	(1 << 3)
#define USBPHY_PORT0_TREQ	(1 << 2)
#define USBPHY_REQ			(1 << 1)
#define USB_AHB_SRST_REQ	(1 << 0)


#define PERI_USB			0x50
#define WORDINTERFACE		(1 << 0)
#define SS_BURST4_EN		(1 << 7)
#define SS_BURST8_EN		(1 << 8)
#define SS_BURST16_EN		(1 << 9)
#define USBOVR_P_CTRL		(1 << 17)
#define MISC_USB			0x54


void hisi_usb_phy_on(struct phy *phy)
{
	int reg;
	struct hisi_priv *priv = phy_get_drvdata(phy);
	/* enable phy ref clk to enable phy */
	reg = readl(priv->peri_ctrl + PERI_CRG28);
	reg |= USB_CKEN;
	writel(reg, priv->peri_ctrl + PERI_CRG28);
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
	reg = readl(priv->peri_ctrl + PERI_CRG28);
	reg &= ~(USBPHY_REQ);
	writel(reg, priv->peri_ctrl + PERI_CRG28);
	udelay(300);

	/* open phy clk */
	writel(0x406, priv->misc_ctrl + MISC_USB);
	udelay(10);
	writel(0x426, priv->misc_ctrl + MISC_USB);
	mdelay(8);

	/* cancel phy utmi reset */
	reg = readl(priv->peri_ctrl + PERI_CRG28);
	reg &= ~(USBPHY_PORT0_TREQ);
	reg &= ~(USBPHY_PORT1_TREQ);
	writel(reg, priv->peri_ctrl + PERI_CRG28);
	udelay(300);

	/* de-assert all the rsts of ctrl */
	reg = readl(priv->peri_ctrl + PERI_CRG28);
	reg &= ~(USB_CTRL_UTMI0_REG);
	reg &= ~(USB_CTRL_UTMI1_REG);
	reg &= ~(USB_CTRL_HUB_REG);
	reg &= ~(USB_AHB_SRST_REQ);
	writel(reg, priv->peri_ctrl + PERI_CRG28);
	udelay(200);
}
EXPORT_SYMBOL(hisi_usb_phy_on);

void hisi_usb_phy_off(struct phy *phy)
{
	int reg;
	/* Disable EHCI clock.
	   If the HS PHY is unused disable it too. */
	struct hisi_priv *priv = phy_get_drvdata(phy);

	reg = readl(priv->peri_ctrl + PERI_CRG28);
	reg &= ~(USB_CKEN);
	reg |= (USB_CTRL_UTMI0_REG);
	reg |= (USB_CTRL_UTMI1_REG);
	reg |= (USB_CTRL_HUB_REG);
	reg |= (USBPHY_PORT0_TREQ);
	reg |= (USBPHY_PORT1_TREQ);
	reg |= (USBPHY_REQ);
	reg |= (USB_AHB_SRST_REQ);
	writel(reg, priv->peri_ctrl + PERI_CRG28);
	udelay(100);
	/* enable phy */
	reg = readl(priv->misc_ctrl + PERI_USB);
	reg |= (WORDINTERFACE);
	reg |= (SS_BURST16_EN);
	reg |= (USBOVR_P_CTRL);
	writel(reg, priv->misc_ctrl + PERI_USB);
	udelay(100);
}
EXPORT_SYMBOL(hisi_usb_phy_off);
