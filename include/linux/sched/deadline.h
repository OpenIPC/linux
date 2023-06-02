#ifndef _SCHED_DEADLINE_H
#define _SCHED_DEADLINE_H

/*
 * SCHED_DEADLINE tasks has negative priorities, reflecting
 * the fact that any of them has higher prio than RT and
 * NORMAL/BATCH tasks.
 */

#define MAX_DL_PRIO		0

static inline int dl_prio(int prio)
{
	if (IS_ENABLED(CONFIG_SCHED_DL) && unlikely(prio < MAX_DL_PRIO))
		return 1;
	return 0;
}

static inline int dl_task(struct task_struct *p)
{
	return dl_prio(p->prio);
}

static inline bool dl_time_before(u64 a, u64 b)
{
	return (s64)(a - b) < 0;
}

#ifdef CONFIG_SCHED_DL
#define dl_deadline(tsk)	((tsk)->dl.deadline)
#else
#define dl_deadline(tsk)	0
#endif

#endif /* _SCHED_DEADLINE_H */
