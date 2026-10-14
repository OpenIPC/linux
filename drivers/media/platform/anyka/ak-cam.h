/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Anyka AK3918EV200 V4L2 capture bridge.
 *
 * Copyright (C) 2026 Alex Zhang <alex@osqdu.org>
 */

#ifndef AK_CAM_H
#define AK_CAM_H

#include <linux/clk.h>
#include <linux/list.h>
#include <linux/mutex.h>
#include <linux/spinlock.h>
#include <linux/workqueue.h>

#include <media/v4l2-device.h>
#include <media/v4l2-ctrls.h>
#include <media/videobuf2-core.h>

#include <plat-anyka/ak_isp_drv.h>

#include "ak-cam-compat.h"

#define AK_CAM_NAME		"ak-cam"
#define AK_CAM_SENSOR_NAME	"aksensor"

/* The ISP2 video output has four address slots and starts only once all four
 * are programmed. Slot n carries buffer index n. */
#define AK_CAM_SLOTS		4

#define AK_CAM_CHAN_MAIN	0
#define AK_CAM_CHAN_SUB		1
#define AK_CAM_CHANNELS		2

#define AK_CAM_MIN_WIDTH	32
#define AK_CAM_MIN_HEIGHT	32
#define AK_CAM_DEF_WIDTH	1280
#define AK_CAM_DEF_HEIGHT	720
#define AK_CAM_SUB_DEF_WIDTH	640
#define AK_CAM_SUB_DEF_HEIGHT	360

enum ak_cam_list_state {
	LIST_ZERO = 1,
	LIST_ONE,
	LIST_TWO,
	LIST_THREE,
	LIST_FOUR,
	LIST_FIVE,
};

struct ak_cam_dev;

struct ak_cam_buffer {
	AK_CAM_VB_BASE		vb;
	struct list_head	list;
};

struct ak_cam_node {
	struct ak_cam_dev	*dev;
	struct video_device	*vdev;
	struct vb2_queue	vq;
	struct list_head	capture;
	struct ak_cam_buffer	*armed[AK_CAM_SLOTS];
	unsigned int		chan;
	unsigned int		width;
	unsigned int		height;
	unsigned int		bytesperline;
	unsigned int		sizeimage;
	unsigned int		sequence;
	unsigned int		streaming;
	unsigned int		users;
};

struct ak_cam_dev {
	struct device		*dev;
	struct v4l2_device	v4l2_dev;
	struct v4l2_ctrl_handler	controls;
	struct v4l2_subdev	*sensor;

	struct clk		*clk;
	struct pinctrl		*pinctrl;
	unsigned long		mclk;
	unsigned int		irq;
	void			*alloc_ctx;
	size_t			pool_bytes;

	spinlock_t		lock;
	struct mutex		mutex;
	struct mutex		stream_lock;

	struct ak_cam_node	node[AK_CAM_CHANNELS];
	unsigned int		users;
	struct v4l2_rect		crop;
	void			*ref_y_cpu;
	void			*ref_uv_cpu;
	dma_addr_t		ref_y_dma;
	dma_addr_t		ref_uv_dma;
	size_t			ref_y_bytes;
	size_t			ref_uv_bytes;
	unsigned int		ref_pixels;

	void			*scratch_cpu;
	dma_addr_t		scratch_dma;
	size_t			scratch_size;
	unsigned long		slot_sub;
	unsigned long		slot_armed;

	enum isp_working_mode	cur_mode;
	AK_CAM_MBUS_CODE cur_mode_class;

	enum ak_cam_list_state	list_state;

	struct delayed_work	awb_work;
	struct delayed_work	ae_work;
	struct work_struct	resume_work;

	int			stream_ctrl_off;
	int			stream_active;
	int			cur_buf_id;
};

#endif /* AK_CAM_H */
