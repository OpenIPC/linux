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

#define IO_REG_USB2_CTRL		0x0130
#define USB2_BUS_CKEN		(1 << 0)
#define USB2_OHCI48M_CKEN	(1 << 1)
#define USB2_OHCI12M_CKEN	(1 << 2)
#define USB2_HST_PHY_CKEN	(1 << 4)
#define USB2_UTMI0_CKEN		(1 << 5)
#define USB2_BUS_SRST_REQ	(1 << 12)
#define USB2_UTMI0_SRST_REQ	(1 << 13)
#define USB2_HST_PHY_SYST_REQ	(1 << 16)

#define IO_REG_USB2_PHY0		0x0134
#define USB_PHY0_REF_CKEN	(1 << 0)
#define USB_PHY0_SRST_REQ	(1 << 8)
#define USB_PHY0_SRST_TREQ	(1 << 9)
#define USB_PHY0_REFCLK_SEL	(1 << 16)

#define IO_REG_USB2_CTRL0		0x0090
#define WORDINTERFACE		(1 << 0)
#define SS_BURST4_EN		(1 << 7)
#define SS_BURST8_EN		(1 << 8)
#define SS_BURST16_EN		(1 << 9)

#define	IO_REG_USB2_CTRL1		0x0094
/* write(0x1 << 5) 0x6 to addr 0x4 */
#define CONFIG_CLK		((0x1 << 5) | (0x6 << 0) | (0x4 << 8))

void hisi_usb_phy_on(struct phy *phy)
{
		int reg;
		struct hisi_priv *priv = phy_get_drvdata(phy);
		/* reset enable */
		reg = readl(priv->peri_ctrl + IO_REG_USB2_CTRL);
		reg |= (USB2_BUS_SRST_REQ
			| USB2_UTMI0_SRST_REQ
			| USB2_HST_PHY_SYST_REQ);

		writel(reg, priv->peri_ctrl + IO_REG_USB2_CTRL);
		udelay(200);

		reg = readl(priv->peri_ctrl + IO_REG_USB2_PHY0);
		reg |= (USB_PHY0_SRST_REQ
			| USB_PHY0_SRST_TREQ);
		writel(reg, priv->peri_ctrl + IO_REG_USB2_PHY0);
		udelay(200);

		reg = readl(priv->misc_ctrl + IO_REG_USB2_CTRL0);
		reg &= ~(WORDINTERFACE);	/* 8bit */
		reg &= ~(SS_BURST16_EN);	/* 16 bit burst disable */
		writel(reg, priv->misc_ctrl + IO_REG_USB2_CTRL0);
		udelay(100);

		/* for ssk usb storage ok */
		msleep(20);

		/* open ref clock */
		reg = readl(priv->peri_ctrl + IO_REG_USB2_PHY0);
		reg |= (USB_PHY0_REF_CKEN);
		writel(reg, priv->peri_ctrl + IO_REG_USB2_PHY0);
		udelay(100);

		/* cancel power on reset */
		reg = readl(priv->peri_ctrl + IO_REG_USB2_PHY0);
		reg &= ~(USB_PHY0_SRST_REQ);
		writel(reg , priv->peri_ctrl + IO_REG_USB2_PHY0);
		udelay(300);

		/* config clock */
		writel(0x0, priv->misc_ctrl + IO_REG_USB2_CTRL1);
		mdelay(200);
		reg = readl(priv->misc_ctrl + IO_REG_USB2_CTRL1);
		reg |= CONFIG_CLK;
		writel(reg, priv->misc_ctrl + IO_REG_USB2_CTRL1);
		udelay(100);

		/* writel(0x0, IO_REG_USB2_CTRL1); */
		/* mdelay(2); */

		/* config u2 eye diagram */
		/* close HS pre-emphasis */
		writel(0x0, priv->misc_ctrl + IO_REG_USB2_CTRL1);
		udelay(10);
		writel(0x1820, priv->misc_ctrl + IO_REG_USB2_CTRL1);
		udelay(100);

		/* cancel port reset */
		reg = readl(priv->peri_ctrl + IO_REG_USB2_PHY0);
		reg &= ~(USB_PHY0_SRST_TREQ);
		writel(reg, priv->peri_ctrl + IO_REG_USB2_PHY0);
		udelay(300);

		/* cancel control reset */
		reg = readl(priv->peri_ctrl + IO_REG_USB2_CTRL);
		reg &= ~(USB2_BUS_SRST_REQ
			| USB2_UTMI0_SRST_REQ
			| USB2_HST_PHY_SYST_REQ);

		reg |= (USB2_BUS_CKEN
			| USB2_OHCI48M_CKEN
			| USB2_OHCI12M_CKEN
			| USB2_HST_PHY_CKEN
			| USB2_UTMI0_CKEN);
		writel(reg, priv->peri_ctrl + IO_REG_USB2_CTRL);
		udelay(200);
}

EXPORT_SYMBOL(hisi_usb_phy_on);

void hisi_usb_phy_off(struct phy *phy)
{
		int reg;
		struct hisi_priv *priv = phy_get_drvdata(phy);

		reg = readl(priv->peri_ctrl + IO_REG_USB2_PHY0);
		reg |= (USB_PHY0_SRST_REQ
			| USB_PHY0_SRST_TREQ);
		writel(reg, priv->peri_ctrl + IO_REG_USB2_PHY0);
		udelay(100);

		/* close clock */
		reg = readl(priv->peri_ctrl + IO_REG_USB2_PHY0);
		reg &= ~(USB_PHY0_REFCLK_SEL
			| USB_PHY0_REF_CKEN);
		writel(reg, priv->peri_ctrl + IO_REG_USB2_PHY0);
		udelay(300);

		/* close clock */
		reg = readl(priv->peri_ctrl + IO_REG_USB2_CTRL);
		reg &= ~(USB2_BUS_CKEN
			| USB2_OHCI48M_CKEN
			| USB2_OHCI12M_CKEN
			| USB2_HST_PHY_CKEN
			| USB2_UTMI0_CKEN);
		writel(reg, priv->peri_ctrl + IO_REG_USB2_CTRL);
		udelay(200);
}
EXPORT_SYMBOL(hisi_usb_phy_off);
