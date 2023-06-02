/**
	@brief Header file of definition of network custom layer.

	@file custnn.h

	@ingroup custnn

	@note Nothing.

	Copyright Novatek Microelectronics Corp. 2019.  All rights reserved.
*/
#ifndef _NVT_NN_H_
#define _NVT_NN_H_

/********************************************************************
	INCLUDE FILES
********************************************************************/
#include "hd_type.h"
#include "vendor_ai_net/nn_verinfo.h"
#include "vendor_ai_net/nn_net.h"
#include "vendor_ai_net/nn_parm.h"

/********************************************************************
	MACRO CONSTANT DEFINITIONS
********************************************************************/

/********************************************************************
	EXTERN VARIABLES & FUNCTION PROTOTYPES DECLARATIONS
********************************************************************/
#if USE_NEON
extern HD_RESULT vendor_ai_cpu_nvtnn(NN_GEN_ENG_TYPE eng, UINT32 parm_addr, UINT32 net_id);
#endif

#endif  /* _NVT_NN_H_ */
