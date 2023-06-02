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
#include "_drv_jpe_dev.h"
#include "_drv_jpe_ctx.h"
#include "drv_jpe_module.h"

static JpeDev_t* _gpDev = NULL;

//=============================================================================
// Description:
//     ISR handler
// Author:
//      Albert.Liao.
// Input:
//      none.
// Output:
//      none.
//=============================================================================
static void _JpeIsr()
{
    if(_gpDev)
    {
        JpeDevIsrFnx(_gpDev);
    }
    else
    {
        JPE_MSG(JPE_MSG_ERR, "_gpDev==NULL, JPE device is removed\n");
    }
}


//=============================================================================
// Description:
//     File open handler
// Author:
//      Albert.Liao.
// Input:
//      pDev: JPEG hardware device handle
//      pCtx: JPEG user handle
// Output:
//      pCtx: a new JPEG user handle
//=============================================================================
JpeCtx_t* JpeOpen(JpeDev_t* pDev, JpeCtx_t* pCtx)
{
    pCtx = JpeCtxAcquire(pDev);
    if(pCtx == NULL)
    {
        JPE_MSG(JPE_MSG_ERR, "JpeCtxAcquire Fail \n");
        return pCtx;
    }

    if(0 <= JpeDevRegister(pDev, pCtx))
    {
        return pCtx;
    }

    JPE_MSG(JPE_MSG_ERR, "JpeDevRegister Fail \n");
    pCtx->release(pCtx);

    return pCtx;
}


//=============================================================================
// Description:
//     File close handler
// Author:
//      Albert.Liao.
// Input:
//      pDev: JPEG hardware device handle
//      pCtx: JPEG user handle
// Output:
//      JPE_IOC_RET_SUCCESS: Success
//      JPE_IOC_RET_FAIL: Failure
//=============================================================================
JPE_IOC_RET_STATUS_e JpeRelease(JpeDev_t* pDev, JpeCtx_t* pCtx)
{
    JPE_IOC_RET_STATUS_e eStatus;

    eStatus = JpeDevUnregister(pDev, pCtx);
    if(JPE_IOC_RET_SUCCESS != eStatus)
    {
        JPE_MSG(JPE_MSG_ERR, "JpeDevUnregister Fail \n");
        return eStatus;
    }

    if(pCtx)
    {
        pCtx->release(pCtx);
    }

    return JPE_IOC_RET_SUCCESS;
}


//=============================================================================
// Description:
//     Platform device probe handler
// Author:
//      Albert.Liao.
// Input:
//      pDev: JPEG hardware device handle
// Output:
//      JpeDev_t: JPEG hardware device handle
//=============================================================================
JpeDev_t* JpeProbe(JpeDev_t* pDev)
{
    JpeIosCB_t* mios = NULL;
    JpeRegIndex_t mregs;
    MsIntInitParam_u uInitParam;
    unsigned long res_base = JPE_OFFSET + IO_START_ADDRESS;

    int res_size = 0x100;
    JPE_IOC_RET_STATUS_e eStatus;

    if(_gpDev)
        return _gpDev;

    do
    {
        pDev = CamOsMemCalloc(1, sizeof(JpeDev_t));
        if(pDev == NULL)
        {
            JPE_MSG(JPE_MSG_ERR, "> Create Mdev Fail \n");
            break;
        }
        CamOsMutexInit(&pDev->m_mutex);
        CamOsTsemInit(&pDev->tGetBitsSem, 1);
        CamOsTsemInit(&pDev->m_wqh, JPE_DEV_STATE_IDLE);

        pDev->p_asicip = JpeIosAcquire("jpe");
        mios = pDev->p_asicip;

        mregs.i_id = 0;
        mregs.base = (unsigned long*)res_base;
        mregs.size = res_size;

        eStatus = mios->setBank(mios, &mregs);
        if(JPE_IOC_RET_SUCCESS != eStatus)
        {
            JPE_MSG(JPE_MSG_ERR, "> set_bank Fail \n");
        }
        uInitParam.intc.eMap         = INTC_MAP_IRQ;
        uInitParam.intc.ePriority    = INTC_PRIORITY_7;
        uInitParam.intc.pfnIsr       = _JpeIsr;
        MsInitInterrupt(&uInitParam, MS_INT_NUM_IRQ_JPE);
        MsUnmaskInterrupt(MS_INT_NUM_IRQ_JPE);

        _gpDev = pDev;
        return pDev;
    }
    while(0);

    return _gpDev;
}


//=============================================================================
// Description:
//     File close handler
// Author:
//      Albert.Liao.
// Input:
//      pDev: JPEG hardware device handle
// Output:
//      JPE_IOC_RET_SUCCESS: Success
//      JPE_IOC_RET_FAIL: Failure
//=============================================================================
JPE_IOC_RET_STATUS_e JpeRemove(JpeDev_t* pDev)
{
    JpeIosCB_t* pIos = NULL;

    if(pDev)
    {
        CamOsMutexDestroy(&pDev->m_mutex);
        CamOsTsemDeinit(&pDev->m_wqh);
        CamOsTsemDeinit(&pDev->tGetBitsSem);
        pIos = pDev->p_asicip;
    }
    else
    {
        JPE_MSG(JPE_MSG_ERR, "pDev==NULL, %s fail\n", __func__);
    }

    MsMaskInterrupt(MS_INT_NUM_IRQ_JPE);
    MsClearInterrupt(MS_INT_NUM_IRQ_JPE);

    if(pIos)
    {
        pIos->release(pIos);
    }

    MsReleaseMemory(pDev);

    _gpDev = NULL;
    return JPE_IOC_RET_SUCCESS;
}