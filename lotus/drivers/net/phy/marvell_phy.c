/*
 * Copyright (c) LOTUS. All rights reserved.
 */
//#define DEBUG
#include <linux/module.h>
#include <linux/netdevice.h>
#include <linux/phy.h>

#include "marvell_phy.h"

static int m88e1116r_config_init(struct phy_device *phydev)
{
	int temp;
	int err;

	temp = phy_read(phydev, MII_BMCR);
	temp |= BMCR_RESET;
	err = phy_write(phydev, MII_BMCR, temp);
	if (err < 0)
		return err;

	mdelay(500);

	err = phy_write(phydev, MII_MARVELL_PHY_PAGE, 0);
	if (err < 0)
		return err;

	temp = phy_read(phydev, MII_M1011_PHY_SCR);
	temp |= (7 << 12);	/* max number of gigabit attempts */
	temp |= (1 << 11);	/* enable downshift */
	temp |= MII_M1011_PHY_SCR_AUTO_CROSS;
	err = phy_write(phydev, MII_M1011_PHY_SCR, temp);
	if (err < 0)
		return err;

	err = phy_write(phydev, MII_MARVELL_PHY_PAGE, 2);
	if (err < 0)
		return err;

	temp = phy_read(phydev, MII_M1116R_CONTROL_REG_MAC);
	temp &= ~(1 << 5);/*care is bit*/
	temp |= (1 << 4);
	err = phy_write(phydev, MII_M1116R_CONTROL_REG_MAC, temp);
	if (err < 0)
		return err;

	err = phy_write(phydev, MII_MARVELL_PHY_PAGE, 0);
	if (err < 0)
		return err;

	temp = phy_read(phydev, MII_BMCR);
	temp |= BMCR_RESET;
	err = phy_write(phydev, MII_BMCR, temp);
	if (err < 0)
		return err;

	mdelay(500);

	return 0;
}

int marvell_phy_init(struct phy_device *phydev)
{
	return m88e1116r_config_init(phydev);
}

