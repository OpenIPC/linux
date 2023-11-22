/*
 * mdrv_noe_lro.h- Sigmastar
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
#ifndef _MDRV_NOE_LRO_H_
#define _MDRV_NOE_LRO_H_

#if FE_HW_LRO
int  MDrv_NOE_LRO_Init(struct net_device *dev);
int  MDrv_NOE_LRO_Recv(struct net_device *dev, struct napi_struct *napi, int budget);
void MDrv_NOE_LRO_Deinit(struct net_device *dev);
#else
static inline int MDrv_NOE_LRO_Init(struct net_device *dev)
{
    return -ENOMEM;
}
static inline int MDrv_NOE_LRO_Recv(struct net_device *dev, struct napi_struct *napi, int budget)
{
    return (budget + 1);
}
static inline void MDrv_NOE_LRO_Deinit(struct net_device *dev) {}
#endif
#endif /* _MDRV_NOE_LRO_H_ */
