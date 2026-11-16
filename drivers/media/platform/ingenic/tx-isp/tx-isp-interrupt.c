// SPDX-License-Identifier: GPL-2.0
/*
 * Ingenic T23 Video Processor Subsystem - Interrupt Management
 *
 * Copyright (C) 2017-2023 Ingenic Semiconductor Co., Ltd.
 * Copyright (C) 2026 Difan Zhang
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>

#include "tx-isp-device.h"

static irqreturn_t isp_irq_handle(int irq, void *data)
{
	struct isp_irq_binding *binding = data;
	struct tx_isp_subdev *sd = binding->sd;
	bool handled = false;
	irqreturn_t ret;

	if (!sd || !sd->ops || !sd->ops->interrupt_service_routine)
		return IRQ_NONE;
	ret = sd->ops->interrupt_service_routine(sd, &handled);
	if (handled)
		return IRQ_HANDLED;
	return ret;
}

int tx_isp_request_irq(struct platform_device *pdev,
		       struct tx_isp_device *ispdev)
{
	int irq;
	int ret;
	struct isp_irq_binding *binding;
	struct tx_isp_subdev *sd;

	if (!pdev || !ispdev)
		return -EINVAL;
	if (pdev->id < 0 || pdev->id >= TX_ISP_SUBDEV_MAX)
		return -EINVAL;
	binding = &ispdev->bindings[pdev->id];
	if (binding->requested)
		return -EBUSY;
	sd = ispdev->subdevs[pdev->id];
	if (!sd || !sd->ops || !sd->ops->interrupt_service_routine)
		return 0;

	irq = platform_get_irq(pdev, 0);
	if (irq < 0)
		return irq;
	binding->owner = ispdev;
	binding->sd = sd;
	binding->irq = irq;

	ret = request_threaded_irq(irq, NULL, isp_irq_handle, IRQF_ONESHOT,
				   dev_name(&pdev->dev), binding);
	if (ret) {
		dev_err(&pdev->dev, "failed to request IRQ %d: %d\n", irq, ret);
		return ret;
	}

	binding->requested = true;
	disable_irq(irq);
	return 0;
}

void tx_isp_free_irq(struct tx_isp_device *ispdev)
{
	int i;

	for (i = ARRAY_SIZE(ispdev->bindings) - 1; i >= 0; i--)
		if (ispdev->bindings[i].owner == ispdev) {
			if (ispdev->bindings[i].requested)
				free_irq(ispdev->bindings[i].irq, &ispdev->bindings[i]);
			memset(&ispdev->bindings[i], 0, sizeof(ispdev->bindings[i]));
		}
}

void tx_isp_enable_irq(struct tx_isp_device *ispdev)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(ispdev->bindings); i++)
		if (ispdev->bindings[i].owner == ispdev && ispdev->bindings[i].requested &&
		    !ispdev->bindings[i].enabled) {
			enable_irq(ispdev->bindings[i].irq);
			ispdev->bindings[i].enabled = true;
		}
}

void tx_isp_disable_irq(struct tx_isp_device *ispdev)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(ispdev->bindings); i++)
		if (ispdev->bindings[i].owner == ispdev && ispdev->bindings[i].requested &&
		    ispdev->bindings[i].enabled) {
			disable_irq(ispdev->bindings[i].irq);
			ispdev->bindings[i].enabled = false;
		}
}
