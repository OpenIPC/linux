// SPDX-License-Identifier: GPL-2.0
/*
 * drivers/rpbuf/rpbuf_controller_sunxi.c
 *
 * (C) Copyright 2020-2025
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 * Junyan Lin <junyanlin@allwinnertech.com>
 *
 * Allwinner RPBuf driver.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/of_reserved_mem.h>
#include <linux/dma-mapping.h>
#include <linux/iommu.h>
#include <linux/ion_sunxi.h>

#include "rpbuf_internal.h"

struct ion_list {
	struct ion_handle *handle;
	struct list_head list;
	struct rpbuf_buffer *buffer;
};

struct sunxi_rpbuf_controller_priv {
	int ctrl_id;
	struct iommu_domain *domain;
	struct ion_client *ion;
	struct list_head list;
};

static int sunxi_rpbuf_alloc_has_iommu(struct device *dev,
				struct sunxi_rpbuf_controller_priv *inst,
				struct rpbuf_buffer *buffer)
{
	void *va;
	dma_addr_t pa;
	int ret;
	struct ion_handle *handle;
	struct ion_list *ion_list;

	ion_list = devm_kzalloc(dev, sizeof(*ion_list), GFP_KERNEL);
	if (IS_ERR_OR_NULL(ion_list)) {
		ret = -ENOMEM;
		goto err_out;
	}

	handle = ion_alloc(inst->ion, buffer->len, PAGE_SIZE,
						ION_HEAP_SYSTEM_MASK, 0);
	if (IS_ERR_OR_NULL(handle)) {
		dev_err(dev, "ion_alloc for len %d failed\n", buffer->len);
		ret = -ENOMEM;
		goto err_ion_alloc;
	}

	/* map to kernel space */
	va = ion_map_kernel(inst->ion, handle);
	if (IS_ERR_OR_NULL(va)) {
		dev_err(dev, "ion_map_kernel failed\n");
		ret = -ENOMEM;
		goto err_map_kernel;
	}

	/* get physical address */
	ret = dma_map_sg_attrs(get_device(dev), handle->buffer->sg_table->sgl,
					handle->buffer->sg_table->nents, DMA_BIDIRECTIONAL,
					DMA_ATTR_SKIP_CPU_SYNC);
	if (ret != 1) {
		dev_err(dev, "dma map failed, ret=%d.\n", ret);
		goto err_dma_map;
	}
	pa = sg_dma_address(handle->buffer->sg_table->sgl);

	dev_dbg(dev, "ion allocate payload iommu memory: va 0x%pK, pa %pad, len %d\n",
		va, &pa, buffer->len);

	buffer->va = va;
	buffer->pa = (phys_addr_t)pa;
	buffer->da = (u64)pa;

	ion_list->handle = handle;
	ion_list->buffer = buffer;
	list_add(&ion_list->list, &inst->list);

	return 0;

err_dma_map:
	ion_unmap_kernel(inst->ion, handle);
err_map_kernel:
	ion_free(inst->ion, handle);
err_ion_alloc:
	devm_kfree(dev, ion_list);
err_out:
	return ret;
}

static void sunxi_rpbuf_free_has_iommu(struct device *dev,
				struct sunxi_rpbuf_controller_priv *inst,
				struct rpbuf_buffer *buffer)
{
	struct ion_list *pos, *tmp;
	struct ion_handle *handle = NULL;

	list_for_each_entry_safe(pos, tmp, &inst->list, list) {
		if (pos->buffer == buffer) {
			handle = pos->handle;
			break;
		}
	}

	if (!handle) {
		dev_warn(dev, "invalid buffer:%s%d,va=%p,", buffer->name,
						buffer->id, buffer->va);
		return;
	}

	list_del(&pos->list);

	dma_unmap_sg_attrs(get_device(dev), handle->buffer->sg_table->sgl,
					handle->buffer->sg_table->nents, DMA_FROM_DEVICE,
					DMA_ATTR_SKIP_CPU_SYNC);
	ion_unmap_kernel(inst->ion, handle);
	ion_free(inst->ion, handle);
	devm_kfree(dev, pos);
}

int sunxi_rpbuf_dev_mmap_sg(struct sunxi_rpbuf_controller_priv *inst,
				struct rpbuf_buffer *buffer, struct vm_area_struct *vma)
{
	int ret, i;
	struct ion_list *pos, *tmp;
	struct ion_handle *handle = NULL;
	struct sg_table *table = NULL;
	unsigned long addr = vma->vm_start;
	unsigned long offset = vma->vm_pgoff * PAGE_SIZE;
	struct scatterlist *sg;

	list_for_each_entry_safe(pos, tmp, &inst->list, list) {
		if (pos->buffer == buffer) {
			handle = pos->handle;
			break;
		}
	}

	if (!handle) {
		pr_err("mmap invalid rpbuf_buffer.\n");
		BUG();
	}

	table = handle->buffer->sg_table;

	vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);

	for_each_sg(table->sgl, sg, table->nents, i) {
		struct page *page = sg_page(sg);
		unsigned long remainder = vma->vm_end - addr;
		unsigned long len = sg->length;

		if (offset >= sg->length) {
			offset -= sg->length;
			continue;
		} else if (offset) {
			page += offset / PAGE_SIZE;
			len = sg->length - offset;
			offset = 0;
		}

		pr_debug("remap 0x%x(pa) -> 0x%lx(va).\n",
							page_to_phys(page), addr);

		len = min(len, remainder);
		ret = remap_pfn_range(vma, addr, page_to_pfn(page), len, vma->vm_page_prot);
		if (ret) {
			pr_err("remap 0x%x(pa) -> 0x%lx(va) failed (ret: %d)\n",
							page_to_phys(page), addr, ret);
			goto err_out;
		}
		addr += len;

		if (addr >= vma->vm_end)
			return 0;
	}

	return 0;

err_out:
	return ret;
}

static int sunxi_rpbuf_alloc(struct device *dev,
				struct sunxi_rpbuf_controller_priv *inst,
				struct rpbuf_buffer *buffer)
{
	void *va;
	dma_addr_t pa;
	int ret;
	struct ion_handle *handle;
	struct ion_list *ion_list;

	ion_list = devm_kzalloc(dev, sizeof(*ion_list), GFP_KERNEL);
	if (IS_ERR_OR_NULL(ion_list)) {
		ret = -ENOMEM;
		goto err_out;
	}

	handle = ion_alloc(inst->ion, buffer->len, PAGE_SIZE,
						ION_HEAP_TYPE_DMA_MASK | ION_HEAP_CARVEOUT_MASK, 0);
	if (IS_ERR_OR_NULL(handle)) {
		dev_err(dev, "ion_alloc for len %d failed\n", buffer->len);
		ret = -ENOMEM;
		goto err_ion_alloc;
	}

	/* map to kernel space */
	va = ion_map_kernel(inst->ion, handle);
	if (IS_ERR_OR_NULL(va)) {
		dev_err(dev, "ion_map_kernel failed\n");
		ret = -ENOMEM;
		goto err_map_kernel;
	}

	/* get physical address */
	ret = ion_phys(inst->ion, handle, (ion_phys_addr_t *)&pa, &buffer->len);
	if (ret) {
		dev_err(dev, "ion phys failed.\n");
		ret = -ENOMEM;
		goto err_ion_phys;
	}

	dev_dbg(dev, "ion allocate payload memory: va 0x%pK, pa %pad, len %d\n",
		va, &pa, buffer->len);

	buffer->va = va;
	buffer->pa = (phys_addr_t)pa;
	buffer->da = (u64)pa;

	ion_list->handle = handle;
	ion_list->buffer = buffer;
	list_add(&ion_list->list, &inst->list);

	return 0;

err_ion_phys:
	ion_unmap_kernel(inst->ion, handle);
err_map_kernel:
	ion_free(inst->ion, handle);
err_ion_alloc:
	devm_kfree(dev, ion_list);
err_out:
	return ret;
}

static void sunxi_rpbuf_free(struct device *dev,
				struct sunxi_rpbuf_controller_priv *inst,
				struct rpbuf_buffer *buffer)
{
	struct ion_list *pos, *tmp;
	struct ion_handle *handle = NULL;

	list_for_each_entry_safe(pos, tmp, &inst->list, list) {
		if (pos->buffer == buffer) {
			handle = pos->handle;
			break;
		}
	}

	if (!handle) {
		dev_warn(dev, "invalid buffer:%s%d,va=%p,", buffer->name,
						buffer->id, buffer->va);
		return;
	}

	list_del(&pos->list);

	ion_unmap_kernel(inst->ion, handle);
	ion_free(inst->ion, handle);
	devm_kfree(dev, pos);
}

static int sunxi_rpbuf_controller_alloc_payload_memory(struct rpbuf_controller *controller,
						       struct rpbuf_buffer *buffer,
						       void *priv)
{
	struct device *dev = controller->dev;
	struct sunxi_rpbuf_controller_priv *controller_priv = priv;

	if (controller_priv->domain)
		return sunxi_rpbuf_alloc_has_iommu(dev, controller_priv, buffer);
	else
		return sunxi_rpbuf_alloc(dev, controller_priv, buffer);
}

static void sunxi_rpbuf_controller_free_payload_memory(struct rpbuf_controller *controller,
						       struct rpbuf_buffer *buffer,
						       void *priv)
{
	struct device *dev = controller->dev;
	struct sunxi_rpbuf_controller_priv *controller_priv = priv;

	if (controller_priv->domain)
		sunxi_rpbuf_free_has_iommu(dev, controller_priv, buffer);
	else
		sunxi_rpbuf_free(dev, controller_priv, buffer);
}

int sunxi_rpbuf_controller_dev_mmap(struct rpbuf_controller *controller, struct rpbuf_buffer *buffer, void *priv,
			    struct vm_area_struct *vma)
{
	struct sunxi_rpbuf_controller_priv *controller_priv = priv;

	if (controller_priv->domain)
		return sunxi_rpbuf_dev_mmap_sg(controller_priv, buffer, vma);
	else
		return sunxi_rpbuf_dev_mmap_sg(controller_priv, buffer, vma);
}


static struct rpbuf_controller_ops sunxi_rpbuf_controller_ops = {
	.alloc_payload_memory = sunxi_rpbuf_controller_alloc_payload_memory,
	.free_payload_memory = sunxi_rpbuf_controller_free_payload_memory,
	.buf_dev_mmap = sunxi_rpbuf_controller_dev_mmap,
};

static int sunxi_rpbuf_controller_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct device_node *np = dev->of_node;
	struct sunxi_rpbuf_controller_priv *controller_priv;
	struct rpbuf_controller *controller;
	struct device_node *rproc_np;
	struct device *rproc;
	struct device_node *tmp_np;
	u32 ctrl_id;
	int ret;

	controller_priv = devm_kzalloc(dev, sizeof(struct sunxi_rpbuf_controller_priv),
				       GFP_KERNEL);
	if (!controller_priv) {
		dev_err(dev, "kzalloc for sunxi_rpbuf_controller_priv failed\n");
		ret = -ENOMEM;
		goto err_out;
	}

	rproc_np = of_parse_phandle(np, "remoteproc", 0);
	if (!rproc_np) {
		dev_err(dev, "no \"remoteproc\" node specified\n");
		ret = -EINVAL;
		goto err_free_controller_priv;
	}
	/* We assume that the remoteproc is always a platform device */
	rproc = rpbuf_bus_find_device_by_of_node(&platform_bus_type, rproc_np);
	if (!rproc) {
		dev_err(dev, "failed to get remoteproc device\n");
		ret = -EINVAL;
		goto err_free_controller_priv;
	}

	if (of_property_read_bool(np, "iommus")) {
		dev_dbg(dev, "has iommu\n");

		controller_priv->domain = iommu_domain_alloc(rproc->bus);
		if (!controller_priv->domain) {
			dev_err(dev, "iommu_domain_alloc failed\n");
			ret = -ENODEV;
			goto err_free_controller_priv;
		}

		ret = iommu_attach_device(controller_priv->domain, dev);
		if (ret)
			dev_err(dev, "can't attach iommu device: %d\n", ret);
	}

	ret = of_property_read_u32(np, "ctrl_id", &ctrl_id);
	if (ret < 0) {
		dev_err(dev, "no \"ctrl_id\" node specified\n");
		goto err_free_iommu_domain;
	}

	tmp_np = of_parse_phandle(np, "memory-region", 0);
	if (tmp_np) {
		ret = of_reserved_mem_device_init(dev);
		if (ret < 0) {
			dev_err(dev, "failed to get reserved memory (ret: %d)\n", ret);
			goto err_free_iommu_domain;
		}
	}

	controller = rpbuf_create_controller(dev, &sunxi_rpbuf_controller_ops, controller_priv);
	if (!controller) {
		dev_err(dev, "rpbuf_create_controller failed\n");
		ret = -ENOMEM;
		goto err_free_iommu_domain;
	}

	/*
	 * Linux can only be MASTER until we ensure the translation from
	 * buffer->pa to buffer->va is correct.
	 */
	/* We use the remoteproc device as rpbuf link token. */
	ret = rpbuf_register_controller(controller, (void *)rproc, RPBUF_ROLE_MASTER);
	if (ret < 0) {
		dev_err(dev, "rpbuf_register_controller failed\n");
		goto err_destroy_controller;
	}

	ret = rpbuf_register_ctrl_dev(dev, ctrl_id, controller);
	if (ret < 0) {
		dev_err(dev, "rpbuf_register_ctrl_dev failed\n");
		goto err_unregister_controller;
	}

	controller_priv->ctrl_id = ctrl_id;
	INIT_LIST_HEAD(&controller_priv->list);

	controller_priv->ion = sunxi_ion_client_create("ion rpbuf");
	if (IS_ERR_OR_NULL(controller_priv->ion)) {
		dev_err(dev, "sunxi_ion_client_create ion rpbuf failed\n");
		goto err_unregister_ctrl_dev;
	}

	/*
	 * We must set rpbuf_controller as the device drvdata, to ensure that it
	 * can be found by rpbuf_get_controller_by_of_node().
	 */
	dev_set_drvdata(dev, controller);

	return 0;
err_unregister_ctrl_dev:
	rpbuf_unregister_ctrl_dev(dev, ctrl_id);
err_unregister_controller:
	rpbuf_unregister_controller(controller);
err_destroy_controller:
	rpbuf_destroy_controller(controller);
err_free_iommu_domain:
	if (controller_priv->domain)
		iommu_domain_free(controller_priv->domain);
err_free_controller_priv:
	devm_kfree(dev, controller_priv);
err_out:
	return ret;
}

static int sunxi_rpbuf_controller_remove(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct rpbuf_controller *controller = dev_get_drvdata(dev);
	struct sunxi_rpbuf_controller_priv *controller_priv = controller->priv;
	int ctrl_id = controller_priv->ctrl_id;
	int ret = 0;

	if (IS_ERR_OR_NULL(controller)) {
		dev_err(dev, "invalid rpbuf_controller ptr\n");
		ret = -EFAULT;
		goto err_out;
	}

	dev_set_drvdata(dev, NULL);

	ion_client_destroy(controller_priv->ion);
	rpbuf_unregister_ctrl_dev(dev, ctrl_id);
	rpbuf_unregister_controller(controller);
	rpbuf_destroy_controller(controller);

	if (controller_priv->domain)
		iommu_domain_free(controller_priv->domain);

	devm_kfree(dev, controller_priv);

	return 0;
err_out:
	return ret;
}

static const struct of_device_id sunxi_rpbuf_controller_ids[] = {
	{ .compatible = "allwinner,rpbuf-controller" },
	{}
};

static struct platform_driver sunxi_rpbuf_controller_driver = {
	.probe	= sunxi_rpbuf_controller_probe,
	.remove	= sunxi_rpbuf_controller_remove,
	.driver	= {
		.owner = THIS_MODULE,
		.name = "sunxi-rpbuf-controller",
		.of_match_table = sunxi_rpbuf_controller_ids,
	},
};

static int __init sunxi_rpbuf_init(void)
{
	int ret;

	ret = platform_driver_register(&sunxi_rpbuf_controller_driver);

	return ret;
}

static void __exit sunxi_rpbuf_exit(void)
{
	platform_driver_unregister(&sunxi_rpbuf_controller_driver);
}

subsys_initcall(sunxi_rpbuf_init);
module_exit(sunxi_rpbuf_exit);

MODULE_DESCRIPTION("Allwinner RPBuf controller driver");
MODULE_AUTHOR("Junyan Lin <junyanlin@allwinnertech.com>");
MODULE_LICENSE("GPL v2");
MODULE_VERSION("1.0.0");
