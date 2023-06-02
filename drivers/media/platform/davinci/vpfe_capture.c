/*
 * Copyright (C) 2008-2009 Texas Instruments Inc
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 *
 * Driver name : VPFE Capture driver
 *    VPFE Capture driver allows applications to capture and stream video
 *    frames on DaVinci SoCs (DM6446, DM355 etc) from a YUV source such as
 *    TVP5146 or  Raw Bayer RGB image data from an image sensor
 *    such as Microns' MT9T001, mt9p031 etc.
 *
 *    These SoCs have, in common, a Video Processing Subsystem (VPSS) that
 *    consists of a Video Processing Front End (VPFE) for capturing
 *    video/raw image data and Video Processing Back End (VPBE) for displaying
 *    YUV data through an in-built analog encoder or Digital LCD port. This
 *    driver is for capture through VPFE. A typical EVM using these SoCs have
 *    following high level configuration.
 *
 *
 *    decoder(TVP5146/		YUV/
 * 	     MT9T001)   -->  Raw Bayer RGB ---> MUX -> VPFE (CCDC/ISIF)
 *    				data input              |      |
 *							V      |
 *						      SDRAM    |
 *							       V
 *							   Image Processor
 *							       |
 *							       V
 *							     SDRAM
 *    The data flow happens from a decoder connected to the VPFE over a
 *    YUV embedded (BT.656/BT.1120) or separate sync or raw bayer rgb interface
 *    and to the input of VPFE through an optional MUX (if more inputs are
 *    to be interfaced on the EVM). The input data is first passed through
 *    CCDC (CCD Controller, a.k.a Image Sensor Interface, ISIF). The CCDC
 *    does very little or no processing on YUV data and does pre-process Raw
 *    Bayer RGB data through modules such as Defect Pixel Correction (DFC)
 *    Color Space Conversion (CSC), data gain/offset etc. After this, data
 *    can be written to SDRAM or can be connected to the image processing
 *    block such as IPIPE (on DM355 only).
 *
 *    Features supported
 *  		- MMAP IO
 *		- Capture using TVP5146 over BT.656
 *		- support for interfacing decoders using sub device model
 *		- Work with DM355 or DM6446 CCDC to do Raw Bayer RGB/YUV
 *		  data capture to SDRAM.
 *    TODO list
 *		- Support multiple REQBUF after open
 *		- Support for de-allocating buffers through REQBUF
 *		- Support for chaining Image Processor
 *		- Support for static allocation of buffers
 *		- Support for STREAMON before QBUF
 *		- Support for control ioctls
 */
#include <linux/module.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/interrupt.h>
#include <linux/version.h>
#include <linux/io.h>
#include <linux/slab.h>

#include <media/v4l2-common.h>
#include <media/davinci/videohd.h>
#include <media/davinci/vpfe_capture.h>
#include <media/davinci/imp_hw_if.h>

#include <mach/cputype.h>

#include "ccdc_hw_device.h"

#define HD_IMAGE_SIZE		(2176 * 2176 * 2)
#define PAL_IMAGE_SIZE		(720 * 576 * 2)
#define SECOND_IMAGE_SIZE_MAX	(640 * 480 * 2)

static int debug = 1;
#ifdef CONFIG_VIDEO_YCBCR
static u32 numbuffers = 4;
#else
static u32 numbuffers = 3;
#endif
static u32 bufsize = HD_IMAGE_SIZE + SECOND_IMAGE_SIZE_MAX;
static int interface;
static u32 cont_bufoffset = 0;
static u32 cont_bufsize = 0;

module_param(interface, bool, S_IRUGO);
module_param(numbuffers, uint, S_IRUGO);
module_param(bufsize, uint, S_IRUGO);
module_param(debug, bool, 0644);
module_param(cont_bufoffset, uint, S_IRUGO);
module_param(cont_bufsize, uint, S_IRUGO);

/**
 * VPFE capture can be used for capturing video such as from TVP5146 or TVP7002
 * and for capture raw bayer data from camera sensors such as mt9p031. At this
 * point there is problem in co-existence of mt9p031 and tvp5146 due to i2c
 * address collision. So set the variable below from bootargs to do either video
 * capture or camera capture.
 * interface = 0 - video capture (from TVP514x or such),
 * interface = 1 - Camera capture (from mt9p031 or such)
 * Re-visit this when we fix the co-existence issue
 */
MODULE_PARM_DESC(interface, "interface 0-1 (default:0)");
MODULE_PARM_DESC(numbuffers, "buffer count (default:3)");
MODULE_PARM_DESC(bufsize, "buffer size in bytes, (default:4147200 bytes)");
MODULE_PARM_DESC(debug, "Debug level 0-1");
MODULE_PARM_DESC(cont_bufoffset, "Capture buffer offset (default 0)");
MODULE_PARM_DESC(cont_bufsize, "Capture buffer size (default 0)");

MODULE_DESCRIPTION("VPFE Video for Linux Capture Driver");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Texas Instruments");

//TODO !!!
#ifdef CONFIG_VIDEO_YCBCR
static size_t     ipipif_dma_size = 0;
static dma_addr_t ipipif_dma_addr_phys = 0;
static void *     ipipif_dma_addr_cpu = NULL;
#endif
/* standard information */
struct vpfe_standard {
	v4l2_std_id std_id;
	unsigned int width;
	unsigned int height;
	struct v4l2_fract pixelaspect;
	/* 0 - progressive, 1 - interlaced */
	int frame_format;
	struct v4l2_fract fps;
};

/* ccdc configuration */
struct ccdc_config {
	/* This make sure vpfe is probed and ready to go */
	int vpfe_probed;
	/* name of ccdc device */
	char name[32];
};

/* data structures */
static struct vpfe_config_params config_params = {
#ifdef CONFIG_VIDEO_YCBCR
	.min_numbuffers = 4,
	.numbuffers = 4,
#else
	.min_numbuffers = 3,
	.numbuffers = 3,
#endif
	.min_bufsize = 1280 * 720 * 2,
	/* DM365 IPIPE supports up to 2176 pixels, otherwise you need to use raw */
	.device_bufsize = 2176 * 2176 * 2,
};

/* ccdc device registered */
static struct ccdc_hw_device *ccdc_dev;
/* lock for accessing ccdc information */
static DEFINE_MUTEX(ccdc_lock);
/* ccdc configuration */
static struct ccdc_config *ccdc_cfg;

/*  hardware interface for image processing pipeline */
static struct imp_hw_interface *imp_hw_if;

const struct vpfe_standard vpfe_standards[] = {
	{V4L2_STD_525_60, 720, 480, {11, 10}, 1, {1001, 30000} },
	{V4L2_STD_625_50, 720, 576, {54, 59}, 1, {1, 25} },
	{V4L2_STD_525P_60, 720, 480, {11, 10}, 0, {1001, 30000} },
	{V4L2_STD_625P_50, 720, 576, {54, 59}, 0, {1, 25} },
	{V4L2_STD_720P_30, 1280, 720, {1, 1}, 0, {1, 30} },
	{V4L2_STD_720P_50, 1280, 720, {1, 1}, 0, {1, 50} },
	{V4L2_STD_720P_60, 1280, 720, {1, 1}, 0, {1, 60} },
	{V4L2_STD_1080I_30, 1920, 1080, {1, 1}, 1, {1, 30} },
	{V4L2_STD_1080I_50, 1920, 1080, {1, 1}, 1, {1, 50} },
	{V4L2_STD_1080I_60, 1920, 1080, {1, 1}, 1, {1, 60} },
	{V4L2_STD_1080P_30, 1920, 1080, {1, 1}, 0, {1, 30} },
	{V4L2_STD_1080P_50, 1920, 1080, {1, 1}, 0, {1, 50} },
	{V4L2_STD_1080P_60, 1920, 1080, {1, 1}, 0, {1, 60} },
};

/* Used when raw Bayer image from ccdc is directly captured to SDRAM */
static const struct vpfe_pixel_format vpfe_pix_fmts[] = {
	{
		.fmtdesc = {
			.index = 0,
			.type = V4L2_BUF_TYPE_VIDEO_CAPTURE,
			.description = "Bayer GrRBGb 8bit A-Law compr.",
			.pixelformat = V4L2_PIX_FMT_SBGGR8,
		},
		.bpp = 1,
		.subdev_pix_fmt = V4L2_PIX_FMT_SGRBG10,
	},
	{
		.fmtdesc = {
			.index = 1,
			.type = V4L2_BUF_TYPE_VIDEO_CAPTURE,
			.description = "Bayer GrRBGb - 16bit",
			.pixelformat = V4L2_PIX_FMT_SBGGR16,
		},
		.bpp = 2,
		.subdev_pix_fmt = V4L2_PIX_FMT_SGRBG10,
	},
	{
		.fmtdesc = {
			.index = 2,
			.type = V4L2_BUF_TYPE_VIDEO_CAPTURE,
			.description = "Bayer GrRBGb 8bit DPCM compr.",
			.pixelformat = V4L2_PIX_FMT_SGRBG10DPCM8,
		},
		.bpp = 1,
		.subdev_pix_fmt = V4L2_PIX_FMT_SGRBG10,
	},
	{
		.fmtdesc = {
			.index = 3,
			.type = V4L2_BUF_TYPE_VIDEO_CAPTURE,
			.description = "YCbCr 4:2:2 Interleaved UYVY",
			.pixelformat = V4L2_PIX_FMT_UYVY,
		},
		.bpp = 2,
		.subdev_pix_fmt = V4L2_PIX_FMT_UYVY,
	},
	{
		.fmtdesc = {
			.index = 4,
			.type = V4L2_BUF_TYPE_VIDEO_CAPTURE,
			.description = "YCbCr 4:2:2 Interleaved YUYV",
			.pixelformat = V4L2_PIX_FMT_YUYV,
		},
		.bpp = 2,
		.subdev_pix_fmt = V4L2_PIX_FMT_UYVY,
	},
	{
		.fmtdesc = {
			.index = 5,
			.type = V4L2_BUF_TYPE_VIDEO_CAPTURE,
			.description = "Y/CbCr 4:2:0 - Semi planar",
			.pixelformat = V4L2_PIX_FMT_NV12,
		},
		.bpp = 1,
		.subdev_pix_fmt = V4L2_PIX_FMT_UYVY,
	},
};

/**
 * vpfe_lookup_pix_format() - lookup an entry in the vpfe pix format table
 * @pix_format: v4l pix format
 * This function lookup an entry in the vpfe pix format table based on
 * pix_format
 */
static const struct vpfe_pixel_format *vpfe_lookup_pix_format(u32 pix_format)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(vpfe_pix_fmts); i++) {
		if (pix_format == vpfe_pix_fmts[i].fmtdesc.pixelformat)
			return &vpfe_pix_fmts[i];
	}
	return NULL;
}

/*
 * vpfe_register_ccdc_device. CCDC module calls this to
 * register with vpfe capture
 */
int vpfe_register_ccdc_device(struct ccdc_hw_device *dev)
{
	int ret = 0;
	printk(KERN_NOTICE "vpfe_register_ccdc_device: %s\n", dev->name);

	BUG_ON(!dev->hw_ops.open);
	BUG_ON(!dev->hw_ops.enable);
	BUG_ON(!dev->hw_ops.set_hw_if_params);
	BUG_ON(!dev->hw_ops.configure);
	BUG_ON(!dev->hw_ops.set_buftype);
	BUG_ON(!dev->hw_ops.get_buftype);
	BUG_ON(!dev->hw_ops.enum_pix);
	BUG_ON(!dev->hw_ops.set_frame_format);
	BUG_ON(!dev->hw_ops.get_frame_format);
	BUG_ON(!dev->hw_ops.get_pixel_format);
	BUG_ON(!dev->hw_ops.set_pixel_format);
	BUG_ON(!dev->hw_ops.set_params);
	BUG_ON(!dev->hw_ops.set_image_window);
	BUG_ON(!dev->hw_ops.get_image_window);
	BUG_ON(!dev->hw_ops.get_line_length);
	BUG_ON(!dev->hw_ops.getfid);

	mutex_lock(&ccdc_lock);
	if (NULL == ccdc_cfg) {
		/*
		 * TODO. Will this ever happen? if so, we need to fix it.
		 * Proabably we need to add the request to a linked list and
		 * walk through it during vpfe probe
		 */
		printk(KERN_ERR "vpfe capture not initialized\n");
		ret = -EFAULT;
		goto unlock;
	}

	if (strcmp(dev->name, ccdc_cfg->name)) {
		/* ignore this ccdc */
		ret = -EINVAL;
		goto unlock;
	}

	if (ccdc_dev) {
		printk(KERN_ERR "ccdc already registered\n");
		ret = -EINVAL;
		goto unlock;
	}

	ccdc_dev = dev;
unlock:
	mutex_unlock(&ccdc_lock);
	return ret;
}
EXPORT_SYMBOL(vpfe_register_ccdc_device);

/*
 * vpfe_unregister_ccdc_device. CCDC module calls this to
 * unregister with vpfe capture
 */
void vpfe_unregister_ccdc_device(struct ccdc_hw_device *dev)
{
	if (NULL == dev) {
		printk(KERN_ERR "invalid ccdc device ptr\n");
		return;
	}

	printk(KERN_NOTICE "vpfe_unregister_ccdc_device, dev->name = %s\n",
		dev->name);

	if (strcmp(dev->name, ccdc_cfg->name)) {
		/* ignore this ccdc */
		return;
	}

	mutex_lock(&ccdc_lock);
	ccdc_dev = NULL;
	mutex_unlock(&ccdc_lock);
	return;
}
EXPORT_SYMBOL(vpfe_unregister_ccdc_device);

/*
 * vpfe_get_camera_frame_params()
 * Get the image parameters such as max height and width, frame format
 * etc and update the stdinfo accordingly. This is a work around to get
 * the maximum width, height and frame format since camera driver doesn't
 * support s_std.
 */
static int vpfe_get_camera_frame_params(struct vpfe_device *vpfe_dev)
{
	struct vpfe_subdev_info *sdinfo = vpfe_dev->current_subdev;
	struct v4l2_format sd_fmt;
	int ret;

	/* TODO: Currently there is no support for setting timings
	 * in sensor similar to S_STD. So get the limits of width and height
	 * using try format. In future we should be able to set
	 * timings for a specific resolution and fps. In that case
	 * we know the limits for the specific timing and this code
	 * would require change.
	 */
	memset(&sd_fmt, 0, sizeof(sd_fmt));
#ifdef CONFIG_VIDEO_YCBCR
	ret = v4l2_device_call_until_err(&vpfe_dev->v4l2_dev,
			sdinfo->grp_id, video, g_fmt, &sd_fmt);
#else
	sd_fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	/* hard code it to match that of mt9p031 sensor */
	sd_fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_SGRBG10;
	/* use a value big enough */
	sd_fmt.fmt.pix.width = 1 << 31;
	sd_fmt.fmt.pix.height = 1 << 31;
	ret = v4l2_device_call_until_err(&vpfe_dev->v4l2_dev,
			sdinfo->grp_id, video, try_fmt, &sd_fmt);
#endif
	if (!ret) {
		vpfe_dev->std_info.active_pixels = sd_fmt.fmt.pix.width;
		vpfe_dev->std_info.active_lines = sd_fmt.fmt.pix.height;
		/* hard code the frame format to be progressive always.  */
		vpfe_dev->std_info.frame_format = 0;
	}
	return ret;
}

/*
 * vpfe_get_ccdc_image_format - Get image parameters based on CCDC settings
 */
static int vpfe_get_ccdc_image_format(struct vpfe_device *vpfe_dev,
				 struct v4l2_format *f)
{
	struct v4l2_rect image_win;
	enum ccdc_buftype buf_type;
	enum ccdc_frmfmt frm_fmt;

	vpfe_dev->crop.top = 0;
	vpfe_dev->crop.left = 0;
	memset(f, 0, sizeof(*f));
#ifdef CONFIG_VIDEO_YCBCR
	f->type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
#else
	f->type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
#endif
	ccdc_dev->hw_ops.get_image_window(&image_win);
	vpfe_dev->crop.width = image_win.width;
	vpfe_dev->crop.height = image_win.height;
	f->fmt.pix.width = image_win.width;
	f->fmt.pix.height = image_win.height;
	f->fmt.pix.bytesperline = ccdc_dev->hw_ops.get_line_length();
	f->fmt.pix.sizeimage = f->fmt.pix.bytesperline *
				f->fmt.pix.height;
	buf_type = ccdc_dev->hw_ops.get_buftype();
	f->fmt.pix.pixelformat = ccdc_dev->hw_ops.get_pixel_format();
	frm_fmt = ccdc_dev->hw_ops.get_frame_format();
	if (frm_fmt == CCDC_FRMFMT_PROGRESSIVE)
		f->fmt.pix.field = V4L2_FIELD_NONE;
	else if (frm_fmt == CCDC_FRMFMT_INTERLACED) {
		if (buf_type == CCDC_BUFTYPE_FLD_INTERLEAVED)
			f->fmt.pix.field = V4L2_FIELD_INTERLACED;
		else if (buf_type == CCDC_BUFTYPE_FLD_SEPARATED)
			f->fmt.pix.field = V4L2_FIELD_SEQ_TB;
		else {
			v4l2_err(&vpfe_dev->v4l2_dev, "Invalid buf_type\n");
			return -EINVAL;
		}
	} else {
		v4l2_err(&vpfe_dev->v4l2_dev, "Invalid frm_fmt\n");
		return -EINVAL;
	}
	return 0;
}

/*
 * vpfe_config_ccdc_image_format()
 * For a pix format, configure ccdc to setup the capture
 */
static int vpfe_config_ccdc_image_format(struct vpfe_device *vpfe_dev)
{
	enum ccdc_frmfmt frm_fmt = CCDC_FRMFMT_INTERLACED;
	u32 pix_fmt;
	int ret = 0;

	pix_fmt = vpfe_dev->fmt.fmt.pix.pixelformat;

	/* At CCDC we need to set pix format based on source. */
	if (vpfe_dev->imp_chained) {
		if (vpfe_dev->current_subdev->is_camera){
#ifdef CONFIG_VIDEO_YCBCR
			if(vpfe_dev->current_subdev->ccdc_if_params.if_type == VPFE_YCBCR_SYNC_8)
			{
				pix_fmt = V4L2_PIX_FMT_YUYV;
			}
			//TODO
			else
			{
				pix_fmt = V4L2_PIX_FMT_SBGGR16;
			}
#else
			pix_fmt = V4L2_PIX_FMT_SBGGR16;
#endif
		}
		else if (pix_fmt == V4L2_PIX_FMT_NV12){
			pix_fmt = V4L2_PIX_FMT_UYVY;
		}
	}

	if (ccdc_dev->hw_ops.set_pixel_format(pix_fmt) < 0) {
		v4l2_err(&vpfe_dev->v4l2_dev,
			"couldn't set pix format in ccdc\n");
		return -EINVAL;
	}
	/* configure the image window */
	ccdc_dev->hw_ops.set_image_window(&vpfe_dev->crop);

	switch (vpfe_dev->fmt.fmt.pix.field) {
	case V4L2_FIELD_INTERLACED:
		/* do nothing, since it is default */
		ret = ccdc_dev->hw_ops.set_buftype(
				CCDC_BUFTYPE_FLD_INTERLEAVED);
		break;
	case V4L2_FIELD_NONE:
		frm_fmt = CCDC_FRMFMT_PROGRESSIVE;
		/* buffer type only applicable for interlaced scan */
		break;
	case V4L2_FIELD_SEQ_TB:
		ret = ccdc_dev->hw_ops.set_buftype(
				CCDC_BUFTYPE_FLD_SEPARATED);
		break;
	default:
		return -EINVAL;
	}

	/* set the frame format */
	if (!ret)
		ret = ccdc_dev->hw_ops.set_frame_format(frm_fmt);
	return ret;
}
/*
 * vpfe_config_image_format()
 * For a given standard, this functions sets up the default
 * pix format & crop values in the vpfe device and ccdc.  It first
 * starts with defaults based values from the standard table.
 * It then checks if sub device support g_fmt and then override the
 * values based on that.Sets crop values to match with scan resolution
 * starting at 0,0. It calls vpfe_config_ccdc_image_format() set the
 * values in ccdc. Not called when sensor is the input source
 */
static int vpfe_config_image_format(struct vpfe_device *vpfe_dev,
				    const v4l2_std_id *std_id)
{
	struct vpfe_subdev_info *sdinfo = vpfe_dev->current_subdev;
	struct v4l2_format sd_fmt;
	int i, ret = 0;

	/* configure the ccdc based on standard */
	for (i = 0; i < ARRAY_SIZE(vpfe_standards); i++) {
		if (vpfe_standards[i].std_id & *std_id) {
			vpfe_dev->std_info.active_pixels =
					vpfe_standards[i].width;
			vpfe_dev->std_info.active_lines =
					vpfe_standards[i].height;
			vpfe_dev->std_info.frame_format =
					vpfe_standards[i].frame_format;
			vpfe_dev->std_info.fps = vpfe_standards[i].fps;
			vpfe_dev->std_index = i;
			break;
		}
	}

	if (i ==  ARRAY_SIZE(vpfe_standards)) {
		v4l2_err(&vpfe_dev->v4l2_dev, "standard not supported\n");
		return -EINVAL;
	}

	vpfe_dev->crop.top = 0;
	vpfe_dev->crop.left = 0;
	vpfe_dev->crop.width = vpfe_dev->std_info.active_pixels;
	vpfe_dev->crop.height = vpfe_dev->std_info.active_lines;
	vpfe_dev->fmt.fmt.pix.width = vpfe_dev->crop.width;
	vpfe_dev->fmt.fmt.pix.height = vpfe_dev->crop.height;

	/* first field and frame format based on standard frame format */
	if (vpfe_dev->std_info.frame_format) {
		vpfe_dev->fmt.fmt.pix.field = V4L2_FIELD_INTERLACED;
		/* assume V4L2_PIX_FMT_UYVY as default */
		vpfe_dev->fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_UYVY;
	} else {
		vpfe_dev->fmt.fmt.pix.field = V4L2_FIELD_NONE;
		vpfe_dev->fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_UYVY;
	}

	sd_fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	/* if sub device supports g_fmt, override the defaults */
	ret = v4l2_device_call_until_err(&vpfe_dev->v4l2_dev,
			sdinfo->grp_id, video, g_fmt, &sd_fmt);

	if (ret && ret != -ENOIOCTLCMD) {
		v4l2_err(&vpfe_dev->v4l2_dev,
			"error in getting g_fmt from sub device\n");
		return ret;
	}
	vpfe_dev->fmt = sd_fmt;
	/* Sets the values in CCDC */
	ret = vpfe_config_ccdc_image_format(vpfe_dev);
	if (ret)
		return ret;

	/* Update the values of sizeimage and bytesperline */
	if (!ret) {
		vpfe_dev->fmt.fmt.pix.bytesperline =
			ccdc_dev->hw_ops.get_line_length();
		vpfe_dev->fmt.fmt.pix.sizeimage =
			vpfe_dev->fmt.fmt.pix.bytesperline *
			vpfe_dev->fmt.fmt.pix.height;
	}
	return ret;
}

/**
 * vpfe_set_format_in_sensor() - Set frame format in the sensor
 * @vpfe_dev - vpfe device object
 *
 * Set the given frame format in the sensor. Assume the sensor
 * supports V4L2_PIX_FMT_SGRBG10
 */
static int vpfe_set_format_in_sensor(struct vpfe_device *vpfe_dev,
				     struct v4l2_format *fmt)
{
	struct vpfe_subdev_info *sdinfo = vpfe_dev->current_subdev;
	struct v4l2_format sd_fmt;
	int ret;

	memset(&sd_fmt, 0, sizeof(sd_fmt));
#ifdef CONFIV_VIDEO_YCBCR
	if(vpfe_dev->current_subdev->ccdc_if_params.if_type == VPFE_YCBCR_SYNC_8)
	{
		sd_fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		sd_fmt.fmt.pix.colorspace = V4L2_COLORSPACE_JPEG;
		sd_fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
		sd_fmt.fmt.pix.width = fmt->fmt.pix.width;
		sd_fmt.fmt.pix.height = fmt->fmt.pix.height;
	}
	//TODO
	else
	{
		sd_fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		sd_fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_SGRBG10;
		sd_fmt.fmt.pix.width = fmt->fmt.pix.width;
		sd_fmt.fmt.pix.height = fmt->fmt.pix.height;
	}
#else
	sd_fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	sd_fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_SGRBG10;
	sd_fmt.fmt.pix.width = fmt->fmt.pix.width;
	sd_fmt.fmt.pix.height = fmt->fmt.pix.height;
#endif
	ret = v4l2_device_call_until_err(&vpfe_dev->v4l2_dev,
			sdinfo->grp_id, video, s_fmt, &sd_fmt);
	return ret;
}

static int vpfe_initialize_device(struct vpfe_device *vpfe_dev)
{
	struct vpfe_subdev_info *sdinfo;
	int ret = 0;

	sdinfo = vpfe_dev->current_subdev;
	/* set first input of current subdevice as the current input */
	vpfe_dev->current_input = 0;
	/*
	 * set default standard. For camera device, we cannot set standard.
	 * So we set it to -1. Otherwise, first entry in the standard is the
	 * is the default
	 */
	if (vpfe_dev->current_subdev->is_camera) {
		vpfe_dev->std_index = -1;
		/* Set the bus/interface parameter for the sub device in ccdc */
		ret = ccdc_dev->hw_ops.set_hw_if_params(&sdinfo->ccdc_if_params);
		if (ret)
			goto unlock_out;
		/*
		 * Configure the vpfe default format information based on ccdc
		 * defaults
		 */
		ret = vpfe_get_ccdc_image_format(vpfe_dev, &vpfe_dev->fmt);
		/* also set the current default format in the sensor */
		if (ret)
			goto out;

		ret = vpfe_set_format_in_sensor(vpfe_dev, &vpfe_dev->fmt);
		/* Get max width and height available for capture from camera */
		if (ret)
			goto out;

		/**
		 * Get the frame information from camera sensor such as maximum
		 * width and height, frame format etc
		 */
		ret = vpfe_get_camera_frame_params(vpfe_dev);

	} else {
		vpfe_dev->std_index = 0;
		/* Configure the default format information */
		ret = vpfe_config_image_format(vpfe_dev,
				&vpfe_standards[vpfe_dev->std_index].std_id);
	}

	if (ret)
		goto out;

	/* now open the ccdc device to initialize it */
	mutex_lock(&ccdc_lock);
	if (NULL == ccdc_dev) {
		v4l2_err(&vpfe_dev->v4l2_dev, "ccdc device not registered\n");
		ret = -ENODEV;
		goto unlock_out;
	}

	if (!try_module_get(ccdc_dev->owner)) {
		v4l2_err(&vpfe_dev->v4l2_dev, "Couldn't lock ccdc module\n");
		ret = -ENODEV;
		goto unlock_out;
	}

	vpfe_dev->imp_chained = 0;
	vpfe_dev->second_output = 0;
	vpfe_dev->second_out_img_sz = 0;
	vpfe_dev->rsz_present = 0;
	vpfe_dev->out_from = VPFE_CCDC_OUT;
	vpfe_dev->skip_frame_count = 1;
	vpfe_dev->skip_frame_count_init = 1;

	/* TODO - revisit for MC */
	if (!(ISNULL(imp_hw_if)) &&
		(imp_hw_if->get_preview_oper_mode() == IMP_MODE_CONTINUOUS)) {
		if (imp_hw_if->get_previewer_config_state()
			== STATE_CONFIGURED) {
#ifdef CONFIG_V2R_DEBUG
			v4l2_dbg(1, debug, &vpfe_dev->v4l2_dev, "IPIPE Chained\n");
			v4l2_info(&vpfe_dev->v4l2_dev, "IPIPE Chained\n");
#endif
			vpfe_dev->imp_chained = 1;
			vpfe_dev->out_from = VPFE_IMP_PREV_OUT;
			if (imp_hw_if->get_resizer_config_state()
				== STATE_CONFIGURED) {
				v4l2_info(&vpfe_dev->v4l2_dev,
					 "Resizer present\n");
				vpfe_dev->rsz_present = 1;
				vpfe_dev->out_from = VPFE_IMP_RSZ_OUT;
				if (imp_hw_if->get_output_state(1)) {
					v4l2_info(&vpfe_dev->v4l2_dev,
						  "second output present\n");
					vpfe_dev->second_output = 1;
					vpfe_dev->second_out_img_sz =
						imp_hw_if->
						get_line_length(1) *
						imp_hw_if->
						get_image_height(1);
				}
			}
		}
	}

	ret = ccdc_dev->hw_ops.open(vpfe_dev->pdev);
	if (!ret)
		vpfe_dev->initialized = 1;
unlock_out:
	mutex_unlock(&ccdc_lock);
out:
	return ret;
}

/*
 * vpfe_open : It creates object of file handle structure and
 * stores it in private_data  member of filepointer
 */
static int vpfe_open(struct file *file)
{
	struct vpfe_device *vpfe_dev = video_drvdata(file);
	struct vpfe_fh *fh;

#ifdef CONFIG_V2R_DEBUG
	v4l2_dbg(1, debug, &vpfe_dev->v4l2_dev, "vpfe_open\n");
#endif
	if (!vpfe_dev->cfg->num_subdevs) {
		v4l2_err(&vpfe_dev->v4l2_dev, "No decoder registered\n");
		return -ENODEV;
	}

	/* Allocate memory for the file handle object */
	fh = kmalloc(sizeof(struct vpfe_fh), GFP_KERNEL);
	if (NULL == fh) {
		v4l2_err(&vpfe_dev->v4l2_dev,
			"unable to allocate memory for file handle object\n");
		return -ENOMEM;
	}
	/* store pointer to fh in private_data member of file */
	file->private_data = fh;
	fh->vpfe_dev = vpfe_dev;
	mutex_lock(&vpfe_dev->lock);
	/* If decoder is not initialized. initialize it */
	if (!vpfe_dev->initialized) {
		if (vpfe_initialize_device(vpfe_dev)) {
			mutex_unlock(&vpfe_dev->lock);
			return -ENODEV;
		}
	}
	/* Increment device usrs counter */
	vpfe_dev->usrs++;
	/* Set io_allowed member to false */
	fh->io_allowed = 0;
	/* Initialize priority of this instance to default priority */
	fh->prio = V4L2_PRIORITY_UNSET;
	v4l2_prio_open(&vpfe_dev->prio, &fh->prio);
	mutex_unlock(&vpfe_dev->lock);
	return 0;
}

static void vpfe_schedule_next_buffer(struct vpfe_device *vpfe_dev)
{
	unsigned long addr;

	vpfe_dev->next_frm = list_entry(vpfe_dev->dma_queue.next,
					struct videobuf_buffer, queue);
	list_del(&vpfe_dev->next_frm->queue);
	vpfe_dev->next_frm->state = VIDEOBUF_ACTIVE;
	addr = videobuf_to_dma_contig(vpfe_dev->next_frm);
	if (vpfe_dev->out_from == VPFE_CCDC_OUT)
		ccdc_dev->hw_ops.setfbaddr(addr);
	else {
		imp_hw_if->update_outbuf1_address(NULL, addr);
		if (vpfe_dev->second_output)
			imp_hw_if->update_outbuf2_address(NULL,
					addr + vpfe_dev->second_off);
	}
}

static void vpfe_schedule_bottom_field(struct vpfe_device *vpfe_dev)
{
	unsigned long addr;

	addr = videobuf_to_dma_contig(vpfe_dev->cur_frm);
	addr += vpfe_dev->field_off;
	ccdc_dev->hw_ops.setfbaddr(addr);
}

static void vpfe_process_buffer_complete(struct vpfe_device *vpfe_dev)
{
	struct timeval timevalue;

	do_gettimeofday(&timevalue);
	vpfe_dev->cur_frm->ts = timevalue;
	vpfe_dev->cur_frm->state = VIDEOBUF_DONE;
	vpfe_dev->cur_frm->size = vpfe_dev->fmt.fmt.pix.sizeimage;
	wake_up_interruptible(&vpfe_dev->cur_frm->done);
	vpfe_dev->cur_frm = vpfe_dev->next_frm;
}
#ifdef CONFIG_VIDEO_YCBCR
extern void ipipeif_set_enable(char en, unsigned int mode);
#endif
/* ISR for VINT0*/
static irqreturn_t vpfe_isr(int irq, void *dev_id)
{
	struct vpfe_device *vpfe_dev = dev_id;
	enum v4l2_field field;
	int fid;

	field = vpfe_dev->fmt.fmt.pix.field;

	/* if streaming not started, don't do anything */
	if (!vpfe_dev->started)
		return IRQ_HANDLED;

	/* only for 6446 this will be applicable */
	if (NULL != ccdc_dev->hw_ops.reset)
		ccdc_dev->hw_ops.reset();

	if (field == V4L2_FIELD_NONE) {
		if (vpfe_dev->imp_chained) {
			vpfe_dev->skip_frame_count--;
			if (!vpfe_dev->skip_frame_count) {
				vpfe_dev->skip_frame_count =
					vpfe_dev->skip_frame_count_init;
#ifdef CONFIG_VIDEO_YCBCR
				//TODO
				if(vpfe_dev->current_subdev->ccdc_if_params.if_type == VPFE_YCBCR_SYNC_8)
				{
					ipipeif_set_enable(1,0);
				}
#endif
				if (imp_hw_if->enable_resize)
					imp_hw_if->enable_resize(1);
			} else {
#ifdef CONFIG_VIDEO_YCBCR
				//TODO
				if(vpfe_dev->current_subdev->ccdc_if_params.if_type == VPFE_YCBCR_SYNC_8)
				{
					ipipeif_set_enable(0,0);
				}
#endif
				if (imp_hw_if->enable_resize)
					imp_hw_if->enable_resize(0);
			}
		} else {
			if (vpfe_dev->cur_frm != vpfe_dev->next_frm)
				vpfe_process_buffer_complete(vpfe_dev);
		
		}
		return IRQ_HANDLED;
	}

	/* interlaced or TB capture check which field we are in hardware */
	fid = ccdc_dev->hw_ops.getfid();

	/* switch the software maintained field id */
	vpfe_dev->field_id ^= 1;
	if (fid == vpfe_dev->field_id) {
		/* we are in-sync here,continue */
		if (fid == 0) {
			/*
			 * One frame is just being captured. If the next frame
			 * is available, release the current frame and move on
			 */
			if (vpfe_dev->cur_frm != vpfe_dev->next_frm)
				vpfe_process_buffer_complete(vpfe_dev);
			/*
			 * based on whether the two fields are stored
			 * interleavely or separately in memory, reconfigure
			 * the CCDC memory address
			 */
			if ((vpfe_dev->out_from == VPFE_CCDC_OUT) &&
			    (field == V4L2_FIELD_SEQ_TB))
				vpfe_schedule_bottom_field(vpfe_dev);

			return IRQ_HANDLED;
		}
		/*
		 * if one field is just being captured configure
		 * the next frame get the next frame from the empty
		 * queue if no frame is available hold on to the
		 * current buffer
		 */
		spin_lock(&vpfe_dev->dma_queue_lock);
		if ((vpfe_dev->out_from == VPFE_CCDC_OUT) &&
		    !list_empty(&vpfe_dev->dma_queue) &&
		    vpfe_dev->cur_frm == vpfe_dev->next_frm)
			vpfe_schedule_next_buffer(vpfe_dev);
		spin_unlock(&vpfe_dev->dma_queue_lock);
	} else if (fid == 0) {
		/*
		 * out of sync. Recover from any hardware out-of-sync.
		 * May loose one frame
		 */
		vpfe_dev->field_id = fid;
	}
	return IRQ_HANDLED;
}

/* vpfe_vdint1_isr - isr handler for VINT1 interrupt */
static irqreturn_t vpfe_vdint1_isr(int irq, void *dev_id)
{
	struct vpfe_device *vpfe_dev = dev_id;

	/* if streaming not started, don't do anything */
	if (!vpfe_dev->started)
		return IRQ_HANDLED;

	spin_lock(&vpfe_dev->dma_queue_lock);
	if ((vpfe_dev->fmt.fmt.pix.field == V4L2_FIELD_NONE) &&
	    !list_empty(&vpfe_dev->dma_queue) &&
	    vpfe_dev->cur_frm == vpfe_dev->next_frm)
		vpfe_schedule_next_buffer(vpfe_dev);
	spin_unlock(&vpfe_dev->dma_queue_lock);
	return IRQ_HANDLED;
}

static irqreturn_t vpfe_imp_dma_isr(int irq, void *dev_id)
{
	struct vpfe_device *vpfe_dev = dev_id;
	int fid;
	enum v4l2_field field;

#ifdef CONFIG_V2R_DEBUG
	v4l2_dbg(1, debug, &vpfe_dev->v4l2_dev, "\nvpfe_imp_dma_isr\n");
#endif

	/* if streaming not started, don't do anything */
	if (!vpfe_dev->started)
		return IRQ_HANDLED;

	field = vpfe_dev->fmt.fmt.pix.field;

	if (field == V4L2_FIELD_NONE) {
		/* handle progressive frame capture */
		if (vpfe_dev->cur_frm != vpfe_dev->next_frm)
			vpfe_process_buffer_complete(vpfe_dev);
	} else {
		fid = ccdc_dev->hw_ops.getfid();

		if (fid == vpfe_dev->field_id) {
			/* we are in-sync here,continue */
			if (fid == 1 && !list_empty(&vpfe_dev->dma_queue) &&
			    vpfe_dev->cur_frm == vpfe_dev->next_frm) {
				spin_lock(&vpfe_dev->dma_queue_lock);
				vpfe_schedule_next_buffer(vpfe_dev);
				spin_unlock(&vpfe_dev->dma_queue_lock);
			}
		}
	}

	return IRQ_HANDLED;
}

static irqreturn_t vpfe_imp_update_isr(int irq, void *dev_id)
{
	struct vpfe_device *vpfe_dev = dev_id;

#ifdef CONFIG_V2R_DEBUG
	v4l2_dbg(1, debug, &vpfe_dev->v4l2_dev, "\nvpfe_imp_update_isr\n");
#endif

	/* if streaming not started, don't do anything */
	if (!vpfe_dev->started)
		return IRQ_HANDLED;

	if (!list_empty(&vpfe_dev->dma_queue) &&
		vpfe_dev->cur_frm == vpfe_dev->next_frm) {
		spin_lock(&vpfe_dev->dma_queue_lock);
		vpfe_schedule_next_buffer(vpfe_dev);
		spin_unlock(&vpfe_dev->dma_queue_lock);
	}

	return IRQ_HANDLED;
}

static void vpfe_detach_irq(struct vpfe_device *vpfe_dev)
{
	enum ccdc_frmfmt frame_format;

	free_irq(vpfe_dev->ccdc_irq0, vpfe_dev);
	if (vpfe_dev->out_from == VPFE_CCDC_OUT) {
		frame_format = ccdc_dev->hw_ops.get_frame_format();
		if (frame_format == CCDC_FRMFMT_PROGRESSIVE)
			free_irq(vpfe_dev->ccdc_irq1, vpfe_dev);
	} else {
		free_irq(vpfe_dev->imp_dma_irq, vpfe_dev);
		if (vpfe_dev->imp_update_irq)
			free_irq(vpfe_dev->imp_update_irq,vpfe_dev);
	}
}

static int vpfe_attach_irq(struct vpfe_device *vpfe_dev)
{
	enum ccdc_frmfmt frame_format;
	int ret;
	enum v4l2_field field;
	
	vpfe_dev->imp_update_irq = 0;

	ret = request_irq(vpfe_dev->ccdc_irq0, vpfe_isr, IRQF_DISABLED,
			  "vpfe_capture0", vpfe_dev);
	if (ret < 0) {
		v4l2_err(&vpfe_dev->v4l2_dev,
			"Error: requesting VINT0 interrupt\n");
		return ret;
	}

	if (vpfe_dev->out_from == VPFE_CCDC_OUT) {
		frame_format = ccdc_dev->hw_ops.get_frame_format();
		if (frame_format == CCDC_FRMFMT_PROGRESSIVE) {
			ret = request_irq(vpfe_dev->ccdc_irq1,
						   vpfe_vdint1_isr,
						   IRQF_DISABLED,
						   "vpfe_capture1", vpfe_dev);
			if (ret < 0) {
				v4l2_err(&vpfe_dev->v4l2_dev,
					"Error: requesting VINT1 interrupt\n");
				free_irq(vpfe_dev->ccdc_irq0, vpfe_dev);
			}
		}
	} else {
		/* output from Previewer/Resizer */
		struct irq_numbers irq_info;
		if (vpfe_dev->rsz_present)
			imp_hw_if->get_rsz_irq(&irq_info);
		else
			imp_hw_if->get_preview_irq(&irq_info);

		field = vpfe_dev->fmt.fmt.pix.field;

		if (field == V4L2_FIELD_NONE) {
			vpfe_dev->imp_update_irq = irq_info.update;
			ret = request_irq(irq_info.update, vpfe_imp_update_isr, IRQF_DISABLED,
					  "Imp_Update_Irq", vpfe_dev);
			if (ret < 0) {
				v4l2_err(&vpfe_dev->v4l2_dev,
					"Error: requesting VINT0 interrupt\n");
				return ret;
			}
		}
		
		vpfe_dev->imp_dma_irq = irq_info.sdram;
		ret = request_irq(irq_info.sdram,
				  vpfe_imp_dma_isr,
				  IRQF_DISABLED,
				  "Imp_Sdram_Irq",
				  vpfe_dev);
		if (ret < 0) {
			v4l2_err(&vpfe_dev->v4l2_dev,
				 "Error: requesting IMP"
				 " IRQ interrupt\n");
			free_irq(vpfe_dev->ccdc_irq0, vpfe_dev);
		}
	}
	return 0;
}

/* vpfe_stop_capture: stop streaming in ccdc/isif */
static void vpfe_stop_capture(struct vpfe_device *vpfe_dev)
{
	vpfe_dev->started = 0;
	ccdc_dev->hw_ops.enable(0);
	if (ccdc_dev->hw_ops.enable_out_to_sdram)
		ccdc_dev->hw_ops.enable_out_to_sdram(0);
	if (vpfe_dev->imp_chained)
		imp_hw_if->enable(0, NULL);
}

/*
 * vpfe_release : This function deletes buffer queue, frees the
 * buffers and the vpfe file  handle
 */
static int vpfe_release(struct file *file)
{
	struct vpfe_device *vpfe_dev = video_drvdata(file);
	struct vpfe_fh *fh = file->private_data;
	struct vpfe_subdev_info *sdinfo;
	int ret;

#ifdef CONFIG_V2R_DEBUG
	v4l2_dbg(1, debug, &vpfe_dev->v4l2_dev, "vpfe_release\n");
#endif

	/* Get the device lock */
	mutex_lock(&vpfe_dev->lock);
	/* if this instance is doing IO */
	if (fh->io_allowed) {
		if (vpfe_dev->started) {
			sdinfo = vpfe_dev->current_subdev;
			ret = v4l2_device_call_until_err(&vpfe_dev->v4l2_dev,
							 sdinfo->grp_id,
							 video, s_stream, 0);
			if (ret && (ret != -ENOIOCTLCMD))
				v4l2_err(&vpfe_dev->v4l2_dev,
					 "stream off failed in subdev\n");
			vpfe_stop_capture(vpfe_dev);
			vpfe_detach_irq(vpfe_dev);
			videobuf_streamoff(&vpfe_dev->buffer_queue);
		}
		vpfe_dev->io_usrs = 0;
		vpfe_dev->numbuffers = config_params.numbuffers;

		if (vpfe_dev->imp_chained) {
			imp_hw_if->enable(0, NULL);
			imp_hw_if->unlock_chain();
		}
	}


	/* Decrement device usrs counter */
	vpfe_dev->usrs--;
	/* Close the priority */
	v4l2_prio_close(&vpfe_dev->prio, &fh->prio);
	/* If this is the last file handle */
	if (!vpfe_dev->usrs) {
		vpfe_dev->initialized = 0;
		if (ccdc_dev->hw_ops.close)
			ccdc_dev->hw_ops.close(vpfe_dev->pdev);
		module_put(ccdc_dev->owner);
	}
	mutex_unlock(&vpfe_dev->lock);
	file->private_data = NULL;
	/* Free memory allocated to file handle object */
	kfree(fh);
	return 0;
}

/*
 * vpfe_mmap : It is used to map kernel space buffers
 * into user spaces
 */
static int vpfe_mmap(struct file *file, struct vm_area_struct *vma)
{
	/* Get the device object and file handle object */
	struct vpfe_device *vpfe_dev = video_drvdata(file);

#ifdef CONFIG_V2R_DEBUG
	v4l2_dbg(1, debug, &vpfe_dev->v4l2_dev, "vpfe_mmap\n");
#endif

	return videobuf_mmap_mapper(&vpfe_dev->buffer_queue, vma);
}

/*
 * vpfe_poll: It is used for select/poll system call
 */
static unsigned int vpfe_poll(struct file *file, poll_table *wait)
{
	struct vpfe_device *vpfe_dev = video_drvdata(file);

#ifdef CONFIG_V2R_DEBUG
	v4l2_dbg(1, debug, &vpfe_dev->v4l2_dev, "vpfe_poll\n");
#endif

	if (vpfe_dev->started)
		return videobuf_poll_stream(file,
					    &vpfe_dev->buffer_queue, wait);
	return 0;
}

static long vpfe_param_handler(struct file *file, void *priv,
		int cmd, void *param)
{
	struct vpfe_device *vpfe_dev = video_drvdata(file);
	int ret = 0;

#ifdef CONFIG_V2R_DEBUG
	v4l2_dbg(1, debug, &vpfe_dev->v4l2_dev, "vpfe_param_handler\n");
#endif

	if (NULL == param) {
#ifdef CONFIG_V2R_DEBUG
		v4l2_dbg(1, debug, &vpfe_dev->v4l2_dev,
			"Invalid user ptr\n");
#endif
	}

	if (vpfe_dev->started) {
		/* only allowed if streaming is not started */
		v4l2_err(&vpfe_dev->v4l2_dev, "device already started\n");
		return -EBUSY;
	}


	switch (cmd) {
	case VPFE_CMD_S_CCDC_RAW_PARAMS:
		v4l2_warn(&vpfe_dev->v4l2_dev,
			  "VPFE_CMD_S_CCDC_RAW_PARAMS: experimental ioctl\n");
		ret = mutex_lock_interruptible(&vpfe_dev->lock);
		if (ret)
			return ret;
		ret = ccdc_dev->hw_ops.set_params(param);
		if (ret) {
#ifdef CONFIG_V2R_DEBUG
			v4l2_dbg(1, debug, &vpfe_dev->v4l2_dev,
				"Error in setting parameters in CCDC\n");
#endif
			goto unlock_out;
		}

		if (vpfe_get_ccdc_image_format(vpfe_dev, &vpfe_dev->fmt) < 0) {
			v4l2_err(&vpfe_dev->v4l2_dev,
				"Invalid image format at CCDC\n");
			ret = -EINVAL;
		}
unlock_out:
		mutex_unlock(&vpfe_dev->lock);
		break;
	case VPFE_CMD_G_CCDC_RAW_PARAMS:
		v4l2_warn(&vpfe_dev->v4l2_dev,
			  "VPFE_CMD_G_CCDC_RAW_PARAMS: experimental ioctl\n");
		if (!ccdc_dev->hw_ops.get_params) {
			ret = -EINVAL;
			break;
		}
		ret = ccdc_dev->hw_ops.get_params(param);
		if (ret) {
#ifdef CONFIG_V2R_DEBUG
			v4l2_dbg(1, debug, &vpfe_dev->v4l2_dev,
				"Error in getting parameters from CCDC\n");
#endif
		}
		break;

	default:
		ret = -EINVAL;
	}
	return ret;
}

static long vpfe_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	if (cmd == VPFE_CMD_S_CCDC_RAW_PARAMS ||
	    cmd == VPFE_CMD_G_CCDC_RAW_PARAMS)
		return vpfe_param_handler(file, file->private_data, cmd,
					 (void *)arg);
	return video_ioctl2(file, cmd, arg);
}

/* vpfe capture driver file operations */
static const struct v4l2_file_operations vpfe_fops = {
	.owner = THIS_MODULE,
	.open = vpfe_open,
	.release = vpfe_release,
	.unlocked_ioctl = vpfe_ioctl,
	.mmap = vpfe_mmap,
	.poll = vpfe_poll
};

/**
 * vpfe_check_format()
 * This function adjust the input pixel format as per hardware
 * capabilities and update the same in pixfmt.
 * Following algorithm used :-
 *
 *	If given pixformat is not in the vpfe list of pix formats or not
 *	supported by the hardware, current value of pixformat in the device
 *	is used
 *	If given field is not supported, then current field is used. If field
 *	is different from current, then it is matched with that from sub device.
 *	Minimum height is 2 lines for interlaced or tb field and 1 line for
 *	progressive. Maximum height is clamped to active active lines of scan
 *	Minimum width is 32 bytes in memory and width is clamped to active
 *	pixels of scan.
 *	bytesperline is a multiple of 32.
 */
static const struct vpfe_pixel_format *
	vpfe_check_format(struct vpfe_device *vpfe_dev,
			  struct v4l2_pix_format *pixfmt)
{
	u32 min_height = 1, min_width = 32, max_width, max_height;
	const struct vpfe_pixel_format *vpfe_pix_fmt;
	u32 pix;
	int temp, found;

	vpfe_pix_fmt = vpfe_lookup_pix_format(pixfmt->pixelformat);
	if (NULL == vpfe_pix_fmt) {
		/*
		 * use current pixel format in the vpfe device. We
		 * will find this pix format in the table
		 */
		pixfmt->pixelformat = vpfe_dev->fmt.fmt.pix.pixelformat;
		vpfe_pix_fmt = vpfe_lookup_pix_format(pixfmt->pixelformat);
	}

	/* check if hw supports it */
	temp = 0;
	found = 0;
	if (vpfe_dev->out_from == VPFE_CCDC_OUT) {
		while (ccdc_dev->hw_ops.enum_pix(&pix, temp) >= 0) {
			if (vpfe_pix_fmt->fmtdesc.pixelformat == pix) {
				found = 1;
				break;
			}
			temp++;
		}
	} else {
		if (imp_hw_if) {
			while (imp_hw_if->enum_pix(&pix, temp) >= 0) {
				if (vpfe_pix_fmt->fmtdesc.pixelformat == pix) {
					found = 1;
					break;
				}
				temp++;
			}
		}
	}

	if (!found) {
		/* use current pixel format */
		pixfmt->pixelformat = vpfe_dev->fmt.fmt.pix.pixelformat;
		/*
		 * Since this is currently used in the vpfe device, we
		 * will find this pix format in the table
		 */
		vpfe_pix_fmt = vpfe_lookup_pix_format(pixfmt->pixelformat);
	}

	/* check what field format is supported */
	if (pixfmt->field == V4L2_FIELD_ANY) {
		/* if field is any, use current value as default */
		pixfmt->field = vpfe_dev->fmt.fmt.pix.field;
	}

	/*
	 * if field is not same as current field in the vpfe device
	 * try matching the field with the sub device field
	 */
	if (vpfe_dev->fmt.fmt.pix.field != pixfmt->field) {
		/*
		 * If field value is not in the supported fields, use current
		 * field used in the device as default
		 */
		switch (pixfmt->field) {
		case V4L2_FIELD_INTERLACED:
		case V4L2_FIELD_SEQ_TB:
			/* if sub device is supporting progressive, use that */
			if (!vpfe_dev->std_info.frame_format)
				pixfmt->field = V4L2_FIELD_NONE;
			break;
		case V4L2_FIELD_NONE:
			if (vpfe_dev->std_info.frame_format)
				pixfmt->field = V4L2_FIELD_INTERLACED;
			break;

		default:
			/* use current field as default */
			pixfmt->field = vpfe_dev->fmt.fmt.pix.field;
			break;
		}
	}

	/* Now adjust image resolutions supported */
	if (pixfmt->field == V4L2_FIELD_INTERLACED ||
	    pixfmt->field == V4L2_FIELD_SEQ_TB)
		min_height = 2;

	max_width = vpfe_dev->std_info.active_pixels;
	max_height = vpfe_dev->std_info.active_lines;
	if (vpfe_dev->imp_chained) {
		/* check with imp hw for the limits */
		max_width  = imp_hw_if->get_max_output_width(0);
		max_height = imp_hw_if->get_max_output_height(0);
	}

	min_width /= vpfe_pix_fmt->bpp;

#ifdef CONFIG_V2R_DEBUG
	v4l2_dbg(1, debug, &vpfe_dev->v4l2_dev, "width = %d, height = %d, bpp = %d\n",
		  pixfmt->width, pixfmt->height, vpfe_pix_fmt->bpp);
#endif

	pixfmt->width = clamp((pixfmt->width), min_width, max_width);
	pixfmt->height = clamp((pixfmt->height), min_height, max_height);

	/* If interlaced, adjust height to be a multiple of 2 */
	if (pixfmt->field == V4L2_FIELD_INTERLACED)
		pixfmt->height &= (~1);
	/*
	 * recalculate bytesperline and sizeimage since width
	 * and height might have changed
	 */
	pixfmt->bytesperline = (((pixfmt->width * vpfe_pix_fmt->bpp) + 31)
				& ~31);
	if (pixfmt->pixelformat == V4L2_PIX_FMT_NV12)
		pixfmt->sizeimage =
			pixfmt->bytesperline * pixfmt->height +
			((pixfmt->bytesperline * pixfmt->height) >> 1);
	else
		pixfmt->sizeimage = pixfmt->bytesperline * pixfmt->height;

#ifdef CONFIG_V2R_DEBUG
	v4l2_dbg(1, debug, &vpfe_dev->v4l2_dev, "adjusted width = %d, height ="
		 " %d, bpp = %d, bytesperline = %d, sizeimage = %d\n",
		 pixfmt->width, pixfmt->height, vpfe_pix_fmt->bpp,
		 pixfmt->bytesperline, pixfmt->sizeimage);
#endif
	return vpfe_pix_fmt;
}

static int vpfe_querycap(struct file *file, void  *priv,
			       struct v4l2_capability *cap)
{
	struct vpfe_device *vpfe_dev = video_drvdata(file);

#ifdef CONFIG_V2R_DEBUG
	v4l2_dbg(1, debug, &vpfe_dev->v4l2_dev, "vpfe_querycap\n");
#endif

	cap->version = VPFE_CAPTURE_VERSION_CODE;
	cap->capabilities = V4L2_CAP_VIDEO_CAPTURE | V4L2_CAP_STREAMING;
	strlcpy(cap->driver, CAPTURE_DRV_NAME, sizeof(cap->driver));
	strlcpy(cap->bus_info, "VPFE", sizeof(cap->bus_info));
	strlcpy(cap->card, vpfe_dev->cfg->card_name, sizeof(cap->card));
	return 0;
}

static int vpfe_g_fmt_vid_cap(struct file *file, void *priv,
				struct v4l2_format *fmt)
{
	struct vpfe_device *vpfe_dev = video_drvdata(file);
	int ret = 0;

#ifdef CONFIG_V2R_DEBUG
	v4l2_dbg(1, debug, &vpfe_dev->v4l2_dev, "vpfe_g_fmt_vid_cap\n");
#endif
	/* Fill in the information about format */
	*fmt = vpfe_dev->fmt;
	return ret;
}

static int vpfe_enum_fmt_vid_cap(struct file *file, void  *priv,
				   struct v4l2_fmtdesc *fmt)
{
	struct vpfe_device *vpfe_dev = video_drvdata(file);
	const struct vpfe_pixel_format *pix_fmt;
	int temp_index;
	u32 pix;

#ifdef CONFIG_V2R_DEBUG
	v4l2_dbg(1, debug, &vpfe_dev->v4l2_dev, "vpfe_enum_fmt_vid_cap\n");
#endif

	if (!vpfe_dev->imp_chained) {
		if (ccdc_dev->hw_ops.enum_pix(&pix, fmt->index) < 0)
			return -EINVAL;
	} else {
		/**
		 * Based on resizer present or not or not, imp module will
		 * enumerate pixel format available at previewer output
		 * or resizer output based on interface type
		 */
		if (imp_hw_if->enum_pix(&pix, fmt->index) < 0)
			return -EINVAL;
	}

	/* Fill in the information about format */
	pix_fmt = vpfe_lookup_pix_format(pix);
	if (NULL != pix_fmt) {
		temp_index = fmt->index;
		*fmt = pix_fmt->fmtdesc;
		fmt->index = temp_index;
		return 0;
	}
	return -EINVAL;
}

/**
 * vpfe_config_imp_image_format - Setup image format in image processor
 * @vpfe_dev: vpfe device object
 *
 * Configure the input and output pixel format, input crop area and output
 * frame sizes in the image processor. This is called during S_INPUT, S_CROP
 * and S_FMT ioctls.
 */
static int vpfe_config_imp_image_format(struct vpfe_device *vpfe_dev)
{
	struct vpfe_subdev_info *sdinfo =
			vpfe_dev->current_subdev;
	int ret = -EINVAL, bytesperline;
	enum imp_pix_formats imp_pix;
	struct imp_window imp_win;

	/* first setup input and output pixel formats */
#ifdef CONFIG_VIDEO_YCBCR
	if (sdinfo->is_camera && sdinfo->ccdc_if_params.if_type == VPFE_YCBCR_SYNC_8)
		imp_pix = IMP_YUYV;
	//TODO
	else
		imp_pix = IMP_BAYER;
#else
	if (sdinfo->is_camera)
		imp_pix = IMP_BAYER;
	else
		imp_pix = IMP_UYVY;
#endif
	if (imp_hw_if->set_in_pixel_format(imp_pix) < 0) {
		v4l2_err(&vpfe_dev->v4l2_dev,
			"Couldn't set in pix format at IMP\n");
		goto imp_exit;
	}

	if (vpfe_dev->fmt.fmt.pix.pixelformat == V4L2_PIX_FMT_SBGGR16)
		imp_pix = IMP_BAYER;
	else if (vpfe_dev->fmt.fmt.pix.pixelformat == V4L2_PIX_FMT_UYVY)
		imp_pix = IMP_UYVY;
	else if (vpfe_dev->fmt.fmt.pix.pixelformat == V4L2_PIX_FMT_NV12)
		imp_pix = IMP_YUV420SP;
	else {
		v4l2_err(&vpfe_dev->v4l2_dev,
			"pixel format not supported at IMP\n");
		goto imp_exit;
	}

	if (imp_hw_if->set_out_pixel_format(imp_pix) < 0) {
		v4l2_err(&vpfe_dev->v4l2_dev, "pixel format not supported"
			 " at IMP\n");
		goto imp_exit;
	}

	if (vpfe_dev->fmt.fmt.pix.field == V4L2_FIELD_INTERLACED) {
		imp_hw_if->set_buftype(0);
		imp_hw_if->set_frame_format(0);
		ccdc_dev->hw_ops.set_frame_format(CCDC_FRMFMT_INTERLACED);
	} else if (vpfe_dev->fmt.fmt.pix.field == V4L2_FIELD_NONE) {
		imp_hw_if->set_frame_format(1);
		ccdc_dev->hw_ops.set_frame_format(CCDC_FRMFMT_PROGRESSIVE);
	} else {
		v4l2_err(&vpfe_dev->v4l2_dev, "\n field error!");
		goto imp_exit;
	}

	/**
	 * Check if we have resizer. Otherwise don't allow crop size to
	 * be different from image size
	 */
	imp_win.width = vpfe_dev->crop.width;
	imp_win.height = vpfe_dev->crop.height;
	imp_win.hst = vpfe_dev->crop.left;
	/* vst start from 1 */
	imp_win.vst = vpfe_dev->crop.top + 1;
	if (imp_hw_if->set_input_win(&imp_win) < 0) {
		v4l2_err(&vpfe_dev->v4l2_dev, "Error in setting crop window"
			 " in IMP\n");
		goto imp_exit;
	}

	/* Set output */
	imp_win.width = vpfe_dev->fmt.fmt.pix.width;
	imp_win.height = vpfe_dev->fmt.fmt.pix.height;
	imp_win.hst = 0;
	imp_win.vst = 0;
	if (imp_hw_if->set_output_win(&imp_win) < 0) {
		v4l2_err(&vpfe_dev->v4l2_dev, "Error in setting image window"
			 " in IMP\n");
		goto imp_exit;
	}

	bytesperline = imp_hw_if->get_line_length(0);
	if (bytesperline !=
		vpfe_dev->fmt.fmt.pix.bytesperline) {
		v4l2_err(&vpfe_dev->v4l2_dev, "Mismatch between bytesperline"
			"at IMP and vpfe\n");
		goto imp_exit;
	}

	if (imp_hw_if->get_output_state(1))
		vpfe_dev->second_out_img_sz = imp_hw_if->get_line_length(1) *
					    imp_hw_if->get_image_height(1);
	ret = 0;
imp_exit:
	return ret;
}

static int vpfe_s_fmt_vid_cap(struct file *file, void *priv,
				struct v4l2_format *fmt)
{
	struct vpfe_device *vpfe_dev = video_drvdata(file);
	const struct vpfe_pixel_format *pix_fmts;
	struct vpfe_subdev_info *sdinfo;
	int ret = 0;

#ifdef CONFIG_V2R_DEBUG
	v4l2_dbg(1, debug, &vpfe_dev->v4l2_dev, "vpfe_s_fmt_vid_cap\n");
#endif

	/* If streaming is started, return error */
	if (vpfe_dev->started) {
		v4l2_err(&vpfe_dev->v4l2_dev, "Streaming is started\n");
		return -EBUSY;
	}

	/* Check for valid frame format */
	pix_fmts = vpfe_check_format(vpfe_dev, &fmt->fmt.pix);

	if (NULL == pix_fmts)
		return -EINVAL;

	/* store the pixel format in the device  object */
	ret = mutex_lock_interruptible(&vpfe_dev->lock);
	if (ret)
		return ret;

	sdinfo = vpfe_dev->current_subdev;
	if (sdinfo->is_camera) {
		/*
		 * TODO. Current implementation of camera sub device calculates
		 * sensor timing values based on S_FMT. So we need to
		 * explicitely call S_FMT first and make sure it succeeds before
		 * setting capture parameters in ccdc. Assuming sensor supports
		 * V4L2_PIX_FMT_SGRBG10
		 */
		ret = vpfe_set_format_in_sensor(vpfe_dev, fmt);
		if (!ret) {
			/**
			 * Set Crop size to frame size. Application needs to call
			 * S_CROP to change it after S_FMT
			 */
			vpfe_dev->crop.width = fmt->fmt.pix.width;
			vpfe_dev->crop.height = fmt->fmt.pix.height;
		} else
			goto s_fmt_out;
	}

	vpfe_dev->fmt = *fmt;

	if (!vpfe_dev->imp_chained) {
		/*
		 * Set Crop size to frame size when only ccdc is involved.
		 * Application needs to cal S_CROP to change it after S_FMT
		 */
		vpfe_dev->crop.width = fmt->fmt.pix.width;
		vpfe_dev->crop.height = fmt->fmt.pix.height;

		/* set image capture parameters in the ccdc if */
		ret = vpfe_config_ccdc_image_format(vpfe_dev);
	} else {
		/*
		 * currently S_FMT does scaling at the sensor and input to
		 * to CCDC is this scaled output for camera capture. So SoC
		 * resizer can be used to zoom/scale up a rectangle of input
		 * frame inside the received frame boundary by setting S_CROP.
		 * But for decoders like tvp7002, we set ccdc sizes based on
		 * given standard and use image processor to scale it down or
		 * up. So processing is different for both cases
		 */
		if (vpfe_dev->current_subdev->is_camera)
			ret = vpfe_config_ccdc_image_format(vpfe_dev);

		if (!ret)
			ret = vpfe_config_imp_image_format(vpfe_dev);
	}

s_fmt_out:
	mutex_unlock(&vpfe_dev->lock);
	return ret;
}

static int vpfe_try_fmt_vid_cap(struct file *file, void *priv,
				  struct v4l2_format *f)
{
	struct vpfe_device *vpfe_dev = video_drvdata(file);
	const struct vpfe_pixel_format *pix_fmts;

#ifdef CONFIG_V2R_DEBUG
	v4l2_dbg(1, debug, &vpfe_dev->v4l2_dev, "vpfe_try_fmt_vid_cap\n");
#endif

	pix_fmts = vpfe_check_format(vpfe_dev, &f->fmt.pix);
	if (NULL == pix_fmts)
		return -EINVAL;
	return 0;
}

/*
 * vpfe_get_subdev_input_index - Get subdev index and subdev input index for a
 * given app input index
 */
static int vpfe_get_subdev_input_index(struct vpfe_device *vpfe_dev,
					int *subdev_index,
					int *subdev_input_index,
					int app_input_index)
{
	struct vpfe_config *cfg = vpfe_dev->cfg;
	struct vpfe_subdev_info *sdinfo;
	int i, j = 0;

	for (i = 0; i < cfg->num_subdevs; i++) {
		sdinfo = &cfg->sub_devs[i];
		if (app_input_index < (j + sdinfo->num_inputs)) {
			*subdev_index = i;
			*subdev_input_index = app_input_index - j;
			return 0;
		}
		j += sdinfo->num_inputs;
	}
	return -EINVAL;
}

/*
 * vpfe_get_app_input - Get app input index for a given subdev input index
 * driver stores the input index of the current sub device and translate it
 * when application request the current input
 */
static int vpfe_get_app_input_index(struct vpfe_device *vpfe_dev,
				    int *app_input_index)
{
	struct vpfe_config *cfg = vpfe_dev->cfg;
	struct vpfe_subdev_info *sdinfo;
	int i, j = 0;

	for (i = 0; i < cfg->num_subdevs; i++) {
		sdinfo = &cfg->sub_devs[i];
		if (!strcmp(sdinfo->module_name,
		     vpfe_dev->current_subdev->module_name)) {
			if (vpfe_dev->current_input >= sdinfo->num_inputs)
				return -EINVAL;
			*app_input_index = j + vpfe_dev->current_input;
			return 0;
		}
		j += sdinfo->num_inputs;
	}
	return -EINVAL;
}

static int vpfe_enum_input(struct file *file, void *priv,
				 struct v4l2_input *inp)
{
	struct vpfe_device *vpfe_dev = video_drvdata(file);
	struct vpfe_subdev_info *sdinfo;
	int subdev, index, temp_index;

#ifdef CONFIG_V2R_DEBUG
	v4l2_dbg(1, debug, &vpfe_dev->v4l2_dev, "vpfe_enum_input\n");
#endif

	temp_index = inp->index;
	if (vpfe_get_subdev_input_index(vpfe_dev,
					&subdev,
					&index,
					inp->index) < 0) {
#ifdef CONFIG_V2R_DEBUG
		v4l2_dbg(1, debug, &vpfe_dev->v4l2_dev, "input information not found"
			 " for the subdev\n");
#endif
		return -EINVAL;
	}
	sdinfo = &vpfe_dev->cfg->sub_devs[subdev];
	memcpy(inp, &sdinfo->inputs[index], sizeof(struct v4l2_input));
	inp->index = temp_index;
	return 0;
}

static int vpfe_g_input(struct file *file, void *priv, unsigned int *index)
{
	struct vpfe_device *vpfe_dev = video_drvdata(file);

#ifdef CONFIG_V2R_DEBUG
	v4l2_dbg(1, debug, &vpfe_dev->v4l2_dev, "vpfe_g_input\n");
#endif

	return vpfe_get_app_input_index(vpfe_dev, index);
}

static int vpfe_s_input(struct file *file, void *priv, unsigned int index)
{
	struct vpfe_device *vpfe_dev = video_drvdata(file);
	int subdev_index, subdev_inp_index, ret;
	struct vpfe_subdev_info *sdinfo;
	struct vpfe_route *route;
	u32 input = 0, output = 0;

#ifdef CONFIG_V2R_DEBUG
	v4l2_dbg(1, debug, &vpfe_dev->v4l2_dev, "vpfe_s_input\n");
#endif

	ret = mutex_lock_interruptible(&vpfe_dev->lock);
	if (ret)
		return ret;

	/*
	 * If streaming is started return device busy
	 * error
	 */
	if (vpfe_dev->started) {
		v4l2_err(&vpfe_dev->v4l2_dev, "Streaming is on\n");
		ret = -EBUSY;
		goto unlock_out;
	}

	if (vpfe_get_subdev_input_index(vpfe_dev,
					&subdev_index,
					&subdev_inp_index,
					index) < 0) {
		v4l2_err(&vpfe_dev->v4l2_dev, "invalid input index\n");
		ret = -EINVAL;
		goto unlock_out;
	}

	sdinfo = &vpfe_dev->cfg->sub_devs[subdev_index];

	if (!sdinfo->registered) {
		ret = -EINVAL;
		goto unlock_out;
	}

	if (vpfe_dev->cfg->setup_input) {
		if (vpfe_dev->cfg->setup_input(sdinfo->grp_id) < 0) {
			ret = -EFAULT;
#ifdef CONFIG_V2R_DEBUG
			v4l2_dbg(1, debug, &vpfe_dev->v4l2_dev,
				 "couldn't setup input for %s\n",
				 sdinfo->module_name);
#endif
			goto unlock_out;
		}
	}

	route = &sdinfo->routes[subdev_inp_index];
	if (route && sdinfo->can_route) {
		input = route->input;
		output = route->output;
		ret = v4l2_device_call_until_err(&vpfe_dev->v4l2_dev,
						 sdinfo->grp_id, video,
						 s_routing, input, output, 0);

		if (ret) {
#ifdef CONFIG_V2R_DEBUG
			v4l2_dbg(1, debug, &vpfe_dev->v4l2_dev,
				"s_input:error in setting input in"
				" decoder \n");
#endif
			ret = -EINVAL;
			goto unlock_out;
		}
	}

	vpfe_dev->current_subdev = sdinfo;
	vpfe_dev->current_input = subdev_inp_index;

	/* set the bus/interface parameter for the sub device in ccdc */
	ret = ccdc_dev->hw_ops.set_hw_if_params(&sdinfo->ccdc_if_params);
	if (ret)
		goto unlock_out;

	/* update the if parameters to imp hw interface */
	if (imp_hw_if && imp_hw_if->set_hw_if_param)
		ret = imp_hw_if->set_hw_if_param(&sdinfo->ccdc_if_params);
	if (ret)
		goto unlock_out;

	/* set the default image parameters in the device */
	if (vpfe_dev->current_subdev->is_camera) {
		vpfe_dev->std_index = -1;
		/* for camera, use ccdc default parameters */
		ret = vpfe_get_ccdc_image_format(vpfe_dev, &vpfe_dev->fmt);
		/* also set the current default format in the sensor */
		if (ret)
			goto unlock_out;

		ret = vpfe_set_format_in_sensor(vpfe_dev, &vpfe_dev->fmt);
	} else {
		vpfe_dev->std_index = 0;
		/*
		 * For non-camera sub device, use standard to configure vpfe
		 * default
		 */
		ret = vpfe_config_image_format(vpfe_dev,
				&vpfe_standards[vpfe_dev->std_index].std_id);
	}
unlock_out:
	mutex_unlock(&vpfe_dev->lock);
	return ret;
}

static int vpfe_querystd(struct file *file, void *priv, v4l2_std_id *std_id)
{
	struct vpfe_device *vpfe_dev = video_drvdata(file);
	struct vpfe_subdev_info *sdinfo;
	int ret = 0;

#ifdef CONFIG_V2R_DEBUG
	v4l2_dbg(1, debug, &vpfe_dev->v4l2_dev, "vpfe_querystd\n");
#endif

	ret = mutex_lock_interruptible(&vpfe_dev->lock);
	sdinfo = vpfe_dev->current_subdev;
	if (ret)
		return ret;
	/* Call querystd function of decoder device */
	ret = v4l2_device_call_until_err(&vpfe_dev->v4l2_dev, sdinfo->grp_id,
					 video, querystd, std_id);
	mutex_unlock(&vpfe_dev->lock);
	return ret;
}

static int vpfe_s_std(struct file *file, void *priv, v4l2_std_id *std_id)
{
	struct vpfe_device *vpfe_dev = video_drvdata(file);
	struct vpfe_subdev_info *sdinfo;
	int ret = 0;

#ifdef CONFIG_V2R_DEBUG
	v4l2_dbg(1, debug, &vpfe_dev->v4l2_dev, "vpfe_s_std\n");
#endif

	/* Call decoder driver function to set the standard */
	ret = mutex_lock_interruptible(&vpfe_dev->lock);
	if (ret)
		return ret;

	sdinfo = vpfe_dev->current_subdev;
	/* If streaming is started, return device busy error */
	if (vpfe_dev->started) {
		v4l2_err(&vpfe_dev->v4l2_dev, "streaming is started\n");
		ret = -EBUSY;
		goto unlock_out;
	}

	/* Set filter value in ths7353 for TVP7002 input path */
	if (sdinfo->grp_id == VPFE_SUBDEV_TVP7002) {
		ret = v4l2_device_call_until_err(&vpfe_dev->v4l2_dev,
				sdinfo->grp_id, video, s_std_output, *std_id);
		if (ret < 0) {
			v4l2_err(&vpfe_dev->v4l2_dev, "Failed to set filter for THS7353\n");
			goto unlock_out;
		}
	}

	ret = v4l2_device_call_until_err(&vpfe_dev->v4l2_dev, sdinfo->grp_id,
					 core, s_std, *std_id);
	if (ret < 0) {
		v4l2_err(&vpfe_dev->v4l2_dev, "Failed to set standard\n");
		goto unlock_out;
	}
	ret = vpfe_config_image_format(vpfe_dev, std_id);

unlock_out:
	mutex_unlock(&vpfe_dev->lock);
	return ret;
}

static int vpfe_g_std(struct file *file, void *priv, v4l2_std_id *std_id)
{
	struct vpfe_device *vpfe_dev = video_drvdata(file);

#ifdef CONFIG_V2R_DEBUG
	v4l2_dbg(1, debug, &vpfe_dev->v4l2_dev, "vpfe_g_std\n");
#endif

	if (vpfe_dev->std_index < 0 ||
	    vpfe_dev->std_index >= ARRAY_SIZE(vpfe_standards)) {
#ifdef CONFIG_V2R_DEBUG
		v4l2_dbg(1, debug, &vpfe_dev->v4l2_dev, "Standard not supported\n");
#endif
		return -EINVAL;
	}
	*std_id = vpfe_standards[vpfe_dev->std_index].std_id;
	return 0;
}

/*
 *  Videobuf operations
 */
static int vpfe_videobuf_setup(struct videobuf_queue *vq,
				unsigned int *count,
				unsigned int *size)
{
	struct vpfe_fh *fh = vq->priv_data;
	struct vpfe_device *vpfe_dev = fh->vpfe_dev;

#ifdef CONFIG_V2R_DEBUG
	v4l2_dbg(1, debug, &vpfe_dev->v4l2_dev, "vpfe_buffer_setup\n");
#endif

	/*
	 * if we are using mmap, check the size of the allocated buffer is less
	 * than or equal to the maximum specified in the driver. Assume here the
	 * user has called S_FMT and sizeimage has been calculated.
	 */
	*size = vpfe_dev->fmt.fmt.pix.sizeimage;
	if (vpfe_dev->second_output)
		*size += vpfe_dev->second_out_img_sz;

	if (vpfe_dev->memory == V4L2_MEMORY_MMAP) {
		/* Limit maximum to what is configured */
		if (*size > config_params.device_bufsize){
			*size = config_params.device_bufsize;
			printk("Limiting v4l2 output buffer size %d\n",*size);
		}
	}

	if (config_params.video_limit) {
		while (*size * *count > config_params.video_limit)
			(*count)--;
	}

	if (*count < config_params.min_numbuffers)
		*count = config_params.min_numbuffers;

#ifdef CONFIG_V2R_DEBUG
	v4l2_dbg(1, debug, &vpfe_dev->v4l2_dev,
		"count=%d, size=%d\n", *count, *size);
#endif
	return 0;
}

static int vpfe_videobuf_prepare(struct videobuf_queue *vq,
				struct videobuf_buffer *vb,
				enum v4l2_field field)
{
	struct vpfe_fh *fh = vq->priv_data;
	struct vpfe_device *vpfe_dev = fh->vpfe_dev;
	unsigned long addr;
	int ret;

	#ifdef CONFIG_V2R_DEBUG
	v4l2_dbg(1, debug, &vpfe_dev->v4l2_dev, "vpfe_videobuf_prepare\n");
	#endif

	/* If buffer is not initialized, initialize it */
	if (VIDEOBUF_NEEDS_INIT == vb->state) {
		vb->width = vpfe_dev->fmt.fmt.pix.width;
		vb->height = vpfe_dev->fmt.fmt.pix.height;
		vb->size = vpfe_dev->fmt.fmt.pix.sizeimage;
		vb->field = field;

		ret = videobuf_iolock(vq, vb, NULL);
		if (ret < 0)
			return ret;

		addr = videobuf_to_dma_contig(vb);
		/* Make sure user addresses are aligned to 32 bytes */
		if (!ALIGN(addr, 32))
			return -EINVAL;

		vb->state = VIDEOBUF_PREPARED;
	}

	return 0;
}

static void vpfe_videobuf_queue(struct videobuf_queue *vq,
				struct videobuf_buffer *vb)
{
	/* Get the file handle object and device object */
	struct vpfe_fh *fh = vq->priv_data;
	struct vpfe_device *vpfe_dev = fh->vpfe_dev;
	unsigned long flags;

	/* add the buffer to the DMA queue */
	spin_lock_irqsave(&vpfe_dev->dma_queue_lock, flags);
	list_add_tail(&vb->queue, &vpfe_dev->dma_queue);
	spin_unlock_irqrestore(&vpfe_dev->dma_queue_lock, flags);

	/* Change state of the buffer */
	vb->state = VIDEOBUF_QUEUED;
}

static void vpfe_videobuf_release(struct videobuf_queue *vq,
				  struct videobuf_buffer *vb)
{
	struct vpfe_fh *fh = vq->priv_data;
	struct vpfe_device *vpfe_dev = fh->vpfe_dev;

#ifdef CONFIG_V2R_DEBUG
	v4l2_dbg(1, debug, &vpfe_dev->v4l2_dev, "vpfe_videobuf_release\n");
#endif

	if (vpfe_dev->memory == V4L2_MEMORY_MMAP)
		videobuf_dma_contig_free(vq, vb);
	vb->state = VIDEOBUF_NEEDS_INIT;
}

static struct videobuf_queue_ops vpfe_videobuf_qops = {
	.buf_setup      = vpfe_videobuf_setup,
	.buf_prepare    = vpfe_videobuf_prepare,
	.buf_queue      = vpfe_videobuf_queue,
	.buf_release    = vpfe_videobuf_release,
};

/*
 * vpfe_reqbufs. currently support REQBUF only once opening
 * the device.
 */
static int vpfe_reqbufs(struct file *file, void *priv,
			struct v4l2_requestbuffers *req_buf)
{
	struct vpfe_device *vpfe_dev = video_drvdata(file);
	struct vpfe_fh *fh = file->private_data;
	int ret = 0;

#ifdef CONFIG_V2R_DEBUG
	v4l2_dbg(1, debug, &vpfe_dev->v4l2_dev, "vpfe_reqbufs\n");
#endif

	if (V4L2_BUF_TYPE_VIDEO_CAPTURE != req_buf->type) {
		v4l2_err(&vpfe_dev->v4l2_dev, "Invalid buffer type\n");
		return -EINVAL;
	}

	ret = mutex_lock_interruptible(&vpfe_dev->lock);
	if (ret)
		return ret;

	if (vpfe_dev->io_usrs != 0) {
		v4l2_err(&vpfe_dev->v4l2_dev, "Only one IO user allowed\n");
		ret = -EBUSY;
		goto unlock_out;
	}

	vpfe_dev->memory = req_buf->memory;
	videobuf_queue_dma_contig_init(&vpfe_dev->buffer_queue,
				&vpfe_videobuf_qops,
				vpfe_dev->pdev,
				&vpfe_dev->irqlock,
				req_buf->type,
				vpfe_dev->fmt.fmt.pix.field,
				sizeof(struct videobuf_buffer),
				fh);

	fh->io_allowed = 1;
	vpfe_dev->io_usrs = 1;
	INIT_LIST_HEAD(&vpfe_dev->dma_queue);
	ret = videobuf_reqbufs(&vpfe_dev->buffer_queue, req_buf);
	if (!ret && vpfe_dev->imp_chained)
		imp_hw_if->lock_chain();
unlock_out:
	mutex_unlock(&vpfe_dev->lock);
	return ret;
}

static int vpfe_querybuf(struct file *file, void *priv,
			 struct v4l2_buffer *buf)
{
	struct vpfe_device *vpfe_dev = video_drvdata(file);

#ifdef CONFIG_V2R_DEBUG
	v4l2_dbg(1, debug, &vpfe_dev->v4l2_dev, "vpfe_querybuf\n");
#endif

	if (V4L2_BUF_TYPE_VIDEO_CAPTURE != buf->type) {
		v4l2_err(&vpfe_dev->v4l2_dev, "Invalid buf type\n");
		return  -EINVAL;
	}

	if (vpfe_dev->memory != V4L2_MEMORY_MMAP) {
		v4l2_err(&vpfe_dev->v4l2_dev, "Invalid memory\n");
		return -EINVAL;
	}
	/* Call videobuf_querybuf to get information */
	return videobuf_querybuf(&vpfe_dev->buffer_queue, buf);
}

static int vpfe_qbuf(struct file *file, void *priv,
		     struct v4l2_buffer *p)
{
	struct vpfe_device *vpfe_dev = video_drvdata(file);
	struct vpfe_fh *fh = file->private_data;

#ifdef CONFIG_V2R_DEBUG
	v4l2_dbg(1, debug, &vpfe_dev->v4l2_dev, "vpfe_qbuf\n");
#endif

	if (V4L2_BUF_TYPE_VIDEO_CAPTURE != p->type) {
		v4l2_err(&vpfe_dev->v4l2_dev, "Invalid buf type\n");
		return -EINVAL;
	}

	/*
	 * If this file handle is not allowed to do IO,
	 * return error
	 */
	if (!fh->io_allowed) {
		v4l2_err(&vpfe_dev->v4l2_dev, "fh->io_allowed\n");
		return -EACCES;
	}
	return videobuf_qbuf(&vpfe_dev->buffer_queue, p);
}

static int vpfe_dqbuf(struct file *file, void *priv,
		      struct v4l2_buffer *buf)
{
	struct vpfe_device *vpfe_dev = video_drvdata(file);

#ifdef CONFIG_V2R_DEBUG
	v4l2_dbg(1, debug, &vpfe_dev->v4l2_dev, "vpfe_dqbuf\n");
#endif

	if (V4L2_BUF_TYPE_VIDEO_CAPTURE != buf->type) {
		v4l2_err(&vpfe_dev->v4l2_dev, "Invalid buf type\n");
		return -EINVAL;
	}
	return videobuf_dqbuf(&vpfe_dev->buffer_queue,
				      buf, file->f_flags & O_NONBLOCK);
}

/**
 * vpfe_calculate_offsets : This function calculates buffers offset
 * @vpfe_dev - device object
 *
 * This function calculates field and second image offsets
 */
static void vpfe_calculate_offsets(struct vpfe_device *vpfe_dev)
{
	struct v4l2_rect image_win;

#ifdef CONFIG_V2R_DEBUG
	v4l2_dbg(1, debug, &vpfe_dev->v4l2_dev, "vpfe_calculate_offsets\n");
#endif

	vpfe_dev->field_off = 0;
	vpfe_dev->second_off = 0;
	if (!vpfe_dev->imp_chained) {
		ccdc_dev->hw_ops.get_image_window(&image_win);
		vpfe_dev->field_off = image_win.height * image_win.width;

	} else {
		if (vpfe_dev->second_output)
			vpfe_dev->second_off = vpfe_dev->fmt.fmt.pix.sizeimage;
	}
	vpfe_dev->field_off = (vpfe_dev->field_off + 31) & ~0x1f;
	/* Adjust the second offset to 32 byte boundary */
	vpfe_dev->second_off = (vpfe_dev->second_off + 31) & ~0x1f;
}

/* vpfe_start_ccdc_capture: start streaming in ccdc/isif */
static void vpfe_start_capture(struct vpfe_device *vpfe_dev)
{
	if (ccdc_dev->hw_ops.enable_out_to_sdram)
#ifdef CONFIG_VIDEO_YCBCR
	ccdc_dev->hw_ops.enable_out_to_sdram((!vpfe_dev->imp_chained || (vpfe_dev->imp_chained && (vpfe_dev->current_subdev->ccdc_if_params.if_type == VPFE_YCBCR_SYNC_8))));
#else
	ccdc_dev->hw_ops.enable_out_to_sdram(!vpfe_dev->imp_chained);
#endif
	if (vpfe_dev->imp_chained)
		imp_hw_if->enable(1, NULL);

	ccdc_dev->hw_ops.enable(1);
	vpfe_dev->started = 1;
}

/*
 * vpfe_streamon. Assume the DMA queue is not empty.
 * application is expected to call QBUF before calling
 * this ioctl. If not, driver returns error
 */
static int vpfe_streamon(struct file *file, void *priv,
			 enum v4l2_buf_type buf_type)
{
	struct vpfe_device *vpfe_dev = video_drvdata(file);
	struct vpfe_fh *fh = file->private_data;
	struct vpfe_subdev_info *sdinfo;
	unsigned long addr;
	unsigned long addr_ipipeif = 0;
	int ret = -EINVAL;

#ifdef CONFIG_V2R_DEBUG
	v4l2_dbg(1, debug, &vpfe_dev->v4l2_dev, "vpfe_streamon\n");
#endif

	if (V4L2_BUF_TYPE_VIDEO_CAPTURE != buf_type) {
		v4l2_err(&vpfe_dev->v4l2_dev, "Invalid buf type\n");
		return ret;
	}

	/* If file handle is not allowed IO, return error */
	if (!fh->io_allowed) {
		v4l2_err(&vpfe_dev->v4l2_dev, "fh->io_allowed\n");
		return -EACCES;
	}

	sdinfo = vpfe_dev->current_subdev;
	ret = v4l2_device_call_until_err(&vpfe_dev->v4l2_dev, sdinfo->grp_id,
					video, s_stream, 1);

	if (ret && (ret != -ENOIOCTLCMD)) {
		v4l2_err(&vpfe_dev->v4l2_dev, "stream on failed in subdev\n");
		return ret;
	}

	/* If buffer queue is empty, return error */
	if (list_empty(&vpfe_dev->buffer_queue.stream)) {
		v4l2_err(&vpfe_dev->v4l2_dev, "buffer queue is empty\n");
		return -EIO;
	}

	/* Call videobuf_streamon to start streaming * in videobuf */
	ret = videobuf_streamon(&vpfe_dev->buffer_queue);
	if (ret)
		return ret;

	ret = mutex_lock_interruptible(&vpfe_dev->lock);
	if (ret)
		goto streamoff;
	/* Get the next frame from the buffer queue */
	vpfe_dev->next_frm = list_entry(vpfe_dev->dma_queue.next,
					struct videobuf_buffer, queue);
	vpfe_dev->cur_frm = vpfe_dev->next_frm;
	/* Remove buffer from the buffer queue */
	list_del(&vpfe_dev->cur_frm->queue);
	/* Mark state of the current frame to active */
	vpfe_dev->cur_frm->state = VIDEOBUF_ACTIVE;
	/* Initialize field_id and started member */
	vpfe_dev->field_id = 0;
	addr = videobuf_to_dma_contig(vpfe_dev->cur_frm);
#ifdef CONFIG_VIDEO_YCBCR
	//TODO
	if(sdinfo->ccdc_if_params.if_type == VPFE_YCBCR_SYNC_8)
	{
		ipipif_dma_size = (vpfe_dev->fmt.fmt.pix.width*vpfe_dev->fmt.fmt.pix.height)*2;
		ipipif_dma_addr_cpu = dma_alloc_coherent(vpfe_dev->pdev,ipipif_dma_size,&ipipif_dma_addr_phys,GFP_KERNEL);
		if(!ipipif_dma_addr_cpu)
		{
			goto unlock_out;
		}
		addr_ipipeif = ipipif_dma_addr_phys;
		ccdc_dev->hw_ops.setfbaddr(addr_ipipeif);
	}
#endif
	/* Calculate field offset */
	vpfe_calculate_offsets(vpfe_dev);

	if (vpfe_attach_irq(vpfe_dev) < 0) {
		v4l2_err(&vpfe_dev->v4l2_dev,
			 "Error in attaching interrupt handle\n");
		ret = -EFAULT;
		goto unlock_out;
	}

	if (ccdc_dev->hw_ops.configure(vpfe_dev->imp_chained) < 0) {
		v4l2_err(&vpfe_dev->v4l2_dev,
			 "Error in configuring ccdc\n");
		goto unlock_out;
	}

	if (!vpfe_dev->imp_chained) {
		ccdc_dev->hw_ops.setfbaddr((unsigned long)(addr));
		goto out;
	}

	/* Image processor chained in the path */
	if (!cpu_is_davinci_dm365() &&
	    !vpfe_dev->current_subdev->is_camera) {
		v4l2_err(&vpfe_dev->v4l2_dev, "Doesn't support chaining\n");
		goto unlock_out;
	}
	if (imp_hw_if->hw_setup(vpfe_dev->pdev, NULL) < 0) {
		v4l2_err(&vpfe_dev->v4l2_dev,
			"Error setting up IMP\n");
		goto unlock_out;
	}
#ifdef CONFIG_VIDEO_YCBCR
	//TODO
	if(sdinfo->ccdc_if_params.if_type == VPFE_YCBCR_SYNC_8)
	{
		imp_hw_if->set_ipipif_addr(vpfe_dev->pdev, NULL,addr_ipipeif);
	}
#endif
	if (imp_hw_if->update_outbuf1_address(NULL, addr) < 0) {
		v4l2_err(&vpfe_dev->v4l2_dev,
			"Error setting up address in IMP output1\n");
		goto unlock_out;
	}

	if (vpfe_dev->second_output) {
		if (imp_hw_if->update_outbuf2_address(NULL,
				(addr + vpfe_dev->second_off)) < 0) {
			v4l2_err(&vpfe_dev->v4l2_dev, "Error setting up"
				 " address in IMP output2\n");
			goto unlock_out;
		}
	}
out:
	ret = 0;
	vpfe_start_capture(vpfe_dev);
	mutex_unlock(&vpfe_dev->lock);
	return ret;
unlock_out:
	mutex_unlock(&vpfe_dev->lock);
streamoff:
	ret = videobuf_streamoff(&vpfe_dev->buffer_queue);
	return ret;
}

static int vpfe_streamoff(struct file *file, void *priv,
			  enum v4l2_buf_type buf_type)
{
	struct vpfe_device *vpfe_dev = video_drvdata(file);
	struct vpfe_fh *fh = file->private_data;
	struct vpfe_subdev_info *sdinfo;
	int ret = 0;

#ifdef CONFIG_V2R_DEBUG
	v4l2_dbg(1, debug, &vpfe_dev->v4l2_dev, "vpfe_streamoff\n");
#endif

	if (V4L2_BUF_TYPE_VIDEO_CAPTURE != buf_type) {
#ifdef CONFIG_V2R_DEBUG
		v4l2_dbg(1, debug, &vpfe_dev->v4l2_dev, "Invalid buf type\n");
#endif
		return -EINVAL;
	}

	/* If io is allowed for this file handle, return error */
	if (!fh->io_allowed) {
#ifdef CONFIG_V2R_DEBUG
		v4l2_dbg(1, debug, &vpfe_dev->v4l2_dev, "fh->io_allowed\n");
#endif
		return -EACCES;
	}

	/* If streaming is not started, return error */
	if (!vpfe_dev->started) {
		v4l2_err(&vpfe_dev->v4l2_dev, "device started\n");
		return -EINVAL;
	}

	ret = mutex_lock_interruptible(&vpfe_dev->lock);
	if (ret)
		return ret;

	vpfe_stop_capture(vpfe_dev);
	vpfe_detach_irq(vpfe_dev);

	sdinfo = vpfe_dev->current_subdev;
	ret = v4l2_device_call_until_err(&vpfe_dev->v4l2_dev, sdinfo->grp_id,
					video, s_stream, 0);

	if (ret && (ret != -ENOIOCTLCMD))
		v4l2_err(&vpfe_dev->v4l2_dev, "stream off failed in subdev\n");

#ifdef CONFIG_VIDEO_YCBCR
	if(ipipif_dma_addr_cpu)
	{
		dma_free_coherent(vpfe_dev->pdev,ipipif_dma_size,ipipif_dma_addr_cpu,ipipif_dma_addr_phys);
		ipipif_dma_addr_cpu = NULL;
		ipipif_dma_size = 0;
		ipipif_dma_addr_phys = 0;
	}
#endif

	ret = videobuf_streamoff(&vpfe_dev->buffer_queue);
	mutex_unlock(&vpfe_dev->lock);
	return ret;
}

static int vpfe_queryctrl(struct file *file, void *priv,
				struct v4l2_queryctrl *qc)
{
	struct vpfe_device *vpfe_dev = video_drvdata(file);
	struct vpfe_subdev_info *sub_dev = vpfe_dev->current_subdev;

#ifdef CONFIG_V2R_DEBUG
	v4l2_dbg(1, debug, &vpfe_dev->v4l2_dev, "vpfe_queryctrl\n");
#endif

	/* pass it to sub device */
	return v4l2_device_call_until_err(&vpfe_dev->v4l2_dev, sub_dev->grp_id,
					  core, queryctrl, qc);
}

static int vpfe_g_ctrl(struct file *file, void *priv,
			struct v4l2_control *ctrl)
{
	struct vpfe_device *vpfe_dev = video_drvdata(file);
	struct vpfe_subdev_info *sub_dev = vpfe_dev->current_subdev;

#ifdef CONFIG_V2R_DEBUG
	v4l2_dbg(1, debug, &vpfe_dev->v4l2_dev, "vpfe_g_ctrl\n");
#endif

	return v4l2_device_call_until_err(&vpfe_dev->v4l2_dev, sub_dev->grp_id,
					  core, g_ctrl, ctrl);
}

static int vpfe_s_ctrl(struct file *file, void *priv,
			     struct v4l2_control *ctrl)
{
	struct vpfe_device *vpfe_dev = video_drvdata(file);
	struct vpfe_subdev_info *sub_dev = vpfe_dev->current_subdev;

#ifdef CONFIG_V2R_DEBUG
	v4l2_dbg(1, debug, &vpfe_dev->v4l2_dev, "vpfe_s_ctrl\n");
#endif

	return v4l2_device_call_until_err(&vpfe_dev->v4l2_dev, sub_dev->grp_id,
					  core, s_ctrl, ctrl);
}

static int vpfe_cropcap(struct file *file, void *priv,
			      struct v4l2_cropcap *crop)
{
	struct vpfe_device *vpfe_dev = video_drvdata(file);
	struct vpfe_subdev_info *sdinfo;

#ifdef CONFIG_V2R_DEBUG
	v4l2_dbg(1, debug, &vpfe_dev->v4l2_dev, "vpfe_cropcap\n");
#endif

	memset(crop, 0, sizeof(struct v4l2_cropcap));
	crop->type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	sdinfo = vpfe_dev->current_subdev;

	if (!sdinfo->is_camera) {

		if (vpfe_dev->std_index < 0 ||
		    vpfe_dev->std_index >= ARRAY_SIZE(vpfe_standards))
			return -EINVAL;

		crop->bounds.width = vpfe_standards[vpfe_dev->std_index].width;
		crop->defrect.width = crop->bounds.width;
		crop->bounds.height =
			vpfe_standards[vpfe_dev->std_index].height;
		crop->defrect.height = crop->bounds.height;
		crop->pixelaspect =
			vpfe_standards[vpfe_dev->std_index].pixelaspect;
	} else {
		/* camera interface */
		crop->bounds.width = vpfe_dev->std_info.active_pixels;
		crop->defrect.width = crop->bounds.width;
		crop->bounds.height = vpfe_dev->std_info.active_lines;
		crop->defrect.height = crop->bounds.height;
		crop->pixelaspect.numerator = 1;
		crop->pixelaspect.denominator = 1;
	}
	return 0;
}

static int vpfe_g_crop(struct file *file, void *priv,
			     struct v4l2_crop *crop)
{
	struct vpfe_device *vpfe_dev = video_drvdata(file);

#ifdef CONFIG_V2R_DEBUG
	v4l2_dbg(1, debug, &vpfe_dev->v4l2_dev, "vpfe_g_crop\n");
#endif

	crop->c = vpfe_dev->crop;
	return 0;
}

static int vpfe_s_crop(struct file *file, void *priv,
			     struct v4l2_crop *crop)
{
	struct vpfe_device *vpfe_dev = video_drvdata(file);
	int ret = 0, max_height, max_width;

#ifdef CONFIG_V2R_DEBUG
	v4l2_dbg(1, debug, &vpfe_dev->v4l2_dev, "vpfe_s_crop\n");
#endif

	if (vpfe_dev->started) {
		/* make sure streaming is not started */
		v4l2_err(&vpfe_dev->v4l2_dev,
			"Cannot change crop when streaming is ON\n");
		return -EBUSY;
	}

	ret = mutex_lock_interruptible(&vpfe_dev->lock);
	if (ret)
		return ret;

	if (crop->c.top < 0 || crop->c.left < 0) {
		v4l2_err(&vpfe_dev->v4l2_dev,
			"doesn't support negative values for top & left\n");
		ret = -EINVAL;
		goto unlock_out;
	}

	/* adjust the width to 16 pixel boundry */
	crop->c.width = ((crop->c.width + 15) & ~0xf);

	/**
	 * When there is no image processor chained, then cropping
	 * happens at the ccdc and image size is the cropped image
	 * size. For Camera, maximum size is limited to frame size
	 * configured at the sensor through S_FMT that happens at
	 * either at device open() or when application calls S_FMT
	 */
	if (!vpfe_dev->current_subdev->is_camera) {
		max_width = vpfe_dev->std_info.active_pixels;
		max_height = vpfe_dev->std_info.active_lines;
	} else {
		max_width = vpfe_dev->fmt.fmt.pix.width;
		max_height = vpfe_dev->fmt.fmt.pix.height;
	}

	if ((crop->c.left + crop->c.width > max_width) ||
	    (crop->c.top + crop->c.height > max_height)) {
		v4l2_err(&vpfe_dev->v4l2_dev, "Error in S_CROP"
			 " params, max_width = %d, max_height = %d\n",
			 max_width, max_height);
		ret = -EINVAL;
		goto unlock_out;
	}

	ccdc_dev->hw_ops.set_image_window(&crop->c);
	if (!vpfe_dev->imp_chained) {
		vpfe_dev->fmt.fmt.pix.width = crop->c.width;
		vpfe_dev->fmt.fmt.pix.height = crop->c.height;
		vpfe_dev->fmt.fmt.pix.bytesperline =
			ccdc_dev->hw_ops.get_line_length();
		vpfe_dev->fmt.fmt.pix.sizeimage =
			vpfe_dev->fmt.fmt.pix.bytesperline *
			vpfe_dev->fmt.fmt.pix.height;
	} else {
		struct imp_window imp_crop_win;

		imp_crop_win.width = crop->c.width;
		imp_crop_win.height = crop->c.height;
		imp_crop_win.hst = crop->c.left;
		/* vst starts from 1 */
		imp_crop_win.vst = crop->c.top + 1;
		if (imp_hw_if->set_input_win(&imp_crop_win) < 0) {
			v4l2_err(&vpfe_dev->v4l2_dev, "Error in setting crop "
				 "window in IMP\n");
			ret = -EINVAL;
			goto unlock_out;
		}
	}
	vpfe_dev->crop = crop->c;
unlock_out:
	mutex_unlock(&vpfe_dev->lock);
	return ret;
}

static int vpfe_s_parm(struct file *file, void *priv,
		       struct v4l2_streamparm *parm)
{
	struct v4l2_captureparm *capparam = &parm->parm.capture;
	struct vpfe_device *vpfe_dev = video_drvdata(file);
	int ret = -EINVAL;

	/* TODO - Revisit it before submitting to upstream */
	if (!cpu_is_davinci_dm365()) {
#ifdef CONFIG_V2R_DEBUG
		v4l2_dbg(1, debug, &vpfe_dev->v4l2_dev,
			"Ioctl not supported on this platform\n");
#endif
		goto out;
	}

	if (vpfe_dev->started) {
#ifdef CONFIG_V2R_DEBUG
		v4l2_dbg(1, debug, &vpfe_dev->v4l2_dev,
			"Steaming ON. Cannot change capture streaming params.");
#endif
		goto out;
	}

	if (vpfe_dev->std_info.frame_format) {
#ifdef CONFIG_V2R_DEBUG
		v4l2_dbg(1, debug, &vpfe_dev->v4l2_dev,
			"Supported only for progressive scan");
#endif
		goto out;
	}

	if (!capparam->timeperframe.numerator ||
	    !capparam->timeperframe.denominator) {
#ifdef CONFIG_V2R_DEBUG
		v4l2_dbg(1, debug, &vpfe_dev->v4l2_dev,
			"invalid timeperframe");
#endif
		goto out;
	}

	if (capparam->timeperframe.numerator !=
	    vpfe_dev->std_info.fps.numerator ||
	    capparam->timeperframe.denominator >
	    vpfe_dev->std_info.fps.denominator) {
#ifdef CONFIG_V2R_DEBUG
		v4l2_dbg(1, debug, &vpfe_dev->v4l2_dev,
			"Invalid capparam, timeperframe.numerator = %d,"
			"timeperframe.denominator = %d,\n"
			"vpfe std_info.numerator = %d,"
			" std_info.denominator = %d",
			capparam->timeperframe.numerator,
			capparam->timeperframe.denominator,
			vpfe_dev->std_info.fps.numerator,
			vpfe_dev->std_info.fps.denominator);
#endif
		goto out;
	}

	ret = mutex_lock_interruptible(&vpfe_dev->lock);
	if (ret)
		goto out;

	vpfe_dev->timeperframe = capparam->timeperframe;
	vpfe_dev->skip_frame_count = vpfe_dev->std_info.fps.denominator/
					capparam->timeperframe.denominator;
	vpfe_dev->skip_frame_count_init = vpfe_dev->skip_frame_count;
	mutex_unlock(&vpfe_dev->lock);
	ret = 0;
out:
	return ret;
}

static int vpfe_g_parm(struct file *file, void *priv,
		       struct v4l2_streamparm *parm)
{
	struct v4l2_captureparm *capparam = &parm->parm.capture;
	struct vpfe_device *vpfe_dev = video_drvdata(file);

	memset(capparam, 0, sizeof(struct v4l2_captureparm));
	capparam->capability = V4L2_CAP_TIMEPERFRAME;
	capparam->timeperframe = vpfe_dev->timeperframe;
	return 0;
}

static int vpfe_enum_framesizes(struct file *file, void *priv,
				struct v4l2_frmsizeenum *frms)
{
	struct vpfe_device *vpfe_dev = video_drvdata(file);
	struct vpfe_subdev_info *sdinfo;
	u32 pixel_format;
	int ret = -EINVAL;

#ifdef CONFIG_V2R_DEBUG
	v4l2_dbg(1, debug, &vpfe_dev->v4l2_dev, "vpfe_enum_framesizes\n");
#endif
	sdinfo = vpfe_dev->current_subdev;

	mutex_lock(&vpfe_dev->lock);

	if (sdinfo->is_camera) {
#ifdef CONFIG_VIDEO_YCBCR
		if(frms->pixel_format == V4L2_PIX_FMT_SGRBG10)
		{
			/* Assume the sensor supports V4L2_PIX_FMT_SGRBG10*/
			ret = v4l2_device_call_until_err(&vpfe_dev->v4l2_dev,
					sdinfo->grp_id, video, enum_framesizes, frms);
		}
		//TODO
		else
		{
			ret = 0;
		}
#else
		/* Assume the sensor supports V4L2_PIX_FMT_SGRBG10*/
		pixel_format = frms->pixel_format;
		frms->pixel_format = V4L2_PIX_FMT_SGRBG10;
		ret = v4l2_device_call_until_err(&vpfe_dev->v4l2_dev,
			sdinfo->grp_id, video, enum_framesizes, frms);
		frms->pixel_format = pixel_format;
#endif
	}

	mutex_unlock(&vpfe_dev->lock);
	return ret;
}

static int vpfe_enum_frameintervals(struct file *file, void *priv,
				struct v4l2_frmivalenum *frmi)
{
	struct vpfe_device *vpfe_dev = video_drvdata(file);
	struct vpfe_subdev_info *sdinfo;
	u32 pixel_format;
	int ret = -EINVAL;

#ifdef CONFIG_V2R_DEBUG
	v4l2_dbg(1, debug, &vpfe_dev->v4l2_dev, "vpfe_enum_frameintervals\n");
#endif
	sdinfo = vpfe_dev->current_subdev;

	mutex_lock(&vpfe_dev->lock);

	if (sdinfo->is_camera) {
#ifdef CONFIG_VIDEO_YCBCR
		if(frmi->pixel_format == V4L2_PIX_FMT_SGRBG10)
		{
			/* Assume the sensor supports V4L2_PIX_FMT_SGRBG10*/
			ret = v4l2_device_call_until_err(&vpfe_dev->v4l2_dev,
					sdinfo->grp_id, video, enum_frameintervals, frmi);
		}
		else
		{
			ret = 0;
		}
#else
		/* Assume the sensor supports V4L2_PIX_FMT_SGRBG10*/
		pixel_format = frmi->pixel_format;
		frmi->pixel_format = V4L2_PIX_FMT_SGRBG10;
		ret = v4l2_device_call_until_err(&vpfe_dev->v4l2_dev,
			sdinfo->grp_id, video, enum_frameintervals, frmi);
		frmi->pixel_format = pixel_format;
#endif
	}

	mutex_unlock(&vpfe_dev->lock);
	return ret;
}

/* vpfe capture ioctl operations */
static const struct v4l2_ioctl_ops vpfe_ioctl_ops = {
	.vidioc_querycap	 = vpfe_querycap,
	.vidioc_g_fmt_vid_cap    = vpfe_g_fmt_vid_cap,
	.vidioc_enum_fmt_vid_cap = vpfe_enum_fmt_vid_cap,
	.vidioc_s_fmt_vid_cap    = vpfe_s_fmt_vid_cap,
	.vidioc_try_fmt_vid_cap  = vpfe_try_fmt_vid_cap,
	.vidioc_enum_input	 = vpfe_enum_input,
	.vidioc_g_input		 = vpfe_g_input,
	.vidioc_s_input		 = vpfe_s_input,
	.vidioc_querystd	 = vpfe_querystd,
	.vidioc_s_std		 = vpfe_s_std,
	.vidioc_g_std		 = vpfe_g_std,
	.vidioc_reqbufs		 = vpfe_reqbufs,
	.vidioc_querybuf	 = vpfe_querybuf,
	.vidioc_qbuf		 = vpfe_qbuf,
	.vidioc_dqbuf		 = vpfe_dqbuf,
	.vidioc_streamon	 = vpfe_streamon,
	.vidioc_streamoff	 = vpfe_streamoff,
	.vidioc_queryctrl	 = vpfe_queryctrl,
	.vidioc_g_ctrl		 = vpfe_g_ctrl,
	.vidioc_s_ctrl		 = vpfe_s_ctrl,
	.vidioc_cropcap		 = vpfe_cropcap,
	.vidioc_g_crop		 = vpfe_g_crop,
	.vidioc_s_crop		 = vpfe_s_crop,
	.vidioc_s_parm		 = vpfe_s_parm,
	.vidioc_g_parm		 = vpfe_g_parm,
	.vidioc_enum_framesizes	 = vpfe_enum_framesizes,
	.vidioc_enum_frameintervals = vpfe_enum_frameintervals,
};

static struct vpfe_device *vpfe_initialize(void)
{
	struct vpfe_device *vpfe_dev;

	/* Default number of buffers should be 3 */
	if ((numbuffers > 0) &&
	    (numbuffers < config_params.min_numbuffers))
		numbuffers = config_params.min_numbuffers;

	/*
	 * Set buffer size to min buffers size if invalid buffer size is
	 * given
	 */
	if (bufsize < config_params.min_bufsize)
		bufsize = config_params.min_bufsize;

	config_params.numbuffers = numbuffers;

	if (numbuffers)
		config_params.device_bufsize = ALIGN(bufsize, 4096);

	/* Allocate memory for device objects */
	vpfe_dev = kzalloc(sizeof(*vpfe_dev), GFP_KERNEL);

	return vpfe_dev;
}

static void vpfe_disable_clock(struct vpfe_device *vpfe_dev)
{
#ifndef CONFIG_V2R_VIDEOOUTALWAYSON
	struct vpfe_config *vpfe_cfg = vpfe_dev->cfg;
	int i;

	for (i = 0; i < vpfe_cfg->num_clocks; i++) {
		clk_disable(vpfe_dev->clks[i]);
		clk_put(vpfe_dev->clks[i]);
	}
	kfree(vpfe_dev->clks);
	v4l2_info(vpfe_dev->pdev->driver, "vpfe capture clocks disabled\n");
#endif
}

/**
 * vpfe_enable_clock() - Enable clocks for vpfe capture driver
 * @vpfe_dev - ptr to vpfe capture device
 *
 * Enables clocks defined in vpfe configuration. The function
 * assumes that at least one clock is to be defined which is
 * true as of now. re-visit this if this assumption is not true
 */
static int vpfe_enable_clock(struct vpfe_device *vpfe_dev)
{
	struct vpfe_config *vpfe_cfg = vpfe_dev->cfg;
	int ret = -EFAULT, i;

	if (!vpfe_cfg->num_clocks)
		return 0;

	vpfe_dev->clks = kzalloc(vpfe_cfg->num_clocks *
				   sizeof(struct clock *), GFP_KERNEL);

	if (NULL == vpfe_dev->clks) {
		v4l2_err(vpfe_dev->pdev->driver, "Memory allocation failed\n");
		return -ENOMEM;
	}

	for (i = 0; i < vpfe_cfg->num_clocks; i++) {
		if (NULL == vpfe_cfg->clocks[i]) {
			v4l2_err(vpfe_dev->pdev->driver,
				"clock %s is not defined in vpfe config\n",
				vpfe_cfg->clocks[i]);
			goto out;
		}

		vpfe_dev->clks[i] = clk_get(vpfe_dev->pdev,
					      vpfe_cfg->clocks[i]);
		if (NULL == vpfe_dev->clks[i]) {
			v4l2_err(vpfe_dev->pdev->driver,
				"Failed to get clock %s\n",
				vpfe_cfg->clocks[i]);
			goto out;
		}

		if (clk_enable(vpfe_dev->clks[i])) {
			v4l2_err(vpfe_dev->pdev->driver,
				"vpfe clock %s not enabled\n",
				vpfe_cfg->clocks[i]);
			goto out;
		}

		v4l2_info(vpfe_dev->pdev->driver, "vpss clock %s enabled",
			  vpfe_cfg->clocks[i]);
	}
	return 0;
out:
	for (i = 0; i < vpfe_cfg->num_clocks; i++) {
		if (vpfe_dev->clks[i])
			clk_put(vpfe_dev->clks[i]);
	}
	kfree(vpfe_dev->clks);
	return ret;
}

/**
 * vpfe_probe : vpfe probe function
 * @pdev: platform device pointer
 *
 * This function creates device entries by register itself to the V4L2 driver
 * and initializes fields of each device objects
 */
static int vpfe_probe(struct platform_device *pdev)
{
	struct vpfe_subdev_info *sdinfo;
	struct vpfe_config *vpfe_cfg;
	struct resource *res1;
	struct vpfe_device *vpfe_dev;
	struct i2c_adapter *i2c_adap;
	struct video_device *vfd;
	int ret = -ENOMEM, i, j, err;
	int num_subdevs = 0;
	unsigned long phys_end_kernel;
	size_t size;

	/* Get the pointer to the device object */
	vpfe_dev = vpfe_initialize();

	if (!vpfe_dev) {
		v4l2_err(pdev->dev.driver,
			"Failed to allocate memory for vpfe_dev\n");
		return ret;
	}

	vpfe_dev->pdev = &pdev->dev;
	//printk("Cont buffer size = %d\r\n", cont_bufsize);

	if (cont_bufsize) {
		/* attempt to determine the end of Linux kernel memory */
		//unsigned int tmp_size = 0;
		phys_end_kernel = virt_to_phys((void *)PAGE_OFFSET) +
			(num_physpages << PAGE_SHIFT);
		size = cont_bufsize;
		phys_end_kernel += cont_bufoffset;
		//printk("Cont memory %x -> %x for device %p\r\n", phys_end_kernel, size, &pdev->dev);
		//tmp_size = PAGE_ALIGN(size);
		//printk("Page align %d\r\n", tmp_size);
		err = dma_declare_coherent_memory(&pdev->dev, phys_end_kernel,
				phys_end_kernel, size,
				DMA_MEMORY_MAP | DMA_MEMORY_EXCLUSIVE);
		//printk("Cont memory %d\r\n", err);
		if (!err) {
			dev_err(&pdev->dev, "Unable to declare MMAP memory.\n");
			ret = -ENOENT;
			goto probe_free_dev_mem;
		}
		config_params.video_limit = size;
	}

	if (NULL == pdev->dev.platform_data) {
		v4l2_err(pdev->dev.driver, "Unable to get vpfe config\n");
		ret = -ENOENT;
		goto probe_free_dev_mem;
	}

	vpfe_cfg = pdev->dev.platform_data;
	vpfe_dev->cfg = vpfe_cfg;
	if (NULL == vpfe_cfg->ccdc ||
	    NULL == vpfe_cfg->card_name ||
	    NULL == vpfe_cfg->sub_devs) {
		v4l2_err(pdev->dev.driver, "null ptr in vpfe_cfg\n");
		ret = -ENOENT;
		goto probe_free_dev_mem;
	}

	/* enable vpss clocks */
	ret = vpfe_enable_clock(vpfe_dev);
	if (ret)
		goto probe_free_dev_mem;

	/* Initialise the ipipe hw module if exists */
	if (!cpu_is_davinci_dm644x()) {
		imp_hw_if = imp_get_hw_if();
		if (ISNULL(imp_hw_if))
			return -1;
	}

	mutex_lock(&ccdc_lock);
	/* Allocate memory for ccdc configuration */
	ccdc_cfg = kmalloc(sizeof(struct ccdc_config), GFP_KERNEL);
	if (NULL == ccdc_cfg) {
		v4l2_err(pdev->dev.driver,
			 "Memory allocation failed for ccdc_cfg\n");
		goto probe_disable_clock;
	}

	strncpy(ccdc_cfg->name, vpfe_cfg->ccdc, 32);
	/* Get VINT0 irq resource */
	res1 = platform_get_resource(pdev, IORESOURCE_IRQ, 0);
	if (!res1) {
		v4l2_err(pdev->dev.driver,
			 "Unable to get interrupt for VINT0\n");
		ret = -ENOENT;
		goto probe_disable_clock;
	}
	vpfe_dev->ccdc_irq0 = res1->start;

	/* Get VINT1 irq resource */
	res1 = platform_get_resource(pdev,
				IORESOURCE_IRQ, 1);
	if (!res1) {
		v4l2_err(pdev->dev.driver,
			 "Unable to get interrupt for VINT1\n");
		ret = -ENOENT;
		goto probe_disable_clock;
	}
	vpfe_dev->ccdc_irq1 = res1->start;

	/* Allocate memory for video device */
	vfd = video_device_alloc();
	if (NULL == vfd) {
		ret = -ENOMEM;
		v4l2_err(pdev->dev.driver,
			"Unable to alloc video device\n");
		goto probe_disable_clock;
	}

	/* Initialize field of video device */
	vfd->release		= video_device_release;
	vfd->fops		= &vpfe_fops;
	vfd->ioctl_ops		= &vpfe_ioctl_ops;
	vfd->minor		= -1;
	vfd->tvnorms		= 0;
	vfd->current_norm	= V4L2_STD_NTSC;
	vfd->v4l2_dev 		= &vpfe_dev->v4l2_dev;
	snprintf(vfd->name, sizeof(vfd->name),
		 "%s_V%d.%d.%d",
		 CAPTURE_DRV_NAME,
		 (VPFE_CAPTURE_VERSION_CODE >> 16) & 0xff,
		 (VPFE_CAPTURE_VERSION_CODE >> 8) & 0xff,
		 (VPFE_CAPTURE_VERSION_CODE) & 0xff);
	/* Set video_dev to the video device */
	vpfe_dev->video_dev	= vfd;

	ret = v4l2_device_register(&pdev->dev, &vpfe_dev->v4l2_dev);
	if (ret) {
		v4l2_err(pdev->dev.driver,
			"Unable to register v4l2 device.\n");
		goto probe_out_video_release;
	}
	v4l2_info(&vpfe_dev->v4l2_dev, "v4l2 device registered\n");
	spin_lock_init(&vpfe_dev->irqlock);
	spin_lock_init(&vpfe_dev->dma_queue_lock);
	mutex_init(&vpfe_dev->lock);

	/* Initialize field of the device objects */
	vpfe_dev->numbuffers = config_params.numbuffers;

	/* Initialize prio member of device object */
	v4l2_prio_init(&vpfe_dev->prio);
	/* register video device */
#ifdef CONFIG_V2R_DEBUG
	v4l2_dbg(1, debug, &vpfe_dev->v4l2_dev,
		"trying to register vpfe device.\n");
	v4l2_dbg(1, debug, &vpfe_dev->v4l2_dev,
		"video_dev=%x\n", (int)&vpfe_dev->video_dev);
#endif
	vpfe_dev->fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	ret = video_register_device(vpfe_dev->video_dev,
				    VFL_TYPE_GRABBER, -1);

	if (ret) {
		v4l2_err(pdev->dev.driver,
			"Unable to register video device.\n");
		goto probe_out_v4l2_unregister;
	}

	v4l2_info(&vpfe_dev->v4l2_dev, "video device registered\n");
	/* set the driver data in platform device */
	platform_set_drvdata(pdev, vpfe_dev);
	/* set driver private data */
	video_set_drvdata(vpfe_dev->video_dev, vpfe_dev);
	i2c_adap = i2c_get_adapter(1);
	vpfe_cfg = pdev->dev.platform_data;
	num_subdevs = vpfe_cfg->num_subdevs;
	vpfe_dev->sd = kmalloc(sizeof(struct v4l2_subdev *) * num_subdevs,
				GFP_KERNEL);
	if (NULL == vpfe_dev->sd) {
		v4l2_err(&vpfe_dev->v4l2_dev,
			"unable to allocate memory for subdevice pointers\n");
		ret = -ENOMEM;
		goto probe_out_video_unregister;
	}

	for (i = 0; i < num_subdevs; i++) {
		struct v4l2_input *inps;

		sdinfo = &vpfe_cfg->sub_devs[i];
		/**
		 * register subdevices based on interface setting. Currently
		 * tvp5146 and mt9p031 cannot co-exists due to i2c address
		 * conflicts. So only one of them is registered. Re-visit this
		 * once we have support for i2c switch handling in i2c driver
		 * framework
		 */
		if (interface == sdinfo->is_camera) {
			/* setup input path */
			if (vpfe_cfg->setup_input) {
				if (vpfe_cfg->setup_input(sdinfo->grp_id) < 0) {
					ret = -EFAULT;
					v4l2_info(&vpfe_dev->v4l2_dev, "could"
						  " not setup input for %s\n",
						  sdinfo->module_name);
					goto probe_sd_out;
				}
			}
			/* Load up the subdevice */
			vpfe_dev->sd[i] =
				v4l2_i2c_new_subdev_board(&vpfe_dev->v4l2_dev,
							  i2c_adap,
							  sdinfo->module_name,
							  &sdinfo->board_info,
							  NULL);
			if (vpfe_dev->sd[i]) {
				v4l2_info(&vpfe_dev->v4l2_dev,
					  "v4l2 sub device %s registered\n",
					  sdinfo->module_name);
				vpfe_dev->sd[i]->grp_id = sdinfo->grp_id;
				/* update tvnorms from the sub devices */
				for (j = 0; j < sdinfo->num_inputs; j++) {
					inps = &sdinfo->inputs[j];
					vfd->tvnorms |= inps->std;
				}
				sdinfo->registered = 1;
			} else {
				v4l2_info(&vpfe_dev->v4l2_dev,
					  "v4l2 sub device %s register fails\n",
					  sdinfo->module_name);
			}
		}
	}

	/* We need at least one sub device to do capture */
	for (i = 0; i < num_subdevs; i++) {
		sdinfo = &vpfe_cfg->sub_devs[i];
		if (sdinfo->registered) {
			/* set this as the current sub device */
			vpfe_dev->current_subdev = &vpfe_cfg->sub_devs[i];
			break;
		}
	}

	/* if we don't have any sub device registered, return error */
	if (i == num_subdevs) {
		printk(KERN_NOTICE "No sub devices registered\n");
		goto probe_sd_out;
	}

	/* We have at least one sub device to work with */
	mutex_unlock(&ccdc_lock);
	return 0;

probe_sd_out:
	kfree(vpfe_dev->sd);
probe_out_video_unregister:
	video_unregister_device(vpfe_dev->video_dev);
probe_out_v4l2_unregister:
	v4l2_device_unregister(&vpfe_dev->v4l2_dev);
probe_out_video_release:
	if (vpfe_dev->video_dev->minor == -1)
		video_device_release(vpfe_dev->video_dev);
probe_disable_clock:
	vpfe_disable_clock(vpfe_dev);
	mutex_unlock(&ccdc_lock);
	kfree(ccdc_cfg);
probe_free_dev_mem:
	kfree(vpfe_dev);
	return ret;
}

/*
 * vpfe_remove : It un-register device from V4L2 driver
 */
static int vpfe_remove(struct platform_device *pdev)
{
	struct vpfe_device *vpfe_dev = platform_get_drvdata(pdev);

	v4l2_info(pdev->dev.driver, "vpfe_remove\n");

	kfree(vpfe_dev->sd);
	v4l2_device_unregister(&vpfe_dev->v4l2_dev);
	video_unregister_device(vpfe_dev->video_dev);
	vpfe_disable_clock(vpfe_dev);
	kfree(vpfe_dev);
	kfree(ccdc_cfg);
	return 0;
}

static int
vpfe_suspend(struct device *dev)
{
	/* add suspend code here later */
	return -1;
}

static int
vpfe_resume(struct device *dev)
{
	/* add resume code here later */
	return -1;
}

static struct dev_pm_ops vpfe_dev_pm_ops = {
	.suspend = vpfe_suspend,
	.resume = vpfe_resume,
};

static struct platform_driver vpfe_driver = {
	.driver = {
		.name = CAPTURE_DRV_NAME,
		.owner = THIS_MODULE,
		.pm = &vpfe_dev_pm_ops,
	},
	.probe = vpfe_probe,
	.remove = (vpfe_remove),
};

static __init int vpfe_init(void)
{
	printk(KERN_NOTICE "vpfe_init\n");
	/* Register driver to the kernel */
	return platform_driver_register(&vpfe_driver);
}

/**
 * vpfe_cleanup : This function un-registers device driver
 */
static void vpfe_cleanup(void)
{
	platform_driver_unregister(&vpfe_driver);
}

module_init(vpfe_init);
module_exit(vpfe_cleanup);
