/**
	@brief Source file of cpu prelu.

	@file vendor_ai_cpu_prelu.c

	@ingroup vendor_ai_cpu_prelu

	@note Nothing.

	Copyright Novatek Microelectronics Corp. 2019.  All rights reserved.
*/

/*-----------------------------------------------------------------------------*/
/* Include Files                                                               */
/*-----------------------------------------------------------------------------*/
#include "hd_type.h"
#include "vendor_ai_util.h"
	
#include <string.h>
#include "vendor_ai_cpu_prelu.h"
#include "nvtnn/nvtnn_lib.h"

/*-----------------------------------------------------------------------------*/
/* Local Constant Definitions                                                  */
/*-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------*/
/* Local Types Declarations                                                    */
/*-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------*/
/* Extern Function Prototype                                                   */
/*-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------*/
/* Local Global Variables                                                      */
/*-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------*/
/* Internal Functions                                                          */
/*-----------------------------------------------------------------------------*/
#if USE_NEON
HD_RESULT vendor_ais_prelu(NN_PRELU_PARM *p_parm, UINT32 net_id)
{
#if NN_USE_DSP
	UINT32 in_va, out_va, slope_va;

	in_va    = p_parm->in_addr;
	out_va   = p_parm->out_addr;
	slope_va = p_parm->slope_addr;

	p_parm->in_addr    = vendor_ais_user_buff_va2pa (p_parm->in_addr   , net_id);
	p_parm->out_addr   = vendor_ais_user_buff_va2pa (p_parm->out_addr  , net_id);
	p_parm->slope_addr = vendor_ais_user_model_va2pa(p_parm->slope_addr, net_id);
#endif

	nvtnn_prelu_process(p_parm);

#if NN_USE_DSP
	p_parm->in_addr    = in_va;
	p_parm->out_addr   = out_va;
	p_parm->slope_addr = slope_va;
#endif

	return HD_OK;
}
#endif

/*-----------------------------------------------------------------------------*/
/* Interface Functions                                                         */
/*-----------------------------------------------------------------------------*/

