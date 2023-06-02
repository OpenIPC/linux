/*
    Register definition header file for timer module.

    This file is the header file that define the address offset and bit
    definition of registers of timer module.

    @file       timer_reg.h
    @ingroup    miDrvTimer_Timer
    @note       Nothing.

    Copyright   Novatek Microelectronics Corp. 2018.  All rights reserved.
*/

#ifndef _TIMER_REG_H
#define _TIMER_REG_H

#if defined __UITRON || defined __ECOS
#include "DrvCommon.h"
#else
#include "mach/rcw_macro.h"
#include "uitron_wrapper/type.h"
#include "timer_platform.h"
#endif


// Timer register offset definitions
#define TIMER_0_CONTROL_REG_OFS     0x10
#define TIMER_0_TARGET_REG_OFS      0x14
#define TIMER_0_COUNTER_REG_OFS     0x18

// Timer Destination Register 0
REGDEF_OFFSET(TIMER_DST0_REG, 0x00)
REGDEF_BEGIN(TIMER_DST0_REG)            // --> Register "TIMER_DST0_REG" begin ---
REGDEF_BIT(TimerToCPU1, 20)
REGDEF_BIT(, 12)
REGDEF_END(TIMER_DST0_REG)              // --- Register "TIMER_DST0_REG" end   <--

// Timer Destination Register 1
REGDEF_OFFSET(TIMER_DST1_REG, 0x04)
REGDEF_BEGIN(TIMER_DST1_REG)            // --> Register "TIMER_DST1_REG" begin ---
REGDEF_BIT(TimerToCPU2, 20)
REGDEF_BIT(, 12)
REGDEF_END(TIMER_DST1_REG)              // --- Register "TIMER_DST1_REG" end   <--

// Timer Destination Register 2
REGDEF_OFFSET(TIMER_DST2_REG, 0x08)
REGDEF_BEGIN(TIMER_DST2_REG)            // --> Register "TIMER_DST2_REG" begin ---
REGDEF_BIT(TimerToDSP, 20)
REGDEF_BIT(, 12)
REGDEF_END(TIMER_DST2_REG)              // --- Register "TIMER_DST2_REG" end   <--

// Timer Destination Register 3
REGDEF_OFFSET(TIMER_DST3_REG, 0x0C)
REGDEF_BEGIN(TIMER_DST3_REG)            // --> Register "TIMER_DST3_REG" begin ---
REGDEF_BIT(TimerToDSP2, 20)
REGDEF_BIT(, 12)
REGDEF_END(TIMER_DST3_REG)              // --- Register "TIMER_DST3_REG" end   <--

#if (_EMULATION_ == ENABLE)
#if (_EMULATION_ON_CPU2_ == ENABLE)
// Timer Status Register 0
REGDEF_OFFSET(TIMER_STS0_REG, 0x14)
REGDEF_BEGIN(TIMER_STS0_REG)            // --> Register "TIMER_STS0_REG" begin ---
REGDEF_BIT(TimerStsCPU1, 20)
REGDEF_BIT(, 12)
REGDEF_END(TIMER_STS0_REG)              // --- Register "TIMER_STS0_REG" end   <--
#else
// Timer Status Register 0
REGDEF_OFFSET(TIMER_STS0_REG, 0x10)
REGDEF_BEGIN(TIMER_STS0_REG)            // --> Register "TIMER_STS0_REG" begin ---
REGDEF_BIT(TimerStsCPU1, 20)
REGDEF_BIT(, 12)
REGDEF_END(TIMER_STS0_REG)              // --- Register "TIMER_STS0_REG" end   <--

#endif
#else
// Timer Status Register 0
REGDEF_OFFSET(TIMER_STS0_REG, 0x10)
REGDEF_BEGIN(TIMER_STS0_REG)            // --> Register "TIMER_STS0_REG" begin ---
REGDEF_BIT(TimerStsCPU1, 20)
REGDEF_BIT(, 12)
REGDEF_END(TIMER_STS0_REG)              // --- Register "TIMER_STS0_REG" end   <--

#endif


// Timer Status Register 1
REGDEF_OFFSET(TIMER_STS1_REG, 0x14)
REGDEF_BEGIN(TIMER_STS1_REG)            // --> Register "TIMER_STS1_REG" begin ---
REGDEF_BIT(TimerStsCPU2, 20)
REGDEF_BIT(, 12)
REGDEF_END(TIMER_STS1_REG)              // --- Register "TIMER_STS1_REG" end   <--

// Timer Status Register 2
REGDEF_OFFSET(TIMER_STS2_REG, 0x18)
REGDEF_BEGIN(TIMER_STS2_REG)            // --> Register "TIMER_STS2_REG" begin ---
REGDEF_BIT(TimerStsDSP, 20)
REGDEF_BIT(, 12)
REGDEF_END(TIMER_STS2_REG)              // --- Register "TIMER_STS2_REG" end   <--

// Timer Status Register 2
REGDEF_OFFSET(TIMER_STS3_REG, 0x1C)
REGDEF_BEGIN(TIMER_STS3_REG)            // --> Register "TIMER_STS3_REG" begin ---
REGDEF_BIT(TimerStsDSP2, 20)
REGDEF_BIT(, 12)
REGDEF_END(TIMER_STS3_REG)              // --- Register "TIMER_STS3_REG" end   <--

// Timer Interrupt Register 0
#if (_EMULATION_ == ENABLE)
#if (_EMULATION_ON_CPU2_ == ENABLE)
REGDEF_OFFSET(TIMER_INT0_REG, 0x24)
REGDEF_BEGIN(TIMER_INT0_REG)            // --> Register "TIMER_INT0_REG" begin ---
REGDEF_BIT(TimerINTCPU1, 20)
REGDEF_BIT(, 12)
REGDEF_END(TIMER_INT0_REG)              // --- Register "TIMER_INT0_REG" end   <--
#else
REGDEF_OFFSET(TIMER_INT0_REG, 0x20)
REGDEF_BEGIN(TIMER_INT0_REG)            // --> Register "TIMER_INT0_REG" begin ---
REGDEF_BIT(TimerINTCPU1, 20)
REGDEF_BIT(, 12)
REGDEF_END(TIMER_INT0_REG)              // --- Register "TIMER_INT0_REG" end   <--
#endif
#else
REGDEF_OFFSET(TIMER_INT0_REG, 0x20)
REGDEF_BEGIN(TIMER_INT0_REG)            // --> Register "TIMER_INT0_REG" begin ---
REGDEF_BIT(TimerINTCPU1, 20)
REGDEF_BIT(, 12)
REGDEF_END(TIMER_INT0_REG)              // --- Register "TIMER_INT0_REG" end   <--
#endif
// Timer Interrupt Register 1
REGDEF_OFFSET(TIMER_INT1_REG, 0x24)
REGDEF_BEGIN(TIMER_INT1_REG)            // --> Register "TIMER_INT1_REG" begin ---
REGDEF_BIT(TimerINTCPU2, 20)
REGDEF_BIT(, 12)
REGDEF_END(TIMER_INT1_REG)              // --- Register "TIMER_INT1_REG" end   <--

// Timer Interrupt Register 2
REGDEF_OFFSET(TIMER_INT2_REG, 0x28)
REGDEF_BEGIN(TIMER_INT2_REG)            // --> Register "TIMER_INT2_REG" begin ---
REGDEF_BIT(TimerINTDSP, 20)
REGDEF_BIT(, 12)
REGDEF_END(TIMER_INT2_REG)              // --- Register "TIMER_INT2_REG" end   <--

// Timer Interrupt Register 3
REGDEF_OFFSET(TIMER_INT3_REG, 0x2C)
REGDEF_BEGIN(TIMER_INT3_REG)            // --> Register "TIMER_INT3_REG" begin ---
REGDEF_BIT(TimerINTDSP2, 20)
REGDEF_BIT(, 12)
REGDEF_END(TIMER_INT3_REG)              // --- Register "TIMER_INT3_REG" end   <--

// Timer Clock Divider Register
REGDEF_OFFSET(TIMER_CLKDIV_REG, 0x30)
REGDEF_BEGIN(TIMER_CLKDIV_REG)          // --> Register "TIMER_CLKDIV_REG" begin ---
REGDEF_BIT(Divider0, 8)
REGDEF_BIT(Divider1, 8)
REGDEF_BIT(Divider0En, 1)
REGDEF_BIT(Divider1En, 1)
REGDEF_BIT(, 14)
REGDEF_END(TIMER_CLKDIV_REG)            // --- Register "TIMER_CLKDIV_REG" end   <--

// Reserved (0x34 ~ 0xFC)

// Timer Control Register
REGDEF_OFFSET(TIMER_CTRLx_REG, 0x100)
REGDEF_BEGIN(TIMER_CTRLx_REG)           // --> Register "TIMER_CTRLx_REG" begin ---
REGDEF_BIT(Enable, 1)
REGDEF_BIT(Mode, 1)
REGDEF_BIT(SrcClk, 1)
REGDEF_BIT(, 29)
REGDEF_END(TIMER_CTRLx_REG)             // --- Register "TIMER_CTRLx_REG" end   <--

// Timer Target Register
REGDEF_OFFSET(TIMER_TARGETx_REG, 0x104)
REGDEF_BEGIN(TIMER_TARGETx_REG)         // --> Register "TIMER_TARGETx_REG" begin ---
REGDEF_BIT(Target, 32)
REGDEF_END(TIMER_TARGETx_REG)           // --- Register "TIMER_TARGETx_REG" end   <--

// Timer Counter Register
REGDEF_OFFSET(TIMER_COUNTERx_REG, 0x108)
REGDEF_BEGIN(TIMER_COUNTERx_REG)        // --> Register "TIMER_COUNTERx_REG" begin ---
REGDEF_BIT(Counter, 32)
REGDEF_END(TIMER_COUNTERx_REG)          // --- Register "TIMER_COUNTERx_REG" end   <--

// Timer Reload Register
REGDEF_OFFSET(TIMER_RELOADx_REG, 0x10C)
REGDEF_BEGIN(TIMER_RELOADx_REG)        // --> Register "TIMER_RELOADx_REG" begin ---
REGDEF_BIT(RELOAD, 1)
REGDEF_BIT(, 31)
REGDEF_END(TIMER_RELOADx_REG)          // --- Register "TIMER_RELOADx_REG" end   <--

#endif
