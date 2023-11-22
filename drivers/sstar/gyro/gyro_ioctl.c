/*
 * gyro_ioctl.c - Sigmastar
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
#include <linux/uaccess.h>

#include <asm/string.h>
#include "gyro.h"
#include "gyro_core.h"
#include "gyro_internal.h"

static dev_t       devid;
static struct cdev cdev;
static struct class *class;
static struct device *device;

static int gyro_ioctl_cmd_set_sample_rate_div(struct gyro_dev *dev, void *uarg)
{
    struct gyro_arg_sample_rate arg;
    if (copy_from_user(&arg, uarg, sizeof(arg)))
    {
        return -EFAULT;
    }
    return gyro_set_sample_rate_div(arg);
}

static int gyro_ioctl_cmd_set_gyro_range(struct gyro_dev *dev, void *uarg)
{
    struct gyro_arg_gyro_range arg;
    if (copy_from_user(&arg, uarg, sizeof(arg)))
    {
        return -EFAULT;
    }
    return gyro_set_gyro_range(arg);
}

static int gyro_ioctl_cmd_set_accel_range(struct gyro_dev *dev, void *uarg)
{
    struct gyro_arg_accel_range arg;
    if (copy_from_user(&arg, uarg, sizeof(arg)))
    {
        return -EFAULT;
    }
    return gyro_set_accel_range(arg);
}

static int gyro_ioctl_cmd_get_gyro_range(struct gyro_dev *dev, void *uarg)
{
    int                        ret = 0;
    struct gyro_arg_gyro_range arg;
    ret = gyro_get_gyro_range(&arg);
    if (ret != 0)
    {
        return ret;
    }
    ret = copy_to_user(uarg, &arg, sizeof(arg));
    return ret ? -EFAULT : 0;
}

static int gyro_ioctl_cmd_get_gyro_sensitivity(struct gyro_dev *dev, void *uarg)
{
    int                         ret = 0;
    struct gyro_arg_sensitivity arg;
    ret = gyro_get_gyro_sensitivity(&arg);
    if (ret != 0)
    {
        return ret;
    }
    ret = copy_to_user(uarg, &arg, sizeof(arg));
    return ret ? -EFAULT : 0;
}

static int gyro_ioctl_cmd_get_accel_range(struct gyro_dev *dev, void *uarg)
{
    int                         ret = 0;
    struct gyro_arg_accel_range arg;
    ret = gyro_get_accel_range(&arg);
    if (ret != 0)
    {
        return ret;
    }
    ret = copy_to_user(uarg, &arg, sizeof(arg));
    return ret ? -EFAULT : 0;
}

static int gyro_ioctl_cmd_get_accel_sensitivity(struct gyro_dev *dev, void *uarg)
{
    int                         ret = 0;
    struct gyro_arg_sensitivity arg;
    ret = gyro_get_gyro_sensitivity(&arg);
    if (ret != 0)
    {
        return ret;
    }
    ret = copy_to_user(uarg, &arg, sizeof(arg));
    return ret ? -EFAULT : 0;
}

static int gyro_ioctl_cmd_read_fifodata(struct gyro_dev *dev, void *uarg)
{
    int                        ret = 0;
    struct gyro_arg_frame_data arg;
    ret = gyro_read_fifodata(&arg, 1);
    if (ret < 1)
    {
        return ret == 1 ? -EFAULT : 0;
    }
    ret = copy_to_user(uarg, &arg, sizeof(arg));
    return ret ? -EFAULT : 0;
}

static int gyro_ioctl_cmd_read_gyro_xyz(struct gyro_dev *dev, void *uarg)
{
    int                      ret = 0;
    struct gyro_arg_gyro_xyz arg;
    ret = gyro_read_gyro_xyz(&arg);
    if (ret != 0)
    {
        return ret;
    }
    ret = copy_to_user(uarg, &arg, sizeof(arg));
    return ret ? -EFAULT : 0;
}

static int gyro_ioctl_cmd_read_accel_xyz(struct gyro_dev *dev, void *uarg)
{
    int                       ret = 0;
    struct gyro_arg_accel_xyz arg;
    ret = gyro_read_accel_xyz(&arg);
    if (ret != 0)
    {
        return ret;
    }
    ret = copy_to_user(uarg, &arg, sizeof(arg));
    return ret ? -EFAULT : 0;
}

static int gyro_ioctl_cmd_read_temp(struct gyro_dev *dev, void *uarg)
{
    int                  ret = 0;
    struct gyro_arg_temp arg;
    ret = gyro_read_temp(&arg);
    if (ret != 0)
    {
        return ret;
    }
    ret = copy_to_user(uarg, &arg, sizeof(arg));
    return ret ? -EFAULT : 0;
}

static int gyro_ioctl_cmd_set_dev_mode(struct gyro_dev *dev, void *uarg)
{
    struct gyro_arg_dev_mode arg;
    if (copy_from_user(&arg, uarg, sizeof(arg)))
    {
        return -EFAULT;
    }
    return gyro_set_dev_mode(arg);
}

static int gyro_file_open(struct inode *pinode, struct file *pfile)
{
    return gyro_enable();
}

static int gyro_file_release(struct inode *pinode, struct file *pfile)
{
    return gyro_disable();
}

static long gyro_file_ioctl(struct file *pfile, unsigned int cmd, unsigned long arg)
{
    struct gyro_dev *dev  = (struct gyro_dev *)pfile->private_data;
    void *           uarg = (void *)arg;
    u8               nr   = 0;

    /* !!! Warnning, Don't change the order of the array @cmd_ops members!! */
    static int (*cmd_ops[GYRO_CMD_COUNT])(struct gyro_dev *, void *) = {
        gyro_ioctl_cmd_set_sample_rate_div,
        gyro_ioctl_cmd_set_gyro_range,
        gyro_ioctl_cmd_set_accel_range,
        gyro_ioctl_cmd_get_gyro_range,
        gyro_ioctl_cmd_get_gyro_sensitivity,
        gyro_ioctl_cmd_get_accel_range,
        gyro_ioctl_cmd_get_accel_sensitivity,
        gyro_ioctl_cmd_read_fifodata,
        gyro_ioctl_cmd_read_gyro_xyz,
        gyro_ioctl_cmd_read_accel_xyz,
        gyro_ioctl_cmd_read_temp,
        gyro_ioctl_cmd_set_dev_mode,
    };

    if (_IOC_TYPE(cmd) != GYRO_MAGIC)
    {
        GYRO_ERR("command type %x, need %x", _IOC_TYPE(cmd), GYRO_MAGIC);
        return -EINVAL;
    }

    nr = _IOC_NR(cmd);
    if (nr >= GYRO_CMD_COUNT)
    {
        GYRO_ERR("command nr %d, need %d ~ %d", nr, 0, GYRO_CMD_COUNT - 1);
        return -EINVAL;
    }

    return cmd_ops[nr](dev, uarg);
}

static struct file_operations gyro_ops = {
    .owner          = THIS_MODULE,
    .open           = gyro_file_open,
    .release        = gyro_file_release,
    .unlocked_ioctl = gyro_file_ioctl,
};

int gyro_cdev_init(struct gyro_dev *dev)
{
    int ret = 0;
    /* alloc char device numbers */
    ret = alloc_chrdev_region(&devid, 0, GYRO_DEV_COUNT, GYRO_DEVICNAME);
    if (ret < 0)
    {
        GYRO_ERR("alloc devid failed");
        goto err_alloc_chrdev;
    }

    /* init cdev */
    cdev_init(&cdev, &gyro_ops);
    cdev.owner = THIS_MODULE;
    ret        = cdev_add(&cdev, devid, GYRO_DEV_COUNT);
    if (ret < 0)
    {
        GYRO_ERR("cdev_add failed");
        goto err_cdev_add;
    }

    /* create device node */
    class = class_create(THIS_MODULE, GYRO_DEVICNAME);
    if (IS_ERR(class))
    {
        GYRO_ERR("class create failed");
        goto err_class_create;
    }
    device = device_create(class, NULL, devid, NULL, GYRO_DEVICNAME);
    if (IS_ERR(device))
    {
        GYRO_ERR("device create failed");
        goto err_device_create;
    }

    GYRO_DBG("gyro_cdev init");
    return 0;

err_device_create:
    class_destroy(class);
err_class_create:
    cdev_del(&cdev);
err_cdev_add:
    unregister_chrdev_region(devid, 1);
err_alloc_chrdev:
    return ret;
}

void gyro_cdev_deinit(struct gyro_dev *dev)
{
    device_destroy(class, devid);
    class_destroy(class);
    cdev_del(&cdev);
    unregister_chrdev_region(devid, GYRO_DEV_COUNT);
    GYRO_DBG("gyro_cdev deinit");
}

MODULE_LICENSE("GPL");
