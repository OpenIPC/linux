//#include <string.h>
#include <linux/interrupt.h>
//#include <asm/delay.h>
#include "ms_platform.h"
#include "hal_jpe.h"
#include "_hal_jpe_reg.h"
#include "_hal_jpe_platform.h"
//#include "hal_drv_util.h"
//#include "hal_miu_arb.h"
//#include "sys_sys_arm.h"
//#include "hal_int_ctrl.h"
//#include "sys_MsWrapper_cus_os_int_ctrl.h"
#define IMG_CAP_DBG_LEVEL                 (_CUS2 | LEVEL_5)/* Trace Filter of image capture  */

#define TRACE
#define DEBUG_LEVEL 0
#include "hal_jpe_ms_dprintf.h"

//volatile JpeReg_t jpe_reg;
#define TIME_OUT_CNT 10

//#define IOBASE      0xFD000000
//#define JPEBASE     0x132000
//#define LPLLBASE    0x103300

//static u16
//reg_ld(u32 addr) {
//    return *((volatile u16*)addr);
//}

//static void
//reg_st(u32 addr, u16 val) {
//    *((volatile u16*)addr) = val;
//}

#define REGW(base,idx,val,cmt)  ms_writew(val,((uint)base+(idx)*4))
#define REGR(base,idx,cmt)      ms_readw(((uint)base+(idx)*4))

//#if 0
//#define REGR(idx,cmt)           reg_ld(IOBASE+((JPEBASE)*2+(idx)*4))
//#define REGW(idx,val,cmt)       reg_st(IOBASE+((JPEBASE)*2+(idx)*4),val)
//#define REGR_LPLL(idx,cmt)      reg_ld(IOBASE+((LPLLBASE)*2+(idx)*4))
//#define REGW_LPLL(idx,val,cmt)  reg_st(IOBASE+((LPLLBASE)*2+(idx)*4),val)
//#else
//#define REGR(idx,cmt)           reg_ld(IOBASE+((JPEBASE)*2+(idx)*4))
//#define REGW(idx,val,cmt)       do {reg_st(IOBASE+((JPEBASE)*2+(idx)*4),val); JPE_MSG(1, "Write %04X to reg %02X (%0X), %s", val, idx, idx*2, cmt);} while(0)
//#define REGR_LPLL(idx,cmt)      reg_ld(IOBASE+((LPLLBASE)*2+(idx)*4))
//#define REGW_LPLL(idx,val,cmt)  do {reg_st(IOBASE+((LPLLBASE)*2+(idx)*4),val); JPE_MSG(1, "Write "cmt, val, idx);} while(0)
//#endif

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                                                                                                                                  //
//                                                                 IRQ API                                                                          //
//                                                                                                                                                  //
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/*******************************************************************************************************************
 * _HalJpeIrqMaskConvertEnumerate2Real
 *   Convert enumerated mask to real mask.
 *   The converter simplifies the IF of IRQ and can handle the mux of bits in the IRQ mask
 *
 * Parameters:
 *   mask: enumerated mask
 *
 * Return:
 *   Real bitwise mask
 */
static u16 _HalJpeIrqMaskConvertEnumerate2Real(JpeReg_t* pJpeReg,JpeHalIrqEvent_e mask)
{
    u16 real_mask = (u16)(mask & 0xFFFF);

    // Bit 4 when reg_jpe_g_vs_irq_en = 0
    if(mask & JPE_HAL_IRQ_TXIP_TIMEOUT && !pJpeReg->uReg73.tReg73.nRegJpeGVsIrqEn)
    {
        real_mask |= 0x0010;
    }

    // Bit 4 when reg_jpe_g_vs_irq_en = 1
    if(mask & JPE_HAL_IRQ_VS && pJpeReg->uReg73.tReg73.nRegJpeGVsIrqEn)
    {
        real_mask |= 0x0010;
    }

    // Bit 6 when reg_jpe_g_row_down_irq_en = 0
    if(mask & JPE_HAL_IRQ_NET_TRIGGER && !pJpeReg->uReg73.tReg73.nRegJpeGRowDownIrqEn)
    {
        real_mask |= 0x0040;
    }

    // Bit 6 when reg_jpe_g_row_down_irq_en = 1
    if(mask & JPE_HAL_IRQ_VIU_ROW_DONE && pJpeReg->uReg73.tReg73.nRegJpeGRowDownIrqEn)
    {
        real_mask |= 0x0040;
    }

    return real_mask;
}

/*******************************************************************************************************************
 * _HalJpeIrqMaskConvertReal2Enumerate
 *   Convert real mask to enumated mask.
 *   The converter simplifies the IF of IRQ and can handle the mux of bits in the IRQ mask
 *
 * Parameters:
 *   mask: real bitwise mask
 *
 * Return:
 *   Enumerated IRQ
 */
static JpeHalIrqEvent_e _HalJpeIrqMaskConvertReal2Enumerate( JpeReg_t* pJpeReg,u16 mask)
{
    JpeHalIrqEvent_e enumerate = (u16)(mask & 0xFFAF);

    // Bit 4 when reg_jpe_g_vs_irq_en = 0
    if(mask & 0x0010 && !pJpeReg->uReg73.tReg73.nRegJpeGVsIrqEn)
    {
        enumerate |= JPE_HAL_IRQ_TXIP_TIMEOUT;
    }

    // Bit 4 when reg_jpe_g_vs_irq_en = 1
    if(mask & 0x0010 && pJpeReg->uReg73.tReg73.nRegJpeGVsIrqEn)
    {
        enumerate |= JPE_HAL_IRQ_VS;
    }

    // Bit 6 when reg_jpe_g_row_down_irq_en = 0
    if(mask & 0x0040 && !pJpeReg->uReg73.tReg73.nRegJpeGRowDownIrqEn)
    {
        enumerate |= JPE_HAL_IRQ_NET_TRIGGER;
    }

    // Bit 6 when reg_jpe_g_row_down_irq_en = 1
    if(mask & 0x0040 && pJpeReg->uReg73.tReg73.nRegJpeGRowDownIrqEn)
    {
        enumerate |= JPE_HAL_IRQ_VIU_ROW_DONE;
    }

    return enumerate;
}


/*******************************************************************************************************************
 * HalJpeSetIrqMask
 *   Set interrupt trigger mask
 *
 * Parameters:
 *   mask: Bitwise mask, defined in JpeHalIrqEvent_e
 *
 * Return:
 *   none
 */
void HalJpeSetIrqMask(JpeHalHandle_t* handle, JpeHalIrqEvent_e mask)
{
    u32 nBaseAddr = handle->nBaseAddr;
    JpeReg_t* pJpeReg = handle->pJpeReg;
    pJpeReg->uReg1c.tReg1c.nRegJpeGIrqMask = _HalJpeIrqMaskConvertEnumerate2Real(pJpeReg,mask);
    REGW(nBaseAddr,0x1c, pJpeReg->uReg1c.nReg1c, "[%x] reg[%x] : irq mask\n");
}

/*******************************************************************************************************************
 * HalJpeClearIrq
 *   Clear triggered interrupt
 *
 * Parameters:
 *   mask: Bitwise mask, defined in JpeHalIrqEvent_e
 *
 * Return:
 *   none
 */
void HalJpeClearIrq(JpeHalHandle_t* handle,JpeHalIrqEvent_e mask)
{
    u32 nBaseAddr = handle->nBaseAddr;
    JpeReg_t* pJpeReg = handle->pJpeReg;

    pJpeReg->uReg1d.nReg1d = REGR(nBaseAddr,0x1d, "[%x] reg[%x] :IRQ register\n");

    pJpeReg->uReg1d.tReg1d.nRegJpeGIrqClr = _HalJpeIrqMaskConvertEnumerate2Real(pJpeReg,mask);
    REGW(nBaseAddr,0x1d, pJpeReg->uReg1d.nReg1d, "[%x] reg[%x] : Clear IRQ\n");
    pJpeReg->uReg1d.nReg1d = REGR(nBaseAddr,0x1d, "[%x] reg[%x] :IRQ register\n"); // HW is write-one-clear, so update again
}

/*******************************************************************************************************************
 * HalJpeGetIrqCheck
 *   Check current IRQ status
 *
 * Parameters:
 *   mask: Bitwise mask to be checked
 *
 * Return:
 *   Checked result, bitwise
 */
JpeHalIrqEvent_e HalJpeGetIrqCheck(JpeHalHandle_t* handle,JpeHalIrqEvent_e mask)
{
    u32 nBaseAddr = handle->nBaseAddr;
    JpeReg_t* pJpeReg = handle->pJpeReg;

    u16 reg_value;
    reg_value = REGR(nBaseAddr,0x1e, "[%x] reg[%x] :IRQ cpu and ip status\n");

    return _HalJpeIrqMaskConvertReal2Enumerate(pJpeReg,reg_value) & mask;
}

/*******************************************************************************************************************
 * HalJpeGetIrq
 *   Get current interrupt trigger status
 *
 * Return:
 *   Bitwise status
 * Parameters:
 *   mask: Bitwise mask, defined in JpeHalIrqEvent_e
 *
 * Return:
 *   none
 */
u16 HalJpeGetIrq(JpeHalHandle_t* handle)
{
    u32 nBaseAddr = handle->nBaseAddr;
    u16 u16Reg;
    u16Reg = REGR(nBaseAddr, 0x1e, "[%x] reg[%x] :IRQ cpu and ip status\n");

    return u16Reg;
}

/*******************************************************************************************************************
 * HalJpeGetIrqEvent
 *   IRQ bit mask check
 *
 * Parameters:
 *   u16IrqReg: Bitwise IRQ status
 *   IrqEventMask: Bitwise mask
 *
 * Return:
 *   none
 */
//JpeHalIrqEvent_e HalJpeGetIrqEvent(u16 u16IrqReg, JpeHalIrqEvent_e IrqEventMask)
//{
//    return _HalJpeIrqMaskConvertReal2Enumerate(u16IrqReg) & IrqEventMask;
//}



//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                                                                                                                                  //
//                                                            Quntization Tables Setting                                                            //
//                                                                                                                                                  //
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/*******************************************************************************************************************
 * _HalJpeQTableEntryRead
 *   Read a entry of Q table
 *
 * Parameters:
 *   index: Q table index
 *   value: the pointer of value address
 *
 * Return:
 *   >=0: success
 =   <0: failed
 */
static int _HalJpeQTableEntryRead(JpeReg_t* pJpeReg,u32 nBaseAddr,u16 index, u16 *value)
{
    int retry = TIME_OUT_CNT;


    //read data
    pJpeReg->uReg49.tReg49.nRegJpeSTbcRw = 0;
    pJpeReg->uReg48.tReg48.nRegJpeSTbcEn = 1;
    pJpeReg->uReg49.tReg49.nRegJpeSTbcAdr = index;
    REGW(nBaseAddr,0x49, pJpeReg->uReg49.nReg49, "[%x] reg[%x] : table address\n");
    REGW(nBaseAddr,0x48, pJpeReg->uReg48.nReg48, "[%x] reg[%x] : table read enable\n");
    pJpeReg->uReg48.tReg48.nRegJpeSTbcEn = 0;    // write one clear

    while(retry-- > 0) {
        pJpeReg->uReg49.nReg49 = REGR(nBaseAddr,0x49, "[%x] reg[%x] : table write enable\n");
        if(pJpeReg->uReg49.tReg49.nRegJpeSTbcDone) {
            // Clear done
            pJpeReg->uReg49.tReg49.nRegJpeSTbcDoneClr = 1;
            REGW(nBaseAddr,0x49, pJpeReg->uReg49.nReg49, "[%x] reg[%x] : table write enable\n");
            pJpeReg->uReg49.tReg49.nRegJpeSTbcDoneClr = 0;

            // Get value
            *value = REGR(nBaseAddr,0x4b, "[%x] reg[%x] : table read enable\n");
            break;
        }
    }

    return retry;
}

/*******************************************************************************************************************
 * _HalJpeQTableEntryWrite
 *   Write a entry of Q table
 *
 * Parameters:
 *   index: Q table index
 *   value: Quantization value
 *
 * Return:
 *   >=0: success
 *   <=0: failed
 */
static int _HalJpeQTableEntryWrite(JpeReg_t* pJpeReg,u32 nBaseAddr,u16 index, u16 value)
{
    int retry = TIME_OUT_CNT;

    // Set value
    pJpeReg->uReg4a.tReg4a.nRegJpeSTbcWdata = value;
    pJpeReg->uReg49.tReg49.nRegJpeSTbcRw = 1;
    pJpeReg->uReg48.tReg48.nRegJpeSTbcEn = 1;
    pJpeReg->uReg49.tReg49.nRegJpeSTbcAdr = index;
    REGW(nBaseAddr,0x4a, pJpeReg->uReg4a.nReg4a, "[%x] reg[%x] : table write data\n");
    REGW(nBaseAddr,0x49, pJpeReg->uReg49.nReg49, "[%x] reg[%x] : table address\n");
    REGW(nBaseAddr,0x48, pJpeReg->uReg48.nReg48, "[%x] reg[%x] : table write enable\n");
    pJpeReg->uReg48.tReg48.nRegJpeSTbcEn = 0;    // write one clear

    // Wait for set done
    while(retry-- > 0) {
        pJpeReg->uReg49.nReg49 = REGR(nBaseAddr,0x49, "[%x] reg[%x] : table write enable\n");
        if(pJpeReg->uReg49.tReg49.nRegJpeSTbcDone) {
            // Clear done
            pJpeReg->uReg49.tReg49.nRegJpeSTbcDoneClr = 1;
            REGW(nBaseAddr,0x49, pJpeReg->uReg49.nReg49, "[%x] reg[%x] : table write enable\n");
            pJpeReg->uReg49.tReg49.nRegJpeSTbcDoneClr = 0;
            break;
        }
    }

    return retry;
}

/*******************************************************************************************************************
 * HalJpeSetQTable
 *   Update Q table
 *
 * Parameters:
 *   q_table_y: Q table for Y
 *   q_table_c: Q table for CbCr
 *   size: table size
 *
 * Return:
 *   >=0: success
 =   <0: failed
 */
int HalJpeSetQTable(JpeHalHandle_t* handle,const u16 *q_table_y, const u16 *q_table_c, u32 size)
{
    u32 nBaseAddr = handle->nBaseAddr;
    JpeReg_t* pJpeReg = handle->pJpeReg;
    u16 value_w, value_r;
    int i, retry, ret = 0;

    // Switch SW mode to update Q table
    pJpeReg->uReg03.tReg03.nRegJpeGTbcMode = 0;
    REGW(nBaseAddr,0x3, pJpeReg->uReg03.nReg03, "[%x] reg[%x] : tbc_mode=0\n");

    // ?
    // jpe_reg.uReg48.nReg48 = 0;

    // Udate Y table
    for(i=0; i<size; i++) {
        retry = TIME_OUT_CNT;
        while(retry-- > 0) {
            value_w = q_table_y[((i&0x7)<<3)|(i>>3)];

            // Write
            if(_HalJpeQTableEntryWrite(pJpeReg,nBaseAddr,i, value_w) <= 0) {
                ret = -1;
                goto RETURN;
            }

            // Read
            if(_HalJpeQTableEntryRead(pJpeReg,nBaseAddr,i, &value_r) <= 0) {
                ret = -1;
                goto RETURN;
            }

            // Compare
            if(value_r == value_w)
                break;
        }
    }

    // Udate C table
    for(i=0; i<size; i++) {
        retry = TIME_OUT_CNT;
        while(retry-- > 0) {
            value_w = q_table_c[((i&0x7)<<3)|(i>>3)];

            // Write
            if(_HalJpeQTableEntryWrite(pJpeReg,nBaseAddr,i+64, value_w) <= 0) {
                ret = -1;
                goto RETURN;
            }

            // Read
            if(_HalJpeQTableEntryRead(pJpeReg,nBaseAddr,i+64, &value_r) <= 0) {
                ret = -1;
                goto RETURN;
            }

            // Compare
            if(value_r == value_w)
                break;
        }
    }

    // update Index last identyfier
    // The default size of Quantization table is 64, but JPE can accept that size smaller than 64 and
    // treat others as 0 ( I guess...)
    pJpeReg->uReg2c.tReg2c.nRegJpeSQuanIdxLast = size;
    pJpeReg->uReg2c.tReg2c.nRegJpeSQuanIdxSwlast = (size < 63) ? 1 : 0;
    REGW(nBaseAddr,0x2c, pJpeReg->uReg2c.nReg2c, "[%x] reg[%x] : Last zigzag\n");

RETURN:
    // Switch to hw mode
    pJpeReg->uReg03.tReg03.nRegJpeGTbcMode = 1;
    REGW(nBaseAddr,0x3, pJpeReg->uReg03.nReg03, "[%x] reg[%x] : tbc_mode=1\n");

    return ret;
}

/*******************************************************************************************************************
 * HalJpeReadQTable
 *   Read Q table set last time
 *
 * Parameters:
 *   q_table_y: Q table for Y
 *   q_table_c: Q table for CbCr
 *   size: table size
 *
 * Return:
 *   >=0: success
 =   <0: failed
 */
int HalJpeReadQTable(JpeHalHandle_t* handle,u16 *q_table_y, u16 *q_table_c, u32 *size)
{
    int i, ret = 0;
    u32 nBaseAddr = handle->nBaseAddr;
    JpeReg_t* pJpeReg = handle->pJpeReg;
    // Switch SW mode to read Q table
    pJpeReg->uReg03.tReg03.nRegJpeGTbcMode = 0;
    REGW(nBaseAddr,0x3, pJpeReg->uReg03.nReg03, "[%x] reg[%x] : tbc_mode=0\n");

    // Is the size of table smaller than 64?
    if(pJpeReg->uReg2c.tReg2c.nRegJpeSQuanIdxSwlast) {
        *size = pJpeReg->uReg2c.tReg2c.nRegJpeSQuanIdxLast;
    } else {
        *size = 64;
    }

    // ?
    // jpe_reg.uReg48.nReg48 = 0;

    // Udate Y table
    for(i=0; i<*size; i++) {
        // Read
        if(_HalJpeQTableEntryRead(pJpeReg,nBaseAddr,i, q_table_y+i) <= 0) {
            ret = -1;
            goto RETURN;
        }
    }

    // Udate C table
    for(i=0; i<*size; i++) {
        // Read
        if(_HalJpeQTableEntryRead(pJpeReg,nBaseAddr,i+64, q_table_c+i) <= 0) {
            ret = -1;
            goto RETURN;
        }
    }

RETURN:
    // Switch to hw mode
    pJpeReg->uReg03.tReg03.nRegJpeGTbcMode = 1;
    REGW(nBaseAddr,0x3, pJpeReg->uReg03.nReg03, "[%x] reg[%x] : tbc_mode=1\n");

    return ret;
}


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                                                                                                                                  //
//                                                                 Configurations                                                                   //
//                                                                                                                                                  //
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/*******************************************************************************************************************
 * HalJpeSetJpeMode
 *   Config JPEG Mode
 *
 * Parameters:
 *   bUseJPE: 1 is JPEG encoder, 0 is JPEG decoder
 *
 * Return:
 *   none
 */
void HalJpeSetJpeMode(JpeHalHandle_t* handle,u32 bUseJPE)
{
    u32 nBaseAddr = handle->nBaseAddr;
    JpeReg_t* pJpeReg = handle->pJpeReg;
    if (bUseJPE)
        pJpeReg->uReg00.tReg00.nRegJpeGMode = 0;
    else
        pJpeReg->uReg00.tReg00.nRegJpeGMode = 1;

    REGW(nBaseAddr,0x0, pJpeReg->uReg00.nReg00, "[%x] reg[%x] : Set JPE mode\n");
}

/*******************************************************************************************************************
 * HalJpeSetCodecFormat
 *   Config the encode froamt of HW codec
 *
 * Parameters:
 *   format: JPE_HAL_CODEC_JPEG:       JPEG Mode
 *           JPE_HAL_CODEC_H263I:      HW not support
 *           JPE_HAL_CODEC_ENCODE_DCT: SW not implemented yet
 *
 * Return:
 *   none
 */
void HalJpeSetCodecFormat(JpeHalHandle_t* handle,JpeHalCodecFormat_e format)
{
    u32 nBaseAddr = handle->nBaseAddr;
    JpeReg_t* pJpeReg = handle->pJpeReg;
    switch(format) {
        case JPE_HAL_CODEC_JPEG:
            pJpeReg->uReg00.tReg00.nRegJpeGEncMode = 0;
            pJpeReg->uReg00.tReg00.nRegJpeGDctOnlyEn = 0;
            break;

        case JPE_HAL_CODEC_H263I:
            pJpeReg->uReg00.tReg00.nRegJpeGEncMode = 1;
            pJpeReg->uReg00.tReg00.nRegJpeGDctOnlyEn = 0;
            break;

        case JPE_HAL_CODEC_ENCODE_DCT:
            pJpeReg->uReg00.tReg00.nRegJpeGEncMode = 0;
            pJpeReg->uReg00.tReg00.nRegJpeGDctOnlyEn = 1;
            break;
    }

    REGW(nBaseAddr,0x00, pJpeReg->uReg00.nReg00, "[%x] reg[%x] : Codec mode\n");
}

/*******************************************************************************************************************
 * HalJpeSetEncFormat
 *   Config the RAW format for HW codec
 *
 * Parameters:
 *   format: JPE_HAL_RAW_YUYV: 422 YUYV
 *           JPE_HAL_RAW_YVYU: 422 YVYU
 *           JPE_HAL_RAW_NV12: 420 NV12
 *           JPE_HAL_RAW_NV21: 420 NV21
 *
 * Return:
 *   none
 */
void HalJpeSetRawFormat(JpeHalHandle_t* handle,JpeHalRawFormat_e format)
{
    u32 nBaseAddr = handle->nBaseAddr;
    JpeReg_t* pJpeReg = handle->pJpeReg;
    switch(format) {
        case JPE_HAL_RAW_YUYV:
            pJpeReg->uReg00.tReg00.nRegJpeGEncFormat = 1;
            pJpeReg->uReg5f.tReg5f.nRegSJpeYuvldrCWwap = 0;
            break;

        case JPE_HAL_RAW_YVYU:
            pJpeReg->uReg00.tReg00.nRegJpeGEncFormat = 1;
            pJpeReg->uReg5f.tReg5f.nRegSJpeYuvldrCWwap = 1;
            break;

        case JPE_HAL_RAW_NV12:
            pJpeReg->uReg00.tReg00.nRegJpeGEncFormat = 0;
            pJpeReg->uReg5f.tReg5f.nRegSJpeYuvldrCWwap = 0;
            break;

        case JPE_HAL_RAW_NV21:
            pJpeReg->uReg00.tReg00.nRegJpeGEncFormat = 0;
            pJpeReg->uReg5f.tReg5f.nRegSJpeYuvldrCWwap = 1;
            break;
    }

    REGW(nBaseAddr,0x00, pJpeReg->uReg00.nReg00, "[%x] reg[%x] : YUV format\n");
    REGW(nBaseAddr,0x5F, pJpeReg->uReg5f.nReg5f, "[%x] reg[%x] : YUV format\n");
}



/*******************************************************************************************************************
 * HalJpeSetPicDim
 *   Config the input dimension (for encoder only?)
 *
 * Parameters:
 *   width:  Image width in pixel
 *   height: Image height in pixel
 *
 * Return:
 *   none
 */
void HalJpeSetPicDim(JpeHalHandle_t* handle,u32 width, u32 height)
{
    u32 nBaseAddr = handle->nBaseAddr;
    JpeReg_t* pJpeReg = handle->pJpeReg;
    pJpeReg->uReg01.tReg01.nRegJpeGPicWidth = width; // (width+15)&~15;
    pJpeReg->uReg02.tReg02.nRegJpeGPicHeight = (height+7)&~7;
    pJpeReg->uReg5e.tReg5e.nRegGJpeYPitch = (width+15) / 16;
    pJpeReg->uReg5e.tReg5e.nRegGJpeCPitch = (width+15) / 16;
    REGW(nBaseAddr,0x01, pJpeReg->uReg01.nReg01, "[%x] reg[%x] : picture width\n");
    REGW(nBaseAddr,0x02, pJpeReg->uReg02.nReg02, "[%x] reg[%x] : picture height\n");
    REGW(nBaseAddr,0x5E, pJpeReg->uReg5e.nReg5e, "[%x] reg[%x] : picture pitch\n");
}

/*******************************************************************************************************************
 * HalJpeToggleOutBuf
 *   Toggle output buffer
 *
 * Parameters:
 *   nObufIdx: new output buffer index
 *
 * Return:
 *   none
 */
void HalJpeToggleOutBuf(JpeHalHandle_t* handle,u32 nObufIdx)
{
    u32 nBaseAddr = handle->nBaseAddr;
    JpeReg_t* pJpeReg = handle->pJpeReg;
    if (nObufIdx==0)
    {
        ms_dprintf(0,"nRegJpeObufToggleObuf_0\n");
        pJpeReg->uReg3b.nReg3b = REGR(nBaseAddr,0x3b, "[%x] reg[%x] : SW read buffer 0\n");
        pJpeReg->uReg3b.tReg3b.nRegJpeObufToggleObuf0Status = 1;
        REGW(nBaseAddr,0x3b, pJpeReg->uReg3b.nReg3b, "[%x] reg[%x] : SW read buffer 0\n");
        pJpeReg->uReg3b.tReg3b.nRegJpeObufToggleObuf0Status = 0;
    }
    else
    {
        ms_dprintf(0,"nRegJpeObufToggleObuf_1\n");
        pJpeReg->uReg3b.nReg3b = REGR(nBaseAddr,0x3b, "[%x] reg[%x] : SW read buffer 1\n");
        pJpeReg->uReg3b.tReg3b.nRegJpeObufToggleObuf1Status = 1;
        REGW(nBaseAddr,0x3b, pJpeReg->uReg3b.nReg3b, "[%x] reg[%x] : SW read buffer 1\n");
        pJpeReg->uReg3b.tReg3b.nRegJpeObufToggleObuf1Status = 0;
    }
}

/*******************************************************************************************************************
 * HalJpeSetInputMode
 *   Config the operation mode of input buffer
 *
 * Parameters:
 *   mode: JPE_HAL_BUFFER_ROW_MODE:   Double buffer mode (for ROW of IMI?)
 *         Others:                    Normal (single) frame mode (for MIU?)
 *
 * Return:
 *   none
 */
void HalJpeSetInputMode(JpeHalHandle_t* handle,JpeHalInBufMode_e mode)
{
    u32 nBaseAddr = handle->nBaseAddr;
    JpeReg_t* pJpeReg = handle->pJpeReg;
    if(mode == JPE_HAL_BUFFER_ROW_MODE)
        pJpeReg->uReg18.tReg18.nRegJpeGJpeBuffermode = 0;
    else
        pJpeReg->uReg18.tReg18.nRegJpeGJpeBuffermode = 1;
    REGW(nBaseAddr,0x18, pJpeReg->uReg18.nReg18, "[%x] reg[%x] : input buffer mode\n");
}

/*******************************************************************************************************************
 * HalJpeSetOutputMode
 *   Config the operation mode of output buffer
 *
 * Parameters:
 *   mode: JPE_HAL_OBUF_SINGLE_MODE: All outputs are written to only one buffer
 *         JPE_HAL_OBUF_MULTI_MODE:  Outputs are written to may buffers
 *
 * Return:
 *   none
 */
void HalJpeSetOutputMode(JpeHalHandle_t *handle,JpeHalOutBufMode_e mode)
{
    // End of config??
    uint nBaseAddr = handle->nBaseAddr;
    JpeReg_t* pJpeReg = handle->pJpeReg;
    pJpeReg->uReg3b.tReg3b.nRegJpeObufHwEn = mode;
    REGW(nBaseAddr,0x3b, pJpeReg->uReg3b.nReg3b, "[%x] reg[%x] : output buffer mode\n");
}

/*******************************************************************************************************************
 * HalJpeSetInBuf
 *   Config input & output address
 *
 * Parameters:
 *   ptInBufCfg: Input buffer configuration
 *
 * Return:
 *   none
 */
void HalJpeSetInBuf(JpeHalHandle_t* handle,pJpeHalInBufCfg ptInBufCfg)
{
    u32 addr;
    uint nBaseAddr = handle->nBaseAddr;
    JpeReg_t* pJpeReg = handle->pJpeReg;
    //assert((ptBufInfo->nInBufYAddr[0]&0xFF)==0);
    //assert((ptBufInfo->nInBufYAddr[1]&0xFF)==0);
    //assert((ptBufInfo->nInBufCAddr[0]&0xFF)==0);
    //assert((ptBufInfo->nInBufCAddr[1]&0xFF)==0);

    // Y0
    addr = Chip_Phys_to_MIU(ptInBufCfg->nInBufYAddr[0]);
    pJpeReg->uReg06.tReg06.nRegJpeGBufYAdr0Low = (addr>>8)&0xFFFF;
    pJpeReg->uReg07.tReg07.nRegJpeGBufYAdr0High = addr>>(8+16);

    // Y1
    addr = Chip_Phys_to_MIU(ptInBufCfg->nInBufYAddr[1]);
    pJpeReg->uReg08.tReg08.nRegJpeGBufYAdr1Low = (addr>>8)&0xFFFF;
    pJpeReg->uReg09.tReg09.nRegJpeGBufYAdr1High = addr>>(8+16);

    // C0
    addr = Chip_Phys_to_MIU(ptInBufCfg->nInBufCAddr[0]);
    pJpeReg->uReg0a.tReg0a.nRegJpeGBufCAdr0Low = (addr>>8)&0xFFFF;
    pJpeReg->uReg0b.tReg0b.nRegJpeGBufCAdr0High = addr>>(8+16);

    // C1
    addr = Chip_Phys_to_MIU(ptInBufCfg->nInBufCAddr[1]);
    pJpeReg->uReg0c.tReg0c.nRegJpeGBufCAdr1Low = (addr>>8)&0xFFFF;
    pJpeReg->uReg0d.tReg0d.nRegJpeGBufCAdr1High = addr>>(8+16);

    // Input buffers
    //
    REGW(nBaseAddr,0x06, pJpeReg->uReg06.nReg06, "[%x] reg[%x] : Y0_low\n");
    REGW(nBaseAddr,0x07, pJpeReg->uReg07.nReg07, "[%x] reg[%x] : Y0_high\n");
    REGW(nBaseAddr,0x08, pJpeReg->uReg08.nReg08, "[%x] reg[%x] : Y1_low\n");
    REGW(nBaseAddr,0x09, pJpeReg->uReg09.nReg09, "[%x] reg[%x] : Y1 high\n");
    REGW(nBaseAddr,0x0a, pJpeReg->uReg0a.nReg0a, "[%x] reg[%x] : C0_low\n");
    REGW(nBaseAddr,0x0b, pJpeReg->uReg0b.nReg0b, "[%x] reg[%x] : C0_high\n");
    REGW(nBaseAddr,0x0c, pJpeReg->uReg0c.nReg0c, "[%x] reg[%x] : C1_low\n");
    REGW(nBaseAddr,0x0d, pJpeReg->uReg0d.nReg0d, "[%x] reg[%x] : C1_high\n");
}

/*******************************************************************************************************************
 * _HalJpeSetOutBuf
 *   Set output buffer
 *
 * Parameters:
 *   addr: physical address
 *   id:   index of addres
 *         0: start of 1st address
 *         1: end of 1st address
 *         2: start of 2nd address
 *         3: end of 2nd address
 *
 * Return:
 *   none
 */
static void _HalJpeSetOutBuf(JpeReg_t* pJpeReg,u32 nBaseAddr,u32 addr, u32 id)
{
    addr =  Chip_Phys_to_MIU(addr);

    // Write address info
    pJpeReg->uReg3c.tReg3c.nRegJpeSBspobufAdrLow = (addr>>2)&0xFFFF;
    pJpeReg->uReg3d.tReg3d.nRegJpeSBspobufAdrHigh = addr>>(2+16);
    pJpeReg->uReg3d.tReg3d.nRegJpeSBspobufId = id;
    REGW(nBaseAddr,0x3c, pJpeReg->uReg3c.nReg3c, "[%x] reg[%x] : bsp obuf address: \n");
    REGW(nBaseAddr,0x3d, pJpeReg->uReg3d.nReg3d, "[%x] reg[%x] : bsp obuf address high\n");

    // Enable write action?
    pJpeReg->uReg3e.tReg3e.nRegJpeSBspspobufWriteIdAdr = 1;
    REGW(nBaseAddr,0x3e, pJpeReg->uReg3e.nReg3e, "[%x] reg[%x] : bsp obuf write id address: \n");

    // jpe_reg.uReg3e.tReg3e.nRegJpeSBspspobufWriteIdAdr = 0;
    // REGW(0x3e, jpe_reg.uReg3e.nReg3e, "[%x] reg[%x] : bsp obuf write disable\n");
}


/*******************************************************************************************************************
 * HalJpeSetOutBuf
 *   Config input & output address
 *
 * Parameters:
 *   ptBufCfg:  ???
 *
 * Return:
 *   none
 */
void HalJpeSetOutBuf(JpeHalHandle_t* handle,pJpeHalOutBufCfg ptOutBufCfg)
{
    u32 nBaseAddr = handle->nBaseAddr;
    JpeReg_t* pJpeReg = handle->pJpeReg;
    // Start to config Output buffer
    pJpeReg->uReg3b.tReg3b.nRegJpeSBspFdcOffset = ptOutBufCfg->nJpeOutBitOffset;
#if defined(ADD_JPDE_0_1)
    pJpeReg->uReg3b.tReg3b.nRegJpeSBspobufSetAdr = 0;
    pJpeReg->uReg3b.tReg3b.nRegJpeObufToggleObuf0Status = 0;
    pJpeReg->uReg3b.tReg3b.nRegJpeObufToggleObuf1Status = 0;

    // Start to config output buffer (write one clear)
    pJpeReg->uReg3b.tReg3b.nRegJpeObufUpdateAdr = 1;
    REGW(nBaseAddr,0x3b, pJpeReg->uReg3b.nReg3b, "[%x] reg[%x] : bsp obuf update adr\n");
    pJpeReg->uReg3b.tReg3b.nRegJpeObufUpdateAdr = 0;	 // HW is write-one-clear
#endif


    // Set 1st address
    if(ptOutBufCfg->nOutBufAddr[0] != 0 && ptOutBufCfg->nOutBufSize[0] != 0)
    {
        JPE_MSG(JPE_MSG_DEBUG, "Set[0] 0x%08X, size %d\n", ptOutBufCfg->nOutBufAddr[0], ptOutBufCfg->nOutBufSize[0]);
        _HalJpeSetOutBuf(pJpeReg, nBaseAddr, ptOutBufCfg->nOutBufAddr[0], 0);
        _HalJpeSetOutBuf(pJpeReg, nBaseAddr, ptOutBufCfg->nOutBufAddr[0] + ptOutBufCfg->nOutBufSize[0]-1, 1);

    }

    // Set 2nd address
    if(ptOutBufCfg->nOutBufAddr[1] != 0 && ptOutBufCfg->nOutBufSize[1] != 0)
    {
        JPE_MSG(JPE_MSG_DEBUG, "Set[1] 0x%08X, size %d\n", ptOutBufCfg->nOutBufAddr[1], ptOutBufCfg->nOutBufSize[1]);
        _HalJpeSetOutBuf(pJpeReg,nBaseAddr,ptOutBufCfg->nOutBufAddr[1], 2);
        _HalJpeSetOutBuf(pJpeReg,nBaseAddr,ptOutBufCfg->nOutBufAddr[1] + ptOutBufCfg->nOutBufSize[1]-1, 3);
    }

    // Set configured address
    pJpeReg->uReg3b.tReg3b.nRegJpeSBspobufSetAdr = 1;
    REGW(nBaseAddr,0x3b, pJpeReg->uReg3b.nReg3b, "[%x] reg[%x] : buffer mode\n");
    pJpeReg->uReg3b.tReg3b.nRegJpeSBspobufSetAdr  = 0; // HW is write-one-clear


#if defined(ADD_JPDE_0_1)
    // Stop configuring output buffer (write one clear)
    pJpeReg->uReg3b.tReg3b.nRegJpeObufUpdateAdr = 1;
    REGW(nBaseAddr,0x3b, pJpeReg->uReg3b.nReg3b, "[%x] reg[%x] : bsp obuf update adr\n");
    pJpeReg->uReg3b.tReg3b.nRegJpeObufUpdateAdr = 0;	 // HW is write-one-clear
#endif
}

/*******************************************************************************************************************
 * HalJpeSetDctMbAddr
 *   ???
 *
 * Parameters:
 *   PhyAddr:  ???
 *
 * Return:
 *   none
 */
void HalJpeSetDctMbAddr(JpeHalHandle_t* handle,u32 PhyAddr)
{
    u32 nBaseAddr = handle->nBaseAddr;
    JpeReg_t* pJpeReg = handle->pJpeReg;
    u32 nMiuStartAddr, nMiuEndAddr, nY0MiuAddr;

    nMiuStartAddr = PhyAddr;
    nMiuEndAddr = PhyAddr+768;
    nY0MiuAddr = PhyAddr;

    REGW(nBaseAddr,0x06,(nY0MiuAddr>>8)&0xFFFF, "[%x] reg[%x] : Y0_low\n");
    REGW(nBaseAddr,0x07, nY0MiuAddr>>(8+16), "[%x] reg[%x] : Y0_high\n");
    // Output buffer
    pJpeReg->uReg3b.tReg3b.nRegJpeSBspFdcOffset = 0;
#if defined(ADD_JPDE_0_1)
    pJpeReg->uReg3b.tReg3b.nRegJpeSBspobufSetAdr = 0;
    pJpeReg->uReg3b.tReg3b.nRegJpeObufToggleObuf0Status = 0;
    pJpeReg->uReg3b.tReg3b.nRegJpeObufToggleObuf1Status = 0;
    pJpeReg->uReg3b.tReg3b.nRegJpeObufUpdateAdr = 1;
    REGW(nBaseAddr,0x3b, pJpeReg->uReg3b.nReg3b, "[%x] reg[%x] : bsp obuf update adr\n");
    pJpeReg->uReg3b.tReg3b.nRegJpeObufUpdateAdr = 0;
#endif

    pJpeReg->uReg3c.tReg3c.nRegJpeSBspobufAdrLow = (nMiuStartAddr>>2)&0xFFFF;
    pJpeReg->uReg3d.tReg3d.nRegJpeSBspobufAdrHigh = nMiuStartAddr>>(2+16);
    pJpeReg->uReg3d.tReg3d.nRegJpeSBspobufId = 0;
    pJpeReg->uReg3e.tReg3e.nRegJpeSBspspobufWriteIdAdr = 1;
    REGW(nBaseAddr,0x3c, pJpeReg->uReg3c.nReg3c, "[%x] reg[%x] : bsp obuf address: \n");
    REGW(nBaseAddr,0x3d, pJpeReg->uReg3d.nReg3d, "[%x] reg[%x] : bsp obuf address high\n");
    REGW(nBaseAddr,0x3e, pJpeReg->uReg3e.nReg3e, "[%x] reg[%x] : bsp obuf write id address: \n");

    pJpeReg->uReg3c.tReg3c.nRegJpeSBspobufAdrLow = (nMiuEndAddr>>2)&0xFFFF;
    pJpeReg->uReg3d.tReg3d.nRegJpeSBspobufAdrHigh = nMiuEndAddr>>(2+16);
    pJpeReg->uReg3d.tReg3d.nRegJpeSBspobufId = 1;
    pJpeReg->uReg3e.tReg3e.nRegJpeSBspspobufWriteIdAdr = 1;
    REGW(nBaseAddr,0x3c, pJpeReg->uReg3c.nReg3c, "[%x] reg[%x] : bsp obuf address: \n");
    REGW(nBaseAddr,0x3d, pJpeReg->uReg3d.nReg3d, "[%x] reg[%x] : bsp obuf address high\n");
    REGW(nBaseAddr,0x3e, pJpeReg->uReg3e.nReg3e, "[%x] reg[%x] : bsp obuf write id address: \n");

    pJpeReg->uReg3b.tReg3b.nRegJpeSBspobufSetAdr = 1;
    REGW(nBaseAddr,0x3b, pJpeReg->uReg3b.nReg3b, "[%x] reg[%x] : buffer mode\n");
    pJpeReg->uReg3b.tReg3b.nRegJpeSBspobufSetAdr  = 0;	// HW is write-one-clear
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                                                                                                                                  //
//                                                                  Controls                                                                        //
//                                                                                                                                                  //
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/*******************************************************************************************************************
 * _HalJpeSoftReset
 *   Reset JPEG engine
 *
 * Parameters:
 *   none
 *
 * Return:
 *   none
 */
static void _HalJpeSoftReset(JpeReg_t* pJpeReg,u32 nBaseAddr)
{
    pJpeReg->uReg00.tReg00.nRegJpeGMode = 0;     // Assure JPE is running.
    pJpeReg->uReg00.tReg00.nRegJpeGSoftRstz =0;
    REGW(nBaseAddr,0x0, pJpeReg->uReg00.nReg00, "[%x] reg[%x] : SW reset 0\n");

    // delay for lower MCM frequence
    // Repeated write to replace udelay(1) becase 1 us is too long
    REGW(nBaseAddr,0x0, pJpeReg->uReg00.nReg00, "[%x] reg[%x] : SW reset 0\n");
    REGW(nBaseAddr,0x0, pJpeReg->uReg00.nReg00, "[%x] reg[%x] : SW reset 0\n");
    REGW(nBaseAddr,0x0, pJpeReg->uReg00.nReg00, "[%x] reg[%x] : SW reset 0\n");
    REGW(nBaseAddr,0x0, pJpeReg->uReg00.nReg00, "[%x] reg[%x] : SW reset 0\n");

    pJpeReg->uReg00.tReg00.nRegJpeGSoftRstz = 1;
    REGW(nBaseAddr,0x0, pJpeReg->uReg00.nReg00, "[%x] reg[%x] : SW reset 1\n");
}

/*******************************************************************************************************************
 * HalJpeSoftReset
 *   SW reset JPEG engine & internal variables
 *
 * Parameters:
 *   handle: HAL handle
 *
 * Return:
 *   none
 */
void HalJpeSoftReset(JpeHalHandle_t *handle)
{
    u32 nBaseAddr = handle->nBaseAddr;
    JpeReg_t* pJpeReg = handle->pJpeReg;
#define _UPDATE_REG_VAR(nBaseAddr,reg_no) pJpeReg->uReg##reg_no.nReg##reg_no = REGR(nBaseAddr,0x##reg_no, "\n");//REGR(0x##reg_no, "\n");
    // SW reset at first
    _HalJpeSoftReset(pJpeReg,nBaseAddr);

    // Sync all HW setting to jpe_reg
    _UPDATE_REG_VAR(nBaseAddr,00);
    _UPDATE_REG_VAR(nBaseAddr,01);
    _UPDATE_REG_VAR(nBaseAddr,02);
    _UPDATE_REG_VAR(nBaseAddr,03);
    _UPDATE_REG_VAR(nBaseAddr,06);
    _UPDATE_REG_VAR(nBaseAddr,07);
    _UPDATE_REG_VAR(nBaseAddr,08);
    _UPDATE_REG_VAR(nBaseAddr,09);
    _UPDATE_REG_VAR(nBaseAddr,0a);
    _UPDATE_REG_VAR(nBaseAddr,0b);
    _UPDATE_REG_VAR(nBaseAddr,0c);
    _UPDATE_REG_VAR(nBaseAddr,0d);
    _UPDATE_REG_VAR(nBaseAddr,0f);
    _UPDATE_REG_VAR(nBaseAddr,10);
    _UPDATE_REG_VAR(nBaseAddr,16);
    _UPDATE_REG_VAR(nBaseAddr,18);
    _UPDATE_REG_VAR(nBaseAddr,1c);
    _UPDATE_REG_VAR(nBaseAddr,1d);
    _UPDATE_REG_VAR(nBaseAddr,1e);
    _UPDATE_REG_VAR(nBaseAddr,1f);
    _UPDATE_REG_VAR(nBaseAddr,20);
    _UPDATE_REG_VAR(nBaseAddr,21);
    _UPDATE_REG_VAR(nBaseAddr,2c);
    _UPDATE_REG_VAR(nBaseAddr,2d);
    _UPDATE_REG_VAR(nBaseAddr,2e);
    _UPDATE_REG_VAR(nBaseAddr,32);
    _UPDATE_REG_VAR(nBaseAddr,33);
    _UPDATE_REG_VAR(nBaseAddr,34);
    _UPDATE_REG_VAR(nBaseAddr,35);
    _UPDATE_REG_VAR(nBaseAddr,36);
    _UPDATE_REG_VAR(nBaseAddr,3b);
    _UPDATE_REG_VAR(nBaseAddr,3c);
    _UPDATE_REG_VAR(nBaseAddr,3d);
    _UPDATE_REG_VAR(nBaseAddr,3e);
    _UPDATE_REG_VAR(nBaseAddr,42);
    _UPDATE_REG_VAR(nBaseAddr,44);
    _UPDATE_REG_VAR(nBaseAddr,45);
    _UPDATE_REG_VAR(nBaseAddr,48);
    _UPDATE_REG_VAR(nBaseAddr,49);
    _UPDATE_REG_VAR(nBaseAddr,4a);
    _UPDATE_REG_VAR(nBaseAddr,4b);
    _UPDATE_REG_VAR(nBaseAddr,54);
    _UPDATE_REG_VAR(nBaseAddr,55);
    _UPDATE_REG_VAR(nBaseAddr,56);
    _UPDATE_REG_VAR(nBaseAddr,57);
    _UPDATE_REG_VAR(nBaseAddr,58);
    _UPDATE_REG_VAR(nBaseAddr,59);
    _UPDATE_REG_VAR(nBaseAddr,5a);
    _UPDATE_REG_VAR(nBaseAddr,5b);
    _UPDATE_REG_VAR(nBaseAddr,5c);
    _UPDATE_REG_VAR(nBaseAddr,5d);
    _UPDATE_REG_VAR(nBaseAddr,5e);
    _UPDATE_REG_VAR(nBaseAddr,5f);
    _UPDATE_REG_VAR(nBaseAddr,60);
    _UPDATE_REG_VAR(nBaseAddr,61);
    _UPDATE_REG_VAR(nBaseAddr,64);
    _UPDATE_REG_VAR(nBaseAddr,6f);
    _UPDATE_REG_VAR(nBaseAddr,70);
    _UPDATE_REG_VAR(nBaseAddr,71);
    _UPDATE_REG_VAR(nBaseAddr,72);
    _UPDATE_REG_VAR(nBaseAddr,73);
    _UPDATE_REG_VAR(nBaseAddr,74);
    _UPDATE_REG_VAR(nBaseAddr,75);
    _UPDATE_REG_VAR(nBaseAddr,76);
    _UPDATE_REG_VAR(nBaseAddr,77);
    _UPDATE_REG_VAR(nBaseAddr,78);
    _UPDATE_REG_VAR(nBaseAddr,79);
    _UPDATE_REG_VAR(nBaseAddr,7a);
    _UPDATE_REG_VAR(nBaseAddr,7b);
    _UPDATE_REG_VAR(nBaseAddr,7c);
}

/*******************************************************************************************************************
 * HalJpeSetEnable
 *   Enable JPEG enging
 *
 * Parameters:
 *   none
 *
 * Return:
 *   none
 */
void HalJpeSetEnable(JpeHalHandle_t *handle)
{
    u32 nBaseAddr = handle->nBaseAddr;
    JpeReg_t* pJpeReg = handle->pJpeReg;
    pJpeReg->uReg00.tReg00.nRegJpeGFrameStartSw = 1;
    REGW(nBaseAddr,0x00, pJpeReg->uReg00.nReg00, "[%x] reg[%x] : frame start\n");
    pJpeReg->uReg00.tReg00.nRegJpeGFrameStartSw = 0; // write-one-clear
}

/*******************************************************************************************************************
 * HalJpeHwRowCounterReset
 *   SW reset JPEG VIU?
 *
 * Parameters:
 *   none
 *
 * Return:
 *   none
 */
void HalJpeHwRowCounterReset(JpeHalHandle_t *handle)
{
    u32 nBaseAddr = handle->nBaseAddr;
    JpeReg_t* pJpeReg = handle->pJpeReg;
    pJpeReg->uReg18.tReg18.nRegJpeGViuSoftRstz=0;
    REGW(nBaseAddr,0x18,pJpeReg->uReg18.nReg18, "[%x] reg[%x] : VIU reset 0\n");
    pJpeReg->uReg18.tReg18.nRegJpeGViuSoftRstz=1;
    REGW(nBaseAddr,0x18, pJpeReg->uReg18.nReg18, "[%x] reg[%x] : VIU reset 1\n");
}

/*******************************************************************************************************************
 * HalJpeSetSwVsnc
 *   ???
 *
 * Parameters:
 *   none
 *
 * Return:
 *   none
 */
void HalJpeSetSWRowDown(JpeHalHandle_t *handle)
{
    u32 nBaseAddr = handle->nBaseAddr;
    JpeReg_t* pJpeReg = handle->pJpeReg;
    pJpeReg->uReg75.tReg75.nRegJpeGSwRowDone = 1;
    REGW(nBaseAddr,0x75, pJpeReg->uReg75.nReg75,"[%x] reg[%x] :SW Row Down\n");
    pJpeReg->uReg75.tReg75.nRegJpeGSwRowDone = 0; // write one clear
}


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                                                                                                                                  //
//                                                                  Status                                                                          //
//                                                                                                                                                  //
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/*******************************************************************************************************************
 * HalJpeResetSafe
 *   to indicate there're no MIU activities that need to pay attention to
 *
 * Parameters:
 *   none
 *
 * Return:
 *   true:  MIU accesse is active
 *   false: no MIU activity
 */
bool HalJpeResetSafe(JpeHalHandle_t *handle)
{
    u32 nBaseAddr = handle->nBaseAddr;
    JpeReg_t* pJpeReg = handle->pJpeReg;
    pJpeReg->uReg1d.nReg1d = REGR(nBaseAddr,0x1d, "[%x] reg[%x] :reset safe\n");

    ms_dprintf(0, "uReg1d.tReg1d.nRegJpeGSwrstSafe is %d\n", pJpeReg->uReg1d.tReg1d.nRegJpeGSwrstSafe);

    return (pJpeReg->uReg1d.tReg1d.nRegJpeGSwrstSafe == 1) ? true : false;
}

/*******************************************************************************************************************
 * HalJpeGetOutputByteSize
 *   Retrive the output size
 *
 * Parameters:
 *   nObufSel: buffer index
 *
 * Return:
 *   Output buffer size
 */
u32 HalJpeGetOutputByteSize(JpeHalHandle_t *handle,u32 nObufSel)
{
    u32 nBaseAddr = handle->nBaseAddr;
    JpeReg_t* pJpeReg = handle->pJpeReg;
    u16 u16Reg1, u16Reg2, u16Reg3;
    u32 wptr, oBufSadr;
    u16 u16Reg;
    //u16 i;

    u16Reg = REGR(nBaseAddr,0x3b, "[%x] reg[%x] : obuf status\n");
    pJpeReg->uReg3b.nReg3b = u16Reg;
    pJpeReg->uReg3b.tReg3b.nRegJpeObufAdrRchkEn = 1;
/*
    for (i=0; i<4; i++) {
        jpe_reg.uReg3b.tReg3b.nRegJpeObufAdrRchkSe1 = i;
        REGW(0x3b, jpe_reg.uReg3b.nReg3b, "[%x] reg[%x] : write 3b\n");
        _ReadRegJpe(0x44, &u16Reg1, "[%x] reg[%x] : wptr low\n");
        _ReadRegJpe(0x45, &u16Reg2, "[%x] reg[%x] : wptr high\n");
        oBufSadr = ((u16Reg2<<16) | u16Reg1)<<2;
        ms_dprintf(1,"Obuf%d %d %d(s=0,e=1) = 0x%08x\n",i,i>>1, i&1, oBufSadr);
    }
*/
    //obuf adr read back check selection(0/1/2/3:s0/e0/s1/e1)
    if (nObufSel == JPE_OBUF_0)
        pJpeReg->uReg3b.tReg3b.nRegJpeObufAdrRchkSe1 = 0; //s0
    else
        pJpeReg->uReg3b.tReg3b.nRegJpeObufAdrRchkSe1 = 2; //s1

    REGW(nBaseAddr,0x3b, pJpeReg->uReg3b.nReg3b, "[%x] reg[%x] : write 3b\n");
    u16Reg1 = REGR(nBaseAddr,0x44, "[%x] reg[%x] : wptr low\n");
    u16Reg2 = REGR(nBaseAddr,0x45, "[%x] reg[%x] : wptr high\n");
    oBufSadr = ((u16Reg2<<16) | u16Reg1)<<2;

    pJpeReg->uReg3b.tReg3b.nRegJpeObufAdrRchkEn = 0;
    REGW(nBaseAddr,0x3b, pJpeReg->uReg3b.nReg3b, "[%x] reg[%x] : write 3b\n");
    u16Reg1 = REGR(nBaseAddr,0x44, "[%x] reg[%x] : wptr low\n");
    u16Reg2 = REGR(nBaseAddr,0x45, "[%x] reg[%x] : wptr high\n");
    wptr = ((u16Reg2<<16) | u16Reg1)<<2;

    u16Reg3 = REGR(nBaseAddr,0x42, "[%x] reg[%x] : last bytes in miu\n");

    u16Reg3 &= 3;
    u16Reg3 += 1;

    return wptr - oBufSadr + u16Reg3;
}

/*******************************************************************************************************************
 * HalJpeGetRowDownCnt
 *   ???
 *
 * Parameters:
 *   none
 *
 * Return:
 *   none
 */
u16 HalJpeGetRowDownCnt(JpeHalHandle_t *handle)
{
    u32 nBaseAddr = handle->nBaseAddr;
    u16 u16Reg;
    u16Reg = REGR(nBaseAddr,0x10, "[%x] reg[%x] :Row down count\n");
    return u16Reg&0x0F;
}


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                                                                                                                                  //
//                                                                   TXIP?                                                                          //
//                                                                                                                                                  //
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/*******************************************************************************************************************
 * HalJpeStop
 *   ???
 *
 * Parameters:
 *   opt: ???
 *
 * Return:
 *   none
 */
void HalJpeStop(JpeHalHandle_t *handle,bool opt)
{
    u32 nBaseAddr = handle->nBaseAddr;
    JpeReg_t* pJpeReg = handle->pJpeReg;
    pJpeReg->uReg2d.tReg2d.nRegJpeSTxipSngSet = opt;
    REGW(nBaseAddr,0x2d, pJpeReg->uReg2d.nReg2d, "[%x] reg[%x] : stop\n");
}

/*******************************************************************************************************************
 * HalJpeGetRowCount
 *   ???
 *
 * Parameters:
 *   none
 *
 * Return:
 *   none
 */
u16 HalJpeGetRowCount(JpeHalHandle_t *handle)
{
    u32 nBaseAddr = handle->nBaseAddr;
    JpeReg_t* pJpeReg = handle->pJpeReg;
    pJpeReg->uReg2e.nReg2e = REGR(nBaseAddr,0x2e, "[%x] reg[%x] :Row count\n");
    return pJpeReg->uReg2e.nReg2e;
}

/*******************************************************************************************************************
 * HalJpeIdleCount
 *   ???
 *
 * Parameters:
 *   none
 *
 * Return:
 *   none
 */
void HalJpeIdleCount(JpeHalHandle_t *handle,u16 nIdleCount)
{
    u32 nBaseAddr = handle->nBaseAddr;
    JpeReg_t* pJpeReg = handle->pJpeReg;
    pJpeReg->uReg32.tReg32.nRegJpeSTxipWaitMode = 0;
    pJpeReg->uReg33.tReg33.nRegJpeSTxipIdlecnt = nIdleCount;

    REGW(nBaseAddr,0x32, pJpeReg->uReg32.nReg32, "[%x] reg[%x] : wait mode\n");
    REGW(nBaseAddr,0x33, pJpeReg->uReg33.nReg33, "[%x] reg[%x] : idle count\n");
}


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                                                                                                                                  //
//                                                                   Debugs?                                                                        //
//                                                                                                                                                  //
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/*******************************************************************************************************************
 * HalJpeSetCycleReport
 *   ???
 *
 * Parameters:
 *   mode: ???
 *
 * Return:
 *   none
 */
void HalJpeSetCycleReport(JpeHalHandle_t *handle,u16 mode)
{
    u32 nBaseAddr = handle->nBaseAddr;
    JpeReg_t* pJpeReg = handle->pJpeReg;
#ifdef CYCLE_REPORT
    pJpeReg->uReg73.tReg73.nRegJpeGDebugTcycleChkEn = 1;
    pJpeReg->uReg73.tReg73.nRegJpeGDebugTcycleChkSel = 1;
#else
    pJpeReg->uReg73.tReg73.nRegJpeGDebugTcycleChkEn = 0;
    pJpeReg->uReg73.tReg73.nRegJpeGDebugTcycleChkSel = 0;
#endif
    REGW(nBaseAddr,0x73, pJpeReg->uReg73.nReg73, "[%x] reg[%x] : cycle report\n");
}

/*******************************************************************************************************************
 * HalJpeSetCrc
 *   ???
 *
 * Parameters:
 *   mode: ???
 *
 * Return:
 *   none
 */
void HalJpeSetCrc(JpeHalHandle_t *handle,u16 mode)
{
    u32 nBaseAddr = handle->nBaseAddr;
    JpeReg_t* pJpeReg = handle->pJpeReg;
    pJpeReg->uReg73.tReg73.nRegJpeGCrcMode = 0x2;
    REGW(nBaseAddr,0x73, pJpeReg->uReg73.nReg73, "[%x] reg[%x] : crc mode\n");
}



//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                                                                                                                                  //
//                                                                Capibilities?                                                                     //
//                                                                                                                                                  //
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/*******************************************************************************************************************
 * HalJpeGetBufferModeSupportInfo
 *   Capability of supported mode of current JPEG HW
 *
 * Parameters:
 *   none
 *
 * Return:
 *   The bitwise combination of supported mode
 */
u8 HalJpeGetBufferModeSupportInfo(void)
{
    return JPE_HAL_BUFFER_ROW_MODE|JPE_HAL_BUFFER_FRAME_MODE;
}

/*******************************************************************************************************************
 * HalJpeGetSclHandShakeSupportInfo
 *   Capability of scaler data handshake
 *
 * Parameters:
 *   none
 *
 * Return:
 *   JpeHalSclHandShake_e
 */
u8 HalJpeGetSclHandShakeSupportInfo(void)
{
    return JPE_HAL_SCL_HANDSHAKE_ON;
}

/*******************************************************************************************************************
 * HalJpeGetCodecSupportInfo
 *   Capability of supported codec format
 *
 * Parameters:
 *   none
 *
 * Return:
 *   The bitwize combination of supported codec mode
 */
u8 HalJpeGetCodecSupportInfo(void)
{
    return JPE_HAL_CODEC_JPEG;
}

/*******************************************************************************************************************
 * HalJpeGetOutBufSLogAddrAlignInfo
 *   Alignment of output buffer address (start)
 *
 * Parameters:
 *   none
 *
 * Return:
 *   The alignment
 */
u32 HalJpeGetOutBufSLogAddrAlignInfo(void)
{
    return 4;
}

/*******************************************************************************************************************
 * HalJpeGetOutBufELogAddrAlignInfo
 *   Alignment of output buffer address (end)
 *
 * Parameters:
 *   none
 *
 * Return:
 *   The alignment
 */
u32 HalJpeGetOutBufELogAddrAlignInfo(void)
{
    return 4;
}

/*******************************************************************************************************************
 * HalJpeGetRefCLogAddrAlignInfo
 *   Alignment of input buffer address (Y)
 *
 * Parameters:
 *   bufID: not use....
 *
 * Return:
 *   The alignment
 */
u32 HalJpeGetRefYLogAddrAlignInfo(u8 bufID)
{
    return 256;

}

/*******************************************************************************************************************
 * HalJpeGetRefCLogAddrAlignInfo
 *   Alignment of input buffer address (C)
 *
 * Parameters:
 *   bufID: not use....
 *
 * Return:
 *   The alignment
 */
u32 HalJpeGetRefCLogAddrAlignInfo(u8 bufID)
{
    return 256;
}

