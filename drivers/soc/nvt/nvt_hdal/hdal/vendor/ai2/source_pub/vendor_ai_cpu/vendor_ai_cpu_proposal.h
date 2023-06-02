/**
	@brief Header file of definition of cpu proposal layer.

	@file vendor_ai_cput_proposal.h

	@ingroup vendor_ai_cpu_proposal

	@note Nothing.

	Copyright Novatek Microelectronics Corp. 2019.  All rights reserved.
*/
#ifndef _VENDOR_AI_CPU_PROPOSAL_H_
#define _VENDOR_AI_CPU_PROPOSAL_H_

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
extern HD_RESULT vendor_ais_proposal(UINT32 parm_addr);

#endif  /* _VENDOR_AI_CPU_PROPOSAL_H_ */
