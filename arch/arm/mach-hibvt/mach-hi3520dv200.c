// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Hi3520DV200 (single Cortex-A9 NVR/DVR SoC).
 *
 * Everything the kernel needs comes from the device tree; the GIC and the
 * SP804 timers have generic drivers.
 */

#include <linux/irqchip.h>
#include <asm/hardware/cache-hil2v200.h>
#include <asm/mach/arch.h>

static const char *const hi3520dv200_compat[] __initconst = {
	"hisilicon,hi3520dv200",
	NULL,
};

/*
 * The L2 is HiSilicon's own controller, so it is brought up here, at the
 * point where the core would initialise a PL310.
 */
static void __init hi3520dv200_init_irq(void)
{
	irqchip_init();
	hil2v200_of_init();
}

DT_MACHINE_START(HI3520DV200_DT, "Hisilicon Hi3520DV200 (Flattened Device Tree)")
	.dt_compat	= hi3520dv200_compat,
	.init_irq	= hi3520dv200_init_irq,
MACHINE_END
