/*
 * hal_camclk.h- Sigmastar
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

#ifndef __HAL_CAMCLK_H__
#define __HAL_CAMCLK_H__
#include "drv_camclk_DataType.h"
#include "hal_camclk_st.h"
#include "hal_camclk_if_st.h"

const extern HalCamClkNode_t  gCamClkSrcNode[HAL_CAMCLK_SRC_Id_MAX];
extern HalCamClkTopClk_t*     gCamClkTopCurrent;
extern u16                    gu16HandlerCnt;
extern struct CamOsListHead_t gstClkListHead;

#endif
