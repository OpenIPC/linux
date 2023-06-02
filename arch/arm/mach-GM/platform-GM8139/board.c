/*
 *  board depdenent initialization
 *
 *  Copyright (C) 2009 Faraday Corp. (http://www.faraday-tech.com)
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
#include <asm/mach/arch.h>
#include <asm/mach/map.h>
#include <asm/mach/time.h>
#include <asm/mach-types.h>
#include <asm/sizes.h>
#include <asm/setup.h>
#include <mach/platform/board.h>
#include <mach/platform/pmu.h>
#include <mach/platform/platform_io.h>
#include <mach/platform/system.h>
#include <mach/ftintc030.h>
#include <mach/fttmr010.h>
#include <mach/fmem.h>
#include <linux/dma-mapping.h>

#ifdef CONFIG_CACHE_FTL2CC031
#include <asm/hardware/cache-ftl2cc031.h>
#endif

/* External functions and variables declaration
 */
extern void platform_devices_init(void);

/* Local variables declaration
 */
void __iomem *ftintc030_base_addr;
void __iomem *ftintc030_base_cpu_0_irq_base; //entry-macro.S will reference it

/******************************************************************************
 * IPs virtual address mapping
 *****************************************************************************/
static struct map_desc board_io_desc[] __initdata = {
    /* PMU */
    {
        .virtual	= PMU_FTPMU010_VA_BASE,
		.pfn		= __phys_to_pfn(PMU_FTPMU010_PA_BASE),
		.length		= PMU_FTPMU010_VA_SIZE,
		.type		= MT_DEVICE,
    },
    /* UART0 */
    {
        .virtual	= UART_FTUART010_0_VA_BASE,
		.pfn		= __phys_to_pfn(UART_FTUART010_0_PA_BASE),
		.length		= UART_FTUART010_0_VA_SIZE,
		.type		= MT_DEVICE,
    },
    /* UART1 */
    {
        .virtual	= UART_FTUART010_1_VA_BASE,
		.pfn		= __phys_to_pfn(UART_FTUART010_1_PA_BASE),
		.length		= UART_FTUART010_1_VA_SIZE,
		.type		= MT_DEVICE,
    },
    /* UART2 */
    {
        .virtual	= UART_FTUART010_2_VA_BASE,
		.pfn		= __phys_to_pfn(UART_FTUART010_2_PA_BASE),
		.length		= UART_FTUART010_2_VA_SIZE,
		.type		= MT_DEVICE,
    },
    /* TIMER */
    {
        .virtual	= TIMER_FTTMR010_VA_BASE,
		.pfn		= __phys_to_pfn(TIMER_FTTMR010_PA_BASE),
		.length		= TIMER_FTTMR010_VA_SIZE,
		.type		= MT_DEVICE,
    },
    /* INTC030 */
    {
        .virtual	= INTC_FTINTC030_VA_BASE,
		.pfn		= __phys_to_pfn(INTC_FTINTC030_PA_BASE),
		.length		= INTC_FTINTC030_VA_SIZE,
		.type		= MT_DEVICE,
    },
#ifdef CONFIG_CACHE_FTL2CC031
    /* L2 cache */
	{
		.virtual	= L2CACHE_FTL2CC031_VA_BASE,
		.pfn		= __phys_to_pfn(L2CACHE_FTL2CC031_PA_BASE),
		.length		= L2CACHE_FTL2CC031_VA_SIZE,
		.type		= MT_DEVICE,
	},
#endif
};

/* fixup the memory for MACHINE_START. The parameters come from commandline of menuconfig
 */
static void __init board_fixup_memory(struct tag *tag, char **cmdline, struct meminfo *mi)
{
    fmem_fixup_memory(tag, cmdline, mi);
}

/* 2. create the iotable for IPs in MACHINE_START
 */
static void __init board_map_io(void)
{
    iotable_init(&board_io_desc[0], ARRAY_SIZE(board_io_desc));

    /* set the base to pmu module */
    pmu_earlyinit((void *)PMU_FTPMU010_VA_BASE);

    /* init dma coherent mapping size,
     * CONSISTENT_DMA_SIZE is defined in memory.h
     * max allowable dma consistent size is 14M (0xFFE0_0000 - 0xFF00_00000)
     * set as 12M to avoid unexpected data overlay, NISH_20121015
     */
    #ifdef CONSISTENT_DMA_SIZE
    init_consistent_dma_size(CONSISTENT_DMA_SIZE);
    #endif
}

static void __init board_init_irq(void)
{
	struct ftintc030_trigger_type trigger_type = {
		.irqmode[0]	= PLATFORM_IRQ_TRIGGER_MODE1,
		.irqlevel[0]	= ~PLATFORM_IRQ_TRIGGER_LEVEL1,
		.fiqmode[0]	= PLATFORM_FIQ_TRIGGER_MODE1,
		.fiqlevel[0]	= ~PLATFORM_FIQ_TRIGGER_LEVEL1,

		.irqmode[1]	= PLATFORM_IRQ_TRIGGER_MODE2,
		.irqlevel[1]	= ~PLATFORM_IRQ_TRIGGER_LEVEL2,
		.fiqmode[1]	= PLATFORM_FIQ_TRIGGER_MODE2,
		.fiqlevel[1]	= ~PLATFORM_FIQ_TRIGGER_LEVEL2,

		.irqmode[2]	= PLATFORM_IRQ_TRIGGER_MODE3,
		.irqlevel[2]	= ~PLATFORM_IRQ_TRIGGER_LEVEL3,
		.fiqmode[2]	= PLATFORM_FIQ_TRIGGER_MODE3,
		.fiqlevel[2]	= ~PLATFORM_FIQ_TRIGGER_LEVEL3,

		.irqmode[3]	= PLATFORM_IRQ_TRIGGER_MODE4,
		.irqlevel[3]	= ~PLATFORM_IRQ_TRIGGER_LEVEL4,
		.fiqmode[3]	= PLATFORM_FIQ_TRIGGER_MODE4,
		.fiqlevel[3]	= ~PLATFORM_FIQ_TRIGGER_LEVEL4,

		.irqmode[4]	= PLATFORM_IRQ_TRIGGER_MODE5,
		.irqlevel[4]	= ~PLATFORM_IRQ_TRIGGER_LEVEL5,
		.fiqmode[4]	= PLATFORM_FIQ_TRIGGER_MODE5,
		.fiqlevel[4]	= ~PLATFORM_FIQ_TRIGGER_LEVEL5,

		.irqmode[5]	= PLATFORM_IRQ_TRIGGER_MODE6,
		.irqlevel[5]	= ~PLATFORM_IRQ_TRIGGER_LEVEL6,
		.fiqmode[5]	= PLATFORM_FIQ_TRIGGER_MODE6,
		.fiqlevel[5]	= ~PLATFORM_FIQ_TRIGGER_LEVEL6,

		.irqmode[6]	= PLATFORM_IRQ_TRIGGER_MODE7,
		.irqlevel[6]	= ~PLATFORM_IRQ_TRIGGER_LEVEL7,
		.fiqmode[6]	= PLATFORM_FIQ_TRIGGER_MODE7,
		.fiqlevel[6]	= ~PLATFORM_FIQ_TRIGGER_LEVEL7,

		.irqmode[7]	= PLATFORM_IRQ_TRIGGER_MODE8,
		.irqlevel[7]	= ~PLATFORM_IRQ_TRIGGER_LEVEL8,
		.fiqmode[7]	= PLATFORM_FIQ_TRIGGER_MODE8,
		.fiqlevel[7]	= ~PLATFORM_FIQ_TRIGGER_LEVEL8,
	};

	/*
	 * initialize primary interrupt controller
	 */
	ftintc030_base_addr = __io(INTC_FTINTC030_VA_BASE);
	ftintc030_base_cpu_0_irq_base = __io(INTC_FTINTC030_VA_BASE + FTINTC030_OFFSET_CPU_0_IRQ);

	ftintc030_init(0, ftintc030_base_addr, 0, &trigger_type);
}

/******************************************************************************
 * timer - clockevent and clocksource
 *****************************************************************************/
static struct fttmr010_clockevent fttmr010_0_clockevent = {
	.clockevent = {
		.name	= "fttmr010:0",
		.irq	= TIMER_FTTMR010_IRQ0,
	},
	.base	= (void __iomem *)TIMER_FTTMR010_VA_BASE,
	.id	= 0,
};

static struct fttmr010_clocksource fttmr010_1_clocksource = {
	.clocksource = {
		.name	= "fttmr010:1",
	},
	.base	= (void __iomem *)TIMER_FTTMR010_VA_BASE,
	.id	= 1,
};

static void __init board_sys_timer_init(void)
{
    unsigned int pclk = pmu_get_apb_clk();

	fttmr010_0_clockevent.freq = pclk;
	fttmr010_clockevent_init(&fttmr010_0_clockevent);

	fttmr010_1_clocksource.freq = pclk;
	fttmr010_clocksource_init(&fttmr010_1_clocksource);
}

struct sys_timer board_sys_timer = {
	.init	= board_sys_timer_init,
};

/* board init */
static void __init board_init_machine(void)
{
    int i;

    platform_devices_init();

    /* dump iotable information
     */
    for (i = 0; i < ARRAY_SIZE(board_io_desc); i ++) {
        printk("iotable: VA: 0x%x, PA: 0x%x, Length: %d \n",
                (u32)board_io_desc[i].virtual,
                (u32)board_io_desc[i].pfn << PAGE_SHIFT,
                (u32)board_io_desc[i].length);
    }

#ifdef CONFIG_CACHE_FTL2CC031
    /* 256 KB (32KB/way), 8-way associativity, parity enabled */
	ftl2cc031_init(__io(L2CACHE_FTL2CC031_VA_BASE), (AUX_WAYSIZE(WAYSIZE_32KB) | AUX_PAR_CHK_EN | 0x23), 0xF3777);
#endif
}

MACHINE_START(GM, BOARD_NAME)
	.atag_offset	= BOOT_PARAMETER_PA_OFFSET,   //boot command line, after kernel 3.2 change as relative address
	.map_io		    = board_map_io,
	.init_irq	    = board_init_irq,
	.timer		    = &board_sys_timer,
	.fixup          = board_fixup_memory,
	.init_machine	= board_init_machine,
	.restart	= arch_reset,
MACHINE_END
