#ifndef _EMU_H264_ENC_H_
#define _EMU_H264_ENC_H_

//#include "kwrap/type.h"

// for hw trigger //
#include "h26x.h"

// for sw driver //
#include "h26x_def.h"
#include "h26xenc_api.h"
#include "h264enc_api.h"

// for emulation //
#include "emu_h26x_common.h"
#include "emu_h264_common.h"

#define cmp_smart_rec 0 //520 x
#define set_tmnr_apb 0 //520 x
#define cmp_sourceout_onebyone 0	//compare source_out.dat



//#if (EMU_H26X == ENABLE || AUTOTEST_H26X == ENABLE)



typedef struct _rdo_t_{
	unsigned char slope[3][4];

	unsigned char cost_bias_I4;
	unsigned char cost_bias_I8;
	unsigned char cost_bias_I16;
	unsigned char cost_bias_P4;
	unsigned char cost_bias_P8;
	unsigned char cost_bias_SKIP;
	unsigned char cost_bias_SKIP_bg;
	unsigned char cost_bias_shift;

	unsigned char I16_cost_tweak_V;
	unsigned char I16_cost_tweak_H;
	unsigned char I16_cost_tweak_DC;
	unsigned char I16_cost_tweak_PL;

	unsigned char ICm_cost_tweak_DC;
	unsigned char ICm_cost_tweak_H;
	unsigned char ICm_cost_tweak_V;
	unsigned char ICm_cost_tweak_PL;

	unsigned char luma_coeff_cost;
	unsigned char chroma_coeff_cost;
    unsigned char avc_rate_est;
} rdo_t;

typedef struct _var_t_{
	unsigned int  var_t;
	unsigned char avg_min;
	unsigned char avg_max;
	unsigned char delta;
	unsigned char i_range_delta;
	unsigned char p_range_delta;
} var_t;

typedef struct _fro_t_{
	unsigned char enable;
	unsigned int  dc[3][4];
	unsigned char ac[3][4];
	unsigned char st[3][4];
	unsigned char mx[3][4];
} fro_t;

typedef struct _mask_line_t_{
	unsigned int  coeffa;
	unsigned int  coeffb;
	unsigned int  coeffc;
	unsigned char comp;
} mask_line_t;

typedef struct _mask_win_t_{
	unsigned char enable;
	unsigned char did;
	unsigned char bitmap;
	unsigned char pal_sel;
	unsigned char line_hit_op;
	unsigned int  alpha;

	mask_line_t line[4];
} mask_win_t;

typedef struct _mask_t_{
	unsigned char enable;
	unsigned char bitmap_did;
	unsigned char mosaic_blkw;
	unsigned char mosaic_blkh;
	unsigned char pal_y[8];
	unsigned char pal_cb[8];
	unsigned char pal_cr[8];

	mask_win_t win[8];
} mask_t;

typedef struct _gdr_t_{
	unsigned char enable;
	unsigned int  period;
	unsigned int  number;
	unsigned char gdr_qp_en;
	unsigned char gdr_qp;
} gdr_t;

typedef struct _roi_win_t_{
	unsigned char enable;

	unsigned int left;
	unsigned int top;
	unsigned int width;
	unsigned int height;

	unsigned char qp;
	unsigned char qp_mode;
	unsigned char fro_mode;
	unsigned char lpm_mode;
	unsigned char bgr_mode;
	unsigned char maq_mode;
	unsigned char tnr_mode;
	unsigned char skip_label;
} roi_win_t;

typedef struct _roi_t_{
	roi_win_t win[10];
} roi_t;

typedef struct _rrc_seq_t_{
	unsigned char enable;

	unsigned char scale;
	unsigned char range;
	unsigned char step;
	unsigned char min_cost_th;
	unsigned char min_qp;
	unsigned char max_qp;

	unsigned int  chk_en;
	unsigned char zero_bit_mode;
	unsigned char ndqp;
	unsigned char ndqp_range;
	unsigned char ndqp_step;
    unsigned char rrc_mode; //0:original, 1: new
} rrc_seq_t;

typedef struct _rrc_pic_t_{
	unsigned char enable;
	unsigned char init_qp;
	unsigned char pred_wt;

	unsigned int  planned_stop;
	unsigned int  planned_top;
	unsigned int  planned_bot;
	unsigned int  frm_coeff;
	unsigned int  frm_cost_lsb;
	unsigned int  frm_cost_msb;
	unsigned int  frm_cmpx_lsb;
	unsigned int  frm_cmpx_msb;
    int beta;
    int th_0;
    int th_1;
    int th_2;
    int th_M;
    int th_3;
    int th_4;
    int mod_u_frm;
    int mod_d_Frm;
	int planned_top_init;
    long long i_all_ref_pred_tmpl_init;
    int target_bits_scale;
} rrc_pic_t;

typedef struct _sraq_t_{
	unsigned char enable;

	unsigned char ic2;
	unsigned char i_str;
	unsigned char p_str;
	unsigned char min_dqp;
	unsigned char max_dqp;

	unsigned char i_aslog2;
	unsigned char plane_x;
	unsigned char plane_y;
	unsigned int  th[H26X_AQ_TH_TBL_SIZE];
} sraq_t;

typedef struct _lpm_t_{
	unsigned char rmd_sad_en;
	unsigned char ime_stop_en;
	unsigned char ime_stop_th;
	unsigned char rdo_stop_en;
	unsigned char rdo_stop_th;
	unsigned char qp_map_defulat;
	unsigned char ira_i16_on;
	unsigned char ira_en;
	unsigned char ira_th;
} lpm_t;

typedef struct _rnd_t_{
	unsigned char enable;

	unsigned int  seed;
	unsigned char range;
} rnd_t;

typedef struct _scd_t_{
	unsigned int  th;
	unsigned char sc;
	unsigned char override_rrc;
} scd_t;

typedef struct _tmnr_luma_t_{
	unsigned char enable;

	unsigned char avoid_residue_th;
	unsigned char nr_str_3d;
	unsigned char nr_str_2d;
	unsigned char small_vibrat_supp_en;
	unsigned char center_wzero_2d_en;
	unsigned char center_wzero_3d_en;
	unsigned char blur_str;
	unsigned char max_blk_edgvar;
	unsigned char lut_2d_th[4];
	unsigned char lut_3d_th[2][4];
	unsigned char motion_level_th[2][2];
	unsigned char motion_2x2_to8x8_th[2];

	unsigned int base[8];
	unsigned int std[8];
	unsigned int coefa[8];
	unsigned int coefb[8];
	unsigned int line_offset;
} tmnr_luma_t;

typedef struct _tmnr_chrm_t_{
	unsigned char enable;

	unsigned char avoid_residue_th;
	unsigned char nr_str_3d;
	unsigned char nr_str_2d;
	unsigned char lut_2d_th[4];
	unsigned char lut_3d_th[4];
	unsigned char motion_level_th[2];

	unsigned int mean[2][8];
	unsigned int std[2][8];
	unsigned int line_offset;
} tmnr_chrm_t;

typedef struct _tmnr_t_{
	unsigned char enable;

	unsigned char ne_blk_sample_step;
	unsigned char ref_w;
	unsigned char sta_w;
	unsigned char display_motion_map_en;
	unsigned char display_motion_map_mode;
	unsigned char out_3d_en;
	unsigned char out_2d_en;
	unsigned char mt_en[2];
	unsigned char fcs_en;
	unsigned char fcs_str;
	unsigned char fcs_th;
	unsigned char mby_step;
	unsigned char mbx_step;
	unsigned char lcuy_step;
	unsigned char lcux_step;

	unsigned int info_x_num;
	unsigned int info_x_oft;
	unsigned int info_y_num;
	unsigned int info_y_oft;

	tmnr_luma_t luma;
	tmnr_chrm_t chrm;
    unsigned char refr_1d;              //0~1,  default=0
    unsigned char refw_1d;              //0~1,  default=0
    unsigned char m_sce;                //0~1,  default=0

    unsigned char ref_as_zero;              //bool  new in 321.
    unsigned char status_ctrl;             // U2   new in 321.
    unsigned char err_compensate;          //bool  new in 321.
} tmnr_t;

typedef struct _osg_graph_t_{
	unsigned char type;
	unsigned int  width ;
	unsigned int  height;
	unsigned int lofs;
	unsigned int  uv_type;
} osg_graph_t;

typedef struct _osg_disp_t_{
	unsigned char mode;
	unsigned int  x_str;
	unsigned int  y_str;
	unsigned char bg_alpha;
	unsigned char fg_alpha;
	unsigned char mask_type;
	unsigned char mask_bd_size;
	unsigned char mask_blk_size;
	unsigned char mask_y[2];
	unsigned char mask_cb;
	unsigned char mask_cr;
} osg_disp_t;

typedef struct _osg_gcac_t_{
	unsigned char enable;

	unsigned char blk_width;
	unsigned char blk_height;
	unsigned char blk_h_num;
	unsigned char org_color_lv;
	unsigned char inv_color_lv;
	unsigned char nor_diff_th;
	unsigned char inv_diff_th;
	unsigned char full_eval_mode;
	unsigned char eval_lum_targ;
	unsigned char sta_only_mode;
} osg_gcac_t;

typedef struct _osg_qpmap_t_{
	unsigned char lpm_mode;
	unsigned char tnr_mode;
	unsigned char fro_mode;
	unsigned char qp_mode;
	char qp_val;
	unsigned char bgr_mode;
	unsigned char maq_mode;
	unsigned char skip_label;
} osg_qpmap_t;

typedef struct _osg_colorkey_t_{
	unsigned char key_en;
	unsigned char alpha_en;
	unsigned char alpha;
	unsigned char red;
	unsigned char green;
	unsigned char blue;
} osg_colorkey_t;

typedef struct _osg_palette_t_{
	unsigned char alpha;
	unsigned char red;
	unsigned char green;
	unsigned char blue;
} osg_palette_t;

typedef struct _osg_win_t_{
	unsigned char enable;

	osg_graph_t graph;
	osg_disp_t  disp;
	osg_gcac_t  gcac;
	osg_qpmap_t qpmap;
	osg_colorkey_t key;
} osg_win_t;

typedef struct _osg_t_{
	unsigned char rgb2yuv[3][3];
	osg_win_t win[10];	//layer0 0~15, layer1 16~31
	osg_palette_t palette[16];
} osg_t;

typedef struct _maq_t_{
	unsigned char mode;
	unsigned char roi_th;
	unsigned char dqp_8x8to16x16_th;
	char dqp[6];
	unsigned char dqpnum;
    unsigned char dqp_mot_th;
} maq_t;

typedef struct _jnd_t_{
	unsigned char enable;
	unsigned char str;
	unsigned char level;
	unsigned char th;
	unsigned char c_str;
	unsigned char c_r5_flag;
	unsigned char bila_flag;
	unsigned char lsigma_th;
	unsigned char lsigma;
} jnd_t;
typedef struct _bgr_t_{
	unsigned char enable;
    unsigned char bgr_typ;
    int  bgr_th[2];
    int  bgr_qp[2];
    int  bgr_vt[2];
    int  bgr_dq[2];
    int  bgr_dth[2];
    int  bgr_bth[2];
} bgr_t;


typedef struct _rmd_t_{
	int    rmd_vh4_y;
	int    rmd_vh8_y;
	int    rmd_vh16_y;
	int    rmd_pl16_y;
	int    rmd_ot4_Y;
	int    rmd_ot8_Y;
} rmd_t;

typedef struct _tnr_t_{
	int  nr_3d_mode;						// 0 = nr off, 1 = nr on
	int  tnr_osd_mode;
	int  mctf_p2p_pixel_blending;
	int  tnr_p2p_sad_mode;
	int  tnr_mctf_sad_mode;
	int  tnr_mctf_bias_mode;

	int  nr_3d_adp_th_p2p[3];
	int  nr_3d_adp_weight_p2p[3];
	int  tnr_p2p_border_check_th;
	int  tnr_p2p_border_check_sc;
	int  tnr_p2p_input;
	int  tnr_p2p_input_weight;
	int  cur_p2p_mctf_motion_th;
	int  ref_p2p_mctf_motion_th;
	int  tnr_p2p_mctf_motion_wt[4];

	int  nr3d_temporal_spatial_y[3];
	int  nr3d_temporal_spatial_c[3];
	int  nr3d_temporal_range_y[3];
	int  nr3d_temporal_range_c[3];
	int  nr3d_clampy_th;
	int  nr3d_clampy_div;
	int  nr3d_clampc_th;
	int  nr3d_clampc_div;

	int  nr3d_temporal_spatial_y_mctf[3];
	int  nr3d_temporal_spatial_c_mctf[3];
	int  nr3d_temporal_range_y_mctf[3];
	int  nr3d_temporal_range_c_mctf[3];
	int  nr3d_clampy_th_mctf;
	int  nr3d_clampy_div_mctf;
	int  nr3d_clampc_th_mctf;
	int  nr3d_clampc_div_mctf;

    int  cur_motion_rat_th;
    int  cur_motion_sad_th;
    int  ref_motion_twr_p2p_th[2];
    int  cur_motion_twr_p2p_th[2];
    int  ref_motion_twr_mctf_th[2];
    int  cur_motion_twr_mctf_th[2];
    int  nr3d_temporal_spatial_y_1[3];
    int  nr3d_temporal_spatial_c_1[3];
    int  nr3d_temporal_spatial_y_mctf_1[3];
    int  nr3d_temporal_spatial_c_mctf_1[3];
    int  sad_twr_p2p_th [2];
    int  sad_twr_mctf_th[2];
} tnr_t;

typedef struct _lambda_t_{
    int adaptlambda_en;
    int lambda_table[52];
    int sqrt_lambda_table[52];
} lambda_t;

typedef struct _eskip_t_{
	unsigned char md_th;
	unsigned char mv_th;
	unsigned int  skip_cost_th;
} eskip_t;

typedef struct _spn_t_{
	char Enable;
	int EdgeWeightTh;
	int EdgeWeightGain;
	int EdgeSharpStr1;
	int EdgeSharpStr2;
	int FlatSharpStr;
	int CoringTh;
	int BrightHaloClip;
	int DarkHaloClip;
	int NoiseLevel;
	int BlendInvGamma;

	int NoiseCurve[17];

	int w_con_eng;
	int th_flat;
	int th_edge;
	int w_low;
	int w_high;
	int slope_con_eng;
	int th_flat_HLD;
	int th_edge_HLD;
	int th_HLD_lum;
	int w_low_HLD;
	int w_high_HLD;
	int slope_con_eng_HLD;

	int flat_region_str;
	int edge_region_str;
	int transition_region_str;

	int motion_edgeWeight_str;
	int static_edgeWeight_str;
} spn_t;

typedef struct _h264_pat_t_{
	char name[64];
	unsigned int idx;
	unsigned int pic_num;

	file_t file;

	unsigned int seq_obj_size;
	unsigned int pic_obj_size;
	unsigned int chk_obj_size;

	info_t info;
	seq_t  seq;
	pic_t  pic;
	chk_t  chk;

	unsigned char rotate;
	unsigned char src_cbcr_iv;
	unsigned int  rrc_chk_en;
	unsigned char aq_chk_en;

	unsigned int slice_number;
	unsigned int bsdma_buf_size;
	unsigned int bsdma_buf_addr;
	unsigned int slice_hdr_buf_size;
	unsigned int slice_hdr_buf_addr;
	unsigned int slice_hdr_len[H264ENC_NAL_MAXSIZE];
	unsigned int slice_len[H264ENC_NAL_MAXSIZE];
	unsigned int source_out_y_tmp_addr;
	unsigned int source_out_c_tmp_addr;

	unsigned int bs_buf_size;
	unsigned int bs_buf_addr;
	unsigned int bs_buf_32b;
	unsigned int res_bs_size;

	unsigned int tmp_src_y_addr;	// use for soucre post processing //
	unsigned int tmp_src_c_addr;

	unsigned int tmp_big_share_mem_addr; // use for mbqp rotate/rotate/source out //
	unsigned int rand_seed;

	h264_perf_t perf;
	unsigned int stable_bs_len;
	unsigned int rnd_slc_hdr;
	unsigned int mbqp_addr;
	unsigned int src_d2d_en;
	unsigned int src_d2d_mode;

	unsigned int usr_qp_map_offset;

	unsigned int performance_check;
	unsigned int performance_osg_range;
}h264_pat_t;

typedef struct _h264_emu_t_{
	H26XENC_VAR  var_obj;
	H264ENC_INIT init_obj;
	H264ENC_INFO info_obj;
}h264_emu_t;

typedef struct _h264_ctx_t_{
	h264_folder_t folder;
	h264_pat_t    pat;
	h264_emu_t    emu;
}h264_ctx_t;

BOOL emu_h264_setup(h26x_ctrl_t *p_ctrl, UINT8 pucDir[265], h26x_srcd2d_t *p_src_d2d);
BOOL emu_h264_prepare_one_pic(h26x_job_t *p_job, h26x_ver_item_t *p_ver_item, h26x_srcd2d_t *p_src_d2d);
BOOL emu_h264_chk_one_pic(h26x_ctrl_t *p_ctrl, h26x_job_t *p_job, UINT32 interrupt, unsigned int rec_out_en, unsigned int cmp_bs_en);
void emu_h264_set_nxt_bsbuf(h26x_job_t *p_job, h26x_ver_item_t *p_ver_item,UINT32 interrupt);
void emu_h264_reset_bsbuf(h26x_job_t *p_job, UINT32 bs_len, unsigned int write_prot);
BOOL emu_h264_do_recout_flow(h26x_ctrl_t *p_ctrl,h26x_job_t *p_job);
BOOL emu_h264_compare_stable_len(h26x_job_t *p_job);

//#endif // (EMU_H26X == ENABLE || AUTOTEST_H26X == ENABLE)

#endif	// _EMU_H264_ENC_H_
