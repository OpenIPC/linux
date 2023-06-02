#include <linux/mutex.h>
#include <linux/time.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include "hal_jpe.h"
#include "drv_jpe.h"
#include "mdrv_jpe.h"

/*******************************************************************************************************************
 * _DrvJpeCheckCfg
 *   Config check
 *
 * Parameters:
 *   pJpeCgfg:  JPEG configurations
 *
 * Return:
 *   JPE_IOC_RET_STATUS_e
 */
static JPE_IOC_RET_STATUS_e _DrvJpeCheckCfg(JpeCfg_t *pCfg)
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

    return JPE_IOC_RET_SUCCESS;
}

/*******************************************************************************************************************
 * _DrvJpeSetInBuf
 *   Assign an input buffer.
 *   This is a static wrapper function because upper layer use only one pointer as a input buffer point.
 *
 * Parameters:
 *   jpeHandle: device handle
 *   jpeBuf: input buffer
 *
 * Return:
 *   JPE_IOC_RET_STATUS_e
 */
//static JPE_IOC_RET_STATUS_e _DrvJpeSetInBuf(JpeHandle_t *jpeHandle, JpeBufInfo_t *jpeBuf)
static JPE_IOC_RET_STATUS_e _DrvJpeSetInBuf(JpeHandle_t *jpeHandle, JpeCfg_t *pJpeCfg)
{
    u32 size;
    JpeHalInBufCfg_t  inBufCfg;

    JPE_MSG(JPE_MSG_DEBUG, "set 0x%08lX, size %ld as input buffer\n", pJpeCfg->InBuf[JPE_COLOR_PLAN_LUMA].nAddr, pJpeCfg->InBuf[JPE_COLOR_PLAN_LUMA].nSize);

    // Set input buffer
    switch(jpeHandle->jpeCfg.eRawFormat)
    {
    case JPE_RAW_YUYV:
    case JPE_RAW_YVYU:
        if(jpeHandle->jpeCfg.eInBufMode == JPE_IBUF_FRAME_MODE)
        {
            inBufCfg.nInBufYAddr[0] = pJpeCfg->InBuf[JPE_COLOR_PLAN_LUMA].nAddr;
            inBufCfg.nInBufYAddr[1] = 0;
            inBufCfg.nInBufCAddr[0] = 0;
            inBufCfg.nInBufCAddr[1] = 0;
        }
        else if(jpeHandle->jpeCfg.eInBufMode == JPE_IBUF_ROW_MODE)
        {
            // Assume the user allocate big enough buffer and scaler also follow the same order
            // If not, we may change the IF to follow the behavior of scaler
            size = pJpeCfg->InBuf[JPE_COLOR_PLAN_LUMA].nSize / 2;
            inBufCfg.nInBufYAddr[0] = pJpeCfg->InBuf[JPE_COLOR_PLAN_LUMA].nAddr;
            inBufCfg.nInBufYAddr[1] = inBufCfg.nInBufYAddr[0] + size;
            inBufCfg.nInBufCAddr[0] = 0;
            inBufCfg.nInBufCAddr[1] = 0;
        }
        else
        {
            return JPE_IOC_RET_BAD_INBUF;
        }
        break;
    case JPE_RAW_NV12:
    case JPE_RAW_NV21:
        if(jpeHandle->jpeCfg.eInBufMode == JPE_IBUF_FRAME_MODE)
        {
//            size = ((jpeHandle->jpeCfg.nWidth+0x0f)&~0x0f) * ((jpeHandle->jpeCfg.nHeight+0x0f)&~0x0f);
            inBufCfg.nInBufYAddr[0] = pJpeCfg->InBuf[JPE_COLOR_PLAN_LUMA].nAddr;
            inBufCfg.nInBufYAddr[1] = 0;
            inBufCfg.nInBufCAddr[0] = pJpeCfg->InBuf[JPE_COLOR_PLAN_CHROMA].nAddr;
            inBufCfg.nInBufCAddr[1] = 0;
        }
        else if(jpeHandle->jpeCfg.eInBufMode == JPE_IBUF_ROW_MODE)
        {
            // Assume the user allocate big enough buffer and scaler also follow the same order
            // If not, we may change the IF to follow the behavior of scaler
            size =  pJpeCfg->InBuf[JPE_COLOR_PLAN_LUMA].nSize / 4;
            inBufCfg.nInBufYAddr[0] = pJpeCfg->InBuf[JPE_COLOR_PLAN_LUMA].nAddr;
            inBufCfg.nInBufYAddr[1] = inBufCfg.nInBufYAddr[0] + size;
            inBufCfg.nInBufCAddr[0] = pJpeCfg->InBuf[JPE_COLOR_PLAN_CHROMA].nAddr;
            inBufCfg.nInBufCAddr[1] = inBufCfg.nInBufCAddr[0] + size;
        }
        else
        {
            return JPE_IOC_RET_BAD_INBUF;
        }
        break;
    }
    HalJpeSetInBuf(&jpeHandle->jpeHalHandle, &inBufCfg);

    return JPE_IOC_RET_SUCCESS;
}

/*******************************************************************************************************************
 * JpeIsrHandler
 *   ISR handler
 *
 * Parameters:
 *   eIntNum: interrupt number
 *   jpeHandle: device handle
 *
 * Return:
 *   None
 */
void  JpeIsrHandler(int eIntNum, JpeHandle_t *jpeHandle)
{
    JPE_MSG(JPE_MSG_DEBUG, "Interrupt: %X\n", HalJpeGetIrq(&jpeHandle->jpeHalHandle));

    /* JPE_IRQ_BSPOBUF_FULL */
    if(HalJpeGetIrqCheck(&jpeHandle->jpeHalHandle,JPE_HAL_IRQ_BSPOBUF0_FULL))
    {
        /* Status update */
        jpeHandle->nEncodeSize = HalJpeGetOutputByteSize(&jpeHandle->jpeHalHandle,0);
        jpeHandle->ejpeDevStatus = JPE_DEV_OUTBUF_FULL;

        /* Clear IRQ */
        HalJpeClearIrq(&jpeHandle->jpeHalHandle,JPE_HAL_IRQ_BSPOBUF0_FULL);
    }

    /* JPE_IRQ_BSPOBUF1_FULL */
    if(HalJpeGetIrqCheck(&jpeHandle->jpeHalHandle,JPE_HAL_IRQ_BSPOBUF1_FULL))
    {
        /* Status update */
        jpeHandle->nEncodeSize = HalJpeGetOutputByteSize(&jpeHandle->jpeHalHandle,1);
        jpeHandle->ejpeDevStatus = JPE_DEV_OUTBUF_FULL;

        /* Clear IRQ */
        HalJpeClearIrq(&jpeHandle->jpeHalHandle,JPE_HAL_IRQ_BSPOBUF1_FULL);
    }

    /* JPE_IRQ_FRAME_DONE */
    if(HalJpeGetIrqCheck(&jpeHandle->jpeHalHandle,JPE_HAL_IRQ_FRAME_DONE))
    {
        /* Status update */
        jpeHandle->nEncodeSize = HalJpeGetOutputByteSize(&jpeHandle->jpeHalHandle,0);
        jpeHandle->ejpeDevStatus = JPE_DEV_ENC_DONE;

//      JPE_MSG(JPE_MSG_DEBUG, "size = %d\n", jpeHandle->nEncodeSize);

        /* Clear IRQ */
        HalJpeClearIrq(&jpeHandle->jpeHalHandle,JPE_HAL_IRQ_FRAME_DONE);
    }
}

/*******************************************************************************************************************
 * DrvJpeReset
 *   Reset low level driver & variables
 *
 * Parameters:
 *   jpeHandle: device handle
 *
 * Return:
 *   none
 */
void DrvJpeReset(JpeHandle_t *jpeHandle)
{
    /* Reset HAL JPE */
//    HalJpeSoftReset(&jpeHandle->jpeHalHandle);

    /* Reset variables */
    jpeHandle->ejpeDevStatus = JPE_DEV_INIT;
    jpeHandle->nEncodeSize = 0;
}

/*******************************************************************************************************************
 * DrvJpeInit
 *   Init JPEG settings
 *
 * Parameters:
 *   jpeHandle: device handle
 *   pJpeCgfg:  JPEG configurations
 *
 * Return:
 *   JPE_IOC_RET_STATUS_e
 */
JPE_IOC_RET_STATUS_e DrvJpeInit(JpeHandle_t *jpeHandle, pJpeCfg pJpeCfg)
{
    JPE_IOC_RET_STATUS_e ret = JPE_IOC_RET_SUCCESS;

    /* HW reset */
    DrvJpeReset(jpeHandle);

    ret = _DrvJpeCheckCfg(pJpeCfg);
    if(ret != JPE_IOC_RET_SUCCESS)
    {
        JPE_MSG(JPE_MSG_DEBUG, "some parameter settings are wrong...\n");
        return ret;
    }

    memcpy((void*)&jpeHandle->jpeCfg, pJpeCfg, sizeof(JpeCfg_t));

    /* Force output mode as single buffer mode because it is simpler
     * and has the same overhead as toggled output buffer
     */
//    HalJpeSetOutputMode(&jpeHandle->jpeHalHandle,JPE_HAL_OBUF_SINGLE_MODE);

    return JPE_IOC_RET_SUCCESS;
}

/*******************************************************************************************************************
 * DrvJpeSetOutBuf
 *   Assign a output buffer.
 *   Output buffer setting is separated from normal config because driver support virtual memory as output buffer.
 *   I.e. the upper layer can assign smaller buffer and assign again when previous buffer is full
 *
 *   This implementation for virtual memory does not introduce the toggle buffer because single buffer is simpler and
 *   the same overhead.
 *
 * Parameters:
 *   jpeHandle: device handle
 *   jpeBuf: output buffer
 *
 * Return:
 *   JPE_IOC_RET_STATUS_e
 */
JPE_IOC_RET_STATUS_e DrvJpeSetOutBuf(JpeHandle_t *jpeHandle, JpeBufInfo_t *jpeBuf)
{
    JpeHalOutBufCfg_t jpeHalBuf;

    jpeHalBuf.nOutBufAddr[0]   = jpeBuf->nAddr;
    jpeHalBuf.nOutBufSize[0]   = jpeBuf->nSize;
    jpeHalBuf.nOutBufSize[1]   = 0;
    jpeHalBuf.nOutBufAddr[1]   = 0;

    jpeHalBuf.nJpeOutBitOffset = jpeHandle->jpeCfg.nJpeOutBitOffset;

    HalJpeSetOutBuf(&jpeHandle->jpeHalHandle,&jpeHalBuf);

    return JPE_IOC_RET_SUCCESS;
}

/*******************************************************************************************************************
 * DrvJpeSetQTable
 *   Calculate & assign quantization tables
 *
 * Parameters:
 *   jpeHandle: device handle
 *   q_table_y: Q table for Y
 *   q_table_c: Q table for C
 *   size:      size of Q table
 *   q_scale:   Quantization scale, a value which is 100-based value, default is 50
 *
 * Return:
 *   JPE_IOC_RET_STATUS_e
 */
static JPE_IOC_RET_STATUS_e DrvJpeSetQTable(JpeHandle_t *jpeHandle, const u16 *q_table_y, const u16 *q_table_c, u32 size, const u16 q_scale)
{
    u16 *y_table, *c_table;
    JPE_IOC_RET_STATUS_e ret = JPE_IOC_RET_SUCCESS;
    int i;

    y_table = kcalloc(size, sizeof(u16), GFP_KERNEL);
    c_table = kcalloc(size, sizeof(u16), GFP_KERNEL);

    for(i=0; i<size; i++)
    {
        y_table[i] = (u16)(((u32)q_table_y[i] * (u32)q_scale + 50) / 100);
        y_table[i] = y_table[i] ? y_table[i] : 1;
        c_table[i] = (u16)(((u32)q_table_c[i] * (u32)q_scale + 50) / 100);
        c_table[i] = c_table[i] ? c_table[i] : 1;
    }

    if(HalJpeSetQTable(&jpeHandle->jpeHalHandle,y_table, c_table, size) != 0)
    {
        ret = JPE_IOC_RET_BAD_QTABLE;
    }

    kfree(y_table);
    kfree(c_table);

    return JPE_IOC_RET_SUCCESS;
}

/*******************************************************************************************************************
 * DrvJpeEncodeOneFrame
 *   Start JPE to encode one frame into a JPEG
 *
 * Parameters:
 *   jpeHandle: device handle
 *
 * Return:
 *   JPE_IOC_RET_STATUS_e
 */
JPE_IOC_RET_STATUS_e DrvJpeEncodeOneFrame(JpeHandle_t *jpeHandle, JpeHalOutBufCfg_t* jpeHalBuf)
{
    JPE_IOC_RET_STATUS_e ret;

    /* Can't trigger HW if it is triggered */
    if(jpeHandle->ejpeDevStatus != JPE_DEV_INIT && jpeHandle->ejpeDevStatus != JPE_DEV_ENC_DONE)
    {
        return JPE_IOC_RET_HW_IS_RUNNING;
    }

    /* HW reset */
//  HalJpeSoftReset();

    HalJpeSoftReset(&jpeHandle->jpeHalHandle);

    HalJpeSetOutputMode(&jpeHandle->jpeHalHandle, JPE_HAL_OBUF_SINGLE_MODE);

    HalJpeSetOutBuf(&jpeHandle->jpeHalHandle, jpeHalBuf);
    /* Clear IRQ */
    HalJpeClearIrq(&jpeHandle->jpeHalHandle, JPE_HAL_IRQ_ALL);

    /* Set image dimension */
    HalJpeSetPicDim(&jpeHandle->jpeHalHandle, jpeHandle->jpeCfg.nWidth, jpeHandle->jpeCfg.nHeight);

    /* Set codec mode */
    HalJpeSetCodecFormat(&jpeHandle->jpeHalHandle, jpeHandle->jpeCfg.eCodecFormat);

    /* Set YUV format */
    HalJpeSetRawFormat(&jpeHandle->jpeHalHandle, jpeHandle->jpeCfg.eRawFormat);

    /* Set input mode */
    HalJpeSetInputMode(&jpeHandle->jpeHalHandle, jpeHandle->jpeCfg.eInBufMode);

    /* Quantization table */
    ret = DrvJpeSetQTable(jpeHandle, jpeHandle->jpeCfg.YQTable, jpeHandle->jpeCfg.CQTable, ARRAY_SIZE(jpeHandle->jpeCfg.YQTable), jpeHandle->jpeCfg.nQScale);
    if(ret != JPE_IOC_RET_SUCCESS)
    {
        return ret;
    }

    /* Set input buffer */
    if(jpeHandle->jpeCfg.InBuf[JPE_COLOR_PLAN_LUMA].nAddr != 0 && jpeHandle->jpeCfg.InBuf[JPE_COLOR_PLAN_LUMA].nSize != 0)
    {
        _DrvJpeSetInBuf(jpeHandle, &jpeHandle->jpeCfg);
    }
    else
    {
        /* unlike the output buffer, this is a only place to set input buffer
         * So we return error if buffer is not assigned
         */
        JPE_MSG(JPE_MSG_DEBUG,                "Illegal input buffer addr 0x%08lX, size %ld\n", jpeHandle->jpeCfg.InBuf[JPE_COLOR_PLAN_LUMA].nAddr, jpeHandle->jpeCfg.InBuf[JPE_COLOR_PLAN_LUMA].nSize);
        return JPE_IOC_RET_BAD_INBUF;
    }

    /* Remvoe IRQ mask, and only 3 bits are meaningful */
    HalJpeSetIrqMask(&jpeHandle->jpeHalHandle,0);

    /* Enable HW */
    HalJpeSetEnable(&jpeHandle->jpeHalHandle);
    jpeHandle->ejpeDevStatus = JPE_DEV_BUSY;

    return JPE_IOC_RET_SUCCESS;

}

/*******************************************************************************************************************
 * DrvJpeGetCaps
 *   Start JPE to encode one frame into a JPEG
 *
 * Parameters:
 *   jpeHandle: device handle
 *   pCaps: JPE capabilities
 *
 * Return:
 *   JPE_IOC_RET_STATUS_e
 */
JPE_IOC_RET_STATUS_e DrvJpeGetCaps(JpeHandle_t *jpeHandle, JpeCaps_t *pCaps)
{

    if(pCaps == 0) return JPE_IOC_RET_BAD_NULLPTR;

    pCaps->nBufferModeSupport = HalJpeGetBufferModeSupportInfo();
    pCaps->nCodecSupport = HalJpeGetCodecSupportInfo();
    pCaps->nOutBufSLogAddrAlign = HalJpeGetOutBufSLogAddrAlignInfo();
    pCaps->nRefCLogAddrAlign[0] = HalJpeGetRefCLogAddrAlignInfo(0);
    pCaps->nRefCLogAddrAlign[1] = HalJpeGetRefCLogAddrAlignInfo(1);
    pCaps->nRefYLogAddrAlign[0] = HalJpeGetRefYLogAddrAlignInfo(0);
    pCaps->nRefYLogAddrAlign[1] = HalJpeGetRefYLogAddrAlignInfo(1);
    pCaps->nSclHandShakeSupport = HalJpeGetSclHandShakeSupportInfo();

    return JPE_IOC_RET_SUCCESS;
}

