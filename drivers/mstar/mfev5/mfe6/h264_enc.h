
#ifndef _H264_ENC_H_
#define _H264_ENC_H_

typedef struct bitstream
{
    int     bits_pos;
    int     length;             //
    int     left_count;         // left count of 16bit unit
    uint    code_bits;          // encode bits
    uchar*  curr;
    uchar*  buffer;
    int     ebsp_zeros;
} bs_t;

int avc_reset(bs_t*,uchar*, int);
int avc_flush(bs_t*);
int avc_count(bs_t*);

#define NAL_SLICE           1
#define NAL_IDR             5
#define NAL_SPS             7
#define NAL_PPS             8
#define NAL_FILLER          12

typedef struct sps
{
    int i_sps_id;
    int i_profile_idc;
    int i_level_idc;
    int b_constraint_set0;
    int b_constraint_set1;
    int b_constraint_set2;
    int b_constraint_set3;
    int i_log2_max_frame_num;
    int i_poc_type;
    int i_log2_max_poc_lsb;
    int i_num_ref_frames;
    int b_gaps_in_frame_num_value_allow;
    int i_mb_w;
    int i_mb_h;
    int b_direct8x8_inference;
    int b_crop;
    struct
    {
        int i_left, i_right;
        int i_top, i_bottom;
    } crop;
    int b_vui_param_pres;
    struct vui
    {
        int b_video_signal_pres;
        int i_video_format;
        int b_video_full_range;
        int b_timing_info_pres;
        uint i_num_units_in_tick;
        uint i_time_scale;
        int b_fixed_frame_rate;
        int b_colour_desc_pres;
        int i_colour_primaries;
        int i_transf_character;
        int i_matrix_coeffs;
    } vui;
} sps_t;

typedef struct vui  vui_t;

typedef struct pps
{
    sps_t*  sps;
    int i_pps_id;
    int b_cabac;
    /*only support slice_groups==1*/
    int i_num_ref_idx_l0_default_active;
    int i_num_ref_idx_l1_default_active;
    /*not support weighted pred*/
    int i_pic_init_qp;
    int i_pic_init_qs;
    int i_cqp_idx_offset;
    int b_deblocking_filter_control;
    int b_constrained_intra_pred;
    int b_redundant_pic_cnt;
} pps_t;

typedef struct ref_pic_list_t
{
    int modification_of_pic_nums_idc;
    union
    {
        int abs_diff_pic_num_minus1;
        int long_term_pic_num;
        int abs_diff_view_idx_minus1;
    };
} ref_pic_list_t;

typedef struct mem_mng_ctl_t
{
    int memory_management_control;
    int difference_of_pic_nums_minus1;
    int long_term_pic_num;
    int long_term_frame_idx;
    int max_long_term_frame_idx_plus1;
} mem_mng_ctl_t;

typedef struct slice
{
    int b_idr_pic;
#define NAL_PRIO_DISPOSED   0
#define NAL_PRIO_LOW        1
#define NAL_PRIO_HIGH       2
#define NAL_PRIO_HIGHEST    3
    int i_ref_idc;
    sps_t*  sps;
    pps_t*  pps;
#define SLICE_P 0
#define SLICE_B 1
#define SLICE_I 2
    int i_type;
    int i_first_mb;
    int i_frm_num;
    int i_idr_pid;
    int b_long_term_reference;
    int i_poc;
    int i_redundant_pic_cnt;
    int b_direct_spatial_mv_pred;
    int b_num_ref_idx_override;
    int i_num_ref_idx_l0_active;
#define MAX_MULTI_REF_FRAME_PLUS1 3
    int b_ref_pic_list_modification_flag_l0;
    ref_pic_list_t rpl0_t[MAX_MULTI_REF_FRAME_PLUS1];
    int i_num_ref_idx_l1_active;
    int b_ref_pic_list_modification_flag_l1;
    ref_pic_list_t rpl1_t[MAX_MULTI_REF_FRAME_PLUS1]; // used in B-frame
    int i_cabac_init_idc;
    int i_qp;
    int i_disable_deblocking_filter_idc;
    int i_alpha_c0_offset_div2;
    int i_beta_offset_div2;
    int b_adaptive_ref_pic_marking_mode;
    mem_mng_ctl_t mmc_t[MAX_MULTI_REF_FRAME_PLUS1];
} slice_t;

int avc_write_nal(bs_t*, uchar );
int avc_write_sps(bs_t*, sps_t*);
int avc_write_pps(bs_t*, pps_t*);
int avc_write_slice_header(bs_t*, slice_t*);

#endif/*_H264_ENC_H_*/
