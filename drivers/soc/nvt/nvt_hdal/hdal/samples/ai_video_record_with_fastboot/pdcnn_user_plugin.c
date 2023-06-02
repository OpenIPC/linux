/**
	@brief Sample code of user plugin.\n

	@file pdcnn_user_plugin.c

	@author JeahYen

	@ingroup mhdal

	@note Nothing.

	Copyright Novatek Microelectronics Corp. 2020.  All rights reserved.
*/

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include "hdal.h"
#include "hd_debug.h"

#include "vendor_ai.h"
#include "vendor_ai_plugin.h"
#include "pdcnn_api.h"
#include "pdcnn_user_plugin.h"

#define CHKPNT			printf("\033[37mCHK: %s, %s: %d\033[0m\r\n",__FILE__,__func__,__LINE__)
#define DBGH(x)			printf("\033[0;35m%s=0x%08X\033[0m\r\n", #x, x)
#define DBGD(x)			printf("\033[0;35m%s=%d\033[0m\r\n", #x, x)

///////////////////////////////////////////////////////////////////////////////

PD_MEM g_pd_mem = {0};
PD_RESULT_INFO g_pd_result = {0};

static HD_RESULT pdcnn_get(UINT32 proc_id, UINT32 layer_id, UINT32 mode, UINT32 layer_param, UINT32 parm_addr, UINT32 cmd, UINT32* buf_addr, UINT32* buf_size)
{
	HD_RESULT rv = HD_ERR_NOT_SUPPORT;
	//printf("[_vendor_ai_cpu1_proc] callback !! mode = %lu, eng = %lu, parm_addr = 0x%08x, proc_id = %lu\n", mode, eng, (UINT)parm_addr, proc_id);

	switch (cmd) {
	case VENDOR_AI_PLUGIN_BUFTYPE:
		//query buffer type
		buf_addr[0] = MAKEFOURCC('R','T','P','D');
		rv = HD_OK;
		break;
	case VENDOR_AI_PLUGIN_BUFSIZE:
		//query buffer size
		buf_size[0] = pd_get_mem_size();
		buf_size[0] = ALIGN_CEIL(buf_size[0], 64);
		rv = HD_OK;
		break;
	case VENDOR_AI_PLUGIN_RESULT:
		//get buffer result
		buf_addr[0] = (UINT32)(void*)&g_pd_result;
		buf_size[0] = sizeof(PD_RESULT_INFO);
		rv = HD_OK;
	default:
		break;
	}
	return rv;
}

static HD_RESULT pdcnn_set(UINT32 proc_id, UINT32 layer_id, UINT32 mode, UINT32 layer_param, UINT32 parm_addr, UINT32 cmd, UINT32 buf_addr, UINT32 buf_size)
{
	HD_RESULT rv = HD_ERR_NOT_SUPPORT;
	//printf("[_vendor_ai_cpu1_proc] callback !! mode = %lu, eng = %lu, parm_addr = 0x%08x, proc_id = %lu\n", mode, eng, (UINT)parm_addr, proc_id);

	switch (cmd) {
	case VENDOR_AI_PLUGIN_BUFADDR:
		//config buffer addr
		pd_set_mem(&g_pd_mem, buf_addr);
		rv = HD_OK;
	default:
		break;
	}

	return rv;
}

static HD_RESULT pdcnn_proc(UINT32 proc_id, UINT32 layer_id, UINT32 mode, UINT32 layer_param, UINT32 parm_addr)
{
	HD_RESULT rv = HD_OK;
	VENDOR_AI_BUF	out_buf = {0};
	UINT32 in_va[6];
	
	//get 6 input from backbone_out
	//cls_prob_32_mem  before depthwise opt gentool 52x version 1347, after depthwise opt gentool 52x version 112, 56x 171 
	rv = vendor_ai_net_get(proc_id, VENDOR_AI_NET_PARAM_OUT(171, 0), &(out_buf));
	hd_common_mem_flush_cache((VOID *)out_buf.va, out_buf.size);
	in_va[0] = out_buf.va;
	//bbox_pred_32_mem 1350, 52x 113, 56x 172
	rv = vendor_ai_net_get(proc_id, VENDOR_AI_NET_PARAM_OUT(172, 0), &(out_buf));
	hd_common_mem_flush_cache((VOID *)out_buf.va, out_buf.size);
	in_va[1] = out_buf.va;
	//cls_prob_16_mem 1426, 52x 126, 56x 189
	rv = vendor_ai_net_get(proc_id, VENDOR_AI_NET_PARAM_OUT(189, 0), &(out_buf));
	hd_common_mem_flush_cache((VOID *)out_buf.va, out_buf.size);
	in_va[2] = out_buf.va;
	//bbox_pred_16_mem 1427, 52x 127, 56x 190
	rv = vendor_ai_net_get(proc_id, VENDOR_AI_NET_PARAM_OUT(190, 0), &(out_buf));
	hd_common_mem_flush_cache((VOID *)out_buf.va, out_buf.size);
	in_va[3] = out_buf.va;
	//cls_prob_8_mem 1579, 52x 152, 56x 228
	rv = vendor_ai_net_get(proc_id, VENDOR_AI_NET_PARAM_OUT(228, 0), &(out_buf));
	hd_common_mem_flush_cache((VOID *)out_buf.va, out_buf.size);
	in_va[4] = out_buf.va;
	//bbox_pred_8_mem 1580, 52x 155, 56x 233
	rv = vendor_ai_net_get(proc_id, VENDOR_AI_NET_PARAM_OUT(233, 0), &(out_buf));
	hd_common_mem_flush_cache((VOID *)out_buf.va, out_buf.size);
	in_va[5] = out_buf.va;
	
	//do postproc
	get_pdcnn_postpara(&g_pd_mem, in_va);

	//set result
	g_pd_result.sign = MAKEFOURCC('R','T','P','D');
	g_pd_result.chunk_size = 0;
	g_pd_result.result_num = g_pd_mem.out_num;
	g_pd_result.p_result = (PD_RESULT *)g_pd_mem.out_result.va;

	return rv;
}

static VENDOR_AI_ENGINE_PLUGIN pdcnn_postproc = {
	.sign = MAKEFOURCC('R','T','P','D'),
	.eng = 1, //cpu
	.ch = 0,
	.get_cb = pdcnn_get,
	.set_cb = pdcnn_set,
	.proc_cb = pdcnn_proc
};

void* get_pdcnn_postproc_plugin(void)
{
	return (void*)&pdcnn_postproc;
}

