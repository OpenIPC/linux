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
 * Feature description
 * ===================
 *
 * VPFE hardware setup
 *
 * case 1: Capture to SDRAM with out IPIPE
 * ****************************************
 *
 *            parallel
 *                port
 *
 * Image sensor/       ________
 * Yuv decoder    ---->| CCDC |--> SDRAM
 *                     |______|
 *
 * case 2: Capture to SDRAM with IPIPE Preview modules in Continuous
 *          (On the Fly mode)
 *
 * Image sensor/       ________    ____________________
 * Yuv decoder    ---->| CCDC |--> | Previewer modules |--> SDRAM
 *                     |______|    |___________________|
 *
 * case 3: Capture to SDRAM with IPIPE Preview modules  & Resizer
 *         in continuous (On the Fly mode)
 *
 * Image sensor/       ________    _____________   ___________
 * Yuv decoder    ---->| CCDC |--> | Previewer  |->| Resizer  |-> SDRAM
 *                     |______|    |____________|  |__________|
 *
 * case 4: Capture to SDRAM with IPIPE Resizer
 *         in continuous (On the Fly mode)
 *
 * Image sensor/       ________    ___________
 * Yuv decoder    ---->| CCDC |--> | Resizer  |-> SDRAM
 *                     |______|    |__________|
 *
 * case 5: Read from SDRAM and do preview and/or Resize
 *         in Single shot mode
 *
 *                   _____________   ___________
 *    SDRAM   ----> | Previewer  |->| Resizer  |-> SDRAM
 *                  |____________|  |__________|
 *
 *
 * Previewer allows fine tuning of the input image using different
 * tuning modules in IPIPE. Some examples :- Noise filter, Defect
 * pixel correction etc. It essentially operate on Bayer Raw data
 * or YUV raw data. To do image tuning, application call,
 * PREV_QUERY_CAP, and then call PREV_SET_PARAM to set parameter
 * for a module.
 *
 *
 * Resizer allows upscaling or downscaling a image to a desired
 * resolution. There are 2 resizer modules. both operating on the
 * same input image, but can have different output resolution.
 */

#ifndef DM365_IPIPE_H
#define DM365_IPIPE_H

#include <media/davinci/dm3xx_ipipe.h>

/**********************************************************************
**      Previewer API Structures
**********************************************************************/

/* Previewer module IDs used in PREV_SET/GET_PARAM IOCTL. Some
 * modules can be also be updated during IPIPE operation. They are
 * marked as control ID
 */
/* LUT based Defect Pixel Correction */
#define PREV_LUTDPC		1
/* On the fly (OTF) Defect Pixel Correction */
#define PREV_OTFDPC		2
/* Noise Filter - 1 */
#define PREV_NF1 		3
/* Noise Filter - 2 */
#define PREV_NF2		4
/* White Balance.  Also a control ID */
#define PREV_WB			5
/* 1st RGB to RBG Blend module */
#define PREV_RGB2RGB_1		6
/* 2nd RGB to RBG Blend module */
#define PREV_RGB2RGB_2		7
/* Gamma Correction */
#define PREV_GAMMA		8
/* 3D LUT color conversion */
#define PREV_3D_LUT 		9
/* RGB to YCbCr module */
#define PREV_RGB2YUV 	   	10
/* YUV 422 conversion module */
#define PREV_YUV422_CONV   	11
/* Luminance Adjustment module.  Also a control ID */
#define PREV_LUM_ADJ	   	12
/* Edge Enhancement */
#define PREV_YEE           	13
/* Green Imbalance Correction */
#define PREV_GIC		14
/* CFA Interpolation */
#define PREV_CFA		15
/* Chroma Artifact Reduction */
#define PREV_CAR		16
/* Chroma Gain Suppression */
#define PREV_CGS		17
/* Global brighness and contrast control */
#define PREV_GBCE		18
/* Last module ID */
#define PREV_MAX_MODULES	18

struct ipipe_float_u16 {
	unsigned short integer;
	unsigned short decimal;
};

struct ipipe_float_s16 {
	short integer;
	unsigned short decimal;
};

struct ipipe_float_u8 {
	unsigned char integer;
	unsigned char decimal;
};

struct ipipe_win {
	/* vertical start line */
	unsigned int vst;
	/* horizontal start pixel */
	unsigned int hst;
	/* width */
	unsigned int width;
	/* height */
	unsigned int height;
};

/* Copy method selection for vertical correction
 *  Used when ipipe_dfc_corr_meth is PREV_DPC_CTORB_AFTER_HINT
 */
enum ipipe_dpc_corr_meth {
	/* replace by black or white dot specified by repl_white */
	IPIPE_DPC_REPL_BY_DOT = 0,
	/* Copy from left */
	IPIPE_DPC_CL,
	/* Copy from right */
	IPIPE_DPC_CR,
	/* Horizontal interpolation */
	IPIPE_DPC_H_INTP,
	/* Vertical interpolation */
	IPIPE_DPC_V_INTP,
	/* Copy from top  */
	IPIPE_DPC_CT,
	/* Copy from bottom */
	IPIPE_DPC_CB,
	/* 2D interpolation */
	IPIPE_DPC_2D_INTP,
};

struct ipipe_lutdpc_entry {
	/* Horizontal position */
	unsigned short horz_pos;
	/* vertical position */
	unsigned short vert_pos;
	enum ipipe_dpc_corr_meth method;
};

#define MAX_SIZE_DPC 256
/* Struct for configuring DPC module */
struct prev_lutdpc {
	/* 0 - disable, 1 - enable */
	unsigned char en;
	/* 0 - replace with black dot, 1 - white dot when correction
	 * method is  IPIPE_DFC_REPL_BY_DOT=0,
	 */
	unsigned char repl_white;
	/* number of entries in the correction table. Currently only
	 * support upto 256 entries. infinite mode is not supported
	 */
	unsigned short dpc_size;
	struct ipipe_lutdpc_entry *table;
};

enum ipipe_otfdpc_det_meth {
	IPIPE_DPC_OTF_MIN_MAX,
	IPIPE_DPC_OTF_MIN_MAX2
};

struct ipipe_otfdpc_thr {
	unsigned short r;
	unsigned short gr;
	unsigned short gb;
	unsigned short b;
};

enum ipipe_otfdpc_alg {
	IPIPE_OTFDPC_2_0,
	IPIPE_OTFDPC_3_0
};

struct prev_otfdpc_2_0 {
	/* defect detection threshold for MIN_MAX2 method  (DPC 2.0 alg) */
	struct ipipe_otfdpc_thr det_thr;
	/* defect correction threshold for MIN_MAX2 method (DPC 2.0 alg) or
	 * maximum value for MIN_MAX method
	 */
	struct ipipe_otfdpc_thr corr_thr;
};

struct prev_otfdpc_3_0 {
	/* DPC3.0 activity adj shf. activity = (max2-min2) >> (6 -shf)
	 */
	unsigned char act_adj_shf;
	/* DPC3.0 detection threshold, THR */
	unsigned short det_thr;
	/* DPC3.0 detection threshold slope, SLP */
	unsigned short det_slp;
	/* DPC3.0 detection threshold min, MIN */
	unsigned short det_thr_min;
	/* DPC3.0 detection threshold max, MAX */
	unsigned short det_thr_max;
	/* DPC3.0 correction threshold, THR */
	unsigned short corr_thr;
	/* DPC3.0 correction threshold slope, SLP */
	unsigned short corr_slp;
	/* DPC3.0 correction threshold min, MIN */
	unsigned short corr_thr_min;
	/* DPC3.0 correction threshold max, MAX */
	unsigned short corr_thr_max;
};

struct prev_otfdpc {
	/* 0 - disable, 1 - enable */
	unsigned char en;
	/* defect detection method */
	enum ipipe_otfdpc_det_meth det_method;
	/* Algorith used. Applicable only when IPIPE_DPC_OTF_MIN_MAX2 is
	 * used
	 */
	enum ipipe_otfdpc_alg alg;
	union {
		/* if alg is IPIPE_OTFDPC_2_0 */
		struct prev_otfdpc_2_0 dpc_2_0;
		/* if alg is IPIPE_OTFDPC_3_0 */
		struct prev_otfdpc_3_0 dpc_3_0;
	} alg_cfg;
};

/* Threshold values table size */
#define IPIPE_NF_THR_TABLE_SIZE 8
/* Intensity values table size */
#define IPIPE_NF_STR_TABLE_SIZE 8

/* NF, sampling method for green pixels */
enum ipipe_nf_sampl_meth {
	/* Same as R or B */
	IPIPE_NF_BOX,
	/* Diamond mode */
	IPIPE_NF_DIAMOND
};

/* Struct for configuring NF module */
struct prev_nf {
	/* 0 - disable, 1 - enable */
	unsigned char en;
	/* Sampling method for green pixels */
	enum ipipe_nf_sampl_meth gr_sample_meth;
	/* Down shift value in LUT reference address
	 */
	unsigned char shft_val;
	/* Spread value in NF algorithm
	 */
	unsigned char spread_val;
	/* Apply LSC gain to threshold. Enable this only if
	 * LSC is enabled in ISIF
	 */
	unsigned char apply_lsc_gain;
	/* Threshold values table */
	unsigned short thr[IPIPE_NF_THR_TABLE_SIZE];
	/* intensity values table */
	unsigned char str[IPIPE_NF_STR_TABLE_SIZE];
	/* Edge detection minimum threshold */
	unsigned short edge_det_min_thr;
	/* Edge detection maximum threshold */
	unsigned short edge_det_max_thr;
};

enum ipipe_gic_alg {
	IPIPE_GIC_ALG_CONST_GAIN,
	IPIPE_GIC_ALG_ADAPT_GAIN
};

enum ipipe_gic_thr_sel {
	IPIPE_GIC_THR_REG,
	IPIPE_GIC_THR_NF
};

enum ipipe_gic_wt_fn_type {
	/* Use difference as index */
	IPIPE_GIC_WT_FN_TYP_DIF,
	/* Use weight function as index */
	IPIPE_GIC_WT_FN_TYP_HP_VAL
};

/* structure for Green Imbalance Correction */
struct prev_gic {
	/* 0 - disable, 1 - enable */
	unsigned char en;
	/* 0 - Constant gain , 1 - Adaptive gain algorithm */
	enum ipipe_gic_alg gic_alg;
	/* GIC gain or weight. Used for Constant gain and Adaptive algorithms
	 */
	unsigned short gain;
	/* Threshold selection. GIC register values or NF2 thr table */
	enum ipipe_gic_thr_sel thr_sel;
	/* thr1. Used when thr_sel is  IPIPE_GIC_THR_REG */
	unsigned short thr;
	/* this value is used for thr2-thr1, thr3-thr2 or
	 * thr4-thr3 when wt_fn_type is index. Otherwise it
	 * is the
	 */
	unsigned short slope;
	/* Apply LSC gain to threshold. Enable this only if
	 * LSC is enabled in ISIF & thr_sel is IPIPE_GIC_THR_REG
	 */
	unsigned char apply_lsc_gain;
	/* Multiply Nf2 threshold by this gain. Use this when thr_sel
	 * is IPIPE_GIC_THR_NF
	 */
	struct ipipe_float_u8 nf2_thr_gain;
	/* Weight function uses difference as index or high pass value.
	 * Used for adaptive gain algorithm
	 */
	enum ipipe_gic_wt_fn_type wt_fn_type;
};

/* Struct for configuring WB module */
struct prev_wb {
	/* Offset (S12) for R */
	short ofst_r;
	/* Offset (S12) for Gr */
	short ofst_gr;
	/* Offset (S12) for Gb */
	short ofst_gb;
	/* Offset (S12) for B */
	short ofst_b;
	/* Gain (U13Q9) for Red */
	struct ipipe_float_u16 gain_r;
	/* Gain (U13Q9) for Gr */
	struct ipipe_float_u16 gain_gr;
	/* Gain (U13Q9) for Gb */
	struct ipipe_float_u16 gain_gb;
	/* Gain (U13Q9) for Blue */
	struct ipipe_float_u16 gain_b;
};

enum ipipe_cfa_alg {
	/* Algorithm is 2DirAC */
	IPIPE_CFA_ALG_2DIRAC,
	/* Algorithm is 2DirAC + Digital Antialiasing (DAA) */
	IPIPE_CFA_ALG_2DIRAC_DAA,
	/* Algorithm is DAA */
	IPIPE_CFA_ALG_DAA
};

/* Structure for CFA Interpolation */
struct prev_cfa {
	/* 2DirAC or 2DirAC + DAA */
	enum ipipe_cfa_alg alg;
	/* 2Dir CFA HP value Low Threshold */
	unsigned short hpf_thr_2dir;
	/* 2Dir CFA HP value slope */
	unsigned short hpf_slp_2dir;
	/* 2Dir CFA HP mix threshold */
	unsigned short hp_mix_thr_2dir;
	/* 2Dir CFA HP mix slope */
	unsigned short hp_mix_slope_2dir;
	/* 2Dir Direction threshold */
	unsigned short dir_thr_2dir;
	/* 2Dir Direction slope */
	unsigned short dir_slope_2dir;
	/* 2Dir NonDirectional Weight */
	unsigned short nd_wt_2dir;
	/* DAA Mono Hue Fraction */
	unsigned short hue_fract_daa;
	/* DAA Mono Edge threshold */
	unsigned short edge_thr_daa;
	/* DAA Mono threshold minimum */
	unsigned short thr_min_daa;
	/* DAA Mono threshold slope */
	unsigned short thr_slope_daa;
	/* DAA Mono slope minimum */
	unsigned short slope_min_daa;
	/* DAA Mono slope slope */
	unsigned short slope_slope_daa;
	/* DAA Mono LP wight */
	unsigned short lp_wt_daa;
};

/* Struct for configuring RGB2RGB blending module */
struct prev_rgb2rgb {
	/* Matrix coefficient for RR S12Q8 for ID = 1 and S11Q8 for ID = 2 */
	struct ipipe_float_s16 coef_rr;
	/* Matrix coefficient for GR S12Q8/S11Q8 */
	struct ipipe_float_s16 coef_gr;
	/* Matrix coefficient for BR S12Q8/S11Q8 */
	struct ipipe_float_s16 coef_br;
	/* Matrix coefficient for RG S12Q8/S11Q8 */
	struct ipipe_float_s16 coef_rg;
	/* Matrix coefficient for GG S12Q8/S11Q8 */
	struct ipipe_float_s16 coef_gg;
	/* Matrix coefficient for BG S12Q8/S11Q8 */
	struct ipipe_float_s16 coef_bg;
	/* Matrix coefficient for RB S12Q8/S11Q8 */
	struct ipipe_float_s16 coef_rb;
	/* Matrix coefficient for GB S12Q8/S11Q8 */
	struct ipipe_float_s16 coef_gb;
	/* Matrix coefficient for BB S12Q8/S11Q8 */
	struct ipipe_float_s16 coef_bb;
	/* Output offset for R S13/S11 */
	int out_ofst_r;
	/* Output offset for G S13/S11 */
	int out_ofst_g;
	/* Output offset for B S13/S11 */
	int out_ofst_b;
};

#define MAX_SIZE_GAMMA 512

enum ipipe_gamma_tbl_size {
	IPIPE_GAMMA_TBL_SZ_64,
	IPIPE_GAMMA_TBL_SZ_128,
	IPIPE_GAMMA_TBL_SZ_256,
	IPIPE_GAMMA_TBL_SZ_512
};

enum ipipe_gamma_tbl_sel {
	IPIPE_GAMMA_TBL_RAM,
	IPIPE_GAMMA_TBL_ROM
};

struct ipipe_gamma_entry {
	/* 10 bit slope */
	short slope;
	/* 10 bit offset */
	unsigned short offset;
};

/* Struct for configuring Gamma correction module */
struct prev_gamma {
	/* 0 - Enable Gamma correction for Red
	 * 1 - bypass Gamma correction. Data is divided by 16
	 */
	unsigned char bypass_r;
	/* 0 - Enable Gamma correction for Blue
	 * 1 - bypass Gamma correction. Data is divided by 16
	 */
	unsigned char bypass_b;
	/* 0 - Enable Gamma correction for Green
	 * 1 - bypass Gamma correction. Data is divided by 16
	 */
	unsigned char bypass_g;
	/* PREV_GAMMA_TBL_RAM or PREV_GAMMA_TBL_ROM */
	enum ipipe_gamma_tbl_sel tbl_sel;
	/* Table size for RAM gamma table.
	 */
	enum ipipe_gamma_tbl_size tbl_size;
	/* R table */
	struct ipipe_gamma_entry *table_r;
	/* Blue table */
	struct ipipe_gamma_entry *table_b;
	/* Green table */
	struct ipipe_gamma_entry *table_g;
};

#define MAX_SIZE_3D_LUT 	(729)

struct ipipe_3d_lut_entry {
	/* 10 bit entry for red */
	unsigned short r;
	/* 10 bit entry for green */
	unsigned short g;
	/* 10 bit entry for blue */
	unsigned short b;
};

/* structure for 3D-LUT */
struct prev_3d_lut {
	/* enable/disable 3D lut */
	unsigned char en;
	/* 3D - LUT table entry */
	struct ipipe_3d_lut_entry *table;
};

/* Struct for configuring Luminance Adjustment module */
struct prev_lum_adj {
	/* Brightness adjustments */
	unsigned char brightness;
	/* contrast adjustments */
	unsigned char contrast;
};

/* Struct for configuring rgb2ycbcr module */
struct prev_rgb2yuv {
	/* Matrix coefficient for RY S12Q8 */
	struct ipipe_float_s16 coef_ry;
	/* Matrix coefficient for GY S12Q8 */
	struct ipipe_float_s16 coef_gy;
	/* Matrix coefficient for BY S12Q8 */
	struct ipipe_float_s16 coef_by;
	/* Matrix coefficient for RCb S12Q8 */
	struct ipipe_float_s16 coef_rcb;
	/* Matrix coefficient for GCb S12Q8 */
	struct ipipe_float_s16 coef_gcb;
	/* Matrix coefficient for BCb S12Q8 */
	struct ipipe_float_s16 coef_bcb;
	/* Matrix coefficient for RCr S12Q8 */
	struct ipipe_float_s16 coef_rcr;
	/* Matrix coefficient for GCr S12Q8 */
	struct ipipe_float_s16 coef_gcr;
	/* Matrix coefficient for BCr S12Q8 */
	struct ipipe_float_s16 coef_bcr;
	/* Output offset for R S11 */
	int out_ofst_y;
	/* Output offset for Cb S11 */
	int out_ofst_cb;
	/* Output offset for Cr S11 */
	int out_ofst_cr;
};

enum ipipe_gbce_type {
	IPIPE_GBCE_Y_VAL_TBL,
	IPIPE_GBCE_GAIN_TBL
};

#define MAX_SIZE_GBCE_LUT 1024

/* structure for Global brighness and Contrast */
struct prev_gbce {
	/* enable/disable GBCE */
	unsigned char en;
	/* Y - value table or Gain table */
	enum ipipe_gbce_type type;
	/* ptr to LUT for GBCE with 1024 entries */
	unsigned short *table;
};

/* Chrominance position. Applicable only for YCbCr input
 * Applied after edge enhancement
 */
enum ipipe_chr_pos {
	/* Cositing, same position with luminance */
	IPIPE_YUV422_CHR_POS_COSITE,
	/* Centering, In the middle of luminance */
	IPIPE_YUV422_CHR_POS_CENTRE
};

/* Struct for configuring yuv422 conversion module */
struct prev_yuv422_conv {
	/* Max Chrominance value */
	unsigned char en_chrom_lpf;
	/* 1 - enable LPF for chrminance, 0 - disable */
	enum ipipe_chr_pos chrom_pos;
};

#define MAX_SIZE_YEE_LUT 1024

enum ipipe_yee_merge_meth {
	IPIPE_YEE_ABS_MAX,
	IPIPE_YEE_EE_ES
};

/* Struct for configuring YUV Edge Enhancement module */
struct prev_yee {
	/* 1 - enable enhancement, 0 - disable */
	unsigned char en;
	/* enable/disable halo reduction in edge sharpner */
	unsigned char en_halo_red;
	/* Merge method between Edge Enhancer and Edge sharpner */
	enum ipipe_yee_merge_meth merge_meth;
	/* HPF Shift length */
	unsigned char hpf_shft;
	/* HPF Coefficient 00, S10 */
	short hpf_coef_00;
	/* HPF Coefficient 01, S10 */
	short hpf_coef_01;
	/* HPF Coefficient 02, S10 */
	short hpf_coef_02;
	/* HPF Coefficient 10, S10 */
	short hpf_coef_10;
	/* HPF Coefficient 11, S10 */
	short hpf_coef_11;
	/* HPF Coefficient 12, S10 */
	short hpf_coef_12;
	/* HPF Coefficient 20, S10 */
	short hpf_coef_20;
	/* HPF Coefficient 21, S10 */
	short hpf_coef_21;
	/* HPF Coefficient 22, S10 */
	short hpf_coef_22;
	/* Lower threshold before refering to LUT */
	unsigned short yee_thr;
	/* Edge sharpener Gain */
	unsigned short es_gain;
	/* Edge sharpener lowe threshold */
	unsigned short es_thr1;
	/* Edge sharpener upper threshold */
	unsigned short es_thr2;
	/* Edge sharpener gain on gradient */
	unsigned short es_gain_grad;
	/* Edge sharpener offset on gradient */
	unsigned short es_ofst_grad;
	/* Ptr to EE table. Must have 1024 entries */
	short *table;
};

enum ipipe_car_meth {
	/* Chromatic Gain Control */
	IPIPE_CAR_CHR_GAIN_CTRL,
	/* Dynamic switching between CHR_GAIN_CTRL
	 * and MED_FLTR
	 */
	IPIPE_CAR_DYN_SWITCH,
	/* Median Filter */
	IPIPE_CAR_MED_FLTR
};

enum ipipe_car_hpf_type {
	IPIPE_CAR_HPF_Y,
	IPIPE_CAR_HPF_H,
	IPIPE_CAR_HPF_V,
	IPIPE_CAR_HPF_2D,
	/* 2D HPF from YUV Edge Enhancement */
	IPIPE_CAR_HPF_2D_YEE
};

struct ipipe_car_gain {
	/* csup_gain */
	unsigned char gain;
	/* csup_shf. */
	unsigned char shft;
	/* gain minimum */
	unsigned short gain_min;
};

/* Structure for Chromatic Artifact Reduction */
struct prev_car {
	/* enable/disable */
	unsigned char en;
	/* Gain control or Dynamic switching */
	enum ipipe_car_meth meth;
	/* Gain1 function configuration for Gain control */
	struct ipipe_car_gain gain1;
	/* Gain2 function configuration for Gain control */
	struct ipipe_car_gain gain2;
	/* HPF type used for CAR */
	enum ipipe_car_hpf_type hpf;
	/* csup_thr: HPF threshold for Gain control */
	unsigned char hpf_thr;
	/* Down shift value for hpf. 2 bits */
	unsigned char hpf_shft;
	/* switch limit for median filter */
	unsigned char sw0;
	/* switch coefficient for Gain control */
	unsigned char sw1;
};

/* structure for Chromatic Gain Suppression */
struct prev_cgs {
	/* enable/disable */
	unsigned char en;
	/* gain1 bright side threshold */
	unsigned char h_thr;
	/* gain1 bright side slope */
	unsigned char h_slope;
	/* gain1 down shift value for bright side */
	unsigned char h_shft;
	/* gain1 bright side minimum gain */
	unsigned char h_min;
};

/* various pixel formats supported */
enum ipipe_pix_formats {
	IPIPE_BAYER_8BIT_PACK,
	IPIPE_BAYER_8BIT_PACK_ALAW,
	IPIPE_BAYER_8BIT_PACK_DPCM,
	IPIPE_BAYER_12BIT_PACK,
	IPIPE_BAYER,		/* 16 bit */
	IPIPE_UYVY,
	IPIPE_YUYV,
	IPIPE_RGB565,
	IPIPE_RGB888,
	IPIPE_YUV420SP,
	IPIPE_420SP_Y,
	IPIPE_420SP_C
};

enum ipipe_dpaths_bypass_t {
	IPIPE_BYPASS_OFF,
	IPIPE_BYPASS_ON
};

enum ipipe_colpat_t {
	IPIPE_RED,
	IPIPE_GREEN_RED,
	IPIPE_GREEN_BLUE,
	IPIPE_BLUE
};

enum down_scale_ave_sz {
	IPIPE_DWN_SCALE_1_OVER_2,
	IPIPE_DWN_SCALE_1_OVER_4,
	IPIPE_DWN_SCALE_1_OVER_8,
	IPIPE_DWN_SCALE_1_OVER_16,
	IPIPE_DWN_SCALE_1_OVER_32,
	IPIPE_DWN_SCALE_1_OVER_64,
	IPIPE_DWN_SCALE_1_OVER_128,
	IPIPE_DWN_SCALE_1_OVER_256
};

/* Max pixels allowed in the input. If above this either decimation
 * or frame division mode to be enabled
 */
#define IPIPE_MAX_INPUT_WIDTH 2600

/* Max pixels in resizer - A output. In downscale
 * (DSCALE) mode, image quality is better, but has lesser
 * maximum width allowed
 */
#define IPIPE_MAX_OUTPUT1_WIDTH_NORMAL 2176
#define IPIPE_MAX_OUTPUT1_WIDTH_DSCALE 1088

/* Max pixels in resizer - B output. In downscale
 * (DSCALE) mode, image quality is better, but has lesser
 * maximum width allowed
 */
#define IPIPE_MAX_OUTPUT2_WIDTH_NORMAL 1088
#define IPIPE_MAX_OUTPUT2_WIDTH_DSCALE 544

/* Structure for configuring Single Shot mode in the previewer
 *   channel
 */
struct prev_ss_input_spec {
	/* width of the image in SDRAM. */
	unsigned int image_width;
	/* height of the image in SDRAM */
	unsigned int image_height;
	/* line length. This will allow application to set a
	 * different line length than that calculated based on
	 * width. Set it to zero, if not used,
	 */
	unsigned int line_length;
	/* vertical start position of the image
	 * data to IPIPE
	 */
	unsigned int vst;
	/* horizontal start position of the image
	 * data to IPIPE
	 */
	unsigned int hst;
	/* Global frame HD rate */
	unsigned int ppln;
	/* Global frame VD rate */
	unsigned int lpfr;
	/* dpcm predicator selection */
	enum ipipeif_dpcm_pred pred;
	/* clock divide to bring down the pixel clock */
	struct ipipeif_5_1_clkdiv clk_div;
	/* Shift data as per image sensor capture format
	 * only applicable for RAW Bayer inputs
	 */
	enum ipipeif_5_1_data_shift data_shift;
	/* Enable decimation 1 - enable, 0 - disable
	 * This is used when image width is greater than
	 * ipipe line buffer size
	 */
	enum ipipeif_decimation dec_en;
	/* used when en_dec = 1. Resize ratio for decimation
	 * when frame size is  greater than what hw can handle.
	 * 16 to 112. IPIPE input width is calculated as follows.
	 * width = image_width * 16/ipipeif_rsz. For example
	 * if image_width is 1920 and user want to scale it down
	 * to 1280, use ipipeif_rsz = 24. 1920*16/24 = 1280
	 */
	unsigned char rsz;
	/* When input image width is greater that line buffer
	 * size, use this to do resize using frame division. The
	 * frame is divided into two vertical slices and resize
	 * is performed on each slice. Use either frame division
	 *  mode or decimation, NOT both
	 */
	unsigned char frame_div_mode_en;
	/* Enable/Disable avg filter at IPIPEIF.
	 * 1 - enable, 0 - disable
	 */
	unsigned char avg_filter_en;
	/* Simple defect pixel correction based on a threshold value */
	struct ipipeif_dpc dpc;
	/* gain applied to the ipipeif output */
	unsigned short gain;
	/* clipped to this value at the ipipeif */
	unsigned short clip;
	/* Align HSync and VSync to rsz_start */
	unsigned char align_sync;
	/* ipipeif resize start position */
	unsigned int rsz_start;
	/* Input pixels formats
	 */
	enum ipipe_pix_formats pix_fmt;
	/* pix order for YUV */
	enum ipipeif_pixel_order pix_order;
	/* Color pattern for odd line, odd pixel */
	enum ipipe_colpat_t colp_olop;
	/* Color pattern for odd line, even pixel */
	enum ipipe_colpat_t colp_olep;
	/* Color pattern for even line, odd pixel */
	enum ipipe_colpat_t colp_elop;
	/* Color pattern for even line, even pixel */
	enum ipipe_colpat_t colp_elep;
};

struct prev_ss_output_spec {
	/* output pixel format */
	enum ipipe_pix_formats pix_fmt;
};

struct prev_single_shot_config {
	/* Bypass image processing. RAW -> RAW */
	enum ipipe_dpaths_bypass_t bypass;
	/* Input specification for the image data */
	struct prev_ss_input_spec input;
	/* Output specification for the image data */
	struct prev_ss_output_spec output;
};

struct prev_cont_input_spec {
	/* 1 - enable, 0 - disable df subtraction */
	unsigned char en_df_sub;
	/* DF gain enable */
	unsigned char en_df_gain;
	/* DF gain value */
	unsigned int df_gain;
	/* DF gain threshold value */
	unsigned short df_gain_thr;
	/* Enable decimation 1 - enable, 0 - disable
	 * This is used for bringing down the line size
	 * to that supported by IPIPE. DM355 IPIPE
	 * can process only 1344 pixels per line.
	 */
	enum ipipeif_decimation dec_en;
	/* used when en_dec = 1. Resize ratio for decimation
	 * when frame size is  greater than what hw can handle.
	 * 16 to 112. IPIPE input width is calculated as follows.
	 * width = image_width * 16/ipipeif_rsz. For example
	 * if image_width is 1920 and user want to scale it down
	 * to 1280, use ipipeif_rsz = 24. 1920*16/24 = 1280
	 */
	unsigned char rsz;
	/* Enable/Disable avg filter at IPIPEIF.
	 * 1 - enable, 0 - disable
	 */
	unsigned char avg_filter_en;
	/* Gain applied at IPIPEIF. 1 - 1023. divided by 512.
	 * So can be from 1/512 to  1/1023.
	 */
	unsigned short gain;
	/* clipped to this value at the output of IPIPEIF */
	unsigned short clip;
	/* Align HSync and VSync to rsz_start */
	unsigned char align_sync;
	/* ipipeif resize start position */
	unsigned int rsz_start;
	/* Simple defect pixel correction based on a threshold value */
	struct ipipeif_dpc dpc;
	/* Color pattern for odd line, odd pixel */
	enum ipipe_colpat_t colp_olop;
	/* Color pattern for odd line, even pixel */
	enum ipipe_colpat_t colp_olep;
	/* Color pattern for even line, odd pixel */
	enum ipipe_colpat_t colp_elop;
	/* Color pattern for even line, even pixel */
	enum ipipe_colpat_t colp_elep;
};

/* Structure for configuring Continuous mode in the previewer
 * channel . In continuous mode, only following parameters are
 * available for configuration from user. Rest are configured
 * through S_CROP and S_FMT IOCTLs in CCDC driver. In this mode
 * data to IPIPEIF comes from CCDC
 */
struct prev_continuous_config {
	/* Bypass image processing. RAW -> RAW */
	enum ipipe_dpaths_bypass_t bypass;
	/* Input specification for the image data */
	struct prev_cont_input_spec input;
};

/*******************************************************************
**  Resizer API structures
*******************************************************************/
/* Interpolation types used for horizontal rescale */
enum rsz_intp_t {
	RSZ_INTP_CUBIC,
	RSZ_INTP_LINEAR
};

/* Horizontal LPF intensity selection */
enum rsz_h_lpf_lse_t {
	RSZ_H_LPF_LSE_INTERN,
	RSZ_H_LPF_LSE_USER_VAL
};

/* Structure for configuring resizer in single shot mode.
 * This structure is used when operation mode of the
 * resizer is single shot. The related IOCTL is
 * RSZ_S_CONFIG & RSZ_G_CONFIG. When chained, data to
 * resizer comes from previewer. When not chained, only
 * UYVY data input is allowed for resizer operation.
 * To operate on RAW Bayer data from CCDC, chain resizer
 * with previewer by setting chain field in the
 * rsz_channel_config structure.
 */

struct rsz_ss_input_spec {
	/* width of the image in SDRAM. */
	unsigned int image_width;
	/* height of the image in SDRAM */
	unsigned int image_height;
	/* line length. This will allow application to set a
	 * different line length than that calculated based on
	 * width. Set it to zero, if not used,
	 */
	unsigned int line_length;
	/* vertical start position of the image
	 * data to IPIPE
	 */
	unsigned int vst;
	/* horizontal start position of the image
	 * data to IPIPE
	 */
	unsigned int hst;
	/* Global frame HD rate */
	unsigned int ppln;
	/* Global frame VD rate */
	unsigned int lpfr;
	/* clock divide to bring down the pixel clock */
	struct ipipeif_5_1_clkdiv clk_div;
	/* Enable decimation 1 - enable, 0 - disable.
	 * Used when input image width is greater than ipipe
	 * line buffer size, this is enabled to do resize
	 * at the input of the IPIPE to clip the size
	 */
	enum ipipeif_decimation dec_en;
	/* used when en_dec = 1. Resize ratio for decimation
	 * when frame size is  greater than what hw can handle.
	 * 16 to 112. IPIPE input width is calculated as follows.
	 * width = image_width * 16/ipipeif_rsz. For example
	 * if image_width is 1920 and user want to scale it down
	 * to 1280, use ipipeif_rsz = 24. 1920*16/24 = 1280
	 */
	unsigned char rsz;
	/* When input image width is greater that line buffer
	 * size, use this to do resize using frame division. The
	 * frame is divided into two vertical slices and resize
	 * is performed on each slice
	 */
	unsigned char frame_div_mode_en;
	/* Enable/Disable avg filter at IPIPEIF.
	 * 1 - enable, 0 - disable
	 */
	unsigned char avg_filter_en;
	/* Align HSync and VSync to rsz_start */
	unsigned char align_sync;
	/* ipipeif resize start position */
	unsigned int rsz_start;
	/* Input pixels formats
	 */
	enum ipipe_pix_formats pix_fmt;
};

struct rsz_output_spec {
	/* enable the resizer output */
	unsigned char enable;
	/* output pixel format. Has to be UYVY */
	enum ipipe_pix_formats pix_fmt;
	/* enable horizontal flip */
	unsigned char h_flip;
	/* enable vertical flip */
	unsigned char v_flip;
	/* width in pixels. must be multiple of 16. */
	unsigned int width;
	/* height in lines */
	unsigned int height;
	/* line start offset for y. */
	unsigned int vst_y;
	/* line start offset for c. Only for 420 */
	unsigned int vst_c;
	/* vertical rescale interpolation type, YCbCr or Luminance */
	enum rsz_intp_t v_typ_y;
	/* vertical rescale interpolation type for Chrominance */
	enum rsz_intp_t v_typ_c;
	/* vertical lpf intensity - Luminance */
	unsigned char v_lpf_int_y;
	/* vertical lpf intensity - Chrominance */
	unsigned char v_lpf_int_c;
	/* horizontal rescale interpolation types, YCbCr or Luminance  */
	enum rsz_intp_t h_typ_y;
	/* horizontal rescale interpolation types, Chrominance */
	enum rsz_intp_t h_typ_c;
	/* horizontal lpf intensity - Luminance */
	unsigned char h_lpf_int_y;
	/* horizontal lpf intensity - Chrominance */
	unsigned char h_lpf_int_c;
	/* Use down scale mode for scale down */
	unsigned char en_down_scale;
	/* if downscale, set the downscale more average size for horizontal
	 * direction. Used only if output width and height is less than
	 * input sizes
	 */
	enum down_scale_ave_sz h_dscale_ave_sz;
	/* if downscale, set the downscale more average size for vertical
	 * direction. Used only if output width and height is less than
	 * input sizes
	 */
	enum down_scale_ave_sz v_dscale_ave_sz;
	/* Y offset. If set, the offset would be added to the base address
	 */
	unsigned int user_y_ofst;
	/* C offset. If set, the offset would be added to the base address
	 */
	unsigned int user_c_ofst;
};

/* In continuous mode, few parameters are set by ccdc driver. So only
 * part of the output spec is available for user configuration
 */
struct rsz_part_output_spec {
	/* enable the resizer output */
	unsigned char enable;
	/* enable horizontal flip */
	unsigned char h_flip;
	/* vertical rescale interpolation type, YCbCr or Luminance */
	unsigned char v_flip;
	/* vertical rescale interpolation type for Chrominance */
	enum rsz_intp_t v_typ_y;
	/* vertical rescale interpolation types  */
	enum rsz_intp_t v_typ_c;
	/* vertical lpf intensity - Luminance */
	unsigned char v_lpf_int_y;
	/* horizontal rescale interpolation types, YCbCr or Luminance  */
	unsigned char v_lpf_int_c;
	/* horizontal rescale interpolation types, Chrominance */
	enum rsz_intp_t h_typ_y;
	/* vertical lpf intensity - Chrominance */
	enum rsz_intp_t h_typ_c;
	/* horizontal lpf intensity - Luminance */
	unsigned char h_lpf_int_y;
	/* Use down scale mode for scale down */
	unsigned char h_lpf_int_c;
	/* horizontal lpf intensity - Chrominance */
	unsigned char en_down_scale;
	/* if downscale, set the downscale more average size for horizontal
	 * direction. Used only if output width and height is less than
	 * input sizes
	 */
	enum down_scale_ave_sz h_dscale_ave_sz;
	/* if downscale, set the downscale more average size for vertical
	 * direction. Used only if output width and height is less than
	 * input sizes
	 */
	enum down_scale_ave_sz v_dscale_ave_sz;
	/* Y offset. If set, the offset would be added to the base address
	 */
	unsigned int user_y_ofst;
	/* C offset. If set, the offset would be added to the base address
	 */
	unsigned int user_c_ofst;
};

struct rsz_single_shot_config {
	/* input spec of the image data (UYVY). non-chained
	 * mode. Only valid when not chained. For chained
	 * operation, previewer settings are used
	 */
	struct rsz_ss_input_spec input;
	/* output spec of the image data coming out of resizer - 0(UYVY).
	 */
	struct rsz_output_spec output1;
	/* output spec of the image data coming out of resizer - 1(UYVY).
	 */
	struct rsz_output_spec output2;
	/* 0 , chroma sample at odd pixel, 1 - even pixel */
	unsigned char chroma_sample_even;
	unsigned char yuv_y_min;
	unsigned char yuv_y_max;
	unsigned char yuv_c_min;
	unsigned char yuv_c_max;
	enum ipipe_chr_pos out_chr_pos;
};

struct rsz_continuous_config {
	/* A subset of output spec is configured by application.
	 * Others such as size, position etc are set by CCDC driver
	 */
	struct rsz_part_output_spec output1;
	struct rsz_output_spec output2;
	/* output spec of the image data coming out of resizer - 1(UYVY).
	 */
	unsigned char chroma_sample_even;
	/* 0 , chroma sample at odd pixel, 1 - even pixel */
	unsigned char yuv_y_min;
	unsigned char yuv_y_max;
	unsigned char yuv_c_min;
	unsigned char yuv_c_max;
	enum ipipe_chr_pos out_chr_pos;
};

#ifdef __KERNEL__
#include <media/davinci/imp_common.h>

/* Used for driver storage */
struct ipipe_otfdpc_2_0 {
	/* 0 - disable, 1 - enable */
	unsigned char en;
	/* defect detection method */
	enum ipipe_otfdpc_det_meth det_method;
	/* Algorith used. Applicable only when IPIPE_DPC_OTF_MIN_MAX2 is
	 * used
	 */
	enum ipipe_otfdpc_alg alg;
	struct prev_otfdpc_2_0 otfdpc_2_0;
};

struct ipipe_otfdpc_3_0 {
	/* 0 - disable, 1 - enable */
	unsigned char en;
	/* defect detection method */
	enum ipipe_otfdpc_det_meth det_method;
	/* Algorith used. Applicable only when IPIPE_DPC_OTF_MIN_MAX2 is
	 * used
	 */
	enum ipipe_otfdpc_alg alg;
	struct prev_otfdpc_3_0 otfdpc_3_0;
};

enum enable_disable_t {
	DISABLE,
	ENABLE
};

#define CEIL(a, b)	(((a) + (b-1)) / (b))
#define IPIPE_MAX_PASSES	2

struct f_div_pass {
	unsigned int o_hsz;
	unsigned int i_hps;
	unsigned int h_phs;
	unsigned int src_hps;
	unsigned int src_hsz;
};

struct f_div_param {
	unsigned char en;
	unsigned int num_passes;
	struct f_div_pass pass[IPIPE_MAX_PASSES];
};

/* Resizer Rescale Parameters*/
struct ipipe_rsz_rescale_param {
	enum ipipe_oper_mode mode;
	enum enable_disable_t h_flip;
	enum enable_disable_t v_flip;
	enum enable_disable_t cen;
	enum enable_disable_t yen;
	unsigned short i_vps;
	unsigned short i_hps;
	unsigned short o_vsz;
	unsigned short o_hsz;
	unsigned short v_phs_y;
	unsigned short v_phs_c;
	unsigned short v_dif;
	/* resize method - Luminance */
	enum rsz_intp_t v_typ_y;
	/* resize method - Chrominance */
	enum rsz_intp_t v_typ_c;
	/* vertical lpf intensity - Luminance */
	unsigned char v_lpf_int_y;
	/* vertical lpf intensity - Chrominance */
	unsigned char v_lpf_int_c;
	unsigned short h_phs;
	unsigned short h_dif;
	/* resize method - Luminance */
	enum rsz_intp_t h_typ_y;
	/* resize method - Chrominance */
	enum rsz_intp_t h_typ_c;
	/* horizontal lpf intensity - Luminance */
	unsigned char h_lpf_int_y;
	/* horizontal lpf intensity - Chrominance */
	unsigned char h_lpf_int_c;
	enum enable_disable_t dscale_en;
	enum down_scale_ave_sz h_dscale_ave_sz;
	enum down_scale_ave_sz v_dscale_ave_sz;
	/* store the calculated frame division parameter */
	struct f_div_param f_div;
};

enum ipipe_rsz_rgb_t {
	OUTPUT_32BIT,
	OUTPUT_16BIT
};

enum ipipe_rsz_rgb_msk_t {
	NOMASK,
	MASKLAST2
};

/* Resizer RGB Conversion Parameters */
struct ipipe_rsz_resize2rgb {
	enum enable_disable_t rgb_en;
	enum ipipe_rsz_rgb_t rgb_typ;
	enum ipipe_rsz_rgb_msk_t rgb_msk0;
	enum ipipe_rsz_rgb_msk_t rgb_msk1;
	unsigned int rgb_alpha_val;
};

/* Resizer External Memory Parameters */
struct ipipe_ext_mem_param {
	unsigned int rsz_sdr_oft_y;
	unsigned int rsz_sdr_ptr_s_y;
	unsigned int rsz_sdr_ptr_e_y;
	unsigned int rsz_sdr_oft_c;
	unsigned int rsz_sdr_ptr_s_c;
	unsigned int rsz_sdr_ptr_e_c;
	/* offset to be added to buffer start when flipping for y/ycbcr */
	unsigned int flip_ofst_y;
	/* offset to be added to buffer start when flipping for c */
	unsigned int flip_ofst_c;
	/* c offset for YUV 420SP */
	unsigned int c_offset;
	/* User Defined Y offset for YUV 420SP or YUV420ILE data */
	unsigned int user_y_ofst;
	/* User Defined C offset for YUV 420SP data */
	unsigned int user_c_ofst;
};

enum rsz_data_source {
	IPIPE_DATA,
	IPIPEIF_DATA
};

/* data paths */
enum ipipe_data_paths {
	IPIPE_RAW2YUV,
	/* Bayer RAW input to YCbCr output */
	IPIPE_RAW2RAW,
	/* Bayer Raw to Bayer output */
	IPIPE_RAW2BOX,
	/* Bayer Raw to Boxcar output */
	IPIPE_YUV2YUV
	/* YUV Raw to YUV Raw output */
};

enum rsz_src_img_fmt {
	RSZ_IMG_422,
	RSZ_IMG_420
};

struct rsz_common_params {
	unsigned int vps;
	unsigned int vsz;
	unsigned int hps;
	unsigned int hsz;
	/* 420 or 422 */
	enum rsz_src_img_fmt src_img_fmt;
	/* Y or C when src_fmt is 420, 0 - y, 1 - c */
	unsigned char y_c;
	/* flip raw or ycbcr */
	unsigned char raw_flip;
	/* IPIPE or IPIPEIF data */
	enum rsz_data_source source;
	enum ipipe_dpaths_bypass_t passthrough;
	unsigned char yuv_y_min;
	unsigned char yuv_y_max;
	unsigned char yuv_c_min;
	unsigned char yuv_c_max;
	enum enable_disable_t rsz_seq_crv;
	enum ipipe_chr_pos out_chr_pos;
};

struct ipipe_params {
	struct ipipeif ipipeif_param;
	enum ipipe_oper_mode ipipe_mode;
	/* input/output datapath through IPIPE */
	enum ipipe_data_paths ipipe_dpaths_fmt;
	/* color pattern register */
	enum ipipe_colpat_t ipipe_colpat_elep;
	enum ipipe_colpat_t ipipe_colpat_elop;
	enum ipipe_colpat_t ipipe_colpat_olep;
	enum ipipe_colpat_t ipipe_colpat_olop;
	/* horizontal/vertical start, horizontal/vertical size
	 * for both IPIPE and RSZ input
	 */
	unsigned int ipipe_vps;
	unsigned int ipipe_vsz;
	unsigned int ipipe_hps;
	unsigned int ipipe_hsz;

	struct rsz_common_params rsz_common;
	struct ipipe_rsz_rescale_param rsz_rsc_param[2];
	struct ipipe_rsz_resize2rgb rsz2rgb[2];
	struct ipipe_ext_mem_param ext_mem_param[2];
	enum enable_disable_t rsz_en[2];

};

void ipipe_hw_dump_config(void);
int ipipe_hw_setup(struct ipipe_params *config);
int ipipe_set_lutdpc_regs(struct prev_lutdpc *lutdpc);
int ipipe_set_otfdpc_regs(struct prev_otfdpc *otfdpc);
int ipipe_set_d2f_regs(unsigned int id, struct prev_nf *noise_filter);
int ipipe_set_wb_regs(struct prev_wb *wb);
int ipipe_set_gic_regs(struct prev_gic *gic);
int ipipe_set_cfa_regs(struct prev_cfa *cfa);
int ipipe_set_rgb2rgb_regs(unsigned int id, struct prev_rgb2rgb *rgb);
int ipipe_set_gamma_regs(struct prev_gamma *gamma);
int ipipe_set_3d_lut_regs(struct prev_3d_lut *lut_3d);
int ipipe_set_lum_adj_regs(struct prev_lum_adj *lum_adj);
int ipipe_set_rgb2ycbcr_regs(struct prev_rgb2yuv *yuv);
int ipipe_set_yuv422_conv_regs(struct prev_yuv422_conv *conv);
int ipipe_set_gbce_regs(struct prev_gbce *gbce);
int ipipe_set_ee_regs(struct prev_yee *ee);
int ipipe_set_car_regs(struct prev_car *car);
int ipipe_set_cgs_regs(struct prev_cgs *cgs);
int rsz_enable(int rsz_id, int enable);
void rsz_src_enable(int enable);
int rsz_set_output_address(struct ipipe_params *params,
			      int resize_no, unsigned int address);
int rsz_set_in_pix_format(unsigned char y_c);
#ifdef CONFIG_VIDEO_YCBCR
int ipipe_hw_set_ipipeif_addr(struct ipipe_params *config, unsigned int address);
#endif
#endif
#endif
