/**
	@brief Header file of vendor audiodec module.\n
	This file contains the functions which is related to vendor audiodec.

	@file vendor_audiodec.h

	@ingroup mhdal

	@note Nothing.

	Copyright Novatek Microelectronics Corp. 2018.  All rights reserved.
*/

#ifndef _VENDOR_AUDIODEC_H_
#define _VENDOR_AUDIODEC_H_

/********************************************************************
	INCLUDE FILES
********************************************************************/
#include "hd_type.h"

/********************************************************************
	MACRO CONSTANT DEFINITIONS
********************************************************************/
#define VENDOR_AUDIODEC_NAME_LEN 32

/********************************************************************
	MACRO FUNCTION DEFINITIONS
********************************************************************/

/********************************************************************
	TYPE DEFINITION
********************************************************************/
typedef enum _VENDOR_AUDIODEC_ITEM {
	VENDOR_AUDIODEC_ITEM_CODEC_HEADER,      ///< codec header
	ENUM_DUMMY4WORD(VENDOR_AUDIODEC_ITEM)
} VENDOR_AUDIODEC_ITEM;

/********************************************************************
	EXTERN VARIABLES & FUNCTION PROTOTYPES DECLARATIONS
********************************************************************/
HD_RESULT vendor_audiodec_set(UINT32 id, VENDOR_AUDIODEC_ITEM item, VOID *p_param);
HD_RESULT vendor_audiodec_get(UINT32 id, VENDOR_AUDIODEC_ITEM item, VOID *p_param);
#endif

