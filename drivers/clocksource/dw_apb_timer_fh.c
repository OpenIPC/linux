// SPDX-License-Identifier: GPL-2.0
/*
 * Fullhan FH8626V100 Synopsys DW APB dual timer
 *
 * Copyright (C) 2026 Difan Zhang
 *
 * Channel 1 is a free-running down-counter used as clocksource, sched_clock
 * and delay timer; channel 0 is the clockevent. Both channels share an
 * input divider whose rate is requested from the DT clock-frequency.
 */

#define pr_fmt(fmt) "fh8626-timer: " fmt

#include <linux/bitops.h>
#include <linux/clk.h>
#include <linux/clockchips.h>
#include <linux/clocksource.h>
#include <linux/delay.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/irq.h>
#include <linux/kernel.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/sched_clock.h>

#define TMR0_LOAD_COUNT		0x00
#define TMR0_CURRENT_VALUE	0x04
#define TMR0_CONTROL		0x08
#define TMR0_EOI		0x0c

#define TMR1_LOAD_COUNT		0x14
#define TMR1_CURRENT_VALUE	0x18
#define TMR1_CONTROL		0x1c
#define TMR1_EOI		0x20

#define TMR_CTRL_ENABLE		BIT(0)
#define TMR_CTRL_INT_MASK	BIT(2)

static void __iomem *fh_timer_base;
static u32 fh_timer_rate;

/* Channel 1 counts down; invert it to get a monotonic count. */
static u64 fh_timer_clocksource_read(struct clocksource *cs)
{
	return (u64)~readl_relaxed(fh_timer_base + TMR1_CURRENT_VALUE);
}

static u64 notrace fh_read_sched_clock(void)
{
	return (u64)~readl_relaxed(fh_timer_base + TMR1_CURRENT_VALUE);
}

#ifdef CONFIG_ARM
static unsigned long fh_timer_delay_read(void)
{
	return ~readl_relaxed(fh_timer_base + TMR1_CURRENT_VALUE);
}

static struct delay_timer fh_delay_timer = {
	.read_current_timer = fh_timer_delay_read,
};
#endif

static struct clocksource fh_clocksource = {
	.name	= "fh_timer1",
	.rating	= 300,
	.read	= fh_timer_clocksource_read,
	.mask	= CLOCKSOURCE_MASK(32),
	.flags	= CLOCK_SOURCE_IS_CONTINUOUS,
};

static struct clock_event_device fh_clockevent;

static int fh_timer_shutdown(struct clock_event_device *evt)
{
	writel(TMR_CTRL_INT_MASK, fh_timer_base + TMR0_CONTROL);
	readl(fh_timer_base + TMR0_EOI);
	return 0;
}

static int fh_timer_set_oneshot(struct clock_event_device *evt)
{
	/*
	 * Do not inherit the bootloader's interrupt-mask or periodic-mode
	 * bit.
	 */
	writel(0, fh_timer_base + TMR0_CONTROL);
	readl(fh_timer_base + TMR0_EOI);
	return 0;
}

static int fh_timer_next_event(unsigned long delta,
			       struct clock_event_device *evt)
{
	unsigned int tries;

	writel(0, fh_timer_base + TMR0_CONTROL);
	/*
	 * Wait for CURRENT to clear after the disable. This may run in hard-IRQ
	 * context, so the poll is bounded and does not use udelay().
	 */
	for (tries = 0; tries < 50; tries++) {
		if (!readl(fh_timer_base + TMR0_CURRENT_VALUE))
			break;
		cpu_relax();
	}
	if (tries == 50)
		return -ETIME;
	readl(fh_timer_base + TMR0_EOI);
	writel(delta, fh_timer_base + TMR0_LOAD_COUNT);
	writel(TMR_CTRL_ENABLE, fh_timer_base + TMR0_CONTROL);
	return 0;
}

static irqreturn_t fh_timer_irq_handler(int irq, void *dev_id)
{
	struct clock_event_device *evt = dev_id;

	/* Disarm before the callback, which may install the next deadline. */
	writel(0, fh_timer_base + TMR0_CONTROL);
	readl(fh_timer_base + TMR0_EOI);
	evt->event_handler(evt);
	return IRQ_HANDLED;
}

static int __init fh_timer_of_init(struct device_node *np)
{
	u32 requested_rate = 0;
	struct clk *clk;
	int irq, ret;

	fh_timer_base = of_iomap(np, 0);
	if (!fh_timer_base) {
		pr_err("failed to map timer registers\n");
		return -ENOMEM;
	}

	irq = irq_of_parse_and_map(np, 0);
	if (irq <= 0) {
		pr_err("failed to parse IRQ from DT\n");
		ret = -EINVAL;
		goto err_unmap;
	}

	ret = of_property_read_u32(np, "clock-frequency", &requested_rate);
	if (ret || !requested_rate) {
		pr_err("invalid or missing clock-frequency\n");
		ret = ret ?: -EINVAL;
		goto err_unmap;
	}

	/* Stop both inherited channels before changing their shared divider. */
	writel(TMR_CTRL_INT_MASK, fh_timer_base + TMR0_CONTROL);
	writel(TMR_CTRL_INT_MASK, fh_timer_base + TMR1_CONTROL);
	readl(fh_timer_base + TMR0_EOI);
	readl(fh_timer_base + TMR1_EOI);

	clk = of_clk_get(np, 0);
	if (IS_ERR(clk)) {
		ret = PTR_ERR(clk);
		pr_err("failed to get input clock: %d\n", ret);
		goto err_unmap;
	}
	ret = clk_prepare_enable(clk);
	if (ret) {
		pr_err("failed to enable input clock: %d\n", ret);
		goto err_put_clk;
	}
	ret = clk_set_rate(clk, requested_rate);
	if (ret) {
		pr_err("failed to set input clock rate: %d\n", ret);
		goto err_disable_clk;
	}
	fh_timer_rate = clk_get_rate(clk);
	if (!fh_timer_rate) {
		pr_err("invalid input clock rate\n");
		ret = -EINVAL;
		goto err_disable_clk;
	}

	fh_clockevent.name = "fh_timer0";
	fh_clockevent.features = CLOCK_EVT_FEAT_ONESHOT;
	fh_clockevent.rating = 300;
	fh_clockevent.cpumask = cpu_possible_mask;
	fh_clockevent.set_state_shutdown = fh_timer_shutdown;
	fh_clockevent.set_state_oneshot = fh_timer_set_oneshot;
	fh_clockevent.set_next_event = fh_timer_next_event;

	ret = request_irq(irq, fh_timer_irq_handler, IRQF_TIMER | IRQF_IRQPOLL,
			  "fh_timer", &fh_clockevent);
	if (ret) {
		pr_err("failed to request IRQ %d: %d\n", irq, ret);
		goto err_disable_clk;
	}

	/* Channel 1: free-running full-range counter, interrupt masked. */
	writel(0, fh_timer_base + TMR1_CONTROL);
	writel(0xffffffff, fh_timer_base + TMR1_LOAD_COUNT);
	writel(TMR_CTRL_ENABLE | TMR_CTRL_INT_MASK,
	       fh_timer_base + TMR1_CONTROL);

	ret = clocksource_register_hz(&fh_clocksource, fh_timer_rate);
	if (ret) {
		pr_err("failed to register clocksource: %d\n", ret);
		goto err_free_irq;
	}

	sched_clock_register(fh_read_sched_clock, 32, fh_timer_rate);

#ifdef CONFIG_ARM
	fh_delay_timer.freq = fh_timer_rate;
	register_current_timer_delay(&fh_delay_timer);
#endif

	clockevents_config_and_register(&fh_clockevent, fh_timer_rate, 15,
					0x7fffffff);
	return 0;

err_free_irq:
	free_irq(irq, &fh_clockevent);
err_disable_clk:
	clk_disable_unprepare(clk);
err_put_clk:
	clk_put(clk);
err_unmap:
	iounmap(fh_timer_base);
	fh_timer_base = NULL;
	return ret;
}

TIMER_OF_DECLARE(fh8626v100_timer, "fullhan,fh8626v100-timer",
		 fh_timer_of_init);
TIMER_OF_DECLARE(fh8626_timer, "fullhan,fh8626-timer", fh_timer_of_init);
