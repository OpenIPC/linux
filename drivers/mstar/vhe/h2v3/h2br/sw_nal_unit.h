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

#ifndef SW_NAL_UNIT_H
#define SW_NAL_UNIT_H

#include "base_type.h"
#include "sw_put_bits.h"
#include "enccommon.h"


/* See table Table 7-1 NAL unit type codes and NAL unit type classes */
enum nal_type
{
  TRAIL_N = 0, // 0
  TRAIL_R,     // 1
  TSA_N,       // 2
  TSA_R,       // 3
  STSA_N,      // 4
  STSA_R,      // 5
  RADL_N,      // 6
  RADL_R,      // 7
  RASL_N,      // 8
  RASL_R,      // 9
  RSV_VCL_N10,
  RSV_VCL_R11,
  RSV_VCL_N12,
  RSV_VCL_R13,
  RSV_VCL_N14,
  RSV_VCL_R15,

  BLA_W_LP  = 16,
  BLA_W_RADL  = 17,
  BLA_N_LP  = 18,
  IDR_W_RADL  = 19,
  IDR_N_LP  = 20,
  CRA_NUT = 21,
  RSV_IRAP_VCL22  = 22,
  RSV_IRAP_VCL23  = 23,
  VPS_NUT   = 32,
  SPS_NUT   = 33,
  PPS_NUT   = 34,
  EOS_NUT      = 36,
  EOB_NUT     = 37,
  FD_NUT      = 38,
  PREFIX_SEI_NUT = 39,
  SUFFIX_SEI_NUT = 40,

  /* Reference picture sets uses same store than vps/sps/vps */
  RPS   = 64
};

struct nal_unit
{
  enum nal_type type;
  i32 temporal_id;
};

void nal_unit(struct buffer *, struct nal_unit *nal_unit);
void HevcNalUnitHdr(struct buffer *stream, enum nal_type nalType,
                    true_e byteStream);


#endif
