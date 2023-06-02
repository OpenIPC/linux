
#ifndef _IME_ENG_LCAF_BASE_H_
#define _IME_ENG_LCAF_BASE_H_

#ifdef __cplusplus
extern "C" {
#endif


#include "kwrap/type.h"
#include "ime_eng_int_comm.h"




extern VOID ime_eng_set_lcaf_enable_buf_reg(BOOL set_en);
extern VOID ime_eng_set_lcaf_filter_ychannel_enable_buf_reg(UINT32 op);
extern VOID ime_eng_set_lcaf_filter_size_buf_reg(UINT32 filter_size, UINT32 edge_size);



extern VOID ime_eng_set_lcaf_reference_center_cal_y_buf_reg(UINT8 *p_range_th_y, UINT8 wet_y, UINT8 *p_range_wet_y, UINT8 outlier_th_y);
extern VOID ime_eng_set_lcaf_reference_center_cal_uv_buf_reg(UINT8 *p_range_th_uv, UINT8 wet_uv, UINT8 *p_range_wet_uv, UINT8 outlier_th_uv);
extern VOID ime_eng_set_lcaf_outlier_difference_threshold_buf_reg(UINT32 dif_th_y, UINT32 dif_th_u, UINT32 dif_th_v);

extern VOID ime_eng_set_lcaf_edge_direction_threshold_buf_reg(UINT32 pn_th, UINT32 hv_th);
extern VOID ime_eng_set_lcaf_filter_computation_param_y_buf_reg(UINT8 *p_th_y, UINT8 *p_wet_y);
extern VOID ime_eng_set_lcaf_filter_computation_param_u_buf_reg(UINT8 *p_th_u, UINT8 *p_wet_u);
extern VOID ime_eng_set_lcaf_filter_computation_param_v_buf_reg(UINT8 *p_th_v, UINT8 *p_wet_v);

extern VOID ime_eng_set_lcaf_statistical_information_threshold_buf_reg(UINT32 u_th0, UINT32 u_th1, UINT32 v_th0, UINT32 v_th1);

extern VOID ime_eng_set_lcaf_debug_mode_buf_reg(UINT32 edge_en, UINT32 rc_en, UINT32 eng_en);


#if (IME_GET_API_EN == 1)


#endif


#ifdef __cplusplus
}  // extern "C" {
#endif

#endif // _IFE2_BASE_H_


