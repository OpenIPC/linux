/*
 * arch/arm/mach-gk/vi.c
 *
 * Author: Steven Yu, <yulindeng@gokemicro.com>
 * Copyright (C) 2012-2015, goke, Inc.
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

#include <linux/kernel.h>
#include <linux/i2c.h>

#include <mach/hardware.h>
#include <mach/idc.h>

/* ==========================================================================*/
struct i2c_board_info gk_board_vi_infos[2] = {
	[0] = {
		.type			= "gk_vi0",
		.addr			= 0x02,
	},
	[1] = {
		.type			= "gk_vi1",
		.addr			= 0x03,
	},
};

struct i2c_board_info gk_board_hdmi_infos[2] = {
	[0] = {
		.type			=  "rtc8563",
		.addr			= 0x51,
	},
	[1] = {
		.type			= "bma250",
		.addr			= 0x18,
	},
};

