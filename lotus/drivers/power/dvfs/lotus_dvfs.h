/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) LOTUS. All rights reserved.
 */

#ifndef __LOTUS_DVFS_H__
#define __LOTUS_DVFS_H__

void lotus_cpufreq_set_volt(unsigned long freq);
void lotus_init_dvfs_volt(void);
void lotus_exit_dvfs_volt(void);

#endif /* __LOTUS_DVFS_H__ */
