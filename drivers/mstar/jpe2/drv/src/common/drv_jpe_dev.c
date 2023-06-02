////////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2006-2017 MStar Semiconductor, Inc.
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

#include "hal_jpe_ios.h"
#include "hal_jpe_ops.h"
#include "drv_jpe_io_st_kernel.h"

static u32 _GetTime(void)
{
    CamOsTimespec_t tTime;
    CamOsGetMonotonicTime(&tTime);
    return ((tTime.nSec*1000000+ tTime.nNanoSec/1000)/1000);
}


int JpeDevRegister(JpeDev_t* pDev, JpeCtx_t* pCtx)
{
    int i = 0;
    CamOsMutexLock(&pDev->m_mutex);
    pCtx->i_index = -1;
    while(i < JPE_STREAM_NR)
    {
        if(pDev->user[i++].pCtx)
            continue;
        pDev->user[--i].pCtx = pCtx;
        pCtx->p_device = pDev;
        pCtx->i_index = i;
        break;
    }
    CamOsMutexUnlock(&pDev->m_mutex);
    return pCtx->i_index;
}


JPE_IOC_RET_STATUS_e JpeDevUnregister(JpeDev_t* pDev, JpeCtx_t* pCtx)
{
    if((!pDev) || (!pCtx))
    {
        return JPE_IOC_RET_FAIL;
    }

    CamOsMutexLock(&pDev->m_mutex);
    pDev->user[pCtx->i_index].pCtx = NULL;
    pCtx->p_device = NULL;
    pCtx->i_index = -1;
    CamOsMutexUnlock(&pDev->m_mutex);
    return JPE_IOC_RET_SUCCESS;
}


int JpeDevPushJob(JpeDev_t* pDev, JpeCtx_t* pCtxIn)
{
    JpeOpsCB_t* pOpsCB = pCtxIn->p_handle;
    JpeOps_t* pOps = pCtxIn->p_handle;
    JpeIos_t* pIos = pDev->p_asicip;
    JpeIosCB_t* pIosCB = pDev->p_asicip;
    JpeJob_t* pJob = pOpsCB->jpeJob(pOpsCB);
    JPE_IOC_RET_STATUS_e eStatus = JPE_IOC_RET_SUCCESS;
    int otmr, itmr, id = pCtxIn->i_index;

    if((!pOps) || (!pJob) || (!pIosCB))
    {
        return JPE_IOC_RET_FAIL;
    }

    otmr = _GetTime();
    CamOsTsemDown(&pDev->tGetBitsSem);
    itmr = _GetTime();
    pDev->i_state = JPE_DEV_STATE_BUSY;

    eStatus = pIosCB->encFire(pIosCB, &pOps->tJpeHalHandle, pJob);
    if(JPE_IOC_RET_SUCCESS != eStatus)
    {
        CamOsTsemUp(&pDev->tGetBitsSem);
        return (int)eStatus;
    }
    CamOsTsemWait(&pDev->m_wqh);
    //CamOsTsemTimedWait(&pDev->m_wqh, 100*1000);

    // After encode done, we should change
    // Below 2 arguments remain the same.
    pCtxIn->tEncOutBuf.nOutputSize = pIos->nEncodeSize;
    pCtxIn->tEncOutBuf.eState = pIos->eJpeDevStatus;

    itmr = _GetTime() - itmr;
    CamOsTsemUp(&pDev->tGetBitsSem);
    otmr = _GetTime() - otmr;

    //CamOsDebug("itmr:%d,  otmr:%d \n", itmr, otmr);

    // TODO: update below counter value to /proc
    if(otmr > pDev->i_thresh && pDev->i_thresh > 0)
        pDev->i_counts[id][0]++;
    if(otmr > pDev->i_counts[id][1])
        pDev->i_counts[id][1] = otmr;
    if(itmr > pDev->i_counts[id][2])
        pDev->i_counts[id][2] = itmr;
    if(pJob->i_tick > pDev->i_counts[id][3])
        pDev->i_counts[id][3] = pJob->i_tick;
    return eStatus;
}


int JpeDevIsrFnx(JpeDev_t* pDev)
{
    JpeIosCB_t* pIosCB = pDev->p_asicip;

    if(!pIosCB->isrFunc(pIosCB, 0))
    {
        pDev->i_state = JPE_DEV_STATE_IDLE;
        CamOsTsemSignal(&pDev->m_wqh);
    }

    return 0;
}