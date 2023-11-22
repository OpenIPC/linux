/*
 * mdrv_gpic.c- Sigmastar
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
#include <linux/init.h>
#include <linux/sched.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/delay.h>
#include <linux/kdev_t.h>
#include <linux/slab.h>
#include <linux/mm.h>
#include <linux/ioport.h>
#include <linux/interrupt.h>
#include <linux/workqueue.h>
#include <linux/poll.h>
#include <linux/wait.h>
#include <linux/cdev.h>
#include <linux/time.h>
#include <linux/timer.h>
#include <linux/device.h>
#include <asm/io.h>
#include <linux/semaphore.h>
#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/of_device.h>
#include <linux/err.h>
#include <linux/irq.h>
#include <linux/io.h>
#include <linux/gpio.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <linux/module.h>
#include <linux/irqchip/chained_irq.h>
#include <linux/pinctrl/consumer.h>
#include <linux/pm.h>

#include <mhal_gpic.h>
#include <mdrv_gpic.h>
#include <ms_platform.h>
#include <cam_os_wrapper.h>
#include <cam_os_util_list.h>

#if 0
#define gpic_dbg(fmt, ...) printk(KERN_ERR "[GPIC] " fmt, ##__VA_ARGS__)
#else
#define gpic_dbg(fmt, ...)
#endif
#define gpic_err(fmt, ...) printk(KERN_ERR "[GPIC] " fmt, ##__VA_ARGS__)

struct gpic_data_t
{
    u32 group;
    u32 irqnum;
    u8  irqname[32];
    void (*callback)(void *);
    void *                 para;
    struct hal_gpic_t      hal;
    struct CamOsListHead_t list;
};

static CAM_OS_LIST_HEAD(camdriver_gpic_list);

static const struct of_device_id camdriver_gpic_of_match[] = {
    {.compatible = "sstar,gpic"},
    {},
};

void *camdriver_gpic_get_handler(u8 group)
{
    struct gpic_data_t *gpic_data = NULL;
    CAM_OS_LIST_FOR_EACH_ENTRY(gpic_data, &camdriver_gpic_list, list)
    {
        if (gpic_data->group == group)
        {
            return (void *)gpic_data;
        }
    }
    return NULL;
}
EXPORT_SYMBOL(camdriver_gpic_get_handler);

int camdriver_gpic_config(void *handler, GpicCfg_t *cfg)
{
    struct gpic_data_t *gpic_data = NULL;

    if (!handler || !cfg)
        return -1;

    gpic_data = (struct gpic_data_t *)handler;

    gpic_data->hal.pad    = cfg->pad;
    gpic_data->hal.dir    = cfg->dir;
    gpic_data->hal.edge   = cfg->edge;
    gpic_data->hal.init   = cfg->init;
    gpic_data->hal.target = cfg->target;
    gpic_data->callback   = cfg->callback;
    gpic_data->para       = cfg->para;

    return 0;
}
EXPORT_SYMBOL(camdriver_gpic_config);

int camdriver_gpic_enable(void *handler)
{
    struct gpic_data_t *gpic_data = NULL;

    if (!handler)
        return -1;

    gpic_data = (struct gpic_data_t *)handler;
    return MHal_GPIC_Enable(&gpic_data->hal);
}
EXPORT_SYMBOL(camdriver_gpic_enable);

int camdriver_gpic_disable(void *handler)
{
    struct gpic_data_t *gpic_data = NULL;

    if (!handler)
        return -1;

    gpic_data = (struct gpic_data_t *)handler;
    return MHal_GPIC_Disable(&gpic_data->hal);
}
EXPORT_SYMBOL(camdriver_gpic_disable);

int camdriver_gpic_get_count(void *handler, u16 *count)
{
    struct gpic_data_t *gpic_data = NULL;

    if (!handler || !count)
        return -1;

    gpic_data = (struct gpic_data_t *)handler;
    return MHal_GPIC_GetCount(&gpic_data->hal, count);
}
EXPORT_SYMBOL(camdriver_gpic_get_count);

static void camdriver_gpic_interrupt(u32 irq, void *para)
{
    struct gpic_data_t *gpic_data = (struct gpic_data_t *)para;
    gpic_dbg("[%s] %px\n", __FUNCTION__, gpic_data);
    if (gpic_data)
    {
        if (gpic_data->callback)
        {
            gpic_data->callback(gpic_data->para);
        }
    }
}

static int camdriver_gpic_probe(struct platform_device *pdev)
{
    int                 err;
    u32                 group;
    u32                 irqnum;
    void __iomem *      base;
    struct resource *   resource;
    struct gpic_data_t *gpic_data;

    err = of_property_read_u32(pdev->dev.of_node, "group", &group);
    if (err)
    {
        gpic_err("get group property fail\n");
        return err;
    }

    irqnum = irq_of_parse_and_map(pdev->dev.of_node, 0);
    if (irqnum == 0)
    {
        gpic_err("get group %d interrupts property fail\n", group);
        return -ENOENT;
    }

    resource = platform_get_resource(pdev, IORESOURCE_MEM, 0);
    if (!resource)
    {
        gpic_err("get group %d reg property fail\n", group);
        return -ENOENT;
    }
    base = (void *)(IO_ADDRESS(resource->start));

    gpic_dbg("group %d irq number is %d\n", group, irqnum);

    gpic_data = CamOsMemAlloc(sizeof(struct gpic_data_t));
    if (!gpic_data)
    {
        err = -ENOMEM;
        goto mem_err;
    }

    gpic_dbg("group %d data pointer %px\n", group, gpic_data);

    if (!snprintf(gpic_data->irqname, sizeof(gpic_data->irqname), "gipc%d", group))
    {
        gpic_err("group %d irq name reformat failed\n", group);
        err = -ENOENT;
        goto mem_err;
    }

    gpic_data->group    = group;
    gpic_data->irqnum   = irqnum;
    gpic_data->hal.base = (u32)base;
    platform_set_drvdata(pdev, gpic_data);

    err = CamOsIrqRequest(irqnum, camdriver_gpic_interrupt, gpic_data->irqname, gpic_data);
    if (err == 0)
    {
        gpic_dbg("%s registered\n", gpic_data->irqname);
    }
    else
    {
        gpic_err("%s register fail\n", gpic_data->irqname);
        goto irq_err;
    }

    CAM_OS_LIST_ADD_TAIL(&gpic_data->list, &camdriver_gpic_list);

    return 0;

irq_err:
    CamOsIrqFree(irqnum, gpic_data);
mem_err:
    CamOsMemRelease(gpic_data);

    return err;
}

#ifdef CONFIG_PM_SLEEP
static int camdriver_gpic_suspend(struct device *dev)
{
    struct platform_device *pdev      = to_platform_device(dev);
    struct gpic_data_t *    gpic_data = platform_get_drvdata(pdev);

    if (gpic_data->hal.enable)
    {
        if (MHal_GPIC_GetCount(&gpic_data->hal, &gpic_data->hal.init))
            return -EIO;
    }

    return 0;
}

static int camdriver_gpic_resume(struct device *dev)
{
    struct platform_device *pdev      = to_platform_device(dev);
    struct gpic_data_t *    gpic_data = platform_get_drvdata(pdev);

    if (gpic_data->hal.enable)
    {
        if (MHal_GPIC_Enable(&gpic_data->hal))
            return -EIO;
    }

    return 0;
}
#else
#define camdriver_gpic_suspend NULL
#define camdriver_gpic_resume  NULL
#endif

static const struct dev_pm_ops camdriver_gpic_pm_ops = {
    .suspend = camdriver_gpic_suspend,
    .resume  = camdriver_gpic_resume,
};

static struct platform_driver camdriver_gpic_driver = {
    .driver =
        {
            .name           = "gpic",
            .owner          = THIS_MODULE,
            .of_match_table = camdriver_gpic_of_match,
            .pm             = &camdriver_gpic_pm_ops,
        },
    .probe = camdriver_gpic_probe,
};

static int __init camdriver_gpic_init(void)
{
    return platform_driver_register(&camdriver_gpic_driver);
}
postcore_initcall(camdriver_gpic_init);

MODULE_AUTHOR("SSTAR");
MODULE_DESCRIPTION("GPIC driver");
MODULE_LICENSE("GPL");
