/*
 * /linux/arch/arm/mach-anycloud/mach-ak39ev330.c
 *
 * Copyright (C) 2020 Anyka(Guangzhou) Microelectronics Technology Co., Ltd.
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

#include <linux/platform_device.h>
#include <linux/irq.h>
#include <linux/of_platform.h>

#include <asm/mach/arch.h>
#include <asm/mach/map.h>
#include <asm/mach-types.h>
#include <asm/irq.h>
#include <asm/sizes.h>

#include <mach/ak_l2.h>
#include <mach/map.h>

#include <linux/input.h>
#include <linux/irqchip.h>

#define AK_CPU_ID			(AK_VA_SYSCTRL + 0x00)

#define AKCPU_VALUE			0x20160101
#define AKCPU_TYPE			"AK39XXEV330"

/* The Cloud39EV300 SDK's release value, its mach-ak39/cpu.c. */
#define AKCPU_VALUE_EV300		0x20160100
#define AKCPU_TYPE_EV300		"AK39XXEV300"

/*
 * AK3918EV200 boards report one of two CPU-ID values at this register;
 * both identify AK3918EV200 silicon.
 */
#define AKCPU_VALUE_EV200_1		0x20120100
#define AKCPU_VALUE_EV200_2		0x20150200
#define AKCPU_TYPE_EV200		"AK3918EV200"

/* The Aimer39 SDK's own release value, its mach-ak39/cpu.c. */
#define AKCPU_VALUE_AK3918		0x20120100
#define AKCPU_TYPE_AK3918		"AK3918"

#define IODESC_ENT(x) 							\
{												\
	.virtual = (unsigned long)AK_VA_##x,		\
	.pfn	 = __phys_to_pfn(AK_PA_##x),		\
	.length	 = AK_SZ_##x,						\
	.type	 = MT_DEVICE						\
}

static struct map_desc ak39ev330_iodesc[] __initdata = {
	IODESC_ENT(SYSCTRL),
	IODESC_ENT(CAMERA),
	IODESC_ENT(SUBCTRL),
	IODESC_ENT(MIPI1),
	IODESC_ENT(L2MEM),
#ifndef CONFIG_AK3918_FAMILY
	IODESC_ENT(RESERVED_MEM),
#endif
};

void __init ak39ev330_map_io(void)
{
	unsigned long regval = 0x0;
    
	/* initialise the io descriptors we need for initialisation */
	iotable_init(ak39ev330_iodesc, ARRAY_SIZE(ak39ev330_iodesc));

	regval = __raw_readl(AK_CPU_ID);
	if (regval == AKCPU_VALUE)
		pr_info("ANYKA CPU %s (ID 0x%lx)\n", AKCPU_TYPE, regval);
#ifdef CONFIG_MACH_AK3918EV300
	else if (regval == AKCPU_VALUE_EV300)
		pr_info("ANYKA CPU %s (ID 0x%lx)\n", AKCPU_TYPE_EV300, regval);
#endif
#ifdef CONFIG_MACH_AK3918
	else if (regval == AKCPU_VALUE_AK3918)
		pr_info("ANYKA CPU %s (ID 0x%lx)\n", AKCPU_TYPE_AK3918, regval);
#endif
	else if (regval == AKCPU_VALUE_EV200_1 || regval == AKCPU_VALUE_EV200_2)
		pr_info("ANYKA CPU %s (ID 0x%lx)\n", AKCPU_TYPE_EV200, regval);
	else
		pr_info("Unknown ANYKA CPU ID: 0x%lx\n", regval);

}

void wdt_enable(void);
void wdt_keepalive(unsigned int heartbeat);

static void ak39ev330_restart(enum reboot_mode mode, const char *cmd)
{
#if defined CONFIG_AK_WATCHDOG_TOP
	wdt_enable();
	wdt_keepalive(2);
#endif
}

static void __init ak39ev330_init(void)
{	
    int ret;
	l2_init();

	ret = of_platform_populate(NULL, of_default_bus_match_table, NULL,
				   NULL);
	if (ret) {
		pr_err("of_platform_populate failed: %d\n", ret);
		BUG();
	}    
	
	return;
}


#ifndef CONFIG_MACH_AK3918EV300
static const char * const ak39ev330_dt_compat[] = {
    "anyka,ak3916ev330",
	"anyka,ak3918ev330",
	"anyka,ak3919ev330",
	"anyka,ak39ev330",
	"anyka,ak3916ev331",
	"anyka,ak3918ev331",
	"anyka,ak3919ev331",
	"anyka,ak39ev331",
	NULL
};

DT_MACHINE_START(AK39xxEV330, "AK39EV330")
/* Maintainer: Anyka(Guangzhou) Microelectronics Technology Co., Ltd */
	.dt_compat	= ak39ev330_dt_compat,
	.map_io = ak39ev330_map_io,
	.init_time = NULL,
	.init_machine = ak39ev330_init,
	.init_early = NULL,
	.reserve = NULL,
    .restart = ak39ev330_restart,
MACHINE_END
#endif /* !CONFIG_MACH_AK3918EV300 */

#ifdef CONFIG_MACH_AK3918EV300
/*
 * "CLOUD39EV3_AK3918EV300_MNBD" is the Hardware line the Cloud39EV300 SDK's
 * own machines (mach-cloud39ev3_ak3918ev300_mnbd.c and its AK3916/AK3919
 * siblings) gave /proc/cpuinfo. It replaces the EV330 machine rather than
 * joining ak39ev330_dt_compat: this kernel drives only the five base timers,
 * and the EV330 one drives timers 6 and 7, which the EV300 does not have, so
 * each kernel takes only its own chip's device trees and says nothing for
 * the other's instead of hanging at the first tick.
 */
static const char * const ak3918ev300_dt_compat[] = {
	"anyka,ak3918ev300",
	"anyka,ak3916ev300",
	"anyka,ak3919ev300",
	NULL
};

DT_MACHINE_START(AK3918EV300, "CLOUD39EV3_AK3918EV300_MNBD")
	.dt_compat	= ak3918ev300_dt_compat,
	.map_io = ak39ev330_map_io,
	.init_time = NULL,
	.init_machine = ak39ev330_init,
	.init_early = NULL,
	.reserve = NULL,
	.restart = ak39ev330_restart,
MACHINE_END
#endif /* CONFIG_MACH_AK3918EV300 */

#ifdef CONFIG_AK3918_FAMILY

#define AK3918EV200_DRAM_BASE	0x80000000

#ifndef CONFIG_PHYS_OFFSET
#error "AK3918/AK3918EV200 need a fixed CONFIG_PHYS_OFFSET; ARM_PATCH_PHYS_VIRT cannot align it"
#endif
#if CONFIG_PHYS_OFFSET != AK3918EV200_DRAM_BASE + CONFIG_VIDEO_RESERVED_MEM_SIZE
#error "CONFIG_PHYS_OFFSET must equal 0x80000000 + CONFIG_VIDEO_RESERVED_MEM_SIZE"
#endif
#endif

#ifdef CONFIG_MACH_AK3918
/*
 * "Aimer39_AK3918_MB_V1.0.0" is the Hardware line the Aimer39 SDK's own machine
 * (mach-aimer39_ak3918.c) gave /proc/cpuinfo, which is what ipctool and
 * the vendor tools match on.
 */
static const char * const ak3918_dt_compat[] = {
	"anyka,ak3918",
	"anyka,ak3916",
	/*
	 * The SDK's third package of the same die: same CPU ID 0x20120100,
	 * eleven pads not bonded out (ak39-gpio.c ak3910_invalid_gpio[]).
	 */
	"anyka,ak3910",
	NULL
};

DT_MACHINE_START(AK3918, "Aimer39_AK3918_MB_V1.0.0")
	.dt_compat	= ak3918_dt_compat,
	.map_io = ak39ev330_map_io,
	.init_time = NULL,
	.init_machine = ak39ev330_init,
	.init_early = NULL,
	.reserve = NULL,
	.restart = ak39ev330_restart,
MACHINE_END
#endif /* CONFIG_MACH_AK3918 */

#ifdef CONFIG_MACH_AK3918EV200

/*
 * The second DT_MACHINE_START() argument is what ends up in
 * /proc/cpuinfo's "Hardware" line: "Cloud39EV2_AK3918E80PIN_MNBD".
 */
static const char * const ak3918ev200_dt_compat[] = {
	"anyka,ak3918ev200",
	NULL
};

DT_MACHINE_START(AK3918EV200, "Cloud39EV2_AK3918E80PIN_MNBD")
/* Maintainer: Alex Zhang <alex@osqdu.org> */
	.dt_compat	= ak3918ev200_dt_compat,
	.map_io = ak39ev330_map_io,
	.init_time = NULL,
	.init_machine = ak39ev330_init,
	.init_early = NULL,
	.reserve = NULL,
	.restart = ak39ev330_restart,
MACHINE_END
#endif /* CONFIG_MACH_AK3918EV200 */
