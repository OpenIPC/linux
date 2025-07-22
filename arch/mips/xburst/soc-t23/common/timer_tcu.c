/*
 * linux/arch/mips/jz4760/time.c
 *
 * Setting up the clock on the JZ4760 boards.
 *
 * Copyright (C) 2008 Ingenic Semiconductor Inc.
 * Author: <jlwei@ingenic.cn>
 *
 *  This program is free software; you can distribute it and/or modify it
 *  under the terms of the GNU General Public License (Version 2) as
 *  published by the Free Software Foundation.
 *
 *  This program is distributed in the hope it will be useful, but WITHOUT
 *  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 *  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 *  for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  59 Temple Place - Suite 330, Boston MA 02111-1307, USA.
 *
 */

/* #define DEBUG */
/* #define VERBOSE_DEBUG */

#include <linux/types.h>
#include <linux/interrupt.h>
#include <linux/err.h>
#include <linux/time.h>
#include <linux/clockchips.h>
#include <linux/clk.h>
#include <linux/notifier.h>
#include <linux/cpu.h>

#include <soc/base.h>
#include <soc/extal.h>
#include <soc/tcu.h>
#include <soc/irq.h>

#define CLKSOURCE_DIV   16
#define CLKEVENT_DIV    64
#define CLKSOURCE_CH	15
#define CLKEVENT_CH		5
#define CSRDIV(x)      ({int n = 0;int d = x; while(d){ d >>= 2;n++;};(n-1) << 3;})

#define  APB_OST_IOBASE		0x10002000
#define apbost_readl(reg)			inl(APB_OST_IOBASE + reg)
#define apbost_writel(reg,value)	outl(value, APB_OST_IOBASE + reg)

#define tcu_readl(reg)			inl(TCU_IOBASE + reg)
#define tcu_writel(reg,value)	outl(value, TCU_IOBASE + reg)
struct tmr_src {
	struct clocksource cs;
	struct clk *clk_gate;
	unsigned int channel;
};
static cycle_t jz_get_cycles(struct clocksource *cs)
{
	union clycle_type
	{
		cycle_t cycle64;
		unsigned int cycle32[2];
	} cycle;

	do{
		cycle.cycle32[1] = apbost_readl(OST_CNTH);
		cycle.cycle32[0] = apbost_readl(OST_CNTL);
	}while(cycle.cycle32[1] != apbost_readl(OST_CNTH));

	return cycle.cycle64;
}
static int tmr_src_enable(struct clocksource *cs)
{
	struct tmr_src *tmr = container_of(cs,struct tmr_src,cs);
	unsigned int ctrlbit = 1 << tmr->channel;
	tcu_writel(TCU_TSCR, ctrlbit);
	tcu_writel(TCU_TESR, ctrlbit);
	return 0;
}
static 	void tmr_src_disable(struct clocksource *cs)
{
	struct tmr_src *tmr = container_of(cs,struct tmr_src,cs);
	unsigned int ctrlbit = 1 << tmr->channel;
	tcu_writel(TCU_TECR, ctrlbit);
	tcu_writel(TCU_TSSR, ctrlbit);
}
static	void tmr_src_suspend(struct clocksource *cs)
{
	struct tmr_src *tmr = container_of(cs,struct tmr_src,cs);
	if(tmr->clk_gate)
		clk_disable(tmr->clk_gate);
}
static void tmr_src_resume(struct clocksource *cs)
{
	struct tmr_src *tmr = container_of(cs,struct tmr_src,cs);
	if(tmr->clk_gate)
		clk_enable(tmr->clk_gate);
}
static struct tmr_src tmr_src ={
	.cs = {
		.name 		= "jz_clocksource",
		.rating		= 400,
		.read		= jz_get_cycles,
		.mask		= 0x7FFFFFFFFFFFFFFFULL,
		.shift 		= 10,
		.flags		= CLOCK_SOURCE_WATCHDOG | CLOCK_SOURCE_IS_CONTINUOUS,
		.enable         = tmr_src_enable,
		.disable        = tmr_src_disable,
		.suspend        = tmr_src_suspend,
		.resume         = tmr_src_resume,
	}

};

unsigned long long sched_clock(void)
{
	return ((cycle_t)jz_get_cycles(0) * tmr_src.cs.mult) >> tmr_src.cs.shift;
}

void __cpuinit jz_clocksource_init(void)
{
	struct clk *ext_clk = clk_get(NULL,"ext1");
	tmr_src.cs.mult =
		clocksource_hz2mult(clk_get_rate(ext_clk) / CLKSOURCE_DIV, tmr_src.cs.shift);
	clk_put(ext_clk);
	clocksource_register(&tmr_src.cs);
	tmr_src.clk_gate = clk_get(NULL,"tcu");
	if(IS_ERR(tmr_src.clk_gate)) {
		tmr_src.clk_gate = NULL;
		printk("warning: tcu clk get fail!\n");
	}
	if(tmr_src.clk_gate)
		clk_enable(tmr_src.clk_gate);

	tmr_src.channel = CLKSOURCE_CH;
	tcu_writel(TCU_TSCR, 1 << CLKSOURCE_CH);
	apbost_writel(OST_CNTL, 0);
	apbost_writel(OST_CNTH, 0);
	apbost_writel(OST_DR, 0);
	tcu_writel(TCU_TFCR, TFR_OSTF);
	tcu_writel(TCU_TMSR, TMR_OSTM);
	apbost_writel(OST_CSR, OSTCSR_CNT_MD | CSRDIV(CLKSOURCE_DIV) | CSR_EXT_EN); // 16 prescale ext clk
}

struct jz_timerevent {
	int curmode;
	spinlock_t lock;
	unsigned int rate;
	struct clock_event_device clkevt;
	struct irqaction evt_action;
	struct clk *clk_gate;
} jzclockevent;

static inline void stoptimer(void) {
	tcu_writel(TCU_TECR , (1 << CLKEVENT_CH));
	tcu_writel(TCU_TFCR , (1 << CLKEVENT_CH));	//we just use half interrupt
}
static inline void restarttimer(void) {
	tcu_writel(TCU_TFCR , (1 << CLKEVENT_CH));
	tcu_writel(TCU_TESR , (1 << CLKEVENT_CH));
}
static inline void resettimer(int count) {
	tcu_writel(CH_TDFR(CLKEVENT_CH),count);
	tcu_writel(CH_TCNT(CLKEVENT_CH),0);
	tcu_writel(TCU_TMCR , (1 << CLKEVENT_CH));
	tcu_writel(TCU_TFCR , (1 << CLKEVENT_CH));
	tcu_writel(TCU_TESR , (1 << CLKEVENT_CH));
}

static int jz_set_next_event(unsigned long evt,
			     struct clock_event_device *clk_evt_dev)
{
	struct jz_timerevent *evt_dev = container_of(clk_evt_dev,struct jz_timerevent,clkevt);
	unsigned long flags;
	spin_lock_irqsave(&evt_dev->lock,flags);
	if(evt <= 1) {
		WARN_ON(1);
		evt = 2;
	}
	resettimer(evt - 1);
	spin_unlock_irqrestore(&evt_dev->lock,flags);
	return 0;
}

static void jz_set_mode(enum clock_event_mode mode,
			struct clock_event_device *clkevt)
{
	struct jz_timerevent *evt_dev = container_of(clkevt,struct jz_timerevent,clkevt);
	unsigned long flags;
	unsigned int latch = (evt_dev->rate + (HZ >> 1)) / HZ;
	spin_lock_irqsave(&evt_dev->lock,flags);
	switch (mode) {
		case CLOCK_EVT_MODE_PERIODIC:
			if(!clk_is_enabled(evt_dev->clk_gate))
				clk_enable(evt_dev->clk_gate);
			evt_dev->curmode = mode;
			resettimer(latch - 1);
			break;
		case CLOCK_EVT_MODE_ONESHOT:
			evt_dev->curmode = mode;
			break;
		case CLOCK_EVT_MODE_UNUSED:
		case CLOCK_EVT_MODE_SHUTDOWN:
			stoptimer();
			if(evt_dev->clk_gate)
				clk_disable(evt_dev->clk_gate);
			break;

		case CLOCK_EVT_MODE_RESUME:
			if(evt_dev->clk_gate)
				clk_enable(evt_dev->clk_gate);
			restarttimer();
			break;
	}
	spin_unlock_irqrestore(&evt_dev->lock,flags);
}

static irqreturn_t jz_timer_interrupt(int irq, void *dev_id)
{
	struct jz_timerevent *evt_dev = dev_id;
	int ctrlbit = 1 << (CLKEVENT_CH);

	if(tcu_readl(TCU_TFR) & ctrlbit) {
		tcu_writel(TCU_TFCR,ctrlbit);
		if(evt_dev->curmode == CLOCK_EVT_MODE_ONESHOT) {
			stoptimer();
		}
		evt_dev->clkevt.event_handler(&evt_dev->clkevt);
	}

	return IRQ_HANDLED;
}
static void jz_clockevent_init(struct jz_timerevent *evt_dev) {
	struct clock_event_device *cd = &evt_dev->clkevt;
	struct clk *ext_clk = clk_get(NULL,"ext1");

	spin_lock_init(&evt_dev->lock);
	evt_dev->rate = clk_get_rate(ext_clk) / CLKEVENT_DIV;
	clk_put(ext_clk);
	evt_dev->clk_gate = clk_get(NULL,"tcu");
	if(IS_ERR(evt_dev->clk_gate)) {
		evt_dev->clk_gate = NULL;
		printk("warning: tcu clk get fail!\n");
	}
	if(evt_dev->clk_gate)
		clk_enable(evt_dev->clk_gate);
	stoptimer();
	tcu_writel(CH_TCSR(CLKEVENT_CH),CSRDIV(CLKEVENT_DIV) | CSR_EXT_EN);
	evt_dev->evt_action.handler = jz_timer_interrupt;
	evt_dev->evt_action.thread_fn = NULL;
	evt_dev->evt_action.flags = IRQF_DISABLED | IRQF_TIMER;
	evt_dev->evt_action.name = "jz-timerirq";
	evt_dev->evt_action.dev_id = (void*)evt_dev;

	if(setup_irq(IRQ_TCU1, &evt_dev->evt_action) < 0) {
		pr_err("timer request irq error\n");
		BUG();
	}

	memset(cd,0,sizeof(struct clock_event_device));
	cd->name = "jz-clockenvent";
	cd->features = CLOCK_EVT_FEAT_ONESHOT | CLOCK_EVT_FEAT_PERIODIC;
	cd->shift = 10;
	cd->rating = 400;
	cd->set_mode = jz_set_mode;
	cd->set_next_event = jz_set_next_event;
	cd->irq = IRQ_TCU1;
	cd->cpumask = cpumask_of(0);
	clockevents_config_and_register(cd,evt_dev->rate,4,65536);
	printk("clockevents_config_and_register success.\n");
}

void __cpuinit jzcpu_timer_setup(void)
{
	struct jz_timerevent *evt = &jzclockevent;

	tcu_writel(TCU_TSCR,(1 << CLKEVENT_CH));
	tcu_writel(TCU_TMSR,(1 << CLKEVENT_CH)|(1 << (CLKEVENT_CH + 16)));
	tcu_writel(CH_TDHR(CLKEVENT_CH), 0xffff);
	jz_clockevent_init(evt);
}
