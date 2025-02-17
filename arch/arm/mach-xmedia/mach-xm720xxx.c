/*
 * Copyright (c) XMEDIA. All rights reserved.
 */

#include <linux/of_address.h>
#include <asm/smp_scu.h>

#include "mach-common.h"

#ifdef CONFIG_SMP
void bsp_set_cpu(unsigned int cpu, bool enable)
{
	return;
}

static const struct smp_operations bsp_smp_ops __initconst = {
	.smp_prepare_cpus       = bsp_smp_prepare_cpus,
	.smp_boot_secondary     = bsp_boot_secondary,
};

CPU_METHOD_OF_DECLARE(x572050500_smp, "xmedia,xm720xxx-smp", &bsp_smp_ops);
#endif /* CONFIG_SMP */
