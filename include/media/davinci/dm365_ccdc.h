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
 *
 * ccdc header file for DM365 ISIF
 */

#ifndef _DM365_CCDC_H
#define _DM365_CCDC_H
#include <media/davinci/ccdc_types.h>
#include <media/davinci/vpfe_types.h>
/**
 * ccdc float type S8Q8/U8Q8
 */
struct ccdc_float_8 {
	/* 8 bit integer part */
	unsigned char integer;
	/* 8 bit decimal part */
	unsigned char decimal;
};

/**
 * brief ccdc float type U16Q16/S16Q16
 */
struct ccdc_float_16 {
	/* 16 bit integer part */
	unsigned short integer;
	/* 16 bit decimal part */
	unsigned short decimal;
};

/*
 * ccdc image(target) window parameters
 */
struct ccdc_cropwin {
	/* horzontal offset of the top left corner in pixels */
	unsigned int left;
	/* vertical offset of the top left corner in pixels */
	unsigned int top;
	/* width in pixels of the rectangle */
	unsigned int width;
	/* height in lines of the rectangle */
	unsigned int height;
};

/************************************************************************
 *   Vertical Defect Correction parameters
 ***********************************************************************/

/**
 * vertical defect correction methods
 */
enum ccdc_vdfc_corr_mode {
	/* Defect level subtraction. Just fed through if saturating */
	CCDC_VDFC_NORMAL,
	/**
	 * Defect level subtraction. Horizontal interpolation ((i-2)+(i+2))/2
	 * if data saturating
	 */
	CCDC_VDFC_HORZ_INTERPOL_IF_SAT,
	/* Horizontal interpolation (((i-2)+(i+2))/2) */
	CCDC_VDFC_HORZ_INTERPOL
};

/**
 * Max Size of the Vertical Defect Correction table
 */
#define CCDC_VDFC_TABLE_SIZE 8

/**
 * Values used for shifting up the vdfc defect level
 */
enum ccdc_vdfc_shift {
	/* No Shift */
	CCDC_VDFC_NO_SHIFT,
	/* Shift by 1 bit */
	CCDC_VDFC_SHIFT_1,
	/* Shift by 2 bit */
	CCDC_VDFC_SHIFT_2,
	/* Shift by 3 bit */
	CCDC_VDFC_SHIFT_3,
	/* Shift by 4 bit */
	CCDC_VDFC_SHIFT_4
};

/**
 * Defect Correction (DFC) table entry
 */
struct ccdc_vdfc_entry {
	/* vertical position of defect */
	unsigned short pos_vert;
	/* horizontal position of defect */
	unsigned short pos_horz;
	/**
	 * Defect level of Vertical line defect position. This is subtracted
	 * from the data at the defect position
	 */
	unsigned char level_at_pos;
	/**
	 * Defect level of the pixels upper than the vertical line defect.
	 * This is subtracted from the data
	 */
	unsigned char level_up_pixels;
	/**
	 * Defect level of the pixels lower than the vertical line defect.
	 * This is subtracted from the data
	 */
	unsigned char level_low_pixels;
};

/**
 * Structure for Defect Correction (DFC) parameter
 */
struct ccdc_dfc {
	/* enable vertical defect correction */
	unsigned char en;
	/* Correction methods */
	enum ccdc_vdfc_corr_mode corr_mode;
	/**
	 * 0 - whole line corrected, 1 - not
	 * pixels upper than the defect
	 */
	unsigned char corr_whole_line;
	/**
	 * defect level shift value. level_at_pos, level_upper_pos,
	 * and level_lower_pos can be shifted up by this value
	 */
	enum ccdc_vdfc_shift def_level_shift;
	/* defect saturation level */
	unsigned short def_sat_level;
	/* number of vertical defects. Max is CCDC_VDFC_TABLE_SIZE */
	short num_vdefects;
	/* VDFC table ptr */
	struct ccdc_vdfc_entry table[CCDC_VDFC_TABLE_SIZE];
};

/************************************************************************
*   Digital/Black clamp or DC Subtract parameters
************************************************************************/
/**
 * Horizontal Black Clamp modes
 */
enum ccdc_horz_bc_mode {
	/**
	 * Horizontal clamp disabled. Only vertical clamp
	 * value is subtracted
	 */
	CCDC_HORZ_BC_DISABLE,
	/**
	 * Horizontal clamp value is calculated and subtracted
	 * from image data along with vertical clamp value
	 */
	CCDC_HORZ_BC_CLAMP_CALC_ENABLED,
	/**
	 * Horizontal clamp value calculated from previous image
	 * is subtracted from image data along with vertical clamp
	 * value. How the horizontal clamp value for the first image
	 * is calculated in this case ???
	 */
	CCDC_HORZ_BC_CLAMP_NOT_UPDATED
};

/**
 * Base window selection for Horizontal Black Clamp calculations
 */
enum ccdc_horz_bc_base_win_sel {
	/* Select Most left window for bc calculation */
	CCDC_SEL_MOST_LEFT_WIN,

	/* Select Most right window for bc calculation */
	CCDC_SEL_MOST_RIGHT_WIN,
};

/* Size of window in horizontal direction for horizontal bc */
enum ccdc_horz_bc_sz_h {
	CCDC_HORZ_BC_SZ_H_2PIXELS,
	CCDC_HORZ_BC_SZ_H_4PIXELS,
	CCDC_HORZ_BC_SZ_H_8PIXELS,
	CCDC_HORZ_BC_SZ_H_16PIXELS
};

/* Size of window in vertcal direction for vertical bc */
enum ccdc_horz_bc_sz_v {
	CCDC_HORZ_BC_SZ_H_32PIXELS,
	CCDC_HORZ_BC_SZ_H_64PIXELS,
	CCDC_HORZ_BC_SZ_H_128PIXELS,
	CCDC_HORZ_BC_SZ_H_256PIXELS
};

/**
 * Structure for Horizontal Black Clamp config params
 */
struct ccdc_horz_bclamp {
	/* horizontal clamp mode */
	enum ccdc_horz_bc_mode mode;
	/**
	 * pixel value limit enable.
	 *  0 - limit disabled
	 *  1 - pixel value limited to 1023
	 */
	unsigned char clamp_pix_limit;
	/**
	 * Select most left or right window for clamp val
	 * calculation
	 */
	enum ccdc_horz_bc_base_win_sel base_win_sel_calc;
	/* Window count per color for calculation. range 1-32 */
	unsigned char win_count_calc;
	/* Window start position - horizontal for calculation. 0 - 8191 */
	unsigned short win_start_h_calc;
	/* Window start position - vertical for calculation 0 - 8191 */
	unsigned short win_start_v_calc;
	/* Width of the sample window in pixels for calculation */
	enum ccdc_horz_bc_sz_h win_h_sz_calc;
	/* Height of the sample window in pixels for calculation */
	enum ccdc_horz_bc_sz_v win_v_sz_calc;
};

/**
 * Black Clamp vertical reset values
 */
enum ccdc_vert_bc_reset_val_sel {
	/* Reset value used is the clamp value calculated */
	CCDC_VERT_BC_USE_HORZ_CLAMP_VAL,
	/* Reset value used is reset_clamp_val configured */
	CCDC_VERT_BC_USE_CONFIG_CLAMP_VAL,
	/* No update, previous image value is used */
	CCDC_VERT_BC_NO_UPDATE
};

enum ccdc_vert_bc_sz_h {
	CCDC_VERT_BC_SZ_H_2PIXELS,
	CCDC_VERT_BC_SZ_H_4PIXELS,
	CCDC_VERT_BC_SZ_H_8PIXELS,
	CCDC_VERT_BC_SZ_H_16PIXELS,
	CCDC_VERT_BC_SZ_H_32PIXELS,
	CCDC_VERT_BC_SZ_H_64PIXELS
};

/**
 * Structure for Vetical Black Clamp configuration params
 */
struct ccdc_vert_bclamp {
	/* Reset value selection for vertical clamp calculation */
	enum ccdc_vert_bc_reset_val_sel reset_val_sel;
	/* U12 value if reset_sel = CCDC_BC_VERT_USE_CONFIG_CLAMP_VAL */
	unsigned short reset_clamp_val;
	/**
	 * U8Q8. Line average coefficient used in vertical clamp
	 * calculation
	 */
	unsigned char line_ave_coef;
	/* Width in pixels of the optical black region used for calculation. */
	enum ccdc_vert_bc_sz_h ob_h_sz_calc;
	/* Height of the optical black region for calculation */
	unsigned short ob_v_sz_calc;
	/* Optical black region start position - horizontal. 0 - 8191 */
	unsigned short ob_start_h;
	/* Optical black region start position - vertical 0 - 8191 */
	unsigned short ob_start_v;
};

/**
 * Structure for Black Clamp configuration params
 */
struct ccdc_black_clamp {
	/**
	 * this offset value is added irrespective of the clamp
	 * enable status. S13
	 */
	unsigned short dc_offset;
	/**
	 * Enable black/digital clamp value to be subtracted
	 * from the image data
	 */
	unsigned char en;
	/**
	 * black clamp mode. same/separate clamp for 4 colors
	 * 0 - disable - same clamp value for all colors
	 * 1 - clamp value calculated separately for all colors
	 */
	unsigned char bc_mode_color;
	/* Vrtical start position for bc subtraction */
	unsigned short vert_start_sub;
	/* Black clamp for horizontal direction */
	struct ccdc_horz_bclamp horz;
	/* Black clamp for vertical direction */
	struct ccdc_vert_bclamp vert;
};

/*************************************************************************
** Color Space Convertion (CSC)
*************************************************************************/
/**
 * Number of Coefficient values used for CSC
 */
#define CCDC_CSC_NUM_COEFF 16

/*************************************************************************
**  Color Space Conversion parameters
*************************************************************************/
/**
 * Structure used for CSC config params
 */
struct ccdc_color_space_conv {
	/* Enable color space conversion */
	unsigned char en;
	/**
	 * csc coeffient table. S8Q5, M00 at index 0, M01 at index 1, and
	 * so forth
	 */
	struct ccdc_float_8 coeff[CCDC_CSC_NUM_COEFF];
};

/**
 * CCDC image data size
 */
enum ccdc_data_size {
	/* 8 bits */
	CCDC_8_BITS,
	/* 9 bits */
	CCDC_9_BITS,
	/* 10 bits */
	CCDC_10_BITS,
	/* 11 bits */
	CCDC_11_BITS,
	/* 12 bits */
	CCDC_12_BITS,
	/* 13 bits */
	CCDC_13_BITS,
	/* 14 bits */
	CCDC_14_BITS,
	/* 15 bits */
	CCDC_15_BITS,
	/* 16 bits */
	CCDC_16_BITS
};

/**
 * CCDC image data shift to right
 */
enum ccdc_datasft {
	/* No Shift */
	CCDC_NO_SHIFT,
	/* 1 bit Shift */
	CCDC_1BIT_SHIFT,
	/* 2 bit Shift */
	CCDC_2BIT_SHIFT,
	/* 3 bit Shift */
	CCDC_3BIT_SHIFT,
	/* 4 bit Shift */
	CCDC_4BIT_SHIFT,
	/* 5 bit Shift */
	CCDC_5BIT_SHIFT,
	/* 6 bit Shift */
	CCDC_6BIT_SHIFT
};

/**
 * MSB of image data connected to sensor port
 */
enum ccdc_data_msb {
	/* MSB b15 */
	CCDC_BIT_MSB_15,
	/* MSB b14 */
	CCDC_BIT_MSB_14,
	/* MSB b13 */
	CCDC_BIT_MSB_13,
	/* MSB b12 */
	CCDC_BIT_MSB_12,
	/* MSB b11 */
	CCDC_BIT_MSB_11,
	/* MSB b10 */
	CCDC_BIT_MSB_10,
	/* MSB b9 */
	CCDC_BIT_MSB_9,
	/* MSB b8 */
	CCDC_BIT_MSB_8,
	/* MSB b7 */
	CCDC_BIT_MSB_7
};

/*************************************************************************
**  Black  Compensation parameters
*************************************************************************/
/**
 * Structure used for Black Compensation
 */
struct ccdc_black_comp {
	/* Comp for Red */
	char r_comp;
	/* Comp for Gr */
	char gr_comp;
	/* Comp for Blue */
	char b_comp;
	/* Comp for Gb */
	char gb_comp;
};

/*************************************************************************
**  Gain parameters
*************************************************************************/
/**
 * Structure for Gain parameters
 */
struct ccdc_gain {
	/* Gain for Red or ye */
	struct ccdc_float_16 r_ye;
	/* Gain for Gr or cy */
	struct ccdc_float_16 gr_cy;
	/* Gain for Gb or g */
	struct ccdc_float_16 gb_g;
	/* Gain for Blue or mg */
	struct ccdc_float_16 b_mg;
};

/**
 * Predicator types for DPCM compression
 */
enum ccdc_dpcm_predictor {
	/* Choose Predictor1 for DPCM compression */
	CCDC_DPCM_PRED1,
	/* Choose Predictor2 for DPCM compression */
	CCDC_DPCM_PRED2
};

#define CCDC_LINEAR_TAB_SIZE 192
/*************************************************************************
**  Linearization parameters
*************************************************************************/
/**
 * Structure for Sensor data linearization
 */
struct ccdc_linearize {
	/* Enable or Disable linearization of data */
	unsigned char en;
	/* Shift value applied */
	enum ccdc_datasft corr_shft;
	/* scale factor applied U11Q10 */
	struct ccdc_float_16 scale_fact;
	/* Size of the linear table */
	unsigned short table[CCDC_LINEAR_TAB_SIZE];
};

enum ccdc_cfa_pattern {
	CCDC_CFA_PAT_MOSAIC,
	CCDC_CFA_PAT_STRIPE
};

enum ccdc_colpats {
	CCDC_RED,
	CCDC_GREEN_RED,
	CCDC_GREEN_BLUE,
	CCDC_BLUE
};

struct ccdc_col_pat {
	enum ccdc_colpats olop;
	enum ccdc_colpats olep;
	enum ccdc_colpats elop;
	enum ccdc_colpats elep;
};

/*************************************************************************
**  CCDC Raw configuration parameters
*************************************************************************/
enum ccdc_fmt_mode {
	CCDC_SPLIT,
	CCDC_COMBINE
};

enum ccdc_lnum {
	CCDC_1LINE,
	CCDC_2LINES,
	CCDC_3LINES,
	CCDC_4LINES
};

enum ccdc_line {
	CCDC_1STLINE,
	CCDC_2NDLINE,
	CCDC_3RDLINE,
	CCDC_4THLINE
};

struct ccdc_fmtplen {
	/**
	 * number of program entries for SET0, range 1 - 16
	 * when fmtmode is CCDC_SPLIT, 1 - 8 when fmtmode is
	 * CCDC_COMBINE
	 */
	unsigned short plen0;
	/**
	 * number of program entries for SET1, range 1 - 16
	 * when fmtmode is CCDC_SPLIT, 1 - 8 when fmtmode is
	 * CCDC_COMBINE
	 */
	unsigned short plen1;
	/**
	 * number of program entries for SET2, range 1 - 16
	 * when fmtmode is CCDC_SPLIT, 1 - 8 when fmtmode is
	 * CCDC_COMBINE
	 */
	unsigned short plen2;
	/**
	 * number of program entries for SET3, range 1 - 16
	 * when fmtmode is CCDC_SPLIT, 1 - 8 when fmtmode is
	 * CCDC_COMBINE
	 */
	unsigned short plen3;
};

struct ccdc_fmt_cfg {
	/* Split or combine or line alternate */
	enum ccdc_fmt_mode fmtmode;
	/* enable or disable line alternating mode */
	unsigned char ln_alter_en;
	/* Split/combine line number */
	enum ccdc_lnum lnum;
	/* Address increment Range 1 - 16 */
	unsigned int addrinc;
};

struct ccdc_fmt_addr_ptr {
	/* Initial address */
	unsigned int init_addr;
	/* output line number */
	enum ccdc_line out_line;
};

struct ccdc_fmtpgm_ap {
	/* program address pointer */
	unsigned char pgm_aptr;
	/* program address increment or decrement */
	unsigned char pgmupdt;
};

struct ccdc_data_formatter {
	/* Enable/Disable data formatter */
	unsigned char en;
	/* data formatter configuration */
	struct ccdc_fmt_cfg cfg;
	/* Formatter program entries length */
	struct ccdc_fmtplen plen;
	/* first pixel in a line fed to formatter */
	unsigned short fmtrlen;
	/* HD interval for output line. Only valid when split line */
	unsigned short fmthcnt;
	/* formatter address pointers */
	struct ccdc_fmt_addr_ptr fmtaddr_ptr[16];
	/* program enable/disable */
	unsigned char pgm_en[32];
	/* program address pointers */
	struct ccdc_fmtpgm_ap fmtpgm_ap[32];
};

struct ccdc_df_csc {
	/* Color Space Conversion confguration, 0 - csc, 1 - df */
	unsigned int df_or_csc;
	/* csc configuration valid if df_or_csc is 0 */
	struct ccdc_color_space_conv csc;
	/* data formatter configuration valid if df_or_csc is 1 */
	struct ccdc_data_formatter df;
	/* start pixel in a line at the input */
	unsigned int start_pix;
	/* number of pixels in input line */
	unsigned int num_pixels;
	/* start line at the input */
	unsigned int start_line;
	/* number of lines at the input */
	unsigned int num_lines;
};

struct ccdc_gain_offsets_adj {
	/* Gain adjustment per color */
	struct ccdc_gain gain;
	/* Offset adjustment */
	unsigned short offset;
	/* Enable or Disable Gain adjustment for SDRAM data */
	unsigned char gain_sdram_en;
	/* Enable or Disable Gain adjustment for IPIPE data */
	unsigned char gain_ipipe_en;
	/* Enable or Disable Gain adjustment for H3A data */
	unsigned char gain_h3a_en;
	/* Enable or Disable Gain adjustment for SDRAM data */
	unsigned char offset_sdram_en;
	/* Enable or Disable Gain adjustment for IPIPE data */
	unsigned char offset_ipipe_en;
	/* Enable or Disable Gain adjustment for H3A data */
	unsigned char offset_h3a_en;
};

struct ccdc_cul {
	/* Horizontal Cull pattern for odd lines */
	unsigned char hcpat_odd;
	/* Horizontal Cull pattern for even lines */
	unsigned char hcpat_even;
	/* Vertical Cull pattern */
	unsigned char vcpat;
	/* Enable or disable lpf. Apply when cull is enabled */
	unsigned char en_lpf;
};

enum ccdc_compress_alg {
	CCDC_ALAW,
	CCDC_DPCM,
	CCDC_NO_COMPRESSION
};

struct ccdc_compress {
	/* Enable or diable A-Law or DPCM compression. */
	enum ccdc_compress_alg alg;
	/* Predictor for DPCM compression */
	enum ccdc_dpcm_predictor pred;
};

/* all the stuff in this struct will be provided by userland */
struct ccdc_config_params_raw {
	/* Linearization parameters for image sensor data input */
	struct ccdc_linearize linearize;
	/* Data formatter or CSC */
	struct ccdc_df_csc df_csc;
	/* Defect Pixel Correction (DFC) confguration */
	struct ccdc_dfc dfc;
	/* Black/Digital Clamp configuration */
	struct ccdc_black_clamp bclamp;
	/* Gain, offset adjustments */
	struct ccdc_gain_offsets_adj gain_offset;
	/* Culling */
	struct ccdc_cul culling;
	/* A-Law and DPCM compression options */
	struct ccdc_compress compress;
	/* horizontal offset for Gain/LSC/DFC */
	unsigned short horz_offset;
	/* vertical offset for Gain/LSC/DFC */
	unsigned short vert_offset;
	/* color pattern for field 0 */
	struct ccdc_col_pat col_pat_field0;
	/* color pattern for field 1 */
	struct ccdc_col_pat col_pat_field1;
	/* data size from 8 to 16 bits */
	enum ccdc_data_size data_size;
	/* Data shift applied before storing to SDRAM */
	enum ccdc_datasft data_shift;
	/* enable input test pattern generation */
	unsigned char test_pat_gen;
};

#ifdef __KERNEL__
struct ccdc_ycbcr_config {
	/* ccdc pixel format */
	enum ccdc_pixfmt pix_fmt;
	/* ccdc frame format */
	enum ccdc_frmfmt frm_fmt;
	/* CCDC crop window */
	struct v4l2_rect win;
	/* field polarity */
	enum vpfe_pin_pol fid_pol;
	/* interface VD polarity */
	enum vpfe_pin_pol vd_pol;
	/* interface HD polarity */
	enum vpfe_pin_pol hd_pol;
	/* ccdc pix order. Only used for ycbcr capture */
	enum ccdc_pixorder pix_order;
	/* ccdc buffer type. Only used for ycbcr capture */
	enum ccdc_buftype buf_type;
};

struct ccdc_params_raw {
	/* ccdc pixel format */
	enum ccdc_pixfmt pix_fmt;
	/* ccdc frame format */
	enum ccdc_frmfmt frm_fmt;
	/* video window */
	struct v4l2_rect win;
	/* field polarity */
	enum vpfe_pin_pol fid_pol;
	/* interface VD polarity */
	enum vpfe_pin_pol vd_pol;
	/* interface HD polarity */
	enum vpfe_pin_pol hd_pol;
	/* buffer type. Applicable for interlaced mode */
	enum ccdc_buftype buf_type;
	/* Gain values */
	struct ccdc_gain gain;
	/* cfa pattern */
	enum ccdc_cfa_pattern cfa_pat;
	/* Data MSB position */
	enum ccdc_data_msb data_msb;
	/* Enable horizontal flip */
	unsigned char horz_flip_en;
	/* Enable image invert vertically */
	unsigned char image_invert_en;

	/*all the userland defined stuff*/
	struct ccdc_config_params_raw config_params;
};

enum ccdc_data_pack {
	CCDC_PACK_16BIT,
	CCDC_PACK_12BIT,
	CCDC_PACK_8BIT
};

#define CCDC_WIN_NTSC				{0, 0, 720, 480}
#define CCDC_WIN_VGA				{0, 0, 640, 480}
#define ISP5_CCDCMUX				0x20
#endif
#endif
