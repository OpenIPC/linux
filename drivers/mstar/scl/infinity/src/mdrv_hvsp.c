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

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <asm/uaccess.h>
#include <linux/clk.h>
#include <linux/clk-provider.h>
#include "MsCommon.h"
#include "MsTypes.h"
#include "MsOS.h"
#include "drvhvsp_st.h"
#include "drvhvsp.h"

#include "mdrv_hvsp_io_st.h"
#include "mdrv_scl_dbg.h"
#include "mdrv_hvsp.h"
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

//-------------------------------------------------------------------------------------------------
//  Variable
//-------------------------------------------------------------------------------------------------
ST_MDRV_HVSP_SCALING_CONFIG gstHvspScalingCfg;
ST_MDRV_HVSP_POSTCROP_CONFIG gstHvspPostCropCfg;
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

//-------------------------------------------------------------------------------------------------
//  Function
//-------------------------------------------------------------------------------------------------
unsigned char MDrv_HVSP_GetCMDQDoneStatus(void)
{
    return Drv_HVSP_GetCMDQDoneStatus();
}

wait_queue_head_t * MDrv_HVSP_GetWaitQueueHead(void)
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
void MDrv_HVSP_Release(EN_MDRV_HVSP_ID_TYPE enHVSP_ID)
{
    EN_HVSP_ID_TYPE enID;
    enID = enHVSP_ID == E_MDRV_HVSP_ID_2 ? E_HVSP_ID_2 :
           enHVSP_ID == E_MDRV_HVSP_ID_3 ? E_HVSP_ID_3 :
                                           E_HVSP_ID_1;
    Drv_HVSP_Release(enID);
}

unsigned char MDrv_HVSP_Init(EN_MDRV_HVSP_ID_TYPE enHVSP_ID, ST_MDRV_HVSP_INIT_CONFIG *pCfg)
{
    ST_HVSP_INIT_CONFIG stInitCfg;
    static unsigned char bInit = FALSE;

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
            MsOS_Memset(&gstHvspScalingCfg, 0, sizeof(ST_MDRV_HVSP_SCALING_CONFIG));
            MsOS_Memset(&gstHvspPostCropCfg, 0, sizeof(ST_MDRV_HVSP_POSTCROP_CONFIG));
        }
        bInit = TRUE;

        return TRUE;
    }

}
void MDrv_HVSP_SetMemoryAllocateReady(unsigned char bEn)
{
    gbMemReadyForMdrv = bEn;
    Drv_HVSP_SetMemoryAllocateReady(gbMemReadyForMdrv);
}
ST_HVSP_IPM_CONFIG _MDrv_HVSP_FillIPMStruct(ST_MDRV_HVSP_IPM_CONFIG *pCfg)
{
    ST_HVSP_IPM_CONFIG stIPMCfg;
    stIPMCfg.u16Fetch       = pCfg->u16Width;
    stIPMCfg.u16Vsize       = pCfg->u16Height;
    stIPMCfg.bRead          = (pCfg->enRW)&0x01;
    stIPMCfg.bWrite         = ((pCfg->enRW)>>1)&0x01;
    stIPMCfg.u32BaseAddr    = pCfg->u32PhyAddr;
    stIPMCfg.u32MemSize     = pCfg->u32MemSize;
    return stIPMCfg;
}

ST_HVSP_LDC_FRAMEBUFFER_CONFIG _MDrv_HVSP_FillLDCStruct(ST_MDRV_HVSP_IPM_CONFIG *pCfg, MS_BOOL bWrite)
{
    ST_HVSP_LDC_FRAMEBUFFER_CONFIG stLDCCfg;
    stLDCCfg.bEnSWMode      = 0x00;
    stLDCCfg.u16Height      = pCfg->u16Height;
    stLDCCfg.u16Width       = pCfg->u16Width;
    stLDCCfg.u32FBaddr      = pCfg->u32PhyAddr;
    stLDCCfg.u8FBidx        = 0x00;
    stLDCCfg.u8FBrwdiff     = SCL_DELAY2FRAMEINDOUBLEBUFFERMode ? 0x0 : 0x1;
                                    // delay 1 frame, set 1 is true, because Frame_buffer_idx = hw_w_idx - reg_ldc_fb_hw_rw_diff
                                    // So ,if want delay 2 frame, need to set '0' for 2 DNR buffer.
                                    //if only 1 DNR buffer , set '0' also delay 1 frame.
    stLDCCfg.bEnDNR         = bWrite;
    return stLDCCfg;
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
    SCL_DBG(SCL_DBG_LV_HVSP()&(Get_DBGMG_HVSP(enHVSP_ID)), "%s:%d:PhyAddr=%lx, width=%x, height=%x \n",  __FUNCTION__,enHVSP_ID, pCfg->u32PhyAddr, pCfg->u16Width, pCfg->u16Height);
    stIPMCfg = _MDrv_HVSP_FillIPMStruct(pCfg);
    stLDCCfg = _MDrv_HVSP_FillLDCStruct(pCfg, stIPMCfg.bWrite);
    Drv_HVSP_SetLDCFrameBuffer_Config(stLDCCfg);
    if(Drv_HVSP_Set_IPM_Config(stIPMCfg) == FALSE)
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
        Drv_HVSP_SetCropWindowSize();
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

    SCL_DBG(SCL_DBG_LV_HVSP()&(Get_DBGMG_HVSP(enHVSP_ID)), "%s\n", __FUNCTION__);

    enID = enHVSP_ID == E_MDRV_HVSP_ID_2 ? E_HVSP_ID_2 :
           enHVSP_ID == E_MDRV_HVSP_ID_3 ? E_HVSP_ID_3 :
                                           E_HVSP_ID_1;

    MsOS_Memcpy(&gstHvspPostCropCfg, pCfg, sizeof(ST_MDRV_HVSP_POSTCROP_CONFIG));
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

    if(pCfg->bFmCntEn)
    {
        stScalingCfg.stCmdTrigCfg.enType = E_HVSP_CMD_TRIG_CMDQ_FRMCNT;
        stScalingCfg.stCmdTrigCfg.u8Fmcnt = pCfg->u8FmCnt;
    }
    else
    {
        stScalingCfg.stCmdTrigCfg.enType = E_HVSP_CMD_TRIG_NONE; //ToDo
        stScalingCfg.stCmdTrigCfg.u8Fmcnt = 0;
    }

    ret = (unsigned char)Drv_HVSP_SetScaling(enID, stScalingCfg,(ST_HVSP_CLK_CONFIG *)pCfg->stclk);
    return  ret;


}

unsigned char MDrv_HVSP_SetScalingConfig(EN_MDRV_HVSP_ID_TYPE enHVSP_ID, ST_MDRV_HVSP_SCALING_CONFIG *pCfg )
{
    EN_HVSP_ID_TYPE enID;
    ST_HVSP_SCALING_CONFIG stScalingCfg;
    unsigned char ret;

    SCL_DBG(SCL_DBG_LV_HVSP()&(Get_DBGMG_HVSP(enHVSP_ID)), "%s:%d\n", __FUNCTION__,enHVSP_ID);

    enID = enHVSP_ID == E_MDRV_HVSP_ID_2 ? E_HVSP_ID_2 :
           enHVSP_ID == E_MDRV_HVSP_ID_3 ? E_HVSP_ID_3 :
                                           E_HVSP_ID_1;

    MsOS_Memcpy(&gstHvspScalingCfg, pCfg, sizeof(ST_MDRV_HVSP_SCALING_CONFIG));
    stScalingCfg.bCropEn[DRV_HVSP_CROP_1]        = 0;
    stScalingCfg.bCropEn[DRV_HVSP_CROP_2]        = pCfg->stCropWin.bEn;
    stScalingCfg.u16Crop_X[DRV_HVSP_CROP_2]      = pCfg->stCropWin.u16X;
    stScalingCfg.u16Crop_Y[DRV_HVSP_CROP_2]      = pCfg->stCropWin.u16Y;
    stScalingCfg.u16Crop_Width[DRV_HVSP_CROP_2]  = pCfg->stCropWin.u16Width;
    stScalingCfg.u16Crop_Height[DRV_HVSP_CROP_2] = pCfg->stCropWin.u16Height;

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

    ret = (unsigned char)Drv_HVSP_SetScaling(enID, stScalingCfg,(ST_HVSP_CLK_CONFIG *)pCfg->stclk);
    return  ret;
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


    if(copy_from_user(pBuf, (ST_MDRV_HVSP_MISC_CONFIG  __user *)pCfg->u32Addr, pCfg->u32Size))
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
     enID = enHVSP_ID == E_MDRV_HVSP_ID_2 ? E_HVSP_ID_2 :
            enHVSP_ID == E_MDRV_HVSP_ID_3 ? E_HVSP_ID_3 :
                                            E_HVSP_ID_1;
     MsOS_Memcpy(pstCfg, &stOSdCfg, sizeof(ST_MDRV_HVSP_OSD_CONFIG));
     Drv_HVSP_SetOSDConfig(enID, stOSdCfg);
     SCL_DBG(SCL_DBG_LV_HVSP()&(Get_DBGMG_HVSP(enID)),
        "%s::ID:%d, OnOff:%hhd ,Bypass:%hhd\n",
        __FUNCTION__,enID,stOSdCfg.stOsdOnOff.bOSDEn,stOSdCfg.stOsdOnOff.bOSDBypass);
     return Ret;
}

unsigned char Mdrv_HVSP_GetDNRBufferInformation(void)
{
    return gu8IPMBufferNum;
}

unsigned char MDrv_HVSP_SetFbManageConfig(EN_MDRV_HVSP_FBMG_SET_TYPE enSet)
{
    unsigned char Ret = TRUE;
    ST_DRV_HVSP_SET_FB_MANAGE_CONFIG stCfg;
    SCL_DBG(SCL_DBG_LV_HVSP()&(Get_DBGMG_HVSP(3)), "%s\n", __FUNCTION__);
    stCfg.enSet = enSet;
    if (stCfg.enSet & EN_DRV_HVSP_FBMG_SET_DNR_COMDE_ON)
    {
        SCL_ERR( "COMDE_ON\n");
        MDrv_VIP_SetDNRConpressForDebug(1);
    }
    else if (stCfg.enSet & EN_DRV_HVSP_FBMG_SET_DNR_COMDE_OFF)
    {
        SCL_ERR( "COMDE_OFF\n");
        MDrv_VIP_SetDNRConpressForDebug(0);
    }
    else if (stCfg.enSet & EN_DRV_HVSP_FBMG_SET_DNR_COMDE_265OFF)
    {
        SCL_ERR( "COMDE_265OFF\n");
        MDrv_VIP_SetDNRConpressForDebug(0);
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
    Drv_HVSP_SetFbManageConfig(stCfg);
    return Ret;
}
void MDrv_HVSP_IDCLKRelease(ST_MDRV_HVSP_CLK_CONFIG* stclk)
{
    SCL_DBG(SCL_DBG_LV_HVSP()&(Get_DBGMG_HVSP(3)), "%s\n", __FUNCTION__);
    Drv_HVSP_IDCLKRelease((ST_HVSP_CLK_CONFIG *)stclk);
}

unsigned long MDrv_HVSP_HWMonitor(unsigned char u8flag)
{
    if(u8flag == EN_MDRV_HVSP_MONITOR_CROPCHECK)
    {
        return Drv_HVSP_CropCheck();
    }
    else if(u8flag >= EN_MDRV_HVSP_MONITOR_DMA1FRMCHECK)
    {
        return Drv_HVSP_DMACheck(u8flag);
    }
    return 0;
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
    if (__clk_get_enable_count((MSOS_ST_CLK*)adjclk)==0)
    {
    }
    else
    {
        if (NULL != (pstclock = clk_get_parent_by_index((MSOS_ST_CLK*)adjclk, (u8Idx &0x0F))))
        {
            clk_set_parent((MSOS_ST_CLK*)adjclk, pstclock);
        }
    }
}
void MDrv_HVSP_SetCLKOnOff(void* adjclk,unsigned char bEn)
{
    MSOS_ST_CLK* pstclock = NULL;
    if (__clk_get_enable_count((MSOS_ST_CLK*)adjclk)==0 &&bEn)
    {
        if (NULL != (pstclock = clk_get_parent_by_index((MSOS_ST_CLK*)adjclk, 0)))
        {
            clk_set_parent((MSOS_ST_CLK*)adjclk, pstclock);
            clk_prepare_enable((MSOS_ST_CLK*)adjclk);
        }
    }
    else if(__clk_get_enable_count((MSOS_ST_CLK*)adjclk)!=0 && !bEn)
    {
        if (NULL != (pstclock = clk_get_parent_by_index((MSOS_ST_CLK*)adjclk, 0)))
        {
            clk_set_parent((MSOS_ST_CLK*)adjclk, pstclock);
            clk_disable_unprepare((MSOS_ST_CLK*)adjclk);
        }
    }
}
ssize_t MDrv_HVSP_monitorHWShow(char *buf,int VsyncCount ,int MonitorErrCount)
{
	char *str = buf;
	char *end = buf + PAGE_SIZE;
    str += scnprintf(str, end - str, "========================SCL monitor HW BUTTON======================\n");
    str += scnprintf(str, end - str, "CROP Monitor    :1\n");
    str += scnprintf(str, end - str, "DMA1FRM Monitor :2\n");
    str += scnprintf(str, end - str, "DMA1SNP Monitor :3\n");
    str += scnprintf(str, end - str, "DMA2FRM Monitor :4\n");
    str += scnprintf(str, end - str, "DMA3FRM Monitor :5\n");
    str += scnprintf(str, end - str, "========================SCL monitor HW ======================\n");
    str += scnprintf(str, end - str, "vysnc count:%d",VsyncCount);
    str += scnprintf(str, end - str, "Monitor Err count:%d\n",MonitorErrCount);
	return (str - buf);
}
ssize_t MDrv_HVSP_DbgmgFlagShow(char *buf)
{
	char *str = buf;
	char *end = buf + PAGE_SIZE;
    str += scnprintf(str, end - str, "========================SCL Debug Message BUTTON======================\n");
    str += scnprintf(str, end - str, "CONFIG            ECHO        STATUS\n");
    str += scnprintf(str, end - str, "MDRV_CONFIG       (1)         0x%x\n",gbdbgmessage[EN_DBGMG_MDRV_CONFIG]);
    str += scnprintf(str, end - str, "IOCTL_CONFIG      (2)         0x%x\n",gbdbgmessage[EN_DBGMG_IOCTL_CONFIG]);
    str += scnprintf(str, end - str, "HVSP_CONFIG       (3)         0x%x\n",gbdbgmessage[EN_DBGMG_HVSP_CONFIG]);
    str += scnprintf(str, end - str, "SCLDMA_CONFIG     (4)         0x%x\n",gbdbgmessage[EN_DBGMG_SCLDMA_CONFIG]);
    str += scnprintf(str, end - str, "PNL_CONFIG        (5)         0x%x\n",gbdbgmessage[EN_DBGMG_PNL_CONFIG]);
    str += scnprintf(str, end - str, "VIP_CONFIG        (6)         0x%x\n",gbdbgmessage[EN_DBGMG_VIP_CONFIG]);
    str += scnprintf(str, end - str, "DRVPQ_CONFIG      (7)         0x%x\n",gbdbgmessage[EN_DBGMG_DRVPQ_CONFIG]);
    str += scnprintf(str, end - str, "INST_ENTRY_CONFIG (8)         0x%x\n",gbdbgmessage[EN_DBGMG_INST_ENTRY_CONFIG]);
    str += scnprintf(str, end - str, "INST_LOCK_CONFIG  (9)         0x%x\n",gbdbgmessage[EN_DBGMG_INST_LOCK_CONFIG]);
    str += scnprintf(str, end - str, "INST_FUNC_CONFIG  (A)         0x%x\n",gbdbgmessage[EN_DBGMG_INST_FUNC_CONFIG]);
    str += scnprintf(str, end - str, "DRVHVSP_CONFIG    (B)         0x%x\n",gbdbgmessage[EN_DBGMG_DRVHVSP_CONFIG]);
    str += scnprintf(str, end - str, "DRVSCLDMA_CONFIG  (C)         0x%x\n",gbdbgmessage[EN_DBGMG_DRVSCLDMA_CONFIG]);
    str += scnprintf(str, end - str, "DRVSCLIRQ_CONFIG  (D)         0x%x\n",gbdbgmessage[EN_DBGMG_DRVSCLIRQ_CONFIG]);
    str += scnprintf(str, end - str, "DRVCMDQ_CONFIG    (E)         0x%x\n",gbdbgmessage[EN_DBGMG_DRVCMDQ_CONFIG]);
    str += scnprintf(str, end - str, "DRVVIP_CONFIG     (F)         0x%x\n",gbdbgmessage[EN_DBGMG_DRVVIP_CONFIG]);
    str += scnprintf(str, end - str, "PRIORITY_CONFIG   (G)         0x%x\n",gbdbgmessage[EN_DBGMG_PRIORITY_CONFIG]);
    str += scnprintf(str, end - str, "ALL Reset         (0)\n");
    str += scnprintf(str, end - str, "========================SCL Debug Message BUTTON======================\n");
    str += scnprintf(str, end - str, "========================SCL Debug Message LEVEL======================\n");
    str += scnprintf(str, end - str, "default is level 1\n");
    str += scnprintf(str, end - str, "-------------------------------IOCTL LEVEL---------------------------\n");
    str += scnprintf(str, end - str, "0x 1 : SC1\n");
    str += scnprintf(str, end - str, "0x 2 : SC2\n");
    str += scnprintf(str, end - str, "0x 4 : SC3\n");
    str += scnprintf(str, end - str, "0x 8 : VIP\n");
    str += scnprintf(str, end - str, "0x10 : SC1HLEVEL\n");
    str += scnprintf(str, end - str, "0x20 : SC2HLEVEL\n");
    str += scnprintf(str, end - str, "0x40 : ELSE\n");
    str += scnprintf(str, end - str, "-------------------------------HVSP LEVEL---------------------------\n");
    str += scnprintf(str, end - str, "0x 1 : HVSP1\n");
    str += scnprintf(str, end - str, "0x 2 : HVSP2\n");
    str += scnprintf(str, end - str, "0x 4 : HVSP3\n");
    str += scnprintf(str, end - str, "0x 8 : ELSE\n");
    str += scnprintf(str, end - str, "-------------------------------SCLDMA LEVEL-------------------------\n");
    str += scnprintf(str, end - str, "0x 1 : SC1 FRM\n");
    str += scnprintf(str, end - str, "0x 2 : SC1 SNP \n");
    str += scnprintf(str, end - str, "0x 4 : SC2 FRM\n");
    str += scnprintf(str, end - str, "0x 8 : SC3 FRM\n");
    str += scnprintf(str, end - str, "0x10 : SC1 FRM HL\n");
    str += scnprintf(str, end - str, "0x20 : SC1 SNP HL\n");
    str += scnprintf(str, end - str, "0x40 : SC2 FRM HL\n");
    str += scnprintf(str, end - str, "0x80 : ELSE\n");
    str += scnprintf(str, end - str, "-------------------------------VIP LEVEL(IOlevel)-------------------\n");
    str += scnprintf(str, end - str, "0x 1 : NORMAL\n");
    str += scnprintf(str, end - str, "0x 2 : VIP LOG \n");
    str += scnprintf(str, end - str, "0x 4 : VIP SUSPEND\n");
    str += scnprintf(str, end - str, "0x 8 : ELSE\n");
    str += scnprintf(str, end - str, "------------------------------MULTI LEVEL---------------------------\n");
    str += scnprintf(str, end - str, "0x 1 : SC1\n");
    str += scnprintf(str, end - str, "0x 2 : SC3\n");
    str += scnprintf(str, end - str, "0x 4 : DISP\n");
    str += scnprintf(str, end - str, "0x 8 : ELSE\n");
    str += scnprintf(str, end - str, "-------------------------------SCLIRQ LEVEL(drvlevel)---------------\n");
    str += scnprintf(str, end - str, "0x 1 : NORMAL\n");
    str += scnprintf(str, end - str, "0x 2 : SC1RINGA \n");
    str += scnprintf(str, end - str, "0x 4 : SC1RINGN\n");
    str += scnprintf(str, end - str, "0x 8 : SC1SINGLE\n");
    str += scnprintf(str, end - str, "0x10 : SC2RINGA\n");
    str += scnprintf(str, end - str, "0x20 : SC2RINGN \n");
    str += scnprintf(str, end - str, "0x40 : SC3SINGLE\n");
    str += scnprintf(str, end - str, "0x80 : ELSE\n");
    str += scnprintf(str, end - str, "-------------------------------CMDQ LEVEL(drvlevel)-----------------\n");
    str += scnprintf(str, end - str, "0x 1 : LOW\n");
    str += scnprintf(str, end - str, "0x 2 : NORMAL \n");
    str += scnprintf(str, end - str, "0x 4 : HIGH\n");
    str += scnprintf(str, end - str, "0x 8 : ISR\n");
    str += scnprintf(str, end - str, "\n");
    str += scnprintf(str, end - str, "========================SCL Debug Message LEVEL======================\n");
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
    unsigned char bLDC;
    ST_DRV_HVSP_OSD_CONFIG stOsdCfg;
    Drv_HVSP_GetCrop12Inform(&stInformCfg);
    Drv_HVSP_GetSCLInform(E_HVSP_ID_1,&stzoomformCfg);
    Drv_HVSP_GetHVSPAttribute(E_HVSP_ID_1,&sthvspformCfg);
    Drv_HVSP_GetHVSPAttribute(E_HVSP_ID_2,&sthvsp2formCfg);
    Drv_HVSP_GetHVSPAttribute(E_HVSP_ID_3,&sthvsp3formCfg);
    Drv_HVSP_GetOSDAttribute(E_HVSP_ID_1,&stOsdCfg);
    bLDC = Drv_HVSP_GetFrameBufferAttribute(E_HVSP_ID_1,&stIpmformCfg);
    str += scnprintf(str, end - str, "========================SCL PROC FRAMEWORK======================\n");
    str += scnprintf(str, end - str, "\n");
    str += scnprintf(str, end - str, "------------------------SCL INPUT MUX----------------------\n");
    if(stInformCfg.enMux==0)
    {
        str += scnprintf(str, end - str, "Input SRC :BT656\n");
    }
    else if(stInformCfg.enMux==1)
    {
        str += scnprintf(str, end - str, "Input SRC :ISP\n");
    }
    else if(stInformCfg.enMux==3)
    {
        str += scnprintf(str, end - str, "Input SRC :PTGEN\n");
    }
    else
    {
        str += scnprintf(str, end - str, "Input SRC :OTHER\n");
    }
    str += scnprintf(str, end - str, "Input H   :%hd\n",stInformCfg.u16inWidth);
    str += scnprintf(str, end - str, "Input V   :%hd\n",stInformCfg.u16inHeight);
    str += scnprintf(str, end - str, "Receive H :%hd\n",stInformCfg.u16inWidthcount);
    str += scnprintf(str, end - str, "Receive V :%hd\n",stInformCfg.u16inHeightcount);
    str += scnprintf(str, end - str, "------------------------SCL FB-----------------------------\n");
    str += scnprintf(str, end - str, "FB H          :%hd\n",stIpmformCfg.u16Fetch);
    str += scnprintf(str, end - str, "FB V          :%hd\n",stIpmformCfg.u16Vsize);
    str += scnprintf(str, end - str, "FB Addr       :%lx\n",stIpmformCfg.u32BaseAddr);
    str += scnprintf(str, end - str, "FB memsize    :%ld\n",stIpmformCfg.u32MemSize);
    str += scnprintf(str, end - str, "FB Buffer     :%hhd\n",gu8IPMBufferNum);
    str += scnprintf(str, end - str, "FB Write      :%hhd\n",stIpmformCfg.bWrite);
    if(bLDC)
    {
        str += scnprintf(str, end - str, "READ PATH     :LDC\n");
    }
    else if(stIpmformCfg.bRead)
    {
        str += scnprintf(str, end - str, "READ PATH     :DNR\n");
    }
    else
    {
        str += scnprintf(str, end - str, "READ PATH     :NONE\n");
    }
    str += scnprintf(str, end - str, "------------------------SCL Zoom----------------------------\n");
    str += scnprintf(str, end - str, "Zoom      :%hhd\n",stzoomformCfg.bEn);
    str += scnprintf(str, end - str, "ZoomX     :%hd\n",stzoomformCfg.u16X);
    str += scnprintf(str, end - str, "ZoomY     :%hd\n",stzoomformCfg.u16Y);
    str += scnprintf(str, end - str, "ZoomOutW  :%hd\n",stzoomformCfg.u16crop2OutWidth);
    str += scnprintf(str, end - str, "ZoomOutH  :%hd\n",stzoomformCfg.u16crop2OutHeight);
    str += scnprintf(str, end - str, "SrcW      :%hd\n",stzoomformCfg.u16crop2inWidth);
    str += scnprintf(str, end - str, "SrcH      :%hd\n",stzoomformCfg.u16crop2inHeight);
    str += scnprintf(str, end - str, "------------------------SCL OSD----------------------------\n");
    str += scnprintf(str, end - str, "ONOFF     :%hhd\n",stOsdCfg.stOsdOnOff.bOSDEn);
    if(stOsdCfg.enOSD_loc)
    {
        str += scnprintf(str, end - str, "Locate: Before\n");
    }
    else
    {
        str += scnprintf(str, end - str, "Locate: After\n");
    }
    str += scnprintf(str, end - str, "------------------------SCL HVSP1----------------------------\n");
    str += scnprintf(str, end - str, "InputH    :%hd\n",sthvspformCfg.u16inWidth);
    str += scnprintf(str, end - str, "InputV    :%hd\n",sthvspformCfg.u16inHeight);
    str += scnprintf(str, end - str, "OutputH   :%hd\n",sthvspformCfg.u16Width);
    str += scnprintf(str, end - str, "OutputV   :%hd\n",sthvspformCfg.u16Height);
    str += scnprintf(str, end - str, "function  :%hhd\n",sthvspformCfg.bEn);
    str += scnprintf(str, end - str, "------------------------SCL HVSP2----------------------------\n");
    str += scnprintf(str, end - str, "InputH    :%hd\n",sthvsp2formCfg.u16inWidth);
    str += scnprintf(str, end - str, "InputV    :%hd\n",sthvsp2formCfg.u16inHeight);
    str += scnprintf(str, end - str, "OutputH   :%hd\n",sthvsp2formCfg.u16Width);
    str += scnprintf(str, end - str, "OutputV   :%hd\n",sthvsp2formCfg.u16Height);
    str += scnprintf(str, end - str, "function  :%hhd\n",sthvsp2formCfg.bEn);
    str += scnprintf(str, end - str, "------------------------SCL HVSP3----------------------------\n");
    str += scnprintf(str, end - str, "InputH    :%hd\n",sthvsp3formCfg.u16inWidth);
    str += scnprintf(str, end - str, "InputV    :%hd\n",sthvsp3formCfg.u16inHeight);
    str += scnprintf(str, end - str, "OutputH   :%hd\n",sthvsp3formCfg.u16Width);
    str += scnprintf(str, end - str, "OutputV   :%hd\n",sthvsp3formCfg.u16Height);
    str += scnprintf(str, end - str, "function  :%hhd\n",sthvsp3formCfg.bEn);
    str += scnprintf(str, end - str, "\n");
    str += scnprintf(str, end - str, "========================SCL PROC FRAMEWORK======================\n");
	return (str - buf);
}
ssize_t MDrv_HVSP_ClkFrameworkShow(char *buf,ST_MDRV_HVSP_CLK_CONFIG* stclk)
{
    char *str = buf;
    char *end = buf + PAGE_SIZE;
    str += scnprintf(str, end - str, "========================SCL CLK FRAMEWORK======================\n");
    str += scnprintf(str, end - str, "echo 1 > clk :open force mode\n");
    str += scnprintf(str, end - str, "echo 0 > clk :close force mode\n");
    str += scnprintf(str, end - str, "echo 2 > clk :fclk1 max\n");
    str += scnprintf(str, end - str, "echo 3 > clk :fclk1 med\n");
    str += scnprintf(str, end - str, "echo 4 > clk :fclk1 open\n");
    str += scnprintf(str, end - str, "echo 5 > clk :fclk1 close\n");
    str += scnprintf(str, end - str, "echo 6 > clk :fclk2 max\n");
    str += scnprintf(str, end - str, "echo 7 > clk :fclk2 med\n");
    str += scnprintf(str, end - str, "echo 8 > clk :fclk2 open\n");
    str += scnprintf(str, end - str, "echo 9 > clk :fclk2 close\n");
    str += scnprintf(str, end - str, "echo : > clk :idclk ISP\n");
    str += scnprintf(str, end - str, "echo D > clk :idclk BT656\n");
    str += scnprintf(str, end - str, "echo B > clk :idclk open\n");
    str += scnprintf(str, end - str, "echo = > clk :idclk close\n");
    str += scnprintf(str, end - str, "echo C > clk :odclk MAX\n");
    str += scnprintf(str, end - str, "echo ? > clk :odclk LPLL\n");
    str += scnprintf(str, end - str, "echo @ > clk :odclk open\n");
    str += scnprintf(str, end - str, "echo A > clk :odclk close\n");
    str += scnprintf(str, end - str, "========================SCL CLK STATUS======================\n");
    str += scnprintf(str, end - str, "force mode :%hhd\n",Drv_HVSP_GetCLKForcemode());
    if(__clk_get_enable_count((MSOS_ST_CLK*)stclk->fclk1))
    {
        str += scnprintf(str, end - str, "fclk1 open :%d ,%ld\n",
            __clk_get_enable_count((MSOS_ST_CLK*)stclk->fclk1),__clk_get_rate((MSOS_ST_CLK*)stclk->fclk1));
    }
    else
    {
        str += scnprintf(str, end - str, "fclk1 close\n");
    }
    if(__clk_get_enable_count((MSOS_ST_CLK*)stclk->fclk2))
    {
        str += scnprintf(str, end - str, "fclk2 open :%d,%ld\n",
            __clk_get_enable_count((MSOS_ST_CLK*)stclk->fclk2),__clk_get_rate((MSOS_ST_CLK*)stclk->fclk2));
    }
    else
    {
        str += scnprintf(str, end - str, "fclk2 close\n");
    }
    if(__clk_get_enable_count((MSOS_ST_CLK*)stclk->idclk))
    {
        if(__clk_get_rate((MSOS_ST_CLK*)stclk->idclk) > 10)
        {
            str += scnprintf(str, end - str, "idclk open :ISP\n");
        }
        else if(__clk_get_rate((MSOS_ST_CLK*)stclk->idclk) == 1)
        {
            str += scnprintf(str, end - str, "idclk open :BT656\n");
        }
    }
    else
    {
        str += scnprintf(str, end - str, "idclk close\n");
    }
    if(__clk_get_enable_count((MSOS_ST_CLK*)stclk->odclk))
    {
        if(__clk_get_rate((MSOS_ST_CLK*)stclk->odclk) == 432000000)
        {
            str += scnprintf(str, end - str, "odclk open LPLL:%ld\n",(Drv_PNL_GetLPLLDclk()/10000)*10000);
        }
        else
        {
            str += scnprintf(str, end - str, "odclk open :%ld\n",__clk_get_rate(stclk->odclk));
        }
    }
    else if(Drv_PNL_GetPnlOpen())
    {
        str += scnprintf(str, end - str, "odclk manual open LPLL:%ld\n",(Drv_PNL_GetLPLLDclk()/10000)*10000);
    }
    else
    {
        str += scnprintf(str, end - str, "odclk close\n");
    }
    return (str - buf);
}
ssize_t MDrv_HVSP_FBMGShow(char *buf)
{
	char *str = buf;
	char *end = buf + PAGE_SIZE;
    str += scnprintf(str, end - str, "------------------------SCL FBMG----------------------------\n");
    str += scnprintf(str, end - str, "LDCPATH_ON    :1\n");
    str += scnprintf(str, end - str, "LDCPATH_OFF   :2\n");
    str += scnprintf(str, end - str, "DNRRead_ON    :3\n");
    str += scnprintf(str, end - str, "DNRRead_OFF   :4\n");
    str += scnprintf(str, end - str, "DNRWrite_ON   :5\n");
    str += scnprintf(str, end - str, "DNRWrite_OFF  :6\n");
    str += scnprintf(str, end - str, "DNRBuf1       :7\n");
    str += scnprintf(str, end - str, "DNRBuf2       :8\n");
    str += scnprintf(str, end - str, "UNLOCK        :9\n");
    str += scnprintf(str, end - str, "------------------------SCL FBMG----------------------------\n");
    return (str - buf);
}
ssize_t MDrv_HVSP_OsdShow(char *buf)
{
	char *str = buf;
	char *end = buf + PAGE_SIZE;
    ST_DRV_HVSP_OSD_CONFIG stOsdCfg;
    Drv_HVSP_GetOSDAttribute(E_HVSP_ID_1,&stOsdCfg);
    str += scnprintf(str, end - str, "------------------------SCL OSD----------------------------\n");
    str += scnprintf(str, end - str, "ONOFF     :%hhd\n",stOsdCfg.stOsdOnOff.bOSDEn);
    if(stOsdCfg.enOSD_loc)
    {
        str += scnprintf(str, end - str, "Locate: Before\n");
    }
    else
    {
        str += scnprintf(str, end - str, "Locate: After\n");
    }
    str += scnprintf(str, end - str, "Bypass    :%hhd\n",stOsdCfg.stOsdOnOff.bOSDBypass);
    str += scnprintf(str, end - str, "WTM Bypass:%hhd\n",stOsdCfg.stOsdOnOff.bWTMBypass);
    str += scnprintf(str, end - str, "------------------------SCL OSD----------------------------\n");
    str += scnprintf(str, end - str, "echo 1 > OSD :open OSD\n");
    str += scnprintf(str, end - str, "echo 0 > OSD :close OSD\n");
    str += scnprintf(str, end - str, "echo 2 > OSD :Set OSD before\n");
    str += scnprintf(str, end - str, "echo 3 > OSD :Set OSD After\n");
    str += scnprintf(str, end - str, "echo 4 > OSD :Set OSD Bypass\n");
    str += scnprintf(str, end - str, "echo 5 > OSD :Set OSD Bypass Off\n");
    str += scnprintf(str, end - str, "echo 6 > OSD :Set OSD WTM Bypass\n");
    str += scnprintf(str, end - str, "echo 7 > OSD :Set OSD WTM Bypass Off\n");
    str += scnprintf(str, end - str, "------------------------SCL OSD----------------------------\n");
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
        Drv_HVSP_SetOSDConfig(enID, stOSdCfg);
    }
    else if((int)*str == 48)  //input 0
    {
        SCL_ERR( "[OSD]close OSD %d\n",(int)*str);
        stOSdCfg.stOsdOnOff.bOSDEn = 0;
        Drv_HVSP_SetOSDConfig(enID, stOSdCfg);
    }
    else if((int)*str == 50)  //input 2
    {
        SCL_ERR( "[OSD]Set OSD before %d\n",(int)*str);
        stOSdCfg.enOSD_loc = EN_DRV_HVSP_OSD_LOC_BEFORE;
        Drv_HVSP_SetOSDConfig(enID, stOSdCfg);
    }
    else if((int)*str == 51)  //input 3
    {
        SCL_ERR( "[OSD]Set OSD After %d\n",(int)*str);
        stOSdCfg.enOSD_loc = EN_DRV_HVSP_OSD_LOC_AFTER;
        Drv_HVSP_SetOSDConfig(enID, stOSdCfg);
    }
    else if((int)*str == 52)  //input 4
    {
        SCL_ERR( "[OSD]Set OSD Bypass %d\n",(int)*str);
        stOSdCfg.stOsdOnOff.bOSDBypass = 1;
        Drv_HVSP_SetOSDConfig(enID, stOSdCfg);
    }
    else if((int)*str == 53)  //input 5
    {
        SCL_ERR( "[OSD]Set OSD Bypass Off %d\n",(int)*str);
        stOSdCfg.stOsdOnOff.bOSDBypass = 0;
        Drv_HVSP_SetOSDConfig(enID, stOSdCfg);
    }
    else if((int)*str == 54)  //input 6
    {
        SCL_ERR( "[OSD]Set OSD WTM Bypass %d\n",(int)*str);
        stOSdCfg.stOsdOnOff.bWTMBypass = 1;
        Drv_HVSP_SetOSDConfig(enID, stOSdCfg);
    }
    else if((int)*str == 55)  //input 7
    {
        SCL_ERR( "[OSD]Set OSD WTM Bypass Off %d\n",(int)*str);
        stOSdCfg.stOsdOnOff.bWTMBypass = 0;
        Drv_HVSP_SetOSDConfig(enID, stOSdCfg);
    }
}
#endif
