/**
	@brief Header file of definition of cpu softmax layer.

	@file vendor_ai_cpu_softmax.h

	@ingroup vendor_ai_cpu_softmax

	@note Nothing.

	Copyright Novatek Microelectronics Corp. 2019.  All rights reserved.
*/
#ifndef _VENDOR_AI_CPU_SOFTMAX_H_
#define _VENDOR_AI_CPU_SOFTMAX_H_

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
extern HD_RESULT vendor_ais_softmax(NN_SOFTMAX_PARM *p_parm, UINT32 net_id);
#else // USE_NEON
extern HD_RESULT vendor_ais_softmax(NN_SOFTMAX_PARM *p_parm);
#endif

#endif  /* _VENDOR_AI_CPU_SOFTMAX_H_ */
