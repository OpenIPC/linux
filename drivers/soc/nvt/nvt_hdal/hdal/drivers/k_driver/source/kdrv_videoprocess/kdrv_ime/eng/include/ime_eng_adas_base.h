

#ifndef _IME_ENG_ADAS_BASE_H_
#define _IME_ENG_ADAS_BASE_H_

#ifdef __cplusplus
extern "C" {
#endif


#include "kwrap/type.h"
#include "ime_eng_int_comm.h"


//-------------------------------------------------------------------------------
// statistical
extern VOID ime_eng_set_adas_enable_buf_reg(UINT32 set_en);
extern VOID ime_eng_set_adas_flip_enable_buf_reg(UINT32 set_en);
extern VOID ime_eng_set_adas_median_filter_enable_buf_reg(UINT32 set_en);
extern VOID ime_eng_set_adas_after_filter_out_sel_buf_reg(UINT32 sel);
extern VOID ime_eng_set_adas_row_position_buf_reg(UINT32 row0, UINT32 row1);
extern VOID ime_eng_set_adas_edge_kernel_enable_buf_reg(UINT32 set_en0, UINT32 set_en1);
extern VOID ime_eng_set_adas_edge_kernel_sel_buf_reg(UINT32 ker_sel0, UINT32 ker_sel1);
extern VOID ime_eng_set_adas_edge_kernel_shift_buf_reg(UINT32 ker_sft0, UINT32 ker_sft1);
extern VOID ime_eng_set_adas_histogram_set0_buf_reg(UINT32 pos_h, UINT32 pos_v, UINT32 size_h, UINT32 size_v);
extern VOID ime_eng_set_adas_histogram_set1_buf_reg(UINT32 pos_h, UINT32 pos_v, UINT32 size_h, UINT32 size_v);
extern VOID ime_eng_set_adas_histogram_acc_target_set0_buf_reg(UINT32 acc_tag);
extern VOID ime_eng_set_adas_histogram_acc_target_set1_buf_reg(UINT32 acc_tag);
extern VOID ime_eng_set_adas_roi_threshold0_buf_reg(UINT32 th0, UINT32 th1, UINT32 th2, UINT32 src);
extern VOID ime_eng_set_adas_roi_threshold1_buf_reg(UINT32 th0, UINT32 th1, UINT32 th2, UINT32 src);
extern VOID ime_eng_set_adas_roi_threshold2_buf_reg(UINT32 th0, UINT32 th1, UINT32 th2, UINT32 src);
extern VOID ime_eng_set_adas_roi_threshold3_buf_reg(UINT32 th0, UINT32 th1, UINT32 th2, UINT32 src);
extern VOID ime_eng_set_adas_roi_threshold4_buf_reg(UINT32 th0, UINT32 th1, UINT32 th2, UINT32 src);
extern VOID ime_eng_set_adas_roi_threshold5_buf_reg(UINT32 th0, UINT32 th1, UINT32 th2, UINT32 src);
extern VOID ime_eng_set_adas_roi_threshold6_buf_reg(UINT32 th0, UINT32 th1, UINT32 th2, UINT32 src);
extern VOID ime_eng_set_adas_roi_threshold7_buf_reg(UINT32 th0, UINT32 th1, UINT32 th2, UINT32 src);
extern VOID ime_eng_set_adas_edge_map_dam_addr_buf_reg(UINT32 addr);
extern VOID ime_eng_set_adas_edge_map_dam_lineoffset_buf_reg(UINT32 lofs);
extern VOID ime_eng_set_adas_histogram_dam_addr_buf_reg(UINT32 addr);

#if (IME_GET_API_EN == 1)

extern UINT32 ime_eng_get_adas_edge_map_dam_lineoffset_buf_reg(VOID);
extern VOID ime_eng_get_adas_max_edge_buf_reg(UINT32 *p_get_max0, UINT32 *p_get_max1);
extern VOID ime_eng_get_adas_histogram_bin_buf_reg(UINT32 *p_get_hist_bin0, UINT32 *p_get_hist_bin1);
extern VOID ime_eng_get_adas_edge_map_addr_buf_reg(UINT32 *p_get_addr);
extern VOID ime_eng_get_adas_histogram_addr_buf_reg(UINT32 *p_get_addr);
extern UINT32 ime_eng_get_adas_enable_buf_reg(VOID);
extern VOID ime_eng_get_adas_burst_length_buf_reg(VOID);

#endif

#ifdef __cplusplus
}
#endif


#endif
