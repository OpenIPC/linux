/*
 * mdrv_noe_ethtool.h- Sigmastar
 *
 * Copyright (c) [2019~2020] SigmaStar Technology.
 *
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License version 2 for more details.
 *
 */
#ifndef _MDRV_NOE_ETHTOOL_H_
#define _MDRV_NOE_ETHTOOL_H_

unsigned char MDrv_NOE_ETHTOOL_Get_Phy_Address(void);
u32           MDrv_NOE_ETHTOOL_Get_Link(struct net_device *dev);
int           MDrv_NOE_ETHTOOL_Get_Settings(struct net_device *dev, struct ethtool_cmd *cmd);
int           MDrv_NOE_ETHTOOL_Set_Settings(struct net_device *dev, struct ethtool_cmd *cmd);
void          MDrv_NOE_ETHTOOL_Init(struct net_device *dev);
u32           MDrv_NOE_ETHTOOL_Virt_Get_Link(struct net_device *dev);
int           MDrv_NOE_ETHTOOL_Virt_Set_Settings(struct net_device *dev, struct ethtool_cmd *cmd);
int           MDrv_NOE_ETHTOOL_Virt_Get_Settings(struct net_device *dev, struct ethtool_cmd *cmd);
void          MDrv_NOE_ETHTOOL_Virt_Init(struct net_device *dev);

#endif /* _MDRV_NOE_ETHTOOL_H_ */
