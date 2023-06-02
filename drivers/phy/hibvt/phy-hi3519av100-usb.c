/*
 * phy-hi3519av100-usb.c
 *
 * USB phy driver.
 *
 * Copyright (C) Hisilicon Technologies Co., Ltd. 2018-2019. All rights reserved.
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under the terms of  the GNU General Public License as published by the
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
#include <linux/usb/ch9.h>

#include "phy-hisi-usb.h"

#define USB3_CTRL_REGBASE 0x04110000
#define USB2_CTRL_REGBASE 0x04120000
#define PORT_CAP_DIR      (0x3 << 12)
#define DEFAULT_HOST_MOD  (0x1 << 12)

#define USB2_PHY            0x184
#define USB2_PHY0_CKEN      (0x1 << 5)
#define USB2_PHY1_CKEN      (0x1 << 4)
#define USB2_PHY0_PORT_TREQ (0x1 << 3)
#define USB2_PHY1_PORT_TREQ (0x1 << 2)
#define USB2_PHY0_REQ       (0x1 << 1)
#define USB2_PHY1_REQ       (0x1 << 0)

#define USB3_COMBPHY      0x188
#define COMBPHY0_REF_CKEN (0x1 << 8)
#define COMBPHY_SRST_REQ  (0x1 << 0)

#define USB3_CTRL         0x190
#define USB3_PCLK_OCC_SEL (0x1 << 30)
#define USB3_UTMI_CKSEL   (0x1 << 29)
#define USB3_VCC_SRST_REQ (0x1 << 16)
#define USB2_UTMI_CKSEL   (0x1 << 13)
#define USB2_VCC_SRST_REQ (0x1 << 0)

#define GTXTHRCFG 0xc108
#define GRXTHRCFG 0xc10c
#define REG_GCTL  0xc110

#define REG_GUCTL1            0xc11c
#define PARKMODE_DISABLE_FSLS (0x1 << 15)
#define PARKMODE_DISABLE_HS   (0x1 << 16)
#define PARKMODE_DISABLE_SS   (0x1 << 17)

#define PERI_USB3_GTXTHRCFG 0x2310000

#define REG_GUSB3PIPECTL0   0xc2c0
#define PCS_SSP_SOFT_RESET  (0x1 << 31)
#define SUSPEND_USB3_SS_PHY (0x1 << 17)
#define USB3_TX_MARGIN      (0x7 << 3)
#define USB3_TX_MARGIN_VAL  (0x2 << 3)

#define PORT0_CTRL 0x38
#define U3_ENABLE  (0x1 << 3)

#define USB2_PHY0            0x24
#define USB2_PHY0_TXVREFTUNE (0xf << 4)
#define USB2_PHY0_VREF_VAL   (0x5 << 4)
#define USB2_PHY0_TXPRE      (0x3 << 12)
#define USB2_PHY0_PRE_VAL    (0x1 << 12)

#define USB2_PHY1            0x30
#define USB2_PHY1_TXVREFTUNE (0xf << 4)
#define USB2_PHY1_VREF_VAL   (0x5 << 4)
#define USB2_PHY1_TXPRE      (0x3 << 12)
#define USB2_PHY1_PRE_VAL    (0x1 << 12)

#define USB3_PCIE_COMBO_PHY   0x14
#define TX_SWING_COMP_CFG     0x913
#define TX_SWING_COMP_RCFG    0x953
#define TX_SWING_COMP_CFG_VAL 0x913

#define SYSCTRL_REGBASE     0x04520000
#define SYSSTAT             0x8c
#define PCIE_USB3_MASK      (0x3 << 12)
#define get_combphy_mode(p) (((p) & PCIE_USB3_MASK) >> 12)
#define USB3                0x1
#define PCIE_X1             0x0

#define HPM_INFO_OFFSET   0x158
#define HPM_INFO_MASK     0x1ff
#define hpm_core_val(p)   (((p) >> 18) & HPM_INFO_MASK)
#define KEEP_DEFAULT_FLAG 0x18e

#define USB3_DEF_CRG      0x1f010000
#define USB3_DEF_CFG_MASK 0xffff0000
#define USB2_DEF_CRG      0x00001301
#define USB2_DEF_CFG_MASK 0x0000ffff

#define COMBPHY_IN_USE	0x1
#define COMBPHY_NO_IN_USE	0x0

void hisi_usb_crg_config(struct phy *phy)
{
	unsigned int reg;
	struct hisi_priv *priv = phy_get_drvdata(phy);

	/* set usb2 CRG default val */
	reg = readl(priv->peri_ctrl + USB3_CTRL);
	reg &= ~(USB2_DEF_CFG_MASK);
	reg |= USB2_DEF_CRG;
	writel(reg, priv->peri_ctrl + USB3_CTRL);
	udelay(U_LEVEL6);

	/* U2 vcc reset */
	reg = readl(priv->peri_ctrl + USB3_CTRL);
	reg |= USB2_VCC_SRST_REQ;
	writel(reg, priv->peri_ctrl + USB3_CTRL);
	udelay(U_LEVEL5);

	/* release TPOR default release */
	reg = readl(priv->peri_ctrl + USB2_PHY);
	reg &= ~USB2_PHY1_PORT_TREQ;
	writel(reg, priv->peri_ctrl + USB2_PHY);
	udelay(U_LEVEL6);

	/* utmi clock sel */
	reg = readl(priv->peri_ctrl + USB3_CTRL);
	reg &= ~USB2_UTMI_CKSEL;
	writel(reg, priv->peri_ctrl + USB3_CTRL);
	udelay(U_LEVEL6);

	/* open phy ref clk default open */
	reg = readl(priv->peri_ctrl + USB2_PHY);
	reg |= USB2_PHY1_CKEN;
	writel(reg, priv->peri_ctrl + USB2_PHY);
	udelay(U_LEVEL6);

	/* U2 phy reset release */
	reg = readl(priv->peri_ctrl + USB2_PHY);
	reg &= ~USB2_PHY1_REQ;
	writel(reg, priv->peri_ctrl + USB2_PHY);
	udelay(U_LEVEL6);

	/* config U2 Controller release */
	reg = readl(priv->peri_ctrl + USB3_CTRL);
	reg &= ~USB2_VCC_SRST_REQ;
	writel(reg, priv->peri_ctrl + USB3_CTRL);
	udelay(U_LEVEL5);
}

void hisi_usb_ctrl_config(struct phy *phy)
{
	unsigned int reg;
	struct hisi_priv *priv = phy_get_drvdata(phy);

	priv->ctrl_base = ioremap_nocache(USB2_CTRL_REGBASE, __64K__);
	if (priv->ctrl_base == NULL)
		return;

	reg = readl(priv->ctrl_base + REG_GUCTL1);
	reg |= PARKMODE_DISABLE_FSLS;
	reg |= PARKMODE_DISABLE_HS;
	reg |= PARKMODE_DISABLE_SS;
	writel(reg, priv->ctrl_base + REG_GUCTL1);
	udelay(U_LEVEL2);

	/* u2 port default host */
	reg = readl(priv->ctrl_base + REG_GCTL);
	reg &= ~PORT_CAP_DIR;
	reg |= DEFAULT_HOST_MOD;
	writel(reg, priv->ctrl_base + REG_GCTL);

	iounmap(priv->ctrl_base);
}

void hisi_usb_eye_config(struct phy *phy)
{
	unsigned int reg;
	struct hisi_priv *priv = phy_get_drvdata(phy);

	/* port0 phy high-spped DC adjust: 0% --> 4% */
	/* port0 pre elec adjust: 0 --> 1x */
	reg = readl(priv->misc_ctrl + USB2_PHY0);
	reg &= ~USB2_PHY0_TXVREFTUNE;
	reg &= ~USB2_PHY0_TXPRE;
	reg |= USB2_PHY0_VREF_VAL;
	reg |= USB2_PHY0_PRE_VAL;
	writel(reg, priv->misc_ctrl + USB2_PHY0);
	udelay(U_LEVEL5);

	/* port1 phy high-spped DC adjust: 0% --> 4% */
	/* port1 pre elec adjust: 0 --> 1x */
	reg = readl(priv->misc_ctrl + USB2_PHY1);
	reg &= ~USB2_PHY1_TXVREFTUNE;
	reg &= ~USB2_PHY1_TXPRE;
	reg |= USB2_PHY1_VREF_VAL;
	reg |= USB2_PHY1_PRE_VAL;
	writel(reg, priv->misc_ctrl + USB2_PHY1);
	udelay(U_LEVEL5);
}

void usb_combphy_config(struct phy *phy, unsigned rst)
{
	unsigned int reg;
	struct hisi_priv *priv = phy_get_drvdata(phy);

	priv->sys_ctrl = ioremap_nocache(SYSCTRL_REGBASE, __4K__);
	if (priv->sys_ctrl == NULL)
		return;

	if (rst) {
		/*
		 * According to description of SYSSYAT register, different
		 * HW modes of COMBPHY require specific configurations.
		 */
		reg = readl(priv->sys_ctrl + SYSSTAT);
		if (get_combphy_mode(reg) == USB3) {
			/* combphy reset in USB3 mode */
			reg = readl(priv->peri_ctrl + USB3_COMBPHY);
			reg |= COMBPHY_SRST_REQ;
			writel(reg, priv->peri_ctrl + USB3_COMBPHY);
			udelay(U_LEVEL5);
		} else if (get_combphy_mode(reg) == PCIE_X1) {
			/* Choose clock from CRG in PCIE X1 mode */
			reg = readl(priv->peri_ctrl + USB3_CTRL);
			reg |= USB3_PCLK_OCC_SEL;
			writel(reg, priv->peri_ctrl + USB3_CTRL);
			udelay(U_LEVEL5);
		}
	} else {
		/* Release the specific configuration of COMBPHY. */
		reg = readl(priv->sys_ctrl + SYSSTAT);
		if (get_combphy_mode(reg) == USB3) {
			/* open ref CKEN */
			reg = readl(priv->peri_ctrl + USB3_COMBPHY);
			reg |= COMBPHY0_REF_CKEN;
			writel(reg, priv->peri_ctrl + USB3_COMBPHY);
			udelay(U_LEVEL5);

			reg = readl(priv->peri_ctrl + USB3_COMBPHY);
			reg &= ~COMBPHY_SRST_REQ;
			writel(reg, priv->peri_ctrl + USB3_COMBPHY);
			udelay(U_LEVEL5);
		}
	}

	iounmap(priv->sys_ctrl);
}

void hisi_usb3_crg_config(struct phy *phy)
{
	unsigned int reg;
	struct hisi_priv *priv = phy_get_drvdata(phy);

	/* set usb3 CRG default val */
	reg = readl(priv->peri_ctrl + USB3_CTRL);
	reg &= ~(USB3_DEF_CFG_MASK);
	reg |= USB3_DEF_CRG;
	writel(reg, priv->peri_ctrl + USB3_CTRL);
	udelay(U_LEVEL6);

	/* enable ss port */
	reg = readl(priv->misc_ctrl + PORT0_CTRL);
	reg &= ~U3_ENABLE;
	writel(reg, priv->misc_ctrl + PORT0_CTRL);
	udelay(U_LEVEL5);

	usb_combphy_config(phy, COMBPHY_IN_USE);

	/* release TPOR default release */
	reg = readl(priv->peri_ctrl + USB2_PHY);
	reg &= ~USB2_PHY0_PORT_TREQ;
	writel(reg, priv->peri_ctrl + USB2_PHY);
	udelay(U_LEVEL6);

	/* utmi clock sel */
	reg = readl(priv->peri_ctrl + USB3_CTRL);
	reg &= ~USB3_UTMI_CKSEL;
	writel(reg, priv->peri_ctrl + USB3_CTRL);
	udelay(U_LEVEL6);

	/* open phy ref clk default open */
	reg = readl(priv->peri_ctrl + USB2_PHY);
	reg |= USB2_PHY0_CKEN;
	writel(reg, priv->peri_ctrl + USB2_PHY);
	udelay(U_LEVEL6);

	/* U2 phy reset release */
	reg = readl(priv->peri_ctrl + USB2_PHY);
	reg &= ~USB2_PHY0_REQ;
	writel(reg, priv->peri_ctrl + USB2_PHY);
	udelay(U_LEVEL6);

	usb_combphy_config(phy, COMBPHY_NO_IN_USE);

	/* config U3 Controller release */
	reg = readl(priv->peri_ctrl + USB3_CTRL);
	reg &= ~USB3_VCC_SRST_REQ;
	writel(reg, priv->peri_ctrl + USB3_CTRL);
	udelay(U_LEVEL5);
}

void hisi_usb3_ctrl_config(struct phy *phy)
{
	unsigned int reg;
	struct hisi_priv *priv = phy_get_drvdata(phy);

	priv->ctrl_base = ioremap_nocache(USB3_CTRL_REGBASE, __64K__);
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
	udelay(U_LEVEL6);

	/* u3 port default host */
	reg = readl(priv->ctrl_base + REG_GCTL);
	reg &= ~PORT_CAP_DIR;
	reg |= DEFAULT_HOST_MOD;
	writel(reg, priv->ctrl_base + REG_GCTL);
	udelay(U_LEVEL2);

	reg = readl(priv->ctrl_base + REG_GUSB3PIPECTL0);
	reg &= ~PCS_SSP_SOFT_RESET;
	reg &= ~SUSPEND_USB3_SS_PHY;
	writel(reg, priv->ctrl_base + REG_GUSB3PIPECTL0);
	udelay(U_LEVEL2);

	writel(PERI_USB3_GTXTHRCFG, priv->ctrl_base + GTXTHRCFG);
	udelay(U_LEVEL2);

	iounmap(priv->ctrl_base);
}

void hisi_usb3_eye_config(struct phy *phy)
{
	unsigned int reg;
	struct hisi_priv *priv = phy_get_drvdata(phy);

	priv->ctrl_base = ioremap_nocache(USB3_CTRL_REGBASE, __64K__);
	if (priv->ctrl_base == NULL)
		return;

	priv->sys_ctrl = ioremap_nocache(SYSCTRL_REGBASE, __4K__);
	if (priv->sys_ctrl == NULL)
		goto usb_unmap;

	/*
	 * If HPM core less than or equal to FLAG, TX_SWING_COMP
	 * adjust 0x1000 --> 0x1001.
	 */
	reg = readl(priv->sys_ctrl + HPM_INFO_OFFSET);
	if (hpm_core_val(reg) <= KEEP_DEFAULT_FLAG) {
		writel(TX_SWING_COMP_CFG, priv->misc_ctrl + USB3_PCIE_COMBO_PHY);
		writel(TX_SWING_COMP_RCFG, priv->misc_ctrl + USB3_PCIE_COMBO_PHY);
		writel(TX_SWING_COMP_CFG_VAL, priv->misc_ctrl + USB3_PCIE_COMBO_PHY);
		udelay(U_LEVEL2);
	}

	/* usb3 Tx margin adjust: 0 --> 900mv */
	reg = readl(priv->ctrl_base + REG_GUSB3PIPECTL0);
	reg &= ~USB3_TX_MARGIN;
	reg |= USB3_TX_MARGIN_VAL;
	writel(reg, priv->ctrl_base + REG_GUSB3PIPECTL0);

	iounmap(priv->sys_ctrl);
usb_unmap:
	iounmap(priv->ctrl_base);
	return;
}

void hisi_usb_phy_on(struct phy *phy)
{
	hisi_usb_crg_config(phy);

	hisi_usb_ctrl_config(phy);

	hisi_usb_eye_config(phy);
}
EXPORT_SYMBOL(hisi_usb_phy_on);

void hisi_usb3_phy_on(struct phy *phy)
{
	hisi_usb3_crg_config(phy);

	hisi_usb3_ctrl_config(phy);

	hisi_usb3_eye_config(phy);
}
EXPORT_SYMBOL(hisi_usb3_phy_on);

void hisi_usb_phy_off(struct phy *phy)
{
	unsigned int reg;
	struct hisi_priv *priv = phy_get_drvdata(phy);

	/* U2 vcc reset */
	reg = readl(priv->peri_ctrl + USB3_CTRL);
	reg |= USB2_VCC_SRST_REQ;
	writel(reg, priv->peri_ctrl + USB3_CTRL);
	udelay(U_LEVEL5);
}
EXPORT_SYMBOL(hisi_usb_phy_off);

void hisi_usb3_phy_off(struct phy *phy)
{
	unsigned int reg;
	struct hisi_priv *priv = phy_get_drvdata(phy);

	priv->sys_ctrl = ioremap_nocache(SYSCTRL_REGBASE, __4K__);
	if (priv->sys_ctrl == NULL)
		return;

	/* U3 vcc reset */
	reg = readl(priv->peri_ctrl + USB3_CTRL);
	reg |= USB3_VCC_SRST_REQ;
	writel(reg, priv->peri_ctrl + USB3_CTRL);
	udelay(U_LEVEL5);

	reg = readl(priv->sys_ctrl + SYSSTAT);
	if (get_combphy_mode(reg) == USB3) {
		/* combphy reset */
		reg = readl(priv->peri_ctrl + USB3_COMBPHY);
		reg |= COMBPHY_SRST_REQ;
		writel(reg, priv->peri_ctrl + USB3_COMBPHY);
		udelay(U_LEVEL5);
	}
	iounmap(priv->sys_ctrl);
}
EXPORT_SYMBOL(hisi_usb3_phy_off);
