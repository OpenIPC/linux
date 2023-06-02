#include <linux/init.h>
#include <linux/device.h>
#include <linux/dma-mapping.h>
#include <linux/platform_device.h>
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
#include <asm/mach-types.h>
#include <linux/irqchip/arm-gic.h>    

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
#include "mach/clock.h" 
#include "platsmp.h"

#define GPIO0_MULT_USE_EN (GPIO_BASE)



static struct map_desc xm530_io_desc[] __initdata = {
	{
		.virtual        = XM530_IOCH1_VIRT,
		.pfn            = __phys_to_pfn(XM530_IOCH1_PHYS),
		.length         = XM530_IOCH1_SIZE,
		.type           = MT_DEVICE
	},
	{
		.virtual        = XM530_IOCH2_VIRT,
		.pfn            = __phys_to_pfn(XM530_IOCH2_PHYS),
		.length         = XM530_IOCH2_SIZE,
		.type           = MT_DEVICE
	}
};


void __init xm530_map_io(void)
{
	int i;

	iotable_init(xm530_io_desc, ARRAY_SIZE(xm530_io_desc));

	for (i = 0; i < ARRAY_SIZE(xm530_io_desc); i++) {
		edb_putstr(" V: ");     edb_puthex(xm530_io_desc[i].virtual);
		edb_putstr(" P: ");     edb_puthex(xm530_io_desc[i].pfn);
		edb_putstr(" S: ");     edb_puthex(xm530_io_desc[i].length);
		edb_putstr(" T: ");     edb_putul(xm530_io_desc[i].type);
		edb_putstr("\n");
	}

	edb_trace();
}


void __iomem *xm530_gic_cpu_base_addr;     
void __init xm530_gic_init_irq(void)
{
	edb_trace();
	xm530_gic_cpu_base_addr = __io_address(CFG_GIC_CPU_BASE);
#ifdef CONFIG_LOCAL_TIMERS
	gic_init(0, IRQ_LOCALTIMER, __io_address(CFG_GIC_DIST_BASE),
			__io_address(CFG_GIC_CPU_BASE));
#else
	gic_init(0, XM530_GIC_IRQ_START, __io_address(CFG_GIC_DIST_BASE),
			__io_address(CFG_GIC_CPU_BASE));
#endif
}


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
		.irq            = { base##_IRQ, base##_IRQ, }		\
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
	.rate   = 12000000,
};
static struct clk sp804_clk = { 
	.rate = 12000000,
};
static struct clk dma_clk = { 
	.rate = 12000000,
};

//正式芯片为CPU时钟的1/4 或与CPU时钟相等
static struct clk twd_clk = { 
	.rate = 150000000,
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
	{ /* SP804 timers */
		.dev_id     = "sp804",
		.clk        = &sp804_clk,
	},
	{ 
		.dev_id     = "dma-pl330",
		.clk        = &dma_clk,
	},
	{ 
		.dev_id     = "smp_twd",
		.clk        = &twd_clk,
	},
};

static void __init xm530_init_early(void)    
{
	unsigned int tmp;
	unsigned int pllclk;
	edb_trace();
	tmp = readl(__io_address(PLL_PLLA_CTRL));
	pllclk = 12000000 / (tmp & 0x3F) * ((tmp >> 6) & 0xFFF) / (((tmp >> 19) & 0x1) + 1);

	tmp = readl(__io_address(PLL_CPUCLK_CTRL));
	twd_clk.rate = pllclk / ((tmp  & 0xFF) + 1) / (((tmp >> 20) & 0x1) == 0 ? 1 : 4);

	clkdev_add_table(lookups, ARRAY_SIZE(lookups));

}

void __init xm530_init(void)
{
	unsigned long i;

	edb_trace();
	

	for (i = 0; i < ARRAY_SIZE(amba_devs); i++) {
		amba_device_register(amba_devs[i], &iomem_resource);
	}
}
static void __init xm530_reserve(void)
{
}
void xm530_restart(char mode, const char *cmd)
{
	writel(1, __io_address(SYS_CTRL_BASE + REG_SYS_SOFT_RSTEN));
	writel(0xca110000, __io_address(SYS_CTRL_BASE + REG_SYS_SOFT_RST));
}

extern void __init xm530_timer_init(void);

asmlinkage void asmprint(void)
{
	edb_trace();
}

MACHINE_START(XM530, "xm530")
	.atag_offset  = 0x100,
	.map_io         = xm530_map_io,
	.init_early     = xm530_init_early,
	.init_irq       = xm530_gic_init_irq,
	.init_time    	= xm530_timer_init,
	.init_machine   = xm530_init,
	.smp          = smp_ops(xm530_smp_ops),
	.reserve      = xm530_reserve,
	.restart      = xm530_restart,
MACHINE_END
