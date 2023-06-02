/**
	@brief Header file of definition of vendor user-space net flow sample.

	@file vendor_ai_plugin.h

	@ingroup vendor_ai_plugin

	@note Nothing.

	Copyright Novatek Microelectronics Corp. 2018.  All rights reserved.
*/
#ifndef _VENDOR_AI_PLUGIN_H_
#define _VENDOR_AI_PLUGIN_H_

/********************************************************************
	INCLUDE FILES
********************************************************************/
#include "hd_type.h"

/********************************************************************
	MACRO CONSTANT DEFINITIONS
********************************************************************/

/********************************************************************
	TYPE DEFINITION
********************************************************************/

typedef HD_RESULT (*VENDOR_AI_ENG_CB)(UINT32 proc_id, UINT32 job_id);

/**
	Command of extend engine
*/
#define VENDOR_AI_PLUGIN_BUFADDR	0
#define VENDOR_AI_PLUGIN_BUFTYPE	1
#define VENDOR_AI_PLUGIN_BUFSIZE	2
#define VENDOR_AI_PLUGIN_RESULT		3

/**
	Callback function for layer processing
*/
typedef HD_RESULT (*VENDOR_AI_NET_PROC_CB)(UINT32 proc_id, UINT32 layer_id, UINT32 mode, UINT32 layer_param, UINT32 parm_addr);
typedef HD_RESULT (*VENDOR_AI_NET_SET_CB)(UINT32 proc_id, UINT32 layer_id, UINT32 mode, UINT32 layer_param, UINT32 parm_addr, UINT32 cmd, UINT32 buf_addr, UINT32 buf_size);
typedef HD_RESULT (*VENDOR_AI_NET_GET_CB)(UINT32 proc_id, UINT32 layer_id, UINT32 mode, UINT32 layer_param, UINT32 parm_addr, UINT32 cmd, UINT32* buf_addr, UINT32* buf_size);

/**
	Parameters of extend engine
*/
typedef struct _VENDOR_AI_ENGINE_PLUGIN {
	UINT32 sign;          ///< signature = MAKEFOURCC('A','E','N','G')
	UINT32 eng;           ///< engine id: 0=dla, 1=cpu, 2=dsp
	UINT32 ch;            ///< channel id: 0~4
	VENDOR_AI_NET_PROC_CB proc_cb;  ///< callback function for layer processing
	VENDOR_AI_NET_GET_CB get_cb;  ///< callback function for layer get buf
	VENDOR_AI_NET_SET_CB set_cb;  ///< callback function for layer set buf
} VENDOR_AI_ENGINE_PLUGIN;




/********************************************************************
	EXTERN VARIABLES & FUNCTION PROTOTYPES DECLARATIONS
********************************************************************/


#endif  /* _VENDOR_AI_PLUGIN_H_ */
