/**
	@brief Source file of cpu postproc.

	@file vendor_ai_cpu_postproc.c

	@ingroup vendor_ai_cpu_postproc

	@note Nothing.

	Copyright Novatek Microelectronics Corp. 2019.  All rights reserved.
*/

/*-----------------------------------------------------------------------------*/
/* Include Files                                                               */
/*-----------------------------------------------------------------------------*/
#include "hd_type.h"
#include "vendor_ai_util.h"

#include "vendor_ai_cpu/vendor_ai_cpu_builtin.h"
#include "vendor_ai_cpu_postproc.h"
#include <string.h>

/*-----------------------------------------------------------------------------*/
/* Local Constant Definitions                                                  */
/*-----------------------------------------------------------------------------*/
#define POST_USE_CPU_STRUCT 0

/*-----------------------------------------------------------------------------*/
/* Local Types Declarations                                                    */
/*-----------------------------------------------------------------------------*/
typedef struct _VENDOR_AI_POSTPROC_PRIV {
	UINT32 rslt_info_addr;
	UINT32 rslt_mem_addr;
	UINT32 rslt_size; // include rslt_info + rslt_mem
	VENDOR_AIS_SHAPE shape;
} VENDOR_AI_POSTPROC_PRIV;

/*-----------------------------------------------------------------------------*/
/* Extern Function Prototype                                                   */
/*-----------------------------------------------------------------------------*/
extern HD_RESULT vendor_ai_cpu_accuracy_process(VENDOR_AIS_ACCURACY_PARM *p_parm);

/*-----------------------------------------------------------------------------*/
/* Local Global Variables                                                      */
/*-----------------------------------------------------------------------------*/
VENDOR_AI_POSTPROC_PRIV g_postproc_priv[MAX_PROC_CNT] = {0};

/*-----------------------------------------------------------------------------*/
/* Internal Functions                                                          */
/*-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------*/
/* Interface Functions                                                         */
/*-----------------------------------------------------------------------------*/

UINT32 _vendor_ai_cpu_postproc_get_rslt_info_size(VOID)
{
	return sizeof(VENDOR_AI_POSTPROC_RESULT_INFO) + sizeof(VENDOR_AI_POSTPROC_RESULT);
}

UINT32 _vendor_ai_cpu_postproc_get_rslt_mem_size(VOID)
{
	return sizeof(VENDOR_AIS_OUTPUT_CLASS) * NN_POSTPROC_TOP_N;
}

HD_RESULT _vendor_ais_set_results(UINT32 proc_id, NN_POSTPROC_PARM *p_parm)
{
	UINT32 rslt_info_addr = g_postproc_priv[proc_id].rslt_info_addr;
	UINT32 rslt_info_size = _vendor_ai_cpu_postproc_get_rslt_info_size();
	UINT32 rslt_num = 0;
	UINT32 req_size = 0;

	{
		VENDOR_AIS_OUTPUT_CLASS *p_out = (VENDOR_AIS_OUTPUT_CLASS *)g_postproc_priv[proc_id].rslt_mem_addr;
		VENDOR_AI_POSTPROC_RESULT_INFO *p_rslt = (VENDOR_AI_POSTPROC_RESULT_INFO *)rslt_info_addr;
#if USE_NEON
#if AI_V4
		UINT32 width    = p_parm->shape.width;
		UINT32 height   = p_parm->shape.height;
		UINT32 channels = p_parm->shape.channel;
		UINT32 num      = p_parm->shape.batch_num;
#else
		UINT32 width	= p_parm->width;
		UINT32 height	= p_parm->height;
		UINT32 channels = p_parm->channel;
		UINT32 num		= p_parm->batch_num;
#endif
		UINT32 top_n	= p_parm->top_n;
		UINT32 i, j, k, r;

		if (width == 1 && height == 1) {
			rslt_num = num;
		} else {
			rslt_num = num * channels * height;
		}
#else
		UINT32 i = 0;
		rslt_num = 1;
#endif
		req_size = sizeof(VENDOR_AI_POSTPROC_RESULT_INFO) + rslt_num * sizeof(VENDOR_AI_POSTPROC_RESULT);
		if (rslt_info_size < req_size) {
			printf("cpu_pp: err, results mem size(%d) is not enough, require: %d\r\n", (int)rslt_info_size, (int)req_size);
			return HD_ERR_BAD_DATA;
		}

		p_rslt->result_num = rslt_num;
		p_rslt->p_result = (VENDOR_AI_POSTPROC_RESULT *)(rslt_info_addr + sizeof(VENDOR_AI_POSTPROC_RESULT_INFO));
#if USE_NEON
		if (width == 1 && height == 1) {
			for (i = 0; i < top_n; i++) {
				p_rslt->p_result[0].no[i]		= p_out[i].no;
				p_rslt->p_result[0].score[i]	= p_out[i].score;
			}
		} else {
			for (i = 0; i < num; i++) {
				for (j = 0; j < channels; j++) {
					for (k = 0; k < height; k++) {
						r = i * channels * height + j * height + k;
						p_rslt->p_result[r].no[0]		= p_out[0].no;
						p_rslt->p_result[r].score[0]	= p_out[0].score;

						p_out += top_n;
					}
				}
			}
		}
#else
		for (i = 0; i < NN_POSTPROC_TOP_N; i++) {
			p_rslt->p_result[0].no[i]		= p_out[i].no;
			p_rslt->p_result[0].score[i]	= p_out[i].score;
		}
#endif

	}

	return HD_OK;
}

HD_RESULT _vendor_ais_set_shape(UINT32 proc_id, VENDOR_AIS_SHAPE shape)
{
	VENDOR_AIS_SHAPE *p_shape;

	if (proc_id >= MAX_PROC_CNT) {
		printf("cpu_pp: err, proc_id(%lu) >= max limit(%d)r\n", proc_id, MAX_PROC_CNT);
		return HD_ERR_LIMIT;
	}

	p_shape = &g_postproc_priv[proc_id].shape;
	p_shape->num = shape.num;
	p_shape->channels = shape.channels;
	p_shape->height = shape.height;
	p_shape->width = shape.width;
	p_shape->lineofs = shape.lineofs;

#if 0 // debug
	printf("set_shape: proc_id(%lu) num(%ld), chn(%ld), h(%ld), w(%ld), lineofs(%ld)\n",
			proc_id,
			g_postproc_priv[proc_id].shape.num,
			g_postproc_priv[proc_id].shape.channels,
			g_postproc_priv[proc_id].shape.height,
			g_postproc_priv[proc_id].shape.width,
			g_postproc_priv[proc_id].shape.lineofs);
#endif

	return HD_OK;
}

#if POST_USE_CPU_STRUCT
static HD_RESULT _vendor_ais_postproc(UINT32 proc_id, NN_CPU_PARM *p_parm)
#else
static HD_RESULT _vendor_ais_postproc(UINT32 proc_id, NN_POSTPROC_PARM *p_parm)
#endif
{
#if CNN_25_MATLAB
	#if POST_USE_CPU_STRUCT
	INT16 *p_in     = (INT16 *)p_parm->addr_in;
	VENDOR_AIS_OUTPUT_CLASS *p_classes = (VENDOR_AIS_OUTPUT_CLASS *)g_postproc_priv[proc_id].rslt_mem_addr;
	#else
	INT16 *p_in     = (INT16 *)p_parm->in_addr;
	VENDOR_AIS_OUTPUT_CLASS *p_classes = (VENDOR_AIS_OUTPUT_CLASS *)g_postproc_priv[proc_id].rslt_mem_addr;
	#endif
#else
#if POST_USE_CPU_STRUCT
	UINT32 in_addr = p_parm->addr_in;
	VENDOR_AIS_OUTPUT_CLASS *p_classes = (VENDOR_AIS_OUTPUT_CLASS *)g_postproc_priv[proc_id].rslt_mem_addr;
#else
	UINT32 in_addr = p_parm->in_addr;
	VENDOR_AIS_OUTPUT_CLASS *p_classes = (VENDOR_AIS_OUTPUT_CLASS *)g_postproc_priv[proc_id].rslt_mem_addr;
#endif
#endif
	//UINT8 *p_buftop = (UINT8 *)p_parm->addr_out;
	#if POST_USE_CPU_STRUCT
	UINT32 width        = 1;
	UINT32 height       = 1;
	UINT32 channel      = p_parm->channel;
	UINT32 batch_num    = p_parm->batch;
	#else // POST_USE_CPU_STRUCT
#if AI_V4
	UINT32 width        = p_parm->shape.width;
	UINT32 height       = p_parm->shape.height;
	UINT32 channel      = p_parm->shape.channel;
	UINT32 batch_num    = p_parm->shape.batch_num;
#else
#if USE_NEON
	UINT32 width        = p_parm->width;
	UINT32 height       = p_parm->height;
#else
	UINT32 width        = 1;
	UINT32 height       = 1;
#endif
	UINT32 channel      = p_parm->channel;
	UINT32 batch_num    = p_parm->batch_num;
#endif
	#endif // POST_USE_CPU_STRUCT
	VENDOR_AIS_ACCURACY_PARM parm = {0};
	VENDOR_AIS_SHAPE shape = {0};
	//INT32 *p_idx = (INT32 *)vendor_ais_getbuffrom(&p_buftop, channel * sizeof(INT32));
	INT32 idx_buf[channel];
	INT32 *p_idx = idx_buf;     //p_parm->tmp_addr;

	shape.num      = batch_num;
	shape.channels = channel;
	shape.height   = height;
	shape.width    = width;
	#if POST_USE_CPU_STRUCT
	//printf("[postproc] addr in = 0x%08X, out = 0x%08X\n", (int)p_parm->addr_in, (int)p_parm->addr_out);
	//printf("[postproc] batch num = %d, ch = %d\n", (int)batch_num, (int)channel);
	#else
	//printf("[postproc] addr in = 0x%08X, out = 0x%08X, tmp = 0x%08X\n", (int)p_parm->in_addr, (int)p_parm->out_addr, (int)p_parm->tmp_addr);
	//printf("[postproc] batch num = %d, ch = %d, top_n = %d\n", (int)batch_num, (int)channel, (int)p_parm->top_n);
	#endif

	_vendor_ais_set_shape(proc_id, shape);

	//INT32 size = batch_num * channel;
	//vendor_ais_readbin((UINT32)p_in, size * sizeof(INT16), "softmax/output.bin");
	/*if (g_class_labels_inited == FALSE) {
		vendor_ais_readtxt((UINT32)g_class_labels, VENDOR_AIS_LBL_LEN, channel, "accuracy/labels.txt");
		g_class_labels_inited = TRUE;
	}
	*/
#if CNN_25_MATLAB
	parm.input	   = p_in;
#else
	parm.in_addr   = in_addr;
#endif
	//parm.labels    = g_class_labels;
	parm.classes   = p_classes;
	parm.shape     = shape;
	parm.top_n     = NN_POSTPROC_TOP_N;
	//parm.expsum    = g_expsum;
	parm.class_idx = p_idx;
	vendor_ais_accuracy_process(&parm);
	/*
#if (NN_MSG_SHOW)
	DBG_DUMP("\r\nClassification Results:\r\n");
	for (i = 0; i < parm.top_n; i++) {
		DBG_DUMP("%ld. no=%ld, label=%s, score=%f\r\n", i + 1, parm.classes[i].no, parm.classes[i].label, parm.classes[i].score);
	}
#endif
	DBG_DUMP("\r\nClassification Results:\r\n");
	for (i = 0; i < parm.top_n; i++) {
		DBG_DUMP("%ld. no=%ld, label=%s, score=%f\r\n", i + 1, parm.classes[i].no, parm.classes[i].label, parm.classes[i].score);
	}*/
	#if POST_USE_CPU_STRUCT
	#else
	p_parm->top_n = parm.top_n;
	_vendor_ais_set_results(proc_id, p_parm);
	memcpy((void*)p_parm->out_addr, (void*)g_postproc_priv[proc_id].rslt_mem_addr, _vendor_ai_cpu_postproc_get_rslt_mem_size()); // copy to original SAO buffer
	#endif
	return HD_OK;
}

/*-----------------------------------------------------------------------------*/
/* Unit Test Functions                                                         */
/*-----------------------------------------------------------------------------*/
HD_RESULT vendor_ais_accuracy_test(UINT32 buf)
{
#define BATCH_NUM   1
#define CHANNELS    1000

	UINT8 *p_buftop = (UINT8 *)buf;
	VENDOR_AIS_ACCURACY_PARM parm = {0};
	VENDOR_AIS_SHAPE shape = {BATCH_NUM, CHANNELS, 1, 1, 1};
	INT32 size = shape.num * shape.channels * shape.height * shape.width;
	INT16 *p_in = (INT16 *)vendor_ais_getbuffrom(&p_buftop, size * sizeof(INT16));
	CHAR *p_labels = (CHAR *)vendor_ais_getbuffrom(&p_buftop, CHANNELS * VENDOR_AIS_LBL_LEN);
	INT32 *p_idx = (INT32 *)vendor_ais_getbuffrom(&p_buftop, CHANNELS * sizeof(INT32));
	VENDOR_AIS_OUTPUT_CLASS *p_classes = (VENDOR_AIS_OUTPUT_CLASS *)vendor_ais_getbuffrom(&p_buftop, BATCH_NUM * NN_POSTPROC_TOP_N * sizeof(VENDOR_AIS_OUTPUT_CLASS));
	INT16 *p_chk = (INT16 *)vendor_ais_getbuffrom(&p_buftop, 3 * sizeof(INT16));
	INT32 i;
	HD_RESULT ret = HD_OK;

	p_chk[0] = 11;
	p_chk[1] = 12;
	p_chk[2] = 13;

	vendor_ais_readbin((UINT32)p_in, size * sizeof(INT16), "softmax/output.bin");
	vendor_ais_readtxt((UINT32)p_labels, VENDOR_AIS_LBL_LEN, CHANNELS, "accuracy/labels.txt");

#if CNN_25_MATLAB
	parm.input     = p_in;
#else
	parm.in_addr   = (UINT32)p_in;
#endif
	parm.classes   = p_classes;
	parm.shape     = shape;
	parm.top_n     = NN_POSTPROC_TOP_N;
	parm.class_idx = p_idx;
	ret = vendor_ais_accuracy_process(&parm);

	DBG_DUMP("Classification Results:\r\n");
	for (i = 0; i < parm.top_n; i++) {
		DBG_DUMP("%ld. no=%ld, label=%s, score=%f\r\n", i + 1, parm.classes[i].no,
				 &p_labels[parm.classes[i].no * VENDOR_AIS_LBL_LEN], parm.classes[i].score);
	}
	DBG_IND("chk: %d, %d, %d\r\n", p_chk[0], p_chk[1], p_chk[2]);

	return ret;
}

/*-----------------------------------------------------------------------------*/
/* Interface Functions                                                         */
/*-----------------------------------------------------------------------------*/
HD_RESULT vendor_ais_postproc(UINT32 proc_id, UINT32 parm_addr)
{
	HD_RESULT rv;

	if (proc_id >= MAX_PROC_CNT) {
		printf("cpu_pp: err, proc_id(%lu) >= max limit(%d)r\n", proc_id, MAX_PROC_CNT);
		return HD_ERR_LIMIT;
	}
	
	if (g_postproc_priv[proc_id].rslt_info_addr == 0 || g_postproc_priv[proc_id].rslt_mem_addr == 0 || g_postproc_priv[proc_id].rslt_size == 0) {
		printf("cpu_pp: FATAL ERROR : result buffer is NOT set yet. Please call vendor_ai_cpu_postproc_setbuffer() first !! \n");
		return HD_ERR_USER;
	}
	
	#if POST_USE_CPU_STRUCT
	rv = _vendor_ais_postproc(proc_id, (NN_CPU_PARM *)parm_addr);
	#else
	rv = _vendor_ais_postproc(proc_id, (NN_POSTPROC_PARM *)parm_addr);
	#endif

	return rv;
}

UINT32 vendor_ai_cpu_postproc_calcbuffersize(VOID)
{
	UINT32 rslt_info_size = _vendor_ai_cpu_postproc_get_rslt_info_size();
	UINT32 rslt_mem_size  = _vendor_ai_cpu_postproc_get_rslt_mem_size();
	return rslt_info_size + rslt_mem_size;
}

HD_RESULT vendor_ai_cpu_postproc_setbuffer(UINT32 proc_id, UINT32 p_out_addr, UINT32 out_size)
{
	if (proc_id >= MAX_PROC_CNT) {
		printf("cpu_pp: err, proc_id(%lu) >= max limit(%d)r\n", proc_id, MAX_PROC_CNT);
		return HD_ERR_LIMIT;
	}

	if (p_out_addr == 0) {
		printf("cpu_pp: err, p_out_addr = 0?\n");
		return HD_ERR_NULL_PTR;
	}

	if (out_size == 0) {
		printf("cpu_pp: err, out_size = 0?\n");
		return HD_ERR_INV;
	}

	memset((void*)p_out_addr, 0, out_size);

	g_postproc_priv[proc_id].rslt_info_addr = p_out_addr;
	g_postproc_priv[proc_id].rslt_mem_addr  = p_out_addr + _vendor_ai_cpu_postproc_get_rslt_info_size();
	g_postproc_priv[proc_id].rslt_size      = out_size;
	
	return HD_OK;
}

VENDOR_AI_POSTPROC_RESULT_INFO *vendor_ai_cpu_postproc_getresult(UINT32 proc_id)
{
	if (proc_id >= MAX_PROC_CNT) {
		printf("cpu_pp: err, proc_id(%lu) >= max limit(%d)r\n", proc_id, MAX_PROC_CNT);
		return NULL;
	}

	if (g_postproc_priv[proc_id].rslt_info_addr == 0 || g_postproc_priv[proc_id].rslt_size == 0) {
		return NULL;
	}
	
	return (VENDOR_AI_POSTPROC_RESULT_INFO *)g_postproc_priv[proc_id].rslt_info_addr;
}

HD_RESULT vendor_ai_cpu_postproc_get_labelnum(UINT32 proc_id, UINT32 *p_label_num)
{
	if (p_label_num == NULL) {
		printf("cpu_pp: err, p_label_num is null\n");
		return HD_ERR_NULL_PTR;
	}
	if (proc_id >= MAX_PROC_CNT) {
		printf("cpu_pp: err, proc_id(%lu) >= max limit(%d)r\n", proc_id, MAX_PROC_CNT);
		return HD_ERR_LIMIT;
	}

#if 0 // debug
	printf("get_labelnum: proc_id(%lu) num(%ld), chn(%ld), h(%ld), w(%ld), lineofs(%ld)\n",
			proc_id,
			g_postproc_priv[proc_id].shape.num,
			g_postproc_priv[proc_id].shape.channels,
			g_postproc_priv[proc_id].shape.height,
			g_postproc_priv[proc_id].shape.width,
			g_postproc_priv[proc_id].shape.lineofs);
#endif

	*p_label_num = g_postproc_priv[proc_id].shape.channels;

	return HD_OK;
}

