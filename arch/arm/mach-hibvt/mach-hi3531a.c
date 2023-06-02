/*
 * Copyright (c) 2016-2017 HiSilicon Technologies Co., Ltd.
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
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
*/

#include <linux/delay.h>
#include <linux/smp.h>
#include <linux/io.h>
#include <linux/of_address.h>
#include <asm/cacheflush.h>
#include <asm/smp_plat.h>
#include <asm/smp_scu.h>
#include <asm/mach/arch.h>
#include <asm/mach/map.h>
#include <mach/io.h>
#include <mach/platform.h>

#include "mach-common.h"

/*
 * This table is only for optimization. Since ioremap() could always share
 * the same mapping if it's defined as static IO mapping.
 *
 * Without this table, system could also work. The cost is some virtual address
 * spaces wasted since ioremap() may be called multi times for the same
 * IO space.
 */
static struct map_desc hi3531a_io_desc[] __initdata = {
	/* hi3531a_IOCH1 */
	{
		.pfn		= __phys_to_pfn(HI3531A_IOCH1_PHYS),
		.virtual	= HI3531A_IOCH1_VIRT,
		.length		= HI3531A_IOCH1_SIZE,
		.type		= MT_DEVICE,
	},

	/* hi3531a_IOCH2 */
	{
		.pfn		= __phys_to_pfn(HI3531A_IOCH2_PHYS),
		.virtual	= HI3531A_IOCH2_VIRT,
		.length		= HI3531A_IOCH2_SIZE,
		.type		= MT_DEVICE,
	},

	/* hi3531a_IOCH3 */
	{
		.pfn        = __phys_to_pfn(HI3531A_IOCH3_PHYS),
		.virtual    = HI3531A_IOCH3_VIRT,
		.length     = HI3531A_IOCH3_SIZE,
		.type       = MT_DEVICE
	},

	/* hi3531a_IOCH4 */
	{
		.pfn        = __phys_to_pfn(HI3531A_IOCH4_PHYS),
		.virtual    = HI3531A_IOCH4_VIRT,
		.length     = HI3531A_IOCH4_SIZE,
		.type       = MT_DEVICE
	},
};

static void __init hi3531a_map_io(void)
{
	/* debug_ll_io_init(); */
	iotable_init(hi3531a_io_desc, ARRAY_SIZE(hi3531a_io_desc));
}

static void __init hi3531a_init_early(void)
{
	/*
	 * 1. enable L1 prefetch                       [2]
	 * 4. enable allocation in one cache way only. [8]
	 */
	asm volatile (
			"   mrc p15, 0, r0, c1, c0, 1\n"
			"   orr r0, r0, #0x104\n"
			"   mcr p15, 0, r0, c1, c0, 1\n"
			:
			:
			: "r0", "cc");

}

static const char *const hi3531a_compat[] __initconst = {
	"hisilicon,hi3531a",
	NULL,
};

DT_MACHINE_START(HI3531A_DT, "Hisilicon Hi3531A (Flattened Device Tree)")
	.map_io		= hi3531a_map_io,
	.dt_compat	= hi3531a_compat,
	.init_early = hi3531a_init_early,
MACHINE_END

#if CONFIG_SMP

void hi35xx_set_cpu(unsigned int cpu, bool enable)
{
	struct device_node *np = NULL;
	unsigned int regval;
	void __iomem *crg_base;

	np = of_find_compatible_node(NULL, NULL, "hisilicon,hi3531a-clock");
	if (!np) {
		pr_err("failed to find hisilicon clock node\n");
		return;
	}

	crg_base = of_iomap(np, 0);
	if (!crg_base) {
		pr_err("failed to map address\n");
		return;
	}

	if (enable) {
		/* clear the slave cpu reset */
		regval = readl(crg_base + REG_A9_SRST_CRG);
		regval &= ~CPU1_SRST_REQ;
		writel(regval, (crg_base + REG_A9_SRST_CRG));
	} else {
		regval = readl(crg_base + REG_A9_SRST_CRG);
		regval |= (WDG1_SRST_REQ | DBG1_SRST_REQ | CPU1_SRST_REQ);
		writel(regval, (crg_base + REG_A9_SRST_CRG));
	}

	iounmap(crg_base);
}

void hi35xx_secondary_init(unsigned int cpu)
{
	/*
	 * 1. enable L1 prefetch                       [2]
	 * 2. enable L2 prefetch hint                  [1]a
	 * 3. enable write full line of zeros mode.    [3]a
	 * 4. enable allocation in one cache way only. [8]
	 *   a: This feature must be enabled only when the slaves
	 *      connected on the Cortex-A17 AXI master port support it.
	 */
	asm volatile (
			"   mrc p15, 0, r0, c1, c0, 1\n"
			"   orr r0, r0, #0x0104\n"
			"   orr r0, r0, #0x02\n"
			"   mcr p15, 0, r0, c1, c0, 1\n"
			:
			:
			: "r0", "cc");
}

#ifdef CONFIG_HOTPLUG_CPU
void hi35xx_cpu_die(unsigned int cpu)
{
	flush_cache_all();
	hi35xx_set_cpu(cpu, false);
	BUG();
}

int hi35xx_cpu_kill(unsigned int cpu)
{
	return 0;
}
#endif /* CONFIG_HOTPLUG_CPU */

static const struct smp_operations hi35xx_smp_ops __initconst = {
	.smp_prepare_cpus       = hi35xx_smp_prepare_cpus,
	.smp_secondary_init		= hi35xx_secondary_init,
	.smp_boot_secondary     = hi35xx_boot_secondary,
#ifdef CONFIG_HOTPLUG_CPU
	.cpu_die		= hi35xx_cpu_die,
	.cpu_kill		= hi35xx_cpu_kill,
#endif
};

CPU_METHOD_OF_DECLARE(hi3531a_smp, "hisilicon,hi3531a-smp", &hi35xx_smp_ops);
#endif /* CONFIG_SMP */
