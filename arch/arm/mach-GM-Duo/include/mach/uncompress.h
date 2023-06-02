/*
 *  arch/arm/mach-GM-Duo/include/mach/uncompress.h
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

#include <mach/platform/board.h>

#define SERIAL_THR	0x00
#define SERIAL_LSR	0x14
#define SERIAL_LSR_THRE	0x20

#define readl(a)	(*(volatile unsigned int *)(a))
#define writel(v,a)	(*(volatile unsigned int *)(a) = (v))

/*
 * This does not append a newline
 */
static inline void putc(int c)
{
	unsigned long base = DEBUG_LL_FTUART010_PA_BASE;

	while ((readl(base + SERIAL_LSR) & SERIAL_LSR_THRE) == 0)
		barrier();

	writel(c, base + SERIAL_THR);
}

static inline void flush(void)
{
}

/*
 * nothing to do
 */
#define arch_decomp_setup()
#define arch_decomp_wdog()
