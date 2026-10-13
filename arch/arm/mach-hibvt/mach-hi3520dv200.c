// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Hi3520DV200 (single Cortex-A9 NVR/DVR SoC).
 *
 * Everything the kernel needs comes from the device tree; the GIC and the
 * SP804 timers have generic drivers.
 */

#include <asm/mach/arch.h>

static const char *const hi3520dv200_compat[] __initconst = {
	"hisilicon,hi3520dv200",
	NULL,
};

DT_MACHINE_START(HI3520DV200_DT, "Hisilicon Hi3520DV200 (Flattened Device Tree)")
	.dt_compat	= hi3520dv200_compat,
MACHINE_END
