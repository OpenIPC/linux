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
#define HAL_HVSP_C

#ifdef MSOS_TYPE_LINUX_KERNEL
//#include <linux/wait.h>
//#include <linux/irqreturn.h>
//#include <asm/div64.h>
#endif

//-------------------------------------------------------------------------------------------------
//  Include Files
//-------------------------------------------------------------------------------------------------
// Common Definition
#include "MsCommon.h"
#include "MsTypes.h"
#include "MsOS.h"
// Internal Definition
#include "hwreg.h"
#include "halhvsp_utility.h"
#include "drvhvsp_st.h"
#include "drvsclirq_st.h"
#include "drvsclirq.h"
#include "halhvsp.h"
#include "drvCMDQ.h"
#include "MsDbg.h"
#include "mdrv_scl_dbg.h"
#include "drvPQ_Define.h"
#include "Infinity3e_Main.h"             // table config parameter
//-------------------------------------------------------------------------------------------------
//  Driver Compiler Options
//-------------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------
//  Defines & Macro
//-------------------------------------------------------------------------------------------------
#define HVSP_CMD_TRIG_BUFFER_SIZE 100
#define Is_InputSource(Src)             (genIpType == (Src))
#define Is_CLK_Increase(height,width,rate)    (((height) > 720 || (width) > 1280)&& (rate) < 172000000)
#define Is_CLK_Decrease(height,width,rate)    (((height) <= 720 && (width) <= 1280)&& (rate) >= 172000000)
#define Is_Reg_Type(type)               (gstHvspCmdTrigCfg.enType == (type))
#define Is_IP_First_Set(ip)             (gbhvspset[(ip)]==0)
#define HAL_HVSP_RATIO(input, output)           ((MS_U32)((MS_U64)((input) * 1048576) / (output)))
#define DISABLE_CLK 0x1
#define LOW_CLK 0x4
#define LOCK_FB()                       (gblockfbmg)
#define _SetHVSPType(ID,u8type)                    (gstSclFea[(ID)].u16ModeYCVH |= (u8type))
#define _ReSetHVSPType(ID,u8type)                  (gstSclFea[(ID)].u16ModeYCVH &= ~(u8type))
#define Is_CannotUseCMDQToSetReg()      (gbUseCMDQ == 0)

typedef struct
{
    MS_U32 u32Reg;
    MS_U16 u16Msk;
    MS_U16 u16Val;
}ST_HVSP_CMD_TRIG_BUFFER_TYPE;

typedef struct
{
    ST_HVSP_CMD_TRIG_BUFFER_TYPE stCmdTrigBuf[HVSP_CMD_TRIG_BUFFER_SIZE];
    MS_U16 u16Idx;
}ST_HVSP_CMD_TRIG_BUFFER_CONFIG;

typedef struct
{
    MS_U16 u16ModeYCVH;
    MS_U16 u16DithCoring;
}ST_HVSP_SCALING_FEATURE_CONFIG;
//-------------------------------------------------------------------------------------------------
//  Variable
//-------------------------------------------------------------------------------------------------
MS_U32 HVSP_RIU_BASE;
/////////////////
/// gstHvspCmdTrigCfg
/// To save set Register type and used in scaling function.
////////////////
ST_HVSP_CMD_TRIG_CONFIG gstHvspCmdTrigCfg;
/////////////////
/// gstHvspCmdTrigBufCfg
/// To save CMD and used in scaling function.
////////////////
ST_HVSP_CMD_TRIG_BUFFER_CONFIG gstHvspCmdTrigBufCfg;
/////////////////
/// genIpType
/// To save Input Source Type.
////////////////
EN_HVSP_IP_MUX_TYPE genIpType;
/////////////////
/// gu16height
/// To save resolution and used in dynamic change clk.
////////////////
MS_U16 gu16height[2]={0,0};
/////////////////
/// gu16height
/// To save resolution and used in dynamic change clk.
////////////////
MS_U16 gu16width[2]={0,0};
/////////////////
/// genFilterMode
/// To save scaling type for handle CMDQ can't mask issue.
////////////////
EN_HVSP_FILTER_MODE genFilterMode;
/////////////////
/// gbhvspset
/// The first time set HVSP scaling config can't use CMDQ(no sync to trig),so need to save this information.
////////////////
MS_BOOL gbhvspset[E_HVSP_ID_MAX]={0,0,0};
ST_HVSP_SCALING_FEATURE_CONFIG gstSclFea[E_HVSP_ID_MAX];
MS_BOOL gbUseCMDQ;
MS_BOOL gbResetDNR;
MS_BOOL gblockfbmg = 0;
ST_HVSP_SIZE_CONFIG gstHalSrcSize;
MS_BOOL gbSramCMDQ = 0;

//-------------------------------------------------------------------------------------------------
//  Functions
//-------------------------------------------------------------------------------------------------
void Hal_HVSP_Exit(void)
{
    gblockfbmg = 0;
    genIpType = E_HVSP_IP_MUX_ISP;
    genFilterMode = E_HVSP_FILTER_MODE_BYPASS;
    gu16width[0]=0;
    gu16height[0]=0;
    gu16width[1]=0;
    gu16height[1]=0;
    MsOS_Memset(&gstHvspCmdTrigCfg, 0,sizeof(ST_HVSP_CMD_TRIG_CONFIG));
    MsOS_Memset(&gstHvspCmdTrigBufCfg, 0,sizeof(ST_HVSP_CMD_TRIG_BUFFER_CONFIG));
}
//============CLK================================
#if CONFIG_OF
void Hal_HVSP_SetIdclkOnOff(MS_BOOL bEn,ST_HVSP_CLK_CONFIG* stclk)
{
    struct clk* pstclock = NULL;
    MS_U32 u32Events;
    if(Drv_SCLIRQ_GetEachDMAEn()||(!Drv_SCLIRQ_GetIsBlankingRegion()))
    {
        SCL_ERR("[IDCLK]%s %d,Wait\n", __FUNCTION__, __LINE__);
        MsOS_WaitEvent(Drv_SCLIRQ_Get_IRQ_SYNCEventID(),
            (E_SCLIRQ_EVENT_ISPFRMEND|E_SCLIRQ_EVENT_FRMENDSYNC), &u32Events, E_AND, 200); // get status: FRM END
    }
    if(bEn)
    {
        if(Is_InputSource(E_HVSP_IP_MUX_PAT_TGEN))
        {
            W2BYTEMSK(REG_SCL_CLK_61_L,0x0000,0x0F00);//h61
        }

        if(Is_InputSource(E_HVSP_IP_MUX_BT656))
        {
            W2BYTEMSK(REG_BLOCK_24_L,0x0004,0x0005);//idclk
            W2BYTEMSK(REG_CHIPTOP_0F_L,0x0010,0x0010);//ccir mode
            if (MsOS_clk_get_enable_count(stclk->idclk)==0)
            {
                if (NULL != (pstclock = MsOS_clk_get_parent_by_index(stclk->idclk, 1)))
                {
                    MsOS_clk_set_parent(stclk->idclk, pstclock);
                    MsOS_clk_prepare_enable(stclk->idclk);
                    //printf("[idclk]enable count=%d\n", MsOS_clk_get_enable_count(stclk->idclk));
                }
                else
                {
                    printf("[idclk]BT656 NULL\n");

                }
            }
        }
        else
        {
            W2BYTEMSK(REG_CHIPTOP_0F_L,0x0000,0x0010);//ccir mode
            W2BYTEMSK(REG_BLOCK_24_L,0x0000,0x0005);//idclk
            if (MsOS_clk_get_enable_count(stclk->idclk)==0)
            {
                if (NULL != (pstclock = MsOS_clk_get_parent_by_index(stclk->idclk, 0)))
                {
                    MsOS_clk_set_parent(stclk->idclk, pstclock);
                    MsOS_clk_prepare_enable(stclk->idclk);
                }
                else
                {
                    SCL_ERR("[HAlHVSP]CLK Tree Not Ready \n");
                }
            }
        }
    }
    else
    {
        if(Is_InputSource(E_HVSP_IP_MUX_PAT_TGEN))
        {
            W2BYTEMSK(REG_SCL_CLK_61_L,0x0100,0x0F00);//h61
        }

        if(Is_InputSource(E_HVSP_IP_MUX_BT656))
        {
            W2BYTEMSK(REG_BLOCK_24_L,0x0005,0x0005);//idclk
            W2BYTEMSK(REG_CHIPTOP_0F_L,0x0000,0x0010);//ccir mode
            if (NULL != (pstclock = MsOS_clk_get_parent_by_index(stclk->idclk, 0)))
            {
                MsOS_clk_set_parent(stclk->idclk, pstclock);
            }
            else
            {
                SCL_ERR("[HAlHVSP]CLK Tree Not Ready \n");
            }
            while (MsOS_clk_get_enable_count(stclk->idclk))
            {
                MsOS_clk_disable_unprepare(stclk->idclk);
            }
        }
        else
        {
            W2BYTEMSK(REG_BLOCK_24_L,0x0001,0x0005);//idclk
            W2BYTEMSK(REG_CHIPTOP_0F_L,0x0000,0x0010);//ccir mode
            if (NULL != (pstclock = MsOS_clk_get_parent_by_index(stclk->idclk, 0)))
            {
                MsOS_clk_set_parent(stclk->idclk, pstclock);
            }
            else
            {
                SCL_ERR("[HAlHVSP]CLK Tree Not Ready \n");
            }
            while (MsOS_clk_get_enable_count(stclk->idclk))
            {
                MsOS_clk_disable_unprepare(stclk->idclk);
            }
        }

    }
}
void Hal_HVSP_SetCLKRate(unsigned char u8Idx)
{
    if((u8Idx &EN_HVSP_CLKATTR_BT656))//BT656
    {
        W2BYTEMSK(REG_BLOCK_24_L,0x0004,0x0005);//idclk
        W2BYTEMSK(REG_CHIPTOP_0F_L,0x0010,0x0010);//ccir mode
    }
    else if((u8Idx &EN_HVSP_CLKATTR_ISP))//ISP
    {
        printf("[hal]ISP xxxxxxxxx\n");
        W2BYTEMSK(REG_CHIPTOP_0F_L,0x0000,0x0010);//ccir mode
        W2BYTEMSK(REG_BLOCK_24_L,0x0000,0x0005);//idclk
    }
    if((u8Idx &EN_HVSP_CLKATTR_FORCEMODE))
    {
        printf("[hal]FORCEMODE xxxxxxxxx\n");
        W2BYTEMSK(REG_SCL_CLK_61_L,0x0000,0x0F00);//h61
    }
}

void Hal_HVSP_FCLK1(ST_HVSP_CLK_CONFIG *stclk)
{
#if CLKDynamic
    struct clk* pstclock = NULL;
    if (MsOS_clk_get_enable_count(stclk->fclk1)==0)
    {
    }
    else
    {
        if(Is_CLK_Increase(gu16height[0],gu16width[0],MsOS_clk_get_rate(stclk->fclk1)))
        {
            if (NULL != (pstclock = MsOS_clk_get_parent_by_index(stclk->fclk1, 0)))
            {
                MsOS_clk_set_parent(stclk->fclk1, pstclock);
            }
            else
            {
                SCL_ERR("[HAlHVSP]CLK Tree Not Ready \n");
            }
    #if OSDinverseBug
            if (NULL != (pstclock = MsOS_clk_get_parent_by_index(stclk->fclk2, 0)))
            {
                MsOS_clk_set_parent(stclk->fclk2, pstclock);
            }
            else
            {
                SCL_ERR("[HAlHVSP]CLK Tree Not Ready \n");
            }
    #endif
        }
        else if(Is_CLK_Decrease(gu16height[0],gu16width[0],MsOS_clk_get_rate(stclk->fclk1)))
        {
            if (NULL != (pstclock = MsOS_clk_get_parent_by_index(stclk->fclk1, 1)))
            {
                MsOS_clk_set_parent(stclk->fclk1, pstclock);
            }
            else
            {
                SCL_ERR("[HAlHVSP]CLK Tree Not Ready \n");
            }
    #if OSDinverseBug
            if (NULL != (pstclock = MsOS_clk_get_parent_by_index(stclk->fclk2, 1)))
            {
                MsOS_clk_set_parent(stclk->fclk2, pstclock);
            }
            else
            {
                SCL_ERR("[HAlHVSP]CLK Tree Not Ready \n");
            }
    #endif
        }
    }
#endif
}

void Hal_HVSP_FCLK2(ST_HVSP_CLK_CONFIG *stclk)
{
    #if OSDinverseBug
    #else
    struct clk* pstclock = NULL;
    if (MsOS_clk_get_enable_count(stclk->fclk2)==0)
    {
    }
    else
    {
        if(Is_CLK_Increase(gu16height[1],gu16width[1],MsOS_clk_get_rate(stclk->fclk2)))
        {
            if (NULL != (pstclock = MsOS_clk_get_parent_by_index(stclk->fclk2, 0)))
            {
                MsOS_clk_set_parent(stclk->fclk2, pstclock);
            }
            else
            {
                SCL_ERR("[HAlHVSP]CLK Tree Not Ready \n");
            }
            //printf("[HVSP]CLK UP\n");
        }
        else if(Is_CLK_Decrease(gu16height[1],gu16width[1],MsOS_clk_get_rate(stclk->fclk2)))
        {
            if (NULL != (pstclock = MsOS_clk_get_parent_by_index(stclk->fclk2, 1)))
            {
                MsOS_clk_set_parent(stclk->fclk2, pstclock);
            }
            else
            {
                SCL_ERR("[HAlHVSP]CLK Tree Not Ready \n");
            }
            //printf("[HVSP]CLK down\n");
        }
    }
#endif
}

#else
void Hal_HVSP_SetIdclkOnOff(MS_BOOL bEn,ST_HVSP_CLK_CONFIG* stclk)
{
    printf("[hal]NO OF\n");
    if(bEn)
    {
        if(Is_InputSource(E_HVSP_IP_MUX_PAT_TGEN))
        {
            W2BYTEMSK(REG_SCL_CLK_61_L,0x0000,0x0F00);//h61
        }

        if(Is_InputSource(E_HVSP_IP_MUX_BT656))
        {
            W2BYTEMSK(REG_BLOCK_24_L,0x0004,0x0005);//idclk
            W2BYTEMSK(REG_CHIPTOP_0F_L,0x0010,0x0010);//ccir mode
            if ((R2BYTE(REG_SCL_CLK_63_L)&DISABLE_CLK))
            {
                W2BYTEMSK(REG_SCL_CLK_63_L,0x0004,0x000F);//h63
            }
        }
        else
        {
            W2BYTEMSK(REG_CHIPTOP_0F_L,0x0000,0x0010);//ccir mode
            W2BYTEMSK(REG_BLOCK_24_L,0x0000,0x0005);//idclk
            if ((R2BYTE(REG_SCL_CLK_63_L)&DISABLE_CLK))
            {
                W2BYTEMSK(REG_SCL_CLK_63_L,0x0000,0x000F);//h63
            }
        }

    }
    else
    {
        if(Is_InputSource(E_HVSP_IP_MUX_PAT_TGEN))
        {
            W2BYTEMSK(REG_SCL_CLK_61_L,0x0100,0x0F00);//h61
        }

        if(Is_InputSource(E_HVSP_IP_MUX_BT656))
        {
            W2BYTEMSK(REG_BLOCK_24_L,0x0005,0x0005);//idclk
            W2BYTEMSK(REG_CHIPTOP_0F_L,0x0000,0x0010);//ccir mode
            W2BYTEMSK(REG_SCL_CLK_63_L,0x0005,0x000F);//h63
        }
        else
        {
            W2BYTEMSK(REG_BLOCK_24_L,0x0001,0x0005);//idclk
            W2BYTEMSK(REG_CHIPTOP_0F_L,0x0000,0x0010);//ccir mode
            W2BYTEMSK(REG_SCL_CLK_63_L,0x0001,0x000F);//h63
        }

    }
}
void Hal_HVSP_SetCLKRate(unsigned char u8Idx)
{
    if((u8Idx &EN_HVSP_CLKATTR_BT656))//BT656
    {
        W2BYTEMSK(REG_BLOCK_24_L,0x0004,0x0005);//idclk
        W2BYTEMSK(REG_CHIPTOP_0F_L,0x0010,0x0010);//ccir mode
    }
    else if((u8Idx &EN_HVSP_CLKATTR_ISP))//ISP
    {
        printf("[hal]ISP xxxxxxxxx\n");
        W2BYTEMSK(REG_CHIPTOP_0F_L,0x0000,0x0010);//ccir mode
        W2BYTEMSK(REG_BLOCK_24_L,0x0000,0x0005);//idclk
    }
    if((u8Idx &EN_HVSP_CLKATTR_FORCEMODE))
    {
        printf("[hal]FORCEMODE xxxxxxxxx\n");
        W2BYTEMSK(REG_SCL_CLK_61_L,0x0000,0x0F00);//h61
    }
}

void Hal_HVSP_FCLK1(ST_HVSP_CLK_CONFIG *stclk)
{
    if ((R2BYTE(REG_SCL_CLK_64_L)&DISABLE_CLK))
    {
    }
    else
    {
        if((gu16height[0]>720||gu16width[0]>1280) && (R2BYTE(REG_SCL_CLK_64_L)&LOW_CLK))
        {
            W2BYTEMSK(REG_SCL_CLK_64_L,0x0000,0x000F);//h61
        }
        else if((gu16height[0]<=720 && ,gu16width[0]<=1280)&& !(R2BYTE(REG_SCL_CLK_64_L)&LOW_CLK))
        {
            W2BYTEMSK(REG_SCL_CLK_64_L,LOW_CLK,0x000F);//h61
        }
    }

}

void Hal_HVSP_FCLK2(ST_HVSP_CLK_CONFIG *stclk)
{
    if ((R2BYTE(REG_SCL_CLK_65_L)&DISABLE_CLK))
    {
    }
    else
    {
        if((gu16height[1]>720||gu16width[1]>1280) && (R2BYTE(REG_SCL_CLK_65_L)&LOW_CLK))
        {
            W2BYTEMSK(REG_SCL_CLK_65_L,0x0000,0x000F);//h61
        }
        else if((gu16height[1]<=720 && ,gu16width[1]<=1280) && !(R2BYTE(REG_SCL_CLK_64_L)&LOW_CLK))
        {
            W2BYTEMSK(REG_SCL_CLK_65_L,LOW_CLK,0x000F);//h61
        }
    }
}


#endif

//===========================================================
void Hal_HVSP_SetFrameBufferManageLock(MS_BOOL bEn)
{
    gblockfbmg = bEn;
}
void Hal_HVSP_SetCMDQTrigCfg(ST_HVSP_CMD_TRIG_CONFIG *stCmdTrigCfg)
{
    gbUseCMDQ = Drv_SCLIRQ_GetEachDMAEn();
    gstHvspCmdTrigCfg.enType = stCmdTrigCfg->enType;
    gstHvspCmdTrigCfg.u8Fmcnt = stCmdTrigCfg->u8Fmcnt;
}
void Hal_HVSP_SetResetDNR(MS_BOOL bEn)
{
    gbResetDNR = bEn;
}
void Hal_HVSP_GetCMDQTrigCfg(ST_HVSP_CMD_TRIG_CONFIG *pCfg)
{
    MsOS_Memcpy(pCfg, &gstHvspCmdTrigCfg, sizeof(ST_HVSP_CMD_TRIG_CONFIG));
}

void Hal_HVSP_SetCMDQTrigFrameCnt(MS_U16 u16Idx)
{
    gstHvspCmdTrigBufCfg.u16Idx = u16Idx;
}

void Hal_HVSP_SetCMDQTrigFire(void)
{
    MS_U16 i;
    MS_CMDQ_CMDReg stCfg;
#if SCALING_BLOCK
    MS_U32 u32Events = 0;
#endif
    MsOS_Memset(&stCfg,0,sizeof(MS_CMDQ_CMDReg));
    //printf("[HVSP]Hal_HVSP_SetCMDQTrigFire   gstHvspCmdTrigCfg.enType:%d \n",gstHvspCmdTrigCfg.enType);
    if(Is_Reg_Type(E_HVSP_CMD_TRIG_CMDQ_LDC_SYNC))
    {
        Drv_CMDQ_GetModuleMutex(EN_CMDQ_TYPE_IP0,1);
        for(i=0; i< gstHvspCmdTrigBufCfg.u16Idx; i++)
        {
            Drv_CMDQ_FillCmd(&stCfg, gstHvspCmdTrigBufCfg.stCmdTrigBuf[i].u32Reg,
                gstHvspCmdTrigBufCfg.stCmdTrigBuf[i].u16Val, gstHvspCmdTrigBufCfg.stCmdTrigBuf[i].u16Msk);
            Drv_CMDQ_WriteCmd(EN_CMDQ_TYPE_IP0,&stCfg,0);
        }
        if(gbResetDNR)
        {
            MsOS_SetEvent_IRQ(Drv_SCLIRQ_Get_IRQ_SYNCEventID(), E_SCLIRQ_EVENT_BRESETDNR);
            gbResetDNR = 0;
        }
        MsOS_ClearEventIRQ(Drv_SCLIRQ_Get_IRQ_SYNCEventID(),(E_SCLIRQ_EVENT_CHANGESIZEDONE));
        MsOS_SetEvent_IRQ(Drv_SCLIRQ_Get_IRQ_SYNCEventID(), E_SCLIRQ_EVENT_SETCHANGESIZE);
        Drv_CMDQ_Fire(EN_CMDQ_TYPE_IP0,TRUE);
        Drv_CMDQ_GetModuleMutex(EN_CMDQ_TYPE_IP0,0);
        #if SCALING_BLOCK
        if(gstHvspCmdTrigBufCfg.u16Idx &&Drv_SCLIRQ_GetEachDMAEn())
        {
            MsOS_WaitEvent(Drv_SCLIRQ_Get_IRQ_SYNCEventID(), E_SCLIRQ_EVENT_CHANGESIZEDONE, &u32Events, E_OR, MSOS_WAIT_FOREVER);
            MsOS_ClearEventIRQ(Drv_SCLIRQ_Get_IRQ_SYNCEventID(),(E_SCLIRQ_EVENT_CHANGESIZEDONE));
        }
        #endif
    }
    else if(Is_Reg_Type(E_HVSP_CMD_TRIG_POLL_LDC_SYNC))
    {
        for(i=0; i< gstHvspCmdTrigBufCfg.u16Idx; i++)
        {
            W2BYTEMSK(gstHvspCmdTrigBufCfg.stCmdTrigBuf[i].u32Reg,
                      gstHvspCmdTrigBufCfg.stCmdTrigBuf[i].u16Val,
                      gstHvspCmdTrigBufCfg.stCmdTrigBuf[i].u16Msk);
        }
    }

}


MS_BOOL _Hal_HVSP_Write_Cmd(MS_U32 u32Reg,  MS_U16 u16Val, MS_U16 u16Msk)
{
    MS_U16 u16idx = gstHvspCmdTrigBufCfg.u16Idx;
    gstHvspCmdTrigBufCfg.stCmdTrigBuf[u16idx].u32Reg = u32Reg;
    gstHvspCmdTrigBufCfg.stCmdTrigBuf[u16idx].u16Msk = u16Msk;
    gstHvspCmdTrigBufCfg.stCmdTrigBuf[u16idx].u16Val = u16Val;
    //printf("[HVSP]_Hal_HVSP_Write_Cmd   u32Reg:%lx u16Msk:%hx u16Val:%hx \n",u32Reg,u16Msk,u16Val);
    gstHvspCmdTrigBufCfg.u16Idx++;
    if(gstHvspCmdTrigBufCfg.u16Idx >= HVSP_CMD_TRIG_BUFFER_SIZE)
    {
        gstHvspCmdTrigBufCfg.u16Idx = HVSP_CMD_TRIG_BUFFER_SIZE-1;
        SCL_ERR("[HVSP]CMDQ FULL\n");
        return FALSE;
    }
    else
    {
        return TRUE;
    }
}

void Hal_HVSP_SetRiuBase(MS_U32 u32RiuBase)
{
    HVSP_RIU_BASE = u32RiuBase;
}
void Hal_HVSP_SetInputSrcSize(ST_HVSP_SIZE_CONFIG *stSrc)
{
    MsOS_Memcpy(&gstHalSrcSize,stSrc,sizeof(ST_HVSP_SIZE_CONFIG) );
}

void Hal_HVSP_Set_Reset(void)
{
    gbhvspset[0]= 0;
    gbhvspset[1]= 0;
    gbhvspset[2]= 0;
    gbUseCMDQ = 0;
    gstSclFea[0].u16ModeYCVH= 0;
    gstSclFea[1].u16ModeYCVH= 0;
    gstSclFea[2].u16ModeYCVH= 0;
    gstSclFea[0].u16DithCoring= 0;
    gstSclFea[1].u16DithCoring= 0;
    gstSclFea[2].u16DithCoring= 0;
    Hal_HVSP_SetNLMEn(0);
    //Hal_HVSP_SetHWInputMux(E_HVSP_IP_MUX_BT656);
    W2BYTEMSK(REG_SCL2_50_L, 0, BIT0);//crop1
    W2BYTEMSK(REG_SCL2_58_L, 0, BIT0);//crop2
    Hal_HVSP_SetIPMYCMReadEn(0);
    Hal_HVSP_SetIPMYCMWriteEn(0);
    Hal_HVSP_SetIPMCIIRReadEn(0);
    Hal_HVSP_SetIPMCIIRWriteEn(0);
    Hal_HVSP_SetLDCPathSel(0);
    Hal_HVSP_SetLDCBypass(1);
    Hal_HVSP_SetPrv2CropOnOff(0);
    Hal_HVSP_PriMask_bEn(EN_HVSP_TRIGGER_OFF);
    Hal_HVSP_Set_DNR_Reset(1);
    Hal_HVSP_Set_SW_Reset(1);
}
void Hal_HVSP_Set_CLKOFF(void)
{
    W2BYTEMSK(REG_SCL_CLK_64_L,0x0001,0x000F);// ToDo
}
void Hal_HVSP_SetVIPSize(MS_U16 u16Width, MS_U16 u16Height)
{
    if(Is_Reg_Type(E_HVSP_CMD_TRIG_NONE) || Is_IP_First_Set(E_HVSP_ID_1) || Is_CannotUseCMDQToSetReg())
    {
        W2BYTEMSK(REG_VIP_LCE_6E_L, u16Width, 0x0FFF);
        W2BYTEMSK(REG_VIP_LCE_6F_L, u16Height, 0x1FFF);
    }
    else
    {
        _Hal_HVSP_Write_Cmd(REG_VIP_LCE_6E_L, u16Width, 0x0FFF);
        _Hal_HVSP_Write_Cmd(REG_VIP_LCE_6F_L, u16Height, 0x1FFF);
    }
}
void Hal_HVSP_SetWDRGlobalSize(MS_U16 u16Width, MS_U16 u16Height)
{
    if(Is_Reg_Type(E_HVSP_CMD_TRIG_NONE) || Is_IP_First_Set(E_HVSP_ID_1) || Is_CannotUseCMDQToSetReg())
    {
        W2BYTEMSK(REG_VIP_WDR_35_L, u16Width-1, 0x0FFF);
        W2BYTEMSK(REG_VIP_WDR_36_L, u16Height-1, 0x1FFF);
    }
    else
    {
        _Hal_HVSP_Write_Cmd(REG_VIP_WDR_35_L, u16Width-1, 0x0FFF);
        _Hal_HVSP_Write_Cmd(REG_VIP_WDR_36_L, u16Height-1, 0x1FFF);
    }
}
void Hal_HVSP_SetWDRLocalSize(MS_U16 u16Width, MS_U16 u16Height)
{
    if(Is_Reg_Type(E_HVSP_CMD_TRIG_NONE) || Is_IP_First_Set(E_HVSP_ID_1) || Is_CannotUseCMDQToSetReg())
    {
        W2BYTEMSK(REG_VIP_WDR_7C_L, u16Width-1, 0x0FFF);
        W2BYTEMSK(REG_VIP_WDR_7D_L, u16Height-1, 0x1FFF);
    }
    else
    {
        _Hal_HVSP_Write_Cmd(REG_VIP_WDR_7C_L, u16Width-1, 0x0FFF);
        _Hal_HVSP_Write_Cmd(REG_VIP_WDR_7D_L, u16Height-1, 0x1FFF);
    }
}
void Hal_HVSP_SetMXNRSize(MS_U16 u16Width, MS_U16 u16Height)
{
    if(Is_Reg_Type(E_HVSP_CMD_TRIG_NONE) || Is_IP_First_Set(E_HVSP_ID_1) || Is_CannotUseCMDQToSetReg())
    {
        W2BYTEMSK(REG_VIP_SCNR_0A_L, u16Width -1, 0x0FFF);
        W2BYTEMSK(REG_VIP_SCNR_0B_L, u16Height -1, 0x1FFF);
    }
    else
    {
        _Hal_HVSP_Write_Cmd(REG_VIP_SCNR_0A_L, u16Width -1, 0x0FFF);
        _Hal_HVSP_Write_Cmd(REG_VIP_SCNR_0B_L, u16Height -1, 0x1FFF);
    }
}
void Hal_HVSP_SetUVadjSize(MS_U16 u16Width, MS_U16 u16Height)
{
    if(Is_Reg_Type(E_HVSP_CMD_TRIG_NONE) || Is_IP_First_Set(E_HVSP_ID_1) || Is_CannotUseCMDQToSetReg())
    {
        W2BYTEMSK(REG_VIP_SCNR_3B_L, u16Width-1, 0x0FFF);
        W2BYTEMSK(REG_VIP_SCNR_3C_L, u16Height-1, 0x1FFF);
    }
    else
    {
        _Hal_HVSP_Write_Cmd(REG_VIP_SCNR_3B_L, u16Width-1, 0x0FFF);
        _Hal_HVSP_Write_Cmd(REG_VIP_SCNR_3C_L, u16Height-1, 0x1FFF);
    }
}
void Hal_HVSP_SetXNRSize(MS_U16 u16Width, MS_U16 u16Height)
{
    if(Is_Reg_Type(E_HVSP_CMD_TRIG_NONE) || Is_IP_First_Set(E_HVSP_ID_1) || Is_CannotUseCMDQToSetReg())
    {
        W2BYTEMSK(REG_VIP_SCNR_5C_L, u16Width-1, 0x0FFF);
        W2BYTEMSK(REG_VIP_SCNR_5D_L, u16Height-1, 0x1FFF);
    }
    else
    {
        _Hal_HVSP_Write_Cmd(REG_VIP_SCNR_5C_L, u16Width-1, 0x0FFF);
        _Hal_HVSP_Write_Cmd(REG_VIP_SCNR_5D_L, u16Height-1, 0x1FFF);
    }
}
void Hal_HVSP_PriMask_Width(MS_U16 u16W)
{
    if(Is_Reg_Type(E_HVSP_CMD_TRIG_NONE) || Is_IP_First_Set(E_HVSP_ID_1) || Is_CannotUseCMDQToSetReg())
    {
        W2BYTE(REG_VIP_SCNR_15_L, u16W-1);
    }
    else
    {
        _Hal_HVSP_Write_Cmd(REG_VIP_SCNR_15_L, u16W-1, 0x0FFF);
    }
}
void Hal_HVSP_PriMask_Height(MS_U16 u16H)
{
    if(Is_Reg_Type(E_HVSP_CMD_TRIG_NONE) || Is_IP_First_Set(E_HVSP_ID_1) || Is_CannotUseCMDQToSetReg())
    {
        W2BYTE(REG_VIP_SCNR_16_L, u16H-1);
    }
    else
    {
        _Hal_HVSP_Write_Cmd(REG_VIP_SCNR_16_L, u16H-1, 0x0FFF);
    }
}

void Hal_HVSP_Set_SW_Reset(MS_BOOL bEn)
{
    W2BYTEMSK(REG_SCL0_01_L, bEn ? BIT0 : 0, BIT0);
    W2BYTEMSK(REG_SCL0_01_L, 0, BIT0);
}
void Hal_HVSP_Set_DNR_Reset(MS_BOOL bEn)
{
// for issue reset rstz_ctrl_clk_miu_sc_dnr cause to overwrite 0x0
    W2BYTEMSK(REG_SCL_DNR1_24_L, bEn ? BIT6 : 0, BIT6);
    W2BYTEMSK(REG_SCL_DNR1_60_L, bEn ? BIT0 : 0, BIT0);
}

void Hal_HVSP_SetNLMLineBufferSize(MS_U16 u16Width, MS_U16 u16Height)
{
	  W2BYTEMSK(REG_SCL0_19_L, 0x8000, 0x8000);
      if(Is_Reg_Type(E_HVSP_CMD_TRIG_NONE) || Is_IP_First_Set(E_HVSP_ID_1)|| Is_CannotUseCMDQToSetReg())
      {
        W2BYTEMSK(REG_SCL0_1A_L, u16Width, 0x0FFF);
        W2BYTEMSK(REG_SCL0_1B_L, u16Height, 0x1FFF);
      }
      else
      {
          _Hal_HVSP_Write_Cmd(REG_SCL0_1A_L, u16Width, 0x0FFF);
          _Hal_HVSP_Write_Cmd(REG_SCL0_1B_L, u16Height, 0x1FFF);
      }
}

void Hal_HVSP_SetNLMEn(MS_BOOL bEn)
{
    W2BYTEMSK(REG_SCL0_18_L, bEn ? BIT0 : 0, BIT0);
}
void Hal_HVSP_SetVpsSRAMEn(MS_BOOL bEn)
{
    W2BYTEMSK(REG_VIP_PK_10_L, bEn ? BIT7 : 0, BIT7);
}


//-------------------------------------------------------------------------------------------------
// Crop
//-------------------------------------------------------------------------------------------------
void Hal_HVSP_SetBT656SrcConfig(MS_BOOL bEn,MS_U16 u16Lineoffset)
{
    if(Is_InputSource(E_HVSP_IP_MUX_BT656) )
    {
        W2BYTEMSK(REG_SCL0_61_L, bEn ? BIT7 : 0, BIT7);
        W2BYTEMSK(REG_SCL0_63_L, u16Lineoffset, 0x07FF);
        W2BYTEMSK(REG_SCL0_62_L, 8, 0x07FF);
    }
}


void Hal_HVSP_SetCropConfig(EN_HVSP_CROP_ID_TYPE enID, ST_HVSP_CROP_INFO *stCropInfo)
{
    MS_U32 u32reg_idx = enID == E_HVSP_CROP_ID_1 ? 0x00 : 0x10;

    //Crop01: 0x50
    //Crop02: 0x58
    if(Is_Reg_Type(E_HVSP_CMD_TRIG_NONE) || Is_IP_First_Set(E_HVSP_ID_1)|| Is_CannotUseCMDQToSetReg())
    {
        if((stCropInfo->u16In_hsize == stCropInfo->u16Hsize) && (stCropInfo->u16Vsize == stCropInfo->u16In_vsize))
        {
            W2BYTEMSK(REG_SCL2_50_L + u32reg_idx, 0, BIT0);
        }
        else
        {
            W2BYTEMSK(REG_SCL2_50_L + u32reg_idx, stCropInfo->bEn ? BIT0 : 0, BIT0);
        }
        W2BYTEMSK(REG_SCL2_51_L + u32reg_idx, stCropInfo->u16In_hsize, 0x0FFF);
        W2BYTEMSK(REG_SCL2_52_L + u32reg_idx, stCropInfo->u16In_vsize, 0x0FFF);
        W2BYTEMSK(REG_SCL2_53_L + u32reg_idx, stCropInfo->u16Hst, 0x0FFF);
        W2BYTEMSK(REG_SCL2_54_L + u32reg_idx, stCropInfo->u16Hsize, 0x0FFF);
        W2BYTEMSK(REG_SCL2_55_L + u32reg_idx, stCropInfo->u16Vst, 0x0FFF);
        W2BYTEMSK(REG_SCL2_56_L + u32reg_idx, stCropInfo->u16Vsize, 0x0FFF);
    }
    else
    {
        if((stCropInfo->u16In_hsize == stCropInfo->u16Hsize) && (stCropInfo->u16Vsize == stCropInfo->u16In_vsize))
        {
            _Hal_HVSP_Write_Cmd(REG_SCL2_50_L + u32reg_idx, 0, BIT0);
        }
        else
        {
            _Hal_HVSP_Write_Cmd(REG_SCL2_50_L + u32reg_idx, stCropInfo->bEn ? BIT0 : 0, BIT0);
        }
        _Hal_HVSP_Write_Cmd(REG_SCL2_51_L + u32reg_idx, stCropInfo->u16In_hsize, 0x0FFF);
        _Hal_HVSP_Write_Cmd(REG_SCL2_52_L + u32reg_idx, stCropInfo->u16In_vsize, 0x0FFF);
        _Hal_HVSP_Write_Cmd(REG_SCL2_53_L + u32reg_idx, stCropInfo->u16Hst, 0x0FFF);
        _Hal_HVSP_Write_Cmd(REG_SCL2_54_L + u32reg_idx, stCropInfo->u16Hsize, 0x0FFF);
        _Hal_HVSP_Write_Cmd(REG_SCL2_55_L + u32reg_idx, stCropInfo->u16Vst, 0x0FFF);
        _Hal_HVSP_Write_Cmd(REG_SCL2_56_L + u32reg_idx, stCropInfo->u16Vsize, 0x0FFF);
    }
}

//-------------------------------------------------------------------------------------------------
// IP Mux
//-------------------------------------------------------------------------------------------------
void Hal_HVSP_SetInputMuxType(EN_HVSP_IP_MUX_TYPE enIpType)
{
    genIpType = enIpType;
}

EN_HVSP_IP_MUX_TYPE Hal_HVSP_GetInputSrcMux(void)
{
    return genIpType;
}

void Hal_HVSP_SetHWInputMux(EN_HVSP_IP_MUX_TYPE enIpType)
{
    W2BYTEMSK(REG_SCL0_60_L, enIpType == E_HVSP_IP_MUX_BT656 ? 0 : BIT0, BIT0);
}


//-------------------------------------------------------------------------------------------------
// IPM
//-------------------------------------------------------------------------------------------------
void Hal_HVSP_SetIPMYCMReadEn(MS_BOOL bEn)
{
    if(!LOCK_FB())
    W2BYTEMSK(REG_SCL_DNR1_01_L, bEn ? BIT0 : 0, BIT0);
}

void Hal_HVSP_SetIPMYCMWriteEn(MS_BOOL bEn)
{
    if(!LOCK_FB())
    W2BYTEMSK(REG_SCL_DNR1_01_L, bEn ? BIT1 : 0, BIT1);
}
void Hal_HVSP_SetIPMCIIRReadEn(MS_BOOL bEn)
{
    if(!LOCK_FB())
    W2BYTEMSK(REG_SCL_DNR1_01_L, bEn ? BIT2 : 0, BIT2);
}

void Hal_HVSP_SetIPMCIIRWriteEn(MS_BOOL bEn)
{
    if(!LOCK_FB())
    W2BYTEMSK(REG_SCL_DNR1_01_L, bEn ? BIT3 : 0, BIT3);
}

void Hal_HVSP_SetIPMYCBase(MS_U32 u32Base)
{
    MS_U16 u16Base_Lo, u16Base_Hi;
    u32Base = u32Base>>4;
    u16Base_Lo = u32Base & 0xFFFF;
    u16Base_Hi = (u32Base & 0x07FF0000) >> 16;
    W2BYTE(REG_SCL_DNR1_08_L, u16Base_Lo);
    W2BYTE(REG_SCL_DNR1_09_L, u16Base_Hi);
}
void Hal_HVSP_SetIPMMotionBase(MS_U32 u32Base)
{
    MS_U16 u16Base_Lo, u16Base_Hi;
    u32Base = u32Base>>4;
    u16Base_Lo = u32Base & 0xFFFF;
    u16Base_Hi = (u32Base & 0x07FF0000) >> 16;
    W2BYTE(REG_SCL_DNR1_10_L, u16Base_Lo);
    W2BYTE(REG_SCL_DNR1_11_L, u16Base_Hi);
}
void Hal_HVSP_SetIPMCIIRBase(MS_U32 u32Base)
{
    MS_U16 u16Base_Lo, u16Base_Hi;
    u32Base = u32Base>>4;
    u16Base_Lo = u32Base & 0xFFFF;
    u16Base_Hi = (u32Base & 0x07FF0000) >> 16;
    W2BYTE(REG_SCL_DNR1_18_L, u16Base_Lo);
    W2BYTE(REG_SCL_DNR1_19_L, u16Base_Hi);
}

#if ENABLE_HVSP_UNUSED_FUNCTION
void Hal_HVSP_Set_IPM_Read_Req(MS_U8 u8Thrd, MS_U8 u8Limit)
{
    W2BYTE(REG_SCL_DNR1_0A_L, ((((MS_U16)u8Limit)<<8) | ((MS_U16)u8Thrd)));
}

void Hal_HVSP_Set_IPM_Write_Req(MS_U8 u8Thrd, MS_U8 u8Limit)
{
    W2BYTE(REG_SCL_DNR1_0B_L, ((((MS_U16)u8Limit)<<8) | ((MS_U16)u8Thrd)));
}
#endif

void Hal_HVSP_SetIPMvSize(MS_U16 u16Vsize)
{
    if(Is_Reg_Type(E_HVSP_CMD_TRIG_NONE))
    {
        W2BYTEMSK(REG_SCL_DNR1_0D_L, u16Vsize, 0x1FFF);
    }
    else
    {
        _Hal_HVSP_Write_Cmd(REG_SCL_DNR1_0D_L, u16Vsize, 0x1FFF);
    }
    gu16height[0] = u16Vsize;
}

void Hal_HVSP_SetIPMLineOffset(MS_U16 u16Lineoffset)
{
    if(Is_Reg_Type(E_HVSP_CMD_TRIG_NONE))
    {
        W2BYTEMSK(REG_SCL_DNR1_0E_L, u16Lineoffset, 0x1FFF);
    }
    else
    {
        _Hal_HVSP_Write_Cmd(REG_SCL_DNR1_0E_L, u16Lineoffset, 0x1FFF);
    }
    gu16width[0] = u16Lineoffset;
}

void Hal_HVSP_SetIPMFetchNum(MS_U16 u16FetchNum)
{
    if(Is_Reg_Type(E_HVSP_CMD_TRIG_NONE))
    {
        W2BYTEMSK(REG_SCL_DNR1_0F_L, u16FetchNum, 0x1FFF);
    }
    else
    {
        _Hal_HVSP_Write_Cmd(REG_SCL_DNR1_0F_L, u16FetchNum, 0x1FFF);
    }
}
void Hal_HVSP_SetIPMBufferNumber(MS_U8 u8Num)
{
    if(!LOCK_FB())
    W2BYTEMSK(REG_SCL_DNR1_02_L, u8Num , 0x7);
}

//-------------------------------------------------------------------------------------------------
// Scaling
//-------------------------------------------------------------------------------------------------

void Hal_HVSP_SetScalingVeEn(EN_HVSP_ID_TYPE enID, MS_BOOL ben)
{
    MS_U32 u32reg = enID == E_HVSP_ID_2 ? REG_SCL_HVSP1_0A_L :
                    enID == E_HVSP_ID_3 ? REG_SCL_HVSP2_0A_L :
                                           REG_SCL_HVSP0_0A_L;

    if(Is_Reg_Type(E_HVSP_CMD_TRIG_NONE) || Is_IP_First_Set(enID)|| Is_CannotUseCMDQToSetReg())
    {
        W2BYTEMSK(u32reg, ben ? BIT8 : 0, BIT8);
    }
    else
    {
        //_Hal_HVSP_Write_Cmd(u32reg, ben ? BIT8 : 0, BIT8);
    }
}
void Hal_HVSP_SetScalingVeFactor(EN_HVSP_ID_TYPE enID, MS_U32 u32Ratio)
{
    MS_U32 u32reg = enID == E_HVSP_ID_2 ? REG_SCL_HVSP1_09_L :
                    enID == E_HVSP_ID_3 ? REG_SCL_HVSP2_09_L :
                                           REG_SCL_HVSP0_09_L;

    if(Is_Reg_Type(E_HVSP_CMD_TRIG_NONE) || Is_IP_First_Set(enID)|| Is_CannotUseCMDQToSetReg())
    {
        W3BYTE(u32reg, u32Ratio);
    }
    else
    {
        MS_U16 u16val_L = u32Ratio & 0xFFFF;
        MS_U16 u16val_H = (u32Ratio >> 16) & 0x00FF;
        if(u32Ratio)
        {
            u16val_H=(u16val_H|0x0100);
        }
        _Hal_HVSP_Write_Cmd(u32reg,   u16val_L, 0xFFFF);
        _Hal_HVSP_Write_Cmd(u32reg+2, u16val_H, 0x01FF);
    }
}

void Hal_HVSP_SetScalingHoEn(EN_HVSP_ID_TYPE enID, MS_BOOL ben)
{
    MS_U32 u32reg = enID == E_HVSP_ID_2 ? REG_SCL_HVSP1_08_L :
                    enID == E_HVSP_ID_3 ? REG_SCL_HVSP2_08_L :
                                          REG_SCL_HVSP0_08_L;

    if(Is_Reg_Type(E_HVSP_CMD_TRIG_NONE) || Is_IP_First_Set(enID)|| Is_CannotUseCMDQToSetReg())
    {
        W2BYTEMSK(u32reg, ben ? BIT8 : 0, BIT8);
    }
    else
    {
        //_Hal_HVSP_Write_Cmd(u32reg, ben ? BIT8 : 0, BIT8);
    }
}

void Hal_HVSP_SetScalingHoFacotr(EN_HVSP_ID_TYPE enID, MS_U32 u32Ratio)
{
    MS_U32 u32reg = enID == E_HVSP_ID_2 ? REG_SCL_HVSP1_07_L :
                    enID == E_HVSP_ID_3 ? REG_SCL_HVSP2_07_L :
                                          REG_SCL_HVSP0_07_L;

    if(Is_Reg_Type(E_HVSP_CMD_TRIG_NONE) || Is_IP_First_Set(enID)|| Is_CannotUseCMDQToSetReg())
    {
        W3BYTE(u32reg, u32Ratio);
    }
    else
    {
        MS_U16 u16val_L = u32Ratio & 0xFFFF;
        MS_U16 u16val_H = (u32Ratio >> 16) & 0x00FF;
        if(u32Ratio)
        {
            u16val_H=(u16val_H|0x0100);
        }
        _Hal_HVSP_Write_Cmd(u32reg,   u16val_L, 0xFFFF);
        _Hal_HVSP_Write_Cmd(u32reg+2, u16val_H, 0x01FF);
    }
}
void Hal_HVSP_SetModeYHo(EN_HVSP_ID_TYPE enID, EN_HVSP_FILTER_MODE enFilterMode)
{
    MS_U32 u32reg = enID == E_HVSP_ID_2 ? REG_SCL_HVSP1_0B_L :
                    enID == E_HVSP_ID_3 ? REG_SCL_HVSP2_0B_L :
                                           REG_SCL_HVSP0_0B_L;
    genFilterMode=enFilterMode;
    if(Is_Reg_Type(E_HVSP_CMD_TRIG_NONE) || Is_IP_First_Set(enID)|| Is_CannotUseCMDQToSetReg())
    {
        W2BYTEMSK(u32reg, enFilterMode == E_HVSP_FILTER_MODE_BYPASS ? 0 : BIT0, BIT0);
        W2BYTEMSK(u32reg, (enFilterMode == E_HVSP_FILTER_MODE_SRAM_0 || enFilterMode == E_HVSP_FILTER_MODE_SRAM_1) ? BIT6 : 0, BIT6);
        W2BYTEMSK(u32reg, enFilterMode == E_HVSP_FILTER_MODE_SRAM_1 ? BIT7 : 0, BIT7);
    }
    else
    {
        //_Hal_HVSP_Write_Cmd(u32reg, bBypass ? 0 : BIT0, BIT0);
        if(enFilterMode == E_HVSP_FILTER_MODE_BYPASS)
        {
            _ReSetHVSPType(enID,BIT0|BIT6|BIT7);
        }
        else if(enFilterMode == E_HVSP_FILTER_MODE_BILINEAR)
        {
            _SetHVSPType(enID,BIT0);
            _ReSetHVSPType(enID,BIT6|BIT7);
        }
        else
        {
            _SetHVSPType(enID,BIT0|BIT6);
            if(enFilterMode == E_HVSP_FILTER_MODE_SRAM_1)
            {
                _SetHVSPType(enID,BIT7);
            }
            else
            {
                _ReSetHVSPType(enID,BIT7);
            }
        }
    }
}

void Hal_HVSP_SetModeYVe(EN_HVSP_ID_TYPE enID, EN_HVSP_FILTER_MODE enFilterMode)
{
    MS_U32 u32reg = enID == E_HVSP_ID_2 ? REG_SCL_HVSP1_0B_L :
                    enID == E_HVSP_ID_3 ? REG_SCL_HVSP2_0B_L :
                                           REG_SCL_HVSP0_0B_L;

    if(Is_Reg_Type(E_HVSP_CMD_TRIG_NONE) || Is_IP_First_Set(enID)|| Is_CannotUseCMDQToSetReg())
    {
        W2BYTEMSK(u32reg, enFilterMode == E_HVSP_FILTER_MODE_BYPASS ? 0 : BIT8, BIT8);
        W2BYTEMSK(u32reg, (enFilterMode == E_HVSP_FILTER_MODE_SRAM_0 || enFilterMode == E_HVSP_FILTER_MODE_SRAM_1) ? BIT14 : 0, BIT14);
        W2BYTEMSK(u32reg, enFilterMode == E_HVSP_FILTER_MODE_SRAM_1 ? BIT15 : 0, BIT15);
    }
    else
    {
        if(enFilterMode == E_HVSP_FILTER_MODE_BYPASS)
        {
            _ReSetHVSPType(enID,BIT15|BIT14|BIT8);
        }
        else if(enFilterMode == E_HVSP_FILTER_MODE_BILINEAR)
        {
            _SetHVSPType(enID,BIT8);
            _ReSetHVSPType(enID,BIT14|BIT15);
        }
        else
        {
            _SetHVSPType(enID,BIT8|BIT14);
            if(enFilterMode == E_HVSP_FILTER_MODE_SRAM_1)
            {
                _SetHVSPType(enID,BIT15);
            }
            else
            {
                _ReSetHVSPType(enID,BIT15);
            }
        }
        //_Hal_HVSP_Write_Cmd(u32reg, bBypass ? 0 : BIT8, BIT8);
    }
}

void Hal_HVSP_SetModeCHo(EN_HVSP_ID_TYPE enID, EN_HVSP_FILTER_MODE enFilterMode, EN_HVSP_SRAM_SEL_TYPE enSramSel)
{
    MS_U32 u32reg = enID == E_HVSP_ID_2 ? REG_SCL_HVSP1_0B_L :
                    enID == E_HVSP_ID_3 ? REG_SCL_HVSP2_0B_L :
                                           REG_SCL_HVSP0_0B_L;

    if(Is_Reg_Type(E_HVSP_CMD_TRIG_NONE) || Is_IP_First_Set(enID)|| Is_CannotUseCMDQToSetReg())
    {
        if(enFilterMode == E_HVSP_FILTER_MODE_BYPASS || enFilterMode == E_HVSP_FILTER_MODE_BILINEAR)
        {
            W2BYTEMSK(u32reg, enFilterMode == E_HVSP_FILTER_MODE_BYPASS ? (0<<1) : (1<<1), (BIT3|BIT2|BIT1));
            W2BYTEMSK(u32reg, 0x0000, BIT4); // ram_en
            W2BYTEMSK(u32reg, enSramSel == E_HVSP_SRAM_SEL_0 ? (0<<5) : (1<<5), BIT5); //ram_sel
        }
        else
        {
            W2BYTEMSK(u32reg, enFilterMode == E_HVSP_FILTER_MODE_SRAM_0 ? (2<<1) : (3<<1), (BIT3|BIT2|BIT1));
            W2BYTEMSK(u32reg, BIT4, BIT4); // ram_en
            W2BYTEMSK(u32reg, enSramSel == E_HVSP_SRAM_SEL_0 ? (0<<5) : (1<<5), BIT5); //ram_sel
        }
    }
    else
    {
        if(enFilterMode == E_HVSP_FILTER_MODE_BYPASS)
        {
            _ReSetHVSPType(enID,BIT3|BIT2|BIT1|BIT4);
            if(enSramSel == E_HVSP_SRAM_SEL_0)
            {
                _ReSetHVSPType(enID,BIT5);
            }
            else
            {
                _SetHVSPType(enID,BIT5);
            }
        }
        else if(enFilterMode == E_HVSP_FILTER_MODE_BILINEAR)
        {
            _SetHVSPType(enID,BIT1);
            _ReSetHVSPType(enID,BIT3|BIT2|BIT4);
            if(enSramSel == E_HVSP_SRAM_SEL_0)
            {
                _ReSetHVSPType(enID,BIT5);
            }
            else
            {
                _SetHVSPType(enID,BIT5);
            }
        }
        else
        {
            _SetHVSPType(enID,BIT4|BIT2);
            _ReSetHVSPType(enID,BIT3);
            if(enFilterMode == E_HVSP_FILTER_MODE_SRAM_1)
            {
                _SetHVSPType(enID,BIT1);
            }
            else
            {
                _ReSetHVSPType(enID,BIT1);
            }
            if(enSramSel == E_HVSP_SRAM_SEL_0)
            {
                _ReSetHVSPType(enID,BIT5);
            }
            else
            {
                _SetHVSPType(enID,BIT5);
            }
        }
    }
}

void Hal_HVSP_SetModeCVe(EN_HVSP_ID_TYPE enID, EN_HVSP_FILTER_MODE enFilterMode, EN_HVSP_SRAM_SEL_TYPE enSramSel)
{
    MS_U32 u32reg = enID == E_HVSP_ID_2 ? REG_SCL_HVSP1_0B_L :
                    enID == E_HVSP_ID_3 ? REG_SCL_HVSP2_0B_L :
                                           REG_SCL_HVSP0_0B_L;
    if(Is_Reg_Type(E_HVSP_CMD_TRIG_NONE) || Is_IP_First_Set(enID)|| Is_CannotUseCMDQToSetReg())
    {
        if(enFilterMode == E_HVSP_FILTER_MODE_BYPASS || enFilterMode == E_HVSP_FILTER_MODE_BILINEAR)
        {
            W2BYTEMSK(u32reg, enFilterMode == E_HVSP_FILTER_MODE_BYPASS ? (0<<9) : (1<<9), (BIT11|BIT10|BIT9));
            W2BYTEMSK(u32reg, 0x0000, BIT12); // ram_en
            W2BYTEMSK(u32reg, enSramSel == E_HVSP_SRAM_SEL_0 ? (0<<13) : (1<<13), BIT13); //ram_sel
        }
        else
        {
            W2BYTEMSK(u32reg, enFilterMode == E_HVSP_FILTER_MODE_SRAM_0 ? (2<<9) : (3<<9), (BIT11|BIT10|BIT9));
            W2BYTEMSK(u32reg, BIT12, BIT12); // ram_en
            W2BYTEMSK(u32reg, enSramSel == E_HVSP_SRAM_SEL_0 ? (0<<13) : (1<<13), BIT13); //ram_sel
        }
    }
    else
    {
        if(enFilterMode == E_HVSP_FILTER_MODE_BYPASS)
        {
            _ReSetHVSPType(enID,BIT9|BIT10|BIT11|BIT12);
            if(enSramSel == E_HVSP_SRAM_SEL_0)
            {
                _ReSetHVSPType(enID,BIT13);
            }
            else
            {
                _SetHVSPType(enID,BIT13);
            }
        }
        else if(enFilterMode == E_HVSP_FILTER_MODE_BILINEAR)
        {
            _SetHVSPType(enID,BIT9);
            _ReSetHVSPType(enID,BIT10|BIT11|BIT12);
            if(enSramSel == E_HVSP_SRAM_SEL_0)
            {
                _ReSetHVSPType(enID,BIT13);
            }
            else
            {
                _SetHVSPType(enID,BIT13);
            }
        }
        else
        {
            _SetHVSPType(enID,BIT10|BIT12);
            _ReSetHVSPType(enID,BIT11);
            if(enFilterMode == E_HVSP_FILTER_MODE_SRAM_1)
            {
                _SetHVSPType(enID,BIT9);
            }
            else
            {
                _ReSetHVSPType(enID,BIT9);
            }
            if(enSramSel == E_HVSP_SRAM_SEL_0)
            {
                _ReSetHVSPType(enID,BIT13);
            }
            else
            {
                _SetHVSPType(enID,BIT13);
            }
        }
        _Hal_HVSP_Write_Cmd(u32reg,gstSclFea[enID].u16ModeYCVH,0xFFFF);
    }

}

void Hal_HVSP_SetHspDithEn(EN_HVSP_ID_TYPE enID, MS_BOOL bEn)
{
    MS_U32 u32reg = enID == E_HVSP_ID_2 ? REG_SCL_HVSP1_0C_L :
                    enID == E_HVSP_ID_3 ? REG_SCL_HVSP2_0C_L :
                                           REG_SCL_HVSP0_0C_L;

    if(Is_Reg_Type(E_HVSP_CMD_TRIG_NONE) || Is_IP_First_Set(enID)|| Is_CannotUseCMDQToSetReg())
    {
        W2BYTEMSK(u32reg, bEn ? (BIT0) : (0), BIT0);
    }
    else
    {
        //_Hal_HVSP_Write_Cmd(u32reg, bEn ? (BIT0) : (0), BIT0);
    }
}


void Hal_HVSP_SetVspDithEn(EN_HVSP_ID_TYPE enID, MS_BOOL bEn)
{
    MS_U32 u32reg = enID == E_HVSP_ID_2 ? REG_SCL_HVSP1_0C_L :
                    enID == E_HVSP_ID_3 ? REG_SCL_HVSP2_0C_L :
                                           REG_SCL_HVSP0_0C_L;

    if(Is_Reg_Type(E_HVSP_CMD_TRIG_NONE) || Is_IP_First_Set(enID)|| Is_CannotUseCMDQToSetReg())
    {
        W2BYTEMSK(u32reg, bEn ? (BIT1) : (0), BIT1);
    }
    else
    {
        //_Hal_HVSP_Write_Cmd(u32reg, bEn ? (BIT1) : (0), BIT1);
    }
}

void Hal_HVSP_SetHspCoringEnC(EN_HVSP_ID_TYPE enID, MS_BOOL bEn)
{
    MS_U32 u32reg = enID == E_HVSP_ID_2 ? REG_SCL_HVSP1_0C_L :
                    enID == E_HVSP_ID_3 ? REG_SCL_HVSP2_0C_L :
                                           REG_SCL_HVSP0_0C_L;

    if(Is_Reg_Type(E_HVSP_CMD_TRIG_NONE) || Is_IP_First_Set(enID)|| Is_CannotUseCMDQToSetReg())
    {
        W2BYTEMSK(u32reg, bEn ? (BIT8) : (0), BIT8);
    }
    else
    {
        //_Hal_HVSP_Write_Cmd(u32reg, bEn ? (BIT8) : (0), BIT8);
    }
}

void Hal_HVSP_SetHspCoringEnY(EN_HVSP_ID_TYPE enID, MS_BOOL bEn)
{
    MS_U32 u32reg = enID == E_HVSP_ID_2 ? REG_SCL_HVSP1_0C_L :
                    enID == E_HVSP_ID_3 ? REG_SCL_HVSP2_0C_L :
                                           REG_SCL_HVSP0_0C_L;

    if(Is_Reg_Type(E_HVSP_CMD_TRIG_NONE) || Is_IP_First_Set(enID)|| Is_CannotUseCMDQToSetReg())
    {
        W2BYTEMSK(u32reg, bEn ? (BIT9) : (0), BIT9);
    }
    else
    {
        //_Hal_HVSP_Write_Cmd(u32reg, bEn ? (BIT11|BIT10|BIT9|BIT8|BIT1|BIT0) : (0), BIT11|BIT10|BIT9|BIT8|BIT1|BIT0);
    }
}

void Hal_HVSP_SetVspCoringEnC(EN_HVSP_ID_TYPE enID, MS_BOOL bEn)
{
    MS_U32 u32reg = enID == E_HVSP_ID_2 ? REG_SCL_HVSP1_0C_L :
                    enID == E_HVSP_ID_3 ? REG_SCL_HVSP2_0C_L :
                                           REG_SCL_HVSP0_0C_L;

    if(Is_Reg_Type(E_HVSP_CMD_TRIG_NONE) || Is_IP_First_Set(enID)|| Is_CannotUseCMDQToSetReg())
    {
        W2BYTEMSK(u32reg, bEn ? (BIT10) : (0), BIT10);
    }
    else
    {
        //_Hal_HVSP_Write_Cmd(u32reg, bEn ? (BIT10) : (0), BIT10);
    }
}

void Hal_HVSP_SetVspCoringEnY(EN_HVSP_ID_TYPE enID, MS_BOOL bEn)
{
    MS_U32 u32reg = enID == E_HVSP_ID_2 ? REG_SCL_HVSP1_0C_L :
                    enID == E_HVSP_ID_3 ? REG_SCL_HVSP2_0C_L :
                                           REG_SCL_HVSP0_0C_L;

    if(Is_Reg_Type(E_HVSP_CMD_TRIG_NONE) || Is_IP_First_Set(enID)|| Is_CannotUseCMDQToSetReg())
    {
        W2BYTEMSK(u32reg, bEn ? (BIT11) : (0), BIT11);
    }
    else
    {
        _Hal_HVSP_Write_Cmd(u32reg, bEn ? (BIT11|BIT10|BIT9|BIT8|BIT1|BIT0) : (0), BIT11|BIT10|BIT9|BIT8|BIT1|BIT0);
    }
}

void Hal_HVSP_SetHspCoringThrdC(EN_HVSP_ID_TYPE enID, MS_U16 u16Thread)
{
    MS_U32 u32reg = enID == E_HVSP_ID_2 ? REG_SCL_HVSP1_0D_L :
                    enID == E_HVSP_ID_3 ? REG_SCL_HVSP2_0D_L :
                                           REG_SCL_HVSP0_0D_L;

    W2BYTEMSK(u32reg, u16Thread, 0x00FF);
}

void Hal_HVSP_SetHspCoringThrdY(EN_HVSP_ID_TYPE enID, MS_U16 u16Thread)
{
    MS_U32 u32reg = enID == E_HVSP_ID_2 ? REG_SCL_HVSP1_0D_L :
                    enID == E_HVSP_ID_3 ? REG_SCL_HVSP2_0D_L :
                                           REG_SCL_HVSP0_0D_L;

    W2BYTEMSK(u32reg, u16Thread<<8, 0xFF00);
}

void Hal_HVSP_SetVspCoringThrdC(EN_HVSP_ID_TYPE enID, MS_U16 u16Thread)
{
    MS_U32 u32reg = enID == E_HVSP_ID_2 ? REG_SCL_HVSP1_0E_L :
                    enID == E_HVSP_ID_3 ? REG_SCL_HVSP2_0E_L :
                                           REG_SCL_HVSP0_0E_L;

    W2BYTEMSK(u32reg, u16Thread, 0x00FF);
}

void Hal_HVSP_SetVspCoringThrdY(EN_HVSP_ID_TYPE enID, MS_U16 u16Thread)
{
    MS_U32 u32reg = enID == E_HVSP_ID_2 ? REG_SCL_HVSP1_0E_L :
                    enID == E_HVSP_ID_3 ? REG_SCL_HVSP2_0E_L :
                                           REG_SCL_HVSP0_0E_L;

    W2BYTEMSK(u32reg, u16Thread<<8, 0xFF00);
}
#if ENABLE_HVSP_UNUSED_FUNCTION

void Hal_HVSP_Set_Hvsp_Sram_Coeff(EN_HVSP_ID_TYPE enID, EN_HVSP_FILTER_SRAM_SEL_TYPE enSramSel, MS_BOOL bC_SRAM, MS_U8 *pData)
{
    MS_U32 u32reg_41, u32reg_42, u32reg_43;
    MS_U8 u8Ramcode[10];
    MS_U16 u16IdxBase = 0;
    MS_U16 i, j, x;

    if(enID == E_HVSP_ID_2)
    {
        u32reg_41 = REG_SCL_HVSP1_41_L;
        u32reg_42 = REG_SCL_HVSP1_42_L;
        u32reg_43 = REG_SCL_HVSP1_43_L;
    }
    else if(enID == E_HVSP_ID_3)
    {
        u32reg_41 = REG_SCL_HVSP2_41_L;
        u32reg_42 = REG_SCL_HVSP2_42_L;
        u32reg_43 = REG_SCL_HVSP2_43_L;
    }
    else
    {
        u32reg_41 = REG_SCL_HVSP0_41_L;
        u32reg_42 = REG_SCL_HVSP0_42_L;
        u32reg_43 = REG_SCL_HVSP0_43_L;
    }

    u16IdxBase = enSramSel == E_HVSP_FILTER_SRAM_SEL_1 ? 0x00 :
                 enSramSel == E_HVSP_FILTER_SRAM_SEL_2 ? 0x40 :
                 enSramSel == E_HVSP_FILTER_SRAM_SEL_3 ? 0x80 :
                                                          0xC0 ;

    W2BYTEMSK(u32reg_41, bC_SRAM ? BIT1 : BIT0, BIT1|BIT0); // reg_cram_rw_en

    for(i=0; i<64; i++)
    {
        while(R2BYTE(u32reg_41) & BIT8);
        j=i*5;

        W2BYTEMSK(u32reg_42,(i|u16IdxBase), 0x00FF);
        for ( x=0;x<5;x++ )
        {
            u8Ramcode[x] = pData[j+x];
        }

        W2BYTE(u32reg_43+0x00, (((MS_U16)u8Ramcode[1])<<8|(MS_U16)u8Ramcode[0]));
        W2BYTE(u32reg_43+0x02, (((MS_U16)u8Ramcode[3])<<8|(MS_U16)u8Ramcode[2]));
        W2BYTEMSK(u32reg_43+0x04, ((MS_U16)u8Ramcode[4]), 0x00FF);


        W2BYTEMSK(u32reg_41, BIT8, BIT8);
    }
}
#endif

void Hal_HVSP_SetHVSPInputSize(EN_HVSP_ID_TYPE enID, MS_U16 u16Width, MS_U16 u16Height)
{
    MS_U32 u32reg_20, u32reg_21;

    if(enID == E_HVSP_ID_2)
    {
        u32reg_20 = REG_SCL_HVSP1_20_L;
        u32reg_21 = REG_SCL_HVSP1_21_L;
        gu16height[0] = (gu16height[0]<u16Height) ? u16Height : gu16height[0];
        gu16width[0] = (gu16width[0]<u16Width) ? u16Width : gu16width[0];
    }
    else if(enID == E_HVSP_ID_3)
    {
        u32reg_20 = REG_SCL_HVSP2_20_L;
        u32reg_21 = REG_SCL_HVSP2_21_L;
        gu16height[1] = u16Height;
        gu16width[1] = u16Width;
    }
    else
    {
        u32reg_20 = REG_SCL_HVSP0_20_L;
        u32reg_21 = REG_SCL_HVSP0_21_L;
        gu16height[0] =  (gu16height[0]<u16Height) ? u16Height : gu16height[0];
        gu16width[0] =  (gu16width[0]<u16Width) ? u16Width : gu16width[0];
    }

    if(Is_Reg_Type(E_HVSP_CMD_TRIG_NONE) || Is_IP_First_Set(enID)|| Is_CannotUseCMDQToSetReg())
    {
        W2BYTEMSK(u32reg_20, u16Width,  0x1FFF);
        W2BYTEMSK(u32reg_21, u16Height, 0x0FFF);
    }
    else
    {
        _Hal_HVSP_Write_Cmd(u32reg_20, u16Width,  0x1FFF);
        _Hal_HVSP_Write_Cmd(u32reg_21, u16Height, 0x0FFF);
    }
}

void Hal_HVSP_SetHVSPOutputSize(EN_HVSP_ID_TYPE enID, MS_U16 u16Width, MS_U16 u16Height)
{
    MS_U32 u32reg_22, u32reg_23;
    MS_U32 u32ratio;
    MS_U16 u16val_L,u16val_H;
    MS_U64 u64temp;
    MS_U16 u16val_SC1;
    if(enID == E_HVSP_ID_2)
    {
        u32reg_22 = REG_SCL_HVSP1_22_L;
        u32reg_23 = REG_SCL_HVSP1_23_L;
        gu16height[0] = (gu16height[0]<u16Height) ? u16Height : gu16height[0];;
        gu16width[0] = (gu16width[0]<u16Width) ? u16Width : gu16width[0];
    }
    else if(enID == E_HVSP_ID_3)
    {
        u32reg_22 = REG_SCL_HVSP2_22_L;
        u32reg_23 = REG_SCL_HVSP2_23_L;
        gu16height[1] =(gu16height[1]<u16Height) ? u16Height : gu16height[1];
        gu16width[1] = (gu16width[1]<u16Width) ? u16Width : gu16width[1];
    }
    else
    {
        u32reg_22 = REG_SCL_HVSP0_22_L;
        u32reg_23 = REG_SCL_HVSP0_23_L;
        gu16height[0] = (gu16height[0]<u16Height) ? u16Height : gu16height[0];;
        gu16width[0] = (gu16width[0]<u16Width) ? u16Width : gu16width[0];
    }
    if(Is_Reg_Type(E_HVSP_CMD_TRIG_NONE) || Is_IP_First_Set(enID)|| Is_CannotUseCMDQToSetReg())
    {
        W2BYTEMSK(u32reg_22, u16Width,  0x1FFF);
        W2BYTEMSK(u32reg_23, u16Height, 0x0FFF);
        gbhvspset[enID]= 1;
        if(enID ==E_HVSP_ID_1)
        {
            W2BYTE(REG_SCL_DMA0_1A_L, u16Width);
            W2BYTE(REG_SCL_DMA0_1B_L, u16Height);
            W2BYTE(REG_SCL_DMA1_4A_L, u16Width);
            W2BYTE(REG_SCL_DMA1_4B_L, u16Height);
            W2BYTE(REG_SCL_DMA0_4A_L, u16Width);
            W2BYTE(REG_SCL_DMA0_4B_L, u16Height);
        }
        else if(enID ==E_HVSP_ID_2)
        {
            W2BYTE(REG_SCL_DMA0_62_L, u16Width );
            W2BYTE(REG_SCL_DMA0_63_L, u16Height);
            W2BYTE(REG_SCL_DMA1_62_L, u16Width );
            W2BYTE(REG_SCL_DMA1_63_L, u16Height);
        }
    }
    else
    {
        _Hal_HVSP_Write_Cmd(u32reg_22, u16Width,  0x1FFF);
        _Hal_HVSP_Write_Cmd(u32reg_23, u16Height, 0x0FFF);
        if(enID ==E_HVSP_ID_1)
        {
            if(R2BYTE(REG_SCL_DMA0_1A_L) != u16Width)
                _Hal_HVSP_Write_Cmd(REG_SCL_DMA0_1A_L, u16Width, 0xFFFF);
            if(R2BYTE(REG_SCL_DMA0_1B_L) != u16Height)
                _Hal_HVSP_Write_Cmd(REG_SCL_DMA0_1B_L, u16Height,0xFFFF);
            if(R2BYTE(REG_SCL_DMA1_4A_L) != u16Width)
                _Hal_HVSP_Write_Cmd(REG_SCL_DMA1_4A_L, u16Width, 0xFFFF);
            if(R2BYTE(REG_SCL_DMA1_4B_L) != u16Height)
                _Hal_HVSP_Write_Cmd(REG_SCL_DMA1_4B_L, u16Height,0xFFFF);
            if(R2BYTE(REG_SCL_DMA0_4A_L) != u16Width)
                _Hal_HVSP_Write_Cmd(REG_SCL_DMA0_4A_L, u16Width, 0xFFFF);
            if(R2BYTE(REG_SCL_DMA0_4B_L) != u16Height)
                _Hal_HVSP_Write_Cmd(REG_SCL_DMA0_4B_L, u16Height,0xFFFF);
            if((R2BYTE(REG_SCL_HVSP1_20_L) != u16Width)&&(gbhvspset[E_HVSP_ID_2]))
            {
                u16val_SC1 = R2BYTE(REG_SCL_HVSP1_22_L);
                if(u16val_SC1)
                {
                    u64temp = ((MS_U64)u16Width * 1048576);
                        do_div(u64temp,u16val_SC1);
                    u32ratio = (MS_U32)u64temp;
                    if(u32ratio)
                    {
                        u16val_L = u32ratio & 0xFFFF;
                        u16val_H = (u32ratio >> 16) & 0x00FF;
                        u16val_H = (u16val_H|0x0100);
                        _Hal_HVSP_Write_Cmd(REG_SCL_HVSP1_07_L,   u16val_L, 0xFFFF);
                        _Hal_HVSP_Write_Cmd(REG_SCL_HVSP1_07_L+2, u16val_H, 0x01FF);
                    }
                    _Hal_HVSP_Write_Cmd(REG_SCL_HVSP1_20_L, u16Width,0xFFFF);
                }
            }
            if((R2BYTE(REG_SCL_HVSP1_21_L) != u16Height)&&(gbhvspset[E_HVSP_ID_2]))
            {
                u16val_SC1 = R2BYTE(REG_SCL_HVSP1_23_L);
                if(u16val_SC1)
                {
                    u64temp = ((MS_U64)u16Height * 1048576);
                        do_div(u64temp,u16val_SC1);
                    u32ratio = (MS_U32)u64temp;
                    if(u32ratio)
                    {
                        u16val_L = u32ratio & 0xFFFF;
                        u16val_H = (u32ratio >> 16) & 0x00FF;
                        u16val_H = (u16val_H|0x0100);
                        _Hal_HVSP_Write_Cmd(REG_SCL_HVSP1_09_L,   u16val_L, 0xFFFF);
                        _Hal_HVSP_Write_Cmd(REG_SCL_HVSP1_09_L+2, u16val_H, 0x01FF);
                    }
                    _Hal_HVSP_Write_Cmd(REG_SCL_HVSP1_21_L, u16Height,0xFFFF);
                }
            }
        }
        else if(enID ==E_HVSP_ID_2)
        {
            if(R2BYTE(REG_SCL_DMA0_62_L) != u16Width)
                _Hal_HVSP_Write_Cmd(REG_SCL_DMA0_62_L, u16Width, 0xFFFF);
            if(R2BYTE(REG_SCL_DMA0_63_L) != u16Height)
                _Hal_HVSP_Write_Cmd(REG_SCL_DMA0_63_L, u16Height,0xFFFF);
            if(R2BYTE(REG_SCL_DMA1_62_L) != u16Width)
                _Hal_HVSP_Write_Cmd(REG_SCL_DMA1_62_L, u16Width, 0xFFFF);
            if(R2BYTE(REG_SCL_DMA1_63_L) != u16Height)
                _Hal_HVSP_Write_Cmd(REG_SCL_DMA1_63_L, u16Height,0xFFFF);
        }
    }
}

// input tgen
void Hal_HVSP_SetPatTgEn(MS_BOOL bEn)
{
    W2BYTEMSK(REG_SCL0_40_L, bEn ? (BIT0|BIT1|BIT2|BIT15) : 0, BIT0|BIT1|BIT2|BIT15);
    W2BYTEMSK(REG_SCL0_70_L, bEn ? BIT0 : 0, BIT0);
    W2BYTEMSK(REG_SCL0_44_L, bEn ? BIT0 : 0, BIT0); // h/v
}

void Hal_HVSP_SetTestPatCfg(void)
{
    W2BYTE(REG_SCL0_42_L, 0x1010); // cb h/v width
    W2BYTE(REG_SCL0_43_L, 0x0008); // shfit time
    W2BYTE(REG_SCL0_44_L, 0x0C20); // h/v
}


void Hal_HVSP_SetPatTgVsyncSt(MS_U16 u16Val)
{
    W2BYTEMSK(REG_SCL0_71_L, u16Val, 0x0FFF);
}

void Hal_HVSP_SetPatTgVsyncEnd(MS_U16 u16Val)
{
    W2BYTEMSK(REG_SCL0_72_L, u16Val, 0x0FFF);
}

void Hal_HVSP_SetPatTgVfdeSt(MS_U16 u16Val)
{
    W2BYTEMSK(REG_SCL0_73_L, u16Val, 0x0FFF);
}

void Hal_HVSP_SetPatTgVfdeEnd(MS_U16 u16Val)
{
    W2BYTEMSK(REG_SCL0_74_L, u16Val, 0x0FFF);
}

void Hal_HVSP_SetPatTgVdeSt(MS_U16 u16Val)
{
    W2BYTEMSK(REG_SCL0_75_L, u16Val, 0x0FFF);
}

void Hal_HVSP_SetPatTgVdeEnd(MS_U16 u16Val)
{
    W2BYTEMSK(REG_SCL0_76_L, u16Val, 0x0FFF);
}

void Hal_HVSP_SetPatTgVtt(MS_U16 u16Val)
{
    W2BYTEMSK(REG_SCL0_77_L, u16Val, 0x0FFF);
}

void Hal_HVSP_SetPatTgHsyncSt(MS_U16 u16Val)
{
    W2BYTEMSK(REG_SCL0_79_L, u16Val, 0x0FFF);
}

void Hal_HVSP_SetPatTgHsyncEnd(MS_U16 u16Val)
{
    W2BYTEMSK(REG_SCL0_7A_L, u16Val, 0x0FFF);
}

void Hal_HVSP_SetPatTgHfdeSt(MS_U16 u16Val)
{
    W2BYTEMSK(REG_SCL0_7B_L, u16Val, 0x0FFF);
}

void Hal_HVSP_SetPatTgHfdeEnd(MS_U16 u16Val)
{
    W2BYTEMSK(REG_SCL0_7C_L, u16Val, 0x0FFF);
}

void Hal_HVSP_SetPatTgHdeSt(MS_U16 u16Val)
{
    W2BYTEMSK(REG_SCL0_7D_L, u16Val, 0x0FFF);
}

void Hal_HVSP_SetPatTgHdeEnd(MS_U16 u16Val)
{
    W2BYTEMSK(REG_SCL0_7E_L, u16Val, 0x0FFF);
}

void Hal_HVSP_SetPatTgHtt(MS_U16 u16Val)
{
    W2BYTEMSK(REG_SCL0_7F_L, u16Val, 0x0FFF);
}


// LDC frame control
//#if ENABLE_HVSP_UNUSED_FUNCTION
void Hal_HVSP_SetLDCPathSel(MS_BOOL bEn)
{
    if(!LOCK_FB())
    {
        if(Is_Reg_Type(E_HVSP_CMD_TRIG_NONE) || Is_IP_First_Set(E_HVSP_ID_1)|| Is_CannotUseCMDQToSetReg())
        {
            W2BYTEMSK(REG_SCL0_03_L, bEn ? BIT12 : 0, BIT12);
            MsOS_SetSCLFrameDelay(bEn ? 1 : 0);
        }
        else
        {
            _Hal_HVSP_Write_Cmd(REG_SCL0_03_L, bEn ? BIT12 : 0, BIT12);
            MsOS_SetEvent_IRQ(Drv_SCLIRQ_Get_IRQ_SYNCEventID(), E_SCLIRQ_EVENT_SETDELAY);
        }
        SCL_DBG(SCL_DBG_LV_DRVHVSP()&EN_DBGMG_HVSPLEVEL_ELSE, "LDC:%hhd\n", bEn);
    }
}

MS_BOOL Hal_HVSP_GetLDCPathSel(void)
{
    MS_U16 bEn;
    bEn = R2BYTE(REG_SCL0_03_L);
    return (MS_BOOL)((bEn&BIT12)>>12);
}
MS_U16 Hal_HVSP_GetShiftMultipleByISPClk(EN_HVSP_ISPCLK_TYPE enISPClkType)
{
    MS_U16 u16DspH;
    switch(enISPClkType)
    {
        case EN_HVSP_ISPCLK_123M:
            u16DspH = 8;//40%
            break;
        case EN_HVSP_ISPCLK_86M:
            u16DspH = 6;//30%
            break;
        case EN_HVSP_ISPCLK_72M:
            u16DspH = 5; //25 %
            break;
        case EN_HVSP_ISPCLK_54M:
            u16DspH = 4; //20%
            break;
        case EN_HVSP_ISPCLK_144M:
            u16DspH = 9; //45%
            break;
        case EN_HVSP_ISPCLK_OFF:
        default:
           u16DspH =  0;
                break;
    }
    return u16DspH;
}

MS_U16 Hal_HVSP_GetLimitationByISPClk(EN_HVSP_ISPCLK_TYPE enISPClkType, MS_U16 u16SrcWidth, MS_U16 u16DspHeight)
{
    MS_U16 u16CropHeightLim;
    if (u16SrcWidth<=1920)//for LDC issue
    {
        switch(enISPClkType)
        {
            case EN_HVSP_ISPCLK_123M:
                if(gstHalSrcSize.u16Height*gstHalSrcSize.u16Width >= 4500000 &&gstHalSrcSize.bSet)
                {
                    u16CropHeightLim = u16DspHeight;//92% 1.1X
                }
                else
                {
                    u16CropHeightLim = (u16DspHeight/20)*15;//75% 1.33X
                }
                break;
            case EN_HVSP_ISPCLK_86M:
                u16CropHeightLim = (u16DspHeight/20)*10;//50%
                break;
            case EN_HVSP_ISPCLK_72M:
                u16CropHeightLim = (u16DspHeight/3); //33 %
                break;
            case EN_HVSP_ISPCLK_54M:
                u16CropHeightLim = u16DspHeight/3; //33%
                break;
            case EN_HVSP_ISPCLK_144M:
                if(gstHalSrcSize.u16Height*gstHalSrcSize.u16Width >= 4500000 &&gstHalSrcSize.bSet)
                {
                    u16CropHeightLim = u16DspHeight;//90% 1.1X
                }
                else
                {
                    u16CropHeightLim = (u16DspHeight/20)*15;//75% 1.33X
                }
                break;
            case EN_HVSP_ISPCLK_OFF:
            default:
               u16CropHeightLim =  u16DspHeight;
                    break;
        }
    }
    else
    {
        if(gstHalSrcSize.u16Height*gstHalSrcSize.u16Width >= 4500000 &&gstHalSrcSize.bSet)
        {
            u16CropHeightLim = u16DspHeight;//90% 1.1X
        }
        else
        {
            u16CropHeightLim = (u16DspHeight/20)*15;//75% 1.33X
        }
    }
    return u16CropHeightLim;
}

EN_HVSP_ISPCLK_TYPE Hal_HVSP_GetISPClkType(void)
{
    MS_U16 u16clk;
    EN_HVSP_ISPCLK_TYPE enISPClkType;
    u16clk = (R2BYTE(REG_SCL_CLK_61_L)&0x1F00)>>8;
    switch(u16clk)
    {
        case EN_HVSP_ISPCLK_123M:
            enISPClkType = EN_HVSP_ISPCLK_123M;
            break;
        case EN_HVSP_ISPCLK_OFF:
            enISPClkType = EN_HVSP_ISPCLK_OFF;
            break;
        case EN_HVSP_ISPCLK_86M:
            enISPClkType = EN_HVSP_ISPCLK_86M;
            break;
        case EN_HVSP_ISPCLK_72M:
            enISPClkType = EN_HVSP_ISPCLK_72M;
            break;
        case EN_HVSP_ISPCLK_54M:
            enISPClkType = EN_HVSP_ISPCLK_54M;
            break;
        case EN_HVSP_ISPCLK_144M:
            enISPClkType = EN_HVSP_ISPCLK_144M;
            break;
        default:
           enISPClkType =  EN_HVSP_ISPCLK_NUM;
                break;
    }
    return enISPClkType;
}

void Hal_HVSP_SetLDCBypass(MS_BOOL bEn)
{
    W2BYTEMSK(REG_SCL_LDC_0F_L, bEn ? BIT0 : 0, BIT0);
}
//#endif

void Hal_HVSP_SetLDCWidth(MS_U16 u16Width)
{
    if(Is_Reg_Type(E_HVSP_CMD_TRIG_NONE) || Is_IP_First_Set(E_HVSP_ID_1)|| Is_CannotUseCMDQToSetReg())
    {
        W2BYTEMSK(REG_SCL_LDC_03_L, u16Width, 0x0FFF);
    }
    else
    {
        _Hal_HVSP_Write_Cmd(REG_SCL_LDC_03_L, u16Width, 0x0FFF);
    }
}

void Hal_HVSP_SetLDCHeight(MS_U16 u16Height)
{
    if(Is_Reg_Type(E_HVSP_CMD_TRIG_NONE) || Is_IP_First_Set(E_HVSP_ID_1)|| Is_CannotUseCMDQToSetReg())
    {
        W2BYTEMSK(REG_SCL_LDC_04_L, u16Height, 0x0FFF);
    }
    else
    {
        _Hal_HVSP_Write_Cmd(REG_SCL_LDC_04_L, u16Height, 0x0FFF);
    }
}

void Hal_HVSP_SetLDCBase(MS_U32 u32Base)
{
    W4BYTE(REG_SCL_LDC_05_L, u32Base);
}

void Hal_HVSP_SetLDCHWrwDiff(MS_U16 u8Val)
{
    W2BYTEMSK(REG_SCL_LDC_11_L, ((MS_U16)u8Val), BIT1|BIT0);
}
void Hal_HVSP_SetLDCSW_Idx(MS_U8 idx)
{
    W2BYTEMSK(REG_SCL_LDC_10_L, ((MS_U16)idx), BIT1|BIT0);
}
void Hal_HVSP_SetLDCSW_Mode(MS_U8 bEn)
{
    W2BYTEMSK(REG_SCL_LDC_12_L, ((MS_U16)bEn)? BIT0 :0, BIT0);
}
void Hal_HVSP_SetLDCBank_Mode(EN_DRV_HVSP_LDCLCBANKMODE_TYPE enType)
{
    W2BYTEMSK(REG_SCL_LDC_09_L, ((MS_U16)enType == EN_DRV_HVSP_LDCLCBANKMODE_64)? BIT0 :0, BIT0);
}

#if ENABLE_HVSP_UNUSED_FUNCTION
void Hal_HVSP_SetLDCPitch(MS_U32 u32Pitch)
{
    W4BYTE(REG_SCL_LDC_07_L, u32Pitch);
}


void Hal_HVSP_SetLDCDmap_Base(MS_U32 u32Base)
{
    W4BYTE(REG_SCL_LDC_0A_L, u32Base);
}

void Hal_HVSP_SetLDCDmap_Pitch(MS_U32 u32Pitch)
{
    W4BYTE(REG_SCL_LDC_0C_L, u32Pitch);
}
#endif
void Hal_HVSP_SetOSDLocate(EN_HVSP_ID_TYPE enID,EN_DRV_HVSP_OSD_LOC_TYPE enLoc)
{
    if(enID == E_HVSP_ID_2)
    {
        W2BYTEMSK(REG_SCL2_64_L, (enLoc==EN_DRV_HVSP_OSD_LOC_BEFORE) ? BIT1: 0, BIT1);
    }
    else if(enID == E_HVSP_ID_3)
    {
        W2BYTEMSK(REG_SCL2_68_L, (enLoc==EN_DRV_HVSP_OSD_LOC_BEFORE) ? BIT1: 0, BIT1);
    }
    else
    {
        W2BYTEMSK(REG_SCL2_60_L, (enLoc==EN_DRV_HVSP_OSD_LOC_BEFORE) ? BIT1: 0, BIT1);
    }
}
void Hal_HVSP_SetOSDOnOff(EN_HVSP_ID_TYPE enID,MS_BOOL bEn)
{
    if(enID == E_HVSP_ID_2)
    {
        W2BYTEMSK(REG_SCL2_64_L, bEn? BIT0: 0, BIT0);
    }
    else if(enID == E_HVSP_ID_3)
    {
        W2BYTEMSK(REG_SCL2_68_L, bEn? BIT0: 0, BIT0);
    }
    else
    {
        W2BYTEMSK(REG_SCL2_60_L, bEn? BIT0: 0, BIT0);
    }
}
void Hal_HVSP_SetOSDbypass(EN_HVSP_ID_TYPE enID,MS_BOOL bEn)
{
    if(enID == E_HVSP_ID_2)
    {
        W2BYTEMSK(REG_SCL2_67_L, bEn? BIT7: 0, BIT7);
    }
    else if(enID == E_HVSP_ID_3)
    {
        W2BYTEMSK(REG_SCL2_6B_L, bEn? BIT7: 0, BIT7);
    }
    else
    {
        W2BYTEMSK(REG_SCL2_63_L, bEn? BIT7: 0, BIT7);
    }
}
void Hal_HVSP_SetOSDbypassWTM(EN_HVSP_ID_TYPE enID,MS_BOOL bEn)
{
    if(enID == E_HVSP_ID_2)
    {
        W2BYTEMSK(REG_SCL2_64_L, bEn? BIT7: 0, BIT7);
    }
    else if(enID == E_HVSP_ID_3)
    {
        W2BYTEMSK(REG_SCL2_68_L, bEn? BIT7: 0, BIT7);
    }
    else
    {
        W2BYTEMSK(REG_SCL2_60_L, bEn? BIT7: 0, BIT7);
    }
}
EN_DRV_HVSP_OSD_LOC_TYPE Hal_HVSP_GetOSDLocate(EN_HVSP_ID_TYPE enID)
{
    MS_U16 u16En = 0;
    if(enID == E_HVSP_ID_2)
    {
        u16En = (R2BYTE(REG_SCL2_64_L)& BIT1)>>1;
    }
    else if(enID == E_HVSP_ID_3)
    {
        u16En = (R2BYTE(REG_SCL2_68_L)& BIT1)>>1;
    }
    else
    {
        u16En = (R2BYTE(REG_SCL2_60_L)& BIT1)>>1;
    }
    return u16En ? EN_DRV_HVSP_OSD_LOC_BEFORE : EN_DRV_HVSP_OSD_LOC_AFTER;
}
MS_BOOL Hal_HVSP_GetOSDOnOff(EN_HVSP_ID_TYPE enID)
{
    MS_BOOL bEn = 0;
    if(enID == E_HVSP_ID_2)
    {
        bEn = R2BYTE(REG_SCL2_64_L)& BIT0;
    }
    else if(enID == E_HVSP_ID_3)
    {
        bEn = R2BYTE(REG_SCL2_68_L)& BIT0;
    }
    else
    {
        bEn = R2BYTE(REG_SCL2_60_L)& BIT0;
    }
    return bEn;
}
MS_BOOL Hal_HVSP_GetOSDbypass(EN_HVSP_ID_TYPE enID)
{
    MS_BOOL bEn;
    if(enID == E_HVSP_ID_2)
    {
        bEn = (R2BYTE(REG_SCL2_67_L)& BIT7)>>7;
    }
    else if(enID == E_HVSP_ID_3)
    {
        bEn = (R2BYTE(REG_SCL2_6B_L)& BIT7)>>7;
    }
    else
    {
        bEn = (R2BYTE(REG_SCL2_63_L)& BIT7)>>7;
    }
    return bEn;
}
MS_BOOL Hal_HVSP_GetOSDbypassWTM(EN_HVSP_ID_TYPE enID)
{
    MS_BOOL bEn;
    if(enID == E_HVSP_ID_2)
    {
        bEn = (R2BYTE(REG_SCL2_64_L)& BIT7)>>7;
    }
    else if(enID == E_HVSP_ID_3)
    {
        bEn = (R2BYTE(REG_SCL2_68_L)& BIT7)>>7;
    }
    else
    {
        bEn = (R2BYTE(REG_SCL2_60_L)& BIT7)>>7;
    }
    return bEn;
}

void Hal_HVSP_Set_Reg(MS_U32 u32Reg, MS_U8 u8Val, MS_U8 u8Mask)
{
    WBYTEMSK(u32Reg, u8Val, u8Mask);
}
void Hal_HVSP_Set_2ByteReg(MS_U32 u32Reg, MS_U16 u16Val, MS_U16 u16Mask)
{
    WBYTEMSK(u32Reg, u16Val, u16Mask);
}
MS_U16 Hal_HVSP_Get_2ByteReg(MS_U32 u32Reg)
{
    MS_U16 u16Val;
    u16Val = R2BYTE(u32Reg);
    return u16Val;
}

MS_U16 Hal_HVSP_Get_Crop_X(void)
{
    MS_U16 u16crop1;
    u16crop1 = R2BYTE(REG_SCL2_53_L);
    return u16crop1;
}

MS_U16 Hal_HVSP_Get_Crop_Y(void)
{
    MS_U16 u16crop1;
    u16crop1 = R2BYTE(REG_SCL2_55_L);
    return u16crop1;
}
MS_U16 Hal_HVSP_GetCrop2Xinfo(void)
{
    MS_U16 u16crop2;
    u16crop2 = R2BYTE(REG_SCL2_5B_L);
    return u16crop2;
}

MS_U16 Hal_HVSP_GetCrop2Yinfo(void)
{
    MS_U16 u16crop2;
    u16crop2 = R2BYTE(REG_SCL2_5D_L);
    return u16crop2;
}

MS_U16 Hal_HVSP_GetCrop1WidthCount(void)
{
    MS_U16 u16crop1;
    u16crop1 = R2BYTE(REG_SCL2_70_L);
    return u16crop1;
}
MS_U16 Hal_HVSP_GetCrop1HeightCount(void)
{
    MS_U16 u16crop1;
    u16crop1 = R2BYTE(REG_SCL2_71_L);
    return u16crop1;
}
MS_U16 Hal_HVSP_Get_Crop2_CountH(void)
{
    MS_U16 u16crop1;
    u16crop1 = R2BYTE(REG_SCL2_72_L);
    return u16crop1;
}
MS_U16 Hal_HVSP_Get_Crop2_CountV(void)
{
    MS_U16 u16crop1;
    u16crop1 = R2BYTE(REG_SCL2_73_L);
    return u16crop1;
}
MS_U16 Hal_HVSP_GetCrop1Width(void)
{
    MS_U16 u16crop1;
    u16crop1 = R2BYTE(REG_SCL2_51_L);
    return u16crop1;
}
MS_U16 Hal_HVSP_GetCrop1Height(void)
{
    MS_U16 u16crop1;
    u16crop1 = R2BYTE(REG_SCL2_52_L);
    return u16crop1;
}
MS_U16 Hal_HVSP_GetDMAHeight(EN_HVSP_MONITOR_TYPE enMonitorType)
{
    MS_U16 u16def;

    switch(enMonitorType)
        {
        case EN_HVSP_MONITOR_DMA1FRMCHECK:
            u16def = R2BYTE(REG_SCL_DMA0_1B_L);
            break;

        case EN_HVSP_MONITOR_DMA1SNPCHECK:
            u16def = R2BYTE(REG_SCL_DMA0_4B_L);
            break;

        case EN_HVSP_MONITOR_DMA2FRMCHECK:
            u16def = R2BYTE(REG_SCL_DMA0_63_L);
            break;
        case EN_HVSP_MONITOR_DMA3FRMCHECK:
            u16def = R2BYTE(REG_SCL_DMA1_1B_L);
            break;
        default:
            u16def = 0x0;
            break;
        }
    return ((u16def ));
}
MS_U16 Hal_HVSP_GetDMAHeightCount(EN_HVSP_MONITOR_TYPE enMonitorType)
{
    MS_U16 u16Idx;

    switch(enMonitorType)
        {
        case EN_HVSP_MONITOR_DMA1FRMCHECK:
            u16Idx = R2BYTE(REG_SCL_DMA2_10_L);
            break;

        case EN_HVSP_MONITOR_DMA1SNPCHECK:
            u16Idx = R2BYTE(REG_SCL_DMA2_12_L);
            break;

        case EN_HVSP_MONITOR_DMA2FRMCHECK:
            u16Idx = R2BYTE(REG_SCL_DMA2_13_L);
            break;
        case EN_HVSP_MONITOR_DMA3FRMCHECK:
            u16Idx = R2BYTE(REG_SCL_DMA2_15_L);
            break;
        default:
            u16Idx = 0x0;
            break;
        }
    return ((u16Idx));
}

MS_U16 Hal_HVSP_GetDMAEn(EN_HVSP_MONITOR_TYPE enMonitorType)
{
    MS_U16 bEn;
    switch(enMonitorType)
    {
    case EN_HVSP_MONITOR_DMA1FRMCHECK:
        bEn = R2BYTE(REG_SCL_DMA0_01_L);
        bEn = (bEn & 0x8000)>>15;
        break;

    case EN_HVSP_MONITOR_DMA1SNPCHECK:
        bEn = R2BYTE(REG_SCL_DMA0_03_L);
        bEn = (bEn & 0x8000)>>15;
        break;

    case EN_HVSP_MONITOR_DMA2FRMCHECK:
        bEn = R2BYTE(REG_SCL_DMA0_04_L);
        bEn = (bEn & 0x8000)>>15;
        break;
    case EN_HVSP_MONITOR_DMA3FRMCHECK:
        bEn = R2BYTE(REG_SCL_DMA1_01_L);
        bEn = (bEn & 0x8000)>>15;
        break;
    default:
        bEn = 0x0;
        break;
    }
    return bEn;
}
MS_U16 Hal_HVSP_GetHVSPInputWidth(EN_HVSP_ID_TYPE enID)
{
    MS_U16 u16size;
    if(enID == E_HVSP_ID_2)
    {
        u16size = R2BYTE(REG_SCL_HVSP1_20_L);
    }
    else if(enID == E_HVSP_ID_3)
    {
        u16size = R2BYTE(REG_SCL_HVSP2_20_L);
    }
    else
    {
        u16size = R2BYTE(REG_SCL_HVSP0_20_L);
    }
    return u16size;
}

MS_U16 Hal_HVSP_GetHVSPOutputWidth(EN_HVSP_ID_TYPE enID)
{
    MS_U16 u16size;
    if(enID == E_HVSP_ID_2)
    {
        u16size = R2BYTE(REG_SCL_HVSP1_22_L);
    }
    else if(enID == E_HVSP_ID_3)
    {
        u16size = R2BYTE(REG_SCL_HVSP2_22_L);
    }
    else
    {
        u16size = R2BYTE(REG_SCL_HVSP0_22_L);
    }
    return u16size;
}

MS_U16 Hal_HVSP_GetCrop2InputWidth(void)
{
    MS_U16 u16size;
    u16size = R2BYTE(REG_SCL2_59_L);
    return u16size;
}

MS_U16 Hal_HVSP_GetCrop2InputHeight(void)
{
    MS_U16 u16size;
    u16size = R2BYTE(REG_SCL2_5A_L);
    return u16size;
}
MS_U16 Hal_HVSP_GetCrop2OutputWidth(void)
{
    MS_U16 u16size;
    u16size = R2BYTE(REG_SCL2_5C_L);
    return u16size;
}

MS_U16 Hal_HVSP_GetCrop2OutputHeight(void)
{
    MS_U16 u16size;
    u16size = R2BYTE(REG_SCL2_5E_L);
    return u16size;
}
MS_U16 Hal_HVSP_GetCrop1En(void)
{
    MS_U16 u16size;
    u16size = (R2BYTE(REG_SCL2_50_L) & BIT0);
    return u16size;
}
MS_U16 Hal_HVSP_GetCrop2En(void)
{
    MS_U16 u16size;
    u16size = (R2BYTE(REG_SCL2_58_L) & BIT0);
    return u16size;
}
MS_U16 Hal_HVSP_GetHVSPOutputHeight(EN_HVSP_ID_TYPE enID)
{
    MS_U16 u16size;
    if(enID == E_HVSP_ID_2)
    {
        u16size = R2BYTE(REG_SCL_HVSP1_23_L);
    }
    else if(enID == E_HVSP_ID_3)
    {
        u16size = R2BYTE(REG_SCL_HVSP2_23_L);
    }
    else
    {
        u16size = R2BYTE(REG_SCL_HVSP0_23_L);
    }
    return u16size;
}
MS_U16 Hal_HVSP_GetHVSPInputHeight(EN_HVSP_ID_TYPE enID)
{
    MS_U16 u16size;
    if(enID == E_HVSP_ID_2)
    {
        u16size = R2BYTE(REG_SCL_HVSP1_21_L);
    }
    else if(enID == E_HVSP_ID_3)
    {
        u16size = R2BYTE(REG_SCL_HVSP2_21_L);
    }
    else
    {
        u16size = R2BYTE(REG_SCL_HVSP0_21_L);
    }
    return u16size;
}
MS_U16 Hal_HVSP_GetScalingFunctionStatus(EN_HVSP_ID_TYPE enID)
{
    MS_U16 u16size;
    if(enID == E_HVSP_ID_2)
    {
        u16size = (R2BYTE(REG_SCL_HVSP1_08_L) & BIT8)>>8;
        u16size |= (R2BYTE(REG_SCL_HVSP1_0A_L)& BIT8)>>7;
    }
    else if(enID == E_HVSP_ID_3)
    {
        u16size = (R2BYTE(REG_SCL_HVSP2_08_L) & BIT8)>>8;
        u16size |= (R2BYTE(REG_SCL_HVSP2_0A_L)& BIT8)>>7;
    }
    else
    {
        u16size = (R2BYTE(REG_SCL_HVSP0_08_L) & BIT8)>>8;
        u16size |= (R2BYTE(REG_SCL_HVSP0_0A_L)& BIT8)>>7;
    }
    return u16size;
}
MS_U16 Hal_HVSP_GetHVSPOutputHeightCount(EN_HVSP_ID_TYPE enID)
{
    MS_U16 u16size;
    if(enID == E_HVSP_ID_2)
    {
        u16size = R2BYTE(REG_SCL_HVSP1_25_L);
    }
    else if(enID == E_HVSP_ID_3)
    {
        u16size = R2BYTE(REG_SCL_HVSP2_26_L);
    }
    else
    {
        u16size = R2BYTE(REG_SCL_HVSP0_26_L);
    }
    return u16size;
}
MS_U32 Hal_HVSP_GetCMDQStatus(void)
{
    return R2BYTE(0x11208A);
}
void Hal_HVSP_PriMask_ColorY(MS_U16 u16Y)
{
    W2BYTE(REG_VIP_SCNR_12_L, u16Y);
}
void Hal_HVSP_PriMask_ColorU(MS_U16 u16U)
{
    W2BYTE(REG_VIP_SCNR_13_L, u16U);
}
void Hal_HVSP_PriMask_ColorV(MS_U16 u16V)
{
    W2BYTE(REG_VIP_SCNR_14_L, u16V);
}

void Hal_HVSP_SetPrv2CropOnOff(MS_BOOL bEn)
{
    if(!LOCK_FB())
    {
        if(Is_Reg_Type(E_HVSP_CMD_TRIG_NONE) || Is_IP_First_Set(E_HVSP_ID_1)|| Is_CannotUseCMDQToSetReg())
        {
            W2BYTEMSK(REG_SCL_DNR1_01_L, bEn ? BIT5 : 0, BIT5);
        }
        else
        {
            _Hal_HVSP_Write_Cmd(REG_SCL_DNR1_01_L, bEn ? BIT5 : 0, BIT5);
        }
        MsOS_SetSCLFrameDelay(bEn ? 1 :0);
        if(bEn)
        {
            if(Is_Reg_Type(E_HVSP_CMD_TRIG_NONE) || Is_IP_First_Set(E_HVSP_ID_1)|| Is_CannotUseCMDQToSetReg())
            {
                W2BYTEMSK(REG_VIP_MCNR_01_L,BIT0, BIT0);
                if(!(R2BYTE(REG_SCL_DNR1_01_L)&BIT0))
                {
                    W2BYTEMSK(REG_SCL_DNR1_01_L, BIT0 , BIT0);
                }
            }
            else
            {
                _Hal_HVSP_Write_Cmd(REG_VIP_MCNR_01_L,BIT0, BIT0);
                if(!(R2BYTE(REG_SCL_DNR1_01_L)&BIT0))
                {
                    _Hal_HVSP_Write_Cmd(REG_SCL_DNR1_01_L,BIT0, BIT0);
                }
            }
        }
    }

}
MS_BOOL Hal_HVSP_GetPrv2CropOnOff(void)
{
    MS_U16 u16Reg;
    u16Reg = R2BYTE(REG_SCL_DNR1_01_L);
    return ((u16Reg &BIT5)>>5);
}

void Hal_HVSP_PriMask_bEn(MS_BOOL bEn)
{
    MS_U32 u32Events;
    MS_CMDQ_CMDReg stCfg;
    MsOS_Memset(&stCfg,0,sizeof(MS_CMDQ_CMDReg));
    if(Is_Reg_Type(E_HVSP_CMD_TRIG_NONE) || Is_IP_First_Set(E_HVSP_ID_1) || Is_CannotUseCMDQToSetReg())
    {
        W2BYTEMSK(REG_VIP_SCNR_10_L, bEn ? BIT0 : 0, BIT0);
    }
    else if(bEn == EN_HVSP_TRIGGER_OFF)
    {
        if(Drv_SCLIRQ_GetEachDMAEn()&& VIPSETRULE())
        {
            Drv_CMDQ_GetModuleMutex(EN_CMDQ_TYPE_IP0,1);
            Drv_CMDQ_FillCmd(&stCfg, REG_VIP_SCNR_10_L, 0, BIT0);
            Drv_CMDQ_WriteCmd(EN_CMDQ_TYPE_IP0,&stCfg,1);
            Drv_CMDQ_Fire(EN_CMDQ_TYPE_IP0,TRUE);
            Drv_CMDQ_GetModuleMutex(EN_CMDQ_TYPE_IP0,0);
        }
        else
        {
            if(Drv_SCLIRQ_GetEachDMAEn()||(!Drv_SCLIRQ_GetIsBlankingRegion()))
            {
                MsOS_WaitEvent(Drv_SCLIRQ_Get_IRQ_SYNCEventID(),
                    (E_SCLIRQ_EVENT_FRMENDSYNC), &u32Events, E_AND, 2000); // get status: FRM END
            }
            W2BYTEMSK(REG_VIP_SCNR_10_L,0, BIT0);
        }
    }
    else if(bEn == EN_HVSP_TRIGGER_ON)
    {
        if(Drv_SCLIRQ_GetEachDMAEn()&& VIPSETRULE())
        {
            Drv_CMDQ_GetModuleMutex(EN_CMDQ_TYPE_IP0,1);
            Drv_CMDQ_FillCmd(&stCfg, REG_VIP_SCNR_10_L, BIT0, BIT0);
            Drv_CMDQ_WriteCmd(EN_CMDQ_TYPE_IP0,&stCfg,1);
            Drv_CMDQ_Fire(EN_CMDQ_TYPE_IP0,TRUE);
            Drv_CMDQ_GetModuleMutex(EN_CMDQ_TYPE_IP0,0);
        }
        else
        {
            if(Drv_SCLIRQ_GetEachDMAEn()||(!Drv_SCLIRQ_GetIsBlankingRegion()))
            {
                MsOS_WaitEvent(Drv_SCLIRQ_Get_IRQ_SYNCEventID(),
                    (E_SCLIRQ_EVENT_FRMENDSYNC), &u32Events, E_AND, 2000); // get status: FRM END
            }
            W2BYTEMSK(REG_VIP_SCNR_10_L,BIT0, BIT0);
        }
    }
    else if(bEn == EN_HVSP_TRIGGER_CMDQ)
    {
        _Hal_HVSP_Write_Cmd(REG_VIP_SCNR_10_L,BIT0, BIT0);
    }
}
void Hal_HVSP_PriMask_SRAMEn(MS_BOOL bEn,MS_U8 u8Idx)
{
    MS_U32 u32Events;
    MS_CMDQ_CMDReg stCfg;
    MsOS_Memset(&stCfg,0,sizeof(MS_CMDQ_CMDReg));
    if(Drv_SCLIRQ_GetEachDMAEn()&& VIPSETRULE())
    {
        if(bEn)
        {
            Drv_CMDQ_FillCmd(&stCfg, REG_VIP_SCNR_11_L, ((MS_U16)(u8Idx<<8)| 0x1 ), 0xFF01);
            Drv_CMDQ_WriteCmd(EN_CMDQ_TYPE_IP0,&stCfg,1);
        }
        else
        {
            Drv_CMDQ_FillCmd(&stCfg, REG_VIP_SCNR_11_L, ((MS_U16)(u8Idx<<8)), 0xFF01);
            Drv_CMDQ_WriteCmd(EN_CMDQ_TYPE_IP0,&stCfg,1);
        }
    }
    else
    {
        if(Drv_SCLIRQ_GetEachDMAEn()||(!Drv_SCLIRQ_GetIsBlankingRegion()))
        {
            MsOS_WaitEvent(Drv_SCLIRQ_Get_IRQ_SYNCEventID(),
                (E_SCLIRQ_EVENT_FRMENDSYNC), &u32Events, E_AND, 2000); // get status: FRM END
        }
        W2BYTEMSK(REG_VIP_SCNR_11_L, (MS_U16)(u8Idx<<8), 0xFF00);
        W2BYTEMSK(REG_VIP_SCNR_11_L, bEn ? BIT0 : 0, BIT0);
    }
}
void Hal_HVSP_PriMask_Fire(void)
{
    if(VIPSETRULE())
    {
        Drv_CMDQ_Fire(EN_CMDQ_TYPE_IP0,TRUE);
    }
}

void Hal_HVSP_PriMask_SRAMOffsetRe(MS_BOOL bEn)
{
    W2BYTEMSK(REG_VIP_SCNR_11_L, bEn ? BIT1 : 0, BIT1);
}
void Hal_HVSP_PriMask_SRAMRdIdx(MS_U8 u8Idx)
{
    W2BYTEMSK(REG_VIP_SCNR_11_L, u8Idx<<5, BIT5|BIT6|BIT7);
}
void Hal_HVSP_PriMask_SRAMWriteIdx(MS_U8 u8Idx)
{
    W2BYTEMSK(REG_VIP_SCNR_11_L, (MS_U16)(u8Idx<<8), 0xFF00);
}
void Hal_HVSP_PriMaskSetSRAM(MS_U8 u8idx,MS_U16 u16val)
{
    MS_U32 u32Events;
    MS_CMDQ_CMDReg stCfg;
    MS_U32 u32reg = (u8idx == 0) ? REG_VIP_SCNR_17_L :
                    (u8idx == 1) ? REG_VIP_SCNR_18_L :
                    (u8idx == 2) ? REG_VIP_SCNR_19_L :
                    (u8idx == 3) ? REG_VIP_SCNR_1A_L :
                    (u8idx == 4) ? REG_VIP_SCNR_1B_L :
                    (u8idx == 5) ? REG_VIP_SCNR_1C_L :
                                   REG_VIP_SCNR_17_L;
    MsOS_Memset(&stCfg,0,sizeof(MS_CMDQ_CMDReg));
    if(Drv_SCLIRQ_GetEachDMAEn()&& VIPSETRULE())
    {
        Drv_CMDQ_FillCmd(&stCfg, u32reg, u16val, 0xFFFF);
        Drv_CMDQ_WriteCmd(EN_CMDQ_TYPE_IP0,&stCfg,1);
    }
    else
    {
        if(Drv_SCLIRQ_GetEachDMAEn()||(!Drv_SCLIRQ_GetIsBlankingRegion()))
        {
            MsOS_WaitEvent(Drv_SCLIRQ_Get_IRQ_SYNCEventID(),
                (E_SCLIRQ_EVENT_FRMENDSYNC), &u32Events, E_AND, 2000); // get status: FRM END
        }
        W2BYTE(u32reg, u16val);
    }

}
MS_U16 Hal_HVSP_PriMask_GetSRAMData(void)
{
    return R2BYTE(REG_VIP_SCNR_1D_L);
}
void Hal_HVSP_SetWriteRegType(MS_BOOL bEn)
{
    if(bEn && (VIPSETRULE()==E_VIPSetRule_CMDQAll || VIPSETRULE()==E_VIPSetRule_CMDQAllCheck
        ||(VIPSETRULE() == E_VIPSetRule_CMDQAllONLYSRAMCheck)))
    {
        Drv_CMDQ_GetModuleMutex(EN_CMDQ_TYPE_IP0,1);
        Drv_CMDQ_SetForceSkip(1);
        if(Drv_SCLIRQ_GetEachDMAEn())
        {
            gbSramCMDQ = 1;
        }
        else
        {
            gbSramCMDQ = 0;
        }
    }
    else if(bEn && VIPSETRULE())
    {
        Drv_CMDQ_GetModuleMutex(EN_CMDQ_TYPE_IP0,1);
        Drv_CMDQ_SetForceSkip(1);
        if(Drv_SCLIRQ_GetEachDMAEn()&& !Drv_SCLIRQ_GetIsBlankingRegion())
        {
            gbSramCMDQ = 1;
        }
        else
        {
            gbSramCMDQ = 0;
        }
    }
    else
    {
        gbSramCMDQ = 0;
        Drv_CMDQ_SetForceSkip(0);
        Drv_CMDQ_GetModuleMutex(EN_CMDQ_TYPE_IP0,0);
    }
}
void Hal_HVSP_WriteReg(MS_U32 u32Reg,MS_U16 u16Val,MS_U16 u16Mask)
{
    MS_CMDQ_CMDReg stCfg;
    MsOS_Memset(&stCfg,0,sizeof(MS_CMDQ_CMDReg));
    if(gbSramCMDQ)
    {
        Drv_CMDQ_FillCmd(&stCfg, u32Reg, u16Val, u16Mask);
        Drv_CMDQ_WriteCmd(EN_CMDQ_TYPE_IP0, &stCfg, 1);
    }
    else
    {
        W2BYTEMSK(u32Reg, u16Val, u16Mask);//sec
    }
}
void Hal_HVSP_SRAMDumpHSP(MS_U32 u32Sram,MS_U32 u32reg)
{
    MS_U16 u16addr = 0,u16tvalue = 0,u16tcount = 0,u16readdata;
    //clear
    for(u16addr = 64;u16addr<128;u16addr++)
    {
        Hal_HVSP_WriteReg(u32reg, BIT0, BIT0);//Yenable
        Hal_HVSP_WriteReg(u32reg+2, u16addr, 0xFF);
        Hal_HVSP_WriteReg(u32reg+4, 0, 0xFFFF);
        Hal_HVSP_WriteReg(u32reg+6, 0, 0xFFFF);
        Hal_HVSP_WriteReg(u32reg+8, 0, 0xFF);
        Hal_HVSP_WriteReg(u32reg, BIT0|BIT8, BIT0|BIT8);//W pulse
        Hal_HVSP_WriteReg(u32reg, 0, BIT0);//Yenable

        Hal_HVSP_WriteReg(u32reg, BIT1, BIT1);//Cenable
        Hal_HVSP_WriteReg(u32reg+4, 0, 0xFFFF);
        Hal_HVSP_WriteReg(u32reg+6, 0, 0xFFFF);
        Hal_HVSP_WriteReg(u32reg+8, 0, 0xFF);
        Hal_HVSP_WriteReg(u32reg, BIT1|BIT8, BIT1|BIT8);//W pulse
        Hal_HVSP_WriteReg(u32reg, 0, BIT1);//Cenable
    }
    //SRAM 0 //0~64 entry is V  65~127 is H

    for(u16addr=0;u16addr<(PQ_IP_SRAM1_SIZE_Main/5);u16addr++)
    {
           // 64~127
        Hal_HVSP_WriteReg(u32reg, BIT0, BIT0);//Yenable
        Hal_HVSP_WriteReg(u32reg+2, u16addr+64, 0xFF);
        if(u32Sram&0x1)
        {
            u16tvalue = MST_SRAM1_Main[((u32Sram&0xF0)>>4)][u16tcount] |
                (MST_SRAM1_Main[((u32Sram&0xF0)>>4)][u16tcount+1]<<8);
        }
        else
        {
            u16tvalue = MST_SRAM2_Main[((u32Sram&0xF0)>>4)][u16tcount] |
                (MST_SRAM2_Main[((u32Sram&0xF0)>>4)][u16tcount+1]<<8);
        }
        SCL_DBG(SCL_DBG_LV_DRVHVSP()&EN_DBGMG_HVSPLEVEL_ELSE, "[HVSP]VYtval12:%hx\n", u16tvalue);
        Hal_HVSP_WriteReg(u32reg+4, u16tvalue, 0xFFFF);
        if(u32Sram&0x1)
        {
            u16tvalue = MST_SRAM1_Main[((u32Sram&0xF0)>>4)][u16tcount+2] |
                (MST_SRAM1_Main[((u32Sram&0xF0)>>4)][u16tcount+3]<<8);
        }
        else
        {
            u16tvalue = MST_SRAM2_Main[((u32Sram&0xF0)>>4)][u16tcount+2] |
                (MST_SRAM2_Main[((u32Sram&0xF0)>>4)][u16tcount+3]<<8);
        }
        Hal_HVSP_WriteReg(u32reg+6, u16tvalue, 0xFFFF);
        SCL_DBG(SCL_DBG_LV_DRVHVSP()&EN_DBGMG_HVSPLEVEL_ELSE, "[HVSP]VYtval34:%hx\n", u16tvalue);
        if(u32Sram&0x1)
        {
            u16tvalue = MST_SRAM1_Main[((u32Sram&0xF0)>>4)][u16tcount+4];
        }
        else
        {
            u16tvalue = MST_SRAM2_Main[((u32Sram&0xF0)>>4)][u16tcount+4];
        }
        SCL_DBG(SCL_DBG_LV_DRVHVSP()&EN_DBGMG_HVSPLEVEL_ELSE, "[HVSP]Vtval5:%hx\n", u16tvalue);
        Hal_HVSP_WriteReg(u32reg+8, u16tvalue, 0xFF);
        Hal_HVSP_WriteReg(u32reg, BIT0|BIT8|BIT9, BIT0|BIT8|BIT9);//8 W pulse//9 R pulse
        //if(SCL_DBG_LV_DRVHVSP()&EN_DBGMG_HVSPLEVEL_ELSE)
        if(!gbSramCMDQ)
        {
            u16readdata = R2BYTE(u32reg+10);
            SCL_DBG(SCL_DBG_LV_DRVHVSP()&EN_DBGMG_HVSPLEVEL_ELSE, "[HVSP]Vread Yval 12:%hx\n", u16readdata);
            u16readdata = R2BYTE(u32reg+12);
            SCL_DBG(SCL_DBG_LV_DRVHVSP()&EN_DBGMG_HVSPLEVEL_ELSE, "[HVSP]Vread Yval 34:%hx\n", u16readdata);
            u16readdata = R2BYTE(u32reg+14);
            SCL_DBG(SCL_DBG_LV_DRVHVSP()&EN_DBGMG_HVSPLEVEL_ELSE, "[HVSP]Vread Yval 5:%hx\n", u16readdata);
        }
        Hal_HVSP_WriteReg(u32reg, 0, BIT0);//Yenable

        Hal_HVSP_WriteReg(u32reg, BIT1, BIT1);//Cenable
        u16tvalue = MST_C_SRAM1_Main[0][u16tcount] | (MST_C_SRAM1_Main[0][u16tcount+1]<<8);
        Hal_HVSP_WriteReg(u32reg+4, u16tvalue, 0xFFFF);
        u16tvalue = MST_C_SRAM1_Main[0][u16tcount+2] | (MST_C_SRAM1_Main[0][u16tcount+3]<<8);
        Hal_HVSP_WriteReg(u32reg+6, u16tvalue, 0xFFFF);
        u16tvalue = MST_C_SRAM1_Main[0][u16tcount+4];
        Hal_HVSP_WriteReg(u32reg+8, u16tvalue, 0xFF);
        Hal_HVSP_WriteReg(u32reg, BIT1|BIT8|BIT9, BIT1|BIT8|BIT9);//8 W pulse 9R pulse
        //if(SCL_DBG_LV_DRVHVSP()&EN_DBGMG_HVSPLEVEL_ELSE)
        if(!gbSramCMDQ)
        {
            u16readdata = R2BYTE(u32reg+10);
            SCL_DBG(SCL_DBG_LV_DRVHVSP()&EN_DBGMG_HVSPLEVEL_ELSE, "[HVSP]Vread Cval 12:%hx\n", u16readdata);
            u16readdata = R2BYTE(u32reg+12);
            SCL_DBG(SCL_DBG_LV_DRVHVSP()&EN_DBGMG_HVSPLEVEL_ELSE, "[HVSP]Vread Cval 34:%hx\n", u16readdata);
            u16readdata = R2BYTE(u32reg+14);
            SCL_DBG(SCL_DBG_LV_DRVHVSP()&EN_DBGMG_HVSPLEVEL_ELSE, "[HVSP]Vread Cval 5:%hx\n", u16readdata);
        }
        Hal_HVSP_WriteReg(u32reg, 0, BIT1);//Cenable
        u16tcount += 5;
    }
    //SRAM 1 //128~256 entry  link to 0~127 , so do not use
}
void Hal_HVSP_SRAMDumpVSP(MS_U32 u32Sram,MS_U32 u32reg)
{
    MS_U16 u16addr = 0,u16tvalue = 0,u16tcount = 0,u16readdata;
    //clear
    for(u16addr = 0;u16addr<64;u16addr++)
    {
        Hal_HVSP_WriteReg(u32reg, BIT0, BIT0);//Yenable
        Hal_HVSP_WriteReg(u32reg+2, u16addr, 0xFF);
        Hal_HVSP_WriteReg(u32reg+4, 0, 0xFFFF);
        Hal_HVSP_WriteReg(u32reg+6, 0, 0xFFFF);
        Hal_HVSP_WriteReg(u32reg+8, 0, 0xFF);
        Hal_HVSP_WriteReg(u32reg, BIT8|BIT0, BIT0|BIT8);//W pulse
        Hal_HVSP_WriteReg(u32reg, 0, BIT0);//Yenable

        Hal_HVSP_WriteReg(u32reg, BIT1, BIT1);//Cenable
        Hal_HVSP_WriteReg(u32reg+4, 0, 0xFFFF);
        Hal_HVSP_WriteReg(u32reg+6, 0, 0xFFFF);
        Hal_HVSP_WriteReg(u32reg+8, 0, 0xFF);
        Hal_HVSP_WriteReg(u32reg, BIT1|BIT8, BIT1|BIT8);//W pulse
        Hal_HVSP_WriteReg(u32reg, 0, BIT1);//Cenable
    }
    //SRAM 0 ///0~64 entry is V  65~127 is H

    for(u16addr=0;u16addr<(PQ_IP_SRAM1_SIZE_Main/5);u16addr++)
    {
        Hal_HVSP_WriteReg(u32reg, BIT0, BIT0);//Yenable
        Hal_HVSP_WriteReg(u32reg+2, u16addr, 0xFF);
        if(u32Sram&0x1)
        {
            u16tvalue = MST_SRAM1_Main[((u32Sram&0xF0)>>4)][u16tcount] |
                (MST_SRAM1_Main[((u32Sram&0xF0)>>4)][u16tcount+1]<<8);
        }
        else
        {
            u16tvalue = MST_SRAM2_Main[((u32Sram&0xF0)>>4)][u16tcount] |
                (MST_SRAM2_Main[((u32Sram&0xF0)>>4)][u16tcount+1]<<8);
        }
        Hal_HVSP_WriteReg(u32reg+4, u16tvalue, 0xFFFF);
        SCL_DBG(SCL_DBG_LV_DRVHVSP()&EN_DBGMG_HVSPLEVEL_ELSE, "[HVSP]Ytval12:%hx\n", u16tvalue);
        if(u32Sram&0x1)
        {
            u16tvalue = MST_SRAM1_Main[((u32Sram&0xF0)>>4)][u16tcount+2] |
                (MST_SRAM1_Main[((u32Sram&0xF0)>>4)][u16tcount+3]<<8);
        }
        else
        {
            u16tvalue = MST_SRAM2_Main[((u32Sram&0xF0)>>4)][u16tcount+2] |
                (MST_SRAM2_Main[((u32Sram&0xF0)>>4)][u16tcount+3]<<8);
        }
        Hal_HVSP_WriteReg(u32reg+6, u16tvalue, 0xFFFF);
        SCL_DBG(SCL_DBG_LV_DRVHVSP()&EN_DBGMG_HVSPLEVEL_ELSE, "[HVSP]Ytval34:%hx\n", u16tvalue);
        if(u32Sram&0x1)
        {
            u16tvalue = MST_SRAM1_Main[((u32Sram&0xF0)>>4)][u16tcount+4];
        }
        else
        {
            u16tvalue = MST_SRAM2_Main[((u32Sram&0xF0)>>4)][u16tcount+4];
        }
        Hal_HVSP_WriteReg(u32reg+8, u16tvalue, 0xFF);
        SCL_DBG(SCL_DBG_LV_DRVHVSP()&EN_DBGMG_HVSPLEVEL_ELSE, "[HVSP]Ytval5:%hx\n", u16tvalue);
        Hal_HVSP_WriteReg(u32reg, BIT0|BIT8|BIT9, BIT0|BIT8|BIT9);//8 W pulse 9R
        //if(SCL_DBG_LV_DRVHVSP()&EN_DBGMG_HVSPLEVEL_ELSE)
        if(!gbSramCMDQ)
        {
            u16readdata = R2BYTE(u32reg+10);
            SCL_DBG(SCL_DBG_LV_DRVHVSP()&EN_DBGMG_HVSPLEVEL_ELSE, "[HVSP]read Yval 12:%hx\n", u16readdata);
            u16readdata = R2BYTE(u32reg+12);
            SCL_DBG(SCL_DBG_LV_DRVHVSP()&EN_DBGMG_HVSPLEVEL_ELSE, "[HVSP]read Yval 34:%hx\n", u16readdata);
            u16readdata = R2BYTE(u32reg+14);
            SCL_DBG(SCL_DBG_LV_DRVHVSP()&EN_DBGMG_HVSPLEVEL_ELSE, "[HVSP]read Yval 5:%hx\n", u16readdata);
        }
        Hal_HVSP_WriteReg(u32reg, 0, BIT0);//Yenable

        Hal_HVSP_WriteReg(u32reg, BIT1, BIT1);//Cenable
        u16tvalue = MST_C_SRAM1_Main[0][u16tcount] | (MST_C_SRAM1_Main[0][u16tcount+1]<<8);
        SCL_DBG(SCL_DBG_LV_DRVHVSP()&EN_DBGMG_HVSPLEVEL_ELSE, "[HVSP]Ctval12:%hx\n", u16tvalue);
        Hal_HVSP_WriteReg(u32reg+4, u16tvalue, 0xFFFF);
        u16tvalue = MST_C_SRAM1_Main[0][u16tcount+2] | (MST_C_SRAM1_Main[0][u16tcount+3]<<8);
        SCL_DBG(SCL_DBG_LV_DRVHVSP()&EN_DBGMG_HVSPLEVEL_ELSE, "[HVSP]Ctval34:%hx\n", u16tvalue);
        Hal_HVSP_WriteReg(u32reg+6, u16tvalue, 0xFFFF);
        u16tvalue = MST_C_SRAM1_Main[0][u16tcount+4];
        SCL_DBG(SCL_DBG_LV_DRVHVSP()&EN_DBGMG_HVSPLEVEL_ELSE, "[HVSP]Ctval5:%hx\n", u16tvalue);
        Hal_HVSP_WriteReg(u32reg+8, u16tvalue, 0xFF);

        //if(SCL_DBG_LV_DRVHVSP()&EN_DBGMG_HVSPLEVEL_ELSE)
        Hal_HVSP_WriteReg(u32reg, BIT1|BIT8|BIT9, BIT1|BIT8|BIT9);//8W 9R pulse
        if(!gbSramCMDQ)
        {
            u16readdata = R2BYTE(u32reg+10);
            SCL_DBG(SCL_DBG_LV_DRVHVSP()&EN_DBGMG_HVSPLEVEL_ELSE, "[HVSP]read Cval 12:%hx\n", u16readdata);
            u16readdata = R2BYTE(u32reg+12);
            SCL_DBG(SCL_DBG_LV_DRVHVSP()&EN_DBGMG_HVSPLEVEL_ELSE, "[HVSP]read Cval 34:%hx\n", u16readdata);
            u16readdata = R2BYTE(u32reg+14);
            SCL_DBG(SCL_DBG_LV_DRVHVSP()&EN_DBGMG_HVSPLEVEL_ELSE, "[HVSP]read Cval 5:%hx\n", u16readdata);
        }
        Hal_HVSP_WriteReg(u32reg, 0, BIT1);//Cenable
        u16tcount += 5;
    }
    //SRAM 1 //128~256 entry  link to 0~127 , so do not use
}
MS_BOOL Hal_HVSP_SetSRAMDump(EN_HVSP_SRAM_DUMP_TYPE endump,MS_U32 u32Sram,MS_U32 u32reg)
{
    if(endump == EN_HVSP_SRAM_DUMP_HVSP_V || endump == EN_HVSP_SRAM_DUMP_HVSP_V_1||
        endump == EN_HVSP_SRAM_DUMP_HVSP_V_2)
    {
        Hal_HVSP_SRAMDumpVSP(u32Sram,u32reg);
    }
    else if(endump == EN_HVSP_SRAM_DUMP_HVSP_H || endump == EN_HVSP_SRAM_DUMP_HVSP_H_1||
        endump == EN_HVSP_SRAM_DUMP_HVSP_H_2)
    {
        Hal_HVSP_SRAMDumpHSP(u32Sram,u32reg);
    }
    return 1;
}
MS_BOOL Hal_HVSP_SRAM_Dump(EN_HVSP_SRAM_DUMP_TYPE endump,MS_U32 u32Sram)
{
    MS_U16 u16clkreg;
    MS_BOOL bRet;
    MS_U32 u32reg = endump == EN_HVSP_SRAM_DUMP_HVSP_V ? REG_SCL_HVSP0_41_L:
                    endump == EN_HVSP_SRAM_DUMP_HVSP_V_1 ? REG_SCL_HVSP1_41_L:
                    endump == EN_HVSP_SRAM_DUMP_HVSP_V_2 ? REG_SCL_HVSP2_41_L:
                    endump == EN_HVSP_SRAM_DUMP_HVSP_H ? REG_SCL_HVSP0_41_L:
                    endump == EN_HVSP_SRAM_DUMP_HVSP_H_1 ? REG_SCL_HVSP1_41_L:
                    endump == EN_HVSP_SRAM_DUMP_HVSP_H_2 ? REG_SCL_HVSP2_41_L:
                        0;
    //clk open
    if(endump == EN_HVSP_SRAM_DUMP_HVSP_V_2 || endump == EN_HVSP_SRAM_DUMP_HVSP_H_2)
    {
        u16clkreg = R2BYTE(REG_SCL_CLK_65_L);
        W2BYTE(REG_SCL_CLK_65_L,0x0);
    }
    else
    {
        u16clkreg = R2BYTE(REG_SCL_CLK_64_L);
        W2BYTE(REG_SCL_CLK_64_L,0x0);
        Hal_HVSP_SetWriteRegType(1);
    }
    bRet = Hal_HVSP_SetSRAMDump(endump,u32Sram,u32reg);

    //clk close
    if(endump == EN_HVSP_SRAM_DUMP_HVSP_V_2|| endump == EN_HVSP_SRAM_DUMP_HVSP_H_2)
    {
        W2BYTE(REG_SCL_CLK_65_L,u16clkreg);
    }
    else
    {
        if(gbSramCMDQ)
        {
            Drv_CMDQ_Fire(EN_CMDQ_TYPE_IP0,TRUE);
        }
        Hal_HVSP_SetWriteRegType(0);
        W2BYTE(REG_SCL_CLK_64_L,u16clkreg);
    }
    return bRet;
}

#undef HAL_HVSP_C
