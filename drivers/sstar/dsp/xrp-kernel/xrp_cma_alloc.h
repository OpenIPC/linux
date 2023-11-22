/*
 * xrp_cma_alloc.h- Sigmastar
 *
 * Copyright (c) [2019~2020] SigmaStar Technology.
 *
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License version 2 for more details.
 *
 */

#ifndef _XRP_CMA_ALLOC_H_
#define _XRP_CMA_ALLOC_H_

#include "xrp_alloc.h"

struct device;

#ifdef CONFIG_CMA
long xrp_init_cma_pool(struct xrp_allocation_pool** pool, struct device* dev);
#else
static inline long xrp_init_cma_pool(struct xrp_allocation_pool** pool, struct device* dev)
{
    return -ENXIO;
}
#endif

#endif /*_XRP_CMA_ALLOC_H_*/
