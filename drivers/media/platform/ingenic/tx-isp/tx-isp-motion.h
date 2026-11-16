/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2026 Difan Zhang */
#ifndef __TX_ISP_MOTION_H__
#define __TX_ISP_MOTION_H__

#include <linux/bitmap.h>
#include <linux/media/ingenic/t23-isp-config.h>
#include <media/v4l2-ctrls.h>
#include <media/v4l2-dev.h>
#include <media/v4l2-subdev.h>

#define T23_MOTION_REGIONS	32

struct t23_motion {
	struct v4l2_subdev *sd;		/* ISP subdev receiving events */
	struct video_device *vdev[3];	/* nodes that receive the events */
	s32 mode;			/* enum v4l2_detect_md_mode */
	u16 threshold;
	u16 cell_threshold[T23_ISP_ZONES];
	u8 cell_region[T23_ISP_ZONES];
	u32 background[T23_ISP_ZONES];	/* zone luma sum, Q8 */
	u32 luma[T23_ISP_ZONES];	/* this frame's zone luma sums */
	u32 ratio[T23_ISP_ZONES];	/* scratch for the frame gain */
	DECLARE_BITMAP(over, T23_ISP_ZONES);	/* zones over threshold last frame */
	u8 hold[T23_MOTION_REGIONS];	/* frames left to report a region */
	u8 learning;			/* frames left before reporting */
	u32 mask;			/* regions last reported */
	bool valid;
};

void t23_motion_init(struct t23_motion *md, struct v4l2_ctrl_handler *hdl,
		     struct v4l2_subdev *sd,
		     struct video_device *main, struct video_device *scaled0,
		     struct video_device *scaled1);
void t23_motion_reset(struct t23_motion *md);
/* @ae: the input's AE statistics as the ISP wrote them */
void t23_motion_frame(struct t23_motion *md, const u32 *ae, u32 sequence);

#endif
