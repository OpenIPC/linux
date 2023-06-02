/*
 * Copyright (c) 2016-2017 HiSilicon Technologies Co., Ltd.
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
#include <linux/phy/phy.h>
#include <linux/of_address.h>
#include "phy-hisi-usb.h"

#define PERI_CRG91					0x16C
#define usb2_phy1_ref_cken			(1 << 0)
#define usb2_phy1_srst_req			(1 << 8)
#define usb2_phy1_srst_treq			(1 << 9)

#define PERI_CRG72					0x120
#define combphy1_refclk1_sel		(3 <<14)
#define combphy1_lane1_srst_req		(1 <<11)
#define combphy1_ref1_cken			(1 << 9)

#define PERI_CRG75					0x12c
#define usb3_vcc_srst_req			(1 <<13)


void hisi_usb3_phy_on(struct phy *phy)
{
	int reg;
	struct hisi_priv *priv = phy_get_drvdata(phy);

	/* reset enable */
	reg = readl(priv->peri_ctrl + PERI_CRG75);
	reg |= (usb3_vcc_srst_req);
	writel_relaxed(reg, priv->peri_ctrl + PERI_CRG75);
	udelay(200);
	reg = readl(priv->peri_ctrl + PERI_CRG72);
	reg |= (combphy1_lane1_srst_req);
	writel_relaxed(reg, priv->peri_ctrl + PERI_CRG72);
	udelay(200);
	reg = readl(priv->peri_ctrl + PERI_CRG91);
	reg |= (usb2_phy1_srst_treq | usb2_phy1_srst_req);
	writel_relaxed(reg, priv->peri_ctrl + PERI_CRG91);
	udelay(200);
	reg = readl(priv->peri_ctrl + PERI_CRG91);
	reg &= ~(usb2_phy1_ref_cken);
	writel_relaxed(reg, priv->peri_ctrl + PERI_CRG91);
	udelay(200);

	/*open usb2.0 bus clock*/
	reg = readl(priv->peri_ctrl + PERI_CRG91);
	reg |= usb2_phy1_ref_cken;
	writel_relaxed(reg, priv->peri_ctrl + PERI_CRG91);
	udelay(200);

	reg = readl(priv->peri_ctrl + PERI_CRG91);
	reg &= ~(usb2_phy1_srst_req);
	writel_relaxed(reg, priv->peri_ctrl + PERI_CRG91);
	mdelay(2);

	reg = readl(priv->peri_ctrl + PERI_CRG91);
	reg &= ~(usb2_phy1_srst_treq);
	writel_relaxed(reg, priv->peri_ctrl + PERI_CRG91);
	udelay(200);

	reg = readl(priv->peri_ctrl + PERI_CRG72);
	reg &= ~(combphy1_refclk1_sel);
	writel_relaxed(reg, priv->peri_ctrl + PERI_CRG72);
	udelay(200);
	
	reg = readl(priv->peri_ctrl + PERI_CRG72);
	reg |= combphy1_ref1_cken;
	writel_relaxed(reg, priv->peri_ctrl + PERI_CRG72);
	udelay(200);
	/* cancel reset */
	reg = readl(priv->peri_ctrl + PERI_CRG72);
	reg &= ~(combphy1_lane1_srst_req);
	writel_relaxed(reg, priv->peri_ctrl + PERI_CRG72);
	udelay(100);

	reg = readl(priv->peri_ctrl + PERI_CRG75);
	reg &= ~(usb3_vcc_srst_req);
	writel_relaxed(reg, priv->peri_ctrl + PERI_CRG75);
	udelay(200);
}
EXPORT_SYMBOL(hisi_usb3_phy_on);


void hisi_usb3_phy_off(struct phy *phy)
{
	int reg;
	struct hisi_priv *priv = phy_get_drvdata(phy);

	/* reset enable */
	reg = readl(priv->peri_ctrl + PERI_CRG75);
	reg |= (usb3_vcc_srst_req);
	writel_relaxed(reg, priv->peri_ctrl + PERI_CRG75);
	udelay(200);
	reg = readl(priv->peri_ctrl + PERI_CRG72);
	reg |= (combphy1_lane1_srst_req);
	writel_relaxed(reg, priv->peri_ctrl + PERI_CRG72);
	udelay(200);
	reg = readl(priv->peri_ctrl + PERI_CRG91);
	reg |= (usb2_phy1_srst_treq | usb2_phy1_srst_req);
	writel_relaxed(reg, priv->peri_ctrl + PERI_CRG91);
	udelay(200);
	reg = readl(priv->peri_ctrl + PERI_CRG91);
	reg &= ~(usb2_phy1_ref_cken);
	writel_relaxed(reg, priv->peri_ctrl + PERI_CRG91);
	udelay(200);
	
}
EXPORT_SYMBOL(hisi_usb3_phy_off);
