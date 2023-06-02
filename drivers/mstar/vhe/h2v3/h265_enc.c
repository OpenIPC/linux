#include <linux/errno.h>
#include <h2v3_def.h>
#include <h265_enc.h>

#define NAL_LEN 16
#define ALIGN_EQU_ONE 1
#define ALIGN8(x) (((x)+0x7)&~0x7)


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
h265_reset_bs(
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
h265_flush_bs(
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
h265_count_bs(
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

static int bs_write_32(
    bs_t*   bs,
    uint    v)
{
    int len;
    len = bs_write(bs, 16, (v>>16)&0xFFFF);
    len += bs_write(bs, 16, v&0xFFFF);
    return len;
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
#define _put_32(v,coment)       bs_write_32(bs,(uint)v)

int get_used_bits(base* ld)
{
	int tmp=0;

	tmp=ld->Bitcnt;

	return (ld->Bitcnt);
}

void Flush_Buffer(base* ld, int N)
{
    int Incnt;

    ld->Bfr <<= N;
    Incnt = ld->Incnt -= N;
    if (Incnt <= 24)
    {
        do
        {
            ld->Bfr |= *ld->Rdptr++ << (24 - Incnt);
            Incnt += 8;
        }
        while (Incnt <= 24);

        ld->Incnt = Incnt;
    }
}

unsigned int Show_Bits(base* ld, int N)
{
    return ld->Bfr >> (32-N);
}

unsigned int bs_read(base* ld, int N)
{
    unsigned int Val;
    unsigned int count=0;
    unsigned int remindbits=0;

    if(N<32)
    {
        Val = Show_Bits(ld, N);
        Flush_Buffer(ld, N);
    }
    else
    {
        do
        {
            remindbits=N-count;
            if(remindbits<16)
            {
                Val = Show_Bits(ld, remindbits);
                Flush_Buffer(ld, remindbits);
                count+=remindbits;
                break;
            }
            else
            {
                Val = Show_Bits(ld, 16);
                Flush_Buffer(ld, 16);
                count+=16;
            }
        }while(1);
    }
    ld->Bitcnt +=N;
    return Val;
}

unsigned int bs_read_u1(base* ld)
{
    return bs_read(ld, 1);
}

void Fill_Buffer(base *ld, unsigned char *pData,unsigned int Datalength)
{
    int Buffer_Level;

    memset(ld->Rdbfr, 0, sizeof(ld->Rdbfr));
    Buffer_Level=(Datalength<=BIT_BUFFER_SIZE)?Datalength : BIT_BUFFER_SIZE;
    memcpy(ld->Rdbfr,pData,Buffer_Level);

    ld->Rdptr = ld->Rdbfr;
    Flush_Buffer(ld, 0);
}

unsigned int bs_read_ue(base* ld)
{
    unsigned int tmp=0;
    unsigned int length=0;
    unsigned int ret=0;

    do
    {
        tmp=bs_read(ld, 1)|(tmp<<1);
        if(tmp!=0)
        {
            break;
        }
        length++;
    }while(1);

    if(length!=0)
    {
        tmp=bs_read(ld, length);
        ret=tmp+(1<<length)-1;
    }
    else
    {
        ret=0;
    }

    return ret;
}

int bs_read_se(base* ld)
{
    unsigned int tmp=0;
    unsigned int length=0;
    unsigned int ret=0;

    do
    {
        tmp=bs_read(ld, 1)|(tmp<<1);
        if(tmp!=0)
        {
            break;
        }
        length++;
    }while(1);

    if(length!=0)
    {
        tmp=bs_read(ld, length);
        tmp+=(1<<length);
        ret=( tmp & 1) ? -(int)(tmp>>1) : (int)(tmp>>1);
    }
    else
    {
        ret=0;
    }

    return ret;
}

#define _get_ue(coment)       bs_read_ue(ld)
#define _get_se(coment)       bs_read_se(ld)
#define _get_u1(coment)       bs_read_u1(ld)
#define _get(N, coment)       bs_read(ld, N)

static int write_ptl(bs_t*, ptl_t*, int sublayers);

static int
h265_write_vps(
    bs_t*   bs,
    vps_t*  vps)
{
    int bpos = bs->bits_pos;

    bs_write_nal(bs, NUH_PACK(NUT_VPS , 0, 0));

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
    _put_u1(vps->b_timing_info_pres,                    "timing_info_present_flag");
    if(vps->b_timing_info_pres)
    {
        _put_32(vps->i_num_units_in_tick,               "num_units_in_tick");
        _put_32(vps->i_time_scale,                      "time_scale");
        _put_u1(0,                                      "poc_proportional_to_timing_flag");
        _put_ue(0,                                      "hrd_parameters_present_flag");
    }
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
h265_write_sps(
    bs_t*   bs,
    sps_t*  sps)
{
    int i;
    rps_t* rps = 0;
    int bpos = bs->bits_pos;

    bs_write_nal(bs, NUH_PACK(NUT_SPS, 0, 0));

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
//        rps = (rps_t*)h265_pset_get(sps->q_rps, HEVC_RPS, i);
        rps = &sps->rps[i];
        write_rps_st(bs, rps);
    }
    _put_u1(sps->b_long_term_ref_pics_pres,                 "long_term_ref_pics_present_flag");
    if (sps->b_long_term_ref_pics_pres)
    {
        _put_ue(0,                                          "num_long_term_ref_pics_sps");
    }
    /* support "b_long_term_ref_pics_pres" == OFF only */
    _put_u1(sps->b_temporal_mvp_enable,                     "temporal_mvp_enable_flag");
    _put_u1(sps->b_strong_intra_smoothing_enabled,          "strong_intra_smoothing_enabled_flag");
    _put_u1(sps->b_vui_param_pres,                          "vui_parameters_present_flag");
    if (sps->b_vui_param_pres)
        write_vui(bs, &sps->vui);
    _put_u1(0,                                              "extension_flag");
    bs_trail(bs);

    return (bs->bits_pos - bpos);
}

/* write SPS reference table */
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
     * input support progressive frame and interlaced field picture:
     * pic_struct=0(display in progressive),
     * source_scan_type=1(source is progressive),
     * duplicate_flag=0(not duplicate frame)
     */
    _put_u1(0,                                              "default_display_window_flag");
    _put_u1(vui->b_timing_info_pres,                        "timing_info_present_flag");
    if (vui->b_timing_info_pres)
    {
        _put_32(vui->i_num_units_in_tick,                   "num_units_in_tick");
        _put_32(vui->i_time_scale,                          "time_scale");
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
h265_write_pps(
    bs_t*   bs,
    pps_t*  pps)
{
    int bpos = bs->bits_pos;

    bs_write_nal(bs, NUH_PACK(NUT_PPS, 0, 0));

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

int
h265_sh_writer(
    h265_enc* h265)
{
    pps_t *pps = h265_find_set(h265, HEVC_PPS, 0);
    sps_t *sps = h265_find_set(h265, HEVC_SPS, 0);
    slice_t *sh= h265_find_set(h265, HEVC_SLICE, 0);
    bs_t outbs,*bs = &outbs;
    int bpos = 0;

    h265_reset_bs(bs, h265->m_swsh, SLICE_HEADER_BIAS_SIZE);

    bs_write_nal(bs, NUH_PACK(sh->i_nal_type, 0, 0));

    bpos = bs->bits_pos;

    _put_u1(sh->first_slice_segment_in_pic_flag,                "first_slice_segment_in_pic_flag");
    if (sh->i_nal_type >= BLA_W_LP && sh->i_nal_type <= RSV_IRAP_VCL23)
        _put_u1(sh->no_output_of_prior_pics_flag,                "no_output_of_prior_pics_flag");
    _put_ue(sh->slice_pic_parameter_set_id,                      "slice_pic_parameter_set_id");
#if 0
    if( !first_slice_segment_in_pic_flag )
    {
        ...
    }
#endif
    _put_ue(sh->i_slice_type,                                    "slice_type");
    if (sh->i_slice_type != SLICE_TYPE_I)
    {
        _put(sps->i_log2_max_poc_lsb, (uint)h265->i_poc & ((1 << sps->i_log2_max_poc_lsb) - 1),
                                                                 "slice_pic_order_cnt_lsb");
        _put_u1(1,                                               "short_term_ref_pic_set_sps_flag");
        _put(1, sh->short_term_ref_pic_set_idx,                  "short_term_ref_pic_set_idx");
        if(sps->b_long_term_ref_pics_pres)
        {
            _put_ue(sh->num_long_term_pics,                      "num_long_term_pics");
            if (sh->num_long_term_pics)
            {
//                for( i = 0; i < num_long_term_sps + num_long_term_pics; i++ )
//                {
                _put(sps->i_log2_max_poc_lsb, sh->poc_lsb_lt[0],"poc_lsb_lt[0]");
                _put_u1(sh->used_by_curr_pic_lt_flag[0],        "used_by_curr_pic_lt_flag[0]");
                _put_u1(sh->delta_poc_msb_present_flag[0],      "delta_poc_msb_present_flag[0]");
//                }
            }
        }

        if(sps->b_temporal_mvp_enable)
        {
            _put_u1(sh->slice_temporal_mvp_enabled_flag,         "slice_temporal_mvp_enabled_flag");
        }
    }

    if(sps->b_sao_enabled)
    {
        _put_u1(sh->slice_sao_luma_flag,                         "slice_sao_luma_flag");
        _put_u1(sh->slice_sao_chroma_flag,                       "slice_sao_chroma_flag");
    }

    if (sh->i_slice_type == SLICE_TYPE_P || sh->i_slice_type == SLICE_TYPE_B)
    {
        _put_u1(0,                                               "num_ref_idx_active_override_flag");
        if (pps->b_cabac_init_pres)
        {
            _put_u1(0,                                           "cabac_init_flag");
        }
        _put_ue(1,                                               "five_minus_max_num_merge_cand");
    }
    _put_se(sh->i_qp - pps->i_init_qp,                           "slice_qp_delta");

    if(pps->b_deblocking_filter_override_enabled)
    {
        _put_u1(sh->deblocking_filter_override_flag,             "deblocking_filter_override_flag");
    }

    if(sh->deblocking_filter_override_flag)
    {
        _put_u1(sh->slice_deblocking_filter_disabled_flag,       "slice_deblocking_filter_disabled_flag");
        if(!sh->slice_deblocking_filter_disabled_flag)
        {
             _put_se(sh->slice_beta_offset_div2,                 "slice_beta_offset_div2");
             _put_se(sh->slice_tc_offset_div2,                   "slice_tc_offset_div2");
        }
    }
    _put_u1(1,                                                   "slice_loop_filter_across_slices_enabled_flag");

    bs_trail(bs);
    h265_flush_bs(bs);

    return (bs->bits_pos - bpos)/8 + 6;
}

static vps_t vps_def =
{
    .i_id = 0,
    .i_max_num_sub_layers = 1,
    .b_sub_layers_ordering_pres = 1,
    .i_max_dec_pic_buffering = MAX_DEC_PIC_BUFFERING_DEFAULT,
    .i_max_num_reorder_pics = 0,
    .i_max_latency_increase = -1,
    .ptl =
    {
        .i_profile = HEVC_PROFILE_MAIN,
//        .i_level = HEVC_LEVEL_6,
        .i_level = HEVC_LEVEL_4_1,
        .b_interlaced = 0,
    },
};

static sps_t sps_def =
{
    .i_id = 0,
    .i_max_num_sub_layers = 1,
    .b_sub_layers_ordering_pres = 1,
    .i_max_dec_pic_buffering = MAX_DEC_PIC_BUFFERING_DEFAULT,
    .i_max_num_reorder_pics = 0,
    .i_max_latency_increase = -1,
    .ptl =
    {
        .i_profile = HEVC_PROFILE_MAIN,
//        .i_level = HEVC_LEVEL_6,
        .i_level = HEVC_LEVEL_4_1,
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
    .b_temporal_mvp_enable = 0,
    .b_strong_intra_smoothing_enabled = 0,
    .b_long_term_ref_pics_pres = 0,
    /* vui params */
    .b_vui_param_pres = 1,
    .vui =
    {
        .b_video_signal_pres = 1,
        .i_video_format = 5,
        .b_video_full_range = 1,
        .b_colour_desc_pres = 0,
        .i_colour_primaries = 1,
        .i_transf_character = 1,
        .i_matrix_coeffs = 1,
    },
};

static pps_t pps_def =
{
    .i_id = 0,
    .b_cabac_init_pres = 1,
    .i_num_ref_idx_l0_default_active = 1,
    .i_num_ref_idx_l1_default_active = 1,
    .i_init_qp = 26,
    .b_constrained_intra_pred = 0,
    .b_transform_skip_enabled = 0,
    .b_cu_qp_delta_enabled = 0,
    .i_diff_cu_qp_delta_depth = 0,
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

static slice_t sh_def =
{
    .first_slice_segment_in_pic_flag = 1,
    .no_output_of_prior_pics_flag = 0,
    .slice_pic_parameter_set_id = 0,
    .short_term_ref_pic_set_sps_flag = 0,
    .short_term_ref_pic_set_idx = 0,
    .num_long_term_pics = 0,
    .poc_lsb_lt[0] = 0,
    .used_by_curr_pic_lt_flag[0] = 0,
    .delta_poc_msb_present_flag[0] = 0,
    .slice_temporal_mvp_enabled_flag = 1,
    .b_active_override = 1,
    .b_cabac_init = 0,
    .i_max_num_merge_cand = 4,
    .slice_loop_filter_across_slices_enabled_flag = 1,
};

static pic_t*
find_rpb_poc(
    h265_enc*   h265,
    int         poc)
{
    int i;
    pic_t* pic;
    for (i = 0; i < h265->i_rpbn; i++)
    {
        pic = h265->m_pics+i;
        if (poc == pic->i_poc)
            return pic;
    }
    return NULL;
}

void*
h265_find_set(
    h265_enc*   h265,
    int type,
    int id)
{
    void* set = NULL;
    switch (type)
    {
    case HEVC_VPS:
        set = &h265->m_vps;
        break;
    case HEVC_SPS:
        set = &h265->m_sps;
        break;
    case HEVC_PPS:
        set = &h265->m_pps;
        break;
    case HEVC_SLICE:
        set = &h265->m_sh;
        break;
    default:
        break;
    }
    return set;
}

void*
h265_find_rpl(
    h265_enc*   h265,
    int list,
    int idx)
{
    sps_t* sps = h265_find_set(h265, HEVC_SPS, 0);
    int max_list = sps->b_long_term_ref_pics_pres? HEVC_REF_MAX : HEVC_REF_MAX-1;

    if ((unsigned)list >= max_list || (unsigned)idx >= MAX_REF_SIZE)
        return NULL;
    return h265->p_rpls[list][idx];
}

static void _release(void* vptr) { MEM_FREE(vptr); }

h265_enc*
h265enc_acquire(void)
{
    h265_enc* h265 = MEM_ALLC(sizeof(h265_enc));
    pps_t*  pps = NULL;
    sps_t*  sps = NULL;
    vps_t*  vps = NULL;
    slice_t* sh = NULL;

    if (!h265)
    {
        printk(KERN_ERR"%s() alloc fail\n", __func__);
        return h265;
    }

    h265->release = _release;
    h265->m_vps = vps_def;
    h265->m_sps = sps_def;
    h265->m_pps = pps_def;
    h265->m_sh  = sh_def;
    pps = &h265->m_pps;
    sps = &h265->m_sps;
    vps = &h265->m_vps;
    sh  = &h265->m_sh;
    pps->p_sps = sps;
    sps->p_vps = vps;
    sh->p_sps  = sps;
    sh->p_pps  = pps;

    h265->i_profile = HEVC_PROFILE_MAIN;
//        h265->i_level = HEVC_LEVEL_6;
    h265->i_level = HEVC_LEVEL_4_1;
    h265->b_deblocking_override = 0;
    h265->b_deblocking_disable = pps->b_deblocking_filter_disabled;
    h265->i_tc_offset = pps->i_tc_offset;
    h265->i_beta_offset = pps->i_beta_offset;

    return h265;
}

int
h265_seq_init(
    h265_enc*   h265,
    int         rpbn)
{
    pps_t* pps = h265_find_set(h265, HEVC_PPS, 0);
    sps_t* sps = h265_find_set(h265, HEVC_SPS, 0);
    rps_t* rps;
    pic_t* pic;
    int i;

    sps->i_num_short_term_ref_pic_sets = 2;
    h265->i_num_short_term_ref_pic_sets = 2;
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
    if (sps->b_long_term_ref_pics_pres)
        rps->i_num_neg_pics = 0;
    else
        rps->i_num_neg_pics = 1;
    rps->i_dpoc[0] = -2;
    rps->b_used[0] = 1;
    rps->i_num_pos_pics = 0;

    /* initialize picture buffer */
    for (i = 0; i < rpbn; i++)
    {
        pic = h265->m_pics+i;
        pic->i_id = i;
        pic->i_poc = POC_BUFFER_FREE;
        pic->b_reference = 0;
        pic->b_ltr_keep = 0;
        pic->b_cached = 0;
        pic->p_pps = pps;
        pic->p_sps = sps;
    }
    /* clear pic_t if not in used */
    for (i = rpbn; i < MAX_RPB_SIZE; i++)
    {
        h265->m_pics[i].i_poc = POC_BUFFER_INVL;
        h265->m_pics[i].p_pps = NULL;
        h265->m_pics[i].p_sps = NULL;
    }
    h265->i_rpbn = rpbn;

    sps->ptl.i_profile = h265->i_profile;
    sps->ptl.i_level = h265->i_level;
    sps->ptl.b_interlaced = h265->b_interlaced;

    h265->i_cb_w = (h265->i_picw + ((1 << sps->i_log2_max_cb_size) - 1)) >> sps->i_log2_max_cb_size;
    h265->i_cb_h = (h265->i_pich + ((1 << sps->i_log2_max_cb_size) - 1)) >> sps->i_log2_max_cb_size;

    h265->i_poc = 0;
    h265->b_active_override = 1;

    return 0;
}

int
h265_seq_sync(
    h265_enc*   h265)
{
    pic_t* pic;
    int i;

    /* initialize picture buffer */
    for (i = 0; i < h265->i_rpbn; i++)
    {
        pic = h265->m_pics+i;
        pic->i_poc = POC_BUFFER_FREE;
//        pic->b_ltr_keep = 0;
//        pic->b_reference = 0;
    }
    return 0;
}

int
h265_seq_conf(
    h265_enc*   h265)
{
    pps_t* pps = h265_find_set(h265, HEVC_PPS, 0);
    sps_t* sps = h265_find_set(h265, HEVC_SPS, 0);
    vps_t* vps = h265_find_set(h265, HEVC_VPS, 0);
    bs_t outbs,*bs = &outbs;

    h265->i_cb_w = (h265->i_picw + ((1 << sps->i_log2_max_cb_size) - 1)) >> sps->i_log2_max_cb_size;
    h265->i_cb_h = (h265->i_pich + ((1 << sps->i_log2_max_cb_size) - 1)) >> sps->i_log2_max_cb_size;

    h265->i_slices = 1;
    if (h265->i_rows > 0)
        h265->i_slices = (h265->i_cb_h + h265->i_rows - 1) / h265->i_rows;

    sps->i_pic_w_min_cbs = (h265->i_picw+MIN_CB_SIZE-1)&~(MIN_CB_SIZE-1);
    sps->i_pic_h_min_cbs = (h265->i_pich+MIN_CB_SIZE-1)&~(MIN_CB_SIZE-1);
    sps->b_conformance_window = (sps->i_pic_w_min_cbs != h265->i_picw) || (sps->i_pic_h_min_cbs != h265->i_pich);
    if (sps->b_conformance_window)
    {
        sps->i_crop_l = 0;
        sps->i_crop_t = 0;
        sps->i_crop_r = (h265->i_picw-sps->i_pic_w_min_cbs) >> 1;
        sps->i_crop_b = (h265->i_pich-sps->i_pic_h_min_cbs) >> 1;
    }
    h265->i_poc = 0;

    h265_reset_bs(bs, h265->m_seqh, 128);
    h265_write_vps(bs, vps);
    h265_write_sps(bs, sps);
    h265_write_pps(bs, pps);
    h265_flush_bs(bs);
    h265->i_seqh = h265_count_bs(bs)/8;

    return 0;
}

int
h265_seq_done(
    h265_enc*   h265)
{
    return 0;
}

static pic_t* _pic_avail(h265_enc* h265);
static pic_t* _rps_reset(h265_enc* h265, int ipoc, int rpsi);

int
h265_enc_buff(
    h265_enc*   h265,
    int         type,
    int         refp)
{
    sps_t* sps = &h265->m_sps;
    pic_t* rec = h265->p_recn;
    int dref = !refp;
    int rpsi = !h265->b_refp;

    if (rec)
    {
        if (HEVC_ISLICE == type)
        {
            h265->b_refp = 1;
            h265->i_poc = 0;
            rec->i_poc = 0;
            rec->p_rps = NULL;
        }
        if (sps->b_long_term_ref_pics_pres)
        {
            if (HEVC_ISLICE == type || (h265->b_enable_pred && HEVC_LTRPSLICE == type))
                rec->b_ltr_keep = 1;
        }
        rec->i_type = type;
        return rec->i_id;
    }

    h265->i_poc++;
    if (HEVC_ISLICE == type)
        h265->i_poc = rpsi = dref = 0;
    if (sps->b_long_term_ref_pics_pres)
        h265->i_swsh = 0;

    if ((unsigned)rpsi >= sps->i_num_short_term_ref_pic_sets)
        return -1;
#if defined(HEVC_RPS_PRESET)
    if ((rec = _pic_avail(h265)))
#else
    if ((rec = _rps_reset(h265, h265->i_poc, rpsi)) ||
        (rec = _pic_avail(h265)))
#endif
    {
        rec->i_type = type;
        rec->i_poc = h265->i_poc;
        rec->p_rps = type==HEVC_ISLICE ? NULL : &sps->rps[rpsi];
        h265->b_refp = !dref;
        h265->p_recn = rec;
        if (sps->b_long_term_ref_pics_pres)
        {
            if (HEVC_ISLICE == type || (h265->b_enable_pred && HEVC_LTRPSLICE == type))
                rec->b_ltr_keep = 1;
        }
        return rec->i_id;
    }
    printk(KERN_ERR"vhe-found null recn buffer\n");
    return -1;
}

int
h265_enc_done(
    h265_enc*   h265)
{
    int idx = -1;
#if defined(HEVC_RPS_PRESET)
    pic_t* pic = _rps_reset(h265, h265->i_poc+1, !h265->b_refp);
    if (pic)
    {
        pic->i_poc = POC_BUFFER_FREE;
        idx = pic->i_id;
//        printk("clear buf[%d]-poc(%d), ref.(%d), keep(%d)\n", pic->i_id, pic->i_poc, pic->b_reference, pic->b_ltr_keep);
    }
    else
    {
        printk(KERN_ERR"vhe-return buffer is null\n");
    }
#endif
    h265->p_recn = NULL;

//    for (i=0;i<h265->i_rpbn;i++)
//    {
//        pic = h265->m_pics+i;
//        printk("[%d<idx-%d>]-poc(%d), ref.(%d), keep(%d)\n", i, pic->i_id, pic->i_poc, pic->b_reference, pic->b_ltr_keep);
//    }
    return idx;
}

int
h265_write_padding_head(void* buff, int len)
{
    bs_t outbs,*bs = &outbs;

    if (len < 6)
        return -1;

    h265_reset_bs(bs, buff, 128);
    bs_write_nal(bs, NUH_PACK(RSV_NVCL41, 0, 0));
    bs_trail(bs);
    h265_flush_bs(bs);

    return h265_count_bs(bs)/8;
}

/* find available picture buffer */
static pic_t* _pic_avail(
    h265_enc*   h265)
{
    int i;
    pic_t* pic = h265->m_pics;

    for (i = 0, pic = h265->m_pics; i < h265->i_rpbn; i++, pic++)
    {
        /* find not short/long term reference buffer */
        if (IS_POC_FREE(pic->i_poc) && !pic->b_reference && !pic->b_cached)
        {
            if (!h265->i_poc && pic->b_ltr_keep)
                printk(KERN_ERR"%s(idx-%d)-ref.(%d)/keep(%d)/cached(%d)\n", __func__, i, pic->b_reference, pic->b_ltr_keep, pic->b_cached);
            return pic;
        }
    }

    printk(KERN_ERR"%s()-found null picture buffer(idx<%d>\n", __func__, i);
    for (i = 0, pic = h265->m_pics; i < MAX_RPB_SIZE; i++, pic++)
    {
        printk(KERN_ERR"m_pics[%d-idx<%d>] poc<%d>, ref.<%d>, keep<%d>, cached(%d)\n", i, pic->i_id, pic->i_poc, pic->b_reference, pic->b_ltr_keep, pic->b_cached);
    }
    return NULL;
}

/* _rps_reset
 * return value is idx of buffer can be freed
 * h265-
 * ipoc-next encode picture poc
 * rpsi-reference picture idx
 * return value: dropped picture
 */
static pic_t* _rps_reset(
    h265_enc*   h265,
    int         ipoc,
    int         rpsi)
{
    pic_t *pic, *out = NULL, *rec = h265->p_recn;
    rps_t* rps;
    short pocs[MAX_RPS_SIZE] = {0};
    int i, j;
    int b_ltr_keep = 0;

    /* get LTR status */
    if (rec)
    {
        b_ltr_keep = rec->b_ltr_keep;
        /* shrink mode: tag cached buffer */
        rec->b_cached = 1;
    }
    /* clear current STR list */
    for (i = 0; i < MAX_REF_SIZE; i++)
    {
        h265->p_rpls[HEVC_REF_ACTIVE_L0][i] = h265->p_rpls[HEVC_REF_ACTIVE_L1][i] = NULL;
    }

    /* clear reference count and buffer ref. status */
    h265->i_active_l0_count = h265->i_active_l1_count = 0;
    h265->i_num_ref_idx_l0_default_active = 1;
    for (pic = h265->m_pics, i = 0; i < h265->i_rpbn; i++, pic++)
    {
        pic->b_reference = 0;
    }

    /* seems not be into this scope */
    if (!ipoc)
    {   /* Intra-frame or IDR-frame */
        for (pic = h265->m_pics, i = 0; i < h265->i_rpbn; i++, pic++)
        {
            pic->b_ltr_keep = 0;
            if (pic->i_poc >= 0)
                out = pic;
        }
        return out;
    }

    /* update LTR ref. list */
    if (b_ltr_keep)
    {
        pic = h265->p_rpls[HEVC_REF_LTR][0];
        /* return last LTR picture */
        if (pic && pic != rec)
        {
            out = pic;
            /* let last LTR's POC equal to POC_BUFFER_FREE to avoid find two picture with the same POC */
            out->i_poc = POC_BUFFER_FREE;
            out->b_ltr_keep = 0;
            out->b_cached = 0;
        }
        /* update LTR list and picture status */
        h265->p_rpls[HEVC_REF_LTR][0] = rec;
        rec->b_reference = 1;
        rec->b_ltr_keep = 1;
    }
    /* find the STR referenced picture */
    rps = &h265->m_sps.rps[rpsi];
    for (i = 0; i < rps->i_num_neg_pics + rps->i_num_pos_pics; i++)
    {
        pic = find_rpb_poc(h265, ipoc + rps->i_dpoc[i]);
        if (!pic)
        {
            printk(KERN_ERR"vhe-%s()null ref. pic and return\n", __func__);
            return pic;
        }
        pocs[i] = ipoc + rps->i_dpoc[i];
        pic->b_reference = 1;
    }
    /* find the return buffer */
    for (pic = h265->m_pics, i = 0; i < h265->i_rpbn; i++, pic++)
    {
        if (!pic->b_reference)
        {
            /* keep LTR picture still be referenced in current frame is STR */
            if (pic->b_ltr_keep)
            {
                pic->b_reference = 1;
            }
            else if (!pic->b_cached)
            {
                continue;
            }
            else if (!out)
            {
                out = pic;
                out->b_cached = 0;
            }
        }
    }
    /* update HEVC_REF_ACTIVE_L0 */
    for (i = j = 0; i < rps->i_num_neg_pics; i++)
        if (rps->b_used[i] && (h265->p_rpls[HEVC_REF_ACTIVE_L0][j] = find_rpb_poc(h265, pocs[i])))
            j++;
    h265->i_active_l0_count = j;
    /* update HEVC_REF_ACTIVE_L1 */
    for (j = 0; i < rps->i_num_neg_pics + rps->i_num_pos_pics; i++)
        if (rps->b_used[i] && (h265->p_rpls[HEVC_REF_ACTIVE_L1][j] = find_rpb_poc(h265, pocs[i])))
            j++;
    h265->i_active_l1_count = j;

    return out;
}

/* initialize buffer, call once before first getbits or showbits */

static
void Initialize_Buffer(base *ld, void *pData,unsigned int Datalength)
{
    ld->Incnt = 0;
    ld->Rdptr = ld->Rdbfr + 32;
    ld->Rdmax = ld->Rdptr;
    ld->Bitcnt = 0;

    ld->Bfr = 0;
    Fill_Buffer(ld,pData,Datalength);
}

int
h265_sh_parser(
    h265_enc* h265)
{
    slice_t *pSh = h265_find_set(h265, HEVC_SLICE, 0);
    sps_t *sps = h265_find_set(h265, HEVC_SPS, 0);
    pps_t *pps = pSh->p_pps;
    NAL_Header NAL;
    NAL_Header *pNal=&NAL;
    int TotalLength = SLICE_HEADER_PARSE_SIZE;
    int used_bits = 0;
    base BitsControl, *ld = &BitsControl;

    Initialize_Buffer(ld, h265->m_hwsh+START_CODE_LEN, TotalLength-START_CODE_LEN);

    /** NAL_Parser **/
    pNal->forbidden_zero_bit =              _get(1,                         "forbidden_zero_bit");
    pNal->nal_unit_type =                   _get(6,                         "nal_unit_type");
    pNal->nuh_layer_id =                    _get(6,                         "nuh_layer_id");
    pNal->nuh_temporal_id_plus1 =           _get(3,                         "nuh_temporal_id_plus1");
    pSh->i_nal_type = pNal->nal_unit_type;

    /** SH_Parser **/
    pSh->first_slice_segment_in_pic_flag =  _get_u1(                        "first_slice_segment_in_pic_flag");

    if (pNal->nal_unit_type >= BLA_W_LP && pNal->nal_unit_type <= RSV_IRAP_VCL23)
        pSh->no_output_of_prior_pics_flag = _get_u1(                        "no_output_of_prior_pics_flag");

    pSh->slice_pic_parameter_set_id =       _get_ue(                        "slice_pic_parameter_set_id");
    pSh->i_slice_type =                     _get_ue(                        "slice_type");

    if (pSh->i_slice_type != SLICE_TYPE_I)
    {
        /* vhe hw fixed poc field length to 8 bits */
//        pSh->i_poc =                        _get(sps->i_log2_max_poc_lsb,   "slice_pic_order_cnt_lsb");
        pSh->i_poc =                        _get(8,                         "slice_pic_order_cnt_lsb");
        pSh->short_term_ref_pic_set_sps_flag = _get_u1(                     "short_term_ref_pic_set_sps_flag");
        pSh->short_term_ref_pic_set_idx =   _get(1,                         "short_term_ref_pic_set_idx");
#if 0
        if(sps->b_long_term_ref_pics_pres)
        {
            pSh->num_long_term_pics =       _get_ue(                        "num_long_term_pics");
            pSh->poc_lsb_lt[0] =            _get(8,                         "poc_lsb_lt[0]");
            pSh->used_by_curr_pic_lt_flag[0] = _get_u1(                     "used_by_curr_pic_lt_flag[0]");
            pSh->delta_poc_msb_present_flag[0] = _get_u1(                   "delta_poc_msb_present_flag[0]");
        }

        if(sps->b_temporal_mvp_enable)
        {
            pSh->slice_temporal_mvp_enabled_flag = _get_u1(                 "slice_temporal_mvp_enabled_flag");
        }
#endif
    }

    if(sps->b_sao_enabled)
    {
        pSh->slice_sao_luma_flag =          _get_u1(                        "slice_sao_luma_flag");
        pSh->slice_sao_chroma_flag =        _get_u1(                        "slice_sao_chroma_flag");
    }

    if (pSh->i_slice_type == SLICE_TYPE_P || pSh->i_slice_type == SLICE_TYPE_B)
    {
        h265->b_active_override =           _get_u1(                        "num_ref_idx_active_override_flag");

        if (pps->b_cabac_init_pres)
        {
            pSh->b_cabac_init =             _get_u1(                        "cabac_init_flag");
        }
        pSh->i_max_num_merge_cand = 5 -     _get_ue(                        "five_minus_max_num_merge_cand");
    }

    pSh->i_qp =                             _get_se(                        "slice_qp_delta") + pps->i_init_qp;

    if(pps->b_deblocking_filter_override_enabled)
    {
        pSh->deblocking_filter_override_flag = _get_u1(                     "deblocking_filter_override_flag");
    }

    if(pSh->deblocking_filter_override_flag)
    {
        pSh->slice_deblocking_filter_disabled_flag = _get_u1(               "slice_deblocking_filter_disabled_flag");
        if(!pSh->slice_deblocking_filter_disabled_flag)
        {
            pSh->slice_beta_offset_div2 =   _get_se(                        "slice_beta_offset_div2");
            pSh->slice_tc_offset_div2 =     _get_se(                        "slice_tc_offset_div2");
        }
    }

    pSh->slice_loop_filter_across_slices_enabled_flag = _get_u1(            "slice_loop_filter_across_slices_enabled_flag");

    used_bits = get_used_bits(ld)-NAL_LEN+ALIGN_EQU_ONE;
    used_bits = ALIGN8(used_bits) + NAL_LEN + 4*8;        // add forbidden and start syntax

    return used_bits/8;
}
