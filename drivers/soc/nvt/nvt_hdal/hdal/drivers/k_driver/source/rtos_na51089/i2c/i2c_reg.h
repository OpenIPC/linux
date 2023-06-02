/*
    Register offset and bit definition for I2C module

    Register offset and bit definition for I2C module.

    @file       i2c_reg.h
    @ingroup    mIDrvIO_I2C
    @note       Nothing

    Copyright   Novatek Microelectronics Corp. 2012.  All rights reserved.
*/

#ifndef _I2C_REG_H
#define _I2C_REG_H

#ifndef __KERNEL__
#include "rcw_macro.h"
#include "kwrap/type.h"
#else
#include "mach/rcw_macro.h"
#include "kwrap/type.h"
#endif

/*
    @addtogroup mIDrvIO_I2C
*/
//@{

// I2C Control Register
REGDEF_OFFSET(I2C_CTRL_REG, 0x00)
REGDEF_BEGIN(I2C_CTRL_REG)              // --> Register "I2C_CTRL_REG" begin ---
REGDEF_BIT(I2C_En, 1)
REGDEF_BIT(SCL_En, 1)
REGDEF_BIT(GC_En, 1)
REGDEF_BIT(TB_En, 1)
REGDEF_BIT(DT_IntEn, 1)
REGDEF_BIT(DR_IntEn, 1)
REGDEF_BIT(BERR_IntEn, 1)
REGDEF_BIT(STOP_IntEn, 1)
REGDEF_BIT(SAM_IntEn, 1)
REGDEF_BIT(GC_IntEn, 1)
REGDEF_BIT(AL_IntEn, 1)
REGDEF_BIT(DMAED_IntEn, 1)
REGDEF_BIT(SCLTimeout_IntEn, 1)
REGDEF_BIT(, 2)
REGDEF_BIT(DESCERR_IntEn, 1)
REGDEF_BIT(BC_En, 1)
REGDEF_BIT(, 3)
REGDEF_BIT(DMA_DESC, 1)
REGDEF_BIT(, 3)
//REGDEF_BIT(VD_SRC, 1)
//REGDEF_BIT(, 3)
REGDEF_BIT(VD_SRC, 3) // 520 : 1bit, 528 : 3bits
REGDEF_BIT(, 1) // 520 : 3bits, 528 : 1bit
REGDEF_BIT(VD_SYNC, 1)
REGDEF_BIT(DMA_RCWRITE, 1)
REGDEF_BIT(DMA_Dir, 1)
REGDEF_BIT(DMA_En, 1)
REGDEF_END(I2C_CTRL_REG)                // --- Register "I2C_CTRL_REG" end   <--

// I2C Status Register
REGDEF_OFFSET(I2C_STS_REG, 0x04)
REGDEF_BEGIN(I2C_STS_REG)               // --> Register "I2C_STS_REG" begin ---
REGDEF_BIT(RW, 1)
REGDEF_BIT(NACK, 1)
REGDEF_BIT(Busy, 1)
REGDEF_BIT(BusBusy, 1)
REGDEF_BIT(DT, 1)
REGDEF_BIT(DR, 1)
REGDEF_BIT(BERR, 1)
REGDEF_BIT(STOP, 1)
REGDEF_BIT(SAM, 1)
REGDEF_BIT(GC, 1)
REGDEF_BIT(AL, 1)
REGDEF_BIT(DMAED, 1)
REGDEF_BIT(SCLTimeout, 1)
REGDEF_BIT(, 2)
REGDEF_BIT(DESCERR, 1)
REGDEF_BIT(, 12)
REGDEF_BIT(BusFree, 1)
//REGDEF_BIT(, 3)
REGDEF_BIT(SCL, 1)
REGDEF_BIT(SCLERR, 1)
REGDEF_BIT(, 1)
REGDEF_END(I2C_STS_REG)                 // --- Register "I2C_STS_REG" end   <--

// I2C Configuration Register
REGDEF_OFFSET(I2C_CONFIG_REG, 0x08)
REGDEF_BEGIN(I2C_CONFIG_REG)            // --> Register "I2C_CONFIG_REG" begin ---
REGDEF_BIT(AccessMode, 1)
REGDEF_BIT(, 3)
REGDEF_BIT(NACK_Gen0, 1)
REGDEF_BIT(NACK_Gen1, 1)
REGDEF_BIT(NACK_Gen2, 1)
REGDEF_BIT(NACK_Gen3, 1)
REGDEF_BIT(Start_Gen0, 1)
REGDEF_BIT(Start_Gen1, 1)
REGDEF_BIT(Start_Gen2, 1)
REGDEF_BIT(Start_Gen3, 1)
REGDEF_BIT(Stop_Gen0, 1)
REGDEF_BIT(Stop_Gen1, 1)
REGDEF_BIT(Stop_Gen2, 1)
REGDEF_BIT(Stop_Gen3, 1)
REGDEF_BIT(PIODataSize, 3)
REGDEF_BIT(, 13)
REGDEF_END(I2C_CONFIG_REG)              // --- Register "I2C_CONFIG_REG" end   <--

// I2C Bus Clock Register
REGDEF_OFFSET(I2C_BUSCLK_REG, 0x0C)
REGDEF_BEGIN(I2C_BUSCLK_REG)            // --> Register "I2C_BUSCLK_REG" begin ---
REGDEF_BIT(LowCounter, 10)
REGDEF_BIT(, 6)
REGDEF_BIT(HighCounter, 10)
REGDEF_BIT(, 6)
REGDEF_END(I2C_BUSCLK_REG)              // --- Register "I2C_BUSCLK_REG" end   <--

// I2C Data Register
REGDEF_OFFSET(I2C_DATA_REG, 0x10)
REGDEF_BEGIN(I2C_DATA_REG)              // --> Register "I2C_DATA_REG" begin ---
REGDEF_BIT(DR0, 8)
REGDEF_BIT(DR1, 8)
REGDEF_BIT(DR2, 8)
REGDEF_BIT(DR3, 8)
REGDEF_END(I2C_DATA_REG)                // --- Register "I2C_DATA_REG" end   <--

// I2C Slave Address Register
REGDEF_OFFSET(I2C_SAR_REG, 0x14)
REGDEF_BEGIN(I2C_SAR_REG)               // --> Register "I2C_SAR_REG" begin ---
REGDEF_BIT(SAR, 10)
REGDEF_BIT(, 21)
REGDEF_BIT(TenBits, 1)
REGDEF_END(I2C_SAR_REG)                 // --- Register "I2C_SAR_REG" end   <--

// I2C Set / Hold Time and Glitch Suppression Setting Register
REGDEF_OFFSET(I2C_TIMING_REG, 0x18)
REGDEF_BEGIN(I2C_TIMING_REG)            // --> Register "I2C_TIMING_REG" begin ---
REGDEF_BIT(TSR, 10)
REGDEF_BIT(, 2)
REGDEF_BIT(GSR, 4)
REGDEF_BIT(SCLTimeout, 10)
REGDEF_BIT(, 6)
REGDEF_END(I2C_TIMING_REG)              // --- Register "I2C_TIMING_REG" end   <--

// I2C Bus Monitor Register
REGDEF_OFFSET(I2C_BUS_REG, 0x1C)
REGDEF_BEGIN(I2C_BUS_REG)               // --> Register "I2C_BUS_REG" begin ---
REGDEF_BIT(SDA, 1)
REGDEF_BIT(SCL, 1)
REGDEF_BIT(, 30)
REGDEF_END(I2C_BUS_REG)                 // --- Register "I2C_BUS_REG" end   <--

// I2C DMA Transfer Size Register
REGDEF_OFFSET(I2C_DMASIZE_REG, 0x20)
REGDEF_BEGIN(I2C_DMASIZE_REG)           // --> Register "I2C_DMASIZE_REG" begin ---
REGDEF_BIT(DMA_BufSize, 16)
REGDEF_BIT(DMA_TranCount, 16)
REGDEF_END(I2C_DMASIZE_REG)             // --- Register "I2C_DMASIZE_REG" end   <--

// I2C DMA Starting Address Register
REGDEF_OFFSET(I2C_DMAADDR_REG, 0x24)
REGDEF_BEGIN(I2C_DMAADDR_REG)           // --> Register "I2C_DMAADDR_REG" begin ---
REGDEF_BIT(DMA_StartAddr, 32)
REGDEF_END(I2C_DMAADDR_REG)             // --- Register "I2C_DMAADDR_REG" end   <--

// I2C Bus Free Register
REGDEF_OFFSET(I2C_BUSFREE_REG, 0x28)
REGDEF_BEGIN(I2C_BUSFREE_REG)           // --> Register "I2C_BUSFREE_REG" begin ---
REGDEF_BIT(BusFree, 20)
REGDEF_BIT(, 12)
REGDEF_END(I2C_BUSFREE_REG)             // --- Register "I2C_BUSFREE_REG" end   <--

// I2C DMA VD Group Size Register
REGDEF_OFFSET(I2C_DMA_VD_SIZE_REG, 0x2C)
REGDEF_BEGIN(I2C_DMA_VD_SIZE_REG)       // --> Register "I2C_DMA_VD_SIZE_REG" begin ---
REGDEF_BIT(DMA_TRANS_SIZE2, 16)
REGDEF_BIT(DMA_TRANS_SIZE3, 16)
REGDEF_END(I2C_DMA_VD_SIZE_REG)         // --- Register "I2C_DMA_VD_SIZE_REG" end   <--

// I2C DMA VD Control Register
REGDEF_OFFSET(I2C_VD_CONTROL_REG, 0x34)
REGDEF_BEGIN(I2C_VD_CONTROL_REG)        // --> Register "I2C_VD_CONTROL_REG" begin ---
REGDEF_BIT(DMA_TRANS_DB1, 8)
REGDEF_BIT(DMA_TRANS_DB2, 8)
REGDEF_BIT(DMA_TRANS_DB3, 8)
REGDEF_BIT(DMA_VD_NUMBER, 2)
REGDEF_BIT(, 6)
REGDEF_END(I2C_VD_CONTROL_REG)          // --- Register "I2C_VD_CONTROL_REG" end   <--

// I2C DMA VD Group Delay Interval Register
REGDEF_OFFSET(I2C_VD_DELAY_REG, 0x38)
REGDEF_BEGIN(I2C_VD_DELAY_REG)          // --> Register "I2C_VD_DELAY_REG" begin ---
REGDEF_BIT(DMA_VD_DELAY, 24)
REGDEF_BIT(DMA_VD_INTVAL, 4)
REGDEF_BIT(DMA_VD_INTVAL2, 4)
REGDEF_END(I2C_VD_DELAY_REG)            // --- Register "I2C_VD_DELAY_REG" end   <--

// I2C DMA DESC info Register
REGDEF_OFFSET(DMA_DESC_INFO_REG, 0x3C)
REGDEF_BEGIN(DMA_DESC_INFO_REG)         // --> Register "DMA_DESC_INFO_REG" begin ---
REGDEF_BIT(DMA_DESC_COUNTER, 16)
REGDEF_BIT(DMA_POLL_DATA1, 8)
REGDEF_BIT(DMA_POLL_DATA2, 8)
REGDEF_END(DMA_DESC_INFO_REG)           // --- Register "DMA_DESC_INFO_REG" end   <--

//@}

#endif
