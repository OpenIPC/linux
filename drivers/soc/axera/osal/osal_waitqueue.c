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
#include <linux/wait.h>
#include <linux/sched.h>
#include <linux/sched/signal.h>
#include <linux/slab.h>
#include "osal_ax.h"

int AX_OSAL_SYNC_waitqueue_init(AX_WAIT_T * wait)
{
	wait_queue_head_t *wq;
	if (wait == NULL) {
		printk("%s - parameter invalid!\n", __func__);
		return -1;
	}
	wq = (wait_queue_head_t *) kmalloc(sizeof(wait_queue_head_t), GFP_ATOMIC);
	if (wq == NULL) {
		printk("%s - kmalloc error!\n", __func__);
		return -1;
	}
	init_waitqueue_head(wq);
	wait->wait = wq;
	return 0;

}

EXPORT_SYMBOL(AX_OSAL_SYNC_waitqueue_init);

unsigned int __AX_OSAL_SYNC_wait_uninterruptible(AX_WAIT_T * wait, AX_WAIT_COND_FUNC_T func, void * param)
{
	wait_queue_head_t *wq;
	DEFINE_WAIT(__wait);
	long ret = 0;
	int condition = 0;

	if (wait == NULL) {
		printk("%s - parameter invalid!\n", __func__);
		return -1;
	}

	wq = (wait_queue_head_t *) (wait->wait);
	if (wq == NULL) {
		printk("%s - wait->wait is NULL!\n", __func__);
		return -1;
	}
	prepare_to_wait(wq, &__wait, TASK_UNINTERRUPTIBLE);
	/* if wakeup the queue brefore prepare_to_wait, the func will return true. And will not go to schedule */
	if (NULL != func) {
		condition = func(param);
	}

	if (!condition) {
		schedule();
	}

	finish_wait(wq, &__wait);
	return ret;
}

EXPORT_SYMBOL(__AX_OSAL_SYNC_wait_uninterruptible);

unsigned int __AX_OSAL_SYNC_wait_interruptible(AX_WAIT_T * wait, AX_WAIT_COND_FUNC_T func, void * param)
{
	wait_queue_head_t *wq;
	DEFINE_WAIT(__wait);
	long ret = 0;
	int condition = 0;

	if (wait == NULL) {
		printk("%s - parameter invalid!\n", __func__);
		return -1;
	}

	wq = (wait_queue_head_t *) (wait->wait);
	if (wq == NULL) {
		printk("%s - wait->wait is NULL!\n", __func__);
		return -1;
	}
	prepare_to_wait(wq, &__wait, TASK_INTERRUPTIBLE);

	if (NULL != func) {
		condition = func(param);
	}

	if (!condition) {
		if (!signal_pending(current)) {
			schedule();
		}
		if (signal_pending(current))
			ret = -ERESTARTSYS;
	} else {
		ret = -2;
	}

	finish_wait(wq, &__wait);
	return ret;

}

EXPORT_SYMBOL(__AX_OSAL_SYNC_wait_interruptible);

unsigned int __AX_OSAL_SYNC_wait_uninterruptible_timeout(AX_WAIT_T * wait, AX_WAIT_COND_FUNC_T func, void * param,
						 unsigned long timeout)
{
	wait_queue_head_t *wq;
	DEFINE_WAIT(__wait);
	long ret = timeout;
	int condition = 0;

	if (wait == NULL) {
		printk("%s wait == NULL\n", __func__);
		return -1;
	}

	wq = (wait_queue_head_t *) (wait->wait);
	if (wq == NULL) {
		printk("%swq == NULL\n", __func__);
		return -1;
	}
	prepare_to_wait(wq, &__wait, TASK_UNINTERRUPTIBLE);

	if (NULL != func) {
		condition = func(param);
	}

	if (!condition) {
		ret = schedule_timeout(ret);
	}

	finish_wait(wq, &__wait);

	return ret;
}

EXPORT_SYMBOL(__AX_OSAL_SYNC_wait_uninterruptible_timeout);

unsigned int __AX_OSAL_SYNC_wait_interruptible_timeout(AX_WAIT_T * wait, AX_WAIT_COND_FUNC_T func, void * param,
					       unsigned long timeout)
{
	wait_queue_head_t *wq;
	DEFINE_WAIT(__wait);
	long ret = timeout;
	int condition = 0;

	if (wait == NULL) {
		printk("%s wq == NULL\n", __func__);
		return -1;
	}

	wq = (wait_queue_head_t *) (wait->wait);
	if (wq == NULL) {
		printk("%s wq == NULL\n", __func__);
		return -1;
	}
	prepare_to_wait(wq, &__wait, TASK_INTERRUPTIBLE);
	/* if wakeup the queue brefore prepare_to_wait, the func will return true. And will not go to schedule */
	if (NULL != func) {
		condition = func(param);
	}

	if (!condition) {
		if (!signal_pending(current)) {
			ret = schedule_timeout(ret);
		}
		if (signal_pending(current))
			ret = -ERESTARTSYS;
	}

	finish_wait(wq, &__wait);

	return ret;
}

EXPORT_SYMBOL(__AX_OSAL_SYNC_wait_interruptible_timeout);

void AX_OSAL_SYNC_wakeup(AX_WAIT_T * wait, void * key)
{
	wait_queue_head_t *wq;

	wq = (wait_queue_head_t *) (wait->wait);
	if (wq == NULL) {
		printk("%s wq == NULL\n", __func__);
		return;
	}
	wake_up_all(wq);
}

EXPORT_SYMBOL(AX_OSAL_SYNC_wakeup);

void AX_OSAL_SYNC_wake_up_interruptible(AX_WAIT_T * wait, void * key)
{
	wait_queue_head_t *wq;

	wq = (wait_queue_head_t *) (wait->wait);
	if (wq == NULL) {
		printk("%s wq == NULL\n", __func__);
		return;
	}
	wake_up_interruptible(wq);

	return;
}

EXPORT_SYMBOL(AX_OSAL_SYNC_wake_up_interruptible);

void AX_OSAL_SYNC_wake_up_interruptible_all(AX_WAIT_T * wait, void * key)
{
	wait_queue_head_t *wq;

	wq = (wait_queue_head_t *) (wait->wait);
	if (wq == NULL) {
		printk("%s wq == NULL\n", __func__);
		return;
	}
	wake_up_interruptible_all(wq);

	return;
}

EXPORT_SYMBOL(AX_OSAL_SYNC_wake_up_interruptible_all);

void AX_OSAL_SYNC_wait_destroy(AX_WAIT_T * wait)
{
	wait_queue_head_t *wq;

	wq = (wait_queue_head_t *) (wait->wait);
	if (wq == NULL) {
		printk("%s wq == NULL\n", __func__);
		return;
	}
	kfree(wq);
	wait->wait = NULL;
}

EXPORT_SYMBOL(AX_OSAL_SYNC_wait_destroy);
