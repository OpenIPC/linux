/*
 * arch/arm/plat-ambarella/misc/service.c
 *
 * Author: Cao Rongrong <rrcao@ambarella.com>
 *
 * Copyright (C) 2012-2016, Ambarella, Inc.
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
#include <plat/iav_helper.h>
#include <mach/init.h>
#include <asm/cacheflush.h>


/*===========================================================================*/

static LIST_HEAD(ambarella_svc_list);

int ambarella_register_service(struct ambarella_service *amb_svc)
{
	struct ambarella_service *svc;

	if (!amb_svc || !amb_svc->func)
		return -EINVAL;

	list_for_each_entry(svc, &ambarella_svc_list, node) {
		if (svc->service == amb_svc->service) {
			pr_err("%s: service (%d) is already existed\n",
					__func__, amb_svc->service);
			return -EEXIST;
		}
	}

	list_add_tail(&amb_svc->node, &ambarella_svc_list);

	return 0;
}
EXPORT_SYMBOL(ambarella_register_service);

int ambarella_unregister_service(struct ambarella_service *amb_svc)
{
	struct ambarella_service *svc;

	if (!amb_svc)
		return -EINVAL;

	list_for_each_entry(svc, &ambarella_svc_list, node) {
		if (svc->service == amb_svc->service) {
			list_del(&svc->node);
			break;
		}
	}

	return 0;
}
EXPORT_SYMBOL(ambarella_unregister_service);

int ambarella_request_service(int service, void *arg, void *result)
{
	struct ambarella_service *svc;
	int found = 0;

	list_for_each_entry(svc, &ambarella_svc_list, node) {
		if (svc->service == service) {
			found = 1;
			break;
		}
	}

	if (found == 0) {
		pr_err("%s: no such service (%d)\n", __func__, service);
		return -ENODEV;
	}

	return svc->func(arg, result);
}
EXPORT_SYMBOL(ambarella_request_service);

/*===========================================================================*/

void ambcache_clean_range(void *addr, unsigned int size)
{
	__sync_cache_range_w(addr, size);
}
EXPORT_SYMBOL(ambcache_clean_range);

void ambcache_inv_range(void *addr, unsigned int size)
{
	__sync_cache_range_r(addr, size);
}

EXPORT_SYMBOL(ambcache_inv_range);


