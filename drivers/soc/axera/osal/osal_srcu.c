/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/

#include <linux/slab.h>
#include "osal_ax.h"
#include "osal_logdebug_ax.h"

#ifndef CONFIG_DEBUG_LOCK_ALLOC
int AX_OSAL_init_srcu_struct(AX_OSAL_srcu_struct_t *ssp)
{
	struct srcu_struct *p = NULL;
	int ret;
	if (ssp == NULL) {
		printk("%s error ssp == NULL\n", __func__);
		return -1;
	}
	p = (struct srcu_struct *)kmalloc(sizeof(struct srcu_struct), GFP_KERNEL);
	if (p == NULL) {
		printk("%s alloc srcu_struct failed\n", __func__);
		return -1;
	}
	ret = init_srcu_struct(p);
	if (ret != 0) {
		kfree(p);
		ssp->ssp = NULL;
		printk("%s init_srcu_struct failed\n", __func__);
		return -1;
	}
	ssp->ssp = p;

	return 0;
}
EXPORT_SYMBOL(AX_OSAL_init_srcu_struct);
#else
void *AX_OSAL_DBG_init_srcu_struct(AX_OSAL_srcu_struct_t *ssp)
{
	struct srcu_struct *p = NULL;
	if (ssp == NULL) {
		printk("%s error ssp == NULL\n", __func__);
		return NULL;
	}
	p = (struct srcu_struct *)kmalloc(sizeof(struct srcu_struct), GFP_KERNEL);
	if (p == NULL) {
		ssp->ssp = NULL;
		printk("%s alloc srcu_struct failed\n", __func__);
		return NULL;
	}
	ssp->ssp = p;

	return p;
}
EXPORT_SYMBOL(AX_OSAL_DBG_init_srcu_struct);
#endif

void AX_OSAL_cleanup_srcu_struct(AX_OSAL_srcu_struct_t *ssp)
{
	struct srcu_struct *p = NULL;
	if (ssp == NULL || ssp->ssp == NULL) {
		printk("%s error NULL param\n", __func__);
		return;
	}

	p = (struct srcu_struct *)ssp->ssp;
	cleanup_srcu_struct(p);
	kfree(ssp->ssp);
	ssp->ssp = NULL;
}
EXPORT_SYMBOL(AX_OSAL_cleanup_srcu_struct);

int AX_OSAL_srcu_read_lock(AX_OSAL_srcu_struct_t *ssp)
{
	struct srcu_struct *p = NULL;
	if (ssp == NULL || ssp->ssp == NULL) {
		printk("%s error NULL param\n", __func__);
		return -1;
	}

	p = (struct srcu_struct *)ssp->ssp;
	return srcu_read_lock(p);
}
EXPORT_SYMBOL(AX_OSAL_srcu_read_lock);

void AX_OSAL_srcu_read_unlock(AX_OSAL_srcu_struct_t *ssp, int idx)
{
	struct srcu_struct *p = NULL;
	if (ssp == NULL || ssp->ssp == NULL) {
		printk("%s error NULL param\n", __func__);
		return;
	}

	p = (struct srcu_struct *)ssp->ssp;
	srcu_read_unlock(p, idx);
}
EXPORT_SYMBOL(AX_OSAL_srcu_read_unlock);

void AX_OSAL_synchronize_srcu(AX_OSAL_srcu_struct_t *ssp)
{
	struct srcu_struct *p = NULL;
	if (ssp == NULL || ssp->ssp == NULL) {
		printk("%s error NULL param\n", __func__);
		return;
	}

	p = (struct srcu_struct *)ssp->ssp;
	synchronize_srcu(p);
}
EXPORT_SYMBOL(AX_OSAL_synchronize_srcu);
