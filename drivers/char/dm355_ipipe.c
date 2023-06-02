/*
 * Copyright (C) 2005-2008 Texas Instruments Inc
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
#include <media/davinci/dm355_ipipe.h>
#include <media/davinci/imp_hw_if.h>
#include <media/davinci/vpss.h>

#include <mach/irqs.h>

#include "dm355_ipipe_hw.h"
#include "dm355_def_para.h"

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
	 * scan
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
/* enable or disable serializer */
static u32 en_serializer;
module_param(en_serializer, uint, S_IRUGO);

/* ipipe module operation state & configuration */
static struct ipipe_oper_state oper_state;

/* Pre-filter data */
static struct prev_prefilter pf;
static int validate_pf_params(struct device *dev);
static int set_pf_params(struct device *dev, void *param, int len);
static int get_pf_params(struct device *dev, void *param, int len);

/* Defect pixel correction data */
static struct prev_dfc dfc;
static int validate_dfc_params(struct device *dev);
static int set_dfc_params(struct device *dev, void *param, int len);
static int get_dfc_params(struct device *dev, void *param, int len);

/* Noise filter */
static struct prev_nf nf;
static int validate_nf_params(struct device *dev);
static int set_nf_params(struct device *dev, void *param, int len);
static int get_nf_params(struct device *dev, void *param, int len);

/* White Balance */
static struct prev_wb wb;
static int validate_wb_params(struct device *dev);
static int set_wb_params(struct device *dev, void *param, int len);
static int get_wb_params(struct device *dev, void *param, int len);

/* RGB2RGB conversion */
static struct prev_rgb2rgb rgb2rgb;
static int validate_rgb2rgb_params(struct device *dev);
static int set_rgb2rgb_params(struct device *dev, void *param, int len);
static int get_rgb2rgb_params(struct device *dev, void *param, int len);

/* Gamma correction */
static struct prev_gamma gamma;
static int validate_gamma_params(struct device *dev);
static int set_gamma_params(struct device *dev, void *param, int len);
static int get_gamma_params(struct device *dev, void *param, int len);

/* RGB2YUV conversion */
static struct prev_rgb2yuv rgb2yuv;
static int validate_rgb2yuv_params(struct device *dev);
static int set_rgb2yuv_params(struct device *dev, void *param, int len);
static int get_rgb2yuv_params(struct device *dev, void *param, int len);

/* Lumina Adjustment */
static struct prev_lum_adj lum_adj;
static int validate_lum_adj_params(struct device *dev);
static int set_lum_adj_params(struct device *dev, void *param, int len);
static int get_lum_adj_params(struct device *dev, void *param, int len);

/* YUV 422 conversion */
static struct prev_yuv422_conv yuv422_conv;
static int validate_yuv422_conv_params(struct device *dev);
static int set_yuv422_conv_params(struct device *dev, void *param, int len);
static int get_yuv422_conv_params(struct device *dev, void *param, int len);

/* Edge Enhancement */
static struct prev_yee yee;
static int validate_yee_params(struct device *dev);
static int set_yee_params(struct device *dev, void *param, int len);
static int get_yee_params(struct device *dev, void *param, int len);

/* False Color Suppression */
static struct prev_fcs fcs;
static int validate_fcs_params(struct device *dev);
static int set_fcs_params(struct device *dev, void *param, int len);
static int get_fcs_params(struct device *dev, void *param, int len);

/* Tables for various tuning modules */
struct ipipe_dfc_entry ipipe_dfc_table[MAX_SIZE_DFC];
struct ipipe_gamma_entry ipipe_gamma_table_r[MAX_SIZE_GAMMA];
struct ipipe_gamma_entry ipipe_gamma_table_b[MAX_SIZE_GAMMA];
struct ipipe_gamma_entry ipipe_gamma_table_g[MAX_SIZE_GAMMA];
struct ipipe_gamma_entry ipipe_gamma_table_all[MAX_SIZE_GAMMA];
short ipipe_yee_table[MAX_SIZE_EEC];

static struct prev_module_if prev_modules[PREV_MAX_MODULES] = {
	{
		.version = "3.1",
		.module_id = PREV_PRE_FILTER,
		.module_name = "Pre-filter",
		.control = 0,
		.path = (IMP_RAW2RAW | IMP_RAW2YUV),
		.set = set_pf_params,
		.get = get_pf_params
	},
	{
		.version = "3.1",
		.module_id = PREV_DFC,
		.module_name = "Defect correction",
		.control = 0,
		.path = IMP_RAW2RAW | IMP_RAW2YUV,
		.set = set_dfc_params,
		.get = get_dfc_params
	},
	{
		.version = "3.1",
		.module_id = PREV_NF,
		.module_name = "2-D Noise filter",
		.control = 0,
		.path = IMP_RAW2RAW | IMP_RAW2YUV,
		.set = set_nf_params,
		.get = get_nf_params
	},
	{
		.version = "3.1",
		.module_id = PREV_WB,
		.module_name = "White balance",
		.control = 0,
		.path = IMP_RAW2RAW | IMP_RAW2YUV,
		.set = set_wb_params,
		.get = get_wb_params
	},
	{
		.version = "3.1",
		.module_id = PREV_RGB2RGB,
		.module_name = "RGB-RGB Conversion",
		.control = 0,
		.path = IMP_RAW2YUV,
		.set = set_rgb2rgb_params,
		.get = get_rgb2rgb_params
	},
	{
		.version = "3.1",
		.module_id = PREV_GAMMA,
		.module_name = "Gamma Correction",
		.control = 0,
		.path = IMP_RAW2YUV,
		.set = set_gamma_params,
		.get = get_gamma_params
	},
	{
		.version = "3.1",
		.module_id = PREV_RGB2YUV,
		.module_name = "RGB-YCbCr conversion",
		.control = 0,
		.path = IMP_RAW2YUV,
		.set = set_rgb2yuv_params,
		.get = get_rgb2yuv_params
	},
	{
		.version = "3.1",
		.module_id = PREV_LUM_ADJ,
		.module_name = "Luminance Adjustment",
		.control = 1,
		.path = IMP_RAW2YUV,
		.set = set_lum_adj_params,
		.get = get_lum_adj_params
	},
	{
		.version = "3.1",
		.module_id = PREV_YUV422_CONV,
		.module_name = "YUV 422 conversion",
		.control = 0,
		.path = IMP_RAW2YUV,
		.set = set_yuv422_conv_params,
		.get = get_yuv422_conv_params
	},
	{
		.version = "3.1",
		.module_id = PREV_YEE,
		.module_name = "Edge Enhancer",
		.control = 1,
		.path = IMP_YUV2YUV,
		.set = set_yee_params,
		.get = get_yee_params
	},
	{
		.version = "3.1",
		.module_id = PREV_FCS,
		.module_name = "False Color Suppression",
		.control = 1,
		.path = IMP_YUV2YUV,
		.set = set_fcs_params,
		.get = get_fcs_params
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
#define IPIPE_MAX_OUTPUT_WIDTH_A	1344
#define IPIPE_MAX_OUTPUT_WIDTH_B	640

/* Based on max resolution supported. QXGA */
#define IPIPE_MAX_OUTPUT_HEIGHT_A	1536
/* Based on max resolution supported. VGA */
#define IPIPE_MAX_OUTPUT_HEIGHT_B	480
#define RSZ_A	0
#define RSZ_B	1

/* Raw YUV formats */
static u32 ipipe_raw_yuv_pix_formats[] =
                {V4L2_PIX_FMT_UYVY};


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

static struct imp_hw_interface dm355_ipipe_interface = {
	.name = "DM355 IPIPE",
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
	.dealloc_config_block = ipipe_dealloc_user_config_block,
	.set_user_config_defaults = ipipe_set_user_config_defaults,
	.set_preview_config = ipipe_set_preview_config,
	.set_resizer_config = ipipe_set_resize_config,
	.update_inbuf_address = ipipe_set_ipipe_if_address,
	.update_outbuf1_address = ipipe_update_outbuf1_address,
	.update_outbuf2_address = ipipe_update_outbuf2_address,
	.enable = ipipe_enable,
	.hw_setup = ipipe_do_hw_setup,
	.get_preview_irq = ipipe_get_irq,
	.get_rsz_irq = ipipe_get_irq,
	.get_resizer_config_state = ipipe_get_rsz_config_state,
	.get_previewer_config_state = ipipe_get_prev_config_state,
	/* Below used by CCDC driver to set input and output params */
	.set_input_win = ipipe_set_input_win,
	.get_input_win = ipipe_get_input_win,
	.set_in_pixel_format = ipipe_set_in_pixel_format,
	.set_out_pixel_format = ipipe_set_out_pixel_format,
	.set_buftype = ipipe_set_buftype,
	.set_frame_format = ipipe_set_frame_format,
	.set_output_win = ipipe_set_output_win,
	.get_output_state = ipipe_get_output_state,
	.get_line_length = ipipe_get_line_length,
	.get_image_height = ipipe_get_image_height,
	.get_max_output_width = ipipe_get_max_output_width,
	.get_max_output_height = ipipe_get_max_output_height,
	.enum_pix = ipipe_enum_pix,
	/* debug function */
	.dump_hw_config = ipipe_dump_hw_config,
};

static int ipipe_set_ipipe_if_address(void *config, unsigned int address)
{
	return (ipipe_set_ipipeif_address
		((struct ipipe_params *)config, address));
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
	char packed = 0, a_law = 0;

	if (in_pix_fmt == IPIPE_BAYER_8BIT_PACK) {
		temp_pix_fmt = IPIPE_BAYER;
		packed = 1;
	} else if (in_pix_fmt == IPIPE_BAYER_8BIT_PACK_ALAW) {
		packed = 1;
		a_law = 1;
		temp_pix_fmt = IPIPE_BAYER;
	} else if (in_pix_fmt == IPIPE_BAYER)
		temp_pix_fmt = IPIPE_BAYER;
	else if (in_pix_fmt == IPIPE_UYVY)
		temp_pix_fmt = IPIPE_UYVY;
	else
		return -1;

	if (temp_pix_fmt == IPIPE_BAYER)
		if (out_pix_fmt == IPIPE_BAYER)
			param->ipipe_dpaths_fmt = IPIPE_RAW2RAW;
		else if (out_pix_fmt == IPIPE_UYVY)
			param->ipipe_dpaths_fmt = IPIPE_RAW2YUV;
		else
			return -1;
	else if ((temp_pix_fmt == IPIPE_UYVY) && (out_pix_fmt == IPIPE_UYVY))
		param->ipipe_dpaths_fmt = IPIPE_YUV2YUV;
	else
		return -1;
	if (packed)
		param->ipipeif_param.pack_mode = EIGHT_BIT;
	if (a_law)
		param->ipipeif_param.ialaw = ALAW_ON;
	return 0;
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
		param = oper_state.shared_config_param;
		/* continuous mode setup */
		ipipe_process_pix_fmts(oper_state.in_pixel_format,
				       oper_state.out_pixel_format,
				       param);
		param->rsz_rsc_param[0].rsz_h_dif =
		    ((param->ipipe_hsz + 1) * 256) /
		    (param->rsz_rsc_param[0].rsz_o_hsz + 1);
		param->rsz_rsc_param[0].rsz_v_dif =
		    ((param->ipipe_vsz + 1) * 256) /
		    (param->rsz_rsc_param[0].rsz_o_vsz + 1);
	}
	ret = ipipe_hw_setup(param);
	mutex_unlock(&oper_state.lock);
	return ret;
}
static void ipipe_get_irq(struct irq_numbers *irq)
{
	irq->sdram = IRQ_DM355_IPIPE_SDR;
	irq->update = -1;
}

static unsigned int ipipe_rsz_chain_state(void)
{
	return oper_state.rsz_chained;
}

#define CHECKRANGE(val, val1, val2) \
	{ if (val < val1 || val > val2) \
		return -1; }

#define CHECKMORE(val, val1) \
	{ if (val > val1) \
		 return -1; }

static int validate_pf_params(struct device *dev)
{
	CHECKMORE((pf.en), 1);
	if (pf.en) {
		CHECKMORE((pf.en_adapt_prefilter), 1);
		CHECKMORE((pf.en_adapt_dotred), 1);
		CHECKMORE((pf.pre_gain), 255);
		CHECKMORE((pf.pre_shf), 15);
		CHECKMORE((pf.pre_thr_g), 0x3fff);
		CHECKMORE((pf.pre_thr_b), 0x3fff);
		CHECKMORE((pf.pre_thr_1), 0x3fff);
	}
	return 0;
}

static int ipipe_update_outbuf1_address(void *config, unsigned int address)
{
	if ((ISNULL(config)) && (oper_mode == IMP_MODE_CONTINUOUS))
		return (ipipe_set_resizer_address
			(oper_state.shared_config_param, 0, address));
	return (ipipe_set_resizer_address
		((struct ipipe_params *)config, 0, address));
}

static int ipipe_update_outbuf2_address(void *config, unsigned int address)
{
	if ((ISNULL(config)) && (oper_mode == IMP_MODE_CONTINUOUS))
		return (ipipe_set_resizer_address
			(oper_state.shared_config_param, 1, address));
	return (ipipe_set_resizer_address
		((struct ipipe_params *)config, 1, address));
}

static void ipipe_enable(unsigned char en, void *config)
{
	unsigned char val = 0;

	if (en)
		val = 1;
	if (en)
		regw_ip(0xff, IRQ_EN);
	else
		regw_ip(0x0, IRQ_EN);

	if (oper_mode == IMP_MODE_CONTINUOUS) {
		vpss_enable_clock(VPSS_IPIPE_CLOCK, 1);
		regw_if(val, IPIPEIF_ENABLE);
		if (oper_state.shared_config_param->rsz_en[0])
			rsz_enable(0, en);
		if (oper_state.shared_config_param->rsz_en[1])
			rsz_enable(1, en);
		regw_ip(val, IPIPE_EN);
	} else {
		while (regr_ip(IPIPE_EN));
		regw_ip(val, IPIPE_EN);

		while (regr_if(IPIPEIF_ENABLE));
		regw_if(val, IPIPEIF_ENABLE);
	}
}
static int set_pf_params(struct device *dev, void *param, int len)
{
	struct prev_prefilter *pf_param = (struct prev_prefilter *)param;

	if (ISNULL(pf_param))
		/* Copy defaults for pf */
		memcpy((void *)&pf,
			(void *)&dm355_pf_defaults,
			sizeof(struct prev_prefilter));
	else {
		if (copy_from_user(&pf,
			(struct prev_prefilter *)pf_param,
			sizeof(struct prev_prefilter))) {
			dev_err(dev,
				"set_pf_params: Error in copy to kernel\n");
			return -EFAULT;
		}
		if (validate_pf_params(dev) < 0)
			return -EINVAL;
	}

	/* Now set the values in the hw */
	return (ipipe_set_pf_regs(&pf));
}
static int get_pf_params(struct device *dev, void *param, int len)
{
	struct prev_prefilter *pf_param = (struct prev_prefilter *)param;

	if (copy_to_user((struct prev_prefilter *)pf_param,
		&pf,
		sizeof(struct prev_prefilter))) {
		dev_err(dev, "get_pf_params: Error in copy from kernel\n");
		return -EFAULT;
	}
	return 0;
}

static int validate_dfc_params(struct device *dev)
{
	int i;

	CHECKMORE(dfc.en, 1);
	if (dfc.en) {
		CHECKRANGE((dfc.dfc_size), 1, MAX_SIZE_DFC);
		if ((dfc.en) && (ISNULL(dfc.table)))
			return -1;
		for (i = 0; i < dfc.dfc_size; i++) {
			CHECKMORE((dfc.table[i].horz_pos), 0xfff);
			CHECKMORE((dfc.table[i].vert_pos), 0xfff);
		}
	}
	return 0;
}

static int set_dfc_params(struct device *dev, void *param, int len)
{
	struct prev_dfc dfc_param;
	struct ipipe_dfc_entry *temp;

	if (ISNULL(param)) {
		/* Copy defaults for dfc */
		temp = dfc.table;
		memcpy((void *)&dfc,
			(void *)&dm355_dfc_defaults,
			sizeof(struct prev_dfc));
		dfc.table = temp;
	} else {
		if (copy_from_user(&dfc_param,
			(struct prev_dfc *)param,
			sizeof(struct prev_dfc))) {
			dev_err(dev,
				"set_dfc_params: Error in copy to kernel\n");
			return -EFAULT;
		}

		dfc.dfc_size = dfc_param.dfc_size;
		dfc.en = dfc_param.en;
		if (dfc.en) {
			dfc.vert_copy_method = dfc_param.vert_copy_method;
			if (copy_from_user(dfc.table,
				(struct ipipe_dfc_entry *)dfc_param.table,
				(dfc.dfc_size *
				sizeof(struct ipipe_dfc_entry)))) {
				dev_err(dev,
					"set_dfc_params: Error in copying "
					"dfc table to kernel\n");
				return -EFAULT;
			}
			if (validate_dfc_params(dev) < 0)
				return -EINVAL;
		}
	}
	return (ipipe_set_dfc_regs(&dfc));
}

static int get_dfc_params(struct device *dev, void *param, int len)
{
	struct prev_dfc user_dfc;
	struct prev_dfc *dfc_param = (struct prev_dfc *)param;

	/* First copy to temp since we need to preserve the dfc table ptr */
	if (copy_from_user(&user_dfc,
		dfc_param, sizeof(struct prev_dfc))) {
		dev_err(dev, "get_dfc_params: Error in copy to kernel\n");
		return -EFAULT;
	}
	user_dfc.en = dfc.en;
	if (dfc.en) {
		user_dfc.vert_copy_method = dfc.vert_copy_method;
		user_dfc.dfc_size = dfc.dfc_size;
		if (ISNULL(user_dfc.table)) {
			dev_err(dev,
				"get_dfc_params:dfc table ptr is null."
				" Allocate for max entry\n");
			return -EINVAL;
		}
		if (copy_to_user(user_dfc.table,
			dfc.table,
			(user_dfc.dfc_size *
			sizeof(struct ipipe_dfc_entry)))) {
			dev_err(dev,
				"get_dfc_params: Error in copy dfc table"
				" to user\n");
			return -EFAULT;
		}
	}
	if (copy_to_user(dfc_param,
		&user_dfc,
		sizeof(struct prev_dfc))) {
		dev_err(dev, "get_dfc_params: Error in copy to user\n");
		return -EFAULT;
	}
	return 0;
}

static int validate_nf_params(struct device *dev)
{
	int i;

	CHECKMORE((nf.en), 1);
	if (nf.en) {
		CHECKMORE((nf.shft_val), 3);
		CHECKMORE((nf.spread_val), 3);
		for (i = 0; i < IPIPE_NF_THR_TABLE_SIZE; i++)
			CHECKMORE((nf.thr[i]), 0xfff);
		for (i = 0; i < IPIPE_NF_STR_TABLE_SIZE; i++)
			CHECKMORE((nf.str[i]), 31);
	}
	return 0;
}

static int set_nf_params(struct device *dev, void *param, int len)
{
	struct prev_nf *nf_param = (struct prev_nf *)param;

	if (ISNULL(nf_param)) {
		/* Copy defaults for nf */
		memcpy((void *)&nf,
		       (void *)&dm355_nf_defaults,
			sizeof(struct prev_nf));
		memset((void *)nf.thr, 0, IPIPE_NF_THR_TABLE_SIZE);
		memset((void *)nf.str, 0, IPIPE_NF_THR_TABLE_SIZE);
	} else {

		if (copy_from_user(&nf,
			nf_param,
			sizeof(struct prev_nf))) {
			dev_err(dev,
				"set_nf_params: Error in copy to kernel\n");
			return -EFAULT;
		}
		if (validate_nf_params(dev) < 0)
			return -EINVAL;
	}
	/* Now set the values in the hw */
	return (ipipe_set_d2f_nf_regs(&nf));
}
static int get_nf_params(struct device *dev, void *param, int len)
{
	struct prev_nf *nf_param = (struct prev_nf *)param;

	if (copy_to_user((struct prev_nf *)nf_param,
		&nf,
		sizeof(struct prev_nf))) {
		dev_err(dev, "get_nf_params: Error in copy from kernel\n");
		return -EFAULT;
	}
	return 0;
}

static int validate_wb_params(struct device *dev)
{
	/* U10Q8 */
	CHECKMORE((wb.dgn.integer), 3);
	CHECKMORE((wb.dgn.decimal), 255);
	/* U10Q7 */
	CHECKMORE((wb.gain_r.integer), 7);
	CHECKMORE((wb.gain_r.decimal), 127);
	CHECKMORE((wb.gain_gr.integer), 7);
	CHECKMORE((wb.gain_gr.decimal), 127);
	CHECKMORE((wb.gain_gb.integer), 7);
	CHECKMORE((wb.gain_gb.decimal), 127);
	CHECKMORE((wb.gain_b.integer), 7);
	CHECKMORE((wb.gain_b.decimal), 127);
	return 0;
}
static int set_wb_params(struct device *dev, void *param, int len)
{
	struct prev_wb *wb_param = (struct prev_wb *)param;

	dev_dbg(dev, "set_wb_params");
	if (ISNULL(wb_param))
		/* Copy defaults for wb */
		memcpy((void *)&wb,
			(void *)&dm355_wb_defaults,
			sizeof(struct prev_wb));
	else {
		if (copy_from_user(&wb,
			wb_param,
			sizeof(struct prev_wb))) {
			dev_err(dev,
				"set_wb_params: Error in copy to kernel\n");
			return -EFAULT;
		}
		if (validate_wb_params(dev) < 0)
			return -EINVAL;
	}

	/* Now set the values in the hw */
	return (ipipe_set_wb_regs(&wb));
}
static int get_wb_params(struct device *dev, void *param, int len)
{
	struct prev_wb *wb_param = (struct prev_wb *)param;

	dev_dbg(dev, "get_wb_params\n");
	if (copy_to_user((struct prev_wb *)wb_param,
		&wb,
		sizeof(struct prev_wb))) {
		dev_err(dev, "get_wb_params: Error in copy from kernel\n");
		return -EFAULT;
	}
	return 0;
}

static int validate_rgb2rgb_params(struct device *dev)
{
	CHECKMORE((rgb2rgb.coef_rr.decimal), 0xff);
	CHECKMORE((rgb2rgb.coef_rr.integer), 0xf);
	CHECKMORE((rgb2rgb.coef_gr.decimal), 0xff);
	CHECKMORE((rgb2rgb.coef_gr.integer), 0xf);
	CHECKMORE((rgb2rgb.coef_br.decimal), 0xff);
	CHECKMORE((rgb2rgb.coef_br.integer), 0xf);
	CHECKMORE((rgb2rgb.coef_rg.decimal), 0xff);
	CHECKMORE((rgb2rgb.coef_rg.integer), 0xf);
	CHECKMORE((rgb2rgb.coef_gg.decimal), 0xff);
	CHECKMORE((rgb2rgb.coef_gg.integer), 0xf);
	CHECKMORE((rgb2rgb.coef_bg.decimal), 0xff);
	CHECKMORE((rgb2rgb.coef_bg.integer), 0xf);
	CHECKMORE((rgb2rgb.coef_rb.decimal), 0xff);
	CHECKMORE((rgb2rgb.coef_rb.integer), 0xf);
	CHECKMORE((rgb2rgb.coef_gb.decimal), 0xff);
	CHECKMORE((rgb2rgb.coef_gb.integer), 0xf);
	CHECKMORE((rgb2rgb.coef_bb.decimal), 0xff);
	CHECKMORE((rgb2rgb.coef_bb.integer), 0xf);
	/* S14Q0 */
	CHECKMORE((rgb2rgb.out_ofst_r), 0x3fff);
	CHECKMORE((rgb2rgb.out_ofst_g), 0x3fff);
	CHECKMORE((rgb2rgb.out_ofst_b), 0x3fff);
	return 0;
}
static int set_rgb2rgb_params(struct device *dev, void *param, int len)
{
	struct prev_rgb2rgb *rgb2rgb_param = (struct prev_rgb2rgb *)param;

	if (ISNULL(rgb2rgb_param))
		/* Copy defaults for rgb2rgb conversion */
		memcpy((void *)&rgb2rgb,
		       (void *)&dm355_rgb2rgb_defaults,
		       sizeof(struct prev_rgb2rgb));
	else {
		if (copy_from_user(&rgb2rgb,
			rgb2rgb_param,
			sizeof(struct prev_rgb2rgb))) {
			dev_err(dev,
				"set_rgb2rgb_params: Error in "
				"copy to kernel\n");
			return -EFAULT;
		}
		if (validate_rgb2rgb_params(dev) < 0)
			return -EINVAL;
	}
	return (ipipe_set_rgb2rgb_regs(&rgb2rgb));
}
static int get_rgb2rgb_params(struct device *dev, void *param, int len)
{
	struct prev_rgb2rgb *rgb2rgb_param = (struct prev_rgb2rgb *)param;
	if (copy_to_user((struct prev_rgb2rgb *)rgb2rgb_param,
		&rgb2rgb,
		sizeof(struct prev_rgb2rgb))) {
		dev_err(dev, "get_rgb2rgb_params: Error in copy to user\n");
		return -EFAULT;
	}
	return 0;
}

static int validate_gamma_params(struct device *dev)
{
	int table_size = 0, i;

	CHECKMORE((gamma.bypass_r), 1);
	CHECKMORE((gamma.bypass_b), 1);
	CHECKMORE((gamma.bypass_g), 1);
	if (gamma.tbl_sel == IPIPE_GAMMA_TBL_RAM) {
		if (gamma.tbl_size == IPIPE_GAMMA_TBL_SZ_128)
			table_size = 128;
		else if (gamma.tbl_size == IPIPE_GAMMA_TBL_SZ_256)
			table_size = 256;
		else if (gamma.tbl_size == IPIPE_GAMMA_TBL_SZ_512)
			table_size = 512;
		if (!gamma.bypass_r) {
			if (ISNULL(gamma.table_r))
				return -1;
			else {
				for (i = 0; i < table_size; i++) {
					CHECKMORE((gamma.table_r[i].
						    slope), 0x3ff);
					CHECKMORE((gamma.table_r[i].
						    offset), 0x3ff);
				}
			}
		}
		if (!gamma.bypass_b) {
			if (ISNULL(gamma.table_b))
				return -1;
			else {
				for (i = 0; i < table_size; i++) {
					CHECKMORE((gamma.table_b[i].
						    slope), 0x3ff);
					CHECKMORE((gamma.table_b[i].
						    offset), 0x3ff);
				}
			}
		}
		if (!gamma.bypass_g) {
			if (ISNULL(gamma.table_g))
				return -1;
			else {
				for (i = 0; i < table_size; i++) {
					CHECKMORE((gamma.table_g[i].
						    slope), 0x3ff);
					CHECKMORE((gamma.table_g[i].
						    offset), 0x3ff);
				}
			}
		}
		if (!(ISNULL(gamma.table_rgb_all))) {
			for (i = 0; i < table_size; i++) {
				CHECKMORE((gamma.table_g[i].slope),
						0x3ff);
				CHECKMORE((gamma.table_g[i].offset),
					   0x3ff);
			}
		}
	}
	return 0;
}
static int set_gamma_params(struct device *dev, void *param, int len)
{
	int table_size = 0;
	struct prev_gamma user_gamma;
	struct prev_gamma *gamma_param = (struct prev_gamma *)param;

	if (ISNULL(gamma_param)) {
		/* Copy defaults for gamma */
		gamma.bypass_r = dm355_gamma_defaults.bypass_r;
		gamma.bypass_g = dm355_gamma_defaults.bypass_g;
		gamma.bypass_b = dm355_gamma_defaults.bypass_b;
		gamma.tbl_sel = dm355_gamma_defaults.tbl_sel;
		gamma.tbl_size = dm355_gamma_defaults.tbl_size;
		/* By default, we bypass the gamma correction.
		   So no values by default for tables
		 */
	} else {
		if (copy_from_user(&user_gamma,
			gamma_param,
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
			if (user_gamma.tbl_size == IPIPE_GAMMA_TBL_SZ_128)
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
				if (copy_from_user(gamma.table_r,
					user_gamma.table_r,
					(table_size *
					sizeof(struct ipipe_gamma_entry)))) {
					dev_err(dev,
						"set_gamma_params: R-Error"
						" in copy to kernel\n");
					return -EFAULT;
				}
			}

			if (!user_gamma.bypass_b) {
				if (copy_from_user(gamma.table_b,
					user_gamma.table_b,
					(table_size *
					sizeof(struct ipipe_gamma_entry)))) {
					dev_err(dev,
						"set_gamma_params: B-Error"
						" in copy to kernel\n");
					return -EFAULT;
				}
			}

			if (!user_gamma.bypass_g) {
				if (copy_from_user(gamma.table_g,
					user_gamma.table_g,
					(table_size *
					sizeof(struct ipipe_gamma_entry)))) {
					dev_err(dev,
						"set_gamma_params: G-Error "
						"in copy to kernel\n");
					return -EFAULT;
				}
			}

			if ((!ISNULL(user_gamma.table_rgb_all))) {
				if (copy_from_user(gamma.table_rgb_all,
					user_gamma.table_rgb_all,
					(table_size *
					sizeof(struct ipipe_gamma_entry)))) {
					dev_err(dev,
						"set_gamma_params: All-Error"
						" in copy to kernel\n");
					return -EFAULT;
				}
			}
		}

		if (validate_gamma_params(dev) < 0)
			return -EINVAL;
	}
	return (ipipe_set_gamma_regs(&gamma));
}
static int get_gamma_params(struct device *dev, void *param, int len)
{
	int table_size = 0;
	struct prev_gamma user_gamma;
	struct prev_gamma *gamma_param = (struct prev_gamma *)param;

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

		if (ISNULL(user_gamma.table_rgb_all)) {
			dev_err(dev,
				"get_gamma_params: table ptr empty "
				"for RGB-all\n");
			return -EINVAL;
		} else {
			if (copy_from_user(user_gamma.table_rgb_all,
				gamma.table_rgb_all,
				(table_size *
				sizeof(struct ipipe_gamma_entry)))) {
				dev_err(dev,
					"set_gamma_params: RGB-all Table,"
					" copy error\n");
				return -EFAULT;
			}
		}
	}
	if (copy_to_user(gamma_param,
		&user_gamma,
		sizeof(struct prev_gamma))) {
		dev_err(dev, "get_dfc_params: Error in copy from kernel\n");
		return -EFAULT;
	}
	return 0;
}

static int validate_rgb2yuv_params(struct device *dev)
{
	/* S10Q8 */
	CHECKMORE((rgb2yuv.coef_ry.decimal), 255);
	CHECKMORE((rgb2yuv.coef_ry.integer), 3);
	CHECKMORE((rgb2yuv.coef_gy.decimal), 255);
	CHECKMORE((rgb2yuv.coef_gy.integer), 3);
	CHECKMORE((rgb2yuv.coef_by.decimal), 255);
	CHECKMORE((rgb2yuv.coef_by.integer), 3);
	CHECKMORE((rgb2yuv.coef_rcb.decimal), 255);
	CHECKMORE((rgb2yuv.coef_rcb.integer), 3);
	CHECKMORE((rgb2yuv.coef_gcb.decimal), 255);
	CHECKMORE((rgb2yuv.coef_gcb.integer), 3);
	CHECKMORE((rgb2yuv.coef_bcb.decimal), 255);
	CHECKMORE((rgb2yuv.coef_bcb.integer), 3);
	CHECKMORE((rgb2yuv.coef_rcr.decimal), 255);
	CHECKMORE((rgb2yuv.coef_rcr.integer), 3);
	CHECKMORE((rgb2yuv.coef_gcr.decimal), 255);
	CHECKMORE((rgb2yuv.coef_gcr.integer), 3);
	CHECKMORE((rgb2yuv.coef_bcr.decimal), 255);
	CHECKMORE((rgb2yuv.coef_bcr.integer), 3);
	/* S9Q0 */
	CHECKMORE((rgb2yuv.out_ofst_y), 0x1ff);
	CHECKMORE((rgb2yuv.out_ofst_cb), 0x1ff);
	CHECKMORE((rgb2yuv.out_ofst_cr), 0x1ff);
	return 0;
}
static int set_rgb2yuv_params(struct device *dev, void *param, int len)
{
	struct prev_rgb2yuv *rgb2yuv_param = (struct prev_rgb2yuv *)param;

	if (ISNULL(rgb2yuv_param))
		/* Copy defaults for rgb2yuv conversion  */
		memcpy((void *)&rgb2yuv,
		       (void *)&dm355_rgb2yuv_defaults,
		       sizeof(struct prev_rgb2yuv));
	else {
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
	return (ipipe_set_rgb2ycbcr_regs(&rgb2yuv));
}
static int get_rgb2yuv_params(struct device *dev, void *param, int len)
{
	struct prev_rgb2yuv *rgb2yuv_param = (struct prev_rgb2yuv *)param;

	if (copy_to_user((struct prev_rgb2yuv *)rgb2yuv_param,
		&rgb2yuv,
		sizeof(struct prev_rgb2yuv))) {
		dev_err(dev, "get_rgb2yuv_params: Error in"
				" copy from kernel\n");
		return -EFAULT;
	}
	return 0;
}

static int validate_lum_adj_params(struct device *dev)
{
	return 0;
}

static int set_lum_adj_params(struct device *dev, void *param, int len)
{
	struct prev_lum_adj *lum_adj_param = (struct prev_lum_adj *)param;

	if (ISNULL(lum_adj_param))
		/* Copy defaults for Luminance adjustments */
		memcpy((void *)&lum_adj,
		       (void *)&dm355_lum_adj_defaults,
		       sizeof(struct prev_lum_adj));
	else {
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
	return (ipipe_set_lum_adj_regs(&lum_adj));
}
static int get_lum_adj_params(struct device *dev, void *param, int len)
{
	struct prev_lum_adj *lum_adj_param = (struct prev_lum_adj *)param;

	if (copy_to_user(lum_adj_param,
		&lum_adj,
		sizeof(struct prev_lum_adj))) {
		dev_err(dev, "get_lum_adj_params: Error in copy"
				" from kernel\n");
		return -EFAULT;
	}
	return 0;
}

static int validate_yuv422_conv_params(struct device *dev)
{
	CHECKMORE((yuv422_conv.en_chrom_lpf), 1);
	return 0;
}

static int set_yuv422_conv_params(struct device *dev, void *param, int len)
{
	struct prev_yuv422_conv *yuv422_conv_param =
	    (struct prev_yuv422_conv *)param;

	if (ISNULL(yuv422_conv_param))
		/* Copy defaults for yuv 422 conversion */
		memcpy((void *)&yuv422_conv,
		       (void *)&dm355_yuv422_conv_defaults,
		       sizeof(struct prev_yuv422_conv));
	else {
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
	return (ipipe_set_yuv422_conv_regs(&yuv422_conv));
}
static int get_yuv422_conv_params(struct device *dev, void *param, int len)
{
	struct prev_yuv422_conv *yuv422_conv_param =
	    (struct prev_yuv422_conv *)param;

	if (copy_to_user(yuv422_conv_param,
		&yuv422_conv,
		sizeof(struct prev_yuv422_conv))) {
		dev_err(dev,
			"get_yuv422_conv_params: Error in copy"
			" from kernel\n");
		return -EFAULT;
	}
	return 0;
}

static int validate_yee_params(struct device *dev)
{
	CHECKMORE((yee.en), 1);
	if (yee.en) {
		CHECKMORE((yee.en_emf), 1);
		CHECKMORE((yee.hpf_shft), 15);
		CHECKMORE((yee.hpf_coef_00), 0x3ff);
		CHECKMORE((yee.hpf_coef_01), 0x3ff);
		CHECKMORE((yee.hpf_coef_02), 0x3ff);
		CHECKMORE((yee.hpf_coef_10), 0x3ff);
		CHECKMORE((yee.hpf_coef_11), 0x3ff);
		CHECKMORE((yee.hpf_coef_12), 0x3ff);
		CHECKMORE((yee.hpf_coef_20), 0x3ff);
		CHECKMORE((yee.hpf_coef_21), 0x3ff);
		CHECKMORE((yee.hpf_coef_22), 0x3ff);
		if (ISNULL(yee.table)) {
			dev_err(dev, "EE table ptr null\n");
			return -1;
		}
	}
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
		       (void *)&dm355_yee_defaults,
			sizeof(struct prev_yee));
		yee.table = temp_table;
	} else {
		if (copy_from_user(&user_yee,
			yee_param,
			sizeof(struct prev_yee))) {
			dev_err(dev,
				"set_yee_params: Error in copy from user\n");
			return -EFAULT;
		}
		yee.en = user_yee.en;
		if (user_yee.en) {
			yee.en_emf = user_yee.en_emf;
			yee.hpf_shft = user_yee.hpf_shft;
			yee.hpf_coef_00 = user_yee.hpf_coef_00;
			yee.hpf_coef_01 = user_yee.hpf_coef_01;
			yee.hpf_coef_02 = user_yee.hpf_coef_02;
			yee.hpf_coef_10 = user_yee.hpf_coef_10;
			yee.hpf_coef_11 = user_yee.hpf_coef_11;
			yee.hpf_coef_12 = user_yee.hpf_coef_12;
			yee.hpf_coef_20 = user_yee.hpf_coef_20;
			yee.hpf_coef_21 = user_yee.hpf_coef_21;
			yee.hpf_coef_22 = user_yee.hpf_coef_22;

			if (ISNULL(user_yee.table)) {
				dev_err(dev, "get_yee_params: yee table"
						" ptr null\n");
				return -EINVAL;
			}
			if (copy_from_user(yee.table,
				user_yee.table,
				(MAX_SIZE_EEC * sizeof(short)))) {
				dev_err(dev,
					"set_yee_params: Error in copy"
					" from user\n");
				return -EFAULT;
			}
		}
		if (validate_yee_params(dev) < 0)
			return -EINVAL;
	}
	return (ipipe_set_ee_regs(&yee));
}
static int get_yee_params(struct device *dev, void *param, int len)
{
	struct prev_yee *yee_param = (struct prev_yee *)param;
	struct prev_yee user_yee;

	if (copy_from_user(&user_yee, yee_param, sizeof(struct prev_yee))) {
		dev_err(dev, "get_yee_params: Error in copy to kernel\n");
		return -EFAULT;
	}
	if (ISNULL(user_yee.table)) {
		dev_err(dev, "get_yee_params: yee table ptr null\n");
		return -EINVAL;
	}
	if (copy_to_user(user_yee.table,
		yee.table,
		(MAX_SIZE_EEC * sizeof(short)))) {
		dev_err(dev, "get_yee_params: Error in copy from kernel\n");
		return -EFAULT;
	}
	user_yee.en = yee.en;
	user_yee.en_emf = yee.en_emf;
	user_yee.hpf_shft = yee.hpf_shft;
	user_yee.hpf_coef_00 = yee.hpf_coef_00;
	user_yee.hpf_coef_01 = yee.hpf_coef_01;
	user_yee.hpf_coef_02 = yee.hpf_coef_02;
	user_yee.hpf_coef_10 = yee.hpf_coef_10;
	user_yee.hpf_coef_11 = yee.hpf_coef_11;
	user_yee.hpf_coef_12 = yee.hpf_coef_12;
	user_yee.hpf_coef_20 = yee.hpf_coef_20;
	user_yee.hpf_coef_21 = yee.hpf_coef_21;
	user_yee.hpf_coef_22 = yee.hpf_coef_22;

	if (copy_to_user(yee_param,
		&user_yee,
		sizeof(struct prev_yee))) {
		dev_err(dev, "get_yee_params: Error in copy from kernel\n");
		return -EFAULT;
	}
	return 0;
}

static int validate_fcs_params(struct device *dev)
{
	CHECKMORE((fcs.en), 1);
	if (fcs.en) {
		CHECKMORE((fcs.hpf_shft_y), 3);
		CHECKMORE((fcs.gain_shft_c), 7);
		CHECKMORE((fcs.thr), 255);
		CHECKMORE((fcs.sgn), 255);
		CHECKMORE((fcs.lth), 256);
	}
	return 0;
}
static int set_fcs_params(struct device *dev, void *param, int len)
{
	struct prev_fcs *fcs_param = (struct prev_fcs *)param;

	if (ISNULL(fcs_param))
		/* Copy defaults for ns */
		memcpy((void *)&fcs,
		       (void *)&dm355_fcs_defaults,
			sizeof(struct prev_fcs));
	else {
		if (copy_from_user(&fcs,
			fcs_param,
			sizeof(struct prev_fcs))) {
			dev_err(dev,
				"set_fcs_params: Error in copy from user\n");
			return -EFAULT;
		}
		if (validate_fcs_params(dev) < 0)
			return -EINVAL;
	}
	return (ipipe_set_fcs_regs(&fcs));
}
static int get_fcs_params(struct device *dev, void *param, int len)
{
	struct prev_fcs *fcs_param = (struct prev_fcs *)param;

	if (copy_to_user(fcs_param,
		&fcs,
		sizeof(struct prev_fcs))) {
		dev_err(dev, "get_fcs_params: Error in copy from kernel\n");
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

	return (&prev_modules[index]);
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
	return oper_state.prev_config_state;
}

static unsigned int ipipe_get_rsz_config_state(void)
{
	return oper_state.rsz_config_state;
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
		config_block = kmalloc(sizeof(struct ipipe_params),
					GFP_KERNEL);
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
			       (void *)&dm355_prev_ss_config_defs,
			       sizeof(struct prev_single_shot_config));
		} else {
			dev_dbg(dev, "SS-Resize\n");
			/* resizer channel in single shot mode */
			memcpy(config,
			       (void *)&dm355_rsz_ss_config_defs,
			       sizeof(struct rsz_single_shot_config));
		}
	} else if (oper_mode == IMP_MODE_CONTINUOUS) {
		/* Continuous mode */
		if (chan_type == IMP_PREVIEWER) {
			dev_dbg(dev, "Cont Preview\n");
			/* previewer defaults */
			memcpy(config,
			       (void *)&dm355_prev_cont_config_defs,
			       sizeof(struct prev_continuous_config));
		} else {
			dev_dbg(dev, "Cont resize\n");
			/* resizer defaults */
			memcpy(config,
			       (void *)&dm355_rsz_cont_config_defs,
			       sizeof(struct rsz_continuous_config));
		}
	} else {
		dev_err(dev, "Incorrect mode used\n");
	}
}

static void ipipe_config_resize_out_param(struct ipipe_params *param,
					  int index,
					  struct rsz_ss_output_spec *output,
					  unsigned char flag)
{
	if (output->enable) {
		param->rsz_en[index] = ENABLE;
		param->rsz_rsc_param[index].rsz_o_vsz = output->height - 1;
		param->rsz_rsc_param[index].rsz_o_hsz = output->width - 1;
		param->rsz_rsc_param[index].rsz_o_hst = output->hst;
		if (flag) {
			param->rsz_rsc_param[index].rsz_h_typ =
			    output->h_intp_type;
			param->rsz_rsc_param[index].rsz_h_lse_sel =
			    output->h_lpf_lse_sel;
			param->rsz_rsc_param[index].rsz_h_lpf =
			    output->lpf_user_val;
		}
		if (!output->line_length)
			param->ext_mem_param[index].rsz_sdr_oft =
				(output->width * 2);
		else
			param->ext_mem_param[index].rsz_sdr_oft =
					output->line_length;
		/* adjust the line length to be a multiple of 32 */
		param->ext_mem_param[index].rsz_sdr_oft =
			((param->ext_mem_param[index].rsz_sdr_oft + 31) & ~0x1f);
		param->ext_mem_param[index].rsz_sdr_ptr_s = output->vst;
		param->ext_mem_param[index].rsz_sdr_ptr_e = output->height;
	} else
		param->rsz_en[index] = DISABLE;
}

/* Call this after setting IPIPE input size and resizer
 * output size
 */
static void ipipe_config_resize_in_param(struct ipipe_params *param, int index)
{
	param->rsz_rsc_param[index].rsz_i_vst = param->ipipe_vst;
	param->rsz_rsc_param[index].rsz_i_vsz = param->ipipe_vsz;
	param->rsz_rsc_param[index].rsz_i_hst = param->ipipe_hst;
	param->rsz_rsc_param[index].rsz_h_dif =
	    ((param->ipipe_hsz + 1) * 256) /
	    (param->rsz_rsc_param[index].rsz_o_hsz + 1);
	param->rsz_rsc_param[index].rsz_v_dif =
	    ((param->ipipe_vsz + 1) * 256) /
	    (param->rsz_rsc_param[index].rsz_o_vsz + 1);
}

static int ipipe_set_resize_config(struct device *dev,
				   unsigned int oper_mode,
				   int resizer_chained,
				   void *user_config, void *config)
{
	int ret = 0;
	struct ipipe_params *param = (struct ipipe_params *)config;

	dev_dbg(dev, "ipipe_set_resize_config\n");
	if ((ISNULL(user_config)) || (ISNULL(config))) {
		dev_err(dev, "Invalid user_config or config ptr\n");
		return -EINVAL;
	}

	memcpy((void *)config,
		(void *)&dm355_ipipe_defs,
		sizeof(struct ipipe_params));

	if (oper_mode == IMP_MODE_SINGLE_SHOT) {
		/* resizer in standalone mode. In this mode if serializer
		 *  is enabled, we need to set config params in the hw.
		 */
		struct rsz_single_shot_config *ss_config =
		    (struct rsz_single_shot_config *)user_config;

		if (resizer_chained) {
			/* shared block */
			if ((!ss_config->output1.enable) &&
			    (!ss_config->output1.enable)) {
				dev_err(dev,
					"One of the resizer output"
					" must be enabled\n");
				return -EINVAL;
			}
			ret = mutex_lock_interruptible(&oper_state.lock);
			if (ret)
				return ret;
			if (ss_config->output1.enable) {
				if (ss_config->output1.pix_fmt != IPIPE_UYVY) {
					dev_err(dev,
						"output1:Only UYVY "
						"supported\n");
					mutex_unlock(&oper_state.lock);
					return -EINVAL;
				}
				if ((ss_config->output1.width == 0) ||
				    (ss_config->output1.height == 0)) {
					dev_err(dev,
						"output1:invalid width or "
						"height\n");
					mutex_unlock(&oper_state.lock);
					return -EINVAL;
				}
				if (ss_config->output1.width % 16) {
					dev_err(dev,
						"output1:width to be a "
						"multiple of 16\n");
					mutex_unlock(&oper_state.lock);
					return -EINVAL;
				}
			}

			if (ss_config->output2.enable) {
				if (ss_config->output2.pix_fmt != IPIPE_UYVY) {
					dev_err(dev,
						"output2:Only UYVY"
						" supported\n");
					mutex_unlock(&oper_state.lock);
					return -EINVAL;
				}
				if ((ss_config->output2.width == 0) ||
				    (ss_config->output2.height == 0)) {
					dev_err(dev,
						"output2:invalid width"
						" or height\n");
					mutex_unlock(&oper_state.lock);
					return -EINVAL;
				}
				if (ss_config->output2.width % 16) {
					dev_err(dev,
						"output2:width to be a "
						"multiple of 16\n");
					mutex_unlock(&oper_state.lock);
					return -EINVAL;
				}
			}

			ipipe_config_resize_out_param(param, 0,
						      &ss_config->output1, 1);
			ipipe_config_resize_out_param(param, 1,
						      &ss_config->output2, 1);

			if (ss_config->en_flip_vert)
				param->rsz_seq_vrv = ENABLE;
			if (ss_config->en_flip_vert)
				param->rsz_seq_hrv = ENABLE;
			if (ss_config->chroma_sample_even)
				param->rsz_seq_crv = ENABLE;
			oper_state.rsz_chained = 1;
			oper_state.rsz_config_state = STATE_CONFIGURED;
			mutex_unlock(&oper_state.lock);
		} else {
			if ((ss_config->input.image_width == 0) ||
			    (ss_config->input.image_height == 0)) {
				dev_err(dev,
					"input image width or "
					"height invalid\n");
				return -EINVAL;
			}
			if (ss_config->input.image_width % 16) {
				dev_err(dev,
					"input image width to "
					"be a multiple of 16\n");
				return -EINVAL;
			}

			param->ipipeif_param.source = SDRAM_YUV;
			param->ipipeif_param.hnum =
			    ss_config->input.image_width;
			param->ipipeif_param.vnum =
			    ss_config->input.image_height;
			param->ipipeif_param.glob_hor_size =
			    ss_config->input.ppln;
			param->ipipeif_param.glob_ver_size =
			    ss_config->input.lpfr;
			param->ipipeif_param.clk_div = ss_config->input.clk_div;
			if (!ss_config->input.line_length)
				param->ipipeif_param.adofs =
					ss_config->input.image_width * 2;
			else
				param->ipipeif_param.adofs =
						ss_config->input.line_length;
			/* adjust the line length to be a multiple of 32 */
			param->ipipeif_param.adofs =
				((param->ipipeif_param.adofs + 31) & ~0x1f);
			param->ipipe_hsz = ss_config->input.image_width - 1;
			if (ss_config->input.dec_en) {
				if ((ss_config->input.rsz < 16) ||
				    (ss_config->input.rsz > 112)) {
					dev_err(dev,
						"rsz range is 16 to 112\n");
					return -EINVAL;
				}
				param->ipipeif_param.decimation =
				    IPIPEIF_DECIMATION_ON;
				param->ipipeif_param.rsz = ss_config->input.rsz;
				param->ipipeif_param.avg_filter =
				    (enum ipipeif_avg_filter)ss_config->input.
				    avg_filter_en;
				param->ipipe_hsz =
				    (((ss_config->input.image_width * 16) /
				      ss_config->input.rsz) - 1);
			}
			param->ipipe_vsz = ss_config->input.image_height - 1;
			param->ipipe_dpaths_fmt = IPIPE_YUV2YUV;
			/* TODO check if ipipe vsz and hsz are same as resizer
			   input sizes */
			ipipe_config_resize_out_param(param, 0,
						      &ss_config->output1, 1);
			ipipe_config_resize_out_param(param, 1,
						      &ss_config->output2, 1);
			/* Setup resizer input configuration */
			if (param->rsz_en[0])
				ipipe_config_resize_in_param(param, 0);

			if (param->rsz_en[1])
				ipipe_config_resize_in_param(param, 1);

			if (ss_config->en_flip_vert)
				param->rsz_seq_vrv = ENABLE;
			if (ss_config->en_flip_vert)
				param->rsz_seq_hrv = ENABLE;
			if (ss_config->chroma_sample_even)
				param->rsz_seq_crv = ENABLE;
			if (!en_serializer)
				ret = ipipe_hw_setup(config);
		}

	} else {
		/* Continuous mode. This is a shared config block */
		struct rsz_continuous_config *cont_config =
		    (struct rsz_continuous_config *)user_config;

		ret = mutex_lock_interruptible(&oper_state.lock);
		if (ret)
			return ret;
		if (resizer_chained) {
			if (!cont_config->en_output1) {
				dev_err(dev, "enable resizer - 0\n");
				mutex_unlock(&oper_state.lock);
				return -EINVAL;
			} else {
				param->rsz_en[0] = ENABLE;
				param->rsz_rsc_param[0].rsz_mode = CONTINUOUS;
			}
			if (cont_config->output2.enable) {
				if (cont_config->output2.pix_fmt
					!= IPIPE_UYVY) {
					dev_err(dev, "Only UYVY supported\n");
					mutex_unlock(&oper_state.lock);
					return -EINVAL;
				}
				if ((cont_config->output2.width == 0) ||
				    (cont_config->output2.height == 0)) {
					dev_err(dev,
						"output2:invalid width or"
						" height\n");
					mutex_unlock(&oper_state.lock);
					return -EINVAL;
				}
				if (cont_config->output2.width % 16) {
					dev_err(dev,
						"width to be a multiple"
						" of 16\n");
					mutex_unlock(&oper_state.lock);
					return -EINVAL;
				}
			}
			ipipe_config_resize_out_param(param, 1,
						      &cont_config->output2, 1);
			if (cont_config->en_flip_vert)
				param->rsz_seq_vrv = ENABLE;
			if (cont_config->en_flip_vert)
				param->rsz_seq_hrv = ENABLE;
			if (cont_config->chroma_sample_even)
				param->rsz_seq_crv = ENABLE;
			if (cont_config->en_vaaf)
				param->rsz_aal = ENABLE;
			oper_state.rsz_chained = 1;
		} else {
			param->ipipeif_param.clock_select = PIXCEL_CLK;
			param->ipipeif_param.source = CCDC;
			param->ipipe_dpaths_fmt = IPIPE_YUV2YUV;
			if (cont_config->input.dec_en) {
				if ((cont_config->input.rsz < 16) ||
				    (cont_config->input.rsz > 112)) {
					dev_err(dev,
						"rsz range is 16 to 112\n");
					mutex_unlock(&oper_state.lock);
					return -EINVAL;
				}
				param->ipipeif_param.decimation =
				    IPIPEIF_DECIMATION_ON;
				param->ipipeif_param.rsz =
				    cont_config->input.rsz;
				param->ipipeif_param.avg_filter =
				    (enum ipipeif_avg_filter)cont_config->input.
				    avg_filter_en;
			}
			param->ipipeif_param.mode = CONTINUOUS;
			if (!cont_config->en_output1)
				param->rsz_en[0] = DISABLE;
			else
				param->rsz_rsc_param[0].rsz_mode = CONTINUOUS;
			if (cont_config->output2.enable) {
				if (cont_config->output2.pix_fmt
					!= IPIPE_UYVY) {
					dev_err(dev, "Only UYVY supported\n");
					mutex_unlock(&oper_state.lock);
					return -EINVAL;
				}
				if ((cont_config->output2.width == 0) ||
				    (cont_config->output2.height == 0)) {
					dev_err(dev,
						"output2:invalid width"
						" or height\n");
					mutex_unlock(&oper_state.lock);
					return -EINVAL;
				}
				if (cont_config->output2.width % 16) {
					dev_err(dev,
						"width to be a multiple"
						" of 16\n");
					mutex_unlock(&oper_state.lock);
					return -EINVAL;
				}
				param->rsz_rsc_param[1].rsz_mode = CONTINUOUS;
			}
			ipipe_config_resize_out_param(param, 1,
						      &cont_config->output2, 1);
			if (cont_config->en_flip_vert)
				param->rsz_seq_vrv = ENABLE;
			if (cont_config->en_flip_vert)
				param->rsz_seq_hrv = ENABLE;
			if (cont_config->chroma_sample_even)
				param->rsz_seq_crv = ENABLE;
			if (cont_config->en_vaaf)
				param->rsz_aal = ENABLE;
		}
		oper_state.rsz_config_state = STATE_CONFIGURED;
		mutex_unlock(&oper_state.lock);
	}
	return ret;
}

static void ipipe_config_resizer_bypass(struct ipipe_params *param)
{
	param->rsz_seq_seq = DISABLE;
	param->rsz_seq_tmm = DISABLE;
	param->rsz_aal = DISABLE;
	param->rsz_rsc_param[0].rsz_v_phs = 0;
	param->rsz_rsc_param[0].rsz_h_phs = 0;
	param->rsz_rsc_param[0].rsz_v_dif = 256;
	param->rsz_rsc_param[0].rsz_h_dif = 256;
	param->rsz_rsc_param[0].rsz_h_lse_sel = RSZ_H_LPF_LSE_INTERN;
	param->rsz_en[0] = ENABLE;
	param->rsz_en[1] = DISABLE;
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
		printk(KERN_NOTICE "Serializer is diabled\n");
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
}
#endif
static int ipipe_set_preview_config(struct device *dev,
				    unsigned int oper_mode,
				    void *user_config, void *config)
{
	int ret = 0;
	struct ipipe_params *param = (struct ipipe_params *)config;

	dev_err(dev, "ipipe_set_preview_config\n");
	if ((ISNULL(user_config)) || (ISNULL(config))) {
		dev_err(dev, "Invalid user_config or config ptr\n");
		return -EINVAL;
	}

	if (!oper_state.rsz_chained) {
		/* For chained resizer, defaults are set by resizer */
		memcpy((void *)config,
		       (void *)&dm355_ipipe_defs,
			sizeof(struct ipipe_params));
	}

	/* shared block */
	ret = mutex_lock_interruptible(&oper_state.lock);
	if (ret)
		return ret;
	if (oper_mode == IMP_MODE_SINGLE_SHOT) {
		/* previewer in standalone mode. In this mode if serializer
		 * is enabled, we need to set config params in the hw.
		 */
		struct prev_single_shot_config *ss_config =
		    (struct prev_single_shot_config *)user_config;
		if ((ss_config->input.image_width == 0) ||
		    (ss_config->input.image_height == 0)) {
			dev_err(dev, "input image width or height invalid\n");
			mutex_unlock(&oper_state.lock);
			return -EINVAL;
		}
		if (ss_config->input.image_width % 16) {
			dev_err(dev,
				"input image width to be a multiple of 16\n");
			mutex_unlock(&oper_state.lock);
			return -EINVAL;
		}

		if (ipipe_process_pix_fmts(ss_config->input.pix_fmt,
					   ss_config->output.pix_fmt,
					   param) < 0) {
			dev_err(dev, "error in input or output pix format\n");
			mutex_unlock(&oper_state.lock);
			return -EINVAL;
		}
		param->ipipe_dpaths_bypass = ss_config->bypass;
		param->ipipeif_param.hnum = ss_config->input.image_width;
		param->ipipeif_param.vnum = ss_config->input.image_height;
		param->ipipeif_param.glob_hor_size = ss_config->input.ppln;
		param->ipipeif_param.glob_ver_size = ss_config->input.lpfr;
		param->ipipeif_param.clk_div = ss_config->input.clk_div;
		if (!ss_config->input.line_length) {
			/* Calculate the line length */
			if ((param->ipipeif_param.ialaw) ||
			    (param->ipipeif_param.pack_mode))
				param->ipipeif_param.adofs =
					ss_config->input.image_width;
			else
				param->ipipeif_param.adofs =
					ss_config->input.image_width * 2;
		} else
			param->ipipeif_param.adofs =
				ss_config->input.line_length;
		/* Adjust adofs to be a multiple of 32 */
		param->ipipeif_param.adofs =
			((param->ipipeif_param.adofs + 31) & ~0x1f);
		param->ipipe_vst = ss_config->input.vst;
		param->ipipe_hst = ss_config->input.hst;
		param->ipipe_colpat_olop = ss_config->input.colp_olop;
		param->ipipe_colpat_olep = ss_config->input.colp_olep;
		param->ipipe_colpat_elop = ss_config->input.colp_elop;
		param->ipipe_colpat_elep = ss_config->input.colp_elep;
		param->ipipe_hsz = ss_config->input.image_width - 1;
		if (ss_config->input.dec_en) {
			if ((ss_config->input.rsz < 16) ||
			    (ss_config->input.rsz > 112)) {
				dev_err(dev, "rsz range is 16 to 112\n");
				mutex_unlock(&oper_state.lock);
				return -EINVAL;
			}
			param->ipipeif_param.decimation = IPIPEIF_DECIMATION_ON;
			param->ipipeif_param.rsz = ss_config->input.rsz;
			param->ipipeif_param.avg_filter =
			    (enum ipipeif_avg_filter)ss_config->input.
			    avg_filter_en;
			param->ipipe_hsz =
			    (((ss_config->input.image_width * 16) /
			      ss_config->input.rsz) - 1);
		}
		param->ipipe_vsz = ss_config->input.image_height - 1;
		param->ipipeif_param.gain = ss_config->input.gain;
		if (ss_config->input.pix_fmt == IPIPE_UYVY)
			param->ipipeif_param.source = SDRAM_YUV;
		else
			param->ipipeif_param.source = SDRAM_RAW;

		/* update the resize parameters */
		if (oper_state.rsz_chained) {
			if (param->rsz_en[0])
				ipipe_config_resize_in_param(param, 0);
			if (param->rsz_en[1])
				ipipe_config_resize_in_param(param, 1);
		} else {
			/* Using IPIPE resizer as pass through */
			param->rsz_rsc_param[0].rsz_o_vsz = param->ipipe_vsz;
			param->rsz_rsc_param[0].rsz_o_hsz = param->ipipe_hsz;
			param->rsz_rsc_param[0].rsz_o_hst = param->ipipe_hst;
			param->ext_mem_param[0].rsz_sdr_oft =
			    ss_config->input.image_width * 2;
			param->ext_mem_param[0].rsz_sdr_ptr_s =
			    param->ipipe_vst;
			param->ext_mem_param[0].rsz_sdr_ptr_e =
			    (ss_config->input.image_height);
			ipipe_config_resizer_bypass(param);
		}
		if (!en_serializer)
			ret = ipipe_hw_setup(config);
	} else {
		/* continuous mode */
		struct prev_continuous_config *cont_config =
		    (struct prev_continuous_config *)user_config;

		param->ipipe_dpaths_bypass = cont_config->bypass;
		if (cont_config->input.en_df_sub) {
			dev_err(dev, "DF subtraction is not supported\n");
			mutex_unlock(&oper_state.lock);
			return -EINVAL;
		}
		param->ipipeif_param.source = CCDC;
		param->ipipeif_param.clock_select = PIXCEL_CLK;
		param->ipipeif_param.mode = CONTINUOUS;
		if (cont_config->input.dec_en) {
			if ((cont_config->input.rsz < 16) ||
			    (cont_config->input.rsz > 112)) {
				dev_err(dev, "rsz range is 16 to 112\n");
				mutex_unlock(&oper_state.lock);
				return -EINVAL;
			}
			param->ipipeif_param.decimation = IPIPEIF_DECIMATION_ON;
			param->ipipeif_param.rsz = cont_config->input.rsz;
			param->ipipeif_param.avg_filter =
			    (enum ipipeif_avg_filter)cont_config->input.
			    avg_filter_en;
		}
		/* update the resize parameters */
		param->ipipe_mode = CONTINUOUS;
		param->ipipe_colpat_olop = cont_config->input.colp_olop;
		param->ipipe_colpat_olep = cont_config->input.colp_olep;
		param->ipipe_colpat_elop = cont_config->input.colp_elop;
		param->ipipe_colpat_elep = cont_config->input.colp_elep;
		param->ipipeif_param.gain = cont_config->input.gain;
		if (!oper_state.rsz_chained) {
			param->rsz_en[0] = ENABLE;
			param->rsz_rsc_param[0].rsz_mode = CONTINUOUS;
			param->rsz_en[1] = DISABLE;
		}
		oper_state.prev_config_state = STATE_CONFIGURED;
	}
	mutex_unlock(&oper_state.lock);
	return 0;
}
struct imp_hw_interface *imp_get_hw_if(void)
{
	return (&dm355_ipipe_interface);
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
		    ((win->width * 16) / param->ipipeif_param.rsz) - 1;
	} else
		param->ipipe_hsz = win->width - 1;
	if (!oper_state.frame_format) {
		param->ipipe_vsz = (win->height / 2) - 1;
		param->ipipe_vst = (win->vst / 2);
	} else {
		param->ipipe_vsz = win->height - 1;
		param->ipipe_vst = win->vst;
	}
	param->ipipe_hst = win->hst;
	if (param->rsz_en[0]) {
		param->rsz_rsc_param[0].rsz_i_vst = param->ipipe_vst;
		param->rsz_rsc_param[0].rsz_i_vsz = param->ipipe_vsz;
		param->rsz_rsc_param[0].rsz_i_hst = param->ipipe_hst;
	}
	if (param->rsz_en[1]) {
		param->rsz_rsc_param[1].rsz_i_vst = param->ipipe_vst;
		param->rsz_rsc_param[1].rsz_i_vsz = param->ipipe_vsz;
		param->rsz_rsc_param[1].rsz_i_hst = param->ipipe_hst;
	}
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
		    ((param->ipipe_hsz + 1) * param->ipipeif_param.rsz) / 16;
	else
		win->width = param->ipipe_hsz + 1;
	if (!oper_state.frame_format) {
		win->height = (param->ipipe_vsz + 1) * 2;
		win->vst = (param->ipipe_vst * 2);
	} else {
		win->height = param->ipipe_vsz + 1;
		win->vst = param->ipipe_vst;
	}
	win->hst = param->ipipe_hst;
	mutex_unlock(&oper_state.lock);
	return 0;
}

static int ipipe_set_in_pixel_format(enum imp_pix_formats pix_fmt)
{
	int ret;

	ret = mutex_lock_interruptible(&oper_state.lock);
	if (ret)
		return ret;
	oper_state.in_pixel_format = pix_fmt;
	mutex_unlock(&oper_state.lock);
	return 0;
}

static int ipipe_set_out_pixel_format(enum imp_pix_formats pix_fmt)
{
	int ret;

	ret = mutex_lock_interruptible(&oper_state.lock);
	if (ret)
		return ret;
	oper_state.out_pixel_format = pix_fmt;
	mutex_unlock(&oper_state.lock);
	return 0;
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
	int ret;
	struct ipipe_params *param = oper_state.shared_config_param;
	struct rsz_ss_output_spec output_specs;

	if (!param->rsz_en[0]) {
		printk(KERN_ERR "resizer output1 not enabled\n");
		return -1;
	}
	output_specs.enable = 1;
	output_specs.width = win->width;
	/* Always set output height same as in height
	   for de-interlacing
	 */
	output_specs.height = win->height;
	output_specs.vst = win->vst;
	output_specs.hst = win->hst;
	ret = mutex_lock_interruptible(&oper_state.lock);
	if (ret)
		return ret;
	ipipe_config_resize_out_param(param, 0, &output_specs, 0);
	mutex_unlock(&oper_state.lock);
	return 0;
}
static int ipipe_get_output_state(unsigned char out_sel)
{
	struct ipipe_params *param = oper_state.shared_config_param;

	if ((out_sel != 0) && (out_sel != 1))
		return 0;
	return (param->rsz_en[out_sel]);
}

/* This should be called only after setting the output
 * window params. This also assumes the corresponding
 * output is configured prior to calling this.
 */
static int ipipe_get_line_length(unsigned char out_sel)
{
	struct ipipe_params *param = oper_state.shared_config_param;
	int line_len;

	if ((out_sel != 0) && (out_sel != 1))
		return -1;
	/* assume output is always UYVY. Change this if we
	 * support RGB
	 */
	if (!param->rsz_en[out_sel])
		return -1;

	line_len = (param->rsz_rsc_param[out_sel].rsz_o_hsz + 1) * 2;

	/* Adjust the length to 32 byte boundary */
	line_len = ((line_len + 31) & ~0x1F);
	return line_len;
}

static int ipipe_get_image_height(unsigned char out_sel)
{
	struct ipipe_params *param = oper_state.shared_config_param;
	if ((out_sel != 0) && (out_sel != 1))
		return -1;
	/* assume output is always UYVY. Change this if we
	 * support RGB
	 */
	if (!param->rsz_en[out_sel])
		return -1;

	return (param->rsz_rsc_param[out_sel].rsz_o_vsz + 1);
}

static int dm355_ipipe_init(void)
{
	oper_state.shared_config_param =
		kmalloc(sizeof(struct ipipe_params), GFP_KERNEL);
	if (ISNULL(oper_state.shared_config_param)) {
		printk(KERN_ERR
		       "dm355_ipipe_init: failed to allocate memory\n");
		return -ENOMEM;
	}
	dfc.table = ipipe_dfc_table;
	gamma.table_r = ipipe_gamma_table_r;
	gamma.table_b = ipipe_gamma_table_b;
	gamma.table_g = ipipe_gamma_table_g;
	gamma.table_rgb_all = ipipe_gamma_table_all;
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
		       "DM355 IPIPE intialized in Single Shot mode\n");
	else
		printk(KERN_NOTICE
		       "DM355 IPIPE intialized in Continuous mode\n");
	return 0;
}

static void dm355_ipipe_cleanup(void)
{
	kfree(oper_state.shared_config_param);
	printk(KERN_NOTICE "DM355 IPIPE harware module exited\n");
}

subsys_initcall(dm355_ipipe_init);
module_exit(dm355_ipipe_cleanup);
MODULE_LICENSE("GPL");
