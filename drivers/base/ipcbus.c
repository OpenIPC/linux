/*!
*****************************************************************************
** \file        drivers/base/ipcbus.c
**
** \version     $Id$
**
** \brief
**
** \attention   THIS SAMPLE CODE IS PROVIDED AS IS. GOKE MICROELECTRONICS
**              ACCEPTS NO RESPONSIBILITY OR LIABILITY FOR ANY ERRORS OR
**              OMMISSIONS
**
** (C) Copyright 2012-2015 by GOKE MICROELECTRONICS CO.,LTD
**
*****************************************************************************
*/

#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/dma-mapping.h>
#include <linux/ipcbus.h>

static struct device gk_bus = {
    .init_name = "ipc_bus"
};

struct ipc_dev {
    struct device dev;
    struct device *next;
};

#define to_gk_dev(x) container_of((x), struct ipc_dev, dev)

static int gk_bus_match(struct device *dev, struct device_driver *driver)
{
    struct ipc_driver *ipc_driver = to_ipc_driver(driver);

    if (dev->platform_data == ipc_driver) {
        if (!ipc_driver->match || ipc_driver->match(dev))
            return 1;
        dev->platform_data = NULL;
    }
    return 0;
}

static int gk_bus_probe(struct device *dev)
{
    struct ipc_driver *ipc_driver = dev->platform_data;

    if (ipc_driver->probe)
        return ipc_driver->probe(dev);

    return 0;
}

static int gk_bus_remove(struct device *dev)
{
    struct ipc_driver *ipc_driver = dev->platform_data;

    if (ipc_driver->remove)
        return ipc_driver->remove(dev);

    return 0;
}

static void gk_bus_shutdown(struct device *dev)
{
    struct ipc_driver *ipc_driver = dev->platform_data;

    if (ipc_driver->shutdown)
        ipc_driver->shutdown(dev);
}

static int gk_bus_suspend(struct device *dev, pm_message_t state)
{
    struct ipc_driver *ipc_driver = dev->platform_data;

    if (ipc_driver->suspend)
        return ipc_driver->suspend(dev, state);

    return 0;
}

static int gk_bus_resume(struct device *dev)
{
    struct ipc_driver *ipc_driver = dev->platform_data;

    if (ipc_driver->resume)
        return ipc_driver->resume(dev);

    return 0;
}

static struct bus_type gk_bus_type = {
    .name       = "ipc_bus",
    .match      = gk_bus_match,
    .probe      = gk_bus_probe,
    .remove     = gk_bus_remove,
    .shutdown   = gk_bus_shutdown,
    .suspend    = gk_bus_suspend,
    .resume     = gk_bus_resume
};

static void gk_dev_release(struct device *dev)
{
    kfree(to_gk_dev(dev));
}

void ipc_unregister_driver(struct ipc_driver *ipc_driver)
{
    struct device *dev = ipc_driver->devices;

    while (dev) {
        struct device *tmp = to_gk_dev(dev)->next;
        device_unregister(dev);
        dev = tmp;
    }
    driver_unregister(&ipc_driver->driver);
}
EXPORT_SYMBOL(ipc_unregister_driver);

int ipc_register_driver(struct ipc_driver *ipc_driver)
{
    int error;
    unsigned int id;

    ipc_driver->driver.bus    = &gk_bus_type;
    ipc_driver->devices    = NULL;

    error = driver_register(&ipc_driver->driver);
    if (error)
        return error;

    for (id = 0; id < 1; id++) {
        struct ipc_dev *ipc_dev;

        ipc_dev = kzalloc(sizeof *ipc_dev, GFP_KERNEL);
        if (!ipc_dev) {
            error = -ENOMEM;
            break;
        }

        ipc_dev->dev.parent = &gk_bus;
        ipc_dev->dev.bus    = &gk_bus_type;

        dev_set_name(&ipc_dev->dev, "%s",
                 ipc_driver->driver.name);
        ipc_dev->dev.platform_data  = ipc_driver;
        ipc_dev->dev.release        = gk_dev_release;

        ipc_dev->dev.coherent_dma_mask = DMA_BIT_MASK(24);
        ipc_dev->dev.dma_mask = &ipc_dev->dev.coherent_dma_mask;

        error = device_register(&ipc_dev->dev);
        if (error) {
            put_device(&ipc_dev->dev);
            break;
        }

        if (ipc_dev->dev.platform_data) {
            ipc_dev->next = ipc_driver->devices;
            ipc_driver->devices = &ipc_dev->dev;
        } else
            device_unregister(&ipc_dev->dev);
    }

    if (!error && !ipc_driver->devices)
        error = -ENODEV;

    if (error)
        ipc_unregister_driver(ipc_driver);

    return error;
}
EXPORT_SYMBOL(ipc_register_driver);

static int __init gk_bus_init(void)
{
    int error;

    error = bus_register(&gk_bus_type);
    if (!error) {
        error = device_register(&gk_bus);
        if (error)
            bus_unregister(&gk_bus_type);
    }
    return error;
}

device_initcall(gk_bus_init);
