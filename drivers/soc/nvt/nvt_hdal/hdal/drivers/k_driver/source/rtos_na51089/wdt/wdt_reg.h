/*
    Register offset and bit definition for WDT module

    Register offset and bit definition for WDT module.

    @file       wdt_reg.h
    @ingroup    miDrvTimer_WDT
    @note       Nothing.

    Copyright   Novatek Microelectronics Corp. 2011.  All rights reserved.
*/

#ifndef _WDT_REG_H
#define _WDT_REG_H

#include <rcw_macro.h>

/*
    @addtogroup miDrvTimer_WDT
*/
//@{

// WDT Control Register
REGDEF_OFFSET(WDT_CTRL_REG, 0x00)
REGDEF_BEGIN(WDT_CTRL_REG)                  // --> Register "WDT_CONTROL_REG" begin ---
REGDEF_BIT(Enable, 1)
REGDEF_BIT(Mode, 1)
REGDEF_BIT(, 2)
REGDEF_BIT(external_reset, 1)
REGDEF_BIT(reset_num_en, 1)
REGDEF_BIT(reset_num1_en, 1)
REGDEF_BIT(, 1)
REGDEF_BIT(MSB, 8)
REGDEF_BIT(Key_Ctrl, 16)
REGDEF_END(WDT_CTRL_REG)                    // --- Register "WDT_CONTROL_REG" end   <--

// WDT Status and Counter Register
REGDEF_OFFSET(WDT_STS_REG, 0x04)
REGDEF_BEGIN(WDT_STS_REG)                   // --> Register "WDT_STS_REG" begin ---
REGDEF_BIT(Cnt, 20)
REGDEF_BIT(, 10)
REGDEF_BIT(En_Status, 1)
REGDEF_BIT(Status, 1)
REGDEF_END(WDT_STS_REG)                     // --- Register "WDT_STS_REG" end   <--

// WDT Trigger Register
REGDEF_OFFSET(WDT_TRIG_REG, 0x08)
REGDEF_BEGIN(WDT_TRIG_REG)                  // --> Register "WDT_TRIG_REG" begin ---
REGDEF_BIT(Trigger, 1)
REGDEF_BIT(, 31)
REGDEF_END(WDT_TRIG_REG)                    // --- Register "WDT_TRIG_REG" end   <--

// WDT Manual Reset Register
REGDEF_OFFSET(WDT_MANUAL_RESET_REG, 0x0C)
REGDEF_BEGIN(WDT_MANUAL_RESET_REG)          // --> Register "WDT_MANUAL_RESET_REG" begin ---
REGDEF_BIT(Manual_Reset, 1)
REGDEF_BIT(, 31)
REGDEF_END(WDT_MANUAL_RESET_REG)            // --- Register "WDT_MANUAL_RESET_REG" end   <--

// Reserved (0x10 ~ 0x7C)

// WDT Timeout Record Register
REGDEF_OFFSET(WDT_REC_REG, 0x80)
REGDEF_BEGIN(WDT_REC_REG)                   // --> Register "WDT_REC_REG" begin ---
REGDEF_BIT(Reset_Num, 8)
REGDEF_BIT(Reset_Num1, 8)
REGDEF_BIT(, 16)
REGDEF_END(WDT_REC_REG)                     // --- Register "WDT_REC_REG" end   <--

// WDT User Data Register
REGDEF_OFFSET(WDT_UDATA_REG, 0x84)
REGDEF_BEGIN(WDT_UDATA_REG)                 // --> Register "WDT_UDATA_REG" begin ---
REGDEF_BIT(User_Data, 32)
REGDEF_END(WDT_UDATA_REG)                   // --- Register "WDT_UDATA_REG" end   <--

//@}

#endif
