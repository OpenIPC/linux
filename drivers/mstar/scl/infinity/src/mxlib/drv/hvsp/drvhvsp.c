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
#define DRV_HVSP_C

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
#include "MsDbg.h"
#include <linux/kthread.h>
#include <linux/irq.h>
#include <linux/clk.h>
#include <linux/clk-provider.h>

#include "drvhvsp_st.h"
#include "halhvsp.h"
#include "drvhvsp.h"
#include "drvsclirq_st.h"
#include "drvsclirq.h"
#include "irqs.h"
#include "mdrv_scl_dbg.h"

//-------------------------------------------------------------------------------------------------
//  Defines & Macro
//-------------------------------------------------------------------------------------------------
#define DRV_HVSP_DBG(x)
#define DRV_HVSP_ERR(x)      x
#define DRV_HVSP_MUTEX_LOCK()            MsOS_ObtainMutex(_HVSP_Mutex,MSOS_WAIT_FOREVER)
#define DRV_HVSP_MUTEX_UNLOCK()          MsOS_ReleaseMutex(_HVSP_Mutex)
#define FHD_Width   1920
#define FHD_Height  1080
#define HD_Width    1280
#define HD_Height   720
#define PNL_Width   800
#define PNL_Height  480
#define Is_DNRBufferReady() (gbMemReadyForDrv)
#define Is_FrameBufferTooSmall(u32ReqMemSize,u32IPMMemSize) (u32ReqMemSize > u32IPMMemSize)
#define Is_PreCropWidthNeedToOpen() (gstSrcSize.u16Width > gstIPMCfg.u16Fetch)
#define Is_PreCropHeightNeedToOpen() (gstSrcSize.u16Height> gstIPMCfg.u16Vsize)
#define Is_InputSrcRotate() (gstSrcSize.u16Height > gstSrcSize.u16Width)
#define Is_IPM_NotSetReady() (gstIPMCfg.u16Fetch == 0 || gstIPMCfg.u16Vsize == 0)
#define Is_INPUTMUX_SetReady() (gstSrcSize.bSet == 1)
#define Is_PreCropNotNeedToOpen() ((gstIPMCfg.u16Fetch == gstSrcSize.u16Width) && \
    (gstIPMCfg.u16Vsize == gstSrcSize.u16Height))
#define VIP_HVSP_ID_SRAM_OFFSET 2
#define HVSP_RATIO(input, output)           ((input * 1048576) / output)
#define HVSP_CROP_RATIO(u16src, u16crop1, u16crop2)  ((MS_U16)(((MS_U32)u16crop2 * (MS_U32)u16crop1) / (MS_U32)u16src ))
#define HVSP_CROP_CHECK(u16croph,u16cropch,u16cropv,u16cropcv)  (((u16croph) != (u16cropch))|| ((u16cropv) != (u16cropcv)))
#define HVSP_DMA_CHECK(u16cropv,u16cropcv)  (((u16cropv) != (u16cropcv)))
#define _Change_SRAM_Qmap(enHVSP_ID,up)         (((gstScalinInfo[enHVSP_ID].stSizeAfterCrop.u16Height\
    > gstScalinInfo[enHVSP_ID].stSizeAfterScaling.u16Height)&&up) || (((gstScalinInfo[enHVSP_ID].stSizeAfterCrop.u16Height\
    < gstScalinInfo[enHVSP_ID].stSizeAfterScaling.u16Height) && !up)))
//-------------------------------------------------------------------------------------------------
//  Variable
//-------------------------------------------------------------------------------------------------
ST_HVSP_SCALING_INFO gstScalinInfo[E_HVSP_ID_MAX];
ST_HVSP_IPM_CONFIG   gstIPMCfg = {0};
ST_HVSP_SIZE_CONFIG gstSrcSize;
/////////////////
/// gbMemReadyForDrv
/// if True ,DNR buffer was allocated.
////////////////
MS_BOOL gbMemReadyForDrv;
/////////////////
/// bLDCEn
/// if True ,LDC is open.(To sync VIP and HVSP driver)
////////////////
MS_BOOL gbLDCEn=0;
MS_U8 gu8FBBufferNum ;
/////////////////
/// gbHvspSuspend
/// To Save suspend status.
////////////////
MS_BOOL gbHvspSuspend = 0;
/////////////////
/// bclkforcemode
/// use in Drvscldma and Drvhvsp
/// if True ,can't dynamic set Clk
////////////////
unsigned char gbclkforcemode = 0;//extern
MS_S32 _HVSP_Mutex = -1;

//-------------------------------------------------------------------------------------------------
//  Private Functions
//-------------------------------------------------------------------------------------------------
void _Drv_HVSP_InitSWVarialbe(EN_HVSP_ID_TYPE HVSP_IP)
{
    MsOS_Memset(&gstScalinInfo[HVSP_IP], 0, sizeof(ST_HVSP_SCALING_INFO));
    if(HVSP_IP == E_HVSP_ID_1)
    {
        MsOS_Memset(&gstIPMCfg, 0, sizeof(ST_HVSP_IPM_CONFIG));
        gbLDCEn = 0;
        gstSrcSize.u16Height    = FHD_Height;
        gstSrcSize.u16Width     = FHD_Width;
        gstSrcSize.bSet         = 0;
    }
}

ST_HVSP_CROP_INFO _Drv_HVSP_FillPreCropInfo(void)
{
    ST_HVSP_CROP_INFO stCropInfo;
    if(Is_IPM_NotSetReady() || Is_PreCropNotNeedToOpen())
    {
        if(Is_IPM_NotSetReady())
        {
            DRV_HVSP_ERR(printf("[DRVHVSP]%s %d:: IPM without setting\n", __FUNCTION__, __LINE__));
        }
        stCropInfo.bEn = 0;
        stCropInfo.u16Vst = 0;
        stCropInfo.u16Hst = 0;
        stCropInfo.u16Hsize = 0;
        stCropInfo.u16Vsize = 0;
    }
    else
    {
        stCropInfo.bEn = 1;
        if(Is_InputSrcRotate())//rotate
        {
            stCropInfo.u16Hst = 0;
            stCropInfo.u16Vst = 0;
        }
        else if(Is_PreCropWidthNeedToOpen() || Is_PreCropHeightNeedToOpen())
        {
            if(Is_PreCropWidthNeedToOpen())
            {
                stCropInfo.u16Hst      = (gstSrcSize.u16Width - gstIPMCfg.u16Fetch)/2;
            }
            if(Is_PreCropHeightNeedToOpen())
            {
                stCropInfo.u16Vst      = (gstSrcSize.u16Height - gstIPMCfg.u16Vsize)/2;
            }
        }
        else
        {
            stCropInfo.u16Hst = 0;
            stCropInfo.u16Vst = 0;
        }
        stCropInfo.u16Hsize    = gstIPMCfg.u16Fetch;
        stCropInfo.u16Vsize    = gstIPMCfg.u16Vsize;
    }
    // crop1
    stCropInfo.u16In_hsize = gstSrcSize.u16Width;
    stCropInfo.u16In_vsize = gstSrcSize.u16Height;
    if(stCropInfo.u16In_hsize == 0)
    {
        stCropInfo.u16In_hsize = FHD_Width;
    }
    if(stCropInfo.u16In_vsize == 0)
    {
        stCropInfo.u16In_vsize = FHD_Height;
    }
    return stCropInfo;
}
void _Drv_HVSP_SetCoringThrdOn(EN_HVSP_ID_TYPE enHVSP_ID)
{
    Hal_HVSP_SetHspCoringThrdC(enHVSP_ID,0x1);
    Hal_HVSP_SetHspCoringThrdY(enHVSP_ID,0x1);
    Hal_HVSP_SetVspCoringThrdC(enHVSP_ID,0x1);
    Hal_HVSP_SetVspCoringThrdY(enHVSP_ID,0x1);
}


//-------------------------------------------------------------------------------------------------
//  Public Functions
//-------------------------------------------------------------------------------------------------
MS_BOOL Drv_HVSP_SetCMDQTrigType(ST_HVSP_CMD_TRIG_CONFIG stCmdTrigCfg)
{
    Hal_HVSP_SetCMDQTrigCfg(stCmdTrigCfg);
    Hal_HVSP_SetCMDQTrigFrameCnt(0);
    return TRUE;
}

MS_BOOL Drv_HVSP_GetCMDQTrigType(ST_HVSP_CMD_TRIG_CONFIG *pCfg)
{
    Hal_HVSP_GetCMDQTrigCfg(pCfg);
    return TRUE;
}

MS_BOOL Drv_HVSP_GetCMDQDoneStatus(void)
{
    MS_U32 u32Events;
    MS_U32 u32Time;
    static MS_BOOL sbDone = 0;
    u32Time = ((MS_U32)MsOS_GetSystemTime());
    MsOS_ObtainMutex(_HVSP_Mutex, MSOS_WAIT_FOREVER);
    u32Events = MsOS_GetEvent(Drv_SCLIRQ_Get_IRQ_SYNCEventID()); // get status: FRM END
    if(u32Events& E_SCLIRQ_EVENT_CMDQGOING)
    {
        sbDone = 0;
    }
    if(u32Events &E_SCLIRQ_EVENT_CMDQDONE)
    {
        MsOS_ClearEventIRQ(Drv_SCLIRQ_Get_IRQ_SYNCEventID(),u32Events);
        sbDone = 1;
        MsOS_ReleaseMutex(_HVSP_Mutex);
        SCL_DBG(SCL_DBG_LV_DRVCMDQ()&EN_DBGMG_CMDQEVEL_NORMAL, "[drvhvsp]CMDQ done @%lu\n", u32Time);
        return 1;
    }
    else if (sbDone)
    {
        MsOS_ReleaseMutex(_HVSP_Mutex);
        SCL_DBG(SCL_DBG_LV_DRVCMDQ()&EN_DBGMG_CMDQEVEL_NORMAL, "[drvhvsp]CMDQ done(sb) @%lu\n", u32Time);
        return 1;
    }
    MsOS_ReleaseMutex(_HVSP_Mutex);
    return 0;
}

wait_queue_head_t * Drv_HVSP_GetWaitQueueHead(void)
{
    return Drv_SCLIRQ_GetSyncQueue();
}
void Drv_HVSP_Release(EN_HVSP_ID_TYPE HVSP_IP)
{
    Hal_HVSP_Set_Reset();
    _Drv_HVSP_InitSWVarialbe(HVSP_IP);
}
MS_BOOL Drv_HVSP_Suspend(ST_HVSP_SUSPEND_RESUME_CONFIG *pCfg)
{
    ST_SCLIRQ_SUSPEND_RESUME_CONFIG stSclirq;
    MS_BOOL bRet = TRUE;

    MsOS_ObtainMutex(_HVSP_Mutex, MSOS_WAIT_FOREVER);
    Hal_HVSP_Set_Reset();
    if(gbHvspSuspend == 0)
    {
        stSclirq.u32IRQNUM =  pCfg->u32IRQNUM;
        stSclirq.u32CMDQIRQNUM =  pCfg->u32CMDQIRQNUM;
        if(Drv_SCLIRQ_Suspend(&stSclirq))
        {
            bRet = TRUE;
            gbHvspSuspend = 1;
        }
        else
        {
            bRet = FALSE;
            DRV_HVSP_ERR(printf("[DRVHVSP]%s(%d) Suspend IRQ Fail\n", __FUNCTION__, __LINE__));
        }
    }
    else
    {
        SCL_DBG(SCL_DBG_LV_DRVHVSP()&(Get_DBGMG_HVSP(3)), "[DRVHVSP]%s(%d) alrady suspned\n", __FUNCTION__, __LINE__);
        bRet = TRUE;
    }

    MsOS_ReleaseMutex(_HVSP_Mutex);

    return bRet;
}

MS_BOOL Drv_HVSP_Resume(ST_HVSP_SUSPEND_RESUME_CONFIG *pCfg)
{
    ST_SCLIRQ_SUSPEND_RESUME_CONFIG stSclirq;
    MS_BOOL bRet = TRUE;

    MsOS_ObtainMutex(_HVSP_Mutex, MSOS_WAIT_FOREVER);

    //printf("%s,(%d) %d\n", __FUNCTION__, __LINE__, gbHvspSuspend);
    if(gbHvspSuspend == 1)
    {
        stSclirq.u32IRQNUM =  pCfg->u32IRQNUM;
        stSclirq.u32CMDQIRQNUM =  pCfg->u32CMDQIRQNUM;
        if(Drv_SCLIRQ_Resume(&stSclirq))
        {
            Drv_SCLIRQ_InterruptEnable(SCLIRQ_VSYNC_FCLK_LDC);
            Hal_HVSP_SetTestPatCfg();
            gbHvspSuspend = 0;
            bRet = TRUE;
        }
        else
        {

            DRV_HVSP_ERR(printf("[DRVHVSP]%s(%d) Resume IRQ Fail\n", __FUNCTION__, __LINE__));
            bRet = FALSE;
        }
    }
    else
    {
        SCL_DBG(SCL_DBG_LV_DRVHVSP()&(Get_DBGMG_HVSP(3)), "[DRVHVSP]%s(%d) alrady resume\n", __FUNCTION__, __LINE__);
        bRet = TRUE;
    }

    MsOS_ReleaseMutex(_HVSP_Mutex);

    return bRet;
}
void Drv_HVSP_SetBufferNum(MS_U8 u8Num)
{
    gu8FBBufferNum = u8Num;
}
MS_BOOL Drv_HVSP_GetBufferNum(void)
{
    return gu8FBBufferNum;
}
MS_BOOL Drv_HVSP_Init(ST_HVSP_INIT_CONFIG *pInitCfg)
{
    char word[] = {"_HVSP_Mutex"};
    ST_SCLIRQ_INIT_CONFIG stIRQInitCfg;
    MS_U8 u8IDidx;

    if(_HVSP_Mutex != -1)
    {
        SCL_DBG(SCL_DBG_LV_DRVHVSP()&(Get_DBGMG_HVSP(3)), "[DRVHVSP]%s(%d) alrady done\n", __FUNCTION__, __LINE__);
        return TRUE;
    }

    if(MsOS_Init() == FALSE)
    {
        DRV_HVSP_ERR(printf("[DRVHVSP]%s(%d) MsOS_Init Fail\n", __FUNCTION__, __LINE__));
        return FALSE;
    }

    _HVSP_Mutex = MsOS_CreateMutex(E_MSOS_FIFO, word, MSOS_PROCESS_SHARED);

    if (_HVSP_Mutex == -1)
    {
        DRV_HVSP_ERR(printf("[DRVHVSP]%s(%d): create mutex fail\n", __FUNCTION__, __LINE__));
        return FALSE;
    }

    stIRQInitCfg.u32RiuBase = pInitCfg->u32RIUBase;
    stIRQInitCfg.u32IRQNUM  = pInitCfg->u32IRQNUM;
    stIRQInitCfg.u32CMDQIRQNUM  = pInitCfg->u32CMDQIRQNUM;
    if(Drv_SCLIRQ_Init(&stIRQInitCfg) == FALSE)
    {
        DRV_HVSP_ERR(printf("[DRVHVSP]%s(%d) Init IRQ Fail\n", __FUNCTION__, __LINE__));
        return FALSE;
    }

    Hal_HVSP_SetRiuBase(pInitCfg->u32RIUBase);
    DRV_HVSP_MUTEX_LOCK();
    Hal_HVSP_Set_Reset();
    for(u8IDidx = E_HVSP_ID_1; u8IDidx<E_HVSP_ID_MAX; u8IDidx++)
    {
        _Drv_HVSP_InitSWVarialbe(u8IDidx);
        _Drv_HVSP_SetCoringThrdOn(u8IDidx);
    }
    Drv_HVSP_SetCMDQTrigTypeByRIU();
    DRV_HVSP_MUTEX_UNLOCK();
    Drv_SCLIRQ_InterruptEnable(SCLIRQ_VSYNC_FCLK_LDC);
    Hal_HVSP_SetTestPatCfg();
    Hal_HVSP_SetVpsSRAMEn(1);
    Hal_HVSP_SetFrameBufferManageLock(0);
    Hal_HVSP_SetIPMBufferNumber(gu8FBBufferNum);
    return TRUE;
}

ST_HVSP_CMD_TRIG_CONFIG Drv_HVSP_SetCMDQTrigTypeByRIU(void)
{
    ST_HVSP_CMD_TRIG_CONFIG stCmdTrigCfg,stori;
    Drv_HVSP_GetCMDQTrigType(&stori);
    stCmdTrigCfg.enType     = E_HVSP_CMD_TRIG_NONE;
    stCmdTrigCfg.u8Fmcnt    = 0;
    Drv_HVSP_SetCMDQTrigType(stCmdTrigCfg);
    return stori;
}
void _Drv_HVSP_SetPreCropWhenInputMuxReady(void)
{
    ST_HVSP_CROP_INFO stCropInfo_1;
    if(Is_INPUTMUX_SetReady())
    {
        stCropInfo_1 = _Drv_HVSP_FillPreCropInfo();
        Hal_HVSP_SetCropConfig(E_HVSP_CROP_ID_1, stCropInfo_1);
    }
}
MS_BOOL Drv_HVSP_Set_IPM_Config(ST_HVSP_IPM_CONFIG stCfg)
{
    ST_HVSP_CMD_TRIG_CONFIG stCmdTrigCfg;
    SCL_DBG(SCL_DBG_LV_DRVHVSP()&(Get_DBGMG_HVSP(0)), "[DRVHVSP]%s(%d): Base=%lx, Fetch=%d, Vsize=%d\n"
        , __FUNCTION__, __LINE__, stCfg.u32BaseAddr, stCfg.u16Fetch, stCfg.u16Vsize);
    MsOS_Memcpy(&gstIPMCfg, &stCfg, sizeof(ST_HVSP_IPM_CONFIG));
    DRV_HVSP_MUTEX_LOCK();
    stCmdTrigCfg = Drv_HVSP_SetCMDQTrigTypeByRIU();
    _Drv_HVSP_SetPreCropWhenInputMuxReady();
    Hal_HVSP_SetBT656SrcConfig(1,stCfg.u16Fetch);
    Hal_HVSP_SetIPMBase(stCfg.u32BaseAddr);
    Hal_HVSP_SetIPMFetchNum(stCfg.u16Fetch);
    Hal_HVSP_SetIPMLineOffset(stCfg.u16Fetch);
    Hal_HVSP_SetIPMvSize(stCfg.u16Vsize);
    Hal_HVSP_SetIPMReadEn(stCfg.bRead);
    Hal_HVSP_SetIPMWriteEn(stCfg.bWrite);
    Drv_HVSP_SetCMDQTrigType(stCmdTrigCfg);
    DRV_HVSP_MUTEX_UNLOCK();
    return TRUE;
}
void Drv_HVSP_SetFbManageConfig(ST_DRV_HVSP_SET_FB_MANAGE_CONFIG stCfg)
{
    (printf("[DRVHVSP]%s(%d):%x \n",
        __FUNCTION__, __LINE__, stCfg.enSet));
    Hal_HVSP_SetFrameBufferManageLock(0);
    if(stCfg.enSet & EN_DRV_HVSP_FBMG_SET_LDCPATH_ON)
    {
        Hal_HVSP_SetLDCPathSel(1);
        printf("LDC ON\n");
    }
    else if(stCfg.enSet & EN_DRV_HVSP_FBMG_SET_LDCPATH_OFF)
    {
        Hal_HVSP_SetLDCPathSel(0);
        printf("LDC OFF\n");
    }
    if(stCfg.enSet & EN_DRV_HVSP_FBMG_SET_DNR_Read_ON)
    {
        Hal_HVSP_SetIPMReadEn(1);
        printf("DNRR ON\n");
    }
    else if(stCfg.enSet & EN_DRV_HVSP_FBMG_SET_DNR_Read_OFF)
    {
        Hal_HVSP_SetIPMReadEn(0);
        printf("DNRR OFF\n");
    }
    if(stCfg.enSet & EN_DRV_HVSP_FBMG_SET_DNR_Write_ON)
    {
        Hal_HVSP_SetIPMWriteEn(1);
        printf("DNRW ON\n");
    }
    else if(stCfg.enSet & EN_DRV_HVSP_FBMG_SET_DNR_Write_OFF)
    {
        Hal_HVSP_SetIPMWriteEn(0);
        printf("DNRW OFF\n");
    }
    if(stCfg.enSet & EN_DRV_HVSP_FBMG_SET_DNR_BUFFER_1)
    {
        Hal_HVSP_SetIPMBufferNumber(1);
        printf("DNRB 1\n");
    }
    else if(stCfg.enSet & EN_DRV_HVSP_FBMG_SET_DNR_BUFFER_2)
    {
        Hal_HVSP_SetIPMBufferNumber(2);
        printf("DNRB 2\n");
    }
    Hal_HVSP_SetFrameBufferManageLock(1);
    if(stCfg.enSet & EN_DRV_HVSP_FBMG_SET_UNLOCK)
    {
        Hal_HVSP_SetFrameBufferManageLock(0);
        printf("UNLOCK\n");
    }
}

void _Drv_HVSP_SetCMDQTrigFire(ST_HVSP_CMD_TRIG_CONFIG stCmdTrigCfg,EN_HVSP_ID_TYPE HVSP_IP)
{
    MS_U32 u32Time;
    MS_U32 u32Events = 0;
    u32Time = ((MS_U32)MsOS_GetSystemTime());
    if(stCmdTrigCfg.enType != E_HVSP_CMD_TRIG_NONE)
    {
        if(stCmdTrigCfg.enType == E_HVSP_CMD_TRIG_POLL_LDC_SYNC)
        {
            while(1)
            {
                if(HVSP_IP == E_HVSP_ID_3)
                {
                    MsOS_WaitEvent(Drv_SCLIRQ_Get_IRQ_SC3EventID(),
                        E_SCLIRQ_SC3EVENT_HVSPST, &u32Events, E_OR, MSOS_WAIT_FOREVER); // get status: FRM END
                    MsOS_ClearEventIRQ(Drv_SCLIRQ_Get_IRQ_SC3EventID(),E_SCLIRQ_SC3EVENT_HVSPST);
                    break;
                }
                else
                {
                    MsOS_WaitEvent(Drv_SCLIRQ_Get_IRQ_EventID(),
                        E_SCLIRQ_EVENT_HVSPST, &u32Events, E_OR, MSOS_WAIT_FOREVER); // get status: FRM END
                    MsOS_ClearEventIRQ(Drv_SCLIRQ_Get_IRQ_EventID(),E_SCLIRQ_EVENT_HVSPST);
                    break;
                }
                if((MS_U32)MsOS_GetSystemTime() - u32Time > 50)
                {
                    break;
                }
            }
        }
        Hal_HVSP_SetCMDQTrigFire();
        SCL_DBG(SCL_DBG_LV_DRVHVSP()&(Get_DBGMG_HVSP(HVSP_IP)),
            "[DRVHVSP]%s id:%d @:%lu\n", __FUNCTION__,HVSP_IP,(MS_U32)MsOS_GetSystemTime());
    }
}
void Drv_HVSP_SetMemoryAllocateReady(MS_BOOL bEn)
{
    gbMemReadyForDrv = bEn;
}
void _Drv_HVSP_SetHorizotnalScalingConfig(EN_HVSP_ID_TYPE enHVSP_ID,MS_BOOL bEn)
{

    Hal_HVSP_SetScalingHoEn(enHVSP_ID, bEn);
    Hal_HVSP_SetScalingHoFacotr(enHVSP_ID, bEn ? gstScalinInfo[enHVSP_ID].u32ScalingRatio_H: 0);
    Hal_HVSP_SetModeYHo(enHVSP_ID,bEn ? E_HVSP_FILTER_MODE_SRAM_0: E_HVSP_FILTER_MODE_BYPASS);
    Hal_HVSP_SetModeCHo(enHVSP_ID,bEn ? E_HVSP_FILTER_MODE_BILINEAR: E_HVSP_FILTER_MODE_BYPASS,E_HVSP_SRAM_SEL_0);
    Hal_HVSP_SetHspDithEn(enHVSP_ID,bEn);
    Hal_HVSP_SetHspCoringEnC(enHVSP_ID,bEn);
    Hal_HVSP_SetHspCoringEnY(enHVSP_ID,bEn);
}
void _Drv_HVSP_SetVerticalScalingConfig(EN_HVSP_ID_TYPE enHVSP_ID,MS_BOOL bEn)
{
    Hal_HVSP_SetScalingVeEn(enHVSP_ID, bEn);
    Hal_HVSP_SetScalingVeFactor(enHVSP_ID,  bEn ? gstScalinInfo[enHVSP_ID].u32ScalingRatio_V: 0);
    Hal_HVSP_SetModeYVe(enHVSP_ID,bEn ? E_HVSP_FILTER_MODE_SRAM_0: E_HVSP_FILTER_MODE_BYPASS);
    Hal_HVSP_SetModeCVe(enHVSP_ID,bEn ? E_HVSP_FILTER_MODE_BILINEAR: E_HVSP_FILTER_MODE_BYPASS,E_HVSP_SRAM_SEL_0);
    Hal_HVSP_SetVspDithEn(enHVSP_ID,bEn);
    Hal_HVSP_SetVspCoringEnC(enHVSP_ID,bEn);
    Hal_HVSP_SetVspCoringEnY(enHVSP_ID,bEn);
}
MS_BOOL Drv_HVSP_SetScaling(EN_HVSP_ID_TYPE enHVSP_ID, ST_HVSP_SCALING_CONFIG stCfg, ST_HVSP_CLK_CONFIG* stclk)
{
    static MS_BOOL bScalingup[E_HVSP_ID_MAX] ={1,0,0};
    if(enHVSP_ID == E_HVSP_ID_1)
    {
        ST_HVSP_CROP_INFO stCropInfo_2;
        MS_U32 u32FrameBufferMemSize = gstIPMCfg.u32MemSize;
        MS_U32 u32ReqMemSize = stCfg.bCropEn[DRV_HVSP_CROP_1] ?
                                 stCfg.u16Crop_Width[DRV_HVSP_CROP_1] * stCfg.u16Crop_Height[DRV_HVSP_CROP_1] * 4 :
                                 stCfg.u16Src_Width * stCfg.u16Src_Height * 2 * Drv_HVSP_GetBufferNum();

        SCL_DBG(SCL_DBG_LV_DRVHVSP()&(Get_DBGMG_HVSP(enHVSP_ID)), "[DRVHVSP]%s id:%d @:%lu\n",
            __FUNCTION__,enHVSP_ID,(MS_U32)MsOS_GetSystemTime());

        if(Is_DNRBufferReady())
        {
            if(Is_FrameBufferTooSmall(u32ReqMemSize, u32FrameBufferMemSize))
            {
                DRV_HVSP_ERR(printf("[DRVHVSP]%s %d:: Out of memory\n", __FUNCTION__, __LINE__));
                return FALSE;
            }
        }
        else
        {
            DRV_HVSP_ERR(printf("[DRVHVSP]%s %d:: Can't Use DNR (FB Not Ready)\n", __FUNCTION__, __LINE__));
            if(Is_FrameBufferTooSmall(u32ReqMemSize, u32FrameBufferMemSize))
            {
                DRV_HVSP_ERR(printf("[DRVHVSP]%s %d:: Out of memory\n", __FUNCTION__, __LINE__));
            }
        }
        if(stCfg.u16Src_Width != gstIPMCfg.u16Fetch)
        {
            DRV_HVSP_ERR(printf("[DRVHVSP]%s %d:: H Src Size Mismatch(%hd,%hd)\n",
                __FUNCTION__, __LINE__,stCfg.u16Src_Width,gstIPMCfg.u16Fetch));
        }
        if(stCfg.u16Src_Height != gstIPMCfg.u16Vsize)
        {
            DRV_HVSP_ERR(printf("[DRVHVSP]%s %d:: V Src Size Mismatch(%hd,%hd)\n",
                __FUNCTION__, __LINE__,stCfg.u16Src_Height,gstIPMCfg.u16Vsize));
        }

        // setup cmd trig config
        DRV_HVSP_MUTEX_LOCK();
        Drv_HVSP_SetCMDQTrigType(stCfg.stCmdTrigCfg);

        if(stCfg.bCropEn[DRV_HVSP_CROP_2])
        {
            // crop2
            stCropInfo_2.bEn         = stCfg.bCropEn[DRV_HVSP_CROP_2];
            if(gstIPMCfg.u16Fetch &&gstIPMCfg.u16Vsize)
            {
                stCropInfo_2.u16In_hsize = gstIPMCfg.u16Fetch;
                stCropInfo_2.u16In_vsize = gstIPMCfg.u16Vsize;
            }
            else
            {
                stCropInfo_2.u16In_hsize = stCfg.u16Src_Width;
                stCropInfo_2.u16In_vsize = stCfg.u16Src_Height;
            }
            stCropInfo_2.u16Hsize    = stCfg.u16Crop_Width[DRV_HVSP_CROP_2];
            stCropInfo_2.u16Vsize    = stCfg.u16Crop_Height[DRV_HVSP_CROP_2];
            stCropInfo_2.u16Hst      = stCfg.u16Crop_X[DRV_HVSP_CROP_2];
            stCropInfo_2.u16Vst      = stCfg.u16Crop_Y[DRV_HVSP_CROP_2];
        }
        else
        {
            stCropInfo_2.bEn         = 0;
            if(gstIPMCfg.u16Fetch &&gstIPMCfg.u16Vsize)
            {
                stCropInfo_2.u16In_hsize = gstIPMCfg.u16Fetch;
                stCropInfo_2.u16In_vsize = gstIPMCfg.u16Vsize;
                stCropInfo_2.u16Hsize    = gstIPMCfg.u16Fetch;//stCfg.u16Src_Width;
                stCropInfo_2.u16Vsize    = gstIPMCfg.u16Vsize;//stCfg.u16Src_Height;
            }
            else
            {
                stCropInfo_2.u16In_hsize = stCfg.u16Src_Width;
                stCropInfo_2.u16In_vsize = stCfg.u16Src_Height;
                stCropInfo_2.u16Hsize    = stCfg.u16Src_Width;//stCfg.u16Src_Width;
                stCropInfo_2.u16Vsize    = stCfg.u16Src_Height;//stCfg.u16Src_Height;
            }
            stCropInfo_2.u16Hst      = 0;
            stCropInfo_2.u16Vst      = 0;
        }

        if(stCfg.bCropEn[DRV_HVSP_CROP_2])
        {
            gstScalinInfo[enHVSP_ID].stSizeAfterCrop.u16Width  = stCfg.u16Crop_Width[DRV_HVSP_CROP_2];
            gstScalinInfo[enHVSP_ID].stSizeAfterCrop.u16Height = stCfg.u16Crop_Height[DRV_HVSP_CROP_2];
        }
        else
        {
            gstScalinInfo[enHVSP_ID].stSizeAfterCrop.u16Width  = stCropInfo_2.u16In_hsize;//stCfg.u16Src_Width;
            gstScalinInfo[enHVSP_ID].stSizeAfterCrop.u16Height = stCropInfo_2.u16In_vsize;//stCfg.u16Src_Height;
        }

        gstScalinInfo[enHVSP_ID].stSizeAfterScaling.u16Width  = stCfg.u16Dsp_Width;
        gstScalinInfo[enHVSP_ID].stSizeAfterScaling.u16Height = stCfg.u16Dsp_Height;

        // Crop2
        Hal_HVSP_SetCropConfig(E_HVSP_CROP_ID_2, stCropInfo_2);

        // NLM size
        Hal_HVSP_SetNLMSize(gstScalinInfo[enHVSP_ID].stSizeAfterCrop.u16Width,
                gstScalinInfo[enHVSP_ID].stSizeAfterCrop.u16Height);
        Hal_HVSP_SetNLMLineBufferSize(gstScalinInfo[enHVSP_ID].stSizeAfterCrop.u16Width,
                gstScalinInfo[enHVSP_ID].stSizeAfterCrop.u16Height);
        Hal_HVSP_SetNLMEn(1);

    }
    else
    {
        DRV_HVSP_MUTEX_LOCK();
        Drv_HVSP_SetCMDQTrigType(stCfg.stCmdTrigCfg);
        gstScalinInfo[enHVSP_ID].stSizeAfterCrop.u16Width      = stCfg.u16Src_Width;
        gstScalinInfo[enHVSP_ID].stSizeAfterCrop.u16Height     = stCfg.u16Src_Height;
        gstScalinInfo[enHVSP_ID].stSizeAfterScaling.u16Width   = stCfg.u16Dsp_Width;
        gstScalinInfo[enHVSP_ID].stSizeAfterScaling.u16Height  = stCfg.u16Dsp_Height;
    }

    gstScalinInfo[enHVSP_ID].u32ScalingRatio_H = (MS_U32)HVSP_RATIO(gstScalinInfo[enHVSP_ID].stSizeAfterCrop.u16Width,
                                                            gstScalinInfo[enHVSP_ID].stSizeAfterScaling.u16Width);

    gstScalinInfo[enHVSP_ID].u32ScalingRatio_V = (MS_U32)HVSP_RATIO(gstScalinInfo[enHVSP_ID].stSizeAfterCrop.u16Height,
                                                            gstScalinInfo[enHVSP_ID].stSizeAfterScaling.u16Height);

    SCL_DBG(SCL_DBG_LV_DRVHVSP()&(Get_DBGMG_HVSP(enHVSP_ID)),
        "[DRVHVSP]%s(%d):: HVSP_%d, AfterCrop(%d, %d)\n", __FUNCTION__, __LINE__,
        enHVSP_ID, gstScalinInfo[enHVSP_ID].stSizeAfterCrop.u16Width, gstScalinInfo[enHVSP_ID].stSizeAfterCrop.u16Height);
    SCL_DBG(SCL_DBG_LV_DRVHVSP()&(Get_DBGMG_HVSP(enHVSP_ID)),
        "[DRVHVSP]%s(%d):: HVSP_%d, AfterScaling(%d, %d)\n", __FUNCTION__, __LINE__,
        enHVSP_ID, gstScalinInfo[enHVSP_ID].stSizeAfterScaling.u16Width, gstScalinInfo[enHVSP_ID].stSizeAfterScaling.u16Height);
    SCL_DBG(SCL_DBG_LV_DRVHVSP()&(Get_DBGMG_HVSP(enHVSP_ID)),
        "[DRVHVSP]%s(%d):: HVSP_%d, Ratio(%lx, %lx)\n", __FUNCTION__, __LINE__,
        enHVSP_ID, gstScalinInfo[enHVSP_ID].u32ScalingRatio_H, gstScalinInfo[enHVSP_ID].u32ScalingRatio_V);

    // horizotnal HVSP Scaling
    if(gstScalinInfo[enHVSP_ID].stSizeAfterCrop.u16Width == gstScalinInfo[enHVSP_ID].stSizeAfterScaling.u16Width)
    {
        _Drv_HVSP_SetHorizotnalScalingConfig(enHVSP_ID, FALSE);
    }
    else
    {
        _Drv_HVSP_SetHorizotnalScalingConfig(enHVSP_ID, TRUE);
    }

    // vertical HVSP Scaling
    if(gstScalinInfo[enHVSP_ID].stSizeAfterCrop.u16Height == gstScalinInfo[enHVSP_ID].stSizeAfterScaling.u16Height)
    {
        _Drv_HVSP_SetVerticalScalingConfig(enHVSP_ID, FALSE);
    }
    else
    {
        _Drv_HVSP_SetVerticalScalingConfig(enHVSP_ID, TRUE);
    }
    if(_Change_SRAM_Qmap(enHVSP_ID,bScalingup[enHVSP_ID]))
    {
        bScalingup[enHVSP_ID] = (bScalingup[enHVSP_ID]^1);//XOR 1 :reverse
        Hal_VIP_SRAM_Dump(enHVSP_ID+VIP_HVSP_ID_SRAM_OFFSET,bScalingup[enHVSP_ID]); //level 1 :up 0:down
        SCL_DBG(SCL_DBG_LV_DRVHVSP()&(Get_DBGMG_HVSP(enHVSP_ID)), "[DRVHVSP]Change Qmap SRAM id:%d scaling UP:%hhu\n",
            enHVSP_ID,bScalingup[enHVSP_ID]);
    }

    // HVSP In size
    Hal_HVSP_SetHVSPInputSize(enHVSP_ID,
                                 gstScalinInfo[enHVSP_ID].stSizeAfterCrop.u16Width,
                                 gstScalinInfo[enHVSP_ID].stSizeAfterCrop.u16Height);

    // HVSP Out size
    Hal_HVSP_SetHVSPOutputSize(enHVSP_ID,
                                  gstScalinInfo[enHVSP_ID].stSizeAfterScaling.u16Width,
                                  gstScalinInfo[enHVSP_ID].stSizeAfterScaling.u16Height);
    if(enHVSP_ID == E_HVSP_ID_3)
    {
        if(!gbclkforcemode)
        {
            Hal_HVSP_FCLK2(stclk);
        }
    }
    else
    {
        if(!gbclkforcemode)
        {
            Hal_HVSP_FCLK1(stclk);
        }
    }
    _Drv_HVSP_SetCMDQTrigFire(stCfg.stCmdTrigCfg,enHVSP_ID);
    DRV_HVSP_MUTEX_UNLOCK();
    return TRUE;
}
void Drv_HVSP_SetInputSrcSize(MS_U16 u16Height,MS_U16 u16Width)
{
    if(u16Height > 0)
    {
        gstSrcSize.u16Height    = u16Height;
    }
    if(u16Width > 0)
    {
        gstSrcSize.u16Width     = u16Width;
    }
    gstSrcSize.bSet = 1;
}
void Drv_HVSP_SetCropWindowSize(void)
{
    ST_HVSP_CROP_INFO stCropInfo_1;
    ST_HVSP_CMD_TRIG_CONFIG stCmdTrigCfg;
    stCropInfo_1 = _Drv_HVSP_FillPreCropInfo();
    DRV_HVSP_MUTEX_LOCK();
    stCmdTrigCfg = Drv_HVSP_SetCMDQTrigTypeByRIU();
    Hal_HVSP_SetCropConfig(E_HVSP_CROP_ID_1, stCropInfo_1);
    Drv_HVSP_SetCMDQTrigType(stCmdTrigCfg);
    DRV_HVSP_MUTEX_UNLOCK();
    SCL_DBG(SCL_DBG_LV_DRVHVSP()&(Get_DBGMG_HVSP(0)), "[DRVHVSP]%s Input:(%hd,%hd) Crop:(%hd,%hd)\n", __FUNCTION__,
        stCropInfo_1.u16In_hsize,stCropInfo_1.u16In_vsize,stCropInfo_1.u16Hsize,stCropInfo_1.u16Vsize);
}

MS_BOOL Drv_HVSP_SetInputMux(EN_HVSP_IP_MUX_TYPE enIP,ST_HVSP_CLK_CONFIG* stclk)
{
    DRV_HVSP_DBG(printf("[DRVHVSP]%s(%d): IP=%x\n", __FUNCTION__,  __LINE__,enIP));
    SCL_DBG(SCL_DBG_LV_DRVHVSP()&(Get_DBGMG_HVSP(0)), "[DRVHVSP]%s(%d): IP=%x\n", __FUNCTION__,  __LINE__,enIP);
    Hal_HVSP_SetInputMuxType(enIP);
    if(!gbclkforcemode && stclk != NULL)
    {
        Hal_HVSP_SetIdclkOnOff(1,stclk);
    }
    if(enIP >= E_HVSP_IP_MUX_MAX)
    {
        DRV_HVSP_ERR(printf("[DRVHVSP]%s(%d):: Wrong IP Type\n", __FUNCTION__, __LINE__));
        return FALSE;
    }

    Hal_HVSP_SetHWInputMux(enIP);
    return TRUE;
}

MS_BOOL Drv_HVSP_SetRegisterForce(MS_U32 u32Reg, MS_U8 u8Val, MS_U8 u8Msk)
{
    Hal_HVSP_Set_Reg(u32Reg, u8Val, u8Msk);
    return TRUE;
}
void Drv_HVSP_SetOSDConfig(EN_HVSP_ID_TYPE enID, ST_DRV_HVSP_OSD_CONFIG stOSdCfg)
{
    Hal_HVSP_SetOSDLocate(enID,stOSdCfg.enOSD_loc);
    Hal_HVSP_SetOSDOnOff(enID,stOSdCfg.stOsdOnOff.bOSDEn);
    Hal_HVSP_SetOSDbypass(enID,stOSdCfg.stOsdOnOff.bOSDBypass);
    Hal_HVSP_SetOSDbypassWTM(enID,stOSdCfg.stOsdOnOff.bWTMBypass);
}
MS_BOOL Drv_HVSP_GetSCLInform(EN_HVSP_ID_TYPE enID,ST_DRV_HVSP_SCINFORM_CONFIG *stInformCfg)
{
    stInformCfg->u16X               = Hal_HVSP_GetCrop2Xinfo();
    stInformCfg->u16Y               = Hal_HVSP_GetCrop2Yinfo();
    stInformCfg->u16Width           = Hal_HVSP_GetHVSPOutputWidth(enID);
    stInformCfg->u16Height          = Hal_HVSP_GetHVSPOutputHeight(enID);
    stInformCfg->u16crop2inWidth    = Hal_HVSP_GetCrop2InputWidth();
    stInformCfg->u16crop2inHeight   = Hal_HVSP_GetCrop2InputHeight();
    stInformCfg->u16crop2OutWidth   = Hal_HVSP_GetCrop2OutputWidth();
    stInformCfg->u16crop2OutHeight  = Hal_HVSP_GetCrop2OutputHeight();
    stInformCfg->bEn                = Hal_HVSP_GetCrop2En();
    return TRUE;
}
MS_BOOL Drv_HVSP_GetHVSPAttribute(EN_HVSP_ID_TYPE enID,ST_DRV_HVSP_HVSPINFORM_CONFIG *stInformCfg)
{
    stInformCfg->u16Width           = Hal_HVSP_GetHVSPOutputWidth(enID);
    stInformCfg->u16Height          = Hal_HVSP_GetHVSPOutputHeight(enID);
    stInformCfg->u16inWidth         = Hal_HVSP_GetHVSPInputWidth(enID);
    stInformCfg->u16inHeight        = Hal_HVSP_GetHVSPInputHeight(enID);
    stInformCfg->bEn                = Hal_HVSP_GetScalingFunctionStatus(enID);
    return TRUE;
}
void Drv_HVSP_GetOSDAttribute(EN_HVSP_ID_TYPE enID,ST_DRV_HVSP_OSD_CONFIG *stOsdCfg)
{
    stOsdCfg->enOSD_loc = Hal_HVSP_GetOSDLocate(enID);
    stOsdCfg->stOsdOnOff.bOSDEn = Hal_HVSP_GetOSDOnOff(enID);
    stOsdCfg->stOsdOnOff.bOSDBypass = Hal_HVSP_GetOSDbypass(enID);
    stOsdCfg->stOsdOnOff.bWTMBypass = Hal_HVSP_GetOSDbypassWTM(enID);
}
MS_BOOL Drv_HVSP_GetFrameBufferAttribute(EN_HVSP_ID_TYPE enID,ST_HVSP_IPM_CONFIG *stInformCfg)
{
    stInformCfg->bWrite = gstIPMCfg.bWrite;
    stInformCfg->u16Fetch = gstIPMCfg.u16Fetch;
    stInformCfg->u16Vsize = gstIPMCfg.u16Vsize;
    stInformCfg->u32BaseAddr = gstIPMCfg.u32BaseAddr+0x20000000;
    stInformCfg->u32MemSize = gstIPMCfg.u32MemSize;
    stInformCfg->bRead = gstIPMCfg.bRead;
    return Hal_HVSP_GetLDCPathSel();
}
MS_BOOL Drv_HVSP_SetLDCFrameBuffer_Config(ST_HVSP_LDC_FRAMEBUFFER_CONFIG stLDCCfg)
{
    ST_HVSP_CMD_TRIG_CONFIG stCmdTrigCfg;
    SCL_DBG(SCL_DBG_LV_DRVHVSP()&(Get_DBGMG_HVSP(0)), "[DRVHVSP]%s(%d): Width=%hx,Height:%x\n", __FUNCTION__,
        __LINE__, stLDCCfg.u16Width,stLDCCfg.u16Height);
    DRV_HVSP_MUTEX_LOCK();
    stCmdTrigCfg = Drv_HVSP_SetCMDQTrigTypeByRIU();
    Hal_HVSP_SetLDCWidth(stLDCCfg.u16Width);
    Hal_HVSP_SetLDCHeight(stLDCCfg.u16Height);
    Hal_HVSP_SetLDCBase(stLDCCfg.u32FBaddr);
    Hal_HVSP_SetLDCHWrwDiff((((MS_U16)stLDCCfg.u8FBrwdiff<<4)|((MS_U16)stLDCCfg.u8FBidx<<2)|((MS_U16)stLDCCfg.bEnSWMode<<8)));
    Hal_HVSP_SetLDCPathSel(stLDCCfg.bEnDNR);
    if(!gbLDCEn)
    {
        Hal_HVSP_SetLDCBypass(1);
    }
    Drv_HVSP_SetCMDQTrigType(stCmdTrigCfg);
    DRV_HVSP_MUTEX_UNLOCK();
    return TRUE;
}
void Drv_HVSP_SetLDCONOFF(MS_BOOL bEn)
{
    gbLDCEn = bEn;
}
MS_BOOL Drv_HVSP_SetPatTgen(MS_BOOL bEn, ST_DRV_HVSP_PAT_TGEN_CONFIG *pCfg)
{
    MS_U16 u16VSync_St, u16HSync_St;
    MS_BOOL bRet = TRUE;
    Drv_SCLIRQ_SetPTGenStatus(bEn);
    if(bEn)
    {
        u16VSync_St = 1;
        u16HSync_St = 0;
        if(pCfg)
        {
            if((u16VSync_St + pCfg->u16VSyncWidth + pCfg->u16VBackPorch + pCfg->u16VActive - 1 )<1125)
            {
                Hal_HVSP_SetPatTgVtt(1125); //scaling up need bigger Vtt, , using vtt of 1920x1080 for all timing
            }
            else
            {
                Hal_HVSP_SetPatTgVtt(2200); //rotate
            }
            Hal_HVSP_SetPatTgVsyncSt(u16VSync_St);
            Hal_HVSP_SetPatTgVsyncEnd(u16VSync_St + pCfg->u16VSyncWidth - 1);
            Hal_HVSP_SetPatTgVdeSt(u16VSync_St + pCfg->u16VSyncWidth + pCfg->u16VBackPorch);
            Hal_HVSP_SetPatTgVdeEnd(u16VSync_St + pCfg->u16VSyncWidth + pCfg->u16VBackPorch + pCfg->u16VActive - 1);
            Hal_HVSP_SetPatTgVfdeSt(u16VSync_St + pCfg->u16VSyncWidth + pCfg->u16VBackPorch);
            Hal_HVSP_SetPatTgVfdeEnd(u16VSync_St + pCfg->u16VSyncWidth + pCfg->u16VBackPorch + pCfg->u16VActive - 1);

            Hal_HVSP_SetPatTgHtt(2200); // scaling up need bigger Vtt, , using vtt of 1920x1080 for all timing
            Hal_HVSP_SetPatTgHsyncSt(u16HSync_St);
            Hal_HVSP_SetPatTgHsyncEnd(u16HSync_St + pCfg->u16HSyncWidth - 1);
            Hal_HVSP_SetPatTgHdeSt(u16HSync_St + pCfg->u16HSyncWidth + pCfg->u16HBackPorch);
            Hal_HVSP_SetPatTgHdeEnd(u16HSync_St + pCfg->u16HSyncWidth + pCfg->u16HBackPorch + pCfg->u16HActive - 1);
            Hal_HVSP_SetPatTgHfdeSt(u16HSync_St + pCfg->u16HSyncWidth + pCfg->u16HBackPorch);
            Hal_HVSP_SetPatTgHfdeEnd(u16HSync_St + pCfg->u16HSyncWidth + pCfg->u16HBackPorch + pCfg->u16HActive - 1);

            Hal_HVSP_SetPatTgEn(TRUE);
            bRet = TRUE;
        }
        else
        {
            bRet = FALSE;
        }
    }
    else
    {
        Hal_HVSP_SetPatTgEn(FALSE);
        bRet = TRUE;
    }
    return bRet;
}
void Drv_HVSP_IDCLKRelease(ST_HVSP_CLK_CONFIG* stclk)
{
    if(!gbclkforcemode)
    Hal_HVSP_SetIdclkOnOff(0,stclk);
}
unsigned long Drv_HVSP_CMDQStatusReport(void)
{
    unsigned long u32Reg;
    static MS_U32 u32savereg;
    u32Reg = Hal_HVSP_GetCMDQStatus();
    if(u32savereg == u32Reg)
    {
        return 0;
    }
    else
    {
        u32savereg = u32Reg;
        return u32Reg ;
    }
}
unsigned long Drv_HVSP_CropCheck(void)
{
    MS_U16 u16croph,u16cropv,u16cropch,u16cropcv;
    static MS_BOOL sbPrint = 0;
    if(!sbPrint)
    {
        Drv_SCLIRQ_Set_Checkcropflag(EN_HVSP_MONITOR_CROPCHECK);
        printf("[DRVHVSP]check crop\n");
    }
    u16croph    = Hal_HVSP_GetCrop1Width();
    u16cropv    = Hal_HVSP_GetCrop1Height();
    u16cropch   = Hal_HVSP_GetCrop1WidthCount();
    u16cropcv   = Hal_HVSP_GetCrop1HeightCount();
    if(HVSP_CROP_CHECK(u16croph,u16cropch,u16cropv,u16cropcv))
    {
        (printf("[DRVHVSP]crop set    :(H,V)=(%hd,%hd)\n         crop receive:(H,V)=(%hd,%hd)\n",
            u16croph,u16cropv,u16cropch+1,u16cropcv ));
        return 1;
    }
    sbPrint = 1 ;
    return 0;
}
unsigned long Drv_HVSP_DMACheck(EN_HVSP_MONITOR_TYPE enMonitorType)
{
    MS_U16 u16dmav,u16dmacv,bEn,u16hvsp,u16hvsp2,u16hvsp3;
    MS_U32 u32Time;
    static MS_BOOL sbPrint = 0;
    if(!sbPrint)
    {
        Drv_SCLIRQ_Set_Checkcropflag(enMonitorType);
        printf("[DRVHVSP]check DMA:%d\n",enMonitorType);
    }
    u16dmav    = Hal_HVSP_GetDMAHeight(enMonitorType);
    u16dmacv   = Hal_HVSP_GetDMAHeightCount(enMonitorType);
    bEn        = Hal_HVSP_GetDMAEn(enMonitorType);
    if(HVSP_DMA_CHECK(u16dmav,u16dmacv)&& bEn)
    {
        u32Time = ((MS_U32)MsOS_GetSystemTime());
        u16hvsp = Hal_HVSP_GetHVSPOutputHeightCount(E_HVSP_ID_1);
        u16hvsp2= Hal_HVSP_GetHVSPOutputHeightCount(E_HVSP_ID_2);
        u16hvsp3= Hal_HVSP_GetHVSPOutputHeightCount(E_HVSP_ID_3);
        (printf("[DRVHVSP]dma set    :(V)=(%hd) @%lu\n         dma receive:(V)=(%hd) hvsp(3,2,1):(%hd,%hd,%hd)\n",
            u16dmav,u32Time,u16dmacv,u16hvsp3,u16hvsp2,u16hvsp ));
        return 1;
    }
    sbPrint = 1 ;
    return 0;
}
void Drv_HVSP_GetCrop12Inform(ST_DRV_HVSP_INPUTINFORM_CONFIG *stInformCfg)
{
    stInformCfg->u16inWidth         = Hal_HVSP_GetCrop1Width();
    stInformCfg->u16inHeight        = Hal_HVSP_GetCrop1Height();
    stInformCfg->u16inWidthcount    = Hal_HVSP_GetCrop1WidthCount();
    if(stInformCfg->u16inWidthcount)
    {
        stInformCfg->u16inWidthcount++;
    }
    stInformCfg->u16inHeightcount   = Hal_HVSP_GetCrop1HeightCount();
    stInformCfg->enMux              = Hal_HVSP_GetInputSrcMux();

}
void Drv_HVSP_SetCLKForcemode(unsigned char bEn)
{
    gbclkforcemode = bEn;
}
MS_BOOL Drv_HVSP_GetCLKForcemode(void)
{
    return gbclkforcemode;
}
void Drv_HVSP_SetCLKRate(unsigned char u8Idx)
{
    if(gbclkforcemode)
    {
        u8Idx |= EN_HVSP_CLKATTR_FORCEMODE;
    }
    Hal_HVSP_SetCLKRate(u8Idx);
}
MS_U8 Drv_HVSP_CheckInputVSync(void)
{
        MS_U32 u32Events = 0;
        MsOS_WaitEvent(Drv_SCLIRQ_Get_IRQ_SYNCEventID(), E_SCLIRQ_EVENT_SYNC, &u32Events, E_OR, MSOS_WAIT_FOREVER); // get status: FRM END
        if(u32Events)
        {
            MsOS_ClearEventIRQ(Drv_SCLIRQ_Get_IRQ_SYNCEventID(),(E_SCLIRQ_EVENT_SYNC));
            return 1;
        }
        return 0;
}
#undef DRV_HVSP_C
