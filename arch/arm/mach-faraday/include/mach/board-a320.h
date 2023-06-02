/*
 *  arch/arm/mach-faraday/include/mach/board-a320.h
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

#ifndef __MACH_BOARD_A320_H
#define __MACH_BOARD_A320_H

#ifdef CONFIG_PLATFORM_A320

/*
 * Base addresses
 */

#ifdef CONFIG_CPU_FMP626

/* private memory region */
#define PLATFORM_SCU_PA_BASE		0xe0000000
#define PLATFORM_SCU_VA_BASE		0xf9100000

#define PLATFORM_GIC_CPU_VA_BASE	(PLATFORM_SCU_VA_BASE + 0x100)
#define PLATFORM_TWD_VA_BASE		(PLATFORM_SCU_VA_BASE + 0x600)

#define PLATFORM_GIC_DIST_PA_BASE	0xe0001000
#define PLATFORM_GIC_DIST_VA_BASE	0xf9101000

#endif	/* CONFIG_CPU_FMP626 */

/* A320 */
#define A320_FTFLH010_0_PA_BASE		0x80440000
#define A320_FTDMAC020_0_PA_BASE	0x90400000
#define A320_FTAPBB020_0_PA_BASE	0x90500000
#define A320_FTLCDC100_0_PA_BASE	0x90600000
#define A320_FTMAC100_0_PA_BASE		0x90900000
#define A320_FTNANDC020_0_PA_BASE	0x90c00000
#define A320_FTSDC020_0_PA_BASE		0x92000000
#define A320_FTPMU010_0_PA_BASE		0x98100000
#define A320_FTUART010_0_PA_BASE	0x98200000
#define A320_FTUART010_1_PA_BASE	0x98300000
#define A320_FTTMR010_0_PA_BASE		0x98400000
#define A320_FTWDT010_0_PA_BASE		0x98500000
#define A320_FTRTC010_0_PA_BASE		0x98600000
#define A320_FTGPIO010_0_PA_BASE	0x98700000
#define A320_FTINTC010_0_PA_BASE	0x98800000
#define A320_FTI2C010_0_PA_BASE		0x98a00000
#define A320_FTCFC010_0_PA_BASE		0x98d00000
#define A320_FTSDC010_0_PA_BASE		0x98e00000
#define A320_FTSSP010_0_PA_BASE		0x99400000
#define A320_FTUART010_2_PA_BASE	0x99600000

#define A320_FTPMU010_0_VA_BASE		0xf9810000
#define A320_FTUART010_0_VA_BASE	0xf9820000
#define A320_FTUART010_1_VA_BASE	0xf9830000
#define A320_FTTMR010_0_VA_BASE		0xf9840000
#define A320_FTINTC010_0_VA_BASE	0xf9880000
#define A320_FTUART010_2_VA_BASE	0xf9960000

/* A321 */
#define A321_FTPCI100_0_PA_BASE		0x90c00000
#define A321_FTAPBB020_0_PA_BASE	0x90e00000
#define A321_FTMAC100_0_PA_BASE		0x92000000
#define A321_FTINTC010_0_PA_BASE	0xb0800000
#define A321_FTKBC010_0_PA_BASE		0xb1300000
#define A321_FTKBC010_1_PA_BASE		0xb1700000

#define A321_FTPCI100_0_VA_BASE		0xf9cc0000
#define A321_FTINTC010_0_VA_BASE	0xfb080000

/* USB */
#define A320_FUSB220_0_PA_BASE		0x90b00000
#define A320_FOTG2XX_0_PA_BASE		0x92000000
#define A320_FUSBH200_0_PA_BASE		0x92000000

/* PCIMEM */
#define PCIMEM_0_PA_BASE	0xa0000000
#define PCIMEM_0_PA_LIMIT	0xafffffff
#define PCIMEM_0_PA_SIZE	0x10000000
#define PCIMEM_0_VA_BASE	0xe0000000
#define PCIMEM_0_VA_LIMIT	0xefffffff
#define PCIMEM_0_VA_SIZE	0x10000000
#define PCIMEM_PA_BASE	PCIMEM_0_PA_BASE	/* used by ftpci100.c */
#define PCIMEM_PA_LIMIT	PCIMEM_0_PA_LIMIT	/* used by ftpci100.c */
#define PCIMEM_PA_SIZE	PCIMEM_0_PA_SIZE
#define PCIMEM_VA_BASE	PCIMEM_0_VA_BASE
#define PCIMEM_VA_LIMIT	PCIMEM_0_VA_LIMIT
#define PCIMEM_VA_SIZE	PCIMEM_0_VA_SIZE

/* PCIIO */
#define PCIIO_0_PA_BASE		0x90c01000
#define PCIIO_0_PA_LIMIT	0x90cfffff
#define PCIIO_0_PA_SIZE		0x000ff000
#define PCIIO_0_VA_BASE		0xf9d01000
#define PCIIO_0_VA_LIMIT	0xf9dfffff
#define PCIIO_0_VA_SIZE		0x000ff000
#define PCIIO_PA_BASE	PCIIO_0_PA_BASE		/* used by ftpci100.c */
#define PCIIO_PA_LIMIT	PCIIO_0_PA_LIMIT
#define PCIIO_PA_SIZE	PCIIO_0_PA_SIZE
#define PCIIO_VA_BASE	PCIIO_0_VA_BASE		/* used by ftpci100.c */
#define PCIIO_VA_LIMIT	PCIIO_0_VA_LIMIT	/* used by ftpci100.c */
#define PCIIO_VA_SIZE	PCIIO_0_VA_SIZE

#define DEBUG_LL_FTUART010_PA_BASE	A320_FTUART010_0_PA_BASE
#define DEBUG_LL_FTUART010_VA_BASE	A320_FTUART010_0_VA_BASE

/*
 * The "Main CLK" Oscillator on the board which is used by the PLL to generate
 * CPU/AHB/APB clocks.
 */
#define MAIN_CLK	3686400

#endif	/* CONFIG_PLATFORM_A320 */

#endif	/* __MACH_BOARD_A320_H */
