/*
	Header file for DRTC controller register

	This file is the header file that define register for DRTC module

	@file       drtc_reg.h
	Copyright   Novatek Microelectronics Corp. 2018.  All rights reserved
	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License version 2 as
	published by the Free Software Foundation.
*/

#ifndef _DRTC_REG_H
#define _DRTC_REG_H

#include <mach/rcw_macro.h>

/* DRTC Timer Register */
#define DRTC_TIMER_REG_OFS 0x00
union DRTC_TIMER_REG {
	uint32_t reg;
	struct {
	unsigned int sec:6;
	unsigned int reserved0:2;
	unsigned int min:6;
	unsigned int reserved1:2;
	unsigned int hour:5;
	unsigned int reserved2:11;
	} bit;
};

/* DRTC Day Register */
#define DRTC_DAY_REG_OFS 0x04
union DRTC_DAY_REG {
	uint32_t reg;
	struct {
	unsigned int day:17;
	unsigned int reserved0:15;
	} bit;
};

/* DRTC Alarm Register */
#define DRTC_ALARM_TIMER_REG_OFS 0x08
union DRTC_ALARM_TIMER_REG {
	uint32_t reg;
	struct {
	unsigned int sec:6;
	unsigned int reserved0:2;
	unsigned int min:6;
	unsigned int reserved1:2;
	unsigned int hour:5;
	unsigned int reserved2:11;
	} bit;
};

/* DRTC Alarm Day Register */
#define DRTC_ALARM_DAY_REG_OFS 0x0C
union DRTC_ALARM_DAY_REG {
	uint32_t reg;
	struct {
	unsigned int day:17;
	unsigned int reserved0:15;
	} bit;
};

/* DRTC Control Register */
#define DRTC_CTRL_REG_OFS 0x10
union DRTC_CTRL_REG {
	uint32_t reg;
	struct {
	unsigned int alarm_inten:1;
	unsigned int reserved0:3;
	unsigned int time_en:1;
	unsigned int reserved1:3;
	unsigned int alarm_period:4;
	unsigned int initial_division:8;
	unsigned int reserved2:12;
	} bit;
};

/* DRTC Status Register */
#define DRTC_STATUS_REG_OFS 0x14
union DRTC_STATUS_REG {
	uint32_t reg;
	struct {
	unsigned int alarm_sts:1;
	unsigned int offset_period_sts:1;
	unsigned int reserved0:30;
	} bit;
};
#endif
