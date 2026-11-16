// SPDX-License-Identifier: GPL-2.0
/*
 * Ingenic T23 Video Processor Subsystem - Platform Driver
 *
 * Copyright (C) 2017-2023 Ingenic Semiconductor Co., Ltd.
 * Copyright (C) 2026 Difan Zhang
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/of.h>
#include <linux/of_reserved_mem.h>
#include <linux/dma-mapping.h>
#include <media/videobuf2-dma-contig.h>

#include "tx-isp-device.h"

struct tx_isp_device *g_tx_isp_dev;

int tx_isp_subdev_init(struct platform_device *pdev, struct tx_isp_subdev *sd,
		       struct tx_isp_subdev_ops *ops)
{
	struct resource *res;

	if (!sd || !pdev || !g_tx_isp_dev)
		return -EINVAL;
	if (pdev->id < 0 || pdev->id >= TX_ISP_SUBDEV_MAX)
		return -EINVAL;
	if (g_tx_isp_dev->subdevs[pdev->id])
		return -EBUSY;

	sd->ops = ops;
	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (res) {
		sd->res = request_mem_region(res->start, resource_size(res),
					     dev_name(&pdev->dev));
		if (!sd->res)
			return -EBUSY;
		sd->base = ioremap(res->start, resource_size(res));
		if (!sd->base) {
			release_mem_region(res->start, resource_size(res));
			sd->res = NULL;
			return -ENOMEM;
		}
	}

	g_tx_isp_dev->subdevs[pdev->id] = sd;
	g_tx_isp_dev->subdev_cnt =
		max_t(unsigned int, g_tx_isp_dev->subdev_cnt, pdev->id + 1);

	return 0;
}

void tx_isp_subdev_deinit(struct tx_isp_subdev *sd)
{
	int i;

	if (!sd || !g_tx_isp_dev)
		return;
	if (sd->base) {
		iounmap(sd->base);
		sd->base = NULL;
	}
	if (sd->res) {
		release_mem_region(sd->res->start, resource_size(sd->res));
		sd->res = NULL;
	}

	for (i = 0; i < g_tx_isp_dev->subdev_cnt; i++) {
		if (g_tx_isp_dev->subdevs[i] == sd) {
			g_tx_isp_dev->subdevs[i] = NULL;
			break;
		}
	}
	while (g_tx_isp_dev->subdev_cnt &&
	       !g_tx_isp_dev->subdevs[g_tx_isp_dev->subdev_cnt - 1])
		g_tx_isp_dev->subdev_cnt--;
}

static struct {
	const char *name;
	const char *resource_name;
	bool has_irq;
	struct resource resources[3];
	unsigned int count;
	int (*probe)(struct platform_device *pdev);
	int (*remove)(struct platform_device *pdev);
	struct platform_device *pdev;
	bool probed;
} children[] = {
	[TX_ISP_SUBDEV_VIC] = { .name = "tx-isp-vic",
				.resource_name = "vic",
				.has_irq = true,
				.probe = tx_isp_vic_probe,
				.remove = tx_isp_vic_remove },
	[TX_ISP_SUBDEV_CSI] = { .name = "tx-isp-csi",
				.resource_name = "csi",
				.probe = tx_isp_csi_probe,
				.remove = tx_isp_csi_remove },
	[TX_ISP_SUBDEV_CORE] = { .name = "tx-isp-core",
				 .resource_name = "core",
				 .has_irq = true,
				 .probe = tx_isp_core_probe,
				 .remove = tx_isp_core_remove },
};

static int tx_isp_get_resources(struct platform_device *pdev)
{
	unsigned int i;

	for (i = 0; i < ARRAY_SIZE(children); i++) {
		struct resource *res;
		int irq;

		children[i].count = 0;
		if (!children[i].resource_name)
			continue;
		res = platform_get_resource_byname(pdev, IORESOURCE_MEM,
						   children[i].resource_name);
		if (!res)
			return -EINVAL;
		children[i].resources[children[i].count++] = *res;
		if (i == TX_ISP_SUBDEV_CSI) {
			res = platform_get_resource_byname(pdev, IORESOURCE_MEM,
							   "phy");
			if (!res)
				return -EINVAL;
			children[i].resources[children[i].count++] = *res;
		}
		if (!children[i].has_irq)
			continue;
		irq = platform_get_irq_byname(pdev, children[i].resource_name);
		if (irq < 0)
			return irq;
		children[i].resources[children[i].count++] = (struct resource){
			.start = irq,
			.end = irq,
			.flags = IORESOURCE_IRQ,
		};
	}
	return 0;
}

static void tx_isp_remove_children(void)
{
	int i;
	/* IRQ callbacks must finish before any subdevice is freed. */
	tx_isp_free_irq(g_tx_isp_dev);
	for (i = ARRAY_SIZE(children) - 1; i >= 0; i--) {
		if (children[i].probed) {
			children[i].remove(children[i].pdev);
			children[i].probed = false;
		}
		if (children[i].pdev) {
			platform_device_unregister(children[i].pdev);
			children[i].pdev = NULL;
		}
	}
}

struct tx_isp_clk_desc {
	const char *name;
	size_t offset;
	unsigned long rate;
};

static const struct tx_isp_clk_desc isp_clk_descs[] = {
	{ "cgu_isp", offsetof(struct tx_isp_device, cgu_isp), 0 },
	{ "isp",     offsetof(struct tx_isp_device, isp_gate), 0 },
	{ "csi",     offsetof(struct tx_isp_device, csi_gate), 0 },
	{ "cgu_vpu", offsetof(struct tx_isp_device, cgu_vpu), 416000000 },
	{ "ivdc",    offsetof(struct tx_isp_device, ivdc_gate), 0 },
};

static void tx_isp_clocks_disable(struct tx_isp_device *ispdev)
{
	int i;

	for (i = ARRAY_SIZE(isp_clk_descs) - 1; i >= 0; i--) {
		struct clk **clk_p = (void *)ispdev + isp_clk_descs[i].offset;

		if (*clk_p) {
			clk_disable_unprepare(*clk_p);
			clk_put(*clk_p);
			*clk_p = NULL;
		}
	}
}

static int tx_isp_clocks_enable(struct tx_isp_device *ispdev)
{
	struct device *dev = &ispdev->pdev->dev;
	int i, ret;

	for (i = 0; i < ARRAY_SIZE(isp_clk_descs); i++) {
		const struct tx_isp_clk_desc *desc = &isp_clk_descs[i];
		struct clk **clk_p = (void *)ispdev + desc->offset;
		struct clk *clk;

		clk = clk_get(dev, desc->name);
		if (IS_ERR(clk)) {
			ret = PTR_ERR(clk);
			goto err;
		}
		if (desc->rate) {
			ret = clk_set_rate(clk, desc->rate);
			if (ret) {
				clk_put(clk);
				goto err;
			}
		}
		ret = clk_prepare_enable(clk);
		if (ret) {
			clk_put(clk);
			goto err;
		}
		*clk_p = clk;
	}
	return 0;

err:
	tx_isp_clocks_disable(ispdev);
	return ret;
}

static int tx_isp_probe(struct platform_device *pdev)
{
	struct tx_isp_device *ispdev;
	struct resource *res;
	int ret, i;

	if (g_tx_isp_dev)
		return -EBUSY;
	ret = dma_set_mask_and_coherent(&pdev->dev, DMA_BIT_MASK(32));
	if (ret)
		return ret;
	vb2_dma_contig_set_max_seg_size(&pdev->dev, DMA_BIT_MASK(32));

	ret = tx_isp_get_resources(pdev);
	if (ret)
		return ret;
	ispdev = kzalloc(sizeof(*ispdev), GFP_KERNEL);
	if (!ispdev)
		return -ENOMEM;

	ispdev->pdev = pdev;
	res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "ivdc");
	ispdev->ivdc_base = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(ispdev->ivdc_base)) {
		ret = PTR_ERR(ispdev->ivdc_base);
		kfree(ispdev);
		return ret;
	}
	ispdev->reset = devm_reset_control_get_exclusive(&pdev->dev, "isp");
	if (IS_ERR(ispdev->reset)) {
		ret = PTR_ERR(ispdev->reset);
		kfree(ispdev);
		return ret;
	}
	platform_set_drvdata(pdev, ispdev);
	g_tx_isp_dev = ispdev;
	ret = of_reserved_mem_device_init(&pdev->dev);
	if (ret && ret != -ENODEV)
		goto fail_clocks;

	ispdev->scratch_size = PAGE_SIZE;
	ispdev->scratch_buf = dma_alloc_coherent(&pdev->dev,
						 ispdev->scratch_size,
						 &ispdev->scratch_dma,
						 GFP_KERNEL);
	if (!ispdev->scratch_buf) {
		ret = -ENOMEM;
		goto fail_clocks;
	}

	ret = tx_isp_clocks_enable(ispdev);
	if (ret) {
		dev_err(&pdev->dev, "failed to enable ISP clocks: %d\n", ret);
		goto fail_clocks;
	}

	for (i = 0; i < ARRAY_SIZE(children); i++) {
		struct platform_device *child;

		if (!children[i].name)
			continue;
		child = platform_device_register_resndata(
			&pdev->dev, children[i].name, i, children[i].resources,
			children[i].count, NULL, 0);
		if (IS_ERR(child)) {
			ret = PTR_ERR(child);
			goto fail;
		}
		children[i].pdev = child;
		ret = children[i].probe(child);
		if (ret)
			goto fail;
		children[i].probed = true;
	}
	for (i = 0; i < ARRAY_SIZE(children); i++) {
		struct tx_isp_subdev *sd = ispdev->subdevs[i];

		if (!children[i].pdev)
			continue;
		if (platform_get_resource(children[i].pdev, IORESOURCE_IRQ,
					  0) &&
		    (!sd || !sd->ops ||
		     !sd->ops->interrupt_service_routine)) {
			ret = -ENXIO;
			dev_err(&pdev->dev, "%s has no interrupt handler\n",
				children[i].name);
			goto fail;
		}
	}
	for (i = 0; i < ARRAY_SIZE(children); i++) {
		if (!children[i].pdev)
			continue;
		if (!platform_get_resource(children[i].pdev, IORESOURCE_IRQ, 0))
			continue;
		ret = tx_isp_request_irq(children[i].pdev, ispdev);
		if (ret)
			goto fail;
	}

	ret = t23_isp_video_register(ispdev);
	if (ret) {
		dev_err(&pdev->dev, "failed to register V4L2 device: %d\n",
			ret);
		goto fail;
	}

	return 0;
fail:
	t23_isp_video_unregister();
	tx_isp_remove_children();
	tx_isp_clocks_disable(ispdev);
fail_clocks:
	if (ispdev->scratch_buf && !ispdev->dma_quiescence_failed) {
		dma_free_coherent(&pdev->dev, ispdev->scratch_size,
				  ispdev->scratch_buf, ispdev->scratch_dma);
	}
	if (ispdev->dma_buffer && !ispdev->dma_quiescence_failed) {
		dma_free_coherent(&pdev->dev, ispdev->dma_size,
				  ispdev->dma_buffer, ispdev->dma_address);
	}
	if (!ispdev->dma_quiescence_failed)
		of_reserved_mem_device_release(&pdev->dev);
	platform_set_drvdata(pdev, NULL);
	g_tx_isp_dev = NULL;
	kfree(ispdev);
	return ret;
}

static void tx_isp_remove(struct platform_device *pdev)
{
	struct tx_isp_device *ispdev = platform_get_drvdata(pdev);

	if (ispdev) {
		t23_isp_video_unregister();
		tx_isp_remove_children();
		tx_isp_clocks_disable(ispdev);
		if (ispdev->scratch_buf && !ispdev->dma_quiescence_failed) {
			dma_free_coherent(&pdev->dev, ispdev->scratch_size,
					  ispdev->scratch_buf,
					  ispdev->scratch_dma);
		}
		if (ispdev->dma_buffer && !ispdev->dma_quiescence_failed) {
			dma_free_coherent(&pdev->dev, ispdev->dma_size,
					  ispdev->dma_buffer, ispdev->dma_address);
		}
		if (!ispdev->dma_quiescence_failed)
			of_reserved_mem_device_release(&pdev->dev);
		platform_set_drvdata(pdev, NULL);
		kfree(ispdev);
		g_tx_isp_dev = NULL;
	}
}

static const struct of_device_id tx_isp_of_match[] = {
	{ .compatible = "ingenic,t23-isp" },
	{}
};
MODULE_DEVICE_TABLE(of, tx_isp_of_match);

static struct platform_driver tx_isp_driver = {
	.probe  = tx_isp_probe,
	.remove = tx_isp_remove,
	.driver = {
		.name  = "tx-isp",
		.of_match_table = tx_isp_of_match,
	},
};

module_platform_driver(tx_isp_driver);

MODULE_AUTHOR("Ingenic Semiconductor");
MODULE_AUTHOR("Difan Zhang");
MODULE_DESCRIPTION("Ingenic T23 Video Processor Subsystem Driver");
MODULE_LICENSE("GPL");
MODULE_IMPORT_NS(DMA_BUF);
