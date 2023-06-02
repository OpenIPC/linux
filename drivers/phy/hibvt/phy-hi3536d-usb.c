#include <linux/delay.h>
#include <linux/phy/phy.h>
#include <linux/of_address.h>
#include "phy-hisi-usb.h"

#define USB2_CTRL		0xb8
#define USB2_BUS_CKEN		(1 << 0)
#define USB2_OHCI48M_CKEN	(1 << 1)
#define USB2_OHCI12M_CKEN	(1 << 2)
#define USB2_HST_PHY_CKEN	(1 << 4)
#define USB2_UTMI0_CKEN		(1 << 5)
#define USB2_UTMI1_CKEN		(1 << 6)
#define USB2_BUS_SRST_REQ	(1 << 12)
#define USB2_UTMI0_SRST_REQ	(1 << 13)
#define USB2_UTMI1_SRST_REQ	(1 << 14)
#define USB2_HST_PHY_SYST_REQ	(1 << 16)

#define REG_USB2_PHY0		0xbc
#define USB_PHY0_REF_CKEN	(1 << 0)
#define USB_PHY0_SRST_REQ	(1 << 1)
#define USB_PHY0_SRST_TREQ	(1 << 2)
#define USB_PHY1_SRST_TREQ	(1 << 3)
#define USB_PHY0_TEST_SRST_REQ	(1 << 4)

#define MISC_CTRL_TRIM		0x50
#define USB_R_TUNING_1		0x800c
#define USB_R_TUNING_2		0x840c
#define MISC_CTRL20_23		(1 << 23)
#define MISC_CTRL20_24		(1 << 24)
#define MISC_CTRL20_25		(1 << 25)
#define MISC_CTRL20_26		(1 << 26)
#define MISC_CTRL20_27		(1 << 27)
#define TRIM_CONFIG_2		(1 << 2)
#define TRIM_CONFIG_3		(1 << 3)
#define TRIM_CONFIG_4		(1 << 4)
#define TRIM_CONFIG_5		(1 << 5)
#define TRIM_CONFIG_6		(1 << 6)

#define MISC_CTRL			(1 << 22)
#define REG_USB2_CFG		0x8018
#define USB2_CFG_VAL		0x05
#define USB2_PHY0_CTLL		0x54

/* param config */
#define EYE_HEIGHT_CFG		0x8008
#define EYE_HEIGHT_RECFG	0x8408
#define EYE_HEIGHT_VAL		0x5c

#define PRE_OPEN_CFG		0x8000
#define PRE_OPEN_RECFG		0x8400
#define PRE_OPEN_VAL		0x1c

#define PRE_IMP_CFG			0x8014
#define PRE_IMP_RECFG		0x8414
#define PRE_IMP_VAL			0x80

#define DISC_CFG			0x8010
#define DISC_RECFG			0x8410
#define DISC_CFG_VAL		0x17


void hisi_usb_phy_on(struct phy *phy)
{
	int reg, trim_reg;
	struct hisi_priv *priv = phy_get_drvdata(phy);

	/* misc ctrl */
	reg = readl(priv->misc_ctrl + USB2_PHY0_CTLL);
	reg |= MISC_CTRL;
	writel_relaxed(reg, priv->misc_ctrl + USB2_PHY0_CTLL);
	udelay(200);

	/* reset enable */
	reg = readl(priv->peri_ctrl + USB2_CTRL);
	reg |= (USB2_BUS_SRST_REQ
		| USB2_UTMI0_SRST_REQ
		| USB2_HST_PHY_SYST_REQ
		| USB2_UTMI1_SRST_REQ);
	writel(reg, priv->peri_ctrl + USB2_CTRL);
	udelay(200);

	reg = readl(priv->peri_ctrl + REG_USB2_PHY0);
	reg |= (USB_PHY0_SRST_REQ
		| USB_PHY0_SRST_TREQ
		| USB_PHY1_SRST_TREQ);
	writel(reg, priv->peri_ctrl + REG_USB2_PHY0);
	udelay(200);

	/* open ref clock */
	reg = readl(priv->peri_ctrl + REG_USB2_PHY0);
	reg |= (USB_PHY0_REF_CKEN);
	writel(reg, priv->peri_ctrl + REG_USB2_PHY0);
	udelay(100);

	/* cancel power on reset */
	reg = readl(priv->peri_ctrl + REG_USB2_PHY0);
	reg &= ~(USB_PHY0_SRST_REQ);
	reg &= ~(USB_PHY0_TEST_SRST_REQ);
	writel(reg, priv->peri_ctrl + REG_USB2_PHY0);
	udelay(300);

	writel(USB2_CFG_VAL, priv->misc_ctrl + REG_USB2_CFG);
	udelay(200);

	/* cancel port reset */
	reg = readl(priv->peri_ctrl + REG_USB2_PHY0);
	reg &= ~(USB_PHY0_SRST_TREQ);
	reg &= ~(USB_PHY1_SRST_TREQ);
	writel(reg, priv->peri_ctrl + REG_USB2_PHY0);
	udelay(300);

	/* cancel control reset */
	reg = readl(priv->peri_ctrl + USB2_CTRL);
	reg &= ~(USB2_BUS_SRST_REQ
		| USB2_UTMI0_SRST_REQ
		| USB2_HST_PHY_SYST_REQ
		| USB2_UTMI1_SRST_REQ);
	reg |= (USB2_BUS_CKEN
		| USB2_OHCI48M_CKEN
		| USB2_OHCI12M_CKEN
		| USB2_HST_PHY_CKEN
		| USB2_UTMI0_CKEN
		| USB2_UTMI1_CKEN);
	writel(reg, priv->peri_ctrl + USB2_CTRL);
	udelay(200);

	/* Trim config */
	reg = readl(priv->misc_ctrl + MISC_CTRL_TRIM);
	reg &= (MISC_CTRL20_23
			| MISC_CTRL20_24
			| MISC_CTRL20_25
			| MISC_CTRL20_26
			| MISC_CTRL20_27);
	reg = reg >> 21;
	if(reg){
		trim_reg = readl(priv->misc_ctrl + USB_R_TUNING_1);
		trim_reg &= ~(TRIM_CONFIG_2
				| TRIM_CONFIG_3
				| TRIM_CONFIG_4
				| TRIM_CONFIG_5
				| TRIM_CONFIG_6);
		trim_reg |= reg;
		writel(trim_reg, priv->misc_ctrl + USB_R_TUNING_1);

		trim_reg = readl(priv->misc_ctrl + USB_R_TUNING_2);
		trim_reg &= ~(TRIM_CONFIG_2
				| TRIM_CONFIG_3
				| TRIM_CONFIG_4
				| TRIM_CONFIG_5
				| TRIM_CONFIG_6);
		trim_reg |= reg;
		writel(trim_reg, priv->misc_ctrl + USB_R_TUNING_2);
	}

	/* eye height config */
	writel_relaxed(EYE_HEIGHT_VAL, priv->misc_ctrl + EYE_HEIGHT_CFG);
	udelay(100);
	writel_relaxed(EYE_HEIGHT_VAL, priv->misc_ctrl + EYE_HEIGHT_RECFG);
	udelay(100);

	/* pre open */
	writel_relaxed(PRE_OPEN_VAL, priv->misc_ctrl + PRE_OPEN_CFG);
	udelay(100);
	writel_relaxed(PRE_OPEN_VAL, priv->misc_ctrl + PRE_OPEN_RECFG);
	udelay(100);

	/* pre improve */
	writel_relaxed(PRE_IMP_VAL, priv->misc_ctrl + PRE_IMP_CFG);
	udelay(100);
	writel_relaxed(PRE_IMP_VAL, priv->misc_ctrl + PRE_IMP_RECFG);
	udelay(100);

	/* disconnects */
	writel_relaxed(DISC_CFG_VAL, priv->misc_ctrl + DISC_CFG);
	udelay(100);
	writel_relaxed(DISC_CFG_VAL, priv->misc_ctrl + DISC_RECFG);
	udelay(100);
}
EXPORT_SYMBOL(hisi_usb_phy_on);

void hisi_usb_phy_off(struct phy *phy)
{
	int reg;
	struct hisi_priv *priv = phy_get_drvdata(phy);

	reg = readl(priv->peri_ctrl + REG_USB2_PHY0);
	reg |= (USB_PHY0_SRST_REQ
		| USB_PHY0_SRST_TREQ
		| USB_PHY1_SRST_TREQ);
	writel(reg, priv->peri_ctrl + REG_USB2_PHY0);
	udelay(100);

	/* close clock */
	reg = readl(priv->peri_ctrl + REG_USB2_PHY0);
	reg &= ~USB_PHY0_REF_CKEN;
	writel(reg, priv->peri_ctrl + REG_USB2_PHY0);
	udelay(300);

	/* close clock */
	reg = readl(priv->peri_ctrl + USB2_CTRL);
	reg &= ~(USB2_BUS_CKEN
		| USB2_OHCI48M_CKEN
		| USB2_OHCI12M_CKEN
		| USB2_HST_PHY_CKEN
		| USB2_UTMI0_CKEN
		| USB2_UTMI1_CKEN);
	writel(reg, priv->peri_ctrl + USB2_CTRL);
	udelay(200);
}
EXPORT_SYMBOL(hisi_usb_phy_off);
