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
// Copyright (c) 2006-20012 MStar Semiconductor, Inc.
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
// file:   regCMDQ.h
// Description: CMDQ Register Definition
// @author MStar Semiconductor,Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef _CMDQ_REG_MCU_H_
#define _CMDQ_REG_MCU_H_
#include "MsTypes.h"


//#include "MsCommon.h"


//--------------------------------------------------------------------------------------------------
//  Global Definition
//--------------------------------------------------------------------------------------------------



//--------------------------------------------------------------------------------------------------
//  Compliation Option
//--------------------------------------------------------------------------------------------------


//-------------------------------------------------------------------------------------------------
//  Harware Capability
//-------------------------------------------------------------------------------------------------


//-------------------------------------------------------------------------------------------------
//  Type and Structure
//-------------------------------------------------------------------------------------------------
extern MS_U32 _CMDQ_RIU_BASE;
//=============================================================
#define _BITMASK(loc_msb, loc_lsb) ((1U << (loc_msb)) - (1U << (loc_lsb)) + (1U << (loc_msb)))
#define BITMASK(x) _BITMASK(1?x, 0?x)

#define HBMASK    0xFF00
#define LBMASK    0x00FF

#define RIU_MACRO_START     do {
#define RIU_MACRO_END       } while (0)

#define RIU_READ_BYTE(addr)         ( READ_BYTE( _CMDQ_RIU_BASE + (addr) ) )
#define RIU_READ_2BYTE(addr)        ( READ_WORD( _CMDQ_RIU_BASE + (addr) ) )
#define RIU_WRITE_BYTE(addr, val)   WRITE_BYTE( _CMDQ_RIU_BASE + (addr), val)
#define RIU_WRITE_2BYTE(addr, val)  WRITE_WORD( _CMDQ_RIU_BASE + (addr), val)

//=============================================================
// Standard Form
#define R2BYTE( u32Reg ) RIU_READ_2BYTE( (u32Reg) << 1)

#define R2BYTEMSK( u32Reg, u16mask)\
			( ( RIU_READ_2BYTE( (u32Reg)<< 1) & u16mask  ) )
#define R4BYTE( u32Reg )\
            ( { ((RIU_READ_2BYTE( (u32Reg) << 1)) | ((MS_U32)(RIU_READ_2BYTE( ( (u32Reg) + 2 ) << 1) ) << 16)) ; } )

#define W2BYTE( u32Reg, u16Val) RIU_WRITE_2BYTE( (u32Reg) << 1 , u16Val )

#define W2BYTEMSK( u32Reg, u16Val, u16Mask)\
              RIU_WRITE_2BYTE( (u32Reg)<< 1 , (RIU_READ_2BYTE((u32Reg) << 1) & ~(u16Mask)) | ((u16Val) & (u16Mask)) )

#define W4BYTE( u32Reg, u32Val)\
            ( { RIU_WRITE_2BYTE( (u32Reg) << 1, ((u32Val) & 0x0000FFFF) ); \
                RIU_WRITE_2BYTE( ( (u32Reg) + 2) << 1 , (((u32Val) >> 16) & 0x0000FFFF)) ; } )

#define W3BYTE( u32Reg, u32Val)\
            ( { RIU_WRITE_2BYTE( (u32Reg) << 1,  u32Val); \
                RIU_WRITE_BYTE( (u32Reg + 2) << 1 ,  ((u32Val) >> 16)); } )


//=============================================================
//CMDQ
#ifndef REG_TABLE_END
#define REG_TABLE_END            0xFFFF
#endif


#define CMDQ_CRASH_15_0_BIT           0x0000   //0x43 bit:3 command[15:0]
#define CMDQ_CRASH_31_16_BIT          0x0001   //0x43 bit:3 command[31:16]
#define CMDQ_CRASH_55_32_BIT          0x0002   //0x43 bit:3 command[55:32]
#define CMDQ_CRASH_63_56_BIT          0x0003   //0x43 bit:3 command[63:56]



#endif // #ifndef _CMDQ_REG_MCU_H_
