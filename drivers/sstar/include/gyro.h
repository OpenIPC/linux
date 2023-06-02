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

enum gyro_fifo_type {
    GYROSENSOR_ZA_FIFO_EN        = 0x01,
    GYROSENSOR_YA_FIFO_EN        = 0x02,
    GYROSENSOR_XA_FIFO_EN        = 0x04,
    GYROSENSOR_ZG_FIFO_EN        = 0x10,
    GYROSENSOR_YG_FIFO_EN        = 0x20,
    GYROSENSOR_XG_FIFO_EN        = 0x40,
    GYROSENSOR_TEMP_FIFO_EN      = 0x80,
    GYROSENSOR_FIFO_MAX_EN       = 0xFF,
};

struct gyro_arg_sample_rate {
    unsigned int rate;
};

struct gyro_arg_gyro_range {
    unsigned int range;
};

struct gyro_arg_accel_range {
    unsigned int range;
};

struct gyro_arg_sensitivity {
    unsigned short num;
    unsigned short den;
};

struct gyro_arg_gyro_xyz {
    short x;
    short y;
    short z;
};

struct gyro_arg_accel_xyz {
    short x;
    short y;
    short z;
};

struct gyro_arg_temp {
    short temp;
};

struct gyro_arg_dev_mode {
    char fifo_mode; /* 1 or 0 */
    unsigned char fifo_type;
};

struct gyro_arg_fifo_info {
    unsigned char gx_start, gx_end;
    unsigned char gy_start, gy_end;
    unsigned char gz_start, gz_end;
    unsigned char ax_start, ax_end;
    unsigned char ay_start, ay_end;
    unsigned char az_start, az_end;
    unsigned char temp_start, temp_end;
    unsigned char bytes_pre_data;
    unsigned char is_big_endian;
    unsigned short max_fifo_cnt;
};

typedef struct MHAL_DIS_GyroRegisterHander_s
{
    int (* pGyroSetSampleRate)(struct gyro_arg_sample_rate arg);
    int (* pGyroGetSampleRate)(struct gyro_arg_sample_rate *arg);
    int (* pGyroSetGyroRange)(struct gyro_arg_gyro_range arg);
    int (* pGyroSetAccelRange)(struct gyro_arg_accel_range arg);
    int (* pGyroGetGyroRange)(struct gyro_arg_gyro_range *arg);
    int (* pGyroGetAccelRange)(struct gyro_arg_accel_range *arg);
    int (* pGyroGetAccelSensitivity)(struct gyro_arg_sensitivity *arg);
    int (* pGyroGetGyroSensitivity)(struct gyro_arg_sensitivity *arg);
    /* fifo ops */
    int (* pGyroReadFifodata)(u8* fifo_data, u16 fifo_cnt);
    int (* pGyroReadFifocnt)(u16 *fifo_cnt);
    int (* pGyroResetFifo)(void);

    int (* pGyroReadGyroXyz)(struct gyro_arg_gyro_xyz *arg);
    int (* pGyroReadAccelXyz)(struct gyro_arg_accel_xyz *arg);
    int (* pGyroReadTemp)(struct gyro_arg_temp *arg);
    int (* pGyroSetDevMode)(struct gyro_arg_dev_mode dev_mode, struct gyro_arg_fifo_info *fifo_info);
    int (* pGyroEnable)(void);
    int (* pGyroDisable)(void);
}MHAL_DIS_GyroRegisterHander_t;

#endif /* ifndef GYRO_H */
