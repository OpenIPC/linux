/******************************************************************************
 *    COPYRIGHT (C) 2013 Czyong. Hisilicon
 *    All rights reserved.
 * ***
 *    Create by Czyong 2013-03-12
 *
******************************************************************************/

#include <linux/sched.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/err.h>
#include <linux/clk.h>
#include <linux/clockchips.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/percpu.h>

#include <asm/localtimer.h>

#include <mach/hardware.h>
#include <mach/platform.h>
#include <mach/irqs.h>
#include <asm/hardware/timer-sp.h>
#include <asm/mach/time.h>
#include <asm/sched_clock.h>
#include <asm/hardware/arm_timer.h>
#include <asm/smp_twd.h>

#include <mach/early-debug.h>

/*****************************************************************************/

struct xm_timer_t {
	int init;          /* timer initialize times */
	int index;         /* timer index */
	char *name;
	unsigned int addr; /* timer control base address */
	struct irqaction irq;
	void *priv;
};

#define XM_TIMER(_index, _irq, _addr) { \
	.init = 0, \
	.index = _index, \
	.name = "timer"#_index, \
	.addr = IO_ADDRESS(_addr), \
	.priv = NULL, \
	.irq.irq  = _irq,\
	.irq.name = "timer"#_index, \
	.irq.flags = (IRQF_TIMER | IRQF_NOBALANCING | IRQF_PERCPU), \
}

static struct xm_timer_t xm_timer[] = {
	XM_TIMER(0, INTNR_TIMER_0, TIMER0_REG_BASE),
	XM_TIMER(1, INTNR_TIMER_1, TIMER1_REG_BASE),
	XM_TIMER(2, INTNR_TIMER_2, TIMER2_REG_BASE),
	XM_TIMER(3, INTNR_TIMER_3, TIMER3_REG_BASE),
};

#define TIMER(_index)        (&xm_timer[_index])

//#define GET_SMP_TIMER(_cpu)  (&xm_timer[((_cpu) << 1) + 4])
#define GET_SMP_TIMER(_cpu)  (&xm_timer[(_cpu) + 2])
/*****************************************************************************/

static long __init sp804_get_clock_rate(const char *name)
{
	struct clk *clk;
	long rate;
	int err;

	clk = clk_get_sys("sp804", name);
	if (IS_ERR(clk)) {
		pr_err("sp804: %s clock not found: %d\n", name,
			(int)PTR_ERR(clk));
		return PTR_ERR(clk);
	}

	err = clk_prepare(clk);
	if (err) {
		pr_err("sp804: %s clock failed to prepare: %d\n", name, err);
		clk_put(clk);
		return err;
	}

	err = clk_enable(clk);
	if (err) {
		pr_err("sp804: %s clock failed to enable: %d\n", name, err);
		clk_unprepare(clk);
		clk_put(clk);
		return err;
	}

	rate = clk_get_rate(clk);
	if (rate < 0) {
		pr_err("sp804: %s clock failed to get rate: %ld\n", name, rate);
		clk_disable(clk);
		clk_unprepare(clk);
		clk_put(clk);
	}

	return rate;
}

#if 0

static unsigned long local_timer_rate;

static void sp804_set_mode(enum clock_event_mode mode,
			   struct clock_event_device *evt)
{
	struct xm_timer_t *timer = GET_SMP_TIMER(smp_processor_id());
	unsigned long ctrl = TIMER_CTRL_32BIT | TIMER_CTRL_IE;
	unsigned int clkevt_base = timer->addr;

	writel(ctrl, IOMEM(clkevt_base + TIMER_CTRL));

	switch (mode) {
	case CLOCK_EVT_MODE_PERIODIC:
		writel(DIV_ROUND_CLOSEST(local_timer_rate, HZ),
		       IOMEM(clkevt_base + TIMER_LOAD));
		ctrl |= TIMER_CTRL_PERIODIC | TIMER_CTRL_ENABLE;
		break;

	case CLOCK_EVT_MODE_ONESHOT:
		/* period set, and timer enabled in 'next_event' hook */
		ctrl |= TIMER_CTRL_ONESHOT;
		break;

	case CLOCK_EVT_MODE_UNUSED:
	case CLOCK_EVT_MODE_SHUTDOWN:
	default:
		break;
	}

	writel(ctrl, IOMEM(clkevt_base + TIMER_CTRL));
}

static int sp804_set_next_event(unsigned long next,
				struct clock_event_device *evt)
{
	struct xm_timer_t *timer = GET_SMP_TIMER(smp_processor_id());
	unsigned int clkevt_base = timer->addr;
	unsigned long ctrl = readl(IOMEM(clkevt_base + TIMER_CTRL));

	writel(next, IOMEM(clkevt_base + TIMER_LOAD));
	writel(ctrl | TIMER_CTRL_ENABLE, IOMEM(clkevt_base + TIMER_CTRL));

	return 0;
}

static irqreturn_t sp804_timer_isr(int irq, void *dev_id)
{
	struct xm_timer_t *timer = (struct xm_timer_t *)dev_id;
	unsigned int clkevt_base = timer->addr;
	struct clock_event_device *evt
		= (struct clock_event_device *)timer->priv;

	/* clear the interrupt */
	writel(1, IOMEM(clkevt_base + TIMER_INTCLR));

	evt->event_handler(evt);

	return IRQ_HANDLED;
}
/*****************************************************************************/

/* every should have it's timer irq. */
static int __cpuinit xm530_local_timer_setup(struct clock_event_device *evt)
{
	unsigned int cpu = smp_processor_id();
	struct xm_timer_t *timer = GET_SMP_TIMER(cpu);
	struct irqaction *irq = &timer->irq;



	evt->name = timer->name;
	evt->irq  = irq->irq;
	evt->features = CLOCK_EVT_FEAT_PERIODIC | CLOCK_EVT_FEAT_ONESHOT
		| CLOCK_EVT_FEAT_C3STOP;

	evt->set_mode = sp804_set_mode;
	evt->set_next_event = sp804_set_next_event;
	evt->rating = 350;

	timer->priv = (void *)evt;

	clockevents_config_and_register(evt, local_timer_rate, 0xf, 0xffffffff);
	irq_set_affinity(evt->irq, evt->cpumask);
	enable_irq(evt->irq);


	return 0;
}

static void xm530_local_timer_stop(struct clock_event_device *evt)
{
	evt->set_mode(CLOCK_EVT_MODE_UNUSED, evt);
	disable_irq(evt->irq);
}
/*****************************************************************************/

static struct local_timer_ops xm530_timer_tick_ops __cpuinitdata = {
	.setup	= xm530_local_timer_setup,
	.stop	= xm530_local_timer_stop,
};

static void __init xm530_local_timer_init(void)
{
	unsigned int cpu = 0;
	unsigned int ncores = num_possible_cpus();

	local_timer_rate = sp804_get_clock_rate("sp804");

	for (cpu = 0; cpu < ncores; cpu++) {
		struct xm_timer_t *cpu_timer = GET_SMP_TIMER(cpu);
		cpu_timer->irq.handler = sp804_timer_isr;
		cpu_timer->irq.dev_id = (void *)cpu_timer;
		setup_irq(cpu_timer->irq.irq, &cpu_timer->irq);
		disable_irq(cpu_timer->irq.irq);
	}

	local_timer_register(&xm530_timer_tick_ops);
}
#endif


/*****************************************************************************/
#ifdef CONFIG_LOCAL_TIMERS
DEFINE_TWD_LOCAL_TIMER(twd_localtimer, (resource_size_t)(ARM_INTNL_BASE + REG_A5_PERI_PRI_TIMER_WDT), (resource_size_t)IRQ_LOCALTIMER);
#endif

/*****************************************************************************/

struct xm530_clocksource {
	void __iomem *base;
	struct clocksource clksrc;
};

static struct xm530_clocksource xm530_clocksource = {0};

static inline struct xm530_clocksource
			*to_xm530_clksrc(struct clocksource *cs)
{
	return container_of(cs, struct xm530_clocksource, clksrc);
}

static void xm530_clocksource_start(void __iomem *base)
{
	writel(0, IOMEM(base + TIMER_CTRL));
	writel(0xffffffff, IOMEM(base + TIMER_LOAD));
	writel(0xffffffff, IOMEM(base + TIMER_VALUE));
	writel(TIMER_CTRL_32BIT | TIMER_CTRL_ENABLE | TIMER_CTRL_PERIODIC,
		IOMEM(base + TIMER_CTRL));
}

static cycle_t xm530_clocksource_read(struct clocksource *cs)
{
	return ~readl_relaxed(to_xm530_clksrc(cs)->base + TIMER_VALUE);
}

static notrace u32 xm530_sched_clock_read(void)
{
	return ~readl_relaxed(xm530_clocksource.base + TIMER_VALUE);
}

static void xm530_clocksource_resume(struct clocksource *cs)
{
	xm530_clocksource_start(to_xm530_clksrc(cs)->base);
}

static void __init xm530_clocksource_init(void __iomem *base,
						const char *name)
{
	long rate = sp804_get_clock_rate(name);
	struct clocksource *clksrc = &xm530_clocksource.clksrc;

	if (rate < 0)
		return;

	clksrc->name   = name;
	clksrc->rating = 200;
	clksrc->read   = xm530_clocksource_read;
	clksrc->mask   = CLOCKSOURCE_MASK(32),
	clksrc->flags  = CLOCK_SOURCE_IS_CONTINUOUS,
	clksrc->resume = xm530_clocksource_resume,

	xm530_clocksource.base = base;

	xm530_clocksource_start(base);

	clocksource_register_hz(clksrc, rate);

	setup_sched_clock(xm530_sched_clock_read, 32, rate);
}


void __init xm530_timer_init(void)
{

	/* set the bus clock for all timer */

#ifdef CONFIG_LOCAL_TIMERS
	//xm530_local_timer_init();
	twd_local_timer_register(&twd_localtimer);
#endif

	xm530_clocksource_init((void *)TIMER(0)->addr,
		TIMER(0)->name);

	sp804_clockevents_init((void *)TIMER(1)->addr,
		TIMER(1)->irq.irq, TIMER(1)->name);

}

