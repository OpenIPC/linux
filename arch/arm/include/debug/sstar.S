/*
* sstar.S- Sigmastar
*
* Copyright (c) [2019~2020] SigmaStar Technology.
*
*
* This software is licensed under the terms of the GNU General Public
* License version 2, as published by the Free Software Foundation, and
* may be copied, distributed, and modified under those terms.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License version 2 for more details.
*
*/
#define DEBUG_UART_PHYS     0x1F221000
#define DEBUG_UART_VIRT     0xFD221000
#define UART_FIFO_REG       0x00
#define UART_LSR_REG        0x28
#define UART_LSR_TXEMPTY    0x5
#define UART_USR_REG        0x38
#define UART_USR_TFE        0x2

    .macro  addruart, rp, rv, tmp
    ldr \rp, =DEBUG_UART_PHYS
    ldr \rv, =DEBUG_UART_VIRT
    .endm

    .macro  senduart, rd, rx
    /* word access do not work */
    strb    \rd, [\rx, #UART_FIFO_REG]
    .endm

    .macro  waituart, rd, rx
1001:   ldr \rd, [\rx, #UART_LSR_REG]
    tst \rd, #(1 << UART_LSR_TXEMPTY)
    beq 1001b
    .endm

    .macro  waituarttxrdy,rd,rx
    .endm

    .macro  busyuart, rd, rx
1002:   ldr \rd, [\rx, #UART_USR_REG]
    tst \rd, #(1 << UART_USR_TFE)
    beq 1002b
    .endm
