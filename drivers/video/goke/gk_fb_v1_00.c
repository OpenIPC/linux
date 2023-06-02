/*
 * drivers/video/goke/gk_fb_v1_00.c
 *
 * Author: Steven Yu, <yulindeng@gokemicro.com>
 * Copyright (C) 2012-2015, goke, Inc.
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

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/string.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/mm.h>
#include <linux/fb.h>
#include <linux/init.h>
#include <linux/ioport.h>
#include <linux/list.h>
#include <linux/amba/bus.h>
#include <linux/amba/clcd.h>
#include <linux/clk.h>
#include <linux/proc_fs.h>
#include <linux/platform_device.h>

#include <asm/sizes.h>
#include <asm/io.h>
#include <asm/uaccess.h>
#include <asm/page.h>
#include <asm/pgtable.h>

#include <plat/fb.h>
#include CONFIG_GK_CHIP_INCLUDE_FILE

/* video=gk_fb0:<x_res>x<y_res>,<x_virtual>x<y_virtual>,
   <color_format>,<conversion_buffer>[,<prealloc_start>,<prealloc_length>] */

static const struct gk_fb_color_table_s {
	enum gk_fb_color_format	color_format;
	int				bits_per_pixel;
	struct fb_bitfield		red;
	struct fb_bitfield		green;
	struct fb_bitfield		blue;
	struct fb_bitfield		transp;
} gk_fb_color_format_table[] =
{
	{
		.color_format = GK_FB_COLOR_CLUT_8BPP,
		.bits_per_pixel = 8,
		.red =
			{
				.offset = 0,
				.length = 0,
				.msb_right = 0,
			},
		.green =
			{
				.offset = 0,
				.length = 0,
				.msb_right = 0,
			},
		.blue =
			{
				.offset = 0,
				.length = 0,
				.msb_right = 0,
			},
		.transp =
			{
				.offset = 0,
				.length = 0,
				.msb_right = 0,
		},
	},

	{
		.color_format = GK_FB_COLOR_RGB565,
		.bits_per_pixel = 16,
		.red =
			{
				.offset = 11,
				.length = 5,
				.msb_right = 0,
			},
		.green =
			{
				.offset = 5,
				.length = 6,
				.msb_right = 0,
			},
		.blue =
			{
				.offset = 0,
				.length = 5,
				.msb_right = 0,
			},
		.transp =
			{
				.offset = 0,
				.length = 0,
				.msb_right = 0,
		},
	},

	{
		.color_format = GK_FB_COLOR_BGR565,
		.bits_per_pixel = 16,
		.red =
			{
				.offset = 0,
				.length = 5,
				.msb_right = 0,
			},
		.green =
			{
				.offset = 5,
				.length = 6,
				.msb_right = 0,
			},
		.blue =
			{
				.offset = 11,
				.length = 5,
				.msb_right = 0,
			},
		.transp =
			{
				.offset = 0,
				.length = 0,
				.msb_right = 0,
		},
	},

	{
		.color_format = GK_FB_COLOR_AGBR4444,
		.bits_per_pixel = 16,
		.red =
			{
				.offset = 0,
				.length = 4,
				.msb_right = 0,
			},
		.green =
			{
				.offset = 8,
				.length = 4,
				.msb_right = 0,
			},
		.blue =
			{
				.offset = 4,
				.length = 4,
				.msb_right = 0,
			},
		.transp =
			{
				.offset = 12,
				.length = 4,
				.msb_right = 0,
		},
	},

	{
		.color_format = GK_FB_COLOR_RGBA4444,
		.bits_per_pixel = 16,
		.red =
			{
				.offset = 12,
				.length = 4,
				.msb_right = 0,
			},
		.green =
			{
				.offset = 8,
				.length = 4,
				.msb_right = 0,
			},
		.blue =
			{
				.offset = 4,
				.length = 4,
				.msb_right = 0,
			},
		.transp =
			{
				.offset = 0,
				.length = 4,
				.msb_right = 0,
		},
	},

	{
		.color_format = GK_FB_COLOR_BGRA4444,
		.bits_per_pixel = 16,
		.red =
			{
				.offset = 4,
				.length = 4,
				.msb_right = 0,
			},
		.green =
			{
				.offset = 8,
				.length = 4,
				.msb_right = 0,
			},
		.blue =
			{
				.offset = 12,
				.length = 4,
				.msb_right = 0,
			},
		.transp =
			{
				.offset = 0,
				.length = 4,
				.msb_right = 0,
		},
	},

	{
		.color_format = GK_FB_COLOR_ABGR4444,
		.bits_per_pixel = 16,
		.red =
			{
				.offset = 0,
				.length = 4,
				.msb_right = 0,
			},
		.green =
			{
				.offset = 4,
				.length = 4,
				.msb_right = 0,
			},
		.blue =
			{
				.offset = 8,
				.length = 4,
				.msb_right = 0,
			},
		.transp =
			{
				.offset = 12,
				.length = 4,
				.msb_right = 0,
		},
	},

	{
		.color_format = GK_FB_COLOR_ARGB4444,
		.bits_per_pixel = 16,
		.red =
			{
				.offset = 8,
				.length = 4,
				.msb_right = 0,
			},
		.green =
			{
				.offset = 4,
				.length = 4,
				.msb_right = 0,
			},
		.blue =
			{
				.offset = 0,
				.length = 4,
				.msb_right = 0,
			},
		.transp =
			{
				.offset = 12,
				.length = 4,
				.msb_right = 0,
		},
	},

	{
		.color_format = GK_FB_COLOR_AGBR1555,
		.bits_per_pixel = 16,
		.red =
			{
				.offset = 0,
				.length = 5,
				.msb_right = 0,
			},
		.green =
			{
				.offset = 10,
				.length = 5,
				.msb_right = 0,
			},
		.blue =
			{
				.offset = 5,
				.length = 5,
				.msb_right = 0,
			},
		.transp =
			{
				.offset = 15,
				.length = 1,
				.msb_right = 0,
		},
	},

	{
		.color_format = GK_FB_COLOR_GBR1555,
		.bits_per_pixel = 16,
		.red =
			{
				.offset = 0,
				.length = 5,
				.msb_right = 0,
			},
		.green =
			{
				.offset = 10,
				.length = 5,
				.msb_right = 0,
			},
		.blue =
			{
				.offset = 5,
				.length = 5,
				.msb_right = 0,
			},
		.transp =
			{
				.offset = 0,
				.length = 0,
				.msb_right = 0,
		},
	},

	{
		.color_format = GK_FB_COLOR_RGBA5551,
		.bits_per_pixel = 16,
		.red =
			{
				.offset = 11,
				.length = 5,
				.msb_right = 0,
			},
		.green =
			{
				.offset = 6,
				.length = 5,
				.msb_right = 0,
			},
		.blue =
			{
				.offset = 1,
				.length = 5,
				.msb_right = 0,
			},
		.transp =
			{
				.offset = 0,
				.length = 1,
				.msb_right = 0,
		},
	},

	{
		.color_format = GK_FB_COLOR_BGRA5551,
		.bits_per_pixel = 16,
		.red =
			{
				.offset = 1,
				.length = 5,
				.msb_right = 0,
			},
		.green =
			{
				.offset = 6,
				.length = 5,
				.msb_right = 0,
			},
		.blue =
			{
				.offset = 11,
				.length = 5,
				.msb_right = 0,
			},
		.transp =
			{
				.offset = 0,
				.length = 1,
				.msb_right = 0,
		},
	},

	{
		.color_format = GK_FB_COLOR_ABGR1555,
		.bits_per_pixel = 16,
		.red =
			{
				.offset = 0,
				.length = 5,
				.msb_right = 0,
			},
		.green =
			{
				.offset = 5,
				.length = 5,
				.msb_right = 0,
			},
		.blue =
			{
				.offset = 10,
				.length = 5,
				.msb_right = 0,
			},
		.transp =
			{
				.offset = 15,
				.length = 1,
				.msb_right = 0,
		},
	},

	{
		.color_format = GK_FB_COLOR_ARGB1555,
		.bits_per_pixel = 16,
		.red =
			{
				.offset = 10,
				.length = 5,
				.msb_right = 0,
			},
		.green =
			{
				.offset = 5,
				.length = 5,
				.msb_right = 0,
			},
		.blue =
			{
				.offset = 0,
				.length = 5,
				.msb_right = 0,
			},
		.transp =
			{
				.offset = 15,
				.length = 1,
				.msb_right = 0,
		},
	},

	{
		.color_format = GK_FB_COLOR_AGBR8888,
		.bits_per_pixel = 32,
		.red =
			{
				.offset = 0,
				.length = 8,
				.msb_right = 0,
			},
		.green =
			{
				.offset = 16,
				.length = 8,
				.msb_right = 0,
			},
		.blue =
			{
				.offset = 8,
				.length = 8,
				.msb_right = 0,
			},
		.transp =
			{
				.offset = 24,
				.length = 8,
				.msb_right = 0,
		},
	},

	{
		.color_format = GK_FB_COLOR_RGBA8888,
		.bits_per_pixel = 32,
		.red =
			{
				.offset = 24,
				.length = 8,
				.msb_right = 0,
			},
		.green =
			{
				.offset = 16,
				.length = 8,
				.msb_right = 0,
			},
		.blue =
			{
				.offset = 8,
				.length = 8,
				.msb_right = 0,
			},
		.transp =
			{
				.offset = 0,
				.length = 8,
				.msb_right = 0,
		},
	},

	{
		.color_format = GK_FB_COLOR_BGRA8888,
		.bits_per_pixel = 32,
		.red =
			{
				.offset = 8,
				.length = 8,
				.msb_right = 0,
			},
		.green =
			{
				.offset = 16,
				.length = 8,
				.msb_right = 0,
			},
		.blue =
			{
				.offset = 24,
				.length = 8,
				.msb_right = 0,
			},
		.transp =
			{
				.offset = 0,
				.length = 8,
				.msb_right = 0,
		},
	},

	{
		.color_format = GK_FB_COLOR_ABGR8888,
		.bits_per_pixel = 32,
		.red =
			{
				.offset = 0,
				.length = 8,
				.msb_right = 0,
			},
		.green =
			{
				.offset = 8,
				.length = 8,
				.msb_right = 0,
			},
		.blue =
			{
				.offset = 16,
				.length = 8,
				.msb_right = 0,
			},
		.transp =
			{
				.offset = 24,
				.length = 8,
				.msb_right = 0,
		},
	},

	{
		.color_format = GK_FB_COLOR_ARGB8888,
		.bits_per_pixel = 32,
		.red =
			{
				.offset = 16,
				.length = 8,
				.msb_right = 0,
			},
		.green =
			{
				.offset = 8,
				.length = 8,
				.msb_right = 0,
			},
		.blue =
			{
				.offset = 0,
				.length = 8,
				.msb_right = 0,
			},
		.transp =
			{
				.offset = 24,
				.length = 8,
				.msb_right = 0,
		},
	},
};

/* ========================================================================== */
static int gk_fb_proc_read(char *page, char **start,
	off_t off, int count, int *eof, void *data)
{
	int					len;
	struct gk_platform_fb		*gk_fb_data;
	struct fb_info				*info;

	gk_fb_data = (struct gk_platform_fb *)data;
	info = gk_fb_data->proc_fb_info;

	*start = page + off;
	wait_event_interruptible(gk_fb_data->proc_wait,
		(gk_fb_data->proc_wait_flag > 0));
	gk_fb_data->proc_wait_flag = 0;
	dev_dbg(info->device, "%s:%d %d.\n", __func__, __LINE__,
		gk_fb_data->proc_wait_flag);
	len = sprintf(*start, "%04d %04d %04d %04d %04d %04d\n",
		info->var.xres, info->var.yres,
		info->fix.line_length,
		info->var.xoffset, info->var.yoffset,
		gk_fb_data->color_format);

	if (off + len > count) {
		*eof = 1;
		len = 0;
	}

	return len;
}

/* ========================================================================== */
static int gk_fb_setcmap(struct fb_cmap *cmap, struct fb_info *info)
{
	int					errorCode = 0;
	struct gk_platform_fb		*gk_fb_data;
	int					pos;
	u16					*r, *g, *b, *t;
	u8					*pclut_table, *pblend_table;

	if (cmap == &info->cmap) {
		errorCode = 0;
		goto gk_fb_setcmap_exit;
	}

	if (cmap->start != 0 || cmap->len != 256) {
		dev_dbg(info->device,
			"%s: Incorrect parameters: start = %d, len = %d\n",
			__func__, cmap->start, cmap->len);
		errorCode = -1;
		goto gk_fb_setcmap_exit;
	}

	if (!cmap->red || !cmap->green || !cmap->blue) {
		dev_dbg(info->device, "%s: Incorrect rgb pointers!\n",
			__func__);
		errorCode = -1;
		goto gk_fb_setcmap_exit;
	}

	gk_fb_data = (struct gk_platform_fb *)info->par;

	mutex_lock(&gk_fb_data->lock);

	r = cmap->red;
	g = cmap->green;
	b = cmap->blue;
	t = cmap->transp;
	pclut_table = gk_fb_data->clut_table;
	pblend_table = gk_fb_data->blend_table;
	for (pos = 0; pos < 256; pos++) {
		*pclut_table++ = *r++;
		*pclut_table++ = *g++;
		*pclut_table++ = *b++;
		if (t) *pblend_table++ = *t++;
	}

	if (gk_fb_data->setcmap) {
		mutex_unlock(&gk_fb_data->lock);
		errorCode = gk_fb_data->setcmap(cmap, info);
	} else {
		mutex_unlock(&gk_fb_data->lock);
	}

gk_fb_setcmap_exit:
	dev_dbg(info->device, "%s:%d %d.\n", __func__, __LINE__, errorCode);
	return errorCode;
}

static int gk_fb_check_var(struct fb_var_screeninfo *var, struct fb_info *info)
{
	int					errorCode = 0;
	struct gk_platform_fb		*gk_fb_data;
	u32					framesize = 0;

	gk_fb_data = (struct gk_platform_fb *)info->par;

	mutex_lock(&gk_fb_data->lock);
	if (gk_fb_data->check_var) {
		errorCode = gk_fb_data->check_var(var, info);
	}
	mutex_unlock(&gk_fb_data->lock);

	if (var->xres_virtual * var->bits_per_pixel / 8 > info->fix.line_length) {
		errorCode = -ENOMEM;
		dev_err(info->device, "%s: xres_virtual[%d] too big [%d]!\n",
			__func__, var->xres_virtual * var->bits_per_pixel / 8,
			info->fix.line_length);
	}

	framesize = info->fix.line_length * var->yres_virtual;
	if (framesize > info->fix.smem_len) {
		errorCode = -ENOMEM;
		dev_err(info->device, "%s: framesize[%d] too big [%d]!\n",
			__func__, framesize, info->fix.smem_len);
	}

	dev_dbg(info->device, "%s:%d %d.\n", __func__, __LINE__, errorCode);

	return errorCode;
}

static int gk_fb_set_par(struct fb_info *info)
{
	int					errorCode = 0, i;
	struct gk_platform_fb		*gk_fb_data;
	struct fb_var_screeninfo		*pvar;
	static struct fb_var_screeninfo		*cur_var;
	int					res_changed = 0;
	int					color_format_changed = 0;
	enum gk_fb_color_format		new_color_format;
	const struct gk_fb_color_table_s	*pTable;

	gk_fb_data = (struct gk_platform_fb *)info->par;
	mutex_lock(&gk_fb_data->lock);
	cur_var = &gk_fb_data->screen_var;
	pvar = &info->var;

	if (!gk_fb_data->set_par)
		goto gkfb_set_par_quick_exit;

	/* Resolution changed */
	if (pvar->xres != cur_var->xres || pvar->yres != cur_var->yres) {
		res_changed = 1;
	}

	/* Color format changed */
	if (pvar->bits_per_pixel != cur_var->bits_per_pixel ||
		pvar->red.offset != cur_var->red.offset ||
		pvar->red.length != cur_var->red.length ||
		pvar->red.msb_right != cur_var->red.msb_right ||
		pvar->green.offset != cur_var->green.offset ||
		pvar->green.length != cur_var->green.length ||
		pvar->green.msb_right != cur_var->green.msb_right ||
		pvar->blue.offset != cur_var->blue.offset ||
		pvar->blue.length != cur_var->blue.length ||
		pvar->blue.msb_right != cur_var->blue.msb_right ||
		pvar->transp.offset != cur_var->transp.offset ||
		pvar->transp.length != cur_var->transp.length ||
		pvar->transp.msb_right != cur_var->transp.msb_right) {

		color_format_changed = 1;
	}

	if (!res_changed && !color_format_changed)
		goto gkfb_set_par_quick_exit;

	/* Find color format */
	new_color_format = gk_fb_data->color_format;
	pTable = gk_fb_color_format_table;
	for (i = 0; i < ARRAY_SIZE(gk_fb_color_format_table); i++) {
		if (pTable->bits_per_pixel == pvar->bits_per_pixel &&
			pTable->red.offset == pvar->red.offset &&
			pTable->red.length == pvar->red.length &&
			pTable->red.msb_right == pvar->red.msb_right &&
			pTable->green.offset == pvar->green.offset &&
			pTable->green.length == pvar->green.length &&
			pTable->green.msb_right == pvar->green.msb_right &&
			pTable->blue.offset == pvar->blue.offset &&
			pTable->blue.length == pvar->blue.length &&
			pTable->blue.msb_right == pvar->blue.msb_right &&
			pTable->transp.offset == pvar->transp.offset &&
			pTable->transp.length == pvar->transp.length &&
			pTable->transp.msb_right == pvar->transp.msb_right) {

			new_color_format = pTable->color_format;
			break;
		}
		pTable++;
	}

	gk_fb_data->color_format = new_color_format;
	memcpy(cur_var, pvar, sizeof(*pvar));
	mutex_unlock(&gk_fb_data->lock);

	errorCode = gk_fb_data->set_par(info);
	goto gkfb_set_par_normal_exit;

gkfb_set_par_quick_exit:
	memcpy(cur_var, pvar, sizeof(*pvar));
	mutex_unlock(&gk_fb_data->lock);

gkfb_set_par_normal_exit:
	dev_dbg(info->device, "%s:%d %d.\n", __func__, __LINE__, errorCode);

	return errorCode;
}

static int gk_fb_pan_display(struct fb_var_screeninfo *var,
	struct fb_info *info)
{
	int					errorCode = 0;
	struct gk_platform_fb		*gk_fb_data;

	gk_fb_data = (struct gk_platform_fb *)info->par;

	gk_fb_data->proc_wait_flag++;
	wake_up(&(gk_fb_data->proc_wait));
	mutex_lock(&gk_fb_data->lock);
	gk_fb_data->screen_var.xoffset = var->xoffset;
	gk_fb_data->screen_var.yoffset = var->yoffset;
	if (gk_fb_data->pan_display) {
		errorCode = gk_fb_data->pan_display(var, info);
	}
	mutex_unlock(&gk_fb_data->lock);

	dev_dbg(info->device, "%s:%d %d.\n", __func__, __LINE__, errorCode);

	return 0;
}

static int gk_fb_mmap(struct fb_info *info, struct vm_area_struct *vma)
{
	unsigned long size = vma->vm_end - vma->vm_start;
	unsigned long offset = vma->vm_pgoff << PAGE_SHIFT;

	if (offset + size > info->fix.smem_len)
		return -EINVAL;

	offset += info->fix.smem_start;
	if (remap_pfn_range(vma, vma->vm_start, offset >> PAGE_SHIFT,
			    size, vma->vm_page_prot))
		return -EAGAIN;

	dev_dbg(info->device, "%s: P(0x%08lx)->V(0x%08lx), size = 0x%08lx.\n",
		__func__, offset, vma->vm_start, size);

	return 0;
}

static int gk_fb_blank(int blank_mode, struct fb_info *info)
{
	int					errorCode = 0;
	struct gk_platform_fb		*gk_fb_data;

	gk_fb_data = (struct gk_platform_fb *)info->par;

	mutex_lock(&gk_fb_data->lock);
	if (gk_fb_data->set_blank) {
		errorCode = gk_fb_data->set_blank(blank_mode, info);
	}
	mutex_unlock(&gk_fb_data->lock);

	dev_dbg(info->device, "%s:%d %d.\n", __func__, __LINE__, errorCode);

	return errorCode;
}

static struct fb_ops gk_fb_ops = {
	.owner          = THIS_MODULE,
	.fb_check_var	= gk_fb_check_var,
	.fb_set_par	= gk_fb_set_par,
	.fb_pan_display	= gk_fb_pan_display,
	.fb_fillrect	= sys_fillrect,
	.fb_copyarea	= sys_copyarea,
	.fb_imageblit	= sys_imageblit,
	.fb_mmap	= gk_fb_mmap,
	.fb_setcmap	= gk_fb_setcmap,
	.fb_blank	= gk_fb_blank,
};

static int __init gk_fb_setup(struct device *dev, char *options,
	struct gk_platform_fb *gk_fb_data)
{
	int					retval = -1;
	int					ssret;
	int					cl_xres;
	int					cl_yres;
	int					cl_xvirtual;
	int					cl_yvirtual;
	int					cl_format;
	int					cl_cvs_buf;
	unsigned int				cl_prealloc_start;
	unsigned int				cl_prealloc_length;

	if (!options || !*options) {
		goto gkfb_setup_exit;
	}

	ssret = sscanf(options, "%dx%d,%dx%d,%d,%d,%x,%x", &cl_xres, &cl_yres,
		&cl_xvirtual, &cl_yvirtual, &cl_format, &cl_cvs_buf,
		&cl_prealloc_start, &cl_prealloc_length);
	if (ssret == 6) {
		gk_fb_data->screen_var.xres = cl_xres;
		gk_fb_data->screen_var.yres = cl_yres;
		gk_fb_data->screen_var.xres_virtual = cl_xvirtual;
		gk_fb_data->screen_var.yres_virtual = cl_yvirtual;
		gk_fb_data->color_format = cl_format;
		gk_fb_data->conversion_buf.available = cl_cvs_buf;
		dev_dbg(dev, "%dx%d,%dx%d,%d,%d\n", cl_xres, cl_yres,
			cl_xvirtual, cl_yvirtual, cl_format, cl_cvs_buf);
		retval = 0;
	} else
	if (ssret == 8) {
		gk_fb_data->screen_var.xres = cl_xres;
		gk_fb_data->screen_var.yres = cl_yres;
		gk_fb_data->screen_var.xres_virtual = cl_xvirtual;
		gk_fb_data->screen_var.yres_virtual = cl_yvirtual;
		gk_fb_data->color_format = cl_format;
		gk_fb_data->conversion_buf.available = cl_cvs_buf;

		gk_fb_data->screen_fix.smem_start = cl_prealloc_start;
		gk_fb_data->screen_fix.smem_len = cl_prealloc_length;
		gk_fb_data->use_prealloc = 1;
		dev_dbg(dev, "%dx%d,%dx%d,%d,%d,%x,%x\n", cl_xres, cl_yres,
			cl_xvirtual, cl_yvirtual, cl_format, cl_cvs_buf,
			cl_prealloc_start, cl_prealloc_length);
		retval = 0;
	} else {
		dev_err(dev, "Can not support %s@%d!\n", options, ssret);
	}

gkfb_setup_exit:
	return retval;
}

static int __devinit gk_fb_probe(struct platform_device *pdev)
{
	int					errorCode = 0;
	struct fb_info				*info;
	char					fb_name[64];
	char					*option;
	struct gk_platform_fb		*gk_fb_data = NULL;
	u32					i;
	u32					framesize;
	u32					line_length;

//printk("gk framebuffer probe...\n");
	gk_fb_data = (struct gk_platform_fb *)pdev->dev.platform_data;
	if (gk_fb_data == NULL) {
		dev_err(&pdev->dev, "%s: gk_fb_data is NULL!\n", __func__);
		errorCode = -ENODEV;
		goto gkfb_probe_exit;
	}
//printk("gk framebuffer use_prealloc ... %d\n", gk_fb_data->use_prealloc);
	if (gk_fb_data->use_prealloc) {
		dev_info(&pdev->dev, "%s: use prealloc.\n", __func__);
	} else {
		snprintf(fb_name, sizeof(fb_name), "gk_fb%d", pdev->id);
		if (fb_get_options(fb_name, &option)) {
			errorCode = -ENODEV;
			printk("gk framebuffer no options...\n");
			goto gkfb_probe_exit;
		}
		//printk("gk framebuffer setup...%s\n", option);
		if (gk_fb_setup(&pdev->dev, option, gk_fb_data)) {
			errorCode = -ENODEV;
			goto gkfb_probe_exit;
		}
	}
//printk("gk framebuffer alloc...\n");
	info = framebuffer_alloc(sizeof(gk_fb_data), &pdev->dev);
	if (info == NULL) {
		dev_err(&pdev->dev, "%s: framebuffer_alloc fail!\n", __func__);
		errorCode = -ENOMEM;
		goto gkfb_probe_exit;
	}

	mutex_lock(&gk_fb_data->lock);

	info->fbops = &gk_fb_ops;
	info->par = gk_fb_data;
	info->var = gk_fb_data->screen_var;
	info->fix = gk_fb_data->screen_fix;
	info->flags = FBINFO_FLAG_DEFAULT;

	/* Fill Color-related Variables */
	for (i = 0; i < ARRAY_SIZE(gk_fb_color_format_table); i++) {
		if (gk_fb_color_format_table[i].color_format ==
			gk_fb_data->color_format)
			break;
	}
	if (i < ARRAY_SIZE(gk_fb_color_format_table)) {
		info->var.bits_per_pixel =
			gk_fb_color_format_table[i].bits_per_pixel;
		info->var.red = gk_fb_color_format_table[i].red;
		info->var.green = gk_fb_color_format_table[i].green;
		info->var.blue = gk_fb_color_format_table[i].blue;
		info->var.transp = gk_fb_color_format_table[i].transp;
	}

	/* Malloc Framebuffer Memory */
	line_length = (info->var.xres_virtual *
		(info->var.bits_per_pixel / 8) + 31) & 0xffffffe0;
	if (gk_fb_data->use_prealloc == 0) {
		info->fix.line_length = line_length;
	} else {
		info->fix.line_length =
			(line_length > gk_fb_data->prealloc_line_length) ?
			line_length : gk_fb_data->prealloc_line_length;
	}
	framesize = info->fix.line_length * info->var.yres_virtual;
	if (framesize % PAGE_SIZE) {
		framesize /= PAGE_SIZE;
		framesize++;
		framesize *= PAGE_SIZE;
	}
	if (gk_fb_data->use_prealloc == 0) {
		info->screen_base = kzalloc(framesize, GFP_KERNEL);
		if (info->screen_base == NULL) {
			dev_err(&pdev->dev, "%s(%d): Can't get fbmem!\n",
				__func__, __LINE__);
			errorCode = -ENOMEM;
			goto gkfb_probe_release_framebuffer;
		}
		if (gk_fb_data->conversion_buf.available) {
			gk_fb_data->conversion_buf.ping_buf =
				kzalloc(framesize, GFP_KERNEL);
			if (gk_fb_data->conversion_buf.ping_buf == NULL) {
				gk_fb_data->conversion_buf.available = 0;
				dev_err(&pdev->dev, "%s(%d): Can't get fbmem!\n",
					__func__, __LINE__);
				errorCode = -ENOMEM;
				goto gkfb_probe_release_framebuffer;
			} else {
				gk_fb_data->conversion_buf.ping_buf_size
					= framesize;
			}

			gk_fb_data->conversion_buf.pong_buf =
				kzalloc(framesize, GFP_KERNEL);
			if (gk_fb_data->conversion_buf.pong_buf == NULL) {
				gk_fb_data->conversion_buf.available = 0;
				dev_err(&pdev->dev, "%s(%d): Can't get fbmem!\n",
					__func__, __LINE__);
				errorCode = -ENOMEM;
				goto gkfb_probe_release_framebuffer;
			} else {
				gk_fb_data->conversion_buf.pong_buf_size
					= framesize;
			}
		}
		info->fix.smem_start = virt_to_phys(info->screen_base);
		info->fix.smem_len = framesize;
	} else {
		if ((info->fix.smem_start == 0) ||
			(info->fix.smem_len < framesize)) {
			dev_err(&pdev->dev, "%s: prealloc[0x%08x < 0x%08x]!\n",
				__func__, info->fix.smem_len, framesize);
			errorCode = -ENOMEM;
			goto gkfb_probe_release_framebuffer;
		}

		info->screen_base = (char __iomem *)
			gk_phys_to_virt(info->fix.smem_start);
		//memset(info->screen_base, 0, info->fix.smem_len);

		if (gk_fb_data->conversion_buf.available) {
			if (info->fix.smem_len < 3 * framesize) {
				gk_fb_data->conversion_buf.available = 0;
				dev_err(&pdev->dev,
					"%s: prealloc[0x%08x < 0x%08x]!\n",
					__func__, info->fix.smem_len,
					3 * framesize);
				errorCode = -ENOMEM;
				goto gkfb_probe_release_framebuffer;
			} else {
				gk_fb_data->conversion_buf.ping_buf =
					info->screen_base + framesize;
				gk_fb_data->conversion_buf.ping_buf_size =
					framesize;
				gk_fb_data->conversion_buf.ping_buf =
					info->screen_base + framesize;
				gk_fb_data->conversion_buf.ping_buf_size =
					framesize;
			}
		}
	}

	errorCode = fb_alloc_cmap(&info->cmap, 256, 1);
	if (errorCode < 0) {
		dev_err(&pdev->dev, "%s: fb_alloc_cmap fail!\n", __func__);
		errorCode = -ENOMEM;
		goto gkfb_probe_release_framebuffer;
	}
	for (i = info->cmap.start; i < info->cmap.len; i++) {
		info->cmap.red[i] =
			gk_fb_data->clut_table[i * GK_CLUT_BYTES + 0];
		info->cmap.red[i] <<= 8;
		info->cmap.green[i] =
			gk_fb_data->clut_table[i * GK_CLUT_BYTES + 1];
		info->cmap.green[i] <<= 8;
		info->cmap.blue[i] =
			gk_fb_data->clut_table[i * GK_CLUT_BYTES + 2];
		info->cmap.blue[i] <<= 8;
		if (info->cmap.transp) {
			info->cmap.transp[i] = gk_fb_data->blend_table[i];
			info->cmap.transp[i] <<= 8;
		}
	}

	platform_set_drvdata(pdev, info);
	gk_fb_data->fb_status = GK_FB_ACTIVE_MODE;
	mutex_unlock(&gk_fb_data->lock);

	errorCode = register_framebuffer(info);
	if (errorCode < 0) {
		dev_err(&pdev->dev, "%s: register_framebuffer fail!\n",
			__func__);
		errorCode = -ENOMEM;
		mutex_lock(&gk_fb_data->lock);
		goto gkfb_probe_dealloc_cmap;
	}

	gk_fb_data->proc_fb_info = info;
	gk_fb_data->proc_file = create_proc_entry(dev_name(&pdev->dev),
			S_IRUGO | S_IWUSR, get_gk_proc_dir());
	if (gk_fb_data->proc_file == NULL) {
		dev_err(&pdev->dev, "Register %s failed!\n",
			dev_name(&pdev->dev));
		errorCode = -ENOMEM;
		goto gkfb_probe_unregister_framebuffer;
	} else {
		gk_fb_data->proc_file->read_proc = gk_fb_proc_read;
		gk_fb_data->proc_file->data = gk_fb_data;
	}

	mutex_lock(&gk_fb_data->lock);
	gk_fb_data->screen_var = info->var;
	gk_fb_data->screen_fix = info->fix;
	mutex_unlock(&gk_fb_data->lock);

	dev_info(&pdev->dev,
		"probe p[%dx%d] v[%dx%d] c[%d] b[%d] l[%d] @ [0x%08lx:0x%08x]!\n",
		info->var.xres, info->var.yres, info->var.xres_virtual,
		info->var.yres_virtual, gk_fb_data->color_format,
		gk_fb_data->conversion_buf.available,
		info->fix.line_length,
		info->fix.smem_start, info->fix.smem_len);
	goto gkfb_probe_exit;

gkfb_probe_unregister_framebuffer:
	unregister_framebuffer(info);

gkfb_probe_dealloc_cmap:
	fb_dealloc_cmap(&info->cmap);

gkfb_probe_release_framebuffer:
	framebuffer_release(info);
	gk_fb_data->fb_status = GK_FB_STOP_MODE;
	mutex_unlock(&gk_fb_data->lock);

gkfb_probe_exit:
	return errorCode;
}

static int __devexit gk_fb_remove(struct platform_device *pdev)
{
	struct fb_info				*info;
	struct gk_platform_fb		*gk_fb_data = NULL;

	info = platform_get_drvdata(pdev);
	if (info) {
		remove_proc_entry(dev_name(&pdev->dev),
			get_gk_proc_dir());
		unregister_framebuffer(info);

		gk_fb_data = (struct gk_platform_fb *)info->par;
		mutex_lock(&gk_fb_data->lock);
		gk_fb_data->fb_status = GK_FB_STOP_MODE;
		fb_dealloc_cmap(&info->cmap);
		if (gk_fb_data->use_prealloc == 0) {
			if (info->screen_base) {
				kfree(info->screen_base);
			}
			if (gk_fb_data->conversion_buf.available) {
				if (gk_fb_data->conversion_buf.ping_buf) {
					kfree(gk_fb_data->conversion_buf.ping_buf);
				}
				if (gk_fb_data->conversion_buf.pong_buf) {
					kfree(gk_fb_data->conversion_buf.pong_buf);
				}
			}
			gk_fb_data->screen_fix.smem_start = 0;
			gk_fb_data->screen_fix.smem_len = 0;
		}
		platform_set_drvdata(pdev, NULL);
		framebuffer_release(info);
		mutex_unlock(&gk_fb_data->lock);
	}

	return 0;
}

#ifdef CONFIG_PM
static int gk_fb_suspend(struct platform_device *pdev, pm_message_t state)
{
	int					errorCode = 0;

	dev_dbg(&pdev->dev, "%s exit with %d @ %d\n",
		__func__, errorCode, state.event);

	return errorCode;
}

static int gk_fb_resume(struct platform_device *pdev)
{
	int					errorCode = 0;

	dev_dbg(&pdev->dev, "%s exit with %d\n", __func__, errorCode);

	return errorCode;
}
#endif

static struct platform_driver gk_fb_driver = {
	.probe		= gk_fb_probe,
	.remove 	= __devexit_p(gk_fb_remove),
#ifdef CONFIG_PM
	.suspend        = gk_fb_suspend,
	.resume		= gk_fb_resume,
#endif
	.driver = {
		.name	= "gk-fb",
	},
};

static int __init gk_fb_init(void)
{
	//printk("init gk framebuffer\n");
	return platform_driver_register(&gk_fb_driver);
}

static void __exit gk_fb_exit(void)
{
	platform_driver_unregister(&gk_fb_driver);
}

module_init(gk_fb_init);
module_exit(gk_fb_exit);

MODULE_DESCRIPTION("GK framebuffer driver");
MODULE_AUTHOR("Goke Microelectronics Inc.");
MODULE_LICENSE("GPL");

