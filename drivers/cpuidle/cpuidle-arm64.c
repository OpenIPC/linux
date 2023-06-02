/*
 * ARM64 generic CPU idle driver.
 *
 * Copyright (C) 2014 ARM Ltd.
 * Author: Lorenzo Pieralisi <lorenzo.pieralisi@arm.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#define pr_fmt(fmt) "CPUidle arm64: " fmt

#include <linux/cpuidle.h>
#include <linux/cpumask.h>
#include <linux/cpu_pm.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/of.h>

#include <asm/io.h>
#include <asm/cpuidle.h>
#include <asm/suspend.h>

#include "dt_idle_states.h"

//#define ARM64_PMC_PWRDN_DEBUG

#define CPU_PMC_BASE                    0x1D820000
/* TODO: should be adapted for boot form cluster1(cpu2). should use mpidr. */
#define PMC_CORE_PWRDN_REQ(cpu)   	 	((cpu) << 8) 		  /* core power down request */
#define PMC_CORE_PWRDN_MODE(cpu)   	 	(((cpu) << 8) + 0x4)  /* core power down mode */
#define PMC_CORE_PWRDN_PARAM(cpu)   	(((cpu) << 8) + 0x8)  /* core power down param */
#define PMC_CORE_PWRDN_CNT_EN(cpu)   	(((cpu) << 8) + 0xc)  /* core power down/on counter enable */
#define PMC_CORE_PWRDN_CNT(cpu)   	 	(((cpu) << 8) + 0x10) /* core power down time */
#define PMC_CORE_PWRON_CNT(cpu)   	 	(((cpu) << 8) + 0x14) /* core power on time */
#define PMC_CORE_PWRDN_DONE_CNT(cpu)   	(((cpu) << 8) + 0x18) /* core power off time */
#define PMC_CORE_PWR_STATUS(cpu)   	 	(((cpu) << 8) + 0x1c) /* core power status */

#define PMC_CLUSTER_PWRDN_REQ(cls)      (0x1000 + ((cls) << 8))         /* cluster power down request */
#define PMC_CLUSTER_PWRDN_MODE(cls)   	((0x1000 + ((cls) << 8)) + 0x4) /* cluster power down mode */
#define PMC_CLUSTER_PWRDN_PARAM(cls)   	((0x1000 + ((cls) << 8)) + 0x8) /* cluster power down param */
#define PMC_CLUSTER_PWR_STATUS(cls)   	((0x1000 + ((cls) << 8)) + 0xc) /* cluster power status */
#define PMC_CLUSTER_PWRDN_CNT_EN(cls)   ((0x1000 + ((cls) << 8)) + 0x10) /* cluster power down/on counter enable */
#define PMC_CLUSTER_PWRDN_CNT(cls)      ((0x1000 + ((cls) << 8)) + 0x14) /* cluster power down time */
#define PMC_CLUSTER_PWRON_CNT(cls)      ((0x1000 + ((cls) << 8)) + 0x18) /* cluster power on time */
#define PMC_CLUSTER_PWRDN_DONE_CNT(cls) ((0x1000 + ((cls) << 8)) + 0x1c) /* cluster power off time */

#ifdef ARM64_PMC_PWRDN_DEBUG
static void arm64_cpu_pmc_display_counter(void)
{
	int cpu = smp_processor_id();
	unsigned int cluster = cpu >> 1;
	unsigned int cpu_pwrdn_cnt;
	unsigned int cpu_pwron_cnt;
	unsigned int cpu_pwrdn_done_cnt;
	unsigned int cluster_pwrdn_cnt = 0;
	unsigned int cluster_pwron_cnt = 0;
	unsigned int cluster_pwrdn_done_cnt = 0;
	static int counter_cpu[4];
	static int counter_cluster[4];
	void __iomem *pmc_virt = ioremap(CPU_PMC_BASE, PAGE_SIZE * 2);

	writel(0x0, pmc_virt + PMC_CORE_PWRDN_CNT_EN(cpu)); /* disable cpu power on/down counter */

	cpu_pwrdn_cnt = readl(pmc_virt + PMC_CORE_PWRDN_CNT(cpu));
	cpu_pwron_cnt = readl(pmc_virt + PMC_CORE_PWRON_CNT(cpu));
	cpu_pwrdn_done_cnt = readl(pmc_virt + PMC_CORE_PWRDN_DONE_CNT(cpu));
	
	writel(0x0, pmc_virt + PMC_CORE_PWRDN_CNT(cpu)); /* clean counter */
	writel(0x0, pmc_virt + PMC_CORE_PWRON_CNT(cpu));
	writel(0x0, pmc_virt + PMC_CORE_PWRDN_DONE_CNT(cpu));

	if (counter_cpu[cpu % 4] % 2000 == 0) {
		printk("[%s][%d]     cpu[%d] pwrdn_cnt[%8d]us pwron_cnt[%8d]us pwrdn_done_cnt[%8d]us count[%8d][%8d][%8d][%8d]\n", __func__, __LINE__, 
			cpu, cpu_pwrdn_cnt * 5, cpu_pwron_cnt * 5, cpu_pwrdn_done_cnt * 5,
			counter_cpu[0], counter_cpu[1], counter_cpu[2], counter_cpu[3]);
	}
	counter_cpu[cpu % 4]++;

	if (readl(pmc_virt + PMC_CLUSTER_PWRDN_CNT_EN(cluster))) {
		writel(0x0, pmc_virt + PMC_CLUSTER_PWRDN_CNT_EN(cluster)); /* disable cluster power on/down counter */
		cluster_pwrdn_cnt = readl(pmc_virt + PMC_CLUSTER_PWRDN_CNT(cluster));
		cluster_pwron_cnt = readl(pmc_virt + PMC_CLUSTER_PWRON_CNT(cluster));
		cluster_pwrdn_done_cnt = readl(pmc_virt + PMC_CLUSTER_PWRDN_DONE_CNT(cluster));
		
		writel(0x0, pmc_virt + PMC_CLUSTER_PWRDN_CNT(cluster)); /* clean counter */
		writel(0x0, pmc_virt + PMC_CLUSTER_PWRON_CNT(cluster)); 
		writel(0x0, pmc_virt + PMC_CLUSTER_PWRDN_DONE_CNT(cluster)); 

		if (counter_cluster[cluster % 2] % 2000 == 0) {
			printk("[%s][%d] cluster[%d] pwrdn_cnt[%8d]us pwron_cnt[%8d]us pwrdn_done_cnt[%8d]us count[%8d][%8d]\n", __func__, __LINE__, 
				cluster, cluster_pwrdn_cnt * 5, cluster_pwron_cnt * 5, cluster_pwrdn_done_cnt * 5,
				counter_cluster[0], counter_cluster[1]);
		}
		counter_cluster[cluster % 2]++;
	}

	iounmap(pmc_virt);
}
#endif


/*
 * arm64_enter_idle_state - Programs CPU to enter the specified state
 *
 * dev: cpuidle device
 * drv: cpuidle driver
 * idx: state index
 *
 * Called from the CPUidle framework to program the device to the
 * specified target state selected by the governor.
 */
static int arm64_enter_idle_state(struct cpuidle_device *dev,
				  struct cpuidle_driver *drv, int idx)
{
	int ret;
	
	if (!idx) {
		cpu_do_idle();
		return idx;
	}

	ret = cpu_pm_enter();
	if (!ret) {
		/*
		 * Pass idle state index to cpu_suspend which in turn will
		 * call the CPU ops suspend protocol with idle index as a
		 * parameter.
		 */
		ret = arm_cpuidle_suspend(idx);

		cpu_pm_exit();

#ifdef ARM64_PMC_PWRDN_DEBUG
		arm64_cpu_pmc_display_counter();
#endif
	}

	return ret ? -1 : idx;
}

static struct cpuidle_driver arm64_idle_driver = {
	.name = "arm64_idle",
	.owner = THIS_MODULE,
	/*
	 * State at index 0 is standby wfi and considered standard
	 * on all ARM platforms. If in some platforms simple wfi
	 * can't be used as "state 0", DT bindings must be implemented
	 * to work around this issue and allow installing a special
	 * handler for idle state index 0.
	 */
	.states[0] = {
		.enter                  = arm64_enter_idle_state,
		.exit_latency           = 1,
		.target_residency       = 1,
		.power_usage		= UINT_MAX,
		.flags                  = CPUIDLE_FLAG_TIME_VALID,
		.name                   = "WFI",
		.desc                   = "ARM64 WFI",
	},
};

static const struct of_device_id arm64_idle_state_match[] __initconst = {
	{ .compatible = "arm,idle-state",
	  .data = arm64_enter_idle_state },
	{ },
};

static void arm64_cpu_pmc_init(int cpu)
{
	void __iomem *pmc_virt = ioremap(CPU_PMC_BASE, PAGE_SIZE * 2);
	unsigned int cluster = cpu >> 1;
	unsigned int cpu_param = readl(pmc_virt + PMC_CORE_PWRDN_PARAM(cpu));
	unsigned int cluster_param = readl(pmc_virt + PMC_CLUSTER_PWRDN_PARAM(cluster));

	printk("%s cpu:%d enter\n", __func__, cpu);

	/* cpu */
	cpu_param = (cpu_param & 0xffff) | 0x80800000; 
	writel(cpu_param, pmc_virt + PMC_CORE_PWRDN_PARAM(cpu)); /* power down/on wait 128ns */
	writel(readl(pmc_virt + PMC_CORE_PWRDN_MODE(cpu)) | 0x1, pmc_virt + PMC_CORE_PWRDN_MODE(cpu)); /* cpu auto power off mode */

	/* cluster */
	cluster_param = (cluster_param & 0xffff) | 0x01010000;  
	writel(cluster_param, pmc_virt + PMC_CLUSTER_PWRDN_PARAM(cluster)); /* power down/on wait 100ns */
	writel(0x0, pmc_virt + PMC_CLUSTER_PWRDN_CNT(cluster)); /* clean counter */
	writel(readl(pmc_virt + PMC_CLUSTER_PWRDN_MODE(cluster)) | 0x1, pmc_virt + PMC_CLUSTER_PWRDN_MODE(cluster)); /* cluster auto power off mode  */

	iounmap(pmc_virt);
}

/*
 * arm64_idle_init
 *
 * Registers the arm64 specific cpuidle driver with the cpuidle
 * framework. It relies on core code to parse the idle states
 * and initialize them using driver data structures accordingly.
 */
static int __init arm64_idle_init(void)
{
	int cpu, ret;
	struct cpuidle_driver *drv = &arm64_idle_driver;

	/*
	 * Initialize idle states data, starting at index 1.
	 * This driver is DT only, if no DT idle states are detected (ret == 0)
	 * let the driver initialization fail accordingly since there is no
	 * reason to initialize the idle driver if only wfi is supported.
	 */
	ret = dt_init_idle_driver(drv, arm64_idle_state_match, 1);
	if (ret <= 0) {
		if (ret)
			pr_err("failed to initialize idle states\n");
		return ret ? : -ENODEV;
	}

	/*
	 * Call arch CPU operations in order to initialize
	 * idle states suspend back-end specific data
	 */
	for_each_possible_cpu(cpu) {

		ret = arm_cpuidle_init(cpu);
		if (ret) {
			pr_err("CPU %d failed to init idle CPU ops\n", cpu);
			return ret;
		}

		arm64_cpu_pmc_init(cpu);
	}

	ret = cpuidle_register(drv, NULL);
	if (ret) {
		pr_err("failed to register cpuidle driver\n");
		return ret;
	}

	return 0;
}
device_initcall(arm64_idle_init);
