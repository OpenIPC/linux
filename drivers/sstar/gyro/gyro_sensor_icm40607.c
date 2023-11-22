/*
 * gyro_sensor_icm40607.c - Sigmastar
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

#include <linux/delay.h>
#include "gyro_core.h"
#include "gyro.h"

#define GYROSENSOR_ICM40607_INT_LEVEL_L       0x80
#define GYROSENSOR_ICM40607_INT_LEVEL_H       0x00
#define GYROSENSOR_ICM40607_INT_OPEN_DRAIN    0x40
#define GYROSENSOR_ICM40607_INT_PUSH_PULL     0x00
#define GYROSENSOR_ICM40607_LATCH_INT_EN      0x20
#define GYROSENSOR_ICM40607_INT_READ_CLEA     0x10
#define GYROSENSOR_ICM40607_FSYNC_INT_LEVEL_L 0x08
#define GYROSENSOR_ICM40607_FSYNC_INT_LEVEL_H 0x00
#define GYROSENSOR_ICM40607_FSYNC_INT_MODEN   0x04

#define GYROSENSOR_ICM40607_INT_NONE       0x00
#define GYROSENSOR_ICM40607_INT_FIFO_FULL  0x10
#define GYROSENSOR_ICM40607_INT_DATA_READY 0x01

#define GYROSENSOR_ICM40607_FIFO_RD_EN     0x40
#define GYROSENSOR_ICM40607_SPI_INTERFACEN 0x10
#define GYROSENSOR_ICM40607_RESET_FIFO     0x04

enum
{
    GSEN_ICM40607_CONFIG      = 0x11,
    GSEN_ICM40607_SPI_SPEED   = 0x13,
    GSEN_ICM40607_INT_CONFIG  = 0x14,
    GSEN_ICM40607_FIFO_CONFIG = 0x16,

    GSEN_ICM40607_TEMP_OUT_H        = 0x1D,
    GSEN_ICM40607_TEMP_OUT_L        = 0x1E,
    GSEN_ICM40607_ACCEL_XOUT_H      = 0x1F,
    GSEN_ICM40607_ACCEL_XOUT_L      = 0x20,
    GSEN_ICM40607_ACCEL_YOUT_H      = 0x21,
    GSEN_ICM40607_ACCEL_YOUT_L      = 0x22,
    GSEN_ICM40607_ACCEL_ZOUT_H      = 0x23,
    GSEN_ICM40607_ACCEL_ZOUT_L      = 0x24,
    GSEN_ICM40607_GYRO_XOUT_H       = 0x25,
    GSEN_ICM40607_GYRO_XOUT_L       = 0x26,
    GSEN_ICM40607_GYRO_YOUT_H       = 0x27,
    GSEN_ICM40607_GYRO_YOUT_L       = 0x28,
    GSEN_ICM40607_GYRO_ZOUT_H       = 0x29,
    GSEN_ICM40607_GYRO_ZOUT_L       = 0x2A,
    GSEN_ICM40607_TMST_FSYNC_H      = 0x2B,
    GSEN_ICM40607_TMST_FSYNC_L      = 0x2C,
    GSEN_ICM40607_INT_STATUS        = 0x2D,
    GSEN_ICM40607_FIFO_COUNTH       = 0x2E,
    GSEN_ICM40607_FIFO_COUNTL       = 0x2F,
    GSEN_ICM40607_FIFO_DATA         = 0x30,
    GSEN_ICM40607_SIGNAL_PATH_RESET = 0x4B,

    GSEN_ICM40607_INTF_CONFIG0       = 0x4C,
    GSEN_ICM40607_PWR_MGMT_0         = 0x4E,
    GSEN_ICM40607_GYRO_CONFIG0       = 0x4F,
    GSEN_ICM40607_ACCEL_CONFIG0      = 0x50,
    GSEN_ICM40607_GYRO_CONFIG1       = 0x51,
    GSEN_ICM40607_GYRO_ACCEL_CONFIG0 = 0x52,
    GSEN_ICM40607_ACCEL_CONFIG1      = 0x53,

    GSEN_ICM40607_TMST_CONFIG  = 0x54,
    GSEN_ICM40607_SMD_CONFIG   = 0x57,
    GSEN_ICM40607_FIFO_CONFIG1 = 0x5F,
    GSEN_ICM40607_FIFO_CONFIG2 = 0x60,
    GSEN_ICM40607_FIFO_CONFIG3 = 0x61,
    GSEN_ICM40607_FSYNC_CONFIG = 0x62,
    GSEN_ICM40607_INT_CONFIG0  = 0x63,
    GSEN_ICM40607_INT_CONFIG1  = 0x64,

    GSEN_ICM40607_INT_SOURCE0 = 0x65,
    GSEN_ICM40607_INT_SOURCE1 = 0x66,
    GSEN_ICM40607_INT_SOURCE3 = 0x68,
    GSEN_ICM40607_INT_SOURCE4 = 0x69,

    GSEN_ICM40607_FIFO_LOST_PKT0   = 0x6C,
    GSEN_ICM40607_FIFO_LOST_PKT1   = 0x6D,
    GSEN_ICM40607_SELF_TEST_CONFIG = 0x70,

    GSEN_ICM40607_WHO_AM_I     = 0x75,
    GSEN_ICM40607_REG_BANK_SEL = 0x76,
    GSEN_ICM40607_OFFSET_USER0 = 0x77,
    GSEN_ICM40607_OFFSET_USER1 = 0x78,
    GSEN_ICM40607_OFFSET_USER2 = 0x79,
    GSEN_ICM40607_OFFSET_USER3 = 0x7A,
    GSEN_ICM40607_OFFSET_USER4 = 0x7B,
    GSEN_ICM40607_OFFSET_USER5 = 0x7C,
    GSEN_ICM40607_OFFSET_USER6 = 0x7D,
    GSEN_ICM40607_OFFSET_USER7 = 0x7E,
    GSEN_ICM40607_OFFSET_USER8 = 0x7F,
};

static int icm40607_init(struct gyro_dev *dev)
{
    int ret = 0;

    /* reset */
    ret = dev->reg_ops->write_reg(dev, GSEN_ICM40607_CONFIG, 0x01);
    if (ret < 0)
    {
        return ret;
    }

    msleep(100);

    ret = dev->reg_ops->write_reg(dev, GSEN_ICM40607_PWR_MGMT_0, 0x0f);
    if (ret < 0)
    {
        return ret;
    }
    ret = dev->reg_ops->write_reg(dev, GSEN_ICM40607_FIFO_CONFIG, 0xc0);
    if (ret < 0)
    {
        return ret;
    }

    return 0;
}

static int icm40607_enable_fifo(struct gyro_dev *dev, struct gyro_arg_dev_mode mode,
                                struct gyro_arg_fifo_info *fifo_info)
{
    int ret    = 0;
    u8  val    = 0;
    u8  offset = 0;
    u8  tmp    = 0;
    int i      = 0;
    struct __icg40607_fifo_info
    {
        u8  fifo_type;
        u8 *axis_start;
        u8 *axis_end;
        u8  size;
        u8  reg_setting;
    } info[] = {
        {0xff, &tmp, &tmp, 1, 0x00},
        {GYROSENSOR_ZA_FIFO_EN | GYROSENSOR_YA_FIFO_EN | GYROSENSOR_XA_FIFO_EN, &fifo_info->ax_start,
         &fifo_info->ax_end, 2, 0x01},
        {GYROSENSOR_ZA_FIFO_EN | GYROSENSOR_YA_FIFO_EN | GYROSENSOR_XA_FIFO_EN, &fifo_info->ay_start,
         &fifo_info->ay_end, 2, 0x01},
        {GYROSENSOR_ZA_FIFO_EN | GYROSENSOR_YA_FIFO_EN | GYROSENSOR_XA_FIFO_EN, &fifo_info->az_start,
         &fifo_info->az_end, 2, 0x01},
        {GYROSENSOR_XG_FIFO_EN | GYROSENSOR_YG_FIFO_EN | GYROSENSOR_ZG_FIFO_EN, &fifo_info->gx_start,
         &fifo_info->gx_end, 2, 0x02},
        {GYROSENSOR_YG_FIFO_EN | GYROSENSOR_YG_FIFO_EN | GYROSENSOR_ZG_FIFO_EN, &fifo_info->gy_start,
         &fifo_info->gy_end, 2, 0x02},
        {GYROSENSOR_ZG_FIFO_EN | GYROSENSOR_YG_FIFO_EN | GYROSENSOR_ZG_FIFO_EN, &fifo_info->gz_start,
         &fifo_info->gz_end, 2, 0x02},
        {0xff, &fifo_info->temp_start, &fifo_info->temp_end, 1, 0x00},
    };
    if (mode.fifo_mode)
    {
        for (i = 0; i < sizeof(info) / sizeof(info[0]); ++i)
        {
            if (mode.fifo_type & (info[i].fifo_type))
            {
                *info[i].axis_start = offset;
                *info[i].axis_end   = offset + info[i].size - 1;
                val |= info[i].reg_setting;
                offset += info[i].size;
            }
        }
        fifo_info->bytes_pre_data = offset > 8 ? 16 : offset;
        fifo_info->max_fifo_cnt   = 2048;
        fifo_info->is_big_endian  = 1;
    }
    else
    {
        val = 0;
    }
    ret = dev->reg_ops->write_reg(dev, GSEN_ICM40607_FIFO_CONFIG1, val);
    return ret;
}

static int icm40607_set_sample_rate(struct gyro_dev *dev, enum gyro_sample_rate rate)
{
    u8 gyro_cfg_val  = 0;
    u8 accel_cfg_val = 0;
    dev->reg_ops->read_reg(dev, GSEN_ICM40607_GYRO_CONFIG0, &gyro_cfg_val);
    dev->reg_ops->read_reg(dev, GSEN_ICM40607_GYRO_CONFIG0, &accel_cfg_val);
    gyro_cfg_val &= 0xf0;
    accel_cfg_val &= 0xf0;
    switch (rate)
    {
        case GYROSENSOR_SAMPLE_RATE_8KHZ:
        {
            gyro_cfg_val |= 0x03;
            accel_cfg_val |= 0x03;
        }
        break;
        case GYROSENSOR_SAMPLE_RATE_4KHZ:
        {
            gyro_cfg_val |= 0x04;
            accel_cfg_val |= 0x04;
        }
        break;
        case GYROSENSOR_SAMPLE_RATE_2KHZ:
        {
            gyro_cfg_val |= 0x05;
            accel_cfg_val |= 0x05;
        }
        break;
        case GYROSENSOR_SAMPLE_RATE_1KHZ:
        {
            gyro_cfg_val |= 0x06;
            accel_cfg_val |= 0x06;
        }
        break;

        default:
        {
            GYRO_ERR("sample rate is not supported.");
            return -1;
        }
    }
    dev->reg_ops->write_reg(dev, GSEN_ICM40607_GYRO_CONFIG0, gyro_cfg_val);
    dev->reg_ops->write_reg(dev, GSEN_ICM40607_ACCEL_CONFIG0, accel_cfg_val);
    return 0;
}
static int icm40607_get_sample_rate(struct gyro_dev *dev, enum gyro_sample_rate *rate)
{
    u8 gyro_cfg_val  = 0;
    u8 accel_cfg_val = 0;
    dev->reg_ops->read_reg(dev, GSEN_ICM40607_GYRO_CONFIG0, &gyro_cfg_val);
    dev->reg_ops->read_reg(dev, GSEN_ICM40607_GYRO_CONFIG0, &accel_cfg_val);
    gyro_cfg_val &= 0x0f;
    accel_cfg_val &= 0x0f;
    if (gyro_cfg_val != accel_cfg_val)
    {
        GYRO_ERR("sample rate is different.");
        return -1;
    }
    switch (gyro_cfg_val)
    {
        case 0x03:
            *rate = GYROSENSOR_SAMPLE_RATE_8KHZ;
            break;
        case 0x04:
            *rate = GYROSENSOR_SAMPLE_RATE_4KHZ;
            break;
        case 0x05:
            *rate = GYROSENSOR_SAMPLE_RATE_2KHZ;
            break;
        case 0x06:
            *rate = GYROSENSOR_SAMPLE_RATE_1KHZ;
            break;
        default:
            GYRO_ERR("sample rate 0x%x", gyro_cfg_val);
            return -1;
    }
    return 0;
}
static int icm40607_set_gyro_range(struct gyro_dev *dev, enum gyro_gyro_range range)
{
    int ret = 0;
    u8  val;

    ret = dev->reg_ops->read_reg(dev, GSEN_ICM40607_GYRO_CONFIG0, &val);
    if (ret != 0)
    {
        return ret;
    }
    val &= ~(0xe0);

    switch (range)
    {
        case GYROSENSOR_GYRO_RANGE_250:
            val |= 0x60;
            break;
        case GYROSENSOR_GYRO_RANGE_500:
            val |= 0x40;
            break;
        case GYROSENSOR_GYRO_RANGE_1000:
            val |= 0x20;
            break;
        case GYROSENSOR_GYRO_RANGE_2000:
            val |= 0x00;
            break;

        default:
            GYRO_ERR("gyro range is not supported.");
            return -1;
    }

    return dev->reg_ops->write_reg(dev, GSEN_ICM40607_GYRO_CONFIG0, val);
}
static int icm40607_set_accel_range(struct gyro_dev *dev, enum gyro_accel_range range)
{
    int ret = 0;
    u8  val;

    ret = dev->reg_ops->read_reg(dev, GSEN_ICM40607_ACCEL_CONFIG0, &val);
    if (ret != 0)
    {
        return ret;
    }
    val &= ~(0xe0);

    switch (range)
    {
        case GYROSENSOR_ACCEL_RANGE_2G:
            val |= 0x60;
            break;
        case GYROSENSOR_ACCEL_RANGE_4G:
            val |= 0x40;
            break;
        case GYROSENSOR_ACCEL_RANGE_8G:
            val |= 0x20;
            break;
        case GYROSENSOR_ACCEL_RANGE_16G:
            val |= 0x00;
            break;

        default:
            GYRO_ERR("accel range is not supported.");
            return -1;
    }

    return dev->reg_ops->write_reg(dev, GSEN_ICM40607_ACCEL_CONFIG0, val);
}
static int icm40607_get_gyro_range(struct gyro_dev *dev, enum gyro_gyro_range *range)
{
    int ret = 0;
    u8  val;

    ret = dev->reg_ops->read_reg(dev, GSEN_ICM40607_GYRO_CONFIG0, &val);
    if (ret != 0)
    {
        return ret;
    }
    val &= 0xe0;

    switch (val)
    {
        case 0x00:
            *range = GYROSENSOR_GYRO_RANGE_2000;
            break;
        case 0x20:
            *range = GYROSENSOR_GYRO_RANGE_1000;
            break;
        case 0x40:
            *range = GYROSENSOR_GYRO_RANGE_500;
            break;
        case 0x60:
            *range = GYROSENSOR_GYRO_RANGE_250;
            break;

        default:
            GYRO_ERR("gyro range 0x%x", val);
            return -1;
    }

    return ret;
}
static int icm40607_get_gyro_sensitivity(struct gyro_dev *dev, u16 *num, u16 *den)
{
    int ret = 0;
    u8  val = 0;

    ret = dev->reg_ops->read_reg(dev, GSEN_ICM40607_GYRO_CONFIG0, &val);
    if (ret != 0)
    {
        return ret;
    }
    val &= 0xe0;

    switch (val)
    {
        case 0x00:
            *num = 164;
            *den = 10;
            break;
        case 0x20:
            *num = 328;
            *den = 10;
            break;
        case 0x40:
            *num = 655;
            *den = 10;
            break;
        case 0x60:
            *num = 131;
            *den = 1;
            break;
        default:
            GYRO_ERR("gyro range 0x%x", val);
            return -1;
    }

    return ret;
}
static int icm40607_get_accel_range(struct gyro_dev *dev, enum gyro_accel_range *range)
{
    int ret = 0;
    u8  val;

    ret = dev->reg_ops->read_reg(dev, GSEN_ICM40607_ACCEL_CONFIG0, &val);
    if (ret != 0)
    {
        return ret;
    }

    GYRO_INFO("val = %x", val);
    val &= 0xe0;

    switch (val)
    {
        case 0x00:
            *range = GYROSENSOR_ACCEL_RANGE_16G;
            break;
        case 0x20:
            *range = GYROSENSOR_ACCEL_RANGE_8G;
            break;
        case 0x40:
            *range = GYROSENSOR_ACCEL_RANGE_4G;
            break;
        case 0x60:
            *range = GYROSENSOR_ACCEL_RANGE_2G;
            break;

        default:
            GYRO_ERR("accel range 0x%x", val);
            return -1;
    }

    return ret;
}
static int icm40607_get_accel_sensitivity(struct gyro_dev *dev, u16 *num, u16 *den)
{
    int ret = 0;
    u8  val = 0;

    ret = dev->reg_ops->read_reg(dev, GSEN_ICM40607_ACCEL_CONFIG0, &val);
    if (ret != 0)
    {
        return ret;
    }
    val &= 0xe0;

    switch (val)
    {
        case 0x00:
            *num = 2048;
            *den = 1;
            break;
        case 0x20:
            *num = 4096;
            *den = 1;
            break;
        case 0x40:
            *num = 8192;
            *den = 1;
            break;
        case 0x60:
            *num = 16384;
            *den = 1;
            break;

        default:
            GYRO_ERR("accel range 0x%x", val);
            return -1;
    }

    return ret;
}
// static int icm40607_read_gyro_xyz(struct gyro_arg_gyro_xyz *arg);
//{
//
// }
// static int (*read_accel_xyz)(struct gyro_arg_accel_xyz *arg);
//{
//
// }
// static int (*read_temp)(struct gyro_arg_temp *arg);
//{
//
// }
//
static int icm40607_read_fifo_cnt(struct gyro_dev *dev, u16 *cnt)
{
    u8 val_h = 0;
    u8 val_l = 0;
    dev->reg_ops->read_reg(dev, GSEN_ICM40607_FIFO_COUNTH, &val_h);
    dev->reg_ops->read_reg(dev, GSEN_ICM40607_FIFO_COUNTL, &val_l);
    *cnt = (val_h << 8) + val_l;
    return 0;
}

static int icm40607_read_fifo_data(struct gyro_dev *dev, u8 *fifo_data, u16 fifo_cnt)
{
    return dev->reg_ops->read_regs(dev, GSEN_ICM40607_FIFO_DATA, fifo_data, fifo_cnt);
}

static int icm40607_reset_fifo(struct gyro_dev *dev)
{
    return dev->reg_ops->write_reg(dev, GSEN_ICM40607_SIGNAL_PATH_RESET, 0x02);
}

static struct gyro_sensor_ops gyro_icm40607_ops = {
    .init                = icm40607_init,
    .enable_fifo         = icm40607_enable_fifo,
    .set_sample_rate_div = icm40607_set_sample_rate,
    .get_sample_rate_div = icm40607_get_sample_rate,

    .set_gyro_range  = icm40607_set_gyro_range,
    .set_accel_range = icm40607_set_accel_range,

    .get_gyro_range       = icm40607_get_gyro_range,
    .get_gyro_sensitivity = icm40607_get_gyro_sensitivity,

    .get_accel_range       = icm40607_get_accel_range,
    .get_accel_sensitivity = icm40607_get_accel_sensitivity,

    .read_fifo_data = icm40607_read_fifo_data,
    .read_fifo_cnt  = icm40607_read_fifo_cnt,
    .reset_fifo     = icm40607_reset_fifo,
};

int gyro_sensor_init(struct gyro_dev *dev)
{
    dev->sensor_ops = &gyro_icm40607_ops;
    return 0;
}

void gyro_sensor_deinit(struct gyro_dev *dev)
{
    dev->sensor_ops = NULL;
}
