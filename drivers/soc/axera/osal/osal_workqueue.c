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
#include <linux/workqueue.h>
#include <linux/kernel.h>
#include <linux/printk.h>
#include <linux/wait.h>
#include <linux/sched.h>
#include <linux/sched/signal.h>
#include <linux/slab.h>
#include "osal_list_ax.h"
#include "osal_logdebug_ax.h"
#include "osal_ax.h"

AX_OSAL_LIST_HEAD(wq_list);
struct wq_node {
	AX_WORK_T *osal_work;
	struct work_struct *work;
	AX_LIST_HEAD_T node;
};

static AX_WORK_T *AX_OSAL_SYNC_find_work(struct work_struct *work)
{
	AX_LIST_HEAD_T *this = NULL;
	if (AX_OSAL_LIB_list_empty(&wq_list)) {
		printk("%s wq_list is empty\n", __func__);
		return NULL;
	}
	AX_OSAL_LIB_list_for_each(this, &wq_list) {
		struct wq_node *ws = AX_OSAL_LIB_list_entry(this, struct wq_node, node);
		if (ws->work == work) {
			return ws->osal_work;
		}
	}
	printk("%s failed\n", __func__);
	return NULL;
}

static int AX_OSAL_SYNC_del_work(struct work_struct *work)
{
	AX_LIST_HEAD_T *this = NULL;
	if (AX_OSAL_LIB_list_empty(&wq_list)) {
		printk("%s error wq_list is empty\n", __func__);
		return -1;
	}
	AX_OSAL_LIB_list_for_each(this, &wq_list) {
		struct wq_node *ws = AX_OSAL_LIB_list_entry(this, struct wq_node, node);
		if (ws->work == work) {
			AX_OSAL_LIB_list_del(this);
			kfree(ws);
			return 0;
		}
	}
	printk("%s failed\n", __func__);
	return -1;
}

static void AX_OSAL_SYNC_work_func(struct work_struct *work)
{
	//struct osal_work_struct *ow = container_of(work, struct osal_work_struct, work);
	AX_WORK_T *ow = AX_OSAL_SYNC_find_work(work);
	if (ow != NULL && ow->func != NULL)
		ow->func(ow);
}

int AX_OSAL_SYNC_init_work(AX_WORK_T * work, AX_WORK_FUNC_T func)
{
	struct work_struct *w = NULL;
	struct wq_node *w_node = NULL;
	w = kmalloc(sizeof(struct work_struct), GFP_ATOMIC);
	if (w == NULL) {
		printk("osal_init_work kmalloc failed!\n");
		return -1;
	}

	w_node = kmalloc(sizeof(struct wq_node), GFP_ATOMIC);
	if (w_node == NULL) {
		printk("osal_init_work kmalloc failed!\n");
		kfree(w);
		return -1;
	}
	INIT_WORK(w, AX_OSAL_SYNC_work_func);
	work->work = w;
	work->func = func;
	w_node->osal_work = work;
	w_node->work = w;
	AX_OSAL_LIB_list_add(&(w_node->node), &wq_list);
	return 0;
}

EXPORT_SYMBOL(AX_OSAL_SYNC_init_work);

int AX_OSAL_SYNC_schedule_work(AX_WORK_T * work)
{
	if (work != NULL && work->work != NULL)
		return (int)schedule_work(work->work);
	else
		return (int)0;
}

EXPORT_SYMBOL(AX_OSAL_SYNC_schedule_work);

void AX_OSAL_SYNC_destroy_work(AX_WORK_T * work)
{
	if (work != NULL && work->work != NULL) {
		AX_OSAL_SYNC_del_work(work->work);
		kfree((struct work_struct *)work->work);
		work->work = NULL;
	}
}

EXPORT_SYMBOL(AX_OSAL_SYNC_destroy_work);

int AX_OSAL_SYNC_init_delayed_work(AX_WORK_T * osal_work, AX_WORK_FUNC_T func)
{
	struct delayed_work *w = NULL;
	struct wq_node *w_node = NULL;
	w = kmalloc(sizeof(struct delayed_work), GFP_ATOMIC);
	if (w == NULL) {
		printk("%s alloc delayed_work failed!\n", __func__);
		return -1;
	}

	w_node = kmalloc(sizeof(struct wq_node), GFP_ATOMIC);
	if (w_node == NULL) {
		printk("%s failed\n", __func__);
		kfree(w);
		return -1;
	}
	INIT_DELAYED_WORK(w, AX_OSAL_SYNC_work_func);
	osal_work->work = w;
	osal_work->func = func;
	w_node->osal_work = osal_work;
	w_node->work = &w->work;
	AX_OSAL_LIB_list_add(&(w_node->node), &wq_list);
	return 0;
}

EXPORT_SYMBOL(AX_OSAL_SYNC_init_delayed_work);

int AX_OSAL_SYNC_schedule_delayed_work(AX_WORK_T * osal_work, unsigned long delay)
{
	if (osal_work != NULL && osal_work->work != NULL)
		return (int)schedule_delayed_work(osal_work->work, delay);
	else
		return (int)0;
}

EXPORT_SYMBOL(AX_OSAL_SYNC_schedule_delayed_work);

int AX_OSAL_SYNC_cancel_delayed_work(AX_WORK_T * osal_work)
{
	if (osal_work != NULL && osal_work->work != NULL)
		return (int)cancel_delayed_work(osal_work->work);
	else
		return (int)0;
}

EXPORT_SYMBOL(AX_OSAL_SYNC_cancel_delayed_work);

int AX_OSAL_SYNC_cancel_delayed_work_sync(AX_WORK_T * osal_work)
{
	if (osal_work != NULL && osal_work->work != NULL)
		return (int)cancel_delayed_work_sync(osal_work->work);
	else
		return (int)0;
}

EXPORT_SYMBOL(AX_OSAL_SYNC_cancel_delayed_work_sync);

void AX_OSAL_SYNC_destroy_delayed_work(AX_WORK_T * osal_work)
{
	if (osal_work != NULL && osal_work->work != NULL) {
		AX_OSAL_SYNC_del_work(osal_work->work);
		kfree((struct work_struct *)osal_work->work);
		osal_work->work = NULL;
	}
}

EXPORT_SYMBOL(AX_OSAL_SYNC_destroy_delayed_work);
