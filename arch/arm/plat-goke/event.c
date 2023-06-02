/*
 * arch/arm/mach-gk/event.c
 *
 * Author: Steven Yu, <yulindeng@gokemicro.com>
 * Copyright (C) 2012-2015, goke, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/cpu.h>
#include <linux/platform_device.h>
#include <linux/dma-mapping.h>
#include <linux/errno.h>
#include <linux/bootmem.h>
#include <mach/event.h>


#include <mach/hardware.h>

/* ==========================================================================*/
static BLOCKING_NOTIFIER_HEAD(blocking_event_list);
static RAW_NOTIFIER_HEAD(raw_event_list);

/* ==========================================================================*/
int gk_register_event_notifier(void *nb)
{
	return blocking_notifier_chain_register(&blocking_event_list, nb);
}
EXPORT_SYMBOL(gk_register_event_notifier);

int gk_unregister_event_notifier(void *nb)
{
	return blocking_notifier_chain_unregister(&blocking_event_list, nb);
}
EXPORT_SYMBOL(gk_unregister_event_notifier);

int gk_set_event(unsigned long val, void *v)
{
	return blocking_notifier_call_chain(&blocking_event_list, val, v);
}
EXPORT_SYMBOL(gk_set_event);

int gk_register_raw_event_notifier(void *nb)
{
	return raw_notifier_chain_register(&raw_event_list, nb);
}
EXPORT_SYMBOL(gk_register_raw_event_notifier);

int gk_unregister_raw_event_notifier(void *nb)
{
	return raw_notifier_chain_unregister(&raw_event_list, nb);
}
EXPORT_SYMBOL(gk_unregister_raw_event_notifier);

int gk_set_raw_event(unsigned long val, void *v)
{
	return raw_notifier_call_chain(&raw_event_list, val, v);
}
EXPORT_SYMBOL(gk_set_raw_event);

int gk_event_pool_init(struct gk_event_pool *pool)
{
	if (!pool)
		return -EINVAL;

	memset(pool, 0, sizeof(struct gk_event_pool));
	mutex_init(&pool->op_mutex);
	return 0;
}
EXPORT_SYMBOL(gk_event_pool_init);

int gk_event_pool_affuse(struct gk_event_pool *pool,
	struct gk_event event)
{
	if (!pool)
		return -EINVAL;

	if (event.type == GK_EV_NONE)
		return 0;

	mutex_lock(&pool->op_mutex);
	pool->ev_sno++;
	pool->events[pool->ev_index].sno = pool->ev_sno;
	pool->events[pool->ev_index].time_code = 0;		//FIX ME
	pool->events[pool->ev_index].type = event.type;
	memcpy(pool->events[pool->ev_index].data, event.data, sizeof(event.data));
	pool->ev_index++;
	mutex_unlock(&pool->op_mutex);

	return 0;
}
EXPORT_SYMBOL(gk_event_pool_affuse);

int gk_event_pool_query_index(struct gk_event_pool *pool)
{
	unsigned char			index;

	if (!pool)
		return -EINVAL;

	mutex_lock(&pool->op_mutex);
	index = pool->ev_index - 1;
	mutex_unlock(&pool->op_mutex);

	return (int)index;
}
EXPORT_SYMBOL(gk_event_pool_query_index);

int gk_event_pool_query_event(struct gk_event_pool *pool,
	struct gk_event *event, unsigned char index)
{
	int				retval = 0;

	if (!pool || !event)
		return -EINVAL;

	mutex_lock(&pool->op_mutex);

	if (pool->events[index].type == GK_EV_NONE) {
		retval = -EAGAIN;
		goto gk_event_pool_query_event_exit;
	}

	if (index == pool->ev_index) {
		retval = -EAGAIN;
		goto gk_event_pool_query_event_exit;
	}

	*event = pool->events[index];

gk_event_pool_query_event_exit:
	mutex_unlock(&pool->op_mutex);
	return retval;
}
EXPORT_SYMBOL(gk_event_pool_query_event);

int gk_event_report_uevent(struct kobject *kobj, enum kobject_action action,
		       char *envp_ext[])
{
	return kobject_uevent_env(kobj, action, envp_ext);
}
EXPORT_SYMBOL(gk_event_report_uevent);


