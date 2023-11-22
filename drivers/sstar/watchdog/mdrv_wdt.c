/*
 * mdrv_wdt.c- Sigmastar
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
#include <linux/of_irq.h>
#include <linux/of_platform.h>
#include <linux/clk-provider.h>
#include "../include/ms_types.h"
#include "../include/ms_platform.h"
#include "mdrv_wdt.h"
#ifdef CONFIG_SS_DUALOS
#include "drv_dualos.h"
#include "cam_inter_os.h"
#endif
#include "drv_camclk_Api.h"
#include "registers.h"

//#define OPEN_WDT_DEBUG

#ifdef OPEN_WDT_DEBUG // switch printk
#define wdtDbg printk
#else
#define wdtDbg(...)
#endif
#define wdtErr                       printk
#define OPTIONS                      (WDIOF_SETTIMEOUT | WDIOF_KEEPALIVEPING | WDIOF_PRETIMEOUT)
#define CONFIG_WATCHDOG_ATBOOT       (0)
#define CONFIG_WATCHDOG_DEFAULT_TIME (10) // unit=SEC
#define REPEAT_DELAY                 500  // unit=10ms

struct sstar_wdt
{
    u32                     irq;
    struct device *         dev;
    struct watchdog_device *wdt_device;
    phys_addr_t             reg_base;
    spinlock_t              lock;
    u32                     clock;
#ifdef CONFIG_PM_SLEEP
    u8 opened;
#endif
    unsigned long next_hb;
    unsigned int  margin;
};

#ifdef CONFIG_SS_DUALOS
static int write_riu_via_rtos(u32 addr, u16 val)
{
    rtkinfo_t *rtk = NULL;

    rtk = get_rtkinfo();
    if (rtk)
    {
        snprintf(rtk->sbox, sizeof(rtk->sbox), "regset 0x%08X 0x%04X", (u32)addr, val);
        CamInterOsSignal(INTEROS_SC_L2R_RTK_CLI, 0, 0, 0);
    }
    else
    {
        pr_warn("[WatchDog] Get RTOS handle fail\n");
    }

    return 0;
}
#endif

static u32 __sstar_wdt_get_clk_rate(struct sstar_wdt *wdt)
{
#ifdef CONFIG_CAM_CLK
    u32 WdtClk = 0;
#else
    struct clk *   clk;
    struct clk_hw *clk_hw;
#endif
    u8  i;
    u8  num;
    u32 rate = 0;
    u32 pare_rate;

#ifdef CONFIG_CAM_CLK
    of_property_read_u32_index(wdt->dev.of_node, "camclk", 0, &(WdtClk));
    if (!WdtClk)
    {
        printk(KERN_DEBUG "[%s] Fail to get clk!\n", __func__);
    }
    else
    {
        rate = CamClkRateGet(WdtClk);
    }
#else
    clk = of_clk_get(wdt->dev->of_node, 0);
    if (IS_ERR(clk))
    {
        return 0;
    }

    num = clk_hw_get_num_parents(__clk_get_hw(clk));
    for (i = 0; i < num; i++)
    {
        clk_hw    = clk_hw_get_parent_by_index(__clk_get_hw(clk), i);
        pare_rate = (u32)clk_get_rate(clk_hw->clk);
        if (i == 0)
        {
            rate = pare_rate;
        }
        else
        {
            rate = min(rate, pare_rate);
        }
    }

    if (clk_set_rate(clk, rate))
    {
        rate = 0;
    }

    clk_put(clk);
#endif
    return rate;
}

static void check_osc_clk(struct sstar_wdt *wdt)
{
    wdt->clock = __sstar_wdt_get_clk_rate(wdt);
    if (wdt->clock == 0)
    {
        wdt->clock = OSC_CLK_12000000;
    }
}

static void __sstar_wdt_stop(struct sstar_wdt *wdt)
{
    wdtDbg("__sstar_wdt_stop \n");
    OUTREG16(wdt->reg_base + WDT_WDT_CLR, 0);
    OUTREG16(wdt->reg_base + WDT_MAX_PRD_H, 0x0000);
    OUTREG16(wdt->reg_base + WDT_MAX_PRD_L, 0x0000);
}

static int sstar_wdt_stop(struct watchdog_device *wdd)
{
    struct sstar_wdt *wdt = watchdog_get_drvdata(wdd);

    wdtDbg("[WatchDog]sstar_wdt_stop \n");

    spin_lock(&wdt->lock);
    __sstar_wdt_stop(wdt);
#ifdef CONFIG_PM_SLEEP
    wdt->opened = 0;
#endif
    spin_unlock(&wdt->lock);

    return 0;
}

static void __sstar_wdt_start(struct sstar_wdt *wdt, u32 launch_time)
{
    wdtDbg("__sstar_wdt_start \n");

    OUTREG16(wdt->reg_base + WDT_INTR_PERIOD, 0xFFFF);

    OUTREG16(wdt->reg_base + WDT_WDT_CLR, CLEAR_WDT);
    OUTREG16(wdt->reg_base + WDT_MAX_PRD_H, (((wdt->clock * launch_time) >> 16) & 0xFFFF));
    OUTREG16(wdt->reg_base + WDT_MAX_PRD_L, ((wdt->clock * launch_time) & 0xFFFF));
}

static int sstar_wdt_start(struct watchdog_device *wdd)
{
    unsigned long     current_jiffies;
    struct sstar_wdt *wdt = watchdog_get_drvdata(wdd);

    wdtDbg("[WatchDog]sstar_wdt_start \n");
    spin_lock(&wdt->lock);

    __sstar_wdt_stop(wdt);

    __sstar_wdt_start(wdt, wdt->margin);

    current_jiffies = jiffies;
    wdt->next_hb    = current_jiffies + REPEAT_DELAY * wdt->margin;

#ifdef CONFIG_PM_SLEEP
    wdt->opened = 1;
#endif
    spin_unlock(&wdt->lock);
    return 0;
}

static int sstar_wdt_set_timeout(struct watchdog_device *wdd, unsigned int timeout)
{
    unsigned long     current_jiffies;
    struct sstar_wdt *wdt = watchdog_get_drvdata(wdd);

    wdtDbg("[WatchDog]sstar_wdt_set_timeout=%d \n", timeout);
    if (timeout < 5)
        timeout = 5;

    current_jiffies = jiffies;
    wdt->next_hb    = current_jiffies + REPEAT_DELAY * timeout;

    spin_lock(&wdt->lock);
    wdt->margin = timeout;
    OUTREG16(wdt->reg_base + WDT_MAX_PRD_H, (((wdt->clock * wdt->margin) >> 16) & 0xFFFF));
    OUTREG16(wdt->reg_base + WDT_MAX_PRD_L, ((wdt->clock * wdt->margin) & 0xFFFF));

    spin_unlock(&wdt->lock);
    wdtDbg("[WatchDog]sstar_wdt_set_timeout data=%lx \r\n", (unsigned long)wdt->margin);
    return 0;
}

static int sstar_wdt_set_pretimeout(struct watchdog_device *wdd, unsigned int timeout)
{
    struct sstar_wdt *wdt = watchdog_get_drvdata(wdd);

    wdtDbg("[WatchDog]sstar_wdt_set_pretimeout=%d \n", timeout);

    spin_lock(&wdt->lock);
    OUTREG16(wdt->reg_base + WDT_INTR_PERIOD, (((wdt->clock * timeout) >> 16) & 0xFFFF));

    spin_unlock(&wdt->lock);
    wdtDbg("[WatchDog]sstar_wdt_set_pretimeout data=%lx \r\n", (unsigned long)timeout);
    return 0;
}

static int sstar_wdt_ping(struct watchdog_device *wdd)
{
    struct sstar_wdt *wdt = watchdog_get_drvdata(wdd);

    /* If we got a heartbeat pulse within the WDT_US_INTERVAL
     * we agree to ping the WDT
     */
    unsigned long current_jiffies;

    if (time_before(jiffies, wdt->next_hb))
    {
        // wdtDbg("[WatchDog] sstar_wdt_ping tmr_margin=%lx \r\n",(jiffies-next_heartbeat));

        /* Ping the WDT */
#ifdef CONFIG_SS_DUALOS
        // write_riu_via_rtos(BASE_REG_WDT_PA + WDT_MAX_PRD_H, (((g_u32Clock*tmr_margin)>>16) & 0xFFFF));
        // write_riu_via_rtos(BASE_REG_WDT_PA + WDT_MAX_PRD_L, ((g_u32Clock*tmr_margin) & 0xFFFF));
        write_riu_via_rtos(BASE_REG_WDT_PA + WDT_WDT_CLR, CLEAR_WDT);
        spin_lock(&wdt->lock);
#else
        spin_lock(&wdt->lock);
        // OUTREG16(BASE_REG_WDT_PA + WDT_MAX_PRD_H, (((g_u32Clock*tmr_margin)>>16) & 0xFFFF));
        // OUTREG16(BASE_REG_WDT_PA + WDT_MAX_PRD_L, ((g_u32Clock*tmr_margin) & 0xFFFF));
        OUTREG16(wdt->reg_base + WDT_WDT_CLR, CLEAR_WDT);
#endif
        current_jiffies = jiffies;
        wdt->next_hb    = current_jiffies + REPEAT_DELAY * wdt->margin;

        spin_unlock(&wdt->lock);
        wdtDbg("[WatchDog] sstar_wdt_ping tmr_margin=%x \r\n", wdt->margin);
    }
    else
    {
        pr_warn("Heartbeat lost! Will not ping the watchdog\n");
    }

    return 0;
}

static int sstar_wdt_set_heartbeat(struct watchdog_device *wdd, unsigned int timeout)
{
    struct sstar_wdt *wdt = watchdog_get_drvdata(wdd);

    if (timeout < 1)
        return -EINVAL;

    wdtDbg("[WatchDog]sstar_wdt_set_heartbeat \n");

    OUTREG16(wdt->reg_base + WDT_WDT_CLR, CLEAR_WDT);

    wdd->timeout = timeout;

    return 0;
}

static irqreturn_t sstar_wdt_interrupt(int irq, void *dev_id)
{
    struct sstar_wdt *wdt = (struct sstar_wdt *)dev_id;

    printk("[%s]\n", __func__);

    watchdog_notify_pretimeout(wdt->wdt_device);

    return IRQ_HANDLED;
}

static const struct watchdog_info sstar_wdt_ident = {
    .options          = OPTIONS,
    .firmware_version = 0,
    .identity         = "SSTAR Watchdog",
};

static struct watchdog_ops sstar_wdt_ops = {
    .owner          = THIS_MODULE,
    .start          = sstar_wdt_start,
    .stop           = sstar_wdt_stop,
    .set_timeout    = sstar_wdt_set_timeout,
    .set_pretimeout = sstar_wdt_set_pretimeout,
    .ping           = sstar_wdt_ping,
};

static int sstar_wdt_probe(struct platform_device *pdev)
{
    int                     ret = 0;
    struct device *         dev;
    struct sstar_wdt *      wdt;
    struct watchdog_device *wdd;
    int                     started = 0;
    struct resource *       res;

    wdtDbg("[WatchDog]sstar_wdt_probe \n");
    dev = &pdev->dev;
    wdt = devm_kzalloc(dev, sizeof(struct sstar_wdt), GFP_KERNEL);
    if (!wdt)
        return -ENOMEM;

    wdd = devm_kzalloc(dev, sizeof(struct watchdog_device), GFP_KERNEL);
    if (!wdd)
        return -ENOMEM;

    spin_lock_init(&wdt->lock);

    wdt->dev    = &pdev->dev;
    wdt->margin = CONFIG_WATCHDOG_DEFAULT_TIME;

    res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
    if (!res)
    {
        wdtDbg("[%s]: failed to get IORESOURCE_MEM\n", __func__);
        return -ENODEV;
    }
    wdt->reg_base = res->start;

    wdt->irq = irq_of_parse_and_map(pdev->dev.of_node, 0);
    if (wdt->irq == 0)
    {
        wdtDbg("[%s]: can't find interrupts property\n", __func__);
        return -ENOENT;
    }

    if (devm_request_irq(dev, wdt->irq, sstar_wdt_interrupt, 0, "WDT Interrupt", wdt))
    {
        wdtDbg("[%s]: WDT Interrupt register failed\n", __func__);
        return -ENOENT;
    }

    check_osc_clk(wdt);
    wdd->ops        = &sstar_wdt_ops;
    wdd->info       = &sstar_wdt_ident;
    wdt->wdt_device = wdd;

    platform_set_drvdata(pdev, wdt);
    watchdog_set_drvdata(wdt->wdt_device, wdt);

    if (sstar_wdt_set_heartbeat(wdd, wdt->margin))
    {
        started = sstar_wdt_set_heartbeat(wdd, CONFIG_WATCHDOG_DEFAULT_TIME);

        if (started == 0)
            dev_info(dev, "tmr_margin value out of range, default %d used\n", CONFIG_WATCHDOG_DEFAULT_TIME);
        else
            dev_info(dev, "default timer value is out of range, cannot start\n");
    }

    ret = watchdog_register_device(wdd);
    if (ret)
    {
        dev_err(dev, "cannot register watchdog (%d)\n", ret);
        goto err;
    }

    return 0;

err:
    watchdog_unregister_device(wdd);

    return ret;
}

static int sstar_wdt_remove(struct platform_device *dev)
{
    struct sstar_wdt *wdt = platform_get_drvdata(dev);

    wdtDbg("[WatchDog]sstar_wdt_remove\n");
    sstar_wdt_stop(wdt->wdt_device);
    watchdog_unregister_device(wdt->wdt_device);

    return 0;
}

static void sstar_wdt_shutdown(struct platform_device *dev)
{
    struct sstar_wdt *wdt = platform_get_drvdata(dev);

    wdtDbg("[WatchDog]sstar_wdt_shutdown\n");

    sstar_wdt_stop(wdt->wdt_device);
}

#ifdef CONFIG_PM_SLEEP
static int sstar_wdt_suspend(struct platform_device *dev, pm_message_t state)
{
    struct sstar_wdt *wdt = platform_get_drvdata(dev);

    wdtDbg("[WatchDog]sstar_wdt_suspend\n");
    if (wdt->opened == 1)
    {
        sstar_wdt_stop(wdt->wdt_device);
        wdt->opened = 1; // flag is clear by sstar_wdt_stop, need to be set again for resume
    }
    return 0;
}

static int sstar_wdt_resume(struct platform_device *dev)
{
    struct sstar_wdt *wdt = platform_get_drvdata(dev);

    wdtDbg("[WatchDog]sstar_wdt_resume\n");
    /* Restore watchdog state. */
    if (wdt->opened == 1)
    {
        sstar_wdt_start(wdt->wdt_device);
    }
    return 0;
}
#else
#define sstar_wdt_suspend NULL
#define sstar_wdt_resume  NULL
#endif /* CONFIG_PM_SLEEP */

static const struct of_device_id sstar_wdt_of_match_table[] = {{.compatible = "sstar,wdt"}, {}};
MODULE_DEVICE_TABLE(of, sstar_wdt_of_match_table);

static struct platform_driver sstar_wdt_driver = {
    .probe    = sstar_wdt_probe,
    .remove   = sstar_wdt_remove,
    .shutdown = sstar_wdt_shutdown,
#ifdef CONFIG_PM_SLEEP
    .suspend = sstar_wdt_suspend,
    .resume  = sstar_wdt_resume,
#endif
    .driver =
        {
            .owner          = THIS_MODULE,
            .name           = "sstar-wdt",
            .of_match_table = sstar_wdt_of_match_table,
        },
};

module_platform_driver(sstar_wdt_driver);

MODULE_AUTHOR("SSTAR");
MODULE_DESCRIPTION("SSTAR Watchdog Device Driver");
MODULE_LICENSE("GPL");
MODULE_ALIAS_MISCDEV(WATCHDOG_MINOR);
MODULE_ALIAS("platform:sstar-wdt");
