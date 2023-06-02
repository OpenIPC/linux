/**
	@brief Source file of custom layers.

	@file vendor_ai_dsp_custnn.c

	@ingroup vendor_ai_dsp_custnn

	@note Nothing.

	Copyright Novatek Microelectronics Corp. 2019.  All rights reserved.
*/

/*-----------------------------------------------------------------------------*/
/* Include Files                                                               */
/*-----------------------------------------------------------------------------*/
#include "hd_type.h"
#include "vendor_ai_util.h"

#include "vendor_ai_dsp_custnn.h"         // NN_CUSTOMER
#include "vendor_ai_dsp_custnn_sample.h"
#include <stdio.h>
#include <string.h>

/*-----------------------------------------------------------------------------*/
/* Macro Constant Definitions                                                  */
/*-----------------------------------------------------------------------------*/
#define CUST_PRINT_PARM     FALSE

/*-----------------------------------------------------------------------------*/
/* Interface Functions                                                         */
/*-----------------------------------------------------------------------------*/
HD_RESULT vendor_ai_dsp_cust_init(UINT32 parm_addr, UINT32 net_id)
{
	/*
		custom parm format:
		1. custom_layer_type_id(UINT32)
		2. weight_num(UINT32), weight_size_1(UINT32), weight_size_2(UINT32), ...
		   (weights at p_head->model.va)
		3. prototxt_parm_len(UINT32),
		4. isf_len(INT32), isf_mul_1(INT32), isf_shift_1(INT32), isf_mul_2(INT32), isf_shift_2(INT32), ...
		5. osf_len(INT32), osf_mul_1(INT32), osf_shift_1(INT32), osf_mul_2(INT32), osf_shift_2(INT32), ...
		6. bin_parm_len (UINT32), bin_parm 
		7. tmp_buf addr(NN_DATA)
	*/

	return HD_OK;
}

HD_RESULT vendor_ai_dsp_cust_uninit(UINT32 parm_addr, UINT32 net_id)
{
	/*
		custom parm format:
		1. custom_layer_type_id(UINT32)
		2. weight_num(UINT32), weight_size_1(UINT32), weight_size_2(UINT32), ...
		   (weights at p_head->model.va)
		3. prototxt_parm_len(UINT32),
		4. isf_len(INT32), isf_mul_1(INT32), isf_shift_1(INT32), isf_mul_2(INT32), isf_shift_2(INT32), ...
		5. osf_len(INT32), osf_mul_1(INT32), osf_shift_1(INT32), osf_mul_2(INT32), osf_shift_2(INT32), ...
		6. bin_parm_len (UINT32), bin_parm 
		7. tmp_buf addr(NN_DATA)
	*/

	return HD_OK;
}

HD_RESULT vendor_ai_dsp_cust_set_tmp_buf(NN_DATA tmp_buf, UINT32 parm_addr)
{
	/*
	custom parm format:
    1. custom_layer_type_id(UINT32)
    2. weight_num(UINT32), weight_size_1(UINT32), weight_size_2(UINT32), ...
       (weights at p_head->model.va)
    3. prototxt_parm_len(UINT32), 
	4. isf_len(INT32), isf_mul_1(INT32), isf_shift_1(INT32), isf_mul_2(INT32), isf_shift_2(INT32), ...
	5. osf_len(INT32), osf_mul_1(INT32), osf_shift_1(INT32), osf_mul_2(INT32), osf_shift_2(INT32), ...
	6. bin_parm_len (UINT32), bin_parm 
	7. tmp_buf addr(NN_DATA)
	*/
	NN_CUSTOM_PARM *p_head = (NN_CUSTOM_PARM *)(parm_addr);
	UINT32 *p_layer_type_id = (UINT32 *)(p_head + 1);
	UINT32 *p_weight_num = (UINT32 *)(p_layer_type_id + 1);
	UINT32 weight_num = *p_weight_num;
	UINT32 *p_layer_parm_len = (UINT32 *)(p_weight_num + 1 + weight_num);

	UINT32 *p_isf_len = (UINT32 *)(p_layer_parm_len + 1);
	UINT32 isf_num = (*p_isf_len) / 8;
	INT32 *p_isf_parm = (INT32 *)(p_isf_len + 1);
	UINT32 *p_osf_len = (UINT32 *)(p_isf_parm + isf_num*2);
	UINT32 osf_num = (*p_osf_len) / 8;
	INT32 *p_osf_parm = (INT32 *)(p_osf_len + 1);
	UINT32 *p_bin_parm_len = (UINT32 *)(p_osf_parm + osf_num*2);
	UINT32 bin_parm_len = p_bin_parm_len[0];
	NN_DATA* p_tmp_buf = NULL;
	
	if (tmp_buf.size > 0 && tmp_buf.va > 0) {
		p_tmp_buf = (NN_DATA*)(parm_addr + sizeof(NN_CUSTOM_PARM) + bin_parm_len);
		memcpy(p_tmp_buf, &tmp_buf, sizeof(NN_DATA));
	}
	
	return HD_OK;
}

HD_RESULT vendor_ai_dsp_cust(UINT32 parm_addr, UINT32 net_id)
{
	/*
	custom parm format:
    1. custom_layer_type_id(UINT32)
    2. weight_num(UINT32), weight_size_1(UINT32), weight_size_2(UINT32), ...
       (weights at p_head->model.va)
    3. prototxt_parm_len(UINT32), 
	4. isf_len(INT32), isf_mul_1(INT32), isf_shift_1(INT32), isf_mul_2(INT32), isf_shift_2(INT32), ...
	5. osf_len(INT32), osf_mul_1(INT32), osf_shift_1(INT32), osf_mul_2(INT32), osf_shift_2(INT32), ...
	6. bin_parm_len (UINT32), bin_parm 
	7. tmp_buf addr(NN_DATA)
	*/
	NN_CUSTOM_PARM *p_head = (NN_CUSTOM_PARM *)(parm_addr);
	UINT32 *p_layer_type_id = (UINT32 *)(p_head + 1);
	UINT32 layer_type_id = *p_layer_type_id;
	//UINT32 *p_weight_num = (UINT32 *)(p_layer_type_id + 1);
	//UINT32 weight_num = *p_weight_num;
	//UINT32 *p_layer_parm_len = (UINT32 *)(p_weight_num + 1 + weight_num);

	//UINT32 *p_isf_len = (UINT32 *)(p_layer_parm_len + 1);
	//UINT32 isf_num = (*p_isf_len) / 8;
	//INT32 *p_isf_parm = (INT32 *)(p_isf_len + 1);
	//UINT32 *p_osf_len = (UINT32 *)(p_isf_parm + isf_num*2);
	//UINT32 osf_num = (*p_osf_len) / 8;
	//INT32 *p_osf_parm = (INT32 *)(p_osf_len + 1);
	//UINT32 *p_bin_parm_len = (UINT32 *)(p_osf_parm + osf_num*2);
	//CUSTNN_ELT_PARM *p_elt_parm = (CUSTNN_ELT_PARM *)(p_bin_parm_len + 1);
	//CUSTNN_POOL_PARM *p_pool_parm = (CUSTNN_POOL_PARM *)(p_bin_parm_len + 1);
#if NN_USE_DSP
	NN_POOL_PARM parm = {0};
#endif

	// user can access tmp_buf if model support this feature
	/*
	UINT32 bin_parm_len = p_bin_parm_len[0];
	NN_DATA* p_tmp_buf_struct = (NN_DATA*)(parm_addr + sizeof(NN_CUSTOM_PARM) + bin_parm_len);
	UINT32* p_tmp_buf = (UINT32*)p_tmp_buf_struct->va;
	
	printf("tmp_buf addr/size = 0x%08X, %d\n", (unsigned int)p_tmp_buf_struct->va, (unsigned int)p_tmp_buf_struct->size);
	printf("0x%08X\n", (unsigned int)p_tmp_buf[0]);
	p_tmp_buf = (UINT32*)(p_tmp_buf_struct->va + p_tmp_buf_struct->size/2);
	printf("0x%08X\n", (unsigned int)p_tmp_buf[0]);
	p_tmp_buf = (UINT32*)(p_tmp_buf_struct->va + p_tmp_buf_struct->size - 1);
	printf("0x%08X\n", (unsigned int)p_tmp_buf[0]);
	memset((VOID*)p_tmp_buf_struct->va, 0, p_tmp_buf_struct->size);
	p_tmp_buf = (UINT32*)(p_tmp_buf_struct->va + p_tmp_buf_struct->size/2);
	printf("after 0x%08X\n", (unsigned int)p_tmp_buf[0]);
	p_tmp_buf = (UINT32*)(p_tmp_buf_struct->va + p_tmp_buf_struct->size - 1);
	printf("after 0x%08X\n", (unsigned int)p_tmp_buf[0]);
	*/
	
#if NN_USE_DSP
	UINT32 in_pa, out_pa;
#endif

#if CUST_PRINT_PARM
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

	DBGD(*p_layer_type_id);
	DBGD(weight_num);
	//DBGD(layer_parm_len);
	DBGD(*p_bin_parm_len);
	
	//DBG_DUMP("layer_parm=%s\r\n", p_layer_parm);
#endif

	switch (layer_type_id) {
#if NN_USE_DSP
	case 1: // pooling
		in_pa  = p_head->input.pa;
		out_pa = p_head->output.pa;
		vendor_ai_dsp_cust_pool(&parm, in_pa, out_pa);
		break;
#endif
	default:
		DBG_DUMP("DSP layer type id %d: not support\r\n", (INT)layer_type_id);
		return HD_ERR_NOT_SUPPORT;
	}

	return HD_OK;
}
