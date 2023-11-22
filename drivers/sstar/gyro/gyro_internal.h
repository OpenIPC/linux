/*
 * gyro_internal.h - Sigmastar
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
#ifndef _GYRO_INTERNAL_H
#define _GYRO_INTERNAL_H

#include "gyro.h"

int gyro_set_sample_rate_div(struct gyro_arg_sample_rate arg);
int gyro_get_sample_rate_div(struct gyro_arg_sample_rate *arg);
int gyro_set_gyro_range(struct gyro_arg_gyro_range arg);
int gyro_set_accel_range(struct gyro_arg_accel_range arg);
int gyro_get_gyro_range(struct gyro_arg_gyro_range *arg);
int gyro_get_gyro_sensitivity(struct gyro_arg_sensitivity *arg);
int gyro_get_accel_range(struct gyro_arg_accel_range *arg);
int gyro_get_accel_sensitivity(struct gyro_arg_sensitivity *arg);

/* fifo ops */
int gyro_read_fifodata(u8 *fifo_data, u16 fifo_cnt);
int gyro_read_fifocnt(u16 *fifo_cnt);
int gyro_reset_fifo(void);

int gyro_read_gyro_xyz(struct gyro_arg_gyro_xyz *arg);
int gyro_read_accel_xyz(struct gyro_arg_accel_xyz *arg);
int gyro_read_temp(struct gyro_arg_temp *arg);
int gyro_set_dev_mode(struct gyro_arg_dev_mode dev_mode, struct gyro_arg_fifo_info *fifo_info);
int gyro_enable(void);
int gyro_disable(void);

#endif /* _GYRO_INTERNAL_H */
