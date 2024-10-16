// SPDX-License-Identifier: GPL-2.0
/* Copyright (c) 2018-2021 Arm Technology (China) Co., Ltd. All rights reserved. */
/**
 * @file aipu_irq.c
 * Implementation of the interrupt request and handlers' abstraction
 */

#include <linux/slab.h>
#include <linux/interrupt.h>
#include <linux/irqreturn.h>
#include "aipu_irq.h"
#include "aipu_core.h"

static irqreturn_t aipu_irq_handler_upper_half(int irq, void *dev_id)
{
	struct aipu_core *core = (struct aipu_core *)(((struct device *)dev_id)->driver_data);

	return core->ops->upper_half(core);
}

static void aipu_irq_handler_bottom_half(struct work_struct *work)
{
	struct aipu_irq_object *irq_obj = NULL;
	struct aipu_core *core = NULL;

	if (work) {
		irq_obj = container_of(work, struct aipu_irq_object, work);
		core = irq_obj->core;
		core->ops->bottom_half(core);
	}
}

struct aipu_irq_object *aipu_create_irq_object(u32 irqnum, void *core, char *description)
{
	int ret = 0;
	struct aipu_irq_object *irq_obj = NULL;

	if ((!core) || (!description))
		return NULL;

	irq_obj = kzalloc(sizeof(*irq_obj), GFP_KERNEL);
	if (!irq_obj)
		goto finish;

	irq_obj->aipu_wq = NULL;
	irq_obj->irqnum = 0;
	irq_obj->dev = ((struct aipu_core *)core)->dev;

	irq_obj->aipu_wq = create_singlethread_workqueue(description);
	if (!irq_obj->aipu_wq)
		goto err_handle;

	INIT_WORK(&irq_obj->work, aipu_irq_handler_bottom_half);

	ret = request_irq(irqnum, aipu_irq_handler_upper_half, IRQF_SHARED | IRQF_TRIGGER_HIGH,
	    description, irq_obj->dev);
	if (ret) {
		dev_err(irq_obj->dev, "request IRQ (num %u) failed! (errno = %d)", irqnum, ret);
		goto err_handle;
	}

	irq_obj->irqnum = irqnum;
	irq_obj->core = core;

	/* success */
	goto finish;

err_handle:
	aipu_destroy_irq_object(irq_obj);
	irq_obj = NULL;

finish:
	return irq_obj;
}

void aipu_irq_schedulework(struct aipu_irq_object *irq_obj)
{
	if (irq_obj)
		queue_work(irq_obj->aipu_wq, &irq_obj->work);
}

void aipu_irq_flush_workqueue(struct aipu_irq_object *irq_obj)
{
	/* only one workqueue currently */
	flush_workqueue(irq_obj->aipu_wq);
}

void aipu_destroy_irq_object(struct aipu_irq_object *irq_obj)
{
	if (irq_obj) {
		if (irq_obj->aipu_wq) {
			flush_workqueue(irq_obj->aipu_wq);
			destroy_workqueue(irq_obj->aipu_wq);
			irq_obj->aipu_wq = NULL;
		}
		if (irq_obj->irqnum)
			free_irq(irq_obj->irqnum, irq_obj->dev);
		kfree(irq_obj);
		flush_scheduled_work();
	}
}
