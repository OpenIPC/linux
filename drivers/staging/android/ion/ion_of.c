/*
 * Based on work from:
 *   Andrew Andrianov <andrew@ncrmnt.org>
 *   Google
 *   The Linux Foundation
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/of.h>
#include <linux/of_platform.h>
#include <linux/of_address.h>
#include <linux/clk.h>
#include <linux/dma-mapping.h>
#include <linux/cma.h>
#include <linux/dma-contiguous.h>
#include <linux/io.h>
#include <linux/of_reserved_mem.h>
#include <linux/memblock.h>
#include "ion.h"
#include "ion_priv.h"
#include "ion_of.h"
#include <linux/ion_sunxi.h>

#ifdef CONFIG_ARM64
struct ion_mem_reserve_info ion_reserve_mem;
#endif
extern struct ion_mem_reserve_info ion_reserve_mem;
static int ion_parse_dt_heap_common(struct device_node *heap_node,
				    struct ion_platform_heap *heap,
				    struct ion_of_heap *compatible)
{
	int i;

	for (i = 0; compatible[i].name; i++) {
		if (of_device_is_compatible(heap_node, compatible[i].compat))
			break;
	}

	if (!compatible[i].name)
		return -ENODEV;

	heap->id = compatible[i].heap_id;
	heap->type = compatible[i].type;
	heap->name = compatible[i].name;
	heap->align = compatible[i].align;
	pr_info("%s: id %d type %d name %s align %lx\n", __func__,
		heap->id, heap->type, heap->name, heap->align);

	if (heap->id == ION_HEAP_TYPE_CARVEOUT) {
		u32 dts_reserve_base, dts_reserve_size, dts_reserve_end;
		int ret = 0;

		ret = of_property_read_u32(heap_node, "heap-base", &dts_reserve_base);
		ret = of_property_read_u32(heap_node, "heap-size", &dts_reserve_size);
		if ((ret == 0) && (ion_reserve_mem.size == 0)) {
			/*Only set carveout reserve info in dts*/
			heap->base = dts_reserve_base;
			heap->size = dts_reserve_size;
			pr_info("%s: carveout(dts): base 0x%x, size 0x%x\n",
					__func__, (unsigned int)heap->base, (unsigned int)heap->size);
			return 0;
		} else if ((ret != 0) && (ion_reserve_mem.size != 0)) {
			/*Only set carveout reserve info in cmdline*/
			heap->base = ion_reserve_mem.start;
			heap->size = ion_reserve_mem.size;
			pr_info("%s: carveout(ion_carveout_list-1): base 0x%x, size 0x%x\n",
					__func__, (unsigned int)heap->base, (unsigned int)heap->size);
			return 0;
		} else if ((ret != 0) && (ion_reserve_mem.size == 0)) {
			/*No carveout reserve info be set in dts or cmdline*/
			pr_err("You need to set carveout reserve info in dts or cmdline!\n");
			return 0;
		}

		/*If the heap or size in dts are zero, just use the cmdline config.*/
		if ((heap->base == 0) || (heap->size == 0)) {
			heap->base = ion_reserve_mem.start;
			heap->size = ion_reserve_mem.size;
			pr_info("%s: carveout(ion_carveout_list-2): base 0x%x, size 0x%x\n",
					__func__, (unsigned int)heap->base, (unsigned int)heap->size);
			return 0;
		}
		/*
		 * Carveout reserve info was set in dts(not zero) and cmdline at the same time.
		 *
		 * In this case, the priority is : cmdline > dts,
		 * so we should free the extra reserve mem caused by dts reserve.
		 *
		 */
		dts_reserve_end = dts_reserve_base + dts_reserve_size;
		if (dts_reserve_end <= ion_reserve_mem.start) {
			/*Free all dts reserve mem*/
			memblock_free(dts_reserve_base, dts_reserve_size);
			free_reserved_area((void *)__phys_to_virt(dts_reserve_base),
						(void *)__phys_to_virt(dts_reserve_end), -1, NULL);
		} else {
			if ((ion_reserve_mem.start - dts_reserve_base) > 0) {
				/*Free part of dts reserve mem*/
				memblock_free(dts_reserve_base, ion_reserve_mem.start - dts_reserve_base);
				free_reserved_area((void *)__phys_to_virt(dts_reserve_base),
							(void *)__phys_to_virt(ion_reserve_mem.start), -1, NULL);
			}
		}
		heap->base = ion_reserve_mem.start;
		heap->size = ion_reserve_mem.size;
		pr_info("%s: carveout(ion_carveout_list-3): base 0x%x, size 0x%x\n",
				__func__, (unsigned int)heap->base, (unsigned int)heap->size);
	}

	return 0;
}

static int ion_setup_heap_common(struct platform_device *parent,
				 struct device_node *heap_node,
				 struct ion_platform_heap *heap)
{
	int ret = 0;

	switch (heap->type) {
	case ION_HEAP_TYPE_CARVEOUT:
	case ION_HEAP_TYPE_CHUNK:
		if (heap->base && heap->size)
			return 0;

		ret = of_reserved_mem_device_init(heap->priv);
		break;
	default:
		break;
	}

	return ret;
}

struct ion_platform_data *ion_parse_dt(struct platform_device *pdev,
				       struct ion_of_heap *compatible)
{
	int num_heaps, ret;
	const struct device_node *dt_node = pdev->dev.of_node;
	struct device_node *node;
	struct ion_platform_heap *heaps;
	struct ion_platform_data *data;
	int i = 0;

	num_heaps = of_get_available_child_count(dt_node);

	if (!num_heaps)
		return ERR_PTR(-EINVAL);

	heaps = devm_kzalloc(&pdev->dev,
			     sizeof(struct ion_platform_heap) * num_heaps,
			     GFP_KERNEL);
	if (!heaps)
		return ERR_PTR(-ENOMEM);

	data = devm_kzalloc(&pdev->dev, sizeof(struct ion_platform_data),
			    GFP_KERNEL);
	if (!data)
		return ERR_PTR(-ENOMEM);

	for_each_available_child_of_node(dt_node, node) {
		struct platform_device *heap_pdev;

		ret = ion_parse_dt_heap_common(node, &heaps[i], compatible);
		if (ret)
			return ERR_PTR(ret);

		heap_pdev = of_platform_device_create(node, heaps[i].name,
						      &pdev->dev);
		if (!heap_pdev)
			return ERR_PTR(-ENOMEM);
		heap_pdev->dev.platform_data = &heaps[i];

		heaps[i].priv = &heap_pdev->dev;

		ret = ion_setup_heap_common(pdev, node, &heaps[i]);
		if (ret)
			goto out_err;
		i++;
	}

	data->heaps = heaps;
	data->nr = num_heaps;
	return data;

out_err:
	for ( ; i >= 0; i--)
		if (heaps[i].priv)
			of_device_unregister(to_platform_device(heaps[i].priv));

	return ERR_PTR(ret);
}

void ion_destroy_platform_data(struct ion_platform_data *data)
{
	int i;

	for (i = 0; i < data->nr; i++)
		if (data->heaps[i].priv)
			of_device_unregister(to_platform_device(
				data->heaps[i].priv));
}

#ifdef CONFIG_OF_RESERVED_MEM
#include <linux/of.h>
#include <linux/of_fdt.h>
#include <linux/of_reserved_mem.h>

static int rmem_ion_device_init(struct reserved_mem *rmem, struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct ion_platform_heap *heap = pdev->dev.platform_data;

	heap->base = rmem->base;
	heap->base = rmem->size;
	pr_debug("%s: heap %s base %pa size %pa dev %p\n", __func__,
		 heap->name, &rmem->base, &rmem->size, dev);
	return 0;
}

static void rmem_ion_device_release(struct reserved_mem *rmem,
				    struct device *dev)
{
	return;
}

static const struct reserved_mem_ops rmem_dma_ops = {
	.device_init	= rmem_ion_device_init,
	.device_release	= rmem_ion_device_release,
};

static int __init rmem_ion_setup(struct reserved_mem *rmem)
{
	phys_addr_t size = rmem->size;

	size = size / 1024;

	pr_info("Ion memory setup at %pa size %pa MiB\n",
		&rmem->base, &size);
	rmem->ops = &rmem_dma_ops;
	return 0;
}

RESERVEDMEM_OF_DECLARE(ion, "ion-region", rmem_ion_setup);
#endif
