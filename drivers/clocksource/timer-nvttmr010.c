/*
 * Novatek tmr010 driver.
 *
 * Copyright (C) 2019 Novatek MicroElectronics Corp.
 *
 * Updated by JJ Chen Jan 2020
 *
 * ----------------------------------------------------------------------------
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
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 * ----------------------------------------------------------------------------
 *
 */

#include <linux/interrupt.h>
#include <linux/clockchips.h>
#include <linux/clocksource.h>
#include <linux/clk.h>
#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/of_address.h>
#include <linux/sched_clock.h>
#include <asm/io.h>
#include <asm/mach/irq.h>
#include <plat/nvttmr010.h>
#include <linux/clk.h>
#include <linux/clkdev.h>
#include <linux/platform_device.h>

//For reset and clk-gating
#include <plat/hardware.h>
#include <plat/cg-reg.h>

#define DRV_VERSION     "1.0.0"   // major change, interface update, bug fix

#define NVTTMR010_COUNTER		0x00
#define NVTTMR010_LOAD			0x04
#define NVTTMR010_MATCH1		0x08
#define NVTTMR010_MATCH2		0x0c
#define NVTTMR010_TIMER(x)		((x) * 0x10)
#define NVTTMR010_CR			0x30
#define NVTTMR010_INTR_STATE	0x34
#define NVTTMR010_INTR_MASK		0x38

/*
 * Timer Control Register
 */
#define NVTTMR010_TM3_UPDOWN	(1 << 11)
#define NVTTMR010_TM2_UPDOWN	(1 << 10)
#define NVTTMR010_TM1_UPDOWN	(1 << 9)
#define NVTTMR010_TM3_OFENABLE	(1 << 8)
#define NVTTMR010_TM3_CLOCK		(1 << 7)
#define NVTTMR010_TM3_ENABLE	(1 << 6)
#define NVTTMR010_TM2_OFENABLE	(1 << 5)
#define NVTTMR010_TM2_CLOCK		(1 << 4)
#define NVTTMR010_TM2_ENABLE	(1 << 3)
#define NVTTMR010_TM1_OFENABLE	(1 << 2)
#define NVTTMR010_TM1_CLOCK		(1 << 1)
#define NVTTMR010_TM1_ENABLE	(1 << 0)

/*
 * Timer Interrupt State & Mask Registers
 */
#define NVTTMR010_TM3_OVERFLOW	(1 << 8)
#define NVTTMR010_TM3_MATCH2	(1 << 7)
#define NVTTMR010_TM3_MATCH1	(1 << 6)
#define NVTTMR010_TM2_OVERFLOW	(1 << 5)
#define NVTTMR010_TM2_MATCH2	(1 << 4)
#define NVTTMR010_TM2_MATCH1	(1 << 3)
#define NVTTMR010_TM1_OVERFLOW	(1 << 2)
#define NVTTMR010_TM1_MATCH2	(1 << 1)
#define NVTTMR010_TM1_MATCH1	(1 << 0)

#define EVTTMR  0
#define SRCTMR  1

static void __iomem *nvttmr010_base;
static unsigned int clk_reload;
static unsigned int ext_clk;

/******************************************************************************
 * internal functions
 *****************************************************************************/
static const unsigned int nvttmr010_cr_mask[3] = {
	NVTTMR010_TM1_ENABLE | NVTTMR010_TM1_CLOCK |
	NVTTMR010_TM1_OFENABLE | NVTTMR010_TM1_UPDOWN,

	NVTTMR010_TM2_ENABLE | NVTTMR010_TM2_CLOCK |
	NVTTMR010_TM2_OFENABLE | NVTTMR010_TM2_UPDOWN,

	NVTTMR010_TM3_ENABLE | NVTTMR010_TM3_CLOCK |
	NVTTMR010_TM3_OFENABLE | NVTTMR010_TM3_UPDOWN,
};

/* we always use down counter */
static const unsigned int nvttmr010_cr_enable_flag[3] = {
	NVTTMR010_TM1_ENABLE | NVTTMR010_TM1_OFENABLE,
	NVTTMR010_TM2_ENABLE | NVTTMR010_TM2_OFENABLE,
	NVTTMR010_TM3_ENABLE | NVTTMR010_TM3_OFENABLE,
};

static const unsigned int nvttmr010_cr_enable_noirq_flag[3] = {
	NVTTMR010_TM1_ENABLE,
	NVTTMR010_TM2_ENABLE,
	NVTTMR010_TM3_ENABLE,
};

static const unsigned int nvttmr010_ext_clk[3] = {
	NVTTMR010_TM1_CLOCK,
	NVTTMR010_TM2_CLOCK,
	NVTTMR010_TM3_CLOCK,
};

static void nvttmr010_enable(unsigned int id)
{
	unsigned int cr = readl(nvttmr010_base + NVTTMR010_CR);

	cr &= ~nvttmr010_cr_mask[id];
	cr |= nvttmr010_cr_enable_flag[id];
	if (ext_clk==1) {
		cr |= nvttmr010_ext_clk[id];
	}
	writel(cr, nvttmr010_base + NVTTMR010_CR);

	panic("333");
}

static void nvttmr010_enable_noirq(unsigned int id)
{
	unsigned int cr = readl(nvttmr010_base + NVTTMR010_CR);

	cr &= ~nvttmr010_cr_mask[id];
	cr |= nvttmr010_cr_enable_noirq_flag[id];
	if (ext_clk==1) {
		cr |= nvttmr010_ext_clk[id];
	}
	writel(cr, nvttmr010_base + NVTTMR010_CR);
}

static void nvttmr010_disable(unsigned int id)
{
	unsigned int cr = readl(nvttmr010_base + NVTTMR010_CR);

	cr &= ~nvttmr010_cr_mask[id];
	writel(cr, nvttmr010_base + NVTTMR010_CR);
}

static void nvttmr010_disable_all(void)
{
    int id;
	unsigned int cr = readl(nvttmr010_base + NVTTMR010_CR);

    for (id = 0; id < ARRAY_SIZE(nvttmr010_cr_mask); id ++)
	    cr &= ~nvttmr010_cr_mask[id];

	writel(cr, nvttmr010_base + NVTTMR010_CR);
}

static const unsigned int nvttmr010_irq_mask[3] = {
	NVTTMR010_TM1_MATCH1 | NVTTMR010_TM1_MATCH2 | NVTTMR010_TM1_OVERFLOW,
	NVTTMR010_TM2_MATCH1 | NVTTMR010_TM2_MATCH2 | NVTTMR010_TM2_OVERFLOW,
	NVTTMR010_TM3_MATCH1 | NVTTMR010_TM3_MATCH2 | NVTTMR010_TM3_OVERFLOW,
};

static void nvttmr010_mask_allirq(void)
{
    unsigned int id, mask = 0;

    for (id = 0; id < ARRAY_SIZE(nvttmr010_irq_mask); id ++)
        mask |= nvttmr010_irq_mask[id];

    writel(mask, nvttmr010_base + NVTTMR010_INTR_MASK);
}

static void nvttmr010_unmask_irq(unsigned int id)
{
	unsigned int mask;

	mask = readl(nvttmr010_base + NVTTMR010_INTR_MASK);
	mask &= ~nvttmr010_irq_mask[id];
	writel(mask, nvttmr010_base + NVTTMR010_INTR_MASK);
}

static inline void nvttmr010_write_timer(unsigned int id, unsigned int reg,
					unsigned int value)
{
	void __iomem *addr = nvttmr010_base + NVTTMR010_TIMER(id) + reg;
	writel (value, addr);
}

static inline unsigned int nvttmr010_read_timer(unsigned int id,
					       unsigned int reg)
{
	void __iomem *addr = nvttmr010_base + NVTTMR010_TIMER(id) + reg;
	return readl(addr);
}

static void nvttmr010_set_counter(unsigned int id, unsigned int value)
{
	nvttmr010_write_timer(id, NVTTMR010_COUNTER, value);
}

static void nvttmr010_set_reload(unsigned int id, unsigned int value)
{
	nvttmr010_write_timer(id, NVTTMR010_LOAD, value);
}

static void nvttmr010_set_match1(unsigned int id, unsigned int value)
{
	nvttmr010_write_timer(id, NVTTMR010_MATCH1, value);
}

static void nvttmr010_set_match2(unsigned int id, unsigned int value)
{
	nvttmr010_write_timer(id, NVTTMR010_MATCH2, value);
}

/******************************************************************************
 * clockevent functions
 *****************************************************************************/
static int nvttmr010_set_next_event(unsigned long clc,
				   struct clock_event_device *ce)
{
	nvttmr010_set_counter(EVTTMR, clc);
	return 0;
}

/*
static void nvttmr010_set_mode(enum clock_event_state mode,
			      struct clock_event_device *ce)
{
	switch (mode) {
	case CLOCK_EVT_STATE_PERIODIC:
		nvttmr010_set_reload(EVTTMR, clk_reload);
		nvttmr010_set_counter(EVTTMR, clk_reload);
		nvttmr010_enable(EVTTMR);
		break;

	case CLOCK_EVT_STATE_ONESHOT_STOPPED:
		nvttmr010_enable(EVTTMR);
		break;

	case CLOCK_EVT_STATE_ONESHOT:
		nvttmr010_set_reload(EVTTMR, 0);
		nvttmr010_enable(EVTTMR);
		break;

	case CLOCK_EVT_STATE_DETACHED:
	case CLOCK_EVT_STATE_SHUTDOWN:
	default:
		nvttmr010_disable(EVTTMR);
		printk("%s, shutdown tmr%d \n", __func__, EVTTMR);
		break;
	}
}
*/

static int nvttmr010_clock_event_shutdown(struct clock_event_device *ce)
{
	nvttmr010_disable(EVTTMR);
	printk("%s, shutdown tmr%d \n", __func__, EVTTMR);
	return 0;
}

static int nvttmr010_clock_event_periodic(struct clock_event_device *ce)
{
	nvttmr010_set_reload(EVTTMR, clk_reload);
	nvttmr010_set_counter(EVTTMR, clk_reload);
	nvttmr010_enable(EVTTMR);

	return 0;
}

static int nvttmr010_clock_event_oneshot(struct clock_event_device *ce)
{
	nvttmr010_set_reload(EVTTMR, 0);
	nvttmr010_enable(EVTTMR);

	return 0;
}

static irqreturn_t nvttmr010_clockevent_interrupt(int irq, void *dev_id)
{
	unsigned int state;
    struct clock_event_device *ce = (struct clock_event_device *)dev_id;

	state = readl_relaxed(nvttmr010_base + NVTTMR010_INTR_STATE);
	if (!(state & nvttmr010_irq_mask[EVTTMR])) {
	    printk("%s, error, tmrid=%d, state:0x%x, mask:0x%x! \n", __func__, EVTTMR, state, nvttmr010_irq_mask[EVTTMR]);
		return IRQ_NONE;
	}

	//state &= ~nvttmr010_irq_mask[EVTTMR];
	writel_relaxed(state, nvttmr010_base + NVTTMR010_INTR_STATE);

	if (ce == NULL || ce->event_handler == NULL) {
		pr_warn("nvttmr010: event_handler is not found!\n");
		return IRQ_NONE;
	}

	ce->event_handler(ce);

	return IRQ_HANDLED;
}

static struct clock_event_device nvttmr010_clkevt = {
        .name           	= "nvttmr010",
        .features       	= CLOCK_EVT_FEAT_PERIODIC | CLOCK_EVT_FEAT_ONESHOT,
        .rating         	= 200,
        .set_next_event 	= nvttmr010_set_next_event,
        //.set_mode       = nvttmr010_set_mode,
		.set_state_shutdown = nvttmr010_clock_event_shutdown,
		.set_state_periodic = nvttmr010_clock_event_periodic,
		.set_state_oneshot 	= nvttmr010_clock_event_oneshot,
};

static struct irqaction nvttmr010_irq = {
        .name           = "nvttmr010",
        .flags          = IRQF_TIMER | IRQF_IRQPOLL,
        .handler        = nvttmr010_clockevent_interrupt,
        .dev_id         = &nvttmr010_clkevt,
};

#if 0 // register nvttmr010 timer as the sched_clock is not necessarily required
static u64 notrace nvttmr010_sched_clock_read(void)
{
	return ~readl_relaxed(nvttmr010_base + NVTTMR010_TIMER(SRCTMR) +
			      NVTTMR010_COUNTER);
}
#endif

struct clocksource_mmio {
        void __iomem *reg;
        struct clocksource clksrc;
};

struct nvttmr010_clocksource nvttmr010;
bool nvttmr010_clksrc_is_init = false;
struct clocksource_mmio *cs = NULL;
int __init nvttmr010_clocksource_init(void __iomem *base, const char *name,
        unsigned long hz, int rating, unsigned bits,
        u64 (*read)(struct clocksource *))
{
    if (bits > 32 || bits < 16)
            return -EINVAL;

    cs = kzalloc(sizeof(struct clocksource_mmio), GFP_KERNEL);
    if (!cs)
            return -ENOMEM;

    cs->reg = base;
    cs->clksrc.name = name;
    cs->clksrc.rating = rating;
    cs->clksrc.read = read;
    cs->clksrc.mask = CLOCKSOURCE_MASK(bits);
    cs->clksrc.flags = CLOCK_SOURCE_IS_CONTINUOUS;

    return clocksource_register_hz(&cs->clksrc, hz);
}

u64 clocksource_mmio_readl_downA(struct clocksource *c)
{
        //return ~(u64)readl_relaxed(to_mmio_clksrc(c)->reg) & c->mask;
        return ~(u64)readl_relaxed(nvttmr010_base + NVTTMR010_TIMER(SRCTMR) +
			      NVTTMR010_COUNTER);
}

nvttmr010_clocksource nvttmr010_get_clocksource(void)
{
	if (!nvttmr010_clksrc_is_init) {
		return NULL;
	} else {
		return &nvttmr010;
	}
}

void __init nvttmr010_init(void __iomem *base, int irq, unsigned long clk_freq)
{
	nvttmr010_base = base;
	nvttmr010_irq.irq = irq;

    /* disable all timers first because they may be used in uboot or romcode */
    nvttmr010_disable_all();
    nvttmr010_mask_allirq();

	/* setup as free-running clocksource */
	nvttmr010_disable(SRCTMR);
	nvttmr010_set_match1(SRCTMR, 0);
	nvttmr010_set_match2(SRCTMR, 0);
	nvttmr010_set_reload(SRCTMR, 0xffffffff);
	nvttmr010_set_counter(SRCTMR, 0xffffffff);
	nvttmr010_enable_noirq(SRCTMR);

#if 0 // register nvttmr010 timer as the sched_clock is not necessarily required
	sched_clock_register(nvttmr010_sched_clock_read, BITS_PER_LONG,
			     clk_freq);
#endif

	//if (clocksource_mmio_init(nvttmr010_base + NVTTMR010_TIMER(SRCTMR) +
	if (nvttmr010_clocksource_init(nvttmr010_base + NVTTMR010_TIMER(SRCTMR) +
				  NVTTMR010_COUNTER, "nvttmr010_clksrc", clk_freq,
				  200/*300*/, 32, clocksource_mmio_readl_downA)) {
                pr_err("Failed to register clocksource\n");
                BUG();
	}

	/* initialize to a known state */
	nvttmr010_disable(EVTTMR);
	nvttmr010_set_match1(EVTTMR, 0);
	nvttmr010_set_match2(EVTTMR, 0);
	nvttmr010_unmask_irq(EVTTMR);

	/* setup reload value for periodic clockevents */
	clk_reload = clk_freq / HZ;

	/* Make irqs happen for the system timer */

	if (setup_irq(nvttmr010_irq.irq, &nvttmr010_irq)) {
                pr_err("Failed to register timer IRQ\n");
                BUG();
	}

	/* setup struct clock_event_device */
#if defined(CONFIG_CPU_CA7) && defined(CONFIG_SMP)
	nvttmr010_clkevt.cpumask	= cpu_all_mask;
#else
	nvttmr010_clkevt.cpumask	= cpumask_of(0);
#endif
	nvttmr010_clkevt.irq = nvttmr010_irq.irq;

	clockevents_config_and_register(&nvttmr010_clkevt, clk_freq, 0xf,
					0xffffffff);

	/* Set clocksource */
	nvttmr010.clocksource = cs->clksrc;
	nvttmr010.base = base;
	nvttmr010.id = SRCTMR;
	nvttmr010.freq = clk_freq;
	nvttmr010_clksrc_is_init = true;
}

int __init nvttmr010_of_init(struct device_node *np)
{
	//struct clk *clk;
	u32 *pfreq, freq=0;
	u32 *pext_clk;
	void __iomem *base;
	int irq;

	base = of_iomap(np, 0);
	if (WARN_ON(!base))
		return -1;

	irq = irq_of_parse_and_map(np, 0);
	if (irq <= 0)
		goto err;

	//clk = of_clk_get(np, 0);
	//clk_prepare_enable(clk);
	//freq = clk_get_rate(clk);
 	//printk("nvttmr010_of_init...base=%x, irq=%d, clk=%x, freq=%d\n", base, irq, clk, freq);

	printk("NVTTMR010 Driver Version: %s\n", DRV_VERSION);

	nvttmr010_clkevt.name = of_get_property(np, "compatible", NULL);
	//printk("nvttmr010_of_init...nvttmr010_clkevt.name=%s base=%x, irq=%d, freq=%d\n", nvttmr010_clkevt.name, base, irq, freq);

	pfreq = (u32 *)of_get_property(np, "clocks", NULL);
	freq = __be32_to_cpu(*pfreq);

	pext_clk = (u32 *)of_get_property(np, "ext_clk", NULL);
	ext_clk = __be32_to_cpu(*pext_clk);

	nvttmr010_init(base, irq, freq);
	return 0;

err:
	iounmap(base);
    return -1;
}

TIMER_OF_DECLARE(nvttmr010, "faraday,fttmr010", nvttmr010_of_init);
