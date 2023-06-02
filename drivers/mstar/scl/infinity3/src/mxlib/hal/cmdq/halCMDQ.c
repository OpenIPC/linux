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
// Copyright (c) 2006-2009 MStar Semiconductor, Inc.
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

////////////////////////////////////////////////////////////////////////////////////////////////////
// file   halCMDQ.c
// @brief  CMDQ HAL
// @author MStar Semiconductor,Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////
#ifdef MSOS_TYPE_LINUX_KERNEL
//#include <linux/wait.h>
//#include <linux/irqreturn.h>
//#include <asm/div64.h>
#endif

#include "MsCommon.h"
#include "MsTypes.h"

#include "hwreg.h"
#include "regCMDQ.h"
#include "drvCMDQ.h"
#include "halCMDQ.h"

#define  CMDQDBG(x)



//--------------------------------------------------------------------------------------------------
//  Driver Compiler Option
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
//  Macro of bit operations
//--------------------------------------------------------------------------------------------------
MS_U32 _CMDQ_RIU_BASE;



//--------------------------------------------------------------------------------------------------
//  Inline Function
//--------------------------------------------------------------------------------------------------

void Hal_CMDQ_InitRIUBase(MS_U32 u32PM_riu_base)
{
    _CMDQ_RIU_BASE = u32PM_riu_base ;
}

//---------------------------------------------------------------------------
///set the element of _CMDQCtrl
///  .CMDQ_Enable
///  .CMDQ_Length_ReadMode
///  .CMDQ_Mask_Setting
//---------------------------------------------------------------------------
MS_U32 Hal_CMDQ_GetRegFromIPID(EN_CMDQ_IP_TYPE enIPType, MS_U8 u8addr)
{
    MS_U32 Reg = 0;
    switch(enIPType)
    {
        case EN_CMDQ_TYPE_IP0:
            Reg = REG_CMDQ_BASE;
            break;

        case EN_CMDQ_TYPE_IP1:
            Reg = REG_CMDQ1_BASE;
            break;
        case EN_CMDQ_TYPE_IP2:
            Reg = REG_CMDQ2_BASE;
            break;
        default:
            Reg = REG_CMDQ_BASE;
            break;
    }
    Reg += (u8addr*2);
    return Reg;

}
void Hal_CMDQ_SetRegPassWaitPoll(EN_CMDQ_IP_TYPE enIPType,MS_BOOL bEn)
{
    MS_U32 Reg = 0;
    Reg = Hal_CMDQ_GetRegFromIPID(enIPType ,0x24);
    W2BYTEMSK(Reg,bEn ? 0xE : 0,0xE);
}
MS_U16 Hal_CMDQ_GetFinalIrq(EN_CMDQ_IP_TYPE enIPType,MS_U16 u16Mask)
{
    MS_U16 u16val;
    MS_U32 Reg = 0;
    Reg = Hal_CMDQ_GetRegFromIPID(enIPType ,0x44);
    u16val = R2BYTEMSK(Reg, u16Mask);;
    return u16val;
}
MS_U16 Hal_CMDQ_GetRawIrq(EN_CMDQ_IP_TYPE enIPType,MS_U16 u16Mask)
{
    MS_U16 u16val;
    MS_U32 Reg = 0;
    Reg = Hal_CMDQ_GetRegFromIPID(enIPType ,0x45);
    u16val = R2BYTEMSK(Reg, u16Mask);;
    return u16val;
}

void Hal_CMDQ_Enable(EN_CMDQ_IP_TYPE enIPType,MS_BOOL bEn)
{
    MS_U32 Reg = 0;
    Reg = Hal_CMDQ_GetRegFromIPID(enIPType ,0x1);
    if(bEn)
    {
      W2BYTEMSK(Reg, 0x01, 0x01);

    }
    else
    {
      W2BYTEMSK(Reg, 0x00, 0x01);

    }

}
void Hal_CMDQ_SetBaseEn(EN_CMDQ_IP_TYPE enIPType, MS_BOOL bEn)
{
    MS_U32 Reg = 0;
    MS_U32 Reg1 = 0;
    MS_U32 Reg2 = 0;
    Reg = Hal_CMDQ_GetRegFromIPID(enIPType ,0x1);
    Reg1 = Hal_CMDQ_GetRegFromIPID(enIPType ,0x11);
    Reg2 = Hal_CMDQ_GetRegFromIPID(enIPType ,0x20);
    if(bEn)
    {
      W2BYTEMSK(Reg, 0x01, 0x01);
      W2BYTEMSK(Reg1, 0x190d, 0x1FFF);
      W2BYTEMSK(Reg2, 0x0080, 0x0080);
    }
    else
    {

      W2BYTEMSK(Reg1, 0x00, 0x10FF);
      W2BYTEMSK(Reg2, 0x00, 0x0080);
    }

}


//---------------------------------------------------------------------------
///set the element of _CMDQCtrl
///  .CMDQ_En_Clk_Miu
//---------------------------------------------------------------------------
void Hal_CMDQ_Reset(EN_CMDQ_IP_TYPE enIPType, MS_BOOL bEn)
{
    MS_U32 Reg = 0;
    Reg = Hal_CMDQ_GetRegFromIPID(enIPType ,0x31);
    if(bEn)
    {
      W2BYTEMSK(Reg, 0x00, 0x01);
    }
    else
    {
      W2BYTEMSK(Reg, 0x01, 0x01);
    }
}
void Hal_CMDQ_SetTrigAlways(EN_CMDQ_IP_TYPE enIPType, MS_BOOL bEn)
{
    MS_U32 Reg = 0;
    Reg = Hal_CMDQ_GetRegFromIPID(enIPType ,0x2);
    if(bEn)
        W2BYTEMSK(Reg, 0x00, 0x01);
    else
        W2BYTEMSK(Reg, 0x01, 0x01);
}
MS_BOOL Hal_CMDQ_SetDMAMode(EN_CMDQ_IP_TYPE enIPType,int ModeSel)
{
    MS_U32 Reg = 0;
    Reg = Hal_CMDQ_GetRegFromIPID(enIPType ,0x2);
    if (ModeSel==1)
    {
        W2BYTEMSK(Reg, 0x00, 0x02);
        return TRUE;
    }
    else if(ModeSel==0)
    {
        W2BYTEMSK(Reg, 0x01, 0x02);
        return TRUE;
    }
    else
    {
        W2BYTEMSK(Reg, 0x04, 0x04);
        return TRUE;
    }
}

void Hal_CMDQ_SetStartPointer(EN_CMDQ_IP_TYPE enIPType,MS_U32 StartAddr)
{
    MS_U32 temp;
    MS_U32 Reg = 0;
    Reg = Hal_CMDQ_GetRegFromIPID(enIPType ,0x4);
    temp=(MS_U32)(StartAddr);
    temp=temp/16;
    W4BYTE(Reg,temp);
}

void Hal_CMDQ_SetEndPointer(EN_CMDQ_IP_TYPE enIPType,MS_U32 EndAddr)
{
    MS_U32 temp;
    MS_U32 Reg = 0;
    Reg = Hal_CMDQ_GetRegFromIPID(enIPType ,0x6);
    temp=(MS_U32)(EndAddr);
    temp=temp/16;
    W4BYTE(Reg,temp);
}

void Hal_CMDQ_SetOffsetPointer(EN_CMDQ_IP_TYPE enIPType,MS_U32 OffsetAddr)
{
    MS_U32 temp;
    MS_U32 Reg = 0;
    Reg = Hal_CMDQ_GetRegFromIPID(enIPType ,0x8);
    temp=(MS_U32)(OffsetAddr);
    temp=temp>>4;
    CMDQDBG(printf("End_Pointer _bits:%lx\n", temp));
    W4BYTE(Reg,temp);
}

void Hal_CMDQ_SetTimer(EN_CMDQ_IP_TYPE enIPType,MS_U32 time)
{
    MS_U32 Reg = 0;
    Reg = Hal_CMDQ_GetRegFromIPID(enIPType ,0x4A);
    W2BYTE(Reg , time);
}

void Hal_CMDQ_SetRatio(EN_CMDQ_IP_TYPE enIPType,MS_U32 Ratio)
{
    MS_U32 Reg = 0;
    Reg = Hal_CMDQ_GetRegFromIPID(enIPType ,0x4B);
    W2BYTEMSK(Reg, Ratio, 0x00FF);
}
void Hal_CMDQ_Set_Waittrig(EN_CMDQ_IP_TYPE enIPType,MS_U16 trig)
{
    MS_U32 Reg = 0;
    Reg = Hal_CMDQ_GetRegFromIPID(enIPType ,0x22);
    W2BYTEMSK(Reg, trig, 0x00FF);
}
void Hal_CMDQ_SetSkipPollWhenWaitTimeout(EN_CMDQ_IP_TYPE enIPType,MS_BOOL ben)
{
    MS_U32 Reg = 0;
    Reg = Hal_CMDQ_GetRegFromIPID(enIPType ,0x29);
    if(ben)
        W2BYTEMSK(Reg, 0x0080, 0x0080);
    else
        W2BYTEMSK(Reg, 0x0000, 0x0080);
}
void Hal_CMDQ_SetTimeoutAmount(EN_CMDQ_IP_TYPE enIPType)
{
    MS_U32 Reg = 0;
    MS_U32 Reg1 = 0;
    Reg = Hal_CMDQ_GetRegFromIPID(enIPType ,0x28);
    Reg1 = Hal_CMDQ_GetRegFromIPID(enIPType ,0x29);
    W2BYTE(Reg, (MS_U16)(CMDQ_base_amount&0xFFFF));
    W2BYTEMSK(Reg1, (MS_U16)((CMDQ_base_amount>>16) &0xF), 0x000F);
    W2BYTEMSK(Reg1, (MS_U16)(CMDQ_timeout_amount <<8), 0xFF00);

}
void Hal_CMDQ_ClearIRQByFlag(EN_CMDQ_IP_TYPE enIPType,MS_U16 u16IRQ)
{
    MS_U32 Reg = 0;
    Reg = Hal_CMDQ_GetRegFromIPID(enIPType ,0x48);
    W2BYTE(Reg, u16IRQ);
    W2BYTE(Reg, 0x0);
}
void Hal_CMDQ_ResetSoftInterrupt(EN_CMDQ_IP_TYPE enIPType)
{
    MS_U32 Reg = 0;
    Reg = Hal_CMDQ_GetRegFromIPID(enIPType ,0x48);
    W2BYTE(Reg, 0xFFFF);
    W2BYTE(Reg, 0x0);
}

void Hal_CMDQ_SetISRMSK(EN_CMDQ_IP_TYPE enIPType,MS_U16 u16Msk)
{
    MS_U32 Reg = 0;
    Reg = Hal_CMDQ_GetRegFromIPID(enIPType ,0x47);
    W2BYTE(Reg, u16Msk);
}

void Hal_CMDQ_WriteRegDirect(MS_U32 u32Addr,MS_U16 u16Data)
{
    W2BYTE(u32Addr, u16Data);
}
void Hal_CMDQ_WriteRegMaskDirect(MS_U32 u32Addr,MS_U16 u16Data,MS_U16 u16Mask)
{
    W2BYTEMSK(u32Addr,u16Data,u16Mask);

}
MS_U16 Hal_CMDQ_Read2ByteReg(MS_U32 u32Addr)
{
    MS_U16 u16val;
    u16val = R2BYTE(u32Addr);
    return u16val;
}
MS_U16 Hal_CMDQ_Read2ByteMaskDirect(MS_U32 u32Addr,MS_U16 u16Mask)
{
    MS_U16 u16val;
    u16val = R2BYTEMSK(u32Addr, u16Mask);
    return u16val;
}
MS_U16 Hal_CMDQ_Get_ISP_Cnt(void)
{
    MS_U16 isp_cnt;
    MS_U32 reg;
    reg=0x1302f4;
    isp_cnt=R2BYTE(reg);
    return isp_cnt;
}
//---------------------------------------------------------------------------
///Trigger for update start pointer and end pointer
//---------------------------------------------------------------------------
void Hal_CMDQ_Start(EN_CMDQ_IP_TYPE enIPType,MS_BOOL bEn)
{
    MS_U32 Reg = 0;
    Reg = Hal_CMDQ_GetRegFromIPID(enIPType ,0x3);
    if (bEn)
    {
        W2BYTEMSK(Reg, 0x0002, 0x0002);
    }

}
MS_U32 Hal_CMDQ_GetMIUReadAddr(EN_CMDQ_IP_TYPE enIPType)
{
    MS_U32 reg_value=0;
    MS_U32 Reg = 0;
    MS_U32 Reg1 = 0;
    Reg = Hal_CMDQ_GetRegFromIPID(enIPType ,0x3);
    Reg1 = Hal_CMDQ_GetRegFromIPID(enIPType ,0xA);
    W2BYTEMSK(Reg,0x0100 , 0x0100);
    reg_value=R4BYTE(Reg1);
    return reg_value;
}

//---------------------------------------------------------------------------
///Set Previous Dummy Register bit to be 1(which means this CAF is already write to DRAM)
//---------------------------------------------------------------------------
MS_U32 Hal_CMDQ_ErrorCommand(EN_CMDQ_IP_TYPE enIPType,MS_U32 select_bit)
{
    MS_U32 reg_value=0;
    MS_U32 Reg = 0;
    MS_U32 Reg1 = 0;
    Reg = Hal_CMDQ_GetRegFromIPID(enIPType ,0x43);
    Reg1 = Hal_CMDQ_GetRegFromIPID(enIPType ,0x40);
    W2BYTEMSK(Reg,select_bit , 0x0007);
    reg_value= R2BYTE(Reg1);
    return reg_value;

}

void Hal_CMDQ_Setmiusel(EN_CMDQ_IP_TYPE enIPType, MS_U8 u8MIUSel)
{
    MS_U32 Reg = 0;
    Reg = Hal_CMDQ_GetRegFromIPID(enIPType ,0x10);
    if (u8MIUSel == 0)
    {
        W2BYTEMSK(Reg, 0x0000, 0x0002);
    }
    else
    {
        W2BYTEMSK(Reg, 0x0002, 0x0002);
    }
}
#if CMDQ_nonuse_Function
/*
MS_U32 Hal_CMDQ_Read_Start_Pointer(void)
{
    MS_U32 reg_value=0;
    reg_value=R4BYTE(REG_CMDQ_04_L);
    return reg_value;
}

MS_U32 Hal_CMDQ_Read_End_Pointer(void)
{
    MS_U32 reg_value=0;
    reg_value=R4BYTE(REG_CMDQ_06_L);
    return reg_value;
}
*/
void Hal_CMDQ_Reset_Start_Pointer_bit(EN_CMDQ_IP_TYPE enIPType)
{
    MS_U32 Reg = 0;
    Reg = Hal_CMDQ_GetRegFromIPID(enIPType ,0x3);
    W2BYTEMSK(Reg, 0x0004, 0x0004);
}
MS_U16 Hal_CMDQ_get_status(EN_CMDQ_IP_TYPE enIPType)
{
    MS_U32 Reg = 0;
    Reg = Hal_CMDQ_GetRegFromIPID(enIPType ,0x1);
    return ((R2BYTE(Reg) & 0x0001));
}
MS_BOOL Hal_CMDQ_Get_LDCtirg(void)
{
    MS_U16 trig;
    MS_BOOL ret;
    trig=(R2BYTE(0x12183E)&0x0800)>>11;
    ret= trig == 0x1 ? 0 :1;
    return ret;
}

#endif
