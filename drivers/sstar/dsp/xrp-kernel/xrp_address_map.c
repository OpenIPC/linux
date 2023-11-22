/*
 * xrp_address_map.c- Sigmastar
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

#include <linux/bsearch.h>
#include <linux/device.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/slab.h>
#include <linux/sort.h>
#include "xrp_address_map.h"

#ifndef DEBUG
#define DEBUG
#endif
#if IS_ENABLED(CONFIG_OF)
static int xrp_compare_address_sort(const void *a, const void *b)
{
    const struct xrp_address_map_entry *pa = a;
    const struct xrp_address_map_entry *pb = b;

    if (pa->src_addr < pb->src_addr && pb->src_addr - pa->src_addr >= pa->size)
        return -1;
    if (pa->src_addr > pb->src_addr && pa->src_addr - pb->src_addr >= pb->size)
        return 1;
    return 0;
}
#endif

int xrp_init_address_map(struct device *dev, struct xrp_address_map *map)
{
    int ret = 0;
#if IS_ENABLED(CONFIG_OF)
    struct device_node *pnode = dev->of_node;
    struct device_node *node;
    int                 rlen, off;
    const __be32 *      ranges = of_get_property(pnode, "ranges", &rlen);
    int                 na, pna, ns;
    int                 i;
    pr_err("%s %d\n", __FUNCTION__, __LINE__);
    if (!ranges)
    {
        pr_err("%s: no 'ranges' property in the device tree, no translation at that level\n", __func__);
        goto empty;
    }
    pr_err("%s %d\n", __FUNCTION__, __LINE__);

    node = of_get_next_child(pnode, NULL);
    if (!node)
    {
        pr_err("%s: no child node found in the device tree, no translation at that level\n", __func__);
        goto empty;
    }
    pr_err("%s %d\n", __FUNCTION__, __LINE__);

    na  = of_n_addr_cells(node);
    ns  = of_n_size_cells(node);
    pna = of_n_addr_cells(pnode);

    rlen /= 4;
    map->n     = rlen / (na + pna + ns);
    map->entry = kmalloc_array(map->n, sizeof(*map->entry), GFP_KERNEL);
    if (!map->entry)
    {
        ret = -ENOMEM;
        goto err;
    }
    pr_err("%s %d\n", __FUNCTION__, __LINE__);

    pr_err("%s: na = %d, pna = %d, ns = %d, rlen = %d cells, n = %d\n", __func__, na, pna, ns, rlen, map->n);

    for (off = 0, i = 0; off < rlen; off += na + pna + ns, ++i)
    {
        map->entry[i].src_addr = of_translate_address(node, ranges + off);
        map->entry[i].dst_addr = of_read_number(ranges + off, na);
        map->entry[i].size     = of_read_number(ranges + off + na + pna, ns);
        pr_err("  src_addr = 0x%llx, dst_addr = 0x%lx, size = 0x%lx\n", (unsigned long long)map->entry[i].src_addr,
               (unsigned long)map->entry[i].dst_addr, (unsigned long)map->entry[i].size);
    }
    pr_err("%s %d\n", __FUNCTION__, __LINE__);

    sort(map->entry, map->n, sizeof(*map->entry), xrp_compare_address_sort, NULL);
err:
    of_node_put(node);
    return ret;

empty:
#endif
    map->n               = 1;
    map->entry           = kmalloc(sizeof(*map->entry), GFP_KERNEL);
    map->entry->src_addr = 0;
    map->entry->dst_addr = 0;
    map->entry->size     = ~0u;
    return ret;
}

void xrp_free_address_map(struct xrp_address_map *map)
{
    kfree(map->entry);
}

static int xrp_compare_address_search(const void *a, const void *b)
{
    const phys_addr_t *pa = a;
    return xrp_compare_address(*pa, b);
}

struct xrp_address_map_entry *xrp_get_address_mapping(const struct xrp_address_map *map, phys_addr_t addr)
{
    struct xrp_address_map_entry *entry =
        bsearch(&addr, map->entry, map->n, sizeof(*map->entry), xrp_compare_address_search);
    return entry;
}

u32 xrp_translate_to_dsp(const struct xrp_address_map *map, phys_addr_t addr)
{
    struct xrp_address_map_entry *entry = xrp_get_address_mapping(map, addr);

    if (!entry)
        return XRP_NO_TRANSLATION;
    return entry->dst_addr + addr - entry->src_addr;
}
