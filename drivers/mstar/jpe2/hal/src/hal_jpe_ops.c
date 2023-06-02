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

#ifndef ARRAY_SIZE  // include/linux/kernel.h already defined this macro
#define ARRAY_SIZE(a) (sizeof(a)/sizeof(a[0]))
#endif

#define TIME_OUT_CNT 10
#define QTABLE_MAX  255
#define QTABLE_MIN  1

#ifndef _MAX
#define _MAX(a,b)   ((a)>(b)?(a):(b))
#endif
#ifndef _MIN
#define _MIN(a,b)   ((a)<(b)?(a):(b))
#endif

static JPE_IOC_RET_STATUS_e _JpeCheckCfg(JpeCfg_t *pCfg)
{
    int align;

    // Support JPEG only
    if(pCfg->eCodecFormat != JPE_CODEC_JPEG)
        return JPE_IOC_RET_FMT_NOT_SUPPORT;

    // Alignment check
    align = (pCfg->eRawFormat == JPE_RAW_NV12 || pCfg->eRawFormat == JPE_RAW_NV21) ? 16 : 8;
    if(pCfg->nWidth % align)
    {
        JPE_MSG(JPE_MSG_ERR, "source width is not aligned to %d\n", align);
        return JPE_IOC_RET_BAD_INBUF;
    }
    if(pCfg->nHeight % align)
    {
        JPE_MSG(JPE_MSG_ERR, "source height is not aligned to %d\n", align);
        return JPE_IOC_RET_BAD_INBUF;
    }

    // Input buffer check
    if(pCfg->InBuf[JPE_COLOR_PLAN_LUMA].nAddr == 0 || pCfg->InBuf[JPE_COLOR_PLAN_LUMA].nSize == 0)
    {
        JPE_MSG(JPE_MSG_ERR, "Input buffer is not assigned\n");
        return JPE_IOC_RET_BAD_INBUF;
    }

    // Output buffer check
    if(pCfg->OutBuf.nAddr == 0 || pCfg->OutBuf.nSize == 0)
    {
        // When jpeg init, user can provide a empty output buffer.
        //JPE_MSG(JPE_MSG_ERR, "Output buffer is not assigned\n");
        return JPE_IOC_RET_BAD_OUTBUF;
    }

    return JPE_IOC_RET_SUCCESS;
}

static JPE_IOC_RET_STATUS_e _JpeSetInBuf(JpeHalHandle_t* pJpeHalHandle, JpeCfg_t *pJpeCfg)
{
    u32 size;
    JpeHalInBufCfg_t  *pInBufCfg = &pJpeHalHandle->tJpeHalInBufCfg;

    JPE_MSG(JPE_MSG_DEBUG, "set 0x%08lX, size %ld as input buffer\n", pJpeCfg->InBuf[JPE_COLOR_PLAN_LUMA].nAddr, pJpeCfg->InBuf[JPE_COLOR_PLAN_LUMA].nSize);

    // Set input buffer
    switch(pJpeCfg->eRawFormat)
    {
        case JPE_RAW_YUYV:
        case JPE_RAW_YVYU:
            if(pJpeCfg->eInBufMode == JPE_IBUF_FRAME_MODE)
            {
                pInBufCfg->nInBufYAddr[0] = pJpeCfg->InBuf[JPE_COLOR_PLAN_LUMA].nAddr;
                pInBufCfg->nInBufYAddr[1] = 0;
                pInBufCfg->nInBufCAddr[0] = 0;
                pInBufCfg->nInBufCAddr[1] = 0;
            }
            else if(pJpeCfg->eInBufMode == JPE_IBUF_ROW_MODE)
            {
                // Assume the user allocate big enough buffer and scaler also follow the same order
                // If not, we may change the IF to follow the behavior of scaler
                size = pJpeCfg->InBuf[JPE_COLOR_PLAN_LUMA].nSize / 2;
                pInBufCfg->nInBufYAddr[0] = pJpeCfg->InBuf[JPE_COLOR_PLAN_LUMA].nAddr;
                pInBufCfg->nInBufYAddr[1] = pInBufCfg->nInBufYAddr[0] + size;
                pInBufCfg->nInBufCAddr[0] = 0;
                pInBufCfg->nInBufCAddr[1] = 0;
            }
            else
            {
                return JPE_IOC_RET_BAD_INBUF;
            }
            break;
        case JPE_RAW_NV12:
        case JPE_RAW_NV21:
            if(pJpeCfg->eInBufMode == JPE_IBUF_FRAME_MODE)
            {
                pInBufCfg->nInBufYAddr[0] = pJpeCfg->InBuf[JPE_COLOR_PLAN_LUMA].nAddr;
                pInBufCfg->nInBufYAddr[1] = 0;
                pInBufCfg->nInBufCAddr[0] = pJpeCfg->InBuf[JPE_COLOR_PLAN_CHROMA].nAddr;
                pInBufCfg->nInBufCAddr[1] = 0;
            }
            else if(pJpeCfg->eInBufMode == JPE_IBUF_ROW_MODE)
            {
                // TODO: the luma size seems incorrect
                // For YUV420P (NV12 or NV21)
                // luma should be      InBuf[JPE_COLOR_PLAN_LUMA].nSize
                // chroma should be  InBuf[JPE_COLOR_PLAN_CHROMA].nSize/2  (U+V)

                // Assume the user allocate big enough buffer and scaler also follow the same order
                // If not, we may change the IF to follow the behavior of scaler
                size =  pJpeCfg->InBuf[JPE_COLOR_PLAN_LUMA].nSize / 4;
                pInBufCfg->nInBufYAddr[0] = pJpeCfg->InBuf[JPE_COLOR_PLAN_LUMA].nAddr;
                pInBufCfg->nInBufYAddr[1] = pInBufCfg->nInBufYAddr[0] + size;
                pInBufCfg->nInBufCAddr[0] = pJpeCfg->InBuf[JPE_COLOR_PLAN_CHROMA].nAddr;
                pInBufCfg->nInBufCAddr[1] = pInBufCfg->nInBufCAddr[0] + size;
            }
            else
            {
                return JPE_IOC_RET_BAD_INBUF;
            }
            break;
    }

    return JPE_IOC_RET_SUCCESS;
}

static JPE_IOC_RET_STATUS_e _JpeSetQTable(JpeHalHandle_t* pJpeHalHandle, const u16 *q_table_y, const u16 *q_table_c, u32 size, const u16 q_scale)
{
    u16 *pYTable, *pCTable;
    JPE_IOC_RET_STATUS_e ret = JPE_IOC_RET_SUCCESS;
    int i;

    if(!pJpeHalHandle)
        return JPE_IOC_RET_FAIL;

    pYTable = pJpeHalHandle->YQTable;
    pCTable = pJpeHalHandle->CQTable;

    for(i = 0; i < size; i++)
    {
        pYTable[i] = (u16)(((u32)q_table_y[i] * (u32)q_scale + 50) / 100);
        //pYTable[i] = pYTable[i] ? pYTable[i] : 1;
        pYTable[i] = _MAX(QTABLE_MIN, _MIN(pYTable[i], QTABLE_MAX));
        pCTable[i] = (u16)(((u32)q_table_c[i] * (u32)q_scale + 50) / 100);
        //pCTable[i] = pCTable[i] ? pCTable[i] : 1;
        pCTable[i] = _MAX(QTABLE_MIN, _MIN(pCTable[i], QTABLE_MAX));
    }

    return ret;
}

static void* _JpeJob(JpeOpsCB_t* mops)
{
    JpeOps_t* pJpeOps = (JpeOps_t*)mops;

    return &(pJpeOps->tJpeHalHandle.tJpeReg);
}

static void _OpsFree(JpeOpsCB_t* mops)
{
    CamOsMemRelease((void*)mops);
}

static JPE_IOC_RET_STATUS_e _Init(JpeOpsCB_t* pJpeOpsCBIn, JpeCfg_t* pJpeCfgIn)
{
    JPE_IOC_RET_STATUS_e eStatus = JPE_IOC_RET_SUCCESS;

    if(!pJpeCfgIn)
    {
        return JPE_IOC_RET_FAIL;
    }

    eStatus = _JpeCheckCfg(pJpeCfgIn);
    // The output buffer information can be set latter, when encode frame
    if((eStatus != JPE_IOC_RET_SUCCESS) && (eStatus != JPE_IOC_RET_BAD_OUTBUF))
    {
        JPE_MSG(JPE_MSG_ERR, "some parameter settings are wrong...\n");
        return eStatus;
    }

    return JPE_IOC_RET_SUCCESS;
}

static JPE_IOC_RET_STATUS_e  _GetCaps(JpeOpsCB_t* pOpsCBIn, JpeCaps_t *pCaps)
{
    if(pCaps == 0)
    {
        return JPE_IOC_RET_BAD_NULLPTR;
    }

    pCaps->nBufferModeSupport = JPE_HAL_BUFFER_ROW_MODE | JPE_HAL_BUFFER_FRAME_MODE;
    pCaps->nCodecSupport = JPE_HAL_CODEC_JPEG;
    pCaps->nOutBufSLogAddrAlign = 4;
    pCaps->nRefCLogAddrAlign[0] = 256;
    pCaps->nRefCLogAddrAlign[1] = 256;
    pCaps->nRefYLogAddrAlign[0] = 256;
    pCaps->nRefYLogAddrAlign[1] = 256;
    pCaps->nSclHandShakeSupport = JPE_HAL_SCL_HANDSHAKE_ON;

    return JPE_IOC_RET_SUCCESS;
}

static JPE_IOC_RET_STATUS_e _SetConf(JpeOpsCB_t* pJpeOpsIn, JpeCfg_t* pJpeCfgIn)
{
    JPE_IOC_RET_STATUS_e ret = JPE_IOC_RET_FAIL;
    JpeOps_t* pJpeOps = (JpeOps_t*)pJpeOpsIn;
    JpeHalHandle_t *pJpeHalHandle = &pJpeOps->tJpeHalHandle;
    JpeHalOutBufCfg_t*   pJpeHalOutBufCfg = &pJpeHalHandle->tJpeHalOutBufCfg;

    JpeReg_t* pJpeReg = &pJpeHalHandle->tJpeReg;

    if((!pJpeOpsIn) || (!pJpeCfgIn))
    {
        JPE_MSG(JPE_MSG_ERR, "wrong parameters, pJpeOpsIn=%p, pJpeCfgIn=%p\n", pJpeOpsIn, pJpeCfgIn);
        return JPE_IOC_RET_FAIL;
    }

    ret = _JpeCheckCfg(pJpeCfgIn);
    if(ret != JPE_IOC_RET_SUCCESS)
    {
        JPE_MSG(JPE_MSG_ERR, "jpeg encoder config fail\n");
        return ret;
    }


    /* HW-IP related regs setting */
    // set clk

    // 0: Enable, 1:Disable
    pJpeReg->regClk_jpe_clock_setting = 0;

    // The default clock is set to 288MHZ
    // clk-select = <0>; // 0: 288MHz  1: 216MHz  2: 54MHz  3: 27MHz
    pJpeReg->regClk_clock_source = pJpeOps->nClkSelect;

    // Jpeg configuration
    pJpeHalHandle->nInBufMode = (int)pJpeCfgIn->eInBufMode;
    pJpeHalHandle->nRawFormat = (int)pJpeCfgIn->eRawFormat;
    pJpeHalHandle->nCodecFormat = (int)pJpeCfgIn->eCodecFormat;

    pJpeHalHandle->nWidth = pJpeCfgIn->nWidth;
    pJpeHalHandle->nHeight = pJpeCfgIn->nHeight;

    pJpeHalHandle->nQScale = pJpeCfgIn->nQScale;

    /* Quantization table */
    ret = _JpeSetQTable(pJpeHalHandle, pJpeCfgIn->YQTable, pJpeCfgIn->CQTable, ARRAY_SIZE(pJpeCfgIn->YQTable), pJpeCfgIn->nQScale);
    if(ret != JPE_IOC_RET_SUCCESS)
    {
        JPE_MSG(JPE_MSG_ERR, "_JpeSetQTable() fail\n");
        return ret;
    }


    /* Set input buffer */
    if(pJpeCfgIn->InBuf[JPE_COLOR_PLAN_LUMA].nAddr != 0 && pJpeCfgIn->InBuf[JPE_COLOR_PLAN_LUMA].nSize != 0)
    {
        _JpeSetInBuf(pJpeHalHandle, pJpeCfgIn);
    }
    else
    {
        JPE_MSG(JPE_MSG_DEBUG,
                "Illegal input buffer addr 0x%08lX, size %ld\n", pJpeCfgIn->InBuf[JPE_COLOR_PLAN_LUMA].nAddr, pJpeCfgIn->InBuf[JPE_COLOR_PLAN_LUMA].nSize);
        return JPE_IOC_RET_BAD_INBUF;
    }

    /* Set output buffer */
    pJpeHalOutBufCfg->nOutBufAddr[0]   = pJpeCfgIn->OutBuf.nAddr;
    pJpeHalOutBufCfg->nOutBufSize[0]   = pJpeCfgIn->OutBuf.nSize;
    pJpeHalOutBufCfg->nOutBufSize[1]   = 0;
    pJpeHalOutBufCfg->nOutBufAddr[1]   = 0;

    return JPE_IOC_RET_SUCCESS;
}

static JPE_IOC_RET_STATUS_e _GetConf(JpeOpsCB_t* pJpeOpsCB, JpeCfg_t* pJpeCfgOut)
{
    JpeOps_t* pJpeOps = (JpeOps_t*)pJpeOpsCB;
    JpeHalHandle_t* pJpeHalHandle = &pJpeOps->tJpeHalHandle;

    pJpeCfgOut->eInBufMode = pJpeHalHandle->nInBufMode;
    pJpeCfgOut->eRawFormat = pJpeHalHandle->nRawFormat;
    pJpeCfgOut->eCodecFormat = pJpeHalHandle->nCodecFormat;

    pJpeCfgOut->nWidth = pJpeHalHandle->nWidth;
    pJpeCfgOut->nHeight = pJpeHalHandle->nHeight;

    memcpy(pJpeCfgOut->YQTable, pJpeHalHandle->YQTable, sizeof(u16) * 64);
    memcpy(pJpeCfgOut->CQTable, pJpeHalHandle->CQTable, sizeof(u16) * 64);
    pJpeCfgOut->nQScale = pJpeHalHandle->nQScale;

    pJpeCfgOut->InBuf[JPE_COLOR_PLAN_LUMA].nAddr = pJpeHalHandle->tJpeHalInBufCfg.nInBufYAddr[0];
    //pJpeCfgOut->InBuf[JPE_COLOR_PLAN_LUMA].nSize = pJpeHalHandle->tJpeHalInBufCfg.nInBufCAddr[0];
    pJpeCfgOut->InBuf[JPE_COLOR_PLAN_CHROMA].nAddr = pJpeHalHandle->tJpeHalInBufCfg.nInBufCAddr[0];
    //pJpeCfgOut->InBuf[JPE_COLOR_PLAN_CHROMA].nSize = pJpeHalHandle->tJpeHalInBufCfg.nInBufCAddr[1];

    pJpeCfgOut->OutBuf.nAddr = pJpeHalHandle->tJpeHalOutBufCfg.nOutBufAddr[0];
    pJpeCfgOut->OutBuf.nSize = pJpeHalHandle->tJpeHalOutBufCfg.nOutBufSize[0];
    pJpeCfgOut->nJpeOutBitOffset = pJpeHalHandle->tJpeHalOutBufCfg.nJpeOutBitOffset;

    return JPE_IOC_RET_SUCCESS;
}

/**
 * Description:
 *    Check the parameter sanity.
 *
 * Author:
 *    Albert.Liao
 *
 * Input:
 *     pCfg: pointer to JPEG configurations
 *
 * Output:
 *     Success: JPE_IOC_RET_SUCCESS
 *    Failure:   JPE_IOC_RET_BAD_INBUF, JPE_IOC_RET_FMT_NOT_SUPPORT
 */
JpeOpsCB_t* JpeOpsAcquire(int id)
{
    JpeOpsCB_t* pJpeOpsCB = NULL;
    JpeOps_t* pJpeOps = NULL;

    if(NULL != (pJpeOps = CamOsMemCalloc(1, sizeof(JpeOps_t))))
    {
        pJpeOpsCB = &pJpeOps->ops;
        pJpeOpsCB->init = _Init;
        pJpeOpsCB->release  = _OpsFree;
        pJpeOpsCB->setConf = _SetConf;
        pJpeOpsCB->getConf = _GetConf;
        pJpeOpsCB->getCaps = _GetCaps;
        pJpeOpsCB->jpeJob  = _JpeJob;
    }
    return (JpeOpsCB_t*)pJpeOps;
}
