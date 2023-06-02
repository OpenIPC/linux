/* Copyright (c) 2018-2019 Sigmastar Technology Corp.
 All rights reserved.

 Unless otherwise stipulated in writing, any and all information contained
herein regardless in any format shall remain the sole proprietary of
Sigmastar Technology Corp. and be kept in strict confidence
(Sigmastar Confidential Information) by the recipient.
Any unauthorized act including without limitation unauthorized disclosure,
copying, use, reproduction, sale, distribution, modification, disassembling,
reverse engineering and compiling of the contents of Sigmastar Confidential
Information is unlawful and strictly prohibited. Sigmastar hereby reserves the
rights to any and all damages, losses, costs and expenses resulting therefrom.
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
    CAMCLK_SET_ATTR_RATE = 0x2,
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
    CAMCLK_Set_Attr_e eSetType;
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
    u32 u32ClkId;
    u8  pDevName[16];
    u8  bEnable;
    u32 u32HandlerId;
    u32 u32RoundRate;
    u32 u32CurrentParent;
    union
    {
        u32 u32Rate;
        u32 u32Parent;
    } attribute;
} CAMCLK_Handler;
#endif
