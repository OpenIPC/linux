/*
 * Copyright (c) XMEDIA. All rights reserved.
 */
#ifndef __SMP_COMMON_H
#define __SMP_COMMON_H

#ifdef CONFIG_SMP
void bsp_set_cpu(unsigned int cpu, bool enable);
void __init bsp_smp_prepare_cpus(unsigned int max_cpus);
int bsp_boot_secondary(unsigned int cpu, struct task_struct *idle);
#endif /* CONFIG_SMP */
#endif /* __SMP_COMMON_H */
