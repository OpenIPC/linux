/*
 * mdrv_noe_mac.h- Sigmastar
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
#ifndef _MDRV_NOE_MAC_H_
#define _MDRV_NOE_MAC_H_
void MDrv_NOE_MAC_Init(struct net_device *dev);
void MDrv_NOE_MAC_Detect_Link_Status(struct net_device *dev);
#endif /* _MDRV_NOE_MAC_H_ */
