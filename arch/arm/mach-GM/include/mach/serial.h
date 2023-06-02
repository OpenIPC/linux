/*
 *  arch/arm/mach-faraday/include/mach/serial.h
 *  
 *  Platform Independent UART Console Configuration
 *  
 *  Copyright (C) 2005 Faraday Corp. (http://www.faraday-tech.com)
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
 *  
 * Note
 *
 *  The first UART (FTUART010) in the system is used as the console.
 * 
 * ChangeLog
 *
 *  Luke Lee  09/15/2005  Created.
 */

#ifndef __FARADAY_PLATFORM_INDEPENDENT_SERIAL_HEADER__
#define __FARADAY_PLATFORM_INDEPENDENT_SERIAL_HEADER__

#ifndef __ASSEMBLY__

#include <mach/platform/board.h>
#include <linux/serial.h>
/* Include platform *dependent* UART console configuration */
#include <mach/platform/serial.h>

#ifndef STD_COM_FLAGS
#define STD_COM_FLAGS  (ASYNC_BOOT_AUTOCONF | ASYNC_SKIP_TEST)
#endif

#ifndef PLATFORM_MORE_SERIAL_PORTS
#define PLATFORM_MORE_SERIAL_PORTS
#endif

#ifndef SERIAL_PORT_DFNS
     /* UART CLK        PORT                 IRQ                 FLAGS          hub6 io_type iomem_base reg_shift*/
#define SERIAL_PORT_DFNS	\
     { 0, BASE_BAUD, UART_FTUART010_0_VA_BASE, UART_FTUART010_0_IRQ, STD_COM_FLAGS,   0,    0,     NULL,       2 }, /* ttyS0 */ \
     PLATFORM_MORE_SERIAL_PORTS
#endif

/*
 * We use a 18.432MHz clock rather than typical 1.8432 MHz clock for UART.
 */
#define BASE_BAUD       (CONFIG_UART_CLK / 16)

#endif /* __ASSEMBLY__ */

#endif /* __FARADAY_PLATFORM_INDEPENDENT_SERIAL_HEADER__ */

