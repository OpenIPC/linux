/* SPDX-License-Identifier: GPL-2.0 */
/*
 * sunxi's Remote Processor Control Driver
 * base on st_remoteproc.c
 *
 * Copyright (C) 2015 Allwinnertech - All Rights Reserved
 *
 * Author: fuyao <fuyao@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <linux/clk.h>
#include <linux/clk/sunxi.h>
#include <linux/clk-provider.h>
#include <linux/dma-mapping.h>
#include <linux/err.h>
#include <linux/interrupt.h>
#include <linux/kernel.h>
#include <linux/mfd/syscon.h>
#include <linux/module.h>
#include <linux/iommu.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/of_address.h>
#include <linux/of_reserved_mem.h>
#include <linux/platform_device.h>
#include <linux/regmap.h>
#include <linux/remoteproc.h>
#include <linux/reset.h>
#include <linux/mailbox_client.h>

#include "sunxi_rproc_internal.h"

#define MBOX_NAME			"mbox-chan"

static LIST_HEAD(sunxi_rproc_list);

struct sunxi_resource_map_table {
	u64 pa; /* Address of cpu's address */
	u64 da; /* Address of rproc's address */
	u32 len;
	void __iomem *va;
};

struct sunxi_mbox {
	char name[32];
	struct mbox_chan *chan;
	struct mbox_client client;
	struct work_struct vq_work;
	int notifyid;
};

struct sunxi_rproc {
	struct sunxi_resource_map_table *mem_maps;
	int mem_maps_cnt;
	struct sunxi_mbox mb;
	const char *core_name;

	struct workqueue_struct *workqueue;
	int irq;
	struct sunxi_core *core;
	struct list_head list;
	struct rproc *rproc;
	struct iommu_domain *domain;
};

/*  mbox work funciton */
static void sunxi_rproc_mb_vq_work(struct work_struct *work)
{
	struct sunxi_mbox *mb = container_of(work, struct sunxi_mbox, vq_work);
	struct rproc *rproc = dev_get_drvdata(mb->client.dev);

	/* tell remoteproc that a virtqueue is interrupted */
	if (rproc_vq_interrupt(rproc, mb->notifyid) == IRQ_NONE)
		dev_dbg(&rproc->dev, "no message found in vq%d\n", mb->notifyid);
}

static void sunxi_rproc_mb_rx_callback(struct mbox_client *cl, void *data)
{
	struct rproc *rproc = dev_get_drvdata(cl->dev);
	struct sunxi_mbox *mb = container_of(cl, struct sunxi_mbox, client);
	struct sunxi_rproc *ddata = rproc->priv;

	dev_dbg(&rproc->dev, "mbox recv data:0x%x\n", *(uint32_t *)data);
	mb->notifyid = *(uint32_t *)data;

	queue_work(ddata->workqueue, &mb->vq_work);
}

static void sunxi_rproc_mb_tx_done(struct mbox_client *cl, void *msg, int r)
{
	struct rproc *rproc = dev_get_drvdata(cl->dev);
	devm_kfree(&rproc->dev, msg);
}

static int sunxi_rproc_request_mbox(struct rproc *rproc)
{
	struct sunxi_rproc *ddata = rproc->priv;
	struct device *dev = &rproc->dev;
	const char *name;
	struct mbox_client *cl;

	/* Static init mbox info */
	strcpy(ddata->mb.name, MBOX_NAME);
	ddata->mb.client.rx_callback = sunxi_rproc_mb_rx_callback;
	ddata->mb.client.tx_block = false;
	ddata->mb.client.tx_done = sunxi_rproc_mb_tx_done;

	name = ddata->mb.name;
	cl = &ddata->mb.client;

	/* set device parent */
	cl->dev = dev->parent;
	ddata->mb.chan = mbox_request_channel_byname(cl, name);
	if (IS_ERR(ddata->mb.chan)) {
		dev_warn(dev, "cannot get %s channel (ret=%ld)\n", name,
						PTR_ERR(ddata->mb.chan));
			goto err_probe;
	}

	INIT_WORK(&ddata->mb.vq_work, sunxi_rproc_mb_vq_work);

	return 0;

err_probe:
	return -EPROBE_DEFER;
}

static void sunxi_rproc_free_mbox(struct rproc *rproc)
{
	struct sunxi_rproc *ddata = rproc->priv;

	if (ddata->mb.chan)
		mbox_free_channel(ddata->mb.chan);
	ddata->mb.chan = NULL;
}

int sunxi_rproc_start(struct rproc *rproc)
{
	struct sunxi_rproc *ddata = rproc->priv;

	sunxi_core_set_start_addr(ddata->core, rproc->bootaddr);
	sunxi_core_set_freq(ddata->core, 600*1000*1000);

	return sunxi_core_start(ddata->core);
}

int sunxi_rproc_stop(struct rproc *rproc)
{
	struct sunxi_rproc *ddata = rproc->priv;

	return sunxi_core_stop(ddata->core);
}

static void *sunxi_da_to_va(struct rproc *rproc, u64 da, int len)
{
	struct device *dev = &rproc->dev;
	struct sunxi_rproc *ddata = rproc->priv;
	const struct sunxi_resource_map_table *t;
	int i;
	void *va;

	for (i = 0; i < ddata->mem_maps_cnt; i++) {
		t = &ddata->mem_maps[i];

		if (da >= t->da && (da + len) <= (t->da + t->len)) {
			va = t->va + (da - t->da);
			return va;
		}
	}

	dev_err(dev, "failed da_to_va\n");

	return NULL;
}

static void sunxi_rproc_kick(struct rproc *rproc, int notifyid)
{
	struct sunxi_rproc *ddata = rproc->priv;
//	int i;
	int err;
	struct sunxi_mbox *mb;
	u32 *msg = NULL;

	dev_dbg(&rproc->dev, "notifyid=%d kick\n", notifyid);

	mb = &ddata->mb;

	msg = devm_kzalloc(&rproc->dev, sizeof(u32), GFP_KERNEL);
	if (!msg) {
		dev_err(&rproc->dev, "(%s:%d) alloc mem failed\n",
			__func__, __LINE__);
		return;
	}
	*msg = notifyid;
	err = mbox_send_message(mb->chan, (void *)msg);
	if (err < 0) {
		dev_err(&rproc->dev, "(%s:%d) kick failed (%s, err:%d)\n",
			__func__, __LINE__, mb->name, err);
		return;
	}
}

static struct rproc_ops sunxi_rproc_ops = {
	.start		= sunxi_rproc_start,
	.stop		= sunxi_rproc_stop,
	.da_to_va	= sunxi_da_to_va,
	.kick		= sunxi_rproc_kick,
};

/*
 * Ferpmsg_send_offchannel_rawtch state of the processor: 0 is off, 1 is on.
 */
static int sunxi_rproc_state(struct rproc *rproc)
{
	struct device *dev = rproc->dev.parent;
	struct sunxi_rproc *ddata = rproc->priv;

	if (sunxi_core_is_start(ddata->core)) {
		dev_dbg(dev, "Remote process already running\r\n");
		return 1;
	}

	dev_dbg(dev, "Remote process need to booot\r\n");
	return 0;
}

static const struct of_device_id sunxi_rproc_match[] = {
	{ .compatible = "allwinner,sun8iw21p1-e907-rproc" },
	{},
};
MODULE_DEVICE_TABLE(of, sunxi_rproc_match);

static int sunxi_rproc_parse_dt(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct rproc *rproc = platform_get_drvdata(pdev);
	struct sunxi_rproc *ddata = rproc->priv;
	struct device_node *np = dev->of_node;
	struct device_node *fw_np = NULL;
	u32 *map_array;
	const char *fw_name;
	int ret = 0;
	int i;

	ret = of_property_read_string(np, "core-name", &ddata->core_name);
	if (ret < 0) {
		dev_err(dev, "fial to get core-name\n");
		return -EINVAL;
	}

	ddata->core = sunxi_remote_core_find(ddata->core_name);
	if (!ddata->core) {
		dev_err(dev, "Failed to find remote core\n");
		return -ENXIO;
	}

	ret = of_property_read_string(np, "firmware-name", &fw_name);
	if (ret < 0) {
		dev_err(dev, "failed to get firmware-name\n");
		return -EINVAL;
	}

	fw_np = of_parse_phandle(np, "fw-region", 0);
	if (fw_np) {
		struct resource r;
		int len;

		ret = of_address_to_resource(fw_np, 0, &r);
		if (!ret) {
			len = resource_size(&r);

			dev_dbg(dev, "register memory fw:%s, size:0x%x.\n", fw_name, len);
			ret = sunxi_register_memory_fw(fw_name, r.start, len);
			if (ret < 0)
				dev_dbg(dev, "register memory fw:%s failed.\n", fw_name);
		} else
			dev_err(dev, "Failed to get fw-region(ret=%d)\n", ret);
	}

	ret = of_property_count_elems_of_size(np, "memory-mappings", sizeof(u32) * 3);
	if (ret < 0) {
		dev_err(dev, "fail to get memory-mappings\n");
		return -ENXIO;
	}

	if (ret < 1) {
		dev_err(dev, "Please define memory-mappings property in dts\n");
		return -EINVAL;
	}

	ddata->mem_maps_cnt = ret;

	ddata->mem_maps = devm_kcalloc(dev, ret,
				sizeof(struct sunxi_resource_map_table), GFP_KERNEL);
	if (!ddata->mem_maps) {
		dev_err(dev, "fail to alloc memory-mappings\n");
		return -ENOMEM;
	}

	map_array = devm_kcalloc(dev, ret * 3, sizeof(u32), GFP_KERNEL);
	if (!map_array) {
		dev_err(dev, "fail to alloc map_array\n");
		return -ENOMEM;
	}

	ret = of_property_read_u32_array(np, "memory-mappings", map_array,
			ddata->mem_maps_cnt * 3);
	if (ret < 0) {
		dev_err(dev, "fail to read memory-mappings\n");
		return -ENXIO;
	}

	for (i = 0; i < ddata->mem_maps_cnt; i++) {
		ddata->mem_maps[i].da = map_array[i * 3 + 0];
		ddata->mem_maps[i].len = map_array[i * 3 + 1];
		ddata->mem_maps[i].pa = map_array[i * 3 + 2];
		ddata->mem_maps[i].va = devm_ioremap_wc(dev, ddata->mem_maps[i].pa,
				ddata->mem_maps[i].len);

		if (!PTR_ERR(ddata->mem_maps[i].va)) {
			dev_err(dev, "ioremap failed %llx - %llx\n", ddata->mem_maps[i].pa,
				ddata->mem_maps[i].pa + ddata->mem_maps[i].len - 1);
			ret = -ENOMEM;
			goto free_mem_maps;
		}

		dev_dbg(dev, "memory-mappings[%d]: da: 0x%llx, len: 0x%x, pa: 0x%llx va: 0x%p\n",
				i, ddata->mem_maps[i].da, ddata->mem_maps[i].len,
				ddata->mem_maps[i].pa, ddata->mem_maps[i].va);

		if (ddata->domain) {
			ret = iommu_map(ddata->domain, ddata->mem_maps[i].da, ddata->mem_maps[i].pa,
							ddata->mem_maps[i].len, IOMMU_READ | IOMMU_WRITE);
			if (ret) {
				dev_err(dev, "failed to iommu_map : %llx -> %llx\n", ddata->mem_maps[i].pa,
								ddata->mem_maps[i].da);
				goto free_iomap;
			}
		}
	}

	devm_kfree(dev, map_array);

	return 0;

free_iomap:
	for (; i >= 0; i--)
		iommu_unmap(ddata->domain, ddata->mem_maps[i].da, ddata->mem_maps[i].len);
free_mem_maps:
	devm_kfree(dev, ddata->mem_maps);

	return ret;
}

static int sunxi_rproc_register_mem(struct rproc *rproc)
{
	int i = 0;
	int ret;
	struct device *dev = rproc->dev.parent;
	struct device_node *np;
	struct resource r;
	void *va;
	u32 da;
	int len;
	/* pre-register mem entry */
	while ((np = of_parse_phandle(dev->of_node, "memory-region", i))) {
		ret = of_address_to_resource(np, 0, &r);
		if (ret) {
			dev_err(dev, "Failed to get memory-region(ret=%d)\n", ret);
			return ret;
		}

		if (strcmp(np->name, "vdev0buffer") == 0) {
			/* Initial reserved memory resources */
			ret = of_reserved_mem_device_init_by_idx(dev, dev->of_node, i);
			dev_dbg(dev, "Register a shared buffer(start:0x%08x len:0x%x)\n",
							r.start, r.end - r.start + 1);
			if (ret) {
				dev_err(dev, "Failed to get shared-memory(ret=%d)\n", ret);
				return ret;
			}
			dma_set_coherent_mask(dev, 0xffffffff);
			va = NULL;
		} else {
			len = resource_size(&r);
			va = devm_ioremap_wc(dev, r.start, len);
			if (!PTR_ERR(va)) {
				dev_err(dev, "Fialed to remap memory-region\n");
				return -ENOMEM;
			}
		}

		da = r.start;

		rproc_add_mem_entry(rproc, np->name, r.start, da, va, len);

		dev_dbg(dev, "memory-region%d:%s 0x%08x [0x%08x - 0x%08x]\n", i,
						np->name, (uint32_t)va, r.start, r.end);

		i++;
	}

	return 0;
}

static void sunxi_rproc_unregister_mem(struct rproc *rproc)
{
	struct device *dev = rproc->dev.parent;

	rproc_clean_mem_entry(rproc);

	of_reserved_mem_device_release(dev);
}

int sunxi_rproc_report_crash(char *name, enum rproc_crash_type type)
{
	struct sunxi_rproc *ddata, *tmp;

	list_for_each_entry_safe(ddata, tmp, &sunxi_rproc_list, list) {
		if (!strcmp(ddata->rproc->name, name)) {
			rproc_report_crash(ddata->rproc, type);
			return 0;
		}
	}

	return -1;
}
EXPORT_SYMBOL(sunxi_rproc_report_crash);

static int sunxi_rproc_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct sunxi_rproc *ddata;
	struct device_node *np = dev->of_node;
	struct rproc *rproc;
	const char *fw_name;
	int enabled;
	int ret;
	bool has_iommu;

	/* we need to read firmware name at first. */
	ret = of_property_read_string(np, "firmware-name", &fw_name);
	if (ret < 0) {
		dev_err(dev, "failed to get firmware-name\n");
		return -EINVAL;
	}

	rproc = rproc_alloc(dev, np->name, &sunxi_rproc_ops, fw_name, sizeof(*ddata));
	if (!rproc)
		return -ENOMEM;

	rproc->has_iommu = false;
	rproc->auto_boot = true;
	ddata = rproc->priv;
	ddata->rproc = rproc;

	platform_set_drvdata(pdev, rproc);

	has_iommu = of_property_read_bool(np, "iommus");
	if (has_iommu) {
		dev_dbg(dev, "E907 use iommu.\r\n");
		rproc->has_iommu = true;
		ddata->domain = iommu_domain_alloc(dev->bus);
		if (!ddata->domain) {
			dev_err(dev, "can't alloc iommu domain\n");
			ret = -ENOMEM;
			goto free_rproc;
		}

		ret = iommu_attach_device(ddata->domain, dev);
		if (ret)
			dev_err(dev, "can't attach iommu device: %d\n", ret);

	} else {
		ddata->domain = NULL;
		dev_dbg(dev, "E907 not use iommu.\r\n");
	}

	ddata->workqueue = create_workqueue(dev_name(dev));
	if (!ddata->workqueue) {
		dev_err(dev, "Cannot create workqueue\n");
		ret = -ENOMEM;
		goto free_domain;
	}

	ret = sunxi_rproc_parse_dt(pdev);
	if (ret)
		goto destroy_workqueue;

	sunxi_core_init(ddata->core);

	ret = sunxi_rproc_request_mbox(rproc);
	if (ret < 0) {
		dev_err(dev, "sunxi_rproc_request_mbox failed\n");
		goto destroy_workqueue;
	}

	ret = sunxi_rproc_register_mem(rproc);
	if (ret < 0) {
		dev_err(dev, "Fialed to parser memory-region\n");
		goto destroy_workqueue;
	}

	/* check remote process whether already running */
	enabled = sunxi_rproc_state(rproc);
	if (enabled < 0) {
		ret = enabled;
		goto destroy_workqueue;
	}

	if (enabled) {
		atomic_inc(&rproc->power);
		rproc->state = RPROC_RUNNING;
	}

	ret = rproc_add(rproc);
	if (ret < 0)
		goto free_mbox;

	list_add(&ddata->list, &sunxi_rproc_list);

	return 0;

free_mbox:
	sunxi_rproc_free_mbox(rproc);
destroy_workqueue:
	destroy_workqueue(ddata->workqueue);
free_domain:
	if (ddata->domain)
		ddata->domain = NULL;
free_rproc:
	rproc_free(rproc);
	return ret;
}

static int sunxi_rproc_remove(struct platform_device *pdev)
{
	struct rproc *rproc = platform_get_drvdata(pdev);
	struct sunxi_rproc *ddata = rproc->priv;

	if (atomic_read(&rproc->power) > 0)
		rproc_shutdown(rproc);

	sunxi_rproc_unregister_mem(rproc);

	rproc_del(rproc);

	sunxi_rproc_free_mbox(rproc);

	destroy_workqueue(ddata->workqueue);

	sunxi_core_deinit(ddata->core);

	/*
	 * all device shared a domain in sunxi-iommu driver,
	 * so we don't need to release that domain.
	 * */
	if (ddata->domain)
		ddata->domain = NULL;

	rproc_free(rproc);

	list_del(&ddata->list);

	return 0;
}

static struct platform_driver sunxi_rproc_driver = {
	.probe = sunxi_rproc_probe,
	.remove = sunxi_rproc_remove,
	.driver = {
		.name = "sunxi-rproc",
		.of_match_table = sunxi_rproc_match,
	},
};

static int __init sunxi_rproc_init(void)
{
	int ret;

	ret = platform_driver_register(&sunxi_rproc_driver);

	return ret;
}

static void __exit sunxi_rproc_exit(void)
{
	platform_driver_unregister(&sunxi_rproc_driver);
}

#ifdef CONFIG_SUNXI_RPROC_FASTBOOT
postcore_initcall(sunxi_rproc_init);
#else
module_init(sunxi_rproc_init);
#endif
module_exit(sunxi_rproc_exit);

MODULE_DESCRIPTION("SUNXI Remote Processor Control Driver");
MODULE_AUTHOR("lijiajian <lijiajian@allwinnertech.com>");
MODULE_LICENSE("GPL v2");
