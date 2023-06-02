/*
    Affine module register header

    Affine module register header

    @file       affine_reg.h
    @ingroup    mIDrvIPP_Affine
    @note       Nothing

    Copyright   Novatek Microelectronics Corp. 2012.  All rights reserved.
*/
#ifndef  _AFFINE_REG_H
#define  _AFFINE_REG_H

#if defined __UITRON || defined __ECOS
#include "DrvCommon.h"

#elif defined(__FREERTOS)
#include "rcw_macro.h"
#include "kwrap/type.h"
//#include "affine_platform.h"
//#include "affine_dbg.h"

#else
#include "mach/rcw_macro.h"
#include "kwrap/type.h"
//#include "affine_platform.h"
//#include "affine_dbg.h"

#endif
//#include "kwrap/error_no.h"

#define AFFINE_LL_BUF_SIZE            (4096)

//
//  affine register access definition
//
#define AFFINE_REG_ADDR(ofs)       (IOADDR_AFFINE_REG_BASE+(ofs))
#define AFFINE_GETREG(ofs)          INW(AFFINE_REG_ADDR(ofs))
#define AFFINE_SETREG(ofs,value)    OUTW(AFFINE_REG_ADDR(ofs), (value))

/*
    TRIG_OP_ENUM
*/
typedef enum {
	TRIG_OP_NOP,                        //< NOP
	TRIG_OP_EN,                         //< Trigger Enable

	ENUM_DUMMY4WORD(TRIG_OP_ENUM)
} TRIG_OP_ENUM;

/*
    OP_PRECISION_ENUM
*/
typedef enum {
	OP_PRECISION_8BITS,                 //< 8 bits precision
	OP_PRECISION_16BITS,                //< 16 bits precision (only for UV packed 16 bit)

	ENUM_DUMMY4WORD(OP_PRECISION_ENUM)
} OP_PRECISION_ENUM;


//0x00 Operation Control Register
#define AFFINE_CTRL_REG_OFS                 0x00
REGDEF_BEGIN(AFFINE_CTRL_REG)
REGDEF_BIT(TRIG_OP, 1)              // Trigger Operation
REGDEF_BIT(, 27)                    // Reserved
REGDEF_BIT(LL_FIRE, 1)              // Trigger Link List
REGDEF_BIT(, 3)                     // Reserved
REGDEF_END(AFFINE_CTRL_REG)

//0x04 Operation Configuration Register
#define AFFINE_CONFIG_REG_OFS               0x04
REGDEF_BEGIN(AFFINE_CONFIG_REG)
REGDEF_BIT(OP_PRECISION, 1)         // Operation Precision
REGDEF_BIT(, 31)
REGDEF_END(AFFINE_CONFIG_REG)

//0x08 Interrupt Enable Register
#define AFFINE_INTEN_REG_OFS                0x08
REGDEF_BEGIN(AFFINE_INTEN_REG)
REGDEF_BIT(INT_EN, 1)               // Interrupt Enable
REGDEF_BIT(LLEND_INT_EN, 1)			// LLEND Interrupt Enable
REGDEF_BIT(LLERROR_INT_EN, 1) 		// LLERROR Interrupt Enable
REGDEF_BIT(, 29)                    // Reserved
REGDEF_END(AFFINE_INTEN_REG)

//0x0C Interrupt Status Register
#define AFFINE_INTSTS_REG_OFS               0x0C
REGDEF_BEGIN(AFFINE_INTSTS_REG)
REGDEF_BIT(INT_STS, 1)              // Interrupt Status
REGDEF_BIT(LLEND_INT_STS, 1)		// LLEND Interrupt Status
REGDEF_BIT(LLERROR_INT_STS, 1)		// LLERROR Interrupt Status
REGDEF_BIT(, 29)                    // Reserved
REGDEF_END(AFFINE_INTSTS_REG)

//0x10 Coefficient Register
#define AFFINE_COEFF_REG_OFS                0x10
REGDEF_BEGIN(AFFINE_COEFF_REG)
REGDEF_BIT(COEFF_A, 17)             // Coefficient A of Q1.15
REGDEF_BIT(, 15)
REGDEF_END(AFFINE_COEFF_REG)

//0x14 Coefficient Register 2
#define AFFINE_COEFF2_REG_OFS               0x14
REGDEF_BEGIN(AFFINE_COEFF2_REG)
REGDEF_BIT(COEFF_B, 17)             // Coefficient B of Q1.15
REGDEF_BIT(, 15)
REGDEF_END(AFFINE_COEFF2_REG)

//0x18 Coefficient Register 3
#define AFFINE_COEFF3_REG_OFS               0x18
REGDEF_BEGIN(AFFINE_COEFF3_REG)
REGDEF_BIT(COEFF_C, 30)             // Coefficient C of Q14.15
REGDEF_BIT(, 2)
REGDEF_END(AFFINE_COEFF3_REG)

//0x1C Coefficient Register 4
#define AFFINE_COEFF4_REG_OFS               0x1C
REGDEF_BEGIN(AFFINE_COEFF4_REG)
REGDEF_BIT(COEFF_D, 17)             // Coefficient D of Q1.15
REGDEF_BIT(, 15)
REGDEF_END(AFFINE_COEFF4_REG)

//0x20 Coefficient Register 5
#define AFFINE_COEFF5_REG_OFS               0x20
REGDEF_BEGIN(AFFINE_COEFF5_REG)
REGDEF_BIT(COEFF_E, 17)             // Coefficient E of Q1.15
REGDEF_BIT(, 15)
REGDEF_END(AFFINE_COEFF5_REG)

//0x24 Coefficient Register 6
#define AFFINE_COEFF6_REG_OFS               0x24
REGDEF_BEGIN(AFFINE_COEFF6_REG)
REGDEF_BIT(COEFF_F, 30)             // Coefficient F of Q14.15
REGDEF_BIT(, 2)
REGDEF_END(AFFINE_COEFF6_REG)

//0x28 Subblock Register
#define AFFINE_SUBBLOCK_REG_OFS             0x28
REGDEF_BEGIN(AFFINE_SUBBLOCK_REG)
REGDEF_BIT(BURST_LENGTH, 9)         // Sub Block burst length (unit: word)
REGDEF_BIT(, 23)
REGDEF_END(AFFINE_SUBBLOCK_REG)

//0x2C Subblock Register
#define AFFINE_SUBBLOCK2_REG_OFS            0x2C
REGDEF_BEGIN(AFFINE_SUBBLOCK2_REG)
REGDEF_BIT(VECTOR_X, 6)             // Sub Block vector X (unit: pixel)
REGDEF_BIT(rsv1, 10)
REGDEF_BIT(VECTOR_Y, 6)             // Sub Block vector Y (unit: pixel)
REGDEF_BIT(rsv2, 10)
REGDEF_END(AFFINE_SUBBLOCK2_REG)

//0x30 Source Image Buffer Address Register
#define AFFINE_IMG_SRC_ADDR_REG_OFS         0x30
REGDEF_BEGIN(AFFINE_IMG_SRC_ADDR_REG)
REGDEF_BIT(IMG_SRC_ADDR, 31)        // Starting Address of Source Image (unit: byte)
REGDEF_BIT(, 1)
REGDEF_END(AFFINE_IMG_SRC_ADDR_REG)

//0x34 Source Image Buffer Line Offset Register
#define AFFINE_IMG_SRC_LOFF_REG_OFS         0x34
REGDEF_BEGIN(AFFINE_IMG_SRC_LOFF_REG)
REGDEF_BIT(IMAGE_SRC_LOFF, 17)      // Line Offset of Source Image (unit: byte)
REGDEF_BIT(, 15)
REGDEF_END(AFFINE_IMG_SRC_LOFF_REG)

//0x38 Image Active Height Register
#define AFFINE_LCNT_REG_OFS                 0x38
REGDEF_BEGIN(AFFINE_LCNT_REG)
REGDEF_BIT(ACT_HEIGHT, 10)          // Active Height (unit: block)
REGDEF_BIT(, 22)
REGDEF_END(AFFINE_LCNT_REG)

//0x3C Image Active Width Register
#define AFFINE_XRGN_REG_OFS                 0x3C
REGDEF_BEGIN(AFFINE_XRGN_REG)
REGDEF_BIT(ACT_WIDTH, 10)           // Active Width (unit: block)
REGDEF_BIT(, 22)
REGDEF_END(AFFINE_XRGN_REG)

//0x40 Destination Image Buffer Address Register
#define AFFINE_IMG_DST_ADDR_REG_OFS         0x40
REGDEF_BEGIN(AFFINE_IMG_DST_ADDR_REG)
REGDEF_BIT(IMG_DST_ADDR, 31)        // Starting Address of Destination Image (unit: byte)
REGDEF_BIT(, 1)
REGDEF_END(AFFINE_IMG_DST_ADDR_REG)

//0x44 Destination Image Buffer Line Offset Register
#define AFFINE_IMG_DST_LOFF_REG_OFS         0x44
REGDEF_BEGIN(AFFINE_IMG_DST_LOFF_REG)
REGDEF_BIT(IMG_DST_LOFF, 17)        // Line Offset of Destination Image (unit: byte)
REGDEF_BIT(, 15)
REGDEF_END(AFFINE_IMG_DST_LOFF_REG)

//0x54 LinkList Buffer Address Register
#define LL_DMA_ADDR_REG_OFS                 0x54
REGDEF_BEGIN(LL_DMA_ADDR_REG)
REGDEF_BIT(ADDR, 32)                // Starting Address of LinkList
REGDEF_END(LL_DMA_ADDR_REG)

#endif

