/**
 * system/src/rpclnt/rpclnt.h
 *
 * History:
 *    2012/08/15 - [Tzu-Jung Lee] created file
 *
 * Copyright 2008-2015 Ambarella Inc.  All Rights Reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */
 
#include <linux/module.h>
#include <linux/kthread.h>
#include <linux/interrupt.h>
#include <linux/io.h>

#include <plat/remoteproc_cfg.h>
#include <plat/rpdev.h>
#include <plat/rct.h>

#include "rpclnt.h"
#include "vq.h"

extern void rpdev_svq_cb(struct vq *vq);
extern void rpdev_rvq_cb(struct vq *vq);

DECLARE_COMPLETION(rpclnt_comp);

static struct rpclnt G_rproc_ca9_b = {

	.name		= "rpclnt_ca9_a_and_b",
	.inited		= 0,

	.svq_tx_irq	= VRING_CA9_B_TO_A_HOST_IRQ,
	.svq_rx_irq	= VRING_CA9_B_TO_A_CLNT_IRQ,
	.svq_num_bufs	= RPMSG_NUM_BUFS >> 1,
	.svq_buf_phys	= VRING_BUF_CA9_A_AND_B,
	.svq_vring_phys	= VRING_CA9_B_TO_A,
	.svq_vring_algn	= RPMSG_VRING_ALIGN,

	.rvq_tx_irq	= VRING_CA9_A_TO_B_HOST_IRQ,
	.rvq_rx_irq	= VRING_CA9_A_TO_B_CLNT_IRQ,
	.rvq_num_bufs	= RPMSG_NUM_BUFS >> 1,
	.rvq_buf_phys	= VRING_BUF_CA9_A_AND_B + (RPMSG_TOTAL_BUF_SPACE >> 1),
	.rvq_vring_phys	= VRING_CA9_A_TO_B,
	.rvq_vring_algn	= RPMSG_VRING_ALIGN,
};

struct rpclnt *rpclnt_sync(const char *bus_name)
{
	struct rpclnt *rpclnt = &G_rproc_ca9_b;

	if (strcmp(bus_name, "ca9_a_and_b"))
		return NULL;

	if (!rpclnt->inited)
		wait_for_completion(&rpclnt_comp);

	return &G_rproc_ca9_b;
}

static void rpclnt_complete_registration(struct rpclnt *rpclnt)
{
	u32 buf = rpclnt->rvq_buf_phys;

	vq_init_unused_bufs(rpclnt->rvq, (void*)buf, RPMSG_BUF_SIZE);

	rpclnt->inited = 1;
	complete_all(&rpclnt_comp);
}

static irqreturn_t rpclnt_isr(int irq, void *dev_id)
{
	struct rpclnt *rpclnt = dev_id;

	if (!rpclnt->inited && irq == rpclnt->svq_rx_irq) {

		rpclnt_complete_registration(rpclnt);

		amba_writel(AHB_SCRATCHPAD_REG(0x14),
			    0x1 << (irq - AXI_SOFT_IRQ(0)));

		return IRQ_HANDLED;
	}

	/*
	 * Before scheduling the bottom half for processing the messages,
	 * we tell the remote host not to notify us (generate interrupts) when
	 * subsequent messages are enqueued.  The bottom half will pull out
	 * this message and the pending ones.  Once it processed all the messages
	 * in the queue, it will re-enable remote host for the notification.
	 */
	if (irq == rpclnt->rvq_rx_irq) {

		vq_disable_used_notify(rpclnt->rvq);
		schedule_work(&rpclnt->rvq_work);

	} else if (irq == rpclnt->svq_rx_irq) {

		vq_disable_used_notify(rpclnt->svq);
		schedule_work(&rpclnt->svq_work);
	}

	amba_writel(AHB_SCRATCHPAD_REG(0x14),
		    0x1 << (irq - AXI_SOFT_IRQ(0)));

	return IRQ_HANDLED;
}

static void rpclnt_rvq_worker(struct work_struct *work)
{
	struct rpclnt *rpclnt = container_of(work, struct rpclnt, rvq_work);
	struct vq *vq = rpclnt->rvq;

	if (vq->cb)
		vq->cb(vq);
}

static void rpclnt_svq_worker(struct work_struct *work)
{
	struct rpclnt *rpclnt = container_of(work, struct rpclnt, svq_work);
	struct vq *vq = rpclnt->svq;

	if (vq->cb)
		vq->cb(vq);
}

static void rpclnt_kick_rvq(struct vq *vq)
{
	/*
	 * Honor the flag set by the remote host.
	 *
	 * Most of the time, the remote host want to supress their
	 * tx-complete interrupts.  In this case, we don't bother it.
	 */
	if (vq_kick_prepare(vq)) {

		amba_writel(AHB_SCRATCHPAD_REG(0x10),
			    0x1 << (107 - AXI_SOFT_IRQ(0)));
	}
}

static void rpclnt_kick_svq(struct vq *vq)
{
	/*
	 * Honor the flag set by the remote host.
	 *
	 * When the remote host is already busy enough processing the
	 * messages, it might suppress the interrupt and work in polling
	 * mode.
	 */
	if (vq_kick_prepare(vq)) {

		amba_writel(AHB_SCRATCHPAD_REG(0x10),
			    0x1 << (109 - AXI_SOFT_IRQ(0)));
	}
}

static int rpclnt_drv_init(void)
{
	int ret;
	struct rpclnt *rpclnt = &G_rproc_ca9_b;

	rpclnt->svq = vq_create(rpdev_svq_cb,
				rpclnt_kick_svq,
				rpclnt->svq_num_bufs,
				ambarella_phys_to_virt(rpclnt->svq_vring_phys),
				rpclnt->svq_vring_algn);

	rpclnt->rvq = vq_create(rpdev_rvq_cb,
				rpclnt_kick_rvq,
				rpclnt->rvq_num_bufs,
				ambarella_phys_to_virt(rpclnt->rvq_vring_phys),
				rpclnt->rvq_vring_algn);

	INIT_WORK(&rpclnt->svq_work, rpclnt_svq_worker);
	INIT_WORK(&rpclnt->rvq_work, rpclnt_rvq_worker);

	ret = request_irq(rpclnt->svq_rx_irq, rpclnt_isr,
			  IRQF_SHARED, "rpclnt_svq_rx", rpclnt);
	if (ret)
		printk("Error: failed to request svq_rx_irq: %d, err: %d\n",
		       rpclnt->svq_rx_irq, ret);

	ret = request_irq(rpclnt->rvq_rx_irq, rpclnt_isr,
			  IRQF_SHARED, "rpclnt_rvq_rx", rpclnt);

	if (ret)
		printk("Error: failed to request rvq_rx_irq: %d, err: %d\n",
		       rpclnt->rvq_rx_irq, ret);

	return 0;
}

module_init(rpclnt_drv_init);
