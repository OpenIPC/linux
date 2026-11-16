// SPDX-License-Identifier: GPL-2.0
// Copyright (C) 2026 Difan Zhang
/*
 * Motion detection on the 15x15 AE zone statistics, reported through the
 * V4L2 detection controls and V4L2_EVENT_MOTION_DET.
 *
 * Each zone keeps a background luma. The median ratio of zone luma to
 * background is the frame's gain, so exposure changes and global light
 * changes cancel while a moving subject in fewer than half the zones does
 * not. Zones are compared on a square-root scale with the frame mean at
 * 128, close to the output gamma: a 1% flicker in a highlight five times
 * the mean is two levels there, against eight on a linear scale. A zone
 * moves when it differs from its scaled background by more than its
 * threshold in those levels on two frames running, which drops the
 * single frames where the sensor applies exposure and gain apart. A change
 * in most zones at once is a lamp switching: it is reported, and the
 * background is learned again while auto exposure settles.
 */
#include <linux/math64.h>
#include <media/v4l2-event.h>

#include "tx-isp-motion.h"

#define MD_DEFAULT_THRESHOLD	8
#define MD_HOLD_FRAMES		12	/* about half a second */
#define MD_TRACK_SHIFT		4	/* background follows a still zone */
#define MD_TRACK_MOVING_SHIFT	5	/* and absorbs a stopped subject */
#define MD_LEARN_FRAMES		24	/* about a second of fast tracking */
#define MD_GAIN_ONE		(1U << 12)

static const struct v4l2_ctrl_ops t23_motion_ops;

static const struct v4l2_ctrl_config t23_motion_threshold_grid = {
	.ops = &t23_motion_ops,
	.id = V4L2_CID_DETECT_MD_THRESHOLD_GRID,
	.dims = { T23_ISP_GRID_ROWS, T23_ISP_GRID_COLS },
	.max = 255,
	.step = 1,
	.def = MD_DEFAULT_THRESHOLD,
};

static const struct v4l2_ctrl_config t23_motion_region_grid = {
	.ops = &t23_motion_ops,
	.id = V4L2_CID_DETECT_MD_REGION_GRID,
	.dims = { T23_ISP_GRID_ROWS, T23_ISP_GRID_COLS },
	.max = T23_MOTION_REGIONS - 1,
	.step = 1,
};

static void t23_motion_queue(struct t23_motion *md, u32 mask, u32 sequence)
{
	struct v4l2_event ev = {
		.type = V4L2_EVENT_MOTION_DET,
		.u.motion_det = {
			.flags = V4L2_EVENT_MD_FL_HAVE_FRAME_SEQ,
			.frame_sequence = sequence,
			.region_mask = mask,
		},
	};
	unsigned int i;

	md->mask = mask;
	if (md->sd && md->sd->devnode)
		v4l2_subdev_notify_event(md->sd, &ev);
	for (i = 0; i < ARRAY_SIZE(md->vdev); i++)
		if (md->vdev[i] && video_is_registered(md->vdev[i]))
			v4l2_event_queue(md->vdev[i], &ev);
}

void t23_motion_reset(struct t23_motion *md)
{
	md->valid = false;
	memset(md->hold, 0, sizeof(md->hold));
	if (md->mask)
		t23_motion_queue(md, 0, 0);
}

static int t23_motion_s_ctrl(struct v4l2_ctrl *ctrl)
{
	struct t23_motion *md = ctrl->priv;

	switch (ctrl->id) {
	case V4L2_CID_DETECT_MD_MODE:
		md->mode = ctrl->val;
		t23_motion_reset(md);
		break;
	case V4L2_CID_DETECT_MD_GLOBAL_THRESHOLD:
		md->threshold = ctrl->val;
		break;
	case V4L2_CID_DETECT_MD_THRESHOLD_GRID:
		memcpy(md->cell_threshold, ctrl->p_new.p_u16,
		       sizeof(md->cell_threshold));
		break;
	case V4L2_CID_DETECT_MD_REGION_GRID:
		memcpy(md->cell_region, ctrl->p_new.p_u8,
		       sizeof(md->cell_region));
		break;
	default:
		return -EINVAL;
	}
	return 0;
}

static const struct v4l2_ctrl_ops t23_motion_ops = {
	.s_ctrl = t23_motion_s_ctrl,
};

void t23_motion_init(struct t23_motion *md, struct v4l2_ctrl_handler *hdl,
		     struct v4l2_subdev *sd,
		     struct video_device *main, struct video_device *scaled0,
		     struct video_device *scaled1)
{
	struct v4l2_ctrl *ctrls[4];
	unsigned int i;

	md->sd = sd;
	md->vdev[0] = main;
	md->vdev[1] = scaled0;
	md->vdev[2] = scaled1;
	md->mode = V4L2_DETECT_MD_MODE_DISABLED;
	md->threshold = MD_DEFAULT_THRESHOLD;
	for (i = 0; i < T23_ISP_ZONES; i++)
		md->cell_threshold[i] = MD_DEFAULT_THRESHOLD;
	ctrls[0] = v4l2_ctrl_new_std_menu(hdl, &t23_motion_ops,
					  V4L2_CID_DETECT_MD_MODE,
					  V4L2_DETECT_MD_MODE_REGION_GRID, 0,
					  V4L2_DETECT_MD_MODE_DISABLED);
	ctrls[1] = v4l2_ctrl_new_std(hdl, &t23_motion_ops,
				     V4L2_CID_DETECT_MD_GLOBAL_THRESHOLD,
				     1, 255, 1, MD_DEFAULT_THRESHOLD);
	ctrls[2] = v4l2_ctrl_new_custom(hdl, &t23_motion_threshold_grid, NULL);
	ctrls[3] = v4l2_ctrl_new_custom(hdl, &t23_motion_region_grid, NULL);
	for (i = 0; i < ARRAY_SIZE(ctrls); i++)
		if (ctrls[i])
			ctrls[i]->priv = md;
}

/* Median of @n values, reordering them */
static u32 t23_motion_median(u32 *v, int n)
{
	int lo = 0, hi = n - 1, k = n / 2;

	while (lo < hi) {
		u32 pivot = v[(lo + hi) / 2];
		int i = lo, j = hi;

		while (i <= j) {
			while (v[i] < pivot)
				i++;
			while (v[j] > pivot)
				j--;
			if (i <= j)
				swap(v[i++], v[j--]);
		}
		if (k <= j)
			hi = j;
		else if (k >= i)
			lo = i;
		else
			break;
	}
	return v[k];
}

/* Luma sum of zone @i: the low, middle and high band sums. */
static u32 t23_motion_luma(const u32 *ae, unsigned int i)
{
	const u32 *w = ae + i * 4;

	return (w[0] & 0x1fffff) + ((w[1] & 0x3ff) << 11 | w[0] >> 21) +
	       ((w[1] & 0x7ffffc00) >> 10);
}

/* 16 * 128 * sqrt(@sum / mean) for @scale = 2^48 / mean, mean in Q8 */
static u32 t23_motion_level(u32 sum, u32 scale)
{
	u32 ratio = min_t(u64, (u64)sum * scale >> 28, 1U << 20); /* Q12 */

	return int_sqrt(ratio << 10);
}

void t23_motion_frame(struct t23_motion *md, const u32 *ae, u32 sequence)
{
	u32 *luma = md->luma, gain, scale, regions = 0, mask = 0, moved = 0;
	u64 mean = 0;
	unsigned int i;
	s32 mode = READ_ONCE(md->mode);

	if (mode == V4L2_DETECT_MD_MODE_DISABLED)
		return;
	for (i = 0; i < T23_ISP_ZONES; i++)
		luma[i] = t23_motion_luma(ae, i);
	if (!md->valid) {
		for (i = 0; i < T23_ISP_ZONES; i++)
			md->background[i] = luma[i] << 8;
		bitmap_zero(md->over, T23_ISP_ZONES);
		md->learning = MD_LEARN_FRAMES;
		md->valid = true;
		return;
	}

	for (i = 0; i < T23_ISP_ZONES; i++)
		md->ratio[i] = (luma[i] << 8) / max(md->background[i] >> 12, 1U);
	gain = t23_motion_median(md->ratio, T23_ISP_ZONES);
	/* A day/night switch or a lamp halves or doubles the whole frame. */
	if (gain > 2 * MD_GAIN_ONE || gain < MD_GAIN_ONE / 2) {
		md->valid = false;
		return;
	}
	for (i = 0; i < T23_ISP_ZONES; i++) {
		md->background[i] = min_t(u64, (u64)md->background[i] * gain >> 12,
					  U32_MAX);
		mean += md->background[i];
	}
	mean = div_u64(mean, T23_ISP_ZONES);
	/* Below a zone mean of one level the frame is black. */
	if (mean <= 1U << 16)
		return;
	scale = div64_u64(1ULL << 48, mean);

	for (i = 0; i < T23_ISP_ZONES; i++) {
		s64 delta = ((s64)luma[i] << 8) - md->background[i];
		u32 threshold = mode == V4L2_DETECT_MD_MODE_GLOBAL ?
				md->threshold : md->cell_threshold[i];
		u32 now = t23_motion_level(luma[i], scale);
		u32 then = t23_motion_level(md->background[i] >> 8, scale);
		u32 level = abs((s32)(now - then));
		bool over = threshold && level > threshold * 16 && !md->learning;
		bool moving = over && test_bit(i, md->over);
		unsigned int shift = md->learning ? 1 : moving ?
				     MD_TRACK_MOVING_SHIFT : MD_TRACK_SHIFT;

		if (moving) {
			regions |= BIT(mode == V4L2_DETECT_MD_MODE_REGION_GRID ?
				       md->cell_region[i] : 0);
			moved++;
		}
		__assign_bit(i, md->over, over);
		md->background[i] += delta >> shift;
	}
	if (md->learning)
		md->learning--;
	if (moved > T23_ISP_ZONES / 2)
		md->valid = false;
	for (i = 0; i < T23_MOTION_REGIONS; i++) {
		if (regions & BIT(i))
			md->hold[i] = MD_HOLD_FRAMES;
		else if (md->hold[i])
			md->hold[i]--;
		if (md->hold[i])
			mask |= BIT(i);
	}
	if (mask != md->mask)
		t23_motion_queue(md, mask, sequence);
}
