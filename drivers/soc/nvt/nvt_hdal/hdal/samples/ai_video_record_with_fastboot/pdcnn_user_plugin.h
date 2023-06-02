/**
	@brief Header file of user plugin.

	@file pdcnn_user_plugin.h

	@ingroup mhdal

	@note Nothing.

	Copyright Novatek Microelectronics Corp. 2020.  All rights reserved.
*/
#ifndef _PDCNN_USER_PLUGIN_H_
#define _PDCNN_USER_PLUGIN_H_

/********************************************************************
	INCLUDE FILES
********************************************************************/
#include "kwrap/type.h"
#include "vendor_ai.h"
#include "pdcnn_api.h"

/********************************************************************
	TYPE DEFINITION
********************************************************************/

#define PDCNN_IN_SIZE_W	960 //input dim of net
#define PDCNN_IN_SIZE_H	540  //input dim of net

/********************************************************************
	EXTERN VARIABLES & FUNCTION PROTOTYPES DECLARATIONS
********************************************************************/

extern void* get_pdcnn_postproc_plugin(void);

#endif  /* _PDCNN_USER_PLUGIN_H_ */
