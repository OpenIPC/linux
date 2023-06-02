/*
	Pinmux module internal header file

	Pinmux module internal header file

	@file		top_reg.h
	@ingroup
	@note		Nothing

	Copyright   Novatek Microelectronics Corp. 2018.  All rights reserved
	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License version 2 as
	published by the Free Software Foundation.
*/

#ifndef _PINMUX_REG_H
#define _PINMUX_REG_H

#include <mach/rcw_macro.h>
#define ENUM_DUMMY4WORD(name)   E_##name = 0x10000000

#define TOP_REGGPIO_START_OFS    0x20
#define TOP_REGGPIO_END_OFS      0x80

#endif
