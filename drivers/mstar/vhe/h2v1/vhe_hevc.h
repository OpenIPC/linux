
#ifndef _VHE_HEVC_H_
#define _VHE_HEVC_H_

typedef struct bs_t {
    int     bits_pos;
    int     bits_len;
    int     left_cnt;
    uint    code_bit;
    uchar*  curr;
    uchar*  buff;
    int     zero_cnt;
} bs_t;

typedef enum HEVC_PSET_TYPE {
    HEVC_SET =-1,
    HEVC_VPS = 0,
    HEVC_SPS,
    HEVC_PPS,
    HEVC_RPS,
    HEVC_PIC,
} HEVC_PSET_TYPE;

/* hevc spec: Nal Unit Types */
#define NUT_VPS     32
#define NUT_SPS     33
#define NUT_PPS     34
#define NUH_PACK(typ,lid,tid)   ((((typ)&63)<<9)+(((lid)&63)<<3)+((tid)&7))
/* hevc spec: Profile */
typedef enum HEVC_PROFILE_IDC {
    HEVC_PROFILE_NONE               = 0,
    HEVC_PROFILE_MAIN               = 1,
    HEVC_PROFILE_MAIN10             = 2,
    HEVC_PROFILE_MAINSTILLPICTURE   = 3,
    HEVC_PROFILE_MAINREXT           = 4,
    HEVC_PROFILE_HIGHTHROUGHPUTREXT = 5,
} HEVC_PROFILE_IDC;
/* hevc spec: Levels */
typedef enum HEVC_LEVEL_IDC {
    HEVC_LEVEL_1    =  30,
    HEVC_LEVEL_2    =  60,
    HEVC_LEVEL_2_1  =  63,
    HEVC_LEVEL_3    =  90,
    HEVC_LEVEL_3_1  =  93,
    HEVC_LEVEL_4    = 120,
    HEVC_LEVEL_4_1  = 123,
    HEVC_LEVEL_5    = 150,
    HEVC_LEVEL_5_1  = 153,
    HEVC_LEVEL_5_2  = 156,
    HEVC_LEVEL_6    = 180,
    HEVC_LEVEL_6_1  = 183,
    HEVC_LEVEL_6_2  = 186,
} HEVC_LEVEL_IDC;
/* hevc spec: ctb */
#define MIN_CB_SIZE     8
#define MAX_CB_SIZE     64

typedef struct vps_t vps_t;
typedef struct rps_t rps_t;
typedef struct ptl_t ptl_t;
typedef struct vui_t vui_t;
typedef struct sps_t sps_t;
typedef struct pps_t pps_t;
typedef struct pic_t pic_t;

/* profile_tier_level */
struct ptl_t {
    int     i_profile;
    int     i_level;
    int     b_interlaced;
};

#define EXTENDED_SAR            255
/* max MV length is 1024 horizontal and 256 vertical in quarter pixel resolution */
#define LOG2_MAX_MV_LENGTH_HOR  10
#define LOG2_MAX_MV_LENGTH_VER  8
#define BPS_SHIFT               6
#define CPB_SHIFT               4
/* video usability information */
struct vui_t {
    int     i_sar_w, i_sar_h;
    int     b_video_signal_pres;
    int     i_video_format;
    int     b_video_full_range;
    int     b_colour_desc_pres;
    int     i_colour_primaries;
    int     i_transf_character;
    int     i_matrix_coeffs;
    int     b_field_seq;
    int     b_frame_field_info_pres;
    int     b_timing_info_pres;
    int     i_num_units_in_tick;
    int     i_time_scale;
    int     b_hrd_parameters_pres;
    int     b_nal_hrd_pres;
    int     i_bit_rate;
    int     i_cpb_size;
    int     i_init_cpb_removal_delay_len;
    int     i_au_cpb_removal_delay_len;
    int     i_dpb_output_delay_len;
    int     b_bitstream_restriction;
};

/* video_parameter_set */
struct vps_t {
    int     i_id;
    int     b_sub_layers_ordering_pres;
    int     b_temporal_id_nesting;
    int     i_max_num_sub_layers;
    int     i_max_dec_pic_buffering;
    int     i_max_num_reorder_pics;
    int     i_max_latency_increase;
    /* profile tier level */
    ptl_t   ptl;
};

#define MAX_RPS_SIZE    8
#define MAX_REF_SIZE    4
#define MAX_RPB_SIZE    4

/* reference picture set */
struct rps_t {
    short   i_id;
    short   i_num_neg_pics;
    short   i_num_pos_pics;
    short   i_cnt_before;
    short   i_cnt_after;
    short   i_cnt_foll;
    short   i_dpoc[MAX_RPS_SIZE];
    short   b_used[MAX_RPS_SIZE];
    short   i_pocs[MAX_RPS_SIZE];
};

/* sequence_parameter_set */
struct sps_t {
    int     i_id;
    vps_t*  p_vps;
    int     i_max_num_sub_layers;
    int     i_chroma_format_idc;
    int     i_pic_w_min_cbs;
    int     i_pic_h_min_cbs;
    int     b_conformance_window;
    int     i_crop_l, i_crop_r, i_crop_t, i_crop_b;
    int     i_log2_max_poc_lsb;
    int     b_sub_layers_ordering_pres;
    int     i_max_dec_pic_buffering;
    int     i_max_num_reorder_pics;
    int     i_max_latency_increase;
    int     i_log2_min_cb_size;
    int     i_log2_max_cb_size;
    int     i_log2_min_tr_size;
    int     i_log2_max_tr_size;
    int     i_max_tr_hierarchy_depth_inter;
    int     i_max_tr_hierarchy_depth_intra;
    int     b_scaling_list_enable;
    int     b_scaling_list_data_pres;
    int     b_amp_enabled;
    int     b_sao_enabled;
    int     b_pcm_enabled;
    int     i_num_short_term_ref_pic_sets;
    int     b_long_term_ref_pics_pres;
    int     b_temporal_mvp_enable;
    int     b_strong_intra_smoothing_enabled;
    int     b_vui_param_pres;
    /* video usability information */
    vui_t   vui;
    /* profile tier level */
    ptl_t   ptl;
    rps_t   rps[2];
};

/* picture_parameter_set */
struct pps_t {
    int     i_id;
    sps_t*  p_sps;
    int     b_cabac_init_pres;
    int     i_num_ref_idx_l0_default_active;
    int     i_num_ref_idx_l1_default_active;
    int     i_init_qp;
    int     b_constrained_intra_pred;
    int     b_transform_skip_enabled;
    int     b_cu_qp_delta_enabled;
    int     i_diff_cu_qp_delta_depth;
    int     i_cb_qp_offset;
    int     i_cr_qp_offset;
    int     b_slice_chroma_qp_offsets_pres;
    int     b_loop_filter_across_slices_enabled;
    int     b_deblocking_filter_control_pres;
    int     b_deblocking_filter_override_enabled;
    int     b_deblocking_filter_disabled;
    int     i_beta_offset;
    int     i_tc_offset;
    int     i_log2_parallel_merge_level;
};

struct pic_t {
    short   i_id;
    short   i_poc;
    short   i_type;
    short   b_reference;
    sps_t*  p_sps;
    pps_t*  p_pps;
    rps_t*  p_rps;
};

typedef struct hevc_enc {
    void    (*release) (void*);
    vps_t   m_vps;
    sps_t   m_sps;
    pps_t   m_pps;
    /* sequence layer params */
    int     i_profile;
    int     i_level;
    int     i_picw;
    int     i_pich;
    int     i_cb_w;
    int     i_cb_h;
    int     b_interlaced;
    int     i_rpbn;
    /* picture layer params */
    /* picture type */
  #define HEVC_ISLICE       0
  #define HEVC_PSLICE       1
    int     i_type;
    int     b_refp;
    int     i_picq;
    int     i_poc;
    /* slice params */
    /* slice split */
    int     i_rows;
    int     i_slices;
    /* slice syntax */
    int     b_cabac_init;
    int     b_deblocking_override;
    int     b_deblocking_disable;
    short   i_tc_offset;
    short   i_beta_offset;
    int     b_active_override;
    /* picture variables */
    int     i_active_l0_count;
    int     i_active_l1_count;
    pic_t*  p_rpls[2][MAX_REF_SIZE]; /* reference pic. list */
    pic_t*  p_recn;
    pic_t   m_rpbs[MAX_RPB_SIZE];
    int     i_seqh;
    char    m_seqh[128];
} hevc_enc;

#define HEVC_RPS_PRESET 1

hevc_enc* h265enc_acquire(void);

void* hevc_find_rpl(hevc_enc*, int, int);
void* hevc_find_set(hevc_enc*, int, int);
int   hevc_seq_init(hevc_enc*, int);
int   hevc_seq_conf(hevc_enc*);
int   hevc_seq_done(hevc_enc*);
int   hevc_enc_buff(hevc_enc*, int, int);
int   hevc_enc_done(hevc_enc*);

#endif//_VHE_HEVC_H_
