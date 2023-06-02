/**
 * @file kdrv_grph.h
 * @brief type definition of KDRV API.
 * @author ESW
 * @date in the year 2019
 */

#ifndef __KDRV_GRPH_H__
#define __KDRV_GRPH_H__

#include "kdrv_type.h"

#define GRPH_WIDTH_HEIGHT_MAX       ((1<<14) - 1)
#define GRPH_LINEOFFSET_MAX         ((1<<16) - 1)
#define __ALIGN_FLOOR_2(a)          ((a) & ~0x01)
#define __ALIGN_FLOOR_4(a)          ((a) & ~0x03)
#define __ALIGN_FLOOR_8(a)          ((a) & ~0x07)
#define __ALIGN_FLOOR_16(a)         ((a) & ~0x0F)

// AOP0
#define GRPH_AOP0_8BITS_WMIN        (4)				// unit: byte
#define GRPH_AOP0_8BITS_WMAX        (GRPH_WIDTH_HEIGHT_MAX)	// unit: byte
#define GRPH_AOP0_8BITS_WALIGN      (0)                         // unit: byte
#define GRPH_AOP0_8BITS_HMIN        (1)                         // unit: line
#define GRPH_AOP0_8BITS_HMAX        (GRPH_WIDTH_HEIGHT_MAX)     // unit: line
#define GRPH_AOP0_8BITS_HALIGN      (0)                         // unit: line
#define GRPH_AOP0_8BITS_ADDR_ALIGN  (0)                         // unit: byte



/* struct _KDRV_GRPH_EXEC_MODE: structure for KDRV_GRPH_PARAM_MODE
 *
 * This can be used to set kdrv_grph_trigger() to blocking or non-blocking mode
 */
typedef enum _KDRV_GRPH_EXEC_MODE {
	KDRV_GRPH_EXEC_NONBLOCKING,	// (default) Non-blocking mode
	KDRV_GRPH_EXEC_BLOCKING,	// Blocking mode
} KDRV_GRPH_EXEC_MODE;

/* struct _KDRV_GRPH_ABORT_MODE: structure for KDRV_GRPH_PARAM_ABORT
 *
 * This can be used to abort kdrv_grph_trigger()
 */
typedef enum _KDRV_GRPH_ABORT_MODE {
	KDRV_GRPH_ABORT_NONE,			// (default) NOP
	KDRV_GRPH_ABORT_CONFIRM = 0x55AA0066,	// Abort execution
} KDRV_GRPH_ABORT_MODE;

/* struct _KDRV_GRPH_PARAM_ID: structure for set/get functions
 *
 */
typedef enum _KDRV_GRPH_PARAM_ID {
	KDRV_GRPH_PARAM_FREQ,	///< [set/get] Frequency config. (unit: MHz)
				///<	valid setting: 240, 320, 480
	KDRV_GRPH_PARAM_MODE,   ///< [set/get] Execution mode
				///<	valid setting: refer to KDRV_GRPH_EXEC_MODE
	KDRV_GRPH_PARAM_ABORT,  ///< [set] Abort current graphic operation (force kdrv_grph_trigger abort)
				///<	valid setting: refer to KDRV_GRPH_EXEC_MODE
} KDRV_GRPH_PARAM_ID;



/*
 * structured passed to callback's 2nd parameter "user_data"
 */
typedef struct _KDRV_GRPH_EVENT_CB_INFO {
	UINT32       handle;     ///< handle feed by kdrv_grph_trigger()
	UINT32       timestamp;  ///< timestamp when job complete
	UINT32	     acc_result; ///< Computing result for GRPH_CMD_ACC
} KDRV_GRPH_EVENT_CB_INFO;

/*
 * Graphic operation command
 *
 * Select which operation for graphic engine operation.
 *
 * @note Used for KDRV_GRPH_TRIGGER_PARAM.
 */
typedef enum _GRPH_CMD {
	// GOP
	GRPH_CMD_ROT_90,                        ///< Rotate 90 degree clockwise
	///< - (GRPH_ID_2)
	///< - Format: GRPH_FORMAT_1BITS, GRPH_FORMAT_8BITS, GRPH_FORMAT_16BITS, GRPH_FORMAT_32BITS
	///< - Source image: GRPH_IMG_ID_A
	///< - Destination image: GRPH_IMG_ID_C
	GRPH_CMD_ROT_270,                       ///< Rotate 270 degree clockwise
	///< - (GRPH_ID_2)
	///< - Format: GRPH_FORMAT_8BITS, GRPH_FORMAT_16BITS, GRPH_FORMAT_32BITS
	///< - Source image: GRPH_IMG_ID_A
	///< - Destination image: GRPH_IMG_ID_C
	GRPH_CMD_ROT_180,                       ///< Rotate 180 degree
	///< - (GRPH_ID_2)
	///< - Format: GRPH_FORMAT_8BITS, GRPH_FORMAT_16BITS, GRPH_FORMAT_32BITS
	///< - Source image: GRPH_IMG_ID_A
	///< - Destination image: GRPH_IMG_ID_C
	GRPH_CMD_HRZ_FLIP,                      ///< Horizontal flip
	///< - (GRPH_ID_2)
	///< - Format: GRPH_FORMAT_8BITS, GRPH_FORMAT_16BITS, GRPH_FORMAT_32BITS
	///< - Source image: GRPH_IMG_ID_A
	///< - Destination image: GRPH_IMG_ID_C
	GRPH_CMD_VTC_FLIP,                      ///< Vertical flip
	///< - (GRPH_ID_2)
	///< - Format: GRPH_FORMAT_8BITS, GRPH_FORMAT_16BITS, GRPH_FORMAT_32BITS
	///< - Source image: GRPH_IMG_ID_A
	///< - Destination image: GRPH_IMG_ID_C
	GRPH_CMD_HRZ_FLIP_ROT_90,               ///< Horizontal flip + ROT90
	///< - (GRPH_ID_2)
	///< - Format: GRPH_FORMAT_8BITS, GRPH_FORMAT_16BITS, GRPH_FORMAT_32BITS
	///< - Source image: GRPH_IMG_ID_A
	///< - Destination image: GRPH_IMG_ID_C
	GRPH_CMD_HRZ_FLIP_ROT_270,              ///< Horizontal flip + ROT270
	///< - (GRPH_ID_2)
	///< - Format: GRPH_FORMAT_8BITS, GRPH_FORMAT_16BITS, GRPH_FORMAT_32BITS
	///< - Source image: GRPH_IMG_ID_A
	///< - Destination image: GRPH_IMG_ID_C
	GRPH_CMD_ROT_0 = 7,                     ///< Rotate 0 degree (NOT support)
	///< - (GRPH_ID_2)
	///< - Format: GRPH_FORMAT_1BITS, GRPH_FORMAT_8BITS, GRPH_FORMAT_16BITS, GRPH_FORMAT_32BITS
	///< - Source image: GRPH_IMG_ID_A
	///< - Destination image: GRPH_IMG_ID_C
	GRPH_CMD_VCOV = 8,                      ///< Video covering
	///< - (GRPH_ID_2)
	///< - Format: GRPH_FORMAT_8BITS, GRPH_FORMAT_16BITS
	///< - Source image: GRPH_IMG_ID_A
	///< - Source image: GRPH_IMG_ID_B (option if mosaic source from scaled down image)
	///< - Destination image: GRPH_IMG_ID_C
	GRPH_CMD_GOPMAX,                        //< Max GOP count

	// AOP
	GRPH_CMD_A_COPY = 0x10000,              ///< A -> destination
	///< - (GRPH_ID_1, GRPH_ID_2)
	///< - Format: GRPH_FORMAT_8BITS, GRPH_FORMAT_32BITS_ARGB8888_RGB, GRPH_FORMAT_32BITS_ARGB8888_A, GRPH_FORMAT_16BITS_ARGB1555_RGB, GRPH_FORMAT_16BITS_ARGB1555_A, GRPH_FORMAT_16BITS_ARGB4444_RGB, GRPH_FORMAT_16BITS_ARGB4444_A
	///< - GRPH_ID_1: GRPH_FORMAT_8BITS, GRPH_FORMAT_32BITS_ARGB8888_RGB, GRPH_FORMAT_32BITS_ARGB8888_A, GRPH_FORMAT_16BITS_ARGB1555_RGB, GRPH_FORMAT_16BITS_ARGB1555_A, GRPH_FORMAT_16BITS_ARGB4444_RGB, GRPH_FORMAT_16BITS_ARGB4444_A
	///< - GRPH_ID_2: GRPH_FORMAT_8BITS
	///< - Source image: GRPH_IMG_ID_A
	///< - Destination image: GRPH_IMG_ID_C
	GRPH_CMD_PLUS_SHF,                      ///< (A+(B>>SHF[1:0])) -> destination
	///< - (GRPH_ID_1 only)
	///< - Format: GRPH_FORMAT_8BITS, GRPH_FORMAT_16BITS, GRPH_FORMAT_16BITS_UVPACK, GRPH_FORMAT_16BITS_UVPACK_U, GRPH_FORMAT_16BITS_UVPACK_V
	///< - Source image A: GRPH_IMG_ID_A
	///< - Source image B: GRPH_IMG_ID_B
	///< - Destination image: GRPH_IMG_ID_C
	///< - Property: GRPH_PLUS_PROPTY()
	GRPH_CMD_MINUS_SHF,                     ///< (A-(B>>SHF[1:0])) -> destination
	///< - (GRPH_ID_1 only)
	///< - Format: GRPH_FORMAT_8BITS, GRPH_FORMAT_16BITS, GRPH_FORMAT_16BITS_UVPACK, GRPH_FORMAT_16BITS_UVPACK_U, GRPH_FORMAT_16BITS_UVPACK_V
	///< - Source image A: GRPH_IMG_ID_A
	///< - Source image B: GRPH_IMG_ID_B
	///< - Destination image: GRPH_IMG_ID_C
	///< - Property: GRPH_SUB_PROPTY()
	GRPH_CMD_COLOR_EQ,                      ///< (B == KEY) ? A:B -> destination with color key (=)
	///< - GRPH_ID_1: GRPH_FORMAT_8BITS, GRPH_FORMAT_16BITS, GRPH_FORMAT_16BITS_UVPACK, GRPH_FORMAT_16BITS_UVPACK_U, GRPH_FORMAT_16BITS_UVPACK_V, GRPH_FORMAT_32BITS_ARGB8888_RGB, GRPH_FORMAT_16BITS_ARGB1555_RGB, GRPH_FORMAT_16BITS_ARGB4444_RGB
	///< - GRPH_ID_2: GRPH_FORMAT_8BITS, GRPH_FORMAT_16BITS, GRPH_FORMAT_16BITS_UVPACK, GRPH_FORMAT_16BITS_UVPACK_U, GRPH_FORMAT_16BITS_UVPACK_V, GRPH_FORMAT_32BITS_ARGB8888_RGB
	///< - Source image A: GRPH_IMG_ID_A
	///< - Source image B: GRPH_IMG_ID_B
	///< - Destination image: GRPH_IMG_ID_C
	///< - Property: GRPH_COLOR_KEY_PROPTY()
	GRPH_CMD_COLOR_LE,                      ///< (B < KEY) ? A:B -> destination with color key (<)
	///< - (GRPH_ID_1, GRPH_ID_2)
	///< - GRPH_ID_1: GRPH_FORMAT_8BITS, GRPH_FORMAT_16BITS_UVPACK, GRPH_FORMAT_16BITS_UVPACK_U, GRPH_FORMAT_16BITS_UVPACK_V
	///< - GRPH_ID_2: GRPH_FORMAT_8BITS, GRPH_FORMAT_16BITS_UVPACK, GRPH_FORMAT_16BITS_UVPACK_U, GRPH_FORMAT_16BITS_UVPACK_V
	///< - Source image A: GRPH_IMG_ID_A
	///< - Source image B: GRPH_IMG_ID_B
	///< - Destination image: GRPH_IMG_ID_C
	///< - Property: GRPH_COLOR_KEY_PROPTY()
	GRPH_CMD_A_AND_B,                       ///< A & B -> destination
	///< - (GRPH_ID_1 only)
	///< - Format: GRPH_FORMAT_8BITS
	///< - Source image A: GRPH_IMG_ID_A
	///< - Source image B: GRPH_IMG_ID_B
	///< - Destination image: GRPH_IMG_ID_C
	GRPH_CMD_A_OR_B,                        ///< A | B -> destination
	///< - (GRPH_ID_1 only)
	///< - Format: GRPH_FORMAT_8BITS
	///< - Source image A: GRPH_IMG_ID_A
	///< - Source image B: GRPH_IMG_ID_B
	///< - Destination image: GRPH_IMG_ID_C
	GRPH_CMD_A_XOR_B,                       ///< A ^ B -> destination
	///< - (GRPH_ID_1 only)
	///< - Format: GRPH_FORMAT_8BITS
	///< - Source image A: GRPH_IMG_ID_A
	///< - Source image B: GRPH_IMG_ID_B
	///< - Destination image: GRPH_IMG_ID_C
	GRPH_CMD_TEXT_COPY,                     ///< text -> destination
	///< - (GRPH_ID_1, GRPH_ID_2)
	///< - Format:
	///< - GRPH_ID_1: GRPH_FORMAT_8BITS, GRPH_FORMAT_16BITS_UVPACK, GRPH_FORMAT_16BITS_UVPACK_U, GRPH_FORMAT_16BITS_UVPACK_V, GRPH_FORMAT_32BITS_ARGB8888_RGB, GRPH_FORMAT_32BITS_ARGB8888_A, GRPH_FORMAT_16BITS_ARGB1555_RGB, GRPH_FORMAT_16BITS_ARGB1555_A, GRPH_FORMAT_16BITS_ARGB4444_RGB, GRPH_FORMAT_16BITS_ARGB4444_A
	///< - GRPH_ID_2: GRPH_FORMAT_8BITS
	///< - Destination image: GRPH_IMG_ID_C
	GRPH_CMD_TEXT_AND_A,                    ///< text & A -> destination
	///< - (GRPH_ID_1 only)
	///< - Format: GRPH_FORMAT_8BITS, GRPH_FORMAT_16BITS_UVPACK, GRPH_FORMAT_16BITS_UVPACK_U, GRPH_FORMAT_16BITS_UVPACK_V
	///< - Source image A: GRPH_IMG_ID_A
	///< - Destination image: GRPH_IMG_ID_C
	GRPH_CMD_TEXT_OR_A,                     ///< text | A -> destination
	///< - (GRPH_ID_1 only)
	///< - Format: GRPH_FORMAT_8BITS, GRPH_FORMAT_16BITS_UVPACK, GRPH_FORMAT_16BITS_UVPACK_U, GRPH_FORMAT_16BITS_UVPACK_V
	///< - Source image A: GRPH_IMG_ID_A
	///< - Destination image: GRPH_IMG_ID_C
	GRPH_CMD_TEXT_XOR_A,                    ///< text ^ A -> destination
	///< - (GRPH_ID_1 only)
	///< - Format: GRPH_FORMAT_8BITS, GRPH_FORMAT_16BITS_UVPACK, GRPH_FORMAT_16BITS_UVPACK_U, GRPH_FORMAT_16BITS_UVPACK_V
	///< - Source image A: GRPH_IMG_ID_A
	///< - Destination image: GRPH_IMG_ID_C
	GRPH_CMD_TEXT_AND_AB,                   ///< (text & A) | (~text & B) -> destination
	///< - (GRPH_ID_1 only)
	///< - Format: GRPH_FORMAT_8BITS, GRPH_FORMAT_16BITS_UVPACK, GRPH_FORMAT_16BITS_UVPACK_U, GRPH_FORMAT_16BITS_UVPACK_V
	///< - Source image A: GRPH_IMG_ID_A
	///< - Source image B: GRPH_IMG_ID_B
	///< - Destination image: GRPH_IMG_ID_C
	GRPH_CMD_BLENDING,                      ///< ((A * WA) + (B * WB) + 128) >> 8 -> destination
	///< - (GRPH_ID_1, GRPH_ID_2)
	///< - Format:
	///< - GRPH_ID_1: GRPH_FORMAT_8BITS, GRPH_FORMAT_16BITS_UVPACK, GRPH_FORMAT_16BITS_UVPACK_U, GRPH_FORMAT_16BITS_UVPACK_V, GRPH_FORMAT_16BITS_RGB565, GRPH_FORMAT_32BITS_ARGB8888_RGB, GRPH_FORMAT_16BITS_ARGB1555_RGB, GRPH_FORMAT_16BITS_ARGB4444_RGB
	///< - GRPH_ID_2: GRPH_FORMAT_8BITS, GRPH_FORMAT_16BITS_UVPACK, GRPH_FORMAT_16BITS_UVPACK_U, GRPH_FORMAT_16BITS_UVPACK_V, GRPH_FORMAT_16BITS_RGB565, GRPH_FORMAT_32BITS_ARGB8888_RGB
	///< - Source image A: GRPH_IMG_ID_A
	///< - Source image B: GRPH_IMG_ID_B
	///< - Destination image: GRPH_IMG_ID_C
	///< - Property: GRPH_BLD_WA_WB_THR()
	GRPH_CMD_ACC,                           ///< Acc(A): pixel accumulation
	///< - (GRPH_ID_1 only)
	///< - Format: GRPH_FORMAT_8BITS, GRPH_FORMAT_16BITS
	///< - Source image A: GRPH_IMG_ID_A
	///< - Property: Not required
	GRPH_CMD_MULTIPLY_DIV,                  ///< (A * B + 1<<(DIV[2:0]-1))>>DIV[2:0] -> destination
	///< - (GRPH_ID_1 only)
	///< - Format: GRPH_FORMAT_8BITS, GRPH_FORMAT_16BITS, GRPH_FORMAT_16BITS_UVPACK, GRPH_FORMAT_16BITS_UVPACK_U, GRPH_FORMAT_16BITS_UVPACK_V
	///< - Source image A: GRPH_IMG_ID_A
	///< - Source image B: GRPH_IMG_ID_B
	///< - Destination image: GRPH_IMG_ID_C
	///< - Property: GRPH_MULT_PROPTY()
	GRPH_CMD_PACKING,                       ///< Pack(A,B)->C
	///< - (GRPH_ID_1 only)
	///< - Format: GRPH_FORMAT_8BITS
	///< - Source image A: GRPH_IMG_ID_A
	///< - Source image B: GRPH_IMG_ID_B
	///< - Destination image: GRPH_IMG_ID_C
	GRPH_CMD_DEPACKING,                     ///< Unpack(A) ->(B,C)
	///< - (GRPH_ID_1 only)
	///< - Format: GRPH_FORMAT_16BITS
	///< - Source image A: GRPH_IMG_ID_A
	///< - Destination image B: GRPH_IMG_ID_B
	///< - Destination image C: GRPH_IMG_ID_C
	GRPH_CMD_TEXT_MUL,                      ///< (A*text + 1<<(SHF-1)) >> SHF -> destination
	///< - (GRPH_ID_1 only)
	///< - Format: GRPH_FORMAT_8BITS, GRPH_FORMAT_16BITS_UVPACK, GRPH_FORMAT_16BITS_UVPACK_U, GRPH_FORMAT_16BITS_UVPACK_V, GRPH_FORMAT_32BITS_ARGB8888_RGB, GRPH_FORMAT_32BITS_ARGB8888_A, GRPH_FORMAT_16BITS_ARGB1555_RGB, GRPH_FORMAT_16BITS_ARGB4444_RGB, GRPH_FORMAT_16BITS_ARGB4444_A
	///< - Source image A: GRPH_IMG_ID_A
	///< - Source image B: GRPH_IMG_ID_B
	///< - Destination image: GRPH_IMG_ID_C
	///< - Property: GRPH_TEXT_MULT_PROPTY()
	GRPH_CMD_PLANE_BLENDING,                ///< (A*B + C*(256-B) + 128) >> 8 -> destination
	///< - (GRPH_ID_1 only)
	///< - Format: GRPH_FORMAT_8BITS, GRPH_FORMAT_16BITS_UVPACK, GRPH_FORMAT_16BITS_UVPACK_U, GRPH_FORMAT_16BITS_UVPACK_V, GRPH_FORMAT_16BITS_RGB565, GRPH_FORMAT_32BITS_ARGB8888_RGB, GRPH_FORMAT_16BITS_ARGB1555_RGB, GRPH_FORMAT_16BITS_ARGB4444_RGB
	///< - Source image A: GRPH_IMG_ID_A
	///< - Source image B: GRPH_IMG_ID_B
	///< - Source image C: GRPH_IMG_ID_C
	///< - Destination image: GRPH_IMG_ID_C
	///< - Property: GRPH_ALPHA_CONST()\n
	///<            GRPH_COLOR_CONST() if GRPH_FORMAT_8BITS, GRPH_FORMAT_16BITS_UVPACK, GRPH_FORMAT_16BITS_UVPACK_U, GRPH_FORMAT_16BITS_UVPACK_V\n
	///<            GRPH_ENABLE_ALPHA_FROM_A or GRPH_ENABLE_ALPHA_FROM_C if GRPH_FORMAT_32BITS_ARGB8888_RGB
	GRPH_CMD_1D_LUT = GRPH_CMD_PLANE_BLENDING + 3,  ///< 1D Look up table OP -> destinaion
	///< - (GRPH_ID_1 only)
	///< - Format: GRPH_FORMAT_8BITS, GRPH_FORMAT_16BITS_UVPACK, GRPH_FORMAT_16BITS_UVPACK_U, GRPH_FORMAT_16BITS_UVPACK_V
	///< - Source image A: GRPH_IMG_ID_A
	///< - Destination image: GRPH_IMG_ID_C
	GRPH_CMD_2D_LUT,                        ///< 2D Look up table OP -> destinaion
	///< - (GRPH_ID_1 only)
	///< - Format: GRPH_FORMAT_8BITS, GRPH_FORMAT_16BITS_UVPACK_U, GRPH_FORMAT_16BITS_UVPACK_V
	///< - Source image A: GRPH_IMG_ID_A
	///< - Source image B: GRPH_IMG_ID_B
	///< - Destination image: GRPH_IMG_ID_C
	GRPH_CMD_RGBYUV_BLEND,                  ///< RGB blending on YUV
	///< - (GRPH_ID_1 only)
	///< - Format: GRPH_FORMAT_16BITS_ARGB1555_RGB, GRPH_FORMAT_16BITS_ARGB4444_RGB, GRPH_FORMAT_16BITS_RGB565
	///< - Source image A: GRPH_IMG_ID_A (ARGB)
	///< - Source/Destination image B: GRPH_IMG_ID_B (Y)
	///< - Source/Destination image: GRPH_IMG_ID_C (UV pack)
	GRPH_CMD_RGBYUV_COLORKEY,               ///< RGB color key on YUV
	///< - (GRPH_ID_1 only)
	///< - Format: GRPH_FORMAT_16BITS_RGB565
	///< - Source image A: GRPH_IMG_ID_A (RGB)
	///< - Source/Destination image B: GRPH_IMG_ID_B (Y)
	///< - Source/Destination image: GRPH_IMG_ID_C (UV pack)
	GRPH_CMD_RGB_INVERT,                    ///< RGB invert
	///< - (GRPH_ID_1 only)
	///< - Format: GRPH_FORMAT_16BITS_RGB565, GRPH_FORMAT_32BITS_ARGB8888_RGB, GRPH_FORMAT_16BITS_ARGB4444_RGB, GRPH_FORMAT_16BITS_ARGB1555_RGB
	///< - Source image A: GRPH_IMG_ID_A (ARGB)
	///< - Destination image: GRPH_IMG_ID_C (ARGB)
	GRPH_CMD_AOPMAX,                        //< Max AOP count


	GRPH_CMD_MINUS_SHF_ABS = 0x10000 + 0x80, ///< abs(A-(B>>SHF[1:0])) -> destination
	///< - (GRPH_ID_1 only)
	///< - Format: GRPH_FORMAT_8BITS, GRPH_FORMAT_16BITS, GRPH_FORMAT_16BITS_UVPACK, GRPH_FORMAT_16BITS_UVPACK_U, GRPH_FORMAT_16BITS_UVPACK_V
	///< - Source image A: GRPH_IMG_ID_A
	///< - Source image B: GRPH_IMG_ID_B
	///< - Destination image: GRPH_IMG_ID_C
	///< - Property: GRPH_SUB_PROPTY()
	GRPH_CMD_COLOR_MR,                      ///< (B >= KEY) ? A:B -> destination with color key (>)
	///< - GRPH_ID_1: GRPH_FORMAT_8BITS, GRPH_FORMAT_16BITS_UVPACK, GRPH_FORMAT_16BITS_UVPACK_U, GRPH_FORMAT_16BITS_UVPACK_V
	///< - GRPH_ID_2: GRPH_FORMAT_8BITS, GRPH_FORMAT_16BITS_UVPACK, GRPH_FORMAT_16BITS_UVPACK_U, GRPH_FORMAT_16BITS_UVPACK_V
	///< - Source image A: GRPH_IMG_ID_A
	///< - Source image B: GRPH_IMG_ID_B
	///< - Destination image: GRPH_IMG_ID_C
	///< - Property: GRPH_COLOR_KEY_PROPTY()
	GRPH_CMD_COLOR_FILTER,                  ///< 3 channel color key filter
	///< - (GRPH_ID_1 only)
	///< - Format: GRPH_FORMAT_8BITS, GRPH_FORMAT_16BITS_UVPACK
	///< - When format is GRPH_FORMAT_8BITS, Source GRPH_IMG_ID_A/GRPH_IMG_ID_B/GRPH_IMG_ID_C, Destination: GRPH_IMG_ID_A/GRPH_IMG_ID_B/GRPH_IMG_ID_C.
	///< - When format is GRPH_FORMAT_16BITS_UVPACK, Source GRPH_IMG_ID_A/GRPH_IMG_ID_B, Destination: GRPH_IMG_ID_A/GRPH_IMG_ID_B.
	///< - Property: GRPH_COLOR_FLT_PROPTY()

	ENUM_DUMMY4WORD(GRPH_CMD)
} GRPH_CMD;

/*
 * Graphic data format
 *
 * Select data format of graphic operation.
 *
 * @note For KDRV_GRPH_TRIGGER_PARAM
 */
typedef enum _GRPH_FORMAT {
	GRPH_FORMAT_1BIT,                       ///< Normal 1 bits data
	GRPH_FORMAT_4BITS,                      ///< Normal 4 bits data
	GRPH_FORMAT_8BITS,                      ///< Normal 8 bits data
	GRPH_FORMAT_16BITS,                     ///< Normal 16 bits data
	GRPH_FORMAT_16BITS_UVPACK,              ///< 16 bits U/V pack data and apply to both plane
	GRPH_FORMAT_16BITS_UVPACK_U,            ///< 16 bits U/V pack data and apply to U plane only
	GRPH_FORMAT_16BITS_UVPACK_V,            ///< 16 bits U/V pack data and apply to V plane only
	GRPH_FORMAT_16BITS_RGB565,              ///< 16 bits RGB565 pack data and apply to all plane
	GRPH_FORMAT_32BITS,                     ///< Normal 32 bits data
	GRPH_FORMAT_32BITS_ARGB8888_RGB,        ///< 32 bits ARGB8888 pack data and apply to RGB plane only
	GRPH_FORMAT_32BITS_ARGB8888_A,          ///< 32 bits ARGB8888 pack data and apply to A plane only
	GRPH_FORMAT_16BITS_ARGB1555_RGB,        ///< 16 bits ARGB1555 pack data and apply to RGB only
	GRPH_FORMAT_16BITS_ARGB1555_A,          ///< 16 bits ARGB1555 pack data and apply to A(alpha) only
	GRPH_FORMAT_16BITS_ARGB4444_RGB,        ///< 16 bits ARGB4444 pack data and apply to RGB only
	GRPH_FORMAT_16BITS_ARGB4444_A,          ///< 16 bits ARGB4444 pack data and apply to A(alpha) only

	GRPH_FORMAT_PALETTE_1BIT,               ///< 1 bit palette
	GRPH_FORMAT_PALETTE_2BITS,              ///< 2 bits palette
	GRPH_FORMAT_PALETTE_4BITS,              ///< 4 bits palette

	ENUM_DUMMY4WORD(GRPH_FORMAT)
} GRPH_FORMAT;

/*
 * Graphic Image identifier
 *
 * Select Image A/B/C.
 *
 * @note For GRPH_IMG
 */
typedef enum _GRPH_IMG_ID {
	GRPH_IMG_ID_A,                          ///< Image A
	GRPH_IMG_ID_B,                          ///< Image B
	GRPH_IMG_ID_C,                          ///< Image C

	ENUM_DUMMY4WORD(GRPH_IMG_ID)
} GRPH_IMG_ID;

/*
 * Graphic Image YUV format
 *
 * Select YUV422 or YUV411
 *
 * @note For GRPH_PROPERTY_ID_YUVFMT
 */
typedef enum _GRPH_YUV_FMT {
	GRPH_YUV_422,                           ///< YUV 422
	GRPH_YUV_411,                           ///< YUV 411

	ENUM_DUMMY4WORD(GRPH_YUV_FMT)
} GRPH_YUV_FMT;

/*
 * Graphic UV subsampling
 *
 * Select co-cited or centered
 *
 * @note For GRPH_PROPERTY_ID_UV_SUBSAMPLE
 */
typedef enum _GRPH_UV_SUBSAMPLE {
	GRPH_UV_COCITED,			///< UV co-cited (pick top/left)
	GRPH_UV_CENTERED,			///< UV center (UV averaged)

	ENUM_DUMMY4WORD(GRPH_UV_SUBSAMPLE)
} GRPH_UV_SUBSAMPLE;

/*
 * Mosaic source format
 *
 * @note For GRPH_PROPERTY_ID_MOSAIC_SRC_FMT
 */
typedef enum {
	GRPH_MOSAIC_FMT_NORMAL,                 ///< Normal (the same format with GRPH_REQUEST.format)
	GRPH_MOSAIC_FMT_YUV444,                 ///< YUV444

	ENUM_DUMMY4WORD(GRPH_MOSAIC_FMT)
} GRPH_MOSAIC_FMT;

/*
 * Graphic GOP direction
 *
 * @note For GRPH_CMD_ROT_90
 */
typedef enum _GRPH_GOP_DIR {
	GRPH_GOP_DIR_VER_READ,			///< vertical read, horizontal write
	GRPH_GOP_DIR_HORI_READ,			///< horizontal read, vertical write

	ENUM_DUMMY4WORD(GRPH_GOP_DIR)
} GRPH_GOP_DIR;

/*
 * Graphic command property identifier
 *
 * Select property for which plane.
 *
 * @note For GRPH_PROPERTY
 */
typedef enum _GRPH_PROPERTY_ID {
	GRPH_PROPERTY_ID_NORMAL,                ///< property for normal plane
	GRPH_PROPERTY_ID_U,                     ///< property for U plane
	GRPH_PROPERTY_ID_V,                     ///< property for V plane
	GRPH_PROPERTY_ID_R,                     ///< property for R plane
	GRPH_PROPERTY_ID_G,                     ///< property for G plane
	GRPH_PROPERTY_ID_B,                     ///< property for B plane
	GRPH_PROPERTY_ID_A,                     ///< property for A(alpha) plane

	GRPH_PROPERTY_ID_ACC_SKIPCTRL,          ///< (OBSOLETE) Skip control for GRPH_CMD_ACC command. property can be any of GRPH_ACC_SKIP_CTL.
	GRPH_PROPERTY_ID_ACC_FULL_FLAG,         ///< (OBSOLETE) full flag for GRPH_CMD_ACC command. property will  return full flag of accumulation operation.
	GRPH_PROPERTY_ID_PIXEL_CNT,             ///< (OBSOLETE) pixel count for GRPH_CMD_ACC command. property will return pixel count of accumulation operation
	GRPH_PROPERTY_ID_VALID_PIXEL_CNT,       ///< (OBSOLETE) valid pixel count for GRPH_CMD_ACC command
	GRPH_PROPERTY_ID_ACC_RESULT,            ///< acc result for GRPH_CMD_ACC command
	GRPH_PROPERTY_ID_ACC_RESULT2,           ///< (2nd) acc result for V part of UV in GRPH_CMD_ACC command

	GRPH_PROPERTY_ID_LUT_BUF,               ///< Buffer stores LUT table referenced by GRPH_CMD_1D_LUT and GRPH_CMD_2D_LUT (word alignment).

	GRPH_PROPERTY_ID_YUVFMT,                ///< YUV format (for RGB + YUV operations). Valid setting can be:
						///< - @b GRPH_YUV_422: YUV422
						///< - @b GRPH_YUV_411: YUV411
	GRPH_PROPERTY_ID_ALPHA0_INDEX,          ///< For GRPH_CMD_RGBYUV_BLEND. Index (0~15) value when alpha (of ARGB1555) is 0
	GRPH_PROPERTY_ID_ALPHA1_INDEX,          ///< For GRPH_CMD_RGBYUV_BLEND. Index (0~15) value when alpha (of ARGB1555) is 1
	GRPH_PROPERTY_ID_INVRGB,                ///< YUV format (for RGB + YUV operations). Valid setting can be:
						///< - @b FALSE: RGB is NOT inverted (Normal)
						///< - @b TRUE: RGB is iverted before apply with YUV
	GRPH_PROPERTY_ID_PAL_BUF,		///< Buffer stores palette referenced by GRPH_CMD_RGBYUV_BLEND and GRPH_CMD_RGBYUV_COLORKEY (word alignment).
						///< If format is GRPH_FORMAT_PALETTE_1BIT, buffer size should be 2 words
						///< If format is GRPH_FORMAT_PALETTE_2BIT, buffer size should be 4 words
						///< If format is GRPH_FORMAT_PALETTE_4BIT, buffer size should be 16 words

	GRPH_PROPERTY_ID_QUAD_PTR,              ///< This property is a pointer to quadrilateral descriptor (GRPH_QUAD_DESC)
	GRPH_PROPERTY_ID_QUAD_INNER_PTR,        ///< This property is a pointer to inner quadrilateral descriptor (GRPH_QUAD_DESC)
	GRPH_PROPERTY_ID_MOSAIC_SRC_FMT,        ///< This property describes image format of mosaic source of GRPH_CMD_VCOV. Valid setting can be:
	///< - @b GRPH_MOSAIC_FMT_NORMAL: the same format with GRPH_REQUEST.format (default)
	///< - @b GRPH_MOSAIC_FMT_YUV444: image B is YUV444

	GRPH_PROPERTY_ID_UV_SUBSAMPLE,		///< UV subsampling (for GRPH_CMD_RGBYUV_BLEND) . Valid setting can be:
						///< - @b GRPH_UV_COCITED: UV co-cited
						///< - @b GRPH_UV_CENTERED: UV centered

	GRPH_PROPERTY_ID_GOP_DIR,		///< GOP0 read/write direction (for GRPH_CMD_ROT_90 and GRPH_CMD_HRZ_FLIP_ROT_90) . Valid setting can be:
						///< - @b GRPH_GOP_DIR_VER_READ: vertical read, horizontal write
						///< - @b GRPH_GOP_DIR_HORI_READ: horizontal read, vertical write

#if defined(_NVT_EMULATION_)
    GRPH_PROPERTY_ID_QUAD_COMP,				///< Engineer Usage: change comparative for quad edge
#endif

	ENUM_DUMMY4WORD(GRPH_PROPERTY_ID)
} GRPH_PROPERTY_ID;

/*
 * Graphic IN/OUT operation identifier
 *
 * Select In/OUT operation and select operated plane.
 *
 * @note For GRPH_INOUTOP
 */
typedef enum _GRPH_INOUT_ID {
	GRPH_INOUT_ID_IN_A,                     ///< IN operation on image A
	GRPH_INOUT_ID_IN_A_U,                   ///< IN operation on U plane of UV packed image A
	GRPH_INOUT_ID_IN_A_V,                   ///< IN operation on V plane of UV packed image A

	GRPH_INOUT_ID_IN_B,                     ///< IN operation on image B
	GRPH_INOUT_ID_IN_B_U,                   ///< IN operation on U plane of UV packed image B
	GRPH_INOUT_ID_IN_B_V,                   ///< IN operation on V plane of UV packed image B

	GRPH_INOUT_ID_OUT_C,                    ///< OUT operation on image C
	GRPH_INOUT_ID_OUT_C_U,                  ///< OUT operation on U plane of UV packed image C
	GRPH_INOUT_ID_OUT_C_V,                  ///< OUT operation on V plane of UV packed image C

	ENUM_DUMMY4WORD(GRPH_INOUT_ID)
} GRPH_INOUT_ID;

/*
 * Color key filter select
 *
 * @note For GRPH_COLOR_FLT_PROPTY
 */
typedef enum _GRPH_COLORKEY_FLTSEL {
	GRPH_COLORKEY_FLTSEL_0,                     ///< filter by FKEY1 <  Pixel_Value <  FKEY2
	GRPH_COLORKEY_FLTSEL_1,                     ///< filter by FKEY1 <= Pixel_Value <  FKEY2
	GRPH_COLORKEY_FLTSEL_2,                     ///< filter by FKEY1 <  Pixel_Value <= FKEY2

	ENUM_DUMMY4WORD(GRPH_COLORKEY_FLTSEL)
} GRPH_COLORKEY_FLTSEL;

/*
 * Color key filter mode
 *
 * @note For GRPH_COLOR_FLT_PROPTY
 */
typedef enum _GRPH_COLORKEY_FLTMODE {
	GRPH_COLORKEY_FLTMODE_BOTH,                 ///< Y1/U/V and Y2/U/V both match
	GRPH_COLORKEY_FLTMODE_OR,                   ///< Y1/U/V or Y2/U/V match
	GRPH_COLORKEY_FLTMODE_1ST,                  ///< Y1/U/V match
	GRPH_COLORKEY_FLTMODE_2ND,                  ///< Y2/U/V match

	ENUM_DUMMY4WORD(GRPH_COLORKEY_FLTMODE)
} GRPH_COLORKEY_FLTMODE;

/*
 * @name PLUS Operation Property
 *
 * Generate property value for plus operation (GRPH_CMD_PLUS_SHF).
 *
 * @param[in] SHF      Right shift value after plus. valid value: 0~3
 *
 * @note For property parameter work with aopMode = GRPH_CMD_PLUS_SHF. \n
 *	Only valid with 16 bits (non-pack) operation.
 */
//@{
#define GRPH_PLUS_PROPTY(SHF)   ((SHF)&0x3)         ///< GRPH_CMD_PLUS_SHF property
//@}

/*
 * @name SUB Operation Property
 *
 * Generate property value for sub operation (GRPH_CMD_MINUS_SHF/GRPH_CMD_MINUS_SHF_ABS).
 *
 * @param[in] SHF      Right shift value after plus. valid value: 0~3
 *
 * @note For property parameter work with aopMode = GRPH_CMD_MINUS_SHF or GRPH_CMD_MINUS_SHF_ABS. \n
 *	Only valid with 16 bits (non-pack) operation.
 */
//@{
#define GRPH_SUB_PROPTY(SHF)    ((SHF)&0x3)         ///< GRPH_CMD_MINUS_SHF/GRPH_CMD_MINUS_SHF_ABS property
//@}

/*
 * @name Color Key Operation Property
 *
 * Generate property value for color key operation (GRPH_CMD_COLOR_EQ/GRPH_CMD_COLOR_LE/GRPH_CMD_COLOR_MR).
 *
 * @param[in] COLOR    Color key value. valid value: 0~FF
 *
 * @note For property parameter work with aopMode = GRPH_CMD_COLOR_EQ or GRPH_CMD_COLOR_LE or GRPH_CMD_COLOR_MR. \n
 */
//@{
#define GRPH_COLOR_KEY_PROPTY(COLOR) ((COLOR)&0xFF)                  ///< GRPH_CMD_COLOR_EQ/GRPH_CMD_COLOR_LE/GRPH_CMD_COLOR_MR property
//@}

/*
 * @name Color Key Filter Operation Property
 *
 * Generate property value for color key filter operation (GRPH_CMD_COLOR_FILTER).
 * This property MACRO should be used when you need to apply color key filter on YUV image simultaneously.
 *
 * @param[in] FLT_SEL  Filter select
 *			- @b GRPH_COLORKEY_FLTSEL_0: filter by FKEY1 < Pixel_Value < FKEY2
 *			- @b GRPH_COLORKEY_FLTSEL_1: filter by FKEY1 <= Pixel_Value < FKEY2
 *			- @b GRPH_COLORKEY_FLTSEL_2: filter by FKEY1 < Pixel_Value <= FKEY2
 * @param[in] FLT_MODE Filter mode
 *			- @b GRPH_COLORKEY_FLTMODE_BOTH: Y1/U/V and Y2/U/V both match
 *			- @b GRPH_COLORKEY_FLTMODE_OR: Y1/U/V or Y2/U/V match
 *			- @b GRPH_COLORKEY_FLTMODE_1ST: Y1/U/V match
 *			- @b GRPH_COLORKEY_FLTMODE_2ND: Y2/U/V match
 * @param[in] COLORKEY Color key value. From LSB to MSB is Y, U, V
 *
 * @note For property parameter work with aopMode = GRPH_CMD_COLOR_FILTER.
 */
//@{
#define GRPH_COLOR_FLT_PROPTY(FLT_SEL, FLT_MODE, COLORKEY) ((((FLT_SEL)&0x3)<<30) | 0x20000000 | (((FLT_MODE)&0x3)<<26) | ((COLORKEY)&0xFFFFFF))    ///< GRPH_CMD_COLOR_LE/GRPH_CMD_COLOR_MR property
//@}



/*
 * @name Advanced blending control
 *
 * Select which blending control weighting & blending threshold for graphic engine operation.
 *
 * @param[in] WA       Blending weight of image A. valid value: 0~255
 * @param[in] WB       Blending weight of image B. valid value: 0~255
 * @param[in] THR      (OBSOLETE) Don't care by graphic driver
 *
 * @note For property parameter in graph_request() work with aopMode = GRPH_CMD_BLENDING.
 */
//@{
#define GRPH_BLD_WA_WB_THR(WA, WB, THR)	(((WA)&0xFF)|(((WB)&0xFF)<<8)|(((THR)&0xFF)<<16))  ///< Weighting is (WA/256) & (WB/256). THR is don't care field
//@}

/*
 * @name Accumulation Operation Property (OBSOLETE)
 *
 * Generate property value for accumulation operation (GRPH_CMD_ACC).
 *
 * @note NT96680 only support checksum mode (i.e. chksumMode = TRUE) . This property is not required.
 *
 * @param[in] MODE	Checksum mode select
 *			- @b FALSE: Disable checksum mode. i.e. accumulation mode
 *			- @b TRUE: Enable checksum mode
 * @param[in] THR	Threshold value for accumulation mode. (Valid when chksumMode==FALSE)
 *			Accumulation is only applied when pixel value >= THR.
 *			Valid value: 0~0xFFF
 *
 * @note For property parameter work with aopMode = GRPH_CMD_ACC.
 */
//@{
#define GRPH_ACC_PROPTY(MODE, THR) ((((MODE)&0x1)<<31) | ((THR)&0xFFF))     ///< GRPH_CMD_ACC property
//@}

/*
 * @name Multiplication Operation Property
 *
 * Generate property value for multiplication operation(GRPH_CMD_MULTIPLY_DIV).
 *
 * @param[in] DITHER_EN	Select dithering enable
 *				- @b FALSE: Disable dithering after multiplication
 *				- @b TRUE: Enable dithering after multiplication
 * @param[in] SQ1		SQ1 for dithering (valid when ditherEn==TRUE). Valid value: 0x0~0xF
 * @param[in] SQ2		SQ2 for dithering (valid when ditherEn==TRUE). Valid value: 0x0~0x7FFF
 * @param[in] SHF		Right shift value after multiplication. Valid value: 0x0~0xF
 *
 * @note For property parameter work with aopMode = GRPH_CMD_MULTIPLY_DIV.
 */
//@{
#define GRPH_MULT_PROPTY(DITHER_EN, SQ1, SQ2, SHF)   ((((DITHER_EN)&0x1)<<31) | (((SQ1)&0xF)<<24) | (((SQ2)&0x7FFF)<<8) | ((SHF)&0xF))    ///< GRPH_CMD_MULTIPLY_DIV property
//@}

/*
 * @name Text MUL control
 *
 * Text MUL control for GRPH_CMD_TEXT_MUL
 *
 * - Byte Mode: C[7..0] = A*Text[11..4] >> SHF[3..0]
 *		-# A and C support IN/OUT operation respectively.
 *		-# Text supports signed/unsigned value.
 * - (OBSOLETE) HWORD Mode: C[15..0] = (A*Text[11..4] >> SHF[3..0]) + B[15..0]
 *		-# A support IN operation.
 *		-# C[15..0] & B[15..0] are signed value.
 *		-# Text supports signed/unsigned value.
 */
//@{
#define GRPH_TEXTMUL_BYTE         0x00000000    ///< Byte Mode. (Exclusive use with GRPH_TEXTMUL_HWORD)
#define GRPH_TEXTMUL_HWORD        0x80000000    ///< (OBSOLETE) Half-word Mode. (Exclusive use with GRPH_TEXTMUL_BYTE)

#define GRPH_TEXTMUL_SIGNED       0x40000000    ///< Signed Multiply (Exclusive use with GRPH_TEXTMUL_UNSIGNED)
#define GRPH_TEXTMUL_UNSIGNED     0x00000000    ///< Un-signed Multiply (Exclusive use with GRPH_TEXTMUL_SIGNED)
//@}

/*
 * @name Text Multiplication Operation Property
 *
 * Generate property value for text multiplication operation (GRPH_CMD_TEXT_MUL).
 *
 * @param[in] PRECISION	Select output precision (Only support byte mode)
 *				- @b GRPH_TEXTMUL_BYTE: Output 8 bits data (Byte mode)
 * @param[in] SIGNED		Select signed of CTEX
 *				- @b GRPH_TEXTMUL_SIGNED: CTEX is signed value
 *				- @b GRPH_TEXTMUL_UNSIGNED: CTEX is unsigned value
 * @param[in] CTEX		Constant value to multiply with pixels. Valid value: 0x00~0xFF
 * @param[in] SHF		Right shift value after multiplication. Valid value: 0x0~0xF
 *
 *  @note For property parameter work with aopMode = GRPH_CMD_TEXT_MUL.
 */
//@{
#define GRPH_TEXT_MULT_PROPTY(PRECISION, SIGNED, CTEX, SHF)  (((PRECISION)&GRPH_TEXTMUL_HWORD) | ((SIGNED)&GRPH_TEXTMUL_SIGNED) | (((CTEX)&0xFF)<<4) | ((SHF)&0xF))    ///< GRPH_CMD_TEXT_MUL property
//@}

/*
 * @name Plane-Blending
 *
 * Plane-Blending for GRPH_CMD_PLANE_BLENDING
 *
 * Select the operation mode for plane blending operation.
 */
//@{
#define GRPH_ENABLE_COLOR_CONST     0x10000     ///< Enable constant color source instead of image A.
///< If constant color is enabled, GRPH_COLOR_CONST() will decide color source.
#define GRPH_ENABLE_ALPHA_CONST     0x20000     ///< Enable constant alpha source instead of image B.
///< If constant alpha is enabled, GRPH_ALPHA_CONST() will decide color source.
#define GRPH_ENABLE_ALPHA_SRC       0x40000     ///< Select alpha from image C (for GRPH_FORMAT_32BITS_ARGB8888_RGB)
///< Else default will select alpha from image A
#define GRPH_ENABLE_ALPHA_FROM_C    0x40000     ///< Select alpha from image C (for GRPH_FORMAT_32BITS_ARGB8888_RGB)
#define GRPH_ENABLE_ALPHA_FROM_A    0x00000     ///< Select alpha from image A (for GRPH_FORMAT_32BITS_ARGB8888_RGB)
#define GRPH_COLOR_ALPHA_MASK       (GRPH_ENABLE_COLOR_CONST|GRPH_ENABLE_ALPHA_CONST)   ///< Bit mask to filter GRPH_ENABLE_COLOR_CONST and GRPH_ENABLE_ALPHA_CONST

#define GRPH_COLOR_CONST(x)         ((((x) & 0xFF) <<  0) | GRPH_ENABLE_COLOR_CONST)///< Constant color when GRPH_ENABLE_COLOR_CONST is enabled
#define GRPH_ALPHA_CONST(x)         ((((x) & 0xFF) <<  8) | GRPH_ENABLE_ALPHA_CONST)///< Constant alpha when GRPH_ENABLE_ALPHA_CONST is enabled
#define GRPH_CONST_MASK             0x0000FFFF          ///< Bit mask to filter constant color/alpha
//@}

/**
 * RGB invert Property
 *
 * Generate property value for RGB invert (GRPH_CMD_RGBYUV_BLEND,
 * GRPH_CMD_RGBYUV_COLORKEY, GRPH_CMD_RGB_INVERT).
 *
 * Assume input RGB value is A, and max value is MAX_RGB (ex: MAX_RGB is 32
 * if ARGB1555).
 * If (abs(A - ~A) < threshold), output result will be ajusted from ~A.
 * Else output result will be ~A.
 *
 * @param[in] threshold    Select invert threshold.
 * @param[in] inv_alpha    Select if alpha should be inverted
 *                          - @b FALSE: skip alpha
 *                          - @b TRUE: alpha should be inverted
 *
 * @note For property parameter work with aopMode = GRPH_CMD_RGBYUV_BLEND or
 *	GRPH_CMD_RGBYUV_COLORKEY or GRPH_CMD_RGB_INVERT.
 */
#define GRPH_RGB_INV_PROPTY(threshold, inv_alpha)  (((threshold)&0xFF) | ((inv_alpha)<<8))

/*
 * INPUT operation control
 *
 * Definition for the INPUT operation control.
 *
 * @note For GRPH_INOUTOP.
 */
typedef enum _GRPH_INOPCTL {
	GRPH_INOP_NONE,         ///< None:              A -> A
	GRPH_INOP_INVERT,       ///< Invert:            A -> ~A
	GRPH_INOP_SHIFTR_ADD,   ///< Shift right & Add: A -> ((A>>SHF[3..0]) + Constant)
	GRPH_INOP_SHIFTL_ADD,   ///< Shift left & Add:  A -> ((A<<SHF[3..0]) + Constant)
	GRPH_INOP_SHIFTL_SUB,   ///< Shift left & Sub:  A -> ((A<<SHF[3..0]) - Constant)

	ENUM_DUMMY4WORD(GRPH_INOPCTL)
} GRPH_INOPCTL;

/*
 * OUTPUT operation control
 *
 * Definition for the OUTPUT operation control.
 *
 * @note For GRPH_INOUTOP.
 */
typedef enum _GRPH_OUTOPCTL {
	GRPH_OUTOP_SHF,                 ///< Shift:         C = (C >> SHF[3..0])
	GRPH_OUTOP_INVERT,              ///< Invert:        C = ~C
	GRPH_OUTOP_ADD,                 ///< Add:           C = C + Constant
	GRPH_OUTOP_SUB,                 ///< Sub:           C = C - Constant
	GRPH_OUTOP_ABS,                 ///< Absolute       C = ABS(C)

	ENUM_DUMMY4WORD(GRPH_OUTOPCTL)
} GRPH_OUTOPCTL;

/* struct: _GRPH_INOUTOP: structure to describe in/out operations
 * Graphic INPUT/OUTPUT operation descriptor
 *
 * Structure for the INPUT/OUTPUT operation.
 *
 * @note For GRPH_IMG.
 */
typedef struct _GRPH_INOUTOP {
	GRPH_INOUT_ID   id;             ///< Identifier for input operation
	UINT32          op;     ///< Input/Output Operation Control. Can be:
	///< - @b GRPH_INOPCTL: if id indicates an IN operation
	///< - @b GRPH_OUTOPCTL: if id indicates an OUT operation
	UINT32          shifts;         ///< Shift Value for Operation.
	UINT32          constant;       ///< Constant value for Operation

	struct _GRPH_INOUTOP *p_next;   ///< Link to next IN/OUT OP. (Fill NULL if this is last item)
} GRPH_INOUTOP, *PGRPH_INOUTOP;

/* struct: _GRPH_IMG: structure to describe a image
 * Graphic image structure
 *
 * Description of graphic image
 *
 * @note For KDRV_GRPH_TRIGGER_PARAM
 */
typedef struct _GRPH_IMG {
	GRPH_IMG_ID img_id;             ///< Image identifier
	UINT32 dram_addr;               ///< Image DRAM address (unit: byte)
	UINT32 lineoffset;              ///< Image lineoffset (unit: byte) (Must be 4 byte alignment)
	UINT32 width;                   ///< Image width (unit: byte)
	UINT32 height;                  ///< Image height (unit: line)
	PGRPH_INOUTOP p_inoutop;        ///< List of IN/OUT operation for this image (NULL if this is not required)

	struct _GRPH_IMG *p_next;          ///< Link to next image. (Fill NULL if this is last item)
} GRPH_IMG, *PGRPH_IMG;

/* struct _GRPH_PROPERTY: structure to describe additional properties
 * Graphic command property description
 *
 * Description property value for each command
 *
 * @note For KDRV_GRPH_TRIGGER_PARAM
 */
typedef struct _GRPH_PROPERTY {
	GRPH_PROPERTY_ID id;            ///< Property identifier
	UINT32 property;                ///< Property value

	struct _GRPH_PROPERTY *p_next;  ///< Link to next property description. (Fill NULL if this is last item)
} GRPH_PROPERTY, *PGRPH_PROPERTY;

/* struct _GRPH_CKEYFILTER: structure to describe color key filter
 * ColorKey filter control
 *
 * Definition for the color key filter control.
 *
 * @note For KDRV_GRPH_TRIGGER_PARAM
 */
typedef struct _GRPH_CKEYFILTER {
	UINT32   FKEY1;         ///< color key filter key 1
	UINT32   FKEY2;         ///< color key filter key 2
	UINT32   RKEY;          ///< color key filter replace key
} GRPH_CKEYFILTER, *PGRPH_CKEYFILTER;

/* struct _GRPH_POINT_DESC: structure to describe a point in quadrilateral
 * Point Descriptor
 *
 * @note For GRPH_QUAD_DESC
 */
typedef struct _GRPH_POINT_DESC {
	INT32   x;          ///< x coordinator
	INT32   y;          ///< y coordinator
//	KDRV_IPOINT	ipoint;	///< point coordinator

	struct GRPH_POINT_DESC *p_next;
} GRPH_POINT_DESC, *PGRPH_POINT_DESC;


/* struct _GRPH_QUAD_DESC: structure to describe a quadrilateral
 *
 * The described quadrilateral should be inside a 4k * 4K region
 *
 * If mosaic_width and mosaic_height are not zero, the region will be filled with mosaic effect.
 * Else the region will be filled with constant blending with background image (image A)
 *
 * @note For GRPH_PROPERTY_ID_QUAD_PTR
 */
typedef struct _GRPH_QUAD_DESC {
	BOOL   blend_en;                ///< enable/disable blend with background color
					///< - @b TRUE: enable blending
					///< - @b FALSE: disable blending
	UINT32 alpha;                   ///< alpha value for blending

	UINT32 mosaic_width;            ///< mosaic width (unit: byte)
	UINT32 mosaic_height;           ///< mosaic height (unit: line)

	GRPH_POINT_DESC top_left;       ///< point 0 (left/top)
	GRPH_POINT_DESC top_right;      ///< point 1 (right/top)
	GRPH_POINT_DESC bottom_right;   ///< point 2 (right/bottom)
	GRPH_POINT_DESC bottom_left;    ///< point 3 (left/bottom)

	struct GRPH_QUAD_DESC *p_next;
} GRPH_QUAD_DESC, *PGRPH_QUAD_DESC;

/* struct  _KDRV_GRPH_TRIGGER_PARAM: structure to describe a job
 *
 * @note For kdrv_grph_trigger()
 */
typedef struct _KDRV_GRPH_TRIGGER_PARAM {
	UINT32		 ver;		///< driver version (reserved)
	GRPH_CMD         command;	///< Graphic Command
	GRPH_FORMAT	 format;	///< format of operated images
	GRPH_IMG         *p_images;	///< Description of images for this command
	GRPH_PROPERTY    *p_property;	///< Descriptions of properties
	GRPH_CKEYFILTER  *p_ckeyfilter;	///< ColorKey filter control info. Valid when command is GRPH_AOP_COLOR_LE or GRPH_AOP_COLOR_MR and property is GRPH_COLOR_FLT_PROPTY
	BOOL		 is_skip_cache_flush;	///< Inform driver to skip flushing cache (instead YOU should do it). 0: normal, 1: skip cache flush

	struct _KDRV_GRPH_TRIGGER_PARAM *p_next;
} KDRV_GRPH_TRIGGER_PARAM;



/*!
 * @fn INT32 kdrv_grph_open(UINT32 chip, UINT32 engine)
 * @brief open hardware engine
 * @param chip      the chip id of hardware
 * @param engine    the engine id of hardware
 *			- @b KDRV_GFX2D_GRPH0: graphic engine 0
 *			- @b KDRV_GFX2D_GRPH1: graphic engine 1
 * @return return 0 on success, -1 on error
 */
INT32 kdrv_grph_open(UINT32 chip, UINT32 engine);

/*!
 * @fn INT32 kdrv_grph_close(UINT32 chip, UINT32 engine)
 * @brief close hardware engine
 * @param chip      the chip id of hardware
 * @param engine    the engine id of hardware
 *			- @b KDRV_GFX2D_GRPH0: graphic engine 0
 *			- @b KDRV_GFX2D_GRPH1: graphic engine 1
 * @return return 0 on success, -1 on error
 */
INT32 kdrv_grph_close(UINT32 chip, UINT32 engine);

/*!
 * @fn INT32 kdrv_grph_trigger(INT32 handler, KDRV_GRPH_TRIGGER_PARAM *p_param,
				KDRV_CALLBACK_FUNC *p_cb_func,
				VOID *p_user_data);
 * @brief trigger hardware engine
 * @param id			the id of hardware
 * @param p_param               the parameter for trigger
 * @param p_cb_func             the callback function
 * @param p_user_data           the private user data
 * @return return 0 on success, -1 on error
 */
INT32 kdrv_grph_trigger(UINT32 id, KDRV_GRPH_TRIGGER_PARAM *p_param,
						KDRV_CALLBACK_FUNC *p_cb_func,
						VOID *p_user_data);

/*!
 * @fn INT32 kdrv_grph_get(UINT32 handler, KDRV_GRPH_PARAM_ID id, VOID *p_param)
 * @brief get parameters from hardware engine
 * @param id		the id of hardware
 * @param param_id	the id of parameters
 * @param p_param	the parameters
 * @return return 0 on success, -1 on error
 */
INT32 kdrv_grph_get(UINT32 id, KDRV_GRPH_PARAM_ID param_id, VOID *p_param);

/*!
 * @fn INT32 kdrv_grph_set(UINT32 handler, KDRV_GRPH_PARAM_ID id, VOID *p_param)
 * @brief set parameters to hardware engine
 * @param id        the id of hardware
 * @param param_id  the id of parameters
 * @param p_param   the parameters
 * @return return 0 on success, -1 on error
 */
INT32 kdrv_grph_set(UINT32 id, KDRV_GRPH_PARAM_ID param_id, VOID *p_param);


#endif
