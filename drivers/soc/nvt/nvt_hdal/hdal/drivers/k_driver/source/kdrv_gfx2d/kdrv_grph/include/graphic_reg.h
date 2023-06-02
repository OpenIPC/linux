/*
    Graphic module register header

    Graphic module register header

    @file       grph_reg.h
    @ingroup    mIDrvIPP_Graph
    @note       Nothing

    Copyright   Novatek Microelectronics Corp. 2016.  All rights reserved.
*/
#ifndef _GRPH_REG_H
#define _GRPH_REG_H

#if defined __UITRON || defined __ECOS
#include "DrvCommon.h"

#elif defined(__FREERTOS)
#include "rcw_macro.h"
#include "kwrap/type.h"

#else
#include "mach/rcw_macro.h"
#include "kwrap/type.h"
#endif

// SW impelemented cache layer above APB and Link List

typedef enum _GRPH_REG_FLG {
	GRPH_REG_FLG_DIRTY = 0x01,

	ENUM_DUMMY4WORD(GRPH_REG_FLG)
} GRPH_REG_FLG;

typedef struct _GRPH_REG_CACHE {
	UINT8   flag;
	UINT32  value;
} GRPH_REG_CACHE;

/*
    OP_TYPE_ENUM
*/
typedef enum {
	OP_TYPE_GOP,                        //< GOP
	OP_TYPE_AOP,                        //< AOP

	ENUM_DUMMY4WORD(OP_TYPE_ENUM)
} OP_TYPE_ENUM;

/*
    OP_PRECISION_ENUM
*/
typedef enum {
	OP_PRECISION_4BITS,                 //< 4 bits precision (removed from NT96660)
	OP_PRECISION_8BITS,                 //< 8 bits precision
	OP_PRECISION_16BITS,                //< 16 bits precision
	OP_PRECISION_32BITS,                //< 32 bits precision
	OP_PRECISION_1BIT,                  //< 1 bit precision
	OP_PRECISION_PALETTE,               //< palette

	ENUM_DUMMY4WORD(OP_PRECISION_ENUM)
} OP_PRECISION_ENUM;

/*
    FORMAT_16BIT_ENUM
*/
typedef enum {
	FORMAT_16BIT_NORMAL,                //< normal planar
	FORMAT_16BIT_UVPACK,                //< UV pack
	FORMAT_16BIT_RGBPACK,               //< RGB pack
	FORMAT_16BIT_ARGB1555,              //< ARGB1555 pack
	FORMAT_16BIT_ARGB4444,              //< ARGB4444 pack

	ENUM_DUMMY4WORD(FORMAT_16BIT_ENUM)
} FORMAT_16BIT_ENUM;

/*
    FORMAT_32BIT_ENUM
*/
typedef enum {
	FORMAT_32BIT_NORMAL,                //< normal planar
	FORMAT_32BIT_ARGB8888,              //< ARGB8888 pack

	ENUM_DUMMY4WORD(FORMAT_32BIT_ENUM)
} FORMAT_32BIT_ENUM;

/*
    UVPACK_ATTRIB_ENUM
*/
typedef enum {
	UVPACK_ATTRIB_BOTH,                 //< both U/V
	UVPACK_ATTRIB_U,                    //< U only
	UVPACK_ATTRIB_V,                    //< V only

	ENUM_DUMMY4WORD(UVPACK_ATTRIB_ENUM)
} UVPACK_ATTRIB_ENUM;

/*
    ARGB_ATTRIB_ENUM
*/
typedef enum {
	ARGB_ATTRIB_RGB,                //< RGB only
	ARGB_ATTRIB_A,                  //< A only

	ENUM_DUMMY4WORD(ARGB_ATTRIB_ENUM)
} ARGB_ATTRIB_ENUM;

/*
    FORMAT_PAL_ENUM
*/
typedef enum {
	FORMAT_PAL_ENUM_1BIT,               //< 1 bit palette
	FORMAT_PAL_ENUM_2BITS,              //< 2 bits palette
	FORMAT_PAL_ENUM_4BITS,              //< 4 bits palette

	ENUM_DUMMY4WORD(FORMAT_PAL_ENUM)
} FORMAT_PAL_ENUM;

//0x00 Operation Control Register
#define GRPH_CTRL_REG_OFS                   0x00
REGDEF_BEGIN(GRPH_CTRL_REG)
REGDEF_BIT(TRIG_OP, 1)              // Trigger Operation
REGDEF_BIT(SW_RST, 1)               // SW reset
REGDEF_BIT(, 26)                    // Reserved
REGDEF_BIT(LL_FIRE, 1)              // Trigger Link List
REGDEF_BIT(LL_TERMINATE, 1)         // Terminate Link List
REGDEF_BIT(, 2)                     // Reserved
REGDEF_END(GRPH_CTRL_REG)

//0x04 Operation Configuration Register
#define GRPH_CONFIG_REG_OFS                 0x04
REGDEF_BEGIN(GRPH_CONFIG_REG)
REGDEF_BIT(OP_TYPE, 1)              // Operation Type: GOP/AOP
REGDEF_BIT(, 3)
REGDEF_BIT(OP_PRECISION, 3)         // Operation Precision
REGDEF_BIT(, 1)
REGDEF_BIT(FORMAT_16BIT, 3)         // Plane format of 16 bits
REGDEF_BIT(FORMAT_32BIT, 1)         // Plane format of 32 bits
REGDEF_BIT(UVPACK_ATTRIB, 2)        // UV pack attribute
REGDEF_BIT(ARGBPACK_ATTRIB, 1)      // ARGB8888 pack attribute
REGDEF_BIT(, 1)
REGDEF_BIT(GE_MODE, 4)              // GOP mode
REGDEF_BIT(PALETTE_FORMAT, 2)       // Palette format
REGDEF_BIT(, 2)
REGDEF_BIT(AOP_MODE, 5)             // AOP mode
REGDEF_BIT(, 3)
REGDEF_END(GRPH_CONFIG_REG)

//0x08 Interrupt Enable Register
#define GRPH_INTEN_REG_OFS                  0x08
REGDEF_BEGIN(GRPH_INTEN_REG)
REGDEF_BIT(INT_EN, 1)               // Graphic Interrupt Enable
REGDEF_BIT(LLEND_INT_EN, 1)         // Link List End Interrupt Enable
REGDEF_BIT(LLERR_INT_EN, 1)         // Link List Error Interrupt Enable
REGDEF_BIT(, 1)                     // Reserved
REGDEF_BIT(DMAIDLE_INT_EN, 1)       // DMA Idle Interrupt Enable
REGDEF_BIT(, 27)                    // Reserved
REGDEF_END(GRPH_INTEN_REG)

//0x0C Interrupt Status Register
#define GRPH_INTSTS_REG_OFS                 0x0C
REGDEF_BEGIN(GRPH_INTSTS_REG)
REGDEF_BIT(INT_STS, 1)              // Graphic Interrupt Status
REGDEF_BIT(LLEND_INT_STS, 1)        // Link List End Interrupt Status
REGDEF_BIT(LLERR_INT_STS, 1)        // Link List Error Interrupt Status
REGDEF_BIT(, 1)                     // Reserved
REGDEF_BIT(DMAIDLE_INT_STS, 1)      // DMA Idle Interrupt Status
REGDEF_BIT(, 27)                    // Reserved
REGDEF_END(GRPH_INTSTS_REG)

//0x10 Blending Control Register
#define GRPH_BLD_REG_OFS                    0x10
REGDEF_BEGIN(GRPH_BLD_REG)
REGDEF_BIT(BLD_WA, 8)               // Weighting-A Value for Blendind Operation
REGDEF_BIT(BLD_WB, 8)               // Weighting-B Value for Blendind Operation
REGDEF_BIT(COL_CONST_MODE, 1)
REGDEF_BIT(ALPHA_CONST_MODE, 1)
REGDEF_BIT(ALPHA_PLANE_SRC, 1)
REGDEF_BIT(, 1)
REGDEF_END(GRPH_BLD_REG)

//0x14 Blending Control Register 2
#define GRPH_BLD2_REG_OFS                   0x14
REGDEF_BEGIN(GRPH_BLD2_REG)
REGDEF_BIT(BLD_WA, 8)               // Weighting-A Value for Blendind Operation (V plane)
REGDEF_BIT(BLD_WB, 8)               // Weighting-B Value for Blendind Operation (V plane)
REGDEF_BIT(COL_CONST_MODE, 1)
REGDEF_BIT(ALPHA_CONST_MODE, 1)
REGDEF_BIT(, 14)
REGDEF_END(GRPH_BLD2_REG)

//0x18 Accumulation Control Register
#define GRPH_ACCU_REG_OFS                   0x18
REGDEF_BEGIN(GRPH_ACCU_REG)
REGDEF_BIT(ACC_RESULT, 32)          // Accumulation Results
REGDEF_END(GRPH_ACCU_REG)

//0x1C Accumulation Control Register 2
#define GRPH_ACCU2_REG_OFS                  0x1C
REGDEF_BEGIN(GRPH_ACCU2_REG)
REGDEF_BIT(VLD_PX_CNT, 9)           // Valid Pixel Count for Accumulation Operation
REGDEF_BIT(, 7)
REGDEF_BIT(ACC_TO_DRAM, 1)          // Select ACC result to DRAM
REGDEF_BIT(, 15)
REGDEF_END(GRPH_ACCU2_REG)

//0x20 Image-1 INput Operation Register
#define GRPH_IN1_REG_OFS                    0x20
REGDEF_BEGIN(GRPH_IN1_REG)
REGDEF_BIT(IN1_OP, 3)               // Input Operation Selection
REGDEF_BIT(, 5)
REGDEF_BIT(INSHF_1, 4)              // Input Operation Shift Value
REGDEF_BIT(, 4)
REGDEF_BIT(INCONST_1, 8)            // Input Operation Constant Value
REGDEF_BIT(, 8)
REGDEF_END(GRPH_IN1_REG)

//0x24 Image-2 INput Operation Register
#define GRPH_IN2_REG_OFS                    0x24
REGDEF_BEGIN(GRPH_IN2_REG)
REGDEF_BIT(IN2_OP, 3)               // Input Operation Selection
REGDEF_BIT(, 5)
REGDEF_BIT(INSHF_2, 4)              // Input Operation Shift Value
REGDEF_BIT(, 4)
REGDEF_BIT(INCONST_2, 8)            // Input Operation Constant Value
REGDEF_BIT(, 8)
REGDEF_END(GRPH_IN2_REG)

//0x28 Image-3 OUTput Operation Register
#define GRPH_OUT3_REG_OFS                   0x28
REGDEF_BEGIN(GRPH_OUT3_REG)
REGDEF_BIT(OUT_OP, 3)            // Output Operation Selection
REGDEF_BIT(, 5)
REGDEF_BIT(OUTSHF, 4)            // Output Operation Shift Value
REGDEF_BIT(, 4)
REGDEF_BIT(OUTCONST, 8)          // Output Operation Constant Value
REGDEF_BIT(, 8)
REGDEF_END(GRPH_OUT3_REG)

//0x2C Reserved Register

//0x30 Image-1 INput Operation Register 2
#define GRPH_IN1_2_REG_OFS                  0x30
REGDEF_BEGIN(GRPH_IN1_2_REG)
REGDEF_BIT(IN1_OP, 3)               // Input Operation Selection
REGDEF_BIT(, 5)
REGDEF_BIT(INSHF_1, 4)              // Input Operation Shift Value
REGDEF_BIT(, 4)
REGDEF_BIT(INCONST_1, 8)            // Input Operation Constant Value
REGDEF_BIT(, 8)
REGDEF_END(GRPH_IN1_2_REG)

//0x34 Image-2 INput Operation Register 2
#define GRPH_IN2_2_REG_OFS                  0x34
REGDEF_BEGIN(GRPH_IN2_2_REG)
REGDEF_BIT(IN2_OP, 3)               // Input Operation Selection
REGDEF_BIT(, 5)
REGDEF_BIT(INSHF_2, 4)              // Input Operation Shift Value
REGDEF_BIT(, 4)
REGDEF_BIT(INCONST_2, 8)            // Input Operation Constant Value
REGDEF_BIT(, 8)
REGDEF_END(GRPH_IN2_2_REG)

//0x38 Image-3 OUTput Operation Register 2
#define GRPH_OUT3_2_REG_OFS                 0x38
REGDEF_BEGIN(GRPH_OUT3_2_REG)
REGDEF_BIT(OUT_OP, 3)               // Output Operation Selection
REGDEF_BIT(, 5)
REGDEF_BIT(OUTSHF, 4)               // Output Operation Shift Value
REGDEF_BIT(, 4)
REGDEF_BIT(OUTCONST, 8)             // Output Operation Constant Value
REGDEF_BIT(, 8)
REGDEF_END(GRPH_OUT3_2_REG)

//0x3C Reserved Register

//0x40 GOP8 line0 register 1
//0x48 GOP8 line1 register 1
//0x50 GOP8 line2 register 1
//0x58 GOP8 line3 register 1
#define GRPH_GOP8_LINE0_REG1_OFS            0x40
#define GRPH_GOP8_LINE1_REG1_OFS            0x48
#define GRPH_GOP8_LINE2_REG1_OFS            0x50
#define GRPH_GOP8_LINE3_REG1_OFS            0x58
REGDEF_BEGIN(GRPH_GOP8_LINE_REG1)
REGDEF_BIT(A_PARAM, 14)
REGDEF_BIT(rsv1, 1)
REGDEF_BIT(A_PARAM_SIGN_BIT, 1)
REGDEF_BIT(B_PARAM, 14)
REGDEF_BIT(rsv2, 1)
REGDEF_BIT(B_PARAM_SIGN_BIT, 1)
REGDEF_END(GRPH_GOP8_LINE_REG1)

//0x44 GOP8 line0 Register 2
//0x4C GOP8 line1 Register 2
//0x54 GOP8 line2 Register 2
//0x5C GOP8 line3 Register 2
#define GRPH_GOP8_LINE0_REG2_OFS            0x44
#define GRPH_GOP8_LINE1_REG2_OFS            0x4C
#define GRPH_GOP8_LINE2_REG2_OFS            0x54
#define GRPH_GOP8_LINE3_REG2_OFS            0x5C
REGDEF_BEGIN(GRPH_GOP8_LINE_REG2)
REGDEF_BIT(C_PARAM, 28)
REGDEF_BIT(rsv1, 2)
REGDEF_BIT(COMPARE, 2)
REGDEF_END(GRPH_GOP8_LINE_REG2)

//0x60 Constant Value Register
#define GRPH_CONST_REG_OFS                  0x60
REGDEF_BEGIN(GRPH_CONST_REG)
REGDEF_BIT(CONST_VAL, 32)           // Constant Value for multiple purposes
REGDEF_END(GRPH_CONST_REG)

//0x64 Constant Value Register 2
#define GRPH_CONST_REG2_OFS                 0x64
REGDEF_BEGIN(GRPH_CONST_REG2)
REGDEF_BIT(CONST_VAL2, 32)          // Constant Value for multiple purposes
REGDEF_END(GRPH_CONST_REG2)

//0x68 Constant Value Register 3
#define GRPH_CONST_REG3_OFS                 0x68
REGDEF_BEGIN(GRPH_CONST_REG3)
REGDEF_BIT(CONST_VAL3, 32)          // Constant Value for multiple purposes
REGDEF_END(GRPH_CONST_REG3)

//0x6C Constant Value Register 4
#define GRPH_CONST_REG4_OFS                 0x6C
REGDEF_BEGIN(GRPH_CONST_REG4)
REGDEF_BIT(CONST_VAL4, 32)          // Constant Value for multiple purposes
REGDEF_END(GRPH_CONST_REG4)

//0x70 Image-1 Buffer Address Register
#define GRPH_IMG1_ADDR_REG_OFS              0x70
REGDEF_BEGIN(GRPH_IMG1_ADDR_REG)
REGDEF_BIT(IMG1_ADDR, 31)           // Starting Address of Image-1
REGDEF_BIT(, 1)
REGDEF_END(GRPH_IMG1_ADDR_REG)

//0x74 Image-1 Buffer Line Offset Register
#define GRPH_IMG1_LOFF_REG_OFS              0x74
REGDEF_BEGIN(GRPH_IMG1_LOFF_REG)
REGDEF_BIT(IMG1_LOFF, 17)           // Line Offset of Image-1
REGDEF_BIT(, 15)
REGDEF_END(GRPH_IMG1_LOFF_REG)

//0x78 Image Active Height Register
#define GRPH_LCNT_REG_OFS                   0x78
REGDEF_BEGIN(GRPH_LCNT_REG)
REGDEF_BIT(ACT_HEIGHT, 14)          // Active Height
REGDEF_BIT(, 2)
REGDEF_BIT(ACT_HEIGHT2, 14)         // Active Height 2 (for warping usage only.)
REGDEF_BIT(, 2)
REGDEF_END(GRPH_LCNT_REG)

//0x7C Image Active Width Register
#define GRPH_XRGN_REG_OFS                   0x7C
REGDEF_BEGIN(GRPH_XRGN_REG)
REGDEF_BIT(ACT_WIDTH, 14)           // Active Width
REGDEF_BIT(, 2)
REGDEF_BIT(ACT_WIDTH2, 14)          // Active Width 2
REGDEF_BIT(, 2)
REGDEF_END(GRPH_XRGN_REG)

//0x80 Image-2 Buffer Address Register
#define GRPH_IMG2_ADDR_REG_OFS              0x80
REGDEF_BEGIN(GRPH_IMG2_ADDR_REG)
REGDEF_BIT(IMG2_ADDR, 31)           // Starting Address of Image-2
REGDEF_BIT(, 1)
REGDEF_END(GRPH_IMG2_ADDR_REG)

//0x84 Image-2 Buffer Line Offset Register
#define GRPH_IMG2_LOFF_REG_OFS              0x84
REGDEF_BEGIN(GRPH_IMG2_LOFF_REG)
REGDEF_BIT(IMG2_LOFF, 17)           // Line Offset of Image-2
REGDEF_BIT(, 15)
REGDEF_END(GRPH_IMG2_LOFF_REG)

//0x88~0x8C Reserved Register

//0x90 Image-3 Buffer Address Register
#define GRPH_IMG3_ADDR_REG_OFS              0x90
REGDEF_BEGIN(GRPH_IMG3_ADDR_REG)
REGDEF_BIT(IMG3_ADDR, 31)           // Starting Address of Image-1
REGDEF_BIT(, 1)
REGDEF_END(GRPH_IMG3_ADDR_REG)

//0x94 Image-3 Buffer Line Offset Register
#define GRPH_IMG3_LOFF_REG_OFS              0x94
REGDEF_BEGIN(GRPH_IMG3_LOFF_REG)
REGDEF_BIT(IMG3_LOFF, 17)           // Line Offset of Image-3
REGDEF_BIT(, 15)
REGDEF_END(GRPH_IMG3_LOFF_REG)

//0xA0 CST coefficient Register
#define GRPH_CST_COEFF_REG_OFS              0xA0
REGDEF_BEGIN(GRPH_CST_COEFF_REG)
REGDEF_BIT(CST_COEFF0, 8)
REGDEF_BIT(CST_COEFF1, 8)
REGDEF_BIT(CST_COEFF2, 8)
REGDEF_BIT(CST_COEFF3, 8)
REGDEF_END(GRPH_CST_COEFF_REG)

//0xA4 Accumulation Control Register (2nd)
#define GRPH_2ND_ACCU_REG_OFS               0xA4
REGDEF_BEGIN(GRPH_2ND_ACCU_REG)
REGDEF_BIT(ACC_RESULT, 32)          // Accumulation Results (for V plane)
REGDEF_END(GRPH_2ND_ACCU_REG)

//0xB0 GOP8 inner line0 register 1
//0xB8 GOP8 inner line1 register 1
//0xC0 GOP8 inner line2 register 1
//0xC8 GOP8 inner line3 register 1
#define GRPH_GOP8_INNER_LINE0_REG1_OFS            0xB0
#define GRPH_GOP8_INNER_LINE1_REG1_OFS            0xB8
#define GRPH_GOP8_INNER_LINE2_REG1_OFS            0xC0
#define GRPH_GOP8_INNER_LINE3_REG1_OFS            0xC8
REGDEF_BEGIN(GRPH_GOP8_INNER_LINE_REG1)
REGDEF_BIT(A_PARAM, 14)
REGDEF_BIT(rsv1, 1)
REGDEF_BIT(A_PARAM_SIGN_BIT, 1)
REGDEF_BIT(B_PARAM, 14)
REGDEF_BIT(rsv2, 1)
REGDEF_BIT(B_PARAM_SIGN_BIT, 1)
REGDEF_END(GRPH_GOP8_INNER_LINE_REG1)

//0xB4 GOP8 inner line0 Register 2
//0xBC GOP8 inner line1 Register 2
//0xC4 GOP8 inner line2 Register 2
//0xCC GOP8 inner line3 Register 2
#define GRPH_GOP8_INNER_LINE0_REG2_OFS            0xB4
#define GRPH_GOP8_INNER_LINE1_REG2_OFS            0xBC
#define GRPH_GOP8_INNER_LINE2_REG2_OFS            0xC4
#define GRPH_GOP8_INNER_LINE3_REG2_OFS            0xCC
REGDEF_BEGIN(GRPH_GOP8_INNER_LINE_REG2)
REGDEF_BIT(C_PARAM, 28)
REGDEF_BIT(rsv1, 2)
REGDEF_BIT(COMPARE, 2)
REGDEF_END(GRPH_GOP8_INNER_LINE_REG2)

//0xD0 LinkList Buffer Address Register
#define LL_DMA_ADDR_REG_OFS                 0xD0
REGDEF_BEGIN(LL_DMA_ADDR_REG)
REGDEF_BIT(ADDR, 32)                // Starting Address of LinkList
REGDEF_END(LL_DMA_ADDR_REG)

//0x100~0x220 LUT Registers
#define GRPH_LUT_REG_OFS                    0x100
REGDEF_BEGIN(GRPH_LUT_REG)
REGDEF_BIT(LUT0, 8)
REGDEF_BIT(LUT1, 8)
REGDEF_BIT(LUT2, 8)
REGDEF_BIT(LUT3, 8)
REGDEF_END(GRPH_LUT_REG)

//0x300~0x33C Palette Registers
#define GRPH_PAL_REG_OFS                    0x300
REGDEF_BEGIN(GRPH_PAL_REG)
REGDEF_BIT(B, 8)
REGDEF_BIT(G, 8)
REGDEF_BIT(R, 8)
REGDEF_BIT(A, 8)
REGDEF_END(GRPH_PAL_REG)

//0x380 DMA Control Register
#define GRPH_DMA_CTRL_REG_OFS			0x380
REGDEF_BEGIN(GRPH_DMA_CTRL_REG)
REGDEF_BIT(, 1)
REGDEF_BIT(DMA_DISABLE, 1)
REGDEF_BIT(DMA_IDLE, 1)
REGDEF_BIT(, 29)
REGDEF_END(GRPH_DMA_CTRL_REG)


#endif
