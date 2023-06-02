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
#include <media/davinci/dm355_ipipe.h>
/* Defaults for pre-filter */
struct prev_prefilter dm355_pf_defaults = {
	.en = 0,
	.en_adapt_prefilter = 1,
	.en_adapt_dotred = 0,
	.aver_meth_gs1 = PREV_PF_AVG4PIX,
	.aver_meth_gs2 = PREV_PF_AVG4PIX,
	.pre_gain = 128,
	.pre_shf = 9,
	.pre_thr_g = 500,
	.pre_thr_b = 4096,
	.pre_thr_1 = 800
};

/* Defaults for dfc */
struct prev_dfc dm355_dfc_defaults = {
	.en = 0,
	.vert_copy_method = IPIPE_DFC_COPY_FROM_TOP,
	.dfc_size = 0
};

/* Defaults for 2D - nf */
struct prev_nf dm355_nf_defaults = {
	.en = 0,
	.gr_sample_meth = IPIPE_NF_BOX,
	.shft_val = 0,
	.spread_val = 0
};

/* Defaults for white balance */
struct prev_wb dm355_wb_defaults = {
	.dgn = {2, 0},		/* 512 */
	.gain_r = {2, 0x10},
	.gain_gr = {1, 0x70},
	.gain_gb = {1, 0x70},
	.gain_b = {2, 0x30}
};

/* Defaults for rgb2rgb */
struct prev_rgb2rgb dm355_rgb2rgb_defaults = {
	.coef_rr = {1, 0xa1},	/* 256 */
	.coef_gr = {0xf, 0x8a},
	.coef_br = {0xf, 0xd5},
	.coef_rg = {0xf, 0xa1},
	.coef_gg = {1, 0xc4},	/* 256 */
	.coef_bg = {0xf, 0x9b},
	.coef_rb = {0xf, 0xbd},
	.coef_gb = {0xf, 0xb1},
	.coef_bb = {1, 0x92},	/* 256 */
	.out_ofst_r = 0,
	.out_ofst_g = 0,
	.out_ofst_b = 0
};

/* Defaults for gamma correction */
struct prev_gamma dm355_gamma_defaults = {
	.bypass_r = 1,
	.bypass_b = 1,
	.bypass_g = 1,
	.tbl_sel = IPIPE_GAMMA_TBL_ROM
};

/* Defaults for rgb2yuv conversion */
struct prev_rgb2yuv dm355_rgb2yuv_defaults = {
	.coef_ry = {0, 0x4d},
	.coef_gy = {0, 0x96},
	.coef_by = {0, 0x1d},
	.coef_rcb = {3, 0xD4},	/* 981 */
	.coef_gcb = {3, 0xAC},	/* 939 */
	.coef_bcb = {0, 0x80},
	.coef_rcr = {0, 0x80},
	.coef_gcr = {3, 0x95},	/* 917 */
	.coef_bcr = {3, 0xEB},	/* 1003 */
	.out_ofst_y = 0,
	.out_ofst_cb = 0x80,
	.out_ofst_cr = 0x80
};

/* Defaults for lumina adjustments */
struct prev_lum_adj dm355_lum_adj_defaults = {
	.brightness = 16,
	.contast = 16
};

/* Defaults for yuv 422 conversion */
struct prev_yuv422_conv dm355_yuv422_conv_defaults = {
	.lum_min = 0,
	.lum_max = 255,
	.chrom_min = 0,
	.chrom_max = 255,
	.en_chrom_lpf = 0,
	.chrom_pos = IPIPE_YUV422_CHR_POS_CENTRE
};

/* Defaults for yuv 422 conversion */
struct prev_yee dm355_yee_defaults = {
	.en = 0,
	.en_emf = 0,
	.hpf_shft = 4,
	.hpf_coef_00 = 48,
	.hpf_coef_01 = 12,
	.hpf_coef_02 = 1014,
	.hpf_coef_10 = 12,
	.hpf_coef_11 = 0,
	.hpf_coef_12 = 1018,
	.hpf_coef_20 = 1014,
	.hpf_coef_21 = 1018,
	.hpf_coef_22 = 1022
};

/* Defaults for yuv 422 conversion */
struct prev_fcs dm355_fcs_defaults = {
	.en = 0,
	.type = IPIPE_FCS_Y,
	.hpf_shft_y = 0,
	.gain_shft_c = 7,
	.thr = 235,
	.sgn = 0,
	.lth = 0
};

#define  WIDTH_I 640
#define  HEIGHT_I 480
#define  WIDTH_O 640
#define  HEIGHT_O 480

struct ipipe_params dm355_ipipe_defs = {
	.ipipeif_param = {
		.data_shift = IPIPEIF_BITS9_0,
		.clock_select = SDRAM_CLK,
		.ialaw = ALAW_OFF,
		.pack_mode = SIXTEEN_BIT,
		.avg_filter = AVG_OFF,
		.clk_div = IPIPEIF_DIVIDE_SIXTH,
		.source = SDRAM_RAW,
		.decimation = IPIPEIF_DECIMATION_OFF,
		.mode = ONE_SHOT,
		.glob_hor_size = WIDTH_I + 8,
		.glob_ver_size = HEIGHT_I + 10,
		.hnum = WIDTH_I,
		.vnum = HEIGHT_I,
		.adofs = WIDTH_I * 2,
		/* resize ratio 16/rsz */
		.rsz = 16,
		/* U10Q9 */
		.gain = 0x200,
	},
	.ipipe_mode = ONE_SHOT,
	.ipipe_dpaths_fmt = IPIPE_RAW2YUV,
	.ipipe_dpaths_bypass = IPIPE_BYPASS_OFF,
	.ipipe_colpat_olop = IPIPE_GREEN_BLUE,
	.ipipe_colpat_olep = IPIPE_BLUE,
	.ipipe_colpat_elop = IPIPE_RED,
	.ipipe_colpat_elep = IPIPE_GREEN_RED,
	.ipipe_vst = 0,
	.ipipe_vsz = HEIGHT_I - 1,
	.ipipe_hst = 0,
	.ipipe_hsz = WIDTH_I - 1,
	.rsz_seq_seq = DISABLE,
	.rsz_seq_tmm = DISABLE,
	/* output confined mode (normal mode) */
	.rsz_seq_hrv = DISABLE,
	.rsz_seq_vrv = DISABLE,
	.rsz_seq_crv = DISABLE,
	.rsz_aal = DISABLE,
	.rsz_rsc_param = {
		{
			.rsz_mode = ONE_SHOT,
			.rsz_i_vst = 0,
			.rsz_i_vsz = 0,
			.rsz_i_hst = 0,
			.rsz_o_vsz = HEIGHT_O - 1,
			.rsz_o_hsz = WIDTH_O - 1,
			.rsz_o_hst = 0,
			.rsz_v_phs = 0,
			.rsz_v_dif = 243,
			.rsz_h_phs = 0,
			.rsz_h_dif = 243,
			.rsz_h_typ = RSZ_H_INTP_CUBIC,
			.rsz_h_lse_sel = RSZ_H_LPF_LSE_INTERN,
			.rsz_h_lpf = 0
		},
		{
			ONE_SHOT,
			0,
			0,
			0,
			239,
			319,
			0,
			0,
			256,
			0,
			256,
			RSZ_H_INTP_CUBIC,
			RSZ_H_LPF_LSE_INTERN,
			0
		}
	},
	.rsz2rgb = {
		{
			.rsz_rgb_en = DISABLE,
		},
		{
			DISABLE,
		}
	},
	.ext_mem_param = {
		{
			.rsz_sdr_bad_h = 0,
			.rsz_sdr_bad_l = 0,
			.rsz_sdr_sad_h = 0,
			.rsz_sdr_sad_l = 0,
			.rsz_sdr_oft = WIDTH_O * 2,
			.rsz_sdr_ptr_s = 0,
			.rsz_sdr_ptr_e = WIDTH_O
		},
		{
			0,
			0,
			0,
			0,
			WIDTH_O * 2,
			0,
			8191
		}
	},
	.rsz_en[0] = ENABLE,
	.rsz_en[1] = DISABLE
};

struct prev_single_shot_config dm355_prev_ss_config_defs = {
	.bypass = IPIPE_BYPASS_OFF,
	.input = {
		.image_width = WIDTH_I,
		.image_height = HEIGHT_I,
		.vst = 0,
		.hst = 0,
		.ppln = WIDTH_I + 8,
		.lpfr = HEIGHT_I + 10,
		.clk_div = IPIPEIF_DIVIDE_SIXTH,
		.data_shift = IPIPEIF_BITS9_0,
		.dec_en = 0,
		/* resize ratio 16/rsz */
		.rsz = 16,
		.avg_filter_en = AVG_OFF,
		.gain = 0x200,
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

struct prev_continuous_config dm355_prev_cont_config_defs = {
	.bypass = IPIPE_BYPASS_OFF,
	.input = {
		.en_df_sub = 0,
		.dec_en = 0,
		.rsz = 16,
		.avg_filter_en = AVG_OFF,
		.gain = 0x200,
		.colp_olop = IPIPE_GREEN_BLUE,
		.colp_olep = IPIPE_BLUE,
		.colp_elop = IPIPE_RED,
		.colp_elep = IPIPE_GREEN_RED
	}
};

struct rsz_single_shot_config dm355_rsz_ss_config_defs = {
	.input = {
		.image_width = WIDTH_I,
		.image_height = HEIGHT_I,
		.vst = 0,
		.hst = 0,
		.ppln = WIDTH_I + 8,
		.lpfr = HEIGHT_I + 10,
		.clk_div = IPIPEIF_DIVIDE_SIXTH,
		.dec_en = 0,
		/* resize ratio 16/rsz */
		.rsz = 16,
		.avg_filter_en = AVG_OFF,
		.pix_fmt = IPIPE_UYVY
	},
	.output1 = {
		.enable = 1,
		.pix_fmt = IPIPE_UYVY,
		.width = WIDTH_O,
		.height = HEIGHT_O,
		.vst = 0,
		.hst = 0,
		.h_intp_type = RSZ_H_INTP_CUBIC,
		.h_lpf_lse_sel = RSZ_H_LPF_LSE_INTERN,
		.lpf_user_val = 0
	},
	.output1 = {
		.enable = 1,
		.pix_fmt = IPIPE_UYVY,
		.width = WIDTH_O,
		.height = HEIGHT_O,
		.vst = 0,
		.hst = 0,
		.h_intp_type = RSZ_H_INTP_CUBIC,
		.h_lpf_lse_sel = RSZ_H_LPF_LSE_INTERN,
		.lpf_user_val = 0
	},
	.en_flip_vert = 0,
	.en_flip_horz = 0,
	.chroma_sample_even = 0,
	.en_vaaf = 0
};

struct rsz_continuous_config dm355_rsz_cont_config_defs = {
	.input = {
		.dec_en = 0,
		/* resize ratio 16/rsz */
		.rsz = 16,
		.avg_filter_en = AVG_OFF,
		.gain = 0x200
	},
	.en_output1 = 1,
	.output2 = {
		.enable = 0,
		.pix_fmt = IPIPE_UYVY,
		.width = WIDTH_O,
		.height = HEIGHT_O,
		.vst = 0,
		.hst = 0,
		.h_intp_type = RSZ_H_INTP_CUBIC,
		.h_lpf_lse_sel = RSZ_H_LPF_LSE_INTERN,
		.lpf_user_val = 0
	},
	.en_flip_vert = 0,
	.en_flip_horz = 0,
	.chroma_sample_even = 0,
	.en_vaaf = 0
};
