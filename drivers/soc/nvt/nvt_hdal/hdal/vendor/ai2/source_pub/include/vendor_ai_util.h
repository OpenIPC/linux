/**
	@brief Header file of utility functions.

	@file vendor_ai_util.h

	@ingroup vendor_ai

	@note Nothing.

	Copyright Novatek Microelectronics Corp. 2018.  All rights reserved.
*/
#ifndef _VENDOR_AI_UTIL_H_
#define _VENDOR_AI_UTIL_H_

/********************************************************************
	INCLUDE FILES
********************************************************************/
#include "hd_type.h"
#include "kwrap/nvt_type.h"

/********************************************************************
	MACRO CONSTANT DEFINITIONS
********************************************************************/
#define NET_UTIL_MODULE_VERSION 	"1.01.000"

#define NN_USE_DSP              	FALSE

#define NN_DEBUG                    DISABLE
#if NN_DEBUG
#define NN_MSG_SHOW                 ENABLE
#else
#define NN_MSG_SHOW                 DISABLE
#endif

#define MAX_PROC_CNT       			128

#undef MIN
#define MIN(a, b)           ((a) < (b) ? (a) : (b))
#undef MAX
#define MAX(a, b)           ((a) > (b) ? (a) : (b))
#undef ABS
#define ABS(a)              ((a) >= 0 ? (a) : (-a))
#undef CLAMP
#define CLAMP(x,min,max)    (((x) > (max)) ? (max) : (((x) > (min)) ? (x) : (min)))

#undef SWAP
#define SWAP(a, b, t)       (t) = (a); (a) = (b); (b) = (t)


#define DBG_COLOR_ERR "~R"
#define DBG_COLOR_WRN "~Y"
#define DBG_COLOR_IND ""
#define DBG_COLOR_MSG ""

#define DBG_ERR(fmtstr, args...) printf(DBG_COLOR_ERR "ERR:%s() " fmtstr, \
		__func__, ##args)

#define DBG_WRN(fmtstr, args...) printf(DBG_COLOR_WRN "WRN:%s() " fmtstr, \
		__func__, ##args)

#define DBG_IND(fmtstr, args...) printf(DBG_COLOR_IND "%s(): " fmtstr, \
		__func__, ##args)

#define DBG_DUMP(fmtstr, args...) printf(fmtstr, \
		##args)

#define CHKPNT    printf("\033[37mCHK: %d, %s\033[0m\r\n",__LINE__,__func__)    ///< Show a color sting of line count and function name in your insert codes
#define DBGD(x)   printf("\033[0;35m%s=%d\033[0m\r\n",#x,(int)(x))              ///< Show a color sting of variable name and variable deciaml value
#define DBGH(x)   printf("\033[0;35m%s=0x%08X\033[0m\r\n",#x,(unsigned int)(x)) ///< Show a color sting of variable name and variable hexadecimal value
#define DBGF(x)   printf("\033[0;35m%s=%f\033[0m\r\n",#x,(float)(x))            ///< Show a color sting of variable name and variable float value

/********************************************************************
	EXTERN VARIABLES & FUNCTION PROTOTYPES DECLARATIONS
********************************************************************/
#ifdef __cplusplus
extern "C" {
#endif

extern UINT8 *vendor_ais_getbuffrom(UINT8 **buftop, UINT32 nbyte);
extern HD_RESULT vendor_ais_readbin (UINT32 addr, UINT32 size, const CHAR *filename);
extern HD_RESULT vendor_ais_writebin(UINT32 addr, UINT32 size, const CHAR *filename);
extern INT32     vendor_ais_loadbin (UINT32 addr, const CHAR *filename);
extern HD_RESULT vendor_ais_readtxt (UINT32 addr, UINT32 line_len, UINT32 line_num, const CHAR *filename);
extern HD_RESULT vendor_ai_cpu_util_float2fixed (FLOAT *in_data, FLOAT in_scale_ratio, VOID *out_data, HD_VIDEO_PXLFMT out_fmt, INT32 data_size);
extern HD_RESULT vendor_ai_cpu_util_fixed2float (VOID *in_data, HD_VIDEO_PXLFMT in_fmt, FLOAT *out_data, FLOAT out_scale_ratio, INT32 data_size);
#ifdef __cplusplus
}
#endif

#endif  /* _VENDOR_AI_UTIL_H_ */
