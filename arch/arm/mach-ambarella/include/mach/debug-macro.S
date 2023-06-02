/*
 * arch/arm/plat-ambarella/include/mach/debug-macro.S
 *
 * History:
 *	2006/12/27 - [Charles Chiou] created file
 *	2010/03/29 - [Cao Rongrong] port to BOSS
 *	2010/11/04 - [Cao Rongrong] port to Linux-2.6.36+
 *
 * Copyright (C) 2004-2009, Ambarella, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 */

#include <mach/hardware.h>
#include <plat/uart.h>

#define AMBA_UART_BASE_P		(APB_PHYS_BASE + UART_OFFSET)
#define AMBA_UART_BASE_V		(APB_BASE + UART_OFFSET)

#if defined(CONFIG_AMBARELLA_DEBUG_LL_UART0)
	.macro	addruart, rp, rv, tmp
	ldr	\rp, =AMBA_UART_BASE_P
	ldr	\rv, =AMBA_UART_BASE_V
	.endm

	.macro	senduart, rd, rx
	str	\rd, [\rx, #UART_TH_OFFSET]
	.endm

	.macro	waituart, rd, rx
1001:
	ldr	\rd, [\rx, #UART_LS_OFFSET]
	tst	\rd, #UART_LS_TEMT
	beq	1001b
	.endm

	.macro	busyuart, rd, rx
1002:
	ldr	\rd, [\rx, #UART_LS_OFFSET]
	tst	\rd, #UART_LS_TEMT
	beq	1002b
	.endm
#elif defined(CONFIG_DEBUG_LL_UART_NONE)
	.macro	addruart, rp, rv, tmp
	ldr	\rp, =AMBA_UART_BASE_P
	ldr	\rv, =AMBA_UART_BASE_V
	.endm

	.macro	senduart,rd,rx
	.endm

	.macro	waituart,rd,rx
	.endm

	.macro	busyuart,rd,rx
	.endm
#endif

