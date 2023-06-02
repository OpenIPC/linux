
/**
 * @file kdrv_affine_lmt.h
 * @brief parameter limitation of KDRV Affine
 * @author ESW
 * @date in the year 2019
 */

#ifndef _KDRV_AFFINE_LIMIT_H_
#define _KDRV_AFFINE_LIMIT_H_

#include "comm/drv_lmt.h"
#include "kdrv_type.h"

#define AFFINE_WIDTH_MAX         ((1 << 10) * 16)
#define AFFINE_HEIGHT_MAX        ((1 << 10) * 8)
#define AFFINE_LINEOFFSET_MAX    ((1<<16) - 1)
#define __ALIGN_FLOOR_8(a)       ((a) & ~0x07)
#define __ALIGN_FLOOR_16(a)      ((a) & ~0x0F)

//=====================================================
// input limitation for Y-8bit
#define AFFINE_SRCBUF_Y8BIT_WMIN        (16)                                // unit: byte
#define AFFINE_SRCBUF_Y8BIT_WMAX        __ALIGN_FLOOR_16(AFFINE_WIDTH_MAX)  // unit: byte
#define AFFINE_SRCBUF_Y8BIT_WALIGN      (DRV_LIMIT_ALIGN_4WORD)             // unit: byte

#define AFFINE_SRCBUF_Y8BIT_HMIN        (8)                                 // unit: line
#define AFFINE_SRCBUF_Y8BIT_HMAX        __ALIGN_FLOOR_8(AFFINE_HEIGHT_MAX)  // unit: line
#define AFFINE_SRCBUF_Y8BIT_HALIGN      (DRV_LIMIT_ALIGN_2WORD)             // unit: line

#define AFFINE_SRCBUF_Y8BIT_ADDR_ALIGN  DRV_LIMIT_ALIGN_WORD                // unit: byte

//=====================================================
// input limitation for UV-packing
#define AFFINE_SRCBUF_UVP_WMIN         (16)                                 // unit: byte
#define AFFINE_SRCBUF_UVP_WMAX         __ALIGN_FLOOR_16(AFFINE_WIDTH_MAX)   // unit: byte
#define AFFINE_SRCBUF_UVP_WALIGN       (DRV_LIMIT_ALIGN_4WORD)              // unit: byte

#define AFFINE_SRCBUF_UVP_HMIN         (8)                                  // unit: line
#define AFFINE_SRCBUF_UVP_HMAX         __ALIGN_FLOOR_8(AFFINE_HEIGHT_MAX)   // unit: line
#define AFFINE_SRCBUF_UVP_HALIGN       (DRV_LIMIT_ALIGN_2WORD)              // unit: line

#define AFFINE_SRCBUF_UVP_ADDR_ALIGN   DRV_LIMIT_ALIGN_WORD                 // unit: byte

//=====================================================
// output limitation for Y-8bit
#define AFFINE_DSTBUF_Y8BIT_WMIN        (16)                                // unit: byte
#define AFFINE_DSTBUF_Y8BIT_WMAX        __ALIGN_FLOOR_16(AFFINE_WIDTH_MAX)  // unit: byte
#define AFFINE_DSTBUF_Y8BIT_WALIGN      (DRV_LIMIT_ALIGN_4WORD)             // unit: byte

#define AFFINE_DSTBUF_Y8BIT_HMIN        (8)                                 // unit: line
#define AFFINE_DSTBUF_Y8BIT_HMAX        __ALIGN_FLOOR_8(AFFINE_HEIGHT_MAX)  // unit: line
#define AFFINE_DSTBUF_Y8BIT_HALIGN      (DRV_LIMIT_ALIGN_2WORD)             // unit: line

#define AFFINE_DSTBUF_Y8BIT_ADDR_ALIGN  DRV_LIMIT_ALIGN_WORD                // unit: byte

//=====================================================
// output limitation for UV-packing
#define AFFINE_DSTBUF_UVP_WMIN         (16)                                 // unit: byte
#define AFFINE_DSTBUF_UVP_WMAX         __ALIGN_FLOOR_16(AFFINE_WIDTH_MAX)   // unit: byte
#define AFFINE_DSTBUF_UVP_WALIGN       (DRV_LIMIT_ALIGN_4WORD)              // unit: byte

#define AFFINE_DSTBUF_UVP_HMIN         (8)                                  // unit: line
#define AFFINE_DSTBUF_UVP_HMAX         __ALIGN_FLOOR_8(AFFINE_HEIGHT_MAX)   // unit: line
#define AFFINE_DSTBUF_UVP_HALIGN       (DRV_LIMIT_ALIGN_2WORD)              // unit: line

#define AFFINE_DSTBUF_UVP_ADDR_ALIGN   DRV_LIMIT_ALIGN_WORD                 // unit: byte

#endif


