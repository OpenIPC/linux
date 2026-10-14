/* SPDX-License-Identifier: GPL-2.0 */

#ifndef __SMP_COMMON_H
#define __SMP_COMMON_H

enum {
	INIT_TYPE = 0,
	DDR3_TYPE = 1,
	DDR4_TYPE = 2,
	LPDDR4_TYPE = 3,
	UNKNOWN_DDR_TYPE = 0xff
};

void calc_bandwidth_util(int numerator, int denominator, int *integer, int *decimal);
char *get_ddr_type_string(int *length);
int get_ddr_bandwidth_util(u32 time_ms, int *integer, int *fract);

int ddrc_init(void);
void ddrc_exit(void);

#ifdef CONFIG_SMP
void lotus_set_cpu(unsigned int cpu, bool enable);
void __init lotus_smp_prepare_cpus(unsigned int max_cpus);
int lotus_boot_secondary(unsigned int cpu, struct task_struct *idle);
#endif /* CONFIG_SMP */
#endif /* __SMP_COMMON_H */
