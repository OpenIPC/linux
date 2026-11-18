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
#include <linux/errno.h>
#include <linux/string.h>
#include <linux/mm.h>
#include <linux/vmalloc.h>
#include <linux/platform_device.h>
#include <linux/fb.h>
#include <linux/dma-buf.h>
#include <linux/uaccess.h>
#include <linux/ax_vfb.h>
#include <drm/drm.h>
#include <drm/drm_print.h>

#include "ax_drm_vfb.h"

static int vfb_check_var(struct fb_var_screeninfo *var, struct fb_info *info);
static int vfb_set_par(struct fb_info *info);
static int vfb_pan_display(struct fb_var_screeninfo *var, struct fb_info *info);
/*static int vfb_mmap(struct fb_info *info, struct vm_area_struct *vma);*/
static int vfb_ioctl(struct fb_info *info, unsigned int cmd, unsigned long arg);

static struct fb_ops vfb_ops = {
	.fb_check_var = vfb_check_var,
	.fb_set_par = vfb_set_par,
	.fb_pan_display = vfb_pan_display,
	.fb_ioctl = vfb_ioctl,
};

void vfb_helper_fill_fix(struct fb_info *info, u64 addr, u32 len, u32 depth)
{
	memcpy(info->fix.id, "Virtual FB", 10);
	info->fix.type = FB_TYPE_PACKED_PIXELS;
	info->fix.visual =
	    depth == 8 ? FB_VISUAL_PSEUDOCOLOR : FB_VISUAL_TRUECOLOR;
	info->fix.mmio_start = 0;
	info->fix.mmio_len = 0;
	info->fix.type_aux = 0;
	info->fix.xpanstep = 1;	/* doing it in hw */
	info->fix.ypanstep = 1;	/* doing it in hw */
	info->fix.ywrapstep = 1;
	info->fix.accel = FB_ACCEL_NONE;
	info->fix.smem_start = addr;
	info->fix.smem_len = len;
}

/*
 *  Internal routines
 */
static u_long get_line_length(int xres_virtual, int bpp)
{
	u_long length;

	length = xres_virtual * bpp;
	length = (length + 31) & ~31;
	length >>= 3;

	return length;
}

/*
 *  Setting the video mode has been split into two parts.
 *  First part, xxxfb_check_var, must not write anything
 *  to hardware, it should only verify and adjust var.
 *  This means it doesn't alter par but it does use hardware
 *  data from it to check this var.
 */
static int vfb_check_var(struct fb_var_screeninfo *var, struct fb_info *info)
{
	u_long line_length;

	/*
	 *  FB_VMODE_CONUPDATE and FB_VMODE_SMOOTH_XPAN are equal!
	 *  as FB_VMODE_SMOOTH_XPAN is only used internally
	 */
	if (var->vmode & FB_VMODE_CONUPDATE) {
		var->vmode |= FB_VMODE_YWRAP;
		var->xoffset = info->var.xoffset;
		var->yoffset = info->var.yoffset;
	}

	/*
	 *  Some very basic checks
	 */
	if (!var->xres)
		var->xres = 1;
	if (!var->yres)
		var->yres = 1;
	if (var->xres > var->xres_virtual)
		var->xres_virtual = var->xres;
	if (var->yres > var->yres_virtual)
		var->yres_virtual = var->yres;

	if (var->bits_per_pixel <= 1)
		var->bits_per_pixel = 1;
	else if (var->bits_per_pixel <= 8)
		var->bits_per_pixel = 8;
	else if (var->bits_per_pixel <= 16)
		var->bits_per_pixel = 16;
	else if (var->bits_per_pixel <= 24)
		var->bits_per_pixel = 24;
	else if (var->bits_per_pixel <= 32)
		var->bits_per_pixel = 32;
	else
		return -EINVAL;

	if (var->xres_virtual < var->xoffset + var->xres)
		var->xres_virtual = var->xoffset + var->xres;
	if (var->yres_virtual < var->yoffset + var->yres)
		var->yres_virtual = var->yoffset + var->yres;

	/*
	 *  Memory limit
	 */
	line_length = get_line_length(var->xres_virtual, var->bits_per_pixel);
	if (line_length * var->yres_virtual > info->fix.smem_len) {
		DRM_ERROR("fb size out of maximum range, max size: 0x%x\n",
			  info->fix.smem_len);
		return -ENOMEM;
	}

	var->red.msb_right = 0;
	var->green.msb_right = 0;
	var->blue.msb_right = 0;
	var->transp.msb_right = 0;

	return 0;
}

/* This routine actually sets the video mode. It's in here where we
 * the hardware state info->par and fix which can be affected by the
 * change in par. For this driver it doesn't do much.
 */
static int vfb_set_par(struct fb_info *info)
{
	switch (info->var.bits_per_pixel) {
	case 1:
		info->fix.visual = FB_VISUAL_MONO01;
		break;
	case 8:
		info->fix.visual = FB_VISUAL_PSEUDOCOLOR;
		break;
	case 16:
	case 24:
	case 32:
		info->fix.visual = FB_VISUAL_TRUECOLOR;
		break;
	}

	info->fix.line_length = get_line_length(info->var.xres_virtual,
						info->var.bits_per_pixel);

	return 0;
}

/*
 *  Pan or Wrap the Display
 *
 *  This call looks only at xoffset, yoffset and the FB_VMODE_YWRAP flag
 */
static int vfb_pan_display(struct fb_var_screeninfo *var, struct fb_info *info)
{
	if (var->vmode & FB_VMODE_YWRAP) {
		if (var->yoffset >= info->var.yres_virtual || var->xoffset)
			return -EINVAL;
	} else {
		if (var->xoffset + info->var.xres > info->var.xres_virtual ||
		    var->yoffset + info->var.yres > info->var.yres_virtual)
			return -EINVAL;
	}

	info->var.xoffset = var->xoffset;
	info->var.yoffset = var->yoffset;

	if (var->vmode & FB_VMODE_YWRAP)
		info->var.vmode |= FB_VMODE_YWRAP;
	else
		info->var.vmode &= ~FB_VMODE_YWRAP;
	return 0;
}

#if 0
int vfb_mmap(struct fb_info *info, struct vm_area_struct *vma)
{
	int ret;
	unsigned long vm_size;
	struct ax_fb_device *fbdev = info->par;

	vma->vm_flags &= ~VM_PFNMAP;
	vma->vm_pgoff = 0;

	vm_size = vma->vm_end - vma->vm_start;
	/* check if user-requested size is valid. */
	if (vm_size > fbdev->size)
		return -EINVAL;

	ret = dma_common_mmap(info->dev, vma, fbdev->buf_viraddr,
			      fbdev->buf_phyaddr, fbdev->size);

	if (ret < 0) {
		DRM_ERROR("failed to mmap.\n");
		return ret;
	}

	return 0;
}
#endif

static int vfb_ioctl(struct fb_info *info, unsigned int cmd, unsigned long arg)
{
	struct axfb_cursor_info *cursor_info;
	struct axfb_cursor_pos hot;
	struct axfb_cursor_res res;
	struct axfb_colorkey colorkey;
	__u16 cursor_show;
	void __user *argp = (void __user *)arg;
	int ret = 0;
	struct ax_fb_device *vfbdev;
	__u16 cusor_flag;


	if (!info)
		return -ENODEV;

	vfbdev = info->par;
	cusor_flag = vfbdev->is_cursor;

	cursor_info = (struct axfb_cursor_info *)info->fbcon_par;
	switch (cmd) {
	case AX_FBIOPUT_CURSOR_POS:
		if (cusor_flag == 1) {
			if (copy_from_user(&hot, argp, sizeof(hot)))
				return -EFAULT;
			cursor_info->hot = hot;
		} else {
			DRM_ERROR("fb%d not support cursor\n", vfbdev->id);
			ret = -EACCES;
		}
		break;
	case AX_FBIOPUT_CURSOR_RES:
		if (cusor_flag == 1) {
			if (copy_from_user(&res, argp, sizeof(res)))
				return -EFAULT;
			cursor_info->res = res;
		} else {
			DRM_ERROR("fb%d not support cursor\n", vfbdev->id);
			ret = -EACCES;
		}
		break;
	case AX_FBIOPUT_CURSOR_SHOW:
		if (cusor_flag == 1) {
			if (copy_from_user(&cursor_show, argp, sizeof(cursor_show)))
				return -EFAULT;
			cursor_info->enable = cursor_show;
		} else {
			DRM_ERROR("fb%d not support cursor\n", vfbdev->id);
			ret = -EACCES;
		}
		break;
	case AX_FBIOGET_TYPE:
		ret = copy_to_user(argp, &cusor_flag, sizeof(__u16)) ? -EFAULT : 0;
		break;
	case AX_FBIOGET_CURSORINFO:
		if (cusor_flag == 1)
			ret = copy_to_user(argp, cursor_info, sizeof(*cursor_info)) ? -EFAULT : 0;
		else {
			DRM_ERROR("fb%d not support cursor\n", vfbdev->id);
			ret = -EACCES;
		}
		break;
	case AX_FBIOGET_COLORKEY:
		colorkey.key_low = vfbdev->colorkey.key_low;
		colorkey.key_high = vfbdev->colorkey.key_high;
		colorkey.enable = vfbdev->colorkey.enable;
		colorkey.inv = vfbdev->colorkey.inv;

		ret = copy_to_user(argp, &colorkey, sizeof(struct axfb_colorkey)) ? -EFAULT : 0;
		break;
	case AX_FBIOPUT_COLORKEY:
		if (copy_from_user(&colorkey, argp, sizeof(struct axfb_colorkey)))
			return -EFAULT;

		vfbdev->colorkey.key_low = colorkey.key_low;
		vfbdev->colorkey.key_high = colorkey.key_high;
		vfbdev->colorkey.enable = colorkey.enable;
		vfbdev->colorkey.inv = colorkey.inv;

		break;
	default:
		ret = -ENOTTY;
	}

	return ret;
}

int ax_vfb_register(struct ax_fb_device *vfbdev)
{
	int ret = 0;
	struct fb_info *info = NULL;
	struct platform_device *pdev = NULL;
	struct device *dev = NULL;
	struct axfb_cursor_info *cursor = NULL;

	if (!vfbdev) {
		DRM_ERROR("vfbdev is NULL\n");
		return -EINVAL;
	}

	pdev = vfbdev->pdev;
	dev = &pdev->dev;

	ret = dma_set_coherent_mask(dev, DMA_BIT_MASK(36));
	if (ret) {
		DRM_ERROR("DMA enable failed\n");
		return -EINVAL;
	}

	info = framebuffer_alloc(sizeof(u32) * 256, dev);
	if (!info) {
		DRM_ERROR("alloc fb_info failed\n");
		return -ENOMEM;
	}

	if (vfbdev->is_cursor) {
		cursor = kzalloc(sizeof(*cursor), GFP_KERNEL);
		if (!cursor) {
			DRM_ERROR("alloc cursor mem failed\n");
			return -ENOMEM;
			goto exit0;
		}
		info->fbcon_par = cursor;
	}

	info->fbops = &vfb_ops;
	info->screen_base = (void *)vfbdev->buf_viraddr;

	vfb_helper_fill_fix(info, vfbdev->buf_phyaddr, vfbdev->size, vfbdev->bpp);

	info->var.xres = vfbdev->width;
	info->var.yres = vfbdev->height;
	info->var.xres_virtual = vfbdev->width;
	info->var.yres_virtual = vfbdev->height * vfbdev->buf_nr;
	info->var.bits_per_pixel = vfbdev->bpp;
	info->var.height = -1;
	info->var.width = -1;
	info->var.vmode = FB_VMODE_NONINTERLACED;

	info->flags = FBINFO_FLAG_DEFAULT;
	info->par = vfbdev;

	DRM_INFO("vfb%d is_cursor:%d screen_base = 0x%px, smem_start = 0x%lx xres:%d yres:%d\n",
		 vfbdev->id, vfbdev->is_cursor, info->screen_base, info->fix.smem_start,
		 info->var.xres, info->var.yres);

	ret = fb_alloc_cmap(&info->cmap, 256, 0);
	if (ret < 0) {
		DRM_ERROR("alloc cmap failed\n");
		goto exit1;
	}

	ret = register_framebuffer(info);
	if (ret < 0) {
		DRM_ERROR("register framebuffer failed\n");
		goto exit2;
	}

	vfb_set_par(info);

	vfbdev->data = info;

exit2:
	if (ret)
		fb_dealloc_cmap(&info->cmap);
exit1:
	if (ret)
		kfree(cursor);
exit0:
	if (ret)
		framebuffer_release(info);

	DRM_INFO("%s done, ret = %d\n", __func__, ret);

	return ret;
}
EXPORT_SYMBOL(ax_vfb_register);

int ax_vfb_unregister(struct ax_fb_device *vfbdev)
{
	struct fb_info *info = vfbdev->data;

	if (info) {
		unregister_framebuffer(info);
		if(info->fbcon_par) {
			kfree(info->fbcon_par);
			info->fbcon_par = NULL;
		}
		fb_dealloc_cmap(&info->cmap);
		framebuffer_release(info);

		vfbdev->data = NULL;
	}

	return 0;
}
EXPORT_SYMBOL(ax_vfb_unregister);


