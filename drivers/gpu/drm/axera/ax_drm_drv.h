/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/

#ifndef __AX_DRM_DRV_H
#define __AX_DRM_DRV_H

#include <drm/drm_device.h>
#include <drm/drm_fb_helper.h>
#include <drm/drm_atomic_helper.h>
#include <drm/drm_gem.h>

#include <linux/module.h>
#include <linux/component.h>

#include <linux/ax_display_hal.h>

#define AX_CRTC_MAX 2

enum {
	AX_DISP_DEV_STATUS_UNREGISTERED,
	AX_DISP_DEV_STATUS_REGISTERED,
	AX_DISP_DEV_STATUS_UNUSED,
	AX_DISP_DEV_STATUS_USED,
};

struct ax_display_dev {
	int status;
	struct ax_display_funcs *display_funs;
	void *data;
};

struct ax_display_mgr {
	struct ax_display_dev display_dev[AX_CRTC_MAX];
};

struct ax_drm_fbdev {
	u32 id;

	u32 size;
	u32 width;
	u32 height;
	u32 bpp;
	u32 buf_nr;

	struct drm_gem_object *fbdev_bo;

	struct notifier_block notifier;

	struct device *dev;
	struct drm_device *drm_dev;

	void *data;
};

struct ax_display_init_state {
	bool active;
	u32 interface_type;
	struct drm_display_mode mode;
};

struct ax_drm_mgr {
	int num_crtcs;
	struct drm_crtc *crtc[AX_CRTC_MAX];

	struct ax_display_mgr *dp_mgr;

	struct drm_device *drm_dev;

	struct ax_display_init_state init_state;

	struct mutex sleep_lock;
	struct drm_atomic_state *state;
};

extern struct platform_driver crtc_platform_driver;
extern struct platform_driver ax_vfb_driver;
extern struct platform_driver ax_mipi_dsi_driver;
extern struct platform_driver bt_dpi_platform_driver;
extern struct platform_driver ax_lvds_platform_driver;
#endif /* __AX_DRM_DRV_H */
