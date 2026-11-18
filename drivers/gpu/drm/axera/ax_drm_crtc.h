/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/

#ifndef __AX_DRM_CRTC_H
#define __AX_DRM_CRTC_H

#include "ax_drm_drv.h"

#define AX_PLANES_MAX 3

struct logo_mem_reserved {
	u32 id;
	phys_addr_t base;
	phys_addr_t size;
};

enum composer_state {
	COMPOSER_INIT_STATE = 0,
	NEED_COMPOSE_STATE,
	COMPOSE_START_STATE,
	COMPOSE_DONE_STATE,
};

struct ax_composer {
	int num_active_planes;
	struct drm_plane_state *active_planes[AX_PLANES_MAX];
};

enum {
	CRTC_STATUS_INIT = 0,
	CRTC_STATUS_CONFIGURED,
	CRTC_STATUS_ENABLED,
	CRTC_STATUS_DISABLED,
};


/* CRTC definition */
struct ax_crtc {
	u32 id;
	u32 status;
	struct drm_crtc base;
	struct drm_pending_vblank_event *event;
	struct ax_drm_mgr *drm_mgr;

	struct drm_plane *primary_plane;

	int num_planes;
	struct ax_plane *plane;

	u32 fb_nr;
	struct ax_fb fb[AX_PLANES_MAX];

	struct ax_composer composer;

	u32 work_mode;

	struct ax_disp_mode mode;
	struct ax_display_dev *display_dev;

	struct ax_disp_props disp_props;

	struct {
		struct drm_property *id;
		struct drm_property *brightness;
		struct drm_property *contrast;
		struct drm_property *saturation;
		struct drm_property *hue;
		struct drm_property *csc;
		struct drm_property *work_mode;
	} props;
};

#define to_ax_crtc(x) container_of(x, struct ax_crtc, base)

int ax_dispc_fb_info_ioctl(struct drm_device *dev, void *data,
			   struct drm_file *filp);
int ax_dispc_sleep_sta_ioctl(struct drm_device *dev, void *data,
			     struct drm_file *filp);

#endif /* __AX_DRM_CRTC_H */
