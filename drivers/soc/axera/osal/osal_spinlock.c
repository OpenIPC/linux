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
#include <linux/spinlock.h>
#include <linux/slab.h>
#include "osal_logdebug_ax.h"
#include "osal_ax.h"


#ifndef CONFIG_DEBUG_SPINLOCK
int AX_OSAL_SYNC_spin_lock_init(AX_SPINLOCK_T * lock)
{
	spinlock_t *p = NULL;
	if (lock == NULL) {
		printk("%s error lock == NULL\n", __func__);
		return -1;
	}
	p = (spinlock_t *) kmalloc(sizeof(spinlock_t), GFP_KERNEL);
	if (p == NULL) {
		printk("%s alloc spinlock failed\n", __func__);
		return -1;
	}
	spin_lock_init(p);
	lock->lock = p;
	return 0;
}

EXPORT_SYMBOL(AX_OSAL_SYNC_spin_lock_init);

#else
spinlock_t *AX_OSAL_DBG_spin_lock_init(AX_SPINLOCK_T *lock)
{
	spinlock_t *p = NULL;
	if (lock == NULL) {
		printk("%s error lock == NULL\n", __func__);
		return NULL;
	}

	p = (spinlock_t *) kmalloc(sizeof(spinlock_t), GFP_KERNEL);
	if (p == NULL) {
		printk("%s alloc spinlock failed\n", __func__);
		return  NULL;
	}

	lock->lock = p;
	return p;
}

EXPORT_SYMBOL(AX_OSAL_DBG_spin_lock_init);
#endif


void AX_OSAL_SYNC_spin_lock(AX_SPINLOCK_T * lock)
{
	spinlock_t *p = NULL;

	p = (spinlock_t *) (lock->lock);
	spin_lock(p);
}

EXPORT_SYMBOL(AX_OSAL_SYNC_spin_lock);

int AX_OSAL_SYNC_spin_trylock(AX_SPINLOCK_T * lock)
{
	spinlock_t *p = NULL;
	if (lock == NULL) {
		printk("%s error lock == NULL\n", __func__);
		return -1;
	}
	p = (spinlock_t *) (lock->lock);
	return spin_trylock(p);
}

EXPORT_SYMBOL(AX_OSAL_SYNC_spin_trylock);

void AX_OSAL_SYNC_spin_unlock(AX_SPINLOCK_T * lock)
{
	spinlock_t *p = NULL;

	p = (spinlock_t *) (lock->lock);
	spin_unlock(p);
}

EXPORT_SYMBOL(AX_OSAL_SYNC_spin_unlock);

void AX_OSAL_SYNC_spin_lock_irqsave(AX_SPINLOCK_T * lock, u32 * flags)
{
	spinlock_t *p = NULL;
	unsigned long f;

	p = (spinlock_t *) (lock->lock);
	spin_lock_irqsave(p, f);
	*flags = f;
}

EXPORT_SYMBOL(AX_OSAL_SYNC_spin_lock_irqsave);

void AX_OSAL_SYNC_spin_unlock_irqrestore(AX_SPINLOCK_T * lock, u32 * flags)
{
	spinlock_t *p = NULL;
	unsigned long f;

	p = (spinlock_t *) (lock->lock);
	f = *flags;
	spin_unlock_irqrestore(p, f);
}

EXPORT_SYMBOL(AX_OSAL_SYNC_spin_unlock_irqrestore);

void AX_OSAL_SYNC_spinLock_destory(AX_SPINLOCK_T * lock)
{
	spinlock_t *p = NULL;
	p = (spinlock_t *) (lock->lock);
	kfree(p);
	lock->lock = NULL;
}

EXPORT_SYMBOL(AX_OSAL_SYNC_spinLock_destory);
