/*
	Header file for RTC controller register

	This file is the header file that define register for RTC module

	@file       rtc_reg.h
	Copyright   Novatek Microelectronics Corp. 2018.  All rights reserved
	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License version 2 as
	published by the Free Software Foundation.
*/

#ifndef _RTC_REG_H
#define _RTC_REG_H

#include <mach/rcw_macro.h>

/*
	@addtogroup mIHALRTC
*/

/*RTC Timer Register*/
#define RTC_TIMER_REG_OFS 0x00
union RTC_TIMER_REG {
	uint32_t reg;
	struct {
	unsigned int sec:6;
	unsigned int min:6;
	unsigned int hour:5;
	unsigned int reserved0:15;
	} bit;
};

/*RTC Day & Key Register*/
#define RTC_DAYKEY_REG_OFS 0x04
union RTC_DAYKEY_REG {
	uint32_t reg;
	struct {
	unsigned int day:16;
	unsigned int key:4;
	unsigned int reserved0:12;
	} bit;
};

/*RTC Alarm Register*/
#define RTC_ALARM_REG_OFS 0x08
union RTC_ALARM_REG {
	uint32_t reg;
	struct {
	unsigned int sec:6;
	unsigned int min:6;
	unsigned int hour:5;
	unsigned int reserved0:15;
	} bit;
};

/*RTC Status Register*/
#define RTC_STATUS_REG_OFS 0x0C
union RTC_STATUS_REG {
	uint32_t reg;
	struct {
	unsigned int alarm_sts:1;
	unsigned int srst_sts:1;
	unsigned int ready:1;
	unsigned int reserved0:5;
	unsigned int cset_sts:1;
	unsigned int reserved1:23;
	} bit;
};

/*RTC Control Register*/
#define RTC_CTRL_REG_OFS 0x10
union RTC_CTRL_REG {
	uint32_t reg;
	struct {
	unsigned int alarm_inten:1;
	unsigned int srst:1;
	unsigned int cset:1;
	unsigned int time_sel:1;
	unsigned int day_sel:1;
	unsigned int key_sel:1;
	unsigned int pwralarmtime_sel:1;
	unsigned int pwralarmday_sel:1;
	unsigned int cset_inten:1;
	unsigned int reserved0:3;
	unsigned int data0_sel:1;
	unsigned int reserved1:19;
	} bit;
};

// RTC Data Register
#define RTC_DATA_REG_OFS 0x14
union RTC_DATA_REG {
	uint32_t reg;
	struct {
	unsigned int data0:8;
	unsigned int reserved:24;
	} bit;
};

// RTC Power Button Control Register
#define RTC_PWBC_REG_OFS 0x18
union RTC_PWBC_REG {
	uint32_t reg;
	struct {
	unsigned int reset_sdt_timer:1;
	unsigned int pwr_off:1;
	unsigned int reserved0:1;
	unsigned int pwralarm_en:1;
	unsigned int pwralarm_dis:1;
	unsigned int reserved1:19;
	unsigned int pwr_sw1_inten:1;
	unsigned int pwr_sw2_inten:1;
	unsigned int reserved2:6;
	} bit;
};

// RTC Power Button Control Status Register
#define RTC_PWBCSTS_REG_OFS 0x1C
union RTC_PWBCSTS_REG {
	uint32_t reg;
	struct {
	unsigned int pwralarm_en_sts:1;
	unsigned int corepwr_valid:1;
	unsigned int reserved0:6;
	unsigned int pwronsrc_sw1:1;
	unsigned int pwronsrc_sw2:1;
	unsigned int pwronsrc_sw3:1;
	unsigned int pwronsrc_sw4:1;
	unsigned int reserved1:3;
	unsigned int pwronsrc_pwralarm:1;
	unsigned int pwr_sw1_pin:1;
	unsigned int pwr_sw2_pin:1;
	unsigned int pwr_sw3_pin:1;
	unsigned int pwr_sw4_pin:1;
	unsigned int reserved2:4;
	unsigned int pwr_sw1_sts:1;
	unsigned int pwr_sw2_sts:1;
	unsigned int reserved3:6;
	} bit;
};


/*RTC Power Alarm Timer Register*/
#define RTC_PWRALM_REG_OFS 0x20
union RTC_PWRALM_REG {
	uint32_t reg;
	struct {
	unsigned int sec:6;
	unsigned int min:6;
	unsigned int hour:5;
	unsigned int reserved0:3;
	unsigned int day:5;
	unsigned int reserved1:7;
	} bit;
};

/*RTC OSC Analog Register*/
#define RTC_OSCAN_REG_OFS 0x24
union RTC_OSCAN_REG {
	uint32_t reg;
	struct {
	unsigned int osc_analogcfg:8;
	unsigned int reserved0:24;
	} bit;
};
#endif
