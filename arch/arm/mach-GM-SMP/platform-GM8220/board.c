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
#include <asm/arch_timer.h>
#include <mach/platform/board.h>
#include <mach/platform/pmu.h>
#include <mach/platform/platform_io.h>
#include <mach/platform/system.h>
//#include <mach/ftintc030.h>
#include <mach/fttmr010.h>
//#include <mach/arm_global_timer.h>
#include <mach/fmem.h>
#include <linux/dma-mapping.h>

#ifdef CONFIG_CPU_HAS_GIC
#include <asm/hardware/gic.h>
#endif

/* External functions and variables declaration
 */
extern void platform_devices_init(void);

/* Local variables declaration
 */
//void __iomem *ftintc030_base_addr;
//void __iomem *ftintc030_base_cpu_0_irq_base; //entry-macro.S will reference it

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
    /* GIC DIST */
    {    
        .virtual    = PLATFORM_GIC_DIST_VA_BASE,
        .pfn        = __phys_to_pfn(PLATFORM_GIC_DIST_PA_BASE),
        .length     = PLATFORM_GIC_DIST_VA_SIZE,
        .type       = MT_DEVICE,
    },
    { /* GIC CPU */
        .virtual    = PLATFORM_GIC_CPU_VA_BASE,
        .pfn        = __phys_to_pfn(PLATFORM_GIC_CPU_PA_BASE),
        .length     = PLATFORM_GIC_CPU_VA_SIZE,
        .type       = MT_DEVICE,
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
#ifndef CONFIG_FPGA
	/* UART3 */
    {
        .virtual	= UART_FTUART010_3_VA_BASE,
		.pfn		= __phys_to_pfn(UART_FTUART010_3_PA_BASE),
		.length		= UART_FTUART010_3_VA_SIZE,
		.type		= MT_DEVICE,
    },
    /* UART4 */
    {
        .virtual	= UART_FTUART010_4_VA_BASE,
		.pfn		= __phys_to_pfn(UART_FTUART010_4_PA_BASE),
		.length		= UART_FTUART010_4_VA_SIZE,
		.type		= MT_DEVICE,
    },
    /* UART5 */
    {
        .virtual	= UART_FTUART010_5_VA_BASE,
		.pfn		= __phys_to_pfn(UART_FTUART010_5_PA_BASE),
		.length		= UART_FTUART010_5_VA_SIZE,
		.type		= MT_DEVICE,
    },
	/* UART6 */
    {
        .virtual	= UART_FTUART010_6_VA_BASE,
		.pfn		= __phys_to_pfn(UART_FTUART010_6_PA_BASE),
		.length		= UART_FTUART010_6_VA_SIZE,
		.type		= MT_DEVICE,
    },
    /* UART7 */
    {
        .virtual	= UART_FTUART010_7_VA_BASE,
		.pfn		= __phys_to_pfn(UART_FTUART010_7_PA_BASE),
		.length		= UART_FTUART010_7_VA_SIZE,
		.type		= MT_DEVICE,
    },
#endif
#ifdef CONFIG_FTTMR010
    /* TIMER */
    {
        .virtual	= TIMER_FTTMR010_VA_BASE,
		.pfn		= __phys_to_pfn(TIMER_FTTMR010_PA_BASE),
		.length		= TIMER_FTTMR010_VA_SIZE,
		.type		= MT_DEVICE,
    },
#endif
#ifdef CONFIG_FPGA
    /* SRAM */ //<====================================== for secondary boot
    {
        .virtual	= HAPS_SRAM_0_VA_BASE,
		.pfn		= __phys_to_pfn(HAPS_SRAM_0_PA_BASE),
		.length		= SZ_4K,
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
	/*
	 * initialize primary interrupt controller
	 */
    printk("board_init_irq=%d\n", IRQ_LOCALTIMER);
    gic_init(0, IRQ_LOCALTIMER, __io(PLATFORM_GIC_DIST_VA_BASE), __io(PLATFORM_GIC_CPU_VA_BASE));

#ifdef CONFIG_FTTMR010
#ifdef CONFIG_FPGA
    //irq_set_irq_type(IRQ_FTTMR010_0_T, IRQ_TYPE_EDGE_RISING);
    irq_set_irq_type(IRQ_GIC_START + 0, IRQ_TYPE_EDGE_RISING);
#endif
    irq_set_irq_type(TIMER_FTTMR010_IRQ0, IRQ_TYPE_EDGE_RISING);
    irq_set_irq_type(TIMER_FTTMR010_IRQ1, IRQ_TYPE_EDGE_RISING);
    irq_set_irq_type(TIMER_FTTMR010_IRQ2, IRQ_TYPE_EDGE_RISING);
#endif
}

/******************************************************************************
 * timer - clockevent and clocksource
 *****************************************************************************/
#ifdef CONFIG_FTTMR010
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
#endif

#ifdef CONFIG_ARM_ARCH_TIMER
static struct arch_timer arm_arch_timer __initdata = {
    .res[0] = {
        .start  = IRQ_LOCALTIMER,
        .flags  = IORESOURCE_IRQ,
    },
	.res[1] = {
        .start  = IRQ_LOCALWDOG,
        .flags  = IORESOURCE_IRQ,
    },
};
#endif

static void __init board_sys_timer_init(void)
{
    //unsigned int pclk = pmu_get_apb_clk();

#ifdef CONFIG_ARM_ARCH_TIMER
	printk("arm_arch_timer\n");
    arch_timer_register(&arm_arch_timer);
#endif

#ifdef CONFIG_FTTMR010
    fttmr010_0_clockevent.freq = CLOCK_TICK_RATE;//pclk;
    fttmr010_clockevent_init(&fttmr010_0_clockevent);

    fttmr010_1_clocksource.freq = CLOCK_TICK_RATE;//pclk;
    fttmr010_clocksource_init(&fttmr010_1_clocksource);
#endif
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
}

MACHINE_START(GM, BOARD_NAME)
	.atag_offset	= BOOT_PARAMETER_PA_OFFSET,   //boot command line, after kernel 3.2 change as relative address
	.map_io		    = board_map_io,
	.init_irq	    = board_init_irq,
	.timer		    = &board_sys_timer,
	.fixup          = board_fixup_memory,
	.init_machine	= board_init_machine,
	.restart	= arch_reset,
#ifdef CONFIG_CPU_HAS_GIC
    .handle_irq     = gic_handle_irq,
#endif
MACHINE_END
