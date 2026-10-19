/*
 * drivers/watchdog/ak_top_wdt.c
 * Watchdog driver for ak37d/ak39ev330 processors
 * Author: zhang zhipeng
 *
 * Adapted from the IXP2000 watchdog driver by Lennert Buytenhek.
 * The original version carries these notices:
 *
 * Author: Deepak Saxena <dsaxena@plexity.net>
 *
 * Copyright 2004 (c) MontaVista, Software, Inc.
 * Based on sa1100 driver, Copyright (C) 2000 Oleg Drokin <green@crimea.edu>
 *
 * This file is licensed under  the terms of the GNU General Public
 * License version 2. This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/watchdog.h>
#include <linux/init.h>
#include <linux/bitops.h>
#include <linux/uaccess.h>
#include <linux/reboot.h>

#include <mach/map.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/of_platform.h>
#include <asm/io.h>

#define UNIT_NS  21333
/* 1 seconed = 1000000*64/3  */
#define WATCH_DOG_1_SECOND_SET  (1000000000 / UNIT_NS)
/* max feed dog time = 0xffffff* 21333 ~= 357S */
#define MAX_FEED_DOG 357

static int nowayout = WATCHDOG_NOWAYOUT;
static unsigned int heartbeat = 0;
static unsigned int def_heartbeat = 0;
static unsigned int now_heartbeat = 0;

#define MODULE_RESET_CON1       (AK_VA_SYSCTRL + 0x20)
#define MODULE_WDT_CFG1 (AK_VA_SYSCTRL + 0xe4)
#define MODULE_WDT_CFG2 (AK_VA_SYSCTRL + 0Xe8)

#define MODULE_WDT_ACCESS (AK_VA_SYSCTRL + 0x3bc)
#define MODULE_WDT_ENB    (AK_VA_SYSCTRL + 0X3c0)

static unsigned long in_use;
static atomic_t in_write = ATOMIC_INIT(0);
static DEFINE_SPINLOCK(wdt_lock);

static int ak_wdt_disable_nb(struct notifier_block *n, unsigned long state,
    void *cmd);
static struct notifier_block ak_wdt_nb = {
    .notifier_call = ak_wdt_disable_nb,
};

module_param(def_heartbeat, int, 0);
MODULE_PARM_DESC(def_heartbeat, "Watchdog heartbeat in seconds");

module_param(nowayout, int, 0);
MODULE_PARM_DESC(nowayout, "Watchdog cannot be stopped once started");

void wdt_enable(void)
{
    u32 regval;
    
    unsigned int cfg_val = (unsigned int)def_heartbeat;
    spin_lock(&wdt_lock);   
    /* configure the watchdog time*/
    regval = __raw_readl(MODULE_WDT_CFG1);
    regval = ((0x55 << 24) | cfg_val);
    __raw_writel(regval,MODULE_WDT_CFG1);
    regval = __raw_readl(AK_VA_SYSCTRL);
    /* enable watchdog */
    regval = __raw_readl(MODULE_WDT_CFG2);
    regval = ((0xaaUL << 24) | 0x1);
    __raw_writel(regval,MODULE_WDT_CFG2);
     regval = __raw_readl(AK_VA_SYSCTRL);
    /* start watchdog */
    regval = __raw_readl(MODULE_WDT_CFG2);
    regval = ((0xaaUL << 24) | 0x3);
    __raw_writel(regval,MODULE_WDT_CFG2);
     regval = __raw_readl(AK_VA_SYSCTRL);
     
    spin_unlock(&wdt_lock);
}

static void wdt_disable(void)
{
    u32 regval;
    
    pr_debug("%s\n", __func__);
    spin_lock(&wdt_lock);
    
    regval = __raw_readl(MODULE_WDT_CFG2);
    regval = ((0xaaUL << 24));
    __raw_writel(regval,MODULE_WDT_CFG2);
     regval = __raw_readl(AK_VA_SYSCTRL);
    
    spin_unlock(&wdt_lock);
}

 /* feed watchdog time setting and enable watchdog. */
void wdt_keepalive(unsigned int heartbeat)
{
    u32 regval;
    unsigned int cfg_val = (unsigned int)heartbeat; 

    spin_lock(&wdt_lock);
    /* configure the watchdog time*/
    regval = __raw_readl(MODULE_WDT_CFG1);
    regval = ((0x55 << 24) | cfg_val);
    __raw_writel(regval,MODULE_WDT_CFG1);
     regval = __raw_readl(AK_VA_SYSCTRL);
    /* enable watchdog */
    regval = __raw_readl(MODULE_WDT_CFG2);
    regval = ((0xaaUL << 24) | 0x1);
    __raw_writel(regval,MODULE_WDT_CFG2);
     regval = __raw_readl(AK_VA_SYSCTRL);
    /* feed watchdog */
    regval = __raw_readl(MODULE_WDT_CFG2);
    regval = ((0xaaUL << 24) | 0x3);
    __raw_writel(regval,MODULE_WDT_CFG2);
     regval = __raw_readl(AK_VA_SYSCTRL);
    
    spin_unlock(&wdt_lock);
}

static int ak_wdt_disable_nb(struct notifier_block *n, unsigned long state,
    void *cmd)
{
        wdt_disable();
        return NOTIFY_DONE;
}

static struct watchdog_info ident = {
    .options    = WDIOF_MAGICCLOSE | WDIOF_SETTIMEOUT | WDIOF_KEEPALIVEPING,
    .identity   = "ANYKA ak Watchdog",
};

static long ak_wdt_ioctl(struct file *file, unsigned int cmd,
                            unsigned long arg)
{
    void __user *argp = (void __user *)arg;
    int __user *p = argp;
    int ret = -ENOTTY;
    int time;

    switch (cmd) {

    case WDIOC_GETSUPPORT:
        return copy_to_user((struct watchdog_info *)argp, &ident,
                   sizeof(ident)) ? -EFAULT : 0;
    case WDIOC_GETSTATUS:
    case WDIOC_GETBOOTSTATUS:
        return put_user(0, p);

    case WDIOC_KEEPALIVE:
        wdt_keepalive(now_heartbeat);
        return 0;

    case WDIOC_SETTIMEOUT:
        if(get_user(time, p))
            return -EFAULT;

        if(time == -1)
        {
            pr_debug("\n[close watch_dog]\n");
            wdt_disable();
            atomic_set(&in_write, 0);
            pr_debug("\n[MODULE_WDT_CFG2]:0x%08x\n",
                __raw_readl(MODULE_WDT_CFG2));
            return 0;
        }
        
        if (time > MAX_FEED_DOG){
            pr_debug("\n[Beyond the limits of feeddog time.]\n");
            return -EINVAL;
        }

        now_heartbeat = time * WATCH_DOG_1_SECOND_SET;
        wdt_keepalive(now_heartbeat);
        return 0;

    case WDIOC_GETTIMEOUT:
        return put_user((now_heartbeat + 1) / WATCH_DOG_1_SECOND_SET, p);

    default:
        return -ENOTTY;
    }

    return ret;
}


static ssize_t ak_wdt_write(struct file *file, const char *data, size_t len,
    loff_t *ppos)
{
    if (len) {
        size_t i;

        atomic_set(&in_write, 1);
        for (i = 0; i != len; i++) {
            char c;

            if (get_user(c, data + i))
                return -EFAULT;
            if (c == 'V') {
                pr_debug("Detect \"V\" Magic Character\n");
                atomic_set(&in_write, 0);
            }
        }
        wdt_keepalive(def_heartbeat);
    }

    return len;
}

/**
 * @brief ak_wdt_open
 * 
 * @param [in]: struct inode *inode, struct file *file
 * @RETURN: int.
 */
static int ak_wdt_open(struct inode *inode, struct file *file)
{
    if (test_and_set_bit(0, &in_use))
        return -EBUSY;
    
    if (nowayout)
        __module_get(THIS_MODULE);

    wdt_enable();

    return nonseekable_open(inode, file);
}

/**
 * @brief ak_wdt_release
 * 
 * @param [in]: struct inode *inode, struct file *file
 * @RETURN: int.
 */
static int ak_wdt_release(struct inode *inode, struct file *file)
{
    if (nowayout)
        pr_info("WATCHDOG: Driver support nowayout option -"
                            "no way to disable watchdog\n");
    else if (atomic_read(&in_write))
        pr_info("WATCHDOG: Device closed unexpectedly - "
                            "timer will not stop\n");
    else
        wdt_disable();
    clear_bit(0, &in_use);

    return 0;
}

static const struct file_operations ak_wdt_fops = {
    .owner      = THIS_MODULE,
    .llseek     = no_llseek,
    .write      = ak_wdt_write,
    .unlocked_ioctl = ak_wdt_ioctl,
    .open       = ak_wdt_open,
    .release    = ak_wdt_release,
};

static struct miscdevice ak_wdt_miscdev = {
    .minor      = WATCHDOG_MINOR,
    .name       = "watchdog",
    .fops       = &ak_wdt_fops,
};

/**
 * @brief ak_watchdog_probe
 * 
 * @param [in]: struct platform_device *pdev
 * @RETURN: int.
 */
static int ak_watchdog_probe(struct platform_device *pdev)
{

    int ret = 0;
#if defined (CONFIG_MACH_KM01A) || defined(CONFIG_MACH_AK3918AV130)
    u32 regval;
#endif
    pr_info("ak_wdt_init: watchdog register...\n");
    register_reboot_notifier(&ak_wdt_nb);

    ret = of_property_read_u32(pdev->dev.of_node, "def_heartbeat",
                   &heartbeat);
    if (ret < 0) {
        dev_warn(&pdev->dev,
             "Could not read linein-detect property\n");
    } 

    def_heartbeat = heartbeat * WATCH_DOG_1_SECOND_SET;
    now_heartbeat = heartbeat * WATCH_DOG_1_SECOND_SET;
    
    ret = misc_register(&ak_wdt_miscdev);
    if(ret) {
        pr_info("%s reg miscdev failed.\n", __func__);
        ret = -ENOENT;
    }
    
#if defined (CONFIG_MACH_KM01A) || defined(CONFIG_MACH_AK3918AV130)
    pr_debug("close wdt2 for test\r\n");
    
    __raw_writel(0x00535334,MODULE_WDT_ACCESS);
     regval = __raw_readl(AK_VA_SYSCTRL);
    __raw_writel(0x0,MODULE_WDT_ENB);
     regval = __raw_readl(AK_VA_SYSCTRL);
    __raw_writel(0x0,MODULE_WDT_ACCESS);
     regval = __raw_readl(AK_VA_SYSCTRL);
#endif

    return ret;
}

/**
 * @brief ak_watchdog_remove
 * 
 * @param [in]: struct platform_device *pdev
 * @RETURN: int.
 */
static int ak_watchdog_remove(struct platform_device *pdev)
{
    unregister_reboot_notifier(&ak_wdt_nb);

    misc_deregister(&ak_wdt_miscdev);

    return 0;
}

#ifdef CONFIG_PM

static uint32_t wdt_cfg1;
static uint32_t wdt_cfg2;

/**
 * @brief ak_watchdog_suspend
 * 
 * @param [in]: struct platform_device *pdev, pm_message_t state
 * @RETURN: int.
 */
static int ak_watchdog_suspend(struct platform_device *pdev,
    pm_message_t state)
{
    wdt_cfg1 = __raw_readl(MODULE_WDT_CFG1);
    wdt_cfg2 = __raw_readl(MODULE_WDT_CFG2);

    /* turn it off */
    wdt_disable();

    pr_info("watchdog suspended, wdt_cfg1 is 0x%x, wdt_cfg2 is 0x%x\n",
        wdt_cfg1, wdt_cfg2);
    return 0;
}

/**
 * @brief ak_watchdog_resume
 * 
 * @param [in]: struct platform_device *pdev
 * @RETURN: int.
 */
static int ak_watchdog_resume(struct platform_device *pdev)
{
    u32 regval;
    /* restore watchdog state */
    wdt_cfg1 &= ~(0xffUL << 24);
    wdt_cfg1 |= (0x55 << 24);
    __raw_writel(wdt_cfg1, MODULE_WDT_CFG1);
     regval = __raw_readl(AK_VA_SYSCTRL);

    wdt_cfg2 &= 1;
    wdt_cfg2 |= (0xaaUL << 24);
    __raw_writel(wdt_cfg2, MODULE_WDT_CFG2);
     regval = __raw_readl(AK_VA_SYSCTRL);

    pr_info("watchdog resume %sabled\n", (wdt_cfg2 & 1)? "en": "dis");
    return 0;
}

#else
#define ak_watchdog_suspend NULL
#define ak_watchdog_resume  NULL
#endif /* CONFIG_PM */

static const struct of_device_id ak_watchdog_of_ids[] = {
    { .compatible = "anyka,ak37d-wdt" },
    { .compatible = "anyka,ak39ev330-wdt" },
    { .compatible = "anyka,ak37e-wdt"},
    { .compatible = "anyka,ak3918av100-wdt" },
    { .compatible = "anyka,ak3918ev300l-wdt" },
    { .compatible = "anyka,ak3918av130-wdt" },
    { .compatible = "anyka,km01a-wdt" },
    {},
};
MODULE_DEVICE_TABLE(of, ak_watchdog_of_ids);

static struct platform_driver ak_watchdog_driver = {
    .driver     = {
        .name   = "ak-watchdog",
        .of_match_table = of_match_ptr(ak_watchdog_of_ids),
        .owner  = THIS_MODULE,
    },
    .probe      = ak_watchdog_probe,
    .remove     = ak_watchdog_remove,
    .suspend    = ak_watchdog_suspend,
    .resume     = ak_watchdog_resume,
};

static int __init ak_wdt_init(void)
{   
    return platform_driver_register(&ak_watchdog_driver);
}

static void __exit ak_wdt_exit(void)
{
    platform_driver_unregister(&ak_watchdog_driver);
}
module_init(ak_wdt_init);
module_exit(ak_wdt_exit);

MODULE_DESCRIPTION("ANYKA ak Watchdog driver");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Anyka Microelectronic");
MODULE_ALIAS_MISCDEV(WATCHDOG_MINOR);
MODULE_VERSION("2.0.00");
