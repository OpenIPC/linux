/*
 * arch/arm/plat-ambarella/include/plat/gdma.h
 *
 * History:
 *	2013/11/26 - Ken He <jianhe@ambarella.com> created file
 *
 * Copyright (C) 2012-2016, Ambarella, Inc.
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

#ifndef __PLAT_AMBARELLA_GDMA_H__
#define __PLAT_AMBARELLA_GDMA_H__

/****************************************************/
/* Capabilities based on chip revision              */
/****************************************************/

#define GDMA_OFFSET			0x15000
#define GDMA_BASE			(AHB_BASE + GDMA_OFFSET)
#define GDMA_REG(x)			(GDMA_BASE + (x))

#if (CHIP_REV == A5S)
#define GDMA_SUPPORT_ALPHA_BLEND	0
#else
#define GDMA_SUPPORT_ALPHA_BLEND	1
#endif

struct gdma_param {
	u32 dest_addr;
	u32 dest_virt_addr;
	u32 src_addr;
	u32 src_virt_addr;
	u8 dest_non_cached;
	u8 src_non_cached;
	u8 reserved[2];
	u16 src_pitch;
	u16 dest_pitch;
	u16 width;
	u16 height;
};

/****************************************************/
/* Controller registers definitions                 */
/****************************************************/
#define GDMA_SRC_1_BASE_OFFSET		0x00
#define GDMA_SRC_1_PITCH_OFFSET		0x04
#define GDMA_SRC_2_BASE_OFFSET		0x08
#define GDMA_SRC_2_PITCH_OFFSET		0x0c
#define GDMA_DST_BASE_OFFSET		0x10
#define GDMA_DST_PITCH_OFFSET		0x14
#define GDMA_WIDTH_OFFSET		0x18
#define GDMA_HIGHT_OFFSET		0x1c
#define GDMA_TRANSPARENT_OFFSET		0x20
#define GDMA_OPCODE_OFFSET		0x24
#define GDMA_PENDING_OPS_OFFSET		0x28

#if (GDMA_SUPPORT_ALPHA_BLEND == 1)
#define GDMA_PIXELFORMAT_OFFSET		0x2c
#define GDMA_ALPHA_OFFSET		0x30
#define GDMA_CLUT_BASE_OFFSET		0x400
#endif

#define GDMA_SRC_1_BASE_REG		GDMA_REG(GDMA_SRC_1_BASE_OFFSET)
#define GDMA_SRC_1_PITCH_REG		GDMA_REG(GDMA_SRC_1_PITCH_OFFSET)
#define GDMA_SRC_2_BASE_REG		GDMA_REG(GDMA_SRC_2_BASE_OFFSET)
#define GDMA_SRC_2_PITCH_REG		GDMA_REG(GDMA_SRC_2_PITCH_OFFSET)
#define GDMA_DST_BASE_REG		GDMA_REG(GDMA_DST_BASE_OFFSET)
#define GDMA_DST_PITCH_REG		GDMA_REG(GDMA_DST_PITCH_OFFSET)
#define GDMA_WIDTH_REG			GDMA_REG(GDMA_WIDTH_OFFSET)
#define GDMA_HEIGHT_REG			GDMA_REG(GDMA_HIGHT_OFFSET)
#define GDMA_TRANSPARENT_REG		GDMA_REG(GDMA_TRANSPARENT_OFFSET)
#define GDMA_OPCODE_REG			GDMA_REG(GDMA_OPCODE_OFFSET)
#define GDMA_PENDING_OPS_REG		GDMA_REG(GDMA_PENDING_OPS_OFFSET)

#if (GDMA_SUPPORT_ALPHA_BLEND == 1)
#define GDMA_PIXELFORMAT_REG		GDMA_REG(GDMA_PIXELFORMAT_OFFSET)
#define GDMA_ALPHA_REG			GDMA_REG(GDMA_ALPHA_OFFSET)
#define GDMA_CLUT_BASE_REG		GDMA_REG(GDMA_CLUT_BASE_OFFSET)

/* GDMA_PIXELFORMAT_REG */
#define GDMA_PIXELFORMAT_THROTTLE_DRAM	(1L << 11)

#endif /* (GDMA_SUPPORT_ALPHA_BLEND == 1)*/

#endif

