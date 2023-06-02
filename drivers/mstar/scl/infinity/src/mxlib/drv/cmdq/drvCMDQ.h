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
///
/// @file   drvCMDQ.h
/// @brief  CMDQ Driver Interface
/// @author MStar Semiconductor,Inc.
///
////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef _DRVCMDQ_H_
#define _DRVCMDQ_H_
#ifdef MSOS_TYPE_LINUX_KERNEL
#include <linux/wait.h>
#include <linux/irqreturn.h>
#include <asm/div64.h>
#endif

//#include "MsTypes.h"
#include "MsCommon.h"
#include "MsTypes.h"
//#include <linux/irq.h>





//--------------------------------------------------------------------------------------------------
//  Define
//--------------------------------------------------------------------------------------------------
#define DIRECT_MODE                 0x01    // direct mode :nonuse
#define INCREMENT_MODE              0x00    // increment mode :nonuse
#define RING_BUFFER_MODE            0x04    // ring mode
#define CMDQ_DBUF                   0       // whether double buffer
#define CMDQ_irq                    0       // isr open
#define CMDQ_nonuse_Function        0       // mark no use function
#define CMDQ_poll_timer             0x4D    // poll timer
#define CMDQ_timer_ratio            0x0     // time ratio
#define CMDQ_base_amount            0x1FFFF // wait count amount
#define CMDQ_timeout_amount         0x40    // wait timeout amount
#define CMDQ_NUMBER                 1       // number of CMDQ ip
#define CMDQ_ADD_DELAYFRAME_SCLSELF 0       // when=1 is delay 2 frame ,when =0 is delay 2frame but input isp count add 1 itself.
#define CMDQ_ALLOW_ERROR_COUNT      1       // allow overpass count
#define CMDQ_IRQ_STATE_TRIG         (1 << 0)
#define CMDQ_IRQ_STATE_DONE         (1 << 2)
#define CMDQ_IRQ_WAIT_TIMEOUT       (1 << 10)
#define CMDQ_CMDTYPE_WRITE 0x10
#define CMDQ_CMDTYPE_WAIT 0x20
#define CMDQ_CMDTYPE_POLLEQ 0x30
#define CMDQ_CMDTYPE_POLLNEQ 0xb0
//--------------------------------------------------------------------------------------------------
//  Driver Capability
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
//  Local variable
//-----------------------------------------------------------------------------------------------
extern MS_U32 _CMDQ_RIU_BASE;

//--------------------------------------------------------------------------------------------------
//  Type and Structure
//--------------------------------------------------------------------------------------------------
typedef enum
{
    EN_CMDQ_TYPE_IP0 ,
    EN_CMDQ_TYPE_IP1 ,
    EN_CMDQ_TYPE_IP2 ,
    EN_CMDQ_TYPE_MAX,
}EN_CMDQ_IP_TYPE;

typedef enum
{
    EN_CMDQ_MIU_0 ,
    EN_CMDQ_MIU_1 ,
}EN_CMDQ_MIU_TYPE;

typedef enum
{
    CMDQ_FLAG_FIRE              = 0x1,  // whether already fire or need to fire
    CMDQ_FLAG_FIRST_NONFRAMECNT = 0x2,  // first non-frame cnt cmd need wait
    CMDQ_FLAG_FRAMECNT_CMD      = 0x4,  // cmd whether framecnt
    CMDQ_FLAG_ROTATION          = 0x8,  // using in nonframecnt , if full
    CMDQ_FLAG_SAMEFRAME         = 0x10, // using in framecnt , if trig but next cmd is same frame ,don't add poll
    CMDQ_FLAG_NONFRAMEBUFFER    = 0x20,
}CMDQ_FLAG_TYPE;
typedef struct
{
    MS_U16 u16WPoint;           // current cmd end
    MS_U16 u16RPoint;           // last trig
    MS_U16 u16FPoint;           // fire point (128 bit/unit)
    MS_U16 u16assFrame_Point;   // assign frame count
    MS_PHYADDR PhyAddr;         // by Chip_Phys_to_MIU
    MS_U16 u16MaxCmdCnt;        // assframe buf count
    MS_U16 u16BufCmdCnt;        // nonframecnt buf count
    MS_BOOL bEnable;            // CMDQ enable ((nonuse
    MS_U32 u32VirAddr;          // kernel virtul after allocate
    MS_PHYADDR PhyAddrEnd;      // by Chip_Phys_to_MIU+cnt
    MS_U16 u16LPoint;           // To get trig Count
}MS_CMDQ_Info;

typedef struct
{
    MS_U8 u8MIUSel_IP;
}CMDQ_Buffer_MIU;
typedef struct
{
    MS_U16 u16StartPoint;
    MS_U16 u16EndPoint;
    MS_U32 u32CmdDiffCnt;
    MS_U32 u32ActualCmdDiffCnt;
    MS_BOOL bframecount;
    MS_BOOL Ret;
    MS_U32 u32addr;
    MS_U16 u16mask;
    MS_U16 u16data;
    MS_U64 *pu32Addr;
}CMDQ_CheckCmdinfo;

typedef struct
{
   MS_U8 u8type;    // CMD type(write,wait,poll)
   MS_U16 u16Data;  // 16bit data
   MS_U32 u32Addr;  // 16bit Bank addr + 8bit 16bit-regaddr
   MS_U16 u16Mask;  // inverse normal case
   MS_U64 u64Cmd;   // 64bit to consist CMDQ CMD
   MS_U8 u8AssignFramecnt;
   MS_U8 bCntWarn;
   MS_U8 bAddPollFunc;
}MS_CMDQ_CMD;


typedef enum
{
    E_CMDQ_EVENT_RUN = 0x00000001,
    E_CMDQ_EVENT_IRQ = 0x00000002,
} MDrvHDMITXEvent;

#define DRVCMDQ_OK                   0x00000000
#define DRVCMDQ_FAIL                 0x00000001


//--------------------------------------------------------------------------------------------------
//  Function Prototype
//--------------------------------------------------------------------------------------------------
//==============================Enable===============================================
 void Drv_CMDQ_Enable(MS_BOOL bEnable,EN_CMDQ_IP_TYPE enIPType);
void Drv_CMDQ_InitRIUBase(MS_U32 u32RIUBase);

//==============================Delete===============================================
void Drv_CMDQ_Delete(void);
void Drv_CMDQ_SetRPoint(EN_CMDQ_IP_TYPE enIPType);
MS_U64 Drv_CMDQ_GetCMDFromPoint(MS_U16 u16Point);
MS_U32 Drv_CMDQ_GetCMDBankFromCMD(MS_U64 u64Cmd);
//===============================init==============================================
//-------------------------------------------------------------------------------------------------
/// Drv_CMDQ_Init
/// is CMDQ IP init function, to set timer,address and size
/// @param  PhyAddr1        \b IN: from msys_request_dmem,and transform to MIU address
/// @param  u32VirAddr1  \b IN:like phyaddr,map to kernel virtual
/// @param  u32BufByteLen \b IN:already allocate memory size
/// @param  u32RIUBase     \b IN:RIU's base shift
//-------------------------------------------------------------------------------------------------
 void Drv_CMDQ_Init(MS_PHYADDR PhyAddr1, MS_U32 u32VirAddr1, MS_U32 u32BufByteLen,MS_U32 u32RIUBase);

//-------------------------------------------------------------------------------------------------
/// Drv_CMDQ_Set_timer_ratio
/// set poll times in 1sec
/// set time / ratio, total wait time is (wait_time * (ratio + 1)) =>ex. ( FF * (F + 1) / 216  MHz ) = sec
///                                                                                      4D*1=4D~=77 *(216/M) 1 ~=60(frame/s)
/// the polling_timer is for re-checking polling value, if the time_interval is too small, cmdq will polling RIU frequently, so that RIU will very busy
/// @param  time  \b IN: poll wait time :#CMDQ_poll_timer
/// @param  ratio  \b IN: time ratio       :# CMDQ_timer_ratio
/// retval : OK
//-------------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------
/// Drv_CMDQ_SetSkipPollWhenWaitTimeOut
/// set wait timeout count and whether jump timeout
/// @param   u16bjump \b IN:if true, timeout will jump wait, and  carry out  command
//-------------------------------------------------------------------------------------------------
void Drv_CMDQ_SetSkipPollWhenWaitTimeOut(EN_CMDQ_IP_TYPE enIPType,MS_U16 bEn);
//--------------------------------------------------------------------------------------------------
// Drv_CMDQ_Set_Buffer
// set addr st and end pointer ,and set move range
// In ring-buffer mode, this function will trig for update reg_sw_wr_mi_wadr
// The CMDQ will keep on executing cmd until reg_rd_mi_radr reach reg_sw_wr_mi_wadr
/// @param   StartAddr \b IN:MIU addr(byte base) --> IP need 16byte addr,so function inside will /16
/// @param   EndAddr \b IN:MIU addr
//--------------------------------------------------------------------------------------------------
MS_U16 Drv_CMDQ_GetFinalIrq(EN_CMDQ_IP_TYPE enIPType);
void Drv_CMDQ_ClearIrqByFlag(EN_CMDQ_IP_TYPE enIPType,MS_U16 u16Irq);
void Drv_CMDQ_SetISRStatus(MS_BOOL bEn);
void Drv_CMDQ_GetModuleMutex(EN_CMDQ_IP_TYPE enIPType,MS_BOOL bEn);
MS_CMDQ_Info Drv_CMDQ_GetCMDQInformation(EN_CMDQ_IP_TYPE enIPType);

//===============================write==============================================
//-------------------------------------------------------------------------------------------------
/// Drv_CMDQ_WriteCmd
/// write cmd API for any case ,to add cmd to MIU
/// @param  u32Addr   \b IN: 8bit-addr
/// @param  u16Data  \b IN:  16bit data
/// @param  u16Mask \b IN:  16bit ~mask,
/// @param  bSkipCheckSameAddr     \b IN:if true,don't need to check .if false don't use mask(for RIU 32bit)
//-------------------------------------------------------------------------------------------------
MS_BOOL Drv_CMDQ_WriteCmd(MS_U32 u32Addr, MS_U16 u16Data, MS_U16 u16Mask,MS_BOOL bSkipCheckSameAddr);

//-------------------------------------------------------------------------------------------------
/// Drv_CMDQ_AssignFrameWriteCmd
/// write cmd API for frame count case ,to handle frame count case and add to MIU(use Drv_CMDQ_WriteCmd)
/// @param  u32Addr         \b IN: 8bit-addr
/// @param  u16Data         \b IN:  16bit data
/// @param  u16Mask         \b IN:  16bit ~mask,
/// @param  u8framecnt     \b IN: want to set cmd in this count
//-------------------------------------------------------------------------------------------------
MS_BOOL Drv_CMDQ_AssignFrameWriteCmd(MS_U32 u32Addr, MS_U16 u16Data, MS_U16 u16Mask,MS_U8 u8framecnt);
void Drv_CMDQ_release(void);

//-------------------------------------------------------------------------------------------------
/// Drv_CMDQ_AddCmd
/// To add CMD to MIU, _Buf is use to nonframe count
/// @param  u64Cmd         \b IN: the CMD want to write to MIU
/// @param  bSkipCheckSameAddr         \b IN:  if true,don't need to check .if false don't use mask(for RIU 32bit)
//-------------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------
/// Drv_CMDQ_MoveBufCmd
/// To move nonframe count CMD form nonframe count region to assframe count region
/// @param  enIPType         \b IN: Buf number,nonuse
//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
/// Drv_CMDQ_AddCmd_Check
/// To check the CMD whether already exist. if yes ,to handle it.
/// @param  u64Cmd  \b IN: the CMD want to write to MIU
/// @param  bframecount      \b IN: if true, is frame count case.
/// @param  bSkipCheckSameAddr     \b IN:if true,don't need to check .if false don't use mask(for RIU 32bit)
//-------------------------------------------------------------------------------------------------

 //===============================wait==============================================
 //-------------------------------------------------------------------------------------------------
 /// Drv_CMDQ_WaitCmd
 /// To add wait CMD
 /// @param  u16bus         \b IN: wait trigger bus
 //-------------------------------------------------------------------------------------------------
 void Drv_CMDQ_WaitCmd(MS_U16 u16bus);

 //===============================poll==============================================
 //-------------------------------------------------------------------------------------------------
 /// Drv_CMDQ_PollingEqCmd
 /// add Polling CMD ,or neq CMD
 /// @param  u32Addr   \b IN: 8bit-addr
 /// @param  u16Data  \b IN:  16bit data
 /// @param  u16Mask \b IN:  16bit ~mask,
 //-------------------------------------------------------------------------------------------------
 void Drv_CMDQ_PollingEqCmd(MS_U32 u32Addr, MS_U16 u16Data, MS_U16 u16Mask);
 void Drv_CMDQ_PollingNeqCmd(MS_U32 u32Addr, MS_U16 u16Data, MS_U16 u16Mask);

 //===============================fire==============================================
 //-------------------------------------------------------------------------------------------------
 /// Drv_CMDQ_Fire
 /// To trig CMDQ
 /// @param  bStart   \b IN: trig
 //-------------------------------------------------------------------------------------------------
MS_BOOL Drv_CMDQ_Fire(EN_CMDQ_IP_TYPE enIPType,MS_BOOL bStart);

//===============================check==============================================
//-------------------------------------------------------------------------------------------------
/// Drv_CMDQ_CheckIPAlreadyDone
/// To check CMDQ status
//-------------------------------------------------------------------------------------------------
MS_BOOL Drv_CMDQ_CheckIPAlreadyDone(EN_CMDQ_IP_TYPE enIPType);

//===============================write Register==============================================
#endif // _DRVCMDQ_H_
