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
#include "vendor_ai_cpu_custnn.h"         // NN_CUSTOMER
#include "vendor_ai_cpu_custnn_sample.h"
#include "hd_common.h"
#include <string.h>

/*-----------------------------------------------------------------------------*/
/* Macro Constant Definitions                                                  */
/*-----------------------------------------------------------------------------*/
#define CUST_CONCAT_PRINT_PARM FALSE

/*-----------------------------------------------------------------------------*/
/* Interface Functions                                                         */
/*-----------------------------------------------------------------------------*/
#if CUST_SUPPORT_MULTI_IO
HD_RESULT custnn_cpu_concat_process(CUSTNN_CONCAT_PARM* p_parm, UINT32 temp_buf_addr, 
 NN_DATA* input_info, NN_CUSTOM_DIM* input_dim, INT32 *p_isf_parm, UINT32 input_num, 
 NN_DATA* output_info, NN_CUSTOM_DIM* output_dim, INT32 *p_osf_parm, UINT32 output_num)			
{
	UINT32 i = 0;
	UINT32 output_addr = 0;
	INT32 shift = 0;
	UINT32 *proc_in_addr = NULL;
	UINT32 temp_buf_pointer = temp_buf_addr;
	
	#if CUST_CONCAT_PRINT_PARM
	printf("in1 addr = 0x%08X\n", (unsigned int)input_info[0].va);
	#endif
	
	proc_in_addr = (UINT32*)malloc(input_num*sizeof(UINT32));
	for (i = 0; i < input_num; i++) {		
		proc_in_addr[i] = temp_buf_pointer;
		if (proc_in_addr[i] == 0 || input_info[i].va == 0) {
			printf("concat fail: null input\n");
			free(proc_in_addr);
			return -1;
		}
		custnn_cpu_scaleshift_process(input_info[i].va, proc_in_addr[i], input_info[i].size, custnn_get_type(input_info[i].fmt), p_isf_parm[2*i], p_isf_parm[2*i+1]);
		temp_buf_pointer += input_info[i].size;
	}
	
	output_addr = output_info[0].va;
	if (output_addr == 0) {
		printf("concat fail: null output\n");
		free(proc_in_addr);
		return -1;
	}
	
	for (i = 0; i < input_num; i++) {
		memcpy((VOID*)output_addr, (VOID*)proc_in_addr[i], input_info[i].size);
		if (input_info[i].fmt.frac_bits != output_info[0].fmt.frac_bits) {
			shift = input_info[i].fmt.frac_bits - output_info[0].fmt.frac_bits;
			custnn_cpu_scaleshift_process(output_addr, output_addr, input_info[i].size, custnn_get_type(input_info[i].fmt), 1, shift);
		}
		output_addr += input_info[i].size;
	}
	
	for (i = 0; i < output_num; i++) {
		custnn_cpu_scaleshift_process(output_info[i].va, output_info[i].va, output_info[i].size, custnn_get_type(output_info[i].fmt), p_osf_parm[2*i], p_osf_parm[2*i+1]);
	}
	
	free(proc_in_addr);
	
	return HD_OK;
}
#endif

