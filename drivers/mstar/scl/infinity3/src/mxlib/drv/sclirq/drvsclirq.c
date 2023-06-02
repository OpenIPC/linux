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
#define DRV_SCLIRQ_C

#ifdef MSOS_TYPE_LINUX_KERNEL
//#include <linux/wait.h>
//#include <linux/irqreturn.h>
//#include <asm/div64.h>
#endif

//-------------------------------------------------------------------------------------------------
//  Include Files
//-------------------------------------------------------------------------------------------------
#include "MsCommon.h"
#include "MsTypes.h"
#include "MsOS.h"
#include "irqs.h"
//#include <linux/kthread.h>
#include "hwreg.h"
#include "drvsclirq_st.h"
#include "drvsclirq.h"
#include "mdrv_scl_dbg.h"
#include "drvscldma_st.h"
#include "halscldma.h"
#include "drvscldma.h"
#include "regCMDQ.h"
#include "drvCMDQ.h"
#include "drvhvsp_st.h"
#include "drvhvsp.h"
#include "halsclirq.h"
//-------------------------------------------------------------------------------------------------
//  Defines & Macro
//-------------------------------------------------------------------------------------------------
#define DRV_SCLIRQ_DBG(x)
#define DRV_SCLIRQ_DBG_H(x)
#define DRV_SCLIRQ_ERR(x)      x
#define TASKLET_ALREADY 0x8000000000000000
#define IN_TASKLET_ALREADY(u64flag) (((MS_U64)u64flag & TASKLET_ALREADY)!= 0)
#define freerunID               5
#define ENABLE_ISR              0
#define ENABLE_ACTIVEID_ISR     1
#define ENABLE_CMDQ_ISR         1
#define CRITICAL_SECTION_TIME 80
#define CHANGE_CRITICAL_SECTION_TIMING_RANGE 3000 // (ns)
#define CHANGE_CRITICAL_SECTION_TIMING_COUNT 60 // (ns)
#define CRITICAL_SECTION_TIMING_MIN 500 // (ns)
#define _Is_SCLDMA_RingMode(enClientType)            (gstScldmaInfo.enBuffMode[(enClientType)] == E_SCLDMA_BUF_MD_RING)
#define _Is_SCLDMA_SingleMode(enClientType)         (gstScldmaInfo.enBuffMode[(enClientType)] == E_SCLDMA_BUF_MD_SINGLE)
#define _Is_SCLDMA_SWRingMode(enClientType)         (gstScldmaInfo.enBuffMode[(enClientType)] == E_SCLDMA_BUF_MD_SWRING)
#define _IsFrmIN_Vsync()                                  (gbFRMInFlag & (E_SCLIRQ_FRM_IN_COUNT_NOW ))
#define _IsFrmIN()                                  (gbFRMInFlag & (E_SCLIRQ_FRM_IN_COUNT_NOW << SCL_DELAYFRAME))
#define _IsFrmIN_Last()                                   (gbFRMInFlag & (E_SCLIRQ_FRM_IN_COUNT_NOW<< (SCL_DELAYFRAME+1) ))
#define _IsFrmDrop()                                  (gstDropFrameCount)
#if USE_RTK
#define DRV_SCLIRQ_MUTEX_LOCK_ISR()        MsOS_ObtainMutex(*_pSCLIRQ_SCLDMA_Mutex, MSOS_WAIT_FOREVER)
#define DRV_SCLIRQ_MUTEX_UNLOCK_ISR()        MsOS_ReleaseMutex(*_pSCLIRQ_SCLDMA_Mutex)
#else
#define DRV_SCLIRQ_MUTEX_LOCK_ISR()        MsOS_ObtainMutex_IRQ(*_pSCLIRQ_SCLDMA_Mutex)
#define DRV_SCLIRQ_MUTEX_UNLOCK_ISR()        MsOS_ReleaseMutex_IRQ(*_pSCLIRQ_SCLDMA_Mutex)
#endif
#define DRV_SCLIRQ_MUTEX_LOCK()             MsOS_ObtainMutex(_SCLIRQ_Mutex , MSOS_WAIT_FOREVER)
#define DRV_SCLIRQ_MUTEX_UNLOCK()             MsOS_ReleaseMutex(_SCLIRQ_Mutex)
#define INT_COUNT_TOLERANCE 13
//-------------------------------------------------------------------------------------------------
//  structure
//-------------------------------------------------------------------------------------------------
#if (ENABLE_ISR)
typedef struct
{
    MS_U64  u64IRQ;
    MS_U64  u64Flag;
    MS_U8   u8Count[E_SCLDMA_CLIENT_NUM];
}ST_SCLIRQ_IFNO_TYPE;
#endif

//-------------------------------------------------------------------------------------------------
//  Variable
//-------------------------------------------------------------------------------------------------
/////////////////
/// _SCLIRQ_Mutex
/// use in sclirq mutex,not include isr
////////////////
MS_S32 _SCLIRQ_Mutex = -1;

/////////////////
/// _pSCLIRQ_SCLDMA_Mutex
/// scldma and sclirq mutex, include isr, send form scldma
////////////////
MS_S32* _pSCLIRQ_SCLDMA_Mutex = NULL;

/////////////////
/// gsclirqstate
/// record irq state and set event as frame done
////////////////
SCLIRQTXEvent gsclirqstate;
MS_U32 gu32Time;
MS_U32 gu32FrmEndTime;

/////////////////
/// gscl3irqstate
/// record irq state and set event as frame done
////////////////
SCLIRQSC3Event gscl3irqstate;

/////////////////
/// _bSCLIRQ_Suspend
/// record Suspend
////////////////
MS_BOOL _bSCLIRQ_Suspend = 0;

/////////////////
/// gu32IRQNUM
/// record IRQ number to exit
////////////////
MS_U32 gu32IRQNUM;
MS_U32 gu32CMDQIRQNUM;
/////////////////
/// _s32FRMENDEventId
/// the frame done wait queue ID
////////////////
MS_S32  _s32FRMENDEventId;

/////////////////
/// _s32SYNCEventId
/// the fclk sync wait queue ID
////////////////
MS_S32  _s32SYNCEventId;

/////////////////
/// _s32SC3EventId
/// the Sc3 wait queue ID
////////////////
MS_S32  _s32SC3EventId;
MS_S32  _s32IRQEventId[EN_SCLIRQ_SCIRQ_MAX];

/////////////////
/// gbFRMInFlag
/// use in Drvhvsp and Drvsclirq, and pass to Drvscldma
/// if pre-crop (ISP) is receive full count ,ISR will set this TRUE;
////////////////
MS_BOOL gbFRMInFlag;

/////////////////
/// gbPtgenMode
/// use in Drvscldma,Drvhvsp and Drvsclirq
/// if True ,is debug ptgen mode. interrupt FRM_IN is inefficacy,so need to ignore.
////////////////
unsigned char gbPtgenMode;
unsigned char gbDMADoneEarlyISP;

/////////////////
/// gstThreadCfg
/// IST thread
////////////////
ST_SCLDMA_THREAD_CONFIG gstThreadCfg;
SCLIRQDAZAEvent genDAZAEvent;

/////////////////
/// gbMonitorCropMode
/// it use by device attribute to judge checking type
////////////////
unsigned char gbMonitorCropMode;
/////////////////
/// gbEachDMAEnable
/// record  frame status
////////////////
static MS_BOOL gbEachDMAEnable;
MS_BOOL gbBypassDNR;
volatile MS_BOOL gbInBlanking;
ST_SCLIRQ_SCINTS_TYPE gstSCInts;
MS_U8 gstDropFrameCount;
MS_BOOL gbsc2frame2reopen = 0;
#if (ENABLE_ISR)
ST_SCLIRQ_IFNO_TYPE gstSCLIrqInfo;
#endif
ST_SCLIRQ_DMAHANG_CNT gstDmaHang;

//-------------------------------------------------------------------------------------------------
//  Functions
//-------------------------------------------------------------------------------------------------
#if (ENABLE_CMDQ_ISR)
#if USE_RTK
irqreturn_t  _Drv_SCLIRQ_CMDQ_isr(int eIntNum, void* dev_id)
{
    MS_U32 u32IrqFlag;
    MS_U32 u32Time;
    u32IrqFlag = Drv_CMDQ_GetFinalIrq(EN_CMDQ_TYPE_IP0);
    Drv_CMDQ_ClearIrqByFlag(EN_CMDQ_TYPE_IP0,u32IrqFlag);
    u32Time = ((MS_U32)MsOS_GetSystemTimeStamp());
    if(u32IrqFlag & CMDQ_IRQ_STATE_TRIG)
    {
        MsOS_SetEventRing(EN_SCLIRQ_SCIRQ_CMDQ0TRIG);
    }
    if(u32IrqFlag & CMDQ_IRQ_STATE_DONE)
    {
        MsOS_SetEventRing(EN_SCLIRQ_SCIRQ_CMDQ0DONE);
    }
    MsOS_QueueWork(1,gstThreadCfg.s32Taskid[EN_SCLIRQ_SCTASKID_IRQ],
        gstThreadCfg.s32HandlerId[EN_SCLIRQ_SCTASK_WORKQUEUEIRQ],0);
    SCL_DBG(SCL_DBG_LV_DRVCMDQ()&EN_DBGMG_CMDQEVEL_ISR, "[CMDQ]u32IrqFlag:%lx  %lu\n",u32IrqFlag,u32Time);
    return IRQ_HANDLED;
}

#else
void _Drv_SCLIRQ_CMDQ_StateTrig(void)
{
    MsOS_SetEvent(_s32SYNCEventId, E_SCLIRQ_EVENT_CMDQGOING);
}
void _Drv_SCLIRQ_CMDQ_StateDone(void)
{
    MS_U32 u32Events;
    MS_BOOL bLDC = 0;
    u32Events = MsOS_GetEvent(_s32SYNCEventId);
    if(u32Events & E_SCLIRQ_EVENT_BRESETDNR)
    {
        SCL_DBGERR("[DRVSCLIRQ]%s:!!!E_SCLIRQ_DAZA_BRESETDNR@:%lu\n",__FUNCTION__,((MS_U32)MsOS_GetSystemTimeStamp()));
        Hal_SCLIRQ_Set_Reg(REG_SCL0_03_L, 0, BIT12); //LDC path close
        Hal_SCLIRQ_SetDNRBypass(1);
        gbBypassDNR = 1;
        MsOS_SetSCLFrameDelay(0);
        MsOS_ClearEvent(_s32SYNCEventId,(E_SCLIRQ_EVENT_BRESETDNR));
    }
    if(u32Events & E_SCLIRQ_EVENT_SETDELAY)
    {
        bLDC = ((Hal_SCLIRQ_Get_RegVal(REG_SCL0_03_L)&BIT12)? 1 : 0);
        SCL_DBG(SCL_DBG_LV_DRVCMDQ()&EN_DBGMG_CMDQEVEL_ISR,
            "[DRVSCLIRQ]%s:!!!E_SCLIRQ_EVENT_SETDELAY %hhd@:%lu\n",__FUNCTION__,bLDC,((MS_U32)MsOS_GetSystemTimeStamp()));
        MsOS_SetSCLFrameDelay(bLDC);
        MsOS_ClearEvent(_s32SYNCEventId,(E_SCLIRQ_EVENT_SETDELAY));
    }
    if(u32Events & E_SCLIRQ_EVENT_SETCHANGESIZE)
    {
        MsOS_SetEvent(_s32SYNCEventId, E_SCLIRQ_EVENT_CHANGESIZEDONE);
        MsOS_ClearEvent(_s32SYNCEventId,(E_SCLIRQ_EVENT_SETCHANGESIZE));
        SCL_DBG(SCL_DBG_LV_DRVCMDQ()&EN_DBGMG_CMDQEVEL_ISR,
            "[DRVSCLIRQ]%s:E_SCLIRQ_EVENT_SETCHANGESIZE Done@:%lu\n",__FUNCTION__,((MS_U32)MsOS_GetSystemTimeStamp()));
    }
    MsOS_SetEvent(_s32SYNCEventId, E_SCLIRQ_EVENT_CMDQDONE);
    MsOS_QueueWork(1,gstThreadCfg.s32Taskid[EN_SCLIRQ_SCTASKID_DAZA],
        gstThreadCfg.s32HandlerId[EN_SCLIRQ_SCTASK_WORKQUEUECMDQ],0);
    Drv_CMDQ_SetLPoint(EN_CMDQ_TYPE_IP0);
}

irqreturn_t  _Drv_SCLIRQ_CMDQ_isr(int eIntNum, void* dev_id)
{
    MS_U32 u32IrqFlag;
    MS_U32 u32Time;
    u32IrqFlag = Drv_CMDQ_GetFinalIrq(EN_CMDQ_TYPE_IP0);
    Drv_CMDQ_ClearIrqByFlag(EN_CMDQ_TYPE_IP0,u32IrqFlag);
    u32Time = ((MS_U32)MsOS_GetSystemTimeStamp());
    if(u32IrqFlag & CMDQ_IRQ_STATE_TRIG)
    {
        _Drv_SCLIRQ_CMDQ_StateTrig();
    }
    if(u32IrqFlag & CMDQ_IRQ_STATE_DONE)
    {
        _Drv_SCLIRQ_CMDQ_StateDone();
    }
    if(u32IrqFlag & (0x700))
    {
        Drv_CMDQ_SetSkipPollWhenWaitTimeOut(EN_CMDQ_TYPE_IP0,1);
        SCL_ERR("[CMDQ]POLL/Wait timeout\n");
        Drv_CMDQ_SetSkipPollWhenWaitTimeOut(EN_CMDQ_TYPE_IP0,0);
    }
    SCL_DBG(SCL_DBG_LV_DRVCMDQ()&EN_DBGMG_CMDQEVEL_ISR, "[CMDQ]u32IrqFlag:%lx  %lu\n",u32IrqFlag,u32Time);
    return IRQ_HANDLED;
}
#endif
#endif
void _Drv_SCLIRQ_SclFrameEndConnectToISP(void)
{
    scl_ve_isr();
}
void _Drv_SCLIRQ_SclFrameStartConnectToISP(void)
{
    scl_vs_isr();
}
#if ENABLE_ISR
irqreturn_t  _Drv_SCLIRQ_isr(int eIntNum, void* dev_id)
{
    MS_U64 u64Flag = Hal_SCLIRQ_Get_Flag(EN_SCLIRQ_SCTOP_0,0xFFFFFFFFFF);

    Hal_SCLIRQ_Set_Clear(EN_SCLIRQ_SCTOP_0,u64Flag, 1);

    Hal_SCLIRQ_Set_Clear(EN_SCLIRQ_SCTOP_0,u64Flag, 0);

    return IRQ_HANDLED;
}

#elif ENABLE_ACTIVEID_ISR
SCLIRQTXEvent _Drv_SCLIRQ_GetISTEventFlag(EN_SCLDMA_CLIENT_TYPE enClient)
{
    switch(enClient)
    {
        case E_SCLDMA_1_FRM_W:
            return E_SCLIRQ_EVENT_ISTSC1FRM;
        case E_SCLDMA_1_SNP_W:
            return E_SCLIRQ_EVENT_ISTSC1SNP;
        case E_SCLDMA_2_FRM_W:
            return E_SCLIRQ_EVENT_ISTSC2FRM;
        case E_SCLDMA_2_FRM2_W:
            return E_SCLIRQ_EVENT_ISTSC2FRM2;
        default :
            return E_SCLIRQ_EVENT_CLEAR;
    }
}
void _Drv_SCLIRQ_SetIsBlankingRegion(MS_BOOL bBk)
{
    gbInBlanking = bBk;
}
MS_BOOL Drv_SCLIRQ_GetIsVIPBlankingRegion(void)
{
    MS_U32 u32Time;
    u32Time = ((MS_U32)MsOS_GetSystemTimeStamp());
    if(gbInBlanking && gu32FrmEndTime && gstSCInts.u32ISPDoneCount&&
        ((u32Time - gu32FrmEndTime)>((gstSCInts.u32ISPBlankingTime))))
    {
        if((u32Time - gu32FrmEndTime)>((gstSCInts.u32ISPBlankingTime)*2))
        {
            // error
            return gbInBlanking;
        }
        return 0;
    }
    else if(MsOS_GetEvent(_s32SYNCEventId) & E_SCLIRQ_EVENT_CMDQDONE)
    {
        return gbInBlanking;
    }
    else
    {
        return gbInBlanking;
    }
}
MS_BOOL Drv_SCLIRQ_GetIsBlankingRegion(void)
{
    MS_BOOL bRet = gbInBlanking;
    MS_U32 u32Time;
    u32Time = ((MS_U32)MsOS_GetSystemTimeStamp());
    //critical section
    if(gbInBlanking && gu32FrmEndTime && gstSCInts.u32ISPDoneCount&&
        ((u32Time - gu32FrmEndTime)>((gstSCInts.u32ISPBlankingTime)-CRITICAL_SECTION_TIME)))
    {
        bRet = 0;
        if((u32Time - gu32FrmEndTime)>((gstSCInts.u32ISPBlankingTime)*2))
        {
            // error
            bRet = gbInBlanking;
        }
        else
        {
            SCL_DBG(SCL_DBG_LV_DRVSCLIRQ()==(EN_DBGMG_SCLIRQLEVEL_ELSE),
                "[SCLIRQ]Blanking Critical Section (%ld ,%ld)\n",
                (u32Time - gu32FrmEndTime),((gstSCInts.u32ISPBlankingTime)-CRITICAL_SECTION_TIME));
        }
    }
    else
    {
        bRet = gbInBlanking;
    }
    return bRet;
}
void _Drv_SCLIRQ_SetSingleModeDMAInfo(EN_SCLDMA_CLIENT_TYPE enclient, MS_U32 u32Events)
{
    if ((u32Events & (SCLIRQ_ISTEVENT_BASE << enclient)) && _Is_SCLDMA_SingleMode(enclient))
    {
        DRV_SCLIRQ_MUTEX_LOCK_ISR();
        gstScldmaInfo.bDMAOnOff[enclient] = 0;
        Drv_SCLDMA_SetFrameResolution(enclient);
        DRV_SCLIRQ_MUTEX_UNLOCK_ISR();
    }
}
void _Drv_SCLIRQ_SetSingleModeDMAInfoNoLock(EN_SCLDMA_CLIENT_TYPE enclient, MS_U32 u32Events)
{
    if ((u32Events & (SCLIRQ_ISTEVENT_BASE << enclient)) && _Is_SCLDMA_SingleMode(enclient))
    {
        gstScldmaInfo.bDMAOnOff[enclient] = 0;
        Drv_SCLDMA_SetFrameResolution(enclient);
    }
}
static void Drv_SCLIRQ_RingMode_Active(EN_SCLDMA_CLIENT_TYPE enClient)
{
    MS_U8 u8RPoint = 0;
    MS_U32 u32Time;
    MS_U16  u16RealIdx;
    u32Time = ((MS_U32)MsOS_GetSystemTimeStamp());
    _SetANDGetFlagType(enClient,E_SCLDMA_FLAG_ACTIVE,(~E_SCLDMA_FLAG_FRMDONE));
    u8RPoint = (_GetIdxType(enClient,E_SCLDMA_ACTIVE_BUFFER_OMX))>>4;
    if(!_IsFlagType(enClient,E_SCLDMA_FLAG_DMAOFF))
    {
        u16RealIdx = Hal_SCLDMA_Get_RW_Idx(enClient);
    }
    else
    {
        u16RealIdx = _GetIdxType(enClient,E_SCLDMA_ACTIVE_BUFFER_SCL);
    }
    Drv_SCLDMA_SetFrameResolution(enClient);
    if(u8RPoint==u16RealIdx && !_IsFlagType(enClient,E_SCLDMA_FLAG_BLANKING))//5 is debug freerun
    {
        //trig off
        if(u8RPoint != freerunID )
        {
            Drv_SCLDMA_SetISRHandlerDMAOff(enClient,0);
            _SetFlagType(enClient,E_SCLDMA_FLAG_EVERDMAON);
            _SetANDGetIdxType(enClient,(u16RealIdx),(E_SCLDMA_ACTIVE_BUFFER_OMX));
            _SetFlagType(enClient,(E_SCLDMA_FLAG_DMAOFF|E_SCLDMA_FLAG_ACTIVE));
        }
        else
        {
            _SetANDGetIdxType(enClient,(u16RealIdx),(E_SCLDMA_ACTIVE_BUFFER_OMX));
            _SetANDGetFlagType(enClient,E_SCLDMA_FLAG_ACTIVE,(~E_SCLDMA_FLAG_DMAOFF));
        }
    }
    else
    {
        _SetANDGetIdxType(enClient,u16RealIdx,E_SCLDMA_ACTIVE_BUFFER_OMX);
        _SetFlagType(enClient,E_SCLDMA_FLAG_ACTIVE);
    }
    SCL_DBG(SCL_DBG_LV_DRVSCLIRQ()&Get_DBGMG_SCLIRQclient(enClient,0),
        "[DRVSCLDMA]%d ISR R_P=%hhx RealIdx=%hx flag:%hx @:%lu\n"
    ,enClient,u8RPoint,u16RealIdx,gstScldmaInfo.bDmaflag[enClient],u32Time);
}
static void Drv_SCLIRQ_RingMode_Active_N(EN_SCLDMA_CLIENT_TYPE enclient)
{
    _SetANDGetFlagType(enclient,E_SCLDMA_FLAG_FRMDONE,(~E_SCLDMA_FLAG_ACTIVE));
    _ReSetFlagType(enclient,E_SCLDMA_FLAG_ACTIVE);
    if(_IsFlagType(enclient,E_SCLDMA_FLAG_DMAOFF))
    {
        gstScldmaInfo.bDMAOnOff[enclient] = 0;
    }
    if(_IsFlagType(enclient,E_SCLDMA_FLAG_BLANKING))
    {
        _ReSetFlagType(enclient,E_SCLDMA_FLAG_BLANKING);
    }
    SCL_DBG(SCL_DBG_LV_DRVSCLIRQ()&Get_DBGMG_SCLIRQclient(enclient,1), "[DRVSCLDMA]%d ISR actN flag:%hx idx:%hhd @:%lu\n"
    ,enclient,gstScldmaInfo.bDmaflag[enclient],gstScldmaInfo.bDMAidx[enclient],((MS_U32)MsOS_GetSystemTimeStamp()));
    //printf("ACTIVE_N:%lu\n",u32Time);
}
static void Drv_SCLIRQ_RingMode_ActiveWithoutDoublebuffer(EN_SCLDMA_CLIENT_TYPE enClient)
{
    MS_U8 u8RPoint = 0;
    MS_U16  u16RealIdx;
    if(_IsFlagType(enClient,E_SCLDMA_FLAG_ACTIVE))
    {
        _SetFlagType(enClient,E_SCLDMA_FLAG_FRMIGNORE);
        SCL_DBGERR("[DRVSCLIRQ]%d!!!!!!!!!!!!double active\n",enClient);
    }
    else
    {
        _SetANDGetFlagType(enClient,E_SCLDMA_FLAG_ACTIVE,(~E_SCLDMA_FLAG_FRMDONE));
    }
    u8RPoint = (_GetIdxType(enClient,E_SCLDMA_ACTIVE_BUFFER_OMX))>>4;
    if(!_IsFlagType(enClient,E_SCLDMA_FLAG_DMAOFF))
    {
        u16RealIdx = Hal_SCLDMA_Get_RW_Idx(enClient);
        if(u16RealIdx == 0 && _GetIdxType(enClient,E_SCLDMA_ACTIVE_BUFFER_SCL) != gstScldmaInfo.bMaxid[enClient]&&
            !(_IsFlagType(enClient,E_SCLDMA_FLAG_BLANKING)))
        {
            _SetFlagType(enClient,E_SCLDMA_FLAG_FRMIGNORE);
            SCL_DBG(SCL_DBG_LV_DRVSCLIRQ()&Get_DBGMG_SCLIRQclient(enClient,0),
                "[DRVSCLDMA]%d wodb ISR ignore\n"
            ,enClient);
        }
    }
    else
    {
        u16RealIdx = _GetIdxType(enClient,E_SCLDMA_ACTIVE_BUFFER_SCL);
    }
    Drv_SCLDMA_SetFrameResolution(enClient);
    _SetANDGetIdxType(enClient,u16RealIdx,E_SCLDMA_ACTIVE_BUFFER_OMX);
    SCL_DBG(SCL_DBG_LV_DRVSCLIRQ()&Get_DBGMG_SCLIRQclient(enClient,0),
        "[DRVSCLDMA]%d wodb ISR R_P=%hhx RealIdx=%hx flag:%hx @:%lu\n"
    ,enClient,u8RPoint,u16RealIdx,gstScldmaInfo.bDmaflag[enClient],gu32Time);
}
static MS_BOOL Drv_SCLIRQ_RingMode_Active_NWithoutDoublebuffer(EN_SCLDMA_CLIENT_TYPE enClient)
{
    MS_U8 u8RPoint = 0;
    MS_U16  u16RealIdx;
    if((!_IsFlagType(enClient,E_SCLDMA_FLAG_FRMIN)))
    {
        _SetFlagType(enClient,E_SCLDMA_FLAG_FRMIGNORE);
    }
    if(!_IsFlagType(enClient,E_SCLDMA_FLAG_FRMIGNORE))
    {
        u8RPoint = (_GetIdxType(enClient,E_SCLDMA_ACTIVE_BUFFER_OMX))>>4;
        u16RealIdx = _GetIdxType(enClient,E_SCLDMA_ACTIVE_BUFFER_SCL);
        if(u8RPoint==u16RealIdx && !_IsFlagType(enClient,E_SCLDMA_FLAG_BLANKING))//5 is debug freerun
        {
            //trig off
            if(u8RPoint != freerunID )
            {
                Drv_SCLDMA_SetSWReTrigCount(enClient,1);
                Drv_SCLDMA_SetISRHandlerDMAOff(enClient,0);
                _SetANDGetFlagType
                    (enClient,E_SCLDMA_FLAG_FRMDONE|E_SCLDMA_FLAG_EVERDMAON|E_SCLDMA_FLAG_DMAOFF,(~E_SCLDMA_FLAG_ACTIVE));
                _SetANDGetIdxType(enClient,(u16RealIdx),(E_SCLDMA_ACTIVE_BUFFER_OMX));
            }
            else
            {
                _SetANDGetIdxType(enClient,(u16RealIdx),(E_SCLDMA_ACTIVE_BUFFER_OMX));
                _SetANDGetFlagType(enClient,E_SCLDMA_FLAG_FRMDONE,(~(E_SCLDMA_FLAG_DMAOFF|E_SCLDMA_FLAG_ACTIVE)));
            }
        }
        else
        {
            _SetANDGetFlagType(enClient,E_SCLDMA_FLAG_FRMDONE,(~E_SCLDMA_FLAG_ACTIVE));
        }
        if(_IsFlagType(enClient,E_SCLDMA_FLAG_DMAOFF))
        {
            gstScldmaInfo.bDMAOnOff[enClient] = 0;
        }
        if(_IsFlagType(enClient,E_SCLDMA_FLAG_BLANKING))
        {
            _ReSetFlagType(enClient,E_SCLDMA_FLAG_BLANKING);
        }
        SCL_DBG(SCL_DBG_LV_DRVSCLIRQ()&Get_DBGMG_SCLIRQclient(enClient,1), "[DRVSCLDMA]%d ISR actN wodb flag:%hx idx:%hhx @:%lu\n"
        ,enClient,gstScldmaInfo.bDmaflag[enClient],gstScldmaInfo.bDMAidx[enClient],((MS_U32)MsOS_GetSystemTimeStamp()));
        return 1;
    }
    else
    {
        _ReSetFlagType(enClient,E_SCLDMA_FLAG_FRMIGNORE);
        if(u8RPoint != freerunID )
        {
            Drv_SCLDMA_SetDMAIgnoreCount(enClient,1);
            Drv_SCLDMA_SetISRHandlerDMAOff(enClient,0);
            SCL_DBG(SCL_DBG_LV_DRVSCLIRQ()&Get_DBGMG_SCLIRQclient(enClient,1),
                "[DRVSCLDMA]%d ISR actN wodb ignore flag:%hx\n"
            ,enClient,gstScldmaInfo.bDmaflag[(enClient)]);
            _SetANDGetFlagType
                (enClient,E_SCLDMA_FLAG_FRMDONE|E_SCLDMA_FLAG_EVERDMAON|E_SCLDMA_FLAG_DMAOFF,(~(E_SCLDMA_FLAG_ACTIVE)));
        }
        if(_IsFlagType(enClient,E_SCLDMA_FLAG_DMAOFF))
        {
            gstScldmaInfo.bDMAOnOff[enClient] = 0;
        }
        _ReSetFlagType(enClient,E_SCLDMA_FLAG_FRMIN);
        MsOS_SetEvent(_s32FRMENDEventId, (0x1 <<enClient));
        SCL_DBG(SCL_DBG_LV_DRVSCLIRQ()&Get_DBGMG_SCLIRQclient(enClient,1),
            "[DRVSCLDMA]%d ISR actN wodb ignore DMA off:%hhx flag:%hx@:%lu\n"
        ,enClient,gstScldmaInfo.bDMAOnOff[enClient],gstScldmaInfo.bDmaflag[(enClient)],((MS_U32)MsOS_GetSystemTimeStamp()));
        return 0;
    }
    //printf("ACTIVE_N:%lu\n",u32Time);
}
static void Drv_SCLIRQ_SWRingMode_Active(EN_SCLDMA_CLIENT_TYPE enclient)
{
    MS_U32 u32Time;
    MS_U16  u16RealIdx;
    u32Time = ((MS_U32)MsOS_GetSystemTimeStamp());
    if(_IsFlagType(enclient,E_SCLDMA_FLAG_BLANKING))
    {
        u16RealIdx = 0;
    }
    else
    {
        u16RealIdx = Drv_SCLDMA_GetActiveBufferIdx(enclient);
    }
    if(_IsFlagType(enclient,E_SCLDMA_FLAG_ACTIVE))
    {
        _SetFlagType(enclient,E_SCLDMA_FLAG_FRMIGNORE);
        SCL_DBGERR("[DRVSCLIRQ]!!!!!!!!!!!!double active\n");
    }
    else
    {
        _SetANDGetFlagType(enclient,E_SCLDMA_FLAG_ACTIVE,(~(E_SCLDMA_FLAG_FRMDONE|E_SCLDMA_FLAG_FRMIGNORE)));
    }
    _SetANDGetIdxType(enclient,u16RealIdx,E_SCLDMA_ACTIVE_BUFFER_OMX);
    SCL_DBG(SCL_DBG_LV_DRVSCLIRQ()&Get_DBGMG_SCLIRQclient(enclient,0), "[DRVSCLDMA]%d ISR SWRING flag:%hx ACT:%hhd@:%lu\n"
    ,enclient,gstScldmaInfo.bDmaflag[enclient],u16RealIdx,u32Time);
}
static MS_BOOL Drv_SCLIRQ_SWRingMode_Active_N(EN_SCLDMA_CLIENT_TYPE enclient)
{
    ST_SCLDMA_FRAME_BUFFER_CONFIG stTarget;
    MS_BOOL Ret = 1;
    MsOS_Memset(&stTarget,0,sizeof(ST_SCLDMA_FRAME_BUFFER_CONFIG));
    if(!_IsFlagType(enclient,E_SCLDMA_FLAG_FRMIN)|| _IsFlagType(enclient,E_SCLDMA_FLAG_DROP)
        || _IsFlagType(enclient,E_SCLDMA_FLAG_FRMIGNORE))
    {
        Ret = 0;
        _ReSetFlagType(enclient,E_SCLDMA_FLAG_ACTIVE);
        if( _IsFlagType(enclient,E_SCLDMA_FLAG_FRMIGNORE))
        {
            Drv_SCLDMA_SetDMAIgnoreCount(enclient,1);
            Drv_SCLDMA_SetISRHandlerDMAOff(enclient,0);
            _SetFlagType(enclient,E_SCLDMA_FLAG_EVERDMAON);
        }
        else
        {
            _SetFlagType(enclient,E_SCLDMA_FLAG_FRMIGNORE);
        }
        SCL_DBG(SCL_DBG_LV_DRVSCLIRQ()&Get_DBGMG_SCLIRQclient(enclient,1),
            "[DRVSCLDMA]%d ISR SWRING ignore flag:%hx idx:%hhd\n"
        ,enclient,gstScldmaInfo.bDmaflag[enclient],gstScldmaInfo.bDMAidx[enclient]);
    }
    else
    {
        _SetANDGetFlagType(enclient,E_SCLDMA_FLAG_FRMDONE,(~E_SCLDMA_FLAG_ACTIVE));
        _ReSetFlagType(enclient,E_SCLDMA_FLAG_ACTIVE);
        stTarget.u8FrameAddrIdx = _GetIdxType(enclient,E_SCLDMA_ACTIVE_BUFFER_SCL);
        stTarget.u32FrameAddr   = gstScldmaInfo.u32Base_Y[enclient][stTarget.u8FrameAddrIdx];
        stTarget.u64FRMDoneTime = Drv_SCLDMA_GetSclFrameDoneTime(enclient,0);
        stTarget.u16FrameWidth   = gstScldmaInfo.u16FrameWidth[enclient];
        stTarget.u16FrameHeight  = gstScldmaInfo.u16FrameHeight[enclient];
        if(Drv_SCLDMA_MakeSureNextActiveId(enclient))
        {
            if(Drv_SCLDMA_BufferEnQueue(enclient,&stTarget))
            {
                Drv_SCLDMA_ChangeBufferIdx(enclient);
            }
        }
        if(_IsFlagType(enclient,E_SCLDMA_FLAG_BLANKING))
        {
            _ReSetFlagType(enclient,E_SCLDMA_FLAG_BLANKING);
        }
        SCL_DBG(SCL_DBG_LV_DRVSCLIRQ()&Get_DBGMG_SCLIRQclient(enclient,1), "[DRVSCLDMA]%d ISR SWRING actN flag:%hx idx:%hhd\n"
        ,enclient,gstScldmaInfo.bDmaflag[enclient],gstScldmaInfo.bDMAidx[enclient]);
    }
    return Ret;
}
static void Drv_SCLIRQ_SingleMode_Active(EN_SCLDMA_CLIENT_TYPE enclient)
{
    MS_U32 u32Time;
    _SetANDGetFlagType(enclient,E_SCLDMA_FLAG_ACTIVE,(~E_SCLDMA_FLAG_FRMDONE));
    if(enclient< E_SCLDMA_3_FRM_R)
    {
        Drv_SCLDMA_SetISRHandlerDMAOff(enclient,0);
        u32Time = ((MS_U32)MsOS_GetSystemTimeStamp());
        SCL_DBG(SCL_DBG_LV_DRVSCLIRQ()&Get_DBGMG_SCLIRQclient(enclient,1), "[DRVSCLIRQ]single act:%lu\n",u32Time);
    }
}
static void Drv_SCLIRQ_SingleMode_Active_N(EN_SCLDMA_CLIENT_TYPE enclient)
{
    MS_U32 u32Time;
    _SetANDGetFlagType(enclient,E_SCLDMA_FLAG_FRMDONE,(~E_SCLDMA_FLAG_ACTIVE));
    if(enclient< E_SCLDMA_3_FRM_R)
    {
        u32Time = ((MS_U32)MsOS_GetSystemTimeStamp());
        _Drv_SCLIRQ_SetSingleModeDMAInfoNoLock(enclient,(SCLIRQ_ISTEVENT_BASE << enclient));
        SCL_DBG(SCL_DBG_LV_DRVSCLIRQ()&Get_DBGMG_SCLIRQclient(enclient,1), "[DRVSCLIRQ]single actN:%lu\n",u32Time);
    }
}
static void Drv_SCLIRQ_SingleMode_ActiveWithoutDoublebuffer(EN_SCLDMA_CLIENT_TYPE enclient)
{
    MS_U32 u32Time;
    if(_IsFlagType(enclient,E_SCLDMA_FLAG_ACTIVE))
    {
        _SetFlagType(enclient,E_SCLDMA_FLAG_FRMIGNORE);
        SCL_DBGERR("[DRVSCLIRQ]!!!!!!! double active\n");
    }
    else
    {
        _SetANDGetFlagType(enclient,E_SCLDMA_FLAG_ACTIVE,(~(E_SCLDMA_FLAG_FRMDONE|E_SCLDMA_FLAG_FRMIGNORE)));
    }
    u32Time = ((MS_U32)MsOS_GetSystemTimeStamp());
    SCL_DBG(SCL_DBG_LV_DRVSCLIRQ()&EN_DBGMG_SCLIRQLEVEL_SC1SINGLE, "[DRVSCLIRQ]singlewodb act:%lu\n",u32Time);
}
static MS_BOOL Drv_SCLIRQ_SingleMode_Active_NWithoutDoublebuffer(EN_SCLDMA_CLIENT_TYPE enclient)
{
    MS_U32 u32Time;
    MS_BOOL Ret = 1;
    u32Time = ((MS_U32)MsOS_GetSystemTimeStamp());
    _SetANDGetFlagType(enclient,E_SCLDMA_FLAG_FRMDONE,(~E_SCLDMA_FLAG_ACTIVE));
    if(enclient< E_SCLDMA_3_FRM_R)
    {
        if(!_IsFlagType(enclient,E_SCLDMA_FLAG_FRMIN) || _IsFlagType(enclient,E_SCLDMA_FLAG_DROP)
            || _IsFlagType(enclient,E_SCLDMA_FLAG_FRMIGNORE))
        {
            Ret = 0;
            if( _IsFlagType(enclient,E_SCLDMA_FLAG_FRMIGNORE))
            {
                Drv_SCLDMA_SetDMAIgnoreCount(enclient,1);
                Drv_SCLDMA_SetISRHandlerDMAOff(enclient,0);
                _SetFlagType(enclient,E_SCLDMA_FLAG_EVERDMAON);
            }
            else
            {
                _SetFlagType(enclient,E_SCLDMA_FLAG_FRMIGNORE);
            }
            SCL_DBG(SCL_DBG_LV_DRVSCLIRQ()&EN_DBGMG_SCLIRQLEVEL_SC1SINGLE, "[DRVSCLIRQ]singlewodb ignore actN:%lu\n",u32Time);
        }
        else
        {
            _SetFlagType(enclient,E_SCLDMA_FLAG_DMAOFF);
            //Drv_SCLDMA_SetISRHandlerDMAOff(enclient,0);
            _Drv_SCLIRQ_SetSingleModeDMAInfoNoLock(enclient,(SCLIRQ_ISTEVENT_BASE << enclient));
            SCL_DBG(SCL_DBG_LV_DRVSCLIRQ()&EN_DBGMG_SCLIRQLEVEL_SC1SINGLE, "[DRVSCLIRQ]singlewodb actN:%lu\n",u32Time);
        }
    }
    return Ret;
}
static void Drv_SCLIRQ_SC3SingleMode_Active(void)
{
    MS_U32 u32Time;
    u32Time = ((MS_U32)MsOS_GetSystemTimeStamp());
    SCL_DBG(SCL_DBG_LV_DRVSCLIRQ()&EN_DBGMG_SCLIRQLEVEL_SC3SINGLE, "[DRVSCLIRQ]SC3 single act @:%lu\n",u32Time);
    _SetANDGetFlagType(E_SCLDMA_3_FRM_W,E_SCLDMA_FLAG_ACTIVE,(~E_SCLDMA_FLAG_FRMDONE));
    _SetANDGetFlagType(E_SCLDMA_3_FRM_R,E_SCLDMA_FLAG_ACTIVE,(~E_SCLDMA_FLAG_FRMDONE));
}
static void Drv_SCLIRQ_SC3SingleMode_Active_N(void)
{
    MS_U32 u32Time;
    MS_U32 u32Events = 0;
    u32Time = ((MS_U32)MsOS_GetSystemTimeStamp());
    Drv_SCLDMA_SetISRHandlerDMAOff(E_SCLDMA_3_FRM_W,0);
    u32Events = MsOS_GetEvent(_s32SC3EventId);//get now ir
    _SetANDGetFlagType(E_SCLDMA_3_FRM_W,(E_SCLDMA_FLAG_DMAOFF|E_SCLDMA_FLAG_FRMDONE),~(E_SCLDMA_FLAG_EVERDMAON|E_SCLDMA_FLAG_ACTIVE));
    _SetANDGetFlagType(E_SCLDMA_3_FRM_R,(E_SCLDMA_FLAG_DMAOFF|E_SCLDMA_FLAG_FRMDONE),~(E_SCLDMA_FLAG_EVERDMAON|E_SCLDMA_FLAG_ACTIVE));
    _Drv_SCLIRQ_SetSingleModeDMAInfoNoLock(E_SCLDMA_3_FRM_W,(SCLIRQ_ISTEVENT_BASE << E_SCLDMA_3_FRM_W));
    _Drv_SCLIRQ_SetSingleModeDMAInfoNoLock(E_SCLDMA_3_FRM_R,(SCLIRQ_ISTEVENT_BASE << E_SCLDMA_3_FRM_R));
    SCL_DBG(SCL_DBG_LV_DRVSCLIRQ()&EN_DBGMG_SCLIRQLEVEL_SC3SINGLE, "[DRVSCLIRQ]SC3 single actN @:%lu\n",u32Time);
    MsOS_ClearEvent(_s32SC3EventId,u32Events &(E_SCLIRQ_SC3EVENT_ISTCLEAR));
    MsOS_SetEvent(_s32SC3EventId, (E_SCLIRQ_SC3EVENT_DONE |E_SCLIRQ_SC3EVENT_HVSPST));
}
static void _Drv_SCLIRQ_SetFrameInFlag(EN_SCLDMA_CLIENT_TYPE enclient)
{
    if(_IsFrmDrop())
    {
        _SetFlagType(enclient,E_SCLDMA_FLAG_DROP);
        SCL_DBG(SCL_DBG_LV_DRVSCLIRQ()&EN_DBGMG_SCLIRQLEVEL_ELSE,
            "[DRVSCLIRQ]%dIsFrmDrop @:%lu\n",enclient,((MS_U32)MsOS_GetSystemTimeStamp()));
    }
    else
    {
        _ReSetFlagType(enclient,E_SCLDMA_FLAG_DROP);
    }
    if(_IsFrmIN())
    {
        _SetFlagType(enclient,E_SCLDMA_FLAG_FRMIN);
    }
    else if(!MsOS_GetSCLFrameDelay())
    {
        MS_U16 u16Crop;
        MS_U16 u16Crop2;
        u16Crop = Hal_SCLIRQ_Get_RegVal(REG_SCL2_50_L);
        u16Crop &= 0x1;
        u16Crop2 = Hal_SCLIRQ_Get_RegVal(REG_SCL2_58_L);
        u16Crop2 &= 0x1;
        if((u16Crop || u16Crop2)&&_IsFrmIN_Last())
        {
            _SetFlagType(enclient,E_SCLDMA_FLAG_FRMIN);
        }
        else
        {
            _ReSetFlagType(enclient,E_SCLDMA_FLAG_FRMIN);
        }
    }
    else
    {
        _ReSetFlagType(enclient,E_SCLDMA_FLAG_FRMIN);
    }
}
void _Drv_SCLIRQ_MonitorSystemTimer(void)
{
    MS_U64 u64Time;
    static MS_U64 u64lasttime = 0;
    u64Time = ((MS_U64)Chip_Get_US_Ticks());
    if(u64Time > u64lasttime)
    {
        u64lasttime = u64Time;
    }
    else
    {
        printf("[DRVSCLIRQ]!!!!!!!! system time reset last:%lld this:%lld\n",u64lasttime,u64Time);
        u64lasttime = u64Time;
    }
}
void _Drv_SCLIRQ_FrmDoneEventHandler(void)
{
    SCLIRQTXEvent enEvent = gsclirqstate;
    if(enEvent)
    {
        enEvent |= (E_SCLIRQ_EVENT_IRQ|E_SCLIRQ_EVENT_HVSPST);
        MsOS_SetEvent(_s32FRMENDEventId, enEvent);
        if(gsclirqstate &E_SCLIRQ_EVENT_SC1FRM && (gbMonitorCropMode == 2))
        {
            MsOS_SetEvent(_s32SYNCEventId, E_SCLIRQ_EVENT_SYNC);
        }
        gsclirqstate &= (~enEvent);
        gsclirqstate |= E_SCLIRQ_EVENT_SET;
        SCL_DBG(SCL_DBG_LV_DRVSCLIRQ()&(EN_DBGMG_SCLIRQLEVEL_ELSE),
            "%s @:%lu\n",__FUNCTION__,gu32Time);
    }
}
void _Drv_SCLIRQ_CMDQBlankingHandler(void)
{
    if(MsOS_GetEvent(_s32SYNCEventId)&E_SCLIRQ_EVENT_CMDQFIRE )
    {
        Drv_CMDQ_BeTrigger(EN_CMDQ_TYPE_IP0,1);
        MsOS_ClearEvent(Drv_SCLIRQ_Get_IRQ_SYNCEventID(),(E_SCLIRQ_EVENT_CMDQFIRE));
        SCL_DBG(SCL_DBG_LV_DRVCMDQ()&(EN_DBGMG_CMDQEVEL_ISR),
            "%s @:%lu\n",__FUNCTION__,gu32Time);
    }
}
void _Drv_SCLIRQ_SetFRMINWhenDMADoneEarlyISP(MS_BOOL bEn)
{
    if(bEn)
    {
        _SetFlagType(E_SCLDMA_1_FRM_W,E_SCLDMA_FLAG_FRMIN);
        _SetFlagType(E_SCLDMA_1_SNP_W,E_SCLDMA_FLAG_FRMIN);
        _SetFlagType(E_SCLDMA_2_FRM_W,E_SCLDMA_FLAG_FRMIN);
        _SetFlagType(E_SCLDMA_2_FRM2_W,E_SCLDMA_FLAG_FRMIN);
    }
    else
    {
        _ReSetFlagType(E_SCLDMA_1_FRM_W,E_SCLDMA_FLAG_FRMIN);
        _ReSetFlagType(E_SCLDMA_1_SNP_W,E_SCLDMA_FLAG_FRMIN);
        _ReSetFlagType(E_SCLDMA_2_FRM_W,E_SCLDMA_FLAG_FRMIN);
        _ReSetFlagType(E_SCLDMA_2_FRM2_W,E_SCLDMA_FLAG_FRMIN);
    }
}
void _Drv_SCLIRQ_FrmEndhandler(void)
{
    unsigned char idx;
    unsigned char loop = 0;
    if(!gbEachDMAEnable)
    {
        gbEachDMAEnable = 1;
    }
    else if(gsclirqstate)
    {
    }
    else
    {
        for(idx = 0;idx<E_SCLDMA_CLIENT_NUM;idx++)
        {
            if(gstScldmaInfo.bDMAOnOff[idx])
            {
                loop = 1;
                break;
            }
        }
        if(!loop)
        {
            gbEachDMAEnable = 0;
        }
    }
    MsOS_SetEvent(_s32SYNCEventId, E_SCLIRQ_EVENT_FRMENDSYNC);
    MsOS_ClearEvent(Drv_SCLIRQ_Get_IRQ_SYNCEventID(),(E_SCLIRQ_EVENT_LDCSYNC));
    //TODO:not open this time
    if((!gbDMADoneEarlyISP)||(SCL_DELAYFRAME) || (gbPtgenMode))
    {
        _Drv_SCLIRQ_FrmDoneEventHandler();
    }
}
void _Drv_SCLIRQ_SetDMAOnOffWithoutDoubleBuffer(EN_SCLDMA_CLIENT_TYPE enclient)
{
    if((_IsFlagType(enclient,E_SCLDMA_FLAG_DMAOFF))&&_Is_SCLDMA_SingleMode(enclient))
    {
        _ReSetFlagType(enclient,E_SCLDMA_FLAG_DMAOFF);

        Drv_SCLDMA_SetISRHandlerDMAOff(enclient,0);
        gstScldmaInfo.bDMAOnOff[enclient] = 0;
    }
    else if((_IsFlagType(enclient,E_SCLDMA_FLAG_EVERDMAON))&&
        (_Is_SCLDMA_SingleMode(enclient)||_Is_SCLDMA_SWRingMode(enclient)))
    {
        _ReSetFlagType(enclient,E_SCLDMA_FLAG_EVERDMAON);
        Drv_SCLDMA_SetISRHandlerDMAOff(enclient,1);
    }
    if ((_IsFlagType(enclient,E_SCLDMA_FLAG_NEXT_OFF)))
    {
        _ReSetFlagType(enclient,E_SCLDMA_FLAG_NEXT_OFF);
        Drv_SCLDMA_SetISRHandlerDMAOff(enclient,0);
        gstScldmaInfo.bDMAOnOff[enclient] = 0;
    }
    if ((_IsFlagType(enclient,E_SCLDMA_FLAG_NEXT_ON))&&(!_IsFlagType(enclient,E_SCLDMA_FLAG_DMAFORCEOFF)))
    {
        _ReSetFlagType(enclient,E_SCLDMA_FLAG_NEXT_ON);
        Drv_SCLDMA_SetISRHandlerDMAOff(enclient,1);
        gstScldmaInfo.bDMAOnOff[enclient] = 1;
    }
}
void _Drv_SCLIRQ_SetDMAOnOffWithoutDoubleBufferHandler(void)
{
    _Drv_SCLIRQ_SetDMAOnOffWithoutDoubleBuffer(E_SCLDMA_1_FRM_W);
    _Drv_SCLIRQ_SetDMAOnOffWithoutDoubleBuffer(E_SCLDMA_1_SNP_W);
    _Drv_SCLIRQ_SetDMAOnOffWithoutDoubleBuffer(E_SCLDMA_2_FRM_W);
    //for HW bug
    _Drv_SCLIRQ_SetDMAOnOffWithoutDoubleBuffer(E_SCLDMA_2_FRM2_W);
}
void _Drv_SCLIRQ_ReduceDropFrameCount(void)
{
    if(gstDropFrameCount)
    {
        gstDropFrameCount--;
    }
}
static void Drv_SCLIRQ_SetRealTimeFrmEndHandler(MS_U32 u32Time)
{
    _Drv_SCLIRQ_SetIsBlankingRegion(1);
    if(MsOS_GetEvent(_s32SYNCEventId)&E_SCLIRQ_EVENT_ISPFRMEND)
    {
        Drv_SCLIRQ_SetDMADoneEarlyISP(0);
    }
    else
    {
        Drv_SCLIRQ_SetDMADoneEarlyISP(1);
    }
    if(_IsFlagType(E_SCLDMA_2_FRM2_W,E_SCLDMA_FLAG_ACTIVE))
    {
        gbsc2frame2reopen = 1;
    }
    else
    {
        _Drv_SCLIRQ_SetDMAOnOffWithoutDoubleBufferHandler();
        if((!gbDMADoneEarlyISP) || (gbPtgenMode))
        {
            _Drv_SCLIRQ_CMDQBlankingHandler();
        }
        _Drv_SCLIRQ_ReduceDropFrameCount();
    }
}
static void Drv_SCLIRQ_SetFrmEndInterruptStatus(MS_U32 u32Time)
{
    //_Drv_SCLIRQ_MonitorSystemTimer();
    Drv_SCLDMA_SetISPFrameCount();
    _Drv_SCLIRQ_FrmEndhandler();
    _Drv_SCLIRQ_SclFrameEndConnectToISP();
}

void _Drv_SCLIRQ_HandlerFRMIN(MS_BOOL bEn)
{
    if(!gbPtgenMode)
    {
        gbFRMInFlag = (MS_BOOL)((gbFRMInFlag)|bEn);
    }
}
void _Drv_SCLIRQ_HandlerFRMINShift(void)
{
    if(!gbPtgenMode)
    {
        gbFRMInFlag = (MS_BOOL)((gbFRMInFlag << 1));
    }
}
void _Drv_SCLIRQ_IsAffFullContinue(MS_BOOL *bAffFull ,MS_BOOL *bAffcount)
{
    if((*bAffFull))
    {
        (*bAffcount)++;
    }
}
void _Drv_SCLIRQ_IsAffFullContinually(MS_BOOL *bAffFull ,MS_BOOL *bAffcount)
{
    static MS_U8 u8framecount = 0;
    Drv_SCLIRQ_InterruptEnable(SCLIRQ_AFF_FULL);
    if(!(*bAffFull))
    {
        //Hal_SCLIRQ_Set_Mask(EN_SCLIRQ_SCTOP_0,SCLIRQ_MSK_AFF_FULL,0);
        u8framecount = 0;
    }
    else
    {
        //Hal_SCLIRQ_Set_Mask(EN_SCLIRQ_SCTOP_0,SCLIRQ_MSK_AFF_FULL,1);
        u8framecount++;
        *bAffFull = 0;
    }
    if((u8framecount)>2)
    {
        *bAffcount = 0;
        Drv_SCLIRQ_SetDAZAQueueWork(E_SCLIRQ_DAZA_BRESETFCLK);
        if(gstDropFrameCount <= 1)
        {
            gstDropFrameCount = 1;
        }
        u8framecount = 0;
    }
}
void _Drv_SCLIRQ_AffFullHandler(MS_BOOL *bAffFull ,MS_BOOL *bAffcount)
{
    SCL_DBG(SCL_DBG_LV_DRVSCLIRQ()&(EN_DBGMG_SCLIRQLEVEL_ELSE),
        "SC AFF FULL %lu @:%lu\n",gstSCInts.u32AffCount,gu32Time);
    SCL_DBG(SCL_DBG_LV_DRVSCLIRQ()&(EN_DBGMG_SCLIRQLEVEL_ELSE),
        "CROP H:%hd V:%hd \n",Hal_SCLIRQ_Get_RegVal(REG_SCL2_70_L),Hal_SCLIRQ_Get_RegVal(REG_SCL2_71_L));
    SCL_DBG(SCL_DBG_LV_DRVSCLIRQ()&(EN_DBGMG_SCLIRQLEVEL_ELSE),
        "CROP2 H:%hd V:%hd \n",Hal_SCLIRQ_Get_RegVal(REG_SCL2_72_L),Hal_SCLIRQ_Get_RegVal(REG_SCL2_73_L));
    *bAffFull = 1;
    Drv_SCLIRQ_Disable(SCLIRQ_AFF_FULL);
    _Drv_SCLIRQ_IsAffFullContinue(bAffFull,bAffcount);
    if(gstSCInts.u32AffCount == INT_COUNT_TOLERANCE)
    {
        Drv_SCLIRQ_SetDAZAQueueWork(E_SCLIRQ_DAZA_BCLOSECIIR);
    }
    else if(gstSCInts.u32AffCount == 30)
    {
        MsOS_ClearEvent(_s32SYNCEventId, E_SCLIRQ_EVENT_BCLOSECIIR);
    }
    else if(gstSCInts.u32AffCount == 40)
    {
        Drv_SCLIRQ_SetDAZAQueueWork(E_SCLIRQ_DAZA_BCLOSECIIR);
    }
}
void _Drv_SCLIRQ_FrameInEndHandler(MS_BOOL *bNonFRMEndCount,MS_BOOL *bISPFramePerfect,MS_BOOL *bAffFull)
{
    if(MsOS_GetEvent(_s32SYNCEventId)&E_SCLIRQ_EVENT_FRMENDSYNC)
    {
        Drv_SCLIRQ_SetDMADoneEarlyISP(1);
    }
    else
    {
        Drv_SCLIRQ_SetDMADoneEarlyISP(0);
    }
    Drv_SCLDMA_SetISPFrameCount();
    *bISPFramePerfect = 1;
    //_Drv_SCLIRQ_HandlerFRMIN((*bAffFull) ? 0 : 1);   //for AFF drop
    _Drv_SCLIRQ_HandlerFRMIN(1);
    if(gbBypassDNR)
    {
        Hal_SCLIRQ_SetDNRBypass(0);
        gbBypassDNR = 0;
    }
    if(gbDMADoneEarlyISP)
    {
        if(!SCL_DELAYFRAME)
        {
            _Drv_SCLIRQ_SetFRMINWhenDMADoneEarlyISP(1);
            //TODO:not open this time
            _Drv_SCLIRQ_FrmDoneEventHandler();
        }
        _Drv_SCLIRQ_CMDQBlankingHandler();
    }
    MsOS_SetEvent(_s32SYNCEventId, E_SCLIRQ_EVENT_ISPFRMEND);
}
void _Drv_SCLIRQ_IDCLKVsyncInRealTimeHandler(void)
{
    MsOS_SetEvent(Drv_SCLIRQ_Get_IRQ_SYNCEventID(), E_SCLIRQ_EVENT_LDCSYNC);
    MsOS_ClearEvent(Drv_SCLIRQ_Get_IRQ_SYNCEventID(),(E_SCLIRQ_EVENT_FRMENDSYNC|E_SCLIRQ_EVENT_ISPFRMEND));
    if(gbEachDMAEnable)
    {
        _Drv_SCLIRQ_SetIsBlankingRegion(0);
    }
    //disable dnr for ISP not perfect
    if(!_IsFrmIN_Vsync())
    {
        Hal_SCLIRQ_SetDNRBypass(1);
        gbBypassDNR = 1;
    }
    if(gsclirqstate &E_SCLIRQ_EVENT_SET)
    {
        gsclirqstate = E_SCLIRQ_EVENT_CLEAR;
    }
    else if(gsclirqstate)
    {
        gstSCInts.u32ErrorCount++;
        if(gbDMADoneEarlyISP)
        {
            if(!SCL_DELAYFRAME)
            {
                SCL_DBGERR("[LDC]:ISP_IN lost:%lu\n",gstSCInts.u32ErrorCount);
                _Drv_SCLIRQ_SetFRMINWhenDMADoneEarlyISP(0);
                //TODO:not open this time
                _Drv_SCLIRQ_FrmDoneEventHandler();
                if(gsclirqstate &E_SCLIRQ_EVENT_SET)
                {
                    gsclirqstate = E_SCLIRQ_EVENT_CLEAR;
                }
            }
            _Drv_SCLIRQ_CMDQBlankingHandler();
        }
    }
    else if(gbDMADoneEarlyISP && !_IsFrmIN_Vsync())
    {
        // for rotate make stream hang up
        SCL_DBGERR("[LDC]ISP_IN lost:%lu\n",gstSCInts.u32ErrorCount);
        _Drv_SCLIRQ_CMDQBlankingHandler();
    }
}
void _Drv_SCLIRQ_LDCVsyncInRealTimeHandler(void)
{
    unsigned char idx;
    unsigned char loop = 0;
    if(gbEachDMAEnable)
    {
        if(!(MsOS_GetEvent(Drv_SCLIRQ_Get_IRQ_SYNCEventID())&(E_SCLIRQ_EVENT_FRMENDSYNC)))
        {
            //not in framedone
            for(idx = 0;idx<E_SCLDMA_CLIENT_NUM;idx++)
            {
                if(gstScldmaInfo.bDMAOnOff[idx])
                {
                    loop = 1;
                    break;
                }
            }
            if(!loop)
            {
                gbEachDMAEnable = 0;
                _Drv_SCLIRQ_SetIsBlankingRegion(1);
            }
        }
    }
    else
    {
        // for issue reset rstz_ctrl_clk_miu_sc_dnr cause to overwrite 0x0
        Hal_SCLIRQ_Set_Reg(REG_SCL_DNR1_24_L, 0, BIT6);
        Hal_SCLIRQ_Set_Reg(REG_SCL_DNR1_60_L, 0, BIT0);
    }
}
void _Drv_SCLIRQ_LDCVsyncInHandler(MS_BOOL *bISPFramePerfect ,MS_BOOL *bAffFull)
{
    _Drv_SCLIRQ_HandlerFRMINShift();
    if(gbMonitorCropMode == 1)
    {
        MsOS_SetEvent(_s32SYNCEventId, E_SCLIRQ_EVENT_SYNC);
    }
    _Drv_SCLIRQ_SclFrameStartConnectToISP();
}
void _Drv_SCLIRQ_DMAActiveHandler(EN_SCLDMA_CLIENT_TYPE enclient)
{
    if(_Is_SCLDMA_RingMode(enclient) && DoubleBufferStatus)
    {
        Drv_SCLIRQ_RingMode_Active(enclient);
    }
    else if(_Is_SCLDMA_RingMode(enclient))
    {
        Drv_SCLIRQ_RingMode_ActiveWithoutDoublebuffer(enclient);
    }
    else if(_Is_SCLDMA_SWRingMode(enclient))
    {
        Drv_SCLIRQ_SWRingMode_Active(enclient);
    }
    else if(enclient ==E_SCLDMA_3_FRM_W &&_Is_SCLDMA_SingleMode(E_SCLDMA_3_FRM_W))
    {
        Drv_SCLIRQ_SC3SingleMode_Active();
    }
    else if(_Is_SCLDMA_SingleMode(enclient) && DoubleBufferStatus)
    {
        Drv_SCLIRQ_SingleMode_Active(enclient);
    }
    else if(_Is_SCLDMA_SingleMode(enclient))
    {
        Drv_SCLIRQ_SingleMode_ActiveWithoutDoublebuffer(enclient);
    }
    Drv_SCLDMA_SetSclFrameDoneTime(enclient ,MsOS_GetSystemTimeStamp());
}
MS_BOOL _Drv_SCLIRQ_DMAActive_NHandler(EN_SCLDMA_CLIENT_TYPE enclient)
{
    MS_BOOL Ret=1;
    if(enclient < E_SCLDMA_3_FRM_R)
    {
        _Drv_SCLIRQ_SetFrameInFlag(enclient);
    }
    if(_Is_SCLDMA_RingMode(enclient) && DoubleBufferStatus)
    {
        Drv_SCLIRQ_RingMode_Active_N(enclient);
    }
    else if(_Is_SCLDMA_RingMode(enclient))
    {
        Ret = Drv_SCLIRQ_RingMode_Active_NWithoutDoublebuffer(enclient);
    }
    else if(_Is_SCLDMA_SWRingMode(enclient))
    {
        Drv_SCLIRQ_SWRingMode_Active_N(enclient);
    }
    else if(enclient ==E_SCLDMA_3_FRM_W && _Is_SCLDMA_SingleMode(E_SCLDMA_3_FRM_W))
    {
        Drv_SCLIRQ_SC3SingleMode_Active_N();
    }
    else if(_Is_SCLDMA_SingleMode(enclient) && DoubleBufferStatus)
    {
        Drv_SCLIRQ_SingleMode_Active_N(enclient);
    }
    else if(_Is_SCLDMA_SingleMode(enclient))
    {
        Ret = Drv_SCLIRQ_SingleMode_Active_NWithoutDoublebuffer(enclient);
    }
    return Ret;
}


static int SCLIRQ_DazaIST(void *arg)
{
    MS_U32 u32Events;
    SCLIRQDAZAEvent enDAZAEvent;
    SCL_DBGERR("[DRVSCLIRQ]%s:going event:%x @:%ld\n",__FUNCTION__,genDAZAEvent,((MS_U32)MsOS_GetSystemTimeStamp()));
    if((MsOS_GetEvent(Drv_SCLIRQ_Get_IRQ_SYNCEventID())&(E_SCLIRQ_EVENT_ISPFRMEND|E_SCLIRQ_EVENT_FRMENDSYNC))
        !=(E_SCLIRQ_EVENT_ISPFRMEND|E_SCLIRQ_EVENT_FRMENDSYNC))
    {
        MsOS_WaitEvent(Drv_SCLIRQ_Get_IRQ_SYNCEventID(), E_SCLIRQ_EVENT_ISPFRMEND, &u32Events, E_OR, 2000); // get status: FRM END
        MsOS_DelayTaskUs(100);
    }
    DRV_SCLIRQ_MUTEX_LOCK_ISR();
    enDAZAEvent = genDAZAEvent;
    genDAZAEvent &= (~enDAZAEvent);
    DRV_SCLIRQ_MUTEX_UNLOCK_ISR();
    if(enDAZAEvent &E_SCLIRQ_DAZA_BRESETFCLK)
    {
        Hal_SCLIRQ_Set_Reg(REG_SCL0_01_L, BIT3, BIT3);
        Hal_SCLIRQ_Set_Reg(REG_SCL0_01_L, 0, BIT3);
    }
    if(enDAZAEvent &E_SCLIRQ_DAZA_BCLOSECIIR)
    {
        MsOS_SetEvent_IRQ(_s32SYNCEventId, E_SCLIRQ_EVENT_BCLOSECIIR);
    }
    if(enDAZAEvent &E_SCLIRQ_DAZA_BCLOSELDCP)
    {
        SCL_DBGERR("[DRVSCLIRQ]%s:!!!E_SCLIRQ_DAZA_BCLOSELDCP@:%lu\n",__FUNCTION__,((MS_U32)MsOS_GetSystemTimeStamp()));
        Hal_SCLIRQ_Set_Reg(REG_SCL0_01_L, BIT3, BIT3);
        Hal_SCLIRQ_Set_Reg(REG_SCL0_01_L, 0, BIT3);
        Hal_SCLIRQ_Set_Reg(REG_SCL0_03_L, 0, BIT12);
        DRV_SCLIRQ_MUTEX_LOCK_ISR();
        if(gstDropFrameCount <= 1)
        {
            gstDropFrameCount = 1;
        }
        MsOS_SetSCLFrameDelay(0);
        DRV_SCLIRQ_MUTEX_UNLOCK_ISR();
    }
    if(enDAZAEvent &E_SCLIRQ_DAZA_BRESETLDCP)
    {
        SCL_DBGERR("[DRVSCLIRQ]%s:!!!E_SCLIRQ_DAZA_BRESETLDCP@:%lu\n",__FUNCTION__,((MS_U32)MsOS_GetSystemTimeStamp()));
        Hal_SCLIRQ_Set_Reg(REG_SCL0_03_L, BIT12, BIT12);
        DRV_SCLIRQ_MUTEX_LOCK_ISR();
        MsOS_SetSCLFrameDelay(1);
        DRV_SCLIRQ_MUTEX_UNLOCK_ISR();
    }
    return 0;
}
void _Drv_SCLIRQ_SetHvspHSram(EN_HVSP_ID_TYPE enHVSP_ID,MS_U32 u32EventFlag)
{
    MS_U32 u32Events;
    if(Drv_SCLIRQ_GetIsBlankingRegion())
    {
        Drv_HVSP_SetSCIQHSRAM(enHVSP_ID);
        MsOS_ClearEventIRQ(_s32SYNCEventId,(u32EventFlag));
    }
    else
    {
        SCL_DBG(SCL_DBG_LV_DRVCMDQ()&EN_DBGMG_CMDQEVEL_ISR, "[DRVSCLIRQ]%s:SRAMSET wating\n",__FUNCTION__);
        MsOS_WaitEvent(Drv_SCLIRQ_Get_IRQ_SYNCEventID(), E_SCLIRQ_EVENT_FRMEND, &u32Events, E_OR, 2000); // get status: FRM END
        Drv_HVSP_SetSCIQHSRAM(enHVSP_ID);
        MsOS_ClearEventIRQ(_s32SYNCEventId,(u32EventFlag));
    }
}
void _Drv_SCLIRQ_SetHvspVSram(EN_HVSP_ID_TYPE enHVSP_ID,MS_U32 u32EventFlag)
{
    MS_U32 u32Events;
    if(Drv_SCLIRQ_GetIsBlankingRegion())
    {
        Drv_HVSP_SetSCIQVSRAM(enHVSP_ID);
        MsOS_ClearEventIRQ(_s32SYNCEventId,(u32EventFlag));
    }
    else
    {
        SCL_DBG(SCL_DBG_LV_DRVCMDQ()&EN_DBGMG_CMDQEVEL_ISR, "[DRVSCLIRQ]%s:SRAMSET wating\n",__FUNCTION__);
        MsOS_WaitEvent(Drv_SCLIRQ_Get_IRQ_SYNCEventID(), E_SCLIRQ_EVENT_FRMEND, &u32Events, E_OR, 2000); // get status: FRM END
        Drv_HVSP_SetSCIQVSRAM(enHVSP_ID);
        MsOS_ClearEventIRQ(_s32SYNCEventId,(u32EventFlag));
    }
}
static int SCLIRQ_CMDQIST(void *arg)
{
    MS_U32 u32Events;
    u32Events = MsOS_GetEvent(_s32SYNCEventId);
    SCL_DBG(SCL_DBG_LV_DRVCMDQ()&EN_DBGMG_CMDQEVEL_ISR, "[DRVSCLIRQ]%s:going\n",__FUNCTION__);
    if(u32Events & E_SCLIRQ_EVENT_BRESETFCLK)
    {
        Drv_SCLIRQ_SetDAZAQueueWorkISR(E_SCLIRQ_DAZA_BRESETFCLK);
        MsOS_ClearEventIRQ(_s32SYNCEventId,(E_SCLIRQ_EVENT_BRESETFCLK));
    }
    if(u32Events&E_SCLIRQ_EVENT_VIP)
    {
        if((VIPSETRULE() == E_VIPSetRule_CMDQCheck)||(VIPSETRULE() == E_VIPSetRule_CMDQAllCheck))
        {
            MsOS_CheckEachIPByCMDQIST();
        }
        MsOS_ClearEventIRQ(_s32SYNCEventId,E_SCLIRQ_EVENT_VIP);
    }
    if(u32Events&E_SCLIRQ_EVENT_GM10to12_R)
    {
        {
            Drv_CMDQ_CheckVIPSRAM(E_SCLIRQ_EVENT_GM10to12_R);
        }
        MsOS_ClearEventIRQ(_s32SYNCEventId,E_SCLIRQ_EVENT_GM10to12_R);
    }
    if(u32Events&E_SCLIRQ_EVENT_GM10to12_G)
    {
        {
            Drv_CMDQ_CheckVIPSRAM(E_SCLIRQ_EVENT_GM10to12_G);
        }
        MsOS_ClearEventIRQ(_s32SYNCEventId,E_SCLIRQ_EVENT_GM10to12_G);
    }
    if(u32Events&E_SCLIRQ_EVENT_GM10to12_B)
    {
        {
            Drv_CMDQ_CheckVIPSRAM(E_SCLIRQ_EVENT_GM10to12_B);
        }
        MsOS_ClearEventIRQ(_s32SYNCEventId,E_SCLIRQ_EVENT_GM10to12_B);
    }
    if(u32Events&E_SCLIRQ_EVENT_GM12to10_R)
    {
        {
            Drv_CMDQ_CheckVIPSRAM(E_SCLIRQ_EVENT_GM12to10_R);
        }
        MsOS_ClearEventIRQ(_s32SYNCEventId,E_SCLIRQ_EVENT_GM12to10_R);
    }
    if(u32Events&E_SCLIRQ_EVENT_GM12to10_G)
    {
        {
            Drv_CMDQ_CheckVIPSRAM(E_SCLIRQ_EVENT_GM12to10_G);
        }
        MsOS_ClearEventIRQ(_s32SYNCEventId,E_SCLIRQ_EVENT_GM12to10_G);
    }
    if(u32Events&E_SCLIRQ_EVENT_GM12to10_B)
    {
        {
            Drv_CMDQ_CheckVIPSRAM(E_SCLIRQ_EVENT_GM12to10_B);
        }
        MsOS_ClearEventIRQ(_s32SYNCEventId,E_SCLIRQ_EVENT_GM12to10_B);
    }
    if(u32Events&E_SCLIRQ_EVENT_GAMMA_Y)
    {
        {
            Drv_CMDQ_CheckVIPSRAM(E_SCLIRQ_EVENT_GAMMA_Y);
        }
        MsOS_ClearEventIRQ(_s32SYNCEventId,E_SCLIRQ_EVENT_GAMMA_Y);
    }
    if(u32Events&E_SCLIRQ_EVENT_GAMMA_U)
    {
        {
            Drv_CMDQ_CheckVIPSRAM(E_SCLIRQ_EVENT_GAMMA_U);
        }
        MsOS_ClearEventIRQ(_s32SYNCEventId,E_SCLIRQ_EVENT_GAMMA_U);
    }
    if(u32Events&E_SCLIRQ_EVENT_GAMMA_V)
    {
        {
            Drv_CMDQ_CheckVIPSRAM(E_SCLIRQ_EVENT_GAMMA_V);
        }
        MsOS_ClearEventIRQ(_s32SYNCEventId,E_SCLIRQ_EVENT_GAMMA_V);
    }
    if(u32Events & E_SCLIRQ_EVENT_SC1HSRAMSET)
    {
        _Drv_SCLIRQ_SetHvspHSram(E_HVSP_ID_1,E_SCLIRQ_EVENT_SC1HSRAMSET);
    }
    if(u32Events & E_SCLIRQ_EVENT_SC1VSRAMSET)
    {
        _Drv_SCLIRQ_SetHvspVSram(E_HVSP_ID_1,E_SCLIRQ_EVENT_SC1VSRAMSET);
    }
    if(u32Events & E_SCLIRQ_EVENT_SC2HSRAMSET)
    {
        _Drv_SCLIRQ_SetHvspHSram(E_HVSP_ID_2,E_SCLIRQ_EVENT_SC2HSRAMSET);
    }
    if(u32Events & E_SCLIRQ_EVENT_SC2VSRAMSET)
    {
        _Drv_SCLIRQ_SetHvspVSram(E_HVSP_ID_2,E_SCLIRQ_EVENT_SC2VSRAMSET);
    }
    if(u32Events & E_SCLIRQ_EVENT_SC3HSRAMSET)
    {
        _Drv_SCLIRQ_SetHvspHSram(E_HVSP_ID_3,E_SCLIRQ_EVENT_SC3HSRAMSET);
    }
    if(u32Events & E_SCLIRQ_EVENT_SC3VSRAMSET)
    {
        _Drv_SCLIRQ_SetHvspVSram(E_HVSP_ID_3,E_SCLIRQ_EVENT_SC3VSRAMSET);
    }

   return 0;

}
void _Drv_SCLIRQ_IsDMAHangUp(MS_U64 u64Flag)
{
    if(u64Flag &SCLIRQ_MSK_VSYNC_FCLK_LDC)
    {
        gstDmaHang.bVsyncWithVend++;
        SCL_DBGERR("[DRVSCLIRQ] bVsyncWithVend :%hhu\n",gstDmaHang.bVsyncWithVend);
    }
    else
    {
        gstDmaHang.bVsyncWithVend = 0;
        gstDmaHang.bLDCEvenOpenCount++;
    }
    if(gstDmaHang.bLDCEvenOpen && gstDmaHang.bLDCEvenOpenCount>100)
    {
        if(gstDmaHang.bResetHangCnt<3)
        {
            Drv_SCLIRQ_SetDAZAQueueWork(E_SCLIRQ_DAZA_BRESETLDCP);
            gstDmaHang.bResetHangCnt++;
            gstDmaHang.bLDCEvenOpen = 0;
            gstDmaHang.bLDCEvenOpenCount = 0;
        }
        else
        {
            gstDmaHang.bLDCEvenOpen = 0;
            gstDmaHang.bLDCEvenOpenCount = 0;
        }
    }
    if(gstDmaHang.bVsyncWithVend > INT_COUNT_TOLERANCE && (Hal_SCLIRQ_Get_RegVal(REG_SCL0_03_L)&BIT12))
    {
        Drv_SCLIRQ_SetDAZAQueueWork(E_SCLIRQ_DAZA_BCLOSELDCP);
        gstDmaHang.bLDCEvenOpen = 1;
        gstDmaHang.bVsyncWithVend = 0;
        gstDmaHang.bLDCEvenOpenCount = 0;
    }
}
MS_BOOL _Drv_SCLIRQ_ISTWork(ST_SCLDMA_THREAD_CONFIG *pstThread)
{
    if (IS_ERR(pstThread->sttask.pThread))
    {
        pstThread->sttask.pThread = NULL;
        Drv_SCLDMA_SetThreadOnOffFlag(0,pstThread);
        DRV_SCLIRQ_ERR(printf("[DRVSCLDMA]Fail:creat thread\n"));
        return FALSE;
    }
    else
    {
        Drv_SCLDMA_SetThreadOnOffFlag(1,pstThread);
        return TRUE;
    }
}
void _Drv_SCLIRQ_SetISPBlankingTime(MS_U32 u32DiffTime)
{
    static MS_BOOL bChangeTimingCount = 0;
    if(gstSCInts.u32ISPBlankingTime > u32DiffTime && (u32DiffTime > CRITICAL_SECTION_TIMING_MIN))
    {
        //for get most critical
        gstSCInts.u32ISPBlankingTime = u32DiffTime;
    }
    else if(gstSCInts.u32ISPBlankingTime == 0)
    {
        //for get inital
        gstSCInts.u32ISPBlankingTime = u32DiffTime;
    }
    else if (gstSCInts.u32ISPBlankingTime < u32DiffTime)
    {
        if(u32DiffTime - gstSCInts.u32ISPBlankingTime > CHANGE_CRITICAL_SECTION_TIMING_RANGE)
        {
            // for timing change
            bChangeTimingCount ++;
        }
        else
        {
            bChangeTimingCount = 0;
        }
        if(bChangeTimingCount > CHANGE_CRITICAL_SECTION_TIMING_COUNT)
        {
            // for ensure timing change
            gstSCInts.u32ISPBlankingTime = u32DiffTime;
            SCL_DBGERR("[SCLIRQ]chang time :%lu\n",gstSCInts.u32ISPBlankingTime);
            bChangeTimingCount = 0;
        }
    }
}
MS_U64 _Drv_SCLIRQ_FlagRefine(MS_U64 u64Flag)
{
    static MS_BOOL bDMAHangUp = 0;
    // for ensure blanking is real if interrupt not stable.

    if((u64Flag &SCLIRQ_MSK_VSYNC_FCLK_LDC)&&(u64Flag &SCLIRQ_MSK_SC1_ENG_FRM_END)&&(u64Flag &SCLIRQ_MSK_SC_IN_FRM_END))
    {
        // for no handle case
        bDMAHangUp = 0;
        return u64Flag;
    }
    if((u64Flag &SCLIRQ_MSK_SC_IN_FRM_END)&&(bDMAHangUp))
    {
        // shift frame end to real blanking when interrupt not stable.
        u64Flag |=(SCLIRQ_MSK_SC1_ENG_FRM_END);
        bDMAHangUp = 0;
    }
    if((u64Flag &SCLIRQ_MSK_VSYNC_FCLK_LDC)&&(u64Flag &SCLIRQ_MSK_SC1_ENG_FRM_END))
    {
        // for make sure frame end is blanking
        u64Flag &=(~SCLIRQ_MSK_SC1_ENG_FRM_END);
        bDMAHangUp = 1;
    }
    else if((u64Flag &SCLIRQ_MSK_VSYNC_FCLK_LDC) && bDMAHangUp &&!(u64Flag &SCLIRQ_MSK_SC1_ENG_FRM_END))
    {
        // for already recover
        bDMAHangUp = 0;
    }
    else if((u64Flag &SCLIRQ_MSK_SC1_ENG_FRM_END) && bDMAHangUp &&!(u64Flag &SCLIRQ_MSK_VSYNC_FCLK_LDC))
    {
        // for already recover
        bDMAHangUp = 0;
    }
    return u64Flag;
}
#if USE_RTK
void _Drv_SCLIRQ_CMDQ_StateTrig(void)
{
    MsOS_ClearEvent(_s32SYNCEventId,E_SCLIRQ_EVENT_CMDQ);
    MsOS_SetEvent(_s32SYNCEventId, E_SCLIRQ_EVENT_CMDQGOING);
}
void _Drv_SCLIRQ_CMDQ_StateDone(void)
{
    MsOS_SetEvent(_s32SYNCEventId, E_SCLIRQ_EVENT_CMDQDONE);
    Drv_CMDQ_SetRPoint(EN_CMDQ_TYPE_IP0);
    MsOS_QueueWork(1,gstThreadCfg.s32Taskid[EN_SCLIRQ_SCTASKID_DAZA],
        gstThreadCfg.s32HandlerId[EN_SCLIRQ_SCTASK_WORKQUEUECMDQ],0);
}
static int SCLIRQ_IST(void *arg)
{
    static MS_BOOL bAffFull = 0,bAffcount=0;
    static MS_BOOL bISPFramePerfect = 0;
    static MS_BOOL bNonFRMEndCount = 0;
    DRV_SCLIRQ_MUTEX_LOCK_ISR();
    SCL_DBG(SCL_DBG_LV_DRVSCLIRQ()&EN_DBGMG_SCLIRQLEVEL_ELSE, "[DRVSCLIRQ]%s:going \n",__FUNCTION__);
    if(MsOS_GetandClearEventRing(EN_SCLIRQ_SCIRQ_AFFFULL))
    {
        _Drv_SCLIRQ_AffFullHandler(&bAffFull, &bAffcount);
    }
    if(MsOS_GetandClearEventRing(EN_SCLIRQ_SCIRQ_INFRMEND))
    {
        _Drv_SCLIRQ_FrameInEndHandler(&bNonFRMEndCount,&bISPFramePerfect,&bAffFull);
    }
    if(MsOS_GetandClearEventRing(EN_SCLIRQ_SCIRQ_FCLKLDC))
    {
        _Drv_SCLIRQ_LDCVsyncInHandler(&bISPFramePerfect,&bAffFull);
        bAffFull = 0;
    }
    // SC1 FRM
    if(MsOS_GetandClearEventRing(EN_SCLIRQ_SCIRQ_SC1FRMWACTIVE))
    {
        _Drv_SCLIRQ_DMAActiveHandler(E_SCLDMA_1_FRM_W);
    }

    if(MsOS_GetandClearEventRing(EN_SCLIRQ_SCIRQ_SC1FRMWACTIVE_N))
    {
        if(_Drv_SCLIRQ_DMAActive_NHandler(E_SCLDMA_1_FRM_W))
        {
            gsclirqstate |= (E_SCLIRQ_EVENT_SC1FRM|E_SCLIRQ_EVENT_ISTSC1FRM);
        }
    }


    // SC2 FRM
    if(MsOS_GetandClearEventRing(EN_SCLIRQ_SCIRQ_SC2FRMWACTIVE))
    {
        _Drv_SCLIRQ_DMAActiveHandler(E_SCLDMA_2_FRM_W);
    }

    if(MsOS_GetandClearEventRing(EN_SCLIRQ_SCIRQ_SC2FRMWACTIVE_N))
    {
        if(_Drv_SCLIRQ_DMAActive_NHandler(E_SCLDMA_2_FRM_W))
        {
            gsclirqstate |= (E_SCLIRQ_EVENT_SC2FRM|E_SCLIRQ_EVENT_ISTSC2FRM);
        }
    }
    // SC2 FRM2
    if(MsOS_GetandClearEventRing(EN_SCLIRQ_SCIRQ_SC2FRM2WACTIVE))
    {
        _Drv_SCLIRQ_DMAActiveHandler(E_SCLDMA_2_FRM2_W);
    }
    if(MsOS_GetandClearEventRing(EN_SCLIRQ_SCIRQ_SC2FRM2WACTIVE_N))
    {
        if(_Drv_SCLIRQ_DMAActive_NHandler(E_SCLDMA_2_FRM2_W))
        {
            MsOS_SetEvent(_s32FRMENDEventId, (E_SCLIRQ_EVENT_SC2FRM2|E_SCLIRQ_EVENT_ISTSC2FRM2));
        }
    }

    // SC1 SNP
    if(MsOS_GetandClearEventRing(EN_SCLIRQ_SCIRQ_SC1SNPWACTIVE))
    {
        _Drv_SCLIRQ_DMAActiveHandler(E_SCLDMA_1_SNP_W);
    }

    if(MsOS_GetandClearEventRing(EN_SCLIRQ_SCIRQ_SC1SNPWACTIVE_N))
    {
        if(_Drv_SCLIRQ_DMAActive_NHandler(E_SCLDMA_1_SNP_W))
        {
            gsclirqstate |= (E_SCLIRQ_EVENT_SC1SNP|E_SCLIRQ_EVENT_ISTSC1SNP);
        }
    }
    // SC SRC FRMEND
    if(MsOS_GetandClearEventRing(EN_SCLIRQ_SCIRQ_SCTOPFRMEND))
    {
        Drv_SCLIRQ_SetFrmEndInterruptStatus(gu32Time);
        bNonFRMEndCount = 0;
        _Drv_SCLIRQ_IsAffFullContinually(&bAffFull, &bAffcount);
    }
    // SC3 FRM
    if(MsOS_GetandClearEventRing(EN_SCLIRQ_SCIRQ_SC3FRMWACTIVE))
    {
        gscl3irqstate = E_SCLIRQ_SC3EVENT_CLEAR;
        gscl3irqstate |= (E_SCLIRQ_SC3EVENT_ACTIVE | E_SCLIRQ_SC3EVENT_ISTACTIVE);
        _Drv_SCLIRQ_DMAActiveHandler(E_SCLDMA_3_FRM_W);
    }

    if(MsOS_GetandClearEventRing(EN_SCLIRQ_SCIRQ_SC3FRMWACTIVE_N))
    {
        gscl3irqstate |= (E_SCLIRQ_SC3EVENT_ACTIVEN | E_SCLIRQ_SC3EVENT_ISTACTIVEN | E_SCLIRQ_SC3EVENT_IRQ);
        _Drv_SCLIRQ_DMAActive_NHandler(E_SCLDMA_3_FRM_W);
    }
    if(MsOS_GetandClearEventRing(EN_SCLIRQ_SCIRQ_CMDQ0TRIG))
    {
        _Drv_SCLIRQ_CMDQ_StateTrig();
    }

    if(MsOS_GetandClearEventRing(EN_SCLIRQ_SCIRQ_CMDQ0DONE))
    {
        _Drv_SCLIRQ_CMDQ_StateDone();
    }
    DRV_SCLIRQ_MUTEX_UNLOCK_ISR();
   return 0;

}
MS_BOOL _Delete_SCLIRQ_IST(void)
{
    gstThreadCfg.flag = 0;
    MsOS_FlushWorkQueue(1,gstThreadCfg.s32Taskid[EN_SCLIRQ_SCTASKID_DAZA]);
    MsOS_FlushWorkQueue(1,gstThreadCfg.s32Taskid[EN_SCLIRQ_SCTASKID_IRQ]);
    MsOS_DestroyWorkQueueTask(gstThreadCfg.s32Taskid[EN_SCLIRQ_SCTASKID_DAZA]);
    MsOS_DestroyWorkQueueTask(gstThreadCfg.s32Taskid[EN_SCLIRQ_SCTASKID_IRQ]);
    return 0;
}
MS_BOOL _Create_SCLIRQ_IST(void)
{
    char *pName2={"SCLDAZA_THREAD"};
    char *pName={"SCLIRQ_THREAD"};
    unsigned char bRet = 0;
    gstThreadCfg.s32Taskid[EN_SCLIRQ_SCTASKID_DAZA] = MsOS_CreateWorkQueueTask(pName2);
    gstThreadCfg.s32Taskid[EN_SCLIRQ_SCTASKID_IRQ] = MsOS_CreateWorkQueueTask(pName);
    gstThreadCfg.s32HandlerId[EN_SCLIRQ_SCTASK_WORKQUEUECMDQ] = MsOS_CreateWorkQueueEvent((void*)SCLIRQ_CMDQIST);
    gstThreadCfg.s32HandlerId[EN_SCLIRQ_SCTASK_WORKQUEUEDAZA] = MsOS_CreateWorkQueueEvent((void*)SCLIRQ_DazaIST);
    gstThreadCfg.s32HandlerId[EN_SCLIRQ_SCTASK_WORKQUEUEIRQ] = MsOS_CreateWorkQueueEvent((void*)SCLIRQ_IST);
    return bRet;
}
irqreturn_t  _Drv_SCLIRQ_isr(int eIntNum, void* dev_id)
{
    static MS_U32 u32ActTime[7];
    MS_U64 u64Flag,u64Mask=0x0;
    MS_U32 u32DiffTime;
    u64Mask = gstScldmaInfo.u64mask;
    u64Flag = Hal_SCLIRQ_Get_Flag(EN_SCLIRQ_SCTOP_0,0xFFFFFFFFFFFF);
    if(_bSCLIRQ_Suspend)
    {
        Hal_SCLIRQ_Set_Clear(EN_SCLIRQ_SCTOP_0,0xFFFFFFFFFFFF, 1);
    }

    Hal_SCLIRQ_Set_Clear(EN_SCLIRQ_SCTOP_0,(~u64Mask)&u64Flag, 1);
    gu32Time = ((MS_U32)MsOS_GetSystemTimeStamp());
    if(u64Flag &SCLIRQ_MSK_AFF_FULL)
    {
        gstSCInts.u32AffCount++;
        if(u64Flag &SCLIRQ_MSK_VSYNC_FCLK_LDC)
        {
            // HW bug : aff fifo full error report
        }
        else
        {
            MsOS_SetEventRing(EN_SCLIRQ_SCIRQ_AFFFULL);
        }
    }
    // next frame IN . because of receive and send in same time.(ISP in frame end)
    // 1.frame coming(vsync)
    // 2.frame receive done.(irq get sc_frm_in_end<before next vsync,maybe>)keep in DNR
    // 3.active from dnr(LDC_trig<next vsync>)
    if(u64Flag &SCLIRQ_MSK_SC_IN_FRM_END)
    {
        gstSCInts.u32ISPDoneCount++;
        u32DiffTime = gu32Time - u32ActTime[5];
        gstSCInts.u32ISPTime += u32DiffTime;
        if(gstSCInts.u32ISPDoneCount > 256)
        {
            gstSCInts.u32ISPTime = gstSCInts.u32ISPTime/gstSCInts.u32ISPDoneCount;
            gstSCInts.u32ISPInCount = (gstSCInts.u32ISPInCount-gstSCInts.u32ISPDoneCount)+1;
            gstSCInts.u32ISPDoneCount = 1;
        }
        MsOS_SetEventRing(EN_SCLIRQ_SCIRQ_INFRMEND);
        SCL_DBG(SCL_DBG_LV_DRVSCLIRQ()&(EN_DBGMG_SCLIRQLEVEL_ELSE),"[FRMIN]@%lu\n",((MS_U32)MsOS_GetSystemTimeStamp()));
    }
    if(u64Flag &SCLIRQ_MSK_VSYNC_FCLK_LDC)
    {
        u32ActTime[5] = gu32Time;
        gstSCInts.u32ISPInCount++;
        if(MsOS_GetEvent(Drv_SCLIRQ_Get_IRQ_SYNCEventID())&(E_SCLIRQ_EVENT_FRMENDSYNC))
        {
            u32DiffTime = gu32Time - u32ActTime[6];
            gstSCInts.u32ISPBlanking += u32DiffTime;
        }
        _Drv_SCLIRQ_LDCVsyncInRealTimeHandler();
        MsOS_SetEventRing(EN_SCLIRQ_SCIRQ_FCLKLDC);
        SCL_DBG(SCL_DBG_LV_DRVSCLIRQ()&(EN_DBGMG_SCLIRQLEVEL_ELSE),"[FRMLDC]Flag:%llx @%lu\n",u64Flag,gu32Time);
    }
    if(u64Flag &SCLIRQ_MSK_VSYNC_IDCLK)
    {
        SCL_DBG(SCL_DBG_LV_DRVSCLIRQ()&(EN_DBGMG_SCLIRQLEVEL_ELSE),"[IDCLK]Flag:%llx @%lu\n",u64Flag,gu32Time);
    }
    // SC1 FRM
    if(u64Flag & SCLIRQ_MSK_SC1_FRM_W_ACTIVE)
    {
        u32ActTime[0] = gu32Time;
        MsOS_SetEventRing(EN_SCLIRQ_SCIRQ_SC1FRMWACTIVE);
    }

    if(u64Flag & SCLIRQ_MSK_SC1_FRM_W_ACTIVE_N)
    {
        MsOS_SetEventRing(EN_SCLIRQ_SCIRQ_SC1FRMWACTIVE_N);
        gstSCInts.u32SC1FrmDoneCount++;
        u32DiffTime = gu32Time - u32ActTime[0];
        gstSCInts.u32SC1FrmActiveTime += u32DiffTime;
        if(gstSCInts.u32SC1FrmDoneCount > 256)
        {
            gstSCInts.u32SC1FrmActiveTime = gstSCInts.u32SC1FrmActiveTime/gstSCInts.u32SC1FrmDoneCount;
            gstSCInts.u32SC1FrmDoneCount = 1;
        }
    }


    // SC2 FRM
    if(u64Flag & SCLIRQ_MSK_SC2_FRM_W_ACTIVE)
    {
        u32ActTime[1] = gu32Time;
        MsOS_SetEventRing(EN_SCLIRQ_SCIRQ_SC2FRMWACTIVE);
    }

    if(u64Flag & SCLIRQ_MSK_SC2_FRM_W_ACTIVE_N)
    {
        MsOS_SetEventRing(EN_SCLIRQ_SCIRQ_SC2FRMWACTIVE_N);
        gstSCInts.u32SC2FrmDoneCount++;
        u32DiffTime = gu32Time -u32ActTime[1];
        gstSCInts.u32SC2FrmActiveTime += u32DiffTime;
        if(gstSCInts.u32SC2FrmDoneCount > 256)
        {
            gstSCInts.u32SC2FrmActiveTime = gstSCInts.u32SC2FrmActiveTime/gstSCInts.u32SC2FrmDoneCount;
            gstSCInts.u32SC2FrmDoneCount = 1;
        }
    }
    // SC2 FRM2
    if(u64Flag & SCLIRQ_MSK_SC2_FRM2_W_ACTIVE)
    {
        u32ActTime[2] = gu32Time;
        MsOS_SetEventRing(EN_SCLIRQ_SCIRQ_SC2FRM2WACTIVE);
    }
    if(u64Flag & SCLIRQ_MSK_SC2_FRM2_W_ACTIVE_N)
    {
        MsOS_SetEventRing(EN_SCLIRQ_SCIRQ_SC2FRM2WACTIVE_N);
        // for HW bug
        //gsclirqstate |= (E_SCLIRQ_EVENT_SC2FRM2|E_SCLIRQ_EVENT_ISTSC2FRM2);
        if(gbsc2frame2reopen)
        {
            gbsc2frame2reopen = 0;
            SCL_DBG(SCL_DBG_LV_DRVSCLIRQ()&(EN_DBGMG_SCLIRQLEVEL_ELSE),
                "[FRMEND]SC2 FRM2 reopen\n");
            if((!gbDMADoneEarlyISP) || (gbPtgenMode))
            {
                _Drv_SCLIRQ_CMDQBlankingHandler();
            }
            _Drv_SCLIRQ_SetDMAOnOffWithoutDoubleBufferHandler();
        }
        gstSCInts.u32SC2Frm2DoneCount++;
        u32DiffTime = gu32Time -u32ActTime[2];
        gstSCInts.u32SC2Frm2ActiveTime += u32DiffTime;
        if(gstSCInts.u32SC2Frm2DoneCount > 256)
        {
            gstSCInts.u32SC2Frm2ActiveTime = gstSCInts.u32SC2Frm2ActiveTime/gstSCInts.u32SC2Frm2DoneCount;
            gstSCInts.u32SC2Frm2DoneCount = 1;
        }
    }

    // SC1 SNP
    if(u64Flag & SCLIRQ_MSK_SC1_SNP_W_ACTIVE)
    {
        u32ActTime[3] = gu32Time;
        MsOS_SetEventRing(EN_SCLIRQ_SCIRQ_SC1SNPWACTIVE);
    }

    if(u64Flag & SCLIRQ_MSK_SC1_SNP_W_ACTIVE_N)
    {
        MsOS_SetEventRing(EN_SCLIRQ_SCIRQ_SC1SNPWACTIVE_N);
        gstSCInts.u32SC1SnpDoneCount++;
        u32DiffTime = gu32Time -u32ActTime[3];
        gstSCInts.u32SC1SnpActiveTime += u32DiffTime;
        if(gstSCInts.u32SC1SnpDoneCount > 256)
        {
            gstSCInts.u32SC1SnpActiveTime = gstSCInts.u32SC1SnpActiveTime/gstSCInts.u32SC1SnpDoneCount;
            gstSCInts.u32SC1SnpDoneCount = 1;
        }
    }
    // SC SRC FRMEND
    if(u64Flag &SCLIRQ_MSK_SC1_ENG_FRM_END)
    {
        Drv_SCLIRQ_SetRealTimeFrmEndHandler(gu32Time);
        MsOS_SetEventRing(EN_SCLIRQ_SCIRQ_SCTOPFRMEND);
        gstSCInts.u32SCLMainDoneCount++;
        u32ActTime[6] = gu32Time;
        gu32FrmEndTime = gu32Time;
        u32DiffTime = gu32Time -u32ActTime[5];
        gstSCInts.u32SCLMainActiveTime += u32DiffTime;
        if(gstSCInts.u32SCLMainDoneCount > 256)
        {
            gstSCInts.u32ISPBlanking = gstSCInts.u32ISPBlankingTime;
            gstSCInts.u32SCLMainActiveTime = gstSCInts.u32SCLMainActiveTime/gstSCInts.u32SCLMainDoneCount;
            gstSCInts.u32SCLMainDoneCount = 1;
            gstSCInts.u8CountReset ++;
        }
        SCL_DBG(SCL_DBG_LV_DRVSCLIRQ()&(EN_DBGMG_SCLIRQLEVEL_ELSE),
            "[FRMEND]%x flag:%llx,%lu\n",gsclirqstate,u64Flag,((MS_U32)MsOS_GetSystemTimeStamp()));
    }

    // SC3 FRM_W
    if(u64Flag & SCLIRQ_MSK_SC3_DMA_W_ACTIVE)
    {
        MsOS_SetEventRing(EN_SCLIRQ_SCIRQ_SC3FRMWACTIVE);
        u32ActTime[4] = gu32Time;
    }

    if(u64Flag & SCLIRQ_MSK_SC3_DMA_W_ACTIVE_N)
    {
        MsOS_SetEventRing(EN_SCLIRQ_SCIRQ_SC3FRMWACTIVE_N);
        gstSCInts.u32SC3DoneCount++;
        u32DiffTime = gu32Time -u32ActTime[4];
        gstSCInts.u32SC3ActiveTime += u32DiffTime;
        if(gstSCInts.u32SC3DoneCount > 256)
        {
            gstSCInts.u32SC3ActiveTime = gstSCInts.u32SC3ActiveTime/gstSCInts.u32SC3DoneCount;
            gstSCInts.u32SC3DoneCount = 1;
        }
    }
    Hal_SCLIRQ_Set_Clear(EN_SCLIRQ_SCTOP_0,(~u64Mask)&u64Flag, 0);
    u32DiffTime = ((MS_U32)MsOS_GetSystemTimeStamp())-gu32Time;
    SCL_DBG(SCL_DBG_LV_DRVSCLIRQ()&(EN_DBGMG_SCLIRQLEVEL_NORMAL),
        "[ISR]Diff:%lu(ns)\n",u32DiffTime);
    MsOS_QueueWork(1,gstThreadCfg.s32Taskid[EN_SCLIRQ_SCTASKID_IRQ],
        gstThreadCfg.s32HandlerId[EN_SCLIRQ_SCTASK_WORKQUEUEIRQ],0);
    return IRQ_HANDLED;
}
#else
MS_BOOL _Delete_SCLIRQ_IST(void)
{
    gstThreadCfg.flag = 0;
    MsOS_FlushWorkQueue(1,gstThreadCfg.s32Taskid[EN_SCLIRQ_SCTASKID_DAZA]);
    MsOS_DestroyWorkQueueTask(gstThreadCfg.s32Taskid[EN_SCLIRQ_SCTASKID_DAZA]);
    return 0;
}
MS_BOOL _Create_SCLIRQ_IST(void)
{
    char pName2[] = {"SCLDAZA_THREAD"};
    unsigned char bRet = 0;
    gstThreadCfg.s32Taskid[EN_SCLIRQ_SCTASKID_DAZA] = MsOS_CreateWorkQueueTask(pName2);
    gstThreadCfg.s32HandlerId[EN_SCLIRQ_SCTASK_WORKQUEUECMDQ] = MsOS_CreateWorkQueueEvent((void*)SCLIRQ_CMDQIST);
    gstThreadCfg.s32HandlerId[EN_SCLIRQ_SCTASK_WORKQUEUEDAZA] = MsOS_CreateWorkQueueEvent((void*)SCLIRQ_DazaIST);
    return bRet;
}
irqreturn_t  _Drv_SCLIRQ_isr(int eIntNum, void* dev_id)
{
    static MS_U32 u32ActTime[7];
    static MS_BOOL bAffFull = 0,bAffcount=0;
    static MS_BOOL bISPFramePerfect = 0;
    static MS_BOOL bNonFRMEndCount = 0;
    MS_U64 u64Flag,u64Mask=0x0;
    MS_U32 u32DiffTime;
    u64Mask = gstScldmaInfo.u64mask;
    u64Flag = Hal_SCLIRQ_Get_Flag(EN_SCLIRQ_SCTOP_0,0xFFFFFFFFFFFF);
    if(_bSCLIRQ_Suspend)
    {
        Hal_SCLIRQ_Set_Clear(EN_SCLIRQ_SCTOP_0,0xFFFFFFFFFFFF, 1);
    }

    Hal_SCLIRQ_Set_Clear(EN_SCLIRQ_SCTOP_0,(~u64Mask)&u64Flag, 1);
    gu32Time = ((MS_U32)MsOS_GetSystemTimeStamp());
    //u64Flag = _Drv_SCLIRQ_FlagRefine(u64Flag);
    if(u64Flag &SCLIRQ_MSK_AFF_FULL)
    {
        if(u64Flag &(SCLIRQ_MSK_VSYNC_FCLK_LDC|SCLIRQ_MSK_VSYNC_IDCLK))
        {
            // HW bug : aff fifo full error report
        }
        else
        {
            if((MsOS_GetEvent(_s32SYNCEventId)& E_SCLIRQ_EVENT_BRESETFCLK) ==0)
            {
                gstSCInts.u32AffCount++;
            }
            _Drv_SCLIRQ_AffFullHandler(&bAffFull, &bAffcount);
        }
    }
    // next frame IN . because of receive and send in same time.(ISP in frame end)
    // 1.frame coming(vsync)
    // 2.frame receive done.(irq get sc_frm_in_end<before next vsync,maybe>)keep in DNR
    // 3.active from dnr(LDC_trig<next vsync>)
    if(u64Flag &SCLIRQ_MSK_SC_IN_FRM_END)
    {
        _Drv_SCLIRQ_FrameInEndHandler(&bNonFRMEndCount,&bISPFramePerfect,&bAffFull);
        gstSCInts.u32ISPDoneCount++;
        u32DiffTime = gu32Time - u32ActTime[5];
        gstSCInts.u32ISPTime += u32DiffTime;
        if(gstSCInts.u32ISPDoneCount > 256)
        {
            gstSCInts.u32ISPTime = gstSCInts.u32ISPTime/gstSCInts.u32ISPDoneCount;
            gstSCInts.u32ISPInCount = (gstSCInts.u32ISPInCount-gstSCInts.u32ISPDoneCount)+1;
            gstSCInts.u32ISPDoneCount = 1;
        }
        SCL_DBG(SCL_DBG_LV_DRVSCLIRQ()&(EN_DBGMG_SCLIRQLEVEL_ELSE),"[FRMIN]@%lu\n",gu32Time);
    }
    if(u64Flag &SCLIRQ_MSK_VSYNC_IDCLK)
    {
        gstSCInts.u32ISPInCount++;
        if(MsOS_GetEvent(Drv_SCLIRQ_Get_IRQ_SYNCEventID())&(E_SCLIRQ_EVENT_FRMENDSYNC))
        {
            u32DiffTime = gu32Time - u32ActTime[6];
            if(!(u64Flag &SCLIRQ_MSK_SC1_ENG_FRM_END))
            {
                _Drv_SCLIRQ_SetISPBlankingTime(u32DiffTime);
            }
            gstSCInts.u32ISPBlanking += u32DiffTime;
        }
        _Drv_SCLIRQ_IDCLKVsyncInRealTimeHandler();
        SCL_DBG(SCL_DBG_LV_DRVSCLIRQ()&(EN_DBGMG_SCLIRQLEVEL_ELSE),"[IDCLK]Flag:%llx @%lu\n",u64Flag,gu32Time);
    }
    if(u64Flag &SCLIRQ_MSK_VSYNC_SC1_HVSP_FINISH)
    {
        SCL_DBG(SCL_DBG_LV_DRVSCLIRQ()&(EN_DBGMG_SCLIRQLEVEL_ELSE),"[HVSP_FINISH]Flag:%llx @%lu\n",u64Flag,gu32Time);
    }
    if(u64Flag &SCLIRQ_MSK_VSYNC_FCLK_LDC)
    {
        u32ActTime[5] = gu32Time;
        _Drv_SCLIRQ_LDCVsyncInRealTimeHandler();
        _Drv_SCLIRQ_LDCVsyncInHandler(&bISPFramePerfect,&bAffFull);
        SCL_DBG(SCL_DBG_LV_DRVSCLIRQ()&(EN_DBGMG_SCLIRQLEVEL_ELSE),"[FRMLDC]Flag:%llx @%lu\n",u64Flag,gu32Time);
    }
    // SC1 FRM
    if(u64Flag & SCLIRQ_MSK_SC1_FRM_W_ACTIVE)
    {
        u32ActTime[0] = gu32Time;
        _Drv_SCLIRQ_DMAActiveHandler(E_SCLDMA_1_FRM_W);
    }

    if(u64Flag & SCLIRQ_MSK_SC1_FRM_W_ACTIVE_N)
    {
        if(_Drv_SCLIRQ_DMAActive_NHandler(E_SCLDMA_1_FRM_W))
        {
            gsclirqstate |= (E_SCLIRQ_EVENT_SC1FRM|E_SCLIRQ_EVENT_ISTSC1FRM);
        }
        gstSCInts.u32SC1FrmDoneCount++;
        u32DiffTime = gu32Time - u32ActTime[0];
        gstSCInts.u32SC1FrmActiveTime += u32DiffTime;
        if(gstSCInts.u32SC1FrmDoneCount > 256)
        {
            gstSCInts.u32SC1FrmActiveTime = gstSCInts.u32SC1FrmActiveTime/gstSCInts.u32SC1FrmDoneCount;
            gstSCInts.u32SC1FrmDoneCount = 1;
        }
    }


    // SC2 FRM
    if(u64Flag & SCLIRQ_MSK_SC2_FRM_W_ACTIVE)
    {
        u32ActTime[1] = gu32Time;
        _Drv_SCLIRQ_DMAActiveHandler(E_SCLDMA_2_FRM_W);
    }

    if(u64Flag & SCLIRQ_MSK_SC2_FRM_W_ACTIVE_N)
    {
        if(_Drv_SCLIRQ_DMAActive_NHandler(E_SCLDMA_2_FRM_W))
        {
            gsclirqstate |= (E_SCLIRQ_EVENT_SC2FRM|E_SCLIRQ_EVENT_ISTSC2FRM);
        }
        gstSCInts.u32SC2FrmDoneCount++;
        u32DiffTime = gu32Time -u32ActTime[1];
        gstSCInts.u32SC2FrmActiveTime += u32DiffTime;
        if(gstSCInts.u32SC2FrmDoneCount > 256)
        {
            gstSCInts.u32SC2FrmActiveTime = gstSCInts.u32SC2FrmActiveTime/gstSCInts.u32SC2FrmDoneCount;
            gstSCInts.u32SC2FrmDoneCount = 1;
        }
    }
    // SC2 FRM2
    if(u64Flag & SCLIRQ_MSK_SC2_FRM2_W_ACTIVE)
    {
        u32ActTime[2] = gu32Time;
        _Drv_SCLIRQ_DMAActiveHandler(E_SCLDMA_2_FRM2_W);
    }
    if(u64Flag & SCLIRQ_MSK_SC2_FRM2_W_ACTIVE_N)
    {
        if(_Drv_SCLIRQ_DMAActive_NHandler(E_SCLDMA_2_FRM2_W))
        {
            MsOS_SetEvent(_s32FRMENDEventId, (E_SCLIRQ_EVENT_SC2FRM2|E_SCLIRQ_EVENT_ISTSC2FRM2));
        }
        else
        {
            if(gbsc2frame2reopen && (u64Flag & SCLIRQ_MSK_SC2_FRM2_W_ACTIVE))
            {
                gbsc2frame2reopen = 0;
                _Drv_SCLIRQ_ReduceDropFrameCount();
            }
        }
        // for HW bug
        //gsclirqstate |= (E_SCLIRQ_EVENT_SC2FRM2|E_SCLIRQ_EVENT_ISTSC2FRM2);
        if(gbsc2frame2reopen)
        {
            gbsc2frame2reopen = 0;
            _Drv_SCLIRQ_ReduceDropFrameCount();
            SCL_DBG(SCL_DBG_LV_DRVSCLIRQ()&(EN_DBGMG_SCLIRQLEVEL_ELSE),
                "[FRMEND]SC2 FRM2 reopen\n");
            _Drv_SCLIRQ_SetDMAOnOffWithoutDoubleBufferHandler();
            if((!gbDMADoneEarlyISP) || (gbPtgenMode))
            {
                _Drv_SCLIRQ_CMDQBlankingHandler();
            }
        }
        gstSCInts.u32SC2Frm2DoneCount++;
        u32DiffTime = gu32Time -u32ActTime[2];
        gstSCInts.u32SC2Frm2ActiveTime += u32DiffTime;
        if(gstSCInts.u32SC2Frm2DoneCount > 256)
        {
            gstSCInts.u32SC2Frm2ActiveTime = gstSCInts.u32SC2Frm2ActiveTime/gstSCInts.u32SC2Frm2DoneCount;
            gstSCInts.u32SC2Frm2DoneCount = 1;
        }
    }

    // SC1 SNP
    if(u64Flag & SCLIRQ_MSK_SC1_SNP_W_ACTIVE)
    {
        u32ActTime[3] = gu32Time;
        _Drv_SCLIRQ_DMAActiveHandler(E_SCLDMA_1_SNP_W);
    }

    if(u64Flag & SCLIRQ_MSK_SC1_SNP_W_ACTIVE_N)
    {
        if(_Drv_SCLIRQ_DMAActive_NHandler(E_SCLDMA_1_SNP_W))
        {
            gsclirqstate |= (E_SCLIRQ_EVENT_SC1SNP|E_SCLIRQ_EVENT_ISTSC1SNP);
        }
        gstSCInts.u32SC1SnpDoneCount++;
        u32DiffTime = gu32Time -u32ActTime[3];
        gstSCInts.u32SC1SnpActiveTime += u32DiffTime;
        if(gstSCInts.u32SC1SnpDoneCount > 256)
        {
            gstSCInts.u32SC1SnpActiveTime = gstSCInts.u32SC1SnpActiveTime/gstSCInts.u32SC1SnpDoneCount;
            gstSCInts.u32SC1SnpDoneCount = 1;
        }
    }
    // SC SRC FRMEND
    if(u64Flag &SCLIRQ_MSK_SC1_ENG_FRM_END)
    {
        Drv_SCLIRQ_SetRealTimeFrmEndHandler(gu32Time);
        Drv_SCLIRQ_SetFrmEndInterruptStatus(gu32Time);
        _Drv_SCLIRQ_IsAffFullContinually(&bAffFull, &bAffcount);
        _Drv_SCLIRQ_IsDMAHangUp(u64Flag);
        bNonFRMEndCount = 0;
        u32ActTime[6] = gu32Time;
        gu32FrmEndTime = gu32Time;
        u32DiffTime = gu32Time -u32ActTime[5];
        gstSCInts.u32SCLMainActiveTime += u32DiffTime;
        if(gstSCInts.u32ISPBlanking)
        {
            gstSCInts.u32SCLMainDoneCount++;
        }
        if(gstSCInts.u32SCLMainDoneCount > 256)
        {
            gstSCInts.u32ISPBlanking = gstSCInts.u32ISPBlankingTime;
            gstSCInts.u32SCLMainActiveTime = gstSCInts.u32SCLMainActiveTime/gstSCInts.u32SCLMainDoneCount;
            gstSCInts.u32SCLMainDoneCount = 1;
            gstSCInts.u8CountReset ++;
        }
        SCL_DBG(SCL_DBG_LV_DRVSCLIRQ()&(EN_DBGMG_SCLIRQLEVEL_ELSE),
            "[FRMEND]%x flag:%llx,%lu\n",gsclirqstate,u64Flag,((MS_U32)MsOS_GetSystemTimeStamp()));
    }

    // SC3 FRM_W
    if(u64Flag & SCLIRQ_MSK_SC3_DMA_W_ACTIVE)
    {
        gscl3irqstate = E_SCLIRQ_SC3EVENT_CLEAR;
        gscl3irqstate |= (E_SCLIRQ_SC3EVENT_ACTIVE | E_SCLIRQ_SC3EVENT_ISTACTIVE);
        _Drv_SCLIRQ_DMAActiveHandler(E_SCLDMA_3_FRM_W);
        u32ActTime[4] = gu32Time;
    }

    if(u64Flag & SCLIRQ_MSK_SC3_DMA_W_ACTIVE_N)
    {
        gscl3irqstate |= (E_SCLIRQ_SC3EVENT_ACTIVEN | E_SCLIRQ_SC3EVENT_ISTACTIVEN | E_SCLIRQ_SC3EVENT_IRQ);
        _Drv_SCLIRQ_DMAActive_NHandler(E_SCLDMA_3_FRM_W);
        gstSCInts.u32SC3DoneCount++;
        u32DiffTime = gu32Time -u32ActTime[4];
        gstSCInts.u32SC3ActiveTime += u32DiffTime;
        if(gstSCInts.u32SC3DoneCount > 256)
        {
            gstSCInts.u32SC3ActiveTime = gstSCInts.u32SC3ActiveTime/gstSCInts.u32SC3DoneCount;
            gstSCInts.u32SC3DoneCount = 1;
        }
    }
    Hal_SCLIRQ_Set_Clear(EN_SCLIRQ_SCTOP_0,(~u64Mask)&u64Flag, 0);
    u32DiffTime = ((MS_U32)MsOS_GetSystemTimeStamp())-gu32Time;
    SCL_DBG(SCL_DBG_LV_DRVSCLIRQ()&(EN_DBGMG_SCLIRQLEVEL_NORMAL),
        "[ISR]Diff:%lu(ns)\n",u32DiffTime);
    return IRQ_HANDLED;
}

#endif  //RTK

#endif //ENABLE_ACTIVEID_ISR


void Drv_SCLIRQ_SetDNRLock(MS_BOOL bEn)
{
    Hal_SCLIRQ_SetDNRLock(bEn);
}
void Drv_SCLIRQ_SetDropFrameFromCMDQDone(unsigned char u8Count)
{
    #if SCALING_BLOCK
    #else
    MS_U32 u32Events;
    if(Drv_SCLIRQ_GetEachDMAEn())
    {
        MsOS_WaitEvent(Drv_SCLIRQ_Get_IRQ_SYNCEventID(), E_SCLIRQ_EVENT_CMDQDONE, &u32Events, E_OR, 2000); // get status: CMDQ Done
    }
    #endif
    DRV_SCLIRQ_MUTEX_LOCK_ISR();
    if(gstDropFrameCount<=u8Count)
    {
        gstDropFrameCount = u8Count;
    }
    DRV_SCLIRQ_MUTEX_UNLOCK_ISR();
}
void Drv_SCLIRQ_SetDropFrameFromISP(unsigned char u8Count)
{
    SCL_ERR("[SCLIRQ]ISP drop Frame Count:%hhd \n",u8Count);
    DRV_SCLIRQ_MUTEX_LOCK_ISR();
    if(gstDropFrameCount<=u8Count)
    {
        gstDropFrameCount = u8Count;
    }
    DRV_SCLIRQ_MUTEX_UNLOCK_ISR();
}
void Drv_SCLIRQ_SetDAZAQueueWorkISR(SCLIRQDAZAEvent enEvent)
{
    if(!(genDAZAEvent&enEvent))
    {
        DRV_SCLIRQ_MUTEX_LOCK_ISR();
        genDAZAEvent |= enEvent;
        DRV_SCLIRQ_MUTEX_UNLOCK_ISR();
        MsOS_QueueWork(1,gstThreadCfg.s32Taskid[EN_SCLIRQ_SCTASKID_DAZA],
            gstThreadCfg.s32HandlerId[EN_SCLIRQ_SCTASK_WORKQUEUEDAZA],0);
    }
}
void Drv_SCLIRQ_SetDAZAQueueWork(SCLIRQDAZAEvent enEvent)
{
    if(!(genDAZAEvent&enEvent))
    {
        genDAZAEvent |= enEvent;
        MsOS_QueueWork(1,gstThreadCfg.s32Taskid[EN_SCLIRQ_SCTASKID_DAZA],
            gstThreadCfg.s32HandlerId[EN_SCLIRQ_SCTASK_WORKQUEUEDAZA],0);
    }
}
MS_BOOL Drv_SCLIRQ_InitVariable(void)
{
    MS_U64 u64Flag;
#if (ENABLE_ISR)
    MsOS_Memset(&gstSCLIrqInfo, 0, sizeof(ST_SCLIRQ_IFNO_TYPE));
#endif
    gsclirqstate        = E_SCLIRQ_EVENT_CLEAR;
    gscl3irqstate       = E_SCLIRQ_SC3EVENT_CLEAR;
    _bSCLIRQ_Suspend    = 0;
    gbFRMInFlag         = 0;
    gbPtgenMode         = 0;
    gbDMADoneEarlyISP   = 0;
    genDAZAEvent        = E_SCLIRQ_DAZA_OFF;
    gbMonitorCropMode   = 0;
    gbEachDMAEnable     = 0;
    gbBypassDNR = 0;
    gstDropFrameCount = 0;
    gbsc2frame2reopen = 0;
    gu32FrmEndTime = 0;
    MsOS_Memset(&gstDmaHang,0x0,sizeof(ST_SCLIRQ_DMAHANG_CNT));
    MsOS_Memset(&gstSCInts,0x0,sizeof(ST_SCLIRQ_SCINTS_TYPE));
    _Drv_SCLIRQ_SetIsBlankingRegion(1);

    MsOS_ClearEventIRQ(_s32SYNCEventId,0xFFFFFFFF);
    MsOS_ClearEventIRQ(_s32FRMENDEventId,0xFFFFFFFF);
    MsOS_ClearEventIRQ(_s32SC3EventId,0xFFFFFFFF);
    u64Flag = Hal_SCLIRQ_Get_Flag(EN_SCLIRQ_SCTOP_0,0xFFFFFFFFFF);
    Hal_SCLIRQ_Set_Clear(EN_SCLIRQ_SCTOP_0,u64Flag, 1);
    Hal_SCLIRQ_Set_Clear(EN_SCLIRQ_SCTOP_0,u64Flag, 0);
    return TRUE;
}
void Drv_SCLIRQ_SetPTGenStatus(MS_BOOL bPTGen)
{
    DRV_SCLIRQ_MUTEX_LOCK();
    gbPtgenMode  = bPTGen;
    if(bPTGen)
    {
        gbFRMInFlag = 0xF;
    }
    DRV_SCLIRQ_MUTEX_UNLOCK();
}
void Drv_SCLIRQ_SetDMADoneEarlyISP(MS_BOOL bDMADoneEarlyISP)
{
    // LDC patch open,crop open,
    gbDMADoneEarlyISP  = bDMADoneEarlyISP;
}

void Drv_SCLIRQ_Set_Checkcropflag(MS_BOOL bcheck)
{
    DRV_SCLIRQ_MUTEX_LOCK();
    gbMonitorCropMode  = bcheck;
    DRV_SCLIRQ_MUTEX_UNLOCK();
}
MS_U8 Drv_SCLIRQ_Get_Checkcropflag(void)
{
    return gbMonitorCropMode;
}
MS_U8 Drv_SCLIRQ_GetEachDMAEn(void)
{
    return gbEachDMAEnable;
}
MS_U8 Drv_SCLIRQ_GetISPIN(void)
{
    return ((MsOS_GetEvent(_s32SYNCEventId)&E_SCLIRQ_EVENT_LDCSYNC)? 1 : 0);
}
ST_SCLIRQ_SCINTS_TYPE* Drv_SCLIRQ_GetSCLInts(void)
{
    return &gstSCInts;
}

MS_BOOL Drv_SCLIRQ_Init(ST_SCLIRQ_INIT_CONFIG *pCfg)
{
    char mutx_word[] = {"_SCLIRQ_Mutex"};

    MS_U8 i;

    if(_SCLIRQ_Mutex != -1)
    {
        SCL_DBG(SCL_DBG_LV_DRVSCLIRQ()&EN_DBGMG_SCLIRQLEVEL_NORMAL, "[DRVSCLIRQ]%s(%d)::Already Done\n", __FUNCTION__, __LINE__);
        return TRUE;
    }

    if(MsOS_Init() == FALSE)
    {
        DRV_SCLIRQ_ERR(printf("[DRVSCLIRQ]%s(%d)::MsOS_Init Fail\n", __FUNCTION__, __LINE__));
        return FALSE;
    }

    _SCLIRQ_Mutex = MsOS_CreateMutex(E_MSOS_FIFO, mutx_word, MSOS_PROCESS_SHARED);
    _pSCLIRQ_SCLDMA_Mutex = Drv_SCLDMA_GetDMAandIRQCommonMutex();
    if (_SCLIRQ_Mutex == -1)
    {
        DRV_SCLIRQ_ERR(printf("[DRVSCLIRQ]%s(%d)::Create Mutex Flag Fail\n", __FUNCTION__, __LINE__));
        return FALSE;
    }

    Hal_SCLIRQ_Set_Riu_Base(pCfg->u32RiuBase);

    // disable all interrupt first
    for(i=0; i<SCLIRQ_NUM; i++)
    {
        Hal_SCLIRQ_Set_Mask(EN_SCLIRQ_SCTOP_0, ((MS_U64)1)<<i, 1);
        Hal_SCLIRQ_Set_Clear(EN_SCLIRQ_SCTOP_0, ((MS_U64)1)<<i, 1);
        Hal_SCLIRQ_Set_Mask(EN_SCLIRQ_SCTOP_1, ((MS_U64)1)<<i, 1);
        Hal_SCLIRQ_Set_Clear(EN_SCLIRQ_SCTOP_1, ((MS_U64)1)<<i, 1);
        Hal_SCLIRQ_Set_Mask(EN_SCLIRQ_SCTOP_2, ((MS_U64)1)<<i, 1);
        Hal_SCLIRQ_Set_Clear(EN_SCLIRQ_SCTOP_2, ((MS_U64)1)<<i, 1);
    }


    Drv_SCLIRQ_InitVariable();

#if (ENABLE_ISR|ENABLE_ACTIVEID_ISR)
    _s32FRMENDEventId   = MsOS_CreateEventGroup("FRMEND_Event");
    _s32SYNCEventId     = MsOS_CreateEventGroup("SYNC_Event");
    _s32SC3EventId      = MsOS_CreateEventGroup("SC3_Event");
    #if USE_RTK
    for(i=0;i<EN_SCLIRQ_SCIRQ_MAX;i++)
    {
        _s32IRQEventId[i] = MsOS_CreateEventGroupRing(i);
    }
    #endif
    gu32IRQNUM      = pCfg->u32IRQNUM;
    if(MsOS_AttachInterrupt((InterruptNum)pCfg->u32IRQNUM, (InterruptCb)_Drv_SCLIRQ_isr ,IRQF_DISABLED, "SCLINTR"))
    {
        DRV_SCLIRQ_ERR(printf("[DRVSCLIRQ]%s(%d):: Request IRQ Fail\n", __FUNCTION__, __LINE__));
        return FALSE;
    }
    MsOS_DisableInterrupt((InterruptNum)pCfg->u32IRQNUM);
    MsOS_EnableInterrupt((InterruptNum)pCfg->u32IRQNUM);
    _Create_SCLIRQ_IST();
#endif
#if (ENABLE_CMDQ_ISR)
    Drv_CMDQ_InitRIUBase(pCfg->u32RiuBase);
    if(MsOS_AttachInterrupt((InterruptNum)pCfg->u32CMDQIRQNUM, (InterruptCb)_Drv_SCLIRQ_CMDQ_isr ,NULL, "CMDQINTR"))
    {
        DRV_SCLIRQ_ERR(printf("[DRVSCLIRQ]%s(%d):: Request IRQ Fail\n", __FUNCTION__, __LINE__));
        return FALSE;
    }
    gu32CMDQIRQNUM      = pCfg->u32CMDQIRQNUM;
    MsOS_DisableInterrupt((InterruptNum)pCfg->u32CMDQIRQNUM);
    MsOS_EnableInterrupt((InterruptNum)pCfg->u32CMDQIRQNUM);
    Drv_CMDQ_SetISRStatus(1);
#endif


    return TRUE;
}
void * Drv_SCLIRQ_GetWaitQueueHead(MS_U32 enID)
{
    if(enID == E_SCLDMA_ID_3_W)
    {
        return MsOS_GetEventQueue(_s32SC3EventId);
    }
    else
    {
        return MsOS_GetEventQueue(_s32FRMENDEventId);
    }
}

void * Drv_SCLIRQ_GetSyncQueue(void)
{
        return MsOS_GetEventQueue(_s32SYNCEventId);
}

MS_S32 Drv_SCLIRQ_Get_IRQ_EventID(void)
{
    return _s32FRMENDEventId;
}
MS_S32 Drv_SCLIRQ_Get_IRQ_SYNCEventID(void)
{
    return _s32SYNCEventId;
}
MS_S32 Drv_SCLIRQ_Get_IRQ_SC3EventID(void)
{
    return _s32SC3EventId;
}

MS_BOOL Drv_SCLIRQ_Suspend(ST_SCLIRQ_SUSPEND_RESUME_CONFIG *pCfg)
{
    SCL_DBG(SCL_DBG_LV_DRVSCLIRQ()&EN_DBGMG_SCLIRQLEVEL_NORMAL, "%s(%d), bSuspend=%d\n", __FUNCTION__, __LINE__, _bSCLIRQ_Suspend);
    _bSCLIRQ_Suspend = 1;
    _Drv_SCLIRQ_SetIsBlankingRegion(1);
    return TRUE;
}

MS_BOOL Drv_SCLIRQ_Resume(ST_SCLIRQ_SUSPEND_RESUME_CONFIG *pCfg)
{
    MS_U16 i;
    SCL_DBG(SCL_DBG_LV_DRVSCLIRQ()&EN_DBGMG_SCLIRQLEVEL_NORMAL, "%s(%d), bResume=%d\n", __FUNCTION__, __LINE__, _bSCLIRQ_Suspend);

    if(_bSCLIRQ_Suspend == 1)
    {
        // disable all interrupt first
        for(i=0; i<SCLIRQ_NUM; i++)
        {
            Hal_SCLIRQ_Set_Mask(EN_SCLIRQ_SCTOP_0,((MS_U64)1)<<i, 1);
            Hal_SCLIRQ_Set_Clear(EN_SCLIRQ_SCTOP_0,((MS_U64)1)<<i, 1);
            Hal_SCLIRQ_Set_Mask(EN_SCLIRQ_SCTOP_1,((MS_U64)1)<<i, 1);
            Hal_SCLIRQ_Set_Clear(EN_SCLIRQ_SCTOP_1,((MS_U64)1)<<i, 1);
            Hal_SCLIRQ_Set_Mask(EN_SCLIRQ_SCTOP_2,((MS_U64)1)<<i, 1);
            Hal_SCLIRQ_Set_Clear(EN_SCLIRQ_SCTOP_2,((MS_U64)1)<<i, 1);
        }
        _bSCLIRQ_Suspend = 0;
    }

    return TRUE;
}

MS_BOOL Drv_SCLIRQ_Exit(void)
{
    MS_BOOL bRet = TRUE;
    MS_U64 u64IRQ;
    if(_SCLIRQ_Mutex != -1)
    {
        MsOS_DeleteMutex(_SCLIRQ_Mutex);
        _SCLIRQ_Mutex = -1;
    }
    else
    {
        return FALSE;
    }
    Drv_SCLIRQ_SetDNRLock(0);

#if (ENABLE_ISR)

    DRV_SCLIRQ_MUTEX_LOCK();
    u64IRQ = gstSCLIrqInfo.u64IRQ;
    DRV_SCLIRQ_MUTEX_UNLOCK();

    if( u64IRQ == 0)
    {
        MsOS_DisableInterrupt(INT_IRQ_SC_TOP);
        MsOS_DetachInterrupt(INT_IRQ_SC_TOP);
        bRet = TRUE;
    }
    else
    {
        DRV_SCLIRQ_ERR(printf("[DRVSCLIRQ]: IRQ no clear %llx\n", gstSCLIrqInfo.u64IRQ));
        bRet = FALSE;
    }
#elif ENABLE_ACTIVEID_ISR
    u64IRQ = Hal_SCLIRQ_Get_Flag(EN_SCLIRQ_SCTOP_0,0xFFFFFFFFFF);
    Hal_SCLIRQ_Set_Clear(EN_SCLIRQ_SCTOP_0,u64IRQ, 1);
    Hal_SCLIRQ_Set_Clear(EN_SCLIRQ_SCTOP_0,u64IRQ, 0);
    _Delete_SCLIRQ_IST();
    MsOS_DisableInterrupt((InterruptNum)gu32IRQNUM);
    MsOS_DetachInterrupt((InterruptNum)gu32IRQNUM);
#endif
#if (ENABLE_CMDQ_ISR)
    MsOS_DisableInterrupt((InterruptNum)gu32CMDQIRQNUM);
    MsOS_DetachInterrupt((InterruptNum)gu32CMDQIRQNUM);
    Drv_CMDQ_SetISRStatus(0);
#endif
    MsOS_Exit();

    return bRet;
}


MS_BOOL Drv_SCLIRQ_InterruptEnable(MS_U16 u16IRQ)
{
    MS_BOOL bRet = TRUE;
    if(u16IRQ < 48)
    {
#if (ENABLE_ISR)
        MsOS_ObtainMutex(_SCLIRQ_Mutex , MSOS_WAIT_FOREVER);
        gstSCLIrqInfo.u64IRQ |= ((MS_U64)1<<u16IRQ);
        DRV_SCLIRQ_MUTEX_UNLOCK();
#endif
        Hal_SCLIRQ_Set_Mask(EN_SCLIRQ_SCTOP_0,((MS_U64)1<<u16IRQ), 0);
        Hal_SCLIRQ_Set_Clear(EN_SCLIRQ_SCTOP_0,((MS_U64)1<<u16IRQ), 0);
        bRet = TRUE;
    }
    else
    {
        bRet = FALSE;
    }

    return bRet;
}

MS_BOOL Drv_SCLIRQ_Disable(MS_U16 u16IRQ)
{
    MS_BOOL bRet = TRUE;

    if(u16IRQ < 48)
    {
#if (ENABLE_ISR)

        DRV_SCLIRQ_MUTEX_LOCK();
        gstSCLIrqInfo.u64IRQ &= ~((MS_U64)1<<u16IRQ);
        DRV_SCLIRQ_MUTEX_UNLOCK();
#endif
        Hal_SCLIRQ_Set_Mask(EN_SCLIRQ_SCTOP_0,((MS_U64)1<<u16IRQ), 1);
        Hal_SCLIRQ_Set_Clear(EN_SCLIRQ_SCTOP_0,((MS_U64)1<<u16IRQ), 1);
        bRet = TRUE;
    }
    else
    {
        bRet = FALSE;
    }
    return bRet;
}



MS_BOOL Drv_SCLIRQ_Get_Flag(MS_U16 u16IRQ, MS_U64 *pFlag)
{
    MS_BOOL bRet = TRUE;
    if(u16IRQ < 48)
    {
#if (ENABLE_ISR)

        DRV_SCLIRQ_MUTEX_LOCK();
        *pFlag = gstSCLIrqInfo.u64Flag & (((MS_U64)1)<<u16IRQ) ? 1 : 0;
        DRV_SCLIRQ_MUTEX_UNLOCK();
#else
        *pFlag = Hal_SCLIRQ_Get_Flag(EN_SCLIRQ_SCTOP_0,((MS_U64)1<<u16IRQ));
#endif
        bRet = TRUE;
    }
    else
    {
        bRet = FALSE;
    }
    return bRet;
}

MS_BOOL Drv_SCLIRQ_Set_Clear(MS_U16 u16IRQ)
{
    MS_BOOL bRet = TRUE;
    if(u16IRQ < 48)
    {
#if (ENABLE_ISR)
        DRV_SCLIRQ_MUTEX_LOCK();
        gstSCLIrqInfo.u64Flag &= ~(((MS_U64)1)<<u16IRQ);
        DRV_SCLIRQ_MUTEX_UNLOCK();
#else
        //DRV_SCLIRQ_ERR(printf("[DRVSCLIRQ]: IRQ clear %hx\n", u16IRQ));
        Hal_SCLIRQ_Set_Clear(EN_SCLIRQ_SCTOP_0,(MS_U64)1<<u16IRQ, 1);
        Hal_SCLIRQ_Set_Clear(EN_SCLIRQ_SCTOP_0,(MS_U64)1<<u16IRQ, 0);
#endif
    }
    else
    {
        bRet = FALSE;
    }
    return bRet;
}

MS_BOOL Drv_SCLIRQ_Set_Mask(MS_U64 u64IRQ)
{
    MS_BOOL bRet = TRUE;
    Hal_SCLIRQ_Set_Mask(EN_SCLIRQ_SCTOP_0,u64IRQ,1);
    return bRet;
}

MS_BOOL Drv_SCLIRQ_Get_Flags_Msk(MS_U64 u64IrqMsk, MS_U64 *pFlags)
{
#if (ENABLE_ISR)

    DRV_SCLIRQ_MUTEX_LOCK();
    *pFlags = gstSCLIrqInfo.u64Flag & u64IrqMsk;
    DRV_SCLIRQ_MUTEX_UNLOCK();
#else
    *pFlags = Hal_SCLIRQ_Get_Flag(EN_SCLIRQ_SCTOP_0,u64IrqMsk);
#endif
    return TRUE;
}


MS_BOOL Drv_SCLIRQ_Set_Clear_Msk(MS_U64 u64IrqMsk)
{
#if ENABLE_ISR
    DRV_SCLIRQ_MUTEX_LOCK();
    gstSCLIrqInfo.u64Flag &= ~(((MS_U64)1)<<u64IrqMsk);
    DRV_SCLIRQ_MUTEX_UNLOCK();
#else
    //DRV_SCLIRQ_ERR(printf("[DRVSCLIRQ]: IRQ clear %hx\n", u16IRQ));
    Hal_SCLIRQ_Set_Clear(EN_SCLIRQ_SCTOP_0,u64IrqMsk, 1);
    Hal_SCLIRQ_Set_Clear(EN_SCLIRQ_SCTOP_0,u64IrqMsk, 0);
#endif
    return TRUE;
}

#undef DRV_SCLIRQ_C
