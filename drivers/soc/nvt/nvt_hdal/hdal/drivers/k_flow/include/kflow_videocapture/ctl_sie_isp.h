/**
    SIE Ctrl Layer, Algorithm Interface

    @file       ctl_sie_isp.h
    @ingroup    mISIE_Ctrl
    @note       None

    Copyright   Novatek Microelectronics Corp. 2012.  All rights reserved.
*/


#ifndef _CTL_SIE_ISP_H
#define _CTL_SIE_ISP_H

#include "kflow_common/isp_if.h"
#include "kflow_common/type_vdo.h"

#define CTL_SIE_ISP_STA_NUM				5	//ref. to KDRV_SIE_MAX_ENG
#define CTL_SIE_ISP_CA_MAX_WINNUM		32	//ref. to KDRV_SIE_CA_MAX_WINNUM
#define CTL_SIE_ISP_LA_MAX_WINNUM		32	//ref. to KDRV_SIE_LA_MAX_WINNUM
#define CTL_SIE_ISP_LA_HIST_NUM			64	//ref. to KDRV_SIE_LA_HIST_BIN
#define CTL_SIE_ISP_Y_MAX_WINNUM		128	//ref. to KDRV_SIE_YOUT_MAX_WIN_NUM
#define CTL_SIE_ISP_VA_MAX_WINNUM		16	//ref. to KDRV_SIE_VA_MAX_WINNUM
#define CTL_SIE_ISP_INDEP_VA_WIN_NUM	5	//ref. to KDRV_SIE_VA_INDEP_NUM

#define CTL_SIE_LA_GMA_TBL_NUM 			65	//ref. to KDRV_SIE_LA_GAMMA_TBL_NUM
#define CTL_SIE_DPC_MAX_PXCNT 			4096//ref. to KDRV_SIE_DPC_MAX_PXCNT
#define CTL_SIE_ECS_MAX_MAP_NUM 		65	//ref. to KDRV_SIE_ECS_MAX_MAP_NUM
#define CTL_SIE_COMPANDING_MAX_LEN 		12	//ref. to KDRV_SIE_COMPANDING_MAX_LEN

/**
	type for KFLOW_SIE_ROI_RATIO

	ratio_base: 			base of each roi ratio
	ca_crop_win_roi: 		ca crop window roi, ca crop window  = sie_crop_size * ca_crop_win_roi / ratio_base
	la_crop_win_roi: 		la crop window roi, la crop window  = sie_crop_size * la_crop_win_roi / ratio_base
	va_crop_win_roi: 		va crop window roi, va crop window  = sie_crop_size * va_crop_win_roi / ratio_base
	va_win_roi:				va window size roi, va win size = (va_crop_window / va_win_num) * va_win_roi / ratio_base
	va_indep_crop_win_roi:	indep va win crop roi, indep va window = sie_crop_size * va_indep_crop_win_roi / ratio_base
*/
typedef struct {
	UINT32 ratio_base;	//roi ratio base
	URECT ca_crop_win_roi;
	URECT la_crop_win_roi;
	URECT va_crop_win_roi;
	USIZE va_win_roi;
	URECT va_indep_crop_win_roi[CTL_SIE_ISP_INDEP_VA_WIN_NUM];
} CTL_SIE_ISP_ROI_RATIO;

/**
	type for CTL_SIE_SET_OB
*/
typedef struct {
	BOOL bypass_enable;	    ///< ob bypass enable, note that ECS/DGain will substract ob_val before function and add it back while end of the function
	UINT32 ob_ofs;			///< ob offset
} CTL_SIE_OB_PARAM;


typedef enum {
	CTL_SIE_CA_SRC_AFTER_OB = 0,
	CTL_SIE_CA_SRC_BEFORE_OB,
	ENUM_DUMMY4WORD(CTL_SIE_CA_SRC_SEL)
} CTL_SIE_CA_SRC_SEL;

/**
	type for CTL_SIE_SET_CA

	buffer calculation:
	CA Output R/G/B/Cnt/IRth @16bit for each window
	buffer_size = win_num_w * win_num_h * 8 * (16/8) bytes
	max_buffer_size = 32 * 32 * 5 * 16/8
*/
typedef struct {
	BOOL enable;			///< ca enable, ca output use sie channel 1
	USIZE win_num;			///< ca window number, 1x1 ~ 32x32

	BOOL th_enable;			///< ca threshold enable
	UINT16 g_th_l;			///< G threshold lower bound
	UINT16 g_th_u;			///< G threshold upper bound
	UINT16 r_th_l;			///< R threshold lower bound
	UINT16 r_th_u;			///< R threshold upper bound
	UINT16 b_th_l;			///< B threshold lower bound
	UINT16 b_th_u;			///< B threshold upper bound
	UINT16 p_th_l;			///< P threshold lower bound
	UINT16 p_th_u;			///< P threshold upper bound

	UINT32 irsub_r_weight;	///< IR weight for R channel, 0~255
	UINT32 irsub_g_weight;	///< IR weight for G channel, 0~255
	UINT32 irsub_b_weight;	///< IR weight for B channel, 0~255
	UINT32 ca_ob_ofs;
	CTL_SIE_CA_SRC_SEL ca_src;
} CTL_SIE_CA_PARAM;

/**
	type for KDRV_SIE_SET_LA

	buffer calculation:
	LA Output PreGamma Lum/PostGamma Lum @16bit for each window, Histogram 64bin@16bit
	buffer_size = win_num_w * win_num_h * 2 * (16/8) + 64 * (16/8) bytes
	max_buffer_size = (32 * 32 * 2 * 16/8) + (64 * 16/8)
*/
typedef enum {
	CTL_SIE_LA_SRC_POST_CG = 0,
	CTL_SIE_LA_SRC_PRE_CG,
	ENUM_DUMMY4WORD(CTL_SIE_LA_SRC_SEL)
} CTL_SIE_LA_SRC_SEL;

typedef enum {
	CTL_SIE_LA_HIST_SRC_POST_GMA = 0,
	CTL_SIE_LA_HIST_SRC_PRE_GMA,
	ENUM_DUMMY4WORD(CTL_SIE_LA_HIST_SRC_SEL)
} CTL_SIE_LA_HIST_SRC_SEL;

typedef enum {
	CTL_SIE_STCS_LA_RGB     = 0,        ///< use RGB channel, transfer to Y
	CTL_SIE_STCS_LA_G,                  ///< use G channel as Y
	ENUM_DUMMY4WORD(CTL_SIE_LA_RGB2Y_MOD_SEL)
} CTL_SIE_LA_RGB2Y_MOD_SEL;

typedef struct {
	BOOL enable;								///< la enable, la output use sie channel 2
	USIZE win_num;								///< la window number, 1x1 ~ 32x32
	CTL_SIE_LA_SRC_SEL la_src;					///< la source selection
	CTL_SIE_LA_RGB2Y_MOD_SEL la_rgb2y1mod;
	CTL_SIE_LA_RGB2Y_MOD_SEL la_rgb2y2mod;

	BOOL cg_enable;								///< la color gain enable, in 3.7 bits format
	UINT16 r_gain;
	UINT16 g_gain;
	UINT16 b_gain;

	BOOL gamma_enable;							///< la gamma enable
	UINT16 gamma_tbl[CTL_SIE_LA_GMA_TBL_NUM];	///< la gamma table, size CTL_SIE_LA_GMA_TBL_NUM

	BOOL hist_enable;							///< la histogram enable
	CTL_SIE_LA_HIST_SRC_SEL histo_src;			///< la histogram src, TRUE --> data before gamma, FALSE --> data after gamma
	UINT32 irsub_r_weight;						///< IR weight for R channel, 0~255
	UINT32 irsub_g_weight;						///< IR weight for G channel, 0~255
	UINT32 irsub_b_weight;						///< IR weight for B channel, 0~255
	UINT32 la_ob_ofs;
	BOOL   lath_enable;
	UINT8  lathy1lower;
	UINT8  lathy1upper;
	UINT8  lathy2lower;
	UINT8  lathy2upper;
} CTL_SIE_LA_PARAM;

/**
	CTL_SIE_SET_CGAIN
*/
typedef struct {
	BOOL enable;
	BOOL sel_37_fmt;	// 0 --> 2.8 bit gain format, 1 --> 3.7 bit gain format
	UINT16 r_gain; 		// r/gr/gb/b/Ir
	UINT16 gr_gain;
	UINT16 gb_gain;
	UINT16 b_gain;
	UINT16 ir_gain;
} CTL_SIE_CGAIN;


/**
	CTL_SIE_SET_DGAIN

	digital gain in 3.7 bits format@680
	digital gain in 8.8 bits format@528
*/

typedef struct {
	BOOL enable;
	UINT32 gain;
} CTL_SIE_DGAIN;

/**
	CTL_SIE_SET_DPC
*/
typedef enum {
	CTL_SIE_50F00_PERCENT = 0,		///< cross-channel weighting = 50% for defect concealment
	CTL_SIE_25F00_PERCENT,			///< cross-channel weighting = 25% for defect concealment
	CTL_SIE_12F50_PERCENT,			///< cross-channel weighting = 12.5% for defect concealment
	CTL_SIE_6F25_PERCENT,			///< cross-channel weighting = 6.25% for defect concealment
	ENUM_DUMMY4WORD(CTL_SIE_DPC_WEIGTH)
} CTL_SIE_DPC_WEIGTH;

typedef struct {
	BOOL enable;							///< dpc function enable
	UINT32 table[CTL_SIE_DPC_MAX_PXCNT];	///< dpc table addr
	CTL_SIE_DPC_WEIGTH weight;				///< dpc weigth select
	UINT32 dp_total_size;					///< defect pixel total size
} CTL_SIE_DPC;

/**
	CTL_SIE_SET_ECS
*/
typedef enum {
	CTL_SIE_ECS_MAP_65x65 = 0,
	CTL_SIE_ECS_MAP_49x49,
	CTL_SIE_ECS_MAP_33x33,
	ENUM_DUMMY4WORD(CTL_SIE_ECS_MAP_SEL)
} CTL_SIE_ECS_MAP_SEL;

/**
	CTL_SIE_SET_ECS
*/
typedef enum {
	CTL_SIE_ECS_3CH_10B = 0,
	CTL_SIE_ECS_4CH_8B,
	ENUM_DUMMY4WORD(CTL_SIE_ECS_BAYER_MODE_SEL)
} CTL_SIE_ECS_BAYER_MODE_SEL;

typedef struct {
	BOOL enable;
	BOOL sel_37_fmt;
	UINT32 map_tbl[CTL_SIE_ECS_MAX_MAP_NUM*CTL_SIE_ECS_MAX_MAP_NUM];
	CTL_SIE_ECS_MAP_SEL map_sel;

	BOOL dthr_enable;
	BOOL dthr_reset;
	UINT32 dthr_level;	///< dithering level; 0x0=bit[1:0], 0x7=bit[8:7]
	CTL_SIE_ECS_BAYER_MODE_SEL bayer_mode;
} CTL_SIE_ECS;

typedef struct {
	UINT32 decomp_kneepts[CTL_SIE_COMPANDING_MAX_LEN-1];
	UINT32 decomp_strpts[CTL_SIE_COMPANDING_MAX_LEN];
	UINT32 decomp_shiftpts[CTL_SIE_COMPANDING_MAX_LEN];
} CTL_SIE_DECOMPANDING_INFO;

typedef struct {
	UINT32 comp_kneepts[CTL_SIE_COMPANDING_MAX_LEN-1];
	UINT32 comp_strpts[CTL_SIE_COMPANDING_MAX_LEN];
	UINT32 comp_shiftpts[CTL_SIE_COMPANDING_MAX_LEN];
} CTL_SIE_COMPANDING_INFO;

typedef struct {
	BOOL enable;
	CTL_SIE_DECOMPANDING_INFO decomp_info;
	CTL_SIE_COMPANDING_INFO	comp_info;
	UINT16 comp_shift;
} CTL_SIE_COMPANDING;

typedef enum {
	CTL_SIE_MD_MASK_COL_ROW = 0,
	CTL_SIE_MD_MASK_8X8,
	ENUM_DUMMY4WORD(CTL_SIE_MD_MASK_MODE)
} CTL_SIE_MD_MASK_MODE;

typedef enum {
	CTL_SIE_MD_SRC_PREGAMMA= 0,
	CTL_SIE_MD_SRC_POSTGAMMA,
	ENUM_DUMMY4WORD(CTL_SIE_MD_SRC)
} CTL_SIE_MD_SRC;

typedef struct {
	BOOL enable;
	CTL_SIE_MD_SRC md_src;
	UINT8 sum_frms;	//range: 0~15 (1~16 frames)
	CTL_SIE_MD_MASK_MODE mask_mode;
	UINT32 mask0;
	UINT32 mask1;
	UINT32 blkdiff_thr;
	UINT32 total_blkdiff_thr;
	UINT16 blkdiff_cnt_thr;
} CTL_SIE_MD_PARAM;

typedef struct _CTL_SIE_IQ_PARAM {
	CTL_SIE_OB_PARAM	*ob_param;
	CTL_SIE_CA_PARAM    *ca_param;
	CTL_SIE_LA_PARAM    *la_param;
	CTL_SIE_CGAIN      	*cgain_param;
	CTL_SIE_DPC        	*dpc_param;
	CTL_SIE_ECS        	*ecs_param;
	CTL_SIE_COMPANDING	*companding_param;
	CTL_SIE_DGAIN      	*dgain_param;
	CTL_SIE_MD_PARAM 	*md_param;
} CTL_SIE_IQ_PARAM;

/**
	CTL_SIE_ETH_INFO

	use channel 4
	buffer calculation:
	eth output 2bit/8bit which is decided by b_8bit_sel for each pixel
	total pixle number is decide by b_out_sel
	if b_out_sel = true  --> (va_roi_w/2) * (va_roi_h/2) * bit_number

	note: eth source is from va_roi
	if va is enable  --> eth source is from va, no need to set eth roi and cg
	if va is disable --> need to set eth roi and cg if needed
*/
typedef struct {
	BOOL enable;		///< eth enable
	BOOL out_bit_sel;	///< 0 --> output 2 bit per pixel, 1 --> output 8 bit per pixel
	BOOL out_sel;		///< 0 --> output all pixel, 1 --> output pixel select by b_h_out_sel/b_v_out_sel
	BOOL h_out_sel;		///< 0 --> select even pixel to output, 1 --> select odd pixel to output
	BOOL v_out_sel;		///< 0 --> select even pixel to output, 1 --> select odd pixel to output

	UINT16 th_low;		///< edge threshold
	UINT16 th_mid;
	UINT16 th_high;
} CTL_SIE_ISP_ETH_INFO;

typedef struct {
	UINT32 buf_addr;
	UINT32 buf_size;	//[Set]kflow will force disable eth when buffer size < eth out size, [Get] none
	UINT32 roi_base;	//[Set]crop_roi ratio base, [Get]none
	URECT  crop_roi;	///< [Set]raio, [Get]pixel_value, source is sie crop win
	CTL_SIE_ISP_ETH_INFO eth_info;
	UINT32 out_lofs;	//[Set]none, [Get] return eth output lineoffset
} CTL_SIE_ISP_ETH_PARAM;

/**
	type for ISP_CA_RST
	ca result, array size should be ca window num_x * num_y
	user need to prepare memory for these pointer
	CA Output R/G/B/Cnt/IRth/Rth/Gth/Bth @16bit for each window
	each array size should be window num_x * num_y * 2(16bit)
*/
typedef struct {
	UINT16 r[CTL_SIE_ISP_CA_MAX_WINNUM * CTL_SIE_ISP_CA_MAX_WINNUM];
	UINT16 g[CTL_SIE_ISP_CA_MAX_WINNUM * CTL_SIE_ISP_CA_MAX_WINNUM];
	UINT16 b[CTL_SIE_ISP_CA_MAX_WINNUM * CTL_SIE_ISP_CA_MAX_WINNUM];
	UINT16 ir[CTL_SIE_ISP_CA_MAX_WINNUM * CTL_SIE_ISP_CA_MAX_WINNUM];
	UINT16 acc_cnt[CTL_SIE_ISP_CA_MAX_WINNUM * CTL_SIE_ISP_CA_MAX_WINNUM];
} CTL_SIE_ISP_CA_RSLT;

/**
	type for ISP_LA_RST
	la result, array size should be la window num_x * num_y
	histogram array size = 64
	user need to prepare memory for these pointer
	p_buf_lum_1 buffer size: la_win_w * la_win_h * 2
	p_buf_lum_2 buffer size: la_win_w * la_win_h * 2
	p_buf_histogram buffer size: 64 * 2
*/
typedef struct {
	UINT16 lum_1[CTL_SIE_ISP_LA_MAX_WINNUM * CTL_SIE_ISP_LA_MAX_WINNUM];	// pre gamma result
	UINT16 lum_2[CTL_SIE_ISP_LA_MAX_WINNUM * CTL_SIE_ISP_LA_MAX_WINNUM];	// post-gamma result
	UINT16 histogram[CTL_SIE_ISP_LA_HIST_NUM];	// [NT96680] only SIE 1~4 support hist out
} CTL_SIE_ISP_LA_RSLT;

/**
	type for CTL_SIE_ISP_ITEM_MD_RSLT
	md_th_rslt, array size should be 32x32x8
*/
typedef struct {
	UINT8   *md_th_rslt;
	UINT16  blk_dif_cnt;
	UINT32	total_blk_diff;
} CTL_SIE_ISP_MD_RSLT;

typedef struct _CTL_SIE_BP {
	UINT32 sie_bp_1;	//set bp value when trig evt include ISP_EVT_SIE_BP1
	UINT32 sie_bp_2;	//set bp value when trig evt include ISP_EVT_SIE_BP2
	UINT32 sie_bp_3;	//set bp value when trig evt include ISP_EVT_SIE_BP3
} CTL_SIE_BP;

typedef struct _CTL_SIE_ISP_IO_SIZE {
	URECT in_act_win;	//sie input active window
	URECT in_crp_win;	//sie input crop window
	USIZE out_sz;		//sie output size
} CTL_SIE_ISP_IO_SIZE;

typedef enum {
	CTL_SIE_ISP_CB_MSG_NONE 	= 0x00000000,
	CTL_SIE_ISP_CB_MSG_MAX,
	ENUM_DUMMY4WORD(CTL_SIE_ISP_CB_MSG)
} CTL_SIE_ISP_CB_MSG;

typedef struct {
	UINT32 buf_id;		///for public buffer control
	UINT32 buf_addr;
	VDO_FRAME vdo_frm;
} CTL_SIE_ISP_HEADER_INFO;

typedef struct {
	UINT32 buf_id;		///out, public buffer id
	UINT32 buf_addr;	///out, public buffer address
	UINT32 frm_cnt;		///in,  current image frame count
	UINT32 buf_size;	///in,  require buffer size
} CTL_SIE_ISP_SIM_BUF_NEW;

typedef struct {
	CTL_SIE_ISP_HEADER_INFO isp_head_info;
	BOOL sim_end;	//simulation end flag, kflow will set back to previous status
} CTL_SIE_ISP_SIM_BUF_PUSH;

typedef struct {
	UINT32 param_id;		///< get param_id, ref to KDRV_SIE_PARAM_ID
	void *data;				///< get result
} CTL_SIE_ISP_KDRV_PARAM;

typedef enum {
	CTL_SIE_ISP_STS_CLOSE = 0,
	CTL_SIE_ISP_STS_READY,
	CTL_SIE_ISP_STS_RUN,
	CTL_SIE_ISP_STS_MAX,
	ENUM_DUMMY4WORD(CTL_SIE_ISP_STATUS)
} CTL_SIE_ISP_STATUS;

typedef enum {
	CTL_SIE_ISP_ITEM_FUNC_EN = 0,	///< [GET],  	data_type: ISP_FUNC_EN
	CTL_SIE_ISP_ITEM_STATUS,		///< [GET],  	data_type: CTL_SIE_ISP_STATUS
	CTL_SIE_ISP_ITEM_IO_SIZE,		///< [GET],  	data_type: CTL_SIE_ISP_IO_SIZE
	CTL_SIE_ISP_ITEM_DUPL_SRC_ID,	///< [GET],  	data_type: CTL_SIE_ID
	CTL_SIE_ISP_ITEM_MULTI_FRM_GRP,	///< [GET],  	data_type: CTL_SEN_OUTPUT_DEST
	CTL_SIE_ISP_ITEM_CA_BUF,		///< [SET],  	data_type: CTL_SIE_ISP_CA_RSLT, SIE will output CA_RSLT when ISP_EVENT_SIE_DRAM_OUT0_END
	CTL_SIE_ISP_ITEM_LA_BUF,		///< [SET],  	data_type: CTL_SIE_ISP_LA_RSLT, SIE will output LA_RSLT when ISP_EVENT_SIE_DRAM_OUT0_END
	CTL_SIE_ISP_ITEM_STA_ROI_RATIO,	///< [SET],  	data_type: CTL_SIE_ISP_ROI_RATIO
	CTL_SIE_ISP_ITEM_IQ_PARAM,		///< [SET],  	data_type: CTL_SIE_IQ_PARAM
	CTL_SIE_ISP_ITEM_BP1,			///< [SET],  	data_type: KDRV_SIE_BP1, set sie bp1 value, disable bp1 interrupt when set bp1 to 0
	CTL_SIE_ISP_ITEM_BP2,			///< [SET],  	data_type: KDRV_SIE_BP2, set sie bp2 value, disable bp2 interrupt when set bp2 to 0
	CTL_SIE_ISP_ITEM_ETH,			///< N.S.
	CTL_SIE_ISP_ITEM_IMG_OUT,		///< [SET/GET], data_type: CTL_SIE_ISP_HEADER_INFO, note that set is used to unlock buffer
	CTL_SIE_ISP_ITEM_KDRV_PARAM,	///< [GET], 	data_type: CTL_SIE_ISP_KDRV_PARAM
	CTL_SIE_ISP_ITEM_SIM_BUF_NEW,	///< [SET], 	data_type: CTL_SIE_ISP_SIM_BUF_NEW
	CTL_SIE_ISP_ITEM_SIM_BUF_PUSH,	///< [SET], 	data_type: CTL_SIE_ISP_SIM_BUF_PUSH
	CTL_SIE_ISP_ITEM_MD_RSLT,		///< [GET], 	data_type: CTL_SIE_ISP_MD_RSLT
	CTL_SIE_ISP_ITEM_MAX,
	CTL_SIE_ISP_ISR_CTX = 0x80000000,///< combine this bit for announce this item is from isr context
	ENUM_DUMMY4WORD(CTL_SIE_ISP_ITEM)
} CTL_SIE_ISP_ITEM;

INT32 ctl_sie_isp_evt_fp_reg(CHAR *name, ISP_EVENT_FP fp, ISP_EVENT evt, CTL_SIE_ISP_CB_MSG cb_msg);
INT32 ctl_sie_isp_evt_fp_unreg(CHAR *name);
INT32 ctl_sie_isp_set(ISP_ID id, CTL_SIE_ISP_ITEM item, void *data);
INT32 ctl_sie_isp_get(ISP_ID id, CTL_SIE_ISP_ITEM item, void *data);

#endif
