/*
 *  arch/arm/mach-faraday/include/mach/board-axi.h
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

#ifndef __MACH_BOARD_AXI_H
#define __MACH_BOARD_AXI_H

#ifdef CONFIG_PLATFORM_AXI

/*
 * Base addresses
 */

#define AXI_FTUART010_0_PA_BASE		0xa8000000
#define AXI_FTUART010_1_PA_BASE		0xa8100000
#define AXI_FTTMR010_0_PA_BASE		0xa8200000
#define AXI_FTINTC010_0_PA_BASE		0xa8300000
#define AXI_FTDMAC030_0_PA_BASE		0xa8500000

#define AXI_FTUART010_0_VA_BASE		0xfa800000
#define AXI_FTUART010_1_VA_BASE		0xfa810000
#define AXI_FTTMR010_0_VA_BASE		0xfa820000
#define AXI_FTINTC010_0_VA_BASE		0xfa830000

#define DEBUG_LL_FTUART010_PA_BASE	AXI_FTUART010_0_PA_BASE
#define DEBUG_LL_FTUART010_VA_BASE	AXI_FTUART010_0_VA_BASE

#endif	/* CONFIG_PLATFORM_AXI */

#endif	/* __MACH_BOARD_AXI_H */
