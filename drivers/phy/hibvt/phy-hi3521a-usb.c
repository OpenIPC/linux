/*
 * phy-hi3521a-usb.c
 *
 * USB phy driver.
 *
 * Copyright (C) Hisilicon Technologies Co., Ltd. 2018-2019. All rights reserved.
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

#define PERI_CRG28         0x70
#define USB_CKEN           (1 << 7)
#define USB_CTRL_UTMI1_REG (1 << 6)
#define USB_CTRL_UTMI0_REG (1 << 5)
#define USB_CTRL_HUB_REG   (1 << 4)
#define USBPHY_PORT1_TREQ  (1 << 3)
#define USBPHY_PORT0_TREQ  (1 << 2)
#define USBPHY_REQ         (1 << 1)
#define USB_AHB_SRST_REQ   (1 << 0)

#define PERI_USB      0x50
#define WORDINTERFACE (1 << 0)
#define SS_BURST4_EN  (1 << 7)
#define SS_BURST8_EN  (1 << 8)
#define SS_BURST16_EN (1 << 9)
#define USBOVR_P_CTRL (1 << 17)
#define MISC_USB      0x54

#define PHY_CLK_ENABLE	0x406
#define OPEN_PHY_CLK	0x426

#define EYE_PATTERN0	0xa
#define EYE_VAL0	0xbb2a
#define EYE_PATTERN1	0x5
#define EYE_VAL1	0x9225
#define EYE_PATTERN2	0x6
#define EYE_VAL2	0x626
#define EYE_PATTERN3	0x0
#define EYE_VAL3	0x1820
#define EYE_PATTERN4	0x10
#define EYE_VAL4	0x1830
#define EYE_PATTERN5	0x10
#define EYE_VAL5	0x1c30

void usb_eye_config(struct phy *phy)
{
	unsigned int reg;
	struct hisi_priv *priv = phy_get_drvdata(phy);

	/* usb2.0 phy eye pattern:Icomp = 212.5 RCOMP = 212.5 */
	writel(EYE_PATTERN0, priv->misc_ctrl + MISC_USB);
	udelay(U_LEVEL1);
	writel(EYE_VAL0, priv->misc_ctrl + MISC_USB);
	mdelay(M_LEVEL3);

	writel(EYE_PATTERN1, priv->misc_ctrl + MISC_USB);
	udelay(U_LEVEL1);
	writel(EYE_VAL1, priv->misc_ctrl + MISC_USB);
	mdelay(M_LEVEL3);

	writel(EYE_PATTERN2, priv->misc_ctrl + MISC_USB);
	udelay(U_LEVEL1);
	writel(EYE_VAL2, priv->misc_ctrl + MISC_USB);
	mdelay(M_LEVEL3);

	/* close eop pre-emphasis */
	writel(EYE_PATTERN3, priv->misc_ctrl + MISC_USB);
	udelay(U_LEVEL1);
	writel(EYE_VAL3, priv->misc_ctrl + MISC_USB);
	mdelay(M_LEVEL3);

	writel(EYE_PATTERN4, priv->misc_ctrl + MISC_USB);
	udelay(U_LEVEL1);
	writel(EYE_VAL4, priv->misc_ctrl + MISC_USB);
	mdelay(M_LEVEL3);

	/* open port1 pre-emphasis */
	writel(EYE_PATTERN5, priv->misc_ctrl + MISC_USB);
	udelay(U_LEVEL1);
	writel(EYE_VAL5, priv->misc_ctrl + MISC_USB);
	mdelay(M_LEVEL3);
}

void hisi_usb_phy_on(struct phy *phy)
{
	unsigned int reg;
	struct hisi_priv *priv = phy_get_drvdata(phy);

	/* enable phy ref clk to enable phy */
	reg = readl(priv->peri_ctrl + PERI_CRG28);
	reg |= USB_CKEN;
	writel(reg, priv->peri_ctrl + PERI_CRG28);
	udelay(U_LEVEL5);

	/* config controller */
	reg = readl(priv->misc_ctrl + PERI_USB);
	reg &= ~(WORDINTERFACE); /* 8bit */
	reg &= ~(SS_BURST16_EN); /* disable ehci burst16 mode */
	reg |= USBOVR_P_CTRL;
	writel(reg, priv->misc_ctrl + PERI_USB);
	udelay(U_LEVEL5);

	/* de-assert phy port */
	reg = readl(priv->peri_ctrl + PERI_CRG28);
	reg &= ~(USBPHY_REQ);
	writel(reg, priv->peri_ctrl + PERI_CRG28);
	udelay(U_LEVEL7);

	/* open phy clk */
	writel(PHY_CLK_ENABLE, priv->misc_ctrl + MISC_USB);
	udelay(U_LEVEL1);
	writel(OPEN_PHY_CLK, priv->misc_ctrl + MISC_USB);
	mdelay(M_LEVEL3);

	usb_eye_config(phy);

	/* cancel phy utmi reset */
	reg = readl(priv->peri_ctrl + PERI_CRG28);
	reg &= ~(USBPHY_PORT0_TREQ);
	reg &= ~(USBPHY_PORT1_TREQ);
	writel(reg, priv->peri_ctrl + PERI_CRG28);
	udelay(U_LEVEL7);

	/* de-assert all the rsts of ctrl */
	reg = readl(priv->peri_ctrl + PERI_CRG28);
	reg &= ~(USB_CTRL_UTMI0_REG);
	reg &= ~(USB_CTRL_UTMI1_REG);
	reg &= ~(USB_CTRL_HUB_REG);
	reg &= ~(USB_AHB_SRST_REQ);
	writel(reg, priv->peri_ctrl + PERI_CRG28);
	udelay(U_LEVEL6);
}
EXPORT_SYMBOL(hisi_usb_phy_on);

void hisi_usb_phy_off(struct phy *phy)
{
	unsigned int reg;
	struct hisi_priv *priv = phy_get_drvdata(phy);

	/*
	 * Disable EHCI clock.
	 * If the HS PHY is unused disable it too.
	 */
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
	udelay(U_LEVEL5);

	/* enable phy */
	reg = readl(priv->misc_ctrl + PERI_USB);
	reg |= (WORDINTERFACE);
	reg |= (SS_BURST16_EN);
	reg |= (USBOVR_P_CTRL);
	writel(reg, priv->misc_ctrl + PERI_USB);
	udelay(U_LEVEL5);
}
EXPORT_SYMBOL(hisi_usb_phy_off);
