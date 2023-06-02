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

#define TIME_OUT_CNT 10

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
static u16 _HalJpeIrqMaskConvertEnumerate2Real(JpeReg_t* pJpeReg, JpeHalIrqEvent_e mask)
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
static JpeHalIrqEvent_e _HalJpeIrqMaskConvertReal2Enumerate(JpeReg_t* pJpeReg, u16 mask)
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
 * HalJpeGetIrqCheck
 *   Check current IRQ status
 *
 * Parameters:
 *   mask: Bitwise mask to be checked
 *
 * Return:
 *   Checked result, bitwise
 */
static JpeHalIrqEvent_e _HalJpeGetIrqCheck(u32 nBaseAddr, JpeReg_t* pJpeReg, JpeHalIrqEvent_e mask)
{
    u16 reg_value;
    reg_value = REGR(nBaseAddr, 0x1e, "[%x] reg[%x] :IRQ cpu and ip status\n");
    return _HalJpeIrqMaskConvertReal2Enumerate(pJpeReg, reg_value) & mask;
}


/*******************************************************************************************************************
 * _HalJpeSetIrqMask
 *   Set interrupt trigger mask
 *
 * Parameters:
 *   mask: Bitwise mask, defined in JpeHalIrqEvent_e
 *
 * Return:
 *   none
 */
static void _HalJpeSetIrqMask(u32 nBaseAddr, JpeReg_t* pJpeReg, JpeHalIrqEvent_e nMask)
{
    pJpeReg->uReg1c.tReg1c.nRegJpeGIrqMask = _HalJpeIrqMaskConvertEnumerate2Real(pJpeReg, nMask);
    REGW(nBaseAddr, 0x1c, pJpeReg->uReg1c.nReg1c, "[%x] reg[%x] : irq mask\n");
}


/*******************************************************************************************************************
 * _HalJpeClearIrq
 *   Clear triggered interrupt
 *
 * Parameters:
 *   mask: Bitwise mask, defined in JpeHalIrqEvent_e
 *
 * Return:
 *   none
 */
static void _HalJpeClearIrq(u32 nBaseAddr, JpeReg_t* pJpeReg, JpeHalIrqEvent_e nMask)
{
    pJpeReg->uReg1d.nReg1d = REGR(nBaseAddr, 0x1d, "[%x] reg[%x] :IRQ register\n");

    pJpeReg->uReg1d.tReg1d.nRegJpeGIrqClr = _HalJpeIrqMaskConvertEnumerate2Real(pJpeReg, nMask);
    REGW(nBaseAddr, 0x1d, pJpeReg->uReg1d.nReg1d, "[%x] reg[%x] : Clear IRQ\n");
    pJpeReg->uReg1d.nReg1d = REGR(nBaseAddr, 0x1d, "[%x] reg[%x] :IRQ register\n"); // HW is write-one-clear, so update again
}


/*******************************************************************************************************************
 * _HalJpeGetIrq
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
static u16 _HalJpeGetIrq(u32 nBaseAddr)
{
    u16 u16Reg;
    u16Reg = REGR(nBaseAddr, 0x1e, "[%x] reg[%x] :IRQ cpu and ip status\n");
    return u16Reg;
}


/*******************************************************************************************************************
 * _HalJpeGetOutputByteSize
 *   Retrive the output size
 *
 * Parameters:
 *   nObufSel: buffer index
 *
 * Return:
 *   Output buffer size
 */
static u32 _HalJpeGetOutputByteSize(u32 nBaseAddr, JpeReg_t* pJpeReg, u32 nObufSel)
{
    u16 u16Reg1, u16Reg2, u16Reg3;
    u32 wptr, oBufSadr;
    u16 u16Reg;

    u16Reg = REGR(nBaseAddr, 0x3b, "[%x] reg[%x] : obuf status\n");
    pJpeReg->uReg3b.nReg3b = u16Reg;
    pJpeReg->uReg3b.tReg3b.nRegJpeObufAdrRchkEn = 1;

    //obuf adr read back check selection(0/1/2/3:s0/e0/s1/e1)
    if(nObufSel == JPE_OBUF_0)
        pJpeReg->uReg3b.tReg3b.nRegJpeObufAdrRchkSe1 = 0; //s0
    else
        pJpeReg->uReg3b.tReg3b.nRegJpeObufAdrRchkSe1 = 2; //s1

    REGW(nBaseAddr, 0x3b, pJpeReg->uReg3b.nReg3b, "[%x] reg[%x] : write 3b\n");
    u16Reg1 = REGR(nBaseAddr, 0x44, "[%x] reg[%x] : wptr low\n");
    u16Reg2 = REGR(nBaseAddr, 0x45, "[%x] reg[%x] : wptr high\n");
    oBufSadr = ((u16Reg2 << 16) | u16Reg1) << 2;

    pJpeReg->uReg3b.tReg3b.nRegJpeObufAdrRchkEn = 0;
    REGW(nBaseAddr, 0x3b, pJpeReg->uReg3b.nReg3b, "[%x] reg[%x] : write 3b\n");
    u16Reg1 = REGR(nBaseAddr, 0x44, "[%x] reg[%x] : wptr low\n");
    u16Reg2 = REGR(nBaseAddr, 0x45, "[%x] reg[%x] : wptr high\n");
    wptr = ((u16Reg2 << 16) | u16Reg1) << 2;

    u16Reg3 = REGR(nBaseAddr, 0x42, "[%x] reg[%x] : last bytes in miu\n");

    u16Reg3 &= 3;
    u16Reg3 += 1;

    return wptr - oBufSadr + u16Reg3;
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
static void __HalJpeSetOutBuf(u32 nBaseAddr, JpeReg_t* pJpeReg, u32 addr, u32 id)
{
    addr =  Chip_Phys_to_MIU(addr);

    // Write address info
    pJpeReg->uReg3c.tReg3c.nRegJpeSBspobufAdrLow = (addr >> 2) & 0xFFFF;
    pJpeReg->uReg3d.tReg3d.nRegJpeSBspobufAdrHigh = addr >> (2 + 16);
    pJpeReg->uReg3d.tReg3d.nRegJpeSBspobufId = id;
    REGW(nBaseAddr, 0x3c, pJpeReg->uReg3c.nReg3c, "[%x] reg[%x] : bsp obuf address: \n");
    REGW(nBaseAddr, 0x3d, pJpeReg->uReg3d.nReg3d, "[%x] reg[%x] : bsp obuf address high\n");

    // Enable write action?
    pJpeReg->uReg3e.tReg3e.nRegJpeSBspspobufWriteIdAdr = 1;
    REGW(nBaseAddr, 0x3e, pJpeReg->uReg3e.nReg3e, "[%x] reg[%x] : bsp obuf write id address: \n");

}


/*******************************************************************************************************************
 * _HalJpeSetOutBuf
 *   Config input & output address
 *
 * Parameters:
 *   ptBufCfg:  ???
 *
 * Return:
 *   none
 */
static void _HalJpeSetOutBuf(u32 nBaseAddr, JpeReg_t* pJpeReg, JpeHalOutBufCfg_t *pOutBufCfg)
{
    // Start to config Output buffer
    pJpeReg->uReg3b.tReg3b.nRegJpeSBspFdcOffset = pOutBufCfg->nJpeOutBitOffset;
#if defined(ADD_JPDE_0_1)
    pJpeReg->uReg3b.tReg3b.nRegJpeSBspobufSetAdr = 0;
    pJpeReg->uReg3b.tReg3b.nRegJpeObufToggleObuf0Status = 0;
    pJpeReg->uReg3b.tReg3b.nRegJpeObufToggleObuf1Status = 0;

    // Start to config output buffer (write one clear)
    pJpeReg->uReg3b.tReg3b.nRegJpeObufUpdateAdr = 1;
    REGW(nBaseAddr, 0x3b, pJpeReg->uReg3b.nReg3b, "[%x] reg[%x] : bsp obuf update adr\n");
    pJpeReg->uReg3b.tReg3b.nRegJpeObufUpdateAdr = 0;	 // HW is write-one-clear
#endif


    // Set 1st address
    if(pOutBufCfg->nOutBufAddr[0] != 0 && pOutBufCfg->nOutBufSize[0] != 0)
    {
        //JPE_MSG(JPE_MSG_DEBUG, "Set[0] 0x%08X, size %d\n", ptOutBufCfg->nOutBufAddr[0], ptOutBufCfg->nOutBufSize[0]);
        __HalJpeSetOutBuf(nBaseAddr, pJpeReg,  pOutBufCfg->nOutBufAddr[0], 0);
        __HalJpeSetOutBuf(nBaseAddr, pJpeReg, pOutBufCfg->nOutBufAddr[0] + pOutBufCfg->nOutBufSize[0] - 1, 1);

    }

    // Set 2nd address
    if(pOutBufCfg->nOutBufAddr[1] != 0 && pOutBufCfg->nOutBufSize[1] != 0)
    {
        //JPE_MSG(JPE_MSG_DEBUG, "Set[1] 0x%08X, size %d\n", ptOutBufCfg->nOutBufAddr[1], ptOutBufCfg->nOutBufSize[1]);
        __HalJpeSetOutBuf(nBaseAddr, pJpeReg, pOutBufCfg->nOutBufAddr[1], 2);
        __HalJpeSetOutBuf(nBaseAddr, pJpeReg, pOutBufCfg->nOutBufAddr[1] + pOutBufCfg->nOutBufSize[1] - 1, 3);
    }

    // Set configured address
    pJpeReg->uReg3b.tReg3b.nRegJpeSBspobufSetAdr = 1;
    REGW(nBaseAddr, 0x3b, pJpeReg->uReg3b.nReg3b, "[%x] reg[%x] : buffer mode\n");
    pJpeReg->uReg3b.tReg3b.nRegJpeSBspobufSetAdr  = 0; // HW is write-one-clear

#if defined(ADD_JPDE_0_1)
    // Stop configuring output buffer (write one clear)
    pJpeReg->uReg3b.tReg3b.nRegJpeObufUpdateAdr = 1;
    REGW(nBaseAddr, 0x3b, pJpeReg->uReg3b.nReg3b, "[%x] reg[%x] : bsp obuf update adr\n");
    pJpeReg->uReg3b.tReg3b.nRegJpeObufUpdateAdr = 0;	 // HW is write-one-clear
#endif
}


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
static int _HalJpeQTableEntryRead(JpeReg_t* pJpeReg, u32 nBaseAddr, u16 index, u16 *value)
{
    int retry = TIME_OUT_CNT;


    //read data
    pJpeReg->uReg49.tReg49.nRegJpeSTbcRw = 0;
    pJpeReg->uReg48.tReg48.nRegJpeSTbcEn = 1;
    pJpeReg->uReg49.tReg49.nRegJpeSTbcAdr = index;
    REGW(nBaseAddr, 0x49, pJpeReg->uReg49.nReg49, "[%x] reg[%x] : table address\n");
    REGW(nBaseAddr, 0x48, pJpeReg->uReg48.nReg48, "[%x] reg[%x] : table read enable\n");
    pJpeReg->uReg48.tReg48.nRegJpeSTbcEn = 0;    // write one clear

    while(retry-- > 0)
    {
        pJpeReg->uReg49.nReg49 = REGR(nBaseAddr, 0x49, "[%x] reg[%x] : table write enable\n");
        if(pJpeReg->uReg49.tReg49.nRegJpeSTbcDone)
        {
            // Clear done
            pJpeReg->uReg49.tReg49.nRegJpeSTbcDoneClr = 1;
            REGW(nBaseAddr, 0x49, pJpeReg->uReg49.nReg49, "[%x] reg[%x] : table write enable\n");
            pJpeReg->uReg49.tReg49.nRegJpeSTbcDoneClr = 0;

            // Get value
            *value = REGR(nBaseAddr, 0x4b, "[%x] reg[%x] : table read enable\n");
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
static int _HalJpeQTableEntryWrite(JpeReg_t* pJpeReg, u32 nBaseAddr, u16 index, u16 value)
{
    int retry = TIME_OUT_CNT;

    // Set value
    pJpeReg->uReg4a.tReg4a.nRegJpeSTbcWdata = value;
    pJpeReg->uReg49.tReg49.nRegJpeSTbcRw = 1;
    pJpeReg->uReg48.tReg48.nRegJpeSTbcEn = 1;
    pJpeReg->uReg49.tReg49.nRegJpeSTbcAdr = index;
    REGW(nBaseAddr, 0x4a, pJpeReg->uReg4a.nReg4a, "[%x] reg[%x] : table write data\n");
    REGW(nBaseAddr, 0x49, pJpeReg->uReg49.nReg49, "[%x] reg[%x] : table address\n");
    REGW(nBaseAddr, 0x48, pJpeReg->uReg48.nReg48, "[%x] reg[%x] : table write enable\n");
    pJpeReg->uReg48.tReg48.nRegJpeSTbcEn = 0;    // write one clear

    // Wait for set done
    while(retry-- > 0)
    {
        pJpeReg->uReg49.nReg49 = REGR(nBaseAddr, 0x49, "[%x] reg[%x] : table write enable\n");
        if(pJpeReg->uReg49.tReg49.nRegJpeSTbcDone)
        {
            // Clear done
            pJpeReg->uReg49.tReg49.nRegJpeSTbcDoneClr = 1;
            REGW(nBaseAddr, 0x49, pJpeReg->uReg49.nReg49, "[%x] reg[%x] : table write enable\n");
            pJpeReg->uReg49.tReg49.nRegJpeSTbcDoneClr = 0;
            break;
        }
    }

    return retry;
}


/*******************************************************************************************************************
 * _HalJpeSetQTable
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
int _HalJpeSetQTable(u32 nBaseAddr, JpeReg_t* pJpeReg, const u16 *q_table_y, const u16 *q_table_c, u32 size)
{
    u16 value_w, value_r;
    int i, retry, ret = 0;

    // Switch SW mode to update Q table
    pJpeReg->uReg03.tReg03.nRegJpeGTbcMode = 0;
    REGW(nBaseAddr, 0x3, pJpeReg->uReg03.nReg03, "[%x] reg[%x] : tbc_mode=0\n");


    // Udate Y table
    for(i = 0; i < size; i++)
    {
        retry = TIME_OUT_CNT;
        while(retry-- > 0)
        {
            value_w = q_table_y[((i & 0x7) << 3) | (i >> 3)];

            // Write
            if(_HalJpeQTableEntryWrite(pJpeReg, nBaseAddr, i, value_w) <= 0)
            {
                ret = -1;
                goto RETURN;
            }

            // Read
            if(_HalJpeQTableEntryRead(pJpeReg, nBaseAddr, i, &value_r) <= 0)
            {
                ret = -1;
                goto RETURN;
            }

            // Compare
            if(value_r == value_w)
                break;
        }
    }

    // Udate C table
    for(i = 0; i < size; i++)
    {
        retry = TIME_OUT_CNT;
        while(retry-- > 0)
        {
            value_w = q_table_c[((i & 0x7) << 3) | (i >> 3)];

            // Write
            if(_HalJpeQTableEntryWrite(pJpeReg, nBaseAddr, i + 64, value_w) <= 0)
            {
                ret = -1;
                goto RETURN;
            }

            // Read
            if(_HalJpeQTableEntryRead(pJpeReg, nBaseAddr, i + 64, &value_r) <= 0)
            {
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
    REGW(nBaseAddr, 0x2c, pJpeReg->uReg2c.nReg2c, "[%x] reg[%x] : Last zigzag\n");

RETURN:
    // Switch to hw mode
    pJpeReg->uReg03.tReg03.nRegJpeGTbcMode = 1;
    REGW(nBaseAddr, 0x3, pJpeReg->uReg03.nReg03, "[%x] reg[%x] : tbc_mode=1\n");

    return ret;
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
static void _HalJpeSetInBuf(JpeHalHandle_t* handle, pJpeHalInBufCfg ptInBufCfg)
{
    u32 addr;
    u32 nBaseAddr = handle->nBaseAddr;
    JpeReg_t* pJpeReg = &handle->tJpeReg;

    // Y0
    if(ptInBufCfg->nInBufYAddr[0])
    {
        addr = Chip_Phys_to_MIU(ptInBufCfg->nInBufYAddr[0]);
    }
    else
    {
        addr = 0;
    }

    pJpeReg->uReg06.tReg06.nRegJpeGBufYAdr0Low = (addr >> 8) & 0xFFFF;
    pJpeReg->uReg07.tReg07.nRegJpeGBufYAdr0High = addr >> (8 + 16);

    // Y1
    if(ptInBufCfg->nInBufYAddr[1])
    {
        addr = Chip_Phys_to_MIU(ptInBufCfg->nInBufYAddr[1]);
    }
    else
    {
        addr = 0;
    }
    pJpeReg->uReg08.tReg08.nRegJpeGBufYAdr1Low = (addr >> 8) & 0xFFFF;
    pJpeReg->uReg09.tReg09.nRegJpeGBufYAdr1High = addr >> (8 + 16);

    // C0
    if(ptInBufCfg->nInBufCAddr[0])
    {
        addr = Chip_Phys_to_MIU(ptInBufCfg->nInBufCAddr[0]);
    }
    else
    {
        addr = 0;
    }
    pJpeReg->uReg0a.tReg0a.nRegJpeGBufCAdr0Low = (addr >> 8) & 0xFFFF;
    pJpeReg->uReg0b.tReg0b.nRegJpeGBufCAdr0High = addr >> (8 + 16);

    // C1
    if(ptInBufCfg->nInBufCAddr[1])
    {
        addr = Chip_Phys_to_MIU(ptInBufCfg->nInBufCAddr[1]);
    }
    else
    {
        addr = 0;
    }
    pJpeReg->uReg0c.tReg0c.nRegJpeGBufCAdr1Low = (addr >> 8) & 0xFFFF;
    pJpeReg->uReg0d.tReg0d.nRegJpeGBufCAdr1High = addr >> (8 + 16);

    // Input buffers
    //
    REGW(nBaseAddr, 0x06, pJpeReg->uReg06.nReg06, "[%x] reg[%x] : Y0_low\n");
    REGW(nBaseAddr, 0x07, pJpeReg->uReg07.nReg07, "[%x] reg[%x] : Y0_high\n");
    REGW(nBaseAddr, 0x08, pJpeReg->uReg08.nReg08, "[%x] reg[%x] : Y1_low\n");
    REGW(nBaseAddr, 0x09, pJpeReg->uReg09.nReg09, "[%x] reg[%x] : Y1 high\n");
    REGW(nBaseAddr, 0x0a, pJpeReg->uReg0a.nReg0a, "[%x] reg[%x] : C0_low\n");
    REGW(nBaseAddr, 0x0b, pJpeReg->uReg0b.nReg0b, "[%x] reg[%x] : C0_high\n");
    REGW(nBaseAddr, 0x0c, pJpeReg->uReg0c.nReg0c, "[%x] reg[%x] : C1_low\n");
    REGW(nBaseAddr, 0x0d, pJpeReg->uReg0d.nReg0d, "[%x] reg[%x] : C1_high\n");
}

static void _Release(void* pIosIn)
{
    JpeIos_t* pIos = pIosIn;
    if(pIos)
    {
        CamOsMemRelease((void*)pIos);
    }
}

static JPE_IOC_RET_STATUS_e _SetBank(void* pIosIn, JpeRegIndex_t* pReg)
{
    int err = JPE_IOC_RET_SUCCESS;
    JpeIos_t* pIos = pIosIn;

    if((!pIos) || (!pReg))
        return JPE_IOC_RET_FAIL;

    switch(pReg->i_id)
    {
        case 0:
            pIos->nBaseAddr = (u32)pReg->base;
            //pIos->tJpeHalHandle.nBaseAddr = (u32)pReg->base;
            //pIos->tJpeHalHandle.nSize = pReg->size; // should be 0x100
            break;
        default:
            err = JPE_IOC_RET_FAIL;
            break;
    }
    return err;
}


static JPE_IOC_RET_STATUS_e _EncFire(void* pIosIn, void* pJpeHalHandleIn, JpeJob_t* pJob)
{
    JpeIos_t* pIos = (JpeIos_t*)pIosIn;
    JpeHalHandle_t *pJpeHalHandle = pJpeHalHandleIn;

    u32 nBaseAddr = pIos->nBaseAddr;

    JpeHalOutBufCfg_t *pJpeHalOutBufCfg = &pJpeHalHandle->tJpeHalOutBufCfg;
    u16 *pYQTable = pJpeHalHandle->YQTable;
    u16 *pCQTable = pJpeHalHandle->CQTable;
    JpeReg_t* pJpeReg = (JpeReg_t*)pJob;

    pJpeHalHandle->nBaseAddr = nBaseAddr;
    pJpeHalHandle->nSize = 0x100;

    pIos->eJpeDevStatus = JPE_DEV_BUSY;

    /* Set interrupt trigger mask, and only 3 bits are meaningful */
    pJpeReg->uReg1c.tReg1c.nRegJpeGIrqMask = _HalJpeIrqMaskConvertEnumerate2Real(pJpeReg, 0);
    REGW(nBaseAddr, 0x1c, pJpeReg->uReg1c.nReg1c, "[%x] reg[%x] : irq mask\n");


    /* Enable JPEG engine */
    pJpeReg->uReg00.tReg00.nRegJpeGFrameStartSw = 1;
    REGW(nBaseAddr, 0x00, pJpeReg->uReg00.nReg00, "[%x] reg[%x] : frame start\n");
    pJpeReg->uReg00.tReg00.nRegJpeGFrameStartSw = 0; // write-one-clear


    /* Set Clock */
#if defined(__I_SW__)
    REGW(REG_CKG_JPE_BASE, 0x00, pJpeReg->regClk, "Set Clk\n");
#endif

    /* Reset JPEG engine */
    // SW reset at first
    pJpeReg->uReg00.tReg00.nRegJpeGMode = 0;     // Assure JPE is running.
    pJpeReg->uReg00.tReg00.nRegJpeGSoftRstz = 0;
    REGW(nBaseAddr, 0x0, pJpeReg->uReg00.nReg00, "[%x] reg[%x] : SW reset 0\n");
    // delay for lower MCM frequence
    // Repeated write to replace udelay(1) becase 1 us is too long
    REGW(nBaseAddr, 0x0, pJpeReg->uReg00.nReg00, "[%x] reg[%x] : SW reset 0\n");
    REGW(nBaseAddr, 0x0, pJpeReg->uReg00.nReg00, "[%x] reg[%x] : SW reset 0\n");
    REGW(nBaseAddr, 0x0, pJpeReg->uReg00.nReg00, "[%x] reg[%x] : SW reset 0\n");
    REGW(nBaseAddr, 0x0, pJpeReg->uReg00.nReg00, "[%x] reg[%x] : SW reset 0\n");
    pJpeReg->uReg00.tReg00.nRegJpeGSoftRstz = 1;
    REGW(nBaseAddr, 0x0, pJpeReg->uReg00.nReg00, "[%x] reg[%x] : SW reset 1\n");


    // Sync all HW setting to jpeg register
#define _UPDATE_REG_VAR(nBaseAddr,reg_no) pJpeReg->uReg##reg_no.nReg##reg_no = REGR(nBaseAddr,0x##reg_no, "\n");//REGR(0x##reg_no, "\n");
    _UPDATE_REG_VAR(nBaseAddr, 00);
    _UPDATE_REG_VAR(nBaseAddr, 01);
    _UPDATE_REG_VAR(nBaseAddr, 02);
    _UPDATE_REG_VAR(nBaseAddr, 03);
    _UPDATE_REG_VAR(nBaseAddr, 06);
    _UPDATE_REG_VAR(nBaseAddr, 07);
    _UPDATE_REG_VAR(nBaseAddr, 08);
    _UPDATE_REG_VAR(nBaseAddr, 09);
    _UPDATE_REG_VAR(nBaseAddr, 0a);
    _UPDATE_REG_VAR(nBaseAddr, 0b);
    _UPDATE_REG_VAR(nBaseAddr, 0c);
    _UPDATE_REG_VAR(nBaseAddr, 0d);
    _UPDATE_REG_VAR(nBaseAddr, 0f);
    _UPDATE_REG_VAR(nBaseAddr, 10);
    _UPDATE_REG_VAR(nBaseAddr, 16);
    _UPDATE_REG_VAR(nBaseAddr, 18);
    _UPDATE_REG_VAR(nBaseAddr, 1c);
    _UPDATE_REG_VAR(nBaseAddr, 1d);
    _UPDATE_REG_VAR(nBaseAddr, 1e);
    _UPDATE_REG_VAR(nBaseAddr, 1f);
    _UPDATE_REG_VAR(nBaseAddr, 20);
    _UPDATE_REG_VAR(nBaseAddr, 21);
    _UPDATE_REG_VAR(nBaseAddr, 2c);
    _UPDATE_REG_VAR(nBaseAddr, 2d);
    _UPDATE_REG_VAR(nBaseAddr, 2e);
    _UPDATE_REG_VAR(nBaseAddr, 32);
    _UPDATE_REG_VAR(nBaseAddr, 33);
    _UPDATE_REG_VAR(nBaseAddr, 34);
    _UPDATE_REG_VAR(nBaseAddr, 35);
    _UPDATE_REG_VAR(nBaseAddr, 36);
    _UPDATE_REG_VAR(nBaseAddr, 3b);
    _UPDATE_REG_VAR(nBaseAddr, 3c);
    _UPDATE_REG_VAR(nBaseAddr, 3d);
    _UPDATE_REG_VAR(nBaseAddr, 3e);
    _UPDATE_REG_VAR(nBaseAddr, 42);
    _UPDATE_REG_VAR(nBaseAddr, 44);
    _UPDATE_REG_VAR(nBaseAddr, 45);
    _UPDATE_REG_VAR(nBaseAddr, 48);
    _UPDATE_REG_VAR(nBaseAddr, 49);
    _UPDATE_REG_VAR(nBaseAddr, 4a);
    _UPDATE_REG_VAR(nBaseAddr, 4b);
    _UPDATE_REG_VAR(nBaseAddr, 54);
    _UPDATE_REG_VAR(nBaseAddr, 55);
    _UPDATE_REG_VAR(nBaseAddr, 56);
    _UPDATE_REG_VAR(nBaseAddr, 57);
    _UPDATE_REG_VAR(nBaseAddr, 58);
    _UPDATE_REG_VAR(nBaseAddr, 59);
    _UPDATE_REG_VAR(nBaseAddr, 5a);
    _UPDATE_REG_VAR(nBaseAddr, 5b);
    _UPDATE_REG_VAR(nBaseAddr, 5c);
    _UPDATE_REG_VAR(nBaseAddr, 5d);
    _UPDATE_REG_VAR(nBaseAddr, 5e);
    _UPDATE_REG_VAR(nBaseAddr, 5f);
    _UPDATE_REG_VAR(nBaseAddr, 60);
    _UPDATE_REG_VAR(nBaseAddr, 61);
    _UPDATE_REG_VAR(nBaseAddr, 64);
    _UPDATE_REG_VAR(nBaseAddr, 6f);
    _UPDATE_REG_VAR(nBaseAddr, 70);
    _UPDATE_REG_VAR(nBaseAddr, 71);
    _UPDATE_REG_VAR(nBaseAddr, 72);
    _UPDATE_REG_VAR(nBaseAddr, 73);
    _UPDATE_REG_VAR(nBaseAddr, 74);
    _UPDATE_REG_VAR(nBaseAddr, 75);
    _UPDATE_REG_VAR(nBaseAddr, 76);
    _UPDATE_REG_VAR(nBaseAddr, 77);
    _UPDATE_REG_VAR(nBaseAddr, 78);
    _UPDATE_REG_VAR(nBaseAddr, 79);
    _UPDATE_REG_VAR(nBaseAddr, 7a);
    _UPDATE_REG_VAR(nBaseAddr, 7b);
    _UPDATE_REG_VAR(nBaseAddr, 7c);


    /* Config the operation mode of output buffer */
    pJpeReg->uReg3b.tReg3b.nRegJpeObufHwEn = JPE_HAL_OBUF_SINGLE_MODE;
    REGW(nBaseAddr, 0x3b, pJpeReg->uReg3b.nReg3b, "[%x] reg[%x] : output buffer mode\n");


    /* Config output address */
    _HalJpeSetOutBuf(nBaseAddr, pJpeReg, pJpeHalOutBufCfg);


    /* Clear IRQ */
    pJpeReg->uReg1d.nReg1d = REGR(nBaseAddr, 0x1d, "[%x] reg[%x] :IRQ register\n");
    pJpeReg->uReg1d.tReg1d.nRegJpeGIrqClr = _HalJpeIrqMaskConvertEnumerate2Real(pJpeReg, JPE_HAL_IRQ_ALL);
    REGW(nBaseAddr, 0x1d, pJpeReg->uReg1d.nReg1d, "[%x] reg[%x] : Clear IRQ\n");
    pJpeReg->uReg1d.nReg1d = REGR(nBaseAddr, 0x1d, "[%x] reg[%x] :IRQ register\n"); // HW is write-one-clear, so update again


    /* Set image dimension */
    //_HalJpeSetPicDim(pJpeHalHandle, pJpeHalHandle->nWidth, pJpeHalHandle->nHeight);
    pJpeReg->uReg01.tReg01.nRegJpeGPicWidth = pJpeHalHandle->nWidth; // (width+15)&~15;
    pJpeReg->uReg02.tReg02.nRegJpeGPicHeight = (u16)((pJpeHalHandle->nHeight + 7) & ~7);
    pJpeReg->uReg5e.tReg5e.nRegGJpeYPitch = (u16)((pJpeHalHandle->nWidth + 15) >> 4);
    pJpeReg->uReg5e.tReg5e.nRegGJpeCPitch = (u16)((pJpeHalHandle->nWidth + 15) >> 4);
    REGW(nBaseAddr, 0x01, pJpeReg->uReg01.nReg01, "[%x] reg[%x] : picture width\n");
    REGW(nBaseAddr, 0x02, pJpeReg->uReg02.nReg02, "[%x] reg[%x] : picture height\n");
    REGW(nBaseAddr, 0x5E, pJpeReg->uReg5e.nReg5e, "[%x] reg[%x] : picture pitch\n");

    /* Set codec mode */
    pJpeReg->uReg00.tReg00.nRegJpeGEncMode = 0;
    pJpeReg->uReg00.tReg00.nRegJpeGDctOnlyEn = 0;
    REGW(nBaseAddr, 0x00, pJpeReg->uReg00.nReg00, "[%x] reg[%x] : Codec mode\n");


    /* Config the RAW format for HW codec */
    switch(pJpeHalHandle->nRawFormat)
    {
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
    REGW(nBaseAddr, 0x00, pJpeReg->uReg00.nReg00, "[%x] reg[%x] : YUV format\n");
    REGW(nBaseAddr, 0x5F, pJpeReg->uReg5f.nReg5f, "[%x] reg[%x] : YUV format\n");


    /* Config the operation mode of input buffer */
    //HalJpeSetInputMode(pJpeHalHandle, pJpeHalHandle->nInBufMode);
    if(pJpeHalHandle->nInBufMode == JPE_HAL_BUFFER_ROW_MODE)
        pJpeReg->uReg18.tReg18.nRegJpeGJpeBuffermode = 0;
    else
        pJpeReg->uReg18.tReg18.nRegJpeGJpeBuffermode = 1;
    REGW(nBaseAddr, 0x18, pJpeReg->uReg18.nReg18, "[%x] reg[%x] : input buffer mode\n");


    /* Quantization table */
    if(_HalJpeSetQTable(nBaseAddr, pJpeReg, pYQTable, pCQTable, 64) != 0)
    {
        JPE_MSG(JPE_MSG_ERR, "HalJpeSetQTable fail, y_table=%p, c_table=%p\n", pYQTable, pCQTable);
        JPE_MSG(JPE_MSG_ERR, "pJpeHalHandle->nBaseAddr=0x%x, tJpeReg=%p\n", nBaseAddr, pJpeReg);
        return JPE_IOC_RET_BAD_QTABLE;
    }

    /* Config input address*/
    _HalJpeSetInBuf(pJpeHalHandle, &pJpeHalHandle->tJpeHalInBufCfg);


    /* Remvoe IRQ mask, and only 3 bits are meaningful */
    pJpeReg->uReg1c.tReg1c.nRegJpeGIrqMask = _HalJpeIrqMaskConvertEnumerate2Real(pJpeReg, 0);
    REGW(nBaseAddr, 0x1c, pJpeReg->uReg1c.nReg1c, "[%x] reg[%x] : irq mask\n");


    /* Enable HW */
    pJpeReg->uReg00.tReg00.nRegJpeGFrameStartSw = 1;
    REGW(nBaseAddr, 0x00, pJpeReg->uReg00.nReg00, "[%x] reg[%x] : frame start\n");
    pJpeReg->uReg00.tReg00.nRegJpeGFrameStartSw = 0; // write-one-clear

    JPE_MSG(JPE_MSG_DEBUG, "[out] %s\n", __func__);

    return JPE_IOC_RET_SUCCESS;
}


static JPE_IOC_RET_STATUS_e _IsrFunc(void* pIosIn, int msk)
{
    JpeIos_t* pIos = (JpeIos_t*)pIosIn;

    u32 nBaseAddr = pIos->nBaseAddr;
    JpeReg_t* pJpeReg = &pIos->tJpeReg;

    JPE_MSG(JPE_MSG_DEBUG, "Interrupt: 0x%x\n", _HalJpeGetIrq(nBaseAddr));

    /* JPE_IRQ_BSPOBUF_FULL */
    if(_HalJpeGetIrqCheck(nBaseAddr, pJpeReg, JPE_HAL_IRQ_BSPOBUF0_FULL))
    {
        /* Status update */
        pIos->nEncodeSize = _HalJpeGetOutputByteSize(nBaseAddr, pJpeReg, 0);
        pIos->eJpeDevStatus = JPE_DEV_OUTBUF_FULL;

        /* Clear IRQ */
        _HalJpeClearIrq(nBaseAddr, pJpeReg, JPE_HAL_IRQ_BSPOBUF0_FULL);
    }

    /* JPE_IRQ_BSPOBUF1_FULL */
    else if(_HalJpeGetIrqCheck(nBaseAddr, pJpeReg, JPE_HAL_IRQ_BSPOBUF1_FULL))
    {
        /* Status update */
        pIos->nEncodeSize = _HalJpeGetOutputByteSize(nBaseAddr, pJpeReg, 1);
        pIos->eJpeDevStatus = JPE_DEV_OUTBUF_FULL;

        /* Clear IRQ */
        _HalJpeClearIrq(nBaseAddr, pJpeReg, JPE_HAL_IRQ_BSPOBUF1_FULL);
    }

    /* JPE_IRQ_FRAME_DONE */
    else if(_HalJpeGetIrqCheck(nBaseAddr, pJpeReg, JPE_HAL_IRQ_FRAME_DONE))
    {
        /* Status update */
        pIos->nEncodeSize = _HalJpeGetOutputByteSize(nBaseAddr, pJpeReg, 0);
        pIos->eJpeDevStatus = JPE_DEV_ENC_DONE;

        /* Clear IRQ */
        _HalJpeClearIrq(nBaseAddr, pJpeReg, JPE_HAL_IRQ_FRAME_DONE);
    }
    else
    {
        u16 nIrq;
        nIrq = _HalJpeGetIrq(nBaseAddr);
        JPE_MSG(JPE_MSG_ERR, "Unexpect Interrupt: 0x%x  size:%d ejpeDevStatus:%d\n", nIrq, pIos->nEncodeSize, pIos->eJpeDevStatus);
        _HalJpeClearIrq(nBaseAddr, pJpeReg, JPE_HAL_IRQ_FRAME_DONE);
    }

    JPE_MSG(JPE_MSG_DEBUG, "Interrupt: %X  size:%d ejpeDevStatus:%d\n", _HalJpeGetIrq(nBaseAddr), pIos->nEncodeSize, pIos->eJpeDevStatus);

    return JPE_IOC_RET_SUCCESS;
}


static JPE_IOC_RET_STATUS_e _IrqMask(void* pIosIn, int nMask)
{
    JpeIos_t* pIos = (JpeIos_t*)pIosIn;

    u32 nBaseAddr = pIos->nBaseAddr;
    JpeReg_t* pJpeReg = &pIos->tJpeReg;

    _HalJpeClearIrq(nBaseAddr, pJpeReg, nMask);
    _HalJpeSetIrqMask(nBaseAddr, pJpeReg, nMask);

    return JPE_IOC_RET_SUCCESS;
}


JpeIosCB_t* JpeIosAcquire(char* tags)
{
    JpeIos_t* pIos = NULL;
    if(!tags)
        return NULL;

    pIos = ((JpeIos_t *)CamOsMemCalloc(1, sizeof(JpeIos_t)));
    if(pIos)
    {
        JpeIosCB_t* mios = &pIos->mios;
        mios->release = _Release;
        mios->setBank = _SetBank;
        mios->encFire = _EncFire;
        mios->isrFunc = _IsrFunc;
        mios->irqMask = _IrqMask;

        // TODO: remove tJpeReg
        memset(&pIos->tJpeReg, 0, sizeof(JpeReg_t));
    }
    return (JpeIosCB_t*)pIos;
}
