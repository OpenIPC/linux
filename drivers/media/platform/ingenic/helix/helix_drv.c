// SPDX-License-Identifier: GPL-2.0
// Copyright (C) 2026 Difan Zhang
#include "helix_drv.h"

int helix_hw_reset(struct helix_dev *hdev)
{
	return reset_control_reset(hdev->reset);
}

static void helix_free_dma(struct helix_dev *hdev);

int helix_hw_quiesce(struct helix_dev *hdev)
{
	int ret;

	lockdep_assert_held(&hdev->hw_lock);
	ret = helix_hw_reset(hdev);
	if (ret) {
		dev_err(hdev->dev, "helix reset failed: %d\n", ret);
		WRITE_ONCE(hdev->fatal, true);
		return ret;
	}

	WRITE_ONCE(hdev->fatal, false);
	WRITE_ONCE(hdev->hw_running, false);
	hdev->dma_owner = NULL;
	if (hdev->h264_hold_valid) {
		helix_free_h264_buffers(hdev, &hdev->h264_hold);
		memset(&hdev->h264_hold, 0, sizeof(hdev->h264_hold));
		hdev->h264_hold_valid = false;
	}
	if (!hdev->dma_users)
		helix_free_dma(hdev);
	return 0;
}

static void helix_free_dma(struct helix_dev *hdev)
{
	if (hdev->task_vaddr)
		dma_free_coherent(hdev->dev, HELIX_TASK_SIZE, hdev->task_vaddr,
				  hdev->task_paddr);
	if (hdev->input_vaddr)
		dma_free_coherent(hdev->dev, hdev->input_size,
				  hdev->input_vaddr, hdev->input_paddr);
	if (hdev->scratch_vaddr)
		dma_free_coherent(hdev->dev, hdev->scratch_size,
				  hdev->scratch_vaddr, hdev->scratch_paddr);
	if (hdev->bounce_vaddr)
		dma_free_coherent(hdev->dev, hdev->bounce_size,
				  hdev->bounce_vaddr, hdev->bounce_paddr);
	hdev->task_vaddr = NULL;
	hdev->input_vaddr = NULL;
	hdev->scratch_vaddr = NULL;
	hdev->bounce_vaddr = NULL;
	hdev->input_size = 0;
	hdev->scratch_size = 0;
}

/*
 * hw_lock held. Allocate CPU fallback input buffer lazily only when an input
 * frame requires format conversion or width/address alignment padding.
 */
int helix_ensure_input_dma(struct helix_dev *hdev, size_t input_size)
{
	struct device *dev = hdev->dev;
	dma_addr_t paddr;
	void *vaddr;

	lockdep_assert_held(&hdev->hw_lock);
	input_size = PAGE_ALIGN(input_size);
	if (input_size <= hdev->input_size && hdev->input_vaddr)
		return 0;
	if (READ_ONCE(hdev->hw_running))
		return -EBUSY;
	vaddr = dma_alloc_coherent(dev, input_size, &paddr, GFP_KERNEL);
	if (!vaddr)
		return -ENOMEM;
	if (hdev->input_vaddr)
		dma_free_coherent(dev, hdev->input_size,
				  hdev->input_vaddr, hdev->input_paddr);
	hdev->input_vaddr = vaddr;
	hdev->input_paddr = paddr;
	hdev->input_size = input_size;
	return 0;
}

/*
 * hw_lock held. Shared task, bitstream bounce, and H.264 intra-frame scratch
 * buffers allocated from CMA pool while any context is actively streaming.
 */
int helix_get_dma(struct helix_dev *hdev, bool need_scratch)
{
	struct device *dev = hdev->dev;

	lockdep_assert_held(&hdev->hw_lock);
	if (need_scratch && !hdev->scratch_vaddr) {
		hdev->scratch_vaddr = dma_alloc_coherent(dev,
							 HELIX_SCRATCH_BUF_SIZE,
							 &hdev->scratch_paddr,
							 GFP_KERNEL);
		if (!hdev->scratch_vaddr)
			goto err;
		hdev->scratch_size = HELIX_SCRATCH_BUF_SIZE;
	}
	if (!hdev->bounce_vaddr) {
		hdev->bounce_vaddr = dma_alloc_coherent(dev, HELIX_BOUNCE_BUF_SIZE,
							&hdev->bounce_paddr,
							GFP_KERNEL);
		if (!hdev->bounce_vaddr)
			goto err;
		hdev->bounce_size = HELIX_BOUNCE_BUF_SIZE;
	}
	if (!hdev->task_vaddr) {
		hdev->task_vaddr = dma_alloc_coherent(dev, HELIX_TASK_SIZE,
						      &hdev->task_paddr,
						      GFP_KERNEL);
		if (!hdev->task_vaddr)
			goto err;
	}
	hdev->dma_users++;
	return 0;

err:
	if (!hdev->dma_users && !READ_ONCE(hdev->hw_running))
		helix_free_dma(hdev);
	return -ENOMEM;
}

/* hw_lock held. A VPU that did not stop may still write these buffers. */
void helix_put_dma(struct helix_dev *hdev)
{
	lockdep_assert_held(&hdev->hw_lock);
	if (!--hdev->dma_users && !READ_ONCE(hdev->hw_running) &&
	    !READ_ONCE(hdev->fatal))
		helix_free_dma(hdev);
}

static irqreturn_t helix_irq_handler(int irq, void *data)
{
	struct helix_dev *hdev = data;
	u32 status = readl(hdev->base + REG_SCH_STAT);
	u32 glbc;

	if (!status)
		return IRQ_NONE;

	/* Mask all scheduler interrupt enables in REG_SCH_GLBC */
	glbc = readl(hdev->base + REG_SCH_GLBC);
	writel(glbc & ~SCH_INTE_MASK, hdev->base + REG_SCH_GLBC);

	if (status & SCH_STAT_ENDFLAG) {
		if (status & SCH_STAT_JPGEND) {
			hdev->bslen = readl(hdev->base + REG_JPGC_STAT) & JPGC_STAT_LEN_MASK;
			writel(readl(hdev->base + REG_JPGC_STAT) & ~JPGC_STAT_ENDF,
			       hdev->base + REG_JPGC_STAT);
		} else {
			hdev->bslen = readl(hdev->base + REG_SDE_CFG9);
			writel(readl(hdev->base + REG_SDE_STAT) & ~SDE_STAT_BSEND,
			       hdev->base + REG_SDE_STAT);
			writel(readl(hdev->base + REG_DBLK_GSTA) & ~DBLK_STAT_DOEND,
			       hdev->base + REG_DBLK_GSTA);
		}
	} else {
		hdev->bslen = 0;
	}

	WRITE_ONCE(hdev->irq_status, status);
	complete(&hdev->completion);
	return IRQ_HANDLED;
}

static void helix_free(struct kref *ref)
{
	struct helix_dev *hdev = container_of(ref, struct helix_dev, ref);

	if (hdev->workqueue)
		destroy_workqueue(hdev->workqueue);
	v4l2_m2m_release(hdev->m2m_dev);
	v4l2_device_unregister(&hdev->v4l2_dev);
	put_device(hdev->dev);
	kfree(hdev);
}

static void helix_video_release(struct video_device *vfd)
{
	struct helix_dev *hdev = video_get_drvdata(vfd);

	kref_put(&hdev->ref, helix_free);
}

static int helix_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct helix_dev *hdev;
	int ret;

	hdev = kzalloc(sizeof(*hdev), GFP_KERNEL);
	if (!hdev)
		return -ENOMEM;
	hdev->dev = get_device(dev);
	kref_init(&hdev->ref);
	mutex_init(&hdev->dev_mutex);
	mutex_init(&hdev->hw_lock);
	init_completion(&hdev->completion);
	INIT_WORK(&hdev->work, helix_job_work);
	INIT_LIST_HEAD(&hdev->contexts);
	hdev->workqueue = alloc_ordered_workqueue(HELIX_NAME, WQ_MEM_RECLAIM);
	if (!hdev->workqueue) {
		ret = -ENOMEM;
		goto err_free;
	}
	platform_set_drvdata(pdev, hdev);
	hdev->base = devm_platform_ioremap_resource(pdev, 0);
	if (IS_ERR(hdev->base)) {
		ret = PTR_ERR(hdev->base);
		goto err_free;
	}
	hdev->reset = devm_reset_control_get_exclusive(dev, NULL);
	if (IS_ERR(hdev->reset)) {
		ret = PTR_ERR(hdev->reset);
		goto err_free;
	}
	hdev->clocks[0].id = "vpu";
	hdev->clocks[1].id = "gate";
	hdev->clocks[2].id = "ivdc";
	ret = devm_clk_bulk_get(dev, ARRAY_SIZE(hdev->clocks), hdev->clocks);
	if (ret)
		goto err_free;
	ret = clk_bulk_prepare_enable(ARRAY_SIZE(hdev->clocks), hdev->clocks);
	if (ret)
		goto err_free;
	ret = helix_hw_reset(hdev);
	if (ret)
		goto err_clk;
	ret = dma_set_mask_and_coherent(dev, DMA_BIT_MASK(32));
	if (ret)
		goto err_clk;
	vb2_dma_contig_set_max_seg_size(dev, DMA_BIT_MASK(32));
	if (!dev->dma_mem) {
		ret = of_reserved_mem_device_init(dev);
		if (ret && ret != -EBUSY && ret != -ENODEV)
			goto err_clk;
	}
	hdev->irq = platform_get_irq(pdev, 0);
	if (hdev->irq < 0) {
		ret = hdev->irq;
		goto err_pool;
	}
	writel(0, hdev->base + REG_SCH_GLBC);
	ret = devm_request_irq(dev, hdev->irq, helix_irq_handler, 0,
			       dev_name(dev), hdev);
	if (ret)
		goto err_pool;
	ret = v4l2_device_register(dev, &hdev->v4l2_dev);
	if (ret)
		goto err_pool;
	hdev->m2m_dev = v4l2_m2m_init(&helix_m2m_ops);
	if (IS_ERR(hdev->m2m_dev)) {
		ret = PTR_ERR(hdev->m2m_dev);
		goto err_v4l2;
	}
	strscpy(hdev->vfd.name, HELIX_NAME, sizeof(hdev->vfd.name));
	hdev->vfd.fops = &helix_v4l2_fops;
	hdev->vfd.ioctl_ops = &helix_v4l2_ioctl_ops;
	hdev->vfd.release = helix_video_release;
	hdev->vfd.lock = &hdev->dev_mutex;
	hdev->vfd.v4l2_dev = &hdev->v4l2_dev;
	hdev->vfd.vfl_dir = VFL_DIR_M2M;
	hdev->vfd.device_caps = V4L2_CAP_VIDEO_M2M | V4L2_CAP_STREAMING;
	video_set_drvdata(&hdev->vfd, hdev);
	kref_get(&hdev->ref);
	ret = video_register_device(&hdev->vfd, VFL_TYPE_VIDEO, -1);
	if (ret) {
		kref_put(&hdev->ref, helix_free);
		goto err_m2m;
	}
	return 0;

err_m2m:
	v4l2_m2m_release(hdev->m2m_dev);
err_v4l2:
	v4l2_device_unregister(&hdev->v4l2_dev);
err_pool:
	of_reserved_mem_device_release(dev);
err_clk:
	clk_bulk_disable_unprepare(ARRAY_SIZE(hdev->clocks), hdev->clocks);
err_free:
	if (hdev->workqueue)
		destroy_workqueue(hdev->workqueue);
	put_device(dev);
	kfree(hdev);
	return ret;
}

static void helix_remove(struct platform_device *pdev)
{
	struct helix_dev *hdev = platform_get_drvdata(pdev);
	struct helix_ctx *ctx;
	int ret;

	mutex_lock(&hdev->dev_mutex);
	WRITE_ONCE(hdev->disconnected, true);
	WRITE_ONCE(hdev->aborting, true);
	list_for_each_entry(ctx, &hdev->contexts, list) {
		WRITE_ONCE(ctx->stopping, true);
		vb2_queue_error(v4l2_m2m_get_src_vq(ctx->m2m_ctx));
		vb2_queue_error(v4l2_m2m_get_dst_vq(ctx->m2m_ctx));
	}
	complete(&hdev->completion);
	flush_workqueue(hdev->workqueue);
	mutex_lock(&hdev->hw_lock);
	writel(0, hdev->base + REG_SCH_GLBC);
	ret = helix_hw_quiesce(hdev);
	mutex_unlock(&hdev->hw_lock);
	list_for_each_entry(ctx, &hdev->contexts, list) {
		v4l2_m2m_streamoff(NULL, ctx->m2m_ctx, V4L2_BUF_TYPE_VIDEO_OUTPUT);
		v4l2_m2m_streamoff(NULL, ctx->m2m_ctx, V4L2_BUF_TYPE_VIDEO_CAPTURE);
	}
	clk_bulk_disable_unprepare(ARRAY_SIZE(hdev->clocks), hdev->clocks);
	if (ret || READ_ONCE(hdev->hw_running) || hdev->h264_hold_valid) {
		/* Keep DMA targets reserved when stop was not acknowledged. */
		dev_err(hdev->dev, "helix DMA not quiescent; retaining DMA buffers\n");
		goto unregister;
	}
	helix_free_dma(hdev);
	of_reserved_mem_device_release(hdev->dev);
unregister:
	mutex_unlock(&hdev->dev_mutex);
	video_unregister_device(&hdev->vfd);
	kref_put(&hdev->ref, helix_free);
}

static const struct of_device_id helix_of_match[] = {
	{ .compatible = "ingenic,t23-helix" },
	{ }
};
MODULE_DEVICE_TABLE(of, helix_of_match);

static struct platform_driver helix_driver = {
	.probe = helix_probe,
	.remove = helix_remove,
	.driver = {
		.name = HELIX_NAME,
		.of_match_table = helix_of_match,
	},
};
module_platform_driver(helix_driver);

MODULE_DESCRIPTION("Ingenic T23 Helix Video Encoder");
MODULE_AUTHOR("Difan Zhang");
MODULE_LICENSE("GPL");
MODULE_IMPORT_NS(DMA_BUF);
