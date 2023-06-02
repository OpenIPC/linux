/*
 *
 * Copyright (C) 2008 Texas Instruments Inc
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
 *
 **************************************************************************/
#include <media/davinci/dm365_ipipe.h>
/* Defaults for lutdpc */
struct prev_lutdpc dm365_lutdpc_defaults = {
	.en = 0
};

/* Defaults for otfdpc */
struct prev_lutdpc dm365_otfdpc_defaults = {
	.en = 0
};

/* Defaults for 2D - nf */
struct prev_nf dm365_nf_defaults = {
	.en = 0
};

/* defaults for GIC */
struct prev_gic dm365_gic_defaults = {
	.en = 0
};

/* Defaults for white balance */
struct prev_wb dm365_wb_defaults = {
	.gain_r = {2, 0x00},
	.gain_gr = {2, 0x00},
	.gain_gb = {2, 0x00},
	.gain_b = {2, 0x00}
};

/* Defaults for CFA */
struct prev_cfa dm365_cfa_defaults = {
	.alg = IPIPE_CFA_ALG_2DIRAC,
	.hpf_thr_2dir = 0,
	.hpf_slp_2dir = 0,
	.hp_mix_thr_2dir = 0,
	.hp_mix_slope_2dir = 0,
	.dir_thr_2dir = 0,
	.dir_slope_2dir = 0,
	.nd_wt_2dir = 0,
	.hue_fract_daa = 0,
	.edge_thr_daa = 0,
	.thr_min_daa = 0,
	.thr_slope_daa = 0,
	.slope_min_daa = 0,
	.slope_slope_daa = 0,
	.lp_wt_daa = 0
};

/* Defaults for rgb2rgb */
struct prev_rgb2rgb dm365_rgb2rgb_defaults = {
	.coef_rr = {1, 0},	/* 256 */
	.coef_gr = {0, 0},
	.coef_br = {0, 0},
	.coef_rg = {0, 0},
	.coef_gg = {1, 0},	/* 256 */
	.coef_bg = {0, 0},
	.coef_rb = {0, 0},
	.coef_gb = {0, 0},
	.coef_bb = {1, 0},	/* 256 */
	.out_ofst_r = 0,
	.out_ofst_g = 0,
	.out_ofst_b = 0
};

/* Defaults for gamma correction */
struct prev_gamma dm365_gamma_defaults = {
	.bypass_r = 0,
	.bypass_b = 0,
	.bypass_g = 0,
	.tbl_sel = IPIPE_GAMMA_TBL_ROM
};

/* Defaults for 3d lut */
struct prev_3d_lut dm365_3d_lut_defaults = {
	.en = 0
};

/* Defaults for lumina adjustments */
struct prev_lum_adj dm365_lum_adj_defaults = {
	.brightness = 0,
	.contrast = 16
};

/* Defaults for rgb2yuv conversion */
struct prev_rgb2yuv dm365_rgb2yuv_defaults = {
	.coef_ry = {0, 0x4D},
	.coef_gy = {0, 0x96},
	.coef_by = {0, 0x1D},
	.coef_rcb = {0xF, 0xD5},
	.coef_gcb = {0xF, 0xAB},
	.coef_bcb = {0, 0x80},
	.coef_rcr = {0, 0x80},
	.coef_gcr = {0xF, 0x95},
	.coef_bcr = {0xF, 0xEB},
	.out_ofst_y = 0,
	.out_ofst_cb = 0x80,
	.out_ofst_cr = 0x80
};

/* Defaults for GBCE */
struct prev_gbce dm365_gbce_defaults = {
	.en = 0
};

/* Defaults for yuv 422 conversion */
struct prev_yuv422_conv dm365_yuv422_conv_defaults = {
	.en_chrom_lpf = 0,
	.chrom_pos = IPIPE_YUV422_CHR_POS_COSITE
};

/* Defaults for Edge Ehnancements  */
struct prev_yee dm365_yee_defaults = {
	.en = 0,
};

/* Defaults for CAR conversion */
struct prev_car dm365_car_defaults = {
	.en = 0,
};

/* Defaults for CGS */
struct prev_cgs dm365_cgs_defaults = {
	.en = 0,
};

#define  WIDTH_I 640
#define  HEIGHT_I 480
#define  WIDTH_O 640
#define  HEIGHT_O 480

/* default ipipeif settings */
struct ipipeif_5_1 ipipeif_5_1_defaults = {
	.pack_mode = IPIPEIF_5_1_PACK_16_BIT,
	.data_shift = IPIPEIF_BITS11_0,
	.source1 = SRC1_PARALLEL_PORT,
	.clk_div = {
		.m = 1,	/* clock = sdram clock * (m/n) */
		.n = 6
	},
	.dpc = {
		.en = 0,
	},
	.dpcm = {
		.en = 0,
		.type = DPCM_8BIT_12BIT,
		.pred = DPCM_SIMPLE_PRED
	},
#ifdef CONFIG_VIDEO_YCBCR
	.pix_order = IPIPEIF_Y_CBCR,
	.isif_port = {
		.if_type = VPFE_YCBCR_SYNC_8,
		.hdpol = VPFE_PINPOL_POSITIVE,
		.vdpol =VPFE_PINPOL_POSITIVE
	},
#else
	.pix_order = IPIPEIF_CBCR_Y,
	.isif_port = {
		.if_type = VPFE_RAW_BAYER,
		.hdpol = VPFE_PINPOL_POSITIVE,
		.vdpol =VPFE_PINPOL_POSITIVE 
	},
#endif
	.clip = 4095,
	.align_sync = 0,
	.rsz_start = 0,
	.df_gain_en = 0
};

struct ipipe_params dm365_ipipe_defs = {
	.ipipeif_param = {
		.mode = ONE_SHOT,
		.source = SDRAM_RAW,
		.clock_select = SDRAM_CLK,
		.glob_hor_size = WIDTH_I + 8,
		.glob_ver_size = HEIGHT_I + 10,
		.hnum = WIDTH_I,
		.vnum = HEIGHT_I,
		.adofs = WIDTH_I * 2,
		.rsz = 16,	/* resize ratio 16/rsz */
		.decimation = IPIPEIF_DECIMATION_OFF,
		.avg_filter = AVG_OFF,
		.gain = 0x200,	/* U10Q9 */
	},
	.ipipe_mode = ONE_SHOT,
	.ipipe_dpaths_fmt = IPIPE_RAW2YUV,
	.ipipe_colpat_olop = IPIPE_GREEN_BLUE,
	.ipipe_colpat_olep = IPIPE_BLUE,
	.ipipe_colpat_elop = IPIPE_RED,
	.ipipe_colpat_elep = IPIPE_GREEN_RED,
	.ipipe_vps = 0,
	.ipipe_vsz = HEIGHT_I - 1,
	.ipipe_hps = 0,
	.ipipe_hsz = WIDTH_I - 1,
	.rsz_common = {
		.vps = 0,
		.vsz = HEIGHT_I - 1,
		.hps = 0,
		.hsz = WIDTH_I - 1,
		.src_img_fmt = RSZ_IMG_422,
		.y_c = 0,
		.raw_flip = 1,	/* flip preserve Raw format */
		.source = IPIPE_DATA,
		.passthrough = IPIPE_BYPASS_OFF,
		.yuv_y_min = 0,
		.yuv_y_max = 255,
		.yuv_c_min = 0,
		.yuv_c_max = 255,
		.rsz_seq_crv = DISABLE,
		.out_chr_pos = IPIPE_YUV422_CHR_POS_COSITE
	},
	.rsz_rsc_param = {
		{
			.mode = ONE_SHOT,
			.h_flip = DISABLE,
			.v_flip = DISABLE,
			.cen = DISABLE,
			.yen = DISABLE,
			.i_vps = 0,
			.i_hps = 0,
			.o_vsz = HEIGHT_O - 1,
			.o_hsz = WIDTH_O - 1,
			.v_phs_y = 0,
			.v_phs_c = 0,
			.v_dif = 256,
			.v_typ_y = RSZ_INTP_CUBIC,
			.h_typ_c = RSZ_INTP_CUBIC,
			.v_lpf_int_y = 0,
			.v_lpf_int_c = 0,
			.h_phs = 0,
			.h_dif = 256,
			.h_typ_y = RSZ_INTP_CUBIC,
			.h_typ_c = RSZ_INTP_CUBIC,
			.h_lpf_int_y = 0,
			.h_lpf_int_c = 0,
			.dscale_en = 0,
			.h_dscale_ave_sz = IPIPE_DWN_SCALE_1_OVER_2,
			.v_dscale_ave_sz = IPIPE_DWN_SCALE_1_OVER_2,
			.f_div.en = 0
		},
		{
			.mode = ONE_SHOT,
			.h_flip = DISABLE,
			.v_flip = DISABLE,
			.cen = DISABLE,
			.yen = DISABLE,
			.i_vps = 0,
			.i_hps = 0,
			.o_vsz = HEIGHT_O - 1,
			.o_hsz = WIDTH_O - 1,
			.v_phs_y = 0,
			.v_phs_c = 0,
			.v_dif = 256,
			.v_typ_y = RSZ_INTP_CUBIC,
			.h_typ_c = RSZ_INTP_CUBIC,
			.v_lpf_int_y = 0,
			.v_lpf_int_c = 0,
			.h_phs = 0,
			.h_dif = 256,
			.h_typ_y = RSZ_INTP_CUBIC,
			.h_typ_c = RSZ_INTP_CUBIC,
			.h_lpf_int_y = 0,
			.h_lpf_int_c = 0,
			.dscale_en = 0,
			.h_dscale_ave_sz = IPIPE_DWN_SCALE_1_OVER_2,
			.v_dscale_ave_sz = IPIPE_DWN_SCALE_1_OVER_2,
			.f_div.en = 0
		},
	},
	.rsz2rgb = {
		{
			.rgb_en = DISABLE
		},
		{
			.rgb_en = DISABLE
		}
	},
	.ext_mem_param = {
		{
			.rsz_sdr_oft_y = WIDTH_O << 1,
			.rsz_sdr_ptr_s_y = 0,
			.rsz_sdr_ptr_e_y = HEIGHT_O,
			.rsz_sdr_oft_c = WIDTH_O,
			.rsz_sdr_ptr_s_c = 0,
			.rsz_sdr_ptr_e_c = HEIGHT_O >> 1,
			.flip_ofst_y = 0,
			.flip_ofst_c = 0,
			.c_offset = 0,
			.user_y_ofst = 0,
			.user_c_ofst = 0
		},
		{
			.rsz_sdr_oft_y = WIDTH_O << 1,
			.rsz_sdr_ptr_s_y = 0,
			.rsz_sdr_ptr_e_y = HEIGHT_O,
			.rsz_sdr_oft_c = WIDTH_O,
			.rsz_sdr_ptr_s_c = 0,
			.rsz_sdr_ptr_e_c = HEIGHT_O,
			.flip_ofst_y = 0,
			.flip_ofst_c = 0,
			.c_offset = 0,
			.user_y_ofst = 0,
			.user_c_ofst = 0
		},
	},
	.rsz_en[0] = ENABLE,
	.rsz_en[1] = DISABLE
};

struct prev_single_shot_config dm365_prev_ss_config_defs = {
	.bypass = IPIPE_BYPASS_OFF,
	.input = {
		.image_width = WIDTH_I,
		.image_height = HEIGHT_I,
		.vst = 0,
		.hst = 0,
		.ppln = WIDTH_I + 8,
		.lpfr = HEIGHT_I + 10,
		.pred = DPCM_SIMPLE_PRED,
		.clk_div = {1, 6},
		.data_shift = IPIPEIF_BITS11_0,
		.dec_en = 0,
		.rsz = 16,	/* resize ratio 16/rsz */
		.frame_div_mode_en = 0,
		.avg_filter_en = AVG_OFF,
		.dpc = {0, 0},
		.gain = 512,
		.clip = 4095,
		.align_sync = 0,
		.rsz_start = 0,
		.pix_fmt = IPIPE_BAYER,
		.colp_olop = IPIPE_GREEN_BLUE,
		.colp_olep = IPIPE_BLUE,
		.colp_elop = IPIPE_RED,
		.colp_elep = IPIPE_GREEN_RED
	},
	.output = {
		.pix_fmt = IPIPE_UYVY
	}
};

struct prev_continuous_config dm365_prev_cont_config_defs = {
	.bypass = IPIPE_BYPASS_OFF,
	.input = {
		.en_df_sub = 0,
		.dec_en = 0,
		.rsz = 16,
		.avg_filter_en = AVG_OFF,
		.gain = 512,
		.clip = 4095,
		//YCBCR
		.colp_olop = IPIPE_GREEN_RED,
		.colp_olep = IPIPE_RED,
		.colp_elop = IPIPE_BLUE,
		.colp_elep = IPIPE_GREEN_BLUE
	},
};

struct rsz_single_shot_config dm365_rsz_ss_config_defs = {
	.input = {
		.image_width = WIDTH_I,
		.image_height = HEIGHT_I,
		.vst = 0,
		.hst = 0,
		.ppln = WIDTH_I + 8,
		.lpfr = HEIGHT_I + 10,
		.clk_div = {1, 6},
		.dec_en = 0,
		.rsz = 16,	/* resize ratio 16/rsz */
		.frame_div_mode_en = 0,
		.avg_filter_en = AVG_OFF,
		.align_sync = 0,
		.rsz_start = 0,
		.pix_fmt = IPIPE_UYVY
	},
	.output1 = {
		.enable = 1,
		.pix_fmt = IPIPE_UYVY,
		.h_flip = 0,
		.v_flip = 0,
		.width = WIDTH_O,
		.height = HEIGHT_O,
		.vst_y = 0,
		.vst_c = 0,
		.v_typ_y = RSZ_INTP_CUBIC,
		.v_typ_c = RSZ_INTP_CUBIC,
		.v_lpf_int_y = 0,
		.v_lpf_int_c = 0,
		.h_typ_y = RSZ_INTP_CUBIC,
		.h_typ_c = RSZ_INTP_CUBIC,
		.h_lpf_int_y = 0,
		.h_lpf_int_c = 0,
		.en_down_scale = 0,
		.h_dscale_ave_sz = IPIPE_DWN_SCALE_1_OVER_2,
		.v_dscale_ave_sz = IPIPE_DWN_SCALE_1_OVER_2,
		.user_y_ofst = 0,
		.user_c_ofst = 0
	},
	.output2 = {
		.enable = 1,
		.pix_fmt = IPIPE_UYVY,
		.h_flip = 0,
		.v_flip = 0,
		.width = WIDTH_O,
		.height = HEIGHT_O,
		.vst_y = 0,
		.vst_c = 0,
		.v_typ_y = RSZ_INTP_CUBIC,
		.v_typ_c = RSZ_INTP_CUBIC,
		.v_lpf_int_y = 0,
		.v_lpf_int_c = 0,
		.h_typ_y = RSZ_INTP_CUBIC,
		.h_typ_c = RSZ_INTP_CUBIC,
		.h_lpf_int_y = 0,
		.h_lpf_int_c = 0,
		.en_down_scale = 0,
		.h_dscale_ave_sz = IPIPE_DWN_SCALE_1_OVER_2,
		.v_dscale_ave_sz = IPIPE_DWN_SCALE_1_OVER_2,
		.user_y_ofst = 0,
		.user_c_ofst = 0
	},
	.chroma_sample_even = 0,
	.yuv_y_min = 0,
	.yuv_y_max = 255,
	.yuv_c_min = 0,
	.yuv_c_max = 255,
	.out_chr_pos = IPIPE_YUV422_CHR_POS_COSITE,
};

struct rsz_continuous_config dm365_rsz_cont_config_defs = {
	.output1 = {
		.enable = 1,
		.h_flip = 0,
		.v_flip = 0,
		.v_typ_y = RSZ_INTP_CUBIC,
		.v_typ_c = RSZ_INTP_CUBIC,
		.v_lpf_int_y = 0,
		.v_lpf_int_c = 0,
		.h_typ_y = RSZ_INTP_CUBIC,
		.h_typ_c = RSZ_INTP_CUBIC,
		.h_lpf_int_y = 0,
		.h_lpf_int_c = 0,
		.en_down_scale = 0,
		.h_dscale_ave_sz = IPIPE_DWN_SCALE_1_OVER_2,
		.v_dscale_ave_sz = IPIPE_DWN_SCALE_1_OVER_2,
		.user_y_ofst = 0,
		.user_c_ofst = 0
	},
	.output2 = {
		.enable = 1,
		.pix_fmt = IPIPE_UYVY,
		.h_flip = 0,
		.v_flip = 0,
		.width = WIDTH_O,
		.height = HEIGHT_O,
		.vst_y = 0,
		.vst_c = 0,
		.v_typ_y = RSZ_INTP_CUBIC,
		.v_typ_c = RSZ_INTP_CUBIC,
		.v_lpf_int_y = 0,
		.v_lpf_int_c = 0,
		.h_typ_y = RSZ_INTP_CUBIC,
		.h_typ_c = RSZ_INTP_CUBIC,
		.h_lpf_int_y = 0,
		.h_lpf_int_c = 0,
		.en_down_scale = 0,
		.h_dscale_ave_sz = IPIPE_DWN_SCALE_1_OVER_2,
		.v_dscale_ave_sz = IPIPE_DWN_SCALE_1_OVER_2,
		.user_y_ofst = 0,
		.user_c_ofst = 0
	},
	.chroma_sample_even = 0,
	.yuv_y_min = 0,
	.yuv_y_max = 255,
	.yuv_c_min = 0,
	.yuv_c_max = 255,
	.out_chr_pos = IPIPE_YUV422_CHR_POS_COSITE,
};
