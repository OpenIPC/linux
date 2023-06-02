/**
	@brief Header file of vendor videodec module.\n
	This file contains the functions which is related to vendor videodec.

	@file vendor_videodec.h

	@ingroup mhdal

	@note Nothing.

	Copyright Novatek Microelectronics Corp. 2018.  All rights reserved.
*/

#ifndef _VENDOR_VIDEODEC_H_
#define _VENDOR_VIDEODEC_H_

/********************************************************************
	INCLUDE FILES
********************************************************************/
#include "hd_type.h"

/********************************************************************
	MACRO CONSTANT DEFINITIONS
********************************************************************/

/********************************************************************
	MACRO FUNCTION DEFINITIONS
********************************************************************/

/********************************************************************
	TYPE DEFINITION
********************************************************************/
//------
typedef enum _VENDOR_VIDEODEC_PARAM_ID {
	VENDOR_VIDEODEC_PARAM_IN_YUV_AUTO_DROP,      ///< CARDV only.  auto drop yuv if raw queue full
	VENDOR_VIDEODEC_PARAM_IN_RAWQUE_MAX_NUM,
	VENDOR_VIDEODEC_PARAM_OUT_STATUS,            ///< CARDV only.  return decode status is 1: start/ 0: stop
	ENUM_DUMMY4WORD(VENDOR_VIDEODEC_PARAM_ID)
} VENDOR_VIDEODEC_PARAM_ID;

typedef struct _VENDOR_VIDEODEC_OUT {
	UINT32 dec_status;                     ///< decoder status
} VENDOR_VIDEODEC_OUT;

typedef struct _VENDOR_VIDEODEC_YUV_AUTO_DROP {
	BOOL enable;                     ///< yuv auto drop 
} VENDOR_VIDEODEC_YUV_AUTO_DROP;

typedef struct _VENDOR_VIDEODEC_RAWQUE_MAX_NUM {
	UINT32 rawque_max_num;                      ///< raw que max number
} VENDOR_VIDEODEC_RAWQUE_MAX_NUM;

/********************************************************************
	EXTERN VARIABLES & FUNCTION PROTOTYPES DECLARATIONS
********************************************************************/
HD_RESULT vendor_videodec_set(HD_PATH_ID path_id, VENDOR_VIDEODEC_PARAM_ID id, VOID *p_param);
HD_RESULT vendor_videodec_get(HD_PATH_ID path_id, VENDOR_VIDEODEC_PARAM_ID id, VOID *p_param);
#endif

