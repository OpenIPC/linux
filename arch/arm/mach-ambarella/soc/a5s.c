/*
 * arch/arm/mach-ambarella/init-coconut.c
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

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/dma-mapping.h>
#include <linux/irqchip.h>
#include <linux/of_platform.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/input.h>

#include <asm/mach-types.h>
#include <asm/mach/arch.h>
#include <asm/system_info.h>
#include <mach/hardware.h>
#include <mach/init.h>

static const char * const a5s_dt_board_compat[] = {
	"ambarella,a5s",
	NULL,
};

DT_MACHINE_START(A5S_DT, "Ambarella A5S (Flattened Device Tree)")
	.restart_mode	= 's',
	.map_io		= ambarella_map_io,
	.init_early	= ambarella_init_early,
	.init_irq	= irqchip_init,
	.init_time	= ambarella_timer_init,
	.init_machine	= ambarella_init_machine,
	.restart	= ambarella_restart_machine,
	.dt_compat	= a5s_dt_board_compat,
MACHINE_END

