/*
 * Copyright (c) 2013 Linaro Ltd.
 * Copyright (c) 2013 Hisilicon Limited.
 * Based on arch/arm/mach-vexpress/platsmp.c, Copyright (C) 2002 ARM Ltd.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 */
#include <linux/smp.h>
#include <linux/io.h>
#include <linux/of_address.h>
#include <linux/delay.h>

#include <asm/cacheflush.h>
#include <asm/smp_plat.h>
#include <asm/smp_scu.h>
#include <asm/mach/map.h>

#include "core.h"

static void __iomem *ctrl_base;
static int use_emc;


void fhca7_set_cpu_jump(int cpu, void *jump_addr)
{
	cpu = cpu_logical_map(cpu);
	if (!cpu || !ctrl_base)
		return;
	if (use_emc == 1)
		writel_relaxed(virt_to_phys(jump_addr), ctrl_base + 0x2000);
	else
		writel_relaxed(virt_to_phys(jump_addr),
						ctrl_base + COH_FUN_BASE);
}

int fhca7_get_cpu_jump(int cpu)
{
	cpu = cpu_logical_map(cpu);
	if (!cpu || !ctrl_base)
		return 0;
	if (use_emc == 1)
		return readl_relaxed(ctrl_base + 0x2000);
	else
		return readl_relaxed(ctrl_base + COH_FUN_BASE);
}



static void __init fhca7_smp_prepare_cpus(unsigned int max_cpus)
{
	struct device_node *np = NULL;
	void __iomem *pmu_base;

	if (!ctrl_base) {
		np = of_find_compatible_node(NULL, NULL, "fh,fh-pmu");
		if (!np) {
			pr_err("failed to find fh,fh-pmu node\n");
			return;
		}
		pmu_base = of_iomap(np, 0);
		if (!pmu_base) {
			pr_err("failed to map address\n");
			return;
		}
		if (readl(pmu_base + COH_MAGIC) != MPCORE_COH_MAGIC) {
			use_emc = 1;
			pr_err("mismatch magic ,return to emc to find coh-pen\n");
			np = of_find_compatible_node(NULL, NULL, "fh,emc-ram");
			if (!np) {
				pr_err("failed to find fh,emc-ramnode\n");
				return;
			}
			ctrl_base = of_iomap(np, 0);
			if (!ctrl_base) {
				pr_err("failed to map address\n");
				return;
			}
		} else {
			ctrl_base = pmu_base;
			use_emc = 0;
		}
	}
}

static int fhca7_boot_secondary(unsigned int cpu, struct task_struct *idle)
{
	fhca7_set_cpu(cpu, true);
	fhca7_set_cpu_jump(cpu, secondary_startup);
	arch_send_wakeup_ipi_mask(cpumask_of(cpu));
	return 0;
}

static const struct smp_operations fhca7_smp_ops __initconst = {
	.smp_prepare_cpus	= fhca7_smp_prepare_cpus,
	.smp_boot_secondary	= fhca7_boot_secondary,
#ifdef CONFIG_HOTPLUG_CPU
	.cpu_die		= fhca7_cpu_die,
	.cpu_kill		= fhca7_cpu_kill,
#endif
};


CPU_METHOD_OF_DECLARE(fhca7_smp, "fh,fh-ca7-smp", &fhca7_smp_ops);
