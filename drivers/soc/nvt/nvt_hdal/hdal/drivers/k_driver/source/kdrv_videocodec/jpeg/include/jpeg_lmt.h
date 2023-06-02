/*
    JPEG driver limitation header file.

    JPEG driver limitation of NT96650.

    @file       jpeg_lmt.h
    @ingroup    mIDriver
    @note       Nothing

    Copyright   Novatek Microelectronics Corp. 2012.  All rights reserved.
*/

#ifndef _JPEG_LMT_H
#define _JPEG_LMT_H

#if defined __UITRON || defined __ECOS
#include "drv_lmt.h"
#else
//#include "sys/drv_lmt.h"
#endif

#define DRV_LIMIT_ALIGN_BYTE        0x00000001      // Byte alignment
#define DRV_LIMIT_ALIGN_HALFWORD    0x00000002      // Half-word (2 Bytes) alignment
#define DRV_LIMIT_ALIGN_WORD        0x00000004      //   Word  ( 4 bytes) alignment
#define DRV_LIMIT_ALIGN_2WORD       0x00000008      // 2 words ( 8 bytes) alignment
#define DRV_LIMIT_ALIGN_4WORD       0x00000010      // 4 words (16 bytes) alignment
#define DRV_LIMIT_ALIGN_8WORD       0x00000020      // 8 words (32 bytes) alignment



/**
    @addtogroup mIDriver
*/
//@{

// Maximum, minimum bitstream length
#define JPEG_BITSTREAM_LENGTH_MIN               0x00000100                  // Minimum bitstream length (256 bytes)
#define JPEG_BITSTREAM_LENGTH_MAX               (0x02000000 - 1)            // Maximum bitstream length (32M - 1 bytes)

// Maximum, minimum image width / height
#define JPEG_W_MAX                              0x00010000                  // Maximum width  (65536 pixels)
#define JPEG_H_MAX                              0x00010000                  // Maximum height (65536 pixels)

// Width, height alignment (MCU alignment)
// MCU 2h11 (422)
#define JPEG_W_ALIGN_MCU_2H11                   DRV_LIMIT_ALIGN_4WORD       // MCU 2h11 (422) width  alignment
#define JPEG_H_ALIGN_MCU_2H11                   DRV_LIMIT_ALIGN_2WORD       // MCU 2h11 (422) height alignment

// MCU 411 (420)
#define JPEG_W_ALIGN_MCU_411                    DRV_LIMIT_ALIGN_4WORD       // MCU 411 (420) width  alignment
#define JPEG_H_ALIGN_MCU_411                    DRV_LIMIT_ALIGN_4WORD       // MCU 411 (420) height alignment

// MCU 2h11 (422), 411(420) only (For all encode mode)
// Note: For others MCU format, the alignment is word alignment,
//       but we only support 2h11 and 411 now.
// Encode Y, UV address alignment
#define JPEG_ENC_Y_ADDR_ALIGN                   DRV_LIMIT_ALIGN_BYTE        // Encode Y  address alignment
#define JPEG_ENC_UV_ADDR_ALIGN                  DRV_LIMIT_ALIGN_HALFWORD    // Encode UV address alignemnt

// Decode Y, UV address alignment (For all decode mode)
#define JPEG_DEC_Y_ADDR_ALIGN                   DRV_LIMIT_ALIGN_WORD        // Decode Y  address alignment
#define JPEG_DEC_UV_ADDR_ALIGN                  DRV_LIMIT_ALIGN_WORD        // Decode UV address alignemnt

// Line offset alignment
// Decode scaling is 8 word alignment
#define JPEG_DEC_SCALING_Y_LINEOFFSET_ALIGN     DRV_LIMIT_ALIGN_8WORD       // Decode scaling Y  line offset alignment
#define JPEG_DEC_SCALING_UV_LINEOFFSET_ALIGN    DRV_LIMIT_ALIGN_8WORD       // Decode scaling UV line offset alignment

// Decode other mode is word alignment
#define JPEG_DEC_Y_LINEOFFSET_ALIGN             DRV_LIMIT_ALIGN_WORD        // Decode Y  line offset alignment
#define JPEG_DEC_UV_LINEOFFSET_ALIGN            DRV_LIMIT_ALIGN_WORD        // Decode UV line offset alignment

// Encode DC out is 8 word alignment
#define JPEG_ENC_DCOUT_Y_LINEOFFSET_ALIGN       DRV_LIMIT_ALIGN_8WORD       // Encode DC out Y  line offset alignment
#define JPEG_ENC_DCOUT_UV_LINEOFFSET_ALIGN      DRV_LIMIT_ALIGN_8WORD       // Encode DC out UV line offset alignment

// Encode normal mode is word alignment
#define JPEG_ENC_Y_LINEOFFSET_ALIGN             DRV_LIMIT_ALIGN_WORD        // Encode Y  line offset alignment
#define JPEG_ENC_UV_LINEOFFSET_ALIGN            DRV_LIMIT_ALIGN_WORD        // Encode UV line offset alignment

// Cropping X, Y alignment (MCU alignment)
// MCU 2h11 (422)
#define JPEG_DEC_CROP_X_ALIGN_MCU_2H11          DRV_LIMIT_ALIGN_4WORD       // MCU 2h11 (422) Decode cropping X alignment
#define JPEG_DEC_CROP_Y_ALIGN_MCU_2H11          DRV_LIMIT_ALIGN_2WORD       // MCU 2h11 (422) Decode cropping Y alignment

// MCU 411 (420)
#define JPEG_DEC_CROP_X_ALIGN_MCU_411           DRV_LIMIT_ALIGN_4WORD       // MCU 411 (420) Decode cropping X alignment
#define JPEG_DEC_CROP_Y_ALIGN_MCU_411           DRV_LIMIT_ALIGN_4WORD       // MCU 411 (420) Decode cropping Y alignment

// Cropping Width, height alignment (MCU alignment)
// MCU 2h11 (422)
#define JPEG_DEC_CROP_W_ALIGN_MCU_2H11          DRV_LIMIT_ALIGN_4WORD       // MCU 2h11 (422) Decode cropping width  alignment
#define JPEG_DEC_CROP_H_ALIGN_MCU_2H11          DRV_LIMIT_ALIGN_2WORD       // MCU 2h11 (422) Decode cropping height alignment

// MCU 411 (420)
#define JPEG_DEC_CROP_W_ALIGN_MCU_411           DRV_LIMIT_ALIGN_4WORD       // MCU 411 (420) Decode cropping width  alignment
#define JPEG_DEC_CROP_H_ALIGN_MCU_411           DRV_LIMIT_ALIGN_4WORD       // MCU 411 (420) Decode cropping height alignment

//@}

#endif
