/**
	@brief Header file of definition of cpu priorbox layer.

	@file vendor_ai_cpu_priorbox.h

	@ingroup vendor_ai_cpu_priorbox

	@note Nothing.

	Copyright Novatek Microelectronics Corp. 2019.  All rights reserved.
*/
#ifndef _VENDOR_AI_CPU_PRIORBOX_H_
#define _VENDOR_AI_CPU_PRIORBOX_H_

/********************************************************************
 MACRO CONSTANT DEFINITIONS
********************************************************************/

/********************************************************************
	INCLUDE FILES
********************************************************************/
#include "hd_type.h"
#include "vendor_ai_net/nn_verinfo.h"
#include "vendor_ai_net/nn_net.h"
#include "vendor_ai_net/nn_parm.h"

/********************************************************************
	TYPE DEFINITION
********************************************************************/

/********************************************************************
	EXTERN VARIABLES & FUNCTION PROTOTYPES DECLARATIONS
********************************************************************/
#if USE_NEON
extern HD_RESULT vendor_ais_priorbox(NN_PRIORBOX_PARM *p_parm, UINT32 net_id);
#endif

#endif  /* _VENDOR_AI_CPU_PRIORBOX_H_ */
