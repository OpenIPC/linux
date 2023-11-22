/*
 * hal_dsp.h- Sigmastar
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

#ifndef _HAL_DSP_H_
#define _HAL_DSP_H_

//-------------------------------------------------------------------------------------------------
//  Defines & Macro
//-------------------------------------------------------------------------------------------------

#define SSTAR_DSP
#define HAL_DSP_DEFAULT_PLL_CLK      900 // M
#define DSP_PLL_SCALAR1_DIV_THRES    4
#define DSP_PLL_SCALAR2_DIV_THRES    16
#define DSP_PLL_SCALAR3_DIV_THRES    4
#define DSP_PLL_LOOP_DIV_THRES       24
#define DSP_PLL_SYNTH_CLK_THRES_LOW  50
#define DSP_PLL_SYNTH_CLK_THRES_HIGH 133
#define MPLL_SOURCE_CLK_FREQ         432 // M

//-------------------------------------------------------------------------------------------------
//  structure & Enum
//-------------------------------------------------------------------------------------------------

typedef enum
{
    HAL_DSP_OK   = 0,
    HAL_DSP_FAIL = -1,
} HalDspRet_e;

typedef enum
{
    HAL_DSP_CORE0 = 0,
    HAL_DSP_CORE1,
    HAL_DSP_CORE2,
    HAL_DSP_CORE3,
    HAL_DSP_COREQUAD,
    HAL_DSP_CORE_NUM
} HalDspCoreID_e;

typedef enum
{
    HAL_DSP_INT_MASK_CORE = 0,
    HAL_DSP_INT_MASK_CMDQ0,
    HAL_DSP_INT_MASK_CMDQ1,
    HAL_DSP_INT_MASK_NUM
} HalDspIntMask_e;

typedef enum
{
    HAL_DSP_CORE_RESET_ALL,
    HAL_DSP_CORE_RESET_CORE,
    HAL_DSP_CORE_RESET_BRIDGE,
    HAL_DSP_CORE_RESET_DEBUG,
    HAL_DSP_GP_RESET_ALL,
    HAL_DSP_GP_RESET_CORE,
    HAL_DSP_GP_RESET_RIU,
    HAL_DSP_GP_RESET_CORE0,
    HAL_DSP_GP_RESET_CORE1,
    HAL_DSP_GP_RESET_CORE2,
    HAL_DSP_GP_RESET_CORE3,
    HAL_DSP_NODIE_RESET_ALL,
    HAL_DSP_NODIE_RESET_DIE,
    HAL_DSP_NODIE_RESET_MCU,
    HAL_DSP_RESET_DONE,
    HAL_DSP_RESET_NUM,
} HalDspResetType_e;

typedef enum
{
    HAL_DSP_ADDRESS_SET0 = 0,
    HAL_DSP_ADDRESS_SET1,
    HAL_DSP_ADDRESS_SET2,
    HAL_DSP_ADDRESS_SET_NUM
} HalDspAddressMapSetID_e;

typedef enum
{
    HAL_DSP_GPIO_GROUP_0_7 = 0,
    HAL_DSP_GPIO_GROUP_8_15,
    HAL_DSP_GPIO_GROUP_16_23,
    HAL_DSP_GPIO_GROUP_24_31,
    HAL_DSP_GPIO_GROUP_NUM
} HalDspGPIOGroupID_e;

typedef enum
{
    HAL_DSP_DBG_PDBGEN  = 1 << 0,
    HAL_DSP_DBG_DBGEN   = 1 << 1,
    HAL_DSP_DBG_NIDEN   = 1 << 2,
    HAL_DSP_DBG_SPIDEN  = 1 << 3,
    HAL_DSP_DBG_SPNIDEN = 1 << 4
} HalDspCoreSightDbgMode_e;

typedef struct
{
    HalDspAddressMapSetID_e eSet;
    __u32                   u32BaseAddress;
    __u32                   u32Len;
    __u64                   offset;
} HalDspAddressMap_t;

typedef struct
{
    bool bEdgeEn;
    bool bMskIrq;
    bool bForceValid;
    bool bClearIrq;
    bool bNegPos;
    bool bStatBeforeMsk;
} HalDspIRQStat_t;

typedef enum
{
    HAL_DSP_IRQ_STAT_EDGEEN     = 1 << 0,
    HAL_DSP_IRQ_STAT_IRQMSK     = 1 << 1,
    HAL_DSP_IRQ_STAT_FORCEVALID = 1 << 2,
    HAL_DSP_IRQ_STAT_CLEAR      = 1 << 3,
    HAL_DSP_IRQ_STAT_POL        = 1 << 4,
    HAL_DSP_IRQ_STAT_BEFOREMSK  = 1 << 5,
    HAL_DSP_IRQ_STAT_ALL        = BIT(0) | BIT(1) | BIT(2) | BIT(3) | BIT(4) | BIT(5),
} HalDspIRQStatItem_e;

//-------------------------------------------------------------------------------------------------
//  Prototype
//-------------------------------------------------------------------------------------------------

#ifdef _HAL_DSP_C_
#define INTERFACE Init
#else
#define INTERFACE extern
#endif

//-------------------------------------------------------------------------------------------------
//  core
//-------------------------------------------------------------------------------------------------
INTERFACE HalDspRet_e HalDspInit(HalDspCoreID_e eCoreID);
INTERFACE HalDspRet_e HalDspReset(HalDspCoreID_e eCoreID, HalDspResetType_e eRstType);
INTERFACE HalDspRet_e HalDspRunStallReset(HalDspCoreID_e eCoreID, bool bEnable);
INTERFACE HalDspRet_e HalDspSetResetVector(HalDspCoreID_e eCoreID, bool bEnable, __u64 u64Phy);
INTERFACE HalDspRet_e HalDspSetCoreSightDbgMode(HalDspCoreID_e eCoreID, HalDspCoreSightDbgMode_e eDbgMode);
INTERFACE HalDspRet_e HalDspSetPRID(HalDspCoreID_e eCoreID, __u16 u16PRID);
INTERFACE HalDspRet_e HalDspSetXIUClkEn(HalDspCoreID_e eCoreID, bool bEnable);
INTERFACE HalDspRet_e HalDspSetAPBSlvClkEn(HalDspCoreID_e eCoreID, bool bEnable);
INTERFACE HalDspRet_e HalDspSetAddressMap(HalDspCoreID_e eCoreID, HalDspAddressMap_t *pstDspAddressMap);
INTERFACE HalDspRet_e HalDspSetInterruptMask(HalDspCoreID_e eCoreID, HalDspIntMask_e eIntMask, __u32 u32Mask);
INTERFACE HalDspRet_e HalDspInterruptTrigger(HalDspCoreID_e eCoreID, __u8 u8IrqNum);
INTERFACE HalDspRet_e HalDspGetGPI(HalDspCoreID_e eCoreID, __u32 *u32Mask);
INTERFACE HalDspRet_e HalDspSetGPI(HalDspCoreID_e eCoreID, __u32 u32Mask);
INTERFACE HalDspRet_e HalDspGPOInterruptFetch(HalDspCoreID_e eCoreID, __u32 *u32Mask);
INTERFACE HalDspRet_e HalDspGetDspIrqNumById(HalDspCoreID_e eCoreID, __u32 *u32IrqNum);
INTERFACE HalDspRet_e HalDspAPBRead(HalDspCoreID_e eCoreID, __u16 u16ApbAddr, __u32 *u32Value, __u32 u32ValueMask);
INTERFACE HalDspRet_e HalDspAPBWrite(HalDspCoreID_e eCoreID, __u16 u16ApbAddr, __u32 u32Value, __u32 u32ValueMask);
INTERFACE HalDspRet_e HalDspGetPFaultInfo(HalDspCoreID_e eCoreID, __u64 *u64PfaultInfo);
INTERFACE HalDspRet_e HalDspGetPFaultStatus(HalDspCoreID_e eCoreID, bool *bPFaultInfoValid, bool *bPFatalError);
INTERFACE HalDspRet_e HalDspAPBSlvErrStatus(HalDspCoreID_e eCoreID, bool *bError);
INTERFACE HalDspRet_e HalDspSetIRQStat(HalDspCoreID_e eCoreID, HalDspIRQStat_t *pstIRQStat,
                                       HalDspIRQStatItem_e eStatItemMask, __u8 u8IrqNo);
INTERFACE HalDspRet_e HalDspGetIRQStat(HalDspCoreID_e eCoreID, HalDspIRQStat_t *pstIRQStat,
                                       HalDspIRQStatItem_e eStatItemMask, __u8 u8IrqNo);
//-------------------------------------------------------------------------------------------------
//  gp
//-------------------------------------------------------------------------------------------------
INTERFACE HalDspRet_e HalDspGPReset(HalDspResetType_e eRstType);
INTERFACE HalDspRet_e HalDspGPSetClkFreq(__u16 u16FreqM);
INTERFACE HalDspRet_e HalDspSetClkGating(HalDspCoreID_e eCoreID, bool bDisable);
INTERFACE HalDspRet_e HalDspSetClkInvert(HalDspCoreID_e eCoreID, bool bEnable);
/*
@param:u8BitMaskIdx
        5'd00:32'b00000000000000000000000000000001'
        ...
        5'd15:32'b01010101010101010101010101010101'
        ...
        5'd31:32'b11111111111111111111111111111111'
*/
INTERFACE HalDspRet_e HalDspSetDFS(HalDspCoreID_e eCoreID, bool bEnable, __u8 u8BitMaskIdx);
INTERFACE HalDspRet_e HalDspSetTOPClk(bool bGateEnable, bool bInvert, bool bFastClk);
INTERFACE HalDspRet_e HalDspSetGPIOGroup(HalDspCoreID_e eCoreID, HalDspGPIOGroupID_e eGPIOGroup);
INTERFACE HalDspRet_e HalDspSetJTAGSelection(HalDspCoreID_e eCoreID);
INTERFACE HalDspRet_e HalDspSetGPIOGroup(HalDspCoreID_e eCoreID, HalDspGPIOGroupID_e eGPIOGroup);
INTERFACE HalDspRet_e HalDspSetAllSramForceON(bool bEnable);
INTERFACE HalDspRet_e HalDspSetCoreSramPower(HalDspCoreID_e eCoreID, bool bOff);
INTERFACE HalDspRet_e HalDspSetCorePGSramPower(HalDspCoreID_e eCoreID, bool bOff);
INTERFACE HalDspRet_e HalDspSetCoreSramSleep(HalDspCoreID_e eCoreID, bool bEnable);
INTERFACE HalDspRet_e HalDspSetCoreSramDeepSleep(HalDspCoreID_e eCoreID, bool bEnable);
INTERFACE HalDspRet_e HalDspSetCorePower(HalDspCoreID_e eCoreID, bool bOff);
INTERFACE HalDspRet_e HalDspSetCoreIso(HalDspCoreID_e eCoreID, bool bEnable);
INTERFACE HalDspRet_e HalDspGetCorePower(HalDspCoreID_e eCoreID, bool *bOff);

//-------------------------------------------------------------------------------------------------
//  nodie
//-------------------------------------------------------------------------------------------------
INTERFACE HalDspRet_e HalDspNoDieReset(HalDspResetType_e eRstType);
INTERFACE HalDspRet_e HalDspNoDieIsoEn(bool bEnable);
INTERFACE HalDspRet_e HalDspNoDieBistFail(__u16 *u16Ret);

#undef INTERFACE
#endif /*_HAL_DSP_H_*/
