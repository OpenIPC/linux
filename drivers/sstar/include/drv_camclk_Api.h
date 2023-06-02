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

#ifndef __DRV_CAMCLKAPI_H__
#define __DRV_CAMCLKAPI_H__
#include "drv_camclk_DataType.h"


//------------------------------------------------------------------------------
/// @brief Clk Register.
/// @param[in] pCfg: Clk handler register.
/// @return DRV_CAMCLK_RET_e
//------------------------------------------------------------------------------
CAMCLK_RET_e CamClkRegister(u8 *pDevName,u32 u32ClkId,void **pvHandlerId);
CAMCLK_RET_e CamClkUnregister(void *pvHandlerId);
//------------------------------------------------------------------------------
/// @brief Get Clk Rate.
/// @param[in] u32ClkId: composite clk id.(from DrvCamClkGetClk)
/// @param[out] pCfg;: config of attribute.
/// @return DRV_CAMCLK_RET_e
//------------------------------------------------------------------------------
CAMCLK_RET_e CamClkAttrGet(void *pvHandlerId, CAMCLK_Get_Attribute *pCfg);
//------------------------------------------------------------------------------
/// @brief Get Clk OnOff.
/// @param[in] u32ClkId: composite clk id.(from DrvCamClkGetClk)
/// @param[out] pbEn: clk OnOff.
/// @return DRV_CAMCLK_RET_e
//------------------------------------------------------------------------------
CAMCLK_RET_e CamClkGetOnOff(void *pvHandlerId, u8 *pbEn);
//------------------------------------------------------------------------------
/// @brief Set Clk parent Attribute.
/// @param[in] pCfg;: config of attribute.
/// @return DRV_CAMCLK_RET_e
//------------------------------------------------------------------------------
CAMCLK_RET_e CamClkAttrSet(void *pvHandlerId,CAMCLK_Set_Attribute *pCfg);

//------------------------------------------------------------------------------
/// @brief Set Clk On/Off
/// @param[in] u32ClkId: composite clk id.(from DrvCamClkGetClk)
/// @param[in] bEn: On/Off.
/// @return DRV_CAMCLK_RET_e
//------------------------------------------------------------------------------
CAMCLK_RET_e CamClkSetOnOff(void *pvHandlerId, u8 bEn);
u32 CamClkRateGet(u32 u32ClkId);
#define CAMCLK_SETRATE_ROUNDUP(stCfg,freq)       \
do{                                                 \
        stCfg.eRoundType = CAMCLK_ROUNDRATE_UP;     \
        stCfg.eSetType = CAMCLK_SET_ATTR_RATE;      \
        stCfg.attribute.u32Rate = freq;             \
} while (0)

#define CAMCLK_SETRATE_ROUNDDOWN(stCfg,freq)       \
do{                                                 \
        stCfg.eRoundType = CAMCLK_ROUNDRATE_DOWN;     \
        stCfg.eSetType = CAMCLK_SET_ATTR_RATE;      \
        stCfg.attribute.u32Rate = freq;             \
} while (0)

#define CAMCLK_SETRATE_ROUND(stCfg,freq)       \
do{                                                 \
        stCfg.eRoundType = CAMCLK_ROUNDRATE_ROUND;     \
        stCfg.eSetType = CAMCLK_SET_ATTR_RATE;      \
        stCfg.attribute.u32Rate = freq;             \
} while (0)

#define CAMCLK_SETPARENT(stCfg,parent)       \
do{                                                 \
        stCfg.eSetType = CAMCLK_SET_ATTR_PARENT;      \
        stCfg.attribute.u32Parent = parent;             \
} while (0)

#endif
