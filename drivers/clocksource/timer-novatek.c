/**
    NVT clocksource function
    NVT clocksource driver
    @file       timer-novatek.c
    @ingroup
    @note
    Copyright   Novatek Microelectronics Corp. 2021.  All rights reserved.

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2 as
    published by the Free Software Foundation.
*/
#include <linux/interrupt.h>
#include <linux/clockchips.h>
#include <linux/clocksource.h>
#include <linux/irqreturn.h>
#include <linux/sched_clock.h>
#include <linux/slab.h>
#include <linux/clk.h>
#include <linux/of_irq.h>
#include <linux/of_address.h>
#include "timer-of.h"

/* For clock event */
#define MAX_VALUE 0xFFFFFFFF
#define SHIFT       32ULL
#define MAX_HZ 3000000
#define MAX_DIV_BIT 255
#define TIMER_NUM (19)
#define TIMER_DIV_SRC 1
#if  (TIMER_DIV_SRC == 0)
        #define TIMER_DIV_DEF 0xFFFFFF00
#else
        #define TIMER_DIV_DEF 0xFFFF00FF
#endif

#define TIMER_FREE_RUN (0x3 + (TIMER_DIV_SRC * 0x4))
#define TIMER_ONE_SHOT (0x1 + (TIMER_DIV_SRC * 0x4))
#define TIMER_DIV_BASE 0x30
#define TIMER_DIV_VAL (1 << ( 16 + TIMER_DIV_SRC) )
#define TIMER_IRQ_ENABLE 0x00
#define TIMER_IRQ_STS 0x10
#define TIMER_IRQ_BASE 0x20
#define TIMER_IRQ_VAL (1 << TIMER_NUM)
#define TIMER_CTL_BASE (0x10 * TIMER_NUM + 0x100)
#define TIMER_TVAL_BASE (TIMER_CTL_BASE + 0x04)
#define TIMER_VALUE_BASE (TIMER_CTL_BASE + 0x08)
#define TIMER_RLD_BASE (TIMER_CTL_BASE + 0x0C)
#define TIMER_ENABLE 1
#define TIMER_DISABLE 0
#define TIMER_FREQ_SHIFT (TIMER_DIV_SRC * 8)
#define TIMER_UP (0x3 + 0x4 * TIMER_DIV_SRC)
#define TIMER_DOWN (TIMER_UP - 1)

/* For NVT HW timer frame work */

#define MAX_TIMER_NUM 20
void __iomem *nvttmr_base;
spinlock_t nvttmr_lock;
u32 nvttmr_in_use;

EXPORT_SYMBOL(nvttmr_base);
EXPORT_SYMBOL(nvttmr_lock);
EXPORT_SYMBOL(nvttmr_in_use);

static struct timer_of to = {
	.flags = TIMER_OF_BASE,

	.clkevt = {
		.name = "nvt-clkevt",
		.rating = 100,
		.cpumask = cpu_possible_mask,
	},

	.of_irq = {
		.flags = IRQF_SHARED | IRQF_TIMER,
	},
};



/*static u64 notrace nvt_read_sched_clock(void)
{
        u32 sum1;
        u64 ns;

        sum1 = readl(timer_of_base(&to) + timer_value_base);
        ns = (s64)sum1;

        return ns;
}
*/
static void nvt_clkevt_time_stop(struct timer_of *to)
{
	u32 val;
	unsigned long flags;

	spin_lock_irqsave(&nvttmr_lock, flags);

	val = readl(timer_of_base(to) + TIMER_CTL_BASE);
	val |= TIMER_DOWN;
	writel( val, (timer_of_base(to) + TIMER_CTL_BASE));

	spin_unlock_irqrestore(&nvttmr_lock, flags);
}

static void nvt_clkevt_time_setup(struct timer_of *to,
				      unsigned long delay)
{
	unsigned long flags;
	u32 val = (u32)(delay);

	spin_lock_irqsave(&nvttmr_lock, flags);

	writel(val, (timer_of_base(to) + TIMER_TVAL_BASE));

	spin_unlock_irqrestore(&nvttmr_lock, flags);
}

static void nvt_clkevt_time_start(struct timer_of *to,
				      bool periodic)
{
	u32 val;
	unsigned long flags;

	spin_lock_irqsave(&nvttmr_lock, flags);

	val = readl(timer_of_base(to) + TIMER_CTL_BASE);
	if (periodic)
		val |= TIMER_FREE_RUN;
	else
		val |= TIMER_ONE_SHOT;
	writel( val, (timer_of_base(to) + TIMER_CTL_BASE));

	spin_unlock_irqrestore(&nvttmr_lock, flags);
}

static int nvt_clkevt_shutdown(struct clock_event_device *clk)
{
	nvt_clkevt_time_stop(to_timer_of(clk));

	return 0;
}

static int nvt_clkevt_set_periodic(struct clock_event_device *clk)
{
	struct timer_of *to = to_timer_of(clk);

	nvt_clkevt_time_stop(to);
	nvt_clkevt_time_setup(to, to->of_clk.period);
	nvt_clkevt_time_start(to, true);
	return 0;
}

static int nvt_clkevt_set_oneshot(struct clock_event_device *clk)
{
	struct timer_of *to = to_timer_of(clk);

	nvt_clkevt_time_stop(to);
	nvt_clkevt_time_setup(to, to->of_clk.period);
	nvt_clkevt_time_start(to, false);

	return 0;
}

static int nvt_clkevt_next_event(unsigned long event,
				     struct clock_event_device *clk)
{
	struct timer_of *to = to_timer_of(clk);

	nvt_clkevt_time_stop(to);
	nvt_clkevt_time_setup(to, event);
	nvt_clkevt_time_start(to, false);

	return 0;
}

static irqreturn_t nvt_interrupt(int irq, void *dev_id)
{
	struct clock_event_device *clkevt = (struct clock_event_device *)dev_id;
	u32 val;
	unsigned long flags;
	int ret = IRQ_HANDLED;

	spin_lock_irqsave(&nvttmr_lock, flags);
	val = readl(timer_of_base(&to) + TIMER_IRQ_STS);
	if (val & (1 << TIMER_NUM)) {
		val = (1 << TIMER_NUM);
		writel( val, (timer_of_base(&to) + TIMER_IRQ_STS));
		if (clkevt->event_handler != NULL) {
			clkevt->event_handler(clkevt);
		}
	}
	else
		ret = IRQ_NONE; 

	spin_unlock_irqrestore(&nvttmr_lock, flags);

	return ret;
}

static int
__init nvt_setup(struct timer_of *to, u32 freq)
{
	u32 val;
	unsigned long flags;

	u8 freq_bit = (MAX_HZ / freq) - 1;
	if ((MAX_HZ / freq) > MAX_DIV_BIT) {
		pr_err("%s: freq too slow should > %d\n", __func__, MAX_HZ / MAX_DIV_BIT);
		return -EINVAL;
	}

	spin_lock_irqsave(&nvttmr_lock, flags);

	val = MAX_VALUE;
	writel( val, (timer_of_base(to) + TIMER_IRQ_STS));

	val = readl(timer_of_base(to) + TIMER_IRQ_ENABLE);
	val |= MAX_VALUE;
	writel( val, (timer_of_base(to) + TIMER_IRQ_ENABLE));

	val = readl(timer_of_base(to) + TIMER_IRQ_BASE);
	val = TIMER_IRQ_VAL;
	writel( val, (timer_of_base(to) + TIMER_IRQ_BASE));

	val = readl(timer_of_base(to) + TIMER_DIV_BASE);
	val &= TIMER_DIV_DEF;
	val |= ((freq_bit << TIMER_FREQ_SHIFT) + TIMER_DIV_VAL);
	writel( val, (timer_of_base(to) + TIMER_DIV_BASE));

	val = readl(timer_of_base(to) + TIMER_TVAL_BASE);
	val |= MAX_VALUE;
	writel( val, (timer_of_base(to) + TIMER_TVAL_BASE));

	val = readl(timer_of_base(to) + TIMER_RLD_BASE);
	val &= ~(MAX_VALUE);
	writel( val, (timer_of_base(to) + TIMER_RLD_BASE));

	val = readl(timer_of_base(to) + TIMER_CTL_BASE);
	val |= TIMER_UP;
	writel( val, (timer_of_base(to) + TIMER_CTL_BASE));
	spin_unlock_irqrestore(&nvttmr_lock, flags);

	return 0;
}

static int __init nvt_common_init(struct device_node *node)
{
	int ret = 0;
	u32 *pfreq = 0;
	u32 freq;
	u32 *pclksrc, clksrc = 0;
	u32 *pbits, bits = 0;
	struct resource res;

	if (MAX_TIMER_NUM < TIMER_NUM) {
		pr_err("Timer num not support\n");
		return -EINVAL;
	}
	if ((nvttmr_in_use & (1 << TIMER_NUM))) {
		pr_err("Timer num %d already in use\n",TIMER_NUM);
		return -EINVAL;
	} else {
		nvttmr_in_use |= (1 << TIMER_NUM);
	}

	to.of_clk.clk = clk_get_sys(node->full_name,"f0040000.timer");
	if (IS_ERR(to.of_clk.clk)) {
		pr_err("%s:%d clk_get fail\n", __func__, __LINE__);
		return PTR_ERR(to.of_clk.clk);
	}
	if (clk_prepare_enable(to.of_clk.clk) != 0) {
		pr_err("%s:%d enable clk fail\n", __func__, __LINE__);
		return PTR_ERR(to.of_clk.clk);
	}

	to.clkevt.features = CLOCK_EVT_FEAT_PERIODIC | CLOCK_EVT_FEAT_ONESHOT;
	to.clkevt.set_state_shutdown = nvt_clkevt_shutdown;
	to.clkevt.set_state_periodic = nvt_clkevt_set_periodic;
	to.clkevt.set_state_oneshot = nvt_clkevt_set_oneshot;
	to.clkevt.tick_resume = nvt_clkevt_shutdown;
	to.clkevt.set_next_event = nvt_clkevt_next_event;
	to.of_irq.handler = nvt_interrupt;
	to.clkevt.name = of_get_property(node, "compatible", NULL);
	to.of_clk.name = of_get_property(node, "clock-names", NULL);


	//printk("fttmr010_of_init...fttmr010_clkevt.name=%s base=%x, irq=%d, freq=%d\n", fttmr010_clkevt.name, base, irq, freq);

	pfreq = (u32 *)of_get_property(node, "clock-frequency2", NULL);
	freq = __be32_to_cpu(*pfreq);

	pclksrc = (u32 *)of_get_property(node, "clock-source", NULL);
	clksrc = __be32_to_cpu(*pclksrc);

	pbits = (u32 *)of_get_property(node, "bits", NULL);
	bits = __be32_to_cpu(*pbits);

	to.of_clk.rate = freq;
	//clocks_calc_mult_shift(&to.clkevt.mult,&to.clkevt.shift,freq,nsec_per_sec,60);
	//ret = timer_of_init(node, &to);
	//to.flags ^= timer_of_irq; 
	ret = of_address_to_resource(node, 0, &res);
	if (ret) {
		pr_err("%s:%d of_address_to_resource fail\n", __func__, __LINE__);
		goto resource_err;
	}

	if (!request_mem_region(res.start, resource_size(&res),node->full_name)) {
		pr_err("%s:%d request_mem_region fail\n", __func__, __LINE__);
		goto req_mem_err;
	}
	spin_lock_init(&nvttmr_lock);
	ret = timer_of_init(node, &to);
	if (ret)
		goto time_init_err;

	ret = nvt_setup(&to, freq);
	if (ret)
		goto time_setup_err;

	to.flags |= TIMER_OF_CLOCK; 
	to.of_irq.irq = irq_of_parse_and_map(node, to.of_irq.index);
	if (to.of_irq.irq < 0) {
		pr_err("%s:%d irq_of_parse_and_map\n", __func__, __LINE__);
		goto irq_err;
	}
	ret = request_irq(to.of_irq.irq, to.of_irq.handler,
                            to.of_irq.flags ? to.of_irq.flags : IRQF_TIMER,
                            node->full_name, &to.clkevt);
	//ret = timer_of_irq_init(np, &to.of_irq);
	if (ret) {
		pr_err("%s:%d request_irq fail\n", __func__, __LINE__);
		goto irq_err;
	}
	to.flags |= TIMER_OF_IRQ; 
	nvttmr_base = timer_of_base(&to);

	/* configure clock source */
	/*clocksource_mmio_init(timer_of_base(&to) + timer_value_base,
			node->name, timer_of_rate(&to), freq, bits,
			clocksource_mmio_readl_up);
	 */
	//sched_clock_register(nvt_read_sched_clock, 32, timer_of_rate(&to));

	/* configure clock event */
	clockevents_config_and_register(&to.clkevt, timer_of_rate(&to),
				1, 0xffffffff);
	return 0;
irq_err:
time_setup_err:
time_init_err:
	release_mem_region(res.start, (res.end - res.start + 1));
req_mem_err:
resource_err:
	clk_disable(to.of_clk.clk);

	return ret;
}

static __init int nvt_timer_init(struct device_node *np)
{
        return nvt_common_init(np);
}
TIMER_OF_DECLARE(nvt_timer, "nvt,nvt_clk_src", nvt_timer_init);
