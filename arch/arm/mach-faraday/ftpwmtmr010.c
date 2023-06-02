/*
 *  linux/arch/arm/mach-faraday/ftpwmtmr010.c
 *
 *  Faraday FTPWMTMR010 Timer
 *
 *  Copyright (C) 2009 Faraday Technology
 *  Copyright (C) 2009 Po-Yu Chuang <ratbert@faraday-tech.com>
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <linux/interrupt.h>
#include <linux/clockchips.h>
#include <linux/clocksource.h>
#include <asm/io.h>
#include <asm/mach/irq.h>

#include <mach/ftpwmtmr010.h>

/******************************************************************************
 * internal functions
 *****************************************************************************/
static inline void ftpwmtmr010_clear_int(void __iomem *base, unsigned int id)
{
	writel(1 << id, base + FTPWMTMR010_OFFSET_INTSTAT);
}

static inline void ftpwmtmr010_write_timer(void __iomem *base, unsigned int id,
		unsigned int reg, unsigned int value)
{
	void __iomem *addr = base + FTPWMTMR010_OFFSET_TIMER(id + 1) + reg;
	writel(value, addr);
}

static inline unsigned int ftpwmtmr010_read_timer(void __iomem *base,
		unsigned int id, unsigned int reg)
{
	void __iomem *addr = base + FTPWMTMR010_OFFSET_TIMER(id + 1) + reg;
	return readl(addr);
}

static void ftpwmtmr010_enable_noirq(void __iomem *base, unsigned int id)
{
	unsigned int cr;

	cr = FTPWMTMR010_CTRL_START | FTPWMTMR010_CTRL_AUTO;

	ftpwmtmr010_write_timer(base, id, FTPWMTMR010_OFFSET_CTRL, cr);
}

static void ftpwmtmr010_disable(void __iomem *base, unsigned int id)
{
	ftpwmtmr010_write_timer(base, id, FTPWMTMR010_OFFSET_CTRL, 0);
}

static unsigned int ftpwmtmr010_get_counter(void __iomem *base, unsigned int id)
{
	return ftpwmtmr010_read_timer(base, id, FTPWMTMR010_OFFSET_CNT);
}

static void ftpwmtmr010_set_reload(void __iomem *base, unsigned int id,
		unsigned int value)
{
	unsigned int cr;

	ftpwmtmr010_write_timer(base, id, FTPWMTMR010_OFFSET_LOAD, value);

	cr = ftpwmtmr010_read_timer(base, id, FTPWMTMR010_OFFSET_CTRL);
	cr |= FTPWMTMR010_CTRL_UPDATE;
	ftpwmtmr010_write_timer(base, id, FTPWMTMR010_OFFSET_CTRL, cr);
}

static void ftpwmtmr010_set_cmp(void __iomem *base, unsigned int id, unsigned int value)
{
	unsigned int cr;

	ftpwmtmr010_write_timer(base, id, FTPWMTMR010_OFFSET_CMP, value);

	cr = ftpwmtmr010_read_timer(base, id, FTPWMTMR010_OFFSET_CTRL);
	cr |= FTPWMTMR010_CTRL_UPDATE;
	ftpwmtmr010_write_timer(base, id, FTPWMTMR010_OFFSET_CTRL, cr);
}

/******************************************************************************
 * clockevent functions
 *****************************************************************************/
static int ftpwmtmr010_set_next_event(unsigned long clc, struct clock_event_device *ce)
{
	struct ftpwmtmr010_clockevent *ftpwmtmr010;

	ftpwmtmr010 = container_of(ce, struct ftpwmtmr010_clockevent, clockevent);

	ftpwmtmr010_set_reload(ftpwmtmr010->base, ftpwmtmr010->id, clc);

	return 0;
}

static void ftpwmtmr010_set_mode(enum clock_event_mode mode, struct clock_event_device *ce)
{
	struct ftpwmtmr010_clockevent *ftpwmtmr010;
	unsigned int cr;

	ftpwmtmr010 = container_of(ce, struct ftpwmtmr010_clockevent, clockevent);

	switch (mode) {
	case CLOCK_EVT_MODE_PERIODIC:
		ftpwmtmr010_set_reload(ftpwmtmr010->base, ftpwmtmr010->id,
				ftpwmtmr010->reload);

		cr = FTPWMTMR010_CTRL_START
		   | FTPWMTMR010_CTRL_AUTO
		   | FTPWMTMR010_CTRL_INT_EN;

		break;

	case CLOCK_EVT_MODE_RESUME:
		cr = ftpwmtmr010_read_timer(ftpwmtmr010->base, ftpwmtmr010->id,
				FTPWMTMR010_OFFSET_CTRL);

		cr |= FTPWMTMR010_CTRL_START;

		break;

	case CLOCK_EVT_MODE_ONESHOT:
		cr = FTPWMTMR010_CTRL_START
		   | FTPWMTMR010_CTRL_INT_EN;

		break;

	case CLOCK_EVT_MODE_UNUSED:
	case CLOCK_EVT_MODE_SHUTDOWN:
	default:
		cr = ftpwmtmr010_read_timer(ftpwmtmr010->base, ftpwmtmr010->id,
				FTPWMTMR010_OFFSET_CTRL);

		cr &= ~FTPWMTMR010_CTRL_START;

		break;
	}

	ftpwmtmr010_write_timer(ftpwmtmr010->base, ftpwmtmr010->id,
			FTPWMTMR010_OFFSET_CTRL, cr);
}

static irqreturn_t ftpwmtmr010_clockevent_interrupt(int irq, void *dev_id)
{
	struct ftpwmtmr010_clockevent *ftpwmtmr010 = dev_id;
	struct clock_event_device *ce = &ftpwmtmr010->clockevent;

	ftpwmtmr010_clear_int(ftpwmtmr010->base, ftpwmtmr010->id);
	ce->event_handler(ce);
	return IRQ_HANDLED;
}

void __init ftpwmtmr010_clockevent_init(struct ftpwmtmr010_clockevent *ftpwmtmr010)
{
	struct clock_event_device *ce = &ftpwmtmr010->clockevent;
	struct irqaction *action = &ftpwmtmr010->irqaction;

	if (!ftpwmtmr010->base || ftpwmtmr010->id >= 8)
		BUG();

	/* initialize to a known state */
	ftpwmtmr010_disable(ftpwmtmr010->base, ftpwmtmr010->id);
	ftpwmtmr010_set_cmp(ftpwmtmr010->base, ftpwmtmr010->id, 0);

	/* setup reload value for periodic clockevents */
	ftpwmtmr010->reload	= ftpwmtmr010->freq / HZ;

	/* Make irqs happen for the system timer */
	action->name		= ce->name;
	action->handler		= ftpwmtmr010_clockevent_interrupt;
	action->flags		= IRQF_DISABLED | IRQF_TIMER | IRQF_IRQPOLL;
	action->dev_id		= ftpwmtmr010;

	setup_irq(ce->irq, action);

	/* setup struct clock_event_device */
	ce->features		= CLOCK_EVT_FEAT_PERIODIC | CLOCK_EVT_FEAT_ONESHOT;
	ce->shift		= 32;
	ce->rating		= 200;
#if 0
	ce->cpumask		= cpu_all_mask,
#else
	ce->cpumask		= cpumask_of(0),
#endif

	ce->mult		= div_sc(ftpwmtmr010->freq, NSEC_PER_SEC, ce->shift);
	ce->max_delta_ns	= clockevent_delta2ns(0xffffffff, ce);
	ce->min_delta_ns	= clockevent_delta2ns(0xf, ce);

	ce->set_next_event	= ftpwmtmr010_set_next_event;
	ce->set_mode		= ftpwmtmr010_set_mode;

	clockevents_register_device(ce);
}

/******************************************************************************
 * clocksource functions
 *****************************************************************************/
static cycle_t ftpwmtmr010_clocksource_read(struct clocksource *cs)
{
	struct ftpwmtmr010_clocksource *ftpwmtmr010;
	cycle_t counter;

	ftpwmtmr010 = container_of(cs, struct ftpwmtmr010_clocksource, clocksource);
	counter = ftpwmtmr010_get_counter(ftpwmtmr010->base, ftpwmtmr010->id);
	return ~counter;
}

void __init ftpwmtmr010_clocksource_init(struct ftpwmtmr010_clocksource *ftpwmtmr010)
{
	struct clocksource *cs = &ftpwmtmr010->clocksource;

	if (!ftpwmtmr010->base || ftpwmtmr010->id >= 8)
		BUG();

	cs->rating	= 300;
	cs->read	= ftpwmtmr010_clocksource_read;
	cs->mask	= CLOCKSOURCE_MASK(32);
	cs->shift	= 20;
	cs->flags	= CLOCK_SOURCE_IS_CONTINUOUS;
	cs->mult	= clocksource_hz2mult(ftpwmtmr010->freq, cs->shift);

	/* setup as free-running clocksource */
	ftpwmtmr010_disable(ftpwmtmr010->base, ftpwmtmr010->id);
	ftpwmtmr010_set_cmp(ftpwmtmr010->base, ftpwmtmr010->id, 0);
	ftpwmtmr010_set_reload(ftpwmtmr010->base, ftpwmtmr010->id, 0xffffffff);
	ftpwmtmr010_enable_noirq(ftpwmtmr010->base, ftpwmtmr010->id);

	clocksource_register(cs);
}
