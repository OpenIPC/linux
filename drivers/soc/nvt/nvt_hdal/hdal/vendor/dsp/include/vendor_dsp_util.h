/**
	@brief Header file of utility functions of vendor dsp module.

	@file vendor_dsp_util.h

	@ingroup vendor_dsp

	@note Nothing.

	Copyright Novatek Microelectronics Corp. 2019.  All rights reserved.
*/

#ifndef _VENDOR_DSP_UTIL_H_
#define _VENDOR_DSP_UTIL_H_

/********************************************************************
	INCLUDE FILES
********************************************************************/
#include "hd_type.h"
#include "vendor_dsp.h"

/********************************************************************
	MACRO CONSTANT DEFINITIONS
********************************************************************/
#define DSP1_APP_IPC_TOKEN      "DspApp"
#define DSP2_APP_IPC_TOKEN      "DspApp2"

/********************************************************************
	MACRO FUNCTION DEFINITIONS
********************************************************************/

/********************************************************************
	TYPE DEFINITION
********************************************************************/

/********************************************************************
	EXTERN VARIABLES & FUNCTION PROTOTYPES DECLARATIONS
********************************************************************/
extern HD_RESULT vendor_dsp_init_share_mem(UINT32 size, UINT32 *p_pa, UINT32 *p_va);
extern HD_RESULT vendor_dsp_uninit_share_mem(VOID);
extern HD_RESULT vendor_dsp_load(DSP_CORE_ID dsp_core_id);

#endif  /* _VENDOR_DSP_UTIL_H_ */
