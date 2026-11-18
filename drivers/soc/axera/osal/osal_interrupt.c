/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/printk.h>
#include <linux/interrupt.h>

#include "osal_ax.h"
#include "osal_dev_ax.h"

int AX_OSAL_DEV_request_threaded_irq_ex(unsigned int irq, AX_IRQ_HANDLER_T handler, AX_IRQ_HANDLER_T thread_fn,
					   unsigned long flags, const char * name, void * dev)
{
	return request_threaded_irq(irq, (irq_handler_t) handler, (irq_handler_t) thread_fn, flags, name, dev);
}

EXPORT_SYMBOL(AX_OSAL_DEV_request_threaded_irq_ex);

int AX_OSAL_DEV_request_threaded_irq(unsigned int irq, AX_IRQ_HANDLER_T handler, AX_IRQ_HANDLER_T thread_fn,
					const char * name, void * dev)
{
	unsigned long flags = IRQF_SHARED;

	return request_threaded_irq(irq, (irq_handler_t) handler, (irq_handler_t) thread_fn, flags, name, dev);
}

EXPORT_SYMBOL(AX_OSAL_DEV_request_threaded_irq);

const void *AX_OSAL_DEV_free_irq(unsigned int irq, void * dev)
{
	const char *devname = free_irq(irq, dev);

	return devname;
}

EXPORT_SYMBOL(AX_OSAL_DEV_free_irq);

int AX_OSAL_DEV_in_interrupt(void)
{
	return in_interrupt();
}

EXPORT_SYMBOL(AX_OSAL_DEV_in_interrupt);

void AX_OSAL_DEV_enable_irq(unsigned int irq)
{
	enable_irq(irq);
}

EXPORT_SYMBOL(AX_OSAL_DEV_enable_irq);

void AX_OSAL_DEV_disable_irq(unsigned int irq)
{
	disable_irq(irq);
}

EXPORT_SYMBOL(AX_OSAL_DEV_disable_irq);

void AX_OSAL_DEV_disable_irq_nosync(unsigned int irq)
{
	disable_irq_nosync(irq);
}

EXPORT_SYMBOL(AX_OSAL_DEV_disable_irq_nosync);

int AX_OSAL_DEV_irq_get_irqchip_state(unsigned int irq, enum AX_OSAL_irqchip_irq_state which, int * state)
{
	return irq_get_irqchip_state(irq, which, (bool *) state);
}

EXPORT_SYMBOL(AX_OSAL_DEV_irq_get_irqchip_state);

int AX_OSAL_DEV_irq_set_irqchip_state(unsigned int irq, enum AX_OSAL_irqchip_irq_state which, int val)
{
	return irq_set_irqchip_state(irq, which, val);
}

EXPORT_SYMBOL(AX_OSAL_DEV_irq_set_irqchip_state);
