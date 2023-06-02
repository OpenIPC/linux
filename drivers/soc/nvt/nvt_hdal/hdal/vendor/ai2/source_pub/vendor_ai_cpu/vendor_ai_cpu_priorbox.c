/**
	@brief Source file of cpu pirorbox.

	@file vendor_ai_cpu_pirorbox.c

	@ingroup vendor_ai_cpu_pirorbox

	@note Nothing.

	Copyright Novatek Microelectronics Corp. 2019.  All rights reserved.
*/

/*-----------------------------------------------------------------------------*/
/* Include Files                                                               */
/*-----------------------------------------------------------------------------*/
#include "hd_type.h"
#include "vendor_ai_util.h"

#include <string.h>
#include "vendor_ai_cpu_priorbox.h"
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
HD_RESULT vendor_ais_priorbox(NN_PRIORBOX_PARM *p_parm, UINT32 net_id)
{
#if NN_USE_DSP
	UINT32 in_va, out_va, in_trans_va, out_trans_va;

	in_va        = p_parm->in_addr;
	out_va       = p_parm->out_addr;
	in_trans_va  = p_parm->in_trans_addr;
	out_trans_va = p_parm->out_trans_addr;

	p_parm->in_addr        = vendor_ais_user_buff_va2pa(p_parm->in_addr       , net_id);
	p_parm->out_addr       = vendor_ais_user_buff_va2pa(p_parm->out_addr      , net_id);
	p_parm->in_trans_addr  = vendor_ais_user_buff_va2pa(p_parm->in_trans_addr , net_id);
	p_parm->out_trans_addr = vendor_ais_user_buff_va2pa(p_parm->out_trans_addr, net_id);
#endif

	nvtnn_priorbox_process(p_parm);

#if NN_USE_DSP
	p_parm->in_addr        = in_va;
	p_parm->out_addr       = out_va;
	p_parm->in_trans_addr  = in_trans_va;
	p_parm->out_trans_addr = out_trans_va;
#endif

	return HD_OK;
}
#endif

/*-----------------------------------------------------------------------------*/
/* Interface Functions                                                         */
/*-----------------------------------------------------------------------------*/


