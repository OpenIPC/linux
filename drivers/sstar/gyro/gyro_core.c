/*
 * gyro_core.c - Sigmastar
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
#include <linux/init.h>
#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/export.h>
#include <linux/of.h>
#include <linux/printk.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/err.h>
#include <linux/delay.h>
#include <linux/irq.h>
#include <linux/of_irq.h>
#include <linux/gpio.h>
#include <linux/of_gpio.h>
#include <linux/interrupt.h>
#include <linux/kthread.h>
#include <linux/sched.h>
#include <linux/wait.h>
#include <linux/mutex.h>
#include <linux/kfifo.h>
#include <linux/timekeeping.h>
#include <asm/uaccess.h>
#include <linux/workqueue.h>

#include <asm/string.h>
#include "gyro.h"
#include "gyro_core.h"
#include "gyro_internal.h"

static struct gyro_dev *gyro_cdev;
static atomic_t         use_count;

int gyro_set_sample_rate_div(struct gyro_arg_sample_rate arg)
{
    if (!gyro_cdev->sensor_ops->set_sample_rate_div)
    {
        GYRO_ERR("gyro_set_sample_rate_div is not support.");
        return -EFAULT;
    }
    return gyro_cdev->sensor_ops->set_sample_rate_div(gyro_cdev, arg.rate);
}

int gyro_get_sample_rate_div(struct gyro_arg_sample_rate *arg)
{
    if (!gyro_cdev->sensor_ops->get_sample_rate_div)
    {
        GYRO_ERR("gyro_get_sample_rate_div is not support.");
        return -EFAULT;
    }
    if (NULL == arg)
    {
        return -EINVAL;
    }
    return gyro_cdev->sensor_ops->get_sample_rate_div(gyro_cdev, &arg->rate);
}

int gyro_set_gyro_range(struct gyro_arg_gyro_range arg)
{
    if (!gyro_cdev->sensor_ops->set_gyro_range)
    {
        GYRO_ERR("gyro_set_gyro_range is not support.");
        return -EFAULT;
    }
    return gyro_cdev->sensor_ops->set_gyro_range(gyro_cdev, (u8)arg.range);
}

int gyro_set_accel_range(struct gyro_arg_accel_range arg)
{
    if (!gyro_cdev->sensor_ops->set_accel_range)
    {
        GYRO_ERR("gyro_set_accel_range is not support.");
        return -EFAULT;
    }
    return gyro_cdev->sensor_ops->set_accel_range(gyro_cdev, (u8)arg.range);
}

int gyro_get_gyro_range(struct gyro_arg_gyro_range *arg)
{
    if (!gyro_cdev->sensor_ops->get_gyro_range)
    {
        GYRO_ERR("gyro_get_gyro_range is not support.");
        return -EFAULT;
    }
    if (NULL == arg)
    {
        return -EINVAL;
    }
    return gyro_cdev->sensor_ops->get_gyro_range(gyro_cdev, &arg->range);
}

int gyro_get_gyro_sensitivity(struct gyro_arg_sensitivity *arg)
{
    if (!gyro_cdev->sensor_ops->get_gyro_sensitivity)
    {
        GYRO_ERR("gyro_get_gyro_sensitivity is not support.");
        return -EFAULT;
    }
    if (NULL == arg)
    {
        return -EINVAL;
    }
    return gyro_cdev->sensor_ops->get_gyro_sensitivity(gyro_cdev, &arg->num, &arg->den);
}

int gyro_get_accel_range(struct gyro_arg_accel_range *arg)
{
    if (!gyro_cdev->sensor_ops->get_accel_range)
    {
        GYRO_ERR("gyro_get_accel_range is not support.");
        return -EFAULT;
    }
    if (NULL == arg)
    {
        return -EINVAL;
    }
    return gyro_cdev->sensor_ops->get_accel_range(gyro_cdev, &arg->range);
}

int gyro_get_accel_sensitivity(struct gyro_arg_sensitivity *arg)
{
    if (!gyro_cdev->sensor_ops->get_accel_sensitivity)
    {
        GYRO_ERR("gyro_get_accel_sensitivity is not support.");
        return -EFAULT;
    }
    if (NULL == arg)
    {
        return -EINVAL;
    }
    return gyro_cdev->sensor_ops->get_accel_sensitivity(gyro_cdev, &arg->num, &arg->den);
}

int gyro_read_fifodata(u8 *fifo_data, u16 fifo_cnt)
{
    if (!gyro_cdev->sensor_ops->read_fifo_data)
    {
        GYRO_ERR("gyro_read_fifodata is not support.");
        return -EFAULT;
    }
    if (NULL == fifo_data)
    {
        return -EINVAL;
    }
    return gyro_cdev->sensor_ops->read_fifo_data(gyro_cdev, fifo_data, fifo_cnt);
}

int gyro_read_fifocnt(u16 *fifo_cnt)
{
    if (!gyro_cdev->sensor_ops->read_fifo_cnt)
    {
        GYRO_ERR("gyro_read_fifocnt is not support.");
        return -EFAULT;
    }
    if (NULL == fifo_cnt)
    {
        return -EINVAL;
    }
    return gyro_cdev->sensor_ops->read_fifo_cnt(gyro_cdev, fifo_cnt);
}

int gyro_reset_fifo(void)
{
    if (!gyro_cdev->sensor_ops->reset_fifo)
    {
        GYRO_ERR("gyro_reset_fifo is nut support.");
        return -EFAULT;
    }
    return gyro_cdev->sensor_ops->reset_fifo(gyro_cdev);
}

int gyro_read_gyro_xyz(struct gyro_arg_gyro_xyz *arg)
{
    if (NULL == arg)
    {
        return -EINVAL;
    }

    return 0;
}

int gyro_read_accel_xyz(struct gyro_arg_accel_xyz *arg)
{
    if (NULL == arg)
    {
        return -EINVAL;
    }

    return 0;
}

int gyro_read_temp(struct gyro_arg_temp *arg)
{
    if (NULL == arg)
    {
        return -EINVAL;
    }

    return 0;
}

int gyro_set_dev_mode(struct gyro_arg_dev_mode dev_mode, struct gyro_arg_fifo_info *fifo_info)
{
    if (!gyro_cdev->sensor_ops->enable_fifo)
    {
        GYRO_ERR("gyro_set_dev_mode is not support.");
        return -EFAULT;
    }
    if (NULL == fifo_info)
    {
        return -EINVAL;
    }
    memset(fifo_info, 0xff, sizeof(struct gyro_arg_fifo_info));
    fifo_info->is_big_endian = 1;
    return gyro_cdev->sensor_ops->enable_fifo(gyro_cdev, dev_mode, fifo_info);
}

int gyro_enable(void)
{
    int ret = 0;

    if (atomic_read(&use_count) > 0)
    {
        atomic_inc(&use_count);
        return 0;
    }

    /* Init gyro device */
    if (!gyro_cdev->sensor_ops->init)
    {
        GYRO_ERR("gyro_init is not support.");
        ret = -EFAULT;
        goto err_init_dev;
    }
    ret = gyro_cdev->sensor_ops->init(gyro_cdev);
    if (ret < 0)
    {
        GYRO_ERR("gyro_device_init failed.");
        goto err_init_dev;
    }

    atomic_inc(&use_count);
    return 0;

err_init_dev:
    atomic_dec(&use_count);
    return ret;
}

int gyro_disable(void)
{
    atomic_dec(&use_count);
    if (atomic_read(&use_count) > 0)
    {
        return 0;
    }

    /* Reset */
    memset(&gyro_cdev->gyro_info, 0, sizeof(gyro_cdev->gyro_info));
    return 0;
}

int gyro_core_init(struct gyro_dev *dev)
{
    if (!dev)
    {
        GYRO_ERR("gyro_dev pointer is NULL");
        return -EFAULT;
    }

    if (!dev->reg_ops)
    {
        GYRO_ERR("reg ops is NULL");
        return -EFAULT;
    }

    if (!dev->transfer_dev)
    {
        GYRO_ERR("transfer_dev is NULL");
        return -EFAULT;
    }

    if (!dev->sensor_ops)
    {
        GYRO_ERR("sensor_ops is NULL");
        return -EFAULT;
    }

    gyro_cdev = dev;

    GYRO_DBG("gyro_core init");

    return 0;
}

void gyro_core_deinit(struct gyro_dev *dev)
{
    GYRO_DBG("gyro_core deinit");
}

EXPORT_SYMBOL(gyro_set_sample_rate_div);
EXPORT_SYMBOL(gyro_get_sample_rate_div);
EXPORT_SYMBOL(gyro_set_gyro_range);
EXPORT_SYMBOL(gyro_set_accel_range);
EXPORT_SYMBOL(gyro_get_gyro_range);
EXPORT_SYMBOL(gyro_get_gyro_sensitivity);
EXPORT_SYMBOL(gyro_get_accel_range);
EXPORT_SYMBOL(gyro_get_accel_sensitivity);

EXPORT_SYMBOL(gyro_read_fifodata);
EXPORT_SYMBOL(gyro_read_fifocnt);
EXPORT_SYMBOL(gyro_reset_fifo);
EXPORT_SYMBOL(gyro_read_gyro_xyz);
EXPORT_SYMBOL(gyro_read_accel_xyz);
EXPORT_SYMBOL(gyro_read_temp);
EXPORT_SYMBOL(gyro_set_dev_mode);

EXPORT_SYMBOL(gyro_enable);
EXPORT_SYMBOL(gyro_disable);

MODULE_LICENSE("GPL");
