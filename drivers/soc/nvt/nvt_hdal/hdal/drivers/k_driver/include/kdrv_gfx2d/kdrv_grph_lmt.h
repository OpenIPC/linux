/**
 * @file kdrv_grph_lmt.h
 * @brief parameter limitation of KDRV GRPH.
 * @author ESW
 * @date in the year 2019
 */

#ifndef __KDRV_GRPH_LIMIT_H__
#define __KDRV_GRPH_LIMIT_H__

#include "comm/drv_lmt.h"
#include "kdrv_type.h"

// NOT refer by external (grph_lmt.h internal)
#define GRPH_WIDTH_HEIGHT_MAX       ((1<<14) - 1)
#define GRPH_LINEOFFSET_MAX         ((1<<16) - 1)
#define __ALIGN_FLOOR_2(a)          ((a) & ~0x01)
#define __ALIGN_FLOOR_4(a)          ((a) & ~0x03)
#define __ALIGN_FLOOR_8(a)          ((a) & ~0x07)
#define __ALIGN_FLOOR_16(a)         ((a) & ~0x0F)

// ALL OP
#define GRPH_ALL_LOFF_MAX           __ALIGN_FLOOR_4(GRPH_LINEOFFSET_MAX)    // unit: byte
#define GRPH_ALL_LOFF_ALIGN         (DRV_LIMIT_ALIGN_WORD)                  // unit: byte

// GOP0
/*
#define GRPH_GOP0_4BITS_WMIN        (8)                                     // unit: byte
#define GRPH_GOP0_4BITS_WMAX        __ALIGN_FLOOR_8(GRPH_WIDTH_HEIGHT_MAX)  // unit: byte
#define GRPH_GOP0_4BITS_WALIGN      (DRV_LIMIT_ALIGN_2WORD)                 // unit: byte
#define GRPH_GOP0_4BITS_HMIN        (16)                                    // unit: line
#define GRPH_GOP0_4BITS_HMAX        __ALIGN_FLOOR_16(GRPH_WIDTH_HEIGHT_MAX) // unit: line
#define GRPH_GOP0_4BITS_HALIGN      (DRV_LIMIT_ALIGN_4WORD)                 // unit: line
#define GRPH_GOP0_4BITS_ADDR_ALIGN  (DRV_LIMIT_ALIGN_WORD)                  // unit: byte
*/

#define GRPH_GOP0_8BITS_WMIN        (8)                                     // unit: byte
#define GRPH_GOP0_8BITS_WMAX        __ALIGN_FLOOR_8(GRPH_WIDTH_HEIGHT_MAX)  // unit: byte
#define GRPH_GOP0_8BITS_WALIGN      (DRV_LIMIT_ALIGN_2WORD)                 // unit: byte
#define GRPH_GOP0_8BITS_HMIN        (8)                                     // unit: line
#define GRPH_GOP0_8BITS_HMAX        __ALIGN_FLOOR_8(GRPH_WIDTH_HEIGHT_MAX)  // unit: line
#define GRPH_GOP0_8BITS_HALIGN      (DRV_LIMIT_ALIGN_2WORD)                 // unit: line
#define GRPH_GOP0_8BITS_ADDR_ALIGN  (DRV_LIMIT_ALIGN_WORD)                  // unit: byte

#define GRPH_GOP0_16BITS_WMIN       (16)                                    // unit: byte
#define GRPH_GOP0_16BITS_WMAX       __ALIGN_FLOOR_16(GRPH_WIDTH_HEIGHT_MAX) // unit: byte
#define GRPH_GOP0_16BITS_WALIGN     (DRV_LIMIT_ALIGN_4WORD)                 // unit: byte
#define GRPH_GOP0_16BITS_HMIN       (4)                                     // unit: line
#define GRPH_GOP0_16BITS_HMAX       __ALIGN_FLOOR_4(GRPH_WIDTH_HEIGHT_MAX)  // unit: line
#define GRPH_GOP0_16BITS_HALIGN     (DRV_LIMIT_ALIGN_WORD)                  // unit: line
#define GRPH_GOP0_16BITS_ADDR_ALIGN (DRV_LIMIT_ALIGN_WORD)                  // unit: byte

#define GRPH_GOP0_32BITS_WMIN       (GRPH_GOP0_16BITS_WMIN)
#define GRPH_GOP0_32BITS_WMAX       (GRPH_GOP0_16BITS_WMAX)
#define GRPH_GOP0_32BITS_WALIGN     (GRPH_GOP0_16BITS_WALIGN)
#define GRPH_GOP0_32BITS_HMIN       (GRPH_GOP0_16BITS_HMIN)
#define GRPH_GOP0_32BITS_HMAX       (GRPH_GOP0_16BITS_HMAX)
#define GRPH_GOP0_32BITS_HALIGN     (GRPH_GOP0_16BITS_HALIGN)
#define GRPH_GOP0_32BITS_ADDR_ALIGN (GRPH_GOP0_16BITS_ADDR_ALIGN)

// GOP1
/*
#define GRPH_GOP1_4BITS_WMIN        (GRPH_GOP0_4BITS_WMIN)
#define GRPH_GOP1_4BITS_WMAX        (GRPH_GOP0_4BITS_WMAX)
#define GRPH_GOP1_4BITS_WALIGN      (GRPH_GOP0_4BITS_WALIGN)
#define GRPH_GOP1_4BITS_HMIN        (GRPH_GOP0_4BITS_HMIN)
#define GRPH_GOP1_4BITS_HMAX        (GRPH_GOP0_4BITS_HMAX)
#define GRPH_GOP1_4BITS_HALIGN      (GRPH_GOP0_4BITS_HALIGN)
#define GRPH_GOP1_4BITS_ADDR_ALIGN  (GRPH_GOP0_4BITS_ADDR_ALIGN)
*/

#define GRPH_GOP1_8BITS_WMIN        (GRPH_GOP0_8BITS_WMIN)
#define GRPH_GOP1_8BITS_WMAX        (GRPH_GOP0_8BITS_WMAX)
#define GRPH_GOP1_8BITS_WALIGN      (GRPH_GOP0_8BITS_WALIGN)
#define GRPH_GOP1_8BITS_HMIN        (GRPH_GOP0_8BITS_HMIN)
#define GRPH_GOP1_8BITS_HMAX        (GRPH_GOP0_8BITS_HMAX)
#define GRPH_GOP1_8BITS_HALIGN      (GRPH_GOP0_8BITS_HALIGN)
#define GRPH_GOP1_8BITS_ADDR_ALIGN  (GRPH_GOP0_8BITS_ADDR_ALIGN)

#define GRPH_GOP1_16BITS_WMIN       (GRPH_GOP0_16BITS_WMIN)
#define GRPH_GOP1_16BITS_WMAX       (GRPH_GOP0_16BITS_WMAX)
#define GRPH_GOP1_16BITS_WALIGN     (GRPH_GOP0_16BITS_WALIGN)
#define GRPH_GOP1_16BITS_HMIN       (GRPH_GOP0_16BITS_HMIN)
#define GRPH_GOP1_16BITS_HMAX       (GRPH_GOP0_16BITS_HMAX)
#define GRPH_GOP1_16BITS_HALIGN     (GRPH_GOP0_16BITS_HALIGN)
#define GRPH_GOP1_16BITS_ADDR_ALIGN (GRPH_GOP0_16BITS_ADDR_ALIGN)

#define GRPH_GOP1_32BITS_WMIN       (GRPH_GOP0_16BITS_WMIN)
#define GRPH_GOP1_32BITS_WMAX       (GRPH_GOP0_16BITS_WMAX)
#define GRPH_GOP1_32BITS_WALIGN     (GRPH_GOP0_16BITS_WALIGN)
#define GRPH_GOP1_32BITS_HMIN       (GRPH_GOP0_16BITS_HMIN)
#define GRPH_GOP1_32BITS_HMAX       (GRPH_GOP0_16BITS_HMAX)
#define GRPH_GOP1_32BITS_HALIGN     (GRPH_GOP0_16BITS_HALIGN)
#define GRPH_GOP1_32BITS_ADDR_ALIGN (GRPH_GOP0_16BITS_ADDR_ALIGN)

// GOP2
/*
#define GRPH_GOP2_4BITS_WMIN        (GRPH_GOP0_4BITS_WMIN)
#define GRPH_GOP2_4BITS_WMAX        (GRPH_GOP0_4BITS_WMAX)
#define GRPH_GOP2_4BITS_WALIGN      (GRPH_GOP0_4BITS_WALIGN)
#define GRPH_GOP2_4BITS_HMIN        (GRPH_GOP0_4BITS_HMIN)
#define GRPH_GOP2_4BITS_HMAX        (GRPH_GOP0_4BITS_HMAX)
#define GRPH_GOP2_4BITS_HALIGN      (GRPH_GOP0_4BITS_HALIGN)
#define GRPH_GOP2_4BITS_ADDR_ALIGN  (GRPH_GOP0_4BITS_ADDR_ALIGN)
*/

#define GRPH_GOP2_8BITS_WMIN        (GRPH_GOP0_8BITS_WMIN)
#define GRPH_GOP2_8BITS_WMAX        (GRPH_GOP0_8BITS_WMAX)
#define GRPH_GOP2_8BITS_WALIGN      (GRPH_GOP0_8BITS_WALIGN)
#define GRPH_GOP2_8BITS_HMIN        (GRPH_GOP0_8BITS_HMIN)
#define GRPH_GOP2_8BITS_HMAX        (GRPH_GOP0_8BITS_HMAX)
#define GRPH_GOP2_8BITS_HALIGN      (GRPH_GOP0_8BITS_HALIGN)
#define GRPH_GOP2_8BITS_ADDR_ALIGN  (GRPH_GOP0_8BITS_ADDR_ALIGN)

#define GRPH_GOP2_16BITS_WMIN       (GRPH_GOP0_16BITS_WMIN)
#define GRPH_GOP2_16BITS_WMAX       (GRPH_GOP0_16BITS_WMAX)
#define GRPH_GOP2_16BITS_WALIGN     (GRPH_GOP0_16BITS_WALIGN)
#define GRPH_GOP2_16BITS_HMIN       (GRPH_GOP0_16BITS_HMIN)
#define GRPH_GOP2_16BITS_HMAX       (GRPH_GOP0_16BITS_HMAX)
#define GRPH_GOP2_16BITS_HALIGN     (GRPH_GOP0_16BITS_HALIGN)
#define GRPH_GOP2_16BITS_ADDR_ALIGN (GRPH_GOP0_16BITS_ADDR_ALIGN)

#define GRPH_GOP2_32BITS_WMIN       (GRPH_GOP0_16BITS_WMIN)
#define GRPH_GOP2_32BITS_WMAX       (GRPH_GOP0_16BITS_WMAX)
#define GRPH_GOP2_32BITS_WALIGN     (GRPH_GOP0_16BITS_WALIGN)
#define GRPH_GOP2_32BITS_HMIN       (GRPH_GOP0_16BITS_HMIN)
#define GRPH_GOP2_32BITS_HMAX       (GRPH_GOP0_16BITS_HMAX)
#define GRPH_GOP2_32BITS_HALIGN     (GRPH_GOP0_16BITS_HALIGN)
#define GRPH_GOP2_32BITS_ADDR_ALIGN (GRPH_GOP0_16BITS_ADDR_ALIGN)

// GOP3
/*
#define GRPH_GOP3_4BITS_WMIN        (GRPH_GOP0_4BITS_WMIN)
#define GRPH_GOP3_4BITS_WMAX        (GRPH_GOP0_4BITS_WMAX)
#define GRPH_GOP3_4BITS_WALIGN      (GRPH_GOP0_4BITS_WALIGN)
#define GRPH_GOP3_4BITS_HMIN        (GRPH_GOP0_4BITS_HMIN)
#define GRPH_GOP3_4BITS_HMAX        (GRPH_GOP0_4BITS_HMAX)
#define GRPH_GOP3_4BITS_HALIGN      (GRPH_GOP0_4BITS_HALIGN)
#define GRPH_GOP3_4BITS_ADDR_ALIGN  (GRPH_GOP0_4BITS_ADDR_ALIGN)
*/

#define GRPH_GOP3_8BITS_WMIN        (GRPH_GOP0_8BITS_WMIN)
#define GRPH_GOP3_8BITS_WMAX        (GRPH_GOP0_8BITS_WMAX)
#define GRPH_GOP3_8BITS_WALIGN      (GRPH_GOP0_8BITS_WALIGN)
#define GRPH_GOP3_8BITS_HMIN        (GRPH_GOP0_8BITS_HMIN)
#define GRPH_GOP3_8BITS_HMAX        (GRPH_GOP0_8BITS_HMAX)
#define GRPH_GOP3_8BITS_HALIGN      (GRPH_GOP0_8BITS_HALIGN)
#define GRPH_GOP3_8BITS_ADDR_ALIGN  (GRPH_GOP0_8BITS_ADDR_ALIGN)

#define GRPH_GOP3_16BITS_WMIN       (GRPH_GOP0_16BITS_WMIN)
#define GRPH_GOP3_16BITS_WMAX       (GRPH_GOP0_16BITS_WMAX)
#define GRPH_GOP3_16BITS_WALIGN     (GRPH_GOP0_16BITS_WALIGN)
#define GRPH_GOP3_16BITS_HMIN       (GRPH_GOP0_16BITS_HMIN)
#define GRPH_GOP3_16BITS_HMAX       (GRPH_GOP0_16BITS_HMAX)
#define GRPH_GOP3_16BITS_HALIGN     (GRPH_GOP0_16BITS_HALIGN)
#define GRPH_GOP3_16BITS_ADDR_ALIGN (GRPH_GOP0_16BITS_ADDR_ALIGN)

#define GRPH_GOP3_32BITS_WMIN       (GRPH_GOP0_16BITS_WMIN)
#define GRPH_GOP3_32BITS_WMAX       (GRPH_GOP0_16BITS_WMAX)
#define GRPH_GOP3_32BITS_WALIGN     (GRPH_GOP0_16BITS_WALIGN)
#define GRPH_GOP3_32BITS_HMIN       (GRPH_GOP0_16BITS_HMIN)
#define GRPH_GOP3_32BITS_HMAX       (GRPH_GOP0_16BITS_HMAX)
#define GRPH_GOP3_32BITS_HALIGN     (GRPH_GOP0_16BITS_HALIGN)
#define GRPH_GOP3_32BITS_ADDR_ALIGN (GRPH_GOP0_16BITS_ADDR_ALIGN)

// GOP4
/*
#define GRPH_GOP4_4BITS_WMIN        (GRPH_GOP0_4BITS_WMIN)
#define GRPH_GOP4_4BITS_WMAX        (GRPH_GOP0_4BITS_WMAX)
#define GRPH_GOP4_4BITS_WALIGN      (GRPH_GOP0_4BITS_WALIGN)
#define GRPH_GOP4_4BITS_HMIN        (GRPH_GOP0_4BITS_HMIN)
#define GRPH_GOP4_4BITS_HMAX        (GRPH_GOP0_4BITS_HMAX)
#define GRPH_GOP4_4BITS_HALIGN      (GRPH_GOP0_4BITS_HALIGN)
#define GRPH_GOP4_4BITS_ADDR_ALIGN  (GRPH_GOP0_4BITS_ADDR_ALIGN)
*/

#define GRPH_GOP4_8BITS_WMIN        (GRPH_GOP0_8BITS_WMIN)
#define GRPH_GOP4_8BITS_WMAX        (GRPH_GOP0_8BITS_WMAX)
#define GRPH_GOP4_8BITS_WALIGN      (GRPH_GOP0_8BITS_WALIGN)
#define GRPH_GOP4_8BITS_HMIN        (GRPH_GOP0_8BITS_HMIN)
#define GRPH_GOP4_8BITS_HMAX        (GRPH_GOP0_8BITS_HMAX)
#define GRPH_GOP4_8BITS_HALIGN      (GRPH_GOP0_8BITS_HALIGN)
#define GRPH_GOP4_8BITS_ADDR_ALIGN  (GRPH_GOP0_8BITS_ADDR_ALIGN)

#define GRPH_GOP4_16BITS_WMIN       (GRPH_GOP0_16BITS_WMIN)
#define GRPH_GOP4_16BITS_WMAX       (GRPH_GOP0_16BITS_WMAX)
#define GRPH_GOP4_16BITS_WALIGN     (GRPH_GOP0_16BITS_WALIGN)
#define GRPH_GOP4_16BITS_HMIN       (GRPH_GOP0_16BITS_HMIN)
#define GRPH_GOP4_16BITS_HMAX       (GRPH_GOP0_16BITS_HMAX)
#define GRPH_GOP4_16BITS_HALIGN     (GRPH_GOP0_16BITS_HALIGN)
#define GRPH_GOP4_16BITS_ADDR_ALIGN (GRPH_GOP0_16BITS_ADDR_ALIGN)

#define GRPH_GOP4_32BITS_WMIN       (GRPH_GOP0_16BITS_WMIN)
#define GRPH_GOP4_32BITS_WMAX       (GRPH_GOP0_16BITS_WMAX)
#define GRPH_GOP4_32BITS_WALIGN     (GRPH_GOP0_16BITS_WALIGN)
#define GRPH_GOP4_32BITS_HMIN       (GRPH_GOP0_16BITS_HMIN)
#define GRPH_GOP4_32BITS_HMAX       (GRPH_GOP0_16BITS_HMAX)
#define GRPH_GOP4_32BITS_HALIGN     (GRPH_GOP0_16BITS_HALIGN)
#define GRPH_GOP4_32BITS_ADDR_ALIGN (GRPH_GOP0_16BITS_ADDR_ALIGN)

#define GRPH_GOP8_8BITS_WMIN        (4)
#define GRPH_GOP8_8BITS_WMAX        (6528)
#define GRPH_GOP8_8BITS_WALIGN      (2)
#define GRPH_GOP8_8BITS_HMIN        (GRPH_GOP0_8BITS_HMIN)
#define GRPH_GOP8_8BITS_HMAX        (GRPH_GOP0_8BITS_HMAX)
#define GRPH_GOP8_8BITS_HALIGN      (2)
#define GRPH_GOP8_8BITS_ADDR_ALIGN  (GRPH_GOP0_8BITS_ADDR_ALIGN)

#define GRPH_GOP8_16BITS_WMIN       (4)
#define GRPH_GOP8_16BITS_WMAX       (6528)
#define GRPH_GOP8_16BITS_WALIGN     (2)
#define GRPH_GOP8_16BITS_HMIN       (GRPH_GOP0_16BITS_HMIN)
#define GRPH_GOP8_16BITS_HMAX       (GRPH_GOP0_16BITS_HMAX)
#define GRPH_GOP8_16BITS_HALIGN     (1)
#define GRPH_GOP8_16BITS_ADDR_ALIGN (GRPH_GOP0_8BITS_ADDR_ALIGN)

// AOP0
#define GRPH_AOP0_8BITS_WMIN        (4)                                     // unit: byte
#define GRPH_AOP0_8BITS_WMAX        (GRPH_WIDTH_HEIGHT_MAX)                 // unit: byte
#define GRPH_AOP0_8BITS_WALIGN      (0)                                     // unit: byte
#define GRPH_AOP0_8BITS_HMIN        (1)                                     // unit: line
#define GRPH_AOP0_8BITS_HMAX        (GRPH_WIDTH_HEIGHT_MAX)                 // unit: line
#define GRPH_AOP0_8BITS_HALIGN      (0)                                     // unit: line
#define GRPH_AOP0_8BITS_ADDR_ALIGN  (0)                                     // unit: byte

#define GRPH_AOP0_16BITS_WMIN       (4)                                     // unit: byte
#define GRPH_AOP0_16BITS_WMAX       (GRPH_WIDTH_HEIGHT_MAX)                 // unit: byte
#define GRPH_AOP0_16BITS_WALIGN     (0)                                     // unit: byte
#define GRPH_AOP0_16BITS_HMIN       (1)                                     // unit: line
#define GRPH_AOP0_16BITS_HMAX       (GRPH_WIDTH_HEIGHT_MAX)                 // unit: line
#define GRPH_AOP0_16BITS_HALIGN     (0)                                     // unit: line
#define GRPH_AOP0_16BITS_ADDR_ALIGN (0)                                     // unit: byte

// AOP0 32 bit not available in Graphic2
#define GRPH_AOP0_32BITS_WMIN       (4)                                     // unit: byte
#define GRPH_AOP0_32BITS_WMAX       __ALIGN_FLOOR_4(GRPH_WIDTH_HEIGHT_MAX)  // unit: byte
#define GRPH_AOP0_32BITS_WALIGN     (DRV_LIMIT_ALIGN_WORD)                  // unit: byte
#define GRPH_AOP0_32BITS_HMIN       (1)                                     // unit: line
#define GRPH_AOP0_32BITS_HMAX       (GRPH_WIDTH_HEIGHT_MAX)                 // unit: line
#define GRPH_AOP0_32BITS_HALIGN     (0)                                     // unit: line
#define GRPH_AOP0_32BITS_ADDR_ALIGN (DRV_LIMIT_ALIGN_WORD)                  // unit: byte

// AOP1
#define GRPH_AOP1_8BITS_WMIN            (4)                                 // unit: byte
#define GRPH_AOP1_8BITS_WMAX            (GRPH_WIDTH_HEIGHT_MAX)             // unit: byte
#define GRPH_AOP1_8BITS_WALIGN          (0)                                 // unit: byte
#define GRPH_AOP1_8BITS_HMIN            (1)                                 // unit: line
#define GRPH_AOP1_8BITS_HMAX            (GRPH_WIDTH_HEIGHT_MAX)             // unit: line
#define GRPH_AOP1_8BITS_HALIGN          (0)                                 // unit: line
#define GRPH_AOP1_8BITS_ADDR_ALIGN      (0)                                 // unit: byte

#define GRPH_AOP1_16BITS_WMIN           (4)                                 // unit: byte
#define GRPH_AOP1_16BITS_WMAX           __ALIGN_FLOOR_2(GRPH_WIDTH_HEIGHT_MAX)  // unit: byte
#define GRPH_AOP1_16BITS_WALIGN         (DRV_LIMIT_ALIGN_HALFWORD)          // unit: byte
#define GRPH_AOP1_16BITS_HMIN           (1)                                 // unit: line
#define GRPH_AOP1_16BITS_HMAX           (GRPH_WIDTH_HEIGHT_MAX)             // unit: line
#define GRPH_AOP1_16BITS_HALIGN         (0)                                 // unit: line
#define GRPH_AOP1_16BITS_ADDR_ALIGN     (DRV_LIMIT_ALIGN_HALFWORD)          // unit: byte

#define GRPH_AOP1_16BITSUV_WMIN         (4)                                 // unit: byte
#define GRPH_AOP1_16BITSUV_WMAX         __ALIGN_FLOOR_2(GRPH_WIDTH_HEIGHT_MAX)  // unit: byte
#define GRPH_AOP1_16BITSUV_WALIGN       (DRV_LIMIT_ALIGN_HALFWORD)          // unit: byte
#define GRPH_AOP1_16BITSUV_HMIN         (1)                                 // unit: line
#define GRPH_AOP1_16BITSUV_HMAX         (GRPH_WIDTH_HEIGHT_MAX)             // unit: line
#define GRPH_AOP1_16BITSUV_HALIGN       (0)                                 // unit: line
#define GRPH_AOP1_16BITSUV_ADDR_ALIGN   (0)                                 // unit: byte

// AOP2
#define GRPH_AOP2_8BITS_WMIN            (GRPH_AOP1_8BITS_WMIN)
#define GRPH_AOP2_8BITS_WMAX            (GRPH_AOP1_8BITS_WMAX)
#define GRPH_AOP2_8BITS_WALIGN          (GRPH_AOP1_8BITS_WALIGN)
#define GRPH_AOP2_8BITS_HMIN            (GRPH_AOP1_8BITS_HMIN)
#define GRPH_AOP2_8BITS_HMAX            (GRPH_AOP1_8BITS_HMAX)
#define GRPH_AOP2_8BITS_HALIGN          (GRPH_AOP1_8BITS_HALIGN)
#define GRPH_AOP2_8BITS_ADDR_ALIGN      (GRPH_AOP1_8BITS_ADDR_ALIGN)

#define GRPH_AOP2_16BITS_WMIN           (GRPH_AOP1_16BITS_WMIN)
#define GRPH_AOP2_16BITS_WMAX           (GRPH_AOP1_16BITS_WMAX)
#define GRPH_AOP2_16BITS_WALIGN         (GRPH_AOP1_16BITS_WALIGN)
#define GRPH_AOP2_16BITS_HMIN           (GRPH_AOP1_16BITS_HMIN)
#define GRPH_AOP2_16BITS_HMAX           (GRPH_AOP1_16BITS_HMAX)
#define GRPH_AOP2_16BITS_HALIGN         (GRPH_AOP1_16BITS_HALIGN)
#define GRPH_AOP2_16BITS_ADDR_ALIGN     (GRPH_AOP1_16BITS_ADDR_ALIGN)

#define GRPH_AOP2_16BITSUV_WMIN         (GRPH_AOP1_16BITSUV_WMIN)
#define GRPH_AOP2_16BITSUV_WMAX         (GRPH_AOP1_16BITSUV_WMAX)
#define GRPH_AOP2_16BITSUV_WALIGN       (GRPH_AOP1_16BITSUV_WALIGN)
#define GRPH_AOP2_16BITSUV_HMIN         (GRPH_AOP1_16BITSUV_HMIN)
#define GRPH_AOP2_16BITSUV_HMAX         (GRPH_AOP1_16BITSUV_HMAX)
#define GRPH_AOP2_16BITSUV_HALIGN       (GRPH_AOP1_16BITSUV_HALIGN)
#define GRPH_AOP2_16BITSUV_ADDR_ALIGN   (GRPH_AOP1_16BITSUV_ADDR_ALIGN)

// AOP3
#define GRPH_AOP3_8BITS_WMIN            (4)                                 // unit: byte
#define GRPH_AOP3_8BITS_WMAX            (GRPH_WIDTH_HEIGHT_MAX)             // unit: byte
#define GRPH_AOP3_8BITS_WALIGN          (0)                                 // unit: byte
#define GRPH_AOP3_8BITS_HMIN            (1)                                 // unit: line
#define GRPH_AOP3_8BITS_HMAX            (GRPH_WIDTH_HEIGHT_MAX)             // unit: line
#define GRPH_AOP3_8BITS_HALIGN          (0)                                 // unit: line
#define GRPH_AOP3_8BITS_ADDR_ALIGN      (0)                                 // unit: byte

#define GRPH_AOP3_16BITS_WMIN           (4)                                 // unit: byte
#define GRPH_AOP3_16BITS_WMAX           __ALIGN_FLOOR_2(GRPH_WIDTH_HEIGHT_MAX)  // unit: byte
#define GRPH_AOP3_16BITS_WALIGN         (DRV_LIMIT_ALIGN_HALFWORD)          // unit: byte
#define GRPH_AOP3_16BITS_HMIN           (1)                                 // unit: line
#define GRPH_AOP3_16BITS_HMAX           (GRPH_WIDTH_HEIGHT_MAX)             // unit: line
#define GRPH_AOP3_16BITS_HALIGN         (0)                                 // unit: line
#define GRPH_AOP3_16BITS_ADDR_ALIGN     (DRV_LIMIT_ALIGN_HALFWORD)          // unit: byte

#define GRPH_AOP3_16BITSUV_WMIN         (4)                                 // unit: byte
#define GRPH_AOP3_16BITSUV_WMAX         __ALIGN_FLOOR_2(GRPH_WIDTH_HEIGHT_MAX)  // unit: byte
#define GRPH_AOP3_16BITSUV_WALIGN       (DRV_LIMIT_ALIGN_HALFWORD)          // unit: byte
#define GRPH_AOP3_16BITSUV_HMIN         (1)                                 // unit: line
#define GRPH_AOP3_16BITSUV_HMAX         (GRPH_WIDTH_HEIGHT_MAX)             // unit: line
#define GRPH_AOP3_16BITSUV_HALIGN       (0)                                 // unit: line
#define GRPH_AOP3_16BITSUV_ADDR_ALIGN   (0)                                 // unit: byte

#define GRPH_AOP3_32BITS_WMIN           (4)                                 // unit: byte
#define GRPH_AOP3_32BITS_WMAX           __ALIGN_FLOOR_4(GRPH_WIDTH_HEIGHT_MAX)  // unit: byte
#define GRPH_AOP3_32BITS_WALIGN         (DRV_LIMIT_ALIGN_WORD)              // unit: byte
#define GRPH_AOP3_32BITS_HMIN           (1)                                 // unit: line
#define GRPH_AOP3_32BITS_HMAX           (GRPH_WIDTH_HEIGHT_MAX)             // unit: line
#define GRPH_AOP3_32BITS_HALIGN         (0)                                 // unit: line
#define GRPH_AOP3_32BITS_ADDR_ALIGN     (DRV_LIMIT_ALIGN_WORD)              // unit: byte

#define GRPH_AOP3_16BITARGB_WMIN        (4)                                 // unit: byte
#define GRPH_AOP3_16BITARGB_WMAX        __ALIGN_FLOOR_2(GRPH_WIDTH_HEIGHT_MAX)  // unit: byte
#define GRPH_AOP3_16BITARGB_WALIGN      (DRV_LIMIT_ALIGN_HALFWORD)          // unit: byte
#define GRPH_AOP3_16BITARGB_HMIN        (1)                                 // unit: line
#define GRPH_AOP3_16BITARGB_HMAX        (GRPH_WIDTH_HEIGHT_MAX)             // unit: line
#define GRPH_AOP3_16BITARGB_HALIGN      (0)                                 // unit: line
#define GRPH_AOP3_16BITARGB_ADDR_ALIGN  (0)                                 // unit: byte

// AOP4
#define GRPH_AOP4_8BITS_WMIN            (4)                                 // unit: byte
#define GRPH_AOP4_8BITS_WMAX            (GRPH_WIDTH_HEIGHT_MAX)             // unit: byte
#define GRPH_AOP4_8BITS_WALIGN          (0)                                 // unit: byte
#define GRPH_AOP4_8BITS_HMIN            (1)                                 // unit: line
#define GRPH_AOP4_8BITS_HMAX            (GRPH_WIDTH_HEIGHT_MAX)             // unit: line
#define GRPH_AOP4_8BITS_HALIGN          (0)                                 // unit: line
#define GRPH_AOP4_8BITS_ADDR_ALIGN      (0)                                 // unit: byte

#define GRPH_AOP4_16BITSUV_WMIN         (4)                                 // unit: byte
#define GRPH_AOP4_16BITSUV_WMAX         __ALIGN_FLOOR_2(GRPH_WIDTH_HEIGHT_MAX)  // unit: byte
#define GRPH_AOP4_16BITSUV_WALIGN       (DRV_LIMIT_ALIGN_HALFWORD)          // unit: byte
#define GRPH_AOP4_16BITSUV_HMIN         (1)                                 // unit: line
#define GRPH_AOP4_16BITSUV_HMAX         (GRPH_WIDTH_HEIGHT_MAX)             // unit: line
#define GRPH_AOP4_16BITSUV_HALIGN       (0)                                 // unit: line
#define GRPH_AOP4_16BITSUV_ADDR_ALIGN   (0)                                 // unit: byte

// AOP5 (not available in Graphic2)
#define GRPH_AOP5_8BITS_WMIN            (4)                                 // unit: byte
#define GRPH_AOP5_8BITS_WMAX            (GRPH_WIDTH_HEIGHT_MAX)             // unit: byte
#define GRPH_AOP5_8BITS_WALIGN          (0)                                 // unit: byte
#define GRPH_AOP5_8BITS_HMIN            (1)                                 // unit: line
#define GRPH_AOP5_8BITS_HMAX            (GRPH_WIDTH_HEIGHT_MAX)             // unit: line
#define GRPH_AOP5_8BITS_HALIGN          (0)                                 // unit: line
#define GRPH_AOP5_8BITS_ADDR_ALIGN      (0)                                 // unit: byte

// AOP6 (not available in Graphic2)
#define GRPH_AOP6_8BITS_WMIN            (4)                                 // unit: byte
#define GRPH_AOP6_8BITS_WMAX            (GRPH_WIDTH_HEIGHT_MAX)             // unit: byte
#define GRPH_AOP6_8BITS_WALIGN          (0)                                 // unit: byte
#define GRPH_AOP6_8BITS_HMIN            (1)                                 // unit: line
#define GRPH_AOP6_8BITS_HMAX            (GRPH_WIDTH_HEIGHT_MAX)             // unit: line
#define GRPH_AOP6_8BITS_HALIGN          (0)                                 // unit: line
#define GRPH_AOP6_8BITS_ADDR_ALIGN      (0)                                 // unit: byte

// AOP7 (not available in Graphic2)
#define GRPH_AOP7_8BITS_WMIN            (4)                                 // unit: byte
#define GRPH_AOP7_8BITS_WMAX            (GRPH_WIDTH_HEIGHT_MAX)             // unit: byte
#define GRPH_AOP7_8BITS_WALIGN          (0)                                 // unit: byte
#define GRPH_AOP7_8BITS_HMIN            (1)                                 // unit: line
#define GRPH_AOP7_8BITS_HMAX            (GRPH_WIDTH_HEIGHT_MAX)             // unit: line
#define GRPH_AOP7_8BITS_HALIGN          (0)                                 // unit: line
#define GRPH_AOP7_8BITS_ADDR_ALIGN      (0)                                 // unit: byte

// AOP8
#define GRPH_AOP8_8BITS_WMIN            (4)                                 // unit: byte
#define GRPH_AOP8_8BITS_WMAX            (GRPH_WIDTH_HEIGHT_MAX)             // unit: byte
#define GRPH_AOP8_8BITS_WALIGN          (0)                                 // unit: byte
#define GRPH_AOP8_8BITS_HMIN            (1)                                 // unit: line
#define GRPH_AOP8_8BITS_HMAX            (GRPH_WIDTH_HEIGHT_MAX)             // unit: line
#define GRPH_AOP8_8BITS_HALIGN          (0)                                 // unit: line
#define GRPH_AOP8_8BITS_ADDR_ALIGN      (0)                                 // unit: byte

// AOP8 16 bit not available in Graphic2
#define GRPH_AOP8_16BITSUV_WMIN         (4)                                 // unit: byte
#define GRPH_AOP8_16BITSUV_WMAX         __ALIGN_FLOOR_2(GRPH_WIDTH_HEIGHT_MAX)  // unit: byte
#define GRPH_AOP8_16BITSUV_WALIGN       (DRV_LIMIT_ALIGN_HALFWORD)          // unit: byte
#define GRPH_AOP8_16BITSUV_HMIN         (1)                                 // unit: line
#define GRPH_AOP8_16BITSUV_HMAX         (GRPH_WIDTH_HEIGHT_MAX)             // unit: line
#define GRPH_AOP8_16BITSUV_HALIGN       (0)                                 // unit: line
#define GRPH_AOP8_16BITSUV_ADDR_ALIGN   (0)                                 // unit: byte

// AOP8 32 bit not available in Graphic2
#define GRPH_AOP8_32BITS_WMIN           (4)                                 // unit: byte
#define GRPH_AOP8_32BITS_WMAX           __ALIGN_FLOOR_4(GRPH_WIDTH_HEIGHT_MAX)  // unit: byte
#define GRPH_AOP8_32BITS_WALIGN         (DRV_LIMIT_ALIGN_WORD)              // unit: byte
#define GRPH_AOP8_32BITS_HMIN           (1)                                 // unit: line
#define GRPH_AOP8_32BITS_HMAX           (GRPH_WIDTH_HEIGHT_MAX)             // unit: line
#define GRPH_AOP8_32BITS_HALIGN         (0)                                 // unit: line
#define GRPH_AOP8_32BITS_ADDR_ALIGN     (DRV_LIMIT_ALIGN_WORD)              // unit: byte

// AOP9 (not available in Graphic2)
#define GRPH_AOP9_8BITS_WMIN            (4)                                 // unit: byte
#define GRPH_AOP9_8BITS_WMAX            (GRPH_WIDTH_HEIGHT_MAX)             // unit: byte
#define GRPH_AOP9_8BITS_WALIGN          (0)                                 // unit: byte
#define GRPH_AOP9_8BITS_HMIN            (1)                                 // unit: line
#define GRPH_AOP9_8BITS_HMAX            (GRPH_WIDTH_HEIGHT_MAX)             // unit: line
#define GRPH_AOP9_8BITS_HALIGN          (0)                                 // unit: line
#define GRPH_AOP9_8BITS_ADDR_ALIGN      (0)                                 // unit: byte

// AOP9 16 bit (not available in Graphic2)
#define GRPH_AOP9_16BITSUV_WMIN         (4)                                 // unit: byte
#define GRPH_AOP9_16BITSUV_WMAX         __ALIGN_FLOOR_2(GRPH_WIDTH_HEIGHT_MAX)  // unit: byte
#define GRPH_AOP9_16BITSUV_WALIGN       (DRV_LIMIT_ALIGN_HALFWORD)          // unit: byte
#define GRPH_AOP9_16BITSUV_HMIN         (1)                                 // unit: line
#define GRPH_AOP9_16BITSUV_HMAX         (GRPH_WIDTH_HEIGHT_MAX)             // unit: line
#define GRPH_AOP9_16BITSUV_HALIGN       (0)                                 // unit: line
#define GRPH_AOP9_16BITSUV_ADDR_ALIGN   (0)                                 // unit: byte

// AOP10 (not available in Graphic2)
#define GRPH_AOP10_8BITS_WMIN           (4)                                 // unit: byte
#define GRPH_AOP10_8BITS_WMAX           (GRPH_WIDTH_HEIGHT_MAX)             // unit: byte
#define GRPH_AOP10_8BITS_WALIGN         (0)                                 // unit: byte
#define GRPH_AOP10_8BITS_HMIN           (1)                                 // unit: line
#define GRPH_AOP10_8BITS_HMAX           (GRPH_WIDTH_HEIGHT_MAX)             // unit: line
#define GRPH_AOP10_8BITS_HALIGN         (0)                                 // unit: line
#define GRPH_AOP10_8BITS_ADDR_ALIGN     (0)                                 // unit: byte

// AOP10 16 bit (not available in Graphic2)
#define GRPH_AOP10_16BITSUV_WMIN        (4)                                 // unit: byte
#define GRPH_AOP10_16BITSUV_WMAX        __ALIGN_FLOOR_2(GRPH_WIDTH_HEIGHT_MAX)  // unit: byte
#define GRPH_AOP10_16BITSUV_WALIGN      (DRV_LIMIT_ALIGN_HALFWORD)          // unit: byte
#define GRPH_AOP10_16BITSUV_HMIN        (1)                                 // unit: line
#define GRPH_AOP10_16BITSUV_HMAX        (GRPH_WIDTH_HEIGHT_MAX)             // unit: line
#define GRPH_AOP10_16BITSUV_HALIGN      (0)                                 // unit: line
#define GRPH_AOP10_16BITSUV_ADDR_ALIGN  (0)                                 // unit: byte

// AOP11 (not available in Graphic2)
#define GRPH_AOP11_8BITS_WMIN           (4)                                 // unit: byte
#define GRPH_AOP11_8BITS_WMAX           (GRPH_WIDTH_HEIGHT_MAX)             // unit: byte
#define GRPH_AOP11_8BITS_WALIGN         (0)                                 // unit: byte
#define GRPH_AOP11_8BITS_HMIN           (1)                                 // unit: line
#define GRPH_AOP11_8BITS_HMAX           (GRPH_WIDTH_HEIGHT_MAX)             // unit: line
#define GRPH_AOP11_8BITS_HALIGN         (0)                                 // unit: line
#define GRPH_AOP11_8BITS_ADDR_ALIGN     (0)                                 // unit: byte

// AOP11 16 bit (not available in Graphic2)
#define GRPH_AOP11_16BITSUV_WMIN        (4)                                 // unit: byte
#define GRPH_AOP11_16BITSUV_WMAX        __ALIGN_FLOOR_2(GRPH_WIDTH_HEIGHT_MAX)  // unit: byte
#define GRPH_AOP11_16BITSUV_WALIGN      (DRV_LIMIT_ALIGN_HALFWORD)          // unit: byte
#define GRPH_AOP11_16BITSUV_HMIN        (1)                                 // unit: line
#define GRPH_AOP11_16BITSUV_HMAX        (GRPH_WIDTH_HEIGHT_MAX)             // unit: line
#define GRPH_AOP11_16BITSUV_HALIGN      (0)                                 // unit: line
#define GRPH_AOP11_16BITSUV_ADDR_ALIGN  (0)                                 // unit: byte

// AOP12 (not available in Graphic2)
#define GRPH_AOP12_8BITS_WMIN           (4)                                 // unit: byte
#define GRPH_AOP12_8BITS_WMAX           (GRPH_WIDTH_HEIGHT_MAX)             // unit: byte
#define GRPH_AOP12_8BITS_WALIGN         (0)                                 // unit: byte
#define GRPH_AOP12_8BITS_HMIN           (1)                                 // unit: line
#define GRPH_AOP12_8BITS_HMAX           (GRPH_WIDTH_HEIGHT_MAX)             // unit: line
#define GRPH_AOP12_8BITS_HALIGN         (0)                                 // unit: line
#define GRPH_AOP12_8BITS_ADDR_ALIGN     (0)                                 // unit: byte

// AOP12 16 bit (not available in Graphic2)
#define GRPH_AOP12_16BITSUV_WMIN        (4)                                 // unit: byte
#define GRPH_AOP12_16BITSUV_WMAX        __ALIGN_FLOOR_2(GRPH_WIDTH_HEIGHT_MAX)  // unit: byte
#define GRPH_AOP12_16BITSUV_WALIGN      (DRV_LIMIT_ALIGN_HALFWORD)          // unit: byte
#define GRPH_AOP12_16BITSUV_HMIN        (1)                                 // unit: line
#define GRPH_AOP12_16BITSUV_HMAX        (GRPH_WIDTH_HEIGHT_MAX)             // unit: line
#define GRPH_AOP12_16BITSUV_HALIGN      (0)                                 // unit: line
#define GRPH_AOP12_16BITSUV_ADDR_ALIGN  (0)                                 // unit: byte

// AOP13
#define GRPH_AOP13_8BITS_WMIN           (4)                                 // unit: byte
#define GRPH_AOP13_8BITS_WMAX           (GRPH_WIDTH_HEIGHT_MAX)             // unit: byte
#define GRPH_AOP13_8BITS_WALIGN         (0)                                 // unit: byte
#define GRPH_AOP13_8BITS_HMIN           (1)                                 // unit: line
#define GRPH_AOP13_8BITS_HMAX           (GRPH_WIDTH_HEIGHT_MAX)             // unit: line
#define GRPH_AOP13_8BITS_HALIGN         (0)                                 // unit: line
#define GRPH_AOP13_8BITS_ADDR_ALIGN     (0)                                 // unit: byte

#define GRPH_AOP13_16BITSUV_WMIN        (4)                                 // unit: byte
#define GRPH_AOP13_16BITSUV_WMAX        __ALIGN_FLOOR_2(GRPH_WIDTH_HEIGHT_MAX)  // unit: byte
#define GRPH_AOP13_16BITSUV_WALIGN      (DRV_LIMIT_ALIGN_HALFWORD)          // unit: byte
#define GRPH_AOP13_16BITSUV_HMIN        (1)                                 // unit: line
#define GRPH_AOP13_16BITSUV_HMAX        (GRPH_WIDTH_HEIGHT_MAX)             // unit: line
#define GRPH_AOP13_16BITSUV_HALIGN      (0)                                 // unit: line
#define GRPH_AOP13_16BITSUV_ADDR_ALIGN  (DRV_LIMIT_ALIGN_HALFWORD)          // unit: byte

#define GRPH_AOP13_16BITSRGB_WMIN       (4)                                 // unit: byte
#define GRPH_AOP13_16BITSRGB_WMAX       __ALIGN_FLOOR_2(GRPH_WIDTH_HEIGHT_MAX)  // unit: byte
#define GRPH_AOP13_16BITSRGB_WALIGN     (DRV_LIMIT_ALIGN_HALFWORD)          // unit: byte
#define GRPH_AOP13_16BITSRGB_HMIN       (1)                                 // unit: line
#define GRPH_AOP13_16BITSRGB_HMAX       (GRPH_WIDTH_HEIGHT_MAX)             // unit: line
#define GRPH_AOP13_16BITSRGB_HALIGN     (0)                                 // unit: line
#define GRPH_AOP13_16BITSRGB_ADDR_ALIGN (DRV_LIMIT_ALIGN_HALFWORD)          // unit: byte

#define GRPH_AOP13_32BITS_WMIN           (4)                                // unit: byte
#define GRPH_AOP13_32BITS_WMAX           __ALIGN_FLOOR_4(GRPH_WIDTH_HEIGHT_MAX) // unit: byte
#define GRPH_AOP13_32BITS_WALIGN         (DRV_LIMIT_ALIGN_WORD)             // unit: byte
#define GRPH_AOP13_32BITS_HMIN           (1)                                // unit: line
#define GRPH_AOP13_32BITS_HMAX           (GRPH_WIDTH_HEIGHT_MAX)            // unit: line
#define GRPH_AOP13_32BITS_HALIGN         (0)                                // unit: line
#define GRPH_AOP13_32BITS_ADDR_ALIGN     (DRV_LIMIT_ALIGN_WORD)             // unit: byte

// AOP14 (not available in Graphic2)
#define GRPH_AOP14_8BITS_WMIN           (4)                                 // unit: byte
#define GRPH_AOP14_8BITS_WMAX           (GRPH_WIDTH_HEIGHT_MAX)             // unit: byte
#define GRPH_AOP14_8BITS_WALIGN         (0)                                 // unit: byte
#define GRPH_AOP14_8BITS_HMIN           (1)                                 // unit: line
#define GRPH_AOP14_8BITS_HMAX           (GRPH_WIDTH_HEIGHT_MAX)             // unit: line
#define GRPH_AOP14_8BITS_HALIGN         (0)                                 // unit: line
#define GRPH_AOP14_8BITS_ADDR_ALIGN     (0)                                 // unit: byte

// AOP14 16 bit (not available in Graphic2)
#define GRPH_AOP14_16BITSUV_WMIN        (4)                                 // unit: byte
#define GRPH_AOP14_16BITSUV_WMAX        __ALIGN_FLOOR_2(GRPH_WIDTH_HEIGHT_MAX)  // unit: byte
#define GRPH_AOP14_16BITSUV_WALIGN      (DRV_LIMIT_ALIGN_HALFWORD)          // unit: byte
#define GRPH_AOP14_16BITSUV_HMIN        (1)                                 // unit: line
#define GRPH_AOP14_16BITSUV_HMAX        (GRPH_WIDTH_HEIGHT_MAX)             // unit: line
#define GRPH_AOP14_16BITSUV_HALIGN      (0)                                 // unit: line
#define GRPH_AOP14_16BITSUV_ADDR_ALIGN  (0)                                 // unit: byte

// AOP15 (not available in Graphic2)
#define GRPH_AOP15_8BITS_WMIN           (GRPH_AOP1_8BITS_WMIN)
#define GRPH_AOP15_8BITS_WMAX           (GRPH_AOP1_8BITS_WMAX)
#define GRPH_AOP15_8BITS_WALIGN         (GRPH_AOP1_8BITS_WALIGN)
#define GRPH_AOP15_8BITS_HMIN           (GRPH_AOP1_8BITS_HMIN)
#define GRPH_AOP15_8BITS_HMAX           (GRPH_AOP1_8BITS_HMAX)
#define GRPH_AOP15_8BITS_HALIGN         (GRPH_AOP1_8BITS_HALIGN)
#define GRPH_AOP15_8BITS_ADDR_ALIGN     (GRPH_AOP1_8BITS_ADDR_ALIGN)

// AOP15 16 bits (not available in Graphic2)
#define GRPH_AOP15_16BITS_WMIN          (GRPH_AOP1_16BITS_WMIN)
#define GRPH_AOP15_16BITS_WMAX          (GRPH_AOP1_16BITS_WMAX)
#define GRPH_AOP15_16BITS_WALIGN        (GRPH_AOP1_16BITS_WALIGN)
#define GRPH_AOP15_16BITS_HMIN          (GRPH_AOP1_16BITS_HMIN)
#define GRPH_AOP15_16BITS_HMAX          (GRPH_AOP1_16BITS_HMAX)
#define GRPH_AOP15_16BITS_HALIGN        (GRPH_AOP1_16BITS_HALIGN)
#define GRPH_AOP15_16BITS_ADDR_ALIGN    (GRPH_AOP1_16BITS_ADDR_ALIGN)

// AOP15 UV pack (not available in Graphic2)
#define GRPH_AOP15_16BITSUV_WMIN        (GRPH_AOP1_16BITSUV_WMIN)
#define GRPH_AOP15_16BITSUV_WMAX        (GRPH_AOP1_16BITSUV_WMAX)
#define GRPH_AOP15_16BITSUV_WALIGN      (GRPH_AOP1_16BITSUV_WALIGN)
#define GRPH_AOP15_16BITSUV_HMIN        (GRPH_AOP1_16BITSUV_HMIN)
#define GRPH_AOP15_16BITSUV_HMAX        (GRPH_AOP1_16BITSUV_HMAX)
#define GRPH_AOP15_16BITSUV_HALIGN      (GRPH_AOP1_16BITSUV_HALIGN)
#define GRPH_AOP15_16BITSUV_ADDR_ALIGN  (GRPH_AOP1_16BITSUV_ADDR_ALIGN)

// AOP16 (not available in Graphic2)
#define GRPH_AOP16_8BITS_WMIN           (GRPH_AOP1_8BITS_WMIN)
#define GRPH_AOP16_8BITS_WMAX           (GRPH_AOP1_8BITS_WMAX)
#define GRPH_AOP16_8BITS_WALIGN         (GRPH_AOP1_8BITS_WALIGN)
#define GRPH_AOP16_8BITS_HMIN           (GRPH_AOP1_8BITS_HMIN)
#define GRPH_AOP16_8BITS_HMAX           (GRPH_AOP1_8BITS_HMAX)
#define GRPH_AOP16_8BITS_HALIGN         (GRPH_AOP1_8BITS_HALIGN)
#define GRPH_AOP16_8BITS_ADDR_ALIGN     (GRPH_AOP1_8BITS_ADDR_ALIGN)

// AOP17 16 bits (not available in Graphic2)
#define GRPH_AOP17_16BITS_WMIN          (GRPH_AOP1_16BITS_WMIN)
#define GRPH_AOP17_16BITS_WMAX          __ALIGN_FLOOR_4(GRPH_WIDTH_HEIGHT_MAX)
#define GRPH_AOP17_16BITS_WALIGN        (GRPH_AOP1_16BITS_WALIGN)
#define GRPH_AOP17_16BITS_HMIN          (GRPH_AOP1_16BITS_HMIN)
#define GRPH_AOP17_16BITS_HMAX          (GRPH_AOP1_16BITS_HMAX)
#define GRPH_AOP17_16BITS_HALIGN        (GRPH_AOP1_16BITS_HALIGN)
#define GRPH_AOP17_16BITS_ADDR_ALIGN    (DRV_LIMIT_ALIGN_WORD)

// AOP18 (not available in Graphic2)
#define GRPH_AOP18_8BITS_WMIN           (4)                                 // unit: byte
#define GRPH_AOP18_8BITS_WMAX           (GRPH_WIDTH_HEIGHT_MAX)             // unit: byte
#define GRPH_AOP18_8BITS_WALIGN         (0)                                 // unit: byte
#define GRPH_AOP18_8BITS_HMIN           (1)                                 // unit: line
#define GRPH_AOP18_8BITS_HMAX           (GRPH_WIDTH_HEIGHT_MAX)             // unit: line
#define GRPH_AOP18_8BITS_HALIGN         (0)                                 // unit: line
#define GRPH_AOP18_8BITS_ADDR_ALIGN     (0)                                 // unit: byte

#define GRPH_AOP18_16BITSUV_WMIN        (4)                                 // unit: byte
#define GRPH_AOP18_16BITSUV_WMAX        __ALIGN_FLOOR_2(GRPH_WIDTH_HEIGHT_MAX)  // unit: byte
#define GRPH_AOP18_16BITSUV_WALIGN      (DRV_LIMIT_ALIGN_HALFWORD)          // unit: byte
#define GRPH_AOP18_16BITSUV_HMIN        (1)                                 // unit: line
#define GRPH_AOP18_16BITSUV_HMAX        (GRPH_WIDTH_HEIGHT_MAX)             // unit: line
#define GRPH_AOP18_16BITSUV_HALIGN      (0)                                 // unit: line
#define GRPH_AOP18_16BITSUV_ADDR_ALIGN  (0)                                 // unit: byte

#define GRPH_AOP18_32BITS_WMIN           (4)                                // unit: byte
#define GRPH_AOP18_32BITS_WMAX           __ALIGN_FLOOR_4(GRPH_WIDTH_HEIGHT_MAX) // unit: byte
#define GRPH_AOP18_32BITS_WALIGN         (DRV_LIMIT_ALIGN_WORD)             // unit: byte
#define GRPH_AOP18_32BITS_HMIN           (1)                                // unit: line
#define GRPH_AOP18_32BITS_HMAX           (GRPH_WIDTH_HEIGHT_MAX)            // unit: line
#define GRPH_AOP18_32BITS_HALIGN         (0)                                // unit: line
#define GRPH_AOP18_32BITS_ADDR_ALIGN     (DRV_LIMIT_ALIGN_WORD)             // unit: byte

// AOP19 (not available in Graphic2)
#define GRPH_AOP19_8BITS_WMIN           (GRPH_AOP13_8BITS_WMIN)
#define GRPH_AOP19_8BITS_WMAX           (GRPH_AOP13_8BITS_WMAX)
#define GRPH_AOP19_8BITS_WALIGN         (GRPH_AOP13_8BITS_WALIGN)
#define GRPH_AOP19_8BITS_HMIN           (GRPH_AOP13_8BITS_HMIN)
#define GRPH_AOP19_8BITS_HMAX           (GRPH_AOP13_8BITS_HMAX)
#define GRPH_AOP19_8BITS_HALIGN         (GRPH_AOP13_8BITS_HALIGN)
#define GRPH_AOP19_8BITS_ADDR_ALIGN     (GRPH_AOP13_8BITS_ADDR_ALIGN)

#define GRPH_AOP19_16BITSUV_WMIN        (GRPH_AOP13_16BITSUV_WMIN)
#define GRPH_AOP19_16BITSUV_WMAX        (GRPH_AOP13_16BITSUV_WMAX)
#define GRPH_AOP19_16BITSUV_WALIGN      (GRPH_AOP13_16BITSUV_WALIGN)
#define GRPH_AOP19_16BITSUV_HMIN        (GRPH_AOP13_16BITSUV_HMIN)
#define GRPH_AOP19_16BITSUV_HMAX        (GRPH_AOP13_16BITSUV_HMAX)
#define GRPH_AOP19_16BITSUV_HALIGN      (GRPH_AOP13_16BITSUV_HALIGN)
#define GRPH_AOP19_16BITSUV_ADDR_ALIGN  (GRPH_AOP13_16BITSUV_ADDR_ALIGN)

#define GRPH_AOP19_16BITSRGB_WMIN       (GRPH_AOP13_16BITSRGB_WMIN)
#define GRPH_AOP19_16BITSRGB_WMAX       (GRPH_AOP13_16BITSRGB_WMAX)
#define GRPH_AOP19_16BITSRGB_WALIGN     (GRPH_AOP13_16BITSRGB_WALIGN)
#define GRPH_AOP19_16BITSRGB_HMIN       (GRPH_AOP13_16BITSRGB_HMIN)
#define GRPH_AOP19_16BITSRGB_HMAX       (GRPH_AOP13_16BITSRGB_HMAX)
#define GRPH_AOP19_16BITSRGB_HALIGN     (GRPH_AOP13_16BITSRGB_HALIGN)
#define GRPH_AOP19_16BITSRGB_ADDR_ALIGN (0)

#define GRPH_AOP19_32BITS_WMIN           (4)                                // unit: byte
#define GRPH_AOP19_32BITS_WMAX           __ALIGN_FLOOR_4(GRPH_WIDTH_HEIGHT_MAX) // unit: byte
#define GRPH_AOP19_32BITS_WALIGN         (DRV_LIMIT_ALIGN_WORD)             // unit: byte
#define GRPH_AOP19_32BITS_HMIN           (1)                                // unit: line
#define GRPH_AOP19_32BITS_HMAX           (GRPH_WIDTH_HEIGHT_MAX)            // unit: line
#define GRPH_AOP19_32BITS_HALIGN         (0)                                // unit: line
#define GRPH_AOP19_32BITS_ADDR_ALIGN     (DRV_LIMIT_ALIGN_WORD)             // unit: byte

// AOP22 (not available in Graphic2)
#define GRPH_AOP22_8BITS_WMIN           (4)                                 // unit: byte
#define GRPH_AOP22_8BITS_WMAX           (GRPH_WIDTH_HEIGHT_MAX)             // unit: byte
#define GRPH_AOP22_8BITS_WALIGN         (0)                                 // unit: byte
#define GRPH_AOP22_8BITS_HMIN           (1)                                 // unit: line
#define GRPH_AOP22_8BITS_HMAX           (GRPH_WIDTH_HEIGHT_MAX)             // unit: line
#define GRPH_AOP22_8BITS_HALIGN         (0)                                 // unit: line
#define GRPH_AOP22_8BITS_ADDR_ALIGN     (0)                                 // unit: byte

#define GRPH_AOP22_16BITSUV_WMIN        (4)                                 // unit: byte
#define GRPH_AOP22_16BITSUV_WMAX        __ALIGN_FLOOR_2(GRPH_WIDTH_HEIGHT_MAX)  // unit: byte
#define GRPH_AOP22_16BITSUV_WALIGN      (DRV_LIMIT_ALIGN_HALFWORD)          // unit: byte
#define GRPH_AOP22_16BITSUV_HMIN        (1)                                 // unit: line
#define GRPH_AOP22_16BITSUV_HMAX        (GRPH_WIDTH_HEIGHT_MAX)             // unit: line
#define GRPH_AOP22_16BITSUV_HALIGN      (0)                                 // unit: line
#define GRPH_AOP22_16BITSUV_ADDR_ALIGN  (0)                                 // unit: byte

// AOP23 (not available in Graphic2)
#define GRPH_AOP23_8BITS_WMIN           (4)                                 // unit: byte
#define GRPH_AOP23_8BITS_WMAX           (GRPH_WIDTH_HEIGHT_MAX)             // unit: byte
#define GRPH_AOP23_8BITS_WALIGN         (0)                                 // unit: byte
#define GRPH_AOP23_8BITS_HMIN           (1)                                 // unit: line
#define GRPH_AOP23_8BITS_HMAX           (GRPH_WIDTH_HEIGHT_MAX)             // unit: line
#define GRPH_AOP23_8BITS_HALIGN         (0)                                 // unit: line
#define GRPH_AOP23_8BITS_ADDR_ALIGN     (0)                                 // unit: byte

#define GRPH_AOP23_16BITSUV_WMIN        (4)                                 // unit: byte
#define GRPH_AOP23_16BITSUV_WMAX        __ALIGN_FLOOR_2(GRPH_WIDTH_HEIGHT_MAX)  // unit: byte
#define GRPH_AOP23_16BITSUV_WALIGN      (DRV_LIMIT_ALIGN_HALFWORD)          // unit: byte
#define GRPH_AOP23_16BITSUV_HMIN        (1)                                 // unit: line
#define GRPH_AOP23_16BITSUV_HMAX        (GRPH_WIDTH_HEIGHT_MAX)             // unit: line
#define GRPH_AOP23_16BITSUV_HALIGN      (0)                                 // unit: line
#define GRPH_AOP23_16BITSUV_ADDR_ALIGN  (0)                                 // unit: byte

// AOP24 16 bits (not available in Graphic2)
#define GRPH_AOP24_16BITS_WMIN          (GRPH_AOP1_16BITS_WMIN)
#define GRPH_AOP24_16BITS_WMAX          __ALIGN_FLOOR_4(GRPH_WIDTH_HEIGHT_MAX)
#define GRPH_AOP24_16BITS_WALIGN        (GRPH_AOP1_16BITS_WALIGN)
#define GRPH_AOP24_16BITS_HMIN          (2)
#define GRPH_AOP24_16BITS_HMAX          __ALIGN_FLOOR_2(GRPH_WIDTH_HEIGHT_MAX)
#define GRPH_AOP24_16BITS_HALIGN        (2)
#define GRPH_AOP24_16BITS_ADDR_ALIGN    (DRV_LIMIT_ALIGN_WORD)
#define GRPH_AOP24_16BITS_ADDR_A_ALIGN	(DRV_LIMIT_ALIGN_WORD)
#define GRPH_AOP24_16BITS_ADDR_B_ALIGN	(DRV_LIMIT_ALIGN_HALFWORD)
#define GRPH_AOP24_16BITS_ADDR_C_ALIGN	(DRV_LIMIT_ALIGN_HALFWORD)

// AOP25 16 bits (not available in Graphic2)
#define GRPH_AOP25_16BITS_WMIN          (GRPH_AOP1_16BITS_WMIN)
#define GRPH_AOP25_16BITS_WMAX          __ALIGN_FLOOR_4(GRPH_WIDTH_HEIGHT_MAX)
#define GRPH_AOP25_16BITS_WALIGN        (GRPH_AOP1_16BITS_WALIGN)
#define GRPH_AOP25_16BITS_HMIN          (2)
#define GRPH_AOP25_16BITS_HMAX          __ALIGN_FLOOR_2(GRPH_WIDTH_HEIGHT_MAX)
#define GRPH_AOP25_16BITS_HALIGN        (2)
#define GRPH_AOP25_16BITS_ADDR_ALIGN    (DRV_LIMIT_ALIGN_WORD)
#define GRPH_AOP25_16BITS_ADDR_A_ALIGN	(DRV_LIMIT_ALIGN_WORD)
#define GRPH_AOP25_16BITS_ADDR_B_ALIGN	(DRV_LIMIT_ALIGN_HALFWORD)
#define GRPH_AOP25_16BITS_ADDR_C_ALIGN	(DRV_LIMIT_ALIGN_HALFWORD)

// AOP26 16 bits (not available in Graphic2)
#define GRPH_AOP26_16BITS_WMIN          (GRPH_AOP1_16BITS_WMIN)
#define GRPH_AOP26_16BITS_WMAX          __ALIGN_FLOOR_2(GRPH_WIDTH_HEIGHT_MAX)
#define GRPH_AOP26_16BITS_WALIGN        (GRPH_AOP1_16BITS_WALIGN)
#define GRPH_AOP26_16BITS_HMIN          (1)
#define GRPH_AOP26_16BITS_HMAX          (GRPH_WIDTH_HEIGHT_MAX)
#define GRPH_AOP26_16BITS_HALIGN        (0)
#define GRPH_AOP26_16BITS_ADDR_ALIGN    (DRV_LIMIT_ALIGN_WORD)

// AOP26 32 bits (not available in Graphic2)
#define GRPH_AOP26_32BITS_WMIN          (4)
#define GRPH_AOP26_32BITS_WMAX          __ALIGN_FLOOR_4(GRPH_WIDTH_HEIGHT_MAX)
#define GRPH_AOP26_32BITS_WALIGN        (DRV_LIMIT_ALIGN_WORD)
#define GRPH_AOP26_32BITS_HMIN          (1)
#define GRPH_AOP26_32BITS_HMAX          (GRPH_WIDTH_HEIGHT_MAX)
#define GRPH_AOP26_32BITS_HALIGN        (0)
#define GRPH_AOP26_32BITS_ADDR_ALIGN    (DRV_LIMIT_ALIGN_WORD)

#endif
