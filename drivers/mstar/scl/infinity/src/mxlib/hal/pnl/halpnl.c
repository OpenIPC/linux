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
// ("MStar Confidential Information") by the recipient.
// Any unauthorized act including without limitation unauthorized disclosure,
// copying, use, reproduction, sale, distribution, modification, disassembling,
// reverse engineering and compiling of the contents of MStar Confidential
// Information is unlawful and strictly prohibited. MStar hereby reserves the
// rights to any and all damages, losses, costs and expenses resulting therefrom.
//
////////////////////////////////////////////////////////////////////////////////
#define HAL_PNL_C

#ifdef MSOS_TYPE_LINUX_KERNEL
#include <linux/wait.h>
#include <linux/irqreturn.h>
#include <asm/div64.h>
#endif

//-------------------------------------------------------------------------------------------------
//  Include Files
//-------------------------------------------------------------------------------------------------
// Common Definition
#include "MsCommon.h"
#include "MsOS.h"
#include "ms_platform.h"
// Internal Definition
#include "hwreg.h"
#include "halpnl_utility.h"
#include "halpnl.h"
#include "Infinity_pnl_lpll_tbl.h"
#include "MsDbg.h"
//-------------------------------------------------------------------------------------------------
//  Driver Compiler Options
//-------------------------------------------------------------------------------------------------


//-------------------------------------------------------------------------------------------------
//  Defines & Macro
//-------------------------------------------------------------------------------------------------
#define DCLK_5MHZ           5000000
#define DCLK_10MHZ         10000000
#define DCLK_20MHZ         20000000
#define DCLK_40MHZ         40000000
#define DCLK_80MHZ         80000000
#define Is_Package_BGA()  (Chip_Get_Package_Type()==MS_PACKAGE_BGA)
#define Is_Package_QFP()  (Chip_Get_Package_Type()==MS_PACKAGE_QFP)
#define Is_Dclk_Less5M(Dclk)      ((Dclk) <= DCLK_5MHZ)
#define Is_Dclk_5MTo10M(Dclk)       ((Dclk > DCLK_5MHZ) && (Dclk <= DCLK_10MHZ))
#define Is_Dclk_10MTo20M(Dclk)      ((Dclk > DCLK_10MHZ) && (Dclk <= DCLK_20MHZ))
#define Is_Dclk_20MTo40M(Dclk)      ((Dclk > DCLK_20MHZ) && (Dclk <= DCLK_40MHZ))
#define Is_Dclk_40MTo80M(Dclk)      ((Dclk > DCLK_40MHZ) && (Dclk <= DCLK_80MHZ))
//-------------------------------------------------------------------------------------------------
//  Variable

MS_U32 PNL_RIU_BASE = 0;


//-------------------------------------------------------------------------------------------------
//  Functions
//-------------------------------------------------------------------------------------------------
void Hal_PNL_Set_Riu_Base(MS_U32 u32Riubase)
{
    PNL_RIU_BASE = u32Riubase;
}

void Hal_PNL_Set_Chiptop(void)
{
    MS_U16 u16chip_top;
    u16chip_top=R2BYTE(REG_CHIPTOP_0F_L);
    if(Is_Package_BGA())
    {
        W2BYTEMSK(REG_CHIPTOP_0F_L,(u16chip_top|0x0040),0x0040);
        //W2BYTE(0x101eA0,0x0000);
    }
    else if(Is_Package_QFP())
    {
        W2BYTEMSK(REG_CHIPTOP_0F_L,(u16chip_top|0x0000),0x0040);
        printf("[PNL]QFP!!!  OPEN PNL ERROR ");
    }
}

void Hal_PNL_Set_Init_Y2R(void)
{
    MS_U16 u16Coef[9]={0xcc4,0x950,0x3ffc,0x397e,0x0950,0x3cde,0x3ffe,0x950,0x1024};//hw setting
    Hal_PNL_Set_CSC_Y2R_En(1);
    Hal_PNL_Set_CSC_Y2R_Offset(0x10,0x80,0x80);
    Hal_PNL_Set_CSC_Y2R_Coef(u16Coef);
}
void Hal_PNL_Set_VSync_St(MS_U16 u16Val)
{
    W2BYTEMSK(REG_SCL2_01_L, u16Val, 0x07FF);
}

void Hal_PNL_Set_VSync_End(MS_U16 u16Val)
{
    W2BYTEMSK(REG_SCL2_02_L, u16Val, 0x07FF);
}

void Hal_PNL_Set_Vfde_St(MS_U16 u16Val)
{
    W2BYTEMSK(REG_SCL2_03_L, u16Val, 0x07FF);
}

void Hal_PNL_Set_Vfde_End(MS_U16 u16Val)
{
    W2BYTEMSK(REG_SCL2_04_L, u16Val, 0x07FF);
}

void Hal_PNL_Set_Vde_St(MS_U16 u16Val)
{
    W2BYTEMSK(REG_SCL2_05_L, u16Val, 0x07FF);
}

void Hal_PNL_Set_Vde_End(MS_U16 u16Val)
{
    W2BYTEMSK(REG_SCL2_06_L, u16Val, 0x07FF);
}

void Hal_PNL_Set_Vtt(MS_U16 u16Val)
{
    W2BYTEMSK(REG_SCL2_07_L, u16Val, 0x07FF);
}

void Hal_PNL_Set_HSync_St(MS_U16 u16Val)
{
    W2BYTEMSK(REG_SCL2_09_L, u16Val, 0x07FF);
}

void Hal_PNL_Set_HSync_End(MS_U16 u16Val)
{
    W2BYTEMSK(REG_SCL2_0A_L, u16Val, 0x07FF);
}

void Hal_PNL_Set_Hfde_St(MS_U16 u16Val)
{
    W2BYTEMSK(REG_SCL2_0B_L, u16Val, 0x07FF);
}

void Hal_PNL_Set_Hfde_End(MS_U16 u16Val)
{
    W2BYTEMSK(REG_SCL2_0C_L, u16Val, 0x07FF);
}

void Hal_PNL_Set_Hde_St(MS_U16 u16Val)
{
    W2BYTEMSK(REG_SCL2_0D_L, u16Val, 0x07FF);
}

void Hal_PNL_Set_Hde_End(MS_U16 u16Val)
{
    W2BYTEMSK(REG_SCL2_0E_L, u16Val, 0x07FF);
}

void Hal_PNL_Set_Htt(MS_U16 u16Val)
{
    W2BYTEMSK(REG_SCL2_0F_L, u16Val, 0x07FF);
}

void Hal_PNL_Set_FrameColr_En(MS_BOOL bEn)
{
    W2BYTEMSK(REG_SCL2_10_L, bEn ? BIT8 : 0, BIT8);
}

#if ENABLE_PNL_UNUSED_FUNCTION
void Hal_PNL_Set_FrameColor(MS_U32 u32Color)
{
    W4BYTE(REG_SCL2_11_L, u32Color);
}
#endif

MS_U16 Hal_PNL_Get_Lpll_Idx(MS_U64 u64Dclk)
{
    MS_U16 u16Idx = 0;

    if(Is_Dclk_Less5M(u64Dclk))
    {
        u16Idx = E_PNL_SUPPORTED_LPLL_HS_LVDS_CH_5TO5MHZ;
    }
    else if(Is_Dclk_5MTo10M(u64Dclk))
    {
        u16Idx = E_PNL_SUPPORTED_LPLL_HS_LVDS_CH_5TO10MHZ;
    }
    else if(Is_Dclk_10MTo20M(u64Dclk))
    {
        u16Idx = E_PNL_SUPPORTED_LPLL_HS_LVDS_CH_10TO20MHZ;
    }
    else if(Is_Dclk_20MTo40M(u64Dclk))
    {
        u16Idx = E_PNL_SUPPORTED_LPLL_HS_LVDS_CH_20TO40MHZ;
    }
    else if(Is_Dclk_40MTo80M(u64Dclk))
    {
        u16Idx = E_PNL_SUPPORTED_LPLL_HS_LVDS_CH_40TO80MHZ;
    }
    else
    {
        u16Idx = 0xFF;
    }
    return u16Idx;
}

MS_U16 Hal_PNL_Get_Lpll_Gain(MS_U16 u16Idx)
{
    return u16LoopGain[u16Idx];
}

MS_U16 Hal_PNL_Get_Lpll_Div(MS_U16 u16Idx)
{
    return u16LoopDiv[u16Idx];
}

void Hal_PNL_Dump_Lpll_Setting(MS_U16 u16Idx)
{
    MS_U16 u16RegIdx;

    for(u16RegIdx=0; u16RegIdx < LPLL_REG_NUM; u16RegIdx++)
    {

        if(LPLLSettingTBL[u16Idx][u16RegIdx].address == 0xFF)
        {
            MsOS_DelayTask(LPLLSettingTBL[u16Idx][u16RegIdx].value);
            continue;
        }

        W2BYTEMSK((REG_SCL_LPLL_BASE | ((MS_U32)LPLLSettingTBL[u16Idx][u16RegIdx].address *2)),
                  LPLLSettingTBL[u16Idx][u16RegIdx].value,
                  LPLLSettingTBL[u16Idx][u16RegIdx].mask);
    }

}

void Hal_PNL_Set_Lpll_Set(MS_U32 u32LpllSet)
{
    MS_U16 u16LpllSet_Lo, u16LpllSet_Hi;
    u16LpllSet_Lo = (MS_U16)(u32LpllSet & 0x0000FFFF);
    u16LpllSet_Hi = (MS_U16)((u32LpllSet & 0x00FF0000) >> 16);
    W2BYTE(REG_SCL_LPLL_48_L, u16LpllSet_Lo);
    W2BYTE(REG_SCL_LPLL_49_L, u16LpllSet_Hi);
}

void Hal_PNL_Set_OpenLpll_CLK(MS_U8 bLpllClk)
{
    MS_U16 u16byte;
    W2BYTEMSK(REG_SCL_CLK_66_L, bLpllClk ? 0x000C : 0x1, 0x000F);
    u16byte=R2BYTE(REG_CHIPTOP_0F_L);
    W2BYTEMSK(REG_CHIPTOP_0F_L, bLpllClk ? (0x0041|u16byte) : u16byte, 0x0041);
}
// CSC Y2R
void Hal_PNL_Set_CSC_Y2R_En(MS_BOOL bEn)
{
    W2BYTEMSK(REG_SCL2_19_L, bEn ? BIT0 : 0, BIT0);
}

void Hal_PNL_Set_CSC_Y2R_Offset(MS_U8 u8Y, MS_U8 u8Cb, MS_U8 u8Cr)
{
    W2BYTEMSK(REG_SCL2_19_L, ((MS_U16)u8Y << 8), 0xFF00);
    W2BYTEMSK(REG_SCL2_1A_L, (((MS_U16)u8Cb << 8)|((MS_U16)u8Cr)), 0xFFFF);
}

void Hal_PNL_Set_CSC_Y2R_Coef(MS_U16 *pu16Coef)
{

    MS_U8 u8idx;

    for(u8idx=0; u8idx<9; u8idx++)
    {
        W2BYTEMSK(REG_SCL2_1B_L + (u8idx<<1), pu16Coef[u8idx], 0x3FFF);
    }
}


#undef HAL_PNL_C
