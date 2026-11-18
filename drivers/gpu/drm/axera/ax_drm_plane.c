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

#include <drm/drm_print.h>
#include <drm/drm_atomic.h>
#include <drm/drm_atomic_helper.h>
#include <drm/drm_plane_helper.h>
#include <drm/drm_gem_framebuffer_helper.h>
#include <drm/drm_modeset_helper_vtables.h>
#include <drm/drm_gem_cma_helper.h>
#include <drm/drm_fb_cma_helper.h>
#include <drm/drm_rect.h>

#include "ax_drm_crtc.h"
#include "ax_drm_plane.h"
#include "ax_drm_gem.h"

#define AX_FB_GET_COLOERKEY_VAL_LOW(val)	((val) & 0x3FFFFFFF)
#define AX_FB_GET_COLOERKEY_VAL_HIGH(val)	(((val) >> 30) & 0x3FFFFFFF)
#define AX_FB_GET_COLOERKEY_EN(val)		(((val) >> 60) & 0x1)
#define AX_FB_GET_COLOERKEY_INV(val)		(((val) >> 61) & 0X1)

static int ax_plane_atomic_check(struct drm_plane *plane,
				 struct drm_plane_state *plane_state)
{
	struct drm_crtc_state *crtc_state;
	int ret;

	if (!plane_state->crtc)
		return 0;

	crtc_state = drm_atomic_get_crtc_state(plane_state->state, plane_state->crtc);
	if (IS_ERR(crtc_state))
		return PTR_ERR(crtc_state);

	DRM_DEBUG_DRIVER("enter, [plane:%d:%s]\n", plane->base.id, plane->name);

	ret = drm_atomic_helper_check_plane_state(plane_state,
						crtc_state,
						DRM_PLANE_HELPER_NO_SCALING,
						DRM_PLANE_HELPER_NO_SCALING,
						true, true);

	return ret;
}

static u32 drm_fmt2display_fmt(u32 drm_format)
{
	u32 format = AX_VO_FORMAT_BUT;

	DRM_DEBUG_DRIVER("drm_format = 0x%x\n", drm_format);

	switch (drm_format) {
	case DRM_FORMAT_NV12:
		format = AX_VO_FORMAT_NV12;
		break;
	case DRM_FORMAT_ARGB1555:
		format = AX_VO_FORMAT_ARGB1555;
		break;
	case DRM_FORMAT_ARGB4444:
		format = AX_VO_FORMAT_ARGB4444;
		break;
	case DRM_FORMAT_RGB565_A8:
		format = AX_VO_FORMAT_RGBA5658;
		break;
	case DRM_FORMAT_ARGB8888:
		format = AX_VO_FORMAT_ARGB8888;
		break;
	case DRM_FORMAT_RGB565:
		format = AX_VO_FORMAT_RGB565;
		break;
	case DRM_FORMAT_RGB888:
		format = AX_VO_FORMAT_RGB888;
		break;
	case DRM_FORMAT_RGBA4444:
		format = AX_VO_FORMAT_RGBA4444;
		break;
	case DRM_FORMAT_RGBA5551:
		format = AX_VO_FORMAT_RGBA5551;
		break;
	case DRM_FORMAT_RGBA8888:
		format = AX_VO_FORMAT_RGBA8888;
		break;
	default:
		DRM_ERROR("unsupported format\n");
	}

	return format;
}

static void ax_plane_atomic_update(struct drm_plane *plane,
				   struct drm_plane_state *old_state)
{
	u32 drm_fmt;
	struct ax_plane *ax_plane = to_ax_plane(plane);
	struct drm_plane_state *new_state = plane->state;
	struct drm_framebuffer *fb = new_state->fb;
	struct drm_gem_object *gobj;
	struct ax_gem_object *ax_gobj;

	DRM_DEBUG_DRIVER("[plane:%d:%s:%d]\n", plane->base.id, plane->name, plane->type);

	DRM_DEBUG_DRIVER("width = %d, height = %d, pitches[0, 1] = [%d, %d]\n",
			 fb->width, fb->height, fb->pitches[0], fb->pitches[1]);
	DRM_DEBUG_DRIVER("src_x = %d, src_y = %d, src_w = %d, src_h = %d\n",
			 new_state->src_x, new_state->src_y, new_state->src_w,
			 new_state->src_h);
	DRM_DEBUG_DRIVER("crtc_x = %d, crtc_y = %d, crtc_w = %d, crtc_h = %d\n",
			 new_state->crtc_x, new_state->crtc_y,
			 new_state->crtc_w, new_state->crtc_h);

	if (plane->type == DRM_PLANE_TYPE_PRIMARY) {
		AX_FB_SET_PLANE_TYPE(ax_plane->fb.type, AX_DISP_FB_TYPE_PRIMARY);
	} else if (plane->type == DRM_PLANE_TYPE_OVERLAY) {
		AX_FB_SET_PLANE_TYPE(ax_plane->fb.type, AX_DISP_FB_TYPE_OVERLAY);
	} else if (plane->type == DRM_PLANE_TYPE_CURSOR) {
		AX_FB_SET_PLANE_TYPE(ax_plane->fb.type, AX_DISP_FB_TYPE_CURSOR);
	}

	drm_fmt = fb->format->format;

	ax_plane->fb.src_x = new_state->src_x >> 16;
	ax_plane->fb.src_y = new_state->src_y >> 16;
	ax_plane->fb.src_w = new_state->src_w >> 16;
	ax_plane->fb.src_h = new_state->src_h >> 16;
	ax_plane->fb.dst_x = new_state->crtc_x;
	ax_plane->fb.dst_y = new_state->crtc_y;
	ax_plane->fb.dst_w = drm_rect_width(&new_state->dst);
	ax_plane->fb.dst_h = drm_rect_height(&new_state->dst);
	ax_plane->fb.format = drm_fmt2display_fmt(drm_fmt);
	ax_plane->fb.fb_w = fb->width;
	ax_plane->fb.fb_h = fb->height;
	ax_plane->fb.stride_y = ALIGN_DOWN(fb->pitches[0], 16);
	if (drm_fmt == DRM_FORMAT_NV12)
		ax_plane->fb.stride_c = ALIGN_DOWN(fb->pitches[1], 16);

	if (!ax_plane->fb.phy_addr_y) {
		gobj = drm_gem_fb_get_obj(fb, 0);
		ax_gobj = to_ax_obj(gobj);
		ax_plane->fb.phy_addr_y = ax_gobj->dma_addr;
		if (drm_fmt == DRM_FORMAT_NV12)
			ax_plane->fb.phy_addr_c = ax_plane->fb.phy_addr_y +
			    ax_plane->fb.stride_y * ax_plane->fb.fb_h;
	}

	DRM_DEBUG_DRIVER("type:%d, fmt:0x%x, stride: {%d, %d}, pddr: {0x%llx, 0x%llx}\n",
	     ax_plane->fb.type, ax_plane->fb.format, ax_plane->fb.stride_y, ax_plane->fb.stride_c,
	     ax_plane->fb.phy_addr_y, ax_plane->fb.phy_addr_c);
}

static void ax_plane_atomic_disable(struct drm_plane *plane,
				    struct drm_plane_state *old_state)
{
	struct ax_plane *ax_plane = to_ax_plane(plane);

	DRM_DEBUG_DRIVER("enter, [plane:%d:%s]\n", plane->base.id, plane->name);
	memset(&ax_plane->fb, 0, sizeof(ax_plane->fb));
}

static int ax_plane_set_property(struct drm_plane *plane,
				 struct drm_plane_state *state,
				 struct drm_property *property, uint64_t val)
{
	struct ax_plane *ax_plane = to_ax_plane(plane);

	DRM_DEBUG_DRIVER("[plane:%d:%s %s:%llx]\n", plane->base.id, plane->name,
			 property->name, val);

	if (property == ax_plane->props.blk_id_y)
		ax_plane->fb.blk_id_y = val;
	else if (property == ax_plane->props.blk_id_c)
		ax_plane->fb.blk_id_c = val;
	else if (property == ax_plane->props.phy_addr_y)
		ax_plane->fb.phy_addr_y = val;
	else if (property == ax_plane->props.phy_addr_c)
		ax_plane->fb.phy_addr_c = val;
	else if (property == ax_plane->props.crtc_x_offs)
		ax_plane->fb.dst_x = val;
	else if (property == ax_plane->props.layer_id)
		AX_FB_SET_LAYER_ID(ax_plane->fb.type, val);
	else if (property == ax_plane->props.active)
		ax_plane->fb.mouse_show = val;
	else if (property == ax_plane->props.colorkey) {
		ax_plane->fb.colorkey_en = AX_FB_GET_COLOERKEY_EN(val);
		ax_plane->fb.colorkey_inv = AX_FB_GET_COLOERKEY_INV(val);
		ax_plane->fb.colorkey_val_low = AX_FB_GET_COLOERKEY_VAL_LOW(val);
		ax_plane->fb.colorkey_val_high = AX_FB_GET_COLOERKEY_VAL_HIGH(val);
	}
	return 0;
}

static int ax_plane_get_property(struct drm_plane *plane,
				 const struct drm_plane_state *state,
				 struct drm_property *property, uint64_t * val)
{
	int i;

	for (i = 0; i < plane->base.properties->count; i++) {
		if (property == plane->base.properties->properties[i]) {
			*val = plane->base.properties->values[i];
			return 0;
		}
	}

	return -EINVAL;
}

static const struct drm_plane_helper_funcs ax_plane_helper_funcs = {
	.atomic_check = ax_plane_atomic_check,
	.atomic_disable = ax_plane_atomic_disable,
	.atomic_update = ax_plane_atomic_update,
};

static const uint64_t format_modifiers[] = {
	DRM_FORMAT_MOD_LINEAR,
	DRM_FORMAT_MOD_INVALID,
};

static bool ax_plane_format_mod_supported(struct drm_plane *plane, u32 format, u64 modifier)
{
	if (modifier == DRM_FORMAT_MOD_LINEAR)
		return true;

	return false;
}

static const struct drm_plane_funcs ax_plane_funcs = {
	.update_plane = drm_atomic_helper_update_plane,
	.disable_plane = drm_atomic_helper_disable_plane,
	.atomic_set_property = ax_plane_set_property,
	.atomic_get_property = ax_plane_get_property,
	.destroy = drm_plane_cleanup,
	.reset = drm_atomic_helper_plane_reset,
	.atomic_duplicate_state = drm_atomic_helper_plane_duplicate_state,
	.atomic_destroy_state = drm_atomic_helper_plane_destroy_state,
	.format_mod_supported = ax_plane_format_mod_supported,
};

static u32 vl_supported_drm_formats[] = {
	DRM_FORMAT_NV12,
	DRM_FORMAT_RGB888,
	DRM_FORMAT_RGB565,
};

static u32 gl_supported_drm_formats[] = {
	DRM_FORMAT_ARGB1555,
	DRM_FORMAT_ARGB4444,
	DRM_FORMAT_RGB565_A8,
	DRM_FORMAT_ARGB8888,
	DRM_FORMAT_RGB565,
	DRM_FORMAT_RGB888,
	DRM_FORMAT_RGBA4444,
	DRM_FORMAT_RGBA5551,
	DRM_FORMAT_RGBA8888,
};

int ax_plane_create(struct drm_crtc *crtc)
{
	int i, num, type, arr_size;
	u32 *formats;
	char *name;
	struct ax_crtc *ax_crtc = to_ax_crtc(crtc);
	struct ax_plane *ax_plane;
	struct drm_plane *plane;
	struct drm_device *drm_dev = ax_crtc->drm_mgr->drm_dev;

	num = ax_crtc->num_planes;
	ax_plane = devm_kzalloc(drm_dev->dev, sizeof(*ax_plane) * num,
				GFP_KERNEL);
	if (!ax_plane)
		return -ENOMEM;

	for (i = 0; i < num; i++, ax_plane++) {
		plane = &ax_plane->base;

		if (i % 3 == 1) {
			type = DRM_PLANE_TYPE_OVERLAY;
			formats = gl_supported_drm_formats;
			arr_size = ARRAY_SIZE(gl_supported_drm_formats);
			name = "ax_overlay_plane";
		} else if (i % 3 == 0) {
			type = DRM_PLANE_TYPE_PRIMARY;
			formats = vl_supported_drm_formats;
			arr_size = ARRAY_SIZE(vl_supported_drm_formats);
			name = "ax_primary_plane";
		} else {
			type = DRM_PLANE_TYPE_CURSOR;
			formats = gl_supported_drm_formats;
			arr_size = ARRAY_SIZE(gl_supported_drm_formats);
			name = "ax_cursor_plane";
		}
		drm_universal_plane_init(drm_dev, plane, 0xFF, &ax_plane_funcs,
					 formats, arr_size, format_modifiers, type, name);

		drm_plane_helper_add(plane, &ax_plane_helper_funcs);

		plane->possible_crtcs = 0xff;

		if (type == DRM_PLANE_TYPE_PRIMARY) {
			if (!ax_crtc->primary_plane)
				ax_crtc->primary_plane = plane;
		} else if (type == DRM_PLANE_TYPE_OVERLAY) {
			ax_plane->props.colorkey = drm_property_create_range(drm_dev, DRM_MODE_PROP_ATOMIC, "COLORKEY", 0 , ~(u64)0);
			drm_object_attach_property(&plane->base, ax_plane->props.colorkey, 0);
		}

		ax_plane->props.crtc_x_offs = drm_property_create_range(drm_dev, DRM_MODE_PROP_ATOMIC, "CRTC_X_OFFS", 0, (u32)2048);
		drm_object_attach_property(&plane->base, ax_plane->props.crtc_x_offs, (u32)0);
		ax_plane->props.blk_id_y = drm_property_create_range(drm_dev, DRM_MODE_PROP_ATOMIC, "BLK_ID_Y", 0, ~(u32)0);
		drm_object_attach_property(&plane->base, ax_plane->props.blk_id_y, ~(u32)0);
		ax_plane->props.blk_id_c = drm_property_create_range(drm_dev, DRM_MODE_PROP_ATOMIC, "BLK_ID_C", 0, ~(u32)0);
		drm_object_attach_property(&plane->base, ax_plane->props.blk_id_c, ~(u32)0);
		ax_plane->props.phy_addr_y = drm_property_create_range(drm_dev, DRM_MODE_PROP_ATOMIC, "PHY_ADDR_Y", 0, ~(u64)0);
		drm_object_attach_property(&plane->base, ax_plane->props.phy_addr_y, 0);
		ax_plane->props.phy_addr_c = drm_property_create_range(drm_dev, DRM_MODE_PROP_ATOMIC, "PHY_ADDR_C", 0, ~(u64)0);
		drm_object_attach_property(&plane->base, ax_plane->props.phy_addr_c, 0);
		ax_plane->props.layer_id = drm_property_create_range(drm_dev, DRM_MODE_PROP_ATOMIC, "LAYER_ID", 0, ~(u32)0);
		drm_object_attach_property(&plane->base, ax_plane->props.layer_id, 0);
		ax_plane->props.active = drm_property_create_bool(drm_dev, DRM_MODE_PROP_ATOMIC, "PLANE_ACTIVE");
		drm_object_attach_property(&plane->base, ax_plane->props.active, 0);
	}

	return 0;
}
