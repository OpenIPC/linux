
#ifndef _IME_ENG_3DNR_BASE_H_
#define _IME_ENG_3DNR_BASE_H_

#ifdef __cplusplus
extern "C" {
#endif


#include "kwrap/type.h"
#include "ime_eng_int_comm.h"

extern VOID ime_eng_set_tmnr_enable_buf_reg(UINT32 set_en);

extern VOID ime_eng_set_tmnr_out_ref_enable_buf_reg(UINT32 set_en);

extern VOID ime_eng_set_tmnr_out_ref_encoder_enable_buf_reg(UINT32 set_en);

extern VOID ime_eng_set_tmnr_in_ref_decoder_enable_buf_reg(UINT32 set_en);

extern VOID ime_eng_set_tmnr_in_ref_flip_enable_buf_reg(UINT32 set_en);

extern VOID ime_eng_set_tmnr_out_ref_flip_enable_buf_reg(UINT32 set_en);

extern VOID ime_eng_set_tmnr_ms_roi_output_enable_buf_reg(UINT32 set_en);

extern VOID ime_eng_set_tmnr_ms_roi_flip_enable_buf_reg(UINT32 set_en);

extern VOID ime_eng_set_tmnr_nr_luma_channel_enable_buf_reg(UINT32 set_en);

extern VOID ime_eng_set_tmnr_nr_chroma_channel_enable_buf_reg(UINT32 set_en);

extern VOID ime_eng_set_tmnr_statistic_data_output_enable_buf_reg(UINT32 set_en);

extern VOID ime_eng_set_tmnr_debug_buf_reg(UINT32 set_mode, UINT32 set_mv0);

extern VOID ime_eng_set_tmnr_y_pre_blur_strength_buf_reg(UINT32 pre_y_blur_str);

extern VOID ime_eng_set_tmnr_me_control_buf_reg(UINT32 set_update_mode, UINT32 set_bndy, UINT32 set_ds_mode);

extern VOID ime_eng_set_tmnr_me_sad_buf_reg(UINT32 set_sad_type, UINT32 set_sad_shift);

extern VOID ime_eng_set_tmnr_nr_false_color_control_enable_buf_reg(UINT32 set_en);

extern VOID ime_eng_set_tmnr_nr_false_color_control_strength_buf_reg(UINT32 set_val);

extern VOID ime_eng_set_tmnr_nr_center_zero_enable_buf_reg(UINT32 set_en);

extern VOID ime_eng_set_tmnr_ps_mv_check_enable_buf_reg(UINT32 set_en);

extern VOID ime_eng_set_tmnr_ps_mv_roi_check_enable_buf_reg(UINT32 set_en);

extern VOID ime_eng_set_tmnr_ps_control_buf_reg(UINT32 set_mv_info_mode, UINT32 set_ps_mode);

extern VOID ime_eng_set_tmnr_ne_sampling_horizontal_buf_reg(UINT32 set_start_pos, UINT32 set_step, UINT32 set_num);

extern VOID ime_eng_set_tmnr_ne_sampling_vertical_buf_reg(UINT32 set_start_pos, UINT32 set_step, UINT32 set_num);

extern VOID ime_eng_set_tmnr_me_sad_penalty_buf_reg(UINT16 *p_sad_penalty);

extern VOID ime_eng_set_tmnr_me_switch_threshold_buf_reg(UINT8 *p_me_switch_th, UINT8 me_switch_ratio);
extern VOID ime_eng_set_tmnr_me_cost_blending_ratio(UINT8 me_cost_blend);

extern VOID ime_eng_set_tmnr_me_detail_penalty_buf_reg(UINT8 *p_me_detail_penalty);

extern VOID ime_eng_set_tmnr_me_probability_buf_reg(UINT8 *p_me_probability);

extern VOID ime_eng_set_tmnr_me_rand_bit_buf_reg(UINT32 me_rand_bitx, UINT32 me_rand_bity);

extern VOID ime_eng_set_tmnr_me_threshold_buf_reg(UINT32 me_min_detail, UINT32 me_periodic_th);

extern VOID ime_eng_set_tmnr_md_final_threshold_buf_reg(UINT32 set_md_k1, UINT32 set_md_k2);

extern VOID ime_eng_set_tmnr_md_roi_final_threshold_buf_reg(UINT32 set_md_roi_k1, UINT32 set_md_roi_k2);

extern VOID ime_eng_set_tmnr_md_sad_coefs_buf_reg(UINT8 *p_set_coefa, UINT16 *p_set_coefb);

extern VOID ime_eng_set_tmnr_md_sad_standard_deviation_buf_reg(UINT16 *p_set_std);

extern VOID ime_eng_set_tmnr_mc_noise_base_level_buf_reg(UINT16 *p_set_base_level);

extern VOID ime_eng_set_tmnr_mc_edge_coefs_offset_buf_reg(UINT8 *p_set_coefa, UINT16 *p_set_coefb);

extern VOID ime_eng_set_tmnr_mc_sad_standard_deviation_buf_reg(UINT16 *p_set_std);

extern VOID ime_eng_set_tmnr_mc_final_threshold_buf_reg(UINT32 set_mc_k1, UINT32 set_mc_k2);

extern VOID ime_eng_set_tmnr_mc_roi_final_threshold_buf_reg(UINT32 set_mc_roi_k1, UINT32 set_mc_roi_k2);

extern VOID ime_eng_set_tmnr_ps_mv_threshold_buf_reg(UINT32 set_mv_th, UINT32 set_roi_mv_th);

extern VOID ime_eng_set_tmnr_ps_mix_ratio_buf_reg(UINT32 set_mix_rto0, UINT32 set_mix_rto1);

extern VOID ime_eng_set_tmnr_ps_mix_threshold_buf_reg(UINT32 set_mix_th0, UINT32 set_mix_th1);

extern VOID ime_eng_set_tmnr_ps_mix_slope_buf_reg(UINT32 set_mix_slp0, UINT32 set_mix_slp1);

extern VOID ime_eng_set_tmnr_ps_down_sample_buf_reg(UINT32 set_ds_th);

extern VOID ime_eng_set_tmnr_ps_roi_down_sample_buf_reg(UINT32 set_ds_roi_th);

extern VOID ime_eng_set_tmnr_ps_edge_control_buf_reg(UINT32 set_start_point, UINT32 set_th0, UINT32 set_th1, UINT32 set_slope);

extern VOID ime_eng_set_tmnr_ps_path_error_threshold_buf_reg(UINT32 set_th);

extern VOID ime_eng_set_tmnr_nr_type_buf_reg(UINT32 set_type);

extern VOID ime_eng_set_tmnr_nr_luma_residue_threshold_buf_reg(UINT8 set_th0, UINT8 set_th1, UINT8 set_th2);

extern VOID ime_eng_set_tmnr_nr_chroma_residue_threshold_buf_reg(UINT32 set_th);

extern VOID ime_eng_set_tmnr_nr_varied_frequency_filter_weight_buf_reg(UINT8 *p_set_wets);

extern VOID ime_eng_set_tmnr_nr_luma_filter_weight_buf_reg(UINT8 *p_set_wets);

extern VOID ime_eng_set_tmnr_nr_prefiltering_type_buf_reg(UINT8 pf_type);
extern VOID ime_eng_set_tmnr_nr_prefiltering_buf_reg(UINT8 *p_set_strs, UINT8 *p_set_rto);

extern VOID ime_eng_set_tmnr_nr_snr_control_buf_reg(UINT8 *p_set_strs, UINT32 set_th);

extern VOID ime_eng_set_tmnr_nr_tnr_control_buf_reg(UINT8 *p_set_strs, UINT32 set_th);

extern VOID ime_eng_set_tmnr_nr_luma_noise_reduction_buf_reg(UINT8 *p_set_lut, UINT8 *p_set_rto);

extern VOID ime_eng_set_tmnr_nr_chroma_noise_reduction_buf_reg(UINT8 *p_set_lut, UINT8 *p_set_rto);
extern VOID ime_eng_set_tmnr_nr_tf0_filter_strength_buf_reg(UINT8 *p_set_str_blur, UINT8 *p_set_str_y, UINT8 *p_set_str_c);


extern VOID ime_eng_set_tmnr_statistic_data_output_start_position_buf_reg(UINT32 set_start_x, UINT32 set_start_y);
extern VOID ime_eng_set_tmnr_statistic_data_output_sample_numbers_buf_reg(UINT32 set_num_x, UINT32 set_num_y);
extern VOID ime_eng_set_tmnr_statistic_data_output_sample_steps_buf_reg(UINT32 set_step_x, UINT32 set_step_y);



extern VOID ime_eng_set_tmnr_in_ref_y_lineoffset_buf_reg(UINT32 set_ofs_y);
extern VOID ime_eng_set_tmnr_in_ref_uv_lineoffset_buf_reg(UINT32 set_ofs_uv);
extern VOID ime_eng_set_tmnr_in_ref_y_addr_buf_reg(UINT32 set_addr_y);
extern VOID ime_eng_set_tmnr_in_ref_uv_addr_buf_reg(UINT32 set_addr_uv);

extern VOID ime_eng_set_tmnr_out_ref_y_lineoffset_buf_reg(UINT32 set_ofs_y);
extern VOID ime_eng_set_tmnr_out_ref_uv_lineoffset_buf_reg(UINT32 set_ofs_uv);
extern VOID ime_eng_set_tmnr_out_ref_y_addr_buf_reg(UINT32 set_addr_y);
extern VOID ime_eng_set_tmnr_out_ref_uv_addr_buf_reg(UINT32 set_addr_uv);

extern VOID ime_eng_set_tmnr_in_motion_status_lineoffset_buf_reg(UINT32 set_ofs);
extern VOID ime_eng_set_tmnr_in_motion_status_address_buf_reg(UINT32 set_addr);
extern VOID ime_eng_set_tmnr_out_motion_status_address_buf_reg(UINT32 set_addr);

extern VOID ime_eng_set_tmnr_out_roi_motion_status_lineoffset_buf_reg(UINT32 set_ofs);
extern VOID ime_eng_set_tmnr_out_roi_motion_status_address_buf_reg(UINT32 set_addr);

extern VOID ime_eng_set_tmnr_in_motion_vector_lineoffset_buf_reg(UINT32 set_ofs);
extern VOID ime_eng_set_tmnr_in_motion_vector_address_buf_reg(UINT32 set_addr);
extern VOID ime_eng_set_tmnr_out_motion_vector_address_buf_reg(UINT32 set_addr);

extern VOID ime_eng_set_tmnr_out_statistic_lineoffset_buf_reg(UINT32 set_ofs);
extern VOID ime_eng_set_tmnr_out_statistic_address_buf_reg(UINT32 set_addr);


extern VOID ime_eng_set_tmnr_fast_converge_enable_buf_reg(UINT8 set_en);
extern VOID ime_eng_set_tmnr_fast_converge_control_buf_reg(UINT8 set_sp, UINT8 set_step);
extern VOID ime_eng_set_tmnr_fast_converge_lineoffset_buf_reg(UINT32 set_lofs);
extern VOID ime_eng_set_tmnr_in_fast_converge_address_buf_reg(UINT32 set_addr);
extern VOID ime_eng_set_tmnr_out_fast_converge_address_buf_reg(UINT32 set_addr);

extern VOID ime_eng_set_tmnr_random_seed_reset_enable_buf_reg(UINT8 set_en);


#if (IME_GET_API_EN == 1)

extern UINT32 ime_eng_get_tmnr_out_ref_encoder_enable_buf_reg(void);
extern UINT32 ime_eng_get_tmnr_enable_buf_reg(VOID);

extern UINT32 ime_eng_get_tmnr_in_ref_y_addr_buf_reg(VOID);
extern UINT32 ime_eng_get_tmnr_in_ref_uv_lineoffset_buf_reg(VOID);
extern UINT32 ime_eng_get_tmnr_in_ref_y_lineoffset_buf_reg(VOID);
extern UINT32 ime_eng_get_tmnr_in_ref_uv_addr_buf_reg(VOID);

extern UINT32 ime_eng_get_tmnr_out_ref_uv_lineoffset_buf_reg(VOID);
extern UINT32 ime_eng_get_tmnr_out_ref_y_addr_buf_reg(VOID);
extern UINT32 ime_eng_get_tmnr_out_ref_y_lineoffset_buf_reg(VOID);
extern UINT32 ime_eng_get_tmnr_out_ref_uv_addr_buf_reg(VOID);

extern UINT32 ime_eng_get_tmnr_in_motion_status_address_buf_reg(VOID);
extern UINT32 ime_eng_get_tmnr_in_motion_status_lineoffset_buf_reg(VOID);
extern UINT32 ime_eng_get_tmnr_out_motion_status_address_buf_reg(VOID);

extern UINT32 ime_eng_get_tmnr_out_roi_motion_status_lineoffset_buf_reg(VOID);
extern UINT32 ime_eng_get_tmnr_out_roi_motion_status_address_buf_reg(VOID);

extern UINT32 ime_eng_get_tmnr_in_motion_vector_lineoffset_buf_reg(VOID);
extern UINT32 ime_eng_get_tmnr_in_motion_vector_address_buf_reg(VOID);
extern UINT32 ime_eng_get_tmnr_out_motion_vector_address_buf_reg(VOID);

extern UINT32 ime_eng_get_tmnr_out_statistic_lineoffset_buf_reg(VOID);
extern UINT32 ime_eng_get_tmnr_out_statistic_address_buf_reg(VOID);


extern UINT32 ime_eng_get_tmnr_sum_of_sad_value_buf_reg(VOID);
extern UINT32 ime_eng_get_tmnr_sum_of_mv_length_buf_reg(VOID);
extern UINT32 ime_eng_get_tmnr_total_sampling_number_buf_reg(VOID);

#endif


#ifdef __cplusplus
}
#endif


#endif // _IME_3DNR_REG_


