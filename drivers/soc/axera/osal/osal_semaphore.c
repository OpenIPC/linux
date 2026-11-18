/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/printk.h>
#include <linux/semaphore.h>
#include <linux/slab.h>
#include "osal_logdebug_ax.h"
#include "osal_ax.h"

int AX_OSAL_SYNC_sema_init(AX_SEMAPHORE_T * sem, int val)
{
	struct semaphore *p = NULL;
	if (sem == NULL) {
		printk("%s error sem == NULL\n", __func__);
		return -1;
	}
	p = kmalloc(sizeof(struct semaphore), GFP_KERNEL);
	if (p == NULL) {
		printk("%s alloc semaphore failed\n", __func__);
		return -1;
	}
	sema_init(p, val);
	sem->sem = p;
	return 0;
}

EXPORT_SYMBOL(AX_OSAL_SYNC_sema_init);

int AX_OSAL_SYNC_sema_down_interruptible(AX_SEMAPHORE_T * sem)
{
	struct semaphore *p = NULL;
	if (sem == NULL) {
		printk("%s error sem == NULL\n", __func__);
		return -1;
	}
	p = (struct semaphore *)(sem->sem);
	return down_interruptible(p);
}

EXPORT_SYMBOL(AX_OSAL_SYNC_sema_down_interruptible);

int AX_OSAL_SYNC_sema_down(AX_SEMAPHORE_T * sem)
{
	struct semaphore *p = NULL;
	if (sem == NULL) {
		printk("%s error sem == NULL\n", __func__);
		return -1;
	}
	p = (struct semaphore *)(sem->sem);
	down(p);
	return 0;
}

EXPORT_SYMBOL(AX_OSAL_SYNC_sema_down);

int AX_OSAL_SYNC_sema_down_timeout(AX_SEMAPHORE_T * sem, long timeout)
{
	struct semaphore *p = NULL;
	if (sem == NULL) {
		printk("%s error sem == NULL\n", __func__);
		return -1;
	}
	p = (struct semaphore *)(sem->sem);
	return down_timeout(p, timeout);
}

EXPORT_SYMBOL(AX_OSAL_SYNC_sema_down_timeout);

int AX_OSAL_SYNC_sema_down_trylock(AX_SEMAPHORE_T * sem)
{
	struct semaphore *p = NULL;
	if (sem == NULL) {
		printk("%s error sem == NULL\n", __func__);
		return -1;
	}
	p = (struct semaphore *)(sem->sem);
	return down_trylock(p);
}

EXPORT_SYMBOL(AX_OSAL_SYNC_sema_down_trylock);

void AX_OSAL_SYNC_sema_up(AX_SEMAPHORE_T * sem)
{
	struct semaphore *p = NULL;
	p = (struct semaphore *)(sem->sem);
	up(p);
}

EXPORT_SYMBOL(AX_OSAL_SYNC_sema_up);

void AX_OSAL_SYNC_sema_destroy(AX_SEMAPHORE_T * sem)
{
	struct semaphore *p = NULL;
	p = (struct semaphore *)(sem->sem);
	kfree(p);
	sem->sem = NULL;
}

EXPORT_SYMBOL(AX_OSAL_SYNC_sema_destroy);
