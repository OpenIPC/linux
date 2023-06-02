/*
 *  arch/arm/mach-faraday/include/mach/debug-macro.S
 *
 *  Copyright (C) 2005 Faraday Technology
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <linux/linkage.h>
#include <mach/hardware.h>
#include <mach/serial.h>

		.macro	addruart, rp, rv, tmp
		ldr	\rp, =DEBUG_LL_FTUART010_PA_BASE	@ physical base address of UART
		ldr	\rv, =DEBUG_LL_FTUART010_VA_BASE	@ virtual base address of UART
		.endm

		.macro	senduart, rd, rx
		strb	\rd, [\rx, #SERIAL_THR]
		.endm

		.macro	waituart, rd, rx
1001:		ldrb	\rd, [\rx, #SERIAL_LSR]	@ LSR
		tst	\rd, #SERIAL_LSR_THRE	@ test empty
		beq	1001b
		.endm

		.macro	busyuart, rd, rx
		mov	\rd, #0x100
1010:		subs	\rd, \rd, #1
		bne	1010b
		.endm
