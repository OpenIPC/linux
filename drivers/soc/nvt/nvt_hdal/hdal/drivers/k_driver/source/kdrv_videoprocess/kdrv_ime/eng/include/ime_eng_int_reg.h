
#ifndef _IME_ENG_INT_REG_H_
#define _IME_ENG_INT_REG_H_

#ifdef __cplusplus
extern "C" {
#endif

#if defined (__LINUX)

#include "rcw_macro.h" //tmp need to check (rcw_macro redefine issue)


#include "plat/top.h"
#include "kwrap/type.h"

//=========================================================================
#elif defined (__FREERTOS)


#include "rcw_macro.h"
#include "plat/top.h"
#include "kwrap/type.h"

#else


#endif



/*
    ime_rst                       :    [0x0, 0x1],          bits : 0
    ime_start                       :    [0x0, 0x1],            bits : 1
    ime_start_load                 :    [0x0, 0x1],         bits : 2
    ime_frameend_load               :    [0x0, 0x1],            bits : 3
    ime_drt_start_load   :    [0x0, 0x1],           bits : 4
    ime_gbl_load_en      :    [0x0, 0x1],           bits : 5
    ime_dmach_dis        :    [0x0, 0x1],           bits : 27
    ime_ll_fire          :    [0x0, 0x1],           bits : 28
*/
#define IME_ENGINE_CONTROL_REGISTE_OFS 0x0000
REGDEF_BEGIN(IME_ENGINE_CONTROL_REGISTE)
REGDEF_BIT(ime_rst,        1)
REGDEF_BIT(ime_start,        1)
REGDEF_BIT(ime_start_tload,        1)
REGDEF_BIT(ime_frameend_tload,        1)
REGDEF_BIT(ime_drt_start_load,        1)
REGDEF_BIT(ime_gbl_load_en,        1)
REGDEF_BIT(,        21)
REGDEF_BIT(ime_dmach_dis,        1)
REGDEF_BIT(ime_ll_fire,        1)
REGDEF_END(IME_ENGINE_CONTROL_REGISTE)


/*
    ime_src                         :    [0x0, 0x1],            bits : 0
    ime_dir_ctrl                       :    [0x0, 0x1],         bits : 1
    ime_p1_en                         :    [0x0, 0x1],          bits : 2
    ime_p2_en                         :    [0x0, 0x1],          bits : 3
    ime_p3_en                         :    [0x0, 0x1],          bits : 4
    ime_chra_en                     :    [0x0, 0x1],            bits : 10
    ime_chra_ca_en                   :    [0x0, 0x1],           bits : 11
    ime_chra_la_en                   :    [0x0, 0x1],           bits : 12
    ime_dbcs_en                     :    [0x0, 0x1],            bits : 13
    ime_ds_cst_en                     :    [0x0, 0x1],          bits : 16
    ime_ds_en0                       :    [0x0, 0x1],           bits : 17
    ime_ds_en1                       :    [0x0, 0x1],           bits : 18
    ime_ds_en2                       :    [0x0, 0x1],           bits : 19
    ime_ds_en3                       :    [0x0, 0x1],           bits : 20
    ime_ds_plt_sel         :    [0x0, 0x3],         bits : 22_21
    ime_chra_subout_en     :    [0x0, 0x1],         bits : 25
    ime_p1_enc_en          :    [0x0, 0x1],         bits : 26
    ime_3dnr_en            :    [0x0, 0x1],         bits : 27
    ime_tmnr_ref_in_dec_en :    [0x0, 0x1],         bits : 29
    ime_tmnr_ref_out_en    :    [0x0, 0x1],         bits : 30
    ime_tmnr_ref_out_enc_en:    [0x0, 0x1],         bits : 31
*/
#define IME_FUNCTION_CONTROL_REGISTER0_OFS 0x0004
REGDEF_BEGIN(IME_FUNCTION_CONTROL_REGISTER0)
REGDEF_BIT(ime_src,        1)
REGDEF_BIT(ime_dir_ctrl,        1)
REGDEF_BIT(ime_p1_en,        1)
REGDEF_BIT(ime_p2_en,        1)
REGDEF_BIT(ime_p3_en,        1)
//REGDEF_BIT(ime_p4_en,        1)
REGDEF_BIT(,        5)
REGDEF_BIT(ime_chra_en,        1)
REGDEF_BIT(ime_chra_ca_en,        1)
REGDEF_BIT(ime_chra_la_en,        1)
REGDEF_BIT(ime_dbcs_en,        1)
//REGDEF_BIT(ime_stl_en,        1)
//REGDEF_BIT(ime_stl_ftr_en,        1)
REGDEF_BIT(,        2)
REGDEF_BIT(ime_ds_cst_en,        1)
REGDEF_BIT(ime_ds_en0,        1)
REGDEF_BIT(ime_ds_en1,        1)
REGDEF_BIT(ime_ds_en2,        1)
REGDEF_BIT(ime_ds_en3,        1)
REGDEF_BIT(ime_ds_plt_sel,        2)
REGDEF_BIT(,        2)
REGDEF_BIT(ime_chra_subout_en,        1)
REGDEF_BIT(ime_p1_enc_en,        1)
REGDEF_BIT(ime_3dnr_en,        1)
REGDEF_BIT(,        1)
REGDEF_BIT(ime_tmnr_ref_in_dec_en,        1)
REGDEF_BIT(ime_tmnr_ref_out_en,        1)
REGDEF_BIT(ime_tmnr_ref_out_enc_en,        1)
REGDEF_END(IME_FUNCTION_CONTROL_REGISTER0)


/*
    ime_pm0_en                       :    [0x0, 0x1],           bits : 0
    ime_pm1_en                       :    [0x0, 0x1],           bits : 1
    ime_pm2_en                       :    [0x0, 0x1],           bits : 2
    ime_pm3_en                       :    [0x0, 0x1],           bits : 3
    ime_pm4_en                       :    [0x0, 0x1],           bits : 4
    ime_pm5_en                       :    [0x0, 0x1],           bits : 5
    ime_pm6_en                       :    [0x0, 0x1],           bits : 6
    ime_pm7_en                       :    [0x0, 0x1],           bits : 7
    ime_low_dly_en         :    [0x0, 0x1],         bits : 8
    ime_low_dly_sel        :    [0x0, 0x3],         bits : 10_9
    ime_p1_flip_en         :    [0x0, 0x1],         bits : 11
    ime_p2_flip_en         :    [0x0, 0x1],         bits : 12
    ime_p3_flip_en         :    [0x0, 0x1],         bits : 13
    ime_3dnr_inref_flip_en :    [0x0, 0x1],         bits : 16
    ime_3dnr_outref_flip_en:    [0x0, 0x1],         bits : 17
    ime_3dnr_ms_roi_flip_en:    [0x0, 0x1],         bits : 18
    ime_ycc_cvt_en         :    [0x0, 0x1],         bits : 20
    ime_ycc_cvt_sel        :    [0x0, 0x1],         bits : 21
*/
#define IME_FUNCTION_CONTROL_REGISTER1_OFS 0x0008
REGDEF_BEGIN(IME_FUNCTION_CONTROL_REGISTER1)
REGDEF_BIT(ime_pm0_en,        1)
REGDEF_BIT(ime_pm1_en,        1)
REGDEF_BIT(ime_pm2_en,        1)
REGDEF_BIT(ime_pm3_en,        1)
REGDEF_BIT(ime_pm4_en,        1)
REGDEF_BIT(ime_pm5_en,        1)
REGDEF_BIT(ime_pm6_en,        1)
REGDEF_BIT(ime_pm7_en,        1)
REGDEF_BIT(ime_low_dly_en,        1)
REGDEF_BIT(ime_low_dly_sel,        2)
REGDEF_BIT(ime_p1_flip_en,        1)
REGDEF_BIT(ime_p2_flip_en,        1)
REGDEF_BIT(ime_p3_flip_en,        1)
//REGDEF_BIT(ime_p4_flip_en,        1)
//REGDEF_BIT(ime_stl_flip_en,        1)
REGDEF_BIT(,        2)
REGDEF_BIT(ime_3dnr_inref_flip_en,        1)
REGDEF_BIT(ime_3dnr_outref_flip_en,        1)
REGDEF_BIT(ime_3dnr_ms_roi_flip_en,        1)
REGDEF_BIT(,        1)
REGDEF_BIT(ime_ycc_cvt_en,        1)
REGDEF_BIT(ime_ycc_cvt_sel,        1)
REGDEF_END(IME_FUNCTION_CONTROL_REGISTER1)


/*
    ime_lca_subout_dram_out_single_en :    [0x0, 0x1],          bits : 0
    ime_3dnr_refout_dram_out_single_en:    [0x0, 0x1],          bits : 1
    ime_3dnr_ms_dram_out_single_en    :    [0x0, 0x1],          bits : 2
    ime_3dnr_ms_roi_dram_out_single_en:    [0x0, 0x1],          bits : 3
    ime_3dnr_mv_dram_out_single_en    :    [0x0, 0x1],          bits : 4
    ime_3dnr_sta_dram_out_single_en   :    [0x0, 0x1],          bits : 5
    ime_outp1_dram_out_single_en      :    [0x0, 0x1],          bits : 6
    ime_outp2_dram_out_single_en      :    [0x0, 0x1],          bits : 7
    ime_outp3_dram_out_single_en      :    [0x0, 0x1],          bits : 8
    ime_3dnr_fc_dram_out_single_en    :    [0x0, 0x1],          bits : 10
    ime_dram_out_mode                 :    [0x0, 0x1],          bits : 31
*/
#define IME_DRAM_SINGLE_OUTPUT_CONTROL_REGISTER_OFS 0x000c
REGDEF_BEGIN(IME_DRAM_SINGLE_OUTPUT_CONTROL_REGISTER)
REGDEF_BIT(ime_lca_subout_dram_out_single_en,        1)
REGDEF_BIT(ime_3dnr_refout_dram_out_single_en,        1)
REGDEF_BIT(ime_3dnr_ms_dram_out_single_en,        1)
REGDEF_BIT(ime_3dnr_ms_roi_dram_out_single_en,        1)
REGDEF_BIT(ime_3dnr_mv_dram_out_single_en,        1)
REGDEF_BIT(ime_3dnr_sta_dram_out_single_en,        1)
REGDEF_BIT(ime_outp1_dram_out_single_en,        1)
REGDEF_BIT(ime_outp2_dram_out_single_en,        1)
REGDEF_BIT(ime_outp3_dram_out_single_en,        1)
REGDEF_BIT(,        1)
REGDEF_BIT(ime_3dnr_fc_dram_out_single_en,        1)
//REGDEF_BIT(ime_outp4_dram_out_single_en,        1)
REGDEF_BIT(reserved,        20)
REGDEF_BIT(ime_dram_out_mode,        1)
REGDEF_END(IME_DRAM_SINGLE_OUTPUT_CONTROL_REGISTER)


/*
    ime_ll_terminate:    [0x0, 0x1],            bits : 0
*/
#define IME_LINKED_LIST_CONTROL_REGISTER1_OFS 0x0010
REGDEF_BEGIN(IME_LINKED_LIST_CONTROL_REGISTER1)
REGDEF_BIT(ime_ll_terminate,        1)
REGDEF_BIT(,        3)
REGDEF_BIT(ime_dmach_idle,        1)
REGDEF_END(IME_LINKED_LIST_CONTROL_REGISTER1)


/*
    ime_dram_ll_sai:    [0x0, 0x3fffffff],          bits : 31_2
*/
#define IME_LINKED_LIST_INPUT_DMA_CHANNEL_REGISTER_OFS 0x0014
REGDEF_BEGIN(IME_LINKED_LIST_INPUT_DMA_CHANNEL_REGISTER)
REGDEF_BIT(,        2)
REGDEF_BIT(ime_dram_ll_sai,        30)
REGDEF_END(IME_LINKED_LIST_INPUT_DMA_CHANNEL_REGISTER)


/*
    ime_inte_ll_end               :    [0x0, 0x1],          bits : 0
    ime_inte_ll_err               :    [0x0, 0x1],          bits : 1
    ime_inte_ll_red_late          :    [0x0, 0x1],          bits : 2
    ime_inte_ll_job_end           :    [0x0, 0x1],          bits : 3
    ime_inte_in_bp1               :    [0x0, 0x1],          bits : 4
    ime_inte_in_bp2               :    [0x0, 0x1],          bits : 5
    ime_inte_in_bp3               :    [0x0, 0x1],          bits : 6
    ime_inte_3dnr_slice_end       :    [0x0, 0x1],          bits : 7
    ime_inte_3dnr_mot_end         :    [0x0, 0x1],          bits : 8
    ime_inte_3dnr_mv_end          :    [0x0, 0x1],          bits : 9
    ime_inte_3dnr_statsitic_end   :    [0x0, 0x1],          bits : 10
    ime_inte_p1_out_enc_ovfl      :    [0x0, 0x1],          bits : 11
    ime_inte_3dnr_ref_out_enc_ovfl:    [0x0, 0x1],          bits : 12
    ime_inte_3dnr_ref_in_dec_err  :    [0x0, 0x1],          bits : 13
    ime_inte_frm_err              :    [0x0, 0x1],          bits : 14
    reserved                      :    [0x0, 0x3fff],           bits : 28_15
    ime_inte_frm_start            :    [0x0, 0x1],          bits : 29
    ime_inte_strp_end             :    [0x0, 0x1],          bits : 30
    ime_inte_frm_end              :    [0x0, 0x1],          bits : 31
*/
#define IME_INTERRUPT_ENABLE_REGISTER_OFS 0x0018
REGDEF_BEGIN(IME_INTERRUPT_ENABLE_REGISTER)
REGDEF_BIT(ime_intpe_ll_end,        1)
REGDEF_BIT(ime_intpe_ll_err,        1)
REGDEF_BIT(ime_intpe_ll_red_late,        1)
REGDEF_BIT(ime_intpe_ll_job_end,        1)
REGDEF_BIT(ime_intpe_in_bp1,        1)
REGDEF_BIT(ime_intpe_in_bp2,        1)
REGDEF_BIT(ime_intpe_in_bp3,        1)
REGDEF_BIT(ime_intpe_3dnr_slice_end,        1)
REGDEF_BIT(ime_intpe_3dnr_mot_end,        1)
REGDEF_BIT(ime_intpe_3dnr_mv_end,        1)
REGDEF_BIT(ime_intpe_3dnr_statsitic_end,        1)
REGDEF_BIT(ime_intpe_p1_out_enc_ovfl,        1)
REGDEF_BIT(ime_intpe_3dnr_ref_out_enc_ovfl,        1)
REGDEF_BIT(ime_intpe_3dnr_ref_in_dec_err,        1)
REGDEF_BIT(ime_intpe_frm_err,        1)
REGDEF_BIT(reserved,        14)
REGDEF_BIT(ime_intpe_frm_start,        1)
REGDEF_BIT(ime_intpe_strp_end,        1)
REGDEF_BIT(ime_intpe_frm_end,        1)
REGDEF_END(IME_INTERRUPT_ENABLE_REGISTER)


/*
    ime_ints_ll_end               :    [0x0, 0x1],          bits : 0
    ime_ints_ll_err               :    [0x0, 0x1],          bits : 1
    ime_ints_ll_red_late          :    [0x0, 0x1],          bits : 2
    ime_ints_ll_job_end           :    [0x0, 0x1],          bits : 3
    ime_ints_in_bp1               :    [0x0, 0x1],          bits : 4
    ime_ints_in_bp2               :    [0x0, 0x1],          bits : 5
    ime_ints_in_bp3               :    [0x0, 0x1],          bits : 6
    ime_ints_3dnr_slice_end       :    [0x0, 0x1],          bits : 7
    ime_ints_3dnr_mot_end         :    [0x0, 0x1],          bits : 8
    ime_ints_3dnr_mv_end          :    [0x0, 0x1],          bits : 9
    ime_ints_3dnr_statsitic_end   :    [0x0, 0x1],          bits : 10
    ime_ints_p1_out_enc_ovfl      :    [0x0, 0x1],          bits : 11
    ime_ints_3dnr_ref_out_enc_ovfl:    [0x0, 0x1],          bits : 12
    ime_ints_3dnr_ref_in_dec_err  :    [0x0, 0x1],          bits : 13
    ime_ints_frm_err              :    [0x0, 0x1],          bits : 14
    reserved                      :    [0x0, 0x3fff],           bits : 28_15
    ime_ints_frm_start            :    [0x0, 0x1],          bits : 29
    ime_ints_strp_end             :    [0x0, 0x1],          bits : 30
    ime_ints_frm_end              :    [0x0, 0x1],          bits : 31
*/
#define IME_STATUS_REGISTER_OFS 0x001c
REGDEF_BEGIN(IME_STATUS_REGISTER)
REGDEF_BIT(ime_intps_ll_end,        1)
REGDEF_BIT(ime_intps_ll_err,        1)
REGDEF_BIT(ime_intps_ll_red_late,        1)
REGDEF_BIT(ime_intps_ll_job_end,        1)
REGDEF_BIT(ime_intps_in_bp1,        1)
REGDEF_BIT(ime_intps_in_bp2,        1)
REGDEF_BIT(ime_intps_in_bp3,        1)
REGDEF_BIT(ime_intps_3dnr_slice_end,        1)
REGDEF_BIT(ime_intps_3dnr_mot_end,        1)
REGDEF_BIT(ime_intps_3dnr_mv_end,        1)
REGDEF_BIT(ime_intps_3dnr_statsitic_end,        1)
REGDEF_BIT(ime_intps_p1_out_enc_ovfl,       1)
REGDEF_BIT(ime_intps_3dnr_ref_out_enc_ovfl,       1)
REGDEF_BIT(ime_intps_3dnr_ref_in_dec_err,       1)
REGDEF_BIT(ime_intps_frm_err,       1)
REGDEF_BIT(reserved,        14)
REGDEF_BIT(ime_intps_frm_start,        1)
REGDEF_BIT(ime_intps_strp_end,        1)
REGDEF_BIT(ime_intps_frm_end,        1)
REGDEF_END(IME_STATUS_REGISTER)


/*
    ime_in_h_size               :    [0x0, 0x3fff],         bits : 15_2
    ime_in_v_size               :    [0x0, 0x3fff],         bits : 31_18
*/
#define IME_INPUT_IMAGE_SIZE_REGISTER_OFS 0x0020
REGDEF_BEGIN(IME_INPUT_IMAGE_SIZE_REGISTER)
REGDEF_BIT(,        2)
REGDEF_BIT(ime_in_h_size,        14)
REGDEF_BIT(,        2)
REGDEF_BIT(ime_in_v_size,        14)
REGDEF_END(IME_INPUT_IMAGE_SIZE_REGISTER)


/*
    ime_imat                    :    [0x0, 0x7],            bits : 2_0
    st_hovlp_sel                :    [0x0, 0x3],            bits : 5_4
    st_prt_sel                :    [0x0, 0x3],          bits : 7_6
    ime_st_size_mode:    [0x0, 0x1],            bits : 8
*/
#define IME_INPUT_STRIPE_HORIZONTAL_DIMENSION0_OFS 0x0024
REGDEF_BEGIN(IME_INPUT_STRIPE_HORIZONTAL_DIMENSION0)
REGDEF_BIT(ime_imat,        3)
REGDEF_BIT(,        1)
REGDEF_BIT(st_hovlp_sel,        2)
REGDEF_BIT(st_prt_sel,        2)
REGDEF_BIT(ime_st_size_mode,        1)
REGDEF_END(IME_INPUT_STRIPE_HORIZONTAL_DIMENSION0)


/*
    st_hn               :    [0x0, 0x7ff],          bits : 10_0
    st_hl               :    [0x0, 0x7ff],          bits : 22_12
    st_hm               :    [0x0, 0xff],           bits : 31_24
*/
#define IME_INPUT_STRIPE_HORIZONTAL_DIMENSION1_OFS 0x0028
REGDEF_BEGIN(IME_INPUT_STRIPE_HORIZONTAL_DIMENSION1)
REGDEF_BIT(st_hn,        11)
REGDEF_BIT(,        1)
REGDEF_BIT(st_hl,        11)
REGDEF_BIT(,        1)
REGDEF_BIT(st_hm,        8)
REGDEF_END(IME_INPUT_STRIPE_HORIZONTAL_DIMENSION1)


/*
    st_vl                  :    [0x0, 0xffff],          bits : 15_0
    st_prt                :    [0x0, 0xff],         bits : 23_16
    st_hovlp                :    [0x0, 0xff],           bits : 31_24
*/
#define IME_INPUT_STRIPE_VERTICAL_DIMENSION_OFS 0x002c
REGDEF_BEGIN(IME_INPUT_STRIPE_VERTICAL_DIMENSION)
REGDEF_BIT(st_vl,        16)
REGDEF_BIT(st_prt,        8)
REGDEF_BIT(st_hovlp,        8)
REGDEF_END(IME_INPUT_STRIPE_VERTICAL_DIMENSION)


/*
    ime_y_dram_ofsi:    [0x0, 0x3ffff],         bits : 19_2
*/
#define IME_INPUT_DMA_LINEOFFSET_REGISTER0_OFS 0x0030
REGDEF_BEGIN(IME_INPUT_DMA_LINEOFFSET_REGISTER0)
REGDEF_BIT(,        2)
REGDEF_BIT(ime_y_dram_ofsi,        18)
REGDEF_END(IME_INPUT_DMA_LINEOFFSET_REGISTER0)


/*
    ime_uv_dram_ofsi:    [0x0, 0x3ffff],            bits : 19_2
*/
#define IME_INPUT_DMA_LINEOFFSET_REGISTER1_OFS 0x0034
REGDEF_BEGIN(IME_INPUT_DMA_LINEOFFSET_REGISTER1)
REGDEF_BIT(,        2)
REGDEF_BIT(ime_uv_dram_ofsi,        18)
REGDEF_END(IME_INPUT_DMA_LINEOFFSET_REGISTER1)


/*
    ime_y_dram_sai:    [0x0, 0x3fffffff],           bits : 31_2
*/
#define IME_INPUT_DMA_REGISTER0_OFS 0x0038
REGDEF_BEGIN(IME_INPUT_DMA_REGISTER0)
REGDEF_BIT(,        2)
REGDEF_BIT(ime_y_dram_sai,        30)
REGDEF_END(IME_INPUT_DMA_REGISTER0)


/*
    ime_u_dram_sai:    [0x0, 0x3fffffff],           bits : 31_2
*/
#define IME_INPUT_DMA_REGISTER1_OFS 0x003c
REGDEF_BEGIN(IME_INPUT_DMA_REGISTER1)
REGDEF_BIT(,        2)
REGDEF_BIT(ime_u_dram_sai,        30)
REGDEF_END(IME_INPUT_DMA_REGISTER1)


/*
    ime_v_dram_sai:    [0x0, 0x3fffffff],           bits : 31_2
*/
#define IME_INPUT_DMA_REGISTER2_OFS 0x0040
REGDEF_BEGIN(IME_INPUT_DMA_REGISTER2)
REGDEF_BIT(,        2)
REGDEF_BIT(ime_v_dram_sai,        30)
REGDEF_END(IME_INPUT_DMA_REGISTER2)


/*
    ime_st_hn0:    [0x0, 0x3ff],            bits : 9_0
    ime_st_hn1:    [0x0, 0x3ff],            bits : 20_11
    ime_st_hn2:    [0x0, 0x3ff],            bits : 31_22
*/
#define IME_VARIED_STRIPE_SIZE_REGISTER0_OFS 0x0044
REGDEF_BEGIN(IME_VARIED_STRIPE_SIZE_REGISTER0)
REGDEF_BIT(ime_st_hn0,        10)
REGDEF_BIT(,        1)
REGDEF_BIT(ime_st_hn1,        10)
REGDEF_BIT(,        1)
REGDEF_BIT(ime_st_hn2,        10)
REGDEF_END(IME_VARIED_STRIPE_SIZE_REGISTER0)


/*
    ime_st_hn3:    [0x0, 0x3ff],            bits : 9_0
    ime_st_hn4:    [0x0, 0x3ff],            bits : 20_11
    ime_st_hn5:    [0x0, 0x3ff],            bits : 31_22
*/
#define IME_VARIED_STRIPE_SIZE_REGISTER1_OFS 0x0048
REGDEF_BEGIN(IME_VARIED_STRIPE_SIZE_REGISTER1)
REGDEF_BIT(ime_st_hn3,        10)
REGDEF_BIT(,        1)
REGDEF_BIT(ime_st_hn4,        10)
REGDEF_BIT(,        1)
REGDEF_BIT(ime_st_hn5,        10)
REGDEF_END(IME_VARIED_STRIPE_SIZE_REGISTER1)


/*
    ime_st_hn6    :    [0x0, 0x3ff],            bits : 9_0
    ime_st_hn7    :    [0x0, 0x3ff],            bits : 20_11
    ime_st_msb_hn0:    [0x0, 0x1],          bits : 24
    ime_st_msb_hn1:    [0x0, 0x1],          bits : 25
    ime_st_msb_hn2:    [0x0, 0x1],          bits : 26
    ime_st_msb_hn3:    [0x0, 0x1],          bits : 27
    ime_st_msb_hn4:    [0x0, 0x1],          bits : 28
    ime_st_msb_hn5:    [0x0, 0x1],          bits : 29
    ime_st_msb_hn6:    [0x0, 0x1],          bits : 30
    ime_st_msb_hn7:    [0x0, 0x1],          bits : 31
*/
#define IME_VARIED_STRIPE_SIZE_REGISTER2_OFS 0x004c
REGDEF_BEGIN(IME_VARIED_STRIPE_SIZE_REGISTER2)
REGDEF_BIT(ime_st_hn6,        10)
REGDEF_BIT(,        1)
REGDEF_BIT(ime_st_hn7,        10)
REGDEF_BIT(,        3)
REGDEF_BIT(ime_st_msb_hn0,        1)
REGDEF_BIT(ime_st_msb_hn1,        1)
REGDEF_BIT(ime_st_msb_hn2,        1)
REGDEF_BIT(ime_st_msb_hn3,        1)
REGDEF_BIT(ime_st_msb_hn4,        1)
REGDEF_BIT(ime_st_msb_hn5,        1)
REGDEF_BIT(ime_st_msb_hn6,        1)
REGDEF_BIT(ime_st_msb_hn7,        1)
REGDEF_END(IME_VARIED_STRIPE_SIZE_REGISTER2)


/*
    reserved:    [0x0, 0xffffffff],         bits : 31_0
*/
#define IME_RESERVED_2_OFS 0x0050
REGDEF_BEGIN(IME_RESERVED_2)
REGDEF_BIT(reserved,        32)
REGDEF_END(IME_RESERVED_2)


/*
    reserved:    [0x0, 0xffffffff],         bits : 31_0
*/
#define IME_RESERVED_3_OFS 0x0054
REGDEF_BEGIN(IME_RESERVED_3)
REGDEF_BIT(reserved,        32)
REGDEF_END(IME_RESERVED_3)


/*
    ime_dend_sts_en:    [0x0, 0x1],         bits : 0
    ime_dend_wbit  :    [0x0, 0x1f],            bits : 8_4
*/
#define IME_DRAM_END_OUTPUT_STATUS_REGISTER0_OFS 0x0058
REGDEF_BEGIN(IME_DRAM_END_OUTPUT_STATUS_REGISTER0)
REGDEF_BIT(ime_dend_sts_en,        1)
REGDEF_BIT(,        3)
REGDEF_BIT(ime_dend_wbit,        5)
REGDEF_END(IME_DRAM_END_OUTPUT_STATUS_REGISTER0)


/*
    ime_dend_status:    [0x0, 0xffffffff],          bits : 31_0
*/
#define IME_DRAM_END_OUTPUT_STATUS_REGISTER1_OFS 0x005c
REGDEF_BEGIN(IME_DRAM_END_OUTPUT_STATUS_REGISTER1)
REGDEF_BIT(ime_dend_status,        32)
REGDEF_END(IME_DRAM_END_OUTPUT_STATUS_REGISTER1)


/*
    ime_out_type_p1                 :    [0x0, 0x1],            bits : 1
    ime_scl_method_p1                 :    [0x0, 0x3],          bits : 3_2
    ime_out_en_p1                     :    [0x0, 0x1],          bits : 4
    ime_sprt_out_en_p1               :    [0x0, 0x1],           bits : 5
    ime_omat1                         :    [0x0, 0x3],          bits : 9_8
    ime_scl_enh_fact_p1             :    [0x0, 0xff],           bits : 19_12
    ime_scl_enh_bit_p1               :    [0x0, 0xf],           bits : 23_20
*/
#define IME_OUTPUT_PATH1_CONTROL_REGISTER0_OFS 0x0060
REGDEF_BEGIN(IME_OUTPUT_PATH1_CONTROL_REGISTER0)
REGDEF_BIT(,        1)
REGDEF_BIT(ime_out_type_p1,        1)
REGDEF_BIT(ime_scl_method_p1,        2)
REGDEF_BIT(ime_out_en_p1,        1)
REGDEF_BIT(ime_sprt_out_en_p1,        1)
REGDEF_BIT(,        2)
REGDEF_BIT(ime_omat1,        2)
REGDEF_BIT(,        2)
REGDEF_BIT(ime_scl_enh_fact_p1,        8)
REGDEF_BIT(ime_scl_enh_bit_p1,        4)
REGDEF_END(IME_OUTPUT_PATH1_CONTROL_REGISTER0)


/*
    h1_ud                     :    [0x0, 0x1],          bits : 0
    v1_ud                     :    [0x0, 0x1],          bits : 1
    h1_dnrate                 :    [0x0, 0x1f],         bits : 6_2
    v1_dnrate                 :    [0x0, 0x1f],         bits : 11_7
    h1_filtmode             :    [0x0, 0x1],            bits : 15
    h1_filtcoef             :    [0x0, 0x3f],           bits : 21_16
    v1_filtmode             :    [0x0, 0x1],            bits : 22
    v1_filtcoef             :    [0x0, 0x3f],           bits : 28_23
*/
#define IME_OUTPUT_PATH1_CONTROL_REGISTER1_OFS 0x0064
REGDEF_BEGIN(IME_OUTPUT_PATH1_CONTROL_REGISTER1)
REGDEF_BIT(h1_ud,        1)
REGDEF_BIT(v1_ud,        1)
REGDEF_BIT(h1_dnrate,        5)
REGDEF_BIT(v1_dnrate,        5)
REGDEF_BIT(,        3)
REGDEF_BIT(h1_filtmode,        1)
REGDEF_BIT(h1_filtcoef,        6)
REGDEF_BIT(v1_filtmode,        1)
REGDEF_BIT(v1_filtcoef,        6)
REGDEF_END(IME_OUTPUT_PATH1_CONTROL_REGISTER1)


/*
    h1_sfact                :    [0x0, 0xffff],         bits : 15_0
    v1_sfact                :    [0x0, 0xffff],         bits : 31_16
*/
#define IME_OUTPUT_PATH1_CONTROL_REGISTER2_OFS 0x0068
REGDEF_BEGIN(IME_OUTPUT_PATH1_CONTROL_REGISTER2)
REGDEF_BIT(h1_sfact,        16)
REGDEF_BIT(v1_sfact,        16)
REGDEF_END(IME_OUTPUT_PATH1_CONTROL_REGISTER2)


/*
    reserved:    [0x0, 0xffffffff],         bits : 31_0
*/
#define IME_RESERVED_6_OFS 0x006c
REGDEF_BEGIN(IME_RESERVED_6)
REGDEF_BIT(reserved,        32)
REGDEF_END(IME_RESERVED_6)


/*
    reserved:    [0x0, 0xffffffff],         bits : 31_0
*/
#define IME_RESERVED_7_OFS 0x0070
REGDEF_BEGIN(IME_RESERVED_7)
REGDEF_BIT(reserved,        32)
REGDEF_END(IME_RESERVED_7)


/*
    reserved:    [0x0, 0xffffffff],         bits : 31_0
*/
#define IME_RESERVED_8_OFS 0x0074
REGDEF_BEGIN(IME_RESERVED_8)
REGDEF_BIT(reserved,        32)
REGDEF_END(IME_RESERVED_8)


/*
    reserved:    [0x0, 0xffffffff],         bits : 31_0
*/
#define IME_RESERVED_9_OFS 0x0078
REGDEF_BEGIN(IME_RESERVED_9)
REGDEF_BIT(reserved,        32)
REGDEF_END(IME_RESERVED_9)


/*
    h1_scl_size             :    [0x0, 0xffff],         bits : 15_0
    v1_scl_size             :    [0x0, 0xffff],         bits : 31_16
*/
#define IME_OUTPUT_PATH1_CONTROL_REGISTER7_OFS 0x007c
REGDEF_BEGIN(IME_OUTPUT_PATH1_CONTROL_REGISTER7)
REGDEF_BIT(h1_scl_size,        16)
REGDEF_BIT(v1_scl_size,        16)
REGDEF_END(IME_OUTPUT_PATH1_CONTROL_REGISTER7)


/*
    ime_cropout_x_p1                :    [0x0, 0xffff],         bits : 15_0
    ime_cropout_y_p1                :    [0x0, 0xffff],         bits : 31_16
*/
#define IME_OUTPUT_PATH1_CONTROL_REGISTER8_OFS 0x0080
REGDEF_BEGIN(IME_OUTPUT_PATH1_CONTROL_REGISTER8)
REGDEF_BIT(ime_cropout_x_p1,        16)
REGDEF_BIT(ime_cropout_y_p1,        16)
REGDEF_END(IME_OUTPUT_PATH1_CONTROL_REGISTER8)


/*
    h1_osize                :    [0x0, 0xffff],         bits : 15_0
    v1_osize                :    [0x0, 0xffff],         bits : 31_16
*/
#define IME_OUTPUT_PATH1_CONTROL_REGISTER9_OFS 0x0084
REGDEF_BEGIN(IME_OUTPUT_PATH1_CONTROL_REGISTER9)
REGDEF_BIT(h1_osize,        16)
REGDEF_BIT(v1_osize,        16)
REGDEF_END(IME_OUTPUT_PATH1_CONTROL_REGISTER9)


/*
    ime_y_clamp_min_p1 :    [0x0, 0xff],            bits : 7_0
    ime_y_clamp_max_p1 :    [0x0, 0xff],            bits : 15_8
    ime_uv_clamp_min_p1:    [0x0, 0xff],            bits : 23_16
    ime_uv_clamp_max_p1:    [0x0, 0xff],            bits : 31_24
*/
#define IME_OUTPUT_PATH1_CONTROL_REGISTER10_OFS 0x0088
REGDEF_BEGIN(IME_OUTPUT_PATH1_CONTROL_REGISTER10)
REGDEF_BIT(ime_y_clamp_min_p1,        8)
REGDEF_BIT(ime_y_clamp_max_p1,        8)
REGDEF_BIT(ime_uv_clamp_min_p1,        8)
REGDEF_BIT(ime_uv_clamp_max_p1,        8)
REGDEF_END(IME_OUTPUT_PATH1_CONTROL_REGISTER10)


/*
    ime_p1_y_dram_ofso:    [0x0, 0x3ffff],          bits : 19_2
*/
#define IME_OUTPUT_PATH1_DMA_LINEOFFSET_REGISTER0_OFS 0x008c
REGDEF_BEGIN(IME_OUTPUT_PATH1_DMA_LINEOFFSET_REGISTER0)
REGDEF_BIT(,        2)
REGDEF_BIT(ime_p1_y_dram_ofso,        18)
REGDEF_END(IME_OUTPUT_PATH1_DMA_LINEOFFSET_REGISTER0)


/*
    ime_p1_uv_dram_ofso:    [0x0, 0x3ffff],         bits : 19_2
*/
#define IME_OUTPUT_PATH1_DMA_LINEOFFSET_REGISTER1_OFS 0x0090
REGDEF_BEGIN(IME_OUTPUT_PATH1_DMA_LINEOFFSET_REGISTER1)
REGDEF_BIT(,        2)
REGDEF_BIT(ime_p1_uv_dram_ofso,        18)
REGDEF_END(IME_OUTPUT_PATH1_DMA_LINEOFFSET_REGISTER1)


/*
    ime_p1_y_dram_sao:    [0x0, 0xffffffff],            bits : 31_0
*/
#define IME_OUTPUT_PATH1_DMA_BUFFER0_REGISTER0_OFS 0x0094
REGDEF_BEGIN(IME_OUTPUT_PATH1_DMA_BUFFER0_REGISTER0)
REGDEF_BIT(ime_p1_y_dram_sao,        32)
REGDEF_END(IME_OUTPUT_PATH1_DMA_BUFFER0_REGISTER0)


/*
    ime_p1_u_dram_sao:    [0x0, 0xffffffff],            bits : 31_0
*/
#define IME_OUTPUT_PATH1_DMA_BUFFER0_REGISTER1_OFS 0x0098
REGDEF_BEGIN(IME_OUTPUT_PATH1_DMA_BUFFER0_REGISTER1)
REGDEF_BIT(ime_p1_u_dram_sao,        32)
REGDEF_END(IME_OUTPUT_PATH1_DMA_BUFFER0_REGISTER1)


/*
    ime_p1_v_dram_sao:    [0x0, 0xffffffff],            bits : 31_0
*/
#define IME_OUTPUT_PATH1_DMA_BUFFER0_REGISTER2_OFS 0x009c
REGDEF_BEGIN(IME_OUTPUT_PATH1_DMA_BUFFER0_REGISTER2)
REGDEF_BIT(ime_p1_v_dram_sao,        32)
REGDEF_END(IME_OUTPUT_PATH1_DMA_BUFFER0_REGISTER2)


/*
    reserved                :    [0x0, 0xffffffff],         bits : 31_0
*/
#define IME_RESERVED_10_OFS 0x00a0
REGDEF_BEGIN(IME_RESERVED_10)
REGDEF_BIT(reserved,        32)
REGDEF_END(IME_RESERVED_10)


/*
    reserved                :    [0x0, 0xffffffff],         bits : 31_0
*/
#define IME_RESERVED_11_OFS 0x00a4
REGDEF_BEGIN(IME_RESERVED_11)
REGDEF_BIT(reserved,        32)
REGDEF_END(IME_RESERVED_11)


/*
    reserved                :    [0x0, 0xffffffff],         bits : 31_0
*/
#define IME_RESERVED_12_OFS 0x00a8
REGDEF_BEGIN(IME_RESERVED_12)
REGDEF_BIT(reserved,        32)
REGDEF_END(IME_RESERVED_12)


/*
    reserved                :    [0x0, 0xffffffff],         bits : 31_0
*/
#define IME_RESERVED_13_OFS 0x00ac
REGDEF_BEGIN(IME_RESERVED_13)
REGDEF_BIT(reserved,        32)
REGDEF_END(IME_RESERVED_13)


/*
    ime_out_type_p2                 :    [0x0, 0x1],            bits : 1
    ime_scl_method_p2                 :    [0x0, 0x3],          bits : 3_2
    ime_out_en_p2                     :    [0x0, 0x1],          bits : 4
    ime_sprt_out_en_p2               :    [0x0, 0x1],           bits : 5
    ime_omat2                         :    [0x0, 0x7],          bits : 10_8
    ime_scl_enh_fact_p2             :    [0x0, 0xff],           bits : 19_12
    ime_scl_enh_bit_p2               :    [0x0, 0xf],           bits : 23_20
*/
#define IME_OUTPUT_PATH2_CONTROL_REGISTER0_OFS 0x00b0
REGDEF_BEGIN(IME_OUTPUT_PATH2_CONTROL_REGISTER0)
REGDEF_BIT(,        1)
REGDEF_BIT(ime_out_type_p2,        1)
REGDEF_BIT(ime_scl_method_p2,        2)
REGDEF_BIT(ime_out_en_p2,        1)
REGDEF_BIT(ime_sprt_out_en_p2,        1)
REGDEF_BIT(,        2)
REGDEF_BIT(ime_omat2,        3)
REGDEF_BIT(,        1)
REGDEF_BIT(ime_scl_enh_fact_p2,        8)
REGDEF_BIT(ime_scl_enh_bit_p2,        4)
REGDEF_END(IME_OUTPUT_PATH2_CONTROL_REGISTER0)


/*
    h2_ud                     :    [0x0, 0x1],          bits : 0
    v2_ud                     :    [0x0, 0x1],          bits : 1
    h2_dnrate                 :    [0x0, 0x1f],         bits : 6_2
    v2_dnrate                 :    [0x0, 0x1f],         bits : 11_7
    h2_filtmode             :    [0x0, 0x1],            bits : 15
    h2_filtcoef             :    [0x0, 0x3f],           bits : 21_16
    v2_filtmode             :    [0x0, 0x1],            bits : 22
    v2_filtcoef             :    [0x0, 0x3f],           bits : 28_23
*/
#define IME_OUTPUT_PATH2_CONTROL_REGISTER1_OFS 0x00b4
REGDEF_BEGIN(IME_OUTPUT_PATH2_CONTROL_REGISTER1)
REGDEF_BIT(h2_ud,        1)
REGDEF_BIT(v2_ud,        1)
REGDEF_BIT(h2_dnrate,        5)
REGDEF_BIT(v2_dnrate,        5)
REGDEF_BIT(,        3)
REGDEF_BIT(h2_filtmode,        1)
REGDEF_BIT(h2_filtcoef,        6)
REGDEF_BIT(v2_filtmode,        1)
REGDEF_BIT(v2_filtcoef,        6)
REGDEF_END(IME_OUTPUT_PATH2_CONTROL_REGISTER1)


/*
    h2_sfact                :    [0x0, 0xffff],         bits : 15_0
    v2_sfact                :    [0x0, 0xffff],         bits : 31_16
*/
#define IME_OUTPUT_PATH2_CONTROL_REGISTER2_OFS 0x00b8
REGDEF_BEGIN(IME_OUTPUT_PATH2_CONTROL_REGISTER2)
REGDEF_BIT(h2_sfact,        16)
REGDEF_BIT(v2_sfact,        16)
REGDEF_END(IME_OUTPUT_PATH2_CONTROL_REGISTER2)


/*
    isd_h_base_p2               :    [0x0, 0x1fff],         bits : 12_0
    isd_v_base_p2               :    [0x0, 0x1fff],         bits : 28_16
*/
#define IME_OUTPUT_PATH2_CONTROL_REGISTER3_OFS 0x00bc
REGDEF_BEGIN(IME_OUTPUT_PATH2_CONTROL_REGISTER3)
REGDEF_BIT(isd_h_base_p2,        13)
REGDEF_BIT(,        3)
REGDEF_BIT(isd_v_base_p2,        13)
REGDEF_END(IME_OUTPUT_PATH2_CONTROL_REGISTER3)


/*
    isd_h_sfact0_p2             :    [0x0, 0x1fff],         bits : 12_0
    isd_v_sfact0_p2             :    [0x0, 0x1fff],         bits : 28_16
*/
#define IME_OUTPUT_PATH2_CONTROL_REGISTER4_OFS 0x00c0
REGDEF_BEGIN(IME_OUTPUT_PATH2_CONTROL_REGISTER4)
REGDEF_BIT(isd_h_sfact0_p2,        13)
REGDEF_BIT(,        3)
REGDEF_BIT(isd_v_sfact0_p2,        13)
REGDEF_END(IME_OUTPUT_PATH2_CONTROL_REGISTER4)


/*
    isd_h_sfact1_p2             :    [0x0, 0x1fff],         bits : 12_0
    isd_v_sfact1_p2             :    [0x0, 0x1fff],         bits : 28_16
*/
#define IME_OUTPUT_PATH2_CONTROL_REGISTER5_OFS 0x00c4
REGDEF_BEGIN(IME_OUTPUT_PATH2_CONTROL_REGISTER5)
REGDEF_BIT(isd_h_sfact1_p2,        13)
REGDEF_BIT(,        3)
REGDEF_BIT(isd_v_sfact1_p2,        13)
REGDEF_END(IME_OUTPUT_PATH2_CONTROL_REGISTER5)


/*
    isd_h_sfact2_p2             :    [0x0, 0x1fff],         bits : 12_0
    isd_v_sfact2_p2             :    [0x0, 0x1fff],         bits : 28_16
*/
#define IME_OUTPUT_PATH2_CONTROL_REGISTER6_OFS 0x00c8
REGDEF_BEGIN(IME_OUTPUT_PATH2_CONTROL_REGISTER6)
REGDEF_BIT(isd_h_sfact2_p2,        13)
REGDEF_BIT(,        3)
REGDEF_BIT(isd_v_sfact2_p2,        13)
REGDEF_END(IME_OUTPUT_PATH2_CONTROL_REGISTER6)


/*
    isd_p2_mode      :    [0x0, 0x1],           bits : 0
    isd_p2_h_coef_num:    [0x0, 0x1f],          bits : 12_8
    isd_p2_v_coef_num:    [0x0, 0x1f],          bits : 20_16
*/
#define IME_OUTPUT_PATH2_CONTROL_REGISTER7_OFS 0x00cc
REGDEF_BEGIN(IME_OUTPUT_PATH2_CONTROL_REGISTER7)
REGDEF_BIT(isd_p2_mode,        1)
REGDEF_BIT(,        7)
REGDEF_BIT(isd_p2_h_coef_num,        5)
REGDEF_BIT(,        3)
REGDEF_BIT(isd_p2_v_coef_num,        5)
REGDEF_END(IME_OUTPUT_PATH2_CONTROL_REGISTER7)


/*
    isd_p2_h_coef0:    [0x0, 0xff],         bits : 7_0
    isd_p2_h_coef1:    [0x0, 0xff],         bits : 15_8
    isd_p2_h_coef2:    [0x0, 0xff],         bits : 23_16
    isd_p2_h_coef3:    [0x0, 0xff],         bits : 31_24
*/
#define IME_OUTPUT_PATH2_CONTROL_REGISTER8_OFS 0x00d0
REGDEF_BEGIN(IME_OUTPUT_PATH2_CONTROL_REGISTER8)
REGDEF_BIT(isd_p2_h_coef0,        8)
REGDEF_BIT(isd_p2_h_coef1,        8)
REGDEF_BIT(isd_p2_h_coef2,        8)
REGDEF_BIT(isd_p2_h_coef3,        8)
REGDEF_END(IME_OUTPUT_PATH2_CONTROL_REGISTER8)


/*
    isd_p2_h_coef4:    [0x0, 0xff],         bits : 7_0
    isd_p2_h_coef5:    [0x0, 0xff],         bits : 15_8
    isd_p2_h_coef6:    [0x0, 0xff],         bits : 23_16
    isd_p2_h_coef7:    [0x0, 0xff],         bits : 31_24
*/
#define IME_OUTPUT_PATH2_CONTROL_REGISTER9_OFS 0x00d4
REGDEF_BEGIN(IME_OUTPUT_PATH2_CONTROL_REGISTER9)
REGDEF_BIT(isd_p2_h_coef4,        8)
REGDEF_BIT(isd_p2_h_coef5,        8)
REGDEF_BIT(isd_p2_h_coef6,        8)
REGDEF_BIT(isd_p2_h_coef7,        8)
REGDEF_END(IME_OUTPUT_PATH2_CONTROL_REGISTER9)


/*
    isd_p2_h_coef8 :    [0x0, 0xff],            bits : 7_0
    isd_p2_h_coef9 :    [0x0, 0xff],            bits : 15_8
    isd_p2_h_coef10:    [0x0, 0xff],            bits : 23_16
    isd_p2_h_coef11:    [0x0, 0xff],            bits : 31_24
*/
#define IME_OUTPUT_PATH2_CONTROL_REGISTER10_OFS 0x00d8
REGDEF_BEGIN(IME_OUTPUT_PATH2_CONTROL_REGISTER10)
REGDEF_BIT(isd_p2_h_coef8,        8)
REGDEF_BIT(isd_p2_h_coef9,        8)
REGDEF_BIT(isd_p2_h_coef10,        8)
REGDEF_BIT(isd_p2_h_coef11,        8)
REGDEF_END(IME_OUTPUT_PATH2_CONTROL_REGISTER10)


/*
    isd_p2_h_coef12:    [0x0, 0xff],            bits : 7_0
    isd_p2_h_coef13:    [0x0, 0xff],            bits : 15_8
    isd_p2_h_coef14:    [0x0, 0xff],            bits : 23_16
    isd_p2_h_coef15:    [0x0, 0xff],            bits : 31_24
*/
#define IME_OUTPUT_PATH2_CONTROL_REGISTER11_OFS 0x00dc
REGDEF_BEGIN(IME_OUTPUT_PATH2_CONTROL_REGISTER11)
REGDEF_BIT(isd_p2_h_coef12,        8)
REGDEF_BIT(isd_p2_h_coef13,        8)
REGDEF_BIT(isd_p2_h_coef14,        8)
REGDEF_BIT(isd_p2_h_coef15,        8)
REGDEF_END(IME_OUTPUT_PATH2_CONTROL_REGISTER11)


/*
    isd_p2_v_coef0:    [0x0, 0xff],         bits : 7_0
    isd_p2_v_coef1:    [0x0, 0xff],         bits : 15_8
    isd_p2_v_coef2:    [0x0, 0xff],         bits : 23_16
    isd_p2_v_coef3:    [0x0, 0xff],         bits : 31_24
*/
#define IME_OUTPUT_PATH2_CONTROL_REGISTER13_OFS 0x00e0
REGDEF_BEGIN(IME_OUTPUT_PATH2_CONTROL_REGISTER13)
REGDEF_BIT(isd_p2_v_coef0,        8)
REGDEF_BIT(isd_p2_v_coef1,        8)
REGDEF_BIT(isd_p2_v_coef2,        8)
REGDEF_BIT(isd_p2_v_coef3,        8)
REGDEF_END(IME_OUTPUT_PATH2_CONTROL_REGISTER13)


/*
    isd_p2_v_coef4:    [0x0, 0xff],         bits : 7_0
    isd_p2_v_coef5:    [0x0, 0xff],         bits : 15_8
    isd_p2_v_coef6:    [0x0, 0xff],         bits : 23_16
    isd_p2_v_coef7:    [0x0, 0xff],         bits : 31_24
*/
#define IME_OUTPUT_PATH2_CONTROL_REGISTER14_OFS 0x00e4
REGDEF_BEGIN(IME_OUTPUT_PATH2_CONTROL_REGISTER14)
REGDEF_BIT(isd_p2_v_coef4,        8)
REGDEF_BIT(isd_p2_v_coef5,        8)
REGDEF_BIT(isd_p2_v_coef6,        8)
REGDEF_BIT(isd_p2_v_coef7,        8)
REGDEF_END(IME_OUTPUT_PATH2_CONTROL_REGISTER14)


/*
    isd_p2_v_coef8 :    [0x0, 0xff],            bits : 7_0
    isd_p2_v_coef9 :    [0x0, 0xff],            bits : 15_8
    isd_p2_v_coef10:    [0x0, 0xff],            bits : 23_16
    isd_p2_v_coef11:    [0x0, 0xff],            bits : 31_24
*/
#define IME_OUTPUT_PATH2_CONTROL_REGISTER15_OFS 0x00e8
REGDEF_BEGIN(IME_OUTPUT_PATH2_CONTROL_REGISTER15)
REGDEF_BIT(isd_p2_v_coef8,        8)
REGDEF_BIT(isd_p2_v_coef9,        8)
REGDEF_BIT(isd_p2_v_coef10,        8)
REGDEF_BIT(isd_p2_v_coef11,        8)
REGDEF_END(IME_OUTPUT_PATH2_CONTROL_REGISTER15)


/*
    isd_p2_v_coef12:    [0x0, 0xff],            bits : 7_0
    isd_p2_v_coef13:    [0x0, 0xff],            bits : 15_8
    isd_p2_v_coef14:    [0x0, 0xff],            bits : 23_16
    isd_p2_v_coef15:    [0x0, 0xff],            bits : 31_24
*/
#define IME_OUTPUT_PATH2_CONTROL_REGISTER16_OFS 0x00ec
REGDEF_BEGIN(IME_OUTPUT_PATH2_CONTROL_REGISTER16)
REGDEF_BIT(isd_p2_v_coef12,        8)
REGDEF_BIT(isd_p2_v_coef13,        8)
REGDEF_BIT(isd_p2_v_coef14,        8)
REGDEF_BIT(isd_p2_v_coef15,        8)
REGDEF_END(IME_OUTPUT_PATH2_CONTROL_REGISTER16)


/*
    isd_p2_h_coef16:    [0x0, 0xfff],           bits : 11_0
    isd_p2_v_coef16:    [0x0, 0xfff],           bits : 27_16
*/
#define IME_OUTPUT_PATH2_CONTROL_REGISTER17_OFS 0x00f0
REGDEF_BEGIN(IME_OUTPUT_PATH2_CONTROL_REGISTER17)
REGDEF_BIT(isd_p2_h_coef16,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(isd_p2_v_coef16,        12)
REGDEF_END(IME_OUTPUT_PATH2_CONTROL_REGISTER17)


/*
    isd_p2_h_coef_msb0:    [0x0, 0xf],          bits : 3_0
    isd_p2_h_coef_msb1:    [0x0, 0xf],          bits : 7_4
    isd_p2_h_coef_msb2:    [0x0, 0xf],          bits : 11_8
    isd_p2_h_coef_msb3:    [0x0, 0xf],          bits : 15_12
    isd_p2_h_coef_msb4:    [0x0, 0xf],          bits : 19_16
    isd_p2_h_coef_msb5:    [0x0, 0xf],          bits : 23_20
    isd_p2_h_coef_msb6:    [0x0, 0xf],          bits : 27_24
    isd_p2_h_coef_msb7:    [0x0, 0xf],          bits : 31_28
*/
#define IME_OUTPUT_PATH2_CONTROL_REGISTER18_OFS 0x00f4
REGDEF_BEGIN(IME_OUTPUT_PATH2_CONTROL_REGISTER18)
REGDEF_BIT(isd_p2_h_coef_msb0,        4)
REGDEF_BIT(isd_p2_h_coef_msb1,        4)
REGDEF_BIT(isd_p2_h_coef_msb2,        4)
REGDEF_BIT(isd_p2_h_coef_msb3,        4)
REGDEF_BIT(isd_p2_h_coef_msb4,        4)
REGDEF_BIT(isd_p2_h_coef_msb5,        4)
REGDEF_BIT(isd_p2_h_coef_msb6,        4)
REGDEF_BIT(isd_p2_h_coef_msb7,        4)
REGDEF_END(IME_OUTPUT_PATH2_CONTROL_REGISTER18)


/*
    isd_p2_h_coef_msb8 :    [0x0, 0xf],         bits : 3_0
    isd_p2_h_coef_msb9 :    [0x0, 0xf],         bits : 7_4
    isd_p2_h_coef_msb10:    [0x0, 0xf],         bits : 11_8
    isd_p2_h_coef_msb11:    [0x0, 0xf],         bits : 15_12
    isd_p2_h_coef_msb12:    [0x0, 0xf],         bits : 19_16
    isd_p2_h_coef_msb13:    [0x0, 0xf],         bits : 23_20
    isd_p2_h_coef_msb14:    [0x0, 0xf],         bits : 27_24
    isd_p2_h_coef_msb15:    [0x0, 0xf],         bits : 31_28
*/
#define IME_OUTPUT_PATH2_CONTROL_REGISTER19_OFS 0x00f8
REGDEF_BEGIN(IME_OUTPUT_PATH2_CONTROL_REGISTER19)
REGDEF_BIT(isd_p2_h_coef_msb8,        4)
REGDEF_BIT(isd_p2_h_coef_msb9,        4)
REGDEF_BIT(isd_p2_h_coef_msb10,        4)
REGDEF_BIT(isd_p2_h_coef_msb11,        4)
REGDEF_BIT(isd_p2_h_coef_msb12,        4)
REGDEF_BIT(isd_p2_h_coef_msb13,        4)
REGDEF_BIT(isd_p2_h_coef_msb14,        4)
REGDEF_BIT(isd_p2_h_coef_msb15,        4)
REGDEF_END(IME_OUTPUT_PATH2_CONTROL_REGISTER19)


/*
    isd_p2_v_coef_msb0:    [0x0, 0xf],          bits : 3_0
    isd_p2_v_coef_msb1:    [0x0, 0xf],          bits : 7_4
    isd_p2_v_coef_msb2:    [0x0, 0xf],          bits : 11_8
    isd_p2_v_coef_msb3:    [0x0, 0xf],          bits : 15_12
    isd_p2_v_coef_msb4:    [0x0, 0xf],          bits : 19_16
    isd_p2_v_coef_msb5:    [0x0, 0xf],          bits : 23_20
    isd_p2_v_coef_msb6:    [0x0, 0xf],          bits : 27_24
    isd_p2_v_coef_msb7:    [0x0, 0xf],          bits : 31_28
*/
#define IME_OUTPUT_PATH2_CONTROL_REGISTER20_OFS 0x00fc
REGDEF_BEGIN(IME_OUTPUT_PATH2_CONTROL_REGISTER20)
REGDEF_BIT(isd_p2_v_coef_msb0,        4)
REGDEF_BIT(isd_p2_v_coef_msb1,        4)
REGDEF_BIT(isd_p2_v_coef_msb2,        4)
REGDEF_BIT(isd_p2_v_coef_msb3,        4)
REGDEF_BIT(isd_p2_v_coef_msb4,        4)
REGDEF_BIT(isd_p2_v_coef_msb5,        4)
REGDEF_BIT(isd_p2_v_coef_msb6,        4)
REGDEF_BIT(isd_p2_v_coef_msb7,        4)
REGDEF_END(IME_OUTPUT_PATH2_CONTROL_REGISTER20)


/*
    isd_p2_v_coef_msb8 :    [0x0, 0xf],         bits : 3_0
    isd_p2_v_coef_msb9 :    [0x0, 0xf],         bits : 7_4
    isd_p2_v_coef_msb10:    [0x0, 0xf],         bits : 11_8
    isd_p2_v_coef_msb11:    [0x0, 0xf],         bits : 15_12
    isd_p2_v_coef_msb12:    [0x0, 0xf],         bits : 19_16
    isd_p2_v_coef_msb13:    [0x0, 0xf],         bits : 23_20
    isd_p2_v_coef_msb14:    [0x0, 0xf],         bits : 27_24
    isd_p2_v_coef_msb15:    [0x0, 0xf],         bits : 31_28
*/
#define IME_OUTPUT_PATH2_CONTROL_REGISTER21_OFS 0x0100
REGDEF_BEGIN(IME_OUTPUT_PATH2_CONTROL_REGISTER21)
REGDEF_BIT(isd_p2_v_coef_msb8,        4)
REGDEF_BIT(isd_p2_v_coef_msb9,        4)
REGDEF_BIT(isd_p2_v_coef_msb10,        4)
REGDEF_BIT(isd_p2_v_coef_msb11,        4)
REGDEF_BIT(isd_p2_v_coef_msb12,        4)
REGDEF_BIT(isd_p2_v_coef_msb13,        4)
REGDEF_BIT(isd_p2_v_coef_msb14,        4)
REGDEF_BIT(isd_p2_v_coef_msb15,        4)
REGDEF_END(IME_OUTPUT_PATH2_CONTROL_REGISTER21)


/*
    h2_scl_size             :    [0x0, 0xffff],         bits : 15_0
    v2_scl_size             :    [0x0, 0xffff],         bits : 31_16
*/
#define IME_OUTPUT_PATH2_CONTROL_REGISTER22_OFS 0x0104
REGDEF_BEGIN(IME_OUTPUT_PATH2_CONTROL_REGISTER22)
REGDEF_BIT(h2_scl_size,        16)
REGDEF_BIT(v2_scl_size,        16)
REGDEF_END(IME_OUTPUT_PATH2_CONTROL_REGISTER22)


/*
    ime_cropout_x_p2                :    [0x0, 0xffff],         bits : 15_0
    ime_cropout_y_p2                :    [0x0, 0xffff],         bits : 31_16
*/
#define IME_OUTPUT_PATH2_CONTROL_REGISTER23_OFS 0x0108
REGDEF_BEGIN(IME_OUTPUT_PATH2_CONTROL_REGISTER23)
REGDEF_BIT(ime_cropout_x_p2,        16)
REGDEF_BIT(ime_cropout_y_p2,        16)
REGDEF_END(IME_OUTPUT_PATH2_CONTROL_REGISTER23)


/*
    h2_osize                :    [0x0, 0xffff],         bits : 15_0
    v2_osize                :    [0x0, 0xffff],         bits : 31_16
*/
#define IME_OUTPUT_PATH2_CONTROL_REGISTER24_OFS 0x010c
REGDEF_BEGIN(IME_OUTPUT_PATH2_CONTROL_REGISTER24)
REGDEF_BIT(h2_osize,        16)
REGDEF_BIT(v2_osize,        16)
REGDEF_END(IME_OUTPUT_PATH2_CONTROL_REGISTER24)


/*
    ime_y_clamp_min_p2 :    [0x0, 0xff],            bits : 7_0
    ime_y_clamp_max_p2 :    [0x0, 0xff],            bits : 15_8
    ime_uv_clamp_min_p2:    [0x0, 0xff],            bits : 23_16
    ime_uv_clamp_max_p2:    [0x0, 0xff],            bits : 31_24
*/
#define IME_OUTPUT_PATH2_CONTROL_REGISTER25_OFS 0x0110
REGDEF_BEGIN(IME_OUTPUT_PATH2_CONTROL_REGISTER25)
REGDEF_BIT(ime_y_clamp_min_p2,        8)
REGDEF_BIT(ime_y_clamp_max_p2,        8)
REGDEF_BIT(ime_uv_clamp_min_p2,        8)
REGDEF_BIT(ime_uv_clamp_max_p2,        8)
REGDEF_END(IME_OUTPUT_PATH2_CONTROL_REGISTER25)


/*
    ime_p2_y_dram_ofso:    [0x0, 0x3ffff],          bits : 19_2
*/
#define IME_OUTPUT_PATH2_DMA_LINEOFFSET_REGISTER0_OFS 0x0114
REGDEF_BEGIN(IME_OUTPUT_PATH2_DMA_LINEOFFSET_REGISTER0)
REGDEF_BIT(,        2)
REGDEF_BIT(ime_p2_y_dram_ofso,        18)
REGDEF_END(IME_OUTPUT_PATH2_DMA_LINEOFFSET_REGISTER0)


/*
    ime_p2_uv_dram_ofso:    [0x0, 0x3ffff],         bits : 19_2
*/
#define IME_OUTPUT_PATH2_DMA_LINEOFFSET_REGISTER1_OFS 0x0118
REGDEF_BEGIN(IME_OUTPUT_PATH2_DMA_LINEOFFSET_REGISTER1)
REGDEF_BIT(,        2)
REGDEF_BIT(ime_p2_uv_dram_ofso,        18)
REGDEF_END(IME_OUTPUT_PATH2_DMA_LINEOFFSET_REGISTER1)


/*
    ime_p2_y_dram_sao:    [0x0, 0xffffffff],            bits : 31_0
*/
#define IME_OUTPUT_PATH2_DMA_BUFFER0_REGISTER0_OFS 0x011c
REGDEF_BEGIN(IME_OUTPUT_PATH2_DMA_BUFFER0_REGISTER0)
REGDEF_BIT(ime_p2_y_dram_sao,        32)
REGDEF_END(IME_OUTPUT_PATH2_DMA_BUFFER0_REGISTER0)


/*
    ime_p2_uv_dram_sao:    [0x0, 0xffffffff],           bits : 31_0
*/
#define IME_OUTPUT_PATH2_DMA_BUFFER0_REGISTER1_OFS 0x0120
REGDEF_BEGIN(IME_OUTPUT_PATH2_DMA_BUFFER0_REGISTER1)
REGDEF_BIT(ime_p2_uv_dram_sao,        32)
REGDEF_END(IME_OUTPUT_PATH2_DMA_BUFFER0_REGISTER1)


/*
    reserved                :    [0x0, 0xffffffff],         bits : 31_0
*/
#define IME_RESERVED_14_OFS 0x0124
REGDEF_BEGIN(IME_RESERVED_14)
REGDEF_BIT(reserved,        32)
REGDEF_END(IME_RESERVED_14)


/*
    reserved                :    [0x0, 0xffffffff],         bits : 31_0
*/
#define IME_RESERVED_15_OFS 0x0128
REGDEF_BEGIN(IME_RESERVED_15)
REGDEF_BIT(reserved,        32)
REGDEF_END(IME_RESERVED_15)


/*
    reserved                :    [0x0, 0xffffffff],         bits : 31_0
*/
#define IME_RESERVED_16_OFS 0x012c
REGDEF_BEGIN(IME_RESERVED_16)
REGDEF_BIT(reserved,        32)
REGDEF_END(IME_RESERVED_16)


/*
    ime_out_type_p3                 :    [0x0, 0x1],            bits : 1
    ime_scl_method_p3      :    [0x0, 0x3],         bits : 3_2
    ime_out_en_p3                     :    [0x0, 0x1],          bits : 4
    ime_sprt_out_en_p3               :    [0x0, 0x1],           bits : 5
    ime_omat3                         :    [0x0, 0x7],          bits : 10_8
    ime_scl_enh_fact_p3             :    [0x0, 0xff],           bits : 19_12
    ime_scl_enh_bit_p3               :    [0x0, 0xf],           bits : 23_20
*/
#define IME_OUTPUT_PATH3_CONTROL_REGISTER0_OFS 0x0130
REGDEF_BEGIN(IME_OUTPUT_PATH3_CONTROL_REGISTER0)
REGDEF_BIT(,        1)
REGDEF_BIT(ime_out_type_p3,        1)
REGDEF_BIT(ime_scl_method_p3,        2)
REGDEF_BIT(ime_out_en_p3,        1)
REGDEF_BIT(ime_sprt_out_en_p3,        1)
REGDEF_BIT(,        2)
REGDEF_BIT(ime_omat3,        3)
REGDEF_BIT(,        1)
REGDEF_BIT(ime_scl_enh_fact_p3,        8)
REGDEF_BIT(ime_scl_enh_bit_p3,        4)
REGDEF_END(IME_OUTPUT_PATH3_CONTROL_REGISTER0)


/*
    h3_ud                     :    [0x0, 0x1],          bits : 0
    v3_ud                     :    [0x0, 0x1],          bits : 1
    h3_dnrate                 :    [0x0, 0x1f],         bits : 6_2
    v3_dnrate                 :    [0x0, 0x1f],         bits : 11_7
    h3_filtmode             :    [0x0, 0x1],            bits : 15
    h3_filtcoef             :    [0x0, 0x3f],           bits : 21_16
    v3_filtmode             :    [0x0, 0x1],            bits : 22
    v3_filtcoef             :    [0x0, 0x3f],           bits : 28_23
*/
#define IME_OUTPUT_PATH3_CONTROL_REGISTER1_OFS 0x0134
REGDEF_BEGIN(IME_OUTPUT_PATH3_CONTROL_REGISTER1)
REGDEF_BIT(h3_ud,        1)
REGDEF_BIT(v3_ud,        1)
REGDEF_BIT(h3_dnrate,        5)
REGDEF_BIT(v3_dnrate,        5)
REGDEF_BIT(,        3)
REGDEF_BIT(h3_filtmode,        1)
REGDEF_BIT(h3_filtcoef,        6)
REGDEF_BIT(v3_filtmode,        1)
REGDEF_BIT(v3_filtcoef,        6)
REGDEF_END(IME_OUTPUT_PATH3_CONTROL_REGISTER1)


/*
    h3_sfact                :    [0x0, 0xffff],         bits : 15_0
    v3_sfact                :    [0x0, 0xffff],         bits : 31_16
*/
#define IME_OUTPUT_PATH3_CONTROL_REGISTER2_OFS 0x0138
REGDEF_BEGIN(IME_OUTPUT_PATH3_CONTROL_REGISTER2)
REGDEF_BIT(h3_sfact,        16)
REGDEF_BIT(v3_sfact,        16)
REGDEF_END(IME_OUTPUT_PATH3_CONTROL_REGISTER2)


/*
    isd_h_base_p3               :    [0x0, 0x1fff],         bits : 12_0
    isd_v_base_p3               :    [0x0, 0x1fff],         bits : 28_16
*/
#define IME_OUTPUT_PATH3_CONTROL_REGISTER3_OFS 0x013c
REGDEF_BEGIN(IME_OUTPUT_PATH3_CONTROL_REGISTER3)
REGDEF_BIT(isd_h_base_p3,        13)
REGDEF_BIT(,        3)
REGDEF_BIT(isd_v_base_p3,        13)
REGDEF_END(IME_OUTPUT_PATH3_CONTROL_REGISTER3)


/*
    isd_h_sfact0_p3             :    [0x0, 0x1fff],         bits : 12_0
    isd_v_sfact0_p3             :    [0x0, 0x1fff],         bits : 28_16
*/
#define IME_OUTPUT_PATH3_CONTROL_REGISTER4_OFS 0x0140
REGDEF_BEGIN(IME_OUTPUT_PATH3_CONTROL_REGISTER4)
REGDEF_BIT(isd_h_sfact0_p3,        13)
REGDEF_BIT(,        3)
REGDEF_BIT(isd_v_sfact0_p3,        13)
REGDEF_END(IME_OUTPUT_PATH3_CONTROL_REGISTER4)


/*
    isd_h_sfact1_p3             :    [0x0, 0x1fff],         bits : 12_0
    isd_v_sfact1_p3             :    [0x0, 0x1fff],         bits : 28_16
*/
#define IME_OUTPUT_PATH3_CONTROL_REGISTER5_OFS 0x0144
REGDEF_BEGIN(IME_OUTPUT_PATH3_CONTROL_REGISTER5)
REGDEF_BIT(isd_h_sfact1_p3,        13)
REGDEF_BIT(,        3)
REGDEF_BIT(isd_v_sfact1_p3,        13)
REGDEF_END(IME_OUTPUT_PATH3_CONTROL_REGISTER5)


/*
    isd_h_sfact2_p3             :    [0x0, 0x1fff],         bits : 12_0
    isd_v_sfact2_p3             :    [0x0, 0x1fff],         bits : 28_16
*/
#define IME_OUTPUT_PATH3_CONTROL_REGISTER6_OFS 0x0148
REGDEF_BEGIN(IME_OUTPUT_PATH3_CONTROL_REGISTER6)
REGDEF_BIT(isd_h_sfact2_p3,        13)
REGDEF_BIT(,        3)
REGDEF_BIT(isd_v_sfact2_p3,        13)
REGDEF_END(IME_OUTPUT_PATH3_CONTROL_REGISTER6)


/*
    isd_p3_mode      :    [0x0, 0x1],           bits : 0
    isd_p3_h_coef_num:    [0x0, 0x1f],          bits : 12_8
    isd_p3_v_coef_num:    [0x0, 0x1f],          bits : 20_16
*/
#define IME_OUTPUT_PATH3_CONTROL_REGISTER7_OFS 0x014c
REGDEF_BEGIN(IME_OUTPUT_PATH3_CONTROL_REGISTER7)
REGDEF_BIT(isd_p3_mode,        1)
REGDEF_BIT(,        7)
REGDEF_BIT(isd_p3_h_coef_num,        5)
REGDEF_BIT(,        3)
REGDEF_BIT(isd_p3_v_coef_num,        5)
REGDEF_END(IME_OUTPUT_PATH3_CONTROL_REGISTER7)


/*
    isd_p3_h_coef0:    [0x0, 0xff],         bits : 7_0
    isd_p3_h_coef1:    [0x0, 0xff],         bits : 15_8
    isd_p3_h_coef2:    [0x0, 0xff],         bits : 23_16
    isd_p3_h_coef3:    [0x0, 0xff],         bits : 31_24
*/
#define IME_OUTPUT_PATH3_CONTROL_REGISTER8_OFS 0x0150
REGDEF_BEGIN(IME_OUTPUT_PATH3_CONTROL_REGISTER8)
REGDEF_BIT(isd_p3_h_coef0,        8)
REGDEF_BIT(isd_p3_h_coef1,        8)
REGDEF_BIT(isd_p3_h_coef2,        8)
REGDEF_BIT(isd_p3_h_coef3,        8)
REGDEF_END(IME_OUTPUT_PATH3_CONTROL_REGISTER8)


/*
    isd_p3_h_coef4:    [0x0, 0xff],         bits : 7_0
    isd_p3_h_coef5:    [0x0, 0xff],         bits : 15_8
    isd_p3_h_coef6:    [0x0, 0xff],         bits : 23_16
    isd_p3_h_coef7:    [0x0, 0xff],         bits : 31_24
*/
#define IME_OUTPUT_PATH3_CONTROL_REGISTER9_OFS 0x0154
REGDEF_BEGIN(IME_OUTPUT_PATH3_CONTROL_REGISTER9)
REGDEF_BIT(isd_p3_h_coef4,        8)
REGDEF_BIT(isd_p3_h_coef5,        8)
REGDEF_BIT(isd_p3_h_coef6,        8)
REGDEF_BIT(isd_p3_h_coef7,        8)
REGDEF_END(IME_OUTPUT_PATH3_CONTROL_REGISTER9)


/*
    isd_p3_h_coef8 :    [0x0, 0xff],            bits : 7_0
    isd_p3_h_coef9 :    [0x0, 0xff],            bits : 15_8
    isd_p3_h_coef10:    [0x0, 0xff],            bits : 23_16
    isd_p3_h_coef11:    [0x0, 0xff],            bits : 31_24
*/
#define IME_OUTPUT_PATH3_CONTROL_REGISTER10_OFS 0x0158
REGDEF_BEGIN(IME_OUTPUT_PATH3_CONTROL_REGISTER10)
REGDEF_BIT(isd_p3_h_coef8,        8)
REGDEF_BIT(isd_p3_h_coef9,        8)
REGDEF_BIT(isd_p3_h_coef10,        8)
REGDEF_BIT(isd_p3_h_coef11,        8)
REGDEF_END(IME_OUTPUT_PATH3_CONTROL_REGISTER10)


/*
    isd_p3_h_coef12:    [0x0, 0xff],            bits : 7_0
    isd_p3_h_coef13:    [0x0, 0xff],            bits : 15_8
    isd_p3_h_coef14:    [0x0, 0xff],            bits : 23_16
    isd_p3_h_coef15:    [0x0, 0xff],            bits : 31_24
*/
#define IME_OUTPUT_PATH3_CONTROL_REGISTER11_OFS 0x015c
REGDEF_BEGIN(IME_OUTPUT_PATH3_CONTROL_REGISTER11)
REGDEF_BIT(isd_p3_h_coef12,        8)
REGDEF_BIT(isd_p3_h_coef13,        8)
REGDEF_BIT(isd_p3_h_coef14,        8)
REGDEF_BIT(isd_p3_h_coef15,        8)
REGDEF_END(IME_OUTPUT_PATH3_CONTROL_REGISTER11)


/*
    isd_p3_v_coef0:    [0x0, 0xff],         bits : 7_0
    isd_p3_v_coef1:    [0x0, 0xff],         bits : 15_8
    isd_p3_v_coef2:    [0x0, 0xff],         bits : 23_16
    isd_p3_v_coef3:    [0x0, 0xff],         bits : 31_24
*/
#define IME_OUTPUT_PATH3_CONTROL_REGISTER12_OFS 0x0160
REGDEF_BEGIN(IME_OUTPUT_PATH3_CONTROL_REGISTER12)
REGDEF_BIT(isd_p3_v_coef0,        8)
REGDEF_BIT(isd_p3_v_coef1,        8)
REGDEF_BIT(isd_p3_v_coef2,        8)
REGDEF_BIT(isd_p3_v_coef3,        8)
REGDEF_END(IME_OUTPUT_PATH3_CONTROL_REGISTER12)


/*
    isd_p3_v_coef4:    [0x0, 0xff],         bits : 7_0
    isd_p3_v_coef5:    [0x0, 0xff],         bits : 15_8
    isd_p3_v_coef6:    [0x0, 0xff],         bits : 23_16
    isd_p3_v_coef7:    [0x0, 0xff],         bits : 31_24
*/
#define IME_OUTPUT_PATH3_CONTROL_REGISTER13_OFS 0x0164
REGDEF_BEGIN(IME_OUTPUT_PATH3_CONTROL_REGISTER13)
REGDEF_BIT(isd_p3_v_coef4,        8)
REGDEF_BIT(isd_p3_v_coef5,        8)
REGDEF_BIT(isd_p3_v_coef6,        8)
REGDEF_BIT(isd_p3_v_coef7,        8)
REGDEF_END(IME_OUTPUT_PATH3_CONTROL_REGISTER13)


/*
    isd_p3_v_coef8 :    [0x0, 0xff],            bits : 7_0
    isd_p3_v_coef9 :    [0x0, 0xff],            bits : 15_8
    isd_p3_v_coef10:    [0x0, 0xff],            bits : 23_16
    isd_p3_v_coef11:    [0x0, 0xff],            bits : 31_24
*/
#define IME_OUTPUT_PATH3_CONTROL_REGISTER14_OFS 0x0168
REGDEF_BEGIN(IME_OUTPUT_PATH3_CONTROL_REGISTER14)
REGDEF_BIT(isd_p3_v_coef8,        8)
REGDEF_BIT(isd_p3_v_coef9,        8)
REGDEF_BIT(isd_p3_v_coef10,        8)
REGDEF_BIT(isd_p3_v_coef11,        8)
REGDEF_END(IME_OUTPUT_PATH3_CONTROL_REGISTER14)


/*
    isd_p3_v_coef12:    [0x0, 0xff],            bits : 7_0
    isd_p3_v_coef13:    [0x0, 0xff],            bits : 15_8
    isd_p3_v_coef14:    [0x0, 0xff],            bits : 23_16
    isd_p3_v_coef15:    [0x0, 0xff],            bits : 31_24
*/
#define IME_OUTPUT_PATH3_CONTROL_REGISTER15_OFS 0x016c
REGDEF_BEGIN(IME_OUTPUT_PATH3_CONTROL_REGISTER15)
REGDEF_BIT(isd_p3_v_coef12,        8)
REGDEF_BIT(isd_p3_v_coef13,        8)
REGDEF_BIT(isd_p3_v_coef14,        8)
REGDEF_BIT(isd_p3_v_coef15,        8)
REGDEF_END(IME_OUTPUT_PATH3_CONTROL_REGISTER15)


/*
    isd_p3_h_coef16:    [0x0, 0xfff],           bits : 11_0
    isd_p3_v_coef16:    [0x0, 0xfff],           bits : 27_16
*/
#define IME_OUTPUT_PATH3_CONTROL_REGISTER16_OFS 0x0170
REGDEF_BEGIN(IME_OUTPUT_PATH3_CONTROL_REGISTER16)
REGDEF_BIT(isd_p3_h_coef16,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(isd_p3_v_coef16,        12)
REGDEF_END(IME_OUTPUT_PATH3_CONTROL_REGISTER16)


/*
    isd_p3_h_coef_msb0:    [0x0, 0xf],          bits : 3_0
    isd_p3_h_coef_msb1:    [0x0, 0xf],          bits : 7_4
    isd_p3_h_coef_msb2:    [0x0, 0xf],          bits : 11_8
    isd_p3_h_coef_msb3:    [0x0, 0xf],          bits : 15_12
    isd_p3_h_coef_msb4:    [0x0, 0xf],          bits : 19_16
    isd_p3_h_coef_msb5:    [0x0, 0xf],          bits : 23_20
    isd_p3_h_coef_msb6:    [0x0, 0xf],          bits : 27_24
    isd_p3_h_coef_msb7:    [0x0, 0xf],          bits : 31_28
*/
#define IME_OUTPUT_PATH3_CONTROL_REGISTER17_OFS 0x0174
REGDEF_BEGIN(IME_OUTPUT_PATH3_CONTROL_REGISTER17)
REGDEF_BIT(isd_p3_h_coef_msb0,        4)
REGDEF_BIT(isd_p3_h_coef_msb1,        4)
REGDEF_BIT(isd_p3_h_coef_msb2,        4)
REGDEF_BIT(isd_p3_h_coef_msb3,        4)
REGDEF_BIT(isd_p3_h_coef_msb4,        4)
REGDEF_BIT(isd_p3_h_coef_msb5,        4)
REGDEF_BIT(isd_p3_h_coef_msb6,        4)
REGDEF_BIT(isd_p3_h_coef_msb7,        4)
REGDEF_END(IME_OUTPUT_PATH3_CONTROL_REGISTER17)


/*
    isd_p3_h_coef_msb8 :    [0x0, 0xf],         bits : 3_0
    isd_p3_h_coef_msb9 :    [0x0, 0xf],         bits : 7_4
    isd_p3_h_coef_msb10:    [0x0, 0xf],         bits : 11_8
    isd_p3_h_coef_msb11:    [0x0, 0xf],         bits : 15_12
    isd_p3_h_coef_msb12:    [0x0, 0xf],         bits : 19_16
    isd_p3_h_coef_msb13:    [0x0, 0xf],         bits : 23_20
    isd_p3_h_coef_msb14:    [0x0, 0xf],         bits : 27_24
    isd_p3_h_coef_msb15:    [0x0, 0xf],         bits : 31_28
*/
#define IME_OUTPUT_PATH3_CONTROL_REGISTER18_OFS 0x0178
REGDEF_BEGIN(IME_OUTPUT_PATH3_CONTROL_REGISTER18)
REGDEF_BIT(isd_p3_h_coef_msb8,        4)
REGDEF_BIT(isd_p3_h_coef_msb9,        4)
REGDEF_BIT(isd_p3_h_coef_msb10,        4)
REGDEF_BIT(isd_p3_h_coef_msb11,        4)
REGDEF_BIT(isd_p3_h_coef_msb12,        4)
REGDEF_BIT(isd_p3_h_coef_msb13,        4)
REGDEF_BIT(isd_p3_h_coef_msb14,        4)
REGDEF_BIT(isd_p3_h_coef_msb15,        4)
REGDEF_END(IME_OUTPUT_PATH3_CONTROL_REGISTER18)


/*
    isd_p3_v_coef_msb0:    [0x0, 0xf],          bits : 3_0
    isd_p3_v_coef_msb1:    [0x0, 0xf],          bits : 7_4
    isd_p3_v_coef_msb2:    [0x0, 0xf],          bits : 11_8
    isd_p3_v_coef_msb3:    [0x0, 0xf],          bits : 15_12
    isd_p3_v_coef_msb4:    [0x0, 0xf],          bits : 19_16
    isd_p3_v_coef_msb5:    [0x0, 0xf],          bits : 23_20
    isd_p3_v_coef_msb6:    [0x0, 0xf],          bits : 27_24
    isd_p3_v_coef_msb7:    [0x0, 0xf],          bits : 31_28
*/
#define IME_OUTPUT_PATH3_CONTROL_REGISTER19_OFS 0x017c
REGDEF_BEGIN(IME_OUTPUT_PATH3_CONTROL_REGISTER19)
REGDEF_BIT(isd_p3_v_coef_msb0,        4)
REGDEF_BIT(isd_p3_v_coef_msb1,        4)
REGDEF_BIT(isd_p3_v_coef_msb2,        4)
REGDEF_BIT(isd_p3_v_coef_msb3,        4)
REGDEF_BIT(isd_p3_v_coef_msb4,        4)
REGDEF_BIT(isd_p3_v_coef_msb5,        4)
REGDEF_BIT(isd_p3_v_coef_msb6,        4)
REGDEF_BIT(isd_p3_v_coef_msb7,        4)
REGDEF_END(IME_OUTPUT_PATH3_CONTROL_REGISTER19)


/*
    isd_p3_v_coef_msb8 :    [0x0, 0xf],         bits : 3_0
    isd_p3_v_coef_msb9 :    [0x0, 0xf],         bits : 7_4
    isd_p3_v_coef_msb10:    [0x0, 0xf],         bits : 11_8
    isd_p3_v_coef_msb11:    [0x0, 0xf],         bits : 15_12
    isd_p3_v_coef_msb12:    [0x0, 0xf],         bits : 19_16
    isd_p3_v_coef_msb13:    [0x0, 0xf],         bits : 23_20
    isd_p3_v_coef_msb14:    [0x0, 0xf],         bits : 27_24
    isd_p3_v_coef_msb15:    [0x0, 0xf],         bits : 31_28
*/
#define IME_OUTPUT_PATH3_CONTROL_REGISTER20_OFS 0x0180
REGDEF_BEGIN(IME_OUTPUT_PATH3_CONTROL_REGISTER20)
REGDEF_BIT(isd_p3_v_coef_msb8,        4)
REGDEF_BIT(isd_p3_v_coef_msb9,        4)
REGDEF_BIT(isd_p3_v_coef_msb10,        4)
REGDEF_BIT(isd_p3_v_coef_msb11,        4)
REGDEF_BIT(isd_p3_v_coef_msb12,        4)
REGDEF_BIT(isd_p3_v_coef_msb13,        4)
REGDEF_BIT(isd_p3_v_coef_msb14,        4)
REGDEF_BIT(isd_p3_v_coef_msb15,        4)
REGDEF_END(IME_OUTPUT_PATH3_CONTROL_REGISTER20)


/*
    h3_scl_size             :    [0x0, 0xffff],         bits : 15_0
    v3_scl_size             :    [0x0, 0xffff],         bits : 31_16
*/
#define IME_OUTPUT_PATH3_CONTROL_REGISTER21_OFS 0x0184
REGDEF_BEGIN(IME_OUTPUT_PATH3_CONTROL_REGISTER21)
REGDEF_BIT(h3_scl_size,        16)
REGDEF_BIT(v3_scl_size,        16)
REGDEF_END(IME_OUTPUT_PATH3_CONTROL_REGISTER21)


/*
    ime_cropout_x_p3                :    [0x0, 0xffff],         bits : 15_0
    ime_cropout_y_p3                :    [0x0, 0xffff],         bits : 31_16
*/
#define IME_OUTPUT_PATH3_CONTROL_REGISTER22_OFS 0x0188
REGDEF_BEGIN(IME_OUTPUT_PATH3_CONTROL_REGISTER22)
REGDEF_BIT(ime_cropout_x_p3,        16)
REGDEF_BIT(ime_cropout_y_p3,        16)
REGDEF_END(IME_OUTPUT_PATH3_CONTROL_REGISTER22)


/*
    h3_osize                :    [0x0, 0xffff],         bits : 15_0
    v3_osize                :    [0x0, 0xffff],         bits : 31_16
*/
#define IME_OUTPUT_PATH3_CONTROL_REGISTER23_OFS 0x018c
REGDEF_BEGIN(IME_OUTPUT_PATH3_CONTROL_REGISTER23)
REGDEF_BIT(h3_osize,        16)
REGDEF_BIT(v3_osize,        16)
REGDEF_END(IME_OUTPUT_PATH3_CONTROL_REGISTER23)


/*
    ime_y_clamp_min_p3 :    [0x0, 0xff],            bits : 7_0
    ime_y_clamp_max_p3 :    [0x0, 0xff],            bits : 15_8
    ime_uv_clamp_min_p3:    [0x0, 0xff],            bits : 23_16
    ime_uv_clamp_max_p3:    [0x0, 0xff],            bits : 31_24
*/
#define IME_OUTPUT_PATH3_CONTROL_REGISTER24_OFS 0x0190
REGDEF_BEGIN(IME_OUTPUT_PATH3_CONTROL_REGISTER24)
REGDEF_BIT(ime_y_clamp_min_p3,        8)
REGDEF_BIT(ime_y_clamp_max_p3,        8)
REGDEF_BIT(ime_uv_clamp_min_p3,        8)
REGDEF_BIT(ime_uv_clamp_max_p3,        8)
REGDEF_END(IME_OUTPUT_PATH3_CONTROL_REGISTER24)


/*
    ime_p3_y_dram_ofso:    [0x0, 0x3ffff],          bits : 19_2
*/
#define IME_OUTPUT_PATH3_DMA_LINEOFFSET_REGISTER0_OFS 0x0194
REGDEF_BEGIN(IME_OUTPUT_PATH3_DMA_LINEOFFSET_REGISTER0)
REGDEF_BIT(,        2)
REGDEF_BIT(ime_p3_y_dram_ofso,        18)
REGDEF_END(IME_OUTPUT_PATH3_DMA_LINEOFFSET_REGISTER0)


/*
    ime_p3_uv_dram_ofso:    [0x0, 0x3ffff],         bits : 19_2
*/
#define IME_OUTPUT_PATH3_DMA_LINEOFFSET_REGISTER1_OFS 0x0198
REGDEF_BEGIN(IME_OUTPUT_PATH3_DMA_LINEOFFSET_REGISTER1)
REGDEF_BIT(,        2)
REGDEF_BIT(ime_p3_uv_dram_ofso,        18)
REGDEF_END(IME_OUTPUT_PATH3_DMA_LINEOFFSET_REGISTER1)


/*
    ime_p3_y_dram_sao:    [0x0, 0xffffffff],            bits : 31_0
*/
#define IME_OUTPUT_PATH3_DMA_BUFFER0_REGISTER0_OFS 0x019c
REGDEF_BEGIN(IME_OUTPUT_PATH3_DMA_BUFFER0_REGISTER0)
REGDEF_BIT(ime_p3_y_dram_sao,        32)
REGDEF_END(IME_OUTPUT_PATH3_DMA_BUFFER0_REGISTER0)


/*
    ime_p3_uv_dram_sao:    [0x0, 0xffffffff],           bits : 31_0
*/
#define IME_OUTPUT_PATH3_DMA_BUFFER0_REGISTER1_OFS 0x01a0
REGDEF_BEGIN(IME_OUTPUT_PATH3_DMA_BUFFER0_REGISTER1)
REGDEF_BIT(ime_p3_uv_dram_sao,        32)
REGDEF_END(IME_OUTPUT_PATH3_DMA_BUFFER0_REGISTER1)


/*
    reserved                :    [0x0, 0xffffffff],         bits : 31_0
*/
#define IME_RESERVED_17_OFS 0x01a4
REGDEF_BEGIN(IME_RESERVED_17)
REGDEF_BIT(reserved,        32)
REGDEF_END(IME_RESERVED_17)


/*
    reserved                :    [0x0, 0xffffffff],         bits : 31_0
*/
#define IME_RESERVED_18_OFS 0x01a8
REGDEF_BEGIN(IME_RESERVED_18)
REGDEF_BIT(reserved,        32)
REGDEF_END(IME_RESERVED_18)


/*
    reserved                :    [0x0, 0xffffffff],         bits : 31_0
*/
#define IME_RESERVED_19_OFS 0x01ac
REGDEF_BEGIN(IME_RESERVED_19)
REGDEF_BIT(reserved,        32)
REGDEF_END(IME_RESERVED_19)


/*
    ime_out_type_p4                 :    [0x0, 0x1],            bits : 1
    ime_scl_method_p4                 :    [0x0, 0x3],          bits : 3_2
    ime_out_en_p4                     :    [0x0, 0x1],          bits : 4
    ime_omat4                         :    [0x0, 0x3],          bits : 9_8
    ime_scl_enh_fact_p4             :    [0x0, 0xff],           bits : 19_12
    ime_scl_enh_bit_p4               :    [0x0, 0x1f],          bits : 24_20
*/
#define IME_OUTPUT_PATH4_CONTROL_REGISTER0_OFS 0x01b0
REGDEF_BEGIN(IME_OUTPUT_PATH4_CONTROL_REGISTER0)
REGDEF_BIT(,        1)
REGDEF_BIT(ime_out_type_p4,        1)
REGDEF_BIT(ime_scl_method_p4,        2)
REGDEF_BIT(ime_out_en_p4,        1)
REGDEF_BIT(,        3)
REGDEF_BIT(ime_omat4,        2)
REGDEF_BIT(,        2)
REGDEF_BIT(ime_scl_enh_fact_p4,        8)
REGDEF_BIT(ime_scl_enh_bit_p4,        5)
REGDEF_END(IME_OUTPUT_PATH4_CONTROL_REGISTER0)


/*
    h4_ud                   :    [0x0, 0x1],            bits : 0
    v4_ud                   :    [0x0, 0x1],            bits : 1
    h4_dnrate               :    [0x0, 0x1f],           bits : 6_2
    v4_dnrate               :    [0x0, 0x1f],           bits : 11_7
*/
#define IME_OUTPUT_PATH4_CONTROL_REGISTER1_OFS 0x01b4
REGDEF_BEGIN(IME_OUTPUT_PATH4_CONTROL_REGISTER1)
REGDEF_BIT(h4_ud,        1)
REGDEF_BIT(v4_ud,        1)
REGDEF_BIT(h4_dnrate,        5)
REGDEF_BIT(v4_dnrate,        5)
REGDEF_END(IME_OUTPUT_PATH4_CONTROL_REGISTER1)


/*
    h4_sfact                :    [0x0, 0xffff],         bits : 15_0
    v4_sfact                :    [0x0, 0xffff],         bits : 31_16
*/
#define IME_OUTPUT_PATH4_CONTROL_REGISTER2_OFS 0x01b8
REGDEF_BEGIN(IME_OUTPUT_PATH4_CONTROL_REGISTER2)
REGDEF_BIT(h4_sfact,        16)
REGDEF_BIT(v4_sfact,        16)
REGDEF_END(IME_OUTPUT_PATH4_CONTROL_REGISTER2)


/*
    reserved:    [0x0, 0xffffffff],         bits : 31_0
*/
#define IME_RESERVED18_OFS 0x01bc
REGDEF_BEGIN(IME_RESERVED18)
REGDEF_BIT(reserved,        32)
REGDEF_END(IME_RESERVED18)


/*
    h4_scl_size             :    [0x0, 0xffff],         bits : 15_0
    v4_scl_size             :    [0x0, 0xffff],         bits : 31_16
*/
#define IME_OUTPUT_PATH4_CONTROL_REGISTER7_OFS 0x01c0
REGDEF_BEGIN(IME_OUTPUT_PATH4_CONTROL_REGISTER7)
REGDEF_BIT(h4_scl_size,        16)
REGDEF_BIT(v4_scl_size,        16)
REGDEF_END(IME_OUTPUT_PATH4_CONTROL_REGISTER7)


/*
    ime_cropout_x_p4                :    [0x0, 0xffff],         bits : 15_0
    ime_cropout_y_p4                :    [0x0, 0xffff],         bits : 31_16
*/
#define IME_OUTPUT_PATH4_CONTROL_REGISTER8_OFS 0x01c4
REGDEF_BEGIN(IME_OUTPUT_PATH4_CONTROL_REGISTER8)
REGDEF_BIT(ime_cropout_x_p4,        16)
REGDEF_BIT(ime_cropout_y_p4,        16)
REGDEF_END(IME_OUTPUT_PATH4_CONTROL_REGISTER8)


/*
    h4_osize                :    [0x0, 0xffff],         bits : 15_0
    v4_osize                :    [0x0, 0xffff],         bits : 31_16
*/
#define IME_OUTPUT_PATH4_CONTROL_REGISTER9_OFS 0x01c8
REGDEF_BEGIN(IME_OUTPUT_PATH4_CONTROL_REGISTER9)
REGDEF_BIT(h4_osize,        16)
REGDEF_BIT(v4_osize,        16)
REGDEF_END(IME_OUTPUT_PATH4_CONTROL_REGISTER9)


/*
    ime_y_clamp_min_p4:    [0x0, 0xff],         bits : 7_0
    ime_y_clamp_max_p4:    [0x0, 0xff],         bits : 15_8
*/
#define IME_OUTPUT_PATH4_CONTROL_REGISTER10_OFS 0x01cc
REGDEF_BEGIN(IME_OUTPUT_PATH4_CONTROL_REGISTER10)
REGDEF_BIT(ime_y_clamp_min_p4,        8)
REGDEF_BIT(ime_y_clamp_max_p4,        8)
REGDEF_BIT(ime_uv_clamp_min_p4,       8)
REGDEF_BIT(ime_uv_clamp_max_p4,       8)
REGDEF_END(IME_OUTPUT_PATH4_CONTROL_REGISTER10)


/*
    dram_ofso_y_p4              :    [0x0, 0x3ffff],            bits : 19_2
*/
#define IME_OUTPUT_PATH4_DMA_LINEOFFSET_REGISTER0_OFS 0x01d0
REGDEF_BEGIN(IME_OUTPUT_PATH4_DMA_LINEOFFSET_REGISTER0)
REGDEF_BIT(,        2)
REGDEF_BIT(dram_ofso_y_p4,        18)
REGDEF_END(IME_OUTPUT_PATH4_DMA_LINEOFFSET_REGISTER0)


/*
    reserved                :    [0x0, 0xffffffff],         bits : 31_0
*/
#define IME_RESERVED19_OFS 0x01d4
REGDEF_BEGIN(IME_RESERVED19)
REGDEF_BIT(reserved,        32)
REGDEF_END(IME_RESERVED19)


/*
    dram_sao_p4_y0              :    [0x0, 0xffffffff],         bits : 31_0
*/
#define IME_OUTPUT_PATH4_DMA_BUFFER0_REGISTER0_OFS 0x01d8
REGDEF_BEGIN(IME_OUTPUT_PATH4_DMA_BUFFER0_REGISTER0)
REGDEF_BIT(dram_sao_p4_y0,        32)
REGDEF_END(IME_OUTPUT_PATH4_DMA_BUFFER0_REGISTER0)


/*
    reserved                :    [0x0, 0xffffffff],         bits : 31_0
*/
#define IME_RESERVED_31_OFS 0x01dc
REGDEF_BEGIN(IME_RESERVED_31)
REGDEF_BIT(reserved,        32)
REGDEF_END(IME_RESERVED_31)


/*
    ime_p2_isd_h_coef_all:    [0x0, 0x1ffff],           bits : 16_0
*/
#define IME_OUTPUT_PATH2_CONTROL_REGISTER26_OFS 0x01e0
REGDEF_BEGIN(IME_OUTPUT_PATH2_CONTROL_REGISTER26)
REGDEF_BIT(ime_p2_isd_h_coef_all,        17)
REGDEF_END(IME_OUTPUT_PATH2_CONTROL_REGISTER26)


/*
    ime_p2_isd_h_coef_half:    [0x0, 0x1ffff],          bits : 16_0
*/
#define IME_OUTPUT_PATH2_CONTROL_REGISTER27_OFS 0x01e4
REGDEF_BEGIN(IME_OUTPUT_PATH2_CONTROL_REGISTER27)
REGDEF_BIT(ime_p2_isd_h_coef_half,        17)
REGDEF_END(IME_OUTPUT_PATH2_CONTROL_REGISTER27)


/*
    ime_p2_isd_v_coef_all:    [0x0, 0x1ffff],           bits : 16_0
*/
#define IME_OUTPUT_PATH2_CONTROL_REGISTER28_OFS 0x01e8
REGDEF_BEGIN(IME_OUTPUT_PATH2_CONTROL_REGISTER28)
REGDEF_BIT(ime_p2_isd_v_coef_all,        17)
REGDEF_END(IME_OUTPUT_PATH2_CONTROL_REGISTER28)


/*
    ime_p2_isd_v_coef_half:    [0x0, 0x1ffff],          bits : 16_0
*/
#define IME_OUTPUT_PATH2_CONTROL_REGISTER29_OFS 0x01ec
REGDEF_BEGIN(IME_OUTPUT_PATH2_CONTROL_REGISTER29)
REGDEF_BIT(ime_p2_isd_v_coef_half,        17)
REGDEF_END(IME_OUTPUT_PATH2_CONTROL_REGISTER29)


/*
    ime_p3_isd_h_coef_all:    [0x0, 0x1ffff],           bits : 16_0
*/
#define IME_OUTPUT_PATH3_CONTROL_REGISTER25_OFS 0x01f0
REGDEF_BEGIN(IME_OUTPUT_PATH3_CONTROL_REGISTER25)
REGDEF_BIT(ime_p3_isd_h_coef_all,        17)
REGDEF_END(IME_OUTPUT_PATH3_CONTROL_REGISTER25)


/*
    ime_p3_isd_h_coef_half:    [0x0, 0x1ffff],          bits : 16_0
*/
#define IME_OUTPUT_PATH3_CONTROL_REGISTER26_OFS 0x01f4
REGDEF_BEGIN(IME_OUTPUT_PATH3_CONTROL_REGISTER26)
REGDEF_BIT(ime_p3_isd_h_coef_half,        17)
REGDEF_END(IME_OUTPUT_PATH3_CONTROL_REGISTER26)


/*
    ime_p3_isd_v_coef_all:    [0x0, 0x1ffff],           bits : 16_0
*/
#define IME_OUTPUT_PATH3_CONTROL_REGISTER27_OFS 0x01f8
REGDEF_BEGIN(IME_OUTPUT_PATH3_CONTROL_REGISTER27)
REGDEF_BIT(ime_p3_isd_v_coef_all,        17)
REGDEF_END(IME_OUTPUT_PATH3_CONTROL_REGISTER27)


/*
    ime_p3_isd_v_coef_half:    [0x0, 0x1ffff],          bits : 16_0
*/
#define IME_OUTPUT_PATH3_CONTROL_REGISTER28_OFS 0x01fc
REGDEF_BEGIN(IME_OUTPUT_PATH3_CONTROL_REGISTER28)
REGDEF_BIT(ime_p3_isd_v_coef_half,        17)
REGDEF_END(IME_OUTPUT_PATH3_CONTROL_REGISTER28)


/*
    chra_isd_h_coef_all:    [0x0, 0x1ffff],         bits : 16_0
*/
#define IME_CHROMA_ADAPTATION_SUBIMAGE_OUTPUT_REGISTER9_OFS 0x0200
REGDEF_BEGIN(IME_CHROMA_ADAPTATION_SUBIMAGE_OUTPUT_REGISTER9)
REGDEF_BIT(chra_isd_h_coef_all,        17)
REGDEF_END(IME_CHROMA_ADAPTATION_SUBIMAGE_OUTPUT_REGISTER9)


/*
    chra_isd_h_coef_half:    [0x0, 0x1ffff],            bits : 16_0
*/
#define IME_CHROMA_ADAPTATION_SUBIMAGE_OUTPUT_REGISTER10_OFS 0x0204
REGDEF_BEGIN(IME_CHROMA_ADAPTATION_SUBIMAGE_OUTPUT_REGISTER10)
REGDEF_BIT(chra_isd_h_coef_half,        17)
REGDEF_END(IME_CHROMA_ADAPTATION_SUBIMAGE_OUTPUT_REGISTER10)


/*
    chra_isd_v_coef_all:    [0x0, 0x1ffff],         bits : 16_0
*/
#define IME_CHROMA_ADAPTATION_SUBIMAGE_OUTPUT_REGISTER11_OFS 0x0208
REGDEF_BEGIN(IME_CHROMA_ADAPTATION_SUBIMAGE_OUTPUT_REGISTER11)
REGDEF_BIT(chra_isd_v_coef_all,        17)
REGDEF_END(IME_CHROMA_ADAPTATION_SUBIMAGE_OUTPUT_REGISTER11)


/*
    chra_isd_v_coef_half:    [0x0, 0x1ffff],            bits : 16_0
*/
#define IME_CHROMA_ADAPTATION_SUBIMAGE_OUTPUT_REGISTER12_OFS 0x020c
REGDEF_BEGIN(IME_CHROMA_ADAPTATION_SUBIMAGE_OUTPUT_REGISTER12)
REGDEF_BIT(chra_isd_v_coef_half,        17)
REGDEF_END(IME_CHROMA_ADAPTATION_SUBIMAGE_OUTPUT_REGISTER12)


/*
    reserved                :    [0x0, 0xffffffff],         bits : 31_0
*/
#define IME_RESERVED_32_OFS 0x0210
REGDEF_BEGIN(IME_RESERVED_32)
REGDEF_BIT(reserved,        32)
REGDEF_END(IME_RESERVED_32)


/*
    reserved                :    [0x0, 0xffffffff],         bits : 31_0
*/
#define IME_RESERVED_33_OFS 0x0214
REGDEF_BEGIN(IME_RESERVED_33)
REGDEF_BIT(reserved,        32)
REGDEF_END(IME_RESERVED_33)


/*
    reserved                :    [0x0, 0xffffffff],         bits : 31_0
*/
#define IME_RESERVED_34_OFS 0x0218
REGDEF_BEGIN(IME_RESERVED_34)
REGDEF_BIT(reserved,        32)
REGDEF_END(IME_RESERVED_34)


/*
    reserved                :    [0x0, 0xffffffff],         bits : 31_0
*/
#define IME_RESERVED_35_OFS 0x021c
REGDEF_BEGIN(IME_RESERVED_35)
REGDEF_BIT(reserved,        32)
REGDEF_END(IME_RESERVED_35)


/*
    chra_h_size             :    [0x0, 0xffff],         bits : 15_0
    chra_v_size             :    [0x0, 0xffff],         bits : 31_16
*/
#define IME_CHROMA_ADAPTATION_INPUT_IMAGE_REGISTER0_OFS 0x0220
REGDEF_BEGIN(IME_CHROMA_ADAPTATION_INPUT_IMAGE_REGISTER0)
REGDEF_BIT(chra_h_size,        16)
REGDEF_BIT(chra_v_size,        16)
REGDEF_END(IME_CHROMA_ADAPTATION_INPUT_IMAGE_REGISTER0)


/*
    chra_h_sfact                :    [0x0, 0xffff],         bits : 15_0
    chra_v_sfact                :    [0x0, 0xffff],         bits : 31_16
*/
#define IME_CHROMA_ADAPTATION_INPUT_IMAGE_REGISTER1_OFS 0x0224
REGDEF_BEGIN(IME_CHROMA_ADAPTATION_INPUT_IMAGE_REGISTER1)
REGDEF_BIT(chra_h_sfact,        16)
REGDEF_BIT(chra_v_sfact,        16)
REGDEF_END(IME_CHROMA_ADAPTATION_INPUT_IMAGE_REGISTER1)


/*
    chra_dram_y_ofsi                :    [0x0, 0x3ffff],            bits : 19_2
*/
#define IME_CHROMA_ADAPTATION_INPUT_IMAGE_REGISTER2_OFS 0x0228
REGDEF_BEGIN(IME_CHROMA_ADAPTATION_INPUT_IMAGE_REGISTER2)
REGDEF_BIT(,        2)
REGDEF_BIT(chra_dram_y_ofsi,        18)
REGDEF_END(IME_CHROMA_ADAPTATION_INPUT_IMAGE_REGISTER2)


/*
    chra_dram_y_sai0                :    [0x0, 0x3fffffff],         bits : 31_2
*/
#define IME_CHROMA_ADAPTATION_INPUT_IMAGE_REGISTER3_OFS 0x022c
REGDEF_BEGIN(IME_CHROMA_ADAPTATION_INPUT_IMAGE_REGISTER3)
REGDEF_BIT(,        2)
REGDEF_BIT(chra_dram_y_sai0,        30)
REGDEF_END(IME_CHROMA_ADAPTATION_INPUT_IMAGE_REGISTER3)


/*
    chra_fmt                      :    [0x0, 0x1],          bits : 0
    chra_in_bypass              :    [0x0, 0x1],            bits : 1
    chra_src                      :    [0x0, 0x1],          bits : 2
    chra_bypass                :    [0x0, 0x1],         bits : 3
*/
#define IME_CHROMA_ADAPTATION_PING_PONG_BUFFER_STATUS_REGISTER0_OFS 0x0230
REGDEF_BEGIN(IME_CHROMA_ADAPTATION_PING_PONG_BUFFER_STATUS_REGISTER0)
REGDEF_BIT(chra_fmt,        1)
REGDEF_BIT(chra_in_bypass,        1)
REGDEF_BIT(chra_src,        1)
REGDEF_BIT(chra_bypass,        1)
REGDEF_END(IME_CHROMA_ADAPTATION_PING_PONG_BUFFER_STATUS_REGISTER0)


/*
    chra_ca_ctr_u               :    [0x0, 0xff],           bits : 7_0
    chra_ca_ctr_v               :    [0x0, 0xff],           bits : 15_8
*/
#define IME_CHROMA_ADJUSTMENT_REGISTER0_OFS 0x0234
REGDEF_BEGIN(IME_CHROMA_ADJUSTMENT_REGISTER0)
REGDEF_BIT(chra_ca_ctr_u,        8)
REGDEF_BIT(chra_ca_ctr_v,        8)
REGDEF_END(IME_CHROMA_ADJUSTMENT_REGISTER0)


/*
    chra_ca_rng               :    [0x0, 0x3],          bits : 1_0
    chra_ca_wtprc               :    [0x0, 0x3],            bits : 3_2
    chra_ca_th                 :    [0x0, 0xf],         bits : 7_4
    chra_ca_wts               :    [0x0, 0x3f],         bits : 17_12
    chra_ca_wte               :    [0x0, 0x3f],         bits : 25_20
*/
#define IME_CHROMA_ADJUSTMENT_REGISTER1_OFS 0x0238
REGDEF_BEGIN(IME_CHROMA_ADJUSTMENT_REGISTER1)
REGDEF_BIT(chra_ca_rng,        2)
REGDEF_BIT(chra_ca_wtprc,        2)
REGDEF_BIT(chra_ca_th,        4)
REGDEF_BIT(,        4)
REGDEF_BIT(chra_ca_wts,        6)
REGDEF_BIT(,        2)
REGDEF_BIT(chra_ca_wte,        6)
REGDEF_END(IME_CHROMA_ADJUSTMENT_REGISTER1)


/*
    chra_refy_wt                :    [0x0, 0x1f],           bits : 4_0
    chra_refc_wt                :    [0x0, 0x1f],           bits : 9_5
    chra_out_wt              :    [0x0, 0x1f],          bits : 14_10
    luma_refy_wt                :    [0x0, 0x1f],           bits : 19_15
    luma_out_wt              :    [0x0, 0x1f],          bits : 24_20
*/
#define IME_CHROMA_ADAPTATION_REGISTER0_OFS 0x023c
REGDEF_BEGIN(IME_CHROMA_ADAPTATION_REGISTER0)
REGDEF_BIT(chra_refy_wt,        5)
REGDEF_BIT(chra_refc_wt,        5)
REGDEF_BIT(chra_out_wt,        5)
REGDEF_BIT(luma_refy_wt,        5)
REGDEF_BIT(luma_out_wt,        5)
REGDEF_END(IME_CHROMA_ADAPTATION_REGISTER0)


/*
    chra_y_rng                :    [0x0, 0x3],          bits : 1_0
    chra_y_wtprc                :    [0x0, 0x3],            bits : 3_2
    chra_y_th                  :    [0x0, 0x1f],            bits : 8_4
    chra_y_wts                :    [0x0, 0x3f],         bits : 17_12
    chra_y_wte                :    [0x0, 0x3f],         bits : 25_20
*/
#define IME_CHROMA_ADAPTATION_REGISTER1_OFS 0x0240
REGDEF_BEGIN(IME_CHROMA_ADAPTATION_REGISTER1)
REGDEF_BIT(chra_y_rng,        2)
REGDEF_BIT(chra_y_wtprc,        2)
REGDEF_BIT(chra_y_th,        5)
REGDEF_BIT(,        3)
REGDEF_BIT(chra_y_wts,        6)
REGDEF_BIT(,        2)
REGDEF_BIT(chra_y_wte,        6)
REGDEF_END(IME_CHROMA_ADAPTATION_REGISTER1)


/*
    chra_uv_rng               :    [0x0, 0x7],          bits : 2_0
    chra_uv_wtprc               :    [0x0, 0x3],            bits : 5_4
    chra_uv_th                 :    [0x0, 0x3f],            bits : 13_8
    chra_uv_wts               :    [0x0, 0x7f],         bits : 22_16
    chra_uv_wte               :    [0x0, 0x7f],         bits : 30_24
*/
#define IME_CHROMA_ADAPTATION_REGISTER2_OFS 0x0244
REGDEF_BEGIN(IME_CHROMA_ADAPTATION_REGISTER2)
REGDEF_BIT(chra_uv_rng,        3)
REGDEF_BIT(,        1)
REGDEF_BIT(chra_uv_wtprc,        2)
REGDEF_BIT(,        2)
REGDEF_BIT(chra_uv_th,        6)
REGDEF_BIT(,        2)
REGDEF_BIT(chra_uv_wts,        7)
REGDEF_BIT(,        1)
REGDEF_BIT(chra_uv_wte,        7)
REGDEF_END(IME_CHROMA_ADAPTATION_REGISTER2)


/*
    luma_rng                  :    [0x0, 0x3],          bits : 1_0
    luma_wtprc              :    [0x0, 0x3],            bits : 3_2
    luma_th                :    [0x0, 0x1f],            bits : 8_4
    luma_wts                  :    [0x0, 0x3f],         bits : 17_12
    luma_wte                  :    [0x0, 0x3f],         bits : 25_20
*/
#define IME_CHROMA_ADAPTATION_REGISTER3_OFS 0x0248
REGDEF_BEGIN(IME_CHROMA_ADAPTATION_REGISTER3)
REGDEF_BIT(luma_rng,        2)
REGDEF_BIT(luma_wtprc,        2)
REGDEF_BIT(luma_th,        5)
REGDEF_BIT(,        3)
REGDEF_BIT(luma_wts,        6)
REGDEF_BIT(,        2)
REGDEF_BIT(luma_wte,        6)
REGDEF_END(IME_CHROMA_ADAPTATION_REGISTER3)


/*
    reserved                :    [0x0, 0xffffffff],         bits : 31_0
*/
#define IME_RESERVED_36_OFS 0x024c
REGDEF_BEGIN(IME_RESERVED_36)
REGDEF_BIT(reserved,        32)
REGDEF_END(IME_RESERVED_36)


/*
    chra_h_dnrate               :    [0x0, 0x1f],           bits : 6_2
    chra_v_dnrate               :    [0x0, 0x1f],           bits : 11_7
    chra_subout_src  :    [0x0, 0x1],           bits : 31
*/
#define IME_CHROMA_ADAPTATION_SUBIMAGE_OUTPUT_REGISTER0_OFS 0x0250
REGDEF_BEGIN(IME_CHROMA_ADAPTATION_SUBIMAGE_OUTPUT_REGISTER0)
REGDEF_BIT(,        2)
REGDEF_BIT(chra_h_dnrate,        5)
REGDEF_BIT(chra_v_dnrate,        5)
REGDEF_BIT(,        19)
REGDEF_BIT(chra_subout_src,        1)
REGDEF_END(IME_CHROMA_ADAPTATION_SUBIMAGE_OUTPUT_REGISTER0)


/*
    chra_h_sfact                :    [0x0, 0xffff],         bits : 15_0
    chra_v_sfact                :    [0x0, 0xffff],         bits : 31_16
*/
#define IME_CHROMA_ADAPTATION_SUBIMAGE_OUTPUT_REGISTER1_OFS 0x0254
REGDEF_BEGIN(IME_CHROMA_ADAPTATION_SUBIMAGE_OUTPUT_REGISTER1)
REGDEF_BIT(chra_h_sfact,        16)
REGDEF_BIT(chra_v_sfact,        16)
REGDEF_END(IME_CHROMA_ADAPTATION_SUBIMAGE_OUTPUT_REGISTER1)


/*
    chra_isd_h_base             :    [0x0, 0x1fff],         bits : 12_0
    chra_isd_v_base             :    [0x0, 0x1fff],         bits : 28_16
*/
#define IME_CHROMA_ADAPTATION_SUBIMAGE_OUTPUT_REGISTER2_OFS 0x0258
REGDEF_BEGIN(IME_CHROMA_ADAPTATION_SUBIMAGE_OUTPUT_REGISTER2)
REGDEF_BIT(chra_isd_h_base,        13)
REGDEF_BIT(,        3)
REGDEF_BIT(chra_isd_v_base,        13)
REGDEF_END(IME_CHROMA_ADAPTATION_SUBIMAGE_OUTPUT_REGISTER2)


/*
    chra_isd_h_sfact0               :    [0x0, 0x1fff],         bits : 12_0
    chra_isd_v_sfact0               :    [0x0, 0x1fff],         bits : 28_16
*/
#define IME_CHROMA_ADAPTATION_SUBIMAGE_OUTPUT_REGISTER3_OFS 0x025c
REGDEF_BEGIN(IME_CHROMA_ADAPTATION_SUBIMAGE_OUTPUT_REGISTER3)
REGDEF_BIT(chra_isd_h_sfact0,        13)
REGDEF_BIT(,        3)
REGDEF_BIT(chra_isd_v_sfact0,        13)
REGDEF_END(IME_CHROMA_ADAPTATION_SUBIMAGE_OUTPUT_REGISTER3)


/*
    chra_isd_h_sfact1               :    [0x0, 0x1fff],         bits : 12_0
    chra_isd_v_sfact1               :    [0x0, 0x1fff],         bits : 28_16
*/
#define IME_CHROMA_ADAPTATION_SUBIMAGE_OUTPUT_REGISTER4_OFS 0x0260
REGDEF_BEGIN(IME_CHROMA_ADAPTATION_SUBIMAGE_OUTPUT_REGISTER4)
REGDEF_BIT(chra_isd_h_sfact1,        13)
REGDEF_BIT(,        3)
REGDEF_BIT(chra_isd_v_sfact1,        13)
REGDEF_END(IME_CHROMA_ADAPTATION_SUBIMAGE_OUTPUT_REGISTER4)


/*
    chra_isd_h_sfact2               :    [0x0, 0x1fff],         bits : 12_0
    chra_isd_v_sfact2               :    [0x0, 0x1fff],         bits : 28_16
*/
#define IME_CHROMA_ADAPTATION_SUBIMAGE_OUTPUT_REGISTER5_OFS 0x0264
REGDEF_BEGIN(IME_CHROMA_ADAPTATION_SUBIMAGE_OUTPUT_REGISTER5)
REGDEF_BIT(chra_isd_h_sfact2,        13)
REGDEF_BIT(,        3)
REGDEF_BIT(chra_isd_v_sfact2,        13)
REGDEF_END(IME_CHROMA_ADAPTATION_SUBIMAGE_OUTPUT_REGISTER5)


/*
    chra_dram_ofso              :    [0x0, 0x3ffff],            bits : 19_2
*/
#define IME_CHROMA_ADAPTATION_SUBIMAGE_OUTPUT_REGISTER6_OFS 0x0268
REGDEF_BEGIN(IME_CHROMA_ADAPTATION_SUBIMAGE_OUTPUT_REGISTER6)
REGDEF_BIT(,        2)
REGDEF_BIT(chra_dram_ofso,        18)
REGDEF_END(IME_CHROMA_ADAPTATION_SUBIMAGE_OUTPUT_REGISTER6)


/*
    chra_dram_sao               :    [0x0, 0x3fffffff],         bits : 31_2
*/
#define IME_CHROMA_ADAPTATION_SUBIMAGE_OUTPUT_REGISTER7_OFS 0x026c
REGDEF_BEGIN(IME_CHROMA_ADAPTATION_SUBIMAGE_OUTPUT_REGISTER7)
REGDEF_BIT(,        2)
REGDEF_BIT(chra_dram_sao,        30)
REGDEF_END(IME_CHROMA_ADAPTATION_SUBIMAGE_OUTPUT_REGISTER7)


/*
    chra_isd_h_coef_num:    [0x0, 0x1f],            bits : 4_0
    chra_isd_v_coef_num:    [0x0, 0x1f],            bits : 12_8
*/
#define IME_CHROMA_ADAPTATION_SUBIMAGE_OUTPUT_REGISTER8_OFS 0x0270
REGDEF_BEGIN(IME_CHROMA_ADAPTATION_SUBIMAGE_OUTPUT_REGISTER8)
REGDEF_BIT(chra_isd_h_coef_num,        5)
REGDEF_BIT(,        3)
REGDEF_BIT(chra_isd_v_coef_num,        5)
REGDEF_END(IME_CHROMA_ADAPTATION_SUBIMAGE_OUTPUT_REGISTER8)


/*
    reserved                :    [0x0, 0xffffffff],         bits : 31_0
*/
#define IME_RESERVED_37_OFS 0x0274
REGDEF_BEGIN(IME_RESERVED_37)
REGDEF_BIT(reserved,        32)
REGDEF_END(IME_RESERVED_37)


/*
    reserved                :    [0x0, 0xffffffff],         bits : 31_0
*/
#define IME_RESERVED_38_OFS 0x0278
REGDEF_BEGIN(IME_RESERVED_38)
REGDEF_BIT(reserved,        32)
REGDEF_END(IME_RESERVED_38)


/*
    reserved                :    [0x0, 0xffffffff],         bits : 31_0
*/
#define IME_RESERVED_39_OFS 0x027c
REGDEF_BEGIN(IME_RESERVED_39)
REGDEF_BIT(reserved,        32)
REGDEF_END(IME_RESERVED_39)


/*
    dbcs_ctr_u      :    [0x0, 0xff],           bits : 7_0
    dbcs_ctr_v                :    [0x0, 0xff],         bits : 15_8
    dbcs_mode                  :    [0x0, 0x3],         bits : 17_16
    dbcs_step_y              :    [0x0, 0x3],           bits : 21_20
    dbcs_step_uv                :    [0x0, 0x3],            bits : 23_22
*/
#define IME_DARK_AND_BRIGHT_REGION_CHROMA_SUPPRESSION_REGISTER0_OFS 0x0280
REGDEF_BEGIN(IME_DARK_AND_BRIGHT_REGION_CHROMA_SUPPRESSION_REGISTER0)
REGDEF_BIT(dbcs_ctr_u,        8)
REGDEF_BIT(dbcs_ctr_v,        8)
REGDEF_BIT(dbcs_mode,        2)
REGDEF_BIT(,        2)
REGDEF_BIT(dbcs_step_y,        2)
REGDEF_BIT(dbcs_step_uv,        2)
REGDEF_END(IME_DARK_AND_BRIGHT_REGION_CHROMA_SUPPRESSION_REGISTER0)


/*
    dbcs_y_wt0              :    [0x0, 0x1f],           bits : 4_0
    dbcs_y_wt1              :    [0x0, 0x1f],           bits : 9_5
    dbcs_y_wt2              :    [0x0, 0x1f],           bits : 14_10
    dbcs_y_wt3              :    [0x0, 0x1f],           bits : 19_15
    dbcs_y_wt4              :    [0x0, 0x1f],           bits : 24_20
    dbcs_y_wt5              :    [0x0, 0x1f],           bits : 29_25
*/
#define IME_DARK_AND_BRIGHT_REGION_CHROMA_SUPPRESSION_WEIGHTING_REGISTER0_OFS 0x0284
REGDEF_BEGIN(IME_DARK_AND_BRIGHT_REGION_CHROMA_SUPPRESSION_WEIGHTING_REGISTER0)
REGDEF_BIT(dbcs_y_wt0,        5)
REGDEF_BIT(dbcs_y_wt1,        5)
REGDEF_BIT(dbcs_y_wt2,        5)
REGDEF_BIT(dbcs_y_wt3,        5)
REGDEF_BIT(dbcs_y_wt4,        5)
REGDEF_BIT(dbcs_y_wt5,        5)
REGDEF_END(IME_DARK_AND_BRIGHT_REGION_CHROMA_SUPPRESSION_WEIGHTING_REGISTER0)


/*
    dbcs_y_wt6               :    [0x0, 0x1f],          bits : 4_0
    dbcs_y_wt7               :    [0x0, 0x1f],          bits : 9_5
    dbcs_y_wt8               :    [0x0, 0x1f],          bits : 14_10
    dbcs_y_wt9               :    [0x0, 0x1f],          bits : 19_15
    dbcs_y_wt10             :    [0x0, 0x1f],           bits : 24_20
    dbcs_y_wt11             :    [0x0, 0x1f],           bits : 29_25
*/
#define IME_DARK_AND_BRIGHT_REGION_CHROMA_SUPPRESSION_WEIGHTING_REGISTER1_OFS 0x0288
REGDEF_BEGIN(IME_DARK_AND_BRIGHT_REGION_CHROMA_SUPPRESSION_WEIGHTING_REGISTER1)
REGDEF_BIT(dbcs_y_wt6,        5)
REGDEF_BIT(dbcs_y_wt7,        5)
REGDEF_BIT(dbcs_y_wt8,        5)
REGDEF_BIT(dbcs_y_wt9,        5)
REGDEF_BIT(dbcs_y_wt10,        5)
REGDEF_BIT(dbcs_y_wt11,        5)
REGDEF_END(IME_DARK_AND_BRIGHT_REGION_CHROMA_SUPPRESSION_WEIGHTING_REGISTER1)


/*
    dbcs_y_wt12             :    [0x0, 0x1f],           bits : 4_0
    dbcs_y_wt13             :    [0x0, 0x1f],           bits : 9_5
    dbcs_y_wt14             :    [0x0, 0x1f],           bits : 14_10
    dbcs_y_wt15             :    [0x0, 0x1f],           bits : 19_15
*/
#define IME_DARK_AND_BRIGHT_REGION_CHROMA_SUPPRESSION_WEIGHTING_REGISTER2_OFS 0x028c
REGDEF_BEGIN(IME_DARK_AND_BRIGHT_REGION_CHROMA_SUPPRESSION_WEIGHTING_REGISTER2)
REGDEF_BIT(dbcs_y_wt12,        5)
REGDEF_BIT(dbcs_y_wt13,        5)
REGDEF_BIT(dbcs_y_wt14,        5)
REGDEF_BIT(dbcs_y_wt15,        5)
REGDEF_END(IME_DARK_AND_BRIGHT_REGION_CHROMA_SUPPRESSION_WEIGHTING_REGISTER2)


/*
    dbcs_c_wt0              :    [0x0, 0x1f],           bits : 4_0
    dbcs_c_wt1              :    [0x0, 0x1f],           bits : 9_5
    dbcs_c_wt2              :    [0x0, 0x1f],           bits : 14_10
    dbcs_c_wt3              :    [0x0, 0x1f],           bits : 19_15
    dbcs_c_wt4              :    [0x0, 0x1f],           bits : 24_20
    dbcs_c_wt5              :    [0x0, 0x1f],           bits : 29_25
*/
#define IME_DARK_AND_BRIGHT_REGION_CHROMA_SUPPRESSION_WEIGHTING_REGISTER3_OFS 0x0290
REGDEF_BEGIN(IME_DARK_AND_BRIGHT_REGION_CHROMA_SUPPRESSION_WEIGHTING_REGISTER3)
REGDEF_BIT(dbcs_c_wt0,        5)
REGDEF_BIT(dbcs_c_wt1,        5)
REGDEF_BIT(dbcs_c_wt2,        5)
REGDEF_BIT(dbcs_c_wt3,        5)
REGDEF_BIT(dbcs_c_wt4,        5)
REGDEF_BIT(dbcs_c_wt5,        5)
REGDEF_END(IME_DARK_AND_BRIGHT_REGION_CHROMA_SUPPRESSION_WEIGHTING_REGISTER3)


/*
    dbcs_c_wt6               :    [0x0, 0x1f],          bits : 4_0
    dbcs_c_wt7               :    [0x0, 0x1f],          bits : 9_5
    dbcs_c_wt8               :    [0x0, 0x1f],          bits : 14_10
    dbcs_c_wt9               :    [0x0, 0x1f],          bits : 19_15
    dbcs_c_wt10             :    [0x0, 0x1f],           bits : 24_20
    dbcs_c_wt11             :    [0x0, 0x1f],           bits : 29_25
*/
#define IME_DARK_AND_BRIGHT_REGION_CHROMA_SUPPRESSION_WEIGHTING_REGISTER4_OFS 0x0294
REGDEF_BEGIN(IME_DARK_AND_BRIGHT_REGION_CHROMA_SUPPRESSION_WEIGHTING_REGISTER4)
REGDEF_BIT(dbcs_c_wt6,        5)
REGDEF_BIT(dbcs_c_wt7,        5)
REGDEF_BIT(dbcs_c_wt8,        5)
REGDEF_BIT(dbcs_c_wt9,        5)
REGDEF_BIT(dbcs_c_wt10,        5)
REGDEF_BIT(dbcs_c_wt11,        5)
REGDEF_END(IME_DARK_AND_BRIGHT_REGION_CHROMA_SUPPRESSION_WEIGHTING_REGISTER4)


/*
    dbcs_c_wt12             :    [0x0, 0x1f],           bits : 4_0
    dbcs_c_wt13             :    [0x0, 0x1f],           bits : 9_5
    dbcs_c_wt14             :    [0x0, 0x1f],           bits : 14_10
    dbcs_c_wt15             :    [0x0, 0x1f],           bits : 19_15
*/
#define IME_DARK_AND_BRIGHT_REGION_CHROMA_SUPPRESSION_WEIGHTING_REGISTER5_OFS 0x0298
REGDEF_BEGIN(IME_DARK_AND_BRIGHT_REGION_CHROMA_SUPPRESSION_WEIGHTING_REGISTER5)
REGDEF_BIT(dbcs_c_wt12,        5)
REGDEF_BIT(dbcs_c_wt13,        5)
REGDEF_BIT(dbcs_c_wt14,        5)
REGDEF_BIT(dbcs_c_wt15,        5)
REGDEF_END(IME_DARK_AND_BRIGHT_REGION_CHROMA_SUPPRESSION_WEIGHTING_REGISTER5)


/*
    reserved                :    [0x0, 0xffffffff],         bits : 31_0
*/
#define IME_RESERVED_40_OFS 0x029c
REGDEF_BEGIN(IME_RESERVED_40)
REGDEF_BIT(reserved,        32)
REGDEF_END(IME_RESERVED_40)


/*
    reserved                :    [0x0, 0xffffffff],         bits : 31_0
*/
#define IME_RESERVED_41_OFS 0x02a0
REGDEF_BEGIN(IME_RESERVED_41)
REGDEF_BIT(reserved,        32)
REGDEF_END(IME_RESERVED_41)


/*
    reserved                :    [0x0, 0xffffffff],         bits : 31_0
*/
#define IME_RESERVED_42_OFS 0x02a4
REGDEF_BEGIN(IME_RESERVED_42)
REGDEF_BIT(reserved,        32)
REGDEF_END(IME_RESERVED_42)


/*
    reserved                :    [0x0, 0xffffffff],         bits : 31_0
*/
#define IME_RESERVED_43_OFS 0x02a8
REGDEF_BEGIN(IME_RESERVED_43)
REGDEF_BIT(reserved,        32)
REGDEF_END(IME_RESERVED_43)


/*
    reserved                :    [0x0, 0xffffffff],         bits : 31_0
*/
#define IME_RESERVED_44_OFS 0x02ac
REGDEF_BEGIN(IME_RESERVED_44)
REGDEF_BIT(reserved,        32)
REGDEF_END(IME_RESERVED_44)


/*
    reserved                :    [0x0, 0xffffffff],         bits : 31_0
*/
#define IME_RESERVED_45_OFS 0x02b0
REGDEF_BEGIN(IME_RESERVED_45)
REGDEF_BIT(reserved,        32)
REGDEF_END(IME_RESERVED_45)


/*
    reserved                :    [0x0, 0xffffffff],         bits : 31_0
*/
#define IME_RESERVED_46_OFS 0x02b4
REGDEF_BEGIN(IME_RESERVED_46)
REGDEF_BIT(reserved,        32)
REGDEF_END(IME_RESERVED_46)


/*
    reserved                :    [0x0, 0xffffffff],         bits : 31_0
*/
#define IME_RESERVED_47_OFS 0x02b8
REGDEF_BEGIN(IME_RESERVED_47)
REGDEF_BIT(reserved,        32)
REGDEF_END(IME_RESERVED_47)


/*
    reserved                :    [0x0, 0xffffffff],         bits : 31_0
*/
#define IME_RESERVED_48_OFS 0x02bc
REGDEF_BEGIN(IME_RESERVED_48)
REGDEF_BIT(reserved,        32)
REGDEF_END(IME_RESERVED_48)


/*
    ime_ds0_hsize               :    [0x0, 0x1fff],         bits : 12_0
    ime_ds0_vsize               :    [0x0, 0x1fff],         bits : 28_16
*/
#define IME_DATA_STAMP_SET0_CONTROL_REGISTER0_OFS 0x02c0
REGDEF_BEGIN(IME_DATA_STAMP_SET0_CONTROL_REGISTER0)
REGDEF_BIT(ime_ds0_hsize,        13)
REGDEF_BIT(,        3)
REGDEF_BIT(ime_ds0_vsize,        13)
REGDEF_END(IME_DATA_STAMP_SET0_CONTROL_REGISTER0)


/*
    ime_ds0_posx                 :    [0x0, 0x1fff],            bits : 12_0
    ime_ds0_posy                 :    [0x0, 0x1fff],            bits : 28_16
    ime_ds0_ck_en               :    [0x0, 0x1],            bits : 29
    ime_ds0_fmt               :    [0x0, 0x3],          bits : 31_30
*/
#define IME_DATA_STAMP_SET0_POSITION_REGISTER_OFS 0x02c4
REGDEF_BEGIN(IME_DATA_STAMP_SET0_POSITION_REGISTER)
REGDEF_BIT(ime_ds0_posx,        13)
REGDEF_BIT(,        3)
REGDEF_BIT(ime_ds0_posy,        13)
REGDEF_BIT(ime_ds0_ck_en,        1)
REGDEF_BIT(ime_ds0_fmt,        2)
REGDEF_END(IME_DATA_STAMP_SET0_POSITION_REGISTER)


/*
    ime_ds0_ck_a:    [0x0, 0xff],           bits : 7_0
    ime_ds0_ck_r:    [0x0, 0xff],           bits : 15_8
    ime_ds0_ck_g:    [0x0, 0xff],           bits : 23_16
    ime_ds0_ck_b:    [0x0, 0xff],           bits : 31_24
*/
#define IME_DATA_STAMP_SET0_COLOR_KEY_REGISTER_OFS 0x02c8
REGDEF_BEGIN(IME_DATA_STAMP_SET0_COLOR_KEY_REGISTER)
REGDEF_BIT(ime_ds0_ck_a,        8)
REGDEF_BIT(ime_ds0_ck_r,        8)
REGDEF_BIT(ime_ds0_ck_g,        8)
REGDEF_BIT(ime_ds0_ck_b,        8)
REGDEF_END(IME_DATA_STAMP_SET0_COLOR_KEY_REGISTER)


/*
    ime_ds0_gawet0              :    [0x0, 0xf],            bits : 3_0
    ime_ds0_gawet1              :    [0x0, 0xf],            bits : 7_4
    ime_ds0_plt_en    :    [0x0, 0x1],          bits : 8
    ime_ds0_ck_mode   :    [0x0, 0x1],          bits : 9
*/
#define IME_DATA_STAMP_SET0_WEIGHT_REGISTER_OFS 0x02cc
REGDEF_BEGIN(IME_DATA_STAMP_SET0_WEIGHT_REGISTER)
REGDEF_BIT(ime_ds0_gawet0,        4)
REGDEF_BIT(ime_ds0_gawet1,        4)
REGDEF_BIT(ime_ds0_plt_en,        1)
REGDEF_BIT(ime_ds0_ck_mode,        1)
REGDEF_END(IME_DATA_STAMP_SET0_WEIGHT_REGISTER)


/*
    ime_ds0_ofsi                :    [0x0, 0x3ffff],            bits : 19_2
*/
#define IME_DATA_STAMP_SET0_DMA_LINEOFFSET_REGISTER_OFS 0x02d0
REGDEF_BEGIN(IME_DATA_STAMP_SET0_DMA_LINEOFFSET_REGISTER)
REGDEF_BIT(,        2)
REGDEF_BIT(ime_ds0_ofsi,        18)
REGDEF_END(IME_DATA_STAMP_SET0_DMA_LINEOFFSET_REGISTER)


/*
    ime_ds0_sai:    [0x0, 0x3fffffff],          bits : 31_2
*/
#define IME_DATA_STAMP_SET0_DMA_ADDRESS_REGISTER_OFS 0x02d4
REGDEF_BEGIN(IME_DATA_STAMP_SET0_DMA_ADDRESS_REGISTER)
REGDEF_BIT(,        2)
REGDEF_BIT(ime_ds0_sai,        30)
REGDEF_END(IME_DATA_STAMP_SET0_DMA_ADDRESS_REGISTER)


/*
    ime_ds1_hsize               :    [0x0, 0x1fff],         bits : 12_0
    ime_ds1_vsize               :    [0x0, 0x1fff],         bits : 28_16
*/
#define IME_DATA_STAMP_SET1_CONTROL_REGISTER_OFS 0x02d8
REGDEF_BEGIN(IME_DATA_STAMP_SET1_CONTROL_REGISTER)
REGDEF_BIT(ime_ds1_hsize,        13)
REGDEF_BIT(,        3)
REGDEF_BIT(ime_ds1_vsize,        13)
REGDEF_END(IME_DATA_STAMP_SET1_CONTROL_REGISTER)


/*
    ime_ds1_posx                 :    [0x0, 0x1fff],            bits : 12_0
    ime_ds1_posy                 :    [0x0, 0x1fff],            bits : 28_16
    ime_ds1_ck_en               :    [0x0, 0x1],            bits : 29
    ime_ds1_fmt               :    [0x0, 0x3],          bits : 31_30
*/
#define IME_DATA_STAMP_SET1_POSITION_REGISTER_OFS 0x02dc
REGDEF_BEGIN(IME_DATA_STAMP_SET1_POSITION_REGISTER)
REGDEF_BIT(ime_ds1_posx,        13)
REGDEF_BIT(,        3)
REGDEF_BIT(ime_ds1_posy,        13)
REGDEF_BIT(ime_ds1_ck_en,        1)
REGDEF_BIT(ime_ds1_fmt,        2)
REGDEF_END(IME_DATA_STAMP_SET1_POSITION_REGISTER)


/*
    ime_ds1_ck_a:    [0x0, 0xff],           bits : 7_0
    ime_ds1_ck_r:    [0x0, 0xff],           bits : 15_8
    ime_ds1_ck_g:    [0x0, 0xff],           bits : 23_16
    ime_ds1_ck_b:    [0x0, 0xff],           bits : 31_24
*/
#define IME_DATA_STAMP_SET1_COLOR_KEY_REGISTER_OFS 0x02e0
REGDEF_BEGIN(IME_DATA_STAMP_SET1_COLOR_KEY_REGISTER)
REGDEF_BIT(ime_ds1_ck_a,        8)
REGDEF_BIT(ime_ds1_ck_r,        8)
REGDEF_BIT(ime_ds1_ck_g,        8)
REGDEF_BIT(ime_ds1_ck_b,        8)
REGDEF_END(IME_DATA_STAMP_SET1_COLOR_KEY_REGISTER)


/*
    ime_ds1_gawet0              :    [0x0, 0xf],            bits : 3_0
    ime_ds1_gawet1              :    [0x0, 0xf],            bits : 7_4
    ime_ds1_plt_en    :    [0x0, 0x1],          bits : 8
    ime_ds1_ck_mode   :    [0x0, 0x1],          bits : 9
*/
#define IME_DATA_STAMP_SET1_WEIGHT_REGISTER_OFS 0x02e4
REGDEF_BEGIN(IME_DATA_STAMP_SET1_WEIGHT_REGISTER)
REGDEF_BIT(ime_ds1_gawet0,        4)
REGDEF_BIT(ime_ds1_gawet1,        4)
REGDEF_BIT(ime_ds1_plt_en,        1)
REGDEF_BIT(ime_ds1_ck_mode,        1)
REGDEF_END(IME_DATA_STAMP_SET1_WEIGHT_REGISTER)


/*
    ime_ds1_ofsi                :    [0x0, 0x3ffff],            bits : 19_2
*/
#define IME_DATA_STAMP_SET1_DMA_LINEOFFSET_REGISTER_OFS 0x02e8
REGDEF_BEGIN(IME_DATA_STAMP_SET1_DMA_LINEOFFSET_REGISTER)
REGDEF_BIT(,        2)
REGDEF_BIT(ime_ds1_ofsi,        18)
REGDEF_END(IME_DATA_STAMP_SET1_DMA_LINEOFFSET_REGISTER)


/*
    ime_ds1_sai             :    [0x0, 0x3fffffff],         bits : 31_2
*/
#define IME_DATA_STAMP_SET1_DMA_ADDRESS_REGISTER_OFS 0x02ec
REGDEF_BEGIN(IME_DATA_STAMP_SET1_DMA_ADDRESS_REGISTER)
REGDEF_BIT(,        2)
REGDEF_BIT(ime_ds1_sai,        30)
REGDEF_END(IME_DATA_STAMP_SET1_DMA_ADDRESS_REGISTER)


/*
    ime_ds2_hsize               :    [0x0, 0x1fff],         bits : 12_0
    ime_ds2_vsize               :    [0x0, 0x1fff],         bits : 28_16
*/
#define IME_DATA_STAMP_SET2_CONTROL_REGISTER_OFS 0x02f0
REGDEF_BEGIN(IME_DATA_STAMP_SET2_CONTROL_REGISTER)
REGDEF_BIT(ime_ds2_hsize,        13)
REGDEF_BIT(,        3)
REGDEF_BIT(ime_ds2_vsize,        13)
REGDEF_END(IME_DATA_STAMP_SET2_CONTROL_REGISTER)


/*
    ime_ds2_posx                 :    [0x0, 0x1fff],            bits : 12_0
    ime_ds2_posy                 :    [0x0, 0x1fff],            bits : 28_16
    ime_ds2_ck_en               :    [0x0, 0x1],            bits : 29
    ime_ds2_fmt               :    [0x0, 0x3],          bits : 31_30
*/
#define IME_DATA_STAMP_SET2_POSITION_REGISTER_OFS 0x02f4
REGDEF_BEGIN(IME_DATA_STAMP_SET2_POSITION_REGISTER)
REGDEF_BIT(ime_ds2_posx,        13)
REGDEF_BIT(,        3)
REGDEF_BIT(ime_ds2_posy,        13)
REGDEF_BIT(ime_ds2_ck_en,        1)
REGDEF_BIT(ime_ds2_fmt,        2)
REGDEF_END(IME_DATA_STAMP_SET2_POSITION_REGISTER)


/*
    ime_ds2_ck_a:    [0x0, 0xff],           bits : 7_0
    ime_ds2_ck_r:    [0x0, 0xff],           bits : 15_8
    ime_ds2_ck_g:    [0x0, 0xff],           bits : 23_16
    ime_ds2_ck_b:    [0x0, 0xff],           bits : 31_24
*/
#define IME_DATA_STAMP_SET2_COLOR_KEY_REGISTER_OFS 0x02f8
REGDEF_BEGIN(IME_DATA_STAMP_SET2_COLOR_KEY_REGISTER)
REGDEF_BIT(ime_ds2_ck_a,        8)
REGDEF_BIT(ime_ds2_ck_r,        8)
REGDEF_BIT(ime_ds2_ck_g,        8)
REGDEF_BIT(ime_ds2_ck_b,        8)
REGDEF_END(IME_DATA_STAMP_SET2_COLOR_KEY_REGISTER)


/*
    ime_ds2_gawet0              :    [0x0, 0xf],            bits : 3_0
    ime_ds2_gawet1              :    [0x0, 0xf],            bits : 7_4
    ime_ds2_plt_en    :    [0x0, 0x1],          bits : 8
    ime_ds2_ck_mode   :    [0x0, 0x1],          bits : 9
*/
#define IME_DATA_STAMP_SET2_WEIGHT_REGISTER_OFS 0x02fc
REGDEF_BEGIN(IME_DATA_STAMP_SET2_WEIGHT_REGISTER)
REGDEF_BIT(ime_ds2_gawet0,        4)
REGDEF_BIT(ime_ds2_gawet1,        4)
REGDEF_BIT(ime_ds2_plt_en,        1)
REGDEF_BIT(ime_ds2_ck_mode,        1)
REGDEF_END(IME_DATA_STAMP_SET2_WEIGHT_REGISTER)


/*
    ime_ds2_ofsi                :    [0x0, 0x3ffff],            bits : 19_2
*/
#define IME_DATA_STAMP_SET2_DMA_LINEOFFSET_REGISTER_OFS 0x0300
REGDEF_BEGIN(IME_DATA_STAMP_SET2_DMA_LINEOFFSET_REGISTER)
REGDEF_BIT(,        2)
REGDEF_BIT(ime_ds2_ofsi,        18)
REGDEF_END(IME_DATA_STAMP_SET2_DMA_LINEOFFSET_REGISTER)


/*
    ime_ds2_sai             :    [0x0, 0x3fffffff],         bits : 31_2
*/
#define IME_DATA_STAMP_SET2_DMA_ADDRESS_REGISTER_OFS 0x0304
REGDEF_BEGIN(IME_DATA_STAMP_SET2_DMA_ADDRESS_REGISTER)
REGDEF_BIT(,        2)
REGDEF_BIT(ime_ds2_sai,        30)
REGDEF_END(IME_DATA_STAMP_SET2_DMA_ADDRESS_REGISTER)


/*
    ime_ds3_hsize               :    [0x0, 0x1fff],         bits : 12_0
    ime_ds3_vsize               :    [0x0, 0x1fff],         bits : 28_16
*/
#define IME_DATA_STAMP_SET3_CONTROL_REGISTER_OFS 0x0308
REGDEF_BEGIN(IME_DATA_STAMP_SET3_CONTROL_REGISTER)
REGDEF_BIT(ime_ds3_hsize,        13)
REGDEF_BIT(,        3)
REGDEF_BIT(ime_ds3_vsize,        13)
REGDEF_END(IME_DATA_STAMP_SET3_CONTROL_REGISTER)


/*
    ime_ds3_posx                 :    [0x0, 0x1fff],            bits : 12_0
    ime_ds3_posy                 :    [0x0, 0x1fff],            bits : 28_16
    ime_ds3_ck_en               :    [0x0, 0x1],            bits : 29
    ime_ds3_fmt               :    [0x0, 0x3],          bits : 31_30
*/
#define IME_DATA_STAMP_SET3_POSITION_REGISTER_OFS 0x030c
REGDEF_BEGIN(IME_DATA_STAMP_SET3_POSITION_REGISTER)
REGDEF_BIT(ime_ds3_posx,        13)
REGDEF_BIT(,        3)
REGDEF_BIT(ime_ds3_posy,        13)
REGDEF_BIT(ime_ds3_ck_en,        1)
REGDEF_BIT(ime_ds3_fmt,        2)
REGDEF_END(IME_DATA_STAMP_SET3_POSITION_REGISTER)


/*
    ime_ds3_ck_a:    [0x0, 0xff],           bits : 7_0
    ime_ds3_ck_r:    [0x0, 0xff],           bits : 15_8
    ime_ds3_ck_g:    [0x0, 0xff],           bits : 23_16
    ime_ds3_ck_b:    [0x0, 0xff],           bits : 31_24
*/
#define IME_DATA_STAMP_SET3_COLOR_KEY_REGISTER_OFS 0x0310
REGDEF_BEGIN(IME_DATA_STAMP_SET3_COLOR_KEY_REGISTER)
REGDEF_BIT(ime_ds3_ck_a,        8)
REGDEF_BIT(ime_ds3_ck_r,        8)
REGDEF_BIT(ime_ds3_ck_g,        8)
REGDEF_BIT(ime_ds3_ck_b,        8)
REGDEF_END(IME_DATA_STAMP_SET3_COLOR_KEY_REGISTER)


/*
    ime_ds3_gawet0              :    [0x0, 0xf],            bits : 3_0
    ime_ds3_gawet1              :    [0x0, 0xf],            bits : 7_4
    ime_ds3_plt_en    :    [0x0, 0x1],          bits : 8
    ime_ds3_ck_mode   :    [0x0, 0x1],          bits : 9
*/
#define IME_DATA_STAMP_SET3_WEIGHT_REGISTER_OFS 0x0314
REGDEF_BEGIN(IME_DATA_STAMP_SET3_WEIGHT_REGISTER)
REGDEF_BIT(ime_ds3_gawet0,        4)
REGDEF_BIT(ime_ds3_gawet1,        4)
REGDEF_BIT(ime_ds3_plt_en,        1)
REGDEF_BIT(ime_ds3_ck_mode,        1)
REGDEF_END(IME_DATA_STAMP_SET3_WEIGHT_REGISTER)


/*
    ime_ds3_ofsi                :    [0x0, 0x3ffff],            bits : 19_2
*/
#define IME_DATA_STAMP_SET3_DMA_LINEOFFSET_REGISTER_OFS 0x0318
REGDEF_BEGIN(IME_DATA_STAMP_SET3_DMA_LINEOFFSET_REGISTER)
REGDEF_BIT(,        2)
REGDEF_BIT(ime_ds3_ofsi,        18)
REGDEF_END(IME_DATA_STAMP_SET3_DMA_LINEOFFSET_REGISTER)


/*
    ime_ds3_sai             :    [0x0, 0x3fffffff],         bits : 31_2
*/
#define IME_DATA_STAMP_SET3_DMA_ADDRESS_REGISTER_OFS 0x031c
REGDEF_BEGIN(IME_DATA_STAMP_SET3_DMA_ADDRESS_REGISTER)
REGDEF_BIT(,        2)
REGDEF_BIT(ime_ds3_sai,        30)
REGDEF_END(IME_DATA_STAMP_SET3_DMA_ADDRESS_REGISTER)


/*
    ime_ds_cst_coef0:    [0x0, 0xff],           bits : 7_0
    ime_ds_cst_coef1:    [0x0, 0xff],           bits : 15_8
    ime_ds_cst_coef2:    [0x0, 0xff],           bits : 23_16
    ime_ds_cst_coef3:    [0x0, 0xff],           bits : 31_24
*/
#define IME_DATA_STAMP_COLOR_SPACE_TRANSFORM_REGISTER_OFS 0x0320
REGDEF_BEGIN(IME_DATA_STAMP_COLOR_SPACE_TRANSFORM_REGISTER)
REGDEF_BIT(ime_ds_cst_coef0,        8)
REGDEF_BIT(ime_ds_cst_coef1,        8)
REGDEF_BIT(ime_ds_cst_coef2,        8)
REGDEF_BIT(ime_ds_cst_coef3,        8)
REGDEF_END(IME_DATA_STAMP_COLOR_SPACE_TRANSFORM_REGISTER)


/*
    ime_ds_pltc_a0:    [0x0, 0xff],         bits : 7_0
    ime_ds_pltc_r0:    [0x0, 0xff],         bits : 15_8
    ime_ds_pltc_g0:    [0x0, 0xff],         bits : 23_16
    ime_ds_pltc_b0:    [0x0, 0xff],         bits : 31_24
*/
#define IME_DATA_STAMP_COLOR_PALETTE_REGISTER0_OFS 0x0324
REGDEF_BEGIN(IME_DATA_STAMP_COLOR_PALETTE_REGISTER0)
REGDEF_BIT(ime_ds_pltc_a0,        8)
REGDEF_BIT(ime_ds_pltc_r0,        8)
REGDEF_BIT(ime_ds_pltc_g0,        8)
REGDEF_BIT(ime_ds_pltc_b0,        8)
REGDEF_END(IME_DATA_STAMP_COLOR_PALETTE_REGISTER0)


/*
    ime_ds_pltc_a1:    [0x0, 0xff],         bits : 7_0
    ime_ds_pltc_r1:    [0x0, 0xff],         bits : 15_8
    ime_ds_pltc_g1:    [0x0, 0xff],         bits : 23_16
    ime_ds_pltc_b1:    [0x0, 0xff],         bits : 31_24
*/
#define IME_DATA_STAMP_COLOR_PALETTE_REGISTER1_OFS 0x0328
REGDEF_BEGIN(IME_DATA_STAMP_COLOR_PALETTE_REGISTER1)
REGDEF_BIT(ime_ds_pltc_a1,        8)
REGDEF_BIT(ime_ds_pltc_r1,        8)
REGDEF_BIT(ime_ds_pltc_g1,        8)
REGDEF_BIT(ime_ds_pltc_b1,        8)
REGDEF_END(IME_DATA_STAMP_COLOR_PALETTE_REGISTER1)


/*
    ime_ds_pltc_a2:    [0x0, 0xff],         bits : 7_0
    ime_ds_pltc_r2:    [0x0, 0xff],         bits : 15_8
    ime_ds_pltc_g2:    [0x0, 0xff],         bits : 23_16
    ime_ds_pltc_b2:    [0x0, 0xff],         bits : 31_24
*/
#define IME_DATA_STAMP_COLOR_PALETTE_REGISTER2_OFS 0x032c
REGDEF_BEGIN(IME_DATA_STAMP_COLOR_PALETTE_REGISTER2)
REGDEF_BIT(ime_ds_pltc_a2,        8)
REGDEF_BIT(ime_ds_pltc_r2,        8)
REGDEF_BIT(ime_ds_pltc_g2,        8)
REGDEF_BIT(ime_ds_pltc_b2,        8)
REGDEF_END(IME_DATA_STAMP_COLOR_PALETTE_REGISTER2)


/*
    ime_ds_pltc_a3:    [0x0, 0xff],         bits : 7_0
    ime_ds_pltc_r3:    [0x0, 0xff],         bits : 15_8
    ime_ds_pltc_g3:    [0x0, 0xff],         bits : 23_16
    ime_ds_pltc_b3:    [0x0, 0xff],         bits : 31_24
*/
#define IME_DATA_STAMP_COLOR_PALETTE_REGISTER3_OFS 0x0330
REGDEF_BEGIN(IME_DATA_STAMP_COLOR_PALETTE_REGISTER3)
REGDEF_BIT(ime_ds_pltc_a3,        8)
REGDEF_BIT(ime_ds_pltc_r3,        8)
REGDEF_BIT(ime_ds_pltc_g3,        8)
REGDEF_BIT(ime_ds_pltc_b3,        8)
REGDEF_END(IME_DATA_STAMP_COLOR_PALETTE_REGISTER3)


/*
    ime_ds_pltc_a4:    [0x0, 0xff],         bits : 7_0
    ime_ds_pltc_r4:    [0x0, 0xff],         bits : 15_8
    ime_ds_pltc_g4:    [0x0, 0xff],         bits : 23_16
    ime_ds_pltc_b4:    [0x0, 0xff],         bits : 31_24
*/
#define IME_DATA_STAMP_COLOR_PALETTE_REGISTER4_OFS 0x0334
REGDEF_BEGIN(IME_DATA_STAMP_COLOR_PALETTE_REGISTER4)
REGDEF_BIT(ime_ds_pltc_a4,        8)
REGDEF_BIT(ime_ds_pltc_r4,        8)
REGDEF_BIT(ime_ds_pltc_g4,        8)
REGDEF_BIT(ime_ds_pltc_b4,        8)
REGDEF_END(IME_DATA_STAMP_COLOR_PALETTE_REGISTER4)


/*
    ime_ds_pltc_a5:    [0x0, 0xff],         bits : 7_0
    ime_ds_pltc_r5:    [0x0, 0xff],         bits : 15_8
    ime_ds_pltc_g5:    [0x0, 0xff],         bits : 23_16
    ime_ds_pltc_b5:    [0x0, 0xff],         bits : 31_24
*/
#define IME_DATA_STAMP_COLOR_PALETTE_REGISTER5_OFS 0x0338
REGDEF_BEGIN(IME_DATA_STAMP_COLOR_PALETTE_REGISTER5)
REGDEF_BIT(ime_ds_pltc_a5,        8)
REGDEF_BIT(ime_ds_pltc_r5,        8)
REGDEF_BIT(ime_ds_pltc_g5,        8)
REGDEF_BIT(ime_ds_pltc_b5,        8)
REGDEF_END(IME_DATA_STAMP_COLOR_PALETTE_REGISTER5)


/*
    ime_ds_pltc_a6:    [0x0, 0xff],         bits : 7_0
    ime_ds_pltc_r6:    [0x0, 0xff],         bits : 15_8
    ime_ds_pltc_g6:    [0x0, 0xff],         bits : 23_16
    ime_ds_pltc_b6:    [0x0, 0xff],         bits : 31_24
*/
#define IME_DATA_STAMP_COLOR_PALETTE_REGISTER6_OFS 0x033c
REGDEF_BEGIN(IME_DATA_STAMP_COLOR_PALETTE_REGISTER6)
REGDEF_BIT(ime_ds_pltc_a6,        8)
REGDEF_BIT(ime_ds_pltc_r6,        8)
REGDEF_BIT(ime_ds_pltc_g6,        8)
REGDEF_BIT(ime_ds_pltc_b6,        8)
REGDEF_END(IME_DATA_STAMP_COLOR_PALETTE_REGISTER6)


/*
    ime_ds_pltc_a7:    [0x0, 0xff],         bits : 7_0
    ime_ds_pltc_r7:    [0x0, 0xff],         bits : 15_8
    ime_ds_pltc_g7:    [0x0, 0xff],         bits : 23_16
    ime_ds_pltc_b7:    [0x0, 0xff],         bits : 31_24
*/
#define IME_DATA_STAMP_COLOR_PALETTE_REGISTER7_OFS 0x0340
REGDEF_BEGIN(IME_DATA_STAMP_COLOR_PALETTE_REGISTER7)
REGDEF_BIT(ime_ds_pltc_a7,        8)
REGDEF_BIT(ime_ds_pltc_r7,        8)
REGDEF_BIT(ime_ds_pltc_g7,        8)
REGDEF_BIT(ime_ds_pltc_b7,        8)
REGDEF_END(IME_DATA_STAMP_COLOR_PALETTE_REGISTER7)


/*
    ime_ds_pltc_a8:    [0x0, 0xff],         bits : 7_0
    ime_ds_pltc_r8:    [0x0, 0xff],         bits : 15_8
    ime_ds_pltc_g8:    [0x0, 0xff],         bits : 23_16
    ime_ds_pltc_b8:    [0x0, 0xff],         bits : 31_24
*/
#define IME_DATA_STAMP_COLOR_PALETTE_REGISTER8_OFS 0x0344
REGDEF_BEGIN(IME_DATA_STAMP_COLOR_PALETTE_REGISTER8)
REGDEF_BIT(ime_ds_pltc_a8,        8)
REGDEF_BIT(ime_ds_pltc_r8,        8)
REGDEF_BIT(ime_ds_pltc_g8,        8)
REGDEF_BIT(ime_ds_pltc_b8,        8)
REGDEF_END(IME_DATA_STAMP_COLOR_PALETTE_REGISTER8)


/*
    ime_ds_pltc_a9:    [0x0, 0xff],         bits : 7_0
    ime_ds_pltc_r9:    [0x0, 0xff],         bits : 15_8
    ime_ds_pltc_g9:    [0x0, 0xff],         bits : 23_16
    ime_ds_pltc_b9:    [0x0, 0xff],         bits : 31_24
*/
#define IME_DATA_STAMP_COLOR_PALETTE_REGISTER9_OFS 0x0348
REGDEF_BEGIN(IME_DATA_STAMP_COLOR_PALETTE_REGISTER9)
REGDEF_BIT(ime_ds_pltc_a9,        8)
REGDEF_BIT(ime_ds_pltc_r9,        8)
REGDEF_BIT(ime_ds_pltc_g9,        8)
REGDEF_BIT(ime_ds_pltc_b9,        8)
REGDEF_END(IME_DATA_STAMP_COLOR_PALETTE_REGISTER9)


/*
    ime_ds_pltc_a10:    [0x0, 0xff],            bits : 7_0
    ime_ds_pltc_r10:    [0x0, 0xff],            bits : 15_8
    ime_ds_pltc_g10:    [0x0, 0xff],            bits : 23_16
    ime_ds_pltc_b10:    [0x0, 0xff],            bits : 31_24
*/
#define IME_DATA_STAMP_COLOR_PALETTE_REGISTER10_OFS 0x034c
REGDEF_BEGIN(IME_DATA_STAMP_COLOR_PALETTE_REGISTER10)
REGDEF_BIT(ime_ds_pltc_a10,        8)
REGDEF_BIT(ime_ds_pltc_r10,        8)
REGDEF_BIT(ime_ds_pltc_g10,        8)
REGDEF_BIT(ime_ds_pltc_b10,        8)
REGDEF_END(IME_DATA_STAMP_COLOR_PALETTE_REGISTER10)


/*
    ime_ds_pltc_a11:    [0x0, 0xff],            bits : 7_0
    ime_ds_pltc_r11:    [0x0, 0xff],            bits : 15_8
    ime_ds_pltc_g11:    [0x0, 0xff],            bits : 23_16
    ime_ds_pltc_b11:    [0x0, 0xff],            bits : 31_24
*/
#define IME_DATA_STAMP_COLOR_PALETTE_REGISTER11_OFS 0x0350
REGDEF_BEGIN(IME_DATA_STAMP_COLOR_PALETTE_REGISTER11)
REGDEF_BIT(ime_ds_pltc_a11,        8)
REGDEF_BIT(ime_ds_pltc_r11,        8)
REGDEF_BIT(ime_ds_pltc_g11,        8)
REGDEF_BIT(ime_ds_pltc_b11,        8)
REGDEF_END(IME_DATA_STAMP_COLOR_PALETTE_REGISTER11)


/*
    ime_ds_pltc_a12:    [0x0, 0xff],            bits : 7_0
    ime_ds_pltc_r12:    [0x0, 0xff],            bits : 15_8
    ime_ds_pltc_g12:    [0x0, 0xff],            bits : 23_16
    ime_ds_pltc_b12:    [0x0, 0xff],            bits : 31_24
*/
#define IME_DATA_STAMP_COLOR_PALETTE_REGISTER12_OFS 0x0354
REGDEF_BEGIN(IME_DATA_STAMP_COLOR_PALETTE_REGISTER12)
REGDEF_BIT(ime_ds_pltc_a12,        8)
REGDEF_BIT(ime_ds_pltc_r12,        8)
REGDEF_BIT(ime_ds_pltc_g12,        8)
REGDEF_BIT(ime_ds_pltc_b12,        8)
REGDEF_END(IME_DATA_STAMP_COLOR_PALETTE_REGISTER12)


/*
    ime_ds_pltc_a13:    [0x0, 0xff],            bits : 7_0
    ime_ds_pltc_r13:    [0x0, 0xff],            bits : 15_8
    ime_ds_pltc_g13:    [0x0, 0xff],            bits : 23_16
    ime_ds_pltc_b13:    [0x0, 0xff],            bits : 31_24
*/
#define IME_DATA_STAMP_COLOR_PALETTE_REGISTER13_OFS 0x0358
REGDEF_BEGIN(IME_DATA_STAMP_COLOR_PALETTE_REGISTER13)
REGDEF_BIT(ime_ds_pltc_a13,        8)
REGDEF_BIT(ime_ds_pltc_r13,        8)
REGDEF_BIT(ime_ds_pltc_g13,        8)
REGDEF_BIT(ime_ds_pltc_b13,        8)
REGDEF_END(IME_DATA_STAMP_COLOR_PALETTE_REGISTER13)


/*
    ime_ds_pltc_a14:    [0x0, 0xff],            bits : 7_0
    ime_ds_pltc_r14:    [0x0, 0xff],            bits : 15_8
    ime_ds_pltc_g14:    [0x0, 0xff],            bits : 23_16
    ime_ds_pltc_b14:    [0x0, 0xff],            bits : 31_24
*/
#define IME_DATA_STAMP_COLOR_PALETTE_REGISTER14_OFS 0x035c
REGDEF_BEGIN(IME_DATA_STAMP_COLOR_PALETTE_REGISTER14)
REGDEF_BIT(ime_ds_pltc_a14,        8)
REGDEF_BIT(ime_ds_pltc_r14,        8)
REGDEF_BIT(ime_ds_pltc_g14,        8)
REGDEF_BIT(ime_ds_pltc_b14,        8)
REGDEF_END(IME_DATA_STAMP_COLOR_PALETTE_REGISTER14)


/*
    ime_ds_pltc_a15:    [0x0, 0xff],            bits : 7_0
    ime_ds_pltc_r15:    [0x0, 0xff],            bits : 15_8
    ime_ds_pltc_g15:    [0x0, 0xff],            bits : 23_16
    ime_ds_pltc_b15:    [0x0, 0xff],            bits : 31_24
*/
#define IME_DATA_STAMP_COLOR_PALETTE_REGISTER15_OFS 0x0360
REGDEF_BEGIN(IME_DATA_STAMP_COLOR_PALETTE_REGISTER15)
REGDEF_BIT(ime_ds_pltc_a15,        8)
REGDEF_BIT(ime_ds_pltc_r15,        8)
REGDEF_BIT(ime_ds_pltc_g15,        8)
REGDEF_BIT(ime_ds_pltc_b15,        8)
REGDEF_END(IME_DATA_STAMP_COLOR_PALETTE_REGISTER15)


/*
    reserved                :    [0x0, 0xffffffff],         bits : 31_0
*/
#define IME_RESERVED_49_OFS 0x0364
REGDEF_BEGIN(IME_RESERVED_49)
REGDEF_BIT(reserved,        32)
REGDEF_END(IME_RESERVED_49)


/*
    reserved                :    [0x0, 0xffffffff],         bits : 31_0
*/
#define IME_RESERVED_50_OFS 0x0368
REGDEF_BEGIN(IME_RESERVED_50)
REGDEF_BIT(reserved,        32)
REGDEF_END(IME_RESERVED_50)


/*
    reserved                :    [0x0, 0xffffffff],         bits : 31_0
*/
#define IME_RESERVED_51_OFS 0x036c
REGDEF_BEGIN(IME_RESERVED_51)
REGDEF_BIT(reserved,        32)
REGDEF_END(IME_RESERVED_51)


/*
    ime_stl_row0                   :    [0x0, 0x3ff],           bits : 9_0
    ime_stl_row1                   :    [0x0, 0x3ff],           bits : 19_10
    ime_stl_emk0                   :    [0x0, 0x3],         bits : 21_20
    ime_stl_emk1                   :    [0x0, 0x3],         bits : 23_22
    ime_stl_emken0               :    [0x0, 0x1],           bits : 24
    ime_stl_emken1               :    [0x0, 0x1],           bits : 25
    ime_stl_out_sel             :    [0x0, 0x1],            bits : 26
*/
#define IME_OUTPUT_PATH4_STATISTICAL_INFORMATION_REGISTER0_OFS 0x0370
REGDEF_BEGIN(IME_OUTPUT_PATH4_STATISTICAL_INFORMATION_REGISTER0)
REGDEF_BIT(ime_stl_row0,        10)
REGDEF_BIT(ime_stl_row1,        10)
REGDEF_BIT(ime_stl_emk0,        2)
REGDEF_BIT(ime_stl_emk1,        2)
REGDEF_BIT(ime_stl_emken0,        1)
REGDEF_BIT(ime_stl_emken1,        1)
REGDEF_BIT(ime_stl_out_sel,        1)
REGDEF_END(IME_OUTPUT_PATH4_STATISTICAL_INFORMATION_REGISTER0)


/*
    ime_stl_posh0               :    [0x0, 0xffff],         bits : 15_0
    ime_stl_posv0               :    [0x0, 0xffff],         bits : 31_16
*/
#define IME_OUTPUT_PATH4_STATISTICAL_POSITION_REGISTER0_OFS 0x0374
REGDEF_BEGIN(IME_OUTPUT_PATH4_STATISTICAL_POSITION_REGISTER0)
REGDEF_BIT(ime_stl_posh0,        16)
REGDEF_BIT(ime_stl_posv0,        16)
REGDEF_END(IME_OUTPUT_PATH4_STATISTICAL_POSITION_REGISTER0)


/*
    ime_stl_sizeh0              :    [0x0, 0x3ff],          bits : 9_0
    ime_stl_sizev0              :    [0x0, 0x3ff],          bits : 19_10
    ime_stl_msft0                :    [0x0, 0xf],           bits : 23_20
    ime_stl_msft1                :    [0x0, 0xf],           bits : 27_24
    ime_stl_sizeh0_msb:    [0x0, 0x1],          bits : 28
    ime_stl_sizev0_msb:    [0x0, 0x1],          bits : 30
*/
#define IME_OUTPUT_PATH4_STATISTICAL_SIZE_REGISTER0_OFS 0x0378
REGDEF_BEGIN(IME_OUTPUT_PATH4_STATISTICAL_SIZE_REGISTER0)
REGDEF_BIT(ime_stl_sizeh0,        10)
REGDEF_BIT(ime_stl_sizev0,        10)
REGDEF_BIT(ime_stl_msft0,        4)
REGDEF_BIT(ime_stl_msft1,        4)
REGDEF_BIT(ime_stl_sizeh0_msb,        1)
REGDEF_BIT(,        1)
REGDEF_BIT(ime_stl_sizev0_msb,        1)
REGDEF_END(IME_OUTPUT_PATH4_STATISTICAL_SIZE_REGISTER0)


/*
    ime_roi0_th0                :    [0x0, 0x3ff],          bits : 9_0
    ime_roi0_th1                :    [0x0, 0x3ff],          bits : 19_10
    ime_roi0_th2                :    [0x0, 0x3ff],          bits : 29_20
*/
#define IME_OUTPUT_PATH4_STATISTICAL_ROI0_THRESHOLD_REGISTER0_OFS 0x037c
REGDEF_BEGIN(IME_OUTPUT_PATH4_STATISTICAL_ROI0_THRESHOLD_REGISTER0)
REGDEF_BIT(ime_roi0_th0,        10)
REGDEF_BIT(ime_roi0_th1,        10)
REGDEF_BIT(ime_roi0_th2,        10)
REGDEF_END(IME_OUTPUT_PATH4_STATISTICAL_ROI0_THRESHOLD_REGISTER0)


/*
    ime_roi1_th0                :    [0x0, 0x3ff],          bits : 9_0
    ime_roi1_th1                :    [0x0, 0x3ff],          bits : 19_10
    ime_roi1_th2                :    [0x0, 0x3ff],          bits : 29_20
*/
#define IME_OUTPUT_PATH4_STATISTICAL_ROI1_THRESHOLD_REGISTER0_OFS 0x0380
REGDEF_BEGIN(IME_OUTPUT_PATH4_STATISTICAL_ROI1_THRESHOLD_REGISTER0)
REGDEF_BIT(ime_roi1_th0,        10)
REGDEF_BIT(ime_roi1_th1,        10)
REGDEF_BIT(ime_roi1_th2,        10)
REGDEF_END(IME_OUTPUT_PATH4_STATISTICAL_ROI1_THRESHOLD_REGISTER0)


/*
    ime_roi2_th0                :    [0x0, 0x3ff],          bits : 9_0
    ime_roi2_th1                :    [0x0, 0x3ff],          bits : 19_10
    ime_roi2_th2                :    [0x0, 0x3ff],          bits : 29_20
*/
#define IME_OUTPUT_PATH4_STATISTICAL_ROI2_THRESHOLD_REGISTER0_OFS 0x0384
REGDEF_BEGIN(IME_OUTPUT_PATH4_STATISTICAL_ROI2_THRESHOLD_REGISTER0)
REGDEF_BIT(ime_roi2_th0,        10)
REGDEF_BIT(ime_roi2_th1,        10)
REGDEF_BIT(ime_roi2_th2,        10)
REGDEF_END(IME_OUTPUT_PATH4_STATISTICAL_ROI2_THRESHOLD_REGISTER0)


/*
    ime_roi3_th0                :    [0x0, 0x3ff],          bits : 9_0
    ime_roi3_th1                :    [0x0, 0x3ff],          bits : 19_10
    ime_roi3_th2                :    [0x0, 0x3ff],          bits : 29_20
*/
#define IME_OUTPUT_PATH4_STATISTICAL_ROI3_THRESHOLD_REGISTER0_OFS 0x0388
REGDEF_BEGIN(IME_OUTPUT_PATH4_STATISTICAL_ROI3_THRESHOLD_REGISTER0)
REGDEF_BIT(ime_roi3_th0,        10)
REGDEF_BIT(ime_roi3_th1,        10)
REGDEF_BIT(ime_roi3_th2,        10)
REGDEF_END(IME_OUTPUT_PATH4_STATISTICAL_ROI3_THRESHOLD_REGISTER0)


/*
    ime_roi4_th0                :    [0x0, 0x3ff],          bits : 9_0
    ime_roi4_th1                :    [0x0, 0x3ff],          bits : 19_10
    ime_roi4_th2                :    [0x0, 0x3ff],          bits : 29_20
*/
#define IME_OUTPUT_PATH4_STATISTICAL_ROI4_THRESHOLD_REGISTER0_OFS 0x038c
REGDEF_BEGIN(IME_OUTPUT_PATH4_STATISTICAL_ROI4_THRESHOLD_REGISTER0)
REGDEF_BIT(ime_roi4_th0,        10)
REGDEF_BIT(ime_roi4_th1,        10)
REGDEF_BIT(ime_roi4_th2,        10)
REGDEF_END(IME_OUTPUT_PATH4_STATISTICAL_ROI4_THRESHOLD_REGISTER0)


/*
    ime_stl_posh1               :    [0x0, 0xffff],         bits : 15_0
    ime_stl_posv1               :    [0x0, 0xffff],         bits : 31_16
*/
#define IME_OUTPUT_PATH4_STATISTICAL_POSITION_REGISTER1_OFS 0x0390
REGDEF_BEGIN(IME_OUTPUT_PATH4_STATISTICAL_POSITION_REGISTER1)
REGDEF_BIT(ime_stl_posh1,        16)
REGDEF_BIT(ime_stl_posv1,        16)
REGDEF_END(IME_OUTPUT_PATH4_STATISTICAL_POSITION_REGISTER1)


/*
    ime_stl_sizeh1              :    [0x0, 0x3ff],          bits : 9_0
    ime_stl_sizev1              :    [0x0, 0x3ff],          bits : 19_10
    ime_stl_sizeh1_msb:    [0x0, 0x1],          bits : 28
    ime_stl_sizev1_msb:    [0x0, 0x1],          bits : 30
*/
#define IME_OUTPUT_PATH4_STATISTICAL_SIZE_REGISTER1_OFS 0x0394
REGDEF_BEGIN(IME_OUTPUT_PATH4_STATISTICAL_SIZE_REGISTER1)
REGDEF_BIT(ime_stl_sizeh1,        10)
REGDEF_BIT(ime_stl_sizev1,        10)
REGDEF_BIT(,        8)
REGDEF_BIT(ime_stl_sizeh1_msb,        1)
REGDEF_BIT(,        1)
REGDEF_BIT(ime_stl_sizev1_msb,        1)
REGDEF_END(IME_OUTPUT_PATH4_STATISTICAL_SIZE_REGISTER1)


/*
    ime_roi5_th0                :    [0x0, 0x3ff],          bits : 9_0
    ime_roi5_th1                :    [0x0, 0x3ff],          bits : 19_10
    ime_roi5_th2                :    [0x0, 0x3ff],          bits : 29_20
*/
#define IME_OUTPUT_PATH4_STATISTICAL_ROI5_THRESHOLD_REGISTER0_OFS 0x0398
REGDEF_BEGIN(IME_OUTPUT_PATH4_STATISTICAL_ROI5_THRESHOLD_REGISTER0)
REGDEF_BIT(ime_roi5_th0,        10)
REGDEF_BIT(ime_roi5_th1,        10)
REGDEF_BIT(ime_roi5_th2,        10)
REGDEF_END(IME_OUTPUT_PATH4_STATISTICAL_ROI5_THRESHOLD_REGISTER0)


/*
    ime_roi6_th0                :    [0x0, 0x3ff],          bits : 9_0
    ime_roi6_th1                :    [0x0, 0x3ff],          bits : 19_10
    ime_roi6_th2                :    [0x0, 0x3ff],          bits : 29_20
*/
#define IME_OUTPUT_PATH4_STATISTICAL_ROI6_THRESHOLD_REGISTER0_OFS 0x039c
REGDEF_BEGIN(IME_OUTPUT_PATH4_STATISTICAL_ROI6_THRESHOLD_REGISTER0)
REGDEF_BIT(ime_roi6_th0,        10)
REGDEF_BIT(ime_roi6_th1,        10)
REGDEF_BIT(ime_roi6_th2,        10)
REGDEF_END(IME_OUTPUT_PATH4_STATISTICAL_ROI6_THRESHOLD_REGISTER0)


/*
    ime_roi7_th0                :    [0x0, 0x3ff],          bits : 9_0
    ime_roi7_th1                :    [0x0, 0x3ff],          bits : 19_10
    ime_roi7_th2                :    [0x0, 0x3ff],          bits : 29_20
*/
#define IME_OUTPUT_PATH4_STATISTICAL_ROI7_THRESHOLD_REGISTER0_OFS 0x03a0
REGDEF_BEGIN(IME_OUTPUT_PATH4_STATISTICAL_ROI7_THRESHOLD_REGISTER0)
REGDEF_BIT(ime_roi7_th0,        10)
REGDEF_BIT(ime_roi7_th1,        10)
REGDEF_BIT(ime_roi7_th2,        10)
REGDEF_END(IME_OUTPUT_PATH4_STATISTICAL_ROI7_THRESHOLD_REGISTER0)


/*
    ime_roi0_msb_th0                :    [0x0, 0x1],            bits : 0
    ime_roi0_msb_th1                :    [0x0, 0x1],            bits : 1
    ime_roi0_msb_th2                :    [0x0, 0x1],            bits : 2
    ime_roi1_msb_th0                :    [0x0, 0x1],            bits : 3
    ime_roi1_msb_th1                :    [0x0, 0x1],            bits : 4
    ime_roi1_msb_th2                :    [0x0, 0x1],            bits : 5
    ime_roi2_msb_th0                :    [0x0, 0x1],            bits : 6
    ime_roi2_msb_th1                :    [0x0, 0x1],            bits : 7
    ime_roi2_msb_th2                :    [0x0, 0x1],            bits : 8
    ime_roi3_msb_th0                :    [0x0, 0x1],            bits : 9
    ime_roi3_msb_th1                :    [0x0, 0x1],            bits : 10
    ime_roi3_msb_th2                :    [0x0, 0x1],            bits : 11
    ime_roi4_msb_th0                :    [0x0, 0x1],            bits : 12
    ime_roi4_msb_th1                :    [0x0, 0x1],            bits : 13
    ime_roi4_msb_th2                :    [0x0, 0x1],            bits : 14
    ime_roi5_msb_th0                :    [0x0, 0x1],            bits : 15
    ime_roi5_msb_th1                :    [0x0, 0x1],            bits : 16
    ime_roi5_msb_th2                :    [0x0, 0x1],            bits : 17
    ime_roi6_msb_th0                :    [0x0, 0x1],            bits : 18
    ime_roi6_msb_th1                :    [0x0, 0x1],            bits : 19
    ime_roi6_msb_th2                :    [0x0, 0x1],            bits : 20
    ime_roi7_msb_th0                :    [0x0, 0x1],            bits : 21
    ime_roi7_msb_th1                :    [0x0, 0x1],            bits : 22
    ime_roi7_msb_th2                :    [0x0, 0x1],            bits : 23
    ime_roi0_src                    :    [0x0, 0x1],            bits : 24
    ime_roi1_src                    :    [0x0, 0x1],            bits : 25
    ime_roi2_src                    :    [0x0, 0x1],            bits : 26
    ime_roi3_src                    :    [0x0, 0x1],            bits : 27
    ime_roi4_src                    :    [0x0, 0x1],            bits : 28
    ime_roi5_src                    :    [0x0, 0x1],            bits : 29
    ime_roi6_src                    :    [0x0, 0x1],            bits : 30
    ime_roi7_src                    :    [0x0, 0x1],            bits : 31
*/
#define IME_OUTPUT_PATH4_STATISTICAL_ROI_THRESHOLD_MSB_REGISTER0_OFS 0x03a4
REGDEF_BEGIN(IME_OUTPUT_PATH4_STATISTICAL_ROI_THRESHOLD_MSB_REGISTER0)
REGDEF_BIT(ime_roi0_msb_th0,        1)
REGDEF_BIT(ime_roi0_msb_th1,        1)
REGDEF_BIT(ime_roi0_msb_th2,        1)
REGDEF_BIT(ime_roi1_msb_th0,        1)
REGDEF_BIT(ime_roi1_msb_th1,        1)
REGDEF_BIT(ime_roi1_msb_th2,        1)
REGDEF_BIT(ime_roi2_msb_th0,        1)
REGDEF_BIT(ime_roi2_msb_th1,        1)
REGDEF_BIT(ime_roi2_msb_th2,        1)
REGDEF_BIT(ime_roi3_msb_th0,        1)
REGDEF_BIT(ime_roi3_msb_th1,        1)
REGDEF_BIT(ime_roi3_msb_th2,        1)
REGDEF_BIT(ime_roi4_msb_th0,        1)
REGDEF_BIT(ime_roi4_msb_th1,        1)
REGDEF_BIT(ime_roi4_msb_th2,        1)
REGDEF_BIT(ime_roi5_msb_th0,        1)
REGDEF_BIT(ime_roi5_msb_th1,        1)
REGDEF_BIT(ime_roi5_msb_th2,        1)
REGDEF_BIT(ime_roi6_msb_th0,        1)
REGDEF_BIT(ime_roi6_msb_th1,        1)
REGDEF_BIT(ime_roi6_msb_th2,        1)
REGDEF_BIT(ime_roi7_msb_th0,        1)
REGDEF_BIT(ime_roi7_msb_th1,        1)
REGDEF_BIT(ime_roi7_msb_th2,        1)
REGDEF_BIT(ime_roi0_src,        1)
REGDEF_BIT(ime_roi1_src,        1)
REGDEF_BIT(ime_roi2_src,        1)
REGDEF_BIT(ime_roi3_src,        1)
REGDEF_BIT(ime_roi4_src,        1)
REGDEF_BIT(ime_roi5_src,        1)
REGDEF_BIT(ime_roi6_src,        1)
REGDEF_BIT(ime_roi7_src,        1)
REGDEF_END(IME_OUTPUT_PATH4_STATISTICAL_ROI_THRESHOLD_MSB_REGISTER0)


/*
    ime_emax0               :    [0x0, 0x7ff],          bits : 10_0
    ime_emax1               :    [0x0, 0x7ff],          bits : 22_12
*/
#define IME_OUTPUT_PATH4_MAX_INFORMATION_REGISTER0_OFS 0x03a8
REGDEF_BEGIN(IME_OUTPUT_PATH4_MAX_INFORMATION_REGISTER0)
REGDEF_BIT(ime_emax0,        11)
REGDEF_BIT(,        1)
REGDEF_BIT(ime_emax1,        11)
REGDEF_END(IME_OUTPUT_PATH4_MAX_INFORMATION_REGISTER0)


/*
    ime_stl_acct0                 :    [0x0, 0xfffff],          bits : 19_0
    ime_stl_hitbin0             :    [0x0, 0x7f],           bits : 30_24
*/
#define IME_OUTPUT_PATH4_HISTOGRAM_ACCUMULATION_TARGET_REGISTER0_OFS 0x03ac
REGDEF_BEGIN(IME_OUTPUT_PATH4_HISTOGRAM_ACCUMULATION_TARGET_REGISTER0)
REGDEF_BIT(ime_stl_acct0,        20)
REGDEF_BIT(,        4)
REGDEF_BIT(ime_stl_hitbin0,        7)
REGDEF_END(IME_OUTPUT_PATH4_HISTOGRAM_ACCUMULATION_TARGET_REGISTER0)


/*
    ime_stl_acct1                 :    [0x0, 0xfffff],          bits : 19_0
    ime_stl_hitbin1             :    [0x0, 0x7f],           bits : 30_24
*/
#define IME_OUTPUT_PATH4_HISTOGRAM_ACCUMULATION_TARGET_REGISTER1_OFS 0x03b0
REGDEF_BEGIN(IME_OUTPUT_PATH4_HISTOGRAM_ACCUMULATION_TARGET_REGISTER1)
REGDEF_BIT(ime_stl_acct1,        20)
REGDEF_BIT(,        4)
REGDEF_BIT(ime_stl_hitbin1,        7)
REGDEF_END(IME_OUTPUT_PATH4_HISTOGRAM_ACCUMULATION_TARGET_REGISTER1)


/*
    ime_em_dram_sao_p4              :    [0x0, 0x1fffffff],         bits : 30_2
*/
#define IME_OUTPUT_PATH4_EDGE_MAP_OUTPUT_DMA_ADDRESS_REGISTER0_OFS 0x03b4
REGDEF_BEGIN(IME_OUTPUT_PATH4_EDGE_MAP_OUTPUT_DMA_ADDRESS_REGISTER0)
REGDEF_BIT(,        2)
REGDEF_BIT(ime_em_dram_sao_p4,        29)
REGDEF_END(IME_OUTPUT_PATH4_EDGE_MAP_OUTPUT_DMA_ADDRESS_REGISTER0)


/*
    ime_hist_dram_sao               :    [0x0, 0x1fffffff],         bits : 30_2
*/
#define IME_OUTPUT_PATH4_EDGE_HISTOGRAM_OUTPUT_DRAM_ADDRESS_REGISTER0_OFS 0x03b8
REGDEF_BEGIN(IME_OUTPUT_PATH4_EDGE_HISTOGRAM_OUTPUT_DRAM_ADDRESS_REGISTER0)
REGDEF_BIT(,        2)
REGDEF_BIT(ime_hist_dram_sao,        29)
REGDEF_END(IME_OUTPUT_PATH4_EDGE_HISTOGRAM_OUTPUT_DRAM_ADDRESS_REGISTER0)


/*
    ime_em_ofso_p4              :    [0x0, 0x3ffff],            bits : 19_2
*/
#define IME_OUTPUT_PATH4_EDGE_MAP_OUTPUT_LINEOFFSET_REGISTER0_OFS 0x03bc
REGDEF_BEGIN(IME_OUTPUT_PATH4_EDGE_MAP_OUTPUT_LINEOFFSET_REGISTER0)
REGDEF_BIT(,        2)
REGDEF_BIT(ime_em_ofso_p4,        18)
REGDEF_END(IME_OUTPUT_PATH4_EDGE_MAP_OUTPUT_LINEOFFSET_REGISTER0)


/*
    ime_sprt_hbl_p1             :    [0x0, 0xffff],         bits : 15_0
*/
#define IME_OUTPUT_PATH1_CONTROL_REGISTER11_OFS 0x03c0
REGDEF_BEGIN(IME_OUTPUT_PATH1_CONTROL_REGISTER11)
REGDEF_BIT(ime_sprt_hbl_p1,        16)
REGDEF_END(IME_OUTPUT_PATH1_CONTROL_REGISTER11)


/*
    ime_p1_y1_dram_ofso:    [0x0, 0x3ffff],         bits : 19_2
*/
#define IME_OUTPUT_PATH1_DMA_LINEOFFSET_REGISTER2_OFS 0x03c4
REGDEF_BEGIN(IME_OUTPUT_PATH1_DMA_LINEOFFSET_REGISTER2)
REGDEF_BIT(,        2)
REGDEF_BIT(ime_p1_y1_dram_ofso,        18)
REGDEF_END(IME_OUTPUT_PATH1_DMA_LINEOFFSET_REGISTER2)


/*
    ime_p1_uv1_dram_ofso:    [0x0, 0x3ffff],            bits : 19_2
*/
#define IME_OUTPUT_PATH1_DMA_LINEOFFSET_REGISTER3_OFS 0x03c8
REGDEF_BEGIN(IME_OUTPUT_PATH1_DMA_LINEOFFSET_REGISTER3)
REGDEF_BIT(,        2)
REGDEF_BIT(ime_p1_uv1_dram_ofso,        18)
REGDEF_END(IME_OUTPUT_PATH1_DMA_LINEOFFSET_REGISTER3)


/*
    ime_p1_y1_dram_sao:    [0x0, 0x7fffffff],           bits : 30_0
*/
#define IME_OUTPUT_PATH1_DMA_BUFFER1_REGISTER3_OFS 0x03cc
REGDEF_BEGIN(IME_OUTPUT_PATH1_DMA_BUFFER1_REGISTER3)
REGDEF_BIT(ime_p1_y1_dram_sao,        31)
REGDEF_END(IME_OUTPUT_PATH1_DMA_BUFFER1_REGISTER3)


/*
    ime_p1_uv1_dram_sao:    [0x0, 0x7fffffff],          bits : 30_0
*/
#define IME_OUTPUT_PATH1_DMA_BUFFER1_REGISTER4_OFS 0x03d0
REGDEF_BEGIN(IME_OUTPUT_PATH1_DMA_BUFFER1_REGISTER4)
REGDEF_BIT(ime_p1_uv1_dram_sao,        31)
REGDEF_END(IME_OUTPUT_PATH1_DMA_BUFFER1_REGISTER4)


/*
    ime_sprt_hbl_p2             :    [0x0, 0xffff],         bits : 15_0
*/
#define IME_OUTPUT_PATH2_CONTROL_REGISTER30_OFS 0x03d4
REGDEF_BEGIN(IME_OUTPUT_PATH2_CONTROL_REGISTER30)
REGDEF_BIT(ime_sprt_hbl_p2,        16)
REGDEF_END(IME_OUTPUT_PATH2_CONTROL_REGISTER30)


/*
    ime_p2_y1_dram_ofso:    [0x0, 0x3ffff],         bits : 19_2
*/
#define IME_OUTPUT_PATH2_DMA_LINEOFFSET_REGISTER2_OFS 0x03d8
REGDEF_BEGIN(IME_OUTPUT_PATH2_DMA_LINEOFFSET_REGISTER2)
REGDEF_BIT(,        2)
REGDEF_BIT(ime_p2_y1_dram_ofso,        18)
REGDEF_END(IME_OUTPUT_PATH2_DMA_LINEOFFSET_REGISTER2)


/*
    ime_p2_uv1_dram_ofso:    [0x0, 0x3ffff],            bits : 19_2
*/
#define IME_OUTPUT_PATH2_DMA_LINEOFFSET_REGISTER3_OFS 0x03dc
REGDEF_BEGIN(IME_OUTPUT_PATH2_DMA_LINEOFFSET_REGISTER3)
REGDEF_BIT(,        2)
REGDEF_BIT(ime_p2_uv1_dram_ofso,        18)
REGDEF_END(IME_OUTPUT_PATH2_DMA_LINEOFFSET_REGISTER3)


/*
    ime_p2_y1_dram_sao:    [0x0, 0x7fffffff],           bits : 30_0
*/
#define IME_OUTPUT_PATH2_DMA_BUFFER2_REGISTER3_OFS 0x03e0
REGDEF_BEGIN(IME_OUTPUT_PATH2_DMA_BUFFER2_REGISTER3)
REGDEF_BIT(ime_p2_y1_dram_sao,        31)
REGDEF_END(IME_OUTPUT_PATH2_DMA_BUFFER2_REGISTER3)


/*
    ime_p2_uv1_dram_sao:    [0x0, 0x7fffffff],          bits : 30_0
*/
#define IME_OUTPUT_PATH2_DMA_BUFFER1_REGISTER4_OFS 0x03e4
REGDEF_BEGIN(IME_OUTPUT_PATH2_DMA_BUFFER1_REGISTER4)
REGDEF_BIT(ime_p2_uv1_dram_sao,        31)
REGDEF_END(IME_OUTPUT_PATH2_DMA_BUFFER1_REGISTER4)


/*
    ime_sprt_hbl_p3             :    [0x0, 0xffff],         bits : 15_0
*/
#define IME_OUTPUT_PATH3_CONTROL_REGISTER30_OFS 0x03e8
REGDEF_BEGIN(IME_OUTPUT_PATH3_CONTROL_REGISTER30)
REGDEF_BIT(ime_sprt_hbl_p3,        16)
REGDEF_END(IME_OUTPUT_PATH3_CONTROL_REGISTER30)


/*
    ime_p3_y1_dram_ofso:    [0x0, 0x3ffff],         bits : 19_2
*/
#define IME_OUTPUT_PATH3_DMA_LINEOFFSET_REGISTER2_OFS 0x03ec
REGDEF_BEGIN(IME_OUTPUT_PATH3_DMA_LINEOFFSET_REGISTER2)
REGDEF_BIT(,        2)
REGDEF_BIT(ime_p3_y1_dram_ofso,        18)
REGDEF_END(IME_OUTPUT_PATH3_DMA_LINEOFFSET_REGISTER2)


/*
    ime_p3_uv1_dram_ofso:    [0x0, 0x3ffff],            bits : 19_2
*/
#define IME_OUTPUT_PATH3_DMA_LINEOFFSET_REGISTER3_OFS 0x03f0
REGDEF_BEGIN(IME_OUTPUT_PATH3_DMA_LINEOFFSET_REGISTER3)
REGDEF_BIT(,        2)
REGDEF_BIT(ime_p3_uv1_dram_ofso,        18)
REGDEF_END(IME_OUTPUT_PATH3_DMA_LINEOFFSET_REGISTER3)


/*
    ime_p3_y1_dram_sao:    [0x0, 0x7fffffff],           bits : 30_0
*/
#define IME_OUTPUT_PATH3_DMA_BUFFER2_REGISTER3_OFS 0x03f4
REGDEF_BEGIN(IME_OUTPUT_PATH3_DMA_BUFFER2_REGISTER3)
REGDEF_BIT(ime_p3_y1_dram_sao,        31)
REGDEF_END(IME_OUTPUT_PATH3_DMA_BUFFER2_REGISTER3)


/*
    ime_p3_uv1_dram_sao:    [0x0, 0x7fffffff],          bits : 30_0
*/
#define IME_OUTPUT_PATH3_DMA_BUFFER1_REGISTER4_OFS 0x03f8
REGDEF_BEGIN(IME_OUTPUT_PATH3_DMA_BUFFER1_REGISTER4)
REGDEF_BIT(ime_p3_uv1_dram_sao,        31)
REGDEF_END(IME_OUTPUT_PATH3_DMA_BUFFER1_REGISTER4)


/*
    reserved                :    [0x0, 0xffffffff],         bits : 31_0
*/
#define IME_RESERVED_72_OFS 0x03fc
REGDEF_BEGIN(IME_RESERVED_72)
REGDEF_BIT(reserved,        32)
REGDEF_END(IME_RESERVED_72)


/*
    reserved                :    [0x0, 0xffffffff],         bits : 31_0
*/
#define IME_RESERVED_73_OFS 0x0400
REGDEF_BEGIN(IME_RESERVED_73)
REGDEF_BIT(reserved,        32)
REGDEF_END(IME_RESERVED_73)


/*
    reserved                :    [0x0, 0xffffffff],         bits : 31_0
*/
#define IME_RESERVED_74_OFS 0x0404
REGDEF_BEGIN(IME_RESERVED_74)
REGDEF_BIT(reserved,        32)
REGDEF_END(IME_RESERVED_74)


/*
    reserved                :    [0x0, 0xffffffff],         bits : 31_0
*/
#define IME_RESERVED_75_OFS 0x0408
REGDEF_BEGIN(IME_RESERVED_75)
REGDEF_BIT(reserved,        32)
REGDEF_END(IME_RESERVED_75)


/*
    reserved                :    [0x0, 0xffffffff],         bits : 31_0
*/
#define IME_RESERVED_76_OFS 0x040c
REGDEF_BEGIN(IME_RESERVED_76)
REGDEF_BIT(reserved,        32)
REGDEF_END(IME_RESERVED_76)


/*
    reserved                :    [0x0, 0xffffffff],         bits : 31_0
*/
#define IME_RESERVED_77_OFS 0x0410
REGDEF_BEGIN(IME_RESERVED_77)
REGDEF_BIT(reserved,        32)
REGDEF_END(IME_RESERVED_77)


/*
    reserved                :    [0x0, 0xffffffff],         bits : 31_0
*/
#define IME_RESERVED_78_OFS 0x0414
REGDEF_BEGIN(IME_RESERVED_78)
REGDEF_BIT(reserved,        32)
REGDEF_END(IME_RESERVED_78)


/*
    reserved                :    [0x0, 0xffffffff],         bits : 31_0
*/
#define IME_RESERVED_79_OFS 0x0418
REGDEF_BEGIN(IME_RESERVED_79)
REGDEF_BIT(reserved,        32)
REGDEF_END(IME_RESERVED_79)


/*
    reserved                :    [0x0, 0xffffffff],         bits : 31_0
*/
#define IME_RESERVED_80_OFS 0x041c
REGDEF_BEGIN(IME_RESERVED_80)
REGDEF_BIT(reserved,        32)
REGDEF_END(IME_RESERVED_80)


/*
    ime_pm0_type                   :    [0x0, 0x1],         bits : 0
    ime_pm_pxlsize               :    [0x0, 0x3],           bits : 2_1
    ime_pm0_color_y             :    [0x0, 0xff],           bits : 11_4
    ime_pm0_color_u             :    [0x0, 0xff],           bits : 19_12
    ime_pm0_color_v             :    [0x0, 0xff],           bits : 27_20
    ime_pm0_hlw_en     :    [0x0, 0x1],         bits : 31
*/
#define IME_PRIVACY_MASK_SET0_REGISTER0_OFS 0x0420
REGDEF_BEGIN(IME_PRIVACY_MASK_SET0_REGISTER0)
REGDEF_BIT(ime_pm0_type,        1)
REGDEF_BIT(ime_pm_pxlsize,        2)
REGDEF_BIT(,        1)
REGDEF_BIT(ime_pm0_color_y,        8)
REGDEF_BIT(ime_pm0_color_u,        8)
REGDEF_BIT(ime_pm0_color_v,        8)
REGDEF_BIT(,        3)
REGDEF_BIT(ime_pm0_hlw_en,        1)
REGDEF_END(IME_PRIVACY_MASK_SET0_REGISTER0)


/*
    ime_pm0_line0_comp               :    [0x0, 0x3],           bits : 1_0
    ime_pm0_line0_coefa             :    [0x0, 0x1fff],         bits : 16_4
    ime_pm0_line0_signa             :    [0x0, 0x1],            bits : 17
    ime_pm0_line0_coefb             :    [0x0, 0x1fff],         bits : 30_18
    ime_pm0_line0_signb             :    [0x0, 0x1],            bits : 31
*/
#define IME_PRIVACY_MASK_SET0_REGISTER1_OFS 0x0424
REGDEF_BEGIN(IME_PRIVACY_MASK_SET0_REGISTER1)
REGDEF_BIT(ime_pm0_line0_comp,        2)
REGDEF_BIT(,        2)
REGDEF_BIT(ime_pm0_line0_coefa,        13)
REGDEF_BIT(ime_pm0_line0_signa,        1)
REGDEF_BIT(ime_pm0_line0_coefb,        13)
REGDEF_BIT(ime_pm0_line0_signb,        1)
REGDEF_END(IME_PRIVACY_MASK_SET0_REGISTER1)


/*
    ime_pm0_line0_coefc             :    [0x0, 0x3ffffff],          bits : 25_0
    ime_pm0_line0_signc             :    [0x0, 0x1],            bits : 26
*/
#define IME_PRIVACY_MASK_SET0_REGISTER2_OFS 0x0428
REGDEF_BEGIN(IME_PRIVACY_MASK_SET0_REGISTER2)
REGDEF_BIT(ime_pm0_line0_coefc,        26)
REGDEF_BIT(ime_pm0_line0_signc,        1)
REGDEF_END(IME_PRIVACY_MASK_SET0_REGISTER2)


/*
    ime_pm0_line1_comp               :    [0x0, 0x3],           bits : 1_0
    ime_pm0_line1_coefa             :    [0x0, 0x1fff],         bits : 16_4
    ime_pm0_line1_signa             :    [0x0, 0x1],            bits : 17
    ime_pm0_line1_coefb             :    [0x0, 0x1fff],         bits : 30_18
    ime_pm0_line1_signb             :    [0x0, 0x1],            bits : 31
*/
#define IME_PRIVACY_MASK_SET0_REGISTER3_OFS 0x042c
REGDEF_BEGIN(IME_PRIVACY_MASK_SET0_REGISTER3)
REGDEF_BIT(ime_pm0_line1_comp,        2)
REGDEF_BIT(,        2)
REGDEF_BIT(ime_pm0_line1_coefa,        13)
REGDEF_BIT(ime_pm0_line1_signa,        1)
REGDEF_BIT(ime_pm0_line1_coefb,        13)
REGDEF_BIT(ime_pm0_line1_signb,        1)
REGDEF_END(IME_PRIVACY_MASK_SET0_REGISTER3)


/*
    ime_pm0_line1_coefc             :    [0x0, 0x3ffffff],          bits : 25_0
    ime_pm0_line1_signc             :    [0x0, 0x1],            bits : 26
*/
#define IME_PRIVACY_MASK_SET0_REGISTER4_OFS 0x0430
REGDEF_BEGIN(IME_PRIVACY_MASK_SET0_REGISTER4)
REGDEF_BIT(ime_pm0_line1_coefc,        26)
REGDEF_BIT(ime_pm0_line1_signc,        1)
REGDEF_END(IME_PRIVACY_MASK_SET0_REGISTER4)


/*
    ime_pm0_line2_comp               :    [0x0, 0x3],           bits : 1_0
    ime_pm0_line2_coefa             :    [0x0, 0x1fff],         bits : 16_4
    ime_pm0_line2_signa             :    [0x0, 0x1],            bits : 17
    ime_pm0_line2_coefb             :    [0x0, 0x1fff],         bits : 30_18
    ime_pm0_line2_signb             :    [0x0, 0x1],            bits : 31
*/
#define IME_PRIVACY_MASK_SET0_REGISTER5_OFS 0x0434
REGDEF_BEGIN(IME_PRIVACY_MASK_SET0_REGISTER5)
REGDEF_BIT(ime_pm0_line2_comp,        2)
REGDEF_BIT(,        2)
REGDEF_BIT(ime_pm0_line2_coefa,        13)
REGDEF_BIT(ime_pm0_line2_signa,        1)
REGDEF_BIT(ime_pm0_line2_coefb,        13)
REGDEF_BIT(ime_pm0_line2_signb,        1)
REGDEF_END(IME_PRIVACY_MASK_SET0_REGISTER5)


/*
    ime_pm0_line2_coefc             :    [0x0, 0x3ffffff],          bits : 25_0
    ime_pm0_line2_signc             :    [0x0, 0x1],            bits : 26
*/
#define IME_PRIVACY_MASK_SET0_REGISTER6_OFS 0x0438
REGDEF_BEGIN(IME_PRIVACY_MASK_SET0_REGISTER6)
REGDEF_BIT(ime_pm0_line2_coefc,        26)
REGDEF_BIT(ime_pm0_line2_signc,        1)
REGDEF_END(IME_PRIVACY_MASK_SET0_REGISTER6)


/*
    ime_pm0_line3_comp               :    [0x0, 0x3],           bits : 1_0
    ime_pm0_line3_coefa             :    [0x0, 0x1fff],         bits : 16_4
    ime_pm0_line3_signa             :    [0x0, 0x1],            bits : 17
    ime_pm0_line3_coefb             :    [0x0, 0x1fff],         bits : 30_18
    ime_pm0_line3_signb             :    [0x0, 0x1],            bits : 31
*/
#define IME_PRIVACY_MASK_SET0_REGISTER7_OFS 0x043c
REGDEF_BEGIN(IME_PRIVACY_MASK_SET0_REGISTER7)
REGDEF_BIT(ime_pm0_line3_comp,        2)
REGDEF_BIT(,        2)
REGDEF_BIT(ime_pm0_line3_coefa,        13)
REGDEF_BIT(ime_pm0_line3_signa,        1)
REGDEF_BIT(ime_pm0_line3_coefb,        13)
REGDEF_BIT(ime_pm0_line3_signb,        1)
REGDEF_END(IME_PRIVACY_MASK_SET0_REGISTER7)


/*
    ime_pm0_line3_coefc             :    [0x0, 0x3ffffff],          bits : 25_0
    ime_pm0_line3_signc             :    [0x0, 0x1],            bits : 26
*/
#define IME_PRIVACY_MASK_SET0_REGISTER8_OFS 0x0440
REGDEF_BEGIN(IME_PRIVACY_MASK_SET0_REGISTER8)
REGDEF_BIT(ime_pm0_line3_coefc,        26)
REGDEF_BIT(ime_pm0_line3_signc,        1)
REGDEF_END(IME_PRIVACY_MASK_SET0_REGISTER8)


/*
    ime_pm1_type                   :    [0x0, 0x1],         bits : 0
    ime_pm1_color_y             :    [0x0, 0xff],           bits : 11_4
    ime_pm1_color_u             :    [0x0, 0xff],           bits : 19_12
    ime_pm1_color_v             :    [0x0, 0xff],           bits : 27_20
*/
#define IME_PRIVACY_MASK_SET1_REGISTER0_OFS 0x0444
REGDEF_BEGIN(IME_PRIVACY_MASK_SET1_REGISTER0)
REGDEF_BIT(ime_pm1_type,        1)
REGDEF_BIT(,        3)
REGDEF_BIT(ime_pm1_color_y,        8)
REGDEF_BIT(ime_pm1_color_u,        8)
REGDEF_BIT(ime_pm1_color_v,        8)
REGDEF_END(IME_PRIVACY_MASK_SET1_REGISTER0)


/*
    ime_pm1_line0_comp               :    [0x0, 0x3],           bits : 1_0
    ime_pm1_line0_coefa             :    [0x0, 0x1fff],         bits : 16_4
    ime_pm1_line0_signa             :    [0x0, 0x1],            bits : 17
    ime_pm1_line0_coefb             :    [0x0, 0x1fff],         bits : 30_18
    ime_pm1_line0_signb             :    [0x0, 0x1],            bits : 31
*/
#define IME_PRIVACY_MASK_SET1_REGISTER1_OFS 0x0448
REGDEF_BEGIN(IME_PRIVACY_MASK_SET1_REGISTER1)
REGDEF_BIT(ime_pm1_line0_comp,        2)
REGDEF_BIT(,        2)
REGDEF_BIT(ime_pm1_line0_coefa,        13)
REGDEF_BIT(ime_pm1_line0_signa,        1)
REGDEF_BIT(ime_pm1_line0_coefb,        13)
REGDEF_BIT(ime_pm1_line0_signb,        1)
REGDEF_END(IME_PRIVACY_MASK_SET1_REGISTER1)


/*
    ime_pm1_line0_coefc             :    [0x0, 0x3ffffff],          bits : 25_0
    ime_pm1_line0_signc             :    [0x0, 0x1],            bits : 26
*/
#define IME_PRIVACY_MASK_SET1_REGISTER2_OFS 0x044c
REGDEF_BEGIN(IME_PRIVACY_MASK_SET1_REGISTER2)
REGDEF_BIT(ime_pm1_line0_coefc,        26)
REGDEF_BIT(ime_pm1_line0_signc,        1)
REGDEF_END(IME_PRIVACY_MASK_SET1_REGISTER2)


/*
    ime_pm1_line1_comp               :    [0x0, 0x3],           bits : 1_0
    ime_pm1_line1_coefa             :    [0x0, 0x1fff],         bits : 16_4
    ime_pm1_line1_signa             :    [0x0, 0x1],            bits : 17
    ime_pm1_line1_coefb             :    [0x0, 0x1fff],         bits : 30_18
    ime_pm1_line1_signb             :    [0x0, 0x1],            bits : 31
*/
#define IME_PRIVACY_MASK_SET1_REGISTER3_OFS 0x0450
REGDEF_BEGIN(IME_PRIVACY_MASK_SET1_REGISTER3)
REGDEF_BIT(ime_pm1_line1_comp,        2)
REGDEF_BIT(,        2)
REGDEF_BIT(ime_pm1_line1_coefa,        13)
REGDEF_BIT(ime_pm1_line1_signa,        1)
REGDEF_BIT(ime_pm1_line1_coefb,        13)
REGDEF_BIT(ime_pm1_line1_signb,        1)
REGDEF_END(IME_PRIVACY_MASK_SET1_REGISTER3)


/*
    ime_pm1_line1_coefc             :    [0x0, 0x3ffffff],          bits : 25_0
    ime_pm1_line1_signc             :    [0x0, 0x1],            bits : 26
*/
#define IME_PRIVACY_MASK_SET1_REGISTER4_OFS 0x0454
REGDEF_BEGIN(IME_PRIVACY_MASK_SET1_REGISTER4)
REGDEF_BIT(ime_pm1_line1_coefc,        26)
REGDEF_BIT(ime_pm1_line1_signc,        1)
REGDEF_END(IME_PRIVACY_MASK_SET1_REGISTER4)


/*
    ime_pm1_line2_comp     :    [0x0, 0x3],         bits : 1_0
    ime_pm1_line2_coefa             :    [0x0, 0x1fff],         bits : 16_4
    ime_pm1_line2_signa             :    [0x0, 0x1],            bits : 17
    ime_pm1_line2_coefb             :    [0x0, 0x1fff],         bits : 30_18
    ime_pm1_line2_signb             :    [0x0, 0x1],            bits : 31
*/
#define IME_PRIVACY_MASK_SET1_REGISTER5_OFS 0x0458
REGDEF_BEGIN(IME_PRIVACY_MASK_SET1_REGISTER5)
REGDEF_BIT(ime_pm1_line2_comp,        2)
REGDEF_BIT(,        2)
REGDEF_BIT(ime_pm1_line2_coefa,        13)
REGDEF_BIT(ime_pm1_line2_signa,        1)
REGDEF_BIT(ime_pm1_line2_coefb,        13)
REGDEF_BIT(ime_pm1_line2_signb,        1)
REGDEF_END(IME_PRIVACY_MASK_SET1_REGISTER5)


/*
    ime_pm1_line2_coefc             :    [0x0, 0x3ffffff],          bits : 25_0
    ime_pm1_line2_signc             :    [0x0, 0x1],            bits : 26
*/
#define IME_PRIVACY_MASK_SET1_REGISTER6_OFS 0x045c
REGDEF_BEGIN(IME_PRIVACY_MASK_SET1_REGISTER6)
REGDEF_BIT(ime_pm1_line2_coefc,        26)
REGDEF_BIT(ime_pm1_line2_signc,        1)
REGDEF_END(IME_PRIVACY_MASK_SET1_REGISTER6)


/*
    ime_pm1_line3_comp               :    [0x0, 0x3],           bits : 1_0
    ime_pm1_line3_coefa             :    [0x0, 0x1fff],         bits : 16_4
    ime_pm1_line3_signa             :    [0x0, 0x1],            bits : 17
    ime_pm1_line3_coefb             :    [0x0, 0x1fff],         bits : 30_18
    ime_pm1_line3_signb             :    [0x0, 0x1],            bits : 31
*/
#define IME_PRIVACY_MASK_SET1_REGISTER7_OFS 0x0460
REGDEF_BEGIN(IME_PRIVACY_MASK_SET1_REGISTER7)
REGDEF_BIT(ime_pm1_line3_comp,        2)
REGDEF_BIT(,        2)
REGDEF_BIT(ime_pm1_line3_coefa,        13)
REGDEF_BIT(ime_pm1_line3_signa,        1)
REGDEF_BIT(ime_pm1_line3_coefb,        13)
REGDEF_BIT(ime_pm1_line3_signb,        1)
REGDEF_END(IME_PRIVACY_MASK_SET1_REGISTER7)


/*
    ime_pm1_line3_coefc             :    [0x0, 0x3ffffff],          bits : 25_0
    ime_pm1_line3_signc             :    [0x0, 0x1],            bits : 26
*/
#define IME_PRIVACY_MASK_SET1_REGISTER8_OFS 0x0464
REGDEF_BEGIN(IME_PRIVACY_MASK_SET1_REGISTER8)
REGDEF_BIT(ime_pm1_line3_coefc,        26)
REGDEF_BIT(ime_pm1_line3_signc,        1)
REGDEF_END(IME_PRIVACY_MASK_SET1_REGISTER8)


/*
    ime_pm2_type                   :    [0x0, 0x1],         bits : 0
    ime_pm2_color_y             :    [0x0, 0xff],           bits : 11_4
    ime_pm2_color_u             :    [0x0, 0xff],           bits : 19_12
    ime_pm2_color_v             :    [0x0, 0xff],           bits : 27_20
    ime_pm2_hlw_en     :    [0x0, 0x1],         bits : 31
*/
#define IME_PRIVACY_MASK_SET2_REGISTER0_OFS 0x0468
REGDEF_BEGIN(IME_PRIVACY_MASK_SET2_REGISTER0)
REGDEF_BIT(ime_pm2_type,        1)
REGDEF_BIT(,        3)
REGDEF_BIT(ime_pm2_color_y,        8)
REGDEF_BIT(ime_pm2_color_u,        8)
REGDEF_BIT(ime_pm2_color_v,        8)
REGDEF_BIT(,        3)
REGDEF_BIT(ime_pm2_hlw_en,        1)
REGDEF_END(IME_PRIVACY_MASK_SET2_REGISTER0)


/*
    ime_pm2_line0_comp               :    [0x0, 0x3],           bits : 1_0
    ime_pm2_line0_coefa             :    [0x0, 0x1fff],         bits : 16_4
    ime_pm2_line0_signa             :    [0x0, 0x1],            bits : 17
    ime_pm2_line0_coefb             :    [0x0, 0x1fff],         bits : 30_18
    ime_pm2_line0_signb             :    [0x0, 0x1],            bits : 31
*/
#define IME_PRIVACY_MASK_SET2_REGISTER1_OFS 0x046c
REGDEF_BEGIN(IME_PRIVACY_MASK_SET2_REGISTER1)
REGDEF_BIT(ime_pm2_line0_comp,        2)
REGDEF_BIT(,        2)
REGDEF_BIT(ime_pm2_line0_coefa,        13)
REGDEF_BIT(ime_pm2_line0_signa,        1)
REGDEF_BIT(ime_pm2_line0_coefb,        13)
REGDEF_BIT(ime_pm2_line0_signb,        1)
REGDEF_END(IME_PRIVACY_MASK_SET2_REGISTER1)


/*
    ime_pm2_line0_coefc             :    [0x0, 0x3ffffff],          bits : 25_0
    ime_pm2_line0_signc             :    [0x0, 0x1],            bits : 26
*/
#define IME_PRIVACY_MASK_SET2_REGISTER2_OFS 0x0470
REGDEF_BEGIN(IME_PRIVACY_MASK_SET2_REGISTER2)
REGDEF_BIT(ime_pm2_line0_coefc,        26)
REGDEF_BIT(ime_pm2_line0_signc,        1)
REGDEF_END(IME_PRIVACY_MASK_SET2_REGISTER2)


/*
    ime_pm2_line1_comp               :    [0x0, 0x3],           bits : 1_0
    ime_pm2_line1_coefa             :    [0x0, 0x1fff],         bits : 16_4
    ime_pm2_line1_signa             :    [0x0, 0x1],            bits : 17
    ime_pm2_line1_coefb             :    [0x0, 0x1fff],         bits : 30_18
    ime_pm2_line1_signb             :    [0x0, 0x1],            bits : 31
*/
#define IME_PRIVACY_MASK_SET2_REGISTER3_OFS 0x0474
REGDEF_BEGIN(IME_PRIVACY_MASK_SET2_REGISTER3)
REGDEF_BIT(ime_pm2_line1_comp,        2)
REGDEF_BIT(,        2)
REGDEF_BIT(ime_pm2_line1_coefa,        13)
REGDEF_BIT(ime_pm2_line1_signa,        1)
REGDEF_BIT(ime_pm2_line1_coefb,        13)
REGDEF_BIT(ime_pm2_line1_signb,        1)
REGDEF_END(IME_PRIVACY_MASK_SET2_REGISTER3)


/*
    ime_pm2_line1_coefc             :    [0x0, 0x3ffffff],          bits : 25_0
    ime_pm2_line1_signc             :    [0x0, 0x1],            bits : 26
*/
#define IME_PRIVACY_MASK_SET2_REGISTER4_OFS 0x0478
REGDEF_BEGIN(IME_PRIVACY_MASK_SET2_REGISTER4)
REGDEF_BIT(ime_pm2_line1_coefc,        26)
REGDEF_BIT(ime_pm2_line1_signc,        1)
REGDEF_END(IME_PRIVACY_MASK_SET2_REGISTER4)


/*
    ime_pm2_line2_comp               :    [0x0, 0x3],           bits : 1_0
    ime_pm2_line2_coefa             :    [0x0, 0x1fff],         bits : 16_4
    ime_pm2_line2_signa             :    [0x0, 0x1],            bits : 17
    ime_pm2_line2_coefb             :    [0x0, 0x1fff],         bits : 30_18
    ime_pm2_line2_signb             :    [0x0, 0x1],            bits : 31
*/
#define IME_PRIVACY_MASK_SET2_REGISTER5_OFS 0x047c
REGDEF_BEGIN(IME_PRIVACY_MASK_SET2_REGISTER5)
REGDEF_BIT(ime_pm2_line2_comp,        2)
REGDEF_BIT(,        2)
REGDEF_BIT(ime_pm2_line2_coefa,        13)
REGDEF_BIT(ime_pm2_line2_signa,        1)
REGDEF_BIT(ime_pm2_line2_coefb,        13)
REGDEF_BIT(ime_pm2_line2_signb,        1)
REGDEF_END(IME_PRIVACY_MASK_SET2_REGISTER5)


/*
    ime_pm2_line2_coefc             :    [0x0, 0x3ffffff],          bits : 25_0
    ime_pm2_line2_signc             :    [0x0, 0x1],            bits : 26
*/
#define IME_PRIVACY_MASK_SET2_REGISTER6_OFS 0x0480
REGDEF_BEGIN(IME_PRIVACY_MASK_SET2_REGISTER6)
REGDEF_BIT(ime_pm2_line2_coefc,        26)
REGDEF_BIT(ime_pm2_line2_signc,        1)
REGDEF_END(IME_PRIVACY_MASK_SET2_REGISTER6)


/*
    ime_pm2_line3_comp               :    [0x0, 0x3],           bits : 1_0
    ime_pm2_line3_coefa             :    [0x0, 0x1fff],         bits : 16_4
    ime_pm2_line3_signa             :    [0x0, 0x1],            bits : 17
    ime_pm2_line3_coefb             :    [0x0, 0x1fff],         bits : 30_18
    ime_pm2_line3_signb             :    [0x0, 0x1],            bits : 31
*/
#define IME_PRIVACY_MASK_SET2_REGISTER7_OFS 0x0484
REGDEF_BEGIN(IME_PRIVACY_MASK_SET2_REGISTER7)
REGDEF_BIT(ime_pm2_line3_comp,        2)
REGDEF_BIT(,        2)
REGDEF_BIT(ime_pm2_line3_coefa,        13)
REGDEF_BIT(ime_pm2_line3_signa,        1)
REGDEF_BIT(ime_pm2_line3_coefb,        13)
REGDEF_BIT(ime_pm2_line3_signb,        1)
REGDEF_END(IME_PRIVACY_MASK_SET2_REGISTER7)


/*
    ime_pm2_line3_coefc             :    [0x0, 0x3ffffff],          bits : 25_0
    ime_pm2_line3_signc             :    [0x0, 0x1],            bits : 26
*/
#define IME_PRIVACY_MASK_SET2_REGISTER8_OFS 0x0488
REGDEF_BEGIN(IME_PRIVACY_MASK_SET2_REGISTER8)
REGDEF_BIT(ime_pm2_line3_coefc,        26)
REGDEF_BIT(ime_pm2_line3_signc,        1)
REGDEF_END(IME_PRIVACY_MASK_SET2_REGISTER8)


/*
    ime_pm3_type                   :    [0x0, 0x1],         bits : 0
    ime_pm3_color_y             :    [0x0, 0xff],           bits : 11_4
    ime_pm3_color_u             :    [0x0, 0xff],           bits : 19_12
    ime_pm3_color_v             :    [0x0, 0xff],           bits : 27_20
*/
#define IME_PRIVACY_MASK_SET3_REGISTER0_OFS 0x048c
REGDEF_BEGIN(IME_PRIVACY_MASK_SET3_REGISTER0)
REGDEF_BIT(ime_pm3_type,        1)
REGDEF_BIT(,        3)
REGDEF_BIT(ime_pm3_color_y,        8)
REGDEF_BIT(ime_pm3_color_u,        8)
REGDEF_BIT(ime_pm3_color_v,        8)
REGDEF_END(IME_PRIVACY_MASK_SET3_REGISTER0)


/*
    ime_pm3_line0_comp               :    [0x0, 0x3],           bits : 1_0
    ime_pm3_line0_coefa             :    [0x0, 0x1fff],         bits : 16_4
    ime_pm3_line0_signa             :    [0x0, 0x1],            bits : 17
    ime_pm3_line0_coefb             :    [0x0, 0x1fff],         bits : 30_18
    ime_pm3_line0_signb             :    [0x0, 0x1],            bits : 31
*/
#define IME_PRIVACY_MASK_SET3_REGISTER1_OFS 0x0490
REGDEF_BEGIN(IME_PRIVACY_MASK_SET3_REGISTER1)
REGDEF_BIT(ime_pm3_line0_comp,        2)
REGDEF_BIT(,        2)
REGDEF_BIT(ime_pm3_line0_coefa,        13)
REGDEF_BIT(ime_pm3_line0_signa,        1)
REGDEF_BIT(ime_pm3_line0_coefb,        13)
REGDEF_BIT(ime_pm3_line0_signb,        1)
REGDEF_END(IME_PRIVACY_MASK_SET3_REGISTER1)


/*
    ime_pm3_line0_coefc             :    [0x0, 0x3ffffff],          bits : 25_0
    ime_pm3_line0_signc             :    [0x0, 0x1],            bits : 26
*/
#define IME_PRIVACY_MASK_SET3_REGISTER2_OFS 0x0494
REGDEF_BEGIN(IME_PRIVACY_MASK_SET3_REGISTER2)
REGDEF_BIT(ime_pm3_line0_coefc,        26)
REGDEF_BIT(ime_pm3_line0_signc,        1)
REGDEF_END(IME_PRIVACY_MASK_SET3_REGISTER2)


/*
    ime_pm3_line1_comp               :    [0x0, 0x3],           bits : 1_0
    ime_pm3_line1_coefa             :    [0x0, 0x1fff],         bits : 16_4
    ime_pm3_line1_signa             :    [0x0, 0x1],            bits : 17
    ime_pm3_line1_coefb             :    [0x0, 0x1fff],         bits : 30_18
    ime_pm3_line1_signb             :    [0x0, 0x1],            bits : 31
*/
#define IME_PRIVACY_MASK_SET3_REGISTER3_OFS 0x0498
REGDEF_BEGIN(IME_PRIVACY_MASK_SET3_REGISTER3)
REGDEF_BIT(ime_pm3_line1_comp,        2)
REGDEF_BIT(,        2)
REGDEF_BIT(ime_pm3_line1_coefa,        13)
REGDEF_BIT(ime_pm3_line1_signa,        1)
REGDEF_BIT(ime_pm3_line1_coefb,        13)
REGDEF_BIT(ime_pm3_line1_signb,        1)
REGDEF_END(IME_PRIVACY_MASK_SET3_REGISTER3)


/*
    ime_pm3_line1_coefc             :    [0x0, 0x3ffffff],          bits : 25_0
    ime_pm3_line1_signc             :    [0x0, 0x1],            bits : 26
*/
#define IME_PRIVACY_MASK_SET3_REGISTER4_OFS 0x049c
REGDEF_BEGIN(IME_PRIVACY_MASK_SET3_REGISTER4)
REGDEF_BIT(ime_pm3_line1_coefc,        26)
REGDEF_BIT(ime_pm3_line1_signc,        1)
REGDEF_END(IME_PRIVACY_MASK_SET3_REGISTER4)


/*
    ime_pm3_line2_comp                  :    [0x0, 0x3],            bits : 1_0
    ime_pm3_line2_coefa                         :    [0x0, 0x1fff],         bits : 16_4
    ime_pm3_line2_signa                :    [0x0, 0x1],         bits : 17
    ime_pm3_line2_coefb                :    [0x0, 0x1fff],          bits : 30_18
    ime_pm3_line2_signb                :    [0x0, 0x1],         bits : 31
*/
#define IME_PRIVACY_MASK_SET3_REGISTER5_OFS 0x04a0
REGDEF_BEGIN(IME_PRIVACY_MASK_SET3_REGISTER5)
REGDEF_BIT(ime_pm3_line2_comp,        2)
REGDEF_BIT(,        2)
REGDEF_BIT(ime_pm3_line2_coefa,        13)
REGDEF_BIT(ime_pm3_line2_signa,        1)
REGDEF_BIT(ime_pm3_line2_coefb,        13)
REGDEF_BIT(ime_pm3_line2_signb,        1)
REGDEF_END(IME_PRIVACY_MASK_SET3_REGISTER5)


/*
    ime_pm3_line2_coefc             :    [0x0, 0x3ffffff],          bits : 25_0
    ime_pm3_line2_signc             :    [0x0, 0x1],            bits : 26
*/
#define IME_PRIVACY_MASK_SET3_REGISTER6_OFS 0x04a4
REGDEF_BEGIN(IME_PRIVACY_MASK_SET3_REGISTER6)
REGDEF_BIT(ime_pm3_line2_coefc,        26)
REGDEF_BIT(ime_pm3_line2_signc,        1)
REGDEF_END(IME_PRIVACY_MASK_SET3_REGISTER6)


/*
    ime_pm3_line3_comp               :    [0x0, 0x3],           bits : 1_0
    ime_pm3_line3_coefa             :    [0x0, 0x1fff],         bits : 16_4
    ime_pm3_line3_signa             :    [0x0, 0x1],            bits : 17
    ime_pm3_line3_coefb             :    [0x0, 0x1fff],         bits : 30_18
    ime_pm3_line3_signb             :    [0x0, 0x1],            bits : 31
*/
#define IME_PRIVACY_MASK_SET3_REGISTER7_OFS 0x04a8
REGDEF_BEGIN(IME_PRIVACY_MASK_SET3_REGISTER7)
REGDEF_BIT(ime_pm3_line3_comp,        2)
REGDEF_BIT(,        2)
REGDEF_BIT(ime_pm3_line3_coefa,        13)
REGDEF_BIT(ime_pm3_line3_signa,        1)
REGDEF_BIT(ime_pm3_line3_coefb,        13)
REGDEF_BIT(ime_pm3_line3_signb,        1)
REGDEF_END(IME_PRIVACY_MASK_SET3_REGISTER7)


/*
    ime_pm3_line3_coefc             :    [0x0, 0x3ffffff],          bits : 25_0
    ime_pm3_line3_signc             :    [0x0, 0x1],            bits : 26
*/
#define IME_PRIVACY_MASK_SET3_REGISTER8_OFS 0x04ac
REGDEF_BEGIN(IME_PRIVACY_MASK_SET3_REGISTER8)
REGDEF_BIT(ime_pm3_line3_coefc,        26)
REGDEF_BIT(ime_pm3_line3_signc,        1)
REGDEF_END(IME_PRIVACY_MASK_SET3_REGISTER8)


/*
    ime_pm0_awet                :    [0x0, 0xff],           bits : 7_0
    ime_pm1_awet                :    [0x0, 0xff],           bits : 15_8
    ime_pm2_awet                :    [0x0, 0xff],           bits : 23_16
    ime_pm3_awet                :    [0x0, 0xff],           bits : 31_24
*/
#define IME_PRIVACY_MASK_ALPHA_REGISTER0_OFS 0x04b0
REGDEF_BEGIN(IME_PRIVACY_MASK_ALPHA_REGISTER0)
REGDEF_BIT(ime_pm0_awet,        8)
REGDEF_BIT(ime_pm1_awet,        8)
REGDEF_BIT(ime_pm2_awet,        8)
REGDEF_BIT(ime_pm3_awet,        8)
REGDEF_END(IME_PRIVACY_MASK_ALPHA_REGISTER0)


/*
    ime_pm4_type                   :    [0x0, 0x1],         bits : 0
    ime_pm4_color_y             :    [0x0, 0xff],           bits : 11_4
    ime_pm4_color_u             :    [0x0, 0xff],           bits : 19_12
    ime_pm4_color_v             :    [0x0, 0xff],           bits : 27_20
    ime_pm4_hlw_en     :    [0x0, 0x1],         bits : 31
*/
#define IME_PRIVACY_MASK_SET4_REGISTER0_OFS 0x04b4
REGDEF_BEGIN(IME_PRIVACY_MASK_SET4_REGISTER0)
REGDEF_BIT(ime_pm4_type,        1)
REGDEF_BIT(,        3)
REGDEF_BIT(ime_pm4_color_y,        8)
REGDEF_BIT(ime_pm4_color_u,        8)
REGDEF_BIT(ime_pm4_color_v,        8)
REGDEF_BIT(,        3)
REGDEF_BIT(ime_pm4_hlw_en,        1)
REGDEF_END(IME_PRIVACY_MASK_SET4_REGISTER0)


/*
    ime_pm4_line0_comp               :    [0x0, 0x3],           bits : 1_0
    ime_pm4_line0_coefa             :    [0x0, 0x1fff],         bits : 16_4
    ime_pm4_line0_signa             :    [0x0, 0x1],            bits : 17
    ime_pm4_line0_coefb             :    [0x0, 0x1fff],         bits : 30_18
    ime_pm4_line0_signb             :    [0x0, 0x1],            bits : 31
*/
#define IME_PRIVACY_MASK_SET4_REGISTER1_OFS 0x04b8
REGDEF_BEGIN(IME_PRIVACY_MASK_SET4_REGISTER1)
REGDEF_BIT(ime_pm4_line0_comp,        2)
REGDEF_BIT(,        2)
REGDEF_BIT(ime_pm4_line0_coefa,        13)
REGDEF_BIT(ime_pm4_line0_signa,        1)
REGDEF_BIT(ime_pm4_line0_coefb,        13)
REGDEF_BIT(ime_pm4_line0_signb,        1)
REGDEF_END(IME_PRIVACY_MASK_SET4_REGISTER1)


/*
    ime_pm4_line0_coefc             :    [0x0, 0x3ffffff],          bits : 25_0
    ime_pm4_line0_signc             :    [0x0, 0x1],            bits : 26
*/
#define IME_PRIVACY_MASK_SET4_REGISTER2_OFS 0x04bc
REGDEF_BEGIN(IME_PRIVACY_MASK_SET4_REGISTER2)
REGDEF_BIT(ime_pm4_line0_coefc,        26)
REGDEF_BIT(ime_pm4_line0_signc,        1)
REGDEF_END(IME_PRIVACY_MASK_SET4_REGISTER2)


/*
    ime_pm4_line1_comp               :    [0x0, 0x3],           bits : 1_0
    ime_pm4_line1_coefa             :    [0x0, 0x1fff],         bits : 16_4
    ime_pm4_line1_signa             :    [0x0, 0x1],            bits : 17
    ime_pm4_line1_coefb             :    [0x0, 0x1fff],         bits : 30_18
    ime_pm4_line1_signb             :    [0x0, 0x1],            bits : 31
*/
#define IME_PRIVACY_MASK_SET4_REGISTER3_OFS 0x04c0
REGDEF_BEGIN(IME_PRIVACY_MASK_SET4_REGISTER3)
REGDEF_BIT(ime_pm4_line1_comp,        2)
REGDEF_BIT(,        2)
REGDEF_BIT(ime_pm4_line1_coefa,        13)
REGDEF_BIT(ime_pm4_line1_signa,        1)
REGDEF_BIT(ime_pm4_line1_coefb,        13)
REGDEF_BIT(ime_pm4_line1_signb,        1)
REGDEF_END(IME_PRIVACY_MASK_SET4_REGISTER3)


/*
    ime_pm4_line1_coefc             :    [0x0, 0x3ffffff],          bits : 25_0
    ime_pm4_line1_signc             :    [0x0, 0x1],            bits : 26
*/
#define IME_PRIVACY_MASK_SET4_REGISTER4_OFS 0x04c4
REGDEF_BEGIN(IME_PRIVACY_MASK_SET4_REGISTER4)
REGDEF_BIT(ime_pm4_line1_coefc,        26)
REGDEF_BIT(ime_pm4_line1_signc,        1)
REGDEF_END(IME_PRIVACY_MASK_SET4_REGISTER4)


/*
    ime_pm4_line2_comp               :    [0x0, 0x3],           bits : 1_0
    ime_pm4_line2_coefa             :    [0x0, 0x1fff],         bits : 16_4
    ime_pm4_line2_signa             :    [0x0, 0x1],            bits : 17
    ime_pm4_line2_coefb             :    [0x0, 0x1fff],         bits : 30_18
    ime_pm4_line2_signb             :    [0x0, 0x1],            bits : 31
*/
#define IME_PRIVACY_MASK_SET4_REGISTER5_OFS 0x04c8
REGDEF_BEGIN(IME_PRIVACY_MASK_SET4_REGISTER5)
REGDEF_BIT(ime_pm4_line2_comp,        2)
REGDEF_BIT(,        2)
REGDEF_BIT(ime_pm4_line2_coefa,        13)
REGDEF_BIT(ime_pm4_line2_signa,        1)
REGDEF_BIT(ime_pm4_line2_coefb,        13)
REGDEF_BIT(ime_pm4_line2_signb,        1)
REGDEF_END(IME_PRIVACY_MASK_SET4_REGISTER5)


/*
    ime_pm4_line2_coefc             :    [0x0, 0x3ffffff],          bits : 25_0
    ime_pm4_line2_signc             :    [0x0, 0x1],            bits : 26
*/
#define IME_PRIVACY_MASK_SET4_REGISTER6_OFS 0x04cc
REGDEF_BEGIN(IME_PRIVACY_MASK_SET4_REGISTER6)
REGDEF_BIT(ime_pm4_line2_coefc,        26)
REGDEF_BIT(ime_pm4_line2_signc,        1)
REGDEF_END(IME_PRIVACY_MASK_SET4_REGISTER6)


/*
    ime_pm4_line3_comp               :    [0x0, 0x3],           bits : 1_0
    ime_pm4_line3_coefa             :    [0x0, 0x1fff],         bits : 16_4
    ime_pm4_line3_signa             :    [0x0, 0x1],            bits : 17
    ime_pm4_line3_coefb             :    [0x0, 0x1fff],         bits : 30_18
    ime_pm4_line3_signb             :    [0x0, 0x1],            bits : 31
*/
#define IME_PRIVACY_MASK_SET4_REGISTER7_OFS 0x04d0
REGDEF_BEGIN(IME_PRIVACY_MASK_SET4_REGISTER7)
REGDEF_BIT(ime_pm4_line3_comp,        2)
REGDEF_BIT(,        2)
REGDEF_BIT(ime_pm4_line3_coefa,        13)
REGDEF_BIT(ime_pm4_line3_signa,        1)
REGDEF_BIT(ime_pm4_line3_coefb,        13)
REGDEF_BIT(ime_pm4_line3_signb,        1)
REGDEF_END(IME_PRIVACY_MASK_SET4_REGISTER7)


/*
    ime_pm4_line3_coefc             :    [0x0, 0x3ffffff],          bits : 25_0
    ime_pm4_line3_signc             :    [0x0, 0x1],            bits : 26
*/
#define IME_PRIVACY_MASK_SET4_REGISTER8_OFS 0x04d4
REGDEF_BEGIN(IME_PRIVACY_MASK_SET4_REGISTER8)
REGDEF_BIT(ime_pm4_line3_coefc,        26)
REGDEF_BIT(ime_pm4_line3_signc,        1)
REGDEF_END(IME_PRIVACY_MASK_SET4_REGISTER8)


/*
    ime_pm5_type                   :    [0x0, 0x1],         bits : 0
    ime_pm5_color_y             :    [0x0, 0xff],           bits : 11_4
    ime_pm5_color_u             :    [0x0, 0xff],           bits : 19_12
    ime_pm5_color_v             :    [0x0, 0xff],           bits : 27_20
*/
#define IME_PRIVACY_MASK_SET5_REGISTER0_OFS 0x04d8
REGDEF_BEGIN(IME_PRIVACY_MASK_SET5_REGISTER0)
REGDEF_BIT(ime_pm5_type,        1)
REGDEF_BIT(,        3)
REGDEF_BIT(ime_pm5_color_y,        8)
REGDEF_BIT(ime_pm5_color_u,        8)
REGDEF_BIT(ime_pm5_color_v,        8)
REGDEF_END(IME_PRIVACY_MASK_SET5_REGISTER0)


/*
    ime_pm5_line0_comp               :    [0x0, 0x3],           bits : 1_0
    ime_pm5_line0_coefa             :    [0x0, 0x1fff],         bits : 16_4
    ime_pm5_line0_signa             :    [0x0, 0x1],            bits : 17
    ime_pm5_line0_coefb             :    [0x0, 0x1fff],         bits : 30_18
    ime_pm5_line0_signb             :    [0x0, 0x1],            bits : 31
*/
#define IME_PRIVACY_MASK_SET5_REGISTER1_OFS 0x04dc
REGDEF_BEGIN(IME_PRIVACY_MASK_SET5_REGISTER1)
REGDEF_BIT(ime_pm5_line0_comp,        2)
REGDEF_BIT(,        2)
REGDEF_BIT(ime_pm5_line0_coefa,        13)
REGDEF_BIT(ime_pm5_line0_signa,        1)
REGDEF_BIT(ime_pm5_line0_coefb,        13)
REGDEF_BIT(ime_pm5_line0_signb,        1)
REGDEF_END(IME_PRIVACY_MASK_SET5_REGISTER1)


/*
    ime_pm5_line0_coefc             :    [0x0, 0x3ffffff],          bits : 25_0
    ime_pm5_line0_signc             :    [0x0, 0x1],            bits : 26
*/
#define IME_PRIVACY_MASK_SET5_REGISTER2_OFS 0x04e0
REGDEF_BEGIN(IME_PRIVACY_MASK_SET5_REGISTER2)
REGDEF_BIT(ime_pm5_line0_coefc,        26)
REGDEF_BIT(ime_pm5_line0_signc,        1)
REGDEF_END(IME_PRIVACY_MASK_SET5_REGISTER2)


/*
    ime_pm5_line1_comp               :    [0x0, 0x3],           bits : 1_0
    ime_pm5_line1_coefa             :    [0x0, 0x1fff],         bits : 16_4
    ime_pm5_line1_signa             :    [0x0, 0x1],            bits : 17
    ime_pm5_line1_coefb             :    [0x0, 0x1fff],         bits : 30_18
    ime_pm5_line1_signb             :    [0x0, 0x1],            bits : 31
*/
#define IME_PRIVACY_MASK_SET5_REGISTER3_OFS 0x04e4
REGDEF_BEGIN(IME_PRIVACY_MASK_SET5_REGISTER3)
REGDEF_BIT(ime_pm5_line1_comp,        2)
REGDEF_BIT(,        2)
REGDEF_BIT(ime_pm5_line1_coefa,        13)
REGDEF_BIT(ime_pm5_line1_signa,        1)
REGDEF_BIT(ime_pm5_line1_coefb,        13)
REGDEF_BIT(ime_pm5_line1_signb,        1)
REGDEF_END(IME_PRIVACY_MASK_SET5_REGISTER3)


/*
    ime_pm5_line1_coefc             :    [0x0, 0x3ffffff],          bits : 25_0
    ime_pm5_line1_signc             :    [0x0, 0x1],            bits : 26
*/
#define IME_PRIVACY_MASK_SET5_REGISTER4_OFS 0x04e8
REGDEF_BEGIN(IME_PRIVACY_MASK_SET5_REGISTER4)
REGDEF_BIT(ime_pm5_line1_coefc,        26)
REGDEF_BIT(ime_pm5_line1_signc,        1)
REGDEF_END(IME_PRIVACY_MASK_SET5_REGISTER4)


/*
    ime_pm5_line2_comp               :    [0x0, 0x3],           bits : 1_0
    ime_pm5_line2_coefa             :    [0x0, 0x1fff],         bits : 16_4
    ime_pm5_line2_signa             :    [0x0, 0x1],            bits : 17
    ime_pm5_line2_coefb             :    [0x0, 0x1fff],         bits : 30_18
    ime_pm5_line2_signb             :    [0x0, 0x1],            bits : 31
*/
#define IME_PRIVACY_MASK_SET5_REGISTER5_OFS 0x04ec
REGDEF_BEGIN(IME_PRIVACY_MASK_SET5_REGISTER5)
REGDEF_BIT(ime_pm5_line2_comp,        2)
REGDEF_BIT(,        2)
REGDEF_BIT(ime_pm5_line2_coefa,        13)
REGDEF_BIT(ime_pm5_line2_signa,        1)
REGDEF_BIT(ime_pm5_line2_coefb,        13)
REGDEF_BIT(ime_pm5_line2_signb,        1)
REGDEF_END(IME_PRIVACY_MASK_SET5_REGISTER5)


/*
    ime_pm5_line2_coefc             :    [0x0, 0x3ffffff],          bits : 25_0
    ime_pm5_line2_signc             :    [0x0, 0x1],            bits : 26
*/
#define IME_PRIVACY_MASK_SET5_REGISTER6_OFS 0x04f0
REGDEF_BEGIN(IME_PRIVACY_MASK_SET5_REGISTER6)
REGDEF_BIT(ime_pm5_line2_coefc,        26)
REGDEF_BIT(ime_pm5_line2_signc,        1)
REGDEF_END(IME_PRIVACY_MASK_SET5_REGISTER6)


/*
    ime_pm5_line3_comp               :    [0x0, 0x3],           bits : 1_0
    ime_pm5_line3_coefa             :    [0x0, 0x1fff],         bits : 16_4
    ime_pm5_line3_signa             :    [0x0, 0x1],            bits : 17
    ime_pm5_line3_coefb             :    [0x0, 0x1fff],         bits : 30_18
    ime_pm5_line3_signb             :    [0x0, 0x1],            bits : 31
*/
#define IME_PRIVACY_MASK_SET5_REGISTER7_OFS 0x04f4
REGDEF_BEGIN(IME_PRIVACY_MASK_SET5_REGISTER7)
REGDEF_BIT(ime_pm5_line3_comp,        2)
REGDEF_BIT(,        2)
REGDEF_BIT(ime_pm5_line3_coefa,        13)
REGDEF_BIT(ime_pm5_line3_signa,        1)
REGDEF_BIT(ime_pm5_line3_coefb,        13)
REGDEF_BIT(ime_pm5_line3_signb,        1)
REGDEF_END(IME_PRIVACY_MASK_SET5_REGISTER7)


/*
    ime_pm5_line3_coefc             :    [0x0, 0x3ffffff],          bits : 25_0
    ime_pm5_line3_signc             :    [0x0, 0x1],            bits : 26
*/
#define IME_PRIVACY_MASK_SET5_REGISTER8_OFS 0x04f8
REGDEF_BEGIN(IME_PRIVACY_MASK_SET5_REGISTER8)
REGDEF_BIT(ime_pm5_line3_coefc,        26)
REGDEF_BIT(ime_pm5_line3_signc,        1)
REGDEF_END(IME_PRIVACY_MASK_SET5_REGISTER8)


/*
    ime_pm6_type                   :    [0x0, 0x1],         bits : 0
    ime_pm6_color_y             :    [0x0, 0xff],           bits : 11_4
    ime_pm6_color_u             :    [0x0, 0xff],           bits : 19_12
    ime_pm6_color_v             :    [0x0, 0xff],           bits : 27_20
    ime_pm6_hlw_en     :    [0x0, 0x1],         bits : 31
*/
#define IME_PRIVACY_MASK_SET6_REGISTER0_OFS 0x04fc
REGDEF_BEGIN(IME_PRIVACY_MASK_SET6_REGISTER0)
REGDEF_BIT(ime_pm6_type,        1)
REGDEF_BIT(,        3)
REGDEF_BIT(ime_pm6_color_y,        8)
REGDEF_BIT(ime_pm6_color_u,        8)
REGDEF_BIT(ime_pm6_color_v,        8)
REGDEF_BIT(,        3)
REGDEF_BIT(ime_pm6_hlw_en,        1)
REGDEF_END(IME_PRIVACY_MASK_SET6_REGISTER0)


/*
    ime_pm6_line0_comp               :    [0x0, 0x3],           bits : 1_0
    ime_pm6_line0_coefa             :    [0x0, 0x1fff],         bits : 16_4
    ime_pm6_line0_signa             :    [0x0, 0x1],            bits : 17
    ime_pm6_line0_coefb             :    [0x0, 0x1fff],         bits : 30_18
    ime_pm6_line0_signb             :    [0x0, 0x1],            bits : 31
*/
#define IME_PRIVACY_MASK_SET6_REGISTER1_OFS 0x0500
REGDEF_BEGIN(IME_PRIVACY_MASK_SET6_REGISTER1)
REGDEF_BIT(ime_pm6_line0_comp,        2)
REGDEF_BIT(,        2)
REGDEF_BIT(ime_pm6_line0_coefa,        13)
REGDEF_BIT(ime_pm6_line0_signa,        1)
REGDEF_BIT(ime_pm6_line0_coefb,        13)
REGDEF_BIT(ime_pm6_line0_signb,        1)
REGDEF_END(IME_PRIVACY_MASK_SET6_REGISTER1)


/*
    ime_pm6_line0_coefc             :    [0x0, 0x3ffffff],          bits : 25_0
    ime_pm6_line0_signc             :    [0x0, 0x1],            bits : 26
*/
#define IME_PRIVACY_MASK_SET6_REGISTER2_OFS 0x0504
REGDEF_BEGIN(IME_PRIVACY_MASK_SET6_REGISTER2)
REGDEF_BIT(ime_pm6_line0_coefc,        26)
REGDEF_BIT(ime_pm6_line0_signc,        1)
REGDEF_END(IME_PRIVACY_MASK_SET6_REGISTER2)


/*
    ime_pm6_line1_comp               :    [0x0, 0x3],           bits : 1_0
    ime_pm6_line1_coefa             :    [0x0, 0x1fff],         bits : 16_4
    ime_pm6_line1_signa             :    [0x0, 0x1],            bits : 17
    ime_pm6_line1_coefb             :    [0x0, 0x1fff],         bits : 30_18
    ime_pm6_line1_signb             :    [0x0, 0x1],            bits : 31
*/
#define IME_PRIVACY_MASK_SET6_REGISTER3_OFS 0x0508
REGDEF_BEGIN(IME_PRIVACY_MASK_SET6_REGISTER3)
REGDEF_BIT(ime_pm6_line1_comp,        2)
REGDEF_BIT(,        2)
REGDEF_BIT(ime_pm6_line1_coefa,        13)
REGDEF_BIT(ime_pm6_line1_signa,        1)
REGDEF_BIT(ime_pm6_line1_coefb,        13)
REGDEF_BIT(ime_pm6_line1_signb,        1)
REGDEF_END(IME_PRIVACY_MASK_SET6_REGISTER3)


/*
    ime_pm6_line1_coefc             :    [0x0, 0x3ffffff],          bits : 25_0
    ime_pm6_line1_signc             :    [0x0, 0x1],            bits : 26
*/
#define IME_PRIVACY_MASK_SET6_REGISTER4_OFS 0x050c
REGDEF_BEGIN(IME_PRIVACY_MASK_SET6_REGISTER4)
REGDEF_BIT(ime_pm6_line1_coefc,        26)
REGDEF_BIT(ime_pm6_line1_signc,        1)
REGDEF_END(IME_PRIVACY_MASK_SET6_REGISTER4)


/*
    ime_pm6_line2_comp               :    [0x0, 0x3],           bits : 1_0
    ime_pm6_line2_coefa             :    [0x0, 0x1fff],         bits : 16_4
    ime_pm6_line2_signa             :    [0x0, 0x1],            bits : 17
    ime_pm6_line2_coefb             :    [0x0, 0x1fff],         bits : 30_18
    ime_pm6_line2_signb             :    [0x0, 0x1],            bits : 31
*/
#define IME_PRIVACY_MASK_SET6_REGISTER5_OFS 0x0510
REGDEF_BEGIN(IME_PRIVACY_MASK_SET6_REGISTER5)
REGDEF_BIT(ime_pm6_line2_comp,        2)
REGDEF_BIT(,        2)
REGDEF_BIT(ime_pm6_line2_coefa,        13)
REGDEF_BIT(ime_pm6_line2_signa,        1)
REGDEF_BIT(ime_pm6_line2_coefb,        13)
REGDEF_BIT(ime_pm6_line2_signb,        1)
REGDEF_END(IME_PRIVACY_MASK_SET6_REGISTER5)


/*
    ime_pm6_line2_coefc             :    [0x0, 0x3ffffff],          bits : 25_0
    ime_pm6_line2_signc             :    [0x0, 0x1],            bits : 26
*/
#define IME_PRIVACY_MASK_SET6_REGISTER6_OFS 0x0514
REGDEF_BEGIN(IME_PRIVACY_MASK_SET6_REGISTER6)
REGDEF_BIT(ime_pm6_line2_coefc,        26)
REGDEF_BIT(ime_pm6_line2_signc,        1)
REGDEF_END(IME_PRIVACY_MASK_SET6_REGISTER6)


/*
    ime_pm6_line3_comp               :    [0x0, 0x3],           bits : 1_0
    ime_pm6_line3_coefa             :    [0x0, 0x1fff],         bits : 16_4
    ime_pm6_line3_signa             :    [0x0, 0x1],            bits : 17
    ime_pm6_line3_coefb             :    [0x0, 0x1fff],         bits : 30_18
    ime_pm6_line3_signb             :    [0x0, 0x1],            bits : 31
*/
#define IME_PRIVACY_MASK_SET6_REGISTER7_OFS 0x0518
REGDEF_BEGIN(IME_PRIVACY_MASK_SET6_REGISTER7)
REGDEF_BIT(ime_pm6_line3_comp,        2)
REGDEF_BIT(,        2)
REGDEF_BIT(ime_pm6_line3_coefa,        13)
REGDEF_BIT(ime_pm6_line3_signa,        1)
REGDEF_BIT(ime_pm6_line3_coefb,        13)
REGDEF_BIT(ime_pm6_line3_signb,        1)
REGDEF_END(IME_PRIVACY_MASK_SET6_REGISTER7)


/*
    ime_pm6_line3_coefc             :    [0x0, 0x3ffffff],          bits : 25_0
    ime_pm6_line3_signc             :    [0x0, 0x1],            bits : 26
*/
#define IME_PRIVACY_MASK_SET6_REGISTER8_OFS 0x051c
REGDEF_BEGIN(IME_PRIVACY_MASK_SET6_REGISTER8)
REGDEF_BIT(ime_pm6_line3_coefc,        26)
REGDEF_BIT(ime_pm6_line3_signc,        1)
REGDEF_END(IME_PRIVACY_MASK_SET6_REGISTER8)


/*
    ime_pm7_type                   :    [0x0, 0x1],         bits : 0
    ime_pm7_color_y             :    [0x0, 0xff],           bits : 11_4
    ime_pm7_color_u             :    [0x0, 0xff],           bits : 19_12
    ime_pm7_color_v             :    [0x0, 0xff],           bits : 27_20
*/
#define IME_PRIVACY_MASK_SET7_REGISTER0_OFS 0x0520
REGDEF_BEGIN(IME_PRIVACY_MASK_SET7_REGISTER0)
REGDEF_BIT(ime_pm7_type,        1)
REGDEF_BIT(,        3)
REGDEF_BIT(ime_pm7_color_y,        8)
REGDEF_BIT(ime_pm7_color_u,        8)
REGDEF_BIT(ime_pm7_color_v,        8)
REGDEF_END(IME_PRIVACY_MASK_SET7_REGISTER0)


/*
    ime_pm7_line0_comp               :    [0x0, 0x3],           bits : 1_0
    ime_pm7_line0_coefa             :    [0x0, 0x1fff],         bits : 16_4
    ime_pm7_line0_signa             :    [0x0, 0x1],            bits : 17
    ime_pm7_line0_coefb             :    [0x0, 0x1fff],         bits : 30_18
    ime_pm7_line0_signb             :    [0x0, 0x1],            bits : 31
*/
#define IME_PRIVACY_MASK_SET7_REGISTER1_OFS 0x0524
REGDEF_BEGIN(IME_PRIVACY_MASK_SET7_REGISTER1)
REGDEF_BIT(ime_pm7_line0_comp,        2)
REGDEF_BIT(,        2)
REGDEF_BIT(ime_pm7_line0_coefa,        13)
REGDEF_BIT(ime_pm7_line0_signa,        1)
REGDEF_BIT(ime_pm7_line0_coefb,        13)
REGDEF_BIT(ime_pm7_line0_signb,        1)
REGDEF_END(IME_PRIVACY_MASK_SET7_REGISTER1)


/*
    ime_pm7_line0_coefc             :    [0x0, 0x3ffffff],          bits : 25_0
    ime_pm7_line0_signc             :    [0x0, 0x1],            bits : 26
*/
#define IME_PRIVACY_MASK_SET7_REGISTER2_OFS 0x0528
REGDEF_BEGIN(IME_PRIVACY_MASK_SET7_REGISTER2)
REGDEF_BIT(ime_pm7_line0_coefc,        26)
REGDEF_BIT(ime_pm7_line0_signc,        1)
REGDEF_END(IME_PRIVACY_MASK_SET7_REGISTER2)


/*
    ime_pm7_line1_comp               :    [0x0, 0x3],           bits : 1_0
    ime_pm7_line1_coefa             :    [0x0, 0x1fff],         bits : 16_4
    ime_pm7_line1_signa             :    [0x0, 0x1],            bits : 17
    ime_pm7_line1_coefb             :    [0x0, 0x1fff],         bits : 30_18
    ime_pm7_line1_signb             :    [0x0, 0x1],            bits : 31
*/
#define IME_PRIVACY_MASK_SET7_REGISTER3_OFS 0x052c
REGDEF_BEGIN(IME_PRIVACY_MASK_SET7_REGISTER3)
REGDEF_BIT(ime_pm7_line1_comp,        2)
REGDEF_BIT(,        2)
REGDEF_BIT(ime_pm7_line1_coefa,        13)
REGDEF_BIT(ime_pm7_line1_signa,        1)
REGDEF_BIT(ime_pm7_line1_coefb,        13)
REGDEF_BIT(ime_pm7_line1_signb,        1)
REGDEF_END(IME_PRIVACY_MASK_SET7_REGISTER3)


/*
    ime_pm7_line1_coefc             :    [0x0, 0x3ffffff],          bits : 25_0
    ime_pm7_line1_signc             :    [0x0, 0x1],            bits : 26
*/
#define IME_PRIVACY_MASK_SET7_REGISTER4_OFS 0x0530
REGDEF_BEGIN(IME_PRIVACY_MASK_SET7_REGISTER4)
REGDEF_BIT(ime_pm7_line1_coefc,        26)
REGDEF_BIT(ime_pm7_line1_signc,        1)
REGDEF_END(IME_PRIVACY_MASK_SET7_REGISTER4)


/*
    ime_pm7_line2_comp               :    [0x0, 0x3],           bits : 1_0
    ime_pm7_line2_coefa             :    [0x0, 0x1fff],         bits : 16_4
    ime_pm7_line2_signa             :    [0x0, 0x1],            bits : 17
    ime_pm7_line2_coefb             :    [0x0, 0x1fff],         bits : 30_18
    ime_pm7_line2_signb             :    [0x0, 0x1],            bits : 31
*/
#define IME_PRIVACY_MASK_SET7_REGISTER5_OFS 0x0534
REGDEF_BEGIN(IME_PRIVACY_MASK_SET7_REGISTER5)
REGDEF_BIT(ime_pm7_line2_comp,        2)
REGDEF_BIT(,        2)
REGDEF_BIT(ime_pm7_line2_coefa,        13)
REGDEF_BIT(ime_pm7_line2_signa,        1)
REGDEF_BIT(ime_pm7_line2_coefb,        13)
REGDEF_BIT(ime_pm7_line2_signb,        1)
REGDEF_END(IME_PRIVACY_MASK_SET7_REGISTER5)


/*
    ime_pm7_line2_coefc             :    [0x0, 0x3ffffff],          bits : 25_0
    ime_pm7_line2_signc             :    [0x0, 0x1],            bits : 26
*/
#define IME_PRIVACY_MASK_SET7_REGISTER6_OFS 0x0538
REGDEF_BEGIN(IME_PRIVACY_MASK_SET7_REGISTER6)
REGDEF_BIT(ime_pm7_line2_coefc,        26)
REGDEF_BIT(ime_pm7_line2_signc,        1)
REGDEF_END(IME_PRIVACY_MASK_SET7_REGISTER6)


/*
    ime_pm7_line3_comp               :    [0x0, 0x3],           bits : 1_0
    ime_pm7_line3_coefa             :    [0x0, 0x1fff],         bits : 16_4
    ime_pm7_line3_signa             :    [0x0, 0x1],            bits : 17
    ime_pm7_line3_coefb             :    [0x0, 0x1fff],         bits : 30_18
    ime_pm7_line3_signb             :    [0x0, 0x1],            bits : 31
*/
#define IME_PRIVACY_MASK_SET7_REGISTER7_OFS 0x053c
REGDEF_BEGIN(IME_PRIVACY_MASK_SET7_REGISTER7)
REGDEF_BIT(ime_pm7_line3_comp,        2)
REGDEF_BIT(,        2)
REGDEF_BIT(ime_pm7_line3_coefa,        13)
REGDEF_BIT(ime_pm7_line3_signa,        1)
REGDEF_BIT(ime_pm7_line3_coefb,        13)
REGDEF_BIT(ime_pm7_line3_signb,        1)
REGDEF_END(IME_PRIVACY_MASK_SET7_REGISTER7)


/*
    ime_pm7_line3_coefc             :    [0x0, 0x3ffffff],          bits : 25_0
    ime_pm7_line3_signc             :    [0x0, 0x1],            bits : 26
*/
#define IME_PRIVACY_MASK_SET7_REGISTER8_OFS 0x0540
REGDEF_BEGIN(IME_PRIVACY_MASK_SET7_REGISTER8)
REGDEF_BIT(ime_pm7_line3_coefc,        26)
REGDEF_BIT(ime_pm7_line3_signc,        1)
REGDEF_END(IME_PRIVACY_MASK_SET7_REGISTER8)


/*
    ime_pm4_awet                :    [0x0, 0xff],           bits : 7_0
    ime_pm5_awet                :    [0x0, 0xff],           bits : 15_8
    ime_pm6_awet                :    [0x0, 0xff],           bits : 23_16
    ime_pm7_awet                :    [0x0, 0xff],           bits : 31_24
*/
#define IME_PRIVACY_MASK_ALPHA_REGISTER1_OFS 0x0544
REGDEF_BEGIN(IME_PRIVACY_MASK_ALPHA_REGISTER1)
REGDEF_BIT(ime_pm4_awet,        8)
REGDEF_BIT(ime_pm5_awet,        8)
REGDEF_BIT(ime_pm6_awet,        8)
REGDEF_BIT(ime_pm7_awet,        8)
REGDEF_END(IME_PRIVACY_MASK_ALPHA_REGISTER1)


/*
    ime_pm_h_size:    [0x0, 0x7ff],         bits : 10_0
    ime_pm_v_size:    [0x0, 0x7ff],         bits : 26_16
    ime_pm_fmt   :    [0x0, 0x3],           bits : 31_30
*/
#define IME_PRIVACY_MASK_SUB_IMAGE_SIZE_REGISTER_OFS 0x0548
REGDEF_BEGIN(IME_PRIVACY_MASK_SUB_IMAGE_SIZE_REGISTER)
REGDEF_BIT(ime_pm_h_size,        11)
REGDEF_BIT(,        5)
REGDEF_BIT(ime_pm_v_size,        11)
REGDEF_BIT(,        3)
REGDEF_BIT(ime_pm_fmt,        2)
REGDEF_END(IME_PRIVACY_MASK_SUB_IMAGE_SIZE_REGISTER)


/*
    ime_pm_y_ofsi:    [0x0, 0x3ffff],           bits : 19_2
*/
#define IME_PRIVACY_MASK_SUB_IMAGE_Y_CHANNEL_LINEOFFSET_REGISTER_OFS 0x054c
REGDEF_BEGIN(IME_PRIVACY_MASK_SUB_IMAGE_Y_CHANNEL_LINEOFFSET_REGISTER)
REGDEF_BIT(,        2)
REGDEF_BIT(ime_pm_y_ofsi,        18)
REGDEF_END(IME_PRIVACY_MASK_SUB_IMAGE_Y_CHANNEL_LINEOFFSET_REGISTER)


/*
    ime_pm_y_sai:    [0x0, 0x1fffffff],         bits : 30_2
*/
#define IME_PRIVACY_MASK_IMAGE_INPUT_Y_CHANNEL_DMA_ADDRESS_REGISTER_OFS 0x0550
REGDEF_BEGIN(IME_PRIVACY_MASK_IMAGE_INPUT_Y_CHANNEL_DMA_ADDRESS_REGISTER)
REGDEF_BIT(,        2)
REGDEF_BIT(ime_pm_y_sai,        29)
REGDEF_END(IME_PRIVACY_MASK_IMAGE_INPUT_Y_CHANNEL_DMA_ADDRESS_REGISTER)


/*
    ime_in_pxl_bp1:    [0x0, 0xffffffff],           bits : 31_0
*/
#define IME_INPUT_PATH_PIXEL_BREAK_POINT_REGISTER0_OFS 0x0554
REGDEF_BEGIN(IME_INPUT_PIXEL_PATH_BREAK_POINT_REGISTER0)
REGDEF_BIT(ime_in_pxl_bp1,        32)
REGDEF_END(IME_INPUT_PIXEL_PATH_BREAK_POINT_REGISTER0)



/*
    ime_in_pxl_bp2:    [0x0, 0xffffffff],           bits : 31_0
*/
#define IME_INPUT_PATH_PIXEL_BREAK_POINT_REGISTER1_OFS 0x0558
REGDEF_BEGIN(IME_INPUT_PIXEL_PATH_BREAK_POINT_REGISTER1)
REGDEF_BIT(ime_in_pxl_bp2,        32)
REGDEF_END(IME_INPUT_PIXEL_PATH_BREAK_POINT_REGISTER1)



/*
    ime_in_pxl_bp3:    [0x0, 0xffffffff],           bits : 31_0
*/
#define IME_INPUT_PATH_PIXEL_BREAK_POINT_REGISTER2_OFS 0x055c
REGDEF_BEGIN(IME_INPUT_PIXEL_PATH_BREAK_POINT_REGISTER2)
REGDEF_BIT(ime_in_pxl_bp3,        32)
REGDEF_END(IME_INPUT_PIXEL_PATH_BREAK_POINT_REGISTER2)



/*
    ime_in_line_bp1:    [0x0, 0xffff],           bits : 15_0
    ime_in_line_bp2:    [0x0, 0xffff],           bits : 31_16
*/
#define IME_INPUT_PATH_LINE_BREAK_POINT_REGISTER0_OFS 0x0560
REGDEF_BEGIN(IME_INPUT_PATH_LINE_BREAK_POINT_REGISTER0)
REGDEF_BIT(ime_in_line_bp1,        16)
REGDEF_BIT(ime_in_line_bp2,        16)
REGDEF_END(IME_INPUT_PATH_LINE_BREAK_POINT_REGISTER0)


/*
    ime_in_line_bp3:    [0x0, 0xffff],          bits : 15_0
    ime_bp_mode    :    [0x0, 0x1],         bits : 31
*/
#define IME_INPUT_PATH_LINE_BREAK_POINT_REGISTER1_OFS 0x0564
REGDEF_BEGIN(IME_INPUT_PATH_LINE_BREAK_POINT_REGISTER1)
REGDEF_BIT(ime_in_line_bp3,        16)
REGDEF_BIT(reserved,               15)
REGDEF_BIT(ime_bp_mode,             1)
REGDEF_END(IME_INPUT_PATH_LINE_BREAK_POINT_REGISTER1)


/*
    reserved                :    [0x0, 0xffffffff],         bits : 31_0
*/
#define IME_RESERVED_81_OFS 0x0568
REGDEF_BEGIN(IME_RESERVED_81)
REGDEF_BIT(reserved,        32)
REGDEF_END(IME_RESERVED_81)


/*
    reserved                :    [0x0, 0xffffffff],         bits : 31_0
*/
#define IME_RESERVED_82_OFS 0x056c
REGDEF_BEGIN(IME_RESERVED_82)
REGDEF_BIT(reserved,        32)
REGDEF_END(IME_RESERVED_82)


/*
    ime_in_bst_y                     :    [0x0, 0x1],           bits : 0
    ime_in_bst_u                     :    [0x0, 0x1],           bits : 1
    ime_in_bst_v                     :    [0x0, 0x1],           bits : 2
    ime_out_p1_bst_y                 :    [0x0, 0x3],           bits : 4_3
    ime_out_p1_bst_u                 :    [0x0, 0x3],           bits : 6_5
    ime_out_p1_bst_v                 :    [0x0, 0x1],           bits : 7
    ime_out_p2_bst_y                 :    [0x0, 0x3],           bits : 9_8
    ime_out_p2_bst_uv               :    [0x0, 0x3],            bits : 11_10
    ime_out_p3_bst_y                 :    [0x0, 0x1],           bits : 12
    ime_out_p3_bst_uv               :    [0x0, 0x1],            bits : 13
    ime_out_p4_bst_y                 :    [0x0, 0x1],           bits : 14
    ime_in_lca_bst                 :    [0x0, 0x1],         bits : 15
    ime_out_lca_bst      :    [0x0, 0x1],           bits : 16
    ime_in_stp_bst                 :    [0x0, 0x1],         bits : 17
    ime_in_pix_bst       :    [0x0, 0x1],           bits : 18
    ime_in_tmnr_bst_y    :    [0x0, 0x1],           bits : 19
    ime_in_tmnr_bst_c    :    [0x0, 0x1],           bits : 20
    ime_out_tmnr_bst_y   :    [0x0, 0x3],           bits : 22_21
    ime_out_tmnr_bst_c   :    [0x0, 0x3],           bits : 24_23
    ime_in_tmnr_bst_m    :    [0x0, 0x1],           bits : 25
    ime_out_tmnr_bst_m   :    [0x0, 0x1],           bits : 26
    ime_out_tmnr_bst_info:    [0x0, 0x1],           bits : 27
    ime_in_md_bst_lv     :    [0x0, 0x1],           bits : 28
    ime_in_md_bst_sta    :    [0x0, 0x1],           bits : 29
    ime_out_md_bst_sta   :    [0x0, 0x1],           bits : 30
*/
#define IME_BURST_LENGTH_REGISTER0_OFS 0x0570
REGDEF_BEGIN(IME_BURST_LENGTH_REGISTER0)
REGDEF_BIT(ime_in_bst_y,        1)
REGDEF_BIT(ime_in_bst_u,        1)
REGDEF_BIT(ime_in_bst_v,        1)
REGDEF_BIT(ime_out_p1_bst_y,        2)
REGDEF_BIT(ime_out_p1_bst_u,        2)
REGDEF_BIT(ime_out_p1_bst_v,        1)
REGDEF_BIT(ime_out_p2_bst_y,        2)
REGDEF_BIT(ime_out_p2_bst_uv,        2)
REGDEF_BIT(ime_out_p3_bst_y,        1)
REGDEF_BIT(ime_out_p3_bst_uv,        1)
REGDEF_BIT(ime_out_p4_bst_y,        1)
REGDEF_BIT(ime_in_lca_bst,        1)
REGDEF_BIT(ime_out_lca_bst,        1)
REGDEF_BIT(ime_in_stp_bst,        1)
REGDEF_BIT(ime_in_pix_bst,        1)
REGDEF_BIT(ime_in_tmnr_bst_y,        1)
REGDEF_BIT(ime_in_tmnr_bst_c,        1)
REGDEF_BIT(ime_out_tmnr_bst_y,        2)
REGDEF_BIT(ime_out_tmnr_bst_c,        2)
REGDEF_BIT(ime_in_tmnr_bst_mv,        1)
REGDEF_BIT(ime_out_tmnr_bst_mv,        1)
REGDEF_BIT(ime_in_tmnr_bst_mo,        1)
REGDEF_BIT(ime_out_tmnr_bst_mo,        1)
REGDEF_BIT(ime_out_tmnr_bst_mo_roi,        1)
REGDEF_BIT(ime_out_tmnr_bst_sta,        1)
REGDEF_END(IME_BURST_LENGTH_REGISTER0)


/*
    ime_frm_start_rst   :    [0x0, 0x1],           bits : 0
    ime_chksum_rst      :    [0x0, 0x1],           bits : 1
*/
#define IME_FRAME_START_RESET_CONTROL_REGISTER_OFS 0x0574
REGDEF_BEGIN(IME_FRAME_START_RESET_CONTROL_REGISTER)
REGDEF_BIT(reserved,        30)
REGDEF_BIT(ime_frm_start_rst,       1)
REGDEF_BIT(ime_chksum_en,      1)
REGDEF_END(IME_FRAME_START_RESET_CONTROL_REGISTER)


/*
    reserved                :    [0x0, 0xffffffff],         bits : 31_0
*/
#define IME_RESERVED_84_OFS 0x0578
REGDEF_BEGIN(IME_RESERVED_84)
REGDEF_BIT(reserved,        32)
REGDEF_END(IME_RESERVED_84)


/*
    reserved                :    [0x0, 0xffffffff],         bits : 31_0
*/
#define IME_RESERVED_85_OFS 0x057c
REGDEF_BEGIN(IME_RESERVED_85)
REGDEF_BIT(reserved,        32)
REGDEF_END(IME_RESERVED_85)


/*
    ime_ll_tab0:    [0x0, 0xff],            bits : 7_0
    ime_ll_tab1:    [0x0, 0xff],            bits : 15_8
    ime_ll_tab2:    [0x0, 0xff],            bits : 23_16
    ime_ll_tab3:    [0x0, 0xff],            bits : 31_24
*/
#define IME_LINKED_LIST_TABLE_INDEX_REGISTER0_OFS 0x0580
REGDEF_BEGIN(IME_LINKED_LIST_TABLE_INDEX_REGISTER0)
REGDEF_BIT(ime_ll_tab0,        8)
REGDEF_BIT(ime_ll_tab1,        8)
REGDEF_BIT(ime_ll_tab2,        8)
REGDEF_BIT(ime_ll_tab3,        8)
REGDEF_END(IME_LINKED_LIST_TABLE_INDEX_REGISTER0)


/*
    ime_ll_tab4:    [0x0, 0xff],            bits : 7_0
    ime_ll_tab5:    [0x0, 0xff],            bits : 15_8
    ime_ll_tab6:    [0x0, 0xff],            bits : 23_16
    ime_ll_tab7:    [0x0, 0xff],            bits : 31_24
*/
#define IME_LINKED_LIST_TABLE_INDEX_REGISTER1_OFS 0x0584
REGDEF_BEGIN(IME_LINKED_LIST_TABLE_INDEX_REGISTER1)
REGDEF_BIT(ime_ll_tab4,        8)
REGDEF_BIT(ime_ll_tab5,        8)
REGDEF_BIT(ime_ll_tab6,        8)
REGDEF_BIT(ime_ll_tab7,        8)
REGDEF_END(IME_LINKED_LIST_TABLE_INDEX_REGISTER1)


/*
    ime_ll_tab8 :    [0x0, 0xff],           bits : 7_0
    ime_ll_tab9 :    [0x0, 0xff],           bits : 15_8
    ime_ll_tab10:    [0x0, 0xff],           bits : 23_16
    ime_ll_tab11:    [0x0, 0xff],           bits : 31_24
*/
#define IME_LINKED_LIST_TABLE_INDEX_REGISTER2_OFS 0x0588
REGDEF_BEGIN(IME_LINKED_LIST_TABLE_INDEX_REGISTER2)
REGDEF_BIT(ime_ll_tab8,        8)
REGDEF_BIT(ime_ll_tab9,        8)
REGDEF_BIT(ime_ll_tab10,        8)
REGDEF_BIT(ime_ll_tab11,        8)
REGDEF_END(IME_LINKED_LIST_TABLE_INDEX_REGISTER2)


/*
    ime_ll_tab12:    [0x0, 0xff],           bits : 7_0
    ime_ll_tab13:    [0x0, 0xff],           bits : 15_8
    ime_ll_tab14:    [0x0, 0xff],           bits : 23_16
    ime_ll_tab15:    [0x0, 0xff],           bits : 31_24
*/
#define IME_LINKED_LIST_TABLE_INDEX_REGISTER3_OFS 0x058c
REGDEF_BEGIN(IME_LINKED_LIST_TABLE_INDEX_REGISTER3)
REGDEF_BIT(ime_ll_tab12,        8)
REGDEF_BIT(ime_ll_tab13,        8)
REGDEF_BIT(ime_ll_tab14,        8)
REGDEF_BIT(ime_ll_tab15,        8)
REGDEF_END(IME_LINKED_LIST_TABLE_INDEX_REGISTER3)


/*
    reserved                :    [0x0, 0xffffffff],         bits : 31_0
*/
#define IME_RESERVED_86_OFS 0x0590
REGDEF_BEGIN(IME_RESERVED_86)
REGDEF_BIT(reserved,        32)
REGDEF_END(IME_RESERVED_86)


/*
    reserved                :    [0x0, 0xffffffff],         bits : 31_0
*/
#define IME_RESERVED_87_OFS 0x0594
REGDEF_BEGIN(IME_RESERVED_87)
REGDEF_BIT(reserved,        32)
REGDEF_END(IME_RESERVED_87)


/*
    reserved                :    [0x0, 0xffffffff],         bits : 31_0
*/
#define IME_RESERVED_88_OFS 0x0598
REGDEF_BEGIN(IME_RESERVED_88)
REGDEF_BIT(reserved,        32)
REGDEF_END(IME_RESERVED_88)


/*
    reserved                :    [0x0, 0xffffffff],         bits : 31_0
*/
#define IME_RESERVED_89_OFS 0x059c
REGDEF_BEGIN(IME_RESERVED_89)
REGDEF_BIT(reserved,        32)
REGDEF_END(IME_RESERVED_89)


/*
    reserved                :    [0x0, 0xffffffff],         bits : 31_0
*/
#define IME_RESERVED_90_OFS 0x05a0
REGDEF_BEGIN(IME_RESERVED_90)
REGDEF_BIT(reserved,        32)
REGDEF_END(IME_RESERVED_90)


/*
    reserved                :    [0x0, 0xffffffff],         bits : 31_0
*/
#define IME_RESERVED_91_OFS 0x05a4
REGDEF_BEGIN(IME_RESERVED_91)
REGDEF_BIT(reserved,        32)
REGDEF_END(IME_RESERVED_91)


/*
    reserved                :    [0x0, 0xffffffff],         bits : 31_0
*/
#define IME_RESERVED_92_OFS 0x05a8
REGDEF_BEGIN(IME_RESERVED_92)
REGDEF_BIT(reserved,        32)
REGDEF_END(IME_RESERVED_92)


/*
    reserved                :    [0x0, 0xffffffff],         bits : 31_0
*/
#define IME_RESERVED_93_OFS 0x05ac
REGDEF_BEGIN(IME_RESERVED_93)
REGDEF_BIT(reserved,        32)
REGDEF_END(IME_RESERVED_93)


/*
    reserved                :    [0x0, 0xffffffff],         bits : 31_0
*/
#define IME_RESERVED_94_OFS 0x05b0
REGDEF_BEGIN(IME_RESERVED_94)
REGDEF_BIT(reserved,        32)
REGDEF_END(IME_RESERVED_94)


/*
    reserved                :    [0x0, 0xffffffff],         bits : 31_0
*/
#define IME_RESERVED_95_OFS 0x05b4
REGDEF_BEGIN(IME_RESERVED_95)
REGDEF_BIT(reserved,        32)
REGDEF_END(IME_RESERVED_95)


/*
    reserved                :    [0x0, 0xffffffff],         bits : 31_0
*/
#define IME_RESERVED_96_OFS 0x05b8
REGDEF_BEGIN(IME_RESERVED_96)
REGDEF_BIT(reserved,        32)
REGDEF_END(IME_RESERVED_96)


/*
    reserved                :    [0x0, 0xffffffff],         bits : 31_0
*/
#define IME_RESERVED_97_OFS 0x05bc
REGDEF_BEGIN(IME_RESERVED_97)
REGDEF_BIT(reserved,        32)
REGDEF_END(IME_RESERVED_97)


/*
    reserved                :    [0x0, 0xffffffff],         bits : 31_0
*/
#define IME_RESERVED_98_OFS 0x05c0
REGDEF_BEGIN(IME_RESERVED_98)
REGDEF_BIT(reserved,        32)
REGDEF_END(IME_RESERVED_98)


/*
    reserved                :    [0x0, 0xffffffff],         bits : 31_0
*/
#define IME_RESERVED_99_OFS 0x05c4
REGDEF_BEGIN(IME_RESERVED_99)
REGDEF_BIT(reserved,        32)
REGDEF_END(IME_RESERVED_99)


/*
    reserved                :    [0x0, 0xffffffff],         bits : 31_0
*/
#define IME_RESERVED_100_OFS 0x05c8
REGDEF_BEGIN(IME_RESERVED_100)
REGDEF_BIT(reserved,        32)
REGDEF_END(IME_RESERVED_100)


/*
    reserved                :    [0x0, 0xffffffff],         bits : 31_0
*/
#define IME_RESERVED_101_OFS 0x05cc
REGDEF_BEGIN(IME_RESERVED_101)
REGDEF_BIT(reserved,        32)
REGDEF_END(IME_RESERVED_101)


/*
    reserved                :    [0x0, 0xffffffff],         bits : 31_0
*/
#define IME_RESERVED_102_OFS 0x05d0
REGDEF_BEGIN(IME_RESERVED_102)
REGDEF_BIT(reserved,        32)
REGDEF_END(IME_RESERVED_102)


/*
    reserved                :    [0x0, 0xffffffff],         bits : 31_0
*/
#define IME_RESERVED_103_OFS 0x05d4
REGDEF_BEGIN(IME_RESERVED_103)
REGDEF_BIT(reserved,        32)
REGDEF_END(IME_RESERVED_103)


/*
    reserved                :    [0x0, 0xffffffff],         bits : 31_0
*/
#define IME_RESERVED_104_OFS 0x05d8
REGDEF_BEGIN(IME_RESERVED_104)
REGDEF_BIT(reserved,        32)
REGDEF_END(IME_RESERVED_104)


/*
    reserved                :    [0x0, 0xffffffff],         bits : 31_0
*/
#define IME_RESERVED_105_OFS 0x05dc
REGDEF_BEGIN(IME_RESERVED_105)
REGDEF_BIT(reserved,        32)
REGDEF_END(IME_RESERVED_105)


/*
    reserved                :    [0x0, 0xffffffff],         bits : 31_0
*/
#define IME_RESERVED_106_OFS 0x05e0
REGDEF_BEGIN(IME_RESERVED_106)
REGDEF_BIT(reserved,        32)
REGDEF_END(IME_RESERVED_106)


/*
    reserved                :    [0x0, 0xffffffff],         bits : 31_0
*/
#define IME_RESERVED_107_OFS 0x05e4
REGDEF_BEGIN(IME_RESERVED_107)
REGDEF_BIT(reserved,        32)
REGDEF_END(IME_RESERVED_107)


/*
    reserved                :    [0x0, 0xffffffff],         bits : 31_0
*/
#define IME_RESERVED_108_OFS 0x05e8
REGDEF_BEGIN(IME_RESERVED_108)
REGDEF_BIT(reserved,        32)
REGDEF_END(IME_RESERVED_108)


/*
    reserved                :    [0x0, 0xffffffff],         bits : 31_0
*/
#define IME_RESERVED_109_OFS 0x05ec
REGDEF_BEGIN(IME_RESERVED_109)
REGDEF_BIT(reserved,        32)
REGDEF_END(IME_RESERVED_109)


/*
    reserved                :    [0x0, 0xffffffff],         bits : 31_0
*/
#define IME_RESERVED_110_OFS 0x05f0
REGDEF_BEGIN(IME_RESERVED_110)
REGDEF_BIT(reserved,        32)
REGDEF_END(IME_RESERVED_110)


/*
    reserved                :    [0x0, 0xffffffff],         bits : 31_0
*/
#define IME_RESERVED_111_OFS 0x05f4
REGDEF_BEGIN(IME_RESERVED_111)
REGDEF_BIT(reserved,        32)
REGDEF_END(IME_RESERVED_111)


/*
    reserved                :    [0x0, 0xffffffff],         bits : 31_0
*/
#define IME_RESERVED_112_OFS 0x05f8
REGDEF_BEGIN(IME_RESERVED_112)
REGDEF_BIT(reserved,        32)
REGDEF_END(IME_RESERVED_112)


/*
    reserved                :    [0x0, 0xffffffff],         bits : 31_0
*/
#define IME_RESERVED_113_OFS 0x05fc
REGDEF_BEGIN(IME_RESERVED_113)
REGDEF_BIT(reserved,        32)
REGDEF_END(IME_RESERVED_113)


/*
    ime_3dnr_pre_y_blur_str      :    [0x0, 0x3],           bits : 1_0
    ime_3dnr_pf_type             :    [0x0, 0x3],           bits : 3_2
    ime_3dnr_me_update_mode      :    [0x0, 0x1],           bits : 4
    ime_3dnr_me_boundary_set     :    [0x0, 0x1],           bits : 5
    ime_3dnr_me_mv_ds_mode       :    [0x0, 0x3],           bits : 7_6
    ime_3dnr_ps_smart_roi_ctrl   :    [0x0, 0x1],           bits : 8
    ime_3dnr_nr_center_wzero_y_3d:    [0x0, 0x1],           bits : 9
    ime_3dnr_ps_mv_check_en      :    [0x0, 0x1],           bits : 10
    ime_3dnr_ps_mv_check_roi_en  :    [0x0, 0x1],           bits : 11
    ime_3dnr_ps_mv_info_mode     :    [0x0, 0x3],           bits : 13_12
    ime_3dnr_ps_mode             :    [0x0, 0x1],           bits : 14
    ime_3dnr_me_sad_type         :    [0x0, 0x1],           bits : 15
    ime_3dnr_me_sad_shift        :    [0x0, 0xf],           bits : 19_16
    ime_3dnr_nr_y_ch_en          :    [0x0, 0x1],           bits : 20
    ime_3dnr_nr_c_ch_en          :    [0x0, 0x1],           bits : 21
    ime_3dnr_nr_c_fsv_en         :    [0x0, 0x1],           bits : 22
    ime_3dnr_seed_reset_en       :    [0x0, 0x1],           bits : 23
    ime_3dnr_nr_c_fsv            :    [0x0, 0xff],          bits : 31_24
*/
#define IME_TMNR_CONTROL_REGISTER0_OFS 0x0600
REGDEF_BEGIN(IME_TMNR_CONTROL_REGISTER0)
REGDEF_BIT(ime_3dnr_pre_y_blur_str,        2)
REGDEF_BIT(ime_3dnr_pf_type,        2)
REGDEF_BIT(ime_3dnr_me_update_mode,        1)
REGDEF_BIT(ime_3dnr_me_boundary_set,        1)
REGDEF_BIT(ime_3dnr_me_mv_ds_mode,        2)
REGDEF_BIT(ime_3dnr_ps_smart_roi_ctrl,        1)
REGDEF_BIT(ime_3dnr_nr_center_wzero_y_3d,        1)
REGDEF_BIT(ime_3dnr_ps_mv_check_en,        1)
REGDEF_BIT(ime_3dnr_ps_mv_check_roi_en,        1)
REGDEF_BIT(ime_3dnr_ps_mv_info_mode,        2)
REGDEF_BIT(ime_3dnr_ps_mode,        1)
REGDEF_BIT(ime_3dnr_me_sad_type,        1)
REGDEF_BIT(ime_3dnr_me_sad_shift,        4)
REGDEF_BIT(ime_3dnr_nr_y_ch_en,        1)
REGDEF_BIT(ime_3dnr_nr_c_ch_en,        1)
REGDEF_BIT(ime_3dnr_nr_c_fsv_en,        1)
REGDEF_BIT(ime_3dnr_seed_reset_en,        1)
REGDEF_BIT(ime_3dnr_nr_c_fsv,        8)
REGDEF_END(IME_TMNR_CONTROL_REGISTER0)


/*
    ime_3dnr_ne_sample_step_x   :    [0x0, 0xff],           bits : 7_0
    ime_3dnr_ne_sample_step_y   :    [0x0, 0xff],           bits : 15_8
    ime_3dnr_statistic_output_en:    [0x0, 0x1],            bits : 16
    ime_3dnr_ps_fastc_en        :    [0x0, 0x1],            bits : 17
    ime_3dnr_dbg_mv0            :    [0x0, 0x1],            bits : 18
    ime_3dnr_dbg_mode           :    [0x0, 0xf],            bits : 31_28
*/
#define IME_TMNR_CONTROL_REGISTER1_OFS 0x0604
REGDEF_BEGIN(IME_TMNR_CONTROL_REGISTER1)
REGDEF_BIT(ime_3dnr_ne_sample_step_x,        8)
REGDEF_BIT(ime_3dnr_ne_sample_step_y,        8)
REGDEF_BIT(ime_3dnr_statistic_output_en,        1)
REGDEF_BIT(ime_3dnr_ps_fastc_en,        1)
REGDEF_BIT(ime_3dnr_dbg_mv0,        1)
REGDEF_BIT(,        9)
REGDEF_BIT(ime_3dnr_dbg_mode,        4)
REGDEF_END(IME_TMNR_CONTROL_REGISTER1)


/*
    reserved                :    [0x0, 0xffffffff],         bits : 31_0
*/
#define IME_RESERVED_114_OFS 0x0608
REGDEF_BEGIN(IME_RESERVED_114)
REGDEF_BIT(reserved,        32)
REGDEF_END(IME_RESERVED_114)


/*
    reserved                :    [0x0, 0xffffffff],         bits : 31_0
*/
#define IME_RESERVED_115_OFS 0x060c
REGDEF_BEGIN(IME_RESERVED_115)
REGDEF_BIT(reserved,        32)
REGDEF_END(IME_RESERVED_115)


/*
    reserved                :    [0x0, 0xffffffff],         bits : 31_0
*/
#define IME_RESERVED_116_OFS 0x0610
REGDEF_BEGIN(IME_RESERVED_116)
REGDEF_BIT(reserved,        32)
REGDEF_END(IME_RESERVED_116)


/*
    reserved                :    [0x0, 0xffffffff],         bits : 31_0
*/
#define IME_RESERVED_117_OFS 0x0614
REGDEF_BEGIN(IME_RESERVED_117)
REGDEF_BIT(reserved,        32)
REGDEF_END(IME_RESERVED_117)


/*
    reserved                :    [0x0, 0xffffffff],         bits : 31_0
*/
#define IME_RESERVED_118_OFS 0x0618
REGDEF_BEGIN(IME_RESERVED_118)
REGDEF_BIT(reserved,        32)
REGDEF_END(IME_RESERVED_118)


/*
    ime_3dnr_ro_sad_sum:    [0x0, 0xffffffff],          bits : 31_0
*/
#define IME_3DNR_READ_ONLY_REGISTER0_OFS 0x061c
REGDEF_BEGIN(IME_3DNR_READ_ONLY_REGISTER0)
REGDEF_BIT(ime_3dnr_ro_sad_sum,        32)
REGDEF_END(IME_3DNR_READ_ONLY_REGISTER0)


/*
    ime_3dnr_ro_mv_sum:    [0x0, 0xffffffff],           bits : 31_0
*/
#define IME_3DNR_READ_ONLY_REGISTER1_OFS 0x0620
REGDEF_BEGIN(IME_3DNR_READ_ONLY_REGISTER1)
REGDEF_BIT(ime_3dnr_ro_mv_sum,        32)
REGDEF_END(IME_3DNR_READ_ONLY_REGISTER1)


/*
    ime_3dnr_ro_sample_cnt:    [0x0, 0xffffffff],           bits : 31_0
*/
#define IME_3DNR_READ_ONLY_REGISTER2_OFS 0x0624
REGDEF_BEGIN(IME_3DNR_READ_ONLY_REGISTER2)
REGDEF_BIT(ime_3dnr_ro_sample_cnt,        32)
REGDEF_END(IME_3DNR_READ_ONLY_REGISTER2)


/*
    ime_3dnr_me_sad_penalty_0:    [0x0, 0x3ff],         bits : 9_0
    ime_3dnr_me_sad_penalty_1:    [0x0, 0x3ff],         bits : 19_10
    ime_3dnr_me_sad_penalty_2:    [0x0, 0x3ff],         bits : 29_20
*/
#define IME_3DNR_MOTION_ESTIMATION_CONTROL_REGISTER0_OFS 0x0628
REGDEF_BEGIN(IME_3DNR_MOTION_ESTIMATION_CONTROL_REGISTER0)
REGDEF_BIT(ime_3dnr_me_sad_penalty_0,        10)
REGDEF_BIT(ime_3dnr_me_sad_penalty_1,        10)
REGDEF_BIT(ime_3dnr_me_sad_penalty_2,        10)
REGDEF_END(IME_3DNR_MOTION_ESTIMATION_CONTROL_REGISTER0)


/*
    ime_3dnr_me_sad_penalty_3:    [0x0, 0x3ff],         bits : 9_0
    ime_3dnr_me_sad_penalty_4:    [0x0, 0x3ff],         bits : 19_10
    ime_3dnr_me_sad_penalty_5:    [0x0, 0x3ff],         bits : 29_20
*/
#define IME_3DNR_MOTION_ESTIMATION_CONTROL_REGISTER1_OFS 0x062c
REGDEF_BEGIN(IME_3DNR_MOTION_ESTIMATION_CONTROL_REGISTER1)
REGDEF_BIT(ime_3dnr_me_sad_penalty_3,        10)
REGDEF_BIT(ime_3dnr_me_sad_penalty_4,        10)
REGDEF_BIT(ime_3dnr_me_sad_penalty_5,        10)
REGDEF_END(IME_3DNR_MOTION_ESTIMATION_CONTROL_REGISTER1)


/*
    ime_3dnr_me_sad_penalty_6:    [0x0, 0x3ff],         bits : 9_0
    ime_3dnr_me_sad_penalty_7:    [0x0, 0x3ff],         bits : 19_10
*/
#define IME_3DNR_MOTION_ESTIMATION_CONTROL_REGISTER2_OFS 0x0630
REGDEF_BEGIN(IME_3DNR_MOTION_ESTIMATION_CONTROL_REGISTER2)
REGDEF_BIT(ime_3dnr_me_sad_penalty_6,        10)
REGDEF_BIT(ime_3dnr_me_sad_penalty_7,        10)
REGDEF_END(IME_3DNR_MOTION_ESTIMATION_CONTROL_REGISTER2)


/*
    ime_3dnr_me_switch_th0:    [0x0, 0xff],         bits : 7_0
    ime_3dnr_me_switch_th1:    [0x0, 0xff],         bits : 15_8
    ime_3dnr_me_switch_th2:    [0x0, 0xff],         bits : 23_16
    ime_3dnr_me_switch_th3:    [0x0, 0xff],         bits : 31_24
*/
#define IME_3DNR_MOTION_ESTIMATION_CONTROL_REGISTER3_OFS 0x0634
REGDEF_BEGIN(IME_3DNR_MOTION_ESTIMATION_CONTROL_REGISTER3)
REGDEF_BIT(ime_3dnr_me_switch_th0,        8)
REGDEF_BIT(ime_3dnr_me_switch_th1,        8)
REGDEF_BIT(ime_3dnr_me_switch_th2,        8)
REGDEF_BIT(ime_3dnr_me_switch_th3,        8)
REGDEF_END(IME_3DNR_MOTION_ESTIMATION_CONTROL_REGISTER3)


/*
    ime_3dnr_me_switch_th4:    [0x0, 0xff],         bits : 7_0
    ime_3dnr_me_switch_th5:    [0x0, 0xff],         bits : 15_8
    ime_3dnr_me_switch_th6:    [0x0, 0xff],         bits : 23_16
    ime_3dnr_me_switch_th7:    [0x0, 0xff],         bits : 31_24
*/
#define IME_3DNR_MOTION_ESTIMATION_CONTROL_REGISTER4_OFS 0x0638
REGDEF_BEGIN(IME_3DNR_MOTION_ESTIMATION_CONTROL_REGISTER4)
REGDEF_BIT(ime_3dnr_me_switch_th4,        8)
REGDEF_BIT(ime_3dnr_me_switch_th5,        8)
REGDEF_BIT(ime_3dnr_me_switch_th6,        8)
REGDEF_BIT(ime_3dnr_me_switch_th7,        8)
REGDEF_END(IME_3DNR_MOTION_ESTIMATION_CONTROL_REGISTER4)


/*
    ime_3dnr_me_switch_ratio:    [0x0, 0xff],           bits : 7_0
    ime_3dnr_me_cost_blend  :    [0x0, 0xf],            bits : 11_8
*/
#define IME_3DNR_MOTION_ESTIMATION_CONTROL_REGISTER5_OFS 0x063c
REGDEF_BEGIN(IME_3DNR_MOTION_ESTIMATION_CONTROL_REGISTER5)
REGDEF_BIT(ime_3dnr_me_switch_ratio,        8)
REGDEF_BIT(ime_3dnr_me_cost_blend,        4)
REGDEF_END(IME_3DNR_MOTION_ESTIMATION_CONTROL_REGISTER5)


/*
    ime_3dnr_me_detail_penalty0:    [0x0, 0xf],         bits : 3_0
    ime_3dnr_me_detail_penalty1:    [0x0, 0xf],         bits : 7_4
    ime_3dnr_me_detail_penalty2:    [0x0, 0xf],         bits : 11_8
    ime_3dnr_me_detail_penalty3:    [0x0, 0xf],         bits : 15_12
    ime_3dnr_me_detail_penalty4:    [0x0, 0xf],         bits : 19_16
    ime_3dnr_me_detail_penalty5:    [0x0, 0xf],         bits : 23_20
    ime_3dnr_me_detail_penalty6:    [0x0, 0xf],         bits : 27_24
    ime_3dnr_me_detail_penalty7:    [0x0, 0xf],         bits : 31_28
*/
#define IME_3DNR_MOTION_ESTIMATION_CONTROL_REGISTER6_OFS 0x0640
REGDEF_BEGIN(IME_3DNR_MOTION_ESTIMATION_CONTROL_REGISTER6)
REGDEF_BIT(ime_3dnr_me_detail_penalty0,        4)
REGDEF_BIT(ime_3dnr_me_detail_penalty1,        4)
REGDEF_BIT(ime_3dnr_me_detail_penalty2,        4)
REGDEF_BIT(ime_3dnr_me_detail_penalty3,        4)
REGDEF_BIT(ime_3dnr_me_detail_penalty4,        4)
REGDEF_BIT(ime_3dnr_me_detail_penalty5,        4)
REGDEF_BIT(ime_3dnr_me_detail_penalty6,        4)
REGDEF_BIT(ime_3dnr_me_detail_penalty7,        4)
REGDEF_END(IME_3DNR_MOTION_ESTIMATION_CONTROL_REGISTER6)


/*
    ime_3dnr_me_probability0:    [0x0, 0x1],            bits : 0
    ime_3dnr_me_probability1:    [0x0, 0x1],            bits : 1
    ime_3dnr_me_probability2:    [0x0, 0x1],            bits : 2
    ime_3dnr_me_probability3:    [0x0, 0x1],            bits : 3
    ime_3dnr_me_probability4:    [0x0, 0x1],            bits : 4
    ime_3dnr_me_probability5:    [0x0, 0x1],            bits : 5
    ime_3dnr_me_probability6:    [0x0, 0x1],            bits : 6
    ime_3dnr_me_probability7:    [0x0, 0x1],            bits : 7
    ime_3dnr_me_rand_bit_x  :    [0x0, 0x7],            bits : 10_8
    ime_3dnr_me_rand_bit_y  :    [0x0, 0x7],            bits : 14_12
    ime_3dnr_me_min_detail  :    [0x0, 0x3fff],         bits : 29_16
*/
#define IME_3DNR_MOTION_ESTIMATION_CONTROL_REGISTER7_OFS 0x0644
REGDEF_BEGIN(IME_3DNR_MOTION_ESTIMATION_CONTROL_REGISTER7)
REGDEF_BIT(ime_3dnr_me_probability0,        1)
REGDEF_BIT(ime_3dnr_me_probability1,        1)
REGDEF_BIT(ime_3dnr_me_probability2,        1)
REGDEF_BIT(ime_3dnr_me_probability3,        1)
REGDEF_BIT(ime_3dnr_me_probability4,        1)
REGDEF_BIT(ime_3dnr_me_probability5,        1)
REGDEF_BIT(ime_3dnr_me_probability6,        1)
REGDEF_BIT(ime_3dnr_me_probability7,        1)
REGDEF_BIT(ime_3dnr_me_rand_bit_x,        3)
REGDEF_BIT(,        1)
REGDEF_BIT(ime_3dnr_me_rand_bit_y,        3)
REGDEF_BIT(,        1)
REGDEF_BIT(ime_3dnr_me_min_detail,        14)
REGDEF_END(IME_3DNR_MOTION_ESTIMATION_CONTROL_REGISTER7)


/*
    ime_3dnr_me_periodic_th:    [0x0, 0x3fff],          bits : 13_0
*/
#define IME_3DNR_MOTION_ESTIMATION_CONTROL_REGISTER8_OFS 0x0648
REGDEF_BEGIN(IME_3DNR_MOTION_ESTIMATION_CONTROL_REGISTER8)
//REGDEF_BIT(ime_3dnr_me_periodic_th,        14)
REGDEF_BIT(reserved,        32)
REGDEF_END(IME_3DNR_MOTION_ESTIMATION_CONTROL_REGISTER8)


/*
    ime_3dnr_ne_sample_num_x:    [0x0, 0xfff],          bits : 11_0
    ime_3dnr_ne_sample_num_y:    [0x0, 0xfff],          bits : 27_16
*/
#define IME_3DNR_MOTION_ESTIMATION_CONTROL_REGISTER9_OFS 0x064c
REGDEF_BEGIN(IME_3DNR_MOTION_ESTIMATION_CONTROL_REGISTER9)
REGDEF_BIT(ime_3dnr_ne_sample_num_x,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(ime_3dnr_ne_sample_num_y,        12)
REGDEF_END(IME_3DNR_MOTION_ESTIMATION_CONTROL_REGISTER9)


/*
    ime_3dnr_ne_sample_start_x:    [0x0, 0xfff],            bits : 11_0
    ime_3dnr_ne_sample_start_y:    [0x0, 0xfff],            bits : 27_16
*/
#define IME_3DNR_MOTION_ESTIMATION_CONTROL_REGISTER10_OFS 0x0650
REGDEF_BEGIN(IME_3DNR_MOTION_ESTIMATION_CONTROL_REGISTER10)
REGDEF_BIT(ime_3dnr_ne_sample_start_x,        12)
REGDEF_BIT(,        4)
REGDEF_BIT(ime_3dnr_ne_sample_start_y,        12)
REGDEF_END(IME_3DNR_MOTION_ESTIMATION_CONTROL_REGISTER10)


/*
    ime_3dnr_fast_converge_sp  :    [0x0, 0xf],         bits : 3_0
    ime_3dnr_fast_converge_step:    [0x0, 0xf],         bits : 7_4
*/
#define IME_3DNR_FAST_CONVERGE_CONTROL_REGISTER0_OFS 0x0654
REGDEF_BEGIN(IME_3DNR_FAST_CONVERGE_CONTROL_REGISTER0)
REGDEF_BIT(ime_3dnr_fast_converge_sp,        4)
REGDEF_BIT(ime_3dnr_fast_converge_step,        4)
REGDEF_END(IME_3DNR_FAST_CONVERGE_CONTROL_REGISTER0)


/*
    reserved                :    [0x0, 0xffffffff],         bits : 31_0
*/
#define IME_RESERVED_120_OFS 0x0658
REGDEF_BEGIN(IME_RESERVED_120)
REGDEF_BIT(reserved,        32)
REGDEF_END(IME_RESERVED_120)


/*
    reserved                :    [0x0, 0xffffffff],         bits : 31_0
*/
#define IME_RESERVED_121_OFS 0x065c
REGDEF_BEGIN(IME_RESERVED_121)
REGDEF_BIT(reserved,        32)
REGDEF_END(IME_RESERVED_121)


/*
    ime_3dnr_md_sad_coef_a0:    [0x0, 0x3f],            bits : 5_0
    ime_3dnr_md_sad_coef_a1:    [0x0, 0x3f],            bits : 13_8
    ime_3dnr_md_sad_coef_a2:    [0x0, 0x3f],            bits : 21_16
    ime_3dnr_md_sad_coef_a3:    [0x0, 0x3f],            bits : 29_24
*/
#define IME_3DNR_MOTION_DETECTION_CONTROL_REGISTER0_OFS 0x0660
REGDEF_BEGIN(IME_3DNR_MOTION_DETECTION_CONTROL_REGISTER0)
REGDEF_BIT(ime_3dnr_md_sad_coef_a0,        6)
REGDEF_BIT(,        2)
REGDEF_BIT(ime_3dnr_md_sad_coef_a1,        6)
REGDEF_BIT(,        2)
REGDEF_BIT(ime_3dnr_md_sad_coef_a2,        6)
REGDEF_BIT(,        2)
REGDEF_BIT(ime_3dnr_md_sad_coef_a3,        6)
REGDEF_END(IME_3DNR_MOTION_DETECTION_CONTROL_REGISTER0)


/*
    ime_3dnr_md_sad_coef_a4:    [0x0, 0x3f],            bits : 5_0
    ime_3dnr_md_sad_coef_a5:    [0x0, 0x3f],            bits : 13_8
    ime_3dnr_md_sad_coef_a6:    [0x0, 0x3f],            bits : 21_16
    ime_3dnr_md_sad_coef_a7:    [0x0, 0x3f],            bits : 29_24
*/
#define IME_3DNR_MOTION_DETECTION_CONTROL_REGISTER1_OFS 0x0664
REGDEF_BEGIN(IME_3DNR_MOTION_DETECTION_CONTROL_REGISTER1)
REGDEF_BIT(ime_3dnr_md_sad_coef_a4,        6)
REGDEF_BIT(,        2)
REGDEF_BIT(ime_3dnr_md_sad_coef_a5,        6)
REGDEF_BIT(,        2)
REGDEF_BIT(ime_3dnr_md_sad_coef_a6,        6)
REGDEF_BIT(,        2)
REGDEF_BIT(ime_3dnr_md_sad_coef_a7,        6)
REGDEF_END(IME_3DNR_MOTION_DETECTION_CONTROL_REGISTER1)


/*
    ime_3dnr_md_sad_coef_b0:    [0x0, 0x3fff],          bits : 13_0
    ime_3dnr_md_sad_coef_b1:    [0x0, 0x3fff],          bits : 29_16
*/
#define IME_3DNR_MOTION_DETECTION_CONTROL_REGISTER2_OFS 0x0668
REGDEF_BEGIN(IME_3DNR_MOTION_DETECTION_CONTROL_REGISTER2)
REGDEF_BIT(ime_3dnr_md_sad_coef_b0,        14)
REGDEF_BIT(,        2)
REGDEF_BIT(ime_3dnr_md_sad_coef_b1,        14)
REGDEF_END(IME_3DNR_MOTION_DETECTION_CONTROL_REGISTER2)


/*
    ime_3dnr_md_sad_coef_b2:    [0x0, 0x3fff],          bits : 13_0
    ime_3dnr_md_sad_coef_b3:    [0x0, 0x3fff],          bits : 29_16
*/
#define IME_3DNR_MOTION_DETECTION_CONTROL_REGISTER3_OFS 0x066c
REGDEF_BEGIN(IME_3DNR_MOTION_DETECTION_CONTROL_REGISTER3)
REGDEF_BIT(ime_3dnr_md_sad_coef_b2,        14)
REGDEF_BIT(,        2)
REGDEF_BIT(ime_3dnr_md_sad_coef_b3,        14)
REGDEF_END(IME_3DNR_MOTION_DETECTION_CONTROL_REGISTER3)


/*
    ime_3dnr_md_sad_coef_b4:    [0x0, 0x3fff],          bits : 13_0
    ime_3dnr_md_sad_coef_b5:    [0x0, 0x3fff],          bits : 29_16
*/
#define IME_3DNR_MOTION_DETECTION_CONTROL_REGISTER4_OFS 0x0670
REGDEF_BEGIN(IME_3DNR_MOTION_DETECTION_CONTROL_REGISTER4)
REGDEF_BIT(ime_3dnr_md_sad_coef_b4,        14)
REGDEF_BIT(,        2)
REGDEF_BIT(ime_3dnr_md_sad_coef_b5,        14)
REGDEF_END(IME_3DNR_MOTION_DETECTION_CONTROL_REGISTER4)


/*
    ime_3dnr_md_sad_coef_b6:    [0x0, 0x3fff],          bits : 13_0
    ime_3dnr_md_sad_coef_b7:    [0x0, 0x3fff],          bits : 29_16
*/
#define IME_3DNR_MOTION_DETECTION_CONTROL_REGISTER5_OFS 0x0674
REGDEF_BEGIN(IME_3DNR_MOTION_DETECTION_CONTROL_REGISTER5)
REGDEF_BIT(ime_3dnr_md_sad_coef_b6,        14)
REGDEF_BIT(,        2)
REGDEF_BIT(ime_3dnr_md_sad_coef_b7,        14)
REGDEF_END(IME_3DNR_MOTION_DETECTION_CONTROL_REGISTER5)


/*
    ime_3dnr_md_sad_std0:    [0x0, 0x3fff],         bits : 13_0
    ime_3dnr_md_sad_std1:    [0x0, 0x3fff],         bits : 29_16
*/
#define IME_3DNR_MOTION_DETECTION_CONTROL_REGISTER6_OFS 0x0678
REGDEF_BEGIN(IME_3DNR_MOTION_DETECTION_CONTROL_REGISTER6)
REGDEF_BIT(ime_3dnr_md_sad_std0,        14)
REGDEF_BIT(,        2)
REGDEF_BIT(ime_3dnr_md_sad_std1,        14)
REGDEF_END(IME_3DNR_MOTION_DETECTION_CONTROL_REGISTER6)


/*
    ime_3dnr_md_sad_std2:    [0x0, 0x3fff],         bits : 13_0
    ime_3dnr_md_sad_std3:    [0x0, 0x3fff],         bits : 29_16
*/
#define IME_3DNR_MOTION_DETECTION_CONTROL_REGISTER7_OFS 0x067c
REGDEF_BEGIN(IME_3DNR_MOTION_DETECTION_CONTROL_REGISTER7)
REGDEF_BIT(ime_3dnr_md_sad_std2,        14)
REGDEF_BIT(,        2)
REGDEF_BIT(ime_3dnr_md_sad_std3,        14)
REGDEF_END(IME_3DNR_MOTION_DETECTION_CONTROL_REGISTER7)


/*
    ime_3dnr_md_sad_std4:    [0x0, 0x3fff],         bits : 13_0
    ime_3dnr_md_sad_std5:    [0x0, 0x3fff],         bits : 29_16
*/
#define IME_3DNR_MOTION_DETECTION_CONTROL_REGISTER8_OFS 0x0680
REGDEF_BEGIN(IME_3DNR_MOTION_DETECTION_CONTROL_REGISTER8)
REGDEF_BIT(ime_3dnr_md_sad_std4,        14)
REGDEF_BIT(,        2)
REGDEF_BIT(ime_3dnr_md_sad_std5,        14)
REGDEF_END(IME_3DNR_MOTION_DETECTION_CONTROL_REGISTER8)


/*
    ime_3dnr_md_sad_std6:    [0x0, 0x3fff],         bits : 13_0
    ime_3dnr_md_sad_std7:    [0x0, 0x3fff],         bits : 29_16
*/
#define IME_3DNR_MOTION_DETECTION_CONTROL_REGISTER9_OFS 0x0684
REGDEF_BEGIN(IME_3DNR_MOTION_DETECTION_CONTROL_REGISTER9)
REGDEF_BIT(ime_3dnr_md_sad_std6,        14)
REGDEF_BIT(,        2)
REGDEF_BIT(ime_3dnr_md_sad_std7,        14)
REGDEF_END(IME_3DNR_MOTION_DETECTION_CONTROL_REGISTER9)


/*
    ime_3dnr_md_k1:    [0x0, 0x3f],         bits : 5_0
    ime_3dnr_md_k2:    [0x0, 0x3f],         bits : 13_8
*/
#define IME_3DNR_MOTION_DETECTION_CONTROL_REGISTER10_OFS 0x0688
REGDEF_BEGIN(IME_3DNR_MOTION_DETECTION_CONTROL_REGISTER10)
REGDEF_BIT(ime_3dnr_md_k1,        6)
REGDEF_BIT(,        2)
REGDEF_BIT(ime_3dnr_md_k2,        6)
REGDEF_END(IME_3DNR_MOTION_DETECTION_CONTROL_REGISTER10)


/*
    reserved                :    [0x0, 0xffffffff],         bits : 31_0
*/
#define IME_RESERVED_122_OFS 0x068c
REGDEF_BEGIN(IME_RESERVED_122)
REGDEF_BIT(reserved,        32)
REGDEF_END(IME_RESERVED_122)


/*
    reserved                :    [0x0, 0xffffffff],         bits : 31_0
*/
#define IME_RESERVED_123_OFS 0x0690
REGDEF_BEGIN(IME_RESERVED_123)
REGDEF_BIT(reserved,        32)
REGDEF_END(IME_RESERVED_123)


/*
    reserved                :    [0x0, 0xffffffff],         bits : 31_0
*/
#define IME_RESERVED_124_OFS 0x0694
REGDEF_BEGIN(IME_RESERVED_124)
REGDEF_BIT(reserved,        32)
REGDEF_END(IME_RESERVED_124)


/*
    reserved                :    [0x0, 0xffffffff],         bits : 31_0
*/
#define IME_RESERVED_125_OFS 0x0698
REGDEF_BEGIN(IME_RESERVED_125)
REGDEF_BIT(reserved,        32)
REGDEF_END(IME_RESERVED_125)


/*
    reserved                :    [0x0, 0xffffffff],         bits : 31_0
*/
#define IME_RESERVED_126_OFS 0x069c
REGDEF_BEGIN(IME_RESERVED_126)
REGDEF_BIT(reserved,        32)
REGDEF_END(IME_RESERVED_126)


/*
    ime_3dnr_mc_sad_base0:    [0x0, 0x3fff],            bits : 13_0
    ime_3dnr_mc_sad_base1:    [0x0, 0x3fff],            bits : 29_16
*/
#define IME_3DNR_MOTION_COMPENSATION_CONTROL_REGISTER0_OFS 0x06a0
REGDEF_BEGIN(IME_3DNR_MOTION_COMPENSATION_CONTROL_REGISTER0)
REGDEF_BIT(ime_3dnr_mc_sad_base0,        14)
REGDEF_BIT(,        2)
REGDEF_BIT(ime_3dnr_mc_sad_base1,        14)
REGDEF_END(IME_3DNR_MOTION_COMPENSATION_CONTROL_REGISTER0)


/*
    ime_3dnr_mc_sad_base2:    [0x0, 0x3fff],            bits : 13_0
    ime_3dnr_mc_sad_base3:    [0x0, 0x3fff],            bits : 29_16
*/
#define IME_3DNR_MOTION_COMPENSATION_CONTROL_REGISTER1_OFS 0x06a4
REGDEF_BEGIN(IME_3DNR_MOTION_COMPENSATION_CONTROL_REGISTER1)
REGDEF_BIT(ime_3dnr_mc_sad_base2,        14)
REGDEF_BIT(,        2)
REGDEF_BIT(ime_3dnr_mc_sad_base3,        14)
REGDEF_END(IME_3DNR_MOTION_COMPENSATION_CONTROL_REGISTER1)


/*
    ime_3dnr_mc_sad_base4:    [0x0, 0x3fff],            bits : 13_0
    ime_3dnr_mc_sad_base5:    [0x0, 0x3fff],            bits : 29_16
*/
#define IME_3DNR_MOTION_COMPENSATION_CONTROL_REGISTER2_OFS 0x06a8
REGDEF_BEGIN(IME_3DNR_MOTION_COMPENSATION_CONTROL_REGISTER2)
REGDEF_BIT(ime_3dnr_mc_sad_base4,        14)
REGDEF_BIT(,        2)
REGDEF_BIT(ime_3dnr_mc_sad_base5,        14)
REGDEF_END(IME_3DNR_MOTION_COMPENSATION_CONTROL_REGISTER2)


/*
    ime_3dnr_mc_sad_base6:    [0x0, 0x3fff],            bits : 13_0
    ime_3dnr_mc_sad_base7:    [0x0, 0x3fff],            bits : 29_16
*/
#define IME_3DNR_MOTION_COMPENSATION_CONTROL_REGISTER3_OFS 0x06ac
REGDEF_BEGIN(IME_3DNR_MOTION_COMPENSATION_CONTROL_REGISTER3)
REGDEF_BIT(ime_3dnr_mc_sad_base6,        14)
REGDEF_BIT(,        2)
REGDEF_BIT(ime_3dnr_mc_sad_base7,        14)
REGDEF_END(IME_3DNR_MOTION_COMPENSATION_CONTROL_REGISTER3)


/*
    ime_3dnr_mc_sad_coef_a0:    [0x0, 0x3f],            bits : 5_0
    ime_3dnr_mc_sad_coef_a1:    [0x0, 0x3f],            bits : 13_8
    ime_3dnr_mc_sad_coef_a2:    [0x0, 0x3f],            bits : 21_16
    ime_3dnr_mc_sad_coef_a3:    [0x0, 0x3f],            bits : 29_24
*/
#define IME_3DNR_MOTION_COMPENSATION_CONTROL_REGISTER4_OFS 0x06b0
REGDEF_BEGIN(IME_3DNR_MOTION_COMPENSATION_CONTROL_REGISTER4)
REGDEF_BIT(ime_3dnr_mc_sad_coef_a0,        6)
REGDEF_BIT(,        2)
REGDEF_BIT(ime_3dnr_mc_sad_coef_a1,        6)
REGDEF_BIT(,        2)
REGDEF_BIT(ime_3dnr_mc_sad_coef_a2,        6)
REGDEF_BIT(,        2)
REGDEF_BIT(ime_3dnr_mc_sad_coef_a3,        6)
REGDEF_END(IME_3DNR_MOTION_COMPENSATION_CONTROL_REGISTER4)


/*
    ime_3dnr_mc_sad_coef_a4:    [0x0, 0x3f],            bits : 5_0
    ime_3dnr_mc_sad_coef_a5:    [0x0, 0x3f],            bits : 13_8
    ime_3dnr_mc_sad_coef_a6:    [0x0, 0x3f],            bits : 21_16
    ime_3dnr_mc_sad_coef_a7:    [0x0, 0x3f],            bits : 29_24
*/
#define IME_3DNR_MOTION_COMPENSATION_CONTROL_REGISTER5_OFS 0x06b4
REGDEF_BEGIN(IME_3DNR_MOTION_COMPENSATION_CONTROL_REGISTER5)
REGDEF_BIT(ime_3dnr_mc_sad_coef_a4,        6)
REGDEF_BIT(,        2)
REGDEF_BIT(ime_3dnr_mc_sad_coef_a5,        6)
REGDEF_BIT(,        2)
REGDEF_BIT(ime_3dnr_mc_sad_coef_a6,        6)
REGDEF_BIT(,        2)
REGDEF_BIT(ime_3dnr_mc_sad_coef_a7,        6)
REGDEF_END(IME_3DNR_MOTION_COMPENSATION_CONTROL_REGISTER5)


/*
    ime_3dnr_mc_sad_coef_b0:    [0x0, 0x3fff],          bits : 13_0
    ime_3dnr_mc_sad_coef_b1:    [0x0, 0x3fff],          bits : 29_16
*/
#define IME_3DNR_MOTION_COMPENSATION_CONTROL_REGISTER6_OFS 0x06b8
REGDEF_BEGIN(IME_3DNR_MOTION_COMPENSATION_CONTROL_REGISTER6)
REGDEF_BIT(ime_3dnr_mc_sad_coef_b0,        14)
REGDEF_BIT(,        2)
REGDEF_BIT(ime_3dnr_mc_sad_coef_b1,        14)
REGDEF_END(IME_3DNR_MOTION_COMPENSATION_CONTROL_REGISTER6)


/*
    ime_3dnr_mc_sad_coef_b2:    [0x0, 0x3fff],          bits : 13_0
    ime_3dnr_mc_sad_coef_b3:    [0x0, 0x3fff],          bits : 29_16
*/
#define IME_3DNR_MOTION_COMPENSATION_CONTROL_REGISTER7_OFS 0x06bc
REGDEF_BEGIN(IME_3DNR_MOTION_COMPENSATION_CONTROL_REGISTER7)
REGDEF_BIT(ime_3dnr_mc_sad_coef_b2,        14)
REGDEF_BIT(,        2)
REGDEF_BIT(ime_3dnr_mc_sad_coef_b3,        14)
REGDEF_END(IME_3DNR_MOTION_COMPENSATION_CONTROL_REGISTER7)


/*
    ime_3dnr_mc_sad_coef_b4:    [0x0, 0x3fff],          bits : 13_0
    ime_3dnr_mc_sad_coef_b5:    [0x0, 0x3fff],          bits : 29_16
*/
#define IME_3DNR_MOTION_COMPENSATION_CONTROL_REGISTER8_OFS 0x06c0
REGDEF_BEGIN(IME_3DNR_MOTION_COMPENSATION_CONTROL_REGISTER8)
REGDEF_BIT(ime_3dnr_mc_sad_coef_b4,        14)
REGDEF_BIT(,        2)
REGDEF_BIT(ime_3dnr_mc_sad_coef_b5,        14)
REGDEF_END(IME_3DNR_MOTION_COMPENSATION_CONTROL_REGISTER8)


/*
    ime_3dnr_mc_sad_coef_b6:    [0x0, 0x3fff],          bits : 13_0
    ime_3dnr_mc_sad_coef_b7:    [0x0, 0x3fff],          bits : 29_16
*/
#define IME_3DNR_MOTION_COMPENSATION_CONTROL_REGISTER9_OFS 0x06c4
REGDEF_BEGIN(IME_3DNR_MOTION_COMPENSATION_CONTROL_REGISTER9)
REGDEF_BIT(ime_3dnr_mc_sad_coef_b6,        14)
REGDEF_BIT(,        2)
REGDEF_BIT(ime_3dnr_mc_sad_coef_b7,        14)
REGDEF_END(IME_3DNR_MOTION_COMPENSATION_CONTROL_REGISTER9)


/*
    ime_3dnr_mc_sad_std0:    [0x0, 0x3fff],         bits : 13_0
    ime_3dnr_mc_sad_std1:    [0x0, 0x3fff],         bits : 29_16
*/
#define IME_3DNR_MOTION_COMPENSATION_CONTROL_REGISTER10_OFS 0x06c8
REGDEF_BEGIN(IME_3DNR_MOTION_COMPENSATION_CONTROL_REGISTER10)
REGDEF_BIT(ime_3dnr_mc_sad_std0,        14)
REGDEF_BIT(,        2)
REGDEF_BIT(ime_3dnr_mc_sad_std1,        14)
REGDEF_END(IME_3DNR_MOTION_COMPENSATION_CONTROL_REGISTER10)


/*
    ime_3dnr_mc_sad_std2:    [0x0, 0x3fff],         bits : 13_0
    ime_3dnr_mc_sad_std3:    [0x0, 0x3fff],         bits : 29_16
*/
#define IME_3DNR_MOTION_COMPENSATION_CONTROL_REGISTER11_OFS 0x06cc
REGDEF_BEGIN(IME_3DNR_MOTION_COMPENSATION_CONTROL_REGISTER11)
REGDEF_BIT(ime_3dnr_mc_sad_std2,        14)
REGDEF_BIT(,        2)
REGDEF_BIT(ime_3dnr_mc_sad_std3,        14)
REGDEF_END(IME_3DNR_MOTION_COMPENSATION_CONTROL_REGISTER11)


/*
    ime_3dnr_mc_sad_std4:    [0x0, 0x3fff],         bits : 13_0
    ime_3dnr_mc_sad_std5:    [0x0, 0x3fff],         bits : 29_16
*/
#define IME_3DNR_MOTION_COMPENSATION_CONTROL_REGISTER12_OFS 0x06d0
REGDEF_BEGIN(IME_3DNR_MOTION_COMPENSATION_CONTROL_REGISTER12)
REGDEF_BIT(ime_3dnr_mc_sad_std4,        14)
REGDEF_BIT(,        2)
REGDEF_BIT(ime_3dnr_mc_sad_std5,        14)
REGDEF_END(IME_3DNR_MOTION_COMPENSATION_CONTROL_REGISTER12)


/*
    ime_3dnr_mc_sad_std6:    [0x0, 0x3fff],         bits : 13_0
    ime_3dnr_mc_sad_std7:    [0x0, 0x3fff],         bits : 29_16
*/
#define IME_3DNR_MOTION_COMPENSATION_CONTROL_REGISTER13_OFS 0x06d4
REGDEF_BEGIN(IME_3DNR_MOTION_COMPENSATION_CONTROL_REGISTER13)
REGDEF_BIT(ime_3dnr_mc_sad_std6,        14)
REGDEF_BIT(,        2)
REGDEF_BIT(ime_3dnr_mc_sad_std7,        14)
REGDEF_END(IME_3DNR_MOTION_COMPENSATION_CONTROL_REGISTER13)


/*
    ime_3dnr_mc_k1:    [0x0, 0x3f],         bits : 5_0
    ime_3dnr_mc_k2:    [0x0, 0x3f],         bits : 13_8
*/
#define IME_3DNR_MOTION_COMPENSATION_CONTROL_REGISTER14_OFS 0x06d8
REGDEF_BEGIN(IME_3DNR_MOTION_COMPENSATION_CONTROL_REGISTER14)
REGDEF_BIT(ime_3dnr_mc_k1,        6)
REGDEF_BIT(,        2)
REGDEF_BIT(ime_3dnr_mc_k2,        6)
REGDEF_END(IME_3DNR_MOTION_COMPENSATION_CONTROL_REGISTER14)


/*
    reserved                :    [0x0, 0xffffffff],         bits : 31_0
*/
#define IME_RESERVED_127_OFS 0x06dc
REGDEF_BEGIN(IME_RESERVED_127)
REGDEF_BIT(reserved,        32)
REGDEF_END(IME_RESERVED_127)


/*
    reserved                :    [0x0, 0xffffffff],         bits : 31_0
*/
#define IME_RESERVED_128_OFS 0x06e0
REGDEF_BEGIN(IME_RESERVED_128)
REGDEF_BIT(reserved,        32)
REGDEF_END(IME_RESERVED_128)


/*
    reserved                :    [0x0, 0xffffffff],         bits : 31_0
*/
#define IME_RESERVED_129_OFS 0x06e4
REGDEF_BEGIN(IME_RESERVED_129)
REGDEF_BIT(reserved,        32)
REGDEF_END(IME_RESERVED_129)


/*
    reserved                :    [0x0, 0xffffffff],         bits : 31_0
*/
#define IME_RESERVED_130_OFS 0x06e8
REGDEF_BEGIN(IME_RESERVED_130)
REGDEF_BIT(reserved,        32)
REGDEF_END(IME_RESERVED_130)


/*
    reserved                :    [0x0, 0xffffffff],         bits : 31_0
*/
#define IME_RESERVED_131_OFS 0x06ec
REGDEF_BEGIN(IME_RESERVED_131)
REGDEF_BIT(reserved,        32)
REGDEF_END(IME_RESERVED_131)


/*
    ime_3dnr_roi_md_k1:    [0x0, 0x3f],         bits : 5_0
    ime_3dnr_roi_md_k2:    [0x0, 0x3f],         bits : 13_8
*/
#define IME_3DNR_ROI_MOTION_DETECTION_CONTROL_REGISTER0_OFS 0x06f0
REGDEF_BEGIN(IME_3DNR_ROI_MOTION_DETECTION_CONTROL_REGISTER0)
REGDEF_BIT(ime_3dnr_roi_md_k1,        6)
REGDEF_BIT(,        2)
REGDEF_BIT(ime_3dnr_roi_md_k2,        6)
REGDEF_END(IME_3DNR_ROI_MOTION_DETECTION_CONTROL_REGISTER0)


/*
    reserved                :    [0x0, 0xffffffff],         bits : 31_0
*/
#define IME_RESERVED_132_OFS 0x06f4
REGDEF_BEGIN(IME_RESERVED_132)
REGDEF_BIT(reserved,        32)
REGDEF_END(IME_RESERVED_132)


/*
    reserved                :    [0x0, 0xffffffff],         bits : 31_0
*/
#define IME_RESERVED_133_OFS 0x06f8
REGDEF_BEGIN(IME_RESERVED_133)
REGDEF_BIT(reserved,        32)
REGDEF_END(IME_RESERVED_133)


/*
    reserved                :    [0x0, 0xffffffff],         bits : 31_0
*/
#define IME_RESERVED_134_OFS 0x06fc
REGDEF_BEGIN(IME_RESERVED_134)
REGDEF_BIT(reserved,        32)
REGDEF_END(IME_RESERVED_134)


/*
    reserved                :    [0x0, 0xffffffff],         bits : 31_0
*/
#define IME_RESERVED_135_OFS 0x0700
REGDEF_BEGIN(IME_RESERVED_135)
REGDEF_BIT(reserved,        32)
REGDEF_END(IME_RESERVED_135)


/*
    reserved                :    [0x0, 0xffffffff],         bits : 31_0
*/
#define IME_RESERVED_136_OFS 0x0704
REGDEF_BEGIN(IME_RESERVED_136)
REGDEF_BIT(reserved,        32)
REGDEF_END(IME_RESERVED_136)


/*
    reserved                :    [0x0, 0xffffffff],         bits : 31_0
*/
#define IME_RESERVED_137_OFS 0x0708
REGDEF_BEGIN(IME_RESERVED_137)
REGDEF_BIT(reserved,        32)
REGDEF_END(IME_RESERVED_137)


/*
    reserved                :    [0x0, 0xffffffff],         bits : 31_0
*/
#define IME_RESERVED_138_OFS 0x070c
REGDEF_BEGIN(IME_RESERVED_138)
REGDEF_BIT(reserved,        32)
REGDEF_END(IME_RESERVED_138)


/*
    ime_3dnr_roi_mc_k1:    [0x0, 0x3f],         bits : 5_0
    ime_3dnr_roi_mc_k2:    [0x0, 0x3f],         bits : 13_8
*/
#define IME_3DNR_ROI_MOTION_COMPENSATION_CONTROL_REGISTER0_OFS 0x0710
REGDEF_BEGIN(IME_3DNR_ROI_MOTION_COMPENSATION_CONTROL_REGISTER0)
REGDEF_BIT(ime_3dnr_roi_mc_k1,        6)
REGDEF_BIT(,        2)
REGDEF_BIT(ime_3dnr_roi_mc_k2,        6)
REGDEF_END(IME_3DNR_ROI_MOTION_COMPENSATION_CONTROL_REGISTER0)


/*
    ime_3dnr_nr_y_tf0_blur_str0:    [0x0, 0xff],            bits : 7_0
    ime_3dnr_nr_y_tf0_blur_str1:    [0x0, 0xff],            bits : 15_8
    ime_3dnr_nr_y_tf0_blur_str2:    [0x0, 0xff],            bits : 23_16
    ime_3dnr_nr_y_tf0_str0     :    [0x0, 0xff],            bits : 31_24
*/
#define IME_3DNR_TF0_REGISTER0_OFS 0x0714
REGDEF_BEGIN(IME_3DNR_TF0_REGISTER0)
REGDEF_BIT(ime_3dnr_nr_y_tf0_blur_str0,        8)
REGDEF_BIT(ime_3dnr_nr_y_tf0_blur_str1,        8)
REGDEF_BIT(ime_3dnr_nr_y_tf0_blur_str2,        8)
REGDEF_BIT(ime_3dnr_nr_y_tf0_str0,        8)
REGDEF_END(IME_3DNR_TF0_REGISTER0)


/*
    ime_3dnr_nr_y_tf0_str1:    [0x0, 0xff],         bits : 7_0
    ime_3dnr_nr_y_tf0_str2:    [0x0, 0xff],         bits : 15_8
    ime_3dnr_nr_c_tf0_str0:    [0x0, 0xff],         bits : 23_16
    ime_3dnr_nr_c_tf0_str1:    [0x0, 0xff],         bits : 31_24
*/
#define IME_3DNR_TF0_REGISTER1_OFS 0x0718
REGDEF_BEGIN(IME_3DNR_TF0_REGISTER1)
REGDEF_BIT(ime_3dnr_nr_y_tf0_str1,        8)
REGDEF_BIT(ime_3dnr_nr_y_tf0_str2,        8)
REGDEF_BIT(ime_3dnr_nr_c_tf0_str0,        8)
REGDEF_BIT(ime_3dnr_nr_c_tf0_str1,        8)
REGDEF_END(IME_3DNR_TF0_REGISTER1)


/*
    ime_3dnr_nr_c_tf0_str2:    [0x0, 0xff],         bits : 7_0
*/
#define IME_3DNR_TF0_REGISTER2_OFS 0x071c
REGDEF_BEGIN(IME_3DNR_TF0_REGISTER2)
REGDEF_BIT(ime_3dnr_nr_c_tf0_str2,        8)
REGDEF_END(IME_3DNR_TF0_REGISTER2)


/*
    reserved                :    [0x0, 0xffffffff],         bits : 31_0
*/
#define IME_RESERVED_142_OFS 0x0720
REGDEF_BEGIN(IME_RESERVED_142)
REGDEF_BIT(reserved,        32)
REGDEF_END(IME_RESERVED_142)


/*
    reserved                :    [0x0, 0xffffffff],         bits : 31_0
*/
#define IME_RESERVED_143_OFS 0x0724
REGDEF_BEGIN(IME_RESERVED_143)
REGDEF_BIT(reserved,        32)
REGDEF_END(IME_RESERVED_143)


/*
    reserved                :    [0x0, 0xffffffff],         bits : 31_0
*/
#define IME_RESERVED_144_OFS 0x0728
REGDEF_BEGIN(IME_RESERVED_144)
REGDEF_BIT(reserved,        32)
REGDEF_END(IME_RESERVED_144)


/*
    reserved                :    [0x0, 0xffffffff],         bits : 31_0
*/
#define IME_RESERVED_145_OFS 0x072c
REGDEF_BEGIN(IME_RESERVED_145)
REGDEF_BIT(reserved,        32)
REGDEF_END(IME_RESERVED_145)


/*
    ime_3dnr_ps_mv_th    :    [0x0, 0x3f],          bits : 5_0
    ime_3dnr_ps_roi_mv_th:    [0x0, 0x3f],          bits : 13_8
    ime_3dnr_ps_fs_th    :    [0x0, 0x3fff],            bits : 29_16
*/
#define IME_3DNR_PS_CONTROL_REGISTER0_OFS 0x0730
REGDEF_BEGIN(IME_3DNR_PS_CONTROL_REGISTER0)
REGDEF_BIT(ime_3dnr_ps_mv_th,        6)
REGDEF_BIT(,        2)
REGDEF_BIT(ime_3dnr_ps_roi_mv_th,        6)
REGDEF_BIT(,        2)
REGDEF_BIT(ime_3dnr_ps_fs_th,        14)
REGDEF_END(IME_3DNR_PS_CONTROL_REGISTER0)


/*
    ime_3dnr_ps_mix_ratio0:    [0x0, 0xff],         bits : 7_0
    ime_3dnr_ps_mix_ratio1:    [0x0, 0xff],         bits : 15_8
*/
#define IME_3DNR_PS_CONTROL_REGISTER1_OFS 0x0734
REGDEF_BEGIN(IME_3DNR_PS_CONTROL_REGISTER1)
REGDEF_BIT(ime_3dnr_ps_mix_ratio0,        8)
REGDEF_BIT(ime_3dnr_ps_mix_ratio1,        8)
REGDEF_END(IME_3DNR_PS_CONTROL_REGISTER1)


/*
    ime_3dnr_ps_mix_th0:    [0x0, 0x3fff],          bits : 13_0
    ime_3dnr_ps_mix_th1:    [0x0, 0x3fff],          bits : 29_16
*/
#define IME_3DNR_PS_CONTROL_REGISTER2_OFS 0x0738
REGDEF_BEGIN(IME_3DNR_PS_CONTROL_REGISTER2)
REGDEF_BIT(ime_3dnr_ps_mix_th0,        14)
REGDEF_BIT(,        2)
REGDEF_BIT(ime_3dnr_ps_mix_th1,        14)
REGDEF_END(IME_3DNR_PS_CONTROL_REGISTER2)


/*
    ime_3dnr_ps_mix_slope0:    [0x0, 0xffff],           bits : 15_0
    ime_3dnr_ps_mix_slope1:    [0x0, 0xffff],           bits : 31_16
*/
#define IME_3DNR_PS_CONTROL_REGISTER3_OFS 0x073c
REGDEF_BEGIN(IME_3DNR_PS_CONTROL_REGISTER3)
REGDEF_BIT(ime_3dnr_ps_mix_slope0,        16)
REGDEF_BIT(ime_3dnr_ps_mix_slope1,        16)
REGDEF_END(IME_3DNR_PS_CONTROL_REGISTER3)


/*
    reserved                :    [0x0, 0xffffffff],         bits : 31_0
*/
#define IME_RESERVED_146_OFS 0x0740
REGDEF_BEGIN(IME_RESERVED_146)
REGDEF_BIT(reserved,        32)
REGDEF_END(IME_RESERVED_146)


/*
    reserved                :    [0x0, 0xffffffff],         bits : 31_0
*/
#define IME_RESERVED_147_OFS 0x0744
REGDEF_BEGIN(IME_RESERVED_147)
REGDEF_BIT(reserved,        32)
REGDEF_END(IME_RESERVED_147)


/*
    reserved                :    [0x0, 0xffffffff],         bits : 31_0
*/
#define IME_RESERVED_148_OFS 0x0748
REGDEF_BEGIN(IME_RESERVED_148)
REGDEF_BIT(reserved,        32)
REGDEF_END(IME_RESERVED_148)


/*
    reserved                :    [0x0, 0xffffffff],         bits : 31_0
*/
#define IME_RESERVED_149_OFS 0x074c
REGDEF_BEGIN(IME_RESERVED_149)
REGDEF_BIT(reserved,        32)
REGDEF_END(IME_RESERVED_149)


/*
    reserved                :    [0x0, 0xffffffff],         bits : 31_0
*/
#define IME_RESERVED_150_OFS 0x0750
REGDEF_BEGIN(IME_RESERVED_150)
REGDEF_BIT(reserved,        32)
REGDEF_END(IME_RESERVED_150)


/*
    reserved                :    [0x0, 0xffffffff],         bits : 31_0
*/
#define IME_RESERVED_151_OFS 0x0754
REGDEF_BEGIN(IME_RESERVED_151)
REGDEF_BIT(reserved,        32)
REGDEF_END(IME_RESERVED_151)


/*
    reserved                :    [0x0, 0xffffffff],         bits : 31_0
*/
#define IME_RESERVED_152_OFS 0x0758
REGDEF_BEGIN(IME_RESERVED_152)
REGDEF_BIT(reserved,        32)
REGDEF_END(IME_RESERVED_152)


/*
    reserved                :    [0x0, 0xffffffff],         bits : 31_0
*/
#define IME_RESERVED_153_OFS 0x075c
REGDEF_BEGIN(IME_RESERVED_153)
REGDEF_BIT(reserved,        32)
REGDEF_END(IME_RESERVED_153)


/*
    ime_3dnr_ps_ds_th    :    [0x0, 0x1f],          bits : 4_0
    ime_3dnr_ps_ds_th_roi:    [0x0, 0x1f],          bits : 12_8
    ime_3dnr_ps_edge_w   :    [0x0, 0xff],          bits : 23_16
*/
#define IME_3DNR_PS_CONTROL_REGISTER4_OFS 0x0760
REGDEF_BEGIN(IME_3DNR_PS_CONTROL_REGISTER4)
REGDEF_BIT(ime_3dnr_ps_ds_th,        5)
REGDEF_BIT(,        3)
REGDEF_BIT(ime_3dnr_ps_ds_th_roi,        5)
REGDEF_BIT(,        3)
REGDEF_BIT(ime_3dnr_ps_edge_w,        8)
REGDEF_END(IME_3DNR_PS_CONTROL_REGISTER4)


/*
    ime_3dnr_ps_edge_th0:    [0x0, 0x3fff],         bits : 13_0
    ime_3dnr_ps_edge_th1:    [0x0, 0x3fff],         bits : 29_16
*/
#define IME_3DNR_PS_CONTROL_REGISTER5_OFS 0x0764
REGDEF_BEGIN(IME_3DNR_PS_CONTROL_REGISTER5)
REGDEF_BIT(ime_3dnr_ps_edge_th0,        14)
REGDEF_BIT(,        2)
REGDEF_BIT(ime_3dnr_ps_edge_th1,        14)
REGDEF_END(IME_3DNR_PS_CONTROL_REGISTER5)


/*
    ime_3dnr_ps_edge_slope:    [0x0, 0xffff],           bits : 15_0
*/
#define IME_3DNR_PS_CONTROL_REGISTER6_OFS 0x0768
REGDEF_BEGIN(IME_3DNR_PS_CONTROL_REGISTER6)
REGDEF_BIT(ime_3dnr_ps_edge_slope,        16)
REGDEF_END(IME_3DNR_PS_CONTROL_REGISTER6)


/*
    ime_3dnr_nr_residue_th_y0:    [0x0, 0xf],           bits : 3_0
    ime_3dnr_nr_residue_th_y1:    [0x0, 0xf],           bits : 11_8
    ime_3dnr_nr_residue_th_y2:    [0x0, 0xf],           bits : 19_16
    ime_3dnr_nr_residue_th_c :    [0x0, 0xf],           bits : 27_24
*/
#define IME_3DNR_NR_CONTROL_REGISTER0_OFS 0x076c
REGDEF_BEGIN(IME_3DNR_NR_CONTROL_REGISTER0)
REGDEF_BIT(ime_3dnr_nr_residue_th_y0,        4)
REGDEF_BIT(,        4)
REGDEF_BIT(ime_3dnr_nr_residue_th_y1,        4)
REGDEF_BIT(,        4)
REGDEF_BIT(ime_3dnr_nr_residue_th_y2,        4)
REGDEF_BIT(,        4)
REGDEF_BIT(ime_3dnr_nr_residue_th_c,        4)
REGDEF_END(IME_3DNR_NR_CONTROL_REGISTER0)


/*
    ime_3dnr_nr_freq_w0:    [0x0, 0xff],            bits : 7_0
    ime_3dnr_nr_freq_w1:    [0x0, 0xff],            bits : 15_8
    ime_3dnr_nr_freq_w2:    [0x0, 0xff],            bits : 23_16
    ime_3dnr_nr_freq_w3:    [0x0, 0xff],            bits : 31_24
*/
#define IME_3DNR_NR_CONTROL_REGISTER1_OFS 0x0770
REGDEF_BEGIN(IME_3DNR_NR_CONTROL_REGISTER1)
REGDEF_BIT(ime_3dnr_nr_freq_w0,        8)
REGDEF_BIT(ime_3dnr_nr_freq_w1,        8)
REGDEF_BIT(ime_3dnr_nr_freq_w2,        8)
REGDEF_BIT(ime_3dnr_nr_freq_w3,        8)
REGDEF_END(IME_3DNR_NR_CONTROL_REGISTER1)


/*
    reserved                :    [0x0, 0xffffffff],         bits : 31_0
*/
#define IME_RESERVED_154_OFS 0x0774
REGDEF_BEGIN(IME_RESERVED_154)
REGDEF_BIT(reserved,        32)
REGDEF_END(IME_RESERVED_154)


/*
    reserved                :    [0x0, 0xffffffff],         bits : 31_0
*/
#define IME_RESERVED_155_OFS 0x0778
REGDEF_BEGIN(IME_RESERVED_155)
REGDEF_BIT(reserved,        32)
REGDEF_END(IME_RESERVED_155)


/*
    reserved                :    [0x0, 0xffffffff],         bits : 31_0
*/
#define IME_RESERVED_156_OFS 0x077c
REGDEF_BEGIN(IME_RESERVED_156)
REGDEF_BIT(reserved,        32)
REGDEF_END(IME_RESERVED_156)


/*
    reserved                :    [0x0, 0xffffffff],         bits : 31_0
*/
#define IME_RESERVED_157_OFS 0x0780
REGDEF_BEGIN(IME_RESERVED_157)
REGDEF_BIT(reserved,        32)
REGDEF_END(IME_RESERVED_157)


/*
    reserved                :    [0x0, 0xffffffff],         bits : 31_0
*/
#define IME_RESERVED_158_OFS 0x0784
REGDEF_BEGIN(IME_RESERVED_158)
REGDEF_BIT(reserved,        32)
REGDEF_END(IME_RESERVED_158)


/*
    reserved                :    [0x0, 0xffffffff],         bits : 31_0
*/
#define IME_RESERVED_159_OFS 0x0788
REGDEF_BEGIN(IME_RESERVED_159)
REGDEF_BIT(reserved,        32)
REGDEF_END(IME_RESERVED_159)


/*
    reserved                :    [0x0, 0xffffffff],         bits : 31_0
*/
#define IME_RESERVED_160_OFS 0x078c
REGDEF_BEGIN(IME_RESERVED_160)
REGDEF_BIT(reserved,        32)
REGDEF_END(IME_RESERVED_160)


/*
    ime_3dnr_nr_luma_w0:    [0x0, 0xff],            bits : 7_0
    ime_3dnr_nr_luma_w1:    [0x0, 0xff],            bits : 15_8
    ime_3dnr_nr_luma_w2:    [0x0, 0xff],            bits : 23_16
    ime_3dnr_nr_luma_w3:    [0x0, 0xff],            bits : 31_24
*/
#define IME_3DNR_NR_CONTROL_REGISTER3_OFS 0x0790
REGDEF_BEGIN(IME_3DNR_NR_CONTROL_REGISTER3)
REGDEF_BIT(ime_3dnr_nr_luma_w0,        8)
REGDEF_BIT(ime_3dnr_nr_luma_w1,        8)
REGDEF_BIT(ime_3dnr_nr_luma_w2,        8)
REGDEF_BIT(ime_3dnr_nr_luma_w3,        8)
REGDEF_END(IME_3DNR_NR_CONTROL_REGISTER3)


/*
    ime_3dnr_nr_luma_w4:    [0x0, 0xff],            bits : 7_0
    ime_3dnr_nr_luma_w5:    [0x0, 0xff],            bits : 15_8
    ime_3dnr_nr_luma_w6:    [0x0, 0xff],            bits : 23_16
    ime_3dnr_nr_luma_w7:    [0x0, 0xff],            bits : 31_24
*/
#define IME_3DNR_NR_CONTROL_REGISTER4_OFS 0x0794
REGDEF_BEGIN(IME_3DNR_NR_CONTROL_REGISTER4)
REGDEF_BIT(ime_3dnr_nr_luma_w4,        8)
REGDEF_BIT(ime_3dnr_nr_luma_w5,        8)
REGDEF_BIT(ime_3dnr_nr_luma_w6,        8)
REGDEF_BIT(ime_3dnr_nr_luma_w7,        8)
REGDEF_END(IME_3DNR_NR_CONTROL_REGISTER4)


/*
    reserved                :    [0x0, 0xffffffff],         bits : 31_0
*/
#define IME_RESERVED_161_OFS 0x0798
REGDEF_BEGIN(IME_RESERVED_161)
REGDEF_BIT(reserved,        32)
REGDEF_END(IME_RESERVED_161)


/*
    reserved                :    [0x0, 0xffffffff],         bits : 31_0
*/
#define IME_RESERVED_162_OFS 0x079c
REGDEF_BEGIN(IME_RESERVED_162)
REGDEF_BIT(reserved,        32)
REGDEF_END(IME_RESERVED_162)


/*
    reserved                :    [0x0, 0xffffffff],         bits : 31_0
*/
#define IME_RESERVED_163_OFS 0x07a0
REGDEF_BEGIN(IME_RESERVED_163)
REGDEF_BIT(reserved,        32)
REGDEF_END(IME_RESERVED_163)


/*
    reserved                :    [0x0, 0xffffffff],         bits : 31_0
*/
#define IME_RESERVED_164_OFS 0x07a4
REGDEF_BEGIN(IME_RESERVED_164)
REGDEF_BIT(reserved,        32)
REGDEF_END(IME_RESERVED_164)


/*
    reserved                :    [0x0, 0xffffffff],         bits : 31_0
*/
#define IME_RESERVED_165_OFS 0x07a8
REGDEF_BEGIN(IME_RESERVED_165)
REGDEF_BIT(reserved,        32)
REGDEF_END(IME_RESERVED_165)


/*
    reserved                :    [0x0, 0xffffffff],         bits : 31_0
*/
#define IME_RESERVED_166_OFS 0x07ac
REGDEF_BEGIN(IME_RESERVED_166)
REGDEF_BIT(reserved,        32)
REGDEF_END(IME_RESERVED_166)


/*
    ime_3dnr_nr_pre_filtering_str0:    [0x0, 0xff],         bits : 7_0
    ime_3dnr_nr_pre_filtering_str1:    [0x0, 0xff],         bits : 15_8
    ime_3dnr_nr_pre_filtering_str2:    [0x0, 0xff],         bits : 23_16
    ime_3dnr_nr_pre_filtering_str3:    [0x0, 0xff],         bits : 31_24
*/
#define IME_3DNR_NR_CONTROL_REGISTER6_OFS 0x07b0
REGDEF_BEGIN(IME_3DNR_NR_CONTROL_REGISTER6)
REGDEF_BIT(ime_3dnr_nr_pre_filtering_str0,        8)
REGDEF_BIT(ime_3dnr_nr_pre_filtering_str1,        8)
REGDEF_BIT(ime_3dnr_nr_pre_filtering_str2,        8)
REGDEF_BIT(ime_3dnr_nr_pre_filtering_str3,        8)
REGDEF_END(IME_3DNR_NR_CONTROL_REGISTER6)


/*
    reserved                :    [0x0, 0xffffffff],         bits : 31_0
*/
#define IME_RESERVED_167_OFS 0x07b4
REGDEF_BEGIN(IME_RESERVED_167)
REGDEF_BIT(reserved,        32)
REGDEF_END(IME_RESERVED_167)


/*
    reserved                :    [0x0, 0xffffffff],         bits : 31_0
*/
#define IME_RESERVED_168_OFS 0x07b8
REGDEF_BEGIN(IME_RESERVED_168)
REGDEF_BIT(reserved,        32)
REGDEF_END(IME_RESERVED_168)


/*
    reserved                :    [0x0, 0xffffffff],         bits : 31_0
*/
#define IME_RESERVED_169_OFS 0x07bc
REGDEF_BEGIN(IME_RESERVED_169)
REGDEF_BIT(reserved,        32)
REGDEF_END(IME_RESERVED_169)


/*
    ime_3dnr_nr_pre_filtering_ratio0:    [0x0, 0xff],           bits : 7_0
    ime_3dnr_nr_pre_filtering_ratio1:    [0x0, 0xff],           bits : 15_8
    ime_3dnr_nr_snr_str0            :    [0x0, 0xff],           bits : 23_16
    ime_3dnr_nr_snr_str1            :    [0x0, 0xff],           bits : 31_24
*/
#define IME_3DNR_NR_CONTROL_REGISTER8_OFS 0x07c0
REGDEF_BEGIN(IME_3DNR_NR_CONTROL_REGISTER8)
REGDEF_BIT(ime_3dnr_nr_pre_filtering_ratio0,        8)
REGDEF_BIT(ime_3dnr_nr_pre_filtering_ratio1,        8)
REGDEF_BIT(ime_3dnr_nr_snr_str0,        8)
REGDEF_BIT(ime_3dnr_nr_snr_str1,        8)
REGDEF_END(IME_3DNR_NR_CONTROL_REGISTER8)


/*
    ime_3dnr_nr_snr_str2:    [0x0, 0xff],           bits : 7_0
    ime_3dnr_nr_tnr_str0:    [0x0, 0xff],           bits : 15_8
    ime_3dnr_nr_tnr_str1:    [0x0, 0xff],           bits : 23_16
    ime_3dnr_nr_tnr_str2:    [0x0, 0xff],           bits : 31_24
*/
#define IME_3DNR_NR_CONTROL_REGISTER9_OFS 0x07c4
REGDEF_BEGIN(IME_3DNR_NR_CONTROL_REGISTER9)
REGDEF_BIT(ime_3dnr_nr_snr_str2,        8)
REGDEF_BIT(ime_3dnr_nr_tnr_str0,        8)
REGDEF_BIT(ime_3dnr_nr_tnr_str1,        8)
REGDEF_BIT(ime_3dnr_nr_tnr_str2,        8)
REGDEF_END(IME_3DNR_NR_CONTROL_REGISTER9)


/*
    ime_3dnr_nr_base_th_snr:    [0x0, 0xffff],          bits : 15_0
    ime_3dnr_nr_base_th_tnr:    [0x0, 0xffff],          bits : 31_16
*/
#define IME_3DNR_NR_CONTROL_REGISTER10_OFS 0x07c8
REGDEF_BEGIN(IME_3DNR_NR_CONTROL_REGISTER10)
REGDEF_BIT(ime_3dnr_nr_base_th_snr,        16)
REGDEF_BIT(ime_3dnr_nr_base_th_tnr,        16)
REGDEF_END(IME_3DNR_NR_CONTROL_REGISTER10)


/*
    ime_3dnr_nr_y_3d_ratio0:    [0x0, 0xff],            bits : 7_0
    ime_3dnr_nr_y_3d_ratio1:    [0x0, 0xff],            bits : 15_8
*/
#define IME_3DNR_NR_CONTROL_REGISTER11_OFS 0x07cc
REGDEF_BEGIN(IME_3DNR_NR_CONTROL_REGISTER11)
REGDEF_BIT(ime_3dnr_nr_y_3d_ratio0,        8)
REGDEF_BIT(ime_3dnr_nr_y_3d_ratio1,        8)
REGDEF_END(IME_3DNR_NR_CONTROL_REGISTER11)


/*
    ime_3dnr_nr_y_3d_lut0:    [0x0, 0x7f],          bits : 6_0
    ime_3dnr_nr_y_3d_lut1:    [0x0, 0x7f],          bits : 14_8
    ime_3dnr_nr_y_3d_lut2:    [0x0, 0x7f],          bits : 22_16
    ime_3dnr_nr_y_3d_lut3:    [0x0, 0x7f],          bits : 30_24
*/
#define IME_3DNR_NR_CONTROL_REGISTER12_OFS 0x07d0
REGDEF_BEGIN(IME_3DNR_NR_CONTROL_REGISTER12)
REGDEF_BIT(ime_3dnr_nr_y_3d_lut0,        7)
REGDEF_BIT(,        1)
REGDEF_BIT(ime_3dnr_nr_y_3d_lut1,        7)
REGDEF_BIT(,        1)
REGDEF_BIT(ime_3dnr_nr_y_3d_lut2,        7)
REGDEF_BIT(,        1)
REGDEF_BIT(ime_3dnr_nr_y_3d_lut3,        7)
REGDEF_END(IME_3DNR_NR_CONTROL_REGISTER12)


/*
    ime_3dnr_nr_y_3d_lut4:    [0x0, 0x7f],          bits : 6_0
    ime_3dnr_nr_y_3d_lut5:    [0x0, 0x7f],          bits : 14_8
    ime_3dnr_nr_y_3d_lut6:    [0x0, 0x7f],          bits : 22_16
    ime_3dnr_nr_y_3d_lut7:    [0x0, 0x7f],          bits : 30_24
*/
#define IME_3DNR_NR_CONTROL_REGISTER13_OFS 0x07d4
REGDEF_BEGIN(IME_3DNR_NR_CONTROL_REGISTER13)
REGDEF_BIT(ime_3dnr_nr_y_3d_lut4,        7)
REGDEF_BIT(,        1)
REGDEF_BIT(ime_3dnr_nr_y_3d_lut5,        7)
REGDEF_BIT(,        1)
REGDEF_BIT(ime_3dnr_nr_y_3d_lut6,        7)
REGDEF_BIT(,        1)
REGDEF_BIT(ime_3dnr_nr_y_3d_lut7,        7)
REGDEF_END(IME_3DNR_NR_CONTROL_REGISTER13)


/*
    ime_3dnr_nr_c_3d_lut0:    [0x0, 0x7f],          bits : 6_0
    ime_3dnr_nr_c_3d_lut1:    [0x0, 0x7f],          bits : 14_8
    ime_3dnr_nr_c_3d_lut2:    [0x0, 0x7f],          bits : 22_16
    ime_3dnr_nr_c_3d_lut3:    [0x0, 0x7f],          bits : 30_24
*/
#define IME_3DNR_NR_CONTROL_REGISTER14_OFS 0x07d8
REGDEF_BEGIN(IME_3DNR_NR_CONTROL_REGISTER14)
REGDEF_BIT(ime_3dnr_nr_c_3d_lut0,        7)
REGDEF_BIT(,        1)
REGDEF_BIT(ime_3dnr_nr_c_3d_lut1,        7)
REGDEF_BIT(,        1)
REGDEF_BIT(ime_3dnr_nr_c_3d_lut2,        7)
REGDEF_BIT(,        1)
REGDEF_BIT(ime_3dnr_nr_c_3d_lut3,        7)
REGDEF_END(IME_3DNR_NR_CONTROL_REGISTER14)


/*
    ime_3dnr_nr_c_3d_lut4:    [0x0, 0x7f],          bits : 6_0
    ime_3dnr_nr_c_3d_lut5:    [0x0, 0x7f],          bits : 14_8
    ime_3dnr_nr_c_3d_lut6:    [0x0, 0x7f],          bits : 22_16
    ime_3dnr_nr_c_3d_lut7:    [0x0, 0x7f],          bits : 30_24
*/
#define IME_3DNR_NR_CONTROL_REGISTER15_OFS 0x07dc
REGDEF_BEGIN(IME_3DNR_NR_CONTROL_REGISTER15)
REGDEF_BIT(ime_3dnr_nr_c_3d_lut4,        7)
REGDEF_BIT(,        1)
REGDEF_BIT(ime_3dnr_nr_c_3d_lut5,        7)
REGDEF_BIT(,        1)
REGDEF_BIT(ime_3dnr_nr_c_3d_lut6,        7)
REGDEF_BIT(,        1)
REGDEF_BIT(ime_3dnr_nr_c_3d_lut7,        7)
REGDEF_END(IME_3DNR_NR_CONTROL_REGISTER15)


/*
    ime_3dnr_nr_c_3d_ratio0:    [0x0, 0xff],            bits : 7_0
    ime_3dnr_nr_c_3d_ratio1:    [0x0, 0xff],            bits : 15_8
*/
#define IME_3DNR_NR_CONTROL_REGISTER16_OFS 0x07e0
REGDEF_BEGIN(IME_3DNR_NR_CONTROL_REGISTER16)
REGDEF_BIT(ime_3dnr_nr_c_3d_ratio0,        8)
REGDEF_BIT(ime_3dnr_nr_c_3d_ratio1,        8)
REGDEF_END(IME_3DNR_NR_CONTROL_REGISTER16)


/*
    reserved                :    [0x0, 0xffffffff],         bits : 31_0
*/
#define IME_RESERVED_170_OFS 0x07e4
REGDEF_BEGIN(IME_RESERVED_170)
REGDEF_BIT(reserved,        32)
REGDEF_END(IME_RESERVED_170)


/*
    reserved                :    [0x0, 0xffffffff],         bits : 31_0
*/
#define IME_RESERVED_171_OFS 0x07e8
REGDEF_BEGIN(IME_RESERVED_171)
REGDEF_BIT(reserved,        32)
REGDEF_END(IME_RESERVED_171)


/*
    reserved                :    [0x0, 0xffffffff],         bits : 31_0
*/
#define IME_RESERVED_172_OFS 0x07ec
REGDEF_BEGIN(IME_RESERVED_172)
REGDEF_BIT(reserved,        32)
REGDEF_END(IME_RESERVED_172)


/*
    reserved                :    [0x0, 0xffffffff],         bits : 31_0
*/
#define IME_RESERVED_173_OFS 0x07f0
REGDEF_BEGIN(IME_RESERVED_173)
REGDEF_BIT(reserved,        32)
REGDEF_END(IME_RESERVED_173)


/*
    reserved                :    [0x0, 0xffffffff],         bits : 31_0
*/
#define IME_RESERVED_174_OFS 0x07f4
REGDEF_BEGIN(IME_RESERVED_174)
REGDEF_BIT(reserved,        32)
REGDEF_END(IME_RESERVED_174)


/*
    reserved                :    [0x0, 0xffffffff],         bits : 31_0
*/
#define IME_RESERVED_175_OFS 0x07f8
REGDEF_BEGIN(IME_RESERVED_175)
REGDEF_BIT(reserved,        32)
REGDEF_END(IME_RESERVED_175)


/*
    reserved                :    [0x0, 0xffffffff],         bits : 31_0
*/
#define IME_RESERVED_176_OFS 0x07fc
REGDEF_BEGIN(IME_RESERVED_176)
REGDEF_BIT(reserved,        32)
REGDEF_END(IME_RESERVED_176)


/*
    ime_3dnr_ref_dram_y_ofsi:    [0x0, 0x3ffff],            bits : 19_2
*/
#define IME_3DNR_INPUT_REFERENCE_IMAGE_LINE_OFFSET_REGISTER0_OFS 0x0800
REGDEF_BEGIN(IME_3DNR_INPUT_REFERENCE_IMAGE_LINE_OFFSET_REGISTER0)
REGDEF_BIT(,        2)
REGDEF_BIT(ime_3dnr_ref_dram_y_ofsi,        18)
REGDEF_END(IME_3DNR_INPUT_REFERENCE_IMAGE_LINE_OFFSET_REGISTER0)


/*
    ime_3dnr_ref_dram_uv_ofsi:    [0x0, 0x3ffff],           bits : 19_2
*/
#define IME_3DNR_INPUT_REFERENCE_IMAGE_LINE_OFFSET_REGISTER1_OFS 0x0804
REGDEF_BEGIN(IME_3DNR_INPUT_REFERENCE_IMAGE_LINE_OFFSET_REGISTER1)
REGDEF_BIT(,        2)
REGDEF_BIT(ime_3dnr_ref_dram_uv_ofsi,        18)
REGDEF_END(IME_3DNR_INPUT_REFERENCE_IMAGE_LINE_OFFSET_REGISTER1)


/*
    ime_3dnr_ref_dram_y_sai:    [0x0, 0x3fffffff],          bits : 31_2
*/
#define IME_3DNR_INPUT_REFERENCE_IMAGE_DMA_STARTING_ADDRESS_REGISTER0_OFS 0x0808
REGDEF_BEGIN(IME_3DNR_INPUT_REFERENCE_IMAGE_DMA_STARTING_ADDRESS_REGISTER0)
REGDEF_BIT(,        2)
REGDEF_BIT(ime_3dnr_ref_dram_y_sai,        30)
REGDEF_END(IME_3DNR_INPUT_REFERENCE_IMAGE_DMA_STARTING_ADDRESS_REGISTER0)


/*
    ime_3dnr_ref_dram_uv_sai:    [0x0, 0x3fffffff],         bits : 31_2
*/
#define IME_3DNR_INPUT_REFERENCE_IMAGE_DMA_STARTING_ADDRESS_REGISTER1_OFS 0x080c
REGDEF_BEGIN(IME_3DNR_INPUT_REFERENCE_IMAGE_DMA_STARTING_ADDRESS_REGISTER1)
REGDEF_BIT(,        2)
REGDEF_BIT(ime_3dnr_ref_dram_uv_sai,        30)
REGDEF_END(IME_3DNR_INPUT_REFERENCE_IMAGE_DMA_STARTING_ADDRESS_REGISTER1)


/*
    ime_3dnr_ref_dram_y_ofso:    [0x0, 0x3ffff],            bits : 19_2
*/
#define IME_3DNR_OUTPUT_REFERENCE_IMAGE_LINE_OFFSET_REGISTER0_OFS 0x0810
REGDEF_BEGIN(IME_3DNR_OUTPUT_REFERENCE_IMAGE_LINE_OFFSET_REGISTER0)
REGDEF_BIT(,        2)
REGDEF_BIT(ime_3dnr_ref_dram_y_ofso,        18)
REGDEF_END(IME_3DNR_OUTPUT_REFERENCE_IMAGE_LINE_OFFSET_REGISTER0)


/*
    ime_3dnr_ref_dram_uv_ofso:    [0x0, 0x3ffff],           bits : 19_2
*/
#define IME_3DNR_OUTPUT_REFERENCE_IMAGE_LINE_OFFSET_REGISTER1_OFS 0x0814
REGDEF_BEGIN(IME_3DNR_OUTPUT_REFERENCE_IMAGE_LINE_OFFSET_REGISTER1)
REGDEF_BIT(,        2)
REGDEF_BIT(ime_3dnr_ref_dram_uv_ofso,        18)
REGDEF_END(IME_3DNR_OUTPUT_REFERENCE_IMAGE_LINE_OFFSET_REGISTER1)


/*
    ime_3dnr_ref_dram_y_sao:    [0x0, 0x3fffffff],          bits : 31_2
*/
#define IME_3DNR_OUTPUT_REFERENCE_IMAGE_DMA_STARTING_ADDRESS_REGISTER0_OFS 0x0818
REGDEF_BEGIN(IME_3DNR_OUTPUT_REFERENCE_IMAGE_DMA_STARTING_ADDRESS_REGISTER0)
REGDEF_BIT(,        2)
REGDEF_BIT(ime_3dnr_ref_dram_y_sao,        30)
REGDEF_END(IME_3DNR_OUTPUT_REFERENCE_IMAGE_DMA_STARTING_ADDRESS_REGISTER0)


/*
    ime_3dnr_ref_dram_uv_sao:    [0x0, 0x3fffffff],         bits : 31_2
*/
#define IME_3DNR_OUTPUT_REFERENCE_IMAGE_DMA_STARTING_ADDRESS_REGISTER1_OFS 0x081c
REGDEF_BEGIN(IME_3DNR_OUTPUT_REFERENCE_IMAGE_DMA_STARTING_ADDRESS_REGISTER1)
REGDEF_BIT(,        2)
REGDEF_BIT(ime_3dnr_ref_dram_uv_sao,        30)
REGDEF_END(IME_3DNR_OUTPUT_REFERENCE_IMAGE_DMA_STARTING_ADDRESS_REGISTER1)


/*
    ime_3dnr_mot_dram_ofsi:    [0x0, 0x3ffff],          bits : 19_2
*/
#define IME_3DNR_MOTION_STATUS_INPUT_DMA_LINEOFFSET_REGISTER0_OFS 0x0820
REGDEF_BEGIN(IME_3DNR_MOTION_STATUS_INPUT_DMA_LINEOFFSET_REGISTER0)
REGDEF_BIT(,        2)
REGDEF_BIT(ime_3dnr_mot_dram_ofsi,        18)
REGDEF_END(IME_3DNR_MOTION_STATUS_INPUT_DMA_LINEOFFSET_REGISTER0)


/*
    ime_3dnr_mot_dram_sai:    [0x0, 0x3fffffff],            bits : 31_2
*/
#define IME_3DNR_MOTION_STATUS_INPUT_DMA_STARTING_ADDRESS_REGISTER0_OFS 0x0824
REGDEF_BEGIN(IME_3DNR_MOTION_STATUS_INPUT_DMA_STARTING_ADDRESS_REGISTER0)
REGDEF_BIT(,        2)
REGDEF_BIT(ime_3dnr_mot_dram_sai,        30)
REGDEF_END(IME_3DNR_MOTION_STATUS_INPUT_DMA_STARTING_ADDRESS_REGISTER0)


/*
    ime_3dnr_mot_dram_ofso:    [0x0, 0x3ffff],          bits : 19_2
*/
#define IME_3DNR_MOTION_STATUS_OUTPUT_DMA_LINEOFFSET_REGISTER0_OFS 0x0828
REGDEF_BEGIN(IME_3DNR_MOTION_STATUS_OUTPUT_DMA_LINEOFFSET_REGISTER0)
REGDEF_BIT(,        2)
REGDEF_BIT(ime_3dnr_mot_dram_ofso,        18)
REGDEF_END(IME_3DNR_MOTION_STATUS_OUTPUT_DMA_LINEOFFSET_REGISTER0)


/*
    ime_3dnr_mot_dram_sao:    [0x0, 0x3fffffff],            bits : 31_2
*/
#define IME_3DNR_MOTION_STATUS_OUTPUT_DMA_STARTING_ADDRESS_REGISTER0_OFS 0x082c
REGDEF_BEGIN(IME_3DNR_MOTION_STATUS_OUTPUT_DMA_STARTING_ADDRESS_REGISTER0)
REGDEF_BIT(,        2)
REGDEF_BIT(ime_3dnr_mot_dram_sao,        30)
REGDEF_END(IME_3DNR_MOTION_STATUS_OUTPUT_DMA_STARTING_ADDRESS_REGISTER0)


/*
    ime_3dnr_mot_roi_dram_ofso:    [0x0, 0x3ffff],          bits : 19_2
*/
#define IME_3DNR_MOTION_STATUS_ROI_OUTPUT_DMA_LINEOFFSET_REGISTER0_OFS 0x0830
REGDEF_BEGIN(IME_3DNR_MOTION_STATUS_ROI_OUTPUT_DMA_LINEOFFSET_REGISTER0)
REGDEF_BIT(,        2)
REGDEF_BIT(ime_3dnr_mot_roi_dram_ofso,        18)
REGDEF_END(IME_3DNR_MOTION_STATUS_ROI_OUTPUT_DMA_LINEOFFSET_REGISTER0)


/*
    ime_3dnr_mot_roi_dram_sao:    [0x0, 0x3fffffff],            bits : 31_2
*/
#define IME_3DNR_MOTION_STATUS_ROI_OUTPUT_DMA_STARTING_ADDRESS_REGISTER0_OFS 0x0834
REGDEF_BEGIN(IME_3DNR_MOTION_STATUS_ROI_OUTPUT_DMA_STARTING_ADDRESS_REGISTER0)
REGDEF_BIT(,        2)
REGDEF_BIT(ime_3dnr_mot_roi_dram_sao,        30)
REGDEF_END(IME_3DNR_MOTION_STATUS_ROI_OUTPUT_DMA_STARTING_ADDRESS_REGISTER0)


/*
    ime_3dnr_mv_dram_ofsi:    [0x0, 0x3ffff],           bits : 19_2
*/
#define IME_3DNR_MOTION_VECTOR_INPUT_DMA_LINEOFFSET_REGISTER0_OFS 0x0838
REGDEF_BEGIN(IME_3DNR_MOTION_VECTOR_INPUT_DMA_LINEOFFSET_REGISTER0)
REGDEF_BIT(,        2)
REGDEF_BIT(ime_3dnr_mv_dram_ofsi,        18)
REGDEF_END(IME_3DNR_MOTION_VECTOR_INPUT_DMA_LINEOFFSET_REGISTER0)


/*
    ime_3dnr_mv_dram_sai:    [0x0, 0x3fffffff],         bits : 31_2
*/
#define IME_3DNR_MOTION_VECTOR_INPUT_DMA_STARTING_ADDRESS_REGISTER0_OFS 0x083c
REGDEF_BEGIN(IME_3DNR_MOTION_VECTOR_INPUT_DMA_STARTING_ADDRESS_REGISTER0)
REGDEF_BIT(,        2)
REGDEF_BIT(ime_3dnr_mv_dram_sai,        30)
REGDEF_END(IME_3DNR_MOTION_VECTOR_INPUT_DMA_STARTING_ADDRESS_REGISTER0)


/*
    ime_3dnr_mv_dram_ofso:    [0x0, 0x3ffff],           bits : 19_2
*/
#define IME_3DNR_MOTION_VECTOR_OUTPUT_DMA_LINEOFFSET_REGISTER0_OFS 0x0840
REGDEF_BEGIN(IME_3DNR_MOTION_VECTOR_OUTPUT_DMA_LINEOFFSET_REGISTER0)
REGDEF_BIT(,        2)
REGDEF_BIT(ime_3dnr_mv_dram_ofso,        18)
REGDEF_END(IME_3DNR_MOTION_VECTOR_OUTPUT_DMA_LINEOFFSET_REGISTER0)


/*
    ime_3dnr_mv_dram_sao:    [0x0, 0x3fffffff],         bits : 31_2
*/
#define IME_3DNR_MOTION_VECTOR_OUTPUT_DMA_STARTING_ADDRESS_REGISTER0_OFS 0x0844
REGDEF_BEGIN(IME_3DNR_MOTION_VECTOR_OUTPUT_DMA_STARTING_ADDRESS_REGISTER0)
REGDEF_BIT(,        2)
REGDEF_BIT(ime_3dnr_mv_dram_sao,        30)
REGDEF_END(IME_3DNR_MOTION_VECTOR_OUTPUT_DMA_STARTING_ADDRESS_REGISTER0)


/*
    ime_3dnr_statistic_dram_ofso:    [0x0, 0x3ffff],            bits : 19_2
*/
#define IME_3DNR_STATISTIC_DATA_OUTPUT_DMA_LINEOFFSET_REGISTER0_OFS 0x0848
REGDEF_BEGIN(IME_3DNR_STATISTIC_DATA_OUTPUT_DMA_LINEOFFSET_REGISTER0)
REGDEF_BIT(,        2)
REGDEF_BIT(ime_3dnr_statistic_dram_ofso,        18)
REGDEF_END(IME_3DNR_STATISTIC_DATA_OUTPUT_DMA_LINEOFFSET_REGISTER0)


/*
    ime_3dnr_statistic_dram_sao:    [0x0, 0x3fffffff],          bits : 31_2
*/
#define IME_3DNR_STATISTIC_DATA_OUTPUT_DMA_STARTING_ADDRESS_REGISTER0_OFS 0x084c
REGDEF_BEGIN(IME_3DNR_STATISTIC_DATA_OUTPUT_DMA_STARTING_ADDRESS_REGISTER0)
REGDEF_BIT(,        2)
REGDEF_BIT(ime_3dnr_statistic_dram_sao,        30)
REGDEF_END(IME_3DNR_STATISTIC_DATA_OUTPUT_DMA_STARTING_ADDRESS_REGISTER0)


/*
    ime_3dnr_fast_converge_dram_sai:    [0x0, 0x3fffffff],          bits : 31_2
*/
#define IME_3DNR_FAST_CONVERGE_INPUT_DMA_ADDRESS_REGISTER0_OFS 0x0850
REGDEF_BEGIN(IME_3DNR_FAST_CONVERGE_INPUT_DMA_ADDRESS_REGISTER0)
REGDEF_BIT(,        2)
REGDEF_BIT(ime_3dnr_fast_converge_dram_sai,        30)
REGDEF_END(IME_3DNR_FAST_CONVERGE_INPUT_DMA_ADDRESS_REGISTER0)


/*
    ime_3dnr_fast_converge_dram_ofs:    [0x0, 0x3ffff],         bits : 19_2
*/
#define IME_3DNR_FAST_CONVERGE_DMA_LINEOFFSET_REGISTER0_OFS 0x0854
REGDEF_BEGIN(IME_3DNR_FAST_CONVERGE_DMA_LINEOFFSET_REGISTER0)
REGDEF_BIT(,        2)
REGDEF_BIT(ime_3dnr_fast_converge_dram_ofs,        18)
REGDEF_END(IME_3DNR_FAST_CONVERGE_DMA_LINEOFFSET_REGISTER0)


/*
    ime_3dnr_fast_converge_dram_sao:    [0x0, 0x3fffffff],          bits : 31_2
*/
#define IME_3DNR_FAST_CONVERGE_OUTPUT_DMA_ADDRESS_REGISTER0_OFS 0x0858
REGDEF_BEGIN(IME_3DNR_FAST_CONVERGE_OUTPUT_DMA_ADDRESS_REGISTER0)
REGDEF_BIT(,        2)
REGDEF_BIT(ime_3dnr_fast_converge_dram_sao,        30)
REGDEF_END(IME_3DNR_FAST_CONVERGE_OUTPUT_DMA_ADDRESS_REGISTER0)


/*
    reserved                :    [0x0, 0xffffffff],         bits : 31_0
*/
#define IME_RESERVED_180_OFS 0x085c
REGDEF_BEGIN(IME_RESERVED_180)
REGDEF_BIT(reserved,        32)
REGDEF_END(IME_RESERVED_180)


/*
    reserved                :    [0x0, 0xffffffff],         bits : 31_0
*/
#define IME_RESERVED_181_OFS 0x0860
REGDEF_BEGIN(IME_RESERVED_181)
REGDEF_BIT(reserved,        32)
REGDEF_END(IME_RESERVED_181)


/*
    reserved                :    [0x0, 0xffffffff],         bits : 31_0
*/
#define IME_RESERVED_182_OFS 0x0864
REGDEF_BEGIN(IME_RESERVED_182)
REGDEF_BIT(reserved,        32)
REGDEF_END(IME_RESERVED_182)


/*
    reserved                :    [0x0, 0xffffffff],         bits : 31_0
*/
#define IME_RESERVED_183_OFS 0x0868
REGDEF_BEGIN(IME_RESERVED_183)
REGDEF_BIT(reserved,        32)
REGDEF_END(IME_RESERVED_183)


/*
    reserved                :    [0x0, 0xffffffff],         bits : 31_0
*/
#define IME_RESERVED_184_OFS 0x086c
REGDEF_BEGIN(IME_RESERVED_184)
REGDEF_BIT(reserved,        32)
REGDEF_END(IME_RESERVED_184)


/*
    reserved                :    [0x0, 0xffffffff],         bits : 31_0
*/
#define IME_RESERVED_185_OFS 0x0870
REGDEF_BEGIN(IME_RESERVED_185)
REGDEF_BIT(reserved,        32)
REGDEF_END(IME_RESERVED_185)


/*
    reserved                :    [0x0, 0xffffffff],         bits : 31_0
*/
#define IME_RESERVED_186_OFS 0x0874
REGDEF_BEGIN(IME_RESERVED_186)
REGDEF_BIT(reserved,        32)
REGDEF_END(IME_RESERVED_186)


/*
    reserved                :    [0x0, 0xffffffff],         bits : 31_0
*/
#define IME_RESERVED_187_OFS 0x0878
REGDEF_BEGIN(IME_RESERVED_187)
REGDEF_BIT(reserved,        32)
REGDEF_END(IME_RESERVED_187)


/*
    reserved                :    [0x0, 0xffffffff],         bits : 31_0
*/
#define IME_RESERVED_188_OFS 0x087c
REGDEF_BEGIN(IME_RESERVED_188)
REGDEF_BIT(reserved,        32)
REGDEF_END(IME_RESERVED_188)


/*
    reserved                :    [0x0, 0xffffffff],         bits : 31_0
*/
#define IME_OUTPUT_PATH1_CONTROL_REGISTER12_OFS 0x0880
REGDEF_BEGIN(IME_OUTPUT_PATH1_CONTROL_REGISTER12)
REGDEF_BIT(reserved,        32)
REGDEF_END(IME_OUTPUT_PATH1_CONTROL_REGISTER12)


/*
    reserved                :    [0x0, 0xffffffff],         bits : 31_0
*/
#define IME_OUTPUT_PATH1_CONTROL_REGISTER13_OFS 0x0884
REGDEF_BEGIN(IME_OUTPUT_PATH1_CONTROL_REGISTER13)
REGDEF_BIT(reserved,        32)
REGDEF_END(IME_OUTPUT_PATH1_CONTROL_REGISTER13)


/*
    reserved                :    [0x0, 0xffffffff],         bits : 31_0
*/
#define IME_RESERVED_189_OFS 0x0888
REGDEF_BEGIN(IME_RESERVED_189)
REGDEF_BIT(reserved,        32)
REGDEF_END(IME_RESERVED_189)


/*
    reserved                :    [0x0, 0xffffffff],         bits : 31_0
*/
#define IME_RESERVED_190_OFS 0x088c
REGDEF_BEGIN(IME_RESERVED_190)
REGDEF_BIT(reserved,        32)
REGDEF_END(IME_RESERVED_190)


/*
    reserved                :    [0x0, 0xffffffff],         bits : 31_0
*/
#define IME_RESERVED_191_OFS 0x0890
REGDEF_BEGIN(IME_RESERVED_191)
REGDEF_BIT(reserved,        32)
REGDEF_END(IME_RESERVED_191)


/*
    reserved                :    [0x0, 0xffffffff],         bits : 31_0
*/
#define IME_RESERVED_192_OFS 0x0894
REGDEF_BEGIN(IME_RESERVED_192)
REGDEF_BIT(reserved,        32)
REGDEF_END(IME_RESERVED_192)


/*
    reserved                :    [0x0, 0xffffffff],         bits : 31_0
*/
#define IME_RESERVED_193_OFS 0x0898
REGDEF_BEGIN(IME_RESERVED_193)
REGDEF_BIT(reserved,        32)
REGDEF_END(IME_RESERVED_193)


/*
    reserved                :    [0x0, 0xffffffff],         bits : 31_0
*/
#define IME_RESERVED_194_OFS 0x089c
REGDEF_BEGIN(IME_RESERVED_194)
REGDEF_BIT(reserved,        32)
REGDEF_END(IME_RESERVED_194)


/*
    reserved                :    [0x0, 0xffffffff],         bits : 31_0
*/
#define IME_OUTPUT_PATH2_CONTROL_REGISTER31_OFS 0x08a0
REGDEF_BEGIN(IME_OUTPUT_PATH2_CONTROL_REGISTER31)
REGDEF_BIT(reserved,        32)
REGDEF_END(IME_OUTPUT_PATH2_CONTROL_REGISTER31)


/*
    reserved                :    [0x0, 0xffffffff],         bits : 31_0
*/
#define IME_OUTPUT_PATH2_CONTROL_REGISTER32_OFS 0x08a4
REGDEF_BEGIN(IME_OUTPUT_PATH2_CONTROL_REGISTER32)
REGDEF_BIT(reserved,        32)
REGDEF_END(IME_OUTPUT_PATH2_CONTROL_REGISTER32)


/*
    reserved                :    [0x0, 0xffffffff],         bits : 31_0
*/
#define IME_OUTPUT_PATH2_CONTROL_REGISTER33_OFS 0x08a8
REGDEF_BEGIN(IME_OUTPUT_PATH2_CONTROL_REGISTER33)
REGDEF_BIT(reserved,        32)
REGDEF_END(IME_OUTPUT_PATH2_CONTROL_REGISTER33)


/*
    reserved                :    [0x0, 0xffffffff],         bits : 31_0
*/
#define IME_OUTPUT_PATH2_CONTROL_REGISTER34_OFS 0x08ac
REGDEF_BEGIN(IME_OUTPUT_PATH2_CONTROL_REGISTER34)
REGDEF_BIT(reserved,        32)
REGDEF_END(IME_OUTPUT_PATH2_CONTROL_REGISTER34)


/*
    reserved                :    [0x0, 0xffffffff],         bits : 31_0
*/
#define IME_OUTPUT_PATH2_CONTROL_REGISTER35_OFS 0x08b0
REGDEF_BEGIN(IME_OUTPUT_PATH2_CONTROL_REGISTER35)
REGDEF_BIT(reserved,        32)
REGDEF_END(IME_OUTPUT_PATH2_CONTROL_REGISTER35)


/*
    reserved                :    [0x0, 0xffffffff],         bits : 31_0
*/
#define IME_OUTPUT_PATH2_CONTROL_REGISTER36_OFS 0x08b4
REGDEF_BEGIN(IME_OUTPUT_PATH2_CONTROL_REGISTER36)
REGDEF_BIT(reserved,        32)
REGDEF_END(IME_OUTPUT_PATH2_CONTROL_REGISTER36)


/*
    reserved                :    [0x0, 0xffffffff],         bits : 31_0
*/
#define IME_RESERVED_195_OFS 0x08b8
REGDEF_BEGIN(IME_RESERVED_195)
REGDEF_BIT(reserved,        32)
REGDEF_END(IME_RESERVED_195)


/*
    reserved                :    [0x0, 0xffffffff],         bits : 31_0
*/
#define IME_RESERVED_196_OFS 0x08bc
REGDEF_BEGIN(IME_RESERVED_196)
REGDEF_BIT(reserved,        32)
REGDEF_END(IME_RESERVED_196)


/*
    reserved                :    [0x0, 0xffffffff],         bits : 31_0
*/
#define IME_OUTPUT_PATH3_CONTROL_REGISTER31_OFS 0x08c0
REGDEF_BEGIN(IME_OUTPUT_PATH3_CONTROL_REGISTER31)
REGDEF_BIT(reserved,        32)
REGDEF_END(IME_OUTPUT_PATH3_CONTROL_REGISTER31)


/*
    reserved                :    [0x0, 0xffffffff],         bits : 31_0
*/
#define IME_OUTPUT_PATH3_CONTROL_REGISTER32_OFS 0x08c4
REGDEF_BEGIN(IME_OUTPUT_PATH3_CONTROL_REGISTER32)
REGDEF_BIT(reserved,        32)
REGDEF_END(IME_OUTPUT_PATH3_CONTROL_REGISTER32)


/*
    reserved                :    [0x0, 0xffffffff],         bits : 31_0
*/
#define IME_OUTPUT_PATH3_CONTROL_REGISTER33_OFS 0x08c8
REGDEF_BEGIN(IME_OUTPUT_PATH3_CONTROL_REGISTER33)
REGDEF_BIT(reserved,        32)
REGDEF_END(IME_OUTPUT_PATH3_CONTROL_REGISTER33)


/*
    reserved                :    [0x0, 0xffffffff],         bits : 31_0
*/
#define IME_OUTPUT_PATH3_CONTROL_REGISTER34_OFS 0x08cc
REGDEF_BEGIN(IME_OUTPUT_PATH3_CONTROL_REGISTER34)
REGDEF_BIT(reserved,        32)
REGDEF_END(IME_OUTPUT_PATH3_CONTROL_REGISTER34)


/*
    reserved                :    [0x0, 0xffffffff],         bits : 31_0
*/
#define IME_OUTPUT_PATH3_CONTROL_REGISTER35_OFS 0x08d0
REGDEF_BEGIN(IME_OUTPUT_PATH3_CONTROL_REGISTER35)
REGDEF_BIT(reserved,        32)
REGDEF_END(IME_OUTPUT_PATH3_CONTROL_REGISTER35)


/*
    reserved                :    [0x0, 0xffffffff],         bits : 31_0
*/
#define IME_OUTPUT_PATH3_CONTROL_REGISTER36_OFS 0x08d4
REGDEF_BEGIN(IME_OUTPUT_PATH3_CONTROL_REGISTER36)
REGDEF_BIT(reserved,        32)
REGDEF_END(IME_OUTPUT_PATH3_CONTROL_REGISTER36)


/*
    reserved                :    [0x0, 0xffffffff],         bits : 31_0
*/
#define IME_RESERVED_197_OFS 0x08d8
REGDEF_BEGIN(IME_RESERVED_197)
REGDEF_BIT(reserved,        32)
REGDEF_END(IME_RESERVED_197)


/*
    reserved                :    [0x0, 0xffffffff],         bits : 31_0
*/
#define IME_RESERVED_198_OFS 0x08dc
REGDEF_BEGIN(IME_RESERVED_198)
REGDEF_BIT(reserved,        32)
REGDEF_END(IME_RESERVED_198)


/*
    reserved                :    [0x0, 0xffffffff],         bits : 31_0
*/
#define IME_RESERVED_199_OFS 0x08e0
REGDEF_BEGIN(IME_RESERVED_199)
REGDEF_BIT(reserved,        32)
REGDEF_END(IME_RESERVED_199)


/*
    reserved                :    [0x0, 0xffffffff],         bits : 31_0
*/
#define IME_RESERVED_200_OFS 0x08e4
REGDEF_BEGIN(IME_RESERVED_200)
REGDEF_BIT(reserved,        32)
REGDEF_END(IME_RESERVED_200)


/*
    reserved                :    [0x0, 0xffffffff],         bits : 31_0
*/
#define IME_RESERVED_201_OFS 0x08e8
REGDEF_BEGIN(IME_RESERVED_201)
REGDEF_BIT(reserved,        32)
REGDEF_END(IME_RESERVED_201)


/*
    reserved                :    [0x0, 0xffffffff],         bits : 31_0
*/
#define IME_RESERVED_202_OFS 0x08ec
REGDEF_BEGIN(IME_RESERVED_202)
REGDEF_BIT(reserved,        32)
REGDEF_END(IME_RESERVED_202)


/*
    reserved                :    [0x0, 0xffffffff],         bits : 31_0
*/
#define IME_RESERVED_203_OFS 0x08f0
REGDEF_BEGIN(IME_RESERVED_203)
REGDEF_BIT(reserved,        32)
REGDEF_END(IME_RESERVED_203)


/*
    reserved                :    [0x0, 0xffffffff],         bits : 31_0
*/
#define IME_RESERVED_204_OFS 0x08f4
REGDEF_BEGIN(IME_RESERVED_204)
REGDEF_BIT(reserved,        32)
REGDEF_END(IME_RESERVED_204)


/*
    reserved                :    [0x0, 0xffffffff],         bits : 31_0
*/
#define IME_RESERVED_205_OFS 0x08f8
REGDEF_BEGIN(IME_RESERVED_205)
REGDEF_BIT(reserved,        32)
REGDEF_END(IME_RESERVED_205)


/*
    reserved                :    [0x0, 0xffffffff],         bits : 31_0
*/
#define IME_RESERVED_206_OFS 0x08fc
REGDEF_BEGIN(IME_RESERVED_206)
REGDEF_BIT(reserved,        32)
REGDEF_END(IME_RESERVED_206)


/*
    ime_p1_enc_smode_en            :    [0x0, 0x1],         bits : 0
    ime_p1_enc_padding_mode        :    [0x0, 0x1],         bits : 1
    ime_3dnr_refout_enc_smode_en   :    [0x0, 0x1],         bits : 4
    me_3dnr_refout_enc_padding_mode:    [0x0, 0x1],         bits : 5
    ime_3dnr_refin_dec_smode_en    :    [0x0, 0x1],         bits : 8
    ime_3dnr_refin_dec_padding_mode:    [0x0, 0x1],         bits : 9
    ime_3dnr_refin_dec_dither_en   :    [0x0, 0x1],         bits : 10
    ime_3dnr_refin_dec_dither_seed0:    [0x0, 0x7fff],          bits : 30_16
*/
#define IME_COMPRESSION_CONTROL_REGISTER0_OFS 0x0900
REGDEF_BEGIN(IME_COMPRESSION_CONTROL_REGISTER0)
REGDEF_BIT(ime_p1_enc_smode_en,        1)
REGDEF_BIT(ime_p1_enc_padding_mode,        1)
REGDEF_BIT(,        2)
REGDEF_BIT(ime_3dnr_refout_enc_smode_en,        1)
REGDEF_BIT(me_3dnr_refout_enc_padding_mode,        1)
REGDEF_BIT(,        2)
REGDEF_BIT(ime_3dnr_refin_dec_smode_en,        1)
REGDEF_BIT(ime_3dnr_refin_dec_padding_mode,        1)
REGDEF_BIT(ime_3dnr_refin_dec_dither_en,        1)
REGDEF_BIT(,        5)
REGDEF_BIT(ime_3dnr_refin_dec_dither_seed0,        15)
REGDEF_END(IME_COMPRESSION_CONTROL_REGISTER0)


/*
    ime_3dnr_refin_dec_dither_seed1:    [0x0, 0xf],         bits : 3_0
*/
#define IME_COMPRESSION_CONTROL_REGISTER1_OFS 0x0904
REGDEF_BEGIN(IME_COMPRESSION_CONTROL_REGISTER1)
REGDEF_BIT(ime_3dnr_refin_dec_dither_seed1,        4)
REGDEF_END(IME_COMPRESSION_CONTROL_REGISTER1)


/*
    reserved                :    [0x0, 0xffffffff],         bits : 31_0
*/
#define IME_RESERVED_207_OFS 0x0908
REGDEF_BEGIN(IME_RESERVED_207)
REGDEF_BIT(reserved,        32)
REGDEF_END(IME_RESERVED_207)


/*
    reserved                :    [0x0, 0xffffffff],         bits : 31_0
*/
#define IME_RESERVED_208_OFS 0x090c
REGDEF_BEGIN(IME_RESERVED_208)
REGDEF_BIT(reserved,        32)
REGDEF_END(IME_RESERVED_208)


/*
    dct_qtbl0_enc_p0:    [0x0, 0x1fff],         bits : 12_0
    dct_qtbl0_enc_p1:    [0x0, 0xfff],          bits : 27_16
*/
#define IME_YCC_ENCODER_Q_TABLE_REGISTER0_OFS 0x0910
REGDEF_BEGIN(IME_YCC_ENCODER_Q_TABLE_REGISTER0)
REGDEF_BIT(dct_qtbl0_enc_p0,        13)
REGDEF_BIT(,        3)
REGDEF_BIT(dct_qtbl0_enc_p1,        12)
REGDEF_END(IME_YCC_ENCODER_Q_TABLE_REGISTER0)


/*
    dct_qtbl0_enc_p2:    [0x0, 0x7ff],          bits : 10_0
*/
#define IME_YCC_ENCODER_Q_TABLE_REGISTER1_OFS 0x0914
REGDEF_BEGIN(IME_YCC_ENCODER_Q_TABLE_REGISTER1)
REGDEF_BIT(dct_qtbl0_enc_p2,        11)
REGDEF_END(IME_YCC_ENCODER_Q_TABLE_REGISTER1)


/*
    dct_qtbl1_enc_p0:    [0x0, 0x1fff],         bits : 12_0
    dct_qtbl1_enc_p1:    [0x0, 0xfff],          bits : 27_16
*/
#define IME_YCC_ENCODER_Q_TABLE_REGISTER2_OFS 0x0918
REGDEF_BEGIN(IME_YCC_ENCODER_Q_TABLE_REGISTER2)
REGDEF_BIT(dct_qtbl1_enc_p0,        13)
REGDEF_BIT(,        3)
REGDEF_BIT(dct_qtbl1_enc_p1,        12)
REGDEF_END(IME_YCC_ENCODER_Q_TABLE_REGISTER2)


/*
    dct_qtbl1_enc_p2:    [0x0, 0x7ff],          bits : 10_0
*/
#define IME_YCC_ENCODER_Q_TABLE_REGISTER3_OFS 0x091c
REGDEF_BEGIN(IME_YCC_ENCODER_Q_TABLE_REGISTER3)
REGDEF_BIT(dct_qtbl1_enc_p2,        11)
REGDEF_END(IME_YCC_ENCODER_Q_TABLE_REGISTER3)


/*
    dct_qtbl2_enc_p0:    [0x0, 0x1fff],         bits : 12_0
    dct_qtbl2_enc_p1:    [0x0, 0xfff],          bits : 27_16
*/
#define IME_YCC_ENCODER_Q_TABLE_REGISTER4_OFS 0x0920
REGDEF_BEGIN(IME_YCC_ENCODER_Q_TABLE_REGISTER4)
REGDEF_BIT(dct_qtbl2_enc_p0,        13)
REGDEF_BIT(,        3)
REGDEF_BIT(dct_qtbl2_enc_p1,        12)
REGDEF_END(IME_YCC_ENCODER_Q_TABLE_REGISTER4)


/*
    dct_qtbl2_enc_p2:    [0x0, 0x7ff],          bits : 10_0
*/
#define IME_YCC_ENCODER_Q_TABLE_REGISTER5_OFS 0x0924
REGDEF_BEGIN(IME_YCC_ENCODER_Q_TABLE_REGISTER5)
REGDEF_BIT(dct_qtbl2_enc_p2,        11)
REGDEF_END(IME_YCC_ENCODER_Q_TABLE_REGISTER5)


/*
    dct_qtbl3_enc_p0:    [0x0, 0x1fff],         bits : 12_0
    dct_qtbl3_enc_p1:    [0x0, 0xfff],          bits : 27_16
*/
#define IME_YCC_ENCODER_Q_TABLE_REGISTER6_OFS 0x0928
REGDEF_BEGIN(IME_YCC_ENCODER_Q_TABLE_REGISTER6)
REGDEF_BIT(dct_qtbl3_enc_p0,        13)
REGDEF_BIT(,        3)
REGDEF_BIT(dct_qtbl3_enc_p1,        12)
REGDEF_END(IME_YCC_ENCODER_Q_TABLE_REGISTER6)


/*
    dct_qtbl3_enc_p2:    [0x0, 0x7ff],          bits : 10_0
*/
#define IME_YCC_ENCODER_Q_TABLE_REGISTER7_OFS 0x092c
REGDEF_BEGIN(IME_YCC_ENCODER_Q_TABLE_REGISTER7)
REGDEF_BIT(dct_qtbl3_enc_p2,        11)
REGDEF_END(IME_YCC_ENCODER_Q_TABLE_REGISTER7)


/*
    dct_qtbl4_enc_p0:    [0x0, 0x1fff],         bits : 12_0
    dct_qtbl4_enc_p1:    [0x0, 0xfff],          bits : 27_16
*/
#define IME_YCC_ENCODER_Q_TABLE_REGISTER8_OFS 0x0930
REGDEF_BEGIN(IME_YCC_ENCODER_Q_TABLE_REGISTER8)
REGDEF_BIT(dct_qtbl4_enc_p0,        13)
REGDEF_BIT(,        3)
REGDEF_BIT(dct_qtbl4_enc_p1,        12)
REGDEF_END(IME_YCC_ENCODER_Q_TABLE_REGISTER8)


/*
    dct_qtbl4_enc_p2:    [0x0, 0x7ff],          bits : 10_0
*/
#define IME_YCC_ENCODER_Q_TABLE_REGISTER9_OFS 0x0934
REGDEF_BEGIN(IME_YCC_ENCODER_Q_TABLE_REGISTER9)
REGDEF_BIT(dct_qtbl4_enc_p2,        11)
REGDEF_END(IME_YCC_ENCODER_Q_TABLE_REGISTER9)


/*
    dct_qtbl5_enc_p0:    [0x0, 0x1fff],         bits : 12_0
    dct_qtbl5_enc_p1:    [0x0, 0xfff],          bits : 27_16
*/
#define IME_YCC_ENCODER_Q_TABLE_REGISTER10_OFS 0x0938
REGDEF_BEGIN(IME_YCC_ENCODER_Q_TABLE_REGISTER10)
REGDEF_BIT(dct_qtbl5_enc_p0,        13)
REGDEF_BIT(,        3)
REGDEF_BIT(dct_qtbl5_enc_p1,        12)
REGDEF_END(IME_YCC_ENCODER_Q_TABLE_REGISTER10)


/*
    dct_qtbl5_enc_p2:    [0x0, 0x7ff],          bits : 10_0
*/
#define IME_YCC_ENCODER_Q_TABLE_REGISTER11_OFS 0x093c
REGDEF_BEGIN(IME_YCC_ENCODER_Q_TABLE_REGISTER11)
REGDEF_BIT(dct_qtbl5_enc_p2,        11)
REGDEF_END(IME_YCC_ENCODER_Q_TABLE_REGISTER11)


/*
    dct_qtbl6_enc_p0:    [0x0, 0x1fff],         bits : 12_0
    dct_qtbl6_enc_p1:    [0x0, 0xfff],          bits : 27_16
*/
#define IME_YCC_ENCODER_Q_TABLE_REGISTER12_OFS 0x0940
REGDEF_BEGIN(IME_YCC_ENCODER_Q_TABLE_REGISTER12)
REGDEF_BIT(dct_qtbl6_enc_p0,        13)
REGDEF_BIT(,        3)
REGDEF_BIT(dct_qtbl6_enc_p1,        12)
REGDEF_END(IME_YCC_ENCODER_Q_TABLE_REGISTER12)


/*
    dct_qtbl6_enc_p2:    [0x0, 0x7ff],          bits : 10_0
*/
#define IME_YCC_ENCODER_Q_TABLE_REGISTER13_OFS 0x0944
REGDEF_BEGIN(IME_YCC_ENCODER_Q_TABLE_REGISTER13)
REGDEF_BIT(dct_qtbl6_enc_p2,        11)
REGDEF_END(IME_YCC_ENCODER_Q_TABLE_REGISTER13)


/*
    dct_qtbl7_enc_p0:    [0x0, 0x1fff],         bits : 12_0
    dct_qtbl7_enc_p1:    [0x0, 0xfff],          bits : 27_16
*/
#define IME_YCC_ENCODER_Q_TABLE_REGISTER14_OFS 0x0948
REGDEF_BEGIN(IME_YCC_ENCODER_Q_TABLE_REGISTER14)
REGDEF_BIT(dct_qtbl7_enc_p0,        13)
REGDEF_BIT(,        3)
REGDEF_BIT(dct_qtbl7_enc_p1,        12)
REGDEF_END(IME_YCC_ENCODER_Q_TABLE_REGISTER14)


/*
    dct_qtbl7_enc_p2:    [0x0, 0x7ff],          bits : 10_0
*/
#define IME_YCC_ENCODER_Q_TABLE_REGISTER15_OFS 0x094c
REGDEF_BEGIN(IME_YCC_ENCODER_Q_TABLE_REGISTER15)
REGDEF_BIT(dct_qtbl7_enc_p2,        11)
REGDEF_END(IME_YCC_ENCODER_Q_TABLE_REGISTER15)


/*
    dct_qtbl8_enc_p0:    [0x0, 0x1fff],         bits : 12_0
    dct_qtbl8_enc_p1:    [0x0, 0xfff],          bits : 27_16
*/
#define IME_YCC_ENCODER_Q_TABLE_REGISTER16_OFS 0x0950
REGDEF_BEGIN(IME_YCC_ENCODER_Q_TABLE_REGISTER16)
REGDEF_BIT(dct_qtbl8_enc_p0,        13)
REGDEF_BIT(,        3)
REGDEF_BIT(dct_qtbl8_enc_p1,        12)
REGDEF_END(IME_YCC_ENCODER_Q_TABLE_REGISTER16)


/*
    dct_qtbl8_enc_p2:    [0x0, 0x7ff],          bits : 10_0
*/
#define IME_YCC_ENCODER_Q_TABLE_REGISTER17_OFS 0x0954
REGDEF_BEGIN(IME_YCC_ENCODER_Q_TABLE_REGISTER17)
REGDEF_BIT(dct_qtbl8_enc_p2,        11)
REGDEF_END(IME_YCC_ENCODER_Q_TABLE_REGISTER17)


/*
    dct_qtbl9_enc_p0:    [0x0, 0x1fff],         bits : 12_0
    dct_qtbl9_enc_p1:    [0x0, 0xfff],          bits : 27_16
*/
#define IME_YCC_ENCODER_Q_TABLE_REGISTER18_OFS 0x0958
REGDEF_BEGIN(IME_YCC_ENCODER_Q_TABLE_REGISTER18)
REGDEF_BIT(dct_qtbl9_enc_p0,        13)
REGDEF_BIT(,        3)
REGDEF_BIT(dct_qtbl9_enc_p1,        12)
REGDEF_END(IME_YCC_ENCODER_Q_TABLE_REGISTER18)


/*
    dct_qtbl9_enc_p2:    [0x0, 0x7ff],          bits : 10_0
*/
#define IME_YCC_ENCODER_Q_TABLE_REGISTER19_OFS 0x095c
REGDEF_BEGIN(IME_YCC_ENCODER_Q_TABLE_REGISTER19)
REGDEF_BIT(dct_qtbl9_enc_p2,        11)
REGDEF_END(IME_YCC_ENCODER_Q_TABLE_REGISTER19)


/*
    dct_qtbl10_enc_p0:    [0x0, 0x1fff],            bits : 12_0
    dct_qtbl10_enc_p1:    [0x0, 0xfff],         bits : 27_16
*/
#define IME_YCC_ENCODER_Q_TABLE_REGISTER20_OFS 0x0960
REGDEF_BEGIN(IME_YCC_ENCODER_Q_TABLE_REGISTER20)
REGDEF_BIT(dct_qtbl10_enc_p0,        13)
REGDEF_BIT(,        3)
REGDEF_BIT(dct_qtbl10_enc_p1,        12)
REGDEF_END(IME_YCC_ENCODER_Q_TABLE_REGISTER20)


/*
    dct_qtbl10_enc_p2:    [0x0, 0x7ff],         bits : 10_0
*/
#define IME_YCC_ENCODER_Q_TABLE_REGISTER21_OFS 0x0964
REGDEF_BEGIN(IME_YCC_ENCODER_Q_TABLE_REGISTER21)
REGDEF_BIT(dct_qtbl10_enc_p2,        11)
REGDEF_END(IME_YCC_ENCODER_Q_TABLE_REGISTER21)


/*
    dct_qtbl11_enc_p0:    [0x0, 0x1fff],            bits : 12_0
    dct_qtbl11_enc_p1:    [0x0, 0xfff],         bits : 27_16
*/
#define IME_YCC_ENCODER_Q_TABLE_REGISTER22_OFS 0x0968
REGDEF_BEGIN(IME_YCC_ENCODER_Q_TABLE_REGISTER22)
REGDEF_BIT(dct_qtbl11_enc_p0,        13)
REGDEF_BIT(,        3)
REGDEF_BIT(dct_qtbl11_enc_p1,        12)
REGDEF_END(IME_YCC_ENCODER_Q_TABLE_REGISTER22)


/*
    dct_qtbl11_enc_p2:    [0x0, 0x7ff],         bits : 10_0
*/
#define IME_YCC_ENCODER_Q_TABLE_REGISTER23_OFS 0x096c
REGDEF_BEGIN(IME_YCC_ENCODER_Q_TABLE_REGISTER23)
REGDEF_BIT(dct_qtbl11_enc_p2,        11)
REGDEF_END(IME_YCC_ENCODER_Q_TABLE_REGISTER23)


/*
    dct_qtbl12_enc_p0:    [0x0, 0x1fff],            bits : 12_0
    dct_qtbl12_enc_p1:    [0x0, 0xfff],         bits : 27_16
*/
#define IME_YCC_ENCODER_Q_TABLE_REGISTER24_OFS 0x0970
REGDEF_BEGIN(IME_YCC_ENCODER_Q_TABLE_REGISTER24)
REGDEF_BIT(dct_qtbl12_enc_p0,        13)
REGDEF_BIT(,        3)
REGDEF_BIT(dct_qtbl12_enc_p1,        12)
REGDEF_END(IME_YCC_ENCODER_Q_TABLE_REGISTER24)


/*
    dct_qtbl12_enc_p2:    [0x0, 0x7ff],         bits : 10_0
*/
#define IME_YCC_ENCODER_Q_TABLE_REGISTER25_OFS 0x0974
REGDEF_BEGIN(IME_YCC_ENCODER_Q_TABLE_REGISTER25)
REGDEF_BIT(dct_qtbl12_enc_p2,        11)
REGDEF_END(IME_YCC_ENCODER_Q_TABLE_REGISTER25)


/*
    dct_qtbl13_enc_p0:    [0x0, 0x1fff],            bits : 12_0
    dct_qtbl13_enc_p1:    [0x0, 0xfff],         bits : 27_16
*/
#define IME_YCC_ENCODER_Q_TABLE_REGISTER26_OFS 0x0978
REGDEF_BEGIN(IME_YCC_ENCODER_Q_TABLE_REGISTER26)
REGDEF_BIT(dct_qtbl13_enc_p0,        13)
REGDEF_BIT(,        3)
REGDEF_BIT(dct_qtbl13_enc_p1,        12)
REGDEF_END(IME_YCC_ENCODER_Q_TABLE_REGISTER26)


/*
    dct_qtbl13_enc_p2:    [0x0, 0x7ff],         bits : 10_0
*/
#define IME_YCC_ENCODER_Q_TABLE_REGISTER27_OFS 0x097c
REGDEF_BEGIN(IME_YCC_ENCODER_Q_TABLE_REGISTER27)
REGDEF_BIT(dct_qtbl13_enc_p2,        11)
REGDEF_END(IME_YCC_ENCODER_Q_TABLE_REGISTER27)


/*
    dct_qtbl14_enc_p0:    [0x0, 0x1fff],            bits : 12_0
    dct_qtbl14_enc_p1:    [0x0, 0xfff],         bits : 27_16
*/
#define IME_YCC_ENCODER_Q_TABLE_REGISTER28_OFS 0x0980
REGDEF_BEGIN(IME_YCC_ENCODER_Q_TABLE_REGISTER28)
REGDEF_BIT(dct_qtbl14_enc_p0,        13)
REGDEF_BIT(,        3)
REGDEF_BIT(dct_qtbl14_enc_p1,        12)
REGDEF_END(IME_YCC_ENCODER_Q_TABLE_REGISTER28)


/*
    dct_qtbl14_enc_p2:    [0x0, 0x7ff],         bits : 10_0
*/
#define IME_YCC_ENCODER_Q_TABLE_REGISTER29_OFS 0x0984
REGDEF_BEGIN(IME_YCC_ENCODER_Q_TABLE_REGISTER29)
REGDEF_BIT(dct_qtbl14_enc_p2,        11)
REGDEF_END(IME_YCC_ENCODER_Q_TABLE_REGISTER29)


/*
    dct_qtbl15_enc_p0:    [0x0, 0x1fff],            bits : 12_0
    dct_qtbl15_enc_p1:    [0x0, 0xfff],         bits : 27_16
*/
#define IME_YCC_ENCODER_Q_TABLE_REGISTER30_OFS 0x0988
REGDEF_BEGIN(IME_YCC_ENCODER_Q_TABLE_REGISTER30)
REGDEF_BIT(dct_qtbl15_enc_p0,        13)
REGDEF_BIT(,        3)
REGDEF_BIT(dct_qtbl15_enc_p1,        12)
REGDEF_END(IME_YCC_ENCODER_Q_TABLE_REGISTER30)


/*
    dct_qtbl15_enc_p2:    [0x0, 0x7ff],         bits : 10_0
*/
#define IME_YCC_ENCODER_Q_TABLE_REGISTER31_OFS 0x098c
REGDEF_BEGIN(IME_YCC_ENCODER_Q_TABLE_REGISTER31)
REGDEF_BIT(dct_qtbl15_enc_p2,        11)
REGDEF_END(IME_YCC_ENCODER_Q_TABLE_REGISTER31)


/*
    dct_qtbl0_dcmax:    [0x0, 0x1ff],           bits : 8_0
    dct_qtbl1_dcmax:    [0x0, 0x1ff],           bits : 24_16
*/
#define IME_YCC_ENCODER_QUALITY_DC_REGISTER0_OFS 0x0990
REGDEF_BEGIN(IME_YCC_ENCODER_QUALITY_DC_REGISTER0)
REGDEF_BIT(dct_qtbl0_dcmax,        9)
REGDEF_BIT(,        7)
REGDEF_BIT(dct_qtbl1_dcmax,        9)
REGDEF_END(IME_YCC_ENCODER_QUALITY_DC_REGISTER0)


/*
    dct_qtbl2_dcmax:    [0x0, 0x1ff],           bits : 8_0
    dct_qtbl3_dcmax:    [0x0, 0x1ff],           bits : 24_16
*/
#define IME_YCC_ENCODER_QUALITY_DC_REGISTER1_OFS 0x0994
REGDEF_BEGIN(IME_YCC_ENCODER_QUALITY_DC_REGISTER1)
REGDEF_BIT(dct_qtbl2_dcmax,        9)
REGDEF_BIT(,        7)
REGDEF_BIT(dct_qtbl3_dcmax,        9)
REGDEF_END(IME_YCC_ENCODER_QUALITY_DC_REGISTER1)


/*
    dct_qtbl4_dcmax:    [0x0, 0x1ff],           bits : 8_0
    dct_qtbl5_dcmax:    [0x0, 0x1ff],           bits : 24_16
*/
#define IME_YCC_ENCODER_QUALITY_DC_REGISTER2_OFS 0x0998
REGDEF_BEGIN(IME_YCC_ENCODER_QUALITY_DC_REGISTER2)
REGDEF_BIT(dct_qtbl4_dcmax,        9)
REGDEF_BIT(,        7)
REGDEF_BIT(dct_qtbl5_dcmax,        9)
REGDEF_END(IME_YCC_ENCODER_QUALITY_DC_REGISTER2)


/*
    dct_qtbl6_dcmax:    [0x0, 0x1ff],           bits : 8_0
    dct_qtbl7_dcmax:    [0x0, 0x1ff],           bits : 24_16
*/
#define IME_YCC_ENCODER_QUALITY_DC_REGISTER3_OFS 0x099c
REGDEF_BEGIN(IME_YCC_ENCODER_QUALITY_DC_REGISTER3)
REGDEF_BIT(dct_qtbl6_dcmax,        9)
REGDEF_BIT(,        7)
REGDEF_BIT(dct_qtbl7_dcmax,        9)
REGDEF_END(IME_YCC_ENCODER_QUALITY_DC_REGISTER3)


/*
    dct_qtbl8_dcmax:    [0x0, 0x1ff],           bits : 8_0
    dct_qtbl9_dcmax:    [0x0, 0x1ff],           bits : 24_16
*/
#define IME_YCC_ENCODER_QUALITY_DC_REGISTER4_OFS 0x09a0
REGDEF_BEGIN(IME_YCC_ENCODER_QUALITY_DC_REGISTER4)
REGDEF_BIT(dct_qtbl8_dcmax,        9)
REGDEF_BIT(,        7)
REGDEF_BIT(dct_qtbl9_dcmax,        9)
REGDEF_END(IME_YCC_ENCODER_QUALITY_DC_REGISTER4)


/*
    dct_qtbl10_dcmax:    [0x0, 0x1ff],          bits : 8_0
    dct_qtbl11_dcmax:    [0x0, 0x1ff],          bits : 24_16
*/
#define IME_YCC_ENCODER_QUALITY_DC_REGISTER5_OFS 0x09a4
REGDEF_BEGIN(IME_YCC_ENCODER_QUALITY_DC_REGISTER5)
REGDEF_BIT(dct_qtbl10_dcmax,        9)
REGDEF_BIT(,        7)
REGDEF_BIT(dct_qtbl11_dcmax,        9)
REGDEF_END(IME_YCC_ENCODER_QUALITY_DC_REGISTER5)


/*
    dct_qtbl12_dcmax:    [0x0, 0x1ff],          bits : 8_0
    dct_qtbl13_dcmax:    [0x0, 0x1ff],          bits : 24_16
*/
#define IME_YCC_ENCODER_QUALITY_DC_REGISTER6_OFS 0x09a8
REGDEF_BEGIN(IME_YCC_ENCODER_QUALITY_DC_REGISTER6)
REGDEF_BIT(dct_qtbl12_dcmax,        9)
REGDEF_BIT(,        7)
REGDEF_BIT(dct_qtbl13_dcmax,        9)
REGDEF_END(IME_YCC_ENCODER_QUALITY_DC_REGISTER6)


/*
    dct_qtbl14_dcmax:    [0x0, 0x1ff],          bits : 8_0
    dct_qtbl15_dcmax:    [0x0, 0x1ff],          bits : 24_16
*/
#define IME_YCC_ENCODER_QUALITY_DC_REGISTER7_OFS 0x09ac
REGDEF_BEGIN(IME_YCC_ENCODER_QUALITY_DC_REGISTER7)
REGDEF_BIT(dct_qtbl14_dcmax,        9)
REGDEF_BIT(,        7)
REGDEF_BIT(dct_qtbl15_dcmax,        9)
REGDEF_END(IME_YCC_ENCODER_QUALITY_DC_REGISTER7)


/*
    yrc_lncnt_lfn0:    [0x0, 0x1fff],           bits : 12_0
    yrc_lncnt_lfn1:    [0x0, 0xfff],            bits : 27_16
*/
#define IME_YCC_ENCODER_QUALITY_TABLE_CHECK_REGISTER0_OFS 0x09b0
REGDEF_BEGIN(IME_YCC_ENCODER_QUALITY_TABLE_CHECK_REGISTER0)
REGDEF_BIT(yrc_lncnt_lfn0,        13)
REGDEF_BIT(,        3)
REGDEF_BIT(yrc_lncnt_lfn1,        12)
REGDEF_END(IME_YCC_ENCODER_QUALITY_TABLE_CHECK_REGISTER0)


/*
    yrc_lncnt_lfn2:    [0x0, 0x1fff],           bits : 12_0
    yrc_lncnt_lfn3:    [0x0, 0xfff],            bits : 27_16
*/
#define IME_YCC_ENCODER_QUALITY_TABLE_CHECK_REGISTER1_OFS 0x09b4
REGDEF_BEGIN(IME_YCC_ENCODER_QUALITY_TABLE_CHECK_REGISTER1)
REGDEF_BIT(yrc_lncnt_lfn2,        13)
REGDEF_BIT(,        3)
REGDEF_BIT(yrc_lncnt_lfn3,        12)
REGDEF_END(IME_YCC_ENCODER_QUALITY_TABLE_CHECK_REGISTER1)


/*
    yrc_lncnt_lfn4:    [0x0, 0x1fff],           bits : 12_0
    yrc_lncnt_lfn5:    [0x0, 0xfff],            bits : 27_16
*/
#define IME_YCC_ENCODER_QUALITY_TABLE_CHECK_REGISTER2_OFS 0x09b8
REGDEF_BEGIN(IME_YCC_ENCODER_QUALITY_TABLE_CHECK_REGISTER2)
REGDEF_BIT(yrc_lncnt_lfn4,        13)
REGDEF_BIT(,        3)
REGDEF_BIT(yrc_lncnt_lfn5,        12)
REGDEF_END(IME_YCC_ENCODER_QUALITY_TABLE_CHECK_REGISTER2)


/*
    dct_maxdist:    [0x0, 0xff],            bits : 7_0
*/
#define IME_MAX_ERROR_FOR_DCT_OFS 0x09bc
REGDEF_BEGIN(IME_MAX_ERROR_FOR_DCT)
REGDEF_BIT(dct_maxdist,        8)
REGDEF_END(IME_MAX_ERROR_FOR_DCT)


/*
    dct_qtbl0_dec_p0:    [0x0, 0x3ff],          bits : 9_0
    dct_qtbl0_dec_p1:    [0x0, 0x3ff],          bits : 25_16
*/
#define IME_DECODER_QUALITY_TABLE_0_REGISTER0_OFS 0x09c0
REGDEF_BEGIN(IME_DECODER_QUALITY_TABLE_0_REGISTER0)
REGDEF_BIT(dct_qtbl0_dec_p0,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(dct_qtbl0_dec_p1,        10)
REGDEF_END(IME_DECODER_QUALITY_TABLE_0_REGISTER0)


/*
    dct_qtbl0_dec_p2:    [0x0, 0x3ff],          bits : 9_0
*/
#define IME_DECODER_QUALITY_TABLE_0_REGISTER1_OFS 0x09c4
REGDEF_BEGIN(IME_DECODER_QUALITY_TABLE_0_REGISTER1)
REGDEF_BIT(dct_qtbl0_dec_p2,        10)
REGDEF_END(IME_DECODER_QUALITY_TABLE_0_REGISTER1)


/*
    dct_qtbl1_dec_p0:    [0x0, 0x3ff],          bits : 9_0
    dct_qtbl1_dec_p1:    [0x0, 0x3ff],          bits : 25_16
*/
#define IME_DECODER_QUALITY_TABLE_1_REGISTER2_OFS 0x09c8
REGDEF_BEGIN(IME_DECODER_QUALITY_TABLE_1_REGISTER2)
REGDEF_BIT(dct_qtbl1_dec_p0,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(dct_qtbl1_dec_p1,        10)
REGDEF_END(IME_DECODER_QUALITY_TABLE_1_REGISTER2)


/*
    dct_qtbl1_dec_p2:    [0x0, 0x3ff],          bits : 9_0
*/
#define IME_DECODER_QUALITY_TABLE_1_REGISTER3_OFS 0x09cc
REGDEF_BEGIN(IME_DECODER_QUALITY_TABLE_1_REGISTER3)
REGDEF_BIT(dct_qtbl1_dec_p2,        10)
REGDEF_END(IME_DECODER_QUALITY_TABLE_1_REGISTER3)


/*
    dct_qtbl2_dec_p0:    [0x0, 0x3ff],          bits : 9_0
    dct_qtbl2_dec_p1:    [0x0, 0x3ff],          bits : 25_16
*/
#define IME_DECODER_QUALITY_TABLE_2_REGISTER4_OFS 0x09d0
REGDEF_BEGIN(IME_DECODER_QUALITY_TABLE_2_REGISTER4)
REGDEF_BIT(dct_qtbl2_dec_p0,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(dct_qtbl2_dec_p1,        10)
REGDEF_END(IME_DECODER_QUALITY_TABLE_2_REGISTER4)


/*
    dct_qtbl2_dec_p2:    [0x0, 0x3ff],          bits : 9_0
*/
#define IME_DECODER_QUALITY_TABLE_2_REGISTER5_OFS 0x09d4
REGDEF_BEGIN(IME_DECODER_QUALITY_TABLE_2_REGISTER5)
REGDEF_BIT(dct_qtbl2_dec_p2,        10)
REGDEF_END(IME_DECODER_QUALITY_TABLE_2_REGISTER5)


/*
    dct_qtbl3_dec_p0:    [0x0, 0x3ff],          bits : 9_0
    dct_qtbl3_dec_p1:    [0x0, 0x3ff],          bits : 25_16
*/
#define IME_DECODER_QUALITY_TABLE_3_REGISTER6_OFS 0x09d8
REGDEF_BEGIN(IME_DECODER_QUALITY_TABLE_3_REGISTER6)
REGDEF_BIT(dct_qtbl3_dec_p0,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(dct_qtbl3_dec_p1,        10)
REGDEF_END(IME_DECODER_QUALITY_TABLE_3_REGISTER6)


/*
    dct_qtbl3_dec_p2:    [0x0, 0x3ff],          bits : 9_0
*/
#define IME_DECODER_QUALITY_TABLE_3_REGISTER7_OFS 0x09dc
REGDEF_BEGIN(IME_DECODER_QUALITY_TABLE_3_REGISTER7)
REGDEF_BIT(dct_qtbl3_dec_p2,        10)
REGDEF_END(IME_DECODER_QUALITY_TABLE_3_REGISTER7)


/*
    dct_qtbl4_dec_p0:    [0x0, 0x3ff],          bits : 9_0
    dct_qtbl4_dec_p1:    [0x0, 0x3ff],          bits : 25_16
*/
#define IME_DECODER_QUALITY_TABLE_4_REGISTER8_OFS 0x09e0
REGDEF_BEGIN(IME_DECODER_QUALITY_TABLE_4_REGISTER8)
REGDEF_BIT(dct_qtbl4_dec_p0,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(dct_qtbl4_dec_p1,        10)
REGDEF_END(IME_DECODER_QUALITY_TABLE_4_REGISTER8)


/*
    dct_qtbl4_dec_p2:    [0x0, 0x3ff],          bits : 9_0
*/
#define IME_DECODER_QUALITY_TABLE_4_REGISTER9_OFS 0x09e4
REGDEF_BEGIN(IME_DECODER_QUALITY_TABLE_4_REGISTER9)
REGDEF_BIT(dct_qtbl4_dec_p2,        10)
REGDEF_END(IME_DECODER_QUALITY_TABLE_4_REGISTER9)


/*
    dct_qtbl5_dec_p0:    [0x0, 0x3ff],          bits : 9_0
    dct_qtbl5_dec_p1:    [0x0, 0x3ff],          bits : 25_16
*/
#define IME_DECODER_QUALITY_TABLE_5_REGISTER10_OFS 0x09e8
REGDEF_BEGIN(IME_DECODER_QUALITY_TABLE_5_REGISTER10)
REGDEF_BIT(dct_qtbl5_dec_p0,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(dct_qtbl5_dec_p1,        10)
REGDEF_END(IME_DECODER_QUALITY_TABLE_5_REGISTER10)


/*
    dct_qtbl5_dec_p2:    [0x0, 0x3ff],          bits : 9_0
*/
#define IME_DECODER_QUALITY_TABLE_5_REGISTER11_OFS 0x09ec
REGDEF_BEGIN(IME_DECODER_QUALITY_TABLE_5_REGISTER11)
REGDEF_BIT(dct_qtbl5_dec_p2,        10)
REGDEF_END(IME_DECODER_QUALITY_TABLE_5_REGISTER11)


/*
    dct_qtbl6_dec_p0:    [0x0, 0x3ff],          bits : 9_0
    dct_qtbl6_dec_p1:    [0x0, 0x3ff],          bits : 25_16
*/
#define IME_DECODER_QUALITY_TABLE_6_REGISTER12_OFS 0x09f0
REGDEF_BEGIN(IME_DECODER_QUALITY_TABLE_6_REGISTER12)
REGDEF_BIT(dct_qtbl6_dec_p0,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(dct_qtbl6_dec_p1,        10)
REGDEF_END(IME_DECODER_QUALITY_TABLE_6_REGISTER12)


/*
    dct_qtbl6_dec_p2:    [0x0, 0x3ff],          bits : 9_0
*/
#define IME_DECODER_QUALITY_TABLE_6_REGISTER13_OFS 0x09f4
REGDEF_BEGIN(IME_DECODER_QUALITY_TABLE_6_REGISTER13)
REGDEF_BIT(dct_qtbl6_dec_p2,        10)
REGDEF_END(IME_DECODER_QUALITY_TABLE_6_REGISTER13)


/*
    dct_qtbl7_dec_p0:    [0x0, 0x3ff],          bits : 9_0
    dct_qtbl7_dec_p1:    [0x0, 0x3ff],          bits : 25_16
*/
#define IME_DECODER_QUALITY_TABLE_7_REGISTER14_OFS 0x09f8
REGDEF_BEGIN(IME_DECODER_QUALITY_TABLE_7_REGISTER14)
REGDEF_BIT(dct_qtbl7_dec_p0,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(dct_qtbl7_dec_p1,        10)
REGDEF_END(IME_DECODER_QUALITY_TABLE_7_REGISTER14)


/*
    dct_qtbl7_dec_p2:    [0x0, 0x3ff],          bits : 9_0
*/
#define IME_DECODER_QUALITY_TABLE_7_REGISTER15_OFS 0x09fc
REGDEF_BEGIN(IME_DECODER_QUALITY_TABLE_7_REGISTER15)
REGDEF_BIT(dct_qtbl7_dec_p2,        10)
REGDEF_END(IME_DECODER_QUALITY_TABLE_7_REGISTER15)


/*
    dct_qtbl8_dec_p0:    [0x0, 0x3ff],          bits : 9_0
    dct_qtbl8_dec_p1:    [0x0, 0x3ff],          bits : 25_16
*/
#define IME_DECODER_QUALITY_TABLE_8_REGISTER16_OFS 0x0a00
REGDEF_BEGIN(IME_DECODER_QUALITY_TABLE_8_REGISTER16)
REGDEF_BIT(dct_qtbl8_dec_p0,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(dct_qtbl8_dec_p1,        10)
REGDEF_END(IME_DECODER_QUALITY_TABLE_8_REGISTER16)


/*
    dct_qtbl8_dec_p2:    [0x0, 0x3ff],          bits : 9_0
*/
#define IME_DECODER_QUALITY_TABLE_8_REGISTER17_OFS 0x0a04
REGDEF_BEGIN(IME_DECODER_QUALITY_TABLE_8_REGISTER17)
REGDEF_BIT(dct_qtbl8_dec_p2,        10)
REGDEF_END(IME_DECODER_QUALITY_TABLE_8_REGISTER17)


/*
    dct_qtbl9_dec_p0:    [0x0, 0x3ff],          bits : 9_0
    dct_qtbl9_dec_p1:    [0x0, 0x3ff],          bits : 25_16
*/
#define IME_DECODER_QUALITY_TABLE_9_REGISTER18_OFS 0x0a08
REGDEF_BEGIN(IME_DECODER_QUALITY_TABLE_9_REGISTER18)
REGDEF_BIT(dct_qtbl9_dec_p0,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(dct_qtbl9_dec_p1,        10)
REGDEF_END(IME_DECODER_QUALITY_TABLE_9_REGISTER18)


/*
    dct_qtbl9_dec_p2:    [0x0, 0x3ff],          bits : 9_0
*/
#define IME_DECODER_QUALITY_TABLE_9_REGISTER19_OFS 0x0a0c
REGDEF_BEGIN(IME_DECODER_QUALITY_TABLE_9_REGISTER19)
REGDEF_BIT(dct_qtbl9_dec_p2,        10)
REGDEF_END(IME_DECODER_QUALITY_TABLE_9_REGISTER19)


/*
    dct_qtbl10_dec_p0:    [0x0, 0x3ff],         bits : 9_0
    dct_qtbl10_dec_p1:    [0x0, 0x3ff],         bits : 25_16
*/
#define IME_DECODER_QUALITY_TABLE_10_REGISTER20_OFS 0x0a10
REGDEF_BEGIN(IME_DECODER_QUALITY_TABLE_10_REGISTER20)
REGDEF_BIT(dct_qtbl10_dec_p0,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(dct_qtbl10_dec_p1,        10)
REGDEF_END(IME_DECODER_QUALITY_TABLE_10_REGISTER20)


/*
    dct_qtbl10_dec_p2:    [0x0, 0x3ff],         bits : 9_0
*/
#define IME_DECODER_QUALITY_TABLE_10_REGISTER21_OFS 0x0a14
REGDEF_BEGIN(IME_DECODER_QUALITY_TABLE_10_REGISTER21)
REGDEF_BIT(dct_qtbl10_dec_p2,        10)
REGDEF_END(IME_DECODER_QUALITY_TABLE_10_REGISTER21)


/*
    dct_qtbl11_dec_p0:    [0x0, 0x3ff],         bits : 9_0
    dct_qtbl11_dec_p1:    [0x0, 0x3ff],         bits : 25_16
*/
#define IME_DECODER_QUALITY_TABLE_11_REGISTER22_OFS 0x0a18
REGDEF_BEGIN(IME_DECODER_QUALITY_TABLE_11_REGISTER22)
REGDEF_BIT(dct_qtbl11_dec_p0,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(dct_qtbl11_dec_p1,        10)
REGDEF_END(IME_DECODER_QUALITY_TABLE_11_REGISTER22)


/*
    dct_qtbl11_dec_p2:    [0x0, 0x3ff],         bits : 9_0
*/
#define IME_DECODER_QUALITY_TABLE_11_REGISTER23_OFS 0x0a1c
REGDEF_BEGIN(IME_DECODER_QUALITY_TABLE_11_REGISTER23)
REGDEF_BIT(dct_qtbl11_dec_p2,        10)
REGDEF_END(IME_DECODER_QUALITY_TABLE_11_REGISTER23)


/*
    dct_qtbl12_dec_p0:    [0x0, 0x3ff],         bits : 9_0
    dct_qtbl12_dec_p1:    [0x0, 0x3ff],         bits : 25_16
*/
#define IME_DECODER_QUALITY_TABLE_12_REGISTER24_OFS 0x0a20
REGDEF_BEGIN(IME_DECODER_QUALITY_TABLE_12_REGISTER24)
REGDEF_BIT(dct_qtbl12_dec_p0,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(dct_qtbl12_dec_p1,        10)
REGDEF_END(IME_DECODER_QUALITY_TABLE_12_REGISTER24)


/*
    dct_qtbl12_dec_p2:    [0x0, 0x3ff],         bits : 9_0
*/
#define IME_DECODER_QUALITY_TABLE_12_REGISTER25_OFS 0x0a24
REGDEF_BEGIN(IME_DECODER_QUALITY_TABLE_12_REGISTER25)
REGDEF_BIT(dct_qtbl12_dec_p2,        10)
REGDEF_END(IME_DECODER_QUALITY_TABLE_12_REGISTER25)


/*
    dct_qtbl13_dec_p0:    [0x0, 0x3ff],         bits : 9_0
    dct_qtbl13_dec_p1:    [0x0, 0x3ff],         bits : 25_16
*/
#define IME_DECODER_QUALITY_TABLE_13_REGISTER26_OFS 0x0a28
REGDEF_BEGIN(IME_DECODER_QUALITY_TABLE_13_REGISTER26)
REGDEF_BIT(dct_qtbl13_dec_p0,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(dct_qtbl13_dec_p1,        10)
REGDEF_END(IME_DECODER_QUALITY_TABLE_13_REGISTER26)


/*
    dct_qtbl13_dec_p2:    [0x0, 0x3ff],         bits : 9_0
*/
#define IME_DECODER_QUALITY_TABLE_13_REGISTER27_OFS 0x0a2c
REGDEF_BEGIN(IME_DECODER_QUALITY_TABLE_13_REGISTER27)
REGDEF_BIT(dct_qtbl13_dec_p2,        10)
REGDEF_END(IME_DECODER_QUALITY_TABLE_13_REGISTER27)


/*
    dct_qtbl14_dec_p0:    [0x0, 0x3ff],         bits : 9_0
    dct_qtbl14_dec_p1:    [0x0, 0x3ff],         bits : 25_16
*/
#define IME_DECODER_QUALITY_TABLE_14_REGISTER28_OFS 0x0a30
REGDEF_BEGIN(IME_DECODER_QUALITY_TABLE_14_REGISTER28)
REGDEF_BIT(dct_qtbl14_dec_p0,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(dct_qtbl14_dec_p1,        10)
REGDEF_END(IME_DECODER_QUALITY_TABLE_14_REGISTER28)


/*
    dct_qtbl14_dec_p2:    [0x0, 0x3ff],         bits : 9_0
*/
#define IME_DECODER_QUALITY_TABLE_14_REGISTER29_OFS 0x0a34
REGDEF_BEGIN(IME_DECODER_QUALITY_TABLE_14_REGISTER29)
REGDEF_BIT(dct_qtbl14_dec_p2,        10)
REGDEF_END(IME_DECODER_QUALITY_TABLE_14_REGISTER29)


/*
    dct_qtbl15_dec_p0:    [0x0, 0x3ff],         bits : 9_0
    dct_qtbl15_dec_p1:    [0x0, 0x3ff],         bits : 25_16
*/
#define IME_DECODER_QUALITY_TABLE_15_REGISTER30_OFS 0x0a38
REGDEF_BEGIN(IME_DECODER_QUALITY_TABLE_15_REGISTER30)
REGDEF_BIT(dct_qtbl15_dec_p0,        10)
REGDEF_BIT(,        6)
REGDEF_BIT(dct_qtbl15_dec_p1,        10)
REGDEF_END(IME_DECODER_QUALITY_TABLE_15_REGISTER30)


/*
    dct_qtbl15_dec_p2:    [0x0, 0x3ff],         bits : 9_0
*/
#define IME_DECODER_QUALITY_TABLE_15_REGISTER31_OFS 0x0a3c
REGDEF_BEGIN(IME_DECODER_QUALITY_TABLE_15_REGISTER31)
REGDEF_BIT(dct_qtbl15_dec_p2,        10)
REGDEF_END(IME_DECODER_QUALITY_TABLE_15_REGISTER31)

/*
    dct_level_th0:    [0x0, 0xff],          bits : 7_0
    dct_level_th1:    [0x0, 0xff],          bits : 15_8
    dct_level_th2:    [0x0, 0xff],          bits : 23_16
    dct_level_th3:    [0x0, 0xff],          bits : 31_24
*/
#define ENCODER_QUALITY_THRESHOLD_REGISTER0_OFS 0x0a40
REGDEF_BEGIN(ENCODER_QUALITY_THRESHOLD_REGISTER0)
REGDEF_BIT(dct_level_th0,        8)
REGDEF_BIT(dct_level_th1,        8)
REGDEF_BIT(dct_level_th2,        8)
REGDEF_BIT(dct_level_th3,        8)
REGDEF_END(ENCODER_QUALITY_THRESHOLD_REGISTER0)


/*
    dct_level_th4:    [0x0, 0xff],          bits : 7_0
    dct_level_th5:    [0x0, 0xff],          bits : 15_8
    dct_level_th6:    [0x0, 0xff],          bits : 23_16
    dct_level_th7:    [0x0, 0xff],          bits : 31_24
*/
#define ENCODER_QUALITY_THRESHOLD_REGISTER1_OFS 0x0a44
REGDEF_BEGIN(ENCODER_QUALITY_THRESHOLD_REGISTER1)
REGDEF_BIT(dct_level_th4,        8)
REGDEF_BIT(dct_level_th5,        8)
REGDEF_BIT(dct_level_th6,        8)
REGDEF_BIT(dct_level_th7,        8)
REGDEF_END(ENCODER_QUALITY_THRESHOLD_REGISTER1)


/*
    reserved                :    [0x0, 0xffffffff],         bits : 31_0
*/
#define IME_RESERVED_211_OFS 0x0a48
REGDEF_BEGIN(IME_RESERVED_211)
REGDEF_BIT(reserved,        32)
REGDEF_END(IME_RESERVED_211)


/*
    reserved                :    [0x0, 0xffffffff],         bits : 31_0
*/
#define IME_RESERVED_212_OFS 0x0a4c
REGDEF_BEGIN(IME_RESERVED_212)
REGDEF_BIT(reserved,        32)
REGDEF_END(IME_RESERVED_212)


/*
    reserved                :    [0x0, 0xffffffff],         bits : 31_0
*/
#define IME_RESERVED_213_OFS 0x0a50
REGDEF_BEGIN(IME_RESERVED_213)
REGDEF_BIT(reserved,        32)
REGDEF_END(IME_RESERVED_213)


/*
    reserved                :    [0x0, 0xffffffff],         bits : 31_0
*/
#define IME_RESERVED_214_OFS 0x0a54
REGDEF_BEGIN(IME_RESERVED_214)
REGDEF_BIT(reserved,        32)
REGDEF_END(IME_RESERVED_214)


/*
    reserved                :    [0x0, 0xffffffff],         bits : 31_0
*/
#define IME_RESERVED_215_OFS 0x0a58
REGDEF_BEGIN(IME_RESERVED_215)
REGDEF_BIT(reserved,        32)
REGDEF_END(IME_RESERVED_215)


/*
    reserved                :    [0x0, 0xffffffff],         bits : 31_0
*/
#define IME_RESERVED_216_OFS 0x0a5c
REGDEF_BEGIN(IME_RESERVED_216)
REGDEF_BIT(reserved,        32)
REGDEF_END(IME_RESERVED_216)


/*
    ime_lcaf_filter_en :    [0x0, 0x1],         bits : 0
    ime_lcaf_yftr_en   :    [0x0, 0x1],         bits : 6
    ime_lcaf_fsize_sel :    [0x0, 0x3],         bits : 17_16
    ime_lcaf_eksize_sel:    [0x0, 0x3],         bits : 21_20
*/
#define IME_LCA_FILTER_INPUT_CONTROL_REGISTER0_OFS 0x0a60
REGDEF_BEGIN(IME_LCA_FILTER_INPUT_CONTROL_REGISTER0)
REGDEF_BIT(ime_lcaf_filter_en,        1)
REGDEF_BIT(,        5)
REGDEF_BIT(ime_lcaf_yftr_en,        1)
REGDEF_BIT(,        9)
REGDEF_BIT(ime_lcaf_fsize_sel,        2)
REGDEF_BIT(,        2)
REGDEF_BIT(ime_lcaf_eksize_sel,        2)
REGDEF_END(IME_LCA_FILTER_INPUT_CONTROL_REGISTER0)


/*
    reserved                :    [0x0, 0xffffffff],         bits : 31_0
*/
#define IME_RESERVED_217_OFS 0x0a64
REGDEF_BEGIN(IME_RESERVED_217)
REGDEF_BIT(reserved,        32)
REGDEF_END(IME_RESERVED_217)


/*
    ime_lcaf_y_rcth0:    [0x0, 0xff],           bits : 7_0
    ime_lcaf_y_rcth1:    [0x0, 0xff],           bits : 15_8
    ime_lcaf_y_rcth2:    [0x0, 0xff],           bits : 23_16
    ime_lcaf_y_cwt  :    [0x0, 0x1f],           bits : 28_24
*/
#define IME_LCA_FILTER_REFERENCE_CENTER_COMPUTATION_REGISTER0_OFS 0x0a68
REGDEF_BEGIN(IME_LCA_FILTER_REFERENCE_CENTER_COMPUTATION_REGISTER0)
REGDEF_BIT(ime_lcaf_y_rcth0,        8)
REGDEF_BIT(ime_lcaf_y_rcth1,        8)
REGDEF_BIT(ime_lcaf_y_rcth2,        8)
REGDEF_BIT(ime_lcaf_y_cwt,        5)
REGDEF_END(IME_LCA_FILTER_REFERENCE_CENTER_COMPUTATION_REGISTER0)


/*
    ime_lcaf_y_rcwt0  :    [0x0, 0xf],          bits : 3_0
    ime_lcaf_y_rcwt1  :    [0x0, 0xf],          bits : 7_4
    ime_lcaf_y_rcwt2  :    [0x0, 0xf],          bits : 11_8
    ime_lcaf_y_rcwt3  :    [0x0, 0xf],          bits : 15_12
    ime_lcaf_y_outl_th:    [0x0, 0x7],          bits : 18_16
*/
#define IME_LCA_FILTER_REFERENCE_CENTER_COMPUTATION_REGISTER1_OFS 0x0a6c
REGDEF_BEGIN(IME_LCA_FILTER_REFERENCE_CENTER_COMPUTATION_REGISTER1)
REGDEF_BIT(ime_lcaf_y_rcwt0,        4)
REGDEF_BIT(ime_lcaf_y_rcwt1,        4)
REGDEF_BIT(ime_lcaf_y_rcwt2,        4)
REGDEF_BIT(ime_lcaf_y_rcwt3,        4)
REGDEF_BIT(ime_lcaf_y_outl_th,        3)
REGDEF_END(IME_LCA_FILTER_REFERENCE_CENTER_COMPUTATION_REGISTER1)


/*
    ime_lcaf_uv_rcth0:    [0x0, 0xff],          bits : 7_0
    ime_lcaf_uv_rcth1:    [0x0, 0xff],          bits : 15_8
    ime_lcaf_uv_rcth2:    [0x0, 0xff],          bits : 23_16
    ime_lcaf_uv_cwt  :    [0x0, 0x1f],          bits : 28_24
*/
#define IME_LCA_FILTER_REFERENCE_CENTER_COMPUTATION_REGISTER2_OFS 0x0a70
REGDEF_BEGIN(IME_LCA_FILTER_REFERENCE_CENTER_COMPUTATION_REGISTER2)
REGDEF_BIT(ime_lcaf_uv_rcth0,        8)
REGDEF_BIT(ime_lcaf_uv_rcth1,        8)
REGDEF_BIT(ime_lcaf_uv_rcth2,        8)
REGDEF_BIT(ime_lcaf_uv_cwt,        5)
REGDEF_END(IME_LCA_FILTER_REFERENCE_CENTER_COMPUTATION_REGISTER2)


/*
    ime_lcaf_uv_rcwt0  :    [0x0, 0xf],         bits : 3_0
    ime_lcaf_uv_rcwt1  :    [0x0, 0xf],         bits : 7_4
    ime_lcaf_uv_rcwt2  :    [0x0, 0xf],         bits : 11_8
    ime_lcaf_uv_rcwt3  :    [0x0, 0xf],         bits : 15_12
    ime_lcaf_uv_outl_th:    [0x0, 0x7],         bits : 18_16
*/
#define IME_LCA_FILTER_REFERENCE_CENTER_COMPUTATION_REGISTER3_OFS 0x0a74
REGDEF_BEGIN(IME_LCA_FILTER_REFERENCE_CENTER_COMPUTATION_REGISTER3)
REGDEF_BIT(ime_lcaf_uv_rcwt0,        4)
REGDEF_BIT(ime_lcaf_uv_rcwt1,        4)
REGDEF_BIT(ime_lcaf_uv_rcwt2,        4)
REGDEF_BIT(ime_lcaf_uv_rcwt3,        4)
REGDEF_BIT(ime_lcaf_uv_outl_th,        3)
REGDEF_END(IME_LCA_FILTER_REFERENCE_CENTER_COMPUTATION_REGISTER3)


/*
    ime_lcaf_y_outl_dth:    [0x0, 0xff],            bits : 7_0
    ime_lcaf_u_outl_dth:    [0x0, 0xff],            bits : 15_8
    ime_lcaf_v_outl_dth:    [0x0, 0xff],            bits : 23_16
*/
#define IME_LCA_FILTER_REFERENCE_CENTER_OUTLIER_DIFFERENCE_THRESHOLD_REGISTER0_OFS 0x0a78
REGDEF_BEGIN(IME_LCA_FILTER_REFERENCE_CENTER_OUTLIER_DIFFERENCE_THRESHOLD_REGISTER0)
REGDEF_BIT(ime_lcaf_y_outl_dth,        8)
REGDEF_BIT(ime_lcaf_u_outl_dth,        8)
REGDEF_BIT(ime_lcaf_v_outl_dth,        8)
REGDEF_END(IME_LCA_FILTER_REFERENCE_CENTER_OUTLIER_DIFFERENCE_THRESHOLD_REGISTER0)


/*
    ime_lcaf_ed_pn_th:    [0x0, 0xff],          bits : 7_0
    ime_lcaf_ed_hv_th:    [0x0, 0xff],          bits : 15_8
*/
#define IME_LCA_FILTER_EDGE_DIRECTION_THRESHOLD_REGISTER0_OFS 0x0a7c
REGDEF_BEGIN(IME_LCA_FILTER_EDGE_DIRECTION_THRESHOLD_REGISTER0)
REGDEF_BIT(ime_lcaf_ed_pn_th,        8)
REGDEF_BIT(ime_lcaf_ed_hv_th,        8)
REGDEF_END(IME_LCA_FILTER_EDGE_DIRECTION_THRESHOLD_REGISTER0)


/*
    ime_lcaf_y_fth0:    [0x0, 0xff],            bits : 7_0
    ime_lcaf_y_fth1:    [0x0, 0xff],            bits : 15_8
    ime_lcaf_y_fth2:    [0x0, 0xff],            bits : 23_16
    ime_lcaf_y_fth3:    [0x0, 0xff],            bits : 31_24
*/
#define IME_LCA_FILTER_FILTER_COMPUTATION_FOR_Y_CHANNEL_REGISTER0_OFS 0x0a80
REGDEF_BEGIN(IME_LCA_FILTER_FILTER_COMPUTATION_FOR_Y_CHANNEL_REGISTER0)
REGDEF_BIT(ime_lcaf_y_fth0,        8)
REGDEF_BIT(ime_lcaf_y_fth1,        8)
REGDEF_BIT(ime_lcaf_y_fth2,        8)
REGDEF_BIT(ime_lcaf_y_fth3,        8)
REGDEF_END(IME_LCA_FILTER_FILTER_COMPUTATION_FOR_Y_CHANNEL_REGISTER0)


/*
    ime_lcaf_y_fth4:    [0x0, 0xff],            bits : 7_0
*/
#define IME_LCA_FILTER_FILTER_COMPUTATION_FOR_Y_CHANNEL_REGISTER1_OFS 0x0a84
REGDEF_BEGIN(IME_LCA_FILTER_FILTER_COMPUTATION_FOR_Y_CHANNEL_REGISTER1)
REGDEF_BIT(ime_lcaf_y_fth4,        8)
REGDEF_END(IME_LCA_FILTER_FILTER_COMPUTATION_FOR_Y_CHANNEL_REGISTER1)


/*
    ime_lcaf_y_fwt0:    [0x0, 0x1f],            bits : 4_0
    ime_lcaf_y_fwt1:    [0x0, 0x1f],            bits : 9_5
    ime_lcaf_y_fwt2:    [0x0, 0x1f],            bits : 14_10
    ime_lcaf_y_fwt3:    [0x0, 0x1f],            bits : 19_15
    ime_lcaf_y_fwt4:    [0x0, 0x1f],            bits : 24_20
    ime_lcaf_y_fwt5:    [0x0, 0x1f],            bits : 29_25
*/
#define IME_LCA_FILTER_FILTER_COMPUTATION_FOR_Y_CHANNEL_REGISTER2_OFS 0x0a88
REGDEF_BEGIN(IME_LCA_FILTER_FILTER_COMPUTATION_FOR_Y_CHANNEL_REGISTER2)
REGDEF_BIT(ime_lcaf_y_fwt0,        5)
REGDEF_BIT(ime_lcaf_y_fwt1,        5)
REGDEF_BIT(ime_lcaf_y_fwt2,        5)
REGDEF_BIT(ime_lcaf_y_fwt3,        5)
REGDEF_BIT(ime_lcaf_y_fwt4,        5)
REGDEF_BIT(ime_lcaf_y_fwt5,        5)
REGDEF_END(IME_LCA_FILTER_FILTER_COMPUTATION_FOR_Y_CHANNEL_REGISTER2)


/*
    ime_lcaf_u_fth0:    [0x0, 0xff],            bits : 7_0
    ime_lcaf_u_fth1:    [0x0, 0xff],            bits : 15_8
    ime_lcaf_u_fth2:    [0x0, 0xff],            bits : 23_16
    ime_lcaf_u_fth3:    [0x0, 0xff],            bits : 31_24
*/
#define IME_LCA_FILTER_FILTER_COMPUTATION_FOR_U_CHANNEL_REGISTER0_OFS 0x0a8c
REGDEF_BEGIN(IME_LCA_FILTER_FILTER_COMPUTATION_FOR_U_CHANNEL_REGISTER0)
REGDEF_BIT(ime_lcaf_u_fth0,        8)
REGDEF_BIT(ime_lcaf_u_fth1,        8)
REGDEF_BIT(ime_lcaf_u_fth2,        8)
REGDEF_BIT(ime_lcaf_u_fth3,        8)
REGDEF_END(IME_LCA_FILTER_FILTER_COMPUTATION_FOR_U_CHANNEL_REGISTER0)


/*
    ime_lcaf_u_fth4:    [0x0, 0xff],            bits : 7_0
*/
#define IME_LCA_FILTER_FILTER_COMPUTATION_FOR_U_CHANNEL_REGISTER1_OFS 0x0a90
REGDEF_BEGIN(IME_LCA_FILTER_FILTER_COMPUTATION_FOR_U_CHANNEL_REGISTER1)
REGDEF_BIT(ime_lcaf_u_fth4,        8)
REGDEF_END(IME_LCA_FILTER_FILTER_COMPUTATION_FOR_U_CHANNEL_REGISTER1)


/*
    ime_lcaf_u_fwt0:    [0x0, 0x1f],            bits : 4_0
    ime_lcaf_u_fwt1:    [0x0, 0x1f],            bits : 9_5
    ime_lcaf_u_fwt2:    [0x0, 0x1f],            bits : 14_10
    ime_lcaf_u_fwt3:    [0x0, 0x1f],            bits : 19_15
    ime_lcaf_u_fwt4:    [0x0, 0x1f],            bits : 24_20
    ime_lcaf_u_fwt5:    [0x0, 0x1f],            bits : 29_25
*/
#define IME_LCA_FILTER_FILTER_COMPUTATION_FOR_U_CHANNEL_REGISTER2_OFS 0x0a94
REGDEF_BEGIN(IME_LCA_FILTER_FILTER_COMPUTATION_FOR_U_CHANNEL_REGISTER2)
REGDEF_BIT(ime_lcaf_u_fwt0,        5)
REGDEF_BIT(ime_lcaf_u_fwt1,        5)
REGDEF_BIT(ime_lcaf_u_fwt2,        5)
REGDEF_BIT(ime_lcaf_u_fwt3,        5)
REGDEF_BIT(ime_lcaf_u_fwt4,        5)
REGDEF_BIT(ime_lcaf_u_fwt5,        5)
REGDEF_END(IME_LCA_FILTER_FILTER_COMPUTATION_FOR_U_CHANNEL_REGISTER2)


/*
    ime_lcaf_v_fth0:    [0x0, 0xff],            bits : 7_0
    ime_lcaf_v_fth1:    [0x0, 0xff],            bits : 15_8
    ime_lcaf_v_fth2:    [0x0, 0xff],            bits : 23_16
    ime_lcaf_v_fth3:    [0x0, 0xff],            bits : 31_24
*/
#define IME_LCA_FILTER_FILTER_COMPUTATION_FOR_V_CHANNEL_REGISTER0_OFS 0x0a98
REGDEF_BEGIN(IME_LCA_FILTER_FILTER_COMPUTATION_FOR_V_CHANNEL_REGISTER0)
REGDEF_BIT(ime_lcaf_v_fth0,        8)
REGDEF_BIT(ime_lcaf_v_fth1,        8)
REGDEF_BIT(ime_lcaf_v_fth2,        8)
REGDEF_BIT(ime_lcaf_v_fth3,        8)
REGDEF_END(IME_LCA_FILTER_FILTER_COMPUTATION_FOR_V_CHANNEL_REGISTER0)


/*
    ime_lcaf_v_fth4:    [0x0, 0xff],            bits : 7_0
*/
#define IME_LCA_FILTER_FILTER_COMPUTATION_FOR_V_CHANNEL_REGISTER1_OFS 0x0a9c
REGDEF_BEGIN(IME_LCA_FILTER_FILTER_COMPUTATION_FOR_V_CHANNEL_REGISTER1)
REGDEF_BIT(ime_lcaf_v_fth4,        8)
REGDEF_END(IME_LCA_FILTER_FILTER_COMPUTATION_FOR_V_CHANNEL_REGISTER1)


/*
    ime_lcaf_v_fwt0:    [0x0, 0x1f],            bits : 4_0
    ime_lcaf_v_fwt1:    [0x0, 0x1f],            bits : 9_5
    ime_lcaf_v_fwt2:    [0x0, 0x1f],            bits : 14_10
    ime_lcaf_v_fwt3:    [0x0, 0x1f],            bits : 19_15
    ime_lcaf_v_fwt4:    [0x0, 0x1f],            bits : 24_20
    ime_lcaf_v_fwt5:    [0x0, 0x1f],            bits : 29_25
*/
#define IME_LCA_FILTER_FILTER_COMPUTATION_FOR_V_CHANNEL_REGISTER2_OFS 0x0aa0
REGDEF_BEGIN(IME_LCA_FILTER_FILTER_COMPUTATION_FOR_V_CHANNEL_REGISTER2)
REGDEF_BIT(ime_lcaf_v_fwt0,        5)
REGDEF_BIT(ime_lcaf_v_fwt1,        5)
REGDEF_BIT(ime_lcaf_v_fwt2,        5)
REGDEF_BIT(ime_lcaf_v_fwt3,        5)
REGDEF_BIT(ime_lcaf_v_fwt4,        5)
REGDEF_BIT(ime_lcaf_v_fwt5,        5)
REGDEF_END(IME_LCA_FILTER_FILTER_COMPUTATION_FOR_V_CHANNEL_REGISTER2)


/*
    ime_lcaf_stl_u_th0:    [0x0, 0xff],         bits : 7_0
    ime_lcaf_stl_u_th1:    [0x0, 0xff],         bits : 15_8
*/
#define IME_LCA_FILTER_U_CHANNEL_GRAY_STATISTICAL_INFORMATION_REGISTER0_OFS 0x0aa4
REGDEF_BEGIN(IME_LCA_FILTER_U_CHANNEL_GRAY_STATISTICAL_INFORMATION_REGISTER0)
REGDEF_BIT(ime_lcaf_stl_u_th0,        8)
REGDEF_BIT(ime_lcaf_stl_u_th1,        8)
REGDEF_END(IME_LCA_FILTER_U_CHANNEL_GRAY_STATISTICAL_INFORMATION_REGISTER0)


/*
    ime_lcaf_stl_u_sum0:    [0x0, 0x7ffff],         bits : 18_0
*/
#define IME_LCA_FILTER_U_CHANNEL_GRAY_STATISTICAL_INFORMATION_REGISTER1_OFS 0x0aa8
REGDEF_BEGIN(IME_LCA_FILTER_U_CHANNEL_GRAY_STATISTICAL_INFORMATION_REGISTER1)
REGDEF_BIT(ime_lcaf_stl_u_sum0,        19)
REGDEF_END(IME_LCA_FILTER_U_CHANNEL_GRAY_STATISTICAL_INFORMATION_REGISTER1)


/*
    ime_lcaf_stl_u_sum1:    [0x0, 0x3ffff],         bits : 17_0
*/
#define IME_LCA_FILTER_U_CHANNEL_GRAY_STATISTICAL_INFORMATION_REGISTER2_OFS 0x0aac
REGDEF_BEGIN(IME_LCA_FILTER_U_CHANNEL_GRAY_STATISTICAL_INFORMATION_REGISTER2)
REGDEF_BIT(ime_lcaf_stl_u_sum1,        18)
REGDEF_END(IME_LCA_FILTER_U_CHANNEL_GRAY_STATISTICAL_INFORMATION_REGISTER2)


/*
    ime_lcaf_stl_v_th0:    [0x0, 0xff],         bits : 7_0
    ime_lcaf_stl_v_th1:    [0x0, 0xff],         bits : 15_8
*/
#define IME_LCA_FILTER_V_CHANNEL_GRAY_STATISTICAL_INFORMATION_REGISTER0_OFS 0x0ab0
REGDEF_BEGIN(IME_LCA_FILTER_V_CHANNEL_GRAY_STATISTICAL_INFORMATION_REGISTER0)
REGDEF_BIT(ime_lcaf_stl_v_th0,        8)
REGDEF_BIT(ime_lcaf_stl_v_th1,        8)
REGDEF_END(IME_LCA_FILTER_V_CHANNEL_GRAY_STATISTICAL_INFORMATION_REGISTER0)


/*
    ime_lcaf_stl_v_sum0:    [0x0, 0x7ffff],         bits : 18_0
*/
#define IME_LCA_FILTER_V_CHANNEL_GRAY_STATISTICAL_INFORMATION_REGISTER1_OFS 0x0ab4
REGDEF_BEGIN(IME_LCA_FILTER_V_CHANNEL_GRAY_STATISTICAL_INFORMATION_REGISTER1)
REGDEF_BIT(ime_lcaf_stl_v_sum0,        19)
REGDEF_END(IME_LCA_FILTER_V_CHANNEL_GRAY_STATISTICAL_INFORMATION_REGISTER1)


/*
    ime_lcaf_stl_v_sum1:    [0x0, 0x3ffff],         bits : 17_0
*/
#define IME_LCA_FILTER_V_CHANNEL_GRAY_STATISTICAL_INFORMATION_REGISTER2_OFS 0x0ab8
REGDEF_BEGIN(IME_LCA_FILTER_V_CHANNEL_GRAY_STATISTICAL_INFORMATION_REGISTER2)
REGDEF_BIT(ime_lcaf_stl_v_sum1,        18)
REGDEF_END(IME_LCA_FILTER_V_CHANNEL_GRAY_STATISTICAL_INFORMATION_REGISTER2)


/*
    ime_lcaf_stl_cnt:    [0x0, 0xfffffff],          bits : 27_0
*/
#define IME_LCA_FILTER_U_CHANNEL_GRAY_STATISTICAL_INFORMATION_REGISTER3_OFS 0x0abc
REGDEF_BEGIN(IME_LCA_FILTER_U_CHANNEL_GRAY_STATISTICAL_INFORMATION_REGISTER3)
REGDEF_BIT(ime_lcaf_stl_cnt,        28)
REGDEF_END(IME_LCA_FILTER_U_CHANNEL_GRAY_STATISTICAL_INFORMATION_REGISTER3)

//---------------------------------------------------------------------------

#if 1

typedef struct _NT98560_IME_REGISTER_STRUCT_ {

	union {
		struct {
			unsigned ime_rst                                : 1;      // bits : 0
			unsigned ime_start                                : 1;        // bits : 1
			unsigned ime_start_tload                          : 1;     // bits : 2
			unsigned ime_frameend_tload                        : 1;        // bits : 3
			unsigned ime_drt_start_load           : 1;        // bits : 4
			unsigned ime_gbl_load_en              : 1;      // bits : 5
			unsigned                              : 21;
			unsigned ime_dmach_dis                : 1;      // bits : 27
			unsigned ime_ll_fire                  : 1;      // bits : 28
		} bit;
		UINT32 word;
	} reg_0; // 0x0000

	union {
		struct {
			unsigned ime_src                                  : 1;        // bits : 0
			unsigned ime_dir_ctrl                            : 1;     // bits : 1
			unsigned ime_p1_en                                  : 1;      // bits : 2
			unsigned ime_p2_en                                  : 1;      // bits : 3
			unsigned ime_p3_en                                  : 1;      // bits : 4
			//unsigned ime_p4_en                                  : 1;      // bits : 5
			unsigned                                : 5;
			unsigned ime_chra_en                              : 1;        // bits : 10
			unsigned ime_chra_ca_en                            : 1;       // bits : 11
			unsigned ime_chra_la_en                            : 1;       // bits : 12
			unsigned ime_dbcs_en                              : 1;        // bits : 13
			//unsigned ime_stl_en                                : 1;       // bits : 14
			//unsigned ime_stl_ftr_en                            : 1;       // bits : 15
			unsigned                                : 2;
			unsigned ime_ds_cst_en                              : 1;      // bits : 16
			unsigned ime_ds_en0                                : 1;       // bits : 17
			unsigned ime_ds_en1                                : 1;       // bits : 18
			unsigned ime_ds_en2                                : 1;       // bits : 19
			unsigned ime_ds_en3                                : 1;       // bits : 20
			unsigned ime_ds_plt_sel                 : 2;      // bits : 22_21
			unsigned                                : 2;
			unsigned ime_chra_subout_en             : 1;      // bits : 25
			unsigned ime_p1_enc_en                  : 1;      // bits : 26
			unsigned ime_3dnr_en                    : 1;      // bits : 27
			unsigned                                : 1;
			unsigned ime_tmnr_ref_in_dec_en         : 1;      // bits : 29
			unsigned ime_tmnr_ref_out_en            : 1;      // bits : 30
			unsigned ime_tmnr_ref_out_enc_en        : 1;      // bits : 31
		} bit;
		UINT32 word;
	} reg_1; // 0x0004

	union {
		struct {
			unsigned ime_pm0_en                                : 1;       // bits : 0
			unsigned ime_pm1_en                                : 1;       // bits : 1
			unsigned ime_pm2_en                                : 1;       // bits : 2
			unsigned ime_pm3_en                                : 1;       // bits : 3
			unsigned ime_pm4_en                                : 1;       // bits : 4
			unsigned ime_pm5_en                                : 1;       // bits : 5
			unsigned ime_pm6_en                                : 1;       // bits : 6
			unsigned ime_pm7_en                                : 1;       // bits : 7
			unsigned ime_low_dly_en                 : 1;      // bits : 8
			unsigned ime_low_dly_sel                : 2;      // bits : 10_9
			unsigned ime_p1_flip_en                 : 1;      // bits : 11
			unsigned ime_p2_flip_en                 : 1;      // bits : 12
			unsigned ime_p3_flip_en                 : 1;      // bits : 13
			//unsigned ime_p4_flip_en                 : 1;      // bits : 14
			//unsigned ime_stl_flip_en                : 1;      // bits : 15
			unsigned                                : 2;
			unsigned ime_3dnr_inref_flip_en         : 1;      // bits : 16
			unsigned ime_3dnr_outref_flip_en        : 1;      // bits : 17
			unsigned ime_3dnr_ms_roi_flip_en        : 1;      // bits : 18
			unsigned                                : 1;
			unsigned ime_ycc_cvt_en                 : 1;      // bits : 20
			unsigned ime_ycc_cvt_sel                : 1;      // bits : 21
		} bit;
		UINT32 word;
	} reg_2; // 0x0008

	union {
		struct {
			unsigned ime_lca_subout_dram_out_single_en         : 1;       // bits : 0
			unsigned ime_3dnr_refout_dram_out_single_en        : 1;       // bits : 1
			unsigned ime_3dnr_ms_dram_out_single_en            : 1;       // bits : 2
			unsigned ime_3dnr_ms_roi_dram_out_single_en        : 1;       // bits : 3
			unsigned ime_3dnr_mv_dram_out_single_en            : 1;       // bits : 4
			unsigned ime_3dnr_sta_dram_out_single_en           : 1;       // bits : 5
			unsigned ime_outp1_dram_out_single_en              : 1;       // bits : 6
			unsigned ime_outp2_dram_out_single_en              : 1;       // bits : 7
			unsigned ime_outp3_dram_out_single_en              : 1;       // bits : 8
			//unsigned ime_outp4_dram_out_single_en              : 1;       // bits : 9
			unsigned                                : 1;
			unsigned ime_3dnr_fc_dram_out_single_en            : 1;     // bits : 10
			unsigned                                           : 20;
			unsigned ime_dram_out_mode                         : 1;       // bits : 31
		} bit;
		UINT32 word;
	} reg_3; // 0x000c

	union {
		struct {
			unsigned ime_ll_terminate        : 1;       // bits : 0
			unsigned                         : 3;
			unsigned ime_dmach_idle          : 1;       // bits : 4
		} bit;
		UINT32 word;
	} reg_4; // 0x0010

	union {
		struct {
			unsigned                        : 2;
			unsigned ime_dram_ll_sai        : 30;     // bits : 31_2
		} bit;
		UINT32 word;
	} reg_5; // 0x0014

	union {
		struct {
			unsigned ime_intpe_ll_end                    : 1;      // bits : 0
			unsigned ime_intpe_ll_err                    : 1;      // bits : 1
			unsigned ime_intpe_ll_red_late               : 1;      // bits : 2
			unsigned ime_intpe_ll_job_end                : 1;      // bits : 3
			unsigned ime_intpe_in_bp1                    : 1;      // bits : 4
			unsigned ime_intpe_in_bp2                    : 1;      // bits : 5
			unsigned ime_intpe_in_bp3                    : 1;      // bits : 6
			unsigned ime_intpe_3dnr_slice_end            : 1;      // bits : 7
			unsigned ime_intpe_3dnr_mot_end              : 1;      // bits : 8
			unsigned ime_intpe_3dnr_mv_end               : 1;      // bits : 9
			unsigned ime_intpe_3dnr_statsitic_end        : 1;      // bits : 10
			unsigned ime_intpe_p1_out_enc_ovfl           : 1;      // bits : 11
			unsigned ime_intpe_3dnr_ref_out_enc_ovfl        : 1;     // bits : 12
			unsigned ime_intpe_3dnr_ref_in_dec_err          : 1;     // bits : 13
			unsigned ime_intpe_frm_err                      : 1;     // bits : 14
			unsigned reserved                              : 14;        // bits : 28_15
			unsigned ime_intpe_frm_start                    : 1;     // bits : 29
			unsigned ime_intpe_strp_end                     : 1;     // bits : 30
			unsigned ime_intpe_frm_end                      : 1;     // bits : 31
		} bit;
		UINT32 word;
	} reg_6; // 0x0018

	union {
		struct {
			unsigned ime_intps_ll_end                    : 1;      // bits : 0
			unsigned ime_intps_ll_err                    : 1;      // bits : 1
			unsigned ime_intps_ll_red_late               : 1;      // bits : 2
			unsigned ime_intps_ll_job_end                : 1;      // bits : 3
			unsigned ime_intps_in_bp1                    : 1;      // bits : 4
			unsigned ime_intps_in_bp2                    : 1;      // bits : 5
			unsigned ime_intps_in_bp3                    : 1;      // bits : 6
			unsigned ime_intps_3dnr_slice_end            : 1;      // bits : 7
			unsigned ime_intps_3dnr_mot_end              : 1;      // bits : 8
			unsigned ime_intps_3dnr_mv_end               : 1;      // bits : 9
			unsigned ime_intps_3dnr_statsitic_end        : 1;      // bits : 10
			unsigned ime_intps_p1_out_enc_ovfl           : 1;      // bits : 11
			unsigned ime_intps_3dnr_ref_out_enc_ovfl        : 1;     // bits : 12
			unsigned ime_intps_3dnr_ref_in_dec_err          : 1;     // bits : 13
			unsigned ime_intps_frm_err                      : 1;     // bits : 14
			unsigned reserved                              : 14;        // bits : 28_15
			unsigned ime_intps_frm_start                    : 1;     // bits : 29
			unsigned ime_intps_strp_end                     : 1;     // bits : 30
			unsigned ime_intps_frm_end                      : 1;     // bits : 31
		} bit;
		UINT32 word;
	} reg_7; // 0x001c

	union {
		struct {
			unsigned                          : 2;
			unsigned ime_in_h_size                        : 14;       // bits : 15_2
			unsigned                          : 2;
			unsigned ime_in_v_size                        : 14;       // bits : 31_18
		} bit;
		UINT32 word;
	} reg_8; // 0x0020

	union {
		struct {
			unsigned ime_imat                         : 3;        // bits : 2_0
			unsigned                         : 1;
			unsigned st_hovlp_sel                     : 2;        // bits : 5_4
			unsigned st_prt_sel                         : 2;      // bits : 7_6
			unsigned ime_st_size_mode        : 1;     // bits : 8
		} bit;
		UINT32 word;
	} reg_9; // 0x0024

	union {
		struct {
			unsigned st_hn                        : 11;       // bits : 10_0
			unsigned                  : 1;
			unsigned st_hl                        : 11;       // bits : 22_12
			unsigned                  : 1;
			unsigned st_hm                        : 8;        // bits : 31_24
		} bit;
		UINT32 word;
	} reg_10; // 0x0028

	union {
		struct {
			unsigned st_vl                           : 16;        // bits : 15_0
			unsigned st_prt                         : 8;      // bits : 23_16
			unsigned st_hovlp                     : 8;        // bits : 31_24
		} bit;
		UINT32 word;
	} reg_11; // 0x002c

	union {
		struct {
			unsigned                        : 2;
			unsigned ime_y_dram_ofsi        : 18;       // bits : 19_2
		} bit;
		UINT32 word;
	} reg_12; // 0x0030

	union {
		struct {
			unsigned                         : 2;
			unsigned ime_uv_dram_ofsi        : 18;      // bits : 19_2
		} bit;
		UINT32 word;
	} reg_13; // 0x0034

	union {
		struct {
			unsigned                       : 2;
			unsigned ime_y_dram_sai        : 30;        // bits : 31_2
		} bit;
		UINT32 word;
	} reg_14; // 0x0038

	union {
		struct {
			unsigned                       : 2;
			unsigned ime_u_dram_sai        : 30;        // bits : 31_2
		} bit;
		UINT32 word;
	} reg_15; // 0x003c

	union {
		struct {
			unsigned                       : 2;
			unsigned ime_v_dram_sai        : 30;        // bits : 31_2
		} bit;
		UINT32 word;
	} reg_16; // 0x0040

	union {
		struct {
			unsigned ime_st_hn0        : 10;      // bits : 9_0
			unsigned                   : 1;
			unsigned ime_st_hn1        : 10;      // bits : 20_11
			unsigned                   : 1;
			unsigned ime_st_hn2        : 10;      // bits : 31_22
		} bit;
		UINT32 word;
	} reg_17; // 0x0044

	union {
		struct {
			unsigned ime_st_hn3        : 10;      // bits : 9_0
			unsigned                   : 1;
			unsigned ime_st_hn4        : 10;      // bits : 20_11
			unsigned                   : 1;
			unsigned ime_st_hn5        : 10;      // bits : 31_22
		} bit;
		UINT32 word;
	} reg_18; // 0x0048

	union {
		struct {
			unsigned ime_st_hn6        : 10;      // bits : 9_0
			unsigned                   : 1;
			unsigned ime_st_hn7        : 10;      // bits : 20_11
			unsigned                   : 3;
			unsigned ime_st_msb_hn0    : 1;       // bits : 24
			unsigned ime_st_msb_hn1    : 1;       // bits : 25
			unsigned ime_st_msb_hn2    : 1;       // bits : 26
			unsigned ime_st_msb_hn3    : 1;       // bits : 27
			unsigned ime_st_msb_hn4    : 1;       // bits : 28
			unsigned ime_st_msb_hn5    : 1;       // bits : 29
			unsigned ime_st_msb_hn6    : 1;       // bits : 30
			unsigned ime_st_msb_hn7    : 1;       // bits : 31
		} bit;
		UINT32 word;
	} reg_19; // 0x004c

	union {
		struct {
			unsigned reserved        : 32;        // bits : 31_0
		} bit;
		UINT32 word;
	} reg_20; // 0x0050

	union {
		struct {
			unsigned reserved        : 32;        // bits : 31_0
		} bit;
		UINT32 word;
	} reg_21; // 0x0054

	union {
		struct {
			unsigned ime_dend_sts_en        : 1;        // bits : 0
			unsigned                        : 3;
			unsigned ime_dend_wbit          : 5;        // bits : 8_4
		} bit;
		UINT32 word;
	} reg_22; // 0x0058

	union {
		struct {
			unsigned ime_dend_status        : 32;       // bits : 31_0
		} bit;
		UINT32 word;
	} reg_23; // 0x005c

	union {
		struct {
			unsigned                                : 1;
			unsigned ime_out_type_p1                          : 1;        // bits : 1
			unsigned ime_scl_method_p1                          : 2;      // bits : 3_2
			unsigned ime_out_en_p1                              : 1;      // bits : 4
			unsigned ime_sprt_out_en_p1                        : 1;       // bits : 5
			unsigned                                : 2;
			unsigned ime_omat1                                  : 2;      // bits : 9_8
			unsigned                                : 2;
			unsigned ime_scl_enh_fact_p1                      : 8;        // bits : 19_12
			unsigned ime_scl_enh_bit_p1                        : 4;       // bits : 23_20
		} bit;
		UINT32 word;
	} reg_24; // 0x0060

	union {
		struct {
			unsigned h1_ud                              : 1;      // bits : 0
			unsigned v1_ud                              : 1;      // bits : 1
			unsigned h1_dnrate                          : 5;      // bits : 6_2
			unsigned v1_dnrate                          : 5;      // bits : 11_7
			unsigned                        : 3;
			unsigned h1_filtmode                      : 1;        // bits : 15
			unsigned h1_filtcoef                      : 6;        // bits : 21_16
			unsigned v1_filtmode                      : 1;        // bits : 22
			unsigned v1_filtcoef                      : 6;        // bits : 28_23
		} bit;
		UINT32 word;
	} reg_25; // 0x0064

	union {
		struct {
			unsigned h1_sfact                     : 16;       // bits : 15_0
			unsigned v1_sfact                     : 16;       // bits : 31_16
		} bit;
		UINT32 word;
	} reg_26; // 0x0068

	union {
		struct {
			unsigned reserved        : 32;        // bits : 31_0
		} bit;
		UINT32 word;
	} reg_27; // 0x006c

	union {
		struct {
			unsigned reserved        : 32;        // bits : 31_0
		} bit;
		UINT32 word;
	} reg_28; // 0x0070

	union {
		struct {
			unsigned reserved        : 32;        // bits : 31_0
		} bit;
		UINT32 word;
	} reg_29; // 0x0074

	union {
		struct {
			unsigned reserved        : 32;        // bits : 31_0
		} bit;
		UINT32 word;
	} reg_30; // 0x0078

	union {
		struct {
			unsigned h1_scl_size                      : 16;       // bits : 15_0
			unsigned v1_scl_size                      : 16;       // bits : 31_16
		} bit;
		UINT32 word;
	} reg_31; // 0x007c

	union {
		struct {
			unsigned ime_cropout_x_p1                     : 16;       // bits : 15_0
			unsigned ime_cropout_y_p1                     : 16;       // bits : 31_16
		} bit;
		UINT32 word;
	} reg_32; // 0x0080

	union {
		struct {
			unsigned h1_osize                     : 16;       // bits : 15_0
			unsigned v1_osize                     : 16;       // bits : 31_16
		} bit;
		UINT32 word;
	} reg_33; // 0x0084

	union {
		struct {
			unsigned ime_y_clamp_min_p1         : 8;      // bits : 7_0
			unsigned ime_y_clamp_max_p1         : 8;      // bits : 15_8
			unsigned ime_uv_clamp_min_p1        : 8;      // bits : 23_16
			unsigned ime_uv_clamp_max_p1        : 8;      // bits : 31_24
		} bit;
		UINT32 word;
	} reg_34; // 0x0088

	union {
		struct {
			unsigned                           : 2;
			unsigned ime_p1_y_dram_ofso        : 18;        // bits : 19_2
		} bit;
		UINT32 word;
	} reg_35; // 0x008c

	union {
		struct {
			unsigned                            : 2;
			unsigned ime_p1_uv_dram_ofso        : 18;       // bits : 19_2
		} bit;
		UINT32 word;
	} reg_36; // 0x0090

	union {
		struct {
			unsigned ime_p1_y_dram_sao        : 32;     // bits : 31_0
		} bit;
		UINT32 word;
	} reg_37; // 0x0094

	union {
		struct {
			unsigned ime_p1_u_dram_sao        : 32;     // bits : 31_0
		} bit;
		UINT32 word;
	} reg_38; // 0x0098

	union {
		struct {
			unsigned ime_p1_v_dram_sao        : 32;     // bits : 31_0
		} bit;
		UINT32 word;
	} reg_39; // 0x009c

	union {
		struct {
			unsigned reserved                     : 32;       // bits : 31_0
		} bit;
		UINT32 word;
	} reg_40; // 0x00a0

	union {
		struct {
			unsigned reserved                     : 32;       // bits : 31_0
		} bit;
		UINT32 word;
	} reg_41; // 0x00a4

	union {
		struct {
			unsigned reserved                     : 32;       // bits : 31_0
		} bit;
		UINT32 word;
	} reg_42; // 0x00a8

	union {
		struct {
			unsigned reserved                     : 32;       // bits : 31_0
		} bit;
		UINT32 word;
	} reg_43; // 0x00ac

	union {
		struct {
			unsigned                                : 1;
			unsigned ime_out_type_p2                            : 1;        // bits : 1
			unsigned ime_scl_method_p2                        : 2;      // bits : 3_2
			unsigned ime_out_en_p2                            : 1;      // bits : 4
			unsigned ime_sprt_out_en_p2                      : 1;       // bits : 5
			unsigned                                : 2;
			unsigned ime_omat2                                : 3;      // bits : 10_8
			unsigned                                : 1;
			unsigned ime_scl_enh_fact_p2                        : 8;        // bits : 19_12
			unsigned ime_scl_enh_bit_p2                      : 4;       // bits : 23_20
		} bit;
		UINT32 word;
	} reg_44; // 0x00b0

	union {
		struct {
			unsigned h2_ud                              : 1;      // bits : 0
			unsigned v2_ud                              : 1;      // bits : 1
			unsigned h2_dnrate                          : 5;      // bits : 6_2
			unsigned v2_dnrate                          : 5;      // bits : 11_7
			unsigned                        : 3;
			unsigned h2_filtmode                      : 1;        // bits : 15
			unsigned h2_filtcoef                      : 6;        // bits : 21_16
			unsigned v2_filtmode                      : 1;        // bits : 22
			unsigned v2_filtcoef                      : 6;        // bits : 28_23
		} bit;
		UINT32 word;
	} reg_45; // 0x00b4

	union {
		struct {
			unsigned h2_sfact                     : 16;       // bits : 15_0
			unsigned v2_sfact                     : 16;       // bits : 31_16
		} bit;
		UINT32 word;
	} reg_46; // 0x00b8

	union {
		struct {
			unsigned isd_h_base_p2                        : 13;       // bits : 12_0
			unsigned                          : 3;
			unsigned isd_v_base_p2                        : 13;       // bits : 28_16
		} bit;
		UINT32 word;
	} reg_47; // 0x00bc

	union {
		struct {
			unsigned isd_h_sfact0_p2                      : 13;       // bits : 12_0
			unsigned                            : 3;
			unsigned isd_v_sfact0_p2                      : 13;       // bits : 28_16
		} bit;
		UINT32 word;
	} reg_48; // 0x00c0

	union {
		struct {
			unsigned isd_h_sfact1_p2                      : 13;       // bits : 12_0
			unsigned                            : 3;
			unsigned isd_v_sfact1_p2                      : 13;       // bits : 28_16
		} bit;
		UINT32 word;
	} reg_49; // 0x00c4

	union {
		struct {
			unsigned isd_h_sfact2_p2                      : 13;       // bits : 12_0
			unsigned                            : 3;
			unsigned isd_v_sfact2_p2                      : 13;       // bits : 28_16
		} bit;
		UINT32 word;
	} reg_50; // 0x00c8

	union {
		struct {
			unsigned isd_p2_mode              : 1;        // bits : 0
			unsigned                          : 7;
			unsigned isd_p2_h_coef_num        : 5;        // bits : 12_8
			unsigned                          : 3;
			unsigned isd_p2_v_coef_num        : 5;        // bits : 20_16
		} bit;
		UINT32 word;
	} reg_51; // 0x00cc

	union {
		struct {
			unsigned isd_p2_h_coef0        : 8;       // bits : 7_0
			unsigned isd_p2_h_coef1        : 8;       // bits : 15_8
			unsigned isd_p2_h_coef2        : 8;       // bits : 23_16
			unsigned isd_p2_h_coef3        : 8;       // bits : 31_24
		} bit;
		UINT32 word;
	} reg_52; // 0x00d0

	union {
		struct {
			unsigned isd_p2_h_coef4        : 8;       // bits : 7_0
			unsigned isd_p2_h_coef5        : 8;       // bits : 15_8
			unsigned isd_p2_h_coef6        : 8;       // bits : 23_16
			unsigned isd_p2_h_coef7        : 8;       // bits : 31_24
		} bit;
		UINT32 word;
	} reg_53; // 0x00d4

	union {
		struct {
			unsigned isd_p2_h_coef8         : 8;      // bits : 7_0
			unsigned isd_p2_h_coef9         : 8;      // bits : 15_8
			unsigned isd_p2_h_coef10        : 8;      // bits : 23_16
			unsigned isd_p2_h_coef11        : 8;      // bits : 31_24
		} bit;
		UINT32 word;
	} reg_54; // 0x00d8

	union {
		struct {
			unsigned isd_p2_h_coef12        : 8;      // bits : 7_0
			unsigned isd_p2_h_coef13        : 8;      // bits : 15_8
			unsigned isd_p2_h_coef14        : 8;      // bits : 23_16
			unsigned isd_p2_h_coef15        : 8;      // bits : 31_24
		} bit;
		UINT32 word;
	} reg_55; // 0x00dc

	union {
		struct {
			unsigned isd_p2_v_coef0        : 8;       // bits : 7_0
			unsigned isd_p2_v_coef1        : 8;       // bits : 15_8
			unsigned isd_p2_v_coef2        : 8;       // bits : 23_16
			unsigned isd_p2_v_coef3        : 8;       // bits : 31_24
		} bit;
		UINT32 word;
	} reg_56; // 0x00e0

	union {
		struct {
			unsigned isd_p2_v_coef4        : 8;       // bits : 7_0
			unsigned isd_p2_v_coef5        : 8;       // bits : 15_8
			unsigned isd_p2_v_coef6        : 8;       // bits : 23_16
			unsigned isd_p2_v_coef7        : 8;       // bits : 31_24
		} bit;
		UINT32 word;
	} reg_57; // 0x00e4

	union {
		struct {
			unsigned isd_p2_v_coef8         : 8;      // bits : 7_0
			unsigned isd_p2_v_coef9         : 8;      // bits : 15_8
			unsigned isd_p2_v_coef10        : 8;      // bits : 23_16
			unsigned isd_p2_v_coef11        : 8;      // bits : 31_24
		} bit;
		UINT32 word;
	} reg_58; // 0x00e8

	union {
		struct {
			unsigned isd_p2_v_coef12        : 8;      // bits : 7_0
			unsigned isd_p2_v_coef13        : 8;      // bits : 15_8
			unsigned isd_p2_v_coef14        : 8;      // bits : 23_16
			unsigned isd_p2_v_coef15        : 8;      // bits : 31_24
		} bit;
		UINT32 word;
	} reg_59; // 0x00ec

	union {
		struct {
			unsigned isd_p2_h_coef16        : 12;     // bits : 11_0
			unsigned                        : 4;
			unsigned isd_p2_v_coef16        : 12;     // bits : 27_16
		} bit;
		UINT32 word;
	} reg_60; // 0x00f0

	union {
		struct {
			unsigned isd_p2_h_coef_msb0        : 4;       // bits : 3_0
			unsigned isd_p2_h_coef_msb1        : 4;       // bits : 7_4
			unsigned isd_p2_h_coef_msb2        : 4;       // bits : 11_8
			unsigned isd_p2_h_coef_msb3        : 4;       // bits : 15_12
			unsigned isd_p2_h_coef_msb4        : 4;       // bits : 19_16
			unsigned isd_p2_h_coef_msb5        : 4;       // bits : 23_20
			unsigned isd_p2_h_coef_msb6        : 4;       // bits : 27_24
			unsigned isd_p2_h_coef_msb7        : 4;       // bits : 31_28
		} bit;
		UINT32 word;
	} reg_61; // 0x00f4

	union {
		struct {
			unsigned isd_p2_h_coef_msb8         : 4;      // bits : 3_0
			unsigned isd_p2_h_coef_msb9         : 4;      // bits : 7_4
			unsigned isd_p2_h_coef_msb10        : 4;      // bits : 11_8
			unsigned isd_p2_h_coef_msb11        : 4;      // bits : 15_12
			unsigned isd_p2_h_coef_msb12        : 4;      // bits : 19_16
			unsigned isd_p2_h_coef_msb13        : 4;      // bits : 23_20
			unsigned isd_p2_h_coef_msb14        : 4;      // bits : 27_24
			unsigned isd_p2_h_coef_msb15        : 4;      // bits : 31_28
		} bit;
		UINT32 word;
	} reg_62; // 0x00f8

	union {
		struct {
			unsigned isd_p2_v_coef_msb0        : 4;       // bits : 3_0
			unsigned isd_p2_v_coef_msb1        : 4;       // bits : 7_4
			unsigned isd_p2_v_coef_msb2        : 4;       // bits : 11_8
			unsigned isd_p2_v_coef_msb3        : 4;       // bits : 15_12
			unsigned isd_p2_v_coef_msb4        : 4;       // bits : 19_16
			unsigned isd_p2_v_coef_msb5        : 4;       // bits : 23_20
			unsigned isd_p2_v_coef_msb6        : 4;       // bits : 27_24
			unsigned isd_p2_v_coef_msb7        : 4;       // bits : 31_28
		} bit;
		UINT32 word;
	} reg_63; // 0x00fc

	union {
		struct {
			unsigned isd_p2_v_coef_msb8         : 4;      // bits : 3_0
			unsigned isd_p2_v_coef_msb9         : 4;      // bits : 7_4
			unsigned isd_p2_v_coef_msb10        : 4;      // bits : 11_8
			unsigned isd_p2_v_coef_msb11        : 4;      // bits : 15_12
			unsigned isd_p2_v_coef_msb12        : 4;      // bits : 19_16
			unsigned isd_p2_v_coef_msb13        : 4;      // bits : 23_20
			unsigned isd_p2_v_coef_msb14        : 4;      // bits : 27_24
			unsigned isd_p2_v_coef_msb15        : 4;      // bits : 31_28
		} bit;
		UINT32 word;
	} reg_64; // 0x0100

	union {
		struct {
			unsigned h2_scl_size                      : 16;       // bits : 15_0
			unsigned v2_scl_size                      : 16;       // bits : 31_16
		} bit;
		UINT32 word;
	} reg_65; // 0x0104

	union {
		struct {
			unsigned ime_cropout_x_p2                     : 16;       // bits : 15_0
			unsigned ime_cropout_y_p2                     : 16;       // bits : 31_16
		} bit;
		UINT32 word;
	} reg_66; // 0x0108

	union {
		struct {
			unsigned h2_osize                     : 16;       // bits : 15_0
			unsigned v2_osize                     : 16;       // bits : 31_16
		} bit;
		UINT32 word;
	} reg_67; // 0x010c

	union {
		struct {
			unsigned ime_y_clamp_min_p2         : 8;      // bits : 7_0
			unsigned ime_y_clamp_max_p2         : 8;      // bits : 15_8
			unsigned ime_uv_clamp_min_p2        : 8;      // bits : 23_16
			unsigned ime_uv_clamp_max_p2        : 8;      // bits : 31_24
		} bit;
		UINT32 word;
	} reg_68; // 0x0110

	union {
		struct {
			unsigned                           : 2;
			unsigned ime_p2_y_dram_ofso        : 18;        // bits : 19_2
		} bit;
		UINT32 word;
	} reg_69; // 0x0114

	union {
		struct {
			unsigned                            : 2;
			unsigned ime_p2_uv_dram_ofso        : 18;       // bits : 19_2
		} bit;
		UINT32 word;
	} reg_70; // 0x0118

	union {
		struct {
			unsigned ime_p2_y_dram_sao        : 32;     // bits : 31_0
		} bit;
		UINT32 word;
	} reg_71; // 0x011c

	union {
		struct {
			unsigned ime_p2_uv_dram_sao        : 32;        // bits : 31_0
		} bit;
		UINT32 word;
	} reg_72; // 0x0120

	union {
		struct {
			unsigned reserved                     : 32;       // bits : 31_0
		} bit;
		UINT32 word;
	} reg_73; // 0x0124

	union {
		struct {
			unsigned reserved                     : 32;       // bits : 31_0
		} bit;
		UINT32 word;
	} reg_74; // 0x0128

	union {
		struct {
			unsigned reserved                     : 32;       // bits : 31_0
		} bit;
		UINT32 word;
	} reg_75; // 0x012c

	union {
		struct {
			unsigned                                : 1;
			unsigned ime_out_type_p3                            : 1;        // bits : 1
			unsigned ime_scl_method_p3              : 2;        // bits : 3_2
			unsigned ime_out_en_p3                            : 1;      // bits : 4
			unsigned ime_sprt_out_en_p3                      : 1;       // bits : 5
			unsigned                                : 2;
			unsigned ime_omat3                                : 3;      // bits : 10_8
			unsigned                                : 1;
			unsigned ime_scl_enh_fact_p3                        : 8;        // bits : 19_12
			unsigned ime_scl_enh_bit_p3                      : 4;       // bits : 23_20
		} bit;
		UINT32 word;
	} reg_76; // 0x0130

	union {
		struct {
			unsigned h3_ud                              : 1;      // bits : 0
			unsigned v3_ud                              : 1;      // bits : 1
			unsigned h3_dnrate                          : 5;      // bits : 6_2
			unsigned v3_dnrate                          : 5;      // bits : 11_7
			unsigned                        : 3;
			unsigned h3_filtmode                      : 1;        // bits : 15
			unsigned h3_filtcoef                      : 6;        // bits : 21_16
			unsigned v3_filtmode                      : 1;        // bits : 22
			unsigned v3_filtcoef                      : 6;        // bits : 28_23
		} bit;
		UINT32 word;
	} reg_77; // 0x0134

	union {
		struct {
			unsigned h3_sfact                     : 16;       // bits : 15_0
			unsigned v3_sfact                     : 16;       // bits : 31_16
		} bit;
		UINT32 word;
	} reg_78; // 0x0138

	union {
		struct {
			unsigned isd_h_base_p3                        : 13;       // bits : 12_0
			unsigned                          : 3;
			unsigned isd_v_base_p3                        : 13;       // bits : 28_16
		} bit;
		UINT32 word;
	} reg_79; // 0x013c

	union {
		struct {
			unsigned isd_h_sfact0_p3                      : 13;       // bits : 12_0
			unsigned                            : 3;
			unsigned isd_v_sfact0_p3                      : 13;       // bits : 28_16
		} bit;
		UINT32 word;
	} reg_80; // 0x0140

	union {
		struct {
			unsigned isd_h_sfact1_p3                      : 13;       // bits : 12_0
			unsigned                            : 3;
			unsigned isd_v_sfact1_p3                      : 13;       // bits : 28_16
		} bit;
		UINT32 word;
	} reg_81; // 0x0144

	union {
		struct {
			unsigned isd_h_sfact2_p3                      : 13;       // bits : 12_0
			unsigned                            : 3;
			unsigned isd_v_sfact2_p3                      : 13;       // bits : 28_16
		} bit;
		UINT32 word;
	} reg_82; // 0x0148

	union {
		struct {
			unsigned isd_p3_mode              : 1;        // bits : 0
			unsigned                          : 7;
			unsigned isd_p3_h_coef_num        : 5;        // bits : 12_8
			unsigned                          : 3;
			unsigned isd_p3_v_coef_num        : 5;        // bits : 20_16
		} bit;
		UINT32 word;
	} reg_83; // 0x014c

	union {
		struct {
			unsigned isd_p3_h_coef0        : 8;       // bits : 7_0
			unsigned isd_p3_h_coef1        : 8;       // bits : 15_8
			unsigned isd_p3_h_coef2        : 8;       // bits : 23_16
			unsigned isd_p3_h_coef3        : 8;       // bits : 31_24
		} bit;
		UINT32 word;
	} reg_84; // 0x0150

	union {
		struct {
			unsigned isd_p3_h_coef4        : 8;       // bits : 7_0
			unsigned isd_p3_h_coef5        : 8;       // bits : 15_8
			unsigned isd_p3_h_coef6        : 8;       // bits : 23_16
			unsigned isd_p3_h_coef7        : 8;       // bits : 31_24
		} bit;
		UINT32 word;
	} reg_85; // 0x0154

	union {
		struct {
			unsigned isd_p3_h_coef8         : 8;      // bits : 7_0
			unsigned isd_p3_h_coef9         : 8;      // bits : 15_8
			unsigned isd_p3_h_coef10        : 8;      // bits : 23_16
			unsigned isd_p3_h_coef11        : 8;      // bits : 31_24
		} bit;
		UINT32 word;
	} reg_86; // 0x0158

	union {
		struct {
			unsigned isd_p3_h_coef12        : 8;      // bits : 7_0
			unsigned isd_p3_h_coef13        : 8;      // bits : 15_8
			unsigned isd_p3_h_coef14        : 8;      // bits : 23_16
			unsigned isd_p3_h_coef15        : 8;      // bits : 31_24
		} bit;
		UINT32 word;
	} reg_87; // 0x015c

	union {
		struct {
			unsigned isd_p3_v_coef0        : 8;       // bits : 7_0
			unsigned isd_p3_v_coef1        : 8;       // bits : 15_8
			unsigned isd_p3_v_coef2        : 8;       // bits : 23_16
			unsigned isd_p3_v_coef3        : 8;       // bits : 31_24
		} bit;
		UINT32 word;
	} reg_88; // 0x0160

	union {
		struct {
			unsigned isd_p3_v_coef4        : 8;       // bits : 7_0
			unsigned isd_p3_v_coef5        : 8;       // bits : 15_8
			unsigned isd_p3_v_coef6        : 8;       // bits : 23_16
			unsigned isd_p3_v_coef7        : 8;       // bits : 31_24
		} bit;
		UINT32 word;
	} reg_89; // 0x0164

	union {
		struct {
			unsigned isd_p3_v_coef8         : 8;      // bits : 7_0
			unsigned isd_p3_v_coef9         : 8;      // bits : 15_8
			unsigned isd_p3_v_coef10        : 8;      // bits : 23_16
			unsigned isd_p3_v_coef11        : 8;      // bits : 31_24
		} bit;
		UINT32 word;
	} reg_90; // 0x0168

	union {
		struct {
			unsigned isd_p3_v_coef12        : 8;      // bits : 7_0
			unsigned isd_p3_v_coef13        : 8;      // bits : 15_8
			unsigned isd_p3_v_coef14        : 8;      // bits : 23_16
			unsigned isd_p3_v_coef15        : 8;      // bits : 31_24
		} bit;
		UINT32 word;
	} reg_91; // 0x016c

	union {
		struct {
			unsigned isd_p3_h_coef16        : 12;     // bits : 11_0
			unsigned                        : 4;
			unsigned isd_p3_v_coef16        : 12;     // bits : 27_16
		} bit;
		UINT32 word;
	} reg_92; // 0x0170

	union {
		struct {
			unsigned isd_p3_coef_h_msb0        : 4;       // bits : 3_0
			unsigned isd_p3_coef_h_msb1        : 4;       // bits : 7_4
			unsigned isd_p3_coef_h_msb2        : 4;       // bits : 11_8
			unsigned isd_p3_coef_h_msb3        : 4;       // bits : 15_12
			unsigned isd_p3_coef_h_msb4        : 4;       // bits : 19_16
			unsigned isd_p3_coef_h_msb5        : 4;       // bits : 23_20
			unsigned isd_p3_coef_h_msb6        : 4;       // bits : 27_24
			unsigned isd_p3_coef_h_msb7        : 4;       // bits : 31_28
		} bit;
		UINT32 word;
	} reg_93; // 0x0174

	union {
		struct {
			unsigned isd_p3_coef_h_msb8         : 4;      // bits : 3_0
			unsigned isd_p3_coef_h_msb9         : 4;      // bits : 7_4
			unsigned isd_p3_coef_h_msb10        : 4;      // bits : 11_8
			unsigned isd_p3_coef_h_msb11        : 4;      // bits : 15_12
			unsigned isd_p3_coef_h_msb12        : 4;      // bits : 19_16
			unsigned isd_p3_coef_h_msb13        : 4;      // bits : 23_20
			unsigned isd_p3_coef_h_msb14        : 4;      // bits : 27_24
			unsigned isd_p3_coef_h_msb15        : 4;      // bits : 31_28
		} bit;
		UINT32 word;
	} reg_94; // 0x0178

	union {
		struct {
			unsigned isd_p3_v_coef_msb0        : 4;       // bits : 3_0
			unsigned isd_p3_v_coef_msb1        : 4;       // bits : 7_4
			unsigned isd_p3_v_coef_msb2        : 4;       // bits : 11_8
			unsigned isd_p3_v_coef_msb3        : 4;       // bits : 15_12
			unsigned isd_p3_v_coef_msb4        : 4;       // bits : 19_16
			unsigned isd_p3_v_coef_msb5        : 4;       // bits : 23_20
			unsigned isd_p3_v_coef_msb6        : 4;       // bits : 27_24
			unsigned isd_p3_v_coef_msb7        : 4;       // bits : 31_28
		} bit;
		UINT32 word;
	} reg_95; // 0x017c

	union {
		struct {
			unsigned isd_p3_v_coef_msb8         : 4;      // bits : 3_0
			unsigned isd_p3_v_coef_msb9         : 4;      // bits : 7_4
			unsigned isd_p3_v_coef_msb10        : 4;      // bits : 11_8
			unsigned isd_p3_v_coef_msb11        : 4;      // bits : 15_12
			unsigned isd_p3_v_coef_msb12        : 4;      // bits : 19_16
			unsigned isd_p3_v_coef_msb13        : 4;      // bits : 23_20
			unsigned isd_p3_v_coef_msb14        : 4;      // bits : 27_24
			unsigned isd_p3_v_coef_msb15        : 4;      // bits : 31_28
		} bit;
		UINT32 word;
	} reg_96; // 0x0180

	union {
		struct {
			unsigned h3_scl_size                      : 16;       // bits : 15_0
			unsigned v3_scl_size                      : 16;       // bits : 31_16
		} bit;
		UINT32 word;
	} reg_97; // 0x0184

	union {
		struct {
			unsigned ime_cropout_x_p3                     : 16;       // bits : 15_0
			unsigned ime_cropout_y_p3                     : 16;       // bits : 31_16
		} bit;
		UINT32 word;
	} reg_98; // 0x0188

	union {
		struct {
			unsigned h3_osize                     : 16;       // bits : 15_0
			unsigned v3_osize                     : 16;       // bits : 31_16
		} bit;
		UINT32 word;
	} reg_99; // 0x018c

	union {
		struct {
			unsigned ime_y_clamp_min_p3         : 8;      // bits : 7_0
			unsigned ime_y_clamp_max_p3         : 8;      // bits : 15_8
			unsigned ime_uv_clamp_min_p3        : 8;      // bits : 23_16
			unsigned ime_uv_clamp_max_p3        : 8;      // bits : 31_24
		} bit;
		UINT32 word;
	} reg_100; // 0x0190

	union {
		struct {
			unsigned                           : 2;
			unsigned ime_p3_y_dram_ofso        : 18;        // bits : 19_2
		} bit;
		UINT32 word;
	} reg_101; // 0x0194

	union {
		struct {
			unsigned                            : 2;
			unsigned ime_p3_uv_dram_ofso        : 18;       // bits : 19_2
		} bit;
		UINT32 word;
	} reg_102; // 0x0198

	union {
		struct {
			unsigned ime_p3_y_dram_sao        : 32;     // bits : 31_0
		} bit;
		UINT32 word;
	} reg_103; // 0x019c

	union {
		struct {
			unsigned ime_p3_uv_dram_sao        : 32;        // bits : 31_0
		} bit;
		UINT32 word;
	} reg_104; // 0x01a0

	union {
		struct {
			unsigned reserved                     : 32;       // bits : 31_0
		} bit;
		UINT32 word;
	} reg_105; // 0x01a4

	union {
		struct {
			unsigned reserved                     : 32;       // bits : 31_0
		} bit;
		UINT32 word;
	} reg_106; // 0x01a8

	union {
		struct {
			unsigned reserved                     : 32;       // bits : 31_0
		} bit;
		UINT32 word;
	} reg_107; // 0x01ac

	union {
		struct {
			unsigned                                : 1;
			unsigned ime_out_type_p4                          : 1;        // bits : 1
			unsigned ime_scl_method_p4                          : 2;      // bits : 3_2
			unsigned ime_out_en_p4                              : 1;      // bits : 4
			unsigned                                : 3;
			unsigned ime_omat4                                  : 2;      // bits : 9_8
			unsigned                                : 2;
			unsigned ime_scl_enh_fact_p4                      : 8;        // bits : 19_12
			unsigned ime_scl_enh_bit_p4                        : 5;       // bits : 24_20
		} bit;
		UINT32 word;
	} reg_108; // 0x01b0

	union {
		struct {
			unsigned h4_ud                            : 1;        // bits : 0
			unsigned v4_ud                            : 1;        // bits : 1
			unsigned h4_dnrate                        : 5;        // bits : 6_2
			unsigned v4_dnrate                        : 5;        // bits : 11_7
		} bit;
		UINT32 word;
	} reg_109; // 0x01b4

	union {
		struct {
			unsigned h4_sfact                     : 16;       // bits : 15_0
			unsigned v4_sfact                     : 16;       // bits : 31_16
		} bit;
		UINT32 word;
	} reg_110; // 0x01b8

	union {
		struct {
			unsigned reserved        : 32;        // bits : 31_0
		} bit;
		UINT32 word;
	} reg_111; // 0x01bc

	union {
		struct {
			unsigned h4_scl_size                      : 16;       // bits : 15_0
			unsigned v4_scl_size                      : 16;       // bits : 31_16
		} bit;
		UINT32 word;
	} reg_112; // 0x01c0

	union {
		struct {
			unsigned ime_cropout_x_p4                     : 16;       // bits : 15_0
			unsigned ime_cropout_y_p4                     : 16;       // bits : 31_16
		} bit;
		UINT32 word;
	} reg_113; // 0x01c4

	union {
		struct {
			unsigned h4_osize                     : 16;       // bits : 15_0
			unsigned v4_osize                     : 16;       // bits : 31_16
		} bit;
		UINT32 word;
	} reg_114; // 0x01c8

	union {
		struct {
			unsigned ime_y_clamp_min_p4        : 8;       // bits : 7_0
			unsigned ime_y_clamp_max_p4        : 8;       // bits : 15_8
		} bit;
		UINT32 word;
	} reg_115; // 0x01cc

	union {
		struct {
			unsigned                           : 2;
			unsigned dram_ofso_y_p4                       : 18;       // bits : 19_2
		} bit;
		UINT32 word;
	} reg_116; // 0x01d0

	union {
		struct {
			unsigned reserved                     : 32;       // bits : 31_0
		} bit;
		UINT32 word;
	} reg_117; // 0x01d4

	union {
		struct {
			unsigned dram_sao_p4_y0                       : 32;       // bits : 31_0
		} bit;
		UINT32 word;
	} reg_118; // 0x01d8

	union {
		struct {
			unsigned reserved                     : 32;       // bits : 31_0
		} bit;
		UINT32 word;
	} reg_119; // 0x01dc

	union {
		struct {
			unsigned ime_p2_isd_h_coef_all        : 17;       // bits : 16_0
		} bit;
		UINT32 word;
	} reg_120; // 0x01e0

	union {
		struct {
			unsigned ime_p2_isd_h_coef_half        : 17;      // bits : 16_0
		} bit;
		UINT32 word;
	} reg_121; // 0x01e4

	union {
		struct {
			unsigned ime_p2_isd_v_coef_all        : 17;       // bits : 16_0
		} bit;
		UINT32 word;
	} reg_122; // 0x01e8

	union {
		struct {
			unsigned ime_p2_isd_v_coef_half        : 17;      // bits : 16_0
		} bit;
		UINT32 word;
	} reg_123; // 0x01ec

	union {
		struct {
			unsigned ime_p3_isd_h_coef_all        : 17;       // bits : 16_0
		} bit;
		UINT32 word;
	} reg_124; // 0x01f0

	union {
		struct {
			unsigned ime_p3_isd_h_coef_half        : 17;      // bits : 16_0
		} bit;
		UINT32 word;
	} reg_125; // 0x01f4

	union {
		struct {
			unsigned ime_p3_isd_v_coef_all        : 17;       // bits : 16_0
		} bit;
		UINT32 word;
	} reg_126; // 0x01f8

	union {
		struct {
			unsigned ime_p3_isd_v_coef_half        : 17;      // bits : 16_0
		} bit;
		UINT32 word;
	} reg_127; // 0x01fc

	union {
		struct {
			unsigned chra_isd_h_coef_all        : 17;     // bits : 16_0
		} bit;
		UINT32 word;
	} reg_128; // 0x0200

	union {
		struct {
			unsigned chra_isd_h_coef_half        : 17;        // bits : 16_0
		} bit;
		UINT32 word;
	} reg_129; // 0x0204

	union {
		struct {
			unsigned chra_isd_v_coef_all        : 17;     // bits : 16_0
		} bit;
		UINT32 word;
	} reg_130; // 0x0208

	union {
		struct {
			unsigned chra_isd_v_coef_half        : 17;        // bits : 16_0
		} bit;
		UINT32 word;
	} reg_131; // 0x020c

	union {
		struct {
			unsigned reserved                     : 32;       // bits : 31_0
		} bit;
		UINT32 word;
	} reg_132; // 0x0210

	union {
		struct {
			unsigned reserved                     : 32;       // bits : 31_0
		} bit;
		UINT32 word;
	} reg_133; // 0x0214

	union {
		struct {
			unsigned reserved                     : 32;       // bits : 31_0
		} bit;
		UINT32 word;
	} reg_134; // 0x0218

	union {
		struct {
			unsigned reserved                     : 32;       // bits : 31_0
		} bit;
		UINT32 word;
	} reg_135; // 0x021c

	union {
		struct {
			unsigned chra_h_size                      : 16;       // bits : 15_0
			unsigned chra_v_size                      : 16;       // bits : 31_16
		} bit;
		UINT32 word;
	} reg_136; // 0x0220

	union {
		struct {
			unsigned chra_h_sfact                     : 16;       // bits : 15_0
			unsigned chra_v_sfact                     : 16;       // bits : 31_16
		} bit;
		UINT32 word;
	} reg_137; // 0x0224

	union {
		struct {
			unsigned                             : 2;
			unsigned chra_dram_y_ofsi                     : 18;       // bits : 19_2
		} bit;
		UINT32 word;
	} reg_138; // 0x0228

	union {
		struct {
			unsigned                             : 2;
			unsigned chra_dram_y_sai0                     : 30;       // bits : 31_2
		} bit;
		UINT32 word;
	} reg_139; // 0x022c

	union {
		struct {
			unsigned chra_fmt                        : 1;     // bits : 0
			unsigned chra_in_bypass                      : 1;        // bits : 1
			unsigned chra_src                        : 1;     // bits : 2
			unsigned chra_bypass                        : 1;     // bits : 3
		} bit;
		UINT32 word;
	} reg_140; // 0x0230

	union {
		struct {
			unsigned chra_ca_ctr_u                        : 8;        // bits : 7_0
			unsigned chra_ca_ctr_v                        : 8;        // bits : 15_8
		} bit;
		UINT32 word;
	} reg_141; // 0x0234

	union {
		struct {
			unsigned chra_ca_rng                        : 2;      // bits : 1_0
			unsigned chra_ca_wtprc                        : 2;        // bits : 3_2
			unsigned chra_ca_th                          : 4;     // bits : 7_4
			unsigned                          : 4;
			unsigned chra_ca_wts                        : 6;      // bits : 17_12
			unsigned                          : 2;
			unsigned chra_ca_wte                        : 6;      // bits : 25_20
		} bit;
		UINT32 word;
	} reg_142; // 0x0238

	union {
		struct {
			unsigned chra_refy_wt                     : 5;        // bits : 4_0
			unsigned chra_refc_wt                     : 5;        // bits : 9_5
			unsigned chra_out_wt                       : 5;       // bits : 14_10
			unsigned luma_refy_wt                     : 5;        // bits : 19_15
			unsigned luma_out_wt                       : 5;       // bits : 24_20
		} bit;
		UINT32 word;
	} reg_143; // 0x023c

	union {
		struct {
			unsigned chra_y_rng                         : 2;      // bits : 1_0
			unsigned chra_y_wtprc                     : 2;        // bits : 3_2
			unsigned chra_y_th                           : 5;     // bits : 8_4
			unsigned                         : 3;
			unsigned chra_y_wts                         : 6;      // bits : 17_12
			unsigned                         : 2;
			unsigned chra_y_wte                         : 6;      // bits : 25_20
		} bit;
		UINT32 word;
	} reg_144; // 0x0240

	union {
		struct {
			unsigned chra_uv_rng                        : 3;      // bits : 2_0
			unsigned                          : 1;
			unsigned chra_uv_wtprc                        : 2;        // bits : 5_4
			unsigned                          : 2;
			unsigned chra_uv_th                          : 6;     // bits : 13_8
			unsigned                          : 2;
			unsigned chra_uv_wts                        : 7;      // bits : 22_16
			unsigned                          : 1;
			unsigned chra_uv_wte                        : 7;      // bits : 30_24
		} bit;
		UINT32 word;
	} reg_145; // 0x0244

	union {
		struct {
			unsigned luma_rng                       : 2;      // bits : 1_0
			unsigned luma_wtprc                       : 2;        // bits : 3_2
			unsigned luma_th                         : 5;     // bits : 8_4
			unsigned                       : 3;
			unsigned luma_wts                       : 6;      // bits : 17_12
			unsigned                       : 2;
			unsigned luma_wte                       : 6;      // bits : 25_20
		} bit;
		UINT32 word;
	} reg_146; // 0x0248

	union {
		struct {
			unsigned reserved                     : 32;       // bits : 31_0
		} bit;
		UINT32 word;
	} reg_147; // 0x024c

	union {
		struct {
			unsigned                          : 2;
			unsigned chra_h_dnrate                        : 5;        // bits : 6_2
			unsigned chra_v_dnrate                        : 5;        // bits : 11_7
			unsigned                          : 19;
			unsigned chra_subout_src          : 1;        // bits : 31
		} bit;
		UINT32 word;
	} reg_148; // 0x0250

	union {
		struct {
			unsigned chra_h_sfact                     : 16;       // bits : 15_0
			unsigned chra_v_sfact                     : 16;       // bits : 31_16
		} bit;
		UINT32 word;
	} reg_149; // 0x0254

	union {
		struct {
			unsigned chra_isd_h_base                      : 13;       // bits : 12_0
			unsigned                            : 3;
			unsigned chra_isd_v_base                      : 13;       // bits : 28_16
		} bit;
		UINT32 word;
	} reg_150; // 0x0258

	union {
		struct {
			unsigned chra_isd_h_sfact0                        : 13;       // bits : 12_0
			unsigned                              : 3;
			unsigned chra_isd_v_sfact0                        : 13;       // bits : 28_16
		} bit;
		UINT32 word;
	} reg_151; // 0x025c

	union {
		struct {
			unsigned chra_isd_h_sfact1                        : 13;       // bits : 12_0
			unsigned                              : 3;
			unsigned chra_isd_v_sfact1                        : 13;       // bits : 28_16
		} bit;
		UINT32 word;
	} reg_152; // 0x0260

	union {
		struct {
			unsigned chra_isd_h_sfact2                        : 13;       // bits : 12_0
			unsigned                              : 3;
			unsigned chra_isd_v_sfact2                        : 13;       // bits : 28_16
		} bit;
		UINT32 word;
	} reg_153; // 0x0264

	union {
		struct {
			unsigned                           : 2;
			unsigned chra_dram_ofso                       : 18;       // bits : 19_2
		} bit;
		UINT32 word;
	} reg_154; // 0x0268

	union {
		struct {
			unsigned                          : 2;
			unsigned chra_dram_sao                        : 30;       // bits : 31_2
		} bit;
		UINT32 word;
	} reg_155; // 0x026c

	union {
		struct {
			unsigned chra_isd_h_coef_num        : 5;      // bits : 4_0
			unsigned                            : 3;
			unsigned chra_isd_v_coef_num        : 5;      // bits : 12_8
		} bit;
		UINT32 word;
	} reg_156; // 0x0270

	union {
		struct {
			unsigned reserved                     : 32;       // bits : 31_0
		} bit;
		UINT32 word;
	} reg_157; // 0x0274

	union {
		struct {
			unsigned reserved                     : 32;       // bits : 31_0
		} bit;
		UINT32 word;
	} reg_158; // 0x0278

	union {
		struct {
			unsigned reserved                     : 32;       // bits : 31_0
		} bit;
		UINT32 word;
	} reg_159; // 0x027c

	union {
		struct {
			unsigned dbcs_ctr_u              : 8;     // bits : 7_0
			unsigned dbcs_ctr_v                         : 8;      // bits : 15_8
			unsigned dbcs_mode                           : 2;     // bits : 17_16
			unsigned                         : 2;
			unsigned dbcs_step_y                       : 2;       // bits : 21_20
			unsigned dbcs_step_uv                     : 2;        // bits : 23_22
		} bit;
		UINT32 word;
	} reg_160; // 0x0280

	union {
		struct {
			unsigned dbcs_y_wt0                       : 5;        // bits : 4_0
			unsigned dbcs_y_wt1                       : 5;        // bits : 9_5
			unsigned dbcs_y_wt2                       : 5;        // bits : 14_10
			unsigned dbcs_y_wt3                       : 5;        // bits : 19_15
			unsigned dbcs_y_wt4                       : 5;        // bits : 24_20
			unsigned dbcs_y_wt5                       : 5;        // bits : 29_25
		} bit;
		UINT32 word;
	} reg_161; // 0x0284

	union {
		struct {
			unsigned dbcs_y_wt6                        : 5;       // bits : 4_0
			unsigned dbcs_y_wt7                        : 5;       // bits : 9_5
			unsigned dbcs_y_wt8                        : 5;       // bits : 14_10
			unsigned dbcs_y_wt9                        : 5;       // bits : 19_15
			unsigned dbcs_y_wt10                      : 5;        // bits : 24_20
			unsigned dbcs_y_wt11                      : 5;        // bits : 29_25
		} bit;
		UINT32 word;
	} reg_162; // 0x0288

	union {
		struct {
			unsigned dbcs_y_wt12                      : 5;        // bits : 4_0
			unsigned dbcs_y_wt13                      : 5;        // bits : 9_5
			unsigned dbcs_y_wt14                      : 5;        // bits : 14_10
			unsigned dbcs_y_wt15                      : 5;        // bits : 19_15
		} bit;
		UINT32 word;
	} reg_163; // 0x028c

	union {
		struct {
			unsigned dbcs_c_wt0                       : 5;        // bits : 4_0
			unsigned dbcs_c_wt1                       : 5;        // bits : 9_5
			unsigned dbcs_c_wt2                       : 5;        // bits : 14_10
			unsigned dbcs_c_wt3                       : 5;        // bits : 19_15
			unsigned dbcs_c_wt4                       : 5;        // bits : 24_20
			unsigned dbcs_c_wt5                       : 5;        // bits : 29_25
		} bit;
		UINT32 word;
	} reg_164; // 0x0290

	union {
		struct {
			unsigned dbcs_c_wt6                        : 5;       // bits : 4_0
			unsigned dbcs_c_wt7                        : 5;       // bits : 9_5
			unsigned dbcs_c_wt8                        : 5;       // bits : 14_10
			unsigned dbcs_c_wt9                        : 5;       // bits : 19_15
			unsigned dbcs_c_wt10                      : 5;        // bits : 24_20
			unsigned dbcs_c_wt11                      : 5;        // bits : 29_25
		} bit;
		UINT32 word;
	} reg_165; // 0x0294

	union {
		struct {
			unsigned dbcs_c_wt12                      : 5;        // bits : 4_0
			unsigned dbcs_c_wt13                      : 5;        // bits : 9_5
			unsigned dbcs_c_wt14                      : 5;        // bits : 14_10
			unsigned dbcs_c_wt15                      : 5;        // bits : 19_15
		} bit;
		UINT32 word;
	} reg_166; // 0x0298

	union {
		struct {
			unsigned reserved                     : 32;       // bits : 31_0
		} bit;
		UINT32 word;
	} reg_167; // 0x029c

	union {
		struct {
			unsigned reserved                     : 32;       // bits : 31_0
		} bit;
		UINT32 word;
	} reg_168; // 0x02a0

	union {
		struct {
			unsigned reserved                     : 32;       // bits : 31_0
		} bit;
		UINT32 word;
	} reg_169; // 0x02a4

	union {
		struct {
			unsigned reserved                     : 32;       // bits : 31_0
		} bit;
		UINT32 word;
	} reg_170; // 0x02a8

	union {
		struct {
			unsigned reserved                     : 32;       // bits : 31_0
		} bit;
		UINT32 word;
	} reg_171; // 0x02ac

	union {
		struct {
			unsigned reserved                     : 32;       // bits : 31_0
		} bit;
		UINT32 word;
	} reg_172; // 0x02b0

	union {
		struct {
			unsigned reserved                     : 32;       // bits : 31_0
		} bit;
		UINT32 word;
	} reg_173; // 0x02b4

	union {
		struct {
			unsigned reserved                     : 32;       // bits : 31_0
		} bit;
		UINT32 word;
	} reg_174; // 0x02b8

	union {
		struct {
			unsigned reserved                     : 32;       // bits : 31_0
		} bit;
		UINT32 word;
	} reg_175; // 0x02bc

	union {
		struct {
			unsigned ime_ds0_hsize                        : 13;       // bits : 12_0
			unsigned                          : 3;
			unsigned ime_ds0_vsize                        : 13;       // bits : 28_16
		} bit;
		UINT32 word;
	} reg_176; // 0x02c0

	union {
		struct {
			unsigned ime_ds0_posx                      : 13;      // bits : 12_0
			unsigned                          : 3;
			unsigned ime_ds0_posy                      : 13;      // bits : 28_16
			unsigned ime_ds0_ck_en                        : 1;        // bits : 29
			unsigned ime_ds0_fmt                        : 2;      // bits : 31_30
		} bit;
		UINT32 word;
	} reg_177; // 0x02c4

	union {
		struct {
			unsigned ime_ds0_ck_a        : 8;     // bits : 7_0
			unsigned ime_ds0_ck_r        : 8;     // bits : 15_8
			unsigned ime_ds0_ck_g        : 8;     // bits : 23_16
			unsigned ime_ds0_ck_b        : 8;     // bits : 31_24
		} bit;
		UINT32 word;
	} reg_178; // 0x02c8

	union {
		struct {
			unsigned ime_ds0_gawet0                       : 4;        // bits : 3_0
			unsigned ime_ds0_gawet1                       : 4;        // bits : 7_4
			unsigned ime_ds0_plt_en            : 1;       // bits : 8
			unsigned ime_ds0_ck_mode           : 1;       // bits : 9
		} bit;
		UINT32 word;
	} reg_179; // 0x02cc

	union {
		struct {
			unsigned                         : 2;
			unsigned ime_ds0_ofsi                     : 18;       // bits : 19_2
		} bit;
		UINT32 word;
	} reg_180; // 0x02d0

	union {
		struct {
			unsigned                    : 2;
			unsigned ime_ds0_sai        : 30;     // bits : 31_2
		} bit;
		UINT32 word;
	} reg_181; // 0x02d4

	union {
		struct {
			unsigned ime_ds1_hsize                        : 13;       // bits : 12_0
			unsigned                          : 3;
			unsigned ime_ds1_vsize                        : 13;       // bits : 28_16
		} bit;
		UINT32 word;
	} reg_182; // 0x02d8

	union {
		struct {
			unsigned ime_ds1_posx                      : 13;      // bits : 12_0
			unsigned                          : 3;
			unsigned ime_ds1_posy                      : 13;      // bits : 28_16
			unsigned ime_ds1_ck_en                        : 1;        // bits : 29
			unsigned ime_ds1_fmt                        : 2;      // bits : 31_30
		} bit;
		UINT32 word;
	} reg_183; // 0x02dc

	union {
		struct {
			unsigned ime_ds1_ck_a        : 8;     // bits : 7_0
			unsigned ime_ds1_ck_r        : 8;     // bits : 15_8
			unsigned ime_ds1_ck_g        : 8;     // bits : 23_16
			unsigned ime_ds1_ck_b        : 8;     // bits : 31_24
		} bit;
		UINT32 word;
	} reg_184; // 0x02e0

	union {
		struct {
			unsigned ime_ds1_gawet0                       : 4;        // bits : 3_0
			unsigned ime_ds1_gawet1                       : 4;        // bits : 7_4
			unsigned ime_ds1_plt_en            : 1;       // bits : 8
			unsigned ime_ds1_ck_mode           : 1;       // bits : 9
		} bit;
		UINT32 word;
	} reg_185; // 0x02e4

	union {
		struct {
			unsigned                         : 2;
			unsigned ime_ds1_ofsi                     : 18;       // bits : 19_2
		} bit;
		UINT32 word;
	} reg_186; // 0x02e8

	union {
		struct {
			unsigned                        : 2;
			unsigned ime_ds1_sai                      : 30;       // bits : 31_2
		} bit;
		UINT32 word;
	} reg_187; // 0x02ec

	union {
		struct {
			unsigned ime_ds2_hsize                        : 13;       // bits : 12_0
			unsigned                          : 3;
			unsigned ime_ds2_vsize                        : 13;       // bits : 28_16
		} bit;
		UINT32 word;
	} reg_188; // 0x02f0

	union {
		struct {
			unsigned ime_ds2_posx                      : 13;      // bits : 12_0
			unsigned                          : 3;
			unsigned ime_ds2_posy                      : 13;      // bits : 28_16
			unsigned ime_ds2_ck_en                        : 1;        // bits : 29
			unsigned ime_ds2_fmt                        : 2;      // bits : 31_30
		} bit;
		UINT32 word;
	} reg_189; // 0x02f4

	union {
		struct {
			unsigned ime_ds2_ck_a        : 8;     // bits : 7_0
			unsigned ime_ds2_ck_r        : 8;     // bits : 15_8
			unsigned ime_ds2_ck_g        : 8;     // bits : 23_16
			unsigned ime_ds2_ck_b        : 8;     // bits : 31_24
		} bit;
		UINT32 word;
	} reg_190; // 0x02f8

	union {
		struct {
			unsigned ime_ds2_gawet0                       : 4;        // bits : 3_0
			unsigned ime_ds2_gawet1                       : 4;        // bits : 7_4
			unsigned ime_ds2_plt_en            : 1;       // bits : 8
			unsigned ime_ds2_ck_mode           : 1;       // bits : 9
		} bit;
		UINT32 word;
	} reg_191; // 0x02fc

	union {
		struct {
			unsigned                         : 2;
			unsigned ime_ds2_ofsi                     : 18;       // bits : 19_2
		} bit;
		UINT32 word;
	} reg_192; // 0x0300

	union {
		struct {
			unsigned                        : 2;
			unsigned ime_ds2_sai                      : 30;       // bits : 31_2
		} bit;
		UINT32 word;
	} reg_193; // 0x0304

	union {
		struct {
			unsigned ime_ds3_hsize                        : 13;       // bits : 12_0
			unsigned                          : 3;
			unsigned ime_ds3_vsize                        : 13;       // bits : 28_16
		} bit;
		UINT32 word;
	} reg_194; // 0x0308

	union {
		struct {
			unsigned ime_ds3_posx                      : 13;      // bits : 12_0
			unsigned                          : 3;
			unsigned ime_ds3_posy                      : 13;      // bits : 28_16
			unsigned ime_ds3_ck_en                        : 1;        // bits : 29
			unsigned ime_ds3_fmt                        : 2;      // bits : 31_30
		} bit;
		UINT32 word;
	} reg_195; // 0x030c

	union {
		struct {
			unsigned ime_ds3_ck_a        : 8;     // bits : 7_0
			unsigned ime_ds3_ck_r        : 8;     // bits : 15_8
			unsigned ime_ds3_ck_g        : 8;     // bits : 23_16
			unsigned ime_ds3_ck_b        : 8;     // bits : 31_24
		} bit;
		UINT32 word;
	} reg_196; // 0x0310

	union {
		struct {
			unsigned ime_ds3_gawet0                       : 4;        // bits : 3_0
			unsigned ime_ds3_gawet1                       : 4;        // bits : 7_4
			unsigned ime_ds3_plt_en            : 1;       // bits : 8
			unsigned ime_ds3_ck_mode           : 1;       // bits : 9
		} bit;
		UINT32 word;
	} reg_197; // 0x0314

	union {
		struct {
			unsigned                         : 2;
			unsigned ime_ds3_ofsi                     : 18;       // bits : 19_2
		} bit;
		UINT32 word;
	} reg_198; // 0x0318

	union {
		struct {
			unsigned                        : 2;
			unsigned ime_ds3_sai                      : 30;       // bits : 31_2
		} bit;
		UINT32 word;
	} reg_199; // 0x031c

	union {
		struct {
			unsigned ime_ds_cst_coef0        : 8;     // bits : 7_0
			unsigned ime_ds_cst_coef1        : 8;     // bits : 15_8
			unsigned ime_ds_cst_coef2        : 8;     // bits : 23_16
			unsigned ime_ds_cst_coef3        : 8;     // bits : 31_24
		} bit;
		UINT32 word;
	} reg_200; // 0x0320

	union {
		struct {
			unsigned ime_ds_pltc_a0        : 8;       // bits : 7_0
			unsigned ime_ds_pltc_r0        : 8;       // bits : 15_8
			unsigned ime_ds_pltc_g0        : 8;       // bits : 23_16
			unsigned ime_ds_pltc_b0        : 8;       // bits : 31_24
		} bit;
		UINT32 word;
	} reg_201; // 0x0324

	union {
		struct {
			unsigned ime_ds_pltc_a1        : 8;       // bits : 7_0
			unsigned ime_ds_pltc_r1        : 8;       // bits : 15_8
			unsigned ime_ds_pltc_g1        : 8;       // bits : 23_16
			unsigned ime_ds_pltc_b1        : 8;       // bits : 31_24
		} bit;
		UINT32 word;
	} reg_202; // 0x0328

	union {
		struct {
			unsigned ime_ds_pltc_a2        : 8;       // bits : 7_0
			unsigned ime_ds_pltc_r2        : 8;       // bits : 15_8
			unsigned ime_ds_pltc_g2        : 8;       // bits : 23_16
			unsigned ime_ds_pltc_b2        : 8;       // bits : 31_24
		} bit;
		UINT32 word;
	} reg_203; // 0x032c

	union {
		struct {
			unsigned ime_ds_pltc_a3        : 8;       // bits : 7_0
			unsigned ime_ds_pltc_r3        : 8;       // bits : 15_8
			unsigned ime_ds_pltc_g3        : 8;       // bits : 23_16
			unsigned ime_ds_pltc_b3        : 8;       // bits : 31_24
		} bit;
		UINT32 word;
	} reg_204; // 0x0330

	union {
		struct {
			unsigned ime_ds_pltc_a4        : 8;       // bits : 7_0
			unsigned ime_ds_pltc_r4        : 8;       // bits : 15_8
			unsigned ime_ds_pltc_g4        : 8;       // bits : 23_16
			unsigned ime_ds_pltc_b4        : 8;       // bits : 31_24
		} bit;
		UINT32 word;
	} reg_205; // 0x0334

	union {
		struct {
			unsigned ime_ds_pltc_a5        : 8;       // bits : 7_0
			unsigned ime_ds_pltc_r5        : 8;       // bits : 15_8
			unsigned ime_ds_pltc_g5        : 8;       // bits : 23_16
			unsigned ime_ds_pltc_b5        : 8;       // bits : 31_24
		} bit;
		UINT32 word;
	} reg_206; // 0x0338

	union {
		struct {
			unsigned ime_ds_pltc_a6        : 8;       // bits : 7_0
			unsigned ime_ds_pltc_r6        : 8;       // bits : 15_8
			unsigned ime_ds_pltc_g6        : 8;       // bits : 23_16
			unsigned ime_ds_pltc_b6        : 8;       // bits : 31_24
		} bit;
		UINT32 word;
	} reg_207; // 0x033c

	union {
		struct {
			unsigned ime_ds_pltc_a7        : 8;       // bits : 7_0
			unsigned ime_ds_pltc_r7        : 8;       // bits : 15_8
			unsigned ime_ds_pltc_g7        : 8;       // bits : 23_16
			unsigned ime_ds_pltc_b7        : 8;       // bits : 31_24
		} bit;
		UINT32 word;
	} reg_208; // 0x0340

	union {
		struct {
			unsigned ime_ds_pltc_a8        : 8;       // bits : 7_0
			unsigned ime_ds_pltc_r8        : 8;       // bits : 15_8
			unsigned ime_ds_pltc_g8        : 8;       // bits : 23_16
			unsigned ime_ds_pltc_b8        : 8;       // bits : 31_24
		} bit;
		UINT32 word;
	} reg_209; // 0x0344

	union {
		struct {
			unsigned ime_ds_pltc_a9        : 8;       // bits : 7_0
			unsigned ime_ds_pltc_r9        : 8;       // bits : 15_8
			unsigned ime_ds_pltc_g9        : 8;       // bits : 23_16
			unsigned ime_ds_pltc_b9        : 8;       // bits : 31_24
		} bit;
		UINT32 word;
	} reg_210; // 0x0348

	union {
		struct {
			unsigned ime_ds_pltc_a10        : 8;      // bits : 7_0
			unsigned ime_ds_pltc_r10        : 8;      // bits : 15_8
			unsigned ime_ds_pltc_g10        : 8;      // bits : 23_16
			unsigned ime_ds_pltc_b10        : 8;      // bits : 31_24
		} bit;
		UINT32 word;
	} reg_211; // 0x034c

	union {
		struct {
			unsigned ime_ds_pltc_a11        : 8;      // bits : 7_0
			unsigned ime_ds_pltc_r11        : 8;      // bits : 15_8
			unsigned ime_ds_pltc_g11        : 8;      // bits : 23_16
			unsigned ime_ds_pltc_b11        : 8;      // bits : 31_24
		} bit;
		UINT32 word;
	} reg_212; // 0x0350

	union {
		struct {
			unsigned ime_ds_pltc_a12        : 8;      // bits : 7_0
			unsigned ime_ds_pltc_r12        : 8;      // bits : 15_8
			unsigned ime_ds_pltc_g12        : 8;      // bits : 23_16
			unsigned ime_ds_pltc_b12        : 8;      // bits : 31_24
		} bit;
		UINT32 word;
	} reg_213; // 0x0354

	union {
		struct {
			unsigned ime_ds_pltc_a13        : 8;      // bits : 7_0
			unsigned ime_ds_pltc_r13        : 8;      // bits : 15_8
			unsigned ime_ds_pltc_g13        : 8;      // bits : 23_16
			unsigned ime_ds_pltc_b13        : 8;      // bits : 31_24
		} bit;
		UINT32 word;
	} reg_214; // 0x0358

	union {
		struct {
			unsigned ime_ds_pltc_a14        : 8;      // bits : 7_0
			unsigned ime_ds_pltc_r14        : 8;      // bits : 15_8
			unsigned ime_ds_pltc_g14        : 8;      // bits : 23_16
			unsigned ime_ds_pltc_b14        : 8;      // bits : 31_24
		} bit;
		UINT32 word;
	} reg_215; // 0x035c

	union {
		struct {
			unsigned ime_ds_pltc_a15        : 8;      // bits : 7_0
			unsigned ime_ds_pltc_r15        : 8;      // bits : 15_8
			unsigned ime_ds_pltc_g15        : 8;      // bits : 23_16
			unsigned ime_ds_pltc_b15        : 8;      // bits : 31_24
		} bit;
		UINT32 word;
	} reg_216; // 0x0360

	union {
		struct {
			unsigned reserved                     : 32;       // bits : 31_0
		} bit;
		UINT32 word;
	} reg_217; // 0x0364

	union {
		struct {
			unsigned reserved                     : 32;       // bits : 31_0
		} bit;
		UINT32 word;
	} reg_218; // 0x0368

	union {
		struct {
			unsigned reserved                     : 32;       // bits : 31_0
		} bit;
		UINT32 word;
	} reg_219; // 0x036c

	union {
		struct {
			unsigned ime_stl_row0                        : 10;        // bits : 9_0
			unsigned ime_stl_row1                        : 10;        // bits : 19_10
			unsigned ime_stl_emk0                        : 2;     // bits : 21_20
			unsigned ime_stl_emk1                        : 2;     // bits : 23_22
			unsigned ime_stl_emken0                        : 1;       // bits : 24
			unsigned ime_stl_emken1                        : 1;       // bits : 25
			unsigned ime_stl_out_sel                      : 1;        // bits : 26
		} bit;
		UINT32 word;
	} reg_220; // 0x0370

	union {
		struct {
			unsigned ime_stl_posh0                        : 16;       // bits : 15_0
			unsigned ime_stl_posv0                        : 16;       // bits : 31_16
		} bit;
		UINT32 word;
	} reg_221; // 0x0374

	union {
		struct {
			unsigned ime_stl_sizeh0                       : 10;       // bits : 9_0
			unsigned ime_stl_sizev0                       : 10;       // bits : 19_10
			unsigned ime_stl_msft0                         : 4;       // bits : 23_20
			unsigned ime_stl_msft1                         : 4;       // bits : 27_24
			unsigned ime_stl_sizeh0_msb        : 1;       // bits : 28
			unsigned                           : 1;
			unsigned ime_stl_sizev0_msb        : 1;       // bits : 30
		} bit;
		UINT32 word;
	} reg_222; // 0x0378

	union {
		struct {
			unsigned ime_roi0_th0                     : 10;       // bits : 9_0
			unsigned ime_roi0_th1                     : 10;       // bits : 19_10
			unsigned ime_roi0_th2                     : 10;       // bits : 29_20
		} bit;
		UINT32 word;
	} reg_223; // 0x037c

	union {
		struct {
			unsigned ime_roi1_th0                     : 10;       // bits : 9_0
			unsigned ime_roi1_th1                     : 10;       // bits : 19_10
			unsigned ime_roi1_th2                     : 10;       // bits : 29_20
		} bit;
		UINT32 word;
	} reg_224; // 0x0380

	union {
		struct {
			unsigned ime_roi2_th0                     : 10;       // bits : 9_0
			unsigned ime_roi2_th1                     : 10;       // bits : 19_10
			unsigned ime_roi2_th2                     : 10;       // bits : 29_20
		} bit;
		UINT32 word;
	} reg_225; // 0x0384

	union {
		struct {
			unsigned ime_roi3_th0                     : 10;       // bits : 9_0
			unsigned ime_roi3_th1                     : 10;       // bits : 19_10
			unsigned ime_roi3_th2                     : 10;       // bits : 29_20
		} bit;
		UINT32 word;
	} reg_226; // 0x0388

	union {
		struct {
			unsigned ime_roi4_th0                     : 10;       // bits : 9_0
			unsigned ime_roi4_th1                     : 10;       // bits : 19_10
			unsigned ime_roi4_th2                     : 10;       // bits : 29_20
		} bit;
		UINT32 word;
	} reg_227; // 0x038c

	union {
		struct {
			unsigned ime_stl_posh1                        : 16;       // bits : 15_0
			unsigned ime_stl_posv1                        : 16;       // bits : 31_16
		} bit;
		UINT32 word;
	} reg_228; // 0x0390

	union {
		struct {
			unsigned ime_stl_sizeh1                       : 10;       // bits : 9_0
			unsigned ime_stl_sizev1                       : 10;       // bits : 19_10
			unsigned                           : 8;
			unsigned ime_stl_sizeh1_msb        : 1;       // bits : 28
			unsigned                           : 1;
			unsigned ime_stl_sizev1_msb        : 1;       // bits : 30
		} bit;
		UINT32 word;
	} reg_229; // 0x0394

	union {
		struct {
			unsigned ime_roi5_th0                     : 10;       // bits : 9_0
			unsigned ime_roi5_th1                     : 10;       // bits : 19_10
			unsigned ime_roi5_th2                     : 10;       // bits : 29_20
		} bit;
		UINT32 word;
	} reg_230; // 0x0398

	union {
		struct {
			unsigned ime_roi6_th0                     : 10;       // bits : 9_0
			unsigned ime_roi6_th1                     : 10;       // bits : 19_10
			unsigned ime_roi6_th2                     : 10;       // bits : 29_20
		} bit;
		UINT32 word;
	} reg_231; // 0x039c

	union {
		struct {
			unsigned ime_roi7_th0                     : 10;       // bits : 9_0
			unsigned ime_roi7_th1                     : 10;       // bits : 19_10
			unsigned ime_roi7_th2                     : 10;       // bits : 29_20
		} bit;
		UINT32 word;
	} reg_232; // 0x03a0

	union {
		struct {
			unsigned ime_roi0_msb_th0                     : 1;        // bits : 0
			unsigned ime_roi0_msb_th1                     : 1;        // bits : 1
			unsigned ime_roi0_msb_th2                     : 1;        // bits : 2
			unsigned ime_roi1_msb_th0                     : 1;        // bits : 3
			unsigned ime_roi1_msb_th1                     : 1;        // bits : 4
			unsigned ime_roi1_msb_th2                     : 1;        // bits : 5
			unsigned ime_roi2_msb_th0                     : 1;        // bits : 6
			unsigned ime_roi2_msb_th1                     : 1;        // bits : 7
			unsigned ime_roi2_msb_th2                     : 1;        // bits : 8
			unsigned ime_roi3_msb_th0                     : 1;        // bits : 9
			unsigned ime_roi3_msb_th1                     : 1;        // bits : 10
			unsigned ime_roi3_msb_th2                     : 1;        // bits : 11
			unsigned ime_roi4_msb_th0                     : 1;        // bits : 12
			unsigned ime_roi4_msb_th1                     : 1;        // bits : 13
			unsigned ime_roi4_msb_th2                     : 1;        // bits : 14
			unsigned ime_roi5_msb_th0                     : 1;        // bits : 15
			unsigned ime_roi5_msb_th1                     : 1;        // bits : 16
			unsigned ime_roi5_msb_th2                     : 1;        // bits : 17
			unsigned ime_roi6_msb_th0                     : 1;        // bits : 18
			unsigned ime_roi6_msb_th1                     : 1;        // bits : 19
			unsigned ime_roi6_msb_th2                     : 1;        // bits : 20
			unsigned ime_roi7_msb_th0                     : 1;        // bits : 21
			unsigned ime_roi7_msb_th1                     : 1;        // bits : 22
			unsigned ime_roi7_msb_th2                     : 1;        // bits : 23
			unsigned ime_roi0_src                         : 1;        // bits : 24
			unsigned ime_roi1_src                         : 1;        // bits : 25
			unsigned ime_roi2_src                         : 1;        // bits : 26
			unsigned ime_roi3_src                         : 1;        // bits : 27
			unsigned ime_roi4_src                         : 1;        // bits : 28
			unsigned ime_roi5_src                         : 1;        // bits : 29
			unsigned ime_roi6_src                         : 1;        // bits : 30
			unsigned ime_roi7_src                         : 1;        // bits : 31
		} bit;
		UINT32 word;
	} reg_233; // 0x03a4

	union {
		struct {
			unsigned ime_emax0                        : 11;       // bits : 10_0
			unsigned                      : 1;
			unsigned ime_emax1                        : 11;       // bits : 22_12
		} bit;
		UINT32 word;
	} reg_234; // 0x03a8

	union {
		struct {
			unsigned ime_stl_acct0                          : 20;     // bits : 19_0
			unsigned                            : 4;
			unsigned ime_stl_hitbin0                      : 7;        // bits : 30_24
		} bit;
		UINT32 word;
	} reg_235; // 0x03ac

	union {
		struct {
			unsigned ime_stl_acct1                          : 20;     // bits : 19_0
			unsigned                            : 4;
			unsigned ime_stl_hitbin1                      : 7;        // bits : 30_24
		} bit;
		UINT32 word;
	} reg_236; // 0x03b0

	union {
		struct {
			unsigned                               : 2;
			unsigned ime_em_dram_sao_p4                       : 30;       // bits : 31_2
		} bit;
		UINT32 word;
	} reg_237; // 0x03b4

	union {
		struct {
			unsigned                              : 2;
			unsigned ime_hist_dram_sao                      : 30;       // bits : 31_2
		} bit;
		UINT32 word;
	} reg_238; // 0x03b8

	union {
		struct {
			unsigned                           : 2;
			unsigned ime_em_ofso_p4                       : 18;       // bits : 19_2
		} bit;
		UINT32 word;
	} reg_239; // 0x03bc

	union {
		struct {
			unsigned ime_sprt_hbl_p1                      : 16;       // bits : 15_0
		} bit;
		UINT32 word;
	} reg_240; // 0x03c0

	union {
		struct {
			unsigned                            : 2;
			unsigned ime_p1_y1_dram_ofso        : 18;       // bits : 19_2
		} bit;
		UINT32 word;
	} reg_241; // 0x03c4

	union {
		struct {
			unsigned                             : 2;
			unsigned ime_p1_uv1_dram_ofso        : 18;      // bits : 19_2
		} bit;
		UINT32 word;
	} reg_242; // 0x03c8

	union {
		struct {
			unsigned ime_p1_y1_dram_sao        : 32;        // bits : 31_0
		} bit;
		UINT32 word;
	} reg_243; // 0x03cc

	union {
		struct {
			unsigned ime_p1_uv1_dram_sao        : 32;       // bits : 31_0
		} bit;
		UINT32 word;
	} reg_244; // 0x03d0

	union {
		struct {
			unsigned ime_sprt_hbl_p2                      : 16;       // bits : 15_0
		} bit;
		UINT32 word;
	} reg_245; // 0x03d4

	union {
		struct {
			unsigned                            : 2;
			unsigned ime_p2_y1_dram_ofso        : 18;       // bits : 19_2
		} bit;
		UINT32 word;
	} reg_246; // 0x03d8

	union {
		struct {
			unsigned                             : 2;
			unsigned ime_p2_uv1_dram_ofso        : 18;      // bits : 19_2
		} bit;
		UINT32 word;
	} reg_247; // 0x03dc

	union {
		struct {
			unsigned ime_p2_y1_dram_sao        : 32;        // bits : 32_0
		} bit;
		UINT32 word;
	} reg_248; // 0x03e0

	union {
		struct {
			unsigned ime_p2_uv1_dram_sao        : 32;       // bits : 32_0
		} bit;
		UINT32 word;
	} reg_249; // 0x03e4

	union {
		struct {
			unsigned ime_sprt_hbl_p3                      : 16;       // bits : 15_0
		} bit;
		UINT32 word;
	} reg_250; // 0x03e8

	union {
		struct {
			unsigned                            : 2;
			unsigned ime_p3_y1_dram_ofso        : 18;       // bits : 19_2
		} bit;
		UINT32 word;
	} reg_251; // 0x03ec

	union {
		struct {
			unsigned                             : 2;
			unsigned ime_p3_uv1_dram_ofso        : 18;      // bits : 19_2
		} bit;
		UINT32 word;
	} reg_252; // 0x03f0

	union {
		struct {
			unsigned ime_p3_y1_dram_sao        : 32;        // bits : 31_0
		} bit;
		UINT32 word;
	} reg_253; // 0x03f4

	union {
		struct {
			unsigned ime_p3_uv1_dram_sao        : 32;       // bits : 31_0
		} bit;
		UINT32 word;
	} reg_254; // 0x03f8

	union {
		struct {
			unsigned reserved                     : 32;       // bits : 31_0
		} bit;
		UINT32 word;
	} reg_255; // 0x03fc

	union {
		struct {
			unsigned reserved                     : 32;       // bits : 31_0
		} bit;
		UINT32 word;
	} reg_256; // 0x0400

	union {
		struct {
			unsigned reserved                     : 32;       // bits : 31_0
		} bit;
		UINT32 word;
	} reg_257; // 0x0404

	union {
		struct {
			unsigned reserved                     : 32;       // bits : 31_0
		} bit;
		UINT32 word;
	} reg_258; // 0x0408

	union {
		struct {
			unsigned reserved                     : 32;       // bits : 31_0
		} bit;
		UINT32 word;
	} reg_259; // 0x040c

	union {
		struct {
			unsigned reserved                     : 32;       // bits : 31_0
		} bit;
		UINT32 word;
	} reg_260; // 0x0410

	union {
		struct {
			unsigned reserved                     : 32;       // bits : 31_0
		} bit;
		UINT32 word;
	} reg_261; // 0x0414

	union {
		struct {
			unsigned reserved                     : 32;       // bits : 31_0
		} bit;
		UINT32 word;
	} reg_262; // 0x0418

	union {
		struct {
			unsigned reserved                     : 32;       // bits : 31_0
		} bit;
		UINT32 word;
	} reg_263; // 0x041c

	union {
		struct {
			unsigned ime_pm0_type                        : 1;     // bits : 0
			unsigned ime_pm_pxlsize                        : 2;       // bits : 2_1
			unsigned                            : 1;
			unsigned ime_pm0_color_y                      : 8;        // bits : 11_4
			unsigned ime_pm0_color_u                      : 8;        // bits : 19_12
			unsigned ime_pm0_color_v                      : 8;        // bits : 27_20
			unsigned                            : 3;
			unsigned ime_pm0_hlw_en             : 1;      // bits : 31
		} bit;
		UINT32 word;
	} reg_264; // 0x0420

	union {
		struct {
			unsigned ime_pm0_line0_comp                        : 2;       // bits : 1_0
			unsigned                                : 2;
			unsigned ime_pm0_line0_coefa                      : 13;       // bits : 16_4
			unsigned ime_pm0_line0_signa                      : 1;        // bits : 17
			unsigned ime_pm0_line0_coefb                      : 13;       // bits : 30_18
			unsigned ime_pm0_line0_signb                      : 1;        // bits : 31
		} bit;
		UINT32 word;
	} reg_265; // 0x0424

	union {
		struct {
			unsigned ime_pm0_line0_coefc                      : 26;       // bits : 25_0
			unsigned ime_pm0_line0_signc                      : 1;        // bits : 26
		} bit;
		UINT32 word;
	} reg_266; // 0x0428

	union {
		struct {
			unsigned ime_pm0_line1_comp                        : 2;       // bits : 1_0
			unsigned                                : 2;
			unsigned ime_pm0_line1_coefa                      : 13;       // bits : 16_4
			unsigned ime_pm0_line1_signa                      : 1;        // bits : 17
			unsigned ime_pm0_line1_coefb                      : 13;       // bits : 30_18
			unsigned ime_pm0_line1_signb                      : 1;        // bits : 31
		} bit;
		UINT32 word;
	} reg_267; // 0x042c

	union {
		struct {
			unsigned ime_pm0_line1_coefc                      : 26;       // bits : 25_0
			unsigned ime_pm0_line1_signc                      : 1;        // bits : 26
		} bit;
		UINT32 word;
	} reg_268; // 0x0430

	union {
		struct {
			unsigned ime_pm0_line2_comp                        : 2;       // bits : 1_0
			unsigned                                : 2;
			unsigned ime_pm0_line2_coefa                      : 13;       // bits : 16_4
			unsigned ime_pm0_line2_signa                      : 1;        // bits : 17
			unsigned ime_pm0_line2_coefb                      : 13;       // bits : 30_18
			unsigned ime_pm0_line2_signb                      : 1;        // bits : 31
		} bit;
		UINT32 word;
	} reg_269; // 0x0434

	union {
		struct {
			unsigned ime_pm0_line2_coefc                      : 26;       // bits : 25_0
			unsigned ime_pm0_line2_signc                      : 1;        // bits : 26
		} bit;
		UINT32 word;
	} reg_270; // 0x0438

	union {
		struct {
			unsigned ime_pm0_line3_comp                        : 2;       // bits : 1_0
			unsigned                                : 2;
			unsigned ime_pm0_line3_coefa                      : 13;       // bits : 16_4
			unsigned ime_pm0_line3_signa                      : 1;        // bits : 17
			unsigned ime_pm0_line3_coefb                      : 13;       // bits : 30_18
			unsigned ime_pm0_line3_signb                      : 1;        // bits : 31
		} bit;
		UINT32 word;
	} reg_271; // 0x043c

	union {
		struct {
			unsigned ime_pm0_line3_coefc                      : 26;       // bits : 25_0
			unsigned ime_pm0_line3_signc                      : 1;        // bits : 26
		} bit;
		UINT32 word;
	} reg_272; // 0x0440

	union {
		struct {
			unsigned ime_pm1_type                        : 1;     // bits : 0
			unsigned                            : 3;
			unsigned ime_pm1_color_y                      : 8;        // bits : 11_4
			unsigned ime_pm1_color_u                      : 8;        // bits : 19_12
			unsigned ime_pm1_color_v                      : 8;        // bits : 27_20
		} bit;
		UINT32 word;
	} reg_273; // 0x0444

	union {
		struct {
			unsigned ime_pm1_line0_comp                        : 2;       // bits : 1_0
			unsigned                                : 2;
			unsigned ime_pm1_line0_coefa                      : 13;       // bits : 16_4
			unsigned ime_pm1_line0_signa                      : 1;        // bits : 17
			unsigned ime_pm1_line0_coefb                      : 13;       // bits : 30_18
			unsigned ime_pm1_line0_signb                      : 1;        // bits : 31
		} bit;
		UINT32 word;
	} reg_274; // 0x0448

	union {
		struct {
			unsigned ime_pm1_line0_coefc                      : 26;       // bits : 25_0
			unsigned ime_pm1_line0_signc                      : 1;        // bits : 26
		} bit;
		UINT32 word;
	} reg_275; // 0x044c

	union {
		struct {
			unsigned ime_pm1_line1_comp                        : 2;       // bits : 1_0
			unsigned                                : 2;
			unsigned ime_pm1_line1_coefa                      : 13;       // bits : 16_4
			unsigned ime_pm1_line1_signa                      : 1;        // bits : 17
			unsigned ime_pm1_line1_coefb                      : 13;       // bits : 30_18
			unsigned ime_pm1_line1_signb                      : 1;        // bits : 31
		} bit;
		UINT32 word;
	} reg_276; // 0x0450

	union {
		struct {
			unsigned ime_pm1_line1_coefc                      : 26;       // bits : 25_0
			unsigned ime_pm1_line1_signc                      : 1;        // bits : 26
		} bit;
		UINT32 word;
	} reg_277; // 0x0454

	union {
		struct {
			unsigned ime_pm1_line2_comp             : 2;      // bits : 1_0
			unsigned                                : 2;
			unsigned ime_pm1_line2_coefa                      : 13;       // bits : 16_4
			unsigned ime_pm1_line2_signa                      : 1;        // bits : 17
			unsigned ime_pm1_line2_coefb                      : 13;       // bits : 30_18
			unsigned ime_pm1_line2_signb                      : 1;        // bits : 31
		} bit;
		UINT32 word;
	} reg_278; // 0x0458

	union {
		struct {
			unsigned ime_pm1_line2_coefc                      : 26;       // bits : 25_0
			unsigned ime_pm1_line2_signc                      : 1;        // bits : 26
		} bit;
		UINT32 word;
	} reg_279; // 0x045c

	union {
		struct {
			unsigned ime_pm1_line3_comp                        : 2;       // bits : 1_0
			unsigned                                : 2;
			unsigned ime_pm1_line3_coefa                      : 13;       // bits : 16_4
			unsigned ime_pm1_line3_signa                      : 1;        // bits : 17
			unsigned ime_pm1_line3_coefb                      : 13;       // bits : 30_18
			unsigned ime_pm1_line3_signb                      : 1;        // bits : 31
		} bit;
		UINT32 word;
	} reg_280; // 0x0460

	union {
		struct {
			unsigned ime_pm1_line3_coefc                      : 26;       // bits : 25_0
			unsigned ime_pm1_line3_signc                      : 1;        // bits : 26
		} bit;
		UINT32 word;
	} reg_281; // 0x0464

	union {
		struct {
			unsigned ime_pm2_type                        : 1;     // bits : 0
			unsigned                            : 3;
			unsigned ime_pm2_color_y                      : 8;        // bits : 11_4
			unsigned ime_pm2_color_u                      : 8;        // bits : 19_12
			unsigned ime_pm2_color_v                      : 8;        // bits : 27_20
			unsigned                            : 3;
			unsigned ime_pm2_hlw_en             : 1;      // bits : 31
		} bit;
		UINT32 word;
	} reg_282; // 0x0468

	union {
		struct {
			unsigned ime_pm2_line0_comp                        : 2;       // bits : 1_0
			unsigned                                : 2;
			unsigned ime_pm2_line0_coefa                      : 13;       // bits : 16_4
			unsigned ime_pm2_line0_signa                      : 1;        // bits : 17
			unsigned ime_pm2_line0_coefb                      : 13;       // bits : 30_18
			unsigned ime_pm2_line0_signb                      : 1;        // bits : 31
		} bit;
		UINT32 word;
	} reg_283; // 0x046c

	union {
		struct {
			unsigned ime_pm2_line0_coefc                      : 26;       // bits : 25_0
			unsigned ime_pm2_line0_signc                      : 1;        // bits : 26
		} bit;
		UINT32 word;
	} reg_284; // 0x0470

	union {
		struct {
			unsigned ime_pm2_line1_comp                        : 2;       // bits : 1_0
			unsigned                                : 2;
			unsigned ime_pm2_line1_coefa                      : 13;       // bits : 16_4
			unsigned ime_pm2_line1_signa                      : 1;        // bits : 17
			unsigned ime_pm2_line1_coefb                      : 13;       // bits : 30_18
			unsigned ime_pm2_line1_signb                      : 1;        // bits : 31
		} bit;
		UINT32 word;
	} reg_285; // 0x0474

	union {
		struct {
			unsigned ime_pm2_line1_coefc                      : 26;       // bits : 25_0
			unsigned ime_pm2_line1_signc                      : 1;        // bits : 26
		} bit;
		UINT32 word;
	} reg_286; // 0x0478

	union {
		struct {
			unsigned ime_pm2_line2_comp                        : 2;       // bits : 1_0
			unsigned                                : 2;
			unsigned ime_pm2_line2_coefa                      : 13;       // bits : 16_4
			unsigned ime_pm2_line2_signa                      : 1;        // bits : 17
			unsigned ime_pm2_line2_coefb                      : 13;       // bits : 30_18
			unsigned ime_pm2_line2_signb                      : 1;        // bits : 31
		} bit;
		UINT32 word;
	} reg_287; // 0x047c

	union {
		struct {
			unsigned ime_pm2_line2_coefc                      : 26;       // bits : 25_0
			unsigned ime_pm2_line2_signc                      : 1;        // bits : 26
		} bit;
		UINT32 word;
	} reg_288; // 0x0480

	union {
		struct {
			unsigned ime_pm2_line3_comp                        : 2;       // bits : 1_0
			unsigned                                : 2;
			unsigned ime_pm2_line3_coefa                      : 13;       // bits : 16_4
			unsigned ime_pm2_line3_signa                      : 1;        // bits : 17
			unsigned ime_pm2_line3_coefb                      : 13;       // bits : 30_18
			unsigned ime_pm2_line3_signb                      : 1;        // bits : 31
		} bit;
		UINT32 word;
	} reg_289; // 0x0484

	union {
		struct {
			unsigned ime_pm2_line3_coefc                      : 26;       // bits : 25_0
			unsigned ime_pm2_line3_signc                      : 1;        // bits : 26
		} bit;
		UINT32 word;
	} reg_290; // 0x0488

	union {
		struct {
			unsigned ime_pm3_type                        : 1;     // bits : 0
			unsigned                            : 3;
			unsigned ime_pm3_color_y                      : 8;        // bits : 11_4
			unsigned ime_pm3_color_u                      : 8;        // bits : 19_12
			unsigned ime_pm3_color_v                      : 8;        // bits : 27_20
		} bit;
		UINT32 word;
	} reg_291; // 0x048c

	union {
		struct {
			unsigned ime_pm3_line0_comp                        : 2;       // bits : 1_0
			unsigned                                : 2;
			unsigned ime_pm3_line0_coefa                      : 13;       // bits : 16_4
			unsigned ime_pm3_line0_signa                      : 1;        // bits : 17
			unsigned ime_pm3_line0_coefb                      : 13;       // bits : 30_18
			unsigned ime_pm3_line0_signb                      : 1;        // bits : 31
		} bit;
		UINT32 word;
	} reg_292; // 0x0490

	union {
		struct {
			unsigned ime_pm3_line0_coefc                      : 26;       // bits : 25_0
			unsigned ime_pm3_line0_signc                      : 1;        // bits : 26
		} bit;
		UINT32 word;
	} reg_293; // 0x0494

	union {
		struct {
			unsigned ime_pm3_line1_comp                        : 2;       // bits : 1_0
			unsigned                                : 2;
			unsigned ime_pm3_line1_coefa                      : 13;       // bits : 16_4
			unsigned ime_pm3_line1_signa                      : 1;        // bits : 17
			unsigned ime_pm3_line1_coefb                      : 13;       // bits : 30_18
			unsigned ime_pm3_line1_signb                      : 1;        // bits : 31
		} bit;
		UINT32 word;
	} reg_294; // 0x0498

	union {
		struct {
			unsigned ime_pm3_line1_coefc                      : 26;       // bits : 25_0
			unsigned ime_pm3_line1_signc                      : 1;        // bits : 26
		} bit;
		UINT32 word;
	} reg_295; // 0x049c

	union {
		struct {
			unsigned ime_pm3_line2_comp                           : 2;        // bits : 1_0
			unsigned                                   : 2;
			unsigned ime_pm3_line2_coefa                                  : 13;       // bits : 16_4
			unsigned ime_pm3_line2_signa                         : 1;     // bits : 17
			unsigned ime_pm3_line2_coefb                         : 13;        // bits : 30_18
			unsigned ime_pm3_line2_signb                         : 1;     // bits : 31
		} bit;
		UINT32 word;
	} reg_296; // 0x04a0

	union {
		struct {
			unsigned ime_pm3_line2_coefc                      : 26;       // bits : 25_0
			unsigned ime_pm3_line2_signc                      : 1;        // bits : 26
		} bit;
		UINT32 word;
	} reg_297; // 0x04a4

	union {
		struct {
			unsigned ime_pm3_line3_comp                        : 2;       // bits : 1_0
			unsigned                                : 2;
			unsigned ime_pm3_line3_coefa                      : 13;       // bits : 16_4
			unsigned ime_pm3_line3_signa                      : 1;        // bits : 17
			unsigned ime_pm3_line3_coefb                      : 13;       // bits : 30_18
			unsigned ime_pm3_line3_signb                      : 1;        // bits : 31
		} bit;
		UINT32 word;
	} reg_298; // 0x04a8

	union {
		struct {
			unsigned ime_pm3_line3_coefc                      : 26;       // bits : 25_0
			unsigned ime_pm3_line3_signc                      : 1;        // bits : 26
		} bit;
		UINT32 word;
	} reg_299; // 0x04ac

	union {
		struct {
			unsigned ime_pm0_awet                     : 8;        // bits : 7_0
			unsigned ime_pm1_awet                     : 8;        // bits : 15_8
			unsigned ime_pm2_awet                     : 8;        // bits : 23_16
			unsigned ime_pm3_awet                     : 8;        // bits : 31_24
		} bit;
		UINT32 word;
	} reg_300; // 0x04b0

	union {
		struct {
			unsigned ime_pm4_type                        : 1;     // bits : 0
			unsigned                            : 3;
			unsigned ime_pm4_color_y                      : 8;        // bits : 11_4
			unsigned ime_pm4_color_u                      : 8;        // bits : 19_12
			unsigned ime_pm4_color_v                      : 8;        // bits : 27_20
			unsigned                            : 3;
			unsigned ime_pm4_hlw_en             : 1;      // bits : 31
		} bit;
		UINT32 word;
	} reg_301; // 0x04b4

	union {
		struct {
			unsigned ime_pm4_line0_comp                        : 2;       // bits : 1_0
			unsigned                                : 2;
			unsigned ime_pm4_line0_coefa                      : 13;       // bits : 16_4
			unsigned ime_pm4_line0_signa                      : 1;        // bits : 17
			unsigned ime_pm4_line0_coefb                      : 13;       // bits : 30_18
			unsigned ime_pm4_line0_signb                      : 1;        // bits : 31
		} bit;
		UINT32 word;
	} reg_302; // 0x04b8

	union {
		struct {
			unsigned ime_pm4_line0_coefc                      : 26;       // bits : 25_0
			unsigned ime_pm4_line0_signc                      : 1;        // bits : 26
		} bit;
		UINT32 word;
	} reg_303; // 0x04bc

	union {
		struct {
			unsigned ime_pm4_line1_comp                        : 2;       // bits : 1_0
			unsigned                                : 2;
			unsigned ime_pm4_line1_coefa                      : 13;       // bits : 16_4
			unsigned ime_pm4_line1_signa                      : 1;        // bits : 17
			unsigned ime_pm4_line1_coefb                      : 13;       // bits : 30_18
			unsigned ime_pm4_line1_signb                      : 1;        // bits : 31
		} bit;
		UINT32 word;
	} reg_304; // 0x04c0

	union {
		struct {
			unsigned ime_pm4_line1_coefc                      : 26;       // bits : 25_0
			unsigned ime_pm4_line1_signc                      : 1;        // bits : 26
		} bit;
		UINT32 word;
	} reg_305; // 0x04c4

	union {
		struct {
			unsigned ime_pm4_line2_comp                        : 2;       // bits : 1_0
			unsigned                                : 2;
			unsigned ime_pm4_line2_coefa                      : 13;       // bits : 16_4
			unsigned ime_pm4_line2_signa                      : 1;        // bits : 17
			unsigned ime_pm4_line2_coefb                      : 13;       // bits : 30_18
			unsigned ime_pm4_line2_signb                      : 1;        // bits : 31
		} bit;
		UINT32 word;
	} reg_306; // 0x04c8

	union {
		struct {
			unsigned ime_pm4_line2_coefc                      : 26;       // bits : 25_0
			unsigned ime_pm4_line2_signc                      : 1;        // bits : 26
		} bit;
		UINT32 word;
	} reg_307; // 0x04cc

	union {
		struct {
			unsigned ime_pm4_line3_comp                        : 2;       // bits : 1_0
			unsigned                                : 2;
			unsigned ime_pm4_line3_coefa                      : 13;       // bits : 16_4
			unsigned ime_pm4_line3_signa                      : 1;        // bits : 17
			unsigned ime_pm4_line3_coefb                      : 13;       // bits : 30_18
			unsigned ime_pm4_line3_signb                      : 1;        // bits : 31
		} bit;
		UINT32 word;
	} reg_308; // 0x04d0

	union {
		struct {
			unsigned ime_pm4_line3_coefc                      : 26;       // bits : 25_0
			unsigned ime_pm4_line3_signc                      : 1;        // bits : 26
		} bit;
		UINT32 word;
	} reg_309; // 0x04d4

	union {
		struct {
			unsigned ime_pm5_type                        : 1;     // bits : 0
			unsigned                            : 3;
			unsigned ime_pm5_color_y                      : 8;        // bits : 11_4
			unsigned ime_pm5_color_u                      : 8;        // bits : 19_12
			unsigned ime_pm5_color_v                      : 8;        // bits : 27_20
		} bit;
		UINT32 word;
	} reg_310; // 0x04d8

	union {
		struct {
			unsigned ime_pm5_line0_comp                        : 2;       // bits : 1_0
			unsigned                                : 2;
			unsigned ime_pm5_line0_coefa                      : 13;       // bits : 16_4
			unsigned ime_pm5_line0_signa                      : 1;        // bits : 17
			unsigned ime_pm5_line0_coefb                      : 13;       // bits : 30_18
			unsigned ime_pm5_line0_signb                      : 1;        // bits : 31
		} bit;
		UINT32 word;
	} reg_311; // 0x04dc

	union {
		struct {
			unsigned ime_pm5_line0_coefc                      : 26;       // bits : 25_0
			unsigned ime_pm5_line0_signc                      : 1;        // bits : 26
		} bit;
		UINT32 word;
	} reg_312; // 0x04e0

	union {
		struct {
			unsigned ime_pm5_line1_comp                        : 2;       // bits : 1_0
			unsigned                                : 2;
			unsigned ime_pm5_line1_coefa                      : 13;       // bits : 16_4
			unsigned ime_pm5_line1_signa                      : 1;        // bits : 17
			unsigned ime_pm5_line1_coefb                      : 13;       // bits : 30_18
			unsigned ime_pm5_line1_signb                      : 1;        // bits : 31
		} bit;
		UINT32 word;
	} reg_313; // 0x04e4

	union {
		struct {
			unsigned ime_pm5_line1_coefc                      : 26;       // bits : 25_0
			unsigned ime_pm5_line1_signc                      : 1;        // bits : 26
		} bit;
		UINT32 word;
	} reg_314; // 0x04e8

	union {
		struct {
			unsigned ime_pm5_line2_comp                        : 2;       // bits : 1_0
			unsigned                                : 2;
			unsigned ime_pm5_line2_coefa                      : 13;       // bits : 16_4
			unsigned ime_pm5_line2_signa                      : 1;        // bits : 17
			unsigned ime_pm5_line2_coefb                      : 13;       // bits : 30_18
			unsigned ime_pm5_line2_signb                      : 1;        // bits : 31
		} bit;
		UINT32 word;
	} reg_315; // 0x04ec

	union {
		struct {
			unsigned ime_pm5_line2_coefc                      : 26;       // bits : 25_0
			unsigned ime_pm5_line2_signc                      : 1;        // bits : 26
		} bit;
		UINT32 word;
	} reg_316; // 0x04f0

	union {
		struct {
			unsigned ime_pm5_line3_comp                        : 2;       // bits : 1_0
			unsigned                                : 2;
			unsigned ime_pm5_line3_coefa                      : 13;       // bits : 16_4
			unsigned ime_pm5_line3_signa                      : 1;        // bits : 17
			unsigned ime_pm5_line3_coefb                      : 13;       // bits : 30_18
			unsigned ime_pm5_line3_signb                      : 1;        // bits : 31
		} bit;
		UINT32 word;
	} reg_317; // 0x04f4

	union {
		struct {
			unsigned ime_pm5_line3_coefc                      : 26;       // bits : 25_0
			unsigned ime_pm5_line3_signc                      : 1;        // bits : 26
		} bit;
		UINT32 word;
	} reg_318; // 0x04f8

	union {
		struct {
			unsigned ime_pm6_type                        : 1;     // bits : 0
			unsigned                            : 3;
			unsigned ime_pm6_color_y                      : 8;        // bits : 11_4
			unsigned ime_pm6_color_u                      : 8;        // bits : 19_12
			unsigned ime_pm6_color_v                      : 8;        // bits : 27_20
			unsigned                            : 3;
			unsigned ime_pm6_hlw_en             : 1;      // bits : 31
		} bit;
		UINT32 word;
	} reg_319; // 0x04fc

	union {
		struct {
			unsigned ime_pm6_line0_comp                        : 2;       // bits : 1_0
			unsigned                                : 2;
			unsigned ime_pm6_line0_coefa                      : 13;       // bits : 16_4
			unsigned ime_pm6_line0_signa                      : 1;        // bits : 17
			unsigned ime_pm6_line0_coefb                      : 13;       // bits : 30_18
			unsigned ime_pm6_line0_signb                      : 1;        // bits : 31
		} bit;
		UINT32 word;
	} reg_320; // 0x0500

	union {
		struct {
			unsigned ime_pm6_line0_coefc                      : 26;       // bits : 25_0
			unsigned ime_pm6_line0_signc                      : 1;        // bits : 26
		} bit;
		UINT32 word;
	} reg_321; // 0x0504

	union {
		struct {
			unsigned ime_pm6_line1_comp                        : 2;       // bits : 1_0
			unsigned                                : 2;
			unsigned ime_pm6_line1_coefa                      : 13;       // bits : 16_4
			unsigned ime_pm6_line1_signa                      : 1;        // bits : 17
			unsigned ime_pm6_line1_coefb                      : 13;       // bits : 30_18
			unsigned ime_pm6_line1_signb                      : 1;        // bits : 31
		} bit;
		UINT32 word;
	} reg_322; // 0x0508

	union {
		struct {
			unsigned ime_pm6_line1_coefc                      : 26;       // bits : 25_0
			unsigned ime_pm6_line1_signc                      : 1;        // bits : 26
		} bit;
		UINT32 word;
	} reg_323; // 0x050c

	union {
		struct {
			unsigned ime_pm6_line2_comp                        : 2;       // bits : 1_0
			unsigned                                : 2;
			unsigned ime_pm6_line2_coefa                      : 13;       // bits : 16_4
			unsigned ime_pm6_line2_signa                      : 1;        // bits : 17
			unsigned ime_pm6_line2_coefb                      : 13;       // bits : 30_18
			unsigned ime_pm6_line2_signb                      : 1;        // bits : 31
		} bit;
		UINT32 word;
	} reg_324; // 0x0510

	union {
		struct {
			unsigned ime_pm6_line2_coefc                      : 26;       // bits : 25_0
			unsigned ime_pm6_line2_signc                      : 1;        // bits : 26
		} bit;
		UINT32 word;
	} reg_325; // 0x0514

	union {
		struct {
			unsigned ime_pm6_line3_comp                        : 2;       // bits : 1_0
			unsigned                                : 2;
			unsigned ime_pm6_line3_coefa                      : 13;       // bits : 16_4
			unsigned ime_pm6_line3_signa                      : 1;        // bits : 17
			unsigned ime_pm6_line3_coefb                      : 13;       // bits : 30_18
			unsigned ime_pm6_line3_signb                      : 1;        // bits : 31
		} bit;
		UINT32 word;
	} reg_326; // 0x0518

	union {
		struct {
			unsigned ime_pm6_line3_coefc                      : 26;       // bits : 25_0
			unsigned ime_pm6_line3_signc                      : 1;        // bits : 26
		} bit;
		UINT32 word;
	} reg_327; // 0x051c

	union {
		struct {
			unsigned ime_pm7_type                        : 1;     // bits : 0
			unsigned                            : 3;
			unsigned ime_pm7_color_y                      : 8;        // bits : 11_4
			unsigned ime_pm7_color_u                      : 8;        // bits : 19_12
			unsigned ime_pm7_color_v                      : 8;        // bits : 27_20
		} bit;
		UINT32 word;
	} reg_328; // 0x0520

	union {
		struct {
			unsigned ime_pm7_line0_comp                        : 2;       // bits : 1_0
			unsigned                                : 2;
			unsigned ime_pm7_line0_coefa                      : 13;       // bits : 16_4
			unsigned ime_pm7_line0_signa                      : 1;        // bits : 17
			unsigned ime_pm7_line0_coefb                      : 13;       // bits : 30_18
			unsigned ime_pm7_line0_signb                      : 1;        // bits : 31
		} bit;
		UINT32 word;
	} reg_329; // 0x0524

	union {
		struct {
			unsigned ime_pm7_line0_coefc                      : 26;       // bits : 25_0
			unsigned ime_pm7_line0_signc                      : 1;        // bits : 26
		} bit;
		UINT32 word;
	} reg_330; // 0x0528

	union {
		struct {
			unsigned ime_pm7_line1_comp                        : 2;       // bits : 1_0
			unsigned                                : 2;
			unsigned ime_pm7_line1_coefa                      : 13;       // bits : 16_4
			unsigned ime_pm7_line1_signa                      : 1;        // bits : 17
			unsigned ime_pm7_line1_coefb                      : 13;       // bits : 30_18
			unsigned ime_pm7_line1_signb                      : 1;        // bits : 31
		} bit;
		UINT32 word;
	} reg_331; // 0x052c

	union {
		struct {
			unsigned ime_pm7_line1_coefc                      : 26;       // bits : 25_0
			unsigned ime_pm7_line1_signc                      : 1;        // bits : 26
		} bit;
		UINT32 word;
	} reg_332; // 0x0530

	union {
		struct {
			unsigned ime_pm7_line2_comp                        : 2;       // bits : 1_0
			unsigned                                : 2;
			unsigned ime_pm7_line2_coefa                      : 13;       // bits : 16_4
			unsigned ime_pm7_line2_signa                      : 1;        // bits : 17
			unsigned ime_pm7_line2_coefb                      : 13;       // bits : 30_18
			unsigned ime_pm7_line2_signb                      : 1;        // bits : 31
		} bit;
		UINT32 word;
	} reg_333; // 0x0534

	union {
		struct {
			unsigned ime_pm7_line2_coefc                      : 26;       // bits : 25_0
			unsigned ime_pm7_line2_signc                      : 1;        // bits : 26
		} bit;
		UINT32 word;
	} reg_334; // 0x0538

	union {
		struct {
			unsigned ime_pm7_line3_comp                        : 2;       // bits : 1_0
			unsigned                                : 2;
			unsigned ime_pm7_line3_coefa                      : 13;       // bits : 16_4
			unsigned ime_pm7_line3_signa                      : 1;        // bits : 17
			unsigned ime_pm7_line3_coefb                      : 13;       // bits : 30_18
			unsigned ime_pm7_line3_signb                      : 1;        // bits : 31
		} bit;
		UINT32 word;
	} reg_335; // 0x053c

	union {
		struct {
			unsigned ime_pm7_line3_coefc                      : 26;       // bits : 25_0
			unsigned ime_pm7_line3_signc                      : 1;        // bits : 26
		} bit;
		UINT32 word;
	} reg_336; // 0x0540

	union {
		struct {
			unsigned ime_pm4_awet                     : 8;        // bits : 7_0
			unsigned ime_pm5_awet                     : 8;        // bits : 15_8
			unsigned ime_pm6_awet                     : 8;        // bits : 23_16
			unsigned ime_pm7_awet                     : 8;        // bits : 31_24
		} bit;
		UINT32 word;
	} reg_337; // 0x0544

	union {
		struct {
			unsigned ime_pm_h_size        : 11;       // bits : 10_0
			unsigned                      : 5;
			unsigned ime_pm_v_size        : 11;       // bits : 26_16
			unsigned                      : 3;
			unsigned ime_pm_fmt           : 2;        // bits : 31_30
		} bit;
		UINT32 word;
	} reg_338; // 0x0548

	union {
		struct {
			unsigned                      : 2;
			unsigned ime_pm_y_ofsi        : 18;       // bits : 19_2
		} bit;
		UINT32 word;
	} reg_339; // 0x054c

	union {
		struct {
			unsigned                     : 2;
			unsigned ime_pm_y_sai        : 30;        // bits : 31_2
		} bit;
		UINT32 word;
	} reg_340; // 0x0550

	union {
		struct {
			unsigned ime_in_pxl_bp1        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_341; // 0x0554

	union {
		struct {
			unsigned ime_in_pxl_bp2        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_342; // 0x0558

	union {
		struct {
			unsigned ime_in_pxl_bp3        : 32;      // bits : 31_0
		} bit;
		UINT32 word;
	} reg_343; // 0x055c

	union {
		struct {
			unsigned ime_in_line_bp1        : 16;      // bits : 15_0
			unsigned ime_in_line_bp2        : 16;      // bits : 31_16
		} bit;
		UINT32 word;
	} reg_344; // 0x0560

	union {
		struct {
			unsigned ime_in_line_bp3        : 16;      // bits : 15_0
			unsigned                        : 15;      // bits : 30_16
			unsigned ime_bp_mode            : 1;       // bits : 31
		} bit;
		UINT32 word;
	} reg_345; // 0x0564

	union {
		struct {
			unsigned reserved                     : 32;       // bits : 31_0
		} bit;
		UINT32 word;
	} reg_346; // 0x0568

	union {
		struct {
			unsigned reserved                     : 32;       // bits : 31_0
		} bit;
		UINT32 word;
	} reg_347; // 0x056c

	union {
		struct {
			unsigned ime_in_bst_y                          : 1;       // bits : 0
			unsigned ime_in_bst_u                          : 1;       // bits : 1
			unsigned ime_in_bst_v                          : 1;       // bits : 2
			unsigned ime_out_p1_bst_y                      : 2;       // bits : 4_3
			unsigned ime_out_p1_bst_u                      : 2;       // bits : 6_5
			unsigned ime_out_p1_bst_v                      : 1;       // bits : 7
			unsigned ime_out_p2_bst_y                      : 2;       // bits : 9_8
			unsigned ime_out_p2_bst_uv                        : 2;        // bits : 11_10
			unsigned ime_out_p3_bst_y                      : 1;       // bits : 12
			unsigned ime_out_p3_bst_uv                        : 1;        // bits : 13
			unsigned ime_out_p4_bst_y                      : 1;       // bits : 14
			unsigned ime_in_lca_bst                          : 1;     // bits : 15
			unsigned ime_out_lca_bst              : 1;        // bits : 16
			unsigned ime_in_stp_bst                          : 1;     // bits : 17
			unsigned ime_in_pix_bst               : 1;        // bits : 18
			unsigned ime_in_tmnr_bst_y            : 1;        // bits : 19
			unsigned ime_in_tmnr_bst_c            : 1;        // bits : 20
			unsigned ime_out_tmnr_bst_y           : 2;        // bits : 22_21
			unsigned ime_out_tmnr_bst_c           : 2;        // bits : 24_23
			unsigned ime_in_3dnr_bst_mv                      : 1;       // bits : 25
			unsigned ime_out_3dnr_bst_mv                     : 1;       // bits : 26
			unsigned ime_in_3dnr_bst_mo                      : 1;       // bits : 27
			unsigned ime_out_3dnr_bst_mo                     : 1;       // bits : 28
			unsigned ime_out_3dnr_bst_mo_roi                 : 1;       // bits : 29
			unsigned ime_out_3dnr_bst_sta                    : 1;       // bits : 30
			unsigned ime_out_md_bst_evt           : 1;      // bits : 31
		} bit;
		UINT32 word;
	} reg_348; // 0x0570

	union {
		struct {
			unsigned reserved                     : 30;       // bits : 29_0
			unsigned ime_frm_start_rst            : 1;
			unsigned ime_chksum_en                : 1;
		} bit;
		UINT32 word;
	} reg_349; // 0x0574

	union {
		struct {
			unsigned reserved                     : 32;       // bits : 31_0
		} bit;
		UINT32 word;
	} reg_350; // 0x0578

	union {
		struct {
			unsigned reserved                     : 32;       // bits : 31_0
		} bit;
		UINT32 word;
	} reg_351; // 0x057c

	union {
		struct {
			unsigned ime_ll_tab0        : 8;      // bits : 7_0
			unsigned ime_ll_tab1        : 8;      // bits : 15_8
			unsigned ime_ll_tab2        : 8;      // bits : 23_16
			unsigned ime_ll_tab3        : 8;      // bits : 31_24
		} bit;
		UINT32 word;
	} reg_352; // 0x0580

	union {
		struct {
			unsigned ime_ll_tab4        : 8;      // bits : 7_0
			unsigned ime_ll_tab5        : 8;      // bits : 15_8
			unsigned ime_ll_tab6        : 8;      // bits : 23_16
			unsigned ime_ll_tab7        : 8;      // bits : 31_24
		} bit;
		UINT32 word;
	} reg_353; // 0x0584

	union {
		struct {
			unsigned ime_ll_tab8         : 8;     // bits : 7_0
			unsigned ime_ll_tab9         : 8;     // bits : 15_8
			unsigned ime_ll_tab10        : 8;     // bits : 23_16
			unsigned ime_ll_tab11        : 8;     // bits : 31_24
		} bit;
		UINT32 word;
	} reg_354; // 0x0588

	union {
		struct {
			unsigned ime_ll_tab12        : 8;     // bits : 7_0
			unsigned ime_ll_tab13        : 8;     // bits : 15_8
			unsigned ime_ll_tab14        : 8;     // bits : 23_16
			unsigned ime_ll_tab15        : 8;     // bits : 31_24
		} bit;
		UINT32 word;
	} reg_355; // 0x058c

	union {
		struct {
			unsigned reserved                     : 32;       // bits : 31_0
		} bit;
		UINT32 word;
	} reg_356; // 0x0590

	union {
		struct {
			unsigned reserved                     : 32;       // bits : 31_0
		} bit;
		UINT32 word;
	} reg_357; // 0x0594

	union {
		struct {
			unsigned reserved                     : 32;       // bits : 31_0
		} bit;
		UINT32 word;
	} reg_358; // 0x0598

	union {
		struct {
			unsigned reserved                     : 32;       // bits : 31_0
		} bit;
		UINT32 word;
	} reg_359; // 0x059c

	union {
		struct {
			unsigned reserved                     : 32;       // bits : 31_0
		} bit;
		UINT32 word;
	} reg_360; // 0x05a0

	union {
		struct {
			unsigned reserved                     : 32;       // bits : 31_0
		} bit;
		UINT32 word;
	} reg_361; // 0x05a4

	union {
		struct {
			unsigned reserved                     : 32;       // bits : 31_0
		} bit;
		UINT32 word;
	} reg_362; // 0x05a8

	union {
		struct {
			unsigned reserved                     : 32;       // bits : 31_0
		} bit;
		UINT32 word;
	} reg_363; // 0x05ac

	union {
		struct {
			unsigned reserved                     : 32;       // bits : 31_0
		} bit;
		UINT32 word;
	} reg_364; // 0x05b0

	union {
		struct {
			unsigned reserved                     : 32;       // bits : 31_0
		} bit;
		UINT32 word;
	} reg_365; // 0x05b4

	union {
		struct {
			unsigned reserved                     : 32;       // bits : 31_0
		} bit;
		UINT32 word;
	} reg_366; // 0x05b8

	union {
		struct {
			unsigned reserved                     : 32;       // bits : 31_0
		} bit;
		UINT32 word;
	} reg_367; // 0x05bc

	union {
		struct {
			unsigned reserved                     : 32;       // bits : 31_0
		} bit;
		UINT32 word;
	} reg_368; // 0x05c0

	union {
		struct {
			unsigned reserved                     : 32;       // bits : 31_0
		} bit;
		UINT32 word;
	} reg_369; // 0x05c4

	union {
		struct {
			unsigned reserved                     : 32;       // bits : 31_0
		} bit;
		UINT32 word;
	} reg_370; // 0x05c8

	union {
		struct {
			unsigned reserved                     : 32;       // bits : 31_0
		} bit;
		UINT32 word;
	} reg_371; // 0x05cc

	union {
		struct {
			unsigned reserved                     : 32;       // bits : 31_0
		} bit;
		UINT32 word;
	} reg_372; // 0x05d0

	union {
		struct {
			unsigned reserved                     : 32;       // bits : 31_0
		} bit;
		UINT32 word;
	} reg_373; // 0x05d4

	union {
		struct {
			unsigned reserved                     : 32;       // bits : 31_0
		} bit;
		UINT32 word;
	} reg_374; // 0x05d8

	union {
		struct {
			unsigned reserved                     : 32;       // bits : 31_0
		} bit;
		UINT32 word;
	} reg_375; // 0x05dc

	union {
		struct {
			unsigned reserved                     : 32;       // bits : 31_0
		} bit;
		UINT32 word;
	} reg_376; // 0x05e0

	union {
		struct {
			unsigned reserved                     : 32;       // bits : 31_0
		} bit;
		UINT32 word;
	} reg_377; // 0x05e4

	union {
		struct {
			unsigned reserved                     : 32;       // bits : 31_0
		} bit;
		UINT32 word;
	} reg_378; // 0x05e8

	union {
		struct {
			unsigned reserved                     : 32;       // bits : 31_0
		} bit;
		UINT32 word;
	} reg_379; // 0x05ec

	union {
		struct {
			unsigned reserved                     : 32;       // bits : 31_0
		} bit;
		UINT32 word;
	} reg_380; // 0x05f0

	union {
		struct {
			unsigned reserved                     : 32;       // bits : 31_0
		} bit;
		UINT32 word;
	} reg_381; // 0x05f4

	union {
		struct {
			unsigned reserved                     : 32;       // bits : 31_0
		} bit;
		UINT32 word;
	} reg_382; // 0x05f8

	union {
		struct {
			unsigned reserved                     : 32;       // bits : 31_0
		} bit;
		UINT32 word;
	} reg_383; // 0x05fc

	union {
		struct {
			unsigned ime_3dnr_pre_y_blur_str              : 2;      // bits : 1_0
			unsigned ime_3dnr_pf_type                     : 2;      // bits : 3_2
			unsigned ime_3dnr_me_update_mode              : 1;      // bits : 4
			unsigned ime_3dnr_me_boundary_set             : 1;      // bits : 5
			unsigned ime_3dnr_me_mv_ds_mode               : 2;      // bits : 7_6
			unsigned ime_3dnr_ps_smart_roi_ctrl           : 1;      // bits : 8
			unsigned ime_3dnr_nr_center_wzero_y_3d        : 1;      // bits : 9
			unsigned ime_3dnr_ps_mv_check_en              : 1;      // bits : 10
			unsigned ime_3dnr_ps_mv_check_roi_en          : 1;      // bits : 11
			unsigned ime_3dnr_ps_mv_info_mode             : 2;      // bits : 13_12
			unsigned ime_3dnr_ps_mode                     : 1;      // bits : 14
			unsigned ime_3dnr_me_sad_type                 : 1;      // bits : 15
			unsigned ime_3dnr_me_sad_shift                : 4;      // bits : 19_16
			unsigned ime_3dnr_nr_y_ch_en                  : 1;      // bits : 20
			unsigned ime_3dnr_nr_c_ch_en                  : 1;      // bits : 21
			unsigned ime_3dnr_nr_c_fsv_en                 : 1;      // bits : 22
			unsigned ime_3dnr_seed_reset_en               : 1;      // bits : 23
			unsigned ime_3dnr_nr_c_fsv                    : 8;      // bits : 31_24
		} bit;
		UINT32 word;
	} reg_384; // 0x0600

	union {
		struct {
			unsigned ime_3dnr_ne_sample_step_x           : 8;       // bits : 7_0
			unsigned ime_3dnr_ne_sample_step_y           : 8;       // bits : 15_8
			unsigned ime_3dnr_statistic_output_en        : 1;       // bits : 16
			unsigned ime_3dnr_ps_fastc_en                : 1;       // bits : 17
			unsigned ime_3dnr_dbg_mv0                    : 1;       // bits : 18
			unsigned                                     : 9;
			unsigned ime_3dnr_dbg_mode                   : 4;       // bits : 31_28
		} bit;
		UINT32 word;
	} reg_385; // 0x0604

	union {
		struct {
			unsigned reserved                     : 32;       // bits : 31_0
		} bit;
		UINT32 word;
	} reg_386; // 0x0608

	union {
		struct {
			unsigned reserved                     : 32;       // bits : 31_0
		} bit;
		UINT32 word;
	} reg_387; // 0x060c

	union {
		struct {
			unsigned reserved                     : 32;       // bits : 31_0
		} bit;
		UINT32 word;
	} reg_388; // 0x0610

	union {
		struct {
			unsigned reserved                     : 32;       // bits : 31_0
		} bit;
		UINT32 word;
	} reg_389; // 0x0614

	union {
		struct {
			unsigned reserved                     : 32;       // bits : 31_0
		} bit;
		UINT32 word;
	} reg_390; // 0x0618

	union {
		struct {
			unsigned ime_3dnr_ro_sad_sum        : 32;       // bits : 31_0
		} bit;
		UINT32 word;
	} reg_391; // 0x061c

	union {
		struct {
			unsigned ime_3dnr_ro_mv_sum        : 32;        // bits : 31_0
		} bit;
		UINT32 word;
	} reg_392; // 0x0620

	union {
		struct {
			unsigned ime_3dnr_ro_sample_cnt        : 32;        // bits : 31_0
		} bit;
		UINT32 word;
	} reg_393; // 0x0624

	union {
		struct {
			unsigned ime_3dnr_me_sad_penalty_0        : 10;       // bits : 9_0
			unsigned ime_3dnr_me_sad_penalty_1        : 10;       // bits : 19_10
			unsigned ime_3dnr_me_sad_penalty_2        : 10;       // bits : 29_20
		} bit;
		UINT32 word;
	} reg_394; // 0x0628

	union {
		struct {
			unsigned ime_3dnr_me_sad_penalty_3        : 10;       // bits : 9_0
			unsigned ime_3dnr_me_sad_penalty_4        : 10;       // bits : 19_10
			unsigned ime_3dnr_me_sad_penalty_5        : 10;       // bits : 29_20
		} bit;
		UINT32 word;
	} reg_395; // 0x062c

	union {
		struct {
			unsigned ime_3dnr_me_sad_penalty_6        : 10;       // bits : 9_0
			unsigned ime_3dnr_me_sad_penalty_7        : 10;       // bits : 19_10
		} bit;
		UINT32 word;
	} reg_396; // 0x0630

	union {
		struct {
			unsigned ime_3dnr_me_switch_th0        : 8;       // bits : 7_0
			unsigned ime_3dnr_me_switch_th1        : 8;       // bits : 15_8
			unsigned ime_3dnr_me_switch_th2        : 8;       // bits : 23_16
			unsigned ime_3dnr_me_switch_th3        : 8;       // bits : 31_24
		} bit;
		UINT32 word;
	} reg_397; // 0x0634

	union {
		struct {
			unsigned ime_3dnr_me_switch_th4        : 8;       // bits : 7_0
			unsigned ime_3dnr_me_switch_th5        : 8;       // bits : 15_8
			unsigned ime_3dnr_me_switch_th6        : 8;       // bits : 23_16
			unsigned ime_3dnr_me_switch_th7        : 8;       // bits : 31_24
		} bit;
		UINT32 word;
	} reg_398; // 0x0638

	union {
		struct {
			unsigned ime_3dnr_me_switch_ratio        : 8;       // bits : 7_0
			unsigned ime_3dnr_me_cost_blend          : 4;       // bits : 11_8
		} bit;
		UINT32 word;
	} reg_399; // 0x063c

	union {
		struct {
			unsigned ime_3dnr_me_detail_penalty0        : 4;      // bits : 3_0
			unsigned ime_3dnr_me_detail_penalty1        : 4;      // bits : 7_4
			unsigned ime_3dnr_me_detail_penalty2        : 4;      // bits : 11_8
			unsigned ime_3dnr_me_detail_penalty3        : 4;      // bits : 15_12
			unsigned ime_3dnr_me_detail_penalty4        : 4;      // bits : 19_16
			unsigned ime_3dnr_me_detail_penalty5        : 4;      // bits : 23_20
			unsigned ime_3dnr_me_detail_penalty6        : 4;      // bits : 27_24
			unsigned ime_3dnr_me_detail_penalty7        : 4;      // bits : 31_28
		} bit;
		UINT32 word;
	} reg_400; // 0x0640

	union {
		struct {
			unsigned ime_3dnr_me_probability0        : 1;     // bits : 0
			unsigned ime_3dnr_me_probability1        : 1;     // bits : 1
			unsigned ime_3dnr_me_probability2        : 1;     // bits : 2
			unsigned ime_3dnr_me_probability3        : 1;     // bits : 3
			unsigned ime_3dnr_me_probability4        : 1;     // bits : 4
			unsigned ime_3dnr_me_probability5        : 1;     // bits : 5
			unsigned ime_3dnr_me_probability6        : 1;     // bits : 6
			unsigned ime_3dnr_me_probability7        : 1;     // bits : 7
			unsigned ime_3dnr_me_rand_bit_x          : 3;     // bits : 10_8
			unsigned                                 : 1;
			unsigned ime_3dnr_me_rand_bit_y          : 3;     // bits : 14_12
			unsigned                                 : 1;
			unsigned ime_3dnr_me_min_detail          : 14;        // bits : 29_16
		} bit;
		UINT32 word;
	} reg_401; // 0x0644

	union {
		struct {
			unsigned reserved        : 32;     // bits : 31_0
		} bit;
		UINT32 word;
	} reg_402; // 0x0648

	union {
		struct {
			unsigned ime_3dnr_ne_sample_num_x        : 12;        // bits : 11_0
			unsigned                                 : 4;
			unsigned ime_3dnr_ne_sample_num_y        : 12;        // bits : 27_16
		} bit;
		UINT32 word;
	} reg_403; // 0x064c

	union {
		struct {
			unsigned ime_3dnr_ne_sample_start_x        : 12;      // bits : 11_0
			unsigned                                   : 4;
			unsigned ime_3dnr_ne_sample_start_y        : 12;      // bits : 27_16
		} bit;
		UINT32 word;
	} reg_404; // 0x0650

	union {
		struct {
			unsigned ime_3dnr_fast_converge_sp          : 4;        // bits : 3_0
			unsigned ime_3dnr_fast_converge_step        : 4;        // bits : 7_4
		} bit;
		UINT32 word;
	} reg_405; // 0x0654

	union {
		struct {
			unsigned reserved                     : 32;       // bits : 31_0
		} bit;
		UINT32 word;
	} reg_406; // 0x0658

	union {
		struct {
			unsigned reserved                     : 32;       // bits : 31_0
		} bit;
		UINT32 word;
	} reg_407; // 0x065c

	union {
		struct {
			unsigned ime_3dnr_md_sad_coef_a0        : 6;      // bits : 5_0
			unsigned                                : 2;
			unsigned ime_3dnr_md_sad_coef_a1        : 6;      // bits : 13_8
			unsigned                                : 2;
			unsigned ime_3dnr_md_sad_coef_a2        : 6;      // bits : 21_16
			unsigned                                : 2;
			unsigned ime_3dnr_md_sad_coef_a3        : 6;      // bits : 29_24
		} bit;
		UINT32 word;
	} reg_408; // 0x0660

	union {
		struct {
			unsigned ime_3dnr_md_sad_coef_a4        : 6;      // bits : 5_0
			unsigned                                : 2;
			unsigned ime_3dnr_md_sad_coef_a5        : 6;      // bits : 13_8
			unsigned                                : 2;
			unsigned ime_3dnr_md_sad_coef_a6        : 6;      // bits : 21_16
			unsigned                                : 2;
			unsigned ime_3dnr_md_sad_coef_a7        : 6;      // bits : 29_24
		} bit;
		UINT32 word;
	} reg_409; // 0x0664

	union {
		struct {
			unsigned ime_3dnr_md_sad_coef_b0        : 14;     // bits : 13_0
			unsigned                                : 2;
			unsigned ime_3dnr_md_sad_coef_b1        : 14;     // bits : 29_16
		} bit;
		UINT32 word;
	} reg_410; // 0x0668

	union {
		struct {
			unsigned ime_3dnr_md_sad_coef_b2        : 14;     // bits : 13_0
			unsigned                                : 2;
			unsigned ime_3dnr_md_sad_coef_b3        : 14;     // bits : 29_16
		} bit;
		UINT32 word;
	} reg_411; // 0x066c

	union {
		struct {
			unsigned ime_3dnr_md_sad_coef_b4        : 14;     // bits : 13_0
			unsigned                                : 2;
			unsigned ime_3dnr_md_sad_coef_b5        : 14;     // bits : 29_16
		} bit;
		UINT32 word;
	} reg_412; // 0x0670

	union {
		struct {
			unsigned ime_3dnr_md_sad_coef_b6        : 14;     // bits : 13_0
			unsigned                                : 2;
			unsigned ime_3dnr_md_sad_coef_b7        : 14;     // bits : 29_16
		} bit;
		UINT32 word;
	} reg_413; // 0x0674

	union {
		struct {
			unsigned ime_3dnr_md_sad_std0        : 14;        // bits : 13_0
			unsigned                             : 2;
			unsigned ime_3dnr_md_sad_std1        : 14;        // bits : 29_16
		} bit;
		UINT32 word;
	} reg_414; // 0x0678

	union {
		struct {
			unsigned ime_3dnr_md_sad_std2        : 14;        // bits : 13_0
			unsigned                             : 2;
			unsigned ime_3dnr_md_sad_std3        : 14;        // bits : 29_16
		} bit;
		UINT32 word;
	} reg_415; // 0x067c

	union {
		struct {
			unsigned ime_3dnr_md_sad_std4        : 14;        // bits : 13_0
			unsigned                             : 2;
			unsigned ime_3dnr_md_sad_std5        : 14;        // bits : 29_16
		} bit;
		UINT32 word;
	} reg_416; // 0x0680

	union {
		struct {
			unsigned ime_3dnr_md_sad_std6        : 14;        // bits : 13_0
			unsigned                             : 2;
			unsigned ime_3dnr_md_sad_std7        : 14;        // bits : 29_16
		} bit;
		UINT32 word;
	} reg_417; // 0x0684

	union {
		struct {
			unsigned ime_3dnr_md_k1        : 6;       // bits : 5_0
			unsigned                       : 2;
			unsigned ime_3dnr_md_k2        : 6;       // bits : 13_8
		} bit;
		UINT32 word;
	} reg_418; // 0x0688

	union {
		struct {
			unsigned reserved                     : 32;       // bits : 31_0
		} bit;
		UINT32 word;
	} reg_419; // 0x068c

	union {
		struct {
			unsigned reserved                     : 32;       // bits : 31_0
		} bit;
		UINT32 word;
	} reg_420; // 0x0690

	union {
		struct {
			unsigned reserved                     : 32;       // bits : 31_0
		} bit;
		UINT32 word;
	} reg_421; // 0x0694

	union {
		struct {
			unsigned reserved                     : 32;       // bits : 31_0
		} bit;
		UINT32 word;
	} reg_422; // 0x0698

	union {
		struct {
			unsigned reserved                     : 32;       // bits : 31_0
		} bit;
		UINT32 word;
	} reg_423; // 0x069c

	union {
		struct {
			unsigned ime_3dnr_mc_sad_base0        : 14;       // bits : 13_0
			unsigned                              : 2;
			unsigned ime_3dnr_mc_sad_base1        : 14;       // bits : 29_16
		} bit;
		UINT32 word;
	} reg_424; // 0x06a0

	union {
		struct {
			unsigned ime_3dnr_mc_sad_base2        : 14;       // bits : 13_0
			unsigned                              : 2;
			unsigned ime_3dnr_mc_sad_base3        : 14;       // bits : 29_16
		} bit;
		UINT32 word;
	} reg_425; // 0x06a4

	union {
		struct {
			unsigned ime_3dnr_mc_sad_base4        : 14;       // bits : 13_0
			unsigned                              : 2;
			unsigned ime_3dnr_mc_sad_base5        : 14;       // bits : 29_16
		} bit;
		UINT32 word;
	} reg_426; // 0x06a8

	union {
		struct {
			unsigned ime_3dnr_mc_sad_base6        : 14;       // bits : 13_0
			unsigned                              : 2;
			unsigned ime_3dnr_mc_sad_base7        : 14;       // bits : 29_16
		} bit;
		UINT32 word;
	} reg_427; // 0x06ac

	union {
		struct {
			unsigned ime_3dnr_mc_sad_coef_a0        : 6;      // bits : 5_0
			unsigned                                : 2;
			unsigned ime_3dnr_mc_sad_coef_a1        : 6;      // bits : 13_8
			unsigned                                : 2;
			unsigned ime_3dnr_mc_sad_coef_a2        : 6;      // bits : 21_16
			unsigned                                : 2;
			unsigned ime_3dnr_mc_sad_coef_a3        : 6;      // bits : 29_24
		} bit;
		UINT32 word;
	} reg_428; // 0x06b0

	union {
		struct {
			unsigned ime_3dnr_mc_sad_coef_a4        : 6;      // bits : 5_0
			unsigned                                : 2;
			unsigned ime_3dnr_mc_sad_coef_a5        : 6;      // bits : 13_8
			unsigned                                : 2;
			unsigned ime_3dnr_mc_sad_coef_a6        : 6;      // bits : 21_16
			unsigned                                : 2;
			unsigned ime_3dnr_mc_sad_coef_a7        : 6;      // bits : 29_24
		} bit;
		UINT32 word;
	} reg_429; // 0x06b4

	union {
		struct {
			unsigned ime_3dnr_mc_sad_coef_b0        : 14;     // bits : 13_0
			unsigned                                : 2;
			unsigned ime_3dnr_mc_sad_coef_b1        : 14;     // bits : 29_16
		} bit;
		UINT32 word;
	} reg_430; // 0x06b8

	union {
		struct {
			unsigned ime_3dnr_mc_sad_coef_b2        : 14;     // bits : 13_0
			unsigned                                : 2;
			unsigned ime_3dnr_mc_sad_coef_b3        : 14;     // bits : 29_16
		} bit;
		UINT32 word;
	} reg_431; // 0x06bc

	union {
		struct {
			unsigned ime_3dnr_mc_sad_coef_b4        : 14;     // bits : 13_0
			unsigned                                : 2;
			unsigned ime_3dnr_mc_sad_coef_b5        : 14;     // bits : 29_16
		} bit;
		UINT32 word;
	} reg_432; // 0x06c0

	union {
		struct {
			unsigned ime_3dnr_mc_sad_coef_b6        : 14;     // bits : 13_0
			unsigned                                : 2;
			unsigned ime_3dnr_mc_sad_coef_b7        : 14;     // bits : 29_16
		} bit;
		UINT32 word;
	} reg_433; // 0x06c4

	union {
		struct {
			unsigned ime_3dnr_mc_sad_std0        : 14;        // bits : 13_0
			unsigned                             : 2;
			unsigned ime_3dnr_mc_sad_std1        : 14;        // bits : 29_16
		} bit;
		UINT32 word;
	} reg_434; // 0x06c8

	union {
		struct {
			unsigned ime_3dnr_mc_sad_std2        : 14;        // bits : 13_0
			unsigned                             : 2;
			unsigned ime_3dnr_mc_sad_std3        : 14;        // bits : 29_16
		} bit;
		UINT32 word;
	} reg_435; // 0x06cc

	union {
		struct {
			unsigned ime_3dnr_mc_sad_std4        : 14;        // bits : 13_0
			unsigned                             : 2;
			unsigned ime_3dnr_mc_sad_std5        : 14;        // bits : 29_16
		} bit;
		UINT32 word;
	} reg_436; // 0x06d0

	union {
		struct {
			unsigned ime_3dnr_mc_sad_std6        : 14;        // bits : 13_0
			unsigned                             : 2;
			unsigned ime_3dnr_mc_sad_std7        : 14;        // bits : 29_16
		} bit;
		UINT32 word;
	} reg_437; // 0x06d4

	union {
		struct {
			unsigned ime_3dnr_mc_k1        : 6;       // bits : 5_0
			unsigned                       : 2;
			unsigned ime_3dnr_mc_k2        : 6;       // bits : 13_8
		} bit;
		UINT32 word;
	} reg_438; // 0x06d8

	union {
		struct {
			unsigned reserved                     : 32;       // bits : 31_0
		} bit;
		UINT32 word;
	} reg_439; // 0x06dc

	union {
		struct {
			unsigned reserved                     : 32;       // bits : 31_0
		} bit;
		UINT32 word;
	} reg_440; // 0x06e0

	union {
		struct {
			unsigned reserved                     : 32;       // bits : 31_0
		} bit;
		UINT32 word;
	} reg_441; // 0x06e4

	union {
		struct {
			unsigned reserved                     : 32;       // bits : 31_0
		} bit;
		UINT32 word;
	} reg_442; // 0x06e8

	union {
		struct {
			unsigned reserved                     : 32;       // bits : 31_0
		} bit;
		UINT32 word;
	} reg_443; // 0x06ec

	union {
		struct {
			unsigned ime_3dnr_roi_md_k1        : 6;       // bits : 5_0
			unsigned                           : 2;
			unsigned ime_3dnr_roi_md_k2        : 6;       // bits : 13_8
		} bit;
		UINT32 word;
	} reg_444; // 0x06f0

	union {
		struct {
			unsigned reserved                     : 32;       // bits : 31_0
		} bit;
		UINT32 word;
	} reg_445; // 0x06f4

	union {
		struct {
			unsigned reserved                     : 32;       // bits : 31_0
		} bit;
		UINT32 word;
	} reg_446; // 0x06f8

	union {
		struct {
			unsigned reserved                     : 32;       // bits : 31_0
		} bit;
		UINT32 word;
	} reg_447; // 0x06fc

	union {
		struct {
			unsigned reserved                     : 32;       // bits : 31_0
		} bit;
		UINT32 word;
	} reg_448; // 0x0700

	union {
		struct {
			unsigned reserved                     : 32;       // bits : 31_0
		} bit;
		UINT32 word;
	} reg_449; // 0x0704

	union {
		struct {
			unsigned reserved                     : 32;       // bits : 31_0
		} bit;
		UINT32 word;
	} reg_450; // 0x0708

	union {
		struct {
			unsigned reserved                     : 32;       // bits : 31_0
		} bit;
		UINT32 word;
	} reg_451; // 0x070c

	union {
		struct {
			unsigned ime_3dnr_roi_mc_k1        : 6;       // bits : 5_0
			unsigned                           : 2;
			unsigned ime_3dnr_roi_mc_k2        : 6;       // bits : 13_8
		} bit;
		UINT32 word;
	} reg_452; // 0x0710

	union {
		struct {
			unsigned ime_3dnr_nr_y_tf0_blur_str0        : 8;        // bits : 7_0
			unsigned ime_3dnr_nr_y_tf0_blur_str1        : 8;        // bits : 15_8
			unsigned ime_3dnr_nr_y_tf0_blur_str2        : 8;        // bits : 23_16
			unsigned ime_3dnr_nr_y_tf0_str0             : 8;        // bits : 31_24
		} bit;
		UINT32 word;
	} reg_453; // 0x0714

	union {
		struct {
			unsigned ime_3dnr_nr_y_tf0_str1        : 8;     // bits : 7_0
			unsigned ime_3dnr_nr_y_tf0_str2        : 8;     // bits : 15_8
			unsigned ime_3dnr_nr_c_tf0_str0        : 8;     // bits : 23_16
			unsigned ime_3dnr_nr_c_tf0_str1        : 8;     // bits : 31_24
		} bit;
		UINT32 word;
	} reg_454; // 0x0718

	union {
		struct {
			unsigned ime_3dnr_nr_c_tf0_str2        : 8;     // bits : 7_0
		} bit;
		UINT32 word;
	} reg_455; // 0x071c

	union {
		struct {
			unsigned reserved                     : 32;       // bits : 31_0
		} bit;
		UINT32 word;
	} reg_456; // 0x0720

	union {
		struct {
			unsigned reserved                     : 32;       // bits : 31_0
		} bit;
		UINT32 word;
	} reg_457; // 0x0724

	union {
		struct {
			unsigned reserved                     : 32;       // bits : 31_0
		} bit;
		UINT32 word;
	} reg_458; // 0x0728

	union {
		struct {
			unsigned reserved                     : 32;       // bits : 31_0
		} bit;
		UINT32 word;
	} reg_459; // 0x072c

	union {
		struct {
			unsigned ime_3dnr_ps_mv_th            : 6;        // bits : 5_0
			unsigned                              : 2;
			unsigned ime_3dnr_ps_roi_mv_th        : 6;        // bits : 13_8
			unsigned                              : 2;
			unsigned ime_3dnr_ps_fs_th            : 14;       // bits : 29_16
		} bit;
		UINT32 word;
	} reg_460; // 0x0730

	union {
		struct {
			unsigned ime_3dnr_ps_mix_ratio0        : 8;       // bits : 7_0
			unsigned ime_3dnr_ps_mix_ratio1        : 8;       // bits : 15_8
		} bit;
		UINT32 word;
	} reg_461; // 0x0734

	union {
		struct {
			unsigned ime_3dnr_ps_mix_th0        : 14;     // bits : 13_0
			unsigned                            : 2;
			unsigned ime_3dnr_ps_mix_th1        : 14;     // bits : 29_16
		} bit;
		UINT32 word;
	} reg_462; // 0x0738

	union {
		struct {
			unsigned ime_3dnr_ps_mix_slope0        : 16;      // bits : 15_0
			unsigned ime_3dnr_ps_mix_slope1        : 16;      // bits : 31_16
		} bit;
		UINT32 word;
	} reg_463; // 0x073c

	union {
		struct {
			unsigned reserved                     : 32;       // bits : 31_0
		} bit;
		UINT32 word;
	} reg_464; // 0x0740

	union {
		struct {
			unsigned reserved                     : 32;       // bits : 31_0
		} bit;
		UINT32 word;
	} reg_465; // 0x0744

	union {
		struct {
			unsigned reserved                     : 32;       // bits : 31_0
		} bit;
		UINT32 word;
	} reg_466; // 0x0748

	union {
		struct {
			unsigned reserved                     : 32;       // bits : 31_0
		} bit;
		UINT32 word;
	} reg_467; // 0x074c

	union {
		struct {
			unsigned reserved                     : 32;       // bits : 31_0
		} bit;
		UINT32 word;
	} reg_468; // 0x0750

	union {
		struct {
			unsigned reserved                     : 32;       // bits : 31_0
		} bit;
		UINT32 word;
	} reg_469; // 0x0754

	union {
		struct {
			unsigned reserved                     : 32;       // bits : 31_0
		} bit;
		UINT32 word;
	} reg_470; // 0x0758

	union {
		struct {
			unsigned reserved                     : 32;       // bits : 31_0
		} bit;
		UINT32 word;
	} reg_471; // 0x075c

	union {
		struct {
			unsigned ime_3dnr_ps_ds_th            : 5;        // bits : 4_0
			unsigned                              : 3;
			unsigned ime_3dnr_ps_ds_th_roi        : 5;        // bits : 12_8
			unsigned                              : 3;
			unsigned ime_3dnr_ps_edge_w           : 8;        // bits : 23_16
		} bit;
		UINT32 word;
	} reg_472; // 0x0760

	union {
		struct {
			unsigned ime_3dnr_ps_edge_th0        : 14;        // bits : 13_0
			unsigned                             : 2;
			unsigned ime_3dnr_ps_edge_th1        : 14;        // bits : 29_16
		} bit;
		UINT32 word;
	} reg_473; // 0x0764

	union {
		struct {
			unsigned ime_3dnr_ps_edge_slope        : 16;      // bits : 15_0
		} bit;
		UINT32 word;
	} reg_474; // 0x0768

	union {
		struct {
			unsigned ime_3dnr_nr_residue_th_y0        : 4;      // bits : 3_0
			unsigned                                  : 4;
			unsigned ime_3dnr_nr_residue_th_y1        : 4;      // bits : 11_8
			unsigned                                  : 4;
			unsigned ime_3dnr_nr_residue_th_y2        : 4;      // bits : 19_16
			unsigned                                  : 4;
			unsigned ime_3dnr_nr_residue_th_c         : 4;      // bits : 27_24
		} bit;
		UINT32 word;
	} reg_475; // 0x076c

	union {
		struct {
			unsigned ime_3dnr_nr_freq_w0        : 8;      // bits : 7_0
			unsigned ime_3dnr_nr_freq_w1        : 8;      // bits : 15_8
			unsigned ime_3dnr_nr_freq_w2        : 8;      // bits : 23_16
			unsigned ime_3dnr_nr_freq_w3        : 8;      // bits : 31_24
		} bit;
		UINT32 word;
	} reg_476; // 0x0770

	union {
		struct {
			unsigned reserved                     : 32;       // bits : 31_0
		} bit;
		UINT32 word;
	} reg_477; // 0x0774

	union {
		struct {
			unsigned reserved                     : 32;       // bits : 31_0
		} bit;
		UINT32 word;
	} reg_478; // 0x0778

	union {
		struct {
			unsigned reserved                     : 32;       // bits : 31_0
		} bit;
		UINT32 word;
	} reg_479; // 0x077c

	union {
		struct {
			unsigned reserved                     : 32;       // bits : 31_0
		} bit;
		UINT32 word;
	} reg_480; // 0x0780

	union {
		struct {
			unsigned reserved                     : 32;       // bits : 31_0
		} bit;
		UINT32 word;
	} reg_481; // 0x0784

	union {
		struct {
			unsigned reserved                     : 32;       // bits : 31_0
		} bit;
		UINT32 word;
	} reg_482; // 0x0788

	union {
		struct {
			unsigned reserved                     : 32;       // bits : 31_0
		} bit;
		UINT32 word;
	} reg_483; // 0x078c

	union {
		struct {
			unsigned ime_3dnr_nr_luma_w0        : 8;      // bits : 7_0
			unsigned ime_3dnr_nr_luma_w1        : 8;      // bits : 15_8
			unsigned ime_3dnr_nr_luma_w2        : 8;      // bits : 23_16
			unsigned ime_3dnr_nr_luma_w3        : 8;      // bits : 31_24
		} bit;
		UINT32 word;
	} reg_484; // 0x0790

	union {
		struct {
			unsigned ime_3dnr_nr_luma_w4        : 8;      // bits : 7_0
			unsigned ime_3dnr_nr_luma_w5        : 8;      // bits : 15_8
			unsigned ime_3dnr_nr_luma_w6        : 8;      // bits : 23_16
			unsigned ime_3dnr_nr_luma_w7        : 8;      // bits : 31_24
		} bit;
		UINT32 word;
	} reg_485; // 0x0794

	union {
		struct {
			unsigned reserved                     : 32;       // bits : 31_0
		} bit;
		UINT32 word;
	} reg_486; // 0x0798

	union {
		struct {
			unsigned reserved                     : 32;       // bits : 31_0
		} bit;
		UINT32 word;
	} reg_487; // 0x079c

	union {
		struct {
			unsigned reserved                     : 32;       // bits : 31_0
		} bit;
		UINT32 word;
	} reg_488; // 0x07a0

	union {
		struct {
			unsigned reserved                     : 32;       // bits : 31_0
		} bit;
		UINT32 word;
	} reg_489; // 0x07a4

	union {
		struct {
			unsigned reserved                     : 32;       // bits : 31_0
		} bit;
		UINT32 word;
	} reg_490; // 0x07a8

	union {
		struct {
			unsigned reserved                     : 32;       // bits : 31_0
		} bit;
		UINT32 word;
	} reg_491; // 0x07ac

	union {
		struct {
			unsigned ime_3dnr_nr_pre_filtering_str0        : 8;       // bits : 7_0
			unsigned ime_3dnr_nr_pre_filtering_str1        : 8;       // bits : 15_8
			unsigned ime_3dnr_nr_pre_filtering_str2        : 8;       // bits : 23_16
			unsigned ime_3dnr_nr_pre_filtering_str3        : 8;       // bits : 31_24
		} bit;
		UINT32 word;
	} reg_492; // 0x07b0

	union {
		struct {
			unsigned reserved                     : 32;       // bits : 31_0
		} bit;
		UINT32 word;
	} reg_493; // 0x07b4

	union {
		struct {
			unsigned reserved                     : 32;       // bits : 31_0
		} bit;
		UINT32 word;
	} reg_494; // 0x07b8

	union {
		struct {
			unsigned reserved                     : 32;       // bits : 31_0
		} bit;
		UINT32 word;
	} reg_495; // 0x07bc

	union {
		struct {
			unsigned ime_3dnr_nr_pre_filtering_ratio0        : 8;     // bits : 7_0
			unsigned ime_3dnr_nr_pre_filtering_ratio1        : 8;     // bits : 15_8
			unsigned ime_3dnr_nr_snr_str0                    : 8;     // bits : 23_16
			unsigned ime_3dnr_nr_snr_str1                    : 8;     // bits : 31_24
		} bit;
		UINT32 word;
	} reg_496; // 0x07c0

	union {
		struct {
			unsigned ime_3dnr_nr_snr_str2        : 8;     // bits : 7_0
			unsigned ime_3dnr_nr_tnr_str0        : 8;     // bits : 15_8
			unsigned ime_3dnr_nr_tnr_str1        : 8;     // bits : 23_16
			unsigned ime_3dnr_nr_tnr_str2        : 8;     // bits : 31_24
		} bit;
		UINT32 word;
	} reg_497; // 0x07c4

	union {
		struct {
			unsigned ime_3dnr_nr_base_th_snr        : 16;     // bits : 15_0
			unsigned ime_3dnr_nr_base_th_tnr        : 16;     // bits : 31_16
		} bit;
		UINT32 word;
	} reg_498; // 0x07c8

	union {
		struct {
			unsigned ime_3dnr_nr_y_3d_ratio0        : 8;        // bits : 7_0
			unsigned ime_3dnr_nr_y_3d_ratio1        : 8;        // bits : 15_8
		} bit;
		UINT32 word;
	} reg_499; // 0x07cc

	union {
		struct {
			unsigned ime_3dnr_nr_y_3d_lut0        : 7;        // bits : 6_0
			unsigned                              : 1;
			unsigned ime_3dnr_nr_y_3d_lut1        : 7;        // bits : 14_8
			unsigned                              : 1;
			unsigned ime_3dnr_nr_y_3d_lut2        : 7;        // bits : 22_16
			unsigned                              : 1;
			unsigned ime_3dnr_nr_y_3d_lut3        : 7;        // bits : 30_24
		} bit;
		UINT32 word;
	} reg_500; // 0x07d0

	union {
		struct {
			unsigned ime_3dnr_nr_y_3d_lut4        : 7;        // bits : 6_0
			unsigned                              : 1;
			unsigned ime_3dnr_nr_y_3d_lut5        : 7;        // bits : 14_8
			unsigned                              : 1;
			unsigned ime_3dnr_nr_y_3d_lut6        : 7;        // bits : 22_16
			unsigned                              : 1;
			unsigned ime_3dnr_nr_y_3d_lut7        : 7;        // bits : 30_24
		} bit;
		UINT32 word;
	} reg_501; // 0x07d4

	union {
		struct {
			unsigned ime_3dnr_nr_c_3d_lut0        : 7;        // bits : 6_0
			unsigned                              : 1;
			unsigned ime_3dnr_nr_c_3d_lut1        : 7;        // bits : 14_8
			unsigned                              : 1;
			unsigned ime_3dnr_nr_c_3d_lut2        : 7;        // bits : 22_16
			unsigned                              : 1;
			unsigned ime_3dnr_nr_c_3d_lut3        : 7;        // bits : 30_24
		} bit;
		UINT32 word;
	} reg_502; // 0x07d8

	union {
		struct {
			unsigned ime_3dnr_nr_c_3d_lut4        : 7;        // bits : 6_0
			unsigned                              : 1;
			unsigned ime_3dnr_nr_c_3d_lut5        : 7;        // bits : 14_8
			unsigned                              : 1;
			unsigned ime_3dnr_nr_c_3d_lut6        : 7;        // bits : 22_16
			unsigned                              : 1;
			unsigned ime_3dnr_nr_c_3d_lut7        : 7;        // bits : 30_24
		} bit;
		UINT32 word;
	} reg_503; // 0x07dc

	union {
		struct {
			unsigned ime_3dnr_nr_c_3d_ratio0        : 8;      // bits : 7_0
			unsigned ime_3dnr_nr_c_3d_ratio1        : 8;      // bits : 15_8
		} bit;
		UINT32 word;
	} reg_504; // 0x07e0

	union {
		struct {
			unsigned reserved                     : 32;       // bits : 31_0
		} bit;
		UINT32 word;
	} reg_505; // 0x07e4

	union {
		struct {
			unsigned reserved                     : 32;       // bits : 31_0
		} bit;
		UINT32 word;
	} reg_506; // 0x07e8

	union {
		struct {
			unsigned reserved                     : 32;       // bits : 31_0
		} bit;
		UINT32 word;
	} reg_507; // 0x07ec

	union {
		struct {
			unsigned reserved                     : 32;       // bits : 31_0
		} bit;
		UINT32 word;
	} reg_508; // 0x07f0

	union {
		struct {
			unsigned reserved                     : 32;       // bits : 31_0
		} bit;
		UINT32 word;
	} reg_509; // 0x07f4

	union {
		struct {
			unsigned reserved                     : 32;       // bits : 31_0
		} bit;
		UINT32 word;
	} reg_510; // 0x07f8

	union {
		struct {
			unsigned reserved                     : 32;       // bits : 31_0
		} bit;
		UINT32 word;
	} reg_511; // 0x07fc

	union {
		struct {
			unsigned                                 : 2;
			unsigned ime_3dnr_ref_dram_y_ofsi        : 18;        // bits : 19_2
		} bit;
		UINT32 word;
	} reg_512; // 0x0800

	union {
		struct {
			unsigned                                  : 2;
			unsigned ime_3dnr_ref_dram_uv_ofsi        : 18;       // bits : 19_2
		} bit;
		UINT32 word;
	} reg_513; // 0x0804

	union {
		struct {
			unsigned                                : 2;
			unsigned ime_3dnr_ref_dram_y_sai        : 30;     // bits : 31_2
		} bit;
		UINT32 word;
	} reg_514; // 0x0808

	union {
		struct {
			unsigned                                 : 2;
			unsigned ime_3dnr_ref_dram_uv_sai        : 30;        // bits : 31_2
		} bit;
		UINT32 word;
	} reg_515; // 0x080c

	union {
		struct {
			unsigned                                 : 2;
			unsigned ime_3dnr_ref_dram_y_ofso        : 18;        // bits : 19_2
		} bit;
		UINT32 word;
	} reg_516; // 0x0810

	union {
		struct {
			unsigned                                  : 2;
			unsigned ime_3dnr_ref_dram_uv_ofso        : 18;       // bits : 19_2
		} bit;
		UINT32 word;
	} reg_517; // 0x0814

	union {
		struct {
			unsigned                                : 2;
			unsigned ime_3dnr_ref_dram_y_sao        : 30;     // bits : 31_2
		} bit;
		UINT32 word;
	} reg_518; // 0x0818

	union {
		struct {
			unsigned                                 : 2;
			unsigned ime_3dnr_ref_dram_uv_sao        : 30;        // bits : 31_2
		} bit;
		UINT32 word;
	} reg_519; // 0x081c

	union {
		struct {
			unsigned                               : 2;
			unsigned ime_3dnr_mot_dram_ofsi        : 18;      // bits : 19_2
		} bit;
		UINT32 word;
	} reg_520; // 0x0820

	union {
		struct {
			unsigned                              : 2;
			unsigned ime_3dnr_mot_dram_sai        : 30;       // bits : 31_2
		} bit;
		UINT32 word;
	} reg_521; // 0x0824

	union {
		struct {
			unsigned                               : 2;
			unsigned ime_3dnr_mot_dram_ofso        : 18;      // bits : 19_2
		} bit;
		UINT32 word;
	} reg_522; // 0x0828

	union {
		struct {
			unsigned                              : 2;
			unsigned ime_3dnr_mot_dram_sao        : 30;       // bits : 31_2
		} bit;
		UINT32 word;
	} reg_523; // 0x082c

	union {
		struct {
			unsigned                                   : 2;
			unsigned ime_3dnr_mot_roi_dram_ofso        : 18;      // bits : 19_2
		} bit;
		UINT32 word;
	} reg_524; // 0x0830

	union {
		struct {
			unsigned                                  : 2;
			unsigned ime_3dnr_mot_roi_dram_sao        : 30;       // bits : 31_2
		} bit;
		UINT32 word;
	} reg_525; // 0x0834

	union {
		struct {
			unsigned                              : 2;
			unsigned ime_3dnr_mv_dram_ofsi        : 18;       // bits : 19_2
		} bit;
		UINT32 word;
	} reg_526; // 0x0838

	union {
		struct {
			unsigned                             : 2;
			unsigned ime_3dnr_mv_dram_sai        : 30;        // bits : 31_2
		} bit;
		UINT32 word;
	} reg_527; // 0x083c

	union {
		struct {
			unsigned                              : 2;
			unsigned ime_3dnr_mv_dram_ofso        : 18;       // bits : 19_2
		} bit;
		UINT32 word;
	} reg_528; // 0x0840

	union {
		struct {
			unsigned                             : 2;
			unsigned ime_3dnr_mv_dram_sao        : 30;        // bits : 31_2
		} bit;
		UINT32 word;
	} reg_529; // 0x0844

	union {
		struct {
			unsigned                                     : 2;
			unsigned ime_3dnr_statistic_dram_ofso        : 18;        // bits : 19_2
		} bit;
		UINT32 word;
	} reg_530; // 0x0848

	union {
		struct {
			unsigned                                    : 2;
			unsigned ime_3dnr_statistic_dram_sao        : 30;     // bits : 31_2
		} bit;
		UINT32 word;
	} reg_531; // 0x084c

	union {
		struct {
			unsigned                                        : 2;
			unsigned ime_3dnr_fast_converge_dram_sai        : 30;       // bits : 31_2
		} bit;
		UINT32 word;
	} reg_532; // 0x0850

	union {
		struct {
			unsigned                                        : 2;
			unsigned ime_3dnr_fast_converge_dram_ofs        : 18;       // bits : 19_2
		} bit;
		UINT32 word;
	} reg_533; // 0x0854

	union {
		struct {
			unsigned                                        : 2;
			unsigned ime_3dnr_fast_converge_dram_sao        : 30;       // bits : 31_2
		} bit;
		UINT32 word;
	} reg_534; // 0x0858

	union {
		struct {
			unsigned reserved                     : 32;       // bits : 31_0
		} bit;
		UINT32 word;
	} reg_535; // 0x085c

	union {
		struct {
			unsigned reserved                     : 32;       // bits : 31_0
		} bit;
		UINT32 word;
	} reg_536; // 0x0860

	union {
		struct {
			unsigned reserved                     : 32;       // bits : 31_0
		} bit;
		UINT32 word;
	} reg_537; // 0x0864

	union {
		struct {
			unsigned reserved                     : 32;       // bits : 31_0
		} bit;
		UINT32 word;
	} reg_538; // 0x0868

	union {
		struct {
			unsigned reserved                     : 32;       // bits : 31_0
		} bit;
		UINT32 word;
	} reg_539; // 0x086c

	union {
		struct {
			unsigned reserved                     : 32;       // bits : 31_0
		} bit;
		UINT32 word;
	} reg_540; // 0x0870

	union {
		struct {
			unsigned reserved                     : 32;       // bits : 31_0
		} bit;
		UINT32 word;
	} reg_541; // 0x0874

	union {
		struct {
			unsigned reserved                     : 32;       // bits : 31_0
		} bit;
		UINT32 word;
	} reg_542; // 0x0878

	union {
		struct {
			unsigned reserved                     : 32;       // bits : 31_0
		} bit;
		UINT32 word;
	} reg_543; // 0x087c

	union {
		struct {
			unsigned                           : 2;
			unsigned ime_p1_uv_h_dnrate        : 5;       // bits : 6_2
			unsigned ime_p1_uv_v_dnrate        : 5;       // bits : 11_7
		} bit;
		UINT32 word;
	} reg_544; // 0x0880

	union {
		struct {
			unsigned ime_p1_uv_h_sfact        : 16;       // bits : 15_0
			unsigned ime_p1_uv_v_sfact        : 16;       // bits : 31_16
		} bit;
		UINT32 word;
	} reg_545; // 0x0884

	union {
		struct {
			unsigned reserved                     : 32;       // bits : 31_0
		} bit;
		UINT32 word;
	} reg_546; // 0x0888

	union {
		struct {
			unsigned reserved                     : 32;       // bits : 31_0
		} bit;
		UINT32 word;
	} reg_547; // 0x088c

	union {
		struct {
			unsigned reserved                     : 32;       // bits : 31_0
		} bit;
		UINT32 word;
	} reg_548; // 0x0890

	union {
		struct {
			unsigned reserved                     : 32;       // bits : 31_0
		} bit;
		UINT32 word;
	} reg_549; // 0x0894

	union {
		struct {
			unsigned reserved                     : 32;       // bits : 31_0
		} bit;
		UINT32 word;
	} reg_550; // 0x0898

	union {
		struct {
			unsigned reserved                     : 32;       // bits : 31_0
		} bit;
		UINT32 word;
	} reg_551; // 0x089c

	union {
		struct {
			unsigned                           : 2;
			unsigned ime_p2_uv_h_dnrate        : 5;       // bits : 6_2
			unsigned ime_p2_uv_v_dnrate        : 5;       // bits : 11_7
		} bit;
		UINT32 word;
	} reg_552; // 0x08a0

	union {
		struct {
			unsigned ime_p2_uv_h_sfact        : 16;       // bits : 15_0
			unsigned ime_p2_uv_v_sfact        : 16;       // bits : 31_16
		} bit;
		UINT32 word;
	} reg_553; // 0x08a4

	union {
		struct {
			unsigned ime_p2_uv_isd_h_base        : 13;        // bits : 12_0
			unsigned                             : 3;
			unsigned ime_p2_uv_isd_v_base        : 13;        // bits : 28_16
		} bit;
		UINT32 word;
	} reg_554; // 0x08a8

	union {
		struct {
			unsigned ime_p2_uv_isd_h_sfact0        : 13;      // bits : 12_0
			unsigned                               : 3;
			unsigned ime_p2_uv_isd_v_sfact0        : 13;      // bits : 28_16
		} bit;
		UINT32 word;
	} reg_555; // 0x08ac

	union {
		struct {
			unsigned ime_p2_uv_isd_h_sfact1        : 13;      // bits : 12_0
			unsigned                               : 3;
			unsigned ime_p2_uv_isd_v_sfact1        : 13;      // bits : 28_16
		} bit;
		UINT32 word;
	} reg_556; // 0x08b0

	union {
		struct {
			unsigned ime_p2_uv_isd_h_sfact2        : 13;      // bits : 12_0
			unsigned                               : 3;
			unsigned ime_p2_uv_isd_v_sfact2        : 13;      // bits : 28_16
		} bit;
		UINT32 word;
	} reg_557; // 0x08b4

	union {
		struct {
			unsigned reserved                     : 32;       // bits : 31_0
		} bit;
		UINT32 word;
	} reg_558; // 0x08b8

	union {
		struct {
			unsigned reserved                     : 32;       // bits : 31_0
		} bit;
		UINT32 word;
	} reg_559; // 0x08bc

	union {
		struct {
			unsigned                           : 2;
			unsigned ime_p3_uv_h_dnrate        : 5;       // bits : 6_2
			unsigned ime_p3_uv_v_dnrate        : 5;       // bits : 11_7
		} bit;
		UINT32 word;
	} reg_560; // 0x08c0

	union {
		struct {
			unsigned ime_p3_uv_h_sfact        : 16;       // bits : 15_0
			unsigned ime_p3_uv_v_sfact        : 16;       // bits : 31_16
		} bit;
		UINT32 word;
	} reg_561; // 0x08c4

	union {
		struct {
			unsigned ime_p3_uv_isd_h_base        : 13;        // bits : 12_0
			unsigned                             : 3;
			unsigned ime_p3_uv_isd_v_base        : 13;        // bits : 28_16
		} bit;
		UINT32 word;
	} reg_562; // 0x08c8

	union {
		struct {
			unsigned ime_p3_uv_isd_h_sfact0        : 13;      // bits : 12_0
			unsigned                               : 3;
			unsigned ime_p3_uv_isd_v_sfact0        : 13;      // bits : 28_16
		} bit;
		UINT32 word;
	} reg_563; // 0x08cc

	union {
		struct {
			unsigned ime_p3_uv_isd_h_sfact1        : 13;      // bits : 12_0
			unsigned                               : 3;
			unsigned ime_p3_uv_isd_v_sfact1        : 13;      // bits : 28_16
		} bit;
		UINT32 word;
	} reg_564; // 0x08d0

	union {
		struct {
			unsigned ime_p3_uv_isd_h_sfact2        : 13;      // bits : 12_0
			unsigned                               : 3;
			unsigned ime_p3_uv_isd_v_sfact2        : 13;      // bits : 28_16
		} bit;
		UINT32 word;
	} reg_565; // 0x08d4

	union {
		struct {
			unsigned reserved                     : 32;       // bits : 31_0
		} bit;
		UINT32 word;
	} reg_566; // 0x08d8

	union {
		struct {
			unsigned reserved                     : 32;       // bits : 31_0
		} bit;
		UINT32 word;
	} reg_567; // 0x08dc

	union {
		struct {
			unsigned reserved                     : 32;       // bits : 31_0
		} bit;
		UINT32 word;
	} reg_568; // 0x08e0

	union {
		struct {
			unsigned reserved                     : 32;       // bits : 31_0
		} bit;
		UINT32 word;
	} reg_569; // 0x08e4

	union {
		struct {
			unsigned reserved                     : 32;       // bits : 31_0
		} bit;
		UINT32 word;
	} reg_570; // 0x08e8

	union {
		struct {
			unsigned reserved                     : 32;       // bits : 31_0
		} bit;
		UINT32 word;
	} reg_571; // 0x08ec

	union {
		struct {
			unsigned reserved                     : 32;       // bits : 31_0
		} bit;
		UINT32 word;
	} reg_572; // 0x08f0

	union {
		struct {
			unsigned reserved                     : 32;       // bits : 31_0
		} bit;
		UINT32 word;
	} reg_573; // 0x08f4

	union {
		struct {
			unsigned reserved                     : 32;       // bits : 31_0
		} bit;
		UINT32 word;
	} reg_574; // 0x08f8

	union {
		struct {
			unsigned reserved                     : 32;       // bits : 31_0
		} bit;
		UINT32 word;
	} reg_575; // 0x08fc

	union {
		struct {
			unsigned ime_p1_enc_smode_en                    : 1;        // bits : 0
			unsigned ime_p1_enc_padding_mode                : 1;        // bits : 1
			unsigned                                        : 2;
			unsigned ime_3dnr_refout_enc_smode_en           : 1;        // bits : 4
			unsigned me_3dnr_refout_enc_padding_mode        : 1;        // bits : 5
			unsigned                                        : 2;
			unsigned ime_3dnr_refin_dec_smode_en            : 1;        // bits : 8
			unsigned ime_3dnr_refin_dec_padding_mode        : 1;        // bits : 9
			unsigned ime_3dnr_refin_dec_dither_en           : 1;        // bits : 10
			unsigned                                        : 5;
			unsigned ime_3dnr_refin_dec_dither_seed0        : 15;       // bits : 30_16
		} bit;
		UINT32 word;
	} reg_576; // 0x0900

	union {
		struct {
			unsigned ime_3dnr_refin_dec_dither_seed1        : 4;        // bits : 3_0
		} bit;
		UINT32 word;
	} reg_577; // 0x0904

	union {
		struct {
			unsigned reserved                     : 32;       // bits : 31_0
		} bit;
		UINT32 word;
	} reg_578; // 0x0908

	union {
		struct {
			unsigned reserved                     : 32;       // bits : 31_0
		} bit;
		UINT32 word;
	} reg_579; // 0x090c

	union {
		struct {
			unsigned dct_qtbl0_enc_p0        : 13;        // bits : 12_0
			unsigned                         : 3;
			unsigned dct_qtbl0_enc_p1        : 12;        // bits : 27_16
		} bit;
		UINT32 word;
	} reg_580; // 0x0910

	union {
		struct {
			unsigned dct_qtbl0_enc_p2        : 11;        // bits : 10_0
		} bit;
		UINT32 word;
	} reg_581; // 0x0914

	union {
		struct {
			unsigned dct_qtbl1_enc_p0        : 13;        // bits : 12_0
			unsigned                         : 3;
			unsigned dct_qtbl1_enc_p1        : 12;        // bits : 27_16
		} bit;
		UINT32 word;
	} reg_582; // 0x0918

	union {
		struct {
			unsigned dct_qtbl1_enc_p2        : 11;        // bits : 10_0
		} bit;
		UINT32 word;
	} reg_583; // 0x091c

	union {
		struct {
			unsigned dct_qtbl2_enc_p0        : 13;        // bits : 12_0
			unsigned                         : 3;
			unsigned dct_qtbl2_enc_p1        : 12;        // bits : 27_16
		} bit;
		UINT32 word;
	} reg_584; // 0x0920

	union {
		struct {
			unsigned dct_qtbl2_enc_p2        : 11;        // bits : 10_0
		} bit;
		UINT32 word;
	} reg_585; // 0x0924

	union {
		struct {
			unsigned dct_qtbl3_enc_p0        : 13;        // bits : 12_0
			unsigned                         : 3;
			unsigned dct_qtbl3_enc_p1        : 12;        // bits : 27_16
		} bit;
		UINT32 word;
	} reg_586; // 0x0928

	union {
		struct {
			unsigned dct_qtbl3_enc_p2        : 11;        // bits : 10_0
		} bit;
		UINT32 word;
	} reg_587; // 0x092c

	union {
		struct {
			unsigned dct_qtbl4_enc_p0        : 13;        // bits : 12_0
			unsigned                         : 3;
			unsigned dct_qtbl4_enc_p1        : 12;        // bits : 27_16
		} bit;
		UINT32 word;
	} reg_588; // 0x0930

	union {
		struct {
			unsigned dct_qtbl4_enc_p2        : 11;        // bits : 10_0
		} bit;
		UINT32 word;
	} reg_589; // 0x0934

	union {
		struct {
			unsigned dct_qtbl5_enc_p0        : 13;        // bits : 12_0
			unsigned                         : 3;
			unsigned dct_qtbl5_enc_p1        : 12;        // bits : 27_16
		} bit;
		UINT32 word;
	} reg_590; // 0x0938

	union {
		struct {
			unsigned dct_qtbl5_enc_p2        : 11;        // bits : 10_0
		} bit;
		UINT32 word;
	} reg_591; // 0x093c

	union {
		struct {
			unsigned dct_qtbl6_enc_p0        : 13;        // bits : 12_0
			unsigned                         : 3;
			unsigned dct_qtbl6_enc_p1        : 12;        // bits : 27_16
		} bit;
		UINT32 word;
	} reg_592; // 0x0940

	union {
		struct {
			unsigned dct_qtbl6_enc_p2        : 11;        // bits : 10_0
		} bit;
		UINT32 word;
	} reg_593; // 0x0944

	union {
		struct {
			unsigned dct_qtbl7_enc_p0        : 13;        // bits : 12_0
			unsigned                         : 3;
			unsigned dct_qtbl7_enc_p1        : 12;        // bits : 27_16
		} bit;
		UINT32 word;
	} reg_594; // 0x0948

	union {
		struct {
			unsigned dct_qtbl7_enc_p2        : 11;        // bits : 10_0
		} bit;
		UINT32 word;
	} reg_595; // 0x094c

	union {
		struct {
			unsigned dct_qtbl8_enc_p0        : 13;        // bits : 12_0
			unsigned                         : 3;
			unsigned dct_qtbl8_enc_p1        : 12;        // bits : 27_16
		} bit;
		UINT32 word;
	} reg_596; // 0x0950

	union {
		struct {
			unsigned dct_qtbl8_enc_p2        : 11;        // bits : 10_0
		} bit;
		UINT32 word;
	} reg_597; // 0x0954

	union {
		struct {
			unsigned dct_qtbl9_enc_p0        : 13;        // bits : 12_0
			unsigned                         : 3;
			unsigned dct_qtbl9_enc_p1        : 12;        // bits : 27_16
		} bit;
		UINT32 word;
	} reg_598; // 0x0958

	union {
		struct {
			unsigned dct_qtbl9_enc_p2        : 11;        // bits : 10_0
		} bit;
		UINT32 word;
	} reg_599; // 0x095c

	union {
		struct {
			unsigned dct_qtbl10_enc_p0        : 13;       // bits : 12_0
			unsigned                          : 3;
			unsigned dct_qtbl10_enc_p1        : 12;       // bits : 27_16
		} bit;
		UINT32 word;
	} reg_600; // 0x0960

	union {
		struct {
			unsigned dct_qtbl10_enc_p2        : 11;     // bits : 10_0
		} bit;
		UINT32 word;
	} reg_601; // 0x0964

	union {
		struct {
			unsigned dct_qtbl11_enc_p0        : 13;       // bits : 12_0
			unsigned                          : 3;
			unsigned dct_qtbl11_enc_p1        : 12;       // bits : 27_16
		} bit;
		UINT32 word;
	} reg_602; // 0x0968

	union {
		struct {
			unsigned dct_qtbl11_enc_p2        : 11;       // bits : 10_0
		} bit;
		UINT32 word;
	} reg_603; // 0x096c

	union {
		struct {
			unsigned dct_qtbl12_enc_p0        : 13;       // bits : 12_0
			unsigned                          : 3;
			unsigned dct_qtbl12_enc_p1        : 12;       // bits : 27_16
		} bit;
		UINT32 word;
	} reg_604; // 0x0970

	union {
		struct {
			unsigned dct_qtbl12_enc_p2        : 11;       // bits : 10_0
		} bit;
		UINT32 word;
	} reg_605; // 0x0974

	union {
		struct {
			unsigned dct_qtbl13_enc_p0        : 13;       // bits : 12_0
			unsigned                          : 3;
			unsigned dct_qtbl13_enc_p1        : 12;       // bits : 27_16
		} bit;
		UINT32 word;
	} reg_606; // 0x0978

	union {
		struct {
			unsigned dct_qtbl13_enc_p2        : 11;       // bits : 10_0
		} bit;
		UINT32 word;
	} reg_607; // 0x097c

	union {
		struct {
			unsigned dct_qtbl14_enc_p0        : 13;       // bits : 12_0
			unsigned                          : 3;
			unsigned dct_qtbl14_enc_p1        : 12;       // bits : 27_16
		} bit;
		UINT32 word;
	} reg_608; // 0x0980

	union {
		struct {
			unsigned dct_qtbl14_enc_p2        : 11;       // bits : 10_0
		} bit;
		UINT32 word;
	} reg_609; // 0x0984

	union {
		struct {
			unsigned dct_qtbl15_enc_p0        : 13;       // bits : 12_0
			unsigned                          : 3;
			unsigned dct_qtbl15_enc_p1        : 12;       // bits : 27_16
		} bit;
		UINT32 word;
	} reg_610; // 0x0988

	union {
		struct {
			unsigned dct_qtbl15_enc_p2        : 11;       // bits : 10_0
		} bit;
		UINT32 word;
	} reg_611; // 0x098c

	union {
		struct {
			unsigned dct_qtbl0_dcmax        : 9;      // bits : 8_0
			unsigned                        : 7;
			unsigned dct_qtbl1_dcmax        : 9;      // bits : 24_16
		} bit;
		UINT32 word;
	} reg_612; // 0x0990

	union {
		struct {
			unsigned dct_qtbl2_dcmax        : 9;      // bits : 8_0
			unsigned                        : 7;
			unsigned dct_qtbl3_dcmax        : 9;      // bits : 24_16
		} bit;
		UINT32 word;
	} reg_613; // 0x0994

	union {
		struct {
			unsigned dct_qtbl4_dcmax        : 9;      // bits : 8_0
			unsigned                        : 7;
			unsigned dct_qtbl5_dcmax        : 9;      // bits : 24_16
		} bit;
		UINT32 word;
	} reg_614; // 0x0998

	union {
		struct {
			unsigned dct_qtbl6_dcmax        : 9;      // bits : 8_0
			unsigned                        : 7;
			unsigned dct_qtbl7_dcmax        : 9;      // bits : 24_16
		} bit;
		UINT32 word;
	} reg_615; // 0x099c

	union {
		struct {
			unsigned dct_qtbl8_dcmax        : 9;      // bits : 8_0
			unsigned                        : 7;
			unsigned dct_qtbl9_dcmax        : 9;      // bits : 24_16
		} bit;
		UINT32 word;
	} reg_616; // 0x09a0

	union {
		struct {
			unsigned dct_qtbl10_dcmax        : 9;     // bits : 8_0
			unsigned                         : 7;
			unsigned dct_qtbl11_dcmax        : 9;     // bits : 24_16
		} bit;
		UINT32 word;
	} reg_617; // 0x09a4

	union {
		struct {
			unsigned dct_qtbl12_dcmax        : 9;     // bits : 8_0
			unsigned                         : 7;
			unsigned dct_qtbl13_dcmax        : 9;     // bits : 24_16
		} bit;
		UINT32 word;
	} reg_618; // 0x09a8

	union {
		struct {
			unsigned dct_qtbl14_dcmax        : 9;     // bits : 8_0
			unsigned                         : 7;
			unsigned dct_qtbl15_dcmax        : 9;     // bits : 24_16
		} bit;
		UINT32 word;
	} reg_619; // 0x09ac

	union {
		struct {
			unsigned yrc_lncnt_lfn0        : 13;      // bits : 12_0
			unsigned                       : 3;
			unsigned yrc_lncnt_lfn1        : 12;      // bits : 27_16
		} bit;
		UINT32 word;
	} reg_620; // 0x09b0

	union {
		struct {
			unsigned yrc_lncnt_lfn2        : 13;      // bits : 12_0
			unsigned                       : 3;
			unsigned yrc_lncnt_lfn3        : 12;      // bits : 27_16
		} bit;
		UINT32 word;
	} reg_621; // 0x09b4

	union {
		struct {
			unsigned yrc_lncnt_lfn4        : 13;      // bits : 12_0
			unsigned                       : 3;
			unsigned yrc_lncnt_lfn5        : 12;      // bits : 27_16
		} bit;
		UINT32 word;
	} reg_622; // 0x09b8

	union {
		struct {
			unsigned dct_maxdist        : 8;      // bits : 7_0
		} bit;
		UINT32 word;
	} reg_623; // 0x09bc

	union {
		struct {
			unsigned dct_qtbl0_dec_p0        : 10;        // bits : 9_0
			unsigned                         : 6;
			unsigned dct_qtbl0_dec_p1        : 10;        // bits : 25_16
		} bit;
		UINT32 word;
	} reg_624; // 0x09c0

	union {
		struct {
			unsigned dct_qtbl0_dec_p2        : 10;        // bits : 9_0
		} bit;
		UINT32 word;
	} reg_625; // 0x09c4

	union {
		struct {
			unsigned dct_qtbl1_dec_p0        : 10;        // bits : 9_0
			unsigned                         : 6;
			unsigned dct_qtbl1_dec_p1        : 10;        // bits : 25_16
		} bit;
		UINT32 word;
	} reg_626; // 0x09c8

	union {
		struct {
			unsigned dct_qtbl1_dec_p2        : 10;        // bits : 9_0
		} bit;
		UINT32 word;
	} reg_627; // 0x09cc

	union {
		struct {
			unsigned dct_qtbl2_dec_p0        : 10;        // bits : 9_0
			unsigned                         : 6;
			unsigned dct_qtbl2_dec_p1        : 10;        // bits : 25_16
		} bit;
		UINT32 word;
	} reg_628; // 0x09d0

	union {
		struct {
			unsigned dct_qtbl2_dec_p2        : 10;        // bits : 9_0
		} bit;
		UINT32 word;
	} reg_629; // 0x09d4

	union {
		struct {
			unsigned dct_qtbl3_dec_p0        : 10;        // bits : 9_0
			unsigned                         : 6;
			unsigned dct_qtbl3_dec_p1        : 10;        // bits : 25_16
		} bit;
		UINT32 word;
	} reg_630; // 0x09d8

	union {
		struct {
			unsigned dct_qtbl3_dec_p2        : 10;        // bits : 9_0
		} bit;
		UINT32 word;
	} reg_631; // 0x09dc

	union {
		struct {
			unsigned dct_qtbl4_dec_p0        : 10;        // bits : 9_0
			unsigned                         : 6;
			unsigned dct_qtbl4_dec_p1        : 10;        // bits : 25_16
		} bit;
		UINT32 word;
	} reg_632; // 0x09e0

	union {
		struct {
			unsigned dct_qtbl4_dec_p2        : 10;        // bits : 9_0
		} bit;
		UINT32 word;
	} reg_633; // 0x09e4

	union {
		struct {
			unsigned dct_qtbl5_dec_p0        : 10;        // bits : 9_0
			unsigned                         : 6;
			unsigned dct_qtbl5_dec_p1        : 10;        // bits : 25_16
		} bit;
		UINT32 word;
	} reg_634; // 0x09e8

	union {
		struct {
			unsigned dct_qtbl5_dec_p2        : 10;        // bits : 9_0
		} bit;
		UINT32 word;
	} reg_635; // 0x09ec

	union {
		struct {
			unsigned dct_qtbl6_dec_p0        : 10;        // bits : 9_0
			unsigned                         : 6;
			unsigned dct_qtbl6_dec_p1        : 10;        // bits : 25_16
		} bit;
		UINT32 word;
	} reg_636; // 0x09f0

	union {
		struct {
			unsigned dct_qtbl6_dec_p2        : 10;        // bits : 9_0
		} bit;
		UINT32 word;
	} reg_637; // 0x09f4

	union {
		struct {
			unsigned dct_qtbl7_dec_p0        : 10;        // bits : 9_0
			unsigned                         : 6;
			unsigned dct_qtbl7_dec_p1        : 10;        // bits : 25_16
		} bit;
		UINT32 word;
	} reg_638; // 0x09f8

	union {
		struct {
			unsigned dct_qtbl7_dec_p2        : 10;        // bits : 9_0
		} bit;
		UINT32 word;
	} reg_639; // 0x09fc

	union {
		struct {
			unsigned dct_qtbl8_dec_p0        : 10;        // bits : 9_0
			unsigned                         : 6;
			unsigned dct_qtbl8_dec_p1        : 10;        // bits : 25_16
		} bit;
		UINT32 word;
	} reg_640; // 0x0a00

	union {
		struct {
			unsigned dct_qtbl8_dec_p2        : 10;        // bits : 9_0
		} bit;
		UINT32 word;
	} reg_641; // 0x0a04

	union {
		struct {
			unsigned dct_qtbl9_dec_p0        : 10;        // bits : 9_0
			unsigned                         : 6;
			unsigned dct_qtbl9_dec_p1        : 10;        // bits : 25_16
		} bit;
		UINT32 word;
	} reg_642; // 0x0a08

	union {
		struct {
			unsigned dct_qtbl9_dec_p2        : 10;        // bits : 9_0
		} bit;
		UINT32 word;
	} reg_643; // 0x0a0c

	union {
		struct {
			unsigned dct_qtbl10_dec_p0        : 10;       // bits : 9_0
			unsigned                          : 6;
			unsigned dct_qtbl10_dec_p1        : 10;       // bits : 25_16
		} bit;
		UINT32 word;
	} reg_644; // 0x0a10

	union {
		struct {
			unsigned dct_qtbl10_dec_p2        : 10;       // bits : 9_0
		} bit;
		UINT32 word;
	} reg_645; // 0x0a14

	union {
		struct {
			unsigned dct_qtbl11_dec_p0        : 10;       // bits : 9_0
			unsigned                          : 6;
			unsigned dct_qtbl11_dec_p1        : 10;       // bits : 25_16
		} bit;
		UINT32 word;
	} reg_646; // 0x0a18

	union {
		struct {
			unsigned dct_qtbl11_dec_p2        : 10;       // bits : 9_0
		} bit;
		UINT32 word;
	} reg_647; // 0x0a1c

	union {
		struct {
			unsigned dct_qtbl12_dec_p0        : 10;       // bits : 9_0
			unsigned                          : 6;
			unsigned dct_qtbl12_dec_p1        : 10;       // bits : 25_16
		} bit;
		UINT32 word;
	} reg_648; // 0x0a20

	union {
		struct {
			unsigned dct_qtbl12_dec_p2        : 10;       // bits : 9_0
		} bit;
		UINT32 word;
	} reg_649; // 0x0a24

	union {
		struct {
			unsigned dct_qtbl13_dec_p0        : 10;       // bits : 9_0
			unsigned                          : 6;
			unsigned dct_qtbl13_dec_p1        : 10;       // bits : 25_16
		} bit;
		UINT32 word;
	} reg_650; // 0x0a28

	union {
		struct {
			unsigned dct_qtbl13_dec_p2        : 10;       // bits : 9_0
		} bit;
		UINT32 word;
	} reg_651; // 0x0a2c

	union {
		struct {
			unsigned dct_qtbl14_dec_p0        : 10;       // bits : 9_0
			unsigned                          : 6;
			unsigned dct_qtbl14_dec_p1        : 10;       // bits : 25_16
		} bit;
		UINT32 word;
	} reg_652; // 0x0a30

	union {
		struct {
			unsigned dct_qtbl14_dec_p2        : 10;       // bits : 9_0
		} bit;
		UINT32 word;
	} reg_653; // 0x0a34

	union {
		struct {
			unsigned dct_qtbl15_dec_p0        : 10;       // bits : 9_0
			unsigned                          : 6;
			unsigned dct_qtbl15_dec_p1        : 10;       // bits : 25_16
		} bit;
		UINT32 word;
	} reg_654; // 0x0a38

	union {
		struct {
			unsigned dct_qtbl15_dec_p2        : 10;       // bits : 9_0
		} bit;
		UINT32 word;
	} reg_655; // 0x0a3c

	union {
		struct {
			unsigned dct_level_th0        : 8;      // bits : 7_0
			unsigned dct_level_th1        : 8;      // bits : 15_8
			unsigned dct_level_th2        : 8;      // bits : 23_16
			unsigned dct_level_th3        : 8;      // bits : 31_24
		} bit;
		UINT32 word;
	} reg_656; // 0x0a40

	union {
		struct {
			unsigned dct_level_th4        : 8;      // bits : 7_0
			unsigned dct_level_th5        : 8;      // bits : 15_8
			unsigned dct_level_th6        : 8;      // bits : 23_16
			unsigned dct_level_th7        : 8;      // bits : 31_24
		} bit;
		UINT32 word;
	} reg_657; // 0x0a44

	union {
		struct {
			unsigned reserved                       : 32;       // bits : 31_0
		} bit;
		UINT32 word;
	} reg_658; // 0x0a48

	union {
		struct {
			unsigned reserved                       : 32;       // bits : 31_0
		} bit;
		UINT32 word;
	} reg_659; // 0x0a4c

	union {
		struct {
			unsigned reserved                       : 32;       // bits : 31_0
		} bit;
		UINT32 word;
	} reg_660; // 0x0a50

	union {
		struct {
			unsigned reserved                       : 32;       // bits : 31_0
		} bit;
		UINT32 word;
	} reg_661; // 0x0a54

	union {
		struct {
			unsigned reserved                       : 32;       // bits : 31_0
		} bit;
		UINT32 word;
	} reg_662; // 0x0a58

	union {
		struct {
			unsigned reserved                       : 32;       // bits : 31_0
		} bit;
		UINT32 word;
	} reg_663; // 0x0a5c

	union {
		struct {
			unsigned ime_lcaf_filter_en         : 1;        // bits : 0
			unsigned                            : 5;
			unsigned ime_lcaf_yftr_en           : 1;        // bits : 6
			unsigned                            : 9;
			unsigned ime_lcaf_fsize_sel         : 2;        // bits : 17_16
			unsigned                            : 2;
			unsigned ime_lcaf_eksize_sel        : 2;        // bits : 21_20
		} bit;
		UINT32 word;
	} reg_664; // 0x0a60

	union {
		struct {
			unsigned reserved                       : 32;       // bits : 31_0
		} bit;
		UINT32 word;
	} reg_665; // 0x0a64

	union {
		struct {
			unsigned ime_lcaf_y_rcth0        : 8;       // bits : 7_0
			unsigned ime_lcaf_y_rcth1        : 8;       // bits : 15_8
			unsigned ime_lcaf_y_rcth2        : 8;       // bits : 23_16
			unsigned ime_lcaf_y_cwt          : 5;       // bits : 28_24
		} bit;
		UINT32 word;
	} reg_666; // 0x0a68

	union {
		struct {
			unsigned ime_lcaf_y_rcwt0          : 4;     // bits : 3_0
			unsigned ime_lcaf_y_rcwt1          : 4;     // bits : 7_4
			unsigned ime_lcaf_y_rcwt2          : 4;     // bits : 11_8
			unsigned ime_lcaf_y_rcwt3          : 4;     // bits : 15_12
			unsigned ime_lcaf_y_outl_th        : 3;     // bits : 18_16
		} bit;
		UINT32 word;
	} reg_667; // 0x0a6c

	union {
		struct {
			unsigned ime_lcaf_uv_rcth0        : 8;      // bits : 7_0
			unsigned ime_lcaf_uv_rcth1        : 8;      // bits : 15_8
			unsigned ime_lcaf_uv_rcth2        : 8;      // bits : 23_16
			unsigned ime_lcaf_uv_cwt          : 5;      // bits : 28_24
		} bit;
		UINT32 word;
	} reg_668; // 0x0a70

	union {
		struct {
			unsigned ime_lcaf_uv_rcwt0          : 4;        // bits : 3_0
			unsigned ime_lcaf_uv_rcwt1          : 4;        // bits : 7_4
			unsigned ime_lcaf_uv_rcwt2          : 4;        // bits : 11_8
			unsigned ime_lcaf_uv_rcwt3          : 4;        // bits : 15_12
			unsigned ime_lcaf_uv_outl_th        : 3;        // bits : 18_16
		} bit;
		UINT32 word;
	} reg_669; // 0x0a74

	union {
		struct {
			unsigned ime_lcaf_y_outl_dth        : 8;        // bits : 7_0
			unsigned ime_lcaf_u_outl_dth        : 8;        // bits : 15_8
			unsigned ime_lcaf_v_outl_dth        : 8;        // bits : 23_16
		} bit;
		UINT32 word;
	} reg_670; // 0x0a78

	union {
		struct {
			unsigned ime_lcaf_ed_pn_th        : 8;      // bits : 7_0
			unsigned ime_lcaf_ed_hv_th        : 8;      // bits : 15_8
		} bit;
		UINT32 word;
	} reg_671; // 0x0a7c

	union {
		struct {
			unsigned ime_lcaf_y_fth0        : 8;        // bits : 7_0
			unsigned ime_lcaf_y_fth1        : 8;        // bits : 15_8
			unsigned ime_lcaf_y_fth2        : 8;        // bits : 23_16
			unsigned ime_lcaf_y_fth3        : 8;        // bits : 31_24
		} bit;
		UINT32 word;
	} reg_672; // 0x0a80

	union {
		struct {
			unsigned ime_lcaf_y_fth4        : 8;        // bits : 7_0
		} bit;
		UINT32 word;
	} reg_673; // 0x0a84

	union {
		struct {
			unsigned ime_lcaf_y_fwt0        : 5;        // bits : 4_0
			unsigned ime_lcaf_y_fwt1        : 5;        // bits : 9_5
			unsigned ime_lcaf_y_fwt2        : 5;        // bits : 14_10
			unsigned ime_lcaf_y_fwt3        : 5;        // bits : 19_15
			unsigned ime_lcaf_y_fwt4        : 5;        // bits : 24_20
			unsigned ime_lcaf_y_fwt5        : 5;        // bits : 29_25
		} bit;
		UINT32 word;
	} reg_674; // 0x0a88

	union {
		struct {
			unsigned ime_lcaf_u_fth0        : 8;        // bits : 7_0
			unsigned ime_lcaf_u_fth1        : 8;        // bits : 15_8
			unsigned ime_lcaf_u_fth2        : 8;        // bits : 23_16
			unsigned ime_lcaf_u_fth3        : 8;        // bits : 31_24
		} bit;
		UINT32 word;
	} reg_675; // 0x0a8c

	union {
		struct {
			unsigned ime_lcaf_u_fth4        : 8;        // bits : 7_0
		} bit;
		UINT32 word;
	} reg_676; // 0x0a90

	union {
		struct {
			unsigned ime_lcaf_u_fwt0        : 5;        // bits : 4_0
			unsigned ime_lcaf_u_fwt1        : 5;        // bits : 9_5
			unsigned ime_lcaf_u_fwt2        : 5;        // bits : 14_10
			unsigned ime_lcaf_u_fwt3        : 5;        // bits : 19_15
			unsigned ime_lcaf_u_fwt4        : 5;        // bits : 24_20
			unsigned ime_lcaf_u_fwt5        : 5;        // bits : 29_25
		} bit;
		UINT32 word;
	} reg_677; // 0x0a94

	union {
		struct {
			unsigned ime_lcaf_v_fth0        : 8;        // bits : 7_0
			unsigned ime_lcaf_v_fth1        : 8;        // bits : 15_8
			unsigned ime_lcaf_v_fth2        : 8;        // bits : 23_16
			unsigned ime_lcaf_v_fth3        : 8;        // bits : 31_24
		} bit;
		UINT32 word;
	} reg_678; // 0x0a98

	union {
		struct {
			unsigned ime_lcaf_v_fth4        : 8;        // bits : 7_0
		} bit;
		UINT32 word;
	} reg_679; // 0x0a9c

	union {
		struct {
			unsigned ime_lcaf_v_fwt0        : 5;        // bits : 4_0
			unsigned ime_lcaf_v_fwt1        : 5;        // bits : 9_5
			unsigned ime_lcaf_v_fwt2        : 5;        // bits : 14_10
			unsigned ime_lcaf_v_fwt3        : 5;        // bits : 19_15
			unsigned ime_lcaf_v_fwt4        : 5;        // bits : 24_20
			unsigned ime_lcaf_v_fwt5        : 5;        // bits : 29_25
		} bit;
		UINT32 word;
	} reg_680; // 0x0aa0

	union {
		struct {
			unsigned ime_lcaf_stl_u_th0        : 8;     // bits : 7_0
			unsigned ime_lcaf_stl_u_th1        : 8;     // bits : 15_8
		} bit;
		UINT32 word;
	} reg_681; // 0x0aa4

	union {
		struct {
			unsigned ime_lcaf_stl_u_sum0        : 19;       // bits : 18_0
		} bit;
		UINT32 word;
	} reg_682; // 0x0aa8

	union {
		struct {
			unsigned ime_lcaf_stl_u_sum1        : 18;       // bits : 17_0
		} bit;
		UINT32 word;
	} reg_683; // 0x0aac

	union {
		struct {
			unsigned ime_lcaf_stl_v_th0        : 8;     // bits : 7_0
			unsigned ime_lcaf_stl_v_th1        : 8;     // bits : 15_8
		} bit;
		UINT32 word;
	} reg_684; // 0x0ab0

	union {
		struct {
			unsigned ime_lcaf_stl_v_sum0        : 19;       // bits : 18_0
		} bit;
		UINT32 word;
	} reg_685; // 0x0ab4

	union {
		struct {
			unsigned ime_lcaf_stl_v_sum1        : 18;       // bits : 17_0
		} bit;
		UINT32 word;
	} reg_686; // 0x0ab8

	union {
		struct {
			unsigned ime_lcaf_stl_cnt        : 28;      // bits : 27_0
		} bit;
		UINT32 word;
	} reg_687; // 0x0abc

} NT98560_IME_REGISTER_STRUCT;

#endif


#ifdef __cplusplus
}
#endif


#endif


