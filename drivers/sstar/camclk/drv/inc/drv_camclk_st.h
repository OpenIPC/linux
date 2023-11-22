/*
 * drv_camclk_st.h- Sigmastar
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

#ifndef __DRV_CAMCLK_ST_H__
#define __DRV_CAMCLK_ST_H__
#include "camclk_id.h"
#include "hal_camclk_if_st.h"
#include "drv_camclk_DataType.h"
typedef enum
{
    DRV_CAMCLK_SHOW_CLK_COM = 0,
    DRV_CAMCLK_SHOW_CLK_ALL,
    DRV_CAMCLK_SHOW_CLK_MAX,
} DrvCamClkShowId_e;

typedef enum
{
    DRV_CAMCLK_SHAREMEM_TOPCURRENT = 0,
    DRV_CAMCLK_SHAREMEM_TYPE,
} DrvCamClkShareMemoryType_e;
typedef struct
{
    u32   u32LockNum;
    void *pMem;
} DrvCamClkGetDosInfo_t;

typedef struct
{
    HalCamClkSrcId_e        u32Id;
    u32                     u32Freq;
    u32                     u32RetFreq;
    HalCamClkSrcId_e        u32ParentId;
    CAMCLK_ROUNDRATE_TYPE_e enType;
} DrvCamClkGetRoundRateParent_t;
typedef struct
{
    HalCamClkSrcId_e u32Id;
    u32              u32Freq;
    u32              u32ParentFreq;
    HalCamClkSrcId_e u32ParentId;
} DrvCamClkSetAdjRoundRate_t;
typedef struct
{
    HalCamClkSrcId_e u32Id;
    HalCamClkSrcId_e u32ParentId;
} DrvCamClkGetParent_t;
typedef struct
{
    HalCamClkSrcId_e u32Id;
    HalCamClkSrcId_e u32ParentId[MAX_CLK_SRC_PARENT_NODE_CNT];
    u32              u32ParentCount;
} DrvCamClkGetAllParent_t;
typedef struct
{
    HalCamClkSrcId_e u32Id;
    HalCamClkSrcId_e u32ParentId;
    u32              u32ParentFreq;
} DrvCamClkSetParent_t;
typedef struct
{
    HalCamClkSrcId_e u32Id;
    u32              u32Freq;
} DrvCamClkGetRate_t;
typedef struct
{
    HalCamClkSrcId_e u32Id;
    u8 *             pbEn;
} DrvCamClkGetOnOff_t;
typedef struct
{
    HalCamClkSrcId_e u32Id;
    u8               bEn;
} DrvCamClkSetOnOff_t;
typedef struct
{
    u8  bLess;
    u32 u32diff;
} DrvCamClkRateDiff_t;
#endif
