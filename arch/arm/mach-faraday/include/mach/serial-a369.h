/*
 *  arch/arm/mach-faraday/include/mach/serial-a369.h
 *
 *  Copyright (C) 2010 Faraday Technology
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

#ifndef __MACH_SERIAL_A369_H
#define __MACH_SERIAL_A369_H

#ifdef CONFIG_PLATFORM_A369

#include <mach/board-a369.h>
#include <mach/irqs-a369.h>

#define SERIAL_PORT_DFNS									\
	FTUART010_SERIAL_PORT(A369_FTUART010_0_VA_BASE, IRQ_A369_FTUART010_0),	/* ttyS0 */	\
	FTUART010_SERIAL_PORT(A369_FTUART010_1_VA_BASE, IRQ_A369_FTUART010_1),	/* ttyS1 */	\
	FTUART010_SERIAL_PORT(A369_FTUART010_2_VA_BASE, IRQ_A369_FTUART010_2),	/* ttyS2 */	\
	FTUART010_SERIAL_PORT(A369_FTUART010_3_VA_BASE, IRQ_A369_FTUART010_3),	/* ttyS3 */

#endif	/* CONFIG_PLATFORM_A369 */

#endif	/* __MACH_SERIAL_A369_H */
