/*
 * hal_dsp_dbg.h- Sigmastar
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

#ifndef _HAL_DSP_DBG_H_
#define _HAL_DSP_DBG_H_

#include "hal_dsp.h"

//-------------------------------------------------------------------------------------------------
//  Defines & Macro
//-------------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------
//  structure & Enum
//-------------------------------------------------------------------------------------------------

typedef enum
{
    HAL_DSP_DBG_PM0 = 0,
    HAL_DSP_DBG_PM1,
    HAL_DSP_DBG_PM2,
    HAL_DSP_DBG_PM3,
    HAL_DSP_DBG_PM4,
    HAL_DSP_DBG_PM5,
    HAL_DSP_DBG_PM6,
    HAL_DSP_DBG_PM7,
    HAL_DSP_DBG_PM_NUM
} HalDspDbgPMId_e;

typedef enum
{
    HAL_DSP_DBG_PM_SEL0 = 0,
    HAL_DSP_DBG_PM_SEL_NUM
} HalDspDbgPMSel_e;

//-------------------------------------------------------------------------------------------------
//  Prototype
//-------------------------------------------------------------------------------------------------

#ifdef _HAL_DSP_C_
#define INTERFACE Init
#else
#define INTERFACE extern
#endif

//-------------------------------------------------------------------------------------------------
//  PM
//-------------------------------------------------------------------------------------------------
INTERFACE HalDspRet_e HalDspDbgEnablePMG(HalDspCoreID_e eCoreID, bool bTrue);
INTERFACE HalDspRet_e HalDspDbgSetPMGCtrl(HalDspCoreID_e eCoreID, HalDspDbgPMId_e ePMId, bool bEnableINT,
                                          bool bEnableKRNL, __u8 u8TraceScope, HalDspDbgPMSel_e ePMSel,
                                          __u16 u16SelMask);
INTERFACE HalDspRet_e HalDspDbgGetPMGCnt(HalDspCoreID_e eCoreID, HalDspDbgPMId_e ePMId, __u32* u32Cnt);
INTERFACE HalDspRet_e HalDspDbgGetPMGStat(HalDspCoreID_e eCoreID, HalDspDbgPMId_e ePMId, __u32* u32Cnt);
INTERFACE HalDspRet_e HalDspDbgClearPMGStat(HalDspCoreID_e eCoreID, HalDspDbgPMId_e ePMId);

#undef INTERFACE
#endif /*_HAL_DSP_DBG_H_*/
