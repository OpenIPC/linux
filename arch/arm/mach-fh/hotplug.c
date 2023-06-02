/*
 * Copyright (c) 2013 Linaro Ltd.
 * Copyright (c) 2013 Hisilicon Limited.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 */

#include <linux/cpu.h>
#include <linux/delay.h>
#include <linux/io.h>
#include <linux/of_address.h>
#include <linux/of_platform.h>
#include <asm/cacheflush.h>
#include <asm/smp_plat.h>
#include <asm/system_misc.h>
#include "core.h"

/* Sysctrl registers in Hi3620 SoC */
#define SCISOEN				0xc0
#define SCISODIS			0xc4
#define SCPERPWREN			0xd0
#define SCPERPWRDIS			0xd4
#define SCCPUCOREEN			0xf4
#define SCCPUCOREDIS			0xf8
#define SCPERCTRL0			0x200
#define SCCPURSTEN			0x410
#define SCCPURSTDIS			0x414

/*
 * bit definition in SCISOEN/SCPERPWREN/...
 *
 * CPU2_ISO_CTRL	(1 << 5)
 * CPU3_ISO_CTRL	(1 << 6)
 * ...
 */
#define CPU2_ISO_CTRL			(1 << 5)

/*
 * bit definition in SCPERCTRL0
 *
 * CPU0_WFI_MASK_CFG	(1 << 28)
 * CPU1_WFI_MASK_CFG	(1 << 29)
 * ...
 */
#define CPU0_WFI_MASK_CFG		(1 << 28)

/*
 * bit definition in SCCPURSTEN/...
 *
 * CPU0_SRST_REQ_EN	(1 << 0)
 * CPU1_SRST_REQ_EN	(1 << 1)
 * ...
 */
#define CPU0_HPM_SRST_REQ_EN		(1 << 22)
#define CPU0_DBG_SRST_REQ_EN		(1 << 12)
#define CPU0_NEON_SRST_REQ_EN		(1 << 4)
#define CPU0_SRST_REQ_EN		(1 << 0)

#define HIX5HD2_PERI_CRG20		0x50
#define CRG20_CPU1_RESET		(1 << 17)

#define HIX5HD2_PERI_PMC0		0x1000
#define PMC0_CPU1_WAIT_MTCOMS_ACK	(1 << 8)
#define PMC0_CPU1_PMC_ENABLE		(1 << 7)
#define PMC0_CPU1_POWERDOWN		(1 << 3)

#define HIP01_PERI9                    0x50
#define PERI9_CPU1_RESET               (1 << 1)


static void __iomem *ctrl_base;


static int fhca7_hotplug_init(void)
{
	struct device_node *node;

	node = of_find_compatible_node(NULL, NULL, "fh,fh-pmu");
	if (!node) {
		return -ENOENT;
	}

	ctrl_base = of_iomap(node, 0);
	of_node_put(node);
	if (!ctrl_base) {
		return -ENOMEM;
	}

	return 0;
}

void fhca7_set_cpu(int cpu, bool enable)
{
	if (!ctrl_base) {
		if (fhca7_hotplug_init() < 0)
			return;
	}

}

static inline void cpu_enter_lowpower(void)
{
	unsigned int v;

	flush_cache_all();

	/*
	 * Turn off coherency and L1 D-cache
	 */
	asm volatile(
	"	mrc	p15, 0, %0, c1, c0, 1\n"
	"	bic	%0, %0, #0x40\n"
	"	mcr	p15, 0, %0, c1, c0, 1\n"
	"	mrc	p15, 0, %0, c1, c0, 0\n"
	"	bic	%0, %0, #0x04\n"
	"	mcr	p15, 0, %0, c1, c0, 0\n"
	  : "=&r" (v)
	  : "r" (0)
	  : "cc");
}

#ifdef CONFIG_HOTPLUG_CPU
void fhca7_cpu_die(unsigned int cpu)
{
	cpu_enter_lowpower();
	arch_cpu_idle();

	/* We should have never returned from idle */
	panic("cpu %d unexpectedly exit from shutdown\n", cpu);
}

int fhca7_cpu_kill(unsigned int cpu)
{
	fhca7_set_cpu(cpu, false);

	return 1;
}
#endif
