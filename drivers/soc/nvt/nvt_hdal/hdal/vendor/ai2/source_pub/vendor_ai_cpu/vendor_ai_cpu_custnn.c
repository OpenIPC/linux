/**
	@brief Source file of custom layers.

	@file net_cust_layer.c

	@ingroup custnn

	@note Nothing.

	Copyright Novatek Microelectronics Corp. 2019.  All rights reserved.
*/

/*-----------------------------------------------------------------------------*/
/* Include Files                                                               */
/*-----------------------------------------------------------------------------*/
#include "hd_type.h"
#include "vendor_ai_util.h"
#include "vendor_ai_comm.h"
#include "vendor_ai_net_flow.h"
#include "vendor_ai_cpu_custnn.h"         // NN_CUSTOMER
#include "vendor_ai_cpu_custnn_sample.h"
#include <stdio.h>
#include <string.h>
#if NN_USE_DSP
#include "custnn_dsp.h"
#endif
#include "hd_common.h"



/*-----------------------------------------------------------------------------*/
/* Macro Constant Definitions                                                  */
/*-----------------------------------------------------------------------------*/
#define CUST_PRINT_PARM     FALSE

typedef struct _CUSTNN_POOL_PROTO_PARM {
	INT32 pool_type;	// 0:MAX; 1:average
	INT32 kerl_sz;
	INT32 stride;
	INT32 pad;
} CUSTNN_POOL_PROTO_PARM;

typedef struct _CUSTNN_SLICE_PROTO_PARM {
	INT32 axis;
	INT32 slice_point_cnt;
	INT32 slice_point[256];
} CUSTNN_SLICE_PROTO_PARM;
/*-----------------------------------------------------------------------------*/
/* Interface Functions                                                         */
/*-----------------------------------------------------------------------------*/
static INT32 get_hashcode(CHAR *p_str)
{
	INT32 hash = 0, i;
	INT32 len = strlen(p_str);
	if (len == 0) {
		return hash;
	}
	for (i = 0; i < len; i++) {
		hash = (31 * hash) + (INT32)p_str[i];
	}
	return hash;
}
UINT32 vendor_ai_cpu_cust_get_layer_id(UINT32 parm_addr)
{
	NN_CUSTOM_PARM *p_head = (NN_CUSTOM_PARM *)parm_addr;
#if CUST_SUPPORT_MULTI_IO
	UINT32 input_num = p_head->input_num;
	UINT32 output_num = p_head->output_num;
	UINT32 model_num = p_head->model_num;
	UINT32 dim_addr = (UINT32)(parm_addr + sizeof(NN_CUSTOM_PARM) + (input_num+output_num+model_num)*sizeof(NN_DATA));
	UINT32 *p_layer_type_id = (UINT32 *)(dim_addr + sizeof(NN_CUSTOM_DIM)*(input_num+output_num));
#else
	UINT32 *p_layer_type_id = (UINT32 *)(p_head + 1);
#endif

	return p_layer_type_id[0];
}

HD_RESULT vendor_ai_cpu_cust_init(UINT32 parm_addr, UINT32 net_id)
{
	/*
		custom parm format:
		1. custom_layer_type_id(UINT32)
		2. weight_num(UINT32), weight_size_1(UINT32), weight_size_2(UINT32), ...
		   (weights at p_head->model.va)
		3. prototxt_parm_len(UINT32),
		4. isf_len(INT32), isf_mul_1(INT32), isf_shift_1(INT32), isf_mul_2(INT32), isf_shift_2(INT32), ...
		5. osf_len(INT32), osf_mul_1(INT32), osf_shift_1(INT32), osf_mul_2(INT32), osf_shift_2(INT32), ...
		6. bin_parm_len(UINT32), p_prototxt_parm, p_bin_parm
	*/

	return HD_OK;
}

HD_RESULT vendor_ai_cpu_cust_uninit(UINT32 parm_addr, UINT32 net_id)
{
	/*
		custom parm format:
		1. custom_layer_type_id(UINT32)
		2. weight_num(UINT32), weight_size_1(UINT32), weight_size_2(UINT32), ...
		   (weights at p_head->model.va)
		3. prototxt_parm_len(UINT32),
		4. isf_len(INT32), isf_mul_1(INT32), isf_shift_1(INT32), isf_mul_2(INT32), isf_shift_2(INT32), ...
		5. osf_len(INT32), osf_mul_1(INT32), osf_shift_1(INT32), osf_mul_2(INT32), osf_shift_2(INT32), ...
		6. bin_parm_len(UINT32), p_prototxt_parm, p_bin_parm
	*/

	return HD_OK;
}

HD_RESULT vendor_ai_cpu_cust_set_tmp_buf(NN_DATA tmp_buf, UINT32 parm_addr)
{
	/*
		custom parm format:
		1. custom_layer_type_id(UINT32)
		2. weight_num(UINT32), weight_size_1(UINT32), weight_size_2(UINT32), ...
		   (weights at p_head->model.va)
		3. prototxt_parm_len(UINT32),
		4. isf_len(INT32), isf_mul_1(INT32), isf_shift_1(INT32), isf_mul_2(INT32), isf_shift_2(INT32), ...
		5. osf_len(INT32), osf_mul_1(INT32), osf_shift_1(INT32), osf_mul_2(INT32), osf_shift_2(INT32), ...
		6. bin_parm_len(UINT32), p_prototxt_parm, p_bin_parm
	*/
#if 0
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
#endif

	return HD_OK;
}

static HD_RESULT _vendor_ai_cpu_cust_set_ext_info(UINT32 proc_id, UINT32 output_num, NN_CUSTOM_DIM* p_output_dim)
{
	VENDOR_AI_NET_INFO_PROC *p_proc = _vendor_ai_info(proc_id);
	VENDOR_AIS_FLOW_MAP_MEM_PARM *p_mem_manager;
	VENDOR_AIS_FLOW_MEM_PARM *p_mem;
	NN_GEN_NET_INFO net_info = {0};
	NN_GEN_MODEL_HEAD *p_head;
	NN_GEN_MODE_CTRL *p_mctrl;
	//NN_DATA *p_sai, *p_sao;
	HD_RESULT rv = HD_OK;
	UINT32* p_ex_head = NULL;
	UINT32 ex_total_size = 0;
	UINT32 dst_tag = (UINT32)((UINT32)('O') | ((UINT32)('I')<<8) | ((UINT32)('N')<<16) | ((UINT32)('F')<<24));
	UINT32 num_output_layer = 0, tmp_size = 0;

	if (p_output_dim == NULL) {
		DBG_ERR("p_output_dim is null?\r\n");
		return HD_ERR_NULL_PTR;
	}

	p_mem_manager = &p_proc->mem_manager;
	if (p_mem_manager == NULL) {
		DBG_ERR("p_mem_manager is null?\r\n");
		return HD_ERR_INV;
	}

	p_mem = &p_mem_manager->user_parm;
	if ((p_mem == NULL) || (p_mem->pa == 0) || (p_mem->va == 0) || (p_mem->size == 0)) {
		DBG_ERR("null input\r\n");
		return HD_ERR_INV;
	}

	rv = vendor_ais_get_net_info(&net_info, p_mem->va);
	if (rv != HD_OK) {
		DBG_ERR("vendor_ais_get_net_info fail...\r\n");
		return rv;
	}

	p_mctrl = net_info.p_mctrl;

	// get layer info
	p_head = net_info.p_head;
	p_ex_head = (UINT32*)(p_mem->va + sizeof(NN_GEN_MODEL_HEAD) + p_head->mode_ctrl_num*sizeof(NN_GEN_MODE_CTRL) + p_head->layer_id_list_size);
	ex_total_size = p_head->external_size;
	while (tmp_size < ex_total_size) {
		UINT32* p_tmp_head = (UINT32*)(((UINT32)p_ex_head) + tmp_size);
		if (p_tmp_head[1] == dst_tag) { // bit1 represent tag_type
			NN_IN_OUT_BUF_INFO* ext_info = (NN_IN_OUT_BUF_INFO *)(((UINT32)p_tmp_head) + 2*sizeof(UINT32));
			if (ext_info == NULL) {
				DBG_ERR("ext_info pointer NULL!\r\n");
				rv = HD_ERR_NULL_PTR;
				goto exit;
			}
			num_output_layer = (p_tmp_head[0] - 2*sizeof(UINT32)) / sizeof(NN_IN_OUT_BUF_INFO); // number of external info

			for(UINT32 i = 0; i < num_output_layer; i++) {
				UINT32 buf_attr, mctrl_id; //, port_id;
				UINT32 imem_addr, omem_addr;

				// parse ext_id
				buf_attr = (ext_info[i].ext_id & 0xf0000000) >> 28;   // 0x9: input buffer, 0xa: output buffer
				//port_id = (ext_info[i].ext_id & 0xfff0000) >> 16;
				mctrl_id = (ext_info[i].ext_id & 0xffff);

				// debug
				/*printf("buf_attr(0x%lx) port_id(0x%lx) mctrl_id(0x%lx) ext_info_num(%lu)\n", buf_attr, port_id, mctrl_id, ext_info_num);
				printf("========= i(%d) ==============\n", (int)i);
				printf("layer_name = %s\n", ext_info[i].layer_name);
				printf("layer_type = %s\n", ext_info[i].layer_type);
				printf("caffe_layer_index = %d\n", ext_info[i].caffe_layer_index);
				printf("fusion_layer_index = %d\n", ext_info[i].fusion_layer_index);
				printf("out_bitdepth = %d\n", ext_info[i].out_bitdepth);
				printf("out_sign_bit_num = %d\n", ext_info[i].out_sign_bit_num);
				printf("out_int_bit_num = %d\n", ext_info[i].out_int_bit_num);
				printf("out_frac_bit_num = %d\n", ext_info[i].out_frac_bit_num);
				printf("out_scale_ratio = %f\n", ext_info[i].out_scale_ratio);
				printf("width = %d\n", ext_info[i].width);
				printf("height = %d\n", ext_info[i].height);
				printf("channel = %d\n", ext_info[i].channel);
				printf("batch = %d\n", ext_info[i].batch);
				printf("time = %d\n", ext_info[i].time);
				printf("out_lofs = %d\n", ext_info[i].out_lofs);
				printf("out_ch_ofs = %d\n", ext_info[i].out_ch_ofs);
				printf("out_batch_ofs  = %lu\n", ext_info[i].out_batch_ofs);
				printf("out_time_ofs = %lu\n", ext_info[i].out_time_ofs);
				printf("ext_id(0x%lx)\n", ext_info[i].ext_id);
				printf("dataorder = %s\n", ext_info[i].data_order);
				printf("\n\n");*/

				// check va fixing
				imem_addr = p_mctrl[mctrl_id].iomem.imem_addr;
				if (imem_addr < p_mem->va) imem_addr += p_mem->va; // if not fix yet(call this funciton before gen_init(), fix it
				omem_addr = p_mctrl[mctrl_id].iomem.omem_addr;
				if (omem_addr < p_mem->va) omem_addr += p_mem->va; // if not fix yet(call this funciton before gen_init(), fix it

				// config to NN_DATA
				if (buf_attr == 0x9) {
					//p_sai = (NN_DATA*)imem_addr;
					//printf("p_sai: ext_info_num(%u), mctrl_id(%lu), port_id(%lu)\n", p_sai[port_id].fmt.reserved, mctrl_id, port_id);
				} else if (buf_attr == 0xa) {
					//p_sao = (NN_DATA*)omem_addr;
					//printf("p_sao: ext_info_num(%u), mctrl_id(%lu), port_id(%lu)\n", p_sao[port_id].fmt.reserved, mctrl_id, port_id);
					ext_info[i].width = p_output_dim->dim[0];
					ext_info[i].height = p_output_dim->dim[1];
					ext_info[i].channel = p_output_dim->dim[2];
					ext_info[i].batch = p_output_dim->dim[3];
					ext_info[i].time = p_output_dim->dim[4];

					ext_info[i].out_lofs = p_output_dim->ofs[0];
					ext_info[i].out_ch_ofs = p_output_dim->ofs[1];
					ext_info[i].out_batch_ofs = p_output_dim->ofs[2];
					ext_info[i].out_time_ofs = p_output_dim->ofs[3];
				} else {
					DBG_ERR("Invalid buf_attr(0x%lx)\r\n", buf_attr);
					rv = HD_ERR_NG;
					goto exit;
				}
			}
			break;
		} else {
			if (p_tmp_head[0] == 0) { // bit0 represent size
				break;
			}
			tmp_size += p_tmp_head[0]; // add tmp_size, move to next
		}
	}

exit:
	return rv;
}



HD_RESULT vendor_ai_cpu_cust(UINT32 parm_addr, UINT32 net_id)
{
	/*
		custom parm format:
		1. custom_layer_type_id(UINT32)
		2. weight_num(UINT32), weight_size_1(UINT32), weight_size_2(UINT32), ...
		   (weights at p_head->model.va)
		3. prototxt_parm_len(UINT32),
		4. isf_len(INT32), isf_mul_1(INT32), isf_shift_1(INT32), isf_mul_2(INT32), isf_shift_2(INT32), ...
		5. osf_len(INT32), osf_mul_1(INT32), osf_shift_1(INT32), osf_mul_2(INT32), osf_shift_2(INT32), ...
		6. bin_parm_len(UINT32), p_prototxt_parm, p_bin_parm
		7. tmp_buf addr(NN_DATA)
	*/
	NN_CUSTOM_PARM *p_head = (NN_CUSTOM_PARM *)(parm_addr);
#if CUST_SUPPORT_MULTI_IO
	UINT32 input_num = p_head->input_num;
	UINT32 output_num = p_head->output_num;
	UINT32 model_num = p_head->model_num;
	NN_DATA* input_info = (NN_DATA*)(parm_addr + sizeof(NN_CUSTOM_PARM));
	NN_DATA* output_info = (NN_DATA*)(parm_addr + sizeof(NN_CUSTOM_PARM) + input_num*sizeof(NN_DATA));
	UINT32 dim_addr = (UINT32)(parm_addr + sizeof(NN_CUSTOM_PARM) + (input_num+output_num+model_num)*sizeof(NN_DATA));
	NN_CUSTOM_DIM* input_dim = (NN_CUSTOM_DIM*)(dim_addr);
	NN_CUSTOM_DIM* output_dim = (NN_CUSTOM_DIM*)(dim_addr + sizeof(NN_CUSTOM_DIM)*input_num);
	UINT32 *p_layer_type_id = (UINT32 *)(dim_addr + sizeof(NN_CUSTOM_DIM)*(input_num+output_num));
#else
	UINT32 *p_layer_type_id = (UINT32 *)(p_head + 1);
#endif
	INT32 layer_type_id = (INT32)(*p_layer_type_id);
	UINT32 *p_weight_num = (UINT32 *)(p_layer_type_id + 1);
	UINT32 weight_num = *p_weight_num;
	UINT32 *p_prototxt_parm_len = (UINT32 *)(p_weight_num + 1 + weight_num);

	UINT32 *p_isf_len = (UINT32 *)(p_prototxt_parm_len + 1);
	UINT32 isf_num = (*p_isf_len) / 8;
	INT32 *p_isf_parm = (INT32 *)(p_isf_len + 1);
	UINT32 *p_osf_len = (UINT32 *)(p_isf_parm + isf_num*2);
	UINT32 osf_num = (*p_osf_len) / 8;
	INT32 *p_osf_parm = (INT32 *)(p_osf_len + 1);
#if 0
	UINT32 *p_bin_parm_len = (UINT32 *)(p_osf_parm + osf_num*2);
	CUSTNN_ELT_PARM *p_elt_parm = (CUSTNN_ELT_PARM *)(p_bin_parm_len + 1);
	CUSTNN_POOL_PARM *p_pool_parm = (CUSTNN_POOL_PARM *)(p_bin_parm_len + 1);
#if CUST_SUPPORT_MULTI_IO
	CUSTNN_CONCAT_PARM *p_concat_parm = (CUSTNN_CONCAT_PARM *)(p_bin_parm_len + 1);
	CUSTNN_SLICE_PARM *p_slice_parm = (CUSTNN_SLICE_PARM *)(p_bin_parm_len + 1);
#endif
#else
	UINT32 *p_bin_parm_len = (UINT32 *)(p_osf_parm + osf_num * 2);
	CHAR *p_prototxt_parm = (CHAR *)(p_bin_parm_len + 1);
	CHAR *p_bin_parm = (CHAR *)(p_prototxt_parm + *p_prototxt_parm_len);
	
	CUSTNN_ELT_PARM *p_elt_parm = (CUSTNN_ELT_PARM *)(p_bin_parm);
	CUSTNN_POOL_PARM *p_pool_parm = (CUSTNN_POOL_PARM *)(p_bin_parm);
#if CUST_SUPPORT_MULTI_IO
	CUSTNN_CONCAT_PARM *p_concat_parm = (CUSTNN_CONCAT_PARM *)(p_bin_parm);
	CUSTNN_SLICE_PARM *p_slice_parm = (CUSTNN_SLICE_PARM *)(p_bin_parm);
#endif
#endif
#if 0
	CUSTNN_POOL_PROTO_PARM *p_proto_pool_parm = (CUSTNN_POOL_PROTO_PARM*)p_prototxt_parm;
	UINT32 i = 0;
	//CUSTNN_SLICE_PROTO_PARM *p_proto_slice_parm = (CUSTNN_SLICE_PROTO_PARM*)p_prototxt_parm;
	printf("pool_type: %d\n", (int)p_proto_pool_parm->pool_type);
	printf("kerl_sz: %d\n", (int)p_proto_pool_parm->kerl_sz);
	printf("stride: %d\n", (int)p_proto_pool_parm->stride);
	printf("pad: %d\n", (int)p_proto_pool_parm->pad);

	printf("temp buf size = %d\n", (unsigned int)p_head->temp_buf_size);

	printf("input num = %d\n", (int)input_num);
	for (i = 0; i < input_num; i++) {
		printf("input %d size = %d\n", (int)i, (unsigned int)input_info[i].size);
		printf("         fmt (frac/int/sign) = %d %d %d\n", (int)input_info[i].fmt.frac_bits, (int)input_info[i].fmt.int_bits, (int)input_info[i].fmt.sign_bits);
		printf("         dim = %d %d %d %d %d\n", (unsigned int)input_dim[i].dim[0], (unsigned int)input_dim[i].dim[1],
		(unsigned int)input_dim[i].dim[2], (unsigned int)input_dim[i].dim[3], (unsigned int)input_dim[i].dim[4] );
		printf("         ofs = %d %d %d %d\n", (unsigned int)input_dim[i].ofs[0], (unsigned int)input_dim[i].ofs[1],
		(unsigned int)input_dim[i].ofs[2], (unsigned int)input_dim[i].ofs[3]);
	}
	
	printf("output num = %d\n", (int)output_num);
	for (i = 0; i < output_num; i++) {
		printf("output %d size = %d\n", (int)i, (unsigned int)output_info[i].size);
		printf("         fmt (frac/int/sign) = %d %d %d\n", (int)output_info[i].fmt.frac_bits, (int)output_info[i].fmt.int_bits, (int)output_info[i].fmt.sign_bits);
		printf("         dim = %d %d %d %d %d\n", (unsigned int)output_dim[i].dim[0], (unsigned int)output_dim[i].dim[1],
		(unsigned int)output_dim[i].dim[2], (unsigned int)output_dim[i].dim[3], (unsigned int)output_dim[i].dim[4] );
		printf("         ofs = %d %d %d %d\n", (unsigned int)output_dim[i].ofs[0], (unsigned int)output_dim[i].ofs[1],
		(unsigned int)output_dim[i].ofs[2], (unsigned int)output_dim[i].ofs[3]);
	}
#endif

#if NN_USE_DSP
	UINT32 in_pa, out_pa;
	NN_POOL_PARM parm = {0};
#endif

#if CUST_PRINT_PARM
#if CUST_SUPPORT_MULTI_IO
	UINT32 i = 0;
	NN_DATA* model_info = (NN_DATA*)(parm_addr + sizeof(NN_CUSTOM_PARM) + (input_num+output_num)*sizeof(NN_DATA));
	for (i = 0; i < input_num; i++) {
		DBGH(input_info[i].pa);
		DBGH(input_info[i].va);
		DBGH(input_info[i].size);
		DBGH(*(UINT32 *)&input_info[i].fmt);
	}
	for (i = 0; i < output_num; i++) {
		DBGH(output_info[i].pa);
		DBGH(output_info[i].va);
		DBGH(output_info[i].size);
		DBGH(*(UINT32 *)&output_info[i].fmt);
	}
	for (i = 0; i < model_num; i++) {
		DBGH(model_info[i].pa);
		DBGH(model_info[i].va);
		DBGH(model_info[i].size);
		DBGH(*(UINT32 *)&model_info[i].fmt);
	}
#else
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
#endif
	DBGD(p_head->parm_size);

	DBGD(*p_layer_type_id);
	DBGD(weight_num);
	//DBGD(layer_parm_len);
	DBGD(*p_bin_parm_len);
	
	//DBG_DUMP("layer_parm=%s\r\n", p_layer_parm);
#endif

	if (layer_type_id == get_hashcode("Pooling")) {
		// pooling
#if CUST_SUPPORT_MULTI_IO
		custnn_cpu_scaleshift_process(input_info[0].va, p_head->temp_buf_addr, input_info[0].size, p_pool_parm->in_type, p_isf_parm[0], p_isf_parm[1]);
		custnn_cpu_pool_process(p_pool_parm, p_head->temp_buf_addr, output_info[0].va);
		custnn_cpu_scaleshift_process(output_info[0].va, output_info[0].va, output_info[0].size, p_pool_parm->out_type, p_osf_parm[0], p_osf_parm[1]);
#else
		custnn_cpu_scaleshift_process(p_head->input.va, p_head->input.va, p_head->input.size, p_pool_parm->in_type, p_isf_parm[0], p_isf_parm[1]);
		custnn_cpu_pool_process(p_pool_parm, p_head->input.va, p_head->output.va);
		custnn_cpu_scaleshift_process(p_head->output.va, p_head->output.va, p_head->output.size, p_pool_parm->out_type, p_osf_parm[0], p_osf_parm[1]);
#endif
} else if (layer_type_id == get_hashcode("Eltwise")) {
		// eltwise
#if CUST_SUPPORT_MULTI_IO
		UINT32 input0_addr = p_head->temp_buf_addr;
		UINT32 input1_addr = p_head->temp_buf_addr + input_info[0].size;
		UINT32 output_addr = output_info[0].va;
		UINT32 input0_size = input_info[0].size;
		UINT32 input1_size = input_info[1].size;
		UINT32 output_size = output_info[0].size;

		custnn_cpu_scaleshift_process(input_info[0].va, input0_addr, input0_size, p_elt_parm->in0_type, p_isf_parm[0], p_isf_parm[1]);
		custnn_cpu_scaleshift_process(input_info[1].va, input1_addr, input1_size, p_elt_parm->in1_type, p_isf_parm[2], p_isf_parm[3]);
		custnn_cpu_elt_process(p_elt_parm, input0_addr, input1_addr, output_addr);
		custnn_cpu_scaleshift_process(output_addr, output_addr, output_size, p_elt_parm->out_type, p_osf_parm[0], p_osf_parm[1]);
#else
		custnn_cpu_scaleshift_process(p_head->input.va, p_head->input.va, p_head->input.size, p_elt_parm->in0_type, p_isf_parm[0], p_isf_parm[1]);
		custnn_cpu_scaleshift_process(p_head->model.va, p_head->model.va, p_head->model.size, p_elt_parm->in1_type, p_isf_parm[2], p_isf_parm[3]);
		custnn_cpu_elt_process(p_elt_parm, p_head->input.va, p_head->model.va, p_head->output.va);
		custnn_cpu_scaleshift_process(p_head->output.va, p_head->output.va, p_head->output.size, p_elt_parm->out_type, p_osf_parm[0], p_osf_parm[1]);
#endif
#if CUST_SUPPORT_MULTI_IO
	} else if (layer_type_id == get_hashcode("Concat")) {
		// concat
		custnn_cpu_concat_process(p_concat_parm, p_head->temp_buf_addr, input_info, input_dim, p_isf_parm, input_num, output_info, output_dim, p_osf_parm, output_num);
	} else if (layer_type_id == get_hashcode("Slice")) {
		// slice
		custnn_cpu_slice_process(p_slice_parm, p_head->temp_buf_addr, input_info, input_dim, p_isf_parm, input_num, output_info, output_dim, p_osf_parm, output_num);
#endif
	} else {
		printf("custnn: unknown customer layer type\n");
	}

	if (_vendor_ai_cpu_cust_set_ext_info(net_id, output_num, output_dim) != HD_OK) {
		printf("custnn: cpu_cust_set_ext_info fail!\n");
	}

	return HD_OK;
}
