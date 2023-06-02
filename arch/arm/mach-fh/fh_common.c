/*
 * fh_common.c
 *
 * Copyright (C) 2018 Fullhan Microelectronics Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <linux/types.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <mach/system.h>

/**
 * fh_setscheduler - change the scheduling policy and/or RT priority of `current` thread.
 * @policy: new policy. egz. SCHED_RR
 * @priority: new priority. egz MAX_USER_RT_PRIO-12
 * @param: structure containing the new RT priority.
 *
 * NOTE that the task may be already dead.
 */
void fh_setscheduler(int policy, int priority)
{
	struct sched_param param;
	param.sched_priority = priority;
	sched_setscheduler(current, policy, &param);
}
EXPORT_SYMBOL(fh_setscheduler);
