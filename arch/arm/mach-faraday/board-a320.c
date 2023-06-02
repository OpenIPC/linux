/*
 *  linux/arch/arm/mach-faraday/board-a320.c
 *
 *  Copyright (C) 2009 Faraday Technology
 *  Copyright (C) 2009 Po-Yu Chuang <ratbert@faraday-tech.com>
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <linux/clk.h>
#include <linux/clkdev.h>
#include <linux/dma-mapping.h>
#include <linux/platform_device.h>

#include <asm/mach-types.h>
#ifdef CONFIG_CPU_FMP626
#include <asm/localtimer.h>
#include <asm/hardware/gic.h>
#include <asm/pmu.h>
#endif

#include <asm/mach/arch.h>
#include <asm/mach/map.h>
#include <asm/mach/time.h>

#include <mach/board-a320.h>
#include <mach/ftintc010.h>
#include <mach/fttmr010.h>
#include <mach/ftpci100.h>
#include <mach/ftpmu010.h>

#include "clock.h"
#ifdef CONFIG_CPU_FMP626
#include "core.h"
#endif

/******************************************************************************
 * platform devices
 *****************************************************************************/

#ifdef CONFIG_CPU_FMP626
static struct resource pmu_resources[] = {
	[0] = {
		.start		= IRQ_FMP626_PMU_CPU0,
		.flags		= IORESOURCE_IRQ,
	},
	[1] = {
		.start		= IRQ_FMP626_PMU_CPU1,
		.flags		= IORESOURCE_IRQ,
	},
};

static struct platform_device pmu_device = {
	.name			= "arm-pmu",
	.id			= ARM_PMU_DEVICE_CPU,
	.num_resources		= ARRAY_SIZE(pmu_resources),
	.resource		= pmu_resources,
};
#endif

/*
 * FTAPBB020
 */
static struct resource ftapbb020_0_resources[] = {
	{
		.start	= A320_FTAPBB020_0_PA_BASE,
		.end	= A320_FTAPBB020_0_PA_BASE + SZ_4K - 1,
		.flags	= IORESOURCE_MEM,
	}, {
		.start	= IRQ_A320_FTAPBB020_0,
		.flags	= IORESOURCE_IRQ,
	},
};

static struct platform_device ftapbb020_0_device = {
	.name		= "ftapbb020",
	.id		= 0,
	.num_resources	= ARRAY_SIZE(ftapbb020_0_resources),
	.resource	= ftapbb020_0_resources,
};


/*
 * FTLCDC100
 */
static struct resource ftlcdc100_0_resources[] = {
	{
		.start	= A320_FTLCDC100_0_PA_BASE,
		.end	= A320_FTLCDC100_0_PA_BASE + SZ_4K - 1,
		.flags	= IORESOURCE_MEM,
	}, {
		.start	= IRQ_A320_FTLCDC100_0,
		.flags	= IORESOURCE_IRQ,
	},
};

static struct platform_device ftlcdc100_0_device = {
	.name		= "ftlcdc100",
	.num_resources	= ARRAY_SIZE(ftlcdc100_0_resources),
	.resource	= ftlcdc100_0_resources,
};

/*
 * FTMAC100
 */
static struct resource ftmac100_0_resources[] = {
	{
		.start	= A320_FTMAC100_0_PA_BASE,
		.end	= A320_FTMAC100_0_PA_BASE + SZ_4K - 1,
		.flags	= IORESOURCE_MEM,
	}, {
		.start	= IRQ_A320_FTMAC100_0,
		.flags	= IORESOURCE_IRQ,
	},
};

static struct platform_device ftmac100_0_device = {
	.name		= "ftmac100",
	.id		= 0,
	.dev		= {
		.coherent_dma_mask	= DMA_BIT_MASK(32),
	},
	.num_resources	= ARRAY_SIZE(ftmac100_0_resources),
	.resource	= ftmac100_0_resources,
};

#ifdef CONFIG_PLATFORM_FIA321
static struct resource ftmac100_1_resources[] = {
	{
		.start	= A321_FTMAC100_0_PA_BASE,
		.end	= A321_FTMAC100_0_PA_BASE + SZ_4K - 1,
		.flags	= IORESOURCE_MEM,
	}, {
		.start	= IRQ_A321_FTMAC100_0,
		.flags	= IORESOURCE_IRQ,
	},
};

static struct platform_device ftmac100_1_device = {
	.name		= "ftmac100",
	.id		= 1,
	.dev		= {
		.coherent_dma_mask	= DMA_BIT_MASK(32),
	},
	.num_resources	= ARRAY_SIZE(ftmac100_1_resources),
	.resource	= ftmac100_1_resources,
};
#endif

/*
 * FTRTC010
 */
static struct resource ftrtc010_0_resources[] = {
	{
		.start	= A320_FTRTC010_0_PA_BASE,
		.end	= A320_FTRTC010_0_PA_BASE + SZ_4K - 1,
		.flags	= IORESOURCE_MEM,
	}, {		/* alarm interrupt */
		.start	= IRQ_A320_FTRTC010_0_ALRM,
		.flags	= IORESOURCE_IRQ,
	}, {		/* periodic interrupt */
		.start	= IRQ_A320_FTRTC010_0_SEC,
		.flags	= IORESOURCE_IRQ,
	},
};

static struct platform_device ftrtc010_0_device = {
	.name		= "ftrtc010",
	.id		= 0,
	.num_resources	= ARRAY_SIZE(ftrtc010_0_resources),
	.resource	= ftrtc010_0_resources,
};

static struct platform_device *a320_devices[] __initdata = {
#ifdef CONFIG_CPU_FMP626
	&pmu_device,
#endif
	&ftapbb020_0_device,
	&ftlcdc100_0_device,
	&ftmac100_0_device,
#ifdef CONFIG_PLATFORM_FIA321
	&ftmac100_1_device,
#endif
	&ftrtc010_0_device,
};

/******************************************************************************
 * clock functions
 *****************************************************************************/
static struct clk_lookup a320_clk_lookups[] = {
	{
		.con_id = "cpuclk",
		.clk	= &ftpmu010_cpuclk,
	}, {
		.con_id	= "hclk",
		.clk	= &ftpmu010_hclk,
	}, {
		.con_id	= "pclk",
		.clk	= &ftpmu010_pclk,
	}, {
		.con_id	= "irda",
		.clk	= &ftpmu010_irda_clk,
	}, {
		.con_id	= "ssp",
		.clk	= &ftpmu010_ssp_clk,
	}, {
		.con_id	= "i2s",
		.clk	= &ftpmu010_i2s_clk,
	}, {
		.con_id	= "ac97:1",
		.clk	= &ftpmu010_ac97_clk1,
	}, {
		.con_id	= "ac97:2",
		.clk	= &ftpmu010_ac97_clk2,
	}, {
		.con_id	= "uart",
		.clk	= &ftpmu010_uart_clk,
	}, {
		.con_id	= "32768hz",
		.clk	= &ftpmu010_32768hz_clk,
	}
};

static int __init a320_clk_init(void)
{
	void __iomem *base = (void __iomem *)A320_FTPMU010_0_VA_BASE;

	ftpmu010_main_clk.rate = MAIN_CLK;
	ftpmu010_init(base);
	clkdev_add_table(a320_clk_lookups, ARRAY_SIZE(a320_clk_lookups));
	return 0;
}

/******************************************************************************
 * timer - clockevent and clocksource
 *****************************************************************************/
static struct fttmr010_clockevent fttmr010_0_clockevent = {
	.clockevent = {
		.name	= "fttmr010:0",
		.irq	= IRQ_A320_FTTMR010_0_T0,
	},
	.base	= (void __iomem *)A320_FTTMR010_0_VA_BASE,
	.id	= 0,
};

static struct fttmr010_clocksource fttmr010_1_clocksource = {
	.clocksource = {
		.name	= "fttmr010:1",
	},
	.base	= (void __iomem *)A320_FTTMR010_0_VA_BASE,
	.id	= 1,
};

static void __init a320_sys_timer_init(void)
{
	struct clk *clk;
	unsigned long cpuclk;
	unsigned long hclk;
	unsigned long pclk;

	clk = clk_get(NULL, "cpuclk");
	clk_enable(clk);
	cpuclk = clk_get_rate(clk);

	clk = clk_get(NULL, "hclk");
	clk_enable(clk);
	hclk = clk_get_rate(clk);

	clk = clk_get(NULL, "pclk");
	clk_enable(clk);
	pclk = clk_get_rate(clk);

	printk(KERN_INFO "CPU: %ld Hz, HCLK: %ld Hz, PCLK: %ld Hz\n", cpuclk, hclk, pclk);

#ifdef CONFIG_LOCAL_TIMERS
	twd_base = __io(PLATFORM_TWD_VA_BASE);
#endif

	fttmr010_0_clockevent.freq = pclk;
	fttmr010_clockevent_init(&fttmr010_0_clockevent);

	fttmr010_1_clocksource.freq = pclk;
	fttmr010_clocksource_init(&fttmr010_1_clocksource);
}

static struct sys_timer a320_sys_timer = {
	.init	= a320_sys_timer_init,
};

/******************************************************************************
 * platform dependent functions
 *****************************************************************************/
static struct map_desc a320_io_desc[] __initdata = {
	{
#ifdef CONFIG_CPU_FMP626
		/* SCU, GIC CPU and TWD */
		.virtual	= PLATFORM_SCU_VA_BASE,
		.pfn		= __phys_to_pfn(PLATFORM_SCU_PA_BASE),
		.length		= SZ_4K,
		.type		= MT_DEVICE,
	}, {	/* GIC DIST */
		.virtual	= PLATFORM_GIC_DIST_VA_BASE,
		.pfn		= __phys_to_pfn(PLATFORM_GIC_DIST_PA_BASE),
		.length		= SZ_4K,
		.type		= MT_DEVICE,
	}, {
#endif
		.virtual	= A320_FTINTC010_0_VA_BASE,
		.pfn		= __phys_to_pfn(A320_FTINTC010_0_PA_BASE),
		.length		= SZ_4K,
		.type		= MT_DEVICE,
	}, {
		.virtual	= A320_FTUART010_0_VA_BASE,
		.pfn		= __phys_to_pfn(A320_FTUART010_0_PA_BASE),
		.length		= SZ_4K,
		.type		= MT_DEVICE,
	}, {
		.virtual	= A320_FTUART010_1_VA_BASE,
		.pfn		= __phys_to_pfn(A320_FTUART010_1_PA_BASE),
		.length		= SZ_4K,
		.type		= MT_DEVICE,
	}, {
		.virtual	= A320_FTUART010_2_VA_BASE,
		.pfn		= __phys_to_pfn(A320_FTUART010_2_PA_BASE),
		.length		= SZ_4K,
		.type		= MT_DEVICE,
	}, {
		.virtual	= A320_FTTMR010_0_VA_BASE,
		.pfn		= __phys_to_pfn(A320_FTTMR010_0_PA_BASE),
		.length		= SZ_4K,
		.type		= MT_DEVICE,
	}, {
		.virtual	= A320_FTPMU010_0_VA_BASE,
		.pfn		= __phys_to_pfn(A320_FTPMU010_0_PA_BASE),
		.length		= SZ_4K,
		.type		= MT_DEVICE,
#ifdef CONFIG_PLATFORM_FIA321
	}, {
		.virtual	= A321_FTINTC010_0_VA_BASE,
		.pfn		= __phys_to_pfn(A321_FTINTC010_0_PA_BASE),
		.length		= SZ_4K,
		.type		= MT_DEVICE,
	}, {
		.virtual	= A321_FTPCI100_0_VA_BASE,
		.pfn		= __phys_to_pfn(A321_FTPCI100_0_PA_BASE),
		.length		= SZ_4K,
		.type		= MT_DEVICE,
#endif
	},
};

static void __init a320_map_io(void)
{
	iotable_init((struct map_desc*)a320_io_desc, ARRAY_SIZE(a320_io_desc));
	init_consistent_dma_size(SZ_8M);
	a320_clk_init();

#ifdef CONFIG_FTPCI100
	ftpci100_init(0, __io(A321_FTPCI100_0_VA_BASE));
#endif
}

static void __init a320_init_irq(void)
{
	/*
	 * initialize primary interrupt controller
	 */
#ifndef CONFIG_CPU_FMP626
	ftintc010_init(0, __io(A320_FTINTC010_0_VA_BASE), IRQ_A320_START);
#else
	gic_init(0, IRQ_LOCALTIMER, __io(PLATFORM_GIC_DIST_VA_BASE),__io(PLATFORM_GIC_CPU_VA_BASE));

	/*
	 * initialize second tier interrupt controller
	 */
	ftintc010_init(0, __io(A320_FTINTC010_0_VA_BASE), IRQ_A320_START);

	ftintc010_cascade_irq(0, PLATFORM_LEGACY_IRQ);
#endif

	ftintc010_set_irq_type(IRQ_A320_FTGPIO010_0, IRQ_TYPE_EDGE_RISING);
	ftintc010_set_irq_type(IRQ_A320_FTTMR010_0_T1, IRQ_TYPE_EDGE_RISING);
	ftintc010_set_irq_type(IRQ_A320_FTTMR010_0_T2, IRQ_TYPE_EDGE_RISING);
	ftintc010_set_irq_type(IRQ_A320_FTRTC010_0_ALRM, IRQ_TYPE_EDGE_RISING);
	ftintc010_set_irq_type(IRQ_A320_FTRTC010_0_SEC, IRQ_TYPE_EDGE_RISING);
	ftintc010_set_irq_type(IRQ_A320_FTTMR010_0_T0, IRQ_TYPE_EDGE_RISING);

#ifdef CONFIG_PLATFORM_FIA321
	/*
	 * initialize second tier interrupt controller
	 */
	ftintc010_init(1, __io(A321_FTINTC010_0_VA_BASE), IRQ_A321_START);

	ftintc010_cascade_irq(1, IRQ_A320_A321);

	ftintc010_set_irq_type(IRQ_A321_FTKBC010_0, IRQ_TYPE_EDGE_RISING);
	ftintc010_set_irq_type(IRQ_A321_FTKBC010_1, IRQ_TYPE_EDGE_RISING);

#ifdef CONFIG_FTPCI100
	/*
	 * initialize third tier interrupt controller
	 * because all 4 irqs of FTPCI100 are connected to the same pin of A321
	 * we are compelled to make FTPCI100 be a third tier interrupt controller
	 */
	ftpci100_int_init(0, IRQ_FTPCI100_0_START);

	ftpci100_int_cascade_irq(0, IRQ_A321_FTPCI100_0);
#endif
#endif
}

static void __init a320_board_init(void)
{
	platform_add_devices(a320_devices, ARRAY_SIZE(a320_devices));
}

MACHINE_START(FARADAY, "A320")
	.atag_offset	= 0x100,
	.map_io		= a320_map_io,
	.init_irq	= a320_init_irq,
	.timer		= &a320_sys_timer,
	.init_machine	= a320_board_init,
#ifdef CONFIG_CPU_FMP626
	.handle_irq	= gic_handle_irq,
#endif
MACHINE_END
