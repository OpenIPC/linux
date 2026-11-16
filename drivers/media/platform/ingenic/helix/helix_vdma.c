// SPDX-License-Identifier: GPL-2.0
// Copyright (C) 2026 Difan Zhang

#include "helix_drv.h"
#include "helix_vdma.h"

void helix_vdma_task_init(struct helix_vdma_task *task, __le32 *vaddr,
			  dma_addr_t paddr, size_t max_bytes)
{
	task->vaddr = vaddr;
	task->paddr = paddr;
	task->max_words = max_bytes / sizeof(__le32);
	task->count = 0;
	task->overflow = false;
}

void helix_vdma_append(struct helix_vdma_task *task, u32 reg, u32 val)
{
	if (task->count + 2 > task->max_words) {
		task->overflow = true;
		return;
	}

	task->vaddr[task->count++] = cpu_to_le32(val);
	task->vaddr[task->count++] = cpu_to_le32(VDMA_ACFG_VLD | VDMA_ACFG_IDX(reg));
}

void helix_vdma_terminate(struct helix_vdma_task *task)
{
	if (task->overflow || task->count < 2)
		return;

	task->vaddr[task->count - 1] |= cpu_to_le32(VDMA_ACFG_TERM);
}

int helix_vdma_kick(struct helix_dev *hdev, struct helix_vdma_task *task)
{
	if (WARN_ON_ONCE(task->overflow))
		return -ENOSPC;
	if (task->count < 2)
		return -EINVAL;

	writel(SCH_GLBC_HIAXI | SCH_INTE_ENDF | SCH_INTE_TLBERR |
	       SCH_INTE_BSERR | SCH_INTE_ACFGERR | SCH_INTE_RESERR,
	       hdev->base + REG_SCH_GLBC);
	dma_wmb();
	writel(VDMA_ACFG_DHA(task->paddr) | VDMA_ACFG_RUN, hdev->base + REG_VDMA_TASKRG_T21);
	return 0;
}

int helix_vdma_wait(struct helix_dev *hdev, unsigned int timeout_ms)
{
	unsigned long timeout = msecs_to_jiffies(timeout_ms ? timeout_ms : 2000);
	long ret;

	ret = wait_for_completion_timeout(&hdev->completion, timeout);
	if (ret == 0)
		WRITE_ONCE(hdev->irq_status, readl(hdev->base + REG_SCH_STAT));
	writel(0, hdev->base + REG_SCH_GLBC);
	synchronize_irq(hdev->irq);

	if (ret == 0)
		return -ETIMEDOUT;
	if (READ_ONCE(hdev->aborting) && !READ_ONCE(hdev->irq_status))
		return -ECANCELED;

	if (!(hdev->irq_status & SCH_STAT_ENDFLAG) ||
	    hdev->irq_status & (SCH_STAT_BSERR | SCH_STAT_ACFGERR | SCH_STAT_ORESERR))
		return -EIO;

	return 0;
}

int helix_alloc_h264_buffers(struct helix_dev *hdev,
			     struct helix_h264_buffers *bufs,
			     u32 width, u32 height)
{
	u32 coded_w = ALIGN(width, 16);
	u32 coded_h = ALIGN(height, 16);
	u32 pad = 64;
	size_t y_size = (coded_h + pad) * coded_w;
	size_t c_size = (coded_h / 2 + (pad / 2)) * coded_w;
	int i;

	memset(bufs, 0, sizeof(*bufs));
	bufs->recon_y_size = y_size;
	bufs->recon_c_size = c_size;
	bufs->cur_slot = 0;

	for (i = 0; i < 2; i++) {
		bufs->recon_y_vaddr[i] = dma_alloc_coherent(hdev->dev, y_size,
							    &bufs->recon_y_paddr[i],
							    GFP_KERNEL);
		if (!bufs->recon_y_vaddr[i])
			goto err_free;

		memset(bufs->recon_y_vaddr[i], 0, y_size);

		bufs->recon_c_vaddr[i] = dma_alloc_coherent(hdev->dev, c_size,
							    &bufs->recon_c_paddr[i],
							    GFP_KERNEL);
		if (!bufs->recon_c_vaddr[i])
			goto err_free;

		memset(bufs->recon_c_vaddr[i], 0x80, c_size);
	}

	return 0;

err_free:
	helix_free_h264_buffers(hdev, bufs);
	return -ENOMEM;
}

void helix_free_h264_buffers(struct helix_dev *hdev,
			     struct helix_h264_buffers *bufs)
{
	int i;

	for (i = 0; i < 2; i++) {
		if (bufs->recon_c_vaddr[i]) {
			dma_free_coherent(hdev->dev, bufs->recon_c_size,
					  bufs->recon_c_vaddr[i],
					  bufs->recon_c_paddr[i]);
			bufs->recon_c_vaddr[i] = NULL;
		}
		if (bufs->recon_y_vaddr[i]) {
			dma_free_coherent(hdev->dev, bufs->recon_y_size,
					  bufs->recon_y_vaddr[i],
					  bufs->recon_y_paddr[i]);
			bufs->recon_y_vaddr[i] = NULL;
		}
	}
}
