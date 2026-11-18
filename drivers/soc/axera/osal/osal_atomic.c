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
#include <asm/atomic.h>
#include <linux/printk.h>
#include <linux/slab.h>
#include <linux/version.h>
#include "osal_ax.h"
#include "osal_logdebug_ax.h"

int AX_OSAL_SYNC_atomic_init(AX_ATOMIC_T * atomic)
{
	atomic_t *p;
	if (atomic == NULL) {
		printk("%s error atomic == NULL\n", __func__);
		return -1;
	}
	p = (atomic_t *) kmalloc(sizeof(atomic_t), GFP_KERNEL);
	if (p == NULL) {
		printk("%s - kmalloc failed\n", __func__);
		return -1;
	}
	atomic->atomic = p;
	return 0;
}

EXPORT_SYMBOL(AX_OSAL_SYNC_atomic_init);

int AX_OSAL_SYNC_atomic_read(AX_ATOMIC_T * atomic)
{
	atomic_t *p;
	if (atomic == NULL) {
		printk("%s error atomic == NULL\n", __func__);
		return -1;
	}
	p = (atomic_t *) (atomic->atomic);
	return atomic_read(p);
}

EXPORT_SYMBOL(AX_OSAL_SYNC_atomic_read);

void AX_OSAL_SYNC_atomic_set(AX_ATOMIC_T * atomic, int val)
{
	atomic_t *p;
	p = (atomic_t *) (atomic->atomic);
	atomic_set(p, val);
}

EXPORT_SYMBOL(AX_OSAL_SYNC_atomic_set);

int AX_OSAL_SYNC_atomic_inc_return(AX_ATOMIC_T * atomic)
{
	atomic_t *p;
	if (atomic == NULL) {
		printk("%s error atomic == NULL\n", __func__);
		return -1;
	}
	p = (atomic_t *) (atomic->atomic);
	return atomic_inc_return(p);
}

EXPORT_SYMBOL(AX_OSAL_SYNC_atomic_inc_return);

int AX_OSAL_SYNC_atomic_dec_return(AX_ATOMIC_T * atomic)
{
	atomic_t *p;
	if (atomic == NULL) {
		printk("%s error atomic == NULL\n", __func__);
		return -1;
	}
	p = (atomic_t *) (atomic->atomic);
	return atomic_dec_return(p);
}

EXPORT_SYMBOL(AX_OSAL_SYNC_atomic_dec_return);

int AX_OSAL_SYNC_atomic_cmpxchg(AX_ATOMIC_T * atomic, int old, int new)
{
	atomic_t *p;
	if (atomic == NULL) {
		printk("%s error atomic == NULL\n", __func__);
		return -1;
	}
	p = (atomic_t *) (atomic->atomic);
	return atomic_cmpxchg(p, old, new);
}

EXPORT_SYMBOL(AX_OSAL_SYNC_atomic_cmpxchg);

bool AX_OSAL_SYNC_atomic_try_cmpxchg(AX_ATOMIC_T * atomic, int *old, int new)
{
	atomic_t *p;
	if (atomic == NULL) {
		printk("%s error atomic == NULL\n", __func__);
		return -1;
	}
	p = (atomic_t *) (atomic->atomic);
	return atomic_try_cmpxchg(p, old, new);
}

EXPORT_SYMBOL(AX_OSAL_SYNC_atomic_try_cmpxchg);

void AX_OSAL_SYNC_atomic_and(int val,AX_ATOMIC_T * atomic)
{
       atomic_t *p;
       if (atomic == NULL) {
               printk("%s error atomic == NULL\n", __func__);
               return ;
       }
       p = (atomic_t *) (atomic->atomic);
       return atomic_and(val ,p);
}

EXPORT_SYMBOL(AX_OSAL_SYNC_atomic_and);

void AX_OSAL_SYNC_atomic_or(int val,AX_ATOMIC_T * atomic)
{
       atomic_t *p;
       if (atomic == NULL) {
               printk("%s error atomic == NULL\n", __func__);
               return ;
       }
       p = (atomic_t *) (atomic->atomic);
       return atomic_or(val ,p);
}

EXPORT_SYMBOL(AX_OSAL_SYNC_atomic_or);

int AX_OSAL_SYNC_atomic_fetch_add_ge(AX_ATOMIC_T * atomic, int add, int used)
{
	atomic_t *p;
	int c;
	if (atomic == NULL) {
		printk("error atomic == NULL\n");
		return -1;
	}
	p = (atomic_t *) (atomic->atomic);

	do {
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 15, 0))
		c = arch_atomic_read(p);
#else
		c = atomic_read(p);
#endif
		if(unlikely(c < used))
			break;
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 15, 0))
	} while (!arch_atomic_try_cmpxchg(p, &c, c + add));
#else
	} while (!atomic_try_cmpxchg(p, &c, c + add));
#endif
	return c;
}

EXPORT_SYMBOL(AX_OSAL_SYNC_atomic_fetch_add_ge);

void AX_OSAL_SYNC_atomic_destroy(AX_ATOMIC_T * atomic)
{
	kfree(atomic->atomic);
	atomic->atomic = NULL;
}

EXPORT_SYMBOL(AX_OSAL_SYNC_atomic_destroy);
