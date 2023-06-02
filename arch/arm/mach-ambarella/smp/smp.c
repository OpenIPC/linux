/*
 * arch/arm/plat-ambarella/smp/smp.c
 *
 * Author: Anthony Ginger <hfjiang@ambarella.com>
 *
 * Copyright (C) 2004-2010, Ambarella, Inc.
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
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 */

#include <linux/init.h>
#include <linux/errno.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/jiffies.h>
#include <linux/smp.h>
#include <linux/io.h>
#include <linux/bootmem.h>
#include <linux/of.h>
#include <linux/of_fdt.h>

#include <asm/cacheflush.h>
#include <asm/smp_plat.h>
#include <asm/smp_scu.h>
#include <mach/hardware.h>
#include <mach/common.h>
#include <mach/init.h>

static void __iomem *scu_base = __io(AMBARELLA_VA_SCU_BASE);
static DEFINE_SPINLOCK(boot_lock);

static u32 *cpux_jump_virt = NULL;
extern void ambarella_secondary_startup(void);
extern void ambvic_smp_softirq_init(void);


/* Write pen_release in a way that is guaranteed to be visible to all
 * observers, irrespective of whether they're taking part in coherency
 * or not.  This is necessary for the hotplug code to work reliably. */
static void write_pen_release(int val)
{
	pen_release = val;
	smp_wmb();
	__cpuc_flush_dcache_area((void *)&pen_release, sizeof(pen_release));
	outer_clean_range(__pa(&pen_release), __pa(&pen_release + 1));
}

static void write_cpux_jump_addr(unsigned int cpu, int addr)
{
	cpux_jump_virt[cpu] = addr;
	smp_wmb();
	__cpuc_flush_dcache_area(
		&cpux_jump_virt[cpu], sizeof(cpux_jump_virt[cpu]));
	outer_clean_range(ambarella_virt_to_phys((u32)&cpux_jump_virt[cpu]),
			ambarella_virt_to_phys((u32)&cpux_jump_virt[cpu] + 1));
}

/* running on CPU1 */
static void __cpuinit ambarella_smp_secondary_init(unsigned int cpu)
{
	/* let the primary processor know we're out of the
	 * pen, then head off into the C entry point */
	write_pen_release(-1);

	/* Synchronise with the boot thread. */
	spin_lock(&boot_lock);
	spin_unlock(&boot_lock);
}
/* running on CPU0 */
static int __cpuinit ambarella_smp_boot_secondary(unsigned int cpu,
	struct task_struct *idle)
{
	unsigned long timeout;
	unsigned long phys_cpu = cpu_logical_map(cpu);

	BUG_ON(cpux_jump_virt == NULL);

	scu_enable(scu_base);

	/* Set synchronisation state between this boot processor
	 * and the secondary one */
	spin_lock(&boot_lock);

	/* The secondary processor is waiting to be released from
	 * the holding pen - release it, then wait for it to flag
	 * that it has been released by resetting pen_release.
	 *
	 * Note that "pen_release" is the hardware CPU ID, whereas
	 * "cpu" is Linux's internal ID. */
	write_pen_release(phys_cpu);

	write_cpux_jump_addr(cpu, virt_to_phys(ambarella_secondary_startup));

#ifdef CONFIG_PLAT_AMBARELLA_SUPPORT_VIC
	/* IPI interrupt on CPU1 may be unmasked, so this init is necessary */
	ambvic_smp_softirq_init();
#endif

	/* Send the secondary CPU a soft interrupt, thereby causing
	 * the boot monitor to read the system wide flags register,
	 * and branch to the address found there. */
	timeout = jiffies + (1 * HZ);
	while (time_before(jiffies, timeout)) {
		smp_rmb();

		arch_send_wakeup_ipi_mask(cpumask_of(cpu));

		if (pen_release == -1)
			break;

		udelay(10);
	}

	spin_unlock(&boot_lock);

	return pen_release != -1 ? -ENOSYS : 0;
}

/* running on CPU0 */
static void __init ambarella_smp_init_cpus(void)
{
	int i;
	unsigned int ncores;

	ncores = scu_get_core_count(scu_base);
	if (ncores > nr_cpu_ids) {
		pr_warning("SMP: cores(%u) greater than maximum(%u), clipping\n",
			ncores, nr_cpu_ids);
		ncores = nr_cpu_ids;
	}

	for (i = 0; i < ncores; i++)
		set_cpu_possible(i, true);
}

/* running on CPU0 */
static void __init ambarella_smp_prepare_cpus(unsigned int max_cpus)
{
	u32 cpux_jump, start_limit, end_limit;
	int i, rval;

	rval = of_property_read_u32(of_chosen, "ambarella,cpux_jump", &cpux_jump);
	if (rval < 0) {
		pr_err("No jump address for secondary cpu!\n");
		return;
	}

	start_limit = get_ambarella_ppm_phys();
	end_limit = get_ambarella_ppm_phys() + get_ambarella_ppm_size();
	if (cpux_jump < start_limit || cpux_jump > end_limit) {
		pr_err("Invalid secondary cpu jump address, 0x%08x!\n", cpux_jump);
		return;
	}

	cpux_jump_virt = (u32 *)ambarella_phys_to_virt(cpux_jump);

	for (i = 0; i < max_cpus; i++)
		set_cpu_present(i, true);

	scu_enable(scu_base);

	for (i = 1; i < max_cpus; i++)
		write_cpux_jump_addr(i, virt_to_phys(ambarella_secondary_startup));
}

#ifdef CONFIG_HOTPLUG_CPU
static inline void cpu_enter_lowpower(void)
{
	unsigned int v;

	flush_cache_all();
	asm volatile(
	"	mrc	p15, 0, %0, c1, c0, 1\n"
	"	bic	%0, %0, #(1 << 6)\n"
	"	bic	%0, %0, #(1 << 0)\n"
	"	mcr	p15, 0, %0, c1, c0, 1\n"
	"	mrc	p15, 0, %0, c1, c0, 0\n"
	"	bic	%0, %0, #(1 << 2)\n"
	"	mcr	p15, 0, %0, c1, c0, 0\n"
		: "=&r" (v)
		: "r" (0)
		: "cc");
}

static inline void cpu_leave_lowpower(void)
{
	unsigned int v;

	asm volatile(
	"	mrc	p15, 0, %0, c1, c0, 0\n"
	"	orr	%0, %0, #(1 << 2)\n"
	"	mcr	p15, 0, %0, c1, c0, 0\n"
	"	mrc	p15, 0, %0, c1, c0, 1\n"
	"	orr	%0, %0, #(1 << 6)\n"
	"	orr	%0, %0, #(1 << 0)\n"
	"	mcr	p15, 0, %0, c1, c0, 1\n"
		: "=&r" (v)
		:
		: "cc");
}

static inline void platform_do_lowpower(unsigned int cpu, int *spurious)
{
	for (;;) {
		wfi();

		if (pen_release == cpu_logical_map(cpu)) {
			/* OK, proper wakeup, we're done */
			break;
		}

		/* Getting here, means that we have come out of WFI without
		 * having been woken up - this shouldn't happen
		 *
		 * Just note it happening - when we're woken, we can report
		 * its occurrence. */
		(*spurious)++;
	}
}

/* running on CPU1 */
static void ambarella_smp_cpu_die(unsigned int cpu)
{
	int spurious = 0;

	cpu_enter_lowpower();

	platform_do_lowpower(cpu, &spurious);

	cpu_leave_lowpower();

	if (spurious)
		pr_warn("CPU%u: %u spurious wakeup calls\n", cpu, spurious);
}

/* running on CPU1 */
static int ambarella_smp_cpu_disable(unsigned int cpu)
{
	return cpu == 0 ? -EPERM : 0;
}
#endif

struct smp_operations ambarella_smp_ops __initdata = {
	.smp_init_cpus		= ambarella_smp_init_cpus,
	.smp_prepare_cpus	= ambarella_smp_prepare_cpus,
	.smp_boot_secondary	= ambarella_smp_boot_secondary,
	.smp_secondary_init	= ambarella_smp_secondary_init,
#ifdef CONFIG_HOTPLUG_CPU
	.cpu_disable		= ambarella_smp_cpu_disable,
	.cpu_die		= ambarella_smp_cpu_die,
#endif
};

