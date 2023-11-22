/*
 * irq.h- Sigmastar
 *
 * Copyright (c) [2019~2020] SigmaStar Technology.
 *
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License version 2 for more details.
 *
 */

#ifndef __ASM_ARM_MACH_IRQ_H
#define __ASM_ARM_MACH_IRQ_H

#include <linux/irq.h>

struct seq_file;

/*
 * This is internal.  Do not use it.
 */
extern void init_FIQ(int);
extern int  show_fiq_list(struct seq_file *, int);

/*
 * This is for easy migration, but should be changed in the source
 */
#define do_bad_IRQ(desc)              \
    do                                \
    {                                 \
        raw_spin_lock(&desc->lock);   \
        handle_bad_irq(desc);         \
        raw_spin_unlock(&desc->lock); \
    } while (0)

#endif
