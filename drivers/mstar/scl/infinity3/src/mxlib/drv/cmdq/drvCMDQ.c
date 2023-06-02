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
////////////////////////////////////////////////////////////////////////////////////////////////////
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
////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// file    drvCMDQ.c
// @brief  CMDQ Driver
// @author MStar Semiconductor,Inc.
//
////////////////////////////////////////////////////////////////////////////////////////////////////
#define DRV_CMDQ_C

#ifdef MSOS_TYPE_LINUX_KERNEL
//#include <linux/wait.h>
//#include <linux/irqreturn.h>
//#include <asm/div64.h>
#endif

#include "MsCommon.h"
#include "MsTypes.h"
#include "MsIRQ.h"
#include "MsOS.h"
#include "MsVersion.h"
#include "MsDevice.h"

//#include <linux/kthread.h>
//#include <linux/interrupt.h>
#include "drvvip.h"
#include "drvCMDQ.h"
#include "halCMDQ.h"
#include "drvsclirq_st.h"
#include "drvsclirq.h"
#include "halCHIP.h"
#include "irqs.h"
//#include <ms_platform.h>
#include "mdrv_scl_dbg.h"

static MS_CMDQ_Info gstCMDQInfo[EN_CMDQ_TYPE_MAX] = {{0,0,0,0,0,FALSE,0,0,NULL}};
static MS_U8 gCMDQStatusFlag[EN_CMDQ_TYPE_MAX];
static MS_U8 gbCMDQBuferrFull[EN_CMDQ_TYPE_MAX];
static CMDQ_Buffer_MIU gstMIUSel[EN_CMDQ_TYPE_MAX];
//--------------------------------------------------------------------------------------------------
//  Global Variable
//--------------------------------------------------------------------------------------------------
static MS_U32 gu32CMDQMode;
static EN_CMDQ_IP_TYPE gstCurrworkIP;
static MS_U32 gu32LasttimeFramecnt;//1.last time isp cnt
MS_BOOL gbISRopen;
MS_U16  gu16ISRflag;
MS_S32 _gCMDQHVSPMUTEX[EN_CMDQ_TYPE_MAX] = {-1,-1,-1};
static MS_S32 _gs32CMDQMutex = -1;
#define CMDQ_MUTEX_CREATE()  _gs32CMDQMutex = MsOS_CreateMutex(E_MSOS_FIFO, "CMDQMUTEX" , MSOS_PROCESS_SHARED)
#define CMDQ_MUTEX_LOCK()                       MsOS_ObtainMutex(_gs32CMDQMutex,MSOS_WAIT_FOREVER)
#define CMDQ_MUTEX_UNLOCK()                     MsOS_ReleaseMutex(_gs32CMDQMutex)
#define CMDQ_MUTEX_DELETE()                     MsOS_DeleteMutex(_gs32CMDQMutex)
#define _AlignTo(value, align)                  ( ((value) + ((align)-1)) & ~((align)-1) )
#define _CMDQGetAddr(u16Point)                (gstCMDQInfo[gstCurrworkIP].u32VirAddr + ((u16Point)* MS_CMDQ_CMD_LEN) )
#define _CMDQGetCMD(u8type,u32Addr,u16Data,u16Mask) ((MS_U64)(((MS_U64)(u8type)<<(14*4))|((MS_U64)(u32Addr)<<(8*4))|\
                                                ((MS_U64)(u16Data)<<(4*4))|((MS_U64)(u16Mask))))
#define _IsIspCountReset(u8isp_cnt,u8framecnt)  (((gu32LasttimeFramecnt)>(u8isp_cnt))&&\
                                                ((gu32LasttimeFramecnt)>(u8framecnt)))
#define _IsAssignCountReset(u8isp_cnt,u8framecnt)  ((u8framecnt<u8isp_cnt)&&\
                                                    ((gu32LasttimeFramecnt)>=(u8framecnt)))
#define _IsFrameCountError(u8isp_cnt,u8framecnt,u8allowframeerror) (((MS_S16)u8isp_cnt)>(MS_S16)((u8framecnt)+(u8allowframeerror)))\
                                                                &&((u8isp_cnt-u8framecnt)<10)
#define _IsFrameCountWarn(u8isp_cnt,u8framecnt,u8allowframeerror) (((MS_S16)(u8isp_cnt)-(MS_S16)(u8framecnt))>=0&&\
                                                      ((MS_S16)(u8isp_cnt)-(MS_S16)(u8framecnt))<=(u8allowframeerror))
#define _IsCMDQExecuteDone(enIPType)              (Hal_CMDQ_GetFinalIrq(enIPType, 0x0003)==0x3 && Hal_CMDQ_GetRawIrq(enIPType, 0x0800))
#define _IsCMDQExecuteDoneISR(enIPType)           (((gu16ISRflag&0x4) == 0x4) && Hal_CMDQ_GetRawIrq(enIPType, 0x0800))
#define _IsCMDQIPIdle(enIPType)               (Hal_CMDQ_GetRawIrq(enIPType, 0x0800))
#define _IsCMDQDMADone(enIPType)                   (Hal_CMDQ_GetRawIrq(enIPType, 0x0002))
#define _IsCMDQFlagType(IpNum,u8type)               (gCMDQStatusFlag[IpNum] & (u8type))
#define _SetCMDQFlagType(IpNum,u8type)              (gCMDQStatusFlag[IpNum] |= (u8type))
#define _ReSetCMDQFlagType(IpNum,u8type)            (gCMDQStatusFlag[IpNum] &= ~(u8type))
#define _IsThisCmdAddrAlreadyExist(u32addr,u64cpcmd) (u32addr==((MS_U32)((u64cpcmd &0xFFFFFFFF00000000)>>32)))
#define _IsIdxLargeThan(u32ShiftIdx,u16Num) (u32ShiftIdx>u16Num)
//--------------------------------------------------------------------------------------------------
//  Debug Function
//--------------------------------------------------------------------------------------------------
#define  CMDQDBG(x)
#define  CMDQERR(x) x
#define  CMDQCMD128 0 //one cmd 64bit but add one null cmd
#define  NotToCheckSameAddr 1
#define  ToCheckSameAddr 0
#define  RETURN_ERROR 2
#define CheckingTimes 35
//#define  INT_IRQ_CMDQ 52
//--------------------------------------------------------------------------------------------------
//  Global Function
//--------------------------------------------------------------------------------------------------

//---------------------------------------------------------------------------
///Get MMIO_Base and set struct _REG_CMDQCtrl * _CMDQCtrl
//---------------------------------------------------------------------------
void Drv_CMDQ_SetForceSkip(bool bEn)
{
}


//DRV-----------------------------------------------------------------------------------------------
MS_U32 _Drv_CMDQ_SetRingTrigPointer(EN_CMDQ_IP_TYPE enIPType,MS_U32 StartAddr)
{
    Hal_CMDQ_SetOffsetPointer(enIPType,StartAddr);
    return 1;
}

MS_U32 _Drv_CMDQ_SetStartPointer(EN_CMDQ_IP_TYPE enIPType,MS_U32 StartAddr)
{
    Hal_CMDQ_SetStartPointer(enIPType,StartAddr);
    return DRVCMDQ_OK;
}

MS_U32 _Drv_CMDQ_SetEndPointer(EN_CMDQ_IP_TYPE enIPType,MS_U32 EndAddr)
{
    Hal_CMDQ_SetEndPointer(enIPType,EndAddr);
    return DRVCMDQ_OK;
}

MS_U32 _Drv_CMDQ_SetTimerRatio(EN_CMDQ_IP_TYPE enIPType,MS_U32 u32time,MS_U32 u32ratio)
{
    Hal_CMDQ_SetTimer(enIPType, u32time);
    Hal_CMDQ_SetRatio(enIPType, u32ratio);
    return DRVCMDQ_OK;
}
MS_U64 *_Drv_CMDQ_GetPointerFromPoint(MS_U16 u16Point,MS_U64 *pu32Addr)
{
    MS_PHYADDR DstAddr;
    DstAddr = _CMDQGetAddr(u16Point);
    pu32Addr = (MS_U64 *)(DstAddr);
    return pu32Addr;
}
void _Drv_CMDQ_SetCmdToPointer(MS_U64 u64Cmd ,MS_BOOL bSkipCheckSameAddr,MS_U64 *pu32Addr)
{
    if(!bSkipCheckSameAddr)
    {
        *pu32Addr = ((u64Cmd &0xFFFFFFFFFFFF0000)|0x0000);
    }
    else
    {
        *pu32Addr =u64Cmd ;
    }
}
MS_BOOL _Drv_CMDQ_AddCmd(MS_U64 u64Cmd ,MS_BOOL bSkipCheckSameAddr)
{
    MS_U64 *pu32Addr = NULL;
    if(gbCMDQBuferrFull[gstCurrworkIP])
    {
        return 0;
    }
    pu32Addr = _Drv_CMDQ_GetPointerFromPoint(gstCMDQInfo[gstCurrworkIP].u16WPoint,pu32Addr);
    _Drv_CMDQ_SetCmdToPointer(u64Cmd,bSkipCheckSameAddr,pu32Addr);
    gstCMDQInfo[gstCurrworkIP].u16WPoint++;
    SCL_DBG(SCL_DBG_LV_DRVCMDQ()&EN_DBGMG_CMDQEVEL_NORMAL,"[CMDQ]add u16WPoint=%d \n"
        ,gstCMDQInfo[gstCurrworkIP].u16WPoint);
    if( ((gstCMDQInfo[gstCurrworkIP].u16WPoint)) ==((gstCMDQInfo[gstCurrworkIP].u16DPoint)))
    {
        gbCMDQBuferrFull[gstCurrworkIP] = 1;
        SCL_ERR("[CMDQ]CMDQ BUFFER FULL @:%lu\n",((MS_U32)MsOS_GetSystemTimeStamp()));
        SCL_ERR("[CMDQ]add u16DPoint=%d u16WPoint=%d\n"
            ,gstCMDQInfo[gstCurrworkIP].u16DPoint,gstCMDQInfo[gstCurrworkIP].u16WPoint);
    }
    gu32LasttimeFramecnt++;
    return 1;
}

void _Drv_CMDQ_ResetCmdPointIfBottom(EN_CMDQ_IP_TYPE enIPType)
{
    if(gstCMDQInfo[enIPType].u16WPoint >= gstCMDQInfo[enIPType].u16MaxCmdCnt)
    {
        gstCMDQInfo[enIPType].u16WPoint = 0;
        //SCL_DBGERR("[CMDQ]CMDQ BUFFER ResetCmdPoint @:%lu\n",((MS_U32)MsOS_GetSystemTimeStamp()));
    }
}
void _Drv_CMDQ_AddNull(EN_CMDQ_IP_TYPE enIPType)
{
    MS_U64 nullcmd = MS_CMDQ_NULL_CMD;
    do
    {
        _Drv_CMDQ_AddCmd((MS_U64)nullcmd, ToCheckSameAddr);
        _Drv_CMDQ_ResetCmdPointIfBottom(enIPType);
    }while (gstCMDQInfo[enIPType].u16WPoint%MS_CMDQ_CMD_ALIGN != 0);
}

void _Drv_CMDQ_WriteRegDirect(MS_U32 u32Addr,MS_U16 u16Data)
{
    Hal_CMDQ_WriteRegDirect(u32Addr,u16Data);
}

MS_U32 _Drv_CMDQ_PrintfCrashCommand(EN_CMDQ_IP_TYPE enIPType)
{
    MS_U32 Command_15_0_bit, Command_31_16_bit, Command_55_32_bit, Command_63_56_bit,read_addr;
    Command_15_0_bit = Hal_CMDQ_ErrorCommand(enIPType,CMDQ_CRASH_15_0_BIT);
    Command_31_16_bit = Hal_CMDQ_ErrorCommand(enIPType,CMDQ_CRASH_31_16_BIT);
    Command_55_32_bit = Hal_CMDQ_ErrorCommand(enIPType,CMDQ_CRASH_55_32_BIT);
    Command_63_56_bit = Hal_CMDQ_ErrorCommand(enIPType,CMDQ_CRASH_63_56_BIT);
    read_addr = Hal_CMDQ_GetMIUReadAddr(enIPType);
    SCL_DBG(SCL_DBG_LV_DRVCMDQ()&EN_DBGMG_CMDQEVEL_NORMAL,
        "[CMDQ]\033[35mCommand _bits: %lx, %lx, %lx, %lx read_addr: %lx\033[m\n"
        , Command_63_56_bit, Command_55_32_bit, Command_31_16_bit, Command_15_0_bit,read_addr);
    return DRVCMDQ_OK;
}
MS_U32 Drv_CMDQ_BeTrigger(EN_CMDQ_IP_TYPE enIPType,MS_BOOL bStart)
{
    if(bStart)
    {
        Hal_CMDQ_Enable(enIPType,bStart);
        Hal_CMDQ_Start(enIPType,bStart);
        _Drv_CMDQ_PrintfCrashCommand(enIPType);
        Drv_CMDQ_SetDPoint(enIPType);
        return DRVCMDQ_OK;
    }
    else
    {
        CMDQERR(printf("bStart %d, \n",bStart));
        return DRVCMDQ_FAIL;
    }
}
void _Drv_CMDQ_CheckMIUAddr(void)
{
    MS_U64 *pu32Addr = NULL;
    #if CMDQCMD128
        pu32Addr = _Drv_CMDQ_GetPointerFromPoint((gstCMDQInfo[gstCurrworkIP].u16WPoint -2),pu32Addr);
        SCL_DBG(SCL_DBG_LV_DRVCMDQ()&EN_DBGMG_CMDQEVEL_HIGH,"[CMDQ] WP:%hd MIU:%llx"
            ,gstCMDQInfo[gstCurrworkIP].u16WPoint -2,*pu32Addr);
    #else
        pu32Addr = _Drv_CMDQ_GetPointerFromPoint((gstCMDQInfo[gstCurrworkIP].u16WPoint -1),pu32Addr);
        SCL_DBG(SCL_DBG_LV_DRVCMDQ()&EN_DBGMG_CMDQEVEL_HIGH,"[CMDQ] WP:%hd MIU:%llx"
            ,gstCMDQInfo[gstCurrworkIP].u16WPoint -1,*pu32Addr);
    #endif
}
void Drv_CMDQ_SetRPoint(EN_CMDQ_IP_TYPE enIPType)
{
    gstCMDQInfo[enIPType].u16RPoint =
        (gstCMDQInfo[enIPType].u16FPoint>0)? (gstCMDQInfo[enIPType].u16FPoint-1): gstCMDQInfo[enIPType].u16MaxCmdCnt;
}
void Drv_CMDQ_SetDPoint(EN_CMDQ_IP_TYPE enIPType)
{
    gstCMDQInfo[enIPType].u16DPoint =
        (gstCMDQInfo[enIPType].u16FPoint>0)? (gstCMDQInfo[enIPType].u16FPoint-1): gstCMDQInfo[enIPType].u16MaxCmdCnt;
}
void Drv_CMDQ_SetLPoint(EN_CMDQ_IP_TYPE enIPType)
{
    static MS_U16 u16lp=0;
    gstCMDQInfo[enIPType].u16LPoint = u16lp;
    u16lp = gstCMDQInfo[enIPType].u16DPoint;
    _Drv_CMDQ_PrintfCrashCommand(enIPType);
}
MS_U32 _Drv_CMDQ_GetEndCmdAddr(EN_CMDQ_IP_TYPE enIPType)
{
    MS_U32 u32EndAddr = 0;
    gstCMDQInfo[enIPType].u16FPoint = gstCMDQInfo[enIPType].u16WPoint + 1;
    SCL_DBG(SCL_DBG_LV_DRVCMDQ()&EN_DBGMG_CMDQEVEL_LOW,"[CMDQ]u16WPoint=%u u16RPoint=%u u16DPoint=%u\n",
        gstCMDQInfo[enIPType].u16WPoint,gstCMDQInfo[enIPType].u16RPoint,gstCMDQInfo[enIPType].u16DPoint);

    u32EndAddr = gstCMDQInfo[enIPType].PhyAddr+
        (((gstCMDQInfo[enIPType].u16FPoint)/MS_CMDQ_CMD_ALIGN)*MS_CMDQ_MEM_BASE_UNIT);
    return u32EndAddr;
}
void _Drv_CMDQ_ReSetBuferrFull(EN_CMDQ_IP_TYPE enIPType)
{
    gbCMDQBuferrFull[enIPType] = 0;
}
void _Drv_CMDQ_FillInitIPInfo
    (EN_CMDQ_IP_TYPE enIPType,MS_PHYADDR IP0PhyAddr, MS_U32 u32IP0VirAddr, MS_U32 u32CMDQBufSize)
{
    SCL_DBG(SCL_DBG_LV_DRVCMDQ()&EN_DBGMG_CMDQEVEL_NORMAL,
        "[CMDQ]PhyAddr:%lx   u32VirAddr:%lx \n",IP0PhyAddr,u32IP0VirAddr);
    gstCMDQInfo[enIPType].bEnable = FALSE;
    gstCMDQInfo[enIPType].u16RPoint = 0;             //last trig
    gstCMDQInfo[enIPType].u16DPoint = 0;             //last done
    gstCMDQInfo[enIPType].u16WPoint = 0;             //current cmd end
    gstCMDQInfo[enIPType].u16FPoint = 0;             //fire point (128 bit/unit)
    gstCMDQInfo[enIPType].u16MaxCmdCnt = (MS_U16)(u32CMDQBufSize / MS_CMDQ_CMD_LEN);
    gstCMDQInfo[enIPType].PhyAddr = IP0PhyAddr;
    gstCMDQInfo[enIPType].u32VirAddr = u32IP0VirAddr;
    gstCMDQInfo[enIPType].PhyAddrEnd = IP0PhyAddr+
        (gstCMDQInfo[enIPType].u16MaxCmdCnt/2)*MS_CMDQ_MEM_BASE_UNIT;
    gCMDQStatusFlag[enIPType] = (CMDQ_FLAG_FIRE);
    gbCMDQBuferrFull[enIPType] = 0;
}
void _Drv_CMDQ_Init(MS_U32 phyAddr1)
{
    if (phyAddr1 >= HAL_MIU1_BASE)
    {
        gstMIUSel[0].u8MIUSel_IP = EN_CMDQ_MIU_1;
        SCL_DBG(SCL_DBG_LV_DRVCMDQ()&EN_DBGMG_CMDQEVEL_LOW,"[CMDQ]%s: Init BF1 at MIU 1\n"
            ,__FUNCTION__);
    }
    else
    {
        gstMIUSel[0].u8MIUSel_IP = EN_CMDQ_MIU_0;
        SCL_DBG(SCL_DBG_LV_DRVCMDQ()&EN_DBGMG_CMDQEVEL_LOW,"[CMDQ]%s: Init BF1 at MIU 0 \n"
            ,__FUNCTION__);
    }
    gu32CMDQMode = RING_BUFFER_MODE;
    gu32LasttimeFramecnt = 0;
    gstCurrworkIP = EN_CMDQ_TYPE_IP0;
}
void _Drv_CMDQ_Reset(EN_CMDQ_IP_TYPE enIPType)
{
    MS_U32 u32CMDQBufSize =
        (gstCMDQInfo[enIPType].u16MaxCmdCnt)*MS_CMDQ_CMD_LEN;
    _Drv_CMDQ_FillInitIPInfo(enIPType, gstCMDQInfo[enIPType].PhyAddr,
        gstCMDQInfo[enIPType].u32VirAddr,u32CMDQBufSize);
    Drv_CMDQ_Enable(1,enIPType);
    _Drv_CMDQ_Init(gstCMDQInfo[enIPType].PhyAddr);
    Hal_CMDQ_Reset(enIPType, ENABLE);
    Hal_CMDQ_Reset(enIPType, DISABLE);
    _Drv_CMDQ_AddNull(enIPType);//add 2 null
    gstCMDQInfo[enIPType].u16FPoint = gstCMDQInfo[enIPType].u16WPoint + 1;
}
MS_BOOL _Drv_CMDQ_Fire(EN_CMDQ_IP_TYPE enIPType,MS_BOOL bStart)
{
    MS_U32 u32EndAddr = 0;
    MS_U8 ret=0;
    MS_U32 u32Events=0;
    SCL_DBG(SCL_DBG_LV_DRVCMDQ()&EN_DBGMG_CMDQEVEL_LOW,"[CMDQ]%s count:%ld\n",__FUNCTION__,gu32LasttimeFramecnt);
    if(gbCMDQBuferrFull[enIPType])
    {
        gstCMDQInfo[enIPType].u16WPoint = gstCMDQInfo[enIPType].u16RPoint;
        ret=0;
    }
    else
    {
        gu32LasttimeFramecnt = 0;
        u32EndAddr = _Drv_CMDQ_GetEndCmdAddr(enIPType);
        ret =_Drv_CMDQ_SetRingTrigPointer(enIPType,u32EndAddr);  //in function,addr already add 1
    }
    if(ret)
    {
        //MsOS_ChipFlushCacheRange(gstCMDQInfo[enIPType].u32VirAddr,(gstCMDQInfo[enIPType].u16MaxCmdCnt*MS_CMDQ_CMD_LEN));
        MsOS_WaitForCPUWriteToDMem();
        _Drv_CMDQ_CheckMIUAddr();
        MsOS_ClearEventIRQ(Drv_SCLIRQ_Get_IRQ_SYNCEventID(),E_SCLIRQ_EVENT_CMDQ);
        MsOS_SetEvent_IRQ(Drv_SCLIRQ_Get_IRQ_SYNCEventID(), E_SCLIRQ_EVENT_CMDQFIRE);
        //_Drv_CMDQ_BeTrigger(enIPType,bStart);
        Drv_CMDQ_SetRPoint(enIPType);
        SCL_DBG(SCL_DBG_LV_DRVCMDQ()&EN_DBGMG_CMDQEVEL_NORMAL,"[CMDQ]go CMDQ u32EndAddr:%lx\n",u32EndAddr);
    }
    else
    {
        CMDQERR(printf("[CMDQ]Block Fire for CMDQ FULL!!!\n"));
        if(MsOS_WaitEvent(Drv_SCLIRQ_Get_IRQ_SYNCEventID(),
            E_SCLIRQ_EVENT_CMDQDONE, &u32Events, E_OR, 1000))
        {
            _Drv_CMDQ_ReSetBuferrFull(enIPType);
        }
        else
        {
            _Drv_CMDQ_Reset(enIPType);
            _Drv_CMDQ_ReSetBuferrFull(enIPType);
            CMDQERR(printf("[CMDQ]Reset CMDQ!!!\n"));
        }
        return FALSE;
    }

    return TRUE;
}
MS_U8 _Drv_CMDQ_SetBufferConfig(EN_CMDQ_IP_TYPE enIPType,MS_U32 StartAddr, MS_U32 EndAddr)
{
    MS_U8 ret,ret2;
    ret = _Drv_CMDQ_SetStartPointer(enIPType,StartAddr);
    ret2 = _Drv_CMDQ_SetEndPointer(enIPType,EndAddr);
    _Drv_CMDQ_SetRingTrigPointer(enIPType,StartAddr);
    if((ret==DRVCMDQ_OK)&&(ret2==DRVCMDQ_OK))
    return 1;
    else
    return 0;
}
void _Drv_CMDQ_initSCtopIRQ(void)
{
    Hal_CMDQ_WriteRegDirect(0x12183C,0xFFFF);//clear sctop irq
    Hal_CMDQ_WriteRegDirect(0x121838,0xFFFD);//mask
}
MS_U32 _Drv_CMDQ_InitByIP(EN_CMDQ_IP_TYPE enIPType)
{
    char word[] = {"_CMDQ_Mutex"};
    char word1[] = {"_CMDQ1_Mutex"};
    char word2[] = {"_CMDQ2_Mutex"};
    Hal_CMDQ_Setmiusel(enIPType, gstMIUSel[enIPType].u8MIUSel_IP);
    _Drv_CMDQ_AddNull(enIPType);//add 2 null
    gstCMDQInfo[enIPType].u16FPoint = gstCMDQInfo[enIPType].u16WPoint + 1;
    Hal_CMDQ_SetBaseEn(enIPType, ENABLE);
    Hal_CMDQ_SetTrigAlways(enIPType, ENABLE);
    Hal_CMDQ_Reset(enIPType, ENABLE);
    Hal_CMDQ_Reset(enIPType, DISABLE);

    if(FALSE == Hal_CMDQ_SetDMAMode(enIPType, gu32CMDQMode))
    {
        CMDQERR(printf("[CMDQ]\033[35mCMDQ Set Mode FAIL!!\033[m\n"));
        return DRVCMDQ_FAIL;
    }

    /* the polling_timer is for re-checking polling value,
        if the time_interval is too small, cmdq will polling RIU frequently, so that RIU will very busy */
    _Drv_CMDQ_SetTimerRatio(enIPType, CMDQ_poll_timer, CMDQ_timer_ratio);
    // set time / ratio, total wait time is (wait_time * (ratio + 1)) => ( FF * (F + 1) / 216  MHz ) = sec
    //4D*1=4D~=77 *(216/M) 1 ~=60(frame/s)
    Drv_CMDQ_SetSkipPollWhenWaitTimeOut(enIPType, 0);
    _Drv_CMDQ_SetBufferConfig(enIPType, gstCMDQInfo[enIPType].PhyAddr,gstCMDQInfo[enIPType].PhyAddrEnd);
    _Drv_CMDQ_initSCtopIRQ();
    Hal_CMDQ_ResetSoftInterrupt(enIPType);
    Hal_CMDQ_SetISRMSK(enIPType,0xE0F8);
    Hal_CMDQ_SetRegPassWaitPoll(enIPType,1);
    _gCMDQHVSPMUTEX[enIPType] = MsOS_CreateMutex(E_MSOS_FIFO,
        (enIPType==EN_CMDQ_TYPE_IP0) ? word : (enIPType==EN_CMDQ_TYPE_IP1) ? word1 : word2, MSOS_PROCESS_SHARED);
    if (_gCMDQHVSPMUTEX[enIPType] == -1)
    {
        CMDQERR(printf("%s: Init MUTEX fail\n", __FUNCTION__));
        return FALSE;
    }
//---------------------------  irq
    return DRVCMDQ_OK;
}
//function-----------------------------------------------------------------------------
MS_BOOL _Drv_CMDQ_FillCheckCmdInfo(MS_U64 u64Cmd,CMDQ_CheckCmdinfo *stCMDQChkinfo)
{
    stCMDQChkinfo->Ret = 0;
    stCMDQChkinfo->u32addr = (MS_U32)((u64Cmd &0xFFFFFFFF00000000)>>32);
    stCMDQChkinfo->u16mask = (MS_U16)(u64Cmd &0xFFFF);
    stCMDQChkinfo->u16data = (MS_U16)((u64Cmd>>16) &0xFFFF);
    stCMDQChkinfo->u16StartPoint = gstCMDQInfo[gstCurrworkIP].u16DPoint;
    if(gstCMDQInfo[gstCurrworkIP].u16WPoint>0)
    {
        stCMDQChkinfo->u16EndPoint = gstCMDQInfo[gstCurrworkIP].u16WPoint-1;//for read last time cmd
    }
    else
    {
        stCMDQChkinfo->u16EndPoint = gstCMDQInfo[gstCurrworkIP].u16MaxCmdCnt-1;
    }

    if (stCMDQChkinfo->u16EndPoint+1==stCMDQChkinfo->u16StartPoint)
    {
        stCMDQChkinfo->Ret = 1;
        return stCMDQChkinfo->Ret;
    }

    if(stCMDQChkinfo->u16EndPoint+1>=stCMDQChkinfo->u16StartPoint)
    {
        stCMDQChkinfo->u32CmdDiffCnt = stCMDQChkinfo->u16EndPoint+1-stCMDQChkinfo->u16StartPoint;
    }
    else   //ring
    {
        stCMDQChkinfo->u32CmdDiffCnt =
            stCMDQChkinfo->u16EndPoint+1+(gstCMDQInfo[gstCurrworkIP].u16MaxCmdCnt-stCMDQChkinfo->u16StartPoint);
    }

    return stCMDQChkinfo->Ret;
}
MS_U16 _Drv_CMDQ_GetCurrentCmdPoint(CMDQ_CheckCmdinfo *stCMDQChkinfo, MS_U32 u32ShiftIdx)
{
    MS_U16 u16CurrentPoint;
    MS_U16 u16Temp;
    if(((stCMDQChkinfo->u16EndPoint+1)>=stCMDQChkinfo->u16StartPoint))
    {
        u16CurrentPoint = (stCMDQChkinfo->u16EndPoint-u32ShiftIdx);
    }
    else if(((stCMDQChkinfo->u16EndPoint+1)<stCMDQChkinfo->u16StartPoint))
    {
        if((stCMDQChkinfo->u16EndPoint)>=u32ShiftIdx)//top
        {
            u16CurrentPoint = (stCMDQChkinfo->u16EndPoint-u32ShiftIdx);
        }
        else//bottom
        {
            u16Temp = u32ShiftIdx-stCMDQChkinfo->u16EndPoint;
            u16CurrentPoint = (gstCMDQInfo[gstCurrworkIP].u16MaxCmdCnt-u16Temp);
        }
    }
    else
    {
        u16CurrentPoint = (stCMDQChkinfo->u16EndPoint-u32ShiftIdx);
    }
    return u16CurrentPoint;
}
MS_BOOL _Drv_CMDQ_CheckingAlreadyExist(CMDQ_CheckCmdinfo *stCMDQChkinfo)
{
    MS_U64 u64ExistedCmd;
    MS_U16 u16ExistCmdData;
    MS_U32 u32ShiftIdx;
    MS_U16 u16CurrentPoint;
    for(u32ShiftIdx = 0;u32ShiftIdx<stCMDQChkinfo->u32CmdDiffCnt;u32ShiftIdx++)
    {
        //doing
        u16CurrentPoint = _Drv_CMDQ_GetCurrentCmdPoint(stCMDQChkinfo,u32ShiftIdx);
        stCMDQChkinfo->pu32Addr = _Drv_CMDQ_GetPointerFromPoint(u16CurrentPoint,stCMDQChkinfo->pu32Addr);
        u64ExistedCmd = *stCMDQChkinfo->pu32Addr;
        if(_IsThisCmdAddrAlreadyExist(stCMDQChkinfo->u32addr,u64ExistedCmd))
        {
            //there is same address
            u16ExistCmdData = (MS_U16)((u64ExistedCmd>>16) &0xFFFF);
            u16ExistCmdData = ((u16ExistCmdData&stCMDQChkinfo->u16mask) | (~stCMDQChkinfo->u16mask&stCMDQChkinfo->u16data));
            u64ExistedCmd = ((u64ExistedCmd &0xFFFFFFFF00000000)|((MS_U32)u16ExistCmdData<<16)|0x0000);
            if( _IsIdxLargeThan(u32ShiftIdx,10)) // TO avoid this cmd too far away next cmd.
            {
                _Drv_CMDQ_AddCmd(u64ExistedCmd,0);
#if CMDQCMD128
                _Drv_CMDQ_AddNull(gstCurrworkIP);
#endif
                *stCMDQChkinfo->pu32Addr = MS_CMDQ_NULL_CMD;
            }
            else
            {
                *stCMDQChkinfo->pu32Addr = u64ExistedCmd;
            }
            stCMDQChkinfo->Ret = 0;
            SCL_DBG(SCL_DBG_LV_DRVCMDQ()&EN_DBGMG_CMDQEVEL_HIGH,"[CMDQ]find cmd same address %lx\n"
                ,stCMDQChkinfo->u32addr);
            break;
        }
#if CMDQCMD128
        else if(_IsIdxLargeThan(u32ShiftIdx,CheckingTimes*2))
        {
            stCMDQChkinfo->Ret = 1;
            SCL_DBG(SCL_DBG_LV_DRVCMDQ()&EN_DBGMG_CMDQEVEL_HIGH,"[CMDQ]check over 20\n");
            break;
        }
#else
        else if(_IsIdxLargeThan(u32ShiftIdx,CheckingTimes))
        {
            stCMDQChkinfo->Ret = 1;
            SCL_DBG(SCL_DBG_LV_DRVCMDQ()&EN_DBGMG_CMDQEVEL_HIGH,"[CMDQ]check over 10\n");
            break;
        }
#endif
        else
        {
            stCMDQChkinfo->Ret = 1;
        }

    }
    return stCMDQChkinfo->Ret;
}
MS_BOOL _Drv_CMDQ_CheckAddedCmdSameRegPos(MS_U64 u64Cmd,MS_BOOL bSkipCheckSameAddr)
{
    CMDQ_CheckCmdinfo stCMDQChkinfo;
    MsOS_Memset(&stCMDQChkinfo,0,sizeof(CMDQ_CheckCmdinfo));
    if (bSkipCheckSameAddr)
    {
        stCMDQChkinfo.Ret = 1;
        SCL_DBG(SCL_DBG_LV_DRVCMDQ()&EN_DBGMG_CMDQEVEL_HIGH,"[CMDQ]bSkipCheckSameAddr check cmd repeat:%hhx\n"
            ,bSkipCheckSameAddr);
        return stCMDQChkinfo.Ret;
    }
    _Drv_CMDQ_FillCheckCmdInfo(u64Cmd,&stCMDQChkinfo);
    if(stCMDQChkinfo.Ret)
    {
        return stCMDQChkinfo.Ret;
    }
    SCL_DBG(SCL_DBG_LV_DRVCMDQ()&EN_DBGMG_CMDQEVEL_HIGH,"[CMDQ]u16EndPoint:%hd u16StartPoint:%hd u32CmdDiffCnt:%ld\n"
        ,stCMDQChkinfo.u16EndPoint,stCMDQChkinfo.u16StartPoint,stCMDQChkinfo.u32CmdDiffCnt);
    //checking
    stCMDQChkinfo.Ret = _Drv_CMDQ_CheckingAlreadyExist(&stCMDQChkinfo);
    return stCMDQChkinfo.Ret;
}

MS_BOOL _Drv_CMDQ_BufferEmpty(EN_CMDQ_IP_TYPE eCMDQBuf)
{
    if((gstCMDQInfo[eCMDQBuf].u16WPoint == gstCMDQInfo[eCMDQBuf].u16RPoint) &&
       (gstCMDQInfo[eCMDQBuf].u16WPoint == gstCMDQInfo[eCMDQBuf].u16FPoint))
        return TRUE;
    else
        return FALSE;
}
MS_BOOL _Drv_CMDQ_WriteCmd(MS_U64 u64Cmd,MS_BOOL bSkipCheckSameAddr)
{
    MS_BOOL bRet = 0;
    MS_U16 u16DummyCmdIdx;
    EN_CMDQ_IP_TYPE enCMDQIP = gstCurrworkIP;
    CMDQDBG(printf("[CMDQ] u64Cmd:%llx\n",u64Cmd));
    if( _Drv_CMDQ_BufferEmpty(enCMDQIP) )
    {
        CMDQERR(printf("[CMDQ] buffer empty init\n"));
        gstCMDQInfo[enCMDQIP].u16RPoint = 0;
        gstCMDQInfo[enCMDQIP].u16WPoint = 0;
        gstCMDQInfo[enCMDQIP].u16FPoint = 0;
        _Drv_CMDQ_AddNull(gstCurrworkIP);
        _Drv_CMDQ_AddCmd(u64Cmd,bSkipCheckSameAddr);
        bRet = TRUE;
    }
    else
    {

        u16DummyCmdIdx=(gstCMDQInfo[enCMDQIP].u16WPoint/2+1)*2;
        if(u16DummyCmdIdx < gstCMDQInfo[enCMDQIP].u16MaxCmdCnt)//med
        {
            if(_Drv_CMDQ_CheckAddedCmdSameRegPos(u64Cmd,bSkipCheckSameAddr))
            {
                _Drv_CMDQ_AddCmd(u64Cmd,bSkipCheckSameAddr);
            #if CMDQCMD128
                _Drv_CMDQ_AddNull(gstCurrworkIP);
            #endif
            }
            bRet = TRUE;
        }
        else if(u16DummyCmdIdx == gstCMDQInfo[enCMDQIP].u16MaxCmdCnt)//last
        {
            if(gstCMDQInfo[enCMDQIP].u16WPoint+1 < gstCMDQInfo[enCMDQIP].u16MaxCmdCnt)//last+2
            {
                if(_Drv_CMDQ_CheckAddedCmdSameRegPos(u64Cmd,bSkipCheckSameAddr))
                {
                    _Drv_CMDQ_AddCmd(u64Cmd,bSkipCheckSameAddr);
                #if CMDQCMD128
                    _Drv_CMDQ_AddNull(gstCurrworkIP);
                #endif
                    SCL_DBG(SCL_DBG_LV_DRVCMDQ()&EN_DBGMG_CMDQEVEL_NORMAL,
                    "[CMDQ]FC last+2 u16MaxCmdCnt=%d u16WPoint=%d\n"
                        ,gstCMDQInfo[enCMDQIP].u16MaxCmdCnt,gstCMDQInfo[enCMDQIP].u16WPoint);
                }
                bRet = TRUE;
            }
            else if(gstCMDQInfo[enCMDQIP].u16WPoint+1 == gstCMDQInfo[enCMDQIP].u16MaxCmdCnt)//last+1
            {
                if(_Drv_CMDQ_CheckAddedCmdSameRegPos(u64Cmd,bSkipCheckSameAddr))
                {
                    _Drv_CMDQ_AddCmd(u64Cmd,bSkipCheckSameAddr);
                    SCL_DBG(SCL_DBG_LV_DRVCMDQ()&EN_DBGMG_CMDQEVEL_NORMAL,
                        "[CMDQ]FC last+1 u16MaxCmdCnt=%d u16WPoint=%d\n"
                        ,gstCMDQInfo[enCMDQIP].u16MaxCmdCnt,gstCMDQInfo[enCMDQIP].u16WPoint);
                }
                _Drv_CMDQ_ResetCmdPointIfBottom(enCMDQIP);
                bRet = TRUE;
            }

        }
        else if(u16DummyCmdIdx > gstCMDQInfo[enCMDQIP].u16MaxCmdCnt)//last
        {
            _Drv_CMDQ_ResetCmdPointIfBottom(enCMDQIP);
            if(_Drv_CMDQ_CheckAddedCmdSameRegPos(u64Cmd,bSkipCheckSameAddr))
            {
                SCL_DBG(SCL_DBG_LV_DRVCMDQ()&EN_DBGMG_CMDQEVEL_NORMAL,"[CMDQ] frame cnt buffer full -->reset\n");
                _Drv_CMDQ_AddCmd(u64Cmd,bSkipCheckSameAddr);
            #if CMDQCMD128
                _Drv_CMDQ_AddNull(gstCurrworkIP);
            #endif
                SCL_DBG(SCL_DBG_LV_DRVCMDQ()&EN_DBGMG_CMDQEVEL_NORMAL,"[CMDQ]FC last u16MaxCmdCnt=%d u16WPoint=%d\n"
                    ,gstCMDQInfo[enCMDQIP].u16MaxCmdCnt,gstCMDQInfo[enCMDQIP].u16WPoint);
            }
            bRet = TRUE;
        }
        else
        {
            bRet = FALSE;
        }


    }
    return bRet;
}
MS_U64 _Drv_CMDQ_GetCmd(MS_U8 u8type,MS_U32 u32Addr, MS_U16 u16Data, MS_U16 u16Mask)
{
    MS_U16 u16Bank;
    MS_U8 u8addr;
    MS_CMDQ_CMD data;
    u16Bank = (MS_U16)((u32Addr >> 8) & 0xFFFF);
    u8addr = (MS_U8)((u32Addr & 0xFF) );
    if(u8addr%2)  //Hbyte
    {
        data.u16Mask=~(u16Mask<<8);
        data.u16Data = (Hal_CMDQ_Read2ByteReg(u32Addr-1) & ~(u16Mask<<8)) | ((u16Data & u16Mask)<<8);
    }
    else
    {
        data.u16Mask=~(u16Mask);
        if( u16Mask == 0xFFFF )
        {
            data.u16Data = u16Data;
        }
        else
        {
            data.u16Data = (Hal_CMDQ_Read2ByteReg(u32Addr) & ~u16Mask) | (u16Data & u16Mask);
        }
    }

    data.u8type = u8type;
    data.u32Addr =((((MS_U32)u16Bank<<4)*8)|((MS_U32)u8addr/2));
    data.u64Cmd = _CMDQGetCMD(data.u8type,data.u32Addr,data.u16Data,data.u16Mask);
    return data.u64Cmd;
}
void _Drv_CMDQ_PutCmd(MS_U8 u8type, MS_CMDQ_CMDReg *stCfg,MS_BOOL bSkipCheckSameAddr)
{
    MS_U64 u64CMD;
    u64CMD = _Drv_CMDQ_GetCmd(u8type,stCfg->u32Addr,stCfg->u16Data,stCfg->u16Mask);
    _Drv_CMDQ_WriteCmd(u64CMD,bSkipCheckSameAddr);//cmd:real cmd
}
void _Drv_CMDQ_AddWaitLDCTrigCMD(EN_CMDQ_IP_TYPE enIPType)
{
    #if 0
    _Drv_CMDQ_AddNull(enIPType);
    _Drv_CMDQ_PutCmd(CMDQ_CMDTYPE_WRITE,0x12183C,0xFFFD,0xFFFF,1);
    Drv_CMDQ_WaitCmd(sc2cmdq_lv_trig);//wait
    _Drv_CMDQ_AddNull(enIPType);
    _Drv_CMDQ_PutCmd(CMDQ_CMDTYPE_WRITE,0x12183C,0xFFFF,0xFFFF,1);
    #endif
}
//--------------------------------------------------------------------------------------------------
// In ring-buffer mode, this function will trig for update reg_sw_wr_mi_wadr
// The CMDQ will keep on executing cmd until reg_rd_mi_radr reach reg_sw_wr_mi_wadr
//--------------------------------------------------------------------------------------------------

//API--------------------------------------------------------------------------------------------------
void _Drv_CMDQ_WriteRegWithMaskDirect(MS_U32 u32Addr,MS_U16 u16Data,MS_U16 u16Mask)
{
    Hal_CMDQ_WriteRegMaskDirect(u32Addr,u16Data,u16Mask);
}

MS_U64 Drv_CMDQ_GetCMDFromPoint(EN_CMDQ_IP_TYPE enIPType ,MS_U16 u16Point)
{
    MS_U64 *pu32Addr = NULL;
    CMDQ_MUTEX_LOCK();
    gstCurrworkIP = enIPType;
    pu32Addr = _Drv_CMDQ_GetPointerFromPoint((u16Point),pu32Addr);
    CMDQ_MUTEX_UNLOCK();
    return (MS_U64)(*pu32Addr);
}
MS_U32 Drv_CMDQ_GetCMDBankFromCMD(MS_U64 u64Cmd)
{
    MS_U32 u32BankAddr;
    MS_U16 u16Addr;
    MS_U32 u32Bank;
    u32BankAddr = (MS_U32)((u64Cmd &0x00FFFFFF00000000 )>>32) ;
    if((u32BankAddr&0x0000FF)>=0x80) // odd bank
    {
        u16Addr = (MS_U16)((u32BankAddr&0x0000FF)-0x80);
    }
    else // even bank
    {
        u16Addr = (MS_U16)(u32BankAddr&0x0000FF);
    }
    u32Bank = (MS_U32)(((u32BankAddr - u16Addr))*2) ;
    u32BankAddr = (MS_U32)(u32Bank + u16Addr);

    return u32BankAddr;
}

MS_U8 Drv_CMDQ_GetISPHWCnt(void)
{
    MS_U16 isp_cnt;
    isp_cnt = Hal_CMDQ_Get_ISP_Cnt();
    isp_cnt = ((isp_cnt>>8)&0x7F);
    return (MS_U8)isp_cnt;

}

void Drv_CMDQ_SetSkipPollWhenWaitTimeOut(EN_CMDQ_IP_TYPE enIPType ,MS_U16 bEn)
{
    Hal_CMDQ_SetSkipPollWhenWaitTimeout(enIPType,bEn);
    Hal_CMDQ_SetTimeoutAmount(enIPType);
}

void Drv_CMDQ_Enable(MS_BOOL bEnable,EN_CMDQ_IP_TYPE enIPType)
{
    CMDQ_MUTEX_LOCK();
    if(bEnable)
    {
        gstCMDQInfo[enIPType].bEnable++;
    }
    else
    {
        if(gstCMDQInfo[enIPType].bEnable)
        {
            gstCMDQInfo[enIPType].bEnable--;
        }
    }
    CMDQ_MUTEX_UNLOCK();
    SCL_DBG(SCL_DBG_LV_DRVCMDQ()&EN_DBGMG_CMDQEVEL_LOW, "[CMDQ]%s enable:%hhd\n"
        ,__FUNCTION__,gstCMDQInfo[enIPType].bEnable);
}
void Drv_CMDQ_Init(ST_VIP_OPEN_CONFIG *stCMDQIniCfg)
{
    MS_PHYADDR IP0PhyAddr;
    MS_U32 u32CMDQBufSize;
    MS_U32 u32IP0VirAddr;
    EN_CMDQ_IP_TYPE i;
    Hal_CMDQ_InitRIUBase(stCMDQIniCfg->u32RiuBase);
    CMDQ_MUTEX_CREATE();
    for(i=EN_CMDQ_TYPE_IP0 ;i<EN_CMDQ_TYPE_MAX;i++)
    {
        if(stCMDQIniCfg->u32CMDQ_Phy[i] && stCMDQIniCfg->u32CMDQ_Vir[i]&& stCMDQIniCfg->u32CMDQ_Size[i])
        {
            u32CMDQBufSize = stCMDQIniCfg->u32CMDQ_Size[i] / CMDQ_NUMBER;//  0x4000/2
            IP0PhyAddr = stCMDQIniCfg->u32CMDQ_Phy[i];
            u32IP0VirAddr = stCMDQIniCfg->u32CMDQ_Vir[i];
            MsOS_Memset((void *)stCMDQIniCfg->u32CMDQ_Vir[i], 0x00, stCMDQIniCfg->u32CMDQ_Size[i]);

            CMDQ_MUTEX_LOCK();
            _Drv_CMDQ_Init(IP0PhyAddr);
            _Drv_CMDQ_FillInitIPInfo(i, IP0PhyAddr,
                u32IP0VirAddr, u32CMDQBufSize);
            _Drv_CMDQ_InitByIP(i);
            CMDQ_MUTEX_UNLOCK();
            SCL_DBG(SCL_DBG_LV_DRVCMDQ()&EN_DBGMG_CMDQEVEL_LOW, "[CMDQ]init %s: 0x%lx,0x%lx, 0x%lx\n"
                , __FUNCTION__, stCMDQIniCfg->u32CMDQ_Phy[i],stCMDQIniCfg->u32CMDQ_Vir[i], stCMDQIniCfg->u32CMDQ_Size[i]);
        }
    }

}
void Drv_CMDQ_Delete(EN_CMDQ_IP_TYPE enIPType)
{
    Hal_CMDQ_ClearIRQByFlag(enIPType,0x0800);   //clear idle
    while(1)
    {
        if(_IsCMDQIPIdle(enIPType))                // wait idle
        {
            break;
        }
    }
    Hal_CMDQ_SetISRMSK(enIPType,0xFFFF);
    Hal_CMDQ_ResetSoftInterrupt(enIPType);
    CMDQ_MUTEX_DELETE();
    if (_gCMDQHVSPMUTEX[enIPType] != -1)
    {
        MsOS_DeleteMutex(_gCMDQHVSPMUTEX[enIPType]);
    }
}
void Drv_CMDQ_release(EN_CMDQ_IP_TYPE enIPType)
{
    int u32Time;
    if(gstCMDQInfo[enIPType].bEnable == 1 || gstCMDQInfo[enIPType].bEnable == 0)
    {
        u32Time = MsOS_GetSystemTime();
        Drv_CMDQ_BeTrigger(enIPType,1);
        Drv_CMDQ_SetSkipPollWhenWaitTimeOut(enIPType,1);
        while(1)
        {
            if(Drv_CMDQ_CheckIPAlreadyDone(enIPType))
            {
                Drv_CMDQ_SetSkipPollWhenWaitTimeOut(enIPType,0);
                break;
            }
            else if(MsOS_Timer_DiffTimeFromNow(u32Time)>1000)
            {
                Drv_CMDQ_SetSkipPollWhenWaitTimeOut(enIPType,0);
                break;
            }
        }
        _Drv_CMDQ_Reset(enIPType);
        SCL_DBG(SCL_DBG_LV_DRVCMDQ()&EN_DBGMG_CMDQEVEL_LOW, "[CMDQ]%s enable:%hhd\n"
            ,__FUNCTION__,gstCMDQInfo[0].bEnable);
    }
    else
    {
        Drv_CMDQ_Enable(0,enIPType);
    }
}
void Drv_CMDQ_FillCmd(MS_CMDQ_CMDReg *stCfg,MS_U32 u32Addr, MS_U16 u16Data, MS_U16 u16Mask)
{
    stCfg->u16Data = u16Data;
    stCfg->u32Addr = u32Addr;
    stCfg->u16Mask = u16Mask;
}

//u32Addr :8bit addr
MS_BOOL Drv_CMDQ_WriteCmd(EN_CMDQ_IP_TYPE enIPType, MS_CMDQ_CMDReg *stCfg, MS_BOOL bSkipCheckSameAddr)
{
    MS_BOOL bRet=1;
    if(_gs32CMDQMutex == -1 || gstCMDQInfo[enIPType].bEnable==0)
    {
        return 0;
    }
    SCL_DBG(SCL_DBG_LV_DRVCMDQ()&EN_DBGMG_CMDQEVEL_NORMAL,"[CMDQ]%s\n",__FUNCTION__);
    CMDQ_MUTEX_LOCK();
    gstCurrworkIP = enIPType;
    SCL_DBG(SCL_DBG_LV_DRVCMDQ()&EN_DBGMG_CMDQEVEL_NORMAL,"[CMDQ]Write: %04lx %04x %04x flag: %hhx\n"
        ,stCfg->u32Addr, stCfg->u16Data, stCfg->u16Mask,gCMDQStatusFlag[enIPType]);
    #if USE_Utility
	Hal_CMDQ_WriteRegMaskBuffer(stCfg->u32Addr,stCfg->u16Data, stCfg->u16Mask);
    _Drv_CMDQ_PutCmd(CMDQ_CMDTYPE_WRITE,stCfg,NotToCheckSameAddr);
    #else
    _Drv_CMDQ_PutCmd(CMDQ_CMDTYPE_WRITE,stCfg,bSkipCheckSameAddr);
    #endif
    CMDQ_MUTEX_UNLOCK();
    return bRet;
}
MS_CMDQ_Info *Drv_CMDQ_GetCMDQInformation(EN_CMDQ_IP_TYPE enIPType)
{
    return &gstCMDQInfo[enIPType];
}
void Drv_CMDQ_GetModuleMutex(EN_CMDQ_IP_TYPE enIPType,MS_BOOL bEn)
{
    static MS_BOOL block = 0;
    if(bEn)
    {
        MsOS_ObtainMutex(_gCMDQHVSPMUTEX[enIPType],MSOS_WAIT_FOREVER);
        block = 1;
        SCL_DBG(SCL_DBG_LV_DRVCMDQ()&EN_DBGMG_CMDQEVEL_NORMAL,"[CMDQ]IN Mutex\n");
    }
    else if(block && !bEn)
    {
        block = 0;
        MsOS_ReleaseMutex(_gCMDQHVSPMUTEX[enIPType]);
        SCL_DBG(SCL_DBG_LV_DRVCMDQ()&EN_DBGMG_CMDQEVEL_NORMAL,"[CMDQ]Out Mutex\n");
    }
}
void Drv_CMDQ_InitRIUBase(MS_U32 u32RIUBase)
{
    Hal_CMDQ_InitRIUBase(u32RIUBase);
}
MS_U16 Drv_CMDQ_GetFinalIrq(EN_CMDQ_IP_TYPE enIPType)
{
    gu16ISRflag = Hal_CMDQ_GetFinalIrq(enIPType, 0xFFFF);
    return gu16ISRflag;
}
void Drv_CMDQ_ClearIrqByFlag(EN_CMDQ_IP_TYPE enIPType,MS_U16 u16Irq)
{
    Hal_CMDQ_ClearIRQByFlag(enIPType,u16Irq);
}
void Drv_CMDQ_SetISRStatus(MS_BOOL bEn)
{
    gbISRopen = bEn;
}
//-------------------------------------------------------------------------------------------------
/// Fire the commands
/// @return  TRUE if succeed, FALSE if failed
//-------------------------------------------------------------------------------------------------
MS_BOOL Drv_CMDQ_Fire(EN_CMDQ_IP_TYPE enIPType,MS_BOOL bStart)
{
    if(_gs32CMDQMutex == -1  || gstCMDQInfo[enIPType].bEnable==0)
    {
        SCL_DBGERR("[CMDQ]%s can't use\n",__FUNCTION__);
        return 0;
    }
    CMDQ_MUTEX_LOCK();
    gstCurrworkIP = enIPType;
#if (CMDQCMD128==0)
    if(gstCMDQInfo[enIPType].u16WPoint%2)
    {
        _Drv_CMDQ_AddNull(enIPType);
    }
#endif
    _Drv_CMDQ_Fire(enIPType,1);
    _SetCMDQFlagType(enIPType,CMDQ_FLAG_FIRE);
    CMDQ_MUTEX_UNLOCK();

    return true;

}
void Drv_CMDQ_SetEventForFire(void)
{
    MsOS_SetEvent_IRQ(Drv_SCLIRQ_Get_IRQ_SYNCEventID(),  E_SCLIRQ_EVENT_VIP);
}
void Drv_CMDQ_WaitCmd(EN_CMDQ_IP_TYPE enIPType,MS_U16 u16bus)
{
    MS_U64 u64Cmd;
    CMDQ_MUTEX_LOCK();
    gstCurrworkIP = enIPType;
    u64Cmd = MS_CMDQ_WAIT_CMD + u16bus;
    _Drv_CMDQ_WriteCmd((MS_U64)u64Cmd, NotToCheckSameAddr);
    CMDQ_MUTEX_UNLOCK();
}

void Drv_CMDQ_PollingEqCmd(EN_CMDQ_IP_TYPE enIPType,MS_CMDQ_CMDReg *stCfg)
{
    CMDQ_MUTEX_LOCK();
    gstCurrworkIP = enIPType;
    _Drv_CMDQ_PutCmd(CMDQ_CMDTYPE_POLLEQ,stCfg,NotToCheckSameAddr);
    CMDQ_MUTEX_UNLOCK();
}

void Drv_CMDQ_PollingNeqCmd(EN_CMDQ_IP_TYPE enIPType,MS_CMDQ_CMDReg *stCfg)
{
    CMDQ_MUTEX_LOCK();
    gstCurrworkIP = enIPType;
    _Drv_CMDQ_PutCmd(CMDQ_CMDTYPE_POLLNEQ,stCfg,NotToCheckSameAddr);
    CMDQ_MUTEX_UNLOCK();
}
void Drv_CMDQ_CheckVIPSRAM(MS_U32 u32Type)
{
    Drv_VIP_CheckVIPSRAM(u32Type);
}
MS_BOOL Drv_CMDQ_CheckIPAlreadyDone(EN_CMDQ_IP_TYPE enIPType)
{
    MS_BOOL bEn;
    if(_IsCMDQDMADone(enIPType)) //cmdq dma done
    {
        bEn = TRUE;
        Hal_CMDQ_ClearIRQByFlag(enIPType,0x0003);
    }
    else if(gbISRopen)
    {
        if(_IsCMDQExecuteDoneISR(enIPType))
        {
            bEn = TRUE;
        }
        else if(MsOS_GetEvent(Drv_SCLIRQ_Get_IRQ_SYNCEventID())&E_SCLIRQ_EVENT_CMDQDONE)
        {
            bEn = TRUE;
        }
        else
        {
            bEn = FALSE;
        }
    }
    else if(_IsCMDQExecuteDone(enIPType))
    {
        bEn = TRUE;
        Hal_CMDQ_ClearIRQByFlag(enIPType,0x0003);
    }
    else
    {
        bEn = FALSE;
    }
    return bEn;
}
