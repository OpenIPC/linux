/*------------------------------------------------------------------------------
--                                                                                                                               --
--       This software is confidential and proprietary and may be used                                   --
--        only as expressly authorized by a licensing agreement from                                     --
--                                                                                                                               --
--                            Verisilicon.                                                                                    --
--                                                                                                                               --
--                   (C) COPYRIGHT 2014 VERISILICON                                                            --
--                            ALL RIGHTS RESERVED                                                                    --
--                                                                                                                               --
--                 The entire notice above must be reproduced                                                 --
--                  on all copies and should not be removed.                                                    --
--                                                                                                                               --
--------------------------------------------------------------------------------*/

#ifndef CABAC_DEFINES_H
#define CABAC_DEFINES_H

#include "base_type.h"
#include "sw_put_bits.h"

struct cabac
{
  struct buffer b;
  u8 ctx[158];
  u8 sao_merge_flag;    /* Table Table 9 5  offsets... */
  u8 sao_type_idx;    /* Table 9-6 */
  u8 split_cu_flag;   /* Table 9-7 */
  u8 cu_transquant_bypass_flag; /* Table 9-8 */
  u8 skip_flag;     /* Table 9-9 */
  u8 cu_qp_delta_abs;   /* Table 9-24 */
  u8 pred_mode_flag;    /* Table 9-10 */
  u8 part_mode;     /* Table 9-11 */
  u8 part_mode_amp;     /* Table non- exist, just consistent with G2*/
  u8 prev_intra_luma_pred_flag; /* Table 9-12 */
  u8 intra_chroma_pred_mode;  /* Table 9-13 */
  u8 merge_flag;      /* Table 9-15 */
  u8 merge_idx;     /* Table 9-16 */
  u8 inter_pred_idc;    /* Table 9-17 */
  u8 ref_idx_l0;      /* Table 9-18 */
  u8 ref_idx_l1;
  u8 abs_mvd_greater0_flag; /* Table 9-23 */
  u8 abs_mvd_greater1_flag;
  u8 mvp_l0_flag;     /* Table 9-19 */
  u8 mvp_l1_flag;
  u8 no_residual_syntax_flag; /* 9-14 */
  u8 split_transform_flag;  /* Table 9-20 */
  u8 cbf_luma;      /* Table 9-21 */
  u8 cbf_ch;      /* Table 9-22 */
  u8 transform_skip_flag_lum; /* Table 9-25 */
  u8 transform_skip_flag_ch;  /* Table 9-25 */
  u8 ls_coeff_x_prefix;   /* Table 9-26 */
  u8 ls_coeff_y_prefix;   /* Table 9-27 */
  u8 coded_sub_block_flag;  /* Table 9-28 */
  u8 significant_coeff_flag;  /* Table 9-29 */
  u8 coeff_abs_level_g1_flag; /* Table 9-30 */
  u8 coeff_abs_level_g2_flag; /* Table 9-31 */
  i32 cod_low;
  i32 cod_range;
  i32 bits_outstanding;
  i32 first_bit;
#ifdef TEST_DATA
  i32 test_bits;
  i32 test_bits_num;
  i32 terminate_flag;
#endif
};

#endif
