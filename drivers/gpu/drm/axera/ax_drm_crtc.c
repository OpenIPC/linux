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
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/platform_device.h>
#include <linux/memblock.h>
#include <linux/of_fdt.h>
#include <linux/of_reserved_mem.h>
#include <linux/sort.h>

#include <drm/drm_print.h>
#include <drm/drm_atomic.h>
#include <drm/drm_atomic_helper.h>
#include <drm/drm_flip_work.h>
#include <drm/drm_crtc_helper.h>
#include <drm/drm_gem_cma_helper.h>
#include <drm/drm_vblank.h>
#include <drm/drm_modeset_helper_vtables.h>

#include "ax_drm_crtc.h"
#include "ax_drm_plane.h"
#include "ax_drm_gem.h"
#include "ax_drm_virt_connector.h"

struct logo_mem_reserved boot_logo_mem[AX_DISPLAY_MAX];

static char *crtc_stat[] = {
	"init", "configured", "enabled", "disabled"
};

static char *crtc_mode[] = {
	"offline", "online"
};

static int ax_crtc_enable_vblank(struct drm_crtc *crtc)
{
	struct ax_crtc *ax_crtc = to_ax_crtc(crtc);
	struct ax_display_dev *dp_dev = ax_crtc->display_dev;
	struct ax_display_funcs *dp_funs = dp_dev->display_funs;

	DRM_DEBUG_DRIVER("enter, crtc%d %s %s\n", ax_crtc->id, crtc_mode[ax_crtc->work_mode], crtc_stat[ax_crtc->status]);

	if (!dp_funs) {
		DRM_WARN("crtc%d display funcs invalid\n", ax_crtc->id);
		return -EINVAL;
	}

	if (dp_funs->dispc_int_unmask)
		dp_funs->dispc_int_unmask(dp_dev->data);

	return 0;
}

static void ax_crtc_disable_vblank(struct drm_crtc *crtc)
{
	struct ax_crtc *ax_crtc = to_ax_crtc(crtc);
	struct ax_display_dev *dp_dev = ax_crtc->display_dev;
	struct ax_display_funcs *dp_funs = dp_dev->display_funs;

	DRM_DEBUG_DRIVER("enter, crtc%d %s %s\n", ax_crtc->id, crtc_mode[ax_crtc->work_mode], crtc_stat[ax_crtc->status]);

	if (!dp_funs) {
		DRM_WARN("crtc%d display funcs invalid\n", ax_crtc->id);
		return;
	}

	if (dp_funs->dispc_int_mask)
		dp_funs->dispc_int_mask(dp_dev->data);
}

static int compare_z_coordinate(const void *a, const void *b)
{
	struct ax_fb *fb1 = (struct ax_fb *)a;
	struct ax_fb *fb2 = (struct ax_fb *)b;

	return AX_FB_GET_PLANE_TYPE(fb1->type) - AX_FB_GET_PLANE_TYPE(fb2->type);
}

static void composer(struct drm_crtc *crtc)
{
	int i;
	struct ax_crtc *ax_crtc = to_ax_crtc(crtc);
	struct ax_composer *composer = &ax_crtc->composer;
	struct ax_plane *ax_plane;
	struct drm_plane *plane;
	struct ax_display_dev *dp_dev = ax_crtc->display_dev;
	struct ax_display_funcs *dp_funs = dp_dev->display_funs;
	struct ax_composer_info composer_info = {0,};

	DRM_DEBUG_DRIVER("num_active_planes = %d\n", composer->num_active_planes);

	if (!dp_funs) {
		DRM_WARN("crtc%d display funcs invalid\n", ax_crtc->id);
		return;
	}

	for (i = 0, ax_crtc->fb_nr = 0; i < composer->num_active_planes; i++) {
		if (composer->active_planes[i]) {
			plane = composer->active_planes[i]->plane;
			ax_plane = to_ax_plane(plane);

			DRM_DEBUG_DRIVER("plane = 0x%p, type = %d\n", plane, plane->type);
			ax_crtc->fb[ax_crtc->fb_nr] = ax_plane->fb;
			ax_crtc->fb_nr += 1;
		}
	}

	if (ax_crtc->fb_nr == 0) {
		DRM_DEBUG_DRIVER("no active planes\n");
		return;
	}

	sort(ax_crtc->fb, ax_crtc->fb_nr, sizeof(ax_crtc->fb[0]), compare_z_coordinate, NULL);

	composer_info.reso_w = crtc->mode.hdisplay;
	composer_info.reso_h = crtc->mode.vdisplay;

	composer_info.props = ax_crtc->disp_props;
	ax_crtc->disp_props.flags = 0;

	if (dp_funs->dispc_fb_commit) {
		composer_info.fb_nr = ax_crtc->fb_nr;
		composer_info.fb = ax_crtc->fb;
		dp_funs->dispc_fb_commit(dp_dev->data, &composer_info);
	}
}

static int ax_crtc_atomic_set_property(struct drm_crtc *crtc,
				       struct drm_crtc_state *state,
				       struct drm_property *property, uint64_t val)
{
	struct ax_crtc *ax_crtc = to_ax_crtc(crtc);

	if (property == ax_crtc->props.brightness) {
		ax_crtc->disp_props.brightness = (u8)val;
		ax_crtc->disp_props.flags |= DISP_PROP_FLAG_BRIGHTNESS;
	} else if (property == ax_crtc->props.contrast) {
		ax_crtc->disp_props.contrast = (u8)val;
		ax_crtc->disp_props.flags |= DISP_PROP_FLAG_CONTRAST;
	} else if (property == ax_crtc->props.saturation) {
		ax_crtc->disp_props.saturation = (u8)val;
		ax_crtc->disp_props.flags |= DISP_PROP_FLAG_SATURATION;
	} else if (property == ax_crtc->props.hue) {
		ax_crtc->disp_props.hue = (u8)val;
		ax_crtc->disp_props.flags |= DISP_PROP_FLAG_HUE;
	} else if (property == ax_crtc->props.csc) {
		ax_crtc->disp_props.csc = (u8)val;
		ax_crtc->disp_props.flags |= DISP_PROP_FLAG_CSC;
	} else if (property == ax_crtc->props.work_mode) {
		ax_crtc->work_mode = (u32)val;
	}

	return 0;
}

static int ax_crtc_atomic_get_property(struct drm_crtc *crtc,
				       const struct drm_crtc_state *state,
				       struct drm_property *property, uint64_t * val)
{
	int ret = 0;
	struct ax_crtc *ax_crtc = to_ax_crtc(crtc);
	struct ax_display_dev *dp_dev = ax_crtc->display_dev;
	struct ax_display_funcs *dp_funs = dp_dev->display_funs;
	struct ax_disp_props disp_props = {0,};

	if (property == ax_crtc->props.id) {
		*val = ax_crtc->id;
		return 0;
	} else if (property == ax_crtc->props.work_mode) {
		*val = ax_crtc->work_mode;
		return 0;
	}

	if (property == ax_crtc->props.brightness)
		disp_props.flags |= DISP_PROP_FLAG_BRIGHTNESS;
	else if (property == ax_crtc->props.contrast)
		disp_props.flags |= DISP_PROP_FLAG_CONTRAST;
	else if (property == ax_crtc->props.saturation)
		disp_props.flags |= DISP_PROP_FLAG_SATURATION;
	else if (property == ax_crtc->props.hue)
		disp_props.flags |= DISP_PROP_FLAG_HUE;
	else if (property == ax_crtc->props.csc)
		disp_props.flags |= DISP_PROP_FLAG_CSC;

	if (dp_funs && dp_funs->dispc_get_props) {
		ret = dp_funs->dispc_get_props(dp_dev->data, &disp_props);
		if (!ret) {
			if (property == ax_crtc->props.brightness)
				*val = disp_props.brightness;
			else if (property == ax_crtc->props.contrast)
				*val = disp_props.contrast;
			else if (property == ax_crtc->props.saturation)
				*val = disp_props.saturation;
			else if (property == ax_crtc->props.hue)
				*val = disp_props.hue;
			else if (property == ax_crtc->props.csc)
				*val = disp_props.csc;
		}
	}

	return ret;
}

static const struct drm_crtc_funcs ax_crtc_funcs = {
	.atomic_destroy_state = drm_atomic_helper_crtc_destroy_state,
	.atomic_duplicate_state = drm_atomic_helper_crtc_duplicate_state,
	.destroy = drm_crtc_cleanup,
	.page_flip = drm_atomic_helper_page_flip,
	.reset = drm_atomic_helper_crtc_reset,
	.set_config = drm_atomic_helper_set_config,
	.atomic_set_property = ax_crtc_atomic_set_property,
	.atomic_get_property = ax_crtc_atomic_get_property,
	.enable_vblank = ax_crtc_enable_vblank,
	.disable_vblank = ax_crtc_disable_vblank,
};

static void ax_dispc_irq_handler(void *param)
{
	unsigned long flags;
	struct ax_crtc *ax_crtc = (struct ax_crtc *)param;
	struct ax_drm_mgr *drm_mgr = ax_crtc->drm_mgr;

	/* DRM_DEBUG_DRIVER("enter, crtc%d\n", drm_crtc_index(&ax_crtc->base)); */

	drm_crtc_handle_vblank(&ax_crtc->base);

	spin_lock_irqsave(&drm_mgr->drm_dev->event_lock, flags);

	if (ax_crtc->event) {
		drm_crtc_vblank_put(&ax_crtc->base);
		ax_crtc->event = NULL;
	}
	spin_unlock_irqrestore(&drm_mgr->drm_dev->event_lock, flags);
}

static int ax_crtc_atomic_set_gamma(struct ax_crtc *ax_crtc,
				    struct drm_crtc_state *state)
{
	if (!state->color_mgmt_changed || !state->gamma_lut)
		return 0;

	ax_crtc->disp_props.gamma_blob = (u64)state->gamma_lut->data;
	ax_crtc->disp_props.flags |= DISP_PROP_FLAG_HSV_LUT;
	return 0;
}

static void ax_crtc_atomic_enable(struct drm_crtc *crtc, struct drm_crtc_state *old_crtc_state)
{
	int ret = 0;
	struct ax_crtc *ax_crtc = to_ax_crtc(crtc);
	struct logo_mem_reserved *logo_mem = &boot_logo_mem[ax_crtc->id];
	struct drm_crtc_state *crtc_state = crtc->state;
	struct drm_display_mode *adjusted_mode = &crtc_state->adjusted_mode;
	struct ax_display_dev *dp_dev = ax_crtc->display_dev;
	struct ax_display_funcs *dp_funs = dp_dev->display_funs;
	struct ax_drm_mgr *drm_mgr = ax_crtc->drm_mgr;

	DRM_DEBUG_DRIVER("enter, crtc%d %s\n", ax_crtc->id, crtc_mode[ax_crtc->work_mode]);
	DRM_DEBUG_DRIVER("mode: " DRM_MODE_FMT "\n", DRM_MODE_ARG(&crtc_state->mode));
	DRM_DEBUG_DRIVER("adjusted_mode: " DRM_MODE_FMT "\n", DRM_MODE_ARG(adjusted_mode));

	if (!dp_funs) {
		DRM_WARN("crtc%d display funcs invalid\n",ax_crtc->id);
		return;
	}

	mutex_lock(&drm_mgr->sleep_lock);

	if (dp_funs->dpu_reset)
		dp_funs->dpu_reset(dp_dev->data);

	mutex_unlock(&drm_mgr->sleep_lock);

	ax_crtc_atomic_set_gamma(ax_crtc, crtc_state);

	if (logo_mem->base && logo_mem->size) {
		DRM_DEBUG_DRIVER("free display%d boot logo, base: 0x%px, size: 0x%px\n", logo_mem->id,
				 (void *)logo_mem->base, (void *)logo_mem->size);
		memblock_free(logo_mem->base, logo_mem->size);
		logo_mem->base = 0;
		logo_mem->size = 0;
	}

	if (dp_funs->dispc_set_irq_callback)
		dp_funs->dispc_set_irq_callback(dp_dev->data, ax_dispc_irq_handler, ax_crtc);

	ax_crtc->mode.fmt_in = AX_VO_FORMAT_NV12;
	ax_crtc->mode.hdisplay = adjusted_mode->hdisplay;
	ax_crtc->mode.hsync_start = adjusted_mode->hsync_start;
	ax_crtc->mode.hsync_end = adjusted_mode->hsync_end;
	ax_crtc->mode.htotal = adjusted_mode->htotal;
	ax_crtc->mode.vdisplay = adjusted_mode->vdisplay;
	ax_crtc->mode.vsync_start = adjusted_mode->vsync_start;
	ax_crtc->mode.vsync_end = adjusted_mode->vsync_end;
	ax_crtc->mode.vtotal = adjusted_mode->vtotal;
	ax_crtc->mode.clock = adjusted_mode->clock;
	ax_crtc->mode.vrefresh = drm_mode_vrefresh(adjusted_mode);
	/* dpu registor hs vs polarity bit is 0 for low active,
	 * 1 for high active. For history reason, Invert hs vs
	 * setting . */
	ax_crtc->mode.hp_pol = (adjusted_mode->flags & DRM_MODE_FLAG_PHSYNC) ? 0 : 1;
	ax_crtc->mode.vp_pol = (adjusted_mode->flags & DRM_MODE_FLAG_PVSYNC) ? 0 : 1;

	if (dp_funs->dispc_config) {
		ret = dp_funs->dispc_config(dp_dev->data,ax_crtc->work_mode, &ax_crtc->mode);
		if (!ret) {
			ax_crtc->status = CRTC_STATUS_CONFIGURED;
			DRM_DEBUG_DRIVER("crtc%d %s %s\n", ax_crtc->id, crtc_mode[ax_crtc->work_mode], crtc_stat[ax_crtc->status]);
		} else {
			DRM_ERROR("crtc%d %s config failed\n", ax_crtc->id, crtc_mode[ax_crtc->work_mode]);
		}

		dp_dev->status = AX_DISP_DEV_STATUS_USED;
	}
}

static void ax_crtc_atomic_disable(struct drm_crtc *crtc, struct drm_crtc_state *old_crtc_state)
{
	struct ax_crtc *ax_crtc = to_ax_crtc(crtc);
	struct ax_display_dev *dp_dev = ax_crtc->display_dev;
	struct ax_display_funcs *dp_funs = dp_dev->display_funs;
	struct ax_drm_mgr *drm_mgr = ax_crtc->drm_mgr;

	DRM_DEBUG_DRIVER("enter, crtc%d %s %s\n", ax_crtc->id, crtc_mode[ax_crtc->work_mode], crtc_stat[ax_crtc->status]);

	memset(&ax_crtc->mode, 0, sizeof(ax_crtc->mode));

	if (!dp_funs) {
		DRM_WARN("crtc%d display funcs invalid\n", ax_crtc->id);
		return;
	}

	if (dp_funs->dispc_disable) {
		dp_funs->dispc_disable(dp_dev->data, AX_DISP_CTRL_TYPE_NORMAL);
		dp_dev->status = AX_DISP_DEV_STATUS_UNUSED;
	}

	drm_crtc_vblank_off(crtc);

	if (crtc->state->event && !crtc->state->active) {
		spin_lock_irq(&crtc->dev->event_lock);
		drm_crtc_send_vblank_event(crtc, crtc->state->event);
		spin_unlock_irq(&crtc->dev->event_lock);

		crtc->state->event = NULL;
	}

	mutex_lock(&drm_mgr->sleep_lock);

	mutex_unlock(&drm_mgr->sleep_lock);

	ax_crtc->status = CRTC_STATUS_DISABLED;
}

static void ax_crtc_atomic_begin(struct drm_crtc *crtc, struct drm_crtc_state *old_crtc_state)
{
	unsigned long flags;
	struct ax_crtc *ax_crtc = to_ax_crtc(crtc);

	DRM_DEBUG_DRIVER("enter, crtc%d %s %s\n", ax_crtc->id, crtc_mode[ax_crtc->work_mode], crtc_stat[ax_crtc->status]);

	drm_crtc_vblank_on(crtc);

	if (crtc->state->event) {
		WARN_ON(drm_crtc_vblank_get(crtc) != 0);

		spin_lock_irqsave(&crtc->dev->event_lock, flags);
		ax_crtc->event = crtc->state->event;
		if (ax_crtc->event) {
			drm_crtc_send_vblank_event(&ax_crtc->base, ax_crtc->event);
		}
		spin_unlock_irqrestore(&crtc->dev->event_lock, flags);
		crtc->state->event = NULL;
	}
}

static void ax_crtc_atomic_flush(struct drm_crtc *crtc,
				 struct drm_crtc_state *old_crtc_state)
{
	struct ax_crtc *ax_crtc = to_ax_crtc(crtc);
	struct ax_display_dev *dp_dev = ax_crtc->display_dev;
	struct ax_display_funcs *dp_funs = dp_dev->display_funs;

	DRM_DEBUG_DRIVER("crtc%d %s %s\n", ax_crtc->id, crtc_mode[ax_crtc->work_mode], crtc_stat[ax_crtc->status]);

	composer(crtc);
	if (ax_crtc->status == CRTC_STATUS_CONFIGURED) {
		if (dp_funs && dp_funs->dispc_enable)
			dp_funs->dispc_enable(dp_dev->data, AX_DISP_CTRL_TYPE_NORMAL);

		ax_crtc->status = CRTC_STATUS_ENABLED;
	}
}

static int ax_crtc_atomic_check_gamma(struct drm_crtc *crtc,
				      struct drm_crtc_state *state)
{
	size_t lut_size;

	if (!state->color_mgmt_changed || !state->gamma_lut)
		return 0;

	if (crtc->state->gamma_lut &&
	    (crtc->state->gamma_lut->base.id == state->gamma_lut->base.id))
		return 0;

	if (state->gamma_lut->length % sizeof(struct drm_color_lut))
		return -EINVAL;

	lut_size = state->gamma_lut->length / sizeof(struct drm_color_lut);
	if (lut_size != DISP_GAMMA_SIZE)
		return -EINVAL;

	return 0;
}

static int ax_crtc_atomic_check(struct drm_crtc *crtc,
				struct drm_crtc_state *crtc_state)
{
	struct ax_crtc *ax_crtc = to_ax_crtc(crtc);
	struct ax_composer *composer = &ax_crtc->composer;
	struct drm_plane *plane;
	struct drm_plane_state *plane_state;
	int ret = 0;

	composer->num_active_planes = 0;
	drm_for_each_plane_mask(plane, crtc->dev, crtc_state->plane_mask) {
		plane_state = drm_atomic_get_new_plane_state(crtc_state->state, plane);
		if (plane_state && plane_state->visible) {
			composer->active_planes[composer->num_active_planes] = plane_state;
			composer->num_active_planes += 1;
		}
	}

	DRM_DEBUG("crtc%d %s %s, active_planes:%d\n", ax_crtc->id, crtc_mode[ax_crtc->work_mode],
		  crtc_stat[ax_crtc->status],
		  composer->num_active_planes);

	ret = ax_crtc_atomic_check_gamma(crtc, crtc_state);

	return ret;
}

static const struct drm_crtc_helper_funcs ax_crtc_helper_funcs = {
	.atomic_check = ax_crtc_atomic_check,
	.atomic_begin = ax_crtc_atomic_begin,
	.atomic_flush = ax_crtc_atomic_flush,
	.atomic_enable = ax_crtc_atomic_enable,
	.atomic_disable = ax_crtc_atomic_disable,
};

static int ax_crtc_create(struct device *dev, u32 id, struct ax_drm_mgr *drm_mgr)
{
	struct ax_crtc *ax_crtc;
	struct drm_crtc *crtc;
	struct platform_device *pdev = to_platform_device(dev);
	struct drm_device *drm_dev = drm_mgr->drm_dev;
	int ret;

	DRM_INFO("create crtc, id = %d\n", id);

	if (id >= AX_CRTC_MAX) {
		DRM_ERROR("vo id invalid, id = %d\n", id);
		return -EINVAL;
	}

	ax_crtc = devm_kzalloc(drm_dev->dev, sizeof(*ax_crtc), GFP_KERNEL);
	if (!ax_crtc)
		return -ENOMEM;

	ax_crtc->id = id;
	ax_crtc->drm_mgr = drm_mgr;

	/* Each crtc only supports two planes, one is video plane and the other is graph plane */
	ax_crtc->num_planes = AX_PLANES_MAX;
	crtc = &ax_crtc->base;

	crtc->port = of_get_next_child(dev->of_node, NULL);
	of_node_put(dev->of_node);
	if (IS_ERR(crtc->port)) {
		DRM_ERROR("crtc port not found, of_node: %pOF\n", dev->of_node);
		ret = PTR_ERR(crtc->port);
		return ret;
	}

	drm_mgr->crtc[id] = crtc;
	ax_crtc->display_dev = &drm_mgr->dp_mgr->display_dev[id];

	ret = ax_plane_create(crtc);
	if (ret) {
		DRM_ERROR("failed to create plane\n");
		return ret;
	}

	ret = drm_crtc_init_with_planes(drm_dev, crtc, ax_crtc->primary_plane, NULL, &ax_crtc_funcs, "ax_crtc");
	if (ret) {
		DRM_ERROR("failed to init CRTC\n");
		return ret;
	}

	ax_crtc->props.id = drm_property_create_range(drm_dev, DRM_MODE_PROP_ATOMIC, "DEV_ID", 0, AX_CRTC_MAX);
	if (ret) {
		DRM_ERROR("failed to create dsi_sel property\n");
		return ret;
	}

	drm_object_attach_property(&crtc->base, ax_crtc->props.id, id);

	ax_crtc->props.work_mode = drm_property_create_range(drm_dev, DRM_MODE_PROP_ATOMIC, "WORK_MODE", AX_DISP_MODE_OFFLINE, AX_DISP_MODE_BUTT);
	if (ret) {
		DRM_ERROR("failed to create work mode property\n");
		return ret;
	}

	drm_object_attach_property(&crtc->base, ax_crtc->props.work_mode, AX_DISP_MODE_OFFLINE);

	ax_crtc->props.brightness = drm_property_create_range(drm_dev, 0, "BRIGHTNESS", 0, DISP_USER_BRIGHTNESS_MAX);
	if (!ax_crtc->props.brightness)
		return -ENOMEM;

	ax_crtc->props.contrast = drm_property_create_range(drm_dev, 0, "CONTRAST", 0, DISP_USER_CONTRAST_MAX);
	if (!ax_crtc->props.contrast)
		return -ENOMEM;

	ax_crtc->props.saturation = drm_property_create_range(drm_dev, 0, "SATURATION", 0, DISP_USER_SATURATION_MAX);
	if (!ax_crtc->props.saturation)
		return -ENOMEM;

	ax_crtc->props.hue = drm_property_create_range(drm_dev, 0, "HUE", 0, DISP_USER_HUE_MAX);
	if (!ax_crtc->props.hue)
		return -ENOMEM;

	ax_crtc->props.csc = drm_property_create_range(drm_dev, 0, "CSC", 0, AX_VO_CSC_MATRIX_BUTT - 1);
	if (!ax_crtc->props.csc)
		return -ENOMEM;

	drm_object_attach_property(&crtc->base, ax_crtc->props.brightness, 0);
	drm_object_attach_property(&crtc->base, ax_crtc->props.contrast, 0);
	drm_object_attach_property(&crtc->base, ax_crtc->props.saturation, 0);
	drm_object_attach_property(&crtc->base, ax_crtc->props.hue, 0);
	drm_object_attach_property(&crtc->base, ax_crtc->props.csc, 0);

	drm_mode_crtc_set_gamma_size(crtc, DISP_GAMMA_SIZE);
	/* No inverse-gamma, No ctm. */
	drm_crtc_enable_color_mgmt(crtc, 0, false, DISP_GAMMA_SIZE);

	drm_crtc_helper_add(crtc, &ax_crtc_helper_funcs);

	ax_crtc->status = CRTC_STATUS_INIT;

	platform_set_drvdata(pdev, ax_crtc);

	return 0;
}

static int ax_crtc_bind(struct device *dev, struct device *master, void *data)
{
	int ret;
	u32 id;
	struct drm_device *drm_dev = (struct drm_device *)data;
	struct ax_drm_mgr *ax_drm_mgr = drm_dev->dev_private;

	DRM_INFO("crtc bind\n");

	if (!dev->of_node) {
		DRM_ERROR("of_node is null, dev: %px\n", dev);
		ret = -ENODEV;
		goto err0;
	}

	ret = of_property_read_u32(dev->of_node, "id", &id);
	if (ret) {
		DRM_ERROR("vo id not defined\n");
		goto err0;
	}

	ret = ax_crtc_create(dev, id, ax_drm_mgr);
	if (ret) {
		DRM_ERROR("failed to create crtc, ret = %d\n", ret);
		goto err0;
	}

	return 0;

err0:
	DRM_ERROR("crtc bind failed, ret = %d\n", ret);

	return ret;
}

static void ax_crtc_unbind(struct device *dev, struct device *master, void *data)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct ax_crtc *ax_crtc = platform_get_drvdata(pdev);

	DRM_INFO("crtc%d unbind\n", ax_crtc->id);
}

const struct component_ops crtc_component_ops = {
	.bind = ax_crtc_bind,
	.unbind = ax_crtc_unbind,
};

static int ax_crtc_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;

	if (!dev->of_node) {
		DRM_ERROR("can't find crtc device\n");
		return -ENODEV;
	}

	DRM_INFO("crtc probe, of_node: %pOF\n", pdev->dev.of_node);

	return component_add(dev, &crtc_component_ops);
}

static int ax_crtc_remove(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;

	DRM_INFO("crtc remove\n");

	component_del(dev, &crtc_component_ops);

	return 0;
}

static const struct of_device_id crtc_drm_dt_ids[] = {
	{
	 .compatible = "axera,crtc",
	 },
	{},
};

MODULE_DEVICE_TABLE(of, crtc_drm_dt_ids);

struct platform_driver crtc_platform_driver = {
	.probe = ax_crtc_probe,
	.remove = ax_crtc_remove,
	.driver = {
		   .name = "ax-crtc-drv",
		   .of_match_table = of_match_ptr(crtc_drm_dt_ids),
		   },
};

static int __init reserved_mem_boot_logo(struct reserved_mem *rmem)
{
	u32 devid, len;
	const __be32 *prop;

	if (rmem == NULL)
		return -EINVAL;

	prop = of_get_flat_dt_prop(rmem->fdt_node, "id", &len);
	if (!prop)
		return -ENOENT;

	if (len != sizeof(__be32)) {
		pr_err("invalid devid property in boot_logo_reserved node\n");
		return -EINVAL;
	}

	devid = be32_to_cpu(*prop);
	if (devid >= AX_DISPLAY_MAX) {
		pr_err("invalid devid(%d)\n", devid);
		return -EINVAL;
	}

	boot_logo_mem[devid].id = devid;
	boot_logo_mem[devid].base = rmem->base;
	boot_logo_mem[devid].size = rmem->size;

	pr_info("display%d boot logo memory at 0x%px, size is 0x%px\n",
		devid, (void *)rmem->base, (void *)rmem->size);

	return 0;
}

RESERVEDMEM_OF_DECLARE(boot_logo_reserved, "boot_logo", reserved_mem_boot_logo);

MODULE_DESCRIPTION("axera dpu driver");
MODULE_LICENSE("GPL v2");
