
#ifndef _MFE_H264_H_
#define _MFE_H264_H_

typedef struct bitstream {
    int     bits_pos;
    int     length;
    int     left_count;
    uint    code_bits;
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

#define NAL_PRIO_DISPOSED   0
#define NAL_PRIO_LOW        1
#define NAL_PRIO_HIGH       2
#define NAL_PRIO_HIGHEST    3

typedef struct sps {
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
    struct {
        int i_left, i_right;
        int i_top, i_bottom;
    } crop;
    int b_vui_param_pres;
    struct vui {
    int b_video_signal_pres;
    int i_video_format;
    int b_video_full_range;
    int b_colour_desc_pres;
    int i_colour_primaries;
    int i_transf_character;
    int i_matrix_coeffs;
    } vui;
} sps_t;

typedef struct vui  vui_t;

typedef struct pps {
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

#define SLICE_P 0
#define SLICE_B 1
#define SLICE_I 2

typedef struct slice {
    int b_idr_pic;
    int i_ref_idc;
    sps_t*  sps;
    pps_t*  pps;
    int i_type;
    int i_first_mb;
    int i_frame_num;
    int i_idr_pic_id;
    int i_poc;
    int i_redundant_pic_cnt;
    int b_direct_spatial_mv_pred;
    int b_num_ref_idx_override;
    int i_num_ref_idx_l0_active;
    int i_num_ref_idx_l1_active;
    int i_cabac_init_idc;
    int i_qp;
    int i_disable_deblocking_filter_idc;
    int i_alpha_c0_offset_div2;
    int i_beta_offset_div2;
} slice_t;

int avc_write_nal(bs_t*, uchar );
int avc_write_sps(bs_t*, sps_t*);
int avc_write_pps(bs_t*, pps_t*);
int avc_write_slice_header(bs_t*, slice_t*);

#endif/*_MFE_H264_H_*/
