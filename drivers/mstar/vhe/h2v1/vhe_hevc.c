
#include <linux/errno.h>
#include <vhe_defs.h>
#include <vhe_hevc.h>

static void
ebsp_put(
    bs_t*   bs,
    uchar   b)
{
    if (2 == bs->zero_cnt && 4 > b)
    {
        *bs->curr++ = 0x3;
        bs->zero_cnt = 0;
        bs->bits_pos += 8;
    }
    if (0 == b)
        bs->zero_cnt++;
    else
        bs->zero_cnt = 0;
    *bs->curr++ = b;
}

static int
hevc_reset_bs(
    bs_t*   bs,
    uchar*  p,
    int     l)
{
    bs->bits_pos = 0;
    bs->left_cnt = 16;
    bs->code_bit = 0;
    bs->curr = bs->buff = p;
    bs->bits_len = l;
    return 0;
}

static int
hevc_flush_bs(
    bs_t*   bs)
{
    while (bs->left_cnt < 16)
    {
        ebsp_put(bs, (uchar)(bs->code_bit>>8));
        bs->left_cnt += 8;
        bs->code_bit<<= 8;
    }
    bs->left_cnt = 16;
    bs->code_bit = 0;
    return bs->bits_pos;
}

static int
hevc_count_bs(
    bs_t*   bs)
{
    return bs->bits_pos;
}

static int
bits_put(
    bs_t*   bs,
    int     l,
    uint    b)
{
    if (bs->left_cnt > l)
    {
        bs->left_cnt -= l;
        bs->code_bit += b << bs->left_cnt;
    }
    else
    {
        int left = (16 - l + bs->left_cnt);
        uint pat = (bs->code_bit << 16) + (b << left);
        bs->left_cnt = left;
        bs->code_bit = pat & 0xFFFF;
        ebsp_put(bs, (uchar)(pat>>24));
        ebsp_put(bs, (uchar)(pat>>16));
    }
    bs->bits_pos += l;
    return l;
}

static const uchar ue_code_length[256] = {
 1, 1, 3, 3, 5, 5, 5, 5, 7, 7, 7, 7, 7, 7, 7, 7,
 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9,
11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,
11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,
13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,
13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,
13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,
13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,
15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,
15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,
15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,
15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,
15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,
15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,
15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,
15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,
};

static int
bs_write_ue(
    bs_t*   bs,
    uint    v)
{
    int l;
    int u = (v + 1) >> 8;
    if (0 == u)
        return bits_put(bs, (int)ue_code_length[v+1], (uint)v+1);
    u = (ue_code_length[u] + 16) >> 1;
    l = bits_put(bs, u, 0);
    return (l + bits_put(bs, u + 1, v + 1));
}

static int
bs_write_se(
    bs_t*   bs,
    int     v)
{
    v = 1 - 2 * v;
    v = v < 0 ? (1 - v) : v;
    return bits_put(bs, (int)ue_code_length[v], (uint)v);
}

static int
bs_write(
    bs_t*   bs,
    int     l,
    uint    v)
{
    return bits_put(bs, l, v);
}

static int
bs_write_u1(
    bs_t*   bs,
    int     v)
{
    return bits_put(bs, 1, (uint)v!=0);
}

static int
bs_trail(
    bs_t*   bs)
{
    int len = 0x7 & (--bs->left_cnt);
    bs->code_bit += (1 << bs->left_cnt);
    bs->bits_pos = (bs->bits_pos + 8) & ~0x7;
    bs->left_cnt &= ~0x7;
    if (0 == bs->left_cnt)
    {
        ebsp_put(bs, (uchar)(bs->code_bit>>8));
        ebsp_put(bs, (uchar)(bs->code_bit));
    }
    else if (8 == bs->left_cnt)
        ebsp_put(bs, (uchar)(bs->code_bit>>8));
    bs->left_cnt = 16;
    bs->code_bit = 0;
    return ++len;
}

static int
bs_write_nal(
    bs_t*   bs,
    ushort  nuh)
{
    /* start code */
    *bs->curr++ = 0;
    *bs->curr++ = 0;
    *bs->curr++ = 0;
    *bs->curr++ = 1;
    *bs->curr++ = (uchar)(nuh>>8);
    *bs->curr++ = (uchar)(nuh +1);
    bs->bits_pos += 48;
    bs->left_cnt = 16;
    bs->code_bit = 0;
    bs->zero_cnt = 0;
    return 48;
}

#define _put_ue(v,coment)       bs_write_ue(bs,(uint)v)
#define _put_se(v,coment)       bs_write_se(bs,( int)v)
#define _put_u1(v,coment)       bs_write_u1(bs,(uint)v)
#define _put(l, v,coment)       bs_write(bs, l,(uint)v)

static int write_ptl(bs_t*, ptl_t*, int sublayers);

static int
hevc_write_vps(
    bs_t*   bs,
    vps_t*  vps)
{
    int bpos = bs->bits_pos;

    bs_write_nal(bs, NUH_PACK(NUT_VPS,0,0));

    _put(4, vps->i_id,                                  "vps_id");
    _put(2, 3,                                          "reserved_three_2bits");
    _put(6, 0,                                          "max_layers_minus1");
    _put(3, vps->i_max_num_sub_layers -1,               "max_sub_layers_minus1");
    _put_u1(vps->i_max_num_sub_layers==1,               "temporal_id_nesting_flag");
    _put(8, 0xFF,                                       "reserved_0xff_8bits");
    _put(8, 0xFF,                                       "reserved_0xff_8bits");
    /* profile tier level */
    write_ptl(bs, &vps->ptl, vps->i_max_num_sub_layers);
    _put_u1(vps->b_sub_layers_ordering_pres,            "sub_layer_ordering_info_present_flag");
    if (vps->b_sub_layers_ordering_pres)
    {   /* max_num_sub_layers < 2 */
        if (vps->i_max_num_sub_layers > 0)
        {
            _put_ue(vps->i_max_dec_pic_buffering-1,     "max_dec_pic_buffering_minux1");
            _put_ue(vps->i_max_num_reorder_pics,        "max_num_reorder_pics");
            _put_ue(vps->i_max_latency_increase +1,     "max_latency_increase_plus1");
        }
    }
    _put(6, 0,                                          "max_layer_id");
    _put_ue(0,                                          "num_layer_sets_minus1");
    _put_u1(0,                                          "timing_info_present_flag");
    /* TODO: If "timing_info_present_flag" is ON, .... */
    _put_u1(0,                                          "extension_flag");

    bs_trail(bs);

    return (bs->bits_pos - bpos);
}

static int
write_ptl(
    bs_t*   bs,
    ptl_t*  ptl,
    int     sublayers)
{
    int i;

    _put(2, 0,                                          "profile_space");
    _put_u1(0,                                          "tier_flag");
    _put(5, ptl->i_profile,                             "profile_idc");
    for (i = 0; i < 32; i++)
        _put_u1(0,                                      "profile_compatibility_flag");
    _put_u1(!ptl->b_interlaced,                         "progressive_source_flag");
    _put_u1( ptl->b_interlaced,                         "interlaced_source_flag");
    _put_u1(0,                                          "non_packed_constraint_flag");
    _put_u1(0,                                          "frame_only_constraint_flag");
    /* "general_reserved_zero_44bits" */
    _put(16,0,                                          "reserved_zero_16bits");
    _put(16,0,                                          "reserved_zero_16bits");
    _put(12,0,                                          "reserved_zero_12bits");
    _put(8, ptl->i_level,                               "level_idc");
    if (sublayers > 1)
    {
        return -1;
    }
    return 0;
}

static int write_rps_st(bs_t*, rps_t*);
static int write_vui(bs_t*, vui_t*);

static int
hevc_write_sps(
    bs_t*   bs,
    sps_t*  sps)
{
    int i;
    rps_t* rps = 0;
    int bpos = bs->bits_pos;

    bs_write_nal(bs, NUH_PACK(NUT_SPS,0,0));

    _put(4, sps->p_vps->i_id,                               "vps_id");
    _put(3, sps->i_max_num_sub_layers -1,                   "max_sub_layers_minus1");
    _put_u1(sps->i_max_num_sub_layers==1,                   "temporal_id_nesting_flag");
    /* profile tier level */
    write_ptl(bs, &sps->ptl, sps->i_max_num_sub_layers);
    _put_ue(sps->i_id,                                      "sps_id");
    _put_ue(sps->i_chroma_format_idc,                       "chroma_format_idc");
    /* support chroma_format_idc == 1 only */
    /* Note that pic_width_in_luma_samples and pic_height_in_luma_samples
     * shall be an integer multiple of (1 << Log2MinCbSize). */
    _put_ue(sps->i_pic_w_min_cbs,                           "pic_w_in_luma_samples");
    _put_ue(sps->i_pic_h_min_cbs,                           "pic_h_in_luma_samples");
    _put_u1(sps->b_conformance_window,                      "conformance_window_flag");
    if (sps->b_conformance_window)
    {
        _put_ue(sps->i_crop_l,                              "conf_win_left_offset");
        _put_ue(sps->i_crop_r,                              "conf_win_right_offset");
        _put_ue(sps->i_crop_t,                              "conf_win_top_offset");
        _put_ue(sps->i_crop_b,                              "conf_win_bottom_offset");
    }
    _put_ue(0,                                              "bit_depth_luma_minus8");
    _put_ue(0,                                              "bit_depth_chroma_minus8");
    _put_ue(sps->i_log2_max_poc_lsb-4,                      "log2_max_pic_order_cnt_lsb_minus4");
    _put_u1(sps->b_sub_layers_ordering_pres,                "sub_layer_ordering_info_present_flag");
    if (sps->b_sub_layers_ordering_pres)
    {
        if (sps->i_max_num_sub_layers > 0)
        {
            _put_ue(sps->i_max_dec_pic_buffering-1,         "max_dec_pic_buffering_minus1");
            _put_ue(sps->i_max_num_reorder_pics,            "max_num_reorder_pics");
            _put_ue(sps->i_max_latency_increase +1,         "max_latency_increase_plus1");
        }
    }
    _put_ue(sps->i_log2_min_cb_size-3,                      "log2_min_coding_block_size_minus3");
    _put_ue(sps->i_log2_max_cb_size-sps->i_log2_min_cb_size,"log2_diff_max_min_coding_block_size");
    _put_ue(sps->i_log2_min_tr_size-2,                      "log2_min_transform_block_size_minus2");
    _put_ue(sps->i_log2_max_tr_size-sps->i_log2_min_tr_size,"log2_diff_max_min_transform_block_size");
    _put_ue(sps->i_max_tr_hierarchy_depth_inter,            "max_transform_hierarchy_depth_inter");
    _put_ue(sps->i_max_tr_hierarchy_depth_intra,            "max_transform_hierarchy_depth_intra");
    /* NOT support "scaling_list_enable_flag" == 1. */
    _put_u1(sps->b_scaling_list_enable,                     "scaling_list_enable_flag");
    if (sps->b_scaling_list_enable)
    {   /* NOT support "b_scaling_list_data_pres" == 1. */
        _put_u1(0,                                          "scaling_list_data_present_flag");
    }
    _put_u1(sps->b_amp_enabled,                             "amp_enabled_flag");
    _put_u1(sps->b_sao_enabled,                             "sao_enabled_flag");
    _put_u1(sps->b_pcm_enabled,                             "pcm_enabled_flag");
    /* support "b_pcm_enabled" == OFF only */
    _put_ue(sps->i_num_short_term_ref_pic_sets,             "num_short_term_ref_pic_sets");
    for (i = 0; i < sps->i_num_short_term_ref_pic_sets; i++)
    {
        //rps = (rps_t*)hevc_pset_get(sps->q_rps, HEVC_RPS, i);
        rps = &sps->rps[i];
        write_rps_st(bs, rps);
    }
    _put_u1(sps->b_long_term_ref_pics_pres,                 "long_term_ref_pics_present_flag");
    /* support "b_long_term_ref_pics_pres" == OFF only */
    _put_u1(sps->b_temporal_mvp_enable,                     "temporal_mvp_enable_flag");
    _put_u1(sps->b_strong_intra_smoothing_enabled,          "strong_intra_smoothing_enabled_flag");
    _put_u1(sps->b_vui_param_pres,                          "vui_parameters_present_flag");
    /* TODO: if (vui_parameters_present_flag)... */
    if (sps->b_vui_param_pres)
        write_vui(bs, &sps->vui);
    _put_u1(0,                                              "extension_flag");
    bs_trail(bs);

    return (bs->bits_pos - bpos);
}

static int
write_rps_st(
    bs_t*   bs,
    rps_t*  rps)
{
    int i, poc;
    if (rps->i_id)
        _put_u1(0,                                          "inter_ref_pic_set_prediction_flag");
    /*
    if (b_inter_rps_pred) {
          ....
        return 0;
    }
    */
    _put_ue(rps->i_num_neg_pics,                            "num_negative_pics");
    _put_ue(rps->i_num_pos_pics,                            "num_positive_pics");
    for (i = poc = 0; i < rps->i_num_neg_pics; i++)
    {
        poc = poc - (int)rps->i_dpoc[i];
        _put_ue(poc - 1,                                    "delta_poc_s0_minus1");
        _put_u1(rps->b_used[i],                             "used_by_curr_pic_s0_flag");
        poc = (int)rps->i_dpoc[i];
    }
    for (poc = 0; i < (rps->i_num_neg_pics + rps->i_num_pos_pics); i++)
    {
        poc = (int)rps->i_dpoc[i] - poc;
        _put_ue(poc-1,                                      "delta_poc_s0_minus1");
        _put_u1(rps->b_used[i],                             "used_by_curr_pic_s0_flag");
        poc = (int)rps->i_dpoc[i];
    }
    return 0;
}

static int get_aspect_ratio_idc(int, int);
static int calc_scale(int);

static int
write_vui(
    bs_t*   bs,
    vui_t*  vui)
{
    int sar_idc = get_aspect_ratio_idc(vui->i_sar_w, vui->i_sar_h);

    if (0 == sar_idc)
        _put_u1(0,                                          "aspect_ratio_info_present_flag");
    else
    {
        _put_u1(1,                                          "aspect_ratio_info_present_flag");
        _put(8, sar_idc,                                    "aspect_ratio_idc");
        if (sar_idc == EXTENDED_SAR)
        {
            _put(16, vui->i_sar_w,                          "sar_width");
            _put(16, vui->i_sar_h,                          "sar_height");
        }
    }
    _put_u1(0,                                              "overscan_info_present_flag");
    _put_u1(vui->b_video_signal_pres,                       "video_signal_type_present_flag");
    if (vui->b_video_signal_pres)
    {
        _put(3, vui->i_video_format,                        "video_format: unspecified");
        _put_u1(vui->b_video_full_range,                    "video_full_range_flag");
        _put_u1(vui->b_colour_desc_pres,                    "colour_description_present_flag");
        if (vui->b_colour_desc_pres)
        {
            _put(8, vui->i_colour_primaries,                "colour_primaries: unspecified");
            _put(8, vui->i_transf_character,                "transf_characteristics: unspecified");
            _put(8, vui->i_matrix_coeffs,                   "matrix_coeffs: unspecified");
        }
    }
    _put_u1(0,                                              "chroma_loc_info_present_flag");
    _put_u1(0,                                              "neutral_chroma_indication_flag");

    _put_u1(vui->b_field_seq,                               "field_seq_flag");
    _put_u1(vui->b_frame_field_info_pres,                   "frame_field_info_present_flag");
    /*
     * input support progressive frame and interlacedfield picture:
     *   pic_struct=0(display in progressive),
     *   source_scan_type=1(source is progressive),
     *   duplicate_flag=0(not duplicate frame)
     */
    _put_u1(0,                                              "default_display_window_flag");
    _put_u1(vui->b_timing_info_pres,                        "timing_info_present_flag");
    if (vui->b_timing_info_pres)
    {
        _put(32,vui->i_num_units_in_tick,                   "num_units_in_tick");
        _put(32,vui->i_time_scale,                          "time_scale");
        _put_u1(0,                                          "poc_proportional_to_timing_flag");
        _put_u1(vui->b_hrd_parameters_pres,                 "hrd_parameters_present_flag");
        if (vui->b_hrd_parameters_pres)
        {
            uint val;
            uint bps_scale = 1;
            uint cpb_scale = 1;
            _put_u1(vui->b_nal_hrd_pres,                    "nal_hrd_parameters_present_flag");
            _put_u1(0,                                      "vcl_hrd_parameters_present_flag");
            _put_u1(0,                                      "sub_pic_hrd_params_present_flag");
            bps_scale = calc_scale(vui->i_bit_rate);
            cpb_scale = calc_scale(vui->i_cpb_size);
            _put(4, bps_scale-BPS_SHIFT,                    "bit_rate_scale");
            _put(4, cpb_scale-CPB_SHIFT,                    "cpb_size_scale");
            _put(5, vui->i_init_cpb_removal_delay_len-1,    "initial_cpb_removal_delay_length_minus1");
            _put(5, vui->i_au_cpb_removal_delay_len-1,      "au_cpb_removal_delay_length_minus1");
            _put(5, vui->i_dpb_output_delay_len-1,          "dpb_output_delay_length_minus1");
            _put_u1(1,                                      "b_fixed_pic_rate_general[i]");
            _put_ue(0,                                      "elemental_duration_in_tc_minus1[i]");
            _put_ue(0,                                      "cpb_cnt_minus1[i]");
            val = vui->i_bit_rate >> (BPS_SHIFT + bps_scale);
            _put_ue(val - 1,                                "bit_rate_value_minus1");
            val = vui->i_cpb_size >> (CPB_SHIFT + cpb_scale);
            _put_ue(val - 1,                                "cpb_size_value_minus1");
            _put_u1(0,                                      "cbr_flag");
        }
    }

    _put_u1(vui->b_bitstream_restriction,                   "bitstream_restriction_flag");
    if (vui->b_bitstream_restriction)
    {
        _put_u1(1,                                          "tiles_fixed_structure_flag");
        _put_u1(1,                                          "motion_vectors_over_pic_boundaries");
        _put_u1(1,                                          "restricted_ref_pic_lists_flag");
        _put_ue(0,                                          "min_spatial_segmentation_idc");
        _put_ue(0,                                          "max_bytes_per_pic_denom");
        _put_ue(0,                                          "max_bits_per_min_cu_denom");
        _put_ue(LOG2_MAX_MV_LENGTH_HOR,                     "log2_max_mv_length_horizontal");
        _put_ue(LOG2_MAX_MV_LENGTH_VER,                     "log2_max_mv_length_vertical");
    }

    return 0;
}

static int
calc_scale(
    int v)
{
    int i = 0, m = 1;
    while (v >= m)
    {
        m <<= 1;
        i++;
    }
    return i;
}

static int
get_aspect_ratio_idc(
    int sar_w,
    int sar_h)
{
    int sar_idc = 0;

    if (sar_w == 0 || sar_h == 0) /* unspecified */
        sar_idc = 0;
    else if (sar_w == sar_h) /* square, 1:1 */
        sar_idc = 1;
    else if (sar_h == 11)
    {
        if (sar_w == 12)        /* 12:11 */
            sar_idc = 2;
        else if (sar_w == 10)   /* 10:11 */
            sar_idc = 3;
        else if (sar_w == 16)   /* 16:11 */
            sar_idc = 4;
        else if (sar_w == 24)   /* 24:11 */
            sar_idc = 6;
        else if (sar_w == 20)   /* 20:11 */
            sar_idc = 7;
        else if (sar_w == 32)   /* 32:11 */
            sar_idc = 8;
        else if (sar_w == 18)   /* 18:11 */
            sar_idc = 10;
        else if (sar_w == 15)   /* 15:11 */
            sar_idc = 11;
        else                    /* Extended_SAR */
            sar_idc = EXTENDED_SAR;
    }
    else if (sar_h == 33)
    {
        if (sar_w == 40)        /* 40:33 */
            sar_idc = 5;
        else if (sar_w == 80)   /* 80:33 */
            sar_idc = 9;
        else if (sar_w == 64)   /* 64:33 */
            sar_idc = 12;
        else                    /* Extended_SAR */
            sar_idc = EXTENDED_SAR;
    }
    else if (sar_w == 160 && sar_h == 99)   /* 160:99 */
        sar_idc = 13;
    else if (sar_w == 4 && sar_h == 3)  /* 4:3 */
        sar_idc = 14;
    else if (sar_w == 3 && sar_h == 2)  /* 3:2 */
        sar_idc = 15;
    else if (sar_w == 2 && sar_h == 1)  /* 2:1 */
        sar_idc = 16;
    else                                /* Extended_SAR */
        sar_idc = EXTENDED_SAR;

    return (sar_idc);
}

static int
hevc_write_pps(
    bs_t*   bs,
    pps_t*  pps)
{
    int bpos = bs->bits_pos;

    bs_write_nal(bs, NUH_PACK(NUT_PPS,0,0));

    _put_ue(pps->i_id,                                      "pps_id");
    _put_ue(pps->p_sps->i_id,                               "sps_id");
    _put_u1(0,                                              "dependent_slice_segments_enabled_flag");
    _put_u1(0,                                              "output_flag_present_flag");
    _put(3, 0,                                              "num_extra_slice_header_bits");
    _put_u1(0,                                              "sign_data_hiding_flag");
    _put_u1(pps->b_cabac_init_pres,                         "cabac_init_present_flag");
    _put_ue(pps->i_num_ref_idx_l0_default_active - 1,       "num_ref_idx_l0_default_active_minus1");
    _put_ue(pps->i_num_ref_idx_l1_default_active - 1,       "num_ref_idx_l1_default_active_minus1");
    _put_se(pps->i_init_qp - 26,                            "init_qp_minus26");
    _put_u1(pps->b_constrained_intra_pred,                  "constrained_intra_pred_flag");
    _put_u1(pps->b_transform_skip_enabled,                  "transform_skip_enabled_flag");
    _put_u1(pps->b_cu_qp_delta_enabled,                     "cu_qp_delta_enabled_flag");
    if (pps->b_cu_qp_delta_enabled)
        _put_ue(pps->i_diff_cu_qp_delta_depth,              "diff_cu_qp_delta_depth");
    _put_se(pps->i_cb_qp_offset,                            "pps_cb_qp_offset");
    _put_se(pps->i_cr_qp_offset,                            "pps_cr_qp_offset");
    _put_u1(pps->b_slice_chroma_qp_offsets_pres,            "pps_slice_chroma_qp_offsets_present_flag");
    _put_u1(0,                                              "weighted_pred_flag");
    _put_u1(0,                                              "weighted_bipred_flag");
    _put_u1(0,                                              "transquant_bypass_enable_flag");
    _put_u1(0,                                              "tiles_enabled_flag");
    _put_u1(0,                                              "entropy_coding_sync_enabled_flag");
    /* "pps->tiles_enabled_flag" MUST BE 0. */
    /*
    if (pps->tiles_enabled_flag)
    {
        _put_ue(pps->i_num_tile_columns - 1,                "num_tile_columns_minus1");
        _put_ue(pps->i_num_tile_rows - 1,                   "num_tile_rows_minus1");
        _put_u1(pps->b_uniform_spacing,                     "uniform_spacing_flag");
        if (!pps->b_uniform_spacing)
        {
            for (i = 0; i < pps->i_num_tile_columns - 1; i++)
            {
                _put_ue(p->i_col_width[i] - 1,              "column_width_minus1[i]");
            }
            for (i = 0; i < p->num_tile_rows - 1; i++)
            {
                _put_ue(p->i_row_height[i] - 1,             "row_height_minus1[i]");
            }
        }
        _put_u1(b, pps->b_loop_filter_across_tiles_enabled, "loop_filter_across_tiles_enabled_flag");
    }
    */
    _put_u1(pps->b_loop_filter_across_slices_enabled,       "loop_filter_across_slices_enabled_flag");
    _put_u1(pps->b_deblocking_filter_control_pres,          "deblocking_filter_control_present_flag");
    if (pps->b_deblocking_filter_control_pres)
    {
        _put_u1(pps->b_deblocking_filter_override_enabled,  "deblocking_filter_override_enabled_flag");
        _put_u1(pps->b_deblocking_filter_disabled,          "deblocking_filter_disabled_flag");
        if (!pps->b_deblocking_filter_disabled)
        {
            _put_se(pps->i_beta_offset / 2,                 "beta_offset_div2");
            _put_se(pps->i_tc_offset / 2,                   "tc_offset_div2");
        }
    }
    _put_u1(0,                                              "scaling_list_data_present_flag");
    /* "b_scaling_list_data_pres" MUST BE 0. */
    /*
    if (p->b_scaling_list_data_pres)
    {
        scaling_list(0);
    }
    */
    _put_u1(0,                                              "lists_modification_present_flag");
    _put_ue(pps->i_log2_parallel_merge_level - 2,           "log2_parallel_merge_level_minus2");
    _put_u1(0,                                              "slice_segment_header_extension_present_flag");
    _put_u1(0,                                              "extension");

    bs_trail(bs);

    return (bs->bits_pos - bpos);
}

static vps_t vps_def = {
    .i_id = 0,
    .i_max_num_sub_layers = 1,
    .b_sub_layers_ordering_pres = 1,
    .i_max_dec_pic_buffering = 5,
    .i_max_num_reorder_pics = 0,
    .i_max_latency_increase = -1,
    .ptl = {
        .i_profile = HEVC_PROFILE_MAIN,
        .i_level = HEVC_LEVEL_6,
        .b_interlaced = 0,
    },
};

static sps_t sps_def = {
    .i_id = 0,
    .i_max_num_sub_layers = 1,
    .b_sub_layers_ordering_pres = 1,
    .i_max_dec_pic_buffering = 5,
    .i_max_num_reorder_pics = 0,
    .i_max_latency_increase = -1,
    .ptl = {
        .i_profile = HEVC_PROFILE_MAIN,
        .i_level = HEVC_LEVEL_6,
        .b_interlaced = 0,
    },
    .i_chroma_format_idc = 1,
    .i_pic_w_min_cbs = 0,
    .i_pic_h_min_cbs = 0,
    .b_conformance_window = 0,
    .i_crop_l = 0,
    .i_crop_t = 0,
    .i_crop_r = 0,
    .i_crop_b = 0,
    .i_log2_max_poc_lsb = 8,
    .b_sub_layers_ordering_pres = 1,
    .i_log2_min_cb_size = 3,
    .i_log2_max_cb_size = 6,
    .i_log2_min_tr_size = 2,
    .i_log2_max_tr_size = 4,
    .i_max_tr_hierarchy_depth_inter = 4,
    .i_max_tr_hierarchy_depth_intra = 2,
    .b_scaling_list_enable = 0,
    .b_amp_enabled = 1,
    .b_sao_enabled = 1,
    .b_pcm_enabled = 0,
    .b_strong_intra_smoothing_enabled = 1,
    .b_long_term_ref_pics_pres = 0,
    /* vui params */
    .b_vui_param_pres = 1,
    .vui = {
        .b_video_signal_pres = 1,
        .i_video_format = 5,
        .b_video_full_range = 1,
        .b_colour_desc_pres = 0,
        .i_colour_primaries = 1,
        .i_transf_character = 1,
        .i_matrix_coeffs = 1,
    },
};

static pps_t pps_def = {
    .i_id = 0,
    .b_cabac_init_pres = 1,
    .i_num_ref_idx_l0_default_active = 4,
    .i_num_ref_idx_l1_default_active = 4,
    .i_init_qp = 26,
    .b_constrained_intra_pred = 0,
    .b_transform_skip_enabled = 0,
    .b_cu_qp_delta_enabled = 0,
    .i_cb_qp_offset = 0,
    .i_cr_qp_offset = 0,
    .b_slice_chroma_qp_offsets_pres = 0,
    .b_loop_filter_across_slices_enabled = 1,
    .b_deblocking_filter_control_pres = 1,
    .b_deblocking_filter_override_enabled = 0,
    .b_deblocking_filter_disabled = 0,
    .i_beta_offset = 10,
    .i_tc_offset = -4,
    .i_log2_parallel_merge_level = 2,
};

static pic_t*
find_rpb_poc(
    hevc_enc*   hevc,
    int         poc)
{
    int i;
    for (i = 0; i < hevc->i_rpbn; i++)
    {
        pic_t* pic = hevc->m_rpbs+i;
        if (poc == pic->i_poc)
            return pic;
    }
    return NULL;
}

void*
hevc_find_set(
    hevc_enc*   hevc,
    int type,
    int id)
{
    void* set = NULL;
    switch (type)
    {
    case HEVC_VPS:
        set = &hevc->m_vps;
        break;
    case HEVC_SPS:
        set = &hevc->m_sps;
        break;
    case HEVC_PPS:
        set = &hevc->m_pps;
        break;
    default:
        break;
    }
    return set;
}

void*
hevc_find_rpl(
    hevc_enc*   hevc,
    int list,
    int idx)
{
    if ((unsigned)list >= 2 || (unsigned)idx >= MAX_REF_SIZE)
        return NULL;
    return hevc->p_rpls[list][idx];
}

static void _release(void* vptr) { MEM_FREE(vptr); }

hevc_enc*
h265enc_acquire(void)
{
    hevc_enc* hevc = NULL;
    do
    {
        pps_t* pps = NULL;
        sps_t* sps = NULL;
        vps_t* vps = NULL;

        if (!(hevc = MEM_ALLC(sizeof(hevc_enc))))
            return hevc;

        hevc->release = _release;
        hevc->m_vps = vps_def;
        hevc->m_sps = sps_def;
        hevc->m_pps = pps_def;
        pps = &hevc->m_pps;
        sps = &hevc->m_sps;
        vps = &hevc->m_vps;
        pps->p_sps = sps;
        sps->p_vps = vps;

        hevc->i_profile = HEVC_PROFILE_MAIN;
        hevc->i_level = HEVC_LEVEL_6;
        hevc->b_deblocking_override = 0;
        hevc->b_deblocking_disable = pps->b_deblocking_filter_disabled;
        hevc->i_tc_offset = pps->i_tc_offset;
        hevc->i_beta_offset = pps->i_beta_offset;

        return hevc;
    }
    while (0);
    hevc->release(hevc);
    hevc = NULL;
    return hevc;
}

int
hevc_seq_init(
    hevc_enc*   hevc,
    int         rpbn)
{
    pps_t* pps = hevc_find_set(hevc, HEVC_PPS, 0);
    sps_t* sps = pps->p_sps;
    rps_t* rps;
    int i;

    sps->i_num_short_term_ref_pic_sets = 2;
    /* rps-0: refer previous picture */
    rps = &sps->rps[0];
    rps->i_id = 0;
    rps->i_num_neg_pics = 1;
    rps->i_dpoc[0] = -1;
    rps->b_used[0] = 1;
    rps->i_num_pos_pics = 0;
    /* rps-1: refer previous 2nd picture */
    rps = &sps->rps[1];
    rps->i_id = 1;
    rps->i_num_neg_pics = 1;
    rps->i_dpoc[0] = -2;
    rps->b_used[0] = 1;
    rps->i_num_pos_pics = 0;

    for (i = 0; i < rpbn; i++)
    {
        pic_t* pic = hevc->m_rpbs+i;
        pic->i_id = i;
        pic->i_poc = -1;
        pic->b_reference = 0;
        pic->p_pps = pps;
        pic->p_sps = sps;
    }
    for (i = rpbn; i < MAX_RPB_SIZE; i++)
    {
        hevc->m_rpbs[i].i_poc = -2;
        hevc->m_rpbs[i].p_pps = NULL;
        hevc->m_rpbs[i].p_sps = NULL;
    }
    hevc->i_rpbn = rpbn;

    sps->ptl.i_profile = hevc->i_profile;
    sps->ptl.i_level = hevc->i_level;
    sps->ptl.b_interlaced = hevc->b_interlaced;

    hevc->i_cb_w = (hevc->i_picw + ((1 << sps->i_log2_max_cb_size) - 1)) >> sps->i_log2_max_cb_size;
    hevc->i_cb_h = (hevc->i_pich + ((1 << sps->i_log2_max_cb_size) - 1)) >> sps->i_log2_max_cb_size;

    hevc->i_poc = 0;
    hevc->b_active_override = 1;

    return 0;
}

int
hevc_seq_conf(
    hevc_enc*   hevc)
{
    pps_t* pps = hevc_find_set(hevc, HEVC_PPS, 0);
    sps_t* sps = pps->p_sps;
    vps_t* vps = sps->p_vps;
    bs_t outbs,*bs = &outbs;

    hevc->i_cb_w = (hevc->i_picw + ((1 << sps->i_log2_max_cb_size) - 1)) >> sps->i_log2_max_cb_size;
    hevc->i_cb_h = (hevc->i_pich + ((1 << sps->i_log2_max_cb_size) - 1)) >> sps->i_log2_max_cb_size;

    hevc->i_slices = 1;
    if (hevc->i_rows > 0)
        hevc->i_slices = (hevc->i_cb_h + hevc->i_rows - 1) / hevc->i_rows;

    sps->i_pic_w_min_cbs = (hevc->i_picw+MIN_CB_SIZE-1)&~(MIN_CB_SIZE-1);
    sps->i_pic_h_min_cbs = (hevc->i_pich+MIN_CB_SIZE-1)&~(MIN_CB_SIZE-1);
    sps->b_conformance_window = (sps->i_pic_w_min_cbs != hevc->i_picw) || (sps->i_pic_h_min_cbs != hevc->i_pich);
    if (sps->b_conformance_window)
    {
        sps->i_crop_l = 0;
        sps->i_crop_t = 0;
        sps->i_crop_r = (hevc->i_picw-sps->i_pic_w_min_cbs) >> 1;
        sps->i_crop_b = (hevc->i_pich-sps->i_pic_h_min_cbs) >> 1;
    }
    hevc->i_poc = 0;

    hevc_reset_bs(bs, hevc->m_seqh, 128);
    hevc_write_vps(bs, vps);
    hevc_write_sps(bs, sps);
    hevc_write_pps(bs, pps);
    hevc_flush_bs(bs);
    hevc->i_seqh = hevc_count_bs(bs)/8;

    return 0;
}

int
hevc_seq_done(
    hevc_enc*   hevc)
{
    return 0;
}

static pic_t* _pic_avail(hevc_enc* hevc);
static pic_t* _rps_reset(hevc_enc* hevc, int ipoc, int rpsi);

int
hevc_enc_buff(
    hevc_enc*   hevc,
    int         type,
    int         refp)
{
    sps_t* sps = &hevc->m_sps;
    pic_t* rec = NULL;
    int dref = !refp;
    int rpsi = !hevc->b_refp;

    if (hevc->p_recn)
        return 0;

    hevc->i_type = type;
    hevc->i_poc++;

    if (HEVC_ISLICE == hevc->i_type)
        hevc->i_poc = rpsi = dref = 0;

    if ((unsigned)rpsi >= sps->i_num_short_term_ref_pic_sets)
        return -1;
#if defined(HEVC_RPS_PRESET)
    if ((rec = _pic_avail(hevc)))
#else
    if ((rec = _rps_reset(hevc, hevc->i_poc, rpsi)) ||
        (rec = _pic_avail(hevc)))
#endif
    {
        rec->i_type = type;
        rec->i_poc = hevc->i_poc;
        rec->p_rps = type==HEVC_ISLICE?NULL:&sps->rps[rpsi];
        hevc->b_refp = !dref;
        hevc->p_recn = rec;
        return rec->i_id;
    }
    return -1;
}

int
hevc_enc_done(
    hevc_enc*   hevc)
{
    int idx = -1;
#if defined(HEVC_RPS_PRESET)
   pic_t* pic = _rps_reset(hevc, hevc->i_poc+1, !hevc->b_refp);
    if (pic)
    {
        pic->i_poc = -1;
        idx = pic->i_id;
    }
#endif
    hevc->p_recn = NULL;
    return idx;
}

static pic_t* _pic_avail(
    hevc_enc*   hevc)
{
    int i;
    pic_t* rpb = hevc->m_rpbs;
    for (i = 0; i < hevc->i_rpbn; i++, rpb++)
        if (rpb->i_poc < 0)
            return rpb;
    return NULL;
}

static pic_t* _rps_reset(
    hevc_enc*   hevc,
    int         ipoc,
    int         rpsi)
{
    pic_t* pics = hevc->m_rpbs, *rpb, *pic;
    rps_t* rps;
    short pocs[MAX_RPS_SIZE];
    int i, j;

    for (i = 0; i < MAX_REF_SIZE; i++)
        hevc->p_rpls[0][i] = hevc->p_rpls[1][i] = NULL;

    hevc->i_active_l0_count = hevc->i_active_l1_count = 0;
    for (rpb = pics, i = 0; i < hevc->i_rpbn; i++, rpb++)
        rpb->b_reference = 0;
    if (!ipoc)
    {   /* Intra-frame or IDR-frame */
        for (rpb = pics, pic = NULL, i = 0; i < hevc->i_rpbn; i++, rpb++)
            if (rpb->i_poc >= 0)
                pic = rpb;
        return pic;
    }

    rps = &hevc->m_sps.rps[rpsi];
    for (i = 0; i < rps->i_num_neg_pics + rps->i_num_pos_pics; i++)
    {
        pic = find_rpb_poc(hevc, ipoc + rps->i_dpoc[i]);
        if (!pic)
            return pic;
        pic->b_reference = 1;
        pocs[i] = ipoc + rps->i_dpoc[i];
    }
    for (    i = 0; i < hevc->i_rpbn; i++)
        if (!pics[i].b_reference)
            pic = &pics[i];
    for (i = j = 0; i < rps->i_num_neg_pics; i++)
        if (rps->b_used[i] && (hevc->p_rpls[0][j] = find_rpb_poc(hevc, pocs[i])))
            j++;
    hevc->i_active_l0_count = j;
    for (    j = 0; i < rps->i_num_neg_pics + rps->i_num_pos_pics; i++)
        if (rps->b_used[i] && (hevc->p_rpls[1][j] = find_rpb_poc(hevc, pocs[i])))
            j++;
    hevc->i_active_l1_count = j;

    return pic;
}
