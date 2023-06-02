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
#ifndef _HAL_VIP_H
#define _HAL_VIP_H


//-------------------------------------------------------------------------------------------------
//  Defines & ENUM
//-------------------------------------------------------------------------------------------------
typedef enum
{
    EN_VIP_SRAM_SEC_0 =289,
    EN_VIP_SRAM_SEC_1 =272,
    EN_VIP_SRAM_SEC_2 =272,
    EN_VIP_SRAM_SEC_3 =256,
}EN_VIP_SRAM_SEC_NUM;

typedef enum
{
    EN_VIP_SRAM_DUMP_ICC,
    EN_VIP_SRAM_DUMP_IHC,
    EN_VIP_SRAM_DUMP_HVSP,
    EN_VIP_SRAM_DUMP_HVSP_1,
    EN_VIP_SRAM_DUMP_HVSP_2,
}EN_VIP_SRAM_DUMP_TYPE;
typedef enum
{
    EN_VIP_VTRACK_ENABLE_ON,
    EN_VIP_VTRACK_ENABLE_OFF,
    EN_VIP_VTRACK_ENABLE_DEBUG,
}EN_VIP_VTRACK_ENABLE_TYPE;
//-------------------------------------------------------------------------------------------------
//  Structure
//-------------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------
//  Prototype
//-------------------------------------------------------------------------------------------------
#ifndef HAL_VIP_C
#define INTERFACE extern
#else
#define INTERFACE
#endif
INTERFACE void Hal_VIP_SetLDCFrameBufferSetting(MS_U8 u8Val);
INTERFACE void Hal_VIP_SetLDC422_444_allMd(MS_U8 u8md);
INTERFACE void Hal_VIP_Set_Riu_Base(MS_U32 u32riubase);
INTERFACE void Hal_VIP_DLCHistVarOnOff(MS_U16 u16var);
INTERFACE void Hal_VIP_SetDLCstatMIU(MS_U8 u8value,MS_U32 u32addr1,MS_U32 u32addr2);
INTERFACE void Hal_VIP_SetDLCshift(MS_U8 u8value);
INTERFACE void HAl_VIP_SetDLCmode(MS_U8 u8value);
INTERFACE void Hal_VIP_DLCHistSetRange(MS_U8 u8value,MS_U8 u8range);
INTERFACE MS_U32 Hal_VIP_DLC_Hist_GetRange(MS_U8 u8range);
INTERFACE void Hal_VIP_SetLDCBypass(MS_BOOL bEn);
INTERFACE void Hal_VIP_SetLDCDmapPitch(MS_U32 u32Pitch);
INTERFACE void Hal_VIP_SetLDCDmapBase(MS_U32 u32Base);
INTERFACE void Hal_VIP_SetLDCDmapPS(MS_U8 bEn);
INTERFACE void Hal_VIP_SetLDCDmapOffset(MS_U8 u8offset);
INTERFACE void Hal_VIP_SetLDCSramAmount(MS_U16 u16hor,MS_U16 u16ver);
INTERFACE void Hal_VIP_SetLDCSramBase(MS_U32 u32hor,MS_U32 u32ver);
INTERFACE void Hal_VIP_SetLDCSramStr(MS_U16 u16hor,MS_U16 u16ver);
INTERFACE MS_U8 Hal_VIP_DLCGetBaseidx(void);
INTERFACE MS_U32 Hal_VIP_DLCGetPC(void);
INTERFACE MS_U32 Hal_VIP_DLCGetPW(void);
INTERFACE MS_U8 Hal_VIP_DLCGetMinP(void);
INTERFACE MS_U8 Hal_VIP_DLCGetMaxP(void);
INTERFACE void Hal_VIP_SetDLCActWin(MS_BOOL bEn,MS_U16 u16Vst,MS_U16 u16Hst,MS_U16 u16Vnd,MS_U16 u16Hnd);
INTERFACE void Hal_VIP_SetDNRIPMRead(MS_BOOL bEn);

INTERFACE void Hal_VIP_SetAutodownloadAddr(MS_U32 u32baseadr,MS_U16 u16iniaddr,MS_U8 u8cli);
INTERFACE void Hal_VIP_SetAutodownloadReq(MS_U16 u16depth,MS_U16 u16reqlen,MS_U8 u8cli);
INTERFACE void Hal_VIP_SetAutodownload(MS_U8 bCLientEn,MS_U8 btrigContinue,MS_U8 u8cli);
INTERFACE void Hal_VIP_SetAutodownloadTimer(MS_U8 bCLientEn);
INTERFACE void Hal_VIP_GetNLMSRAM(MS_U16 u16entry);
INTERFACE void Hal_VIP_SetNLMSRAMbyCPU(MS_U16 u16entry,MS_U32 u32tvalue);
INTERFACE void Hal_VIP_SRAM_Dump(EN_VIP_SRAM_DUMP_TYPE endump,MS_BOOL u8Sram);
INTERFACE void Hal_VIP_VtrackEnable(MS_U8 u8FrameRate, EN_VIP_VTRACK_ENABLE_TYPE bEnable);
INTERFACE void Hal_VIP_VtrackSetUserDefindedSetting(MS_BOOL bUserDefinded, MS_U8 *pu8Setting);
INTERFACE void Hal_VIP_VtrackSetKey(MS_BOOL bUserDefinded, MS_U8 *pu8Setting);
INTERFACE void Hal_VIP_VtrackSetPayloadData(MS_U16 u16Timecode, MS_U8 u8OperatorID);
INTERFACE MS_BOOL Hal_VIP_GetVIPBypass(void);
INTERFACE MS_BOOL Hal_VIP_GetDNRBypass(void);
INTERFACE MS_BOOL Hal_VIP_GetSNRBypass(void);
INTERFACE MS_BOOL Hal_VIP_GetLDCBypass(void);
INTERFACE MS_BOOL Hal_VIP_GetNLMBypass(void);
    #undef INTERFACE

#endif
