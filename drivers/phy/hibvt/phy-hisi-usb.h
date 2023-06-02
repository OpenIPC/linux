/*
 * Copyright (c) 2017 HiSilicon Technologies Co., Ltd.
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

#ifndef USB2_INCLUDE_PHY_H
#define USB2_INCLUDE_PHY_H

extern void hisi_usb_phy_on(struct phy *phy);
extern void hisi_usb_phy_off(struct phy *phy);
extern void hisi_usb3_phy_on(struct phy *phy);
extern void hisi_usb3_phy_off(struct phy *phy);

#if defined(CONFIG_ARCH_HI3559AV100)
extern int hisi_usb3_init_para(struct phy *phy, struct device_node *np);
#endif

struct hisi_priv {
	void __iomem *sys_ctrl;
	void __iomem *peri_ctrl;
	void __iomem *combphy_base;
	void __iomem *misc_ctrl;
	unsigned int phyid;
	void __iomem *ctrl_base;
	void __iomem *switch_base;
};

typedef enum mode {
	PCIE_X2 = 0,
	PCIE_X1,
	USB3
} combphy_mode;

#define U_LEVEL1 10
#define U_LEVEL2 20
#define U_LEVEL3 30
#define U_LEVEL4 50
#define U_LEVEL5 100
#define U_LEVEL6 200
#define U_LEVEL7 300
#define U_LEVEL8 500

#define M_LEVEL1 2
#define M_LEVEL2 5
#define M_LEVEL3 10
#define M_LEVEL4 20
#define M_LEVEL5 50
#define M_LEVEL6 100
#define M_LEVEL7 200

#endif
