/**
	@brief Header file of Custom NN Neural Network functions operating using DSP.

	@file custnn_dsp.h

	@ingroup custnn

	@note Nothing.

	Copyright Novatek Microelectronics Corp. 2019.  All rights reserved.
*/
#ifndef _CUSTNN_DSP_H_
#define _CUSTNN_DSP_H_

/********************************************************************
	INCLUDE FILES
********************************************************************/
#include "hd_type.h"
#include "vendor_ai_net/nn_verinfo.h"
#include "vendor_ai_net/nn_net.h"
#include "vendor_ai_net/nn_parm.h"

/********************************************************************
	EXTERN VARIABLES & FUNCTION PROTOTYPES DECLARATIONS
********************************************************************/
#ifdef __cplusplus
extern "C" {
#endif

/* Pooling layer */
extern HD_RESULT vendor_ai_dsp_cust_pool(NN_POOL_PARM *p_parm, UINT32 in_pa, UINT32 out_pa);

#ifdef __cplusplus
}
#endif

#endif  /* _CUSTNN_DSP_H_ */
