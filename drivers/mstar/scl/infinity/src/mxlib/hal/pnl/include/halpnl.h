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
#ifndef _HAL_PNL_H
#define _HAL_PNL_H


//-------------------------------------------------------------------------------------------------
//  Defines & ENUM
//-------------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------
//  Structure
//-------------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------
//  Prototype
//-------------------------------------------------------------------------------------------------
#ifndef HAL_PNL_C
#define INTERFACE extern
#else
#define INTERFACE
#endif

INTERFACE void   Hal_PNL_Set_Riu_Base(MS_U32 u32Riubase);
INTERFACE void   Hal_PNL_Set_Chiptop(void);
INTERFACE void   Hal_PNL_Set_Init_Y2R(void);
INTERFACE void   Hal_PNL_Set_VSync_St(MS_U16 u16Val);
INTERFACE void   Hal_PNL_Set_VSync_End(MS_U16 u16Val);
INTERFACE void   Hal_PNL_Set_Vfde_St(MS_U16 u16Val);
INTERFACE void   Hal_PNL_Set_Vfde_End(MS_U16 u16Val);
INTERFACE void   Hal_PNL_Set_Vde_St(MS_U16 u16Val);
INTERFACE void   Hal_PNL_Set_Vde_End(MS_U16 u16Val);
INTERFACE void   Hal_PNL_Set_Vtt(MS_U16 u16Val);
INTERFACE void   Hal_PNL_Set_HSync_St(MS_U16 u16Val);
INTERFACE void   Hal_PNL_Set_HSync_End(MS_U16 u16Val);
INTERFACE void   Hal_PNL_Set_Hfde_St(MS_U16 u16Val);
INTERFACE void   Hal_PNL_Set_Hfde_End(MS_U16 u16Val);
INTERFACE void   Hal_PNL_Set_Hde_St(MS_U16 u16Val);
INTERFACE void   Hal_PNL_Set_Hde_End(MS_U16 u16Val);
INTERFACE void   Hal_PNL_Set_Htt(MS_U16 u16Val);
INTERFACE void   Hal_PNL_Set_FrameColr_En(MS_BOOL bEn);
INTERFACE void   Hal_PNL_Set_FrameColor(MS_U32 u32Color);
INTERFACE void   Hal_PNL_Dump_Lpll_Setting(MS_U16 u16Idx);
INTERFACE void   Hal_PNL_Set_Lpll_Set(MS_U32 u32LpllSet);
INTERFACE MS_U16 Hal_PNL_Get_Lpll_Idx(MS_U64 u64Dclk);
INTERFACE MS_U16 Hal_PNL_Get_Lpll_Gain(MS_U16 u16Idx);
INTERFACE MS_U16 Hal_PNL_Get_Lpll_Div(MS_U16 u16Idx);
INTERFACE void   Hal_PNL_Set_CSC_Y2R_En(MS_BOOL bEn);
INTERFACE void   Hal_PNL_Set_CSC_Y2R_Offset(MS_U8 u8Y, MS_U8 u8Cb, MS_U8 u8Cr);
INTERFACE void   Hal_PNL_Set_CSC_Y2R_Coef(MS_U16 *pu16Coef);
INTERFACE void Hal_PNL_Set_OpenLpll_CLK(MS_U8 bLpllClk);


#undef INTERFACE

#endif
