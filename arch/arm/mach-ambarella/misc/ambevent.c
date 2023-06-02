/*
 * arch/arm/plat-ambarella/misc/ambevent.c
 *
 * Author: Zhenwu Xue, <zwxue@ambarella.com>
 *
 * Copyright (C) 2004-2010, Ambarella, Inc.
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
#include <linux/export.h>
#include <linux/errno.h>
#include <linux/bootmem.h>
#include <linux/platform_device.h>
#include <plat/ambevent.h>

int amb_event_pool_init(struct amb_event_pool *pool)
{
	if (!pool)
		return -EINVAL;

	memset(pool, 0, sizeof(struct amb_event_pool));
	mutex_init(&pool->op_mutex);
	return 0;
}
EXPORT_SYMBOL(amb_event_pool_init);

int amb_event_pool_affuse(struct amb_event_pool *pool,
	struct amb_event event)
{
	if (!pool)
		return -EINVAL;

	if (event.type == AMB_EV_NONE)
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
EXPORT_SYMBOL(amb_event_pool_affuse);

int amb_event_pool_query_index(struct amb_event_pool *pool)
{
	unsigned char			index;

	if (!pool)
		return -EINVAL;

	mutex_lock(&pool->op_mutex);
	index = pool->ev_index - 1;
	mutex_unlock(&pool->op_mutex);

	return (int)index;
}
EXPORT_SYMBOL(amb_event_pool_query_index);

int amb_event_pool_query_event(struct amb_event_pool *pool,
	struct amb_event *event, unsigned char index)
{
	int				retval = 0;

	if (!pool || !event)
		return -EINVAL;

	mutex_lock(&pool->op_mutex);

	if (pool->events[index].type == AMB_EV_NONE) {
		retval = -EAGAIN;
		goto amb_event_pool_query_event_exit;
	}

	if (index == pool->ev_index) {
		retval = -EAGAIN;
		goto amb_event_pool_query_event_exit;
	}

	*event = pool->events[index];

amb_event_pool_query_event_exit:
	mutex_unlock(&pool->op_mutex);
	return retval;
}
EXPORT_SYMBOL(amb_event_pool_query_event);

int amb_event_report_uevent(struct kobject *kobj, enum kobject_action action,
		       char *envp_ext[])
{
	return kobject_uevent_env(kobj, action, envp_ext);
}
EXPORT_SYMBOL(amb_event_report_uevent);

