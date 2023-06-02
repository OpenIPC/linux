/*
 * linux/arch/arm/mach-gk/gk.c
 *
 * Author: Steven Yu, <yulindeng@gokemicro.com>
 * Copyright (C) 2012-2015, goke, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

//#define KE_DEBUG

#include CONFIG_GK_CHIP_INCLUDE_FILE

#include <mach/hardware.h>
#include <mach/irqs.h>
#include <mach/uart.h>

#include <asm/io.h>
#include <asm/bitops.h>
#include <asm/sizes.h>
#include <asm/page.h>
#include <asm/memory.h>
#include <asm/mach/map.h>
#include <asm/hardware/vic.h>
#include <asm/system_misc.h>

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>

#include <linux/err.h>
#include <linux/delay.h>
#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/dma-mapping.h>
#include <linux/spinlock.h>


#include <linux/platform_device.h>
#include <linux/proc_fs.h>
#include <linux/memblock.h>

#include <hal/hal.h>
#include <mach/io.h>
#include <mach/gpio.h>
#include <mach/wdt.h>
#include <plat/input.h>
#include <mach/flash.h>

extern u8* load_data_init(void);


struct gk_mem_map_desc
{
    char            name[32];
    struct map_desc io_desc;
};

/* the module name MUST match hardware.h */
#define IOMAP_DESC_ENTRY(module, len) {         \
    .virtual = GK_VA_##module,              \
    .pfn = __phys_to_pfn(GK_PA_##module),   \
    .length = (len),                            \
    .type = MT_DEVICE                           \
}

#define GK_IOMAP(module_name, module, len) {           \
    .name = module_name,          \
    IOMAP_DESC_ENTRY(module, len),          \
}


#define GK_IO_DESC_AHB_ID    0
#define GK_IO_DESC_APB_ID    1
#define GK_IO_DESC_PPM_ID    2
#define GK_IO_DESC_BSB_ID    3
#define GK_IO_DESC_DSP_ID    4
#define GK_IO_DESC_USR_ID    5

static struct gk_mem_map_desc gk_mem_desc[] =
{
    // reserved 0xF0000000 to hal1, and 0xF1000000 to hal2
    // used in get_hal1_virt & get_hal2_virt;
    [GK_IO_DESC_AHB_ID] =
    {/*0*/
        .name       = "AHB",
        .io_desc    =
        {
            .virtual= GK_VA_AHB_BASE,
            .pfn    = __phys_to_pfn(GK_PA_AHB_BASE),
            .length = 0x01000000,
            .type   = MT_DEVICE,
        },
    },
    [GK_IO_DESC_APB_ID] =
    {/*1*/
        .name       = "APB",
        .io_desc    =
        {
            .virtual= GK_VA_APB_BASE,
            .pfn    = __phys_to_pfn(GK_PA_APB_BASE),
            .length = 0x01000000,
            .type   = MT_DEVICE,
        },
    },
    [GK_IO_DESC_PPM_ID] =
    {/*2*/
        .name       = "PPM",    /*Private Physical Memory*/
        .io_desc    =
        {
            .virtual= 0xc0000000,
            .pfn    = __phys_to_pfn(0xc0000000/*DEFAULT_MEM_START*/),
            .length = CONFIG_PHYS_OFFSET-0xc0000000,
            .type   = MT_MEMORY,
        },
    },
    [GK_IO_DESC_BSB_ID] =
    {/*3*/
        .name       = "BSB",
        .io_desc    =
        {
            .virtual= 0xf5000000,
            .pfn    = __phys_to_pfn(0xc2800000),
            .length = 0x00200000,
            .type   = MT_MEMORY,
        },
    },
    [GK_IO_DESC_DSP_ID] =
    {/*4*/
        .name       = "DSP",
        .io_desc    =
        {
            .virtual= 0xf6000000,
            .pfn    = __phys_to_pfn(0xC2a00000),
            .length = 0x01500000,
            .type   = MT_MEMORY,
        },
    },
    [GK_IO_DESC_USR_ID] =
    {/*5*/
        .name       = "USR",
        .io_desc    =
        {
            .virtual= 0xfe000000, // dsp_size <= 0xfe000000-0xf6000000
            .pfn    = __phys_to_pfn(0xC3FF0000),
            .length = 0x00010000,
            .type   = MT_MEMORY,
        },
    },
};



#if 1
//-----------HAL1-------------
u32 get_hal1_virt(void)
{
    return 0xF2000000;
}
EXPORT_SYMBOL(get_hal1_virt);

//-----------HAL2-------------
u32 get_hal2_virt(void)
{
    return 0xF3000000;
}
EXPORT_SYMBOL(get_hal2_virt);

//-----------AHB-------------
u32 get_ahb_phys(void)
{
    return __pfn_to_phys(gk_mem_desc[GK_IO_DESC_AHB_ID].io_desc.pfn);
}
EXPORT_SYMBOL(get_ahb_phys);

u32 get_ahb_virt(void)
{
    return gk_mem_desc[GK_IO_DESC_AHB_ID].io_desc.virtual;
}
EXPORT_SYMBOL(get_ahb_virt);

u32 get_ahb_size(void)
{
    return gk_mem_desc[GK_IO_DESC_AHB_ID].io_desc.length;
}
EXPORT_SYMBOL(get_ahb_size);

//-----------APB-------------
u32 get_apb_phys(void)
{
    return __pfn_to_phys(gk_mem_desc[GK_IO_DESC_APB_ID].io_desc.pfn);
}
EXPORT_SYMBOL(get_apb_phys);

u32 get_apb_virt(void)
{
    return gk_mem_desc[GK_IO_DESC_APB_ID].io_desc.virtual;
}
EXPORT_SYMBOL(get_apb_virt);

u32 get_apb_size(void)
{
    return gk_mem_desc[GK_IO_DESC_APB_ID].io_desc.length;
}
EXPORT_SYMBOL(get_apb_size);

//-----------PPM-------------
u32 get_ppm_phys(void)
{
    return __pfn_to_phys(gk_mem_desc[GK_IO_DESC_PPM_ID].io_desc.pfn);
}
EXPORT_SYMBOL(get_ppm_phys);

u32 get_ppm_virt(void)
{
    return gk_mem_desc[GK_IO_DESC_PPM_ID].io_desc.virtual;
}
EXPORT_SYMBOL(get_ppm_virt);

u32 get_ppm_size(void)
{
    return gk_mem_desc[GK_IO_DESC_PPM_ID].io_desc.length;
}
EXPORT_SYMBOL(get_ppm_size);

//-----------DSP-------------
u32 get_dspmem_virt(void)
{
    return gk_mem_desc[GK_IO_DESC_DSP_ID].io_desc.virtual;
}
EXPORT_SYMBOL(get_dspmem_virt);

u32 get_dspmem_size(void)
{
    return gk_mem_desc[GK_IO_DESC_DSP_ID].io_desc.length;
}
EXPORT_SYMBOL(get_dspmem_size);

u32 get_dspmem_phys(void)
{
    return __pfn_to_phys(gk_mem_desc[GK_IO_DESC_DSP_ID].io_desc.pfn);
}
EXPORT_SYMBOL(get_dspmem_phys);

//-----------BSB-------------
u32 get_bsbmem_phys(void)
{
    return __pfn_to_phys(gk_mem_desc[GK_IO_DESC_BSB_ID].io_desc.pfn);
}
EXPORT_SYMBOL(get_bsbmem_phys);

u32 get_bsbmem_virt(void)
{
    return gk_mem_desc[GK_IO_DESC_BSB_ID].io_desc.virtual;
}
EXPORT_SYMBOL(get_bsbmem_virt);

u32 get_bsbmem_size(void)
{
    return gk_mem_desc[GK_IO_DESC_BSB_ID].io_desc.length;
}
EXPORT_SYMBOL(get_bsbmem_size);
u32 get_usrmem_phys(void)
{
    return __pfn_to_phys(gk_mem_desc[GK_IO_DESC_USR_ID].io_desc.pfn);
}
EXPORT_SYMBOL(get_usrmem_phys);
u32 get_usrmem_virt(void)
{
    return  gk_mem_desc[GK_IO_DESC_USR_ID].io_desc.virtual;
}
EXPORT_SYMBOL(get_usrmem_virt);
u32 get_usrmem_size(void)
{
    return gk_mem_desc[GK_IO_DESC_USR_ID].io_desc.length;
}
EXPORT_SYMBOL(get_usrmem_size);



u32 gk_phys_to_virt(u32 paddr)
{
    int                    i;
    u32                    phystart;
    u32                    phylength;
    u32                    phyoffset;
    u32                    vstart;

    for (i = 0; i < ARRAY_SIZE(gk_mem_desc); i++)
    {
        phystart = __pfn_to_phys(gk_mem_desc[i].io_desc.pfn);
        phylength = gk_mem_desc[i].io_desc.length;
        vstart = gk_mem_desc[i].io_desc.virtual;
        if ((paddr >= phystart) && (paddr < (phystart + phylength)))
        {
            phyoffset = paddr - phystart;
            return (vstart + phyoffset);
        }
    }

    return __phys_to_virt(paddr);
}
EXPORT_SYMBOL(gk_phys_to_virt);

u32 gk_virt_to_phys(u32 vaddr)
{
    int                    i;
    u32                    phystart;
    u32                    vlength;
    u32                    voffset;
    u32                    vstart;

    for (i = 0; i < ARRAY_SIZE(gk_mem_desc); i++)
    {
        phystart    = __pfn_to_phys(gk_mem_desc[i].io_desc.pfn);
        vlength     = gk_mem_desc[i].io_desc.length;
        vstart      = gk_mem_desc[i].io_desc.virtual;
        if ((vaddr >= vstart) && (vaddr < (vstart + vlength)))
        {
            voffset = vaddr - vstart;
            return (phystart + voffset);
        }
    }

    return __virt_to_phys(vaddr);
}
EXPORT_SYMBOL(gk_virt_to_phys);
#endif

void get_stepping_info(int *chip, int *major, int *minor)
{
    *chip  = 0x5;
    *major = 0x1;
    *minor = 0x0;
}
EXPORT_SYMBOL(get_stepping_info);

/* ==========================================================================*/
u64 gk_dmamask = DMA_BIT_MASK(32);
EXPORT_SYMBOL(gk_dmamask);


/* ==========================================================================*/
static struct proc_dir_entry *gk_proc_dir = NULL;

int __init gk_create_proc_dir(void)
{
    int                    retval = 0;

    printk("create proc dir\n");

    gk_proc_dir = proc_mkdir("goke", NULL);
    if (!gk_proc_dir)
        retval = -ENOMEM;

    return retval;
}

struct proc_dir_entry *get_gk_proc_dir(void)
{
    return gk_proc_dir;
}
EXPORT_SYMBOL(get_gk_proc_dir);

/* ==========================================================================*/
typedef void* (*hal_function_t) (unsigned long, unsigned long, unsigned long, unsigned long, unsigned long) ;

struct hw_ops *g_hw;
EXPORT_SYMBOL(g_hw);

#if HW_HAL_MODE
HAL_IO(hw)
#else
DIR_IO(hw)
#endif

#if ADC_HAL_MODE
HAL_IO(adc)
#else
DIR_IO(adc)
#endif
#if AUD_HAL_MODE
HAL_IO(aud)
#else
DIR_IO(aud)
#endif
#if CRY_HAL_MODE
HAL_IO(cry)
#else
DIR_IO(cry)
#endif
#if DDR_HAL_MODE
HAL_IO(ddr)
#else
DIR_IO(ddr)
#endif
#if DMA_HAL_MODE
HAL_IO(dma)
#else
DIR_IO(dma)
#endif
#if DSP_HAL_MODE
HAL_IO(dsp)
#else
DIR_IO(dsp)
#endif
#if ETH_HAL_MODE
HAL_IO(eth)
#else
DIR_IO(eth)
#endif
#if GREG_HAL_MODE
HAL_IO(greg)
#else
DIR_IO(greg)
#endif
#if GPIO_HAL_MODE
HAL_IO(gpio)
#else
DIR_IO(gpio)
#endif
#if I2C_HAL_MODE
HAL_IO(i2c)
#else
DIR_IO(i2c)
#endif
#if I2S_HAL_MODE
HAL_IO(i2s)
#else
DIR_IO(i2s)
#endif
#if IR_HAL_MODE
HAL_IO(ir)
#else
DIR_IO(ir)
#endif
#if IRQ_HAL_MODE
HAL_IO(irq)
#else
DIR_IO(irq)
#endif
#if MCU_HAL_MODE
HAL_IO(mcu)
#else
DIR_IO(mcu)
#endif
#if PWM_HAL_MODE
HAL_IO(pwm)
#else
DIR_IO(pwm)
#endif
#if RCT_HAL_MODE
HAL_IO(rct)
#else
DIR_IO(rct)
#endif
#if SD_HAL_MODE
HAL_IO(sd)
#else
DIR_IO(sd)
#endif
#if SF_HAL_MODE
HAL_IO(sf)
#else
DIR_IO(sf)
#endif
#if SSI_HAL_MODE
HAL_IO(ssi)
#else
DIR_IO(ssi)
#endif
#if TIMER_HAL_MODE
HAL_IO(timer)
#else
DIR_IO(timer)
#endif
#if UART_HAL_MODE
HAL_IO(uart)
#else
DIR_IO(uart)
#endif
#if USB_HAL_MODE
HAL_IO(usb)
#else
DIR_IO(usb)
#endif
#if VOUT_HAL_MODE
HAL_IO(vout)
#else
DIR_IO(vout)
#endif
#if WDT_HAL_MODE
HAL_IO(wdt)
#else
DIR_IO(wdt)
#endif
#if EFUSE_HAL_MODE
HAL_IO(efuse)
#else
DIR_IO(efuse)
#endif
#if USB_DMA_ADD_MODE
unsigned char (*gk_usb_readb)(unsigned int ptr, unsigned int offset);
unsigned short (*gk_usb_readw)(unsigned int ptr, unsigned int offset);
unsigned int (*gk_usb_readl)(unsigned int ptr, unsigned int offset);
void (*gk_usb_writeb)(unsigned int ptr, unsigned int offset, unsigned char value);
void (*gk_usb_writew)(unsigned int ptr, unsigned int offset, unsigned short value);
void (*gk_usb_writel)(unsigned int ptr, unsigned int offset, unsigned int value);

unsigned int (*gk_dma_readl)(unsigned int ptr);
void (*gk_dma_writel)(unsigned int ptr, unsigned int value);
#endif
static inline unsigned int gk_hal_init (void *hal_base_address, unsigned long arg0, unsigned long arg1, unsigned long arg2, unsigned long arg3, unsigned long arg4)
{
    unsigned int rval=0;
#if 1
    hal_function_t hal_init = (hal_function_t) (hal_base_address) ;

    g_hw = (struct hw_ops *)hal_init (arg0, arg1, arg2, arg3, arg4) ;
    printk("hal version = %x \n", (u32)g_hw->get_version());
#endif

    return rval ;
}


static int __init early_boot_splash_logo(char *p)
{
    printk("set boot splash logo = 1 \n");
    gk_boot_splash_logo = 1;
    return 0;
}
early_param("boot_splash_logo_on", early_boot_splash_logo);

void __init gk_map_io(void)
{
    int i;
    u32 iop, ios, iov, hal_add;
    u32 usr_phy_addr;
    u32 bsb_phy_addr;
    u32 bsb_size;
    u32 dsp_phy_addr;
    u32 dsp_size;
#if 0
    u32 soc_type; //0:, 1:7102
#endif
    u32 mem_size;
    u32 mem_total;
    u32 usr_size;  //user used, cmem module
    struct memblock_region *reg;
    unsigned long kernelMemoryGet=0;

    /*************************/
    /*          AHB          */
    /*-----------------------*/
    /*          APB          */
    /*-----------------------*/
    /*          PPM  <3M>    */
    /*-----------------------*/
    /*          KER          */
    /*-----------------------*/
    /*          BSB  <2M/4M> */
    /*-----------------------*/
    /*          DSP          */
    /*-----------------------*/
    /*          USR  <64K>   */
    /*************************/

    for (i = 0; i < GK_IO_DESC_BSB_ID; i++)
    {
        iop = __pfn_to_phys(gk_mem_desc[i].io_desc.pfn);
        ios = gk_mem_desc[i].io_desc.length;
        iov = gk_mem_desc[i].io_desc.virtual;
        printk("%s: 0x%x  0x%x  -- 0x%x\n", gk_mem_desc[i].name, iop, iov, ios);
        if (ios > 0)
        {
            iotable_init(&(gk_mem_desc[i].io_desc), 1);
        }
    }

    /* calc mem_size by uboot params parsing*/
    for_each_memblock(memory, reg)
    {
        unsigned long pages = memblock_region_memory_end_pfn(reg) -
            memblock_region_memory_base_pfn(reg);
        kernelMemoryGet = pages >> (20 - PAGE_SHIFT);
    }
    mem_size = kernelMemoryGet*1024*1024;

#if 0
    soc_type = *(volatile u32 *)CONFIG_U2K_SOC_ADDR;

    /* if can't get correct value from uboot env, use GK7102 for default*/
    if(soc_type>1)
        soc_type = 1;



	mem_total = *(volatile u32 *)CONFIG_U2K_TOTAL_MEM;
	if(mem_total != 64 && mem_total != 128)
	{
	    /* set mem_total and bsb size by chip ID*/
	    switch(soc_type)
	    {
	    case CONFIG_SOC_GK7102:     /*GK7102*/
	        mem_total = 64*1024*1024;
	        //bsb_size = 0x200000;   /*use 2M for GK7102*/
	        break;
	    case CONFIG_SOC_GK7101:     /*GK*/
	    default:
	        mem_total = 128*1024*1024;
	        //bsb_size = 0x400000;  /*use 4M for GK*/
	        break;
	    }
	}
	else mem_total = mem_total*1024*1024;
#else
	mem_total = (*(volatile u32 *)CONFIG_U2K_TOTAL_MEM)*1024*1024;
#endif

    bsb_size = *(volatile u32 *)CONFIG_U2K_BSB_ADDR;
    usr_size = 0x10000;
    usr_phy_addr = 0xC0000000 + mem_total - usr_size;
    bsb_phy_addr = 0xC0000000 + get_ppm_size() + mem_size;
    dsp_phy_addr = bsb_phy_addr + bsb_size;
    dsp_size = mem_total - get_ppm_size() - mem_size - bsb_size - usr_size;

    gk_mem_desc[GK_IO_DESC_BSB_ID].io_desc.pfn = __phys_to_pfn(bsb_phy_addr);
    gk_mem_desc[GK_IO_DESC_BSB_ID].io_desc.length = bsb_size;

    gk_mem_desc[GK_IO_DESC_DSP_ID].io_desc.pfn = __phys_to_pfn(dsp_phy_addr);
    gk_mem_desc[GK_IO_DESC_DSP_ID].io_desc.length = dsp_size;

    gk_mem_desc[GK_IO_DESC_USR_ID].io_desc.pfn = __phys_to_pfn(usr_phy_addr);
    gk_mem_desc[GK_IO_DESC_USR_ID].io_desc.length = usr_size;

    for (i = GK_IO_DESC_BSB_ID; i < ARRAY_SIZE(gk_mem_desc); i++)
    {
        iop = __pfn_to_phys(gk_mem_desc[i].io_desc.pfn);
        ios = gk_mem_desc[i].io_desc.length;
        iov = gk_mem_desc[i].io_desc.virtual;
        printk("%s: 0x%x  0x%x  -- 0x%x\n", gk_mem_desc[i].name, iop, iov, ios);
        if (ios > 0)
        {
            iotable_init(&(gk_mem_desc[i].io_desc), 1);
        }
    }

    hal_add = *(volatile u32 *)CONFIG_U2K_HAL_ADDR;
    gk_hal_init((void*)(gk_phys_to_virt(hal_add)), get_hal1_virt(), get_hal2_virt(), get_ahb_virt(), get_apb_virt(), (u32)iotable_init);
}

void gk_load_51mcu_code(u32 code)
{
    int i;
    u8   *pData = NULL;

    pData = load_data_init();

    /* Software Reset */
    gk_mcu_setbitsl( MCU_SYS_BASE + 0x0000, 1 << 5);

    /* Enable the download command */
    gk_mcu_setbitsl( MCU_SYS_BASE + 0x002C, 1 << 4);

    /* Disable the MCU51 core clock */
    gk_mcu_clrbitsl( MCU_SYS_BASE + 0x002C, 1 << 1);

    /* Download MCU code to boot ram */
    for(i=0;i<4096;i++)
    {
       gk_mcu_writel((GK_VA_PMU_C51_CODE + (i*4)), pData[i]);
    }
    gk_mcu_writel(MCU_SYS_BASE + 0x005C, code);
    gk_mcu_writel(MCU_SYS_BASE + 0x002C, 0xB2);// 1011 0010
    gk_mcu_writel(MCU_SYS_BASE + 0x002C, 0x63);// 0110 0011
}

void gk_power_off(void)
{

    u32 code = PMU_POWER_OFF_CPU;
    /* Disable interrupts first */
    local_irq_disable();
    local_fiq_disable();
    printk("gk power off...\n");

#ifdef CONFIG_PMU_AUTOMOTIVE
    code = PMU_AUTOMOTIVE_KEY;
#endif
#ifdef CONFIG_PMU_POWER_OFF_CPU
    code = PMU_POWER_OFF_CPU;
#endif
#ifdef CONFIG_PMU_ALWAYS_RUNNING
    // get pmu controller
    gk_gpio_config(gk_all_gpio_cfg.pmu_ctl, GPIO_TYPE_OUTPUT_1);
    msleep(1);
    gk_gpio_config(gk_all_gpio_cfg.pmu_ctl, GPIO_TYPE_OUTPUT_0);
    msleep(1);
    code = PMU_ALWAYS_RUNNING_POWEROFF;
#endif
    gk_load_51mcu_code(code);

    //printk("c51 start ... ...\n");
    while(1){;}
}


//reboot, user watch dog
void gk_restart(char mode, const char *cmd)
{
    printk("gk reboot...\n");

    gk_wdt_writel( WDOG_RELOAD_REG,  0xff);
    gk_wdt_writel( WDOG_RESTART_REG, 0x4755);
    gk_wdt_writel( WDOG_CONTROL_REG, WDOG_CTR_RST_EN | WDOG_CTR_EN);
    mdelay(1000);
    printk("watchdog reset failed...\n");
}


/* ==========================================================================*/
DEFINE_SPINLOCK(gk_reg_lock);
unsigned long gk_reg_flags;
u32 gk_reglock_count = 0;

/* ==========================================================================*/
EXPORT_SYMBOL(gk_reg_lock);
EXPORT_SYMBOL(gk_reg_flags);
EXPORT_SYMBOL(gk_reglock_count);
module_param (gk_reglock_count, uint, S_IRUGO);


