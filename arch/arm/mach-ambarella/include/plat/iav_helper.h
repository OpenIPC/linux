/*
 * arch/arm/plat-ambarella/include/plat/service.h
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

#ifndef __PLAT_AMBARELLA_SERVICE_H
#define __PLAT_AMBARELLA_SERVICE_H

#ifndef __ASSEMBLER__

/*===========================================================================*/

/* gpio service for private operation */
enum ambsvc_gpio_service_id {
	AMBSVC_GPIO_REQUEST = 0,
	AMBSVC_GPIO_OUTPUT,
	AMBSVC_GPIO_INPUT,
	AMBSVC_GPIO_FREE,
	AMBSVC_GPIO_TO_IRQ,
};

struct ambsvc_gpio {
	int svc_id;
	int gpio;
	int value;
};

/* pll service for private operation */
enum ambsvc_pll_service_id {
	AMBSVC_PLL_GET_RATE = 0,
	AMBSVC_PLL_SET_RATE = 1,
};

struct ambsvc_pll {
	int svc_id;
	char *name;
	int rate;
};

enum ambarella_service_id {
	AMBARELLA_SERVICE_GPIO = 1,
	AMBARELLA_SERVICE_PLL = 2,
};

typedef int (*ambarella_service_func)(void *arg, void *result);

struct ambarella_service {
	int service;
	ambarella_service_func func;
	struct list_head node;
};

extern int ambarella_register_service(struct ambarella_service *amb_svc);
extern int ambarella_unregister_service(struct ambarella_service *amb_svc);
extern int ambarella_request_service(int service, void *arg, void *result);

/*===========================================================================*/
extern void ambcache_clean_all(void *addr, unsigned int size);
extern void ambcache_clean_range(void *addr, unsigned int size);
extern void ambcache_inv_range(void *addr, unsigned int size);

/*===========================================================================*/

#endif /* __ASSEMBLER__ */

#endif

