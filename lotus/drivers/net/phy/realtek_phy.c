// SPDX-License-Identifier: GPL-2.0

//#define DEBUG
#include <linux/module.h>
#include <linux/netdevice.h>
#include <linux/phy.h>

#include "realtek_phy.h"

static int rtl821x_write_page(struct phy_device *phydev, int page)
{
	return phy_write(phydev, RTL821x_PAGE_SELECT, page);
}

static int rtl8211f_config_init(struct phy_device *phydev)
{
	int temp;
	int err;

	/* change page to 0xa43 */
	err = rtl821x_write_page(phydev, 0xa43);
	if (err < 0)
		return err;

	/* config  page 0xa43 offset  0x18 */
	temp = phy_read(phydev, RTL8211F_PHYCR1);
	temp |= RTL8211F_ALDPS_ENABLE | RTL8211F_ALDPS_PLL_OFF | RTL8211F_ALDPS_XTAL_OFF;

	err = phy_write(phydev, RTL8211F_PHYCR1, temp);
	if (err < 0)
		return err;

	/* change page to 0xd08 */
	err = rtl821x_write_page(phydev, 0xd08);
		if (err < 0)
			return err;

	/* config  page 0xd08 offset  0x11 */
	temp = phy_read(phydev, 0x11);

	temp |= RTL8211F_TX_DELAY;

	err = phy_write(phydev, 0x11, temp);
	if (err < 0)
		return err;

	/* config  page 0xd08 offset  0x15 */
	temp = phy_read(phydev, 0x15);

	temp |= RTL8211F_RX_DELAY;

	err = phy_write(phydev, 0x15, temp);
		if (err < 0)
			return err;

	/* change page to 0x0 */
	err = rtl821x_write_page(phydev, 0);
	if (err < 0)
		return err;

	return 0;
}

int  realtek_phy_init(struct phy_device *phydev)
{
	int err = 0;
	u16 phy_id;

	phy_id = phy_read(phydev, MII_PHYSID2);

	switch (phy_id) {
	case PHY_RTL8211F:
		err = rtl8211f_config_init(phydev);
		break;
	default:
		break;
	}

	return err;
}

