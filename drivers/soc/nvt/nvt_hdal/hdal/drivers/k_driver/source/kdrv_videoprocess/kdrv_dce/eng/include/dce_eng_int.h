#ifndef _DCE_ENG_INT_H_
#define _DCE_ENG_INT_H_

#ifdef __KERNEL__
#include "kwrap/type.h"
#include <mach/rcw_macro.h>
#include "kwrap/error_no.h"
#elif defined(__FREERTOS)
#include "kwrap/type.h"
#include "rcw_macro.h"
#include "kwrap/error_no.h"
#else
#endif


#include "dce_eng_int_reg.h"
#include "dce_eng.h"

#ifndef max
#define max(a, b)           (((a) > (b)) ? (a) : (b))
#endif
#ifndef min
#define min(a, b)           (((a) < (b)) ? (a) : (b))
#endif
#ifndef clamp
#define clamp(in, lb, ub)     (((in) < (lb)) ? (lb) : (((in) > (ub)) ? (ub) : (in)))
#endif

extern volatile NT98560_DCE_REGISTER_STRUCT *p_dce_reg;
extern volatile UINT8 *p_dce_reg_chg_flag;

#if 0
typedef enum {
	DCE_STPMODE_MST     = 0,  ///< DCE horizontal multiple stripe mode
	DCE_STPMODE_SST     = 1,  ///< DCE horizontal 1 stripe mode
	DCE_STPMODE_2ST     = 2,  ///< DCE horizontal 2 stripe mode
	DCE_STPMODE_3ST     = 3,  ///< DCE horizontal 3 stripe mode
	DCE_STPMODE_4ST     = 4,  ///< DCE horizontal 4 stripe mode
	DCE_STPMODE_5ST     = 5,  ///< DCE horizontal 5 stripe mode
	DCE_STPMODE_6ST     = 6,  ///< DCE horizontal 6 stripe mode
	DCE_STPMODE_7ST     = 7,  ///< DCE horizontal 7 stripe mode
	DCE_STPMODE_8ST     = 8,  ///< DCE horizontal 8 stripe mode
	ENUM_DUMMY4WORD(DCE_STP_MODE)
} DCE_STP_MODE;
#endif

typedef enum {
	DCSEL_GDC_CAC_ONLY = 0,    // GDC/CAC only
	DCSEL_2DLUT_ONLY   = 1,    // 2D lut only
	ENUM_DUMMY4WORD(DCE_DC_SEL)
} DCE_DC_SEL;

typedef struct {
	UINT16 buf_height_g;
	UINT16 buf_height_rgb;
	UINT16 buf_height_pix;// old design, always 0
	UINT16 h_stp_st_x;
	UINT16 h_stp_ed_x;
	UINT16 h_stp_clm_st_x;
	UINT16 h_stp_clm_ed_x;
	UINT8 h_stp_buf_mode;
} DCE_BUFFER_INFO;

ER dce_eng_int_cal_stripe_simple(UINT32 stripe_num, DCE_ENG_CAL_STP_INFO *p_cal_info, DCE_ENG_STP_RESULT *p_ns_result);
ER dce_eng_int_cal_stripe_with_dce_info(DCE_ENG_CAL_STP_INFO *p_cal_info, DCE_ENG_STP_RESULT *p_ns_result);

DCE_ENG_HSTP_IPEOLAP_SEL dce_eng_int_trans_ipe_overlap_to_sel(UINT32 ipe_overlap);
DCE_ENG_HSTP_IMEOLAP_SEL dce_eng_int_trans_ime_overlap_to_sel(UINT32 ime_overlap);
UINT32 dce_eng_int_get_stripe_num(DCE_ENG_CAL_STP_INFO *p_cal_info);

void dce_eng_int_set_mode_hw_reg(DCE_ENG_HANDLE *p_eng, DCE_ENG_CAL_STP_INFO *p_cal_info);
void dce_eng_int_set_interrupt_enable_hw_reg(DCE_ENG_HANDLE *p_eng, DCE_ENG_INTERRUPT inte_mask);
void dce_eng_int_set_interrupt_disable_hw_reg(DCE_ENG_HANDLE *p_eng, DCE_ENG_INTERRUPT inte_mask);
void dce_eng_int_set_input_size_hw_reg(DCE_ENG_HANDLE *p_eng, UINT32 width, UINT32 height);
void dce_eng_int_set_behavior_param_hw_reg(DCE_ENG_HANDLE *p_eng, DCE_ENG_CAL_STP_INFO *p_cal_info);
void dce_eng_int_set_gdc_cac_hw_reg(DCE_ENG_HANDLE *p_eng, DCE_ENG_CAL_STP_INFO *p_cal_info);
void dce_eng_int_set_gdc_cac_lut_hw_reg(DCE_ENG_HANDLE *p_eng, UINT16 * p_gdc_lut, INT16 *p_cac_lut_r, INT16 *p_cac_lut_b);
void dce_eng_int_set_crop_hw_reg(DCE_ENG_HANDLE *p_eng, UINT32 crop_width, UINT32 crop_hstart);
void dce_eng_int_set_load_hw_reg(DCE_ENG_HANDLE *p_eng, DCE_ENG_LOAD_TYPE type);
void dce_eng_int_get_buf_info_hw_reg(DCE_ENG_HANDLE *p_eng, DCE_BUFFER_INFO * p_hstp_status);

#endif
