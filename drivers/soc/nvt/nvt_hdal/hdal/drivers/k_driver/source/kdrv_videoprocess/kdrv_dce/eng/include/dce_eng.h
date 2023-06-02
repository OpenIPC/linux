/**
    Public header file for DCE module.

    @file       dce_eng.h
    @ingroup    mIIPPDCE

    @brief

    Copyright   Novatek Microelectronics Corp. 2014.  All rights reserved.
*/


#ifndef _DCE_ENG_H_
#define _DCE_ENG_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "kwrap/type.h"
#include "kwrap/semaphore.h"
#include "kdrv_type.h"

#define DCE_ENG_REG_NUM	395

#if defined (_NVT_EMULATION_)
#define DCE_ENG_IMG_MAX_STP_NUM 64
#else
#define DCE_ENG_IMG_MAX_STP_NUM 16
#endif
#define DCE_ENG_GDC_TABLE_NUM 65
#define DCE_ENG_CAC_TABLE_NUM 65
#define DCE_ENG_STP_PARTITION_NUM 16

#define DCE_ENG_CFA_FREQ_BLEND_LUT_SZ 16
#define DCE_ENG_CFA_LUMA_WEIGHT_LUT_SZ 17
#define DCE_ENG_CFA_FCS_STRENGTH_LUT_SZ 16

#define DCE_ENG_WDR_SUBIMG_LPF_COEFF_NUM 3
#define DCE_ENG_WDR_IN_BLEND_NUM 17
#define DCE_ENG_WDR_COEFF_NUM 4
#define DCE_ENG_WDR_TONE_CURVE_IDX_NUM 32
#define DCE_ENG_WDR_TONE_CURVE_SPLIT_NUM 32
#define DCE_ENG_WDR_TONE_CURVE_VAL_NUM 65
#define DCE_ENG_WDR_OUTPUT_BLEND_IDX_NUM 32
#define DCE_ENG_WDR_OUTPUT_BLEND_SPLIT_NUM 32
#define DCE_ENG_WDR_OUTPUT_BLEND_VAL_NUM 65

#define DCE_ENG_WDR_HIST_BIN_NUM 128


/*****************************************************************************/
typedef void (*DCE_ISR_CB)(void *eng, UINT32 status, void *reserve);

typedef enum {
	DCE_ENG_START_LOAD = 0,
	DCE_ENG_FRAME_START_LOAD,
	DCE_ENG_FRAME_END_LOAD,
	DCE_ENG_GLOBAL_LOAD,
} DCE_ENG_LOAD_TYPE;

typedef struct {
	UINT32 chip_id;
	UINT32 eng_id;
	CHAR name[16];

	SEM_HANDLE sem;
	ID flg_id;
	UINT32 clock_rate;
	void *pclk;			/* linux struct clk*, not used in freertos */
	UINT32 reg_io_base;
	UINT32 irq_id;

	DCE_ISR_CB isr_cb;
} DCE_ENG_HANDLE;

typedef struct {
	UINT32 chip_num;
	UINT32 eng_num;
	UINT32 total_ch;
	DCE_ENG_HANDLE *p_eng;
} DCE_ENG_CTL;

typedef struct {
	UINT32 ofs;
	UINT32 val;
} DCE_ENG_REG;

/*****************************************************************************/

typedef enum {
	DCE_ENG_FULL_RANGE = 0,		// IFE -> DCE -> IPE -> IME
	DCE_ENG_BT601_RANGE,		// DRAM -> DCE -> IPE -> IME
	DCE_ENG_BT709_RANGE, 		// SIE -> IPE -> DCE -> IPE -> IME -> H264
	DCE_ENG_YUV2RGB_RANGE_MAX,
} DCE_ENG_YUVRGB_FMT;


typedef enum {
	DCE_ENG_INTERRUPT_FRMST				= 0x00000001,
	DCE_ENG_INTERRUPT_FRMEND			= 0x00000002,
	DCE_ENG_INTERRUPT_STEND				= 0x00000004,
	DCE_ENG_INTERRUPT_STP_ERR			= 0x00000008,
	DCE_ENG_INTERRUPT_LB_OVF			= 0x00000010,
	DCE_ENG_INTERRUPT_STP_OB			= 0x00000040,
	DCE_ENG_INTERRUPT_Y_BACK			= 0x00000080,
	DCE_ENG_INTERRUPT_LL_END			= 0x00000100,
	DCE_ENG_INTERRUPT_LL_ERROR			= 0x00000200,
	DCE_ENG_INTERRUPT_LL_ERROR2			= 0x00000400,
	DCE_ENG_INTERRUPT_LL_JOBEND			= 0x00000800,
	DCE_ENG_INTERRUPT_FRAMEERR			= 0x00001000,
	DCE_ENG_INTERRUPT_ALL 				= (DCE_ENG_INTERRUPT_FRMST | DCE_ENG_INTERRUPT_FRMEND | DCE_ENG_INTERRUPT_STEND | DCE_ENG_INTERRUPT_STP_ERR | DCE_ENG_INTERRUPT_LB_OVF |
											DCE_ENG_INTERRUPT_STP_OB | DCE_ENG_INTERRUPT_Y_BACK | DCE_ENG_INTERRUPT_LL_END | DCE_ENG_INTERRUPT_LL_ERROR | DCE_ENG_INTERRUPT_LL_ERROR2 |
											DCE_ENG_INTERRUPT_LL_JOBEND | DCE_ENG_INTERRUPT_FRAMEERR),
} DCE_ENG_INTERRUPT;

typedef enum {
	/* 3 channel */
	DCE_ENG_FMT_Y_PACK_UV422 = 0,	// 422 format y planar UV pack UVUVUVUVUV.....
	DCE_ENG_FMT_Y_PACK_UV420,		// 420 format y planar UV pack UVUVUVUVUV.....
	DCE_ENG_FMT_BAYER_8BIT,			// 444 format y planar UV pack UVUVUVUVUV.....
} DCE_ENG_FMT;

typedef enum {
	DCE_ENG_DRAM_OUT_NORMAL = 0,    // sub-image output depend on function
	DCE_ENG_DRAM_OUT_SINGLE = 1,    // sub-image output depend on single output enable
} DCE_ENG_DRAM_OUTPUT_MODE;

typedef enum {
	DCE_ENG_BURST_32W = 0,   // 32 word DMA burst
	DCE_ENG_BURST_16W = 1,   // 16 word DMA burst
} DCE_ENG_DMA_BURST;

typedef enum {
	DCE_ENG_CFA_SUBOUT_CH0 = 0,         // Select channel 0 as output channel
	DCE_ENG_CFA_SUBOUT_CH1 = 1,         // Select channel 1 as output channel
	DCE_ENG_CFA_SUBOUT_CH2 = 2,         // Select channel 2 as output channel
	DCE_ENG_CFA_SUBOUT_CH3 = 3,         // Select channel 3 as output channel
} DCE_ENG_CFA_SUBOUT_CH_SEL;

typedef enum {
	DCE_ENG_CFA_SUBOUT_1BYTE = 0,       // Output 1 byte per pixel for CFA subout
	DCE_ENG_CFA_SUBOUT_2BYTE = 1,       // Output 2 byte per pixel for CFA subout
} DCE_ENG_CFA_SUBOUT_BYTE;

typedef enum {
	DCE_ENG_OUT_NORMAL = 0,
	DCE_ENG_OUT_SINGLE = 1,
} DCE_ENG_DRAM_OUT_MODE;

typedef struct {
	DCE_ENG_DRAM_OUT_MODE out_mode;
	UINT8 out0_single_en;
	UINT8 out1_single_en;
} DCE_ENG_DMA_OUT_INFO;

typedef struct {
	UINT32 frm_start_cnt;
	UINT32 frm_end_cnt;
	UINT32 strp_end_cnt;
	UINT32 strp_err_cnt;
	UINT32 lbuf_ovfl_cnt;
	UINT32 strp_out_of_bnd_cnt;
	UINT32 y_back_cnt;
	UINT32 dce_ll_end_cnt;
	UINT32 dce_ll_jobend_cnt;
	UINT32 dce_ll_err_cnt;
	UINT32 dce_ll_err2_cnt;
} DCE_ENG_INT_STATUS_CNT;

//--------------------- Behavior ----------------------
typedef enum {
	DCE_ENG_OPMODE_DIRECT_IPP = 0,	// Flow 0,        IFE -> DCE -> IPE -> IME
	DCE_ENG_OPMODE_DIRECT_IME,		// Flow 1,               DCE -> IPE -> IME
	DCE_ENG_OPMODE_DIRECT_ALL, 		// Flow 2, SIE -> IFE -> DCE -> IPE -> IME
	DCE_ENG_OPMODE_D2D, 			// Engine D2D
	DCE_ENG_OPMODE_MAX,
} DCE_ENG_OPMODE;

typedef enum {
	DCE_ENG_SRAM_DCE = 0,       // Use DCE SRAM only
	DCE_ENG_SRAM_CNN = 1,       // Use DCE and CNN SRAM
} DCE_ENG_SRAM_MODE;

typedef enum {
	DCE_ENG_STP_SST     = 0,  // DCE horizontal single stripe mode
	DCE_ENG_STP_MST     = 1,  // DCE horizontal multiple stripe mode
} DCE_ENG_STP_MODE;

typedef struct {
	DCE_ENG_OPMODE op_mode;         // Operation mode selection
	DCE_ENG_STP_MODE stp_mode;      // Stripe mode selection
	DCE_ENG_SRAM_MODE sram_mode;		// SRAM buffer mode selection
} DCE_ENG_BEHAVIOR_PARAM;


//--------------------- CFA ----------------------
typedef enum {
	DCE_ENG_BAYER_2X2 = 0,         // Bayer 2x2: R, Gr, Gb, B
	DCE_ENG_RGBIR_4X4 = 1,         // RGBIR 4x4: R, Gr, Gir1, Ir1, B, Gb, Gir2, Ir2
} DCE_ENG_RAW_FMT;

typedef enum {
	//RGGB
	DCE_ENG_CFA_RGGB = 0,
	DCE_ENG_CFA_GRBG = 1,
	DCE_ENG_CFA_GBRG = 2,
	DCE_ENG_CFA_BGGR = 3,

	//RGBIR
	DCE_ENG_CFA_RGBG_GIGI = 0,
	DCE_ENG_CFA_GBGR_IGIG = 1,
	DCE_ENG_CFA_GIGI_BGRG = 2,
	DCE_ENG_CFA_IGIG_GRGB = 3,
	DCE_ENG_CFA_BGRG_GIGI = 4,
	DCE_ENG_CFA_GRGB_IGIG = 5,
	DCE_ENG_CFA_GIGI_RGBG = 6,
	DCE_ENG_CFA_IGIG_GBGR = 7,
} DCE_ENG_CFA_PAT;

typedef struct {
	BOOL flip;
	UINT8 r_shift_bit;
	UINT32 cfa_subout_addr;
	UINT32 cfa_subout_ofst;
	DCE_ENG_CFA_SUBOUT_CH_SEL ch_sel;
	DCE_ENG_CFA_SUBOUT_BYTE byte;
} DCE_ENG_CFA_SUBOUT;

typedef struct {
	UINT32 edge_dth;    		// Edge threshold 1 for Bayer CFA interpolation
	UINT32 edge_dth2;   		// Edge threshold 2 for Bayer CFA interpolation
	UINT32 freq_th;
	UINT8 *p_freq_blend_lut;	// CFA frequency blending weight look-up table, 16 entries, define DCE_ENG_CFA_FREQ_BLEND_LUT_SZ
	UINT8 *p_luma_wt;			// Luma gain for CFA, 17 entries, define DCE_ENG_CFA_LUMA_WEIGHT_LUT_SZ
} DCE_ENG_CFA_INTERP;

typedef struct {
	BOOL      rb_corr_en;   	// Enable for RB channel correction
	UINT32    rb_corr_th1;  	// RB correction noise threshold1
	UINT32    rb_corr_th2;  	// RB correction noise threshold2
} DCE_ENG_CFA_RB_CORR;

typedef struct {
	BOOL fcs_dirsel;       	// Direction selection for detecting high frequency, 0: detect 2 directions, 1: detect 4 directions
	UINT32 fcs_coring;      // Coring threshold for false color suppression
	UINT32 fcs_weight;      // Global strength for false color suppression
	UINT8 *p_fcs_strength;	// CFA false color suppression strength, 16 entries, define DCE_ENG_CFA_FCS_STRENGTH_LUT_SZ
} DCE_ENG_CFA_FCS;

typedef struct {
	BOOL      ir_cl_check_en;       // Enable cross line check
	BOOL      ir_hf_check_en;       // Enable high frequency check
	BOOL      ir_average_mode;      // Interpolation method selection when flat region detect
	BOOL      ir_cl_sel;            // operation type when cross line is not found
	UINT32    ir_cl_th;             // Cross line checking threshold
	UINT32    ir_hf_gth;            // G channel smoothness checking threshold
	UINT32    ir_hf_diff;           // Difference threshold value between Gr and Gb to determing high frequency pattern detected or not
	UINT32    ir_hf_eth;            // Threshold to determine H or V direction high frequency pattern detected
	UINT32    ir_g_edge_th;         // Noise margin for edge in Green component interpolation
	UINT32    ir_rb_cstrength;      // RB channel smooth strength
} DCE_ENG_CFA_IR_HFC;

typedef struct {
	UINT32    ir_sub_r;         // R channel IR sub.
	UINT32    ir_sub_g;         // G channel IR sub.
	UINT32    ir_sub_b;         // B channel IR sub.
	UINT32    ir_sub_wt_lb;     // Lower bound of IR local weighting
	UINT32    ir_sub_th;        // Threshold to decay IR local weighting
	UINT32    ir_sub_range;     // Range to decay IR local weighting
	UINT32    ir_sat_gain;      // Saturation gain multiplied after IR sub
} DCE_ENG_CFA_IR_SUB;

typedef struct {
	DCE_ENG_CFA_IR_HFC high_freq;
	DCE_ENG_CFA_IR_SUB ir_sub;
} DCE_ENG_CFA_RGBIR;

typedef enum {
	DCE_ENG_CGAIN_2_8 = 0,   // gain range integer/fraction 2.8
	DCE_ENG_CGAIN_3_7 = 1,   // gain range integer/fraction 3.7
} DCE_ENG_CGAIN_RANGE;

typedef struct {
	UINT32    gain_r;          // R channel gain
	UINT32    gain_g;          // G channel gain
	UINT32    gain_b;          // B channel gain
	DCE_ENG_CGAIN_RANGE gain_range;  // select fraction type of gain: 2.8 or 3.7
} DCE_ENG_CFA_CGAIN;

typedef enum {
	DCE_ENG_PINKR_MOD_G  = 0,   // modify G channel
	DCE_ENG_PINKR_MOD_RB = 1,   // modify RB channel
} DCE_ENG_PINKR_MODE;

typedef struct {
	BOOL pink_reduc_enable;
	DCE_ENG_PINKR_MODE pink_rd_mode;    // pink reduction mode
	UINT32    pink_rd_th1;          // pink reduction threshold 1
	UINT32    pink_rd_th2;          // pink reduction threshold 2
	UINT32    pink_rd_th3;          // pink reduction threshold 3
	UINT32    pink_rd_th4;          // pink reduction threshold 4
} DCE_ENG_CFA_PINKR;

typedef struct {
	DCE_ENG_RAW_FMT raw_fmt;
	DCE_ENG_CFA_PAT cfa_pat;
} DCE_ENG_CFA_PARAM;

typedef struct {
	UINT32 h_size;    // Horizontal image size, unit : pix
	UINT32 v_size;    // Vertical image size, uint : line
} DCE_ENG_IMG_SIZE;

typedef struct {
	UINT32 h_fact;    // horizontal factor
	UINT32 v_fact;    // vertical factor
} DCE_ENG_HV_FACTOR;

typedef struct {
	UINT32 wdr_subimg_width;        // Wdr subimg width
	UINT32 wdr_subimg_height;       // Wdr subimg height

	UINT32 wdr_subin_addr;          // Wdr subimg input address
	UINT32 wdr_subin_ofst;          // Wdr subimg input line offset
	DCE_ENG_HV_FACTOR subin_scal_fact; 	// Wdr subimg scaling factor

	UINT32 wdr_subout_addr;         // Wdr subimg output address
	UINT32 wdr_subout_ofst;         // Wdr subimg output line offset
	DCE_ENG_IMG_SIZE blk_size;        	// Wdr subimg output block size
	DCE_ENG_HV_FACTOR blk_cent_fact;  	// Wdr subimg output block center factor
} DCE_ENG_WDR_SUBIMG;

typedef enum {
	DCE_ENG_WDR_ROUNDING        = 0,  // rounding
	DCE_ENG_WDR_HALFTONING      = 1,  // halftone rounding
	DCE_ENG_WDR_RANDOMLSB       = 2,  // random rounding
} DCE_ENG_WDR_DITHER_MODE;

typedef struct {
	BOOL wdr_rand_rst;
	DCE_ENG_WDR_DITHER_MODE wdr_rand_sel;
} DCE_ENG_WDR_DITHER;

typedef enum {
	DCE_ENG_INPUT_BLD_3x3_Y  = 0,   // use Y from bayer 3x3 as the input blending source selection
	DCE_ENG_INPUT_BLD_GMEAN  = 1,   // use Gmean from bayer 3x3 as the input blending source selection
	DCE_ENG_INPUT_BLD_BAYER  = 2,   // use Bayer as the input blending source selection
} DCE_ENG_WDR_INPUT_BLDSEL;

typedef struct {
	DCE_ENG_WDR_INPUT_BLDSEL wdr_bld_sel;
	UINT32 wdr_bld_wt;
	UINT8 *p_inblend_lut;	// 17 entries, DCE_ENG_WDR_IN_BLEND_NUM
} DCE_ENG_WDR_IN_BLD;

typedef struct {
	INT16 *p_wdr_coeff;		// 4 entries, DCE_ENG_WDR_COEFF_NUM
	UINT32 strength;
	UINT32 contrast;
} DCE_ENG_WDR_STRENGTH;

typedef struct {
	BOOL gainctrl_en;
	UINT32 max_gain;
	UINT32 min_gain;
} DCE_ENG_WDR_GAINCTRL;

typedef struct {
	UINT32 sat_th;
	UINT32 sat_wt_low;
	UINT32 sat_delta;
} DCE_ENG_WDR_SAT_REDUCT;

typedef enum {
	DCE_ENG_HIST_BEFORE_WDR      = 0,    // histogram statistics before wdr algorithm
	DCE_ENG_HIST_AFTER_WDR       = 1,    // histogram statistics after wdr algorithm
} DCE_ENG_WDR_HIST_SEL;

typedef struct {
	BOOL histogram_enable;
	DCE_ENG_WDR_HIST_SEL hist_sel;
	UINT32 hist_step_h;
	UINT32 hist_step_v;
} DCE_ENG_WDR_HIST;

typedef struct {
	DCE_ENG_WDR_DITHER dither;
	DCE_ENG_WDR_IN_BLD wdr_input_bld;
	DCE_ENG_WDR_STRENGTH wdr_strength;
	DCE_ENG_WDR_GAINCTRL wdr_gainctrl;
	DCE_ENG_WDR_SAT_REDUCT wdr_sat_reduct;
	DCE_ENG_WDR_HIST wdr_hist;
} DCE_ENG_WDR_PARAM;

typedef struct {
	UINT16 *p_hist_stcs;	// 128 entries, DCE_ENG_WDR_HIST_BIN_NUM
} DCE_ENG_WDR_HIST_RESULT;

typedef struct {
	UINT32 h_stp_buf_mode;              // Last H stripe buffer mode used
	UINT32 h_stp_st_x;                  // Last H stripe starting input X coordinate
	UINT32 h_stp_ed_x;                  // Last H stripe ending input X coordinate
	UINT32 h_stp_clm_st_x;              // Last H stripe starting input X coordinate of the first column
	UINT32 h_stp_clm_ed_x;              // Last H stripe ending input X coordinate of the first column
	UINT32 buf_height_g;                // Last H stripe vertical buffer used by G
	UINT32 buf_height_rgb;              // Last H stripe vertical buffer used by RGB
	UINT32 buf_height_pix;              // Last H stripe vertical buffer used by RGB of single pixel
} DCE_ENG_HSTP_STATUS;

typedef struct {
	INT32  x_cent;    		// Horizontal center, unit : pix
	INT32  y_cent;    		// Vertical center, uint : line
	UINT32 oval_x_dist;    	// Horizontal distance ratio for oval model
	UINT32 oval_y_dist;    	// Vertical distance ratio for oval model
	UINT32 nrm_fact_sel;	// select different factor bit number
	UINT32 nrm_fact;    	// Normalization factor
	UINT32 nrm_fact_10b;	// Normalization factor 10 bit
	UINT32 nrm_bit;     	// Normalization bit
} DCE_ENG_DIST_NORM;

typedef enum {
	DCE_ENG_XY_BOTH = 0,        // processing both X and Y
	DCE_ENG_X_ONLY  = 1,        // processing X only
} DCE_ENG_DC_MODE;

typedef enum {
	DCE_ENG_CAC_RGB_FIXED_GAIN = 0,    // Correct CA with G LUT and R, G, B LUTGAIN
	DCE_ENG_CAC_RGB_GAIN_LUT = 1,      // Correct CA with R, G, B LUT
} DCE_ENG_CAC_SEL;

typedef struct {
	UINT32 r_lut_gain;    // R channel lut gain
	UINT32 g_lut_gain;    // G channel lut gain
	UINT32 b_lut_gain;    // B channel lut gain
} DCE_ENG_CAC_GAIN;

typedef enum {
	DCE_ENG_FOV_BOUND_DUPLICATE = 0,    // Replace out of boundary pixels with duplicate boundary pixels
	DCE_ENG_FOV_BOUND_REG_RGB   = 1,    // Replace out of boundary pixels with GEO_BOUNDR, GEO_BOUNDG, GEO_BOUNDB
} DCE_ENG_FOV_OUTBND_SEL;

typedef struct {
	UINT32    r_val;    // R boundary value
	UINT32    g_val;    // G boundary value
	UINT32    b_val;    // B boundary value
} DCE_ENG_FOV_OUTBND_RGB;

typedef struct {
	UINT32 fov_gain;    // FOV lut gain
	DCE_ENG_FOV_OUTBND_SEL fov_outbnd_sel;   // FOV out-of-boundary type selection
	DCE_ENG_FOV_OUTBND_RGB fov_outbnd_rgb;   // FOV out-of-boundary RGB definition
} DCE_ENG_FOV_GAIN;

typedef struct {
	DCE_ENG_DC_MODE dc_mode;   // process both X & Y or X only
	DCE_ENG_CAC_SEL cac_sel;   // CAC compensation method selection
	DCE_ENG_CAC_GAIN cac_gain; // CAC RGB lut gain
	DCE_ENG_FOV_GAIN fov;
} DCE_ENG_GDC_CAC_PARAM;

typedef enum {
	DCE_ENG_2DLUT_65_65_GRID = 0,         // 65x65 grid 2D lut distortion correction
	DCE_ENG_2DLUT_33_33_GRID = 1,         // 33x33 grid 2D lut distortion correction
	DCE_ENG_2DLUT_17_17_GRID = 2,         // 17x17 grid 2D lut distortion correction
	DCE_ENG_2DLUT_9_9_GRID   = 3,         // 9x9 grid 2D lut distortion correction
} DCE_ENG_2DLUT_NUM;

typedef struct {
	UINT32    x_ofs_int;   // 2D lut x integer offset
	UINT32    x_ofs_frac;  // 2D lut x fraction offset
	UINT32    y_ofs_int;   // 2D lut y integer offset
	UINT32    y_ofs_frac;  // 2D lut y fraction offset
} DCE_ENG_2DLUT_TABLE_OFST;

typedef struct {
	UINT32 lut2d_addr;                  // 2D lut DRAM address
	BOOL ymin_auto_en;
	DCE_ENG_DC_MODE dc_mode;   			// process both X & Y or X only
	DCE_ENG_2DLUT_NUM lut2d_num_sel;    // 2D lut grid number
	DCE_ENG_HV_FACTOR lut2d_scale_fact;
	DCE_ENG_2DLUT_TABLE_OFST lut2d_offset;
} DCE_ENG_2DLUT_PARAM;

typedef enum {
	DCE_ENG_FULL_YUV        = 0,    // Full range YUV input
	DCE_ENG_BT601_YUV       = 1,    // BT601 YUV input
	DCE_ENG_BT709_YUV       = 2,    // BT709 YUV input
} DCE_ENG_YUV2RGB_FMT;

typedef struct {
	DCE_ENG_FMT d2d_fmt;
	BOOL yuv2rgb_enable;
	DCE_ENG_YUV2RGB_FMT yuv2rgb_fmt;
	BOOL d2d_random_lsb_enable;
	BOOL d2d_random_seed_rst;
	BOOL d2d_uv_lpf_enable;
	BOOL d2d_io_stop;
	BOOL crop_enable; //crop_enable should be 0 when running in direct mode
	UINT32 crop_hstart;
	UINT32 crop_width;
} DCE_ENG_D2D_PARAM;

typedef struct {
	UINT32 input_y_addr;
	UINT32 input_uv_addr;
	UINT32 input_y_ofst;
	UINT32 input_uv_ofst;
} DCE_ENG_DRAM_IN_PARAM;

typedef struct {
	UINT32 output_y_addr;
	UINT32 output_uv_addr;
	UINT32 output_y_ofst;
	UINT32 output_uv_ofst;
} DCE_ENG_DRAM_OUT_PARAM;

typedef struct {
	DCE_ENG_DMA_BURST in_burst;
	DCE_ENG_DMA_BURST out_burst;
} DCE_ENG_DMA_BURST_PARAM;

//--------------------- Stripe ----------------------
typedef enum {
	DCE_ENG_1ST_STP_POS_CALC = 0,    // H,V Input starts from DCE calculated position
	DCE_ENG_1ST_STP_POS_ZERO = 1,    // H,V Input starts from 0
} DCE_ENG_STP_INST;

typedef enum {
	DCE_ENG_HSTP_IPEOLAP_8  = 0,    // 4 pixels overlap
	DCE_ENG_HSTP_IPEOLAP_16 = 1,    // 8 pixels overlap
} DCE_ENG_HSTP_IPEOLAP_SEL;

typedef enum {
	DCE_ENG_HSTP_IMEOLAP_16   = 0,    // 16 pixels overlap
	DCE_ENG_HSTP_IMEOLAP_24   = 1,    // 24 pixels overlap
	DCE_ENG_HSTP_IMEOLAP_32   = 2,    // 32 pixels overlap
	DCE_ENG_HSTP_IMEOLAP_USER = 3,    // user-defined value
} DCE_ENG_HSTP_IMEOLAP_SEL;

typedef struct {
	UINT8 hstp_maxinc;
	UINT8 hstp_maxdec;
	UINT8 lbuf_back_rsv_line;
	UINT8 hstp_vmaxdec;
	DCE_ENG_HSTP_IPEOLAP_SEL ipe_ovlp_sel;
	DCE_ENG_HSTP_IMEOLAP_SEL ime_ovlp_sel;
	UINT16 imeolap_user_val;     	// user-defined overlap value
	UINT16 *p_hstp; 			// 16 entries, DCE_ENG_STP_PARTITION_NUM
} DCE_ENG_STRIPE_PARAM;

typedef enum {
	DCE_ENG_STRP_GDC_HIGH_PRIOR = 0,     	// stripe configuration for GDC fair performance + low latency
	DCE_ENG_STRP_LOW_LAT_HIGH_PRIOR = 1, 	// stripe configuration for low latency best performance
	DCE_ENG_STRP_2DLUT_HIGH_PRIOR = 2,   	// stripe configuration for 2DLUT best performance + low latency
	DCE_ENG_STRP_GDC_BEST = 3,     			// stripe configuration for GDC best performance without low latency
} DCE_ENG_STRP_RULE_SEL;

typedef enum {
	DCE_ENG_STRIPE_AUTO = 0,           //Auto calculation
	DCE_ENG_STRIPE_MANUAL_1STRIPE,     //Force 1 stripe, equal to no stripe
	DCE_ENG_STRIPE_MANUAL_2STRIPE,     //Force 2 stripe
	DCE_ENG_STRIPE_MANUAL_3STRIPE,     //Force 3 stripe
	DCE_ENG_STRIPE_MANUAL_4STRIPE,     //Force 4 stripe
	DCE_ENG_STRIPE_MANUAL_5STRIPE,     //Force 5 stripe
	DCE_ENG_STRIPE_MANUAL_6STRIPE,     //Force 6 stripe
	DCE_ENG_STRIPE_MANUAL_7STRIPE,     //Force 7 stripe
	DCE_ENG_STRIPE_MANUAL_8STRIPE,     //Force 8 stripe

	DCE_ENG_STRIPE_MANUAL_MULTI,       //Force mult-stripe
	DCE_ENG_STRIPE_MANUAL_TEST,        //Accept user-defined stripe param
} DCE_ENG_STRIPE_TYPE;

typedef struct {
	BOOL ime_enc;            // enable status of IME encode
	BOOL ime_3dnr;           // enable status of IME 3DNR
	BOOL ime_dec;            // enable status of IME decode
	BOOL ime_isd;            // enable status of IME ISD
	BOOL ime_ref_out;        // enable status of IME reference out
	BOOL ime_p1_enc;         // enable status of IME path 1 encode
	BOOL low_latency;        // enable status of IME/Codec low latency
	BOOL dc_en;				 // enable status of DCE Distortion correction
	BOOL ipe_eth_2bit;		 // enable status of IPE Eth 2bit
} DCE_ENG_EXTEND_INFO;

typedef struct {
	DCE_ENG_STRIPE_TYPE stripe_type;
	DCE_ENG_STRP_RULE_SEL strp_rule;//for DCE_ENG_STRIPE_AUTO only
	DCE_ENG_OPMODE op_mode;
	DCE_ENG_SRAM_MODE sram_mode;
	UINT32 in_width;
	UINT32 in_height;
	UINT16 ipe_overlap;
	UINT16 ime_overlap;
	UINT16 ipe_stripe_multiple;
	UINT16 ime_stripe_multiple;

	//variables below are for STRIPE_TYPE = DCE_ENG_STRIPE_MANUAL_MULTI
	DCE_ENG_HANDLE *p_eng;
	BOOL crop_enable; //crop_enable should be 0 when DCE is not at D2D mode
	BOOL cfa_enable;
	BOOL dc_enable;
	BOOL cac_enable;
	UINT32 crop_x;
	UINT32 crop_width;
	DCE_ENG_GDC_CAC_PARAM *p_gdc_cac_param;
	DCE_ENG_DIST_NORM *p_dist_norm;
	UINT16 *p_gdc_lut; //DCE_ENG_GDC_TABLE_NUM
	INT16 *p_cac_lut_r; //DCE_ENG_CAC_TABLE_NUM
	INT16 *p_cac_lut_b; //DCE_ENG_CAC_TABLE_NUM

	DCE_ENG_EXTEND_INFO ext_info;
} DCE_ENG_CAL_STP_INFO;

typedef struct {
	//for DCE registers
	DCE_ENG_STRIPE_PARAM strp_param;

	//for flow
	UINT32 hstp_num;  // H stripe number for flow
	UINT16 *p_dce_out_hstp;	// H stripe results of DCE output image, DCE_ENG_IMG_MAX_STP_NUM
	UINT16 *p_ime_in_hstp;	// H stripe results of IME input image, DCE_ENG_IMG_MAX_STP_NUM
	UINT16 *p_ime_out_hstp;	// H stripe results of IME output image, DCE_ENG_IMG_MAX_STP_NUM
} DCE_ENG_STP_RESULT;

//----------------------------------------------
extern DCE_ENG_INT_STATUS_CNT dce_int_stat_cnt;

//----------------------------------------------
extern UINT32 dce_eng_get_reg_base_buf_size(VOID);
extern UINT32 dce_eng_get_reg_flag_buf_size(VOID);
extern VOID dce_eng_set_reg_buf(UINT32 reg_base_addr, UINT32 reg_flag_addr);

//------- calculate parameter functions --------
extern UINT32 dce_eng_cal_stripe(DCE_ENG_CAL_STP_INFO *p_cal_info, DCE_ENG_STP_RESULT *p_stripe_rst);
extern void dce_eng_get_output_stp_setting(DCE_ENG_CAL_STP_INFO *p_cal_info, DCE_ENG_STP_RESULT *p_ns_result);
extern void dce_eng_cal_gdc_norm_factor(DCE_ENG_DIST_NORM *p_dist_norm, UINT32 in_width, UINT32 in_height);

//------- write to physical register --------
extern INT32 dce_eng_init(UINT32 chip_num, UINT32 eng_num);
extern INT32 dce_eng_release(void);
extern INT32 dce_eng_init_resource(DCE_ENG_HANDLE *p_eng);
extern DCE_ENG_HANDLE* dce_eng_get_handle(UINT32 chip_id, UINT32 eng_id);
extern void dce_eng_reg_isr_callback(DCE_ENG_HANDLE *p_eng, DCE_ISR_CB cb);
extern INT32 dce_eng_open(DCE_ENG_HANDLE *p_eng);
extern INT32 dce_eng_close(DCE_ENG_HANDLE *p_eng);
extern void dce_eng_set_load_hw_reg(DCE_ENG_HANDLE *p_eng, DCE_ENG_LOAD_TYPE type);
extern void dce_eng_trig_single_hw_reg(DCE_ENG_HANDLE *p_eng);
extern void dce_eng_stop_single_hw_reg(DCE_ENG_HANDLE *p_eng);
extern void dce_eng_trig_ll_hw_reg(DCE_ENG_HANDLE *p_eng, UINT32 ll_addr);
extern void dce_eng_stop_ll_hw_reg(DCE_ENG_HANDLE *p_eng);
extern void dce_eng_write_reg(DCE_ENG_HANDLE *p_eng, UINT32 reg_ofs, UINT32 val);
extern void dce_eng_isr(DCE_ENG_HANDLE *p_eng);
extern void dce_eng_set_reset_hw_reg(DCE_ENG_HANDLE *p_eng, BOOL enable);
extern void dce_eng_clear_interrupt_status_hw_reg(DCE_ENG_HANDLE *p_eng, UINT32 mask);
extern void dce_eng_hard_reset_hw_reg(DCE_ENG_HANDLE *p_eng);
extern VOID dce_eng_set_dma_channel_enable_hw_reg(DCE_ENG_HANDLE *p_eng, BOOL set_en);

//------- read from physical register --------
extern UINT8 dce_eng_get_dma_channel_status_hw_reg(DCE_ENG_HANDLE *p_eng);
extern void dce_eng_get_wdr_hist_hw_reg(DCE_ENG_HANDLE *p_eng, DCE_ENG_WDR_HIST_RESULT *p_hist_result);
extern UINT32 dce_eng_get_interrupt_status_hw_reg(DCE_ENG_HANDLE *p_eng);

//------- write to virtual register buffer --------
extern void dce_eng_set_interrupt_enable_buf_reg(UINT32 mask);
extern void dce_eng_set_input_size_buf_reg(UINT32 width, UINT32 height);
extern void dce_eng_set_dram_single_out_buf_reg(BOOL wdr_subout_sgo_en, BOOL cfa_subout_sgo_en, DCE_ENG_DRAM_OUTPUT_MODE dram_out_mode);
extern void dce_eng_set_behavior_param_buf_reg(DCE_ENG_BEHAVIOR_PARAM *p_behav_param);
extern void dce_eng_set_stripe_param_buf_reg(DCE_ENG_STRIPE_PARAM *p_strp_param);

extern void dce_eng_set_wdr_enable_buf_reg(BOOL wdr_enable);
extern void dce_eng_set_wdr_subout_enable_buf_reg(BOOL wdr_subout_enable);
extern void dce_eng_set_wdr_buf_reg(DCE_ENG_WDR_PARAM *p_wdr_param);
extern void dce_eng_set_wdr_subimg_buf_reg(DCE_ENG_WDR_SUBIMG *p_wdr_subimg);
extern void dce_eng_set_wdr_subimg_lpf_coeff_buf_reg(UINT8 *p_lpf_coef); //DCE_ENG_WDR_SUBIMG_LPF_COEFF_NUM

extern void dce_eng_set_wdr_tone_curve_enable_buf_reg(BOOL tone_curve_enable);
extern void dce_eng_set_wdr_tone_curve_index_table_buf_reg(UINT8 *p_lut_idx); //DCE_ENG_WDR_TONE_CURVE_IDX_NUM
extern void dce_eng_set_wdr_tone_curve_split_table_buf_reg(UINT8 *p_lut_split); //DCE_ENG_WDR_TONE_CURVE_SPLIT_NUM
extern void dce_eng_set_wdr_tone_curve_value_table_buf_reg(UINT16 *p_lut_val); //DCE_ENG_WDR_TONE_CURVE_VAL_NUM

extern void dce_eng_set_wdr_output_blend_enable_buf_reg(BOOL outbld_enable);
extern void dce_eng_set_wdr_output_blend_index_table_buf_reg(UINT8 *p_lut_idx); //DCE_ENG_WDR_OUTPUT_BLEND_IDX_NUM
extern void dce_eng_set_wdr_output_blend_split_table_buf_reg(UINT8 *p_lut_split); //DCE_ENG_WDR_OUTPUT_BLEND_SPLIT_NUM
extern void dce_eng_set_wdr_output_blend_value_table_buf_reg(UINT16 *p_lut_val); //DCE_ENG_WDR_OUTPUT_BLEND_VAL_NUM

extern void dce_eng_set_cfa_enable_buf_reg(BOOL cfa_enable);
extern void dce_eng_set_cfa_subout_enable_buf_reg(BOOL cfa_subout_enable);
extern void dce_eng_set_cfa_buf_reg(DCE_ENG_CFA_PARAM *p_cfa_param);
extern void dce_eng_set_cfa_subout_buf_reg(DCE_ENG_CFA_SUBOUT *p_cfa_sub_out);
extern void dce_eng_set_cfa_interp_buf_reg(DCE_ENG_CFA_INTERP *p_color_interp);
extern void dce_eng_set_cfa_rb_correct_buf_reg(DCE_ENG_CFA_RB_CORR *p_rb_correct);
extern void dce_eng_set_cfa_false_color_supp_buf_reg(DCE_ENG_CFA_FCS *p_false_color_supp);
extern void dce_eng_set_cfa_rgbir_buf_reg(DCE_ENG_CFA_RGBIR *p_rgbir);
extern void dce_eng_set_cfa_color_gain_buf_reg(DCE_ENG_CFA_CGAIN *p_color_gain);
extern void dce_eng_set_cfa_pink_reduc_buf_reg(DCE_ENG_CFA_PINKR *p_pink_reduc);

extern void dce_eng_set_dc_enable_buf_reg(BOOL dc_enable);
extern void dce_eng_set_cac_enable_buf_reg(BOOL cac_enable);
extern void dce_eng_set_gdc_cac_buf_reg(DCE_ENG_GDC_CAC_PARAM *p_gdc_cac_param);
extern void dce_eng_set_gdc_cac_dist_norm_buf_reg(DCE_ENG_DIST_NORM *p_dist_norm);
extern void dce_eng_set_gdc_lut_buf_reg(UINT16 *p_gdc_lut); //DCE_ENG_GDC_TABLE_NUM
extern void dce_eng_set_cac_r_lut_buf_reg(INT16 *p_cac_lut_r); //DCE_ENG_CAC_TABLE_NUM
extern void dce_eng_set_cac_b_lut_buf_reg(INT16 *p_cac_lut_b); //DCE_ENG_CAC_TABLE_NUM
extern void dce_eng_set_2dlut_buf_reg(DCE_ENG_2DLUT_PARAM *p_2dlut_param);

extern void dce_eng_set_d2d_buf_reg(DCE_ENG_D2D_PARAM *p_d2d_param);
extern void dce_eng_set_dram_input_buf_reg(DCE_ENG_DRAM_IN_PARAM *p_dram_in);
extern void dce_eng_set_dram_output_buf_reg(DCE_ENG_DRAM_OUT_PARAM *p_dram_out);
extern void dce_eng_set_uv_out_disable_buf_reg(BOOL uv_out_disable);
extern void dce_eng_set_dma_burst_buf_reg(DCE_ENG_DMA_BURST_PARAM *p_dma_burst);

//------- check limitation for virtual register buffer --------
extern INT32 dce_eng_chk_limitation(UINT32 reg_base_addr, UINT32 reg_flag_addr);

//------- wait flag --------
VOID dce_eng_wait_flag_frame_end(BOOL clear_flag);

//------- for emulation ----------
#if defined (_NVT_EMULATION_)
extern BOOL dce_end_time_out_status;
#endif

#endif
