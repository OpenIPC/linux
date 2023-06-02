/**
	@brief Header file of definition of cpu prelu layer.

	@file vendor_ai_cpu_prelu.h

	@ingroup vendor_ai_cpu_prelu

	@note Nothing.

	Copyright Novatek Microelectronics Corp. 2019.  All rights reserved.
*/
#ifndef _VENDOR_AI_CPU_PRELU_H_
#define _VENDOR_AI_CPU_PRELU_H_

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
extern HD_RESULT vendor_ais_prelu(NN_PRELU_PARM *p_parm, UINT32 net_id);
#endif

#endif  /* _VENDOR_AI_CPU_PRELU_H_ */
