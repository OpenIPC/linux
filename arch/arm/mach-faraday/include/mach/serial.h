/*
 *  arch/arm/mach-faraday/include/mach/serial.h
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

#ifndef __ASM_ARCH_SERIAL_H
#define __ASM_ARCH_SERIAL_H

#ifndef __ASSEMBLY__
#include <linux/serial.h>

/*
 * We use a 18.432MHz clock rather than typical 1.8432 MHz clock for UART.
 */
#define UART_CLK	18432000

#define BASE_BAUD	(UART_CLK / 16)
#define STD_COM_FLAGS	(UPF_BOOT_AUTOCONF)

#define FTUART010_SERIAL_PORT(_port, _irq)	{	\
		.baud_base	= BASE_BAUD,		\
		.port		= (_port),		\
		.irq		= (_irq),		\
		.flags		= STD_COM_FLAGS,	\
		.io_type	= UPIO_PORT,		\
		.iomem_reg_shift= 2,			\
	}

#endif	/* __ASSEMBLY__ */

#define SERIAL_THR	0x00
#define SERIAL_LSR	0x14
#define SERIAL_LSR_THRE	0x20

#include <mach/serial-a320.h>
#include <mach/serial-a369.h>
#include <mach/serial-axi.h>

#endif /* __ASM_ARCH_SERIAL_H */
