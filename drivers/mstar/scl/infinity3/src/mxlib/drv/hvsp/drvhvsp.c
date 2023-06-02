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
//#include <linux/wait.h>
//#include <linux/irqreturn.h>
//#include <asm/div64.h>
#endif
#include <asm/div64.h>

//-------------------------------------------------------------------------------------------------
//  Include Files
//-------------------------------------------------------------------------------------------------
#include "MsCommon.h"
#include "MsTypes.h"
#include "MsOS.h"
#include "MsDbg.h"
//#include <linux/kthread.h>
//#include <linux/irq.h>
//#include <linux/clk.h>
//#include <linux/clk-provider.h>

#include "drvhvsp_st.h"
#include "halhvsp.h"
#include "drvhvsp.h"
#include "drvscldma_st.h"
#include "drvsclirq_st.h"
#include "drvsclirq.h"
#include "irqs.h"
#include "mdrv_scl_dbg.h"
#include "hwreg.h"
#include "drvCMDQ.h"

//-------------------------------------------------------------------------------------------------
//  Defines & Macro
//-------------------------------------------------------------------------------------------------
#define DRV_HVSP_DBG(x)
#define DRV_HVSP_ERR(x)      x
#define DRV_HVSP_MUTEX_LOCK()            MsOS_ObtainMutex(_HVSP_Mutex,MSOS_WAIT_FOREVER)
#define DRV_HVSP_MUTEX_UNLOCK()          MsOS_ReleaseMutex(_HVSP_Mutex)
#define FHD_Width   1920
#define FHD_Height  1080
#define _3M_Width   2048
#define _3M_Height  1536
#define HD_Width    1280
#define HD_Height   720
#define D_Width    704
#define D_Height   576
#define PNL_Width   800
#define PNL_Height  480
#define SRAMFORSCALDOWN 0x10
#define SRAMFORSCALUP 0x21
#define SRAMFORSC2ALDOWN 0x20
#define SRAMFORSC2ALUP 0x21
#define SRAMFORSC3HDOWN 0x00
#define SRAMFORSC3HUP 0x21
#define SRAMFORSC3VDOWN 0x00
#define SRAMFORSC3VUP 0x21
#define HeightRange (gstSrcSize.u16Height ? ((gstSrcSize.u16Height+1)/20) : (FHD_Height/20))
#define Is_StartAndEndInSameByte(u16XStartoffsetbit,u16idx,u16Xoffset) \
    (u16XStartoffsetbit && (u16idx ==0) && (u16Xoffset==1))
#define Is_StartByteOffsetBit(u16XStartoffsetbit,u16idx) \
        (u16XStartoffsetbit && (u16idx ==0))
#define Is_EndByteOffsetBit(u16XEndoffsetbit,u16Xoffset,u16idx) \
    (u16XEndoffsetbit && (u16idx ==(u16Xoffset-1)))
#define Is_DNRBufferReady() (gbMemReadyForDrv)
#define Is_FrameBufferTooSmall(u32ReqMemSize,u32IPMMemSize) (u32ReqMemSize > u32IPMMemSize)
#define Is_PreCropWidthNeedToOpen() (gstSrcSize.u16Width > gstIPMCfg.u16Fetch)
#define Is_PreCropHeightNeedToOpen() (gstSrcSize.u16Height> gstIPMCfg.u16Vsize)
#define Is_InputSrcRotate() (gstSrcSize.u16Height > gstSrcSize.u16Width)
#define Is_IPM_NotSetReady() (gstIPMCfg.u16Fetch == 0 || gstIPMCfg.u16Vsize == 0)
#define Is_INPUTMUX_SetReady() (gstSrcSize.bSet == 1)
#define Is_PreCropNotNeedToOpen() ((gstIPMCfg.u16Fetch == gstSrcSize.u16Width) && \
    (gstIPMCfg.u16Vsize == gstSrcSize.u16Height))
#define HVSP_ID_SRAM_V_OFFSET 2
#define HVSP_ID_SRAM_H_OFFSET 15
#define HVSP_RATIO(input, output)           ((MS_U32)((MS_U64)((input) * 1048576) / (output)))
#define HVSP_CROP_RATIO(u16src, u16crop1, u16crop2)  ((MS_U16)(((MS_U32)u16crop2 * (MS_U32)u16crop1) / (MS_U32)u16src ))
#define HVSP_CROP_CHECK(u16croph,u16cropch,u16cropv,u16cropcv)  (((u16croph) != (u16cropch))|| ((u16cropv) != (u16cropcv)))
#define HVSP_DMA_CHECK(u16cropv,u16cropcv)  (((u16cropv) != (u16cropcv)))
#define _Change_SRAM_V_Qmap(enHVSP_ID,up)         (((gstScalinInfo[enHVSP_ID].stSizeAfterCrop.u16Height\
    > gstScalinInfo[enHVSP_ID].stSizeAfterScaling.u16Height)&&(up)) || (((gstScalinInfo[enHVSP_ID].stSizeAfterCrop.u16Height\
    < gstScalinInfo[enHVSP_ID].stSizeAfterScaling.u16Height) && !(up))))
#define _Change_SRAM_H_Qmap(enHVSP_ID,up)         (((gstScalinInfo[enHVSP_ID].stSizeAfterCrop.u16Width\
        > gstScalinInfo[enHVSP_ID].stSizeAfterScaling.u16Width)&&(up)) || (((gstScalinInfo[enHVSP_ID].stSizeAfterCrop.u16Width\
        < gstScalinInfo[enHVSP_ID].stSizeAfterScaling.u16Width) && !(up))))

#define _Is_SingleBufferAndZoomSizeBiggerthanLimitation(u8buffernum,cropV,DspV) (((u8buffernum)==1) && \
    ((DspV) > (cropV)))
#define _Is_SingleBufferAndNOLDCZoomSizeBiggerthanLimitation(bnoLDC,u8buffernum,cropV,DspV) ((bnoLDC)&&((u8buffernum)==1) && \
    ((DspV) > (cropV)))
#define _Is_Src5MSize() ((gstSrcSize.u16Height*gstSrcSize.u16Width) >= 4500000 &&(gstSrcSize.bSet))
#define _IsChangeFBBufferResolution(u16Width ,u16Height) ((u16Width !=gstIPMCfg.u16Fetch)||(u16Height !=gstIPMCfg.u16Vsize))
#define _IsChangePreCropPosition(u16X ,u16Y ,u16oriX ,u16oriY ) (((u16X !=u16oriX)||(u16Y !=u16oriY)))
#define _IsZoomOut(u16Width ,u16Height) ((u16Width < gstIPMCfg.u16Fetch)||(u16Height < gstIPMCfg.u16Vsize))
//-------------------------------------------------------------------------------------------------
//  Variable
//-------------------------------------------------------------------------------------------------
ST_HVSP_SCALING_INFO gstScalinInfo[E_HVSP_ID_MAX];
ST_HVSP_IPM_CONFIG   gstIPMCfg = {0};
ST_HVSP_SIZE_CONFIG gstSrcSize;
unsigned char gbPriMaskPending;
unsigned char *gpu8PriMaskBuf;
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
static MS_BOOL gbVScalingup[E_HVSP_ID_MAX] ={1,0,0};
static MS_BOOL gbHScalingup[E_HVSP_ID_MAX] ={1,0,0};
MS_BOOL gbSc3FirstHSet = 1;
MS_BOOL gbSc3FirstVSet = 1;

//-------------------------------------------------------------------------------------------------
//  Private Functions
//-------------------------------------------------------------------------------------------------
void _Drv_HVSP_InitSWVarialbe(EN_HVSP_ID_TYPE HVSP_IP)
{
    MsOS_Memset(&gstScalinInfo[HVSP_IP], 0, sizeof(ST_HVSP_SCALING_INFO));
    gbVScalingup[HVSP_IP] = 0;
    gbHScalingup[HVSP_IP] = 0;
    gbSc3FirstHSet = 1;
    gbSc3FirstVSet = 1;
    if(HVSP_IP == E_HVSP_ID_1)
    {
        MsOS_Memset(&gstIPMCfg, 0, sizeof(ST_HVSP_IPM_CONFIG));
        gbLDCEn = 0;
        gbPriMaskPending = 0;
        gbVScalingup[HVSP_IP] = SRAMFORSCALUP;
        gbHScalingup[HVSP_IP] = SRAMFORSCALUP;
        gstSrcSize.u16Height    = FHD_Height;
        gstSrcSize.u16Width     = FHD_Width;
        gstSrcSize.bSet         = 0;
        Hal_HVSP_SetInputSrcSize(&gstSrcSize);
    }
    else if(HVSP_IP == E_HVSP_ID_2)
    {
        gbVScalingup[HVSP_IP] = SRAMFORSC2ALDOWN;
        gbHScalingup[HVSP_IP] = SRAMFORSC2ALDOWN;
    }
    else if(HVSP_IP == E_HVSP_ID_3)
    {

        gbVScalingup[HVSP_IP] = SRAMFORSC3VDOWN;
        gbHScalingup[HVSP_IP] = SRAMFORSC3HDOWN;
    }

}

void _Drv_HVSP_FillPreCropInfo(ST_HVSP_CROP_INFO *stCropInfo)
{
    if(Is_IPM_NotSetReady() || Is_PreCropNotNeedToOpen())
    {
        if(Is_IPM_NotSetReady())
        {
            DRV_HVSP_ERR(printf("[DRVHVSP]%s %d:: IPM without setting\n", __FUNCTION__, __LINE__));
        }
        stCropInfo->bEn = 0;
        stCropInfo->u16Vst = 0;
        stCropInfo->u16Hst = 0;
        stCropInfo->u16Hsize = 0;
        stCropInfo->u16Vsize = 0;
    }
    else
    {
        stCropInfo->bEn = 1;
        if(Is_InputSrcRotate())//rotate
        {
            stCropInfo->u16Hst = 0;
            stCropInfo->u16Vst = 0;
        }
        else if(Is_PreCropWidthNeedToOpen() || Is_PreCropHeightNeedToOpen())
        {
            if(Is_PreCropWidthNeedToOpen())
            {
                stCropInfo->u16Hst      = (gstSrcSize.u16Width - gstIPMCfg.u16Fetch)/2;
            }
            if(Is_PreCropHeightNeedToOpen())
            {
                stCropInfo->u16Vst      = (gstSrcSize.u16Height - gstIPMCfg.u16Vsize)/2;
            }
        }
        else
        {
            stCropInfo->u16Hst = 0;
            stCropInfo->u16Vst = 0;
        }
        stCropInfo->u16Hsize    = gstIPMCfg.u16Fetch;
        stCropInfo->u16Vsize    = gstIPMCfg.u16Vsize;
    }
    // crop1
    stCropInfo->u16In_hsize = gstSrcSize.u16Width;
    stCropInfo->u16In_vsize = gstSrcSize.u16Height;
    if(stCropInfo->u16In_hsize == 0)
    {
        stCropInfo->u16In_hsize = FHD_Width;
    }
    if(stCropInfo->u16In_vsize == 0)
    {
        stCropInfo->u16In_vsize = FHD_Height;
    }
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
MS_BOOL Drv_HVSP_SetCMDQTrigType(ST_HVSP_CMD_TRIG_CONFIG *stCmdTrigCfg)
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

MS_BOOL Drv_HVSP_GetCMDQDoneStatus(EN_HVSP_POLL_ID_TYPE enPollId)
{
    MS_U32 u32Events = 0;
    static MS_BOOL sbDone = 0;
    MS_BOOL bRet = 0;

    if(((sbDone&(0x1<<E_HVSP_POLL_ID_1)) &&enPollId ==E_HVSP_POLL_ID_2)||
        ((sbDone&(0x1<<E_HVSP_POLL_ID_2)) &&enPollId ==E_HVSP_POLL_ID_1))
    {
        bRet = 1;
    }
    else
    {
        MsOS_WaitEvent(Drv_SCLIRQ_Get_IRQ_SYNCEventID(),E_SCLIRQ_EVENT_CHANGESIZEDONE, &u32Events, E_OR, 100);
    }
    if(sbDone&&(MsOS_GetEvent(Drv_SCLIRQ_Get_IRQ_SYNCEventID())&
        (E_SCLIRQ_EVENT_CHANGESIZEDONE|E_SCLIRQ_EVENT_CMDQFIRE|E_SCLIRQ_EVENT_CMDQ)))
    {
        DRV_HVSP_MUTEX_LOCK();
        sbDone = 0;
        DRV_HVSP_MUTEX_UNLOCK();
    }
    if(u32Events&E_SCLIRQ_EVENT_CHANGESIZEDONE)
    {
        if((enPollId ==E_HVSP_POLL_ID_1||enPollId ==E_HVSP_POLL_ID_2))
        {
            DRV_HVSP_MUTEX_LOCK();
            sbDone = (0x1<<enPollId);
            DRV_HVSP_MUTEX_UNLOCK();
        }
        bRet = 1;
    }
    SCL_DBG(SCL_DBG_LV_IOCTL()&(0x1<<enPollId), "[HVSP]u32Events=%lx,bRet=%hhd,@:%lx\n",u32Events,bRet,MsOS_GetEvent(Drv_SCLIRQ_Get_IRQ_SYNCEventID()));
    return bRet;
}

void * Drv_HVSP_GetWaitQueueHead(void)
{
    return Drv_SCLIRQ_GetSyncQueue();
}
void Drv_HVSP_Release(EN_HVSP_ID_TYPE HVSP_IP)
{
    if(HVSP_IP == E_HVSP_ID_1)
    {
        Drv_HVSP_SetPriMaskTrigger(EN_HVSP_PRIMASK_DISABLE);
        Hal_HVSP_Set_Reset();
        if(!MsOS_ReleaseMutexAll())
        {
            SCL_DBG(SCL_DBG_LV_MDRV_IO(), "[DRVHVSP]!!!!!!!!!!!!!!!!!!! HVSP Release Mutex fail\n");
        }
    }
    _Drv_HVSP_InitSWVarialbe(HVSP_IP);
    if(HVSP_IP == E_HVSP_ID_1)
    {
        Drv_CMDQ_release(EN_CMDQ_TYPE_IP0);
    }
    SCL_DBG(SCL_DBG_LV_DRVHVSP()&(Get_DBGMG_HVSP(HVSP_IP)), "[DRVHVSP]%s(%d) HVSP %d Release\n", __FUNCTION__, __LINE__,HVSP_IP);
}
void Drv_HVSP_Open(EN_HVSP_ID_TYPE HVSP_IP)
{
    if(HVSP_IP==E_HVSP_ID_1)
    {
        Drv_CMDQ_Enable(1,EN_CMDQ_TYPE_IP0);
    }
}
MS_BOOL Drv_HVSP_Suspend(ST_HVSP_SUSPEND_RESUME_CONFIG *pCfg)
{
    ST_SCLIRQ_SUSPEND_RESUME_CONFIG stSclirq;
    MS_BOOL bRet = TRUE;
    MsOS_Memset(&stSclirq,0,sizeof(ST_SCLIRQ_SUSPEND_RESUME_CONFIG));
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
void _Drv_HVSP_PriMaskInit(void)
{
    Hal_HVSP_PriMask_ColorY(0x0);
    Hal_HVSP_PriMask_ColorU(0x200);
    Hal_HVSP_PriMask_ColorV(0x200);
}
void _Drv_HVSP_SetHWinit(void)
{
    _Drv_HVSP_PriMaskInit();
    Hal_HVSP_SetTestPatCfg();
    Hal_HVSP_SetVpsSRAMEn(1);
    Hal_HVSP_SetIPMBufferNumber(gu8FBBufferNum);
    Hal_HVSP_SetOSDbypass(E_HVSP_ID_1, 1);
    Hal_HVSP_SetOSDbypassWTM(E_HVSP_ID_1, 1);
    Hal_HVSP_SRAM_Dump(EN_HVSP_SRAM_DUMP_HVSP_V,SRAMFORSCALUP);
    Hal_HVSP_SRAM_Dump(EN_HVSP_SRAM_DUMP_HVSP_V_1,SRAMFORSC2ALDOWN);
    Hal_HVSP_SRAM_Dump(EN_HVSP_SRAM_DUMP_HVSP_V_2,SRAMFORSC3VDOWN);
    Hal_HVSP_SRAM_Dump(EN_HVSP_SRAM_DUMP_HVSP_H,SRAMFORSCALUP);
    Hal_HVSP_SRAM_Dump(EN_HVSP_SRAM_DUMP_HVSP_H_1,SRAMFORSC2ALDOWN);
    Hal_HVSP_SRAM_Dump(EN_HVSP_SRAM_DUMP_HVSP_H_2,SRAMFORSC3HDOWN);
    gbVScalingup[E_HVSP_ID_3] = SRAMFORSC3VDOWN;
    gbHScalingup[E_HVSP_ID_3] = SRAMFORSC3HDOWN;
}

MS_BOOL Drv_HVSP_Resume(ST_HVSP_SUSPEND_RESUME_CONFIG *pCfg)
{
    ST_SCLIRQ_SUSPEND_RESUME_CONFIG stSclirq;
    MS_BOOL bRet = TRUE;
    MsOS_Memset(&stSclirq,0,sizeof(ST_SCLIRQ_SUSPEND_RESUME_CONFIG));
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
        _Drv_HVSP_SetHWinit();
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
void Drv_HVSP_PriMaskBufferReset(void)
{
    DRV_HVSP_MUTEX_LOCK();
    MsOS_Memset(gpu8PriMaskBuf,0,HVSP_PRIMASK_SIZE);
    DRV_HVSP_MUTEX_UNLOCK();
}
void Drv_HVSP_Exit(unsigned char bCloseISR)
{
    if(_HVSP_Mutex != -1)
    {
        MsOS_DeleteMutex(_HVSP_Mutex);
        _HVSP_Mutex = -1;
    }
    if(bCloseISR)
    {
        Drv_SCLIRQ_Exit();
    }
    if(gpu8PriMaskBuf)
    {
        MsOS_VirMemFree(gpu8PriMaskBuf);  //probe ,free
        gpu8PriMaskBuf = NULL;
    }
    Hal_HVSP_Exit();
    gbMemReadyForDrv = 0;
    gu8FBBufferNum = 0;
    gbclkforcemode = 0;
}

MS_BOOL Drv_HVSP_Init(ST_HVSP_INIT_CONFIG *pInitCfg)
{
    char word[] = {"_HVSP_Mutex"};
    ST_SCLIRQ_INIT_CONFIG stIRQInitCfg;
    MS_U8 u8IDidx;
    ST_HVSP_CMD_TRIG_CONFIG stori;
    MsOS_Memset(&stIRQInitCfg,0,sizeof(ST_SCLIRQ_INIT_CONFIG));
    MsOS_Memset(&stori,0,sizeof(ST_HVSP_CMD_TRIG_CONFIG));
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
        _Drv_HVSP_InitSWVarialbe((EN_HVSP_ID_TYPE)u8IDidx);
        _Drv_HVSP_SetCoringThrdOn((EN_HVSP_ID_TYPE)u8IDidx);
    }
    Drv_HVSP_SetCMDQTrigTypeByRIU(&stori);
    DRV_HVSP_MUTEX_UNLOCK();
    gpu8PriMaskBuf = MsOS_VirMemalloc(HVSP_PRIMASK_SIZE);  //probe ,not free yet
    if(!gpu8PriMaskBuf)
    {
        DRV_HVSP_ERR(printf("[DRVHVSP]%s(%d) Init gpu8PriMaskBuf Fail\n", __FUNCTION__, __LINE__));
        return FALSE;
    }
    Drv_HVSP_PriMaskBufferReset();
    Drv_SCLIRQ_InterruptEnable(SCLIRQ_VSYNC_FCLK_LDC);
    Hal_HVSP_SetFrameBufferManageLock(0);
    Drv_SCLIRQ_SetDNRLock(0);
    _Drv_HVSP_SetHWinit();
    return TRUE;
}
void Drv_HVSP_SetCMDQTrigTypeByRIU(ST_HVSP_CMD_TRIG_CONFIG *stori)
{
    ST_HVSP_CMD_TRIG_CONFIG stCmdTrigCfg;
    MsOS_Memset(&stCmdTrigCfg,0,sizeof(ST_HVSP_CMD_TRIG_CONFIG));
    Drv_HVSP_GetCMDQTrigType(stori);
    stCmdTrigCfg.enType     = E_HVSP_CMD_TRIG_NONE;
    stCmdTrigCfg.u8Fmcnt    = 0;
    Drv_HVSP_SetCMDQTrigType(&stCmdTrigCfg);
}
void _Drv_HVSP_SetPreCropWhenInputMuxReady(void)
{
    ST_HVSP_CROP_INFO stCropInfo_1;
    MsOS_Memset(&stCropInfo_1,0,sizeof(ST_HVSP_CROP_INFO));
    if(Is_INPUTMUX_SetReady())
    {
        _Drv_HVSP_FillPreCropInfo(&stCropInfo_1);
        Hal_HVSP_SetCropConfig(E_HVSP_CROP_ID_1, &stCropInfo_1);
    }
}
MS_BOOL Drv_HVSP_Set_IPM_Config(ST_HVSP_IPM_CONFIG *stCfg)
{
    ST_HVSP_CMD_TRIG_CONFIG stCmdTrigCfg;
    MsOS_Memset(&stCmdTrigCfg,0,sizeof(ST_HVSP_CMD_TRIG_CONFIG));
    SCL_DBG(SCL_DBG_LV_DRVHVSP()&(Get_DBGMG_HVSP(0)), "[DRVHVSP]%s(%d): Base=%lx, Fetch=%d, Vsize=%d\n"
        , __FUNCTION__, __LINE__, stCfg->u32YCBaseAddr, stCfg->u16Fetch, stCfg->u16Vsize);
    MsOS_Memcpy(&gstIPMCfg, stCfg, sizeof(ST_HVSP_IPM_CONFIG));
    DRV_HVSP_MUTEX_LOCK();
    if(gstIPMCfg.u16Fetch%2)
    {
        gstIPMCfg.u16Fetch--;
        stCfg->u16Fetch--;
        SCL_ERR("[DRVHVSP]IPM Width not align 2\n");
    }
    Drv_HVSP_SetCMDQTrigTypeByRIU(&stCmdTrigCfg);
    //_Drv_HVSP_SetPreCropWhenInputMuxReady();
    Hal_HVSP_SetBT656SrcConfig(1,stCfg->u16Fetch);
    Hal_HVSP_SetIPMYCBase(stCfg->u32YCBaseAddr);
    Hal_HVSP_SetIPMMotionBase(stCfg->u32MBaseAddr);
    Hal_HVSP_SetIPMCIIRBase(stCfg->u32CIIRBaseAddr);
    Hal_HVSP_SetIPMFetchNum(stCfg->u16Fetch);
    Hal_HVSP_SetIPMLineOffset(stCfg->u16Fetch);
    Hal_HVSP_SetIPMvSize(stCfg->u16Vsize);
    Hal_HVSP_SetIPMYCMReadEn(stCfg->bYCMRead);
    Hal_HVSP_SetIPMYCMWriteEn(stCfg->bYCMWrite);
    Hal_HVSP_SetIPMCIIRReadEn(stCfg->bCIIRRead);
    Hal_HVSP_SetIPMCIIRWriteEn(stCfg->bCIIRWrite);
    Drv_HVSP_SetCMDQTrigType(&stCmdTrigCfg);
    DRV_HVSP_MUTEX_UNLOCK();
    return TRUE;
}
void Drv_HVSP_SetFbManageConfig(ST_DRV_HVSP_SET_FB_MANAGE_CONFIG *stCfg)
{
    (printf("[DRVHVSP]%s(%d):%x \n",
        __FUNCTION__, __LINE__, stCfg->enSet));
    Hal_HVSP_SetFrameBufferManageLock(0);
    if(stCfg->enSet & EN_DRV_HVSP_FBMG_SET_LDCPATH_ON)
    {
        Hal_HVSP_SetLDCPathSel(1);
        printf("LDC ON\n");
    }
    else if(stCfg->enSet & EN_DRV_HVSP_FBMG_SET_LDCPATH_OFF)
    {
        Hal_HVSP_SetLDCPathSel(0);
        printf("LDC OFF\n");
    }
    if(stCfg->enSet & EN_DRV_HVSP_FBMG_SET_PRVCROP_ON)
    {
        Hal_HVSP_SetPrv2CropOnOff(1);
        printf("PRVCROP ON\n");
    }
    else if(stCfg->enSet & EN_DRV_HVSP_FBMG_SET_PRVCROP_OFF)
    {
        Hal_HVSP_SetPrv2CropOnOff(0);
        printf("PRVCROP OFF\n");
    }
    if(stCfg->enSet & EN_DRV_HVSP_FBMG_SET_DNR_Read_ON)
    {
        Hal_HVSP_SetIPMYCMReadEn(1);
        printf("DNRR ON\n");
    }
    else if(stCfg->enSet & EN_DRV_HVSP_FBMG_SET_DNR_Read_OFF)
    {
        Hal_HVSP_SetIPMYCMReadEn(0);
        printf("DNRR OFF\n");
    }
    if(stCfg->enSet & EN_DRV_HVSP_FBMG_SET_CIIR_ON)
    {
        Hal_HVSP_SetIPMCIIRReadEn(1);
        Hal_HVSP_SetIPMCIIRWriteEn(1);
        printf("DNRR ON\n");
    }
    else if(stCfg->enSet & EN_DRV_HVSP_FBMG_SET_CIIR_OFF)
    {
        Hal_HVSP_SetIPMCIIRReadEn(0);
        Hal_HVSP_SetIPMCIIRWriteEn(0);
        printf("DNRR OFF\n");
    }
    if(stCfg->enSet & EN_DRV_HVSP_FBMG_SET_DNR_Write_ON)
    {
        Hal_HVSP_SetIPMYCMWriteEn(1);
        printf("DNRW ON\n");
    }
    else if(stCfg->enSet & EN_DRV_HVSP_FBMG_SET_DNR_Write_OFF)
    {
        Hal_HVSP_SetIPMYCMWriteEn(0);
        printf("DNRW OFF\n");
    }
    if(stCfg->enSet & EN_DRV_HVSP_FBMG_SET_DNR_BUFFER_1)
    {
        Hal_HVSP_SetIPMBufferNumber(1);
        printf("DNRB 1\n");
    }
    else if(stCfg->enSet & EN_DRV_HVSP_FBMG_SET_DNR_BUFFER_2)
    {
        Hal_HVSP_SetIPMBufferNumber(2);
        printf("DNRB 2\n");
    }
    Hal_HVSP_SetFrameBufferManageLock(1);
    if(stCfg->enSet & EN_DRV_HVSP_FBMG_SET_UNLOCK)
    {
        Hal_HVSP_SetFrameBufferManageLock(0);
        Drv_SCLIRQ_SetDNRLock(0);
        printf("UNLOCK\n");
    }
    if(stCfg->enSet & EN_DRV_HVSP_FBMG_SET_LOCK)
    {
        Hal_HVSP_SetFrameBufferManageLock(1);
        Drv_SCLIRQ_SetDNRLock(1);
        printf("LOCK\n");
    }
}

void _Drv_HVSP_SetCMDQTrigFire(ST_HVSP_CMD_TRIG_CONFIG *stCmdTrigCfg,EN_HVSP_ID_TYPE HVSP_IP)
{
    MS_U32 u32Events = 0;
    if(stCmdTrigCfg->enType != E_HVSP_CMD_TRIG_NONE)
    {
        if(stCmdTrigCfg->enType == E_HVSP_CMD_TRIG_POLL_LDC_SYNC)
        {
            if(HVSP_IP == E_HVSP_ID_3)
            {
                MsOS_WaitEvent(Drv_SCLIRQ_Get_IRQ_SC3EventID(),
                    E_SCLIRQ_SC3EVENT_HVSPST, &u32Events, E_OR, MSOS_WAIT_FOREVER); // get status: FRM END
                MsOS_ClearEventIRQ(Drv_SCLIRQ_Get_IRQ_SC3EventID(),E_SCLIRQ_SC3EVENT_HVSPST);
            }
            else
            {
                MsOS_WaitEvent(Drv_SCLIRQ_Get_IRQ_EventID(),
                    E_SCLIRQ_EVENT_HVSPST, &u32Events, E_OR, MSOS_WAIT_FOREVER); // get status: FRM END
                MsOS_ClearEventIRQ(Drv_SCLIRQ_Get_IRQ_EventID(),E_SCLIRQ_EVENT_HVSPST);
            }
        }
        Hal_HVSP_SetCMDQTrigFire();
        SCL_DBG(SCL_DBG_LV_DRVHVSP()&(Get_DBGMG_HVSP(HVSP_IP)),
            "[DRVHVSP]%s id:%d @:%lu\n", __FUNCTION__,HVSP_IP,(MS_U32)MsOS_GetSystemTime());
    }
    else
    {
        SCL_DBG(SCL_DBG_LV_DRVHVSP()&(Get_DBGMG_HVSP(HVSP_IP)),
            "[DRVHVSP]%s Not Support id:%d @:%lu\n", __FUNCTION__,HVSP_IP,(MS_U32)MsOS_GetSystemTime());
    }
}
void Drv_HVSP_SetMemoryAllocateReady(MS_BOOL bEn)
{
    gbMemReadyForDrv = bEn;
}
MS_U8 _Drv_HVSP_GetScalingHRatioConfig(EN_HVSP_ID_TYPE enHVSP_ID,MS_U8 bUp)
{
    MS_U8 bret = 0;
    if(bUp)
    {
        if(enHVSP_ID ==E_HVSP_ID_1)
        {
            bret = SRAMFORSCALUP;
        }
        else if(enHVSP_ID ==E_HVSP_ID_2)
        {
            bret = SRAMFORSC2ALUP;
        }
        else if(enHVSP_ID ==E_HVSP_ID_3)
        {
            bret = SRAMFORSC3HUP;
        }
    }
    else
    {
        if(enHVSP_ID ==E_HVSP_ID_1)
        {
            bret = SRAMFORSCALDOWN;
        }
        else if(enHVSP_ID ==E_HVSP_ID_2)
        {
            if(gstScalinInfo[enHVSP_ID].stSizeAfterScaling.u16Width >=HD_Width)
            {
                bret = SRAMFORSCALDOWN;
            }
            else
            {
                bret = SRAMFORSC2ALDOWN;
            }
        }
        else if(enHVSP_ID ==E_HVSP_ID_3)
        {
            // if sc 2 output > 704x576 use talbe 1.
            if(gstScalinInfo[E_HVSP_ID_2].stSizeAfterScaling.u16Width >=D_Width)
            {
                bret = SRAMFORSC2ALDOWN;
            }
            else
            {
                bret = SRAMFORSC3HDOWN;
            }
        }
    }
    return bret;
}
MS_U8 _Drv_HVSP_GetScalingVRatioConfig(EN_HVSP_ID_TYPE enHVSP_ID,MS_U8 bUp)
{
    MS_U8 bret = 0;
    if(bUp)
    {
        if(enHVSP_ID ==E_HVSP_ID_1)
        {
            bret = SRAMFORSCALUP;
        }
        else if(enHVSP_ID ==E_HVSP_ID_2)
        {
            bret = SRAMFORSC2ALUP;
        }
        else if(enHVSP_ID ==E_HVSP_ID_3)
        {
            bret = SRAMFORSC3VUP;
        }
    }
    else
    {
        if(enHVSP_ID ==E_HVSP_ID_1)
        {
            bret = SRAMFORSCALDOWN;
        }
        else if(enHVSP_ID ==E_HVSP_ID_2)
        {
            if(gstScalinInfo[enHVSP_ID].stSizeAfterScaling.u16Height >=HD_Height)
            {
                bret = SRAMFORSCALDOWN;
            }
            else
            {
                bret = SRAMFORSC2ALDOWN;
            }
        }
        else if(enHVSP_ID ==E_HVSP_ID_3)
        {
            // if sc 2 output > 704x576 use talbe 1.
            if(gstScalinInfo[E_HVSP_ID_2].stSizeAfterScaling.u16Height >=D_Height)
            {
                bret = SRAMFORSCALDOWN;
            }
            else
            {
                bret = SRAMFORSC3VDOWN;
            }
        }
    }
    return bret;
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
void _Drv_HVSP_SetHTbl(EN_HVSP_ID_TYPE enHVSP_ID)
{
    if(gbHScalingup[enHVSP_ID] &0x1)
    {
        if(enHVSP_ID ==E_HVSP_ID_1)
        {
            gbHScalingup[enHVSP_ID] = SRAMFORSCALUP;
        }
        else if(enHVSP_ID ==E_HVSP_ID_2)
        {
            gbHScalingup[enHVSP_ID] = SRAMFORSC2ALUP;
        }
        else if(enHVSP_ID ==E_HVSP_ID_3)
        {
            gbHScalingup[enHVSP_ID] = SRAMFORSC3HUP;
        }
    }
    else
    {
        if(enHVSP_ID ==E_HVSP_ID_1)
        {
            gbHScalingup[enHVSP_ID] = SRAMFORSCALDOWN;
        }
        else if(enHVSP_ID ==E_HVSP_ID_2)
        {
            if(gstScalinInfo[enHVSP_ID].stSizeAfterScaling.u16Width >=HD_Width)
            {
                gbHScalingup[enHVSP_ID] = SRAMFORSCALDOWN;
            }
            else
            {
                gbHScalingup[enHVSP_ID] = SRAMFORSC2ALDOWN;
            }
        }
        else if(enHVSP_ID ==E_HVSP_ID_3)
        {
            // if sc 2 output > 704x576 use talbe 1.
            if(gstScalinInfo[E_HVSP_ID_2].stSizeAfterScaling.u16Width >=D_Width)
            {
                gbHScalingup[enHVSP_ID] = SRAMFORSC2ALDOWN;
            }
            else
            {
                gbHScalingup[enHVSP_ID] = SRAMFORSC3HDOWN;
            }
        }
    }
}
void _Drv_HVSP_SetVTbl(EN_HVSP_ID_TYPE enHVSP_ID)
{
    if(gbVScalingup[enHVSP_ID] &0x1)
    {
        if(enHVSP_ID ==E_HVSP_ID_1)
        {
            gbVScalingup[enHVSP_ID] = SRAMFORSCALUP;
        }
        else if(enHVSP_ID ==E_HVSP_ID_2)
        {
            gbVScalingup[enHVSP_ID] = SRAMFORSC2ALUP;
        }
        else if(enHVSP_ID ==E_HVSP_ID_3)
        {
            gbVScalingup[enHVSP_ID] = SRAMFORSC3VUP;
        }
    }
    else
    {
        if(enHVSP_ID ==E_HVSP_ID_1)
        {
            gbVScalingup[enHVSP_ID] = SRAMFORSCALDOWN;
        }
        else if(enHVSP_ID ==E_HVSP_ID_2)
        {
            if(gstScalinInfo[enHVSP_ID].stSizeAfterScaling.u16Height>=HD_Height)
            {
                gbVScalingup[enHVSP_ID] = SRAMFORSCALDOWN;
            }
            else
            {
                gbVScalingup[enHVSP_ID] = SRAMFORSC2ALDOWN;
            }
        }
        else if(enHVSP_ID ==E_HVSP_ID_3)
        {
            if(gstScalinInfo[E_HVSP_ID_2].stSizeAfterScaling.u16Height >=D_Height)
            {
                gbVScalingup[enHVSP_ID] = SRAMFORSCALDOWN;
            }
            else
            {
                gbVScalingup[enHVSP_ID] = SRAMFORSC3VDOWN;
            }
        }
    }
}
void _Drv_HVSP_SetHorizotnalSRAMTbl(EN_HVSP_ID_TYPE enHVSP_ID)
{
    MS_U32 u32flag;
    if((_Change_SRAM_H_Qmap(enHVSP_ID,(gbHScalingup[enHVSP_ID]&0x1))))
    {
        gbHScalingup[enHVSP_ID] &= 0x1;
        gbHScalingup[enHVSP_ID] = (gbHScalingup[enHVSP_ID]^1);//XOR 1 :reverse
        //gbHScalingup[enHVSP_ID] |= ((_Drv_HVSP_GetScalingHRatioConfig(enHVSP_ID,gbHScalingup[enHVSP_ID]&0x1))<<4);
        _Drv_HVSP_SetHTbl(enHVSP_ID);
        u32flag = (enHVSP_ID==E_HVSP_ID_1) ? E_SCLIRQ_EVENT_SC1HSRAMSET :
                  (enHVSP_ID==E_HVSP_ID_2) ? E_SCLIRQ_EVENT_SC2HSRAMSET :
                                             E_SCLIRQ_EVENT_SC3HSRAMSET;
        if(Drv_SCLIRQ_GetIsBlankingRegion()|| enHVSP_ID ==E_HVSP_ID_3 || VIPSETRULE())
        {
            Hal_HVSP_SRAM_Dump((EN_HVSP_SRAM_DUMP_TYPE)(enHVSP_ID+HVSP_ID_SRAM_H_OFFSET),gbHScalingup[enHVSP_ID]); //level 1 :up 0:down
            SCL_DBG(SCL_DBG_LV_DRVHVSP()&(Get_DBGMG_HVSP(enHVSP_ID)),
                "[DRVHVSP]Change and do H Qmap SRAM id:%d scaling UP:%hhx\n",enHVSP_ID,gbHScalingup[enHVSP_ID]);
        }
        else
        {
            MsOS_SetEvent_IRQ(Drv_SCLIRQ_Get_IRQ_SYNCEventID(), u32flag);
            SCL_DBG(SCL_DBG_LV_DRVHVSP()&(Get_DBGMG_HVSP(enHVSP_ID)),
                "[DRVHVSP]Change H Qmap SRAM id:%d scaling UP:%hhx\n",enHVSP_ID,gbHScalingup[enHVSP_ID]);
        }
    }
    else if(gbSc3FirstHSet && enHVSP_ID ==E_HVSP_ID_3)
    {
        gbSc3FirstHSet = 0;
        _Drv_HVSP_SetHTbl(enHVSP_ID);
         if(enHVSP_ID ==E_HVSP_ID_3)
         {
             Hal_HVSP_SRAM_Dump((EN_HVSP_SRAM_DUMP_TYPE)(enHVSP_ID+HVSP_ID_SRAM_H_OFFSET),gbHScalingup[enHVSP_ID]); //level 1 :up 0:down
             SCL_DBG(SCL_DBG_LV_DRVHVSP()&(Get_DBGMG_HVSP(enHVSP_ID)),
                 "[DRVHVSP]Change and do H Qmap SRAM id:%d scaling static UP:%hhx\n",enHVSP_ID,gbHScalingup[enHVSP_ID]);
         }
    }
    else if(enHVSP_ID !=E_HVSP_ID_3 && ((_Drv_HVSP_GetScalingHRatioConfig(enHVSP_ID,gbHScalingup[enHVSP_ID]&0x1))!=
        ((gbHScalingup[enHVSP_ID]))))
    {
        //gbHScalingup[enHVSP_ID] &= 0x1;
        //gbHScalingup[enHVSP_ID] |= ((_Drv_HVSP_GetScalingHRatioConfig(enHVSP_ID,gbHScalingup[enHVSP_ID]&0x1))<<4);
        _Drv_HVSP_SetHTbl(enHVSP_ID);
        u32flag = (enHVSP_ID==E_HVSP_ID_1) ? E_SCLIRQ_EVENT_SC1HSRAMSET :
                  (enHVSP_ID==E_HVSP_ID_2) ? E_SCLIRQ_EVENT_SC2HSRAMSET :
                                             E_SCLIRQ_EVENT_SC3HSRAMSET;
        if(Drv_SCLIRQ_GetIsBlankingRegion()|| enHVSP_ID ==E_HVSP_ID_3 || VIPSETRULE())
        {
            Hal_HVSP_SRAM_Dump((EN_HVSP_SRAM_DUMP_TYPE)(enHVSP_ID+HVSP_ID_SRAM_H_OFFSET),gbHScalingup[enHVSP_ID]); //level 1 :up 0:down
            SCL_DBG(SCL_DBG_LV_DRVHVSP()&(Get_DBGMG_HVSP(enHVSP_ID)),
                "[DRVHVSP]Change and do H Qmap SRAM id:%d scaling RUP:%hhx\n",enHVSP_ID,gbHScalingup[enHVSP_ID]);
        }
        else
        {
            MsOS_SetEvent_IRQ(Drv_SCLIRQ_Get_IRQ_SYNCEventID(), u32flag);
            SCL_DBG(SCL_DBG_LV_DRVHVSP()&(Get_DBGMG_HVSP(enHVSP_ID)),
                "[DRVHVSP]Change H Qmap SRAM id:%d scaling RUP:%hhx\n",enHVSP_ID,gbHScalingup[enHVSP_ID]);
        }
    }
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
void _Drv_HVSP_SetVerticalSRAMTbl(EN_HVSP_ID_TYPE enHVSP_ID)
{
    MS_U32 u32flag;
    if(_Change_SRAM_V_Qmap(enHVSP_ID,(gbVScalingup[enHVSP_ID]&0x1)))
    {
        gbVScalingup[enHVSP_ID] &= 0x1;
        gbVScalingup[enHVSP_ID] = (gbVScalingup[enHVSP_ID]^1);//XOR 1 :reverse
        //gbVScalingup[enHVSP_ID] |= ((_Drv_HVSP_GetScalingVRatioConfig(enHVSP_ID,gbVScalingup[enHVSP_ID]&0x1))<<4);
        _Drv_HVSP_SetVTbl(enHVSP_ID);
        u32flag = (enHVSP_ID==E_HVSP_ID_1) ? E_SCLIRQ_EVENT_SC1VSRAMSET :
                  (enHVSP_ID==E_HVSP_ID_2) ? E_SCLIRQ_EVENT_SC2VSRAMSET :
                                             E_SCLIRQ_EVENT_SC3VSRAMSET;

        if(Drv_SCLIRQ_GetIsBlankingRegion()|| enHVSP_ID ==E_HVSP_ID_3 || VIPSETRULE())
        {
            Hal_HVSP_SRAM_Dump((EN_HVSP_SRAM_DUMP_TYPE)(enHVSP_ID+HVSP_ID_SRAM_V_OFFSET),gbVScalingup[enHVSP_ID]); //level 1 :up 0:down
            SCL_DBG(SCL_DBG_LV_DRVHVSP()&(Get_DBGMG_HVSP(enHVSP_ID)),
                "[DRVHVSP]Change and do V Qmap SRAM id:%d scaling UP:%hhx\n",enHVSP_ID,gbVScalingup[enHVSP_ID]);
        }
        else
        {
            MsOS_SetEvent_IRQ(Drv_SCLIRQ_Get_IRQ_SYNCEventID(), u32flag);
            SCL_DBG(SCL_DBG_LV_DRVHVSP()&(Get_DBGMG_HVSP(enHVSP_ID)),
                "[DRVHVSP]Change V Qmap SRAM id:%d scaling UP:%hhx\n",enHVSP_ID,gbVScalingup[enHVSP_ID]);
        }
    }
    else if(gbSc3FirstVSet && enHVSP_ID ==E_HVSP_ID_3)
    {
        gbSc3FirstVSet = 0;
        _Drv_HVSP_SetVTbl(enHVSP_ID);
         if(enHVSP_ID ==E_HVSP_ID_3)
         {
             Hal_HVSP_SRAM_Dump((EN_HVSP_SRAM_DUMP_TYPE)(enHVSP_ID+HVSP_ID_SRAM_V_OFFSET),gbVScalingup[enHVSP_ID]); //level 1 :up 0:down
             SCL_DBG(SCL_DBG_LV_DRVHVSP()&(Get_DBGMG_HVSP(enHVSP_ID)),
                 "[DRVHVSP]Change and do V Qmap SRAM id:%d scaling static UP:%hhx\n",enHVSP_ID,gbVScalingup[enHVSP_ID]);
         }
    }
    else if(enHVSP_ID !=E_HVSP_ID_3 && ((_Drv_HVSP_GetScalingVRatioConfig(enHVSP_ID,gbVScalingup[enHVSP_ID]&0x1))!=
        ((gbVScalingup[enHVSP_ID]))))
    {
        //gbVScalingup[enHVSP_ID] &= 0x1;
        //gbVScalingup[enHVSP_ID] |= ((_Drv_HVSP_GetScalingVRatioConfig(enHVSP_ID,gbVScalingup[enHVSP_ID]&0x1))<<4);
        _Drv_HVSP_SetVTbl(enHVSP_ID);
        u32flag = (enHVSP_ID==E_HVSP_ID_1) ? E_SCLIRQ_EVENT_SC1VSRAMSET :
                  (enHVSP_ID==E_HVSP_ID_2) ? E_SCLIRQ_EVENT_SC2VSRAMSET :
                                             E_SCLIRQ_EVENT_SC3VSRAMSET;

        if(Drv_SCLIRQ_GetIsBlankingRegion()|| enHVSP_ID ==E_HVSP_ID_3 || VIPSETRULE())
        {
            Hal_HVSP_SRAM_Dump((EN_HVSP_SRAM_DUMP_TYPE)(enHVSP_ID+HVSP_ID_SRAM_V_OFFSET),gbVScalingup[enHVSP_ID]); //level 1 :up 0:down
            SCL_DBG(SCL_DBG_LV_DRVHVSP()&(Get_DBGMG_HVSP(enHVSP_ID)),
                "[DRVHVSP]Change and do V Qmap SRAM id:%d scaling RUP:%hhx\n",enHVSP_ID,gbVScalingup[enHVSP_ID]);
        }
        else
        {
            MsOS_SetEvent_IRQ(Drv_SCLIRQ_Get_IRQ_SYNCEventID(), u32flag);
            SCL_DBG(SCL_DBG_LV_DRVHVSP()&(Get_DBGMG_HVSP(enHVSP_ID)),
                "[DRVHVSP]Change V Qmap SRAM id:%d scaling RUP:%hhx\n",enHVSP_ID,gbVScalingup[enHVSP_ID]);
        }
    }
}
void _Drv_HVSP_SetCrop(MS_BOOL u8CropID,ST_HVSP_SCALING_CONFIG *stCfg,ST_HVSP_CROP_INFO *stCropInfo)
{
    MS_U16 u16In_hsize;
    MS_U16 u16In_vsize;
    u16In_hsize = (u8CropID ==DRV_HVSP_CROP_1) ? stCfg->u16Src_Width : stCfg->u16Crop_Width[DRV_HVSP_CROP_1];
    u16In_vsize = (u8CropID ==DRV_HVSP_CROP_1) ? stCfg->u16Src_Height : stCfg->u16Crop_Height[DRV_HVSP_CROP_1];
    if(stCfg->bCropEn[u8CropID])
    {
        stCropInfo->bEn         = stCfg->bCropEn[u8CropID];
        if(u16In_hsize && u16In_vsize)
        {
            stCropInfo->u16In_hsize = u16In_hsize;
            stCropInfo->u16In_vsize = u16In_vsize;
        }
        else
        {
            stCropInfo->u16In_hsize = stCfg->u16Src_Width;
            stCropInfo->u16In_vsize = stCfg->u16Src_Height;
        }
        stCropInfo->u16Hsize    = stCfg->u16Crop_Width[u8CropID];
        stCropInfo->u16Vsize    = stCfg->u16Crop_Height[u8CropID];
        if(stCfg->u16Crop_Width[u8CropID]%2)
        {
            stCropInfo->u16Hsize      = stCfg->u16Crop_Width[u8CropID]-1;
        }
        if(u8CropID == DRV_HVSP_CROP_1)
        {
            gstIPMCfg.u16Fetch = stCropInfo->u16Hsize;
            gstIPMCfg.u16Vsize = stCropInfo->u16Vsize;
            Drv_HVSP_SetInputSrcSize(stCfg->u16Src_Width,stCfg->u16Src_Height);
        }
        if(stCfg->u16Crop_X[u8CropID]%2)
        {
            stCropInfo->u16Hst      = stCfg->u16Crop_X[u8CropID]-1;
        }
        else
        {
            stCropInfo->u16Hst      = stCfg->u16Crop_X[u8CropID];
        }
        stCropInfo->u16Vst      = stCfg->u16Crop_Y[u8CropID];
    }
    else
    {
        stCropInfo->bEn         = 0;
        if(u16In_hsize && u16In_vsize)
        {
            stCropInfo->u16In_hsize = u16In_hsize;
            stCropInfo->u16In_vsize = u16In_vsize;
            stCropInfo->u16Hsize    = u16In_hsize;//stCfg.u16Src_Width;
            stCropInfo->u16Vsize    = u16In_vsize;//stCfg.u16Src_Height;
        }
        else
        {
            stCropInfo->u16In_hsize = stCfg->u16Src_Width;
            stCropInfo->u16In_vsize = stCfg->u16Src_Height;
            stCropInfo->u16Hsize    = stCfg->u16Src_Width;//stCfg.u16Src_Width;
            stCropInfo->u16Vsize    = stCfg->u16Src_Height;//stCfg.u16Src_Height;
        }
        if(u8CropID == DRV_HVSP_CROP_1)
        {
            gstIPMCfg.u16Fetch = stCropInfo->u16Hsize;
            gstIPMCfg.u16Vsize = stCropInfo->u16Vsize;
            Drv_HVSP_SetInputSrcSize(stCropInfo->u16In_hsize,stCropInfo->u16In_vsize);
        }
        stCropInfo->u16Hst      = 0;
        stCropInfo->u16Vst      = 0;
    }
}
void _Drv_HVSP_SetScalingRetVal(ST_HVSP_CROP_INFO *stCropInfo_1, ST_HVSP_CROP_INFO *stCropInfo_2, ST_HVSP_SCALING_CONFIG *stCfg)
{
    stCfg->bCropEn[DRV_HVSP_CROP_1] = stCropInfo_1->bEn;
    stCfg->u16Crop_Height[DRV_HVSP_CROP_1] = stCropInfo_1->u16Vsize;
    stCfg->u16Crop_Width[DRV_HVSP_CROP_1] = stCropInfo_1->u16Hsize;
    stCfg->u16Crop_X[DRV_HVSP_CROP_1] = stCropInfo_1->u16Hst;
    stCfg->u16Crop_Y[DRV_HVSP_CROP_1] = stCropInfo_1->u16Vst;
    stCfg->bCropEn[DRV_HVSP_CROP_2] = stCropInfo_2->bEn;
    stCfg->u16Crop_Height[DRV_HVSP_CROP_2] = stCropInfo_2->u16Vsize;
    stCfg->u16Crop_Width[DRV_HVSP_CROP_2] = stCropInfo_2->u16Hsize;
    stCfg->u16Crop_X[DRV_HVSP_CROP_2] = stCropInfo_2->u16Hst;
    stCfg->u16Crop_Y[DRV_HVSP_CROP_2] = stCropInfo_2->u16Vst;
}
MS_BOOL Drv_HVSP_SetScaling(EN_HVSP_ID_TYPE enHVSP_ID, ST_HVSP_SCALING_CONFIG *stCfg, ST_HVSP_CLK_CONFIG* stclk)
{
    unsigned char u8FBbuffer = Drv_HVSP_GetBufferNum();
    EN_HVSP_ISPCLK_TYPE enISPClkType = Hal_HVSP_GetISPClkType();
    ST_HVSP_CROP_INFO stCropInfo_2;
    ST_HVSP_CROP_INFO stCropInfo_1;
    MS_U16  u16Hst = Hal_HVSP_Get_Crop_X();
    MS_U16  u16Vst = Hal_HVSP_Get_Crop_Y();
    MS_BOOL bChangePreCropPosition = 0;
    MS_U16 u16FBWidth = gstIPMCfg.u16Fetch;
    MS_U16 u16FBHeight = gstIPMCfg.u16Vsize;
    MS_U32 u32bLDCEvent = 0;
    MS_U64 u64temp;
    MsOS_Memset(&stCropInfo_2,0,sizeof(ST_HVSP_CROP_INFO));
    MsOS_Memset(&stCropInfo_1,0,sizeof(ST_HVSP_CROP_INFO));
    stCropInfo_1.bEn = 0;
    stCropInfo_2.bEn = 0;
    stCfg->bRet = 1;
    if(enHVSP_ID == E_HVSP_ID_1)
    {
        MS_U32 u32FrameBufferMemSize = gstIPMCfg.u32MemSize;
        MS_U32 u32ReqMemSize = stCfg->u16Src_Width * stCfg->u16Src_Height * 2 * u8FBbuffer;


        SCL_DBG(SCL_DBG_LV_DRVHVSP()&(Get_DBGMG_HVSP(enHVSP_ID)), "[DRVHVSP]%s id:%d @:%lu\n",
            __FUNCTION__,enHVSP_ID,(MS_U32)MsOS_GetSystemTime());

        if(Is_DNRBufferReady())
        {
            if(Is_FrameBufferTooSmall(u32ReqMemSize, u32FrameBufferMemSize))
            {
                DRV_HVSP_ERR(printf("[DRVHVSP]%s %d:: Out of memory\n", __FUNCTION__, __LINE__));
                stCfg->bRet = 0;
                return stCfg->bRet;
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
        if(stCfg->u16Src_Width != gstIPMCfg.u16Fetch)
        {
            SCL_DBGERR("[DRVHVSP]%s %d:: H Src Size Mismatch(%hd,%hd)\n",
                __FUNCTION__, __LINE__,stCfg->u16Src_Width,gstIPMCfg.u16Fetch);
        }
        if(stCfg->u16Src_Height != gstIPMCfg.u16Vsize)
        {
            SCL_DBGERR("[DRVHVSP]%s %d:: V Src Size Mismatch(%hd,%hd)\n",
                __FUNCTION__, __LINE__,stCfg->u16Src_Height,gstIPMCfg.u16Vsize);
        }

        // setup cmd trig config
        DRV_HVSP_MUTEX_LOCK();
        Drv_HVSP_SetCMDQTrigType(&stCfg->stCmdTrigCfg);
        _Drv_HVSP_SetCrop(DRV_HVSP_CROP_1,stCfg,&stCropInfo_1);
        _Drv_HVSP_SetCrop(DRV_HVSP_CROP_2,stCfg,&stCropInfo_2);
        if(_Is_SingleBufferAndZoomSizeBiggerthanLimitation
            (u8FBbuffer,stCropInfo_2.u16Vsize, Hal_HVSP_GetLimitationByISPClk
            (enISPClkType, stCropInfo_1.u16Hsize ,stCfg->u16Dsp_Height)))
        {
            if(stCfg->bCropEn[DRV_HVSP_CROP_1])
            {
                stCfg->u16Crop_Height[DRV_HVSP_CROP_1] =
                    Hal_HVSP_GetLimitationByISPClk(enISPClkType, stCropInfo_1.u16Hsize ,stCfg->u16Dsp_Height) ;
                stCropInfo_1.u16Vsize    = stCfg->u16Crop_Height[DRV_HVSP_CROP_1];
                stCropInfo_1.u16Vst = ((stCropInfo_1.u16Vst+stCropInfo_1.u16Vsize)>stCropInfo_1.u16In_vsize) ?
                    (stCropInfo_1.u16In_vsize - stCropInfo_1.u16Vsize) : stCropInfo_1.u16Vst;
                gstIPMCfg.u16Vsize = stCropInfo_1.u16Vsize;
                stCropInfo_2.u16In_vsize = gstIPMCfg.u16Vsize;
                stCropInfo_2.u16Vsize    = gstIPMCfg.u16Vsize;//stCfg.u16Src_Height;
                SCL_ERR("[DRVHVSP]%s %d:: Buffer num : %hhd ,Zoom size bigger than Limitation and force to change(%hd,%hd,%hd,%hd)\n"
                    , __FUNCTION__, __LINE__,u8FBbuffer,stCropInfo_1.u16Hst,stCropInfo_1.u16Vst,
                    stCfg->u16Crop_Width[DRV_HVSP_CROP_1],stCfg->u16Crop_Height[DRV_HVSP_CROP_1]);
            }
            if(stCfg->bCropEn[DRV_HVSP_CROP_2])
            {
                stCfg->u16Crop_Height[DRV_HVSP_CROP_2] =
                    Hal_HVSP_GetLimitationByISPClk(enISPClkType, stCropInfo_2.u16Hsize ,stCfg->u16Dsp_Height);
                stCropInfo_2.u16Vsize    = stCfg->u16Crop_Height[DRV_HVSP_CROP_2];
                stCropInfo_2.u16Vst = ((stCropInfo_2.u16Vst+stCropInfo_2.u16Vsize)>stCropInfo_2.u16In_vsize) ?
                    (stCropInfo_2.u16In_vsize - stCropInfo_2.u16Vsize) : stCropInfo_2.u16Vst;
               SCL_ERR("[DRVHVSP]%s %d:: Buffer num : %hhd ,Zoom size bigger than Limitation and force to change(%hd,%hd,%hd,%hd)\n"
                    , __FUNCTION__, __LINE__,u8FBbuffer,stCropInfo_2.u16Hst,stCropInfo_2.u16Vst,
                    stCfg->u16Crop_Width[DRV_HVSP_CROP_2],stCfg->u16Crop_Height[DRV_HVSP_CROP_2]);
            }
            _Drv_HVSP_SetScalingRetVal(&stCropInfo_1,&stCropInfo_2,stCfg);
            stCfg->bRet = 0;
        }

        gstScalinInfo[enHVSP_ID].stSizeAfterCrop.u16Width  = stCropInfo_2.u16Hsize;//stCfg.u16Src_Width;
        gstScalinInfo[enHVSP_ID].stSizeAfterCrop.u16Height = stCropInfo_2.u16Vsize;//stCfg.u16Src_Height;
        gstScalinInfo[enHVSP_ID].stSizeAfterScaling.u16Width  = stCfg->u16Dsp_Width;
        gstScalinInfo[enHVSP_ID].stSizeAfterScaling.u16Height = stCfg->u16Dsp_Height;
        gstIPMCfg.u16Fetch = stCropInfo_2.u16In_hsize;
        gstIPMCfg.u16Vsize = stCropInfo_2.u16In_vsize;
        // Crop1
        Hal_HVSP_SetCropConfig(E_HVSP_CROP_ID_1, &stCropInfo_1);
        // Crop2
        Hal_HVSP_SetCropConfig(E_HVSP_CROP_ID_2, &stCropInfo_2);
        Hal_HVSP_SetIPMvSize(stCropInfo_2.u16In_vsize);
        Hal_HVSP_SetIPMLineOffset(stCropInfo_2.u16In_hsize);
        Hal_HVSP_SetIPMFetchNum(stCropInfo_2.u16In_hsize);
        Hal_HVSP_SetLDCWidth(stCropInfo_2.u16In_hsize);
        Hal_HVSP_SetLDCHeight(stCropInfo_2.u16In_vsize);
        if(stCfg->bCropEn[DRV_HVSP_CROP_1])
        {
            // ToDo
            //for rotate
            if(stCropInfo_2.u16In_hsize <= FHD_Width && stCropInfo_2.u16In_vsize <= FHD_Width)
            {
                Drv_HVSP_SetPrv2CropOnOff(0);
                Hal_HVSP_SetLDCPathSel(stCfg->bCropEn[DRV_HVSP_CROP_1]);
                u32bLDCEvent = stCfg->bCropEn[DRV_HVSP_CROP_1];
            }
            else
            {
                Hal_HVSP_SetLDCPathSel(0);
                u32bLDCEvent = 0;
                // disable for BW issue
                //Drv_HVSP_SetPrv2CropOnOff(stCfg.bCropEn[DRV_HVSP_CROP_1]);
            }
        }
        else
        {
            if(Hal_HVSP_GetPrv2CropOnOff())
            {
                Drv_HVSP_SetPrv2CropOnOff(stCfg->bCropEn[DRV_HVSP_CROP_1]);
            }
            //for rotate
            else if(gstSrcSize.u16Width >FHD_Width || gstSrcSize.u16Height >FHD_Width)
            {
                Hal_HVSP_SetLDCPathSel(stCfg->bCropEn[DRV_HVSP_CROP_1]);
                u32bLDCEvent = stCfg->bCropEn[DRV_HVSP_CROP_1];
            }
            else
            {
                u32bLDCEvent = Hal_HVSP_GetLDCPathSel();
            }
        }
        // NLM size
        Hal_HVSP_SetVIPSize(gstScalinInfo[enHVSP_ID].stSizeAfterCrop.u16Width,
                gstScalinInfo[enHVSP_ID].stSizeAfterCrop.u16Height);
        Hal_HVSP_SetNLMLineBufferSize(stCropInfo_2.u16In_hsize,stCropInfo_2.u16In_vsize);
        Hal_HVSP_SetNLMEn(1);
        //AIP size
        Hal_HVSP_SetXNRSize(stCropInfo_2.u16In_hsize,stCropInfo_2.u16In_vsize);
        Hal_HVSP_SetWDRGlobalSize(gstScalinInfo[enHVSP_ID].stSizeAfterCrop.u16Width,
                gstScalinInfo[enHVSP_ID].stSizeAfterCrop.u16Height);
        Hal_HVSP_SetWDRLocalSize(gstScalinInfo[enHVSP_ID].stSizeAfterCrop.u16Width,
                gstScalinInfo[enHVSP_ID].stSizeAfterCrop.u16Height);
        Hal_HVSP_SetMXNRSize(gstScalinInfo[enHVSP_ID].stSizeAfterCrop.u16Width,
                gstScalinInfo[enHVSP_ID].stSizeAfterCrop.u16Height);
        Hal_HVSP_SetUVadjSize(gstScalinInfo[enHVSP_ID].stSizeAfterCrop.u16Width,
                gstScalinInfo[enHVSP_ID].stSizeAfterCrop.u16Height);
        Hal_HVSP_PriMask_Width(gstScalinInfo[enHVSP_ID].stSizeAfterCrop.u16Width);
        Hal_HVSP_PriMask_Height(gstScalinInfo[enHVSP_ID].stSizeAfterCrop.u16Height);
        if(gbPriMaskPending)
        {
            gbPriMaskPending = 0;
            Hal_HVSP_PriMask_bEn(EN_HVSP_TRIGGER_CMDQ);
        }
        bChangePreCropPosition = _IsChangePreCropPosition(stCropInfo_1.u16Hst,stCropInfo_1.u16Vst,u16Hst,u16Vst);
    }
    else
    {
        if(enHVSP_ID == E_HVSP_ID_3 &&_IsFlagType(E_SCLDMA_3_FRM_W,E_SCLDMA_FLAG_ACTIVE))
        {
            stCfg->bRet = 0;
            return stCfg->bRet;
        }
        DRV_HVSP_MUTEX_LOCK();
        Drv_HVSP_SetCMDQTrigType(&stCfg->stCmdTrigCfg);
        gstScalinInfo[enHVSP_ID].stSizeAfterCrop.u16Width      = stCfg->u16Src_Width;
        gstScalinInfo[enHVSP_ID].stSizeAfterCrop.u16Height     = stCfg->u16Src_Height;
        gstScalinInfo[enHVSP_ID].stSizeAfterScaling.u16Width   = stCfg->u16Dsp_Width;
        gstScalinInfo[enHVSP_ID].stSizeAfterScaling.u16Height  = stCfg->u16Dsp_Height;
    }
    if(gstScalinInfo[enHVSP_ID].stSizeAfterScaling.u16Width>1 && (gstScalinInfo[enHVSP_ID].stSizeAfterCrop.u16Width>1))
    {
        u64temp = (((MS_U64)gstScalinInfo[enHVSP_ID].stSizeAfterCrop.u16Width) * 1048576);
        do_div(u64temp, gstScalinInfo[enHVSP_ID].stSizeAfterScaling.u16Width);
        gstScalinInfo[enHVSP_ID].u32ScalingRatio_H = (MS_U32)u64temp;
    }
    else
    {
        SCL_ERR("[DRVHVSP]%d Ratio Error\n",enHVSP_ID);
    }
    if(gstScalinInfo[enHVSP_ID].stSizeAfterScaling.u16Height>1 && (gstScalinInfo[enHVSP_ID].stSizeAfterCrop.u16Height>1))
    {
        u64temp = (((MS_U64)gstScalinInfo[enHVSP_ID].stSizeAfterCrop.u16Height) * 1048576);
        do_div(u64temp, gstScalinInfo[enHVSP_ID].stSizeAfterScaling.u16Height);
        gstScalinInfo[enHVSP_ID].u32ScalingRatio_V = (MS_U32)u64temp;
    }
    else
    {
        SCL_ERR("[DRVHVSP]%d Ratio Error\n",enHVSP_ID);
    }
/*
    gstScalinInfo[enHVSP_ID].u32ScalingRatio_H = (MS_U32)HVSP_RATIO(gstScalinInfo[enHVSP_ID].stSizeAfterCrop.u16Width,
                                                            gstScalinInfo[enHVSP_ID].stSizeAfterScaling.u16Width);

    gstScalinInfo[enHVSP_ID].u32ScalingRatio_V = (MS_U32)HVSP_RATIO(gstScalinInfo[enHVSP_ID].stSizeAfterCrop.u16Height,
                                                            gstScalinInfo[enHVSP_ID].stSizeAfterScaling.u16Height);
*/
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
        _Drv_HVSP_SetHorizotnalSRAMTbl(enHVSP_ID);
    }

    // vertical HVSP Scaling
    if(gstScalinInfo[enHVSP_ID].stSizeAfterCrop.u16Height == gstScalinInfo[enHVSP_ID].stSizeAfterScaling.u16Height)
    {
        _Drv_HVSP_SetVerticalScalingConfig(enHVSP_ID, FALSE);
    }
    else
    {
        _Drv_HVSP_SetVerticalScalingConfig(enHVSP_ID, TRUE);
        _Drv_HVSP_SetVerticalSRAMTbl(enHVSP_ID);
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
    if(ISZOOMDROPFRAME)
    {
        _Drv_HVSP_SetCMDQTrigFire(&stCfg->stCmdTrigCfg,enHVSP_ID);
        if(_IsChangeFBBufferResolution(u16FBWidth,u16FBHeight) || bChangePreCropPosition)
        {
            //for 3DNR
            Drv_SCLIRQ_SetDropFrameFromCMDQDone(1);
        }
    }
    else
    {
        if(_IsChangeFBBufferResolution(u16FBWidth,u16FBHeight) || bChangePreCropPosition)
        {
            if(u32bLDCEvent)
            {
                SCL_DBG(SCL_DBG_LV_DRVHVSP()&(Get_DBGMG_HVSP(enHVSP_ID)),"[DRVHVSP]Without Drop frame LDC close\n");
                Hal_HVSP_SetResetDNR(1);
                _Drv_HVSP_SetCMDQTrigFire(&stCfg->stCmdTrigCfg,enHVSP_ID);
#if SCALING_BLOCK
#else
                if(Drv_SCLIRQ_GetEachDMAEn())
                {
                    MsOS_WaitEvent(Drv_SCLIRQ_Get_IRQ_SYNCEventID(), E_SCLIRQ_EVENT_CHANGESIZEDONE, &u32bLDCEvent, E_OR, 100);
                    MsOS_ClearEventIRQ(Drv_SCLIRQ_Get_IRQ_SYNCEventID(),(E_SCLIRQ_EVENT_CHANGESIZEDONE));
                }
#endif
                Hal_HVSP_SetLDCPathSel(1);
            }
            else
            {
                SCL_DBG(SCL_DBG_LV_DRVHVSP()&(Get_DBGMG_HVSP(enHVSP_ID)),"[DRVHVSP]Without Drop frame \n");
                Hal_HVSP_SetResetDNR(1);
            }
        }
        _Drv_HVSP_SetCMDQTrigFire(&stCfg->stCmdTrigCfg,enHVSP_ID);
    }
#if SCALING_BLOCK
    SCL_DBG(SCL_DBG_LV_DRVHVSP()&(Get_DBGMG_HVSP(enHVSP_ID)),"[DRVHVSP]Set Scaling factor Done\n");
#endif
/*
    if(stCfg.bCropEn[DRV_HVSP_CROP_1])
    {
        Drv_HVSP_SetCMDQTrigType(stCfg.stCmdTrigCfg);
        _Drv_HVSP_SetCMDQTrigFire(stCfg.stCmdTrigCfg,enHVSP_ID);
    }
*/
    DRV_HVSP_MUTEX_UNLOCK();
    stCfg->bRet = 1;
    return stCfg->bRet;
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
    Hal_HVSP_SetInputSrcSize(&gstSrcSize);
}
void Drv_HVSP_SetCropWindowSize(void)
{
    ST_HVSP_CROP_INFO stCropInfo_1;
    ST_HVSP_CMD_TRIG_CONFIG stCmdTrigCfg;
    MsOS_Memset(&stCropInfo_1,0,sizeof(ST_HVSP_CROP_INFO));
    MsOS_Memset(&stCmdTrigCfg,0,sizeof(ST_HVSP_CMD_TRIG_CONFIG));
    _Drv_HVSP_FillPreCropInfo(&stCropInfo_1);
    DRV_HVSP_MUTEX_LOCK();
    Drv_HVSP_SetCMDQTrigTypeByRIU(&stCmdTrigCfg);
    Hal_HVSP_SetCropConfig(E_HVSP_CROP_ID_1, &stCropInfo_1);
    Drv_HVSP_SetCMDQTrigType(&stCmdTrigCfg);
    DRV_HVSP_MUTEX_UNLOCK();
    SCL_DBG(SCL_DBG_LV_DRVHVSP()&(Get_DBGMG_HVSP(0)), "[DRVHVSP]%s Input:(%hd,%hd) Crop:(%hd,%hd)\n", __FUNCTION__,
        stCropInfo_1.u16In_hsize,stCropInfo_1.u16In_vsize,stCropInfo_1.u16Hsize,stCropInfo_1.u16Vsize);
}

EN_HVSP_IP_MUX_TYPE Drv_HVSP_GetInputSrcMux(void)
{
    return Hal_HVSP_GetInputSrcMux();
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
void Drv_HVSP_SetSCIQVSRAM(EN_HVSP_ID_TYPE enHVSP_ID)
{
    SCL_DBG(SCL_DBG_LV_DRVHVSP()&(Get_DBGMG_HVSP(enHVSP_ID)),
        "[DRVHVSP]Doing V Qmap SRAM id:%d scaling UP:%hhx\n",enHVSP_ID,gbVScalingup[enHVSP_ID]);
    Hal_HVSP_SRAM_Dump((EN_HVSP_SRAM_DUMP_TYPE)(enHVSP_ID+HVSP_ID_SRAM_V_OFFSET),gbVScalingup[enHVSP_ID]); //level 1 :up 0:down
}
void Drv_HVSP_SetSCIQHSRAM(EN_HVSP_ID_TYPE enHVSP_ID)
{
    SCL_DBG(SCL_DBG_LV_DRVHVSP()&(Get_DBGMG_HVSP(enHVSP_ID)),
        "[DRVHVSP]Doing H Qmap SRAM id:%d scaling UP:%hhx\n",enHVSP_ID,gbHScalingup[enHVSP_ID]);
    Hal_HVSP_SRAM_Dump((EN_HVSP_SRAM_DUMP_TYPE)(enHVSP_ID+HVSP_ID_SRAM_H_OFFSET),gbHScalingup[enHVSP_ID]); //level 1 :up 0:down
}
void Drv_HVSP_SetOSDConfig(EN_HVSP_ID_TYPE enID, ST_DRV_HVSP_OSD_CONFIG *stOSdCfg)
{
    Hal_HVSP_SetOSDLocate(enID,stOSdCfg->enOSD_loc);
    Hal_HVSP_SetOSDOnOff(enID,stOSdCfg->stOsdOnOff.bOSDEn);
    Hal_HVSP_SetOSDbypass(enID,stOSdCfg->stOsdOnOff.bOSDBypass);
    Hal_HVSP_SetOSDbypassWTM(enID,stOSdCfg->stOsdOnOff.bWTMBypass);
}
void _Drv_HVSP_SetPriMaskBufferByByte(unsigned char * pu8Addr,unsigned char u8MaskVal)
{
   *pu8Addr =  u8MaskVal;
}
void _Drv_HVSP_SetPriMaskBufferByBit(unsigned char * pu8Addr,unsigned char u8MaskVal,unsigned char mask)
{
    unsigned char u8val;
    u8val = *pu8Addr;
   *pu8Addr =  ((u8val&mask)|u8MaskVal);
}
unsigned char _Drv_HVSP_GetPriMaskBufferByByte(unsigned char * pu8Addr)
{
   unsigned char u8val;
   u8val = *pu8Addr;
   return u8val;
}
unsigned char * _Drv_HVSP_GetPriMaskStartPoint(unsigned short u16X,unsigned short u16Y)
{
    unsigned char *pu8StartPoint;
    unsigned short u16newX;
    unsigned short u16newY;
    u16newX = (u16X/HVSP_PRIMASK_BLOCK_SIZE); // bit/shift,ex.300 ->300/32 = 9.xx ->bit9 start(9th end is 288 10th start is 289)
    u16newY = (u16Y/HVSP_PRIMASK_BLOCK_SIZE); // bit/shift (line)
    pu8StartPoint = gpu8PriMaskBuf + (HVSP_PRIMASK_Width_SIZE*(u16newY))+(u16newX/BYTESIZE);
    SCL_DBG(SCL_DBG_LV_DRVHVSP()&(Get_DBGMG_HVSP(0)), "[DRVHVSP]%s u16newX = %hd,u16newY = %hd,pu8StartPoint = %lx\n"
        ,__FUNCTION__, u16newX,u16newY,(unsigned long)pu8StartPoint);
    return pu8StartPoint;
}
unsigned short _Drv_HVSP_GetPriMaskOffsetbit(unsigned short u16P)
{
    unsigned short u16Offsetbit;
    unsigned short u16Xbyte;
    u16Offsetbit = u16P/HVSP_PRIMASK_BLOCK_SIZE; // bit/shift,ex.300 ->300/32 = 9.xx ->bit9 start(9th end is 288 10th start is 289)
    u16Xbyte = u16Offsetbit/BYTESIZE;
    u16Offsetbit = u16Offsetbit - (u16Xbyte*BYTESIZE);
    SCL_DBG(SCL_DBG_LV_DRVHVSP()&(Get_DBGMG_HVSP(0)), "[DRVHVSP]%s offset = %hd\n",__FUNCTION__, u16Offsetbit);
    return u16Offsetbit;
}
unsigned short _Drv_HVSP_GetPriMaskXOffset
    (unsigned short u16P,unsigned short u16Leng)
{
    unsigned short u16Offeset;
    unsigned short u16Offsetnow;
    unsigned short u16lastOffeset;
    unsigned short u16XOffset=0;
    u16Offeset = u16Leng;//256
    u16Offsetnow = (u16P/HVSP_PRIMASK_BLOCK_SIZE);// 4
    u16Offsetnow = u16Offsetnow/BYTESIZE;// 0
    u16lastOffeset = ((u16Offeset)/HVSP_PRIMASK_BLOCK_SIZE);// 8
    u16lastOffeset = u16lastOffeset/BYTESIZE;// 1
    if(u16Offeset%(BYTESIZE*HVSP_PRIMASK_BLOCK_SIZE))
    {
        u16XOffset = u16lastOffeset-u16Offsetnow+1;// 2
    }
    else
    {
        u16XOffset = u16lastOffeset-u16Offsetnow;// 1 full byte
    }
    SCL_DBG(SCL_DBG_LV_DRVHVSP()&(Get_DBGMG_HVSP(0)), "[DRVHVSP]%s offset = %hd\n",__FUNCTION__, u16XOffset);
    return u16XOffset;
}
unsigned short _Drv_HVSP_GetPriMaskOffset(unsigned short u16Leng)
{
    unsigned short u16newOffeset;
    u16newOffeset = u16Leng/HVSP_PRIMASK_BLOCK_SIZE;
    if(((u16Leng)%HVSP_PRIMASK_BLOCK_SIZE)>(HVSP_PRIMASK_BLOCK_SIZE/2))
    {
        u16newOffeset+=1;
    }
    SCL_DBG(SCL_DBG_LV_DRVHVSP()&(Get_DBGMG_HVSP(0)), "[DRVHVSP]%s offset = %hd\n",__FUNCTION__, u16newOffeset);
    return u16newOffeset;
}
void Drv_HVSP_SetPriMaskConfig(ST_HVSP_PRIMASK_CONFIG *stCfg)
{
    unsigned char *pu8StartPoint;
    unsigned short u16X;//x
    unsigned short u16Y;//x
    unsigned short u16Xend;//x+width
    unsigned short u16XStartoffsetbit;//first byte
    unsigned short u16XEndoffsetbit;//last byte
    unsigned short u16Xoffset;
    unsigned short u16Yoffset;
    unsigned short u16idx;
    unsigned short u16idy;
    unsigned char u8mask;
    unsigned char u8orimask;
    u16X = ((stCfg->u16X/HVSP_PRIMASK_BLOCK_SIZE)*HVSP_PRIMASK_BLOCK_SIZE);
    if((stCfg->u16X%HVSP_PRIMASK_BLOCK_SIZE)>(HVSP_PRIMASK_BLOCK_SIZE/2))
    {
        u16X+=HVSP_PRIMASK_BLOCK_SIZE;
    }
    u16Y = ((stCfg->u16Y/HVSP_PRIMASK_BLOCK_SIZE)*HVSP_PRIMASK_BLOCK_SIZE);
    if((stCfg->u16Y%HVSP_PRIMASK_BLOCK_SIZE)>(HVSP_PRIMASK_BLOCK_SIZE/2))
    {
        u16Y+=HVSP_PRIMASK_BLOCK_SIZE;
    }
    u16Xend = (((stCfg->u16X+stCfg->u16Width)/HVSP_PRIMASK_BLOCK_SIZE)*HVSP_PRIMASK_BLOCK_SIZE);
    if(((stCfg->u16X+stCfg->u16Width)%HVSP_PRIMASK_BLOCK_SIZE)>(HVSP_PRIMASK_BLOCK_SIZE/2))
    {
        u16Xend+=HVSP_PRIMASK_BLOCK_SIZE;
    }
    pu8StartPoint = _Drv_HVSP_GetPriMaskStartPoint(u16X,u16Y);//byte/unit
    u16XStartoffsetbit = _Drv_HVSP_GetPriMaskOffsetbit(u16X);// bit/unit
    u16XEndoffsetbit = _Drv_HVSP_GetPriMaskOffsetbit(u16Xend);// bit/unit
    if(u16XEndoffsetbit)
    {
        u16XEndoffsetbit = BYTESIZE-u16XEndoffsetbit;
    }
    u16Xoffset = _Drv_HVSP_GetPriMaskXOffset(u16X,u16Xend);//byte/unit
    u16Yoffset = _Drv_HVSP_GetPriMaskOffset(stCfg->u16Height);// bit/unit
    for(u16idy = 0;u16idy<u16Yoffset;u16idy++)
    {
        for(u16idx = 0;u16idx<(u16Xoffset);u16idx++)
        {
            u8mask = (stCfg->bMask) ? 0xFF :0;
            if(Is_StartAndEndInSameByte(u16XStartoffsetbit,u16idx,u16Xoffset))
            {
                u8orimask = _Drv_HVSP_GetPriMaskBufferByByte(pu8StartPoint+(u16idy*HVSP_PRIMASK_Width_SIZE));
                if(stCfg->bMask)
                {
                    u8mask = u8orimask | ((0xFF>>u16XEndoffsetbit)&(0xFF<<u16XStartoffsetbit));
                }
                else
                {
                    u8mask = (u8orimask&(~((0xFF>>u16XEndoffsetbit)&(0xFF<<u16XStartoffsetbit))));
                }
            }
            else if(Is_StartByteOffsetBit(u16XStartoffsetbit,u16idx))
            {
                u8orimask = _Drv_HVSP_GetPriMaskBufferByByte(pu8StartPoint+(u16idy*HVSP_PRIMASK_Width_SIZE));
                if(stCfg->bMask)
                {
                    u8mask = u8orimask | (u8mask&(0xFF<<u16XStartoffsetbit));
                }
                else
                {
                    u8mask = (u8orimask&(~(0xFF<<u16XStartoffsetbit)));
                }
            }
            else if(Is_EndByteOffsetBit(u16XEndoffsetbit,u16Xoffset,u16idx))
            {
                u8orimask = _Drv_HVSP_GetPriMaskBufferByByte(pu8StartPoint+u16idx+(u16idy*HVSP_PRIMASK_Width_SIZE));
                if(stCfg->bMask)
                {
                    u8mask = u8orimask | (u8mask&(0xFF>>(u16XEndoffsetbit)));
                }
                else
                {
                    u8mask = u8orimask &(~(0xFF>>(u16XEndoffsetbit)));
                }
            }
            _Drv_HVSP_SetPriMaskBufferByByte(pu8StartPoint+u16idx+(u16idy*HVSP_PRIMASK_Width_SIZE),u8mask);
        }
    }
}
unsigned short Drv_HVSP_PriMaskGetSRAM(unsigned char u8idx, unsigned char u8idy)
{
    unsigned short u16val = 0;
    unsigned char* p8valpoint;
    p8valpoint = (unsigned char*)gpu8PriMaskBuf;
    u16val = 0;
    if(u8idx==(HVSP_PRIMASK_Width_SIZE/2))
    {
        u16val = (*(p8valpoint+(u8idx*2)+(u8idy*HVSP_PRIMASK_Width_SIZE)));
    }
    else
    {
        u16val = (*(p8valpoint+(u8idx*2)+(u8idy*HVSP_PRIMASK_Width_SIZE))|
            (*(p8valpoint+(u8idx*2)+1+(u8idy*HVSP_PRIMASK_Width_SIZE)))<<8);
    }
    return u16val;
}
void _Drv_HVSP_PriMaskFillSRAM(void)
{
    unsigned char u8idx = 0;
    unsigned char u8idy = 0;
    unsigned short u16val = 0;
    unsigned char* p8valpoint;
    p8valpoint = (unsigned char*)gpu8PriMaskBuf;
    _Drv_HVSP_PriMaskInit();
    DRV_HVSP_MUTEX_LOCK();
    Drv_CMDQ_GetModuleMutex(EN_CMDQ_TYPE_IP0,1);
    for(u8idy = 0;u8idy<HVSP_PRIMASK_Height_SIZE;u8idy++)
    {
        //Hal_HVSP_PriMask_SRAMWriteIdx(u8idy);
        SCL_DBG(SCL_DBG_LV_DRVHVSP()&(Get_DBGMG_HVSP(0)), "[DRVHVSP]%s",__FUNCTION__);
        for(u8idx =0;u8idx<=(HVSP_PRIMASK_Width_SIZE/2);u8idx++)
        {
            u16val = 0;
            if(u8idx==(HVSP_PRIMASK_Width_SIZE/2))
            {
                u16val = (*(p8valpoint+(u8idx*2)+(u8idy*HVSP_PRIMASK_Width_SIZE)));
            }
            else
            {
                u16val = (*(p8valpoint+(u8idx*2)+(u8idy*HVSP_PRIMASK_Width_SIZE))|
                    (*(p8valpoint+(u8idx*2)+1+(u8idy*HVSP_PRIMASK_Width_SIZE)))<<8);
            }
            Hal_HVSP_PriMaskSetSRAM(u8idx,u16val);
            SCL_DBG(SCL_DBG_LV_DRVHVSP()&(Get_DBGMG_HVSP(0)), " val:%hx,",u16val);
        }
        SCL_DBG(SCL_DBG_LV_DRVHVSP()&(Get_DBGMG_HVSP(0)), "idy:%hhd\n",u8idy);
        Hal_HVSP_PriMask_SRAMEn(1,u8idy);
    }
    Hal_HVSP_PriMask_Fire();
    Drv_CMDQ_GetModuleMutex(EN_CMDQ_TYPE_IP0,0);
    DRV_HVSP_MUTEX_UNLOCK();
}
void Drv_HVSP_SetPriMaskTrigger(EN_HVSP_PRIMASK_TYPE enType)
{
    //Fill SRAM
    //clk open
    MS_U16 u16clkreg;
    u16clkreg = Hal_HVSP_Get_2ByteReg(REG_SCL_CLK_64_L);
    Hal_HVSP_Set_2ByteReg(REG_SCL_CLK_64_L,0x0,0xFFFF);
    if(enType == EN_HVSP_PRIMASK_DISABLE)
    {
        Drv_HVSP_PriMaskBufferReset();
    }
    _Drv_HVSP_PriMaskFillSRAM();
    //TRIGGER
    if(enType == EN_HVSP_PRIMASK_DISABLE || enType == EN_HVSP_PRIMASK_ONLYHWOFF)
    {
        Hal_HVSP_PriMask_bEn(EN_HVSP_TRIGGER_OFF);
    }
    else if(enType == EN_HVSP_PRIMASK_ENABLE)
    {
        Hal_HVSP_PriMask_bEn(EN_HVSP_TRIGGER_ON);
    }
    else if (enType == EN_HVSP_PRIMASK_PENDING)
    {
        gbPriMaskPending = 1;
    }
    //clk close
    Hal_HVSP_Set_2ByteReg(REG_SCL_CLK_64_L,u16clkreg,0xFFFF);
}
void Drv_HVSP_SetPrv2CropOnOff(unsigned char bEn)
{
    Hal_HVSP_SetPrv2CropOnOff(bEn);
}
void Drv_HVSP_GetSCLInts(ST_HVSP_SCINTS_TYPE *sthvspints)
{
    ST_SCLIRQ_SCINTS_TYPE *stints;
    stints = Drv_SCLIRQ_GetSCLInts();
    MsOS_Memcpy(sthvspints,stints,sizeof(ST_HVSP_SCINTS_TYPE));
}
void Drv_HVSP_SCLIQ(EN_HVSP_ID_TYPE enID,EN_DRV_HVSP_IQ_TYPE enIQ)
{
    ST_HVSP_CMD_TRIG_CONFIG stCmdTrigCfg;
    MsOS_Memset(&stCmdTrigCfg,0,sizeof(ST_HVSP_CMD_TRIG_CONFIG));
    switch(enIQ)
    {
        case EN_DRV_HVSP_IQ_H_Tbl0:
            DRV_HVSP_MUTEX_LOCK();
            Drv_HVSP_SetCMDQTrigTypeByRIU(&stCmdTrigCfg);
            Hal_HVSP_SetModeYHo(enID,E_HVSP_FILTER_MODE_SRAM_0);
            Drv_HVSP_SetCMDQTrigType(&stCmdTrigCfg);
            DRV_HVSP_MUTEX_UNLOCK();
            Hal_HVSP_SRAM_Dump((EN_HVSP_SRAM_DUMP_TYPE)(enID+HVSP_ID_SRAM_H_OFFSET),gbHScalingup[enID]&0x1); //level 1 :up 0:down
            gbHScalingup[enID] = gbHScalingup[enID]&0x1;
        break;
        case EN_DRV_HVSP_IQ_H_Tbl1:
            DRV_HVSP_MUTEX_LOCK();
            Drv_HVSP_SetCMDQTrigTypeByRIU(&stCmdTrigCfg);
            Hal_HVSP_SetModeYHo(enID,E_HVSP_FILTER_MODE_SRAM_0);
            Drv_HVSP_SetCMDQTrigType(&stCmdTrigCfg);
            DRV_HVSP_MUTEX_UNLOCK();
            Hal_HVSP_SRAM_Dump((EN_HVSP_SRAM_DUMP_TYPE)(enID+HVSP_ID_SRAM_H_OFFSET),(0x10 |(gbHScalingup[enID]&0x1))); //level 1 :up 0:down
            gbHScalingup[enID] = (0x10 |(gbHScalingup[enID]&0x1));
        break;
        case EN_DRV_HVSP_IQ_H_Tbl2:
            DRV_HVSP_MUTEX_LOCK();
            Drv_HVSP_SetCMDQTrigTypeByRIU(&stCmdTrigCfg);
            Hal_HVSP_SetModeYHo(enID,E_HVSP_FILTER_MODE_SRAM_0);
            Drv_HVSP_SetCMDQTrigType(&stCmdTrigCfg);
            DRV_HVSP_MUTEX_UNLOCK();
            Hal_HVSP_SRAM_Dump((EN_HVSP_SRAM_DUMP_TYPE)(enID+HVSP_ID_SRAM_H_OFFSET),(0x20 |(gbHScalingup[enID]&0x1))); //level 1 :up 0:down
            gbHScalingup[enID] = (0x20 |(gbHScalingup[enID]&0x1));
        break;
        case EN_DRV_HVSP_IQ_H_Tbl3:
            DRV_HVSP_MUTEX_LOCK();
            Drv_HVSP_SetCMDQTrigTypeByRIU(&stCmdTrigCfg);
            Hal_HVSP_SetModeYHo(enID,E_HVSP_FILTER_MODE_SRAM_0);
            Drv_HVSP_SetCMDQTrigType(&stCmdTrigCfg);
            DRV_HVSP_MUTEX_UNLOCK();
            Hal_HVSP_SRAM_Dump((EN_HVSP_SRAM_DUMP_TYPE)(enID+HVSP_ID_SRAM_H_OFFSET),(0x30 |(gbHScalingup[enID]&0x1))); //level 1 :up 0:down
            gbHScalingup[enID] = (0x30 |(gbHScalingup[enID]&0x1));
        break;
        case EN_DRV_HVSP_IQ_H_BYPASS:
            DRV_HVSP_MUTEX_LOCK();
            Drv_HVSP_SetCMDQTrigTypeByRIU(&stCmdTrigCfg);
            Hal_HVSP_SetModeYHo(enID,E_HVSP_FILTER_MODE_BYPASS);
            Drv_HVSP_SetCMDQTrigType(&stCmdTrigCfg);
            DRV_HVSP_MUTEX_UNLOCK();
        break;
        case EN_DRV_HVSP_IQ_H_BILINEAR:
            DRV_HVSP_MUTEX_LOCK();
            Drv_HVSP_SetCMDQTrigTypeByRIU(&stCmdTrigCfg);
            Hal_HVSP_SetModeYHo(enID,E_HVSP_FILTER_MODE_BILINEAR);
            Drv_HVSP_SetCMDQTrigType(&stCmdTrigCfg);
            DRV_HVSP_MUTEX_UNLOCK();
        break;
        case EN_DRV_HVSP_IQ_V_Tbl0:
            DRV_HVSP_MUTEX_LOCK();
            Drv_HVSP_SetCMDQTrigTypeByRIU(&stCmdTrigCfg);
            Hal_HVSP_SetModeYVe(enID,E_HVSP_FILTER_MODE_SRAM_0);
            Drv_HVSP_SetCMDQTrigType(&stCmdTrigCfg);
            DRV_HVSP_MUTEX_UNLOCK();
            Hal_HVSP_SRAM_Dump((EN_HVSP_SRAM_DUMP_TYPE)(enID+HVSP_ID_SRAM_V_OFFSET),gbVScalingup[enID]&0x1); //level 1 :up 0:down
            gbVScalingup[enID] = gbVScalingup[enID]&0x1;
        break;
        case EN_DRV_HVSP_IQ_V_Tbl1:
            DRV_HVSP_MUTEX_LOCK();
            Drv_HVSP_SetCMDQTrigTypeByRIU(&stCmdTrigCfg);
            Hal_HVSP_SetModeYVe(enID,E_HVSP_FILTER_MODE_SRAM_0);
            Drv_HVSP_SetCMDQTrigType(&stCmdTrigCfg);
            DRV_HVSP_MUTEX_UNLOCK();
            Hal_HVSP_SRAM_Dump((EN_HVSP_SRAM_DUMP_TYPE)(enID+HVSP_ID_SRAM_V_OFFSET),(0x10 |(gbVScalingup[enID]&0x1))); //level 1 :up 0:down
            gbVScalingup[enID] = (0x10 |(gbVScalingup[enID]&0x1));
        break;
        case EN_DRV_HVSP_IQ_V_Tbl2:
            DRV_HVSP_MUTEX_LOCK();
            Drv_HVSP_SetCMDQTrigTypeByRIU(&stCmdTrigCfg);
            Hal_HVSP_SetModeYVe(enID,E_HVSP_FILTER_MODE_SRAM_0);
            Drv_HVSP_SetCMDQTrigType(&stCmdTrigCfg);
            DRV_HVSP_MUTEX_UNLOCK();
            Hal_HVSP_SRAM_Dump((EN_HVSP_SRAM_DUMP_TYPE)(enID+HVSP_ID_SRAM_V_OFFSET),(0x20 |(gbVScalingup[enID]&0x1))); //level 1 :up 0:down
            gbVScalingup[enID] = (0x20 |(gbVScalingup[enID]&0x1));
        break;
        case EN_DRV_HVSP_IQ_V_Tbl3:
            DRV_HVSP_MUTEX_LOCK();
            Drv_HVSP_SetCMDQTrigTypeByRIU(&stCmdTrigCfg);
            Hal_HVSP_SetModeYVe(enID,E_HVSP_FILTER_MODE_SRAM_0);
            Drv_HVSP_SetCMDQTrigType(&stCmdTrigCfg);
            DRV_HVSP_MUTEX_UNLOCK();
            Hal_HVSP_SRAM_Dump((EN_HVSP_SRAM_DUMP_TYPE)(enID+HVSP_ID_SRAM_V_OFFSET),(0x30 |(gbVScalingup[enID]&0x1))); //level 1 :up 0:down
            gbVScalingup[enID] = (0x30 |(gbVScalingup[enID]&0x1));
        break;
        case EN_DRV_HVSP_IQ_V_BYPASS:
            DRV_HVSP_MUTEX_LOCK();
            Drv_HVSP_SetCMDQTrigTypeByRIU(&stCmdTrigCfg);
            Hal_HVSP_SetModeYVe(enID,E_HVSP_FILTER_MODE_BYPASS);
            Drv_HVSP_SetCMDQTrigType(&stCmdTrigCfg);
            DRV_HVSP_MUTEX_UNLOCK();
        break;
        case EN_DRV_HVSP_IQ_V_BILINEAR:
            DRV_HVSP_MUTEX_LOCK();
            Drv_HVSP_SetCMDQTrigTypeByRIU(&stCmdTrigCfg);
            Hal_HVSP_SetModeYVe(enID,E_HVSP_FILTER_MODE_BILINEAR);
            Drv_HVSP_SetCMDQTrigType(&stCmdTrigCfg);
            DRV_HVSP_MUTEX_UNLOCK();
        break;
        default:
        break;
    }
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
    stInformCfg->bEn |= (gbVScalingup[enID]&0xF0);
    stInformCfg->bEn |= ((gbHScalingup[enID]&0xF0)<<2);
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
    MS_BOOL bLDCorPrvCrop;
    stInformCfg->bYCMWrite = gstIPMCfg.bYCMWrite;
    stInformCfg->u16Fetch = gstIPMCfg.u16Fetch;
    stInformCfg->u16Vsize = gstIPMCfg.u16Vsize;
    stInformCfg->u32YCBaseAddr = gstIPMCfg.u32YCBaseAddr+0x20000000;
    stInformCfg->u32MemSize = gstIPMCfg.u32MemSize;
    stInformCfg->bYCMRead = gstIPMCfg.bYCMRead;
    bLDCorPrvCrop = (Hal_HVSP_GetLDCPathSel()) ? 1 :
                    (Hal_HVSP_GetPrv2CropOnOff())? 2 : 0;
    return bLDCorPrvCrop;
}
MS_BOOL Drv_HVSP_SetLDCFrameBuffer_Config(ST_HVSP_LDC_FRAMEBUFFER_CONFIG *stLDCCfg)
{
    ST_HVSP_CMD_TRIG_CONFIG stCmdTrigCfg;
    MsOS_Memset(&stCmdTrigCfg,0,sizeof(ST_HVSP_CMD_TRIG_CONFIG));
    SCL_DBG(SCL_DBG_LV_DRVHVSP()&(Get_DBGMG_HVSP(0)), "[DRVHVSP]%s(%d): Width=%hx,Height:%x\n", __FUNCTION__,
        __LINE__, stLDCCfg->u16Width,stLDCCfg->u16Height);
    DRV_HVSP_MUTEX_LOCK();
    Drv_HVSP_SetCMDQTrigTypeByRIU(&stCmdTrigCfg);
    Hal_HVSP_SetLDCWidth(stLDCCfg->u16Width);
    Hal_HVSP_SetLDCHeight(stLDCCfg->u16Height);
    Hal_HVSP_SetLDCBase(stLDCCfg->u32FBaddr);
    Hal_HVSP_SetLDCHWrwDiff(((MS_U16)stLDCCfg->u8FBrwdiff));
    Hal_HVSP_SetLDCSW_Idx(stLDCCfg->u8FBidx);
    Hal_HVSP_SetLDCSW_Mode(stLDCCfg->bEnSWMode);
    Hal_HVSP_SetLDCBank_Mode(stLDCCfg->enLDCType);
    Hal_HVSP_SetLDCPathSel(stLDCCfg->bEnDNR);
    if(!gbLDCEn)
    {
        Hal_HVSP_SetLDCBypass(1);
    }
    Drv_HVSP_SetCMDQTrigType(&stCmdTrigCfg);
    DRV_HVSP_MUTEX_UNLOCK();
    return TRUE;
}
void Drv_HVSP_SetLDCONOFF(MS_BOOL bEn)
{
    unsigned long u32Events;
    static MS_BOOL bEnPrv2Crop = 0;
    //ToDo
    if(Hal_HVSP_GetLDCPathSel()!= bEn)
    {
        if(!Drv_SCLIRQ_GetIsBlankingRegion())
        {
            MsOS_WaitEvent(Drv_SCLIRQ_Get_IRQ_SYNCEventID(), E_SCLIRQ_EVENT_FRMENDSYNC, &u32Events, E_OR, 1000); // get status: FRM END
        }
        if(Hal_HVSP_GetPrv2CropOnOff() == bEn)
        {
            if(bEnPrv2Crop &&(!bEn))
            {
                Drv_HVSP_SetPrv2CropOnOff(1);
            }
            else
            {
                Drv_HVSP_SetPrv2CropOnOff(0);
            }
            bEnPrv2Crop = bEn;
        }
        Hal_HVSP_SetLDCPathSel(bEn);
    }
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
                Hal_HVSP_SetPatTgVtt(1800); //scaling up need bigger Vtt, , using vtt of 1920x1080 for all timing
            }
            else
            {
                Hal_HVSP_SetPatTgVtt(2500); //rotate
            }
            Hal_HVSP_SetPatTgVsyncSt(u16VSync_St);
            Hal_HVSP_SetPatTgVsyncEnd(u16VSync_St + pCfg->u16VSyncWidth - 1);
            Hal_HVSP_SetPatTgVdeSt(u16VSync_St + pCfg->u16VSyncWidth + pCfg->u16VBackPorch);
            Hal_HVSP_SetPatTgVdeEnd(u16VSync_St + pCfg->u16VSyncWidth + pCfg->u16VBackPorch + pCfg->u16VActive - 1);
            Hal_HVSP_SetPatTgVfdeSt(u16VSync_St + pCfg->u16VSyncWidth + pCfg->u16VBackPorch);
            Hal_HVSP_SetPatTgVfdeEnd(u16VSync_St + pCfg->u16VSyncWidth + pCfg->u16VBackPorch + pCfg->u16VActive - 1);

            Hal_HVSP_SetPatTgHtt(2750); // scaling up need bigger Vtt, , using vtt of 1920x1080 for all timing
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
    stInformCfg->bEn = Hal_HVSP_GetCrop1En();
    stInformCfg->u16inWidth         = Hal_HVSP_GetCrop1Width();
    stInformCfg->u16inHeight        = Hal_HVSP_GetCrop1Height();
    stInformCfg->u16inCropWidth         = Hal_HVSP_GetCrop2InputWidth();
    stInformCfg->u16inCropHeight        = Hal_HVSP_GetCrop2InputHeight();
    stInformCfg->u16inCropX         = Hal_HVSP_Get_Crop_X();
    stInformCfg->u16inCropY        = Hal_HVSP_Get_Crop_Y();
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
