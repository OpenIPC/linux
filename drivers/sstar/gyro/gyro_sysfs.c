/*
 * gyro_sysfs.c - Sigmastar
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

#include <linux/stat.h>
#include <linux/device.h>
#include <linux/string.h>
#include <linux/kthread.h>
#include <linux/delay.h>
#include "ms_msys.h"
#include "gyro_core.h"

#define CHECK_RESULT(result, expectation, function, fmt, ...)         \
    do {                                                              \
        if ((expectation) == (function)) {                            \
            printk("[ Success ] " #fmt "\n", ##__VA_ARGS__);          \
        } else {                                                      \
            printk("[ Failed  ] " #fmt "\n", ##__VA_ARGS__);          \
            result = !result;                                         \
        }                                                             \
    } while(0)

static dev_t devid;
static struct device *gyro_sysfs_dev;

static void show_one_frame_fifo_data(u8 *data, struct gyro_arg_fifo_info fifo_info)
{
    struct fifo_parse {
        char *name;
        u8 start;
        u8 end;
    } fp[] = {
        { "gx", fifo_info.gx_start, fifo_info.gx_end },
        { "gy", fifo_info.gy_start, fifo_info.gy_end },
        { "gz", fifo_info.gz_start, fifo_info.gz_end },
        { "ax", fifo_info.ax_start, fifo_info.ax_end },
        { "ay", fifo_info.ay_start, fifo_info.ay_end },
        { "az", fifo_info.az_start, fifo_info.az_end },
        { "te", fifo_info.gx_start, fifo_info.gx_end },
    };
    unsigned int i = 0;
    unsigned char j = 0;
    int num = 0;
    int shift = 0;

    printk(KERN_DEBUG KERN_CONT "| ");
    for (i = 0; i < sizeof(fp)/sizeof(fp[0]); ++i) {
        if (fp[i].start > fifo_info.bytes_pre_data || fp[i].end > fifo_info.bytes_pre_data) {
            continue;
        }
        num = 0;
        shift = fifo_info.is_big_endian ? ( fp[i].end - fp[i].start ) * 8 : 0;
        for (j = fp[i].start; j <= fp[i].end; ++j) {
            num |= ( (signed char)data[j] << shift );
            shift += fifo_info.is_big_endian ? -8 : 8;
        }
        printk(KERN_DEBUG KERN_CONT "%4s: %6d | ", fp[i].name, num);
    }
    printk(KERN_DEBUG KERN_CONT "\n");
}

static void gyro_self_check(void)
{
    struct gyro_arg_dev_mode dev_mode;
    struct gyro_arg_gyro_range gyro_range;
    struct gyro_arg_accel_range accel_range;
    struct gyro_arg_sample_rate sample_rate;
    struct gyro_arg_fifo_info fifo_info;

	bool result = true;
    int i;
    unsigned int test_sample_rate[] = {
        100,
        200,
        500,
        1000,
        2000,
        4000,
        8000,
        16000,
        200,
    };
    unsigned int test_gyro_range[] = {
        125,
        250,
        500,
        1000,
        2000,
    };
    unsigned int test_accel_range[] = {
        2,
        4,
        8,
        16
    };
    struct gyro_arg_dev_mode test_dev_mode[] = {
        { 0, 0 },
        { 1, GYROSENSOR_ZA_FIFO_EN | GYROSENSOR_YA_FIFO_EN | GYROSENSOR_XA_FIFO_EN |
             GYROSENSOR_ZG_FIFO_EN | GYROSENSOR_YG_FIFO_EN | GYROSENSOR_XG_FIFO_EN | GYROSENSOR_TEMP_FIFO_EN},
        { 1, GYROSENSOR_ZA_FIFO_EN | GYROSENSOR_YA_FIFO_EN | GYROSENSOR_XA_FIFO_EN },
        { 1, GYROSENSOR_ZG_FIFO_EN | GYROSENSOR_YG_FIFO_EN | GYROSENSOR_XG_FIFO_EN },
    };

    /* enable gyro */
    CHECK_RESULT(result, 0, gyro_enable(), "gyro_enable");
    if (!result) {
        return;
    }

    printk("----gyro range test --------------------------------------\n");
    for (i = 0; i < sizeof(test_gyro_range)/sizeof(test_gyro_range[0]); ++i) {
        gyro_range.range = test_gyro_range[i];
        CHECK_RESULT(result , 0      , gyro_set_gyro_range(gyro_range)        , "gyro_set_gyro_range (%u)"    , test_gyro_range[i] );
        CHECK_RESULT(result , 0      , gyro_get_gyro_range(&gyro_range)       , "gyro_get_gyro_range"                              );
        CHECK_RESULT(result , true   , gyro_range.range == test_gyro_range[i] , "read back check."                                 );
        CHECK_RESULT(result , true   , result                                 , "result check gyro_range [%u]", test_gyro_range[i] );
    }

    printk("----accel range test --------------------------------------\n");
    for (i = 0; i < sizeof(test_accel_range)/sizeof(test_accel_range[0]); ++i) {
        accel_range.range = test_accel_range[i];
        CHECK_RESULT(result , 0      , gyro_set_accel_range(accel_range)        , "gyro_set_accel_range (%u)"     , test_accel_range[i] );
        CHECK_RESULT(result , 0      , gyro_get_accel_range(&accel_range)       , "gyro_get_accel_range"                                );
        CHECK_RESULT(result , true   , accel_range.range == test_accel_range[i] , "read back check."                                    );
        CHECK_RESULT(result , true   , result                                   , "result check accel_range [%u]" , test_accel_range[i] );
    }

    printk("----sample rate test --------------------------------------\n");
    for (i = 0; i < sizeof(test_sample_rate)/sizeof(test_sample_rate[0]); ++i) {
        sample_rate.rate = test_sample_rate[i];
        CHECK_RESULT(result , 0    , gyro_set_sample_rate(sample_rate)       , "gyro_set_sample_rate (%u)"     , test_sample_rate[i] );
        CHECK_RESULT(result , 0    , gyro_get_sample_rate(&sample_rate)      , "gyro_get_sample_rate"                                );
        CHECK_RESULT(result , true , sample_rate.rate == test_sample_rate[i] , "read back check."                                    );
        CHECK_RESULT(result , true , result                                  , "result check sample_rate [%u]" , test_sample_rate[i] );
    }

    printk("----gyro_set_dev_mode -------------------------------------\n");
    for (i = 0; i < sizeof(test_dev_mode)/sizeof(test_dev_mode[0]); ++i) {
        dev_mode = test_dev_mode[i];
        CHECK_RESULT(result, 0, gyro_set_dev_mode(dev_mode, &fifo_info), "gyro_set_dev_mode (%d, 0x%x)", dev_mode.fifo_mode, dev_mode.fifo_type);
        printk("\tgx_start, gx_end      %d, %d\n", fifo_info.gx_start, fifo_info.gx_end);
        printk("\tgy_start, gy_end      %d, %d\n", fifo_info.gy_start, fifo_info.gy_end);
        printk("\tgz_start, gz_end      %d, %d\n", fifo_info.gz_start, fifo_info.gz_end);
        printk("\tax_start, ax_end      %d, %d\n", fifo_info.ax_start, fifo_info.ax_end);
        printk("\tay_start, ay_end      %d, %d\n", fifo_info.ay_start, fifo_info.ay_end);
        printk("\taz_start, az_end      %d, %d\n", fifo_info.az_start, fifo_info.az_end);
        printk("\ttemp_start, temp_end  %d, %d\n", fifo_info.temp_start, fifo_info.temp_end);
        printk("\tbytes_pre_data        %d\n", fifo_info.bytes_pre_data);
        printk("\tis_big_endian         %d\n", fifo_info.is_big_endian);
        printk("\tmax_fifo_cnt          %d\n", fifo_info.max_fifo_cnt);
    }

    printk("----gyro fifo test ---------------------------------------\n");
    for (i = 0; i < 10; ++i) {
        u8 fifo_data[2048];
        u16 cnt = 0;
        u16 j = 0;

        msleep(30);
        gyro_read_fifocnt(&cnt);
        if (cnt > 2048) {
            cnt = 2048 / fifo_info.bytes_pre_data * fifo_info.bytes_pre_data;
        }
        gyro_read_fifodata(fifo_data, cnt);
        printk(KERN_DEBUG "-------------------------------------------------------------------------\n");
        for (j = 0; j <= cnt - fifo_info.bytes_pre_data; j += fifo_info.bytes_pre_data) {
            show_one_frame_fifo_data(&fifo_data[j], fifo_info);
        }
    }

    gyro_disable();
}

static ssize_t gyro_sysfs_self_check_show(struct device *dev, struct device_attribute *attr,
        char *buf)
{
    gyro_self_check();
    return scnprintf(buf, PAGE_SIZE, "Show whole log in kmsg\n");
}

static DEVICE_ATTR(self_check, S_IRUGO, gyro_sysfs_self_check_show, NULL);

int gyro_sysfs_init(struct gyro_dev *dev)
{
    int ret = 0;
    ret = alloc_chrdev_region(&devid, 0, GYRO_DEV_COUNT, GYRO_DEVICNAME);
    if (ret < 0) {
        GYRO_ERR("alloc_chrdev_region for sysfs failed.");
        goto err_alloc_chrdev_region;
    }
    gyro_sysfs_dev = device_create(msys_get_sysfs_class(), NULL, devid, NULL, "gyro");
    if (IS_ERR(gyro_sysfs_dev)) {
        GYRO_ERR("device_create for sysfs failed.");
        goto err_device_create;
    }

    device_create_file(gyro_sysfs_dev, &dev_attr_self_check);
    return 0;

err_device_create:
    unregister_chrdev_region(devid, GYRO_DEV_COUNT);
err_alloc_chrdev_region:
    return ret;
}

void gyro_sysfs_deinit(struct gyro_dev *dev)
{
    device_destroy(msys_get_sysfs_class(), devid);
    unregister_chrdev_region(devid, GYRO_DEV_COUNT);
}

