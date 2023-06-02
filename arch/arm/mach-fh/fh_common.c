/*
 * fh_common.c
 *
 * Copyright (C) 2019 Fullhan Microelectronics Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <linux/types.h>
#include <linux/sched.h>
#include <mach/fh_common.h>

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

int fh_clk_enable(struct clk *clk)
{
	return clk_enable(clk);
}
EXPORT_SYMBOL(fh_clk_enable);

unsigned long fh_clk_get_rate(struct clk *clk)
{
	return clk_get_rate(clk);
}
EXPORT_SYMBOL(fh_clk_get_rate);

int fh_clk_set_rate(struct clk *clk, unsigned long rate)
{
	return clk_set_rate(clk, rate);
}
EXPORT_SYMBOL(fh_clk_set_rate);

int fh_clk_prepare(struct clk *clk)
{
	return clk_prepare(clk);
}
EXPORT_SYMBOL(fh_clk_prepare);

void fh_clk_disable(struct clk *clk)
{
	clk_disable(clk);
}
EXPORT_SYMBOL(fh_clk_disable);

void fh_clk_unprepare(struct clk *clk)
{
	clk_unprepare(clk);
}
EXPORT_SYMBOL(fh_clk_unprepare);

int fh_clk_prepare_enable(struct clk *clk)
{
	return clk_prepare_enable(clk);
}
EXPORT_SYMBOL(fh_clk_prepare_enable);

void fh_clk_disable_unprepare(struct clk *clk)
{
	clk_disable_unprepare(clk);
}
EXPORT_SYMBOL(fh_clk_disable_unprepare);

bool fh_clk_is_enabled(struct clk *clk)
{
	return __clk_is_enabled(clk);
}
EXPORT_SYMBOL(fh_clk_is_enabled);


int fh_clk_set_parent(struct clk *clk, struct clk *parent)
{
	return clk_set_parent(clk, parent);
}
EXPORT_SYMBOL(fh_clk_set_parent);

struct clk *fh_clk_get_parent(struct clk *clk)
{
	return clk_get_parent(clk);
}
EXPORT_SYMBOL(fh_clk_get_parent);

bool fh_clk_has_parent(struct clk *clk, struct clk *parent)
{
	return clk_has_parent(clk, parent);
}
EXPORT_SYMBOL(fh_clk_has_parent);

unsigned int fh_irq_create_mapping(struct irq_domain *domain,
				irq_hw_number_t hwirq)
{
	return irq_create_mapping(domain, hwirq);
}
EXPORT_SYMBOL(fh_irq_create_mapping);
