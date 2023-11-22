/*
 * mdrv_dsp.h- Sigmastar
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

#ifndef _MDRV_DSP_H_
#define _MDRV_DSP_H_

///////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2018 Sigmastar Semiconductor, Inc. All rights reserved.
//
// Wrapper device interface in kernel space
///////////////////////////////////////////////////////////////////////////////
#include "mhal_common.h"

//-------------------------------------------------------------------------------------------------
//  Defines & Macro
//-------------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------
//  structure & Enum
//-------------------------------------------------------------------------------------------------

typedef enum
{
    E_MDRV_DSP_OK   = 0,
    E_MDRV_DSP_FAIL = -1,
} MDRV_DSP_Ret_e;

typedef enum
{
    E_MDRV_DSP_CORE0 = 0,
    E_MDRV_DSP_CORE1,
    E_MDRV_DSP_CORE2,
    E_MDRV_DSP_CORE3,
    E_MDRV_DSP_COREQ,
    E_MDRV_DSP_CORE_NUM
} MDRV_DSP_CoreID_e;

typedef enum
{
    E_MDRV_DSP_INT_MASK_DSP = 0,
    E_MDRV_DSP_INT_MASK_CMDQ0,
    E_MDRV_DSP_INT_MASK_CMDQ1,
    E_MDRV_DSP_INT_MASK_NUM
} MDRV_DSP_IntMask_e;

typedef enum
{
    E_MDRV_DSP_CORE_RESET_ALL,
    E_MDRV_DSP_CORE_RESET_CORE,
    E_MDRV_DSP_CORE_RESET_BRIDGE,
    E_MDRV_DSP_CORE_RESET_DEBUG,
    E_MDRV_DSP_GP_RESET_ALL,
    E_MDRV_DSP_GP_RESET_CORE,
    E_MDRV_DSP_GP_RESET_RIU,
    E_MDRV_DSP_GP_RESET_CORE0,
    E_MDRV_DSP_GP_RESET_CORE1,
    E_MDRV_DSP_GP_RESET_CORE2,
    E_MDRV_DSP_GP_RESET_CORE3,
    E_MDRV_DSP_NODIE_RESET_ALL,
    E_MDRV_DSP_NODIE_RESET_DIE,
    E_MDRV_DSP_NODIE_RESET_MCU,
    E_MDRV_DSP_RESET_DONE,
    E_MDRV_DSP_RESET_NUM,
} MDRV_DSP_ResetType_e;

typedef enum
{
    E_MDRV_DSP_ADDRESS_SET0 = 0,
    E_MDRV_DSP_ADDRESS_SET1,
    E_MDRV_DSP_ADDRESS_SET2,
    E_MDRV_DSP_ADDRESS_SET_NUM
} MDRV_DSP_AddressMapSetID_e;

typedef enum
{
    E_MDRV_DSP_DBG_PDBGEN  = 1 << 0,
    E_MDRV_DSP_DBG_DBGEN   = 1 << 1,
    E_MDRV_DSP_DBG_NIDEN   = 1 << 2,
    E_MDRV_DSP_DBG_SPIDEN  = 1 << 3,
    E_MDRV_DSP_DBG_SPNIDEN = 1 << 4
} MDRV_DSP_CoreSightDbgMode_e;

typedef struct
{
    MDRV_DSP_AddressMapSetID_e eSet;
    MS_U32                     u32DspPhy;
    MS_U32                     u32Len;
    MS_PHY                     cpuPhy;
} MDRV_DSP_AddressMap_t;

typedef struct
{
    MS_BOOL bEdgeEn;
    MS_BOOL bMskIrq;
    MS_BOOL bForceValid;
    MS_BOOL bClearIrq;
    MS_BOOL bNegPol;
    MS_BOOL bStatBeforeMsk;
} MDRV_DSP_IRQStat_t;

typedef enum
{
    E_MDRV_DSP_IRQ_STAT_EDGEEN     = 1 << 0,
    E_MDRV_DSP_IRQ_STAT_IRQMSK     = 1 << 1,
    E_MDRV_DSP_IRQ_STAT_FORCEVALID = 1 << 2,
    E_MDRV_DSP_IRQ_STAT_CLEAR      = 1 << 3,
    E_MDRV_DSP_IRQ_STAT_POL        = 1 << 4,
    E_MDRV_DSP_IRQ_STAT_BEFOREMSK  = 1 << 5,
    E_MDRV_DSP_IRQ_STAT_ALL = E_MDRV_DSP_IRQ_STAT_EDGEEN | E_MDRV_DSP_IRQ_STAT_IRQMSK | E_MDRV_DSP_IRQ_STAT_FORCEVALID
                              | E_MDRV_DSP_IRQ_STAT_CLEAR | E_MDRV_DSP_IRQ_STAT_POL | E_MDRV_DSP_IRQ_STAT_BEFOREMSK
} MDRV_DSP_IRQStatItem_e;

typedef struct
{
    MS_U16                      u16Prid;
    MS_U32                      u32ResetVecAddr;
    MDRV_DSP_CoreSightDbgMode_e eDbgMode;
    MDRV_DSP_AddressMap_t       astAddrMap[E_MDRV_DSP_ADDRESS_SET_NUM];
} MDRV_DSP_ResetCtx_t;

//-------------------------------------------------------------------------------------------------
//  Prototype
//-------------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------
//  core
//-------------------------------------------------------------------------------------------------
MDRV_DSP_Ret_e Mdrv_DSP_Init(MDRV_DSP_CoreID_e eCoreID);
MDRV_DSP_Ret_e Mdrv_DSP_ResetCore(MDRV_DSP_CoreID_e eCoreID, MDRV_DSP_ResetType_e eType);
MDRV_DSP_Ret_e Mdrv_DSP_Reset(MDRV_DSP_CoreID_e eCoreID, MDRV_DSP_ResetCtx_t *pstRstCtx);
MDRV_DSP_Ret_e Mdrv_DSP_RunStallReset(MDRV_DSP_CoreID_e eCoreID, MS_BOOL bEnable);
MDRV_DSP_Ret_e Mdrv_DSP_SetResetVector(MDRV_DSP_CoreID_e eCoreID, MS_BOOL bEnable, MS_U64 u64Phy);
MDRV_DSP_Ret_e Mdrv_DSP_SetCoreSightDbgMode(MDRV_DSP_CoreID_e eCoreID, MDRV_DSP_CoreSightDbgMode_e eDbgMode);
MDRV_DSP_Ret_e Mdrv_DSP_SetPRID(MDRV_DSP_CoreID_e eCoreID, MS_U16 u16PRID);
MDRV_DSP_Ret_e Mdrv_DSP_SetAddressMap(MDRV_DSP_CoreID_e eCoreID, MDRV_DSP_AddressMap_t *pstDspAddressMap);
MDRV_DSP_Ret_e Mdrv_DSP_SetInterruptMask(MDRV_DSP_CoreID_e eCoreID, MDRV_DSP_IntMask_e eIntMask, MS_U32 u32Mask);
MDRV_DSP_Ret_e Mdrv_DSP_InterruptTrigger(MDRV_DSP_CoreID_e eCoreID, MS_U8 u8IrqNum);
MDRV_DSP_Ret_e Mdrv_DSP_GetGPI(MDRV_DSP_CoreID_e eCoreID, MS_U32 *u32Mask);
MDRV_DSP_Ret_e Mdrv_DSP_SetGPI(MDRV_DSP_CoreID_e eCoreID, MS_U32 u32Mask);
MDRV_DSP_Ret_e Mdrv_DSP_GetGPO(MDRV_DSP_CoreID_e eCoreID, MS_U32 *u32Mask);
MDRV_DSP_Ret_e Mdrv_DSP_GetDspIrqNumById(MDRV_DSP_CoreID_e eCoreID, MS_U32 *u32IrqNum);
MDRV_DSP_Ret_e Mdrv_DSP_APBRead(MDRV_DSP_CoreID_e eCoreID, MS_U16 u16ApbAddr, MS_U32 *u32Value, MS_U32 u32ValueMask);
MDRV_DSP_Ret_e Mdrv_DSP_APBWrite(MDRV_DSP_CoreID_e eCoreID, MS_U16 u16ApbAddr, MS_U32 u32Value, MS_U32 u32ValueMask);
MDRV_DSP_Ret_e Mdrv_DSP_GetPFaultInfo(MDRV_DSP_CoreID_e eCoreID, MS_U64 *u64PfaultInfo);
MDRV_DSP_Ret_e Mdrv_DSP_GetPFaultStatus(MDRV_DSP_CoreID_e eCoreID, MS_BOOL *bPFaultInfoValid, MS_BOOL *bPFatalError);
MDRV_DSP_Ret_e Mdrv_DSP_APBSlvErrStatus(MDRV_DSP_CoreID_e eCoreID, MS_BOOL *bError);
MDRV_DSP_Ret_e Mdrv_DSP_SetIRQStat(MDRV_DSP_CoreID_e eCoreID, MDRV_DSP_IRQStat_t *pstIRQStat,
                                   MDRV_DSP_IRQStatItem_e eStatItemMask, MS_U8 u8IrqNo);
MDRV_DSP_Ret_e Mdrv_DSP_GetIRQStat(MDRV_DSP_CoreID_e eCoreID, MDRV_DSP_IRQStat_t *pstIRQStat,
                                   MDRV_DSP_IRQStatItem_e eStatItemMask, MS_U8 u8IrqNo);
//-------------------------------------------------------------------------------------------------
//  gp
//-------------------------------------------------------------------------------------------------
MDRV_DSP_Ret_e Mdrv_DSP_GPReset(MDRV_DSP_ResetType_e eRstType);
/*
@param:u8BitMaskIdx
        5'd00:32'b00000000000000000000000000000001'
        ...
        5'd15:32'b01010101010101010101010101010101'
        ...
        5'd31:32'b11111111111111111111111111111111'
*/
MDRV_DSP_Ret_e Mdrv_DSP_SetDFS(MDRV_DSP_CoreID_e eCoreID, MS_BOOL bEnable, MS_U8 u8BitMaskIdx);
MDRV_DSP_Ret_e Mdrv_DSP_SetTOPClk(MS_BOOL bGateEnable, MS_BOOL bInvert, MS_BOOL bFastClk);
MDRV_DSP_Ret_e Mdrv_DSP_SetJTAGSelection(MDRV_DSP_CoreID_e eCoreID);
MDRV_DSP_Ret_e Mdrv_DSP_SetAllSramForceON(MS_BOOL bEnable);
MDRV_DSP_Ret_e Mdrv_DSP_SetCoreSramPower(MDRV_DSP_CoreID_e eCoreID, MS_BOOL bOff);
MDRV_DSP_Ret_e Mdrv_DSP_SetCorePGSramPower(MDRV_DSP_CoreID_e eCoreID, MS_BOOL bOff);
MDRV_DSP_Ret_e Mdrv_DSP_SetCoreSramSleep(MDRV_DSP_CoreID_e eCoreID, MS_BOOL bEnable);
MDRV_DSP_Ret_e Mdrv_DSP_SetCoreSramDeepSleep(MDRV_DSP_CoreID_e eCoreID, MS_BOOL bEnable);
MDRV_DSP_Ret_e Mdrv_DSP_SetCorePower(MDRV_DSP_CoreID_e eCoreID, MS_BOOL bOff);
MDRV_DSP_Ret_e Mdrv_DSP_SetCoreIso(MDRV_DSP_CoreID_e eCoreID, MS_BOOL bEnable);
MDRV_DSP_Ret_e Mdrv_DSP_GetCorePower(MDRV_DSP_CoreID_e eCoreID, MS_BOOL *bOff);
MDRV_DSP_Ret_e Mdrv_DSP_SetPLLClkFreq(MS_U16 u16FreqM);

int MdrvDSPVirtioInit(MDRV_DSP_CoreID_e eCoreID, MS_U64 shmPhys, void *shmVirt, MS_U64 shmDSP);
int MdrvDSPVirtioDeinit(MDRV_DSP_CoreID_e eCoreID);

#endif /* _MDRV_DSP_H_ */
