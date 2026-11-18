/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/

#ifndef __AX_PLANE_H
#define __AX_PLANE_H

#include "ax_drm_drv.h"

struct ax_plane {
	struct drm_plane base;
	struct {
		struct drm_property *crtc_x_offs;
		struct drm_property *blk_id_y;
		struct drm_property *blk_id_c;
		struct drm_property *phy_addr_y;
		struct drm_property *phy_addr_c;
		struct drm_property *layer_id;
		struct drm_property *active;
		struct drm_property *colorkey;
	} props;

	struct ax_fb fb;
};

#define to_ax_plane(x) container_of(x, struct ax_plane, base)

int ax_plane_create(struct drm_crtc *crtc);

#endif /* __AX_PLANE_H */
