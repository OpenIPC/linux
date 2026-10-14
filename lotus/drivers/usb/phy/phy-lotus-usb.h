/* SPDX-License-Identifier: GPL-2.0 */
#ifndef USB_INCLUDE_PHY_H
#define USB_INCLUDE_PHY_H

#define U_LEVEL1			10
#define U_LEVEL2			20
#define U_LEVEL3			30
#define U_LEVEL4			50
#define U_LEVEL5			100
#define U_LEVEL6			200
#define U_LEVEL7			300
#define U_LEVEL8			500

#define M_LEVEL1			2
#define M_LEVEL2			5
#define M_LEVEL3			10
#define M_LEVEL4			20
#define M_LEVEL5			50
#define M_LEVEL6			100
#define M_LEVEL7			200

#define __1K__				0x400
#define __2K__				0x800
#define __4K__				0x1000
#define __8K__				0x2000
#define __64K__				0x10000

#define MISC_OTG_SUSPENDM_OFFSET	0xa08
#define MISC_OTG_SUSPENDM_HOST		(1 << 0)
#define MISC_OTG_SUSPENDM_DEVICE	(0 << 0)
#define MISC_OTG_SUSPENDM_MASK		(1 << 0)

struct lotus_usb_priv {
	void __iomem *crg_base;
	void __iomem *phy_base;
	void __iomem *misc_base;
	struct phy *phy;
	struct device *dev;
	/* phy eye diagram config */
	u32 eye_cfg_tx_vol1_offset;
	u32 eye_cfg_tx_vol1_val;
	int eye_cfg_tx_vol1_flag;
	u32 eye_cfg_tx_vol2_offset;
	u32 eye_cfg_tx_vol2_val;
	int eye_cfg_tx_vol2_flag;

	u32 phy_vref_cfg_offset;
	u32 phy_vref_cfg_val;
	int phy_vref_cfg_flag;

	unsigned int disconnect_detect_disable:1;
};

enum usb_phy_types {
	LOTUS_USB2_PHY = 0,			/* usb2 PHY */
	LOTUS_USB3_PHY = 1,			/* usb3 PHY */
	LOTUS_USB_PHY_UNKNOWN = 2,	/* unknown PHY type */
};

static const char *const usb_phy_names[] = {
	[LOTUS_USB2_PHY] = "lotus,usb2-phy",
	[LOTUS_USB3_PHY] = "lotus,usb3-phy",
};

void lotus_chip_u2_phy_para_get(struct device *dev,
	struct lotus_usb_priv *priv);
void lotus_chip_u2_phy_config(struct lotus_usb_priv *priv);
int lotus_u2_phy_clk_init(struct lotus_usb_priv *priv);
int lotus_u2_phy_clk_exit(struct lotus_usb_priv *priv);
int lotus_u2_phy_clk_resume(struct lotus_usb_priv *priv);
void lotus_u2_phy_eye_config(struct lotus_usb_priv *priv);

#endif /* USB_INCLUDE_PHY_H */

