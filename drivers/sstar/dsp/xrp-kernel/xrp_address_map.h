/*
 * xrp_address_map.h- Sigmastar
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

#ifndef _XRP_ADDRESS_MAP_H_
#define _XRP_ADDRESS_MAP_H_

#include <linux/types.h>

#define XRP_NO_TRANSLATION ((u32)~0ul)

struct xrp_address_map_entry
{
    phys_addr_t src_addr;
    u32         dst_addr;
    u32         size;
};

struct xrp_address_map
{
    unsigned                      n;
    struct xrp_address_map_entry* entry;
};

int xrp_init_address_map(struct device* dev, struct xrp_address_map* map);

void xrp_free_address_map(struct xrp_address_map* map);

struct xrp_address_map_entry* xrp_get_address_mapping(const struct xrp_address_map* map, phys_addr_t addr);

u32 xrp_translate_to_dsp(const struct xrp_address_map* map, phys_addr_t addr);

static inline int xrp_compare_address(phys_addr_t addr, const struct xrp_address_map_entry* entry)
{
    if (addr < entry->src_addr)
    {
        return -1;
    }

    if (addr - entry->src_addr < entry->size)
    {
        return 0;
    }

    return 1;
}

#endif /*_XRP_ADDRESS_MAP_H_*/
