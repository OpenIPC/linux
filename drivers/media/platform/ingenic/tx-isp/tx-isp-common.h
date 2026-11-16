/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2026 Difan Zhang */
#ifndef __TX_ISP_COMMON_H__
#define __TX_ISP_COMMON_H__

#include <linux/interrupt.h>
#include <linux/platform_device.h>
struct tx_isp_subdev;

struct tx_isp_subdev_ops {
	irqreturn_t (*interrupt_service_routine)(struct tx_isp_subdev *sd,
					       bool *handled);
};

struct tx_isp_subdev {
	struct resource *res;
	void __iomem *base;
	struct tx_isp_subdev_ops *ops;
	void *dev_priv;
};

int tx_isp_subdev_init(struct platform_device *pdev, struct tx_isp_subdev *sd,
		       struct tx_isp_subdev_ops *ops);
void tx_isp_subdev_deinit(struct tx_isp_subdev *sd);
#endif
