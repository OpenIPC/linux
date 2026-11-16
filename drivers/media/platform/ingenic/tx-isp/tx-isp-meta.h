/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2026 Difan Zhang */
#ifndef __TX_ISP_META_H__
#define __TX_ISP_META_H__

#include <linux/list.h>
#include <linux/mutex.h>
#include <linux/spinlock.h>
#include <media/media-entity.h>
#include <media/v4l2-dev.h>
#include <media/v4l2-device.h>
#include <media/videobuf2-v4l2.h>

#define T23_META_INPUTS 2

struct t23_meta;
struct tx_isp_subdev;

struct t23_meta_node {
	struct t23_meta *meta;
	unsigned int input;
	bool output;
	u32 dataformat;
	struct video_device vdev;
	struct media_pad pad;
	struct vb2_queue queue;
	struct list_head bufs;
	bool streaming;
	bool queue_initialized;
	bool entity_initialized;
	bool registered;
};

struct t23_meta {
	struct device *dev;
	spinlock_t lock;
	struct t23_meta_node stats[T23_META_INPUTS];
	struct t23_meta_node params[T23_META_INPUTS];
	bool reset[T23_META_INPUTS];	/* drop the input's parameters */
};

struct t23_meta_frame {
	u32 sequence;
	u64 timestamp;
	const void *hw;		/* AE at +0, histogram at +0x1000, AWB at +0x2000 */
	u32 valid;		/* T23_ISP_STATS_* */
	u16 zone_width;
	u16 zone_height;
	u8 ae_low;
	u8 ae_high;
};

/*
 * The ISP entity of each input must have a sink pad @params_pad and a source
 * pad @stats_pad.
 */
int t23_meta_register(struct t23_meta *meta, struct v4l2_device *v4l2_dev,
		      struct mutex *lock, struct media_entity *isp[],
		      unsigned int params_pad, unsigned int stats_pad);
void t23_meta_unregister(struct t23_meta *meta);
void t23_meta_release(struct t23_meta *meta);
/* Both run in atomic context: the capture path holds a spinlock. */
void t23_meta_params_apply(struct t23_meta *meta, unsigned int input,
			   struct tx_isp_subdev *core);
void t23_meta_stats_done(struct t23_meta *meta, unsigned int input,
			 const struct t23_meta_frame *frame);

#endif
