/*
 *  linux/arch/arm/mach-faraday/board-axi.c
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

#include <linux/platform_device.h>

#include <asm/mach-types.h>

#include <asm/mach/arch.h>
#include <asm/mach/map.h>
#include <asm/mach/time.h>

#include <mach/board-axi.h>
#include <mach/fttmr010.h>
#include <mach/ftintc010.h>

/******************************************************************************
 * timer - clockevent and clocksource
 *****************************************************************************/
static struct fttmr010_clockevent fttmr010_0_clockevent = {
	.clockevent = {
		.name	= "fttmr010:0",
		.irq	= IRQ_AXI_FTTMR010_0_T0,
	},
	.base	= (void __iomem *)AXI_FTTMR010_0_VA_BASE,
	.id	= 0,
};

static struct fttmr010_clocksource fttmr010_1_clocksource = {
	.clocksource = {
		.name	= "fttmr010:1",
	},
	.base	= (void __iomem *)AXI_FTTMR010_0_VA_BASE,
	.id	= 1,
};

static void __init axi_sys_timer_init(void)
{
	fttmr010_0_clockevent.freq = CLOCK_TICK_RATE;
	fttmr010_clockevent_init(&fttmr010_0_clockevent);

	fttmr010_1_clocksource.freq = CLOCK_TICK_RATE;
	fttmr010_clocksource_init(&fttmr010_1_clocksource);
}

static struct sys_timer axi_sys_timer = {
	.init	= axi_sys_timer_init,
};

/******************************************************************************
 * platform dependent functions
 *****************************************************************************/
static struct map_desc axi_io_desc[] __initdata = {
	{
		.virtual	= AXI_FTINTC010_0_VA_BASE,
		.pfn		= __phys_to_pfn(AXI_FTINTC010_0_PA_BASE),
		.length		= SZ_4K,
		.type		= MT_DEVICE,
	}, {
		.virtual	= AXI_FTUART010_0_VA_BASE,
		.pfn		= __phys_to_pfn(AXI_FTUART010_0_PA_BASE),
		.length		= SZ_4K,
		.type		= MT_DEVICE,
	}, {
		.virtual	= AXI_FTUART010_1_VA_BASE,
		.pfn		= __phys_to_pfn(AXI_FTUART010_1_PA_BASE),
		.length		= SZ_4K,
		.type		= MT_DEVICE,
	}, {
		.virtual	= AXI_FTTMR010_0_VA_BASE,
		.pfn		= __phys_to_pfn(AXI_FTTMR010_0_PA_BASE),
		.length		= SZ_4K,
		.type		= MT_DEVICE,
	},
};

static void __init axi_map_io(void)
{
	iotable_init((struct map_desc*)axi_io_desc, ARRAY_SIZE(axi_io_desc));
}

static void __init axi_init_irq(void)
{
	/*
	 * initialize primary interrupt controller
	 */
	ftintc010_init(0, __io(AXI_FTINTC010_0_VA_BASE), IRQ_AXI_START);

	ftintc010_set_irq_type(IRQ_AXI_FTTMR010_0_T0, IRQ_TYPE_EDGE_RISING);
	ftintc010_set_irq_type(IRQ_AXI_FTTMR010_0_T1, IRQ_TYPE_EDGE_RISING);
	ftintc010_set_irq_type(IRQ_AXI_FTTMR010_0_T2, IRQ_TYPE_EDGE_RISING);
}

MACHINE_START(FARADAY, "AXI")
	.boot_params	= PHYS_OFFSET + 0x00000100,
	.map_io		= axi_map_io,
	.init_irq	= axi_init_irq,
	.timer		= &axi_sys_timer,
MACHINE_END
