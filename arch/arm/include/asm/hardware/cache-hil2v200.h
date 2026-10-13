/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef __ASM_ARM_HARDWARE_CACHE_HIL2V200_H
#define __ASM_ARM_HARDWARE_CACHE_HIL2V200_H

#include <linux/errno.h>
#include <linux/init.h>

#ifdef CONFIG_CACHE_HIL2V200
int __init hil2v200_of_init(void);
#else
static inline int hil2v200_of_init(void) { return -ENODEV; }
#endif

#endif
