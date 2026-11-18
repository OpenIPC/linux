/*
 * (C) Copyright 2009 Intel Corporation
 * Author: Jacob Pan (jacob.jun.pan@intel.com)
 *
 * Shared with ARM platforms, Jamie Iles, Picochip 2011
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * Support for the Synopsys DesignWare APB Timers.
 */
#ifndef __AX_HRTIMER_H__
#define __AX_HRTIMER_H__


struct ax_hrtimer {
	struct rb_node node;
	struct list_head list;
	int (*function) (void *);
	unsigned long delay;
	struct task_struct *task;
	int id;
	unsigned long expires;
	s64 start_time;
	atomic_t timer_wake;
	void *private;
};

struct ax_hrtimer *ax_hrtimer_alloc(void);
void ax_hrtimer_destroy(struct ax_hrtimer *timer);
int ax_hrtimer_start(struct ax_hrtimer *timer);
int ax_hrtimer_stop(struct ax_hrtimer *timer);
long ax_usleep(unsigned long use);

#endif


