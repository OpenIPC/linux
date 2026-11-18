/*
 *
 *  Copyright (C) 2002 ARM Ltd.
 *  All Rights Reserved
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <linux/errno.h>
#include <linux/smp.h>
#include <linux/io.h>
#include <linux/of.h>
#include <asm/cacheflush.h>
#include <asm/smp_plat.h>
#include <linux/delay.h>

static u32 cpus_release_paddr;
static bool cpus_inited = false;
static DEFINE_SPINLOCK(boot_lock);
extern void secondary_holding_pen(void);

/*
 * Write pen_release in a way that is guaranteed to be visible to all
 * observers, irrespective of whether they're taking part in coherency
 * or not.  This is necessary for the hotplug code to work reliably.
 */
static void write_pen_release(int val)
{
	pen_release = val;
	smp_wmb();
	sync_cache_w(&pen_release);
}

static void ax620x_smp_init_cpus(void)
{
	struct device_node *cpus_node = NULL;

	cpus_node = of_find_node_by_path("/cpus");
	if (!cpus_node) {
		pr_err("No CPU information found in DT\n");
		return;
	}

	if (of_property_read_u32(cpus_node,
				 "secondary-boot-reg",
				 &cpus_release_paddr)) {
		pr_err("required secondary release register not specified for cpus\n");
		return;
	}

	cpus_inited = true;
}

static void ax620x_smp_prepare_cpus(unsigned int max_cpus)
{
	void __iomem *cpus_release_vaddr;
	phys_addr_t secondary_startup_paddr;

	if (!cpus_inited)
		return;

	cpus_release_vaddr = ioremap_nocache((phys_addr_t)cpus_release_paddr,
											sizeof(phys_addr_t));
	if (!cpus_release_vaddr) {
		cpus_inited = false;
		pr_err("unable to ioremap secondary_release register for cpus\n");
		return;
	}

	secondary_startup_paddr = __pa_symbol(secondary_holding_pen);
	BUG_ON(secondary_startup_paddr > (phys_addr_t)U32_MAX);

	/* core1-core3 hold pending at secondary_holding_pen */
	writel_relaxed(secondary_startup_paddr, cpus_release_vaddr);
	sev();

	iounmap(cpus_release_vaddr);
}

static int ax620x_smp_boot_secondary(unsigned int cpu,
					struct task_struct *idle)
{
	unsigned long timeout;

	if (!cpus_inited)
		return -ENOSYS;
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
	arch_send_wakeup_ipi_mask(cpumask_of(cpu));

	timeout = jiffies + (2* HZ);
	while (time_before(jiffies, timeout)) {
		smp_rmb();
		if (pen_release == -1)
			break;

		udelay(10);
	}

	/*
	 * now the secondary core is starting up let it run its
	 * calibrations, then wait for it to finish
	 */
	spin_unlock(&boot_lock);

	return pen_release != -1 ? -ENOSYS : 0;
}

static void ax620x_secondary_init(unsigned int cpu)
{
	/*
	 * let the primary processor know we're out of the
	 * pen, then head off into the C entry point
	 */
	write_pen_release(-1);

	/*
	 * Synchronise with the boot thread.
	 */
	// spin_lock(&boot_lock);
	// spin_unlock(&boot_lock);
}

#ifdef CONFIG_HOTPLUG_CPU
#include <asm/cp15.h>
#include <asm/smp_plat.h>

static inline void cpu_enter_lowpower(void)
{
	unsigned int v;

	asm volatile(
		"mcr	p15, 0, %1, c7, c5, 0\n"
	"	mcr	p15, 0, %1, c7, c10, 4\n"
	/*
	 * Turn off coherency
	 */
	"	mrc	p15, 0, %0, c1, c0, 1\n"
	"	bic	%0, %0, %3\n"
	"	mcr	p15, 0, %0, c1, c0, 1\n"
	"	mrc	p15, 0, %0, c1, c0, 0\n"
	"	bic	%0, %0, %2\n"
	"	mcr	p15, 0, %0, c1, c0, 0\n"
	  : "=&r" (v)
	  : "r" (0), "Ir" (CR_C), "Ir" (0x40)
	  : "cc");
}

static inline void cpu_leave_lowpower(void)
{
	unsigned int v;

	asm volatile(
		"mrc	p15, 0, %0, c1, c0, 0\n"
	"	orr	%0, %0, %1\n"
	"	mcr	p15, 0, %0, c1, c0, 0\n"
	"	mrc	p15, 0, %0, c1, c0, 1\n"
	"	orr	%0, %0, %2\n"
	"	mcr	p15, 0, %0, c1, c0, 1\n"
	  : "=&r" (v)
	  : "Ir" (CR_C), "Ir" (0x40)
	  : "cc");
}

static inline void platform_do_lowpower(unsigned int cpu, int *spurious)
{
	/*
	 * there is no power-control hardware on this platform, so all
	 * we can do is put the core into WFI; this is safe as the calling
	 * code will have already disabled interrupts
	 */
	for (;;) {
		wfi();

		if (pen_release == cpu_logical_map(cpu)) {
			/*
			 * OK, proper wakeup, we're done
			 */
			break;
		}

		/*
		 * Getting here, means that we have come out of WFI without
		 * having been woken up - this shouldn't happen
		 *
		 * Just note it happening - when we're woken, we can report
		 * its occurrence.
		 */
		(*spurious)++;
	}
}

/*
 * platform-specific code to shutdown a CPU
 *
 * Called with IRQs disabled
 */
static void ax620x_cpu_die(unsigned int cpu)
{
	int spurious = 0;

	/*
	 * we're ready for shutdown now, so do it
	 */
	cpu_enter_lowpower();
	platform_do_lowpower(cpu, &spurious);

	/*
	 * bring this CPU back into the world of cache
	 * coherency, and then restore interrupts
	 */
	cpu_leave_lowpower();

	if (spurious)
		pr_warn("CPU%u: %u spurious wakeup calls\n", cpu, spurious);
}
#endif

const struct smp_operations ax620x_smp_ops __initconst = {
	.smp_init_cpus = ax620x_smp_init_cpus,
	.smp_prepare_cpus = ax620x_smp_prepare_cpus,
	.smp_boot_secondary = ax620x_smp_boot_secondary,
	.smp_secondary_init = ax620x_secondary_init,
#ifdef CONFIG_HOTPLUG_CPU
	.cpu_die = ax620x_cpu_die,
#endif
};

CPU_METHOD_OF_DECLARE(axera_smp_ax620x, "spin-table", &ax620x_smp_ops);
