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
#include <asm/setup.h>

#include <asm/io.h>
#include <mach/map.h>
#include <linux/delay.h>
#include <linux/spinlock.h>


static spinlock_t counter_lock;

#ifdef CONFIG_AK_PM_TIME_TEST

static void gpio70_on_off(void)
{
    void __iomem *gpio_regs;
    u32 gpio_val;
    gpio_regs = ioremap(0x08000194, 4);
    gpio_val = __raw_readl(gpio_regs);
    gpio_val &=  ~(0 << 24);
    __raw_writel(gpio_val, gpio_regs);
    gpio_val =__raw_readl(AK_VA_SYSCTRL);
    
    /*设置输出*/
    gpio_regs = ioremap(0x20170008, 4);
    gpio_val = __raw_readl(gpio_regs + 0x0);
    __raw_writel(gpio_val | ( 0x1 << 6) , gpio_regs + 0x0);
     gpio_val =__raw_readl(AK_VA_SYSCTRL);


    gpio_regs = ioremap(0x2017001c, 4);
    /*输出1*/
    gpio_val = __raw_readl(gpio_regs + 0x0);
    __raw_writel(gpio_val | (0x1 << 6 ) , gpio_regs + 0x0);
    gpio_val =__raw_readl(AK_VA_SYSCTRL);
    udelay(1);
    /*输出0*/
    gpio_val = __raw_readl(gpio_regs + 0x0);
    __raw_writel(gpio_val &  ~(0x1 << 6 ) , gpio_regs + 0x0);
    gpio_val =__raw_readl(AK_VA_SYSCTRL);
}

#endif


int ak_timer_set_timers_irq(int index, int irq);

/*
 * use ak timer as clocksource device
 */
static cycle_t ak_cs_timer_read(struct clocksource *cs)
{
    u32 ctrl1;
    unsigned long flags;
    //gpio_on_off_on(1);

    local_irq_save(flags);
#if defined(CONFIG_MACH_AK3918AV130) || defined(CONFIG_MACH_KM01A) 
    ctrl1 = __raw_readl(AK_CS_CTRL + RET_TIMER_VALUE);
#else
    ctrl1 = __raw_readl(AK_CS_CTRL1);
#endif
    local_irq_restore(flags);
    //gpio_on_off_on(0);


    return (cycle_t)~ctrl1;
}

#ifdef CONFIG_PM
#if 0
#if  defined(CONFIG_MACH_KM01A)
static u32 suspend_cs_timer_value;
#endif
#endif

/*
 * @BRIEF        ak_cs_timer_suspend
 * @PARAM[in]    *cs
 * @RETURN       void
 */
static void ak_cs_timer_suspend(struct clocksource *cs)
{
        /*gpio计算上电时间*/
#ifdef CONFIG_AK_PM_TIME_TEST
            gpio70_on_off();
#endif
#if 0
    //unsigned long flags;
#if  defined(CONFIG_MACH_KM01A)
    suspend_cs_timer_value = __raw_readl(AK_CS_CTRL + RET_TIMER_VALUE);

#endif
#endif
    pr_info("%s\n",__func__);
    return;
}

/*
 * @BRIEF        ak_cs_timer_resume
 * @PARAM[in]    *cs
 * @RETURN       void
 */
static void ak_cs_timer_resume(struct clocksource *cs)
{
#if  defined(CONFIG_MACH_KM01A)

u32 timer_val;
    /*gpio计算上电时间*/
#ifdef CONFIG_AK_PM_TIME_TEST
        gpio70_on_off();
#endif

    //__raw_writel(0x97b7e0, AK_CS_CTRL + CFG_TIMER_LOAD);
    
    __raw_writel(AK_TIMER_INT_EN, AK_CS_CTRL + CFG_TIMER_INT_EN);
   
   
    timer_val = __raw_readl(AK_CS_CTRL + CFG_TIMER_DIV);
    timer_val &= ~(0xFF);
    timer_val |=  1;
    __raw_writel(timer_val, AK_CS_CTRL + CFG_TIMER_DIV);

    timer_val = __raw_readl(AK_CS_CTRL + CFG_TIMER_CTRL);
    timer_val &= ~(0x3 << 1);
    //timer_val |= (0x1 << 1);//PERIOD模式
    timer_val |= (0x0 << 1);//free模式
    timer_val |= (0x1 << 0);
    __raw_writel(timer_val|AK_UPDATE_RELOAD, AK_CS_CTRL + CFG_TIMER_CTRL);
#endif
        pr_info("%s\n",__func__);
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

static int ak_ce_timer_mode;

/* clock event device */
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
#if defined(CONFIG_MACH_AK3918AV130) || defined(CONFIG_MACH_KM01A) 
    switch (mode)
    {
        case 0: //CLOCK_EVT_MODE_PERIODIC:
            __raw_writel((TIMER_CNT - 1), AK_CE_CTRL + CFG_TIMER_LOAD);
            __raw_writel(AK_TIMER_INT_EN, AK_CE_CTRL + CFG_TIMER_INT_EN);
            __raw_writel((MODE_PERIOD_TIMER | ENABLE_TIMER |AK_UPDATE_RELOAD),
                    AK_CE_CTRL + CFG_TIMER_CTRL);
            break;

        case 1: //CLOCK_EVT_MODE_ONESHOT:
            __raw_writel(0xffffffff, AK_CE_CTRL + CFG_TIMER_LOAD);
            __raw_writel(AK_TIMER_INT_EN, AK_CE_CTRL + CFG_TIMER_INT_EN);
            __raw_writel((MODE_ONE_SHOT_TIMER | ENABLE_TIMER |AK_UPDATE_RELOAD),
                    AK_CE_CTRL + CFG_TIMER_CTRL);
            break;

        default:
            ret = -1;
            break;
    }
#else
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
#endif
    return ret;
}

/*
 * ak_ce_timer_set_periodic
 */
static int ak_ce_timer_set_periodic(struct clock_event_device *evt)
{
    ak_ce_timer_mode = 0;
    return ak_ce_timer_set_mode(0, evt);
}

/*
 * ak_ce_timer_set_oneshot
 */
static int ak_ce_timer_set_oneshot(struct clock_event_device *evt)
{
    ak_ce_timer_mode = 1;
    return ak_ce_timer_set_mode(1, evt);
}

/*
 * ak_ce_timer_set_next_event
 */
static int ak_ce_timer_set_next_event(unsigned long next,
        struct clock_event_device *evt)
{
#if defined(CONFIG_MACH_AK3918AV130) || defined(CONFIG_MACH_KM01A) 
    __raw_writel(next, AK_CE_CTRL + CFG_TIMER_LOAD);
    __raw_writel((ENABLE_TIMER | MODE_ONE_SHOT_TIMER | AK_UPDATE_RELOAD),
            AK_CE_CTRL + CFG_TIMER_CTRL);
#else
    __raw_writel(next, AK_CE_CTRL1);
    __raw_writel((TIMER_ENABLE_BIT | MODE_ONE_SHOT_TIMER |
                TIMER_FEED_BIT),
            AK_CE_CTRL2);
#endif
    return 0;
}

#ifdef CONFIG_PM
#if defined(CONFIG_MACH_KM01A) 
static unsigned long suspend_ce_timer_value;
#endif

/*
 * ak_ce_timer_suspend
 */
static void ak_ce_timer_suspend(struct clock_event_device *ce)
{
    //unsigned long flags;
#if defined(CONFIG_MACH_KM01A) 
    suspend_ce_timer_value = __raw_readl(AK_CE_CTRL + RET_TIMER_VALUE);
    pr_info("enter %s timer value %ld...\n", __func__, suspend_ce_timer_value);
#endif
    return;
}

/*
 * ak_ce_timer_resume
 */
static void ak_ce_timer_resume(struct clock_event_device *ce)
{
#if defined(CONFIG_MACH_KM01A) 
    u32 reg_val;

    reg_val = __raw_readl(AK_CE_CTRL + CFG_TIMER_DIV);
    reg_val &= ~(0xFF);
    reg_val |=  1;
    __raw_writel(reg_val, AK_CE_CTRL + CFG_TIMER_DIV);
    __raw_writel(1, AK_CE_CTRL + CFG_TIMER_INTCLR);
    ak_ce_timer_set_mode(ak_ce_timer_mode, NULL);
    if (ak_ce_timer_mode == 1) {
        __raw_writel(suspend_ce_timer_value, AK_CE_CTRL + CFG_TIMER_LOAD);
        __raw_writel((ENABLE_TIMER | MODE_ONE_SHOT_TIMER | AK_UPDATE_RELOAD),
                AK_CE_CTRL + CFG_TIMER_CTRL);
    }
#endif
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

/*
 * interrupt handler of ak timer1
 */
 
static irqreturn_t ak_ce_timer_interrupt(int irq, void *handle)
{
    struct clock_event_device *dev = handle;
    //unsigned long flags;
    u32 ctrl2;
    //static unsigned long num = 0;

    
#if defined(CONFIG_MACH_AK3918AV130) || defined(CONFIG_MACH_KM01A)      


    ctrl2 = __raw_readl(AK_CE_CTRL + RET_TIMER_INT_STA);
    if (ctrl2 & TIMER_INT_STA_BIT) 
    {
#if 0
        num += 1;
        if ((num % 10) == 0)
        {
             gpio_on_off_on(1);
             gpio_on_off_on(0);
        }
#endif
         dev->event_handler(dev);
        __raw_writel(TIMER_INT_STA_BIT, AK_CE_CTRL + CFG_TIMER_INTCLR);//clear interrupt
        return IRQ_HANDLED;
    }
     
#else
    ctrl2 = __raw_readl(AK_CE_CTRL2);
    if (ctrl2 & TIMER_STATUS_BIT)
    {
        dev->event_handler(dev);
        __raw_writel(ctrl2 | TIMER_CLEAR_BIT, AK_CE_CTRL2);
        return IRQ_HANDLED;
    }
#endif
  

    return IRQ_NONE;
}

static struct irqaction ak_ce_timer_irq = {
    .name = "ak_ce_timer irq",
    .flags = IRQF_IRQPOLL,
    .handler = ak_ce_timer_interrupt,
    .dev_id = &ak_ced,
};

/*
 * ak_read_sched_clock
 */
static u64 ak_read_sched_clock(void)
{
    u32 ctrl1;
    unsigned long flags;

    local_irq_save(flags);
#if defined(CONFIG_MACH_AK3918AV130) || defined(CONFIG_MACH_KM01A) 
    ctrl1 = __raw_readl(AK_CS_CTRL + RET_TIMER_VALUE);
#else
    ctrl1 = __raw_readl(AK_CS_CTRL1);
#endif
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

#if defined(CONFIG_MACH_AK3918AV130) || defined(CONFIG_MACH_KM01A) 
    num_irqs = 5;
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

/*
 * timer init
 */
static void __init ak_timer_init(struct device_node *node)
{
    int irq;
#if defined(CONFIG_MACH_AK3918AV130) || defined(CONFIG_MACH_KM01A)
    u32 timer_val;
#endif

#ifdef CONFIG_AK_PM_TIME_TEST
           printk("[ ak_timer_init:0x%08x]\n",__raw_readl(AK_CE_CTRL + RET_TIMER_VALUE));
           //上电时间~内核time初始化期间
            gpio70_on_off();
#endif


    if (parse_and_map_all_timers(node))
        panic("Can't parse&map all timers");

    irq = irq_of_parse_and_map(node, AK_CE_TIMER_INDEX);
    if (irq <= 0)
        panic("Can't parse IRQ");

    ak_ced.irq = irq;

    // 初始化自旋锁
    spin_lock_init(&counter_lock);

#if defined(CONFIG_MACH_AK3918AV130) || defined(CONFIG_MACH_KM01A) 


    __raw_writel(AK_TIMER_INTCLR,  AK_CS_CTRL+CFG_TIMER_INTCLR);
    __raw_writel(AK_TIMER_INTCLR,  AK_CE_CTRL+CFG_TIMER_INTCLR);
    __raw_writel(timer_val & ~ENABLE_TIMER,   AK_CS_CTRL+CFG_TIMER_CTRL);
    __raw_writel(timer_val & ~ENABLE_TIMER,   AK_CE_CTRL+CFG_TIMER_CTRL);
  
    #if 0
   __raw_writel(0x97b7e0, AK_CS_CTRL + CFG_TIMER_LOAD);
    #else
    __raw_writel(0xffffffff, AK_CS_CTRL + CFG_TIMER_LOAD);
    #endif
    __raw_writel(AK_TIMER_INT_EN, AK_CS_CTRL + CFG_TIMER_INT_EN);
   
   
    timer_val = __raw_readl(AK_CS_CTRL + CFG_TIMER_DIV);
    timer_val &= ~(0xFF);
    timer_val |=  1;
    __raw_writel(timer_val, AK_CS_CTRL + CFG_TIMER_DIV);

    timer_val = __raw_readl(AK_CS_CTRL + CFG_TIMER_CTRL);
    timer_val &= ~(0x3 << 1);
    timer_val |= (0x1 << 1);
    timer_val |= (0x1 << 0);
    __raw_writel(timer_val, AK_CS_CTRL + CFG_TIMER_CTRL);
    
    timer_val = __raw_readl(AK_CE_CTRL + CFG_TIMER_DIV);
    timer_val &= ~(0xFF);
    timer_val |=  1;
    __raw_writel(timer_val, AK_CE_CTRL + CFG_TIMER_DIV);

#else
    /* enable clocksource timer */
    /* ak timer clocksource init */
    __raw_writel(0, AK_CS_CTRL2);
    __raw_writel(0xffffffff, AK_CS_CTRL1);
    __raw_writel((TIMER_ENABLE_BIT | MODE_AUTO_RELOAD_TIMER |
                TIMER_FEED_BIT | TIMER_READ_SEL_BIT),
            AK_CS_CTRL2);
#endif
    /* register to clocksource framework */
    if (clocksource_register_hz(&ak_cs, TIMER_CLK_INPUT))
        pr_err("ak_sys_timer_init: clocksource_register failed for %s\n",
                ak_cs.name);

    /* register to clock event framework */
    ak_ced.cpumask = cpumask_of(0);
    clockevents_config_and_register(&ak_ced, TIMER_CLK_INPUT, 120000, 0xffffffff);

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

#ifdef CONFIG_MACH_AK3918AV130
CLOCKSOURCE_OF_DECLARE(ak_hrtimer, "anyka,ak3918av130-system-timer",
        ak_timer_init);
#endif

#ifdef CONFIG_MACH_KM01A
CLOCKSOURCE_OF_DECLARE(ak_hrtimer, "anyka,km01a-system-timer",
        ak_timer_init);
#endif
MODULE_VERSION("2.0.00");
