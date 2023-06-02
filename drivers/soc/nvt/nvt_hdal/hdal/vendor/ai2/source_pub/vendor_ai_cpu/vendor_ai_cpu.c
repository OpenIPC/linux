/**
	@brief Source file of cpu1 processing flow.

	@file vendor_ai_cpu.c

	@ingroup vendor_ai_cpu

	@note Nothing.

	Copyright Novatek Microelectronics Corp. 2019.  All rights reserved.
*/

/*-----------------------------------------------------------------------------*/
/* Include Files                                                               */
/*-----------------------------------------------------------------------------*/
#include "hd_type.h"
#include "vendor_ai_util.h"

#include "vendor_ai_net/nn_verinfo.h"
#include "vendor_ai_net/nn_net.h"
#include "vendor_ai_net/nn_parm.h"

#include "vendor_ai_plugin.h"
#include "vendor_ai_cpu/vendor_ai_cpu.h"  // NN_BNSCALE, NN_POOL, NN_FC_POST
#include "vendor_ai_cpu/vendor_ai_cpu_builtin.h" // NN_POSTPROC, NN_SOFTMAX
#include "vendor_ai_cpu_custnn.h"         // NN_CUSTOMER
#include "vendor_ai_cpu_softmax.h"        // NN_SOFTMAX
#include "vendor_ai_cpu_postproc.h"       // NN_POSTPROC
#include "vendor_ai_cpu_proposal.h"       // NN_PROPOSAL
#include "vendor_ai_cpu_detout.h"         // NN_DETOUT
#include "vendor_ai_cpu_priorbox.h"       // NN_PRIORBOX
#include "vendor_ai_cpu_prelu.h"          // NN_PRELU


static HD_RESULT _vendor_ai_cpu1_proc(UINT32 proc_id, UINT32 layer_id, UINT32 mode, UINT32 layer_param, UINT32 parm_addr)
{
	HD_RESULT rv = HD_OK;
	//printf("[_vendor_ai_cpu1_proc] callback !! mode = %lu, eng = %lu, parm_addr = 0x%08x, proc_id = %lu\n", mode, eng, (UINT)parm_addr, proc_id);

	switch ((NN_MODE)mode) {

	case NN_PROPOSAL:
		rv = vendor_ais_proposal(parm_addr);
		break;
		
	case NN_POSTPROC:
		rv = vendor_ais_postproc(proc_id, parm_addr);
		break;

	case NN_CUSTOMER:
		{
#if USE_NEON
			vendor_ai_cpu_cust(parm_addr, proc_id);
		
#else
#if !CUST_SUPPORT_MULTI_IO
		NN_GEN_MODE_CTRL *p_mctrl = (NN_GEN_MODE_CTRL *)layer_param;
		//NN_DATA *p_sai = (NN_DATA*)(p_mctrl->iomem.imem_addr);
		NN_DATA *p_sao = (NN_DATA*)(p_mctrl->iomem.omem_addr);
		vendor_ai_cpu_cust_set_tmp_buf(p_sao[2], parm_addr);
#endif
		vendor_ai_cpu_cust(parm_addr, proc_id);
#endif
		}
		break;

	case NN_SOFTMAX:
#if CNN_25_MATLAB
		rv = vendor_ais_softmax_process((NN_SOFTMAX_PARM *)parm_addr);
#else
#if USE_NEON
		rv = vendor_ais_softmax((NN_SOFTMAX_PARM *)parm_addr, proc_id);
#else
		rv = vendor_ais_softmax((NN_SOFTMAX_PARM *)parm_addr);
#endif
#endif
		break;
		
	case NN_FC_POST:
#if AI_V4
        rv = nvtnn_fc_post_process((NN_FC_POST_PARM *)parm_addr);
#else
		rv = vendor_ai_fc_post_process((NN_FC_POST_PARM *)parm_addr);
#endif
		break;
		
	case NN_POOL:
#if AI_V4
        rv = nvtnn_pool_process((NN_POOL_PARM *)parm_addr);
#else
		rv = vendor_ai_pool_process((NN_POOL_PARM *)parm_addr);
#endif
		break;

	case NN_BNSCALE:
		//rv = vendor_ai_bnscale_process((NN_BNSCALE_PARM *)parm_addr);
		//break;

	//--- [ not support ] ---
	case NN_PREPROC:
		/*
		p_pre_pram = (NN_PRE_PARM *)(p_mctrl[process_index].addr);
		pre_buff.src.va 		= p_io_mem[layer_index].SAI[0].address;
		if (process_index == 0) {
			pre_buff.src.pa		= in_img.phy_addr[0];
		} else {
			pre_buff.src.pa		= vendor_ais_user_buff_va2pa(pre_buff.src.va, net_id);
		}
		pre_buff.src.size 		= p_io_mem[layer_index].SAI[0].size;
		pre_buff.dst.va 		= p_io_mem[layer_index].SAO[0].address;
		pre_buff.dst.pa			= vendor_ais_user_buff_va2pa(pre_buff.dst.va, net_id);
		pre_buff.dst.size 		= p_io_mem[layer_index].SAO[0].size;
		pre_buff.interm.va 		= p_io_mem[layer_index].SAI[2].address;
		pre_buff.interm.pa		= vendor_ais_user_buff_va2pa(pre_buff.interm.va, net_id);
		pre_buff.interm.size 	= p_io_mem[layer_index].SAI[2].size;
		pre_buff.mean.va 		= p_io_mem[layer_index].SAI[1].address;
		pre_buff.mean.pa		= vendor_ais_user_model_va2pa(pre_buff.mean.va, net_id);
		pre_buff.mean.size 		= p_io_mem[layer_index].SAI[1].size;
		vendor_ais_preproc(p_pre_pram, pre_buff, net_id);
		break;
		*/
#if USE_NEON
	case NN_PRELU:
		vendor_ais_prelu((NN_PRELU_PARM *)parm_addr, proc_id);
		break;
	case NN_PRIORBOX:
		vendor_ais_priorbox((NN_PRIORBOX_PARM *)parm_addr, proc_id);
		break;
	case NN_DETOUT:
		vendor_ais_detout((NN_DETOUT_PARM *)parm_addr, proc_id);
		break;
#endif
    case NN_RESHAPE:
		nvtnn_permute_process((NN_PERMUTE_PARM *)parm_addr);
		break;
	case NN_REVERSE:
		nvtnn_reverse_process((NN_REVERSE_PARM *)parm_addr);
		break;
	case NN_LSTM:
		nvtnn_lstm_process((NN_LSTM_PARM *)parm_addr);
		break;
	case NN_CONV:
	case NN_DECONV:
	case NN_SVM:
	case NN_FC:
	case NN_ROIPOOLING:
	case NN_ELTWISE:
	case NN_REORGANIZATION:
	default:
		printf("_vendor_ai_cpu1_proc: err, Mode %lu: not support\n", mode);
		return HD_ERR_NOT_SUPPORT;
	}
	return rv;
}

static HD_RESULT _vendor_ai_cpu1_get(UINT32 proc_id, UINT32 layer_id, UINT32 mode, UINT32 layer_param, UINT32 parm_addr, UINT32 cmd, UINT32* buf_addr, UINT32* buf_size)
{
	HD_RESULT rv = HD_ERR_NOT_SUPPORT;
	//printf("[_vendor_ai_cpu1_proc] callback !! mode = %lu, eng = %lu, parm_addr = 0x%08x, proc_id = %lu\n", mode, eng, (UINT)parm_addr, proc_id);

	switch ((NN_MODE)mode) {

	case NN_POSTPROC:
		switch (cmd) {
		case VENDOR_AI_PLUGIN_BUFTYPE:
			//query buffer type
			buf_addr[0] = MAKEFOURCC('A','C','0','1');
			rv = HD_OK;
			break;
		case VENDOR_AI_PLUGIN_BUFSIZE:
			//query buffer size
			buf_size[0] = vendor_ai_cpu_postproc_calcbuffersize();
			rv = HD_OK;
			break;
		case VENDOR_AI_PLUGIN_RESULT:
			//get buffer result
			buf_addr[0] = (UINT32)(void*)vendor_ai_cpu_postproc_getresult(proc_id);
			buf_size[0] = sizeof(VENDOR_AI_POSTPROC_RESULT_INFO);
			rv = HD_OK;
		default:
			break;
		}
		break;
	case NN_DETOUT:
		switch (cmd) {
		case VENDOR_AI_PLUGIN_BUFTYPE:
			//query buffer type
			buf_addr[0] = MAKEFOURCC('A','D','0','1');
			rv = HD_OK;
			break;
		case VENDOR_AI_PLUGIN_BUFSIZE:
			//query buffer size
			buf_size[0] = vendor_ai_cpu_detout_calcbuffersize(proc_id, ((NN_DETOUT_PARM *)parm_addr)->num_classes);
			rv = HD_OK;
			break;
		case VENDOR_AI_PLUGIN_RESULT:
			//get buffer result
			buf_addr[0] = (UINT32)(void*)vendor_ai_cpu_detout_getresult(proc_id);
			buf_size[0] = sizeof(VENDOR_AI_DETOUT_RESULT_INFO);
			rv = HD_OK;
		default:
			break;
		}
		break;
	default:
		break;
	}
	return rv;
}

static HD_RESULT _vendor_ai_cpu1_set(UINT32 proc_id, UINT32 layer_id, UINT32 mode, UINT32 layer_param, UINT32 parm_addr, UINT32 cmd, UINT32 buf_addr, UINT32 buf_size)
{
	HD_RESULT rv = HD_ERR_NOT_SUPPORT;
	//printf("[_vendor_ai_cpu1_proc] callback !! mode = %lu, eng = %lu, parm_addr = 0x%08x, proc_id = %lu\n", mode, eng, (UINT)parm_addr, proc_id);

	switch ((NN_MODE)mode) {

	case NN_POSTPROC:
		switch (cmd) {
		case VENDOR_AI_PLUGIN_BUFADDR:
			//config buffer addr
			vendor_ai_cpu_postproc_setbuffer(proc_id, buf_addr, buf_size);
			rv = HD_OK;
		default:
			break;
		}
		break;
	case NN_DETOUT:
		switch (cmd) {
		case VENDOR_AI_PLUGIN_BUFADDR:
			//config buffer addr
			vendor_ai_cpu_detout_setbuffer(proc_id, buf_addr, buf_size, ((NN_DETOUT_PARM *)parm_addr)->num_classes);
			rv = HD_OK;
		default:
			break;
		}
		break;
	default:
		break;
	}
	return rv;
}

static VENDOR_AI_ENGINE_PLUGIN _vendor_ai_cpu1 = {
	.sign = MAKEFOURCC('A','E','N','G'),
	.eng = 1, //cpu
	.ch = 0,
	.proc_cb = _vendor_ai_cpu1_proc,
	.get_cb = _vendor_ai_cpu1_get,
	.set_cb = _vendor_ai_cpu1_set
};

void* vendor_ai_cpu1_get_engine(void)
{
	return (void*)&_vendor_ai_cpu1;
}

