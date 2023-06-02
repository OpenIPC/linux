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
    HalCamClkSrcId_e u32Id;
    u32 u32Freq;
    u32 u32RetFreq;
    HalCamClkSrcId_e u32ParentId;
    CAMCLK_ROUNDRATE_TYPE_e enType;
} DrvCamClkGetRoundRateParent_t;
typedef struct
{
    HalCamClkSrcId_e u32Id;
    u32 u32Freq;
    u32 u32ParentFreq;
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
    u32 u32ParentCount;
} DrvCamClkGetAllParent_t;
typedef struct
{
    HalCamClkSrcId_e u32Id;
    HalCamClkSrcId_e u32ParentId;
    u32 u32ParentFreq;
} DrvCamClkSetParent_t;
typedef struct
{
    HalCamClkSrcId_e u32Id;
    u32 u32Freq;
} DrvCamClkGetRate_t;
typedef struct
{
    HalCamClkSrcId_e u32Id;
    u8 *pbEn;
} DrvCamClkGetOnOff_t;
typedef struct
{
    HalCamClkSrcId_e u32Id;
    u8 bEn;
} DrvCamClkSetOnOff_t;
typedef struct
{
    u8 bLess;
    u32 u32diff;
} DrvCamClkRateDiff_t;
#endif
