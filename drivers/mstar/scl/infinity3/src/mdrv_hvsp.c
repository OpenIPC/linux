////////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2006-2011 MStar Semiconductor, Inc.
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
#define _MDRV_HVSP_C

//#include <linux/module.h>
//#include <linux/kernel.h>
//#include <linux/delay.h>
//#include <linux/slab.h>
//#include <asm/uaccess.h>
//#include <linux/clk.h>
//#include <linux/clk-provider.h>
#include "MsCommon.h"
#include "MsTypes.h"
#include "MsOS.h"
#include "drvhvsp_st.h"
#include "drvhvsp.h"

//#include "mdrv_hvsp_io_i3_st.h"
#include "mdrv_scl_dbg.h"
#include "mdrv_hvsp.h"
#include "mdrv_multiinst.h"
#include "drvpnl.h"
//-------------------------------------------------------------------------------------------------
//  Defines & Macro
//-------------------------------------------------------------------------------------------------
#define PARSING_PAT_TGEN_TIMING(x)  (x == E_MDRV_HVSP_PAT_TGEN_TIMING_1920_1080_30  ?  "E_MDRV_HVSP_PAT_TGEN_TIMING_1920_1080_30" : \
                                     x == E_MDRV_HVSP_PAT_TGEN_TIMING_1024_768_60   ?  "E_MDRV_HVSP_PAT_TGEN_TIMING_1024_768_60" : \
                                     x == E_MDRV_HVSP_PAT_TGEN_TIMING_640_480_60    ?  "E_MDRV_HVSP_PAT_TGEN_TIMING_640_480_60" : \
                                     x == E_MDRV_HVSP_PAT_TGEN_TIMING_UNDEFINED     ?  "E_MDRV_HVSP_PAT_TGEN_TIMING_UNDEFINED" : \
                                                                                        "UNKNOWN")
#define Is_Wrong_Type(enHVSP_ID,enSrcType)((enHVSP_ID == E_MDRV_HVSP_ID_1 && (enSrcType == E_MDRV_HVSP_SRC_DRAM)) ||\
                        (enHVSP_ID == E_MDRV_HVSP_ID_2 && enSrcType != E_MDRV_HVSP_SRC_HVSP) ||\
                        (enHVSP_ID == E_MDRV_HVSP_ID_3 && enSrcType != E_MDRV_HVSP_SRC_DRAM))
#define Is_PTGEN_FHD(u16Htotal,u16Vtotal,u16Vfrequency) ((u16Htotal) == 2200 && (u16Vtotal) == 1125 && (u16Vfrequency) == 30)
#define Is_PTGEN_HD(u16Htotal,u16Vtotal,u16Vfrequency) ((u16Htotal) == 1344 && (u16Vtotal) == 806 && (u16Vfrequency) == 60)
#define Is_PTGEN_SD(u16Htotal,u16Vtotal,u16Vfrequency) ((u16Htotal) == 800 && (u16Vtotal) == 525 && (u16Vfrequency) == 60)
#define IS_HVSPNotOpen(u16Src_Width,u16Dsp_Width) (u16Src_Width == 0 && u16Dsp_Width == 0)
#define IS_NotScalingAfterCrop(bEn,u16Dsp_Height,u16Height,u16Dsp_Width,u16Width) (bEn && \
                        (u16Dsp_Height == u16Height) && (u16Dsp_Width== u16Width))
#define RATIO_CONFIG 512
#define CAL_HVSP_RATIO(input,output) ((unsigned long)((unsigned long long)((unsigned long)input * RATIO_CONFIG )/(unsigned long)output))
#define SCALE(numerator, denominator,value) ((unsigned short)((unsigned long)(value * RATIO_CONFIG *  numerator) / (denominator * RATIO_CONFIG)))

#define PriMaskPercentBase 10000
#define MDRV_HVSP_MUTEX_LOCK()            MsOS_ObtainMutex(_MHVSP_Mutex,MSOS_WAIT_FOREVER)
#define MDRV_HVSP_MUTEX_UNLOCK()          MsOS_ReleaseMutex(_MHVSP_Mutex)
//-------------------------------------------------------------------------------------------------
//  Variable
//-------------------------------------------------------------------------------------------------
ST_MDRV_HVSP_SCALING_CONFIG gstHvspScalingCfg;
ST_MDRV_HVSP_POSTCROP_CONFIG gstHvspPostCropCfg;
ST_MDRV_HVSP_PRIMASK_CONFIG gstHvspPriMaskCfg[MDRV_MULTI_INST_PRIVATE_ID_NUM][PriMaskNum];
unsigned char gu8PriMaskNum[MDRV_MULTI_INST_PRIVATE_ID_NUM];
unsigned char gu8PriMaskInst;
/////////////////
/// gbMemReadyForMdrv
/// if True ,DNR buffer was allocated.
////////////////
unsigned char gbMemReadyForMdrv;
unsigned char gu8IPMBufferNum;
ST_DRV_HVSP_PAT_TGEN_CONFIG gstPatTgenCfg[E_MDRV_HVSP_PAT_TGEN_TIMING_MAX] =
{
    {1125,  4,  5, 36, 1080, 2200, 88,  44, 148, 1920}, // 1920_1080_30
    { 806,  3,  6, 29,  768, 1344, 24, 136, 160, 1024}, // 1024_768_60
    { 525, 33,  2, 10,  480,  800, 16,  96,  48,  640}, // 640_480_60
    {   0, 20, 10, 20,    0,    0, 30,  15,  30,    0}, // undefined
};
MS_S32 _MHVSP_Mutex = -1;

//-------------------------------------------------------------------------------------------------
//  Function
//-------------------------------------------------------------------------------------------------
unsigned char MDrv_HVSP_GetCMDQDoneStatus(EN_MDRV_HVSP_ID_TYPE enHVSP_ID)
{
    EN_HVSP_POLL_ID_TYPE enPollId;
    enPollId = enHVSP_ID == E_MDRV_HVSP_ID_2 ? E_HVSP_POLL_ID_2 :
           enHVSP_ID == E_MDRV_HVSP_ID_3 ? E_HVSP_POLL_ID_3 :
                                           E_HVSP_POLL_ID_1;
    return Drv_HVSP_GetCMDQDoneStatus(enPollId);
}

void * MDrv_HVSP_GetWaitQueueHead(void)
{
    return Drv_HVSP_GetWaitQueueHead();
}
void MDrv_HVSP_SetPollWait
    (void *filp, void *pWaitQueueHead, void *pstPollQueue)
{
    MsOS_SetPollWait(filp, pWaitQueueHead, pstPollQueue);
}
unsigned char MDrv_HVSP_Suspend(EN_MDRV_HVSP_ID_TYPE enHVSP_ID, ST_MDRV_HVSP_SUSPEND_RESUME_CONFIG *pCfg)
{
    ST_HVSP_SUSPEND_RESUME_CONFIG stSuspendResumeCfg;
    unsigned char bRet = TRUE;;
    MsOS_Memset(&stSuspendResumeCfg,0,sizeof(ST_HVSP_SUSPEND_RESUME_CONFIG));
    stSuspendResumeCfg.u32IRQNUM = pCfg->u32IRQNum;
    stSuspendResumeCfg.u32CMDQIRQNUM = pCfg->u32CMDQIRQNum;
    if(Drv_HVSP_Suspend(&stSuspendResumeCfg))
    {
        bRet = TRUE;
    }
    else
    {
        SCL_DBG(SCL_DBG_LV_HVSP()&(Get_DBGMG_HVSP(enHVSP_ID)), "[HVSP]%s Suspend Fail\n", __FUNCTION__);
        bRet = FALSE;
    }

    return bRet;
}

unsigned char MDrv_HVSP_Resume(EN_MDRV_HVSP_ID_TYPE enHVSP_ID, ST_MDRV_HVSP_SUSPEND_RESUME_CONFIG *pCfg)
{
    ST_HVSP_SUSPEND_RESUME_CONFIG stSuspendResumeCfg;
    unsigned char bRet = TRUE;;
    MsOS_Memset(&stSuspendResumeCfg,0,sizeof(ST_HVSP_SUSPEND_RESUME_CONFIG));
    stSuspendResumeCfg.u32IRQNUM = pCfg->u32IRQNum;
    stSuspendResumeCfg.u32CMDQIRQNUM = pCfg->u32CMDQIRQNum;
    if(Drv_HVSP_Resume(&stSuspendResumeCfg))
    {
        bRet = TRUE;
    }
    else
    {
        SCL_DBG(SCL_DBG_LV_HVSP()&(Get_DBGMG_HVSP(enHVSP_ID)), "[HVSP]%s Resume Fail\n", __FUNCTION__);
        bRet = FALSE;
    }

    return bRet;
}
void _MDrv_HVSP_ResetPriMaskStorage(void)
{
    MDRV_HVSP_MUTEX_LOCK();
    MsOS_Memset(&gstHvspPriMaskCfg[gu8PriMaskInst], 0, sizeof(ST_MDRV_HVSP_PRIMASK_CONFIG)*PriMaskNum);
    gu8PriMaskNum[gu8PriMaskInst] = 0;
    MDRV_HVSP_MUTEX_UNLOCK();
}
void _MDrv_HVSP_SWinit(void)
{
    unsigned char i;
    MDRV_HVSP_MUTEX_LOCK();
    MsOS_Memset(&gstHvspScalingCfg, 0, sizeof(ST_MDRV_HVSP_SCALING_CONFIG));
    MsOS_Memset(&gstHvspPostCropCfg, 0, sizeof(ST_MDRV_HVSP_POSTCROP_CONFIG));
    MDRV_HVSP_MUTEX_UNLOCK();
    for(i = 0 ;i <MDRV_MULTI_INST_PRIVATE_ID_NUM;i++)
    {
        MDRV_HVSP_MUTEX_LOCK();
        gu8PriMaskInst = i;
        MDRV_HVSP_MUTEX_UNLOCK();
        _MDrv_HVSP_ResetPriMaskStorage();
    }
    MDRV_HVSP_MUTEX_LOCK();
    gu8PriMaskInst = 0;
    MDRV_HVSP_MUTEX_UNLOCK();
}
void MDrv_HVSP_SetPriMaskInstId(unsigned char u8Id)
{
    MDRV_HVSP_MUTEX_LOCK();
    gu8PriMaskInst = u8Id;
    MDRV_HVSP_MUTEX_UNLOCK();
}
void MDrv_HVSP_Release(EN_MDRV_HVSP_ID_TYPE enHVSP_ID)
{
    EN_HVSP_ID_TYPE enID;
    enID = enHVSP_ID == E_MDRV_HVSP_ID_2 ? E_HVSP_ID_2 :
           enHVSP_ID == E_MDRV_HVSP_ID_3 ? E_HVSP_ID_3 :
                                           E_HVSP_ID_1;
   if(enHVSP_ID == E_MDRV_HVSP_ID_1)
   {
       _MDrv_HVSP_SWinit();
   }
   Drv_HVSP_Release(enID);
}
void MDrv_HVSP_Open(EN_MDRV_HVSP_ID_TYPE enHVSP_ID)
{
    EN_HVSP_ID_TYPE enID;
    enID = enHVSP_ID == E_MDRV_HVSP_ID_2 ? E_HVSP_ID_2 :
           enHVSP_ID == E_MDRV_HVSP_ID_3 ? E_HVSP_ID_3 :
                                           E_HVSP_ID_1;
    Drv_HVSP_Open(enID);
}
unsigned char MDrv_HVSP_Exit(unsigned char bCloseISR)
{
    if(_MHVSP_Mutex != -1)
    {
        MsOS_DeleteMutex(_MHVSP_Mutex);
        _MHVSP_Mutex = -1;
    }
    Drv_HVSP_Exit(bCloseISR);
    gbMemReadyForMdrv = 0;
    return 1;
}
unsigned char MDrv_HVSP_Init(EN_MDRV_HVSP_ID_TYPE enHVSP_ID, ST_MDRV_HVSP_INIT_CONFIG *pCfg)
{
    ST_HVSP_INIT_CONFIG stInitCfg;
    static unsigned char bInit = FALSE;
    char word[] = {"_MHVSP_Mutex"};
    MsOS_Memset(&stInitCfg,0,sizeof(ST_HVSP_INIT_CONFIG));
    if(MsOS_Init() == FALSE)
    {
        SCL_DBG(SCL_DBG_LV_HVSP()&(Get_DBGMG_HVSP(enHVSP_ID)), "[MDRVHVSP]%s(%d) init MSOS fail\n",
            __FUNCTION__, __LINE__);
        return FALSE;
    }

    _MHVSP_Mutex = MsOS_CreateMutex(E_MSOS_FIFO, word, MSOS_PROCESS_SHARED);

    if (_MHVSP_Mutex == -1)
    {
        SCL_DBG(SCL_DBG_LV_HVSP()&(Get_DBGMG_HVSP(enHVSP_ID)), "[DRVHVSP]%s(%d): create mutex fail\n",
            __FUNCTION__, __LINE__);
        return FALSE;
    }
    stInitCfg.u32RIUBase    = pCfg->u32Riubase;
    stInitCfg.u32IRQNUM     = pCfg ->u32IRQNUM;
    stInitCfg.u32CMDQIRQNUM = pCfg ->u32CMDQIRQNUM;
    gu8IPMBufferNum = DNRBufferMode;
    Drv_HVSP_SetBufferNum(gu8IPMBufferNum);
    if(Drv_HVSP_Init(&stInitCfg) == FALSE)
    {
        SCL_DBG(SCL_DBG_LV_HVSP()&(Get_DBGMG_HVSP(enHVSP_ID)), "[HVSP]%s Init Fail\n", __FUNCTION__);
        return FALSE;
    }
    else
    {
        if(bInit == FALSE)
        {
            _MDrv_HVSP_SWinit();
        }
        bInit = TRUE;

        return TRUE;
    }

}
void MDrv_HVSP_SetMemoryAllocateReady(unsigned char bEn)
{
    MDRV_HVSP_MUTEX_LOCK();
    gbMemReadyForMdrv = bEn;
    MDRV_HVSP_MUTEX_UNLOCK();
    Drv_HVSP_SetMemoryAllocateReady(gbMemReadyForMdrv);
}
void _MDrv_HVSP_FillIPMStruct(ST_MDRV_HVSP_IPM_CONFIG *pCfg,ST_HVSP_IPM_CONFIG *stIPMCfg)
{
    stIPMCfg->u16Fetch       = pCfg->u16Width;
    stIPMCfg->u16Vsize       = pCfg->u16Height;
    stIPMCfg->bYCMRead       = (unsigned char)((pCfg->enRW)&E_MDRV_HVSP_MCNR_YCM_R);
    stIPMCfg->bYCMWrite      = (unsigned char)(((pCfg->enRW)&E_MDRV_HVSP_MCNR_YCM_W)>>1);
    stIPMCfg->bCIIRRead       = (unsigned char)(((pCfg->enRW)&E_MDRV_HVSP_MCNR_CIIR_R)>>2);
    stIPMCfg->bCIIRWrite      = (unsigned char)(((pCfg->enRW)&E_MDRV_HVSP_MCNR_CIIR_W)>>3);
    stIPMCfg->u32YCBaseAddr    = pCfg->u32YCPhyAddr;
    stIPMCfg->u32MBaseAddr    = pCfg->u32MPhyAddr;
    stIPMCfg->u32CIIRBaseAddr    = pCfg->u32CIIRPhyAddr;
    stIPMCfg->u32MemSize     = pCfg->u32MemSize;
}

void _MDrv_HVSP_FillLDCStruct
    (ST_MDRV_HVSP_IPM_CONFIG *pCfg, MS_BOOL bWrite,ST_HVSP_LDC_FRAMEBUFFER_CONFIG *stLDCCfg)
{
    stLDCCfg->bEnSWMode      = 0x00;
    stLDCCfg->enLDCType      = EN_DRV_HVSP_LDCLCBANKMODE_128;
    stLDCCfg->u16Height      = pCfg->u16Height;
    if(pCfg->u16Width == 2048)
    {
        stLDCCfg->u16Width       = pCfg->u16Width -1;
    }
    else
    {
        stLDCCfg->u16Width       = pCfg->u16Width;
    }
    stLDCCfg->u32FBaddr      = pCfg->u32YCPhyAddr;
    stLDCCfg->u8FBidx        = 0x00;
    stLDCCfg->u8FBrwdiff     = SCL_DELAY2FRAMEINDOUBLEBUFFERMode ? 0x0 : 0x1;
                                    // delay 1 frame, set 1 is true, because Frame_buffer_idx = hw_w_idx - reg_ldc_fb_hw_rw_diff
                                    // So ,if want delay 2 frame, need to set '0' for 2 DNR buffer.
                                    //if only 1 DNR buffer , set '0' also delay 1 frame.
    stLDCCfg->bEnDNR         = bWrite;
}
unsigned char MDrv_HVSP_SetInitIPMConfig(EN_MDRV_HVSP_ID_TYPE enHVSP_ID, ST_MDRV_HVSP_IPM_CONFIG *pCfg)
{
    ST_HVSP_IPM_CONFIG stIPMCfg;
    ST_HVSP_LDC_FRAMEBUFFER_CONFIG stLDCCfg;
    if(enHVSP_ID != E_MDRV_HVSP_ID_1)
    {
        SCL_ERR( "[HVSP]%s ID not correct: %d\n", __FUNCTION__, enHVSP_ID);
        return FALSE;
    }
    MsOS_Memset(&stIPMCfg,0,sizeof(ST_HVSP_IPM_CONFIG));
    MsOS_Memset(&stLDCCfg,0,sizeof(ST_HVSP_LDC_FRAMEBUFFER_CONFIG));
    SCL_DBG(SCL_DBG_LV_HVSP()&(Get_DBGMG_HVSP(enHVSP_ID)), "%s:%d:PhyAddr=%lx, width=%x, height=%x \n",  __FUNCTION__,enHVSP_ID, pCfg->u32YCPhyAddr, pCfg->u16Width, pCfg->u16Height);
    _MDrv_HVSP_FillIPMStruct(pCfg,&stIPMCfg);
    _MDrv_HVSP_FillLDCStruct(pCfg, (stIPMCfg.bYCMRead ? 0 :1),&stLDCCfg);
    Drv_HVSP_SetLDCFrameBuffer_Config(&stLDCCfg);
    // ToDo
    // Drv_HVSP_SetPrv2CropOnOff(stIPMCfg.bYCMRead);
    if(Drv_HVSP_Set_IPM_Config(&stIPMCfg) == FALSE)
    {
        SCL_DBG(SCL_DBG_LV_HVSP()&(Get_DBGMG_HVSP(enHVSP_ID)), "[HVSP] Set IPM Config Fail\n");
        return FALSE;
    }
    else
    {
        return TRUE;;
    }
}


EN_MDRV_HVSP_PAT_TGEN_TIMING_TYPE _MDrv_HVSP_GetPatTGenTiming(ST_MDRV_HVSPTIMING_CONFIG *pTiming)
{
    EN_MDRV_HVSP_PAT_TGEN_TIMING_TYPE enTiming;

    if(Is_PTGEN_FHD(pTiming->u16Htotal,pTiming->u16Vtotal,pTiming->u16Vfrequency))
    {
        enTiming = E_MDRV_HVSP_PAT_TGEN_TIMING_1920_1080_30;
    }
    else if(Is_PTGEN_HD(pTiming->u16Htotal,pTiming->u16Vtotal,pTiming->u16Vfrequency))
    {
        enTiming = E_MDRV_HVSP_PAT_TGEN_TIMING_1024_768_60;
    }
    else if(Is_PTGEN_SD(pTiming->u16Htotal,pTiming->u16Vtotal,pTiming->u16Vfrequency))
    {
        enTiming = E_MDRV_HVSP_PAT_TGEN_TIMING_640_480_60;
    }
    else
    {
        enTiming = E_MDRV_HVSP_PAT_TGEN_TIMING_UNDEFINED;
    }

    SCL_DBG(SCL_DBG_LV_HVSP()&(Get_DBGMG_HVSP(3)), "[HVSP]%s(%d) Timing:%s(%d)", __FUNCTION__, __LINE__, PARSING_PAT_TGEN_TIMING(enTiming), enTiming);
    return enTiming;
}

//---------------------------------------------------------------------------------------------------------
// IOCTL function
//---------------------------------------------------------------------------------------------------------
MS_BOOL _MDrv_HVSP_IsInputSrcPatternGen(EN_HVSP_IP_MUX_TYPE enIPMux, ST_MDRV_HVSP_INPUT_CONFIG *pCfg)
{
    if(enIPMux == E_HVSP_IP_MUX_PAT_TGEN)
    {
        EN_MDRV_HVSP_PAT_TGEN_TIMING_TYPE enPatTgenTiming;
        ST_DRV_HVSP_PAT_TGEN_CONFIG stPatTgenCfg;

        enPatTgenTiming = _MDrv_HVSP_GetPatTGenTiming(&pCfg->stTimingCfg);
        SCL_DBG(SCL_DBG_LV_HVSP()&(Get_DBGMG_HVSP(E_MDRV_HVSP_ID_1)),
            "%s:%d type:%d\n", __FUNCTION__,E_MDRV_HVSP_ID_1,enPatTgenTiming);
        MsOS_Memcpy(&stPatTgenCfg, &gstPatTgenCfg[enPatTgenTiming],sizeof(ST_DRV_HVSP_PAT_TGEN_CONFIG));
        if(enPatTgenTiming == E_MDRV_HVSP_PAT_TGEN_TIMING_UNDEFINED)
        {
            stPatTgenCfg.u16HActive = pCfg->stCaptureWin.u16Width;
            stPatTgenCfg.u16VActive = pCfg->stCaptureWin.u16Height;
            stPatTgenCfg.u16Htt = stPatTgenCfg.u16HActive +
                                  stPatTgenCfg.u16HBackPorch +
                                  stPatTgenCfg.u16HFrontPorch +
                                  stPatTgenCfg.u16HSyncWidth;

            stPatTgenCfg.u16Vtt = stPatTgenCfg.u16VActive +
                                  stPatTgenCfg.u16VBackPorch +
                                  stPatTgenCfg.u16VFrontPorch +
                                  stPatTgenCfg.u16VSyncWidth;

        }
        return (unsigned char)Drv_HVSP_SetPatTgen(TRUE, &stPatTgenCfg);
    }
    else
    {
        return 0;
    }
}
void _MDrv_HVSP_FillPriMaskConfig(ST_MDRV_HVSP_PRIMASK_CONFIG *stCfg,ST_HVSP_PRIMASK_CONFIG *stPriMaskCfg)
{
    stPriMaskCfg->bMask = stCfg->bMask;
    stPriMaskCfg->u16Height = stCfg->stMaskWin.u16Height;
    stPriMaskCfg->u16Width = stCfg->stMaskWin.u16Width;
    stPriMaskCfg->u16X = stCfg->stMaskWin.u16X;
    stPriMaskCfg->u16Y = stCfg->stMaskWin.u16Y;
}
unsigned short _MDrv_HVSP_GetScaleParameter(unsigned short u16Src, unsigned short u16Dsp, unsigned short u16Val)
{
    //((unsigned short)((unsigned long)(value * RATIO_CONFIG *  numerator) / (denominator * RATIO_CONFIG)))
    unsigned long u32input;
    unsigned short u16output;
    u32input = u16Src *u16Val;
    u16output = (unsigned short)(u32input/u16Dsp);
    if((u32input%u16Dsp))
    {
        u16output ++;
    }
    return u16output;
}
void _MDrv_HVSP_TransPercentToPriCfg
    (ST_MDRV_HVSP_PRIMASK_CONFIG *stCfg ,ST_MDRV_HVSP_SCALING_CONFIG *sthvspCfg,ST_MDRV_HVSP_PRIMASK_CONFIG *stPerCfg)
{
    //gstHvspPriMaskCfg[gu8PriMaskInst][idx]
    //stPerCfg
    //ST_MDRV_HVSP_PRIMASK_CONFIG stCfg;
    stCfg->stMaskWin.u16X = (stPerCfg->stMaskWin.u16X * sthvspCfg->u16Dsp_Width) % PriMaskPercentBase;
    if(stCfg->stMaskWin.u16X > 0)
    {
        stCfg->stMaskWin.u16X = (stPerCfg->stMaskWin.u16X * sthvspCfg->u16Dsp_Width) / PriMaskPercentBase;
        stCfg->stMaskWin.u16X++;
    }
    else
    {
        stCfg->stMaskWin.u16X = (stPerCfg->stMaskWin.u16X * sthvspCfg->u16Dsp_Width) / PriMaskPercentBase;
    }
    stCfg->stMaskWin.u16Y = (stPerCfg->stMaskWin.u16Y * sthvspCfg->u16Dsp_Height) % PriMaskPercentBase;
    if(stCfg->stMaskWin.u16Y > 0)
    {
        stCfg->stMaskWin.u16Y = (stPerCfg->stMaskWin.u16Y * sthvspCfg->u16Dsp_Height) / PriMaskPercentBase;
        stCfg->stMaskWin.u16Y++;
    }
    else
    {
        stCfg->stMaskWin.u16Y = (stPerCfg->stMaskWin.u16Y * sthvspCfg->u16Dsp_Height) / PriMaskPercentBase;
    }
    stCfg->stMaskWin.u16Width = (stPerCfg->stMaskWin.u16Width * sthvspCfg->u16Dsp_Width) % PriMaskPercentBase;
    if(stCfg->stMaskWin.u16Width > 0)
    {
        stCfg->stMaskWin.u16Width= (stPerCfg->stMaskWin.u16Width * sthvspCfg->u16Dsp_Width) / PriMaskPercentBase;
        stCfg->stMaskWin.u16Width++;
    }
    else
    {
        stCfg->stMaskWin.u16Width= (stPerCfg->stMaskWin.u16Width * sthvspCfg->u16Dsp_Width) / PriMaskPercentBase;
    }
    stCfg->stMaskWin.u16Height = (stPerCfg->stMaskWin.u16Height * sthvspCfg->u16Dsp_Height) % PriMaskPercentBase;
    if(stCfg->stMaskWin.u16Height > 0)
    {
        stCfg->stMaskWin.u16Height= (stPerCfg->stMaskWin.u16Height * sthvspCfg->u16Dsp_Height) / PriMaskPercentBase;
        stCfg->stMaskWin.u16Height++;
    }
    else
    {
        stCfg->stMaskWin.u16Height= (stPerCfg->stMaskWin.u16Height * sthvspCfg->u16Dsp_Height) / PriMaskPercentBase;
    }
    stCfg->bMask = stPerCfg->bMask;
    stCfg->u8idx = stPerCfg->u8idx;
    SCL_DBG(SCL_DBG_LV_HVSP()&(Get_DBGMG_HVSP(E_MDRV_HVSP_ID_1)),
       "%s:MASK OnOff:%hhd ,Position:(%hd,%hd),w=%hd h=%hd  \n",
       __FUNCTION__,stCfg->bMask,stCfg->stMaskWin.u16X,stCfg->stMaskWin.u16Y,stCfg->stMaskWin.u16Width,
       stCfg->stMaskWin.u16Height);
}
void _MDrv_HVSP_TransPriCfgToPercent
    (ST_MDRV_HVSP_PRIMASK_CONFIG *stCfg ,ST_MDRV_HVSP_SCALING_CONFIG *sthvspCfg,ST_MDRV_HVSP_PRIMASK_CONFIG *stPerCfg)
{
    stPerCfg->stMaskWin.u16X = (stCfg->stMaskWin.u16X * PriMaskPercentBase) / sthvspCfg->u16Dsp_Width;
    stPerCfg->stMaskWin.u16Y = (stCfg->stMaskWin.u16Y * PriMaskPercentBase) / sthvspCfg->u16Dsp_Height;
    stPerCfg->stMaskWin.u16Width = (stCfg->stMaskWin.u16Width * PriMaskPercentBase) / sthvspCfg->u16Dsp_Width;
    stPerCfg->stMaskWin.u16Height = (stCfg->stMaskWin.u16Height * PriMaskPercentBase) / sthvspCfg->u16Dsp_Height;
    stPerCfg->bMask = stCfg->bMask;
    stPerCfg->u8idx = stCfg->u8idx;
    SCL_DBG(SCL_DBG_LV_HVSP()&(Get_DBGMG_HVSP(E_MDRV_HVSP_ID_1)),
       "%s:MASK OnOff:%hhd ,Position:(%hd,%hd),w=%hd h=%hd  (/10000)\n",
       __FUNCTION__,stPerCfg->bMask,stPerCfg->stMaskWin.u16X,stPerCfg->stMaskWin.u16Y,
       stPerCfg->stMaskWin.u16Height,stPerCfg->stMaskWin.u16Width);
}

void _MDrv_HVSP_GetPriMaskConfig
    (ST_MDRV_HVSP_PRIMASK_CONFIG *stCfg, ST_MDRV_HVSP_SCALING_CONFIG *sthvspCfg,ST_HVSP_PRIMASK_CONFIG *stPriMaskCfg)
{
    if(IS_HVSPNotOpen(sthvspCfg->u16Src_Width,sthvspCfg->u16Dsp_Width))
    {
        SCL_DBG(SCL_DBG_LV_HVSP()&(Get_DBGMG_HVSP(E_MDRV_HVSP_ID_1)),
           "%s:, Still Not Setting HVSP ,MASK OnOff:%hhd ,Area =%ld ,Position:(%hd,%hd)\n",
           __FUNCTION__,stCfg->bMask,
           (unsigned long)((unsigned long)stCfg->stMaskWin.u16Height*(unsigned long)stCfg->stMaskWin.u16Width),
           stCfg->stMaskWin.u16X,stCfg->stMaskWin.u16Y);
    }
    else
    {
        if(IS_NotScalingAfterCrop(sthvspCfg->stCropWin.bEn,
            sthvspCfg->u16Dsp_Height,sthvspCfg->stCropWin.u16Height,
            sthvspCfg->u16Dsp_Width,sthvspCfg->stCropWin.u16Width))
        {
            SCL_DBG(SCL_DBG_LV_HVSP()&(Get_DBGMG_HVSP(E_MDRV_HVSP_ID_1)),
               "%s:, NotScalingAfterCrop, Not need to handler,MASK OnOff:%hhd ,Area =%ld ,Position:(%hd,%hd)\n",
               __FUNCTION__,stCfg->bMask,
               (unsigned long)((unsigned long)stCfg->stMaskWin.u16Height*(unsigned long)stCfg->stMaskWin.u16Width),
               stCfg->stMaskWin.u16X,stCfg->stMaskWin.u16Y);
        }
        else
        {
            if(sthvspCfg->stCropWin.bEn)
            {
                stCfg->stMaskWin.u16X = (_MDrv_HVSP_GetScaleParameter
                (sthvspCfg->stCropWin.u16Width,sthvspCfg->u16Dsp_Width,stCfg->stMaskWin.u16X));
                stCfg->stMaskWin.u16Y = (_MDrv_HVSP_GetScaleParameter
                    (sthvspCfg->stCropWin.u16Height,sthvspCfg->u16Dsp_Height,stCfg->stMaskWin.u16Y));
                stCfg->stMaskWin.u16Height = (_MDrv_HVSP_GetScaleParameter
                    (sthvspCfg->stCropWin.u16Height,sthvspCfg->u16Dsp_Height,stCfg->stMaskWin.u16Height));
                stCfg->stMaskWin.u16Width = (_MDrv_HVSP_GetScaleParameter
                    (sthvspCfg->stCropWin.u16Width,sthvspCfg->u16Dsp_Width,stCfg->stMaskWin.u16Width));
            }
            else
            {
                stCfg->stMaskWin.u16X = (_MDrv_HVSP_GetScaleParameter
                (sthvspCfg->u16Src_Width,sthvspCfg->u16Dsp_Width,stCfg->stMaskWin.u16X));
                stCfg->stMaskWin.u16Y = (_MDrv_HVSP_GetScaleParameter
                    (sthvspCfg->u16Src_Height,sthvspCfg->u16Dsp_Height,stCfg->stMaskWin.u16Y));
                stCfg->stMaskWin.u16Height = (_MDrv_HVSP_GetScaleParameter
                    (sthvspCfg->u16Src_Height,sthvspCfg->u16Dsp_Height,stCfg->stMaskWin.u16Height));
                stCfg->stMaskWin.u16Width = (_MDrv_HVSP_GetScaleParameter
                    (sthvspCfg->u16Src_Width,sthvspCfg->u16Dsp_Width,stCfg->stMaskWin.u16Width));
            }
            SCL_DBG(SCL_DBG_LV_HVSP()&(Get_DBGMG_HVSP(E_MDRV_HVSP_ID_1)),
               "%s:, Mask move,MASK OnOff:%hhd ,H =%ld,W=%ld ,Position:(%hd,%hd)\n",
               __FUNCTION__,stCfg->bMask,
               (unsigned long)stCfg->stMaskWin.u16Height,(unsigned long)stCfg->stMaskWin.u16Width,
               stCfg->stMaskWin.u16X,stCfg->stMaskWin.u16Y);
        }
    }
    _MDrv_HVSP_FillPriMaskConfig(stCfg,stPriMaskCfg);
}
unsigned char _MDrv_HVSP_FindUnusePriMask(void)
{
    unsigned char u8Num;
    unsigned char u8lastTimeNum = 0;
    for(u8Num=0;u8Num<PriMaskNum;u8Num++)
    {
        if(gstHvspPriMaskCfg[gu8PriMaskInst][u8Num].bMask == 0)
        {
            u8lastTimeNum = u8Num;
            break;
        }
        if(u8Num == (PriMaskNum-1))
        {
            u8lastTimeNum = PriMaskNum;
            break;
        }

    }
    return u8lastTimeNum;
}
void _MDrv_HVSP_SetPriMaskStorage(ST_MDRV_HVSP_PRIMASK_CONFIG *stPriMaskCfg)
{
    MDRV_HVSP_MUTEX_LOCK();
    gu8PriMaskNum[gu8PriMaskInst] = PriMaskNum;
    MsOS_Memcpy(&gstHvspPriMaskCfg[gu8PriMaskInst][stPriMaskCfg->u8idx],stPriMaskCfg,sizeof(ST_HVSP_PRIMASK_CONFIG));
    MDRV_HVSP_MUTEX_UNLOCK();
}
void MDrv_HVSP_SetPatTgenStatus(unsigned char bEn)
{
    Drv_HVSP_SetPatTgen(bEn, NULL);
}
unsigned char MDrv_HVSP_SetInputConfig(EN_MDRV_HVSP_ID_TYPE enHVSP_ID, ST_MDRV_HVSP_INPUT_CONFIG *pCfg)
{
    unsigned char Ret = TRUE;
    SCL_DBG(SCL_DBG_LV_HVSP()&(Get_DBGMG_HVSP(enHVSP_ID)), "%s:%d\n", __FUNCTION__,enHVSP_ID);

    //SCL_ERR("clk1:%d, clk2:%d\n",(int)pCfg->stclk->idclk,(int)pCfg->stclk->fclk1);

    if(Is_Wrong_Type(enHVSP_ID,pCfg->enSrcType))
    {
        SCL_ERR( "[HVSP] Wrong Input Type: %d, %d\n", enHVSP_ID, pCfg->enSrcType);
        return FALSE;
    }
    if(enHVSP_ID == E_MDRV_HVSP_ID_1)
    {
        EN_HVSP_IP_MUX_TYPE enIPMux;
        enIPMux = pCfg->enSrcType == E_MDRV_HVSP_SRC_ISP      ? E_HVSP_IP_MUX_ISP :
                  pCfg->enSrcType == E_MDRV_HVSP_SRC_BT656    ? E_HVSP_IP_MUX_BT656 :
                  pCfg->enSrcType == E_MDRV_HVSP_SRC_HVSP     ? E_HVSP_IP_MUX_HVSP :
                  pCfg->enSrcType == E_MDRV_HVSP_SRC_PAT_TGEN ? E_HVSP_IP_MUX_PAT_TGEN :
                                                                 E_HVSP_IP_MUX_MAX;
        Ret &= (unsigned char)Drv_HVSP_SetInputMux(enIPMux,(ST_HVSP_CLK_CONFIG *)pCfg->stclk);
        Drv_HVSP_SetInputSrcSize(pCfg->stCaptureWin.u16Height,pCfg->stCaptureWin.u16Width);
        //Drv_HVSP_SetCropWindowSize();
        if(_MDrv_HVSP_IsInputSrcPatternGen(enIPMux, pCfg))
        {
            Ret &= TRUE;
        }
        else
        {
            Ret &= (unsigned char)Drv_HVSP_SetPatTgen(FALSE, NULL);
        }

    }
    return Ret;
}

unsigned char MDrv_HVSP_SetPostCropConfig(EN_MDRV_HVSP_ID_TYPE enHVSP_ID, ST_MDRV_HVSP_POSTCROP_CONFIG *pCfg)
{
    EN_HVSP_ID_TYPE enID;
    ST_HVSP_SCALING_CONFIG stScalingCfg;
    unsigned char ret;
    MsOS_Memset(&stScalingCfg,0,sizeof(ST_HVSP_SCALING_CONFIG));
    SCL_DBG(SCL_DBG_LV_HVSP()&(Get_DBGMG_HVSP(enHVSP_ID)), "%s\n", __FUNCTION__);

    enID = enHVSP_ID == E_MDRV_HVSP_ID_2 ? E_HVSP_ID_2 :
           enHVSP_ID == E_MDRV_HVSP_ID_3 ? E_HVSP_ID_3 :
                                           E_HVSP_ID_1;

    MDRV_HVSP_MUTEX_LOCK();
    MsOS_Memcpy(&gstHvspPostCropCfg, pCfg, sizeof(ST_MDRV_HVSP_POSTCROP_CONFIG));
    MDRV_HVSP_MUTEX_UNLOCK();
    stScalingCfg.bCropEn[DRV_HVSP_CROP_1]        = 0;
    stScalingCfg.bCropEn[DRV_HVSP_CROP_2]        = pCfg->bCropEn;
    stScalingCfg.u16Crop_X[DRV_HVSP_CROP_2]      = pCfg->u16X;
    stScalingCfg.u16Crop_Y[DRV_HVSP_CROP_2]      = pCfg->u16Y;
    stScalingCfg.u16Crop_Width[DRV_HVSP_CROP_2]  = pCfg->u16Width;
    stScalingCfg.u16Crop_Height[DRV_HVSP_CROP_2] = pCfg->u16Height;

    stScalingCfg.u16Src_Width  = gstHvspScalingCfg.u16Src_Width;
    stScalingCfg.u16Src_Height = gstHvspScalingCfg.u16Src_Height;
    stScalingCfg.u16Dsp_Width  = gstHvspScalingCfg.u16Dsp_Width;
    stScalingCfg.u16Dsp_Height = gstHvspScalingCfg.u16Dsp_Height;

    stScalingCfg.stCmdTrigCfg.enType = E_HVSP_CMD_TRIG_NONE; //ToDo

    ret = Drv_HVSP_SetScaling(enID, &stScalingCfg,(ST_HVSP_CLK_CONFIG *)pCfg->stclk);
    return  ret;


}
unsigned char _MDrv_HVSP_bChangePri(ST_MDRV_HVSP_SCALING_CONFIG *pCfg )
{
    unsigned char bChangePri;
    if( pCfg->stCropWin.bEn == gstHvspScalingCfg.stCropWin.bEn)
    {
        bChangePri = 0;
    }
    else
    {
        return 1;
    }
    if(pCfg->stCropWin.u16X == gstHvspScalingCfg.stCropWin.u16X)
    {
        bChangePri = 0;
    }
    else
    {
        return 1;
    }
    if(pCfg->stCropWin.u16Y == gstHvspScalingCfg.stCropWin.u16Y)
    {
        bChangePri = 0;
    }
    else
    {
        return 1;
    }
    if(pCfg->stCropWin.u16Width == gstHvspScalingCfg.stCropWin.u16Width)
    {
        bChangePri = 0;
    }
    else
    {
        return 1;
    }
    if(pCfg->stCropWin.u16Height == gstHvspScalingCfg.stCropWin.u16Height)
    {
        bChangePri = 0;
    }
    else
    {
        return 1;
    }

    return bChangePri;
}
void _MDrv_HVSP_ChangebyZoomSizeLimitation
    (ST_HVSP_SCALING_CONFIG *stCfg ,ST_MDRV_HVSP_SCALING_CONFIG *stRet)
{
    unsigned char u16cropType;
    if( SetPostCrop)
    {
        u16cropType = 1;
    }
    else
    {
        u16cropType = 0;
    }
    stRet->stCropWin.bEn = stCfg->bCropEn[u16cropType];
    stRet->stCropWin.u16Height = stCfg->u16Crop_Height[u16cropType];
    stRet->stCropWin.u16Width = stCfg->u16Crop_Width[u16cropType] ;
    stRet->stCropWin.u16X = stCfg->u16Crop_X[u16cropType];
    stRet->stCropWin.u16Y = stCfg->u16Crop_Y[u16cropType];
    stRet->u16Dsp_Height = gstHvspScalingCfg.u16Dsp_Height;
    stRet->u16Dsp_Width = gstHvspScalingCfg.u16Dsp_Width;
    stRet->u16Src_Height = gstHvspScalingCfg.u16Src_Height;
    stRet->u16Src_Width = gstHvspScalingCfg.u16Src_Width;
}
unsigned char MDrv_HVSP_SetScalingConfig(EN_MDRV_HVSP_ID_TYPE enHVSP_ID, ST_MDRV_HVSP_SCALING_CONFIG *pCfg )
{
    EN_HVSP_ID_TYPE enID;
    ST_HVSP_SCALING_CONFIG stScalingCfg;
    unsigned char ret = 0;
    unsigned char idx = 0;
    unsigned char bChangePri = 0;
    ST_HVSP_PRIMASK_CONFIG stPriMaskCfg;
    ST_MDRV_HVSP_PRIMASK_CONFIG stPriMaskStore;
    ST_MDRV_HVSP_SCALING_CONFIG stChange;
    MsOS_Memset(&stScalingCfg,0,sizeof(ST_HVSP_SCALING_CONFIG));
    MsOS_Memset(&stChange,0,sizeof(ST_MDRV_HVSP_SCALING_CONFIG));
    MsOS_Memset(&stPriMaskCfg,0,sizeof(ST_MDRV_HVSP_PRIMASK_CONFIG));
    MsOS_Memset(&stPriMaskStore,0,sizeof(ST_HVSP_PRIMASK_CONFIG));
    SCL_DBG(SCL_DBG_LV_HVSP()&(Get_DBGMG_HVSP(enHVSP_ID)), "%s:%d\n", __FUNCTION__,enHVSP_ID);
    enID = enHVSP_ID == E_MDRV_HVSP_ID_2 ? E_HVSP_ID_2 :
           enHVSP_ID == E_MDRV_HVSP_ID_3 ? E_HVSP_ID_3 :
               E_HVSP_ID_1;

   if(enID == E_HVSP_ID_1)
   {
       bChangePri = _MDrv_HVSP_bChangePri(pCfg);
       MDRV_HVSP_MUTEX_LOCK();
       MsOS_Memcpy(&gstHvspScalingCfg, pCfg, sizeof(ST_MDRV_HVSP_SCALING_CONFIG));
       MDRV_HVSP_MUTEX_UNLOCK();
   }

    if( SetPostCrop || (Drv_HVSP_GetInputSrcMux()==E_HVSP_IP_MUX_PAT_TGEN))
    {
        stScalingCfg.bCropEn[DRV_HVSP_CROP_1]        = 0;
        stScalingCfg.u16Crop_X[DRV_HVSP_CROP_1]      = 0;
        stScalingCfg.u16Crop_Y[DRV_HVSP_CROP_1]      = 0;
        stScalingCfg.u16Crop_Width[DRV_HVSP_CROP_1]  = pCfg->u16Src_Width;
        stScalingCfg.u16Crop_Height[DRV_HVSP_CROP_1] = pCfg->u16Src_Height;
        stScalingCfg.bCropEn[DRV_HVSP_CROP_2]        = pCfg->stCropWin.bEn;
        stScalingCfg.u16Crop_X[DRV_HVSP_CROP_2]      = pCfg->stCropWin.u16X;
        stScalingCfg.u16Crop_Y[DRV_HVSP_CROP_2]      = pCfg->stCropWin.u16Y;
        stScalingCfg.u16Crop_Width[DRV_HVSP_CROP_2]  = pCfg->stCropWin.u16Width;
        stScalingCfg.u16Crop_Height[DRV_HVSP_CROP_2] = pCfg->stCropWin.u16Height;
    }
    else
    {
        stScalingCfg.bCropEn[DRV_HVSP_CROP_1]        = pCfg->stCropWin.bEn;
        stScalingCfg.u16Crop_X[DRV_HVSP_CROP_1]      = pCfg->stCropWin.u16X;
        stScalingCfg.u16Crop_Y[DRV_HVSP_CROP_1]      = pCfg->stCropWin.u16Y;
        stScalingCfg.u16Crop_Width[DRV_HVSP_CROP_1]  = pCfg->stCropWin.u16Width;
        stScalingCfg.u16Crop_Height[DRV_HVSP_CROP_1] = pCfg->stCropWin.u16Height;
        stScalingCfg.bCropEn[DRV_HVSP_CROP_2]        = 0;
        stScalingCfg.u16Crop_X[DRV_HVSP_CROP_2]      = 0;
        stScalingCfg.u16Crop_Y[DRV_HVSP_CROP_2]      = 0;
        stScalingCfg.u16Crop_Width[DRV_HVSP_CROP_2]  = pCfg->stCropWin.u16Width;
        stScalingCfg.u16Crop_Height[DRV_HVSP_CROP_2] = pCfg->stCropWin.u16Height;
    }
    stScalingCfg.u16Src_Width  = pCfg->u16Src_Width;
    stScalingCfg.u16Src_Height = pCfg->u16Src_Height;
    stScalingCfg.u16Dsp_Width  = pCfg->u16Dsp_Width;
    stScalingCfg.u16Dsp_Height = pCfg->u16Dsp_Height;
    if(enHVSP_ID==E_MDRV_HVSP_ID_1 || enHVSP_ID==E_MDRV_HVSP_ID_2)
    {
        stScalingCfg.stCmdTrigCfg.enType = E_HVSP_CMD_TRIG_CMDQ_LDC_SYNC; //ToDo
    }
    else if(enHVSP_ID==E_MDRV_HVSP_ID_3)
    {
        stScalingCfg.stCmdTrigCfg.enType = E_HVSP_CMD_TRIG_NONE; //ToDo
    }
    else
    {
        stScalingCfg.stCmdTrigCfg.enType = E_HVSP_CMD_TRIG_POLL_LDC_SYNC; //ToDo
    }
    stScalingCfg.stCmdTrigCfg.u8Fmcnt = 0;
    ret = Drv_HVSP_SetScaling(enID, &stScalingCfg,(ST_HVSP_CLK_CONFIG *)pCfg->stclk);

    if(enID == E_HVSP_ID_1)
    {
        if(!ret)
        {
            _MDrv_HVSP_ChangebyZoomSizeLimitation(&stScalingCfg,&stChange);
        }
        else
        {
            MsOS_Memcpy(&stChange, &gstHvspScalingCfg, sizeof(ST_MDRV_HVSP_SCALING_CONFIG));
        }
        if(gu8PriMaskNum[gu8PriMaskInst] && bChangePri)
        {
            //disable trigger
            Drv_HVSP_SetPriMaskTrigger(EN_HVSP_PRIMASK_DISABLE);
            for(idx=0;idx<gu8PriMaskNum[gu8PriMaskInst];idx++)
            {
                if(gstHvspPriMaskCfg[gu8PriMaskInst][idx].u8idx == idx && gstHvspPriMaskCfg[gu8PriMaskInst][idx].bMask)
                {
                    _MDrv_HVSP_TransPercentToPriCfg(&stPriMaskStore,&stChange,&gstHvspPriMaskCfg[gu8PriMaskInst][idx]);
                    _MDrv_HVSP_GetPriMaskConfig(&stPriMaskStore,&stChange,&stPriMaskCfg);
                    Drv_HVSP_SetPriMaskConfig(&stPriMaskCfg);
                }
            }
            Drv_HVSP_SetPriMaskTrigger(EN_HVSP_PRIMASK_ENABLE);
        }
    }
    return ret;
}



static void  _MDrv_HVSP_SetRegisterForce(MS_U32 u32Size, MS_U8 *pBuf)
{
    MS_U32 i;
    MS_U32 u32Reg;
    MS_U16 u16Bank;
    MS_U8  u8Addr, u8Val, u8Msk;

    // bank,  addrr,  val,  msk
    for(i=0; i<u32Size; i+=5)
    {
        u16Bank = (MS_U16)pBuf[i+0] | ((MS_U16)pBuf[i+1])<<8;
        u8Addr  = pBuf[i+2];
        u8Val   = pBuf[i+3];
        u8Msk   = pBuf[i+4];
        u32Reg  = (((MS_U32)u16Bank) << 8) | (MS_U32)u8Addr;

        SCL_DBG(SCL_DBG_LV_HVSP()&(Get_DBGMG_HVSP(3)), "%08lx, %02x, %02x\n", u32Reg, u8Val, u8Msk);
        Drv_HVSP_SetRegisterForce(u32Reg, u8Val, u8Msk);
    }
}


unsigned char MDrv_HVSP_SetMiscConfig(ST_MDRV_HVSP_MISC_CONFIG *pCfg)
{
    MS_U8 *pBuf = NULL;

    SCL_DBG(SCL_DBG_LV_HVSP()&(Get_DBGMG_HVSP(3)), "%s\n", __FUNCTION__);

    pBuf = MsOS_Memalloc(pCfg->u32Size, GFP_KERNEL);

    if(pBuf == NULL)
    {
        SCL_ERR( "[HVSP1] allocate buffer fail\n");
        return 0;
    }


    if(MsOS_copy_from_user(pBuf, (__user ST_MDRV_HVSP_MISC_CONFIG  *)pCfg->u32Addr, pCfg->u32Size))
    {
        SCL_ERR( "[HVSP1] copy msic buffer error\n");
        MsOS_MemFree(pBuf);
        return 0;
    }

    switch(pCfg->u8Cmd)
    {
    case E_MDRV_HVSP_MISC_CMD_SET_REG:
        _MDrv_HVSP_SetRegisterForce(pCfg->u32Size, pBuf);
        break;

    default:
        break;
    }

    MsOS_MemFree(pBuf);

    return 1;
}

unsigned char MDrv_HVSP_SetMiscConfigForKernel(ST_MDRV_HVSP_MISC_CONFIG *pCfg)
{
     SCL_DBG(SCL_DBG_LV_HVSP()&(Get_DBGMG_HVSP(3)), "%s\n", __FUNCTION__);
     switch(pCfg->u8Cmd)
     {
     case E_MDRV_HVSP_MISC_CMD_SET_REG:
         _MDrv_HVSP_SetRegisterForce(pCfg->u32Size, (MS_U8 *)pCfg->u32Addr);
         break;

     default:
         break;
     }
     return 1;
}

unsigned char MDrv_HVSP_GetSCLInform(EN_MDRV_HVSP_ID_TYPE enHVSP_ID, ST_MDRV_HVSP_SCINFORM_CONFIG *pstCfg)
{
     ST_DRV_HVSP_SCINFORM_CONFIG stInformCfg;
     EN_HVSP_ID_TYPE enID;
     unsigned char bRet = 1;
     MsOS_Memset(&stInformCfg,0,sizeof(ST_DRV_HVSP_SCINFORM_CONFIG));
     SCL_DBG(SCL_DBG_LV_HVSP()&(Get_DBGMG_HVSP(enHVSP_ID)), "[HVSP1]%s\n", __FUNCTION__);
     enID = enHVSP_ID == E_MDRV_HVSP_ID_2 ? E_HVSP_ID_2 :
            enHVSP_ID == E_MDRV_HVSP_ID_3 ? E_HVSP_ID_3 :
                                            E_HVSP_ID_1;

     Drv_HVSP_GetSCLInform(enID, &stInformCfg);
     MsOS_Memcpy(pstCfg, &stInformCfg, sizeof(ST_MDRV_HVSP_SCINFORM_CONFIG));
     SCL_DBG(SCL_DBG_LV_HVSP()&(Get_DBGMG_HVSP(enHVSP_ID)),
        "[HVSP1]u16Height:%hd u16Width:%hd\n",stInformCfg.u16Height, stInformCfg.u16Width);
     return bRet;
}

unsigned char MDrv_HVSP_SetOSDConfig(EN_MDRV_HVSP_ID_TYPE enHVSP_ID, ST_MDRV_HVSP_OSD_CONFIG* pstCfg)
{
     unsigned char Ret = TRUE;
     ST_DRV_HVSP_OSD_CONFIG stOSdCfg;
     EN_HVSP_ID_TYPE enID;
     MsOS_Memset(&stOSdCfg,0,sizeof(ST_DRV_HVSP_OSD_CONFIG));
     enID = enHVSP_ID == E_MDRV_HVSP_ID_2 ? E_HVSP_ID_2 :
            enHVSP_ID == E_MDRV_HVSP_ID_3 ? E_HVSP_ID_3 :
                                            E_HVSP_ID_1;
     MsOS_Memcpy(pstCfg, &stOSdCfg, sizeof(ST_MDRV_HVSP_OSD_CONFIG));
     Drv_HVSP_SetOSDConfig(enID, &stOSdCfg);
     SCL_DBG(SCL_DBG_LV_HVSP()&(Get_DBGMG_HVSP(enID)),
        "%s::ID:%d, OnOff:%hhd ,Bypass:%hhd\n",
        __FUNCTION__,enID,stOSdCfg.stOsdOnOff.bOSDEn,stOSdCfg.stOsdOnOff.bOSDBypass);
     return Ret;
}
unsigned char MDrv_HVSP_SetPriMaskConfig(ST_MDRV_HVSP_PRIMASK_CONFIG *stCfg)
{
    unsigned char Ret = TRUE;
    unsigned char u8idx = 0;
    ST_HVSP_PRIMASK_CONFIG stPriMaskCfg;
    ST_MDRV_HVSP_PRIMASK_CONFIG stPriMaskStore;
    MsOS_Memset(&stPriMaskCfg,0,sizeof(ST_HVSP_PRIMASK_CONFIG));
    MsOS_Memset(&stPriMaskStore,0,sizeof(ST_MDRV_HVSP_PRIMASK_CONFIG));
    SCL_DBG(SCL_DBG_LV_HVSP()&(Get_DBGMG_HVSP(E_MDRV_HVSP_ID_1)),
    "%s::ID:%d, OnOff:%hhd id:%hhd\n",__FUNCTION__,E_MDRV_HVSP_ID_1,stCfg->bMask,stCfg->u8idx);
    //need to turn off mask
    //1.turn off same idx
    //2.turn on others idx
    if(stCfg->u8idx == gstHvspPriMaskCfg[gu8PriMaskInst][stCfg->u8idx].u8idx &&
        gstHvspPriMaskCfg[gu8PriMaskInst][stCfg->u8idx].bMask)
    {
        Drv_HVSP_PriMaskBufferReset();
        //ReOpen to avoid disable repeat region.
        for(u8idx = 0 ; u8idx<PriMaskNum; u8idx++)
        {
            if(u8idx == stCfg->u8idx)
            {
                continue;
            }
            else if(u8idx == gstHvspPriMaskCfg[gu8PriMaskInst][u8idx].u8idx
                && gstHvspPriMaskCfg[gu8PriMaskInst][u8idx].bMask)
            {
                _MDrv_HVSP_TransPercentToPriCfg(&stPriMaskStore,&gstHvspScalingCfg,&gstHvspPriMaskCfg[gu8PriMaskInst][u8idx]);
                _MDrv_HVSP_GetPriMaskConfig(&stPriMaskStore,&gstHvspScalingCfg,&stPriMaskCfg);
                Drv_HVSP_SetPriMaskConfig(&stPriMaskCfg);
            }
        }
    }
    _MDrv_HVSP_TransPriCfgToPercent(stCfg,&gstHvspScalingCfg,&stPriMaskStore);
    _MDrv_HVSP_SetPriMaskStorage(&stPriMaskStore);
    _MDrv_HVSP_GetPriMaskConfig(stCfg,&gstHvspScalingCfg,&stPriMaskCfg);
    if(stPriMaskCfg.bMask)
    {
        Drv_HVSP_SetPriMaskConfig(&stPriMaskCfg);
    }
     return Ret;
}
unsigned char MDrv_HVSP_SetPriMaskTrigger(unsigned char bEn)
{
     unsigned char Ret = TRUE;
     SCL_DBG(SCL_DBG_LV_HVSP()&(Get_DBGMG_HVSP(E_MDRV_HVSP_ID_1)),
        "%s, OnOff:%hhd\n",__FUNCTION__,bEn);
     Drv_HVSP_SetPriMaskTrigger(bEn ? EN_HVSP_PRIMASK_ENABLE : EN_HVSP_PRIMASK_DISABLE);
     if(bEn == EN_HVSP_PRIMASK_DISABLE)
     {
         _MDrv_HVSP_ResetPriMaskStorage();
     }
     return Ret;
}

unsigned char Mdrv_HVSP_GetFrameBufferCountInformation(void)
{
    return gu8IPMBufferNum;
}

unsigned char MDrv_HVSP_SetFbManageConfig(EN_MDRV_HVSP_FBMG_SET_TYPE enSet)
{
    unsigned char Ret = TRUE;
    ST_DRV_HVSP_SET_FB_MANAGE_CONFIG stCfg;
    MsOS_Memset(&stCfg,0,sizeof(ST_DRV_HVSP_SET_FB_MANAGE_CONFIG));
    SCL_DBG(SCL_DBG_LV_HVSP()&(Get_DBGMG_HVSP(3)), "%s\n", __FUNCTION__);
    stCfg.enSet = (EN_DRV_HVSP_FBMG_SET_TYPE)enSet;
    if (stCfg.enSet & EN_DRV_HVSP_FBMG_SET_DNR_COMDE_ON)
    {
        SCL_ERR( "COMDE_ON\n");
        MDrv_VIP_SetMCNRConpressForDebug(1);
    }
    else if (stCfg.enSet & EN_DRV_HVSP_FBMG_SET_DNR_COMDE_OFF)
    {
        SCL_ERR( "COMDE_OFF\n");
        MDrv_VIP_SetMCNRConpressForDebug(0);
    }
    else if (stCfg.enSet & EN_DRV_HVSP_FBMG_SET_DNR_COMDE_265OFF)
    {
        SCL_ERR( "COMDE_265OFF\n");
        MDrv_VIP_SetMCNRConpressForDebug(0);
    }
    if(stCfg.enSet &EN_DRV_HVSP_FBMG_SET_DNR_BUFFER_1)
    {
        gu8IPMBufferNum = 1;
        Drv_HVSP_SetBufferNum(gu8IPMBufferNum);
    }
    else if (stCfg.enSet &EN_DRV_HVSP_FBMG_SET_DNR_BUFFER_2)
    {
        gu8IPMBufferNum = 2;
        Drv_HVSP_SetBufferNum(gu8IPMBufferNum);
    }
    Drv_HVSP_SetFbManageConfig(&stCfg);
    return Ret;
}
void MDrv_HVSP_IDCLKRelease(ST_MDRV_HVSP_CLK_CONFIG* stclk)
{
    SCL_DBG(SCL_DBG_LV_HVSP()&(Get_DBGMG_HVSP(3)), "%s\n", __FUNCTION__);
    Drv_HVSP_IDCLKRelease((ST_HVSP_CLK_CONFIG *)stclk);
}



unsigned char MDrv_HVSP_InputVSyncMonitor(void)
{
    if(Drv_HVSP_CheckInputVSync())
    {
        return 1;
    }
    else
    {
        return 0;
    }
}
#if 1
void MDrv_HVSP_SetCLKForcemode(unsigned char bEn)
{
    Drv_HVSP_SetCLKForcemode(bEn);
}
void MDrv_HVSP_SetCLKRate(void* adjclk,unsigned char u8Idx)
{
    MSOS_ST_CLK* pstclock = NULL;
    Drv_HVSP_SetCLKRate(u8Idx);
    if (MsOS_clk_get_enable_count((MSOS_ST_CLK*)adjclk)==0)
    {
    }
    else
    {
        if (NULL != (pstclock = MsOS_clk_get_parent_by_index((MSOS_ST_CLK*)adjclk, (u8Idx &0x0F))))
        {
            MsOS_clk_set_parent((MSOS_ST_CLK*)adjclk, pstclock);
        }
    }
}
void MDrv_HVSP_SetCLKOnOff(void* adjclk,unsigned char bEn)
{
    MSOS_ST_CLK* pstclock = NULL;
    if (MsOS_clk_get_enable_count((MSOS_ST_CLK*)adjclk)==0 &&bEn)
    {
        if (NULL != (pstclock = MsOS_clk_get_parent_by_index((MSOS_ST_CLK*)adjclk, 0)))
        {
            MsOS_clk_set_parent((MSOS_ST_CLK*)adjclk, pstclock);
            MsOS_clk_prepare_enable((MSOS_ST_CLK*)adjclk);
        }
    }
    else if(MsOS_clk_get_enable_count((MSOS_ST_CLK*)adjclk)!=0 && !bEn)
    {
        if (NULL != (pstclock = MsOS_clk_get_parent_by_index((MSOS_ST_CLK*)adjclk, 0)))
        {
            MsOS_clk_set_parent((MSOS_ST_CLK*)adjclk, pstclock);
            MsOS_clk_disable_unprepare((MSOS_ST_CLK*)adjclk);
        }
    }
}
ssize_t MDrv_HVSP_monitorHWShow(char *buf,int VsyncCount ,int MonitorErrCount)
{
    char *str = buf;
    char *end = buf + PAGE_SIZE;
    str += MsOS_scnprintf(str, end - str, "========================SCL monitor HW BUTTON======================\n");
    str += MsOS_scnprintf(str, end - str, "CROP Monitor    :1\n");
    str += MsOS_scnprintf(str, end - str, "DMA1FRM Monitor :2\n");
    str += MsOS_scnprintf(str, end - str, "DMA1SNP Monitor :3\n");
    str += MsOS_scnprintf(str, end - str, "DMA2FRM Monitor :4\n");
    str += MsOS_scnprintf(str, end - str, "DMA3FRM Monitor :5\n");
    str += MsOS_scnprintf(str, end - str, "========================SCL monitor HW ======================\n");
    str += MsOS_scnprintf(str, end - str, "vysnc count:%d",VsyncCount);
    str += MsOS_scnprintf(str, end - str, "Monitor Err count:%d\n",MonitorErrCount);
    return (str - buf);
}
ssize_t MDrv_HVSP_DbgmgFlagShow(char *buf)
{
    char *str = buf;
    char *end = buf + PAGE_SIZE;
    str += MsOS_scnprintf(str, end - str, "========================SCL Debug Message BUTTON======================\n");
    str += MsOS_scnprintf(str, end - str, "CONFIG            ECHO        STATUS\n");
    str += MsOS_scnprintf(str, end - str, "MDRV_CONFIG       (1)         0x%x\n",gbdbgmessage[EN_DBGMG_MDRV_CONFIG]);
    str += MsOS_scnprintf(str, end - str, "IOCTL_CONFIG      (2)         0x%x\n",gbdbgmessage[EN_DBGMG_IOCTL_CONFIG]);
    str += MsOS_scnprintf(str, end - str, "HVSP_CONFIG       (3)         0x%x\n",gbdbgmessage[EN_DBGMG_HVSP_CONFIG]);
    str += MsOS_scnprintf(str, end - str, "SCLDMA_CONFIG     (4)         0x%x\n",gbdbgmessage[EN_DBGMG_SCLDMA_CONFIG]);
    str += MsOS_scnprintf(str, end - str, "PNL_CONFIG        (5)         0x%x\n",gbdbgmessage[EN_DBGMG_PNL_CONFIG]);
    str += MsOS_scnprintf(str, end - str, "VIP_CONFIG        (6)         0x%x\n",gbdbgmessage[EN_DBGMG_VIP_CONFIG]);
    str += MsOS_scnprintf(str, end - str, "DRVPQ_CONFIG      (7)         0x%x\n",gbdbgmessage[EN_DBGMG_DRVPQ_CONFIG]);
    str += MsOS_scnprintf(str, end - str, "INST_ENTRY_CONFIG (8)         0x%x\n",gbdbgmessage[EN_DBGMG_INST_ENTRY_CONFIG]);
    str += MsOS_scnprintf(str, end - str, "INST_LOCK_CONFIG  (9)         0x%x\n",gbdbgmessage[EN_DBGMG_INST_LOCK_CONFIG]);
    str += MsOS_scnprintf(str, end - str, "INST_FUNC_CONFIG  (A)         0x%x\n",gbdbgmessage[EN_DBGMG_INST_FUNC_CONFIG]);
    str += MsOS_scnprintf(str, end - str, "DRVHVSP_CONFIG    (B)         0x%x\n",gbdbgmessage[EN_DBGMG_DRVHVSP_CONFIG]);
    str += MsOS_scnprintf(str, end - str, "DRVSCLDMA_CONFIG  (C)         0x%x\n",gbdbgmessage[EN_DBGMG_DRVSCLDMA_CONFIG]);
    str += MsOS_scnprintf(str, end - str, "DRVSCLIRQ_CONFIG  (D)         0x%x\n",gbdbgmessage[EN_DBGMG_DRVSCLIRQ_CONFIG]);
    str += MsOS_scnprintf(str, end - str, "DRVCMDQ_CONFIG    (E)         0x%x\n",gbdbgmessage[EN_DBGMG_DRVCMDQ_CONFIG]);
    str += MsOS_scnprintf(str, end - str, "DRVVIP_CONFIG     (F)         0x%x\n",gbdbgmessage[EN_DBGMG_DRVVIP_CONFIG]);
    str += MsOS_scnprintf(str, end - str, "PRIORITY_CONFIG   (G)         0x%x\n",gbdbgmessage[EN_DBGMG_PRIORITY_CONFIG]);
    str += MsOS_scnprintf(str, end - str, "ALL Reset         (0)\n");
    str += MsOS_scnprintf(str, end - str, "========================SCL Debug Message BUTTON======================\n");
    str += MsOS_scnprintf(str, end - str, "========================SCL Debug Message LEVEL======================\n");
    str += MsOS_scnprintf(str, end - str, "default is level 1\n");
    str += MsOS_scnprintf(str, end - str, "-------------------------------IOCTL LEVEL---------------------------\n");
    str += MsOS_scnprintf(str, end - str, "0x 1 : SC1\n");
    str += MsOS_scnprintf(str, end - str, "0x 2 : SC2\n");
    str += MsOS_scnprintf(str, end - str, "0x 4 : SC3\n");
    str += MsOS_scnprintf(str, end - str, "0x 8 : VIP\n");
    str += MsOS_scnprintf(str, end - str, "0x10 : SC1HLEVEL\n");
    str += MsOS_scnprintf(str, end - str, "0x20 : SC2HLEVEL\n");
    str += MsOS_scnprintf(str, end - str, "0x40 : LCD\n");
    str += MsOS_scnprintf(str, end - str, "0x80 : ELSE\n");
    str += MsOS_scnprintf(str, end - str, "-------------------------------HVSP LEVEL---------------------------\n");
    str += MsOS_scnprintf(str, end - str, "0x 1 : HVSP1\n");
    str += MsOS_scnprintf(str, end - str, "0x 2 : HVSP2\n");
    str += MsOS_scnprintf(str, end - str, "0x 4 : HVSP3\n");
    str += MsOS_scnprintf(str, end - str, "0x 8 : ELSE\n");
    str += MsOS_scnprintf(str, end - str, "-------------------------------SCLDMA LEVEL-------------------------\n");
    str += MsOS_scnprintf(str, end - str, "0x 1 : SC1 FRM\n");
    str += MsOS_scnprintf(str, end - str, "0x 2 : SC1 SNP \n");
    str += MsOS_scnprintf(str, end - str, "0x 4 : SC2 FRM\n");
    str += MsOS_scnprintf(str, end - str, "0x 8 : SC3 FRM\n");
    str += MsOS_scnprintf(str, end - str, "0x10 : SC1 FRM HL\n");
    str += MsOS_scnprintf(str, end - str, "0x20 : SC1 SNP HL\n");
    str += MsOS_scnprintf(str, end - str, "0x40 : SC2 FRM HL\n");
    str += MsOS_scnprintf(str, end - str, "0x80 : ELSE\n");
    str += MsOS_scnprintf(str, end - str, "-------------------------------VIP LEVEL(IOlevel)-------------------\n");
    str += MsOS_scnprintf(str, end - str, "0x 1 : NORMAL\n");
    str += MsOS_scnprintf(str, end - str, "0x 2 : VIP LOG \n");
    str += MsOS_scnprintf(str, end - str, "0x 4 : VIP SUSPEND\n");
    str += MsOS_scnprintf(str, end - str, "0x 8 : ELSE\n");
    str += MsOS_scnprintf(str, end - str, "------------------------------MULTI LEVEL---------------------------\n");
    str += MsOS_scnprintf(str, end - str, "0x 1 : SC1\n");
    str += MsOS_scnprintf(str, end - str, "0x 2 : SC3\n");
    str += MsOS_scnprintf(str, end - str, "0x 4 : DISP\n");
    str += MsOS_scnprintf(str, end - str, "0x 8 : ELSE\n");
    str += MsOS_scnprintf(str, end - str, "-------------------------------PQ LEVEL---------------------------\n");
    str += MsOS_scnprintf(str, end - str, "0x 1 : befor crop\n");
    str += MsOS_scnprintf(str, end - str, "0x 2 : color eng\n");
    str += MsOS_scnprintf(str, end - str, "0x 4 : VIP Y\n");
    str += MsOS_scnprintf(str, end - str, "0x 8 : VIP C\n");
    str += MsOS_scnprintf(str, end - str, "0x10 : AIP\n");
    str += MsOS_scnprintf(str, end - str, "0x20 : AIP post\n");
    str += MsOS_scnprintf(str, end - str, "0x40 : HVSP\n");
    str += MsOS_scnprintf(str, end - str, "0x80 : ELSE\n");
    str += MsOS_scnprintf(str, end - str, "-------------------------------SCLIRQ LEVEL(drvlevel)---------------\n");
    str += MsOS_scnprintf(str, end - str, "0x 1 : NORMAL\n");
    str += MsOS_scnprintf(str, end - str, "0x 2 : SC1RINGA \n");
    str += MsOS_scnprintf(str, end - str, "0x 4 : SC1RINGN\n");
    str += MsOS_scnprintf(str, end - str, "0x 8 : SC1SINGLE\n");
    str += MsOS_scnprintf(str, end - str, "0x10 : SC2RINGA\n");
    str += MsOS_scnprintf(str, end - str, "0x20 : SC2RINGN \n");
    str += MsOS_scnprintf(str, end - str, "0x40 : SC3SINGLE\n");
    str += MsOS_scnprintf(str, end - str, "0x80 : ELSE\n");
    str += MsOS_scnprintf(str, end - str, "-------------------------------CMDQ LEVEL(drvlevel)-----------------\n");
    str += MsOS_scnprintf(str, end - str, "0x 1 : LOW\n");
    str += MsOS_scnprintf(str, end - str, "0x 2 : NORMAL \n");
    str += MsOS_scnprintf(str, end - str, "0x 4 : HIGH\n");
    str += MsOS_scnprintf(str, end - str, "0x 8 : ISR\n");
    str += MsOS_scnprintf(str, end - str, "0x10 : ISR check\n");
    str += MsOS_scnprintf(str, end - str, "0x10 : SRAM check\n");
    str += MsOS_scnprintf(str, end - str, "\n");
    str += MsOS_scnprintf(str, end - str, "========================SCL Debug Message LEVEL======================\n");
    return (str - buf);
}
ssize_t MDrv_HVSP_IntsShow(char *buf)
{
    char *str = buf;
    char *end = buf + PAGE_SIZE;
    ST_HVSP_SCINTS_TYPE stInts;
    unsigned long u32val;
    MsOS_Memset(&stInts,0,sizeof(ST_HVSP_SCINTS_TYPE));
    Drv_HVSP_GetSCLInts(&stInts);
    str += MsOS_scnprintf(str, end - str, "========================SCL INTS======================\n");
    str += MsOS_scnprintf(str, end - str, "AFF          Count: %lu\n",stInts.u32AffCount);
    str += MsOS_scnprintf(str, end - str, "DMAERROR     Count: %lu\n",stInts.u32ErrorCount);
    str += MsOS_scnprintf(str, end - str, "ISPIn        Count: %lu\n",stInts.u32ISPInCount);
    str += MsOS_scnprintf(str, end - str, "ISPDone      Count: %lu\n",stInts.u32ISPDoneCount);
    str += MsOS_scnprintf(str, end - str, "DIFF         Count: %lu\n",(stInts.u32ISPInCount-stInts.u32ISPDoneCount));
    str += MsOS_scnprintf(str, end - str, "ResetCount   Count: %hhu\n",stInts.u8CountReset);
    str += MsOS_scnprintf(str, end - str, "SC1FrmDone   Count: %lu\n",stInts.u32SC1FrmDoneCount);
    str += MsOS_scnprintf(str, end - str, "SC1SnpDone   Count: %lu\n",stInts.u32SC1SnpDoneCount);
    str += MsOS_scnprintf(str, end - str, "SC2FrmDone   Count: %lu\n",stInts.u32SC2FrmDoneCount);
    str += MsOS_scnprintf(str, end - str, "SC2Frm2Done  Count: %lu\n",stInts.u32SC2Frm2DoneCount);
    str += MsOS_scnprintf(str, end - str, "SC3Done      Count: %lu\n",stInts.u32SC3DoneCount);
    str += MsOS_scnprintf(str, end - str, "SCLMainDone  Count: %lu\n",stInts.u32SCLMainDoneCount);
    if(stInts.u32SC1FrmDoneCount)
    {
        u32val = (unsigned long)(stInts.u32SC1FrmActiveTime/stInts.u32SC1FrmDoneCount);
    }
    else
    {
        u32val = stInts.u32SC1FrmActiveTime;
    }
    str += MsOS_scnprintf(str, end - str, "SC1Frm       ActiveTime: %lu (us)\n",u32val);
    if(stInts.u32SC1SnpDoneCount)
    {
        u32val = (unsigned long)(stInts.u32SC1SnpActiveTime/stInts.u32SC1SnpDoneCount);
    }
    else
    {
        u32val = stInts.u32SC1SnpActiveTime;
    }
    str += MsOS_scnprintf(str, end - str, "SC1Snp       ActiveTime: %lu (us)\n",u32val);
    if(stInts.u32SC2FrmDoneCount)
    {
        u32val = (unsigned long)(stInts.u32SC2FrmActiveTime/stInts.u32SC2FrmDoneCount);
    }
    else
    {
        u32val = stInts.u32SC2FrmDoneCount;
    }
    str += MsOS_scnprintf(str, end - str, "SC2Frm       ActiveTime: %lu (us)\n",u32val);
    if(stInts.u32SC2Frm2DoneCount)
    {
        u32val = (unsigned long)(stInts.u32SC2Frm2ActiveTime/stInts.u32SC2Frm2DoneCount);
    }
    else
    {
        u32val = stInts.u32SC2Frm2DoneCount;
    }
    str += MsOS_scnprintf(str, end - str, "SC2Frm2      ActiveTime: %lu (us)\n",u32val);
    if(stInts.u32SC3DoneCount)
    {
        u32val = (unsigned long)(stInts.u32SC3ActiveTime/stInts.u32SC3DoneCount);
    }
    else
    {
        u32val = stInts.u32SC3DoneCount;
    }
    str += MsOS_scnprintf(str, end - str, "SC3          ActiveTime: %lu (us)\n",u32val);
    if(stInts.u32SCLMainDoneCount)
    {
        u32val = (unsigned long)(stInts.u32SCLMainActiveTime/stInts.u32SCLMainDoneCount);
    }
    else
    {
        u32val = stInts.u32SCLMainActiveTime;
    }
    str += MsOS_scnprintf(str, end - str, "SCLMain       ActiveTime: %lu (us)\n",u32val);
    if(stInts.u32ISPDoneCount)
    {
        u32val = (unsigned long)(stInts.u32ISPTime/stInts.u32ISPDoneCount);
    }
    else
    {
        u32val = stInts.u32ISPTime;
    }
    str += MsOS_scnprintf(str, end - str, "ISP       ActiveTime: %lu (us)\n",u32val);
    str += MsOS_scnprintf(str, end - str, "ISP       BlankingTime: %lu (us)\n",stInts.u32ISPBlankingTime);
    str += MsOS_scnprintf(str, end - str, "========================SCL INTS======================\n");
    return (str - buf);
}
void MDrv_HVSP_SCIQStore(const char *buf,EN_MDRV_HVSP_ID_TYPE enHVSP_ID)
{
    const char *str = buf;
    EN_HVSP_ID_TYPE enID;
    enID = enHVSP_ID == E_MDRV_HVSP_ID_2 ? E_HVSP_ID_2 :
           enHVSP_ID == E_MDRV_HVSP_ID_3 ? E_HVSP_ID_3 :
                                           E_HVSP_ID_1;
    if(NULL!=buf)
    {
        //if(!)
        if((int)*str == 49)    //input 1
        {
            Drv_HVSP_SCLIQ(enID,EN_DRV_HVSP_IQ_H_Tbl0);
            SCL_ERR( "[SCLIQ]Set %d\n",(int)*str);
        }
        else if((int)*str == 50)  //input 2
        {
            Drv_HVSP_SCLIQ(enID,EN_DRV_HVSP_IQ_H_Tbl1);
            SCL_ERR( "[SCLIQ]Set %d\n",(int)*str);
        }
        else if((int)*str == 51)  //input 3
        {
            Drv_HVSP_SCLIQ(enID,EN_DRV_HVSP_IQ_H_Tbl2);
            SCL_ERR( "[SCLIQ]Set %d\n",(int)*str);
        }
        else if((int)*str == 52)  //input 4
        {
            Drv_HVSP_SCLIQ(enID,EN_DRV_HVSP_IQ_H_Tbl3);
            SCL_ERR( "[SCLIQ]Set %d\n",(int)*str);
        }
        else if((int)*str == 53)  //input 5
        {
            Drv_HVSP_SCLIQ(enID,EN_DRV_HVSP_IQ_H_BYPASS);
            SCL_ERR( "[SCLIQ]Set %d\n",(int)*str);
        }
        else if((int)*str == 54)  //input 6
        {
            Drv_HVSP_SCLIQ(enID,EN_DRV_HVSP_IQ_H_BILINEAR);
            SCL_ERR( "[SCLIQ]Set %d\n",(int)*str);
        }
        else if((int)*str == 55)  //input 7
        {
            Drv_HVSP_SCLIQ(enID,EN_DRV_HVSP_IQ_V_Tbl0);
            SCL_ERR( "[SCLIQ]Set %d\n",(int)*str);
        }
        else if((int)*str == 56)  //input 8
        {
            Drv_HVSP_SCLIQ(enID,EN_DRV_HVSP_IQ_V_Tbl1);
            SCL_ERR( "[SCLIQ]Set %d\n",(int)*str);
        }
        else if((int)*str == 57)  //input 9
        {
            Drv_HVSP_SCLIQ(enID,EN_DRV_HVSP_IQ_V_Tbl2);
            SCL_ERR( "[SCLIQ]Set %d\n",(int)*str);
        }
        else if((int)*str == 65)  //input A
        {
            Drv_HVSP_SCLIQ(enID,EN_DRV_HVSP_IQ_V_Tbl3);
            SCL_ERR( "[SCLIQ]Set %d\n",(int)*str);
        }
        else if((int)*str == 66)  //input B
        {
            Drv_HVSP_SCLIQ(enID,EN_DRV_HVSP_IQ_V_BYPASS);
            SCL_ERR( "[SCLIQ]Set %d\n",(int)*str);
        }
        else if((int)*str == 67)  //input C
        {
            Drv_HVSP_SCLIQ(enID,EN_DRV_HVSP_IQ_V_BILINEAR);
            SCL_ERR( "[SCLIQ]Set %d\n",(int)*str);
        }
    }
}
ssize_t MDrv_HVSP_SCIQShow(char *buf, EN_MDRV_HVSP_ID_TYPE enHVSP_ID)
{
    char *str = buf;
    char *end = buf + PAGE_SIZE;
    ST_DRV_HVSP_HVSPINFORM_CONFIG sthvspformCfg;
    MsOS_Memset(&sthvspformCfg,0,sizeof(ST_DRV_HVSP_HVSPINFORM_CONFIG));
    Drv_HVSP_GetHVSPAttribute((EN_HVSP_ID_TYPE)enHVSP_ID,&sthvspformCfg);
    str += MsOS_scnprintf(str, end - str, "------------------------SCL IQ----------------------\n");
    str += MsOS_scnprintf(str, end - str, "H en  :%hhx\n",sthvspformCfg.bEn&0x1);
    str += MsOS_scnprintf(str, end - str, "H function  :%hhx\n",(sthvspformCfg.bEn&0xC0)>>6);
    str += MsOS_scnprintf(str, end - str, "V en  :%hhx\n",(sthvspformCfg.bEn&0x2)>>1);
    str += MsOS_scnprintf(str, end - str, "V function  :%hhx\n",(sthvspformCfg.bEn&0x30)>>4);
    str += MsOS_scnprintf(str, end - str, "SC H   :1~6\n");
    str += MsOS_scnprintf(str, end - str, "SC table 0   :1\n");
    str += MsOS_scnprintf(str, end - str, "SC table 1   :2\n");
    str += MsOS_scnprintf(str, end - str, "SC table 2   :3\n");
    str += MsOS_scnprintf(str, end - str, "SC table 3   :4\n");
    str += MsOS_scnprintf(str, end - str, "SC bypass    :5\n");
    str += MsOS_scnprintf(str, end - str, "SC bilinear  :6\n");
    str += MsOS_scnprintf(str, end - str, "SC V :7~C\n");
    str += MsOS_scnprintf(str, end - str, "SC table 0   :7\n");
    str += MsOS_scnprintf(str, end - str, "SC table 1   :8\n");
    str += MsOS_scnprintf(str, end - str, "SC table 2   :9\n");
    str += MsOS_scnprintf(str, end - str, "SC table 3   :A\n");
    str += MsOS_scnprintf(str, end - str, "SC bypass    :B\n");
    str += MsOS_scnprintf(str, end - str, "SC bilinear  :C\n");
    str += MsOS_scnprintf(str, end - str, "------------------------SCL IQ----------------------\n");
    return (str - buf);
}
ssize_t MDrv_HVSP_ProcShow(char *buf)
{
    char *str = buf;
    char *end = buf + PAGE_SIZE;
    ST_DRV_HVSP_HVSPINFORM_CONFIG sthvspformCfg;
    ST_DRV_HVSP_HVSPINFORM_CONFIG sthvsp2formCfg;
    ST_DRV_HVSP_HVSPINFORM_CONFIG sthvsp3formCfg;
    ST_DRV_HVSP_SCINFORM_CONFIG stzoomformCfg;
    ST_HVSP_IPM_CONFIG stIpmformCfg;
    ST_DRV_HVSP_INPUTINFORM_CONFIG stInformCfg;
    unsigned char bLDCorPrvCrop;
    ST_DRV_HVSP_OSD_CONFIG stOsdCfg;
    Drv_HVSP_GetCrop12Inform(&stInformCfg);
    Drv_HVSP_GetSCLInform(E_HVSP_ID_1,&stzoomformCfg);
    Drv_HVSP_GetHVSPAttribute(E_HVSP_ID_1,&sthvspformCfg);
    Drv_HVSP_GetHVSPAttribute(E_HVSP_ID_2,&sthvsp2formCfg);
    Drv_HVSP_GetHVSPAttribute(E_HVSP_ID_3,&sthvsp3formCfg);
    Drv_HVSP_GetOSDAttribute(E_HVSP_ID_1,&stOsdCfg);
    bLDCorPrvCrop = Drv_HVSP_GetFrameBufferAttribute(E_HVSP_ID_1,&stIpmformCfg);
    str += MsOS_scnprintf(str, end - str, "========================SCL PROC FRAMEWORK======================\n");
    str += MsOS_scnprintf(str, end - str, "\n");
    str += MsOS_scnprintf(str, end - str, "------------------------SCL INPUT MUX----------------------\n");
    if(stInformCfg.enMux==0)
    {
        str += MsOS_scnprintf(str, end - str, "Input SRC :BT656\n");
    }
    else if(stInformCfg.enMux==1)
    {
        str += MsOS_scnprintf(str, end - str, "Input SRC :ISP\n");
    }
    else if(stInformCfg.enMux==3)
    {
        str += MsOS_scnprintf(str, end - str, "Input SRC :PTGEN\n");
    }
    else
    {
        str += MsOS_scnprintf(str, end - str, "Input SRC :OTHER\n");
    }
    str += MsOS_scnprintf(str, end - str, "Input H   :%hd\n",stInformCfg.u16inWidth);
    str += MsOS_scnprintf(str, end - str, "Input V   :%hd\n",stInformCfg.u16inHeight);
    str += MsOS_scnprintf(str, end - str, "Receive H :%hd\n",stInformCfg.u16inWidthcount);
    str += MsOS_scnprintf(str, end - str, "Receive V :%hd\n",stInformCfg.u16inHeightcount);
    str += MsOS_scnprintf(str, end - str, "------------------------SCL FB-----------------------------\n");
    str += MsOS_scnprintf(str, end - str, "FB H          :%hd\n",stIpmformCfg.u16Fetch);
    str += MsOS_scnprintf(str, end - str, "FB V          :%hd\n",stIpmformCfg.u16Vsize);
    str += MsOS_scnprintf(str, end - str, "FB Addr       :%lx\n",stIpmformCfg.u32YCBaseAddr);
    str += MsOS_scnprintf(str, end - str, "FB memsize    :%ld\n",stIpmformCfg.u32MemSize);
    str += MsOS_scnprintf(str, end - str, "FB Buffer     :%hhd\n",gu8IPMBufferNum);
    str += MsOS_scnprintf(str, end - str, "FB Write      :%hhd\n",stIpmformCfg.bYCMWrite);
    if(bLDCorPrvCrop &0x1)
    {
        str += MsOS_scnprintf(str, end - str, "READ PATH     :LDC\n");
    }
    else if(bLDCorPrvCrop &0x2)
    {
        str += MsOS_scnprintf(str, end - str, "READ PATH     :PrvCrop\n");
    }
    else if(stIpmformCfg.bYCMRead)
    {
        str += MsOS_scnprintf(str, end - str, "READ PATH     :MCNR\n");
    }
    else
    {
        str += MsOS_scnprintf(str, end - str, "READ PATH     :NONE\n");
    }
    str += MsOS_scnprintf(str, end - str, "FRAME DELAY     :%hhd\n",MsOS_GetSCLFrameDelay());
    str += MsOS_scnprintf(str, end - str, "------------------------SCL Crop----------------------------\n");
    str += MsOS_scnprintf(str, end - str, "Crop      :%hhd\n",stInformCfg.bEn);
    str += MsOS_scnprintf(str, end - str, "CropX     :%hd\n",stInformCfg.u16inCropX);
    str += MsOS_scnprintf(str, end - str, "CropY     :%hd\n",stInformCfg.u16inCropY);
    str += MsOS_scnprintf(str, end - str, "CropOutW  :%hd\n",stInformCfg.u16inCropWidth);
    str += MsOS_scnprintf(str, end - str, "CropOutH  :%hd\n",stInformCfg.u16inCropHeight);
    str += MsOS_scnprintf(str, end - str, "SrcW      :%hd\n",stInformCfg.u16inWidth);
    str += MsOS_scnprintf(str, end - str, "SrcH      :%hd\n",stInformCfg.u16inHeight);
    str += MsOS_scnprintf(str, end - str, "------------------------SCL Zoom----------------------------\n");
    str += MsOS_scnprintf(str, end - str, "Zoom      :%hhd\n",stzoomformCfg.bEn);
    str += MsOS_scnprintf(str, end - str, "ZoomX     :%hd\n",stzoomformCfg.u16X);
    str += MsOS_scnprintf(str, end - str, "ZoomY     :%hd\n",stzoomformCfg.u16Y);
    str += MsOS_scnprintf(str, end - str, "ZoomOutW  :%hd\n",stzoomformCfg.u16crop2OutWidth);
    str += MsOS_scnprintf(str, end - str, "ZoomOutH  :%hd\n",stzoomformCfg.u16crop2OutHeight);
    str += MsOS_scnprintf(str, end - str, "SrcW      :%hd\n",stzoomformCfg.u16crop2inWidth);
    str += MsOS_scnprintf(str, end - str, "SrcH      :%hd\n",stzoomformCfg.u16crop2inHeight);
    str += MsOS_scnprintf(str, end - str, "------------------------SCL OSD----------------------------\n");
    str += MsOS_scnprintf(str, end - str, "ONOFF     :%hhd\n",stOsdCfg.stOsdOnOff.bOSDEn);
    if(stOsdCfg.enOSD_loc)
    {
        str += MsOS_scnprintf(str, end - str, "Locate: Before\n");
    }
    else
    {
        str += MsOS_scnprintf(str, end - str, "Locate: After\n");
    }
    str += MsOS_scnprintf(str, end - str, "------------------------SCL HVSP1----------------------------\n");
    str += MsOS_scnprintf(str, end - str, "InputH    :%hd\n",sthvspformCfg.u16inWidth);
    str += MsOS_scnprintf(str, end - str, "InputV    :%hd\n",sthvspformCfg.u16inHeight);
    str += MsOS_scnprintf(str, end - str, "OutputH   :%hd\n",sthvspformCfg.u16Width);
    str += MsOS_scnprintf(str, end - str, "OutputV   :%hd\n",sthvspformCfg.u16Height);
    str += MsOS_scnprintf(str, end - str, "H en  :%hhx\n",sthvspformCfg.bEn&0x1);
    str += MsOS_scnprintf(str, end - str, "H function  :%hhx\n",(sthvspformCfg.bEn&0xC0)>>6);
    str += MsOS_scnprintf(str, end - str, "V en  :%hhx\n",(sthvspformCfg.bEn&0x2)>>1);
    str += MsOS_scnprintf(str, end - str, "V function  :%hhx\n",(sthvspformCfg.bEn&0x30)>>4);
    str += MsOS_scnprintf(str, end - str, "------------------------SCL HVSP2----------------------------\n");
    str += MsOS_scnprintf(str, end - str, "InputH    :%hd\n",sthvsp2formCfg.u16inWidth);
    str += MsOS_scnprintf(str, end - str, "InputV    :%hd\n",sthvsp2formCfg.u16inHeight);
    str += MsOS_scnprintf(str, end - str, "OutputH   :%hd\n",sthvsp2formCfg.u16Width);
    str += MsOS_scnprintf(str, end - str, "OutputV   :%hd\n",sthvsp2formCfg.u16Height);
    str += MsOS_scnprintf(str, end - str, "H en  :%hhx\n",sthvsp2formCfg.bEn&0x1);
    str += MsOS_scnprintf(str, end - str, "H function  :%hhx\n",(sthvsp2formCfg.bEn&0xC0)>>6);
    str += MsOS_scnprintf(str, end - str, "V en  :%hhx\n",(sthvsp2formCfg.bEn&0x2)>>1);
    str += MsOS_scnprintf(str, end - str, "V function  :%hhx\n",(sthvsp2formCfg.bEn&0x30)>>4);
    str += MsOS_scnprintf(str, end - str, "------------------------SCL HVSP3----------------------------\n");
    str += MsOS_scnprintf(str, end - str, "InputH    :%hd\n",sthvsp3formCfg.u16inWidth);
    str += MsOS_scnprintf(str, end - str, "InputV    :%hd\n",sthvsp3formCfg.u16inHeight);
    str += MsOS_scnprintf(str, end - str, "OutputH   :%hd\n",sthvsp3formCfg.u16Width);
    str += MsOS_scnprintf(str, end - str, "OutputV   :%hd\n",sthvsp3formCfg.u16Height);
    str += MsOS_scnprintf(str, end - str, "H en  :%hhx\n",sthvsp3formCfg.bEn&0x1);
    str += MsOS_scnprintf(str, end - str, "H function  :%hhx\n",(sthvsp3formCfg.bEn&0xC0)>>6);
    str += MsOS_scnprintf(str, end - str, "V en  :%hhx\n",(sthvsp3formCfg.bEn&0x2)>>1);
    str += MsOS_scnprintf(str, end - str, "V function  :%hhx\n",(sthvsp3formCfg.bEn&0x30)>>4);
    str += MsOS_scnprintf(str, end - str, "\n");
    str += MsOS_scnprintf(str, end - str, "========================SCL PROC FRAMEWORK======================\n");
    return (str - buf);
}
ssize_t MDrv_HVSP_ClkFrameworkShow(char *buf,ST_MDRV_HVSP_CLK_CONFIG* stclk)
{
    char *str = buf;
    char *end = buf + PAGE_SIZE;
    str += MsOS_scnprintf(str, end - str, "========================SCL CLK FRAMEWORK======================\n");
    str += MsOS_scnprintf(str, end - str, "echo 1 > clk :open force mode\n");
    str += MsOS_scnprintf(str, end - str, "echo 0 > clk :close force mode\n");
    str += MsOS_scnprintf(str, end - str, "echo 2 > clk :fclk1 172\n");
    str += MsOS_scnprintf(str, end - str, "echo 3 > clk :fclk1 86\n");
    str += MsOS_scnprintf(str, end - str, "echo E > clk :fclk1 216\n");
    str += MsOS_scnprintf(str, end - str, "echo 4 > clk :fclk1 open\n");
    str += MsOS_scnprintf(str, end - str, "echo 5 > clk :fclk1 close\n");
    str += MsOS_scnprintf(str, end - str, "echo 6 > clk :fclk2 172\n");
    str += MsOS_scnprintf(str, end - str, "echo 7 > clk :fclk2 86\n");
    str += MsOS_scnprintf(str, end - str, "echo F > clk :fclk2 216\n");
    str += MsOS_scnprintf(str, end - str, "echo 8 > clk :fclk2 open\n");
    str += MsOS_scnprintf(str, end - str, "echo 9 > clk :fclk2 close\n");
    str += MsOS_scnprintf(str, end - str, "echo : > clk :idclk ISP\n");
    str += MsOS_scnprintf(str, end - str, "echo D > clk :idclk BT656\n");
    str += MsOS_scnprintf(str, end - str, "echo B > clk :idclk open\n");
    str += MsOS_scnprintf(str, end - str, "echo = > clk :idclk close\n");
    str += MsOS_scnprintf(str, end - str, "echo C > clk :odclk MAX\n");
    str += MsOS_scnprintf(str, end - str, "echo ? > clk :odclk LPLL\n");
    str += MsOS_scnprintf(str, end - str, "echo @ > clk :odclk open\n");
    str += MsOS_scnprintf(str, end - str, "echo A > clk :odclk close\n");
    str += MsOS_scnprintf(str, end - str, "========================SCL CLK STATUS======================\n");
    str += MsOS_scnprintf(str, end - str, "force mode :%hhd\n",Drv_HVSP_GetCLKForcemode());
    if(MsOS_clk_get_enable_count((MSOS_ST_CLK*)stclk->fclk1))
    {
        str += MsOS_scnprintf(str, end - str, "fclk1 open :%d ,%ld\n",
            MsOS_clk_get_enable_count((MSOS_ST_CLK*)stclk->fclk1),MsOS_clk_get_rate((MSOS_ST_CLK*)stclk->fclk1));
    }
    else
    {
        str += MsOS_scnprintf(str, end - str, "fclk1 close\n");
    }
    if(MsOS_clk_get_enable_count((MSOS_ST_CLK*)stclk->fclk2))
    {
        str += MsOS_scnprintf(str, end - str, "fclk2 open :%d,%ld\n",
            MsOS_clk_get_enable_count((MSOS_ST_CLK*)stclk->fclk2),MsOS_clk_get_rate((MSOS_ST_CLK*)stclk->fclk2));
    }
    else
    {
        str += MsOS_scnprintf(str, end - str, "fclk2 close\n");
    }
    if(MsOS_clk_get_enable_count((MSOS_ST_CLK*)stclk->idclk))
    {
        if(MsOS_clk_get_rate((MSOS_ST_CLK*)stclk->idclk) > 10)
        {
            str += MsOS_scnprintf(str, end - str, "idclk open :ISP\n");
        }
        else if(MsOS_clk_get_rate((MSOS_ST_CLK*)stclk->idclk) == 1)
        {
            str += MsOS_scnprintf(str, end - str, "idclk open :BT656\n");
        }
    }
    else
    {
        str += MsOS_scnprintf(str, end - str, "idclk close\n");
    }
    if(MsOS_clk_get_enable_count((MSOS_ST_CLK*)stclk->odclk))
    {
        if(MsOS_clk_get_rate((MSOS_ST_CLK*)stclk->odclk) == 432000000)
        {
            str += MsOS_scnprintf(str, end - str, "odclk open LPLL:%ld\n",((MS_U32)Drv_PNL_GetLPLLDclk()/10000)*10000);
        }
        else
        {
            str += MsOS_scnprintf(str, end - str, "odclk open :%ld\n",MsOS_clk_get_rate(stclk->odclk));
        }
    }
    else if(Drv_PNL_GetPnlOpen())
    {
        str += MsOS_scnprintf(str, end - str, "odclk manual open LPLL:%ld\n",((MS_U32)Drv_PNL_GetLPLLDclk()/10000)*10000);
    }
    else
    {
        str += MsOS_scnprintf(str, end - str, "odclk close\n");
    }
    return (str - buf);
}
ssize_t MDrv_HVSP_FBMGShow(char *buf)
{
    char *str = buf;
    char *end = buf + PAGE_SIZE;
    str += MsOS_scnprintf(str, end - str, "------------------------SCL FBMG----------------------------\n");
    str += MsOS_scnprintf(str, end - str, "LDCPATH_ON    :1\n");
    str += MsOS_scnprintf(str, end - str, "LDCPATH_OFF   :2\n");
    str += MsOS_scnprintf(str, end - str, "DNRRead_ON    :3\n");
    str += MsOS_scnprintf(str, end - str, "DNRRead_OFF   :4\n");
    str += MsOS_scnprintf(str, end - str, "DNRWrite_ON   :5\n");
    str += MsOS_scnprintf(str, end - str, "DNRWrite_OFF  :6\n");
    str += MsOS_scnprintf(str, end - str, "DNRBuf1       :7\n");
    str += MsOS_scnprintf(str, end - str, "DNRBuf2       :8\n");
    str += MsOS_scnprintf(str, end - str, "UNLOCK        :9\n");
    str += MsOS_scnprintf(str, end - str, "PrvCrop_ON    :A\n");
    str += MsOS_scnprintf(str, end - str, "PrvCrop_OFF   :B\n");
    str += MsOS_scnprintf(str, end - str, "CIIR_ON       :C\n");
    str += MsOS_scnprintf(str, end - str, "CIIR_OFF      :D\n");
    str += MsOS_scnprintf(str, end - str, "LOCK          :E\n");
    str += MsOS_scnprintf(str, end - str, "------------------------SCL FBMG----------------------------\n");
    return (str - buf);
}
ssize_t MDrv_HVSP_OdShow(char *buf)
{
    char *str = buf;
    char *end = buf + PAGE_SIZE;
    unsigned char u8idx = 0;
    unsigned char u8idy = 0;
    unsigned char u8idz = 0;
    unsigned short u16val = 0;
    str += MsOS_scnprintf(str, end - str, "------------------------SCL OD----------------------------\n");
    str += MsOS_scnprintf(str, end - str,
        "------0---------320-------640-------960-------1280------1600------1920------2240------2560\n");
    for(u8idy = 0;u8idy<=(gstHvspScalingCfg.stCropWin.u16Height/32)+1;u8idy++)
    {
        if(u8idy==10)
        {
            str += MsOS_scnprintf(str, end - str, "|320  ");
        }
        else if(u8idy==20)
        {
            str += MsOS_scnprintf(str, end - str, "|640  ");
        }
        else if(u8idy==30)
        {
            str += MsOS_scnprintf(str, end - str, "|960  ");
        }
        else if(u8idy==40)
        {
            str += MsOS_scnprintf(str, end - str, "|1280 ");
        }
        else if(u8idy==50)
        {
            str += MsOS_scnprintf(str, end - str, "|1600 ");
        }
        else
        {
            str += MsOS_scnprintf(str, end - str, "|     ");
        }
        for(u8idx =0;u8idx<=(gstHvspScalingCfg.stCropWin.u16Width/(16*32));u8idx++)
        {
            u16val = 0;
            u16val = Drv_HVSP_PriMaskGetSRAM(u8idx, u8idy);
            for(u8idz = 0;u8idz<16;u8idz++)
            {
                if(u16val &(0x1 <<u8idz))
                {
                    str += MsOS_scnprintf(str, end - str, "1");
                }
                else
                {
                    str += MsOS_scnprintf(str, end - str, "0");
                }
            }
        }
        str += MsOS_scnprintf(str, end - str, "\n");
    }
    str += MsOS_scnprintf(str, end - str, "------------------------SCL OD----------------------------\n");
    return (str - buf);
}
ssize_t MDrv_HVSP_OsdShow(char *buf)
{
    char *str = buf;
    char *end = buf + PAGE_SIZE;
    unsigned char idx;
    ST_DRV_HVSP_OSD_CONFIG stOsdCfg;
    Drv_HVSP_GetOSDAttribute(E_HVSP_ID_1,&stOsdCfg);
    str += MsOS_scnprintf(str, end - str, "------------------------SCL OSD----------------------------\n");
    str += MsOS_scnprintf(str, end - str, "ONOFF     :%hhd\n",stOsdCfg.stOsdOnOff.bOSDEn);
    if(stOsdCfg.enOSD_loc)
    {
        str += MsOS_scnprintf(str, end - str, "Locate: Before\n");
    }
    else
    {
        str += MsOS_scnprintf(str, end - str, "Locate: After\n");
    }
    str += MsOS_scnprintf(str, end - str, "Bypass    :%hhd\n",stOsdCfg.stOsdOnOff.bOSDBypass);
    str += MsOS_scnprintf(str, end - str, "WTM Bypass:%hhd\n",stOsdCfg.stOsdOnOff.bWTMBypass);
    str += MsOS_scnprintf(str, end - str, "------------------------SCL OD----------------------------\n");
    if(gu8PriMaskNum[gu8PriMaskInst])
    {
        str += MsOS_scnprintf(str, end - str, "ONOFF     :ON @:%hhd\n",gu8PriMaskInst);
        str += MsOS_scnprintf(str, end - str, "W RATIO   :%ld\n",
            (MS_U32)CAL_HVSP_RATIO(gstHvspScalingCfg.stCropWin.u16Width,gstHvspScalingCfg.u16Dsp_Width));
        str += MsOS_scnprintf(str, end - str, "H RATIO   :%ld\n",
            (MS_U32)CAL_HVSP_RATIO(gstHvspScalingCfg.stCropWin.u16Height,gstHvspScalingCfg.u16Dsp_Height));
        for(idx=0;idx<gu8PriMaskNum[gu8PriMaskInst];idx++)
        {
            if(gstHvspPriMaskCfg[gu8PriMaskInst][idx].u8idx == idx)
            {
                str += MsOS_scnprintf(str, end - str, "IDX     :%hhd, ON:%hhd ,(%hd,%hd,%hd,%hd)\n",
                    idx,gstHvspPriMaskCfg[gu8PriMaskInst][idx].bMask,gstHvspPriMaskCfg[gu8PriMaskInst][idx].stMaskWin.u16X
                    ,gstHvspPriMaskCfg[gu8PriMaskInst][idx].stMaskWin.u16Y,gstHvspPriMaskCfg[gu8PriMaskInst][idx].stMaskWin.u16Width
                    ,gstHvspPriMaskCfg[gu8PriMaskInst][idx].stMaskWin.u16Height);
            }
        }
    }
    else
    {
        str += MsOS_scnprintf(str, end - str, "ONOFF     :OFF\n");
    }
    str += MsOS_scnprintf(str, end - str, "------------------------SCL OSD----------------------------\n");
    str += MsOS_scnprintf(str, end - str, "echo 1 > OSD :open OSD\n");
    str += MsOS_scnprintf(str, end - str, "echo 0 > OSD :close OSD\n");
    str += MsOS_scnprintf(str, end - str, "echo 2 > OSD :Set OSD before\n");
    str += MsOS_scnprintf(str, end - str, "echo 3 > OSD :Set OSD After\n");
    str += MsOS_scnprintf(str, end - str, "echo 4 > OSD :Set OSD Bypass\n");
    str += MsOS_scnprintf(str, end - str, "echo 5 > OSD :Set OSD Bypass Off\n");
    str += MsOS_scnprintf(str, end - str, "echo 6 > OSD :Set OSD WTM Bypass\n");
    str += MsOS_scnprintf(str, end - str, "echo 7 > OSD :Set OSD WTM Bypass Off\n");
    str += MsOS_scnprintf(str, end - str, "echo 8 > OSD :Set Privacy Mask On\n");
    str += MsOS_scnprintf(str, end - str, "echo 9 > OSD :Set Privacy Mask Off\n");
    str += MsOS_scnprintf(str, end - str, "------------------------SCL OSD----------------------------\n");
    return (str - buf);
}
void MDrv_HVSP_OsdStore(const char *buf,EN_MDRV_HVSP_ID_TYPE enHVSP_ID)
{
    const char *str = buf;
    ST_DRV_HVSP_OSD_CONFIG stOSdCfg;
    EN_HVSP_ID_TYPE enID;
    enID = enHVSP_ID == E_MDRV_HVSP_ID_2 ? E_HVSP_ID_2 :
           enHVSP_ID == E_MDRV_HVSP_ID_3 ? E_HVSP_ID_3 :
                                           E_HVSP_ID_1;

    Drv_HVSP_GetOSDAttribute(enID,&stOSdCfg);
    if((int)*str == 49)    //input 1
    {
        SCL_ERR( "[OSD]open OSD %d\n",(int)*str);
        stOSdCfg.stOsdOnOff.bOSDEn = 1;
        Drv_HVSP_SetOSDConfig(enID, &stOSdCfg);
    }
    else if((int)*str == 48)  //input 0
    {
        SCL_ERR( "[OSD]close OSD %d\n",(int)*str);
        stOSdCfg.stOsdOnOff.bOSDEn = 0;
        Drv_HVSP_SetOSDConfig(enID, &stOSdCfg);
    }
    else if((int)*str == 50)  //input 2
    {
        SCL_ERR( "[OSD]Set OSD before %d\n",(int)*str);
        stOSdCfg.enOSD_loc = EN_DRV_HVSP_OSD_LOC_BEFORE;
        Drv_HVSP_SetOSDConfig(enID, &stOSdCfg);
    }
    else if((int)*str == 51)  //input 3
    {
        SCL_ERR( "[OSD]Set OSD After %d\n",(int)*str);
        stOSdCfg.enOSD_loc = EN_DRV_HVSP_OSD_LOC_AFTER;
        Drv_HVSP_SetOSDConfig(enID, &stOSdCfg);
    }
    else if((int)*str == 52)  //input 4
    {
        SCL_ERR( "[OSD]Set OSD Bypass %d\n",(int)*str);
        stOSdCfg.stOsdOnOff.bOSDBypass = 1;
        Drv_HVSP_SetOSDConfig(enID, &stOSdCfg);
    }
    else if((int)*str == 53)  //input 5
    {
        SCL_ERR( "[OSD]Set OSD Bypass Off %d\n",(int)*str);
        stOSdCfg.stOsdOnOff.bOSDBypass = 0;
        Drv_HVSP_SetOSDConfig(enID, &stOSdCfg);
    }
    else if((int)*str == 54)  //input 6
    {
        SCL_ERR( "[OSD]Set OSD WTM Bypass %d\n",(int)*str);
        stOSdCfg.stOsdOnOff.bWTMBypass = 1;
        Drv_HVSP_SetOSDConfig(enID, &stOSdCfg);
    }
    else if((int)*str == 55)  //input 7
    {
        SCL_ERR( "[OSD]Set OSD WTM Bypass Off %d\n",(int)*str);
        stOSdCfg.stOsdOnOff.bWTMBypass = 0;
        Drv_HVSP_SetOSDConfig(enID, &stOSdCfg);
    }
    else if((int)*str == 56)  //input 8
    {
        SCL_ERR( "[OSD]Set OD ON %d\n",(int)*str);
        Drv_HVSP_SetPriMaskTrigger(EN_HVSP_PRIMASK_ENABLE);
    }
    else if((int)*str == 57)  //input 9
    {
        SCL_ERR( "[OSD]Set OD Off %d\n",(int)*str);
        Drv_HVSP_SetPriMaskTrigger(EN_HVSP_PRIMASK_ONLYHWOFF);
    }
}
ssize_t MDrv_HVSP_LockShow(char *buf)
{
    char *str = buf;
    char *end = buf + PAGE_SIZE;
    str += MsOS_scnprintf(str, end - str, "------------------------SCL Lock----------------------------\n");
    str = MsOS_CheckMutex(str,end);
    str += MsOS_scnprintf(str, end - str, "------------------------SCL Lock----------------------------\n");
    return (str - buf);
}
#endif
