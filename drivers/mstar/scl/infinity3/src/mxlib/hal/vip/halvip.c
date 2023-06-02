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
#define HAL_VIP_C

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
#include "MsOS.h"

// Internal Definition
#include "hwreg.h"
#include "drvvip.h"
#include "halvip_utility.h"
#include "halvip.h"
#include "drvCMDQ.h"
#include "drvsclirq_st.h"
#include "drvsclirq.h"
#include "drvPQ_Define.h"
#include "mdrv_scl_dbg.h"
#include "Infinity3e_Main.h"             // table config parameter
//-------------------------------------------------------------------------------------------------
//  Driver Compiler Options
//-------------------------------------------------------------------------------------------------


//-------------------------------------------------------------------------------------------------
//  Defines & Macro
//-------------------------------------------------------------------------------------------------
#define HAL_VIP_DBG(x)
#define HAL_VIP_MUTEX_LOCK()            MsOS_ObtainMutex(_HalVIP_Mutex,MSOS_WAIT_FOREVER)
#define HAL_VIP_MUTEX_UNLOCK()          MsOS_ReleaseMutex(_HalVIP_Mutex)
#define WDR_SRAM_NUM 8
#define WDR_SRAM_BYTENUM (PQ_IP_WDR_Loc_TBL_0_SRAM_SIZE_Main/2)
#define WDR_SRAM_USERBYTENUM (81)
#define GAMMAY_SRAM_BYTENUM (PQ_IP_YUV_Gamma_tblY_SRAM_SIZE_Main/4)
#define GAMMAU_SRAM_BYTENUM (PQ_IP_YUV_Gamma_tblU_SRAM_SIZE_Main/4)
#define GAMMAV_SRAM_BYTENUM (PQ_IP_YUV_Gamma_tblV_SRAM_SIZE_Main/4)
#define GAMMA10to12R_SRAM_BYTENUM (PQ_IP_ColorEng_GM10to12_Tbl_R_SRAM_SIZE_Main/4)
#define GAMMA10to12G_SRAM_BYTENUM (PQ_IP_ColorEng_GM10to12_Tbl_G_SRAM_SIZE_Main/4)
#define GAMMA10to12B_SRAM_BYTENUM (PQ_IP_ColorEng_GM10to12_Tbl_B_SRAM_SIZE_Main/4)
#define GAMMA12to10R_SRAM_BYTENUM (PQ_IP_ColorEng_GM12to10_CrcTbl_R_SRAM_SIZE_Main/4)
#define GAMMA12to10G_SRAM_BYTENUM (PQ_IP_ColorEng_GM12to10_CrcTbl_G_SRAM_SIZE_Main/4)
#define GAMMA12to10B_SRAM_BYTENUM (PQ_IP_ColorEng_GM12to10_CrcTbl_B_SRAM_SIZE_Main/4)
//-------------------------------------------------------------------------------------------------
//  Variable
//-------------------------------------------------------------------------------------------------
MS_U32 VIP_RIU_BASE = 0;
MS_BOOL gbCMDQ = 0;
MS_S32 _HalVIP_Mutex = -1;

//-------------------------------------------------------------------------------------------------
//  Functions
//-------------------------------------------------------------------------------------------------
void Hal_VIP_Exit(void)
{
    if(_HalVIP_Mutex != -1)
    {
        MsOS_DeleteMutex(_HalVIP_Mutex);
        _HalVIP_Mutex = -1;
    }
}
void Hal_VIP_Set_Riu_Base(MS_U32 u32riubase)
{
    char word[] = {"_HalVIP_Mutex"};
    VIP_RIU_BASE = u32riubase;
    _HalVIP_Mutex = MsOS_CreateMutex(E_MSOS_FIFO, word, MSOS_PROCESS_SHARED);

    if (_HalVIP_Mutex == -1)
    {
        SCL_ERR("[DRVHVSP]%s(%d): create mutex fail\n", __FUNCTION__, __LINE__);
    }
}
void Hal_VIP_SeMCNRIPMRead(MS_BOOL bEn)
{
    W2BYTEMSK(REG_SCL_DNR1_01_L, bEn ? BIT0 : 0, BIT0);//IOenable
}
void Hal_VIP_SetIPMConpress(MS_BOOL bEn)
{
    W2BYTEMSK(REG_SCL_DNR1_30_L, bEn ? BIT0 : 0, BIT0);//conpress
    W2BYTEMSK(REG_SCL_DNR1_30_L, bEn ? BIT1 : 0, BIT1);//decon
}
void Hal_VIP_SeCIIRRead(MS_BOOL bEn)
{
    W2BYTEMSK(REG_SCL_DNR1_01_L, bEn ? BIT2 : 0, BIT2);//IOenable
}
void Hal_VIP_SeCIIRWrite(MS_BOOL bEn)
{
    W2BYTEMSK(REG_SCL_DNR1_01_L, bEn ? BIT3 : 0, BIT3);//IOenable
}
void Hal_VIP_SRAMDumpIHCICC(EN_VIP_SRAM_DUMP_TYPE endump,MS_U32 u32reg)
{
    MS_U8  u8sec = 0;
    EN_VIP_SRAM_SEC_NUM enSecNum;
    MS_U16 u16addr = 0,u16tvalue = 0,u16tcount = 0,u16readdata;
    W2BYTEMSK(u32reg, BIT0, BIT0);//IOenable
    for(u8sec=0;u8sec<4;u8sec++)
    {
        switch(u8sec)
        {
            case 0:
                enSecNum = EN_VIP_SRAM_SEC_0;
                break;
            case 1:
                enSecNum = EN_VIP_SRAM_SEC_1;
                break;
            case 2:
                enSecNum = EN_VIP_SRAM_SEC_2;
                break;
            case 3:
                enSecNum = EN_VIP_SRAM_SEC_3;
                break;
        }
        W2BYTEMSK(u32reg, u8sec<<1, BIT1|BIT2);//sec
        for(u16addr=0;u16addr<enSecNum;u16addr++)
        {
            if(endump == EN_VIP_SRAM_DUMP_IHC)
            {
                u16tvalue = MST_VIP_IHC_CRD_SRAM_Main[0][u16tcount] | (MST_VIP_IHC_CRD_SRAM_Main[0][u16tcount+1]<<8);
            }
            else
            {
                u16tvalue = MST_VIP_ICC_CRD_SRAM_Main[0][u16tcount] | (MST_VIP_ICC_CRD_SRAM_Main[0][u16tcount+1]<<8);
            }
            W2BYTEMSK(u32reg+2, u16addr, 0x01FF);//addr
            W2BYTEMSK(u32reg+4, (MS_U16)u16tvalue, 0x3FF);//data
            W2BYTEMSK(u32reg+4, BIT15, BIT15);//wen
            //if(SCL_DBG_LV_DRVVIP()&EN_DBGMG_VIPLEVEL_VIPLOG)
            {
                W2BYTEMSK(u32reg+2, BIT15, BIT15);//ren
                SCL_DBG(SCL_DBG_LV_DRVVIP()&EN_DBGMG_VIPLEVEL_VIPLOG, "[ICCIHC]tval:%hx\n", u16tvalue);
                u16readdata=R2BYTE(u32reg+6);
                SCL_DBG(SCL_DBG_LV_DRVVIP()&EN_DBGMG_VIPLEVEL_VIPLOG, "[ICCIHC]reg tval:%hx\n", u16readdata);
            }
            u16tcount+=2;
        }
    }

    W2BYTEMSK(u32reg, 0, BIT0);//IOenable
}
void Hal_VIP_SRAMDumpHSP(MS_U32 u32Sram,MS_U32 u32reg)
{
    MS_U16 u16addr = 0,u16tvalue = 0,u16tcount = 0,u16readdata;
    //clear
    for(u16addr = 64;u16addr<128;u16addr++)
    {
        W2BYTEMSK(u32reg, BIT0, BIT0);//Yenable
        W2BYTEMSK(u32reg+2, u16addr, 0xFF);
        W2BYTE(u32reg+4, 0);
        W2BYTE(u32reg+6, 0);
        W2BYTEMSK(u32reg+8, 0, 0xFF);
        W2BYTEMSK(u32reg, BIT8, BIT8);//W pulse
        W2BYTEMSK(u32reg, 0, BIT0);//Yenable

        W2BYTEMSK(u32reg, BIT1, BIT1);//Cenable
        W2BYTE(u32reg+4, 0);
        W2BYTE(u32reg+6, 0);
        W2BYTEMSK(u32reg+8, 0, 0xFF);
        W2BYTEMSK(u32reg, BIT8, BIT8);//W pulse
        W2BYTEMSK(u32reg, 0, BIT1);//Cenable
    }
    //SRAM 0 //0~64 entry is V  65~127 is H

    for(u16addr=0;u16addr<(PQ_IP_SRAM1_SIZE_Main/5);u16addr++)
    {
           // 64~127
        W2BYTEMSK(u32reg, BIT0, BIT0);//Yenable
        W2BYTEMSK(u32reg+2, u16addr+64, 0xFF);
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
        SCL_DBG(SCL_DBG_LV_DRVVIP()&EN_DBGMG_VIPLEVEL_VIPLOG, "[HVSP]VYtval12:%hx\n", u16tvalue);
        W2BYTE(u32reg+4, u16tvalue);
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
        W2BYTE(u32reg+6, u16tvalue);
        SCL_DBG(SCL_DBG_LV_DRVVIP()&EN_DBGMG_VIPLEVEL_VIPLOG, "[HVSP]VYtval34:%hx\n", u16tvalue);
        if(u32Sram&0x1)
        {
            u16tvalue = MST_SRAM1_Main[((u32Sram&0xF0)>>4)][u16tcount+4];
        }
        else
        {
            u16tvalue = MST_SRAM2_Main[((u32Sram&0xF0)>>4)][u16tcount+4];
        }
        SCL_DBG(SCL_DBG_LV_DRVVIP()&EN_DBGMG_VIPLEVEL_VIPLOG, "[HVSP]Vtval5:%hx\n", u16tvalue);
        W2BYTEMSK(u32reg+8, u16tvalue, 0xFF);
        W2BYTEMSK(u32reg, BIT8, BIT8);//W pulse
        //if(SCL_DBG_LV_DRVVIP()&EN_DBGMG_VIPLEVEL_VIPLOG)
        {
            W2BYTEMSK(u32reg, BIT9, BIT9);//R pulse
            u16readdata = R2BYTE(u32reg+10);
            SCL_DBG(SCL_DBG_LV_DRVVIP()&EN_DBGMG_VIPLEVEL_VIPLOG, "[HVSP]Vread Yval 12:%hx\n", u16readdata);
            u16readdata = R2BYTE(u32reg+12);
            SCL_DBG(SCL_DBG_LV_DRVVIP()&EN_DBGMG_VIPLEVEL_VIPLOG, "[HVSP]Vread Yval 34:%hx\n", u16readdata);
            u16readdata = R2BYTE(u32reg+14);
            SCL_DBG(SCL_DBG_LV_DRVVIP()&EN_DBGMG_VIPLEVEL_VIPLOG, "[HVSP]Vread Yval 5:%hx\n", u16readdata);
        }
        W2BYTEMSK(u32reg, 0, BIT0);//Yenable

        W2BYTEMSK(u32reg, BIT1, BIT1);//Cenable
        u16tvalue = MST_C_SRAM1_Main[0][u16tcount] | (MST_C_SRAM1_Main[0][u16tcount+1]<<8);
        W2BYTE(u32reg+4, u16tvalue);
        u16tvalue = MST_C_SRAM1_Main[0][u16tcount+2] | (MST_C_SRAM1_Main[0][u16tcount+3]<<8);
        W2BYTE(u32reg+6, u16tvalue);
        u16tvalue = MST_C_SRAM1_Main[0][u16tcount+4];
        W2BYTEMSK(u32reg+8, u16tvalue, 0xFF);
        W2BYTEMSK(u32reg, BIT8, BIT8);//W pulse
        //if(SCL_DBG_LV_DRVVIP()&EN_DBGMG_VIPLEVEL_VIPLOG)
        {
            W2BYTEMSK(u32reg, BIT9, BIT9);//R pulse
            u16readdata = R2BYTE(u32reg+10);
            SCL_DBG(SCL_DBG_LV_DRVVIP()&EN_DBGMG_VIPLEVEL_VIPLOG, "[HVSP]Vread Cval 12:%hx\n", u16readdata);
            u16readdata = R2BYTE(u32reg+12);
            SCL_DBG(SCL_DBG_LV_DRVVIP()&EN_DBGMG_VIPLEVEL_VIPLOG, "[HVSP]Vread Cval 34:%hx\n", u16readdata);
            u16readdata = R2BYTE(u32reg+14);
            SCL_DBG(SCL_DBG_LV_DRVVIP()&EN_DBGMG_VIPLEVEL_VIPLOG, "[HVSP]Vread Cval 5:%hx\n", u16readdata);
        }
        W2BYTEMSK(u32reg, 0, BIT1);//Cenable
        u16tcount += 5;
    }
    //SRAM 1 //128~256 entry  link to 0~127 , so do not use
}
void Hal_VIP_SRAMDumpVSP(MS_U32 u32Sram,MS_U32 u32reg)
{
    MS_U16 u16addr = 0,u16tvalue = 0,u16tcount = 0,u16readdata;
    //clear
    for(u16addr = 0;u16addr<64;u16addr++)
    {
        W2BYTEMSK(u32reg, BIT0, BIT0);//Yenable
        W2BYTEMSK(u32reg+2, u16addr, 0xFF);
        W2BYTE(u32reg+4, 0);
        W2BYTE(u32reg+6, 0);
        W2BYTEMSK(u32reg+8, 0, 0xFF);
        W2BYTEMSK(u32reg, BIT8, BIT8);//W pulse
        W2BYTEMSK(u32reg, 0, BIT0);//Yenable

        W2BYTEMSK(u32reg, BIT1, BIT1);//Cenable
        W2BYTE(u32reg+4, 0);
        W2BYTE(u32reg+6, 0);
        W2BYTEMSK(u32reg+8, 0, 0xFF);
        W2BYTEMSK(u32reg, BIT8, BIT8);//W pulse
        W2BYTEMSK(u32reg, 0, BIT1);//Cenable
    }
    //SRAM 0 ///0~64 entry is V  65~127 is H

    for(u16addr=0;u16addr<(PQ_IP_SRAM1_SIZE_Main/5);u16addr++)
    {
        W2BYTEMSK(u32reg, BIT0, BIT0);//Yenable
        W2BYTEMSK(u32reg+2, u16addr, 0xFF);
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
        W2BYTE(u32reg+4, u16tvalue);
        SCL_DBG(SCL_DBG_LV_DRVVIP()&EN_DBGMG_VIPLEVEL_VIPLOG, "[HVSP]Ytval12:%hx\n", u16tvalue);
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
        W2BYTE(u32reg+6, u16tvalue);
        SCL_DBG(SCL_DBG_LV_DRVVIP()&EN_DBGMG_VIPLEVEL_VIPLOG, "[HVSP]Ytval34:%hx\n", u16tvalue);
        if(u32Sram&0x1)
        {
            u16tvalue = MST_SRAM1_Main[((u32Sram&0xF0)>>4)][u16tcount+4];
        }
        else
        {
            u16tvalue = MST_SRAM2_Main[((u32Sram&0xF0)>>4)][u16tcount+4];
        }
        W2BYTEMSK(u32reg+8, u16tvalue, 0xFF);
        SCL_DBG(SCL_DBG_LV_DRVVIP()&EN_DBGMG_VIPLEVEL_VIPLOG, "[HVSP]Ytval5:%hx\n", u16tvalue);
        W2BYTEMSK(u32reg, BIT8, BIT8);//W pulse
        //if(SCL_DBG_LV_DRVVIP()&EN_DBGMG_VIPLEVEL_VIPLOG)
        {
            W2BYTEMSK(u32reg, BIT9, BIT9);//R pulse
            u16readdata = R2BYTE(u32reg+10);
            SCL_DBG(SCL_DBG_LV_DRVVIP()&EN_DBGMG_VIPLEVEL_VIPLOG, "[HVSP]read Yval 12:%hx\n", u16readdata);
            u16readdata = R2BYTE(u32reg+12);
            SCL_DBG(SCL_DBG_LV_DRVVIP()&EN_DBGMG_VIPLEVEL_VIPLOG, "[HVSP]read Yval 34:%hx\n", u16readdata);
            u16readdata = R2BYTE(u32reg+14);
            SCL_DBG(SCL_DBG_LV_DRVVIP()&EN_DBGMG_VIPLEVEL_VIPLOG, "[HVSP]read Yval 5:%hx\n", u16readdata);
        }
        W2BYTEMSK(u32reg, 0, BIT0);//Yenable

        W2BYTEMSK(u32reg, BIT1, BIT1);//Cenable
        u16tvalue = MST_C_SRAM1_Main[0][u16tcount] | (MST_C_SRAM1_Main[0][u16tcount+1]<<8);
        SCL_DBG(SCL_DBG_LV_DRVVIP()&EN_DBGMG_VIPLEVEL_VIPLOG, "[HVSP]Ctval12:%hx\n", u16tvalue);
        W2BYTE(u32reg+4, u16tvalue);
        u16tvalue = MST_C_SRAM1_Main[0][u16tcount+2] | (MST_C_SRAM1_Main[0][u16tcount+3]<<8);
        SCL_DBG(SCL_DBG_LV_DRVVIP()&EN_DBGMG_VIPLEVEL_VIPLOG, "[HVSP]Ctval34:%hx\n", u16tvalue);
        W2BYTE(u32reg+6, u16tvalue);
        u16tvalue = MST_C_SRAM1_Main[0][u16tcount+4];
        SCL_DBG(SCL_DBG_LV_DRVVIP()&EN_DBGMG_VIPLEVEL_VIPLOG, "[HVSP]Ctval5:%hx\n", u16tvalue);
        W2BYTEMSK(u32reg+8, u16tvalue, 0xFF);
        W2BYTEMSK(u32reg, BIT8, BIT8);//W pulse

        //if(SCL_DBG_LV_DRVVIP()&EN_DBGMG_VIPLEVEL_VIPLOG)
        {
            W2BYTEMSK(u32reg, BIT9, BIT9);//R pulse
            u16readdata = R2BYTE(u32reg+10);
            SCL_DBG(SCL_DBG_LV_DRVVIP()&EN_DBGMG_VIPLEVEL_VIPLOG, "[HVSP]read Cval 12:%hx\n", u16readdata);
            u16readdata = R2BYTE(u32reg+12);
            SCL_DBG(SCL_DBG_LV_DRVVIP()&EN_DBGMG_VIPLEVEL_VIPLOG, "[HVSP]read Cval 34:%hx\n", u16readdata);
            u16readdata = R2BYTE(u32reg+14);
            SCL_DBG(SCL_DBG_LV_DRVVIP()&EN_DBGMG_VIPLEVEL_VIPLOG, "[HVSP]read Cval 5:%hx\n", u16readdata);
        }
        W2BYTEMSK(u32reg, 0, BIT1);//Cenable
        u16tcount += 5;
    }
    //SRAM 1 //128~256 entry  link to 0~127 , so do not use
}
MS_U16 Hal_VIP_GetWDRTvalue(MS_U8 u8sec, MS_U16 u16tcount, void* u32Sram)
{
    MS_U16 u16tvalue;
    MS_U8 *p8buffer;
    MS_U32 u32Val = 0;
    if(u32Sram)
    {
        p8buffer= (MS_U8 *)u32Sram;
        u32Val = WDR_SRAM_USERBYTENUM*2 *u8sec ;
        u16tvalue = (MS_U16)(*(p8buffer+u32Val+u16tcount) | (*(p8buffer+u32Val+u16tcount+1)<<8));
        SCL_DBG(SCL_DBG_LV_DRVVIP()&EN_DBGMG_VIPLEVEL_VIPLOG, "[WDRTvalue]val:%ld\n",u32Val+u16tcount);
    }
    else
    {
        switch(u8sec)
        {
            case 0:
                u16tvalue = MST_WDR_Loc_TBL_0_SRAM_Main[0][u16tcount] | (MST_WDR_Loc_TBL_0_SRAM_Main[0][u16tcount+1]<<8);
                break;
            case 1:
                u16tvalue = MST_WDR_Loc_TBL_1_SRAM_Main[0][u16tcount] | (MST_WDR_Loc_TBL_1_SRAM_Main[0][u16tcount+1]<<8);
                break;
            case 2:
                u16tvalue = MST_WDR_Loc_TBL_2_SRAM_Main[0][u16tcount] | (MST_WDR_Loc_TBL_2_SRAM_Main[0][u16tcount+1]<<8);
                break;
            case 3:
                u16tvalue = MST_WDR_Loc_TBL_3_SRAM_Main[0][u16tcount] | (MST_WDR_Loc_TBL_3_SRAM_Main[0][u16tcount+1]<<8);
                break;
            case 4:
                u16tvalue = MST_WDR_Loc_TBL_4_SRAM_Main[0][u16tcount] | (MST_WDR_Loc_TBL_4_SRAM_Main[0][u16tcount+1]<<8);
                break;
            case 5:
                u16tvalue = MST_WDR_Loc_TBL_5_SRAM_Main[0][u16tcount] | (MST_WDR_Loc_TBL_5_SRAM_Main[0][u16tcount+1]<<8);
                break;
            case 6:
                u16tvalue = MST_WDR_Loc_TBL_6_SRAM_Main[0][u16tcount] | (MST_WDR_Loc_TBL_6_SRAM_Main[0][u16tcount+1]<<8);
                break;
            case 7:
                u16tvalue = MST_WDR_Loc_TBL_7_SRAM_Main[0][u16tcount] | (MST_WDR_Loc_TBL_7_SRAM_Main[0][u16tcount+1]<<8);
                break;
            default:
                u16tvalue = 0;
                break;
        }
    }
    return u16tvalue;
}
void Hal_VIP_SetWriteRegType(MS_BOOL bEn)
{
    if(bEn && (VIPSETRULE()==E_VIPSetRule_CMDQAll || VIPSETRULE()==E_VIPSetRule_CMDQAllCheck
        ||(VIPSETRULE() == E_VIPSetRule_CMDQAllONLYSRAMCheck)))
    {
        Drv_CMDQ_GetModuleMutex(EN_CMDQ_TYPE_IP0,1);
        Drv_CMDQ_SetForceSkip(1);
        if(Drv_VIP_GetEachDMAEn())
        {
            gbCMDQ = 1;
        }
        else
        {
            gbCMDQ = 0;
        }
    }
    else if(bEn && VIPSETRULE())
    {
        Drv_CMDQ_GetModuleMutex(EN_CMDQ_TYPE_IP0,1);
        Drv_CMDQ_SetForceSkip(1);
        if(Drv_VIP_GetEachDMAEn()&& !Drv_VIP_GetIsBlankingRegion())
        {
            gbCMDQ = 1;
        }
        else
        {
            gbCMDQ = 0;
        }
    }
    else
    {
        gbCMDQ = 0;
        Drv_CMDQ_SetForceSkip(0);
        Drv_CMDQ_GetModuleMutex(EN_CMDQ_TYPE_IP0,0);
    }
}
void Hal_VIP_WriteReg(MS_U32 u32Reg,MS_U16 u16Val,MS_U16 u16Mask)
{
    MS_CMDQ_CMDReg stCfg;
    if(gbCMDQ)
    {
        Drv_CMDQ_FillCmd(&stCfg, u32Reg, u16Val, u16Mask);
        Drv_CMDQ_WriteCmd(EN_CMDQ_TYPE_IP0, &stCfg, 1);
    }
    else
    {
        W2BYTEMSK(u32Reg, u16Val, u16Mask);//sec
    }
}
MS_BOOL _Hal_VIP_CheckMonotonicallyIncreasing
    (MS_U16 u16tvalueeven,MS_U16 u16tvalueodd,MS_U16 u16chkodd)
{
    if((u16tvalueodd < u16tvalueeven)||(u16tvalueeven < u16chkodd))
    {
        return 0;
    }
    else
    {
        return 1;
    }
}
void Hal_VIP_SRAMDumpWDR(MS_U32 u32reg, void* u32Sram)
{
    MS_U8  u8sec = 0;
    MS_U16 u16addr = 0,u16tvalue = 0,u16tcount = 0;
    MS_U16 u16addrmax;
    MS_U32 u32Events;
    MS_U8  u8Cnt = 0;
    MS_U16 u16tvalueOri;
    void* u32SramOri;
    u32SramOri = Drv_VIP_GeSRAMGlobal(EN_VIP_DRV_AIP_SRAM_WDR);
    if(u32Sram==NULL)
    {
        u16addrmax = WDR_SRAM_BYTENUM;
    }
    else
    {
        u16addrmax = WDR_SRAM_USERBYTENUM;
    }
    HAL_VIP_MUTEX_LOCK();
    if(WDR_USE_CMDQ())
    {
        Hal_VIP_SetWriteRegType(1);
    }
    for(u8sec=0;u8sec<WDR_SRAM_NUM;u8sec++)
    {
        SCL_DBG(SCL_DBG_LV_DRVVIP()&EN_DBGMG_VIPLEVEL_VIPLOG, "WDR:%hhx\n", u8sec);
        Hal_VIP_WriteReg(u32reg, u8sec, 0xF);//sec
        u16tcount = 0;
        for(u16addr=0;u16addr<u16addrmax;u16addr++)
        {
            if(!WDR_USE_CMDQ())
            {
                if(!Drv_VIP_GetIsBlankingRegion() && (u32Sram))
                {
                    u8Cnt++;
                    if(u8Cnt<5)
                    {
                        HAL_VIP_MUTEX_UNLOCK();
                        MsOS_WaitEvent(Drv_SCLIRQ_Get_IRQ_SYNCEventID(), E_SCLIRQ_EVENT_FRMENDSYNC, &u32Events, E_OR, 2000); // get status: FRM END
                        HAL_VIP_MUTEX_LOCK();
                    }
                }
            }
            u16tvalue = Hal_VIP_GetWDRTvalue(u8sec, u16tcount,u32Sram);
            u16tvalueOri = Hal_VIP_GetWDRTvalue(u8sec, u16tcount,u32SramOri);
            //Hal_VIP_WriteReg(u32reg+4, u16addr, 0x007F);//addr
            if((u16tvalue != u16tvalueOri)||u32Sram==NULL)
            {
                Hal_VIP_WriteReg(u32reg+2, (MS_U16)u16tvalue, 0xFFF);//data
                Hal_VIP_WriteReg(u32reg+4, u16addr|BIT8|BIT9, 0xFFFF);//wen
                SCL_DBG(SCL_DBG_LV_DRVVIP()&EN_DBGMG_VIPLEVEL_VIPLOG, "tval:%hx addr:%hd\n",u16tvalue,u16addr);
            }
            //for i3e can't read back
            //Hal_VIP_WriteReg(u32reg+4, u16addr|BIT9, 0xFFFF);//ren
            //u16readdata=R2BYTE(u32reg+6);
            //HAL_VIP_DBG(printf("reg tval:%hx\n",u16readdata));
            u16tcount+=2;
        }
    }
    if(WDR_USE_CMDQ())
    {
        if(gbCMDQ)
        {
            SCL_DBG(SCL_DBG_LV_DRVCMDQ()&EN_DBGMG_CMDQEVEL_ISR, "gbCMDQ WDR \n");
            Drv_CMDQ_Fire(EN_CMDQ_TYPE_IP0,1);
        }
        Hal_VIP_SetWriteRegType(0);
    }
    HAL_VIP_MUTEX_UNLOCK();
}
MS_U16 Hal_VIP_GetGammaYUVTvalue(MS_U16 u16sec, MS_U16 u16tcount,MS_U8 *u32Sram)
{
    MS_U16 u16tvalue;
    if(u32Sram)
    {
        u16tvalue = (MS_U16)(*(u32Sram+u16tcount) | (*(u32Sram+u16tcount+1)<<8));
    }
    else
    {
        switch(u16sec)
        {
            case 0:
                u16tvalue = MST_YUV_Gamma_tblY_SRAM_Main[0][u16tcount] | (MST_YUV_Gamma_tblY_SRAM_Main[0][u16tcount+1]<<8);
                break;
            case 2:
                u16tvalue = MST_YUV_Gamma_tblU_SRAM_Main[0][u16tcount] | (MST_YUV_Gamma_tblU_SRAM_Main[0][u16tcount+1]<<8);
                break;
            case 4:
                u16tvalue = MST_YUV_Gamma_tblV_SRAM_Main[0][u16tcount] | (MST_YUV_Gamma_tblV_SRAM_Main[0][u16tcount+1]<<8);
                break;
            default:
                u16tvalue = 0;
                break;
        }
    }
    return u16tvalue;
}

MS_BOOL Hal_VIP_SRAMDumpGammaYUV(EN_VIP_SRAM_DUMP_TYPE endump,void * u32Sram,MS_U32 u32reg)
{
    MS_U16  u16sec = 0;
    MS_U8  bRet = 1;
    MS_U16  u16chkodd = 0,u16tvalueoddori = 0,u16tvalueevenori = 0;
    MS_U16 u16addr = 0,u16tvalueodd = 0,u16tvalueeven = 0,u16tcount = 0,u16readdata,u16RegMask,u16size;
    MS_U32 u32rega, u32regd ,u32Events= 0;
    void *u32Sramori;
    EN_VIP_DRV_AIP_SRAM_TYPE entype;
    switch(endump)
    {
        case EN_VIP_SRAM_DUMP_GAMMA_Y:
            u16RegMask = 0x3;
            u32rega = u32reg+2;
            u32regd = u32reg+8;
            u16size = GAMMAY_SRAM_BYTENUM;
            u16sec = 0;
            entype = EN_VIP_DRV_AIP_SRAM_GAMMA_Y;
            break;
        case EN_VIP_SRAM_DUMP_GAMMA_U:
            u16RegMask = 0xC;
            u32rega = u32reg+4;
            u32regd = u32reg+12;
            u16size = GAMMAU_SRAM_BYTENUM;
            u16sec = 2;
            entype = EN_VIP_DRV_AIP_SRAM_GAMMA_U;
            break;
        case EN_VIP_SRAM_DUMP_GAMMA_V:
            u16RegMask = 0x30;
            u32rega = u32reg+6;
            u32regd = u32reg+16;
            u16size = GAMMAV_SRAM_BYTENUM;
            u16sec = 4;
            entype = EN_VIP_DRV_AIP_SRAM_GAMMA_V;
            break;
        default:
            return 0;
    }
    u32Sramori = Drv_VIP_GeSRAMGlobal(entype);
    SCL_DBG(SCL_DBG_LV_DRVVIP()&EN_DBGMG_VIPLEVEL_VIPLOG, "YUVGamma:%hhx\n", u16sec);
    HAL_VIP_MUTEX_LOCK();
    Hal_VIP_SetWriteRegType(1);
    for(u16addr=0;u16addr<u16size;u16addr++)
    {
        if(!Drv_VIP_GetIsBlankingRegion() && !gbCMDQ)
        {
            Hal_VIP_SetWriteRegType(0);
            Hal_VIP_SetWriteRegType(1);
            if(VIPSETRULE())
            {
                SCL_DBG(SCL_DBG_LV_DRVCMDQ()&EN_DBGMG_CMDQEVEL_ISR, "bCMDQ Open YUVGamma:%hhx\n",u16sec);
            }
            else
            {
                Hal_VIP_SetWriteRegType(0);
                HAL_VIP_MUTEX_UNLOCK();
                return 0;
            }
        }
        u16tvalueeven = Hal_VIP_GetGammaYUVTvalue(u16sec, u16tcount,(MS_U8 *)u32Sram);
        u16tvalueodd = Hal_VIP_GetGammaYUVTvalue(u16sec, u16tcount+2,(MS_U8 *)u32Sram);
        u16tvalueevenori = Hal_VIP_GetGammaYUVTvalue(u16sec, u16tcount,(MS_U8 *)u32Sramori);
        u16tvalueoddori = Hal_VIP_GetGammaYUVTvalue(u16sec, u16tcount+2,(MS_U8 *)u32Sramori);
        if(bRet)
        {
            bRet = _Hal_VIP_CheckMonotonicallyIncreasing(u16tvalueeven,u16tvalueodd,u16chkodd);
            if(!bRet)
            {
                //SCL_ERR("[HALVIP]YUVGamma:%hhx @:%hx NOT Monotonically Increasing  (%hx,%hx,%hx)\n"
                //    ,u16sec,u16addr,u16chkodd,u16tvalueeven,u16tvalueodd);
            }
        }
        u16chkodd = u16tvalueodd;
        if((u16tvalueeven!=u16tvalueevenori || u16tvalueodd != u16tvalueoddori)||u32Sram==NULL)
        {
            Hal_VIP_WriteReg(u32rega, (((u16addr)<<8)|u16addr), 0x7F7F);//addr
            Hal_VIP_WriteReg(u32regd, (MS_U16)u16tvalueeven, 0xFFF);//data
            Hal_VIP_WriteReg(u32regd+2, (MS_U16)u16tvalueodd, 0xFFF);//data
            Hal_VIP_WriteReg(u32reg, u16RegMask, u16RegMask);//wen
            if(!gbCMDQ)
            {
                Hal_VIP_WriteReg(u32rega, (((u16addr)<<8)|u16addr), 0x7F7F);//addr
                Hal_VIP_WriteReg(u32reg-2, (u16sec<<3)|BIT2, BIT2|BIT3|BIT4|BIT5);//ridx //ren bit2
                //Hal_VIP_WriteReg(u32reg-2,BIT2 ,BIT2 );
                SCL_DBG(SCL_DBG_LV_DRVVIP()&EN_DBGMG_VIPLEVEL_VIPLOG, "even val:%hx\n", u16tvalueeven);
                u16readdata=R2BYTE(u32reg+28);
                SCL_DBG(SCL_DBG_LV_DRVVIP()&EN_DBGMG_VIPLEVEL_VIPLOG, "read val:%hx\n", u16readdata);
                for(bRet = 0;bRet<10;bRet++)
                {
                    if(u16readdata!=u16tvalueeven)
                    {
                        SCL_DBGERR("[HALVIP]EVEN YUVGamma:%hhx\n",u16sec);
                        SCL_DBGERR( "[HALVIP]EVEN val:%hx\n",u16tvalueeven);
                        SCL_DBGERR( "[HALVIP]read val:%hx\n",u16readdata);
                        if(!Drv_VIP_GetIsBlankingRegion())
                        {
                            MsOS_WaitEvent(Drv_SCLIRQ_Get_IRQ_SYNCEventID(), E_SCLIRQ_EVENT_FRMENDSYNC, &u32Events, E_OR, 2000); // get status: FRM END
                        }
                        Hal_VIP_WriteReg(u32rega, (((u16addr)<<8)|u16addr), 0x7F7F);//addr
                        Hal_VIP_WriteReg(u32regd, (MS_U16)u16tvalueeven, 0xFFF);//data
                        Hal_VIP_WriteReg(u32regd+2, (MS_U16)u16tvalueodd, 0xFFF);//data
                        Hal_VIP_WriteReg(u32reg, u16RegMask, u16RegMask);//wen
                        Hal_VIP_WriteReg(u32reg-2, (((u16sec+1)<<3)|(u16sec<<3)|BIT2), BIT2|BIT3|BIT4|BIT5);//ridx //ren bit2
                        u16readdata=R2BYTE(u32reg+28);
                    }
                    else
                    {
                        break;
                    }
                }
            }
            if(!gbCMDQ)
            {
                Hal_VIP_WriteReg(u32reg-2, ((u16sec+1)<<3)|BIT2, BIT2|BIT3|BIT4|BIT5);//ridx
                //Hal_VIP_WriteReg(u32reg-2, BIT2, BIT2);//ren
                SCL_DBG(SCL_DBG_LV_DRVVIP()&EN_DBGMG_VIPLEVEL_VIPLOG, "odd val:%hx\n", u16tvalueodd);
                u16readdata=R2BYTE(u32reg+28);
                SCL_DBG(SCL_DBG_LV_DRVVIP()&EN_DBGMG_VIPLEVEL_VIPLOG, "read val:%hx\n", u16readdata);
                for(bRet = 0;bRet<10;bRet++)
                {
                    if(u16readdata!=u16tvalueodd)
                    {
                        SCL_DBGERR("[HALVIP]ODD YUVGamma:%hhx\n",u16sec);
                        SCL_DBGERR( "[HALVIP]odd val:%hx\n",u16tvalueodd);
                        SCL_DBGERR( "[HALVIP]read val:%hx\n",u16readdata);
                        if(!Drv_VIP_GetIsBlankingRegion())
                        {
                            MsOS_WaitEvent(Drv_SCLIRQ_Get_IRQ_SYNCEventID(), E_SCLIRQ_EVENT_FRMENDSYNC, &u32Events, E_OR, 2000); // get status: FRM END
                        }
                        Hal_VIP_WriteReg(u32rega, (((u16addr)<<8)|u16addr), 0x7F7F);//addr
                        Hal_VIP_WriteReg(u32regd+2, (MS_U16)u16tvalueodd, 0xFFF);//data
                        Hal_VIP_WriteReg(u32regd, (MS_U16)u16tvalueeven, 0xFFF);//data
                        Hal_VIP_WriteReg(u32reg, u16RegMask, u16RegMask);//wen
                        Hal_VIP_WriteReg(u32reg-2, (((u16sec+1)<<3)|(u16sec<<3)|BIT2), BIT2|BIT3|BIT4|BIT5);//ridx
                        u16readdata=R2BYTE(u32reg+28);
                    }
                    else
                    {
                        break;
                    }
                }
            }
        }
        u16tcount+=4;
    }
    Hal_VIP_WriteReg(u32regd, 0, 0xFFF);//data
    Hal_VIP_WriteReg(u32regd+2, 0, 0xFFF);//data
    if(gbCMDQ)
    {
        SCL_DBG(SCL_DBG_LV_DRVCMDQ()&EN_DBGMG_CMDQEVEL_ISR, "gbCMDQ YUVGamma:%hhx\n",u16sec);
        Drv_CMDQ_Fire(EN_CMDQ_TYPE_IP0,1);
        if(endump==EN_VIP_SRAM_DUMP_GAMMA_Y)
        {
            MsOS_SetEvent_IRQ(Drv_SCLIRQ_Get_IRQ_SYNCEventID(),  E_SCLIRQ_EVENT_GAMMA_Y);
        }
        else if(endump==EN_VIP_SRAM_DUMP_GAMMA_U)
        {
            MsOS_SetEvent_IRQ(Drv_SCLIRQ_Get_IRQ_SYNCEventID(),  E_SCLIRQ_EVENT_GAMMA_U);
        }
        else if(endump==EN_VIP_SRAM_DUMP_GAMMA_V)
        {
            MsOS_SetEvent_IRQ(Drv_SCLIRQ_Get_IRQ_SYNCEventID(),  E_SCLIRQ_EVENT_GAMMA_V);
        }
    }
    Hal_VIP_SetWriteRegType(0);
    HAL_VIP_MUTEX_UNLOCK();
    return bRet;
}
MS_BOOL Hal_VIP_GetSRAMDumpGammaYUVCallback(EN_VIP_SRAM_DUMP_TYPE endump,void *u32Sram,MS_U32 u32reg)
{
    MS_U16  u16sec = 0;
    u8 flag = 0;
    MS_U16 u16addr = 0,u16tcount = 0,u16readdata= 0,u16readdataodd= 0,u16tvalueodd = 0,u16tvalueeven = 0,u16RegMask,u16size;
    MS_U32 u32rega, u32regd;
    switch(endump)
    {
        case EN_VIP_SRAM_DUMP_GAMMA_Y:
            u16RegMask = 0x3;
            u32rega = u32reg+2;
            u32regd = u32reg+8;
            u16size = GAMMAY_SRAM_BYTENUM;
            u16sec = 0;
            break;
        case EN_VIP_SRAM_DUMP_GAMMA_U:
            u16RegMask = 0xC;
            u32rega = u32reg+4;
            u32regd = u32reg+12;
            u16size = GAMMAU_SRAM_BYTENUM;
            u16sec = 2;
            break;
        case EN_VIP_SRAM_DUMP_GAMMA_V:
            u16RegMask = 0x30;
            u32rega = u32reg+6;
            u32regd = u32reg+16;
            u16size = GAMMAV_SRAM_BYTENUM;
            u16sec = 4;
            break;
        default:
            return 0;
    }
    SCL_DBG(SCL_DBG_LV_DRVCMDQ()&EN_DBGMG_CMDQEVEL_ISR, "YUVGamma:%hhx\n", u16sec);
    HAL_VIP_MUTEX_LOCK();
    for(u16addr=0;u16addr<u16size;u16addr++)
    {
        if(!Drv_VIP_GetIsBlankingRegion())
        {
            if(flag)
            {
                SCL_DBG(SCL_DBG_LV_DRVCMDQ()&EN_DBGMG_CMDQEVEL_DBG,"[ISRCheck]Fire\n");
                Hal_VIP_SetWriteRegType(1);
                Hal_VIP_WriteReg(u32regd, 0, 0xFFF);//data
                Hal_VIP_WriteReg(u32regd+2, 0, 0xFFF);//data
                if(gbCMDQ)
                {
                    Drv_CMDQ_Fire(EN_CMDQ_TYPE_IP0,1);
                }
                Hal_VIP_SetWriteRegType(0);
            }
            MsOS_ClearEventIRQ(Drv_SCLIRQ_Get_IRQ_SYNCEventID(),(E_SCLIRQ_EVENT_FRMENDSYNC));
            HAL_VIP_MUTEX_UNLOCK();
            return 0;
        }
        u16tvalueeven = Hal_VIP_GetGammaYUVTvalue(u16sec, u16tcount,(MS_U8 *)u32Sram);
        u16tvalueodd = Hal_VIP_GetGammaYUVTvalue(u16sec, u16tcount+2,(MS_U8 *)u32Sram);
        Hal_VIP_WriteReg(u32rega, (((u16addr)<<8)|u16addr), 0x7F7F);//addr
        Hal_VIP_WriteReg(u32reg-2, (u16sec<<3)|BIT2, BIT2|BIT3|BIT4|BIT5);//ridx //ren bit2
        u16readdata=R2BYTE(u32reg+28);
        Hal_VIP_WriteReg(u32reg-2, ((u16sec+1)<<3)|BIT2, BIT2|BIT3|BIT4|BIT5);//ridx //ren bit2
        u16readdataodd=R2BYTE(u32reg+28);
        SCL_DBG(SCL_DBG_LV_DRVCMDQ()&EN_DBGMG_CMDQEVEL_SRAMCheck, "YUVGamma:%hhx", u16sec);
        SCL_DBG(SCL_DBG_LV_DRVCMDQ()&EN_DBGMG_CMDQEVEL_SRAMCheck, "addr:%hx even val:%hx even read val:%hx\n",u16addr,u16tvalueeven, u16readdata);
        if(u16readdata != u16tvalueeven || u16readdataodd != u16tvalueodd)
        {
            Hal_VIP_SetWriteRegType(1);
            flag = gbCMDQ;
            SCL_DBG(SCL_DBG_LV_DRVCMDQ()&EN_DBGMG_CMDQEVEL_DBG,"YUVGamma:%hhx", u16sec);
            SCL_DBG(SCL_DBG_LV_DRVCMDQ()&EN_DBGMG_CMDQEVEL_DBG,"addr:%hx even val:%hx even read val:%hx\n",u16addr,u16tvalueeven, u16readdata);
            SCL_DBG(SCL_DBG_LV_DRVCMDQ()&EN_DBGMG_CMDQEVEL_DBG,"addr:%hx odd val:%hx odd read val:%hx\n",u16addr,u16tvalueodd, u16readdata);
            Hal_VIP_WriteReg(u32rega, (((u16addr)<<8)|u16addr), 0x7F7F);//addr
            Hal_VIP_WriteReg(u32regd, (MS_U16)u16tvalueeven, 0xFFF);//data
            Hal_VIP_WriteReg(u32regd+2, (MS_U16)u16tvalueodd, 0xFFF);//data
            Hal_VIP_WriteReg(u32reg, u16RegMask, u16RegMask);//wen
            Hal_VIP_WriteReg(u32reg-2, (((u16sec+1)<<3)|(u16sec<<3)|BIT2), BIT2|BIT3|BIT4|BIT5);//ridx
            Hal_VIP_SetWriteRegType(0);
        }
        u16tcount+=4;
    }
    if(flag)
    {
        SCL_DBG(SCL_DBG_LV_DRVCMDQ()&EN_DBGMG_CMDQEVEL_DBG,"[ISRCheck]Fire\n");
        Hal_VIP_SetWriteRegType(1);
        Hal_VIP_WriteReg(u32regd, 0, 0xFFF);//data
        Hal_VIP_WriteReg(u32regd+2, 0, 0xFFF);//data
        if(gbCMDQ)
        {
            Drv_CMDQ_Fire(EN_CMDQ_TYPE_IP0,1);
        }
        Hal_VIP_SetWriteRegType(0);
    }
    HAL_VIP_MUTEX_UNLOCK();
    return 1;
}
MS_BOOL Hal_VIP_GetSRAMDumpGammaYUV(EN_VIP_SRAM_DUMP_TYPE endump,void *u32Sram,MS_U32 u32reg)
{
    MS_U16  u16sec = 0;
    u8 flag = 0;
    MS_U16 u16addr = 0,u16tcount = 0,u16readdata = 0,u16readdataodd = 0,u16tvalueodd = 0,u16tvalueeven = 0,u16RegMask,u16size;
    MS_U32 u32rega, u32regd;
    switch(endump)
    {
        case EN_VIP_SRAM_DUMP_GAMMA_Y:
            u16RegMask = 0x3;
            u32rega = u32reg+2;
            u32regd = u32reg+8;
            u16size = GAMMAY_SRAM_BYTENUM;
            u16sec = 0;
            break;
        case EN_VIP_SRAM_DUMP_GAMMA_U:
            u16RegMask = 0xC;
            u32rega = u32reg+4;
            u32regd = u32reg+12;
            u16size = GAMMAU_SRAM_BYTENUM;
            u16sec = 2;
            break;
        case EN_VIP_SRAM_DUMP_GAMMA_V:
            u16RegMask = 0x30;
            u32rega = u32reg+6;
            u32regd = u32reg+16;
            u16size = GAMMAV_SRAM_BYTENUM;
            u16sec = 4;
            break;
        default:
            return 0;
    }
    SCL_DBG(SCL_DBG_LV_DRVCMDQ()&EN_DBGMG_CMDQEVEL_ISR, "YUVGamma:%hhx\n", u16sec);
    HAL_VIP_MUTEX_LOCK();
    for(u16addr=0;u16addr<u16size;u16addr++)
    {
        if(!Drv_VIP_GetIsBlankingRegion())
        {
            HAL_VIP_MUTEX_UNLOCK();
            return 0;
        }
        u16tvalueeven = Hal_VIP_GetGammaYUVTvalue(u16sec, u16tcount,(MS_U8 *)u32Sram);
        u16tvalueodd = Hal_VIP_GetGammaYUVTvalue(u16sec, u16tcount+2,(MS_U8 *)u32Sram);
        Hal_VIP_WriteReg(u32rega, (((u16addr)<<8)|u16addr), 0x7F7F);//addr
        Hal_VIP_WriteReg(u32reg-2, (u16sec<<3)|BIT2, BIT2|BIT3|BIT4|BIT5);//ridx //ren bit2
        u16readdata=R2BYTE(u32reg+28);
        Hal_VIP_WriteReg(u32reg-2, ((u16sec+1)<<3)|BIT2, BIT2|BIT3|BIT4|BIT5);//ridx //ren bit2
        u16readdataodd=R2BYTE(u32reg+28);
        SCL_DBG(SCL_DBG_LV_DRVCMDQ()&EN_DBGMG_CMDQEVEL_SRAMCheck, "YUVGamma:%hhx", u16sec);
        SCL_DBG(SCL_DBG_LV_DRVCMDQ()&EN_DBGMG_CMDQEVEL_SRAMCheck, "addr:%hx even val:%hx even read val:%hx\n",u16addr,u16tvalueeven, u16readdata);
        if(u16readdata != u16tvalueeven || u16readdataodd != u16tvalueodd)
        {
            Hal_VIP_SetWriteRegType(1);
            flag = gbCMDQ;
            if(Drv_VIP_GetIsBlankingRegion())
            {
                SCL_DBGERR("YUVGamma:%hhx", u16sec);
                SCL_DBGERR("addr:%hx even val:%hx even read val:%hx\n",u16addr,u16tvalueeven, u16readdata);
                SCL_DBGERR("addr:%hx odd val:%hx odd read val:%hx\n",u16addr,u16tvalueodd, u16readdata);
            }
            Hal_VIP_WriteReg(u32rega, (((u16addr)<<8)|u16addr), 0x7F7F);//addr
            Hal_VIP_WriteReg(u32regd, (MS_U16)u16tvalueeven, 0xFFF);//data
            Hal_VIP_WriteReg(u32regd+2, (MS_U16)u16tvalueodd, 0xFFF);//data
            Hal_VIP_WriteReg(u32reg, u16RegMask, u16RegMask);//wen
            Hal_VIP_WriteReg(u32reg-2, (((u16sec+1)<<3)|(u16sec<<3)|BIT2), BIT2|BIT3|BIT4|BIT5);//ridx
            Hal_VIP_SetWriteRegType(0);
        }
        u16tcount+=4;
    }
    if(flag)
    {
        Drv_CMDQ_Fire(EN_CMDQ_TYPE_IP0,1);
    }
    HAL_VIP_MUTEX_UNLOCK();
    return 1;
}
MS_U16 Hal_VIP_GetGammaRGBTvalue(MS_U16 u16sec, MS_U16 u16tcount,MS_U8 *u32Sram,MS_U8  u8type)
{
    MS_U16 u16tvalue;
    if(u32Sram)
    {
        u16tvalue = (MS_U16)(*(u32Sram+u16tcount) | (*(u32Sram+u16tcount+1)<<8));
    }
    else
    {
        switch(u16sec+u8type)
        {
            case 0:
                u16tvalue = MST_ColorEng_GM10to12_Tbl_R_SRAM_Main[0][u16tcount] | (MST_ColorEng_GM10to12_Tbl_R_SRAM_Main[0][u16tcount+1]<<8);
                break;
            case 2:
                u16tvalue = MST_ColorEng_GM10to12_Tbl_G_SRAM_Main[0][u16tcount] | (MST_ColorEng_GM10to12_Tbl_G_SRAM_Main[0][u16tcount+1]<<8);
                break;
            case 4:
                u16tvalue = MST_ColorEng_GM10to12_Tbl_B_SRAM_Main[0][u16tcount] | (MST_ColorEng_GM10to12_Tbl_B_SRAM_Main[0][u16tcount+1]<<8);
                break;
            case 1:
                u16tvalue = MST_ColorEng_GM12to10_CrcTbl_R_SRAM_Main[0][u16tcount] | (MST_ColorEng_GM12to10_CrcTbl_R_SRAM_Main[0][u16tcount+1]<<8);
                break;
            case 3:
                u16tvalue = MST_ColorEng_GM12to10_CrcTbl_G_SRAM_Main[0][u16tcount] | (MST_ColorEng_GM12to10_CrcTbl_G_SRAM_Main[0][u16tcount+1]<<8);
                break;
            case 5:
                u16tvalue = MST_ColorEng_GM12to10_CrcTbl_B_SRAM_Main[0][u16tcount] | (MST_ColorEng_GM12to10_CrcTbl_B_SRAM_Main[0][u16tcount+1]<<8);
                break;
            default:
                u16tvalue = 0;
                break;
        }
    }
    return u16tvalue;
}

MS_U8 Hal_VIP_SRAMDumpGammaRGB(EN_VIP_SRAM_DUMP_TYPE endump,void * u32Sram,MS_U32 u32reg)
{
    MS_U16  u16sec = 0;
    MS_U8  u8type = 0;
    MS_U8  bRet = 1;
    MS_U16 u16addr = 0,u16tvalueodd = 0,u16tvalueeven = 0,u16tcount = 0,u16readdata,u16size;
    MS_U16 u16chkodd = 0;
    MS_U32 u32Events = 0;
    MS_U16 u16tvalueoddori = 0,u16tvalueevenori = 0;
    void * u32Sramori = NULL;
    EN_VIP_DRV_AIP_SRAM_TYPE entype = EN_VIP_DRV_AIP_SRAM_GAMMA_Y;
    switch(endump)
    {
        case EN_VIP_SRAM_DUMP_GM10to12_R:
            u16size = GAMMA10to12R_SRAM_BYTENUM;
            u16sec = 0;
            u8type = 0;
            entype = EN_VIP_DRV_AIP_SRAM_GM10to12_R;
            SCL_DBG(SCL_DBG_LV_IOCTL()&EN_DBGMG_IOCTLEVEL_VIP, "GM10to12_R:%lx\n",u32reg);
            break;
        case EN_VIP_SRAM_DUMP_GM10to12_G:
            u16size = GAMMA10to12G_SRAM_BYTENUM;
            u16sec = 2;
            u8type = 0;
            entype = EN_VIP_DRV_AIP_SRAM_GM10to12_G;
            SCL_DBG(SCL_DBG_LV_IOCTL()&EN_DBGMG_IOCTLEVEL_VIP, "GM10to12_G:%lx\n",u32reg);
            break;
        case EN_VIP_SRAM_DUMP_GM10to12_B:
            u16size = GAMMA10to12B_SRAM_BYTENUM;
            u16sec = 4;
            u8type = 0;
            entype = EN_VIP_DRV_AIP_SRAM_GM10to12_B;
            SCL_DBG(SCL_DBG_LV_IOCTL()&EN_DBGMG_IOCTLEVEL_VIP, "GM10to12_B:%lx\n",u32reg);
            break;
        case EN_VIP_SRAM_DUMP_GM12to10_R:
            u16size = GAMMA12to10R_SRAM_BYTENUM;
            u16sec = 0;
            u8type = 1;
            entype = EN_VIP_DRV_AIP_SRAM_GM12to10_R;
            SCL_DBG(SCL_DBG_LV_IOCTL()&EN_DBGMG_IOCTLEVEL_VIP, "GM12to10R:%lx\n",u32reg);
            break;
        case EN_VIP_SRAM_DUMP_GM12to10_G:
            u16size = GAMMA12to10G_SRAM_BYTENUM;
            u16sec = 2;
            u8type = 1;
            entype = EN_VIP_DRV_AIP_SRAM_GM12to10_G;
            SCL_DBG(SCL_DBG_LV_IOCTL()&EN_DBGMG_IOCTLEVEL_VIP, "GM12to10G:%lx\n",u32reg);
            break;
        case EN_VIP_SRAM_DUMP_GM12to10_B:
            u16size = GAMMA12to10B_SRAM_BYTENUM;
            u16sec = 4;
            u8type = 1;
            entype = EN_VIP_DRV_AIP_SRAM_GM12to10_B;
            SCL_DBG(SCL_DBG_LV_IOCTL()&EN_DBGMG_IOCTLEVEL_VIP, "GM12to10B:%lx\n",u32reg);
            break;
        default:
            return 0;
    }
    SCL_DBG(SCL_DBG_LV_DRVVIP()&EN_DBGMG_VIPLEVEL_VIPLOG, "RGBGamma:%hhx\n",u16sec+u8type);
    HAL_VIP_MUTEX_LOCK();
    u32Sramori = Drv_VIP_GeSRAMGlobal(entype);
    Hal_VIP_SetWriteRegType(1);
    for(u16addr=0;u16addr<u16size;u16addr++)
    {
        if(!Drv_VIP_GetIsBlankingRegion() && !gbCMDQ)
        {
            Hal_VIP_SetWriteRegType(0);
            Hal_VIP_SetWriteRegType(1);
            if(VIPSETRULE())
            {
                SCL_DBG(SCL_DBG_LV_DRVCMDQ()&EN_DBGMG_CMDQEVEL_ISR, "bCMDQ Open RGBGamma:%hhx\n",u16sec+u8type);
            }
            else
            {
                Hal_VIP_SetWriteRegType(0);
                HAL_VIP_MUTEX_UNLOCK();
                return 0;
            }
        }
        u16tvalueeven = Hal_VIP_GetGammaRGBTvalue(u16sec, u16tcount,(MS_U8 *)u32Sram,u8type);
        u16tvalueodd = Hal_VIP_GetGammaRGBTvalue(u16sec, u16tcount+2,(MS_U8 *)u32Sram,u8type);
        u16tvalueevenori = Hal_VIP_GetGammaRGBTvalue(u16sec, u16tcount,(MS_U8 *)u32Sramori,u8type);
        u16tvalueoddori = Hal_VIP_GetGammaRGBTvalue(u16sec, u16tcount+2,(MS_U8 *)u32Sramori,u8type);
        if(bRet)
        {
            bRet = _Hal_VIP_CheckMonotonicallyIncreasing(u16tvalueeven,u16tvalueodd,u16chkodd);
            if(!bRet)
            {
                SCL_ERR("[HALVIP]RGBGamma:%hhx @:%hx NOT Monotonically Increasing  (%hx,%hx,%hx)\n"
                    ,u16sec+u8type,u16addr,u16chkodd,u16tvalueeven,u16tvalueodd);
            }
        }
        u16chkodd = u16tvalueodd;
        //Hal_VIP_WriteReg(u32reg, u16addr, 0x007F);//addr
        //Hal_VIP_WriteReg(u32reg, u16sec<<8, BIT8|BIT9|BIT10);//sec
        if((u16tvalueeven != u16tvalueevenori)||u32Sram==NULL)
        {
            Hal_VIP_WriteReg(u32reg+2, (MS_U16)u16tvalueeven, 0xFFF);//data
            Hal_VIP_WriteReg(u32reg, ((u16addr)|((MS_U16)u16sec<<8)|BIT11), 0xFFFF);//wen
            Hal_VIP_WriteReg(u32reg, ((u16addr)|((MS_U16)u16sec<<8)|BIT12), 0xFFFF);//ren
            if(!gbCMDQ)
            {
                SCL_DBG(SCL_DBG_LV_DRVVIP()&EN_DBGMG_VIPLEVEL_VIPLOG, "even val:%hx\n",u16tvalueeven);
                u16readdata=R2BYTE(u32reg+4);
                SCL_DBG(SCL_DBG_LV_DRVVIP()&EN_DBGMG_VIPLEVEL_VIPLOG, "read val:%hx\n",u16readdata);
                for(bRet = 0;bRet<10;bRet++)
                {
                    if(u16readdata!=u16tvalueeven)
                    {
                        SCL_DBGERR("[HALVIP]EvenRGBGamma:%hhx\n",u16sec+u8type);
                        SCL_DBGERR( "[HALVIP]even val:%hx\n",u16tvalueeven);
                        SCL_DBGERR( "[HALVIP]read val:%hx\n",u16readdata);
                        if(!Drv_VIP_GetIsBlankingRegion())
                        {
                            MsOS_WaitEvent(Drv_SCLIRQ_Get_IRQ_SYNCEventID(), E_SCLIRQ_EVENT_FRMENDSYNC, &u32Events, E_OR, 2000); // get status: FRM END
                        }
                        Hal_VIP_WriteReg(u32reg+2, (MS_U16)u16tvalueeven, 0xFFF);//data
                        Hal_VIP_WriteReg(u32reg, ((u16addr)|((MS_U16)u16sec<<8)|BIT11), 0xFFFF);//wen
                        Hal_VIP_WriteReg(u32reg, ((u16addr)|((MS_U16)u16sec<<8)|BIT12), 0xFFFF);//ren
                        u16readdata=R2BYTE(u32reg+4);
                    }
                    else
                    {
                        break;
                    }
                }
            }
        }
        //if(SCL_DBG_LV_DRVVIP()&EN_DBGMG_VIPLEVEL_VIPLOG)
        //Hal_VIP_WriteReg(u32reg, u16addr, 0x007F);//addr
        //Hal_VIP_WriteReg(u32reg, (u16sec+1)<<8, BIT8|BIT9|BIT10);//sec

        if((u16tvalueodd != u16tvalueoddori)||u32Sram==NULL)
        {
            Hal_VIP_WriteReg(u32reg+2, (MS_U16)u16tvalueodd, 0xFFF);//data
            //Hal_VIP_WriteReg(u32reg, BIT11, BIT11);//wen
            Hal_VIP_WriteReg(u32reg, ((u16addr)|(((MS_U16)u16sec+1)<<8)|BIT11), 0xFFFF);//wen
            Hal_VIP_WriteReg(u32reg, ((u16addr)|(((MS_U16)u16sec+1)<<8)|BIT12), 0xFFFF);//ren
            if(!gbCMDQ)
            {
                SCL_DBG(SCL_DBG_LV_DRVVIP()&EN_DBGMG_VIPLEVEL_VIPLOG, "odd val:%hx\n",u16tvalueodd);
                u16readdata=R2BYTE(u32reg+4);
                SCL_DBG(SCL_DBG_LV_DRVVIP()&EN_DBGMG_VIPLEVEL_VIPLOG, "read val:%hx\n",u16readdata);
                for(bRet = 0;bRet<10;bRet++)
                {
                    if(u16readdata!=u16tvalueodd)
                    {
                        SCL_DBGERR("[HALVIP]odd RGBGamma:%hhx\n",u16sec+u8type);
                        SCL_DBGERR( "[HALVIP]odd val:%hx\n",u16tvalueodd);
                        SCL_DBGERR( "[HALVIP]read val:%hx\n",u16readdata);
                        if(!Drv_VIP_GetIsBlankingRegion())
                        {
                            MsOS_WaitEvent(Drv_SCLIRQ_Get_IRQ_SYNCEventID(), E_SCLIRQ_EVENT_FRMENDSYNC, &u32Events, E_OR, 2000); // get status: FRM END
                        }
                        Hal_VIP_WriteReg(u32reg+2, (MS_U16)u16tvalueodd, 0xFFF);//data
                        Hal_VIP_WriteReg(u32reg, ((u16addr)|(((MS_U16)u16sec+1)<<8)|BIT11), 0xFFFF);//wen
                        Hal_VIP_WriteReg(u32reg, ((u16addr)|(((MS_U16)u16sec+1)<<8)|BIT12), 0xFFFF);//ren
                        u16readdata=R2BYTE(u32reg+4);
                    }
                    else
                    {
                        break;
                    }
                }
            }
        }

        u16tcount+=4;
    }
    Hal_VIP_WriteReg(u32reg+2, 0, 0xFFF);//data
    if(gbCMDQ)
    {
        SCL_DBG(SCL_DBG_LV_DRVCMDQ()&EN_DBGMG_CMDQEVEL_ISR, "gbCMDQ RGBGamma:%hhx\n",u16sec+u8type);
        Drv_CMDQ_Fire(EN_CMDQ_TYPE_IP0,1);
        if(endump == EN_VIP_SRAM_DUMP_GM10to12_R)
        {
            MsOS_SetEvent_IRQ(Drv_SCLIRQ_Get_IRQ_SYNCEventID(),  E_SCLIRQ_EVENT_GM10to12_R);
        }
        else if(endump == EN_VIP_SRAM_DUMP_GM10to12_G)
        {
            MsOS_SetEvent_IRQ(Drv_SCLIRQ_Get_IRQ_SYNCEventID(),  E_SCLIRQ_EVENT_GM10to12_G);
        }
        else if(endump == EN_VIP_SRAM_DUMP_GM10to12_B)
        {
            MsOS_SetEvent_IRQ(Drv_SCLIRQ_Get_IRQ_SYNCEventID(),  E_SCLIRQ_EVENT_GM10to12_B);
        }
        else if(endump == EN_VIP_SRAM_DUMP_GM12to10_R)
        {
            MsOS_SetEvent_IRQ(Drv_SCLIRQ_Get_IRQ_SYNCEventID(),  E_SCLIRQ_EVENT_GM12to10_R);
        }
        else if(endump == EN_VIP_SRAM_DUMP_GM12to10_G)
        {
            MsOS_SetEvent_IRQ(Drv_SCLIRQ_Get_IRQ_SYNCEventID(),  E_SCLIRQ_EVENT_GM12to10_G);
        }
        else if(endump == EN_VIP_SRAM_DUMP_GM12to10_B)
        {
            MsOS_SetEvent_IRQ(Drv_SCLIRQ_Get_IRQ_SYNCEventID(),  E_SCLIRQ_EVENT_GM12to10_B);
        }
    }
    Hal_VIP_SetWriteRegType(0);
    HAL_VIP_MUTEX_UNLOCK();
    return bRet;
}
MS_BOOL Hal_VIP_GetSRAMDumpGammaRGBCallback(EN_VIP_SRAM_DUMP_TYPE endump,void *u32Sram,MS_U32 u32reg)
{
    MS_U16  u16sec = 0;
    MS_U8  u8type = 0;
    MS_U8 flag = 0;
    MS_U8  bRet = 0;
    MS_U16 u16chkodd = 0;
    MS_U16 u16addr = 0,u16tvalueodd = 0,u16tvalueeven = 0,u16tcount = 0,u16readdata,u16size;
    static MS_U16 u16chkNum[6] = {0,0,0,0,0,0};
    switch(endump)
    {
        case EN_VIP_SRAM_DUMP_GM10to12_R:
            u16size = GAMMA10to12R_SRAM_BYTENUM;
            u16sec = 0;
            u8type = 0;
            break;
        case EN_VIP_SRAM_DUMP_GM10to12_G:
            u16size = GAMMA10to12G_SRAM_BYTENUM;
            u16sec = 2;
            u8type = 0;
            break;
        case EN_VIP_SRAM_DUMP_GM10to12_B:
            u16size = GAMMA10to12B_SRAM_BYTENUM;
            u16sec = 4;
            u8type = 0;
            break;
        case EN_VIP_SRAM_DUMP_GM12to10_R:
            u16size = GAMMA12to10R_SRAM_BYTENUM;
            u16sec = 0;
            u8type = 1;
            break;
        case EN_VIP_SRAM_DUMP_GM12to10_G:
            u16size = GAMMA12to10G_SRAM_BYTENUM;
            u16sec = 2;
            u8type = 1;
            break;
        case EN_VIP_SRAM_DUMP_GM12to10_B:
            u16size = GAMMA12to10B_SRAM_BYTENUM;
            u16sec = 4;
            u8type = 1;
            break;
        default:
            u16size = 0;
            u16sec = 0;
            u8type = 0;
            break;
    }
    SCL_DBG(SCL_DBG_LV_DRVCMDQ()&EN_DBGMG_CMDQEVEL_ISR, "RGBGamma:%hhx\n",u16sec+u8type);
    HAL_VIP_MUTEX_LOCK();
    u16tcount = u16chkNum[u16sec+u8type] * 4;
    for(u16addr=u16chkNum[u16sec+u8type];u16addr<u16size;u16addr++)
    {
        if(!Drv_VIP_GetIsBlankingRegion())
        {
            if(flag)
            {
                SCL_DBG(SCL_DBG_LV_DRVCMDQ()&EN_DBGMG_CMDQEVEL_DBG,"[ISRCheck]Fire\n");
                Hal_VIP_SetWriteRegType(1);
                Hal_VIP_WriteReg(u32reg+2, 0, 0xFFF);//data
                if(gbCMDQ)
                {
                    Drv_CMDQ_Fire(EN_CMDQ_TYPE_IP0,1);
                }
                Hal_VIP_SetWriteRegType(0);
            }
            MsOS_ClearEventIRQ(Drv_SCLIRQ_Get_IRQ_SYNCEventID(),(E_SCLIRQ_EVENT_FRMENDSYNC));
            HAL_VIP_MUTEX_UNLOCK();
            u16chkNum[u16sec+u8type] = u16addr;
            return 0;
        }
        u16tvalueeven = Hal_VIP_GetGammaRGBTvalue(u16sec, u16tcount,(MS_U8 *)u32Sram,u8type);
        u16tvalueodd = Hal_VIP_GetGammaRGBTvalue(u16sec, u16tcount+2,(MS_U8 *)u32Sram,u8type);
        Hal_VIP_WriteReg(u32reg, u16addr, 0x007F);//addr
        Hal_VIP_WriteReg(u32reg, u16sec<<8, BIT8|BIT9|BIT10);//sec
        Hal_VIP_WriteReg(u32reg, BIT12, BIT12);//ren
        u16readdata=R2BYTE(u32reg+4);
        SCL_DBG(SCL_DBG_LV_DRVCMDQ()&EN_DBGMG_CMDQEVEL_SRAMCheck, "RGBGamma:%hhx ",u16sec+u8type);
        SCL_DBG(SCL_DBG_LV_DRVCMDQ()&EN_DBGMG_CMDQEVEL_SRAMCheck, "addr:%hx ,even val:%hx ",u16addr,u16tvalueeven);
        SCL_DBG(SCL_DBG_LV_DRVCMDQ()&EN_DBGMG_CMDQEVEL_SRAMCheck, "read val:%hx\n",u16readdata);
        if(u16readdata != u16tvalueeven)
        {
            if(u16tcount>= 2)
            {
                u16chkodd = Hal_VIP_GetGammaRGBTvalue(u16sec, u16tcount-2,(MS_U8 *)u32Sram,u8type);
            }
            else
            {
                u16chkodd = Hal_VIP_GetGammaRGBTvalue(u16sec, u16tcount,(MS_U8 *)u32Sram,u8type);
            }
            bRet = _Hal_VIP_CheckMonotonicallyIncreasing(u16readdata,u16tvalueodd,u16chkodd);
            SCL_DBG(SCL_DBG_LV_DRVCMDQ()&EN_DBGMG_CMDQEVEL_DBG,
                "[ISRCheck]RGBGamma:%hhx read val:%hx odd val:%hx ckodd val:%hx addr:%hd\n"
                ,u16sec+u8type,u16readdata,u16tvalueodd,u16chkodd,u16addr);
            if(!bRet)
            {
                Hal_VIP_SetWriteRegType(1);
                flag = gbCMDQ;
                SCL_DBG(SCL_DBG_LV_DRVCMDQ()&EN_DBGMG_CMDQEVEL_ISRCheck, "[ISRCheck]RGBGamma:%hhx ",u16sec+u8type);
                SCL_DBG(SCL_DBG_LV_DRVCMDQ()&EN_DBGMG_CMDQEVEL_ISRCheck, "addr:%hx ,even val:%hx ",u16addr,u16tvalueeven);
                SCL_DBG(SCL_DBG_LV_DRVCMDQ()&EN_DBGMG_CMDQEVEL_ISRCheck, "read val:%hx\n",u16readdata);
                Hal_VIP_WriteReg(u32reg+2, (MS_U16)u16tvalueeven, 0xFFF);//data
                Hal_VIP_WriteReg(u32reg, ((u16addr)|((MS_U16)u16sec<<8)|BIT11), 0xFFFF);//wen
                Hal_VIP_WriteReg(u32reg, ((u16addr)|((MS_U16)u16sec<<8)|BIT12), 0xFFFF);//ren
                Hal_VIP_SetWriteRegType(0);
            }
        }
        Hal_VIP_WriteReg(u32reg, u16addr, 0x007F);//addr
        Hal_VIP_WriteReg(u32reg, (u16sec+1)<<8, BIT8|BIT9|BIT10);//sec
        Hal_VIP_WriteReg(u32reg, BIT12, BIT12);//ren
        u16readdata=R2BYTE(u32reg+4);
        SCL_DBG(SCL_DBG_LV_DRVCMDQ()&EN_DBGMG_CMDQEVEL_SRAMCheck, "RGBGamma:%hhx ",u16sec+u8type);
        SCL_DBG(SCL_DBG_LV_DRVCMDQ()&EN_DBGMG_CMDQEVEL_SRAMCheck, "addr:%hx ,odd val:%hx ",u16addr,u16tvalueodd);
        SCL_DBG(SCL_DBG_LV_DRVCMDQ()&EN_DBGMG_CMDQEVEL_SRAMCheck, "read val:%hx\n",u16readdata);
        if(u16readdata != u16tvalueodd)
        {
            if(u16tcount< 4*(u16size-1))
            {
                u16chkodd = Hal_VIP_GetGammaRGBTvalue(u16sec, u16tcount+4,(MS_U8 *)u32Sram,u8type);
            }
            else
            {
                u16chkodd = Hal_VIP_GetGammaRGBTvalue(u16sec, u16tcount+2,(MS_U8 *)u32Sram,u8type);
            }
            bRet = _Hal_VIP_CheckMonotonicallyIncreasing(u16readdata,u16chkodd,u16tvalueeven);
            SCL_DBG(SCL_DBG_LV_DRVCMDQ()&EN_DBGMG_CMDQEVEL_DBG,
                "[ISRCheck]RGBGamma:%hhx read val:%hx odd val:%hx ckeven val:%hx addr:%hd\n"
                ,u16sec+u8type,u16readdata,u16chkodd,u16tvalueeven,u16addr);
            if(!bRet)
            {
                Hal_VIP_SetWriteRegType(1);
                flag = gbCMDQ;
                SCL_DBG(SCL_DBG_LV_DRVCMDQ()&EN_DBGMG_CMDQEVEL_ISRCheck, "[ISRCheck]RGBGamma:%hhx ",u16sec+u8type);
                SCL_DBG(SCL_DBG_LV_DRVCMDQ()&EN_DBGMG_CMDQEVEL_ISRCheck, "addr:%hx ,odd val:%hx ",u16addr,u16tvalueodd);
                SCL_DBG(SCL_DBG_LV_DRVCMDQ()&EN_DBGMG_CMDQEVEL_ISRCheck, "read val:%hx\n",u16readdata);
                Hal_VIP_WriteReg(u32reg+2, (MS_U16)u16tvalueodd, 0xFFF);//data
                Hal_VIP_WriteReg(u32reg, ((u16addr)|(((MS_U16)u16sec+1)<<8)|BIT11), 0xFFFF);//wen
                Hal_VIP_WriteReg(u32reg, ((u16addr)|(((MS_U16)u16sec+1)<<8)|BIT12), 0xFFFF);//ren
                Hal_VIP_SetWriteRegType(0);
            }
        }
        u16tcount+=4;
    }
    u16chkNum[u16sec+u8type] = 0;
    if(flag)
    {
        SCL_DBG(SCL_DBG_LV_DRVCMDQ()&EN_DBGMG_CMDQEVEL_DBG,"[ISRCheck]Fire\n");
        Hal_VIP_SetWriteRegType(1);
        Hal_VIP_WriteReg(u32reg+2, 0, 0xFFF);//data
        if(gbCMDQ)
        {
            Drv_CMDQ_Fire(EN_CMDQ_TYPE_IP0,1);
        }
        Hal_VIP_SetWriteRegType(0);
    }
    HAL_VIP_MUTEX_UNLOCK();
    return 1;
}
MS_BOOL Hal_VIP_GetSRAMDumpGammaRGB(EN_VIP_SRAM_DUMP_TYPE endump,void *u32Sram,MS_U32 u32reg)
{
    MS_U16  u16sec = 0;
    MS_U8  u8type = 0;
    u8 flag = 0;
    MS_U16 u16addr = 0,u16tvalueodd = 0,u16tvalueeven = 0,u16tcount = 0,u16readdata,u16size;
    switch(endump)
    {
        case EN_VIP_SRAM_DUMP_GM10to12_R:
            u16size = GAMMA10to12R_SRAM_BYTENUM;
            u16sec = 0;
            u8type = 0;
            break;
        case EN_VIP_SRAM_DUMP_GM10to12_G:
            u16size = GAMMA10to12G_SRAM_BYTENUM;
            u16sec = 2;
            u8type = 0;
            break;
        case EN_VIP_SRAM_DUMP_GM10to12_B:
            u16size = GAMMA10to12B_SRAM_BYTENUM;
            u16sec = 4;
            u8type = 0;
            break;
        case EN_VIP_SRAM_DUMP_GM12to10_R:
            u16size = GAMMA12to10R_SRAM_BYTENUM;
            u16sec = 0;
            u8type = 1;
            break;
        case EN_VIP_SRAM_DUMP_GM12to10_G:
            u16size = GAMMA12to10G_SRAM_BYTENUM;
            u16sec = 2;
            u8type = 1;
            break;
        case EN_VIP_SRAM_DUMP_GM12to10_B:
            u16size = GAMMA12to10B_SRAM_BYTENUM;
            u16sec = 4;
            u8type = 1;
            break;
        default:
            u16size = 0;
            u16sec = 0;
            u8type = 0;
            break;
    }
    SCL_DBG(SCL_DBG_LV_DRVCMDQ()&EN_DBGMG_CMDQEVEL_ISR, "RGBGamma:%hhx\n",u16sec+u8type);
    HAL_VIP_MUTEX_LOCK();
    for(u16addr=0;u16addr<u16size;u16addr++)
    {
        if(!Drv_VIP_GetIsBlankingRegion())
        {
            HAL_VIP_MUTEX_UNLOCK();
            return 0;
        }
        u16tvalueeven = Hal_VIP_GetGammaRGBTvalue(u16sec, u16tcount,(MS_U8 *)u32Sram,u8type);
        u16tvalueodd = Hal_VIP_GetGammaRGBTvalue(u16sec, u16tcount+2,(MS_U8 *)u32Sram,u8type);
        Hal_VIP_WriteReg(u32reg, u16addr, 0x007F);//addr
        Hal_VIP_WriteReg(u32reg, u16sec<<8, BIT8|BIT9|BIT10);//sec
        Hal_VIP_WriteReg(u32reg, BIT12, BIT12);//ren
        u16readdata=R2BYTE(u32reg+4);
        SCL_DBG(SCL_DBG_LV_DRVCMDQ()&EN_DBGMG_CMDQEVEL_SRAMCheck, "RGBGamma:%hhx ",u16sec+u8type);
        SCL_DBG(SCL_DBG_LV_DRVCMDQ()&EN_DBGMG_CMDQEVEL_SRAMCheck, "addr:%hx ,even val:%hx ",u16addr,u16tvalueeven);
        SCL_DBG(SCL_DBG_LV_DRVCMDQ()&EN_DBGMG_CMDQEVEL_SRAMCheck, "read val:%hx\n",u16readdata);
        if(u16readdata != u16tvalueeven)
        {
            Hal_VIP_SetWriteRegType(1);
            flag = gbCMDQ;
            u16readdata=R2BYTE(u32reg+4);
            if(Drv_VIP_GetIsBlankingRegion())
            {
                SCL_DBGERR("[ISRCheck]RGBGamma:%hhx ",u16sec+u8type);
                SCL_DBG(SCL_DBG_LV_DRVCMDQ()&EN_DBGMG_CMDQEVEL_ISRCheck, "addr:%hx ,even val:%hx ",u16addr,u16tvalueeven);
                SCL_DBG(SCL_DBG_LV_DRVCMDQ()&EN_DBGMG_CMDQEVEL_ISRCheck, "read val:%hx\n",u16readdata);
            }
            Hal_VIP_WriteReg(u32reg+2, (MS_U16)u16tvalueeven, 0xFFF);//data
            Hal_VIP_WriteReg(u32reg, ((u16addr)|((MS_U16)u16sec<<8)|BIT11), 0xFFFF);//wen
            Hal_VIP_WriteReg(u32reg, ((u16addr)|((MS_U16)u16sec<<8)|BIT12), 0xFFFF);//ren
            Hal_VIP_SetWriteRegType(0);
        }
        Hal_VIP_WriteReg(u32reg, u16addr, 0x007F);//addr
        Hal_VIP_WriteReg(u32reg, (u16sec+1)<<8, BIT8|BIT9|BIT10);//sec
        Hal_VIP_WriteReg(u32reg, BIT12, BIT12);//ren
        u16readdata=R2BYTE(u32reg+4);
        SCL_DBG(SCL_DBG_LV_DRVCMDQ()&EN_DBGMG_CMDQEVEL_SRAMCheck, "RGBGamma:%hhx ",u16sec+u8type);
        SCL_DBG(SCL_DBG_LV_DRVCMDQ()&EN_DBGMG_CMDQEVEL_SRAMCheck, "addr:%hx ,odd val:%hx ",u16addr,u16tvalueodd);
        SCL_DBG(SCL_DBG_LV_DRVCMDQ()&EN_DBGMG_CMDQEVEL_SRAMCheck, "read val:%hx\n",u16readdata);
        if(u16readdata != u16tvalueodd)
        {
            Hal_VIP_SetWriteRegType(1);
            u16readdata=R2BYTE(u32reg+4);
            flag = gbCMDQ;
            if(Drv_VIP_GetIsBlankingRegion())
            {
                SCL_DBGERR("[ISRCheck]RGBGamma:%hhx ",u16sec+u8type);
                SCL_DBG(SCL_DBG_LV_DRVCMDQ()&EN_DBGMG_CMDQEVEL_ISRCheck, "addr:%hx ,odd val:%hx ",u16addr,u16tvalueodd);
                SCL_DBG(SCL_DBG_LV_DRVCMDQ()&EN_DBGMG_CMDQEVEL_ISRCheck, "read val:%hx\n",u16readdata);
            }
            Hal_VIP_WriteReg(u32reg+2, (MS_U16)u16tvalueodd, 0xFFF);//data
            Hal_VIP_WriteReg(u32reg, ((u16addr)|(((MS_U16)u16sec+1)<<8)|BIT11), 0xFFFF);//wen
            Hal_VIP_WriteReg(u32reg, ((u16addr)|(((MS_U16)u16sec+1)<<8)|BIT12), 0xFFFF);//ren
            Hal_VIP_SetWriteRegType(0);
        }
        u16tcount+=4;
    }
    if(flag)
    {
        Drv_CMDQ_Fire(EN_CMDQ_TYPE_IP0,1);
    }
    HAL_VIP_MUTEX_UNLOCK();
    return 1;
}

MS_BOOL Hal_VIP_SetSRAMDump(EN_VIP_SRAM_DUMP_TYPE endump,void* u32Sram,MS_U32 u32reg)
{
    if(endump == EN_VIP_SRAM_DUMP_IHC || endump == EN_VIP_SRAM_DUMP_ICC)
    {
        Hal_VIP_SRAMDumpIHCICC(endump,u32reg);
    }
    else if(endump == EN_VIP_SRAM_DUMP_WDR)
    {
        Hal_VIP_SRAMDumpWDR(u32reg,u32Sram);
    }
    else if(endump == EN_VIP_SRAM_DUMP_HVSP_V || endump == EN_VIP_SRAM_DUMP_HVSP_V_1||
        endump == EN_VIP_SRAM_DUMP_HVSP_V_2)
    {
        Hal_VIP_SRAMDumpVSP((MS_U32)u32Sram,u32reg);
    }
    else if(endump == EN_VIP_SRAM_DUMP_HVSP_H || endump == EN_VIP_SRAM_DUMP_HVSP_H_1||
        endump == EN_VIP_SRAM_DUMP_HVSP_H_2)
    {
        Hal_VIP_SRAMDumpHSP((MS_U32)u32Sram,u32reg);
    }
    else if(endump == EN_VIP_SRAM_DUMP_GAMMA_Y || endump == EN_VIP_SRAM_DUMP_GAMMA_U||
        endump == EN_VIP_SRAM_DUMP_GAMMA_V)
    {
        return Hal_VIP_SRAMDumpGammaYUV(endump,u32Sram,u32reg);
    }
    else
    {
        return Hal_VIP_SRAMDumpGammaRGB(endump,u32Sram,u32reg);
    }
    return 1;
}
MS_BOOL Hal_VIP_SRAM_Dump(EN_VIP_SRAM_DUMP_TYPE endump,void * u32Sram)
{
    MS_U16 u16clkreg;
    MS_BOOL bRet;
    MS_U32 u32reg = endump == EN_VIP_SRAM_DUMP_IHC ? REG_VIP_ACE2_7C_L :
                    endump == EN_VIP_SRAM_DUMP_ICC ?  REG_VIP_ACE2_78_L :
                    endump == EN_VIP_SRAM_DUMP_HVSP_V ? REG_SCL_HVSP0_41_L:
                    endump == EN_VIP_SRAM_DUMP_HVSP_V_1 ? REG_SCL_HVSP1_41_L:
                    endump == EN_VIP_SRAM_DUMP_HVSP_V_2 ? REG_SCL_HVSP2_41_L:
                    endump == EN_VIP_SRAM_DUMP_HVSP_H ? REG_SCL_HVSP0_41_L:
                    endump == EN_VIP_SRAM_DUMP_HVSP_H_1 ? REG_SCL_HVSP1_41_L:
                    endump == EN_VIP_SRAM_DUMP_HVSP_H_2 ? REG_SCL_HVSP2_41_L:
                    endump == EN_VIP_SRAM_DUMP_WDR ?  REG_VIP_WDR_78_L :
                    endump == EN_VIP_SRAM_DUMP_GAMMA_Y ?  REG_VIP_SCNR_41_L :
                    endump == EN_VIP_SRAM_DUMP_GAMMA_U ?  REG_VIP_SCNR_41_L :
                    endump == EN_VIP_SRAM_DUMP_GAMMA_V ?  REG_VIP_SCNR_41_L :
                    endump == EN_VIP_SRAM_DUMP_GM10to12_R ?  REG_SCL_HVSP1_7A_L :
                    endump == EN_VIP_SRAM_DUMP_GM10to12_G ?  REG_SCL_HVSP1_7A_L :
                    endump == EN_VIP_SRAM_DUMP_GM10to12_B ?  REG_SCL_HVSP1_7A_L :
                    endump == EN_VIP_SRAM_DUMP_GM12to10_R ?  REG_SCL_HVSP1_7D_L :
                    endump == EN_VIP_SRAM_DUMP_GM12to10_G ?  REG_SCL_HVSP1_7D_L :
                    endump == EN_VIP_SRAM_DUMP_GM12to10_B ?  REG_SCL_HVSP1_7D_L :
                        0;
    //clk open
    if(endump == EN_VIP_SRAM_DUMP_HVSP_V_2 || endump == EN_VIP_SRAM_DUMP_HVSP_H_2)
    {
        u16clkreg = R2BYTE(REG_SCL_CLK_65_L);
        W2BYTE(REG_SCL_CLK_65_L,0x0);
    }
    else
    {
        u16clkreg = R2BYTE(REG_SCL_CLK_64_L);
        W2BYTE(REG_SCL_CLK_64_L,0x0);
    }
    bRet = Hal_VIP_SetSRAMDump(endump,u32Sram,u32reg);

    //clk close
    if(endump == EN_VIP_SRAM_DUMP_HVSP_V_2|| endump == EN_VIP_SRAM_DUMP_HVSP_H_2)
    {
        W2BYTE(REG_SCL_CLK_65_L,u16clkreg);
    }
    else
    {
        W2BYTE(REG_SCL_CLK_64_L,u16clkreg);
    }
    return bRet;
}

//-----------------------DLC
void Hal_VIP_DLCHistVarOnOff(MS_U16 u16var)
{
    W2BYTEMSK(REG_VIP_DLC_04_L,u16var,0x25a2);
}

void Hal_VIP_SetDLCstatMIU(MS_U8 u8value,MS_U32 u32addr1,MS_U32 u32addr2)
{
    W2BYTEMSK(REG_VIP_MWE_15_L,u8value,0x0001);
    W4BYTE(REG_VIP_MWE_18_L,u32addr1>>4);
    W4BYTE(REG_VIP_MWE_1A_L,u32addr2>>4);
}
void Hal_VIP_SetDLCshift(MS_U8 u8value)
{
    W2BYTEMSK(REG_VIP_DLC_03_L,u8value,0x0007);
}
void HAl_VIP_SetDLCmode(MS_U8 u8value)
{
    W2BYTEMSK(REG_VIP_MWE_1C_L,u8value<<2,0x0004);
}
void Hal_VIP_SetDLCActWin(MS_BOOL bEn,MS_U16 u16Vst,MS_U16 u16Hst,MS_U16 u16Vnd,MS_U16 u16Hnd)
{
    W2BYTEMSK(REG_VIP_DLC_08_L,bEn<<7,0x0080);
    W2BYTEMSK(REG_VIP_MWE_01_L,u16Vst,0x03FF);
    W2BYTEMSK(REG_VIP_MWE_02_L,u16Vnd,0x03FF);
    W2BYTEMSK(REG_VIP_MWE_03_L,u16Hst,0x01FF);
    W2BYTEMSK(REG_VIP_MWE_04_L,u16Hnd,0x01FF);
}
void Hal_VIP_DLCHistSetRange(MS_U8 u8value,MS_U8 u8range)
{
    MS_U16 u16tvalue;
    MS_U16 u16Mask;
    MS_U32 u32Reg;
    u8range = u8range-1;
    u32Reg = REG_VIP_DLC_0C_L+(((u8range)/2)*2);
    if((u8range%2) == 0)
    {
        u16Mask     = 0x00FF;
        u16tvalue   = ((MS_U16)u8value);

    }
    else
    {
        u16Mask     = 0xFF00;
        u16tvalue   = ((MS_U16)u8value)<<8;
    }

    W2BYTEMSK(u32Reg,(u16tvalue),u16Mask);

}
MS_U32 Hal_VIP_DLC_Hist_GetRange(MS_U8 u8range)
{
    MS_U32 u32tvalue;
    MS_U32 u32Reg;
    u32Reg      = REG_VIP_MWE_20_L+(((u8range)*2)*2);
    u32tvalue   = R4BYTE(u32Reg);
    return u32tvalue;

}

MS_U8 Hal_VIP_DLCGetBaseidx(void)
{
    MS_U8 u8tvalue;
    u8tvalue = R2BYTE(REG_VIP_MWE_15_L);
    u8tvalue = (MS_U8)(u8tvalue&0x80)>>7;
    return u8tvalue;
}

MS_U32 Hal_VIP_DLCGetPC(void)
{
    MS_U32 u32tvalue;
    u32tvalue = R4BYTE(REG_VIP_MWE_08_L);
    return u32tvalue;
}

MS_U32 Hal_VIP_DLCGetPW(void)
{
    MS_U32 u32tvalue;
    u32tvalue = R4BYTE(REG_VIP_MWE_0A_L);
    return u32tvalue;
}

MS_U8 Hal_VIP_DLCGetMinP(void)
{
    MS_U16 u16tvalue;
    u16tvalue = R2BYTE(REG_VIP_DLC_62_L);
    u16tvalue = (u16tvalue>>8);
    return (MS_U8)u16tvalue;
}

MS_U8 Hal_VIP_DLCGetMaxP(void)
{
    MS_U16 u16tvalue;
    u16tvalue = R2BYTE(REG_VIP_DLC_62_L);
    return (MS_U8)u16tvalue;
}
// LDC frame control
void Hal_VIP_SetLDCBypass(MS_BOOL bEn)
{
    W2BYTEMSK(REG_SCL_LDC_0F_L, bEn ? BIT0 : 0, BIT0);
}


void Hal_VIP_SetLDCHWrwDiff(MS_U16 u8Val)
{
    W2BYTEMSK(REG_SCL_LDC_11_L, ((MS_U16)u8Val), BIT1|BIT0);
}
void Hal_VIP_SetLDCSW_Idx(MS_U8 idx)
{
    W2BYTEMSK(REG_SCL_LDC_10_L, ((MS_U16)idx), BIT1|BIT0);
}
void Hal_VIP_SetLDCSW_Mode(MS_U8 bEn)
{
    W2BYTEMSK(REG_SCL_LDC_12_L, ((MS_U16)bEn)? BIT0 :0, BIT0);
}
void Hal_VIP_SetLDCBank_Mode(EN_DRV_VIP_LDCLCBANKMODE_TYPE enType)
{
    W2BYTEMSK(REG_SCL_LDC_09_L, ((MS_U16)enType == EN_DRV_VIP_LDCLCBANKMODE_64)? BIT0 :0, BIT0);
}

void Hal_VIP_SetLDCDmapBase(MS_U32 u32Base)
{
    W4BYTE(REG_SCL_LDC_0A_L, u32Base);
}

void Hal_VIP_SetLDCDmapPitch(MS_U32 u32Pitch)
{
    W4BYTE(REG_SCL_LDC_0C_L, u32Pitch);
}
void Hal_VIP_MCNRInit(void)
{
    W2BYTEMSK(REG_SCL_DNR1_7F_L, 0x0007, BIT2|BIT1|BIT0); //for I3e ECO
}
void Hal_VIP_LDCECO(void)
{
    W2BYTEMSK(REG_SCL_LDC_31_L, BIT0, BIT0); //for I3e ECO
}
void Hal_VIP_AIPDB(MS_U8 u8En)
{
    W2BYTEMSK(REG_VIP_SCNR_7F_L, u8En ? 0 : BIT0, BIT0);
}
void Hal_VIP_SetLDC422_444_Md(MS_U8 u8md)
{
    W2BYTEMSK(REG_SCL_LDC_1D_L, (MS_U16)u8md, BIT1|BIT0);
}
void Hal_VIP_SetLDC444_422_Md(MS_U8 u8md)
{
    W2BYTEMSK(REG_SCL_LDC_1F_L, (MS_U16)u8md, BIT1|BIT0);
}

void Hal_VIP_SetLDCDmapOffset(MS_U8 u8offset)
{
    W2BYTEMSK(REG_SCL_LDC_0E_L, (u8offset), 0x003F);
}

void Hal_VIP_SetLDCDmapPS(MS_U8 bEn)
{
    W2BYTEMSK(REG_SCL_LDC_0E_L, (bEn<<6), BIT6);
}
void Hal_VIP_SetLDCSramAmount(MS_U16 u16hor,MS_U16 u16ver)
{
    W2BYTE(REG_SCL_LDC_22_L, ((MS_U16)u16hor));
    W2BYTE(REG_SCL_LDC_2A_L, ((MS_U16)u16ver));
}
void Hal_VIP_SetLDCSramBase(MS_U32 u32hor,MS_U32 u32ver)
{
    W4BYTE(REG_SCL_LDC_24_L, (u32hor>>4));
    W4BYTE(REG_SCL_LDC_2C_L, (u32ver>>4));
}
void Hal_VIP_SetLDCSramStr(MS_U16 u16hor,MS_U16 u16ver)
{

    W2BYTE(REG_SCL_LDC_23_L, ((MS_U16)u16hor));
    W2BYTE(REG_SCL_LDC_2B_L, ((MS_U16)u16ver));
}

void Hal_VIP_SetAutodownloadAddr(MS_U32 u32baseadr,MS_U16 u16iniaddr,MS_U8 u8cli)
{
    switch(u8cli)
    {
        case 9:
            W2BYTE(REG_SCL1_73_L, (MS_U16)(u32baseadr>>4));
            W2BYTEMSK(REG_SCL1_74_L, (MS_U16)(u32baseadr>>20), 0x01FF);
            W2BYTE(REG_SCL1_77_L, ((MS_U16)u16iniaddr));
            break;
        default:
            SCL_ERR("[HALVIP]default\n");
            return;
    }
}

void Hal_VIP_SetAutodownloadReq(MS_U16 u16depth,MS_U16 u16reqlen,MS_U8 u8cli)
{
    switch(u8cli)
    {
        case 9:
            W2BYTE(REG_SCL1_76_L, ((MS_U16)u16reqlen));
            W2BYTE(REG_SCL1_75_L, ((MS_U16)u16depth));
            break;
        default:
            SCL_ERR("[HALVIP]default\n");
            return;
    }
}

void Hal_VIP_SetAutodownload(MS_U8 bCLientEn,MS_U8 btrigContinue,MS_U8 u8cli)
{
    switch(u8cli)
    {
        case 9:
            W2BYTEMSK(REG_SCL1_72_L, bCLientEn|(btrigContinue<<1), 0x0003);
            break;
        default:
            SCL_ERR("[HALVIP]default\n");
            return;
    }
}

void Hal_VIP_SetAutodownloadTimer(MS_U8 bCLientEn)
{
    W2BYTEMSK(REG_SCL1_78_L, bCLientEn<<15, 0x8000);
}
void Hal_VIP_GetNLMSRAM(MS_U16 u16entry)
{
    MS_U32 u32tvalue1,u32tvalue2;
    W2BYTEMSK(REG_SCL_NLM0_62_L, u16entry, 0x07FF);
    W2BYTEMSK(REG_SCL_NLM0_62_L, 0x8000, 0x8000);
    W2BYTEMSK(REG_SCL_NLM0_62_L, 0x2000, 0x2000);
    u32tvalue1 = R2BYTE(REG_SCL_NLM0_64_L);
    u32tvalue2 = R2BYTE(REG_SCL_NLM0_65_L);
    u32tvalue1 |= ((u32tvalue2&0x00F0)<<12);
    W2BYTEMSK(REG_SCL_NLM0_62_L, 0x0000, 0x8000);
    SCL_DBG(SCL_DBG_LV_DRVVIP()&EN_DBGMG_VIPLEVEL_VIPLOG, "[Get_SRAM]entry%hx :%lx\n",u16entry,u32tvalue1);
}

void Hal_VIP_SetNLMSRAMbyCPU(MS_U16 u16entry,MS_U32 u32tvalue)
{
    W2BYTEMSK(REG_SCL_NLM0_62_L, u16entry, 0x07FF);
    W2BYTEMSK(REG_SCL_NLM0_62_L, 0x8000, 0x8000);
    W2BYTE(REG_SCL_NLM0_63_L, (MS_U16)u32tvalue);
    W2BYTEMSK(REG_SCL_NLM0_65_L, (MS_U16)(u32tvalue>>16), 0x000F);
    W2BYTEMSK(REG_SCL_NLM0_62_L, 0x4000, 0x4000);
    W2BYTEMSK(REG_SCL_NLM0_62_L, 0x0000, 0x8000);
    SCL_DBG(SCL_DBG_LV_DRVVIP()&EN_DBGMG_VIPLEVEL_VIPLOG, "[Set_SRAM]entry%hx :%lx\n",u16entry,u32tvalue);
}

#define  VTRACK_KEY_SETTING_LENGTH 8

static MS_U8 u8VtrackKey[VTRACK_KEY_SETTING_LENGTH]=
{
    0xaa,0x13,0x46,0x90,0x28,0x35,0x29,0xFE,
};

void Hal_VIP_VtrackSetKey(MS_BOOL bUserDefinded, MS_U8 *pu8Setting)
{
    /*
     * Default Setting:
     * setting1 [0e]              => 8'h00
     * setting2 [0f]              => 8'h00
     * setting3 [1a:10]           => 165'h04 21 08 418c6318c4 21084210842108421086318c53
     */

    MS_U8 *pu8Data = NULL;
    MS_U16 u16Index = 0;
    MS_U16 u16Data = 0;

    if (bUserDefinded == TRUE)
    {
        pu8Data = pu8Setting;
    }
    else
    {
        pu8Data = &u8VtrackKey[0];
    }
    for (u16Index = 0; u16Index < VTRACK_KEY_SETTING_LENGTH; u16Index = u16Index+2)
    {
        MS_U8 u8Offset = (( (u16Index) / 2) *2 );
        MS_U32 u32Addr = REG_SCL1_24_L + u8Offset;

        if ( (VTRACK_KEY_SETTING_LENGTH - u16Index) >= 2)
        {
            u16Data = (MS_U16)pu8Data[u16Index +1];
            u16Data = (u16Data << 8) + (MS_U16) pu8Data[u16Index];
            W2BYTE(u32Addr, u16Data);
        }
    }

}

void Hal_VIP_VtrackSetPayloadData(MS_U16 u16Timecode, MS_U8 u8OperatorID)
{
    /*
     * reg_payload_use from 0x28 to 0x2C (1B - 1F)
     * {14'h0, TimeCode[55:40], Unique ID (OTP)[39:8], Operator ID[7:0]}
     */

    W2BYTEMSK(REG_SCL1_28_L, (MS_U16)u8OperatorID, 0x00ff);
    W2BYTEMSK(REG_SCL1_2A_L, (u16Timecode << 8) , 0xff00);
    W2BYTEMSK(REG_SCL1_2B_L, (u16Timecode >> 8) , 0x00ff);

}

#define  VTRACK_SETTING_LENGTH 23

static MS_U8 u8VtrackSetting[VTRACK_SETTING_LENGTH]=
{
    0x00, 0x00, 0xe8, 0x18, 0x32,
    0x86, 0x10, 0x42, 0x08, 0x21,
    0x84, 0x10, 0x42, 0x08, 0x21,
    0xc4, 0x18, 0x63, 0x8c, 0x41,
    0x08, 0x21, 0x04,
};

void Hal_VIP_VtrackSetUserDefindedSetting(MS_BOOL bUserDefinded, MS_U8 *pu8Setting)
{
    /*
     * Default Setting:
     * setting1 [0e]              => 8'h00
     * setting2 [0f]              => 8'h00
     * setting3 [1a:10]           => 165'h04 21 08 418c6318c4 21084210842108421086318c53
     */

    MS_U8 *pu8Data = NULL;
    MS_U16 u16Index = 0;
    MS_U16 u16Data = 0;

    if (bUserDefinded == TRUE)
    {
        pu8Data = pu8Setting;
    }
    else
    {
        pu8Data = &u8VtrackSetting[0];
    }
    //MenuLoad enable
    W2BYTEMSK(REG_SCL1_2E_L, pu8Data[0], 0xff);
    W2BYTEMSK(REG_SCL1_2F_L, pu8Data[1], 0xff);
    for (u16Index = 2; u16Index < VTRACK_SETTING_LENGTH; u16Index = u16Index+2)
    {
        MS_U8 u8Offset = (( (u16Index - 2) / 2) *2 );
        MS_U32 u32Addr = REG_SCL1_30_L + u8Offset;

        if ( (VTRACK_SETTING_LENGTH - u16Index) >= 2)
        {
            u16Data = (MS_U16)pu8Data[u16Index +1];
            u16Data = (u16Data << 8) + (MS_U16) pu8Data[u16Index];
            W2BYTE(u32Addr, u16Data);
        }
        else
        {
            u16Data = (MS_U16) pu8Data[u16Index];
            W2BYTEMSK(u32Addr, u16Data, 0x00ff);
        }
    }

}

#define VIP_VTRACK_MODE 0x1c   //[3]:v_sync_inv_en ;[2]:h_sync_inv_en [4]mux
void Hal_VIP_VtrackEnable(MS_U8 u8FrameRate, EN_VIP_VTRACK_ENABLE_TYPE bEnable)
{
    //FrameRateIn     => 8'h1E
    W2BYTEMSK(REG_SCL1_21_L , u8FrameRate, 0xFF);

    //EnableIn              => 1
    //DebugEn               => 0
    if (bEnable == EN_VIP_VTRACK_ENABLE_ON)
    {
        W2BYTEMSK(REG_SCL1_20_L , BIT(0), BIT(1)|BIT(0) );
        W2BYTEMSK(REG_SCL1_22_L , VIP_VTRACK_MODE, 0x3F );
    }
    else if(bEnable == EN_VIP_VTRACK_ENABLE_DEBUG)
    {
        W2BYTEMSK(REG_SCL1_20_L , BIT(1)|BIT(0), BIT(1)|BIT(0) );
        W2BYTEMSK(REG_SCL1_22_L , VIP_VTRACK_MODE, 0x3F );
    }
    else
    {
        W2BYTEMSK(REG_SCL1_20_L , 0x00, BIT(1)|BIT(0) );
        W2BYTEMSK(REG_SCL1_22_L , VIP_VTRACK_MODE, 0x3F );
    }
}
void Hal_VIP_InitY2R(void)
{
    W2BYTEMSK(REG_SCL_HVSP2_60_L, 0x0A01, 0x1FFF);
    W2BYTEMSK(REG_SCL_HVSP2_61_L, 0x59E, 0x1FFF);
    W2BYTEMSK(REG_SCL_HVSP2_62_L, 0x401, 0x1FFF);
    W2BYTEMSK(REG_SCL_HVSP2_63_L, 0x1FFF, 0x1FFF);
    W2BYTEMSK(REG_SCL_HVSP2_64_L, 0x1D24, 0x1FFF);
    W2BYTEMSK(REG_SCL_HVSP2_65_L, 0x400, 0x1FFF);
    W2BYTEMSK(REG_SCL_HVSP2_66_L, 0x1E9F, 0x1FFF);
    W2BYTEMSK(REG_SCL_HVSP2_67_L, 0x1FFF, 0x1FFF);
    W2BYTEMSK(REG_SCL_HVSP2_68_L, 0x400, 0x1FFF);
    W2BYTEMSK(REG_SCL_HVSP2_69_L, 0x719, 0x1FFF);
    W2BYTEMSK(REG_SCL_HVSP1_6C_L, 0x181, 0x1FFF);
    W2BYTEMSK(REG_SCL_HVSP1_6D_L, 0x1FF, 0x1FFF);
    W2BYTEMSK(REG_SCL_HVSP1_6E_L, 0x1E54, 0x1FFF);
    W2BYTEMSK(REG_SCL_HVSP1_6F_L, 0x1FAD, 0x1FFF);
    W2BYTEMSK(REG_SCL_HVSP1_70_L, 0x132, 0x1FFF);
    W2BYTEMSK(REG_SCL_HVSP1_71_L, 0x259, 0x1FFF);
    W2BYTEMSK(REG_SCL_HVSP1_72_L, 0x75, 0x1FFF);
    W2BYTEMSK(REG_SCL_HVSP1_73_L, 0x1F53, 0x1FFF);
    W2BYTEMSK(REG_SCL_HVSP1_74_L, 0x1EAD, 0x1FFF);
    W2BYTEMSK(REG_SCL_HVSP1_75_L, 0x1FF, 0x1FFF);
}

MS_BOOL Hal_VIP_GetVIPBypass(void)
{
    MS_BOOL bRet;
    bRet = R2BYTE(REG_VIP_LCE_70_L)&BIT0;
    return bRet;
}
MS_BOOL Hal_VIP_GetMCNRBypass(void)
{
    MS_BOOL bRet;
    bRet = R2BYTE(REG_VIP_MCNR_01_L)&(BIT0|BIT1);
    bRet = (((bRet)>>1))? 0: 1;
    return bRet;
}
MS_BOOL Hal_VIP_GetLDCBypass(void)
{
    MS_BOOL bRet;
    bRet = (R2BYTE(REG_SCL_LDC_0F_L)&BIT0)? 1: 0;
    return bRet;
}
MS_BOOL Hal_VIP_GetNLMBypass(void)
{
    MS_BOOL bRet;
    bRet = (R2BYTE(REG_SCL_NLM0_01_L)&BIT0)? 0: 1;
    return bRet;
}
