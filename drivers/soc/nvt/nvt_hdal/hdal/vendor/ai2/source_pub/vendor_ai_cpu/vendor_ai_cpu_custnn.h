/**
	@brief Header file of definition of network custom layer.

	@file custnn.h

	@ingroup custnn

	@note Nothing.

	Copyright Novatek Microelectronics Corp. 2019.  All rights reserved.
*/
#ifndef _CUSTOM_NN_H_
#define _CUSTOM_NN_H_

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
extern HD_RESULT vendor_ai_cpu_cust_init  (UINT32 parm_addr, UINT32 net_id);
extern HD_RESULT vendor_ai_cpu_cust_uninit(UINT32 parm_addr, UINT32 net_id);
extern HD_RESULT vendor_ai_cpu_cust(UINT32 parm_addr, UINT32 net_id);
extern HD_RESULT vendor_ai_cpu_cust_set_tmp_buf(NN_DATA tmp_buf, UINT32 parm_addr);

extern UINT32 vendor_ai_cpu_cust_get_layer_id(UINT32 parm_addr);

#if USE_NEON
extern HD_RESULT vendor_ai_cpu_nvtnn(UINT32 parm_addr, UINT32 net_id);
#endif

#endif  /* _CUSTOM_NN_H_ */
