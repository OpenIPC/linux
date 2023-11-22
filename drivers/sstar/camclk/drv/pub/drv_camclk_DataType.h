/*
 * drv_camclk_DataType.h- Sigmastar
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

#ifndef __DRV_CAMCLKDATATYPE_H__
#define __DRV_CAMCLKDATATYPE_H__
#include "cam_os_wrapper.h"

typedef enum
{
    CAMCLK_RET_OK = 0,
    CAMCLK_RET_RATEERR,
    CAMCLK_RET_NOTSUPPORT,
    CAMCLK_RET_FAIL,
} CAMCLK_RET_e;
typedef enum
{
    CAMCLK_DEV_Id_SCL = 0,
    CAMCLK_DEV_Id_DIP,
    CAMCLK_DEV_Id_GOP,
    CAMCLK_DEV_Id_MAX
} CAMCLK_DEV_Id_e;
typedef enum
{
    CAMCLK_SET_ATTR_PARENT = 0x1,
    CAMCLK_SET_ATTR_RATE   = 0x2,
    CAMCLK_SET_ATTR_MAX
} CAMCLK_Set_Attr_e;
typedef enum
{
    CAMCLK_ROUNDRATE_DOWN = 0,
    CAMCLK_ROUNDRATE_UP,
    CAMCLK_ROUNDRATE_ROUND,
    CAMCLK_ROUNDRATE_TYPE,
} CAMCLK_ROUNDRATE_TYPE_e;

typedef struct
{
    CAMCLK_ROUNDRATE_TYPE_e eRoundType;
    CAMCLK_Set_Attr_e       eSetType;
    union
    {
        u32 u32Rate;
        u32 u32Parent;
    } attribute;
} CAMCLK_Set_Attribute;
typedef struct
{
    u32 u32Rate;
    u32 u32CurrentParent;
    u32 u32Parent[16];
    u32 u32NodeCount;
} CAMCLK_Get_Attribute;
typedef struct
{
    struct CamOsListHead_t stList;
    u32                    u32ClkId;
    u8                     pDevName[16];
    u8                     bEnable;
    u32                    u32HandlerId;
    u32                    u32RoundRate;
    u32                    u32CurrentParent;
    union
    {
        u32 u32Rate;
        u32 u32Parent;
    } attribute;
} CAMCLK_Handler;
#endif
