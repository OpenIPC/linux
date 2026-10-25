/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Fullhan FH8626V100 media pipeline: subdevice links, stream state and
 * the interfaces shared by the CSI, ISP, VPU and capture nodes.
 *
 * Copyright (C) 2026 Difan Zhang
 */

#ifndef _FH8626_PIPELINE_H
#define _FH8626_PIPELINE_H

#include <linux/list.h>
#include <linux/mutex.h>
#include <linux/videodev2.h>
#include <media/v4l2-device.h>
#include <media/media-device.h>
#include <media/v4l2-subdev.h>
#include <media/videobuf2-v4l2.h>

#include "fh8626-frame-contract.h"

struct platform_device;

#ifndef FH_FRAME_WIDTH_MAX
#define FH_FRAME_WIDTH_MAX		1920
#define FH_FRAME_HEIGHT_MAX		1080
#endif

/*
 * VPU output channels. Channel 0 scales the ISP output, channel 1 scales
 * it independently and channel 2 scales the output of channel 1.
 */
#define FH8626_VPU_CHANNELS		3

struct fh8626_buffer {
	struct vb2_v4l2_buffer	vb;
	struct list_head	queue;
	dma_addr_t		paddr;
};

struct fh8626_vpu_dev;
struct fh8626_isp_dev;
struct fh8626_logo_dev;

struct fh8626_pipeline {
	struct v4l2_device		v4l2_dev;
	/* Capture node of channel 0, which also carries the logo windows */
	struct video_device		*video;
	struct video_device		*chan_video[FH8626_VPU_CHANNELS];
	struct media_device		media_dev;
	struct mutex			media_lock;
	struct media_pipeline		media_pipe;
	struct v4l2_async_notifier	notifier;

	struct v4l2_subdev		*sensor_sd;
	struct v4l2_subdev		*csi_sd;
	struct v4l2_subdev		*isp_sd;

	struct fh8626_vpu_dev		*vpu;
	struct fh8626_isp_dev		*isp;
	struct fh8626_logo_dev		*logos;

	/* Serialises starting and stopping the shared producers */
	struct mutex			stream_lock;
	/* Capture channels streaming; the producers run while non-zero */
	unsigned int			stream_count;
};

int fh8626_subdev_nodes_register(struct fh8626_pipeline *pipe);
void fh8626_subdev_nodes_unregister(struct fh8626_pipeline *pipe);
void fh8626_video_withdraw_interface(struct video_device *vd);

/*
 * Logo slot 0 (group 0) uses input coordinates and covers every channel.
 * Slots 1..4 (group 1) use channel 0 coordinates; slots 5..8 (group 2) use
 * channel 1 coordinates. Clear a group's ISP_VPU_ENABLE bypass bit to blend.
 */
#define FH8626_LOGO_SLOTS		9

/*
 * Privacy masks: opaque rectangles in input frame coordinates, drawn
 * through the group 0 window, which they then own.
 */
#define FH8626_PRIVACY_MASKS		8

struct fh8626_mask_rect {
	u16			left;
	u16			top;
	u16			width;
	u16			height;
	/* 0xRRGGBB */
	u32			color;
	bool			on;
};

struct fh8626_logo_plane {
	u32			phys;
	u16			width;
	u16			height;
	u16			stride;
	u16			x;
	u16			y;
	/* Pixel depth code: 4 for 16-bit pixels, 5 for 32-bit pixels */
	u8			depth;
	/* 16-bit layout: 1 RGB565, 2 ARGB1555, 3 ARGB4444 */
	u8			layout;
	/* Window opacity, applied on top of any per-pixel alpha */
	u8			alpha;
	bool			on;
};

int fh8626_vpu_pipeline_start(struct fh8626_vpu_dev *vpu, u32 in_width,
			      u32 in_height);
void fh8626_vpu_pipeline_stop(struct fh8626_vpu_dev *vpu);
int fh8626_vpu_chan_set_format(struct fh8626_vpu_dev *vpu, unsigned int chan,
			       u32 width, u32 height);
int fh8626_vpu_chan_start(struct fh8626_vpu_dev *vpu, unsigned int chan);
void fh8626_vpu_chan_queue(struct fh8626_vpu_dev *vpu, unsigned int chan,
			   struct fh8626_buffer *buf);
int fh8626_vpu_chan_drain(struct fh8626_vpu_dev *vpu, unsigned int chan);
void fh8626_vpu_chan_stop(struct fh8626_vpu_dev *vpu, unsigned int chan,
			  enum vb2_buffer_state state);
int fh8626_vpu_logo_program(struct fh8626_vpu_dev *vpu, unsigned int slot,
			    const struct fh8626_logo_plane *plane);
int fh8626_vpu_set_privacy_masks(struct fh8626_vpu_dev *vpu,
				 const struct fh8626_mask_rect *masks);
struct device *fh8626_vpu_device(struct fh8626_vpu_dev *vpu);
unsigned int fh8626_vpu_logo_channel(unsigned int slot);
void fh8626_vpu_logo_bounds(struct fh8626_vpu_dev *vpu, unsigned int slot,
			    u32 *width, u32 *height);
void fh8626_vpu_irq(struct fh8626_vpu_dev *vpu, u32 status, u64 timestamp);
bool fh8626_vpu_frame_starts(struct fh8626_vpu_dev *vpu, u32 *count);
int fh8626_vpu_wait_frame_starts(struct fh8626_vpu_dev *vpu, u32 since,
				 unsigned int n);
struct fh8626_vpu_dev *fh8626_vpu_create(struct device *dev,
					 struct fh8626_isp_dev *isp);
void fh8626_vpu_destroy(struct fh8626_vpu_dev *vpu);
void fh8626_vpu_set_pipeline(struct fh8626_vpu_dev *vpu,
			     struct fh8626_pipeline *pipe);

struct fh8626_isp_dev *fh8626_isp_from_subdev(struct v4l2_subdev *sd);
struct fh8626_vpu_dev *fh8626_isp_vpu(struct fh8626_isp_dev *isp);
void fh8626_isp_set_digital_gain(struct fh8626_isp_dev *isp, u16 gain_q6);
void fh8626_isp_set_awb_gains(struct fh8626_isp_dev *isp, u16 r, u16 gr,
			      u16 gb, u16 b);
int fh8626_isp_prepare(struct fh8626_isp_dev *isp);
int fh8626_isp_quiesce(struct fh8626_isp_dev *isp);
void __iomem *fh8626_isp_base(struct fh8626_isp_dev *isp);
int fh8626_isp_irq_number(struct fh8626_isp_dev *isp);
void fh8626_isp_update_bits(struct fh8626_isp_dev *isp, u32 reg, u32 mask,
			    u32 value);
void fh8626_isp_dma_fault(struct fh8626_isp_dev *isp);
int fh8626_isp_dma_begin(struct fh8626_isp_dev *isp);
void fh8626_isp_dma_end(struct fh8626_isp_dev *isp);

int fh8626_video_register(struct platform_device *pdev,
			  struct fh8626_pipeline *pipe);
int fh8626_logo_register(struct platform_device *pdev,
			 struct fh8626_pipeline *pipe);
void fh8626_logo_unregister(struct fh8626_pipeline *pipe);
void fh8626_video_ready(struct fh8626_pipeline *pipe);
void fh8626_video_unregister(struct fh8626_pipeline *pipe);
void fh8626_video_shutdown(struct fh8626_pipeline *pipe);

extern struct platform_driver fh8626_csi_driver;
extern struct platform_driver fh8626_isp_driver;
extern struct platform_driver fh8626_camera_driver;

#endif /* _FH8626_PIPELINE_H */
