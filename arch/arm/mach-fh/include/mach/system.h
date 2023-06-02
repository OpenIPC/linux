/*
 * mach/system.h
 *
 * Copyright (C) 2014 Fullhan Microelectronics Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */
#ifndef __ASM_ARCH_SYSTEM_H
#define __ASM_ARCH_SYSTEM_H

#include <mach/io.h>
#include <mach/fh_predefined.h>
#include <mach/pmu.h>
#include <linux/compiler.h>
#include <linux/types.h>
#include <asm/mach/time.h>
#include <asm/proc-fns.h>

extern void fh_intc_init(void);

void fh_irq_suspend(void);
void fh_irq_resume(void);

extern unsigned int fh_cpu_suspend_sz;
extern void fh_cpu_suspend(void);

static inline void arch_idle(void)
{

}

static inline void arch_reset(char mode, const char *cmd)
{
	fh_pmu_set_reg(REG_PMU_SWRST_MAIN_CTRL, 0x7fffffff);
}

/*change the scheduling policy and/or RT priority of `current` thread. */
extern void fh_setscheduler(int policy, int priority);

#endif /* __ASM_ARCH_SYSTEM_H */
