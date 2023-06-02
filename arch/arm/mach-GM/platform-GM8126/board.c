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
#include <mach/ftintc010.h>
#include <mach/fttmr010.h>
#include <mach/fmem.h>
#include <linux/dma-mapping.h> /* NISH_20121015 add for init_consistent_dma_size() */

/* External functions and variables declaration
 */
extern void platform_devices_init(void);

/* Local variables declaration
 */
void __iomem *ftintc010_base_addr;

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
    /* TIMER */
    {
        .virtual	= TIMER_FTTMR010_VA_BASE,
		.pfn		= __phys_to_pfn(TIMER_FTTMR010_PA_BASE),
		.length		= TIMER_FTTMR010_VA_SIZE,
		.type		= MT_DEVICE,
    },
    /* INTC */
    {
        .virtual	= INTC_FTINTC010_VA_BASE,
		.pfn		= __phys_to_pfn(INTC_FTINTC010_PA_BASE),
		.length		= INTC_FTINTC010_VA_SIZE,
		.type		= MT_DEVICE,
    },
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
    struct ftintc010_trigger_type master_trigger_type = {
		.irqmode	= PLATFORM_IRQ_TRIGGER_MODE2,
		.irqlevel	= ~PLATFORM_IRQ_TRIGGER_LEVEL2,
		.fiqmode	= PLATFORM_FIQ_TRIGGER_MODE2,
		.fiqlevel	= ~PLATFORM_FIQ_TRIGGER_LEVEL2,
#ifdef CONFIG_FTINTC010EX
        .irqmodeex	= PLATFORM_IRQ_TRIGGER_MODEEX2,
		.irqlevelex	= ~PLATFORM_IRQ_TRIGGER_LEVELEX2,
		.fiqmodeex	= PLATFORM_FIQ_TRIGGER_MODEEX2,
		.fiqlevelex	= ~PLATFORM_FIQ_TRIGGER_LEVELEX2,
#endif
	};
    
	/*
	 * initialize primary interrupt controller
	 */
	ftintc010_base_addr = __io(INTC_FTINTC010_VA_BASE);
	ftintc010_init(0, ftintc010_base_addr, 0, &master_trigger_type);
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
#ifdef CONFIG_GM_PHY    
    //set PHY ID
    outl(0x321123, PMU_FTPMU010_VA_BASE + 0x88);	
#endif    
    
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
	.atag_offset	= BOOT_PARAMETER_PA_BASE,   //boot command line
	.map_io		    = board_map_io,
	.init_irq	    = board_init_irq,
	.timer		    = &board_sys_timer,
	.fixup          = board_fixup_memory,
	.init_machine	= board_init_machine,
MACHINE_END
