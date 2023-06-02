//<MStar Software>
//******************************************************************************
// MStar Software
// Copyright (c) 2010 - 2012 MStar Semiconductor, Inc. All rights reserved.
// All software, firmware and related documentation herein ("MStar Software") are
// intellectual property of MStar Semiconductor, Inc. ("MStar") and protected by
// law, including, but not limited to, copyright law and international treaties.
// Any use, modification, reproduction, retransmission, or republication of all
// or part of MStar Software is expressly prohibited, unless prior written
// permission has been granted by MStar.
//
// By accessing, browsing and/or using MStar Software, you acknowledge that you
// have read, understood, and agree, to be bound by below terms ("Terms") and to
// comply with all applicable laws and regulations:
//
// 1. MStar shall retain any and all right, ownership and interest to MStar
//    Software and any modification/derivatives thereof.
//    No right, ownership, or interest to MStar Software and any
//    modification/derivatives thereof is transferred to you under Terms.
//
// 2. You understand that MStar Software might include, incorporate or be
//    supplied together with third party`s software and the use of MStar
//    Software may require additional licenses from third parties.
//    Therefore, you hereby agree it is your sole responsibility to separately
//    obtain any and all third party right and license necessary for your use of
//    such third party`s software.
//
// 3. MStar Software and any modification/derivatives thereof shall be deemed as
//    MStar`s confidential information and you agree to keep MStar`s
//    confidential information in strictest confidence and not disclose to any
//    third party.
//
// 4. MStar Software is provided on an "AS IS" basis without warranties of any
//    kind. Any warranties are hereby expressly disclaimed by MStar, including
//    without limitation, any warranties of merchantability, non-infringement of
//    intellectual property rights, fitness for a particular purpose, error free
//    and in conformity with any international standard.  You agree to waive any
//    claim against MStar for any loss, damage, cost or expense that you may
//    incur related to your use of MStar Software.
//    In no event shall MStar be liable for any direct, indirect, incidental or
//    consequential damages, including without limitation, lost of profit or
//    revenues, lost or damage of data, and unauthorized system use.
//    You agree that this Section 4 shall still apply without being affected
//    even if MStar Software has been modified by MStar in accordance with your
//    request or instruction for your use, except otherwise agreed by both
//    parties in writing.
//
// 5. If requested, MStar may from time to time provide technical supports or
//    services in relation with MStar Software to you for your use of
//    MStar Software in conjunction with your or your customer`s product
//    ("Services").
//    You understand and agree that, except otherwise agreed by both parties in
//    writing, Services are provided on an "AS IS" basis and the warranty
//    disclaimer set forth in Section 4 above shall apply.
//
// 6. Nothing contained herein shall be construed as by implication, estoppels
//    or otherwise:
//    (a) conferring any license or right to use MStar name, trademark, service
//        mark, symbol or any other identification;
//    (b) obligating MStar or any of its affiliates to furnish any person,
//        including without limitation, you and your customers, any assistance
//        of any kind whatsoever, or any information; or
//    (c) conferring any license or right under any intellectual property right.
//
// 7. These terms shall be governed by and construed in accordance with the laws
//    of Taiwan, R.O.C., excluding its conflict of law rules.
//    Any and all dispute arising out hereof or related hereto shall be finally
//    settled by arbitration referred to the Chinese Arbitration Association,
//    Taipei in accordance with the ROC Arbitration Law and the Arbitration
//    Rules of the Association by three (3) arbitrators appointed in accordance
//    with the said Rules.
//    The place of arbitration shall be in Taipei, Taiwan and the language shall
//    be English.
//    The arbitration award shall be final and binding to both parties.
//
//******************************************************************************
//<MStar Software>
////////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2008-2009 MStar Semiconductor, Inc.
// All rights reserved.
//
// Unless otherwise stipulated in writing, any and all information contained
// herein regardless in any format shall remain the sole proprietary of
// MStar Semiconductor Inc. and be kept in strict confidence
// (!¡±MStar Confidential Information!¡L) by the recipient.
// Any unauthorized act including without limitation unauthorized disclosure,
// copying, use, reproduction, sale, distribution, modification, disassembling,
// reverse engineering and compiling of the contents of MStar Confidential
// Information is unlawful and strictly prohibited. MStar hereby reserves the
// rights to any and all damages, losses, costs and expenses resulting therefrom.
//
////////////////////////////////////////////////////////////////////////////////
//==============================================================================
#ifndef _HAL_HVSP_H
#define _HAL_HVSP_H


//-------------------------------------------------------------------------------------------------
//  Defines & ENUM
//-------------------------------------------------------------------------------------------------
typedef enum
{
    E_HVSP_FILTER_MODE_BYPASS,
    E_HVSP_FILTER_MODE_BILINEAR,
    E_HVSP_FILTER_MODE_SRAM_0,
    E_HVSP_FILTER_MODE_SRAM_1,
}EN_HVSP_FILTER_MODE;

typedef enum
{
    E_HVSP_SRAM_SEL_0,
    E_HVSP_SRAM_SEL_1,
}EN_HVSP_SRAM_SEL_TYPE;

typedef enum
{
    E_HVSP_FILTER_SRAM_SEL_1,
    E_HVSP_FILTER_SRAM_SEL_2,
    E_HVSP_FILTER_SRAM_SEL_3,
    E_HVSP_FILTER_SRAM_SEL_4,
}EN_HVSP_FILTER_SRAM_SEL_TYPE;

typedef enum
{
    EN_HVSP_CLKATTR_ISP=0x10,
    EN_HVSP_CLKATTR_BT656=0x20,
    EN_HVSP_CLKATTR_FORCEMODE=0x40,
}EN_HVSP_CLKATTR_TYPE;

//-------------------------------------------------------------------------------------------------
//  Structure
//-------------------------------------------------------------------------------------------------

typedef struct
{
    MS_BOOL bEn;
    MS_U16  u16In_hsize;
    MS_U16  u16In_vsize;
    MS_U16  u16Hst;
    MS_U16  u16Hsize;
    MS_U16  u16Vst;
    MS_U16  u16Vsize;
}ST_HVSP_CROP_INFO;
typedef enum
{
    EN_VIP_SRAM_DUMP_ICC,
    EN_VIP_SRAM_DUMP_IHC,
    EN_VIP_SRAM_DUMP_HVSP,
    EN_VIP_SRAM_DUMP_HVSP_1,
    EN_VIP_SRAM_DUMP_HVSP_2,
}EN_VIP_SRAM_DUMP_TYPE;

//-------------------------------------------------------------------------------------------------
//  Prototype
//-------------------------------------------------------------------------------------------------

#ifndef MHAL_HVSP_C
#define INTERFACE extern
#else
#define INTERFACE
#endif

INTERFACE void Hal_HVSP_SetRiuBase(MS_U32 u32RiuBase);
INTERFACE void Hal_VIP_SetLDCSW_Idx(MS_U8 idx);
INTERFACE void Hal_HVSP_SetCropConfig(EN_HVSP_CROP_ID_TYPE enID, ST_HVSP_CROP_INFO stCropInfo);
INTERFACE void Hal_HVSP_SetHWInputMux(EN_HVSP_IP_MUX_TYPE enIpType);
INTERFACE void Hal_HVSP_SetInputMuxType(EN_HVSP_IP_MUX_TYPE enIpType);
INTERFACE MS_U32 Hal_HVSP_GetInputSrcMux(void);
INTERFACE void Hal_HVSP_Set_Reset(void);

// NLM
INTERFACE void Hal_HVSP_SetNLMSize(MS_U16 u16Width, MS_U16 u16Height);
INTERFACE void Hal_HVSP_SetNLMEn(MS_BOOL bEn);
INTERFACE void Hal_HVSP_SetNLMLineBufferSize(MS_U16 u16Width, MS_U16 u16Height);
INTERFACE void Hal_HVSP_SetVpsSRAMEn(MS_BOOL bEn);

// IPM
INTERFACE void Hal_HVSP_SetIPMReadEn(MS_BOOL bEn);
INTERFACE void Hal_HVSP_SetIPMWriteEn(MS_BOOL bEn);
INTERFACE void Hal_HVSP_SetIPMBase(MS_U32 u32Base);
INTERFACE void Hal_HVSP_SetIPMvSize(MS_U16 u16Vsize);
INTERFACE void Hal_HVSP_SetIPMLineOffset(MS_U16 u16Lineoffset);
INTERFACE void Hal_HVSP_SetIPMFetchNum(MS_U16 u16FetchNum);
INTERFACE void Hal_HVSP_Set_IPM_Read_Req(MS_U8 u8Thrd, MS_U8 u8Limit);
INTERFACE void Hal_HVSP_Set_IPM_Write_Req(MS_U8 u8Thrd, MS_U8 u8Limit);
INTERFACE void Hal_HVSP_SetIPMBufferNumber(MS_U8 u8Num);


// Scaling
INTERFACE void Hal_HVSP_SetScalingVeEn(EN_HVSP_ID_TYPE enID, MS_BOOL ben);
INTERFACE void Hal_HVSP_SetScalingVeFactor(EN_HVSP_ID_TYPE enID, MS_U32 u32Ratio);
INTERFACE void Hal_HVSP_SetScalingHoEn(EN_HVSP_ID_TYPE enID, MS_BOOL ben);
INTERFACE void Hal_HVSP_SetScalingHoFacotr(EN_HVSP_ID_TYPE enID, MS_U32 u32Ratio);
INTERFACE void Hal_HVSP_SetModeYHo(EN_HVSP_ID_TYPE enID, EN_HVSP_FILTER_MODE enFilterMode);
INTERFACE void Hal_HVSP_SetModeYVe(EN_HVSP_ID_TYPE enID, EN_HVSP_FILTER_MODE enFilterMode);
INTERFACE void Hal_HVSP_SetModeCHo(EN_HVSP_ID_TYPE enID, EN_HVSP_FILTER_MODE enFilterMode, EN_HVSP_SRAM_SEL_TYPE enSramSel);
INTERFACE void Hal_HVSP_SetModeCVe(EN_HVSP_ID_TYPE enID, EN_HVSP_FILTER_MODE enFilterMode, EN_HVSP_SRAM_SEL_TYPE enSramSel);
INTERFACE void Hal_HVSP_SetHspDithEn(EN_HVSP_ID_TYPE enID, MS_BOOL bEn);
INTERFACE void Hal_HVSP_SetVspDithEn(EN_HVSP_ID_TYPE enID, MS_BOOL bEn);
INTERFACE void Hal_HVSP_SetHspCoringEnC(EN_HVSP_ID_TYPE enID, MS_BOOL bEn);
INTERFACE void Hal_HVSP_SetHspCoringEnY(EN_HVSP_ID_TYPE enID, MS_BOOL bEn);
INTERFACE void Hal_HVSP_SetVspCoringEnC(EN_HVSP_ID_TYPE enID, MS_BOOL bEn);
INTERFACE void Hal_HVSP_SetVspCoringEnY(EN_HVSP_ID_TYPE enID, MS_BOOL bEn);
INTERFACE void Hal_HVSP_SetHspCoringThrdC(EN_HVSP_ID_TYPE enID, MS_U16 u16Thread);
INTERFACE void Hal_HVSP_SetHspCoringThrdY(EN_HVSP_ID_TYPE enID, MS_U16 u16Thread);
INTERFACE void Hal_HVSP_SetVspCoringThrdC(EN_HVSP_ID_TYPE enID, MS_U16 u16Thread);
INTERFACE void Hal_HVSP_SetVspCoringThrdY(EN_HVSP_ID_TYPE enID, MS_U16 u16Thread);
INTERFACE void Hal_HVSP_Set_Sram_Coeff(EN_HVSP_ID_TYPE enID, EN_HVSP_FILTER_SRAM_SEL_TYPE enSramSel, MS_BOOL bC_SRAM, MS_U8 *pData);
INTERFACE void Hal_HVSP_SetHVSPInputSize(EN_HVSP_ID_TYPE enID, MS_U16 u16Width, MS_U16 u16Height);
INTERFACE void Hal_HVSP_SetHVSPOutputSize(EN_HVSP_ID_TYPE enID, MS_U16 u16Width, MS_U16 u16Height);

// PatTg
INTERFACE void Hal_HVSP_SetPatTgEn(MS_BOOL bEn);
INTERFACE void Hal_HVSP_SetPatTgVsyncSt(MS_U16 u16Val);
INTERFACE void Hal_HVSP_SetPatTgVsyncEnd(MS_U16 u16Val);
INTERFACE void Hal_HVSP_SetPatTgVfdeSt(MS_U16 u16Val);
INTERFACE void Hal_HVSP_SetPatTgVfdeEnd(MS_U16 u16Val);
INTERFACE void Hal_HVSP_SetPatTgVdeSt(MS_U16 u16Val);
INTERFACE void Hal_HVSP_SetPatTgVdeEnd(MS_U16 u16Val);
INTERFACE void Hal_HVSP_SetPatTgVtt(MS_U16 u16Val);
INTERFACE void Hal_HVSP_SetPatTgHsyncSt(MS_U16 u16Val);
INTERFACE void Hal_HVSP_SetPatTgHsyncEnd(MS_U16 u16Val);
INTERFACE void Hal_HVSP_SetPatTgHfdeSt(MS_U16 u16Val);
INTERFACE void Hal_HVSP_SetPatTgHfdeEnd(MS_U16 u16Val);
INTERFACE void Hal_HVSP_SetPatTgHdeSt(MS_U16 u16Val);
INTERFACE void Hal_HVSP_SetPatTgHdeEnd(MS_U16 u16Val);
INTERFACE void Hal_HVSP_SetPatTgHtt(MS_U16 u16Val);
INTERFACE void Hal_HVSP_SetTestPatCfg(void);
// REG W
INTERFACE void Hal_HVSP_Set_Reg(MS_U32 u32Reg, MS_U8 u8Val, MS_U8 u8Mask);

// CMD buffer
INTERFACE void Hal_HVSP_SetCMDQTrigCfg(ST_HVSP_CMD_TRIG_CONFIG stCmdTrigCfg);
INTERFACE void Hal_HVSP_GetCMDQTrigCfg(ST_HVSP_CMD_TRIG_CONFIG *pCfg);
INTERFACE void Hal_HVSP_SetCMDQTrigFrameCnt(MS_U16 u16Idx);
INTERFACE void Hal_HVSP_SetCMDQTrigFire(void);
INTERFACE void Hal_HVSP_SetFrameBufferManageLock(MS_BOOL bEn);
INTERFACE void Hal_HVSP_SetLDCPathSel(MS_BOOL bEn);
INTERFACE MS_BOOL Hal_HVSP_GetLDCPathSel(void);
INTERFACE void Hal_HVSP_SetLDCBypass(MS_BOOL bEn);
INTERFACE void Hal_HVSP_SetLDCHeight(MS_U16 u16Height);
INTERFACE void Hal_HVSP_SetLDCWidth(MS_U16 u16Width);
INTERFACE void Hal_HVSP_SetLDCPitch(MS_U32 u32Pitch);
INTERFACE void Hal_HVSP_SetLDCBase(MS_U32 u32Base);
INTERFACE void Hal_HVSP_SetLDCHWrwDiff(MS_U16 u8Val);
INTERFACE void Hal_HVSP_SetLDCSW_Mode(MS_U8 bEn);
INTERFACE void Hal_HVSP_SetLDCSW_Idx(MS_U8 idx);
INTERFACE void Hal_HVSP_SetOSDLocate(EN_HVSP_ID_TYPE enID,EN_DRV_HVSP_OSD_LOC_TYPE enLoc);
INTERFACE void Hal_HVSP_SetOSDbypass(EN_HVSP_ID_TYPE enID,MS_BOOL bEn);
INTERFACE void Hal_HVSP_SetOSDOnOff(EN_HVSP_ID_TYPE enID,MS_BOOL bEn);
INTERFACE void Hal_HVSP_SetOSDbypassWTM(EN_HVSP_ID_TYPE enID,MS_BOOL bEn);
INTERFACE EN_DRV_HVSP_OSD_LOC_TYPE Hal_HVSP_GetOSDLocate(EN_HVSP_ID_TYPE enID);
INTERFACE MS_BOOL Hal_HVSP_GetOSDbypass(EN_HVSP_ID_TYPE enID);
INTERFACE MS_BOOL Hal_HVSP_GetOSDOnOff(EN_HVSP_ID_TYPE enID);
INTERFACE MS_BOOL Hal_HVSP_GetOSDbypassWTM(EN_HVSP_ID_TYPE enID);
INTERFACE void Hal_HVSP_SetIdclkOnOff(MS_BOOL bEn,ST_HVSP_CLK_CONFIG* stclk);
INTERFACE void Hal_HVSP_FCLK1(ST_HVSP_CLK_CONFIG *stclk);
INTERFACE void Hal_HVSP_FCLK2(ST_HVSP_CLK_CONFIG *stclk);
INTERFACE MS_U16 Hal_HVSP_GetHVSPOutputHeight(EN_HVSP_ID_TYPE enID);
INTERFACE MS_U16 Hal_HVSP_GetHVSPOutputHeightCount(EN_HVSP_ID_TYPE enID);
INTERFACE MS_U16 Hal_HVSP_GetScalingFunctionStatus(EN_HVSP_ID_TYPE enID);
INTERFACE MS_U16 Hal_HVSP_GetHVSPInputHeight(EN_HVSP_ID_TYPE enID);
INTERFACE MS_U16 Hal_HVSP_GetHVSPInputWidth(EN_HVSP_ID_TYPE enID);
INTERFACE MS_U16 Hal_HVSP_Get_Crop_X(void);
INTERFACE MS_U16 Hal_HVSP_GetHVSPOutputWidth(EN_HVSP_ID_TYPE enID);
INTERFACE MS_U16 Hal_HVSP_Get_Crop_Y(void);
INTERFACE MS_U16 Hal_HVSP_GetCrop2Yinfo(void);
INTERFACE MS_U16 Hal_HVSP_GetCrop2Xinfo(void);
INTERFACE void Hal_HVSP_SetBT656SrcConfig(MS_BOOL bEn,MS_U16 u16Lineoffset);
INTERFACE MS_U16 Hal_HVSP_GetCrop1WidthCount(void);
INTERFACE MS_U16 Hal_HVSP_GetCrop1HeightCount(void);
INTERFACE MS_U16 Hal_HVSP_GetCrop1Width(void);
INTERFACE MS_U16 Hal_HVSP_GetCrop1Height(void);
INTERFACE MS_U16 Hal_HVSP_GetCrop2InputWidth(void);
INTERFACE MS_U16 Hal_HVSP_GetCrop2InputHeight(void);
INTERFACE MS_U16 Hal_HVSP_GetCrop2OutputWidth(void);
INTERFACE MS_U16 Hal_HVSP_GetCrop2OutputHeight(void);
INTERFACE MS_U16 Hal_HVSP_Get_Crop2_CountH(void);
INTERFACE MS_U16 Hal_HVSP_Get_Crop2_CountV(void);
INTERFACE MS_U16 Hal_HVSP_GetCrop2En(void);
INTERFACE void Hal_HVSP_SetCLKRate(unsigned char u8Idx);
INTERFACE MS_U32 Hal_HVSP_GetCMDQStatus(void);
INTERFACE MS_U16 Hal_HVSP_GetDMAHeight(EN_HVSP_MONITOR_TYPE enMonitorType);
INTERFACE MS_U16 Hal_HVSP_GetDMAHeightCount(EN_HVSP_MONITOR_TYPE enMonitorType);
INTERFACE MS_U16 Hal_HVSP_GetDMAEn(EN_HVSP_MONITOR_TYPE enMonitorType);
INTERFACE void Hal_VIP_SRAM_Dump(EN_VIP_SRAM_DUMP_TYPE endump,MS_BOOL u8Sram);
#undef INTERFACE

#endif
