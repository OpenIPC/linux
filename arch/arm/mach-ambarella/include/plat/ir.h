/*
 * arch/arm/plat-ambarella/include/plat/ir.h
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

#ifndef __PLAT_AMBARELLA_IR_H__
#define __PLAT_AMBARELLA_IR_H__

/* ==========================================================================*/
#define IR_OFFSET			0x6000
#define IR_BASE				(APB_BASE + IR_OFFSET)
#define IR_REG(x)			(IR_BASE + (x))

/* ==========================================================================*/
#define IR_CONTROL_OFFSET		0x00
#define IR_STATUS_OFFSET		0x04
#define IR_DATA_OFFSET			0x08

#define IR_CONTROL_REG			IR_REG(0x00)
#define IR_STATUS_REG			IR_REG(0x04)
#define IR_DATA_REG			IR_REG(0x08)

/* IR_CONTROL_REG */
#define IR_CONTROL_RESET		0x00004000
#define IR_CONTROL_ENB			0x00002000
#define IR_CONTROL_LEVINT		0x00001000
#define IR_CONTROL_INTLEV(x)		(((x) & 0x3f)  << 4)
#define IR_CONTROL_FIFO_OV		0x00000008
#define IR_CONTROL_INTENB		0x00000004

enum ambarella_ir_protocol {
	AMBA_IR_PROTOCOL_NEC = 0,
	AMBA_IR_PROTOCOL_PANASONIC = 1,
	AMBA_IR_PROTOCOL_SONY = 2,
	AMBA_IR_PROTOCOL_PHILIPS = 3,
	AMBA_IR_PROTOCOL_END
};

#endif /* __PLAT_AMBARELLA_IR_H__ */

