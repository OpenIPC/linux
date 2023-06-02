/*
 * arch/arm/mach-gk/timer.c
 *
 * Author: Steven Yu, <yulindeng@gokemicro.com>
 * Copyright (C) 2012-2015, goke, Inc.
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 * Default clock is from APB.
 * Timer 3 is used as clock_event_device.
 * Timer 2 is used as free-running clocksource
 *         if CONFIG_GK_SUPPORT_CLOCKSOURCE is defined
 * Timer 1 is not used.
 */

#include <linux/interrupt.h>
#include <linux/clockchips.h>
#include <linux/delay.h>

#include <asm/mach/irq.h>
#include <asm/mach/time.h>
#include <asm/localtimer.h>
#include <asm/sched_clock.h>

#include <mach/hardware.h>
#include <mach/timer.h>
#include <mach/io.h>
#include <mach/uart.h>
#include <mach/rct.h>
#include <mach/gpio.h>


/* ==========================================================================*/
#define GK_TIMER_FREQ               (get_apb_bus_freq_hz())
#define GK_TIMER_RATING             (300)

static struct clock_event_device        gk_clkevt;
static struct irqaction                 gk_ce_timer_irq;
static u32 timer_reload = 0;
static u32 timer_Mhz = 69;

/* ==========================================================================*/
#define GK_CE_TIMER_STATUS_REG      TIMER3_STATUS_REG
#define GK_CE_TIMER_RELOAD_REG      TIMER3_RELOAD_REG
#define GK_CE_TIMER_MATCH1_REG      TIMER3_MATCH1_REG
#define GK_CE_TIMER_MATCH2_REG      TIMER3_MATCH2_REG
#define GK_CE_TIMER_IRQ             TIMER3_IRQ
#define GK_CE_TIMER_CTR_EN          TIMER_CTR_EN3
#define GK_CE_TIMER_CTR_OF          TIMER_CTR_OF3
#define GK_CE_TIMER_CTR_CSL         TIMER_CTR_CSL3
#define GK_CE_TIMER_CTR_MASK        0x00000F00

int gk_start_highres_timer(u32 ms_to_timeout);

int gk_init_highres_timer(void *pcallback);



static inline void gk_ce_timer_disable(void)
{
    gk_timer_clrbitsl(TIMER_CTR_REG, GK_CE_TIMER_CTR_EN);
}

static inline void gk_ce_timer_enable(void)
{
    gk_timer_setbitsl(TIMER_CTR_REG, GK_CE_TIMER_CTR_EN);
}

static inline void gk_ce_timer_misc(void)
{
    gk_timer_setbitsl(TIMER_CTR_REG, (GK_CE_TIMER_CTR_OF | GK_CE_TIMER_CTR_CSL));
    gk_timer_clrbitsl(TIMER_CTR_REG, GK_CE_TIMER_CTR_CSL);
}

static inline void gk_ce_timer_set_periodic(void)
{
    u32 cnt;

    cnt = GK_TIMER_FREQ / HZ;
    gk_timer_writel(GK_CE_TIMER_STATUS_REG, cnt);
    gk_timer_writel(GK_CE_TIMER_RELOAD_REG, cnt);
    timer_reload = cnt;
    timer_Mhz = GK_TIMER_FREQ/1000000;
    gk_timer_writel(GK_CE_TIMER_MATCH1_REG, 0x0);
    gk_timer_writel(GK_CE_TIMER_MATCH2_REG, 0x0);
    gk_ce_timer_misc();
}

static inline void gk_ce_timer_set_oneshot(void)
{
    gk_timer_writel(GK_CE_TIMER_STATUS_REG, 0x0);
    gk_timer_writel(GK_CE_TIMER_RELOAD_REG, 0xffffffff);
    gk_timer_writel(GK_CE_TIMER_MATCH1_REG, 0x0);
    gk_timer_writel(GK_CE_TIMER_MATCH2_REG, 0x0);
    gk_ce_timer_misc();
}

static void gk_ce_timer_set_mode(enum clock_event_mode mode,
    struct clock_event_device *evt)
{
    switch (mode) {
    case CLOCK_EVT_MODE_PERIODIC:
        gk_ce_timer_disable();
        gk_ce_timer_set_periodic();
        gk_ce_timer_enable();
        break;
    case CLOCK_EVT_MODE_ONESHOT:
        gk_ce_timer_disable();
        gk_ce_timer_set_oneshot();
        gk_ce_timer_enable();
        break;
    case CLOCK_EVT_MODE_UNUSED:
        remove_irq(gk_clkevt.irq, &gk_ce_timer_irq);
    case CLOCK_EVT_MODE_SHUTDOWN:
        gk_ce_timer_disable();
        break;
    case CLOCK_EVT_MODE_RESUME:
        break;
    }
    pr_debug("%s:%d\n", __func__, mode);
}

static int gk_ce_timer_set_next_event(unsigned long delta,
    struct clock_event_device *dev)
{
    gk_timer_writel(GK_CE_TIMER_STATUS_REG, delta);

    return 0;
}

static void gk_timer_irq_clr(u32 timer_irq)
{
    if((timer_irq > TIMER3_IRQ)&&(timer_irq < TIMER1_IRQ))
    {
        printk("Error Timer Irq = %d!\n", timer_irq);
    }

    gk_irq_writel(VIC_EDGE_CLR_REG, (1 << timer_irq));
}

static struct clock_event_device gk_clkevt = {
    .name       = "gk-clkevt",
    .features   = CLOCK_EVT_FEAT_PERIODIC | CLOCK_EVT_FEAT_ONESHOT,
    .rating     = GK_TIMER_RATING,
    .set_next_event = gk_ce_timer_set_next_event,
    .set_mode   = gk_ce_timer_set_mode,
    .mode       = CLOCK_EVT_MODE_UNUSED,
    .irq        = GK_CE_TIMER_IRQ,
};

static irqreturn_t gk_ce_timer_interrupt(int irq, void *dev_id)
{
    gk_timer_irq_clr(irq);
    gk_clkevt.event_handler(&gk_clkevt);
    return IRQ_HANDLED;
}

static struct irqaction gk_ce_timer_irq = {
    .name       = "tick-timer",
    .flags      = IRQF_DISABLED | IRQF_TIMER | IRQF_TRIGGER_RISING,
    .handler    = gk_ce_timer_interrupt,
};

/* ==========================================================================*/
#if defined(CONFIG_SUPPORT_CLOCKSOURCE)
#define GK_CS_TIMER_STATUS_REG      TIMER2_STATUS_REG
#define GK_CS_TIMER_RELOAD_REG      TIMER2_RELOAD_REG
#define GK_CS_TIMER_MATCH1_REG      TIMER2_MATCH1_REG
#define GK_CS_TIMER_MATCH2_REG      TIMER2_MATCH2_REG
#define GK_CS_TIMER_CTR_EN          TIMER_CTR_EN2
#define GK_CS_TIMER_CTR_OF          TIMER_CTR_OF2
#define GK_CS_TIMER_CTR_CSL         TIMER_CTR_CSL2
#define GK_CS_TIMER_CTR_MASK        0x000000F0

static inline void gk_cs_timer_init(void)
{
    gk_timer_clrbitsl(TIMER_CTR_REG, GK_CS_TIMER_CTR_EN);
    gk_timer_clrbitsl(TIMER_CTR_REG, GK_CS_TIMER_CTR_OF);
    gk_timer_clrbitsl(TIMER_CTR_REG, GK_CS_TIMER_CTR_CSL);
    gk_timer_writel(GK_CS_TIMER_STATUS_REG, 0xffffffff);
    gk_timer_writel(GK_CS_TIMER_RELOAD_REG, 0xffffffff);
    gk_timer_writel(GK_CS_TIMER_MATCH1_REG, 0x0);
    gk_timer_writel(GK_CS_TIMER_MATCH2_REG, 0x0);
    gk_timer_setbitsl(TIMER_CTR_REG, GK_CS_TIMER_CTR_EN);
}

static cycle_t gk_cs_timer_read(struct clocksource *cs)
{
    return ((cycle_t)gk_timer_readl(GK_CS_TIMER_STATUS_REG));
}

static struct clocksource gk_cs_timer_clksrc =
{
    .name       = "gk-cs-timer",
    .rating     = GK_TIMER_RATING,
    .read       = gk_cs_timer_read,
    .mask       = CLOCKSOURCE_MASK(32),
    .mult       = 2236962133u,
    .shift      = 27,
    .flags      = CLOCK_SOURCE_IS_CONTINUOUS,
};

#endif

/* ==========================================================================*/
static void __init gk_timer_init(void)
{
    printk("mach gk init timer...\n");
#if defined(CONFIG_SUPPORT_CLOCKSOURCE)
    gk_cs_timer_init();
    clocks_calc_mult_shift(&gk_cs_timer_clksrc.mult,
        &gk_cs_timer_clksrc.shift,
        GK_TIMER_FREQ, NSEC_PER_SEC, 60);
    pr_debug("%s: mult = %u, shift = %u\n",
        gk_cs_timer_clksrc.name,
        gk_cs_timer_clksrc.mult,
        gk_cs_timer_clksrc.shift);
    clocksource_register(&gk_cs_timer_clksrc);
#endif

    gk_clkevt.cpumask = cpumask_of(0);
    setup_irq(gk_clkevt.irq, &gk_ce_timer_irq);
    clockevents_config_and_register(&gk_clkevt, GK_TIMER_FREQ, 0x01, 0xffffffff);


    return;

}

unsigned long gk_timer_offset(void)
{
    u32 val = (timer_reload - gk_timer_readl(GK_CE_TIMER_STATUS_REG))/timer_Mhz;

    return val;
}


struct sys_timer gk_sys_timer = {
    .init       = gk_timer_init,
    .offset     = gk_timer_offset,
};

int __init gk_init_timer(void)
{
    int errCode = 0;

    /* Reset all timers */
    //gk_timer_writel(TIMER_CTR_REG, 0x0);
    timer_reload = gk_timer_readl(GK_CE_TIMER_RELOAD_REG);
    gk_init_highres_timer(NULL);
    gk_start_highres_timer(100);

    return errCode;
}

//----------------------------------------
typedef void (* HIGH_RES_TIMER_CALLBACK) (void);

static HIGH_RES_TIMER_CALLBACK s_pCallback_high_res = NULL;

static u32 dev_id = 0;
static u32 timer_ms_delay = 12;
static u32 timer_intr_cnt = 0;
static void init_hw_timer(void)
{
    u32 interrupt_interval_in_ms = 1;
    u32 apb_clk = get_apb_bus_freq_hz();
    u32 timer_clk = apb_clk * interrupt_interval_in_ms /100;
    gk_timer_clrbitsl(TIMER_CTR_REG, TIMER_CTR_EN2);
    gk_timer_clrbitsl(TIMER_CTR_REG, TIMER_CTR_OF2);
    gk_timer_clrbitsl(TIMER_CTR_REG, TIMER_CTR_CSL2);
    gk_timer_writel(TIMER2_STATUS_REG, timer_clk);
    gk_timer_writel(TIMER2_RELOAD_REG, timer_clk);
    gk_timer_writel(TIMER2_MATCH1_REG, 0x0);
    gk_timer_writel(TIMER2_MATCH2_REG, 0x0);

    gk_timer_setbitsl(TIMER_CTR_REG, (TIMER_CTR_OF2 | TIMER_CTR_CSL2));
    gk_timer_clrbitsl(TIMER_CTR_REG, TIMER_CTR_CSL2);
    //gk_setbitsl(TIMER_CTR_REG, TIMER_CTR_EN2);
}

static void deinit_hw_timer(void)
{
    gk_timer_clrbitsl(TIMER_CTR_REG, TIMER_CTR_EN2);
}

static irqreturn_t hw_timer_irq_handler(int irqno, void *dev_id)
{
    gk_timer_irq_clr(irqno);

    //printk("timer2 irq...%d\n", timer_intr_cnt);

    if (timer_intr_cnt++ != timer_ms_delay)
        return IRQ_HANDLED;

    timer_intr_cnt = 0;
    //run callback, the callback must be fast enough to be put into interrupt context
    if(s_pCallback_high_res)
        s_pCallback_high_res();
    return IRQ_HANDLED;
}



int gk_init_highres_timer(void *pcallback)
{
    int rval;
    HIGH_RES_TIMER_CALLBACK ptimercb = (HIGH_RES_TIMER_CALLBACK)pcallback;

    printk("Init HW timer for DSP communication\n");

    s_pCallback_high_res = ptimercb;
    rval = request_irq(TIMER2_IRQ, hw_timer_irq_handler, IRQF_SHARED | IRQF_TIMER | IRQF_TRIGGER_RISING, "HW Timer2", &dev_id);

    if (rval < 0)
        printk("%s, %d, Can NOT request_irq %d\n",__func__,__LINE__,rval);
    init_hw_timer();
    return 0;
}

int gk_start_highres_timer(u32 ms_to_timeout)
{
    unsigned long flags;
    local_irq_save(flags);
    timer_ms_delay = ms_to_timeout;
    timer_intr_cnt = 0;
    local_irq_restore(flags);
    gk_timer_setbitsl(TIMER_CTR_REG, TIMER_CTR_EN2);
    return 0;
}

int gk_deinit_highres_timer(void)
{
    s_pCallback_high_res = NULL;
    deinit_hw_timer();
    return 0;
}

