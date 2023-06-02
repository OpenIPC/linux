/**
	@brief Header file of utility functions of vendor net sample.

	@file net_util_sample.h

	@ingroup net_util_sample

	@note Nothing.

	Copyright Novatek Microelectronics Corp. 2018.  All rights reserved.
*/
#ifndef _VENDOR_AI_NET_UTIL_H_
#define _VENDOR_AI_NET_UTIL_H_

/********************************************************************
	INCLUDE FILES
********************************************************************/
#include "hd_type.h"
#include "kwrap/nvt_type.h"

/********************************************************************
	MACRO CONSTANT DEFINITIONS
********************************************************************/
#define NET_UTIL_MODULE_VERSION 	"1.00.002"

#define NN_TEST_POWER               DISABLE
#define NN_DEBUG                    DISABLE
#define TEST_POWER_SLEEP_TM         (25000)     //(0)

#if NN_TEST_POWER
#define NN_MSG_SHOW                 DISABLE
#else
#if NN_DEBUG
#define NN_MSG_SHOW                 ENABLE
#else
#define NN_MSG_SHOW                 DISABLE
#endif
#endif
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

#if NN_MSG_SHOW
#define DBG_MSG(fmtstr, args...)    printf(fmtstr, ##args)

#define DBG_DUMP(fmtstr, args...)   printf(fmtstr, ##args)
#else
#define DBG_MSG(fmtstr, args...)

#define DBG_DUMP(fmtstr, args...)   printf(fmtstr, ##args)
#endif

#define CHKPNT    printf("\033[37mCHK: %d, %s\033[0m\r\n",__LINE__,__func__)    ///< Show a color sting of line count and function name in your insert codes
#define DBGD(x)   printf("\033[0;35m%s=%d\033[0m\r\n",#x,(int)(x))              ///< Show a color sting of variable name and variable deciaml value
#define DBGH(x)   printf("\033[0;35m%s=0x%08X\033[0m\r\n",#x,(unsigned int)(x)) ///< Show a color sting of variable name and variable hexadecimal value

/********************************************************************
	EXTERN VARIABLES & FUNCTION PROTOTYPES DECLARATIONS
********************************************************************/

#endif  /* _VENDOR_AI_NET_UTIL_H_ */
