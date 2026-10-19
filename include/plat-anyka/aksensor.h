/* gc0308 Camera
 *
 * Copyright (C) 2008 Renesas Solutions Corp.
 * Kuninori Morimoto <morimoto.kuninori@renesas.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * ak_sensor.c + machine pdata.
 */

#ifndef __AKSENSOR_H__
#define __AKSENSOR_H__

#include <media/soc_camera.h>
#include <plat-anyka/wrap_sensor.h>

s32 aksensor_i2c_write_byte_data(u8 daddr, u8 raddr, u8 *data, u32 size);
s32 aksensor_i2c_read_byte_data(u8 daddr, u8 raddr);

s32 aksensor_i2c_write_byte_short(u8 daddr, u16 raddr, u8 *data, u32 size);
s32 aksensor_i2c_read_byte_short(u8 daddr, u16 raddr);

s32 aksensor_i2c_write_word_data(u8 daddr, u16 raddr, u16 *data, u32 size);
s32 aksensor_i2c_read_word_data(u8 daddr, u16 raddr);

void aksensor_set_param(u32 cmd, u32 data);
u16 aksensor_get_param(u32 cmd);

const char *get_sensor_name(void);

/*
 * for Edge ctrl
 *
 * strength also control Auto or Manual Edge Control Mode
 */
struct aksensor_edge_ctrl {
	unsigned char strength;
	unsigned char threshold;
	unsigned char upper;
	unsigned char lower;
};

/*
 * aksensor camera info
 */
struct aksensor_camera_info {
	unsigned long		   buswidth;
	unsigned long		   flags;
	unsigned long		   pin_avdd;
	unsigned long		   pin_power;
	/* ISP-era ak_sensor.c passes pin_pwdn into sensor_set_power_*_func.
	 * Legacy code used pin_power only; keep both. */
	unsigned long		   pin_pwdn;
	unsigned long		   pin_reset;
	struct soc_camera_link link;
	struct aksensor_edge_ctrl edgectrl;
};

struct aksensor_color_format {
	u32 code;
	enum v4l2_colorspace colorspace;
};

/* Crop path (ak_sensor.c) uses left/top as well as width/height. */
struct aksensor_win_size {
	char                     *name;
	__u32                     left;
	__u32                     top;
	__u32                     width;
	__u32                     height;
};

struct sensor_info
{
    const char *sensor_name;
    int sensor_id;
    const struct v4l2_ctrl_config *ctrls;
    int nr_ctrls;
    const struct aksensor_color_format *formats;
    int num_formats;
	const struct aksensor_win_size *resolution;
	int num_resolution;
    T_CAMERA_FUNCTION_HANDLER *handler;
};

#define SENSOR_MAX_SUPPORT        20

/*
 * The SDK's private controls, at the numbers its include/linux/videodev2.h
 * gave them, which its userspace was built against. Mainline never had them.
 */
#ifndef V4L2_CID_PICTURE
#define V4L2_CID_PICTURE	(V4L2_CID_USER_BASE + 4)
#endif
#ifndef V4L2_CID_NIGHTMODE
#define V4L2_CID_NIGHTMODE	(V4L2_CID_USER_BASE + 6)
#endif

/**
 * @brief register a camera device, which will be probed at camera open
 * @author dengzhou/wudaochao
 * @date 2012-11-06
 * @param[in] struct sensor_info
 * @param[in] handler camera interface
 * @retval 0 register successfully
 * @retval -1 register unsuccessfully
 */
int register_sensor(struct sensor_info *si);
#endif /* END __AKSENSOR_H__ */
