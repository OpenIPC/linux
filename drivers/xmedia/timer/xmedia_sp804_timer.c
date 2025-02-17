/*
 *  Copyright (c) XMEDIA. All rights reserved.
 */

#define pr_fmt(fmt) "xmsp804: " fmt

#include <linux/clk.h>
#include <linux/clocksource.h>
#include <linux/clockchips.h>
#include <linux/err.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/cpu.h>
#include <linux/io.h>
#include <linux/smp.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/sched_clock.h>

#define TIMER_LOAD      0x00             /* ACVR rw */
#define TIMER_VALUE     0x04             /* ACVR ro */
#define TIMER_CTRL      0x08             /* ACVR rw */
#define TIMER_CTRL_ONESHOT      (1 << 0) /*  CVR */
#define TIMER_CTRL_32BIT        (1 << 1) /*  CVR */
#define TIMER_CTRL_DIV1         (0 << 2) /* ACVR */
#define TIMER_CTRL_DIV16        (1 << 2) /* ACVR */
#define TIMER_CTRL_DIV256       (2 << 2) /* ACVR */
#define TIMER_CTRL_IE           (1 << 5) /*   VR */
#define TIMER_CTRL_PERIODIC     (1 << 6) /* ACVR */
#define TIMER_CTRL_ENABLE       (1 << 7) /* ACVR */

#define TIMER_INTCLR    0x0c             /* ACVR wo */
#define TIMER_RIS       0x10             /*  CVR ro */
#define TIMER_MIS       0x14             /*  CVR ro */
#define TIMER_BGLOAD    0x18             /*  CVR rw */

struct xmsp804_clocksource {
	void __iomem *base;
	struct clocksource clksrc;
};

#define to_xmclksrc(e) \
	container_of(e, struct xmsp804_clocksource, clksrc)

static struct xmsp804_clocksource g_xmsp804_clksrc;
static void __iomem *g_xmsp804_sched_clock_base = NULL;

struct ext_sp804_clockevent_device {
	struct clock_event_device clkevt;
	struct irqaction action;
	void __iomem *base;
	unsigned long rate;
	unsigned long reload;
	char name[16]; /* 16 - array size */
};

#define to_xmclkevt(e) \
	container_of(e, struct ext_sp804_clockevent_device, clkevt)

static struct ext_sp804_clockevent_device __percpu *g_xmsp804_clkevt = NULL;

static void xmsp804_clocksource_enable(void __iomem *base)
{
	writel(0, base + TIMER_CTRL);
	writel(0xffffffff, base + TIMER_LOAD);
	writel(0xffffffff, base + TIMER_VALUE);
	writel(TIMER_CTRL_32BIT | TIMER_CTRL_ENABLE | TIMER_CTRL_PERIODIC, base + TIMER_CTRL);
}

static void xmsp804_clocksource_resume(struct clocksource *cs)
{
	xmsp804_clocksource_enable(to_xmclksrc(cs)->base);
}

static u64 notrace xmsp804_sched_clock_read(void)
{
	return ~readl_relaxed(g_xmsp804_sched_clock_base + TIMER_VALUE);
}

static u64 xmsp804_clocksource_read(struct clocksource *cs)
{
	return ~(u64)readl_relaxed(to_xmclksrc(cs)->base + TIMER_VALUE);
}

static void __init xmsp804_clocksource_init(void __iomem *base, unsigned long rate)
{
	g_xmsp804_clksrc.base = base;
	g_xmsp804_clksrc.clksrc.name = "xmsp804";
	g_xmsp804_clksrc.clksrc.rating = 499;
	g_xmsp804_clksrc.clksrc.read = xmsp804_clocksource_read;
	g_xmsp804_clksrc.clksrc.resume = xmsp804_clocksource_resume;
	g_xmsp804_clksrc.clksrc.mask = CLOCKSOURCE_MASK(32);
	g_xmsp804_clksrc.clksrc.flags = CLOCK_SOURCE_IS_CONTINUOUS;

	xmsp804_clocksource_enable(base);
	clocksource_register_hz(&g_xmsp804_clksrc.clksrc, rate);
	g_xmsp804_sched_clock_base = base;
	sched_clock_register(xmsp804_sched_clock_read, 32, rate);
}

static int xmsp804_clockevent_shutdown(struct clock_event_device *clkevt)
{
	struct ext_sp804_clockevent_device *xmclkevt = to_xmclkevt(clkevt);

	writel(0, xmclkevt->base + TIMER_CTRL);

	return 0;
}

static int xmsp804_clockevent_set_next_event(unsigned long next, struct clock_event_device *clkevt)
{
	unsigned long ctrl;
	struct ext_sp804_clockevent_device *xmclkevt = to_xmclkevt(clkevt);

	writel(TIMER_CTRL_32BIT, xmclkevt->base + TIMER_CTRL);
	writel(next, xmclkevt->base + TIMER_LOAD);
	writel(next, xmclkevt->base + TIMER_LOAD);
	ctrl = TIMER_CTRL_32BIT |
		   TIMER_CTRL_IE |
		   TIMER_CTRL_ONESHOT |
		   TIMER_CTRL_ENABLE;
	writel(ctrl, xmclkevt->base + TIMER_CTRL);

	return 0;
}

static int sp804_clockevent_set_periodic(struct clock_event_device *clkevt)
{
	unsigned long ctrl;
	struct ext_sp804_clockevent_device *xmclkevt = to_xmclkevt(clkevt);

	writel(TIMER_CTRL_32BIT, xmclkevt->base + TIMER_CTRL);
	writel(xmclkevt->reload, xmclkevt->base + TIMER_LOAD);
	writel(xmclkevt->reload, xmclkevt->base + TIMER_LOAD);
	ctrl = TIMER_CTRL_32BIT |
		   TIMER_CTRL_IE |
		   TIMER_CTRL_PERIODIC |
		   TIMER_CTRL_ENABLE;
	writel(ctrl, xmclkevt->base + TIMER_CTRL);

	return 0;
}

static irqreturn_t xmsp804_clockevent_timer_interrupt(int irq, void *dev_id)
{
	struct clock_event_device *clkevt = dev_id;
	struct ext_sp804_clockevent_device *xmclkevt = to_xmclkevt(clkevt);

	/* clear the interrupt */
	writel(1, xmclkevt->base + TIMER_INTCLR);
	clkevt->event_handler(clkevt);
	return IRQ_HANDLED;
}

static int xmsp804_clockevent_setup(struct ext_sp804_clockevent_device *xmclkevt)
{
	struct clock_event_device *clkevt = &xmclkevt->clkevt;

	writel(0, xmclkevt->base + TIMER_CTRL);
	WARN_ON(setup_irq(clkevt->irq, &xmclkevt->action));
	irq_force_affinity(clkevt->irq, clkevt->cpumask);
	clockevents_config_and_register(clkevt, xmclkevt->rate, 0xf, 0x7fffffff);

	return 0;
}

static int xmsp804_timer_starting_cpu(unsigned int cpu)
{
	xmsp804_clockevent_setup(this_cpu_ptr(g_xmsp804_clkevt));

	return 0;
}
static int xmsp804_timer_dying_cpu(unsigned int cpu)
{
	struct clock_event_device *clkevt = &(per_cpu_ptr(g_xmsp804_clkevt, cpu)->clkevt);
	struct ext_sp804_clockevent_device *xmclkevt = to_xmclkevt(clkevt);

	pr_info("disable IRQ%d cpu #%d\n", clkevt->irq, smp_processor_id());
	disable_irq(clkevt->irq);
	remove_irq(clkevt->irq, &xmclkevt->action);
	clkevt->set_state_shutdown(clkevt);

	return 0;
}

static void __init clockevent_init(struct ext_sp804_clockevent_device *xmclkevt,
	    void __iomem *base, int irq, int cpu, unsigned long rate, unsigned long reload)
{
	struct irqaction *action = NULL;
	struct clock_event_device *clkevt = NULL;

	xmclkevt->base = base;
	xmclkevt->rate = rate;
	xmclkevt->reload = reload;
	if (snprintf(xmclkevt->name, sizeof(xmclkevt->name) - 1, "clockevent %d", cpu) == -1) {
		pr_err("Failed to snprintf.\n");
		return ;
	}

	clkevt = &xmclkevt->clkevt;

	clkevt->name = xmclkevt->name;
	clkevt->cpumask = cpumask_of(cpu);
	clkevt->irq = irq;
	clkevt->set_next_event = xmsp804_clockevent_set_next_event;
	clkevt->set_state_shutdown = xmsp804_clockevent_shutdown;
	clkevt->set_state_periodic = sp804_clockevent_set_periodic;
	clkevt->features = CLOCK_EVT_FEAT_PERIODIC |
			   CLOCK_EVT_FEAT_ONESHOT |
			   CLOCK_EVT_FEAT_DYNIRQ;
	clkevt->rating = 0x400;

	action = &xmclkevt->action;

	action->name = xmclkevt->name;
	action->dev_id = xmclkevt;
	action->irq = irq;
	action->flags = IRQF_TIMER | IRQF_NOBALANCING;
	action->handler = xmsp804_clockevent_timer_interrupt;
}

static int __init xmsp804_timer_init(struct device_node *node)
{
	int ret, irq, ix, nr_cpus;
	struct clk *clk = NULL;
	void __iomem *base = NULL;
	unsigned long rate, reload;

	g_xmsp804_clkevt = alloc_percpu(struct ext_sp804_clockevent_device);
	if (!g_xmsp804_clkevt) {
		ret = -ENOMEM;
		pr_err("can't alloc memory.\n");
		goto out;
	}

	clk = of_clk_get(node, 0);
	if (IS_ERR(clk)) {
		goto out_free;
	}

	clk_prepare_enable(clk);
	rate = clk_get_rate(clk);
	reload = DIV_ROUND_CLOSEST(rate, HZ);
	nr_cpus = of_irq_count(node);
	if (nr_cpus > num_possible_cpus()) {
		nr_cpus = num_possible_cpus();
	}

	for (ix = 0; ix < nr_cpus; ix++) {
		irq = irq_of_parse_and_map(node, ix);
		base = of_iomap(node, ix + 1);
		if (base == NULL) {
			pr_err("can't iomap timer %d\n", ix);
			while (--ix >= 0) {
				iounmap(per_cpu_ptr(g_xmsp804_clkevt, ix)->base);
			}
			goto out_free;
		}

		clockevent_init(per_cpu_ptr(g_xmsp804_clkevt, ix), base, irq, ix, rate, reload);
	}

	base = of_iomap(node, 0);
	if (base == NULL) {
		pr_err("can't iomap timer %d\n", 0);
		goto out_unmap;
	}

	xmsp804_clocksource_init(base, rate);

	/* Install and invoke hotplug callbacks */
	return cpuhp_setup_state(CPUHP_AP_HLSP_TIMER_STARTING,
							"clockevent:xmsp804",
							xmsp804_timer_starting_cpu,
							xmsp804_timer_dying_cpu);

	return 0;

out_unmap:
	for (ix = 0; ix < nr_irqs; ix++) {
		iounmap(per_cpu_ptr(g_xmsp804_clkevt, ix)->base);
	}
out_free:
	free_percpu(g_xmsp804_clkevt);
out:
	return -ENXIO;
}
CLOCKSOURCE_OF_DECLARE(xmsp804, "xmedia,sp804", xmsp804_timer_init);
