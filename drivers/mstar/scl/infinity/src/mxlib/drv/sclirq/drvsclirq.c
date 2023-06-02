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
#include <linux/wait.h>
#include <linux/irqreturn.h>
#include <asm/div64.h>
#endif

//-------------------------------------------------------------------------------------------------
//  Include Files
//-------------------------------------------------------------------------------------------------
#include "MsCommon.h"
#include "MsTypes.h"
#include "MsOS.h"
#include "irqs.h"
#include <linux/kthread.h>

#include "drvsclirq.h"
#include "mdrv_scl_dbg.h"
#include "drvsclirq_st.h"
#include "drvscldma_st.h"
#include "halscldma.h"
#include "drvscldma.h"
#include "regCMDQ.h"
#include "drvCMDQ.h"
#include "halsclirq.h"
//-------------------------------------------------------------------------------------------------
//  Defines & Macro
//-------------------------------------------------------------------------------------------------
#define DRV_SCLIRQ_DBG(x)
#define DRV_SCLIRQ_DBG_H(x)
#define DRV_SCLIRQ_ERR(x)      x
#define freerunID               5
#define ENABLE_ISR              0
#define ENABLE_ACTIVEID_ISR     1
#define ENABLE_CMDQ_ISR         1
#define _Is_SCLDMA_RingMode(enClientType)            (gstScldmaInfo.enBuffMode[(enClientType)] == E_SCLDMA_BUF_MD_RING)
#define _Is_SCLDMA_SingleMode(enClientType)         (gstScldmaInfo.enBuffMode[(enClientType)] == E_SCLDMA_BUF_MD_SINGLE)
#define _Is_SCLDMA_SWRingMode(enClientType)         (gstScldmaInfo.enBuffMode[(enClientType)] == E_SCLDMA_BUF_MD_SWRING)
#define _IsFrmIN()                                  (gbFRMInFlag & (E_SCLIRQ_FRM_IN_COUNT_NOW << SCL_DELAYFRAME_FROM_ISP))
#define _IsFrmIN_Now()                                   (gbFRMInFlag & (E_SCLIRQ_FRM_IN_COUNT_NOW ))
#define DRV_SCLIRQ_MUTEX_LOCK_ISR()        MsOS_ObtainMutex_IRQ(*_pSCLIRQ_SCLDMA_Mutex)
#define DRV_SCLIRQ_MUTEX_UNLOCK_ISR()        MsOS_ReleaseMutex_IRQ(*_pSCLIRQ_SCLDMA_Mutex)
#define DRV_SCLIRQ_MUTEX_LOCK()             MsOS_ObtainMutex(_SCLIRQ_Mutex , MSOS_WAIT_FOREVER)
#define DRV_SCLIRQ_MUTEX_UNLOCK()             MsOS_ReleaseMutex(_SCLIRQ_Mutex , MSOS_WAIT_FOREVER)
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

/////////////////
/// gstThreadCfg
/// IST thread
////////////////
ST_SCLDMA_THREAD_CONFIG gstThreadCfg;

/////////////////
/// gstSc3ThreadCfg
/// SC3 IST thread
////////////////
ST_SCLDMA_THREAD_CONFIG gstSc3ThreadCfg;

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
MS_BOOL gbInBlanking;

#if (ENABLE_ISR)
ST_SCLIRQ_IFNO_TYPE gstSCLIrqInfo;
#endif
//-------------------------------------------------------------------------------------------------
//  Functions
//-------------------------------------------------------------------------------------------------
#if (ENABLE_CMDQ_ISR)
irqreturn_t  _Drv_SCLIRQ_CMDQ_isr(int eIntNum, void* dev_id)
{
    MS_U32 u32IrqFlag;
    MS_U32 u32Time;
    u32IrqFlag = Drv_CMDQ_GetFinalIrq(EN_CMDQ_TYPE_IP0);
    Drv_CMDQ_ClearIrqByFlag(EN_CMDQ_TYPE_IP0,0xFFFF);
    u32Time = ((MS_U32)MsOS_GetSystemTime());
    if(u32IrqFlag & CMDQ_IRQ_STATE_TRIG)
    {
        MsOS_ClearEvent(_s32SYNCEventId,E_SCLIRQ_EVENT_CMDQ);
        MsOS_SetEvent(_s32SYNCEventId, E_SCLIRQ_EVENT_CMDQGOING);
    }
    if(u32IrqFlag & CMDQ_IRQ_STATE_DONE)
    {
        MsOS_SetEvent(_s32SYNCEventId, E_SCLIRQ_EVENT_CMDQDONE);
        Drv_CMDQ_SetRPoint(EN_CMDQ_TYPE_IP0);
    }
    SCL_DBG(SCL_DBG_LV_DRVCMDQ()&EN_DBGMG_CMDQEVEL_ISR, "[CMDQ]u32IrqFlag:%lx  %lu\n",u32IrqFlag,u32Time);
    return IRQ_HANDLED;
}
#endif
void _Drv_SCLIRQ_SclFrameEndConnectToISP(void)
{
    scl_ve_isr();
}
#if ENABLE_ISR
irqreturn_t  _Drv_SCLIRQ_isr(int eIntNum, void* dev_id)
{
    MS_U64 u64Flag = Hal_SCLIRQ_Get_Flag(0xFFFFFFFFFF);

    Hal_SCLIRQ_Set_Clear(u64Flag, 1);

    Hal_SCLIRQ_Set_Clear(u64Flag, 0);

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
        default :
            return 0;
    }
}
void _Drv_SCLIRQ_SetIsBlankingRegion(MS_BOOL bBk)
{
    gbInBlanking = bBk;
}
MS_BOOL Drv_SCLIRQ_GetIsBlankingRegion(void)
{
    return gbInBlanking;
}
static void Drv_SCLIRQ_RingMode_Active(EN_SCLDMA_CLIENT_TYPE enClient)
{
    MS_U8 u8RPoint = 0;
    MS_U32 u32Time;
    MS_U16  u16RealIdx;
    u32Time = ((MS_U32)MsOS_GetSystemTime());
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
        "[DRVSCLDMA]%d ISR R_P=%hhx RealIdx=%hx flag:%hhd @:%lu\n"
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
    SCL_DBG(SCL_DBG_LV_DRVSCLIRQ()&Get_DBGMG_SCLIRQclient(enclient,1), "[DRVSCLDMA]%d ISR actN flag:%hhd idx:%hhd @:%lu\n"
    ,enclient,gstScldmaInfo.bDmaflag[enclient],gstScldmaInfo.bDMAidx[enclient],((MS_U32)MsOS_GetSystemTime()));
    //printf("ACTIVE_N:%lu\n",u32Time);
}
static void Drv_SCLIRQ_RingMode_ActiveWithoutDoublebuffer(EN_SCLDMA_CLIENT_TYPE enClient)
{
    MS_U8 u8RPoint = 0;
    MS_U32 u32Time;
    MS_U16  u16RealIdx;
    u32Time = ((MS_U32)MsOS_GetSystemTime());
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
    _SetANDGetIdxType(enClient,u16RealIdx,E_SCLDMA_ACTIVE_BUFFER_OMX);
    SCL_DBG(SCL_DBG_LV_DRVSCLIRQ()&Get_DBGMG_SCLIRQclient(enClient,0),
        "[DRVSCLDMA]%d wodb ISR R_P=%hhx RealIdx=%hx flag:%hhd @:%lu\n"
    ,enClient,u8RPoint,u16RealIdx,gstScldmaInfo.bDmaflag[enClient],u32Time);
}
static void Drv_SCLIRQ_RingMode_Active_NWithoutDoublebuffer(EN_SCLDMA_CLIENT_TYPE enClient)
{
    MS_U8 u8RPoint = 0;
    MS_U16  u16RealIdx;
    u8RPoint = (_GetIdxType(enClient,E_SCLDMA_ACTIVE_BUFFER_OMX))>>4;
    u16RealIdx = _GetIdxType(enClient,E_SCLDMA_ACTIVE_BUFFER_SCL);
    if(u8RPoint==u16RealIdx && !_IsFlagType(enClient,E_SCLDMA_FLAG_BLANKING))//5 is debug freerun
    {
        //trig off
        if(u8RPoint != freerunID )
        {
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
    SCL_DBG(SCL_DBG_LV_DRVSCLIRQ()&Get_DBGMG_SCLIRQclient(enClient,1), "[DRVSCLDMA]%d ISR actN wodb flag:%hhd idx:%hhd @:%lu\n"
    ,enClient,gstScldmaInfo.bDmaflag[enClient],gstScldmaInfo.bDMAidx[enClient],((MS_U32)MsOS_GetSystemTime()));
    //printf("ACTIVE_N:%lu\n",u32Time);
}
static void Drv_SCLIRQ_SWRingMode_Active(EN_SCLDMA_CLIENT_TYPE enclient)
{
    MS_U32 u32Time;
    MS_U16  u16RealIdx;
    u32Time = ((MS_U32)MsOS_GetSystemTime());
    if(_IsFlagType(enclient,E_SCLDMA_FLAG_BLANKING))
    {
        u16RealIdx = 0;
    }
    else
    {
        u16RealIdx = Drv_SCLDMA_GetActiveBufferIdx(enclient);
    }
    _SetANDGetIdxType(enclient,u16RealIdx,E_SCLDMA_ACTIVE_BUFFER_OMX);
    _SetANDGetFlagType(enclient,E_SCLDMA_FLAG_ACTIVE,(~E_SCLDMA_FLAG_FRMDONE));
    SCL_DBG(SCL_DBG_LV_DRVSCLIRQ()&Get_DBGMG_SCLIRQclient(enclient,0), "[DRVSCLDMA]%d ISR SWRING flag:%hhd ACT:%hhd@:%lu\n"
    ,enclient,gstScldmaInfo.bDmaflag[enclient],u16RealIdx,u32Time);
}
static void Drv_SCLIRQ_SWRingMode_Active_N(EN_SCLDMA_CLIENT_TYPE enclient)
{
    ST_SCLDMA_FRAME_BUFFER_CONFIG stTarget;
    MS_U64 u64Time;
    u64Time = ((MS_U64)Chip_Get_US_Ticks());
    _SetANDGetFlagType(enclient,E_SCLDMA_FLAG_FRMDONE,(~E_SCLDMA_FLAG_ACTIVE));
    _ReSetFlagType(enclient,E_SCLDMA_FLAG_ACTIVE);
    stTarget.u8FrameAddrIdx = _GetIdxType(enclient,E_SCLDMA_ACTIVE_BUFFER_SCL);
    stTarget.u32FrameAddr   = gstScldmaInfo.u32Base_Y[enclient][stTarget.u8FrameAddrIdx];
    stTarget.u64FRMDoneTime = u64Time;
    stTarget.u16FrameWidth   = gstScldmaInfo.u16FrameWidth[enclient];
    stTarget.u16FrameHeight  = gstScldmaInfo.u16FrameHeight[enclient];
    if(Drv_SCLDMA_MakeSureNextActiveId(enclient))
    {
        if(Drv_SCLDMA_BufferEnQueue(enclient,stTarget))
        {
            Drv_SCLDMA_ChangeBufferIdx(enclient);
        }
    }
    if(_IsFlagType(enclient,E_SCLDMA_FLAG_BLANKING))
    {
        _ReSetFlagType(enclient,E_SCLDMA_FLAG_BLANKING);
    }
    SCL_DBG(SCL_DBG_LV_DRVSCLIRQ()&Get_DBGMG_SCLIRQclient(enclient,1), "[DRVSCLDMA]%d ISR SWRING actN flag:%hhd idx:%hhd\n"
    ,enclient,gstScldmaInfo.bDmaflag[enclient],gstScldmaInfo.bDMAidx[enclient]);
}
static void Drv_SCLIRQ_SC3SingleMode_Active(void)
{
    SCL_DBG(SCL_DBG_LV_DRVSCLIRQ()&EN_DBGMG_SCLIRQLEVEL_SC3SINGLE, "[DRVSCLIRQ]SC3 single act\n");
}
static void Drv_SCLIRQ_SC3SingleMode_Active_N(void)
{
    Drv_SCLDMA_SetISRHandlerDMAOff(E_SCLDMA_3_FRM_W,0);
    _SetANDGetFlagType(E_SCLDMA_3_FRM_W,E_SCLDMA_FLAG_DMAOFF,~(E_SCLDMA_FLAG_EVERDMAON));
    SCL_DBG(SCL_DBG_LV_DRVSCLIRQ()&EN_DBGMG_SCLIRQLEVEL_SC3SINGLE, "[DRVSCLIRQ]SC3 single actN\n");
}
static void Drv_SCLIRQ_SingleMode_Active(EN_SCLDMA_CLIENT_TYPE enclient)
{
    MS_U32 u32Time;
    _SetANDGetFlagType(enclient,E_SCLDMA_FLAG_ACTIVE,(~E_SCLDMA_FLAG_FRMDONE));
    if(enclient< E_SCLDMA_3_FRM_R)
    {
        Drv_SCLDMA_SetISRHandlerDMAOff(enclient,0);
    }
    u32Time = ((MS_U32)MsOS_GetSystemTime());
    SCL_DBG(SCL_DBG_LV_DRVSCLIRQ()&EN_DBGMG_SCLIRQLEVEL_SC1SINGLE, "[DRVSCLIRQ]single act:%lu\n",u32Time);
}
static void Drv_SCLIRQ_SingleMode_Active_N(EN_SCLDMA_CLIENT_TYPE enclient)
{
    MS_U32 u32Time;
    u32Time = ((MS_U32)MsOS_GetSystemTime());
    _SetANDGetFlagType(enclient,E_SCLDMA_FLAG_FRMDONE,(~E_SCLDMA_FLAG_ACTIVE));
    SCL_DBG(SCL_DBG_LV_DRVSCLIRQ()&EN_DBGMG_SCLIRQLEVEL_SC1SINGLE, "[DRVSCLIRQ]single actN:%lu\n",u32Time);
}
static void Drv_SCLIRQ_SingleMode_ActiveWithoutDoublebuffer(EN_SCLDMA_CLIENT_TYPE enclient)
{
    MS_U32 u32Time;
    _SetANDGetFlagType(enclient,E_SCLDMA_FLAG_ACTIVE,(~E_SCLDMA_FLAG_FRMDONE));
    u32Time = ((MS_U32)MsOS_GetSystemTime());
    SCL_DBG(SCL_DBG_LV_DRVSCLIRQ()&EN_DBGMG_SCLIRQLEVEL_SC1SINGLE, "[DRVSCLIRQ]singlewodb act:%lu\n",u32Time);
}
static void Drv_SCLIRQ_SingleMode_Active_NWithoutDoublebuffer(EN_SCLDMA_CLIENT_TYPE enclient)
{
    MS_U32 u32Time;
    u32Time = ((MS_U32)MsOS_GetSystemTime());
    _SetANDGetFlagType(enclient,E_SCLDMA_FLAG_FRMDONE,(~E_SCLDMA_FLAG_ACTIVE));
    if(enclient< E_SCLDMA_3_FRM_R)
    {
        Drv_SCLDMA_SetISRHandlerDMAOff(enclient,0);
    }
    SCL_DBG(SCL_DBG_LV_DRVSCLIRQ()&EN_DBGMG_SCLIRQLEVEL_SC1SINGLE, "[DRVSCLIRQ]singlewodb actN:%lu\n",u32Time);
}
static void _Drv_SCLIRQ_SetFrameInFlag(EN_SCLDMA_CLIENT_TYPE enclient)
{
    if(_IsFrmIN())
    {
        _SetFlagType(enclient,E_SCLDMA_FLAG_FRMIN);
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
void _Drv_SCLIRQ_SetFrmEndEvent(void)
{
    if(!gbEachDMAEnable)
    {
        gbEachDMAEnable = 1;
    }
    if(gsclirqstate)
    {
        gsclirqstate |= (E_SCLIRQ_EVENT_IRQ|E_SCLIRQ_EVENT_HVSPST);
        MsOS_SetEvent(_s32FRMENDEventId, gsclirqstate);
        if(gsclirqstate &E_SCLIRQ_EVENT_SC1FRM && (gbMonitorCropMode == 2))
        {
            MsOS_SetEvent(_s32SYNCEventId, E_SCLIRQ_EVENT_SYNC);
        }
    }
}
void _Drv_SCLIRQ_SetDMAOnOffWithoutDoubleBuffer(EN_SCLDMA_CLIENT_TYPE enclient)
{
    if ((_IsFlagType(enclient,E_SCLDMA_FLAG_NEXT_ON)))
    {
        _ReSetFlagType(enclient,E_SCLDMA_FLAG_NEXT_ON);
        Drv_SCLDMA_SetISRHandlerDMAOff(enclient,1);
    }
}
void _Drv_SCLIRQ_SetDMAOnOffWithoutDoubleBufferHandler(void)
{
    _Drv_SCLIRQ_SetDMAOnOffWithoutDoubleBuffer(E_SCLDMA_1_FRM_W);
    _Drv_SCLIRQ_SetDMAOnOffWithoutDoubleBuffer(E_SCLDMA_1_SNP_W);
    _Drv_SCLIRQ_SetDMAOnOffWithoutDoubleBuffer(E_SCLDMA_2_FRM_W);
}
static void Drv_SCLIRQ_SetFrmEndInterruptStatus(MS_U32 u32Time)
{
    _Drv_SCLIRQ_MonitorSystemTimer();
    Drv_SCLDMA_SetISPFrameCount();
    Drv_SCLDMA_SetSclFrameDoneTime(u32Time);
    _Drv_SCLIRQ_SetFrmEndEvent();
    _Drv_SCLIRQ_SclFrameEndConnectToISP();
    _Drv_SCLIRQ_SetIsBlankingRegion(1);
    _Drv_SCLIRQ_SetDMAOnOffWithoutDoubleBufferHandler();
}

void _Drv_SCLIRQ_HandlerFRMIN(MS_BOOL bEn)
{
    if(!gbPtgenMode)
    {
        gbFRMInFlag = (MS_BOOL)((gbFRMInFlag << 1)|bEn);
    }
}

MS_BOOL _Delete_SCLIRQ_IST(void)
{
    gstThreadCfg.flag = 0;
    gstSc3ThreadCfg.flag = 0;
    if(gstThreadCfg.sttask.pThread)
    {
        kthread_stop(gstThreadCfg.sttask.pThread);
        gstThreadCfg.sttask.pThread = NULL;
        DRV_SCLIRQ_ERR(printf("[DRVSCLIRQ]stop thread\n"));
    }
    if(gstSc3ThreadCfg.sttask.pThread)
    {
        kthread_stop(gstSc3ThreadCfg.sttask.pThread);
        gstSc3ThreadCfg.sttask.pThread = NULL;
        DRV_SCLIRQ_ERR(printf("[DRVSCLIRQ]stop SC3 thread\n"));
    }
    return 0;
}
//To Do:clear single mode irq
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
static int SCLIRQ_IST(void *arg)
{
    MS_U32 u32Events = 0;
    SCL_DBG(SCL_DBG_LV_DRVSCLIRQ()&EN_DBGMG_SCLIRQLEVEL_ELSE, "[DRVSCLIRQ]%s:going\n",__FUNCTION__);
    while(gstThreadCfg.flag == 1)
    {
        MsOS_WaitEvent(_s32FRMENDEventId, E_SCLIRQ_EVENT_IRQ, &u32Events, E_OR, MSOS_WAIT_FOREVER); // get status: FRM END
        u32Events = MsOS_GetEvent(_s32FRMENDEventId);//get now ir
        SCL_DBG(SCL_DBG_LV_DRVSCLIRQ()&EN_DBGMG_SCLIRQLEVEL_ELSE, "[DRVSCLIRQ]%s:u32Events:%lx\n",__FUNCTION__,u32Events);
        _Drv_SCLIRQ_SetSingleModeDMAInfo(E_SCLDMA_1_FRM_W,u32Events);
        _Drv_SCLIRQ_SetSingleModeDMAInfo(E_SCLDMA_1_SNP_W,u32Events);
        _Drv_SCLIRQ_SetSingleModeDMAInfo(E_SCLDMA_2_FRM_W,u32Events);
        MsOS_ClearEventIRQ(_s32FRMENDEventId,(E_SCLIRQ_EVENT_IRQ|E_SCLIRQ_EVENT_ISTFRMEND));
    }

   return 0;

}

//To Do:do SC3 irq Bottom
static int SCLIRQ_SC3IST(void *arg)
{
    MS_U32 u32Events = 0;
    SCL_DBG(SCL_DBG_LV_DRVSCLIRQ()&EN_DBGMG_SCLIRQLEVEL_ELSE, "[DRVSCLIRQ]%s:going\n",__FUNCTION__);
    while(gstSc3ThreadCfg.flag == 1)
    {
        MsOS_WaitEvent(_s32SC3EventId, E_SCLIRQ_SC3EVENT_IRQ, &u32Events, E_OR, MSOS_WAIT_FOREVER); // get status: FRM END
        u32Events = MsOS_GetEvent(_s32SC3EventId);//get now ir
        SCL_DBG(SCL_DBG_LV_DRVSCLIRQ()&EN_DBGMG_SCLIRQLEVEL_ELSE, "[DRVSCLIRQ]%s:u32Events:%lx\n",__FUNCTION__,u32Events);
        if(u32Events & E_SCLIRQ_SC3EVENT_ISTACTIVE)
        {
            if(_Is_SCLDMA_RingMode(E_SCLDMA_3_FRM_W))
            {
            }
            else if(_Is_SCLDMA_SingleMode(E_SCLDMA_3_FRM_W))
            {
                DRV_SCLIRQ_MUTEX_LOCK_ISR();
                Drv_SCLIRQ_SingleMode_Active(E_SCLDMA_3_FRM_W);
                DRV_SCLIRQ_MUTEX_UNLOCK_ISR();
            }
        }

        if(u32Events & E_SCLIRQ_SC3EVENT_ISTACTIVEN)
        {
            if(_Is_SCLDMA_RingMode(E_SCLDMA_3_FRM_W))
            {
            }
            else if(_Is_SCLDMA_SingleMode(E_SCLDMA_3_FRM_W))
            {
                Drv_SCLIRQ_SingleMode_Active_N(E_SCLDMA_3_FRM_W);
                _Drv_SCLIRQ_SetSingleModeDMAInfo(E_SCLDMA_3_FRM_W,(SCLIRQ_ISTEVENT_BASE << E_SCLDMA_3_FRM_W));
                _Drv_SCLIRQ_SetSingleModeDMAInfo(E_SCLDMA_3_FRM_R,(SCLIRQ_ISTEVENT_BASE << E_SCLDMA_3_FRM_R));
            }
        }
        if(u32Events & E_SCLIRQ_SC3EVENT_ISTDONE)
        {
            MsOS_ClearEventIRQ(_s32SC3EventId,u32Events &(E_SCLIRQ_SC3EVENT_ISTCLEAR));
            MsOS_SetEvent_IRQ(_s32SC3EventId, (E_SCLIRQ_SC3EVENT_DONE |E_SCLIRQ_SC3EVENT_HVSPST));
        }
    }

   return 0;

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
MS_BOOL _Create_SCLIRQ_IST(void)
{
    char *pName={"SCLIRQ_THREAD"};
    char *pSc3Name={"SCLIRQ_SC3THREAD"};
    unsigned char bRet = 0;
    gstThreadCfg.s32Taskid  = MsOS_CreateTask((TaskEntry)SCLIRQ_IST,(MS_U32)&gstThreadCfg,TRUE,pName);
    gstSc3ThreadCfg.s32Taskid  = MsOS_CreateTask((TaskEntry)SCLIRQ_SC3IST,(MS_U32)&gstSc3ThreadCfg,TRUE,pSc3Name);
    gstThreadCfg.sttask    = MsOS_GetTaskinfo(gstThreadCfg.s32Taskid);
    gstSc3ThreadCfg.sttask = MsOS_GetTaskinfo(gstSc3ThreadCfg.s32Taskid);
    MsOS_SetUserNice(gstSc3ThreadCfg.sttask,-9);
    SCL_DBG(SCL_DBG_LV_DRVSCLIRQ()&EN_DBGMG_SCLIRQLEVEL_NORMAL,
        "[DRVSCLDMA]SC3 nice :%d Sc1:%d\n",
        MsOS_GetUserNice(gstSc3ThreadCfg.sttask),
        MsOS_GetUserNice(gstThreadCfg.sttask));
    bRet = _Drv_SCLIRQ_ISTWork(&gstThreadCfg);
    bRet = _Drv_SCLIRQ_ISTWork(&gstSc3ThreadCfg);
    return bRet;
}
void _Drv_SCLIRQ_AffFullHandler(MS_U32 u32Time ,MS_BOOL *bAffFull ,MS_BOOL *bAffcount)
{
    if(!(*bAffFull))
    {
        printf("SC AFF FULL @:%lu\n",u32Time);
        *bAffFull = 1;
    }
    if((*bAffcount)>200)
    {
        *bAffFull = 0;
        *bAffcount = 0;
    }
}
void _Drv_SCLIRQ_IsAffFullContinue(MS_BOOL *bAffFull ,MS_BOOL *bAffcount)
{
    if((*bAffFull))
    {
        (*bAffcount)++;
    }
}
void _Drv_SCLIRQ_FrameInEndHandler(MS_BOOL *bNonFRMEndCount,MS_BOOL *bISPFramePerfect)
{
    Drv_SCLDMA_SetISPFrameCount();
    *bISPFramePerfect = 1;
    if(gbEachDMAEnable)
    {
        (*bNonFRMEndCount)++;
        if((*bNonFRMEndCount)>3)
        {
            gbEachDMAEnable = 0;
        }
    }
}
void _Drv_SCLIRQ_LDCVsyncInHandler(MS_BOOL *bISPFramePerfect)
{
    if(!(*bISPFramePerfect))
    {
        _Drv_SCLIRQ_HandlerFRMIN(0);
    }
    else
    {
        _Drv_SCLIRQ_HandlerFRMIN(1);
        *bISPFramePerfect = 0;
    }
    gsclirqstate = E_SCLIRQ_EVENT_CLEAR;
    if(gbEachDMAEnable)
    {
        _Drv_SCLIRQ_SetIsBlankingRegion(0);
    }
    if(gbMonitorCropMode == 1)
    {
        MsOS_SetEvent(_s32SYNCEventId, E_SCLIRQ_EVENT_SYNC);
    }
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
}
void _Drv_SCLIRQ_DMAActive_NHandler(EN_SCLDMA_CLIENT_TYPE enclient)
{
    if(_Is_SCLDMA_RingMode(enclient) && DoubleBufferStatus)
    {
        Drv_SCLIRQ_RingMode_Active_N(enclient);
    }
    else if(_Is_SCLDMA_RingMode(enclient))
    {
        Drv_SCLIRQ_RingMode_Active_NWithoutDoublebuffer(enclient);
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
        Drv_SCLIRQ_SingleMode_Active_NWithoutDoublebuffer(enclient);
    }
    if(enclient < E_SCLDMA_3_FRM_R)
    {
        _Drv_SCLIRQ_SetFrameInFlag(enclient);
    }
}
irqreturn_t  _Drv_SCLIRQ_isr(int eIntNum, void* dev_id)
{
    MS_U64 u64Flag,u64Mask=0x0;
    MS_U32 u32Time;
    static MS_BOOL bISPFramePerfect = 0;
    static MS_BOOL bNonFRMEndCount = 0;
    static MS_BOOL bAffFull = 1,bAffcount=0;
    u64Mask = gstScldmaInfo.u64mask;
    u64Flag = Hal_SCLIRQ_Get_Flag(0xFFFFFFFFFF);
    if(_bSCLIRQ_Suspend)
    {
        Hal_SCLIRQ_Set_Clear(0xFFFFFFFFFF, 1);
    }

    Hal_SCLIRQ_Set_Clear((~u64Mask)&u64Flag, 1);
    u32Time = ((MS_U32)MsOS_GetSystemTime());
    if(u64Flag &SCLIRQ_MSK_AFF_FULL)
    {
        _Drv_SCLIRQ_AffFullHandler(u32Time, &bAffFull, &bAffcount);
    }
    // next frame IN . because of delay 1 frame.
    // 1.frame coming
    // 2.frame receive done.(irq get)keep in DNR
    // 3.active from dnr
    if(u64Flag &SCLIRQ_MSK_SC_IN_FRM_END)
    {
        _Drv_SCLIRQ_FrameInEndHandler(&bNonFRMEndCount,&bISPFramePerfect);
        SCL_DBG(SCL_DBG_LV_DRVSCLIRQ()&(EN_DBGMG_SCLIRQLEVEL_ELSE),"[FRMIN]@%lu\n",MsOS_GetSystemTime());
    }
    if(u64Flag &SCLIRQ_MSK_VSYNC_SC1_HVSP_FINISH)
    {
        //printf("HVSP:%lu,%ld\n",u32Time,count);
    }
    if(u64Flag &SCLIRQ_MSK_VSYNC_FCLK_LDC)
    {
        _Drv_SCLIRQ_LDCVsyncInHandler(&bISPFramePerfect);
        SCL_DBG(SCL_DBG_LV_DRVSCLIRQ()&(EN_DBGMG_SCLIRQLEVEL_ELSE),"[FRMLDC]@%lu\n",MsOS_GetSystemTime());
    }
    // SC1 FRM
    if(u64Flag & SCLIRQ_MSK_SC1_FRM_W_ACTIVE)
    {
        _Drv_SCLIRQ_DMAActiveHandler(E_SCLDMA_1_FRM_W);
    }

    if(u64Flag & SCLIRQ_MSK_SC1_FRM_W_ACTIVE_N)
    {
        gsclirqstate |= (E_SCLIRQ_EVENT_SC1FRM|E_SCLIRQ_EVENT_ISTSC1FRM);
        _Drv_SCLIRQ_DMAActive_NHandler(E_SCLDMA_1_FRM_W);
    }


    // SC2 FRM
    if(u64Flag & SCLIRQ_MSK_SC2_FRM_W_ACTIVE)
    {
        _Drv_SCLIRQ_DMAActiveHandler(E_SCLDMA_2_FRM_W);
    }

    if(u64Flag & SCLIRQ_MSK_SC2_FRM_W_ACTIVE_N)
    {
        gsclirqstate |= (E_SCLIRQ_EVENT_SC2FRM|E_SCLIRQ_EVENT_ISTSC2FRM);
        _Drv_SCLIRQ_DMAActive_NHandler(E_SCLDMA_2_FRM_W);
    }
    // SC1 SNP
    if(u64Flag & SCLIRQ_MSK_SC1_SNP_W_ACTIVE)
    {
        _Drv_SCLIRQ_DMAActiveHandler(E_SCLDMA_1_SNP_W);
    }

    if(u64Flag & SCLIRQ_MSK_SC1_SNP_W_ACTIVE_N)
    {
        gsclirqstate |= (E_SCLIRQ_EVENT_SC1SNP|E_SCLIRQ_EVENT_ISTSC1SNP);
        _Drv_SCLIRQ_DMAActive_NHandler(E_SCLDMA_1_SNP_W);
    }
    // SC SRC FRMEND
    if(u64Flag &SCLIRQ_MSK_SC1_ENG_FRM_END)
    {
        Drv_SCLIRQ_SetFrmEndInterruptStatus(u32Time);
        _Drv_SCLIRQ_IsAffFullContinue(&bAffFull, &bAffcount);
        bNonFRMEndCount = 0;
        SCL_DBG(SCL_DBG_LV_DRVSCLIRQ()&(EN_DBGMG_SCLIRQLEVEL_ELSE),
            "[FRMEND]%x flag:%llx,%lu\n",gsclirqstate,u64Flag,MsOS_GetSystemTime());
    }

    // SC3 FRM_W
    if(u64Flag & SCLIRQ_MSK_SC3_DMA_W_ACTIVE)
    {
        gscl3irqstate = E_SCLIRQ_SC3EVENT_CLEAR;
        gscl3irqstate |= (E_SCLIRQ_SC3EVENT_ACTIVE | E_SCLIRQ_SC3EVENT_ISTACTIVE);
        _Drv_SCLIRQ_DMAActiveHandler(E_SCLDMA_3_FRM_W);
    }

    if(u64Flag & SCLIRQ_MSK_SC3_DMA_W_ACTIVE_N)
    {
        gscl3irqstate |= (E_SCLIRQ_SC3EVENT_ACTIVEN | E_SCLIRQ_SC3EVENT_ISTACTIVEN | E_SCLIRQ_SC3EVENT_IRQ);
        _Drv_SCLIRQ_DMAActive_NHandler(E_SCLDMA_3_FRM_W);
        MsOS_SetEvent(_s32SC3EventId, gscl3irqstate);
    }

    Hal_SCLIRQ_Set_Clear((~u64Mask)&u64Flag, 0);

    return IRQ_HANDLED;
}
#endif
MS_BOOL Drv_SCLIRQ_InitVariable(void)
{
    MS_U64 u64Flag;
#if (ENABLE_ISR)
    MsOS_Memset(&gstSCLIrqInfo, 0, sizeof(ST_SCLIRQ_IFNO_TYPE));
#endif
    gsclirqstate        = 0;
    gscl3irqstate       = 0;
    _bSCLIRQ_Suspend    = 0;
    gbFRMInFlag         = 0;
    gbPtgenMode         = 0;
    gbMonitorCropMode   = 0;
    gbEachDMAEnable     = 0;
    _Drv_SCLIRQ_SetIsBlankingRegion(1);
    u64Flag = Hal_SCLIRQ_Get_Flag(0xFFFFFFFFFF);
    Hal_SCLIRQ_Set_Clear(u64Flag, 1);
    Hal_SCLIRQ_Set_Clear(u64Flag, 0);
    return TRUE;
}
void Drv_SCLIRQ_SetPTGenStatus(MS_BOOL bPTGen)
{
    gbPtgenMode  = bPTGen;
    if(bPTGen)
    {
        gbFRMInFlag = bPTGen | (bPTGen<< SCL_DELAYFRAME_FROM_ISP);
    }
}

void Drv_SCLIRQ_Set_Checkcropflag(MS_BOOL bcheck)
{
    gbMonitorCropMode  = bcheck;
}
MS_U8 Drv_SCLIRQ_Get_Checkcropflag(void)
{
    return gbMonitorCropMode;
}
MS_U8 Drv_SCLIRQ_GetEachDMAEn(void)
{
    return gbEachDMAEnable;
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
        Hal_SCLIRQ_Set_Mask(((MS_U64)1)<<i, 0);
        Hal_SCLIRQ_Set_Clear(((MS_U64)1)<<i, 1);
    }


    Drv_SCLIRQ_InitVariable();

#if (ENABLE_ISR|ENABLE_ACTIVEID_ISR)
    _s32FRMENDEventId   = MsOS_CreateEventGroup("FRMEND_Event");
    _s32SYNCEventId     = MsOS_CreateEventGroup("SYNC_Event");
    _s32SC3EventId      = MsOS_CreateEventGroup("SC3_Event");
    gu32IRQNUM      = pCfg->u32IRQNUM;
	if(MsOS_AttachInterrupt(pCfg->u32IRQNUM, (InterruptCb)_Drv_SCLIRQ_isr ,IRQF_DISABLED, "SCLINTR"))
	{
        DRV_SCLIRQ_ERR(printf("[DRVSCLIRQ]%s(%d):: Request IRQ Fail\n", __FUNCTION__, __LINE__));
        return FALSE;
    }
    MsOS_DisableInterrupt(pCfg->u32IRQNUM);
    MsOS_EnableInterrupt(pCfg->u32IRQNUM);
    _Create_SCLIRQ_IST();
#endif
#if (ENABLE_CMDQ_ISR)
    Drv_CMDQ_InitRIUBase(pCfg->u32RiuBase);
    if(MsOS_AttachInterrupt(pCfg->u32CMDQIRQNUM, (InterruptCb)_Drv_SCLIRQ_CMDQ_isr ,IRQF_DISABLED, "CMDQINTR"))
    {
        DRV_SCLIRQ_ERR(printf("[DRVSCLIRQ]%s(%d):: Request IRQ Fail\n", __FUNCTION__, __LINE__));
        return FALSE;
    }
    gu32CMDQIRQNUM      = pCfg->u32CMDQIRQNUM;
    MsOS_DisableInterrupt(pCfg->u32CMDQIRQNUM);
    MsOS_EnableInterrupt(pCfg->u32CMDQIRQNUM);
    Drv_CMDQ_SetISRStatus(1);
#endif


    return TRUE;
}
wait_queue_head_t * Drv_SCLIRQ_GetWaitQueueHead(MS_U32 enID)
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

wait_queue_head_t * Drv_SCLIRQ_GetSyncQueue(void)
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
            Hal_SCLIRQ_Set_Mask(((MS_U64)1)<<i, 0);
            Hal_SCLIRQ_Set_Clear(((MS_U64)1)<<i, 1);
        }
        _bSCLIRQ_Suspend = 0;
    }

    return TRUE;
}

MS_BOOL Drv_SCLIRQ_Exit(void)
{
    MS_BOOL bRet = TRUE;

#if (ENABLE_ISR)
    MS_U64 u64IRQ;

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
    MS_U64 u64Flag;
    u64Flag = Hal_SCLIRQ_Get_Flag(0xFFFFFFFFFF);
    Hal_SCLIRQ_Set_Clear(u64Flag, 1);
    Hal_SCLIRQ_Set_Clear(u64Flag, 0);
    MsOS_DisableInterrupt(gu32IRQNUM);
    MsOS_DetachInterrupt(gu32IRQNUM);
#endif
#if (ENABLE_CMDQ_ISR)
    MsOS_DisableInterrupt(gu32CMDQIRQNUM);
    MsOS_DetachInterrupt(gu32CMDQIRQNUM);
    Drv_CMDQ_SetISRStatus(0);
#endif
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
        Hal_SCLIRQ_Set_Mask(((MS_U64)1<<u16IRQ), 1);
        Hal_SCLIRQ_Set_Clear(((MS_U64)1<<u16IRQ), 0);
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
        Hal_SCLIRQ_Set_Mask(((MS_U64)1<<u16IRQ), 0);
        Hal_SCLIRQ_Set_Clear(((MS_U64)1<<u16IRQ), 1);
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
        *pFlag = Hal_SCLIRQ_Get_Flag(((MS_U64)1<<u16IRQ));
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
    Hal_SCLIRQ_Set_Clear((MS_U64)1<<u16IRQ, 1);
    Hal_SCLIRQ_Set_Clear((MS_U64)1<<u16IRQ, 0);
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
    Hal_SCLIRQ_Set_Mask(u64IRQ,0);
    return bRet;
}

MS_BOOL Drv_SCLIRQ_Get_Flags_Msk(MS_U64 u64IrqMsk, MS_U64 *pFlags)
{
#if (ENABLE_ISR)

    DRV_SCLIRQ_MUTEX_LOCK();
    *pFlags = gstSCLIrqInfo.u64Flag & u64IrqMsk;
    DRV_SCLIRQ_MUTEX_UNLOCK();
#else
    *pFlags = Hal_SCLIRQ_Get_Flag(u64IrqMsk);
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
    Hal_SCLIRQ_Set_Clear(u64IrqMsk, 1);
    Hal_SCLIRQ_Set_Clear(u64IrqMsk, 0);
#endif
    return TRUE;
}

#undef DRV_SCLIRQ_C
