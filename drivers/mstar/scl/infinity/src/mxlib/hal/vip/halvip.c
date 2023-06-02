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

// Internal Definition
#include "hwreg.h"
#include "halvip_utility.h"
#include "halvip.h"
#include "drvPQ_Define.h"

#include "Infinity_Main.h"             // table config parameter
//-------------------------------------------------------------------------------------------------
//  Driver Compiler Options
//-------------------------------------------------------------------------------------------------


//-------------------------------------------------------------------------------------------------
//  Defines & Macro
//-------------------------------------------------------------------------------------------------
#define HAL_VIP_DBG(x)

//-------------------------------------------------------------------------------------------------
//  Variable
//-------------------------------------------------------------------------------------------------
MS_U32 VIP_RIU_BASE = 0;



//-------------------------------------------------------------------------------------------------
//  Functions
//-------------------------------------------------------------------------------------------------
void Hal_VIP_Set_Riu_Base(MS_U32 u32riubase)
{
    VIP_RIU_BASE = u32riubase;
}
void Hal_VIP_SetDNRIPMRead(MS_BOOL bEn)
{
    W2BYTEMSK(REG_SCL_DNR1_01_L, bEn ? BIT0 : 0, BIT0);//IOenable
}
void Hal_VIP_SRAM_Dump(EN_VIP_SRAM_DUMP_TYPE endump,MS_BOOL u8Sram)
{
    MS_U16 u16addr = 0,u16tvalue = 0,u16tcount = 0,u16clkreg,u16readdata;
    MS_U8  u8sec = 0;
    EN_VIP_SRAM_SEC_NUM enSecNum;
    MS_U32 u32reg = endump == EN_VIP_SRAM_DUMP_IHC ? REG_VIP_ACE2_7C_L :
                    endump == EN_VIP_SRAM_DUMP_ICC ?  REG_VIP_ACE2_78_L :
                    endump == EN_VIP_SRAM_DUMP_HVSP ? REG_SCL_HVSP0_41_L:
                    endump == EN_VIP_SRAM_DUMP_HVSP_1 ? REG_SCL_HVSP1_41_L:
                    endump == EN_VIP_SRAM_DUMP_HVSP_2 ? REG_SCL_HVSP2_41_L:
                        0;
    u16clkreg = R2BYTE(REG_SCL_CLK_64_L);
    W2BYTEMSK(REG_SCL_CLK_64_L,0x0,0xFFFF);
    if(endump == EN_VIP_SRAM_DUMP_HVSP_2)
    {
        u16clkreg = R2BYTE(REG_SCL_CLK_65_L);
        W2BYTEMSK(REG_SCL_CLK_65_L,0x0,0xFFFF);
    }
    if(endump == EN_VIP_SRAM_DUMP_IHC || endump == EN_VIP_SRAM_DUMP_ICC)
    {
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
                default:
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

                W2BYTEMSK(u32reg+2, BIT15, BIT15);//ren
                HAL_VIP_DBG(printf("tval:%hx\n",u16tvalue));
                u16readdata=R2BYTE(u32reg+6);
                HAL_VIP_DBG(printf("reg tval:%hx\n",u16readdata));
                u16tcount+=2;
            }
        }

        W2BYTEMSK(u32reg, 0, BIT0);//IOenable
        W2BYTEMSK(REG_SCL_CLK_64_L,u16clkreg,0xFFFF);
    }
    else
    {
        //clear
        for(u16addr = 0;u16addr<256;u16addr++)
        {
            W2BYTEMSK(u32reg, BIT0, BIT0);//Yenable
            W2BYTEMSK(u32reg+2, u16addr, 0xFF);
            W2BYTEMSK(u32reg+4, 0, 0xFFFF);
            W2BYTEMSK(u32reg+6, 0, 0xFFFF);
            W2BYTEMSK(u32reg+8, 0, 0xFF);
            W2BYTEMSK(u32reg, BIT8, BIT8);//W pulse
            W2BYTEMSK(u32reg, 0, BIT0);//Yenable

            W2BYTEMSK(u32reg, BIT1, BIT1);//Cenable
            W2BYTEMSK(u32reg+4, 0, 0xFFFF);
            W2BYTEMSK(u32reg+6, 0, 0xFFFF);
            W2BYTEMSK(u32reg+8, 0, 0xFF);
            W2BYTEMSK(u32reg, BIT8, BIT8);//W pulse
            W2BYTEMSK(u32reg, 0, BIT1);//Cenable
        }
        //SRAM 0 //0~64 entry 65~127 need repeat 0~64

        for(u16addr=0;u16addr<(PQ_IP_SRAM1_SIZE_Main/5);u16addr++)
        {
            W2BYTEMSK(u32reg, BIT0, BIT0);//Yenable
            W2BYTEMSK(u32reg+2, u16addr, 0xFF);
            if(u8Sram)
            {
                u16tvalue = MST_SRAM1_Main[0][u16tcount] | (MST_SRAM1_Main[0][u16tcount+1]<<8);
            }
            else
            {
                u16tvalue = MST_SRAM2_Main[0][u16tcount] | (MST_SRAM2_Main[0][u16tcount+1]<<8);
            }
            W2BYTEMSK(u32reg+4, u16tvalue, 0xFFFF);
            HAL_VIP_DBG(printf("Ytval:%hx\n",u16tvalue));
            if(u8Sram)
            {
                u16tvalue = MST_SRAM1_Main[0][u16tcount+2] | (MST_SRAM1_Main[0][u16tcount+3]<<8);
            }
            else
            {
                u16tvalue = MST_SRAM2_Main[0][u16tcount+2] | (MST_SRAM2_Main[0][u16tcount+3]<<8);
            }
            W2BYTEMSK(u32reg+6, u16tvalue, 0xFFFF);
            HAL_VIP_DBG(printf("Ytval:%hx\n",u16tvalue));
            if(u8Sram)
            {
                u16tvalue = MST_SRAM1_Main[0][u16tcount+4];
            }
            else
            {
                u16tvalue = MST_SRAM2_Main[0][u16tcount+4];
            }
            W2BYTEMSK(u32reg+8, u16tvalue, 0xFF);
            HAL_VIP_DBG(printf("Ytval:%hx\n",u16tvalue));
            W2BYTEMSK(u32reg, BIT8, BIT8);//W pulse
            W2BYTEMSK(u32reg, BIT9, BIT9);//R pulse
            u16readdata = R2BYTE(u32reg+10);
            HAL_VIP_DBG(printf("reg Ytval:%hx\n",u16readdata));
            u16readdata = R2BYTE(u32reg+12);
            HAL_VIP_DBG(printf("reg Ytval:%hx\n",u16readdata));
            u16readdata = R2BYTE(u32reg+14);
            HAL_VIP_DBG(printf("reg Ytval:%hx\n",u16readdata));
            W2BYTEMSK(u32reg, 0, BIT0);//Yenable

            W2BYTEMSK(u32reg, BIT1, BIT1);//Cenable
            u16tvalue = MST_C_SRAM1_Main[0][u16tcount] | (MST_C_SRAM1_Main[0][u16tcount+1]<<8);
            HAL_VIP_DBG(printf("Ctval:%hx\n",u16tvalue));
            W2BYTEMSK(u32reg+4, u16tvalue, 0xFFFF);
            u16tvalue = MST_C_SRAM1_Main[0][u16tcount+2] | (MST_C_SRAM1_Main[0][u16tcount+3]<<8);
            HAL_VIP_DBG(printf("Ctval:%hx\n",u16tvalue));
            W2BYTEMSK(u32reg+6, u16tvalue, 0xFFFF);
            u16tvalue = MST_C_SRAM1_Main[0][u16tcount+4];
            HAL_VIP_DBG(printf("Ctval:%hx\n",u16tvalue));
            W2BYTEMSK(u32reg+8, u16tvalue, 0xFF);
            W2BYTEMSK(u32reg, BIT8, BIT8);//W pulse
            W2BYTEMSK(u32reg, BIT9, BIT9);//R pulse
            u16readdata = R2BYTE(u32reg+10);
            HAL_VIP_DBG(printf("Creg tval:%hx\n",u16readdata));
            u16readdata = R2BYTE(u32reg+12);
            HAL_VIP_DBG(printf("Creg tval:%hx\n",u16readdata));
            u16readdata = R2BYTE(u32reg+14);
            HAL_VIP_DBG(printf("Creg tval:%hx\n",u16readdata));
            W2BYTEMSK(u32reg, 0, BIT1);//Cenable
            // 64~127
            W2BYTEMSK(u32reg, BIT0, BIT0);//Yenable
            W2BYTEMSK(u32reg+2, u16addr+64, 0xFF);
            if(u8Sram)
            {
                u16tvalue = MST_SRAM1_Main[0][u16tcount] | (MST_SRAM1_Main[0][u16tcount+1]<<8);
            }
            else
            {
                u16tvalue = MST_SRAM2_Main[0][u16tcount] | (MST_SRAM2_Main[0][u16tcount+1]<<8);
            }
            W2BYTEMSK(u32reg+4, u16tvalue, 0xFFFF);
            if(u8Sram)
            {
                u16tvalue = MST_SRAM1_Main[0][u16tcount+2] | (MST_SRAM1_Main[0][u16tcount+3]<<8);
            }
            else
            {
                u16tvalue = MST_SRAM2_Main[0][u16tcount+2] | (MST_SRAM2_Main[0][u16tcount+3]<<8);
            }
            W2BYTEMSK(u32reg+6, u16tvalue, 0xFFFF);
            if(u8Sram)
            {
                u16tvalue = MST_SRAM1_Main[0][u16tcount+4];
            }
            else
            {
                u16tvalue = MST_SRAM2_Main[0][u16tcount+4];
            }
            W2BYTEMSK(u32reg+8, u16tvalue, 0xFF);
            W2BYTEMSK(u32reg, BIT8, BIT8);//W pulse
            W2BYTEMSK(u32reg, 0, BIT0);//Yenable

            W2BYTEMSK(u32reg, BIT1, BIT1);//Cenable
            u16tvalue = MST_C_SRAM1_Main[0][u16tcount] | (MST_C_SRAM1_Main[0][u16tcount+1]<<8);
            W2BYTEMSK(u32reg+4, u16tvalue, 0xFFFF);
            u16tvalue = MST_C_SRAM1_Main[0][u16tcount+2] | (MST_C_SRAM1_Main[0][u16tcount+3]<<8);
            W2BYTEMSK(u32reg+6, u16tvalue, 0xFFFF);
            u16tvalue = MST_C_SRAM1_Main[0][u16tcount+4];
            W2BYTEMSK(u32reg+8, u16tvalue, 0xFF);
            W2BYTEMSK(u32reg, BIT8, BIT8);//W pulse
            W2BYTEMSK(u32reg, 0, BIT1);//Cenable
            u16tcount += 5;
        }
        //SRAM 1 //128~256 entry  link to 0~127 , so do not use

        if(endump == EN_VIP_SRAM_DUMP_HVSP_2)
        {
            W2BYTEMSK(REG_SCL_CLK_65_L,u16clkreg,0xFFFF);
        }
        else
        {
            W2BYTEMSK(REG_SCL_CLK_64_L,u16clkreg,0xFFFF);
        }
        u16tcount = 0;
    }
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


void Hal_VIP_SetLDCFrameBufferSetting(MS_U8 u8Val)
{
    W2BYTEMSK(REG_SCL_LDC_09_L, (u8Val), BIT8|BIT5|BIT4|BIT3|BIT2);
}


void Hal_VIP_SetLDCDmapBase(MS_U32 u32Base)
{
    W4BYTE(REG_SCL_LDC_0A_L, u32Base);
}

void Hal_VIP_SetLDCDmapPitch(MS_U32 u32Pitch)
{
    W4BYTE(REG_SCL_LDC_0C_L, u32Pitch);
}
void Hal_VIP_SetLDC422_444_allMd(MS_U8 u8md)
{
    W2BYTEMSK(REG_SCL_LDC_1F_L, (MS_U16)u8md, BIT4|BIT3|BIT2|BIT1|BIT0);
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
    W2BYTEMSK(REG_SCL_LDC_22_L, ((MS_U16)u16hor), 0xFFFF);
    W2BYTEMSK(REG_SCL_LDC_2A_L, ((MS_U16)u16ver), 0xFFFF);
}
void Hal_VIP_SetLDCSramBase(MS_U32 u32hor,MS_U32 u32ver)
{
    W4BYTE(REG_SCL_LDC_24_L, (u32hor>>4));
    W4BYTE(REG_SCL_LDC_2C_L, (u32ver>>4));
}
void Hal_VIP_SetLDCSramStr(MS_U16 u16hor,MS_U16 u16ver)
{

    W2BYTEMSK(REG_SCL_LDC_23_L, ((MS_U16)u16hor), 0xFFFF);
    W2BYTEMSK(REG_SCL_LDC_2B_L, ((MS_U16)u16ver), 0xFFFF);
}

void Hal_VIP_SetAutodownloadAddr(MS_U32 u32baseadr,MS_U16 u16iniaddr,MS_U8 u8cli)
{
    switch(u8cli)
    {
        case 9:
            W2BYTEMSK(REG_SCL1_73_L, (MS_U16)(u32baseadr>>4), 0xFFFF);
            W2BYTEMSK(REG_SCL1_74_L, (MS_U16)(u32baseadr>>20), 0x01FF);
            W2BYTEMSK(REG_SCL1_77_L, ((MS_U16)u16iniaddr), 0xFFFF);
            break;
        default:
            break;
    }
}

void Hal_VIP_SetAutodownloadReq(MS_U16 u16depth,MS_U16 u16reqlen,MS_U8 u8cli)
{
    switch(u8cli)
    {
        case 9:
            W2BYTEMSK(REG_SCL1_76_L, ((MS_U16)u16reqlen), 0xFFFF);
            W2BYTEMSK(REG_SCL1_75_L, ((MS_U16)u16depth), 0xFFFF);
            break;
        default:
            break;
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
            break;
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
    HAL_VIP_DBG(printf("[Get_SRAM]entry%hx :%lx\n",u16entry,u32tvalue1));
}

void Hal_VIP_SetNLMSRAMbyCPU(MS_U16 u16entry,MS_U32 u32tvalue)
{
    W2BYTEMSK(REG_SCL_NLM0_62_L, u16entry, 0x07FF);
    W2BYTEMSK(REG_SCL_NLM0_62_L, 0x8000, 0x8000);
    W2BYTEMSK(REG_SCL_NLM0_63_L, (MS_U16)u32tvalue, 0xFFFF);
    W2BYTEMSK(REG_SCL_NLM0_65_L, (MS_U16)(u32tvalue>>16), 0x000F);
    W2BYTEMSK(REG_SCL_NLM0_62_L, 0x4000, 0x4000);
    W2BYTEMSK(REG_SCL_NLM0_62_L, 0x0000, 0x8000);
    HAL_VIP_DBG(printf("[Set_SRAM]entry%hx :%lx\n",u16entry,u32tvalue));
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
            W2BYTEMSK(u32Addr, u16Data, 0xffff);
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
            W2BYTEMSK(u32Addr, u16Data, 0xffff);
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
MS_BOOL Hal_VIP_GetVIPBypass(void)
{
    MS_BOOL bRet;
    bRet = R2BYTE(REG_VIP_LCE_70_L)&BIT0;
    return bRet;
}
MS_BOOL Hal_VIP_GetDNRBypass(void)
{
    MS_BOOL bRet;
    bRet = R2BYTE(REG_SCL_DNR0_21_L)&(BIT0|BIT1);
    bRet = (((bRet&BIT1)>>1)|(bRet&BIT0))? 0: 1;
    return bRet;
}
MS_BOOL Hal_VIP_GetSNRBypass(void)
{
    MS_BOOL bRet;
    bRet = R2BYTE(REG_SCL_DNR0_22_L)&(BIT0|BIT1);
    bRet = (((bRet&BIT1)>>1)|(bRet&BIT0))? 0: 1;
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
