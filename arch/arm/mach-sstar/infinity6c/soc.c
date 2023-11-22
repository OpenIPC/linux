/*
* soc.c- Sigmastar
*
* Copyright (c) [2019~2020] SigmaStar Technology.
*
*
* This software is licensed under the terms of the GNU General Public
* License version 2, as published by the Free Software Foundation, and
* may be copied, distributed, and modified under those terms.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License version 2 for more details.
*
*/

#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/init.h>
#include <linux/irqchip.h>
#include <linux/of_platform.h>
#include <linux/of_fdt.h>
#include <linux/sys_soc.h>
#include <linux/slab.h>
#include <linux/suspend.h>
#include <linux/kthread.h>
#include <linux/delay.h>
#include <linux/mutex.h>
#include <linux/timecounter.h>
#include <clocksource/arm_arch_timer.h>
#include <linux/memblock.h>

#include <linux/gpio.h>

#include <asm/mach-types.h>
#include <asm/mach/arch.h>
#include <asm/memory.h>
#include <linux/memblock.h>
#include <asm/io.h>
#include <asm/mach/map.h>
#include "gpio.h"
#include "registers.h"
#include "ms_platform.h"
#include "ms_types.h"
#include "_ms_private.h"
#include "ms_msys.h"
#include "memory.h"
#include <linux/of.h>
#include <linux/of_irq.h>

/* IO tables */
static struct map_desc mstar_io_desc[] __initdata = {
    /* Define Registers' physcial and virtual addresses */
    { IO_VIRT, __phys_to_pfn(IO_PHYS), IO_SIZE, MT_DEVICE },
    { SPI_VIRT, __phys_to_pfn(SPI_PHYS), SPI_SIZE, MT_DEVICE },
    { GIC_VIRT, __phys_to_pfn(GIC_PHYS), GIC_SIZE, MT_DEVICE },
    { IMI_VIRT, __phys_to_pfn(IMI_PHYS), IMI_SIZE, MT_MEMORY_RWX },
};

static const char *mstar_dt_compat[] __initconst = {
    "sstar,infinity6c",
    NULL,
};

static void __init mstar_map_io(void)
{
    iotable_init(mstar_io_desc, ARRAY_SIZE(mstar_io_desc));
}


extern struct ms_chip* ms_chip_get(void);
extern void __init ms_chip_init_default(void);
//extern void __init mstar_init_irqchip(void);


//extern struct timecounter *arch_timer_get_timecounter(void);


/*************************************
*        Sstar chip flush function
*************************************/
static DEFINE_SPINLOCK(mstar_l2prefetch_lock);

static void mstar_uart_disable_line(int line)
{
    //for debug, do not change
    if (line == 0) {
        // UART0_Mode -> X
        //CLRREG16(BASE_REG_CHIPTOP_PA + REG_ID_03, BIT4 | BIT5);
        //CLRREG16(BASE_REG_PMSLEEP_PA + REG_ID_09, BIT11);
    } else if (line == 1) {
        // UART1_Mode -> X
        CLRREG16(BASE_REG_CHIPTOP_PA + REG_ID_03, BIT8 | BIT9);
    } else if (line == 2) {
        // FUART_Mode -> X
        CLRREG16(BASE_REG_CHIPTOP_PA + REG_ID_03, BIT0 | BIT1);
    }
}

static void mstar_uart_enable_line(int line)
{
    //for debug, do not change
    if (line == 0) {
        // UART0_Mode -> PAD_UART0_TX/RX
        //SETREG16(BASE_REG_CHIPTOP_PA + REG_ID_03, BIT4);
    } else if (line == 1) {
        // UART1_Mode -> PAD_UART1_TX/RX
        SETREG16(BASE_REG_CHIPTOP_PA + REG_ID_03, BIT8);
    } else if (line == 2) {
        // FUART_Mode -> PAD_FUART_TX/RX
        SETREG16(BASE_REG_CHIPTOP_PA + REG_ID_03, BIT0);
    }
}

static int mstar_get_device_id(void)
{
    return (int)(INREG16(BASE_REG_PMTOP_PA) & 0x00FF);;
}

static int mstar_get_revision(void)
{
    u16 tmp = 0;
    tmp = INREG16((unsigned int)(BASE_REG_PMTOP_PA + REG_ID_67));
    tmp=((tmp >> 8) & 0x00FF);

    return (tmp+1);
}

static void mstar_chip_flush_miu_pipe(void)
{
    unsigned long  dwLockFlag = 0;
    unsigned short dwReadData = 0;

    spin_lock_irqsave(&mstar_l2prefetch_lock, dwLockFlag);
    //toggle the flush miu pipe fire bit
    *(volatile unsigned short *)(0xFD204414) = 0x10;
    *(volatile unsigned short *)(0xFD204414) = 0x11;

    do {
        dwReadData = *(volatile unsigned short *)(0xFD204440);
        dwReadData &= BIT12;  //Check Status of Flush Pipe Finish

    } while (dwReadData == 0);

    spin_unlock_irqrestore(&mstar_l2prefetch_lock, dwLockFlag);
}

static void mstar_chip_flush_STB_and_miu_pipe(void)
{
    dsb();
    mstar_chip_flush_miu_pipe();
}

static u64 mstar_phys_to_MIU(u64 x)
{
    return ((x) - MIU0_BASE);
}

static u64 mstar_MIU_to_phys(u64 x)
{
    return ((x) + MIU0_BASE);
}


struct soc_device_attribute mstar_soc_dev_attr;

extern const struct of_device_id of_default_bus_match_table[];

static int mstar_get_storage_type(void)
{
//check DIDKEY bank, offset 0x70
#define STORAGE_SPI_NONE                  0x00
#define STORAGE_SPI_NAND_SKIP_SD          BIT2
#define STORAGE_SPI_NAND                  BIT4
#define STORAGE_SPI_NOR                   BIT5
#define STORAGE_SPI_NOR_SKIP_SD           BIT1
#define STORAGE_USB                       BIT12
#define STORAGE_EMMC                      BIT3  //Boot from eMMC
#define STORAGE_EMMC_4                    STORAGE_EMMC
#define STORAGE_EMMC_8                    STORAGE_EMMC|BIT7  //emmc_8bit_mode (0: 4-bit mode, 1: 8-bit mode)
#define STORAGE_EMMC_4_SD1                STORAGE_EMMC|BIT10 //emmc 4-bit source (0: SD0, 1: SD1)
#define STORAGE_BOOT_TYPES                (BIT12|BIT10|BIT7|BIT5|BIT4|BIT3|BIT2|BIT1)

    u16 boot_type = (INREG16(BASE_REG_DIDKEY_PA + 0x70*4) & STORAGE_BOOT_TYPES);

    if (boot_type == STORAGE_SPI_NAND || boot_type == STORAGE_SPI_NAND_SKIP_SD) {
        return (int)MS_STORAGE_SPINAND_ECC;
    } else if ((boot_type & STORAGE_EMMC) == STORAGE_EMMC) {
        return (int)MS_STORAGE_EMMC;
    } else if (boot_type == STORAGE_SPI_NOR || boot_type == STORAGE_SPI_NOR_SKIP_SD) {
        return (int)MS_STORAGE_NOR;
    } else {
        return (int)MS_STORAGE_UNKNOWN;
    }
}

static int mstar_get_package_type(void)
{
    printk(KERN_ERR "!!!!! Machine name [%s] \n", mstar_soc_dev_attr.machine);
    return (INREG16(REG_MAILBOX_PARTNAME) & PARTNAME_PACKAGE_MASK) >> PARTNAME_PACKAGE_SHIFT;

}
static char mstar_platform_name[]=CONFIG_SSTAR_SHORT_NAME;

char* mstar_get_platform_name(void)
{
    return mstar_platform_name;
}

static unsigned long long mstar_chip_get_riu_phys(void)
{
    return IO_PHYS;
}

static int mstar_chip_get_riu_size(void)
{
    return IO_SIZE;
}


static int mstar_ir_enable(int param)
{
    printk(KERN_ERR "NOT YET IMPLEMENTED!![%s]",__FUNCTION__);
    return 0;
}


static int mstar_usb_vbus_control(int param)
{

    int ret;
    //int package = mstar_get_package_type();
    int power_en_gpio=-1;

    struct device_node *np;
    int pin_data;
    int port_num = param >> 16;
    int vbus_enable = param & 0xFF;
    if ((vbus_enable<0 || vbus_enable>1) && (port_num<0 || port_num>1)) {
        printk(KERN_ERR "[%s] param invalid:%d %d\n", __FUNCTION__, port_num, vbus_enable);
        return -EINVAL;
    }

    if (power_en_gpio<0) {
        if (0 == port_num) {
            np = of_find_node_by_path("/soc/Sstar-ehci-1");
        } else {
            np = of_find_node_by_path("/soc/Sstar-ehci-2");
        }

        if (!of_property_read_u32(np, "power-enable-pad", &pin_data)) {
            printk(KERN_ERR "Get power-enable-pad from DTS GPIO(%d)\n", pin_data);
            power_en_gpio = (unsigned char)pin_data;
        } else {
            printk(KERN_ERR "Can't get power-enable-pad from DTS, set default GPIO(%d)\n", pin_data);
            power_en_gpio = PAD_PM_GPIO2;
        }
        ret = gpio_request(power_en_gpio, "USB0-power-enable");
        if (ret < 0) {
            printk(KERN_INFO "Failed to request USB0-power-enable GPIO(%d)\n", power_en_gpio);
            power_en_gpio =-1;
            return ret;
        }
    }
    //disable vbus
    if (0 == vbus_enable) {
        gpio_direction_output(power_en_gpio, 0);
        printk(KERN_INFO "[%s] Disable USB VBUS GPIO(%d)\n", __FUNCTION__,power_en_gpio);
    } else if(1 == vbus_enable) {
        gpio_direction_output(power_en_gpio, 1);
        printk(KERN_INFO "[%s] Enable USB VBUS GPIO(%d)\n", __FUNCTION__,power_en_gpio);
    }
    return 0;
}
static u64 us_ticks_cycle_offset=0;
static u64 us_ticks_factor=1;


static u64 mstar_chip_get_us_ticks(void)
{
	u64 cycles=arch_timer_read_counter();
	u64 usticks=div64_u64(cycles,us_ticks_factor);
	return usticks;
}

void mstar_reset_us_ticks_cycle_offset(void)
{
	us_ticks_cycle_offset=arch_timer_read_counter();
}

static int mstar_chip_function_set(int function_id, int param)
{
    int res=-1;

    printk("CHIP_FUNCTION SET. ID=%d, param=%d\n",function_id,param);
    switch (function_id) {
            case CHIP_FUNC_UART_ENABLE_LINE:
                mstar_uart_enable_line(param);
                break;
            case CHIP_FUNC_UART_DISABLE_LINE:
                mstar_uart_disable_line(param);
                break;
            case CHIP_FUNC_IR_ENABLE:
                mstar_ir_enable(param);
                break;
            case CHIP_FUNC_USB_VBUS_CONTROL:
                mstar_usb_vbus_control(param);
                break;
        default:
            printk(KERN_ERR "Unsupport CHIP_FUNCTION!! ID=%d\n",function_id);

    }

    return res;
}


static void __init mstar_init_early(void)
{


    struct ms_chip *chip=NULL;
    ms_chip_init_default();
    chip=ms_chip_get();

    //enable axi exclusive access
    *(volatile unsigned short *)(0xFD204414) = 0x10;

    chip->chip_flush_miu_pipe=mstar_chip_flush_STB_and_miu_pipe;//dsb
    chip->chip_flush_miu_pipe_nodsb=mstar_chip_flush_miu_pipe;//nodsbchip->phys_to_miu=mstar_phys_to_MIU;
    chip->phys_to_miu=mstar_phys_to_MIU;
    chip->miu_to_phys=mstar_MIU_to_phys;
    chip->chip_get_device_id=mstar_get_device_id;
    chip->chip_get_revision=mstar_get_revision;
    chip->chip_get_platform_name=mstar_get_platform_name;
    chip->chip_get_riu_phys=mstar_chip_get_riu_phys;
    chip->chip_get_riu_size=mstar_chip_get_riu_size;

    chip->chip_function_set=mstar_chip_function_set;
    chip->chip_get_storage_type=mstar_get_storage_type;
    chip->chip_get_package_type=mstar_get_package_type;
    chip->chip_get_us_ticks=mstar_chip_get_us_ticks;

}

#if defined(CONFIG_MIU0_DMA_PFN_OFFSET)
static int mstar_platform_notifier(struct notifier_block *nb,
                      unsigned long event, void *data)
{
    struct device *dev = data;

    if (event != BUS_NOTIFY_ADD_DEVICE)
        return NOTIFY_DONE;

    if (!dev)
        return NOTIFY_BAD;

    if (!dev->of_node) {
        int ret = dma_direct_set_offset(dev, SOC_HIGH_PHYS_START,
                          SOC_LOW_PHYS_START,
                          SOC_HIGH_PHYS_SIZE);
        dev_err(dev, "set dma_offset%08llx%s\n",
              SOC_HIGH_PHYS_START - SOC_LOW_PHYS_START,
              ret ? " failed" : "");
    }
    return NOTIFY_OK;
}

static struct notifier_block platform_nb = {
    .notifier_call = mstar_platform_notifier,
};
#endif

extern char* LX_VERSION;
struct device *parent = NULL;

static irqreturn_t MDrv_RIU_TIMEOUT_interrupt(s32 irq, void *dev_id)
{
    int offset, bank, hit;
    bank = (INREG16(BASE_REG_RIUDBG_PA+BK_REG(0xA))<<8)+((INREG16(BASE_REG_RIUDBG_PA+BK_REG(0x9))&0xff00)>>8);
    offset =((INREG16(BASE_REG_RIUDBG_PA+BK_REG(0x9))&0x00ff)>>1);
    printk("Timeout Bank 0x%x Offset 0x%x\r\n", bank, offset);
    bank = (INREG16(BASE_REG_RIUDBG1_PA+BK_REG(0xA))<<8)+((INREG16(BASE_REG_RIUDBG1_PA+BK_REG(0x9))&0xff00)>>8);
    offset =((INREG16(BASE_REG_RIUDBG1_PA+BK_REG(0x9))&0x00ff)>>1);
    printk("Timeout xiu bridge 0x%x Offset 0x%x\r\n", bank, offset);

    hit =(INREG16(BASE_REG_L3BRIDGE+BK_REG(0x1A))&0x8);
    offset =(INREG16(BASE_REG_L3BRIDGE+BK_REG(0x21))&0xffff);
    offset |= ((INREG16(BASE_REG_L3BRIDGE+BK_REG(0x22))&0xffff)<<16);
    printk("Timeout L3 bridge Hit:%d Offset 0x%x\r\n", hit, offset);

    BUG();
    return IRQ_HANDLED;
}

static void __init mstar_init_machine(void)
{
    struct soc_device *soc_dev;
    char                compat_str[32] = {0};
    struct device_node *dev_node       = NULL;
    int                 iIrqNum        = 0;
    int rc=0;
    pr_info("\n\nVersion : %s\n\n",LX_VERSION);

    mstar_reset_us_ticks_cycle_offset();
    us_ticks_factor=div64_u64(arch_timer_get_rate(),1000000);

    mstar_soc_dev_attr.family = kasprintf(GFP_KERNEL, mstar_platform_name);
    mstar_soc_dev_attr.revision = kasprintf(GFP_KERNEL, "%d", mstar_get_revision());
    mstar_soc_dev_attr.soc_id = kasprintf(GFP_KERNEL, "%u", mstar_get_device_id());
    mstar_soc_dev_attr.api_version = kasprintf(GFP_KERNEL, ms_chip_get()->chip_get_API_version());
    mstar_soc_dev_attr.machine = kasprintf(GFP_KERNEL, of_flat_dt_get_machine_name());

    soc_dev = soc_device_register(&mstar_soc_dev_attr);
    if (IS_ERR(soc_dev)) {
        kfree((void *)mstar_soc_dev_attr.family);
        kfree((void *)mstar_soc_dev_attr.revision);
        kfree((void *)mstar_soc_dev_attr.soc_id);
        kfree((void *)mstar_soc_dev_attr.machine);
        goto out;
    }

    parent = soc_device_to_device(soc_dev);

    /*
     * Finished with the static registrations now; fill in the missing
     * devices
     */
out:
    of_platform_populate(NULL, of_default_bus_match_table, NULL, parent);

    //write log_buf address to mailbox
    OUTREG16(BASE_REG_MAILBOX_PA+BK_REG(0x08), (int)log_buf_addr_get() & 0xFFFF);
    OUTREG16(BASE_REG_MAILBOX_PA+BK_REG(0x09), ((int)log_buf_addr_get() >> 16 )& 0xFFFF);

    //enable all timeout bank
    OUTREG16(BASE_REG_RIUDBG_PA+BK_REG(0x14), 0xFFFF);
    OUTREG16(BASE_REG_RIUDBG_PA+BK_REG(0x15), 0xFFFF);
    OUTREG16(BASE_REG_RIUDBG_PA+BK_REG(0x19), 0xFFFF);
    OUTREG16(BASE_REG_RIUDBG1_PA+BK_REG(0x14), 0xFFFF);
    OUTREG16(BASE_REG_RIUDBG1_PA+BK_REG(0x15), 0xFFFF);
    OUTREG16(BASE_REG_RIUDBG1_PA+BK_REG(0x19), 0xFFFF);
    OUTREG16(BASE_REG_RIUDBG1_PA+BK_REG(0x1b), 0xFFFF);
    OUTREG16(BASE_REG_RIUDBG1_PA+BK_REG(0x1d), 0xFFFF);
    //set ipu timeout counter
    OUTREG16(BASE_REG_L3BRIDGE+BK_REG(0x39), 0xFFFF);

    snprintf(compat_str, sizeof(compat_str) - 1, "sstar,riu");
    dev_node = of_find_compatible_node(NULL, NULL, compat_str);
    if (!dev_node)
    {
        printk("[RIU_TIMEOUT] of_find_compatible_node Fail\r\n");
    }

    iIrqNum = irq_of_parse_and_map(dev_node, 0);

    if (0 != (rc = request_irq(iIrqNum, MDrv_RIU_TIMEOUT_interrupt, IRQF_TRIGGER_HIGH, "RIU_TIMEOUT",dev_node)))
    {
        printk("RIU_TIMEOUT] request_irq [%d] Fail, ErrCode: %d\r\n", iIrqNum, rc);
    }
    //enable timeout counter
    OUTREGMSK16((BASE_REG_RIUDBG_PA + REG_ID_00), 0x1, 0x1);
    OUTREGMSK16((BASE_REG_RIUDBG1_PA + REG_ID_00), 0x1, 0x1);

#if defined(CONFIG_MIU0_DMA_PFN_OFFSET)
    if (PHYS_OFFSET >= SOC_HIGH_PHYS_START) {
        soc_dma_pfn_offset = PFN_DOWN(SOC_HIGH_PHYS_START -
                                      SOC_LOW_PHYS_START);
        bus_register_notifier(&platform_bus_type, &platform_nb);
    }
#endif
}

extern void mstar_create_MIU_node(void);
extern int mstar_pm_init(void);
extern void init_proc_zen(void);
static inline void __init mstar_init_late(void)
{
#ifdef CONFIG_PM_SLEEP
    mstar_pm_init();
#endif
}

static void global_reset(enum reboot_mode mode, const char * cmd)
{
    msys_set_rebootType(cmd);

    while (1) {
        OUTREG8(0x1f001cb8, 0x79);
    }
}

static void __init mstar_pm_reserve(void)
{
    // reserve one page in the beginning of dram for store str info and timestamp records
    memblock_reserve(MIU0_BASE, PAGE_SIZE);
}

static long long __init mstar_pv_fixup(void)
{
    long long offset;
    phys_addr_t mem_start, mem_end;

    mem_start = memblock_start_of_DRAM();
    mem_end = memblock_end_of_DRAM();

    /* nothing to do if we are running out of the <32-bit space */
    if (mem_start >= SOC_LOW_PHYS_START && mem_end   <= SOC_LOW_PHYS_END)
        return 0;

    if (mem_start < SOC_HIGH_PHYS_START || mem_end   > SOC_HIGH_PHYS_END) {
        pr_crit("Invalid address space for memory (%08llx-%08llx)\n",
                (u64)mem_start, (u64)mem_end);
        return 0;
    }

    offset = SOC_HIGH_PHYS_START - SOC_LOW_PHYS_START;

    /* Populate the arch idmap hook */
    arch_phys_to_idmap_offset = -offset;

    return offset;
}

#ifdef CONFIG_SMP
extern struct smp_operations mercury6p_smp_ops;
#endif

DT_MACHINE_START(MS_DT, "SStar Soc (Flattened Device Tree)")
    .dt_compat    = mstar_dt_compat,
    .map_io = mstar_map_io,
    .init_machine = mstar_init_machine,
    .init_early = mstar_init_early,
//    .init_time =  ms_init_timer,
//    .init_irq = mstar_init_irqchip,
    .init_late = mstar_init_late,
    .restart = global_reset,
    .reserve = mstar_pm_reserve,
    .pv_fixup = mstar_pv_fixup,
    #ifdef CONFIG_SMP
    .smp    = smp_ops(mercury6p_smp_ops),
    #endif
MACHINE_END
