/*
 *  linux/arch/arm/mach-faraday/board-a369.c
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
#include <linux/i2c.h>
#include <linux/platform_device.h>
#include <linux/input/matrix_keypad.h>
#include <linux/export.h>

#include <asm/mach-types.h>
#ifdef CONFIG_CPU_FMP626
#include <asm/localtimer.h>
#include <asm/hardware/gic.h>
#include <asm/pmu.h>
#endif

#include <asm/mach/arch.h>
#include <asm/mach/map.h>
#include <asm/mach/time.h>

#include <mach/board-a369.h>
#include <mach/dma-a369.h>
#include <mach/ftahbb020.h>
#include <mach/ftintc010.h>
#include <mach/ftpwmtmr010.h>
#include <mach/ftscu010.h>

#include "clock.h"
#ifdef CONFIG_CPU_FMP626
#include "core.h"
#endif

/******************************************************************************
 * platform keypad config
 *****************************************************************************/
static const unsigned int default_keymap[] = {
	/* KEY(row, col, keycode) */
	KEY(0, 0, KEY_BACK),	KEY(0, 1, KEY_UP),	KEY(0, 2, KEY_HOME),	/* PB1 - PB3 */
	KEY(1, 0, KEY_LEFT),	KEY(1, 1, KEY_MENU),	KEY(1, 2, KEY_RIGHT),	/* PB4 - PB6 */
	KEY(2, 0, KEY_SLEEP),	KEY(2, 1, KEY_DOWN),	KEY(2, 2, KEY_WAKEUP),	/* PB7 - PB9 */
};

static const struct matrix_keymap_data default_keymap_data = {
	.keymap = default_keymap,
	.keymap_size = ARRAY_SIZE(default_keymap),
};

static struct matrix_keypad_platform_data a369_keypad_config = {
	.keymap_data = &default_keymap_data,
	.num_row_gpios = 3,
	.num_col_gpios = 3,
};


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
		.start	= A369_FTAPBB020_0_PA_BASE,
		.end	= A369_FTAPBB020_0_PA_BASE + SZ_4K - 1,
		.flags	= IORESOURCE_MEM,
	}, {
		.start	= IRQ_A369_FTAPBB020_0,
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
 * FTDMAC020
 */
static struct resource ftdmac020_0_resources[] = {
	{
		.start	= A369_FTDMAC020_0_PA_BASE,
		.end	= A369_FTDMAC020_0_PA_BASE + SZ_4K - 1,
		.flags	= IORESOURCE_MEM,
	}, {
		.start	= IRQ_A369_FTDMAC020_0_TC,
		.flags	= IORESOURCE_IRQ,
	}, {
		.start	= IRQ_A369_FTDMAC020_0_ERR,
		.flags	= IORESOURCE_IRQ,
	},
};

static struct platform_device ftdmac020_0_device = {
	.name		= "ftdmac020",
	.id		= 0,
	.dev		= {
		.coherent_dma_mask	= DMA_BIT_MASK(32),
	},
	.num_resources	= ARRAY_SIZE(ftdmac020_0_resources),
	.resource	= ftdmac020_0_resources,
};

static struct resource ftdmac020_1_resources[] = {
	{
		.start	= A369_FTDMAC020_1_PA_BASE,
		.end	= A369_FTDMAC020_1_PA_BASE + SZ_4K - 1,
		.flags	= IORESOURCE_MEM,
	}, {
		.start	= IRQ_A369_FTDMAC020_1_TC,
		.flags	= IORESOURCE_IRQ,
	}, {
		.start	= IRQ_A369_FTDMAC020_1_ERR,
		.flags	= IORESOURCE_IRQ,
	},
};

static struct platform_device ftdmac020_1_device = {
	.name		= "ftdmac020",
	.id		= 1,
	.dev		= {
		.coherent_dma_mask	= DMA_BIT_MASK(32),
	},
	.num_resources	= ARRAY_SIZE(ftdmac020_1_resources),
	.resource	= ftdmac020_1_resources,
};

/*
 * FTGMAC100
 */
static struct resource ftgmac100_0_resources[] = {
	{
		.start	= A369_FTGMAC100_0_PA_BASE,
		.end	= A369_FTGMAC100_0_PA_BASE + SZ_4K - 1,
		.flags	= IORESOURCE_MEM,
	}, {
		.start	= IRQ_A369_FTGMAC100_0_IRQ,
		.flags	= IORESOURCE_IRQ,
	},
};

static struct platform_device ftgmac100_0_device = {
	.name		= "ftgmac100",
	.id		= 0,
	.dev		= {
		.coherent_dma_mask	= DMA_BIT_MASK(32),
	},
	.num_resources	= ARRAY_SIZE(ftgmac100_0_resources),
	.resource	= ftgmac100_0_resources,
};

/*
 * FTIIC010
 */
static struct resource ftiic010_0_resources[] = {
	{
		.start	= A369_FTIIC010_0_PA_BASE,
		.end	= A369_FTIIC010_0_PA_BASE + SZ_4K - 1,
		.flags	= IORESOURCE_MEM,
	}, {
		.start	= IRQ_A369_FTIIC010_0,
		.flags	= IORESOURCE_IRQ,
	},
};

static struct platform_device ftiic010_0_device = {
	.name		= "ftiic010",
	.id		= 0,
	.num_resources	= ARRAY_SIZE(ftiic010_0_resources),
	.resource	= ftiic010_0_resources,
};

static struct resource ftiic010_1_resources[] = {
	{
		.start	= A369_FTIIC010_1_PA_BASE,
		.end	= A369_FTIIC010_1_PA_BASE + SZ_4K - 1,
		.flags	= IORESOURCE_MEM,
	}, {
		.start	= IRQ_A369_FTIIC010_1,
		.flags	= IORESOURCE_IRQ,
	},
};

static struct platform_device ftiic010_1_device = {
	.name		= "ftiic010",
	.id		= 1,
	.num_resources	= ARRAY_SIZE(ftiic010_1_resources),
	.resource	= ftiic010_1_resources,
};

/*
 * FTKBC010
 */
static struct resource ftkbc010_0_resources[] = {
	{
		.start	= A369_FTKBC010_0_PA_BASE,
		.end	= A369_FTKBC010_0_PA_BASE + SZ_4K - 1,
		.flags	= IORESOURCE_MEM,
	}, {
		.start	= IRQ_A369_FTKBC010_0,
		.flags	= IORESOURCE_IRQ,
	},
};

static struct platform_device ftkbc010_0_device = {
	.name		= "ftkbc010",
	.id		= 0,
	.num_resources	= ARRAY_SIZE(ftkbc010_0_resources),
	.resource	= ftkbc010_0_resources,
	.dev		= {
		.platform_data = &a369_keypad_config,
	}
};

/*
 * FTLCDC200
 */
static struct resource ftlcdc200_0_resources[] = {
	{
		.start	= A369_FTLCDC200_0_PA_BASE,
		.end	= A369_FTLCDC200_0_PA_BASE + SZ_64K - 1,
		.flags	= IORESOURCE_MEM,
	}, {	/* bus error */
		.start	= IRQ_A369_FTLCDC200_0_MERR,
		.flags	= IORESOURCE_IRQ,
	}, {	/* FIFO underrun */
		.start	= IRQ_A369_FTLCDC200_0_FUR,
		.flags	= IORESOURCE_IRQ,
	}, {	/* base address update */
		.start	= IRQ_A369_FTLCDC200_0_BAUPD,
		.flags	= IORESOURCE_IRQ,
	}, {	/* vertical status */
		.start	= IRQ_A369_FTLCDC200_0_VSTATUS,
		.flags	= IORESOURCE_IRQ,
	},
};

static struct platform_device ftlcdc200_0_device = {
	.name		= "ftlcdc200",
	.id		= 0,
	.dev		= {
		.coherent_dma_mask	= DMA_BIT_MASK(32),
	},
	.num_resources	= ARRAY_SIZE(ftlcdc200_0_resources),
	.resource	= ftlcdc200_0_resources,
};

/*
 * FTRTC011
 */
static struct resource ftrtc011_0_resources[] = {
	{
		.start	= A369_FTRTC011_0_PA_BASE,
		.end	= A369_FTRTC011_0_PA_BASE + SZ_4K - 1,
		.flags	= IORESOURCE_MEM,
	}, {	/* alarm interrupt */
		.start	= IRQ_A369_FTRTC011_0_ALRM,
		.flags	= IORESOURCE_IRQ,
	}, {	/* periodic interrupt */
		.start	= IRQ_A369_FTRTC011_0_SEC,
		.flags	= IORESOURCE_IRQ,
	},
};

static struct platform_device ftrtc011_0_device = {
	.name		= "ftrtc011",
	.id		= 0,
	.num_resources	= ARRAY_SIZE(ftrtc011_0_resources),
	.resource	= ftrtc011_0_resources,
};

/*
 * FTSDC010
 */
static struct resource ftsdc010_0_resources[] = {
	{
		.start	= A369_FTSDC010_0_PA_BASE,
		.end	= A369_FTSDC010_0_PA_BASE + SZ_4K - 1,
		.flags	= IORESOURCE_MEM,
	}, {
		.start	= IRQ_A369_FTSDC010_0,
		.flags	= IORESOURCE_IRQ,
	}
};

static struct platform_device ftsdc010_0_device = {
	.name		= "ftsdc010",
	.id		= 0,
	.dev		= {
		.coherent_dma_mask	= DMA_BIT_MASK(32),
	},
	.num_resources	= ARRAY_SIZE(ftsdc010_0_resources),
	.resource	= ftsdc010_0_resources,
};

static struct resource ftsdc010_1_resources[] = {
	{
		.start	= A369_FTSDC010_1_PA_BASE,
		.end	= A369_FTSDC010_1_PA_BASE + SZ_4K - 1,
		.flags	= IORESOURCE_MEM,
	}, {
		.start	= IRQ_A369_FTSDC010_1,
		.flags	= IORESOURCE_IRQ,
	}
};

static struct platform_device ftsdc010_1_device = {
	.name		= "ftsdc010",
	.id		= 1,
	.dev		= {
		.coherent_dma_mask	= DMA_BIT_MASK(32),
	},
	.num_resources	= ARRAY_SIZE(ftsdc010_1_resources),
	.resource	= ftsdc010_1_resources,
};

/*
 * FTSSP010
 */
static struct resource ftssp010_0_i2s_resources[] = {
	{
		.start	= A369_FTSSP010_0_PA_BASE,
		.end	= A369_FTSSP010_0_PA_BASE + SZ_4K - 1,
		.flags	= IORESOURCE_MEM,
	}, {
		.start	= IRQ_A369_FTSSP010_0,
		.flags	= IORESOURCE_IRQ,
	},
};

static struct platform_device ftssp010_0_i2s_device = {
	.name		= "ftssp010-i2s",
	.id		= 0,
	.num_resources	= ARRAY_SIZE(ftssp010_0_i2s_resources),
	.resource	= ftssp010_0_i2s_resources,
};

/*
 * FTTSC010
 *
 * Note: Sytlus interrupt of FTTSC010 on A369 is broken.
 */
static struct resource fttsc010_0_resources[] = {
	{
		.start	= A369_FTTSC010_0_PA_BASE,
		.end	= A369_FTTSC010_0_PA_BASE + SZ_4K - 1,
		.flags	= IORESOURCE_MEM,
	}, {	/* ADC interrupt */
		.start	= IRQ_A369_FTTSC010_0_ADC,
		.flags	= IORESOURCE_IRQ,
	}, {	/* stylus interrupt */
		.start	= IRQ_A369_FTTSC010_0_PANEL,
		.flags	= IORESOURCE_IRQ,
	},
};

static struct platform_device fttsc010_0_device = {
	.name		= "fttsc010",
	.id		= 0,
	.num_resources	= ARRAY_SIZE(fttsc010_0_resources),
	.resource	= fttsc010_0_resources,
};

static struct platform_device *a369_devices[] __initdata = {
#ifdef CONFIG_CPU_FMP626
	&pmu_device,
#endif
	&ftapbb020_0_device,
	&ftdmac020_0_device,
	&ftdmac020_1_device,
	&ftgmac100_0_device,
	&ftkbc010_0_device,
	&ftiic010_0_device,
	&ftiic010_1_device,
	&ftlcdc200_0_device,
	&ftrtc011_0_device,
	&ftsdc010_0_device,
	&ftsdc010_1_device,
	&ftssp010_0_i2s_device,
	&fttsc010_0_device,
};

/******************************************************************************
 * I2C devices
 *****************************************************************************/
static struct i2c_board_info __initdata a369_i2c_devices[] = {
	{ I2C_BOARD_INFO("wm8731", 0x1b), },
};

/******************************************************************************
 * clock functions
 *****************************************************************************/
static struct clk_lookup a369_clk_lookups[] = {
	{
		.con_id	= "dclk",
		.clk	= &ftscu010_dclk,
	}, {
		.con_id	= "mclk",
		.clk	= &ftscu010_mclk,
	}, {
		.con_id	= "hclk",
		.clk	= &ftscu010_hclk,
	}, {
		.con_id	= "pclk",
		.clk	= &ftscu010_pclk,
	}, {
		.con_id = "extahb",
		.clk	= &ftscu010_extahb_clk,
	}, {
		.con_id	= "mcpu_clk",
		.clk	= &ftscu010_mcpu_clk,
	}, {
		.con_id = "ssp0-extclk",
		.clk	= &ftscu010_ssp0_extclk,
	}, {
		.con_id = "tsc",
		.clk	= &ftscu010_tsc_clk,
	},
};

static int __init a369_clk_init(void)
{
	void __iomem *base = (void __iomem *)A369_FTSCU010_0_VA_BASE;

	ftscu010_main_clk.rate = MAIN_CLK;
	ftscu010_init(base);
	clkdev_add_table(a369_clk_lookups, ARRAY_SIZE(a369_clk_lookups));
	return 0;
}

/******************************************************************************
 * timer - clockevent and clocksource
 *****************************************************************************/
static struct ftpwmtmr010_clockevent ftpwmtmr010_0_clockevent = {
	.clockevent = {
		.name	= "ftpwmtmr010:0",
		.irq	= IRQ_A369_FTPWMTMR010_0_T0,
	},
	.base	= (void __iomem *)A369_FTPWMTMR010_0_VA_BASE,
	.id	= 0,
};

static struct ftpwmtmr010_clocksource ftpwmtmr010_1_clocksource = {
	.clocksource = {
		.name	= "ftpwmtmr010:1",
	},
	.base	= (void __iomem *)A369_FTPWMTMR010_0_VA_BASE,
	.id	= 1,
};

static void __init a369_sys_timer_init(void)
{
	struct clk *clk;
	unsigned long cpuclk;
	unsigned long hclk;
	unsigned long pclk;
	unsigned long extahbclk;

	clk = clk_get(NULL, "mcpu_clk");
	clk_enable(clk);
	cpuclk = clk_get_rate(clk);

	clk = clk_get(NULL, "hclk");
	clk_enable(clk);
	hclk = clk_get_rate(clk);

	clk = clk_get(NULL, "pclk");
	clk_enable(clk);
	pclk = clk_get_rate(clk);

	clk = clk_get(NULL, "extahb");
	clk_enable(clk);
	extahbclk = clk_get_rate(clk);

	printk(KERN_INFO "CPU: %ld Hz, HCLK: %ld Hz, PCLK: %ld Hz, ExtAHB: %ld\n", cpuclk, hclk, pclk, extahbclk);

#ifdef CONFIG_LOCAL_TIMERS
	twd_base = __io(PLATFORM_TWD_VA_BASE);
#endif

	ftpwmtmr010_0_clockevent.freq = pclk;
	ftpwmtmr010_clockevent_init(&ftpwmtmr010_0_clockevent);

	ftpwmtmr010_1_clocksource.freq = pclk;
	ftpwmtmr010_clocksource_init(&ftpwmtmr010_1_clocksource);
}

static struct sys_timer a369_sys_timer = {
	.init	= a369_sys_timer_init,
};

/******************************************************************************
 * DMA function
 *****************************************************************************/

int a369_dmac_handshake_alloc(const char *name)
{
	return ftscu010_handshake_alloc(name);
}
EXPORT_SYMBOL(a369_dmac_handshake_alloc);

int a369_dmac_handshake_setup(unsigned int handshake, int which)
{
	return ftscu010_handshake_setup(handshake, which);
}
EXPORT_SYMBOL(a369_dmac_handshake_setup);

int a369_dmac_handshake_free(unsigned int handshake)
{
	return ftscu010_handshake_free(handshake);
}
EXPORT_SYMBOL(a369_dmac_handshake_free);

/******************************************************************************
 * platform dependent functions
 *****************************************************************************/

static struct map_desc a369_io_desc[] __initdata = {
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
		.virtual	= A369_FTINTC020_0_VA_BASE,
		.pfn		= __phys_to_pfn(A369_FTINTC020_0_PA_BASE),
		.length		= SZ_4K,
		.type		= MT_DEVICE,
	}, {
		.virtual	= A369_FTUART010_0_VA_BASE,
		.pfn		= __phys_to_pfn(A369_FTUART010_0_PA_BASE),
		.length		= SZ_4K,
		.type		= MT_DEVICE,
	}, {
		.virtual	= A369_FTUART010_1_VA_BASE,
		.pfn		= __phys_to_pfn(A369_FTUART010_1_PA_BASE),
		.length		= SZ_4K,
		.type		= MT_DEVICE,
	}, {
		.virtual	= A369_FTUART010_2_VA_BASE,
		.pfn		= __phys_to_pfn(A369_FTUART010_2_PA_BASE),
		.length		= SZ_4K,
		.type		= MT_DEVICE,
	}, {
		.virtual	= A369_FTUART010_3_VA_BASE,
		.pfn		= __phys_to_pfn(A369_FTUART010_3_PA_BASE),
		.length		= SZ_4K,
		.type		= MT_DEVICE,
	}, {
		.virtual	= A369_FTPWMTMR010_0_VA_BASE,
		.pfn		= __phys_to_pfn(A369_FTPWMTMR010_0_PA_BASE),
		.length		= SZ_4K,
		.type		= MT_DEVICE,
	}, {
		.virtual	= A369_FTSCU010_0_VA_BASE,
		.pfn		= __phys_to_pfn(A369_FTSCU010_0_PA_BASE),
		.length		= SZ_4K,
		.type		= MT_DEVICE,
	}, {
		.virtual	= A369_FTAHBB020_3_VA_BASE,
		.pfn		= __phys_to_pfn(A369_FTAHBB020_3_PA_BASE),
		.length		= SZ_4K,
		.type		= MT_DEVICE,
	},
};

static void __init a369_map_io(void)
{
	iotable_init((struct map_desc*)a369_io_desc, ARRAY_SIZE(a369_io_desc));
	init_consistent_dma_size(SZ_8M);
	a369_clk_init();
}

static void __init a369_init_irq(void)
{
	/*
	 * initialize primary interrupt controller
	 */
#ifndef CONFIG_CPU_FMP626
	ftintc010_init(0, __io(A369_FTINTC020_0_VA_BASE), IRQ_A369_START);
#else
	gic_init(0, IRQ_LOCALTIMER, __io(PLATFORM_GIC_DIST_VA_BASE),__io(PLATFORM_GIC_CPU_VA_BASE));

	/*
	 * initialize second tier interrupt controller
	 */
	ftintc010_init(0, __io(A369_FTINTC020_0_VA_BASE), IRQ_A369_START);

	ftintc010_cascade_irq(0, PLATFORM_LEGACY_IRQ);
#endif

	/*
	 * initialize second tier interrupt controller
	 */
	ftahbb020_init(0, __io(A369_FTAHBB020_3_VA_BASE), IRQ_FTAHBB020_3_START);

	ftahbb020_cascade_irq(0, IRQ_A369_FTAHBB020_3);

	ftahbb020_set_irq_type(IRQ_FTAHBB020_3_EXINT(0), IRQ_TYPE_LEVEL_LOW);
	ftahbb020_set_irq_type(IRQ_FTAHBB020_3_EXINT(1), IRQ_TYPE_LEVEL_LOW);
}

static void __init a369_board_init(void)
{
	/* default pin mux setting */
	ftscu010_pinmux_setup("lcd", "lcd_mice");

	i2c_register_board_info(0, a369_i2c_devices, ARRAY_SIZE(a369_i2c_devices));
	platform_add_devices(a369_devices, ARRAY_SIZE(a369_devices));
}

MACHINE_START(FARADAY, "A369")
	.atag_offset	= 0x100,
	.map_io		= a369_map_io,
	.init_irq	= a369_init_irq,
	.timer		= &a369_sys_timer,
	.init_machine	= a369_board_init,
#ifdef CONFIG_CPU_FMP626
	.handle_irq     = gic_handle_irq,
#endif
MACHINE_END
