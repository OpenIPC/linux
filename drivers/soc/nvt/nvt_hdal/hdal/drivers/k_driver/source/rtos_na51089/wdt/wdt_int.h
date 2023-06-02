/*
    Internal header file for WDT module.

    This file is the internal header file that defines the variables, structures
    and function prototypes of WDT module.

    @file       wdt_int.h
    @ingroup    miDrvTimer_WDT
    @note       Nothing.

    Copyright   Novatek Microelectronics Corp. 2011.  All rights reserved.
*/

#ifndef _WDT_INT_H
#define _WDT_INT_H

//#include "DrvCommon.h"
#include "kwrap/type.h"
#include <kwrap/nvt_type.h>

/**
    @addtogroup miDrvTimer_WDT
*/
//@{

// Key to write WDT Control Register
#define WDT_KEY_VALUE           0x5A96

// Maximum MSB value
#define WDT_MSB_MAX             0xFF

// WDT source clock
// 12 MHz / 1024
#define WDT_SOURCE_CLOCK        (12000000 >> 10)

// Register default value
#define WDT_CTRL_REG_DEFAULT          0x00000F00
#define WDT_STS_REG_DEFAULT           0x0000FFFF
#define WDT_TRIG_REG_DEFAULT          0x00000000
#define WDT_MANUAL_RESET_REG_DEFAULT  0x00000000

/*
    SIF register default value

    SIF register default value.
*/
typedef struct {
	UINT32  uiOffset;
	UINT32  uiValue;
	CHAR    *pName;
} WDT_REG_DEFAULT;

//@}

#endif
