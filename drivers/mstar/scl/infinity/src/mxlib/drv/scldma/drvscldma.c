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
#define DRV_SCLDMA_C

#ifdef MSOS_TYPE_LINUX_KERNEL
#include <linux/wait.h>
#include <linux/irqreturn.h>
#include <asm/div64.h>
#endif

//-------------------------------------------------------------------------------------------------
//  Include Files
//-------------------------------------------------------------------------------------------------
#include <linux/kthread.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>          /* seems do not need this */
#include "MsCommon.h"
#include "MsTypes.h"
#include "MsOS.h"
#include "drvsclirq_st.h"
#include "drvsclirq.h"
#include "drvscldma_st.h"
#include "halscldma.h"
#include "drvscldma.h"
#include "mdrv_scl_dbg.h"
#include "MsDbg.h"
#include "ms_msys.h"

//-------------------------------------------------------------------------------------------------
//  Defines & Macro
//-------------------------------------------------------------------------------------------------
#define DRV_SCLDMA_DBG(x)
#define DRV_SCLDMA_DBG_H(x)
#define DRV_SCLDMA_ERR(x)     x
#define FHD_Width   1920
#define FHD_Height  1080
#define HD_Width    1280
#define HD_Height   720
#define PNL_Width   800
#define PNL_Height  480
#define SINGLE_SKIP 0xF
#define FRM_POLLIN  0x1
#define SNP_POLLIN  0x2
#define SCL_IMIinitAddr 0x14000

#define SINGLE_BUFF_ACTIVE_TIMIEOUT      100
#define _Is_SC1EventNotBeFatch() (!(gu32FRMEvents & (E_SCLIRQ_EVENT_SC1POLL)))
#define _Is_SC2EventNotBeFatch() (!(gu32SC2FRMEvents & (E_SCLIRQ_EVENT_SC2POLL)))
#define _Is_SWRingModeBufferNotReady(enClientType)  (gstScldmaBufferQueue[enClientType].bUsed ==0)
#define _Is_SWRingModeBufferReady(enClientType)  (gstScldmaBufferQueue[enClientType].bUsed)
#define _Is_SWRingModeBufferNotFull(enClientType)   (!gstScldmaBufferQueue[enClientType].bFull)
#define _Is_SWRingModeBufferFull(enClientType)   (gstScldmaBufferQueue[enClientType].bFull)
#define _Is_RingMode(enClientType)                  (gstScldmaInfo.enBuffMode[(enClientType)] == E_SCLDMA_BUF_MD_RING)
#define _Is_SingleMode(enClientType)                (gstScldmaInfo.enBuffMode[(enClientType)] == E_SCLDMA_BUF_MD_SINGLE)
#define _Is_SWRingMode(enClientType)            (gstScldmaInfo.enBuffMode[(enClientType)] == E_SCLDMA_BUF_MD_SWRING)
#define _Is_DMACanReOpen(bReOpen,bRpchange) ((bReOpen)&&(bRpchange))
#define _Is_DMAClientOn(enClientType)           (gstScldmaInfo.bDMAOnOff[enClientType])
#define _Is_DMAClientOff(enClientType)           (!gstScldmaInfo.bDMAOnOff[enClientType])
#define _IsQueueWriteLargeRead(enClientType) \
    (gstScldmaBufferQueue[enClientType].pstWrite > gstScldmaBufferQueue[enClientType].pstRead)
#define _IsQueueWriteSmallRead(enClientType) \
        (gstScldmaBufferQueue[enClientType].pstWrite < gstScldmaBufferQueue[enClientType].pstRead)
#define _GetInQueueCountIfLarge(enClientType)\
        gstScldmaBufferQueue[enClientType].u8InQueueCount =\
    (gstScldmaBufferQueue[enClientType].pstWrite - gstScldmaBufferQueue[enClientType].pstRead)/SCLDMA_BUFFER_QUEUE_OFFSET;
#define _GetInQueueCountIfSmall(enClientType)\
        gstScldmaBufferQueue[enClientType].u8InQueueCount =\
    ((gstScldmaBufferQueue[enClientType].pstWrite- gstScldmaBufferQueue[enClientType].pstHead)+\
    (gstScldmaBufferQueue[enClientType].pstTail- gstScldmaBufferQueue[enClientType].pstRead))\
    /SCLDMA_BUFFER_QUEUE_OFFSET;
#define _Is_IdxRingCircuit(enClientType,idx) (idx == (gstScldmaInfo.bMaxid[enClientType]-1))

#define _Is_SC3Singlemode(enClientType) (_Is_SingleMode(enClientType) && \
    (enClientType == E_SCLDMA_3_FRM_R || enClientType == E_SCLDMA_3_FRM_W))
#define _Is_SC3Ringmode(enClientType) (_Is_RingMode(enClientType) && \
        (enClientType == E_SCLDMA_3_FRM_R || enClientType == E_SCLDMA_3_FRM_W))
#define _Is_SC3SWRingmode(enClientType) (_Is_SWRingMode(enClientType) && \
            (enClientType == E_SCLDMA_3_FRM_R || enClientType == E_SCLDMA_3_FRM_W))

#define _Is_OnlySC1SNPSingleDone(u32Event) (u32Event == E_SCLIRQ_EVENT_ISTSC1SNP &&_Is_SingleMode(E_SCLDMA_1_SNP_W))
#define _Is_OnlySC1FRMSingleDone(u32Event) (u32Event == E_SCLIRQ_EVENT_ISTSC1FRM &&_Is_SingleMode(E_SCLDMA_1_FRM_W))
#define _Is_OnlySC2FRMSingleDone(u32Event) (u32Event == E_SCLIRQ_EVENT_ISTSC2FRM &&_Is_SingleMode(E_SCLDMA_2_FRM_W))
#define _Is_VsrcId(enSCLDMA_ID) ((enSCLDMA_ID ==E_SCLDMA_ID_1_W)||(enSCLDMA_ID ==E_SCLDMA_ID_2_W))
#define _Is_VsrcDoubleBufferNotOpen() (gVsrcDBnotOpen)
#define _Is_VsrcDoubleBufferOpen() (!gVsrcDBnotOpen)
#define SCLDMA_SIZE_ALIGN(x, align)                 ((x+align) & ~(align-1))
#define SCLDMA_CHECK_ALIGN(x, align)                (x & (align-1))

#define DRV_SCLDMA_MUTEX_LOCK()            MsOS_ObtainMutex(_SCLDMA_Mutex,MSOS_WAIT_FOREVER)
#define DRV_SCLDMA_MUTEX_UNLOCK()          MsOS_ReleaseMutex(_SCLDMA_Mutex)
#define DRV_SCLDMA_MUTEX_LOCK_ISR()        MsOS_ObtainMutex_IRQ(_SCLIRQ_SCLDMA_Mutex);
#define DRV_SCLDMA_MUTEX_UNLOCK_ISR()      MsOS_ReleaseMutex_IRQ(_SCLIRQ_SCLDMA_Mutex);


#define PARSING_SCLDMA_ID(x)           (x==E_SCLDMA_ID_1_W   ? "SCLDMA_1_W" : \
                                        x==E_SCLDMA_ID_2_W   ? "SCLDMA_2_W" : \
                                        x==E_SCLDMA_ID_3_W   ? "SCLDMA_3_W" : \
                                        x==E_SCLDMA_ID_3_R   ? "SCLDMA_3_R" : \
                                        x==E_SCLDMA_ID_PNL_R ? "SCLDMA_PNL_R" : \
                                                               "UNKNOWN")



#define PARSING_SCLDMA_RWMD(x)        (x==E_SCLDMA_FRM_W ? "FRM_W" : \
                                       x==E_SCLDMA_SNP_W ? "SNP_W" : \
                                       x==E_SCLDMA_IMI_W ? "IMI_W" : \
                                       x==E_SCLDMA_FRM_R ? "FRM_R" : \
                                       x==E_SCLDMA_DBG_R ? "DBG_R" : \
                                                           "UNKNOWN")

#define PARSING_SCLDMA_BUFMD(x)       (x==E_SCLDMA_BUF_MD_RING   ? "RING" : \
                                       x==E_SCLDMA_BUF_MD_SINGLE ? "SINGLE" : \
                                                                   "UNKNOWN")

#define PARSING_SCLDMA_COLOR(x)       (x==E_SCLDMA_COLOR_YUV422 ? "YUV422" : \
                                       x==E_SCLDMA_COLOR_YUV420 ? "YUV420" : \
                                                                  "UNKNOWN")

#define PARSING_SCLDMA_CLIENT(x)      (x==E_SCLDMA_1_FRM_W ? "E_SCLDMA_1_FRM_W" : \
                                       x==E_SCLDMA_1_SNP_W ? "E_SCLDMA_1_SNP_W" : \
                                       x==E_SCLDMA_1_IMI_W ? "E_SCLDMA_1_IMI_W" : \
                                       x==E_SCLDMA_2_FRM_W ? "E_SCLDMA_2_FRM_W" : \
                                       x==E_SCLDMA_2_IMI_W ? "E_SCLDMA_2_IMI_W" : \
                                       x==E_SCLDMA_3_FRM_W ? "E_SCLDMA_3_FRM_W" : \
                                       x==E_SCLDMA_3_FRM_R ? "E_SCLDMA_3_FRM_R" : \
                                       x==E_SCLDMA_4_FRM_R ? "E_SCLDMA_4_FRM_R" : \
                                                             "UNKNOWN")

#define PARSING_SCLDMA_ISR_LOG(x)       (x==E_SCLDMA_ISR_LOG_ISPOFF   ? "ISPOFF" : \
                                        x==E_SCLDMA_ISR_LOG_SNPONLY   ? "SNPONLY" : \
                                        x==E_SCLDMA_ISR_LOG_SNPISR   ? "SNPISR" : \
                                        x==E_SCLDMA_ISR_LOG_ISPON   ? "ISPON" : \
                                        x==E_SCLDMA_ISR_LOG_SC1ON ? "SC1ON" : \
                                        x==E_SCLDMA_ISR_LOG_SC1OFF ? "SC1OFF" : \
                                        x==E_SCLDMA_ISR_LOG_SC2ON ? "SC2ON" : \
                                        x==E_SCLDMA_ISR_LOG_SC2OFF ? "SC2OFF" : \
                                        x==E_SCLDMA_ISR_LOG_SC1A ? "SC1A" : \
                                        x==E_SCLDMA_ISR_LOG_SC1N ? "SC1N" : \
                                        x==E_SCLDMA_ISR_LOG_SC2A ? "SC2A" : \
                                        x==E_SCLDMA_ISR_LOG_SC2N ? "SC2N" : \
                                        x==E_SCLDMA_ISR_LOG_FRMEND ? "FRMEND" : \
                                                               "UNKNOWN")


//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
//  Variable
//-------------------------------------------------------------------------------------------------
/////////////////
/// _SCLDMA_Mutex
/// use in scldma mutex,not include isr
////////////////
MS_S32 _SCLDMA_Mutex        = -1;

/////////////////
/// _SCLIRQ_SCLDMA_Mutex
/// scldma and sclirq mutex, include isr
////////////////
MS_S32 _SCLIRQ_SCLDMA_Mutex = -1;

/////////////////
/// gVsrcDBnotOpen
/// if True ,DMA switch Double buffer not open
////////////////
MS_BOOL gVsrcDBnotOpen;

/////////////////
/// gu32FRMEvents
/// used to save DMA event for diffenent client
////////////////
MS_U32 gu32FRMEvents;
MS_U32 gu32SC2FRMEvents;

/////////////////
/// gu8ISPcount
/// save frame count from ISP_hw - scl_delay
////////////////
MS_U8 gu8ISPcount;

/////////////////
/// gu32FRMDoneTime
/// save frame done time.
////////////////
MS_U32 gu32FRMDoneTime;

/////////////////
/// gu32TrigCount
/// save dma trig off times.
////////////////
MS_U32 gu32TrigCount[E_SCLDMA_CLIENT_NUM];
MS_BOOL gbForceClose[E_SCLDMA_CLIENT_NUM];
ST_SCLDMA_INFO_TYPE  gstScldmaInfo;
ST_SCLDMA_BUFFER_QUEUE_CONFIG gstScldmaBufferQueue[E_SCLDMA_CLIENT_NUM];

MS_BOOL gbScldmaSuspend;
MS_BOOL gbDBStatus;
#if SCLDMA_IRQ_EN
EN_SCLDMA_IRQ_MODE_TYPE genIrqMode[E_SCLDMA_CLIENT_NUM];
#endif
//-------------------------------------------------------------------------------------------------
//  Functions/
//-------------------------------------------------------------------------------------------------
#if SCLDMA_DBG_Thread
MS_BOOL bcreat                      = 0;
ST_SCLDMA_THREAD_CONFIG g_stthcfg = {0,NULL};
MS_BOOL _Delete_SCLDMA_Thread(void)
{
    g_stthcfg.flag = 0;
    if(g_stthcfg.sttask.pThread)
    {
        kthread_stop(g_stthcfg.sttask.pThread);
        g_stthcfg.sttask.pThread = NULL;
        printf("[HVSP]stop thread\n");
    }
    bcreat = 0;
    return 0;
}

static int SCLDMA_hold_Thread(void *arg)
{
    MS_U16 u16IrqNum;
    MS_U64 u64Flag_E,u64Flag_S;
    MS_U16 u32Scount = 0;
    MS_U16 u16Stime;
    printf("[DRVSCLDMA]%s:doing\n",__FUNCTION__);
    u16IrqNum = SCLIRQ_TRIG_LDC;

    while(g_stthcfg.flag == 1)
    {
        Drv_SCLIRQ_Get_Flag(u16IrqNum, &u64Flag_E);
        Drv_SCLIRQ_Get_Flag(4, &u64Flag_S);
        if(u64Flag_S)
        {
            if(u64Flag_S)
            {
                u16Stime = (MS_U32)MsOS_GetSystemTime();
                u32Scount++;
                Drv_SCLIRQ_Set_Clear(SCLIRQ_VSYNC_IDCLK);
                printf("[DRVSCLDMA]IN_FRM_END time:%d, count:%d\n",u16Stime,u32Scount);
            }
        }
    }
    _Delete_SCLDMA_Thread();

   return 0;

}

MS_BOOL _Create_SCLDMA_Thread(void)
{
    MS_BOOL bCreate     = FALSE;
    const char *pName   = {"HVSP_THREAD"};;
    if(!bcreat)
    g_stthcfg.sttask.pThread   = kthread_create(SCLDMA_hold_Thread,(void *)&g_stthcfg,pName);
    bCreate = TRUE;
    bcreat  = 1;
    if(bCreate)
    {
        if (IS_ERR(g_stthcfg.sttask.pThread))
        {
            g_stthcfg.sttask.pThread = NULL;
            Drv_SCLDMA_SetThreadOnOffFlag(0,&g_stthcfg);
            printf("[DRVSCLDMA]Fail:creat thread\n");
            return FALSE;
        }
        else
        {
            Drv_SCLDMA_SetThreadOnOffFlag(1,&g_stthcfg);
            wake_up_process(g_stthcfg.sttask.pThread );
        }
    }

    return 0;
}
#endif

void Drv_SCLDMA_SetThreadOnOffFlag(MS_BOOL bEn,ST_SCLDMA_THREAD_CONFIG *stthcfg)
{
    stthcfg->flag = bEn;
}
void _Drv_SCLDMA_SC1OnOff(EN_SCLDMA_RW_MODE_TYPE enRW,MS_BOOL bEn)
{
    Hal_SCLDMA_SetSC1HandshakeForce(enRW, bEn);
    Hal_SCLDMA_SetSC1DMAEn(enRW, bEn);
}
void _Drv_SCLDMA_SC1OnOffWithoutDoubleBuffer
    (EN_SCLDMA_RW_MODE_TYPE enRW,MS_BOOL bEn,EN_SCLDMA_CLIENT_TYPE enClientType)
{
    if(Drv_SCLIRQ_GetIsBlankingRegion())
    {
        _Drv_SCLDMA_SC1OnOff(enRW, bEn);
    }
    else
    {
        if(bEn)
        {
            DRV_SCLDMA_MUTEX_LOCK_ISR();
            _SetFlagType(enClientType,E_SCLDMA_FLAG_NEXT_ON);
            DRV_SCLDMA_MUTEX_UNLOCK_ISR();
        }
        else
        {
            _Drv_SCLDMA_SC1OnOff(enRW, bEn);
        }
        SCL_DBG(SCL_DBG_LV_DRVSCLDMA()&(Get_DBGMG_SCLDMAclient(enClientType,1)),
            "[DRMSCLDMA]%s %d wait for blanking\n",
            __FUNCTION__,enClientType);
    }
}
void _Drv_SCLDMA_SC2OnOff(EN_SCLDMA_RW_MODE_TYPE enRW,MS_BOOL bEn)
{
    Hal_SCLDMA_SetSC2HandshakeForce(enRW, bEn);
    Hal_SCLDMA_SetSC1ToSC2HandshakeForce(bEn);
    Hal_SCLDMA_SetSC2DMAEn(enRW, bEn);
}
void _Drv_SCLDMA_SC2OnOffWithoutDoubleBuffer
    (EN_SCLDMA_RW_MODE_TYPE enRW,MS_BOOL bEn,EN_SCLDMA_CLIENT_TYPE enClientType)
{
    if(Drv_SCLIRQ_GetIsBlankingRegion())
    {
        _Drv_SCLDMA_SC2OnOff(enRW, bEn);
    }
    else
    {
        if(bEn)
        {
            DRV_SCLDMA_MUTEX_LOCK_ISR();
            _SetFlagType(enClientType,E_SCLDMA_FLAG_NEXT_ON);
            DRV_SCLDMA_MUTEX_UNLOCK_ISR();
        }
        else
        {
            _Drv_SCLDMA_SC2OnOff(enRW, bEn);
        }
        SCL_DBG(SCL_DBG_LV_DRVSCLDMA()&(Get_DBGMG_SCLDMAclient(enClientType,1)),
            "[DRMSCLDMA]%s %d wait for blanking\n",
            __FUNCTION__,enClientType);
    }
}
void _Drv_SCLDMA_SWRegenVSyncTrigger(EN_SCLDMA_VS_ID_TYPE enIDType)
{
    Hal_SCLDMA_TrigRegenVSync(enIDType,1);
}
void _Drv_SCLDMA_SetVsyncRegenMode(EN_SCLDMA_VS_ID_TYPE enIDType, EN_SCLDMA_VS_TRIG_MODE_TYPE enTrigType)
{
    Hal_SCLDMA_SetVSyncRegenMode(enIDType,enTrigType);
}
void _Drv_SCLDMA_SetVsyncTrigConfig(EN_SCLDMA_VS_ID_TYPE enIDType)
{
    //hard code by hw setting
    Hal_SCLDAM_SetRegenVSyncStartPoint(enIDType,10);
    Hal_SCLDAM_SetRegenVSyncWidth(enIDType,40);
}
void _Drv_SCLDMA_ResetGlobalParameter(void)
{
    MS_U8 u8ClientIdx;
    for(u8ClientIdx=0; u8ClientIdx<E_SCLDMA_VS_ID_NUM; u8ClientIdx++)
    {
        gstScldmaInfo.stVsCfg[u8ClientIdx].enTrigMd = E_SCLDMA_VS_TRIG_MODE_HW_IN_VSYNC;
    }
    gVsrcDBnotOpen = 1;
    gu32FRMEvents = 0;
    gu32SC2FRMEvents = 0;
    gu8ISPcount = 0;
    gu32FRMDoneTime = 0;
    gbScldmaSuspend = 0;
}
void _Drv_SCLDMA_ResetGlobalParameterByClient(EN_SCLDMA_CLIENT_TYPE u8ClientIdx)
{
    MS_U8 u8BufferIdx;
    gstScldmaInfo.enBuffMode[u8ClientIdx] = E_SCLDMA_BUF_MD_NUM;
    if(u8ClientIdx<E_SCLDMA_3_FRM_R)
    {
        gstScldmaInfo.bDMAidx[u8ClientIdx]    = (E_SCLDMA_ACTIVE_BUFFER_SCL|E_SCLDMA_ACTIVE_BUFFER_OMX_RINGFULL);
    }
    else
    {
        gstScldmaInfo.bDMAidx[u8ClientIdx]    = (E_SCLDMA_ACTIVE_BUFFER_SCL|0x50);
    }

    gstScldmaInfo.bDmaflag[u8ClientIdx]   = E_SCLDMA_FLAG_BLANKING;
    gstScldmaInfo.bMaxid[u8ClientIdx]     = 0;
    gstScldmaInfo.bDMAOnOff[u8ClientIdx]  = 0;
    gstScldmaInfo.enColor[u8ClientIdx]    = 0;
    for(u8BufferIdx=0;u8BufferIdx<4;u8BufferIdx++)
    {
        gstScldmaInfo.u32Base_Y[u8ClientIdx][u8BufferIdx]     = 0;
        gstScldmaInfo.u32Base_C[u8ClientIdx][u8BufferIdx]     = 0;
    }
    gu32TrigCount[u8ClientIdx]             = 0;
    if(_Is_SWRingModeBufferReady(u8ClientIdx))
    {
        if(gstScldmaBufferQueue[u8ClientIdx].pstHead)
        {
            MsOS_VirMemFree(gstScldmaBufferQueue[u8ClientIdx].pstHead);
        }
        gstScldmaBufferQueue[u8ClientIdx].bFull = 0;
        gstScldmaBufferQueue[u8ClientIdx].bUsed = 0;
        gstScldmaBufferQueue[u8ClientIdx].enID = 0;
        gstScldmaBufferQueue[u8ClientIdx].enRWMode = 0;
        gstScldmaBufferQueue[u8ClientIdx].pstHead = 0;
        gstScldmaBufferQueue[u8ClientIdx].pstRead = 0;
        gstScldmaBufferQueue[u8ClientIdx].pstTail = 0;
        gstScldmaBufferQueue[u8ClientIdx].pstWrite = 0;
        gstScldmaBufferQueue[u8ClientIdx].pstWriteAlready = 0;
        gstScldmaBufferQueue[u8ClientIdx].u8Bufferflag = 0;
        gstScldmaBufferQueue[u8ClientIdx].u8InQueueCount = 0;
        gstScldmaBufferQueue[u8ClientIdx].u8NextActiveId = 0;
    }
}

static void _Drv_SCLDMA_InitVariable(void)
{
    MS_U8 u8ClientIdx;
    MsOS_Memset(&gstScldmaInfo, 0, sizeof(ST_SCLDMA_INFO_TYPE));
    MsOS_Memset(&gstScldmaBufferQueue, 0, sizeof(ST_SCLDMA_BUFFER_QUEUE_CONFIG)*E_SCLDMA_CLIENT_NUM);
    _Drv_SCLDMA_ResetGlobalParameter();
    gbDBStatus = DoubleBufferDefaultSet;
    for(u8ClientIdx=0; u8ClientIdx<E_SCLDMA_CLIENT_NUM; u8ClientIdx++)
    {
        _Drv_SCLDMA_ResetGlobalParameterByClient(u8ClientIdx);
        gbForceClose[u8ClientIdx]                 = 0;
        #if SCLDMA_IRQ_EN
            genIrqMode[u8ClientIdx]              = E_SCLDMA_IRQ_MODE_PENDING;
        #endif
    }
}


static EN_SCLDMA_CLIENT_TYPE _Drv_SCLDMA_TransToClientType
    (EN_SCLDMA_ID_TYPE enSCLDMA_ID, EN_SCLDMA_RW_MODE_TYPE enRWMode)
{
    EN_SCLDMA_CLIENT_TYPE enClientType;

    if(enSCLDMA_ID == E_SCLDMA_ID_1_W)
    {
        enClientType = enRWMode == E_SCLDMA_FRM_W ? E_SCLDMA_1_FRM_W :
                       enRWMode == E_SCLDMA_SNP_W ? E_SCLDMA_1_SNP_W :
                       enRWMode == E_SCLDMA_IMI_W ? E_SCLDMA_1_IMI_W :
                                                    E_SCLDMA_CLIENT_NUM;

    }
    else if(enSCLDMA_ID == E_SCLDMA_ID_2_W)
    {
        enClientType = enRWMode == E_SCLDMA_FRM_W ? E_SCLDMA_2_FRM_W :
                       enRWMode == E_SCLDMA_IMI_W ? E_SCLDMA_2_IMI_W :
                                                    E_SCLDMA_CLIENT_NUM;
    }
    else if(enSCLDMA_ID == E_SCLDMA_ID_3_R)
    {
        enClientType = enRWMode == E_SCLDMA_FRM_R ? E_SCLDMA_3_FRM_R :
                       enRWMode == E_SCLDMA_IMI_R ? E_SCLDMA_3_IMI_R :
                                                    E_SCLDMA_CLIENT_NUM;
    }
    else if(enSCLDMA_ID == E_SCLDMA_ID_3_W)
    {
        enClientType = enRWMode == E_SCLDMA_FRM_W ? E_SCLDMA_3_FRM_W :
                                                    E_SCLDMA_CLIENT_NUM;

    }
    else if(enSCLDMA_ID == E_SCLDMA_ID_PNL_R)
    {
        enClientType = enRWMode == E_SCLDMA_DBG_R ? E_SCLDMA_4_FRM_R :
                                                    E_SCLDMA_CLIENT_NUM;
    }
    else
    {
        enClientType = enRWMode == E_SCLDMA_FRM_R ? E_SCLDMA_4_FRM_R :
                                                    E_SCLDMA_CLIENT_NUM;
    }

    return enClientType;
}

static void _Drv_SCLDMA_SetDoubleBufferOn
    (EN_SCLDMA_ID_TYPE enSCLDMA_ID, MS_U16 u16IrqNum, EN_SCLDMA_CLIENT_TYPE enClientType)
{
    MS_U32 u32Time;
    MS_U64 u64Active = 0;
    if((enClientType == E_SCLDMA_1_IMI_W))
    {
        Drv_SCLIRQ_Get_Flag(u16IrqNum, &u64Active);
        if(!u64Active)
        {
            Drv_SCLIRQ_Get_Flag(u16IrqNum+1, &u64Active);
            if(u64Active)
            {
                Drv_SCLIRQ_Set_Clear(u16IrqNum+1);  // clear xxxX_ACTIVE_N
            }
        }

        u32Time = ((MS_U32)MsOS_GetSystemTime());
        while(MsOS_Timer_DiffTimeFromNow(u32Time) < 100)
        {
            Drv_SCLIRQ_Get_Flag(u16IrqNum, &u64Active);
            if(u64Active)
            {
                Drv_SCLDMA_DoubleBufferOnOffById(TRUE,enSCLDMA_ID);
                break;
            }
        }
#if ENABLE_RING_DB
        if(u64Active == 0)
        {
            gVsrcDBnotOpen = 1;
        }
#endif
    }
    SCL_DBG(SCL_DBG_LV_DRVSCLDMA()&(Get_DBGMG_SCLDMAclient(enClientType,1)), "[DRMSCLDMA]%s DiffTimeFromNow:%ld DmaId:%d\n",
        __FUNCTION__,MsOS_Timer_DiffTimeFromNow(u32Time),enSCLDMA_ID);
}

void _Drv_SCLDMA_HWInitProcess(void)
{
    //_Drv_SCLDMA_Init_FHD();
    SCL_DBG(SCL_DBG_LV_DRVSCLDMA()&EN_DBGMG_SCLDMALEVEL_ELSE, "[DRMSCLDMA]%s \n",__FUNCTION__);
    Hal_SCLDMA_HWInit();
    _Drv_SCLDMA_SetVsyncRegenMode(E_SCLDMA_VS_ID_SC3, E_SCLDMA_VS_TRIG_MODE_HW_IN_VSYNC);
    _Drv_SCLDMA_SetVsyncRegenMode(E_SCLDMA_VS_ID_SC, E_SCLDMA_VS_TRIG_MODE_HW_IN_VSYNC);
    _Drv_SCLDMA_SetVsyncRegenMode(E_SCLDMA_VS_ID_AFF, E_SCLDMA_VS_TRIG_MODE_HW_IN_VSYNC);
    _Drv_SCLDMA_SetVsyncRegenMode(E_SCLDMA_VS_ID_LDC, E_SCLDMA_VS_TRIG_MODE_HW_IN_VSYNC);
    _Drv_SCLDMA_SetVsyncRegenMode(E_SCLDMA_VS_ID_DISP, E_SCLDMA_VS_TRIG_MODE_HW_IN_VSYNC);
    Hal_SCLDMA_SetRegenVSyncVariableWidthEn(FALSE);

    Hal_SCLDMA_SetSC1HandshakeForce(E_SCLDMA_FRM_W, FALSE);
    Hal_SCLDMA_SetSC1HandshakeForce(E_SCLDMA_SNP_W, FALSE);
    Hal_SCLDMA_SetSC1HandshakeForce(E_SCLDMA_IMI_W, FALSE);
    Hal_SCLDMA_SetSC2HandshakeForce(E_SCLDMA_FRM_W, FALSE);
    Hal_SCLDMA_SetSC2HandshakeForce(E_SCLDMA_IMI_W, FALSE);
    Hal_SCLDMA_SetSC1ToSC2HandshakeForce(FALSE);
    Hal_SCLDMA_SetCheckFrmEndSignal(E_SCLDMA_2_IMI_W, TRUE);
    Hal_SCLDMA_SetCheckFrmEndSignal(E_SCLDMA_1_IMI_W, TRUE);
    Hal_SCLDMA_SetCheckFrmEndSignal(E_SCLDMA_2_FRM_W, TRUE);
    Hal_SCLDMA_SetCheckFrmEndSignal(E_SCLDMA_1_FRM_W, TRUE);
    Hal_SCLDMA_SetCheckFrmEndSignal(E_SCLDMA_1_SNP_W, TRUE);
    Hal_SCLDMA_SetCheckFrmEndSignal(E_SCLDMA_3_FRM_W, TRUE);
#if SCLDMA_DBG_Thread
{
    MS_U8 u8idx;
    for(u8idx = 0; u8idx<48; u8idx++)
    Drv_SCLIRQ_InterruptEnable(u8idx);
}
#endif
    Drv_SCLIRQ_InterruptEnable(SCLIRQ_AFF_FULL);
    Drv_SCLIRQ_InterruptEnable(SCLIRQ_SC_IN_FRM_END);
    Drv_SCLIRQ_InterruptEnable(SCLIRQ_SC3_ENG_FRM_END);
    Drv_SCLIRQ_InterruptEnable(SCLIRQ_SC1_ENG_FRM_END);
    Drv_SCLIRQ_InterruptEnable(SCLIRQ_DISP_DMA_END);
    //Drv_SCLIRQ_InterruptEnable(SCLIRQ_SC1_HVSP_FINISH);
    Drv_SCLIRQ_InterruptEnable(SCLIRQ_SC1_FRM_W_ACTIVE);
    Drv_SCLIRQ_InterruptEnable(SCLIRQ_SC1_FRM_W_ACTIVE_N);
    Drv_SCLIRQ_InterruptEnable(SCLIRQ_SC1_SNP_W_ACTIVE);
    Drv_SCLIRQ_InterruptEnable(SCLIRQ_SC1_SNP_W_ACTIVE_N);
    Drv_SCLIRQ_InterruptEnable(SCLIRQ_SC1_SNPI_W_ACTIVE);
    Drv_SCLIRQ_InterruptEnable(SCLIRQ_SC1_SNPI_W_ACTIVE_N);
    Drv_SCLIRQ_InterruptEnable(SCLIRQ_SC1_DBG_R_ACTIVE);
    Drv_SCLIRQ_InterruptEnable(SCLIRQ_SC1_DBG_R_ACTIVE_N);
    Drv_SCLIRQ_InterruptEnable(SCLIRQ_SC2_FRM_W_ACTIVE);
    Drv_SCLIRQ_InterruptEnable(SCLIRQ_SC2_FRM_W_ACTIVE_N);
    Drv_SCLIRQ_InterruptEnable(SCLIRQ_SC2_FRMI_W_ACTIVE);
    Drv_SCLIRQ_InterruptEnable(SCLIRQ_SC2_FRMI_W_ACTIVE_N);
    Drv_SCLIRQ_InterruptEnable(SCLIRQ_SC3_DMA_R_ACTIVE);
    Drv_SCLIRQ_InterruptEnable(SCLIRQ_SC3_DMA_R_ACTIVE_N);
    Drv_SCLIRQ_InterruptEnable(SCLIRQ_SC3_DMA_W_ACTIVE);
    Drv_SCLIRQ_InterruptEnable(SCLIRQ_SC3_DMA_W_ACTIVE_N);

}
MS_S32* Drv_SCLDMA_GetDMAandIRQCommonMutex(void)
{
    return &_SCLIRQ_SCLDMA_Mutex;
}

void _Drv_SCLDMA_SetSuspendFlagByClient(EN_SCLDMA_CLIENT_TYPE enClientType)
{
    if(_Is_RingMode(enClientType))
    {
        if(_Is_DMAClientOn(enClientType))
        {
            Drv_SCLDMA_SetISRHandlerDMAOff(enClientType,0);
            _SetFlagType(enClientType,E_SCLDMA_FLAG_EVERDMAON);
        }
        _ReSetFlagType(enClientType,E_SCLDMA_FLAG_ACTIVE);
        _SetFlagType(enClientType,E_SCLDMA_FLAG_DMAOFF);
    }
    else
    {
        gstScldmaInfo.bDMAOnOff[enClientType] = FALSE;
        _SetANDGetFlagType(enClientType,E_SCLDMA_FLAG_FRMDONE,(~E_SCLDMA_FLAG_ACTIVE));
    }
}

MS_BOOL Drv_SCLDMA_Suspend(EN_SCLDMA_ID_TYPE enSCLDMA_ID, ST_SCLDMA_SUSPEND_RESUME_CONFIG *pCfg)
{
    ST_SCLIRQ_SUSPEND_RESUME_CONFIG stSclIrqCfg;
    MS_U8 u8Clientidx;
    MS_BOOL bRet = TRUE;
    MS_BOOL bAllClientOn = 0;
    SCL_DBG(SCL_DBG_LV_DRVSCLDMA()&EN_DBGMG_SCLDMALEVEL_ELSE, "[DRVSCLDMA]%s(%d), ID:%s(%d), bSuspend=%d\n",
        __FUNCTION__, __LINE__,PARSING_SCLDMA_ID(enSCLDMA_ID), enSCLDMA_ID,gbScldmaSuspend);

    switch(enSCLDMA_ID)
    {
    case E_SCLDMA_ID_1_W:
        _Drv_SCLDMA_SetSuspendFlagByClient(E_SCLDMA_1_FRM_W);
        _Drv_SCLDMA_SetSuspendFlagByClient(E_SCLDMA_1_SNP_W);
        _Drv_SCLDMA_SetSuspendFlagByClient(E_SCLDMA_1_IMI_W);
        break;

    case E_SCLDMA_ID_2_W:
        _Drv_SCLDMA_SetSuspendFlagByClient(E_SCLDMA_2_FRM_W);
        _Drv_SCLDMA_SetSuspendFlagByClient(E_SCLDMA_2_IMI_W);
        break;

    case E_SCLDMA_ID_3_W:
    case E_SCLDMA_ID_3_R:
        _Drv_SCLDMA_SetSuspendFlagByClient(E_SCLDMA_3_FRM_W);
        _Drv_SCLDMA_SetSuspendFlagByClient(E_SCLDMA_3_FRM_R);
        break;

    case E_SCLDMA_ID_PNL_R:
            gstScldmaInfo.bDMAOnOff[E_SCLDMA_4_FRM_R] = FALSE;
        break;

    default:
        DRV_SCLDMA_ERR(printf("[DRVSCLDMA]%s Suspend fail\n", __FUNCTION__));
        return FALSE;
    }

    for(u8Clientidx=E_SCLDMA_1_FRM_W ;u8Clientidx<E_SCLDMA_CLIENT_NUM; u8Clientidx++)
    {
        bAllClientOn |= gstScldmaInfo.bDMAOnOff[u8Clientidx];
    }

    if(gbScldmaSuspend == 0)
    {
        if(bAllClientOn == 0)
        {
            stSclIrqCfg.u32IRQNUM = pCfg->u32IRQNum;
            if(Drv_SCLIRQ_Suspend(&stSclIrqCfg))
            {
                bRet = TRUE;
                gbScldmaSuspend = 1;
            }
            else
            {
                DRV_SCLDMA_ERR(printf("[DRVSCLDMA]%s Suspend fail\n", __FUNCTION__));
                bRet = FALSE;
            }
        }
        else
        {
            SCL_DBG(SCL_DBG_LV_DRVSCLDMA()&EN_DBGMG_SCLDMALEVEL_ELSE, "[DRVSCLDMA]%s not all scldma suspend\n",__FUNCTION__);
            bRet = TRUE;
        }
    }
    else
    {
        SCL_DBG(SCL_DBG_LV_DRVSCLDMA()&EN_DBGMG_SCLDMALEVEL_ELSE, "[DRVSCLDMA]%s already suspend\n",__FUNCTION__);
        bRet = TRUE;
    }

    return bRet;
}

MS_BOOL Drv_SCLDMA_Resume(EN_SCLDMA_ID_TYPE enSCLDMA_ID, ST_SCLDMA_SUSPEND_RESUME_CONFIG *pCfg)
{
    ST_SCLIRQ_SUSPEND_RESUME_CONFIG stSclIrqCfg;
    MS_BOOL bRet = TRUE;
    SCL_DBG(SCL_DBG_LV_DRVSCLDMA()&EN_DBGMG_SCLDMALEVEL_ELSE, "[DRVSCLDMA]%s(%d), ID:%s(%d), bSuspend=%d\n",
        __FUNCTION__, __LINE__, PARSING_SCLDMA_ID(enSCLDMA_ID), enSCLDMA_ID, gbScldmaSuspend);

    if(gbScldmaSuspend == 1)
    {
        stSclIrqCfg.u32IRQNUM = pCfg->u32IRQNum;
        if(Drv_SCLIRQ_Resume(&stSclIrqCfg))
        {
            gstScldmaInfo.u64mask = 0;
            _Drv_SCLDMA_HWInitProcess();
            gbScldmaSuspend = 0;
            MsOS_SetEvent_IRQ(Drv_SCLIRQ_Get_IRQ_EventID(),  E_SCLIRQ_EVENT_RESUME);
            MsOS_SetEvent_IRQ(Drv_SCLIRQ_Get_IRQ_SC3EventID(),  E_SCLIRQ_SC3EVENT_RESUME);
            bRet = TRUE;
        }
        else
        {
            DRV_SCLDMA_ERR(printf("[DRVSCLDMA]%s Resume fail\n", __FUNCTION__));
            bRet = FALSE;
        }
    }
    else
    {
        SCL_DBG(SCL_DBG_LV_DRVSCLDMA()&EN_DBGMG_SCLDMALEVEL_ELSE, "[DRVSCLDMA]%s already Resume\n",__FUNCTION__);
        bRet = TRUE;
    }

    return bRet;
}

MS_BOOL Drv_SCLDMA_Init(ST_SCLDMA_INIT_CONFIG *pInitCfg)
{
    char word[]     = {"_SCLDMA_Mutex"};
    char word2[]    = {"_IRQDMA_Mutex"};
    //int i;
    ST_SCLIRQ_INIT_CONFIG stIRQInitCfg;

    if(_SCLDMA_Mutex != -1)
    {
        SCL_DBG(SCL_DBG_LV_DRVSCLDMA()&EN_DBGMG_SCLDMALEVEL_ELSE, "[DRVSCLDMA]%s already done\n",__FUNCTION__);
        return TRUE;
    }

    if(MsOS_Init() == FALSE)
    {
        DRV_SCLDMA_ERR(printf("[DRVSCLDMA]%s MsOS_Init Fail\n", __FUNCTION__));
        return FALSE;
    }

    _SCLDMA_Mutex           = MsOS_CreateMutex(E_MSOS_FIFO, word, MSOS_PROCESS_SHARED);
    _SCLIRQ_SCLDMA_Mutex    = MsOS_CreateSpinlock(E_MSOS_FIFO, word2, MSOS_PROCESS_SHARED);
    if (_SCLDMA_Mutex == -1)
    {
        DRV_SCLDMA_ERR(printf("[DRVSCLDMA]%s create mutex fail\n", __FUNCTION__));
        return FALSE;
    }

    stIRQInitCfg.u32RiuBase = pInitCfg->u32RIUBase;


    // init processing

    Hal_SCLDMA_SetRiuBase(pInitCfg->u32RIUBase);
    if(Drv_SCLIRQ_Init(&stIRQInitCfg) == FALSE)
    {
        DRV_SCLDMA_ERR(printf("[DRVSCLDMA]%s Init IRQ Fail\n", __FUNCTION__));
        return FALSE;
    }
    _Drv_SCLDMA_InitVariable();

#if SCLDMA_IRQ_EN

        gstScldmaInfo.s32IrqEventId = MsOS_CreateEventGroup("SCLDMA_IRQ_EVENT");

        if(gstScldmaInfo.s32IrqEventId < 0)
        {
            DRV_SCLDMA_ERR(printf("[DRVSCLDMA]%s(%d) Create Event Fail\n", __FUNCTION__, __LINE__));
            return FALSE;
        }

        gstScldmaInfo.s32IrqTaskid  = MsOS_CreateTask((TaskEntry)_Drv_SCLDMA_Irq_Event_Task,
                                                             (MS_U32)NULL,
                                                              TRUE,
                                                              (char*)"DrvScldma_Event_Task");

        if(gstScldmaInfo.s32IrqTaskid < 0)
        {
            DRV_SCLDMA_ERR(printf("[DRVSCLDMA]%s(%d) Create Task Fail\n", __FUNCTION__, __LINE__));
            MsOS_DeleteEventGroup(gstScldmaInfo.s32IrqEventId);
            return FALSE;
        }

        gstScldmaInfo.s32IrqTimerId = MsOS_CreateTimer( _Drv_SCLDMA_CheckIrq_TimerCallback,
                                                        5,
                                                        5,
                                                        TRUE,
                                                        (char*)"SCLDMA_IRQ_TIMER");

        if(gstScldmaInfo.s32IrqTimerId < 0)
        {
            DRV_SCLDMA_ERR(printf("[DRVSCLDMA]%s(%d) Create Timer Fail\n", __FUNCTION__, __LINE__));
            MsOS_DeleteEventGroup(gstScldmaInfo.s32IrqEventId);
            MsOS_DeleteTask(gstScldmaInfo.s32IrqTaskid);
            return FALSE;
        }
#endif

    _Drv_SCLDMA_HWInitProcess();

    return TRUE;
}

void Drv_SCLDMA_Sys_Init(MS_BOOL bEn)
{
#if ENABLE_RING_DB
    if(bEn)
    {
        ST_SCLDMA_RW_CONFIG stIMICfg;
        stIMICfg.enRWMode       = E_SCLDMA_IMI_W;
        stIMICfg.u16Height      = HD_Height;
        stIMICfg.u16Width       = HD_Width;
        stIMICfg.u8MaxIdx       = 0;
        stIMICfg.u8Flag         = 1;
        stIMICfg.enBuffMode     = E_SCLDMA_BUF_MD_RING;
        stIMICfg.u32Base_Y[0]   = SCL_IMIinitAddr;
        stIMICfg.u32Base_C[0]   = SCL_IMIinitAddr;
        stIMICfg.enColor        = E_SCLDMA_COLOR_YUV420 ;
        Drv_SCLDMA_SetDMAClientConfig(E_SCLDMA_ID_1_W,stIMICfg);
    }
    else
    {
        Hal_SCLDMA_SetIMIClientReset();
    }
#endif

    printf("[DRVSCLDMA] Double Buffer Status :%hhd\n",gbDBStatus);
}
void _Drv_SCLDMA_ResetGlobalSwitchByID(EN_SCLDMA_ID_TYPE enSCLDMA_ID)
{
    MS_U8 u8ClientIdx;
    switch(enSCLDMA_ID)
    {
    case E_SCLDMA_ID_1_W:
        DRV_SCLDMA_MUTEX_LOCK_ISR();

        for(u8ClientIdx=0; u8ClientIdx<E_SCLDMA_2_FRM_W; u8ClientIdx++)
        {
            _Drv_SCLDMA_ResetGlobalParameterByClient(u8ClientIdx);
        #if SCLDMA_IRQ_EN
                genIrqMode[u8ClientIdx] = E_SCLDMA_IRQ_MODE_PENDING;
        #endif
        }
        _Drv_SCLDMA_ResetGlobalParameter();
        MsOS_ClearEventIRQ(Drv_SCLIRQ_Get_IRQ_EventID(),(0xFFFF));
        DRV_SCLDMA_MUTEX_UNLOCK_ISR();
        break;

    case E_SCLDMA_ID_2_W:
        DRV_SCLDMA_MUTEX_LOCK_ISR();
        for(u8ClientIdx=E_SCLDMA_2_FRM_W; u8ClientIdx<E_SCLDMA_3_FRM_R; u8ClientIdx++)
        {
            _Drv_SCLDMA_ResetGlobalParameterByClient(u8ClientIdx);
        #if SCLDMA_IRQ_EN
                genIrqMode[u8ClientIdx] = E_SCLDMA_IRQ_MODE_PENDING;
        #endif
        }
        DRV_SCLDMA_MUTEX_UNLOCK_ISR();
        break;

    case E_SCLDMA_ID_3_W:
    case E_SCLDMA_ID_3_R:
        DRV_SCLDMA_MUTEX_LOCK_ISR();
        for(u8ClientIdx=E_SCLDMA_3_FRM_R; u8ClientIdx<E_SCLDMA_4_FRM_R; u8ClientIdx++)
        {
            _Drv_SCLDMA_ResetGlobalParameterByClient(u8ClientIdx);
        #if SCLDMA_IRQ_EN
                genIrqMode[u8ClientIdx] = E_SCLDMA_IRQ_MODE_PENDING;
        #endif
            MsOS_ClearEventIRQ(Drv_SCLIRQ_Get_IRQ_SC3EventID(),(0xFFFF));
        }
        DRV_SCLDMA_MUTEX_UNLOCK_ISR();
        break;

    case E_SCLDMA_ID_PNL_R:
        _Drv_SCLDMA_ResetGlobalParameterByClient(E_SCLDMA_4_FRM_R);
    #if SCLDMA_IRQ_EN
            genIrqMode[E_SCLDMA_4_FRM_R] = E_SCLDMA_IRQ_MODE_PENDING;
    #endif
        break;

    default:
        DRV_SCLDMA_ERR(printf("[DRVSCLDMA]%s Release fail\n", __FUNCTION__));
        break;
    }
}
void _Drv_SCLDMA_SetClkOnOff(EN_SCLDMA_ID_TYPE enSCLDMA_ID,ST_SCLDMA_CLK_CONFIG *stclk,MS_BOOL bEn)
{
    static MS_BOOL bFClk1 = 0;
    static MS_BOOL bFClk2 = 0;
    static MS_BOOL bODClk2 = 0;
    if(enSCLDMA_ID<= E_SCLDMA_ID_2_W)
    {
        if(bFClk1 != bEn)
        {
            bFClk1 = bEn;
            Hal_SCLDMA_CLKInit(bFClk1,stclk);
        }
    }
    else if(enSCLDMA_ID<= E_SCLDMA_ID_3_R)
    {
        if(bFClk2 != bEn)
        {
            bFClk2 = bEn;
            Hal_SCLDMA_SC3CLKInit(bFClk2,stclk);
        }
    }
    else if(enSCLDMA_ID == E_SCLDMA_ID_PNL_R)
    {
        if(bODClk2 != bEn)
        {
            bODClk2 = bEn;
            Hal_SCLDMA_ODCLKInit(bODClk2,stclk);
        }
    }
}
void Drv_SCLDMA_Release(EN_SCLDMA_ID_TYPE enSCLDMA_ID,ST_SCLDMA_CLK_CONFIG *stclk)
{
    SCL_DBG(SCL_DBG_LV_DRVSCLDMA()&EN_DBGMG_SCLDMALEVEL_ELSE, "[DRVSCLDMA]%s \n",__FUNCTION__);
    Drv_SCLDMA_DoubleBufferOnOffById(FALSE,enSCLDMA_ID);
    _Drv_SCLDMA_ResetGlobalSwitchByID(enSCLDMA_ID);
    Drv_SCLIRQ_InitVariable();
    _Drv_SCLDMA_SetClkOnOff(enSCLDMA_ID,stclk,0);
}


MS_U64 _Drv_SCLDMA_GetIRQMask(void)
{
    EN_SCLDMA_CLIENT_TYPE enclient;
    MS_U64 u64Flag = 0;
    u64Flag |= SCLIRQ_MSK_SC1_SNPI_W_ACTIVE;
    u64Flag |= SCLIRQ_MSK_SC1_SNPI_W_ACTIVE_N;
    u64Flag |= SCLIRQ_MSK_SC3_ENG_FRM_END;
    for(enclient = 0; enclient<E_SCLDMA_3_IMI_R; enclient++)
    {
        if(_Is_SingleMode(enclient))
        {
            switch(enclient)
            {
                case E_SCLDMA_1_FRM_W:
                case E_SCLDMA_1_SNP_W:
                case E_SCLDMA_2_FRM_W:
                    break;
                case E_SCLDMA_1_IMI_W:
                    u64Flag |= (SCLIRQ_MSK_SC1_SNPI_W_ACTIVE|SCLIRQ_MSK_SC1_SNPI_W_ACTIVE_N);
                    break;
                case E_SCLDMA_2_IMI_W:
                    u64Flag |= (SCLIRQ_MSK_SC2_FRMI_W_ACTIVE|SCLIRQ_MSK_SC2_FRMI_W_ACTIVE_N);
                    break;
                case E_SCLDMA_3_FRM_W:
                    break;
                case E_SCLDMA_3_FRM_R:
                    u64Flag |= (SCLIRQ_MSK_SC3_DMA_R_ACTIVE|SCLIRQ_MSK_SC3_DMA_R_ACTIVE_N);
                    break;
                case E_SCLDMA_4_FRM_R:
                    u64Flag |= (SCLIRQ_MSK_SC1_DBG_R_ACTIVE|SCLIRQ_MSK_SC1_DBG_R_ACTIVE_N);
                    break;
                default:
                    break;
            }
        }
    }
    return u64Flag;
}
void _Drv_SCLDMA_SetDMAInformationForGlobal(EN_SCLDMA_CLIENT_TYPE enClientType, ST_SCLDMA_RW_CONFIG stCfg)
{
    MS_U8 u8BufferIdx;
    gstScldmaInfo.enBuffMode[enClientType] = stCfg.enBuffMode;
    gstScldmaInfo.bMaxid[enClientType] = stCfg.u8MaxIdx;
    gstScldmaInfo.u64mask              = _Drv_SCLDMA_GetIRQMask();
    gstScldmaInfo.enColor[enClientType] = stCfg.enColor;
    gstScldmaInfo.u16FrameWidth[enClientType] =  stCfg.u16Width;
    gstScldmaInfo.u16FrameHeight[enClientType] = stCfg.u16Height;
    for (u8BufferIdx = 0; u8BufferIdx <= stCfg.u8MaxIdx; u8BufferIdx++)
    {
            gstScldmaInfo.u32Base_Y[enClientType][u8BufferIdx] = stCfg.u32Base_Y[u8BufferIdx];
            gstScldmaInfo.u32Base_C[enClientType][u8BufferIdx] = stCfg.u32Base_C[u8BufferIdx];
    }
    Drv_SCLIRQ_Set_Mask(gstScldmaInfo.u64mask);
    if(_Is_SWRingMode(enClientType) && _Is_SWRingModeBufferNotReady(enClientType))
    {
        gstScldmaBufferQueue[enClientType].pstHead = MsOS_VirMemalloc(SCLDMA_BUFFER_QUEUE_OFFSET*stCfg.u8MaxIdx);
        gstScldmaBufferQueue[enClientType].pstTail = gstScldmaBufferQueue[enClientType].pstHead
            + SCLDMA_BUFFER_QUEUE_OFFSET* stCfg.u8MaxIdx;
        gstScldmaBufferQueue[enClientType].pstWrite = gstScldmaBufferQueue[enClientType].pstHead;
        gstScldmaBufferQueue[enClientType].pstRead = gstScldmaBufferQueue[enClientType].pstHead;
        gstScldmaBufferQueue[enClientType].pstWriteAlready = gstScldmaBufferQueue[enClientType].pstHead;
        gstScldmaBufferQueue[enClientType].bUsed = 1;
        gstScldmaBufferQueue[enClientType].u8Bufferflag= stCfg.u8Flag;
        stCfg.u8MaxIdx = 0;//single change buffer
    }
}
void _Drv_SCLDMA_SetVsyncTrigMode(EN_SCLDMA_CLIENT_TYPE enClientType)
{
    if(_Is_SC3Singlemode(enClientType))
    {
        _Drv_SCLDMA_SetVsyncTrigConfig(E_SCLDMA_VS_ID_SC3);
        _Drv_SCLDMA_SetVsyncRegenMode(E_SCLDMA_VS_ID_SC3, E_SCLDMA_VS_TRIG_MODE_SWTRIGGER);
    }
    else if(_Is_SC3Ringmode(enClientType)|| _Is_SC3SWRingmode(enClientType))
    {
        _Drv_SCLDMA_SetVsyncRegenMode(E_SCLDMA_VS_ID_SC3, E_SCLDMA_VS_TRIG_MODE_HW_IN_VSYNC);
    }
}
MS_BOOL Drv_SCLDMA_SetDMAClientConfig(EN_SCLDMA_ID_TYPE enSCLDMA_ID, ST_SCLDMA_RW_CONFIG stCfg)
{
    MS_BOOL bRet = TRUE;
    MS_U8 u8BufferIdx;
    EN_SCLDMA_CLIENT_TYPE enClientType;
    enClientType = _Drv_SCLDMA_TransToClientType(enSCLDMA_ID ,stCfg.enRWMode);
    SCL_DBG(SCL_DBG_LV_DRVSCLDMA()&Get_DBGMG_SCLDMAclient(enClientType,0),
        "[DRVSCLDMA]%s %d, DmaID:%s(%d), Flag:%x, RW:%s(%d), Buf:%s(%d), color:%s(%d), (W:%d, H:%d)\n",
        __FUNCTION__, __LINE__,
        PARSING_SCLDMA_ID(enSCLDMA_ID), enSCLDMA_ID,
        stCfg.u8Flag,
        PARSING_SCLDMA_RWMD(stCfg.enRWMode), stCfg.enRWMode,
        PARSING_SCLDMA_BUFMD(stCfg.enBuffMode), stCfg.enBuffMode,
        PARSING_SCLDMA_COLOR(stCfg.enColor), stCfg.enColor,
        stCfg.u16Width, stCfg.u16Height);
    SCL_DBG(SCL_DBG_LV_DRVSCLDMA()&Get_DBGMG_SCLDMAclient(enClientType,0),
        "[DRVSCLDMA]%s %d, maxbuf:%d, (%08lx, %08lx), (%08lx, %08lx), (%08lx, %08lx), (%08lx, %08lx)\n",
        __FUNCTION__, __LINE__,
        stCfg.u8MaxIdx,
        stCfg.u32Base_Y[0], stCfg.u32Base_C[0],
        stCfg.u32Base_Y[1], stCfg.u32Base_C[1],
        stCfg.u32Base_Y[2], stCfg.u32Base_C[2],
        stCfg.u32Base_Y[3], stCfg.u32Base_C[3]);

    // check
    if(SCLDMA_CHECK_ALIGN(stCfg.u16Height, 2))
    {
        DRV_SCLDMA_ERR(printf("[DRVSCLDMA]: Height must be align 2\n"));
        bRet = FALSE;
    }

    if(stCfg.enColor == E_SCLDMA_COLOR_YUV422 &&
       SCLDMA_CHECK_ALIGN(stCfg.u16Width, 8))
    {
        DRV_SCLDMA_ERR(printf("[DRVSCLDMA]%s %d: Width must be align 8\n", __FUNCTION__, __LINE__));
        bRet =  FALSE;
    }
    else if(stCfg.enColor == E_SCLDMA_COLOR_YUV420 &&
            SCLDMA_CHECK_ALIGN(stCfg.u16Width, 16))
    {
        DRV_SCLDMA_ERR(printf("[DRVSCLDMA]%s %d: Width must be align 16\n", __FUNCTION__, __LINE__));
        bRet =  FALSE;
    }

    if(stCfg.enBuffMode == E_SCLDMA_BUF_MD_SINGLE &&  stCfg.u8MaxIdx > 1)
    {
        DRV_SCLDMA_ERR(printf("[DRVSCLDMA]%s %d: More than 1 buffer to SINGLE mode\n", __FUNCTION__, __LINE__));
        bRet =  FALSE;
    }

    if(stCfg.enRWMode == E_SCLDMA_RW_NUM)
    {
        DRV_SCLDMA_ERR(printf("[DRVSCLDMA]%s %d: RW mode is not coreect\n", __FUNCTION__, __LINE__));
        bRet =  FALSE;
    }

    for(u8BufferIdx=0; u8BufferIdx<=stCfg.u8MaxIdx; u8BufferIdx++)
    {
        if( SCLDMA_CHECK_ALIGN(stCfg.u32Base_Y[u8BufferIdx], 8))
        {
            DRV_SCLDMA_ERR(printf("[DRVSCLDMA]%s %d: YBase must be 8 byte align\n", __FUNCTION__, __LINE__));
            bRet = FALSE;
            break;
        }

        if(stCfg.enColor == E_SCLDMA_COLOR_YUV420 &&
            SCLDMA_CHECK_ALIGN(stCfg.u32Base_C[u8BufferIdx], 8))
        {
            DRV_SCLDMA_ERR(printf("[DRVSCLDMA]%s %d: CBase must be 8 byte align DmaID:%s(%d)\n", __FUNCTION__, __LINE__,PARSING_SCLDMA_ID(enSCLDMA_ID), enSCLDMA_ID));
            bRet = FALSE;
            break;
        }
    }

    if(stCfg.enColor == E_SCLDMA_COLOR_YUV422)
    {
        for(u8BufferIdx=0; u8BufferIdx<=stCfg.u8MaxIdx; u8BufferIdx++)
        {
            if((stCfg.u32Base_C[u8BufferIdx] != (stCfg.u32Base_Y[u8BufferIdx] + 16) ))
            {
                DRV_SCLDMA_ERR(printf("[DRVSCLDMA]Error:%s %d: YUV422 CBase_%08lx, YBase_%08lx, \n",
                    __FUNCTION__, __LINE__, stCfg.u32Base_Y[u8BufferIdx], stCfg.u32Base_C[u8BufferIdx]));
                bRet = FALSE;
            }
        }
    }

    if(bRet == FALSE)
    {
        return FALSE;
    }

    _Drv_SCLDMA_SetDMAInformationForGlobal(enClientType, stCfg);
    _Drv_SCLDMA_SetVsyncTrigMode(enClientType);

    switch(enSCLDMA_ID)
    {
    case E_SCLDMA_ID_1_W:
        DRV_SCLDMA_MUTEX_LOCK();
         Hal_SCLDMA_SetSC1DMAConfig(stCfg);
         DRV_SCLDMA_MUTEX_UNLOCK();
        break;

    case E_SCLDMA_ID_2_W:
        DRV_SCLDMA_MUTEX_LOCK();
        Hal_SCLDMA_SetSC2DMAConfig(stCfg);
        DRV_SCLDMA_MUTEX_UNLOCK();
        break;

    case E_SCLDMA_ID_3_W:
    case E_SCLDMA_ID_3_R:
        DRV_SCLDMA_MUTEX_LOCK();
        Hal_SCLDMA_SetSC3DMAConfig(stCfg);
        DRV_SCLDMA_MUTEX_UNLOCK();
        break;

    case E_SCLDMA_ID_PNL_R:
        DRV_SCLDMA_MUTEX_LOCK();
        Hal_SCLDMA_SetDisplayDMAConfig(stCfg);
        DRV_SCLDMA_MUTEX_UNLOCK();
        break;

    default:
        return FALSE;
    }

    return TRUE;
}
MS_U16 _Drv_SCLDMA_GetActiveIRQNum(EN_SCLDMA_CLIENT_TYPE enClientType)
{
    MS_U16 u16IrqNum;

    switch(enClientType)
    {
    case E_SCLDMA_1_FRM_W:
        u16IrqNum = SCLIRQ_SC1_FRM_W_ACTIVE;
        break;
    case E_SCLDMA_1_SNP_W:
        u16IrqNum = SCLIRQ_SC1_SNP_W_ACTIVE;
        break;
    case E_SCLDMA_1_IMI_W:
        u16IrqNum = SCLIRQ_SC1_SNPI_W_ACTIVE;
        break;
    case E_SCLDMA_2_FRM_W:
        u16IrqNum = SCLIRQ_SC2_FRM_W_ACTIVE;
        break;
    case E_SCLDMA_2_IMI_W:
        u16IrqNum = SCLIRQ_SC2_FRMI_W_ACTIVE;
        break;
    case E_SCLDMA_3_FRM_W:
        u16IrqNum = SCLIRQ_SC3_DMA_W_ACTIVE;
        break;
    case E_SCLDMA_3_FRM_R:
        u16IrqNum = SCLIRQ_SC3_DMA_R_ACTIVE;
        break;
    case E_SCLDMA_4_FRM_R:
        u16IrqNum = SCLIRQ_SC1_DBG_R_ACTIVE;
        break;
    default:
        u16IrqNum = SCLIRQ_RESERVED;
        break;
    }

    return u16IrqNum;
}
void _Drv_SCLDMA_ResetTrigCount(EN_SCLDMA_CLIENT_TYPE enClientType,MS_BOOL bEn)
{
    if(!bEn)
    {
        gu32TrigCount[enClientType]++;
        if(gu32TrigCount[enClientType]==0xEFFFFFFE)
        {
            gu32TrigCount[enClientType] = 1;
        }
    }
    else
    {
        gu32TrigCount[enClientType]--;
    }
}
MS_BOOL Drv_SCLDMA_SetISRHandlerDMAOff(EN_SCLDMA_CLIENT_TYPE enClientType,MS_BOOL bEn)
{
    EN_SCLDMA_RW_MODE_TYPE enRWMode;
    MS_U64 u64ActN;
    MS_BOOL bRet = 0;
    if(gstScldmaInfo.bDMAOnOff[enClientType] == 0 &&_Is_RingMode(enClientType))
    {
        SCL_DBG(SCL_DBG_LV_DRVSCLIRQ()&Get_DBGMG_SCLIRQclient(enClientType,1), "[DRVSCLDMA] %s:: enClientType:%d still ON/OFF %hhd\n",
        __FUNCTION__,enClientType,bEn);
    }
    else if(gstScldmaInfo.bDMAOnOff[enClientType] == bEn &&_Is_RingMode(enClientType))
    {
        SCL_DBG(SCL_DBG_LV_DRVSCLIRQ()&Get_DBGMG_SCLIRQclient(enClientType,1), "[DRVSCLDMA] %s:: enClientType:%d still ON/OFF %hhd\n",
        __FUNCTION__,enClientType,bEn);
    }
    switch(enClientType)
    {
        case E_SCLDMA_1_FRM_W:
        case E_SCLDMA_2_FRM_W:
        case E_SCLDMA_3_FRM_W:
            enRWMode = E_SCLDMA_FRM_W;
            break;
        case E_SCLDMA_1_SNP_W:
            enRWMode = E_SCLDMA_SNP_W;
            break;
        case E_SCLDMA_1_IMI_W:
        case E_SCLDMA_2_IMI_W:
            enRWMode = E_SCLDMA_IMI_W;
            break;
        case E_SCLDMA_3_FRM_R:
            enRWMode = E_SCLDMA_FRM_R;
            break;
        case E_SCLDMA_4_FRM_R:
            enRWMode = E_SCLDMA_DBG_R;
            break;
        default:
            enRWMode = E_SCLDMA_RW_NUM;
            break;

    }
    if(enClientType == E_SCLDMA_1_FRM_W || enClientType == E_SCLDMA_1_SNP_W || enClientType == E_SCLDMA_1_IMI_W)
    {
        if(bEn && DoubleBufferStatus)
        {
            Drv_SCLIRQ_Get_Flag((_Drv_SCLDMA_GetActiveIRQNum(enClientType)+1), &u64ActN);
            if(!u64ActN)
            {
                _Drv_SCLDMA_SC1OnOff(enRWMode, bEn);
                bRet = bEn;
            }
        }
        else
        {
            _Drv_SCLDMA_SC1OnOff(enRWMode, bEn);
            bRet = bEn;
        }
        _Drv_SCLDMA_ResetTrigCount(enClientType,bEn);
    }
    else if(enClientType == E_SCLDMA_2_FRM_W || enClientType == E_SCLDMA_2_IMI_W)
    {
        if(bEn && DoubleBufferStatus)
        {
            Drv_SCLIRQ_Get_Flag((_Drv_SCLDMA_GetActiveIRQNum(enClientType)+1), &u64ActN);
            if(!u64ActN)
            {
                _Drv_SCLDMA_SC2OnOff(enRWMode, bEn);
                bRet = bEn;
            }
        }
        else
        {
            _Drv_SCLDMA_SC2OnOff(enRWMode, bEn);
            bRet = bEn;
        }
        _Drv_SCLDMA_ResetTrigCount(enClientType,bEn);
    }
    else if(enClientType == E_SCLDMA_3_FRM_R || enClientType == E_SCLDMA_3_FRM_W)
    {
        if(bEn && DoubleBufferStatus)
        {
            Drv_SCLIRQ_Get_Flag((_Drv_SCLDMA_GetActiveIRQNum(enClientType)+1), &u64ActN);
            if(!u64ActN)
            {
                Hal_SCLDMA_SetSC3DMAEn(E_SCLDMA_FRM_R, bEn);
                Hal_SCLDMA_SetSC3DMAEn(E_SCLDMA_FRM_W, bEn);
                bRet = bEn;
            }
        }
        else
        {
            Hal_SCLDMA_SetSC3DMAEn(E_SCLDMA_FRM_R, bEn);
            Hal_SCLDMA_SetSC3DMAEn(E_SCLDMA_FRM_W, bEn);
            bRet = bEn;
        }
        _Drv_SCLDMA_ResetTrigCount(enClientType,bEn);
    }
    else
    {
        Hal_SCLDMA_SetDisplayDMAEn(enRWMode, bEn);
        _Drv_SCLDMA_ResetTrigCount(enClientType,bEn);
    }

    if(enClientType == E_SCLDMA_3_FRM_R || enClientType == E_SCLDMA_3_FRM_W)
    {
        SCL_DBG(SCL_DBG_LV_DRVSCLIRQ()&Get_DBGMG_SCLIRQclient(enClientType,1), "[DRVSCLDMA]%s Client:%s trig off\n",
        __FUNCTION__,PARSING_SCLDMA_CLIENT(enClientType));
    }
    else
    {
        SCL_DBG(SCL_DBG_LV_DRVSCLIRQ()&Get_DBGMG_SCLIRQclient(8,1), "[DRVSCLDMA]%s Client:%s trig off\n",
        __FUNCTION__,PARSING_SCLDMA_CLIENT(enClientType));
    }
    return bRet;
}
void Drv_SCLDMA_DoubleBufferOnOffById(MS_BOOL bEn, EN_SCLDMA_ID_TYPE enSCLDMA_ID)
{
    if(enSCLDMA_ID <= E_SCLDMA_ID_2_W)
    {
        Hal_SCLDMA_SetHandshakeDoubleBuffer(bEn);
        gVsrcDBnotOpen = !bEn;
    }
    Hal_SCLDMA_SetDMAEnableDoubleBuffer(bEn,enSCLDMA_ID);
}
void Drv_SCLDMA_SetISPFrameCount(void)
{
    gu8ISPcount = Hal_SCLDMA_GetISPFrameCountReg();
    if(SCL_DELAYFRAME_FROM_ISP == 1)
    {
        gu8ISPcount = (gu8ISPcount==0) ? 0x7F : (gu8ISPcount -1);
    }
    else if(SCL_DELAYFRAME_FROM_ISP == 2)
    {
        gu8ISPcount = (gu8ISPcount==0) ? 0x7E :
                      (gu8ISPcount==1) ? 0x7F :(gu8ISPcount -2);
    }
    SCL_DBG(SCL_DBG_LV_DRVSCLIRQ()==EN_DBGMG_SCLIRQLEVEL_ELSE, "[DRVSCLDMA]ISP Count:%hhx\n",gu8ISPcount);
}
void Drv_SCLDMA_SetSclFrameDoneTime(MS_U32 u32FRMDoneTime)
{
    gu32FRMDoneTime = u32FRMDoneTime;
    SCL_DBG(SCL_DBG_LV_DRVSCLIRQ()==EN_DBGMG_SCLIRQLEVEL_ELSE, "[DRVSCLDMA]FRMDoneTime:%lu\n",gu32FRMDoneTime);
}
EN_SCLDMA_DB_STATUS_TYPE Drv_SCLDMA_GetVsrcDoubleBufferStatus(MS_U32 u32Event)
{
    MS_U32 enClientIdx;
    MS_U8 bNeedOff = 0;
    if(_Is_OnlySC1SNPSingleDone(u32Event))//only sc1 snp
    {
        SCL_DBG(SCL_DBG_LV_DRVSCLIRQ()&EN_DBGMG_SCLIRQLEVEL_ELSE,"[DRVSCLDMA] ONLY_SC1_SNP_SI_NEED_OFF \n");
        return EN_SCLDMA_DB_STATUS_NEED_OFF;//db off
    }
    else if(_Is_OnlySC1FRMSingleDone(u32Event))//only sc1 frm snp
    {
        SCL_DBG(SCL_DBG_LV_DRVSCLIRQ()&EN_DBGMG_SCLIRQLEVEL_ELSE,"[DRVSCLDMA] ONLY_SC1_FRM_SI_NEED_OFF \n");
        return EN_SCLDMA_DB_STATUS_NEED_OFF;//db off
    }
    else if(_Is_OnlySC2FRMSingleDone(u32Event))//only sc2 frm snp
    {
        SCL_DBG(SCL_DBG_LV_DRVSCLIRQ()&EN_DBGMG_SCLIRQLEVEL_ELSE,"[DRVSCLDMA] ONLY_SC2_FRM_SI_NEED_OFF \n");
        return EN_SCLDMA_DB_STATUS_NEED_OFF;//db off
    }
    else
    {
        for(enClientIdx= E_SCLDMA_1_FRM_W; enClientIdx<E_SCLDMA_3_FRM_R; enClientIdx++)
        {
            if(_Is_DMAClientOn(enClientIdx))
            {
                if(enClientIdx == E_SCLDMA_1_FRM_W && !(u32Event&E_SCLIRQ_EVENT_ISTSC1FRM) )
                {
                    bNeedOff ++;
                }
                else if(enClientIdx == E_SCLDMA_2_FRM_W && !(u32Event&E_SCLIRQ_EVENT_ISTSC2FRM) )
                {
                    bNeedOff ++;
                }
                else if(enClientIdx == E_SCLDMA_1_SNP_W && !(u32Event&E_SCLIRQ_EVENT_ISTSC1SNP) )
                {
                    bNeedOff ++;
                }
                else if(_Is_VsrcDoubleBufferNotOpen())
                {
                    SCL_DBG(SCL_DBG_LV_DRVSCLIRQ()&EN_DBGMG_SCLIRQLEVEL_ELSE,"[DRVSCLDMA] DB_NEED_ON \n");
                    return EN_SCLDMA_DB_STATUS_NEED_ON; //db on
                }
                else if(_Is_VsrcDoubleBufferOpen())
                {
                    SCL_DBG(SCL_DBG_LV_DRVSCLIRQ()&EN_DBGMG_SCLIRQLEVEL_ELSE,"[DRVSCLDMA] DB_KEEP ON\n");
                    return EN_SCLDMA_DB_STATUS_KEEP; //db keep on
                }
            }
        }
        if(bNeedOff)
        {
            SCL_DBG(SCL_DBG_LV_DRVSCLIRQ()&EN_DBGMG_SCLIRQLEVEL_ELSE,"[DRVSCLDMA] reset stauts NEED_OFF \n");
            return EN_SCLDMA_DB_STATUS_NEED_OFF;//db off
        }
        if(_Is_VsrcDoubleBufferNotOpen())
        {
            SCL_DBG(SCL_DBG_LV_DRVSCLIRQ()&EN_DBGMG_SCLIRQLEVEL_ELSE,"[DRVSCLDMA] DB_KEEP OFF\n");
            return EN_SCLDMA_DB_STATUS_KEEP;//db keep off
        }
        else
        {
            SCL_DBG(SCL_DBG_LV_DRVSCLIRQ()&EN_DBGMG_SCLIRQLEVEL_ELSE,"[DRVSCLDMA] DB_NEED_OFF \n");
            return EN_SCLDMA_DB_STATUS_NEED_OFF;//db off
        }
    }
}
MS_BOOL _Drv_SCLDMA_SetSWRingModeDMAClientOnOff(EN_SCLDMA_ID_TYPE enSCLDMA_ID, ST_SCLDMA_ONOFF_CONFIG stCfg )
{
    MS_U32 u32Time          = 0;
    EN_SCLDMA_CLIENT_TYPE enClientType;
    u32Time = ((MS_U32)MsOS_GetSystemTime());
    enClientType = _Drv_SCLDMA_TransToClientType(enSCLDMA_ID ,stCfg.enRWMode);
    SCL_DBG(SCL_DBG_LV_DRVSCLDMA()&Get_DBGMG_SCLDMAclient(enClientType,1),
        "[DRVSCLDMA]%s(%d), DmaID:%s(%d), RW:%s(%d), En:%d @:%lu\n",
        __FUNCTION__, __LINE__,
        PARSING_SCLDMA_ID(enSCLDMA_ID), enSCLDMA_ID,
        PARSING_SCLDMA_RWMD(stCfg.enRWMode), stCfg.enRWMode,
        stCfg.bEn,u32Time);
    if(gstScldmaInfo.bDMAOnOff[enClientType] == 1 &&  gstScldmaInfo.bDMAOnOff[enClientType] == stCfg.bEn)
    {
        DRV_SCLDMA_ERR(printf("[DRVSCLDMA] %s:: %s_%s still R/W \n",
            __FUNCTION__, PARSING_SCLDMA_ID(enSCLDMA_ID), PARSING_SCLDMA_RWMD(stCfg.enRWMode)));
        return FALSE;
    }
    if(gbForceClose[enClientType])
    {
        DRV_SCLDMA_ERR(printf("[DRVSCLDMA] %s:: %s_%s BLOCK \n",
            __FUNCTION__, PARSING_SCLDMA_ID(enSCLDMA_ID), PARSING_SCLDMA_RWMD(stCfg.enRWMode)));
        return FALSE;
    }
    gstScldmaInfo.bDMAOnOff[enClientType] = stCfg.bEn;

    if(stCfg.bEn)
    {
        gstScldmaInfo.bDmaflag[enClientType]   = E_SCLDMA_FLAG_BLANKING;
        gstScldmaInfo.bDMAidx[enClientType]    = (E_SCLDMA_ACTIVE_BUFFER_SCL|(gstScldmaInfo.bMaxid[enClientType]<<4));
        if(enClientType == E_SCLDMA_3_FRM_R || enClientType == E_SCLDMA_3_FRM_W)
        {
            gstScldmaInfo.bDMAidx[enClientType]    = (E_SCLDMA_ACTIVE_BUFFER_SCL|(0x5<<4));
            gstScldmaInfo.bDmaflag[enClientType]   = 0;
        }
    }
    if(stCfg.bEn)
    {
        _Drv_SCLDMA_SetClkOnOff(enSCLDMA_ID,stCfg.stclk,1);
    }
    switch(enSCLDMA_ID)
    {
    case E_SCLDMA_ID_1_W:
        _Drv_SCLDMA_SC1OnOff(stCfg.enRWMode, stCfg.bEn);
        break;

    case E_SCLDMA_ID_2_W:
        _Drv_SCLDMA_SC2OnOff(stCfg.enRWMode, stCfg.bEn);
        break;

    case E_SCLDMA_ID_3_W:
        Hal_SCLDMA_SetSC3DMAEn(stCfg.enRWMode, stCfg.bEn);
        if(stCfg.bEn)
        {
            Hal_SCLDMA_TrigRegenVSync(E_SCLDMA_VS_ID_SC3,0);
        }
        break;

    case E_SCLDMA_ID_3_R:
        Hal_SCLDMA_SetSC3DMAEn(stCfg.enRWMode, stCfg.bEn);
        break;

    case E_SCLDMA_ID_PNL_R:
        Hal_SCLDMA_SetDisplayDMAEn(stCfg.enRWMode, stCfg.bEn);

        break;

    default:
        return FALSE;
    }
    return TRUE;
}
MS_BOOL _Drv_SCLDMA_SetDMAOnOff(EN_SCLDMA_ID_TYPE enSCLDMA_ID,ST_SCLDMA_ONOFF_CONFIG stCfg)
{
    MS_BOOL bSingleMode;
    EN_SCLDMA_CLIENT_TYPE enClientType;
    enClientType    = _Drv_SCLDMA_TransToClientType(enSCLDMA_ID ,stCfg.enRWMode);
    bSingleMode     = gstScldmaInfo.enBuffMode[enClientType] == E_SCLDMA_BUF_MD_SINGLE ? TRUE : FALSE;
    switch(enSCLDMA_ID)
    {
    case E_SCLDMA_ID_1_W:
        if(DoubleBufferStatus)
        {
            _Drv_SCLDMA_SC1OnOff(stCfg.enRWMode, stCfg.bEn);
        }
        else
        {
            _Drv_SCLDMA_SC1OnOffWithoutDoubleBuffer(stCfg.enRWMode, stCfg.bEn,enClientType);
        }
        break;

    case E_SCLDMA_ID_2_W:
        if(DoubleBufferStatus)
        {
            _Drv_SCLDMA_SC2OnOff(stCfg.enRWMode, stCfg.bEn);
        }
        else
        {
            _Drv_SCLDMA_SC2OnOffWithoutDoubleBuffer(stCfg.enRWMode, stCfg.bEn,enClientType);
        }
        break;

    case E_SCLDMA_ID_3_W:
        Hal_SCLDMA_SetSC3DMAEn(stCfg.enRWMode, stCfg.bEn);
        if(bSingleMode && stCfg.bEn)
        {
            _Drv_SCLDMA_SWRegenVSyncTrigger(E_SCLDMA_VS_ID_SC3);
            _SetANDGetFlagType(E_SCLDMA_3_FRM_W,E_SCLDMA_FLAG_EVERDMAON,~(E_SCLDMA_FLAG_DMAOFF));
        }
        else if(!bSingleMode && stCfg.bEn)
        {
            _Drv_SCLDMA_SetVsyncRegenMode(E_SCLDMA_VS_ID_SC3,E_SCLDMA_VS_TRIG_MODE_HW_IN_VSYNC);
        }
        break;

    case E_SCLDMA_ID_3_R:
        Hal_SCLDMA_SetSC3DMAEn(stCfg.enRWMode, stCfg.bEn);
        break;

    case E_SCLDMA_ID_PNL_R:
        Hal_SCLDMA_SetDisplayDMAEn(stCfg.enRWMode, stCfg.bEn);
        if(bSingleMode && stCfg.bEn)
        {
            Drv_SCLDMA_DoubleBufferOnOffById(TRUE,E_SCLDMA_ID_PNL_R);
        }

        break;

    default:
        return FALSE;
    }
    return TRUE;
}
MS_BOOL Drv_SCLDMA_SetDMAClientOnOff(EN_SCLDMA_ID_TYPE enSCLDMA_ID, ST_SCLDMA_ONOFF_CONFIG stCfg )
{
    MS_BOOL bSingleMode;
    MS_U32 u32Time          = 0;
    EN_SCLDMA_CLIENT_TYPE enClientType;
    u32Time = ((MS_U32)MsOS_GetSystemTime());
    enClientType    = _Drv_SCLDMA_TransToClientType(enSCLDMA_ID ,stCfg.enRWMode);
    if(_Is_SWRingMode(enClientType))
    {
        _Drv_SCLDMA_SetSWRingModeDMAClientOnOff(enSCLDMA_ID,stCfg);
        return 1;
    }
    SCL_DBG(SCL_DBG_LV_DRVSCLDMA()&Get_DBGMG_SCLDMAclient(enClientType,1),
        "[DRVSCLDMA]%s(%d), DmaID:%s(%d), RW:%s(%d), En:%d @:%lu\n",
        __FUNCTION__, __LINE__,
        PARSING_SCLDMA_ID(enSCLDMA_ID), enSCLDMA_ID,
        PARSING_SCLDMA_RWMD(stCfg.enRWMode), stCfg.enRWMode,
        stCfg.bEn,u32Time);
    bSingleMode     = gstScldmaInfo.enBuffMode[enClientType] == E_SCLDMA_BUF_MD_SINGLE ? TRUE : FALSE;
    if(bSingleMode)
    {
        if(stCfg.bEn && _IsFlagType(enClientType,E_SCLDMA_FLAG_ACTIVE))
        {
            DRV_SCLDMA_ERR(printf("[DRVSCLDMA] %s::%d %s_%s still R/W \n",
                __FUNCTION__,enClientType, PARSING_SCLDMA_ID(enSCLDMA_ID), PARSING_SCLDMA_RWMD(stCfg.enRWMode)));
            return FALSE;
        }
        if(gbForceClose[enClientType])
        {
            DRV_SCLDMA_ERR(printf("[DRVSCLDMA] %s:: %s_%s BLOCK \n",
                __FUNCTION__, PARSING_SCLDMA_ID(enSCLDMA_ID), PARSING_SCLDMA_RWMD(stCfg.enRWMode)));
            return FALSE;
        }
    }
    else
    {
        if(gstScldmaInfo.bDMAOnOff[enClientType] == 1 &&  gstScldmaInfo.bDMAOnOff[enClientType] == stCfg.bEn)
        {
            DRV_SCLDMA_ERR(printf("[DRVSCLDMA] %s:: %s_%s still R/W %d\n",
                __FUNCTION__, PARSING_SCLDMA_ID(enSCLDMA_ID), PARSING_SCLDMA_RWMD(stCfg.enRWMode),gstScldmaInfo.enBuffMode[enClientType]));
            //DRV_SCLDMA_MUTEX_UNLOCK();
            return FALSE;
        }
        if(gbForceClose[enClientType])
        {
            DRV_SCLDMA_ERR(printf("[DRVSCLDMA] %s:: %s_%s BLOCK \n",
                __FUNCTION__, PARSING_SCLDMA_ID(enSCLDMA_ID), PARSING_SCLDMA_RWMD(stCfg.enRWMode)));
            return FALSE;
        }
    }
    gstScldmaInfo.bDMAOnOff[enClientType] = stCfg.bEn;

    if(stCfg.bEn)
    {
        if(!bSingleMode)
        {
            gstScldmaInfo.bDmaflag[enClientType]   = E_SCLDMA_FLAG_BLANKING;
            gstScldmaInfo.bDMAidx[enClientType]    = (E_SCLDMA_ACTIVE_BUFFER_SCL|(gstScldmaInfo.bMaxid[enClientType]<<4));
            if(enClientType == E_SCLDMA_3_FRM_R || enClientType == E_SCLDMA_3_FRM_W)
            {
                gstScldmaInfo.bDMAidx[enClientType]    = (E_SCLDMA_ACTIVE_BUFFER_SCL|(0x5<<4));
                gstScldmaInfo.bDmaflag[enClientType]   = 0;
            }
        }
    }
    if(stCfg.bEn)
    {
        _Drv_SCLDMA_SetClkOnOff(enSCLDMA_ID,stCfg.stclk,1);
    }
#if ENABLE_RING_DB
    if(_Is_VsrcDoubleBufferNotOpen() && _Is_VsrcId(enSCLDMA_ID) && DoubleBufferStatus)
    {
        gVsrcDBnotOpen = 0;
        _Drv_SCLDMA_SetDoubleBufferOn(E_SCLDMA_ID_1_W, SCLIRQ_SC1_SNPI_W_ACTIVE, E_SCLDMA_1_IMI_W);
    }
#endif
    return _Drv_SCLDMA_SetDMAOnOff(enSCLDMA_ID,stCfg);
}
void _Drv_SCLDMA_GetInQueueCount(EN_SCLDMA_CLIENT_TYPE enClientType)
{
    //printf("[GetInQueueCount] %hhd ,%lx,%lx\n",gstScldmaBufferQueue[enClientType].u8InQueueCount,(long)gstScldmaBufferQueue[enClientType].pstWrite,(long)gstScldmaBufferQueue[enClientType].pstRead);
    if(_IsQueueWriteLargeRead(enClientType))
    {
        _GetInQueueCountIfLarge(enClientType);
    }
    else if (_IsQueueWriteSmallRead(enClientType))
    {
        _GetInQueueCountIfSmall(enClientType);
    }
    else if(_Is_SWRingModeBufferFull(enClientType))
    {
        gstScldmaBufferQueue[enClientType].u8InQueueCount = gstScldmaInfo.bMaxid[enClientType];
    }
    else
    {
        gstScldmaBufferQueue[enClientType].u8InQueueCount = 0;
    }
}

void Drv_SCLDMA_PeekBufferQueue(ST_SCLDMA_BUFFER_QUEUE_CONFIG *pCfg)
{
    EN_SCLDMA_CLIENT_TYPE enClientType;
    enClientType = _Drv_SCLDMA_TransToClientType(pCfg->enID ,pCfg->enRWMode);
    if(_Is_SWRingModeBufferReady(enClientType))
    {
        DRV_SCLDMA_MUTEX_LOCK_ISR();
        _Drv_SCLDMA_GetInQueueCount(enClientType);
        pCfg->pstRead = gstScldmaBufferQueue[enClientType].pstRead;
        pCfg->u8InQueueCount = gstScldmaBufferQueue[enClientType].u8InQueueCount;
        pCfg->u8AccessId = gstScldmaBufferQueue[enClientType].u8AccessId;
        DRV_SCLDMA_MUTEX_UNLOCK_ISR();
        //printf("[DRVSCLDMA]%d PEEK W_P:%hhd R_P:%hhd Count:%hhd@:%lu\n",enClientType,
        //pCfg->pstWriteAlready->u8FrameAddrIdx, pCfg->pstRead->u8FrameAddrIdx,pCfg->u8InQueueCount,(MS_U32)MsOS_GetSystemTime());
        SCL_DBG(SCL_DBG_LV_DRVSCLDMA()&Get_DBGMG_SCLDMAclient(enClientType,1),
            "[DRVSCLDMA]%d W_P:%hhd R_P:%hhd Count:%hhd@:%lu\n",enClientType,
        pCfg->pstWrite->u8FrameAddrIdx, pCfg->pstRead->u8FrameAddrIdx,pCfg->u8InQueueCount,(MS_U32)MsOS_GetSystemTime());
    }
    else
    {
        DRV_SCLDMA_ERR(printf("[DRVSCLDMA] enClientType:%d non-used Buffer Queue\n",enClientType ));
    }
}
void _Drv_SCLDMA_BufferClearQueue(EN_SCLDMA_CLIENT_TYPE enClientType,unsigned char u8idx)
{
    ST_SCLDMA_FRAME_BUFFER_CONFIG *stCfg;
    unsigned char u8BufferIdx;
    if(_Is_SWRingModeBufferReady(enClientType))
    {
        stCfg = gstScldmaBufferQueue[enClientType].pstHead;
        for(u8BufferIdx = 0; u8BufferIdx<=gstScldmaInfo.bMaxid[enClientType]; u8BufferIdx++)
        {
            if(stCfg->u8FrameAddrIdx == u8idx && stCfg->u32FrameAddr != 0)
            {
                MsOS_Memset(stCfg,0,SCLDMA_BUFFER_QUEUE_OFFSET);
                break;
            }
            else
            {
                stCfg += SCLDMA_BUFFER_QUEUE_OFFSET;
            }
        }
    }
}
ST_SCLDMA_FRAME_BUFFER_CONFIG * _Drv_SCLDMA_MoveBufferQueueReadPoint(EN_SCLDMA_CLIENT_TYPE enClientType)
{
    ST_SCLDMA_FRAME_BUFFER_CONFIG* pstCfg;
    pstCfg = gstScldmaBufferQueue[enClientType].pstRead;
    gstScldmaBufferQueue[enClientType].pstRead = gstScldmaBufferQueue[enClientType].pstRead + SCLDMA_BUFFER_QUEUE_OFFSET;
    if(gstScldmaBufferQueue[enClientType].pstRead >=gstScldmaBufferQueue[enClientType].pstTail)
    {
        gstScldmaBufferQueue[enClientType].pstRead = gstScldmaBufferQueue[enClientType].pstHead;
    }
    if(_Is_SWRingModeBufferFull(enClientType))
    {
        gstScldmaBufferQueue[enClientType].bFull = 0;
    }
    _SetANDGetIdxType(enClientType,(pstCfg->u8FrameAddrIdx<<4),E_SCLDMA_ACTIVE_BUFFER_SCL);
    return pstCfg;
}
void Drv_SCLDMA_BufferDeQueue(ST_SCLDMA_BUFFER_QUEUE_CONFIG *pCfg)
{
    EN_SCLDMA_CLIENT_TYPE enClientType;
    unsigned char u8count;
    enClientType = _Drv_SCLDMA_TransToClientType(pCfg->enID ,pCfg->enRWMode);
    if(_Is_SWRingModeBufferReady(enClientType))
    {
        u8count = gstScldmaBufferQueue[enClientType].u8InQueueCount;
        DRV_SCLDMA_MUTEX_LOCK_ISR();
        if(u8count)
        {
            pCfg->pstRead = _Drv_SCLDMA_MoveBufferQueueReadPoint(enClientType);
        }
        _Drv_SCLDMA_GetInQueueCount(enClientType);
        pCfg->pstHead = gstScldmaBufferQueue[enClientType].pstHead;
        pCfg->pstTail = gstScldmaBufferQueue[enClientType].pstTail;
        pCfg->pstWrite = gstScldmaBufferQueue[enClientType].pstWrite;
        pCfg->pstWriteAlready = gstScldmaBufferQueue[enClientType].pstWriteAlready;
        pCfg->u8InQueueCount = gstScldmaBufferQueue[enClientType].u8InQueueCount;
        DRV_SCLDMA_MUTEX_UNLOCK_ISR();
        //printf("[DRVSCLDMA]%d DeQueue :R_P:%lx R_P:%hhd Count:%hhd@:%lu\n",enClientType,
        //    pCfg->pstRead->u32FrameAddr, pCfg->pstRead->u8FrameAddrIdx,gstScldmaBufferQueue[enClientType].u8InQueueCount,(MS_U32)MsOS_GetSystemTime());
        SCL_DBG(SCL_DBG_LV_DRVSCLDMA()&Get_DBGMG_SCLDMAclient(enClientType,1),
            "[DRVSCLDMA]%d DeQueue :W_P:%hhd R_P:%hhd Count:%hhd@:%lu\n",enClientType,
        pCfg->pstWriteAlready->u8FrameAddrIdx,gstScldmaBufferQueue[enClientType].pstRead->u8FrameAddrIdx,gstScldmaBufferQueue[enClientType].u8InQueueCount,(MS_U32)MsOS_GetSystemTime());
    }
    else
    {
        DRV_SCLDMA_ERR(printf("[DRVSCLDMA] enClientType:%d non-used Buffer Queue\n",enClientType ));
    }
}
void _Drv_SCLDMA_BufferFillQueue(ST_SCLDMA_FRAME_BUFFER_CONFIG *stCfg,const ST_SCLDMA_FRAME_BUFFER_CONFIG stTarget)
{
    MsOS_Memcpy(stCfg,&stTarget,SCLDMA_BUFFER_QUEUE_OFFSET);
}
void _Drv_SCLDMA_MoveBufferQueueWritePoint(EN_SCLDMA_CLIENT_TYPE enClientType)
{
    gstScldmaBufferQueue[enClientType].pstWriteAlready = gstScldmaBufferQueue[enClientType].pstWrite;
    gstScldmaBufferQueue[enClientType].pstWrite += SCLDMA_BUFFER_QUEUE_OFFSET;
    if(gstScldmaBufferQueue[enClientType].pstWrite >=gstScldmaBufferQueue[enClientType].pstTail)
    {
        gstScldmaBufferQueue[enClientType].pstWrite = gstScldmaBufferQueue[enClientType].pstHead;
    }

    if(gstScldmaBufferQueue[enClientType].pstWrite == gstScldmaBufferQueue[enClientType].pstRead)
    {
        gstScldmaBufferQueue[enClientType].bFull = 1;
    }
}
unsigned char Drv_SCLDMA_GetDoneBufferIdx(EN_SCLDMA_CLIENT_TYPE enClientType)
{
    unsigned char u8BufIdx;
    u8BufIdx = gstScldmaBufferQueue[enClientType].pstWriteAlready->u8FrameAddrIdx;
    return u8BufIdx;
}
unsigned char Drv_SCLDMA_GetNextBufferIdx(EN_SCLDMA_CLIENT_TYPE enClientType)
{
    unsigned char u8BufIdx;
    u8BufIdx = gstScldmaBufferQueue[enClientType].u8NextActiveId;
    if(u8BufIdx == gstScldmaInfo.bMaxid[enClientType])
    {
        u8BufIdx = 0;
    }
    else
    {
        u8BufIdx++;
    }
    return u8BufIdx;
}
unsigned char Drv_SCLDMA_GetActiveBufferIdx(EN_SCLDMA_CLIENT_TYPE enClientType)
{
    unsigned char u8BufIdx;
    u8BufIdx = gstScldmaBufferQueue[enClientType].u8NextActiveId;
    return u8BufIdx;
}
void _Drv_SCLDMA_DisableBufferAccess(EN_SCLDMA_CLIENT_TYPE enClientType,MS_U8 u8FrameAddrIdx)
{
    gstScldmaBufferQueue[enClientType].u8Bufferflag &= ~(0x1<<u8FrameAddrIdx);
}
void _Drv_SCLDMA_EnableBufferAccess(EN_SCLDMA_CLIENT_TYPE enClientType,MS_U8 u8FrameAddrIdx)
{
    gstScldmaBufferQueue[enClientType].u8Bufferflag |= (0x1<<u8FrameAddrIdx);
}
MS_BOOL _Drv_SCLDMA_MakeSureBufferIsReady(EN_SCLDMA_CLIENT_TYPE enClientType,MS_U8 u8FrameAddrIdx)
{
    return (gstScldmaBufferQueue[enClientType].u8Bufferflag & (0x1<<u8FrameAddrIdx));
}
void Drv_SCLDMA_EnableBufferAccess(ST_SCLDMA_BUFFER_QUEUE_CONFIG *pCfg)
{
    EN_SCLDMA_CLIENT_TYPE enClientType;
    enClientType = _Drv_SCLDMA_TransToClientType(pCfg->enID ,pCfg->enRWMode);
    //printf("[DRVSCLDMA]EnableBufferAccess%d :%hhd\n",enClientType,pCfg->u8NextActiveId);
    SCL_DBG(SCL_DBG_LV_DRVSCLDMA()&Get_DBGMG_SCLDMAclient(enClientType,1),
        "[DRVSCLDMA]EnableBufferAccess%d :%hhd\n",enClientType,pCfg->u8AccessId);
    _Drv_SCLDMA_BufferClearQueue(enClientType,pCfg->u8AccessId);
    _Drv_SCLDMA_EnableBufferAccess(enClientType,pCfg->u8AccessId);
}
MS_BOOL Drv_SCLDMA_MakeSureNextActiveId(EN_SCLDMA_CLIENT_TYPE enClientType)
{
    MS_U8 u8BufferIdx;
    MS_U8 u8FrameAddrIdx;
    u8FrameAddrIdx = Drv_SCLDMA_GetNextBufferIdx(enClientType);
    for(u8BufferIdx = 0; u8BufferIdx<gstScldmaInfo.bMaxid[enClientType]; u8BufferIdx++)
    {
        if(_Drv_SCLDMA_MakeSureBufferIsReady(enClientType,u8FrameAddrIdx))
        {
            gstScldmaBufferQueue[enClientType].u8NextActiveId = u8FrameAddrIdx;
            //printf("[DRVSCLDMA]  enClientType:%d NextID:%hhd",enClientType,u8FrameAddrIdx);
            SCL_DBG(SCL_DBG_LV_DRVSCLDMA()&Get_DBGMG_SCLDMAclient(enClientType,1),
                "[DRVSCLDMA]  enClientType:%d NextID:%hhd",enClientType,u8FrameAddrIdx);
            return 1;
        }
        else
        {
            u8FrameAddrIdx = u8FrameAddrIdx + 1;
            if(u8FrameAddrIdx>gstScldmaInfo.bMaxid[enClientType])
            {
                u8FrameAddrIdx = 0;
            }

            if(_Is_IdxRingCircuit(enClientType,u8BufferIdx))
            {
                //DRV_SCLDMA_ERR(printf("[DRVSCLDMA] enClientType:%d ChangeBufferIdx Fail\n",enClientType));
                SCL_DBG(SCL_DBG_LV_DRVSCLDMA()&Get_DBGMG_SCLDMAclient(enClientType,1),
                    "[DRVSCLDMA] enClientType:%d ChangeBufferIdx Fail\n",enClientType);
            }
        }
    }
    return 0;
}
void _Drv_SCLDMA_ChangeSC3RBufferOnlyForSC3Mode(EN_SCLDMA_CLIENT_TYPE enClientType,MS_U8 u8FrameAddrIdx)
{
    if(enClientType ==E_SCLDMA_3_FRM_W)
    {
        Hal_SCLDMA_SetDMAOutputBufferAddr(E_SCLDMA_3_FRM_R,
            gstScldmaInfo.u32Base_Y[E_SCLDMA_3_FRM_R][u8FrameAddrIdx],
            gstScldmaInfo.u32Base_C[E_SCLDMA_3_FRM_R][u8FrameAddrIdx]);
    }
}
void Drv_SCLDMA_ChangeBufferIdx(EN_SCLDMA_CLIENT_TYPE enClientType)
{
    MS_U8 u8FrameAddrIdx;
    u8FrameAddrIdx = gstScldmaBufferQueue[enClientType].u8NextActiveId;
    Hal_SCLDMA_SetDMAOutputBufferAddr(enClientType,
        gstScldmaInfo.u32Base_Y[enClientType][u8FrameAddrIdx],
        gstScldmaInfo.u32Base_C[enClientType][u8FrameAddrIdx]);
    _Drv_SCLDMA_ChangeSC3RBufferOnlyForSC3Mode(enClientType,u8FrameAddrIdx);

}
unsigned char _Drv_SCLDMA_GetISPCount(void)
{
    return ((gu8ISPcount==0x7F) ? 0 : (gu8ISPcount +1));
}
void Drv_SCLDMA_SetFrameResolution(EN_SCLDMA_CLIENT_TYPE enClientType)
{
    gstScldmaInfo.u16FrameHeight[enClientType] = Hal_SCLDMA_GetOutputVsize(enClientType);
    gstScldmaInfo.u16FrameWidth[enClientType] = Hal_SCLDMA_GetOutputHsize(enClientType);
}
unsigned char Drv_SCLDMA_BufferEnQueue(EN_SCLDMA_CLIENT_TYPE enClientType,ST_SCLDMA_FRAME_BUFFER_CONFIG stTarget)
{
    if(_Is_SWRingModeBufferReady(enClientType) && _Is_SWRingModeBufferNotFull(enClientType))
    {
        stTarget.u8ISPcount = _Drv_SCLDMA_GetISPCount();
        _Drv_SCLDMA_BufferFillQueue(gstScldmaBufferQueue[enClientType].pstWrite,stTarget);
        _Drv_SCLDMA_MoveBufferQueueWritePoint(enClientType);
        _Drv_SCLDMA_DisableBufferAccess(enClientType,stTarget.u8FrameAddrIdx);
        _Drv_SCLDMA_GetInQueueCount(enClientType);

        //printf("[DRVSCLDMA]%d EnQueue :W_P:%hhd R_P:%hhd Count:%hhd@:%lu\n",enClientType,
        //gstScldmaBufferQueue[enClientType].pstWriteAlready->u8FrameAddrIdx,
        //gstScldmaBufferQueue[enClientType].pstRead->u8FrameAddrIdx,
        //gstScldmaBufferQueue[enClientType].u8InQueueCount,(MS_U32)MsOS_GetSystemTime());
        SCL_DBG(SCL_DBG_LV_DRVSCLDMA()&Get_DBGMG_SCLDMAclient(enClientType,1),
            "[DRVSCLDMA]%d EnQueue :W_P:%hhd R_P:%hhd Count:%hhd@:%lu\n",enClientType,
        gstScldmaBufferQueue[enClientType].pstWriteAlready->u8FrameAddrIdx,
        gstScldmaBufferQueue[enClientType].pstRead->u8FrameAddrIdx,
        gstScldmaBufferQueue[enClientType].u8InQueueCount,(MS_U32)MsOS_GetSystemTime());
        return 1;
    }
    else if(_Is_SWRingModeBufferFull(enClientType))
    {
        //printf("[DRVSCLDMA]%d EnQueue Fail (FULL) :W_P:%hhd R_P:%hhd Count:%hhd@:%lu\n",enClientType,
        //gstScldmaBufferQueue[enClientType].pstWrite->u8FrameAddrIdx,
        //gstScldmaBufferQueue[enClientType].pstRead->u8FrameAddrIdx,
        //gstScldmaBufferQueue[enClientType].u8InQueueCount,(MS_U32)MsOS_GetSystemTime());
        SCL_DBG(SCL_DBG_LV_DRVSCLDMA()&Get_DBGMG_SCLDMAclient(enClientType,1),
            "[DRVSCLDMA]%d EnQueue Fail (FULL) :W_P:%hhd R_P:%hhd Count:%hhd@:%lu\n",enClientType,
        gstScldmaBufferQueue[enClientType].pstWrite->u8FrameAddrIdx,
        gstScldmaBufferQueue[enClientType].pstRead->u8FrameAddrIdx,
        gstScldmaBufferQueue[enClientType].u8InQueueCount,(MS_U32)MsOS_GetSystemTime());
        return 0;
    }
    else
    {
        DRV_SCLDMA_ERR(printf("[DRVSCLDMA] enClientType:%d non-used Buffer Queue\n",enClientType ));
        return 0;
    }
}
MS_BOOL _Drv_SCLDMA_IsVsrcEventNotBeFatch(EN_SCLDMA_ID_TYPE enSCLDMA_ID)
{
    if(enSCLDMA_ID == E_SCLDMA_ID_1_W)
    {
        return _Is_SC1EventNotBeFatch();
    }
    else if(enSCLDMA_ID == E_SCLDMA_ID_2_W)
    {
        return _Is_SC2EventNotBeFatch();
    }
    else
    {
        DRV_SCLDMA_ERR(printf("[DRVSCLDMA] error ID:%d\n",enSCLDMA_ID ));
        return 0;
    }
}
MS_U32 _Drv_SCLDMA_GetEventById(EN_SCLDMA_ID_TYPE enSCLDMA_ID)
{
    MS_U32 u32Events;
    MS_U32 *p32PointToStatic = (enSCLDMA_ID ==E_SCLDMA_ID_1_W) ? &gu32FRMEvents: &gu32SC2FRMEvents;
    MS_U32 *p32PointToOtherStatic = (enSCLDMA_ID ==E_SCLDMA_ID_1_W) ? &gu32SC2FRMEvents: &gu32FRMEvents;
    SCLIRQTXEvent enClearEventById = (enSCLDMA_ID ==E_SCLDMA_ID_1_W)? E_SCLIRQ_EVENT_SC1POLL : E_SCLIRQ_EVENT_SC2POLL;
    SCLIRQTXEvent enRemainEventById = (enSCLDMA_ID ==E_SCLDMA_ID_1_W) ? E_SCLIRQ_EVENT_SC2POLL : E_SCLIRQ_EVENT_SC1POLL;
    if(_Drv_SCLDMA_IsVsrcEventNotBeFatch(enSCLDMA_ID))
    {
        u32Events = MsOS_GetEvent(Drv_SCLIRQ_Get_IRQ_EventID());
        MsOS_ClearEventIRQ(Drv_SCLIRQ_Get_IRQ_EventID(),(u32Events & (E_SCLIRQ_EVENT_ALLPOLL)));
        DRV_SCLDMA_MUTEX_LOCK();
        if(*p32PointToOtherStatic ==0)
        {
            *p32PointToOtherStatic = u32Events& (E_SCLIRQ_EVENT_ALLPOLL) ;
            *p32PointToOtherStatic &= ~enClearEventById;
        }
        else
        {
            *p32PointToOtherStatic = *p32PointToOtherStatic | (u32Events & (enRemainEventById));
        }
        DRV_SCLDMA_MUTEX_UNLOCK();
    }
    else
    {
        DRV_SCLDMA_MUTEX_LOCK();
        u32Events       = *p32PointToStatic;
        *p32PointToStatic   = 0;
        DRV_SCLDMA_MUTEX_UNLOCK();
    }
    return u32Events;
}
MS_BOOL _Drv_SCLDMA_GetSC3DoneFlagAndClearSC3Event(MS_U32 u32Events)
{
    MS_BOOL bDone;
    if((u32Events & E_SCLIRQ_SC3EVENT_DONE) == E_SCLIRQ_SC3EVENT_DONE)  //frmend
    {
        bDone = FRM_POLLIN;
    }
    else if((u32Events & (E_SCLIRQ_SC3EVENT_RESTART)))  //frmend
    {
        bDone = FRM_POLLIN;
    }
    else if(u32Events & E_SCLIRQ_EVENT_RESUME)
    {
        bDone = FRM_POLLIN;
    }
    else   //timeout
    {
        bDone = FALSE;
    }
    if(bDone)
    {
        MsOS_ClearEventIRQ(Drv_SCLIRQ_Get_IRQ_SC3EventID(),(u32Events & E_SCLIRQ_SC3EVENT_POLL));
    }
    return bDone;
}
MS_BOOL _Drv_SCLDMA_GetSC2DoneFlag(MS_U32 u32Events)
{
    MS_BOOL bSINGLE_SKIP = 0;
    MS_BOOL bDone;
    bSINGLE_SKIP = (u32Events&E_SCLIRQ_EVENT_SC2FRM)? !(_IsFlagType(E_SCLDMA_2_FRM_W,E_SCLDMA_FLAG_FRMIN))
                                                     : 0;
    if((u32Events & E_SCLIRQ_EVENT_SC2FRM) && bSINGLE_SKIP)  //ISP FIFO FULL
    {
        bDone = SINGLE_SKIP;
        if((u32Events & (E_SCLIRQ_EVENT_SC2RE)))  //frmrestart
        {
            bDone = FRM_POLLIN;
        }
    }
    else if((u32Events & E_SCLIRQ_EVENT_SC2FRM))  //frmend
    {
        bDone = FRM_POLLIN;
    }
    else if((u32Events & (E_SCLIRQ_EVENT_SC2RESTART)))  //frmend
    {
        bDone = FRM_POLLIN;
    }
    else if(u32Events & E_SCLIRQ_EVENT_RESUME)
    {
        bDone = FRM_POLLIN;
    }
    else   //timeout
    {
        bDone = FALSE;
    }
    return bDone;
}
MS_BOOL _Drv_SCLDMA_GetSC1DoneFlag(MS_U32 u32Events)
{
    MS_BOOL bSINGLE_SKIP = 0;
    MS_BOOL bDone;
    bSINGLE_SKIP = (u32Events&E_SCLIRQ_EVENT_SC1SNP)? !(_IsFlagType(E_SCLDMA_1_SNP_W,E_SCLDMA_FLAG_FRMIN)):
                    (u32Events&E_SCLIRQ_EVENT_SC1FRM)? !(_IsFlagType(E_SCLDMA_1_FRM_W,E_SCLDMA_FLAG_FRMIN)): 0;
    if((u32Events & (E_SCLIRQ_EVENT_SC1)) && bSINGLE_SKIP)  //ISP FIFO FULL
    {
        bDone = SINGLE_SKIP;
        if((u32Events & (E_SCLIRQ_EVENT_SC1RE)))  //frmrestart
        {
            bDone = FRM_POLLIN;
        }
    }
    else if((u32Events & (E_SCLIRQ_EVENT_SC1))==(E_SCLIRQ_EVENT_SC1))
    {
        bDone = FRM_POLLIN |SNP_POLLIN;
    }
    else if((u32Events & (E_SCLIRQ_EVENT_SC1FRM)))  //frmend
    {
        bDone = FRM_POLLIN;
    }
    else if((u32Events & (E_SCLIRQ_EVENT_SC1SNP)))  //frmend
    {
        bDone = SNP_POLLIN;
    }
    else if((u32Events & (E_SCLIRQ_EVENT_SC1RESTART)))  //frmrestart
    {
        bDone = FRM_POLLIN;
    }
    else if(u32Events & E_SCLIRQ_EVENT_RESUME)
    {
        bDone = FRM_POLLIN;
    }
    else   //timeout
    {
        bDone = FALSE;
    }
    return bDone;
}
MS_BOOL _Drv_SCLDMA_GetReadDoneEvent(EN_SCLDMA_CLIENT_TYPE enClientType)
{
    MS_BOOL bDone;
    if( _Is_SingleMode(enClientType))
    {
        MS_U16 u16IrqNum;
        MS_U64 u64Flag, u64Flag_N;

        u16IrqNum = _Drv_SCLDMA_GetActiveIRQNum(enClientType);

        Drv_SCLIRQ_Get_Flag(u16IrqNum, &u64Flag);
        Drv_SCLIRQ_Get_Flag(u16IrqNum+1, &u64Flag_N);
        if(u64Flag && u64Flag_N )
        {
            bDone = TRUE;
        }
        else
        {
            bDone = FALSE;
        }
    }
    else if ( _Is_RingMode(enClientType))
    {
        bDone = TRUE;
    }
    else if(_Is_SWRingMode(enClientType))
    {
        bDone = TRUE;
    }
    else
    {
        bDone = FALSE;
    }
    return bDone;
}
MS_BOOL Drv_SCLDMA_GetDMADoneEvent(EN_SCLDMA_ID_TYPE enSCLDMA_ID, ST_SCLDMA_DONE_CONFIG *pCfg)
{
    MS_BOOL bRet = 1;
    EN_SCLDMA_CLIENT_TYPE enClientType;
    MS_U32 u32Time   = 0;
    MS_U32 u32Events = 0;
    u32Time = ((MS_U32)MsOS_GetSystemTime());
    enClientType = _Drv_SCLDMA_TransToClientType(enSCLDMA_ID ,pCfg->enRWMode);
    if(enSCLDMA_ID ==E_SCLDMA_ID_PNL_R || enSCLDMA_ID ==E_SCLDMA_ID_3_R )
    {
        _Drv_SCLDMA_GetReadDoneEvent(enClientType);
        SCL_DBG(SCL_DBG_LV_DRVSCLDMA()&Get_DBGMG_SCLDMAclient(enClientType,1),
            "[DRVSCLDMA]%s:: Client:%s bDone=%x @:%lu\n",
            __FUNCTION__, PARSING_SCLDMA_ID(enSCLDMA_ID), pCfg->bDone,u32Time);
    }
    else if(enSCLDMA_ID ==E_SCLDMA_ID_3_W )
    {
        u32Events = MsOS_GetEvent(Drv_SCLIRQ_Get_IRQ_SC3EventID());
        pCfg->bDone = _Drv_SCLDMA_GetSC3DoneFlagAndClearSC3Event(u32Events);

        SCL_DBG(SCL_DBG_LV_DRVSCLDMA()&Get_DBGMG_SCLDMAclient(enClientType,3),
            "[DRVSCLDMA]%s:: Client:%s Event:%lx bDone=%x flag:%hhx@:%lu\n",
            __FUNCTION__, PARSING_SCLDMA_CLIENT(enClientType),u32Events, pCfg->bDone,
            gstScldmaInfo.bDmaflag[enClientType],u32Time);
    }
    else
    {
        if(enSCLDMA_ID ==E_SCLDMA_ID_1_W)
        {
            u32Events = _Drv_SCLDMA_GetEventById(E_SCLDMA_ID_1_W);
            pCfg->bDone = _Drv_SCLDMA_GetSC1DoneFlag(u32Events);
        }
        else if(enSCLDMA_ID ==E_SCLDMA_ID_2_W)
        {
            u32Events = _Drv_SCLDMA_GetEventById(E_SCLDMA_ID_2_W);
            pCfg->bDone = _Drv_SCLDMA_GetSC2DoneFlag(u32Events);
        }
        SCL_DBG(SCL_DBG_LV_DRVSCLDMA()&Get_DBGMG_SCLDMAclient(enClientType,1),"[DRVSCLDMA]%s:: Client:%s Event:%lx bDone=%x\n",
             __FUNCTION__, PARSING_SCLDMA_CLIENT(enClientType),u32Events, pCfg->bDone);
    }
    return bRet;
}
MS_BOOL _Drv_SCLDMA_GetLastTimePoint(EN_SCLDMA_CLIENT_TYPE enClientType)
{
    MS_U8 u8LPoint = 0;
    u8LPoint = _GetIdxType(enClientType,E_SCLDMA_ACTIVE_BUFFER_SCL);
    return u8LPoint;
}
MS_BOOL _Drv_SCLDMA_GetDMAWritePoint(EN_SCLDMA_CLIENT_TYPE enClientType, MS_U8 u8LPoint)
{
    MS_U8 u8WPoint = 0;
    if(_Is_DMAClientOn(enClientType) &&_IsFlagType(enClientType,E_SCLDMA_FLAG_BLANKING))
    {
        u8WPoint = 0x0F;
    }
    else if( _IsFlagType(enClientType,E_SCLDMA_FLAG_ACTIVE))
    {
        u8WPoint =(u8LPoint==0) ? gstScldmaInfo.bMaxid[enClientType] : u8LPoint-1;
    }
    else
    {
        u8WPoint = u8LPoint;
    }
    return u8WPoint;
}
MS_BOOL _Drv_SCLDMA_GetAppReadPoint(EN_SCLDMA_CLIENT_TYPE enClientType ,MS_U8 u8AppInfo)
{
    MS_U8 u8RPoint = 0;
    if(_Is_DMAClientOn(enClientType) &&(_IsFlagType(enClientType,E_SCLDMA_FLAG_BLANKING)))
    {
        u8RPoint = gstScldmaInfo.bMaxid[enClientType];
    }
    else
    {
        u8RPoint = (u8AppInfo & E_SCLDMA_ACTIVE_BUFFER_OMX_FLAG) ?
            (u8AppInfo &0x0F ) : (gstScldmaInfo.bDMAidx[enClientType]>>4)&E_SCLDMA_ACTIVE_BUFFER_SCL ;
    }
    return u8RPoint;
}
void _Drv_SCLDMA_DMAOffAtActiveTimeWhetherNeedReOpen(ST_SCLDMA_POINT_CONFIG *stPointCfg)
{
    DRV_SCLDMA_MUTEX_LOCK_ISR();
    if(stPointCfg->bRWequal&&(_IsFlagType(stPointCfg->enClientType,E_SCLDMA_FLAG_DMAOFF)))
    {
        if(Drv_SCLDMA_SetISRHandlerDMAOff(stPointCfg->enClientType,1))
        {
            _ReSetFlagType(stPointCfg->enClientType,(E_SCLDMA_FLAG_EVERDMAON|E_SCLDMA_FLAG_DMAOFF));
            SCL_DBG(SCL_DBG_LV_DRVSCLDMA()&Get_DBGMG_SCLDMAclient(stPointCfg->enClientType,1),
                "[DRVSCLDMA] enClientType:%d ReOpen \n",stPointCfg->enClientType);
        }
        else
        {
            DRV_SCLDMA_ERR(printf(
                "[DRVSCLDMA] enClientType:%d strong collisionR:%hhd\n",stPointCfg->enClientType,stPointCfg->u8RPoint ));
        }
    }
    DRV_SCLDMA_MUTEX_UNLOCK_ISR();
}
void _Drv_SCLDMA_SetDMAOnWhenDMAOff
    (EN_SCLDMA_ID_TYPE enSCLDMA_ID, EN_SCLDMA_CLIENT_TYPE enClientType, ST_SCLDMA_ACTIVE_BUFFER_CONFIG *pCfg)
{
    pCfg->stOnOff.bEn = 1;
    Drv_SCLDMA_SetDMAClientOnOff(enSCLDMA_ID ,pCfg->stOnOff);
    MsOS_SetEvent_IRQ(Drv_SCLIRQ_Get_IRQ_EventID(), (enClientType==E_SCLDMA_1_FRM_W)
        ? E_SCLIRQ_EVENT_SC1RESTART: E_SCLIRQ_EVENT_SC2RESTART);
}
MS_U8 _Drv_SCLDMA_GetDoneIdxAndFlagWhenDMAOff(ST_SCLDMA_POINT_CONFIG *stPointCfg)
{
    MS_U8 u8ActiveBuffer;
    if((_IsFlagType(stPointCfg->enClientType,E_SCLDMA_FLAG_EVERDMAON)) && !stPointCfg->bRPointChange)
    {
        u8ActiveBuffer = stPointCfg->u8WPoint | E_SCLDMA_ACTIVE_BUFFER_OMX_RINGFULL | E_SCLDMA_ACTIVE_BUFFER_OMX_TRIG;
    }
    else if((_IsFlagType(stPointCfg->enClientType,E_SCLDMA_FLAG_EVERDMAON)) && stPointCfg->bRPointChange)
    {
        u8ActiveBuffer = stPointCfg->u8WPoint|E_SCLDMA_ACTIVE_BUFFER_OMX_TRIG;
        _ReSetFlagType(stPointCfg->enClientType,E_SCLDMA_FLAG_EVERDMAON);
    }
    else
    {
        u8ActiveBuffer = stPointCfg->u8WPoint | E_SCLDMA_ACTIVE_BUFFER_OMX_TRIG;
    }
    return u8ActiveBuffer;
}
MS_U8 _Drv_SCLDMA_GetDoneIdxAndFlagWhenDMAOn(ST_SCLDMA_POINT_CONFIG *stPointCfg)
{
    MS_U8 u8ActiveBuffer;
    if((_IsFlagType(stPointCfg->enClientType,E_SCLDMA_FLAG_EVERDMAON)) && !stPointCfg->bRPointChange)
    {
        u8ActiveBuffer = stPointCfg->u8WPoint | E_SCLDMA_ACTIVE_BUFFER_OMX_RINGFULL;
    }
    else if((_IsFlagType(stPointCfg->enClientType,E_SCLDMA_FLAG_EVERDMAON)) && stPointCfg->bRPointChange)
    {
        u8ActiveBuffer = stPointCfg->u8WPoint;
        _ReSetFlagType(stPointCfg->enClientType,E_SCLDMA_FLAG_EVERDMAON);
    }
    else
    {
        u8ActiveBuffer = stPointCfg->u8WPoint;
    }
    return u8ActiveBuffer;
}
ST_SCLDMA_POINT_CONFIG _Drv_SCLDMA_GetPointConfig(EN_SCLDMA_CLIENT_TYPE enClientType ,MS_U8 u8AppInfo)
{
    ST_SCLDMA_POINT_CONFIG stPointCfg;
    stPointCfg.u8LPoint = _Drv_SCLDMA_GetLastTimePoint(enClientType);
    stPointCfg.u8WPoint = _Drv_SCLDMA_GetDMAWritePoint(enClientType, stPointCfg.u8LPoint);
    stPointCfg.u8RPoint = _Drv_SCLDMA_GetAppReadPoint(enClientType,u8AppInfo);
    stPointCfg.bRWequal = (stPointCfg.u8WPoint == stPointCfg.u8RPoint)? 1 : 0;
    stPointCfg.bRPointChange = (u8AppInfo & E_SCLDMA_ACTIVE_BUFFER_OMX_FLAG)? 1 : 0;
    stPointCfg.enClientType = enClientType;
    return stPointCfg;
}
void _Drv_SCLDMA_HandlerBufferWhenDMAOff
    (EN_SCLDMA_ID_TYPE enSCLDMA_ID, ST_SCLDMA_POINT_CONFIG *stPointCfg, ST_SCLDMA_ACTIVE_BUFFER_CONFIG *pCfg)
{
    if(_Is_DMACanReOpen(stPointCfg->bRWequal,stPointCfg->bRPointChange))
    {
        _Drv_SCLDMA_SetDMAOnWhenDMAOff(enSCLDMA_ID,stPointCfg->enClientType,pCfg);
        pCfg->u8ActiveBuffer = 0x0F ;
    }
    else
    {
        pCfg->u8ActiveBuffer = _Drv_SCLDMA_GetDoneIdxAndFlagWhenDMAOff(stPointCfg);
    }
}
void _Drv_SCLDMA_HandlerBufferWhenDMAOn
    (ST_SCLDMA_POINT_CONFIG *stPointCfg, ST_SCLDMA_ACTIVE_BUFFER_CONFIG *pCfg)
{
    _Drv_SCLDMA_DMAOffAtActiveTimeWhetherNeedReOpen(stPointCfg);
    pCfg->u8ActiveBuffer = _Drv_SCLDMA_GetDoneIdxAndFlagWhenDMAOn(stPointCfg);
}
MS_BOOL Drv_SCLDMA_GetDMABufferDoneIdx(EN_SCLDMA_ID_TYPE enSCLDMA_ID, ST_SCLDMA_ACTIVE_BUFFER_CONFIG *pCfg)
{
    EN_SCLDMA_CLIENT_TYPE enClientType;
    ST_SCLDMA_POINT_CONFIG stPointCfg;
    enClientType = _Drv_SCLDMA_TransToClientType(enSCLDMA_ID ,pCfg->enRWMode);
#if ENABLE_RING_DB
    if(_Is_VsrcDoubleBufferNotOpen() && _Is_VsrcId(enSCLDMA_ID) && DoubleBufferStatus)
    {
        gVsrcDBnotOpen = 0;
        _Drv_SCLDMA_SetDoubleBufferOn(E_SCLDMA_ID_1_W, SCLIRQ_SC1_SNPI_W_ACTIVE, E_SCLDMA_1_IMI_W);
    }
#endif
    if(_Is_RingMode(enClientType))
    {
        stPointCfg = _Drv_SCLDMA_GetPointConfig(enClientType,pCfg->u8ActiveBuffer);
        DRV_SCLDMA_MUTEX_LOCK_ISR();
        _SetANDGetIdxType(enClientType,(stPointCfg.u8RPoint<<4),E_SCLDMA_ACTIVE_BUFFER_SCL);
        DRV_SCLDMA_MUTEX_UNLOCK_ISR();
        if(_Is_DMAClientOn(enClientType))
        {
            _Drv_SCLDMA_HandlerBufferWhenDMAOn(&stPointCfg,pCfg);
        }
        else
        {
            _Drv_SCLDMA_HandlerBufferWhenDMAOff(enSCLDMA_ID,&stPointCfg,pCfg);
        }
    }
    else
    {
        pCfg->u8ActiveBuffer    = 0xFF;
    }
    pCfg->u8ISPcount        = gu8ISPcount;
    pCfg->u32FRMDoneTime    = gu32FRMDoneTime;
    return 1;
}
void _Drv_SCLDMA_HandlerBufferWhenDMAOnWithoutDoublebuffer
    (ST_SCLDMA_POINT_CONFIG *stPointCfg, ST_SCLDMA_ACTIVE_BUFFER_CONFIG *pCfg)
{
    pCfg->u8ActiveBuffer = _Drv_SCLDMA_GetDoneIdxAndFlagWhenDMAOn(stPointCfg);
}
MS_BOOL Drv_SCLDMA_GetDMABufferDoneIdxWithoutDoublebuffer
    (EN_SCLDMA_ID_TYPE enSCLDMA_ID, ST_SCLDMA_ACTIVE_BUFFER_CONFIG *pCfg)
{
    EN_SCLDMA_CLIENT_TYPE enClientType;
    ST_SCLDMA_POINT_CONFIG stPointCfg;
    enClientType = _Drv_SCLDMA_TransToClientType(enSCLDMA_ID ,pCfg->enRWMode);
    if(_Is_RingMode(enClientType))
    {
        stPointCfg = _Drv_SCLDMA_GetPointConfig(enClientType,pCfg->u8ActiveBuffer);
        DRV_SCLDMA_MUTEX_LOCK_ISR();
        _SetANDGetIdxType(enClientType,(stPointCfg.u8RPoint<<4),E_SCLDMA_ACTIVE_BUFFER_SCL);
        DRV_SCLDMA_MUTEX_UNLOCK_ISR();
        if(_Is_DMAClientOn(enClientType))
        {
            _Drv_SCLDMA_HandlerBufferWhenDMAOnWithoutDoublebuffer(&stPointCfg,pCfg);
        }
        else
        {
            _Drv_SCLDMA_HandlerBufferWhenDMAOff(enSCLDMA_ID,&stPointCfg,pCfg);
        }
    }
    else
    {
        pCfg->u8ActiveBuffer    = 0xFF;
    }
    pCfg->u8ISPcount        = gu8ISPcount;
    pCfg->u32FRMDoneTime    = gu32FRMDoneTime;
    return 1;
}
void Drv_SCLDMA_SetForceCloseDMA(EN_SCLDMA_ID_TYPE enID,EN_SCLDMA_RW_MODE_TYPE enRWMode,MS_BOOL bEn)
{
    EN_SCLDMA_CLIENT_TYPE client;
    client    = _Drv_SCLDMA_TransToClientType(enID ,enRWMode);
    gbForceClose[client] = bEn;
}

void Drv_SCLDMA_ResetTrigCountByClient(EN_SCLDMA_ID_TYPE enSCLDMA_ID,EN_SCLDMA_RW_MODE_TYPE enRWMode)
{
    EN_SCLDMA_CLIENT_TYPE enClientType;
    MS_U8 u8ClientIdx;
    enClientType = _Drv_SCLDMA_TransToClientType(enSCLDMA_ID ,enRWMode);
    if(enClientType == E_SCLDMA_CLIENT_NUM)
    {
        for(u8ClientIdx=0; u8ClientIdx<E_SCLDMA_CLIENT_NUM; u8ClientIdx++)
        {
            gu32TrigCount[u8ClientIdx] = 0;
        }
    }
    else
    {
        gu32TrigCount[enClientType] = 0;
    }
}
ST_SCLDMA_ATTR_TYPE Drv_SCLDMA_GetDMAInformationByClient(EN_SCLDMA_ID_TYPE enSCLDMA_ID,EN_SCLDMA_RW_MODE_TYPE enRWMode)
{
    EN_SCLDMA_CLIENT_TYPE enClientType;
    ST_SCLDMA_ATTR_TYPE stAttr;
    unsigned char u8BufferIdx;
    enClientType        = _Drv_SCLDMA_TransToClientType(enSCLDMA_ID ,enRWMode);
    stAttr.u16DMAcount  = Hal_SCLDMA_GetDMAOutputCount(enClientType);
    stAttr.u16DMAH      = Hal_SCLDMA_GetOutputHsize(enClientType);
    stAttr.u16DMAV      = Hal_SCLDMA_GetOutputVsize(enClientType);
    stAttr.u32Trigcount = gu32TrigCount[enClientType];
    stAttr.enBuffMode   = gstScldmaInfo.enBuffMode[enClientType];
    stAttr.enColor      = gstScldmaInfo.enColor[enClientType];
    stAttr.u8MaxIdx     = gstScldmaInfo.bMaxid[enClientType];
    for(u8BufferIdx=0;u8BufferIdx<=stAttr.u8MaxIdx;u8BufferIdx++)
    {
        stAttr.u32Base_Y[u8BufferIdx] = gstScldmaInfo.u32Base_Y[enClientType][u8BufferIdx];
        stAttr.u32Base_C[u8BufferIdx] = gstScldmaInfo.u32Base_C[enClientType][u8BufferIdx];
    }
    stAttr.bDMAEn = gstScldmaInfo.bDMAOnOff[enClientType];
    return stAttr;
}
void Drv_SCLDMA_ClkClose(ST_SCLDMA_CLK_CONFIG* stclk)
{
    if(!gbclkforcemode)
    {
        Hal_SCLDMA_CLKInit(0,stclk);
        Hal_SCLDMA_SC3CLKInit(0,stclk);
        Hal_SCLDMA_ODCLKInit(0,stclk);
    }
}
wait_queue_head_t * Drv_SCLDMA_GetWaitQueueHead(EN_SCLDMA_ID_TYPE enID)
{
    return Drv_SCLIRQ_GetWaitQueueHead(enID);
}
#if ENABLE_SCLDMA_UNUSED_FUNCTION
MS_BOOL Drv_SCLDMA_GetDMABufferActiveIdx(EN_SCLDMA_ID_TYPE enSCLDMA_ID, ST_SCLDMA_ACTIVE_BUFFER_CONFIG *pCfg)
{
    EN_SCLDMA_CLIENT_TYPE enClientType;
    MS_BOOL bRet = 1;
    MS_U8 u8RPoint = 0;//R-OMX W-SCL L-last time scl
    MS_U8 u8WPoint = 0;
    MS_U8 u8LPoint = 0;
    enClientType = _Drv_SCLDMA_TransToClientType(enSCLDMA_ID ,pCfg->enRWMode);
#if ENABLE_RING_DB
    if(_Is_VsrcDoubleBufferNotOpen() && _Is_VsrcId(enSCLDMA_ID) && DoubleBufferStatus)
    {
        gVsrcDBnotOpen = 0;
        _Drv_SCLDMA_SetDoubleBufferOn(E_SCLDMA_ID_1_W, SCLIRQ_SC1_SNPI_W_ACTIVE, E_SCLDMA_1_IMI_W);
    }
#endif
    if(_Is_DMAClientOn(enClientType) &&_Is_RingMode(enClientType))
    {
        //scl active id  if E_SCLDMA_FLAG_EVERDMAON means OMX update R point not yet
        u8LPoint = _GetIdxType(enClientType,E_SCLDMA_ACTIVE_BUFFER_SCL);
        if(_IsFlagType(enClientType,E_SCLDMA_FLAG_BLANKING))
        {
            u8WPoint = 0x0F;
        }
        else if( _IsFlagType(enClientType,E_SCLDMA_FLAG_ACTIVE))
        {
            u8WPoint =(u8LPoint==0) ? gstScldmaInfo.bMaxid[enClientType] : u8LPoint-1;
        }
        else
        {
            u8WPoint = u8LPoint;
        }
        //OMX id
        if((_IsFlagType(enClientType,E_SCLDMA_FLAG_BLANKING)))
        {
            u8RPoint = gstScldmaInfo.bMaxid[enClientType];
        }
        else
        {
            u8RPoint = (pCfg->u8ActiveBuffer & E_SCLDMA_ACTIVE_BUFFER_OMX_FLAG) ?
                (pCfg->u8ActiveBuffer &0x0F ) : (gstScldmaInfo.bDMAidx[enClientType]>>4)&E_SCLDMA_ACTIVE_BUFFER_SCL ;
        }
        DRV_SCLDMA_MUTEX_LOCK_ISR();
        _SetANDGetIdxType(enClientType,(u8RPoint<<4),E_SCLDMA_ACTIVE_BUFFER_SCL);
        if(u8WPoint == u8RPoint &&(_IsFlagType(enClientType,E_SCLDMA_FLAG_DMAOFF)))
        {
            if(Drv_SCLDMA_SetISRHandlerDMAOff(enClientType,1))
            {
                _ReSetFlagType(enClientType,(E_SCLDMA_FLAG_EVERDMAON|E_SCLDMA_FLAG_DMAOFF));
                //DRV_SCLDMA_ERR(printf("[DRVSCLDMA] enClientType:%d still ON/OFF R:%hhd\n",enClientType,u8RPoint ));
            }
            else
            {
                DRV_SCLDMA_ERR(printf("[DRVSCLDMA] enClientType:%d strong collisionR:%hhd\n",enClientType,u8RPoint ));
            }
        }
        DRV_SCLDMA_MUTEX_UNLOCK_ISR();
        if((_IsFlagType(enClientType,E_SCLDMA_FLAG_EVERDMAON)) && !(pCfg->u8ActiveBuffer & E_SCLDMA_ACTIVE_BUFFER_OMX_FLAG))
        {
            pCfg->u8ActiveBuffer = u8WPoint | E_SCLDMA_ACTIVE_BUFFER_OMX_RINGFULL;
        }
        else if((_IsFlagType(enClientType,E_SCLDMA_FLAG_EVERDMAON)) && (pCfg->u8ActiveBuffer & E_SCLDMA_ACTIVE_BUFFER_OMX_FLAG))
        {
            pCfg->u8ActiveBuffer = u8WPoint;
            _ReSetFlagType(enClientType,E_SCLDMA_FLAG_EVERDMAON);
        }
        else
        {
            pCfg->u8ActiveBuffer = u8WPoint;
        }
        if(u8WPoint != 0xFF)
        {
            bRet = TRUE;
        }
        else
        {
            bRet = FALSE;
        }
        SCL_DBG(SCL_DBG_LV_DRVSCLDMA()&Get_DBGMG_SCLDMAclient(enClientType,1),"[DRVSCLDMA]%d W_P:%hhd L_P:%hhd @:%lu\n",
                enClientType,u8WPoint, u8LPoint,(MS_U32)MsOS_GetSystemTime());
    }
    else if((_Is_DMAClientOff(enClientType) && _Is_RingMode(enClientType)))
    {
        //ring and trig off
        //scl active id
        u8LPoint = _GetIdxType(enClientType,E_SCLDMA_ACTIVE_BUFFER_SCL);
        if(_IsFlagType(enClientType,E_SCLDMA_FLAG_ACTIVE))
        {
            u8WPoint = (u8LPoint == 0) ? gstScldmaInfo.bMaxid[enClientType] : u8LPoint-1;
        }
        else
        {
            u8WPoint = u8LPoint;
        }
        //OMX id
        u8RPoint = (pCfg->u8ActiveBuffer & E_SCLDMA_ACTIVE_BUFFER_OMX_FLAG) ?
                (pCfg->u8ActiveBuffer &0x0F ) : (gstScldmaInfo.bDMAidx[enClientType]>>4)&E_SCLDMA_ACTIVE_BUFFER_SCL ;
        DRV_SCLDMA_MUTEX_LOCK_ISR();
        _SetANDGetIdxType(enClientType,(u8RPoint<<4),E_SCLDMA_ACTIVE_BUFFER_SCL);
        DRV_SCLDMA_MUTEX_UNLOCK_ISR();
        if((pCfg->u8ActiveBuffer & E_SCLDMA_ACTIVE_BUFFER_OMX_FLAG))
        {
            SCL_DBG(SCL_DBG_LV_DRVSCLDMA()&Get_DBGMG_SCLDMAclient(enClientType,1),
                "[DRVSCLDMA]OFF%d W_P:%hhd L_P:%hhd @:%lu\n",
                enClientType,u8WPoint, u8LPoint,(MS_U32)MsOS_GetSystemTime());
        }
        if((u8RPoint == u8WPoint)&&(pCfg->u8ActiveBuffer & E_SCLDMA_ACTIVE_BUFFER_OMX_FLAG))
        {
            pCfg->stOnOff.bEn = 1;
            Drv_SCLDMA_SetDMAClientOnOff(enSCLDMA_ID ,pCfg->stOnOff);
            MsOS_SetEvent_IRQ(Drv_SCLIRQ_Get_IRQ_EventID(), (enClientType==E_SCLDMA_1_FRM_W)
                ? E_SCLIRQ_EVENT_SC1RESTART: E_SCLIRQ_EVENT_SC2RESTART);
            pCfg->u8ActiveBuffer = 0x0F ;
        }
        else
        {
            if((_IsFlagType(enClientType,E_SCLDMA_FLAG_EVERDMAON)) && !(pCfg->u8ActiveBuffer & E_SCLDMA_ACTIVE_BUFFER_OMX_FLAG))
            {
                pCfg->u8ActiveBuffer = u8WPoint | E_SCLDMA_ACTIVE_BUFFER_OMX_RINGFULL | E_SCLDMA_ACTIVE_BUFFER_OMX_TRIG;
            }
            else if((_IsFlagType(enClientType,E_SCLDMA_FLAG_EVERDMAON)) && (pCfg->u8ActiveBuffer & E_SCLDMA_ACTIVE_BUFFER_OMX_FLAG))
            {
                pCfg->u8ActiveBuffer = u8WPoint|E_SCLDMA_ACTIVE_BUFFER_OMX_TRIG;
                _ReSetFlagType(enClientType,E_SCLDMA_FLAG_EVERDMAON);
            }
            else
            {
                pCfg->u8ActiveBuffer = u8WPoint | E_SCLDMA_ACTIVE_BUFFER_OMX_TRIG;
            }
        }
        bRet = TRUE;
    }
    else
    {
        pCfg->u8ActiveBuffer    = 0xFF;
        bRet                    = FALSE;
    }
    pCfg->u8ISPcount        = gu8ISPcount;
    pCfg->u32FRMDoneTime    = gu32FRMDoneTime;
    return 1;
}
void _Drv_SCLDMA_SetDynamicClkOnOff(MS_BOOL bEn ,EN_SCLDMA_CLIENT_TYPE enClientType,ST_SCLDMA_CLK_CONFIG *stclk)
{
    MS_BOOL bOnOff          = 0;
    MS_BOOL bCheckClient           = 0;
    MS_U32 enCllientIdx;
    EN_SCLDMA_CLIENT_TYPE enPreCondition;
    EN_SCLDMA_CLIENT_TYPE enLasCondition;
    enPreCondition = (enClientType < E_SCLDMA_3_FRM_R) ? 0 : E_SCLDMA_3_FRM_R;
    enLasCondition = (enClientType < E_SCLDMA_3_FRM_R) ? E_SCLDMA_3_FRM_R : E_SCLDMA_4_FRM_R;
    for(enCllientIdx = enPreCondition;enCllientIdx<enLasCondition;enCllientIdx++)
    {
        if(_Is_DMAClientOn(enCllientIdx))
        {
            if(bEn)
            {
                bOnOff = 1;
            }
            bCheckClient++;
        }
        if(enCllientIdx == enClientType)
        {
            bCheckClient |= 0xF0;
        }
    }
    if(bOnOff && ((bCheckClient&0x0F) == 1)&&((bCheckClient&0xF0) == 0xF0))
    {
        if(enClientType < E_SCLDMA_3_FRM_R)
        {
            if(!gbclkforcemode)
            {
                Hal_SCLDMA_CLKInit(bOnOff,stclk);
            }
        }
        else
        {
            if(!gbclkforcemode)
            {
                Hal_SCLDMA_SC3CLKInit(bOnOff,stclk);
            }
        }
    }
    else if((bOnOff == 0)&&((bCheckClient&0x0F) == 0)&&((bCheckClient&0xF0) == 0xF0))
    {
        if(enClientType < E_SCLDMA_3_FRM_R)
        {
            Drv_SCLDMA_DoubleBufferOnOffById(FALSE,E_SCLDMA_ID_1_W);
            Drv_SCLDMA_DoubleBufferOnOffById(FALSE,E_SCLDMA_ID_PNL_R);
            if(!gbclkforcemode)
            {
                Hal_SCLDMA_CLKInit(bOnOff,stclk);
            }
        }
        else
        {
            if(!gbclkforcemode)
            {
                Hal_SCLDMA_SC3CLKInit(bOnOff,stclk);
            }
            Drv_SCLDMA_DoubleBufferOnOffById(FALSE,E_SCLDMA_ID_3_W);
        }
#if SCLDMA_DBG_Thread
        g_stthcfg.flag = 0;
        bcreat = 0;
#endif
    }
}
static void _Drv_SCLDMA_Init_FHD(void)
{
    ST_SCLDMA_RW_CONFIG stCfg;
    stCfg.u16Height     = FHD_Height;
    stCfg.u16Width      = FHD_Width;
    stCfg.u8MaxIdx      = 1;
    stCfg.u8Flag        = 0;
    stCfg.enBuffMode    = E_SCLDMA_BUF_MD_RING;
    stCfg.enColor       = E_SCLDMA_COLOR_YUV420;
    stCfg.enRWMode      = E_SCLDMA_FRM_W;
    Hal_SCLDMA_SetSC1DMAConfig(stCfg);
    Hal_SCLDMA_SetSC2DMAConfig(stCfg);
    Hal_SCLDMA_SetSC3DMAConfig(stCfg);
    stCfg.enRWMode      = E_SCLDMA_FRM_R;
    Hal_SCLDMA_SetSC3DMAConfig(stCfg);
    stCfg.enRWMode      = E_SCLDMA_DBG_R;
    stCfg.u16Height     = PNL_Height;
    stCfg.u16Width      = PNL_Width;
    Hal_SCLDMA_SetDisplayDMAConfig(stCfg);
}

static void _Drv_SCLDMA_Set_SameTime_SC1SC2(EN_SCLDMA_CLIENT_TYPE enClientType)
{
    switch(enClientType)
    {
        case E_SCLDMA_1_FRM_W:
            Hal_SCLDMA_SetSC1HandshakeForce(E_SCLDMA_FRM_W,FALSE);
            break;
        case E_SCLDMA_1_SNP_W:
            Hal_SCLDMA_SetSC1HandshakeForce(E_SCLDMA_SNP_W,FALSE);
            break;
        case E_SCLDMA_1_IMI_W:
            Hal_SCLDMA_SetSC1HandshakeForce(E_SCLDMA_IMI_W,FALSE);
            break;
        case E_SCLDMA_2_FRM_W:
            Hal_SCLDMA_SetSC1ToSC2HandshakeForce(FALSE);
            Hal_SCLDMA_SetSC2HandshakeForce(E_SCLDMA_FRM_W,FALSE);
            break;
        case E_SCLDMA_2_IMI_W:
            Hal_SCLDMA_SetSC1ToSC2HandshakeForce(FALSE);
            Hal_SCLDMA_SetSC2HandshakeForce(E_SCLDMA_IMI_W,FALSE);
            break;
        default:
            break;

    }

}


static MS_U16 _Drv_SCLDMA_IRQNum_Get_Active(MS_U16 u16IrqNum)
{
    EN_SCLDMA_CLIENT_TYPE enClientType;

    switch(u16IrqNum)
    {
    case SCLIRQ_SC1_FRM_W_ACTIVE:
        enClientType = E_SCLDMA_1_FRM_W;
        break;
    case SCLIRQ_SC1_SNP_W_ACTIVE:
        enClientType = E_SCLDMA_1_SNP_W;
        break;
    case SCLIRQ_SC1_SNPI_W_ACTIVE:
        enClientType = E_SCLDMA_1_IMI_W;
        break;
    case SCLIRQ_SC2_FRM_W_ACTIVE:
        enClientType = E_SCLDMA_2_FRM_W;
        break;
    case SCLIRQ_SC2_FRMI_W_ACTIVE:
        enClientType = E_SCLDMA_2_IMI_W;
        break;
    case SCLIRQ_SC3_DMA_W_ACTIVE:
        enClientType = E_SCLDMA_3_FRM_W;
        break;
    case SCLIRQ_SC3_DMA_R_ACTIVE:
        enClientType = E_SCLDMA_3_FRM_R;
        break;
    case SCLIRQ_SC1_DBG_R_ACTIVE:
        enClientType = E_SCLDMA_4_FRM_R;
        break;
    default:
        enClientType = E_SCLDMA_CLIENT_NUM;
        break;
    }

    return enClientType;
}
MS_BOOL Drv_SCLDMA_Set_VsConfig(EN_SCLDMA_VS_ID_TYPE enVsID,  ST_SCLDMA_VS_CONFIG stCfg)
{

    DRV_SCLDMA_DBG(printf("[DRVSCLDMA], VsID:%d, Trig:%d, VsRef:%d, (%d, %d)\n",
        enVsID, stCfg.enTrigMd, stCfg.enVsRefMd, stCfg.u16Vs_St, stCfg.u16Vs_Width));
    SCL_DBG(SCL_DBG_LV_DRVSCLDMA()&EN_DBGMG_SCLDMALEVEL_ELSE,"[DRVSCLDMA], VsID:%d, Trig:%d, VsRef:%d, (%d, %d)\n",
    enVsID, stCfg.enTrigMd, stCfg.enVsRefMd, stCfg.u16Vs_St, stCfg.u16Vs_Width);
    memcpy(&gstScldmaInfo.stVsCfg[enVsID], &stCfg, sizeof(ST_SCLDMA_VS_CONFIG));

    if(gstScldmaInfo.stVsCfg[E_SCLDMA_VS_ID_SC].enTrigMd   != E_SCLDMA_VS_TRIG_MODE_HW_IN_VSYNC ||
       gstScldmaInfo.stVsCfg[E_SCLDMA_VS_ID_AFF].enTrigMd  != E_SCLDMA_VS_TRIG_MODE_HW_IN_VSYNC ||
       gstScldmaInfo.stVsCfg[E_SCLDMA_VS_ID_LDC].enTrigMd  != E_SCLDMA_VS_TRIG_MODE_HW_IN_VSYNC ||
       gstScldmaInfo.stVsCfg[E_SCLDMA_VS_ID_SC3].enTrigMd  != E_SCLDMA_VS_TRIG_MODE_HW_IN_VSYNC ||
       gstScldmaInfo.stVsCfg[E_SCLDMA_VS_ID_DISP].enTrigMd != E_SCLDMA_VS_TRIG_MODE_HW_IN_VSYNC)
    {
        Hal_SCLDMA_SetRegenVSyncVariableWidthEn(TRUE);
    }
    else
    {
        Hal_SCLDMA_SetRegenVSyncVariableWidthEn(FALSE);

    }

    _Drv_SCLDMA_SetVsyncRegenMode(enVsID, stCfg.enTrigMd);
    Hal_SCLDMA_SetRegenVSyncRefEdgeMode(enVsID, stCfg.enVsRefMd);
    Hal_SCLDAM_SetRegenVSyncWidth(enVsID, stCfg.u16Vs_Width);
    Hal_SCLDAM_SetRegenVSyncStartPoint(enVsID, stCfg.u16Vs_St);

    return TRUE;
}
#if SCLDMA_IRQ_EN
EN_SCLDMA_IRQ_MODE_TYPE _Drv_SCLDMA_Irq_Get_Mode(EN_SCLDMA_CLIENT_TYPE enClient)
{
    EN_SCLDMA_IRQ_MODE_TYPE enIrqMode;

    MsOS_ObtainMutex(_SCLDMA_Mutex , MSOS_WAIT_FOREVER);

    enIrqMode = genIrqMode[enClient];

    MsOS_ReleaseMutex(_SCLDMA_Mutex);

    return enIrqMode;
}


void _Drv_SCLDMA_Irq_Process(void)
{
    MS_U64 u64IrqFlags;
    MS_U64 u64IrqMask = (SCLIRQ_MSK_SC1_FRM_W_ACTIVE  | SCLIRQ_MSK_SC1_FRM_W_ACTIVE_N  |
                         SCLIRQ_MSK_SC1_SNP_W_ACTIVE  | SCLIRQ_MSK_SC1_SNP_W_ACTIVE_N  |
                         SCLIRQ_MSK_SC1_SNPI_W_ACTIVE | SCLIRQ_MSK_SC1_SNPI_W_ACTIVE_N |
                         SCLIRQ_MSK_SC2_FRM_W_ACTIVE  | SCLIRQ_MSK_SC2_FRM_W_ACTIVE_N  |
                         SCLIRQ_MSK_SC2_FRMI_W_ACTIVE | SCLIRQ_MSK_SC2_FRMI_W_ACTIVE_N |
                         SCLIRQ_MSK_SC3_ENG_FRM_END   | SCLIRQ_MSK_SC1_ENG_FRM_END     |
                         SCLIRQ_MSK_SC1_DBG_R_ACTIVE  | SCLIRQ_MSK_SC1_DBG_R_ACTIVE_N  |
                         SCLIRQ_MSK_SC3_DMA_R_ACTIVE  | SCLIRQ_MSK_SC3_DMA_R_ACTIVE_N  |
                         SCLIRQ_MSK_SC3_DMA_W_ACTIVE  | SCLIRQ_MSK_SC3_DMA_W_ACTIVE_N);


    MsOS_ObtainMutex(_SCLDMA_Mutex , MSOS_WAIT_FOREVER);

    Drv_SCLIRQ_Get_Flags_Msk(u64IrqMask, &u64IrqFlags);
    Drv_SCLIRQ_Set_Clear_Msk(u64IrqFlags);

    // SC1 FRM
    if(u64IrqFlags & SCLIRQ_MSK_SC1_FRM_W_ACTIVE)
    {
        genIrqMode[E_SCLDMA_1_FRM_W] = E_SCLDMA_IRQ_MODE_ACTIVE;
    }

    if(u64IrqFlags & SCLIRQ_MSK_SC1_FRM_W_ACTIVE_N)
    {
        genIrqMode[E_SCLDMA_1_FRM_W] |= E_SCLDMA_IRQ_MODE_ACTIVE_N;
    }

    // SC1 SNP
    if(u64IrqFlags & SCLIRQ_MSK_SC1_SNP_W_ACTIVE)
    {
        genIrqMode[E_SCLDMA_1_SNP_W] = E_SCLDMA_IRQ_MODE_ACTIVE;
    }

    if(u64IrqFlags & SCLIRQ_MSK_SC1_SNP_W_ACTIVE_N)
    {
        genIrqMode[E_SCLDMA_1_SNP_W] |= E_SCLDMA_IRQ_MODE_ACTIVE_N;
    }

    // SC1 IMI
    if(u64IrqFlags & SCLIRQ_MSK_SC1_SNPI_W_ACTIVE)
    {
        genIrqMode[E_SCLDMA_1_IMI_W] = E_SCLDMA_IRQ_MODE_ACTIVE;
    }

    if(u64IrqFlags & SCLIRQ_MSK_SC1_SNPI_W_ACTIVE_N)
    {
        genIrqMode[E_SCLDMA_1_IMI_W] |= E_SCLDMA_IRQ_MODE_ACTIVE_N;
    }


    // SC2 FRM
    if(u64IrqFlags & SCLIRQ_MSK_SC2_FRM_W_ACTIVE)
    {
        genIrqMode[E_SCLDMA_2_FRM_W] = E_SCLDMA_IRQ_MODE_ACTIVE;
    }

    if(u64IrqFlags & SCLIRQ_MSK_SC2_FRM_W_ACTIVE_N)
    {
        genIrqMode[E_SCLDMA_2_FRM_W] |= E_SCLDMA_IRQ_MODE_ACTIVE_N;
    }

    // SC2 IMI
    if(u64IrqFlags & SCLIRQ_MSK_SC2_FRMI_W_ACTIVE)
    {
        genIrqMode[E_SCLDMA_2_IMI_W] = E_SCLDMA_IRQ_MODE_ACTIVE;
    }

    if(u64IrqFlags & SCLIRQ_MSK_SC2_FRMI_W_ACTIVE_N)
    {
        genIrqMode[E_SCLDMA_2_IMI_W] |= E_SCLDMA_IRQ_MODE_ACTIVE_N;
    }

    if(u64IrqFlags & SCLIRQ_MSK_SC1_ENG_FRM_END)
    {
        genIrqMode[E_SCLDMA_1_FRM_W] |= E_SCLDMA_IRQ_MODE_END;
        genIrqMode[E_SCLDMA_1_SNP_W] |= E_SCLDMA_IRQ_MODE_END;
        genIrqMode[E_SCLDMA_1_IMI_W] |= E_SCLDMA_IRQ_MODE_END;
        genIrqMode[E_SCLDMA_2_FRM_W] |= E_SCLDMA_IRQ_MODE_END;
        genIrqMode[E_SCLDMA_2_IMI_W] |= E_SCLDMA_IRQ_MODE_END;
    }

    // SC3 DMA_R
    if(u64IrqFlags & SCLIRQ_MSK_SC3_DMA_R_ACTIVE)
    {
        genIrqMode[E_SCLDMA_3_FRM_R] = E_SCLDMA_IRQ_MODE_ACTIVE;
    }

    if(u64IrqFlags & SCLIRQ_MSK_SC3_DMA_R_ACTIVE_N)
    {
        genIrqMode[E_SCLDMA_3_FRM_R] |= E_SCLDMA_IRQ_MODE_ACTIVE_N;
    }

    // SC3 DMA_W
    if(u64IrqFlags & SCLIRQ_MSK_SC3_DMA_W_ACTIVE)
    {
        genIrqMode[E_SCLDMA_3_FRM_W] = E_SCLDMA_IRQ_MODE_ACTIVE;
    }

    if(u64IrqFlags & SCLIRQ_MSK_SC3_DMA_W_ACTIVE_N)
    {
        genIrqMode[E_SCLDMA_3_FRM_W] |= E_SCLDMA_IRQ_MODE_ACTIVE_N;
    }

    // SC3 DMA_DONE
    if(u64IrqFlags & SCLIRQ_MSK_SC3_ENG_FRM_END)
    {
        genIrqMode[E_SCLDMA_3_FRM_W] |= E_SCLDMA_IRQ_MODE_END;
    }

    // SC4 DMA R
    if(u64IrqFlags & SCLIRQ_MSK_SC1_DBG_R_ACTIVE)
    {
        genIrqMode[E_SCLDMA_4_FRM_R] = E_SCLDMA_IRQ_MODE_ACTIVE;
    }

    if(u64IrqFlags & SCLIRQ_MSK_SC1_DBG_R_ACTIVE_N)
    {
        genIrqMode[E_SCLDMA_4_FRM_R] |= E_SCLDMA_IRQ_MODE_ACTIVE_N;
    }


    MsOS_ReleaseMutex(_SCLDMA_Mutex);

}

void _Drv_SCLDMA_Irq_Set_Pending(EN_SCLDMA_CLIENT_TYPE enClient)
{
    MS_U32 u32Events = SET_SCLDMA_IRQ_EVENT_PENDING(enClient);
    MsOS_SetEvent(gstScldmaInfo.s32IrqEventId ,u32Events);
}

void _Drv_SCLDMA_Irq_Set_Clear(EN_SCLDMA_CLIENT_TYPE enClient, EN_SCLDMA_IRQ_MODE_TYPE enIrqMode, MS_U16 u16IrqNum)
{
    MS_U32 u32Events = SET_SCLDMA_IRQ_EVENT_CLEAR(enClient, enIrqMode, u16IrqNum);
    MsOS_SetEvent(gstScldmaInfo.s32IrqEventId ,u32Events);
}

static void _Drv_SCLDMA_Irq_Event_Task(void)
{
    #define DRVSCLDMA_EVENT_TASK_DBG 0
    MS_U32 u32Events;

    while(1)
    {
        MsOS_WaitEvent(gstScldmaInfo.s32IrqEventId, E_SCLDMA_IRQ_EVENT_ALL, &u32Events, E_OR_CLEAR, 5000);
        if((u32Events & SCLDMA_IRQ_EVENT_ID_MSK) == E_SCLDMA_IRQ_EVENT_TIMER)
        {
            _Drv_SCLDMA_Irq_Process();
        }
        else if((u32Events & SCLDMA_IRQ_EVENT_ID_MSK) == E_SCLDMA_IRQ_EVENT_PENDING)
        {
            MS_U16 u16Cleint = (MS_U16)GET_SCLDMA_IRQ_EVENT_CLIENT(u32Events);

            if(u16Cleint < E_SCLDMA_CLIENT_NUM)
            {
                MsOS_ObtainMutex(_SCLDMA_Mutex , MSOS_WAIT_FOREVER);
                genIrqMode[u16Cleint] = E_SCLDMA_IRQ_MODE_PENDING;
                MsOS_ReleaseMutex(_SCLDMA_Mutex);
            }
            else
            {
                DRV_SCLDMA_ERR(printf("[DRVSCLDMA]%s(%d):: Idx output of range:%d\n",
                    __FUNCTION__, __LINE__, u16Cleint));
            }
        }
        else if((u32Events & SCLDMA_IRQ_EVENT_ID_MSK) == E_SCLDMA_IRQ_EVENT_CLEAR)
        {
            MS_U16 u16Cleint = (MS_U16)GET_SCLDMA_IRQ_EVENT_CLIENT(u32Events);
            MS_U16 u16IrqNum = (MS_U16)GET_SCLDMA_IRQ_EVENT_CLEAR_IRQNUM(u32Events);
            EN_SCLDMA_IRQ_MODE_TYPE enIrqMode = GET_SCLDMA_IRQ_EVENT_CLEAR_MODE(u32Events);

            if(u16Cleint < E_SCLDMA_CLIENT_NUM && (enIrqMode & E_SCLDMA_IRQ_MODE_ALL) && (u16IrqNum < 48))
            {
                MsOS_ObtainMutex(_SCLDMA_Mutex , MSOS_WAIT_FOREVER);;
                genIrqMode[u16Cleint] &= ~enIrqMode;
                Drv_SCLIRQ_Set_Clear(u16IrqNum);
                MsOS_ReleaseMutex(_SCLDMA_Mutex);
            }
            else
            {
                DRV_SCLDMA_ERR(printf("[DRVSCLDMA]%s(%d):: client:%d irqmode:%d, irqnum=%d\n",
                    __FUNCTION__, __LINE__, u16Cleint, enIrqMode, u16IrqNum));
            }
        }
        else
        {
            DRV_SCLDMA_ERR(printf("[DRVSCLDMA]%s::Unknwon Event\n", __FUNCTION__));
        }
    }
}


static void _Drv_SCLDMA_CheckIrq_TimerCallback(MS_U32 stTimer, MS_U32 u32Data)
{
    MsOS_SetEvent(gstScldmaInfo.s32IrqEventId, E_SCLDMA_IRQ_EVENT_TIMER);
}


static MS_BOOL _Drv_SCLDMA_Check_RWDone(void)
{
    EN_SCLDMA_CLIENT_TYPE enClientType;
    MS_BOOL bRWDone;
    MS_U32 u32Time;

    bRWDone =0x1F;

    for(enClientType=E_SCLDMA_1_FRM_W; enClientType<E_SCLDMA_3_FRM_R; enClientType++)
    {
        if(gstScldmaInfo.enBuffMode[enClientType] == E_SCLDMA_BUF_MD_RING)
        {
            if(gstScldmaInfo.bDMAOnOff[enClientType] == 1)
            {

                DRV_SCLDMA_DBG(printf("[DRVSCLDMA]%s RING mode, %s acitve\n",
                    __FUNCTION__, PARSING_SCLDMA_CLIENT(enClientType)));
                bRWDone = bRWDone^(1<<enClientType);
            }
            else
            {
                bRWDone = bRWDone^(1<<enClientType);
            }
        }
        else if(gstScldmaInfo.enBuffMode[enClientType] == E_SCLDMA_BUF_MD_SINGLE)
        {   // single mode
            MS_BOOL bSignleDone;

           EN_SCLDMA_IRQ_MODE_TYPE enIrqModeMd;
            bSignleDone = FALSE;


            u32Time = ((MS_U32)MsOS_GetSystemTime());
            if((enClientType >= E_SCLDMA_1_FRM_W && enClientType <= E_SCLDMA_2_IMI_W) ||  enClientType == E_SCLDMA_3_FRM_W)
            {
                while(MsOS_Timer_DiffTimeFromNow(u32Time) < 20)
                {
                    enIrqModeMd = _Drv_SCLDMA_Irq_Get_Mode(enClientType);
                    if( (enIrqModeMd & E_SCLDMA_IRQ_MODE_DONE) == E_SCLDMA_IRQ_MODE_DONE )
                    {
                        bSignleDone = 1;
                        break;
                    }
                }
            }
            else
            {
                while(MsOS_Timer_DiffTimeFromNow(u32Time) < 20)
                {
                    if( (_Drv_SCLDMA_Irq_Get_Mode(enClientType) & E_SCLDMA_IRQ_MODE_DONE) == E_SCLDMA_IRQ_MODE_ACTIVE_N )
                    {
                        bSignleDone = 1;
                        break;
                    }
                }
            }

            if(bSignleDone == FALSE)
            {
                DRV_SCLDMA_DBG(printf("[DRVSCLDMA]%s SINGLE mode, %s acitve\n",
                    __FUNCTION__, PARSING_SCLDMA_CLIENT(enClientType)));
                bRWDone = bRWDone^(1<<enClientType);
            }
        }
        else
        {
            if(gstScldmaInfo.bDMAOnOff[enClientType])
            {
                DRV_SCLDMA_ERR(printf("[DRMSCLDMA]%s(%d) wrong BufferMode \n", __FUNCTION__, __LINE__));
            }
            else
            {
                bRWDone = bRWDone^(1<<enClientType);
            }
        }
    }

    DRV_SCLDMA_DBG(printf("[DRMSCLDMA]bRWDone:%hx \n",bRWDone));

    return bRWDone;
}


#else
static MS_U16 _Drv_SCLDMA_Get_Done_IRQNum(EN_SCLDMA_CLIENT_TYPE enClientType)
{
    MS_U16 u16IrqNum;

    switch(enClientType)
    {
    case E_SCLDMA_1_FRM_W:
    case E_SCLDMA_1_SNP_W:
    case E_SCLDMA_1_IMI_W:
    case E_SCLDMA_2_FRM_W:
    case E_SCLDMA_2_IMI_W:
        u16IrqNum = SCLIRQ_SC1_ENG_FRM_END;
        break;
    case E_SCLDMA_3_FRM_W:
        u16IrqNum = SCLIRQ_SC3_ENG_FRM_END;
        break;
    default:
        u16IrqNum = SCLIRQ_RESERVED;
        break;
    }

    return u16IrqNum;
}


static MS_BOOL _Drv_SCLDMA_Check_RWDone(void)
{
    EN_SCLDMA_CLIENT_TYPE enClientType;
    MS_BOOL bRWDone = 1;
    MS_BOOL bHSingle= 0;
    MS_BOOL bflag   = 0;
    MS_U32 u32Time;
    bRWDone = 0x1F;

    for(enClientType=E_SCLDMA_1_FRM_W; enClientType<E_SCLDMA_3_FRM_R; enClientType++)
    {
        if(_Is_RingMode(enClientType))
        {
            if(gstScldmaInfo.bDMAOnOff[enClientType] == 1)
            {

                DRV_SCLDMA_DBG(printf("[DRVSCLDMA]%s RING mode, %s acitve\n",
                    __FUNCTION__, PARSING_SCLDMA_CLIENT(enClientType)));
            }
            else
            {
                bRWDone = bRWDone^(1<<enClientType);
            }
        }
        else if(_Is_SingleMode(enClientType))
        {   // single mode
            if(_IsFlagType(enClientType,E_SCLDMA_FLAG_ACTIVE))
            {
                DRV_SCLDMA_DBG(printf("[DRVSCLDMA]%s Single mode, %s acitve\n",
                    __FUNCTION__, PARSING_SCLDMA_CLIENT(enClientType)));
                bHSingle=1;
                if(bHSingle)
                {
                    u32Time = ((MS_U32)MsOS_GetSystemTime());
                    while(MsOS_Timer_DiffTimeFromNow(u32Time) < 20)
                    {
                        bflag = 0;
                        if(_IsFlagType(enClientType,E_SCLDMA_FLAG_ACTIVE))
                        {
                            DRV_SCLDMA_DBG_H(printf("[DRMSCLDMA]!u64Active log...%d \n",enClientType));
                            bflag = 1;
                        }

                        if(!bflag)
                        {
                            bRWDone = bRWDone^(1<<enClientType);
                            break;
                        }
                    }
                }
            }
            else
            {
                bRWDone = bRWDone^(1<<enClientType);
            }
        }
        else
        {
            if(gstScldmaInfo.bDMAOnOff[enClientType])
            {
                DRV_SCLDMA_ERR(printf("[DRMSCLDMA]%s(%d) wrong BufferMode \n", __FUNCTION__, __LINE__));
            }
            else
            {
                bRWDone = bRWDone^(1<<enClientType);
            }
        }
    }
    u32Time = ((MS_U32)MsOS_GetSystemTime());
    DRV_SCLDMA_DBG(printf("[DRMSCLDMA]bRWDone:%hx, RW DiffTimeFromNow:%lu \n",bRWDone,MsOS_Timer_DiffTimeFromNow(u32Time)));
    return bRWDone;
}
#endif

#endif

#undef DRV_SCLDMA_C
