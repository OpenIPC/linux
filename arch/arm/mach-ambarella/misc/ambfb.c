/*
 * arch/arm/plat-ambarella/video/ambfb.c
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

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/bootmem.h>
#include <linux/delay.h>
#include <linux/irq.h>
#include <linux/dma-mapping.h>

#include <asm/page.h>
#include <asm/io.h>
#include <asm/setup.h>

#include <asm/mach/map.h>

#include <linux/fb.h>

#include <mach/hardware.h>
#include <plat/fb.h>

/* ==========================================================================*/
static struct ambarella_platform_fb ambarella_platform_fb0 = {
	.screen_var		= {
		.xres		= 720,
		.yres		= 480,
		.xres_virtual	= 720,
		.yres_virtual	= 480,
		.xoffset	= 0,
		.yoffset	= 0,
		.bits_per_pixel = 8,
		.red		= {.offset = 0, .length = 8, .msb_right = 0},
		.green		= {.offset = 0, .length = 8, .msb_right = 0},
		.blue		= {.offset = 0, .length = 8, .msb_right = 0},
		.activate	= FB_ACTIVATE_NOW,
		.height		= -1,
		.width		= -1,
		.pixclock	= 36101,
		.left_margin	= 24,
		.right_margin	= 96,
		.upper_margin	= 10,
		.lower_margin	= 32,
		.hsync_len	= 40,
		.vsync_len	= 3,
		.vmode		= FB_VMODE_NONINTERLACED,
	},
	.screen_fix		= {
		.id		= "Ambarella FB",
		.type		= FB_TYPE_PACKED_PIXELS,
		.visual		= FB_VISUAL_PSEUDOCOLOR,
		.xpanstep	= 1,
		.ypanstep	= 1,
		.ywrapstep	= 1,
		.accel		= FB_ACCEL_NONE,
		.line_length	= 0,
		.smem_start	= 0,
		.smem_len	= 0,
	},
	.dsp_status		= AMBA_DSP_UNKNOWN_MODE,
	.fb_status		= AMBFB_UNKNOWN_MODE,
	.color_format		= AMBFB_COLOR_CLUT_8BPP,
	.conversion_buf		= {
		.available	= 0,
		.ping_buf	= NULL,
		.ping_buf_size	= 0,
		.pong_buf	= NULL,
		.pong_buf_size	= 0,
		},
	.use_prealloc		= 0,
	.prealloc_line_length	= 0,

	.pan_display		= NULL,
	.setcmap		= NULL,
	.check_var		= NULL,
	.set_par		= NULL,
	.set_blank		= NULL,

	.proc_fb_info		= NULL,
	.proc_file		= NULL,
};

static struct ambarella_platform_fb ambarella_platform_fb1 = {
	.screen_var		= {
		.xres		= 720,
		.yres		= 480,
		.xres_virtual	= 720,
		.yres_virtual	= 480,
		.xoffset	= 0,
		.yoffset	= 0,
		.bits_per_pixel = 8,
		.red		= {.offset = 0, .length = 8, .msb_right = 0},
		.green		= {.offset = 0, .length = 8, .msb_right = 0},
		.blue		= {.offset = 0, .length = 8, .msb_right = 0},
		.activate	= FB_ACTIVATE_NOW,
		.height		= -1,
		.width		= -1,
		.pixclock	= 36101,
		.left_margin	= 24,
		.right_margin	= 96,
		.upper_margin	= 10,
		.lower_margin	= 32,
		.hsync_len	= 40,
		.vsync_len	= 3,
		.vmode		= FB_VMODE_NONINTERLACED,
	},
	.screen_fix		= {
		.id		= "Ambarella FB",
		.type		= FB_TYPE_PACKED_PIXELS,
		.visual		= FB_VISUAL_PSEUDOCOLOR,
		.xpanstep	= 1,
		.ypanstep	= 1,
		.ywrapstep	= 1,
		.accel		= FB_ACCEL_NONE,
		.line_length	= 0,
		.smem_start	= 0,
		.smem_len	= 0,
	},
	.dsp_status		= AMBA_DSP_UNKNOWN_MODE,
	.fb_status		= AMBFB_UNKNOWN_MODE,
	.color_format		= AMBFB_COLOR_CLUT_8BPP,
	.conversion_buf		= {
		.available	= 0,
		.ping_buf	= NULL,
		.ping_buf_size	= 0,
		.pong_buf	= NULL,
		.pong_buf_size	= 0,
		},
	.use_prealloc		= 0,
	.prealloc_line_length	= 0,

	.pan_display		= NULL,
	.setcmap		= NULL,
	.check_var		= NULL,
	.set_par		= NULL,
	.set_blank		= NULL,

	.proc_fb_info		= NULL,
	.proc_file		= NULL,
};

struct ambarella_platform_fb *ambfb_data_ptr[] = {
	&ambarella_platform_fb0,
	&ambarella_platform_fb1,
};
EXPORT_SYMBOL(ambfb_data_ptr);

int ambarella_fb_get_platform_info(u32 fb_id,
	struct ambarella_platform_fb *platform_info)
{
	struct ambarella_platform_fb *ambfb_data;

	if (fb_id > ARRAY_SIZE(ambfb_data_ptr))
		return -EPERM;

	ambfb_data = ambfb_data_ptr[fb_id];

	mutex_lock(&ambfb_data->lock);
	memcpy(platform_info, ambfb_data, sizeof(struct ambarella_platform_fb));
	mutex_unlock(&ambfb_data->lock);

	return 0;
}
EXPORT_SYMBOL(ambarella_fb_get_platform_info);

int ambarella_fb_set_iav_info(u32 fb_id, struct ambarella_fb_iav_info *iav)
{
	struct ambarella_platform_fb *ambfb_data;

	if (fb_id > ARRAY_SIZE(ambfb_data_ptr))
		return -EPERM;

	ambfb_data = ambfb_data_ptr[fb_id];

	mutex_lock(&ambfb_data->lock);
	ambfb_data->screen_var = iav->screen_var;
	ambfb_data->screen_fix = iav->screen_fix;
	ambfb_data->pan_display = iav->pan_display;
	ambfb_data->setcmap = iav->setcmap;
	ambfb_data->check_var = iav->check_var;
	ambfb_data->set_par = iav->set_par;
	ambfb_data->set_blank = iav->set_blank;
	ambfb_data->dsp_status = iav->dsp_status;
	mutex_unlock(&ambfb_data->lock);

	return 0;
}
EXPORT_SYMBOL(ambarella_fb_set_iav_info);

int __init ambarella_init_fb(void)
{
	struct ambarella_platform_fb *ambfb_data;
	int i;

	for (i = 0; i < ARRAY_SIZE(ambfb_data_ptr); i++) {
		ambfb_data = ambfb_data_ptr[i];
		mutex_init(&ambfb_data->lock);
		init_waitqueue_head(&ambfb_data->proc_wait);
	}

	return 0;
}

