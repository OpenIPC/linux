/**
	@brief Source file of NvtNN layers.

	@file net_nvtnn_layer.c

	@ingroup nvtnnnn

	@note Nothing.

	Copyright Novatek Microelectronics Corp. 2019.  All rights reserved.
*/

/*-----------------------------------------------------------------------------*/
/* Include Files                                                               */
/*-----------------------------------------------------------------------------*/
#include "hd_type.h"
#include "vendor_ai_util.h"

#include "vendor_ai_dsp_nvtnn.h"

/*-----------------------------------------------------------------------------*/
/* Macro Constant Definitions                                                  */
/*-----------------------------------------------------------------------------*/
#define CUST_PRINT_PARM     TRUE

/*-----------------------------------------------------------------------------*/
/* Interface Functions                                                         */
/*-----------------------------------------------------------------------------*/
HD_RESULT vendor_ai_dsp_nvtnn(NN_GEN_ENG_TYPE eng, UINT32 parm_addr, UINT32 net_id)
{
	/*
		custom parm format:
		1. custom_layer_type_id(UINT32)
		2. weight_num(UINT32), weight_size_1(UINT32), weight_size_2(UINT32), ...
		   (weights at p_head->model.va)
		3. str_parm_len(UINT32), bin_parm_len(UINT32), str_parm, bin_parm
	*/
	NN_CUSTOM_PARM *p_head  = (NN_CUSTOM_PARM *)(parm_addr);
	UINT32 *p_layer_type_id = (UINT32 *)(p_head + 1);
	UINT32  layer_type_id   = *p_layer_type_id;
#if CUST_PRINT_PARM && !CUST_SUPPORT_MULTI_IO
	UINT32 *p_weight_num    = (UINT32 *)(p_layer_type_id + 1);
	UINT32  weight_num      = *p_weight_num;
	UINT32 *p_str_parm_len  = (UINT32 *)(p_weight_num + 1 + weight_num);
	UINT32  str_parm_len    = *p_str_parm_len;
	UINT32 *p_bin_parm_len  = (UINT32 *)(p_str_parm_len + 1);
	CHAR   *p_str_parm      = (CHAR   *)(p_bin_parm_len + 1);
#if NN_USE_DSP
	VOID   *p_bin_parm      = (VOID   *)(p_str_parm + str_parm_len);
#endif

	DBGH(p_head->input.pa);
	DBGH(p_head->input.va);
	DBGH(p_head->input.size);
	DBGH(*(UINT32 *)&p_head->input.fmt);
	DBGH(p_head->output.pa);
	DBGH(p_head->output.va);
	DBGH(p_head->output.size);
	DBGH(*(UINT32 *)&p_head->output.fmt);
	DBGH(p_head->model.pa);
	DBGH(p_head->model.va);
	DBGH(p_head->model.size);
	DBGH(*(UINT32 *)&p_head->model.fmt);
	DBGD(p_head->parm_size);

	DBGD(layer_type_id);
	DBGD(weight_num);
	DBGD(str_parm_len);
	DBGD(*p_bin_parm_len);
	DBG_DUMP("str_parm=%s\r\n", p_str_parm);
#endif

	switch (layer_type_id & NN_LAYER_TYPE_ID_MASK) {
#if NN_USE_DSP
	case NVTNN_PRELU: {
			NVTNN_PRELU_PRMS *p_parm = p_bin_parm;
			p_parm->in_addr    = p_head->input.pa;
			p_parm->out_addr   = p_head->output.pa;
			p_parm->slope_addr = p_head->model.pa;
			nvtnn_prelu_process(p_parm, NVTNN_SENDTO_DSP1);
		}
		break;
#endif
	default:
		DBG_DUMP("DSP layer type id %d: not support\r\n", (INT)layer_type_id);
		return HD_ERR_NOT_SUPPORT;
	}

	return HD_OK;
}
