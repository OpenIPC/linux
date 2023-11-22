/*
 * xrp_private_alloc.h- Sigmastar
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

#ifndef _XRP_PRIVATE_ALLOC_H_
#define _XRP_PRIVATE_ALLOC_H_

#include "xrp_alloc.h"

long xrp_init_private_pool(struct xrp_allocation_pool **pool, phys_addr_t start, u32 size);

#endif /*_XRP_PRIVATE_ALLOC_H_*/
