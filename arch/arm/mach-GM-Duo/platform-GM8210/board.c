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
#include <linux/module.h>
#include <asm/mach/arch.h>
#include <asm/mach/map.h>
#include <asm/mach/time.h>
#include <asm/mach-types.h>
#include <asm/sizes.h>
#include <asm/setup.h>
#include <asm/io.h>
#include <asm/cputype.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <mach/platform/board.h>
#include <mach/platform/pmu.h>
#include <mach/platform/platform_io.h>
#include <mach/platform/system.h>
#ifdef CONFIG_GM8312
#include <mach/ftintc010.h>
#include <mach/platform/pmu_pcie.h>
#endif
#include <mach/ftintc030.h>
#include <mach/fttmr010.h>
#ifdef CONFIG_FTDMAC030
#include <mach/ftdmac030.h>
#endif

#include <mach/fmem.h>
#include <linux/dma-mapping.h>

/* External functions and variables declaration
 */
extern void platform_devices_init(void);

/* Local variables declaration
 */
void __iomem *ftintc030_base_addr;
void __iomem *ftintc030_base_cpu_0_irq_base; //entry-macro.S will reference it
void __iomem *ftintc030_base_cpu_1_irq_base; //entry-macro.S will reference it

#ifdef CONFIG_GM8312
void __iomem *ftintc010_base_addr;
#endif

static fmem_pci_id_t  pci_id = -1;
static fmem_cpu_id_t  cpu_id = -1;

static inline u32 get_cpu_id(void)
{
    static u32 cpu_id = 0;

    if (!cpu_id)
        cpu_id = (read_cpuid_id() >> 0x4) & 0xFFF;

    return cpu_id;
}

#ifdef CONFIG_FTDMAC030
/*
 * DMAC030 channel filter.
 */
static int board_dmac030_chan_filter(int chan_id)
{
    int ret = 0;

    if (get_cpu_id() == 0x726) {
        if (pci_id == -1)
            fmem_get_identifier(&pci_id, &cpu_id);

        /* Master CPU only can use 0-3 */
        if (chan_id >= 4)
            ret = -1;
        else
            ret = 0;

        /* EP, chan_id 3 is used for 2ddma */
        if ((pci_id != FMEM_PCI_HOST) && (chan_id == 3))
            ret = -1;
    } else {
        /* FA626 CPU only can use 4, 5. 6 and 7 are used for cpu_comm */
        if (chan_id >= 4)
            ret = 0;
        else
            ret = -1;
    }

    return ret;
}
#endif /* CONFIG_FTDMAC030 */

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
        .virtual    = UART_FTUART010_1_VA_BASE,
        .pfn        = __phys_to_pfn(UART_FTUART010_1_PA_BASE),
        .length     = UART_FTUART010_1_VA_SIZE,
        .type       = MT_DEVICE,
    },
    /* UART2 */
    {
        .virtual	= UART_FTUART010_2_VA_BASE,
		.pfn		= __phys_to_pfn(UART_FTUART010_2_PA_BASE),
		.length		= UART_FTUART010_2_VA_SIZE,
		.type		= MT_DEVICE,
    },
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
    /* TIMER0 */
    {
        .virtual	= TIMER_FTTMR010_0_VA_BASE,
		.pfn		= __phys_to_pfn(TIMER_FTTMR010_0_PA_BASE),
		.length		= TIMER_FTTMR010_0_VA_SIZE,
		.type		= MT_DEVICE,
    },
    /* TIMER2 */
    {
        .virtual	= TIMER_FTTMR010_2_VA_BASE,
		.pfn		= __phys_to_pfn(TIMER_FTTMR010_2_PA_BASE),
		.length		= TIMER_FTTMR010_2_VA_SIZE,
		.type		= MT_DEVICE,
    },
    /* INTC030 */
    {
        .virtual	= INTC_FTINTC030_VA_BASE,
		.pfn		= __phys_to_pfn(INTC_FTINTC030_PA_BASE),
		.length		= INTC_FTINTC030_VA_SIZE,
		.type		= MT_DEVICE,
    },
#ifdef CONFIG_GM8312
    /* PCI_PMU */
    {
        .virtual	= PCIPMU_FTPMU010_VA_BASE,
		.pfn		= __phys_to_pfn(PCIPMU_FTPMU010_PA_BASE),
		.length		= PCIPMU_FTPMU010_VA_SIZE,
		.type		= MT_DEVICE,
    },
    /* INTC010 */
    {
        .virtual	= INTC_FTINTC010_VA_BASE,
		.pfn		= __phys_to_pfn(INTC_FTINTC010_PA_BASE),
		.length		= INTC_FTINTC010_VA_SIZE,
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
#ifdef CONFIG_GM8312
    pmu_pcie_earlyinit((void *)PCIPMU_FTPMU010_VA_BASE);
#endif
    /* init dma coherent mapping size,
     * CONSISTENT_DMA_SIZE is defined in memory.h
     * max allowable dma consistent size is 14M (0xFFE0_0000 - 0xFF00_00000)
     * set as 12M to avoid unexpected data overlay, NISH_20121015
     */
    #ifdef CONSISTENT_DMA_SIZE
    init_consistent_dma_size(CONSISTENT_DMA_SIZE);
    #endif
}

#ifdef CONFIG_GM8312
/* HDCP of HDMI is useless, so we use the last 16 bytes as the memory destination
 * format: [0]init_done value, [4]who owned the key, [8]lock/unlock value, [c]: how long
 */
typedef struct {
    unsigned int init_done;
    unsigned int lock;
    unsigned int owner;
    unsigned int wait_cnt;
    unsigned int lock_cnt;
    unsigned int statistic;   /* statistic only */
    unsigned int last_freer;
    unsigned int reserved[1];
} lock_content_t;

#define LOCK_VAL        0xAAAA0000
#define UNLOCK_VAL      0x00005555
#define INIT_DONE       0x00112233
#define LOCK_OWNER      get_cpu_id() //0x000FA726

static unsigned int hdmi_va;

static void board_bus_lock_vainit(void)
{
    void *vaddr, *vaddr2;
    volatile u32 paddr;

    /* memory clear was done in uboot */
    hdmi_va = (unsigned int)ioremap_nocache(HDMI_FTHDCPMEM_PA_BASE, 512);
    if (!hdmi_va)
        panic("Error to allocate va! \n");

    if (pci_id == -1)
        fmem_get_identifier(&pci_id, &cpu_id);

    switch (pci_id) {
      case FMEM_PCI_HOST:
        if (cpu_id == FMEM_CPU_FA726) {
            /* provide the memory */
            vaddr = (void *)kmalloc(PAGE_SIZE, GFP_KERNEL);
            if (!vaddr) panic("%s, error! \n", __func__);

            paddr = __pa(vaddr);
            vaddr2 = (void *)ioremap(paddr, PAGE_SIZE);
            memset(vaddr2, 0, PAGE_SIZE);
            iowrite32(paddr, hdmi_va);
            /* new hdmi_va */
            hdmi_va = (unsigned int)vaddr2;
        } else {
            while ((paddr = ioread32(hdmi_va)) == 0)
                udelay(100);
            hdmi_va = (unsigned int)ioremap(paddr, PAGE_SIZE);
            if (!hdmi_va) panic("%s, error2! \n", __func__);
        }
        break;

      case FMEM_PCI_DEV0:
        if (cpu_id == FMEM_CPU_FA726) {
            /* provide the memory */
            vaddr = (void *)kmalloc(PAGE_SIZE, GFP_KERNEL);
            if (!vaddr) panic("%s, error! \n", __func__);
            memset(vaddr, 0, PAGE_SIZE);
            paddr = __pa(vaddr);
            iowrite32(paddr, hdmi_va + 0x4);
            /* new hdmi_va */
            hdmi_va = (unsigned int)vaddr;
        } else {
            while ((paddr = ioread32(hdmi_va + 0x4)) == 0)
                udelay(100);
            hdmi_va = (unsigned int)ioremap(paddr, PAGE_SIZE);
            if (!hdmi_va) panic("%s, error2! \n", __func__);
        }
        break;

      default:
        panic("%s, no such pci role! \n", __func__);
        break;
    }
}

void board_bus_lock_init(bus_lock_t bus)
{
    volatile int i;
    volatile u32 base = hdmi_va + bus * sizeof(lock_content_t);
    volatile lock_content_t  *content = (lock_content_t *)base;

    if (content->init_done != INIT_DONE) {
        memset((char *)content, 0, sizeof(lock_content_t));
        content->lock = UNLOCK_VAL;
        content->init_done = INIT_DONE;
    }

    //delay a while prevent from multiple CPUs re-entrant
    for (i = 0; i < 0x100; i ++)
        ioread32(hdmi_va);
}
EXPORT_SYMBOL(board_bus_lock_init);

void board_bus_lock(bus_lock_t bus)
{
    volatile unsigned int retval;
    volatile unsigned int setlock = LOCK_VAL;
    volatile unsigned int ptr, base = hdmi_va + bus * sizeof(lock_content_t);
    volatile lock_content_t  *content = (lock_content_t *)base;

    if (content->init_done != INIT_DONE)
        panic("the locker:%d didn't call init! \n", bus);

    if (content->owner == LOCK_OWNER) {
        content->lock_cnt ++;
        content->statistic ++;
        return;
    }

    ptr = base + 4;
re_try:
    __asm__ __volatile__("swp %0, %1, [%2]"
                : "=&r"(retval)
                : "r" (setlock), "r" (ptr)
                : "memory", "cc");
    if (retval != UNLOCK_VAL) {
        if (irqs_disabled() || in_interrupt())
	    udelay(100);
	else
	    msleep(1);
        content->wait_cnt ++;
        goto re_try;
    }

    content->owner = LOCK_OWNER;
    content->lock_cnt ++;
    content->statistic ++;
    content->wait_cnt = 0;
    content->last_freer = 0;
}
EXPORT_SYMBOL(board_bus_lock);

void board_bus_unlock(bus_lock_t bus)
{
    volatile unsigned int retval;
    volatile unsigned int setunlock = UNLOCK_VAL;
    volatile unsigned int ptr, base = hdmi_va + bus * sizeof(lock_content_t);
    volatile lock_content_t  *content = (lock_content_t *)base;

    if (content->init_done != INIT_DONE)
        panic("the locker:%d didn't call init! \n", bus);

    if (content->owner != LOCK_OWNER)
        panic("the locker:%d has wrong owner:0x%x! \n", bus, content->owner);

    content->lock_cnt --;
    if (content->lock_cnt)
        return;

    content->owner = 0;   //no owner
    content->last_freer = LOCK_OWNER;

    ptr = base + 4;
    __asm__ __volatile__("swp %0, %1, [%2]"
                : "=&r"(retval)
                : "r" (setunlock), "r" (ptr)
                : "memory", "cc");

    if (retval != LOCK_VAL) {
        int i;

        for (;;) {
            printk("Error value 0x%x in locker:%d! \n", retval, bus);
            msleep(990);

            for (i = 0; i < 8; i ++)
                printk("content[%d] = 0x%x \n", i, ioread32(base + i * 4));
        }
    }
}
EXPORT_SYMBOL(board_bus_unlock);

/* return value:
 *  -1 for fail which means the bus was locked by others
 *  0 for grab the lock success
 */
int board_bus_trylock(bus_lock_t bus)
{
    volatile unsigned int retval;
    volatile unsigned int setlock = LOCK_VAL;
    volatile unsigned int ptr, base = hdmi_va + bus * sizeof(lock_content_t);
    volatile lock_content_t  *content = (lock_content_t *)base;

    if (content->init_done != INIT_DONE)
        panic("the locker:%d didn't call init! \n", bus);

    if (content->owner == LOCK_OWNER) {
        content->lock_cnt ++;
        content->statistic ++;
        return 0;
    }

    ptr = base + 4;

    __asm__ __volatile__("swp %0, %1, [%2]"
                : "=&r"(retval)
                : "r" (setlock), "r" (ptr)
                : "memory", "cc");
    if (retval != UNLOCK_VAL)
        return -1;

    content->owner = LOCK_OWNER;
    content->lock_cnt ++;
    content->statistic ++;
    content->wait_cnt = 0;
    content->last_freer = 0;

    return 0;
}
EXPORT_SYMBOL(board_bus_trylock);
#endif /* CONFIG_GM8312 */

static void __init board_init_irq(void)
{
	struct ftintc030_trigger_type master_trigger_type = {
		.irqmode[0]	= PLATFORM_IRQ_TRIGGER_MODE1,
		.irqlevel[0]	= ~PLATFORM_IRQ_TRIGGER_LEVEL1,
		.fiqmode[0]	= PLATFORM_FIQ_TRIGGER_MODE1,
		.fiqlevel[0]	= ~PLATFORM_FIQ_TRIGGER_LEVEL1,

		.irqmode[1]	= PLATFORM_IRQ_TRIGGER_MODE2,
		.irqlevel[1]	= ~PLATFORM_IRQ_TRIGGER_LEVEL2 | (1 << (EXT_INT_0_IRQ - 32)),
		.fiqmode[1]	= PLATFORM_FIQ_TRIGGER_MODE2,
		.fiqlevel[1]	= ~PLATFORM_FIQ_TRIGGER_LEVEL2 | (1 << (EXT_INT_0_IRQ - 32)),

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

#ifdef CONFIG_GM8312
	struct ftintc010_trigger_type pcie_trigger_type = {
		.irqmode	= PLATFORM_IRQ_TRIGGER_MODE2 | (0x1 << 8), /* hdmi edge trigger */
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
#endif
	/*
	 * initialize primary interrupt controller
	 */
	ftintc030_base_addr = __io(INTC_FTINTC030_VA_BASE);
    if (get_cpu_id() == 0x726)
	    ftintc030_base_cpu_0_irq_base = __io(INTC_FTINTC030_VA_BASE + FTINTC030_OFFSET_CPU_0_IRQ);
    else
	    ftintc030_base_cpu_1_irq_base = __io(INTC_FTINTC030_VA_BASE + FTINTC030_OFFSET_CPU_1_IRQ);

	ftintc030_init(0, ftintc030_base_addr, 0, &master_trigger_type);

#ifdef CONFIG_GM8312
    if (((ioread32(PMU_FTPMU010_VA_BASE + 0x04) >> 19) & 0x3) != 0x02) // if role is not PCIe RC
        return;

    if (get_cpu_id() == 0x726) {
    	ftintc010_base_addr = __io(INTC_FTINTC010_VA_BASE);
    	ftintc010_init(0, ftintc010_base_addr, PLATFORM_IRQ_TOTALCOUNT, &pcie_trigger_type);
    	if (1) {
    	    /* Enable prefetch for H2XBrg_7 for DMAC.
    	     * Limitation: If read through AXI-PCIe controller IP¡Aread data width < 64 bit,
    	     * burst length only can be "1". Thus we enable pre-fetech function to solve this issue.
    	     */
    	    void *vbase;
    	    u32 tmp;

    	    vbase = (void *)ioremap_nocache(PCIE_H2XBRG_7_PA_BASE, PCI3_H2SBRG_7_PA_SIZE);
    	    tmp = ioread32(vbase) & ~(0x7 << 4);    // bit [6:4] (prefetch number)
    	    tmp |= ((0xA << 16) | (0x2 << 4));  //suggest from Kay, 2013/3/27 10:57AM
    	    iowrite32(tmp, vbase);
            __iounmap(vbase);
    	}
    }
#endif
}

/******************************************************************************
 * timer - clockevent and clocksource
 *****************************************************************************/
static struct fttmr010_clockevent fttmr010_0_0_clockevent = {
	.clockevent = {
		.name	= "fttmr010:0",
		.irq	= TIMER_FTTMR010_0_IRQ0,
	},
	.base	= (void __iomem *)TIMER_FTTMR010_0_VA_BASE,
	.id	= 0,
};

static struct fttmr010_clocksource fttmr010_0_1_clocksource = {
	.clocksource = {
		.name	= "fttmr010:1",
	},
	.base	= (void __iomem *)TIMER_FTTMR010_0_VA_BASE,
	.id	= 1,
};

static struct fttmr010_clockevent fttmr010_2_0_clockevent = {
	.clockevent = {
		.name	= "fttmr010:0",
		.irq	= TIMER_FTTMR010_2_IRQ0,
	},
	.base	= (void __iomem *)TIMER_FTTMR010_2_VA_BASE,
	.id	= 0,
};

static struct fttmr010_clocksource fttmr010_2_1_clocksource = {
	.clocksource = {
		.name	= "fttmr010:1",
	},
	.base	= (void __iomem *)TIMER_FTTMR010_2_VA_BASE,
	.id	= 1,
};

static void __init board_sys_timer_init(void)
{
    unsigned int pclk = pmu_get_apb0_clk();
	printk("Timer use APB0 clock\n");

    if (get_cpu_id() == 0x726) {
    	fttmr010_0_0_clockevent.freq = pclk;
    	fttmr010_clockevent_init(&fttmr010_0_0_clockevent);

    	fttmr010_0_1_clocksource.freq = pclk;
    	fttmr010_clocksource_init(&fttmr010_0_1_clocksource);
    } else {
    	fttmr010_2_0_clockevent.freq = pclk;
    	fttmr010_clockevent_init(&fttmr010_2_0_clockevent);

    	fttmr010_2_1_clocksource.freq = pclk;
    	fttmr010_clocksource_init(&fttmr010_2_1_clocksource);
    }
}

struct sys_timer board_sys_timer = {
	.init	= board_sys_timer_init,
};

/* board init */
static void __init board_init_machine(void)
{
    int i, vaddr_8210, vaddr_8312;

#ifdef CONFIG_GM8312
    board_bus_lock_vainit();
#endif

    platform_devices_init();

    /* dump iotable information
     */
    for (i = 0; i < ARRAY_SIZE(board_io_desc); i ++) {
        printk("iotable: VA: 0x%x, PA: 0x%x, Length: %d \n",
                (u32)board_io_desc[i].virtual,
                (u32)board_io_desc[i].pfn << PAGE_SHIFT,
                (u32)board_io_desc[i].length);
    }

    /* for debug 8312 purpose */
    vaddr_8210 = (u32)ioremap(PCIE_PLDA_PA_BASE, PAGE_SIZE);
    vaddr_8312 = (u32)ioremap(PCIE_PCIE_PLDA_PA_BASE, PAGE_SIZE);
    printk("8210 PCIE: 0x%x(0x%x), 8312 PCIE: 0x%x(0x%x) \n", vaddr_8210, PCIE_PLDA_PA_BASE,
                    vaddr_8312, PCIE_PCIE_PLDA_PA_BASE);

#ifdef CONFIG_FTDMAC030
    ftdmac030_set_platform_chanfilter(board_dmac030_chan_filter);
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
