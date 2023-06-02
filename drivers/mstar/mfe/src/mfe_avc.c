
#include <types.h>

#include <mfe_avc.h>

static void write_ebsp(bs_t* bs, uchar byte) {
    if (bs->ebsp_zeros == 2 && byte < 4) {
        *bs->curr++ = 0x03;
        bs->ebsp_zeros = 0;
        bs->bits_pos += 8;
    }
    if (byte == 0) {
        bs->ebsp_zeros++;
    }
    *bs->curr++ = byte;
}

int avc_reset(bs_t* bs, uchar* buff, int len) {
    bs->bits_pos = 0;
    bs->left_count = 16;
    bs->code_bits = 0;
    bs->curr = bs->buffer = buff;
    bs->length = len;
    return 0;
}

int avc_flush(bs_t* bs) {

    while (bs->left_count < 16) {
        write_ebsp(bs, (uchar)(bs->code_bits>>8));
        bs->left_count += 8;
        bs->code_bits <<= 8;
    }
    bs->left_count = 16;
    bs->code_bits = 0;
    return bs->bits_pos;
}

int avc_count(bs_t* bs) {
    return bs->bits_pos;
}

static int bs_write(bs_t* bs, int len, uint bit) {
    if (bs->left_count > len) {
        bs->left_count -= len;
        bs->code_bits += bit << bs->left_count;
    } else {
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

static int bs_write_ue(bs_t* bs, int val) {
    return bs_write(bs, (int)ue_code_lengths[val+1], (uint)val+1);
}

static int bs_write_se(bs_t* bs, int val) {
    int size = 0;
    int tmp = 1 - 2*val;
    if (tmp < 0) tmp = 2*val;
    val = tmp;
    if (tmp >= 256) {
        size = 16;
        tmp >>= 8;
    }
    size += ue_code_lengths[tmp];
    return bs_write(bs, size, (uint)val);
}

static int bs_write_u1(bs_t* bs, int val) {
    return bs_write(bs, 1, (uint)(val!=0));
}

static int bs_trail(bs_t* bs) {
    int len;

    bs->left_count--;
    bs->code_bits += 1 << bs->left_count;

    if (!bs->left_count) {
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

int avc_write_nal(bs_t* bs, uchar nal) {
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

int avc_write_sps(bs_t* bs, sps_t* sps) {
    int bit_pos = avc_count(bs);
    bs_write(bs, 8, (uint)sps->i_profile_idc);
    bs_write_u1(bs, sps->b_constraint_set0);
    bs_write_u1(bs, sps->b_constraint_set1);
    bs_write_u1(bs, sps->b_constraint_set2);
    bs_write_u1(bs, sps->b_constraint_set3);
    bs_write(bs, 4, 0);

    bs_write(bs, 8, (uint)sps->i_level_idc);
    bs_write_ue(bs, sps->i_sps_id);
    /* not support high profile
    if (sps->i_profile_idc > PROFILE_HIGH) {
        chroma_format_idc
        ....
    }
    */
    bs_write_ue(bs, sps->i_log2_max_frame_num - 4);
    bs_write_ue(bs, sps->i_poc_type);
    if (sps->i_poc_type == 0) {
        bs_write_ue(bs, sps->i_log2_max_poc_lsb - 4);
    }
    /* not support poc_type == 1
    else if (sps->i_poc_type == 1) {
      ....
    }
    */
    bs_write_ue(bs, sps->i_num_ref_frames);
    bs_write_u1(bs, sps->b_gaps_in_frame_num_value_allow);
    bs_write_ue(bs, sps->i_mb_w - 1);
    bs_write_ue(bs, sps->i_mb_h - 1);
    bs_write_u1(bs, 1/*b_frame_mbs_only*/);
    /* only support frame_mbs */
    bs_write_u1(bs, sps->b_direct8x8_inference);
    bs_write_u1(bs, sps->b_crop);
    if (sps->b_crop) {
        bs_write_ue(bs, sps->crop.i_left);
        bs_write_ue(bs, sps->crop.i_right);
        bs_write_ue(bs, sps->crop.i_top);
        bs_write_ue(bs, sps->crop.i_bottom);
    }
    bs_write_u1(bs, 0); /* skip vui params */
    bs_trail(bs);

    return bs->bits_pos - bit_pos;
}

int avc_write_pps(bs_t* bs, pps_t* pps) {
    int bit_pos = avc_count(bs);
    bs_write_ue(bs, pps->i_pps_id);
    bs_write_ue(bs, pps->sps->i_sps_id);
    bs_write_u1(bs, pps->b_cabac);
    bs_write_u1(bs, 0/*b_bottom_field_pic_order*/);
    bs_write_ue(bs, 0/*i_num_slice_groups - 1*/);
    bs_write_ue(bs, pps->i_num_ref_idx_l0_default_active - 1);
    bs_write_ue(bs, pps->i_num_ref_idx_l1_default_active - 1);
    /* not support weighted pred */
    bs_write_u1(bs, 0/*b_weighted_pred*/);
    bs_write(bs, 2, 0/*b_weighted_bipred*/);
    bs_write_se(bs, pps->i_pic_init_qp - 26);
    bs_write_se(bs, pps->i_pic_init_qs - 26);
    bs_write_se(bs, 0/*i_chroma_qp_index_offset*/);
    bs_write_u1(bs, pps->b_deblocking_filter_control);
    bs_write_u1(bs, pps->b_constrained_intra_pred);
    bs_write_u1(bs, pps->b_redundant_pic_cnt);
    /* not support transform8x8/q-matrix */
    bs_trail(bs);

    return bs->bits_pos - bit_pos;
}

int avc_write_slice_header(bs_t* bs, slice_t* sh) {
    bs_write_ue(bs, sh->i_first_mb);
    bs_write_ue(bs, sh->i_type);
    bs_write_ue(bs, sh->pps->i_pps_id);
    bs_write(bs, sh->sps->i_log2_max_frame_num, (uint)sh->i_frame_num & ((1 << sh->sps->i_log2_max_frame_num) - 1));

    if (sh->b_idr_pic) {
        bs_write_ue(bs, sh->i_idr_pic_id);
    }
    if (sh->sps->i_poc_type == 0) {
        bs_write(bs, sh->sps->i_log2_max_poc_lsb, (uint)sh->i_poc & ((1 << sh->sps->i_log2_max_poc_lsb) - 1));
    }
    if (sh->pps->b_redundant_pic_cnt) {
        bs_write_ue(bs, sh->i_redundant_pic_cnt);
    }
    if (sh->i_type == SLICE_B) {
        bs_write_u1(bs, sh->b_direct_spatial_mv_pred);
    }
    if (sh->i_type == SLICE_P || sh->i_type == SLICE_B) {
        bs_write_u1(bs, sh->b_num_ref_idx_override);
        if (sh->b_num_ref_idx_override) {
            bs_write_ue(bs, sh->i_num_ref_idx_l0_active - 1);
            if (sh->i_type == SLICE_B) {
                bs_write_ue(bs, sh->i_num_ref_idx_l1_active - 1);
            }
        }
    }
    /* not support ref_pic_list_reordering */
    if (sh->i_type != SLICE_I) {
        bs_write_u1(bs, 0);
    }
    if (sh->i_type == SLICE_B) {
        bs_write_u1(bs, 0);
    }
    /* not support weighted pred */
    /* not support dec_ref_pic_marking */
    if (sh->i_ref_idc) {
        if (sh->b_idr_pic) {
            bs_write_u1(bs, 0/*no_output_prior_pics_flag*/);
            bs_write_u1(bs, 0/*long_term_reference_flag*/);
        } else {
            bs_write_u1(bs, 0/*adaptive_ref_pic_marking_mode_flag*/);
        }
    }
    if (sh->pps->b_cabac && sh->i_type == SLICE_I) {
        bs_write_ue(bs, sh->i_cabac_init_idc);
    }
    bs_write_se(bs, sh->i_qp - sh->pps->i_pic_init_qp);
    if (sh->pps->b_deblocking_filter_control) {
        bs_write_ue(bs, sh->i_disable_deblocking_filter_idc);
        if (sh->i_disable_deblocking_filter_idc != 1) {
            bs_write_se(bs, sh->i_alpha_c0_offset >> 1);
            bs_write_se(bs, sh->i_beta_offset >> 1);
        }
    }

    return bs->bits_pos;
}
