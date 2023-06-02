#include <linux/fb.h>
#include "nvt_fb_na51000.h"
#include "nvt_dbg.h"
#include "display.h"
#include "kdrv_videoout/nvt_fb.h"

struct fb_ops nvt_fb_video_ops;

static int nvt_fb_video_fill_para(struct fb_info *fbinfo);

int nvt_fb_video_probe(struct fb_info *fbinfo, unsigned int ide_id, unsigned int layer)
{
	int ret = 0;

	/* fill parameter */
	ret = nvt_fb_video_fill_para(fbinfo);

	return ret;
}

static int nvt_fb_video_fill_para(struct fb_info *fbinfo)
{
	struct nvt_fb_par *fb_par = (struct nvt_fb_par *) fbinfo->par;
	int ret = 0;

	fbinfo->fix.type	    = FB_TYPE_PACKED_PIXELS;
	fbinfo->fix.type_aux	    = 0;
	fbinfo->fix.xpanstep	    = 1;
	fbinfo->fix.ypanstep	    = 1;
	fbinfo->fix.ywrapstep	    = 0;
	fbinfo->fix.accel	    = FB_ACCEL_NONE;

	if (fb_par == NULL) {
		DBG_ERR("fb_par is NULL\n");
		return -EINVAL;
	}

	if (NVT_FB_GET_IDE_ID(fb_par->fb_id) == NVT_IDE1) {
		display_obj_get_video1_param(NVT_FB_GET_IDE_ID(fb_par->fb_id), NULL, &fbinfo->fix);
	} else {
		display_obj_get_video2_param(NVT_FB_GET_IDE_ID(fb_par->fb_id), NULL, &fbinfo->fix);
	}

	fbinfo->fbops = &nvt_fb_video_ops;
	fbinfo->flags = FBINFO_FLAG_DEFAULT;

	fbinfo->var.nonstd      = 0;
	fbinfo->var.activate    = FB_ACTIVATE_NOW;
	fbinfo->var.accel_flags = 0;
	fbinfo->var.vmode       = FB_VMODE_NONINTERLACED;


	//call IDE driver to get display parameter
	ret = nvt_fb_video_check_var(&fbinfo->var, fbinfo);

	if (ret) {
		return ret;
	}

	((struct nvt_fb_par *)fbinfo->par)->palette_sz = 0;

	return ret;
}

int nvt_fb_video_check_var(struct fb_var_screeninfo *var, struct fb_info *info)
{
/*	struct nvt_fb_par *fb_par = (struct nvt_fb_par *) info->par; */

	/* check parameter here */

	return 0;
}

static void nvt_fb_video_activate_var(struct fb_info *info)
{
/*	struct nvt_fb_par *fb_par = (struct nvt_fb_par *) info->par;
	struct fb_var_screeninfo *var = &info->var; */

    /*display_obj_set_osd_param(NVT_FB_GET_IDE_ID(fb_par->fb_id), var, NULL);*/
	/* set video address to IDE driver */
	//	display_obj_set_osd_addr(NVT_FB_GET_IDE_ID(fb_par->fb_id), info->fix.smem_start + var->yoffset, 1);
	nvt_fb_pan_display(&info->var, info);

}

int nvt_fb_video_set_par(struct fb_info *info)
{
	nvt_fb_video_activate_var(info);
	return 0;
}

int nvt_fb_video_blank(int blank_mode, struct fb_info *info)
{
	struct nvt_fb_par *fb_par = (struct nvt_fb_par *) info->par;
	int blank;
	int ret = 0;

	DBG_IND("block_mode:%d\n", blank_mode);

	switch (blank_mode) {
	case FB_BLANK_UNBLANK: /* Unblanking */
		blank = 0;
		info->flags &= (~(int)(NVT_FB_BLANK));
		break;

	case FB_BLANK_NORMAL: /* Normal blanking */
	case FB_BLANK_VSYNC_SUSPEND: /* VESA blank (vsync off) */
	case FB_BLANK_HSYNC_SUSPEND: /* VESA blank (hsync off) */
	case FB_BLANK_POWERDOWN: /* Poweroff */
		blank = 1;
		info->flags |= (int)NVT_FB_BLANK;
		break;

	default:
		ret = -EINVAL;
	}

	if (ret == 0) {
		if (NVT_FB_GET_IDE_ID(fb_par->fb_id)) {
			ret = display_obj_set_video1_blank(NVT_FB_GET_IDE_ID(fb_par->fb_id), blank);
		} else {
			ret = display_obj_set_video2_blank(NVT_FB_GET_IDE_ID(fb_par->fb_id), blank);
		}
	}

	return ret;
}

/*
 *  Frame buffer operations
 */
struct fb_ops nvt_fb_video_ops = {
	.owner		= THIS_MODULE,
	.fb_check_var	= nvt_fb_video_check_var,
	.fb_set_par	= nvt_fb_video_set_par,
	.fb_blank	= nvt_fb_video_blank,
	.fb_fillrect	= cfb_fillrect, /* Use native function */
	.fb_copyarea	= cfb_copyarea, /* Use native function */
	.fb_imageblit	= cfb_imageblit, /* Use native function */
	.fb_pan_display	= nvt_fb_pan_display
};

