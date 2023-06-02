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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
//#include <linux/config.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/string.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/types.h>
#include <linux/cdev.h>
#include <linux/dma-mapping.h>
#include <linux/interrupt.h>
#include <linux/uaccess.h>
#include <linux/mutex.h>
#include <linux/device.h>
#include <linux/videodev2.h>
#include <media/davinci/dm365_ipipe.h>
#include <media/davinci/imp_hw_if.h>

#include <mach/irqs.h>

#include "dm365_ipipe_hw.h"
#include "dm365_def_para.h"

/* IPIPE module operation state */
struct ipipe_oper_state {
	/* Operation state in continuous mode */
	unsigned int state;
	/* Semaphore to protect the common hardware configuration */
	struct mutex lock;
	/* previewer config state */
	unsigned int prev_config_state;
	/* Shared configuration of the hardware */
	struct ipipe_params *shared_config_param;
	/* shared resource in use */
	unsigned int resource_in_use;
	/* resizer config state */
	unsigned int rsz_config_state;
	/* resizer chained with previewer */
	unsigned int rsz_chained;
	/* CCDC API related variables */
	/* Buffer type, interleaved or field seperated for interlaced
	 *  scan
	 */
	unsigned int buffer_type;
	/* frame format, 0 - interlaced, 1 - progressive */
	unsigned int frame_format;
	/* input pixel format */
	enum imp_pix_formats in_pixel_format;
	/* input pixel format */
	enum imp_pix_formats out_pixel_format;
};

/* Operation mode of image processor (imp) */
static u32 oper_mode = IMP_MODE_SINGLE_SHOT;
module_param(oper_mode, uint, S_IRUGO);
/* enable/disable serializer */
static u32 en_serializer;
module_param(en_serializer, uint, S_IRUGO);

#define CONFIG_IPIPE_PARAM_VALIDATION
/* ipipe module operation state & configuration */
static struct ipipe_oper_state oper_state;

/* LUT Defect pixel correction data */
static struct prev_lutdpc lutdpc;
static int set_lutdpc_params(struct device *dev, void *param, int len);
static int get_lutdpc_params(struct device *dev, void *param, int len);

/* LUT Defect pixel correction data */
static struct prev_otfdpc otfdpc;
static int set_otfdpc_params(struct device *dev, void *param, int len);
static int get_otfdpc_params(struct device *dev, void *param, int len);

/* Noise filter */
static struct prev_nf nf1;
static struct prev_nf nf2;
static int set_nf1_params(struct device *dev, void *param, int len);
static int get_nf1_params(struct device *dev, void *param, int len);
static int set_nf2_params(struct device *dev, void *param, int len);
static int get_nf2_params(struct device *dev, void *param, int len);

/* Green Imbalance Correction */
static struct prev_gic gic;
static int set_gic_params(struct device *dev, void *param, int len);
static int get_gic_params(struct device *dev, void *param, int len);

/* White Balance */
static struct prev_wb wb;
static int set_wb_params(struct device *dev, void *param, int len);
static int get_wb_params(struct device *dev, void *param, int len);

/* CFA */
static struct prev_cfa cfa;
static int set_cfa_params(struct device *dev, void *param, int len);
static int get_cfa_params(struct device *dev, void *param, int len);

/* RGB2RGB conversion */
static struct prev_rgb2rgb rgb2rgb_1;
static struct prev_rgb2rgb rgb2rgb_2;
static int set_rgb2rgb_1_params(struct device *dev, void *param, int len);
static int get_rgb2rgb_1_params(struct device *dev, void *param, int len);
static int set_rgb2rgb_2_params(struct device *dev, void *param, int len);
static int get_rgb2rgb_2_params(struct device *dev, void *param, int len);

/* Gamma correction */
static struct prev_gamma gamma;
static int set_gamma_params(struct device *dev, void *param, int len);
static int get_gamma_params(struct device *dev, void *param, int len);

/* 3D LUT */
static struct prev_3d_lut lut_3d;
static int set_3d_lut_params(struct device *dev, void *param, int len);
static int get_3d_lut_params(struct device *dev, void *param, int len);

/* Lumina Adjustment */
static struct prev_lum_adj lum_adj;
static int set_lum_adj_params(struct device *dev, void *param, int len);
static int get_lum_adj_params(struct device *dev, void *param, int len);

/* RGB2YUV conversion */
static struct prev_rgb2yuv rgb2yuv;
static int set_rgb2yuv_params(struct device *dev, void *param, int len);
static int get_rgb2yuv_params(struct device *dev, void *param, int len);

/* YUV 422 conversion */
static struct prev_yuv422_conv yuv422_conv;
static int set_yuv422_conv_params(struct device *dev, void *param, int len);
static int get_yuv422_conv_params(struct device *dev, void *param, int len);

/* GBCE */
static struct prev_gbce gbce;
static int set_gbce_params(struct device *dev, void *param, int len);
static int get_gbce_params(struct device *dev, void *param, int len);

/* Edge Enhancement */
static struct prev_yee yee;
static int set_yee_params(struct device *dev, void *param, int len);
static int get_yee_params(struct device *dev, void *param, int len);

/* Chromatic Artifact Reduction, CAR */
static struct prev_car car;
static int set_car_params(struct device *dev, void *param, int len);
static int get_car_params(struct device *dev, void *param, int len);

/* Chromatic Artifact Reduction, CAR */
static struct prev_cgs cgs;
static int set_cgs_params(struct device *dev, void *param, int len);
static int get_cgs_params(struct device *dev, void *param, int len);
#ifdef CONFIG_VIDEO_YCBCR
static int ipipe_set_ipipeif_addr(struct device *dev, void *config, unsigned int address);
#endif
/* Tables for various tuning modules */
struct ipipe_lutdpc_entry ipipe_lutdpc_table[MAX_SIZE_DPC];
struct ipipe_3d_lut_entry ipipe_3d_lut_table[MAX_SIZE_3D_LUT];
unsigned short ipipe_gbce_table[MAX_SIZE_GBCE_LUT];
struct ipipe_gamma_entry ipipe_gamma_table_r[MAX_SIZE_GAMMA];
struct ipipe_gamma_entry ipipe_gamma_table_b[MAX_SIZE_GAMMA];
struct ipipe_gamma_entry ipipe_gamma_table_g[MAX_SIZE_GAMMA];
short ipipe_yee_table[MAX_SIZE_YEE_LUT];

static struct prev_module_if prev_modules[PREV_MAX_MODULES] = {
	{
		.version = "5.1",
		.module_id = PREV_LUTDPC,
		.module_name = "LUT Defect Correction",
		.control = 0,
		.path = IMP_RAW2RAW | IMP_RAW2YUV,
		.set = set_lutdpc_params,
		.get = get_lutdpc_params
	},
	{
		.version = "5.1",
		.module_id = PREV_OTFDPC,
		.module_name = "OTF Defect Pixel Correction",
		.control = 0,
		.path = IMP_RAW2RAW | IMP_RAW2YUV,
		.set = set_otfdpc_params,
		.get = get_otfdpc_params
	},
	{
		.version = "5.1",
		.module_id = PREV_NF1,
		.module_name = "2-D Noise filter - 1",
		.control = 0,
		.path = IMP_RAW2RAW | IMP_RAW2YUV,
		.set = set_nf1_params,
		.get = get_nf1_params
	},
	{
		.version = "5.1",
		.module_id = PREV_NF2,
		.module_name = "2-D Noise filter - 2",
		.control = 0,
		.path = IMP_RAW2RAW | IMP_RAW2YUV,
		.set = set_nf2_params,
		.get = get_nf2_params
	},
	{
		.version = "5.1",
		.module_id = PREV_GIC,
		.module_name = "Green Imbalance Correction",
		.control = 0,
		.path = IMP_RAW2RAW | IMP_RAW2YUV,
		.set = set_gic_params,
		.get = get_gic_params
	},
	{
		.version = "5.1",
		.module_id = PREV_WB,
		.module_name = "White balance",
		.control = 1,
		.path = IMP_RAW2RAW | IMP_RAW2YUV,
		.set = set_wb_params,
		.get = get_wb_params
	},
	{
		.version = "5.1",
		.module_id = PREV_CFA,
		.module_name = "CFA Interpolation",
		.control = 0,
		.path = IMP_RAW2YUV,
		.set = set_cfa_params,
		.get = get_cfa_params
	},
	{
		.version = "5.1",
		.module_id = PREV_RGB2RGB_1,
		.module_name = "RGB-RGB Conversion - 1",
		.control = 0,
		.path = IMP_RAW2YUV,
		.set = set_rgb2rgb_1_params,
		.get = get_rgb2rgb_1_params
	},
	{
		.version = "5.1",
		.module_id = PREV_GAMMA,
		.module_name = "Gamma Correction",
		.control = 0,
		.path = IMP_RAW2YUV,
		.set = set_gamma_params,
		.get = get_gamma_params
	},
	{
		.version = "5.1",
		.module_id = PREV_RGB2RGB_2,
		.module_name = "RGB-RGB Conversion - 2",
		.control = 0,
		.path = IMP_RAW2YUV,
		.set = set_rgb2rgb_2_params,
		.get = get_rgb2rgb_2_params
	},
	{
		.version = "5.1",
		.module_id = PREV_3D_LUT,
		.module_name = "3D LUT",
		.control = 0,
		.path = IMP_RAW2YUV,
		.set = set_3d_lut_params,
		.get = get_3d_lut_params
	},
	{
		.version = "5.1",
		.module_id = PREV_RGB2YUV,
		.module_name = "RGB-YCbCr conversion",
		.control = 0,
		.path = IMP_RAW2YUV,
		.set = set_rgb2yuv_params,
		.get = get_rgb2yuv_params
	},
	{
		.version = "5.1",
		.module_id = PREV_GBCE,
		.module_name = "Global Brightness,Contrast Control",
		.control = 0,
		.path = IMP_RAW2YUV,
		.set = set_gbce_params,
		.get = get_gbce_params
	},
	{
		.version = "5.1",
		.module_id = PREV_YUV422_CONV,
		.module_name = "YUV 422 conversion",
		.control = 0,
		.path = IMP_RAW2YUV,
		.set = set_yuv422_conv_params,
		.get = get_yuv422_conv_params
	},
	{
		.version = "5.1",
		.module_id = PREV_LUM_ADJ,
		.module_name = "Luminance Adjustment",
		.control = 1,
		.path = IMP_RAW2YUV,
		.set = set_lum_adj_params,
		.get = get_lum_adj_params
	},
	{
		.version = "5.1",
		.module_id = PREV_YEE,
		.module_name = "Edge Enhancer",
		.control = 1,
		.path = IMP_RAW2YUV | IMP_YUV2YUV,
		.set = set_yee_params,
		.get = get_yee_params
	},
	{
		.version = "5.1",
		.module_id = PREV_CAR,
		.module_name = "Chromatic Artifact Reduction",
		.control = 1,
		.path = IMP_RAW2YUV | IMP_YUV2YUV,
		.set = set_car_params,
		.get = get_car_params
	},
	{
		.version = "5.1",
		.module_id = PREV_CGS,
		.module_name = "Chromatic Gain Suppression",
		.control = 1,
		.path = IMP_RAW2YUV | IMP_YUV2YUV,
		.set = set_cgs_params,
		.get = get_cgs_params
	}
};

/* function prototypes */
static struct prev_module_if *prev_enum_preview_cap(struct device *dev,
						    int index);
static unsigned int prev_get_oper_mode(void);
static unsigned int ipipe_get_oper_state(void);
static void ipipe_set_oper_state(unsigned int state);
static unsigned int ipipe_rsz_chain_state(void);
static void *ipipe_alloc_config_block(struct device *dev, int shared);
static void ipipe_dealloc_config_block(struct device *dev, void *config_block);
static void ipipe_set_user_config_defaults(struct device *dev,
					   enum imp_log_chan_t chan_type,
					   unsigned int oper_mode,
					   void *config);
static int ipipe_set_preview_config(struct device *dev,
				    unsigned int oper_mode,
				    void *user_config, void *config);
static int ipipe_set_resize_config(struct device *dev, unsigned int oper_mode,
				   int resizer_chained,
				   void *user_config, void *config);
static int ipipe_reconfig_resizer(struct device *dev,
				struct rsz_reconfig *reconfig,
				void *config);

static void ipipe_enable(unsigned char en, void *config);
static void ipipe_get_irq(struct irq_numbers *irq);
static unsigned int ipipe_get_rsz_config_state(void);
static int ipipe_do_hw_setup(struct device *dev, void *config);
static unsigned int ipipe_get_prev_config_state(void);
static void ipipe_lock_chain(void);
static void ipipe_unlock_chain(void);
static void ipipe_dealloc_user_config_block(struct device *dev,
					    void *config_block);
static void *ipipe_alloc_user_config_block(struct device *dev,
					   enum imp_log_chan_t chan_type,
					   unsigned int oper_mode, int *len);
static int ipipe_update_outbuf1_address(void *config, unsigned int address);
static int ipipe_update_outbuf2_address(void *config, unsigned int address);
static int ipipe_set_ipipe_if_address(void *config, unsigned int address);

/* IPIPE hardware limits */
#define IPIPE_MAX_OUTPUT_WIDTH_A	2176
#define IPIPE_MAX_OUTPUT_WIDTH_B	640

/* Based on max resolution supported. QXGA */
//#define IPIPE_MAX_OUTPUT_HEIGHT_A	1536
#define IPIPE_MAX_OUTPUT_HEIGHT_A	2197
/* Based on max resolution supported. VGA */
#define IPIPE_MAX_OUTPUT_HEIGHT_B	480

/* Raw YUV formats */
static u32 ipipe_raw_yuv_pix_formats[] =
                {V4L2_PIX_FMT_UYVY, V4L2_PIX_FMT_NV12};


static int ipipe_enum_pix(u32 *pix, int i)
{
	if (i >= ARRAY_SIZE(ipipe_raw_yuv_pix_formats))
		return -EINVAL;

	*pix = ipipe_raw_yuv_pix_formats[i];
	return 0;
}

static int ipipe_get_max_output_width(int rsz)
{
	if (rsz == RSZ_A)
		return IPIPE_MAX_OUTPUT_WIDTH_A;
	return IPIPE_MAX_OUTPUT_WIDTH_B;
}

static int ipipe_get_max_output_height(int rsz)
{
	if (rsz == RSZ_A)
		return IPIPE_MAX_OUTPUT_HEIGHT_A;
	return IPIPE_MAX_OUTPUT_HEIGHT_B;
}

static int ipipe_serialize(void)
{
	return en_serializer;
}

static void ipipe_dump_hw_config(void);

/* APIs for CCDC driver */
static int ipipe_set_input_win(struct imp_window *);
static int ipipe_get_input_win(struct imp_window *);
static int ipipe_set_in_pixel_format(enum imp_pix_formats);
static int ipipe_set_out_pixel_format(enum imp_pix_formats);
static int ipipe_set_buftype(unsigned char);
static int ipipe_set_frame_format(unsigned char);
static int ipipe_set_output_win(struct imp_window *win);
static int ipipe_get_output_state(unsigned char out_sel);
static int ipipe_get_line_length(unsigned char out_sel);
static int ipipe_get_image_height(unsigned char out_sel);
static int ipipe_set_hw_if_param(struct vpfe_hw_if_param *if_param);

struct imp_hw_interface dm365_ipipe_interface = {
	.name = "DM365 IPIPE",
	.owner = THIS_MODULE,
	.prev_enum_modules = prev_enum_preview_cap,
	.get_preview_oper_mode = prev_get_oper_mode,
	.get_resize_oper_mode = prev_get_oper_mode,
	.get_hw_state = ipipe_get_oper_state,
	.set_hw_state = ipipe_set_oper_state,
	.resizer_chain = ipipe_rsz_chain_state,
	.lock_chain = ipipe_lock_chain,
	.unlock_chain = ipipe_unlock_chain,
	.serialize = ipipe_serialize,
	.alloc_config_block = ipipe_alloc_config_block,
	.dealloc_config_block = ipipe_dealloc_config_block,
	.alloc_user_config_block = ipipe_alloc_user_config_block,
	.dealloc_user_config_block = ipipe_dealloc_user_config_block,
	.set_user_config_defaults = ipipe_set_user_config_defaults,
	.set_preview_config = ipipe_set_preview_config,
	.set_resizer_config = ipipe_set_resize_config,
	.reconfig_resizer = ipipe_reconfig_resizer,
	.update_inbuf_address = ipipe_set_ipipe_if_address,
	.update_outbuf1_address = ipipe_update_outbuf1_address,
	.update_outbuf2_address = ipipe_update_outbuf2_address,
	.enable = ipipe_enable,
	.enable_resize = rsz_src_enable,
	.hw_setup = ipipe_do_hw_setup,
	.get_preview_irq = ipipe_get_irq,
	.get_rsz_irq = ipipe_get_irq,
	.get_resizer_config_state = ipipe_get_rsz_config_state,
	.get_previewer_config_state = ipipe_get_prev_config_state,
	/* Below used by CCDC driver to set input and output params */
	.set_input_win = ipipe_set_input_win,
	.get_input_win = ipipe_get_input_win,
	.set_hw_if_param = ipipe_set_hw_if_param,
	.set_in_pixel_format = ipipe_set_in_pixel_format,
	.set_out_pixel_format = ipipe_set_out_pixel_format,
	.set_buftype = ipipe_set_buftype,
	.set_frame_format = ipipe_set_frame_format,
	.set_output_win = ipipe_set_output_win,
	.get_output_state = ipipe_get_output_state,
	.get_line_length = ipipe_get_line_length,
	.get_image_height = ipipe_get_image_height,
	.get_image_height = ipipe_get_image_height,
	.get_max_output_width = ipipe_get_max_output_width,
	.get_max_output_height = ipipe_get_max_output_height,
	.enum_pix = ipipe_enum_pix,
#ifdef CONFIG_VIDEO_YCBCR
	.set_ipipif_addr = ipipe_set_ipipeif_addr,
#endif
	/* debug function */
	.dump_hw_config = ipipe_dump_hw_config,
};

static int ipipe_set_ipipe_if_address(void *config, unsigned int address)
{
	struct ipipeif *if_params;
	if (ISNULL(config))
		return -1;
	if_params = &((struct ipipe_params *)config)->ipipeif_param;
	return ipipeif_set_address(if_params, address);
}

static void ipipe_lock_chain(void)
{
	mutex_lock(&oper_state.lock);
	oper_state.resource_in_use = 1;
	mutex_unlock(&oper_state.lock);
}

static void ipipe_unlock_chain(void)
{
	mutex_lock(&oper_state.lock);
	oper_state.resource_in_use = 0;
	oper_state.prev_config_state = STATE_NOT_CONFIGURED;
	oper_state.rsz_config_state = STATE_NOT_CONFIGURED;
	oper_state.rsz_chained = 0;
	mutex_unlock(&oper_state.lock);
}
static int ipipe_process_pix_fmts(enum ipipe_pix_formats in_pix_fmt,
				  enum ipipe_pix_formats out_pix_fmt,
				  struct ipipe_params *param)
{
	enum ipipe_pix_formats temp_pix_fmt;

	switch (in_pix_fmt) {
	case IPIPE_BAYER_8BIT_PACK:
		{
			temp_pix_fmt = IPIPE_BAYER;
			param->ipipeif_param.var.if_5_1.pack_mode
			    = IPIPEIF_5_1_PACK_8_BIT;
			break;
		}
	case IPIPE_BAYER_8BIT_PACK_ALAW:
		{
			param->ipipeif_param.var.if_5_1.pack_mode
			    = IPIPEIF_5_1_PACK_8_BIT_A_LAW;
			temp_pix_fmt = IPIPE_BAYER;
			break;
		}
	case IPIPE_BAYER_8BIT_PACK_DPCM:
		{
			param->ipipeif_param.var.if_5_1.pack_mode
			    = IPIPEIF_5_1_PACK_8_BIT;
			param->ipipeif_param.var.if_5_1.dpcm.en = 1;
			temp_pix_fmt = IPIPE_BAYER;
			break;
		}
	case IPIPE_BAYER:
		{
			param->ipipeif_param.var.if_5_1.pack_mode
			    = IPIPEIF_5_1_PACK_16_BIT;
			temp_pix_fmt = IPIPE_BAYER;
			break;
		}
	case IPIPE_BAYER_12BIT_PACK:
		{
			param->ipipeif_param.var.if_5_1.pack_mode
			    = IPIPEIF_5_1_PACK_12_BIT;
			temp_pix_fmt = IPIPE_BAYER;
			break;
		}
	default:
		{
			temp_pix_fmt = IPIPE_UYVY;
		}
	}

	if (temp_pix_fmt == IPIPE_BAYER)
		if (out_pix_fmt == IPIPE_BAYER)
			param->ipipe_dpaths_fmt = IPIPE_RAW2RAW;
		else if ((out_pix_fmt == IPIPE_UYVY) ||
			 (out_pix_fmt == IPIPE_YUV420SP))
			param->ipipe_dpaths_fmt = IPIPE_RAW2YUV;
		else {
			printk(KERN_ERR "process pix format, unknown output"
				 "format %d", out_pix_fmt);
			return -1;
		}
	else if (temp_pix_fmt == IPIPE_UYVY) {
		if (out_pix_fmt == IPIPE_UYVY)
			param->ipipe_dpaths_fmt = IPIPE_YUV2YUV;
		else if (out_pix_fmt == IPIPE_YUV420SP)
			param->ipipe_dpaths_fmt = IPIPE_YUV2YUV;
		else {
			printk(KERN_ERR "process pix format, unknown output"
				"format %d", out_pix_fmt);
			return -1;
		}
	}
	return 0;
}

/*
 * calculate_resize_ratios()
 *   calculates resize ratio for resizer A or B. This is called after setting
 * the input size or output size
 */ 
static void calculate_resize_ratios(struct ipipe_params *param, int index)
{
	param->rsz_rsc_param[index].h_dif =
	    ((param->ipipe_hsz + 1) * 256) /
	    (param->rsz_rsc_param[index].o_hsz + 1);
	param->rsz_rsc_param[index].v_dif =
	    ((param->ipipe_vsz + 1) * 256) /
	    (param->rsz_rsc_param[index].o_vsz + 1);
}

static int ipipe_do_hw_setup(struct device *dev, void *config)
{
	struct ipipe_params *param = (struct ipipe_params *)config;
	int ret = 0;

	dev_dbg(dev, "ipipe_do_hw_setup\n");
	ret = mutex_lock_interruptible(&oper_state.lock);
	if (ret)
		return ret;
	if ((ISNULL(config)) && (oper_mode == IMP_MODE_CONTINUOUS)) {
		/* continuous mode */
		param = oper_state.shared_config_param;
		if (param->rsz_en[RSZ_A])
			calculate_resize_ratios(param, RSZ_A);
		if (param->rsz_en[RSZ_B])
			calculate_resize_ratios(param, RSZ_B);
		ret = ipipe_hw_setup(param);
	}
	mutex_unlock(&oper_state.lock);
	return ret;
}
#ifdef CONFIG_VIDEO_YCBCR
static int ipipe_set_ipipeif_addr(struct device *dev, void *config, unsigned int address)
{
	struct ipipe_params *param = (struct ipipe_params *)config;
	int ret = 0;

	dev_dbg(dev, "ipipe_do_hw_setup\n");
	ret = mutex_lock_interruptible(&oper_state.lock);
	if (ret)
		return ret;
	if ((ISNULL(config)) && (oper_mode == IMP_MODE_CONTINUOUS)) {
		/* continuous mode */
		param = oper_state.shared_config_param;
		ret = ipipe_hw_set_ipipeif_addr(param,address);
	}
	mutex_unlock(&oper_state.lock);
	return ret;
}
#endif
static void ipipe_get_irq(struct irq_numbers *irq)
{
	irq->sdram = IRQ_PRVUINT;
	irq->update = 4;
}

static unsigned int ipipe_rsz_chain_state(void)
{
	return oper_state.rsz_chained;
}

static int ipipe_update_outbuf1_address(void *config, unsigned int address)
{
	if ((ISNULL(config)) && (oper_mode == IMP_MODE_CONTINUOUS))
		return rsz_set_output_address(oper_state.shared_config_param,
					       0,
					       address);
	return rsz_set_output_address((struct ipipe_params *)config,
				       0,
				       address);
}

static int ipipe_update_outbuf2_address(void *config, unsigned int address)
{
	if ((ISNULL(config)) && (oper_mode == IMP_MODE_CONTINUOUS))
		return rsz_set_output_address(oper_state.shared_config_param,
					       1,
					       address);
	return rsz_set_output_address((struct ipipe_params *)config,
				       1,
				       address);
}

static void ipipe_enable(unsigned char en, void *config)
{
	unsigned char val = 0;
	struct ipipe_params *param = (struct ipipe_params *)config;

	if (en)
		val = 1;
	if (oper_mode == IMP_MODE_CONTINUOUS)
		param = oper_state.shared_config_param;
	if (param->rsz_common.source == IPIPE_DATA)
		regw_ip(val, IPIPE_SRC_EN);
	else
		regw_ip(0, IPIPE_SRC_EN);
	if (param->rsz_en[RSZ_A])
		rsz_enable(RSZ_A, en);
	if (param->rsz_en[RSZ_B])
		rsz_enable(RSZ_B, en);
	if (oper_mode == IMP_MODE_SINGLE_SHOT)
		ipipeif_set_enable(val, oper_mode);
}

static int validate_lutdpc_params(struct device *dev)
{
#ifdef CONFIG_IPIPE_PARAM_VALIDATION
	int i;
	if (lutdpc.en > 1 ||
	    lutdpc.repl_white > 1 ||
	    lutdpc.dpc_size > LUT_DPC_MAX_SIZE)
		return -1;
	if (lutdpc.en && (ISNULL(lutdpc.table)))
		return -1;
	for (i = 0; i < lutdpc.dpc_size; i++) {
		if (lutdpc.table[i].horz_pos > LUT_DPC_H_POS_MASK ||
		    lutdpc.table[i].vert_pos > LUT_DPC_V_POS_MASK)
			return -1;
	}
#endif
	return 0;
}

static int set_lutdpc_params(struct device *dev, void *param, int len)
{
	struct prev_lutdpc dpc_param;
	struct ipipe_lutdpc_entry *temp;

	if (ISNULL(param)) {
		/* Copy defaults for dfc */
		temp = lutdpc.table;
		memcpy((void *)&lutdpc,
		       (void *)&dm365_lutdpc_defaults,
		       sizeof(struct prev_lutdpc));
		lutdpc.table = temp;
	} else {
		if (len != sizeof(struct prev_lutdpc)) {
			dev_err(dev,
				"set_lutdpc_params: param struct length"
				" mismatch\n");
			return -EINVAL;
		}
		if (copy_from_user(&dpc_param,
				   (struct prev_lutdpc *)param,
				   sizeof(struct prev_lutdpc))) {
			dev_err(dev,
				"set_lutdpc_params: Error in copy to kernel\n");
			return -EFAULT;
		}

		if (ISNULL(dpc_param.table)) {
			dev_err(dev, "Invalid user dpc table ptr\n");
			return -EINVAL;
		}
		lutdpc.en = dpc_param.en;
		lutdpc.repl_white = dpc_param.repl_white;
		lutdpc.dpc_size = dpc_param.dpc_size;
		if (copy_from_user
			(lutdpc.table,
			(struct ipipe_dpc_entry *)dpc_param.table,
			(lutdpc.dpc_size *
			sizeof(struct ipipe_lutdpc_entry)))) {
			dev_err(dev,
				"set_lutdpc_params: Error in copying "
				"dfc table to kernel\n");
			return -EFAULT;
		}

		if (validate_lutdpc_params(dev) < 0)
			return -EINVAL;
	}
	return ipipe_set_lutdpc_regs(&lutdpc);
}

static int get_lutdpc_params(struct device *dev, void *param, int len)
{
	struct prev_lutdpc user_lutdpc;
	struct prev_lutdpc *lut_param = (struct prev_lutdpc *)param;
	if (ISNULL(lut_param)) {
		dev_err(dev, "get_lutdpc_params: invalid user ptr");
		return -EINVAL;
	}
	if (len != sizeof(struct prev_lutdpc)) {
		dev_err(dev,
			"get_lutdpc_params: param struct length mismatch\n");
		return -EINVAL;
	}
	if (copy_from_user(&user_lutdpc,
			   lut_param,
			   sizeof(struct prev_lutdpc))) {
		dev_err(dev, "get_lutdpc_params: Error in copy to" " kernel\n");
		return -EFAULT;
	}

	user_lutdpc.en = lutdpc.en;
	user_lutdpc.repl_white = lutdpc.repl_white;
	user_lutdpc.dpc_size = lutdpc.dpc_size;
	if (ISNULL(user_lutdpc.table)) {
		dev_err(dev, "get_lutdpc_params:" " Invalid table ptr");
		return -EINVAL;
	}
	if (copy_to_user(user_lutdpc.table,
			 lutdpc.table,
			 (lutdpc.dpc_size *
			  sizeof(struct ipipe_lutdpc_entry)))) {
		dev_err(dev,
			"get_lutdpc_params:Table Error in" " copy to user\n");
		return -EFAULT;
	}

	if (copy_to_user(lut_param,
			 &user_lutdpc,
			 sizeof(struct prev_lutdpc))) {
		dev_err(dev, "get_lutdpc_params: Error in copy" " to user\n");
		return -EFAULT;
	}
	return 0;
}

static int validate_otfdpc_params(struct device *dev)
{
#ifdef CONFIG_IPIPE_PARAM_VALIDATION
	struct prev_otfdpc *dpc_param = (struct prev_otfdpc *)&otfdpc;
	struct prev_otfdpc_2_0 *dpc_2_0;
	struct prev_otfdpc_3_0 *dpc_3_0;

	if (dpc_param->en > 1)
		return -1;
	if (dpc_param->alg == IPIPE_OTFDPC_2_0) {
		dpc_2_0 = &dpc_param->alg_cfg.dpc_2_0;
		if (dpc_2_0->det_thr.r > OTFDPC_DPC2_THR_MASK ||
		    dpc_2_0->det_thr.gr > OTFDPC_DPC2_THR_MASK ||
		    dpc_2_0->det_thr.gb > OTFDPC_DPC2_THR_MASK ||
		    dpc_2_0->det_thr.b > OTFDPC_DPC2_THR_MASK ||
		    dpc_2_0->corr_thr.r > OTFDPC_DPC2_THR_MASK ||
		    dpc_2_0->corr_thr.gr > OTFDPC_DPC2_THR_MASK ||
		    dpc_2_0->corr_thr.gb > OTFDPC_DPC2_THR_MASK ||
		    dpc_2_0->corr_thr.b > OTFDPC_DPC2_THR_MASK)
			return -1;
	} else {
		dpc_3_0 = &dpc_param->alg_cfg.dpc_3_0;
		if (dpc_3_0->act_adj_shf > OTF_DPC3_0_SHF_MASK ||
		    dpc_3_0->det_thr > OTF_DPC3_0_DET_MASK ||
		    dpc_3_0->det_slp > OTF_DPC3_0_SLP_MASK ||
		    dpc_3_0->det_thr_min > OTF_DPC3_0_DET_MASK ||
		    dpc_3_0->det_thr_max > OTF_DPC3_0_DET_MASK ||
		    dpc_3_0->corr_thr > OTF_DPC3_0_CORR_MASK ||
		    dpc_3_0->corr_slp > OTF_DPC3_0_SLP_MASK ||
		    dpc_3_0->corr_thr_min > OTF_DPC3_0_CORR_MASK ||
		    dpc_3_0->corr_thr_max > OTF_DPC3_0_CORR_MASK)
			return -1;
	}
#endif
	return 0;
}

static int set_otfdpc_params(struct device *dev, void *param, int len)
{
	struct prev_otfdpc *dpc_param = (struct prev_otfdpc *)param;

	if (ISNULL(param)) {
		/* Copy defaults for dpc2.0 defaults */
		memcpy((void *)&otfdpc,
		       (void *)&dm365_otfdpc_defaults,
		       sizeof(struct ipipe_otfdpc_2_0));
	} else {
		if (len != sizeof(struct prev_otfdpc)) {
			dev_err(dev,
				"set_otfdpc_params: param struct length"
				" mismatch\n");
			return -EINVAL;
		}
		if (copy_from_user(&otfdpc,
				   dpc_param,
				   sizeof(struct prev_otfdpc))) {
			dev_err(dev,
				"set_otfdpc_params: Error in copy to kernel\n");
			return -EFAULT;
		}

		if (validate_otfdpc_params(dev) < 0)
			return -EINVAL;
	}
	return ipipe_set_otfdpc_regs(&otfdpc);
}

static int get_otfdpc_params(struct device *dev, void *param, int len)
{
	struct prev_otfdpc *dpc_param = (struct prev_otfdpc *)param;

	if (ISNULL(dpc_param)) {
		dev_err(dev, "get_otfdpc_params: invalid user ptr");
		return -EINVAL;
	}
	if (len != sizeof(struct prev_otfdpc)) {
		dev_err(dev,
			"get_otfdpc_params: param struct length mismatch\n");
		return -EINVAL;
	}
	if (copy_to_user(dpc_param,
			 &otfdpc,
			 sizeof(struct prev_otfdpc))) {
		dev_err(dev,
			"get_otfdpc_params: Error in copy dpc table to user\n");
		return -EFAULT;
	}
	return 0;
}

static int validate_nf_params(struct device *dev, unsigned int id)
{
#ifdef CONFIG_IPIPE_PARAM_VALIDATION
	int i;
	struct prev_nf *nf_param = &nf1;

	if (id)
		nf_param = &nf2;
	if (nf_param->en > 1 ||
	    nf_param->shft_val > D2F_SHFT_VAL_MASK ||
	    nf_param->spread_val > D2F_SPR_VAL_MASK ||
	    nf_param->apply_lsc_gain > 1 ||
	    nf_param->edge_det_min_thr > D2F_EDGE_DET_THR_MASK ||
	    nf_param->edge_det_max_thr > D2F_EDGE_DET_THR_MASK)
		return -1;

	for (i = 0; i < IPIPE_NF_THR_TABLE_SIZE; i++)
		if (nf_param->thr[i] > D2F_THR_VAL_MASK)
			return -1;
	for (i = 0; i < IPIPE_NF_STR_TABLE_SIZE; i++)
		if (nf_param->str[i] > D2F_STR_VAL_MASK)
			return -1;
#endif
	return 0;
}

static int set_nf_params(struct device *dev, unsigned int id,
			 void *param, int len)
{
	struct prev_nf *nf_param = (struct prev_nf *)param;
	struct prev_nf *nf = &nf1;

	if (id)
		nf = &nf2;

	if (ISNULL(nf_param)) {
		/* Copy defaults for nf */
		memcpy((void *)nf,
		       (void *)&dm365_nf_defaults,
		       sizeof(struct prev_nf));
		memset((void *)nf->thr, 0, IPIPE_NF_THR_TABLE_SIZE);
		memset((void *)nf->str, 0, IPIPE_NF_THR_TABLE_SIZE);
	} else {
		if (len != sizeof(struct prev_nf)) {
			dev_err(dev,
				"set_nf_params: param struct length"
				" mismatch\n");
			return -EINVAL;
		}
		if (copy_from_user(nf, nf_param, sizeof(struct prev_nf))) {
			dev_err(dev,
				"set_nf_params: Error in copy to kernel\n");
			return -EFAULT;
		}
		if (validate_nf_params(dev, id) < 0)
			return -EINVAL;
	}
	/* Now set the values in the hw */
	return ipipe_set_d2f_regs(id, nf);
}

static int set_nf1_params(struct device *dev, void *param, int len)
{
	return set_nf_params(dev, 0, param, len);
}

static int set_nf2_params(struct device *dev, void *param, int len)
{
	return set_nf_params(dev, 1, param, len);
}

static int get_nf_params(struct device *dev, unsigned int id, void *param,
			 int len)
{
	struct prev_nf *nf_param = (struct prev_nf *)param;
	struct prev_nf *nf = &nf1;

	if (ISNULL(nf_param)) {
		dev_err(dev, "get_nf_params: invalid user ptr");
		return -EINVAL;
	}
	if (len != sizeof(struct prev_nf)) {
		dev_err(dev,
			"get_nf_params: param struct length mismatch\n");
		return -EINVAL;
	}
	if (id)
		nf = &nf2;
	if (copy_to_user((struct prev_nf *)nf_param, nf,
			 sizeof(struct prev_nf))) {
		dev_err(dev, "get_nf_params: Error in copy from kernel\n");
		return -EFAULT;
	}
	return 0;
}

static int get_nf1_params(struct device *dev, void *param, int len)
{
	return get_nf_params(dev, 0, param, len);
}

static int get_nf2_params(struct device *dev, void *param, int len)
{
	return get_nf_params(dev, 1, param, len);
}

static int validate_gic_params(struct device *dev)
{
#ifdef CONFIG_IPIPE_PARAM_VALIDATION
	if (gic.en > 1 ||
	    gic.gain > GIC_GAIN_MASK ||
	    gic.thr > GIC_THR_MASK ||
	    gic.slope > GIC_SLOPE_MASK ||
	    gic.apply_lsc_gain > 1 ||
	    gic.nf2_thr_gain.integer > GIC_NFGAN_INT_MASK ||
	    gic.nf2_thr_gain.decimal > GIC_NFGAN_DECI_MASK)
		return -1;
#endif
	return 0;
}

static int set_gic_params(struct device *dev, void *param, int len)
{
	struct prev_gic *gic_param = (struct prev_gic *)param;

	if (ISNULL(gic_param)) {
		/* Copy defaults for nf */
		memcpy((void *)&gic,
		       (void *)&dm365_gic_defaults,
		       sizeof(struct prev_gic));
	} else {
		if (len != sizeof(struct prev_gic)) {
			dev_err(dev,
				"set_gic_params: param struct length"
				" mismatch\n");
			return -EINVAL;
		}
		if (copy_from_user(&gic, gic_param, sizeof(struct prev_gic))) {
			dev_err(dev,
				"set_gic_params: Error in copy to kernel\n");
			return -EFAULT;
		}
		if (validate_gic_params(dev) < 0)
			return -EINVAL;
	}
	/* Now set the values in the hw */
	return ipipe_set_gic_regs(&gic);
}

static int get_gic_params(struct device *dev, void *param, int len)
{
	struct prev_gic *gic_param = (struct prev_gic *)param;

	if (ISNULL(gic_param)) {
		dev_err(dev, "get_gic_params: invalid user ptr");
		return -EINVAL;
	}

	if (len != sizeof(struct prev_gic)) {
		dev_err(dev,
			"get_gic_params: param struct length mismatch\n");
		return -EINVAL;
	}

	if (copy_to_user((struct prev_gic *)gic_param,
			 &gic,
			 sizeof(struct prev_gic))) {
		dev_err(dev, "get_gic_params: Error in copy from kernel\n");
		return -EFAULT;
	}
	return 0;
}

static int validate_wb_params(struct device *dev)
{
#ifdef CONFIG_IPIPE_PARAM_VALIDATION
	if (wb.ofst_r > WB_OFFSET_MASK ||
	    wb.ofst_gr > WB_OFFSET_MASK ||
	    wb.ofst_gb > WB_OFFSET_MASK ||
	    wb.ofst_b > WB_OFFSET_MASK ||
	    wb.gain_r.integer > WB_GAIN_INT_MASK ||
	    wb.gain_r.decimal > WB_GAIN_DECI_MASK ||
	    wb.gain_gr.integer > WB_GAIN_INT_MASK ||
	    wb.gain_gr.decimal > WB_GAIN_DECI_MASK ||
	    wb.gain_gb.integer > WB_GAIN_INT_MASK ||
	    wb.gain_gb.decimal > WB_GAIN_DECI_MASK ||
	    wb.gain_b.integer > WB_GAIN_INT_MASK ||
	    wb.gain_b.decimal > WB_GAIN_DECI_MASK)
		return -1;
#endif
	return 0;
}
static int set_wb_params(struct device *dev, void *param, int len)
{
	struct prev_wb *wb_param = (struct prev_wb *)param;

	dev_dbg(dev, "set_wb_params");
	if (ISNULL(wb_param)) {
		/* Copy defaults for wb */
		memcpy((void *)&wb,
		       (void *)&dm365_wb_defaults,
		       sizeof(struct prev_wb));
	} else {
		if (len != sizeof(struct prev_wb)) {
			dev_err(dev,
				"set_wb_params: param struct length"
				" mismatch\n");
			return -EINVAL;
		}
		if (copy_from_user(&wb, wb_param, sizeof(struct prev_wb))) {
			dev_err(dev,
				"set_wb_params: Error in copy to kernel\n");
			return -EFAULT;
		}
		if (validate_wb_params(dev) < 0)
			return -EINVAL;
	}

	/* Now set the values in the hw */
	return ipipe_set_wb_regs(&wb);
}
static int get_wb_params(struct device *dev, void *param, int len)
{
	struct prev_wb *wb_param = (struct prev_wb *)param;

	if (ISNULL(wb_param)) {
		dev_err(dev, "get_wb_params: invalid user ptr");
		return -EINVAL;
	}
	if (len != sizeof(struct prev_wb)) {
		dev_err(dev,
			"get_wb_params: param struct length mismatch\n");
		return -EINVAL;
	}
	if (copy_to_user((struct prev_wb *)wb_param,
			 &wb,
			 sizeof(struct prev_wb))) {
		dev_err(dev, "get_wb_params: Error in copy from kernel\n");
		return -EFAULT;
	}
	return 0;
}

static int validate_cfa_params(struct device *dev)
{
#ifdef CONFIG_IPIPE_PARAM_VALIDATION
	if (cfa.hpf_thr_2dir > CFA_HPF_THR_2DIR_MASK ||
	    cfa.hpf_slp_2dir > CFA_HPF_SLOPE_2DIR_MASK ||
	    cfa.hp_mix_thr_2dir > CFA_HPF_MIX_THR_2DIR_MASK ||
	    cfa.hp_mix_slope_2dir > CFA_HPF_MIX_SLP_2DIR_MASK ||
	    cfa.dir_thr_2dir > CFA_DIR_THR_2DIR_MASK ||
	    cfa.dir_slope_2dir > CFA_DIR_SLP_2DIR_MASK ||
	    cfa.nd_wt_2dir > CFA_ND_WT_2DIR_MASK ||
	    cfa.hue_fract_daa > CFA_DAA_HUE_FRA_MASK ||
	    cfa.edge_thr_daa > CFA_DAA_EDG_THR_MASK ||
	    cfa.thr_min_daa > CFA_DAA_THR_MIN_MASK ||
	    cfa.thr_slope_daa > CFA_DAA_THR_SLP_MASK ||
	    cfa.slope_min_daa > CFA_DAA_SLP_MIN_MASK ||
	    cfa.slope_slope_daa > CFA_DAA_SLP_SLP_MASK ||
	    cfa.lp_wt_daa > CFA_DAA_LP_WT_MASK)
		return -1;
#endif
	return 0;
}
static int set_cfa_params(struct device *dev, void *param, int len)
{
	struct prev_cfa *cfa_param = (struct prev_cfa *)param;

	dev_dbg(dev, "set_cfa_params");
	if (ISNULL(cfa_param)) {
		/* Copy defaults for wb */
		memcpy((void *)&cfa,
		       (void *)&dm365_cfa_defaults,
		       sizeof(struct prev_cfa));
	} else {
		if (len != sizeof(struct prev_cfa)) {
			dev_err(dev,
				"set_cfa_params: param struct length"
				" mismatch\n");
			return -EINVAL;
		}
		if (copy_from_user(&cfa, cfa_param, sizeof(struct prev_cfa))) {
			dev_err(dev,
				"set_cfa_params: Error in copy to kernel\n");
			return -EFAULT;
		}
		if (validate_cfa_params(dev) < 0)
			return -EINVAL;
	}

	/* Now set the values in the hw */
	return ipipe_set_cfa_regs(&cfa);
}
static int get_cfa_params(struct device *dev, void *param, int len)
{
	struct prev_cfa *cfa_param = (struct prev_cfa *)param;
	dev_dbg(dev, "get_cfa_params\n");
	if (ISNULL(cfa_param)) {
		dev_err(dev, "get_cfa_params: invalid user ptr");
		return -EINVAL;
	}
	if (len != sizeof(struct prev_cfa)) {
		dev_err(dev,
			"get_cfa_params: param struct length mismatch\n");
		return -EINVAL;
	}
	if (copy_to_user((struct prev_cfa *)cfa_param,
			 &cfa,
			 sizeof(struct prev_cfa))) {
		dev_err(dev, "get_cfa_params: Error in copy from kernel\n");
		return -EFAULT;
	}
	return 0;
}

static int validate_rgb2rgb_params(struct device *dev, unsigned int id)
{
#ifdef CONFIG_IPIPE_PARAM_VALIDATION
	struct prev_rgb2rgb *rgb2rgb = &rgb2rgb_1;
	u32 offset_upper = RGB2RGB_1_OFST_MASK,
	    gain_int_upper = RGB2RGB_1_GAIN_INT_MASK;

	if (id) {
		rgb2rgb = &rgb2rgb_2;
		offset_upper = RGB2RGB_2_OFST_MASK;
		gain_int_upper = RGB2RGB_2_GAIN_INT_MASK;
	}
	if (rgb2rgb->coef_rr.decimal > RGB2RGB_GAIN_DECI_MASK ||
	    rgb2rgb->coef_rr.integer > gain_int_upper)
		return -1;

	if (rgb2rgb->coef_gr.decimal > RGB2RGB_GAIN_DECI_MASK ||
	    rgb2rgb->coef_gr.integer > gain_int_upper)
		return -1;

	if (rgb2rgb->coef_br.decimal > RGB2RGB_GAIN_DECI_MASK ||
	    rgb2rgb->coef_br.integer > gain_int_upper)
		return -1;

	if (rgb2rgb->coef_rg.decimal > RGB2RGB_GAIN_DECI_MASK ||
	    rgb2rgb->coef_rg.integer > gain_int_upper)
		return -1;

	if (rgb2rgb->coef_gg.decimal > RGB2RGB_GAIN_DECI_MASK ||
	    rgb2rgb->coef_gg.integer > gain_int_upper)
		return -1;

	if (rgb2rgb->coef_bg.decimal > RGB2RGB_GAIN_DECI_MASK ||
	    rgb2rgb->coef_bg.integer > gain_int_upper)
		return -1;

	if (rgb2rgb->coef_rb.decimal > RGB2RGB_GAIN_DECI_MASK ||
	    rgb2rgb->coef_rb.integer > gain_int_upper)
		return -1;

	if (rgb2rgb->coef_gb.decimal > RGB2RGB_GAIN_DECI_MASK ||
	    rgb2rgb->coef_gb.integer > gain_int_upper)
		return -1;

	if (rgb2rgb->coef_bb.decimal > RGB2RGB_GAIN_DECI_MASK ||
	    rgb2rgb->coef_bb.integer > gain_int_upper)
		return -1;

	if (rgb2rgb->out_ofst_r > offset_upper ||
	    rgb2rgb->out_ofst_g > offset_upper ||
	    rgb2rgb->out_ofst_b > offset_upper)
		return -1;
#endif
	return 0;
}

static int set_rgb2rgb_params(struct device *dev, unsigned int id,
			      void *param, int len)
{
	struct prev_rgb2rgb *rgb2rgb = &rgb2rgb_1;
	struct prev_rgb2rgb *rgb2rgb_param = (struct prev_rgb2rgb *)param;

	if (id)
		rgb2rgb = &rgb2rgb_2;
	if (ISNULL(rgb2rgb_param)) {
		/* Copy defaults for rgb2rgb conversion */
		memcpy((void *)rgb2rgb,
		       (void *)&dm365_rgb2rgb_defaults,
		       sizeof(struct prev_rgb2rgb));
	} else {

		if (len != sizeof(struct prev_rgb2rgb)) {
			dev_err(dev,
				"set_rgb2rgb_params: param struct length"
				" mismatch\n");
			return -EINVAL;
		}

		if (copy_from_user(rgb2rgb,
				   rgb2rgb_param,
				   sizeof(struct prev_rgb2rgb))) {
			dev_err(dev,
				"set_rgb2rgb_params: Error in "
				"copy to kernel\n");
			return -EFAULT;
		}
		if (validate_rgb2rgb_params(dev, id) < 0)
			return -EINVAL;
	}
	return ipipe_set_rgb2rgb_regs(id, rgb2rgb);
}

static int set_rgb2rgb_1_params(struct device *dev, void *param, int len)
{
	return set_rgb2rgb_params(dev, 0, param, len);
}

static int set_rgb2rgb_2_params(struct device *dev, void *param, int len)
{
	return set_rgb2rgb_params(dev, 1, param, len);
}

static int get_rgb2rgb_params(struct device *dev, unsigned int id,
			      void *param, int len)
{
	struct prev_rgb2rgb *rgb2rgb_param = (struct prev_rgb2rgb *)param;
	struct prev_rgb2rgb *rgb2rgb = &rgb2rgb_1;

	if (ISNULL(rgb2rgb_param)) {
		dev_err(dev, "get_rgb2rgb_params: invalid user ptr");
		return -EINVAL;
	}

	if (len != sizeof(struct prev_rgb2rgb)) {
		dev_err(dev,
			"get_rgb2rgb_params: param struct length mismatch\n");
		return -EINVAL;
	}

	if (id)
		rgb2rgb = &rgb2rgb_2;
	if (copy_to_user((struct prev_rgb2rgb *)rgb2rgb_param,
			 rgb2rgb,
			 sizeof(struct prev_rgb2rgb))) {
		dev_err(dev, "get_rgb2rgb_params: Error in copy to user\n");
		return -EFAULT;
	}
	return 0;
}

static int get_rgb2rgb_1_params(struct device *dev, void *param, int len)
{
	return get_rgb2rgb_params(dev, 0, param, len);
}

static int get_rgb2rgb_2_params(struct device *dev, void *param, int len)
{
	return get_rgb2rgb_params(dev, 1, param, len);
}

static int validate_gamma_entry(struct ipipe_gamma_entry *table, int size)
{
#ifdef CONFIG_IPIPE_PARAM_VALIDATION
	int i;

	if (ISNULL(table))
		return -1;
	else {
		for (i = 0; i < size; i++) {
			if (table[i].slope > GAMMA_MASK ||
			    table[i].offset > GAMMA_MASK)
				return -1;
		}
	}
#endif
	return 0;
}

static int validate_gamma_params(struct device *dev)
{
#ifdef CONFIG_IPIPE_PARAM_VALIDATION
	int table_size = 0;
	int err = 0;
	if (gamma.bypass_r > 1 ||
	    gamma.bypass_b > 1 ||
	    gamma.bypass_g > 1)
		return -1;

	if (gamma.tbl_sel == IPIPE_GAMMA_TBL_RAM) {
		if (gamma.tbl_size == IPIPE_GAMMA_TBL_SZ_64)
			table_size = 64;
		if (gamma.tbl_size == IPIPE_GAMMA_TBL_SZ_128)
			table_size = 128;
		else if (gamma.tbl_size == IPIPE_GAMMA_TBL_SZ_256)
			table_size = 256;
		else if (gamma.tbl_size == IPIPE_GAMMA_TBL_SZ_512)
			table_size = 512;
		if (!gamma.bypass_r) {
			err = validate_gamma_entry(gamma.table_r, table_size);
			if (err) {
				dev_err(dev, "GAMMA R - table entry invalid\n");
				return err;
			}
		}
		if (!gamma.bypass_b) {
			err = validate_gamma_entry(gamma.table_b, table_size);
			if (err) {
				dev_err(dev, "GAMMA B - table entry invalid\n");
				return err;
			}
		}
		if (!gamma.bypass_g) {
			err = validate_gamma_entry(gamma.table_g, table_size);
			if (err) {
				dev_err(dev, "GAMMA G - table entry invalid\n");
				return err;
			}
		}
	}
#endif
	return 0;
}
static int set_gamma_params(struct device *dev, void *param, int len)
{
	int table_size = 0;
	struct prev_gamma user_gamma;
	struct prev_gamma *gamma_param = (struct prev_gamma *)param;

	if (ISNULL(gamma_param)) {
		/* Copy defaults for gamma */
		gamma.bypass_r = dm365_gamma_defaults.bypass_r;
		gamma.bypass_g = dm365_gamma_defaults.bypass_g;
		gamma.bypass_b = dm365_gamma_defaults.bypass_b;
		gamma.tbl_sel = dm365_gamma_defaults.tbl_sel;
		gamma.tbl_size = dm365_gamma_defaults.tbl_size;
		/* By default, we bypass the gamma correction.
		 * So no values by default for tables
		 */
	} else {
		if (len != sizeof(struct prev_gamma)) {
			dev_err(dev,
				"set_gamma_params: param struct length"
				" mismatch\n");
			return -EINVAL;
		}
		if (copy_from_user(&user_gamma, gamma_param,
				   sizeof(struct prev_gamma))) {
			dev_err(dev,
				"set_gamma_params: Error in copy to kernel\n");
			return -EFAULT;
		}
		gamma.bypass_r = user_gamma.bypass_r;
		gamma.bypass_b = user_gamma.bypass_b;
		gamma.bypass_g = user_gamma.bypass_g;
		gamma.tbl_sel = user_gamma.tbl_sel;
		gamma.tbl_size = user_gamma.tbl_size;

		if (user_gamma.tbl_sel == IPIPE_GAMMA_TBL_RAM) {
			if (user_gamma.tbl_size == IPIPE_GAMMA_TBL_SZ_64)
				table_size = 64;
			else if (user_gamma.tbl_size == IPIPE_GAMMA_TBL_SZ_128)
				table_size = 128;
			else if (user_gamma.tbl_size == IPIPE_GAMMA_TBL_SZ_256)
				table_size = 256;
			else if (user_gamma.tbl_size == IPIPE_GAMMA_TBL_SZ_512)
				table_size = 512;
			else {
				dev_err(dev,
					"set_gamma_params: Invalid"
					" table_size\n");
				return -EINVAL;
			}

			if (!user_gamma.bypass_r) {
				if (ISNULL(user_gamma.table_r)) {
					dev_err(dev,
						"set_gamma_params: Invalid"
						" table ptr for R\n");
					return -EINVAL;
				}
				if (copy_from_user(gamma.table_r,
						   user_gamma.table_r,
						   (table_size *
						   sizeof(struct \
						   ipipe_gamma_entry)))) {
					dev_err(dev,
						"set_gamma_params: R-Error"
						" in copy to kernel\n");
					return -EFAULT;
				}
			}

			if (!user_gamma.bypass_b) {
				if (ISNULL(user_gamma.table_b)) {
					dev_err(dev,
						"set_gamma_params: Invalid"
						" table ptr for B\n");
					return -EINVAL;
				}
				if (copy_from_user(gamma.table_b,
						   user_gamma.table_b,
						   (table_size *
						   sizeof(struct \
						   ipipe_gamma_entry)))) {
					dev_err(dev,
						"set_gamma_params: B-Error"
						" in copy to kernel\n");
					return -EFAULT;
				}
			}

			if (!user_gamma.bypass_g) {
				if (ISNULL(user_gamma.table_g)) {
					dev_err(dev,
						"set_gamma_params: Invalid"
						" table ptr for G\n");
					return -EINVAL;
				}
				if (copy_from_user(gamma.table_g,
						   user_gamma.table_g,
						   (table_size *
						   sizeof(struct \
						   ipipe_gamma_entry)))) {
					dev_err(dev,
						"set_gamma_params: G-Error "
						"in copy to kernel\n");
					return -EFAULT;
				}
			}
		}

		if (validate_gamma_params(dev) < 0)
			return -EINVAL;
	}
	return ipipe_set_gamma_regs(&gamma);
}
static int get_gamma_params(struct device *dev, void *param, int len)
{
	int table_size = 0;
	struct prev_gamma user_gamma;
	struct prev_gamma *gamma_param = (struct prev_gamma *)param;

	if (ISNULL(gamma_param)) {
		dev_err(dev, "get_gamma_params: invalid user ptr");
		return -EINVAL;
	}
	if (len != sizeof(struct prev_gamma)) {
		dev_err(dev,
			"get_gamma_params: param struct length mismatch\n");
		return -EINVAL;
	}
	if (copy_from_user(&user_gamma,
			   gamma_param,
			   sizeof(struct prev_gamma))) {
		dev_err(dev, "get_gamma_params: Error in copy to kernel\n");
		return -EFAULT;
	}

	user_gamma.bypass_r = gamma.bypass_r;
	user_gamma.bypass_g = gamma.bypass_g;
	user_gamma.bypass_b = gamma.bypass_b;
	user_gamma.tbl_sel = gamma.tbl_sel;
	user_gamma.tbl_size = gamma.tbl_size;
	if (gamma.tbl_sel == IPIPE_GAMMA_TBL_RAM) {
		if (gamma.tbl_size == IPIPE_GAMMA_TBL_SZ_128)
			table_size = 128;
		else if (gamma.tbl_size == IPIPE_GAMMA_TBL_SZ_256)
			table_size = 256;
		else if (gamma.tbl_size == IPIPE_GAMMA_TBL_SZ_512)
			table_size = 512;

		if ((!gamma.bypass_r) && ((ISNULL(user_gamma.table_r)))) {
			dev_err(dev,
				"get_gamma_params: table ptr empty for R\n");
			return -EINVAL;
		} else {
			if (copy_to_user(user_gamma.table_r,
					 gamma.table_r,
					 (table_size *
					 sizeof(struct ipipe_gamma_entry)))) {
				dev_err(dev,
					"set_gamma_params: R-Table Error in"
					" copy to user\n");
				return -EFAULT;
			}
		}

		if ((!gamma.bypass_b) && ((ISNULL(user_gamma.table_b)))) {
			dev_err(dev,
				"get_gamma_params: table ptr empty for B\n");
			return -EINVAL;
		} else {
			if (copy_to_user(user_gamma.table_b,
					 gamma.table_b,
					 (table_size *
					  sizeof(struct ipipe_gamma_entry)))) {
				dev_err(dev,
					"set_gamma_params: B-Table Error in"
					" copy to user\n");
				return -EFAULT;
			}
		}

		if ((!gamma.bypass_g) && ((ISNULL(user_gamma.table_g)))) {
			dev_err(dev,
				"get_gamma_params: table ptr empty for G\n");
			return -EINVAL;
		} else {
			if (copy_from_user(gamma.table_g,
					   user_gamma.table_g,
					   (table_size *
					   sizeof(struct ipipe_gamma_entry)))) {
				dev_err(dev,
					"set_gamma_params: G-Table"
					"copy error\n");
				return -EFAULT;
			}
		}

	}
	if (copy_to_user(gamma_param, &user_gamma, sizeof(struct prev_gamma))) {
		dev_err(dev, "get_dfc_params: Error in copy from kernel\n");
		return -EFAULT;
	}
	return 0;
}

static int validate_3d_lut_params(struct device *dev)
{
#ifdef CONFIG_IPIPE_PARAM_VALIDATION
	int i;
	if (lut_3d.en > 1)
		return -1;
	if (lut_3d.en) {
		for (i = 0; i < MAX_SIZE_3D_LUT; i++) {
			if (lut_3d.table[i].r > D3_LUT_ENTRY_MASK ||
			    lut_3d.table[i].g > D3_LUT_ENTRY_MASK ||
			    lut_3d.table[i].b > D3_LUT_ENTRY_MASK)
				return -1;
		}
	}
#endif
	return 0;
}
static int set_3d_lut_params(struct device *dev, void *param, int len)
{
	struct prev_3d_lut user_3d_lut;
	struct prev_3d_lut *lut_param = (struct prev_3d_lut *)param;

	if (ISNULL(lut_param)) {
		/* Copy defaults for gamma */
		lut_3d.en = dm365_3d_lut_defaults.en;
		/* By default, 3D lut is disabled
		 */
	} else {
		if (len != sizeof(struct prev_3d_lut)) {
			dev_err(dev,
				"set_3d_lut_params: param struct"
				" length mismatch\n");
			return -EINVAL;
		}
		if (copy_from_user(&user_3d_lut,
				   lut_param,
				   sizeof(struct prev_3d_lut))) {
			dev_err(dev,
				"set_3d_lut_params: Error in copy to"
				" kernel\n");
			return -EFAULT;
		}
		lut_3d.en = user_3d_lut.en;
		if (ISNULL(user_3d_lut.table)) {
			dev_err(dev, "set_3d_lut_params:" " Invalid table ptr");
			return -EINVAL;
		}
		if (copy_from_user(lut_3d.table,
				   user_3d_lut.table,
				   (MAX_SIZE_3D_LUT *
				   sizeof(struct ipipe_3d_lut_entry)))) {
			dev_err(dev,
				"set_3d_lut_params:Error"
				" in copy to kernel\n");
			return -EFAULT;
		}

		if (validate_3d_lut_params(dev) < 0)
			return -EINVAL;
	}
	return ipipe_set_3d_lut_regs(&lut_3d);
}
static int get_3d_lut_params(struct device *dev, void *param, int len)
{
	struct prev_3d_lut user_3d_lut;
	struct prev_3d_lut *lut_param = (struct prev_3d_lut *)param;

	if (ISNULL(lut_param)) {
		dev_err(dev, "get_3d_lut_params: invalid user ptr");
		return -EINVAL;
	}
	if (len != sizeof(struct prev_3d_lut)) {
		dev_err(dev,
			"get_3d_lut_params: param struct length mismatch\n");
		return -EINVAL;
	}
	if (copy_from_user(&user_3d_lut,
			   lut_param,
			   sizeof(struct prev_3d_lut))) {
		dev_err(dev, "get_3d_lut_params: Error in copy to" " kernel\n");
		return -EFAULT;
	}

	user_3d_lut.en = lut_3d.en;
	if (ISNULL(user_3d_lut.table)) {
		dev_err(dev, "get_3d_lut_params:" " Invalid table ptr");
		return -EINVAL;
	}
	if (copy_to_user(user_3d_lut.table, lut_3d.table,
			 (MAX_SIZE_3D_LUT *
			  sizeof(struct ipipe_3d_lut_entry)))) {
		dev_err(dev,
			"get_3d_lut_params:Table Error in" " copy to user\n");
		return -EFAULT;
	}

	if (copy_to_user(lut_param, &user_3d_lut, sizeof(struct prev_3d_lut))) {
		dev_err(dev, "get_3d_lut_params: Error in copy" " to user\n");
		return -EFAULT;
	}
	return 0;
}

static int validate_lum_adj_params(struct device *dev)
{
	/* nothing to validate */
	return 0;
}

static int set_lum_adj_params(struct device *dev, void *param, int len)
{
	struct prev_lum_adj *lum_adj_param = (struct prev_lum_adj *)param;

	if (ISNULL(lum_adj_param)) {
		/* Copy defaults for Luminance adjustments */
		memcpy((void *)&lum_adj,
		       (void *)&dm365_lum_adj_defaults,
		       sizeof(struct prev_lum_adj));
	} else {
		if (len != sizeof(struct prev_lum_adj)) {
			dev_err(dev,
				"set_lum_adj_params: param struct length"
				" mismatch\n");
			return -EINVAL;
		}
		if (copy_from_user(&lum_adj,
				   lum_adj_param,
				   sizeof(struct prev_lum_adj))) {
			dev_err(dev,
				"set_lum_adj_params: Error in copy"
				" from user\n");
			return -EFAULT;
		}
		if (validate_lum_adj_params(dev) < 0)
			return -EINVAL;
	}
	return ipipe_set_lum_adj_regs(&lum_adj);
}

static int get_lum_adj_params(struct device *dev, void *param, int len)
{
	struct prev_lum_adj *lum_adj_param = (struct prev_lum_adj *)param;
	if (ISNULL(lum_adj_param)) {
		dev_err(dev, "get_lum_adj_params: invalid user ptr");
		return -EINVAL;
	}

	if (len != sizeof(struct prev_lum_adj)) {
		dev_err(dev,
			"get_lum_adj_params: param struct length mismatch\n");
		return -EINVAL;
	}

	if (copy_to_user(lum_adj_param,
			 &lum_adj,
			 sizeof(struct prev_lum_adj))) {
		dev_err(dev, "get_lum_adj_params: Error in copy to" " user\n");
		return -EFAULT;
	}
	return 0;
}

static int validate_rgb2yuv_params(struct device *dev)
{
#ifdef CONFIG_IPIPE_PARAM_VALIDATION
	if (rgb2yuv.coef_ry.decimal > RGB2YCBCR_COEF_DECI_MASK ||
	    rgb2yuv.coef_ry.integer > RGB2YCBCR_COEF_INT_MASK)
		return -1;

	if (rgb2yuv.coef_gy.decimal > RGB2YCBCR_COEF_DECI_MASK ||
	    rgb2yuv.coef_gy.integer > RGB2YCBCR_COEF_INT_MASK)
		return -1;

	if (rgb2yuv.coef_by.decimal > RGB2YCBCR_COEF_DECI_MASK ||
	    rgb2yuv.coef_by.integer > RGB2YCBCR_COEF_INT_MASK)
		return -1;

	if (rgb2yuv.coef_rcb.decimal > RGB2YCBCR_COEF_DECI_MASK ||
	    rgb2yuv.coef_rcb.integer > RGB2YCBCR_COEF_INT_MASK)
		return -1;

	if (rgb2yuv.coef_gcb.decimal > RGB2YCBCR_COEF_DECI_MASK ||
	    rgb2yuv.coef_gcb.integer > RGB2YCBCR_COEF_INT_MASK)
		return -1;

	if (rgb2yuv.coef_bcb.decimal > RGB2YCBCR_COEF_DECI_MASK ||
	    rgb2yuv.coef_bcb.integer > RGB2YCBCR_COEF_INT_MASK)
		return -1;

	if (rgb2yuv.coef_rcr.decimal > RGB2YCBCR_COEF_DECI_MASK ||
	    rgb2yuv.coef_rcr.integer > RGB2YCBCR_COEF_INT_MASK)
		return -1;

	if (rgb2yuv.coef_gcr.decimal > RGB2YCBCR_COEF_DECI_MASK ||
	    rgb2yuv.coef_gcr.integer > RGB2YCBCR_COEF_INT_MASK)
		return -1;

	if (rgb2yuv.coef_bcr.decimal > RGB2YCBCR_COEF_DECI_MASK ||
	    rgb2yuv.coef_bcr.integer > RGB2YCBCR_COEF_INT_MASK)
		return -1;

	if (rgb2yuv.out_ofst_y > RGB2YCBCR_OFST_MASK ||
	    rgb2yuv.out_ofst_cb > RGB2YCBCR_OFST_MASK ||
	    rgb2yuv.out_ofst_cr > RGB2YCBCR_OFST_MASK)
		return -1;
#endif
	return 0;
}
static int set_rgb2yuv_params(struct device *dev, void *param, int len)
{
	struct prev_rgb2yuv *rgb2yuv_param = (struct prev_rgb2yuv *)param;

	if (ISNULL(rgb2yuv_param)) {
		/* Copy defaults for rgb2yuv conversion  */
		memcpy((void *)&rgb2yuv,
		       (void *)&dm365_rgb2yuv_defaults,
		       sizeof(struct prev_rgb2yuv));
	} else {
		if (len != sizeof(struct prev_rgb2yuv)) {
			dev_err(dev,
				"set_rgb2yuv_params: param struct"
				" length mismatch\n");
			return -EINVAL;
		}
		if (copy_from_user(&rgb2yuv,
				   rgb2yuv_param,
				   sizeof(struct prev_rgb2yuv))) {
			dev_err(dev,
				"set_rgb2yuv_params: Error in copy from"
				" user\n");
			return -EFAULT;
		}
		if (validate_rgb2yuv_params(dev) < 0)
			return -EINVAL;
	}
	return ipipe_set_rgb2ycbcr_regs(&rgb2yuv);
}
static int get_rgb2yuv_params(struct device *dev, void *param, int len)
{
	struct prev_rgb2yuv *rgb2yuv_param = (struct prev_rgb2yuv *)param;
	if (ISNULL(rgb2yuv_param)) {
		dev_err(dev, "get_rgb2yuv_params: invalid user ptr");
		return -EINVAL;
	}
	if (len != sizeof(struct prev_rgb2yuv)) {
		dev_err(dev,
			"get_rgb2yuv_params: param struct length mismatch\n");
		return -EINVAL;
	}
	if (copy_to_user((struct prev_rgb2yuv *)rgb2yuv_param,
			 &rgb2yuv,
			 sizeof(struct prev_rgb2yuv))) {
		dev_err(dev, "get_rgb2yuv_params: Error in copy from"
			" kernel\n");
		return -EFAULT;
	}
	return 0;
}

static int validate_gbce_params(struct device *dev)
{
#ifdef CONFIG_IPIPE_PARAM_VALIDATION
	int i;
	u32 max = GBCE_Y_VAL_MASK;
	if (gbce.type == IPIPE_GBCE_GAIN_TBL)
		max = GBCE_GAIN_VAL_MASK;
	if (gbce.en > 1)
		return -1;
	if (gbce.en) {
		for (i = 0; i < MAX_SIZE_GBCE_LUT; i++)
			if (gbce.table[i] > max)
				return -1;
	}
#endif
	return 0;
}
static int set_gbce_params(struct device *dev, void *param, int len)
{
	struct prev_gbce user_gbce;
	struct prev_gbce *gbce_param = (struct prev_gbce *)param;

	if (ISNULL(gbce_param))
		/* Copy defaults for gamma */
		gbce.en = dm365_gbce_defaults.en;
		/* By default, GBCE is disabled
		 */
	else {
		if (len != sizeof(struct prev_gbce)) {
			dev_err(dev,
				"set_gbce_params: param struct"
				" length mismatch\n");
			return -EINVAL;
		}
		if (copy_from_user(&user_gbce,
				   gbce_param,
				   sizeof(struct prev_gbce))) {
			dev_err(dev,
				"set_gbce_params: Error in copy to"
				" kernel\n");
			return -EFAULT;
		}
		gbce.en = user_gbce.en;
		gbce.type = user_gbce.type;
		if (ISNULL(user_gbce.table)) {
			dev_err(dev, "set_gbce_params:" " Invalid table ptr");
			return -EINVAL;
		}

		if (copy_from_user(gbce.table,
				   user_gbce.table,
				   (MAX_SIZE_GBCE_LUT *
				   sizeof(unsigned short)))) {
			dev_err(dev,
				"set_gbce_params:Error" " in copy to kernel\n");
			return -EFAULT;
		}
		if (validate_gbce_params(dev) < 0)
			return -EINVAL;
	}
	return ipipe_set_gbce_regs(&gbce);
}
static int get_gbce_params(struct device *dev, void *param, int len)
{
	struct prev_gbce user_gbce;
	struct prev_gbce *gbce_param = (struct prev_gbce *)param;

	if (ISNULL(gbce_param)) {
		dev_err(dev, "get_gbce_params: invalid user ptr");
		return -EINVAL;
	}
	if (len != sizeof(struct prev_gbce)) {
		dev_err(dev,
			"get_gbce_params: param struct length mismatch\n");
		return -EINVAL;
	}
	if (copy_from_user(&user_gbce, gbce_param, sizeof(struct prev_gbce))) {
		dev_err(dev, "get_gbce_params: Error in copy to" " kernel\n");
		return -EFAULT;
	}

	user_gbce.en = gbce.en;
	user_gbce.type = gbce.type;
	if (ISNULL(user_gbce.table)) {
		dev_err(dev, "get_gbce_params:" " Invalid table ptr");
		return -EINVAL;
	}
	if (copy_to_user(user_gbce.table,
			 gbce.table,
			 (MAX_SIZE_GBCE_LUT *
			 sizeof(unsigned short)))) {
		dev_err(dev,
			"get_gbce_params:Table Error in" " copy to user\n");
		return -EFAULT;
	}

	if (copy_to_user(gbce_param, &user_gbce, sizeof(struct prev_gbce))) {
		dev_err(dev, "get_gbce_params: Error in copy" " to user\n");
		return -EFAULT;
	}
	return 0;
}

static int validate_yuv422_conv_params(struct device *dev)
{
#ifdef CONFIG_IPIPE_PARAM_VALIDATION
	if (yuv422_conv.en_chrom_lpf > 1)
		return -1;
#endif
	return 0;
}

static int set_yuv422_conv_params(struct device *dev, void *param, int len)
{
	struct prev_yuv422_conv *yuv422_conv_param =
	    (struct prev_yuv422_conv *)param;
	if (ISNULL(yuv422_conv_param)) {
		/* Copy defaults for yuv 422 conversion */
		memcpy((void *)&yuv422_conv,
		       (void *)&dm365_yuv422_conv_defaults,
		       sizeof(struct prev_yuv422_conv));
	} else {
		if (len != sizeof(struct prev_yuv422_conv)) {
			dev_err(dev,
				"set_yuv422_conv_params: param struct"
				" length mismatch\n");
			return -EINVAL;
		}
		if (copy_from_user(&yuv422_conv,
				   yuv422_conv_param,
				   sizeof(struct prev_yuv422_conv))) {
			dev_err(dev,
				"set_yuv422_conv_params: Error in copy"
				" from user\n");
			return -EFAULT;
		}
		if (validate_yuv422_conv_params(dev) < 0)
			return -EINVAL;
	}
	return ipipe_set_yuv422_conv_regs(&yuv422_conv);
}
static int get_yuv422_conv_params(struct device *dev, void *param, int len)
{
	struct prev_yuv422_conv *yuv422_conv_param =
	    (struct prev_yuv422_conv *)param;

	if (ISNULL(yuv422_conv_param)) {
		dev_err(dev, "get_yuv422_conv_params: invalid user ptr");
		return -EINVAL;
	}
	if (len != sizeof(struct prev_yuv422_conv)) {
		dev_err(dev,
			"get_yuv422_conv_params: param struct"
			" length mismatch\n");
		return -EINVAL;
	}
	if (copy_to_user(yuv422_conv_param,
			 &yuv422_conv,
			 sizeof(struct prev_yuv422_conv))) {
		dev_err(dev,
			"get_yuv422_conv_params: Error in copy from kernel\n");
		return -EFAULT;
	}
	return 0;
}

static int validate_yee_params(struct device *dev)
{
#ifdef CONFIG_IPIPE_PARAM_VALIDATION
	int i;
	if (yee.en > 1 ||
	    yee.en_halo_red > 1 ||
	    yee.hpf_shft > YEE_HPF_SHIFT_MASK)
		return -1;

	if (yee.hpf_coef_00 > YEE_COEF_MASK ||
	    yee.hpf_coef_01 > YEE_COEF_MASK ||
	    yee.hpf_coef_02 > YEE_COEF_MASK ||
	    yee.hpf_coef_10 > YEE_COEF_MASK ||
	    yee.hpf_coef_11 > YEE_COEF_MASK ||
	    yee.hpf_coef_12 > YEE_COEF_MASK ||
	    yee.hpf_coef_20 > YEE_COEF_MASK ||
	    yee.hpf_coef_21 > YEE_COEF_MASK ||
	    yee.hpf_coef_22 > YEE_COEF_MASK)
		return -1;

	if (yee.yee_thr > YEE_THR_MASK ||
	    yee.es_gain > YEE_ES_GAIN_MASK ||
	    yee.es_thr1 > YEE_ES_THR1_MASK ||
	    yee.es_thr2 > YEE_THR_MASK ||
	    yee.es_gain_grad > YEE_THR_MASK ||
	    yee.es_ofst_grad > YEE_THR_MASK)
		return -1;

	for (i = 0; i < MAX_SIZE_YEE_LUT ; i++)
		if (yee.table[i] > YEE_ENTRY_MASK)
			return -1;
#endif
	return 0;
}
static int set_yee_params(struct device *dev, void *param, int len)
{
	short *temp_table;
	struct prev_yee user_yee;
	struct prev_yee *yee_param = (struct prev_yee *)param;

	if (ISNULL(yee_param)) {
		temp_table = yee.table;
		/* Copy defaults for ns */
		memcpy((void *)&yee,
		       (void *)&dm365_yee_defaults,
		       sizeof(struct prev_yee));
		yee.table = temp_table;
	} else {
		if (len != sizeof(struct prev_yee)) {
			dev_err(dev,
				"set_yee_params: param struct"
				" length mismatch\n");
			return -EINVAL;
		}
		if (copy_from_user(&user_yee,
				   yee_param,
				   sizeof(struct prev_yee))) {
			dev_err(dev,
				"set_yee_params: Error in copy from user\n");
			return -EFAULT;
		}
		if (ISNULL(user_yee.table)) {
			dev_err(dev, "get_yee_params: yee table ptr null\n");
			return -EINVAL;
		}
		if (copy_from_user(yee.table,
				   user_yee.table,
				   (MAX_SIZE_YEE_LUT * sizeof(short)))) {
			dev_err(dev,
				"set_yee_params: Error in copy from user\n");
			return -EFAULT;
		}
		temp_table = yee.table;
		memcpy(&yee, &user_yee, sizeof(struct prev_yee));
		yee.table = temp_table;
		if (validate_yee_params(dev) < 0)
			return -EINVAL;
	}
	return ipipe_set_ee_regs(&yee);
}
static int get_yee_params(struct device *dev, void *param, int len)
{
	short *temp_table;
	struct prev_yee *yee_param = (struct prev_yee *)param;
	struct prev_yee user_yee;

	if (ISNULL(yee_param)) {
		dev_err(dev, "get_yee_params: invalid user ptr");
		return -EINVAL;
	}
	if (len != sizeof(struct prev_yee)) {
		dev_err(dev,
			"get_yee_params: param struct"
			" length mismatch\n");
		return -EINVAL;
	}
	if (copy_from_user(&user_yee, yee_param, sizeof(struct prev_yee))) {
		dev_err(dev, "get_yee_params: Error in copy to kernel\n");
		return -EFAULT;
	}
	if (ISNULL(user_yee.table)) {
		dev_err(dev, "get_yee_params: yee table ptr null\n");
		return -EINVAL;
	}
	if (copy_to_user(user_yee.table, yee.table,
			 (MAX_SIZE_YEE_LUT * sizeof(short)))) {
		dev_err(dev, "get_yee_params: Error in copy from kernel\n");
		return -EFAULT;
	}
	temp_table = user_yee.table;
	memcpy(&user_yee, &yee, sizeof(struct prev_yee));
	user_yee.table = temp_table;

	if (copy_to_user(yee_param, &user_yee, sizeof(struct prev_yee))) {
		dev_err(dev, "get_yee_params: Error in copy from kernel\n");
		return -EFAULT;
	}
	return 0;
}

static int validate_car_params(struct device *dev)
{
#ifdef CONFIG_IPIPE_PARAM_VALIDATION
	if (car.en > 1 ||
	    car.hpf_shft > CAR_HPF_SHIFT_MASK ||
	    car.gain1.shft > CAR_GAIN1_SHFT_MASK ||
	    car.gain1.gain_min > CAR_GAIN_MIN_MASK ||
	    car.gain2.shft > CAR_GAIN2_SHFT_MASK ||
	    car.gain2.gain_min > CAR_GAIN_MIN_MASK)
		return -1;
#endif
	return 0;
}

static int set_car_params(struct device *dev, void *param, int len)
{
	struct prev_car *car_param = (struct prev_car *)param;

	if (ISNULL(car_param)) {
		/* Copy defaults for ns */
		memcpy((void *)&car,
		       (void *)&dm365_car_defaults,
		       sizeof(struct prev_car));
	} else {
		if (len != sizeof(struct prev_car)) {
			dev_err(dev,
				"set_car_params: param struct"
				" length mismatch\n");
			return -EINVAL;
		}
		if (copy_from_user(&car, car_param, sizeof(struct prev_car))) {
			dev_err(dev,
				"set_car_params: Error in copy from user\n");
			return -EFAULT;
		}
		if (validate_car_params(dev) < 0)
			return -EINVAL;
	}
	return ipipe_set_car_regs(&car);
}
static int get_car_params(struct device *dev, void *param, int len)
{
	struct prev_car *car_param = (struct prev_car *)param;

	if (ISNULL(car_param)) {
		dev_err(dev, "get_car_params: invalid user ptr");
		return -EINVAL;
	}
	if (len != sizeof(struct prev_car)) {
		dev_err(dev,
			"get_car_params: param struct"
			" length mismatch\n");
		return -EINVAL;
	}
	if (copy_to_user(car_param, &car, sizeof(struct prev_car))) {
		dev_err(dev, "get_car_params: Error in copy from kernel\n");
		return -EFAULT;
	}
	return 0;
}

static int validate_cgs_params(struct device *dev)
{
#ifdef CONFIG_IPIPE_PARAM_VALIDATION
	if (cgs.en > 1 ||
	    cgs.h_shft > CAR_SHIFT_MASK)
		return -1;
#endif
	return 0;
}

static int set_cgs_params(struct device *dev, void *param, int len)
{
	struct prev_cgs *cgs_param = (struct prev_cgs *)param;

	if (ISNULL(cgs_param)) {
		/* Copy defaults for ns */
		memcpy((void *)&cgs,
		       (void *)&dm365_cgs_defaults,
		       sizeof(struct prev_cgs));
	} else {
		if (len != sizeof(struct prev_cgs)) {
			dev_err(dev,
				"set_cgs_params: param struct"
				" length mismatch\n");
			return -EINVAL;
		}
		if (copy_from_user(&cgs, cgs_param, sizeof(struct prev_cgs))) {
			dev_err(dev,
				"set_cgs_params: Error in copy from user\n");
			return -EFAULT;
		}
		if (validate_cgs_params(dev) < 0)
			return -EINVAL;
	}
	return ipipe_set_cgs_regs(&cgs);
}
static int get_cgs_params(struct device *dev, void *param, int len)
{
	struct prev_cgs *cgs_param = (struct prev_cgs *)param;

	if (ISNULL(cgs_param)) {
		dev_err(dev, "get_cgs_params: invalid user ptr");
		return -EINVAL;
	}
	if (len != sizeof(struct prev_cgs)) {
		dev_err(dev,
			"get_cgs_params: param struct"
			" length mismatch\n");
		return -EINVAL;
	}
	if (copy_to_user(cgs_param, &cgs, sizeof(struct prev_cgs))) {
		dev_err(dev, "get_cgs_params: Error in copy from kernel\n");
		return -EFAULT;
	}
	return 0;
}

static struct prev_module_if *prev_enum_preview_cap(struct device *dev,
						    int index)
{
	dev_dbg(dev, "prev_enum_preview_cap: index = %d\n", index);
	if ((index + 1) > PREV_MAX_MODULES)
		return NULL;

	return &prev_modules[index];
}

static unsigned int prev_get_oper_mode(void)
{
	return oper_mode;
}

static unsigned int ipipe_get_oper_state(void)
{
	return oper_state.state;
}

static void ipipe_set_oper_state(unsigned int state)
{
	mutex_lock(&oper_state.lock);
	oper_state.state = state;
	mutex_unlock(&oper_state.lock);
}

static unsigned int ipipe_get_prev_config_state(void)
{
#ifndef CONFIG_VIDEO_YCBCR
	set_rgb2rgb_params(0, 0, 0, 0);
	set_rgb2rgb_params(0, 1, 0, 0);
#endif
	return oper_state.prev_config_state;
}

static unsigned int ipipe_get_rsz_config_state(void)
{
	return oper_state.rsz_config_state;
}

/* function: calculate_normal_f_div_param
 * Algorithm to calculate the frame division parameters for resizer.
 * in normal mode. Please refer the application note in DM360 functional
 * spec for details of the algorithm
 */
static int calculate_normal_f_div_param(struct device *dev,
					int input_width,
					int output_width,
					struct ipipe_rsz_rescale_param *param)
{
	/* rsz = R, input_width = H, output width = h in the equation */
	unsigned int rsz, temp, temp1, o, h1, h2;
	if (output_width > input_width) {
		dev_err(dev, "frame div mode is used for scale down only\n");
		return -1;
	}

	rsz = (input_width << 8) / output_width;
	temp = rsz << 1;
	temp1 = ((input_width << 8) / temp) + 1;
	o = 14;
	if (!(temp1 % 2)) {
		h1 = temp1;
	} else {
		temp1 = (input_width << 7);
		temp1 -= (rsz >> 1);
		temp1 /= (rsz << 1);
		temp1 <<= 1;
		temp1 += 2;
		o += ((CEIL(rsz, 1024)) << 1);
		h1 = temp1;
	}
	h2 = output_width - h1;
	/* phi */
	temp = (h1 * rsz) - (((input_width >> 1) - o) << 8);
	/* skip */
	temp1 = ((temp - 1024) >> 9) << 1;
	param->f_div.num_passes = IPIPE_MAX_PASSES;
	param->f_div.pass[0].o_hsz = h1 - 1;
	param->f_div.pass[0].i_hps = 0;
	param->f_div.pass[0].h_phs = 0;
	param->f_div.pass[0].src_hps = 0;
	param->f_div.pass[0].src_hsz = (input_width >> 2) + o;
	param->f_div.pass[1].o_hsz = h2 - 1;
	param->f_div.pass[1].i_hps = temp1;
	param->f_div.pass[1].h_phs = (temp - (temp1 << 8));
	param->f_div.pass[1].src_hps = (input_width >> 2) - o;
	param->f_div.pass[1].src_hsz = (input_width >> 2) + o;
	return 0;
}

/* function: calculate_down_scale_f_div_param
 * Algorithm to calculate the frame division parameters for resizer in
 * downscale mode. Please refer the application note in DM360 functional
 * spec for details of the algorithm
 */
static int calculate_down_scale_f_div_param(struct device *dev,
					    int input_width,
					    int output_width,
					    struct ipipe_rsz_rescale_param
					    *param)
{
	/* rsz = R, input_width = H, output width = h in the equation */
	unsigned int upper_h1, upper_h2, rsz, temp, temp1, o, h1, h2, two_power,
	    n;

	upper_h1 = input_width >> 1;
	n = param->h_dscale_ave_sz;
	/* 2 ^ (scale+1) */
	two_power = (1 << (n + 1));
	upper_h1 = (upper_h1 >> (n + 1)) << (n + 1);
	upper_h2 = input_width - upper_h1;
	if (upper_h2 % two_power) {
		dev_err(dev, "frame halves to be a multiple of 2 power n+1 \n");
		return -1;
	}
	two_power = (1 << n);
	rsz = (input_width << 8) / output_width;
	temp = rsz * two_power;
	temp1 = ((upper_h1 << 8) / temp) + 1;
	if (!(temp1 % 2))
		h1 = temp1;
	else {
		temp1 = (upper_h1 << 8);
		temp1 >>= (n + 1);
		temp1 -= (rsz >> 1);
		temp1 /= (rsz << 1);
		temp1 <<= 1;
		temp1 += 2;
		h1 = temp1;
	}
	o = 10 + (two_power << 2);
	if (((input_width << 7) / rsz) % 2)
		o += (((CEIL(rsz, 1024)) << 1) << n);
	h2 = output_width - h1;
	/* phi */
	temp = (h1 * rsz) - (((upper_h1 - (o - 10)) / two_power) << 8);
	/* skip */
	temp1 = ((temp - 1024) >> 9) << 1;
	param->f_div.num_passes = IPIPE_MAX_PASSES;
	param->f_div.pass[0].o_hsz = h1 - 1;
	param->f_div.pass[0].i_hps = 0;
	param->f_div.pass[0].h_phs = 0;
	param->f_div.pass[0].src_hps = 0;
	param->f_div.pass[0].src_hsz = upper_h1 + o;
	param->f_div.pass[1].o_hsz = h2 - 1;
	param->f_div.pass[1].i_hps = 10 + (temp1 * two_power);
	param->f_div.pass[1].h_phs = (temp - (temp1 << 8));
	param->f_div.pass[1].src_hps = (upper_h1 - o);
	param->f_div.pass[1].src_hsz = (upper_h2 + o);
	return 0;
}

/* update the parameter in param for a given input and output width */
static int update_preview_f_div_params(struct device *dev,
				       int input_width,
				       int output_width,
				       struct ipipe_rsz_rescale_param *param)
{
	unsigned int temp;

	temp = input_width >> 1;
	if (temp < 8) {
		dev_err(dev, "input width must me atleast 16 pixels\n");
		return -1;
	}
	param->f_div.en = 1;
	param->f_div.num_passes = IPIPE_MAX_PASSES;
	param->f_div.pass[0].o_hsz = temp;
	param->f_div.pass[0].i_hps = 0;
	param->f_div.pass[0].h_phs = 0;
	param->f_div.pass[0].src_hps = 0;
	param->f_div.pass[0].src_hsz = temp + 10;
	param->f_div.pass[1].o_hsz = temp;
	param->f_div.pass[1].i_hps = 0;
	param->f_div.pass[1].h_phs = 0;
	param->f_div.pass[1].src_hps = temp - 8;
	param->f_div.pass[1].src_hsz = temp + 10;
	return 0;
}

/* Use shared to allocate exclusive blocks as required
 * by resize applications in single shot mode
 */
static void *ipipe_alloc_config_block(struct device *dev, int shared)
{

	struct ipipe_params *config_block;
	if (shared) {
		/* return common data block */
		mutex_lock(&oper_state.lock);
		if (oper_state.resource_in_use) {
			dev_err(dev, "Shared resource in use\n");
			mutex_unlock(&oper_state.lock);
			return NULL;
		}
		mutex_unlock(&oper_state.lock);
		return oper_state.shared_config_param;
	} else {
		config_block = kmalloc(sizeof(struct ipipe_params), GFP_KERNEL);
		if (ISNULL(config_block)) {
			dev_err(dev,
				"imp_alloc_config_block: failed"
				" to allocate memory\n");
		}
	}
	return config_block;
}

/* Used to free only non-shared config block allocated through
 * imp_alloc_config_block
 */
static void ipipe_dealloc_config_block(struct device *dev, void *config_block)
{
	if (config_block) {
		if (config_block != oper_state.shared_config_param)
			kfree(config_block);
		else
			dev_err(dev, "Trying to free shared config block\n");
	}
}

static void ipipe_dealloc_user_config_block(struct device *dev,
					    void *config_block)
{
	kfree(config_block);
}

static void *ipipe_alloc_user_config_block(struct device *dev,
					   enum imp_log_chan_t chan_type,
					   unsigned int oper_mode, int *len)
{
	void *config = NULL;
	if (oper_mode == IMP_MODE_SINGLE_SHOT) {
		if (chan_type == IMP_PREVIEWER) {
			config =
			    kmalloc(sizeof(struct prev_single_shot_config),
				    GFP_KERNEL);
			*len = sizeof(struct prev_single_shot_config);
		} else if (chan_type == IMP_RESIZER) {
			config =
			    kmalloc(sizeof(struct rsz_single_shot_config),
				    GFP_KERNEL);
			*len = sizeof(struct rsz_single_shot_config);
		}

	} else {
		if (chan_type == IMP_PREVIEWER) {
			config =
			    kmalloc(sizeof(struct prev_continuous_config),
				    GFP_KERNEL);
			*len = sizeof(struct prev_continuous_config);
		} else if (chan_type == IMP_RESIZER) {
			config =
			    kmalloc(sizeof(struct rsz_continuous_config),
				    GFP_KERNEL);
			*len = sizeof(struct rsz_continuous_config);
		}
	}
	return config;
}

static void ipipe_set_user_config_defaults(struct device *dev,
					   enum imp_log_chan_t chan_type,
					   unsigned int oper_mode, void *config)
{
	dev_dbg(dev, "ipipe_set_user_config_defaults\n");
	if (oper_mode == IMP_MODE_SINGLE_SHOT) {
		if (chan_type == IMP_PREVIEWER) {
			dev_dbg(dev, "SS-Preview\n");
			/* preview channel in single shot mode */
			memcpy(config,
			       (void *)&dm365_prev_ss_config_defs,
			       sizeof(struct prev_single_shot_config));
		} else {
			dev_dbg(dev, "SS-Resize\n");
			/* resizer channel in single shot mode */
			memcpy(config,
			       (void *)&dm365_rsz_ss_config_defs,
			       sizeof(struct rsz_single_shot_config));
		}
	} else if (oper_mode == IMP_MODE_CONTINUOUS) {
		/* Continuous mode */
		if (chan_type == IMP_PREVIEWER) {
			dev_dbg(dev, "Cont Preview\n");
			/* previewer defaults */
			memcpy(config,
			       (void *)&dm365_prev_cont_config_defs,
			       sizeof(struct prev_continuous_config));
		} else {
			dev_dbg(dev, "Cont resize\n");
			/* resizer defaults */
			memcpy(config,
			       (void *)&dm365_rsz_cont_config_defs,
			       sizeof(struct rsz_continuous_config));
		}
	} else
		dev_err(dev, "Incorrect mode used\n");
}

/* function :calculate_sdram_offsets()
 *	This function calculates the offsets from start of buffer for the C
 *	plane when output format is YUV420SP. It also calculates the offsets
 *	from the start of the buffer when the image is flipped vertically
 *	or horizontally for ycbcr/y/c planes
 */
static int calculate_sdram_offsets(struct ipipe_params *param, int index)
{
	int image_height, image_width, offset = 0, yuv_420 = 0,
	    bytesperline = 2;

	if (!param->rsz_en[index])
		return -1;

	image_height = param->rsz_rsc_param[index].o_vsz + 1;
	image_width = param->rsz_rsc_param[index].o_hsz + 1;
	param->ext_mem_param[index].c_offset = 0;
	param->ext_mem_param[index].flip_ofst_y = 0;
	param->ext_mem_param[index].flip_ofst_c = 0;
	if ((param->ipipe_dpaths_fmt != IPIPE_RAW2RAW) &&
	    (param->ipipe_dpaths_fmt != IPIPE_RAW2BOX)) {
		if ((param->rsz_rsc_param[index].cen) &&
		    (param->rsz_rsc_param[index].yen)) {
			/* YUV 420 */
			yuv_420 = 1;
			bytesperline = 1;
		}
	}
	if (param->rsz_rsc_param[index].h_flip)
		/* width * bytesperline - 1 */
		offset = (image_width * bytesperline) - 1;
	if (param->rsz_rsc_param[index].v_flip)
		offset += ((image_height - 1) *
			   param->ext_mem_param[index].
			   rsz_sdr_oft_y);
	param->ext_mem_param[index].flip_ofst_y = offset;
	if (yuv_420) {
		offset = 0;
		/* half height for c-plane */
		if (param->rsz_rsc_param[index].h_flip)
			/* width * bytesperline - 1 */
			offset = image_width - 1;
			if (param->rsz_rsc_param[index].v_flip)
				offset +=
				    (((image_height >> 1) - 1) *
				     param->ext_mem_param[index].
				     rsz_sdr_oft_c);
			param->ext_mem_param[index].flip_ofst_c =
				    offset;
			param->ext_mem_param[index].c_offset =
			    param->ext_mem_param[index].
			    rsz_sdr_oft_y * image_height;
	}
	return 0;
}

static void enable_422_420_conversion(struct ipipe_params *param,
				      int index, enum enable_disable_t en)
{
	/* Enable 422 to 420 conversion */
	param->rsz_rsc_param[index].cen = en;
	param->rsz_rsc_param[index].yen = en;
}

static int configure_resizer_out_params(struct ipipe_params *param,
					int index,
					void *output_spec,
					unsigned char partial, unsigned flag)
{
	if (partial) {
		struct rsz_part_output_spec *partial_output =
		    (struct rsz_part_output_spec *)output_spec;
		if (partial_output->enable) {
			param->rsz_en[index] = ENABLE;
			param->rsz_rsc_param[index].h_flip =
			    partial_output->h_flip;
			param->rsz_rsc_param[index].v_flip =
			    partial_output->v_flip;
			param->rsz_rsc_param[index].v_typ_y =
			    partial_output->v_typ_y;
			param->rsz_rsc_param[index].v_typ_c =
			    partial_output->v_typ_c;
			param->rsz_rsc_param[index].v_lpf_int_y =
			    partial_output->v_lpf_int_y;
			param->rsz_rsc_param[index].v_lpf_int_c =
			    partial_output->v_lpf_int_c;
			param->rsz_rsc_param[index].h_typ_y =
			    partial_output->h_typ_y;
			param->rsz_rsc_param[index].h_typ_c =
			    partial_output->h_typ_c;
			param->rsz_rsc_param[index].h_lpf_int_y =
			    partial_output->h_lpf_int_y;
			param->rsz_rsc_param[index].h_lpf_int_c =
			    partial_output->h_lpf_int_c;
			param->rsz_rsc_param[index].dscale_en =
			    partial_output->en_down_scale;
			param->rsz_rsc_param[index].h_dscale_ave_sz =
			    partial_output->h_dscale_ave_sz;
			param->rsz_rsc_param[index].v_dscale_ave_sz =
			    partial_output->v_dscale_ave_sz;
			param->ext_mem_param[index].user_y_ofst =
			    (partial_output->user_y_ofst + 31) & ~0x1F;
			param->ext_mem_param[index].user_c_ofst =
			    (partial_output->user_c_ofst + 31) & ~0x1F;

		} else
			param->rsz_en[index] = DISABLE;

	} else {
		struct rsz_output_spec *output =
		    (struct rsz_output_spec *)output_spec;
		if (output->enable) {
			param->rsz_en[index] = ENABLE;
			param->rsz_rsc_param[index].o_vsz = output->height - 1;
			param->rsz_rsc_param[index].o_hsz = output->width - 1;
			param->ext_mem_param[index].rsz_sdr_ptr_s_y =
			    output->vst_y;
			param->ext_mem_param[index].rsz_sdr_ptr_e_y =
			    output->height;
			param->ext_mem_param[index].rsz_sdr_ptr_s_c =
			    output->vst_c;
			param->ext_mem_param[index].rsz_sdr_ptr_e_c =
			    output->height;

			if (flag) {
				/* update common parameters */
				param->rsz_rsc_param[index].h_flip =
				    output->h_flip;
				param->rsz_rsc_param[index].v_flip =
				    output->v_flip;
				param->rsz_rsc_param[index].v_typ_y =
				    output->v_typ_y;
				param->rsz_rsc_param[index].v_typ_c =
				    output->v_typ_c;
				param->rsz_rsc_param[index].v_lpf_int_y =
				    output->v_lpf_int_y;
				param->rsz_rsc_param[index].v_lpf_int_c =
				    output->v_lpf_int_c;
				param->rsz_rsc_param[index].h_typ_y =
				    output->h_typ_y;
				param->rsz_rsc_param[index].h_typ_c =
				    output->h_typ_c;
				param->rsz_rsc_param[index].h_lpf_int_y =
				    output->h_lpf_int_y;
				param->rsz_rsc_param[index].h_lpf_int_c =
				    output->h_lpf_int_c;
				param->rsz_rsc_param[index].dscale_en =
				    output->en_down_scale;
				param->rsz_rsc_param[index].h_dscale_ave_sz =
				    output->h_dscale_ave_sz;
				param->rsz_rsc_param[index].v_dscale_ave_sz =
				    output->h_dscale_ave_sz;
				param->ext_mem_param[index].user_y_ofst =
				    (output->user_y_ofst + 31) & ~0x1F;
				param->ext_mem_param[index].user_c_ofst =
				    (output->user_c_ofst + 31) & ~0x1F;
			}
		} else
			param->rsz_en[index] = DISABLE;
	}
	return 0;
}

/* function :calculate_line_length()
 *	This function calculates the line length of various image
 *	planes at the input and output
 */
static int calculate_line_length(enum ipipe_pix_formats pix,
				 int width,
				 int height, int *line_len, int *line_len_c)
{

	*line_len = 0;
	*line_len_c = 0;

	if ((pix == IPIPE_UYVY) || (pix == IPIPE_BAYER))
		*line_len = width << 1;
	else if (pix == IPIPE_420SP_Y || pix == IPIPE_420SP_C) {
		*line_len = width;
		*line_len_c = width;
	}
	else {
		/* YUV 420 */
		/* round width to upper 32 byte boundary */
		*line_len = width;
		*line_len_c = width;
	}
	/* adjust the line len to be a multiple of 32 */
	*line_len += 31;
	*line_len &= ~0x1f;
	*line_len_c += 31;
	*line_len_c &= ~0x1f;
	return 0;
}

static inline int rsz_validate_input_image_format(struct device *dev,
						  enum ipipe_pix_formats pix,
						  int width,
						  int height, int *line_len)
{
	int temp;
	if (pix != IPIPE_UYVY && pix != IPIPE_420SP_Y && pix != IPIPE_420SP_C) {
		dev_err(dev,
			"rsz_validate_out_pix_formats"
			"pix format not supported, %d\n", pix);
		return -EINVAL;
	}

	if (width == 0 || height == 0) {
		dev_err(dev, "validate_line_length: invalid width or height\n");
		return -EINVAL;
	}

	if (pix == IPIPE_420SP_C)
		return calculate_line_length(pix,
					      width,
					      height,
					      &temp,
					      line_len);
	else
		return calculate_line_length(pix,
					      width,
					      height,
					      line_len,
					      &temp);
}

static inline int rsz_validate_output_image_format(struct device *dev,
						   enum ipipe_pix_formats pix,
						   int width,
						   int height,
						   int *in_line_len,
						   int *in_line_len_c)
{
	if (pix != IPIPE_UYVY
	    && pix != IPIPE_420SP_Y
	    && pix != IPIPE_420SP_C
	    && pix != IPIPE_YUV420SP && pix != IPIPE_BAYER) {
		dev_err(dev,
			"rsz_validate_out_pix_formats"
			"pix format not supported, %d\n", pix);
		return -EINVAL;
	}

	if (width == 0 || height == 0) {
		dev_err(dev, "validate_line_length: invalid width or height\n");
		return -EINVAL;
	}

	return calculate_line_length(pix,
				      width,
				      height, in_line_len, in_line_len_c);
}

static int configure_common_rsz_params(struct device *dev,
				       struct ipipe_params *param,
				       struct rsz_single_shot_config *ss_config)
{
	param->rsz_common.yuv_y_min = ss_config->yuv_y_min;
	param->rsz_common.yuv_y_max = ss_config->yuv_y_max;
	param->rsz_common.yuv_c_min = ss_config->yuv_c_min;
	param->rsz_common.yuv_c_max = ss_config->yuv_c_max;
	param->rsz_common.out_chr_pos = ss_config->out_chr_pos;
	param->rsz_common.rsz_seq_crv = ss_config->chroma_sample_even;
	return 0;
}

static int configure_common_rsz_in_params(struct device *dev,
					  struct ipipe_params *param,
					  int flag, int rsz_chained,
					  void *input_spec)
{
	enum ipipe_pix_formats pix;
	if (!flag) {
		struct prev_ss_input_spec *in_specs =
		    (struct prev_ss_input_spec *)input_spec;
		param->rsz_common.vsz = in_specs->image_height - 1;
		param->rsz_common.hsz = in_specs->image_width - 1;
		pix = in_specs->pix_fmt;
	} else {
		struct rsz_ss_input_spec *in_specs =
		    (struct rsz_ss_input_spec *)input_spec;
		if (!rsz_chained) {
			param->rsz_common.vps = in_specs->vst;
			param->rsz_common.hps = in_specs->hst;
		}
		param->rsz_common.vsz = in_specs->image_height - 1;
		param->rsz_common.hsz = in_specs->image_width - 1;
		pix = in_specs->pix_fmt;
	}
	switch (pix) {
	case IPIPE_BAYER_8BIT_PACK:
	case IPIPE_BAYER_8BIT_PACK_ALAW:
	case IPIPE_BAYER_8BIT_PACK_DPCM:
	case IPIPE_BAYER_12BIT_PACK:
	case IPIPE_BAYER:
		{
			param->rsz_common.src_img_fmt = RSZ_IMG_422;
			param->rsz_common.source = IPIPE_DATA;
			break;
		}
	case IPIPE_UYVY:
		{
			param->rsz_common.src_img_fmt = RSZ_IMG_422;
			if (rsz_chained)
				param->rsz_common.source = IPIPE_DATA;
			else
				param->rsz_common.source = IPIPEIF_DATA;
			param->rsz_common.raw_flip = 0;
			break;
		}
	case IPIPE_420SP_Y:
		{
			param->rsz_common.src_img_fmt = RSZ_IMG_420;
			/* Select y */
			param->rsz_common.y_c = 0;
			param->rsz_common.source = IPIPEIF_DATA;
			param->rsz_common.raw_flip = 0;
			break;
		}
	case IPIPE_420SP_C:
		{
			param->rsz_common.src_img_fmt = RSZ_IMG_420;
			/* Select y */
			param->rsz_common.y_c = 1;
			param->rsz_common.source = IPIPEIF_DATA;
			param->rsz_common.raw_flip = 0;
			break;
		}
	default:
		return -1;
	}
	return 0;
}

static int validate_ipipeif_decimation(struct device *dev,
				       enum ipipeif_decimation dec_en,
				       unsigned char rsz,
				       unsigned char frame_div_mode_en,
				       int width)
{
	if (dec_en && frame_div_mode_en) {
		dev_err(dev,
			"Both dec_en & frame_div_mode_en"
			"can not enabled simultaneously\n");
		return -EINVAL;
	}
	if (frame_div_mode_en) {
		dev_err(dev, "frame_div_mode mode not supported");
		return -EINVAL;
	}
	if (dec_en) {
		if (width <= IPIPE_MAX_INPUT_WIDTH) {
			dev_err(dev,
				"image width to be more than"
				" %d for decimation\n", IPIPE_MAX_INPUT_WIDTH);
			return -EINVAL;
		}
		if ((rsz < IPIPEIF_RSZ_MIN) || (rsz > IPIPEIF_RSZ_MAX)) {
			dev_err(dev, "rsz range is %d to %d\n",
				IPIPEIF_RSZ_MIN, IPIPEIF_RSZ_MAX);
			return -EINVAL;
		}
	}
	return 0;
}

static int configure_resizer_in_ss_mode(struct device *dev,
					void *user_config,
					int resizer_chained,
					struct ipipe_params *param)
{
	/* resizer in standalone mode. In this mode if serializer
	 * is enabled, we need to set config params in the hw.
	 */
	struct rsz_single_shot_config *ss_config =
	    (struct rsz_single_shot_config *)user_config;
	int ret = 0, line_len, line_len_c;

	ret = rsz_validate_input_image_format(dev,
					      ss_config->input.pix_fmt,
					      ss_config->input.image_width,
					      ss_config->input.image_height,
					      &line_len);

	if (ret)
		return -EINVAL;

	/* shared block */
	if ((!ss_config->output1.enable) && (!ss_config->output2.enable)) {
		dev_err(dev, "One of the resizer output must be enabled\n");
		return -EINVAL;
	}
	ret = mutex_lock_interruptible(&oper_state.lock);
	if (ret)
		return ret;
	if (!ss_config->input.line_length)
		param->ipipeif_param.adofs = line_len;
	else {
		param->ipipeif_param.adofs = ss_config->input.line_length;
		param->ipipeif_param.adofs =
				(param->ipipeif_param.adofs + 31) & ~0x1f;
	}
	if (ss_config->output1.enable) {
		param->rsz_en[RSZ_A] = ENABLE;
		param->rsz_rsc_param[RSZ_A].mode = ONE_SHOT;
		ret = rsz_validate_output_image_format(dev,
						       ss_config->output1.
						       pix_fmt,
						       ss_config->output1.width,
						       ss_config->output1.
						       height,
						       &line_len,
						       &line_len_c);
		if (ret) {
			mutex_unlock(&oper_state.lock);
			return ret;
		}
		param->ext_mem_param[RSZ_A].rsz_sdr_oft_y = line_len;
		param->ext_mem_param[RSZ_A].rsz_sdr_oft_c = line_len_c;
		ret = configure_resizer_out_params(param,
						   RSZ_A,
						   &ss_config->output1,
						   0,
						   1);

		if (ss_config->output1.pix_fmt == IMP_BAYER)
			param->rsz_common.raw_flip = 1;
		else
			param->rsz_common.raw_flip = 0;

		if (ss_config->output1.pix_fmt == IPIPE_YUV420SP)
			enable_422_420_conversion(param, RSZ_A, ENABLE);
		else
			enable_422_420_conversion(param, RSZ_A,
							  DISABLE);
		if (ret) {
			mutex_unlock(&oper_state.lock);
			dev_err(dev, "error configuring resizer output1\n");
			return ret;
		}
	}

	if (ss_config->output2.enable) {
		param->rsz_en[RSZ_A] = ENABLE;
		param->rsz_rsc_param[RSZ_B].mode = ONE_SHOT;
		ret = rsz_validate_output_image_format(dev,
						       ss_config->output2.
						       pix_fmt,
						       ss_config->output2.width,
						       ss_config->output2.
						       height,
						       &line_len,
						       &line_len_c);
		if (ret) {
			mutex_unlock(&oper_state.lock);
			return ret;
		}
		param->ext_mem_param[RSZ_B].rsz_sdr_oft_y = line_len;
		param->ext_mem_param[RSZ_B].rsz_sdr_oft_c = line_len_c;
		ret = configure_resizer_out_params(param,
						   RSZ_B,
						   &ss_config->output2,
						   0,
						   1);
		if (ret) {
			dev_err(dev,
				"error configuring resizer output2 params\n");
			mutex_unlock(&oper_state.lock);
			return ret;
		}
		if (ss_config->output2.pix_fmt == IPIPE_YUV420SP)
			enable_422_420_conversion(param, RSZ_B, ENABLE);
		else
			enable_422_420_conversion(param,
						  RSZ_B,
						  DISABLE);
	}
	configure_common_rsz_params(dev, param, ss_config);
	if (resizer_chained) {
		oper_state.rsz_chained = 1;
		oper_state.rsz_config_state = STATE_CONFIGURED;
	} else {
		oper_state.rsz_chained = 0;
		ret = validate_ipipeif_decimation(dev,
						  ss_config->input.dec_en,
						  ss_config->input.rsz,
						  ss_config->input.
						  frame_div_mode_en,
						  ss_config->input.image_width);
		if (ret) {
			mutex_unlock(&oper_state.lock);
			return ret;
		}

		if (ipipe_process_pix_fmts(ss_config->input.pix_fmt,
				   ss_config->output1.pix_fmt,
				   param) < 0) {
			dev_err(dev, "error in input or output pix format\n");
			mutex_unlock(&oper_state.lock);
			return -EINVAL;
		}

		param->ipipeif_param.source = SDRAM_YUV;
		param->ipipeif_param.glob_hor_size = ss_config->input.ppln;
		param->ipipeif_param.glob_ver_size = ss_config->input.lpfr;
		param->ipipeif_param.hnum = ss_config->input.image_width;
		param->ipipeif_param.vnum = ss_config->input.image_height;
		param->ipipeif_param.var.if_5_1.clk_div =
		    ss_config->input.clk_div;
		if (ss_config->input.dec_en) {
			param->ipipeif_param.decimation = IPIPEIF_DECIMATION_ON;
			param->ipipeif_param.rsz = ss_config->input.rsz;
			param->ipipeif_param.avg_filter =
			    (enum ipipeif_avg_filter)ss_config->input.
			    avg_filter_en;
			param->ipipe_hsz =
			    (((ss_config->input.image_width *
			       IPIPEIF_RSZ_CONST) / ss_config->input.rsz) - 1);
		}
		if (ss_config->input.pix_fmt == IPIPE_420SP_Y
			|| ss_config->input.pix_fmt == IPIPE_420SP_C)
		{
			param->ipipeif_param.var.if_5_1.pack_mode
				= IPIPEIF_5_1_PACK_8_BIT;
			param->ipipeif_param.var.if_5_1.source1 = CCDC;
			param->ipipeif_param.var.if_5_1.isif_port.if_type
				= VPFE_YCBCR_SYNC_16;
			param->ipipeif_param.var.if_5_1.data_shift
				= IPIPEIF_5_1_BITS11_0;

			param->ipipeif_param.source = SDRAM_RAW;


		}
		if (ss_config->input.pix_fmt == IPIPE_420SP_C)
			param->ipipeif_param.var.if_5_1.isif_port.if_type
				= VPFE_RAW_BAYER;
		param->ipipe_hsz = ss_config->input.image_width - 1;
		param->ipipe_vsz = ss_config->input.image_height - 1;
		param->ipipe_vps = ss_config->input.vst;
		param->ipipe_hps = ss_config->input.hst;
		param->ipipe_dpaths_fmt = IPIPE_YUV2YUV;
		configure_common_rsz_in_params(dev, param, 1, resizer_chained,
					       &ss_config->input);
		if (param->rsz_en[RSZ_A]) {

			calculate_resize_ratios(param, RSZ_A);
			calculate_sdram_offsets(param, RSZ_A);

			/* Overriding resize ratio calculation */
			if (ss_config->input.pix_fmt == IPIPE_420SP_C)
			{
				param->rsz_rsc_param[RSZ_A].v_dif =
				    (((param->ipipe_vsz + 1) * 2) * 256) /
				    (param->rsz_rsc_param[RSZ_A].o_vsz + 1);
			}
		}

		if (param->rsz_en[RSZ_B]) {
			calculate_resize_ratios(param, RSZ_B);
			calculate_sdram_offsets(param, RSZ_B);

			/* Overriding resize ratio calculation */
			if (ss_config->input.pix_fmt == IPIPE_420SP_C)
			{
				param->rsz_rsc_param[RSZ_B].v_dif =
				    (((param->ipipe_vsz + 1) * 2) * 256) /
				    (param->rsz_rsc_param[RSZ_B].o_vsz + 1);
			}
		}
	}
	mutex_unlock(&oper_state.lock);
	return 0;
}

static int configure_resizer_in_cont_mode(struct device *dev,
					  void *user_config,
					  int resizer_chained,
					  struct ipipe_params *param)
{
	/* Continuous mode. This is a shared config block */
	struct rsz_continuous_config *cont_config =
	    (struct rsz_continuous_config *)user_config;
	int line_len, line_len_c, ret;

	if (resizer_chained) {
		ret = mutex_lock_interruptible(&oper_state.lock);
		if (ret)
			return ret;
		if (!cont_config->output1.enable) {
			dev_err(dev, "enable resizer - 0\n");
			mutex_unlock(&oper_state.lock);
			return -EINVAL;
		}
		param->rsz_en[RSZ_A] = ENABLE;
		param->rsz_rsc_param[RSZ_A].mode = CONTINUOUS;
		ret = configure_resizer_out_params(param,
						   RSZ_A,
						   &cont_config->output1,
						   1,
						   0);
		if (ret) {
			dev_err(dev, "error configuring resizer output1\n");
			mutex_unlock(&oper_state.lock);
			return -EINVAL;
		}
		param->rsz_en[RSZ_B] = DISABLE;

		if (cont_config->output2.enable) {
			param->rsz_rsc_param[RSZ_B].mode = CONTINUOUS;
			ret = rsz_validate_output_image_format(dev,
						       cont_config->output2.
						       pix_fmt,
						       cont_config->output2.
						       width,
						       cont_config->output2.
						       height,
						       &line_len,
						       &line_len_c);
			if (ret) {
				mutex_unlock(&oper_state.lock);
				return ret;
			}
			param->ext_mem_param[RSZ_B].rsz_sdr_oft_y = line_len;
			param->ext_mem_param[RSZ_B].rsz_sdr_oft_c = line_len_c;
			ret = configure_resizer_out_params(param,
						   RSZ_B,
						   &cont_config->output2,
						   0,
						   1);
			if (cont_config->output2.pix_fmt == IPIPE_YUV420SP)
				enable_422_420_conversion(param,
						RSZ_B, ENABLE);
			else
				enable_422_420_conversion(param,
						RSZ_B, DISABLE);

			if (ret) {
				dev_err(dev, "error configuring resizer"
					" output2\n");
				mutex_unlock(&oper_state.lock);
				return -EINVAL;
			}
		}
		oper_state.rsz_chained = 1;
		oper_state.rsz_config_state = STATE_CONFIGURED;
		mutex_unlock(&oper_state.lock);
	} else {
		dev_err(dev, "Resizer cannot be configured in standalone"
			"for continuous mode\n");
		return -EINVAL;
	}
	return 0;
}
static int ipipe_set_resize_config(struct device *dev,
				   unsigned int oper_mode,
				   int resizer_chained,
				   void *user_config, void *config)
{
	int ret = 0;
	struct ipipe_params *param = (struct ipipe_params *)config;

	dev_dbg(dev, "ipipe_set_resize_config, resizer_chained = %d\n",
		resizer_chained);
	if ((ISNULL(user_config)) || (ISNULL(config))) {
		dev_err(dev, "Invalid user_config or config ptr\n");
		return -EINVAL;
	}

	memcpy((void *)config,
	       (void *)&dm365_ipipe_defs,
	       sizeof(struct ipipe_params));

	if (oper_mode == IMP_MODE_SINGLE_SHOT) {
		ret = configure_resizer_in_ss_mode(dev,
						   user_config,
						   resizer_chained,
						   param);
		if (!ret && (!en_serializer && !resizer_chained))
			ret = ipipe_hw_setup(config);
	} else
		ret = configure_resizer_in_cont_mode(dev,
						     user_config,
						     resizer_chained,
						     param);
	return ret;
}

static void configure_resize_passthru(struct ipipe_params *param, int bypass)
{
	param->rsz_rsc_param[RSZ_A].cen = DISABLE;
	param->rsz_rsc_param[RSZ_A].yen = DISABLE;
	param->rsz_rsc_param[RSZ_A].v_phs_y = 0;
	param->rsz_rsc_param[RSZ_A].v_phs_c = 0;
	param->rsz_rsc_param[RSZ_A].v_dif = 256;
	param->rsz_rsc_param[RSZ_A].v_lpf_int_y = 0;
	param->rsz_rsc_param[RSZ_A].v_lpf_int_c = 0;
	param->rsz_rsc_param[RSZ_A].h_phs = 0;
	param->rsz_rsc_param[RSZ_A].h_dif = 256;
	param->rsz_rsc_param[RSZ_A].h_lpf_int_y = 0;
	param->rsz_rsc_param[RSZ_A].h_lpf_int_c = 0;
	param->rsz_rsc_param[RSZ_A].dscale_en = DISABLE;
	param->rsz2rgb[RSZ_A].rgb_en = DISABLE;
	param->rsz_en[RSZ_A] = ENABLE;
	param->rsz_en[RSZ_B] = DISABLE;
	if (bypass) {
		param->rsz_rsc_param[RSZ_A].i_vps = 0;
		param->rsz_rsc_param[RSZ_A].i_hps = 0;
		/* Raw Bypass */
		param->rsz_common.passthrough = IPIPE_BYPASS_ON;
	}
}

static inline int prev_validate_output_image_format(struct device *dev,
						    enum ipipe_pix_formats pix,
						    int *line_len,
						    int in_width, int in_height)
{
	if (pix != IPIPE_UYVY && pix != IPIPE_BAYER) {
		dev_err(dev,
			"prev_validate_output_image_format"
			"pix format not supported, %d\n", pix);
		return -EINVAL;
	}

	if ((in_width == 0) || (in_height == 0)) {
		dev_err(dev,
			"prev_validate_output_image_format:"
			" invalid width or height\n");
		return -EINVAL;
	}

	*line_len = in_width * 2;

	/* Adjust line length to be a multiple of 32 */
	*line_len += 31;
	*line_len &= ~0x1f;
	return 0;
}

static inline int validate_preview_input_spec(struct device *dev,
					      enum ipipe_pix_formats pix,
					      int width,
					      int height, int *line_len)
{
	if (pix != IPIPE_UYVY
	    && pix != IPIPE_BAYER
	    && pix != IPIPE_BAYER_8BIT_PACK
	    && pix != IPIPE_BAYER_8BIT_PACK_ALAW
	    && pix != IPIPE_BAYER_8BIT_PACK_DPCM
	    && pix != IPIPE_BAYER_12BIT_PACK) {
		dev_err(dev,
			"validate_preview_input_spec:"
			"pix format not supported, %d\n", pix);
		return -EINVAL;
	}
	if (width == 0 || height == 0) {
		dev_err(dev,
			"rsz_validate_out_image_formats: "
			"invalid width or height\n");
		return -EINVAL;
	}

	if (pix == IPIPE_UYVY || pix == IPIPE_BAYER)
		*line_len = width * 2;
	else if (pix == IPIPE_BAYER_8BIT_PACK ||
		 pix == IPIPE_BAYER_8BIT_PACK_ALAW ||
		 pix == IPIPE_BAYER_8BIT_PACK_DPCM)
		*line_len = width;
	else
		/* 12 bit */
		*line_len = width + (width >> 1);
	/* Adjust line length to be a multiple of 32 */
	*line_len += 31;
	*line_len &= ~0x1f;
	return 0;
}

static int configure_previewer_in_cont_mode(struct device *dev,
					    void *user_config,
					    struct ipipe_params *param)
{
	int ret;
	struct prev_continuous_config *cont_config =
	    (struct prev_continuous_config *)user_config;

	if (cont_config->input.en_df_sub) {
		dev_err(dev, "DF subtraction is not supported\n");
		return -EINVAL;
	}
	if (cont_config->input.dec_en) {
		if ((cont_config->input.rsz < IPIPEIF_RSZ_MIN)
		    || (cont_config->input.rsz > IPIPEIF_RSZ_MAX)) {
			dev_err(dev, "rsz range is %d to %d\n",
				IPIPEIF_RSZ_MIN, IPIPEIF_RSZ_MAX);
			return -EINVAL;
		}
	}
	ret = mutex_lock_interruptible(&oper_state.lock);
	if (ret)
		return ret;
	param->rsz_common.passthrough = cont_config->bypass;
	param->ipipeif_param.source = CCDC;
	param->ipipeif_param.clock_select = PIXCEL_CLK;
	param->ipipeif_param.mode = CONTINUOUS;
	if (cont_config->input.dec_en) {
		param->ipipeif_param.decimation = IPIPEIF_DECIMATION_ON;
		param->ipipeif_param.rsz = cont_config->input.rsz;
		param->ipipeif_param.avg_filter =
		    (enum ipipeif_avg_filter)cont_config->input.avg_filter_en;
	}
	/* IPIPE mode */
	param->ipipe_mode = CONTINUOUS;
	param->ipipe_colpat_olop = cont_config->input.colp_olop;
	param->ipipe_colpat_olep = cont_config->input.colp_olep;
	param->ipipe_colpat_elop = cont_config->input.colp_elop;
	param->ipipe_colpat_elep = cont_config->input.colp_elep;
	param->ipipeif_param.gain = cont_config->input.gain;
	param->ipipeif_param.var.if_5_1.clip = cont_config->input.clip;
	param->ipipeif_param.var.if_5_1.dpc = cont_config->input.dpc;
	param->ipipeif_param.var.if_5_1.align_sync =
	    cont_config->input.align_sync;
	param->ipipeif_param.var.if_5_1.rsz_start =
	    cont_config->input.rsz_start;
	if (!oper_state.rsz_chained) {
		param->rsz_rsc_param[0].mode = CONTINUOUS;
		/* setup bypass resizer */
		configure_resize_passthru(param, 0);
	}
	if (cont_config->bypass)
		configure_resize_passthru(param, 1);
	oper_state.prev_config_state = STATE_CONFIGURED;
	mutex_unlock(&oper_state.lock);
	return 0;
}

static int configure_previewer_in_ss_mode(struct device *dev,
					  void *user_config,
					  struct ipipe_params *param)
{
	int ret = 0;
	int line_len;
	struct prev_single_shot_config *ss_config =
	    (struct prev_single_shot_config *)user_config;

	ret = validate_preview_input_spec(dev,
					  ss_config->input.pix_fmt,
					  ss_config->input.image_width,
					  ss_config->input.image_height,
					  &line_len);
	if (ret)
		return -EINVAL;

	ret = mutex_lock_interruptible(&oper_state.lock);
	if (ret)
		return ret;

	if (!ss_config->input.line_length)
		param->ipipeif_param.adofs = line_len;
	else {
		param->ipipeif_param.adofs = ss_config->input.line_length;
		param->ipipeif_param.adofs =
				(param->ipipeif_param.adofs + 31) & ~0x1f;
	}
	if (ss_config->input.dec_en && ss_config->input.frame_div_mode_en) {
		dev_err(dev,
			"Both dec_en & frame_div_mode_en"
			"can not enabled simultaneously\n");
		mutex_unlock(&oper_state.lock);
		return -EINVAL;
	}

	ret = validate_ipipeif_decimation(dev,
					  ss_config->input.dec_en,
					  ss_config->input.rsz,
					  ss_config->input.frame_div_mode_en,
					  ss_config->input.image_width);
	if (ret) {
		mutex_unlock(&oper_state.lock);
		return -EINVAL;
	}

	if (!oper_state.rsz_chained) {
		ret = prev_validate_output_image_format(dev,
							ss_config->output.
							pix_fmt, &line_len,
							ss_config->input.
							image_width,
							ss_config->input.
							image_height);
		if (ret) {
			mutex_unlock(&oper_state.lock);
			return -EINVAL;
		}
		param->ext_mem_param[RSZ_A].rsz_sdr_oft_y = line_len;
		if (ss_config->input.frame_div_mode_en)
			ret = update_preview_f_div_params(dev,
							  ss_config->input.
							  image_width,
							  ss_config->input.
							  image_width,
							  &param->
							  rsz_rsc_param[RSZ_A]);
		if (ret) {
			mutex_unlock(&oper_state.lock);
			return -EINVAL;
		}
	} else {
		if (ss_config->input.frame_div_mode_en &&
		    param->rsz_en[RSZ_A]) {
			if (!param->rsz_rsc_param[RSZ_A].dscale_en)
				ret = calculate_normal_f_div_param(
							dev,
							ss_config->input.
							  image_width,
							param->rsz_rsc_param
							  [RSZ_A].
							  o_vsz + 1,
							&param->rsz_rsc_param
							  [RSZ_A]);
			else
				ret = calculate_down_scale_f_div_param(
							dev,
							ss_config->
							input.image_width,
							param->rsz_rsc_param
							  [RSZ_A].o_vsz + 1,
							&param->rsz_rsc_param
							  [RSZ_A]);
			if (ret) {
				mutex_unlock(&oper_state.lock);
				return -EINVAL;
			}
		}
		if (ss_config->input.frame_div_mode_en &&
		    param->rsz_en[RSZ_B]) {
			if (!param->rsz_rsc_param[RSZ_B].dscale_en)
				ret = calculate_normal_f_div_param(
							dev,
							ss_config->input.
							   image_width,
							param->rsz_rsc_param
							   [RSZ_B].o_vsz + 1,
							&param->rsz_rsc_param
							   [RSZ_B]);
			else
				ret = calculate_down_scale_f_div_param(
							dev,
							ss_config->input.
							   image_width,
							param->rsz_rsc_param
							   [RSZ_B].o_vsz + 1,
							&param->rsz_rsc_param
							   [RSZ_B]);
			if (ret) {
				mutex_unlock(&oper_state.lock);
				return -EINVAL;
			}
		}
	}
	if (ipipe_process_pix_fmts(ss_config->input.pix_fmt,
				   ss_config->output.pix_fmt,
				   param) < 0) {
		dev_err(dev, "error in input or output pix format\n");
		mutex_unlock(&oper_state.lock);
		return -EINVAL;
	}
	param->ipipeif_param.hnum = ss_config->input.image_width;
	param->ipipeif_param.vnum = ss_config->input.image_height;
	param->ipipeif_param.glob_hor_size = ss_config->input.ppln;
	param->ipipeif_param.glob_ver_size = ss_config->input.lpfr;
	param->ipipeif_param.var.if_5_1.clk_div = ss_config->input.clk_div;
	param->ipipeif_param.var.if_5_1.pix_order = ss_config->input.pix_order;
	param->ipipeif_param.var.if_5_1.align_sync =
	    ss_config->input.align_sync;
	param->ipipeif_param.var.if_5_1.rsz_start = ss_config->input.rsz_start;
	if (param->ipipeif_param.var.if_5_1.dpcm.en) {
		param->ipipeif_param.var.if_5_1.dpcm.pred =
		    ss_config->input.pred;
		param->ipipeif_param.var.if_5_1.dpcm.type = DPCM_8BIT_12BIT;
	}
	param->ipipeif_param.var.if_5_1.data_shift =
	    ss_config->input.data_shift;

	param->ipipe_hsz = ss_config->input.image_width - 1;
	if (ss_config->input.dec_en) {
		if ((ss_config->input.rsz < IPIPEIF_RSZ_MIN) ||
		    (ss_config->input.rsz > IPIPEIF_RSZ_MAX)) {
			dev_err(dev, "rsz range is %d to %d\n",
				IPIPEIF_RSZ_MIN, IPIPEIF_RSZ_MAX);
			mutex_unlock(&oper_state.lock);
			return -EINVAL;
		}
		param->ipipeif_param.decimation = IPIPEIF_DECIMATION_ON;
		param->ipipeif_param.rsz = ss_config->input.rsz;
		param->ipipeif_param.avg_filter =
		    (enum ipipeif_avg_filter)ss_config->input.avg_filter_en;
		param->ipipe_hsz =
		    (((ss_config->input.image_width * IPIPEIF_RSZ_CONST) /
		      ss_config->input.rsz) - 1);
	}
	param->ipipeif_param.gain = ss_config->input.gain;
	param->ipipeif_param.var.if_5_1.clip = ss_config->input.clip;
	param->ipipeif_param.var.if_5_1.dpc = ss_config->input.dpc;
	param->ipipe_colpat_olop = ss_config->input.colp_olop;
	param->ipipe_colpat_olep = ss_config->input.colp_olep;
	param->ipipe_colpat_elop = ss_config->input.colp_elop;
	param->ipipe_colpat_elep = ss_config->input.colp_elep;
	param->ipipe_vps = ss_config->input.vst;
	param->ipipe_hps = ss_config->input.hst;
	param->ipipe_vsz = ss_config->input.image_height - 1;
	if (ss_config->input.pix_fmt == IPIPE_UYVY)
		param->ipipeif_param.source = SDRAM_YUV;
	else
		param->ipipeif_param.source = SDRAM_RAW;

	configure_common_rsz_in_params(dev, param, 1, oper_state.rsz_chained,
					       &ss_config->input);

	param->rsz_common.passthrough = ss_config->bypass;
	/* update the resize parameters */
	if (ss_config->bypass == IPIPE_BYPASS_ON ||
	    param->ipipe_dpaths_fmt == IPIPE_RAW2RAW)
		/* Bypass resizer */
		configure_resize_passthru(param, 1);
	else {
		if (oper_state.rsz_chained) {
			if (param->rsz_en[RSZ_A]) {
				calculate_resize_ratios(param, RSZ_A);
				calculate_sdram_offsets(param, RSZ_A);
			}
			if (param->rsz_en[RSZ_B]) {
				calculate_resize_ratios(param, RSZ_B);
				calculate_sdram_offsets(param, RSZ_B);
			}
		} else {
			struct rsz_output_spec *output_specs =
				kmalloc(sizeof(struct rsz_output_spec),
					GFP_KERNEL);
			if (ISNULL(output_specs)) {
				dev_err(dev, "Memory Alloc failure\n");
				mutex_unlock(&oper_state.lock);
				return -EINVAL;
			}
				/* Using resizer as pass through */
			configure_resize_passthru(param, 0);
			memset((void *)output_specs, 0,
				sizeof(struct rsz_output_spec));
			output_specs->enable = 1;
			output_specs->pix_fmt = IPIPE_UYVY;
			output_specs->width = ss_config->input.image_width;
			output_specs->height = ss_config->input.image_height;
			output_specs->vst_y = ss_config->input.vst;
			configure_resizer_out_params(param, RSZ_A,
				output_specs, 0, 0);
			calculate_sdram_offsets(param, RSZ_A);
			kfree(output_specs);
		}
	}
	mutex_unlock(&oper_state.lock);
	return 0;
}

static int ipipe_reconfig_resizer(struct device *dev,
				struct rsz_reconfig *reconfig,
				void *config)
{
	struct ipipe_params *param = (struct ipipe_params *)config;
	if (ISNULL(reconfig)) {
		dev_err(dev, "Null User ptr received for reconfig");
		return -EINVAL;
	}

	if ((reconfig->pix_format != IMP_420SP_Y) &&
		(reconfig->pix_format != IMP_420SP_C)) {
		dev_err(dev, "reconfig - pixel format incorrect");
		return -EINVAL;
	}
	if (param->rsz_common.src_img_fmt != RSZ_IMG_420) {
		dev_err(dev, "reconfig - source format originally"
				"configured is not YUV420SP\n");
		return -EINVAL;
	}
	if ((param->rsz_common.y_c) && (reconfig->pix_format == IMP_420SP_C)) {
		dev_err(dev, "reconfig - channel is already configured"
				"for YUV420SP - C data\n");
		return -EINVAL;
	}
	if ((!param->rsz_common.y_c) && (reconfig->pix_format == IMP_420SP_Y)) {
		dev_err(dev, "reconfig - channel is already configured"
				"for YUV420SP - Y data\n");
		return -EINVAL;
	}
	if (reconfig->pix_format == IMP_420SP_Y)
		param->rsz_common.y_c = 0;
	else
		param->rsz_common.y_c = 1;
	if (!en_serializer)
		rsz_set_in_pix_format(param->rsz_common.y_c);
	return 0;
}

#ifdef CONFIG_IMP_DEBUG
static void ipipe_dump_hw_config(void)
{
	printk(KERN_NOTICE "IPIPE Configuration context\n");
	printk(KERN_NOTICE
	       "**************************************************\n");
	if (oper_mode == IMP_MODE_SINGLE_SHOT)
		printk(KERN_NOTICE "Operation mode = SINGLE SHOT\n");
	else
		printk(KERN_NOTICE "Operation mode = CONTINUOUS\n");
	if (oper_state.rsz_chained)
		printk(KERN_NOTICE "Resizer chained\n");
	else
		printk(KERN_NOTICE "Resizer not chained\n");
	if (en_serializer)
		printk(KERN_NOTICE "Serializer is enabled\n");
	else
		printk(KERN_NOTICE "Serializer is disabled\n");
	if (oper_mode != IMP_MODE_SINGLE_SHOT) {
		if (oper_state.prev_config_state)
			printk(KERN_NOTICE "Previewer is configured\n");
		else
			printk(KERN_NOTICE "Previewer is not configured\n");
	}
	if (oper_state.rsz_chained || (oper_mode != IMP_MODE_SINGLE_SHOT)) {
		if (oper_state.rsz_config_state)
			printk(KERN_NOTICE "Resizer is configured\n");
		else
			printk(KERN_NOTICE "Resizer is not configured\n");
	}
	ipipe_hw_dump_config();
}
#else
static void ipipe_dump_hw_config(void)
{
	return ipipe_hw_dump_config();
}
#endif

static int ipipe_set_preview_config(struct device *dev,
				    unsigned int oper_mode,
				    void *user_config, void *config)
{
	int ret = 0;
	struct ipipe_params *param = (struct ipipe_params *)config;
	dev_dbg(dev, "ipipe_set_preview_config\n");

	if ((ISNULL(user_config)) || (ISNULL(config))) {
		dev_err(dev, "Invalid user_config or config ptr\n");
		return -EINVAL;
	}

	if (!oper_state.rsz_chained) {
		/* For chained resizer, defaults are set by resizer */
		memcpy((void *)config,
		       (void *)&dm365_ipipe_defs,
		       sizeof(struct ipipe_params));
	}

	/* shared block */
	if (oper_mode == IMP_MODE_SINGLE_SHOT) {
		/* previewer in standalone mode. In this mode if serializer
		 * is enabled, we need to set config params for hw.
		 */
		ret = configure_previewer_in_ss_mode(dev, user_config, param);

		if ((!ret) && !en_serializer)
			ret = ipipe_hw_setup(config);
	} else
		ret = configure_previewer_in_cont_mode(dev, user_config, param);
	/* continuous mode */
	return ret;
}
struct imp_hw_interface *imp_get_hw_if(void)
{
	return &dm365_ipipe_interface;
}
EXPORT_SYMBOL(imp_get_hw_if);

/* APIs for CCDC driver */
static int ipipe_set_input_win(struct imp_window *win)
{
	int ret;
	struct ipipe_params *param = oper_state.shared_config_param;

	ret = mutex_lock_interruptible(&oper_state.lock);
	if (ret)
		return ret;
	if (param->ipipeif_param.decimation) {
		param->ipipe_hsz =
		    ((win->width * IPIPEIF_RSZ_CONST) /
		     param->ipipeif_param.rsz) - 1;
	} else
		param->ipipe_hsz = win->width - 1;
	if (!oper_state.frame_format) {
		param->ipipe_vsz = (win->height >> 1) - 1;
		param->ipipe_vps = (win->vst >> 1);
	} else {
		param->ipipe_vsz = win->height - 1;
		param->ipipe_vps = win->vst;
	}
	param->ipipe_hps = win->hst;
	param->rsz_common.vsz = param->ipipe_vsz;
	param->rsz_common.hsz = param->ipipe_hsz;
	mutex_unlock(&oper_state.lock);
	return 0;
}
static int ipipe_get_input_win(struct imp_window *win)
{
	int ret;
	struct ipipe_params *param = oper_state.shared_config_param;

	ret = mutex_lock_interruptible(&oper_state.lock);
	if (ret)
		return ret;
	if (param->ipipeif_param.decimation)
		win->width =
		    (((param->ipipe_hsz + 1) * param->ipipeif_param.rsz) >> 4);
	else
		win->width = param->ipipe_hsz + 1;
	if (!oper_state.frame_format) {
		win->height = (param->ipipe_vsz + 1) << 1;
		win->vst = (param->ipipe_vps << 1);
	} else {
		win->height = param->ipipe_vsz + 1;
		win->vst = param->ipipe_vps;
	}
	win->hst = param->ipipe_hps;
	mutex_unlock(&oper_state.lock);
	return 0;
}

static int ipipe_set_in_pixel_format(enum imp_pix_formats pix_fmt)
{
	int ret;
	struct ipipe_params *param = oper_state.shared_config_param;

	ret = mutex_lock_interruptible(&oper_state.lock);
	if (ret)
		return ret;
	oper_state.in_pixel_format = pix_fmt;
	param->rsz_common.src_img_fmt = RSZ_IMG_422;
	mutex_unlock(&oper_state.lock);
	return 0;
}

static int ipipe_set_out_pixel_format(enum imp_pix_formats pix_fmt)
{
	struct ipipe_params *param = oper_state.shared_config_param;
	int err;

	/* if image is RAW, preserve raw image format while flipping.
	 * otherwise preserve, preserve ycbcr format while flipping
	 */
	if (pix_fmt == IMP_BAYER)
		param->rsz_common.raw_flip = 1;
	else
		param->rsz_common.raw_flip = 0;

	err = mutex_lock_interruptible(&oper_state.lock);
	if (err)
		return err;
	oper_state.out_pixel_format = pix_fmt;
	err = ipipe_process_pix_fmts(oper_state.in_pixel_format,
				     oper_state.out_pixel_format,
				     param);

	mutex_unlock(&oper_state.lock);
	return err;
}

static int ipipe_set_buftype(unsigned char buf_type)
{
	int ret;

	ret = mutex_lock_interruptible(&oper_state.lock);
	if (ret)
		return ret;
	oper_state.buffer_type = buf_type;
	mutex_unlock(&oper_state.lock);
	return 0;
}

static int ipipe_set_frame_format(unsigned char frm_fmt)
{
	int ret;

	ret = mutex_lock_interruptible(&oper_state.lock);
	if (ret)
		return ret;
	oper_state.frame_format = frm_fmt;
	mutex_unlock(&oper_state.lock);
	return 0;
}

static int ipipe_set_output_win(struct imp_window *win)
{
	struct ipipe_params *param = oper_state.shared_config_param;
	struct rsz_output_spec output_specs;
	int ret = -1, line_len, line_len_c;
	if (!param->rsz_en[0]) {
		printk(KERN_ERR "resizer output1 not enabled\n");
		return ret;
	}
	output_specs.enable = 1;
	output_specs.width = win->width;
	/* Always set output height same as in height
	   for de-interlacing
	 */
	output_specs.height = win->height;
	output_specs.vst_y = win->vst;
	if (oper_state.out_pixel_format == IPIPE_YUV420SP)
		output_specs.vst_c = win->vst;
	ret = mutex_lock_interruptible(&oper_state.lock);
	if (ret)
		return ret;
	ret = configure_resizer_out_params(param, RSZ_A, &output_specs, 0, 0);
	if (ret) {
		printk(KERN_ERR "error in configuring output1\n");
		mutex_unlock(&oper_state.lock);
		return ret;
	}
	ret |= calculate_line_length(oper_state.out_pixel_format,
				     param->rsz_rsc_param[0].o_hsz + 1,
				     param->rsz_rsc_param[0].o_vsz + 1,
				     &line_len,
				     &line_len_c);
	if (ret) {
		printk(KERN_ERR "error in calculating line length\n");
		mutex_unlock(&oper_state.lock);
		return ret;
	}
	param->ext_mem_param[0].rsz_sdr_oft_y = line_len;
	param->ext_mem_param[0].rsz_sdr_oft_c = line_len_c;
	calculate_resize_ratios(param, RSZ_A);
	if (param->rsz_en[RSZ_B])
		calculate_resize_ratios(param, RSZ_B);
	if (oper_state.out_pixel_format == IPIPE_YUV420SP)
		enable_422_420_conversion(param, RSZ_A, ENABLE);
	else
		enable_422_420_conversion(param, RSZ_A, DISABLE);

	ret = calculate_sdram_offsets(param, RSZ_A);
	if (param->rsz_en[RSZ_B])
		ret = calculate_sdram_offsets(param, RSZ_B);

	if (ret)
		printk(KERN_ERR "error in calculating sdram offsets\n");
	mutex_unlock(&oper_state.lock);
	return ret;
}
static int ipipe_get_output_state(unsigned char out_sel)
{
	struct ipipe_params *param = oper_state.shared_config_param;
	if ((out_sel != RSZ_A) && (out_sel != RSZ_B))
		return 0;
	return param->rsz_en[out_sel];
}

/* This should be called only after setting the output
 * window params. This also assumes the corresponding
 * output is configured prior to calling this.
 */
static int ipipe_get_line_length(unsigned char out_sel)
{
	struct ipipe_params *param = oper_state.shared_config_param;
	if ((out_sel != RSZ_A) && (out_sel != RSZ_B))
		return -1;
	/* assume output is always UYVY. Change this if we
	 * support RGB
	 */
	if (!param->rsz_en[out_sel])
		return -1;

	return param->ext_mem_param[out_sel].rsz_sdr_oft_y;
}

static int ipipe_get_image_height(unsigned char out_sel)
{
	struct ipipe_params *param = oper_state.shared_config_param;
	if ((out_sel != RSZ_A) && (out_sel != RSZ_B))
		return -1;
	if (!param->rsz_en[out_sel])
		return -1;

	return param->rsz_rsc_param[out_sel].o_vsz + 1;
}

/* Assume valid param ptr */
int ipipe_set_hw_if_param(struct vpfe_hw_if_param *if_param)
{
	int ret;
	struct ipipe_params *param = oper_state.shared_config_param;

	ret = mutex_lock_interruptible(&oper_state.lock);
	if (ret)
		return ret;
	param->ipipeif_param.var.if_5_1.isif_port = *if_param;
	mutex_unlock(&oper_state.lock);
	return 0;
}

static int dm365_ipipe_init(void)
{
	oper_state.shared_config_param =
	    kmalloc(sizeof(struct ipipe_params), GFP_KERNEL);
	if (ISNULL(oper_state.shared_config_param)) {
		printk(KERN_ERR
		       "dm365_ipipe_init: failed to allocate memory\n");
		return -ENOMEM;
	}
	memcpy(&dm365_ipipe_defs.ipipeif_param.var.if_5_1,
		&ipipeif_5_1_defaults,
		sizeof(struct ipipeif_5_1));
	lutdpc.table = ipipe_lutdpc_table;
	lut_3d.table = ipipe_3d_lut_table;
	gbce.table = ipipe_gbce_table;
	gamma.table_r = ipipe_gamma_table_r;
	gamma.table_b = ipipe_gamma_table_b;
	gamma.table_g = ipipe_gamma_table_g;
	yee.table = ipipe_yee_table;
	mutex_init(&oper_state.lock);
	oper_state.state = CHANNEL_FREE;
	oper_state.prev_config_state = STATE_NOT_CONFIGURED;
	oper_state.rsz_config_state = STATE_NOT_CONFIGURED;
	oper_state.frame_format = 1;
	oper_state.in_pixel_format = IMP_BAYER;
	oper_state.out_pixel_format = IMP_UYVY;
	if (oper_mode == IMP_MODE_SINGLE_SHOT)
		printk(KERN_NOTICE
		       "DM365 IPIPE initialized in Single Shot mode\n");
	else
		printk(KERN_NOTICE
		       "DM365 IPIPE initialized in Continuous mode\n");
	return 0;
}

static void dm365_ipipe_cleanup(void)
{
	kfree(oper_state.shared_config_param);
	printk(KERN_NOTICE "DM365 IPIPE hardware module exited\n");
}

subsys_initcall(dm365_ipipe_init);
module_exit(dm365_ipipe_cleanup);
MODULE_LICENSE("GPL");
