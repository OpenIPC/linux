/*
 * FH timer subsystem
 *
 * Copyright (C) 2014 Fullhan Microelectronics Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */
#include <linux/cpu.h>
#include <linux/init.h>
#include <linux/percpu.h>
#include <linux/cpumask.h>
#include <linux/clockchips.h>
#include <linux/clocksource.h>
#include <linux/interrupt.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/clk.h>
#include <linux/sched_clock.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/fh_simple_timer.h>


static struct clocksource *g_cs;

#ifdef CONFIG_ARCH_FH865x
#define TMR_CLK (50000000)
#else
#define TMR_CLK (1000000)
#endif


#define	OFFSET_TIMER_LOADCNT		(0x00)
#define	OFFSET_TIMER_CUR_VAL		(0x04)
#define	OFFSET_TIMER_CTRL_REG		(0x08)
#define	OFFSET_TIMER_EOI_REG		(0x0C)
#define	OFFSET_TIMER_INTSTATUS		(0x10)

#define OFFSET_TIMERS_INTSTATUS		(0xa0)

#define TMR_CONTROL_ENABLE			(1 << 0)
/* 1: periodic, 0:free running. */
#define TMR_CONTROL_MODE_PERIODIC	(1 << 1)
#define TMR_CONTROL_INT				(1 << 2)

struct fh_timer {
	void __iomem			*base;
	unsigned long			freq;
	int						irq;
};

struct fh_timer_clock_event_device {
	struct clock_event_device	ced;
	struct fh_timer				timer;
	struct irqaction			irqaction;
	void						(*eoi)(struct fh_timer *);
};

struct fh_timer_clocksource {
	struct fh_timer			timer;
	struct clocksource		cs;
};




#if defined(CONFIG_SMP)
struct clock_event_device fh_bc_tmrevt;
#endif

static inline struct fh_timer_clock_event_device *
ced_to_fh_timer_ced(struct clock_event_device *evt)
{
	return container_of(evt, struct fh_timer_clock_event_device, ced);
}

static inline struct fh_timer_clocksource *
clocksource_to_fh_timer_clocksource(struct clocksource *cs)
{
	return container_of(cs, struct fh_timer_clocksource, cs);
}

/**
 * fh_timer_clocksource_read() - read the current value of a clocksource.
 *
 * @fh_cs:	The clocksource to read.
 */
cycle_t fh_timer_clocksource_read(struct fh_timer_clocksource *fh_cs)
{
	return (cycle_t)~(readl(fh_cs->timer.base + OFFSET_TIMER_CUR_VAL));
}

static cycle_t fh_timer_read_clocksource(struct clocksource *cs)
{
	unsigned long current_count;
	struct fh_timer_clocksource *fh_cs =
		clocksource_to_fh_timer_clocksource(cs);

	current_count = ~(readl(fh_cs->timer.base + OFFSET_TIMER_CUR_VAL));

	return current_count;
}

/**
 * fh_timer_clocksource_start() - start the clocksource counting.
 *
 * @fh_cs:	The clocksource to start.
 *
 * This is used to start the clocksource before registration and can be used
 * to enable calibration of timers.
 */
void fh_timer_clocksource_start(struct fh_timer_clocksource *fh_cs)
{
	/*
	 * start count down from 0xffff_ffff. this is done by toggling the
	 * enable bit then load initial load count to ~0.
	 */
	unsigned long ctrl = readl(fh_cs->timer.base + OFFSET_TIMER_CTRL_REG);

	ctrl &= ~TMR_CONTROL_ENABLE;
	writel(ctrl, fh_cs->timer.base + OFFSET_TIMER_CTRL_REG);
	writel(~0, fh_cs->timer.base + OFFSET_TIMER_LOADCNT);
	/* enable, mask interrupt */
	ctrl &= ~TMR_CONTROL_MODE_PERIODIC;
	ctrl |= (TMR_CONTROL_ENABLE | TMR_CONTROL_INT);
	writel(ctrl, fh_cs->timer.base + OFFSET_TIMER_CTRL_REG);
	/* read it once to get cached counter value initialized */
	fh_timer_clocksource_read(fh_cs);
}

void fh_timer_clock_start(struct fh_timer *timer)
{
	/*
	 * start count down from 0xffff_ffff. this is done by toggling the
	 * enable bit then load initial load count to ~0.
	 */
	unsigned long ctrl = readl(timer->base + OFFSET_TIMER_CTRL_REG);

	ctrl &= ~TMR_CONTROL_ENABLE;
	writel(ctrl, timer->base + OFFSET_TIMER_CTRL_REG);
	writel(~0, timer->base + OFFSET_TIMER_LOADCNT);
	/* enable, mask interrupt */
	ctrl &= ~TMR_CONTROL_MODE_PERIODIC;
	ctrl |= (TMR_CONTROL_ENABLE | TMR_CONTROL_INT);
	writel(ctrl, timer->base + OFFSET_TIMER_CTRL_REG);
	/* read it once to get cached counter value initialized */
	readl(timer->base + OFFSET_TIMER_CUR_VAL);
}

static void fh_timer_restart_clocksource(struct clocksource *cs)
{
	struct fh_timer_clocksource *fh_cs =
		clocksource_to_fh_timer_clocksource(cs);

	fh_timer_clocksource_start(fh_cs);
}

static void __init timer_get_base_and_rate(struct device_node *np,
				int type, void __iomem **base, u32 *rate)
{
	struct clk *timer_clk;
#ifdef CONFIG_USE_OF
	*base = of_iomap(np, 0);

	if (!*base)
		pr_err("Unable to map regs for %s", np->name);

	timer_clk = clk_get(NULL, "tmr0_clk");


	if (!of_property_read_u32(np, "clock-frequency", rate)) {
		if (!IS_ERR(timer_clk)) {
			clk_set_rate(timer_clk, *rate);
			clk_prepare_enable(timer_clk);
		} else {
			pr_err("No clock  for %s", np->name);
		}
	} else {
		pr_err(" No clock-frequency property for %s", np->name);
		*rate = 0;
	}
#else
	timer_clk = clk_get(NULL, "tmr0_clk");
	if (!IS_ERR(timer_clk)) {
		*rate  = TMR_CLK;
		clk_set_rate(timer_clk, *rate);
		clk_prepare_enable(timer_clk);
	} else
		*rate = 1000000;

	if (type == 1)
		*base = (void __iomem *)(((unsigned int)*base)+0x14);

#endif

}

static void fh_timer_eoi(struct fh_timer *timer)
{
#ifdef CONFIG_ARM_FULLHAN_CPUIDLE
	extern void fhca7_wakeup_cpu(unsigned int cpu);
	fhca7_wakeup_cpu(1);
#endif
	readl(timer->base + OFFSET_TIMER_EOI_REG);
}

static irqreturn_t fh_timer_clockevent_irq(int irq, void *data)
{
	struct clock_event_device *evt = data;
	struct fh_timer_clock_event_device *fh_ced = ced_to_fh_timer_ced(evt);

#ifdef CONFIG_FH_SIMPLE_TIMER
	unsigned int status =
		readl(fh_ced->timer.base + OFFSET_TIMERS_INTSTATUS);

	if (status & (1 << SIMPLE_TIMER_BASE))
		fh_simple_timer_interrupt();
#endif


	if (!evt->event_handler) {
		pr_info("Spurious FH timer interrupt %d", irq);
		return IRQ_NONE;
	}

	if (fh_ced->eoi)
		fh_ced->eoi(&fh_ced->timer);

	evt->event_handler(evt);
#if defined(CONFIG_SMP)
	if (fh_bc_tmrevt.broadcast != NULL)
		fh_bc_tmrevt.broadcast(fh_bc_tmrevt.cpumask);

#endif
	return IRQ_HANDLED;
}
static irqreturn_t fh_bc_timer_clockevent_irq(int irq, void *data)
{
	struct clock_event_device *evt = data;
	struct fh_timer_clock_event_device *fh_ced = ced_to_fh_timer_ced(evt);

	if (!evt->event_handler) {
		pr_info("Spurious FH bc timer interrupt %d", irq);
		return IRQ_NONE;
	}

	if (fh_ced->eoi)
		fh_ced->eoi(&fh_ced->timer);

	evt->event_handler(evt);
	return IRQ_HANDLED;
}




static void fh_timer_enable_int(struct fh_timer *timer)
{
	unsigned long ctrl = readl(timer->base + OFFSET_TIMER_CTRL_REG);
	/* clear pending intr */
	readl(timer->base + OFFSET_TIMER_EOI_REG);
	ctrl &= ~TMR_CONTROL_INT;
	writel(ctrl, timer->base + OFFSET_TIMER_CTRL_REG);
}

static int fh_timer_set_periodic(struct clock_event_device *evt)
{
	unsigned long ctrl;
	unsigned long period;
	int sync_cnt = 0;
	struct fh_timer_clock_event_device *fh_ced = ced_to_fh_timer_ced(evt);

	period = DIV_ROUND_UP(fh_ced->timer.freq, HZ);
	ctrl = readl(fh_ced->timer.base + OFFSET_TIMER_CTRL_REG);
	ctrl |= TMR_CONTROL_MODE_PERIODIC;
	writel(ctrl, fh_ced->timer.base + OFFSET_TIMER_CTRL_REG);
	/*
	 * DW APB p. 46, have to disable timer before load counter,
	 * may cause sync problem.
	 */
	ctrl &= ~TMR_CONTROL_ENABLE;
	writel(ctrl, fh_ced->timer.base + OFFSET_TIMER_CTRL_REG);
	while (readl(fh_ced->timer.base + OFFSET_TIMER_CUR_VAL) != 0) {
		sync_cnt++;
		if (sync_cnt >= 50) {
			/* typical cnt is 5 when in 1M timer clk */
			/* so here use 50 to check whether it is err */
			pr_err("timer problem,can't disable");
		}
	}
	pr_debug("Setting clock period %lu for HZ %d\n", period, HZ);
	writel(period, fh_ced->timer.base + OFFSET_TIMER_LOADCNT);
	ctrl |= TMR_CONTROL_ENABLE;
	ctrl &= ~TMR_CONTROL_INT;
	writel(ctrl, fh_ced->timer.base + OFFSET_TIMER_CTRL_REG);
	return 0;
}
static int fh_timer_set_oneshot(struct clock_event_device *evt)
{
#if 0
	unsigned long ctrl;
	unsigned long period;
	struct fh_timer_clock_event_device *fh_ced = ced_to_fh_timer_ced(evt);


		ctrl = readl(fh_ced->timer.base + OFFSET_TIMER_CTRL_REG);
		/*
		 * set free running mode, this mode will let timer reload max
		 * timeout which will give time (3min on 25MHz clock) to rearm
		 * the next event, therefore emulate the one-shot mode.
		 */
		ctrl &= ~TMR_CONTROL_ENABLE;
		ctrl &= ~TMR_CONTROL_MODE_PERIODIC;

		writel(ctrl, fh_ced->timer.base + OFFSET_TIMER_CTRL_REG);
		/* write again to set free running mode */
		writel(ctrl, fh_ced->timer.base + OFFSET_TIMER_CTRL_REG);

		/*
		 * DW APB p. 46, load counter with all 1s before starting free
		 * running mode.
		 */
		writel(~0, fh_ced->timer.base + OFFSET_TIMER_LOADCNT);
		ctrl &= ~TMR_CONTROL_INT;
		ctrl |= TMR_CONTROL_ENABLE;
		writel(ctrl, fh_ced->timer.base + OFFSET_TIMER_CTRL_REG);
#endif
	return 0;
}

static int fh_timer_set_oneshot_stopped(struct clock_event_device *evt)
{
	unsigned long ctrl;
	struct fh_timer_clock_event_device *fh_ced = ced_to_fh_timer_ced(evt);

	ctrl = readl(fh_ced->timer.base + OFFSET_TIMER_CTRL_REG);
	ctrl &= ~TMR_CONTROL_ENABLE;
	writel(ctrl, fh_ced->timer.base + OFFSET_TIMER_CTRL_REG);
	return 0;
}

static int fh_timer_set_shutdown(struct clock_event_device *evt)
{
	unsigned long ctrl;
	struct fh_timer_clock_event_device *fh_ced = ced_to_fh_timer_ced(evt);

	ctrl = readl(fh_ced->timer.base + OFFSET_TIMER_CTRL_REG);
	ctrl &= ~TMR_CONTROL_ENABLE;
	writel(ctrl, fh_ced->timer.base + OFFSET_TIMER_CTRL_REG);
	return 0;
}

static int fh_timer_tick_resume(struct clock_event_device *evt)
{
	struct fh_timer_clock_event_device *fh_ced = ced_to_fh_timer_ced(evt);

	fh_timer_enable_int(&fh_ced->timer);
	return 0;
}



static int fh_timer_next_event(unsigned long delta,
			   struct clock_event_device *evt)
{
	unsigned long ctrl;
	int sync_cnt = 0;
	struct fh_timer_clock_event_device *fh_ced = ced_to_fh_timer_ced(evt);

	/* Disable timer */
	ctrl = readl(fh_ced->timer.base + OFFSET_TIMER_CTRL_REG);
	ctrl &= ~TMR_CONTROL_ENABLE;
	writel(ctrl, fh_ced->timer.base + OFFSET_TIMER_CTRL_REG);
	/* zy/ticket/100 : update apb Timer LOADCNT */
	/* CURRENTVALE could,t be start from new LOADCOUNT */
	/* cause is timer clk 1M hz and apb is 150M hz */
	/* keep disable for 3us to ensure timer detect it */
	while (readl(fh_ced->timer.base + OFFSET_TIMER_CUR_VAL) != 0) {
		sync_cnt++;
		if (sync_cnt >= 50) {
			/* typical cnt is 5 when in 1M timer clk */
			/* so here use 50 to check whether it is err */
			pr_err("timer problem,can't disable");
		}
	}
	/* write new count */
	writel(delta, fh_ced->timer.base + OFFSET_TIMER_LOADCNT);
	ctrl |= TMR_CONTROL_ENABLE;
	writel(ctrl, fh_ced->timer.base + OFFSET_TIMER_CTRL_REG);

	return 0;
}


/**
 * fh_timer_clockevent_init() - use an fh timer as a clock_event_device
 *
 * @cpu:	The CPU the events will be targeted at.
 * @name:	The name used for the timer and the IRQ for it.
 * @rating:	The rating to give the timer.
 * @base:	I/O base for the timer registers.
 * @irq:	The interrupt number to use for the timer.
 * @freq:	The frequency that the timer counts at.
 *
 * This creates a clock_event_device for using with the generic clock layer
 * but does not start and register it.  This should be done with
 * fh_timer_clockevent_register() as the next step.  If this is the first time
 * it has been called for a timer then the IRQ will be requested, if not it
 * just be enabled to allow CPU hotplug to avoid repeatedly requesting and
 * releasing the IRQ.
 */
struct fh_timer_clock_event_device *
fh_timer_clockevent_init(int cpu, const char *name, unsigned int rating,
		       void __iomem *base, int irq, unsigned long freq)
{
	struct fh_timer_clock_event_device *fh_ced =
		kzalloc(sizeof(*fh_ced), GFP_KERNEL);
	int err;

	if (!fh_ced)
		return NULL;

	fh_ced->timer.base = base;
	fh_ced->timer.irq = irq;
	fh_ced->timer.freq = freq;
	/* TODO: maxsec == 4 ? */
	clockevents_calc_mult_shift(&fh_ced->ced, freq, 4);
	fh_ced->ced.max_delta_ns = clockevent_delta2ns(0x7fffffff,
						       &fh_ced->ced);
	fh_ced->ced.min_delta_ns = clockevent_delta2ns(0xf, &fh_ced->ced);
	fh_ced->ced.cpumask = cpumask_of(cpu);
	fh_ced->ced.features = CLOCK_EVT_FEAT_PERIODIC | CLOCK_EVT_FEAT_ONESHOT;
	fh_ced->ced.set_state_periodic = fh_timer_set_periodic;
	fh_ced->ced.set_state_oneshot = fh_timer_set_oneshot;
	fh_ced->ced.set_state_oneshot_stopped = fh_timer_set_oneshot_stopped;
	fh_ced->ced.set_state_shutdown = fh_timer_set_shutdown;
	fh_ced->ced.tick_resume = fh_timer_tick_resume;



	fh_ced->ced.set_next_event = fh_timer_next_event;
	fh_ced->ced.irq = fh_ced->timer.irq;
	fh_ced->ced.rating = rating;
	fh_ced->ced.name = name;

	fh_ced->irqaction.name		= fh_ced->ced.name;
	fh_ced->irqaction.handler	= fh_timer_clockevent_irq;
	fh_ced->irqaction.dev_id	= &fh_ced->ced;
	fh_ced->irqaction.irq		= irq;
	fh_ced->irqaction.flags		= IRQF_TIMER | IRQF_IRQPOLL |
						IRQF_NOBALANCING;

	fh_ced->eoi = fh_timer_eoi;

	err = setup_irq(irq, &fh_ced->irqaction);
	if (err) {
		pr_err("failed to request timer irq\n");
		kfree(fh_ced);
		fh_ced = NULL;
	}

	return fh_ced;
}

struct fh_timer_clock_event_device *
fh_timer_bc_clockevent_init(int cpu, const char *name, unsigned int rating,
		       void __iomem *base, int irq, unsigned long freq)
{
	struct fh_timer_clock_event_device *fh_ced =
		kzalloc(sizeof(*fh_ced), GFP_KERNEL);
	int err;

	if (!fh_ced)
		return NULL;

	fh_ced->timer.base = base;
	fh_ced->timer.irq = irq;
	fh_ced->timer.freq = freq;
	/* TODO: maxsec == 4 ? */
	clockevents_calc_mult_shift(&fh_ced->ced, freq, 4);
	fh_ced->ced.max_delta_ns = clockevent_delta2ns(0x7fffffff,
						       &fh_ced->ced);
	fh_ced->ced.min_delta_ns = clockevent_delta2ns(0xf, &fh_ced->ced);
	fh_ced->ced.cpumask = cpu_all_mask;
	fh_ced->ced.features = CLOCK_EVT_FEAT_PERIODIC | CLOCK_EVT_FEAT_ONESHOT;
	fh_ced->ced.set_state_periodic = fh_timer_set_periodic;
	fh_ced->ced.set_state_oneshot = fh_timer_set_oneshot;
	fh_ced->ced.set_state_oneshot_stopped = fh_timer_set_oneshot_stopped;
	fh_ced->ced.set_state_shutdown = fh_timer_set_shutdown;
	fh_ced->ced.tick_resume = fh_timer_tick_resume;



	fh_ced->ced.set_next_event = fh_timer_next_event;
	fh_ced->ced.irq = fh_ced->timer.irq;
	fh_ced->ced.rating = rating;
	fh_ced->ced.name = "fh_bc_timer";

	fh_ced->irqaction.name		= fh_ced->ced.name;
	fh_ced->irqaction.handler	= fh_bc_timer_clockevent_irq;
	fh_ced->irqaction.dev_id	= &fh_ced->ced;
	fh_ced->irqaction.irq		= irq;
	fh_ced->irqaction.flags		= IRQF_TIMER | IRQF_IRQPOLL |
						IRQF_NOBALANCING;



	fh_ced->eoi = fh_timer_eoi;

	err = setup_irq(irq, &fh_ced->irqaction);
	if (err) {
		pr_err("failed to request timer irq\n");
		kfree(fh_ced);
		fh_ced = NULL;
	}

	return fh_ced;
}

static unsigned long fh_read_current_timer(void)
{
	return g_cs->read(g_cs);
}

static struct delay_timer fh_delay_timer = {
	.read_current_timer = fh_read_current_timer,
};

/**
 * fh_timer_clocksource_init() - use an fh timer as a clocksource.
 *
 * @rating:	The rating to give the clocksource.
 * @name:	The name for the clocksource.
 * @base:	The I/O base for the timer registers.
 * @freq:	The frequency that the timer counts at.
 *
 * This creates a clocksource using an APB timer but does not yet register it
 * with the clocksource system.  This should be done with
 * fh_timer_clocksource_register() as the next step.
 */
struct fh_timer_clocksource *
fh_timer_clocksource_init(unsigned int rating, const char *name,
		void __iomem *base, unsigned long freq)
{
	struct fh_timer_clocksource *fh_cs =
			kzalloc(sizeof(*fh_cs), GFP_KERNEL);

	if (!fh_cs)
		return NULL;

	fh_cs->timer.base = base;
	fh_cs->timer.freq = freq;
	fh_cs->cs.name = name;
	fh_cs->cs.rating = rating;
	fh_cs->cs.read = fh_timer_read_clocksource;
	fh_cs->cs.mask = CLOCKSOURCE_MASK(32);
	fh_cs->cs.flags = CLOCK_SOURCE_IS_CONTINUOUS;
	fh_cs->cs.resume = fh_timer_restart_clocksource;

	g_cs = &fh_cs->cs;

	fh_delay_timer.freq = freq;
	register_current_timer_delay(&fh_delay_timer);

	return fh_cs;
}
/**
 * fh_timer_clockevent_register() - register the clock with the generic layer
 *
 * @fh_ced:	The fh clock to register as a clock_event_device.
 */
void fh_timer_clockevent_register(struct fh_timer_clock_event_device *fh_ced)
{
#if defined(CONFIG_SMP)
	int cpu = 0;
	static struct cpumask bcmask;
#endif
	writel(0, fh_ced->timer.base + OFFSET_TIMER_CTRL_REG);
	clockevents_register_device(&fh_ced->ced);
	fh_timer_enable_int(&fh_ced->timer);

#if defined(CONFIG_SMP)
	for_each_cpu(cpu, cpu_all_mask) {
		if (cpu != 0)
			cpumask_set_cpu(cpu, &bcmask);
	}
	/* register broadcast evt */
	fh_bc_tmrevt.name = "fh_broadcast_timer";
	fh_bc_tmrevt.cpumask = &bcmask;
	fh_bc_tmrevt.rating = 300;
	fh_bc_tmrevt.broadcast = tick_broadcast;
	clockevents_register_device(&fh_bc_tmrevt);
#endif
}
void fh_bc_timer_clockevent_register(struct fh_timer_clock_event_device *fh_ced)
{
	writel(0, fh_ced->timer.base + OFFSET_TIMER_CTRL_REG);
	clockevents_register_device(&fh_ced->ced);
	fh_timer_enable_int(&fh_ced->timer);

}


/**
 * fh_timer_clocksource_register() - register the fh clocksource.
 *
 * @fh_cs:	The clocksource to register.
 */
void fh_timer_clocksource_register(struct fh_timer_clocksource *fh_cs)
{
	clocksource_register_hz(&fh_cs->cs, fh_cs->timer.freq);
}

void __init add_clockevent(struct device_node *event_timer,
	unsigned int iovbase, unsigned int irqno)
{
	void __iomem *iobase = (void __iomem *)iovbase;
	struct fh_timer_clock_event_device *ced;
	u32 irq, rate;
	const char *name = "timer0";
	irq = irq_of_parse_and_map(event_timer, 0);
	if (irq == 0) /* no use_of */
		irq = irq_create_mapping(NULL, irqno);

	if (irq == 0)
		panic("No IRQ for clock event timer");

	if (event_timer != NULL)
		name = event_timer->name;

	timer_get_base_and_rate(event_timer, 0, &iobase, &rate);
	ced = fh_timer_clockevent_init(0, name, 300, iobase, irq,
				     rate);
#ifdef CONFIG_FH_SIMPLE_TIMER
	fh_simple_timer_init((unsigned int)iobase, rate);
#endif


	if (!ced)
		panic("Unable to initialise clockevent device");

	fh_timer_clockevent_register(ced);
}

static void clk_event_handle(struct clock_event_device *evt)
{
}

void __init add_bc_clockevent(struct device_node *event_timer,
	unsigned int iovbase, unsigned int irqno)
{
	void __iomem *iobase = (void __iomem *)iovbase;
	struct fh_timer_clock_event_device *ced;
	u32 irq, rate;
	const char *name = "timer0";

	irq = irq_of_parse_and_map(event_timer, 0);
	if (irq == 0) /* no use_of */
		irq = irq_create_mapping(NULL, irqno);


	if (irq == 0)
		panic("No IRQ for clock event timer");

	timer_get_base_and_rate(event_timer, 0, &iobase, &rate);
	if (event_timer != NULL)
		name = event_timer->name;


	ced = fh_timer_bc_clockevent_init(0, name,
					300, iobase, irq, rate);
	ced->ced.event_handler = clk_event_handle;
	if (!ced)
		panic("Unable to initialise clockevent device");

	fh_bc_timer_clockevent_register(ced);
#ifdef CONFIG_ARM_FULLHAN_CPUIDLE
	fh_timer_clock_start(&ced->timer);
	fh_timer_set_periodic(&ced->ced);
#endif
}

static void __iomem *sched_io_base;
static u32 sched_rate;

void __init add_clocksource(struct device_node *source_timer,
	unsigned int iovbase, unsigned int irqno)
{
	void __iomem *iobase = (void __iomem *)iovbase;
	struct fh_timer_clocksource *cs;
	u32 rate;
	const char *name = "timer1";

	if (source_timer != NULL)
		name = source_timer->name;


	timer_get_base_and_rate(source_timer, 1, &iobase, &rate);
	cs = fh_timer_clocksource_init(300, name, iobase, rate);

	if (!cs)
		panic("Unable to initialize clocksource device");

	fh_timer_clocksource_start(cs);
	fh_timer_clocksource_register(cs);

	/*
	 * Fallback to use the clocksource as sched_clock if no separate
	 * timer is found. sched_io_base then points to the current_value
	 * register of the clocksource timer.
	 */
	sched_io_base = iobase + 0x04;

	sched_rate = rate;

}

static u64 notrace read_sched_clock(void)
{
	return ~__raw_readl(sched_io_base);
}

void __init init_sched_clock(void)
{
	sched_clock_register(read_sched_clock, 32, sched_rate);
}

static int num_called;
static int  __init fh_timer_init(struct device_node *timer)
{
	pr_debug("%s-%d\n", __func__, __LINE__);
	switch (num_called) {
#if defined(CONFIG_HAVE_ARM_ARCH_TIMER) && defined(CONFIG_SMP)
	case 0:
		add_bc_clockevent(timer, 0, 0);
	/* CREATE BroadCast Timer Only */
	default:
		break;
	}
#else
	case 0:
		pr_debug("%s: found clocksource timer\n", __func__);
		add_clocksource(timer, 0, 0);
		init_sched_clock();
		break;
	case 1:
		pr_debug("%s: found clockevent timer\n", __func__);
		add_clockevent(timer, 0, 0);
		break;
	default:
		break;
	}
#endif
	num_called++;
	return 0;
}
CLOCKSOURCE_OF_DECLARE(fh_timer, "fh,fh-timer", fh_timer_init);


void  __init fh_timer_init_no_of(unsigned int iovbase,
	unsigned int irqno)
{
	add_clocksource(NULL, iovbase, irqno);
	init_sched_clock();
	add_clockevent(NULL, iovbase, irqno);
}

