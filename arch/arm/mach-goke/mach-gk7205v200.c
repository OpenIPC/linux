/*
 * Copyright (c) Hunan Goke,Chengdu Goke,Shandong Goke. 2021. All rights reserved.
 */

#include <linux/of_address.h>
#include <asm/smp_scu.h>

#include "mach-common.h"

#ifdef CONFIG_SMP

#define REG_CPU_SRST_CRG    0x78
#define CPU1_SRST_REQ       BIT(2)
#define DBG1_SRST_REQ       BIT(4)

void bsp_set_cpu(unsigned int cpu, bool enable)
{
	struct device_node *np = NULL;
	unsigned int regval;
	void __iomem *crg_base;

	np = of_find_compatible_node(NULL, NULL, "goke,gk7205v200-clock");
	if (!np) {
		pr_err("failed to find goke clock node\n");
		return;
	}

	crg_base = of_iomap(np, 0);
	if (!crg_base) {
		pr_err("failed to map address\n");
		return;
	}

	if (enable) {
		/* clear the slave cpu reset */
		regval = readl(crg_base + REG_CPU_SRST_CRG);
		regval &= ~CPU1_SRST_REQ;
		writel(regval, (crg_base + REG_CPU_SRST_CRG));
	} else {
		regval = readl(crg_base + REG_CPU_SRST_CRG);
		regval |= (DBG1_SRST_REQ | CPU1_SRST_REQ);
		writel(regval, (crg_base + REG_CPU_SRST_CRG));
	}
}

static const struct smp_operations bsp_smp_ops __initconst = {
	.smp_prepare_cpus       = bsp_smp_prepare_cpus,
	.smp_boot_secondary     = bsp_boot_secondary,
};

CPU_METHOD_OF_DECLARE(gk7205v200_smp, "goke,gk7205v200-smp", &bsp_smp_ops);
#endif /* CONFIG_SMP */
