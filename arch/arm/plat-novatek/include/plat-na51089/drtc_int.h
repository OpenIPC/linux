/*
	Internal header file for DRTC

	This file is the internal header file that defines the variables,
	structures and function prototypes of DRTC module.

	@file       drtc_int.h

	Copyright   Novatek Microelectronics Corp. 2018.  All rights reserved
	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License version 2 as
	published by the Free Software Foundation.
*/

#ifndef _DRTC_INT_H
#define _DRTC_INT_H

#define DRTC_INT_MAX_DAYS                     0x1FFFF
#define DRTC_CLEAN_VALUE                      0xFFFFFFFF
#define DRTC_OFFSET_MAX_VALUE                 0x7FFFFF
#define DRTC_OFFSET_MAX_PERIOD                0xFFFF
#define DRTC_OFFSET_MAX_BASE                  0xFF
#define DRTC_OFFSET_MAX_DEALY                 0x7FFF
#define DRTC_OFFSET_MAX_RESIDUAL              0x7FFF

// Register default value
#define DRTC_TIMER_REG_DEFAULT                0x00000000
#define DRTC_TIMER_DAY_REG_DEFAULT            0x00000000
#define DRTC_ALARM_REG_DEFAULT                0x00000000
#define DRTC_ALARM_DAY_REG_DEFAULT            0x00000000
#define DRTC_CTRL_REG_DEFAULT                 0x00000000
#define DRTC_STATUS_REG_DEFAULT               0x00000000
#define DRTC_OSC_CORRECT_REG_DEFAULT          0x00000000
#define DRTC_OSC_CORRECT_RESIDUAL_REG_DEFAULT 0x00000000
#define DRTC_CLK_FREQ_REG_DEFAULT             0x00B71AFE

#endif
