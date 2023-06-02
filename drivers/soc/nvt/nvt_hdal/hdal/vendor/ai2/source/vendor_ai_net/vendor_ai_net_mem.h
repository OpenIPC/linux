/**
	@brief Header file of vendor ai net memory.

	@file vendor_ai_net_mem.h

	@ingroup vendor_ai_net

	@note Nothing.

	Copyright Novatek Microelectronics Corp. 2018.  All rights reserved.
*/
#ifndef _VENDOR_AI_NET_MEM_H_
#define _VENDOR_AI_NET_MEM_H_

/********************************************************************
	INCLUDE FILES
********************************************************************/
#include "hd_type.h"
#include "kflow_ai_net/kflow_ai_net.h"
#include "vendor_ai.h"

/********************************************************************
	TYPE DEFINITION
********************************************************************/

typedef enum {
	VENDOR_AI_NET_BUF_OPT_TEST_ZERO = 3000,
} VENDOR_AI_NET_BUF_OPT_TEST;

/********************************************************************
	EXTERN VARIABLES & FUNCTION PROTOTYPES DECLARATIONS
********************************************************************/
extern HD_RESULT _vendor_ai_net_mem_alloc_mem(UINT32 proc_id, VENDOR_AIS_FLOW_MAP_MEM_PARM *p_mem_manager, UINT32 buf_method, UINT32 job_method);

#endif  /* _VENDOR_AI_NET_MEM_H_ */
