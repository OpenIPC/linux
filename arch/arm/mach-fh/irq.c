/*
 * Fullhan FH board support
 *
 * Copyright (C) 2014 Fullhan Microelectronics Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/io.h>

#include <mach/hardware.h>
#include <mach/system.h>
#include <asm/mach/irq.h>

#include <mach/chip.h>
#include <mach/fh_predefined.h>
#include <mach/irqs.h>

static void fh_intc_ack(struct irq_data *d)
{

}
static void fh_intc_enable(struct irq_data *d)
{
	if (d->irq >= NR_INTERNAL_IRQS)
		return;

	if (d->irq > 31) {
		SET_REG_M(VINTC(REG_IRQ_EN_HIGH), 1 << (d->irq - 32),
			  1 << (d->irq - 32));
	} else
		SET_REG_M(VINTC(REG_IRQ_EN_LOW), 1 << d->irq, 1 << d->irq);

}
static void fh_intc_disable(struct irq_data *d)
{
	if (d->irq >= NR_INTERNAL_IRQS)
		return;
	if (d->irq > 31)
		SET_REG_M(VINTC(REG_IRQ_EN_HIGH), 0, 1 << (d->irq - 32));
	else
		SET_REG_M(VINTC(REG_IRQ_EN_LOW), 0, 1 << d->irq);
}

static void fh_intc_mask(struct irq_data *d)
{
	if (d->irq >= NR_INTERNAL_IRQS)
		return;
	if (d->irq > 31) {
		SET_REG_M(VINTC(REG_IRQ_IRQMASK_HIGH), 1 << (d->irq - 32),
			  1 << (d->irq - 32));
	} else
		SET_REG_M(VINTC(REG_IRQ_IRQMASK_LOW), 1 << d->irq, 1 << d->irq);
}

static void fh_intc_unmask(struct irq_data *d)
{
	if (d->irq >= NR_INTERNAL_IRQS)
		return;
	if (d->irq > 31)
		SET_REG_M(VINTC(REG_IRQ_IRQMASK_HIGH), 0, 1 << (d->irq - 32));
	else
		SET_REG_M(VINTC(REG_IRQ_IRQMASK_LOW), 0, 1 << d->irq);
}

#ifdef CONFIG_PM

static u32 wakeups_high;
static u32 wakeups_low;
static u32 backups_high;
static u32 backups_low;

static int fh_intc_set_wake(struct irq_data *d, unsigned value)
{
	if (unlikely(d->irq >= NR_IRQS))
		return -EINVAL;

	if (value) {
		if (d->irq > 31)
			wakeups_high |= (1 << (d->irq - 32));
		else
			wakeups_low |= (1 << d->irq);
	} else {
		if (d->irq > 31)
			wakeups_high &= ~(1 << (d->irq - 32));
		else
			wakeups_low &= ~(1 << d->irq);
	}
	return 0;
}

void fh_irq_suspend(void)
{
	backups_high = GET_REG(VINTC(REG_IRQ_EN_HIGH));
	backups_low = GET_REG(VINTC(REG_IRQ_EN_LOW));

	SET_REG(VINTC(REG_IRQ_EN_HIGH), wakeups_high);
	SET_REG(VINTC(REG_IRQ_EN_LOW), wakeups_low);
}

void fh_irq_resume(void)
{
	SET_REG(VINTC(REG_IRQ_EN_HIGH), backups_high);
	SET_REG(VINTC(REG_IRQ_EN_LOW), backups_low);
}

#else
#define fh_intc_set_wake   NULL
#endif

static struct irq_chip fh_irq_chip = {
	.name		= "FH_INTC",
	.irq_ack	= fh_intc_ack,
	.irq_mask	= fh_intc_mask,
	.irq_unmask	= fh_intc_unmask,

	.irq_enable = fh_intc_enable,
	.irq_disable = fh_intc_disable,
	.irq_set_wake = fh_intc_set_wake,
};

void __init fh_intc_init(void)
{
	int i;

	//disable all interrupts
	SET_REG(VINTC(REG_IRQ_EN_LOW), 0x0);
	SET_REG(VINTC(REG_IRQ_EN_HIGH), 0x0);

	for (i = 0; i < NR_IRQS; i++) {
		irq_set_chip_and_handler(i, &fh_irq_chip, handle_level_irq);
		set_irq_flags(i, IRQF_VALID | IRQF_PROBE);
	}

}
