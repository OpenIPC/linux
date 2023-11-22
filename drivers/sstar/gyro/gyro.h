/*
 * gyro.h - Sigmastar
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
#ifndef GYRO_H
#define GYRO_H

#include <asm-generic/ioctl.h>

enum gyro_fifo_type
{
    GYROSENSOR_ZA_FIFO_EN   = 0x01,
    GYROSENSOR_YA_FIFO_EN   = 0x02,
    GYROSENSOR_XA_FIFO_EN   = 0x04,
    GYROSENSOR_ZG_FIFO_EN   = 0x10,
    GYROSENSOR_YG_FIFO_EN   = 0x20,
    GYROSENSOR_XG_FIFO_EN   = 0x40,
    GYROSENSOR_TEMP_FIFO_EN = 0x80,
    GYROSENSOR_FIFO_MAX_EN  = 0xFF,
};

enum gyro_gyro_range
{
    GYROSENSOR_GYRO_RANGE_125 = 0,
    GYROSENSOR_GYRO_RANGE_250,
    GYROSENSOR_GYRO_RANGE_500,
    GYROSENSOR_GYRO_RANGE_1000,
    GYROSENSOR_GYRO_RANGE_2000,
    GYROSENSOR_GYRO_RANGE_INVALID,
};

enum gyro_accel_range
{
    GYROSENSOR_ACCEL_RANGE_2G = 0,
    GYROSENSOR_ACCEL_RANGE_4G,
    GYROSENSOR_ACCEL_RANGE_8G,
    GYROSENSOR_ACCEL_RANGE_16G,
    GYROSENSOR_ACCEL_RANGE_INVALID,
};

enum gyro_sample_rate
{
    GYROSENSOR_SAMPLE_RATE_100HZ = 0,
    GYROSENSOR_SAMPLE_RATE_200HZ,
    GYROSENSOR_SAMPLE_RATE_500HZ,
    GYROSENSOR_SAMPLE_RATE_1KHZ,
    GYROSENSOR_SAMPLE_RATE_2KHZ,
    GYROSENSOR_SAMPLE_RATE_4KHZ,
    GYROSENSOR_SAMPLE_RATE_8KHZ,
    GYROSENSOR_SAMPLE_RATE_INVALID,
};

struct gyro_arg_sample_rate
{
    enum gyro_sample_rate rate;
};

struct gyro_arg_gyro_range
{
    enum gyro_gyro_range range;
};

struct gyro_arg_accel_range
{
    enum gyro_accel_range range;
};

struct gyro_arg_sensitivity
{
    unsigned short num;
    unsigned short den;
};

struct gyro_arg_gyro_xyz
{
    short x;
    short y;
    short z;
};

struct gyro_arg_accel_xyz
{
    short x;
    short y;
    short z;
};

struct gyro_arg_temp
{
    short temp;
};

struct gyro_arg_dev_mode
{
    char          fifo_mode; /* 1 or 0 */
    unsigned char fifo_type;
};

struct gyro_arg_fifo_info
{
    unsigned char  gx_start, gx_end;
    unsigned char  gy_start, gy_end;
    unsigned char  gz_start, gz_end;
    unsigned char  ax_start, ax_end;
    unsigned char  ay_start, ay_end;
    unsigned char  az_start, az_end;
    unsigned char  temp_start, temp_end;
    unsigned char  bytes_pre_data;
    unsigned char  is_big_endian;
    unsigned short max_fifo_cnt;
};

#define GYRO_MAGIC 'i'

#define GYRO_CMD_SET_SAMPLERATE_DIV _IOW(GYRO_MAGIC, 0x00, sizeof(struct gyro_arg_sample_rate))

#define GYRO_CMD_SET_GYRO_RANGE  _IOW(GYRO_MAGIC, 0x01, sizeof(struct gyro_arg_gyro_range))
#define GYRO_CMD_SET_ACCEL_RANGE _IOW(GYRO_MAGIC, 0x02, sizeof(struct gyro_arg_accel_range))

#define GYRO_CMD_GET_GYRO_RANGE        _IOR(GYRO_MAGIC, 0x03, sizeof(struct gyro_arg_gyro_range))
#define GYRO_CMD_GET_GYRO_SENSITIVITY  _IOR(GYRO_MAGIC, 0x04, sizeof(struct gyro_arg_sensitivity))
#define GYRO_CMD_GET_ACCEL_RANGE       _IOR(GYRO_MAGIC, 0x05, sizeof(struct gyro_arg_accel_range))
#define GYRO_CMD_GET_ACCEL_SENSITIVITY _IOR(GYRO_MAGIC, 0x06, sizeof(struct gyro_arg_sensitivity))

#define GYRO_CMD_READ_FIFODATA _IOR(GYRO_MAGIC, 0x07, sizeof(struct gyro_arg_frame_data))

#define GYRO_CMD_READ_GYRO_XYZ  _IOR(GYRO_MAGIC, 0x08, sizeof(struct gyro_arg_gyro_xyz))
#define GYRO_CMD_READ_ACCEL_XYZ _IOR(GYRO_MAGIC, 0x09, sizeof(struct gyro_arg_accel_xyz))
#define GYRO_CMD_READ_TEMP      _IOR(GYRO_MAGIC, 0x0a, sizeof(struct gyro_arg_temp))

#define GYRO_CMD_SET_DEV_MODE _IOW(GYRO_MAGIC, 0x0b, sizeof(struct gyro_arg_dev_mode))

#define GYRO_CMD_COUNT 0x0c

#endif /* ifndef GYRO_H */
