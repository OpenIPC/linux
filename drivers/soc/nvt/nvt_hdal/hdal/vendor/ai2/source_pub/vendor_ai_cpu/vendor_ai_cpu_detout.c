/**
	@brief Source file of cpu detout.

	@file vendor_ai_cpu_detout.c

	@ingroup vendor_ai_cpu_detout

	@note Nothing.

	Copyright Novatek Microelectronics Corp. 2019.  All rights reserved.
*/

/*-----------------------------------------------------------------------------*/
/* Include Files                                                               */
/*-----------------------------------------------------------------------------*/
#include "hd_type.h"
#include "vendor_ai_util.h"
	
#include <string.h>
#include "vendor_ai_cpu_detout.h"
#include "nvtnn/nvtnn_lib.h"
#include "vendor_ai_cpu/vendor_ai_cpu_builtin.h"

/*-----------------------------------------------------------------------------*/
/* Local Constant Definitions                                                  */
/*-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------*/
/* Local Types Declarations                                                    */
/*-----------------------------------------------------------------------------*/
typedef struct _VENDOR_AI_DETOUT_PRIV {
	UINT32 rslt_info_addr;
	UINT32 rslt_mem_addr;
	UINT32 rslt_size;                   ///< include rslt_info + rslt_mem
	UINT32 num_classes;                 ///< number of classes
} VENDOR_AI_DETOUT_PRIV;

/*-----------------------------------------------------------------------------*/
/* Extern Function Prototype                                                   */
/*-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------*/
/* Local Global Variables                                                      */
/*-----------------------------------------------------------------------------*/
VENDOR_AI_DETOUT_PRIV g_detout_priv[MAX_PROC_CNT] = {0};

/*-----------------------------------------------------------------------------*/
/* Local Functions                                                          */
/*-----------------------------------------------------------------------------*/
#if USE_NEON
static UINT32 _vendor_ai_cpu_detout_get_rslt_info_size(UINT32 proc_id, UINT32 p_num_classes)
{
	UINT32 rslt_info_size = 0;

	if (proc_id >= MAX_PROC_CNT) {
		DBG_ERR("proc_id(%lu) >= max limit(%d)\r\n", proc_id, MAX_PROC_CNT);
		return HD_ERR_LIMIT;
	}

#if 1 
	g_detout_priv[proc_id].num_classes = p_num_classes;
#endif

	if (g_detout_priv[proc_id].num_classes == 0) {
		DBG_ERR("num_classes = 0?\n");
		return HD_ERR_NOT_ALLOW;
	}

	rslt_info_size = sizeof(VENDOR_AI_DETOUT_RESULT_INFO) + NN_DETOUT_TOP_N * g_detout_priv[proc_id].num_classes * sizeof(VENDOR_AI_DETOUT_RESULT);

	return rslt_info_size;
}

static UINT32 _vendor_ai_cpu_detout_get_rslt_mem_size(VOID)
{
	return sizeof(VENDOR_AIS_OUTPUT_CLASS) * NN_DETOUT_TOP_N;
}

static HD_RESULT _vendor_ais_set_detout_results(UINT32 proc_id, NN_DETOUT_PARM *p_parm)
{
	UINT32 rslt_info_addr = g_detout_priv[proc_id].rslt_info_addr;
	UINT32 rslt_info_size = 0;
	UINT32 rslt_num = 0;
	UINT32 req_size = 0;

	if (p_parm == NULL) {
		DBG_ERR("p_parm is NULL\r\n");
		return HD_ERR_NULL_PTR;
	}

	{
		VENDOR_AI_DETOUT_RESULT_INFO *p_rslt = (VENDOR_AI_DETOUT_RESULT_INFO *)rslt_info_addr;
		UINT32 i = 0, c = 0;
		UINT32 idx = 0;
		FLOAT *p_out = (FLOAT *)p_parm->out_addr;
		if (p_out == NULL) {
			DBG_ERR("p_out is NULL\r\n");
			return HD_ERR_NULL_PTR;
		}
		if (p_rslt == NULL) {
			DBG_ERR("p_rslt is NULL, please set detout buffer first !!\r\n");
			return HD_ERR_NULL_PTR;
		}

		rslt_info_size = _vendor_ai_cpu_detout_get_rslt_info_size(proc_id, p_parm->num_classes);
		rslt_num = NN_DETOUT_TOP_N * p_parm->num_classes;

		p_rslt->p_result = (VENDOR_AI_DETOUT_RESULT *)(rslt_info_addr + sizeof(VENDOR_AI_DETOUT_RESULT_INFO));
		req_size = sizeof(VENDOR_AI_DETOUT_RESULT_INFO) + rslt_num * sizeof(VENDOR_AI_DETOUT_RESULT);

		if (rslt_info_size < req_size) {
			DBG_ERR("results mem size(%d) is not enough, require: %d\r\n", (int)rslt_info_size, (int)req_size);
			return HD_ERR_BAD_DATA;
		}

		for (c = 0; c < p_parm->num_classes; c++) {
			if (c == p_parm->bg_lbl_id) {
				continue;
			}
			for (i = 0; i < NN_DETOUT_TOP_N; i++) {
				if (p_out[0] >= 0) {
					p_rslt->p_result[idx].score[0] = p_out[4];
					p_rslt->p_result[idx].no[0] = c;
					p_rslt->p_result[idx].x = p_out[0];
					p_rslt->p_result[idx].y = p_out[1];
					p_rslt->p_result[idx].w = p_out[2] - p_out[0];
					p_rslt->p_result[idx].h = p_out[3] - p_out[1];
					idx++;
				}
				p_out += 5;
			}
		}
		p_rslt->result_num = idx;
	}

	return HD_OK;
}
#endif // end of USE_NEON

/*-----------------------------------------------------------------------------*/
/* Internal Functions                                                          */
/*-----------------------------------------------------------------------------*/
UINT32 vendor_ai_cpu_detout_calcbuffersize(UINT32 proc_id, UINT32 p_num_classes)
{
#if USE_NEON
	UINT32 rslt_info_size = 0;
	UINT32 rslt_mem_size  = 0;

	if (proc_id >= MAX_PROC_CNT) {
		DBG_ERR("proc_id(%lu) >= max limit(%d)\r\n", proc_id, MAX_PROC_CNT);
		return HD_ERR_LIMIT;
	}

	rslt_info_size = _vendor_ai_cpu_detout_get_rslt_info_size(proc_id, p_num_classes);
	rslt_mem_size = _vendor_ai_cpu_detout_get_rslt_mem_size();

	return rslt_info_size + rslt_mem_size;
#else
	DBG_ERR("Not supported!!\n");
	return 0;
#endif // end of USE_NEON
}

HD_RESULT vendor_ai_cpu_detout_setbuffer(UINT32 proc_id, UINT32 p_out_addr, UINT32 out_size, UINT32 p_num_classes)
{
#if USE_NEON
	if (proc_id >= MAX_PROC_CNT) {
		printf("cpu_detout: err, proc_id(%lu) >= max limit(%d)r\n", proc_id, MAX_PROC_CNT);
		return HD_ERR_LIMIT;
	}

	if (p_out_addr == 0) {
		printf("cpu_detout: err, p_out_addr = 0?\n");
		return HD_ERR_NULL_PTR;
	}

	if (out_size == 0) {
		printf("cpu_detout: err, out_size = 0?\n");
		return HD_ERR_INV;
	}

	memset((void*)p_out_addr, 0, out_size);

	g_detout_priv[proc_id].rslt_info_addr = p_out_addr;
	g_detout_priv[proc_id].rslt_mem_addr  = p_out_addr + _vendor_ai_cpu_detout_get_rslt_info_size(proc_id, p_num_classes);
	g_detout_priv[proc_id].rslt_size      = out_size;
	
	return HD_OK;
#else
	printf("cpu_detout: err, %s is not supported!!\n", __func__);
	return HD_ERR_NOT_SUPPORT;
#endif // end of USE_NEON
}

VENDOR_AI_DETOUT_RESULT_INFO *vendor_ai_cpu_detout_getresult(UINT32 proc_id)
{
#if USE_NEON
	if (proc_id >= MAX_PROC_CNT) {
		printf("cpu_detout: err, proc_id(%lu) >= max limit(%d)r\n", proc_id, MAX_PROC_CNT);
		return NULL;
	}

	if (g_detout_priv[proc_id].rslt_info_addr == 0 || g_detout_priv[proc_id].rslt_size == 0) {
		return NULL;
	}
	
	return (VENDOR_AI_DETOUT_RESULT_INFO *)g_detout_priv[proc_id].rslt_info_addr;
#else
	printf("cpu_detout: err, %s is not supported!!\n", __func__);
	return 0;
#endif // end of USE_NEON
}

HD_RESULT vendor_ais_detout(NN_DETOUT_PARM *p_parm, UINT32 proc_id)
{
#if USE_NEON
#if NN_USE_DSP
	UINT32 in_va[NN_DETOUT_IN_NUM], out_va, in_trans_va, out_trans_va;
	UINT32 i;
#endif

#if !AI_V4
	p_parm->img_width = 1;
	p_parm->img_height = 1;
#endif

#if NN_USE_DSP
	for (i = 0; i < NN_DETOUT_IN_NUM; i++) {
		in_va[i] = p_parm->in_addr[i];
	}
	out_va       = p_parm->out_addr;
	in_trans_va  = p_parm->in_trans_addr;
	out_trans_va = p_parm->out_trans_addr;

	for (i = 0; i < NN_DETOUT_IN_NUM; i++) {
		p_parm->in_addr[i] = vendor_ais_user_buff_va2pa(p_parm->in_addr[i]    , proc_id);
	}
	p_parm->out_addr       = vendor_ais_user_buff_va2pa(p_parm->out_addr      , proc_id);
	p_parm->in_trans_addr  = vendor_ais_user_buff_va2pa(p_parm->in_trans_addr , proc_id);
	p_parm->out_trans_addr = vendor_ais_user_buff_va2pa(p_parm->out_trans_addr, proc_id);
#endif

	nvtnn_detout_process(p_parm);

#if NN_USE_DSP
	for (i = 0; i < NN_DETOUT_IN_NUM; i++) {
		p_parm->in_addr[i] = in_va[i];
	}
	p_parm->out_addr       = out_va;
	p_parm->in_trans_addr  = in_trans_va;
	p_parm->out_trans_addr = out_trans_va;
#endif

#if 0
	{ // debug
		FLOAT *p_out = (FLOAT *)p_parm->out_addr;
		UINT32 c, i;
		DBG_DUMP("\r\nDetection Results (Top 5):\r\n");
		for (c = 0; c < p_parm->num_classes; c++) {
			if (c == p_parm->bg_lbl_id) {
				continue;
			}

			DBG_DUMP("[Class %ld]\r\n", c);
			for (i = 0; i < NN_DETOUT_TOP_N; i++) {
				if (p_out[0] >= 0) {
					DBG_DUMP("%ld. xmin=%f,ymin=%f,xmax=%f,ymax=%f score=%f\r\n", i + 1, p_out[0], p_out[1], p_out[2], p_out[3], p_out[4]);
				}
				p_out += 5;
			}
		}
	}
#endif

	_vendor_ais_set_detout_results(proc_id, p_parm);

	return HD_OK;
#else
	printf("cpu_detout: err, %s is not supported!!\n", __func__);
	return HD_ERR_NOT_SUPPORT;
#endif // end of USE_NEON
}

HD_RESULT vendor_ai_cpu_detout_get_labelnum(UINT32 proc_id, UINT32 *p_label_num)
{
#if USE_NEON
	if (p_label_num == NULL) {
		printf("cpu_detout: err, p_label_num is null\n");
		return HD_ERR_NULL_PTR;
	}
	if (proc_id >= MAX_PROC_CNT) {
		printf("cpu_detout: err, proc_id(%lu) >= max limit(%d)r\n", proc_id, MAX_PROC_CNT);
		return HD_ERR_LIMIT;
	}

	*p_label_num = g_detout_priv[proc_id].num_classes;

	return HD_OK;
#else
	printf("cpu_detout: err, %s is not supported!!\n", __func__);
	return HD_ERR_NOT_SUPPORT;
#endif // end of USE_NEON
}

