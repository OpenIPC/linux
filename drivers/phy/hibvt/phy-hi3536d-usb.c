/*
 * phy-hi3536d-usb.c
 *
 * USB phy driver.
 *
 * Copyright (C) Hisilicon Technologies Co., Ltd. 2017-2018. All rights reserved.
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
#include <linux/of_address.h>
#include <linux/phy/phy.h>

#include "phy-hisi-usb.h"

#define USB2_CTRL             0xb8
#define USB2_BUS_CKEN         (1 << 0)
#define USB2_OHCI48M_CKEN     (1 << 1)
#define USB2_OHCI12M_CKEN     (1 << 2)
#define USB2_HST_PHY_CKEN     (1 << 4)
#define USB2_UTMI0_CKEN       (1 << 5)
#define USB2_UTMI1_CKEN       (1 << 6)
#define USB2_BUS_SRST_REQ     (1 << 12)
#define USB2_UTMI0_SRST_REQ   (1 << 13)
#define USB2_UTMI1_SRST_REQ   (1 << 14)
#define USB2_HST_PHY_SYST_REQ (1 << 16)

#define REG_USB2_PHY0          0xbc
#define USB_PHY0_REF_CKEN      (1 << 0)
#define USB_PHY0_SRST_REQ      (1 << 1)
#define USB_PHY0_SRST_TREQ     (1 << 2)
#define USB_PHY1_SRST_TREQ     (1 << 3)
#define USB_PHY0_TEST_SRST_REQ (1 << 4)

#define MISC_CTRL_TRIM      0x50
#define usb_r_tuning_val(n) ((n) >> 21)
#define USB_R_TUNING_1      0x800c
#define USB_R_TUNING_2      0x840c
#define MISC_CTRL_23        (1 << 23)
#define MISC_CTRL_24        (1 << 24)
#define MISC_CTRL_25        (1 << 25)
#define MISC_CTRL_26        (1 << 26)
#define MISC_CTRL_27        (1 << 27)
#define TRIM_CONFIG_2       (1 << 2)
#define TRIM_CONFIG_3       (1 << 3)
#define TRIM_CONFIG_4       (1 << 4)
#define TRIM_CONFIG_5       (1 << 5)
#define TRIM_CONFIG_6       (1 << 6)

#define MISC_CTRL      (1 << 22)
#define REG_USB2_CFG   0x8018
#define USB2_CFG_VAL   0x05
#define USB2_PHY0_CTLL 0x54

/* param config */
#define EYE_HEIGHT_CFG   0x8008
#define EYE_HEIGHT_RECFG 0x8408
#define EYE_HEIGHT_VAL   0x5c

#define PRE_OPEN_CFG   0x8000
#define PRE_OPEN_RECFG 0x8400
#define PRE_OPEN_VAL   0x1c

#define PRE_IMP_CFG   0x8014
#define PRE_IMP_RECFG 0x8414
#define PRE_IMP_VAL   0x80

#define DISC_CFG     0x8010
#define DISC_RECFG   0x8410
#define DISC_CFG_VAL 0x17

void usb_phy_eye_config(struct phy *phy)
{
	struct hisi_priv *priv = phy_get_drvdata(phy);

	/* eye height config */
	writel_relaxed(EYE_HEIGHT_VAL, priv->misc_ctrl + EYE_HEIGHT_CFG);
	udelay(U_LEVEL5);
	writel_relaxed(EYE_HEIGHT_VAL, priv->misc_ctrl + EYE_HEIGHT_RECFG);
	udelay(U_LEVEL5);

	/* pre open */
	writel_relaxed(PRE_OPEN_VAL, priv->misc_ctrl + PRE_OPEN_CFG);
	udelay(U_LEVEL5);
	writel_relaxed(PRE_OPEN_VAL, priv->misc_ctrl + PRE_OPEN_RECFG);
	udelay(U_LEVEL5);

	/* pre improve */
	writel_relaxed(PRE_IMP_VAL, priv->misc_ctrl + PRE_IMP_CFG);
	udelay(U_LEVEL5);
	writel_relaxed(PRE_IMP_VAL, priv->misc_ctrl + PRE_IMP_RECFG);
	udelay(U_LEVEL5);

	/* disconnects */
	writel_relaxed(DISC_CFG_VAL, priv->misc_ctrl + DISC_CFG);
	udelay(U_LEVEL5);
	writel_relaxed(DISC_CFG_VAL, priv->misc_ctrl + DISC_RECFG);
	udelay(U_LEVEL5);
}

void usb_trim_config(struct phy *phy)
{
	unsigned int reg, trim_reg;
	struct hisi_priv *priv = phy_get_drvdata(phy);

	/* Trim config */
	reg = readl(priv->misc_ctrl + MISC_CTRL_TRIM);
	reg &= (MISC_CTRL_23 | MISC_CTRL_24 | MISC_CTRL_25 | MISC_CTRL_26 |
		MISC_CTRL_27);
	reg = usb_r_tuning_val(reg);
	if (reg) {
		trim_reg = readl(priv->misc_ctrl + USB_R_TUNING_1);
		trim_reg &= ~(TRIM_CONFIG_2 | TRIM_CONFIG_3 | TRIM_CONFIG_4 |
			      TRIM_CONFIG_5 | TRIM_CONFIG_6);
		trim_reg |= reg;
		writel(trim_reg, priv->misc_ctrl + USB_R_TUNING_1);

		trim_reg = readl(priv->misc_ctrl + USB_R_TUNING_2);
		trim_reg &= ~(TRIM_CONFIG_2 | TRIM_CONFIG_3 | TRIM_CONFIG_4 |
			      TRIM_CONFIG_5 | TRIM_CONFIG_6);
		trim_reg |= reg;
		writel(trim_reg, priv->misc_ctrl + USB_R_TUNING_2);
	}
}
void hisi_usb_phy_on(struct phy *phy)
{
	unsigned int reg;
	struct hisi_priv *priv = phy_get_drvdata(phy);

	/* misc ctrl */
	reg = readl(priv->misc_ctrl + USB2_PHY0_CTLL);
	reg |= MISC_CTRL;
	writel_relaxed(reg, priv->misc_ctrl + USB2_PHY0_CTLL);
	udelay(U_LEVEL6);

	/* reset enable */
	reg = readl(priv->peri_ctrl + USB2_CTRL);
	reg |= (USB2_BUS_SRST_REQ | USB2_UTMI0_SRST_REQ |
		USB2_HST_PHY_SYST_REQ | USB2_UTMI1_SRST_REQ);
	writel(reg, priv->peri_ctrl + USB2_CTRL);
	udelay(U_LEVEL6);

	reg = readl(priv->peri_ctrl + REG_USB2_PHY0);
	reg |= (USB_PHY0_SRST_REQ | USB_PHY0_SRST_TREQ | USB_PHY1_SRST_TREQ);
	writel(reg, priv->peri_ctrl + REG_USB2_PHY0);
	udelay(U_LEVEL6);

	/* open ref clock */
	reg = readl(priv->peri_ctrl + REG_USB2_PHY0);
	reg |= (USB_PHY0_REF_CKEN);
	writel(reg, priv->peri_ctrl + REG_USB2_PHY0);
	udelay(U_LEVEL5);

	/* cancel power on reset */
	reg = readl(priv->peri_ctrl + REG_USB2_PHY0);
	reg &= ~(USB_PHY0_SRST_REQ);
	reg &= ~(USB_PHY0_TEST_SRST_REQ);
	writel(reg, priv->peri_ctrl + REG_USB2_PHY0);
	udelay(U_LEVEL7);

	writel(USB2_CFG_VAL, priv->misc_ctrl + REG_USB2_CFG);
	udelay(U_LEVEL6);

	/* cancel port reset */
	reg = readl(priv->peri_ctrl + REG_USB2_PHY0);
	reg &= ~(USB_PHY0_SRST_TREQ | USB_PHY1_SRST_TREQ);
	writel(reg, priv->peri_ctrl + REG_USB2_PHY0);
	udelay(U_LEVEL7);

	/* cancel control reset */
	reg = readl(priv->peri_ctrl + USB2_CTRL);
	reg &= ~(USB2_BUS_SRST_REQ | USB2_UTMI0_SRST_REQ |
		 USB2_HST_PHY_SYST_REQ | USB2_UTMI1_SRST_REQ);
	reg |= (USB2_BUS_CKEN | USB2_OHCI48M_CKEN | USB2_OHCI12M_CKEN |
		USB2_HST_PHY_CKEN | USB2_UTMI0_CKEN | USB2_UTMI1_CKEN);
	writel(reg, priv->peri_ctrl + USB2_CTRL);
	udelay(U_LEVEL6);

	usb_trim_config(phy);

	usb_phy_eye_config(phy);
}
EXPORT_SYMBOL(hisi_usb_phy_on);

void hisi_usb_phy_off(struct phy *phy)
{
	unsigned int reg;
	struct hisi_priv *priv = phy_get_drvdata(phy);

	reg = readl(priv->peri_ctrl + REG_USB2_PHY0);
	reg |= (USB_PHY0_SRST_REQ | USB_PHY0_SRST_TREQ | USB_PHY1_SRST_TREQ);
	writel(reg, priv->peri_ctrl + REG_USB2_PHY0);
	udelay(U_LEVEL5);

	/* close clock */
	reg = readl(priv->peri_ctrl + REG_USB2_PHY0);
	reg &= ~USB_PHY0_REF_CKEN;
	writel(reg, priv->peri_ctrl + REG_USB2_PHY0);
	udelay(U_LEVEL7);

	/* close clock */
	reg = readl(priv->peri_ctrl + USB2_CTRL);
	reg &= ~(USB2_BUS_CKEN | USB2_OHCI48M_CKEN | USB2_OHCI12M_CKEN |
		 USB2_HST_PHY_CKEN | USB2_UTMI0_CKEN | USB2_UTMI1_CKEN);
	writel(reg, priv->peri_ctrl + USB2_CTRL);
	udelay(U_LEVEL6);
}
EXPORT_SYMBOL(hisi_usb_phy_off);
