/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/

#ifndef _AX_GZIPD_ADAPTER_H_
#define _AX_GZIPD_ADAPTER_H_

#include <types.h>
#include <wait.h>
#include <mutex.h>
#include <spinlock.h>
#include <kthread.h>
#include <linux/sched.h>

#define AX_GZIPD_GET_PID task_pid_nr(current)
#define GZIPD_THREAD_NAME_MAX_LEN 32

#define GZIPD_CONDITION_TIMEOUT (-9)

typedef enum {
	gzipd_lock_type_inturrupt = 0,
	gzipd_lock_type_no_schedule,
	gzipd_lock_type_schedule
} gzipd_lock_type_t;

typedef struct spinlock  gzipd_lock_inturrupt_t;
typedef struct spinlock  gzipd_lock_no_schedule_t;
typedef struct mutex gzipd_lock_schedule_t;

typedef struct {
	wait_queue_head_t *wq;
	uint32_t condition;
} gzipd_thread_cond_t;

typedef enum {
	gzipd_thread_priority_null,
	gzipd_thread_priority_1,
	gzipd_thread_priority_2,
	gzipd_thread_priority_3
} gzipd_thread_priority_t;
typedef int32_t gzipd_thread_fun_t(void *data);
typedef void *gzipd_thread_t;

void gzipd_iomem_map(void);
void gzipd_iomem_unmap(void);
uint32_t gzipd_reg_read(uint32_t RegAddrOffset);
void gzipd_reg_write(uint32_t RegAddrOffset, uint32_t val);
void gzipd_udelay(uint64_t cnt);
void gzipd_flash_iomem_map(void);
void gzipd_flash_iomem_unmap(void);
void gzipd_flash_reg_write(uint32_t RegAddrOffset, uint32_t val);
uint32_t gzipd_flash_reg_read(uint32_t RegAddrOffset);
struct task_struct *gzipd_thread_create_v2(gzipd_thread_fun_t *gzipd_thread_fun, void *data, char *name, gzipd_thread_priority_t priority);
gzipd_thread_t gzipd_thread_create(gzipd_thread_fun_t *gzipd_thread_fun, void *data, char *name, gzipd_thread_priority_t priority);
int32_t gzipd_thread_cond_waittime(gzipd_thread_cond_t *thread_cond, void *lock, unsigned long *irqflag, gzipd_lock_type_t lock_type, uint64_t time_ms);
int32_t gzipd_thread_cond_broadcast(gzipd_thread_cond_t *thread_cond);
int32_t gzipd_irq_cond_wait(void);
void gzipd_interrupt_wait_exit(void);
int32_t gzipd_thread_cond_init(gzipd_thread_cond_t *thread_cond);
int32_t gzipd_thread_cond_deinit(gzipd_thread_cond_t *thread_cond);

#endif

