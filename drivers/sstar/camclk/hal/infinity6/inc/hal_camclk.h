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

#ifndef __HAL_CAMCLK_H__
#define __HAL_CAMCLK_H__
#include "drv_camclk_DataType.h"
#include "hal_camclk_st.h"
#include "hal_camclk_if_st.h"

const extern HalCamClkNode_t gCamClkSrcNode[HAL_CAMCLK_SRC_Id_MAX];
extern HalCamClkTopClk_t *gCamClkTopCurrent;
extern u16 gu16HandlerCnt;
extern struct CamOsListHead_t gstClkListHead;

#endif
