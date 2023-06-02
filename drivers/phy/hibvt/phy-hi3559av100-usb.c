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
#include <linux/of_address.h>
#include <linux/phy/phy.h>
#include <linux/usb/ch9.h>

#include "phy-hisi-usb.h"

#define USB2_PHY           0x184
#define USB2_PHY_CKEN      (0x1 << 5)
#define USB2_PHY_PORT_TREQ (0x1 << 3)
#define USB2_PHY_REQ       (0x1 << 1)

#define USB3_COMBPHY      0x188
#define COMBPHY_REF_CKEN1 (0x1 << 24)
#define COMBPHY_SRST_REQ1 (0x1 << 16)
#define COMBPHY_REF_CKEN  (0x1 << 8)
#define COMBPHY_SRST_REQ  (0x1 << 0)

#define USB3_CTRL           0x190
#define USB3_1_PCLK_OCC_SEL (0x1 << 14)
#define USB3_VCC_SRST_REQ   (0x1 << 16)
#define USB3_UTMI_CKSEL     (0x1 << 29)
#define USB3_0_PCLK_OCC_SEL (0x1 << 30)

#define REG_GUCTL1            0xc11c
#define PARKMODE_DISABLE_FSLS (0x1 << 15)
#define PARKMODE_DISABLE_HS   (0x1 << 16)
#define PARKMODE_DISABLE_SS   (0x1 << 17)

#define GTXTHRCFG 0xc108
#define GRXTHRCFG 0xc10c
#define REG_GCTL  0xc110

#define PORT_CAP_DIR     (0x3 << 12)
#define DEFAULT_HOST_MOD (0x1 << 12)

#define USB_TXPKT_CNT_SEL    (0x1 << 29)
#define USB_TXPKT_CNT        (0x11 << 24)
#define USB_MAXTX_BURST_SIZE (0x1 << 20)
#define CLEAN_USB3_GTXTHRCFG 0x0

#define REG_GUSB3PIPECTL0   0xc2c0
#define PCS_SSP_SOFT_RESET  (0x1 << 31)
#define SUSPEND_USB3_SS_PHY (0x1 << 17)
#define USB3_TX_MARGIN_VAL  0x10c0012

#define USB3_COMB_PHY         0x14
#define P0_TX_SWING_COMP_CFG  0x913
#define P0_TX_SWING_COMP_RCFG 0x953
#define P0_TX_SWING_COMP_VAL  0x913
#define P1_TX_SWING_COMP_CFG  0x933
#define P1_TX_SWING_COMP_RCFG 0x973
#define P1_TX_SWING_COMP_VAL  0x933

#define OTP_CPU_REGBASE   0x10250000
#define HPM_CORE_OFFSET   0x28
#define KEEP_DEFAULT_FLAG 0x174

#define USB2_PHY0_CTRL     0x24
#define USB2_PHY1_CTRL     0x30
#define USB2_PHY_VREF_MASK (0xf << 4)
#define USB2_PHY_VREF      (0x5 << 4)
#define USB2_PHY_PRE       (0x3 << 12)

#define USB_PORT0          0x38
#define P0_U3_PORT_DISABLE (0x1 << 3)
#define USB_PORT1          0x3c
#define P1_U3_PORT_DISABLE (0x1 << 3)

#define PCIE_X2_MODE      (0x0 << 12)
#define PCIE_X1_MODE      (0x1 << 12)
#define USB3_MODE         (0x2 << 12)
#define COMBPHY_MODE_MASK (0x3 << 12)
#define SYSSTAT           0x8c

#define USB3_PORT1_CLK (0x1 << 14)

#define U2_CRG_OFFSET 1
#define U3_CRG_OFFSET 16

static combphy_mode mode_flag;

void hisi_switch_func(int otg)
{
}
EXPORT_SYMBOL(hisi_switch_func);

static void get_combphy_mode(struct phy *phy)
{
	int reg;
	struct hisi_priv *priv = phy_get_drvdata(phy);

	reg = readl(priv->combphy_base + SYSSTAT);
	reg &= COMBPHY_MODE_MASK;

	switch (reg) {
	case PCIE_X2_MODE:
		mode_flag = 0;
		break;
	case PCIE_X1_MODE:
		mode_flag = 1;
		break;
	case USB3_MODE:
		mode_flag = 2;
		break;
	default:
		break;
	}
}

static void usb_combphy_config(struct phy *phy, unsigned offset, unsigned rst)
{
	unsigned int reg;
	struct hisi_priv *priv = phy_get_drvdata(phy);

	if (rst) {
		/*
		 * According to description of SYSSYAT register, different
		 * HW modes of COMBPHY require specific configurations.
		 */
		reg = readl(priv->peri_ctrl + USB3_COMBPHY);
		switch (mode_flag) {
		case PCIE_X2:
			reg = readl(priv->peri_ctrl + USB3_CTRL);
			reg |= (USB3_0_PCLK_OCC_SEL | USB3_1_PCLK_OCC_SEL);
			writel(reg, priv->peri_ctrl + USB3_CTRL);
			break;
		case PCIE_X1:
			/* Only reset combphy1...port1 */
			reg |= COMBPHY_SRST_REQ1;
			writel(reg, priv->peri_ctrl + USB3_COMBPHY);
			udelay(U_LEVEL5);

			reg = readl(priv->peri_ctrl + USB3_CTRL);
			reg |= USB3_0_PCLK_OCC_SEL;
			writel(reg, priv->peri_ctrl + USB3_CTRL);
			break;
		case USB3:
			reg |= COMBPHY_SRST_REQ << offset;
			writel(reg, priv->peri_ctrl + USB3_COMBPHY);
			break;
		default:
			break;
		}
	} else {
		/* Release the specific configuration of COMBPHY. */
		reg = readl(priv->peri_ctrl + USB3_COMBPHY);
		if (mode_flag == PCIE_X1) {
			/* open ref CKEN */
			reg |= (COMBPHY_REF_CKEN1);
			/* only release combphy1...port1 */
			reg &= ~(COMBPHY_SRST_REQ1);
		} else if (mode_flag == USB3) {
			reg |= COMBPHY_REF_CKEN << offset;
			reg &= ~(COMBPHY_SRST_REQ << offset);
		}
		writel(reg, priv->peri_ctrl + USB3_COMBPHY);
	}
	udelay(U_LEVEL5);
}

static void hisi_usb3_crg_config(struct phy *phy, int u2_offset, int u3_offset)
{
	unsigned int reg;
	struct hisi_priv *priv = phy_get_drvdata(phy);

	reg = readl(priv->peri_ctrl + USB3_CTRL);
	reg |= (USB3_VCC_SRST_REQ >> u3_offset);
	writel(reg, priv->peri_ctrl + USB3_CTRL);
	udelay(U_LEVEL8);

	usb_combphy_config(phy, u3_offset, 1);

	/* enable port0 ss */
	reg = readl(priv->misc_ctrl + USB_PORT0);
	reg &= ~P0_U3_PORT_DISABLE;
	writel(reg, priv->misc_ctrl + USB_PORT0);

	/* enable port1 ss */
	reg = readl(priv->misc_ctrl + USB_PORT1);
	reg &= ~P1_U3_PORT_DISABLE;
	writel(reg, priv->misc_ctrl + USB_PORT1);

	/* release TPOR default release */
	reg = readl(priv->peri_ctrl + USB2_PHY);
	reg &= ~(USB2_PHY_PORT_TREQ >> u2_offset);
	writel(reg, priv->peri_ctrl + USB2_PHY);
	udelay(U_LEVEL6);

	/* utmi clock sel */
	reg = readl(priv->peri_ctrl + USB3_CTRL);
	reg &= ~(USB3_UTMI_CKSEL >> u3_offset);
	writel(reg, priv->peri_ctrl + USB3_CTRL);
	udelay(U_LEVEL6);

	/* open phy ref clk default open */
	reg = readl(priv->peri_ctrl + USB2_PHY);
	reg |= (USB2_PHY_CKEN << u2_offset);
	writel(reg, priv->peri_ctrl + USB2_PHY);
	udelay(U_LEVEL6);

	/* U2 phy reset release */
	reg = readl(priv->peri_ctrl + USB2_PHY);
	reg &= ~(USB2_PHY_REQ >> u2_offset);
	writel(reg, priv->peri_ctrl + USB2_PHY);
	udelay(U_LEVEL6);

	usb_combphy_config(phy, u3_offset, 0);

	/* config U3 Controller USB3_0 PHY OUTPUT */
	reg = readl(priv->peri_ctrl + USB3_CTRL);
	reg &= ~(USB3_VCC_SRST_REQ >> u3_offset);
	writel(reg, priv->peri_ctrl + USB3_CTRL);
	udelay(U_LEVEL8);
}

static void hisi_usb3_ctrl_config(struct phy *phy)
{
	int reg;
	struct hisi_priv *priv = phy_get_drvdata(phy);

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

	reg = readl(priv->ctrl_base + REG_GCTL);
	reg &= ~PORT_CAP_DIR;
	reg |= DEFAULT_HOST_MOD; /* [13:12] 01: Host; 10: Device; 11: OTG */
	writel(reg, priv->ctrl_base + REG_GCTL);
	udelay(U_LEVEL2);

	reg = readl(priv->ctrl_base + REG_GUSB3PIPECTL0);
	reg &= ~PCS_SSP_SOFT_RESET;
	reg &= ~SUSPEND_USB3_SS_PHY;  // disable suspend
	writel(reg, priv->ctrl_base + REG_GUSB3PIPECTL0);
	udelay(U_LEVEL2);

	reg &= CLEAN_USB3_GTXTHRCFG;
	reg |= USB_TXPKT_CNT_SEL;
	reg |= USB_TXPKT_CNT;
	reg |= USB_MAXTX_BURST_SIZE;
	writel(reg, priv->ctrl_base + GTXTHRCFG);
	udelay(U_LEVEL2);
	writel(reg, priv->ctrl_base + GRXTHRCFG);
	udelay(U_LEVEL2);
}

static void hisi_usb3_eye_config(struct phy *phy)
{
	int reg;
	struct hisi_priv *priv = phy_get_drvdata(phy);
	void __iomem *otp_reg = ioremap_nocache(OTP_CPU_REGBASE, 0x1000);

	if (!otp_reg)
		return;

	/* Port0 usb2 phy0 misc ctrl */
	reg = readl(priv->misc_ctrl + USB2_PHY0_CTRL);
	reg &= ~USB2_PHY_VREF_MASK;
	reg |= USB2_PHY_VREF; /* [7:4] -> (eye vref = 4%) */
	reg |= USB2_PHY_PRE;  /* [13:12] -> (pre electric = 3x) */
	writel(reg, priv->misc_ctrl + USB2_PHY0_CTRL);
	udelay(U_LEVEL4);

	/* Port1 usb2 phy1 misc ctrl */
	reg = readl(priv->misc_ctrl + USB2_PHY1_CTRL);
	reg &= ~USB2_PHY_VREF_MASK;
	reg |= USB2_PHY_VREF; /* [7:4] -> (eye vref = 4%) */
	reg |= USB2_PHY_PRE;  /* [13:12] -> (pre electric = 3x) */
	writel(reg, priv->misc_ctrl + USB2_PHY1_CTRL);
	udelay(U_LEVEL4);

	/*
	 * If HPM core less than or equal to FLAG, TX_SWING_COMP
	 * adjust 0x1000 --> 0x1001.
	 */
	reg = readl(otp_reg + HPM_CORE_OFFSET);
	if (reg <= KEEP_DEFAULT_FLAG) {
		writel(P0_TX_SWING_COMP_CFG, priv->misc_ctrl + USB3_COMB_PHY);
		writel(P0_TX_SWING_COMP_RCFG, priv->misc_ctrl + USB3_COMB_PHY);
		writel(P0_TX_SWING_COMP_VAL, priv->misc_ctrl + USB3_COMB_PHY);
		udelay(U_LEVEL4);

		writel(P1_TX_SWING_COMP_CFG, priv->misc_ctrl + USB3_COMB_PHY);
		writel(P1_TX_SWING_COMP_RCFG, priv->misc_ctrl + USB3_COMB_PHY);
		writel(P1_TX_SWING_COMP_VAL, priv->misc_ctrl + USB3_COMB_PHY);
		udelay(U_LEVEL4);
	}

	/* Port0/Port1 TX margin 1000mv => 900mv */
	writel(USB3_TX_MARGIN_VAL, priv->ctrl_base + REG_GUSB3PIPECTL0);

	iounmap(otp_reg);
}

void hisi_usb3_phy_on(struct phy *phy)
{
	unsigned int u2 = 0;
	unsigned int u3 = 0;
	struct hisi_priv *priv = phy_get_drvdata(phy);

	if (priv->phyid == 1) {
		u2 = U2_CRG_OFFSET;
		u3 = U3_CRG_OFFSET;
	}

	get_combphy_mode(phy);

	hisi_usb3_crg_config(phy, u2, u3);

	hisi_usb3_ctrl_config(phy);

	hisi_usb3_eye_config(phy);
}
EXPORT_SYMBOL(hisi_usb3_phy_on);

void hisi_usb3_phy_off(struct phy *phy)
{
	unsigned int reg;
	unsigned int usb3_offset = 0;
	struct hisi_priv *priv = phy_get_drvdata(phy);

	if (priv->phyid == 1)
		usb3_offset = U3_CRG_OFFSET;

	reg = readl(priv->peri_ctrl + USB3_CTRL);
	reg |= (USB3_VCC_SRST_REQ >> usb3_offset);
	writel(reg, priv->peri_ctrl + USB3_CTRL);
	udelay(U_LEVEL8);

	reg = readl(priv->peri_ctrl + USB3_COMBPHY);
	if (mode_flag == PCIE_X1)
		/* Only reset combphy1...port1 */
		reg |= COMBPHY_SRST_REQ1;
	else if (mode_flag == USB3)
		reg |= (COMBPHY_SRST_REQ << usb3_offset);

	writel(reg, priv->peri_ctrl + USB3_COMBPHY);
	udelay(U_LEVEL5);
}
EXPORT_SYMBOL(hisi_usb3_phy_off);

int hisi_usb3_init_para(struct phy *phy, struct device_node *np)
{
	struct hisi_priv *priv = phy_get_drvdata(phy);

	priv->combphy_base = of_iomap(np, 2);
	if (IS_ERR(priv->combphy_base))
		priv->combphy_base = NULL;

	priv->ctrl_base = of_iomap(np, 3);
	if (IS_ERR(priv->ctrl_base))
		priv->ctrl_base = NULL;

	if (of_property_read_u32(np, "phyid", &priv->phyid))
		return -EINVAL;

	return 0;
}
EXPORT_SYMBOL(hisi_usb3_init_para);
