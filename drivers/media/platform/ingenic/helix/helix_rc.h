/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2026 Difan Zhang */

#ifndef _HELIX_RC_H_
#define _HELIX_RC_H_

#include <linux/types.h>

/* Frame-level H.264 rate control state of one context */
struct helix_rc {
	bool enabled;
	u32 mode;		/* V4L2_MPEG_VIDEO_BITRATE_MODE_* */
	u32 bitrate;		/* bits per second */
	u32 peak;
	u32 tpf_num;		/* seconds per frame */
	u32 tpf_den;

	bool started;
	u64 complexity[2];	/* bits * Qstep (Q8): last I, mean of recent P */
	u32 last_qp[2];
	u32 mean_qp;		/* of recent P frames, Q4 */
	u32 up;			/* largest QP rise for the next P frame */
	u64 target;		/* of the frame being coded */
	s64 left;		/* bits left in the current window */
	u32 frames;		/* frames left in the current window */
};

void helix_rc_init(struct helix_rc *rc);
void helix_rc_reset(struct helix_rc *rc);
u32 helix_rc_fps(const struct helix_rc *rc);
/*
 * QP for the next frame. @idr starts a window of @window frames;
 * @init_qp is used until the frame type has a history.
 */
u32 helix_rc_qp(struct helix_rc *rc, bool idr, u32 window, u32 init_qp,
		u32 min_qp, u32 max_qp);
void helix_rc_update(struct helix_rc *rc, bool idr, u32 qp, u32 bits);

#endif
