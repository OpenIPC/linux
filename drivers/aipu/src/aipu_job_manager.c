// SPDX-License-Identifier: GPL-2.0
/* Copyright (c) 2018-2021 Arm Technology (China) Co., Ltd. All rights reserved. */

/**
 * @file aipu_job_manager.c
 * Job manager module implementation file
 */

#include <linux/string.h>
#include <linux/time.h>
#include <linux/uaccess.h>
#include <linux/poll.h>
#include "aipu_job_manager.h"
#include "aipu_priv.h"

static struct aipu_thread_wait_queue *do_create_thread_wait_queue(int uthread_id, struct file *filp)
{
	struct aipu_thread_wait_queue *new_wait_queue = kzalloc(sizeof(*new_wait_queue), GFP_ATOMIC);

	if (unlikely(!new_wait_queue))
		return ERR_PTR(-ENOMEM);
	new_wait_queue->ref_cnt = 0;
	new_wait_queue->uthread_id = uthread_id;
	new_wait_queue->filp = filp;
	init_waitqueue_head(&new_wait_queue->p_wait);
	INIT_LIST_HEAD(&new_wait_queue->node);
	return new_wait_queue;
}

static struct aipu_thread_wait_queue *get_thread_wait_queue_no_lock(struct aipu_thread_wait_queue *head,
	int uthread_id, struct file *filp)
{
	struct aipu_thread_wait_queue *curr = NULL;

	if (unlikely(!head))
		return ERR_PTR(-EINVAL);

	list_for_each_entry(curr, &head->node, node) {
		if (((curr->uthread_id == uthread_id) && (uthread_id)) ||
		    ((curr->filp == filp) && (filp)))
			return curr;
	}
	return ERR_PTR(-EINVAL);
}

static struct aipu_thread_wait_queue *create_thread_wait_queue_no_lock(struct aipu_thread_wait_queue *head,
	int uthread_id, struct file *filp)
{
	struct aipu_thread_wait_queue *queue = get_thread_wait_queue_no_lock(head, uthread_id, filp);

	if (IS_ERR(queue)) {
		queue = do_create_thread_wait_queue(uthread_id, filp);
		if (!IS_ERR(queue) && head)
			list_add_tail(&queue->node, &head->node);
		else
			return queue;
	}

	queue->ref_cnt++;
	return queue;
}

static void delete_wait_queue(struct aipu_thread_wait_queue **wait_queue_head)
{
	struct aipu_thread_wait_queue *curr = NULL;
	struct aipu_thread_wait_queue *next = NULL;

	if (wait_queue_head && *wait_queue_head) {
		list_for_each_entry_safe(curr, next, &(*wait_queue_head)->node, node) {
			list_del(&curr->node);
			kfree(curr);
		}
		kfree(*wait_queue_head);
		*wait_queue_head = NULL;
	}
}

static int init_aipu_job(struct aipu_job *job, struct aipu_job_desc *desc,
	struct aipu_thread_wait_queue *queue, struct file *filp)
{
	if (unlikely(!job))
		return -EINVAL;

	job->uthread_id = task_pid_nr(current);
	job->filp = filp;
	if (likely(desc))
		job->desc = *desc;
	else
		memset(&job->desc, 0, sizeof(job->desc));
	job->core_id = -1;
	job->thread_queue = &queue->p_wait;
	job->state = AIPU_JOB_STATE_IDLE;
	INIT_LIST_HEAD(&job->node);
	job->sched_time = ns_to_ktime(0);
	job->done_time = ns_to_ktime(0);
	job->wake_up = 0;

	return 0;
}

static void destroy_aipu_job(struct aipu_job_manager *manager, struct aipu_job *job)
{
	struct aipu_thread_wait_queue *job_aipu_wait_queue = NULL;

	BUG_ON(!job);

	if (likely(job->thread_queue)) {
		job_aipu_wait_queue = container_of(job->thread_queue, struct aipu_thread_wait_queue, p_wait);
		job_aipu_wait_queue->ref_cnt--;
	}
	kmem_cache_free(manager->job_cache, job);
}

static struct aipu_job *create_aipu_job(struct aipu_job_manager *manager, struct aipu_job_desc *desc,
	struct aipu_thread_wait_queue *queue, struct file *filp)
{
	int ret = 0;
	struct aipu_job *new_aipu_job = NULL;

	new_aipu_job = kmem_cache_alloc(manager->job_cache, GFP_KERNEL);
	if (unlikely(!new_aipu_job))
		return ERR_PTR(-ENOMEM);

	ret = init_aipu_job(new_aipu_job, desc, queue, filp);
	if (unlikely(ret)) {
		destroy_aipu_job(manager, new_aipu_job);
		new_aipu_job = NULL;
		return ERR_PTR(ret);
	}

	return new_aipu_job;
}

static void remove_aipu_job(struct aipu_job_manager *manager, struct aipu_job *job)
{
	BUG_ON(!job);
	list_del(&job->node);
	destroy_aipu_job(manager, job);
}

static void delete_job_queue(struct aipu_job_manager *manager, struct aipu_job **head)
{
	struct aipu_job *curr = NULL;
	struct aipu_job *next = NULL;

	if (head && *head) {
		list_for_each_entry_safe(curr, next, &(*head)->node, node) {
			remove_aipu_job(manager, curr);
		}
		kmem_cache_free(manager->job_cache, *head);
		*head = NULL;
	}
}

int init_aipu_job_manager(struct aipu_job_manager *manager)
{
	if (!manager)
		return -EINVAL;

	manager->is_init = 0;
	manager->core_cnt = 0;
	manager->cores = NULL;
	manager->idle_bmap = NULL;
	manager->job_cache = kmem_cache_create("aipu_job_cache", sizeof(struct aipu_job),
		0, SLAB_PANIC, NULL);
	manager->scheduled_head = create_aipu_job(manager, NULL, NULL, NULL);
	INIT_LIST_HEAD(&manager->scheduled_head->node);
	spin_lock_init(&manager->lock);
	manager->wait_queue_head = create_thread_wait_queue_no_lock(NULL, 0, NULL);
	mutex_init(&manager->wq_lock);
	manager->exec_flag = 0;

	BUG_ON(IS_ERR(manager->scheduled_head));
	BUG_ON(IS_ERR(manager->wait_queue_head));

	/* success */
	manager->is_init = 1;
	return 0;
}

void deinit_aipu_job_manager(struct aipu_job_manager *manager)
{
	if ((!manager) || (!manager->is_init))
		return;

	kfree(manager->idle_bmap);
	manager->idle_bmap = NULL;
	delete_job_queue(manager, &manager->scheduled_head);
	delete_wait_queue(&manager->wait_queue_head);
	mutex_destroy(&manager->wq_lock);
	kmem_cache_destroy(manager->job_cache);
	manager->job_cache = NULL;
	manager->is_init = 0;
}

void aipu_job_manager_set_cores_info(struct aipu_job_manager *manager, int core_cnt,
	struct aipu_core **cores)
{
	BUG_ON((!manager) || (!core_cnt) || (!cores));
	manager->core_cnt = core_cnt;
	manager->cores = cores;
	kfree(manager->idle_bmap);
	manager->idle_bmap = kmalloc_array(core_cnt, sizeof(bool), GFP_KERNEL);
	memset(manager->idle_bmap, 1, core_cnt);
}

inline bool is_job_version_match(struct aipu_core *core, struct aipu_job_desc *user_job)
{
	if ((core->arch == user_job->aipu_arch) && (user_job->version_compatible))
		return true;

	return (core->arch == user_job->aipu_arch) &&
		(core->version == user_job->aipu_version) &&
		(core->config == user_job->aipu_config);
}

static bool is_user_job_valid(struct aipu_job_manager *manager, struct aipu_job_desc *user_job)
{
	int id = 0;
	struct aipu_core *core = NULL;

	if (unlikely((!manager) || (!user_job)))
		return false;

	if (user_job->is_defer_run) {
		id = user_job->core_id;
		if (id < manager->core_cnt)
			return is_job_version_match(manager->cores[id], user_job);
		return false;
	}

	for (id = 0; id < manager->core_cnt; id++) {
		core = manager->cores[id];
		if (is_job_version_match(core, user_job))
			return true;
	}

	return false;
}

static int get_available_core_no_lock(struct aipu_job_manager *manager, struct aipu_job *job)
{
	int id = 0;
	struct aipu_core *core = NULL;

	if (unlikely(!manager))
		return -1;

	for (id = 0; id < manager->core_cnt; id++) {
		core = manager->cores[id];
		if (!atomic_read(&core->disable) && manager->idle_bmap[id] && is_job_version_match(core, &job->desc))
			return id;
	}

	return -1;
}

static void reserve_core_for_job_no_lock(struct aipu_job_manager *manager, struct aipu_job *job, int do_trigger)
{
	struct aipu_core *sched_core = NULL;

	BUG_ON(job->core_id < 0);
	BUG_ON(job->core_id >= manager->core_cnt);

	sched_core = manager->cores[job->core_id];
	manager->idle_bmap[job->core_id] = 0;
	if (job->desc.enable_prof) {
		sched_core->ops->start_bw_profiling(sched_core);
		job->sched_time = ktime_get();
	}

	if (do_trigger)
		job->state = AIPU_JOB_STATE_RUNNING;

	sched_core->ops->reserve(sched_core, &job->desc, do_trigger);

	if (do_trigger)
		dev_info(sched_core->dev, "[Job %d of Thread %d] trigger job running done\n",
		    job->desc.job_id, job->uthread_id);
	else
		dev_info(sched_core->dev, "[Job %d of Thread %d] reserve for deferred job done\n",
		    job->desc.job_id, job->uthread_id);
}

static int schedule_new_job(struct aipu_job_manager *manager, struct aipu_job_desc *user_job,
	struct file *filp, int do_trigger)
{
	int ret = 0;
	struct aipu_job *kern_job = NULL;
	struct aipu_thread_wait_queue *queue = NULL;
	unsigned long flags;

	mutex_lock(&manager->wq_lock);
	if (user_job->enable_poll_opt)
		queue = create_thread_wait_queue_no_lock(manager->wait_queue_head, 0, filp);
	else
		queue = create_thread_wait_queue_no_lock(manager->wait_queue_head, task_pid_nr(current), NULL);
	mutex_unlock(&manager->wq_lock);

	BUG_ON(IS_ERR(queue));

	kern_job = create_aipu_job(manager, user_job, queue, filp);
	if (IS_ERR(kern_job))
		return PTR_ERR(kern_job);

	/* LOCK */
	spin_lock_irqsave(&manager->lock, flags);
	if (do_trigger) {
		kern_job->state = AIPU_JOB_STATE_PENDING;
		list_add_tail(&kern_job->node, &manager->scheduled_head->node);

		/*
		 * For a job using SRAM managed by AIPU Gbuilder, it should be
		 * executed exclusively in serial with other Gbuilder-managed-SRAM ones,
		 * and parallel scheduling is not allowed.
		 *
		 * Pending it if there has been a Gbuilder-managed-SRAM job running;
		 * otherwise mark the flag and reserve a core for running.
		 */
		if (kern_job->desc.exec_flag & AIPU_JOB_EXEC_FLAG_SRAM_MUTEX) {
			if (manager->exec_flag & AIPU_JOB_EXEC_FLAG_SRAM_MUTEX)
				goto unlock;
			else
				manager->exec_flag |= AIPU_JOB_EXEC_FLAG_SRAM_MUTEX;
		}
		kern_job->core_id = get_available_core_no_lock(manager, kern_job);
		if (kern_job->core_id >= 0)
			reserve_core_for_job_no_lock(manager, kern_job, do_trigger);
	} else {
		if ((user_job->core_id >= manager->core_cnt) || (!manager->idle_bmap[user_job->core_id])) {
			ret = -EINVAL;
			goto unlock;
		}
		kern_job->state = AIPU_JOB_STATE_DEFERRED;
		kern_job->core_id = user_job->core_id;
		list_add_tail(&kern_job->node, &manager->scheduled_head->node);
		reserve_core_for_job_no_lock(manager, kern_job, do_trigger);
	}
unlock:
	spin_unlock_irqrestore(&manager->lock, flags);
	/* UNLOCK */

	return ret;
}

static int trigger_deferred_job_run(struct aipu_job_manager *manager, struct aipu_job_desc *user_job)
{
	unsigned long flags;
	struct aipu_job *curr = NULL;
	struct aipu_core *sched_core = NULL;
	int triggered = 0;

	/* LOCK */
	spin_lock_irqsave(&manager->lock, flags);
	list_for_each_entry(curr, &manager->scheduled_head->node, node) {
		if ((curr->uthread_id == task_pid_nr(current)) &&
			(curr->desc.job_id == user_job->job_id) &&
			(curr->state == AIPU_JOB_STATE_DEFERRED)) {
			curr->state = AIPU_JOB_STATE_RUNNING;
			sched_core = manager->cores[curr->core_id];
			sched_core->ops->trigger(sched_core);
			triggered = 1;
			break;
		}
	}
	spin_unlock_irqrestore(&manager->lock, flags);
	/* UNLOCK */

	if (!triggered)
		return -EINVAL;

	dev_info(sched_core->dev, "[Job %d of Thread %d] trigger deferred job running done\n",
	    user_job->job_id, task_pid_nr(current));
	return 0;
}

int aipu_job_manager_scheduler(struct aipu_job_manager *manager, struct aipu_job_desc *user_job,
	struct file *filp)
{
	int ret = 0;

	if (unlikely((!manager) || (!user_job) || (!filp)))
		return -EINVAL;

	if (unlikely(!is_user_job_valid(manager, user_job)))
		return -EINVAL;

	if (!user_job->is_defer_run)
		ret = schedule_new_job(manager, user_job, filp, 1);
	else if (!user_job->do_trigger)
		ret = schedule_new_job(manager, user_job, filp, 0);
	else
		ret = trigger_deferred_job_run(manager, user_job);

	return ret;
}

void aipu_job_manager_irq_upper_half(struct aipu_core *core, int exception_flag)
{
	struct aipu_job *curr = NULL;
	struct aipu_job_manager *manager = NULL;
	int handled = 0;
	int triggered = 0;

	if (unlikely(!core))
		return;

	manager = (struct aipu_job_manager *)core->job_manager;

	/* LOCK */
	spin_lock(&manager->lock);
	list_for_each_entry(curr, &manager->scheduled_head->node, node) {
		if ((curr->core_id == core->id) && (curr->state == AIPU_JOB_STATE_RUNNING)) {
			if (unlikely(exception_flag)) {
				curr->state = AIPU_JOB_STATE_EXCEP;
				dev_dbg(core->dev, "[IRQ, Job %d of Thread %d] ############ exception\n",
				    curr->desc.job_id, curr->uthread_id);
			} else {
				curr->state = AIPU_JOB_STATE_SUCCESS;
				dev_dbg(core->dev, "[IRQ, Job %d of Thread %d] ############ done\n",
				    curr->desc.job_id, curr->uthread_id);
			}

			if (curr->desc.enable_prof) {
				curr->done_time = ktime_get();
				core->ops->stop_bw_profiling(core);
				core->ops->read_profiling_reg(core, &curr->pdata);
			}

			if (curr->desc.exec_flag & AIPU_JOB_EXEC_FLAG_SRAM_MUTEX)
				manager->exec_flag &= ~AIPU_JOB_EXEC_FLAG_SRAM_MUTEX;

			handled = 1;
			break;
		}
	}

	if (unlikely(!handled))
		dev_dbg(core->dev, "[IRQ] a job was invalidated before done");

	if (!atomic_read(&core->disable)) {
		list_for_each_entry(curr, &manager->scheduled_head->node, node) {
			if ((curr->state == AIPU_JOB_STATE_PENDING) &&
				is_job_version_match(core, &curr->desc)) {
				if (curr->desc.exec_flag & AIPU_JOB_EXEC_FLAG_SRAM_MUTEX) {
					if (manager->exec_flag & AIPU_JOB_EXEC_FLAG_SRAM_MUTEX)
						continue;
					else
						manager->exec_flag |= AIPU_JOB_EXEC_FLAG_SRAM_MUTEX;
				}
				curr->core_id = core->id;
				reserve_core_for_job_no_lock(manager, curr, 1);
				triggered = 1;
				break;
			}
		}
	}

	if (!triggered)
		manager->idle_bmap[core->id] = 1;
	spin_unlock(&manager->lock);
	/* UNLOCK */
}

void aipu_job_manager_irq_bottom_half(struct aipu_core *core)
{
	struct aipu_job *curr = NULL;
	struct aipu_job *next = NULL;
	struct aipu_job_manager *manager = NULL;
	unsigned long flags;

	if (unlikely(!core))
		return;

	manager = core->job_manager;

	/* LOCK */
	spin_lock_irqsave(&manager->lock, flags);
	list_for_each_entry_safe(curr, next, &manager->scheduled_head->node, node) {
		if ((curr->state >= AIPU_JOB_STATE_EXCEP) && (!curr->wake_up) &&
		    (curr->core_id == core->id)) {
			if (curr->desc.enable_prof)
				curr->pdata.execution_time_ns =
				    (long)ktime_to_ns(ktime_sub(curr->done_time, curr->sched_time));
			wake_up_interruptible(curr->thread_queue);
			curr->wake_up = 1;
			dev_dbg(core->dev, "[BH, Job %d of Thread %d] waking up...\n",
			    curr->desc.job_id, curr->uthread_id);
		}
	}
	spin_unlock_irqrestore(&manager->lock, flags);
	/* UNLOCK */
}

int aipu_job_manager_cancel_jobs(struct aipu_job_manager *manager, struct file *filp)
{
	unsigned long flags;
	struct aipu_job *curr = NULL;
	struct aipu_job *next = NULL;
	struct aipu_thread_wait_queue *curr_wq = NULL;
	struct aipu_thread_wait_queue *next_wq = NULL;

	if ((!manager) || (!filp))
		return -EINVAL;

	/* jobs should be cleaned first */
	spin_lock_irqsave(&manager->lock, flags);
	list_for_each_entry_safe(curr, next, &manager->scheduled_head->node, node) {
		if (curr->filp == filp) {
			if (curr->state == AIPU_JOB_STATE_DEFERRED)
				manager->idle_bmap[curr->core_id] = 1;
			remove_aipu_job(manager, curr);
		}
	}
	spin_unlock_irqrestore(&manager->lock, flags);

	mutex_lock(&manager->wq_lock);
	list_for_each_entry_safe(curr_wq, next_wq, &manager->wait_queue_head->node, node) {
		if (!curr_wq->ref_cnt) {
			list_del(&curr_wq->node);
			kfree(curr_wq);
		}
	}
	mutex_unlock(&manager->wq_lock);

	return 0;
}

int aipu_job_manager_invalidate_timeout_job(struct aipu_job_manager *manager, int job_id)
{
	int ret = 0;
	struct aipu_job *curr = NULL;
	struct aipu_job *next = NULL;
	unsigned long flags;

	if (!manager)
		return -EINVAL;

	/* LOCK */
	spin_lock_irqsave(&manager->lock, flags);
	list_for_each_entry_safe(curr, next, &manager->scheduled_head->node, node) {
		if ((curr->uthread_id == task_pid_nr(current)) &&
		    (curr->desc.job_id == job_id)) {
			remove_aipu_job(manager, curr);
			break;
		}
	}
	spin_unlock_irqrestore(&manager->lock, flags);
	/* UNLOCK */

	return ret;
}

int aipu_job_manager_get_job_status(struct aipu_job_manager *manager, struct aipu_job_status_query *job_status,
	struct file *filp)
{
	int ret = 0;
	struct aipu_job_status_desc *status = NULL;
	struct aipu_job *curr = NULL;
	struct aipu_job *next = NULL;
	int poll_iter = 0;
	unsigned long flags;

	if (unlikely((!manager) || (!job_status) || (job_status->max_cnt < 1)))
		return -EINVAL;

	status = kcalloc(job_status->max_cnt, sizeof(*status), GFP_KERNEL);
	if (!status)
		return -ENOMEM;

	job_status->poll_cnt = 0;
	spin_lock_irqsave(&manager->lock, flags);
	list_for_each_entry_safe(curr, next, &manager->scheduled_head->node, node) {
		if (job_status->poll_cnt == job_status->max_cnt)
			break;

		if (curr->state < AIPU_JOB_STATE_EXCEP)
			continue;

		if (curr->filp != filp)
			continue;

		if (((job_status->of_this_thread) && (curr->uthread_id == task_pid_nr(current))) ||
			(!job_status->of_this_thread)) {
			/* update status info */
			status[poll_iter].job_id = curr->desc.job_id;
			status[poll_iter].thread_id = curr->uthread_id;
			status[poll_iter].state = (curr->state == AIPU_JOB_STATE_SUCCESS) ?
			    AIPU_JOB_STATE_DONE : AIPU_JOB_STATE_EXCEPTION;
			if (curr->desc.enable_prof)
				status[poll_iter].pdata = curr->pdata;

			dev_dbg(manager->cores[curr->core_id]->dev, "[Job %d of Thread %d] got status\n",
				curr->desc.job_id, curr->uthread_id);

			/* remove status from kernel */
			remove_aipu_job(manager, curr);
			curr = NULL;

			/* update iterator */
			job_status->poll_cnt++;
			poll_iter++;
		}
	}
	spin_unlock_irqrestore(&manager->lock, flags);

	if (WARN_ON(!job_status->poll_cnt)) {
		ret = -EFAULT;
		goto clean;
	}

	ret = copy_to_user((struct job_status_desc __user *)job_status->status, status,
		job_status->poll_cnt * sizeof(*status));

clean:
	kfree(status);
	return ret;
}

int aipu_job_manager_has_end_job(struct aipu_job_manager *manager, struct file *filp,
	struct poll_table_struct *wait, int uthread_id)
{
	int ret = 0;
	struct aipu_job *curr = NULL;
	struct aipu_thread_wait_queue *wq = NULL;
	unsigned long flags;

	if (unlikely((!manager) || (!filp)))
		return -EINVAL;

	mutex_lock(&manager->wq_lock);
	list_for_each_entry(wq, &manager->wait_queue_head->node, node) {
		if ((wq->uthread_id == uthread_id) || (wq->filp == filp)) {
			poll_wait(filp, &wq->p_wait, wait);
			break;
		}
	}
	mutex_unlock(&manager->wq_lock);

	spin_lock_irqsave(&manager->lock, flags);
	list_for_each_entry(curr, &manager->scheduled_head->node, node) {
		if ((curr->filp == filp) &&
		    (curr->state >= AIPU_JOB_STATE_EXCEP) &&
		    (curr->desc.enable_poll_opt || (curr->uthread_id == uthread_id))) {
			ret = 1;
			break;
		}
	}
	spin_unlock_irqrestore(&manager->lock, flags);

	return ret;
}
