/**
    JPEG bit rate control header file

    JPEG bit rate control header file.

    @file       jpeg_brc.h
    @ingroup
    @note       Nothing.

    Copyright   Novatek Microelectronics Corp. 2012.  All rights reserved.
*/

#ifndef _JPEGBRC_H
#define _JPEGBRC_H

#if defined (__UITRON)
#include "Type.h"
#elif defined __KERNEL__
#include "kwrap/type.h"
#include "kwrap/platform.h"
#include "../hal/jpg_int.h"
#include "../../include/jpeg.h"
#include <linux/module.h>
#include <linux/string.h>
#include <linux/kernel.h>

#include <linux/printk.h>
#define debug_msg        printk

#else
#include "kwrap/type.h"
#include "kwrap/platform.h"
#include "../hal/jpg_int.h"
#include "../../include/jpeg.h"
#include <string.h>

#endif

/**
    @addtogroup
*/
//@{

/**
    @name   Quality value limitation

    Minimum and maximum quality value for JPEG BRC

    Minimum and maximum quality value for JPEG BRC.
*/
//@{
#define JPGBRC_QUALITY_MIN          1       ///< Minimum quality (QF) value
#define JPGBRC_QUALITY_MAX          512     ///< Maximum quality (QF) value
//@}

/**
    JPEG BRC YUV type

    What kind of YUV type of source image.
*/
typedef enum {
	JPGBRC_COMBINED_YUV420,                 ///< Combined YUV420 (predict with a full Y/U/V 420 image)
	JPGBRC_COMBINED_YUV422,                 ///< Combined YUV422 (predict with a full Y/U/V 422 image)
	JPGBRC_SEPERATE_YUV420,                 ///< Seperate YUV420 (predit with YUV420 seperate phase), not supported in NT96650
	JPGBRC_SEPERATE_YUV422,                 ///< Seperate YUV420 (predit with YUV422 seperate phase), not supported in NT96650
	JPGBRC_YUV100,                          ///< YUV100 (Y Only)

	ENUM_DUMMY4WORD(JPGBRC_INPUT_RAW)
} JPGBRC_INPUT_RAW, *PJPGBRC_INPUT_RAW;

/**
    JPEG BRC predict result

    @note For JPEGBRC_PredictQuality()
*/
typedef enum {
	JPGBRC_RESULT_OK,                       ///< Prediction OK
	JPGBRC_RESULT_OVERFLOW,                 ///< Prediction overflow, predicted quality > JPGBRC_QUALITY_MAX
	JPGBRC_RESULT_UNDERFLOW,                ///< Prediction underflow, predicted quality < JPGBRC_QUALITY_MIN

	ENUM_DUMMY4WORD(JPGBRC_RESULT)
} JPGBRC_RESULT, *PJPGBRC_RESULT;

/**
    JPEG BRC parameter

    @note For JPEGBRC_PredictQuality()
*/
typedef struct {
	UINT32  width;                          ///< Image width
	UINT32  height;                         ///< Image height
	UINT32  ubound_byte;                     ///< Upper bound bit-stream size in byte
	UINT32  target_byte;                     ///< Target bit-stream size in byte
	UINT32  lbound_byte;                     ///< Lower bound bit-stream size in byte
} JPGBRC_PARAM, *PJPGBRC_PARAM;

/**
    JPEG BRC information

    JPEG BRC information include image size and target bit rate,
    current QF and bit-stream size...
*/
typedef struct {
	JPGBRC_INPUT_RAW    input_raw;           ///< YUV information of raw image (input parameter)
	UINT32              current_qf;         ///< current QF  (input parameter)
	UINT32              current_rate;       ///< current bit stream size  (input parameter)
	PJPGBRC_PARAM       pbrc_param;          ///< Bit rate control parameter  (input/output parameter)
	JPGBRC_RESULT       brc_result;          ///< Result of next QF prediction (output parameter)
} JPGBRC_INFO, *PJPGBRC_INFO;

// APIs
extern UINT32   jpgbrc_predict_quality(PJPGBRC_INFO p_brcinfo);

extern void     jpgbrc_gen_qtable_y(UINT32 quality, const UINT8 *p_std_ytbl, UINT8 *p_ytbl);
extern void     jpgbrc_gen_qtable_uv(UINT32 quality, const UINT8 *p_std_uvtbl, UINT8 *p_uvtbl);

#endif
