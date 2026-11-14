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


#ifdef CONFIG_WAKELOCK
#include <linux/wakelock.h>
  struct wake_lock pc_wake_lock;
#endif

struct pm_wakeup_info ak_pm_info;


#if 0
static ak_pm_debug_init(void)
{
}
#else
#define ak_pm_debug_init() do { } while(0)
#endif

static int ak_pm_begin(suspend_state_t state)
{
    printk(KERN_INFO"%s ...\n", __func__);
    return 0;
}

static void ak_pm_end(void)
{
    printk(KERN_INFO"%s ...\n", __func__);
    return;
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

#if 0

void L2_LINK(standby) L2FUNC_NAME(standby)(unsigned long param1,
    unsigned long param2,unsigned long param3, unsigned long param4)
{
    // invalidate and disable mmu
    /* unsigned long value;*/

    DISABLE_CACHE_MMU();

    /*
     * is standby, wait for wakeup by other mode,
     * example prees gpio-key, rtc, etc
     */

    /* enable ICache & DCache, mmu */
    ENABLE_CACHE_MMU();
}

#else

#define REG32(_register_) (*(volatile unsigned long*)(_register_))
#define CHIP_CONF_BASE_ADDR                   0x08000000      // chip configurations
#define CLOCK_GATE_REG                      (CHIP_CONF_BASE_ADDR + 0x0000001C)      // module clock control(switch)
#define ISP_CNN_VIDEO_SRC_CLK_CFG_REG       (CHIP_CONF_BASE_ADDR + 0x0000011C)     // ISP_CNN_VIDEO SRC CLK CFG
#define RESET_CTRL_REG                      (CHIP_CONF_BASE_ADDR + 0x00000020)      // module software reset control register
#define WATCHDOG_CFG0_REG                   (CHIP_CONF_BASE_ADDR + 0x000000E4)     // WATCHDOG CFG1
#define WATCHDOG_CFG1_REG                   (CHIP_CONF_BASE_ADDR + 0x000000E8)     // WATCHDOG CFG2

void L2_LINK(standby) L2FUNC_NAME(standby)(unsigned long param1,
    unsigned long param2,unsigned long param3, unsigned long param4)
{
    // invalidate and disable mmu
    unsigned long watchdog_time;
    
    DISABLE_CACHE_MMU();

    //disable all clock expect DDR2 & L2BUF0
    REG32(CLOCK_GATE_REG) = (~((0x1<<9) | (0x3<<24)));

    //disable clock of enc/npu/isp
    REG32(ISP_CNN_VIDEO_SRC_CLK_CFG_REG) &= ~((0x1<<22) | (0x1<<14) | (0x1<<6));

    //HOLD all reset except DDR2&L2BUF0
    REG32(RESET_CTRL_REG) = (~((0x1<<9) | (0x3<<24)));

    //HOLD reset of enc/npu/isp
    REG32(ISP_CNN_VIDEO_SRC_CLK_CFG_REG) |= ((0x1<<23) | (0x1<<15) | (0x1<<7));

    PM_DELAY(100);

    //disable auto refresh
    REG32(0x2100000c) &= ~(0x1<<0);

    //precharge all
    REG32(0x21000010) = ( (0x1<<25)| (0x1<<23) |(0x1<<21) | (0x1<<10) );

    //delay 10cycle
    PM_DELAY(10);

    //enter ram self refresh  
    REG32(0x21000010) = ( (0x1<<26) | (0x1<<23) |(0x1<<22) );

    while(1){
        if((REG32(0x21000010) & (0x1<<23)) == 0)
            break;
    }

    //delay 20cycle
    PM_DELAY(10000);

    //reset MDLU
    REG32(0x2100009c) |= (0x1<<31);

    //because (unit: 21333.3333ns) ,so (46875 is about 1s)， (47 is about 1ms)
    watchdog_time = 1;

    //config watch dog time
    REG32(WATCHDOG_CFG0_REG) = ((0x55<<24) | (watchdog_time | (1<<3)));

    //enable watch dog
    REG32(WATCHDOG_CFG1_REG) = ((0xaa<<24) | 0x1);

    //start watch dog ; if here not feed dog ,the watch dog will not work
    REG32(WATCHDOG_CFG1_REG) = ((0xaa<<24) | 0x3);

    while(1);
} 

#endif



static int ak_pm_enter(suspend_state_t state)
{
#ifdef CONFIG_WAKELOCK
    wake_lock(&pc_wake_lock);
#endif
    printk(KERN_INFO"%s.,line:%d..\n", __func__,__LINE__);
    flush_cache_all();

    /* copy the code that enter standby to l2 memory, then move pc to l2 */
    SPECIFIC_L2BUF_EXEC(standby, 0,0,0,0);

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
static int ak_pm_probe(struct platform_device *pdev)
{
    struct device *dev = &pdev->dev;
    struct device_node *node = dev->of_node;
    int ret = 0;
    int number;
    int index;
    int gpio_index;
    
    number = of_property_count_u32_elems(node, "wakeup-mode");
    if(number < 0){
        printk(KERN_ERR"%s, line:%d, err: no set wakeup-mode!\n",
            __func__,__LINE__);
        ret = -1;
        return ret;
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


