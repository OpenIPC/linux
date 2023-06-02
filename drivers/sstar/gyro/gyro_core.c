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
#include <linux/module.h>
#include <linux/export.h>

#include "gyro_core.h"

//#define SS_GYRO_UT

static struct gyro_dev *gyro_cdev;
static atomic_t use_count;

int gyro_set_sample_rate(struct gyro_arg_sample_rate arg)
{
    if (!gyro_cdev->sensor_ops->set_sample_rate) {
        GYRO_ERR("gyro_set_sample_rate is not support.");
        return -EFAULT;
    }
    return gyro_cdev->sensor_ops->set_sample_rate(gyro_cdev, arg);
}

int gyro_get_sample_rate(struct gyro_arg_sample_rate *arg)
{
    if (!gyro_cdev->sensor_ops->get_sample_rate) {
        GYRO_ERR("gyro_get_sample_rate is not support.");
        return -EFAULT;
    }
    if (NULL == arg) {
        return -EINVAL;
    }
    return gyro_cdev->sensor_ops->get_sample_rate(gyro_cdev, arg);
}

int gyro_set_gyro_range(struct gyro_arg_gyro_range arg)
{
    if (!gyro_cdev->sensor_ops->set_gyro_range) {
        GYRO_ERR("gyro_set_gyro_range is not support.");
        return -EFAULT;
    }
    return gyro_cdev->sensor_ops->set_gyro_range(gyro_cdev, arg);
}

int gyro_set_accel_range(struct gyro_arg_accel_range arg)
{
    if (!gyro_cdev->sensor_ops->set_accel_range) {
        GYRO_ERR("gyro_set_accel_range is not support.");
        return -EFAULT;
    }
    return gyro_cdev->sensor_ops->set_accel_range(gyro_cdev, arg);
}

int gyro_get_gyro_range(struct gyro_arg_gyro_range *arg)
{
    if (!gyro_cdev->sensor_ops->get_gyro_range) {
        GYRO_ERR("gyro_get_gyro_range is not support.");
        return -EFAULT;
    }
    if (NULL == arg) {
        return -EINVAL;
    }
    return gyro_cdev->sensor_ops->get_gyro_range(gyro_cdev, arg);
}

int gyro_get_gyro_sensitivity(struct gyro_arg_sensitivity *arg)
{
    if (!gyro_cdev->sensor_ops->get_gyro_sensitivity) {
        GYRO_ERR("gyro_get_gyro_sensitivity is not support.");
        return -EFAULT;
    }
    if (NULL == arg) {
        return -EINVAL;
    }
    return gyro_cdev->sensor_ops->get_gyro_sensitivity(gyro_cdev, arg);
}

int gyro_get_accel_range(struct gyro_arg_accel_range *arg)
{
    if (!gyro_cdev->sensor_ops->get_accel_range) {
        GYRO_ERR("gyro_get_accel_range is not support.");
        return -EFAULT;
    }
    if (NULL == arg) {
        return -EINVAL;
    }
    return gyro_cdev->sensor_ops->get_accel_range(gyro_cdev, arg);
}

int gyro_get_accel_sensitivity(struct gyro_arg_sensitivity *arg)
{
    if (!gyro_cdev->sensor_ops->get_accel_sensitivity) {
        GYRO_ERR("gyro_get_accel_sensitivity is not support.");
        return -EFAULT;
    }
    if (NULL == arg) {
        return -EINVAL;
    }
    return gyro_cdev->sensor_ops->get_accel_sensitivity(gyro_cdev, arg);
}

int gyro_read_fifodata(u8* fifo_data, u16 fifo_cnt)
{
    if (!gyro_cdev->sensor_ops->read_fifo_data) {
        GYRO_ERR("gyro_read_fifodata is not support.");
        return -EFAULT;
    }
    if (NULL == fifo_data) {
        return -EINVAL;
    }
    return gyro_cdev->sensor_ops->read_fifo_data(gyro_cdev, fifo_data, fifo_cnt);
}

int gyro_read_fifocnt(u16 *fifo_cnt) {
    if (!gyro_cdev->sensor_ops->read_fifo_cnt) {
        GYRO_ERR("gyro_read_fifocnt is not support.");
        return -EFAULT;
    }
    if (NULL == fifo_cnt) {
        return -EINVAL;
    }
    return gyro_cdev->sensor_ops->read_fifo_cnt(gyro_cdev, fifo_cnt);
}

int gyro_reset_fifo(void) {
    if (!gyro_cdev->sensor_ops->reset_fifo) {
        GYRO_ERR("gyro_reset_fifo is nut support.");
        return -EFAULT;
    }
    return gyro_cdev->sensor_ops->reset_fifo(gyro_cdev);
}

int gyro_read_gyro_xyz(struct gyro_arg_gyro_xyz *arg) {
    if (NULL == arg) {
        return -EINVAL;
    }

    return 0;
}

int gyro_read_accel_xyz(struct gyro_arg_accel_xyz *arg) {
    if (NULL == arg) {
        return -EINVAL;
    }

    return 0;
}

int gyro_read_temp(struct gyro_arg_temp *arg) {
    if (NULL == arg) {
        return -EINVAL;
    }

    return 0;
}

int gyro_set_dev_mode(struct gyro_arg_dev_mode dev_mode, struct gyro_arg_fifo_info *fifo_info) {
    if (!gyro_cdev->sensor_ops->enable_fifo) {
        GYRO_ERR("gyro_set_dev_mode is not support.");
        return -EFAULT;
    }
    if (NULL == fifo_info) {
        return -EINVAL;
    }
    memset(fifo_info, 0xff, sizeof(struct gyro_arg_fifo_info));
    fifo_info->is_big_endian = 1;
    return gyro_cdev->sensor_ops->enable_fifo(gyro_cdev, dev_mode, fifo_info);
}

int gyro_enable(void)
{
    int ret = 0;

    if (atomic_read(&use_count) > 0) {
        atomic_inc(&use_count);
        return -EBUSY;
    }

    /* Init gyro device */
    if (!gyro_cdev->sensor_ops->init) {
        GYRO_ERR("gyro_init is not support.");
        ret = -EFAULT;
        goto err_init_dev;
    }
    ret = gyro_cdev->sensor_ops->init(gyro_cdev);
    if (ret < 0) {
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
    if (atomic_read(&use_count) > 0) {
        return 0;
    }

    /* Reset */
    memset(&gyro_cdev->gyro_info, 0, sizeof(gyro_cdev->gyro_info));
    return 0;
}

#ifndef SS_GYRO_UT
static MHAL_DIS_GyroRegisterHander_t dis_register_gyro_hander = {
    .pGyroSetSampleRate       = gyro_set_sample_rate,
    .pGyroGetSampleRate       = gyro_get_sample_rate,
    .pGyroSetGyroRange        = gyro_set_gyro_range,
    .pGyroSetAccelRange       = gyro_set_accel_range,
    .pGyroGetGyroRange        = gyro_get_gyro_range,
    .pGyroGetAccelRange       = gyro_get_accel_range,
    .pGyroGetGyroSensitivity  = gyro_get_gyro_sensitivity,
    .pGyroGetAccelSensitivity = gyro_get_gyro_sensitivity,
    .pGyroReadFifodata        = gyro_read_fifodata,
    .pGyroReadFifocnt         = gyro_read_fifocnt,
    .pGyroResetFifo           = gyro_reset_fifo,
    .pGyroReadGyroXyz         = gyro_read_gyro_xyz,
    .pGyroReadAccelXyz        = gyro_read_accel_xyz,
    .pGyroReadTemp            = gyro_read_temp,
    .pGyroSetDevMode          = gyro_set_dev_mode,
    .pGyroEnable              = gyro_enable,
    .pGyroDisable             = gyro_disable,
};

__attribute__((weak)) int MHal_DIS_RegisterGyroHanderCallback(MHAL_DIS_GyroRegisterHander_t *pHander);
__attribute__((weak)) int MHal_DIS_UnRegisterGyroHanderCallback(void);
#endif

int gyro_core_init(struct gyro_dev *dev)
{
    if (!dev) {
        GYRO_ERR("gyro_dev pointer is NULL");
        return -EFAULT;
    }

    if (!dev->reg_ops) {
        GYRO_ERR("reg ops is NULL");
        return -EFAULT;
    }

    if (!dev->transfer_dev) {
        GYRO_ERR("transfer_dev is NULL");
        return -EFAULT;
    }

    if (!dev->sensor_ops) {
        GYRO_ERR("sensor_ops is NULL");
        return -EFAULT;
    }

    gyro_cdev = dev;

    GYRO_DBG("gyro_core init");

#ifndef SS_GYRO_UT
    MHal_DIS_RegisterGyroHanderCallback(&dis_register_gyro_hander);
#endif
    return 0;
}

void gyro_core_deinit(struct gyro_dev *dev)
{
#ifndef SS_GYRO_UT
    MHal_DIS_UnRegisterGyroHanderCallback();
#endif
    GYRO_DBG("gyro_core deinit");
}

MODULE_LICENSE("GPL");

