#include <linux/fb.h>
#include "nvt_fb_na51000.h"
#include "nvt_dbg.h"
#include "display.h"
#include "dispcomn.h"

unsigned char driver_name_osd[] = "nvt_fb_osd";
struct fb_ops nvt_fb_osd_ops;

static int nvt_fb_osd_fill_para(struct fb_info *fbinfo);

static unsigned int chan_to_field(unsigned int chan, struct fb_bitfield *bf)
{
	DBG_IND("\n");
	return ((chan & 0xffff) >> (16 - bf->length)) << bf->offset;
}

int nvt_fb_osd_probe(struct fb_info *fbinfo, unsigned int ide_id, unsigned int fb_layer)
{
	int ret = 0;

	DBG_IND("+ 0x%x\n", (unsigned int)fbinfo);

	ret = nvt_fb_osd_fill_para(fbinfo);

	if (ret) {
		return ret;
	}

	ret = display_obj_set_osd_addr(ide_id, fbinfo->fix.smem_start, 1);

	return ret;
}

static int nvt_fb_osd_fill_para(struct fb_info *fbinfo)
{
	int ret = 0;

	fbinfo->fix.type      = FB_TYPE_PACKED_PIXELS;
	fbinfo->fix.type_aux  = 0;
	fbinfo->fix.xpanstep  = 1;
	fbinfo->fix.ypanstep  = 1;
	fbinfo->fix.ywrapstep = 0;
	fbinfo->fix.accel     = FB_ACCEL_NONE;
	strcpy(fbinfo->fix.id, driver_name_osd);

	display_obj_get_osd_param(DISP_1, &fbinfo->var, &fbinfo->fix);

	fbinfo->fbops = &nvt_fb_osd_ops;
	fbinfo->flags = FBINFO_FLAG_DEFAULT;

	fbinfo->var.nonstd      = 0;
	fbinfo->var.activate    = FB_ACTIVATE_NOW;
	fbinfo->var.accel_flags = 0;
	fbinfo->var.vmode       = FB_VMODE_NONINTERLACED;

	((struct nvt_fb_par *)fbinfo->par)->palette_sz = 255;

	return ret;
}

int nvt_fb_osd_check_var(struct fb_var_screeninfo *var, struct fb_info *fbinfo)
{
	struct fb_fix_screeninfo *fix = &fbinfo->fix;

 	if ((var->yres_virtual * fix->line_length) > fix->smem_len) {
		nvt_dbg(ERR, "invalid yres_virtual:%d\n", var->yres_virtual);
		return -EINVAL;
	}

 	if ((var->xres > var->xres_virtual) || (var->yres > var->yres_virtual)) {
		nvt_dbg(ERR, "invalid parameter xres:%d yres:%d xres_virt:%d yres_virt:%d\n", var->xres, var->yres,  var->xres_virtual, var->yres_virtual);
		return -EINVAL;
	}

	return 0;
}

static void nvt_fb_osd_activate_var(struct fb_info *info)
{
/*	struct nvt_fb_par *fb_par = (struct nvt_fb_par *) info->par;
	struct fb_var_screeninfo *var = &info->var; */

    /*display_obj_set_osd_param(NVT_FB_GET_IDE_ID(fb_par->fb_id), var, NULL);*/
	/* set video address to IDE driver */
	//	display_obj_set_osd_addr(NVT_FB_GET_IDE_ID(fb_par->fb_id), info->fix.smem_start + var->yoffset, 1);
	nvt_fb_pan_display(&info->var, info);

}

int nvt_fb_osd_set_par(struct fb_info *info)
{
	nvt_fb_osd_activate_var(info);
	return 0;
}

int nvt_fb_osd_blank(int blank_mode, struct fb_info *info)
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
		display_obj_set_osd_blank(NVT_FB_GET_IDE_ID(fb_par->fb_id), blank);
	}

	return ret;
}

static int nvt_fb_osd_setcolreg(unsigned regno,
			       unsigned red, unsigned green, unsigned blue,
			       unsigned transp, struct fb_info *info)
{
	unsigned int val;

	DBG_IND("setcol: regno=%d, rgb=%d,%d,%d tans=%d\n",
		   regno, red, green, blue, transp);

	switch (info->fix.visual) {
	case FB_VISUAL_TRUECOLOR:
		/* true-colour, use pseudo-palette */

		if (regno < 16) {
			u32 *pal = info->pseudo_palette;

			val  = chan_to_field(red,   &info->var.red);
			val |= chan_to_field(green, &info->var.green);
			val |= chan_to_field(blue,  &info->var.blue);

			pal[regno] = val;
		}
		break;

	case FB_VISUAL_PSEUDOCOLOR:
		if (regno < 256) {
			val  = (red & 0xFF00) >> 8;
			val |= (green & 0xFF00) ;
			val |= (blue & 0xFF00) << 8;
			val |= (transp & 0xFF00) << 16;

			/*Call to IDE driver to fill palette register */
			if (display_obj_set_osd_palette(DISP_1, regno, val)) {
				return -EINVAL;
			}

		} else {
			DBG_ERR("Pal range err:%d", regno);
			return -EINVAL;	/* unknown type */
		}

		break;

	default:
		return -EINVAL;	/* unknown type */
	}

	return 0;
}

struct fb_ops nvt_fb_osd_ops = {
	.owner		= THIS_MODULE,
	.fb_check_var	= nvt_fb_osd_check_var,
	.fb_set_par	= nvt_fb_osd_set_par,
	.fb_setcolreg	= nvt_fb_osd_setcolreg,
	.fb_blank	= nvt_fb_osd_blank,
	.fb_fillrect	= cfb_fillrect, /* Use native function */
	.fb_copyarea	= cfb_copyarea, /* Use native function */
	.fb_imageblit	= cfb_imageblit, /* Use native function */
	.fb_pan_display	= nvt_fb_pan_display
};
