/*
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 */

#ifndef __MACH_AXERA_CHIP_PM_H
#define __MACH_AXERA_CHIP_PM_H

void axera_chip_slp_cpu_resume(void);
#ifdef CONFIG_PM_SLEEP
void __init axera_chip_pm_init(void);
#else
static inline void axera_chip_pm_init(void)
{
}
#endif

#endif /* __MACH_AXERA_CHIP_PM_H */
