#include <mfe6_def.h>
#include <h264_enc.h>

static void write_ebsp(bs_t* bs, uchar byte)
{
    if (bs->ebsp_zeros == 2 && byte < 4)
    {
        *bs->curr++ = 0x03;
        bs->ebsp_zeros = 0;
        bs->bits_pos += 8;
    }
    if (byte == 0)
        bs->ebsp_zeros++;
    else
        bs->ebsp_zeros = 0;
    *bs->curr++ = byte;
}

int avc_reset(bs_t* bs, uchar* buff, int len)
{
    bs->bits_pos = 0;
    bs->left_count = 16;
    bs->code_bits = 0;
    bs->curr = bs->buffer = buff;
    bs->length = len;
    return 0;
}

int avc_flush(bs_t* bs)
{
    while (bs->left_count < 16)
    {
        write_ebsp(bs, (uchar)(bs->code_bits>>8));
        bs->left_count += 8;
        bs->code_bits <<= 8;
    }
    bs->left_count = 16;
    bs->code_bits = 0;
    return bs->bits_pos;
}

int avc_count(bs_t* bs)
{
    return bs->bits_pos;
}

static int bs_write(bs_t* bs, int len, uint bit)
{
    if (bs->left_count > len)
    {
        bs->left_count -= len;
        bs->code_bits += bit << bs->left_count;
    }
    else
    {
        int left = (16 - len + bs->left_count);
        uint pattern = (bs->code_bits << 16) + (bit << left);
        bs->left_count = left;
        bs->code_bits = pattern & 0xFFFF;
        write_ebsp(bs, (uchar)(pattern>>24));
        write_ebsp(bs, (uchar)(pattern>>16));
    }
    bs->bits_pos += len;
    return len;
}

static int bs_align(bs_t* bs, int bit)
{
    int  size = bs->left_count&0x7;
    uint code = bit?(1<<size)-1:0;
    if (size > 0)
        bs_write(bs, size, code);
    return size;
}

static const uchar ue_code_lengths[256] = {
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

static int bs_write_ue(bs_t* bs, uint val)
{
    return bs_write(bs, (int)ue_code_lengths[val+1], val+1);
}

static int bs_write_se(bs_t* bs, int val)
{
    int size = 0;
    int tmp = 1 - 2*val;
    if (tmp < 0) tmp = 2*val;
    val = tmp;
    if (tmp >= 256)
    {
        size = 16;
        tmp >>= 8;
    }
    size += ue_code_lengths[tmp];
    return bs_write(bs, size, (uint)val);
}

static int bs_write_u1(bs_t* bs, uint val)
{
    return bs_write(bs, 1, val!=0);
}

static int bs_write_32(bs_t* bs, uint val)
{
    int len;
    len = bs_write(bs, 16, (val>>16)&0xFFFF);
    len += bs_write(bs, 16, val&0xFFFF);
    return len;
}

static int bs_trail(bs_t* bs)
{
    int len;

    bs->left_count--;
    bs->code_bits += 1 << bs->left_count;

    if (!bs->left_count)
    {
        write_ebsp(bs, (uchar)(bs->code_bits>>8));
        write_ebsp(bs, (uchar)(bs->code_bits));
        bs->left_count = 16;
        bs->code_bits = 0;
    }
    bs->bits_pos += 8;
    bs->bits_pos &= ~0x7;

    len = 1 + (bs->left_count&0x7);

    bs->left_count &= ~7;

    return len;
}

int avc_write_nal(bs_t* bs, uchar nal)
{
    *bs->curr++ = 0;
    *bs->curr++ = 0;
    *bs->curr++ = 0;
    *bs->curr++ = 1;
    *bs->curr++ = nal;
    bs->bits_pos += 8*5;
    bs->left_count = 16;
    bs->code_bits = 0;

    return 8*5;
}

static int vui_write_param(bs_t* bs, vui_t* vui);

int avc_write_sps(bs_t* bs, sps_t* sps)
{
    int bit_pos = avc_count(bs);
    /* profile_idc */
    bs_write(bs, 8, (uint)sps->i_profile_idc);
    /* constraint_set0_flag */
    bs_write_u1(bs, sps->b_constraint_set0);
    /* constraint_set1_flag */
    bs_write_u1(bs, sps->b_constraint_set1);
    /* constraint_set2_flag */
    bs_write_u1(bs, sps->b_constraint_set2);
    /* constraint_set3_flag */
    bs_write_u1(bs, sps->b_constraint_set3);
    /* constraint_set4_flag u(1) */
    /* constraint_set5_flag u(1) */
    /*reserved_zero_2bits (equal to 0) */
    bs_write(bs, 4, 0);
    /* level_idc */
    bs_write(bs, 8, (uint)sps->i_level_idc);
    /* seq_parameter_set_id */
    bs_write_ue(bs, sps->i_sps_id);
#if 0 /* not support high profile */
    /* chroma_format_idc */
    if (sps->i_profile_idc > PROFILE_HIGH)
    {
        chroma_format_idc
        ....
    }
#endif
    /* log2_max_frame_num_minus4 */
    bs_write_ue(bs, sps->i_log2_max_frame_num - 4);
    /* pic_order_cnt_type */
    bs_write_ue(bs, sps->i_poc_type);
    if (sps->i_poc_type == 0)
    {
        /* log2_max_pic_order_cnt_lsb_minus4 */
        bs_write_ue(bs, sps->i_log2_max_poc_lsb - 4);
    }
    /* not support poc_type == 1 // B-slice used
    else if (sps->i_poc_type == 1)
    {
      ....
    }
    */
    /* max_num_ref_frames */
    bs_write_ue(bs, sps->i_num_ref_frames);
    /* gaps_in_frame_num_value_allowed_flag */
    bs_write_u1(bs, sps->b_gaps_in_frame_num_value_allow);
    /* pic_width_in_mbs_minus1 */
    bs_write_ue(bs, sps->i_mb_w - 1);
    /* pic_height_in_map_units_minus1 */
    bs_write_ue(bs, sps->i_mb_h - 1);
    /* frame_mbs_only_flag */
    bs_write_u1(bs, 1/*b_frame_mbs_only*/);
    /* only support frame_mbs */
    /* direct_8x8_inference_flag */
    bs_write_u1(bs, sps->b_direct8x8_inference);
    /* frame_cropping_flag */
    bs_write_u1(bs, sps->b_crop);
    if (sps->b_crop)
    {
        /* frame_crop_left_offset */
        bs_write_ue(bs, sps->crop.i_left);
        /* frame_crop_right_offset */
        bs_write_ue(bs, sps->crop.i_right);
        /* frame_crop_top_offset */
        bs_write_ue(bs, sps->crop.i_top);
        /* frame_crop_bottom_offset */
        bs_write_ue(bs, sps->crop.i_bottom);
    }
    /* vui_parameters_present_flag */
    bs_write_u1(bs, sps->b_vui_param_pres);
    if (sps->b_vui_param_pres)
        vui_write_param(bs, &sps->vui);
    bs_trail(bs);

    return bs->bits_pos - bit_pos;
}

static int vui_write_param(bs_t* bs, vui_t* vui)
{
    int bit_pos = avc_count(bs);

    /* aspect_ratio_info_present_flag */
    bs_write_u1(bs, 0);
    /* overscan_info_present_flag */
    bs_write_u1(bs, 0);
    /* video_signal_type_present_flag */
    bs_write_u1(bs, vui->b_video_signal_pres);
    if (vui->b_video_signal_pres)
    {
        /* video_format */
        bs_write(bs, 3, vui->i_video_format);
        /* video_full_range_flag */
        bs_write_u1(bs, vui->b_video_full_range);
        /* colour_description_present_flag */
        bs_write_u1(bs, vui->b_colour_desc_pres);
        if (vui->b_colour_desc_pres)
        {
            /* colour_primaries */
            bs_write(bs, 8, vui->i_colour_primaries);
            /* transfer_characteristics */
            bs_write(bs, 8, vui->i_transf_character);
            /* matrix_coefficients */
            bs_write(bs, 8, vui->i_matrix_coeffs);
        }
    }
    /* chroma_loc_info_present_flag */
    bs_write_u1(bs, 0);
    if (vui->b_timing_info_pres)
    {
        /* timing_info_present_flag */
        bs_write_u1(bs, 1);
        /* num_units_in_tick */
        bs_write_32(bs, vui->i_num_units_in_tick);
        /* time_scale */
        bs_write_32(bs, vui->i_time_scale);
        /* fixed_frame_rate_flag */
        bs_write_u1(bs, vui->b_fixed_frame_rate);
    }
    else
    {
        /* timing_info_present_flag */
        bs_write_u1(bs, 0);
    }
    /* nal_hrd_parameters_present_flag */
    bs_write_u1(bs, 0);
    /* vcl_hrd_parameters_present_flag */
    bs_write_u1(bs, 0);
    /* pic_struct_present_flag */
    bs_write_u1(bs, 0);
    /* bitstream_restriction_flag */
    bs_write_u1(bs, 0);
    return bs->bits_pos - bit_pos;
}

int avc_write_pps(bs_t* bs, pps_t* pps)
{
    int bit_pos = avc_count(bs);
    /* pic_parameter_set_id */
    bs_write_ue(bs, pps->i_pps_id);
    /* seq_parameter_set_id */
    bs_write_ue(bs, pps->sps->i_sps_id);
    /* entropy_coding_mode_flag */
    bs_write_u1(bs, pps->b_cabac);
    /* bottom_field_pic_order_in_frame_present_flag */
    bs_write_u1(bs, 0/*b_bottom_field_pic_order*/);
    /* num_slice_groups_minus1 */
    bs_write_ue(bs, 0/*i_num_slice_groups - 1*/);
    /* num_ref_idx_l0_default_active_minus1 */
    bs_write_ue(bs, pps->i_num_ref_idx_l0_default_active - 1);
    /* num_ref_idx_l1_default_active_minus1 */
    bs_write_ue(bs, pps->i_num_ref_idx_l1_default_active - 1);
    /* not support weighted pred */
    /* weighted_pred_flag */
    bs_write_u1(bs, 0/*b_weighted_pred*/);
    /* weighted_bipred_idc */
    bs_write(bs, 2, 0/*b_weighted_bipred*/);
    /* pic_init_qp_minus26 */
    bs_write_se(bs, pps->i_pic_init_qp - 26);
    /* pic_init_qs_minus26 */
    bs_write_se(bs, pps->i_pic_init_qs - 26);
    /* chroma_qp_index_offset */
    bs_write_se(bs, pps->i_cqp_idx_offset);
    /* deblocking_filter_control_present_flag */
    bs_write_u1(bs, pps->b_deblocking_filter_control);
    /* constrained_intra_pred_flag */
    bs_write_u1(bs, pps->b_constrained_intra_pred);
    /* redundant_pic_cnt_present_flag */
    bs_write_u1(bs, pps->b_redundant_pic_cnt);
    /* not support transform8x8/q-matrix */
    bs_trail(bs);

    return bs->bits_pos - bit_pos;
}

int avc_write_slice_header(bs_t* bs, slice_t* sh)
{
    /* first_mb_in_slice */
    bs_write_ue(bs, sh->i_first_mb);
    /* slice_type */
    bs_write_ue(bs, sh->i_type);
    /* pic_parameter_set_id */
    bs_write_ue(bs, sh->pps->i_pps_id);
    /* frame_num */
    bs_write(bs, sh->sps->i_log2_max_frame_num, (uint)sh->i_frm_num & ((1 << sh->sps->i_log2_max_frame_num) - 1));

    /* idr_pic_id */
    if (sh->b_idr_pic)
        bs_write_ue(bs, sh->i_idr_pid);
    /* pic_order_cnt_type */
    if (sh->sps->i_poc_type == 0)
        bs_write(bs, sh->sps->i_log2_max_poc_lsb, (uint)sh->i_poc & ((1 << sh->sps->i_log2_max_poc_lsb) - 1));
    /* redundant_pic_cnt */
    if (sh->pps->b_redundant_pic_cnt)
        bs_write_ue(bs, sh->i_redundant_pic_cnt);
    /* direct_spatial_mv_pred_flag */
    if (sh->i_type == SLICE_B)
        bs_write_u1(bs, sh->b_direct_spatial_mv_pred);
    if (sh->i_type == SLICE_P || sh->i_type == SLICE_B)
    {
        /* num_ref_idx_active_override_flag */
        bs_write_u1(bs, sh->b_num_ref_idx_override);
        if (sh->b_num_ref_idx_override)
        {
            /* num_ref_idx_l0_active_minus1 */
            bs_write_ue(bs, sh->i_num_ref_idx_l0_active - 1);
            /* num_ref_idx_l1_active_minus1 */
            if (sh->i_type == SLICE_B)
                bs_write_ue(bs, sh->i_num_ref_idx_l1_active - 1);
        }
    }
    /* ref_pic_list_modification() */
    if (sh->i_type != SLICE_I)
    {
        /* LTR is not enable */
        if (!sh->b_long_term_reference)
        {
            /* ref_pic_list_modification_flag_l0 */
            bs_write_u1(bs, 0);
        }
        else
        {
            int i;
            /* ref_pic_list_modification_flag_l0 */
            bs_write_u1(bs, sh->b_ref_pic_list_modification_flag_l0);
            /* fill ref pic list if LTR P-frame */
            if (sh->b_ref_pic_list_modification_flag_l0)
            {
                for (i = 0; i < MAX_MULTI_REF_FRAME_PLUS1; i++)
                {
                    /* modification_of_pic_nums_idc */
                    bs_write_ue(bs, sh->rpl0_t[i].modification_of_pic_nums_idc);
                    if (sh->rpl0_t[i].modification_of_pic_nums_idc == 0 ||
                        sh->rpl0_t[i].modification_of_pic_nums_idc == 1)
                    {
                        /* abs_diff_pic_num_minus1 */
                        bs_write_ue(bs, sh->rpl0_t[i].abs_diff_pic_num_minus1);
                    }
                    else if (sh->rpl0_t[i].modification_of_pic_nums_idc == 2)
                    {
                        /* long_term_pic_num */
                        bs_write_ue(bs, sh->rpl0_t[i].long_term_pic_num);
                    }
                    else if (sh->rpl0_t[i].modification_of_pic_nums_idc == 4 ||
                             sh->rpl0_t[i].modification_of_pic_nums_idc == 5)
                    {
                        /* abs_diff_view_idx_minus1 */
                        bs_write_ue(bs, sh->rpl0_t[i].abs_diff_view_idx_minus1);
                    }
                    else if (sh->rpl0_t[i].modification_of_pic_nums_idc == 3)
                    {
                        break;
                    }
                }
            }
        }
    }
    /* ref_pic_list_modification_flag_l1 */
    if (sh->i_type == SLICE_B)
        bs_write_u1(bs, 0);
    /* not support weighted pred */
    /* dec_ref_pic_marking() */
    if (sh->i_ref_idc)
    {
        /* I-frame */
        if (sh->b_idr_pic)
        {
            /* no_output_prior_pics_flag */
            bs_write_u1(bs, 0);
            /* long_term_reference_flag */
            if (!sh->b_long_term_reference)
                bs_write_u1(bs, 0);
            else
                bs_write_u1(bs, 1);
        }
        else
        {
            int i;
            /* adaptive_ref_pic_marking_mode_flag */
            if(!sh->b_adaptive_ref_pic_marking_mode)
                bs_write_u1(bs, 0);
            else
            {
                bs_write_u1(bs, 1);
                for (i = 0; i < MAX_MULTI_REF_FRAME_PLUS1; i++)
                {
                    /* memory_management_control_operation */
                    bs_write_ue(bs, sh->mmc_t[i].memory_management_control);
                    if(sh->mmc_t[i].memory_management_control == 1 ||
                       sh->mmc_t[i].memory_management_control == 3)
                    {
                        /* difference_of_pic_nums_minus1 */
                        bs_write_ue(bs, sh->mmc_t[i].difference_of_pic_nums_minus1);
                    }
                    if(sh->mmc_t[i].memory_management_control == 2)
                    {
                        /* ong_term_pic_num */
                        bs_write_ue(bs, sh->mmc_t[i].long_term_pic_num);
                    }
                    if(sh->mmc_t[i].memory_management_control == 3 ||
                       sh->mmc_t[i].memory_management_control == 6)
                    {
                        /* long_term_frame_idx */
                        bs_write_ue(bs, sh->mmc_t[i].long_term_frame_idx);
                    }
                    if(sh->mmc_t[i].memory_management_control == 4)
                    {
                        /* max_long_term_frame_idx_plus1 */
                        bs_write_ue(bs, sh->mmc_t[i].max_long_term_frame_idx_plus1);
                    }
                }
            }
        }
    }
    /* cabac_init_idc */
    if (sh->pps->b_cabac && sh->i_type != SLICE_I)
        bs_write_ue(bs, sh->i_cabac_init_idc);
    /* slice_qp_delta */
    bs_write_se(bs, sh->i_qp - sh->pps->i_pic_init_qp);
    if (sh->pps->b_deblocking_filter_control)
    {
        /* disable_deblocking_filter_idc */
        bs_write_ue(bs, sh->i_disable_deblocking_filter_idc);
        if (sh->i_disable_deblocking_filter_idc != 1)
        {
            /* slice_alpha_c0_offset_div2 */
            bs_write_se(bs, sh->i_alpha_c0_offset_div2);
            /* slice_beta_offset_div2 */
            bs_write_se(bs, sh->i_beta_offset_div2);
        }
    }
    if (sh->pps->b_cabac)
        bs_align(bs, 1);

    return bs->bits_pos;
}
