/*
 * hal_dsp_dbg.c- Sigmastar
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

#include "ms_platform.h"
#include "ms_types.h"
#include "registers.h"
#include "cam_os_wrapper.h"
#include "hal_dsp_dbg.h"
#include "reg.h"

HalDspRet_e HalDspDbgEnablePMG(HalDspCoreID_e eCoreID, bool bTrue)
{
    if (bTrue)
        return HalDspAPBWrite(eCoreID, DSP_XIU_APB_PMG, DSP_XIU_APB_PMG_EN, DSP_XIU_APB_PMG_EN_MASK);
    else
        return HalDspAPBWrite(eCoreID, DSP_XIU_APB_PMG, ~DSP_XIU_APB_PMG_EN & DSP_XIU_APB_PMG_EN_MASK,
                              DSP_XIU_APB_PMG_EN_MASK);
}
HalDspRet_e HalDspDbgSetPMGCtrl(HalDspCoreID_e eCoreID, HalDspDbgPMId_e ePMId, bool bEnableINT, bool bEnableKRNL,
                                __u8 u8TraceScope, HalDspDbgPMSel_e ePMSel, __u16 u16SelMask)
{
    __u32 u32Value = (bEnableINT ? 1 : 0) << DSP_CORE_PMCTRL_INTEN_LSB;
    u32Value |= (bEnableKRNL ? 1 : 0) << DSP_CORE_PMCTRL_KRNL_LSB;
    u32Value |= (u8TraceScope << DSP_CORE_PMCTRL_TRACESCOPE_LSB) & DSP_CORE_PMCTRL_TRACESCOPE_MASK;
    u32Value |= (ePMSel << DSP_CORE_PMCTRL_SELECT_LSB) & DSP_CORE_PMCTRL_SELECT_MASK;
    u32Value |= (u16SelMask << DSP_CORE_PMCTRL_MSK_LSB) & DSP_CORE_PMCTRL_MSK_MASK;
    return HalDspAPBWrite(eCoreID, DSP_XIU_APB_PMCRTL0 + (ePMId - HAL_DSP_DBG_PM0) * 4, u32Value, 0XFFFFFFFF);
}
HalDspRet_e HalDspDbgGetPMGCnt(HalDspCoreID_e eCoreID, HalDspDbgPMId_e ePMId, __u32* u32Cnt)
{
    return HalDspAPBRead(eCoreID, DSP_XIU_APB_PM0 + (ePMId - HAL_DSP_DBG_PM0) * 4, u32Cnt, 0XFFFFFFFF);
}
HalDspRet_e HalDspDbgGetPMGStat(HalDspCoreID_e eCoreID, HalDspDbgPMId_e ePMId, __u32* u32Cnt)
{
    return HalDspAPBRead(eCoreID, DSP_XIU_APB_PMSTAT0 + (ePMId - HAL_DSP_DBG_PM0) * 4, u32Cnt, 0XFFFFFFFF);
}
HalDspRet_e HalDspDbgClearPMGStat(HalDspCoreID_e eCoreID, HalDspDbgPMId_e ePMId)
{
    return HAL_DSP_OK;
}
