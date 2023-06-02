/**
	@brief Source file of dsp1 processing flow.

	@file vendor_ai_dsp.c

	@ingroup vendor_ai_dsp

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
#include "vendor_ai_dsp/vendor_ai_dsp.h"  // NN_BNSCALE, NN_POOL, NN_FC_POST, NN_SOFTMAX
#include "vendor_ai_dsp_custnn.h"         // NN_CUSTOMER


static HD_RESULT _vendor_ai_dsp1_proc(UINT32 proc_id, UINT32 layer_id, UINT32 mode, UINT32 layer_param, UINT32 parm_addr)
{
	HD_RESULT rv = HD_OK;
	//printf("[_vendor_ai_dsp1_proc] callback !! mode = %lu, eng = %lu, parm_addr = 0x%08x, proc_id = %lu\n", mode, eng, (UINT)parm_addr, proc_id);
	
	switch ((NN_MODE)mode) {
	//--- [ source_pub ] ---
	case NN_CUSTOMER:
#if CNN_25_MATLAB
		rv = vendor_ai_dsp_cust(parm_addr, proc_id);
#else
		{
#if USE_NEON
		NN_GEN_MODE_CTRL *p_mctrl = (NN_GEN_MODE_CTRL *)layer_param;
		//NN_DATA *p_sai = (NN_DATA*)(p_mctrl->iomem.imem_addr);
		NN_DATA *p_sao = (NN_DATA*)(p_mctrl->iomem.omem_addr);
		NN_CUSTOM_PARM *p_head = (NN_CUSTOM_PARM *)parm_addr;
		UINT32 layer_type_id = *(UINT32 *)(p_head + 1);
		if ((layer_type_id & NN_LAYER_LIB_NVTNN) == NN_LAYER_LIB_NVTNN) {
			vendor_ai_dsp_nvtnn(parm_addr, proc_id);
		} else {
			vendor_ai_dsp_cust_set_tmp_buf(p_sao[2], parm_addr);
			vendor_ai_dsp_cust(parm_addr, proc_id);
		}
#else
		NN_GEN_MODE_CTRL *p_mctrl = (NN_GEN_MODE_CTRL *)layer_param;
		//NN_DATA *p_sai = (NN_DATA*)(p_mctrl->iomem.imem_addr);
		NN_DATA *p_sao = (NN_DATA*)(p_mctrl->iomem.omem_addr);
		vendor_ai_dsp_cust_set_tmp_buf(p_sao[2], parm_addr);
		vendor_ai_dsp_cust(parm_addr, proc_id);
#endif
		}
#endif
		break;

	case NN_PROPOSAL:
	case NN_POSTPROC:
	case NN_SOFTMAX:
	case NN_FC_POST:
	case NN_POOL:
	case NN_BNSCALE:
	case NN_PREPROC:
#if (USE_NEON || (!CNN_25_MATLAB))
	case NN_PRELU:
	case NN_PRIORBOX:
	case NN_DETOUT:
#endif
	case NN_CONV:
	case NN_DECONV:
	case NN_SVM:
	case NN_FC:
	case NN_ROIPOOLING:
	case NN_ELTWISE:
	case NN_REORGANIZATION:
	case NN_RESHAPE:
	default:
		printf("_vendor_ai_dsp1_proc: err, Mode %lu: not support\n", mode);
		return HD_ERR_NOT_SUPPORT;
	}
	return rv;
}


static VENDOR_AI_ENGINE_PLUGIN _vendor_ai_dsp1 = {
	.sign = MAKEFOURCC('A','E','N','G'),
	.eng = 2,  //dsp
	.ch = 0,
	.proc_cb = _vendor_ai_dsp1_proc
};

void* vendor_ai_dsp1_get_engine(void)
{
	return (void*)&_vendor_ai_dsp1;
}

