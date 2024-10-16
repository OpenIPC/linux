/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (c) 2018-2021 Arm Technology (China) Co., Ltd. All rights reserved. */

/**
 * @file aipu_job_manager.h
 * Job manager module header file
 */

#ifndef __AIPU_JOB_MANAGER_H__
#define __AIPU_JOB_MANAGER_H__

#include <linux/slab.h>
#include <linux/wait.h>
#include <linux/list.h>
#include <linux/spinlock.h>
#include "armchina_aipu.h"
#include "aipu_core.h"

enum aipu_state_kern {
	AIPU_JOB_STATE_IDLE,
	AIPU_JOB_STATE_PENDING,
	AIPU_JOB_STATE_DEFERRED,
	AIPU_JOB_STATE_RUNNING,
	AIPU_JOB_STATE_EXCEP,
	AIPU_JOB_STATE_SUCCESS
};

/**
 * struct waitqueue - maintain the waitqueue for a user thread
 *
 * @uthread_id: user thread owns this waitqueue
 * @filp: file struct pointer
 * @ref_cnt: struct reference count
 * @p_wait: wait queue head for polling
 * @node: list head struct
 */
struct aipu_thread_wait_queue {
	int uthread_id;
	struct file *filp;
	int ref_cnt;
	wait_queue_head_t p_wait;
	struct list_head node;
};

/**
 * struct aipu_job - job struct describing a job under scheduling in job manager
 *        Job status will be tracked as soon as interrupt or user evenets come in.
 *
 * @uthread_id: ID of user thread scheduled this job
 * @filp: file struct pointer used when scheduling this job
 * @desc: job desctiptor from userland
 * @core_id: ID of an aipu core this job scheduled on
 * @thread_queue: wait queue of this job to be waken up
 * @state: job state
 * @node: list node
 * @sched_time: job scheduled time (enabled by profiling flag in desc)
 * @done_time: job termination time (enabled by profiling flag in desc)
 * @pdata: profiling data (enabled by profiling flag in desc)
 * @wake_up: wake up flag
 */
struct aipu_job {
	int uthread_id;
	struct file *filp;
	struct aipu_job_desc desc;
	int core_id;
	wait_queue_head_t *thread_queue;
	int state;
	struct list_head node;
	ktime_t sched_time;
	ktime_t done_time;
	struct aipu_ext_profiling_data pdata;
	int wake_up;
};

/**
 * struct aipu_job_manager - job manager
 *        Maintain all jobs and update their statuses
 *
 * @core_cnt: aipu core count
 * @cores: aipu core struct pointer array
 * @idle_bmap: idle flag bitmap for every core
 * @scheduled_head: scheduled job list head
 * @lock: spinlock
 * @wait_queue_head: wait queue list head
 * @wq_lock: waitqueue lock
 * @job_cache: slab cache of aipu_job
 * @is_init: init flag
 * @exec_flag: execution flags propagated to all jobs
 */
struct aipu_job_manager {
	int core_cnt;
	struct aipu_core **cores;
	bool *idle_bmap;
	struct aipu_job *scheduled_head;
	spinlock_t lock;
	struct aipu_thread_wait_queue *wait_queue_head;
	struct mutex wq_lock;
	struct kmem_cache *job_cache;
	int is_init;
	int exec_flag;
};

/**
 * @brief initialize an existing job manager struct during driver probe phase
 *
 * @param manager: job_manager struct pointer allocated in calling function
 *
 * @return 0 if successful; others if failed;
 */
int init_aipu_job_manager(struct aipu_job_manager *manager);
/**
 * @brief de-init the job manager init in init_aipu_job_manager
 *
 * @param manager: job_manager struct pointer allocated in calling function
 *
 * @return void
 */
void deinit_aipu_job_manager(struct aipu_job_manager *manager);
/**
 * @brief set multicore info. while probing
 *
 * @param manager: job_manager struct pointer
 * @param core_cnt: AIPU core count
 * @param cores: AIPU core struct pointer array
 *
 * @return void
 */
void aipu_job_manager_set_cores_info(struct aipu_job_manager *manager, int core_cnt,
	struct aipu_core **cores);
/**
 * @brief schedule a job flushed from userland
 *
 * @param manager: job_manager struct pointer
 * @param user_job: user_job descriptor struct
 * @param filp: file pointer from fops
 *
 * @return 0 if successful; others if failed;
 */
int aipu_job_manager_scheduler(struct aipu_job_manager *manager, struct aipu_job_desc *user_job,
	struct file *filp);
/**
 * @brief aipu interrupt upper half handler called in interrupt contexts
 *
 * @param core: aipu core struct pointer
 * @param exception_flag: exception flag
 *
 * @return void
 */
void aipu_job_manager_irq_upper_half(struct aipu_core *core, int exception_flag);
/**
 * @brief aipu interrupt bottom half handler
 *
 * @param core: aipu core struct pointer
 *
 * @return void
 */
void aipu_job_manager_irq_bottom_half(struct aipu_core *core);
/**
 * @brief cancel all jobs flushed with certain user closing fd
 *
 * @param manager: job_manager struct pointer
 * @param filp: file struct pointer
 *
 * @return 0 if successful; others if failed;
 */
int aipu_job_manager_cancel_jobs(struct aipu_job_manager *manager, struct file *filp);
/**
 * @brief invalidate/kill a timeout job
 *
 * @param manager: job_manager struct pointer
 * @param job_id: job ID
 *
 * @return 0 if successful; others if failed;
 */
int aipu_job_manager_invalidate_timeout_job(struct aipu_job_manager *manager, int job_id);
/**
 * @brief get AIPU jobs' statuses after a polling event returns
 *
 * @param manager: job_manager struct pointer
 * @param job_status: job status array stores statuys info filled by this API
 * @param filp: file struct pointer
 *
 * @return 0 if successful; others if failed;
 */
int aipu_job_manager_get_job_status(struct aipu_job_manager *manager,
	struct aipu_job_status_query *job_status, struct file *filp);
/**
 * @brief check if a user thread has end job(s) to query
 *
 * @param manager: job_manager struct pointer
 * @param filp: file struct pointer
 * @param wait: wait table struct from kernel
 * @param uthread_id: thread ID
 *
 * @return true or false
 */
int aipu_job_manager_has_end_job(struct aipu_job_manager *manager, struct file *filp,
	struct poll_table_struct *wait, int uthread_id);

#endif /* __AIPU_JOB_MANAGER_H__ */
