/* SPDX-License-Identifier: GPL-2.0 */

#ifndef __REALTEK_PHY_H
#define __REALTEK_PHY_H

#define PHY_REALTEK_ID1		(0X1c << 16)
#define PHY_REALTEK_ID1_MASK	(0Xffff<<16)

#define PHY_RTL8211F		0Xc916

#define RTL821x_PAGE_SELECT			0x1f

#define RTL8211F_PHYCR1				0x18

#define RTL8211F_MDI_MODE			BIT(8)
#define RTL8211F_MDI_MODE_EN		BIT(9)

#define RTL8211F_TX_DELAY			BIT(8)
#define RTL8211F_RX_DELAY			BIT(3)

#define RTL8211F_ALDPS_PLL_OFF			BIT(1)
#define RTL8211F_ALDPS_ENABLE			BIT(2)
#define RTL8211F_ALDPS_XTAL_OFF			BIT(12)

int realtek_phy_init(struct phy_device *phydev);

#endif /* __REALTEK_PHY_H */
