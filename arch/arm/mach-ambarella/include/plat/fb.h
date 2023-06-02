/*
 * arch/arm/plat-ambarella/include/plat/fb.h
 *
 * Author: Anthony Ginger <hfjiang@ambarella.com>
 *
 * Copyright (C) 2004-2009, Ambarella, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 */

#ifndef __PLAT_AMBARELLA_FB_H
#define __PLAT_AMBARELLA_FB_H

#define AMBARELLA_FB_MAX_NUM			2

/* ==========================================================================*/
#define AMBARELLA_CLUT_TABLE_SIZE		(256 * 3)
#define AMBARELLA_BLEND_TABLE_SIZE		(256)

/* ==========================================================================*/
#ifndef __ASSEMBLER__
enum ambarella_fb_color_format {
	AMBFB_COLOR_AUTO = 0,

	AMBFB_COLOR_CLUT_8BPP,
	AMBFB_COLOR_RGB565,

	AMBFB_COLOR_BGR565,
	AMBFB_COLOR_AGBR4444,	//AYUV 4:4:4:4
	AMBFB_COLOR_RGBA4444,
	AMBFB_COLOR_BGRA4444,
	AMBFB_COLOR_ABGR4444,
	AMBFB_COLOR_ARGB4444,
	AMBFB_COLOR_AGBR1555,	//AYUV 1:5:5:5
	AMBFB_COLOR_GBR1555,	//YUV 1(ignored):5:5:5
	AMBFB_COLOR_RGBA5551,
	AMBFB_COLOR_BGRA5551,
	AMBFB_COLOR_ABGR1555,
	AMBFB_COLOR_ARGB1555,
	AMBFB_COLOR_AGBR8888,	//AYUV 8:8:8:8
	AMBFB_COLOR_AYUV8888,
	AMBFB_COLOR_RGBA8888,
	AMBFB_COLOR_BGRA8888,
	AMBFB_COLOR_ABGR8888,
	AMBFB_COLOR_ARGB8888,

	AMBFB_COLOR_VYU565,
	AMBFB_COLOR_AYUV4444,
	AMBFB_COLOR_AYUV1555,
	AMBFB_COLOR_YUV555,

	AMBFB_COLOR_UNSUPPORTED,  //Reserved only, not supported
};

enum ambarella_dsp_status {
	AMBA_DSP_ENCODE_MODE	= 0x00,
	AMBA_DSP_DECODE_MODE	= 0x01,
	AMBA_DSP_RESET_MODE	= 0x02,
	AMBA_DSP_UNKNOWN_MODE	= 0x03,
	AMBA_DSP_QUICKLOGO_MODE	= 0x04,
};

enum ambarella_fb_status {
	AMBFB_UNKNOWN_MODE	= 0x00,
	AMBFB_ACTIVE_MODE,
	AMBFB_STOP_MODE,
};

typedef int (*ambarella_fb_pan_display_fn)(struct fb_var_screeninfo *var,
	struct fb_info *info);
typedef int (*ambarella_fb_setcmap_fn)(struct fb_cmap *cmap,
	struct fb_info *info);
typedef int (*ambarella_fb_check_var_fn)(struct fb_var_screeninfo *var,
	struct fb_info *info);
typedef int (*ambarella_fb_set_par_fn)(struct fb_info *info);
typedef int (*ambarella_fb_blank_fn)(int blank_mode, struct fb_info *info);

struct ambarella_fb_cvs_buf {		//Conversion Buffer
	int				available;
	u32				base_buf_phy;
	u8				*ping_buf;
	u32				ping_buf_phy;
	u32				ping_buf_size;
	u8				*pong_buf;
	u32				pong_buf_size;
	u32				pong_buf_phy;
};

struct ambarella_fb_iav_info {
	struct fb_var_screeninfo	screen_var;
	struct fb_fix_screeninfo	screen_fix;
	enum ambarella_dsp_status	dsp_status;

	ambarella_fb_pan_display_fn	pan_display;
	ambarella_fb_setcmap_fn		setcmap;
	ambarella_fb_check_var_fn	check_var;
	ambarella_fb_set_par_fn		set_par;
	ambarella_fb_blank_fn		set_blank;
};

struct ambarella_platform_fb {
	struct mutex			lock;
	struct fb_var_screeninfo	screen_var;
	struct fb_fix_screeninfo	screen_fix;
	enum ambarella_dsp_status	dsp_status;
	enum ambarella_fb_status	fb_status;
	u8				clut_table[AMBARELLA_CLUT_TABLE_SIZE];
	u8				blend_table[AMBARELLA_BLEND_TABLE_SIZE];
	enum ambarella_fb_color_format	color_format;
	struct ambarella_fb_cvs_buf	conversion_buf;
	u32				use_prealloc;
	u32				prealloc_line_length;

	ambarella_fb_pan_display_fn	pan_display;
	ambarella_fb_setcmap_fn		setcmap;
	ambarella_fb_check_var_fn	check_var;
	ambarella_fb_set_par_fn		set_par;
	ambarella_fb_blank_fn		set_blank;

	struct fb_info			*proc_fb_info;
	struct proc_dir_entry		*proc_file;
	wait_queue_head_t		proc_wait;
	u32				proc_wait_flag;
};

/* ==========================================================================*/

extern struct ambarella_platform_fb *ambfb_data_ptr[];

/* ==========================================================================*/
extern int ambarella_fb_get_platform_info(u32, struct ambarella_platform_fb *);
extern int ambarella_fb_set_iav_info(u32, struct ambarella_fb_iav_info *);

#endif /* __ASSEMBLER__ */
/* ==========================================================================*/

#endif

