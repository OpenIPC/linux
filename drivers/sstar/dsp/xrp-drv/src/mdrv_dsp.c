/*
 * mdrv_dsp.c- Sigmastar
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
#include "mdrv_dsp.h"
#include "hal_dsp.h"
#include "reg.h"
#include "hal_dsp_dbg.h"
#include "cam_os_wrapper.h"
#include "cam_sysfs.h"

typedef struct MDRV_DSP_Context_s
{
    bool bNodieInited;
    bool bGpInited;
} MDRV_DSP_Context_t;

static MDRV_DSP_Context_t mhalDspCtx = {0};

#define CORE_VALID_CHECK_RETURN(eCoreID)                                                  \
    {                                                                                     \
        if (eCoreID < E_MDRV_DSP_CORE0 || eCoreID > E_MDRV_DSP_CORE3)                     \
        {                                                                                 \
            CamOsPrintf("[DSP:%d] %s %d unkown core\n", eCoreID, __FUNCTION__, __LINE__); \
            return E_MDRV_DSP_FAIL;                                                       \
        }                                                                                 \
    }

MDRV_DSP_Ret_e Mdrv_DSP_Init(MDRV_DSP_CoreID_e eCoreID)
{
    bool value = -1;
    CORE_VALID_CHECK_RETURN(eCoreID);
    if (!mhalDspCtx.bNodieInited)
    {
        // //nodie
        HalDspGPSetClkFreq(HAL_DSP_DEFAULT_PLL_CLK);
        HalDspNoDieReset(HAL_DSP_NODIE_RESET_ALL);
        CamOsUsSleep(1);
        HalDspNoDieReset(HAL_DSP_RESET_DONE);
        HalDspNoDieIsoEn(false);
        mhalDspCtx.bNodieInited = true;
    }
    if (!mhalDspCtx.bGpInited)
    {
        // gp
        HalDspGPReset(HAL_DSP_GP_RESET_ALL);
        CamOsUsSleep(1);
        HalDspGPReset(HAL_DSP_RESET_DONE);
        mhalDspCtx.bGpInited = true;
    }
    // CamOsMsSleep(10);
    HalDspGPReset(HAL_DSP_GP_RESET_CORE0 + eCoreID);
    HalDspSetClkGating((HalDspCoreID_e)eCoreID, true);
    HalDspSetClkInvert((HalDspCoreID_e)eCoreID, false);

    HalDspSetTOPClk(false, false, true);
    HalDspSetDFS((HalDspCoreID_e)eCoreID, false, 31);
    HalDspSetJTAGSelection((HalDspCoreID_e)eCoreID);
    HalDspSetAllSramForceON(true);
    HalDspSetCorePower((HalDspCoreID_e)eCoreID, false);
    HalDspSetCoreIso((HalDspCoreID_e)eCoreID, false);
    HalDspGetCorePower((HalDspCoreID_e)eCoreID, &value);
    HalDspGPReset(HAL_DSP_RESET_DONE);

    return E_MDRV_DSP_OK;
}
MDRV_DSP_Ret_e Mdrv_DSP_Reset(MDRV_DSP_CoreID_e eCoreID, MDRV_DSP_ResetCtx_t *pstRstCtx)
{
#define PHY_ADDR_MIU_BASE 0x20000000
    MDRV_DSP_Ret_e     eRet = E_MDRV_DSP_OK;
    MS_U8              i    = 0;
    HalDspIRQStat_t    stIRQStat;
    HalDspAddressMap_t stDspAddrMap;
    for (; i <= E_MDRV_DSP_ADDRESS_SET2; i++)
    {
        stDspAddrMap.eSet   = (HalDspAddressMapSetID_e)pstRstCtx->astAddrMap[i].eSet;
        stDspAddrMap.offset = CamOsMemPhysToMiu(pstRstCtx->astAddrMap[i].cpuPhy) - pstRstCtx->astAddrMap[i].u32DspPhy;
        stDspAddrMap.u32BaseAddress = pstRstCtx->astAddrMap[i].u32DspPhy;
        stDspAddrMap.u32Len         = pstRstCtx->astAddrMap[i].u32Len;
        HalDspSetAddressMap((HalDspCoreID_e)eCoreID, &stDspAddrMap);
    }
    HalDspSetPRID((HalDspCoreID_e)eCoreID, pstRstCtx->u16Prid);
    HalDspSetResetVector((HalDspCoreID_e)eCoreID, pstRstCtx->u32ResetVecAddr == 0 ? false : true,
                         pstRstCtx->u32ResetVecAddr);

    HalDspSetInterruptMask((HalDspCoreID_e)eCoreID, HAL_DSP_INT_MASK_CORE, 0x0);
    HalDspSetInterruptMask((HalDspCoreID_e)eCoreID, HAL_DSP_INT_MASK_CMDQ0, 0x0);
    HalDspSetInterruptMask((HalDspCoreID_e)eCoreID, HAL_DSP_INT_MASK_CMDQ1, 0x0);
    HalDspSetCoreSightDbgMode((HalDspCoreID_e)eCoreID, (HalDspCoreSightDbgMode_e)pstRstCtx->eDbgMode);
    HalDspSetXIUClkEn((HalDspCoreID_e)eCoreID, true);
    HalDspSetAPBSlvClkEn((HalDspCoreID_e)eCoreID, true);
    HalDspDbgEnablePMG((HalDspCoreID_e)eCoreID, true);
    HalDspDbgSetPMGCtrl((HalDspCoreID_e)eCoreID, HAL_DSP_DBG_PM0, true, true, 0, 0, 0xFF);
    HalDspDbgSetPMGCtrl((HalDspCoreID_e)eCoreID, HAL_DSP_DBG_PM1, true, true, 0, 1, 0xFF);
    HalDspDbgSetPMGCtrl((HalDspCoreID_e)eCoreID, HAL_DSP_DBG_PM2, true, true, 0, 2, 0xFF);
    HalDspDbgSetPMGCtrl((HalDspCoreID_e)eCoreID, HAL_DSP_DBG_PM3, true, true, 0, 3, 0xFF);
    HalDspDbgSetPMGCtrl((HalDspCoreID_e)eCoreID, HAL_DSP_DBG_PM4, true, true, 0, 4, 0xFF);

    memset(&stIRQStat, 0, sizeof(HalDspIRQStat_t));
    stIRQStat.bMskIrq = 1;
    for (i = 0; i < 32; i++)
        HalDspSetIRQStat((HalDspCoreID_e)eCoreID, &stIRQStat, HAL_DSP_IRQ_STAT_ALL, i);

    HalDspReset((HalDspCoreID_e)eCoreID, HAL_DSP_CORE_RESET_ALL);
    CamOsMsSleep(1);
    HalDspReset((HalDspCoreID_e)eCoreID, HAL_DSP_RESET_DONE);

    return eRet;
}
MDRV_DSP_Ret_e Mdrv_DSP_ResetCore(MDRV_DSP_CoreID_e eCoreID, MDRV_DSP_ResetType_e eType)
{
    MDRV_DSP_Ret_e eRet = E_MDRV_DSP_OK;
    eRet                = (MDRV_DSP_Ret_e)HalDspReset((HalDspCoreID_e)eCoreID, (HalDspResetType_e)eType);
    return eRet;
}
MDRV_DSP_Ret_e Mdrv_DSP_RunStallReset(MDRV_DSP_CoreID_e eCoreID, MS_BOOL bEnable)
{
    MDRV_DSP_Ret_e eRet = E_MDRV_DSP_OK;
    eRet                = (MDRV_DSP_Ret_e)HalDspRunStallReset((HalDspCoreID_e)eCoreID, bEnable);
    return eRet;
}
MDRV_DSP_Ret_e Mdrv_DSP_SetResetVector(MDRV_DSP_CoreID_e eCoreID, MS_BOOL bEnable, __u64 u64Phy)
{
    MDRV_DSP_Ret_e eRet = E_MDRV_DSP_OK;
    eRet                = (MDRV_DSP_Ret_e)HalDspSetResetVector((HalDspCoreID_e)eCoreID, bEnable, u64Phy);
    return eRet;
}
MDRV_DSP_Ret_e Mdrv_DSP_SetCoreSightDbgMode(MDRV_DSP_CoreID_e eCoreID, MDRV_DSP_CoreSightDbgMode_e eDbgMode)
{
    MDRV_DSP_Ret_e eRet = E_MDRV_DSP_OK;
    eRet = (MDRV_DSP_Ret_e)HalDspSetCoreSightDbgMode((HalDspCoreID_e)eCoreID, (HalDspCoreSightDbgMode_e)eDbgMode);
    return eRet;
}
MDRV_DSP_Ret_e Mdrv_DSP_SetPRID(MDRV_DSP_CoreID_e eCoreID, MS_U16 u16PRID)
{
    MDRV_DSP_Ret_e eRet = E_MDRV_DSP_OK;
    eRet                = (MDRV_DSP_Ret_e)HalDspSetPRID((HalDspCoreID_e)eCoreID, u16PRID);
    return eRet;
}
// MDRV_DSP_Ret_e Mdrv_DSP_SetAddressMap(MDRV_DSP_CoreID_e eCoreID, MDRV_DSP_AddressMap_t *pstDspAddressMap)
//{
//#define PHY_ADDR_MIU_BASE 0x20000000
//
//     MDRV_DSP_Ret_e eRet = E_MDRV_DSP_OK;
//     HalDspAddressMap_t pstDspHalAddressMap;
//     pstDspHalAddressMap.eSet = pstDspAddressMap->eSet;
//     pstDspHalAddressMap.offset = pstDspAddressMap->u32CpuPhy-PHY_ADDR_MIU_BASE-pstDspAddressMap->u32DspPhy;
//     pstDspHalAddressMap.u32BaseAddress = pstDspAddressMap->u32DspPhy;
//     pstDspHalAddressMap.u32Len = pstDspAddressMap->u32Len;
//     eRet =  (MDRV_DSP_Ret_e)HalDspSetAddressMap((HalDspCoreID_e)eCoreID,&pstDspHalAddressMap);
//     return eRet;
// }
MDRV_DSP_Ret_e Mdrv_DSP_SetInterruptMask(MDRV_DSP_CoreID_e eCoreID, MDRV_DSP_IntMask_e eIntMask, MS_U32 u32Mask)
{
    MDRV_DSP_Ret_e eRet = E_MDRV_DSP_OK;
    eRet = (MDRV_DSP_Ret_e)HalDspSetInterruptMask((HalDspCoreID_e)eCoreID, (HalDspIntMask_e)eIntMask, u32Mask);
    return eRet;
}
MDRV_DSP_Ret_e Mdrv_DSP_InterruptTrigger(MDRV_DSP_CoreID_e eCoreID, __u8 u8IrqNum)
{
    MDRV_DSP_Ret_e eRet = E_MDRV_DSP_OK;
    eRet                = (MDRV_DSP_Ret_e)HalDspInterruptTrigger((HalDspCoreID_e)eCoreID, u8IrqNum);
    return eRet;
}
MDRV_DSP_Ret_e Mdrv_DSP_GetGPI(MDRV_DSP_CoreID_e eCoreID, MS_U32 *u32Mask)
{
    MDRV_DSP_Ret_e eRet = E_MDRV_DSP_OK;
    eRet                = (MDRV_DSP_Ret_e)HalDspGetGPI((HalDspCoreID_e)eCoreID, u32Mask);
    return eRet;
}
MDRV_DSP_Ret_e Mdrv_DSP_SetGPI(MDRV_DSP_CoreID_e eCoreID, MS_U32 u32Mask)
{
    MDRV_DSP_Ret_e eRet = E_MDRV_DSP_OK;
    eRet                = (MDRV_DSP_Ret_e)HalDspSetGPI((HalDspCoreID_e)eCoreID, u32Mask);
    return eRet;
}
MDRV_DSP_Ret_e Mdrv_DSP_GetGPO(MDRV_DSP_CoreID_e eCoreID, MS_U32 *u32Mask)
{
    MDRV_DSP_Ret_e eRet = E_MDRV_DSP_OK;
    eRet                = (MDRV_DSP_Ret_e)HalDspGPOInterruptFetch((HalDspCoreID_e)eCoreID, u32Mask);
    return eRet;
}
MDRV_DSP_Ret_e Mdrv_DSP_GetDspIrqNumById(MDRV_DSP_CoreID_e eCoreID, MS_U32 *u32IrqNum)
{
    MDRV_DSP_Ret_e eRet = E_MDRV_DSP_OK;
    eRet                = (MDRV_DSP_Ret_e)HalDspGetDspIrqNumById((HalDspCoreID_e)eCoreID, u32IrqNum);
    return eRet;
}
MDRV_DSP_Ret_e Mdrv_DSP_APBRead(MDRV_DSP_CoreID_e eCoreID, MS_U16 u16ApbAddr, MS_U32 *u32Value, MS_U32 u32ValueMask)
{
    MDRV_DSP_Ret_e eRet = E_MDRV_DSP_OK;
    eRet                = (MDRV_DSP_Ret_e)HalDspAPBRead((HalDspCoreID_e)eCoreID, u16ApbAddr, u32Value, u32ValueMask);
    return eRet;
}
MDRV_DSP_Ret_e Mdrv_DSP_APBWrite(MDRV_DSP_CoreID_e eCoreID, MS_U16 u16ApbAddr, MS_U32 u32Value, MS_U32 u32ValueMask)
{
    MDRV_DSP_Ret_e eRet = E_MDRV_DSP_OK;
    return eRet;
}
MDRV_DSP_Ret_e Mdrv_DSP_GetPFaultInfo(MDRV_DSP_CoreID_e eCoreID, __u64 *u64PfaultInfo)
{
    MDRV_DSP_Ret_e eRet = E_MDRV_DSP_OK;
    eRet                = (MDRV_DSP_Ret_e)HalDspGetPFaultInfo((HalDspCoreID_e)eCoreID, u64PfaultInfo);
    return eRet;
}
MDRV_DSP_Ret_e Mdrv_DSP_GetPFaultStatus(MDRV_DSP_CoreID_e eCoreID, MS_BOOL *bPFaultInfoValid, MS_BOOL *bPFatalError)
{
    MDRV_DSP_Ret_e eRet = E_MDRV_DSP_OK;
    eRet =
        (MDRV_DSP_Ret_e)HalDspGetPFaultStatus((HalDspCoreID_e)eCoreID, (bool *)bPFaultInfoValid, (bool *)bPFatalError);
    return eRet;
}
MDRV_DSP_Ret_e Mdrv_DSP_APBSlvErrStatus(MDRV_DSP_CoreID_e eCoreID, MS_BOOL *bError)
{
    MDRV_DSP_Ret_e eRet = E_MDRV_DSP_OK;
    eRet                = (MDRV_DSP_Ret_e)HalDspAPBSlvErrStatus((HalDspCoreID_e)eCoreID, (bool *)bError);
    return eRet;
}
MDRV_DSP_Ret_e Mdrv_DSP_SetIRQStat(MDRV_DSP_CoreID_e eCoreID, MDRV_DSP_IRQStat_t *pstIRQStat,
                                   MDRV_DSP_IRQStatItem_e eStatItemMask, __u8 u8IrqNo)
{
    MDRV_DSP_Ret_e  eRet = E_MDRV_DSP_OK;
    HalDspIRQStat_t stIRQStat;
    stIRQStat.bClearIrq      = pstIRQStat->bClearIrq;
    stIRQStat.bEdgeEn        = pstIRQStat->bEdgeEn;
    stIRQStat.bForceValid    = pstIRQStat->bForceValid;
    stIRQStat.bMskIrq        = pstIRQStat->bMskIrq;
    stIRQStat.bNegPos        = pstIRQStat->bNegPol;
    stIRQStat.bStatBeforeMsk = pstIRQStat->bStatBeforeMsk;
    eRet = (MDRV_DSP_Ret_e)HalDspSetIRQStat((HalDspCoreID_e)eCoreID, &stIRQStat, (HalDspIRQStatItem_e)eStatItemMask,
                                            u8IrqNo);
    return eRet;
}
MDRV_DSP_Ret_e Mdrv_DSP_GetIRQStat(MDRV_DSP_CoreID_e eCoreID, MDRV_DSP_IRQStat_t *pstIRQStat,
                                   MDRV_DSP_IRQStatItem_e eStatItemMask, __u8 u8IrqNo)
{
    MDRV_DSP_Ret_e  eRet = E_MDRV_DSP_OK;
    HalDspIRQStat_t stIRQStat;
    eRet = (MDRV_DSP_Ret_e)HalDspGetIRQStat((HalDspCoreID_e)eCoreID, &stIRQStat, (HalDspIRQStatItem_e)eStatItemMask,
                                            u8IrqNo);
    pstIRQStat->bClearIrq      = stIRQStat.bClearIrq;
    pstIRQStat->bEdgeEn        = stIRQStat.bEdgeEn;
    pstIRQStat->bForceValid    = stIRQStat.bForceValid;
    pstIRQStat->bMskIrq        = stIRQStat.bMskIrq;
    pstIRQStat->bNegPol        = stIRQStat.bNegPos;
    pstIRQStat->bStatBeforeMsk = stIRQStat.bStatBeforeMsk;
    return eRet;
}
//-------------------------------------------------------------------------------------------------
//  gp
//-------------------------------------------------------------------------------------------------
MDRV_DSP_Ret_e Mdrv_DSP_GPReset(MDRV_DSP_ResetType_e eRstType)
{
    MDRV_DSP_Ret_e eRet = E_MDRV_DSP_OK;
    eRet                = (MDRV_DSP_Ret_e)HalDspGPReset((HalDspResetType_e)eRstType);
    return eRet;
}

MDRV_DSP_Ret_e Mdrv_DSP_SetDFS(MDRV_DSP_CoreID_e eCoreID, MS_BOOL bEnable, __u8 u8BitMaskIdx)
{
    MDRV_DSP_Ret_e eRet = E_MDRV_DSP_OK;
    eRet                = (MDRV_DSP_Ret_e)HalDspSetDFS((HalDspCoreID_e)eCoreID, bEnable, u8BitMaskIdx);
    return eRet;
}
MDRV_DSP_Ret_e Mdrv_DSP_SetTOPClk(MS_BOOL bGateEnable, MS_BOOL bInvert, MS_BOOL bFastClk)
{
    MDRV_DSP_Ret_e eRet = E_MDRV_DSP_OK;
    eRet                = (MDRV_DSP_Ret_e)HalDspSetTOPClk(bGateEnable, bInvert, bFastClk);
    return eRet;
}
MDRV_DSP_Ret_e Mdrv_DSP_SetJTAGSelection(MDRV_DSP_CoreID_e eCoreID)
{
    MDRV_DSP_Ret_e eRet = E_MDRV_DSP_OK;
    eRet                = (MDRV_DSP_Ret_e)HalDspSetJTAGSelection((HalDspCoreID_e)eCoreID);
    return eRet;
}
MDRV_DSP_Ret_e Mdrv_DSP_SetAllSramForceON(MS_BOOL bEnable)
{
    MDRV_DSP_Ret_e eRet = E_MDRV_DSP_OK;
    eRet                = (MDRV_DSP_Ret_e)HalDspSetAllSramForceON(bEnable);
    return eRet;
}
MDRV_DSP_Ret_e Mdrv_DSP_SetCoreSramPower(MDRV_DSP_CoreID_e eCoreID, MS_BOOL bOff)
{
    MDRV_DSP_Ret_e eRet = E_MDRV_DSP_OK;
    eRet                = (MDRV_DSP_Ret_e)HalDspSetCoreSramPower((HalDspCoreID_e)eCoreID, bOff);
    return eRet;
}
MDRV_DSP_Ret_e Mdrv_DSP_SetCorePGSramPower(MDRV_DSP_CoreID_e eCoreID, MS_BOOL bOff)
{
    MDRV_DSP_Ret_e eRet = E_MDRV_DSP_OK;
    eRet                = (MDRV_DSP_Ret_e)HalDspSetCorePGSramPower((HalDspCoreID_e)eCoreID, bOff);
    return eRet;
}
MDRV_DSP_Ret_e Mdrv_DSP_SetCoreSramSleep(MDRV_DSP_CoreID_e eCoreID, MS_BOOL bEnable)
{
    MDRV_DSP_Ret_e eRet = E_MDRV_DSP_OK;
    eRet                = (MDRV_DSP_Ret_e)HalDspSetCoreSramSleep((HalDspCoreID_e)eCoreID, bEnable);
    return eRet;
}
MDRV_DSP_Ret_e Mdrv_DSP_SetCoreSramDeepSleep(MDRV_DSP_CoreID_e eCoreID, MS_BOOL bEnable)
{
    MDRV_DSP_Ret_e eRet = E_MDRV_DSP_OK;
    eRet                = (MDRV_DSP_Ret_e)HalDspSetCoreSramDeepSleep((HalDspCoreID_e)eCoreID, bEnable);
    return eRet;
}
MDRV_DSP_Ret_e Mdrv_DSP_SetCorePower(MDRV_DSP_CoreID_e eCoreID, MS_BOOL bOff)
{
    MDRV_DSP_Ret_e eRet = E_MDRV_DSP_OK;
    eRet                = (MDRV_DSP_Ret_e)HalDspSetCorePower((HalDspCoreID_e)eCoreID, bOff);
    return eRet;
}
MDRV_DSP_Ret_e Mdrv_DSP_SetCoreIso(MDRV_DSP_CoreID_e eCoreID, MS_BOOL bEnable)
{
    MDRV_DSP_Ret_e eRet = E_MDRV_DSP_OK;
    eRet                = (MDRV_DSP_Ret_e)HalDspSetCoreIso((HalDspCoreID_e)eCoreID, bEnable);
    return eRet;
}
MDRV_DSP_Ret_e Mdrv_DSP_GetCorePower(MDRV_DSP_CoreID_e eCoreID, MS_BOOL *bOff)
{
    MDRV_DSP_Ret_e eRet = E_MDRV_DSP_OK;
    eRet                = (MDRV_DSP_Ret_e)HalDspGetCorePower((HalDspCoreID_e)eCoreID, (bool *)bOff);
    return eRet;
}
MDRV_DSP_Ret_e Mdrv_DSP_SetPLLClkFreq(MS_U16 u16FreqM)
{
    MDRV_DSP_Ret_e eRet = E_MDRV_DSP_OK;
    eRet                = (MDRV_DSP_Ret_e)HalDspGPSetClkFreq(u16FreqM);
    return eRet;
}

EXPORT_SYMBOL(Mdrv_DSP_Init);
EXPORT_SYMBOL(Mdrv_DSP_Reset);
EXPORT_SYMBOL(Mdrv_DSP_ResetCore);
EXPORT_SYMBOL(Mdrv_DSP_RunStallReset);
EXPORT_SYMBOL(Mdrv_DSP_SetResetVector);
EXPORT_SYMBOL(Mdrv_DSP_SetCoreSightDbgMode);
EXPORT_SYMBOL(Mdrv_DSP_SetPRID);
// EXPORT_SYMBOL(Mdrv_DSP_SetAddressMap);
EXPORT_SYMBOL(Mdrv_DSP_SetInterruptMask);
EXPORT_SYMBOL(Mdrv_DSP_InterruptTrigger);
EXPORT_SYMBOL(Mdrv_DSP_GetGPI);
EXPORT_SYMBOL(Mdrv_DSP_SetGPI);
EXPORT_SYMBOL(Mdrv_DSP_GetGPO);
EXPORT_SYMBOL(Mdrv_DSP_GetDspIrqNumById);
EXPORT_SYMBOL(Mdrv_DSP_APBRead);
EXPORT_SYMBOL(Mdrv_DSP_APBWrite);
EXPORT_SYMBOL(Mdrv_DSP_GetPFaultInfo);
EXPORT_SYMBOL(Mdrv_DSP_GetPFaultStatus);
EXPORT_SYMBOL(Mdrv_DSP_APBSlvErrStatus);
EXPORT_SYMBOL(Mdrv_DSP_SetIRQStat);
EXPORT_SYMBOL(Mdrv_DSP_GetIRQStat);
EXPORT_SYMBOL(Mdrv_DSP_GPReset);
EXPORT_SYMBOL(Mdrv_DSP_SetDFS);
EXPORT_SYMBOL(Mdrv_DSP_SetTOPClk);
EXPORT_SYMBOL(Mdrv_DSP_SetJTAGSelection);
EXPORT_SYMBOL(Mdrv_DSP_SetAllSramForceON);
EXPORT_SYMBOL(Mdrv_DSP_SetCoreSramPower);
EXPORT_SYMBOL(Mdrv_DSP_SetCorePGSramPower);
EXPORT_SYMBOL(Mdrv_DSP_SetCoreSramSleep);
EXPORT_SYMBOL(Mdrv_DSP_SetCoreSramDeepSleep);
EXPORT_SYMBOL(Mdrv_DSP_SetCorePower);
EXPORT_SYMBOL(Mdrv_DSP_SetCoreIso);
EXPORT_SYMBOL(Mdrv_DSP_GetCorePower);
EXPORT_SYMBOL(Mdrv_DSP_SetPLLClkFreq);
