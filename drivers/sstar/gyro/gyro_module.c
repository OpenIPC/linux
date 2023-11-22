/*
 * gyro_module.c - Sigmastar
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
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/printk.h>
#include <linux/spi/spi.h>
#include <linux/of.h>
#include <linux/gpio.h>
#include <linux/string.h>
#include "gyro_core.h"
#include "gyro.h"

struct gyro_dev *gyro_dev;

static int __init gyro_init(void)
{
    int ret  = 0;
    gyro_dev = kzalloc(sizeof(struct gyro_dev), GFP_KERNEL);
    if (NULL == gyro_dev)
    {
        GYRO_ERR("gyro_dev kzalloc failed");
        ret = -ENOMEM;
        goto err_gyro_dev_alloc;
    }

    ret = gyro_transfer_init(gyro_dev);
    if (ret < 0)
    {
        GYRO_ERR("gyro_transfer_init failed");
        goto err_gyro_transfer_init;
    }

    ret = gyro_sensor_init(gyro_dev);
    if (ret < 0)
    {
        GYRO_ERR("gyro_sensor_init failed");
        goto err_gyro_sensor_init;
    }

    ret = gyro_core_init(gyro_dev);
    if (ret < 0)
    {
        GYRO_ERR("gyro_core_init failed");
        goto err_gyro_core_init;
    }

#ifdef SS_GYRO_CDEV
    ret = gyro_cdev_init(gyro_dev);
    if (ret < 0)
    {
        GYRO_ERR("gyro_cdev_init failed");
        goto err_gyro_cdev_init;
    }
#endif

#ifdef SS_GYRO_SYSFS
    ret = gyro_sysfs_init(gyro_dev);
    if (ret < 0)
    {
        GYRO_ERR("gyro_sysfs_init failed");
        goto err_gyro_sysfs_init;
    }
#endif

    return 0;

#ifdef SS_GYRO_SYSFS
err_gyro_sysfs_init:
#endif
#ifdef SS_GYRO_CDEV
    gyro_cdev_deinit(gyro_dev);
err_gyro_cdev_init:
#endif
    gyro_core_deinit(gyro_dev);
err_gyro_core_init:
    gyro_sensor_deinit(gyro_dev);
err_gyro_sensor_init:
    gyro_transfer_deinit(gyro_dev);
err_gyro_transfer_init:
    kfree(gyro_dev);
err_gyro_dev_alloc:
    return ret;
}

static void __exit gyro_exit(void)
{
#ifdef SS_GYRO_SYSFS
    gyro_sysfs_deinit(gyro_dev);
#endif
#ifdef SS_GYRO_CDEV
    gyro_cdev_deinit(gyro_dev);
#endif
    gyro_core_deinit(gyro_dev);
    gyro_core_deinit(gyro_dev);
    gyro_sensor_deinit(gyro_dev);
    gyro_transfer_deinit(gyro_dev);
    kfree(gyro_dev);
}

module_init(gyro_init);
module_exit(gyro_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("SSTAR");
MODULE_DESCRIPTION("Gyro sensor driver");
