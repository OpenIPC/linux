/*
 * linux/drivers/watchdog/gk_wdt.c
 *
 * Author: Steven Yu, <yulindeng@gokemicro.com>
 * Copyright (C) 2012-2015, goke, Inc.
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 */

#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/types.h>
#include <linux/timer.h>
#include <linux/miscdevice.h> /* for MODULE_ALIAS_MISCDEV */
#include <linux/watchdog.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/interrupt.h>
#include <linux/clk.h>
#include <linux/uaccess.h>
#include <linux/io.h>
#include <linux/cpufreq.h>
#include <linux/slab.h>
#include <linux/err.h>

#include <mach/hardware.h>
#include <mach/wdt.h>
#include <mach/rct.h>
#include <mach/io.h>

#define CONFIG_GK_WDT_ATBOOT        (0)
#define CONFIG_GK_WDT_TIMEOUT       (15)

static bool nowayout    = WATCHDOG_NOWAYOUT;
static int tmr_margin   = CONFIG_GK_WDT_TIMEOUT;
static int tmr_atboot   = CONFIG_GK_WDT_ATBOOT;
static int soft_noboot;
static int debug;

module_param(tmr_margin,  int, 0);
module_param(tmr_atboot,  int, 0);
module_param(nowayout,   bool, 0);
module_param(soft_noboot, int, 0);
module_param(debug,   int, 1);

MODULE_PARM_DESC(tmr_margin, "Watchdog tmr_margin in seconds. (default="
        __MODULE_STRING(CONFIG_GK_WDT_TIMEOUT) ")");
MODULE_PARM_DESC(tmr_atboot,
        "Watchdog is started at boot time if set to 1, default="
            __MODULE_STRING(CONFIG_GK_WDT_ATBOOT));
MODULE_PARM_DESC(nowayout, "Watchdog cannot be stopped once started (default="
            __MODULE_STRING(WATCHDOG_NOWAYOUT) ")");
MODULE_PARM_DESC(soft_noboot, "Watchdog action, set to 1 to ignore reboots, "
            "0 to reboot (default 0)");
MODULE_PARM_DESC(debug, "Watchdog debug, set to >1 for debug (default 0)");

static DEFINE_SPINLOCK(wdt_lock);

static int init_mode = WDOG_CTR_RST_EN; //WDOG_CTR_INT_EN | WDOG_CTR_RST_EN
module_param(init_mode, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(init_mode, "Watchdog mode: 0x2=reset, 0x4=irq");

enum gk_wdt_state {
    WDT_CLOSE_STATE_DISABLE,
    WDT_CLOSE_STATE_ALLOW,
};
struct gk_wdt_info
{
    unsigned char __iomem   *regbase;

    struct device           *dev;
    struct resource         *mem;
    unsigned int            irq;

    enum gk_wdt_state   state;

    unsigned int    tmo;
    unsigned int    boot_tmo;

    struct watchdog_device  *wdt_dev;

    unsigned int    ctl_reg;
    unsigned int    init_mode;
    unsigned int    act_timeout;

};

static struct gk_wdt_info *pwdtinfo = NULL;


/* watchdog control routines */

#define DBG(fmt, ...)                   \
do {                            \
    if (debug)                  \
        pr_info(fmt, ##__VA_ARGS__);        \
} while (0)

/* functions */
static int gk_wdt_keepalive(struct watchdog_device *wdd)
{
    struct gk_wdt_info *pinfo = pwdtinfo;

    spin_lock(&wdt_lock);
    gk_wdt_writel((unsigned int)pinfo->regbase + WDOG_RELOAD_OFFSET, pinfo->tmo);
    gk_wdt_writel((unsigned int)pinfo->regbase + WDOG_RESTART_OFFSET, WDT_RESTART_VAL);
    spin_unlock(&wdt_lock);

    return 0;
}

static void __gk_wdt_stop(struct gk_wdt_info *pinfo)
{
    gk_wdt_writel((unsigned int)pinfo->regbase + WDOG_CONTROL_OFFSET, 0);
    while((gk_wdt_readl((unsigned int)pinfo->regbase + WDOG_CONTROL_OFFSET)
        & (WDOG_CTR_RST_EN|WDOG_CTR_INT_EN|WDOG_CTR_EN)) != 0);
}

static int gk_wdt_stop(struct watchdog_device *wdd)
{
    struct gk_wdt_info *pinfo = pwdtinfo;

    spin_lock(&wdt_lock);
    __gk_wdt_stop(pinfo);
    spin_unlock(&wdt_lock);

    return 0;
}

static int __gk_wdt_start(struct gk_wdt_info *pinfo, u32 ctl_reg)
{
    spin_lock(&wdt_lock);

    __gk_wdt_stop(pinfo);

    if (!ctl_reg)
        ctl_reg = pinfo->init_mode | WDOG_CTR_EN;

    if ((ctl_reg & WDOG_CTR_RST_EN) == WDOG_CTR_RST_EN)
    {
        /* Clear the WDT_RST_L_REG to zero */
        //gk_writel(WDT_RST_L_REG, RCT_WDT_RESET_VAL);

        /* Clear software reset bit. */
        //gk_writel(SOFT_RESET_REG, 0x2);
    }

    gk_wdt_writel(WDOG_CONTROL_REG, ctl_reg);
    while(gk_wdt_tstbitsl(WDOG_CONTROL_REG, ctl_reg) != ctl_reg);

    spin_unlock(&wdt_lock);

    DBG("%s: ctl_reg=0x%08x\n", __func__, ctl_reg);

    return 0;

}

static int gk_wdt_start(struct watchdog_device *wdd)
{
    struct gk_wdt_info *pinfo = pwdtinfo;
    __gk_wdt_start(pinfo, 0);
    return 0;
}

static int gk_wdt_is_running(struct gk_wdt_info *pinfo)
{
    return (gk_wdt_readl((unsigned int)pinfo->regbase + WDOG_CONTROL_OFFSET) & WDOG_CTR_EN);
}

static int gk_wdt_set_heartbeat(struct watchdog_device *wdd, u32 timeout)
{
    struct gk_wdt_info *pinfo = pwdtinfo;

    int                 errorCode = 0;
    u32                 freq;
    u32                 max_tmo;

    freq = get_apb_bus_freq_hz();
    if (freq)
        max_tmo = 0xFFFFFFFF / freq;
    else {
        dev_err(pinfo->dev, "freq == 0 !\n");
        errorCode = -EPERM;
        goto wdt_set_heartbeat_exit;
    }

    if (timeout > max_tmo) {
        dev_err(pinfo->dev, "max_tmo is %d, not %d.\n", max_tmo, timeout);
        errorCode = -EINVAL;
        goto wdt_set_heartbeat_exit;
    }

    pinfo->tmo = timeout * freq;
    pinfo->act_timeout = timeout;
    gk_wdt_keepalive(wdd);

    DBG("%s: timeout=%d, freq=%u, count=0x%08x\n", __func__, timeout, freq, pinfo->tmo);

    wdd->timeout = timeout;

wdt_set_heartbeat_exit:
    return errorCode;

}

static const struct watchdog_info gk_wdt_ident = {
    .options          = (WDIOF_SETTIMEOUT | WDIOF_KEEPALIVEPING | WDIOF_MAGICCLOSE),
    .firmware_version = 0,
    .identity         = "GK Watchdog",
};

static struct watchdog_ops gk_wdt_ops = {
    .owner          = THIS_MODULE,
    .start          = gk_wdt_start,
    .stop           = gk_wdt_stop,
    .ping           = gk_wdt_keepalive,
    .set_timeout    = gk_wdt_set_heartbeat,
};

static struct watchdog_device gk_wdd = {
    .info = &gk_wdt_ident,
    .ops = &gk_wdt_ops,
};

/* interrupt handler code */
static irqreturn_t gk_wdt_irq(int irqno, void *devid)
{
    struct gk_wdt_info       *pinfo;

    pinfo = (struct gk_wdt_info *)devid;

    /* clear the IRQ */
    gk_wdt_writel((unsigned int)pinfo->regbase + WDOG_CLR_TMO_OFFSET, 0x01);

    /* Update the timerout register(Feed something to a dog) */
    //gk_wdt_keepalive(pinfo);

    dev_info(pinfo->dev, "Watchdog timer expired!\n");

    return IRQ_HANDLED;
}

static int __devinit gk_wdt_probe(struct platform_device *pdev)
{
    struct device       *wdt_dev;
    struct resource     *wdt_irq;
    struct resource     *wdt_mem;

#if 1
    void __iomem        *wdt_base;
#endif

    struct gk_wdt_info       *pinfo;

    unsigned int wtcon;
    int started = 0;
    int ret;
    //int size;

    printk("[%s]: probe\n", __FUNCTION__);

    DBG("%s: probe=%p\n", __func__, pdev);

    wdt_dev = &pdev->dev;

    wdt_mem = platform_get_resource(pdev, IORESOURCE_MEM, 0);
    if (wdt_mem == NULL) {
        dev_err(wdt_dev, "Get WDT memory resource failed\n");
        return -ENOENT;
    }

    wdt_irq = platform_get_resource(pdev, IORESOURCE_IRQ, 0);
    if (wdt_irq == NULL) {
        dev_err(wdt_dev, "Get WDT irq resource failed\n");
        return -ENOENT;
    }
#if 0
    /* get the memory region for the watchdog timer */
    size = resource_size(wdt_mem);
    if (!request_mem_region(wdt_mem->start, size, pdev->name)) {
        dev_err(wdt_dev, "Failed to get WDT's memory region\n");
        return -EBUSY;
    }

    wdt_base = ioremap(wdt_mem->start, size);
    if (wdt_base == NULL) {
        dev_err(wdt_dev, "failed to ioremap() region\n");
        ret = -EINVAL;
        goto err_req;
    }
#else
	wdt_base = (void __iomem*)wdt_mem->start;
#endif

    DBG("probe: mapped wdt_base=%p\n", wdt_base);
    printk("[%s]: probe mapped wdt_base=%p\n", __FUNCTION__, wdt_base);

    pinfo = kzalloc(sizeof(struct gk_wdt_info), GFP_KERNEL);
    if (pinfo == NULL) {
        dev_err(wdt_dev, "Out of memory!\n");
        ret = -ENOMEM;
        goto err_map;
    }

    pinfo->regbase          = (unsigned char __iomem *)wdt_base;
    pinfo->mem              = wdt_mem;
    pinfo->dev              = &pdev->dev;
    pinfo->irq              = wdt_irq->start;
    pinfo->wdt_dev          = &gk_wdd;
    pinfo->state            = WDT_CLOSE_STATE_DISABLE;
    pinfo->boot_tmo         = gk_wdt_readl((unsigned int)pinfo->regbase + WDOG_TIMEOUT_OFFSET);
    pinfo->init_mode        = (init_mode & (WDOG_CTR_INT_EN | WDOG_CTR_RST_EN));
    platform_set_drvdata(pdev, pinfo);
    pwdtinfo = pinfo;

    /* see if we can actually set the requested timer margin, and if
     * not, try the default value */

    if (gk_wdt_set_heartbeat(pinfo->wdt_dev, tmr_margin))
    {
        started = gk_wdt_set_heartbeat(pinfo->wdt_dev, CONFIG_GK_WDT_TIMEOUT);
        if (started == 0)
            dev_info(wdt_dev,
               "tmr_margin value out of range, default %d used\n", CONFIG_GK_WDT_TIMEOUT);
        else
            dev_info(wdt_dev, "default timer value is out of range, cannot start\n");
    }

    ret = request_irq(wdt_irq->start, gk_wdt_irq, IRQF_TRIGGER_RISING, dev_name(&pdev->dev), pinfo);
    if (ret != 0) {
        dev_err(wdt_dev, "failed to install irq (%d)\n", ret);
        goto err_malloc;
    }

    watchdog_set_nowayout(pinfo->wdt_dev, nowayout);

    ret = watchdog_register_device(pinfo->wdt_dev);
    if (ret) {
        dev_err(wdt_dev, "cannot register watchdog (%d)\n", ret);
        goto err_irq;
    }

    if (tmr_atboot && started == 0)
    {
        dev_info(wdt_dev, "starting watchdog timer\n");
        gk_wdt_start(pinfo->wdt_dev);
    }
    else if (!tmr_atboot)
    {
        /* if we're not enabling the watchdog, then ensure it is
              * disabled if it has been left running from the bootloader
              * or other source */

        gk_wdt_stop(pinfo->wdt_dev);
    }

    /* print out a statement of readiness */

    wtcon = gk_wdt_readl((unsigned int)wdt_base + WDOG_CONTROL_OFFSET);


    printk("watchdog %sactive, reset %sabled, irq %sabled\n",
         (wtcon & WDOG_CTR_EN) ?  "" : "in",
         (wtcon & WDOG_CTR_RST_EN) ? "en" : "dis",
         (wtcon & WDOG_CTR_INT_EN) ? "en" : "dis");


    return 0;

err_irq:
    free_irq(wdt_irq->start, pdev);

err_malloc:
    kfree(pinfo);

err_map:
    //iounmap(wdt_base);

//err_req:
    //release_mem_region(wdt_mem->start, size);

    return ret;

}

static int __devexit gk_wdt_remove(struct platform_device *pdev)
{
    struct gk_wdt_info       *pinfo;
    struct resource         *wdt_mem;

    pinfo = platform_get_drvdata(pdev);

    if (pinfo)
    {
        watchdog_unregister_device(pinfo->wdt_dev);
        gk_wdt_stop(pinfo->wdt_dev);
        free_irq(pinfo->irq, pinfo);
        platform_set_drvdata(pdev, NULL);
        //iounmap((unsigned int)pinfo->regbase);

        wdt_mem = pinfo->mem;
        //release_mem_region(wdt_mem->start, resource_size(wdt_mem));
        kfree(pinfo);
    }

    dev_notice(&pdev->dev,
        "Remove GK Media Processor Watch Dog Timer[%s].\n",
        dev_name(&pdev->dev));

    return 0;

}

static void gk_wdt_shutdown(struct platform_device *pdev)
{
    struct gk_wdt_info       *pinfo;

    pinfo = platform_get_drvdata(pdev);

    if (pinfo)
        gk_wdt_stop(pinfo->wdt_dev);
    else
        dev_err(&pdev->dev, "Cannot find valid pinfo\n");

    dev_dbg(&pdev->dev, "%s exit.\n", __func__);
}

#ifdef CONFIG_PM
static int gk_wdt_suspend(struct platform_device *pdev,
    pm_message_t state)
{
    struct gk_wdt_info       *pinfo;
    int                 errorCode = 0;

    pinfo = platform_get_drvdata(pdev);

    if (pinfo) {
        disable_irq(pinfo->irq);
        pinfo->ctl_reg = gk_wdt_readl((unsigned int)pinfo->regbase + WDOG_CONTROL_OFFSET);
        gk_wdt_stop(pinfo);
    } else {
        dev_err(&pdev->dev, "Cannot find valid pinfo\n");
        errorCode = -ENXIO;
    }

    dev_dbg(&pdev->dev, "%s exit with %d @ %d\n",
        __func__, errorCode, state.event);

    return errorCode;
}

static int gk_wdt_resume(struct platform_device *pdev)
{
    struct gk_wdt_info       *pinfo;
    int                 errorCode = 0;

    pinfo = platform_get_drvdata(pdev);

    if (pinfo) {
        if (pinfo->ctl_reg) {
            gk_wdt_set_heartbeat(pinfo, pinfo->act_timeout);
            gk_wdt_start(pinfo->wdt_dev);
        }
        enable_irq(pinfo->irq);
    } else {
        dev_err(&pdev->dev, "Cannot find valid pinfo\n");
        errorCode = -ENXIO;
    }

    dev_dbg(&pdev->dev, "%s exit with %d\n", __func__, errorCode);

    return errorCode;
}
#endif


#ifdef CONFIG_OF
static const struct of_device_id gk_wdt_match[] = {
    { .compatible = "goke,gk-wdt" },
    {},
};
MODULE_DEVICE_TABLE(of, gk_wdt_match);
#else
#define gk_wdt_match NULL
#endif

static struct platform_driver gk_wdt_driver = {
    .probe      = gk_wdt_probe,
    .remove     = __devexit_p(gk_wdt_remove),
    .shutdown   = gk_wdt_shutdown,
#ifdef CONFIG_PM
    .suspend    = gk_wdt_suspend,
    .resume     = gk_wdt_resume,
#endif
    .driver     = {
        .owner  = THIS_MODULE,
        .name   = "gk-wdt",
        .of_match_table = gk_wdt_match,
    },
};


static int __init gk_wdt_init(void)
{
    pr_info("GK Watchdog Timer, (c) 2014 Goke Microelectronics\n");

    printk("[%s]: init\n", __FUNCTION__);

    return platform_driver_register(&gk_wdt_driver);
}

static void __exit gk_wdt_exit(void)
{
    platform_driver_unregister(&gk_wdt_driver);
}

module_init(gk_wdt_init);
module_exit(gk_wdt_exit);

MODULE_AUTHOR("Goke Microelectronics Inc.");
MODULE_DESCRIPTION("GK Watchdog Device Driver");
MODULE_LICENSE("GPL");
MODULE_ALIAS_MISCDEV(WATCHDOG_MINOR);
MODULE_ALIAS("platform:gk-wdt");
