/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (c) 2015 HiSilicon Technologies Co., Ltd.
 */

#ifndef	__HISI_RESET_H
#define	__HISI_RESET_H

#include <linux/err.h>
#include <linux/spinlock_types.h>

struct device_node;
struct hisi_reset_controller;

#ifdef CONFIG_RESET_CONTROLLER
struct hisi_reset_controller *hisi_reset_init(struct platform_device *pdev);
struct hisi_reset_controller *hisi_reset_init_shared(struct platform_device *pdev,
						     spinlock_t *lock);
void hisi_reset_exit(struct hisi_reset_controller *rstc);
#else
static inline
struct hisi_reset_controller *hisi_reset_init(struct platform_device *pdev)
{
	return NULL;
}

static inline
struct hisi_reset_controller *hisi_reset_init_shared(struct platform_device *pdev,
						     spinlock_t *lock)
{
	return ERR_PTR(-ENODEV);
}

static inline void hisi_reset_exit(struct hisi_reset_controller *rstc)
{}
#endif

#endif	/* __HISI_RESET_H */

#ifdef CONFIG_ARCH_HISI_BVT
int hibvt_reset_init(struct device_node *np, int nr_rsts);
#endif
