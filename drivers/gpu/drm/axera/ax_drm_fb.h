/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/

#ifndef __AX_DRM_FB_H
#define __AX_DRM_FB_H

struct drm_framebuffer *ax_drm_framebuffer_init(struct drm_device *dev,
						const struct drm_mode_fb_cmd2
						*mode_cmd,
						struct drm_gem_object *obj);
void ax_drm_framebuffer_fini(struct drm_framebuffer *fb);

void ax_drm_mode_config_init(struct drm_device *dev);

#endif /* __AX_DRM_FB_H */
