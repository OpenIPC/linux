
#ifndef _H265_ENC_H_
#define _H265_ENC_H_

#define BIT_BUFFER_SIZE 32

typedef struct
{
    unsigned char Rdbfr[BIT_BUFFER_SIZE];
    unsigned char *Rdptr;
    unsigned int Bfr;
    unsigned char *Rdmax;
    int Incnt;
    int Bitcnt;
} base;

typedef struct
{
  unsigned char forbidden_zero_bit;
  unsigned char nal_unit_type;
  unsigned char nuh_layer_id;
  unsigned char nuh_temporal_id_plus1;
} NAL_Header;

typedef struct bs_t
{
    int     bits_pos;
    int     bits_len;
    int     left_cnt;       //left bits of current byte
    uint    code_bit;
    uchar*  curr;           //current bit of seqh
    uchar*  buff;           //start address of seqh
    int     zero_cnt;
} bs_t;

typedef enum HEVC_PSET_TYPE
{
    HEVC_SET =-1,
    HEVC_VPS = 0,
    HEVC_SPS,
    HEVC_PPS,
    HEVC_SLICE,
    HEVC_RPS,
    HEVC_PIC,
} HEVC_PSET_TYPE;

typedef enum HEVC_REF_LIST
{
    HEVC_REF_ACTIVE_L0  = 0,
    HEVC_REF_ACTIVE_L1  = 1,
    HEVC_REF_LTR        = 2,
    HEVC_REF_MAX        = 3,
} HEVC_REF_LIST;

typedef enum NAL_TYPE
{
    TRAIL_N         = 0,// 0
    TRAIL_R,            // 1
    TSA_N,              // 2
    TSA_R,              // 3
    STSA_N,             // 4
    STSA_R,             // 5
    RADL_N,             // 6
    RADL_R,             // 7
    RASL_N,             // 8
    RASL_R,             // 9
    RSV_VCL_N10,
    RSV_VCL_R11,
    RSV_VCL_N12,
    RSV_VCL_R13,
    RSV_VCL_N14,
    RSV_VCL_R15,
    BLA_W_LP        =16,
    BLA_W_RADL      =17,
    BLA_N_LP        =18,
    IDR_W_RADL      =19,
    IDR_N_LP        =20,
    CRA_NUT         =21,
    RSV_IRAP_VCL22  =22,
    RSV_IRAP_VCL23  =23,
    VPS_NUT         =32,
    SPS_NUT         =33,
    PPS_NUT         =34,
    EOS_NUT         =36,
    EOB_NUT         =37,
    FD_NUT          =38,
    PREFIX_SEI_NUT  =39,
    SUFFIX_SEI_NUT  =40,
    RSV_NVCL41      =41,
    UNSPEC48        =48,
    /* Reference picture sets uses same store than vps/sps/vps */
    RPS             =64
} NAL_TYPE;

/* hevc spec: Nal Unit Types */
#define NUT_VPS     32
#define NUT_SPS     33
#define NUT_PPS     34
#define NUH_PACK(typ,lid,tid)   ((((typ)&63)<<9)+(((lid)&63)<<3)+((tid)&7))
/* hevc spec: Profile */
typedef enum HEVC_PROFILE_IDC
{
    HEVC_PROFILE_NONE               = 0,
    HEVC_PROFILE_MAIN               = 1,
    HEVC_PROFILE_MAIN10             = 2,
    HEVC_PROFILE_MAINSTILLPICTURE   = 3,
    HEVC_PROFILE_MAINREXT           = 4,
    HEVC_PROFILE_HIGHTHROUGHPUTREXT = 5,
} HEVC_PROFILE_IDC;
/* hevc spec: Levels */
typedef enum HEVC_LEVEL_IDC
{
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
typedef struct slice_t slice_t;
typedef struct pic_t pic_t;

/* profile_tier_level */
struct ptl_t
{
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
struct vui_t
{
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

#define MAX_DEC_PIC_BUFFERING_DEFAULT   2
/* video_parameter_set */
struct vps_t
{
    int     i_id;
    int     b_sub_layers_ordering_pres;
    int     b_temporal_id_nesting;
    int     i_max_num_sub_layers;
    int     i_max_dec_pic_buffering;
    int     i_max_num_reorder_pics;
    int     i_max_latency_increase;
    int     b_timing_info_pres;
    int     i_num_units_in_tick;
    int     i_time_scale;
    /* profile tier level */
    ptl_t   ptl;
};

#define MAX_RPS_SIZE    8
#define MAX_REF_SIZE    4
#define MAX_RPB_SIZE    4

/* reference picture set */
struct rps_t
{
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
struct sps_t
{
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
struct pps_t
{
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

struct slice_t
{
    sps_t*  p_sps;
    pps_t*  p_pps;
    int     i_nal_type;

    unsigned char first_slice_segment_in_pic_flag;
    unsigned char no_output_of_prior_pics_flag;
    unsigned int slice_pic_parameter_set_id;
#define SLICE_TYPE_B    0
#define SLICE_TYPE_P    1
#define SLICE_TYPE_I    2
    int     i_slice_type;
    int     i_poc;
    unsigned char short_term_ref_pic_set_sps_flag;
    unsigned int short_term_ref_pic_set_idx;

    unsigned int num_long_term_pics;
    unsigned int poc_lsb_lt[1];
    unsigned char used_by_curr_pic_lt_flag[1];
    unsigned char delta_poc_msb_present_flag[1];

    unsigned char slice_temporal_mvp_enabled_flag;
    unsigned char slice_sao_luma_flag;
    unsigned char slice_sao_chroma_flag;
    unsigned char deblocking_filter_override_flag;
    unsigned char slice_deblocking_filter_disabled_flag;
    signed char slice_beta_offset_div2;
    signed char slice_tc_offset_div2;

    int     b_active_override;
    int     b_cabac_init;
    int     i_max_num_merge_cand; ///< 5 - 5_minus_max_num_merge_cand
    int     i_qp;
    unsigned char slice_loop_filter_across_slices_enabled_flag;
};

struct pic_t
{
    short   i_id;               // pic buffer id
#define POC_BUFFER_INVL     -2
#define POC_BUFFER_FREE     -1
#define IS_POC_INVL(s)      ((s)==POC_BUFFER_INVL)
#define IS_POC_FREE(s)      ((s)==POC_BUFFER_FREE)
    int     i_poc;              // poc, -1: available used, -2: not available(not initialize)
    /* picture type */
#define HEVC_ISLICE         0
#define HEVC_PSLICE         1
#define HEVC_LTRPSLICE      2
    char    i_type;             // HEVC_ISLICE/HEVC_PSLICE/HEVC_LTRPSLICE
    char    b_reference;
    char    b_ltr_keep;         // LTR mode buffer keep
    char    b_cached;           // shrink mode: identify picture is cached
    sps_t*  p_sps;
    pps_t*  p_pps;
    rps_t*  p_rps;
};

typedef struct h265_enc
{
    void    (*release) (void*);
    /* prepare for real used */
    vps_t   m_vps;
    sps_t   m_sps;
    pps_t   m_pps;
    slice_t m_sh;
    /* sequence layer params (prepare for hw) */
    int     i_profile;
    int     i_level;
    int     i_picw;
    int     i_pich;
    int     i_cb_w;
    int     i_cb_h;
    int     b_interlaced;
    int     i_num_short_term_ref_pic_sets;
    /* picture layer params (prepare for hw) */
    int     b_refp;
    int     i_picq;                             // encode QP
    int     i_poc;                              // POC
    int     i_num_ref_idx_l0_default_active;
    /* slice params (prepare for hw) */
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
    pic_t*  p_rpls[HEVC_REF_MAX][MAX_REF_SIZE]; // reference pic. list
    /* buffer information */
    int     i_rpbn;                             // initialized reconstruct buffer number
    pic_t   m_pics[MAX_RPB_SIZE];               // recn picture relate to ops m_rpbs[]
    pic_t*  p_recn;                             // current recn buffer
    /* nalu information */
    int     i_seqh;                             // VPS+SPS+PPS size length in byte
    char    m_seqh[128];                        // used for SPS/VPS/PPS
#define     SLICE_HEADER_BIAS_SIZE      32      // used for LTR sw header rewrite
#define     SLICE_HEADER_PARSE_SIZE     SLICE_HEADER_BIAS_SIZE
#define     START_CODE_LEN     4
    int     i_swsh;                             // sw enc slice header size
    char    m_swsh[SLICE_HEADER_BIAS_SIZE];     // sw enc slice header
    int     i_hwsh;                             // hw enc slice header size
    char    m_hwsh[SLICE_HEADER_PARSE_SIZE];    // hw enc slice header
    /* LTR */
    int     b_enable_pred;                      // 0: P ref. I-frame, 1: P ref P-frame
} h265_enc;

#define HEVC_RPS_PRESET 1

h265_enc* h265enc_acquire(void);

/* find reference frame */
void* h265_find_rpl(h265_enc*, int, int);
/* find SPS/PPS/VPS/Slice */
void* h265_find_set(h265_enc*, int, int);
/* sequence initialize */
int   h265_seq_init(h265_enc*, int);
/* sequence synchronous */
int   h265_seq_sync(h265_enc*);
/* sequence configure(write SPS/PPS/VPS header) */
int   h265_seq_conf(h265_enc*);
/* NULL function */
int   h265_seq_done(h265_enc*);
/* find recn buffer idx */
int   h265_enc_buff(h265_enc*, int, int);
/* return available can be free buffer */
int   h265_enc_done(h265_enc*);

/* slice header parser */
int   h265_sh_parser(h265_enc*);
/* slice header writer */
int   h265_sh_writer(h265_enc*);
/* write an RSV_NVCL41 type slice */
int   h265_write_padding_head(void* buff, int len);

#endif/*_H265_ENC_H_*/
