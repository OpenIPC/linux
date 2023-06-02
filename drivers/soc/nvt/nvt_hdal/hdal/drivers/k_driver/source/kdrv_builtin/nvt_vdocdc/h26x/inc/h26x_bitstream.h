/* ///////////////////////////////////////////////////////////// */
/*   File: bitstream.h                                           */
/*   Author: Jerry Peng                                          */
/*   Date: Apr/28/2005                                           */
/* ------------------------------------------------------------- */
/*   ISO MPEG-4 bitstream read/write module.                     */
/*                                                               */
/*   Copyright, 2004-2005.                                       */
/*   Copyright, 2005.                                            */
/*   NOVATEK MICROELECTRONICS CORP.                              */
/*   NO.1-2 Innovation Road 1, Hsinchu Science Park,             */
/*   HsinChu 300 Taiwan, R.O.C.                                  */
/*   E-mail : jerry_peng@novatek.com.tw                          */
/* ///////////////////////////////////////////////////////////// */

#ifndef __BITSTREAM_H26X_H__
#define __BITSTREAM_H26X_H__

#include "kwrap/type.h"

#ifndef __CC_ARM
    #define __CC_ARM    0
#endif

///////////////////////////////////////////////////////////////////
//                   STRUCTURE DECLARATION                       //
///////////////////////////////////////////////////////////////////
typedef struct 
{
    UINT32 *start;
    UINT32 *tail;
    UINT32  bufl;
    UINT32  bufh;
    UINT32  offset;
    UINT32  length;

    UINT32  uiCurrPos;  // @chingho 03 10 2009
    UINT32  uiWordAlignedOffset;

    /* coding info */
    UINT8  *buf;
}bstream;

///////////////////////////////////////////////////////////////////
//                   STRUCTURE DECLARATION                       //
///////////////////////////////////////////////////////////////////

#define MPEG4               4
#define MPEG2               2

#define BS_ERROR     -1
#define BS_OK         0

#define IS_BYTEALIGNED(BS) ((BS->offset % 8) == 0)

#define CEILING_DIV(NUM, DIV) (((NUM)+(DIV)-1) / (DIV))

#if __CC_ARM
#define BSWAP(a)    a = (((((a) & 0xff000000) >> 24) | (((a) & 0x00ff0000) >>  8) | \
                          (((a) & 0x0000ff00) <<  8) | (((a) & 0x000000ff) << 24)))
#else
//#define BSWAP(a) __asm mov eax,a __asm bswap eax __asm mov a, eax
#define BSWAP(a)    a = (((((a) & 0xff000000) >> 24) | (((a) & 0x00ff0000) >>  8) | \
                          (((a) & 0x0000ff00) <<  8) | (((a) & 0x000000ff) << 24)))
#endif

extern void    init_parse_bitstream(bstream * const bs, void * const bitbuffer, UINT32 length);
extern void    init_pack_bitstream(bstream * const bs, void * const bitbuffer, UINT32 length);
extern UINT32  show_bits_bytealign(bstream * const bs, const UINT32 bits);
extern void    move_bytealign_noforcestuffing(bstream * const bs);
extern void    move_nextbyte(bstream * const bs);
extern void    bs_pad(bstream * const bs, const UINT32 stuff_bit);
extern UINT32  bs_pad_ch(bstream * const bs, const UINT32 video_type);
extern UINT32  bs_byte_align_zero_padding(bstream * const bs);
extern UINT32  byte_length(bstream * const bs);
extern UINT32  bit_length(bstream * const bs);
extern UINT32  bs_byte_length_p(bstream * const bs);
extern UINT32  bs_byte_length(bstream * const bs);
extern UINT32  bs_len (bstream * const bs, const UINT32 stuff_bit);
extern UINT32 put_bits(bstream * const bs, const UINT32 value, const UINT32 size);
extern UINT32 write_uvlc_codeword(bstream *const bs, UINT32 value);
extern UINT32 get_bits(bstream *const bs, const UINT32 bits);
extern UINT32 read_uvlc_codeword(bstream *const bs);
extern INT32 read_signed_uvlc_codeword(bstream *const bs);
extern int read_rbsp_trailing_bits(bstream *const bs);
extern int count_bits(UINT32 code);
extern int write_signed_uvlc_codeword(bstream *const bs, int value);
extern int write_rbsp_trailing_bits(bstream *const bs);

#endif//__BITSTREAM_H26X_H__

