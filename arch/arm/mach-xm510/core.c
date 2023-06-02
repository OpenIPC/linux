#include <linux/init.h>
#include <linux/device.h>
#include <linux/dma-mapping.h>
#include <linux/platform_device.h>
#include <linux/sysdev.h>
#include <linux/interrupt.h>
#include <linux/amba/bus.h>
#include <linux/amba/clcd.h>
#include <linux/clocksource.h>
#include <linux/clockchips.h>
#include <linux/cnt32_to_63.h>
#include <linux/io.h>
#include <linux/clkdev.h>
#include <asm/sched_clock.h>

#include <asm/system.h>
#include <asm/irq.h>
#include <linux/leds.h>
#include <asm/hardware/arm_timer.h>
#include <asm/hardware/gic.h>
#include <asm/hardware/vic.h>
#include <asm/mach-types.h>

#include <asm/mach/arch.h>
#include <asm/mach/flash.h>
#include <asm/mach/irq.h>
#include <asm/mach/time.h>
#include <asm/mach/map.h>

#include <mach/time.h>
#include <mach/hardware.h>
#include <mach/early-debug.h>
#include <mach/irqs.h>
#include <mach/dma.h>
#include <linux/bootmem.h>
#include <linux/amba/serial.h> 
#include <linux/amba/pl330.h> 
#include "clock.h"

#define ioaddr_intc(x)	IO_ADDRESS(INTC_BASE + (x))
#define GPIO0_MULT_USE_EN IO_ADDRESS(GPIO_BASE)


#define do_level_IRQ    handle_level_irq
static void xm_ack_irq(struct irq_data *data)
{
	unsigned long irq = data->irq;
	irq -= INTNR_IRQ_START;
	writel(1<<irq, ioaddr_intc(REG_INTC_INTENCLEAR));
}

static void xm_mask_irq(struct irq_data *data)
{
	unsigned long irq = data->irq;
	irq -= INTNR_IRQ_START;
	writel(1<<irq, ioaddr_intc(REG_INTC_INTENCLEAR));
}

static void xm_unmask_irq(struct irq_data *data)
{
	unsigned long irq = data->irq;
	irq -= INTNR_IRQ_START;
	writel(1<<irq, ioaddr_intc(REG_INTC_INTENABLE));
}

static struct irq_chip xm_chip = {
	.irq_ack	= xm_ack_irq,
	.irq_mask	= xm_mask_irq,
	.irq_unmask	= xm_unmask_irq,
	.irq_disable	= xm_mask_irq,
};

static struct map_desc xm510_io_desc[] __initdata = {
	{
		.virtual        = XM510_IOCH1_VIRT,
		.pfn            = __phys_to_pfn(XM510_IOCH1_PHYS),
		.length         = XM510_IOCH1_SIZE,
		.type           = MT_DEVICE
	},
	{
		.virtual        = XM510_IOCH2_VIRT,
		.pfn            = __phys_to_pfn(XM510_IOCH2_PHYS),
		.length         = XM510_IOCH2_SIZE,
		.type           = MT_DEVICE
	}
};

static unsigned long xm510_timer_reload, timer0_clk_hz, timer1_clk_hz,
		     timer0_clk_khz, timer1_clk_khz;


static void early_init(void)
{
	unsigned long timerclk;

	edb_trace();
	timerclk = CFG_TIMER_CLK;
	printk(KERN_INFO "timer clock %ld.\n", timerclk);

	xm510_timer_reload = timerclk / HZ;
	timer0_clk_hz = timerclk;
	timer0_clk_khz = timerclk / 1000;
	timer1_clk_hz = timerclk;
	timer1_clk_khz = timerclk / 1000;

}

void __init xm510_map_io(void)
{
	int i;

	iotable_init(xm510_io_desc, ARRAY_SIZE(xm510_io_desc));

	for (i = 0; i < ARRAY_SIZE(xm510_io_desc); i++) {
		edb_putstr(" V: ");     edb_puthex(xm510_io_desc[i].virtual);
		edb_putstr(" P: ");     edb_puthex(xm510_io_desc[i].pfn);
		edb_putstr(" S: ");     edb_puthex(xm510_io_desc[i].length);
		edb_putstr(" T: ");     edb_putul(xm510_io_desc[i].type);
		edb_putstr("\n");
	}

	early_init();

	edb_trace();
}

void __init xm510_init_irq(void)
{
	unsigned int i;

	edb_trace();

	writel(~0, ioaddr_intc(REG_INTC_INTENCLEAR));
	writel(0, ioaddr_intc(REG_INTC_INTSELECT));
	writel(~0, ioaddr_intc(REG_INTC_SOFTINTCLEAR));

	for (i = INTNR_IRQ_START; i <= INTNR_IRQ_END; i++) {
		irq_set_chip(i, &xm_chip);
		irq_set_handler(i, do_level_IRQ);
		set_irq_flags(i, IRQF_VALID | IRQF_PROBE);
	}
}

static unsigned long long xm510_cycles_2_ns(unsigned long long cyc)
{
	unsigned long cyc2ns_scale = (1000000 << 10)/timer1_clk_khz;
	return (cyc * cyc2ns_scale) >> 10;
}

static unsigned long free_timer_overflows;

unsigned long long sched_clock(void)
{
	unsigned long long ticks64;
	unsigned long ticks2, ticks1;
	ticks2 = 0UL - (unsigned long)readl(CFG_TIMER01_VABASE
			+ REG_TIMER1_VALUE);
	do {
		ticks1 = ticks2;
		ticks64 = free_timer_overflows;
		ticks2 = 0UL - (unsigned long)readl(CFG_TIMER01_VABASE
				+ REG_TIMER1_VALUE);
	} while (ticks1 > ticks2);

	/*
	 ** If INT is not cleaned, means the function is called with irq_save.
	 ** The ticks has overflow but 'free_timer_overflows' is not be update.
	 */
	if (readl(CFG_TIMER01_VABASE + REG_TIMER1_MIS)) {
		ticks64 += 1;
		ticks2 = 0UL - (unsigned long)readl(CFG_TIMER01_VABASE
				+ REG_TIMER1_VALUE);
	}

	//printk(KERN_NOTICE "\033[1;35m REG=%#x, ticks2=%#x \n\033[m",(0UL - (unsigned long)readl(CFG_TIMER01_VABASE + REG_TIMER1_VALUE)), ticks2);
	//printk(KERN_NOTICE"\033[1;35m free_timer=%#x, ticks64=%#x,ticks2=%ld,ns=%lld\n\033[m",free_timer_overflows, ticks64, ticks2, xm510_cycles_2_ns((ticks64 << 32) | ticks2));
	return xm510_cycles_2_ns((ticks64 << 32) | ticks2);
}

static DEFINE_CLOCK_DATA(cd);
static void *ctr;
static void xm510_update_sched_clock(void)
{
	u32 cyc = readl(ctr);
	update_sched_clock(&cd, cyc, (u32)~0);
}
static struct clocksource xm510_clocksource;

static void __init xm510_sched_clock_init(void* reg, unsigned long rate)
{
	ctr = reg;
	init_fixed_sched_clock(&cd, xm510_update_sched_clock,
			32, rate, xm510_clocksource.mult,
			xm510_clocksource.shift);
}


static void timer_set_mode(enum clock_event_mode mode,
		struct clock_event_device *clk)
{
	unsigned long ctrl;
	switch (mode) {
	case CLOCK_EVT_MODE_PERIODIC:
		writel(0, CFG_TIMER01_VABASE + REG_TIMER_CONTROL);
		writel(xm510_timer_reload, CFG_TIMER01_VABASE
				+ REG_TIMER_RELOAD);
		writel(CFG_TIMER_CONTROL, CFG_TIMER01_VABASE
				+ REG_TIMER_CONTROL);
		edb_trace();
		break;
	case CLOCK_EVT_MODE_ONESHOT:
		writel((CFG_TIMER_32BIT | CFG_TIMER_ONESHOT),
				CFG_TIMER01_VABASE + REG_TIMER_CONTROL);
		break;
	case CLOCK_EVT_MODE_UNUSED:
	case CLOCK_EVT_MODE_SHUTDOWN:
	default:
	ctrl = readl(CFG_TIMER01_VABASE + REG_TIMER_CONTROL);
	ctrl &= ~CFG_TIMER_ENABLE;
		writel(ctrl, CFG_TIMER01_VABASE + REG_TIMER_CONTROL);
	}
}

static int timer_set_next_event(unsigned long evt,
		struct clock_event_device *unused)
{
	unsigned long ctrl;
	ctrl = readl(CFG_TIMER01_VABASE + REG_TIMER_CONTROL);
	ctrl &= ~(CFG_TIMER_ENABLE | CFG_TIMER_INTMASK);
	writel(ctrl, CFG_TIMER01_VABASE + REG_TIMER_CONTROL);
	writel(evt, CFG_TIMER01_VABASE + REG_TIMER_RELOAD);
	writel(CFG_TIMER_ONE_CONTROL, CFG_TIMER01_VABASE + REG_TIMER_CONTROL);

	return 0;
}

static struct clock_event_device timer0_clockevent = {
	.name           = "timer0",
	.shift          = 32,
	.features       = CLOCK_EVT_FEAT_PERIODIC | CLOCK_EVT_FEAT_ONESHOT,
	.set_mode       = timer_set_mode,
	.set_next_event = timer_set_next_event,
};

/*
 * IRQ handler for the timer
 */
static irqreturn_t xm510_timer_interrupt(int irq, void *dev_id)
{
	if ((readl(CFG_TIMER01_VABASE+REG_TIMER_RIS)) & 0x1) {
		writel(~0, CFG_TIMER01_VABASE + REG_TIMER_INTCLR);
		timer0_clockevent.event_handler(&timer0_clockevent);
	}

	return IRQ_HANDLED;
}

static irqreturn_t xm510_freetimer_interrupt(int irq, void *dev_id)
{
	if ((readl(CFG_TIMER01_VABASE+REG_TIMER1_RIS)) & 0x1) {
		free_timer_overflows++;
		writel(~0, CFG_TIMER01_VABASE + REG_TIMER1_INTCLR);
	}
	return IRQ_HANDLED;
}

static struct irqaction xm510_timer_irq = {
	.name           = "System Timer Tick",
	.flags          = IRQF_SHARED | IRQF_DISABLED | IRQF_TIMER,
	.handler        = xm510_timer_interrupt,
};

static struct irqaction xm510_freetimer_irq = {
	.name           = "Free Timer",
	.flags          = IRQF_SHARED | IRQF_TIMER,
	.handler        = xm510_freetimer_interrupt,
};
static cycle_t xm510_get_cycles(struct clocksource *cs)
{
	return ~readl(CFG_TIMER01_VABASE + REG_TIMER1_VALUE);
}

static struct clocksource xm510_clocksource = {
	.name           = "timer1",
	.rating         = 200,
	.read           = xm510_get_cycles,
	.mask           = CLOCKSOURCE_MASK(32),
	.shift          = 26,
	.flags          = CLOCK_SOURCE_IS_CONTINUOUS,
};

static int __init xm510_clocksource_init(void)
{
	writel(0, CFG_TIMER01_VABASE + REG_TIMER1_CONTROL);
	writel(0xffffffff, CFG_TIMER01_VABASE + REG_TIMER1_RELOAD);
	writel(0xffffffff, CFG_TIMER01_VABASE + REG_TIMER1_VALUE);
	writel(CFG_TIMER_CONTROL, CFG_TIMER01_VABASE + REG_TIMER1_CONTROL);

	xm510_clocksource.mult =
		clocksource_khz2mult(timer1_clk_khz, xm510_clocksource.shift);

	xm510_sched_clock_init((void *)CFG_TIMER01_VABASE
			+ REG_TIMER1_VALUE, timer1_clk_hz);

	clocksource_register(&xm510_clocksource);

	return 0;
}

static void __init xm510_timer_init(void)
{

	edb_trace();


	setup_irq(TIMER01_IRQ, &xm510_timer_irq);
	setup_irq(TIMER01_IRQ, &xm510_freetimer_irq);

	xm510_clocksource_init();
	timer0_clockevent.mult =
		div_sc(timer0_clk_hz, NSEC_PER_SEC, timer0_clockevent.shift);
	timer0_clockevent.max_delta_ns =
		clockevent_delta2ns(0xffffffff, &timer0_clockevent);
	timer0_clockevent.min_delta_ns =
		clockevent_delta2ns(0xf, &timer0_clockevent);

	timer0_clockevent.cpumask = cpumask_of(0);
	clockevents_register_device(&timer0_clockevent);
	edb_trace();
}

struct sys_timer xm510_timer = {
	.init           = xm510_timer_init,
};

//static struct amba_pl011_data uart1_plat_data = {
	//.dma_filter = pl330_filter,
	//.dma_rx_param = (void *) DMACH_UART1_RX,
	//.dma_tx_param = (void *) DMACH_UART1_TX,
//};

#define XM_AMBADEV_NAME(name) xm_ambadevice_##name

#define XM_AMBA_DEVICE(name, busid, base, platdata)			\
	static struct amba_device XM_AMBADEV_NAME(name) =		\
	{\
		.dev            = {                                     \
			.coherent_dma_mask = ~0,                        \
			.init_name = busid,                             \
			.platform_data = platdata,                      \
		},                                                      \
		.res            = {                                     \
			.start  = base##_BASE,				\
			.end    = base##_BASE + 0x1000 - 1,		\
			.flags  = IORESOURCE_IO,                        \
		},                                                      \
		.dma_mask       = ~0,                                   \
		.irq            = { base##_IRQ, NO_IRQ }		\
	}

XM_AMBA_DEVICE(uart0, "uart:0",  UART0,    NULL);
XM_AMBA_DEVICE(uart1, "uart:1",  UART1,    NULL);
//XM_AMBA_DEVICE(uart1, "uart:1",  UART1,    &uart1_plat_data);

static struct amba_device *amba_devs[] __initdata = {
	&XM_AMBADEV_NAME(uart0),
	&XM_AMBADEV_NAME(uart1),
};

/*
 * These are fixed clocks.
 */
static struct clk uart_clk = {
	.rate   = 24000000,
};

static struct clk_lookup lookups[] = {
	{       /* UART0 */
		.dev_id         = "uart:0",
		.clk            = &uart_clk,
	},
	{       /* UART1 */
		.dev_id         = "uart:1",
		.clk            = &uart_clk,
	},
};

void __init xm510_init(void)
{
	unsigned long i;
	unsigned int tmp;

	edb_trace();
	
	tmp = readl(GPIO0_MULT_USE_EN);
	tmp &= ~((1 << 4) | (1 << 5));
	tmp |= (1 << 12) | (1 << 13);
	writel(tmp, GPIO0_MULT_USE_EN);

	clkdev_add_table(lookups, ARRAY_SIZE(lookups));

	for (i = 0; i < ARRAY_SIZE(amba_devs); i++) {
		edb_trace();
		amba_device_register(amba_devs[i], &iomem_resource);
	}
}

MACHINE_START(XM510, "xm510")
	.boot_params    = PLAT_PHYS_OFFSET + 0x100,
	.map_io         = xm510_map_io,
	.init_irq       = xm510_init_irq,
	.timer          = &xm510_timer,
	.init_machine   = xm510_init,
MACHINE_END
