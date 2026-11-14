/*
 * AKXX hr_timer driver
 *
 * Copyright (C) 2018 Anyka(Guangzhou) Microelectronics Technology Co., Ltd.
 *
 * Author: Feilong Dong <dong_feilong@anyka.com>
 *         Guohong Ye  <ye_guohong@anyka.com>
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
 */

#define pr_fmt(fmt) "ak-timer: " fmt

#include <linux/bitops.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/clocksource.h>
#include <linux/clockchips.h>

#include <linux/clocksource.h>
#include <linux/interrupt.h>
#include <linux/irqreturn.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/of_platform.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/sched_clock.h>

#include <asm/irq.h>
#include <mach/map.h>
#include "ak_timer_common.h"

int ak_timer_set_timers_irq(int index, int irq);

/* use ak timer as clocksource device */
static cycle_t ak_cs_timer_read(struct clocksource *cs)
{
    u32 ctrl1;
    unsigned long flags;

    local_irq_save(flags);
    ctrl1 = __raw_readl(AK_CS_CTRL1);
    local_irq_restore(flags);

    return (cycle_t)~ctrl1;
}

#ifdef CONFIG_PM

unsigned long suspend_cs_ctrl1, suspend_cs_ctrl2;
static void ak_cs_timer_suspend(struct clocksource *cs)
{
    //unsigned long flags;
    pr_info("enter %s...\n", __func__);
    return;
}

/*
 * @BRIEF        ak_cs_timer_resume
 * @PARAM[in]    *cs
 * @RETURN       void
 */
static void ak_cs_timer_resume(struct clocksource *cs)
{
    pr_info("enter %s...\n", __func__);
    return;
}
#else
#define ak_cs_timer_suspend NULL
#define ak_cs_timer_resume NULL
#endif

static struct clocksource ak_cs = {
    .name = "ak_cs_timer",
    .rating = 150,
    .read = ak_cs_timer_read,
    .mask = CLOCKSOURCE_MASK(32),
    .flags = CLOCK_SOURCE_IS_CONTINUOUS,
    .suspend = ak_cs_timer_suspend,
    .resume = ak_cs_timer_resume,
};

/* use ak timer1 as clock event device */
/*
 * @BRIEF        ak_ce_timer_set_mode
 * @PARAM[in]    mode
 * @PARAM[in]    *evt
 * @RETURN       int
 * @RETVAL       ret
 */
static int ak_ce_timer_set_mode(int mode,
                                struct clock_event_device *evt)
{
    int ret = 0;

    switch (mode)
    {
    case 0: //CLOCK_EVT_MODE_PERIODIC:
        __raw_writel((TIMER_CNT - 1), AK_CE_CTRL1);
        __raw_writel((MODE_AUTO_RELOAD_TIMER | TIMER_ENABLE_BIT |
                      TIMER_FEED_BIT),
                     AK_CE_CTRL2);
        break;

    case 1: //CLOCK_EVT_MODE_ONESHOT:
        __raw_writel(0xffffffff, AK_CE_CTRL1);
        __raw_writel((MODE_ONE_SHOT_TIMER | TIMER_ENABLE_BIT |
                      TIMER_FEED_BIT),
                     AK_CE_CTRL2);
        break;

    default:
        ret = -1;
        break;
    }

    return ret;
}

static int ak_ce_timer_set_periodic(struct clock_event_device *evt)
{
    return ak_ce_timer_set_mode(0, evt);
}

static int ak_ce_timer_set_oneshot(struct clock_event_device *evt)
{
    return ak_ce_timer_set_mode(1, evt);
}

static int ak_ce_timer_set_next_event(unsigned long next,
                                      struct clock_event_device *evt)
{
    __raw_writel(next, AK_CE_CTRL1);
    __raw_writel((TIMER_ENABLE_BIT | MODE_ONE_SHOT_TIMER |
                  TIMER_FEED_BIT),
                 AK_CE_CTRL2);

    return 0;
}

#ifdef CONFIG_PM
unsigned long suspend_ce_ctrl1, suspend_ce_ctrl2;
static void ak_ce_timer_suspend(struct clock_event_device *ce)
{
    //unsigned long flags;
    pr_info("enter %s...\n", __func__);
    return;
}

static void ak_ce_timer_resume(struct clock_event_device *ce)
{
    pr_info("enter %s...\n", __func__);
    return;
}

#else
#define ak_ce_timer_suspend NULL
#define ak_ce_timer_resume NULL
#endif

static struct clock_event_device ak_ced = {
    .name = "ak_ce_timer",
    .features = CLOCK_EVT_FEAT_PERIODIC | CLOCK_EVT_FEAT_ONESHOT,
    .shift = 32,
    .rating = 150,
    //.irq        = IRQ_TIMER,
    .set_next_event = ak_ce_timer_set_next_event,
    .set_state_periodic = ak_ce_timer_set_periodic,
    .set_state_oneshot = ak_ce_timer_set_oneshot,
    .suspend = ak_ce_timer_suspend,
    .resume = ak_ce_timer_resume,
};

/* interrupt handler of ak timer1 */
static irqreturn_t ak_ce_timer_interrupt(int irq, void *handle)
{
    struct clock_event_device *dev = handle;
    u32 ctrl2;

    ctrl2 = __raw_readl(AK_CE_CTRL2);
    if (ctrl2 & TIMER_STATUS_BIT)
    {
        dev->event_handler(dev);
        __raw_writel(ctrl2 | TIMER_CLEAR_BIT, AK_CE_CTRL2);
        return IRQ_HANDLED;
    }

    return IRQ_NONE;
}

static struct irqaction ak_ce_timer_irq = {
    .name = "ak_ce_timer irq",
    .flags = IRQF_TIMER | IRQF_IRQPOLL,
    .handler = ak_ce_timer_interrupt,
    .dev_id = &ak_ced,
};

/* use ak timer7 as sched clock */
static u64 ak_read_sched_clock(void)
{
    u32 ctrl1;
    unsigned long flags;

    local_irq_save(flags);
    ctrl1 = __raw_readl(AK_CS_CTRL1);
    local_irq_restore(flags);

    return ~ctrl1;
}

/*
 * parse_and_map_all_timers - parse timer node and map all timers
 * @node:       pointer to timer node
 *
 * @RETURN: 0-success, -1-fail
 */
static int parse_and_map_all_timers(struct device_node *node)
{
    int i;
    int irq;
    int num_irqs = 0;

#ifdef CONFIG_MACH_AK37D
    num_irqs = 7;
#endif
#if defined(CONFIG_MACH_AK39EV330) || defined(CONFIG_MACH_AK37E) || \
    defined(CONFIG_MACH_AK3918AV100) || defined(CONFIG_MACH_AK3918EV300L)
    num_irqs = 10;
#endif

    for (i = 0; i < num_irqs; i++)
    {
        irq = irq_of_parse_and_map(node, i);
        if (irq <= 0)
        {
            pr_err("%s map irq fail, irq:%d, i:%d\n", __func__, irq, i);
            return -1;
        }
        ak_timer_set_timers_irq(i, irq);
    }

    return 0;
}

static void __init ak_timer_init(struct device_node *node)
{
    int irq;
    pr_err("ak_timer_init\n");

    if (parse_and_map_all_timers(node))
        panic("Can't parse&map all timers");

    irq = irq_of_parse_and_map(node, AK_CE_TIMER_INDEX);
    pr_debug("ak_timer_init, clock_event_index=%d, irq=%d\n",
             AK_CE_TIMER_INDEX, irq);
    if (irq <= 0)
        panic("Can't parse IRQ");

    ak_ced.irq = irq;

    /* clock for timers*/

    /* enable clocksource timer */
    /* ak timer clocksource init */
    __raw_writel(0, AK_CS_CTRL2);
    __raw_writel(0xffffffff, AK_CS_CTRL1);
    __raw_writel((TIMER_ENABLE_BIT | MODE_AUTO_RELOAD_TIMER |
                  TIMER_FEED_BIT | TIMER_READ_SEL_BIT),
                 AK_CS_CTRL2);

    /* register to clocksource framework */
    if (clocksource_register_hz(&ak_cs, TIMER_CLK_INPUT))
        pr_err("ak_sys_timer_init: clocksource_register failed for %s\n",
               ak_cs.name);

    /* register to clock event framework */
    ak_ced.cpumask = cpumask_of(0);
    clockevents_config_and_register(&ak_ced, TIMER_CLK_INPUT,
                                    120000, 0xffffffff);

    if (setup_irq(irq, &ak_ce_timer_irq))
        pr_err("ak_sys_timer_init: irq register failed for %s\n",
               ak_ce_timer_irq.name);

    /* register to 64bit general sched clock framework */
    sched_clock_register(ak_read_sched_clock, 32, TIMER_CLK_INPUT);
}

#ifdef CONFIG_MACH_AK37D
CLOCKSOURCE_OF_DECLARE(ak_hrtimer, "anyka,ak37d-system-timer", ak_timer_init);
#endif

#ifdef CONFIG_MACH_AK39EV330
CLOCKSOURCE_OF_DECLARE(ak_hrtimer, "anyka,ak39ev330-system-timer",
                       ak_timer_init);
#endif

#ifdef CONFIG_MACH_AK37E
CLOCKSOURCE_OF_DECLARE(ak_hrtimer, "anyka,ak37e-system-timer", ak_timer_init);
#endif

#ifdef CONFIG_MACH_AK3918AV100
CLOCKSOURCE_OF_DECLARE(ak_hrtimer, "anyka,ak3918av100-system-timer",
                       ak_timer_init);
#endif

#ifdef CONFIG_MACH_AK3918EV300L
CLOCKSOURCE_OF_DECLARE(ak_hrtimer, "anyka,ak3918ev300l-system-timer",
                       ak_timer_init);
#endif

MODULE_VERSION("1.0.04");
