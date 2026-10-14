// SPDX-License-Identifier: GPL-2.0

#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/of_address.h>
#include <asm/smp_scu.h>
#include <asm/smp_plat.h>

#include "mach/common.h"

#define REG_PERI_CRG_BASE     0x12010000
#define REG_PERI_CRG30        (REG_PERI_CRG_BASE + 0x78)
#define REG_CPU_CORE_SRST     REG_PERI_CRG30
#define REG_SYS_CTRL_BASE     0x12020000
#define REG_SYS_CPU_64O32     (REG_SYS_CTRL_BASE + 0x800C)
#define REG_SYS_RVBAR_ADDR0   (REG_SYS_CTRL_BASE + 0x8010)
#define REG_SYS_RVBAR_ADDR1   (REG_SYS_CTRL_BASE + 0x8014)

#define SECONDARY_TIMEOUT_NS	NSEC_PER_MSEC	/* 1 msec (in nanoseconds) */
#define BOOT_ADDR_CPUID_MASK	0x1

#define SECONDARY_BOOT_ADDR 0

#ifdef CONFIG_SMP
void __init lotus_smp_prepare_cpus(unsigned int max_cpus)
{

}

static void set_secondary_boot_addr(phys_addr_t start_addr, phys_addr_t jump_addr)
{
	void __iomem *virt;

	virt = ioremap(start_addr, PAGE_SIZE);

	writel_relaxed(0xe51ff004, virt);	/* ldr pc, [pc, #-4] */
	writel_relaxed(jump_addr, virt + 4);	/* pc jump phy address */
	iounmap(virt);
}

int lotus_boot_secondary(unsigned int cpu, struct task_struct *idle)
{
	void __iomem *core_srst_reg, *cpu_32o64_reg;
	u32 cpu_id;
	u32 val;
	int ret = 0;

	cpu_id = cpu_logical_map(cpu);
	if (cpu_id & ~BOOT_ADDR_CPUID_MASK) {
		pr_err("bad cpu id (%u > %u)\n", cpu_id, BOOT_ADDR_CPUID_MASK);
		return -EINVAL;
	}

	cpu_32o64_reg = ioremap((phys_addr_t)REG_SYS_CPU_64O32,
				   sizeof(phys_addr_t));
	if (!cpu_32o64_reg) {
		pr_err("unable to map cpu 64 bit mode register for cpu %u\n", cpu_id);
		ret = -ENOMEM;
		goto out;
	}

	core_srst_reg = ioremap((phys_addr_t)REG_CPU_CORE_SRST,
				   sizeof(phys_addr_t));
	if (!core_srst_reg) {
		pr_err("unable to map core srst register for cpu %u\n", cpu_id);
		ret = -ENOMEM;
		goto out;
	}

	set_secondary_boot_addr(SECONDARY_BOOT_ADDR, __pa_symbol(secondary_startup));
	barrier();

	val = readl(cpu_32o64_reg);
	val &= ~((u32)(0x3 << 14));
	writel(val, cpu_32o64_reg);

	val = readl(core_srst_reg);
	val &= ~((u32)(0x1 << 1));
	writel(val, core_srst_reg);

	/* Make sure un-reset done before wakeing up slave core */
	smp_wmb();

	arch_send_wakeup_ipi_mask(cpumask_of(cpu));

out:
	iounmap(core_srst_reg);
	iounmap(cpu_32o64_reg);

	return ret;

}

static const struct smp_operations lotus_smp_ops __initconst = {
	.smp_prepare_cpus       = lotus_smp_prepare_cpus,
	.smp_boot_secondary     = lotus_boot_secondary,
};

CPU_METHOD_OF_DECLARE(x5720xxx_smp, "lotus,xmfalcon-smp", &lotus_smp_ops);
#endif /* CONFIG_SMP */
