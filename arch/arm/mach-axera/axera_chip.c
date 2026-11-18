/*
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
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/of_platform.h>
#include <linux/irqchip.h>
#include <linux/mfd/syscon.h>
#include <linux/regmap.h>
#include <asm/mach/arch.h>
#include <asm/mach/map.h>
#include "pm.h"


static void __init axera_chip_dt_init(void)
{
	axera_chip_pm_init();
}

static const char * const axera_chip_board_dt_compat[] = {
	"axera,ax620e",
	NULL,
};

DT_MACHINE_START(AXERACHIP_DT, "Axera_chip (Device Tree)")
	.l2c_aux_val	= 0,
	.l2c_aux_mask	= ~0,
	.dt_compat	= axera_chip_board_dt_compat,
	.init_machine	= axera_chip_dt_init,
MACHINE_END
