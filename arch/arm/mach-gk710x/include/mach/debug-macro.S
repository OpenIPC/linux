/*
 * arch/arm/mach-gk710x/include/mach/debug-macro.S
 *
 * History:
 *  2014/06/18 - [Kewell Liu] created file
 *
 * Copyright (C) 2002-2014, Goke Microelectronics China.
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
#include <mach/uart.h>

#ifdef CONFIG_MMU

    .macro addruart, rp, rv, tmp
        ldr \rp, =GK_PA_APB_BASE   @ Physical Base
        ldr \rv, =GK_VA_APB_BASE   @ Virtual Base
    .endm

#else /* !CONFIG_MMU */

    .macro  addruart, rx, tmp
        mrc p15, 0, \rx, c1, c0
        tst \rx, #1         @ MMU enabled
        moveq   \rx, #GK_PA_APB_BASE    @ physical base address
        movne   \rx, #GK_VA_APB_BASE    @ virtual address
        orr \rx, \rx, #UART_OFFSET
    .endm

#endif /* CONFIG_MMU */

.macro senduart, rd, rx
    str \rd, [\rx, #UART_TH_OFFSET]
.endm

.macro waituart, rd, rx
1001:
    ldr \rd, [\rx, #UART_LS_OFFSET]
    tst \rd, #UART_LS_TEMT
    beq 1001b

.endm

.macro busyuart, rd, rx
1002:
    ldr \rd, [\rx, #UART_LS_OFFSET]
    tst \rd, #UART_LS_TEMT
    beq 1002b
.endm

