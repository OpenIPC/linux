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

#ifndef SW_PUT_BITS_H
#define SW_PUT_BITS_H

#include "base_type.h"
#include "test_data_define.h"


struct buffer
{
#ifdef TEST_DATA
  struct stream_trace *stream_trace;
  u32 input_cabac_BIN_number;
  u32 input_bypass_BIN_number;
#endif
  u8 *stream;   /* get_buffer(): stream = hevc_nal->nal */
  u32 *cnt;   /* get_buffer(): cnt    = &hevc_nal->cnt */
  u32 size;   /* get_buffer(): size of stream buffer */
  u32 cache;    /* Next bits before flushing to stream */
  u32 bit_cnt;    /* Bit count of above cache */
  u32 stream_bus;   /* get_buffer(): stream = hevc_nal->nal */
  u32 byteCnt;    /* Byte counter */
  u32 overflow;       /*show if bitstream buffer is overflow*/
  u32 emulCnt; /* Counter for emulation_3_byte, needed in SEI */
};

void put_bit(struct buffer *, i32, i32);
void put_bit_32(struct buffer *b, i32 value, i32 number);
i32 buffer_full(struct buffer *b);
void put_bit_ue(struct buffer *b, i32 val);
void put_bit_se(struct buffer *b, i32 val);
void rbsp_trailing_bits(struct buffer *);
void put_bits_startcode(struct buffer *b);
void rbsp_flush_bits(struct buffer *b);


#endif
