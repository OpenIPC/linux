/**
    Header file for WDT module.

    This file is the header file that define the API and data type
    for WDT module.

    @file       wdt.h
    @ingroup    miDrvTimer_WDT
    @note       Nothing.

    Copyright   Novatek Microelectronics Corp. 2011.  All rights reserved.
*/

#ifndef _WDT_H
#define _WDT_H

#include <kwrap/nvt_type.h>
#include "../comm/driver.h"

/**
    @addtogroup miDrvTimer_WDT
*/
//@{

/**
    WDT mode

    WDT mode.
*/
typedef enum {
	WDT_MODE_INT,               ///< Issue interrupt when WDT expired
	WDT_MODE_RESET,             ///< Reset system when WDT expired

	ENUM_DUMMY4WORD(WDT_MODE)
} WDT_MODE;

/**
    WDT external reset

    WDT external reset
*/
typedef enum {
	WDT_NORMAL_MODE,            ///< follow mode configuration
	WDT_EXT_RESET,              ///< enable LVD reset

	ENUM_DUMMY4WORD(WDT_EXT_MODE)
} WDT_EXT_MODE;

/**
    WDT manual reset

    WDT manual reset
*/
typedef enum {
	WDT_NORMAL_OP_MODE,            ///< follow mode configuration
	WDT_MANUAL_RESET,           ///< enable manual reset

	ENUM_DUMMY4WORD(WDT_MANUAL_MODE)
} WDT_MANUAL_MODE;


/**
    WDT reset number enable

    WDT reset number enable
*/
typedef enum {
	WDT_RST_NUM_DISABLE,       ///< Reset number will not increase after wdt reset
	WDT_RST_NUM_ENABLE,        ///< Reset number will increase after wdt reset

	ENUM_DUMMY4WORD(WDT_RST_NUM)
} WDT_RST_NUM;

/**
    WDT configuration ID

    WDT configuration ID for wdt_setConfig() and wdt_getConfig().
*/
typedef enum {
	WDT_CONFIG_ID_MODE,         ///< WDT expired mode
	///< - @b WDT_MODE_INT      : Issue interrupt (Default value)
	///< - @b WDT_MODE_RESET    : Reset system

	WDT_CONFIG_ID_TIMEOUT,      ///< WDT expired time
	///< @note  Valid value: 349 ~ 89,478 , unit: ms, resolution about: 350 ms. Default 349 ms
	///<        Actual expired time might longer than your setting.
	///<        Please call wdt_getConfig(WDT_CONFIG_ID_TIMEOUT) to confirm.

	WDT_CONFIG_ID_USERDATA,     ///< WDT user data, it's 32 bits data that will be kept until system HW reset.

	WDT_CONFIG_ID_EXT_RESET,    ///< WDT external reset
	///< - @b Diable    : Follow mode configuration
	///< - @b ENable    : Enable LVD reset, ignore mode configuration

	WDT_CONFIG_ID_MANUAL_RESET, ///< WDT manual reset
	///< - @b Diable    : Follow mode configuration
	///< - @b ENable    : Watchdog reset immediately, ignore mode configuration

	WDT_CONFIG_ID_RST_NUM,      ///< WDT reset number 1
	///< - @b Diable    : Reset number will not increase after wdt reset
	///< - @b ENable    : Reset number will increase after wdt reset

	ENUM_DUMMY4WORD(WDT_CONFIG_ID)
} WDT_CONFIG_ID;

extern ER       wdt_open(void);
extern ER       wdt_close(void);

extern void     wdt_setConfig(WDT_CONFIG_ID ConfigID, UINT32 uiConfig);
extern UINT32   wdt_getConfig(WDT_CONFIG_ID ConfigID);

extern void     wdt_enable(void);
extern void     wdt_disable(void);

extern void     wdt_trigger(void);
extern void     wdt_waitTimeout(void);
extern void     wdt_clearTimeout(void);

extern UINT32   wdt_getResetNum(void);
extern UINT32   wdt_getResetNum1(void);

extern void     wdt_external_reset(void);
//@}

#endif
