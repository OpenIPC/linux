/*
 * arch/arm/plat-ambarella/include/plat/chip.h
 *
 * Author: Anthony Ginger <hfjiang@ambarella.com>
 *
 * Copyright (C) 2004-2013, Ambarella, Inc.
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

#ifndef __PLAT_AMBARELLA_CHIP_H__
#define __PLAT_AMBARELLA_CHIP_H__

/* ==========================================================================*/
#define A5S		(5100)
#define A7L		(7500)
#define A8		(8000)
#define S2		(9000)
#define S2E		(9100)
#define S2L		(12000)
#define S3		(11000)
#define S3L		(13000)
#define S5		(15000)

#define CHIP_ID(x)	((x / 1000))
#define CHIP_MAJOR(x)	((x / 100) % 10)
#define CHIP_MINOR(x)	((x / 10) % 10)

#if	defined(CONFIG_PLAT_AMBARELLA_A5S)
#define CHIP_REV	A5S
#elif	defined(CONFIG_PLAT_AMBARELLA_A7L)
#define CHIP_REV	A7L
#elif	defined(CONFIG_PLAT_AMBARELLA_S2)
#define CHIP_REV	S2
#elif	defined(CONFIG_PLAT_AMBARELLA_S2E)
#define CHIP_REV	S2E
#elif	defined(CONFIG_PLAT_AMBARELLA_A8)
#define CHIP_REV	A8
#elif	defined(CONFIG_PLAT_AMBARELLA_S2L)
#define CHIP_REV	S2L
#elif defined(CONFIG_PLAT_AMBARELLA_S3)
#define CHIP_REV	S3
#elif defined(CONFIG_PLAT_AMBARELLA_S3L)
#define CHIP_REV	S3L
#elif defined(CONFIG_ARCH_AMBARELLA_S5)
#define CHIP_REV	S5
#else
#error "Undefined CHIP_REV"
#endif

/* ==========================================================================*/
#if (CHIP_REV == A8)
#define REF_CLK_FREQ			27000000
#else
#define REF_CLK_FREQ			24000000
#endif

/* ==========================================================================*/
#if (CHIP_REV == A5S) || (CHIP_REV == A7L)
#define	CHIP_BROKEN_UNALIGNED_ACCESS	1
#endif

#if (CHIP_REV == S3)
#define	CHIP_FIX_2NDCORTEX_BOOT	1
#endif

/* ==========================================================================*/

#endif /* __PLAT_AMBARELLA_CHIP_H__ */

