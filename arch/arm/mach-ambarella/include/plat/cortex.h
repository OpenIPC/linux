/*
 * arch/arm/plat-ambarella/include/plat/cortex.h
 *
 * Author: Anthony Ginger <hfjiang@ambarella.com>
 *
 * Copyright (C) 2004-2010, Ambarella, Inc.
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

#ifndef __PLAT_AMBARELLA_CORTEX_H
#define __PLAT_AMBARELLA_CORTEX_H

/* ==========================================================================*/
#define AMBARELLA_VA_SCU_BASE			(AXI_BASE + 0x00000000)
#define AMBARELLA_VA_GIC_CPU_BASE		(AXI_BASE + 0x00000100)
#define AMBARELLA_VA_GT_BASE			(AXI_BASE + 0x00000200)
#define AMBARELLA_VA_PT_WD_BASE			(AXI_BASE + 0x00000600)
#define AMBARELLA_VA_GIC_DIST_BASE		(AXI_BASE + 0x00001000)
#define AMBARELLA_VA_L2CC_BASE			(AXI_BASE + 0x00002000)

#define AMBARELLA_PA_SCU_BASE			(AXI_PHYS_BASE + 0x00000000)
#define AMBARELLA_PA_GIC_CPU_BASE		(AXI_PHYS_BASE + 0x00000100)
#define AMBARELLA_PA_GT_BASE			(AXI_PHYS_BASE + 0x00000200)
#define AMBARELLA_PA_PT_WD_BASE			(AXI_PHYS_BASE + 0x00000600)
#define AMBARELLA_PA_GIC_DIST_BASE		(AXI_PHYS_BASE + 0x00001000)
#define AMBARELLA_PA_L2CC_BASE			(AXI_PHYS_BASE + 0x00002000)

/* ==========================================================================*/
#define PROCESSOR_START_0			(0)
#define PROCESSOR_START_1			(1)
#define PROCESSOR_START_2			(2)
#define PROCESSOR_START_3			(3)

#define PROCESSOR_STATUS_0			(4)
#define PROCESSOR_STATUS_1			(5)
#define PROCESSOR_STATUS_2			(6)
#define PROCESSOR_STATUS_3			(7)

#define ICDISER0_MASK				(8)
#define INTDIS_LOCKER				(9)
#define INTDIS_STATUS				(10)

#define MACHINE_ID				(11)
#define ATAG_DATA				(12)

#define AMBARELLA_BST_HEAD_CACHE_SIZE		(32)

/* ==========================================================================*/
#ifndef __ASSEMBLER__

/* ==========================================================================*/

#endif /* __ASSEMBLER__ */
/* ==========================================================================*/

#endif

