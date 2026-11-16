// SPDX-License-Identifier: GPL-2.0
// Copyright (C) 2026 Difan Zhang
/*
 * Frame-level H.264 CBR/VBR rate control. Allocates bit budget per GOP
 * window based on frame complexity and regulates QP step sizes.
 */
#include <linux/kernel.h>
#include <linux/math64.h>
#include <linux/videodev2.h>

#include "helix_rc.h"

#define RC_I	0
#define RC_P	1
/* P frames get a Qstep 2^(2/6) above the I frame, in Q8 */
#define RC_KP	323

/* H.264 Qstep in Q8: 0.625 at QP 0, doubling every 6 */
static u32 rc_qstep(u32 qp)
{
	static const u16 base[6] = { 160, 176, 208, 224, 256, 288 };

	return base[qp % 6] << (qp / 6);
}

/* The QP whose Qstep is nearest @q on a log scale */
static u32 rc_qp_for_qstep(u64 q)
{
	u32 qp;

	for (qp = 0; qp < 51 && rc_qstep(qp) < q; qp++)
		;
	if (qp && q * q < (u64)rc_qstep(qp) * rc_qstep(qp - 1))
		qp--;
	return qp;
}

static u64 rc_frame_bits(const struct helix_rc *rc, u32 bitrate)
{
	return max_t(u64, div_u64((u64)bitrate * rc->tpf_num, rc->tpf_den), 1);
}

u32 helix_rc_fps(const struct helix_rc *rc)
{
	return max(DIV_ROUND_CLOSEST(rc->tpf_den, rc->tpf_num), 1U);
}

void helix_rc_init(struct helix_rc *rc)
{
	rc->enabled = false;
	rc->mode = V4L2_MPEG_VIDEO_BITRATE_MODE_VBR;
	rc->bitrate = 2000000;
	rc->peak = 4000000;
	rc->tpf_num = 1;
	rc->tpf_den = 25;
	helix_rc_reset(rc);
}

void helix_rc_reset(struct helix_rc *rc)
{
	rc->started = false;
	rc->complexity[RC_I] = 0;
	rc->complexity[RC_P] = 0;
	rc->left = 0;
	rc->frames = 0;
	rc->up = 2;
}

static void rc_start_window(struct helix_rc *rc, u32 frames)
{
	s64 budget = rc_frame_bits(rc, rc->bitrate);
	s64 second = budget * helix_rc_fps(rc);
	s64 carry = rc->started ? rc->left - budget * rc->frames : 0;
	s64 total;

	if (rc->mode == V4L2_MPEG_VIDEO_BITRATE_MODE_CBR)
		carry = clamp(carry, -2 * second, second / 2);
	else
		carry = clamp(carry, -2 * second, 4 * second);
	total = budget * frames + carry;
	if (rc->mode == V4L2_MPEG_VIDEO_BITRATE_MODE_VBR)
		total = min_t(s64, total,
			      rc_frame_bits(rc, max(rc->peak, rc->bitrate)) * frames);
	rc->left = max(total, budget * frames / 4);
	rc->frames = frames;
	rc->started = true;
}

u32 helix_rc_qp(struct helix_rc *rc, bool idr, u32 window, u32 init_qp,
		u32 min_qp, u32 max_qp)
{
	int type = idr ? RC_I : RC_P;
	s64 budget = rc_frame_bits(rc, rc->bitrate);
	u64 floor = max_t(s64, budget / 8, 1);
	u64 xi, xp, w;
	s64 target;
	u32 qp;

	if (idr || !rc->started || !rc->frames)
		rc_start_window(rc, max(window, 1U));

	if (!rc->complexity[type] || (idr && !rc->complexity[RC_P])) {
		rc->target = budget;
		return clamp(init_qp, min_qp, max_qp);
	}

	if (idr) {
		xi = max_t(u64, rc->complexity[RC_I] >> 8, 1);
		xp = rc->complexity[RC_P] >> 8;
		w = div64_u64(xi * RC_KP << 16,
			      xi * RC_KP + (u64)(rc->frames - 1) * xp * 256);
		target = rc->left > 0 ? (u64)rc->left * w >> 16 : 0;
	} else {
		target = budget + div_s64(rc->left - budget * rc->frames,
					  max(rc->frames, helix_rc_fps(rc)));
	}
	rc->target = max_t(s64, target, floor);
	qp = rc_qp_for_qstep(div64_u64(rc->complexity[type], rc->target));

	if (idr) {
		u32 p = DIV_ROUND_CLOSEST(rc->mean_qp, 16);
		u32 below = rc->mode == V4L2_MPEG_VIDEO_BITRATE_MODE_CBR ? 2 : 4;

		qp = clamp(qp, p > below ? p - below : 0, p);
	} else {
		u32 p = rc->last_qp[RC_P];

		qp = clamp(qp, p ? p - 1 : 0, p + rc->up);
	}
	return clamp(qp, min_qp, max_qp);
}

void helix_rc_update(struct helix_rc *rc, bool idr, u32 qp, u32 bits)
{
	int type = idr ? RC_I : RC_P;
	u64 x = max_t(u64, (u64)bits * rc_qstep(qp), 1);

	if (idr) {
		rc->complexity[RC_I] = x;
	} else if (!rc->complexity[RC_P]) {
		rc->complexity[RC_P] = x;
		rc->mean_qp = qp * 16;
	} else {
		rc->complexity[RC_P] = (3 * rc->complexity[RC_P] + x) / 4;
		rc->mean_qp += ((int)qp * 16 - (int)rc->mean_qp) / 4;
	}
	rc->up = bits > 4 * rc->target ? 6 : bits > 2 * rc->target ? 4 : 2;
	rc->last_qp[type] = qp;
	rc->left -= bits;
	if (rc->frames)
		rc->frames--;
}
