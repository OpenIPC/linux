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

#include "comm/drv_lmt.h"

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

// Encode DC out is 8 word alignment
#define JPEG_ENC_DCOUT_Y_LINEOFFSET_ALIGN       DRV_LIMIT_ALIGN_8WORD       // Encode DC out Y  line offset alignment
#define JPEG_ENC_DCOUT_UV_LINEOFFSET_ALIGN      DRV_LIMIT_ALIGN_8WORD       // Encode DC out UV line offset alignment

// Encode normal mode is word alignment
#define JPEG_ENC_Y_LINEOFFSET_ALIGN             DRV_LIMIT_ALIGN_WORD        // Encode Y  line offset alignment
#define JPEG_ENC_UV_LINEOFFSET_ALIGN            DRV_LIMIT_ALIGN_WORD        // Encode UV line offset alignment

//@}

#endif
