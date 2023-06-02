/*
 *  Faraday FTTMR010 Timer
 *
 *  Copyright (C) 2009 Po-Yu Chuang
 *  Copyright (C) 2009 Faraday Corp. (http://www.faraday-tech.com)
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
#include <mach/fttmr010.h>


/******************************************************************************
 * internal functions
 *****************************************************************************/
static const unsigned int fttmr010_cr_mask[3] = {
	FTTMR010_TM1_ENABLE | FTTMR010_TM1_CLOCK |
	FTTMR010_TM1_OFENABLE | FTTMR010_TM1_UPDOWN,

	FTTMR010_TM2_ENABLE | FTTMR010_TM2_CLOCK |
	FTTMR010_TM2_OFENABLE | FTTMR010_TM2_UPDOWN,

	FTTMR010_TM3_ENABLE | FTTMR010_TM3_CLOCK |
	FTTMR010_TM3_OFENABLE | FTTMR010_TM3_UPDOWN,
};

/* we always use down counter */
static const unsigned int fttmr010_cr_enable_flag[3] = {
	FTTMR010_TM1_ENABLE | FTTMR010_TM1_OFENABLE,
	FTTMR010_TM2_ENABLE | FTTMR010_TM2_OFENABLE,
	FTTMR010_TM3_ENABLE | FTTMR010_TM3_OFENABLE,
};

static const unsigned int fttmr010_cr_enable_noirq_flag[3] = {
	FTTMR010_TM1_ENABLE,
	FTTMR010_TM2_ENABLE,
	FTTMR010_TM3_ENABLE,
};

/* TmxEnable + overflow interrupt enable
 */
static void fttmr010_enable(void __iomem *base, unsigned int id)
{
	unsigned int cr = readl(base + FTTMR010_OFFSET_CR);

	cr &= ~fttmr010_cr_mask[id];    /* maskout the mask, all setting goes to zero. */
	cr |= fttmr010_cr_enable_flag[id];
	writel(cr, base + FTTMR010_OFFSET_CR);
}

/* Just no overflow interrupt enable bit
 */
static void fttmr010_enable_noirq(void __iomem *base, unsigned int id)
{
	unsigned int cr = readl(base + FTTMR010_OFFSET_CR);

	cr &= ~fttmr010_cr_mask[id];
	cr |= fttmr010_cr_enable_noirq_flag[id];
	writel(cr, base + FTTMR010_OFFSET_CR);
}

static void fttmr010_disable(void __iomem *base, unsigned int id)
{
	unsigned int cr = readl(base + FTTMR010_OFFSET_CR);

	cr &= ~fttmr010_cr_mask[id];
	writel(cr, base + FTTMR010_OFFSET_CR);
}

static inline void fttmr010_write_timer(void __iomem *base, unsigned int id,
		unsigned int reg, unsigned int value)
{
	void __iomem *addr = base + FTTMR010_OFFSET_TIMER(id) + reg;
	writel (value, addr);
}

static inline unsigned int fttmr010_read_timer(void __iomem *base,
		unsigned int id, unsigned int reg)
{
	void __iomem *addr = base + FTTMR010_OFFSET_TIMER(id) + reg;
	return readl(addr);
}

static void fttmr010_set_counter(void __iomem *base, unsigned int id, unsigned int value)
{
	fttmr010_write_timer(base, id, FTTMR010_OFFSET_COUNTER, value);
}

static inline unsigned int fttmr010_get_counter(void __iomem *base, unsigned int id)
{
	return fttmr010_read_timer(base, id, FTTMR010_OFFSET_COUNTER);
}

static void fttmr010_set_reload(void __iomem *base, unsigned int id, unsigned int value)
{
	fttmr010_write_timer(base, id, FTTMR010_OFFSET_LOAD, value);
}

static void fttmr010_set_match1(void __iomem *base, unsigned int id, unsigned int value)
{
	fttmr010_write_timer(base, id, FTTMR010_OFFSET_MATCH1, value);
}

static void fttmr010_set_match2(void __iomem *base, unsigned int id, unsigned int value)
{
	fttmr010_write_timer(base, id, FTTMR010_OFFSET_MATCH2, value);
}

/******************************************************************************
 * clockevent functions. Set the next trigger event
 *****************************************************************************/
static int fttmr010_set_next_event(unsigned long clc, struct clock_event_device *ce)
{
	struct fttmr010_clockevent *fttmr010;

	fttmr010 = container_of(ce, struct fttmr010_clockevent, clockevent);
	fttmr010_set_counter(fttmr010->base, fttmr010->id, clc);

	return 0;
}

static void fttmr010_set_mode(enum clock_event_mode mode, struct clock_event_device *ce)
{
	struct fttmr010_clockevent *fttmr010;

	fttmr010 = container_of(ce, struct fttmr010_clockevent, clockevent);

	switch (mode) {
	case CLOCK_EVT_MODE_PERIODIC:
		fttmr010_set_reload(fttmr010->base, fttmr010->id, fttmr010->reload);
		fttmr010_set_counter(fttmr010->base, fttmr010->id, fttmr010->reload);
		fttmr010_enable(fttmr010->base, fttmr010->id);
		break;

	case CLOCK_EVT_MODE_RESUME:
		fttmr010_enable(fttmr010->base, fttmr010->id);
		break;

	case CLOCK_EVT_MODE_ONESHOT:
		fttmr010_set_reload(fttmr010->base, fttmr010->id, 0);
		fttmr010_enable(fttmr010->base, fttmr010->id);
		break;

	case CLOCK_EVT_MODE_UNUSED:
	case CLOCK_EVT_MODE_SHUTDOWN:
	default:
		fttmr010_disable(fttmr010->base, fttmr010->id);
		break;
	}
}

static irqreturn_t fttmr010_clockevent_interrupt(int irq, void *dev_id)
{
	struct clock_event_device *ce = dev_id;
	struct fttmr010_clockevent *fttmr010;
    unsigned int tmp;

	fttmr010 = container_of(ce, struct fttmr010_clockevent, clockevent);

    /* Level Trigger needs this fix */
	tmp = readl(fttmr010->base + FTTMR010_OFFSET_INTR_STATE);
	writel(tmp, fttmr010->base + FTTMR010_OFFSET_INTR_STATE);

	ce->event_handler(ce);
	return IRQ_HANDLED;
}

void __init fttmr010_clockevent_init(struct fttmr010_clockevent *fttmr010)
{
	struct clock_event_device *ce = &fttmr010->clockevent;
	struct irqaction *action = &fttmr010->irqaction;

	if (!fttmr010->base || fttmr010->id >= 3)
		BUG();

	/* initialize to a known state */
	fttmr010_disable(fttmr010->base, fttmr010->id);
	fttmr010_set_match1(fttmr010->base, fttmr010->id, 0);
	fttmr010_set_match2(fttmr010->base, fttmr010->id, 0);

	/* setup reload value for periodic clockevents */
	fttmr010->reload	= (fttmr010->freq / HZ) - 1;    /* FIX */

	/* Make irqs happen for the system timer */
	action->name		= ce->name;
	action->handler		= fttmr010_clockevent_interrupt;
	action->flags		= IRQF_DISABLED | IRQF_TIMER;
	action->dev_id		= ce;

	setup_irq(ce->irq, action);

	/* setup struct clock_event_device */
	ce->features	= CLOCK_EVT_FEAT_PERIODIC | CLOCK_EVT_FEAT_ONESHOT;
	ce->shift		= 32;
	ce->rating		= 200;
	ce->cpumask		= cpumask_of(0);    //CPU_MASK_ALL,

	ce->mult		= div_sc(fttmr010->freq, NSEC_PER_SEC, ce->shift);
	ce->max_delta_ns	= clockevent_delta2ns(0xffffffff, ce);
	ce->min_delta_ns	= clockevent_delta2ns(0xff, ce);

	ce->set_next_event	= fttmr010_set_next_event;
	ce->set_mode		= fttmr010_set_mode;

	clockevents_register_device(ce);
}

/******************************************************************************
 * clocksource functions
 *****************************************************************************/

/*
 * We support only one instance now.
 *
 * After 2.6.30, clocksource->read() has a parameter, and we can use it with
 * container_of() to get the private data for each instance.
 */
static cycle_t fttmr010_clocksource_read(struct clocksource *cs)
{
    struct fttmr010_clocksource *fttmr010;
	cycle_t counter;

    fttmr010 = container_of(cs, struct fttmr010_clocksource, clocksource);
	counter = fttmr010_get_counter(fttmr010->base, fttmr010->id);
	return ~counter;
}

void __init fttmr010_clocksource_init(struct fttmr010_clocksource *fttmr010)
{
	struct clocksource *cs = &fttmr010->clocksource;

	if (!fttmr010->base || fttmr010->id >= 3)
		BUG();

	cs->rating	= 300;
	cs->read	= fttmr010_clocksource_read;
	cs->mask	= CLOCKSOURCE_MASK(32);
	cs->shift	= 20;
	cs->flags	= CLOCK_SOURCE_IS_CONTINUOUS;
	cs->mult	= clocksource_hz2mult(fttmr010->freq, cs->shift);

	/* setup as free-running clocksource */
	fttmr010_disable(fttmr010->base, fttmr010->id);
	fttmr010_set_match1(fttmr010->base, fttmr010->id, 0);
	fttmr010_set_match2(fttmr010->base, fttmr010->id, 0);
	fttmr010_set_reload(fttmr010->base, fttmr010->id, 0xffffffff);
	fttmr010_set_counter(fttmr010->base, fttmr010->id, 0xffffffff);
	fttmr010_enable_noirq(fttmr010->base, fttmr010->id);

	clocksource_register(cs);

    if (1) {
        extern int gm_jiffies_init(void *);
	    gm_jiffies_init((void *)fttmr010);
	}
}


