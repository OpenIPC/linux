/*
 * Copyright (c) Hunan Goke,Chengdu Goke,Shandong Goke. 2021. All rights reserved.
 */

#ifndef __ETH_PHY_FIX_H__
#define __ETH_PHY_FIX_H__

#define GOKE_PHY_ID_V272     0x20669901
#define GOKE_PHY_ID_V115     0x20669903
#define GOKE_PHY_MASK        0xffffffff

#define MII_EXPMD 0x1d
#define MII_EXPMA 0x1e

void phy_register_fixups(void);
void phy_unregister_fixups(void);

#endif
