/*
 * Copyright (c) 2017 HiSilicon Technologies Co., Ltd.
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */
#include <linux/delay.h>
#include <linux/io.h>
#include <linux/phy/phy.h>

#include "phy-hisi-usb.h"

#define USB2_SWITCH_OFFSET 0x130
#define PERI_CRG46         0xb8
#define USB_CKEN           (1 << 7)
#define USB_CTRL_UTMI0_REG (1 << 5)
#define USB_CTRL_HUB_REG   (1 << 4)
#define USBPHY_PORT0_TREQ  (1 << 2)
#define USBPHY_REQ         (1 << 1)
#define USB_AHB_SRST_REQ   (1 << 0)

#define PERI_USB      0x78
#define WORDINTERFACE (1 << 0)
#define SS_BURST4_EN  (1 << 7)
#define SS_BURST8_EN  (1 << 8)
#define SS_BURST16_EN (1 << 9)
#define USBOVR_P_CTRL (1 << 17)
#define MISC_USB      0x80

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

void usb_phy_eye_config(struct phy *phy)
{
	struct hisi_priv *priv = phy_get_drvdata(phy);

	writel(0x908, priv->misc_ctrl + MISC_USB);
	udelay(U_LEVEL1);
	writel(0x928, priv->misc_ctrl + MISC_USB);
	mdelay(M_LEVEL1);

	/* open phy clk */
	writel(0xc06, priv->misc_ctrl + MISC_USB);
	udelay(U_LEVEL1);
	writel(0xc26, priv->misc_ctrl + MISC_USB);
	mdelay(M_LEVEL2);

	writel(0x108, priv->misc_ctrl + MISC_USB);
	udelay(U_LEVEL1);
	writel(0x128, priv->misc_ctrl + MISC_USB);
	mdelay(M_LEVEL2);

	/* usb2.0 phy eye pattern */
	writel(0x1c00, priv->misc_ctrl + MISC_USB);
	udelay(U_LEVEL1);
	writel(0x1c20, priv->misc_ctrl + MISC_USB);
	mdelay(M_LEVEL2);

	writel(0x0c09, priv->misc_ctrl + MISC_USB);
	udelay(U_LEVEL1);
	writel(0x0c29, priv->misc_ctrl + MISC_USB);
	mdelay(M_LEVEL2);

	writel(0x1a0a, priv->misc_ctrl + MISC_USB);
	udelay(U_LEVEL1);
	writel(0x1a2a, priv->misc_ctrl + MISC_USB);
	mdelay(M_LEVEL2);
}

void hisi_usb_phy_on(struct phy *phy)
{
	int reg;
	struct hisi_priv *priv = phy_get_drvdata(phy);

	usb2_switch_base = priv->switch_base + USB2_SWITCH_OFFSET;
	/* enable phy ref clk to enable phy */
	reg = readl(priv->peri_ctrl + PERI_CRG46);
	reg |= USB_CKEN;
	writel(reg, priv->peri_ctrl + PERI_CRG46);
	udelay(U_LEVEL5);

	/* config controller */
	reg = readl(priv->misc_ctrl + PERI_USB);
	reg &= ~(WORDINTERFACE); /* 8bit */
	reg &= ~(SS_BURST16_EN); /* disable ehci burst16 mode */
	reg |= USBOVR_P_CTRL;
	writel(reg, priv->misc_ctrl + PERI_USB);
	udelay(U_LEVEL5);

	/* de-assert phy port */
	reg = readl(priv->peri_ctrl + PERI_CRG46);
	reg &= ~(USBPHY_REQ);
	writel(reg, priv->peri_ctrl + PERI_CRG46);
	udelay(U_LEVEL5);

	usb_phy_eye_config(phy);

	/* cancel phy utmi reset */
	reg = readl(priv->peri_ctrl + PERI_CRG46);
	reg &= ~(USBPHY_PORT0_TREQ);
	writel(reg, priv->peri_ctrl + PERI_CRG46);
	udelay(U_LEVEL7);

	/* de-assert all the rsts of ctrl */
	reg = readl(priv->peri_ctrl + PERI_CRG46);
	reg &= ~(USB_CTRL_UTMI0_REG);
	reg &= ~(USB_CTRL_HUB_REG);
	reg &= ~(USB_AHB_SRST_REQ);
	writel(reg, priv->peri_ctrl + PERI_CRG46);
	udelay(U_LEVEL6);

	/* decrease the threshold value from 650 to 550 */
	writel(0xa, priv->misc_ctrl + MISC_USB);
	udelay(U_LEVEL1);
	writel(0x092a, priv->misc_ctrl + MISC_USB);
	mdelay(M_LEVEL2);
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
	udelay(U_LEVEL5);

	/* enable phy */
	reg = readl(priv->misc_ctrl + PERI_USB);
	reg |= (WORDINTERFACE);
	reg |= (SS_BURST16_EN);
	reg |= (USBOVR_P_CTRL);
	writel(reg, priv->misc_ctrl + PERI_USB);
}
EXPORT_SYMBOL(hisi_usb_phy_off);
