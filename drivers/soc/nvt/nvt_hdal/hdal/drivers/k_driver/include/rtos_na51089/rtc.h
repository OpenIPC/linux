/**
    Header file for RTC module.

    This file is the header file that define the API and data type
    for RTC module.

    @file       rtc.h
    @ingroup    miDrvTimer_RTC
    @note       Nothing.

    Copyright   Novatek Microelectronics Corp. 2012.  All rights reserved.
*/

#ifndef _RTC_H
#define _RTC_H

#include <kwrap/type.h>
#include <kwrap/nvt_type.h>
#include <kwrap/platform.h>
#include <comm/driver.h>
//#include "driver.h"

/**
    @addtogroup miDrvTimer_RTC
*/
//@{

/**
    RTC Date
*/
typedef union {
	// Format conformed to RTC register
	_PACKED_BEGIN struct {
		UBITFIELD   day: 5;     ///< Day
		UBITFIELD   month: 4;   ///< Month
		UBITFIELD   year: 12;   ///< Year
	} _PACKED_END s;
	UINT32      value;          ///< Date value all together Y:M:D
} RTC_DATE, *PRTC_DATE;

/**
    RTC Time
*/
typedef union {
	// Format conformed to RTC register
	_PACKED_BEGIN struct {
		UBITFIELD   second: 6;  ///< Second
		UBITFIELD   minute: 6;  ///< Minute
		UBITFIELD   hour: 5;    ///< Hour
	} _PACKED_END s;
	UINT32      value;          ///< Time value all together H:M:S
} RTC_TIME, *PRTC_TIME;

/**
    RTC power on source

    @note For rtc_getPwrOnSource()
*/
typedef enum {
	RTC_PWRON_SRC_PWR_SW,       ///< power on from power switch
	RTC_PWRON_SRC_PWR_SW2,      ///< power on from power switch 2
	RTC_PWRON_SRC_PWR_SW3,      ///< power on from power switch 3 (Usually for USB VBUS, depend on project)
	RTC_PWRON_SRC_PWR_SW4,      ///< power on from power switch 4 (Usually for VBAT, depend on project)
	RTC_PWRON_SRC_PWR_ALM,      ///< Power on from PWR ALARM
	ENUM_DUMMY4WORD(RTC_PWRON_SRC)
} RTC_PWRON_SRC, *PRTC_PWRON_SRC;

#define RTC_PWRON_SRC_VBAT  RTC_PWRON_SRC_PWR_SW4
#define RTC_PWRON_SRC_VBUS  RTC_PWRON_SRC_PWR_SW3

/**
    RTC wake up source

    RTC wake up source for RTC_CONFIG_ID_WAKEUP_SRC.
*/
typedef enum {
	RTC_WAKEUP_SRC_ALARM    = 0x00000001,   ///< RTC time matches alarm time, default is disabled
	RTC_WAKEUP_SRC_PWR_SW   = 0x00000002,   ///< Power switch   is pressed, default is disabled
	RTC_WAKEUP_SRC_PWR_SW2  = 0x00000004,   ///< Power switch 2 is pressed, default is disabled

	ENUM_DUMMY4WORD(RTC_WAKEUP_SRC)
} RTC_WAKEUP_SRC, *PRTC_WAKEUP_SRC;


/**
    RTC Configuration ID

    RTC Configuration ID for rtc_setConfig() and rtc_getConfig().

*/
typedef enum {
	RTC_CONFIG_ID_WAKEUP_ENABLE,    ///< Enable wake up source when IC enter power down mode (Default is none).
	///< Could be ORed of the following value:
	///< - @b RTC_WAKEUP_SRC_ALARM  : RTC time matches alarm time
	///< - @b RTC_WAKEUP_SRC_PWR_SW : Power switch   is pressed
	///< - @b RTC_WAKEUP_SRC_PWR_SW2: Power switch 2 is pressed

	RTC_CONFIG_ID_WAKEUP_DISABLE,   ///< Disable wake up source when IC enter power down mode.
	///< Could be ORed of the following value:
	///< - @b RTC_WAKEUP_SRC_ALARM  : RTC time matches alarm time
	///< - @b RTC_WAKEUP_SRC_PWR_SW : Power switch   is pressed
	///< - @b RTC_WAKEUP_SRC_PWR_SW2: Power switch 2 is pressed


	ENUM_DUMMY4WORD(RTC_CONFIG_ID)
} RTC_CONFIG_ID, *PRTC_CONFIG_ID;

/**
    RTC DATA selection

    Select accessed RTC data. Each selection is 16 bits

    @note For rtc_setData() and rtc_getData().
*/
typedef enum {
	RTC_DATA_0,                     ///< Select data 0

	ENUM_DUMMY4WORD(RTC_DATA)
} RTC_DATA;

/**
    RTC DRAM power off policy

    @note For rtc_set_dram_power_off_policy()
*/
typedef enum {
	RTC_DRAM_POWEROFF_POLICY_NORMAL,        ///< Normal. DRAM will be powered off if RTC PWR_EN becomes LOW
	RTC_DRAM_POWEROFF_POLICY_KEEP,          ///< Normal. DRAM will be stilled powered on if RTC PWR_EN becomes LOW

	ENUM_DUMMY4WORD(RTC_DRAM_POWEROFF_POLICY)
} RTC_DRAM_POWEROFF_POLICY;

// Public APIs

// General
extern ER               rtc_open(void);
extern BOOL             rtc_isOpened(void);
extern void             rtc_reset(void);
extern void             rtc_triggerCSET(void);
extern void             rtc_waitCSETDone(void);
extern BOOL             rtc_isPowerLost(void);

extern void             rtc_setConfig(RTC_CONFIG_ID ConfigID, UINT32 uiConfig);
extern UINT32           rtc_getConfig(RTC_CONFIG_ID ConfigID);

// Date, time
extern void             rtc_setBaseDate(UINT32 uiYear, UINT32 uiMonth, UINT32 uiDay);
extern RTC_DATE         rtc_getBaseDate(void);

extern void             rtc_setMaxDate(UINT32 uiYear, UINT32 uiMonth, UINT32 uiDay);
extern RTC_DATE         rtc_getMaxDate(void);

extern void             rtc_setReadTimeOffset(INT32 iHour, INT32 iMinute);

extern ER               rtc_setDate(UINT32 uiYear, UINT32 uiMonth, UINT32 uiDay);
extern RTC_DATE         rtc_getDate(void);

extern ER               rtc_setTime(UINT32 uiHour, UINT32 uiMinute, UINT32 uiSecond);
extern RTC_TIME         rtc_getTime(void);

extern UINT32           rtc_setData(RTC_DATA select, UINT32 data);
extern UINT32           rtc_getData(RTC_DATA select);

extern UINT32           rtc_set_pwr_en3(BOOL value);
extern BOOL             rtc_get_pwr_en3(void);

extern UINT32           rtc_convertDate2Days(RTC_DATE rtcDate);
extern RTC_DATE         rtc_convertDays2Date(UINT32 uiDays);
extern BOOL             rtc_isLeapYear(UINT32 uiYear);

// Alarm
extern ER               rtc_setAlarm(RTC_DATE rtcDate, RTC_TIME rtcTime, DRV_CB EventHandler);
extern void             rtc_getAlarm(PRTC_DATE prtcDate, PRTC_TIME prtcTime);

// Power alarm
extern ER               rtc_setPWRAlarm(RTC_DATE rtcDate, RTC_TIME rtcTime);
extern void             rtc_getPWRAlarm(PRTC_DATE prtcDate, PRTC_TIME prtcTime);
extern void             rtc_disablePWRAlarm(void);
extern void             rtc_enablePWRAlarm(void);
extern BOOL             rtc_isPWRAlarmEnabled(void);
extern BOOL             rtc_isPWRAlarm(void);

// Power button control
extern void             rtc_resetShutdownTimer(void);
extern void             rtc_poweroffPWR(void);
extern void             rtc_set_dram_power_off_policy(RTC_DRAM_POWEROFF_POLICY policy);
extern RTC_DRAM_POWEROFF_POLICY rtc_get_dram_power_off_policy(void);

extern BOOL             rtc_getPWRStatus(void);
extern BOOL             rtc_getPWR2Status(void);
extern BOOL             rtc_getPWR3Status(void);
extern BOOL             rtc_getPWR4Status(void);
extern BOOL             rtc_getPWREN2Status(void);
extern BOOL             rtc_getPWREN2Log(void);
extern RTC_PWRON_SRC    rtc_getPWROnSource(void);

// Driver probe
extern void             rtc_platform_init(void);
extern void             rtc_platform_uninit(void);

//@}

#endif
