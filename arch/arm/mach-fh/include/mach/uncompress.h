/*
 * Serial port stubs for kernel decompress status messages
 *
 * Initially based on:
 * arch/arm/plat-omap/include/mach/uncompress.h
 *
 * Original copyrights follow.
 *
 * Copyright (C) 2000 RidgeRun, Inc.
 * Author: Greg Lonnon <glonnon@ridgerun.com>
 *
 * Rewritten by:
 * Author: <source@mvista.com>
 * 2004 (c) MontaVista Software, Inc.
 *
 * This file is licensed under the terms of the GNU General Public License
 * version 2. This program is licensed "as is" without any warranty of any
 * kind, whether express or implied.
 */

#include <linux/types.h>

#include <asm/mach-types.h>
#include <mach/chip.h>

#define     REG_UART_THR		(0x0000)
#define     REG_UART_USR		(0x007c)

#define REG_UART0_THR	(*(unsigned char *)(CONSOLE_REG_BASE + REG_UART_THR))
#define REG_UART0_USR	(*(unsigned char *)(CONSOLE_REG_BASE + REG_UART_USR))

static void putc(char c)
{
	while (!(REG_UART0_USR & (1 << 1)))
		barrier();

	REG_UART0_THR = c;
}

static inline void flush(void)
{
	while (!(REG_UART0_USR & (1 << 2)))
		barrier();
}

static inline void set_uart_info(u32 phys, void *__iomem virt)
{

}

static inline void __arch_decomp_setup(unsigned long arch_id)
{

}

#define arch_decomp_setup()	__arch_decomp_setup(arch_id)
#define arch_decomp_wdog()
