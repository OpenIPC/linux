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

#ifndef SW_SLICE_H
#define SW_SLICE_H

#include "base_type.h"
#include "queue.h"
#include "sw_put_bits.h"
#include "sw_nal_unit.h"
#include "sw_picture.h"
#include "sw_cabac_defines.h"

enum slice_type
{
  B_SLICE,
  P_SLICE,
  I_SLICE
};

struct slice
{
  struct node *next;
  struct nal_unit nal_unit;
  struct cabac cabac;
  struct cabac cabit;
  struct cabac cabacStore[RECON_PIPELINE_CTU_NUM];

  enum slice_type type;
  i32 ctb_per_slice;      /* Ctb count of this slice */
  i32 write_header;
  i32 dependent_slice_flag;
  i32 pic_output_flag;
  i32 sao_luma_flag;
  i32 sao_chroma_flag;
  i32 cb_qp_offset;
  i32 cr_qp_offset;
  i32 max_num_merge_cand;
  i32 loop_filter_across_slices_enabled_flag;
  i32 deblocking_filter_override_flag;
  i32 deblocking_filter_disabled_flag;
  i32 beta_offset;
  i32 tc_offset;
  i32 cabac_init_flag;
  i32 slice_address;      /* First ctb->num in slice */
  i32 num;        /* Slice number */
  i32 prev_qp;

  i32 active_override_flag;   /* TODO */
  i32 active_l0_cnt;
  i32 active_l1_cnt;

  //ref pic lists modification
  u32 ref_pic_list_modification_flag_l0;
  u32 list_entry_l0[2];
  u32 ref_pic_list_modification_flag_l1;
  u32 list_entry_l1[2];
};

typedef struct
{
  struct nal_unit nal_unit;
  struct cabac cabac;
  enum slice_type type;
  i32 ctb_per_slice;      /* Ctb count of this slice */
  i32 write_header;
  i32 dependent_slice_flag;
  i32 pic_output_flag;
  i32 sao_luma_flag;
  i32 sao_chroma_flag;
  i32 cb_qp_offset;
  i32 cr_qp_offset;
  i32 max_num_merge_cand;
  i32 loop_filter_across_slices_enabled_flag;
  i32 deblocking_filter_override_flag;
  i32 deblocking_filter_disabled_flag;
  i32 beta_offset;
  i32 tc_offset;
  i32 cabac_init_flag;
  i32 slice_address;      /* First ctb->num in slice */
  i32 num;        /* Slice number */
  i32 prev_qp;

  i32 active_override_flag;   /* TODO */
  i32 active_l0_cnt;
  i32 active_l1_cnt;

} slice_s;


struct slice *sw_create_slice(i32 num);
struct slice *sw_get_slice(struct queue *q, i32 num);
void sw_free_slices(struct queue *q);

#endif
