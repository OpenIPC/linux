/*
 * arch/arm/include/asm/hardware/cache-ftl2cc031.h
 *
 * Copyright (C) 2010-2012 Faraday Limited
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

#ifndef __ASM_ARM_HARDWARE_FTL2CC031_H
#define __ASM_ARM_HARDWARE_FTL2CC031_H

#define L2X0_CTRL               0x000   
#define L2X0_AUX_CTRL           0x008

#define L2X0_EVENT_CNT0_CTRL    0x010
#define L2X0_EVENT_CNT1_CTRL    0x014
#define L2X0_EVENT_CNT1_VAL	0x018
#define L2X0_EVENT_CNT0_VAL	0x01C
#define L2X0_RAW_INTR_SRC	0x020
#define L2X0_MASKED_INTR_SRC    0x024
#define L2X0_INTR_MASK		0x028
#define L2X0_INTR_CLEAR		0x02C

#define L2X0_CACHE_MAINTENAN_PA		0x040
#define L2X0_CACHE_MAINTENAN_CNT	0x044
#define L2X0_LINE_WAY_IDX		0x048
#define L2X0_WAY_IDX			0x04C
#define L2X0_CACHE_MAINTENANCE		0x050
#define L2X0_LOCKDOWN_LINE	0x060
#define L2X0_LOCKDOWN_WAY_I	0x064
#define L2X0_LOCKDOWN_WAY_D	0x068
#define L2X0_PREFETCH_CTRL	0x070

#define L2X0_LINE_DATA_0	0x080
#define L2X0_LINE_DATA_1	0x084
#define L2X0_LINE_DATA_2	0x088
#define L2X0_LINE_DATA_3	0x08C
#define L2X0_LINE_DATA_4	0x090
#define L2X0_LINE_DATA_5	0x094
#define L2X0_LINE_DATA_6	0x098
#define L2X0_LINE_DATA_7	0x09C
#define L2X0_LINE_TAG		0x0A0
#define L2X0_LINE_DATA_PARITY	0x0A4

#define L2X0_CACHE_REVISION	0x200


/////////////////////////////////////////////////////////////
#define CTRL_TAG_INIT		(1 << 1)
#define CTRL_L2CC_EN		(1 << 0)

#define AUX_WAYNUM_16		(1 << 19)
#define AUX_WAYSIZE(x)		((x & 0x7) << 16)
#define AUX_FWALLOC		(1 << 13)
#define AUX_NOT_FWALLOC		(1 << 12)
#define AUX_SHA_ATTRI_EN	(1 << 10)
#define AUX_EXC_CACHE_EN	(1 << 9)
#define AUX_PAR_CHK_EN		(1 << 8)

#define AUX_WAYSIZE_MASK	(0x7 << 16)

#define WAYSIZE_16KB		0x0
#define WAYSIZE_32KB		0x1
#define WAYSIZE_64KB		0x2
#define WAYSIZE_128KB		0x3
#define WAYSIZE_256KB		0x4
#define WAYSIZE_512KB		0x5

// L2X0_CACHE_MAINTENANCE field
#define L2X0_CACHE_SYNC		1
#define L2X0_INV_PA		2
#define L2X0_INV_LINE_WAY	3	// specify which way + which index
#define L2X0_INV_WAY		4	// specify which way
#define L2X0_CLEAN_PA		5
#define L2X0_CLEAN_LINE_WAY	6
#define L2X0_CLEAN_WAY		7
#define L2X0_CLEAN_INV_PA	8
#define L2X0_CLEAN_INV_LINE_WAY	9
#define L2X0_CLEAN_INV_WAY	10
#define L2X0_UNLOCK_WAY		11
#define L2X0_TAG_WRITE		12
#define L2X0_TAG_READ		13
#define L2X0_DATA_WRITE		14
#define L2X0_DATA_READ		15
 
// new added function
#define MASK_PA                   0xFFFFFFE0   // specify physical address   

#ifndef __ASSEMBLY__
extern void __init ftl2cc031_init(void __iomem *base, __u32 aux_val, __u32 aux_mask);
#endif

#endif
