/**
	@brief Header file of vendor videoprocess module.\n
	This file contains the functions which is related to vendor videoprocess.

	@file vendor_videoprocess.h

	@ingroup mhdal

	@note Nothing.

	Copyright Novatek Microelectronics Corp. 2020.  All rights reserved.
*/

#ifndef _VENDOR_VIDEOPROCESS_H_
#define _VENDOR_VIDEOPROCESS_H_

/********************************************************************
	INCLUDE FILES
********************************************************************/
#include "hd_type.h"
#include "hd_common.h"
#include "hd_videoprocess.h"
/********************************************************************
	MACRO CONSTANT DEFINITIONS
********************************************************************/

/********************************************************************
	MACRO FUNCTION DEFINITIONS
********************************************************************/

/********************************************************************
	TYPE DEFINITION
********************************************************************/

typedef enum _VENDOR_VIDEOPROC_PARAM_ID {
	VENDOR_VIDEOPROC_PARAM_HEIGHT_ALIGN,      ///< using device id, refer to VENDOR_VIDEOPROC_USRDATA_CFG struct
	VENDOR_VIDEOPROC_PARAM_IN_DEPTH,	  	///< using in id
	VENDOR_VIDEOPROC_PARAM_DMA_ABORT,		///< using device id, do DMA abort.
	ENUM_DUMMY4WORD(VENDOR_VIDEOPROC_PARAM_ID)
} VENDOR_VIDEOPROC_PARAM_ID;
/********************************************************************
	EXTERN VARIABLES & FUNCTION PROTOTYPES DECLARATIONS
********************************************************************/
HD_RESULT vendor_videoproc_set(UINT32 id, VENDOR_VIDEOPROC_PARAM_ID param_id, VOID *p_param);
#endif

