/*
 *  arch/arm/mach-GM-Duo/include/mach/platform/serial.h
 *
 *  Serial port definition
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
 *  There are 4 UARTs (FTUART010) in GM platform
 *
 * ChangeLog
 *
 *  Justin Shih  10/06/2012  Created.
 */

//#define UART_CLKSRC_OSCH//issue

#ifdef UART_CLKSRC_OSCH
#define CONFIG_UART_CLK			12288000//12000000
#else
#define CONFIG_UART_CLK			25000000//(30000000/16)
#endif

     /* UART CLK        PORT                      IRQ                   FLAGS       hub6 io_type iomem_base reg_shift*/
#ifdef UART_FTUART010_1_VA_BASE
#define EXTENDED_UART_1 \
     { 0, BASE_BAUD, UART_FTUART010_1_VA_BASE, UART_FTUART010_1_IRQ, STD_COM_FLAGS,   0,    0,     NULL,       2 }, /* ttyS1 */
#else
#define EXTENDED_UART_1
#endif

#define PLATFORM_MORE_SERIAL_PORTS \
        EXTENDED_UART_1

