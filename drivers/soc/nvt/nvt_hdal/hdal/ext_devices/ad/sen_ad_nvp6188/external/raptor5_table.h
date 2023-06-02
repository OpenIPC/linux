
#ifndef _RAPTOR5_TABLE_
#define _RAPTOR5_TABLE_

#include "raptor5_common.h"


typedef struct _NC_VIDEO_FMT_INIT_TABLE_S{
	char *name;

	NC_S32 width;
	NC_S32 height;

	NC_FORMAT_FPS_E        fmt_fps;
	NC_FORMAT_STANDARD_E   fmt_standard;
	NC_FORMAT_RESOLUTION_E fmt_resolution;

	/* Video Table *********************/
	NC_U8 nc_table_fmt_std;

	NC_U8 nc_table_fmt_std_mode_set_0x81_sd_ahd;
	NC_U8 nc_table_fmt_std_mode_set_0x85_special;

	NC_U8 nc_table_fmt_std_clock_1x84_vadc;
	NC_U8 nc_table_fmt_std_clock_1x8c_post_pre;

	NC_U8 nc_table_fmt_std_color_5x24_burst_dec_a;
	NC_U8 nc_table_fmt_std_color_5x5f_burst_dec_b;
	NC_U8 nc_table_fmt_std_color_5xd1_burst_dec_c;
	NC_U8 nc_table_fmt_std_color_ax25_y_filter_b;
	NC_U8 nc_table_fmt_std_color_ax27_y_filter_b_sel;

	NC_U8 nc_table_fmt_std_timing_0x89_h_delay_b;
	NC_U8 nc_table_fmt_std_timing_0x8e_h_delay_c;
	NC_U8 nc_table_fmt_std_timing_0xa0_y_delay;

	NC_U8 nc_table_fmt_std_timing_5x6c_v_delay_manual_on;
	NC_U8 nc_table_fmt_std_timing_5x6d_v_delay_manual_val;
	NC_U8 nc_table_fmt_std_timing_5x6e_v_blk_end_manual_on;
	NC_U8 nc_table_fmt_std_timing_5x6f_v_blk_end_manual_val;

	NC_U8 nc_table_fmt_std_timing_9x96_h_scaler1;
	NC_U8 nc_table_fmt_std_timing_9x97_h_scaler2;
	NC_U8 nc_table_fmt_std_timing_9x98_h_scaler3;
	NC_U8 nc_table_fmt_std_timing_9x99_h_scaler4;
	NC_U8 nc_table_fmt_std_timing_9x9a_h_scaler5;
	NC_U8 nc_table_fmt_std_timing_9x9b_h_scaler6;
	NC_U8 nc_table_fmt_std_timing_9x9c_h_scaler7;
	NC_U8 nc_table_fmt_std_timing_9x9d_h_scaler8;
	NC_U8 nc_table_fmt_std_timing_9x9e_h_scaler9;
	NC_U8 nc_table_fmt_std_timing_9x40_pn_auto;
	NC_U8 nc_table_fmt_std_timing_5x90_comb_mode;
	NC_U8 nc_table_fmt_std_timing_5xb9_h_pll_op_a;
	NC_U8 nc_table_fmt_std_timing_5x57_mem_path;
	NC_U8 nc_table_fmt_std_timing_5x25_fsc_lock_speed;
	NC_U8 nc_table_fmt_std_timing_0x64_v_delay;

	/*********************************************/
	NC_U8 nc_table_fmt_std_0x08_video_format;
	NC_U8 nc_table_fmt_std_color_0x0c_brightness;
	NC_U8 nc_table_fmt_std_0x14_h_v_sharpness;
	NC_U8 nc_table_fmt_std_0x1c_n_stb_time;
	NC_U8 nc_table_fmt_std_0x23_novideo_det;
	NC_U8 nc_table_fmt_std_0x34_linemem_md;
	NC_U8 nc_table_fmt_std_0x3c_saturation;
	NC_U8 nc_table_fmt_std_0x5c_v_delay;

	NC_U8 nc_table_fmt_std_1xcc_vport_oclk_sel;

	NC_U8 nc_table_fmt_std_5x00_a_cmp;
	NC_U8 nc_table_fmt_std_5x1b_comb_th_a;
	NC_U8 nc_table_fmt_std_5x1f_y_c_gain;
	NC_U8 nc_table_fmt_std_5x22_u_v_offset2;
	NC_U8 nc_table_fmt_std_5x26_fsc_lock_sense;
	NC_U8 nc_table_fmt_std_5x50_none_;
	NC_U8 nc_table_fmt_std_5x5b_vafe;
	NC_U8 nc_table_fmt_std_5x64_mem_rdp_01;
	NC_U8 nc_table_fmt_std_5xa9_adv_stp_delay1;
	NC_U8 nc_table_fmt_std_5xc8_y_sort_sel;

	NC_U8 nc_table_fmt_std_9x50_fsc_ext_val_1_7_0;
	NC_U8 nc_table_fmt_std_9x51_fsc_ext_val_1_15_8;
	NC_U8 nc_table_fmt_std_9x52_fsc_ext_val_1_23_16;
	NC_U8 nc_table_fmt_std_9x53_fsc_ext_val_1_31_24;

	NC_U8 nc_table_fmt_std_0x30_y_delay;
	NC_U8 nc_table_fmt_std_0xa4_y_c_delay2;
	NC_U8 nc_table_fmt_std_5x6e_v_delay_ex_on;
	NC_U8 nc_table_fmt_std_5x6f_v_delay_val;
	NC_U8 nc_table_fmt_std_5x7b_v_rst_point;

	/* Arbiter Table *********************/
	NC_U8 nc_table_arb_20x01_scale_mode;
	NC_U8 nc_table_arb_20x13_rd_packet_15_8;
	NC_U8 nc_table_arb_20x12_rd_packet_7_0;
	NC_U8 nc_table_arb_20x0d_ch_merge;

	/* Audio Table *********************/
	NC_U8 nc_table_aoc;

	NC_U8 nc_table_aoc_12x01_audio_mode[2];
	NC_U8 nc_table_aoc_12x02_aoc_clk_mode[2];
	NC_U8 nc_table_aoc_12x18_aoc_16k_mode[2];

	NC_U8 nc_table_aoc_12x07_aoc_start_line_high[2];
	NC_U8 nc_table_aoc_12x08_aoc_start_line_low[2];
	NC_U8 nc_table_aoc_12x09_aoc_cat_whd[2];

	NC_U8 nc_table_aoc_12x11_aoc_saa_value[2];
	NC_U8 nc_table_aoc_12x12_aoc_data_value_high[2];
	NC_U8 nc_table_aoc_12x13_aoc_data_value_low[2];
	NC_U8 nc_table_aoc_12x14_aoc_base_unit[2];
	NC_U8 nc_table_aoc_12x15_aoc_catch_point[2];

	/* Video EQ Table *********************/
	NC_U8 nc_table_eq;

    NC_U32 nc_table_eq_distance_value[11];

	NC_U8 nc_table_eq_base_5x01_bypass[11];
	NC_U8 nc_table_eq_base_5x58_band_sel[11];
	NC_U8 nc_table_eq_base_5x5c_gain_sel[11];
	NC_U8 nc_table_eq_base_ax3d_deq_on[11];
	NC_U8 nc_table_eq_base_ax3c_deq_a_sel[11];
	NC_U8 nc_table_eq_base_9x80_deq_b_sel[11];

	NC_U8 nc_table_eq_coeff_ax30_deq_01[11];
	NC_U8 nc_table_eq_coeff_ax31_deq_02[11];
	NC_U8 nc_table_eq_coeff_ax32_deq_03[11];
	NC_U8 nc_table_eq_coeff_ax33_deq_04[11];
	NC_U8 nc_table_eq_coeff_ax34_deq_05[11];
	NC_U8 nc_table_eq_coeff_ax35_deq_06[11];
	NC_U8 nc_table_eq_coeff_ax36_deq_07[11];
	NC_U8 nc_table_eq_coeff_ax37_deq_08[11];
	NC_U8 nc_table_eq_coeff_ax38_deq_09[11];
	NC_U8 nc_table_eq_coeff_ax39_deq_10[11];
	NC_U8 nc_table_eq_coeff_ax3a_deq_11[11];
	NC_U8 nc_table_eq_coeff_ax3b_deq_12[11];

	NC_U8 nc_table_eq_color_0x10_contrast[11];
	NC_U8 nc_table_eq_color_0x18_h_peaking[11];
	NC_U8 nc_table_eq_color_0x21_c_filter[11];
	NC_U8 nc_table_eq_color_0x40_hue[11];
	NC_U8 nc_table_eq_color_0x44_u_gain[11];
	NC_U8 nc_table_eq_color_0x48_v_gain[11];
	NC_U8 nc_table_eq_color_0x4c_u_offset[11];
	NC_U8 nc_table_eq_color_0x50_v_offset[11];
	NC_U8 nc_table_eq_color_5x20_black_level[11];
	NC_U8 nc_table_eq_color_5x27_acc_ref[11];
	NC_U8 nc_table_eq_color_5x28_cti_delay[11];
	NC_U8 nc_table_eq_color_5x2b_sub_saturation[11];
	NC_U8 nc_table_eq_color_5xd5_c_option[11];

	NC_U8 nc_table_eq_timing_0x58_h_delay_a[11];

} NC_VIDEO_FMT_INIT_TABLE_S;

/*=============================================================
 * Coaxial UP/Down stream initialize structure
 ==============================================================*/
typedef struct _NC_COAX_ATTR_TABLE_S{
	char *name;
	unsigned char ch;
	unsigned char rx_src;             //B5/6/7/8 0x7C
	unsigned char rx_slice_lev;       //B5/6/7/8 0x7D
	unsigned char tx_bank;
	unsigned char tx_cmd_addr;
	unsigned char tx_shot_addr;
	unsigned char tx_baud[6];            //B3/4 0x00/80
	unsigned char tx_pel_baud[6];        //B3/4 0x02/82
	unsigned char tx_line_pos0[6];       //B3/4 0x03/83
	unsigned char tx_line_pos1[6];       //B3/4 0x04/84
	unsigned char tx_pel_line_pos0[6];   //B3/4 0x07/87
	unsigned char tx_pel_line_pos1[6];   //B3/4 0x08/88
	unsigned char tx_line_count;         //B3/4 0x05/85
	unsigned char tx_line_count_max;     //B3/4 0x0A/8A
	unsigned char tx_mode;               //B3/4 0x0B/8B
	unsigned char tx_sync_pos0[6];       //B3/4 0x0D/8D
	unsigned char tx_sync_pos1[6];       //B3/4 0x0E/8E
	unsigned char tx_even;            //B3/4 0x2F/AF
	unsigned char tx_zero_length;     //B3/4 0x0C/

	unsigned char rx_comm_on;         //
	unsigned char rx_area;            //
	unsigned char rx_signal_enhance;  //
	unsigned char rx_manual_duty;     //
	unsigned char rx_head_matching;   //
	unsigned char rx_data_rz;         //
	unsigned char rx_sz;              //

	int shot_delay;
	int reset_delay;
}NC_COAX_ATTR_TABLE_S;

/*=============================================================
 * Coaxial UP/Down stream command structure
 ==============================================================*/
typedef struct _NC_COAX_CMD_TABLE_S{
	char *name;
	int ahd_16bit[16];
	int ahd_8bit[16];
	int ahd_4_5m[32];
	int cvi_cmd[32];
	int cvi_new_cmd[32];
	int cvi_checksum[16];
	int tvi_v1_0[16];
	int tvi_v2_0[16];
	int sd[16];
}NC_COAX_CMD_TABLE_S;


NC_VIDEO_FMT_INIT_TABLE_S * nc_drv_table_video_init_vlaue_get( NC_VIVO_CH_FORMATDEF_E def );
NC_VIVO_CH_FORMATDEF_E nc_drv_table_video_format_get( NC_FORMAT_STANDARD_E format_standard, NC_FORMAT_RESOLUTION_E format_resolution, NC_FORMAT_FPS_E format_fps  );

NC_COAX_ATTR_TABLE_S *nc_drv_table_coax_normal_initialize_info_get( NC_VIVO_CH_FORMATDEF_E def );
NC_COAX_ATTR_TABLE_S *nc_drv_table_coax_special_initialize_info_get( NC_VIVO_CH_FORMATDEF_E def );
NC_COAX_CMD_TABLE_S  *nc_drv_table_coax_up_stream_command_get( NC_COAX_CMD_DEF_E def );

NC_VIVO_CH_FORMATDEF_E nc_drv_table_vfc_to_formatdef_get( unsigned char vfc,  char *pstr );
NC_U8 nc_drv_table_formatdef_to_vfc_get( NC_VIVO_CH_FORMATDEF_E fmt_def,  char *pstr );

#endif
/********************************************************************
 *  End of file
 ********************************************************************/
