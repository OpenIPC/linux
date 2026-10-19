/*
 * arch/arm/mach-anycloud/pm.c
 *
 */
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/time.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/delay.h>
#include <linux/suspend.h>
#include <asm/cacheflush.h>
#include <asm/fncpy.h>
#include <mach/ak_l2_exebuf.h>
#include <linux/of.h>
#include <linux/of_platform.h>
#include <linux/of_address.h>
#include <linux/platform_device.h>
#include <linux/module.h>
#include <linux/ioport.h>
#include <linux/init.h>
#include <asm-generic/gpio.h>
#include <linux/gpio.h>
#include <linux/of_gpio.h>
#include <asm/cputype.h>
#include <asm/idmap.h>
#include <asm/procinfo.h>
#include "ak_pm.h"
#include <mach/map.h>


#ifdef CONFIG_WAKELOCK
#include <linux/wakelock.h>
  struct wake_lock pc_wake_lock;
#endif

struct pm_wakeup_info ak_pm_info;
//struct pm_mem_wakeup_info ak_pm_mem_info;


static unsigned int wk_src = 0;

static ssize_t wake_up_src_show(struct device *dev,
                 struct device_attribute *attr, char *buf)
{
    return sprintf(buf, "%d\n", wk_src);
}

static DEVICE_ATTR(wake_up_src, S_IRUSR, wake_up_src_show, NULL);

static struct attribute *wk_src_attributes[] = {
    &dev_attr_wake_up_src.attr,
    NULL,
};

static const struct attribute_group wk_src_attr_group = {
    .attrs = wk_src_attributes,
};
//static suspend_state_t ak_pm_state = -1;
#if defined(CONFIG_MACH_KM01A)
static phys_addr_t ak_pm_resume_pbase;
static void __iomem *ak_pm_resume_code_base = NULL;
 unsigned int *ak_pm_resume_data_base;

extern phys_addr_t __pm_resume_data_pbase;
extern phys_addr_t __pm_resume_data_vbase;
extern u32 ak_pm_resume_in_mem_sz;
//extern void ak_pm_suspend_in_mem(unsigned long param1,
//unsigned long param2, unsigned long param3, unsigned long param4);
extern void ak_pm_resume_in_mem(void);
//extern void ak_pm_save_data_in_mem(unsigned long param);

//typedef void (*ak_pm_suspend_in_mem_t)(unsigned long param1,
//unsigned long param2, unsigned long param3, unsigned long param4);
#endif
#if 0
static ak_pm_debug_init(void)
{
}
#else
#define ak_pm_debug_init() do { } while(0)
#endif
#if 0
int ak_suspend_state(void)
{
    return ak_pm_state;
}
EXPORT_SYMBOL(ak_suspend_state);
#endif
static int ak_pm_begin(suspend_state_t state)
{
    //ak_pm_state = state;
    printk(KERN_INFO"%s ...\n", __func__);
    return 0;
}

static void ak_pm_end(void)
{
   // ak_pm_state = -1;
    printk(KERN_INFO"%s ...\n", __func__);
}

static int ak_pm_valid_state(suspend_state_t state)
{
    switch(state) {
        case PM_SUSPEND_ON:
        case PM_SUSPEND_STANDBY:
        case PM_SUSPEND_MEM:
            return 1;
        default:
            return 0;
    }
}

#define REG32(_register_) (*(volatile unsigned long*)(_register_))
// chip configurations
#define CHIP_CONF_BASE_ADDR                   0x08000000
// module clock control(switch)
#define CLOCK_GATE_REG                      (CHIP_CONF_BASE_ADDR + 0x0000001C)
// ISP_CNN_VIDEO SRC CLK CFG
#define ISP_CNN_VIDEO_SRC_CLK_CFG_REG       (CHIP_CONF_BASE_ADDR + 0x0000011C)
// module software reset control register
#define RESET_CTRL_REG                      (CHIP_CONF_BASE_ADDR + 0x00000020)
// WATCHDOG CFG1
#define WATCHDOG_CFG0_REG                   (CHIP_CONF_BASE_ADDR + 0x000000E4)
// WATCHDOG CFG2
#define WATCHDOG_CFG1_REG                   (CHIP_CONF_BASE_ADDR + 0x000000E8)

#define REBOOT_SELF_REFRESH() do {                                                       \
        /*disable all clock expect DDR2 & L2BUF0*/                                       \
        REG32(CLOCK_GATE_REG) = (~((0x1<<9) | (0x3<<24)));                               \
        REG32(CHIP_CONF_BASE_ADDR) = REG32(CHIP_CONF_BASE_ADDR);                         \
        /*disable clock of enc/npu/isp*/                                                 \
        REG32(ISP_CNN_VIDEO_SRC_CLK_CFG_REG) &= ~((0x1<<22) | (0x1<<14) | (0x1<<6));     \
        REG32(CHIP_CONF_BASE_ADDR) = REG32(CHIP_CONF_BASE_ADDR);                         \
        /*HOLD all reset except DDR2&L2BUF0*/                                            \
        REG32(RESET_CTRL_REG) = (~((0x1<<9) | (0x3<<24)));                               \
        REG32(CHIP_CONF_BASE_ADDR) = REG32(CHIP_CONF_BASE_ADDR);                         \
        /*HOLD reset of enc/npu/isp*/                                                    \
        REG32(ISP_CNN_VIDEO_SRC_CLK_CFG_REG) |= ((0x1<<23) | (0x1<<15) | (0x1<<7));      \
        REG32(CHIP_CONF_BASE_ADDR) = REG32(CHIP_CONF_BASE_ADDR);                         \
        PM_DELAY(100);                                                                   \
        /*disable auto refresh*/                                                         \
        REG32(0x2100000c) &= ~(0x1<<0);                                                  \
        /*precharge all*/                                                                \
        REG32(0x21000010) = ( (0x1<<25)| (0x1<<23) |(0x1<<21) | (0x1<<10) );             \
        /*delay 10cycle*/                                                                \
        PM_DELAY(10);                                                                    \
        /*enter ram self refresh  */                                                     \
        REG32(0x21000010) = ( (0x1<<26) | (0x1<<23) |(0x1<<22) );                        \
        while(1){                                                                        \
            if((REG32(0x21000010) & (0x1<<23)) == 0)                                     \
                break;                                                                   \
        }                                                                                \
        /*delay 20cycle*/                                                                \
        PM_DELAY(10000);                                                                 \
        /*reset MDLU*/                                                                   \
        REG32(0x2100009c) |= (0x1<<31);                                                  \
        /*because (unit: 21333.3333ns) ,so (46875 is about 1s)， (47 is about 1ms)*/     \
        /*config watch dog time*/                                                        \
        REG32(WATCHDOG_CFG0_REG) = ((0x55<<24) | (1 | (1<<3)));                          \
        REG32(CHIP_CONF_BASE_ADDR) = REG32(CHIP_CONF_BASE_ADDR);                         \
        /*enable watch dog*/                                                             \
        REG32(WATCHDOG_CFG1_REG) = ((0xaa<<24) | 0x1);                                   \
        REG32(CHIP_CONF_BASE_ADDR) = REG32(CHIP_CONF_BASE_ADDR);                         \
        /*start watch dog ; if here not feed dog ,the watch dog will not work*/          \
        REG32(WATCHDOG_CFG1_REG) = ((0xaa<<24) | 0x3);                                   \
        REG32(CHIP_CONF_BASE_ADDR) = REG32(CHIP_CONF_BASE_ADDR);                         \
        while(1);                                                                        \
} while(0)


#define DRAM_SELF_REFRESH() do {                                                         \
        /*1、进入DRAM SELF REFRESH*/                                                    \
        /*disable auto refresh*/                                                     \
        REG32(0x2100000c) &= ~(0x1<<0);                                              \
        /*precharge all*/                                                            \
        REG32(0x21000010) = ( (0x1<<25)| (0x1<<23) |(0x1<<21) | (0x1<<10) );         \
        /*延时2cycle 等待TRP */                                                          \
        PM_DELAY(2);                                                                 \
        /*enter ram self refresh*/                                                   \
        REG32(0x21000010) = ( (0x1<<26) | (0x1<<23) |(0x1<<22) );                    \
        /*延时1cycle */                                                                \
        PM_DELAY(1);                                                                 \
        /*复位MDLU */                                                                  \
        REG32(0x2100009c) |= (0x1<<31);                                              \
        /*2、关闭DDR2 IO 并关闭*/                                                          \
        /*关闭DDR2 IO */                                                               \
        REG32(0x21000078) |= (0x1<<31);                                              \
    } while(0)

#define WAIT_RTC_READY() do {                       \
        unsigned int  cnt;                       \
        cnt = 0;                                 \
        while(!(REG32(SYSINT_STATUS_REG)& 0x80))\
        {                                        \
            cnt++;                               \
            if(cnt == 1000)                      \
            {                                    \
                break;                           \
            }                                    \
        }                                        \
    } while(0)

#define READ_RTC_REG(addr) ({                                   \
        unsigned int reg_val;                                        \
        unsigned int data;                                           \
        /*等待RTC READY*/                                            \
        WAIT_RTC_READY();                                            \
        /*使能RTC读写操作*/                                          \
        REG32(RTC_CFG_REG) |= (1<<25);                               \
        reg_val = REG32(RTC_CFG_REG);                                \
        reg_val &= ~0xffffff;                                        \
        reg_val |= (1<<22) | (0x2<<19) | (1<<18) | (addr<<14);       \
        REG32(RTC_CFG_REG) = reg_val;                                \
        /*等待RTC READY*/                                            \
        WAIT_RTC_READY();                                            \
        /*读出数据*/                                                 \
        data = (REG32(RTC_RECE_DAT_REG) & 0x3fff);                   \
        /*停止RTC读写*/                                              \
        REG32(RTC_CFG_REG)&= ~(1 << 25);                             \
        (data);                                                      \
    })

#define   WRITE_RTC_REG(addr, data)  do {                                                            \
        unsigned int  reg_val;                                                                    \
       /*等待RTC READY*/                                                                            \
        WAIT_RTC_READY();                                                                          \
        /*写入RTC寄存器*/                                                                              \
        reg_val = REG32(RTC_CFG_REG);                                                              \
        reg_val &= ~0xffffff;                                                                     \
        reg_val |=  (1 << 25)|(1 << 24)|(1<<22)|(0x2<<19)|((addr&0xF)<<14)|((data&0x3fff)<<0);     \
        reg_val &= ~(1<<18) ;                                                                      \
        REG32(RTC_CFG_REG) = reg_val;                                                               \
       /*等待RTC READY*/                                                                              \
        WAIT_RTC_READY();                                                                          \
        /*停止RTC读写*/                                                                               \
        REG32(RTC_CFG_REG)  &= ~(1 << 25);                                                        \
     }while(0)

#define   REBOOT_DISABLE_CACHE_MMU() do { \
    __asm__ __volatile__(   \
        "reboot_tci_loop: mrc  p15, 0, r15, c7, c14, 3\n\t" /* test, clean,\
                                            invalidate D cache */\
        "bne  reboot_tci_loop\n\t"                                     \
        "mcr  p15, 0, %0, c8, c7, 0\n\t"    /* invalidate  TLBs */  \
        "mcr  p15, 0, %0, c7, c5, 0\n\t"    /* invalidate I caches */   \
        "mrc  p15, 0, %0, c1, c0, 0\n\t"                            \
        "bic  %0, %0, #0x1000\n\t"      /* disable Icache */            \
        "mcr  p15, 0, %0, c1, c0, 0\n\t"\
        "mrc  p15, 0, %0, c1, c0, 0\n\t"                            \
        "bic  %0, %0, #0x0004\n\t"      /* disable Dcache*/     \
        "mcr  p15, 0, %0, c1, c0, 0\n\t"\
        "mrc  p15, 0, %0, c1, c0, 0\n\t"                            \
        "bic  %0, %0, #0x0001\n\t"      /* disable mmu*/        \
        "ldr  %1, =reboot_l2_phys_run\n\t"     /* load 0x480000xx address */   \
        "b  reboot_suspend_turn_off_mmu\n\t"                               \
        "   .align 5\n\t"               /* 32 byte aligned */           \
        "reboot_suspend_turn_off_mmu:\n\t"\
        "mcr  p15, 0, %0, c1, c0, 0\n\t"\
        "mov  pc, %1\n\t"       /* jumpto 0x480000xx then run */        \
        "reboot_l2_phys_run:\n\t"      /* mark the real running addr--> L2 buff */\
        ::"r"(0),"r"(1));   \
    } while(0)

/*用于调试*/
#define   AK_GPIO70(value)  do {                                                            \
        unsigned int  reg_val;                                                                    \
        /*复用gpio*/                                                                            \
        reg_val = REG32(0x08000198);                                                              \
        reg_val &= ~(0x7 << 24);                                                                   \
        REG32(0x08000198) = reg_val;                                                               \
        /*设置输出*/                                                                           \
        reg_val = REG32(0x20170008);                                                              \
        reg_val |= (0x1 << 6);                                                                  \
        REG32(0x20170008) = reg_val;                                                               \
         if (value == 1){  \
        /*输出1*/                                                                               \
        reg_val = REG32(0x2017001c);                                                              \
        reg_val |= (0x1 << 6);                                                                  \
        REG32(0x2017001c) = reg_val;                                                               \
         }else{ \
        /*输出0*/                                                                               \
        reg_val = REG32(0x2017001c);                                                              \
        reg_val &= ~(0x1 << 6);                                                                  \
        REG32(0x2017001c) = reg_val;                                                               \
         } \
     }while(0)

void L2_LINK(standby) L2FUNC_NAME(standby)(int state,unsigned long param1,
    unsigned long param2,unsigned long param3, unsigned long param4)
{

    unsigned long value;
    unsigned long pmc_value __maybe_unused;
    //AK_GPIO70(0);

/*
 *  Debug print 
 */
#if 0
        do {
            value = __raw_readl(AK_VA_SUBCTRL + 0x30018);
        } while (value & (0x1 << 2));
        __raw_writel(0x41 << 16, AK_VA_SUBCTRL + 0x3000c);
#endif

    if(param1){//执行reboot
        REBOOT_DISABLE_CACHE_MMU();
/*
 *  Debug print 
 */
#if 0
    while(REG32(0x20130018) & (0x1 << 2));
    REG32(0x2013000c) = 0x43 << 16;
#endif
        REBOOT_SELF_REFRESH();
    }


    if (state == PM_SUSPEND_STANDBY){

    DISABLE_CACHE_MMU();

    /* close ddr2 auto refresh */
    value = __raw_readl((void __iomem *)0x2100000c);
    value &= ~(0x1<<0);
    __raw_writel(value, (void __iomem *)0x2100000c); 

    /* send precharge command */
    value = __raw_readl((void __iomem *)0x21000010);
    value &= ~(0xFFFFFFFF<<0);
    value |= 0x12A00400;  /* 0x02A00400; */
    __raw_writel(value, (void __iomem *)0x21000010);

    PM_DELAY(2);

    /* enter ram self-refresh 0x04C00000 */
    value = __raw_readl((void __iomem *)0x21000010);
    value &= ~(0xFFFFFFFF<<0);
    value |= 0x04C00000;
    __raw_writel(value, (void __iomem *)0x21000010);    
    PM_DELAY(3);

    /* close ddr2 */
    value = __raw_readl((void __iomem *)0x21000078);
    value &= ~(0x80000000);
    value |= 0x80000000;
    __raw_writel(value, (void __iomem *)0x21000078);
    __asm__ __volatile__("andeq r0, r0, r0");    


    /* enter standby mode */
    value = __raw_readl((void __iomem *)0x21800000);
    value |= 0x1;
    __raw_writel(value, (void __iomem *)0x21800000);
    PM_DELAY(0x5000);

    /*
     * is standby, wait for wakeup by other mode,
     * example prees gpio-key, rtc, etc
     */
    /* open ddr2 */
    value = __raw_readl((void __iomem *)0x21000078);
    value |= 0x80000000;
    value &= ~0x80000000;
    __raw_writel(value, (void __iomem *)0x21000078);
    __asm__ __volatile__("andeq r0, r0, r0");

    /* open auto refresh */
    value = __raw_readl((void __iomem *)0x2100000c);
    value |= 0x1;
    __raw_writel(value, (void __iomem *)0x2100000c); 
    __asm__ __volatile__("andeq r0, r0, r0");

    /* NOP, set CLK hight and exit self-refresh, txsnr = 55*2 */
    value = __raw_readl((void __iomem *)0x21000010);
    value |= (0x1<<25);
    __raw_writel(value, (void __iomem *)0x21000010);

    /* send NOP command */
    value = __raw_readl((void __iomem *)0x21000010);
    value &= ~(0xFFFFFFFF<<0);
    value |= 0x02F00000;
    __raw_writel(value, (void __iomem *)0x21000010);

    PM_DELAY(300); 

    /* send NOP command */
    value = __raw_readl((void __iomem *)0x21000010);
    value &= ~(0xFFFFFFFF<<0);
    value |= 0x02F00000;
    __raw_writel(value, (void __iomem *)0x21000010);

    /* NOP , txsrd= 200 tck*/
    PM_DELAY(200);

 /* enable ICache & DCache, mmu */ 
     ENABLE_CACHE_MMU();

     }else if (state == PM_SUSPEND_MEM){

#if defined(CONFIG_MACH_KM01A)


         //清cache
         REG32(0x00000000) = REG32(0x00000000);

         /*ddr自刷新和关闭DDR2 IO 并关闭*/
         DRAM_SELF_REFRESH();

         /*PowerSaveBootCfg */
         //__raw_readl((void __iomem *)0x08000000);//clean cache
         pmc_value = READ_RTC_REG(PMC_STATUS_REG);
         //将powersave bit 标志位+ retention  写入PMC_STATUS_REG
         pmc_value &= ~(0x1f<<8);
         pmc_value |= (POWER_SAVE_SYMBOL | (0x1<<13));
         WRITE_RTC_REG(PMC_STATUS_REG,pmc_value);


         /*pmc掉电操作  start*/
         pmc_value = READ_RTC_REG(PMC_CONTROL_REG0);
         //设置power down 
         pmc_value |= (0x1<<0);
         WRITE_RTC_REG(PMC_CONTROL_REG0,pmc_value);


         /*让设置值有效*/
         pmc_value = READ_RTC_REG(PMC_CONTROL_REG0);
         //设置BIT 13 使之有效
         pmc_value |= (0x1<<13);
         WRITE_RTC_REG(PMC_CONTROL_REG0,pmc_value);

         while(1)
         {
             pmc_value = READ_RTC_REG(PMC_CONTROL_REG0);
             if((pmc_value & (0x1<<13)) == 0)
             {
              break;
             }
         }

         //清cache
         REG32(0x00000000) = REG32(0x00000000);

        while(1); //占用cpu

#endif
     }

}

static int ak_pm_enter(suspend_state_t state)
{
#ifdef CONFIG_WAKELOCK
    wake_lock(&pc_wake_lock);
#endif
    printk(KERN_INFO"%s.,line:%d..\n", __func__,__LINE__);
    flush_cache_all();

    /* copy the code that enter standby to l2 memory, then move pc to l2 */
    SPECIFIC_L2BUF_EXEC(standby,state, 0,0,0,0);
    return 0;
}

static struct platform_suspend_ops ak_pm_ops = {
    .valid = ak_pm_valid_state,
    .begin = ak_pm_begin,
    .enter = ak_pm_enter,
    .end   = ak_pm_end,
};



/**
* @BRIEF        pm proble function
* @AUTHOR       cao_donghua
* @DATE         2020-02-26
* @PARAM[in]    *pdev:pointer to platform device.
* @RETURN       int
* @RETVAL       if probe ok,return 0, else return -n
* @NOTES
*/
static int ak_standby_init(struct platform_device *pdev)
{
    struct device *dev = &pdev->dev;
    struct device_node *parent = dev->of_node;
    int ret = 0;
    int number;
    int index;
    int gpio_index;

    struct device_node *node = NULL;
    node = of_get_child_by_name(parent, "standby");
    if(!of_device_is_available(node)) {
        pr_err("standby node is not find or not available!\n");
        return -1;

    }

    number = of_property_count_u32_elems(node, "wakeup-mode");
    if(number < 0){
        printk(KERN_ERR"%s, line:%d, err: no set wakeup-mode!\n",
            __func__,__LINE__);
        return -1;
    }

    ak_pm_info.wakeup_mode_cnt = number;
    for (index = 0; index < number; index++) {
        /* get wakeup mode cnt */
        ret = of_property_read_u32_index(node, "wakeup-mode", index,
            &ak_pm_info.wakeup_mode[index]);
        if (ret < 0){
            printk(KERN_ERR"%s, line:%d, err: get no wakeup-mode cnt!\n",
                __func__,__LINE__);
            return ret;
        }

        if (ak_pm_info.wakeup_mode[index] == 0x1){ /* gpio wakeup */    
            /* get wakup gpio mode , gpio config cnt */
            ak_pm_info.wakeup_gpio_cnt =
                of_gpio_named_count(node, "wakeup-gpio");
            if(ak_pm_info.wakeup_gpio_cnt < 0){
                printk(KERN_ERR"%s, line:%d, err: no set wakeup-gpio!\n",
                    __func__,__LINE__);
                continue;
            }

            /* get wakeup gpio index */
            for (gpio_index = 0; gpio_index < ak_pm_info.wakeup_gpio_cnt;
                gpio_index++) {
                ret = of_get_named_gpio(node, "wakeup-gpio", gpio_index);
                ak_pm_info.wakeup_gpio_index[gpio_index] = ret;
            }

            /*
             * get wakup gpio edge config cnt,because every gpio must be had
             * one edge config
             */
            ak_pm_info.wakeup_gpio_edge_cnt =
                of_property_count_u32_elems(node, "wakeup-gpio-edge");
            if(ak_pm_info.wakeup_gpio_edge_cnt < 0){
                printk(KERN_ERR"%s, line:%d, err: get no wakeup-gpio-edge \
                    cnt!\n", __func__,__LINE__);
                continue;
            }

            /*
             * because every gpio must be had one edge config,
             * so gpio index cnt must be equ edge cnt
             */
            if (ak_pm_info.wakeup_gpio_cnt != ak_pm_info.wakeup_gpio_edge_cnt){
                printk(KERN_ERR"%s, line:%d, err: set wakeup-gpio and \
                    wakeup-gpio-edge cnt not equal!\n", __func__,__LINE__);
                continue;
            }

            /* get wakup gpio edge config */
            for (gpio_index = 0; gpio_index < ak_pm_info.wakeup_gpio_edge_cnt;
                gpio_index++) {
                ret = of_property_read_u32_index(node, "wakeup-gpio-edge",
                    gpio_index, &ak_pm_info.wakeup_gpio_edge[gpio_index]);
                if (ret < 0){
                    printk(KERN_ERR"%s, line:%d, err: get no \
                        wakeup-gpio-edge!\n", __func__,__LINE__);
                    break;
                }
            }           
        }

        if (ak_pm_info.wakeup_mode[index] == 0x3){/* ain0 wakeup */
            ret = of_property_read_u32(node, "wakeup-ain0-edge",
                &ak_pm_info.wakeup_ain0_edge);
            if (ret < 0){
                ak_pm_info.wakeup_ain0_edge = 0x0;
                printk(KERN_ERR"%s, line:%d, err: get no wakeup_ain0_edge!\n",
                    __func__,__LINE__);
            }
        }
    }

    return 0;
}
#if defined(CONFIG_MACH_KM01A)
extern struct proc_info_list *lookup_processor(u32 midr);

static int button_gpio;
static struct delayed_work ak_button_work;

static void ak_button_gpio_handler(struct work_struct *work)
{
    while(1){
       if(pmc_read_wakeup_source() == 1) {
           break;
       }
       pr_err("*** first use pmc please press the button***\n");
       msleep(1000);
    }
   //配置输入
   gpio_direction_input(button_gpio);
}

static int ak_shallow_sleep_init(struct platform_device *pdev)
{
    struct resource res;
    struct device_node *memory;
    struct device *dev = &pdev->dev;
    struct device_node *parent = dev->of_node;
    struct proc_info_list *list = lookup_processor(read_cpuid_id());
    int ret;
    int number;
    int index;

    /**************获取dts的配置*****************/
    struct device_node *node = NULL;
    node = of_get_child_by_name(parent, "mem");
    if(!of_device_is_available(node)) {
        pr_err("mem node is not find or not available!\n");
        return -1;
    }


     /******pwr-td0******/
    ret = of_property_read_u32(node, "pwr-td0", 
    &ak_pm_info.mem_pwr_td0);

    AK_BUG("[%s %d]ak_pm_info.mem_pwr_td0:%d\n",
    __func__,__LINE__,ak_pm_info.mem_pwr_td0);

    /*******wakeup-mem-mode*****/
     number = of_property_count_u32_elems(node, "wakeup-mem-mode");
     if(number < 0){
         printk(KERN_ERR"%s, line:%d, err: no set wakeup-mode!\n",
             __func__,__LINE__);
         ret = -1;
         return ret;
     }

     ak_pm_info.mem_wakeup_mode_cnt = number;

     for (index = 0; index < number; index++) {

         /* get wakeup mode cnt */
         ret = of_property_read_u32_index(node, "wakeup-mem-mode", index,
             &ak_pm_info.mem_wakeup_mode[index]);
         if (ret < 0){
             printk(KERN_ERR"%s, line:%d, err: get no wakeup-mem-mode cnt!\n",
                 __func__,__LINE__);
             return ret;
         }
         AK_BUG("[%s %d]ak_pm_info.mem_wakeup_mode[%d]:%d\n",
         __func__,__LINE__,index,ak_pm_info.mem_wakeup_mode[index]);

         if (ak_pm_info.mem_wakeup_mode[index] == 0x1){ /*  pwr-wakeup */
                 ret = of_property_read_u32(node, "pwr-wakeup-trigger", 
                    &ak_pm_info.mem_pwr_wakeup_edge);
                 if (ret < 0){
                     printk(KERN_ERR"%s, line:%d, err: get no \
                          pwr-wakeup-trigger!\n", __func__,__LINE__);
                     break;
                 }

               AK_BUG("[%s %d]ak_pm_info.mem_pwr_wakeup_edge:%d\n",
               __func__,__LINE__,ak_pm_info.mem_pwr_wakeup_edge);
        }

        if (ak_pm_info.mem_wakeup_mode[index] == 0x3){ /*  pwr-button */
              ret = of_property_read_u32(node, "pwr-button-time", 
                 &ak_pm_info.mem_pwr_button_time);
              if (ret < 0){
                  printk(KERN_ERR"%s, line:%d, err: get no \
                    pwr-wakeup-trigger!\n", __func__,__LINE__);
                  break;
              }

            AK_BUG("[%s %d]ak_pm_info.mem_pwr_button_time:%d\n",
            __func__,__LINE__,ak_pm_info.mem_pwr_button_time);
        }

     }


    /*首次上电必须BUTTON按203ms才能让PWR_WAKEUP/RTC WAKEUP控制SEQ的功能生效*/
    button_gpio = of_get_named_gpio(node, "button-gpio", 0);
    if((!(button_gpio < 0)) && (pmc_read_wakeup_source() == 3)){
        gpio_direction_output(button_gpio,0);
        INIT_DELAYED_WORK(&ak_button_work, ak_button_gpio_handler);
        schedule_delayed_work(&ak_button_work,msecs_to_jiffies(250));
    }

    /**************DDR里面保存休眠唤醒对应的代码*****************/
    memory = of_parse_phandle(pdev->dev.of_node, "memory-region", 0);
    ret = of_address_to_resource(memory, 0, &res);
    of_node_put(memory);
    if (ret)
        return ret;

    ak_pm_resume_pbase = res.start;
    ak_pm_resume_code_base = devm_ioremap(&pdev->dev, ak_pm_resume_pbase, resource_size(&res));
    if (IS_ERR(ak_pm_resume_code_base))
        return PTR_ERR(ak_pm_resume_code_base);
    memset(ak_pm_resume_code_base, 0, resource_size(&res););

    ak_pm_resume_data_base = ak_pm_resume_code_base + 0x100;
    __pm_resume_data_pbase = ak_pm_resume_pbase + 0x100;
    __pm_resume_data_vbase = (phys_addr_t)ak_pm_resume_data_base;

    ak_pm_resume_data_base[0] = virt_to_phys(list);
    ak_pm_resume_data_base[1] = list->__cpu_mm_mmu_flags;
    ak_pm_resume_data_base[2] = list->__cpu_flush;

    /* Copy the pm resume handler to ddr */
    fncpy(ak_pm_resume_code_base, &ak_pm_resume_in_mem, ak_pm_resume_in_mem_sz);

    return 0;
} /* end of func */
#endif
/**
* @BRIEF        pm proble function
* @AUTHOR       cao_donghua
* @DATE         2020-02-26
* @PARAM[in]    *pdev:pointer to platform device.
* @RETURN       int
* @RETVAL       if probe ok,return 0, else return -n
* @NOTES
*/
static int ak_pm_probe(struct platform_device *pdev)
{
#if defined(CONFIG_MACH_KM01A)
    ak_shallow_sleep_init(pdev);
#endif
    ak_standby_init(pdev);
    suspend_set_ops(&ak_pm_ops);

    printk(KERN_ERR"%s , line:%d , pm Driver probe\n", __func__, __LINE__);

    return 0;
}


/**
* @BRIEF        pm remove function
* @AUTHOR       cao_donghua
* @DATE         2020-02-26
* @PARAM[in]    *pdev:pointer to platform device.
* @RETURN       int
* @RETVAL       if probe ok,return 0, else return -n
* @NOTES
*/
static int ak_pm_remove(struct platform_device *pdev)
{

    printk(KERN_ERR"%s , line:%d , pm Driver remove \n", __func__, __LINE__);

    return 0;
}


static const struct of_device_id ak_pm_of_ids[] = {
    { .compatible = "anyka,ak37e-pm" },
    { .compatible = "anyka,ak3918av100-pm" },
    { .compatible = "anyka,km01a-pm" },
    { .compatible = "anyka,ak3918av130-pm" },
    {},
};
MODULE_DEVICE_TABLE(of, ak_pm_of_ids);

static struct platform_driver ak_pm_driver = {
    .driver = {
        .name    = "ak_pm",
        .of_match_table = of_match_ptr(ak_pm_of_ids),
        .owner   = THIS_MODULE,
    },
    .probe   = ak_pm_probe,
    .remove  = ak_pm_remove,
};


static int __init ak_pm_init(void)
{
    printk("AK Power Management, (c) 2021 ANYKA\n");
#ifdef CONFIG_WAKELOCK
    wake_lock_init(&pc_wake_lock, WAKE_LOCK_SUSPEND, "pc_wakelock");
#endif
    return platform_driver_register(&ak_pm_driver);
}

module_init(ak_pm_init);


/**
* @BRIEF        pm drv exit function
* @AUTHOR       cao_donghua
* @DATE         2020-02-26
* @PARAM[in]    void.
* @RETURN       void
* @RETVAL       none
* @NOTES
*/
static void __exit ak_pm_exit(void)
{
    printk(KERN_ERR"%s , line:%d , pm Driver\n", __func__, __LINE__);
    platform_driver_unregister(&ak_pm_driver);
}

module_exit(ak_pm_exit);

MODULE_DESCRIPTION("Anyka PM driver");
MODULE_AUTHOR("Anyka Microelectronic Ltd.");
MODULE_LICENSE("GPL");
MODULE_VERSION("1.0.00");


