/*
 * Copyright (c) 2016-2017 HiSilicon Technologies Co., Ltd.
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

#include <linux/delay.h>
#include <linux/of_address.h>
#include <linux/phy/phy.h>

#include "phy-hisi-usb.h"

#define PERI_CRG91          0x16C
#define USB2_PHY1_REF_CKEN  (1 << 0)
#define USB2_PHY1_SRST_REQ  (1 << 8)
#define USB2_PHY1_SRST_TREQ (1 << 9)

#define PERI_CRG72              0x120
#define COMBPHY1_REFCLK1_SEL    (3 << 14)
#define COMBPHY1_LANE1_SRST_REQ (1 << 11)
#define COMBPHY1_REF1_CKEN      (1 << 9)

#define PERI_CRG75        0x12c
#define USB3_VCC_SRST_REQ (1 << 13)

#define REG_GUCTL1            0xc11c
#define PARKMODE_DISABLE_FSLS (0x1 << 15)
#define PARKMODE_DISABLE_HS   (0x1 << 16)
#define PARKMODE_DISABLE_SS   (0x1 << 17)

#define USB3_GUSB2PHYCFGN 0xc200
#define U2_FREECLK_EXIST  (1 << 6)

void usb_ctrl_config(struct phy *phy)
{
	int reg;
	struct hisi_priv *priv = phy_get_drvdata(phy);

	reg = readl(priv->ctrl_base + REG_GUCTL1);
	reg |= PARKMODE_DISABLE_FSLS;
	reg |= PARKMODE_DISABLE_HS;
	reg |= PARKMODE_DISABLE_SS;
	writel(reg, priv->ctrl_base + REG_GUCTL1);
	udelay(U_LEVEL2);

	reg = readl(priv->ctrl_base + USB3_GUSB2PHYCFGN);
	reg &= ~(U2_FREECLK_EXIST);
	writel_relaxed(reg, priv->ctrl_base + USB3_GUSB2PHYCFGN);
	udelay(U_LEVEL6);
}

void usb2_phy_config(struct phy *phy)
{
	int reg;
	struct hisi_priv *priv = phy_get_drvdata(phy);

	reg = readl(priv->peri_ctrl + PERI_CRG91);
	reg |= (USB2_PHY1_SRST_TREQ | USB2_PHY1_SRST_REQ);
	writel_relaxed(reg, priv->peri_ctrl + PERI_CRG91);
	udelay(U_LEVEL6);

	reg = readl(priv->peri_ctrl + PERI_CRG91);
	reg &= ~(USB2_PHY1_REF_CKEN);
	writel_relaxed(reg, priv->peri_ctrl + PERI_CRG91);
	udelay(U_LEVEL6);

	/* open usb2.0 bus clock */
	reg = readl(priv->peri_ctrl + PERI_CRG91);
	reg |= USB2_PHY1_REF_CKEN;
	writel_relaxed(reg, priv->peri_ctrl + PERI_CRG91);
	udelay(U_LEVEL6);

	reg = readl(priv->peri_ctrl + PERI_CRG91);
	reg &= ~(USB2_PHY1_SRST_REQ);
	writel_relaxed(reg, priv->peri_ctrl + PERI_CRG91);
	mdelay(M_LEVEL1);

	reg = readl(priv->peri_ctrl + PERI_CRG91);
	reg &= ~(USB2_PHY1_SRST_TREQ);
	writel_relaxed(reg, priv->peri_ctrl + PERI_CRG91);
	udelay(U_LEVEL6);

	reg = readl(priv->peri_ctrl + PERI_CRG72);
	reg &= ~(COMBPHY1_REFCLK1_SEL);
	writel_relaxed(reg, priv->peri_ctrl + PERI_CRG72);
	udelay(U_LEVEL6);

	reg = readl(priv->peri_ctrl + PERI_CRG72);
	reg |= COMBPHY1_REF1_CKEN;
	writel_relaxed(reg, priv->peri_ctrl + PERI_CRG72);
	udelay(U_LEVEL6);
}

void hisi_usb3_phy_on(struct phy *phy)
{
	int reg;
	struct hisi_priv *priv = phy_get_drvdata(phy);

	/* reset enable */
	reg = readl(priv->peri_ctrl + PERI_CRG75);
	reg |= (USB3_VCC_SRST_REQ);
	writel_relaxed(reg, priv->peri_ctrl + PERI_CRG75);
	udelay(U_LEVEL6);

	reg = readl(priv->peri_ctrl + PERI_CRG72);
	reg |= (COMBPHY1_LANE1_SRST_REQ);
	writel_relaxed(reg, priv->peri_ctrl + PERI_CRG72);
	udelay(U_LEVEL6);

	usb2_phy_config(phy);

	/* cancel reset */
	reg = readl(priv->peri_ctrl + PERI_CRG72);
	reg &= ~(COMBPHY1_LANE1_SRST_REQ);
	writel_relaxed(reg, priv->peri_ctrl + PERI_CRG72);
	udelay(U_LEVEL5);

	reg = readl(priv->peri_ctrl + PERI_CRG75);
	reg &= ~(USB3_VCC_SRST_REQ);
	writel_relaxed(reg, priv->peri_ctrl + PERI_CRG75);
	udelay(U_LEVEL6);

	usb_ctrl_config(phy);
}
EXPORT_SYMBOL(hisi_usb3_phy_on);

void hisi_usb3_phy_off(struct phy *phy)
{
	int reg;
	struct hisi_priv *priv = phy_get_drvdata(phy);

	/* reset enable */
	reg = readl(priv->peri_ctrl + PERI_CRG75);
	reg |= (USB3_VCC_SRST_REQ);
	writel_relaxed(reg, priv->peri_ctrl + PERI_CRG75);
	udelay(U_LEVEL6);

	reg = readl(priv->peri_ctrl + PERI_CRG72);
	reg |= (COMBPHY1_LANE1_SRST_REQ);
	writel_relaxed(reg, priv->peri_ctrl + PERI_CRG72);
	udelay(U_LEVEL6);

	reg = readl(priv->peri_ctrl + PERI_CRG91);
	reg |= (USB2_PHY1_SRST_TREQ | USB2_PHY1_SRST_REQ);
	writel_relaxed(reg, priv->peri_ctrl + PERI_CRG91);
	udelay(U_LEVEL6);

	reg = readl(priv->peri_ctrl + PERI_CRG91);
	reg &= ~(USB2_PHY1_REF_CKEN);
	writel_relaxed(reg, priv->peri_ctrl + PERI_CRG91);
	udelay(U_LEVEL6);
}
EXPORT_SYMBOL(hisi_usb3_phy_off);
