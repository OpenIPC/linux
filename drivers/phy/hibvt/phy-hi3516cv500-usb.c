/*
 * phy-hi3516cv500-usb.c
 *
 * USB phy driver.
 *
 * Copyright (C) Hisilicon Technologies Co., Ltd. 2018-2019. All rights reserved.
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
#include <linux/usb/ch9.h>

#include "phy-hisi-usb.h"

#define CRG_BASE_REG           0x140
#define USB2_UTMI_PCTRL        (0x1 << 15)
#define USB2_PHY_TEST_SRST_REQ (0x1 << 14)
#define USB2_UTMI_CKSEL        (0x1 << 13)
#define USB2_UTMI_CKEN         (0x1 << 12)
#define USB2_REF_CKEN          (0x1 << 9)
#define USB2_BUS_CKEN          (0x1 << 8)
#define USB2_VCC_SRST_REQ      (0x1 << 3)
#define USB2_PHY_CKEN          (0x1 << 2)
#define USB2_PHY_PORT_TREQ     (0x1 << 1)
#define USB2_PHY_REQ           (0x1 << 0)

#define CTRL_BASE_REG 0x100e0000

#define REG_GUSB3PIPECTL0    0xc2c0
#define PCS_SSP_SOFT_RESET   (0x1 << 31)
#define PORT_DISABLE_SUSPEND (0x1 << 17)

#define REG_GCTL      0xc110
#define PORT_CAP_DIR  (0x3 << 12)
#define PORT_SET_HOST (0x1 << 12)

#define GTXTHRCFG       0xc108
#define USB2_G_TXTHRCFG 0x23100000

#define GRXTHRCFG       0xc10c
#define USB2_G_RXTHRCFG 0x23100000

#define REG_GUCTL1            0xc11c
#define PARKMODE_DISABLE_FSLS (0x1 << 15)
#define PARKMODE_DISABLE_HS   (0x1 << 16)
#define PARKMODE_DISABLE_SS   (0x1 << 17)

#define USB2_INNO_PHY_BASE_REG  0x10110000
#define USB2_PHY_CLK_OUTPUT_REG 0x18
#define USB2_PHY_CLK_OUTPUT_VAL 0x0c
#define USB2_INNO_TRIM_OFFSET   0x0c

#define USB2_VBUS_IO_BASE_REG 0x10ff0000
#define USB2_VBUS_IO_OFFSET   0x40
#define USB2_VBUS_IO_VAL      0x431

#define USB_TRIM_BASE_REG    0x100a0000
#define USB_TRIM_OFFSET      0x38
#define USB_INNO_TRIM_MASK   0x7c
#define USB_INNO_TRIM_VAL(a) (((a) << 2) & USB_INNO_TRIM_MASK)
#define USB_TRIM_VAL_MASK    0x1f
#define USB_TRIM_VAL_MIN     0xf
#define USB_TRIM_VAL_MAX     0x1c

#define HS_HIGH_HEIGHT_TUNING_OFFSET 0x8
#define HS_HIGH_HEIGHT_TUNING_MASK   (0x7 << 4)
#define HS_HIGH_HEIGHT_TUNING_VAL    0x5 << 4

#define PRE_EMPHASIS_TUNING_OFFSET 0x0
#define PRE_EMPHASIS_TUNING_MASK   (0x7 << 0)
#define PRE_EMPHASIS_TUNING_VAL    0x7 << 0

#define PRE_EMPHASIS_STRENGTH_OFFSET 0x14
#define PRE_EMPHASIS_STRENGTH_MASK   (0x7 << 2)
#define PRE_EMPHASIS_STRENGTH_VAL    0x3 << 2

#define HS_SLEW_RATE_TUNING_OFFSET 0x74
#define HS_SLEW_RATE_TUNING_MASK   (0x7 << 1)
#define HS_SLEW_RATE_TUNING_VAL    0x7 << 1

#define DISCONNECT_TRIGGER_OFFSET 0x10
#define DISCONNECT_TRIGGER_MASK   (0xf << 4)
#define DISCONNECT_TRIGGER_VAL    0xd << 4

static void usb_vbus_multi_gpio(void)
{
	void __iomem *vbus = ioremap_nocache(USB2_VBUS_IO_BASE_REG, __1K__);
	if (vbus == NULL)
		return;

	writel(USB2_VBUS_IO_VAL, vbus + USB2_VBUS_IO_OFFSET);
	udelay(U_LEVEL2);

	iounmap(vbus);
}

static void usb_trim_c(void)
{
	unsigned int reg;
	unsigned int trim_val;
	void __iomem *inno_clk_output = NULL;
	void __iomem *usb_trim = NULL;

	inno_clk_output = ioremap_nocache(USB2_INNO_PHY_BASE_REG, __1K__);
	if (inno_clk_output == NULL)
		return;

	usb_trim = ioremap_nocache(USB_TRIM_BASE_REG, __1K__);
	if (usb_trim == NULL)
		goto free;

	/* set inno phy output clock */
	writel(USB2_PHY_CLK_OUTPUT_VAL, inno_clk_output + USB2_PHY_CLK_OUTPUT_REG);
	udelay(U_LEVEL2);

	/*
	 * USB Trim config:45ohm HS ODT value tuning & FS/LS
	 * driver strength tuning, adapt it to usb trim val.
	 */
	trim_val = readl(usb_trim + USB_TRIM_OFFSET);
	trim_val &= USB_TRIM_VAL_MASK;
	if ((trim_val >= USB_TRIM_VAL_MIN) && (trim_val <= USB_TRIM_VAL_MAX)) {
		reg = readl(inno_clk_output + USB2_INNO_TRIM_OFFSET);
		udelay(U_LEVEL2);
		reg &= ~USB_INNO_TRIM_MASK;
		reg |= USB_INNO_TRIM_VAL(trim_val);
		writel(reg, inno_clk_output + USB2_INNO_TRIM_OFFSET);
		udelay(U_LEVEL2);
	}

	iounmap(usb_trim);
free:
	iounmap(inno_clk_output);
	return;
}

static void usb_crg_c(struct phy *phy)
{
	unsigned int reg;
	struct hisi_priv *priv = phy_get_drvdata(phy);

	/* usb phy reset */
	reg = readl(priv->peri_ctrl + CRG_BASE_REG);
	reg |= USB2_PHY_TEST_SRST_REQ;
	writel(reg, priv->peri_ctrl + CRG_BASE_REG);
	udelay(U_LEVEL5);

	/* cancel usb phy srst */
	reg = readl(priv->peri_ctrl + CRG_BASE_REG);
	reg &= ~USB2_PHY_TEST_SRST_REQ;
	writel(reg, priv->peri_ctrl + CRG_BASE_REG);
	udelay(U_LEVEL2);

	/* usb2 vcc reset */
	reg = readl(priv->peri_ctrl + CRG_BASE_REG);
	reg |= USB2_VCC_SRST_REQ;
	writel(reg, priv->peri_ctrl + CRG_BASE_REG);
	udelay(U_LEVEL6);

	usb_trim_c();

	/* open phy ref cken, utmi pctrl and utmi cksel */
	reg = readl(priv->peri_ctrl + CRG_BASE_REG);
	reg |= USB2_PHY_CKEN;
	reg &= ~USB2_UTMI_PCTRL;
	reg &= ~USB2_UTMI_CKSEL;
	writel(reg, priv->peri_ctrl + CRG_BASE_REG);
	udelay(U_LEVEL1);

	/* open utmi cken and controller ref cken */
	reg = readl(priv->peri_ctrl + CRG_BASE_REG);
	reg |= USB2_UTMI_CKEN;
	reg |= USB2_REF_CKEN;
	writel(reg, priv->peri_ctrl + CRG_BASE_REG);
	udelay(U_LEVEL1);

	/* open bus cken */
	reg = readl(priv->peri_ctrl + CRG_BASE_REG);
	reg |= USB2_BUS_CKEN;
	writel(reg, priv->peri_ctrl + CRG_BASE_REG);
	udelay(U_LEVEL6);

	/* cancel POR reset, TPOR reset and vcc reset */
	reg = readl(priv->peri_ctrl + CRG_BASE_REG);
	reg &= ~USB2_PHY_REQ;
	reg &= ~USB2_PHY_PORT_TREQ;
	reg &= ~USB2_VCC_SRST_REQ;
	writel(reg, priv->peri_ctrl + CRG_BASE_REG);
	udelay(U_LEVEL6);
}

static void usb_ctrl_c(struct phy *phy)
{
	unsigned int reg;
	struct hisi_priv *priv = phy_get_drvdata(phy);

	priv->ctrl_base = ioremap_nocache(CTRL_BASE_REG, __64K__);
	if (priv->ctrl_base == NULL)
		return;

	reg = readl(priv->ctrl_base + REG_GUCTL1);
	reg |= PARKMODE_DISABLE_FSLS;
	reg |= PARKMODE_DISABLE_HS;
	reg |= PARKMODE_DISABLE_SS;
	writel(reg, priv->ctrl_base + REG_GUCTL1);
	udelay(U_LEVEL2);

	reg = readl(priv->ctrl_base + REG_GUSB3PIPECTL0);
	reg |= PCS_SSP_SOFT_RESET;
	writel(reg, priv->ctrl_base + REG_GUSB3PIPECTL0);
	udelay(U_LEVEL2);

	reg = readl(priv->ctrl_base + REG_GCTL);
	reg &= ~PORT_CAP_DIR;
	reg |= PORT_SET_HOST; /* [13:12] 01: Host; 10: Device; 11: OTG */
	writel(reg, priv->ctrl_base + REG_GCTL);
	udelay(U_LEVEL2);

	reg = readl(priv->ctrl_base + REG_GUSB3PIPECTL0);
	reg &= ~PCS_SSP_SOFT_RESET;
	reg &= ~PORT_DISABLE_SUSPEND;  /* disable suspend */
	writel(reg, priv->ctrl_base + REG_GUSB3PIPECTL0);
	udelay(U_LEVEL2);

	writel(USB2_G_TXTHRCFG, priv->ctrl_base + GTXTHRCFG);
	writel(USB2_G_RXTHRCFG, priv->ctrl_base + GRXTHRCFG);
	udelay(U_LEVEL2);

	iounmap(priv->ctrl_base);
}

static void usb_eye_c(struct phy *phy)
{
	unsigned int reg;
	void __iomem *inno_base = NULL;

	inno_base = ioremap_nocache(USB2_INNO_PHY_BASE_REG, __1K__);
	if (inno_base == NULL)
		return;

	/* HS eye height tuning */
	reg = readl(inno_base + HS_HIGH_HEIGHT_TUNING_OFFSET);
	reg &= ~HS_HIGH_HEIGHT_TUNING_MASK;
	reg |= HS_HIGH_HEIGHT_TUNING_VAL;
	writel(reg, inno_base + HS_HIGH_HEIGHT_TUNING_OFFSET);

	/* Pre-emphasis tuning */
	reg = readl(inno_base + PRE_EMPHASIS_TUNING_OFFSET);
	reg &= ~PRE_EMPHASIS_TUNING_MASK;
	reg |= PRE_EMPHASIS_TUNING_VAL;
	writel(reg, inno_base + PRE_EMPHASIS_TUNING_OFFSET);

	/* Pre-emphasis strength */
	reg = readl(inno_base + PRE_EMPHASIS_STRENGTH_OFFSET);
	reg &= ~PRE_EMPHASIS_STRENGTH_MASK;
	reg |= PRE_EMPHASIS_STRENGTH_VAL;
	writel(reg, inno_base + PRE_EMPHASIS_STRENGTH_OFFSET);

	/* HS driver slew rate tunning */
	reg = readl(inno_base + HS_SLEW_RATE_TUNING_OFFSET);
	reg &= ~HS_SLEW_RATE_TUNING_MASK;
	reg |= HS_SLEW_RATE_TUNING_VAL;
	writel(reg, inno_base + HS_SLEW_RATE_TUNING_OFFSET);

	/* HOST disconnects detection trigger point */
	reg = readl(inno_base + DISCONNECT_TRIGGER_OFFSET);
	reg &= ~DISCONNECT_TRIGGER_MASK;
	reg |= DISCONNECT_TRIGGER_VAL;
	writel(reg, inno_base + DISCONNECT_TRIGGER_OFFSET);
	iounmap(inno_base);
}

void hisi_usb_phy_on(struct phy *phy)
{
	usb_crg_c(phy);

	usb_vbus_multi_gpio();

	usb_eye_c(phy);

	usb_ctrl_c(phy);
}
EXPORT_SYMBOL(hisi_usb_phy_on);

void hisi_usb_phy_off(struct phy *phy)
{
	unsigned int reg;
	struct hisi_priv *priv = phy_get_drvdata(phy);

	/* usb2 vcc reset */
	reg = readl(priv->peri_ctrl + CRG_BASE_REG);
	reg |= USB2_VCC_SRST_REQ;
	writel(reg, priv->peri_ctrl + CRG_BASE_REG);
	udelay(U_LEVEL6);
}
EXPORT_SYMBOL(hisi_usb_phy_off);
