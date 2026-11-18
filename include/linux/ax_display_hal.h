/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/

#ifndef __AX_DISPLAY_HAL_H
#define __AX_DISPLAY_HAL_H

#include <linux/types.h>

#define AX_DISPLAY_MAX	5

#define MODE_FLAG_INTERLACE			(1 << 0)	/* 1: interlace */
#define MODE_FLAG_SYNC_TYPE			(1 << 1)	/* 0: internal sync. 1: external sync */
#define MODE_FLAG_FIELD1_FIRST			(1 << 2)
#define MODE_FLAG_BT_8BIT_LOW			(1 << 3)

#define AX_FB_PLANE_TYPE_MASK			(0xFFFF)
#define AX_FB_LAYER_ID_MASK			(0xFFFF << 16)
#define AX_FB_SET_PLANE_TYPE(val, type)		((val) = (((val) & (~AX_FB_PLANE_TYPE_MASK)) | ((type) & AX_FB_PLANE_TYPE_MASK)))
#define AX_FB_GET_PLANE_TYPE(val)		((val) & AX_FB_PLANE_TYPE_MASK)
#define AX_FB_SET_LAYER_ID(val, layer_id)	((val) = (((val) & (~AX_FB_LAYER_ID_MASK)) | (((layer_id) + 1) << 16)))
#define AX_FB_GET_LAYER_ID(val)			(((val) >> 16) - 1)

#ifdef CONFIG_DRM
#define CONFIG_DRM_VO
#endif

enum {
	AX_DISP_RUNNING = 0x0,
	AX_DISP_SLEEP = 0xA55A,
};

enum {
	AX_DISP_OUT_MODE_BT601 = 0,
	AX_DISP_OUT_MODE_BT656,
	AX_DISP_OUT_MODE_BT1120,
	AX_DISP_OUT_MODE_DPI,
	AX_DISP_OUT_MODE_DSI_DPI_VIDEO,
	AX_DISP_OUT_MODE_DSI_SDI_VIDEO,
	AX_DISP_OUT_MODE_DSI_SDI_CMD,
	AX_DISP_OUT_MODE_LVDS,
	AX_DISP_OUT_MODE_BUT,
};

enum {
	AX_DISP_OUT_FMT_RGB565 = 0,
	AX_DISP_OUT_FMT_RGB666,
	AX_DISP_OUT_FMT_RGB666LP,
	AX_DISP_OUT_FMT_RGB888,
	AX_DISP_OUT_FMT_YUV422,
};

enum {
	AX_VO_FORMAT_NV12,
	AX_VO_FORMAT_NV21,
	AX_VO_FORMAT_ARGB1555,
	AX_VO_FORMAT_ARGB4444,
	AX_VO_FORMAT_RGBA5658,
	AX_VO_FORMAT_ARGB8888,
	AX_VO_FORMAT_RGB565,
	AX_VO_FORMAT_RGB888,
	AX_VO_FORMAT_BGR565,
	AX_VO_FORMAT_BGR888,
	AX_VO_FORMAT_RGBA4444,
	AX_VO_FORMAT_RGBA5551,
	AX_VO_FORMAT_RGBA8888,
	AX_VO_FORMAT_ARGB8565,
	AX_VO_FORMAT_NV16,
	AX_VO_FORMAT_BITMAP,
	AX_VO_FORMAT_BUT,
};

enum {
    AX_VO_CSC_MATRIX_IDENTITY = 0,
    AX_VO_CSC_MATRIX_BT601_TO_BT601,
    AX_VO_CSC_MATRIX_BT601_TO_BT709,
    AX_VO_CSC_MATRIX_BT709_TO_BT709,
    AX_VO_CSC_MATRIX_BT709_TO_BT601,
    AX_VO_CSC_MATRIX_BT601_TO_RGB_PC,
    AX_VO_CSC_MATRIX_BT709_TO_RGB_PC,
    AX_VO_CSC_MATRIX_RGB_TO_BT601_PC,
    AX_VO_CSC_MATRIX_RGB_TO_BT709_PC,
    AX_VO_CSC_MATRIX_RGB_TO_BT2020_PC,
    AX_VO_CSC_MATRIX_BT2020_TO_RGB_PC,
    AX_VO_CSC_MATRIX_RGB_TO_BT601_TV,
    AX_VO_CSC_MATRIX_RGB_TO_BT709_TV,
    AX_VO_CSC_MATRIX_BUTT
};

enum AX_DISP_PROPERTY_E {
	AX_DISP_PROPERTY_BRIGHTNESS = 0,
	AX_DISP_PROPERTY_CONTRAST,
	AX_DISP_PROPERTY_SATURATION,
	AX_DISP_PROPERTY_HSV_LUT,
	AX_DISP_PROPERTY_HUE,
	AX_DISP_PROPERTY_CSC,
	AX_DISP_PROPERTY_BUT,
};

enum {
	AX_DISP_FB_TYPE_PRIMARY,
	AX_DISP_FB_TYPE_OVERLAY,
	AX_DISP_FB_TYPE_CURSOR,
};

enum {
	AX_DISP_MODE_OFFLINE,
	AX_DISP_MODE_ONLINE,
	AX_DISP_MODE_BUTT,
};

enum {
	AX_DISP_CTRL_TYPE_NORMAL,
	AX_DISP_CTRL_TYPE_LOW_LATENCY,
	AX_DISP_CTRL_TYPE_BUTT,
};

struct ax_fb {
	u32 type;

	u32 reso_w;
	u32 reso_h;

	u32 src_x;
	u32 src_y;
	u32 src_w;
	u32 src_h;

	u32 dst_x;
	u32 dst_y;
	u32 dst_w;
	u32 dst_h;

	u32 format;

	u32 fb_w;
	u32 fb_h;

	u32 mouse_show;
	u32 mouse_alpha;
	u32 mouse_pixel;
	u32 mouse_inv_en;
	u32 mouse_inv_pixel;
	u32 mouse_inv_thr;

	u16 colorkey_en;
	u16 colorkey_inv;
	u32 colorkey_val_low;
	u32 colorkey_val_high;

	u32 stride_y;
	u32 stride_c;

	u32 blk_id_y;
	u32 blk_id_c;

	u64 phy_addr_y;
	u64 phy_addr_c;
};

#define DISP_GAMMA_SIZE			(33 * 33)
#define DISP_USER_BRIGHTNESS_MAX	(100)
#define DISP_USER_CONTRAST_MAX		(100)
#define DISP_USER_SATURATION_MAX	(100)
#define DISP_USER_HUE_MAX		(100)

#define DISP_PROP_FLAG_BRIGHTNESS	(1 << 0)
#define DISP_PROP_FLAG_CONTRAST		(1 << 1)
#define DISP_PROP_FLAG_SATURATION	(1 << 2)
#define DISP_PROP_FLAG_HSV_LUT		(1 << 3)
#define DISP_PROP_FLAG_HUE		(1 << 4)
#define DISP_PROP_FLAG_CSC		(1 << 5)

struct ax_disp_props {
	u8 flags;

	u8 brightness;
	u8 contrast;
	u8 saturation;
	u8 hue;
	u8 csc;
	u64 gamma_blob;
};


struct ax_composer_info {
	u32 reso_w;
	u32 reso_h;
	u32 fb_nr;
	struct ax_fb *fb;
	struct ax_disp_props props;
};

struct ax_disp_mode {
	int type;
	int fmt_in;
	int fmt_out;
	int flags;

	int clock;		/* in kHz */
	int vrefresh;
	int hdisplay;
	int hsync_start;
	int hsync_end;
	int htotal;
	int vdisplay;
	int vsync_start;
	int vsync_end;
	int vtotal;

	int hp_pol;
	int vp_pol;
	int de_pol;
};

typedef void (*dispc_irq_cb_t)(void *param);

struct ax_display_funcs {
	void (*dpu_reset)(void *priv_data);
	int (*dpu_mem_alloc)(void *priv_data, u64 *paddr, void **vaddr, u32 size, u32 align, char *token);
	int (*dpu_mem_free)(void *priv_data, u64 paddr, void *vaddr);
	int (*dpu_resume)(void *priv_data);
	int (*dpu_suspend)(void *priv_data);
	int (*dispc_config)(void *priv_data, u32 work_mode, struct ax_disp_mode *disp_mode);
	int (*dispc_set_props)(void *priv_data, struct ax_disp_props *props);
	int (*dispc_get_props)(void *priv_data, struct ax_disp_props *props);
	void (*dispc_enable)(void *priv_data, u8 type);
	void (*dispc_disable)(void *priv_data, u8 type);
	void (*dispc_int_mask)(void *priv_data);
	void (*dispc_int_unmask)(void *priv_data);
	void (*dispc_set_irq_callback)(void *priv_data, dispc_irq_cb_t irq_callback, void *param);
	//void (*dispc_set_buffer)(void *priv_data, u64 addr_y, u64 addr_uv, u32 stride_y, u32 stride_uv);
	void (*dispc_fb_commit)(void *priv_data, struct ax_composer_info *composer_info);
	void (*dispc_set_sleep_sta)(void *priv_data, const void *state);
	void (*dispc_check_mode)(void *priv_data, u8 type);
};

int ax_display_register(int index, struct ax_display_funcs *display_funs, void *data);
int ax_display_unregister(int index);
void ax_display_dpu_open(void);
void ax_display_dpu_close(void);
int ax_display_get_bootlogo_mode(void);
void ax_display_reset_bootlogo_mode(void);
#endif /* __AX_DISPLAY_HAL_H */
