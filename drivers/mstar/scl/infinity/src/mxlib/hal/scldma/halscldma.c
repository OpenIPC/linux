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
#define HAL_SCLDMA_C

#ifdef MSOS_TYPE_LINUX_KERNEL
#include <linux/wait.h>
#include <linux/irqreturn.h>
#endif
#include <asm/div64.h>

//-------------------------------------------------------------------------------------------------
//  Include Files
//-------------------------------------------------------------------------------------------------
// Common Definition
#include "MsCommon.h"
#include "MsOS.h"

// Internal Definition
#include "hwreg.h"
#include "halscldma_utility.h"
#include "drvscldma_st.h"
#include "halscldma.h"
#include <linux/clk.h>
#include <linux/clk-provider.h>
//-------------------------------------------------------------------------------------------------
//  Driver Compiler Options
//-------------------------------------------------------------------------------------------------


//-------------------------------------------------------------------------------------------------
//  Defines & Macro
//-------------------------------------------------------------------------------------------------
#define HAL_SCLDMA_ERR(x)  x
#define DISABLE_CLK 0x1
#define LOW_CLK 0x4
//-------------------------------------------------------------------------------------------------
//  Variable
//-------------------------------------------------------------------------------------------------
MS_U32 SCLDMA_RIU_BASE = 0;
MS_U16 u16gheight[2]  = {1080,720};


//-------------------------------------------------------------------------------------------------
//  Functions
//-------------------------------------------------------------------------------------------------
void Hal_SCLDMA_SetRiuBase(MS_U32 u32riubase)
{
    SCLDMA_RIU_BASE = u32riubase;
}
void Hal_SCLDMA_Set_SW_Reset(void)
{
    W2BYTEMSK(REG_SCL0_01_L,BIT0,BIT0);
    W2BYTEMSK(REG_SCL0_01_L,0,BIT0);
}
void Hal_SCLDMA_SetReqLen(EN_SCLDMA_CLIENT_TYPE enSCLDMA_ID, MS_U8 u8value)
{
    if(enSCLDMA_ID == E_SCLDMA_1_FRM_W)
    {
        W2BYTEMSK(REG_SCL_DMA0_19_L, ((MS_U16)u8value<<8), 0xFF00);
    }
    else if(enSCLDMA_ID == E_SCLDMA_1_SNP_W)
    {
        W2BYTEMSK(REG_SCL_DMA0_49_L, ((MS_U16)u8value<<8), 0xFF00);
    }
    else if(enSCLDMA_ID == E_SCLDMA_1_IMI_W)
    {
        W2BYTEMSK(REG_SCL_DMA1_49_L, ((MS_U16)u8value<<8), 0xFF00);
    }
    else if(enSCLDMA_ID == E_SCLDMA_2_FRM_W)
    {
        W2BYTEMSK(REG_SCL_DMA0_61_L, ((MS_U16)u8value<<8), 0xFF00);
    }
    else if(enSCLDMA_ID == E_SCLDMA_2_IMI_W)
    {
        W2BYTEMSK(REG_SCL_DMA0_79_L, ((MS_U16)u8value<<8), 0xFF00);
    }
    else if(enSCLDMA_ID == E_SCLDMA_3_FRM_R)
    {
        W2BYTEMSK(REG_SCL_DMA1_31_L, ((MS_U16)u8value<<8), 0xFF00);
    }
    else if(enSCLDMA_ID == E_SCLDMA_3_FRM_W)
    {
        W2BYTEMSK(REG_SCL_DMA1_19_L, ((MS_U16)u8value<<8), 0xFF00);
    }
    else if(enSCLDMA_ID == E_SCLDMA_4_FRM_R)
    {
        W2BYTEMSK(REG_SCL_DMA0_31_L, ((MS_U16)u8value<<8), 0xFF00);
    }
    else
    {
        HAL_SCLDMA_ERR(printf("%s %d, wrong enSCLDMA_ID: %d\n", __FUNCTION__, __LINE__, enSCLDMA_ID));
    }
}
void Hal_SCLDMA_SetReqTh(EN_SCLDMA_CLIENT_TYPE enSCLDMA_ID, MS_U8 u8value)
{
    if(enSCLDMA_ID == E_SCLDMA_1_FRM_W)
    {
        W2BYTEMSK(REG_SCL_DMA0_19_L, ((MS_U16)u8value), 0x00FF);
    }
    else if(enSCLDMA_ID == E_SCLDMA_1_SNP_W)
    {
        W2BYTEMSK(REG_SCL_DMA0_49_L, ((MS_U16)u8value), 0x00FF);
    }
    else if(enSCLDMA_ID == E_SCLDMA_1_IMI_W)
    {
        W2BYTEMSK(REG_SCL_DMA1_49_L, ((MS_U16)u8value), 0x00FF);
    }
    else if(enSCLDMA_ID == E_SCLDMA_2_FRM_W)
    {
        W2BYTEMSK(REG_SCL_DMA0_61_L, ((MS_U16)u8value), 0x00FF);
    }
    else if(enSCLDMA_ID == E_SCLDMA_2_IMI_W)
    {
        W2BYTEMSK(REG_SCL_DMA0_79_L, ((MS_U16)u8value), 0x00FF);
    }
    else if(enSCLDMA_ID == E_SCLDMA_3_FRM_R)
    {
        W2BYTEMSK(REG_SCL_DMA1_31_L, ((MS_U16)u8value), 0x00FF);
    }
    else if(enSCLDMA_ID == E_SCLDMA_3_FRM_W)
    {
        W2BYTEMSK(REG_SCL_DMA1_19_L, ((MS_U16)u8value), 0x00FF);
    }
    else if(enSCLDMA_ID == E_SCLDMA_4_FRM_R)
    {
        W2BYTEMSK(REG_SCL_DMA0_31_L, ((MS_U16)u8value), 0x00FF);
    }
    else
    {
        HAL_SCLDMA_ERR(printf("%s %d, wrong enSCLDMA_ID: %d\n", __FUNCTION__, __LINE__, enSCLDMA_ID));
    }
}
void Hal_SCLDMA_SetPriThd(EN_SCLDMA_CLIENT_TYPE enSCLDMA_ID, MS_U8 u8value)
{
    if(enSCLDMA_ID == E_SCLDMA_1_FRM_W)
    {
        W2BYTEMSK(REG_SCL_DMA0_18_L, ((MS_U16)u8value<<8), 0xFF00);
    }
    else if(enSCLDMA_ID == E_SCLDMA_1_SNP_W)
    {
        W2BYTEMSK(REG_SCL_DMA0_48_L, ((MS_U16)u8value<<8), 0xFF00);
    }
    else if(enSCLDMA_ID == E_SCLDMA_1_IMI_W)
    {
        W2BYTEMSK(REG_SCL_DMA1_48_L, ((MS_U16)u8value<<8), 0xFF00);
    }
    else if(enSCLDMA_ID == E_SCLDMA_2_FRM_W)
    {
        W2BYTEMSK(REG_SCL_DMA0_60_L, ((MS_U16)u8value<<8), 0xFF00);
    }
    else if(enSCLDMA_ID == E_SCLDMA_2_IMI_W)
    {
        W2BYTEMSK(REG_SCL_DMA0_78_L, ((MS_U16)u8value<<8), 0xFF00);
    }
    else if(enSCLDMA_ID == E_SCLDMA_3_FRM_R)
    {
        W2BYTEMSK(REG_SCL_DMA1_30_L, ((MS_U16)u8value<<8), 0xFF00);
    }
    else if(enSCLDMA_ID == E_SCLDMA_3_FRM_W)
    {
        W2BYTEMSK(REG_SCL_DMA1_18_L, ((MS_U16)u8value<<8), 0xFF00);
    }
    else if(enSCLDMA_ID == E_SCLDMA_4_FRM_R)
    {
        W2BYTEMSK(REG_SCL_DMA0_30_L, ((MS_U16)u8value<<8), 0xFF00);
    }
    else
    {
        HAL_SCLDMA_ERR(printf("%s %d, wrong enSCLDMA_ID: %d\n", __FUNCTION__, __LINE__, enSCLDMA_ID));
    }
}
void Hal_SCLDMA_HKForceAuto(void)
{
    // sc22sc2_hk_force_auto, sc2out_hk_force_auto
    W2BYTEMSK(REG_SCL0_03_L, BIT11|BIT10|BIT9|BIT8|BIT1|BIT0, BIT11|BIT10|BIT9|BIT8|BIT1|BIT0);
}
void Hal_SCLDMA_HWInit(void)
{
    Hal_SCLDMA_HKForceAuto();
    Hal_SCLDMA_SetMCMByDMAClient(E_SCLDMA_1_SNP_W,0);
    Hal_SCLDMA_SetPriThd(E_SCLDMA_3_FRM_R,0x08);
    Hal_SCLDMA_SetReqLen(E_SCLDMA_3_FRM_R,0x10);
    Hal_SCLDMA_SetReqTh(E_SCLDMA_3_FRM_R,0x12);
    Hal_SCLDMA_SetReqLen(E_SCLDMA_3_FRM_W,0x10);
    Hal_SCLDMA_SetReqTh(E_SCLDMA_3_FRM_W,0x12);
    Hal_SCLDMA_SetReqLen(E_SCLDMA_1_FRM_W,0x10);
    Hal_SCLDMA_SetReqTh(E_SCLDMA_1_FRM_W,0x12);
    Hal_SCLDMA_SetReqLen(E_SCLDMA_1_SNP_W,0x10);
    Hal_SCLDMA_SetReqTh(E_SCLDMA_1_SNP_W,0x12);
    Hal_SCLDMA_SetReqLen(E_SCLDMA_2_FRM_W,0x10);
    Hal_SCLDMA_SetReqTh(E_SCLDMA_2_FRM_W,0x12);
    Hal_SCLDMA_Set_SW_Reset();

}
#if CONFIG_OF
void Hal_SCLDMA_CLKInit(MS_BOOL bEn,ST_SCLDMA_CLK_CONFIG *stclk)
{
    struct clk* stclock = NULL;
    MS_U16 regclk;
    if(bEn)
    {
        if (__clk_get_enable_count(stclk->fclk1) == 0)
        {
            if(u16gheight[0] > 720)
            {
                if (NULL != (stclock = clk_get_parent_by_index(stclk->fclk1, 0)))
                {
                    clk_set_parent(stclk->fclk1, stclock);
                    clk_prepare_enable(stclk->fclk1);
                    regclk = R2BYTE(REG_SCL_CLK_64_L);
                    //printf("[SCLDMA]flag:%ld index:%hx\n",__clk_get_rate(stclk->fclk1),regclk);
                }
            }
            else
            {
                if (NULL != (stclock = clk_get_parent_by_index(stclk->fclk1, 1)))
                {
                    clk_set_parent(stclk->fclk1, stclock);
                    clk_prepare_enable(stclk->fclk1);
                    regclk = R2BYTE(REG_SCL_CLK_64_L);
                    //printf("[SCLDMA]flag:%ld index:%hx\n",__clk_get_rate(stclk->fclk1),regclk);
                }
            }
        }
        else
        {
            if(u16gheight[0] > 720 && __clk_get_rate(stclk->fclk1) < 172000000)
            {
                if (NULL != (stclock = clk_get_parent_by_index(stclk->fclk1, 0)))
                {
                    clk_set_parent(stclk->fclk1, stclock);
                    regclk = R2BYTE(REG_SCL_CLK_64_L);
                    //printf("[SCLDMA]flag:%ld index:%hx\n",__clk_get_rate(stclk->fclk1),regclk);
                }
            }
            else if(u16gheight[0] <= 720 && __clk_get_rate(stclk->fclk1) >= 172000000)
            {
                if (NULL != (stclock = clk_get_parent_by_index(stclk->fclk1, 1)))
                {
                    clk_set_parent(stclk->fclk1, stclock);
                    regclk = R2BYTE(REG_SCL_CLK_64_L);
                    //printf("[SCLDMA]flag:%ld index:%hx\n",__clk_get_rate(stclk->fclk1),regclk);
                }
            }
        }

    }
    else
    {
        if (NULL != (stclock = clk_get_parent_by_index(stclk->fclk1, 0)))
        {
            clk_set_parent(stclk->fclk1, stclock);
        }
        while (__clk_get_enable_count(stclk->fclk1))
        {
            clk_disable_unprepare(stclk->fclk1);
        }
    }
}

void Hal_SCLDMA_SC3CLKInit(MS_BOOL bEn,ST_SCLDMA_CLK_CONFIG *stclk)
{
    struct clk* stclock = NULL;
    if(bEn)
    {
        if (__clk_get_enable_count(stclk->fclk2) == 0)
        {
            if(u16gheight[1] > 720)
            {
                if (NULL != (stclock = clk_get_parent_by_index(stclk->fclk2, 0)))
                {
                    clk_set_parent(stclk->fclk2, stclock);
                    clk_prepare_enable(stclk->fclk2);
                }
            }
            else
            {
                if (NULL != (stclock = clk_get_parent_by_index(stclk->fclk2, 1)))
                {
                    clk_set_parent(stclk->fclk2, stclock);
                    clk_prepare_enable(stclk->fclk2);
                }

            }
        }
        else
        {
            if(u16gheight[1] > 720 && __clk_get_rate(stclk->fclk2) < 172000000)
            {
                if (NULL != (stclock = clk_get_parent_by_index(stclk->fclk2, 0)))
                {
                    clk_set_parent(stclk->fclk2, stclock);
                }
            }
            else if(u16gheight[1] <= 720 && __clk_get_rate(stclk->fclk2) >= 172000000)
            {
                if (NULL != (stclock = clk_get_parent_by_index(stclk->fclk2, 1)))
                {
                    clk_set_parent(stclk->fclk2, stclock);
                }
            }
        }

    }
    else
    {
        if (NULL != (stclock = clk_get_parent_by_index(stclk->fclk2, 0)))
        {
            clk_set_parent(stclk->fclk2, stclock);
        }
        while (__clk_get_enable_count(stclk->fclk2))
        {
            clk_disable_unprepare(stclk->fclk2);
        }
    }
}
void Hal_SCLDMA_ODCLKInit(MS_BOOL bEn,ST_SCLDMA_CLK_CONFIG *stclk)
{
    struct clk* pstclock = NULL;
    if(bEn)
    {
        if(__clk_get_enable_count(stclk->odclk)==0)
        {
            if (NULL != (pstclock = clk_get_parent_by_index(stclk->odclk, 3)))
            {
                clk_set_parent(stclk->odclk, pstclock);
                clk_prepare_enable(stclk->odclk);
            }
            else
            {
                printf("[odclk]NULL\n");

            }
        }
        else
        {
            printf("[odclk]CAN'T/Already OPEN\n");
        }

    }
    else
    {
        if (NULL != (pstclock = clk_get_parent_by_index(stclk->odclk, 0)))
        {
            clk_set_parent(stclk->odclk, pstclock);
        }
        while (__clk_get_enable_count(stclk->odclk))
        {
            clk_disable_unprepare(stclk->odclk);
        }
    }
}

#else
void Hal_SCLDMA_CLKInit(MS_BOOL bEn,ST_SCLDMA_CLK_CONFIG *stclk)
{
    MS_U16 regclk;
    printf("[hal]NO OF\n");
    if(bEn)
    {
        if ((R2BYTE(REG_SCL_CLK_64_L)&DISABLE_CLK))
        {
            if(u16gheight[0] > 720)
            {
                W2BYTEMSK(REG_SCL_CLK_64_L,0x0000,0x000F);//h61
                regclk = R2BYTE(REG_SCL_CLK_64_L);
            }
            else
            {
                W2BYTEMSK(REG_SCL_CLK_64_L,LOW_CLK,0x000F);//h61
                regclk = R2BYTE(REG_SCL_CLK_64_L);
            }
        }
        else
        {
            if(u16gheight[0] > 720 && (R2BYTE(REG_SCL_CLK_64_L)&LOW_CLK))
            {
                W2BYTEMSK(REG_SCL_CLK_64_L,0x0000,0x000F);//h61
                regclk = R2BYTE(REG_SCL_CLK_64_L);
            }
            else if(u16gheight[0] <= 720 && !(R2BYTE(REG_SCL_CLK_64_L)&LOW_CLK))
            {
                W2BYTEMSK(REG_SCL_CLK_64_L,LOW_CLK,0x000F);//h61
                regclk = R2BYTE(REG_SCL_CLK_64_L);
            }
        }

    }
    else
    {
        W2BYTEMSK(REG_SCL_CLK_64_L,0x0,0x000F);//h61
    }
}

void Hal_SCLDMA_SC3CLKInit(MS_BOOL bEn,ST_SCLDMA_CLK_CONFIG *stclk)
{
    MS_U16 regclk;
    if(bEn)
    {
        if ((R2BYTE(REG_SCL_CLK_65_L)&DISABLE_CLK))
        {
            if(u16gheight[1] > 720)
            {
                W2BYTEMSK(REG_SCL_CLK_65_L,0x0000,0x000F);//h61
                regclk = R2BYTE(REG_SCL_CLK_65_L);
            }
            else
            {
                W2BYTEMSK(REG_SCL_CLK_65_L,LOW_CLK,0x000F);//h61
                regclk = R2BYTE(REG_SCL_CLK_65_L);
            }
        }
        else
        {
            if(u16gheight[1] > 720 && (R2BYTE(REG_SCL_CLK_65_L)&LOW_CLK))
            {
                W2BYTEMSK(REG_SCL_CLK_65_L,0x0000,0x000F);//h61
                regclk = R2BYTE(REG_SCL_CLK_65_L);
            }
            else if(u16gheight[1] <= 720 && !(R2BYTE(REG_SCL_CLK_65_L)&LOW_CLK))
            {
                W2BYTEMSK(REG_SCL_CLK_65_L,LOW_CLK,0x000F);//h61
                regclk = R2BYTE(REG_SCL_CLK_65_L);
            }
        }

    }
    else
    {
        W2BYTEMSK(REG_SCL_CLK_65_L,0x0,0x000F);//h61
    }
}
void Hal_SCLDMA_ODCLKInit(MS_BOOL bEn,ST_SCLDMA_CLK_CONFIG *stclk)
{
    struct clk* pstclock = NULL;
    if(bEn)
    {
        if((R2BYTE(REG_SCL_CLK_66_L)&DISABLE_CLK))
        {
            W2BYTEMSK(REG_SCL_CLK_66_L,0x000C,0x000F);//h66
        }
        else
        {
            printf("[odclk]CAN'T/Already OPEN\n");
        }

    }
    else
    {
        W2BYTEMSK(REG_SCL_CLK_66_L,0x000D,0x000F);//h66
    }
}

#endif

void Hal_SCLDMA_SetSC2HandshakeForce(EN_SCLDMA_RW_MODE_TYPE enRWMode, MS_BOOL bEn)
{
    if(enRWMode == E_SCLDMA_FRM_W)
    {
        W2BYTEMSK(REG_SCL0_02_L, bEn ? 0 : BIT0, BIT0);
    }
    else if(enRWMode == E_SCLDMA_IMI_W)
    {
        W2BYTEMSK(REG_SCL0_02_L, bEn ? 0 : BIT1, BIT1);
    }
}

void Hal_SCLDMA_SetSC1HandshakeForce(EN_SCLDMA_RW_MODE_TYPE enRWMode, MS_BOOL bEn)
{
    if(enRWMode == E_SCLDMA_FRM_W)
    {
        W2BYTEMSK(REG_SCL0_02_L, bEn ? 0 : BIT9, BIT9);
    }
    else if(enRWMode == E_SCLDMA_SNP_W)
    {
        W2BYTEMSK(REG_SCL0_02_L, bEn ? 0 : BIT10, BIT10);
    }
    else if(enRWMode == E_SCLDMA_IMI_W)
    {
        W2BYTEMSK(REG_SCL0_02_L, bEn ? 0 : BIT11, BIT11);
    }

    //HAL_SCLDMA_ERR(printf("[HalSCLDMA]HK_Force:%hx\n", R2BYTE(REG_SCL0_02_L)));
}

void Hal_SCLDMA_SetSC1ToSC2HandshakeForce(MS_BOOL bEn)
{
    W2BYTEMSK(REG_SCL0_02_L, bEn ? 0 : BIT8, BIT8);
}
void Hal_SCLDMA_SetMCMByDMAClient(EN_SCLDMA_CLIENT_TYPE enSCLDMA_ID, MS_U8 u8value)
{
    if(enSCLDMA_ID == E_SCLDMA_1_FRM_W)
    {
        W2BYTEMSK(REG_MCM_06_L, ((MS_U16)u8value<<8), 0xFF00);
    }
    else if(enSCLDMA_ID == E_SCLDMA_1_SNP_W)
    {
        W2BYTEMSK(REG_MCM_07_L, u8value, 0xFF);
    }
    else if(enSCLDMA_ID == E_SCLDMA_1_IMI_W)
    {
        W2BYTEMSK(REG_MCM_10_L, u8value, 0xFF);
    }
    else if(enSCLDMA_ID == E_SCLDMA_2_FRM_W)
    {
        W2BYTEMSK(REG_MCM_08_L, u8value, 0xFF);
    }
    else if(enSCLDMA_ID == E_SCLDMA_2_IMI_W)
    {
        W2BYTEMSK(REG_MCM_10_L, ((MS_U16)u8value<<8), 0xFF00);
    }
    else if(enSCLDMA_ID == E_SCLDMA_3_FRM_W)
    {
        W2BYTEMSK(REG_MCM_08_L, ((MS_U16)u8value<<8), 0xFF00);
    }
    else if(enSCLDMA_ID == E_SCLDMA_4_FRM_R)
    {
        W2BYTEMSK(REG_MCM_07_L, ((MS_U16)u8value<<8), 0xFF00);
    }
    else
    {
        HAL_SCLDMA_ERR(printf("%s %d, wrong enSCLDMA_ID: %d\n", __FUNCTION__, __LINE__, enSCLDMA_ID));
    }
}
void Hal_SCLDMA_SetCheckFrmEndSignal(EN_SCLDMA_CLIENT_TYPE enSCLDMA_ID, MS_BOOL bEn)
{
    if(enSCLDMA_ID == E_SCLDMA_1_FRM_W)
    {
        W2BYTEMSK(REG_SCL_DMA2_50_L, bEn ? BIT11 : 0, BIT11);
    }
    else if(enSCLDMA_ID == E_SCLDMA_1_SNP_W)
    {
        W2BYTEMSK(REG_SCL_DMA2_50_L, bEn ? BIT12 : 0, BIT12);
    }
    else if(enSCLDMA_ID == E_SCLDMA_1_IMI_W)
    {
        W2BYTEMSK(REG_SCL_DMA2_50_L, bEn ? BIT13 : 0, BIT13);
    }
    else if(enSCLDMA_ID == E_SCLDMA_2_FRM_W)
    {
        W2BYTEMSK(REG_SCL_DMA2_50_L, bEn ? BIT10 : 0, BIT10);
    }
    else if(enSCLDMA_ID == E_SCLDMA_2_IMI_W)
    {
        W2BYTEMSK(REG_SCL_DMA2_50_L, bEn ? BIT9 : 0, BIT9);
    }
    else if(enSCLDMA_ID == E_SCLDMA_3_FRM_W)
    {
        W2BYTEMSK(REG_SCL_DMA2_50_L, bEn ? BIT8 : 0, BIT8);
    }
    else
    {
        HAL_SCLDMA_ERR(printf("%s %d, wrong enSCLDMA_ID: %d\n", __FUNCTION__, __LINE__, enSCLDMA_ID));
    }
}

void Hal_SCLDMA_SetSC1DMAEn(EN_SCLDMA_RW_MODE_TYPE enRWMode, MS_BOOL bEn)
{
    if(enRWMode == E_SCLDMA_FRM_W)
    {
        W2BYTEMSK(REG_SCL_DMA0_01_L, bEn ? BIT15 : 0, BIT15);
    }
    else if(enRWMode == E_SCLDMA_SNP_W)
    {
        W2BYTEMSK(REG_SCL_DMA0_03_L, bEn ? BIT15 : 0, BIT15);
    }
    else if(enRWMode == E_SCLDMA_IMI_W)
    {
        W2BYTEMSK(REG_SCL_DMA1_03_L, bEn ? BIT15 : 0, BIT15);
    }
    else
    {
        HAL_SCLDMA_ERR(printf("%s %d, wrong RWmode: %d\n", __FUNCTION__, __LINE__, enRWMode));
    }
    //HAL_SCLDMA_ERR(printf("[HalSCLDMA]DMA1_En:%hx\n", R2BYTE(REG_SCL_DMA0_01_L)));

}
void Hal_SCLDMA_GetHVSPResolutionForEnsure(MS_U16 u16height)
{
    MS_U16 u16Srcheight,u16Dspheight,u16FBheight;
    u16Srcheight = R2BYTE(REG_SCL_HVSP0_21_L);
    u16Dspheight = R2BYTE(REG_SCL_HVSP0_23_L);
    u16FBheight  = R2BYTE(REG_SCL_DNR1_0D_L);
    if(u16Srcheight>0)
    {
        u16gheight[0] = (u16Srcheight>u16Dspheight) ? u16Srcheight : u16Dspheight ;
        u16gheight[0] = (u16FBheight>u16gheight[0])? u16FBheight:u16gheight[0];
    }
    u16Srcheight = R2BYTE(REG_SCL_HVSP2_21_L);
    u16Dspheight = R2BYTE(REG_SCL_HVSP2_23_L);
    if(u16Srcheight>0)
    {
        u16gheight[1] = (u16Srcheight>u16Dspheight) ? u16Srcheight : u16Dspheight ;
    }

}
MS_U16 Hal_GetHVSPOutputHSize(EN_SCLDMA_ID_TYPE enSCLDMA_ID)
{
    MS_U16 u16Width;
    u16Width = (enSCLDMA_ID == E_SCLDMA_ID_1_W) ?R2BYTE(REG_SCL_HVSP0_22_L) :
               (enSCLDMA_ID == E_SCLDMA_ID_2_W) ?R2BYTE(REG_SCL_HVSP1_22_L) :
               (enSCLDMA_ID == E_SCLDMA_ID_3_W) ?R2BYTE(REG_SCL_HVSP2_22_L) :
               (enSCLDMA_ID == E_SCLDMA_ID_3_R) ?R2BYTE(REG_SCL_HVSP2_20_L) :
                0;
    return u16Width;
}
MS_U16 Hal_GetHVSPOutputVSize(EN_SCLDMA_ID_TYPE enSCLDMA_ID)
{
    MS_U16 u16Height;
    u16Height = (enSCLDMA_ID == E_SCLDMA_ID_1_W) ? R2BYTE(REG_SCL_HVSP0_23_L) :
                (enSCLDMA_ID == E_SCLDMA_ID_2_W) ? R2BYTE(REG_SCL_HVSP1_23_L) :
                (enSCLDMA_ID == E_SCLDMA_ID_3_W) ? R2BYTE(REG_SCL_HVSP2_23_L) :
                (enSCLDMA_ID == E_SCLDMA_ID_3_R) ? R2BYTE(REG_SCL_HVSP2_21_L) :
                0;
    return u16Height;
}
void Hal_SCLDMA_SetSC1DMAConfig(ST_SCLDMA_RW_CONFIG stSCLDMACfg)
{
    MS_U32 u32yoffset;
    MS_U32 u32coffset;
    if(stSCLDMACfg.enRWMode == E_SCLDMA_FRM_W)
    {
        if(stSCLDMACfg.bvFlag.btsBase_0)
        {
            W4BYTE(REG_SCL_DMA0_08_L, stSCLDMACfg.u32Base_Y[0]>>3);
            W4BYTE(REG_SCL_DMA0_10_L, stSCLDMACfg.u32Base_C[0]>>3);
        }
        if(stSCLDMACfg.bvFlag.btsBase_1)
        {
            W4BYTE(REG_SCL_DMA0_0A_L, stSCLDMACfg.u32Base_Y[1]>>3);
            W4BYTE(REG_SCL_DMA0_12_L, stSCLDMACfg.u32Base_C[1]>>3);
        }
        if(stSCLDMACfg.bvFlag.btsBase_2)
        {
            W4BYTE(REG_SCL_DMA0_0C_L, stSCLDMACfg.u32Base_Y[2]>>3);
            W4BYTE(REG_SCL_DMA0_14_L, stSCLDMACfg.u32Base_C[2]>>3);
        }
        if(stSCLDMACfg.bvFlag.btsBase_3)
        {
            W4BYTE(REG_SCL_DMA0_0E_L, stSCLDMACfg.u32Base_Y[3]>>3);
            W4BYTE(REG_SCL_DMA0_16_L, stSCLDMACfg.u32Base_C[3]>>3);
        }

        W2BYTEMSK(REG_SCL_DMA0_18_L, stSCLDMACfg.u8MaxIdx<<3, BIT4|BIT3);
        Hal_SCLDMA_GetHVSPResolutionForEnsure(stSCLDMACfg.u16Height);
        W2BYTEMSK(REG_SCL_DMA0_1E_L, stSCLDMACfg.enColor == E_SCLDMA_COLOR_YUV420 ? BIT3 : 0, BIT3);   // w_422to420_md[3]
        W2BYTEMSK(REG_SCL_DMA0_1E_L, BIT2, BIT2);
        W2BYTEMSK(REG_SCL_DMA0_01_L, stSCLDMACfg.enColor == E_SCLDMA_COLOR_YUV422 ? BIT14 : 0, BIT14); // w_422_pack[14]
    }
    else if(stSCLDMACfg.enRWMode == E_SCLDMA_SNP_W)
    {
        if(stSCLDMACfg.bvFlag.btsBase_0)
        {
            W4BYTE(REG_SCL_DMA0_38_L, stSCLDMACfg.u32Base_Y[0]>>3);
            W4BYTE(REG_SCL_DMA0_40_L, stSCLDMACfg.u32Base_C[0]>>3);
        }
        if(stSCLDMACfg.bvFlag.btsBase_1)
        {
            W4BYTE(REG_SCL_DMA0_3A_L, stSCLDMACfg.u32Base_Y[1]>>3);
            W4BYTE(REG_SCL_DMA0_42_L, stSCLDMACfg.u32Base_C[1]>>3);
        }
        if(stSCLDMACfg.bvFlag.btsBase_2)
        {
            W4BYTE(REG_SCL_DMA0_3C_L, stSCLDMACfg.u32Base_Y[2]>>3);
            W4BYTE(REG_SCL_DMA0_44_L, stSCLDMACfg.u32Base_C[2]>>3);
        }
        if(stSCLDMACfg.bvFlag.btsBase_3)
        {
            W4BYTE(REG_SCL_DMA0_3E_L, stSCLDMACfg.u32Base_Y[3]>>3);
            W4BYTE(REG_SCL_DMA0_46_L, stSCLDMACfg.u32Base_C[3]>>3);
        }

        W2BYTEMSK(REG_SCL_DMA0_48_L, stSCLDMACfg.u8MaxIdx<<3, BIT4|BIT3);
        W2BYTEMSK(REG_SCL_DMA0_4E_L, stSCLDMACfg.enColor == E_SCLDMA_COLOR_YUV420 ? BIT3 : 0, BIT3);   // w_422to420_md[3]
        W2BYTEMSK(REG_SCL_DMA0_4E_L, BIT2, BIT2);
        W2BYTEMSK(REG_SCL_DMA0_03_L, stSCLDMACfg.enColor == E_SCLDMA_COLOR_YUV422 ? BIT14 : 0, BIT14); // w_422_pack[14]
    }
    else if(stSCLDMACfg.enRWMode == E_SCLDMA_IMI_W)
    {
#if ENABLE_RING_DB
        u32yoffset = 0x20;
        u32coffset = 0x20;
#else
        if(stSCLDMACfg.enColor == E_SCLDMA_COLOR_YUV420)
        {
            u32yoffset=(stSCLDMACfg.u16Width*16)/8-1;
            u32coffset=(stSCLDMACfg.u16Width*8)/8-1;
        }
        else if(stSCLDMACfg.enColor == E_SCLDMA_COLOR_YUV422)
        {
            u32yoffset=(stSCLDMACfg.u16Width*16)/8-2;
            u32coffset=(stSCLDMACfg.u16Width*16)/8-2;
        }
#endif
        if(stSCLDMACfg.bvFlag.btsBase_0)
        {
            W4BYTE(REG_SCL_DMA1_38_L, stSCLDMACfg.u32Base_Y[0]>>3);
            W4BYTE(REG_SCL_DMA1_40_L, stSCLDMACfg.u32Base_C[0]>>3);
        }
        if(stSCLDMACfg.bvFlag.btsBase_1)
        {
            W4BYTE(REG_SCL_DMA1_3A_L, stSCLDMACfg.u32Base_Y[1]>>3);
            W4BYTE(REG_SCL_DMA1_42_L, stSCLDMACfg.u32Base_C[1]>>3);
        }
        if(stSCLDMACfg.bvFlag.btsBase_2)
        {
            W4BYTE(REG_SCL_DMA1_3C_L, stSCLDMACfg.u32Base_Y[2]>>3);
            W4BYTE(REG_SCL_DMA1_44_L, stSCLDMACfg.u32Base_C[2]>>3);
        }
        if(stSCLDMACfg.bvFlag.btsBase_3)
        {
            W4BYTE(REG_SCL_DMA1_3E_L, stSCLDMACfg.u32Base_Y[3]>>3);
            W4BYTE(REG_SCL_DMA1_46_L, stSCLDMACfg.u32Base_C[3]>>3);
        }

        W2BYTEMSK(REG_SCL_DMA1_48_L, stSCLDMACfg.u8MaxIdx<<3, BIT4|BIT3);
        W2BYTEMSK(REG_SCL_DMA1_4E_L, stSCLDMACfg.enColor == E_SCLDMA_COLOR_YUV420 ? BIT3 : 0, BIT3);   // w_422to420_md[3]
        W2BYTEMSK(REG_SCL_DMA1_4E_L, BIT2, BIT2);
        W2BYTEMSK(REG_SCL_DMA1_03_L, stSCLDMACfg.enColor == E_SCLDMA_COLOR_YUV422 ? BIT14 : 0, BIT14); // w_422_pack[14]
        W4BYTE(REG_SCL_DMA2_40_L, u32yoffset); // imi offset y
        W4BYTE(REG_SCL_DMA2_42_L, u32coffset); // imi offset c
        W2BYTEMSK(REG_SCL0_64_L,0, BIT15); // sc1 open
    }
    else
    {
        HAL_SCLDMA_ERR(printf("%s %d, wrong RWmode: %d\n",  __FUNCTION__, __LINE__, stSCLDMACfg.enRWMode));
    }
    if(Hal_GetHVSPOutputHSize(E_SCLDMA_ID_1_W)==stSCLDMACfg.u16Width ||(Hal_GetHVSPOutputHSize(E_SCLDMA_ID_1_W) == 0))
    {
        W2BYTEMSK(REG_SCL_DMA0_1A_L, stSCLDMACfg.u16Width, 0xFFFF);
        W2BYTEMSK(REG_SCL_DMA1_4A_L, stSCLDMACfg.u16Width, 0xFFFF);
        W2BYTEMSK(REG_SCL_DMA0_4A_L, stSCLDMACfg.u16Width, 0xFFFF);
    }
    else
    {
        W2BYTEMSK(REG_SCL_DMA0_1A_L, Hal_GetHVSPOutputHSize(E_SCLDMA_ID_1_W), 0xFFFF);
        W2BYTEMSK(REG_SCL_DMA1_4A_L, Hal_GetHVSPOutputHSize(E_SCLDMA_ID_1_W), 0xFFFF);
        W2BYTEMSK(REG_SCL_DMA0_4A_L, Hal_GetHVSPOutputHSize(E_SCLDMA_ID_1_W), 0xFFFF);
    }
    if(Hal_GetHVSPOutputVSize(E_SCLDMA_ID_1_W) == stSCLDMACfg.u16Height ||(Hal_GetHVSPOutputVSize(E_SCLDMA_ID_1_W) == 0))
    {
        W2BYTEMSK(REG_SCL_DMA0_1B_L, stSCLDMACfg.u16Height,0xFFFF);
        W2BYTEMSK(REG_SCL_DMA1_4B_L, stSCLDMACfg.u16Height,0xFFFF);
        W2BYTEMSK(REG_SCL_DMA0_4B_L, stSCLDMACfg.u16Height,0xFFFF);
    }
    else
    {
        W2BYTEMSK(REG_SCL_DMA0_1A_L, Hal_GetHVSPOutputVSize(E_SCLDMA_ID_1_W), 0xFFFF);
        W2BYTEMSK(REG_SCL_DMA1_4A_L, Hal_GetHVSPOutputVSize(E_SCLDMA_ID_1_W), 0xFFFF);
        W2BYTEMSK(REG_SCL_DMA0_4A_L, Hal_GetHVSPOutputVSize(E_SCLDMA_ID_1_W), 0xFFFF);
    }

}
void Hal_SCLDMA_SetIMIClientReset(void)
{
    MS_U32 u32yoffset;
    MS_U32 u32coffset;
    u32yoffset = 0x0;
    u32coffset = 0x0;
    W4BYTE(REG_SCL_DMA1_38_L, 0);
    W4BYTE(REG_SCL_DMA1_40_L, 0);
    W4BYTE(REG_SCL_DMA1_3A_L, 0);
    W4BYTE(REG_SCL_DMA1_42_L, 0);
    W4BYTE(REG_SCL_DMA1_3C_L, 0);
    W4BYTE(REG_SCL_DMA1_44_L, 0);
    W4BYTE(REG_SCL_DMA1_3E_L, 0);
    W4BYTE(REG_SCL_DMA1_46_L, 0);

    W2BYTEMSK(REG_SCL_DMA1_48_L, 0, BIT4|BIT3);
    W2BYTEMSK(REG_SCL_DMA1_4A_L, 0, 0xFFFF);
    W2BYTEMSK(REG_SCL_DMA1_4B_L, 0, 0xFFFF);
    W2BYTEMSK(REG_SCL_DMA1_4E_L, 0, BIT3);   // w_422to420_md[3]
    W2BYTEMSK(REG_SCL_DMA1_4E_L, 0, BIT2);
    W2BYTEMSK(REG_SCL_DMA1_03_L, 0, BIT14); // w_422_pack[14]
    W4BYTE(REG_SCL_DMA2_40_L, u32yoffset); // imi offset y
    W4BYTE(REG_SCL_DMA2_42_L, u32coffset); // imi offset c
    W2BYTEMSK(REG_SCL0_64_L,0, BIT15); // sc1 open

}
void Hal_SCLDMA_SetDMAOutputBufferAddr
    (EN_SCLDMA_CLIENT_TYPE enClientType,unsigned long u32YBufferAddr,unsigned long u32CBufferAddr)
{
    switch(enClientType)
    {
    case E_SCLDMA_1_FRM_W:
        W4BYTE(REG_SCL_DMA0_08_L, u32YBufferAddr>>3);
        W4BYTE(REG_SCL_DMA0_10_L, u32CBufferAddr>>3);
        break;

    case E_SCLDMA_1_IMI_W:
        W4BYTE(REG_SCL_DMA1_38_L, u32YBufferAddr>>3);
        W4BYTE(REG_SCL_DMA1_40_L, u32CBufferAddr>>3);
        break;

    case E_SCLDMA_1_SNP_W:
        W4BYTE(REG_SCL_DMA0_38_L, u32YBufferAddr>>3);
        W4BYTE(REG_SCL_DMA0_40_L, u32CBufferAddr>>3);
        break;

    case E_SCLDMA_2_FRM_W:
        W4BYTE(REG_SCL_DMA0_50_L, u32YBufferAddr>>3);
        W4BYTE(REG_SCL_DMA0_58_L, u32CBufferAddr>>3);
        break;

    case E_SCLDMA_2_IMI_W:
        W4BYTE(REG_SCL_DMA1_38_L, u32YBufferAddr>>3);
        W4BYTE(REG_SCL_DMA1_40_L, u32CBufferAddr>>3);
        break;

    case E_SCLDMA_3_FRM_R:
        W4BYTE(REG_SCL_DMA1_20_L, u32YBufferAddr>>3);
        W4BYTE(REG_SCL_DMA1_28_L, u32CBufferAddr>>3);
        break;

    case E_SCLDMA_3_FRM_W:
        W4BYTE(REG_SCL_DMA1_08_L, u32YBufferAddr>>3);
        W4BYTE(REG_SCL_DMA1_10_L, u32CBufferAddr>>3);
        break;

    case E_SCLDMA_4_FRM_R:
        W4BYTE(REG_SCL_DMA0_20_L, u32YBufferAddr>>3);
        W4BYTE(REG_SCL_DMA0_28_L, u32CBufferAddr>>3);
        break;
    default:
        break;
    }

}

void Hal_SCLDMA_SetSC2DMAEn(EN_SCLDMA_RW_MODE_TYPE enRWMode, MS_BOOL bEn)
{
    if(enRWMode == E_SCLDMA_FRM_W)
    {
        W2BYTEMSK(REG_SCL_DMA0_04_L, bEn ? BIT15 : 0, BIT15);
    }
    else if(enRWMode == E_SCLDMA_IMI_W)
    {
        W2BYTEMSK(REG_SCL_DMA1_03_L, bEn ? BIT15 : 0, BIT15);
    }
    else
    {
        HAL_SCLDMA_ERR(printf("%s %d, wrong RWmode: %d\n",  __FUNCTION__, __LINE__, enRWMode));
    }

}

void Hal_SCLDMA_SetSC2DMAConfig(ST_SCLDMA_RW_CONFIG stSCLDMACfg)
{
    MS_U32 u32yoffset;
    MS_U32 u32coffset;
    if(stSCLDMACfg.enRWMode == E_SCLDMA_FRM_W)
    {
        if(stSCLDMACfg.bvFlag.btsBase_0)
        {
            W4BYTE(REG_SCL_DMA0_50_L, stSCLDMACfg.u32Base_Y[0]>>3);
            W4BYTE(REG_SCL_DMA0_58_L, stSCLDMACfg.u32Base_C[0]>>3);
        }
        if(stSCLDMACfg.bvFlag.btsBase_1)
        {
            W4BYTE(REG_SCL_DMA0_52_L, stSCLDMACfg.u32Base_Y[1]>>3);
            W4BYTE(REG_SCL_DMA0_5A_L, stSCLDMACfg.u32Base_C[1]>>3);
        }
        if(stSCLDMACfg.bvFlag.btsBase_2)
        {
            W4BYTE(REG_SCL_DMA0_54_L, stSCLDMACfg.u32Base_Y[2]>>3);
            W4BYTE(REG_SCL_DMA0_5C_L, stSCLDMACfg.u32Base_C[2]>>3);
        }
        if(stSCLDMACfg.bvFlag.btsBase_3)
        {
            W4BYTE(REG_SCL_DMA0_56_L, stSCLDMACfg.u32Base_Y[3]>>3);
            W4BYTE(REG_SCL_DMA0_5E_L, stSCLDMACfg.u32Base_C[3]>>3);
        }

        W2BYTEMSK(REG_SCL_DMA0_60_L, stSCLDMACfg.u8MaxIdx<<3, BIT4|BIT3);
        W2BYTEMSK(REG_SCL_DMA0_62_L, stSCLDMACfg.u16Width, 0xFFFF);
        W2BYTEMSK(REG_SCL_DMA0_63_L, stSCLDMACfg.u16Height,0xFFFF);
        W2BYTEMSK(REG_SCL_DMA0_66_L, BIT2, BIT2);
        W2BYTEMSK(REG_SCL_DMA0_66_L, stSCLDMACfg.enColor == E_SCLDMA_COLOR_YUV420 ? BIT3 : 0, BIT3);
        W2BYTEMSK(REG_SCL_DMA0_04_L, stSCLDMACfg.enColor == E_SCLDMA_COLOR_YUV422 ? BIT14 : 0, BIT14);
    }
    else if(stSCLDMACfg.enRWMode == E_SCLDMA_IMI_W)
    {

        if(stSCLDMACfg.enColor == E_SCLDMA_COLOR_YUV420)
        {
            u32yoffset=(stSCLDMACfg.u16Width*16)/8-1;
            u32coffset=(stSCLDMACfg.u16Width*8)/8-1;
        }
        else if(stSCLDMACfg.enColor == E_SCLDMA_COLOR_YUV422)
        {
            u32yoffset=(stSCLDMACfg.u16Width*16)/8-2;
            u32coffset=(stSCLDMACfg.u16Width*16)/8-2;
        }
        if(stSCLDMACfg.bvFlag.btsBase_0)
        {
            W4BYTE(REG_SCL_DMA1_38_L, stSCLDMACfg.u32Base_Y[0]>>3);
            W4BYTE(REG_SCL_DMA1_40_L, stSCLDMACfg.u32Base_C[0]>>3);
        }
        if(stSCLDMACfg.bvFlag.btsBase_1)
        {
            W4BYTE(REG_SCL_DMA1_3A_L, stSCLDMACfg.u32Base_Y[1]>>3);
            W4BYTE(REG_SCL_DMA1_42_L, stSCLDMACfg.u32Base_C[1]>>3);
        }
        if(stSCLDMACfg.bvFlag.btsBase_2)
        {
            W4BYTE(REG_SCL_DMA1_3C_L, stSCLDMACfg.u32Base_Y[2]>>3);
            W4BYTE(REG_SCL_DMA1_44_L, stSCLDMACfg.u32Base_C[2]>>3);
        }
        if(stSCLDMACfg.bvFlag.btsBase_3)
        {
            W4BYTE(REG_SCL_DMA1_3E_L, stSCLDMACfg.u32Base_Y[3]>>3);
            W4BYTE(REG_SCL_DMA1_46_L, stSCLDMACfg.u32Base_C[3]>>3);
        }

        W2BYTEMSK(REG_SCL_DMA1_48_L, stSCLDMACfg.u8MaxIdx<<3, BIT4|BIT3);
        W2BYTEMSK(REG_SCL_DMA1_4A_L, stSCLDMACfg.u16Width, 0xFFFF);
        W2BYTEMSK(REG_SCL_DMA1_4B_L, stSCLDMACfg.u16Height,0xFFFF);
        W2BYTEMSK(REG_SCL_DMA1_4E_L, stSCLDMACfg.enColor == E_SCLDMA_COLOR_YUV420 ? BIT3 : 0, BIT3);   // w_422to420_md[3]
        W2BYTEMSK(REG_SCL_DMA1_4E_L, BIT2, BIT2);
        W2BYTEMSK(REG_SCL_DMA1_03_L, stSCLDMACfg.enColor == E_SCLDMA_COLOR_YUV422 ? BIT14 : 0, BIT14); // w_422_pack[14]
        W4BYTE(REG_SCL_DMA2_40_L, u32yoffset); // imi offset y
        W4BYTE(REG_SCL_DMA2_42_L, u32coffset); // imi offset c
        W2BYTEMSK(REG_SCL0_64_L,BIT15, BIT15); //sc2 open
    }
    else
    {
        HAL_SCLDMA_ERR(printf("%s %d, wrong RWmode: %d\n",  __FUNCTION__, __LINE__, stSCLDMACfg.enRWMode));
    }

}


void Hal_SCLDMA_SetSC3DMAEn(EN_SCLDMA_RW_MODE_TYPE enRWMode, MS_BOOL bEn)
{
    if(enRWMode == E_SCLDMA_FRM_R)
    {
        W2BYTEMSK(REG_SCL_DMA1_02_L, bEn ? BIT15 : 0, BIT15);
    }
    else if(enRWMode == E_SCLDMA_FRM_W)
    {
        W2BYTEMSK(REG_SCL_DMA1_01_L, bEn ? BIT15 : 0, BIT15);
    }
    else
    {
        HAL_SCLDMA_ERR(printf("%s %d, wrong RWmode: %d\n",  __FUNCTION__, __LINE__, enRWMode));
    }

}

void Hal_SCLDMA_SetSC3DMAConfig(ST_SCLDMA_RW_CONFIG stSCLDMACfg)
{
    MS_U32 u32yoffset;
    MS_U32 u32coffset;
    if(stSCLDMACfg.enRWMode == E_SCLDMA_FRM_R)
    {
        if(stSCLDMACfg.bvFlag.btsBase_0)
        {
            W4BYTE(REG_SCL_DMA1_20_L, stSCLDMACfg.u32Base_Y[0]>>3);
            W4BYTE(REG_SCL_DMA1_28_L, stSCLDMACfg.u32Base_C[0]>>3);
        }
        if(stSCLDMACfg.bvFlag.btsBase_1)
        {
            W4BYTE(REG_SCL_DMA1_22_L, stSCLDMACfg.u32Base_Y[1]>>3);
            W4BYTE(REG_SCL_DMA1_2A_L, stSCLDMACfg.u32Base_C[1]>>3);
        }
        if(stSCLDMACfg.bvFlag.btsBase_2)
        {
            W4BYTE(REG_SCL_DMA1_24_L, stSCLDMACfg.u32Base_Y[2]>>3);
            W4BYTE(REG_SCL_DMA1_2C_L, stSCLDMACfg.u32Base_C[2]>>3);
        }
        if(stSCLDMACfg.bvFlag.btsBase_3)
        {
            W4BYTE(REG_SCL_DMA1_26_L, stSCLDMACfg.u32Base_Y[3]>>3);
            W4BYTE(REG_SCL_DMA1_2E_L, stSCLDMACfg.u32Base_C[3]>>3);
        }

        W2BYTEMSK(REG_SCL_DMA1_30_L, stSCLDMACfg.u8MaxIdx<<3, BIT4|BIT3);
        W2BYTEMSK(REG_SCL_DMA1_32_L, stSCLDMACfg.u16Width, 0xFFFF);
        W2BYTEMSK(REG_SCL_DMA1_33_L, stSCLDMACfg.u16Height,0xFFFF);
        W2BYTEMSK(REG_SCL_DMA1_36_L, stSCLDMACfg.enColor == E_SCLDMA_COLOR_YUV422 ? BIT7 : 0, BIT7);
        W2BYTEMSK(REG_SCL_DMA1_36_L, 0x06, 0x0F); //422to444_md[1:0], 420to422_md[2], 420to422_md_avg[3]
        W2BYTEMSK(REG_SCL_DMA1_02_L, stSCLDMACfg.enColor == E_SCLDMA_COLOR_YUV422 ? BIT14 : 0, BIT14);
    }
    else if(stSCLDMACfg.enRWMode == E_SCLDMA_FRM_W)
    {
        if(stSCLDMACfg.bvFlag.btsBase_0)
        {
            W4BYTE(REG_SCL_DMA1_08_L, stSCLDMACfg.u32Base_Y[0]>>3);
            W4BYTE(REG_SCL_DMA1_10_L, stSCLDMACfg.u32Base_C[0]>>3);
        }
        if(stSCLDMACfg.bvFlag.btsBase_1)
        {
            W4BYTE(REG_SCL_DMA1_0A_L, stSCLDMACfg.u32Base_Y[1]>>3);
            W4BYTE(REG_SCL_DMA1_12_L, stSCLDMACfg.u32Base_C[1]>>3);
        }
        if(stSCLDMACfg.bvFlag.btsBase_2)
        {
            W4BYTE(REG_SCL_DMA1_0C_L, stSCLDMACfg.u32Base_Y[2]>>3);
            W4BYTE(REG_SCL_DMA1_14_L, stSCLDMACfg.u32Base_C[2]>>3);
        }
        if(stSCLDMACfg.bvFlag.btsBase_3)
        {
            W4BYTE(REG_SCL_DMA1_0E_L, stSCLDMACfg.u32Base_Y[3]>>3);
            W4BYTE(REG_SCL_DMA1_16_L, stSCLDMACfg.u32Base_C[3]>>3);
        }

        W2BYTEMSK(REG_SCL_DMA1_18_L, stSCLDMACfg.u8MaxIdx<<3, BIT4|BIT3);
        W2BYTEMSK(REG_SCL_DMA1_1A_L, stSCLDMACfg.u16Width, 0xFFFF);
        W2BYTEMSK(REG_SCL_DMA1_1B_L, stSCLDMACfg.u16Height,0xFFFF);
        W2BYTEMSK(REG_SCL_DMA1_1E_L, stSCLDMACfg.enColor == E_SCLDMA_COLOR_YUV420 ? BIT3 : 0, BIT3);
        W2BYTEMSK(REG_SCL_DMA1_1E_L, BIT2, BIT2);
        W2BYTEMSK(REG_SCL_DMA1_01_L, stSCLDMACfg.enColor == E_SCLDMA_COLOR_YUV422 ? BIT14 : 0, BIT14);
        Hal_SCLDMA_GetHVSPResolutionForEnsure(stSCLDMACfg.u16Height);

    }
    else if(stSCLDMACfg.enRWMode == E_SCLDMA_IMI_R)
    {
        if(stSCLDMACfg.enColor == E_SCLDMA_COLOR_YUV420)
        {
            u32yoffset=(stSCLDMACfg.u16Width*16)/8-1;
            u32coffset=(stSCLDMACfg.u16Width*8)/8-1;
        }
        else if(stSCLDMACfg.enColor == E_SCLDMA_COLOR_YUV422)
        {
            u32yoffset=(stSCLDMACfg.u16Width*16)/8-2;
            u32coffset=(stSCLDMACfg.u16Width*16)/8-2;
        }
        if(stSCLDMACfg.bvFlag.btsBase_0)
        {
            W4BYTE(REG_SCL_DMA1_20_L, stSCLDMACfg.u32Base_Y[0]>>3);
            W4BYTE(REG_SCL_DMA1_28_L, stSCLDMACfg.u32Base_C[0]>>3);
        }
        if(stSCLDMACfg.bvFlag.btsBase_1)
        {
            W4BYTE(REG_SCL_DMA1_22_L, stSCLDMACfg.u32Base_Y[1]>>3);
            W4BYTE(REG_SCL_DMA1_2A_L, stSCLDMACfg.u32Base_C[1]>>3);
        }
        if(stSCLDMACfg.bvFlag.btsBase_2)
        {
            W4BYTE(REG_SCL_DMA1_24_L, stSCLDMACfg.u32Base_Y[2]>>3);
            W4BYTE(REG_SCL_DMA1_2C_L, stSCLDMACfg.u32Base_C[2]>>3);
        }
        if(stSCLDMACfg.bvFlag.btsBase_3)
        {
            W4BYTE(REG_SCL_DMA1_26_L, stSCLDMACfg.u32Base_Y[3]>>3);
            W4BYTE(REG_SCL_DMA1_2E_L, stSCLDMACfg.u32Base_C[3]>>3);
        }

        W2BYTEMSK(REG_SCL_DMA1_30_L, stSCLDMACfg.u8MaxIdx<<3, BIT4|BIT3);
        W2BYTEMSK(REG_SCL_DMA1_32_L, stSCLDMACfg.u16Width, 0xFFFF);
        W2BYTEMSK(REG_SCL_DMA1_33_L, stSCLDMACfg.u16Height,0xFFFF);

        W2BYTEMSK(REG_SCL_DMA1_36_L, stSCLDMACfg.enColor == E_SCLDMA_COLOR_YUV422 ? BIT7 : 0, BIT7);
        W2BYTEMSK(REG_SCL_DMA1_36_L, 0x06, 0x0F); //422to444_md[1:0], 420to422_md[2], 420to422_md_avg[3]
        W4BYTE(REG_SCL_DMA2_48_L, u32yoffset); // imi offset y
        W4BYTE(REG_SCL_DMA2_4A_L, u32coffset); // imi offset c
        W2BYTEMSK(REG_SCL_DMA2_02_L, 0x01,0x01); // SC3 mode
    }
    else
    {
        HAL_SCLDMA_ERR(printf("%s %d, wrong RWmode: %d\n",  __FUNCTION__, __LINE__, stSCLDMACfg.enRWMode));
    }

}

void Hal_SCLDMA_SetDisplayDMAEn(EN_SCLDMA_RW_MODE_TYPE enRWMode, MS_BOOL bEn)
{
    if(enRWMode == E_SCLDMA_DBG_R)
    {
        W2BYTEMSK(REG_SCL_DMA0_02_L, bEn ? BIT15 : 0, BIT15);
    }
    else
    {
        HAL_SCLDMA_ERR(printf("%s %d, wrong RWmode: %d\n",  __FUNCTION__, __LINE__, enRWMode));
    }

}


void Hal_SCLDMA_SetDisplayDMAConfig(ST_SCLDMA_RW_CONFIG stSCLDMACfg)
{

    if(stSCLDMACfg.enRWMode == E_SCLDMA_DBG_R)
    {
        if(stSCLDMACfg.bvFlag.btsBase_0)
        {
            W4BYTE(REG_SCL_DMA0_20_L, stSCLDMACfg.u32Base_Y[0]>>3);
            W4BYTE(REG_SCL_DMA0_28_L, stSCLDMACfg.u32Base_C[0]>>3);
        }
        if(stSCLDMACfg.bvFlag.btsBase_1)
        {
            W4BYTE(REG_SCL_DMA0_22_L, stSCLDMACfg.u32Base_Y[1]>>3);
            W4BYTE(REG_SCL_DMA0_2A_L, stSCLDMACfg.u32Base_C[1]>>3);
        }
        if(stSCLDMACfg.bvFlag.btsBase_2)
        {
            W4BYTE(REG_SCL_DMA0_24_L, stSCLDMACfg.u32Base_Y[2]>>3);
            W4BYTE(REG_SCL_DMA0_2C_L, stSCLDMACfg.u32Base_C[2]>>3);
        }
        if(stSCLDMACfg.bvFlag.btsBase_3)
        {
            W4BYTE(REG_SCL_DMA0_26_L, stSCLDMACfg.u32Base_Y[3]>>3);
            W4BYTE(REG_SCL_DMA0_2E_L, stSCLDMACfg.u32Base_C[3]>>3);
        }

        W2BYTEMSK(REG_SCL_DMA0_30_L, stSCLDMACfg.u8MaxIdx<<3, BIT4|BIT3);
        W2BYTEMSK(REG_SCL_DMA0_32_L, stSCLDMACfg.u16Width, 0xFFFF);
        W2BYTEMSK(REG_SCL_DMA0_33_L, stSCLDMACfg.u16Height,0xFFFF);
        W2BYTEMSK(REG_SCL_DMA0_36_L, stSCLDMACfg.enColor == E_SCLDMA_COLOR_YUV422 ? BIT7 : 0, BIT7);
        W2BYTEMSK(REG_SCL_DMA0_36_L, 0x06, 0x0F); //422to444_md[1:0], 420to422_md[2], 420to422_md_avg[3]
        W2BYTEMSK(REG_SCL_DMA0_02_L, stSCLDMACfg.enColor == E_SCLDMA_COLOR_YUV422 ? BIT14 : 0, BIT14);
    }
    else
    {
        HAL_SCLDMA_ERR(printf("%s %d, wrong RWmode: %d\n",  __FUNCTION__, __LINE__, stSCLDMACfg.enRWMode));
    }
}


//SCLDMA Trig
void Hal_SCLDMA_SetVSyncRegenMode(EN_SCLDMA_VS_ID_TYPE enID, EN_SCLDMA_VS_TRIG_MODE_TYPE enTrigMd)
{
    MS_U16 u16val, u16Mask;
    MS_U32 u32Reg;
    if(enID == E_SCLDMA_VS_ID_SC)
    {
        u16val = enTrigMd == E_SCLDMA_VS_TRIG_MODE_HW_IN_VSYNC ? 0x0000 :
                 enTrigMd == E_SCLDMA_VS_TRIG_MODE_SWTRIGGER       ? 0x0001 :
                                                                 0x0002;
        u16Mask = BIT1|BIT0;
        u32Reg = REG_SCL0_26_L;

    }
    else if(enID == E_SCLDMA_VS_ID_AFF)
    {

        u16val = enTrigMd == E_SCLDMA_VS_TRIG_MODE_HW_IN_VSYNC ? 0x0000 :
                 enTrigMd == E_SCLDMA_VS_TRIG_MODE_HW_DELAY    ? 0x0100 :
                 enTrigMd == E_SCLDMA_VS_TRIG_MODE_SWTRIGGER       ? 0x0200 :
                                                                 0x0300;

        u16Mask = BIT9|BIT8;
        u32Reg = REG_SCL0_26_L;

    }
    else if(enID == E_SCLDMA_VS_ID_LDC)
    {
        u16val = enTrigMd == E_SCLDMA_VS_TRIG_MODE_HW_IN_VSYNC ? 0x0000 :
                 enTrigMd == E_SCLDMA_VS_TRIG_MODE_HW_DELAY    ? 0x0001 :
                 enTrigMd == E_SCLDMA_VS_TRIG_MODE_SWTRIGGER       ? 0x0002 :
                                                                 0x0003 ;

        u16Mask = BIT1|BIT0;
        u32Reg = REG_SCL0_20_L;

    }
    else if(enID == E_SCLDMA_VS_ID_SC3)
    {
        u16val = enTrigMd == E_SCLDMA_VS_TRIG_MODE_HW_IN_VSYNC ? 0x0000 :
                 enTrigMd == E_SCLDMA_VS_TRIG_MODE_HW_DELAY    ? 0x0001 :
                 enTrigMd == E_SCLDMA_VS_TRIG_MODE_SWTRIGGER       ? 0x0002 :
                                                                 0x0003 ;

        u16Mask = BIT1|BIT0;
        u32Reg = REG_SCL0_22_L;

    }
    else if(enID == E_SCLDMA_VS_ID_DISP)
    {
        u16val = enTrigMd == E_SCLDMA_VS_TRIG_MODE_HW_IN_VSYNC ? 0x0000 :
                 enTrigMd == E_SCLDMA_VS_TRIG_MODE_HW_DELAY    ? 0x0001 :
                 enTrigMd == E_SCLDMA_VS_TRIG_MODE_SWTRIGGER       ? 0x0002 :
                                                                 0x0003 ;

        u16Mask = BIT1|BIT0;
        u32Reg = REG_SCL0_24_L;
    }
    else
    {
        return;
    }


    W2BYTEMSK(u32Reg, u16val, u16Mask);
}

void Hal_SCLDMA_SetRegenVSyncVariableWidthEn(MS_BOOL bEn)
{
    W2BYTEMSK(REG_SCL0_2D_L, bEn ? BIT7 : 0, BIT7);

}

void Hal_SCLDMA_SetRegenVSyncRefEdgeMode(EN_SCLDMA_VS_ID_TYPE enID, EN_SCLDMA_REGEN_VS_REF_MODE_TYPE enRefMd)
{
    MS_U8 u16val = enRefMd == E_SCLDMA_REF_VS_REF_MODE_RASING ? 1 : 0;

    if(enID == E_SCLDMA_VS_ID_SC)
    {
        W2BYTEMSK(REG_SCL0_2D_L, u16val, BIT0);
    }
    else if(enID == E_SCLDMA_VS_ID_AFF)
    {
        W2BYTEMSK(REG_SCL0_2D_L, u16val<<1, BIT1);
    }
    else if(enID == E_SCLDMA_VS_ID_LDC)
    {
        W2BYTEMSK(REG_SCL0_2D_L, u16val<<2, BIT2);
    }
    else if(enID == E_SCLDMA_VS_ID_SC3)
    {
        W2BYTEMSK(REG_SCL0_2D_L, u16val<<3, BIT3);
    }
    else if(enID == E_SCLDMA_VS_ID_DISP)
    {
        W2BYTEMSK(REG_SCL0_2D_L, u16val<<4, BIT4);
    }
    else
    {
    }
}

void Hal_SCLDAM_SetRegenVSyncWidth(EN_SCLDMA_VS_ID_TYPE enID, MS_U16 u16Vs_Width)
{
    if(enID == E_SCLDMA_VS_ID_SC)
    {
        W2BYTE(REG_SCL0_28_L, u16Vs_Width);
    }
    else if(enID == E_SCLDMA_VS_ID_AFF)
    {
        W2BYTE(REG_SCL0_29_L, u16Vs_Width);
    }
    else if(enID == E_SCLDMA_VS_ID_LDC)
    {
        W2BYTE(REG_SCL0_2A_L, u16Vs_Width);
    }
    else if(enID == E_SCLDMA_VS_ID_SC3)
    {
        W2BYTE(REG_SCL0_2B_L, u16Vs_Width);
    }
    else if(enID == E_SCLDMA_VS_ID_DISP)
    {
        W2BYTE(REG_SCL0_2C_L, u16Vs_Width);
    }
    else
    {
    }
}


void Hal_SCLDAM_SetRegenVSyncStartPoint(EN_SCLDMA_VS_ID_TYPE enID, MS_U16 u16Vs_St)
{

    if(enID == E_SCLDMA_VS_ID_SC)
    {
    }
    else if(enID == E_SCLDMA_VS_ID_AFF)
    {
        W2BYTE(REG_SCL0_27_L, u16Vs_St);
    }
    else if(enID == E_SCLDMA_VS_ID_LDC)
    {
        W2BYTE(REG_SCL0_21_L, u16Vs_St);
    }
    else if(enID == E_SCLDMA_VS_ID_SC3)
    {
        W2BYTE(REG_SCL0_23_L, u16Vs_St);
    }
    else if(enID == E_SCLDMA_VS_ID_DISP)
    {
        W2BYTE(REG_SCL0_25_L, u16Vs_St);
    }
    else
    {
    }
}

void Hal_SCLDMA_TrigRegenVSync(EN_SCLDMA_VS_ID_TYPE enID, MS_BOOL bEn)
{
    if(enID == E_SCLDMA_VS_ID_SC)
    {
    }
    else if(enID == E_SCLDMA_VS_ID_AFF)
    {
        W2BYTEMSK(REG_SCL0_26_L, bEn ? BIT10: 0, BIT10);
    }
    else if(enID == E_SCLDMA_VS_ID_LDC)
    {
        W2BYTEMSK(REG_SCL0_20_L, bEn ? BIT2: 0, BIT2);
    }
    else if(enID == E_SCLDMA_VS_ID_SC3)
    {
        W2BYTEMSK(REG_SCL0_22_L, bEn ? BIT2: 0, BIT2);
    }
    else if(enID == E_SCLDMA_VS_ID_DISP)
    {
        W2BYTEMSK(REG_SCL0_24_L, bEn ? BIT2: 0, BIT2);
    }
    else
    {
    }
}

void Hal_SCLDMA_SetDMAEnableDoubleBuffer(MS_BOOL bEn,EN_SCLDMA_ID_TYPE enSCLDMA_ID)
{
    switch(enSCLDMA_ID)
    {
    case E_SCLDMA_ID_1_W:
    case E_SCLDMA_ID_2_W:
        W2BYTEMSK(REG_SCL_DMA2_50_L, bEn ? BIT4 : 0, BIT4);
        break;

    case E_SCLDMA_ID_3_W:
    case E_SCLDMA_ID_3_R:
        W2BYTEMSK(REG_SCL_DMA2_50_L, bEn ? BIT5 : 0, BIT5);
        break;

    case E_SCLDMA_ID_PNL_R:
        W2BYTEMSK(REG_SCL_DMA2_50_L, bEn ? BIT6 : 0, BIT6);
        break;

    default:
        W2BYTEMSK(REG_SCL_DMA2_50_L, bEn ? BIT7 : 0, BIT7);
        break;

    }
}
void Hal_SCLDMA_SetHandshakeDoubleBuffer(MS_BOOL bEn)
{
    W2BYTEMSK(REG_SCL_DMA2_50_L, bEn ? BIT0 : 0, BIT0);

}
MS_U8 Hal_SCLDMA_GetISPFrameCountReg(void)
{
    MS_U8 u8Value = (MS_U8)((R2BYTE(REG_ISP_7A_L)>>8)&0x7F);
    //u8Value = (u8Value==0) ? 0x7F : (u8Value - 1);
    return u8Value;
}
MS_U16 Hal_SCLDMA_GetDMAOutputCount(EN_SCLDMA_CLIENT_TYPE enClientType)
{
    MS_U16 u16Idx,u16def;

    switch(enClientType)
        {
        case E_SCLDMA_1_FRM_W:
            u16Idx = R2BYTE(REG_SCL_DMA2_10_L);
            u16def = R2BYTE(REG_SCL_DMA0_1B_L);
            break;

        case E_SCLDMA_1_IMI_W:
            u16Idx = R2BYTE(REG_SCL_DMA2_11_L);
            u16def = R2BYTE(REG_SCL_DMA1_4B_L);
            break;

        case E_SCLDMA_1_SNP_W:
            u16Idx = R2BYTE(REG_SCL_DMA2_12_L);
            u16def = R2BYTE(REG_SCL_DMA0_4B_L);
            break;

        case E_SCLDMA_2_FRM_W:
            u16Idx = R2BYTE(REG_SCL_DMA2_13_L);
            u16def = R2BYTE(REG_SCL_DMA0_63_L);
            break;

        case E_SCLDMA_2_IMI_W:
            u16Idx = R2BYTE(REG_SCL_DMA2_14_L);
            u16def = R2BYTE(REG_SCL_DMA0_7B_L);
            break;

        case E_SCLDMA_3_FRM_R:
            u16Idx = R2BYTE(REG_SCL_DMA2_16_L);
            u16def = R2BYTE(REG_SCL_DMA1_33_L);
            break;

        case E_SCLDMA_3_FRM_W:
            u16Idx = R2BYTE(REG_SCL_DMA2_15_L);
            u16def = R2BYTE(REG_SCL_DMA1_1B_L);
            break;

        case E_SCLDMA_4_FRM_R:
            u16Idx = R2BYTE(REG_SCL_DMA2_17_L);
            u16def = R2BYTE(REG_SCL_DMA0_33_L);
            break;
        default:
            u16Idx = 0x0;
            u16def = 0x0;
            break;
        }
    return ((u16Idx));

}
MS_U16 Hal_SCLDMA_GetOutputHsize(EN_SCLDMA_CLIENT_TYPE enClientType)
{
    MS_U16 u16Idx,u16def;

    switch(enClientType)
        {
        case E_SCLDMA_1_FRM_W:
            u16def = R2BYTE(REG_SCL_DMA0_1A_L);
            break;

        case E_SCLDMA_1_IMI_W:
            u16def = R2BYTE(REG_SCL_DMA1_4A_L);
            break;

        case E_SCLDMA_1_SNP_W:
            u16def = R2BYTE(REG_SCL_DMA0_4A_L);
            break;

        case E_SCLDMA_2_FRM_W:
            u16def = R2BYTE(REG_SCL_DMA0_62_L);
            break;

        case E_SCLDMA_2_IMI_W:
            u16def = R2BYTE(REG_SCL_DMA0_7A_L);
            break;

        case E_SCLDMA_3_FRM_R:
            u16def = R2BYTE(REG_SCL_DMA1_32_L);
            break;

        case E_SCLDMA_3_FRM_W:
            u16def = R2BYTE(REG_SCL_DMA1_1A_L);
            break;

        case E_SCLDMA_4_FRM_R:
            u16def = R2BYTE(REG_SCL_DMA0_32_L);
            break;
        default:
            u16Idx = 0x0;
            u16def = 0x0;
            break;
        }
    return ((u16def));

}
MS_U16 Hal_SCLDMA_GetOutputVsize(EN_SCLDMA_CLIENT_TYPE enClientType)
{
    MS_U16 u16Idx,u16def;

    switch(enClientType)
        {
        case E_SCLDMA_1_FRM_W:
            u16def = R2BYTE(REG_SCL_DMA0_1B_L);
            break;

        case E_SCLDMA_1_IMI_W:
            u16def = R2BYTE(REG_SCL_DMA1_4B_L);
            break;

        case E_SCLDMA_1_SNP_W:
            u16def = R2BYTE(REG_SCL_DMA0_4B_L);
            break;

        case E_SCLDMA_2_FRM_W:
            u16def = R2BYTE(REG_SCL_DMA0_63_L);
            break;

        case E_SCLDMA_2_IMI_W:
            u16def = R2BYTE(REG_SCL_DMA0_7B_L);
            break;

        case E_SCLDMA_3_FRM_R:
            u16def = R2BYTE(REG_SCL_DMA1_33_L);
            break;

        case E_SCLDMA_3_FRM_W:
            u16def = R2BYTE(REG_SCL_DMA1_1B_L);
            break;

        case E_SCLDMA_4_FRM_R:
            u16def = R2BYTE(REG_SCL_DMA0_33_L);
            break;
        default:
            u16Idx = 0x0;
            u16def = 0x0;
            break;
        }
    return ((u16def));

}
MS_U16 Hal_SCLDMA_Get_RW_Idx(EN_SCLDMA_CLIENT_TYPE enClient)
{
    MS_U16 u16Idx;
    switch(enClient)
    {
    case E_SCLDMA_1_FRM_W:
        u16Idx = R2BYTEMSK(REG_SCL_DMA2_03_L, (BIT1|BIT0));
        break;

    case E_SCLDMA_1_IMI_W:
        u16Idx = (R2BYTEMSK(REG_SCL_DMA2_03_L, (BIT3|BIT2)))>>2;
        break;

    case E_SCLDMA_1_SNP_W:
        u16Idx = (R2BYTEMSK(REG_SCL_DMA2_03_L, (BIT5|BIT4)))>>4;
        break;

    case E_SCLDMA_2_FRM_W:
        u16Idx = (R2BYTEMSK(REG_SCL_DMA2_03_L, (BIT7|BIT6)))>>6;
        break;

    case E_SCLDMA_2_IMI_W:
        u16Idx = (R2BYTEMSK(REG_SCL_DMA2_03_L, (BIT9|BIT8)))>>8;
        break;

    case E_SCLDMA_3_FRM_R:
        u16Idx = (R2BYTEMSK(REG_SCL_DMA2_03_L, (BIT11|BIT10)))>>10;
        break;

    case E_SCLDMA_3_FRM_W:
        u16Idx = (R2BYTEMSK(REG_SCL_DMA2_03_L, (BIT13|BIT12)))>>12;
        break;

    case E_SCLDMA_4_FRM_R:
        u16Idx = (R2BYTEMSK(REG_SCL_DMA2_03_L, (BIT15|BIT14)))>>14;
        break;
    default:
        u16Idx = 0xFF;
        break;
    }
    return u16Idx;
}

#undef HAL_SCLDMA_C
