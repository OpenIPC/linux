/**
    NVT common header
 
    @file       common.h
    @ingroup
    @note
    Copyright   Novatek Microelectronics Corp. 2019.  All rights reserved.

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2 as
    published by the Free Software Foundation.
*/

#ifndef __ARCH_ARM_MACH_NVT_IVOT_COMMON_H
#define __ARCH_ARM_MACH_NVT_IVOT_COMMON_H
#include <linux/irq.h>

void __init nvt_ivot_map_io(void);
#ifdef CONFIG_TEE
void nvt_smc_l2_write_sec(unsigned long val, unsigned reg);
#endif
#ifdef CONFIG_SMP
void nvt_secondary_startup(void);
extern struct smp_operations nvt_smp_ops;
extern struct smp_operations nvt_ca9_smp_ops;
#endif
#ifdef CONFIG_HOTPLUG_CPU
extern void nvt_cpu_die(unsigned int cpu);
#endif
#endif /* __ARCH_ARM_MACH_NVT_IVOT_COMMON_H */
