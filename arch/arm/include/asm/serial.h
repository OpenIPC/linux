/*
 *  arch/arm/include/asm/serial.h
 *
 *  Copyright (C) 1996 Russell King.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 *  Changelog:
 *   15-10-1996	RMK	Created
 */

#ifndef __ASM_SERIAL_H
#define __ASM_SERIAL_H

#if defined(CONFIG_ARCH_FARADAY)
#include <mach/serial.h>
#endif

#if (defined(CONFIG_ARCH_GM) || defined(CONFIG_ARCH_GM_DUO) || defined(CONFIG_ARCH_GM_SMP))
#include <mach/serial.h>
#endif

#ifndef BASE_BAUD
#define BASE_BAUD	(1843200 / 16)
#endif

#endif
