/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/

#include <linux/stddef.h>
#include <linux/types.h>
#include <linux/delay.h>
#include <linux/io.h>
#include <linux/wait.h>
#include <linux/slab.h>
#include <linux/sched.h>
#include <linux/gfp.h>
#include <linux/string.h>
#include <linux/atomic.h>
#include <linux/kthread.h>
#include <uapi/linux/sched/types.h>
#include <linux/sched.h>

#include "ax_gzipd_reg.h"
#include "ax_gzipd_adapter.h"
#include "ax_gzipd_log.h"

typedef struct {
	gzipd_thread_fun_t *gzipd_thread_fun;
	gzipd_thread_priority_t priority;
	void *data;
	char name[GZIPD_THREAD_NAME_MAX_LEN];
	struct task_struct *thread_handle;
	atomic_t exit_finish;
} gzipd_thread_struct_t;

static void *gzipdRegBaseVirAddr = NULL;
static void *flashSysRegBaseViraddr = NULL;

static int32_t gzipd_thread_entry(void *data)
{
	gzipd_thread_struct_t *gzipd_thread = (gzipd_thread_struct_t *)data;
	gzipd_thread->gzipd_thread_fun(gzipd_thread->data);
	while(!kthread_should_stop()) {
		atomic_set(&gzipd_thread->exit_finish, 1);
		set_current_state(TASK_INTERRUPTIBLE);
		schedule();
	}
	return 0;
}

static int32_t to_real_priority( gzipd_thread_priority_t priority)
{
	int32_t real_pority = -1;
	switch (priority)
	{
	case gzipd_thread_priority_1:
		real_pority = 99;
		break;
	case gzipd_thread_priority_2:
		real_pority = 99;
		break;
	case gzipd_thread_priority_3:
		real_pority = 99;
		break;
	default:
		break;
	}
	return real_pority;
}

struct task_struct *gzipd_thread_create_v2(gzipd_thread_fun_t *gzipd_thread_fun, void *data, char *name, gzipd_thread_priority_t priority)
{
	struct task_struct *thread;
	thread = kthread_run(gzipd_thread_fun, NULL, "%s", name);
	if (IS_ERR(thread)) {
		AX_GZIP_DEV_LOG_ERR("create thread fail");
		return NULL;
	}

	if (priority != gzipd_thread_priority_null) {
		struct sched_attr attr;
		attr.sched_priority = to_real_priority(priority);
		attr.sched_policy = SCHED_FIFO;
		sched_setattr_nocheck(thread, &attr);
	}

	return thread;
}

gzipd_thread_t gzipd_thread_create(gzipd_thread_fun_t *gzipd_thread_fun, void *data, char *name, gzipd_thread_priority_t priority)
{
	gzipd_thread_struct_t *thread_struct;
	thread_struct = kmalloc(sizeof(gzipd_thread_struct_t), GFP_KERNEL);
	if (thread_struct == NULL) {
		return NULL;
	}
	if (name != NULL) {
		strncpy(thread_struct->name, name, GZIPD_THREAD_NAME_MAX_LEN -1);
		thread_struct->name[GZIPD_THREAD_NAME_MAX_LEN -1] = 0;
	} else {
		thread_struct->name[0] = 0;
	}
	thread_struct->data = data;
	thread_struct->gzipd_thread_fun = gzipd_thread_fun;
	atomic_set(&thread_struct->exit_finish, 0);
	thread_struct->thread_handle = kthread_run(gzipd_thread_entry, (void *)thread_struct, name);
	if (thread_struct->thread_handle == NULL) {
		kfree(thread_struct);
		return NULL;
	}
	if (priority != gzipd_thread_priority_null) {
		struct sched_attr attr;
		attr.sched_priority = to_real_priority(priority);
		attr.sched_policy = SCHED_FIFO;
		sched_setattr_nocheck(thread_struct->thread_handle, &attr);
	}
	return (gzipd_thread_t)thread_struct;
}

int32_t gzipd_thread_cancel(gzipd_thread_t gzipd_thread)
{
	gzipd_thread_struct_t *thread_struct = (gzipd_thread_struct_t *)gzipd_thread;
	kthread_stop(thread_struct->thread_handle);
	thread_struct->thread_handle = NULL;
	kfree(thread_struct);
	return 0;
}

int32_t gzipd_thread_join(gzipd_thread_t gzipd_thread)
{
	gzipd_thread_struct_t *thread_struct = (gzipd_thread_struct_t *)gzipd_thread;
	while(0 == atomic_read(&thread_struct->exit_finish)) {
		msleep(1);
	}
	gzipd_thread_cancel(gzipd_thread);
	kfree(gzipd_thread);
	return 0;
}

void gzipd_iomem_map(void)
{
	if (gzipdRegBaseVirAddr == NULL) {
		gzipdRegBaseVirAddr = ioremap(AX_GZIPD_BASE_PADDR, 0x4000);
	}
}

void gzipd_iomem_unmap(void)
{
	if (gzipdRegBaseVirAddr) {
		iounmap(gzipdRegBaseVirAddr);
	}
}

uint32_t gzipd_reg_read(uint32_t RegAddrOffset)
{
	if (gzipdRegBaseVirAddr) {
		return __raw_readl((volatile void *)gzipdRegBaseVirAddr + RegAddrOffset);
	} else {
		AX_GZIP_DEV_LOG_ERR("gzipd reg base addr wasn't mapped when read\n");
		return -1;
	}
}

void gzipd_reg_write(uint32_t RegAddrOffset, uint32_t val)
{
	if (gzipdRegBaseVirAddr) {
		__raw_writel(val, (volatile void *)gzipdRegBaseVirAddr + RegAddrOffset);
	} else {
		AX_GZIP_DEV_LOG_ERR("gzipd reg base addr isn't mapped when write\n");
	}
}

void gzipd_flash_iomem_map(void)
{
	if (flashSysRegBaseViraddr == NULL) {
		flashSysRegBaseViraddr = ioremap(FLASH_SYS_GLB_BASE_ADDR, 0x81BC);
	}
}

void gzipd_flash_iomem_unmap(void)
{
	if (flashSysRegBaseViraddr) {
		iounmap(flashSysRegBaseViraddr);
		flashSysRegBaseViraddr = NULL;
	}
}

void gzipd_flash_reg_write(uint32_t RegAddrOffset, uint32_t val)
{
	if (flashSysRegBaseViraddr) {
		__raw_writel(val, (volatile void *)flashSysRegBaseViraddr + RegAddrOffset);
	} else {
		AX_GZIP_DEV_LOG_ERR("flash reg base addr isn't mapped when write\n");
	}
}

uint32_t gzipd_flash_reg_read(uint32_t RegAddrOffset)
{
	if (flashSysRegBaseViraddr) {
		return __raw_readl(flashSysRegBaseViraddr + RegAddrOffset);
	} else {
		AX_GZIP_DEV_LOG_ERR("flash reg base addr isn't mapped when write\n");
		return -1;
	}
}

void gzipd_udelay(uint64_t cnt)
{
	udelay(cnt);
}

int32_t gzipd_lock_interrupt_init(gzipd_lock_inturrupt_t *lock)
{
	spin_lock_init(lock);
	return 0;
}

int32_t gzipd_lock_interrupt_lock(gzipd_lock_inturrupt_t *lock, unsigned long *flag)
{
	spin_lock_irqsave(lock, *flag);
	return 0;
}

int32_t gzipd_lock_interrupt_unlock(gzipd_lock_inturrupt_t *lock, unsigned long *flag)
{
	spin_unlock_irqrestore(lock,*flag);
	return 0;
}

int32_t gzipd_lock_interrupt_destroy(gzipd_lock_inturrupt_t *lock)
{
	// spinLock_destory(lock->lock);
	return 0;
}

int32_t gzipd_lock_no_schedule_init(gzipd_lock_no_schedule_t *lock, bool inherit)
{
	(void)inherit;
	spin_lock_init(lock);
	return 0;
}

int32_t gzipd_lock_no_schedule_lock(gzipd_lock_no_schedule_t *lock)
{
	spin_lock(lock);
	return 0;
}

int32_t gzipd_lock_no_schedule_unlock(gzipd_lock_no_schedule_t *lock)
{
	spin_unlock(lock);
	return 0;
}

int32_t gzipd_lock_no_schedule_destroy(gzipd_lock_no_schedule_t *lock)
{
	// spinLock_destory(lock->lock);
	return 0;
}

int32_t gzipd_lock_schedule_init(gzipd_lock_schedule_t *lock, bool inherit)
{
	(void)inherit;
	mutex_init(lock);
	return 0;
}

int32_t gzipd_lock_schedule_lock(gzipd_lock_schedule_t *lock)
{
	mutex_lock(lock);
	return 0;
}

int32_t gzipd_lock_schedule_unlock(gzipd_lock_schedule_t *lock)
{
	mutex_unlock(lock);
	return 0;
}

int32_t gzipd_lock_schedule_destroy(gzipd_lock_schedule_t *lock)
{
	mutex_destroy(lock);
	return 0;
}

inline void gzipd_lock_contex(void *lock, gzipd_lock_type_t lock_type, unsigned long *flag)
{
	if (lock_type == gzipd_lock_type_inturrupt) {
		gzipd_lock_interrupt_lock((gzipd_lock_inturrupt_t *)lock, flag);
	} else if (lock_type == gzipd_lock_type_no_schedule) {
		gzipd_lock_no_schedule_lock((gzipd_lock_no_schedule_t *)lock);
	} else if (lock_type == gzipd_lock_type_schedule) {
		gzipd_lock_schedule_lock((gzipd_lock_schedule_t *)lock);
	}
}

inline void gzipd_unlock_contex(void *lock, gzipd_lock_type_t lock_type, unsigned long *flag)
{
	if (lock_type == gzipd_lock_type_inturrupt) {
		gzipd_lock_interrupt_unlock((gzipd_lock_inturrupt_t *)lock, flag);
	} else if (lock_type == gzipd_lock_type_no_schedule) {
		gzipd_lock_no_schedule_unlock((gzipd_lock_no_schedule_t *)lock);
	} else if (lock_type == gzipd_lock_type_schedule) {
		gzipd_lock_schedule_unlock((gzipd_lock_schedule_t *)lock);
	}
}

int32_t gzipd_thread_cond_init(gzipd_thread_cond_t *thread_cond)
{
	wait_queue_head_t *wq;
	wq = (wait_queue_head_t *)kmalloc(sizeof(wait_queue_head_t), GFP_ATOMIC);
	if (wq == NULL) {
		AX_GZIP_DEV_LOG_ERR("%s - kmalloc error!\n", __FUNCTION__);
		return -1;
	}
	init_waitqueue_head(wq);
	thread_cond->wq = wq;
	thread_cond->condition = 0;
	return 0;
}

int32_t gzipd_thread_cond_deinit(gzipd_thread_cond_t *thread_cond)
{
	wait_queue_head_t *wq;
	wq = thread_cond->wq;
	if (wq == NULL) {
		AX_GZIP_DEV_LOG_ERR("free pointer is error!\n");
		return -1;
	}
	kfree(wq);
	thread_cond->wq = NULL;
	return 0;
}

static int32_t ax_wait_cond_fun(void *param)
{
	if (*((uint32_t *)param)) {
		return 1;
	}
	return 0;
}

typedef int(*GZIPD_WAIT_COND_FUNC_T)(void *param);
static uint32_t gzipd_sync_wait_uninterruptible_timeout(wait_queue_head_t *wq, GZIPD_WAIT_COND_FUNC_T func, void * param, uint64_t timeout)
{
	DEFINE_WAIT(__wait);
	long ret = timeout;
	int condition = 0;

	if (wq == NULL) {
		AX_GZIP_DEV_LOG_ERR("%s wq == NULL\n", __FUNCTION__);
		return -1;
	}
	prepare_to_wait(wq, &__wait, TASK_UNINTERRUPTIBLE);

	if (NULL != func) {
		condition = func(param);
	}

	if (!condition) {
		ret = schedule_timeout(msecs_to_jiffies(ret));
		ret = jiffies_to_msecs(ret);
	}

	finish_wait(wq, &__wait);

	return ret;
}

int32_t gzipd_thread_cond_waittime(gzipd_thread_cond_t *thread_cond, void *lock, unsigned long *irqflag, gzipd_lock_type_t lock_type, uint64_t time_ms)
{
	uint64_t ret = GZIPD_CONDITION_TIMEOUT;
	if (!thread_cond->condition) {
		gzipd_unlock_contex(lock, lock_type, irqflag);
		ret = gzipd_sync_wait_uninterruptible_timeout(thread_cond->wq, ax_wait_cond_fun, (void *)(&thread_cond->condition), (uint64_t)time_ms);
		gzipd_lock_contex(lock, lock_type, irqflag);
	}

	if (thread_cond->condition || ret != 0) {
		thread_cond->condition = 0;
		return 0;
	}
	return GZIPD_CONDITION_TIMEOUT;
}

int32_t gzipd_thread_cond_broadcast(gzipd_thread_cond_t *thread_cond)
{
	thread_cond->condition = 1;

	if (thread_cond->wq) {
		wake_up_all(thread_cond->wq);
	}
	return 0;
}
