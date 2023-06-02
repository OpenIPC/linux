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
#define  _MDRV_SCLDMA_C

//#include <linux/module.h>
//#include <linux/kernel.h>
//#include <linux/delay.h>

//#include "mdrv_types.h"
//#include "mdrv_scldma_io_i3_st.h"
#include "MsCommon.h"
#include "MsTypes.h"
#include "MsOS.h"
#include "mdrv_scl_dbg.h"
#include "mdrv_scldma.h"
#include "drvscldma_st.h"
#include "drvscldma.h"

//-------------------------------------------------------------------------------------------------
//  Defines & Macro
//-------------------------------------------------------------------------------------------------
#define BUFFER_NUMBER_TO_HWIDX_OFFSET 1
#define _ISQueueNeedCopyToUser(enUsedType) (enUsedType==EN_MDRV_SCLDMA_BUFFER_QUEUE_TYPE_PEEKQUEUE \
        || enUsedType==EN_MDRV_SCLDMA_BUFFER_QUEUE_TYPE_DEQUEUE)
//-------------------------------------------------------------------------------------------------
//  Variable
//-------------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------
//  Function
//-------------------------------------------------------------------------------------------------
void _MDrv_SCLDMA_BufferQueueHandlerByUsedType
(EN_MDRV_SCLDMA_USED_BUFFER_QUEUE_TYPE  enUsedType,ST_SCLDMA_BUFFER_QUEUE_CONFIG *pstCfg)
{
    switch(enUsedType)
    {
        case EN_MDRV_SCLDMA_BUFFER_QUEUE_TYPE_PEEKQUEUE:
                Drv_SCLDMA_PeekBufferQueue(pstCfg);
            break;
        case EN_MDRV_SCLDMA_BUFFER_QUEUE_TYPE_DEQUEUE:
                Drv_SCLDMA_BufferDeQueue(pstCfg);
            break;
        case EN_MDRV_SCLDMA_BUFFER_QUEUE_TYPE_ENQUEUE:
                Drv_SCLDMA_EnableBufferAccess(pstCfg);
            break;
        default :
            SCL_ERR( "[SCLDMA]%s NO this Queue Handler Type\n", __FUNCTION__);
            break;
    }
}

EN_SCLDMA_RW_MODE_TYPE _MDrv_SCLDMA_SwitchIDtoReadModeForDriverlayer
(EN_MDRV_SCLDMA_ID_TYPE enSCLDMA_ID,EN_MDRV_SCLDMA_MEM_TYPE enMemType,EN_SCLDMA_ID_TYPE *enID)
{
    EN_SCLDMA_RW_MODE_TYPE enRWMode = E_SCLDMA_RW_NUM;
    switch(enSCLDMA_ID)
    {
    case E_MDRV_SCLDMA_ID_3:
        *enID = E_SCLDMA_ID_3_R;
        enRWMode = enMemType == E_MDRV_SCLDMA_MEM_FRM ?  E_SCLDMA_FRM_R :
                                                          E_SCLDMA_RW_NUM;
        break;

    case E_MDRV_SCLDMA_ID_PNL:
        *enID = E_SCLDMA_ID_PNL_R;
        enRWMode = enMemType == E_MDRV_SCLDMA_MEM_FRM ? E_SCLDMA_DBG_R :
                                                         E_SCLDMA_RW_NUM;
        break;

    default:
    case E_MDRV_SCLDMA_ID_1:
    case E_MDRV_SCLDMA_ID_2:
        SCL_ERR( "[SCLDMA]%s %d::Not support In TRIGGER\n",__FUNCTION__, __LINE__);
        break;
    }
    return enRWMode;
}
EN_SCLDMA_RW_MODE_TYPE _MDrv_SCLDMA_SwitchIDtoWriteModeForDriverlayer
(EN_MDRV_SCLDMA_ID_TYPE enSCLDMA_ID,EN_MDRV_SCLDMA_MEM_TYPE enMemType,EN_SCLDMA_ID_TYPE *enID)
{
    EN_SCLDMA_RW_MODE_TYPE enRWMode = E_SCLDMA_RW_NUM;
    switch(enSCLDMA_ID)
    {
        case E_MDRV_SCLDMA_ID_1:
            *enID = E_SCLDMA_ID_1_W;
            enRWMode = enMemType == E_MDRV_SCLDMA_MEM_FRM ? E_SCLDMA_FRM_W :
                        enMemType == E_MDRV_SCLDMA_MEM_SNP ? E_SCLDMA_SNP_W :
                        enMemType == E_MDRV_SCLDMA_MEM_IMI ? E_SCLDMA_IMI_W :
                                                              E_SCLDMA_RW_NUM;
            break;

        case E_MDRV_SCLDMA_ID_2:
            *enID = E_SCLDMA_ID_2_W;
            enRWMode = enMemType == E_MDRV_SCLDMA_MEM_FRM ? E_SCLDMA_FRM_W :
                        enMemType == E_MDRV_SCLDMA_MEM_FRM2 ? E_SCLDMA_FRM2_W :
                        enMemType == E_MDRV_SCLDMA_MEM_IMI ? E_SCLDMA_IMI_W :
                                                              E_SCLDMA_RW_NUM;

            break;

        case E_MDRV_SCLDMA_ID_3:
            *enID = E_SCLDMA_ID_3_W;
            enRWMode = enMemType == E_MDRV_SCLDMA_MEM_FRM ? E_SCLDMA_FRM_W :
                                                              E_SCLDMA_RW_NUM;
            break;

        case E_MDRV_SCLDMA_ID_PNL:
            *enID = E_SCLDMA_ID_PNL_R;
            enRWMode = enMemType == E_MDRV_SCLDMA_MEM_FRM ? E_SCLDMA_DBG_R:
                                                              E_SCLDMA_RW_NUM;
            break;
        default:
            SCL_ERR( "[SCLDMA]%s %d::Not support In SCLDMA\n",__FUNCTION__, __LINE__);
            enRWMode = E_SCLDMA_RW_NUM;
            break;
    }
    return enRWMode;
}
EN_SCLDMA_RW_MODE_TYPE _MDrv_SCLDMA_SwitchIDForDriverlayer
    (EN_MDRV_SCLDMA_ID_TYPE enSCLDMA_ID,EN_MDRV_SCLDMA_MEM_TYPE enMemType,EN_SCLDMA_ID_TYPE *enID,unsigned char bReadDMAMode)
{
    EN_SCLDMA_RW_MODE_TYPE enRWMode;
    if(bReadDMAMode)
    {
        enRWMode = _MDrv_SCLDMA_SwitchIDtoReadModeForDriverlayer(enSCLDMA_ID,enMemType,enID);
    }
    else
    {
        enRWMode = _MDrv_SCLDMA_SwitchIDtoWriteModeForDriverlayer(enSCLDMA_ID,enMemType,enID);
    }
    return enRWMode;
}
void _MDrv_SCLDMA_FillDMAInfoStruct(ST_SCLDMA_ATTR_TYPE *stDrvDMACfg,ST_MDRV_SCLDMA_ATTR_TYPE *stSendToIOCfg)
{
    int u32BufferIdx;
    stSendToIOCfg->u16DMAcount  = stDrvDMACfg->u16DMAcount;
    stSendToIOCfg->u32Trigcount = stDrvDMACfg->u32Trigcount;
    stSendToIOCfg->u16DMAH      = stDrvDMACfg->u16DMAH;
    stSendToIOCfg->u16DMAV      = stDrvDMACfg->u16DMAV;
    stSendToIOCfg->enBufMDType  = (EN_MDRV_SCLDMA_BUFFER_MODE_TYPE)stDrvDMACfg->enBuffMode;
    stSendToIOCfg->enColorType  = (EN_MDRV_SCLDMA_COLOR_TYPE)stDrvDMACfg->enColor;
    stSendToIOCfg->u16BufNum    = stDrvDMACfg->u8MaxIdx+1;
    stSendToIOCfg->bDMAEn       = stDrvDMACfg->bDMAEn;
    for(u32BufferIdx=0;u32BufferIdx<stSendToIOCfg->u16BufNum;u32BufferIdx++)
    {
        stSendToIOCfg->u32Base_Y[u32BufferIdx] = stDrvDMACfg->u32Base_Y[u32BufferIdx];
        stSendToIOCfg->u32Base_C[u32BufferIdx] = stDrvDMACfg->u32Base_C[u32BufferIdx];
        stSendToIOCfg->u32Base_V[u32BufferIdx] = stDrvDMACfg->u32Base_V[u32BufferIdx];
    }
    stSendToIOCfg->bDMAFlag = stDrvDMACfg->bDMAFlag;
    stSendToIOCfg->bDMAReadIdx = stDrvDMACfg->bDMAReadIdx;
    stSendToIOCfg->bDMAWriteIdx = stDrvDMACfg->bDMAWriteIdx;
    stSendToIOCfg->bSendPoll = stDrvDMACfg->bSendPoll;
    stSendToIOCfg->u32FrameDoneTime = stDrvDMACfg->u32FrameDoneTime;
    stSendToIOCfg->u32SendTime = stDrvDMACfg->u32SendTime;
    stSendToIOCfg->u8Count= stDrvDMACfg->u8Count;
    stSendToIOCfg->u8ResetCount= stDrvDMACfg->u8ResetCount;
    stSendToIOCfg->u8DMAErrCount= stDrvDMACfg->u8DMAErrCount;
}

void _MDrv_SCLDMA_FillActiveBufferStruct
    (EN_MDRV_SCLDMA_ID_TYPE enSCLDMA_ID, ST_MDRV_SCLDMA_ACTIVE_BUFFER_CONFIG *pstCfg,
    EN_SCLDMA_ID_TYPE *enID,unsigned char bReadDMAMode,ST_SCLDMA_ACTIVE_BUFFER_CONFIG *stActiveCfg)
{
    stActiveCfg->u8ActiveBuffer = pstCfg->u8ActiveBuffer;
    stActiveCfg->enRWMode = _MDrv_SCLDMA_SwitchIDForDriverlayer(enSCLDMA_ID,pstCfg->enMemType,enID,bReadDMAMode);
    stActiveCfg->stOnOff.bEn = pstCfg->stOnOff.bEn;
    stActiveCfg->stOnOff.enRWMode =stActiveCfg->enRWMode;
    stActiveCfg->stOnOff.stclk=(ST_SCLDMA_CLK_CONFIG *)(pstCfg->stOnOff.stclk);
}

unsigned char _MDrv_SCLDMA_GetDMABufferActiveIdx
    (EN_SCLDMA_ID_TYPE enID,ST_MDRV_SCLDMA_ACTIVE_BUFFER_CONFIG *pstCfg,ST_SCLDMA_ACTIVE_BUFFER_CONFIG *stActiveCfg)
{
    if(DoubleBufferStatus)
    {
        if(Drv_SCLDMA_GetDMABufferDoneIdx(enID, stActiveCfg))
        {
            pstCfg->u8ActiveBuffer  = stActiveCfg->u8ActiveBuffer;
            pstCfg->u8ISPcount      = stActiveCfg->u8ISPcount;
            pstCfg->u64FRMDoneTime  = stActiveCfg->u64FRMDoneTime;
            return TRUE;
        }
        else
        {
            pstCfg->u8ActiveBuffer  = 0xFF;
            pstCfg->u8ISPcount      = 0;
            pstCfg->u64FRMDoneTime  = 0;
            return FALSE;
        }
    }
    else
    {
        if(Drv_SCLDMA_GetDMABufferDoneIdxWithoutDoublebuffer(enID, stActiveCfg))
        {
            pstCfg->u8ActiveBuffer  = stActiveCfg->u8ActiveBuffer;
            pstCfg->u8ISPcount      = stActiveCfg->u8ISPcount;
            pstCfg->u64FRMDoneTime  = stActiveCfg->u64FRMDoneTime;
            return TRUE;
        }
        else
        {
            pstCfg->u8ActiveBuffer  = 0xFF;
            pstCfg->u8ISPcount      = 0;
            pstCfg->u64FRMDoneTime  = 0;
            return FALSE;
        }
    }
}
void _MDrv_SCLDMA_FillRWCfgStruct
    (EN_MDRV_SCLDMA_ID_TYPE enSCLDMA_ID,ST_MDRV_SCLDMA_BUFFER_CONFIG *pCfg,
    EN_SCLDMA_ID_TYPE *enID,unsigned char bReadDMAMode,ST_SCLDMA_RW_CONFIG *stSCLDMACfg)
{
    unsigned short u16BufferIdx;
    MsOS_Memset(stSCLDMACfg, 0, sizeof(ST_SCLDMA_RW_CONFIG));
    stSCLDMACfg->enRWMode = _MDrv_SCLDMA_SwitchIDForDriverlayer(enSCLDMA_ID,pCfg->enMemType,enID,bReadDMAMode);
    stSCLDMACfg->u16Height = pCfg->u16Height;
    stSCLDMACfg->u16Width  = pCfg->u16Width;
    stSCLDMACfg->u8MaxIdx  = pCfg->u16BufNum -BUFFER_NUMBER_TO_HWIDX_OFFSET;
    stSCLDMACfg->u8Flag    = pCfg->u8Flag;
    stSCLDMACfg->enBuffMode= (pCfg->enBufMDType == E_MDRV_SCLDMA_BUFFER_MD_RING) ? E_SCLDMA_BUF_MD_RING :
                            (pCfg->enBufMDType == E_MDRV_SCLDMA_BUFFER_MD_SWRING) ? E_SCLDMA_BUF_MD_SWRING :
                                                                                E_SCLDMA_BUF_MD_SINGLE;

    for(u16BufferIdx=0; u16BufferIdx<pCfg->u16BufNum; u16BufferIdx++)
    {
        stSCLDMACfg->u32Base_Y[u16BufferIdx] = _Phys2Miu(pCfg->u32Base_Y[u16BufferIdx]);
        stSCLDMACfg->u32Base_C[u16BufferIdx] = _Phys2Miu(pCfg->u32Base_C[u16BufferIdx]);
        stSCLDMACfg->u32Base_V[u16BufferIdx] = _Phys2Miu(pCfg->u32Base_V[u16BufferIdx]);
    }

    stSCLDMACfg->enColor = pCfg->enColorType == E_MDRV_SCLDMA_COLOR_YUV422 ? E_SCLDMA_COLOR_YUV422 :
                          pCfg->enColorType == E_MDRV_SCLDMA_COLOR_YUV420 ? E_SCLDMA_COLOR_YUV420 :
                          pCfg->enColorType == E_MDRV_SCLDMA_COLOR_YCSep422 ? E_SCLDMA_COLOR_YCSep422 :
                          pCfg->enColorType == E_MDRV_SCLDMA_COLOR_YUVSep422 ? E_SCLDMA_COLOR_YUVSep422 :
                          pCfg->enColorType == E_MDRV_SCLDMA_COLOR_YUVSep420 ? E_SCLDMA_COLOR_YUVSep420 :
                            E_SCLDMA_COLOR_NUM;
}
void MDrv_SCLDMA_SetPollWait
    (void *filp, void *pWaitQueueHead, void *pstPollQueue)
{
    MsOS_SetPollWait(filp, pWaitQueueHead, pstPollQueue);
}
unsigned char MDrv_SCLDMA_Suspend(EN_MDRV_SCLDMA_ID_TYPE enSCLDMA_ID)
{
    ST_SCLDMA_SUSPEND_RESUME_CONFIG stSCLDMASuspendResumeCfg;
    EN_SCLDMA_ID_TYPE enID;
    unsigned char bRet = TRUE;

    enID =  enSCLDMA_ID == E_MDRV_SCLDMA_ID_1   ? E_SCLDMA_ID_1_W   :
            enSCLDMA_ID == E_MDRV_SCLDMA_ID_2   ? E_SCLDMA_ID_2_W   :
            enSCLDMA_ID == E_MDRV_SCLDMA_ID_3   ? E_SCLDMA_ID_3_W   :
            enSCLDMA_ID == E_MDRV_SCLDMA_ID_PNL ? E_SCLDMA_ID_PNL_R :
                                                  E_SCLDMA_ID_MAX;
    MsOS_Memset(&stSCLDMASuspendResumeCfg, 0, sizeof(ST_SCLDMA_SUSPEND_RESUME_CONFIG));


    if(Drv_SCLDMA_Suspend(enID, &stSCLDMASuspendResumeCfg))
    {
        bRet = TRUE;
    }
    else
    {
        SCL_ERR( "[SCLDMA]%s %d::Suspend Fail\n",__FUNCTION__, __LINE__);
        bRet = FALSE;
    }

    return bRet;
}

unsigned char MDrv_SCLDMA_Resume(EN_MDRV_SCLDMA_ID_TYPE enSCLDMA_ID)
{
    ST_SCLDMA_SUSPEND_RESUME_CONFIG stSCLDMASuspendResumeCfg;
    EN_SCLDMA_ID_TYPE enID;
    unsigned char bRet = TRUE;;

    enID =  enSCLDMA_ID == E_MDRV_SCLDMA_ID_1   ? E_SCLDMA_ID_1_W   :
            enSCLDMA_ID == E_MDRV_SCLDMA_ID_2   ? E_SCLDMA_ID_2_W   :
            enSCLDMA_ID == E_MDRV_SCLDMA_ID_3   ? E_SCLDMA_ID_3_W   :
            enSCLDMA_ID == E_MDRV_SCLDMA_ID_PNL ? E_SCLDMA_ID_PNL_R :
                                                  E_SCLDMA_ID_MAX;

    MsOS_Memset(&stSCLDMASuspendResumeCfg, 0, sizeof(ST_SCLDMA_SUSPEND_RESUME_CONFIG));

    if(Drv_SCLDMA_Resume(enID, &stSCLDMASuspendResumeCfg))
    {
        bRet = TRUE;
    }
    else
    {
        SCL_ERR( "[SCLDMA]%s %d::Resume Fail\n",__FUNCTION__, __LINE__);
        bRet = FALSE;
    }

    return bRet;
}
unsigned char MDrv_SCLDMA_Exit(unsigned char bCloseISR)
{
    Drv_SCLDMA_Exit(bCloseISR);
    return 1;
}

unsigned char MDrv_SCLDMA_Init(EN_MDRV_SCLDMA_ID_TYPE enSCLDMA_ID, ST_MDRV_SCLDMA_INIT_CONFIG *pCfg)
{
    unsigned char bRet = FALSE;
    ST_SCLDMA_INIT_CONFIG stDMAInitCfg;
    MsOS_Memset(&stDMAInitCfg,0,sizeof(ST_SCLDMA_INIT_CONFIG));
    SCL_DBG(SCL_DBG_LV_SCLDMA()&(Get_DBGMG_SCLDMA(enSCLDMA_ID,0,0)), "[SCLDMA]%s\n", __FUNCTION__);

    stDMAInitCfg.u32RIUBase = pCfg->u32Riubase;
    if(Drv_SCLDMA_Init(&stDMAInitCfg) == FALSE)
    {
        SCL_ERR( "[SCLDMA]%s %d::Init Fail\n",__FUNCTION__, __LINE__);
        bRet = FALSE;
    }
    else
    {
        bRet = TRUE;
    }

    return bRet;
}
void MDrv_SCLDMA_Sys_Init(unsigned char bEn)
{
    Drv_SCLDMA_Sys_Init(bEn);
}
unsigned char MDrv_SCLDMA_GetDoubleBufferStatus(void)
{
    return DoubleBufferStatus;
}
void MDrv_SCLDMA_SetDoubleBufferConfig(unsigned char bEn)
{
    gbDBStatus = bEn;
}
void MDrv_SCLDMA_Release(EN_MDRV_SCLDMA_ID_TYPE enSCLDMA_ID,ST_MDRV_SCLDMA_CLK_CONFIG *stclkcfg)
{
    EN_SCLDMA_ID_TYPE enID;
    ST_SCLDMA_CLK_CONFIG *stclk;
    enID =  enSCLDMA_ID == E_MDRV_SCLDMA_ID_1   ? E_SCLDMA_ID_1_W   :
            enSCLDMA_ID == E_MDRV_SCLDMA_ID_2   ? E_SCLDMA_ID_2_W   :
            enSCLDMA_ID == E_MDRV_SCLDMA_ID_3   ? E_SCLDMA_ID_3_W   :
            enSCLDMA_ID == E_MDRV_SCLDMA_ID_PNL ? E_SCLDMA_ID_PNL_R :
                                                  E_SCLDMA_ID_MAX;

    stclk = (ST_SCLDMA_CLK_CONFIG *)(stclkcfg);
    Drv_SCLDMA_Release(enID,stclk);
}

unsigned char MDrv_SCLDMA_SetDMAReadClientConfig
    (EN_MDRV_SCLDMA_ID_TYPE enSCLDMA_ID, ST_MDRV_SCLDMA_BUFFER_CONFIG *pCfg)
{
    EN_SCLDMA_ID_TYPE enID;
    ST_SCLDMA_RW_CONFIG stSCLDMACfg;
    unsigned char  bRet;
    MsOS_Memset(&stSCLDMACfg,0,sizeof(ST_SCLDMA_RW_CONFIG));
    SCL_DBG(SCL_DBG_LV_SCLDMA()&(Get_DBGMG_SCLDMA(enSCLDMA_ID,pCfg->enMemType,0)), "[SCLDMA]%s\n", __FUNCTION__);

    _MDrv_SCLDMA_FillRWCfgStruct(enSCLDMA_ID,pCfg,&enID,1,&stSCLDMACfg);
    bRet = (unsigned char)Drv_SCLDMA_SetDMAClientConfig(enID, &stSCLDMACfg);

    return bRet;
}

unsigned char MDrv_SCLDMA_SetDMAReadClientTrigger
    (EN_MDRV_SCLDMA_ID_TYPE enSCLDMA_ID, ST_MDRV_SCLDMA_TRIGGER_CONFIG *pCfg)
{
    EN_SCLDMA_ID_TYPE enID;
    ST_SCLDMA_ONOFF_CONFIG stOnOffCfg;
    unsigned char  bRet;
    MsOS_Memset(&stOnOffCfg,0,sizeof(ST_SCLDMA_ONOFF_CONFIG));
    SCL_DBG(SCL_DBG_LV_SCLDMA()&(Get_DBGMG_SCLDMA(enSCLDMA_ID,pCfg->enMemType,1)), "[SCLDMA]%s\n", __FUNCTION__);
    stOnOffCfg.enRWMode = _MDrv_SCLDMA_SwitchIDtoReadModeForDriverlayer(enSCLDMA_ID,pCfg->enMemType,&enID);
    stOnOffCfg.bEn = pCfg->bEn;
    stOnOffCfg.stclk = (ST_SCLDMA_CLK_CONFIG *)(pCfg->stclk);
    bRet = (unsigned char)Drv_SCLDMA_SetDMAClientOnOff(enID ,&stOnOffCfg);

    return bRet;
}

unsigned char MDrv_SCLDMA_SetDMAWriteClientConfig
    (EN_MDRV_SCLDMA_ID_TYPE enSCLDMA_ID, ST_MDRV_SCLDMA_BUFFER_CONFIG *pCfg)
{
    EN_SCLDMA_ID_TYPE enID;
    ST_SCLDMA_RW_CONFIG stSCLDMACfg;
    unsigned char  bRet;
    MsOS_Memset(&stSCLDMACfg,0,sizeof(ST_SCLDMA_RW_CONFIG));
    SCL_DBG(SCL_DBG_LV_SCLDMA()&(Get_DBGMG_SCLDMA(enSCLDMA_ID,pCfg->enMemType,0)),
        "[SCLDMA]%s  ID:%d\n", __FUNCTION__,enSCLDMA_ID);

    _MDrv_SCLDMA_FillRWCfgStruct(enSCLDMA_ID,pCfg,&enID,0,&stSCLDMACfg);

    bRet = (unsigned char)Drv_SCLDMA_SetDMAClientConfig(enID, &stSCLDMACfg);

    return bRet;
}

unsigned char MDrv_SCLDMA_SetDMAWriteClientTrigger
    (EN_MDRV_SCLDMA_ID_TYPE enSCLDMA_ID, ST_MDRV_SCLDMA_TRIGGER_CONFIG *pCfg)
{
    EN_SCLDMA_ID_TYPE enID;
    ST_SCLDMA_ONOFF_CONFIG stOnOffCfg;
    unsigned char bRet;
    MsOS_Memset(&stOnOffCfg,0,sizeof(ST_SCLDMA_ONOFF_CONFIG));
    SCL_DBG(SCL_DBG_LV_SCLDMA()&(Get_DBGMG_SCLDMA(enSCLDMA_ID,pCfg->enMemType,1)), "[SCLDMA]%s\n", __FUNCTION__);

    stOnOffCfg.enRWMode = _MDrv_SCLDMA_SwitchIDtoWriteModeForDriverlayer(enSCLDMA_ID,pCfg->enMemType,&enID);
    stOnOffCfg.bEn = pCfg->bEn;
    stOnOffCfg.stclk = (ST_SCLDMA_CLK_CONFIG *)(pCfg->stclk);

    bRet = (unsigned char)Drv_SCLDMA_SetDMAClientOnOff(enID ,&stOnOffCfg);

    return bRet;
}

unsigned char MDrv_SCLDMA_GetInBufferDoneEvent
    (EN_MDRV_SCLDMA_ID_TYPE enSCLDMA_ID, EN_MDRV_SCLDMA_MEM_TYPE enMemType, ST_MDRV_SCLDMA_BUFFER_DONE_CONFIG *pCfg)
{
    unsigned char bRet;
    ST_SCLDMA_DONE_CONFIG stDonCfg;
    EN_SCLDMA_ID_TYPE enID;
    MsOS_Memset(&stDonCfg,0,sizeof(ST_SCLDMA_DONE_CONFIG));
    stDonCfg.enRWMode = _MDrv_SCLDMA_SwitchIDtoReadModeForDriverlayer(enSCLDMA_ID,enMemType,&enID);
    bRet = (unsigned char)Drv_SCLDMA_GetDMADoneEvent(enID, &stDonCfg);
    pCfg->bDone = (unsigned char)stDonCfg.bDone;

    SCL_DBG(SCL_DBG_LV_SCLDMA()&(Get_DBGMG_SCLDMA(enSCLDMA_ID,enMemType,1)),
        "[SCLDMA]%s: ID:%d, bRet: %d, BufferDone:%d\n", __FUNCTION__, enID, bRet, stDonCfg.bDone);
    return bRet;
}

unsigned char MDrv_SCLDMA_GetOutBufferDoneEvent
    (EN_MDRV_SCLDMA_ID_TYPE enSCLDMA_ID, EN_MDRV_SCLDMA_MEM_TYPE enMemType, ST_MDRV_SCLDMA_BUFFER_DONE_CONFIG *pCfg)
{
    unsigned char bRet;
    ST_SCLDMA_DONE_CONFIG stDonCfg;
    EN_SCLDMA_ID_TYPE enID;
    MsOS_Memset(&stDonCfg,0,sizeof(ST_SCLDMA_DONE_CONFIG));
    stDonCfg.enRWMode = _MDrv_SCLDMA_SwitchIDtoWriteModeForDriverlayer(enSCLDMA_ID,enMemType,&enID);
    bRet = (unsigned char)Drv_SCLDMA_GetDMADoneEvent(enID, &stDonCfg);
    pCfg->bDone = (unsigned char)stDonCfg.bDone;

    SCL_DBG(SCL_DBG_LV_SCLDMA()&(Get_DBGMG_SCLDMA(enSCLDMA_ID,enMemType,1)), "[SCLDMA]%s: ID:%d, bRet: %d, BufferDone:%d\n", __FUNCTION__, enID, bRet, stDonCfg.bDone);
    return bRet;
}

unsigned char MDrv_SCLDMA_GetDMAReadBufferActiveIdx
    (EN_MDRV_SCLDMA_ID_TYPE enSCLDMA_ID, ST_MDRV_SCLDMA_ACTIVE_BUFFER_CONFIG *pstCfg)
{
    ST_SCLDMA_ACTIVE_BUFFER_CONFIG stActiveCfg;
    EN_SCLDMA_ID_TYPE enID;
    unsigned char bRet;
    MsOS_Memset(&stActiveCfg,0,sizeof(ST_SCLDMA_ACTIVE_BUFFER_CONFIG));
    SCL_DBG(SCL_DBG_LV_SCLDMA()&(Get_DBGMG_SCLDMA(enSCLDMA_ID,pstCfg->enMemType,1)), "[SCLDMA]%s\n", __FUNCTION__);
    _MDrv_SCLDMA_FillActiveBufferStruct(enSCLDMA_ID,pstCfg,&enID,1,&stActiveCfg);
    bRet = _MDrv_SCLDMA_GetDMABufferActiveIdx(enID, pstCfg, &stActiveCfg);
    return bRet;

}
unsigned char MDrv_SCLDMA_BufferQueueHandle
    (EN_MDRV_SCLDMA_ID_TYPE enSCLDMA_ID, ST_MDRV_SCLDMA_BUFFER_QUEUE_CONFIG *pstCfg)
{
    unsigned char bRet =0;
    EN_SCLDMA_ID_TYPE enID;
    ST_SCLDMA_BUFFER_QUEUE_CONFIG stCfg;
    MsOS_Memset(&stCfg,0,sizeof(ST_SCLDMA_BUFFER_QUEUE_CONFIG));
    SCL_DBG(SCL_DBG_LV_SCLDMA()&(Get_DBGMG_SCLDMA(enSCLDMA_ID,pstCfg->enMemType,1)), "[SCLDMA]%s\n", __FUNCTION__);
    stCfg.enRWMode = _MDrv_SCLDMA_SwitchIDtoWriteModeForDriverlayer(enSCLDMA_ID,pstCfg->enMemType,&enID);
    stCfg.enID = enID;
    stCfg.u8AccessId = pstCfg->u8EnqueueIdx;
    _MDrv_SCLDMA_BufferQueueHandlerByUsedType(pstCfg->enUsedType,&stCfg);
    if(_ISQueueNeedCopyToUser(pstCfg->enUsedType))
    {
        if(stCfg.pstRead != NULL)
        {
            MsOS_Memcpy(&pstCfg->stRead,stCfg.pstRead,MDRV_SCLDMA_BUFFER_QUEUE_OFFSET);
            pstCfg->u8InQueueCount = stCfg.u8InQueueCount;
            pstCfg->u8EnqueueIdx   = stCfg.u8AccessId;
            bRet = 1;
        }
    }
    return bRet;
}
unsigned char MDrv_SCLDMA_GetDMAWriteBufferAcitveIdx
    (EN_MDRV_SCLDMA_ID_TYPE enSCLDMA_ID, ST_MDRV_SCLDMA_ACTIVE_BUFFER_CONFIG *pstCfg)
{
    ST_SCLDMA_ACTIVE_BUFFER_CONFIG stActiveCfg;
    EN_SCLDMA_ID_TYPE enID;
    unsigned char bRet;
    MsOS_Memset(&stActiveCfg,0,sizeof(ST_SCLDMA_ACTIVE_BUFFER_CONFIG));
    SCL_DBG(SCL_DBG_LV_SCLDMA()&(Get_DBGMG_SCLDMA(enSCLDMA_ID, pstCfg->enMemType,1)), "[SCLDMA]%s\n", __FUNCTION__);
    _MDrv_SCLDMA_FillActiveBufferStruct(enSCLDMA_ID, pstCfg, &enID,0,&stActiveCfg);
    bRet = _MDrv_SCLDMA_GetDMABufferActiveIdx(enID, pstCfg, &stActiveCfg);
    return bRet;
}
void MDrv_SCLDMA_SetForceCloseDMAClient
    (EN_MDRV_SCLDMA_ID_TYPE enSCLDMA_ID,EN_MDRV_SCLDMA_MEM_TYPE enMemType,
    unsigned char bReadDMAMode,unsigned char bEnForceClose)
{
    EN_SCLDMA_ID_TYPE enID;
    EN_SCLDMA_RW_MODE_TYPE enRWMode;
    enRWMode = _MDrv_SCLDMA_SwitchIDForDriverlayer(enSCLDMA_ID,enMemType,&enID,bReadDMAMode);
    Drv_SCLDMA_SetForceCloseDMA(enID,enRWMode,bEnForceClose);
}
void MDrv_SCLDMA_ResetTrigCountByClient
    (EN_MDRV_SCLDMA_ID_TYPE enSCLDMA_ID,EN_MDRV_SCLDMA_MEM_TYPE enMemType,unsigned char bReadDMAMode)
{
    EN_SCLDMA_ID_TYPE enID;
    EN_SCLDMA_RW_MODE_TYPE enRWMode;
    enRWMode = _MDrv_SCLDMA_SwitchIDForDriverlayer(enSCLDMA_ID,enMemType,&enID,bReadDMAMode);
    Drv_SCLDMA_ResetTrigCountByClient(enID,enRWMode);
}

void MDrv_SCLDMA_ResetTrigCountAllClient(void)
{
    Drv_SCLDMA_ResetTrigCountByClient(E_SCLDMA_ID_MAX,E_SCLDMA_RW_NUM);
}

void MDrv_SCLDMA_ClkClose(ST_MDRV_SCLDMA_CLK_CONFIG* stclk)
{
   Drv_SCLDMA_ClkClose((ST_SCLDMA_CLK_CONFIG *)stclk);
}
void * MDrv_SCLDMA_GetWaitQueueHead(EN_MDRV_SCLDMA_ID_TYPE enSCLDMA_ID)
{
    EN_SCLDMA_ID_TYPE enID;

    enID =  enSCLDMA_ID == E_MDRV_SCLDMA_ID_1   ? E_SCLDMA_ID_1_W   :
            enSCLDMA_ID == E_MDRV_SCLDMA_ID_2   ? E_SCLDMA_ID_2_W   :
            enSCLDMA_ID == E_MDRV_SCLDMA_ID_3   ? E_SCLDMA_ID_3_W   :
                                                  E_SCLDMA_ID_MAX;
    if(enID == E_SCLDMA_ID_MAX)
    {
        return 0;
    }
    else
    {
        return Drv_SCLDMA_GetWaitQueueHead(enID);
    }
}

void MDrv_SCLDMA_GetDMAInformationByClient
    (EN_MDRV_SCLDMA_ID_TYPE enSCLDMA_ID,EN_MDRV_SCLDMA_MEM_TYPE enMemType,unsigned char bReadDMAMode,ST_MDRV_SCLDMA_ATTR_TYPE *stSendToIOCfg)
{
    EN_SCLDMA_ID_TYPE enID;
    EN_SCLDMA_RW_MODE_TYPE enRWMode;
    ST_SCLDMA_ATTR_TYPE stDrvDMACfg;
    MsOS_Memset(&stDrvDMACfg,0,sizeof(ST_SCLDMA_ATTR_TYPE));
    enRWMode = _MDrv_SCLDMA_SwitchIDForDriverlayer(enSCLDMA_ID,enMemType,&enID,bReadDMAMode);
    Drv_SCLDMA_GetDMAInformationByClient(enID, enRWMode,&stDrvDMACfg);
    _MDrv_SCLDMA_FillDMAInfoStruct(&stDrvDMACfg,stSendToIOCfg);
}
unsigned char _MDrv_IsClosedPacking(ST_MDRV_SCLDMA_ATTR_TYPE stScldmaAttr)
{
    unsigned char bEn = 0;
    if(stScldmaAttr.enColorType == E_MDRV_SCLDMA_COLOR_YUV422 )
    {
        bEn = 1;
    }
    else if(stScldmaAttr.enColorType == E_MDRV_SCLDMA_COLOR_YCSep422 ||
        stScldmaAttr.enColorType == E_MDRV_SCLDMA_COLOR_YUV420)
    {
        bEn = ((stScldmaAttr.u32Base_Y[0]+(stScldmaAttr.u16DMAH*stScldmaAttr.u16DMAV))==stScldmaAttr.u32Base_C[0]) ? 1 :0 ;
    }
    else if(stScldmaAttr.enColorType == E_MDRV_SCLDMA_COLOR_YUVSep422 ||
        stScldmaAttr.enColorType == E_MDRV_SCLDMA_COLOR_YUVSep420)
    {
        bEn = ((stScldmaAttr.u32Base_Y[0]+(stScldmaAttr.u16DMAH*stScldmaAttr.u16DMAV))==stScldmaAttr.u32Base_C[0]) ? 1 :0 ;
        if(stScldmaAttr.enColorType == E_MDRV_SCLDMA_COLOR_YUVSep422 && bEn)
        {
            bEn = ((stScldmaAttr.u32Base_C[0]+(stScldmaAttr.u16DMAH*stScldmaAttr.u16DMAV/2))==stScldmaAttr.u32Base_V[0]) ? 1 :0 ;
        }
        else if(stScldmaAttr.enColorType == E_MDRV_SCLDMA_COLOR_YUVSep420 && bEn)
        {
            bEn = ((stScldmaAttr.u32Base_C[0]+(stScldmaAttr.u16DMAH*stScldmaAttr.u16DMAV/4))==stScldmaAttr.u32Base_V[0]) ? 1 :0 ;
        }
    }
    return bEn;
}
ssize_t MDrv_SCLDMA_ProcShow
    (char *buf, EN_MDRV_SCLDMA_ID_TYPE enID, EN_MDRV_SCLDMA_MEM_TYPE enMem, unsigned char bRread)
{
    ST_MDRV_SCLDMA_ATTR_TYPE stScldmaAttr;
    //out =0,in=1
    char *p8StrBuf = buf;
    char *p8StrEnd = buf + PAGE_SIZE;
    int u32idx;
    unsigned u8bCP;
    //out =0,in=1
    MsOS_Memset(&stScldmaAttr,0,sizeof(ST_MDRV_SCLDMA_ATTR_TYPE));
    MDrv_SCLDMA_GetDMAInformationByClient(enID, enMem,bRread,&stScldmaAttr);
    p8StrBuf += scnprintf(p8StrBuf, p8StrEnd - p8StrBuf
        , "========================SCL PROC FRAMEWORK======================\n");
    p8StrBuf += scnprintf(p8StrBuf, p8StrEnd - p8StrBuf
        , "------------------------ %s %s CLIENT----------------------\n",
        PARSING_SCLDMA_IOID(enID),PARSING_SCLDMA_IOMEM(enMem));
    p8StrBuf += scnprintf(p8StrBuf, p8StrEnd - p8StrBuf
        , "DMA Enable: %hhd\n",stScldmaAttr.bDMAEn);
    p8StrBuf += scnprintf(p8StrBuf, p8StrEnd - p8StrBuf
        , "output width: %hd, output height: %hd\n",stScldmaAttr.u16DMAH,stScldmaAttr.u16DMAV);
    p8StrBuf += scnprintf(p8StrBuf, p8StrEnd - p8StrBuf
        , "DMA color format: %s\n",PARSING_SCLDMA_IOCOLOR(stScldmaAttr.enColorType));
    p8StrBuf += scnprintf(p8StrBuf, p8StrEnd - p8StrBuf
        , "DMA trigger mode: %s\n",PARSING_SCLDMA_IOBUFMD(stScldmaAttr.enBufMDType));
    p8StrBuf += scnprintf(p8StrBuf, p8StrEnd - p8StrBuf
        , "DMA Buffer Num: %hd\n",stScldmaAttr.u16BufNum);
    if(stScldmaAttr.enColorType == E_MDRV_SCLDMA_COLOR_YUV422 ||
        stScldmaAttr.enColorType == E_MDRV_SCLDMA_COLOR_YUVSep422||
        stScldmaAttr.enColorType == E_MDRV_SCLDMA_COLOR_YCSep422)
    {
        p8StrBuf += scnprintf(p8StrBuf, p8StrEnd - p8StrBuf
            , "DMA Buffer length(presume): %d\n",(int)(stScldmaAttr.u16DMAH*stScldmaAttr.u16DMAV*2));
    }
    else
    {
        p8StrBuf += scnprintf(p8StrBuf, p8StrEnd - p8StrBuf
            , "DMA Buffer length(presume): %d\n",(int)(stScldmaAttr.u16DMAH*stScldmaAttr.u16DMAV*3/2));
    }
    for(u32idx=0 ;u32idx<stScldmaAttr.u16BufNum;u32idx++)
    {
        p8StrBuf += scnprintf(p8StrBuf, p8StrEnd - p8StrBuf
            , "DMA Buffer Y Address[%d]: 2%lx\n",u32idx,stScldmaAttr.u32Base_Y[u32idx]);
        p8StrBuf += scnprintf(p8StrBuf, p8StrEnd - p8StrBuf
            , "DMA Buffer C Address[%d]: 2%lx\n",u32idx,stScldmaAttr.u32Base_C[u32idx]);
        p8StrBuf += scnprintf(p8StrBuf, p8StrEnd - p8StrBuf
            , "DMA Buffer V Address[%d]: 2%lx\n",u32idx,stScldmaAttr.u32Base_V[u32idx]);
    }
    u8bCP = _MDrv_IsClosedPacking(stScldmaAttr);
    p8StrBuf += scnprintf(p8StrBuf, p8StrEnd - p8StrBuf
        , "DMA Buffer Closed Packing: %hhx\n",u8bCP);
    p8StrBuf += scnprintf(p8StrBuf, p8StrEnd - p8StrBuf
        , "DMA Buffer Read Pointer: %hhx\n",stScldmaAttr.bDMAReadIdx);
    p8StrBuf += scnprintf(p8StrBuf, p8StrEnd - p8StrBuf
        , "DMA Buffer Write Pointer: %hhx\n",stScldmaAttr.bDMAWriteIdx);
    p8StrBuf += scnprintf(p8StrBuf, p8StrEnd - p8StrBuf
        , "DMA Buffer Count: %hhx\n",stScldmaAttr.u8Count);
    p8StrBuf += scnprintf(p8StrBuf, p8StrEnd - p8StrBuf
        , "DMA Buffer Status: %hhx\n",stScldmaAttr.bDMAFlag);
    p8StrBuf += scnprintf(p8StrBuf, p8StrEnd - p8StrBuf
        , "FrameDoneTime     : %lu\n",stScldmaAttr.u32FrameDoneTime);
    p8StrBuf += scnprintf(p8StrBuf, p8StrEnd - p8StrBuf
        , "ToGetPollTime     : %lu\n",stScldmaAttr.u32SendTime);
    if(stScldmaAttr.bSendPoll &0x10)
    {
        p8StrBuf += scnprintf(p8StrBuf, p8StrEnd - p8StrBuf
            , "Poll hold on, last Status: %hhx\n",(stScldmaAttr.bSendPoll&0xf));
    }
    else
    {
        p8StrBuf += scnprintf(p8StrBuf, p8StrEnd - p8StrBuf
            , "Poll on time, Status: %hhx\n",(stScldmaAttr.bSendPoll&0xf));
    }
    p8StrBuf += scnprintf(p8StrBuf, p8StrEnd - p8StrBuf
        , "output V line: %hd trig Count: %ld \n",stScldmaAttr.u16DMAcount,stScldmaAttr.u32Trigcount);
    p8StrBuf += scnprintf(p8StrBuf, p8StrEnd - p8StrBuf
        , "SWReTrig Count: %hhu \n",stScldmaAttr.u8ResetCount);
    p8StrBuf += scnprintf(p8StrBuf, p8StrEnd - p8StrBuf
        , "DMA ignore Count: %hhu \n",stScldmaAttr.u8DMAErrCount);
    p8StrBuf += scnprintf(p8StrBuf, p8StrEnd - p8StrBuf
        , "========================SCL PROC FRAMEWORK======================\n");
    return (p8StrBuf - buf);
}
