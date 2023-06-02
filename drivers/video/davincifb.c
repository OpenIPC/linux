/*
 * Copyright (C) 2007 MontaVista Software Inc.
 * Copyright (C) 2006 Texas Instruments Inc
 *
 * Andy Lowe (alowe@mvista.com), MontaVista Software
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option)any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/string.h>
#include <linux/mm.h>
#include <linux/tty.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/fb.h>
#include <linux/init.h>
#include <linux/dma-mapping.h>
#include <asm/uaccess.h>
#include <linux/moduleparam.h>	/* for module_param() */
#include <linux/platform_device.h>
#include <asm/system.h>
#include <video/davinci_vpbe.h>
#include <video/davinci_osd.h>
#include <media/davinci/vid_encoder_types.h>
#include <media/davinci/davinci_enc.h>
#include <video/davincifb.h>
#include <video/davincifb_ioctl.h>
#include <mach/cputype.h>

struct davincifb_state {
	bool invert_field;
};

static struct davincifb_state fb_state;
static struct davincifb_state *fb = &fb_state;

static struct davincifb_platform_data davincifb_pdata_default = {
	.invert_field = false,
};

/* return non-zero if the info structure corresponds to OSD0 or OSD1 */
static int is_osd_win(const struct fb_info *info)
{
	const struct vpbe_dm_win_info *win = info->par;

	if (win->layer == WIN_OSD0 || win->layer == WIN_OSD1)
		return 1;
	else
		return 0;
}

/* return non-zero if the info structure corresponds to VID0 or VID1 */
#define is_vid_win(info) (!is_osd_win(info))

/*
 * Convert a framebuffer info pointer to a davinci_osd_layer enumeration.
 * It is up to the caller to verify that the info structure corresponds to
 * either OSD0 or OSD1.
 */
static enum davinci_osd_layer fb_info_to_osd_enum(const struct fb_info *info)
{
	const struct vpbe_dm_win_info *win = info->par;

	if (win->layer == WIN_OSD1)
		return OSDWIN_OSD1;
	else
		return OSDWIN_OSD0;
}

/* macros for testing fb_var_screeninfo attributes */
#define is_attribute_mode(var) (((var)->bits_per_pixel == 4) && \
	((var)->nonstd != 0))
#define is_yuv(var) ((((var)->bits_per_pixel == 16) || \
  ((var)->bits_per_pixel == 8)) \
  && ((var)->nonstd != 0))
#define is_window_interlaced(var) (((var)->vmode & FB_VMODE_INTERLACED) \
	== FB_VMODE_INTERLACED)

/* macros for testing fb_videomode attributes */
#define is_display_interlaced(mode) (((mode)->vmode & FB_VMODE_INTERLACED) \
	== FB_VMODE_INTERLACED)

/*
 * Convert an fb_var_screeninfo struct to a Davinci display layer configuration.
 * lconfig->xpos, lconfig->ypos, and lconfig->line_length are not modified
 * because no information about them is contained in var.
 * The value of the yc_pixfmt argument is returned in lconfig->pixfmt if a
 * the var specifies a YC pixel format.  The value of yc_pixfmt must be either
 * PIXFMT_YCbCrI or PIXFMT_YCrCbI.
 */
static void convert_fb_var_to_osd(const struct fb_var_screeninfo *var,
				  struct davinci_layer_config *lconfig,
				  enum davinci_pix_format yc_pixfmt)
{
	lconfig->xsize = var->xres;
	lconfig->ysize = var->yres;
	lconfig->interlaced = is_window_interlaced(var);

	switch (var->bits_per_pixel) {
	case 1:
		lconfig->pixfmt = PIXFMT_1BPP;
		break;
	case 2:
		lconfig->pixfmt = PIXFMT_2BPP;
		break;
	case 4:
		if (is_attribute_mode(var))
			lconfig->pixfmt = PIXFMT_OSD_ATTR;
		else
			lconfig->pixfmt = PIXFMT_4BPP;
		break;
	case 8:
		if (is_yuv(var))
			lconfig->pixfmt = PIXFMT_NV12;
		else
			lconfig->pixfmt = PIXFMT_8BPP;
		break;
	case 16:
	default:
		if (is_yuv(var))
			lconfig->pixfmt = yc_pixfmt;
		else
			lconfig->pixfmt = PIXFMT_RGB565;
		break;
	case 24:
	case 32:
		lconfig->pixfmt = PIXFMT_RGB888;
		break;
	}
}

/*
 * Convert an fb_info struct to a Davinci display layer configuration.
 */
static void convert_fb_info_to_osd(const struct fb_info *info,
				   struct davinci_layer_config *lconfig)
{
	const struct vpbe_dm_win_info *win = info->par;

	lconfig->line_length = info->fix.line_length;
	lconfig->xpos = win->xpos;
	lconfig->ypos = win->ypos;
	convert_fb_var_to_osd(&info->var, lconfig, win->dm->yc_pixfmt);
}

/*
 * Convert a Davinci display layer configuration to var info.
 * The following members of var are not modified:
 *	var->xres_virtual
 *	var->yres_virtual
 *	var->xoffset
 *	var->yoffset
 *	var->pixclock
 *	var->left_margin
 *	var->right_margin
 *	var->upper_margin
 *	var->lower_margin
 *	var->hsync_len
 *	var->vsync_len
 *	var->sync
 * Only bit 0 of var->vmode (FB_VMODE_INTERLACED) is modified.  All other bits
 * of var->vmode are retained.
 */
static void convert_osd_to_fb_var(const struct davinci_layer_config *lconfig,
				  struct fb_var_screeninfo *var)
{
	var->xres = lconfig->xsize;
	var->yres = lconfig->ysize;
	if (lconfig->interlaced)
		var->vmode |= FB_VMODE_INTERLACED;
	else
		var->vmode &= ~FB_VMODE_INTERLACED;

	var->red.offset = var->green.offset = var->blue.offset = 0;
	var->red.msb_right = var->green.msb_right = var->blue.msb_right = 0;
	var->transp.offset = var->transp.length = var->transp.msb_right = 0;
	var->nonstd = 0;

	switch (lconfig->pixfmt) {
	case PIXFMT_1BPP:
		var->bits_per_pixel = 1;
		var->red.length = var->green.length = var->blue.length =
		    var->bits_per_pixel;
		break;
	case PIXFMT_2BPP:
		var->bits_per_pixel = 2;
		var->red.length = var->green.length = var->blue.length =
		    var->bits_per_pixel;
		break;
	case PIXFMT_4BPP:
		var->bits_per_pixel = 4;
		var->red.length = var->green.length = var->blue.length =
		    var->bits_per_pixel;
		break;
	case PIXFMT_8BPP:
		var->bits_per_pixel = 8;
		var->red.length = var->green.length = var->blue.length =
		    var->bits_per_pixel;
		break;
	case PIXFMT_RGB565:
		var->bits_per_pixel = 16;
		var->red.offset = 11;
		var->red.length = 5;
		var->green.offset = 5;
		var->green.length = 6;
		var->blue.offset = 0;
		var->blue.length = 5;
		break;
	case PIXFMT_YCbCrI:
	case PIXFMT_YCrCbI:
		var->bits_per_pixel = 16;
		var->red.length = var->green.length = var->blue.length = 0;
		var->nonstd = 1;
		break;
	case PIXFMT_NV12:
		if (cpu_is_davinci_dm365()) {
			var->bits_per_pixel = 8;
			var->red.length = var->green.length = var->blue.length =
			    0;
			var->nonstd = 1;
		}
	case PIXFMT_RGB888:
		if (cpu_is_davinci_dm644x()) {
			var->bits_per_pixel = 24;
			var->red.offset = 0;
			var->red.length = 8;
			var->green.offset = 8;
			var->green.length = 8;
			var->blue.offset = 16;
			var->blue.length = 8;
		} else {
			var->bits_per_pixel = 32;
			var->red.offset = 16;
			var->red.length = 8;
			var->green.offset = 8;
			var->green.length = 8;
			var->blue.offset = 0;
			var->blue.length = 8;
			var->transp.offset = 24;
			var->transp.length = 3;
		}
		break;
	case PIXFMT_OSD_ATTR:
		var->bits_per_pixel = 4;
		var->red.length = var->green.length = var->blue.length = 0;
		var->nonstd = 1;
		break;
	}

	var->grayscale = 0;
	var->activate = FB_ACTIVATE_NOW;
	var->height = 0;
	var->width = 0;
	var->accel_flags = 0;
	var->rotate = 0;
}

/*
 * Get the video mode from the encoder manager.
 */
static int get_video_mode(struct fb_videomode *mode)
{
	struct vid_enc_mode_info mode_info;
	int ret;

	memset(&mode_info, 0, sizeof(mode_info));
	memset(mode, 0, sizeof(*mode));

	ret = davinci_enc_get_mode(0, &mode_info);

	mode->name = mode_info.name;
	if (mode_info.fps.denominator) {
		unsigned fps_1000;	/* frames per 1000 seconds */
		unsigned lps;	/* lines per second */
		unsigned pps;	/* pixels per second */
		unsigned vtotal;	/* total lines per frame */
		unsigned htotal;	/* total pixels per line */
		unsigned interlace = (mode_info.interlaced) ? 2 : 1;

		fps_1000 =
		    (1000 * mode_info.fps.numerator +
		     mode_info.fps.denominator / 2) / mode_info.fps.denominator;
		mode->refresh = (interlace * fps_1000 + 1000 / 2) / 1000;

		vtotal =
		    mode_info.yres + mode_info.lower_margin +
		    mode_info.vsync_len + mode_info.upper_margin;
		lps = (fps_1000 * vtotal + 1000 / 2) / 1000;

		htotal =
		    mode_info.xres + mode_info.right_margin +
		    mode_info.hsync_len + mode_info.left_margin;
		pps = lps * htotal;

		if (pps)
			mode->pixclock =
			    ((1000000000UL + pps / 2) / pps) * 1000;
	}
	mode->xres = mode_info.xres;
	mode->yres = mode_info.yres;
	mode->left_margin = mode_info.left_margin;
	mode->right_margin = mode_info.right_margin;
	mode->upper_margin = mode_info.upper_margin;
	mode->lower_margin = mode_info.lower_margin;
	mode->hsync_len = mode_info.hsync_len;
	mode->vsync_len = mode_info.vsync_len;
	if (mode_info.flags & (1 << 0))
		mode->sync |= FB_SYNC_HOR_HIGH_ACT;
	if (mode_info.flags & (1 << 1))
		mode->sync |= FB_SYNC_VERT_HIGH_ACT;
	if (mode_info.std)
		mode->sync |= FB_SYNC_BROADCAST;
	if (mode_info.interlaced)
		mode->vmode |= FB_VMODE_INTERLACED;

	return ret;
}

/*
 * Set a video mode with the encoder manager.
 */
static int set_video_mode(struct fb_videomode *mode)
{
	struct vid_enc_mode_info mode_info;
	int ret;

	davinci_enc_get_mode(0, &mode_info);

	mode_info.name = (unsigned char *)mode->name;
	mode_info.fps.numerator = 0;
	mode_info.fps.denominator = 0;
	if (mode->pixclock && mode->xres && mode->yres) {
		unsigned fps_1000;	/* frames per 1000 seconds */
		unsigned lps;	/* lines per second */
		unsigned pps;	/* pixels per second */
		unsigned vtotal;	/* total lines per frame */
		unsigned htotal;	/* total pixels per line */

		pps =
		    ((1000000000UL +
		      mode->pixclock / 2) / mode->pixclock) * 1000;

		htotal =
		    mode->xres + mode->right_margin + mode->hsync_len +
		    mode->left_margin;
		lps = (pps + htotal / 2) / htotal;

		vtotal =
		    mode->yres + mode->lower_margin + mode->vsync_len +
		    mode->upper_margin;
		fps_1000 = (lps * 1000 + vtotal / 2) / vtotal;

		mode_info.fps.numerator = fps_1000;
		mode_info.fps.denominator = 1000;

		/*
		 * 1000 == 2*2*2*5*5*5, so factor out any common multiples of 2
		 * or 5
		 */
		while ((((mode_info.fps.numerator / 2) * 2) ==
			mode_info.fps.numerator)
		       && (((mode_info.fps.denominator / 2) * 2) ==
			   mode_info.fps.denominator)) {
			mode_info.fps.numerator = mode_info.fps.numerator / 2;
			mode_info.fps.denominator =
			    mode_info.fps.denominator / 2;
		}
		while ((((mode_info.fps.numerator / 5) * 5) ==
			mode_info.fps.numerator)
		       && (((mode_info.fps.denominator / 5) * 5) ==
			   mode_info.fps.denominator)) {
			mode_info.fps.numerator = mode_info.fps.numerator / 5;
			mode_info.fps.denominator =
			    mode_info.fps.denominator / 5;
		}
	}
	mode_info.xres = mode->xres;
	mode_info.yres = mode->yres;
	mode_info.left_margin = mode->left_margin;
	mode_info.right_margin = mode->right_margin;
	mode_info.upper_margin = mode->upper_margin;
	mode_info.lower_margin = mode->lower_margin;
	mode_info.hsync_len = mode->hsync_len;
	mode_info.vsync_len = mode->vsync_len;
	if (mode->sync & FB_SYNC_HOR_HIGH_ACT)
		mode_info.flags |= (1 << 0);
	else
		mode_info.flags &= ~(1 << 0);
	if (mode->sync & FB_SYNC_VERT_HIGH_ACT)
		mode_info.flags |= (1 << 1);
	else
		mode_info.flags &= ~(1 << 1);
	if (mode->sync & FB_SYNC_BROADCAST)
		mode_info.std = 1;
	else
		mode_info.std = 0;
	if (mode->vmode & FB_VMODE_INTERLACED)
		mode_info.interlaced = 1;
	else
		mode_info.interlaced = 0;

	ret = davinci_enc_set_mode(0, &mode_info);

	return ret;
}

/*
 * Construct an fb_var_screeninfo structure from an fb_videomode structure
 * describing the display and a davinci_layer_config structure describing a window.
 * The following members of var not modified:
 *	var->xoffset
 *	var->yoffset
 *	var->xres_virtual
 *	var->yres_virtual
 * The following members of var are loaded with values derived from mode:
 *	var->pixclock
 *	var->left_margin
 *	var->hsync_len
 *	var->vsync_len
 *	var->right_margin
 *	var->upper_margin
 *	var->lower_margin
 *	var->sync
 *	var->vmode (all bits except bit 0: FB_VMODE_INTERLACED)
 * The following members of var are loaded with values derived from lconfig:
 *	var->xres
 *	var->yres
 *	var->bits_per_pixel
 *	var->red
 *	var->green
 *	var->blue
 *	var->transp
 *	var->nonstd
 *	var->grayscale
 *	var->activate
 *	var->height
 *	var->width
 *	var->accel_flags
 *	var->rotate
 *	var->vmode (only bit 0: FB_VMODE_INTERLACED)
 *
 * If the display resolution (xres and yres) specified in mode matches the
 * window resolution specified in lconfig, then the display timing info returned
 * in var is valid and var->pixclock will be the value derived from mode.
 * If the display resolution does not match the window resolution, then
 * var->pixclock will be set to 0 to indicate that the display timing info
 * returned in var is not valid.
 *
 * mode and lconfig are not modified.
 */
static void construct_fb_var(struct fb_var_screeninfo *var,
			     struct fb_videomode *mode,
			     struct davinci_layer_config *lconfig)
{
	fb_videomode_to_var(var, mode);
	convert_osd_to_fb_var(lconfig, var);
	if (lconfig->xsize != mode->xres || lconfig->ysize != mode->yres)
		var->pixclock = 0;
}

/*
 * Update the values in an fb_fix_screeninfo structure based on the values in an
 * fb_var_screeninfo structure.
 * The following members of fix are updated:
 *	fix->visual
 *	fix->xpanstep
 *	fix->ypanstep
 *	fix->ywrapstep
 *	fix->line_length
 * All other members of fix are unmodified.
 */
static void update_fix_info(const struct fb_var_screeninfo *var,
			    struct fb_fix_screeninfo *fix)
{
	fix->visual =
	    (var->bits_per_pixel >
	     8) ? FB_VISUAL_TRUECOLOR : FB_VISUAL_PSEUDOCOLOR;
	/*
	 * xpanstep must correspond to a multiple of the 32-byte cache line size
	 */
	switch (var->bits_per_pixel) {
	case 1:
	case 2:
	case 4:
	case 8:
	case 12:
	case 16:
	case 32:
		fix->xpanstep = (8 * 32) / var->bits_per_pixel;
		break;
	case 24:
		fix->xpanstep = 32;	/* 32 pixels = 3 cache lines */
		break;
	default:
		fix->xpanstep = 0;
		break;
	}
	fix->ypanstep = 1;
	fix->ywrapstep = 0;
	fix->line_length = (var->xres_virtual * var->bits_per_pixel + 7) / 8;
	/* line_length must be a multiple of the 32-byte cache line size */
	fix->line_length = ((fix->line_length + 31) / 32) * 32;
}

/*
 * Determine if the window configuration specified by var will fit in a
 * framebuffer of size fb_size.
 * Returns 1 if the window will fit in the framebuffer, or 0 otherwise.
 */
static int window_will_fit_framebuffer(const struct fb_var_screeninfo *var,
				       unsigned fb_size)
{
	unsigned line_length;

	line_length = (var->bits_per_pixel * var->xres_virtual + 7) / 8;
	/* line length must be a multiple of the cache line size (32) */
	line_length = ((line_length + 31) / 32) * 32;

	if (var->yres_virtual * line_length <= fb_size)
		return 1;
	else
		return 0;
}

/*
 * FBIO_WAITFORVSYNC handler
 */
static int davincifb_wait_for_vsync(struct fb_info *info)
{
	struct vpbe_dm_win_info *win = info->par;
	wait_queue_t wq;
	unsigned long cnt;
	int ret;

	init_waitqueue_entry(&wq, current);

	cnt = win->dm->vsync_cnt;
	ret = wait_event_interruptible_timeout(win->dm->vsync_wait,
					       cnt != win->dm->vsync_cnt,
					       win->dm->timeout);
	if (ret < 0)
		return ret;
	if (ret == 0)
		return -ETIMEDOUT;

	return 0;
}

static void davincifb_vsync_callback(unsigned event, void *arg)
{
	struct vpbe_dm_info *dm = (struct vpbe_dm_info *)arg;
	unsigned long addr = 0;
	static unsigned last_event;

	event &= ~DAVINCI_DISP_END_OF_FRAME;
	if (event == last_event) {
		/* progressive */
		xchg(&addr, dm->win[WIN_OSD0].sdram_address);
		if (addr) {
			davinci_disp_start_layer(dm->win[WIN_OSD0].layer,
						 dm->win[WIN_OSD0].
						 sdram_address,
						 NULL);
			dm->win[WIN_OSD0].sdram_address = 0;
		}
		addr = 0;
		xchg(&addr, dm->win[WIN_OSD1].sdram_address);
		if (addr) {
			davinci_disp_start_layer(dm->win[WIN_OSD1].layer,
						 dm->win[WIN_OSD1].
						 sdram_address,
						 NULL);
			dm->win[WIN_OSD1].sdram_address = 0;
		}
		addr = 0;
		xchg(&addr, dm->win[WIN_VID0].sdram_address);
		if (addr) {
			davinci_disp_start_layer(dm->win[WIN_VID0].layer,
						 dm->win[WIN_VID0].
						 sdram_address,
						 NULL);
			dm->win[WIN_VID0].sdram_address = 0;
		}
		addr = 0;
		xchg(&addr, dm->win[WIN_VID1].sdram_address);
		if (addr) {
			davinci_disp_start_layer(dm->win[WIN_VID1].layer,
						 dm->win[WIN_VID1].
						 sdram_address,
						 NULL);
			dm->win[WIN_VID1].sdram_address = 0;
		}
		++dm->vsync_cnt;
		wake_up_interruptible(&dm->vsync_wait);
	} else {
		/* interlaced */
		if (event & DAVINCI_DISP_SECOND_FIELD) {
			xchg(&addr, dm->win[WIN_OSD0].sdram_address);
			if (addr) {
				davinci_disp_start_layer(dm->win[WIN_OSD0].
							 layer,
							 dm->win[WIN_OSD0].
							 sdram_address,
							 NULL);
				dm->win[WIN_OSD0].sdram_address = 0;
			}
			addr = 0;
			xchg(&addr, dm->win[WIN_OSD1].sdram_address);
			if (addr) {
				davinci_disp_start_layer(dm->win[WIN_OSD1].
							 layer,
							 dm->win[WIN_OSD1].
							 sdram_address,
							 NULL);
				dm->win[WIN_OSD1].sdram_address = 0;
			}
			addr = 0;
			xchg(&addr, dm->win[WIN_VID0].sdram_address);
			if (addr) {
				davinci_disp_start_layer(dm->win[WIN_VID0].
							 layer,
							 dm->win[WIN_VID0].
							 sdram_address,
							 NULL);
				dm->win[WIN_VID0].sdram_address = 0;
			}
			addr = 0;
			xchg(&addr, dm->win[WIN_VID1].sdram_address);
			if (addr) {
				davinci_disp_start_layer(dm->win[WIN_VID1].
							 layer,
							 dm->win[WIN_VID1].
							 sdram_address,
							 NULL);
				dm->win[WIN_VID1].sdram_address = 0;
			}
		} else {
			++dm->vsync_cnt;
			wake_up_interruptible(&dm->vsync_wait);
		}
	}
	last_event = event;
}

/*
 * FBIO_SETATTRIBUTE handler
 *
 * This ioctl is deprecated.  The user can write the attribute values directly
 * to the OSD1 framebuffer.
 *
 * Set a uniform attribute value over a rectangular area on the attribute
 * window. The attribute value (0 to 15) is passed through the fb_fillrect's
 * color parameter.  r->dx and r->width must both be even.  If not, they are
 * rounded down.
 */
static int vpbe_set_attr_blend(struct fb_info *info, struct fb_fillrect *r)
{
	struct vpbe_dm_win_info *win = info->par;
	struct fb_var_screeninfo *var = &info->var;
	char __iomem *start;
	u8 blend;
	u32 width_bytes;

	if (win->layer != WIN_OSD1)
		return -EINVAL;

	if (!is_attribute_mode(var))
		return -EINVAL;

	if (r->dx + r->width > var->xres_virtual)
		return -EINVAL;
	if (r->dy + r->height > var->yres_virtual)
		return -EINVAL;
	if (r->color > 15)
		return -EINVAL;

	width_bytes = (r->width * var->bits_per_pixel) / 8;
	start =
	    info->screen_base + r->dy * info->fix.line_length +
	    (r->dx * var->bits_per_pixel) / 8;

	blend = (((u8) r->color & 0xf) << 4) | ((u8) r->color);
	while (r->height--) {
		memset(start, blend, width_bytes);
		start += info->fix.line_length;
	}

	return 0;
}

/*
 * FBIO_SETPOSX handler
 */
static int vpbe_setposx(struct fb_info *info, unsigned xpos)
{
	struct vpbe_dm_win_info *win = info->par;
	struct fb_var_screeninfo *var = &info->var;
	struct fb_var_screeninfo v;
	unsigned old_xpos = win->xpos;
	int retval;

	if (!win->own_window)
		return -ENODEV;

	memcpy(&v, var, sizeof(v));
	win->xpos = xpos;
	retval = info->fbops->fb_check_var(&v, info);
	if (retval) {
		win->xpos = old_xpos;
		return retval;
	}

	/* update the window position */
	memcpy(var, &v, sizeof(v));
	retval = info->fbops->fb_set_par(info);

	return retval;
}

/*
 * FBIO_SETPOSY handler
 */
static int vpbe_setposy(struct fb_info *info, unsigned ypos)
{
	struct vpbe_dm_win_info *win = info->par;
	struct fb_var_screeninfo *var = &info->var;
	struct fb_var_screeninfo v;
	unsigned old_ypos = win->ypos;
	int retval;

	if (!win->own_window)
		return -ENODEV;

	memcpy(&v, var, sizeof(v));
	win->ypos = ypos;
	retval = info->fbops->fb_check_var(&v, info);
	if (retval) {
		win->ypos = old_ypos;
		return retval;
	}

	/* update the window position */
	memcpy(var, &v, sizeof(v));
	retval = info->fbops->fb_set_par(info);

	return retval;
}

/*
 * FBIO_SETZOOM handler
 */
static int vpbe_set_zoom(struct fb_info *info, struct zoom_params *zoom)
{
	struct vpbe_dm_win_info *win = info->par;
	enum davinci_zoom_factor h_zoom, v_zoom;

	if (!win->own_window)
		return -ENODEV;

	switch (zoom->zoom_h) {
	case 0:
		h_zoom = ZOOM_X1;
		break;
	case 1:
		h_zoom = ZOOM_X2;
		break;
	case 2:
		h_zoom = ZOOM_X4;
		break;
	default:
		return -EINVAL;
	}

	switch (zoom->zoom_v) {
	case 0:
		v_zoom = ZOOM_X1;
		break;
	case 1:
		v_zoom = ZOOM_X2;
		break;
	case 2:
		v_zoom = ZOOM_X4;
		break;
	default:
		return -EINVAL;
	}

	davinci_disp_set_zoom(win->layer, h_zoom, v_zoom);

	return 0;
}

/*
 * FBIO_ENABLE_DISABLE_WIN handler
 *
 * This ioctl is deprecated.  Use the standard FBIOBLANK ioctl instead.
 */
static int vpbe_enable_disable_win(struct fb_info *info, int enable)
{
	struct vpbe_dm_win_info *win = info->par;
	int retval = 0;

	if (!win->own_window)
		return -ENODEV;

	if (enable) {
		win->display_window = 1;
		retval = info->fbops->fb_check_var(&info->var, info);
		if (retval)
			return retval;
		retval = info->fbops->fb_set_par(info);
	} else {
		win->display_window = 0;
		davinci_disp_disable_layer(win->layer);
	}

	return retval;
}

/*
 * FBIO_SET_BITMAP_BLEND_FACTOR handler
 */
static int vpbe_bitmap_set_blend_factor(struct fb_info *info, struct vpbe_bitmap_blend_params
					*blend_para)
{
	enum davinci_osd_layer osdwin = fb_info_to_osd_enum(info);

	if (!is_osd_win(info))
		return -EINVAL;

	if (blend_para->bf > OSD_8_VID_0)
		return -EINVAL;

	davinci_disp_set_blending_factor(osdwin, blend_para->bf);
	if (blend_para->enable_colorkeying)
		davinci_disp_enable_color_key(osdwin, blend_para->colorkey);
	else
		davinci_disp_disable_color_key(osdwin);

	return 0;
}

/*
 * FBIO_SET_BITMAP_WIN_RAM_CLUT handler
 *
 * This ioctl is deprecated.  Use the standard framebuffer ioctl FBIOPUTCMAP
 * instead.  Note that FBIOPUTCMAP colors are expressed in RGB space instead of
 * YCbCr space.
 */
static int vpbe_bitmap_set_ram_clut(struct fb_info *info,
				    unsigned char ram_clut[256][3])
{
	int i;

	if (!is_osd_win(info))
		return -EINVAL;

	for (i = 0; i < 256; i++) {
		davinci_disp_set_clut_ycbcr(i, ram_clut[i][0], ram_clut[i][1],
					    ram_clut[i][2]);
	}
	return 0;
}

/*
 * FBIO_ENABLE_DISABLE_ATTRIBUTE_WIN handler
 *
 * This ioctl is deprecated.  Attribute mode can be enabled via the standard
 * framebuffer ioctl FBIOPUT_VSCREENINFO by setting var->bits_per_pixel to 4
 * and var->nonstd to a non-zero value.  Attribute mode can be disabled by using
 * FBIOPUT_VSCREENINFO to set a standard pixel format.
 *
 * The enabled/disabled status of OSD1 is unchanged by this ioctl.  To avoid
 * display glitches, you should disable OSD1 prior to calling this ioctl.
 *
 * When enabling attribute mode, var->bits_per_pixel is set to 4.  var->xres,
 * var->yres, var->xres_virtual, var->yres_virtual, win->xpos, and win->ypos are
 * all copied from OSD0.  var->xoffset and var->yoffset are set to 0.
 * fix->line_length is updated to be consistent with 4 bits per pixel.  No
 * changes are made to the OSD1 configuration if OSD1 is already in attribute
 * mode.
 *
 * When disabling attribute mode, the window geometry is unchanged.
 * var->bits_per_pixel remains set to 4.  No changes are made to the OSD1
 * configuration if OSD1 is not in attribute mode.
 */
static int vpbe_enable_disable_attribute_window(struct fb_info *info, u32 flag)
{
	struct vpbe_dm_win_info *win = info->par;
	struct fb_var_screeninfo *var = &info->var;
	struct fb_var_screeninfo v;
	struct davinci_layer_config lconfig;
	int retval;

	if (win->layer != WIN_OSD1)
		return -EINVAL;

	/* return with no error if there is nothing to do */
	if ((is_attribute_mode(var) && flag)
	    || (!is_attribute_mode(var) && !flag))
		return 0;

	/* start with the current OSD1 var */
	memcpy(&v, var, sizeof(v));

	if (flag) {		/* enable attribute mode */
		const struct vpbe_dm_win_info *osd0 = &win->dm->win[WIN_OSD0];
		const struct fb_var_screeninfo *osd0_var = &osd0->info->var;
		unsigned old_xpos = win->xpos;
		unsigned old_ypos = win->ypos;
		/* get the OSD0 window configuration */
		convert_fb_var_to_osd(osd0_var, &lconfig, win->dm->yc_pixfmt);
		/* change the pixfmt to attribute mode */
		lconfig.pixfmt = PIXFMT_OSD_ATTR;
		/* update the var for OSD1 */
		convert_osd_to_fb_var(&lconfig, &v);
		/* copy xres_virtual and yres_virtual from OSD0 */
		v.xres_virtual = osd0_var->xres_virtual;
		v.yres_virtual = osd0_var->yres_virtual;
		/* zero xoffset and yoffset */
		v.xoffset = 0;
		v.yoffset = 0;
		/* copy xpos and ypos from OSD0 */
		win->xpos = osd0->xpos;
		win->ypos = osd0->ypos;

		retval = info->fbops->fb_check_var(&v, info);
		if (retval) {
			win->xpos = old_xpos;
			win->ypos = old_ypos;
			return retval;
		}

		/*
		 * Enable attribute mode by replacing info->var and calling
		 * the fb_set_par method to activate it.
		 */
		memcpy(var, &v, sizeof(v));
		retval = info->fbops->fb_set_par(info);
	} else {		/* disable attribute mode */
		/* get the current OSD1 window configuration */
		convert_fb_var_to_osd(var, &lconfig, win->dm->yc_pixfmt);
		/* change the pixfmt to 4-bits-per-pixel bitmap */
		lconfig.pixfmt = PIXFMT_4BPP;
		/* update the var for OSD1 */
		convert_osd_to_fb_var(&lconfig, &v);

		retval = info->fbops->fb_check_var(&v, info);
		if (retval)
			return retval;

		/*
		 * Disable attribute mode by replacing info->var and calling
		 * the fb_set_par method to activate it.
		 */
		memcpy(var, &v, sizeof(v));
		retval = info->fbops->fb_set_par(info);
	}

	return retval;
}

/*
 * FBIO_GET_BLINK_INTERVAL handler
 */
static int vpbe_get_blinking(struct fb_info *info,
			     struct vpbe_blink_option *blink_option)
{
	struct vpbe_dm_win_info *win = info->par;
	enum davinci_blink_interval blink;
	int enabled;

	if (win->layer != WIN_OSD1)
		return -EINVAL;

	davinci_disp_get_blink_attribute(&enabled, &blink);
	blink_option->blinking = enabled;
	blink_option->interval = blink;

	return 0;
}

/*
 * FBIO_SET_BLINK_INTERVAL handler
 */
static int vpbe_set_blinking(struct fb_info *info,
			     struct vpbe_blink_option *blink_option)
{
	struct vpbe_dm_win_info *win = info->par;

	if (win->layer != WIN_OSD1)
		return -EINVAL;

	if (blink_option->interval > BLINK_X4)
		return -EINVAL;

	davinci_disp_set_blink_attribute(blink_option->blinking,
					 blink_option->interval);

	return 0;
}

/*
 * FBIO_GET_VIDEO_CONFIG_PARAMS handler
 *
 * Despite the name, this ioctl can be used on both video windows and OSD
 * (bitmap) windows.
 */
static int vpbe_get_vid_params(struct fb_info *info,
			       struct vpbe_video_config_params *vid_conf_params)
{
	struct vpbe_dm_win_info *win = info->par;
	enum davinci_h_exp_ratio h_exp;
	enum davinci_v_exp_ratio v_exp;

	if (!win->own_window)
		return -ENODEV;

	if (is_vid_win(info))
		davinci_disp_get_vid_expansion(&h_exp, &v_exp);
	else
		davinci_disp_get_osd_expansion(&h_exp, &v_exp);

	vid_conf_params->cb_cr_order =
	    (win->dm->yc_pixfmt == PIXFMT_YCbCrI) ? 0 : 1;
	vid_conf_params->exp_info.horizontal = h_exp;
	vid_conf_params->exp_info.vertical = v_exp;

	return 0;
}

/*
 * FBIO_SET_VIDEO_CONFIG_PARAMS handler
 *
 * Despite the name, this ioctl can be used on both video windows and OSD
 * (bitmap) windows.
 *
 * NOTE: If the cb_cr_order is changed, it won't take effect until an
 * FBIOPUT_VSCREENINFO ioctl is executed on a window with a YC pixel format.
 */
static int vpbe_set_vid_params(struct fb_info *info,
			       struct vpbe_video_config_params *vid_conf_params)
{
	struct vpbe_dm_win_info *win = info->par;
	enum davinci_h_exp_ratio h_exp;
	enum davinci_v_exp_ratio v_exp;

	if (!win->own_window)
		return -ENODEV;

	if (vid_conf_params->exp_info.horizontal > H_EXP_3_OVER_2)
		return -EINVAL;

	if (vid_conf_params->exp_info.vertical > V_EXP_6_OVER_5)
		return -EINVAL;

	win->dm->yc_pixfmt =
	    vid_conf_params->cb_cr_order ? PIXFMT_YCrCbI : PIXFMT_YCbCrI;

	h_exp = vid_conf_params->exp_info.horizontal;
	v_exp = vid_conf_params->exp_info.vertical;
	if (is_vid_win(info))
		davinci_disp_set_vid_expansion(h_exp, v_exp);
	else
		davinci_disp_set_osd_expansion(h_exp, v_exp);

	return 0;
}

/*
 * FBIO_GET_BITMAP_CONFIG_PARAMS handler
 */
static int vpbe_bitmap_get_params(struct fb_info *info, struct vpbe_bitmap_config_params
				  *bitmap_conf_params)
{
	enum davinci_osd_layer osdwin = fb_info_to_osd_enum(info);
	enum davinci_clut clut;

	if (!is_osd_win(info))
		return -EINVAL;

	clut = davinci_disp_get_osd_clut(osdwin);
	if (clut == ROM_CLUT)
		bitmap_conf_params->clut_select = davinci_disp_get_rom_clut();
	else
		bitmap_conf_params->clut_select = 2;

	bitmap_conf_params->attenuation_enable =
	    davinci_disp_get_rec601_attenuation(osdwin);

	memset(&bitmap_conf_params->clut_idx, 0,
	       sizeof(bitmap_conf_params->clut_idx));

	switch (info->var.bits_per_pixel) {
	case 1:
		bitmap_conf_params->clut_idx.for_1bit_bitmap.bitmap_val_0 =
		    davinci_disp_get_palette_map(osdwin, 0);
		bitmap_conf_params->clut_idx.for_1bit_bitmap.bitmap_val_1 =
		    davinci_disp_get_palette_map(osdwin, 1);
		break;
	case 2:
		bitmap_conf_params->clut_idx.for_2bit_bitmap.bitmap_val_0 =
		    davinci_disp_get_palette_map(osdwin, 0);
		bitmap_conf_params->clut_idx.for_2bit_bitmap.bitmap_val_1 =
		    davinci_disp_get_palette_map(osdwin, 1);
		bitmap_conf_params->clut_idx.for_2bit_bitmap.bitmap_val_2 =
		    davinci_disp_get_palette_map(osdwin, 2);
		bitmap_conf_params->clut_idx.for_2bit_bitmap.bitmap_val_3 =
		    davinci_disp_get_palette_map(osdwin, 3);
		break;
	case 4:
		bitmap_conf_params->clut_idx.for_4bit_bitmap.bitmap_val_0 =
		    davinci_disp_get_palette_map(osdwin, 0);
		bitmap_conf_params->clut_idx.for_4bit_bitmap.bitmap_val_1 =
		    davinci_disp_get_palette_map(osdwin, 1);
		bitmap_conf_params->clut_idx.for_4bit_bitmap.bitmap_val_2 =
		    davinci_disp_get_palette_map(osdwin, 2);
		bitmap_conf_params->clut_idx.for_4bit_bitmap.bitmap_val_3 =
		    davinci_disp_get_palette_map(osdwin, 3);
		bitmap_conf_params->clut_idx.for_4bit_bitmap.bitmap_val_4 =
		    davinci_disp_get_palette_map(osdwin, 4);
		bitmap_conf_params->clut_idx.for_4bit_bitmap.bitmap_val_5 =
		    davinci_disp_get_palette_map(osdwin, 5);
		bitmap_conf_params->clut_idx.for_4bit_bitmap.bitmap_val_6 =
		    davinci_disp_get_palette_map(osdwin, 6);
		bitmap_conf_params->clut_idx.for_4bit_bitmap.bitmap_val_7 =
		    davinci_disp_get_palette_map(osdwin, 7);
		bitmap_conf_params->clut_idx.for_4bit_bitmap.bitmap_val_8 =
		    davinci_disp_get_palette_map(osdwin, 8);
		bitmap_conf_params->clut_idx.for_4bit_bitmap.bitmap_val_9 =
		    davinci_disp_get_palette_map(osdwin, 9);
		bitmap_conf_params->clut_idx.for_4bit_bitmap.bitmap_val_10 =
		    davinci_disp_get_palette_map(osdwin, 10);
		bitmap_conf_params->clut_idx.for_4bit_bitmap.bitmap_val_11 =
		    davinci_disp_get_palette_map(osdwin, 11);
		bitmap_conf_params->clut_idx.for_4bit_bitmap.bitmap_val_12 =
		    davinci_disp_get_palette_map(osdwin, 12);
		bitmap_conf_params->clut_idx.for_4bit_bitmap.bitmap_val_13 =
		    davinci_disp_get_palette_map(osdwin, 13);
		bitmap_conf_params->clut_idx.for_4bit_bitmap.bitmap_val_14 =
		    davinci_disp_get_palette_map(osdwin, 14);
		bitmap_conf_params->clut_idx.for_4bit_bitmap.bitmap_val_15 =
		    davinci_disp_get_palette_map(osdwin, 15);
		break;
	default:
		break;
	}

	return 0;
}

/*
 * FBIO_SET_BITMAP_CONFIG_PARAMS handler
 *
 * The palette map is ignored unless the color depth is set to 1, 2, or 4 bits
 * per pixel.  A default palette map is supplied for these color depths where
 * the clut index is equal to the pixel value.  It is not necessary to change
 * the default palette map when using the RAM clut, because the RAM clut values
 * can be changed.  It is only necessary to modify the default palette map when
 * using a ROM clut.
 */
static int vpbe_bitmap_set_params(struct fb_info *info, struct vpbe_bitmap_config_params
				  *bitmap_conf_params)
{
	enum davinci_osd_layer osdwin = fb_info_to_osd_enum(info);
	enum davinci_clut clut = ROM_CLUT;

	if (!is_osd_win(info))
		return -EINVAL;

	if (bitmap_conf_params->clut_select == 0)
		davinci_disp_set_rom_clut(ROM_CLUT0);
	else if (bitmap_conf_params->clut_select == 1)
		davinci_disp_set_rom_clut(ROM_CLUT1);
	else if (bitmap_conf_params->clut_select == 2)
		clut = RAM_CLUT;
	else
		return -EINVAL;

	davinci_disp_set_osd_clut(osdwin, clut);
	davinci_disp_set_rec601_attenuation(osdwin,
					    bitmap_conf_params->
					    attenuation_enable);

	switch (info->var.bits_per_pixel) {
	case 1:
		davinci_disp_set_palette_map(osdwin, 0,
					     bitmap_conf_params->clut_idx.
					     for_1bit_bitmap.bitmap_val_0);
		davinci_disp_set_palette_map(osdwin, 1,
					     bitmap_conf_params->clut_idx.
					     for_1bit_bitmap.bitmap_val_1);
		break;
	case 2:
		davinci_disp_set_palette_map(osdwin, 0,
					     bitmap_conf_params->clut_idx.
					     for_2bit_bitmap.bitmap_val_0);
		davinci_disp_set_palette_map(osdwin, 1,
					     bitmap_conf_params->clut_idx.
					     for_2bit_bitmap.bitmap_val_1);
		davinci_disp_set_palette_map(osdwin, 2,
					     bitmap_conf_params->clut_idx.
					     for_2bit_bitmap.bitmap_val_2);
		davinci_disp_set_palette_map(osdwin, 3,
					     bitmap_conf_params->clut_idx.
					     for_2bit_bitmap.bitmap_val_3);
		break;
	case 4:
		davinci_disp_set_palette_map(osdwin, 0,
					     bitmap_conf_params->clut_idx.
					     for_4bit_bitmap.bitmap_val_0);
		davinci_disp_set_palette_map(osdwin, 1,
					     bitmap_conf_params->clut_idx.
					     for_4bit_bitmap.bitmap_val_1);
		davinci_disp_set_palette_map(osdwin, 2,
					     bitmap_conf_params->clut_idx.
					     for_4bit_bitmap.bitmap_val_2);
		davinci_disp_set_palette_map(osdwin, 3,
					     bitmap_conf_params->clut_idx.
					     for_4bit_bitmap.bitmap_val_3);
		davinci_disp_set_palette_map(osdwin, 4,
					     bitmap_conf_params->clut_idx.
					     for_4bit_bitmap.bitmap_val_4);
		davinci_disp_set_palette_map(osdwin, 5,
					     bitmap_conf_params->clut_idx.
					     for_4bit_bitmap.bitmap_val_5);
		davinci_disp_set_palette_map(osdwin, 6,
					     bitmap_conf_params->clut_idx.
					     for_4bit_bitmap.bitmap_val_6);
		davinci_disp_set_palette_map(osdwin, 7,
					     bitmap_conf_params->clut_idx.
					     for_4bit_bitmap.bitmap_val_7);
		davinci_disp_set_palette_map(osdwin, 8,
					     bitmap_conf_params->clut_idx.
					     for_4bit_bitmap.bitmap_val_8);
		davinci_disp_set_palette_map(osdwin, 9,
					     bitmap_conf_params->clut_idx.
					     for_4bit_bitmap.bitmap_val_9);
		davinci_disp_set_palette_map(osdwin, 10,
					     bitmap_conf_params->clut_idx.
					     for_4bit_bitmap.bitmap_val_10);
		davinci_disp_set_palette_map(osdwin, 11,
					     bitmap_conf_params->clut_idx.
					     for_4bit_bitmap.bitmap_val_11);
		davinci_disp_set_palette_map(osdwin, 12,
					     bitmap_conf_params->clut_idx.
					     for_4bit_bitmap.bitmap_val_12);
		davinci_disp_set_palette_map(osdwin, 13,
					     bitmap_conf_params->clut_idx.
					     for_4bit_bitmap.bitmap_val_13);
		davinci_disp_set_palette_map(osdwin, 14,
					     bitmap_conf_params->clut_idx.
					     for_4bit_bitmap.bitmap_val_14);
		davinci_disp_set_palette_map(osdwin, 15,
					     bitmap_conf_params->clut_idx.
					     for_4bit_bitmap.bitmap_val_15);
		break;
	default:
		break;
	}

	return 0;
}

/*
 * FBIO_SET_BACKG_COLOR handler
 */
static int vpbe_set_backg_color(struct fb_info *info,
				struct vpbe_backg_color *backg_color)
{
	enum davinci_clut clut = ROM_CLUT;

	if (backg_color->clut_select == 0)
		davinci_disp_set_rom_clut(ROM_CLUT0);
	else if (backg_color->clut_select == 1)
		davinci_disp_set_rom_clut(ROM_CLUT1);
	else if (backg_color->clut_select == 2)
		clut = RAM_CLUT;
	else
		return -EINVAL;

	davinci_disp_set_background(clut, backg_color->color_offset);

	return 0;
}

/*
 * FBIO_SETPOS handler
 */
static int vpbe_setpos(struct fb_info *info,
		       struct vpbe_window_position *win_pos)
{
	struct vpbe_dm_win_info *win = info->par;
	struct fb_var_screeninfo *var = &info->var;
	struct fb_var_screeninfo v;
	unsigned old_xpos = win->xpos;
	unsigned old_ypos = win->ypos;
	int retval;

	if (!win->own_window)
		return -ENODEV;

	memcpy(&v, var, sizeof(v));
	win->xpos = win_pos->xpos;
	win->ypos = win_pos->ypos;
	retval = info->fbops->fb_check_var(&v, info);
	if (retval) {
		win->xpos = old_xpos;
		win->ypos = old_ypos;
		return retval;
	}

	/* update the window position */
	memcpy(var, &v, sizeof(v));
	retval = info->fbops->fb_set_par(info);

	return retval;
}

/*
 * FBIO_SET_CURSOR handler
 */
static int vpbe_set_cursor_params(struct fb_info *info,
				  struct fb_cursor *fbcursor)
{
	struct davinci_cursor_config cursor;

	if (!fbcursor->enable) {
		davinci_disp_cursor_disable();
		return 0;
	}

	cursor.xsize = fbcursor->image.width;
	cursor.ysize = fbcursor->image.height;
	cursor.xpos = fbcursor->image.dx;
	cursor.ypos = fbcursor->image.dy;
	cursor.interlaced = is_window_interlaced(&info->var);
	cursor.h_width =
	    (fbcursor->image.depth > 7) ? 7 : fbcursor->image.depth;
	cursor.v_width = cursor.h_width;
	cursor.clut = ROM_CLUT;
	cursor.clut_index = fbcursor->image.fg_color;

	davinci_disp_set_cursor_config(&cursor);

	davinci_disp_cursor_enable();

	return 0;
}

/*
 * fb_ioctl method
 */
static int
davincifb_ioctl(struct fb_info *info, unsigned int cmd,	unsigned long arg)
{
	struct vpbe_dm_win_info *win = info->par;
	void __user *argp = (void __user *)arg;
	struct fb_fillrect rect;
	struct zoom_params zoom;
	int retval = 0;
	struct vpbe_bitmap_blend_params blend_para;
	struct vpbe_blink_option blink_option;
	struct vpbe_video_config_params vid_conf_params;
	struct vpbe_bitmap_config_params bitmap_conf_params;
	struct vpbe_backg_color backg_color;
	struct vpbe_window_position win_pos;
	struct fb_cursor cursor;

	switch (cmd) {
	case FBIO_WAITFORVSYNC:
		/* This ioctl accepts an integer argument to specify a
		 * display.  We only support one display, so we will
		 * simply ignore the argument.
		 */
		return davincifb_wait_for_vsync(info);

	case FBIO_SETATTRIBUTE:
		if (copy_from_user(&rect, argp, sizeof(rect)))
			return -EFAULT;
		return vpbe_set_attr_blend(info, &rect);

	case FBIO_SETPOSX:
		return vpbe_setposx(info, arg);

	case FBIO_SETPOSY:
		return vpbe_setposy(info, arg);

	case FBIO_SETZOOM:
		if (copy_from_user(&zoom, argp, sizeof(zoom)))
			return -EFAULT;
		return vpbe_set_zoom(info, &zoom);

	case FBIO_ENABLE_DISABLE_WIN:
		return vpbe_enable_disable_win(info, arg);

	case FBIO_SET_BITMAP_BLEND_FACTOR:
		if (copy_from_user(&blend_para, argp, sizeof(blend_para)))
			return -EFAULT;
		return vpbe_bitmap_set_blend_factor(info, &blend_para);

	case FBIO_SET_BITMAP_WIN_RAM_CLUT:
		if (copy_from_user(win->dm->ram_clut[0], argp, RAM_CLUT_SIZE))
			return -EFAULT;
		return vpbe_bitmap_set_ram_clut(info, win->dm->ram_clut);

	case FBIO_ENABLE_DISABLE_ATTRIBUTE_WIN:
		return vpbe_enable_disable_attribute_window(info, arg);

	case FBIO_GET_BLINK_INTERVAL:
		if ((retval = vpbe_get_blinking(info, &blink_option)) < 0)
			return retval;
		if (copy_to_user(argp, &blink_option, sizeof(blink_option)))
			return -EFAULT;
		return 0;

	case FBIO_SET_BLINK_INTERVAL:
		if (copy_from_user(&blink_option, argp, sizeof(blink_option)))
			return -EFAULT;
		return vpbe_set_blinking(info, &blink_option);

	case FBIO_GET_VIDEO_CONFIG_PARAMS:
		if ((retval = vpbe_get_vid_params(info, &vid_conf_params)) < 0)
			return retval;
		if (copy_to_user
		    (argp, &vid_conf_params, sizeof(vid_conf_params)))
			return -EFAULT;
		return 0;

	case FBIO_SET_VIDEO_CONFIG_PARAMS:
		if (copy_from_user
		    (&vid_conf_params, argp, sizeof(vid_conf_params)))
			return -EFAULT;
		return vpbe_set_vid_params(info, &vid_conf_params);

	case FBIO_GET_BITMAP_CONFIG_PARAMS:
		if ((retval =
		     vpbe_bitmap_get_params(info, &bitmap_conf_params)) < 0)
			return retval;
		if (copy_to_user
		    (argp, &bitmap_conf_params, sizeof(bitmap_conf_params)))
			return -EFAULT;
		return 0;

	case FBIO_SET_BITMAP_CONFIG_PARAMS:
		if (copy_from_user
		    (&bitmap_conf_params, argp, sizeof(bitmap_conf_params)))
			return -EFAULT;
		return vpbe_bitmap_set_params(info, &bitmap_conf_params);

	case FBIO_SET_BACKG_COLOR:
		if (copy_from_user(&backg_color, argp, sizeof(backg_color)))
			return -EFAULT;
		return vpbe_set_backg_color(info, &backg_color);

	case FBIO_SETPOS:
		if (copy_from_user(&win_pos, argp, sizeof(win_pos)))
			return -EFAULT;
		return vpbe_setpos(info, &win_pos);

	case FBIO_SET_CURSOR:
		if (copy_from_user(&cursor, argp, sizeof(cursor)))
			return -EFAULT;
		return vpbe_set_cursor_params(info, &cursor);

	default:
		return -EINVAL;
	}
}


/*
 * fb_check_var method
 */
static int
davincifb_check_var(struct fb_var_screeninfo *var, struct fb_info *info)
{
	struct vpbe_dm_win_info *win = info->par;
	struct fb_videomode *mode = &win->dm->mode;
	struct davinci_layer_config lconfig;
	struct fb_fix_screeninfo fix;
		
	/*
	 * Get an updated copy of the video mode from the encoder manager, just
	 * in case the display has been switched.
	 */
	get_video_mode(mode);

	/*
	 * xres, yres, xres_virtual, or yres_virtual equal to zero is treated as
	 * a special case.  It indicates that the window should be disabled.  If
	 * the window is a video window, it will also be released.
	 */
	if (var->xres == 0 || var->yres == 0 || var->xres_virtual == 0
	    || var->yres_virtual == 0) {
		var->xres = 0;
		var->yres = 0;
		var->xres_virtual = 0;
		var->yres_virtual = 0;
		return 0;
	}

	switch (var->bits_per_pixel) {
	case 1:
	case 2:
	case 4:
	case 8:
	case 16:
		break;
	case 24:
		if (cpu_is_davinci_dm355())
			return -EINVAL;
		break;
	case 32:
		if (cpu_is_davinci_dm644x())
			return -EINVAL;
		break;
	default:
		return -EINVAL;
	}

	if (var->xres_virtual < var->xres || var->yres_virtual < var->yres)
		return -EINVAL;
	if (var->xoffset > var->xres_virtual - var->xres)
		return -EINVAL;
	if (var->yoffset > var->yres_virtual - var->yres)
		return -EINVAL;
	if (mode->xres < var->xres || mode->yres < var->yres)
		return -EINVAL;
	if (win->xpos > mode->xres - var->xres)
		return -EINVAL;
	if (win->ypos > mode->yres - var->yres)
		return -EINVAL;
	convert_fb_var_to_osd(var, &lconfig, win->dm->yc_pixfmt);

	update_fix_info(var, &fix);
	lconfig.line_length = fix.line_length;
	lconfig.xpos = win->xpos;
	lconfig.ypos = win->ypos;
	/* xoffset must be a multiple of xpanstep */
	if (var->xoffset & ~(fix.xpanstep - 1))
		return -EINVAL;

	/* check if we have enough video memory to support this mode */
	if (!window_will_fit_framebuffer(var, info->fix.smem_len))
		return -EINVAL;

	/* see if the OSD manager approves of this configuration */
	if (davinci_disp_try_layer_config(win->layer, &lconfig))
		return -EINVAL;
	/*
	 * Reject this var if the OSD manager would have to modify the window
	 * geometry to make it work.
	 */
	if (lconfig.xsize != var->xres || lconfig.ysize != var->yres)
		return -EINVAL;
	if (lconfig.xpos != win->xpos || lconfig.ypos != win->ypos)
		return -EINVAL;
	
	/*
	 * At this point we have accepted the var, so now we convert our layer
	 * configuration struct back to the var in order to make all of the
	 * pixel format and geometry values consistent.  The var timing values
	 * will be unmodified, as we have no way to verify them.
	 */
	convert_osd_to_fb_var(&lconfig, var);

	return 0;
}

/*
 * fb_set_par method
 */
static int davincifb_set_par(struct fb_info *info)
{
	struct vpbe_dm_win_info *win = info->par;
	struct fb_var_screeninfo *var = &info->var;
	struct davinci_layer_config lconfig;
	struct fb_videomode mode;
	unsigned start;

	/* update the fix info to be consistent with the var */
	update_fix_info(var, &info->fix);
	convert_fb_info_to_osd(info, &lconfig);

	/* See if we need to pass the timing values to the encoder manager. */
	memcpy(&mode, &win->dm->mode, sizeof(mode));
	fb_var_to_videomode(&mode, var);
	mode.name = win->dm->mode.name;
	if (mode.xres == win->dm->mode.xres && mode.yres == win->dm->mode.yres
	    && mode.pixclock != 0) {
		/*
		 * If the timing parameters from the var are different than the
		 * timing parameters from the encoder, try to update the
		 * timing parameters with the encoder manager.
		 */
		if (!fb_mode_is_equal(&mode, &win->dm->mode))
			set_video_mode(&mode);
	}
	/* update our copy of the encoder video mode */
	get_video_mode(&win->dm->mode);

	/* turn off ping-pong buffer and field inversion to fix
	   the image shaking problem in 1080I mode. The problem i.d. by the
	   DM6446 Advisory 1.3.8 is not seen in 1080I mode, but the ping-pong
	   buffer workaround created a shaking problem. */
	if (win->layer == WIN_VID0 &&
			strcmp(mode.name, VID_ENC_STD_1080I_30) == 0 &&
			fb->invert_field)
		davinci_disp_set_field_inversion(0);

	/*
	 * Update the var with the encoder timing info.  The window geometry
	 * will be preserved.
	 */
	construct_fb_var(var, &win->dm->mode, &lconfig);

	/* need to update interlaced since the mode may have changed */
	lconfig.interlaced = var->vmode = win->dm->mode.vmode;
	/*
	 * xres, yres, xres_virtual, or yres_virtual equal to zero is treated as
	 * a special case.  It indicates that the window should be disabled.  If
	 * the window is a video window, it will also be released.
	 * Note that we disable the window, but we do not set the
	 * win->disable_window flag.  This allows the window to be re-enabled
	 * simply by using the FBIOPUT_VSCREENINFO ioctl to set a valid
	 * configuration.
	 */
	if (lconfig.xsize == 0 || lconfig.ysize == 0) {
		if (win->own_window) {
			davinci_disp_disable_layer(win->layer);
			if (is_vid_win(info)) {
				win->own_window = 0;
				davinci_disp_release_layer(win->layer);
			}
		}
		return 0;
	}

	/*
	 * If we don't currently own this window, we must claim it from the OSD
	 * manager.
	 */
	if (!win->own_window) {
		if (davinci_disp_request_layer(win->layer))
			return -ENODEV;
		win->own_window = 1;
	}

	/* DM365 YUV420 Planar */
	if (cpu_is_davinci_dm365() &&
			info->var.bits_per_pixel == 8 &&
			(win->layer == WIN_VID0 || win->layer == WIN_VID1)
			) {
		start =
			info->fix.smem_start +
			(var->xoffset * 12) / 8 +
			var->yoffset * 3 / 2 * info->fix.line_length;
	} else {
		start =
		info->fix.smem_start + (var->xoffset * var->bits_per_pixel) / 8
		+ var->yoffset * info->fix.line_length;
	}
	davinci_disp_set_layer_config(win->layer, &lconfig);
	davinci_disp_start_layer(win->layer, start, NULL);
	if (win->display_window)
		davinci_disp_enable_layer(win->layer, 0);

	return 0;
}

/*
 * This macro converts a 16-bit color passed to fb_setcolreg to the width
 * supported by the pixel format.
 */
#define CNVT_TOHW(val,width) ((((val)<<(width))+0x7FFF-(val))>>16)

/*
 * fb_setcolreg method
 */
static int davincifb_setcolreg(unsigned regno, unsigned red, unsigned green,
			       unsigned blue, unsigned transp,
			       struct fb_info *info)
{
	unsigned r, g, b, t;

	if (regno >= 256)	/* no. of hw registers */
		return -EINVAL;

	/*
	 * An RGB color palette isn't applicable to a window with a YUV pixel
	 * format or to a window in attribute mode.
	 */
	if (is_yuv(&info->var) || is_attribute_mode(&info->var))
		return -EINVAL;

	switch (info->fix.visual) {
	case FB_VISUAL_TRUECOLOR:
		r = CNVT_TOHW(red, info->var.red.length);
		g = CNVT_TOHW(green, info->var.green.length);
		b = CNVT_TOHW(blue, info->var.blue.length);
		t = CNVT_TOHW(transp, info->var.transp.length);
		break;
	case FB_VISUAL_PSEUDOCOLOR:
	default:
		r = CNVT_TOHW(red, 8);
		g = CNVT_TOHW(green, 8);
		b = CNVT_TOHW(blue, 8);
		t = 0;
		break;
	}

	/* Truecolor has hardware independent palette */
	if (info->fix.visual == FB_VISUAL_TRUECOLOR) {
		u32 v;

		if (regno >= 16)
			return -EINVAL;

		v = (r << info->var.red.offset) |
		    (g << info->var.green.offset) |
		    (b << info->var.blue.offset) |
		    (t << info->var.transp.offset);

		switch (info->var.bits_per_pixel) {
		case 16:
			((u16 *) (info->pseudo_palette))[regno] = v;
			break;
		case 24:
		case 32:
			((u32 *) (info->pseudo_palette))[regno] = v;
			break;
		}
		return 0;
	}

	if (!is_osd_win(info))
		return -EINVAL;

	davinci_disp_set_clut_rgb(regno, r, g, b);

	return 0;
}

/*
 * fb_pan_display method
 *
 * Pan the display using the `xoffset' and `yoffset' fields of the `var'
 * structure.  We don't support wrapping and ignore the FB_VMODE_YWRAP flag.
 */
static int
davincifb_pan_display(struct fb_var_screeninfo *var, struct fb_info *info)
{
	struct vpbe_dm_win_info *win = info->par;
	unsigned start;

	if (!win->own_window)
		return -ENODEV;

	if (var->xoffset > info->var.xres_virtual - info->var.xres)
		return -EINVAL;
	if (var->yoffset > info->var.yres_virtual - info->var.yres)
		return -EINVAL;

	/* xoffset must be a multiple of xpanstep */
	if (var->xoffset & ~(info->fix.xpanstep - 1))
		return -EINVAL;

	/* For DM365 video windows:
   * using bits_per_pixel to calculate start/offset address
   * needs to be changed for YUV420 planar format since
   * it is 8. But consider CbCr the real (avg) bits per pixel
   * is 12. line_length is calcuate using 8, so offset needs
   * to time 1.5 to take C plane into account.
   */
	if (cpu_is_davinci_dm365() &&
			info->var.bits_per_pixel == 8 &&
			(win->layer == WIN_VID0 || win->layer == WIN_VID1)
			) {
		start =
	    info->fix.smem_start +
	    (var->xoffset * 12) / 8 +
	    var->yoffset * 3 / 2 * info->fix.line_length;
	} else {
		start =
	    info->fix.smem_start +
	    (var->xoffset * info->var.bits_per_pixel) / 8 +
	    var->yoffset * info->fix.line_length;
	}
	if (davinci_disp_is_second_field()) {
		davinci_disp_start_layer(win->layer, start, NULL);
	} else
		win->sdram_address = start;

	return 0;
}

/*
 * fb_blank method
 *
 * Blank the screen if blank_mode != 0, else unblank.
 */
int davincifb_blank(int blank_mode, struct fb_info *info)
{
	struct vpbe_dm_win_info *win = info->par;
	int retval = 0;

	if (!win->own_window)
		return -ENODEV;

	if (!blank_mode) {
		win->display_window = 1;
		retval = info->fbops->fb_check_var(&info->var, info);
		if (retval)
			return retval;
		retval = info->fbops->fb_set_par(info);
	} else {
		win->display_window = 0;
		davinci_disp_disable_layer(win->layer);
	}

	return retval;
}

/*
 *  Frame buffer operations
 */
static struct fb_ops davincifb_ops = {
	.owner = THIS_MODULE,
	.fb_check_var = davincifb_check_var,
	.fb_set_par = davincifb_set_par,
	.fb_setcolreg = davincifb_setcolreg,
	.fb_blank = davincifb_blank,
	.fb_pan_display = davincifb_pan_display,
	.fb_fillrect = cfb_fillrect,
	.fb_copyarea = cfb_copyarea,
	.fb_imageblit = cfb_imageblit,
	.fb_rotate = NULL,
	.fb_sync = NULL,
	.fb_ioctl = davincifb_ioctl,
};

static void davincifb_release_window(struct device *dev,
				     struct vpbe_dm_win_info *win)
{
	struct fb_info *info = win->info;

	if (info) {
		unregister_framebuffer(info);
		win->info = NULL;
	}

	if (win->own_window) {
		davinci_disp_release_layer(win->layer);
		win->own_window = 0;
	}
	win->display_window = 0;

	if (info) {
		dma_free_coherent(dev, info->fix.smem_len, info->screen_base,
				  info->fix.smem_start);
		fb_dealloc_cmap(&info->cmap);
		kfree(info);
	}
}

static int davincifb_init_window(struct device *dev,
				 struct vpbe_dm_win_info *win,
				 struct davinci_layer_config *lconfig,
				 unsigned fb_size, const char *name)
{
	struct fb_info *info;
	int err;

	if (!fb_size)
		return 0;

	info = kzalloc(sizeof(*info), GFP_KERNEL);
	if (!info) {
		dev_err(dev, "%s: Can't allocate memory for fb_info struct.\n",
			name);
		return -ENOMEM;
	}
	win->info = info;

	/* initialize fb_info */
	info->par = win;
	info->flags =
	    FBINFO_DEFAULT | FBINFO_HWACCEL_COPYAREA | FBINFO_HWACCEL_FILLRECT |
	    FBINFO_HWACCEL_IMAGEBLIT | FBINFO_HWACCEL_XPAN |
	    FBINFO_HWACCEL_YPAN;
	info->fbops = &davincifb_ops;
	info->screen_size = fb_size;
	info->pseudo_palette = win->pseudo_palette;
	if (fb_alloc_cmap(&info->cmap, 256, 0) < 0) {
		dev_err(dev, "%s: Can't allocate color map.\n", name);
		err = -ENODEV;
		goto cmap_out;
	}

	/* initialize fb_fix_screeninfo */
	strlcpy(info->fix.id, name, sizeof(info->fix.id));
	info->fix.smem_len = fb_size;
	info->fix.type = FB_TYPE_PACKED_PIXELS;

	/* allocate the framebuffer */
	info->screen_base =
	    dma_alloc_coherent(dev, info->fix.smem_len,
			       (dma_addr_t *) & info->fix.smem_start,
			       GFP_KERNEL | GFP_DMA);
	if (!info->screen_base) {
		dev_err(dev, "%s: dma_alloc_coherent failed when allocating "
			"framebuffer.\n", name);
		err = -ENOMEM;
		goto fb_alloc_out;
	}

	/*
	 * Fill the framebuffer with zeros unless it is an OSD1 window in
	 * attribute mode, in which case we fill it with 0x77 to make the OSD0
	 * pixels opaque.
	 */
	memset(info->screen_base,
	       (lconfig->pixfmt == PIXFMT_OSD_ATTR) ? 0x77 : 0,
	       info->fix.smem_len);

	/* initialize fb_var_screeninfo */
	construct_fb_var(&info->var, &win->dm->mode, lconfig);
	win->xpos = lconfig->xpos;
	win->ypos = lconfig->ypos;
	info->var.xres_virtual = info->var.xres;
	info->var.yres_virtual = info->var.yres;

	/* update the fix info to be consistent with the var */
	update_fix_info(&info->var, &info->fix);

	/*
	 * Request ownership of the window from the OSD manager unless this is
	 * a video window and the window size is 0.
	 */
	if (is_osd_win(info) || (info->var.xres != 0 && info->var.yres != 0)) {
		if (!davinci_disp_request_layer(win->layer))
			win->own_window = 1;
	}
	/* bail out if this is an OSD window and we don't own it */
	if (is_osd_win(info) && !win->own_window) {
		dev_err(dev, "%s: Failed to obtain ownership of OSD "
			"window.\n", name);
		err = -ENODEV;
		goto own_out;
	}

	win->display_window = 1;

	if (win->own_window) {
		/* check if our initial window configuration is valid */
		if (info->fbops->fb_check_var(&info->var, info)) {
			dev_warn(dev, "%s: Initial window configuration is "
				 "invalid.\n", name);
		} else
			info->fbops->fb_set_par(info);
	}

	/* register the framebuffer */
	if (register_framebuffer(info)) {
		dev_err(dev, "%s: Failed to register framebuffer.\n", name);
		err = -ENODEV;
		goto register_out;
	}

	dev_info(dev, "%s: %dx%dx%d@%d,%d with framebuffer size %dKB\n",
		 info->fix.id, info->var.xres, info->var.yres,
		 info->var.bits_per_pixel, win->xpos, win->ypos,
		 info->fix.smem_len >> 10);

	return 0;

      register_out:
	if (win->own_window)
		davinci_disp_release_layer(win->layer);
	win->own_window = 0;
      own_out:
	dma_free_coherent(dev, info->fix.smem_len, info->screen_base,
			  info->fix.smem_start);
      fb_alloc_out:
	fb_dealloc_cmap(&info->cmap);
      cmap_out:
	kfree(info);

	return err;
}

static int davincifb_remove(struct device *dev)
{
//	struct device *dev = &pdev->dev;
	struct vpbe_dm_info *dm = dev_get_drvdata(dev);

	dev_set_drvdata(dev, NULL);

	davinci_disp_unregister_callback(&dm->vsync_callback);

	davincifb_release_window(dev, &dm->win[WIN_VID1]);
	davincifb_release_window(dev, &dm->win[WIN_OSD1]);
	davincifb_release_window(dev, &dm->win[WIN_VID0]);
	davincifb_release_window(dev, &dm->win[WIN_OSD0]);

	kfree(dm);

	return 0;
}

/*
 * Return the maximum number of bytes per screen for a display layer at a
 * resolution specified by an fb_videomode struct.
 */
static unsigned davincifb_max_screen_size(enum davinci_disp_layer layer,
					  const struct fb_videomode *mode)
{
	unsigned max_bpp = 32;
	unsigned line_length;
	unsigned size;

	switch (layer) {
	case WIN_OSD0:
	case WIN_OSD1:
		if (cpu_is_davinci_dm355())
			max_bpp = 32;
		else
			max_bpp = 16;
		break;
	case WIN_VID0:
	case WIN_VID1:
		if (cpu_is_davinci_dm355())
			max_bpp = 16;
		else
			max_bpp = 24;
		break;
	}

	line_length = (mode->xres * max_bpp + 7) / 8;
	line_length = ((line_length + 31) / 32) * 32;
	size = mode->yres * line_length;

	return size;
}

static void parse_win_params(struct vpbe_dm_win_info *win,
			     struct davinci_layer_config *lconfig,
			     unsigned *fb_size, char *opt)
{
	char *s, *p, c = 0;
	unsigned bits_per_pixel;

	if (!opt)
		return;

	/* xsize */
	p = strpbrk(opt, "x,@");
	if (p)
		c = *p;
	if ((s = strsep(&opt, "x,@")) == NULL)
		return;
	if (*s)
		lconfig->xsize = simple_strtoul(s, NULL, 0);
	if (!p || !opt)
		return;

	/* ysize */
	if (c == 'x') {
		p = strpbrk(opt, "x,@");
		if (p)
			c = *p;
		if ((s = strsep(&opt, "x,@")) == NULL)
			return;
		if (*s)
			lconfig->ysize = simple_strtoul(s, NULL, 0);
		if (!p || !opt)
			return;
	}

	/* bits per pixel */
	if (c == 'x') {
		p = strpbrk(opt, ",@");
		if (p)
			c = *p;
		if ((s = strsep(&opt, ",@")) == NULL)
			return;
		if (*s) {
			bits_per_pixel = simple_strtoul(s, NULL, 0);
			switch (bits_per_pixel) {
			case 1:
				if (win->layer == WIN_OSD0
				    || win->layer == WIN_OSD1)
					lconfig->pixfmt = PIXFMT_1BPP;
				break;
			case 2:
				if (win->layer == WIN_OSD0
				    || win->layer == WIN_OSD1)
					lconfig->pixfmt = PIXFMT_2BPP;
				break;
			case 4:
				if (win->layer == WIN_OSD0
				    || win->layer == WIN_OSD1)
					lconfig->pixfmt = PIXFMT_4BPP;
				break;
			case 8:
				if (win->layer == WIN_OSD0
				    || win->layer == WIN_OSD1)
					lconfig->pixfmt = PIXFMT_8BPP;
				if (cpu_is_davinci_dm365())
					if (win->layer == WIN_VID0 ||
					    win->layer == WIN_VID1)
						lconfig->pixfmt = PIXFMT_NV12;
				break;
			case 16:
				if (win->layer == WIN_OSD0
				    || win->layer == WIN_OSD1)
					lconfig->pixfmt = PIXFMT_RGB565;
				else
					lconfig->pixfmt = win->dm->yc_pixfmt;
				break;
			case 24:
				if (cpu_is_davinci_dm644x()
				    && (win->layer == WIN_VID0
					|| win->layer == WIN_VID1))
					lconfig->pixfmt = PIXFMT_RGB888;
				break;
			case 32:
				if (cpu_is_davinci_dm355()
				    && (win->layer == WIN_OSD0
					|| win->layer == WIN_OSD1))
					lconfig->pixfmt = PIXFMT_RGB888;
				break;
			default:
				break;
			}
		}
		if (!p || !opt)
			return;
	}

	/* framebuffer size */
	if (c == ',') {
		p = strpbrk(opt, "@");
		if (p)
			c = *p;
		if ((s = strsep(&opt, "@")) == NULL)
			return;
		if (*s) {
			*fb_size = simple_strtoul(s, &s, 0);
			if (*s == 'K')
				*fb_size <<= 10;
			if (*s == 'M')
				*fb_size <<= 20;
		}
		if (!p || !opt)
			return;
	}

	/* xpos */
	if (c == '@') {
		p = strpbrk(opt, ",");
		if (p)
			c = *p;
		if ((s = strsep(&opt, ",")) == NULL)
			return;
		if (*s)
			lconfig->xpos = simple_strtoul(s, NULL, 0);
		if (!p || !opt)
			return;
	}

	/* ypos */
	if (c == ',') {
		s = opt;
		if (*s)
			lconfig->ypos = simple_strtoul(s, NULL, 0);
	}

	return;
}

/*
 * Pass boot-time options by adding the following string to the boot params:
 *	video=davincifb:options
 * Valid options:
 *	osd0=[MxNxP,S@X,Y]
 *      osd1=[MxNxP,S@X,Y]
 *	vid0=[off|MxNxP,S@X,Y]
 *	vid1=[off|MxNxP,S@X,Y]
 *		MxN are the horizontal and vertical window size
 *		P is the color depth (bits per pixel)
 *		S is the framebuffer size with a size suffix such as 'K' or 'M'
 *		X,Y are the window position
 *
 * Only video windows can be turned off.  Turning off a video window means that
 * no framebuffer device will be registered for it,
 *
 * To cause a window to be supported by the framebuffer driver but not displayed
 * initially, pass a value of 0 for the window size.
 *
 * For example:
 *      video=davincifb:osd0=720x480x16@0,0:osd1=720x480:vid0=off:vid1=off
 *
 * This routine returns 1 if the window is to be turned off, or 0 otherwise.
 */
static int davincifb_get_default_win_config(struct device *dev,
					    struct vpbe_dm_win_info *win,
					    struct davinci_layer_config
					    *lconfig, unsigned *fb_size,
					    const char *options)
{
	const char *win_names[] = { "osd0=", "vid0=", "osd1=", "vid1=" };
	const char *this_opt, *next_opt;
	int this_len, opt_len;
	static char opt_buf[128];

	/* supply default values for lconfig and fb_size */
	switch (win->layer) {
	case WIN_OSD0:
		lconfig->pixfmt = PIXFMT_RGB565;
		lconfig->xsize = win->dm->mode.xres;
		lconfig->ysize = win->dm->mode.yres;
		break;
	case WIN_OSD1:
		lconfig->pixfmt = PIXFMT_OSD_ATTR;
		lconfig->xsize = win->dm->mode.xres;
		lconfig->ysize = win->dm->mode.yres;
		break;
	case WIN_VID0:
	case WIN_VID1:
		lconfig->pixfmt = win->dm->yc_pixfmt;
		lconfig->xsize = 0;
		lconfig->ysize = 0;
		break;
	}
	lconfig->xpos = 0;
	lconfig->ypos = 0;

	lconfig->interlaced = is_display_interlaced(&win->dm->mode);
	*fb_size = davincifb_max_screen_size(win->layer, &win->dm->mode);

	next_opt = options;
	while ((this_opt = next_opt)) {
		this_len = strcspn(this_opt, ":");
		next_opt = strpbrk(this_opt, ":");
		if (next_opt)
			++next_opt;

		opt_len = strlen(win_names[win->layer]);
		if (this_len >= opt_len) {
			if (strncmp(this_opt, win_names[win->layer], opt_len))
				continue;
			this_len -= opt_len;
			this_opt += opt_len;
			if ((this_len >= strlen("off"))
			    && !strncmp(this_opt, "off", strlen("off")))
				return 1;
			else {
				strlcpy(opt_buf, this_opt,
					min_t(int, sizeof(opt_buf),
					      this_len + 1));
				parse_win_params(win, lconfig, fb_size,
						 opt_buf);
				return 0;
			}
		}
	}

	return 0;
}

/*
 *     Module parameter definitions
 */
static char *options = "";

module_param(options, charp, S_IRUGO);

static int davincifb_probe(struct device *dev)
{
	struct vpbe_dm_info *dm;
	struct davinci_layer_config lconfig;
	unsigned fb_size;
	int err;
	struct davincifb_platform_data *pdata = dev->platform_data;

	if (!pdata)
		pdata = &davincifb_pdata_default;

	dm = kzalloc(sizeof(*dm), GFP_KERNEL);
	if (!dm) {
		dev_err(dev, "Can't allocate memory for driver state.\n");
		return -ENOMEM;
	}
	dev_set_drvdata(dev, dm);

	/* get the video mode from the encoder manager */
	get_video_mode(&dm->mode);

	/* set the default Cb/Cr order */
	dm->yc_pixfmt = PIXFMT_YCbCrI;

	/* initialize OSD0 */
	dm->win[WIN_OSD0].layer = WIN_OSD0;
	dm->win[WIN_OSD0].dm = dm;
	dm->win[WIN_OSD0].sdram_address = 0;
	davincifb_get_default_win_config(dev, &dm->win[WIN_OSD0], &lconfig,
					 &fb_size, options);
	err =
	    davincifb_init_window(dev, &dm->win[WIN_OSD0], &lconfig, fb_size,
				  OSD0_FBNAME);
	if (err)
		goto osd0_out;

	/* initialize VID0 */
	dm->win[WIN_VID0].layer = WIN_VID0;
	dm->win[WIN_VID0].dm = dm;
	dm->win[WIN_VID0].sdram_address = 0;
	if (!davincifb_get_default_win_config
	    (dev, &dm->win[WIN_VID0], &lconfig, &fb_size, options)) {
		err =
		    davincifb_init_window(dev, &dm->win[WIN_VID0], &lconfig,
					  fb_size, VID0_FBNAME);
		if (err)
			goto vid0_out;
	}

	/* initialize OSD1 */
	dm->win[WIN_OSD1].layer = WIN_OSD1;
	dm->win[WIN_OSD1].dm = dm;
	dm->win[WIN_OSD1].sdram_address = 0;
	davincifb_get_default_win_config(dev, &dm->win[WIN_OSD1], &lconfig,
					 &fb_size, options);
	err =
	    davincifb_init_window(dev, &dm->win[WIN_OSD1], &lconfig, fb_size,
				  OSD1_FBNAME);
	if (err)
		goto osd1_out;

	/* initialize VID1 */
	dm->win[WIN_VID1].layer = WIN_VID1;
	dm->win[WIN_VID1].dm = dm;
	dm->win[WIN_VID1].sdram_address = 0;
	if (!davincifb_get_default_win_config
	    (dev, &dm->win[WIN_VID1], &lconfig, &fb_size, options)) {
		err =
		    davincifb_init_window(dev, &dm->win[WIN_VID1], &lconfig,
					  fb_size, VID1_FBNAME);
		if (err)
			goto vid1_out;
	}

	/* initialize the vsync wait queue */
	init_waitqueue_head(&dm->vsync_wait);
	dm->timeout = HZ / 5;

	/* register the end-of-frame callback */
	dm->vsync_callback.mask = DAVINCI_DISP_FIRST_FIELD |
	    DAVINCI_DISP_SECOND_FIELD | DAVINCI_DISP_END_OF_FRAME;

	dm->vsync_callback.handler = davincifb_vsync_callback;
	dm->vsync_callback.arg = dm;
	davinci_disp_register_callback(&dm->vsync_callback);

	fb->invert_field = pdata->invert_field;

	return 0;

      vid1_out:
	davincifb_release_window(dev, &dm->win[WIN_OSD1]);
      osd1_out:
	davincifb_release_window(dev, &dm->win[WIN_VID0]);
      vid0_out:
	davincifb_release_window(dev, &dm->win[WIN_OSD0]);
      osd0_out:
	kfree(dm);

	return err;
}

static void davincifb_release_dev(struct device *dev)
{
}

static u64 davincifb_dmamask = ~(u32) 0;

/* FIXME: move to board setup file */
static struct platform_device davincifb_device = {
	.name = "davincifb",
	.id = 0,
	.dev = {
		.release = davincifb_release_dev,
		.dma_mask = &davincifb_dmamask,
		.coherent_dma_mask = 0xffffffff,
		},
	.num_resources = 0,
};

static struct device_driver davincifb_driver = {
	.name = "davincifb",
	.bus = &platform_bus_type,
	.probe = davincifb_probe,
	.remove = davincifb_remove,
	.suspend = NULL,
	.resume = NULL,
};
#if 0
static struct platform_driver davincifb_driver = {
	.probe = davincifb_probe,
	.remove = davincifb_remove,
	.driver = {
			.name = "davincifb",
			.owner = THIS_MODULE,
		},
};
#endif
static int __init davincifb_init(void)
{
	struct device *dev = &davincifb_device.dev;
#ifndef MODULE
	{
		char *names[] = { "davincifb", "dm64xxfb", "dm355fb" };
		int i, num_names = 3;

		for (i = 0; i < num_names; i++) {
			if (fb_get_options(names[i], &options)) {
				dev_err(dev, " Disabled on command-line.\n");
				return -ENODEV;
			}
			if (options)
				break;
		}
	}
#endif

	/* Register the device with LDM */
	if (platform_device_register(&davincifb_device)) {
		pr_debug("failed to register davincifb device\n");
		return -ENODEV;
	}

	/* Register the driver with LDM */
	if (driver_register(&davincifb_driver)) {
		dev_err(dev, "failed to register davincifb driver\n");
		platform_device_unregister(&davincifb_device);
		return -ENODEV;
	}

	return 0;
}

static void __exit davincifb_cleanup(void)
{
	driver_unregister(&davincifb_driver);
	platform_device_unregister(&davincifb_device);
}

module_init(davincifb_init);
module_exit(davincifb_cleanup);

MODULE_DESCRIPTION("Framebuffer driver for TI DaVinci");
MODULE_AUTHOR("MontaVista Software");
MODULE_LICENSE("GPL");
