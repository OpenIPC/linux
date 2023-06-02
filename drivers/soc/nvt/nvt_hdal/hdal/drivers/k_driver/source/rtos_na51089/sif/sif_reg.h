/*
    Register offset and bit definition for SIF module

    Register offset and bit definition for SIF module.

    @file       sif_reg.h
    @ingroup    mIDrvIO_SIF
    @note       Nothing

    Copyright   Novatek Microelectronics Corp. 2011.  All rights reserved.
*/

#ifndef _SIF_REG_H
#define _SIF_REG_H


#if defined(_BSP_NA51089_)
#include "rcw_macro.h"
#include "kwrap/type.h"//a header for basic variable type
#endif

/*
    @addtogroup mIDrvIO_SIF
*/
//@{

// SIF Mode Register
REGDEF_OFFSET(SIF_MODE_REG, 0x00)
REGDEF_BEGIN(SIF_MODE_REG)              // --> Register "SIF_MODE_REG" begin ---
REGDEF_BIT(Mode_Ch0,  4)
REGDEF_BIT(Mode_Ch1,  4)
REGDEF_BIT(Mode_Ch2,  4)
REGDEF_BIT(Mode_Ch3,  4)
REGDEF_BIT(, 16)
REGDEF_END(SIF_MODE_REG)                // --- Register "SIF_MODE_REG" end   <--

// SIF Config Register 0
REGDEF_OFFSET(SIF_CONF0_REG, 0x04)
REGDEF_BEGIN(SIF_CONF0_REG)             // --> Register "SIF_CONF0_REG" begin ---
REGDEF_BIT(Start_Ch0,  1)
REGDEF_BIT(Start_Ch1,  1)
REGDEF_BIT(Start_Ch2,  1)
REGDEF_BIT(Start_Ch3,  1)
REGDEF_BIT(, 12)
REGDEF_BIT(Restart_Ch0,  1)
REGDEF_BIT(Restart_Ch1,  1)
REGDEF_BIT(Restart_Ch2,  1)
REGDEF_BIT(Restart_Ch3,  1)
REGDEF_BIT(, 12)
REGDEF_END(SIF_CONF0_REG)               // --- Register "SIF_CONF0_REG" end   <--

// SIF Config Register 1
REGDEF_OFFSET(SIF_CONF1_REG, 0x08)
REGDEF_BEGIN(SIF_CONF1_REG)             // --> Register "SIF_CONF1_REG" begin ---
REGDEF_BIT(Dir_Ch0,  1)
REGDEF_BIT(Dir_Ch1,  1)
REGDEF_BIT(Dir_Ch2,  1)
REGDEF_BIT(Dir_Ch3,  1)
REGDEF_BIT(Cont_Ch0,  1)
REGDEF_BIT(Cont_Ch1,  1)
REGDEF_BIT(, 26)
REGDEF_END(SIF_CONF1_REG)               // --- Register "SIF_CONF1_REG" end   <--

// SIF Config Register 2
REGDEF_OFFSET(SIF_CONF2_REG, 0x0C)
REGDEF_BEGIN(SIF_CONF2_REG)             // --> Register "SIF_CONF2_REG" begin ---
REGDEF_BIT(Ch0_Addr_Bits,  8)
REGDEF_BIT(Ch1_Addr_Bits,  8)
REGDEF_BIT(, 16)
REGDEF_END(SIF_CONF2_REG)               // --- Register "SIF_CONF2_REG" end   <--

// SIF Clock Control Register 0
REGDEF_OFFSET(SIF_CLKCTRL0_REG, 0x10)
REGDEF_BEGIN(SIF_CLKCTRL0_REG)          // --> Register "SIF_CLKCTRL0_REG" begin ---
REGDEF_BIT(ClkDiv_Ch0, 8)
REGDEF_BIT(ClkDiv_Ch1, 8)
REGDEF_BIT(ClkDiv_Ch2, 8)
REGDEF_BIT(ClkDiv_Ch3, 8)
REGDEF_END(SIF_CLKCTRL0_REG)            // --- Register "SIF_CLKCTRL0_REG" end   <--

// Reserved (0x14)
// Reserved (0x18)

// SIF Transmit Size Register 0
REGDEF_OFFSET(SIF_TXSIZE0_REG, 0x1C)
REGDEF_BEGIN(SIF_TXSIZE0_REG)           // --> Register "SIF_TXSIZE0_REG" begin ---
REGDEF_BIT(TxSize_Ch0, 8)
REGDEF_BIT(TxSize_Ch1, 8)
REGDEF_BIT(TxSize_Ch2, 8)
REGDEF_BIT(TxSize_Ch3, 8)
REGDEF_END(SIF_TXSIZE0_REG)             // --- Register "SIF_TXSIZE0_REG" end   <--

// Reserved (0x20~0x28)

// SIF Status Register
REGDEF_OFFSET(SIF_STS_REG, 0x2C)
REGDEF_BEGIN(SIF_STS_REG)               // --> Register "SIF_INTSTS_REG" begin ---
REGDEF_BIT(Status_Ch0, 1)
REGDEF_BIT(Status_Ch1, 1)
REGDEF_BIT(Status_Ch2, 1)
REGDEF_BIT(Status_Ch3, 1)
REGDEF_BIT(, 28)
REGDEF_END(SIF_STS_REG)                 // --- Register "SIF_INTSTS_REG" end   <--

// SIF Data Register 0
REGDEF_OFFSET(SIF_DATA0_REG, 0x30)
REGDEF_BEGIN(SIF_DATA0_REG)             // --> Register "SIF_DATA0_REG" begin ---
REGDEF_BIT(DR0, 32)
REGDEF_END(SIF_DATA0_REG)               // --- Register "SIF_DATA0_REG" end   <--

// SIF Data Register 1
REGDEF_OFFSET(SIF_DATA1_REG, 0x34)
REGDEF_BEGIN(SIF_DATA1_REG)             // --> Register "SIF_DATA1_REG" begin ---
REGDEF_BIT(DR1, 32)
REGDEF_END(SIF_DATA1_REG)               // --- Register "SIF_DATA1_REG" end   <--

// SIF Data Register 2
REGDEF_OFFSET(SIF_DATA2_REG, 0x38)
REGDEF_BEGIN(SIF_DATA2_REG)             // --> Register "SIF_DATA2_REG" begin ---
REGDEF_BIT(DR2, 32)
REGDEF_END(SIF_DATA2_REG)               // --- Register "SIF_DATA2_REG" end   <--

// SIF Data Port Select Register
REGDEF_OFFSET(SIF_DATASEL_REG, 0x3C)
REGDEF_BEGIN(SIF_DATASEL_REG)           // --> Register "SIF_DATASEL_REG" begin ---
REGDEF_BIT(DP_Sel,  2)
REGDEF_BIT(, 30)
REGDEF_END(SIF_DATASEL_REG)             // --- Register "SIF_DATASEL_REG" end   <--

// SIF Delay Register 0
REGDEF_OFFSET(SIF_DELAY0_REG, 0x40)
REGDEF_BEGIN(SIF_DELAY0_REG)            // --> Register "SIF_DELAY0_REG" begin ---
REGDEF_BIT(Delay_Ch0, 8)
REGDEF_BIT(Delay_Ch1, 8)
REGDEF_BIT(Delay_Ch2, 8)
REGDEF_BIT(Delay_Ch3, 8)
REGDEF_END(SIF_DELAY0_REG)              // --- Register "SIF_DELAY0_REG" end   <--

// Reserved (0x44 ~ 0x4C)

// SIF Timing Config Register 0
REGDEF_OFFSET(SIF_TIMING0_REG, 0x50)
REGDEF_BEGIN(SIF_TIMING0_REG)           // --> Register "SIF_TIMING0_REG" begin ---
REGDEF_BIT(SEND_Ch0, 4)
REGDEF_BIT(SENH_Ch0, 3)
REGDEF_BIT(, 1)
REGDEF_BIT(SENS_Ch0, 4)
REGDEF_BIT(, 4)
REGDEF_BIT(SEND_Ch1, 4)
REGDEF_BIT(SENH_Ch1, 3)
REGDEF_BIT(, 1)
REGDEF_BIT(SENS_Ch1, 4)
REGDEF_BIT(, 4)
REGDEF_END(SIF_TIMING0_REG)             // --- Register "SIF_TIMING0_REG" end   <--

// SIF Timing Config Register 1
REGDEF_OFFSET(SIF_TIMING1_REG, 0x54)
REGDEF_BEGIN(SIF_TIMING1_REG)           // --> Register "SIF_TIMING1_REG" begin ---
REGDEF_BIT(SEND_Ch2, 4)
REGDEF_BIT(SENH_Ch2, 3)
REGDEF_BIT(, 1)
REGDEF_BIT(SENS_Ch2, 4)
REGDEF_BIT(, 4)
REGDEF_BIT(SEND_Ch3, 4)
REGDEF_BIT(SENH_Ch3, 3)
REGDEF_BIT(, 1)
REGDEF_BIT(SENS_Ch3, 4)
REGDEF_BIT(, 4)
REGDEF_END(SIF_TIMING1_REG)             // --- Register "SIF_TIMING1_REG" end   <--

// Reserved (0x58 ~ 0x5C)

// SIF DMA0 Control Register 0
REGDEF_OFFSET(SIF_DMA0_CTRL0_REG, 0x60)
REGDEF_BEGIN(SIF_DMA0_CTRL0_REG)        // --> Register "SIF_DMA0_CTRL0_REG" begin ---
REGDEF_BIT(DMA0_En, 1)
REGDEF_BIT(, 3)
REGDEF_BIT(DMA0_Sync_En, 1)
REGDEF_BIT(, 3)
REGDEF_BIT(DMA0_Sync_Src, 3)
REGDEF_BIT(, 1)
REGDEF_BIT(DMA0_Burst_N, 2)
REGDEF_BIT(, 6)
REGDEF_BIT(DMA0_Burst_IntVal1, 3)
REGDEF_BIT(, 1)
REGDEF_BIT(DMA0_Burst_IntVal2, 3)
REGDEF_BIT(, 1)
REGDEF_BIT(DMA0_Delay, 1)
REGDEF_BIT(, 3)
REGDEF_END(SIF_DMA0_CTRL0_REG)          // --- Register "SIF_DMA0_CTRL0_REG" end   <--

// SIF DMA0 Control Register 1
REGDEF_OFFSET(SIF_DMA0_CTRL1_REG, 0x64)
REGDEF_BEGIN(SIF_DMA0_CTRL1_REG)        // --> Register "SIF_DMA0_CTRL1_REG" begin ---
REGDEF_BIT(DMA0_BufSize1, 16)
REGDEF_BIT(DMA0_BufSize2, 16)
REGDEF_END(SIF_DMA0_CTRL1_REG)          // --- Register "SIF_DMA0_CTRL1_REG" end   <--

// SIF DMA0 Control Register 2
REGDEF_OFFSET(SIF_DMA0_CTRL2_REG, 0x68)
REGDEF_BEGIN(SIF_DMA0_CTRL2_REG)        // --> Register "SIF_DMA0_CTRL2_REG" begin ---
REGDEF_BIT(DMA0_BufSize3, 16)
REGDEF_BIT(, 16)
REGDEF_END(SIF_DMA0_CTRL2_REG)          // --- Register "SIF_DMA0_CTRL2_REG" end   <--

// SIF DMA0 Control Register 3
REGDEF_OFFSET(SIF_DMA0_CTRL3_REG, 0x6C)
REGDEF_BEGIN(SIF_DMA0_CTRL3_REG)        // --> Register "SIF_DMA0_CTRL3_REG" begin ---
REGDEF_BIT(DMA_Burst_Delay, 24)
REGDEF_BIT(, 8)
REGDEF_END(SIF_DMA0_CTRL3_REG)          // --- Register "SIF_DMA0_CTRL3_REG" end   <--

// SIF DMA0 Start Address Register
REGDEF_OFFSET(SIF_DMA0_STARTADDR_REG, 0x70)
REGDEF_BEGIN(SIF_DMA0_STARTADDR_REG)     // --> Register "SIF_DMA0_STARTADDR_REG" begin ---
REGDEF_BIT(DMA_StartAddr, 32)
REGDEF_END(SIF_DMA0_STARTADDR_REG)       // --- Register "SIF_DMA0_STARTADDR_REG" end   <--

// SIF DMA1 delay tag Register
REGDEF_OFFSET(SIF_DMA0_DLY_REG, 0x74)
REGDEF_BEGIN(SIF_DMA0_DLY_REG)           // --> Register "SIF_DMA0_DLY_REG" begin ---
REGDEF_BIT(SIF_DLY_TAG,  16)
REGDEF_BIT(, 16)
REGDEF_END(SIF_DMA0_DLY_REG)             // --- Register "SIF_DMA0_DLY_REG" end   <--

// Reserved (0x78~0x7C)

// SIF DMA1 Control Register 0
REGDEF_OFFSET(SIF_DMA1_CTRL0_REG, 0x80)
REGDEF_BEGIN(SIF_DMA1_CTRL0_REG)        // --> Register "SIF_DMA1_CTRL0_REG" begin ---
REGDEF_BIT(DMA1_En, 1)
REGDEF_BIT(, 3)
REGDEF_BIT(DMA1_Sync_En, 1)
REGDEF_BIT(, 3)
REGDEF_BIT(DMA1_Sync_Src, 3)
REGDEF_BIT(, 1)
REGDEF_BIT(DMA1_Burst_N, 2)
REGDEF_BIT(, 6)
REGDEF_BIT(DMA1_Burst_IntVal1, 3)
REGDEF_BIT(, 1)
REGDEF_BIT(DMA1_Burst_IntVal2, 3)
REGDEF_BIT(, 1)
REGDEF_BIT(DMA1_Delay, 1)
REGDEF_BIT(, 3)
REGDEF_END(SIF_DMA1_CTRL0_REG)          // --- Register "SIF_DMA1_CTRL0_REG" end   <--

// SIF DMA1 Control Register 1
REGDEF_OFFSET(SIF_DMA1_CTRL1_REG, 0x84)
REGDEF_BEGIN(SIF_DMA1_CTRL1_REG)        // --> Register "SIF_DMA1_CTRL1_REG" begin ---
REGDEF_BIT(DMA1_BufSize1, 16)
REGDEF_BIT(DMA1_BufSize2, 16)
REGDEF_END(SIF_DMA1_CTRL1_REG)          // --- Register "SIF_DMA1_CTRL1_REG" end   <--

// SIF DMA1 Control Register 2
REGDEF_OFFSET(SIF_DMA1_CTRL2_REG, 0x88)
REGDEF_BEGIN(SIF_DMA1_CTRL2_REG)        // --> Register "SIF_DMA2_CTRL1_REG" begin ---
REGDEF_BIT(DMA1_BufSize3, 16)
REGDEF_BIT(, 16)
REGDEF_END(SIF_DMA1_CTRL2_REG)          // --- Register "SIF_DMA2_CTRL1_REG" end   <--

// SIF DMA1 Control Register 3
REGDEF_OFFSET(SIF_DMA1_CTRL3_REG, 0x8C)
REGDEF_BEGIN(SIF_DMA1_CTRL3_REG)        // --> Register "SIF_DMA1_CTRL3_REG" begin ---
REGDEF_BIT(DMA_Burst_Delay, 24)
REGDEF_BIT(, 8)
REGDEF_END(SIF_DMA1_CTRL3_REG)          // --- Register "SIF_DMA1_CTRL3_REG" end   <--

// SIF DMA1 Start Address Register
REGDEF_OFFSET(SIF_DMA1_STARTADDR_REG, 0x90)
REGDEF_BEGIN(SIF_DMA1_STARTADDR_REG)     // --> Register "SIF_DMA1_STARTADDR_REG" begin ---
REGDEF_BIT(DMA_StartAddr, 32)
REGDEF_END(SIF_DMA1_STARTADDR_REG)       // --- Register "SIF_DMA1_STARTADDR_REG" end   <--


// SIF DMA1 delay tag Register
REGDEF_OFFSET(SIF_DMA1_DLY_REG, 0x94)
REGDEF_BEGIN(SIF_DMA1_DLY_REG)           // --> Register "SIF_DMA1_DLY_REG" begin ---
REGDEF_BIT(SIF_DLY_TAG,  16)
REGDEF_BIT(, 16)
REGDEF_END(SIF_DMA1_DLY_REG)             // --- Register "SIF_DMA1_DLY_REG" end   <--

// SIF Interrupt Enable Register 0
REGDEF_OFFSET(SIF_INTEN0_REG, 0x9C)
REGDEF_BEGIN(SIF_INTEN0_REG)             // --> Register "SIF_INTEN0_REG" begin ---
REGDEF_BIT(Data_Empty_Ch0_IntEn,  1)
REGDEF_BIT(Data_Empty_Ch1_IntEn,  1)
REGDEF_BIT(Data_Empty_Ch2_IntEn,  1)
REGDEF_BIT(Data_Empty_Ch3_IntEn,  1)
REGDEF_BIT(TxEnd_Ch0_IntEn,  1)
REGDEF_BIT(TxEnd_Ch1_IntEn,  1)
REGDEF_BIT(TxEnd_Ch2_IntEn,  1)
REGDEF_BIT(TxEnd_Ch3_IntEn,  1)
REGDEF_BIT(, 16)
REGDEF_BIT(DMAEnd_DMA0_IntEn,  1)
REGDEF_BIT(DMAEnd_DMA1_IntEn,  1)
REGDEF_BIT(,  6)
REGDEF_END(SIF_INTEN0_REG)               // --- Register "SIF_INTEN0_REG" end   <--

// SIF Interrupt Desitenation Register
REGDEF_OFFSET(SIF_INT_DES_REG, 0x98)
REGDEF_BEGIN(SIF_INT_DES_REG)             // --> Register "SIF_INT_DES_REG" begin ---
REGDEF_BIT(INT_TO_CPU1,  4)
REGDEF_BIT(,12)
REGDEF_BIT(INT_TO_CPU2,  4)
REGDEF_BIT(,12)
REGDEF_END(SIF_INT_DES_REG)               // --- Register "SIF_INT_DES_REG" end   <--


// SIF Interrupt Enable Register 1
REGDEF_OFFSET(SIF_INTEN1_REG, 0xA0)
REGDEF_BEGIN(SIF_INTEN1_REG)             // --> Register "SIF_INTEN1_REG" begin ---
REGDEF_BIT(Data_Empty_Ch0_IntEn_core2,  1)
REGDEF_BIT(Data_Empty_Ch1_IntEn_core2,  1)
REGDEF_BIT(Data_Empty_Ch2_IntEn_core2,  1)
REGDEF_BIT(Data_Empty_Ch3_IntEn_core2,  1)
REGDEF_BIT(TxEnd_Ch0_IntEn_core2,  1)
REGDEF_BIT(TxEnd_Ch1_IntEn_core2,  1)
REGDEF_BIT(TxEnd_Ch2_IntEn_core2,  1)
REGDEF_BIT(TxEnd_Ch3_IntEn_core2,  1)
REGDEF_BIT(, 16)
REGDEF_BIT(DMAEnd_DMA0_IntEn_core2,  1)
REGDEF_BIT(DMAEnd_DMA1_IntEn_core2,  1)
REGDEF_BIT(,  6)
REGDEF_END(SIF_INTEN1_REG)               // --- Register "SIF_INTEN1_REG" end   <--


// SIF Interrupt Status Register 0
REGDEF_OFFSET(SIF_INTSTS0_REG, 0xA4)
REGDEF_BEGIN(SIF_INTSTS0_REG)            // --> Register "SIF_INTSTS0_REG" begin ---
REGDEF_BIT(Data_Empty_Ch0,  1)
REGDEF_BIT(Data_Empty_Ch1,  1)
REGDEF_BIT(Data_Empty_Ch2,  1)
REGDEF_BIT(Data_Empty_Ch3,  1)
REGDEF_BIT(TxEnd_Ch0,  1)
REGDEF_BIT(TxEnd_Ch1,  1)
REGDEF_BIT(TxEnd_Ch2,  1)
REGDEF_BIT(TxEnd_Ch3,  1)
REGDEF_BIT(, 16)
REGDEF_BIT(DMAEnd_DMA0,  1)
REGDEF_BIT(DMAEnd_DMA1,  1)
REGDEF_BIT(,  6)
REGDEF_END(SIF_INTSTS0_REG)              // --- Register "SIF_INTSTS0_REG" end   <--

// SIF Interrupt Status Register 1
REGDEF_OFFSET(SIF_INTSTS1_REG, 0xA8)
REGDEF_BEGIN(SIF_INTSTS1_REG)            // --> Register "SIF_INTSTS1_REG" begin ---
REGDEF_BIT(Data_Empty_Ch0_core2,  1)
REGDEF_BIT(Data_Empty_Ch1_core2,  1)
REGDEF_BIT(Data_Empty_Ch2_core2,  1)
REGDEF_BIT(Data_Empty_Ch3_core2,  1)
REGDEF_BIT(TxEnd_Ch0_core2,  1)
REGDEF_BIT(TxEnd_Ch1_core2,  1)
REGDEF_BIT(TxEnd_Ch2_core2,  1)
REGDEF_BIT(TxEnd_Ch3_core2,  1)
REGDEF_BIT(, 16)
REGDEF_BIT(DMAEnd_DMA0_core2,  1)
REGDEF_BIT(DMAEnd_DMA1_core2,  1)
REGDEF_BIT(,  6)
REGDEF_END(SIF_INTSTS1_REG)              // --- Register "SIF_INTSTS1_REG" end   <--


//@}

#endif
