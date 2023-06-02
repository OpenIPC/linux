/*
 *  arch/arm/mach-faraday/include/mach/irqs-a320.h
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

#ifndef __MACH_IRQS_A320_H
#define __MACH_IRQS_A320_H

#ifdef CONFIG_PLATFORM_A320

/*
 * Interrupt numbers of Hierarchical Architecture
 */

#ifdef CONFIG_CPU_FMP626

/* GIC DIST in FMP626 */
#define IRQ_LOCALTIMER		29
#define IRQ_LOCALWDOG		30
#define PLATFORM_LEGACY_IRQ	31
#define IRQ_FMP626_GIC_START	32

#define IRQ_FMP626_PMU_CPU0	(IRQ_FMP626_GIC_START + 17)
#define IRQ_FMP626_PMU_CPU1	(IRQ_FMP626_GIC_START + 18)
#define IRQ_FMP626_PMU_SCU0	(IRQ_FMP626_GIC_START + 21)
#define IRQ_FMP626_PMU_SCU1	(IRQ_FMP626_GIC_START + 22)
#define IRQ_FMP626_PMU_SCU2	(IRQ_FMP626_GIC_START + 23)
#define IRQ_FMP626_PMU_SCU3	(IRQ_FMP626_GIC_START + 24)

#define IRQ_A320_START	256	/* this is determined by irqs supported by GIC */

#else	/* CONFIG_CPU_FMP626 */

#define IRQ_A320_START	0

#endif	/* CONFIG_CPU_FMP626 */

#define IRQ_A320_FTCFC010_0_CD		(IRQ_A320_START + 0)
#define IRQ_A320_FTCFC010_0_DMA		(IRQ_A320_START + 1)
#define IRQ_A320_FTI2C010_0		(IRQ_A320_START + 3)
#define IRQ_A320_FTSDC010_0		(IRQ_A320_START + 5)
#define IRQ_A320_FTSSP010_0		(IRQ_A320_START + 6)
#define IRQ_A320_FTUART010_2		(IRQ_A320_START + 7)
#define IRQ_A320_FTPMU010_0		(IRQ_A320_START + 8)
#define IRQ_A320_FTUART010_0		(IRQ_A320_START + 10)
#define IRQ_A320_FTUART010_1		(IRQ_A320_START + 11)
#define IRQ_A320_FTGPIO010_0		(IRQ_A320_START + 13)
#define IRQ_A320_FTTMR010_0_T1		(IRQ_A320_START + 14)
#define IRQ_A320_FTTMR010_0_T2		(IRQ_A320_START + 15)
#define IRQ_A320_FTWDT010_0		(IRQ_A320_START + 16)
#define IRQ_A320_FTRTC010_0_ALRM	(IRQ_A320_START + 17)
#define IRQ_A320_FTRTC010_0_SEC		(IRQ_A320_START + 18)
#define IRQ_A320_FTTMR010_0_T0		(IRQ_A320_START + 19)
#define IRQ_A320_FTLCDC100_0		(IRQ_A320_START + 20)
#define IRQ_A320_FTDMAC020_0		(IRQ_A320_START + 21)
#define IRQ_A320_FTAPBB020_0		(IRQ_A320_START + 24)
#define IRQ_A320_FTMAC100_0		(IRQ_A320_START + 25)
#define IRQ_A320_FUSB220_0		(IRQ_A320_START + 26)
#define IRQ_A320_FTSDC020_0		(IRQ_A320_START + 29)
#define IRQ_A320_FTNANDC020_0		(IRQ_A320_START + 29)
#define IRQ_A320_FOTG2XX_0		(IRQ_A320_START + 29)
#define IRQ_A320_FUSBH200_0		(IRQ_A320_START + 29)
#define IRQ_A320_A321			(IRQ_A320_START + 30)

/* A321 */
#define IRQ_A321_START			(IRQ_A320_START + 32)

#define IRQ_A321_FTKBC010_0		(IRQ_A321_START + 4)
#define IRQ_A321_FTKBC010_1		(IRQ_A321_START + 9)
#define IRQ_A321_FTAPBB020_0		(IRQ_A321_START + 24)
#define IRQ_A321_FTMAC100_0		(IRQ_A321_START + 25)
#define IRQ_A321_FTPCI100_0		(IRQ_A321_START + 28)

/* PCIC */
#define IRQ_FTPCI100_0_START		(IRQ_A321_START + 32)

#define IRQ_FTPCI100_0_A		(IRQ_FTPCI100_0_START + 0)
#define IRQ_FTPCI100_0_B		(IRQ_FTPCI100_0_START + 1)
#define IRQ_FTPCI100_0_C		(IRQ_FTPCI100_0_START + 2)
#define IRQ_FTPCI100_0_D		(IRQ_FTPCI100_0_START + 3)

#define NR_IRQS				(IRQ_FTPCI100_0_START + 4)

#define MAX_FTINTC010_NR	2

#endif	/* CONFIG_PLATFORM_A320 */

#endif	/* __MACH_IRQS_A320_H */
