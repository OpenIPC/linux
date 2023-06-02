/**
	@brief Source file of cpu softmax.

	@file vendor_ai_cpu_softmax.c

	@ingroup vendor_ai_cpu_softmax

	@note Nothing.

	Copyright Novatek Microelectronics Corp. 2019.  All rights reserved.
*/

/*-----------------------------------------------------------------------------*/
/* Include Files                                                               */
/*-----------------------------------------------------------------------------*/
#include "hd_type.h"
#include "vendor_ai_util.h"

#include "vendor_ai_cpu_softmax.h"
#include "vendor_ai_cpu/vendor_ai_cpu_builtin.h"
#include "nvtnn/nvtnn_lib.h"
#include <string.h>

/*-----------------------------------------------------------------------------*/
/* Local Constant Definitions                                                  */
/*-----------------------------------------------------------------------------*/
#define POST_USE_CPU_STRUCT 0

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
/* Interface Functions                                                         */
/*-----------------------------------------------------------------------------*/

//static ER vendor_ais_softmax(NN_SOFTMAX_PARM *p_parm);
#if USE_NEON
HD_RESULT vendor_ais_softmax(NN_SOFTMAX_PARM *p_parm, UINT32 net_id)
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

	nvtnn_softmax_process(p_parm);

#if NN_USE_DSP
	p_parm->in_addr        = in_va;
	p_parm->out_addr       = out_va;
	p_parm->in_trans_addr  = in_trans_va;
	p_parm->out_trans_addr = out_trans_va;
#endif

	return HD_OK;
}
#else // USE_NEON
HD_RESULT vendor_ais_softmax(NN_SOFTMAX_PARM *p_parm)
{
	vendor_ais_softmax_process(p_parm);
	return HD_OK;
}
#endif // USE_NEON


/*-----------------------------------------------------------------------------*/
/* Unit Test Functions                                                         */
/*-----------------------------------------------------------------------------*/
#if !USE_NEON
HD_RESULT vendor_ais_softmax_test(UINT32 buf)
{
#define BATCH_NUM   1
#define CHANNELS    1000

	UINT8 *buftop = (UINT8 *)buf;
	NN_SOFTMAX_PARM parm = {0};
	INT32 size;
	INT16 *p_in, *p_chk;
	HD_RESULT ret = HD_OK;
#if AI_V4
	parm.shape.width = 1;
	parm.shape.height = 1;
	parm.shape.channel = CHANNELS;
	parm.shape.batch_num = BATCH_NUM;
	size = parm.shape.width * parm.shape.height * parm.shape.channel * parm.shape.batch_num;
#else
	parm.width = 1;
	parm.height = 1;
	parm.channel = CHANNELS;
	parm.batch_num = BATCH_NUM;
	size = parm.width * parm.height * parm.channel * parm.batch_num;
#endif

	p_in = (INT16 *)vendor_ais_getbuffrom(&buftop, size * sizeof(INT16));
	vendor_ais_readbin((UINT32)p_in, size * sizeof(INT16), "softmax/input.bin");

	p_chk = (INT16 *)vendor_ais_getbuffrom(&buftop, 3 * sizeof(INT16));
	p_chk[0] = 11;
	p_chk[1] = 12;
	p_chk[2] = 13;

	parm.in_addr 				= (UINT32)p_in;
	parm.out_addr      			= (UINT32)p_in;
#if AI_V4
	parm.in_fmt.frac_bits       = 10;
#else
	parm.in_bit_fmt.frac_bits   = 10;
#endif
	ret = vendor_ais_softmax_process(&parm);

	vendor_ais_writebin(parm.out_addr, size * sizeof(INT16), "softmax/output.bin");
	DBG_IND("chk: %d, %d, %d\r\n", p_chk[0], p_chk[1], p_chk[2]);

	return ret;
}
#endif //!USE_NEON


