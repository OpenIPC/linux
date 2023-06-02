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

#include "../comm/drv_lmt.h"

/**
    @addtogroup mIDriver
*/
//@{

// Maximum, minimum bitstream length
#define JPEG_BITSTREAM_LENGTH_MIN               0x00000100                  // Minimum bitstream length (256 bytes)
#define JPEG_BITSTREAM_LENGTH_MAX               (0x02000000 - 1)            // Maximum bitstream length (32M - 1 bytes)

// Maximum, minimum image width / height
#define JPEG_W_MAX                              0x00010000                  // Maximum width (65536 pixels)
#define JPEG_H_MAX                              0x00010000                  // Maximum height (65536 pixels)

// Width, height alignment (MCU alignment)
// MCU 2h11 (422)
#define JPEG_W_ALIGN_MCU_2H11                   DRV_LIMIT_ALIGN_4WORD       // MCU 2h11 (422) width  alignment
#define JPEG_H_ALIGN_MCU_2H11                   DRV_LIMIT_ALIGN_2WORD       // MCU 2h11 (422) height alignment

// MCU 411 (420)
#define JPEG_W_ALIGN_MCU_411                    DRV_LIMIT_ALIGN_4WORD       // MCU 411 (420) width  alignment
#define JPEG_H_ALIGN_MCU_411                    DRV_LIMIT_ALIGN_4WORD       // MCU 411 (420) height alignment


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
