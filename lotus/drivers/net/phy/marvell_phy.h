/*
 * Copyright (c) LOTUS. All rights reserved.
 */

#ifndef __MARVELL_PHY_H
#define __MARVELL_PHY_H

#define PHY_MARVELL_ID1		(0X141 << 16)
#define PHY_MARVELL_ID1_MASK	(0Xffff<<16)

#define MII_MARVELL_PHY_PAGE		22

#define MII_M1011_PHY_SCR		0x10
#define MII_M1011_PHY_SCR_MDI		0x0000
#define MII_M1011_PHY_SCR_MDI_X		0x0020
#define MII_M1011_PHY_SCR_AUTO_CROSS	0x0060

#define MII_M1116R_CONTROL_REG_MAC	21

int marvell_phy_init(struct phy_device *phydev);

#endif /* __MARVELL_PHY_H */
