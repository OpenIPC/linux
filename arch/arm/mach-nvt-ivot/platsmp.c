/**
    NVT SMP(Multi-Core) source file
    It's used to do secondary core init.
    @file       platsmp.c
    @ingroup
    @note
    Copyright   Novatek Microelectronics Corp. 2017.  All rights reserved.

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2 as
    published by the Free Software Foundation.
*/

#include <linux/init.h>
#include <linux/device.h>
#include <linux/smp.h>
#include <linux/io.h>
#include <linux/delay.h>
#include <linux/of_address.h>

#include <asm/cacheflush.h>
#include <asm/smp_plat.h>
#include <asm/smp_scu.h>

#include <mach/nvt-io.h>

#include "common.h"

#ifdef CONFIG_OPTEE
#include <linux/arm-smccc.h>
#include <mach/smc_func_id/optee_smc.h>
#else
static DEFINE_SPINLOCK(boot_lock);
#endif

extern void nvt_secondary_startup(void);

/*
 * Initialise the CPU possible map early - this describes the CPUs
 * which may be present or become present in the system.
 */
static const struct of_device_id nvt_smp_dt_scu_match[] __initconst = {
	{ .compatible = "arm,cortex-a9-scu", },
	{}
};

#ifdef CONFIG_OPTEE
static void set_secondary_entry(u32 release_phys)
{
}
static void nvt_secondary_init(unsigned int cpu)
{
}
static int nvt_boot_secondary(unsigned int cpu, struct task_struct *idle)
{
	struct arm_smccc_res res = {0};

	// REF: generic_boot_core_release(size_t core_idx, paddr_t entry)
	arm_smccc_smc(OPTEE_SMC_BOOT_SECONDARY,
		cpu, /* core_idx */
		0,
		virt_to_phys(secondary_startup), /* jump entry */
		0, 0, 0, 0,
		&res);

	if (OPTEE_SMC_RETURN_OK == res.a0) {
		printk("OPTEE_SMC_RETURN_OK, return 0\n");
		return 0;
	} else {
		printk("res.a0 = %ld, return -ENOSYS\n", res.a0);
		return -ENOSYS;
	}
}
#else //#ifdef CONFIG_OPTEE

/*
 * Write the address of secondary startup into the
 * system register. The nvt_secondary_startup start to check
 * the pen_release value.
 */
static void set_secondary_entry(u32 release_phys)
{
	void __iomem *virt = ioremap_cache(release_phys, sizeof(u32 *));

	writel(virt_to_phys(nvt_secondary_startup), virt);
	//printk("release_phys=0x%x, virt=0x%x, read data=0x%x\n", release_phys, virt, readl(virt));

	smp_wmb();
#ifdef CONFIG_OUTER_CACHE
	outer_flush_range(release_phys, release_phys + 4);
#else
	__cpuc_flush_dcache_area(virt, sizeof(u32));
#endif
	iounmap(virt);
}

/*
 * Write pen_release in a way that is guaranteed to be visible to all
 * observers, irrespective of whether they're taking part in coherency
 * or not.  This is necessary for the hotplug code to work reliably.
 * The kernel L2 cache is ready. So call the outer_clean_range API.
 */
static void write_pen_release(int val)
{
  	pen_release = val;
	smp_wmb();
	sync_cache_w(&pen_release);
}

static void nvt_secondary_init(unsigned int cpu)
{
	/*
	 * let the primary processor know we're out of the
	 * pen, then head off into the C entry point
	 */
	write_pen_release(-1);

	/*
	 * Synchronise with the boot thread.
	 */
	spin_lock(&boot_lock);
	spin_unlock(&boot_lock);
}

static int nvt_boot_secondary(unsigned int cpu, struct task_struct *idle)
{
	unsigned long timeout;

	/*
	 * Set synchronisation state between this boot processor
	 * and the secondary one
	 */
	spin_lock(&boot_lock);

	/*
	 * This is really belt and braces; we hold unintended secondary
	 * CPUs in the holding pen until we're ready for them.  However,
	 * since we haven't sent them a soft interrupt, they shouldn't
	 * be there.
	 */
	write_pen_release(cpu_logical_map(cpu));

	/*
	 * Send the secondary CPU a soft interrupt, thereby causing
	 * the boot monitor to read the system wide flags register,
	 * and branch to the address found there.
	 */
	arch_send_wakeup_ipi_mask(cpumask_of(cpu));

	timeout = jiffies + (1 * HZ);
	while (time_before(jiffies, timeout)) {
		smp_rmb();

		if (pen_release == -1) {
			break;
		}

		udelay(10);
	}

	if (pen_release != -1)
		printk("pen_release: %d\n\t", pen_release);

	/*
	 * now the secondary core is starting up let it run its
	 * calibrations, then wait for it to finish
	 */
	spin_unlock(&boot_lock);

	return pen_release != -1 ? -ENOSYS : 0;
}
#endif //#ifdef CONFIG_OPTEE

static void __init nvt_ca9_smp_prepare_cpus(unsigned int max_cpus)
{
	struct device_node *scu = of_find_matching_node(NULL, nvt_smp_dt_scu_match);
	int cpu_count = 0;
	int cpu;

	if (scu)
		scu_enable(of_iomap(scu, 0));

	for_each_possible_cpu(cpu) {
		struct device_node *np;
		u32 release_phys;

		np = of_get_cpu_node(cpu, NULL);
		if (!np)
			continue;
		if (of_property_read_u32(np, "cpu-release-addr"
						, &release_phys))
			continue;

		if (cpu_count < max_cpus) {
			set_cpu_present(cpu, true);
			cpu_count++;
		}

		if (release_phys != 0) {
			set_secondary_entry(release_phys);
		}
	}
}

struct smp_operations nvt_ca9_smp_ops __initdata = {
	.smp_prepare_cpus	= nvt_ca9_smp_prepare_cpus,
	.smp_secondary_init	= nvt_secondary_init,
	.smp_boot_secondary	= nvt_boot_secondary,
#ifdef CONFIG_HOTPLUG_CPU
	.cpu_die		= nvt_cpu_die,
#endif
};
