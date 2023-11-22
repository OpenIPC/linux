/*
 * gyro_core.h - Sigmastar
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
#ifndef _GYRO_CORE_H
#define _GYRO_CORE_H

#include <linux/cdev.h>
#include <linux/kfifo.h>
#include "gyro.h"

#define GYRO_DEVICNAME       "gyro"
#define GYRO_DEV_COUNT       1
#define GYRO_GPIO_INT_NAME   "gyro_int"
#define GYRO_GPIO_FSYNC_NAME "gyro_fsync"

#define GYRO_INFO(fmt, args...)                                           \
    do                                                                    \
    {                                                                     \
        printk(KERN_INFO "[%s]-info: " fmt "\n", GYRO_DEVICNAME, ##args); \
    } while (0)

#define GYRO_ERR(fmt, args...)                                                                           \
    do                                                                                                   \
    {                                                                                                    \
        printk(KERN_ERR "[%s]-error: " fmt " --> %s(%d)\n", GYRO_DEVICNAME, ##args, __FILE__, __LINE__); \
    } while (0)

#ifdef SS_GYRO_DEBUG_ON
#define GYRO_DBG(fmt, args...)                                                                            \
    do                                                                                                    \
    {                                                                                                     \
        printk(KERN_INFO "[%s]-debug: " fmt " --> %s(%d)\n", GYRO_DEVICNAME, ##args, __FILE__, __LINE__); \
    } while (0)
#endif
#ifndef GYRO_DBG
#define GYRO_DBG(fmt, args...) \
    {                          \
    }
#endif

#define MAX_FRAME_CNT (8)

struct gyro_info
{
    struct gyro_arg_gyro_xyz  gyro_xyz;
    struct gyro_arg_accel_xyz accel_xyz;
    struct gyro_arg_temp      temp;
    struct kfifo              frame_fifo; /* cache 5 frame_data */
    u8                        bytes_pre_data;
    bool                      en_fifo;
};

struct gyro_dev;

struct gyro_reg_ops
{
    int (*read_regs)(struct gyro_dev *, u8, void *, int);
    int (*read_reg)(struct gyro_dev *, u8, u8 *);
    int (*write_reg)(struct gyro_dev *, u8, u8);
};

struct gyro_sensor_ops
{
    /* init - Init gyro sensor
     *
     * Return 0 if success.
     */
    int (*init)(struct gyro_dev *dev);

    /* enable_fifo - enable gyro sensor fifo function
     *
     * @mode: device mode
     * @fifo_info: fifo info
     */
    int (*enable_fifo)(struct gyro_dev *dev, struct gyro_arg_dev_mode mode, struct gyro_arg_fifo_info *fifo_info);

    /* set_sample_rate_div - Set sample rate of gyro
     * get_sample_rate_div - Get sample rate of gyro
     *
     * @rate : rate
     */
    int (*set_sample_rate_div)(struct gyro_dev *dev, enum gyro_sample_rate rate);
    int (*get_sample_rate_div)(struct gyro_dev *dev, enum gyro_sample_rate *rate);

    /* set_gyro_range - Set the max range of gyro sensor
     * get_gyro_range - Get the max range of gyro sensor
     *
     * @range : enum of range
     *
     * get_gyro_sensitivity - Get sensitivity of gyro sensor that decided by gyro range
     *
     * @num :
     * @den :
     */
    int (*set_gyro_range)(struct gyro_dev *dev, enum gyro_gyro_range range);
    int (*get_gyro_range)(struct gyro_dev *dev, enum gyro_gyro_range *range);
    int (*get_gyro_sensitivity)(struct gyro_dev *dev, u16 *num, u16 *den);

    /* set_accel_range - Set the max range of accel sensor
     * get_accel_range - Get the max range of accel sensor
     *
     * @range : enum of range
     *
     * get_gyro_sensitivity - Get sensitivity of accel sensor that decided by accel range
     *
     * @num :
     * @den :
     */
    int (*set_accel_range)(struct gyro_dev *dev, enum gyro_accel_range range);
    int (*get_accel_range)(struct gyro_dev *dev, enum gyro_accel_range *range);
    int (*get_accel_sensitivity)(struct gyro_dev *dev, u16 *num, u16 *den);

    int (*read_fifo_cnt)(struct gyro_dev *dev, u16 *cnt);
    int (*reset_fifo)(struct gyro_dev *dev);
    /* read_fifodata - Read fifo data
     *
     * @data : the pointer of data array
     * @cnt  : fifo count
     */
    int (*read_fifo_data)(struct gyro_dev *dev, u8 *data, u16 cnt);
    // int (*read_gyro_xyz)(struct gyro_arg_gyro_xyz *arg);
    // int (*read_accel_xyz)(struct gyro_arg_accel_xyz *arg);
    // int (*read_temp)(struct gyro_arg_temp *arg);
};

struct gyro_dev
{
    struct gyro_info        gyro_info;
    struct device *         transfer_dev;
    struct gyro_reg_ops *   reg_ops;
    struct gyro_sensor_ops *sensor_ops;
};

int  gyro_transfer_init(struct gyro_dev *dev);
void gyro_transfer_deinit(struct gyro_dev *dev);

int  gyro_cdev_init(struct gyro_dev *dev);
void gyro_cdev_deinit(struct gyro_dev *dev);

int  gyro_sysfs_init(struct gyro_dev *dev);
void gyro_sysfs_deinit(struct gyro_dev *dev);

int  gyro_core_init(struct gyro_dev *dev);
void gyro_core_deinit(struct gyro_dev *dev);

int  gyro_sensor_init(struct gyro_dev *dev);
void gyro_sensor_deinit(struct gyro_dev *dev);

#endif
