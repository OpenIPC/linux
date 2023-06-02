/*
	Internal header file for RTC

	This file is the internal header file that defines the variables,
	structures and function prototypes of RTC module.

	@file       rtc_int.h

	Copyright   Novatek Microelectronics Corp. 2018.  All rights reserved
	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License version 2 as
	published by the Free Software Foundation.
*/

#ifndef _RTC_INT_H
#define _RTC_INT_H

/*INT = internal*/
#define RTC_INT_KEY	0xA
#define RTC_INT_KEY_POR	0x1

/*Register default value*/
#define RTC_TIMER_REG_DEFAULT	0x00000000
#define RTC_DAYKEY_REG_DEFAULT	0x00000000
#define RTC_ALARM_REG_DEFAULT	0x0001FFFF
#define RTC_STATUS_REG_DEFAULT	0x00000004	/*RTC_READY is 1*/
#define RTC_CTRL_REG_DEFAULT	0x00000000

#define RTC_INT_MAX_PWRALARM_DAY    0x1F
#define RTC_INT_PRWALARM_DAY_SHIFT  5

#define DRV_INT_RTC	56

/*OSC analog parameter*/
#define RTC_INT_OSD_ANALOG_ORIGINAL 0x0	/*Original*/
#define RTC_INT_OSD_ANALOG_INCREASE 0x1	/*Increase driving switch*/
#define RTC_INT_OSD_ANALOG_SWING5   0x2	/*Regular out swing 0.5*AVDD_RTC*/
#define RTC_INT_OSD_ANALOG_SWING66  0x6	/*Regular out swing 0.66*AVDD_RTC*/
#define RTC_INT_OSD_ANALOG_ENHANCE  0xF	/*Enhance comparator driving*/
#define RTC_INT_OSC_ANALOG_CFG      RTC_INT_OSD_ANALOG_ENHANCE

void nvt_rtc_power_control(int reboot_sec);
#endif
