/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/

#include <linux/kernel.h>
#include <drm/drm.h>
#include <drm/drm_print.h>
#include <drm/drm_atomic.h>
#include <drm/drm_fb_helper.h>
#include <drm/drm_crtc_helper.h>
#include <drm/drm_gem_framebuffer_helper.h>

#include "ax_drm_drv.h"
#include "ax_drm_fb.h"
#include "ax_drm_gem.h"

static const struct drm_framebuffer_funcs ax_drm_fb_funcs = {
	.destroy = drm_gem_fb_destroy,
	.create_handle = drm_gem_fb_create_handle,
};

static struct drm_framebuffer *ax_fb_alloc(struct drm_device *dev, const struct drm_mode_fb_cmd2
					   *mode_cmd,
					   struct drm_gem_object **obj,
					   unsigned int num_planes)
{
	struct drm_framebuffer *fb;
	int ret;
	int i;

	DRM_DEBUG_DRIVER("enter\n");

	fb = kzalloc(sizeof(*fb), GFP_KERNEL);
	if (!fb)
		return ERR_PTR(-ENOMEM);

	drm_helper_mode_fill_fb_struct(dev, fb, mode_cmd);

	for (i = 0; i < num_planes; i++)
		fb->obj[i] = obj[i];

	ret = drm_framebuffer_init(dev, fb, &ax_drm_fb_funcs);
	if (ret) {
		DRM_DEV_ERROR(dev->dev,
			      "Failed to initialize framebuffer: %d\n", ret);
		kfree(fb);
		return ERR_PTR(ret);
	}

	DRM_DEBUG_DRIVER("done\n");

	return fb;
}

static const struct drm_mode_config_helper_funcs ax_mode_config_helpers = {
	.atomic_commit_tail = drm_atomic_helper_commit_tail_rpm,
};

static const struct drm_mode_config_funcs ax_drm_mode_config_funcs = {
	.fb_create = drm_gem_fb_create,
	.output_poll_changed = drm_fb_helper_output_poll_changed,
	.atomic_check = drm_atomic_helper_check,
	.atomic_commit = drm_atomic_helper_commit,
};

struct drm_framebuffer *ax_drm_framebuffer_init(struct drm_device *dev, const struct drm_mode_fb_cmd2
						*mode_cmd,
						struct drm_gem_object *obj)
{
	struct drm_framebuffer *fb;

	fb = ax_fb_alloc(dev, mode_cmd, &obj, 1);
	if (IS_ERR(fb))
		return ERR_CAST(fb);

	return fb;
}

void ax_drm_mode_config_init(struct drm_device *dev)
{
	dev->mode_config.min_width = 0;
	dev->mode_config.min_height = 0;

	/*
	 * set max width and height as default value(4096x4096).
	 * this value would be used to check framebuffer size limitation
	 * at drm_mode_addfb().
	 */
	dev->mode_config.max_width = 4096;
	dev->mode_config.max_height = 4096;

	dev->mode_config.funcs = &ax_drm_mode_config_funcs;
	dev->mode_config.helper_private = &ax_mode_config_helpers;
	dev->mode_config.allow_fb_modifiers = true;
	dev->mode_config.preferred_depth = 32;
	dev->mode_config.prefer_shadow = 1;
}
