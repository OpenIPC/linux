/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (c) 2018-2021 Arm Technology (China) Co., Ltd. All rights reserved. */

/**
 * @file aipu_irq.h
 * Header of the interrupt request and handlers' abstraction
 */

#ifndef __AIPU_IRQ_H__
#define __AIPU_IRQ_H__

#include <linux/device.h>
#include <linux/workqueue.h>

typedef int  (*aipu_irq_uhandler_t) (void *arg);
typedef void (*aipu_irq_bhandler_t) (void *arg);
typedef void (*aipu_irq_trigger_t) (void *arg);
typedef void (*aipu_irq_ack_t) (void *arg);

/**
 * struct aipu_irq_object - IRQ instance for each hw module in AIPU with interrupt function
 *
 * @irqnum: interrupt number used to request IRQ
 * @core: aipu core struct pointer
 * @work: work struct
 * @dev: device pointer
 * @aipu_wq: workqueue struct pointer
 */
struct aipu_irq_object {
	u32 irqnum;
	void *core;
	struct work_struct  work;
	struct device *dev;
	struct workqueue_struct *aipu_wq;
};

/**
 * @brief initialize an AIPU IRQ object for a HW module with interrupt function
 *
 * @param irqnum: interrupt number
 * @param core: aipu core struct pointer
 * @param description: irq object description string
 *
 * @return irq_object pointer if successful; NULL if failed;
 */
struct aipu_irq_object *aipu_create_irq_object(u32 irqnum, void *core, char *description);
/**
 * @brief workqueue schedule API
 *
 * @param irq_obj: interrupt object
 *
 * @return void
 */
void aipu_irq_schedulework(struct aipu_irq_object *irq_obj);
/**
 * @brief workqueue flush API
 *
 * @param irq_obj: interrupt object
 *
 * @return void
 */
void aipu_irq_flush_workqueue(struct aipu_irq_object *irq_obj);
/**
 * @brief workqueue terminate API
 *
 * @param irq_obj: interrupt object
 *
 * @return void
 */
void aipu_destroy_irq_object(struct aipu_irq_object *irq_obj);

#endif /* __AIPU_IRQ_H__ */
