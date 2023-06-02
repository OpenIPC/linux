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

#ifndef __HAL_CAMCLK_IF_ST_H__
#define __HAL_CAMCLK_IF_ST_H__
#include "camclk_id.h"
#include "hal_camclk_st.h"
#include "drv_camclk_DataType.h"
#define MAX_CLK_SRC_PARENT_NODE_CNT 16
typedef enum
{
    HAL_CAMCLK_TYPE_FIXED = 0,
    HAL_CAMCLK_TYPE_FIXED_FACTOR,
    HAL_CAMCLK_TYPE_COMPLEX,
    HAL_CAMCLK_TYPE_COMPOSITE,
    HAL_CAMCLK_TYPE_MAX
} HalCamClkType_e;



//============================================
typedef struct
{
    CAMCLK_RET_e    (*PfnSetAdjInit)(void *pCfg);
    CAMCLK_RET_e    (*PfnSetAdjDeInit)(void *pCfg);
    CAMCLK_RET_e    (*PfnSetAdjRate)(void *pCfg);
    CAMCLK_RET_e    (*PfnGetAdjRate)(void *pCfg);
    CAMCLK_RET_e    (*PfnGetAdjRoundRate)(void *pCfg);
    CAMCLK_RET_e    (*PfnSetAdjOnOff)(void *pCfg);
    CAMCLK_RET_e    (*PfnGetAdjOnOff)(void *pCfg);
} HalCamClkAdjOps_t;
typedef struct
{
    u32 u32Freq;
} HalCamClkFixedClk_t;
typedef struct
{
    HalCamClkSrcId_e eParent;
    u8 u8Div;
    u8 u8Mult;
} HalCamClkFixedFactorClk_t;
typedef struct
{
    HalCamClkSrcId_e eParent;
    HalCamClkAdjOps_t *ptOps;
} HalCamClkComplexClk_t;
typedef struct
{
    u8  u8parent[MAX_CLK_SRC_PARENT_NODE_CNT];
    u32 u32Reg;
    u16 u16Gated;
    u16 u16Glitch;
    u8  u8SelectWidth;
    u8  u8SelectShift;
    u8  u8auto;
} HalCamClkCompositeClt_t;

typedef struct
{
    u8               u8RefCnt;      // be enable
} HalCamClkTopClk_t;
typedef struct
{
    u8 u8Id;
    u8 u8ClkType;
    union
    {
        HalCamClkCompositeClt_t stComposite;
        HalCamClkComplexClk_t stComplex;
        HalCamClkFixedFactorClk_t stFixedFac;
        HalCamClkFixedClk_t stFixed;
    } attribute;
} HalCamClkNode_t;

#endif
