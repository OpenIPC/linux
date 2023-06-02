/*
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
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>

#include <asm/pmu.h>

#include <plat/irq.h>

/* ==========================================================================*/
#if defined(CONFIG_AMBARELLA_PMUSERENR_EN)
static void pmu_set_userspace_access(void *en)
{
	int val = 0;
	/* Read PMUSERENR Register */
	asm volatile("mrc p15, 0, %0, c9, c14, 0" : : "r" (val));
	val &= ~0x1;
	val |= (int)en;
	/* Write PMUSERENR Register */
	asm volatile("mcr p15, 0, %0, c9, c14, 0" : : "r" (val));
}
#endif

/* ==========================================================================*/
#if defined(PMU_IRQ)
/*
 * PMU Interrupt is not connected to A5S/A7L/A8/S2,
 * the counters are still available, and some profiling tools,
 * such as 'perf' can live with that and collect statistics.
 *
 * So we provide PMU_IRQ setup here for all chips.
 */
static struct resource ambarella_pmu_resource[] = {
	DEFINE_RES_IRQ(PMU_IRQ)
};

static struct platform_device ambarella_pmu = {
	.name		= "arm-pmu",
	.id		= -1,
	.num_resources	= ARRAY_SIZE(ambarella_pmu_resource),
	.resource	= ambarella_pmu_resource,
};
#endif

/* ==========================================================================*/
static int __init ambarella_pmu_init(void)
{
	int ret_val = 0;
#if defined(PMU_IRQ)
	ret_val = platform_device_register(&ambarella_pmu);
#endif
#if defined(CONFIG_AMBARELLA_PMUSERENR_EN)
	pr_info("Enable PMUSERENR on all cores ... ");
	ret_val = on_each_cpu(pmu_set_userspace_access, (void*)1, 1);
	pr_info("done\n");
#endif
	return ret_val;
}
arch_initcall(ambarella_pmu_init);

