

#ifndef _IME_ENG_LCA_BASE_H_
#define _IME_ENG_LCA_BASE_H_

#ifdef __cplusplus
extern "C" {
#endif



#include "kwrap/type.h"
#include "ime_eng_int_comm.h"



//-------------------------------------------------------------------------------
// local chroma adaption
extern VOID ime_eng_set_lca_enable_buf_reg(UINT32 set_en);
extern VOID ime_eng_set_lca_chroma_adj_enable_buf_reg(UINT32 set_en);
extern VOID ime_eng_set_lca_la_enable_buf_reg(UINT32 set_en);
extern VOID ime_eng_set_lca_bypass_enable_buf_reg(UINT32 set_en);

extern VOID ime_eng_set_lca_image_size_buf_reg(UINT32 size_h, UINT32 size_v);
extern VOID ime_eng_set_lca_scale_factor_buf_reg(UINT32 factor_h, UINT32 factor_v);
extern VOID ime_eng_set_lca_lineoffset_buf_reg(UINT32 lofs_y, UINT32 lofs_c);
extern VOID ime_eng_set_lca_dma_addr0_buf_reg(UINT32 addr_y);
extern VOID ime_eng_set_lca_format_buf_reg(UINT32 set_fmt);
extern VOID ime_eng_set_lca_source_buf_reg(UINT32 set_src);
extern VOID ime_eng_set_lca_ca_adjust_center_buf_reg(UINT32 u, UINT32 v);
extern VOID ime_eng_set_lca_ca_range_buf_reg(UINT32 rng, UINT32 wet_prc);
extern VOID ime_eng_set_lca_ca_weight_buf_reg(UINT32 th, UINT32 wt_s, UINT32 wt_e);
extern VOID ime_eng_set_lca_chroma_ref_image_weight_buf_reg(UINT32 ref_y_wt, UINT32 ref_c_wt, UINT32 out_wt);
extern VOID ime_eng_set_lca_luma_ref_image_weight_buf_reg(UINT32 ref_y_wt, UINT32 out_wt);
extern VOID ime_eng_set_lca_chroma_range_y_buf_reg(UINT32 rng, UINT32 wet_prc);
extern VOID ime_eng_set_lca_chroma_weight_y_buf_reg(UINT32 th, UINT32 wt_s, UINT32 wt_e);
extern VOID ime_eng_set_lca_chroma_range_uv_buf_reg(UINT32 rng, UINT32 wt_prc);
extern VOID ime_eng_set_lca_chroma_weight_uv_buf_reg(UINT32 th, UINT32 wt_s, UINT32 wt_e);
extern VOID ime_eng_set_lca_luma_range_y_buf_reg(UINT32 rng, UINT32 wt_prc);
extern VOID ime_eng_set_lca_luma_weight_y_buf_reg(UINT32 th, UINT32 wt_s, UINT32 wt_e);
extern VOID ime_eng_set_lca_input_data_by_pass_buf_reg(UINT8 set_in_bypass);

//-------------------------------------------------------------------------------
// LCA-Subout
extern VOID ime_eng_set_lca_subout_enable_buf_reg(UINT32 set_en);
extern VOID ime_eng_set_lca_subout_source_buf_reg(UINT32 src);
extern VOID ime_eng_set_lca_subout_scale_factor_h_buf_reg(UINT32 scl_rate, UINT32 scl_factor);
extern VOID ime_eng_set_lca_subout_scale_factor_v_buf_reg(UINT32 scl_rate, UINT32 scl_factor);
extern VOID ime_eng_set_lca_subout_isd_scale_factor_base_buf_reg(UINT32 scl_base_factor_h, UINT32 scl_base_factor_v);
extern VOID ime_eng_set_lca_subout_isd_scale_factor0_buf_reg(UINT32 scl_factor_h, UINT32 scl_factor_v);
extern VOID ime_eng_set_lca_subout_isd_scale_factor1_buf_reg(UINT32 scl_factor_h, UINT32 scl_factor_v);
extern VOID ime_eng_set_lca_subout_isd_scale_factor2_buf_reg(UINT32 scl_factor_h, UINT32 scl_factor_v);
extern VOID ime_eng_set_lca_subout_isd_scale_coef_number_buf_reg(UINT32 coef_num_h, UINT32 coef_num_v);
extern VOID ime_eng_set_lca_subout_lineoffset_buf_reg(UINT32 lofs);
extern VOID ime_eng_set_lca_subout_dma_addr_buf_reg(UINT32 addr);



#if (IME_GET_API_EN == 1)
extern UINT32 ime_eng_get_lca_enable_buf_reg(VOID);
extern VOID ime_eng_get_lca_image_size_buf_reg(UINT32 *p_get_size_h, UINT32 *p_get_size_v);
extern VOID ime_eng_get_lca_lineoffset_buf_reg(UINT32 *p_get_lofs_y, UINT32 *p_get_lofs_c);
extern VOID ime_eng_get_lca_dma_addr0_buf_reg(UINT32 *p_get_addr_y, UINT32 *p_get_addr_c);
extern UINT32 ime_eng_get_lca_format_buf_reg(VOID);


extern UINT32 ime_eng_get_lca_subout_enable_buf_reg(VOID);
extern UINT32 ime_eng_get_lca_subout_lineoffset_buf_reg(VOID);
extern UINT32 ime_eng_get_lca_subout_dma_addr_buf_reg(VOID);


#endif

#ifdef __cplusplus
}
#endif


#endif

