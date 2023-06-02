/*
 * Fullhan FH8810 board support
 *
 * Copyright (C) 2014 Fullhan Microelectronics Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation version 2.
 *
 * This program is distributed "as is" WITHOUT ANY WARRANTY of any
 * kind, whether express or implied; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/io.h>
#include <linux/init.h>

#include <asm/mach/arch.h>
#include <asm/mach/map.h>
#include <mach/fh_chipid.h>
#include <mach/pmu.h>

#ifdef CONFIG_CPU_V6
#include <asm/system_misc.h>
static void fh_cpu_do_idle(void)
{
}

static int __init fh_cpu_idle_init(void)
{
	arm_pm_idle = fh_cpu_do_idle;
	return 0;
}
early_initcall(fh_cpu_idle_init);
#endif


static int __init fullhan_init_early(void)
{

	fh_pmu_init();
	fh_chipid_init();

#ifdef CONFIG_EMULATION
	pr_info("---Emulation Version---\n");
#endif
	return 0;
}
arch_initcall(fullhan_init_early);

#ifdef CONFIG_USE_OF

static const char *fh8856v200_compat[] __initconst = {
	"fh,fh8856v200",
	NULL,
};

static const char *fh8858v200_compat[] __initconst = {
	"fh,fh8858v200",
	NULL,
};

static const char *fh8852v200_compat[] __initconst = {
	"fh,fh8852v200",
	NULL,
};

static const char *fh8856v210_compat[] __initconst = {
	"fh,fh8856v210",
	NULL,
};

static const char *fh8858v210_compat[] __initconst = {
	"fh,fh8858v210",
	NULL,
};

static const char *fh8852v210_compat[] __initconst = {
	"fh,fh8852v210",
	NULL,
};







DT_MACHINE_START(FH8856V200, "Fullhan FH8856V200 (Flattened Device Tree)")
	.dt_compat      = fh8856v200_compat,
MACHINE_END

DT_MACHINE_START(FH8858V200, "Fullhan FH8858V200 (Flattened Device Tree)")
	.dt_compat      = fh8858v200_compat,
MACHINE_END

DT_MACHINE_START(FH8852V200, "Fullhan FH8852V200 (Flattened Device Tree)")
	.dt_compat      = fh8852v200_compat,
MACHINE_END


DT_MACHINE_START(FH8856V210, "Fullhan FH8856V210 (Flattened Device Tree)")
	.dt_compat      = fh8856v210_compat,
MACHINE_END

DT_MACHINE_START(FH8858V210, "Fullhan FH8858V210 (Flattened Device Tree)")
	.dt_compat      = fh8858v210_compat,
MACHINE_END

DT_MACHINE_START(FH8852V210, "Fullhan FH8852V210 (Flattened Device Tree)")
	.dt_compat      = fh8852v210_compat,
MACHINE_END







#endif
