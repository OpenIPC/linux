#ifndef _H265DEC_HEADER_H_
#define _H265DEC_HEADER_H_

#include "kwrap/type.h"

#include "h26x_bitstream.h"
#include "h265dec_api.h"

#define MAX_VPS_SUB_LAYER_SIZE (2) // tom dbg ori: (63)
#define MAX_VPS_OUTPUT_LAYER_SETS_PLUS1 (1) //tom dbg ori: (2048)
#define MAX_NUM_LAYER_IDS (1)//tom dbg ori: (63)
#define MAX_NUM_ATSC_3_0_LAYERS (1)//(2)
#define MAX_VPS_NUM (1)//(2)    
#define MAX_HEVC_RLIST_ENTRY           (16)
#define MAX_VPS_LAYER_SETS_PLUS1       (1) //tom dbg ori: (1024)
#define ERC   (1)
#define MAX_SPS_NUM                    (16)
#define MAX_PPS_NUM                    (64)
#define MAX_SPS_SUB_LAYER_SIZE         (7)
#define MAX_LT_RP_SIZE                 (1)//tom dbg ori: (32)
#define MAX_ST_RPS_SIZE                (8)//tom dbg ori: (64)
#define MAX_HEVC_WIDTH                 (6656) // 5 tiles: 1408+1280+1280+1280+1408 //tom dbg ori: (5120)//(4096)
#define MAX_HEVC_HEIGHT                (5120)//(2176)
#define MaxLumaPs (MAX_HEVC_WIDTH*MAX_HEVC_HEIGHT)
#define MAX_NUM_REF_PICS               (6) //tom dbg ori: (16)
#define MaxTileCols                    (5)//tom dbg ori: (10)  /* According to spec L1003_v34, max # of tile columns is 10 in profile Level 5.x */
#define MaxTileRows                    (1)//tom dbg ori: (11)  /* According to spec L1003_v34, max # of tile rows is 11 in profile Level 5.x */

static const int SubWidthC[4] = {1, 2, 2, 1};
static const int SubHeightC[4] = {1, 2, 1, 1};

typedef UINT32              u32;
typedef UINT16              u16;
typedef UINT8               u8;
typedef INT8                s8;
typedef INT32               s32;
typedef INT16               s16;

typedef struct {
	u32 profile_present_flag : 1;
	u32 level_present_flag : 1;

	u32 tier_flag : 1;
	u32 profile_idc : 5;
	u32 level_idc : 8;
	u32 profile_compatibility_flag; //32bit

} stProfile;

typedef struct {
	stProfile general;
	stProfile sub_layer[MAX_VPS_SUB_LAYER_SIZE];
} PTL; //profile_tier_level

typedef struct {
	u32 chroma_and_bit_depth_vps_present_flag : 1;
	u32 chroma_format_vps_idc : 2;
	u32 separate_colour_plane_vps_flag : 1;
	u32 bit_depth_vps_luma_minus8 : 4;
	u32 bit_depth_vps_chroma_minus8 : 4;
	u32 conformance_window_vps_flag : 1;
	u16 pic_width_vps_in_luma_samples;
	u16 pic_height_vps_in_luma_samples;
	u32 conf_win_vps_left_offset;
	u32 conf_win_vps_right_offset;
	u32 conf_win_vps_top_offset;
	u32 conf_win_vps_bottom_offset;
} RF;
typedef struct {
	u32 aspect_ratio_info_present_flag : 1;
	u32 video_signal_type_present_flag : 1;
	u32 video_format : 3;
	u32 video_full_range_flag : 1;
	u32 colour_description_present_flag : 1;
	u32 frame_field_info_present_flag : 1;
	u32 default_display_window_flag : 1;
	u32 vui_timing_info_present_flag : 1;
	u32 vui_poc_proportional_to_timing_flag : 1;
	u32 bitstream_restriction_flag : 1;
	u32 tiles_fixed_structure_flag : 1;
	u32 motion_vectors_over_pic_boundaries_flag : 1;
	u32 restricted_ref_pic_lists_flag : 1;
	u8  aspect_ratio_idc; //8bit
	u8  colour_primaries; //8bit
	u8  transfer_characteristics; //8bit
	u8  matrix_coeffs; //8bit
	u16 sar_width; //16bit
	u16 sar_height; //16bit

	u32 def_disp_win_left_offset; //ue(v)
	u32 def_disp_win_right_offset; //ue(v)
	u32 def_disp_win_top_offset; //ue(v)
	u32 def_disp_win_bottom_offset; //ue(v)

	u32 vui_num_units_in_tick; //u(32)
	u32 vui_time_scale; //u(32)
	u32 vui_num_ticks_poc_diff_one_minus1; //ue(v)
	//HP  hrd_parameters;

	u32 min_spatial_segmentation_idc; //ue(v)
	u32 max_bytes_per_pic_denom; //ue(v)
	u32 max_bits_per_min_cu_denom; //ue(v)
	u32 log2_max_mv_length_horizontal; //ue(v)
	u32 log2_max_mv_length_vertical; //ue(v)
} VUIP; //vui_parameters

typedef struct {
	u32 NumNegativePics;
	u32 NumDeltaPocs;
	s32 DeltaPocS0[MAX_NUM_REF_PICS]; //ue(v)

#if MAX_NUM_REF_PICS > 16
	u32 UsedByCurrPicS0; //MAX_NUM_REF_PICS bits
	u32 UsedByCurrPicS1; //MAX_NUM_REF_PICS bits
#else
	u16 UsedByCurrPicS0; //MAX_NUM_REF_PICS bits
	u16 UsedByCurrPicS1; //MAX_NUM_REF_PICS bits
#endif
	u32 curr_num_delta_pocs;
} STRPS; //short_term_ref_pic_set

typedef struct _H265DecVpsObj {
	//QHeader header;
	u32 valid : 1;
	u32 vps_base_layer_internal_flag : 1;
	u32 vps_base_layer_available_flag : 1;
	u32 vps_max_layers_minus1 : 6;
	u32 vps_max_sub_layers_minus1 : 3;
	u32 vps_temporal_id_nesting_flag : 1;
	u32 vps_timing_info_present_flag : 1;
	u32 vps_poc_proportional_to_timing_flag : 1;
	u32 vps_max_layer_id : 6;
	u32 vps_extension_flag : 1;
	u32 splitting_flag : 1;
	u32 vps_extension2_flag : 1;
	u32 vps_sub_layers_max_minus1_present_flag : 1;
	u32 default_ref_layers_active_flag : 1;
	u32 all_ref_layers_active_flag;
	u32 max_tid_ref_present_flag : 1;
	u32 defaultOutputLayerIdc : 2;
	u32 rep_format_idx_present_flag : 1;
	u32 max_one_active_ref_layer_flag : 1;
	u32 vps_poc_lsb_aligned_flag : 1;
	u32 direct_dependency_all_layers_flag : 1;
	u32 vps_vui_present_flag : 1;
	u32 cross_layer_pic_type_aligned_flag : 1;
	u32 cross_layer_irap_aligned_flag : 1;
	u32 all_layers_idr_aligned_flag : 1;
	u32 bit_rate_present_vps_flag : 1;
	u32 pic_rate_present_vps_flag : 1;
	u32 video_signal_info_idx_present_flag : 1;
	u32 tiles_not_in_use_flag : 1;
	u32 wpp_not_in_use_flag : 1;
	u32 single_layer_for_non_irap_flag : 1;
	u32 higher_layer_irap_skip_flag : 1;
	u32 ilp_restricted_ref_layers_flag : 1;
	u32 vps_vui_bsp_hrd_present_flag : 1;

	u32 vps_max_dec_pic_buffering_minus1[MAX_VPS_SUB_LAYER_SIZE];
	u32 vps_max_num_reorder_pics[MAX_VPS_SUB_LAYER_SIZE];
	u32 vps_max_latency_increase_plus1[MAX_VPS_SUB_LAYER_SIZE];

	u32 NumLayerSets;
	u32 NumOutputLayerSets;
	u32 num_add_olss;
	u32 vps_num_units_in_tick;
	u32 vps_time_scale;
	u32 vps_num_ticks_poc_diff_one_minus1;
	u32 vps_num_hrd_parameters;
	u32 vps_num_layer_sets_minus1;
	u32 num_add_layer_sets;
	u32 vps_num_profile_tier_level_minus1;
	u32 vps_num_rep_formats_minus1;
	u32 direct_dep_type_len_minus2;
	u32 direct_dependency_all_layers_type;
	u32 vps_num_add_hrd_params;

	/* The followings will not pass to VPU */
	PTL profile_tier_level[MAX_NUM_LAYER_IDS];
	u32 MaxLayersMinus1;
	u8  LayerSetLayerIdList[MAX_VPS_LAYER_SETS_PLUS1][MAX_NUM_ATSC_3_0_LAYERS];
	u8  NumLayersInIdList[MAX_VPS_LAYER_SETS_PLUS1];
	u8  poc_lsb_not_present_flag[MAX_NUM_ATSC_3_0_LAYERS];
	u8  LayerIdxInVps[MAX_NUM_LAYER_IDS];

} H265DecVpsObj;


typedef struct _H265DecSpsObj {
	//QHeader header;
	u32 valid : 1;
	u32 sps_video_parameter_set_id : 4;
	u32 sps_max_sub_layers_minus1 : 3;
	u32 sps_temporal_id_nesting_flag : 1;
	u32 separate_colour_plane_flag : 1;
	u32 scaling_list_enabled_flag : 1;
	u32 sps_scaling_list_data_present_flag : 1;
	u32 amp_enabled_flag : 1;
	u32 sample_adaptive_offset_enabled_flag : 1;
	u32 long_term_ref_pics_present_flag : 1;
	u32 sps_temporal_mvp_enabled_flag : 1;
	u32 strong_intra_smoothing_enabled_flag : 1;
	u32 sps_infer_scaling_list_flag : 1;
	u32 vui_parameters_present_flag : 1;
	int sps_scaling_list_ref_layer_id;
	PTL profile_tier_level;
	u32 chroma_format_idc; //ue(v)
	u32 pic_width_in_luma_samples; //ue(v)
	u32 pic_height_in_luma_samples; //ue(v)
	u16 conf_win_left_offset; //ue(v)
	u16 conf_win_right_offset; //ue(v)
	u16 conf_win_top_offset; //ue(v)
	u16 conf_win_bottom_offset; //ue(v)
	//u32 bit_depth_luma_minus8; //ue(v)
	//u32 bit_depth_chroma_minus8; //ue(v)
	u32 log2_max_pic_order_cnt_lsb_minus4; //ue(v)
	u32 sps_max_dec_pic_buffering_minus1[MAX_SPS_SUB_LAYER_SIZE]; //ue(v)
	u32 sps_max_num_reorder_pics[MAX_SPS_SUB_LAYER_SIZE]; //ue(v)
	u32 sps_max_latency_increase_plus1[MAX_SPS_SUB_LAYER_SIZE]; //ue(v)
	u32 log2_min_luma_coding_block_size_minus3; //ue(v)
	u32 log2_diff_max_min_luma_coding_block_size; //ue(v)
	u32 log2_min_transform_block_size_minus2; //ue(v)
	u32 log2_diff_max_min_transform_block_size; //ue(v)
	u32 max_transform_hierarchy_depth_inter; //ue(v)
	u32 max_transform_hierarchy_depth_intra; //ue(v)

	//SLD scaling_list_data;
	u32 num_short_term_ref_pic_sets; //ue(v)
	u32 num_long_term_ref_pics_sps; //ue(v)
	u32 used_by_curr_pic_lt_sps_flag; //MAX_LT_RP_SIZE bit
	VUIP  vui_parameters;

	/* The followings will not pass to VPU */
	u32 lt_ref_pic_poc_lsb_sps[MAX_LT_RP_SIZE]; //u(v)
	STRPS short_term_ref_pic_set[MAX_ST_RPS_SIZE];

    UINT32 pic_len_slice_addr;
    UINT32 Log2MinCuQpDeltaSize;

	u32 ucSVCLayer; // 0,1,2

	//need remove
	//UINT32 uiPocType;
	//UINT32 uiFrmMbsOnly;


} H265DecSpsObj;

typedef struct _H265DecPpsObj {
	//QHeader header;
	u32 valid : 1;
	u32 dependent_slice_segments_enabled_flag : 1;
	u32 output_flag_present_flag : 1;
	u32 num_extra_slice_header_bits : 3;
	u32 cabac_init_present_flag : 1;
	u32 cu_qp_delta_enabled_flag : 1;
	u32 pps_slice_chroma_qp_offsets_present_flag : 1;
	u32 weighted_pred_flag : 1;
	u32 tiles_enabled_flag  : 1;
	u32 entropy_coding_sync_enabled_flag : 1;
	u32 uniform_spacing_flag : 1;
	u32 loop_filter_across_tiles_enabled_flag : 1;
	u32 pps_loop_filter_across_slices_enabled_flag : 1;
	u32 deblocking_filter_control_present_flag : 1;
	u32 deblocking_filter_override_enabled_flag : 1;
	u32 pps_deblocking_filter_disabled_flag : 1;
	u32 slice_segment_header_extension_present_flag : 1;
	u32 lists_modification_present_flag : 1;
	u32 chroma_qp_offset_list_enabled_flag : 1;
	u32 poc_reset_info_present_flag : 1;
	u32 pps_infer_scaling_list_flag : 1;
	u32 pps_scaling_list_ref_layer_id : 6;
	u32 colour_mapping_enabled_flag : 1;
	u32 to_calculate_each_tile_boundary : 1; //internal use
	u32 to_fill_color_map_table : 1; //internal use

	u32 pps_seq_parameter_set_id; //ue(v)
	u32 num_ref_idx_l0_default_active_minus1; //ue(v)
	u32 num_ref_idx_l1_default_active_minus1; //ue(v)
	s32 init_qp_minus26; //se(v)

	u32 diff_cu_qp_delta_depth; //ue(v)
	s32 pps_cb_qp_offset; //se(v)
	s32 pps_cr_qp_offset; //se(v)
	u32 num_tile_columns_minus1; //ue(v)
	u32 num_tile_rows_minus1; //ue(v)

	u16 column_width[MaxTileCols];
	u16 row_height[MaxTileRows];

	s32 pps_beta_offset_div2;
	s32 pps_tc_offset_div2;
} H265DecPpsObj;

typedef struct _H265DecSliceObj {
	//QHeader header;
	u32 sliceStartAddr;
	u32 sliceEndAddr;
	u32 sliceSize;
	u32 nalType;
	u32 valid : 1;
	u32 first_slice_segment_in_pic_flag : 1;
	u32 NoClrasOutputFlag : 1;
	u32 NoRaslOutputFlag : 1;
	u32 decode_pic_order_cnt_lsb : 1;
	u32 shvc_inter_layer_se_bits : 6;
	u32 slice_segment_address;

	/* begin of dependent slice information */
	u8 dependent_slice_start;
	u32 slice_type;
	s32 slice_pic_order_cnt_lsb;
	s32 PicOrderCntVal;
	s32 MaxPicOrderCntLsb;

	/* short & long term reference information  */
	STRPS short_term_ref_pic_set;
	u32 num_long_term_sps;
	u32 num_long_term_ref_pics;
	s32 pocLt[32]; //u(v)
	s32 DeltaPocLt[32];
	u32 UsedByCurrPicLt; //total 32 bit

	u32 ref_pic_list_modification_flag_l0 : 1;
	u32 ref_pic_list_modification_flag_l1 : 1;
	u32 slice_sao_luma_flag : 1;
	u32 slice_sao_chroma_flag : 1;
	u32 slice_temporal_mvp_enabled_flag : 1;
	u32 mvd_l1_zero_flag : 1;
	u32 cabac_init_flag : 1;
	u32 collocated_from_l0_flag : 1;
	u32 deblocking_filter_override_flag : 1;
	u32 slice_deblocking_filter_disabled_flag : 1;
	u32 slice_loop_filter_across_slices_enabled_flag : 1;
	u32 inter_layer_pred_enabled_flag : 1;
	u32 NumActiveRefLayerPics;
	u32 num_ref_idx_l0_active;
	u32 num_ref_idx_l1_active;
	s32 list_entry_l0[32];
	s32 list_entry_l1[32];

	UINT32 slice_qp; //uiQP
	s32 slice_tc_offset_div2;
	s32 slice_beta_offset_div2;
	u32 uiTemporalId;
	u32 discalefactor;
    UINT32 pic_len_ref_list;
} H265DecSliceObj;


typedef struct _H265DecSVCObj {
	UINT32 uiSVCFlag;
	UINT32 uiLayer;
	UINT32 uiTemporalId[3];
	UINT32 uiFrmRateInfoFlag;
	UINT32 uiAvgFrmRate[3];
	UINT32 uiFrmSizeInfoFlag;
	UINT32 uiWidth;
	UINT32 uiHeight;
	UINT32 uiTid;       // every frame's temporal id
} H265DecSVCObj;

#define DecNalBufSize 512

typedef struct _H265DecHdrObj {
	H265DecVpsObj   sH265DecVpsObj;
	H265DecSpsObj   sH265DecSpsObj;
	H265DecPpsObj   sH265DecPpsObj;
	H265DecSliceObj sH265DecSliceObj;
	H265DecSVCObj   sH265DecSvcObj;
    UINT8 ucH265DecNalBuf[DecNalBufSize];
} H265DecHdrObj;
extern INT32 H265DecVps(H265DecVpsObj *vps, bstream *pBstream);
extern INT32 H265DecSps(H265DecSpsObj *sps, H265DecVpsObj *vps, bstream *pBs, int nuh_layer_id, H26XDEC_VUI *pVui);
extern INT32 H265DecPps(H265DecPpsObj *pps, bstream *pBs);
extern UINT32 getNumRpsCurrTempList(H265DecSliceObj *slice);
extern void parseRPLM(UINT32 type, H265DecSliceObj *slice, bstream *pBs);
extern int calculateLenOfSyntaxElement(int numVal);
extern int readByteAlignment(bstream *const bs);
extern INT32 H265DecSlice(H265DEC_INFO *pH265DecInfo, H265DecHdrObj *pH265DecHdrObj, UINT32 uiBsAddr, UINT32 uiBsLen,
                            int nuh_layer_id);
extern INT32 H265DecSeqHeader(H265DEC_INIT *pH265DecInit, H265DecHdrObj *pH265DecHdrObj, H26XDEC_VUI *pVui);
extern void parsePTL(int profilePresentFlag, int maxNumSubLayersMinus1, PTL *pptl, bstream *pBs);
extern UINT32 H265DecNal(bstream *pBstream, H265DecSliceObj *slice);
extern int parseSTRPS(int idxRps, int num_short_term_ref_pic_sets, STRPS *rps, bstream *pBs, H265DecSpsObj *sps);
extern INT32 H265DecSeqHdrInfo(UINT32 uiBsAddr, UINT32 uiBsSize, UINT32 uiWorkBuf, UINT32 uiWorkBufSize, H265DEC_HDRINFO *pHdrInfo);

#endif
