/*
	Register offset and bit definition for WDT module

	Register offset and bit definition for WDT module.

	@file       wdt_reg.h
	@ingroup    miDrvTimer_WDT
	@note       Nothing.

	Copyright   Novatek Microelectronics Corp. 2018.  All rights reserved
	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License version 2 as
	published by the Free Software Foundation.
*/

#ifndef __ASM_ARCH_NA51068_WDT_REG_H
#define __ASM_ARCH_NA51068_WDT_REG_H
#include <mach/rcw_macro.h>
#include "hardware.h"

/*12 MHz / 1024*/
#define WDT_SOURCE_CLOCK	(12000000 >> 10)
/*Maximum MSB value*/
#define WDT_MSB_MAX	0xFF
/*Key to write WDT Control Register*/
#define WDT_KEY_VALUE	0x5A96
/*Macro to generate dummy element for enum type to expand enum size to word*/
#define ENUM_DUMMY4WORD(name)   E_##name = 0x10000000

extern void nvt_trigger_manual_reset(void);

enum {
	WDT_MODE_INT,		/*Issue interrupt when WDT expired */
	WDT_MODE_RESET,		/*Reset system when WDT expired */

	ENUM_DUMMY4WORD(WDT_MODE)
};

/*WDT Control Register*/
#define WDT_CTRL_REG_OFS 0x00
union WDT_CTRL_REG {
	uint32_t reg;
	struct {
	unsigned int enable:1;
	unsigned int mode:1;
	unsigned int reserved0:2;
	unsigned int ext_reset:1;
	unsigned int rst_num0_en:1;
	unsigned int rst_num1_en:1;
	unsigned int reserved1:1;
	unsigned int msb:8;
	unsigned int key_ctrl:16;
	} bit;
};

/*WDT Status and Counter Register*/
#define WDT_STS_REG_OFS 0x04
union WDT_STS_REG {
	uint32_t reg;
	struct {
	unsigned int cnt:20;
	unsigned int reserved0:10;
	unsigned int en_status:1;
	unsigned int status:1;
	} bit;
};

/*WDT Trigger Register*/
#define WDT_TRIG_REG_OFS 0x08
union WDT_TRIG_REG {
	uint32_t reg;
	struct {
	unsigned int trigger:1;
	unsigned int reserved0:31;
	} bit;
};

/*WDT MANUAL RESET Register*/
#define WDT_MANUAL_RST_REG_OFS 0x0C
union WDT_MANUAL_RST_REG {
	uint32_t reg;
	struct {
	unsigned int manual_rst:1;
	unsigned int reserved0:31;
	} bit;
};

/*Reserved (0x10 ~ 0x7C)*/

/*WDT Timeout Record Register*/
#define WDT_REC_REG_OFS 0x80
union WDT_REC_REG {
	uint32_t reg;
	struct {
	unsigned int reset_num0:8;
	unsigned int resrt_num1:8;
	unsigned int reserved0:16;
	} bit;
};

/*WDT User Data Register*/
#define WDT_UDATA_REG_OFS 0x84
union WDT_UDATA_REG {
	uint32_t reg;
	struct {
	unsigned int user_data:32;
	} bit;
};
#endif /* __ASM_ARCH_NA51055_WDT_REG_H */
