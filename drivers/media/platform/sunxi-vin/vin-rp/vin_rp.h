/*
 * linux-4.9/drivers/media/platform/sunxi-vin/vin-rp/vin_rp.h
 *
 * Copyright (c) 2007-2021 Allwinnertech Co., Ltd.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef _VIN_RP_H_
#define _VIN_RP_H_

#include <linux/rpbuf.h>
#include <linux/rpmsg.h>

////////////////////////isp rp//////////////////////////////
/* need set the same with melis */
#define ISP_RPBUF_LOAD_NAME "isp_load_rpbuf"
#define ISP_RPBUF_SAVE_NAME "isp_save_rpbuf"
#define ISP_RPBUF_LOAD_LEN ISP_LOAD_DRAM_SIZE
#define ISP_RPBUF_SAVE_LEN (ISP_STAT_TOTAL_SIZE + ISP_SAVE_LOAD_STATISTIC_SIZE)

#define ISP_RPMSG_NAME "sunxi,isp_rpmsg"

struct isp_dev;

enum rpmsg_cmd {
	ISP_REQUEST_SENSOR_INFO = 0,
	ISP_SET_SENSOR_EXP_GAIN,
	ISP_SET_STAT_EN,
	ISP_SET_SAVE_AE,

	VIN_SET_SENSOR_INFO,
	VIN_SET_FRAME_SYNC,
	VIN_SET_IOCTL,
	VIN_SET_CLOSE_ISP,
	VIN_SET_ISP_RESET,
};
//////////////////////////end////////////////////////////

int isp_save_rpbuf_send(struct isp_dev *isp, void *buf);
int isp_rpbuf_create(struct isp_dev *isp);
int isp_rpbuf_destroy(struct isp_dev *isp);

int isp_rpmsg_send(struct rpmsg_device *rpdev, void *data, int len);
int isp_rpmsg_trysend(struct rpmsg_device *rpdev, void *data, int len);

#endif

