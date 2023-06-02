/**
    NVT board init function
    This file will setup initial function
    @file       board-dt.c
    @ingroup
    @note
    Copyright   Novatek Microelectronics Corp. 2019.  All rights reserved.

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2 as
    published by the Free Software Foundation.
*/

#include <linux/console.h>

#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/interrupt.h>
#include <linux/list.h>
#include <linux/timer.h>
#include <linux/init.h>
#include <linux/serial_core.h>
#include <linux/platform_device.h>
#include <linux/io.h>
#include <linux/clocksource.h>
#include <linux/initrd.h>
#include <linux/of_platform.h>
#include <asm/mach-types.h>
#include <asm/mach/arch.h>
#include <asm/mach/map.h>
#include <mach/fmem.h>
#include <mach/common.h>

#include "common.h"

/* To do nvt board initialization */
static void __init nvt_ivot_machine_init(void)
{
	/* Add  device tree peripheral devices resource */
	of_platform_populate(NULL, of_default_bus_match_table, NULL, NULL);
}

#ifdef CONFIG_TEE
static void nvt_l2_write_sec(unsigned long val, unsigned reg)
{
	nvt_smc_l2_write_sec(val, reg);
}
#endif

#if defined(CONFIG_NVT_IVOT_PLAT_NA51068)
static const char * const nvt_dt_match[] __initconst = {
	"nvt,ca9",
	NULL,
};
#elif defined(CONFIG_NVT_IVOT_PLAT_NA51068_CA53)
static const char * const nvt_dt_match[] __initconst = {
	"nvt,ca53",
	NULL,
};
#elif defined(CONFIG_NVT_IVOT_PLAT_NA51055) || defined(CONFIG_NVT_IVOT_PLAT_NA51089)
static const char * const nvt_dt_match[] __initconst = {
	"nvt,ca9",
	NULL,
};
#endif

DT_MACHINE_START(NVT_IVOT_DT, "Novatek Video Platform")
	.dt_compat	= nvt_dt_match,
	.map_io		= nvt_ivot_map_io,
	.init_machine	= nvt_ivot_machine_init,
	.restart	= nvt_ivot_restart,
#ifdef CONFIG_CACHE_L2X0
	.l2c_aux_val    = 0x00000000,
	.l2c_aux_mask   = 0xffffffff,
#ifdef CONFIG_TEE
	.l2c_write_sec	= nvt_l2_write_sec,
#endif /* CONFIG_TEE */
#endif /* CONFIG_CACHE_L2X0 */
#ifdef CONFIG_SMP
	#if (defined(CONFIG_NVT_IVOT_PLAT_NA51068) || defined(CONFIG_NVT_IVOT_PLAT_NA51055) || defined(CONFIG_NVT_IVOT_PLAT_NA51089))
	.smp		= smp_ops(nvt_ca9_smp_ops),
	#else
	.smp		= smp_ops(nvt_smp_ops),
	#endif
#endif
MACHINE_END
