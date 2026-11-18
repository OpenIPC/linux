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
#include <linux/mutex.h>
#include <linux/slab.h>
#include "osal_logdebug_ax.h"
#include <linux/rtmutex.h>
#include "osal_ax.h"

#ifndef CONFIG_DEBUG_MUTEXES
int AX_OSAL_SYNC_mutex_init(AX_MUTEX_T * mutex)
{
	struct mutex *p = NULL;
	if (mutex == NULL) {
		printk("%s error mutex == NULL\n", __func__);
		return -1;
	}
	p = kmalloc(sizeof(struct mutex), GFP_KERNEL);
	if (p == NULL) {
		printk("%s error mutex == NULL\n", __func__);
		return -1;
	}
	mutex_init(p);
	mutex->mutex = p;
	return 0;
}

EXPORT_SYMBOL(AX_OSAL_SYNC_mutex_init);
#else
void *AX_OSAL_DBG_mutex_init(AX_MUTEX_T * mutex)
{
	struct mutex *p = NULL;
	if (mutex == NULL) {
		printk("%s error mutex == NULL\n", __func__);
		return NULL;
	}
	p = kmalloc(sizeof(struct mutex), GFP_KERNEL);
	if (p == NULL) {
		printk("%s error mutex == NULL\n", __func__);
		return NULL;
	}

	mutex->mutex = p;
	return p;
}

EXPORT_SYMBOL(AX_OSAL_DBG_mutex_init);
#endif

int AX_OSAL_SYNC_mutex_lock(AX_MUTEX_T * mutex)
{
	struct mutex *p = NULL;
	if (mutex == NULL) {
		printk("%s error mutex == NULL\n", __func__);
		return -1;
	}
	p = (struct mutex *)(mutex->mutex);
	mutex_lock(p);
	return 0;
}

EXPORT_SYMBOL(AX_OSAL_SYNC_mutex_lock);

int AX_OSAL_SYNC_mutex_lock_interruptible(AX_MUTEX_T * mutex)
{
	struct mutex *p = NULL;
	if (mutex == NULL) {
		printk("%s error mutex == NULL\n", __func__);
		return -1;
	}
	p = (struct mutex *)(mutex->mutex);
	return mutex_lock_interruptible(p);
}

EXPORT_SYMBOL(AX_OSAL_SYNC_mutex_lock_interruptible);

int AX_OSAL_SYNC_mutex_trylock(AX_MUTEX_T * mutex)
{
	struct mutex *p = NULL;
	if (mutex == NULL) {
		printk("%s error mutex == NULL\n", __func__);
		return -1;
	}
	p = (struct mutex *)(mutex->mutex);

	return mutex_trylock(p);
}

EXPORT_SYMBOL(AX_OSAL_SYNC_mutex_trylock);

void AX_OSAL_SYNC_mutex_unlock(AX_MUTEX_T * mutex)
{
	struct mutex *p = NULL;
	p = (struct mutex *)(mutex->mutex);

	mutex_unlock(p);
}

EXPORT_SYMBOL(AX_OSAL_SYNC_mutex_unlock);

void AX_OSAL_SYNC_mutex_destroy(AX_MUTEX_T * mutex)
{
	struct mutex *p = NULL;
	p = (struct mutex *)(mutex->mutex);
	kfree(p);
	mutex->mutex = NULL;
}

EXPORT_SYMBOL(AX_OSAL_SYNC_mutex_destroy);

#ifdef CONFIG_DEBUG_RT_MUTEXES
int AX_OSAL_DBG_rt_mutex_init(AX_RT_MUTEX_T * rt_mutex,struct lock_class_key *__key)
{
	struct rt_mutex *p = NULL;
	if (rt_mutex == NULL) {
        	printk("%s error rt mutex == NULL\n", __func__);
		return -1;
	}
	p = kmalloc(sizeof(struct rt_mutex), GFP_KERNEL);
	if (p == NULL) {
        	printk("%s error mutex == NULL\n", __func__);
		return -1;
	}

	rt_mutex->rt_mutex = p;
	__rt_mutex_init(p, __func__,__key);
	return 0;
}
EXPORT_SYMBOL(AX_OSAL_DBG_rt_mutex_init);
#else
int AX_OSAL_SYNC_rt_mutex_init(AX_RT_MUTEX_T * rt_mutex)
{
	struct rt_mutex *p = NULL;
	if (rt_mutex == NULL) {
        	printk("%s error rt mutex == NULL\n", __func__);
		return -1;
	}
	p = kmalloc(sizeof(struct rt_mutex), GFP_KERNEL);
	if (p == NULL) {
        	printk("%s error mutex == NULL\n", __func__);
		return -1;
	}

	rt_mutex->rt_mutex = p;
	__rt_mutex_init(p, NULL, NULL);
	return 0;
}
EXPORT_SYMBOL(AX_OSAL_SYNC_rt_mutex_init);
#endif

void AX_OSAL_SYNC_rt_mutex_lock(AX_RT_MUTEX_T * rt_mutex)
{
	struct rt_mutex *p = NULL;
	p = (struct rt_mutex *)(rt_mutex->rt_mutex);
	rt_mutex_lock(p);
}
EXPORT_SYMBOL(AX_OSAL_SYNC_rt_mutex_lock);

void AX_OSAL_SYNC_rt_mutex_unlock(AX_RT_MUTEX_T * rt_mutex)
{
	struct rt_mutex *p = NULL;
	p = (struct rt_mutex *)(rt_mutex->rt_mutex);
	rt_mutex_unlock(p);
}
EXPORT_SYMBOL(AX_OSAL_SYNC_rt_mutex_unlock);

void AX_OSAL_SYNC_rt_mutex_destroy(AX_RT_MUTEX_T * rt_mutex)
{
	struct rt_mutex *p = NULL;
	p = (struct rt_mutex *)(rt_mutex->rt_mutex);
#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 15, 0))
	rt_mutex_destroy(p);
#endif
	kfree(p);
	rt_mutex->rt_mutex = NULL;
}
EXPORT_SYMBOL(AX_OSAL_SYNC_rt_mutex_destroy);

