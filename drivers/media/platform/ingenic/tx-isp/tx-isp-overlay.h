/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2026 Difan Zhang */
#ifndef __TX_ISP_OVERLAY_H__
#define __TX_ISP_OVERLAY_H__

#include <linux/dma-buf.h>
#include <linux/list.h>
#include <linux/mutex.h>
#include <linux/spinlock.h>
#include <media/media-entity.h>
#include <media/v4l2-ctrls.h>
#include <media/v4l2-dev.h>
#include <media/v4l2-device.h>
#include <media/videobuf2-v4l2.h>

#include "tx-isp-device.h"

#define T23_OVERLAY_INPUTS 2

struct t23_overlay;
struct t23_overlay_input;
struct t23_osd_buffer;

/* One OSD bitmap output node */
struct t23_osd_region {
	struct t23_overlay_input *input;
	unsigned int index;
	struct video_device vdev;
	struct media_pad pad;
	struct vb2_queue queue;
	struct list_head bufs;		/* queued, not yet shown */
	struct t23_osd_buffer *shown;	/* read by the ISP on each frame */
	struct v4l2_pix_format pix;
	u16 left;
	u16 top;
	bool streaming;
	bool queue_initialized;
	bool entity_initialized;
	bool registered;
};

/* Privacy masks, vector lines and OSD regions of one input */
struct t23_overlay_input {
	struct t23_overlay *overlay;
	unsigned int index;
	u16 width;			/* input frame, 0 until a sensor binds */
	u16 height;
	struct tx_isp_rect mask[T23_MSCA_MASKS];
	u32 mask_yuv;
	struct tx_isp_line line[T23_MSCA_LINES];
	u32 line_yuv;
	struct t23_osd_region osd[T23_MSCA_OSD_REGIONS];
};

struct t23_overlay {
	struct device *dev;
	spinlock_t lock;
	struct t23_overlay_input inputs[T23_OVERLAY_INPUTS];
	/* Wait until no frame of @input is in flight; false on timeout. */
	bool (*sync)(struct t23_overlay *overlay, unsigned int input);
};

void t23_overlay_init(struct t23_overlay *overlay, struct device *dev,
		      bool (*sync)(struct t23_overlay *, unsigned int));
void t23_overlay_init_controls(struct t23_overlay *overlay, unsigned int input,
			       struct v4l2_ctrl_handler *hdl);
/* The ISP entity of each input has a sink pad per OSD region from @osd_pad. */
int t23_overlay_register(struct t23_overlay *overlay,
			 struct v4l2_device *v4l2_dev, struct mutex *lock,
			 struct media_entity *isp[], unsigned int osd_pad);
void t23_overlay_unregister(struct t23_overlay *overlay);
void t23_overlay_release(struct t23_overlay *overlay);
void t23_overlay_set_frame(struct t23_overlay *overlay, unsigned int input,
			   u32 width, u32 height);
/*
 * Fill @out for the frame of @input being armed. Runs in atomic context once
 * the previous frame has completed, so a replaced OSD buffer is returned.
 */
void t23_overlay_frame(struct t23_overlay *overlay, unsigned int input,
		       struct tx_isp_msca_overlay *out);

#endif
