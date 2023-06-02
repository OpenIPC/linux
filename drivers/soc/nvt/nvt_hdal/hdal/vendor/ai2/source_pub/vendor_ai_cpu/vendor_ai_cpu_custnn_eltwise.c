/**
	@brief Source file of custom layers.

	@file vendor_ai_cpu_custnn.c

	@ingroup vendor_ai_cpu_custnn

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

/*-----------------------------------------------------------------------------*/
/* Macro Constant Definitions                                                  */
/*-----------------------------------------------------------------------------*/
#define CUST_ELT_PRINT_PARM FALSE


/*-----------------------------------------------------------------------------*/
/* Interface Functions                                                         */
/*-----------------------------------------------------------------------------*/


HD_RESULT custnn_cpu_elt_process(CUSTNN_ELT_PARM* p_parm, UINT32 in0_addr, UINT32 in1_addr, UINT32 out_addr)

{
	UINT32 img_size = p_parm->size_width*p_parm->size_height;
	UINT32 ch = p_parm->size_channel;
	UINT32 pixel_idx = 0;
	UINT32 ch_idx = 0;
	INT64 val0=0, val1=0, oval=0;// scval=0, sfval=0;
	INT16* in0_16 = (INT16*)in0_addr;
	INT16* in1_16 = (INT16*)in1_addr;
	INT16* out_16 = (INT16*)out_addr;
	INT8* in0_8 = (INT8*)in0_addr;
	INT8* in1_8 = (INT8*)in1_addr;
	INT8* out_8 = (INT8*)out_addr;
	UINT16* in0_u16 = (UINT16*)in0_addr;
	UINT16* in1_u16 = (UINT16*)in1_addr;
	UINT16* out_u16 = (UINT16*)out_addr;
	UINT8* in0_u8 = (UINT8*)in0_addr;
	UINT8* in1_u8 = (UINT8*)in1_addr;
	UINT8* out_u8 = (UINT8*)out_addr;
	
	int elt0_bit = ((p_parm->in0_type >> 1) > 0)?16:8;
	int elt1_bit = ((p_parm->in1_type >> 1) > 0)?16:8;
	int out_bit  = ((p_parm->out_type >> 1) > 0)?16:8;
	int outBitDepth = (out_bit == 16)?1:0;
	
	int elt0_sign = ((p_parm->in0_type & 0x1) > 0)?0:1;
	int elt1_sign = ((p_parm->in1_type & 0x1) > 0)?0:1;
	int out_sign  = ((p_parm->out_type & 0x1) > 0)?0:1;
	
	int elt0_lob = -(1<<(elt0_bit-1));
    int elt0_upb = (1<<(elt0_bit-1))-1;
    int elt1_lob = -(1<<(elt1_bit-1));
    int elt1_upb = (1<<(elt1_bit-1))-1;
    int elt_olob = -(1<<(out_bit-1));
    int elt_oupb = (1<<(out_bit-1))-1;
	
	int elt_out_shift = p_parm->elt_out_shift;

	INT64 elt_in_upb   = (((INT64)1)<<(48-1))-(INT64)1;
    INT64 elt_in_dnb   = -(((INT64)1)<<(48-1));
    INT64 elt_out_upb   = (((INT64)1)<<(32-1))-(INT64)1;
    INT64 elt_out_dnb   = -(((INT64)1)<<(32-1));
	
	int Sout = 0;
	
	//int leakyVal 	= p_parm->relu_leaky_val;
    //int leakyshf 	= p_parm->relu_leaky_shift;
	//int reluShfSign = (p_parm->relu_shift > 0)?1:0;
    //int reluShf     = (p_parm->relu_shift > 0)?p_parm->relu_shift:((-1)*p_parm->relu_shift);
	//int shift = 0;
	#if CUST_ELT_PRINT_PARM
	printf("in1 addr = 0x%08X\n", (unsigned int)in0_addr);
	printf("in2 addr = 0x%08X\n", (unsigned int)in1_addr);
	printf("in0_type: %d\n", (int)p_parm->in0_type);
	printf("in1_type: %d\n", (int)p_parm->in1_type);
	printf("out_type: %d\n", (int)p_parm->out_type);
	printf("size_width: %d\n", (int)p_parm->size_width);
	printf("size_height: %d\n", (int)p_parm->size_height);
	printf("size_channel: %d\n", (int)p_parm->size_channel);
	printf("batch_num: %d\n", (int)p_parm->batch_num);
	printf("in_ofs_line_ofs: %d\n", (int)p_parm->in_ofs_line_ofs);
	printf("in_ofs_channel_ofs: %d\n", (int)p_parm->in_ofs_channel_ofs);
	printf("in_ofs_batch_ofs: %d\n", (int)p_parm->in_ofs_batch_ofs);
	printf("out_ofs_line_ofs: %d\n", (int)p_parm->out_ofs_line_ofs);
	printf("out_ofs_channel_ofs: %d\n", (int)p_parm->out_ofs_channel_ofs);
	printf("out_ofs_batch_ofs: %d\n", (int)p_parm->out_ofs_batch_ofs);
	printf("elt_mode: %d\n", (int)p_parm->elt_mode);
	printf("elt_coeff0: %d\n", (int)p_parm->elt_coeff0);
	printf("elt_coeff1: %d\n", (int)p_parm->elt_coeff1);
	printf("elt_shift0: %d\n", (int)p_parm->elt_shift0);
	printf("elt_shift1: %d\n", (int)p_parm->elt_shift1);
	printf("elt_out_shift: %d\n", (int)p_parm->elt_out_shift);
	/*printf("relu_en: %d\n", (int)p_parm->relu_en);
	printf("relu_shift: %d\n", (int)p_parm->relu_shift);
	printf("relu_leaky_val: %d\n", (int)p_parm->relu_leaky_val);
	printf("relu_leaky_shift: %d\n", (int)p_parm->relu_leaky_shift);*/
	#endif


	






	//hd_common_mem_flush_cache((void *)in0_addr, p_parm->in_ofs_batch_ofs*p_parm->batch_num);
	//hd_common_mem_flush_cache((void *)in1_addr, p_parm->in_ofs_batch_ofs*p_parm->batch_num);
	//hd_common_mem_flush_cache((void *)out_addr, p_parm->out_ofs_batch_ofs*p_parm->batch_num);
	
	for (ch_idx = 0; ch_idx < ch; ch_idx++) {
		if (elt0_bit == 16) {
			if (elt0_sign)
				in0_16 = (INT16*)(in0_addr + p_parm->in_ofs_channel_ofs*ch_idx);
			else
				in0_u16 = (UINT16*)(in0_addr + p_parm->in_ofs_channel_ofs*ch_idx);
		} else {
			if (elt0_sign)
				in0_8 = (INT8*)(in0_addr + p_parm->in_ofs_channel_ofs*ch_idx);
			else
				in0_u8 = (UINT8*)(in0_addr + p_parm->in_ofs_channel_ofs*ch_idx);
		}
		if (elt1_bit == 16) {
			if (elt1_sign)
				in1_16 = (INT16*)(in1_addr + p_parm->in_ofs_channel_ofs*ch_idx);
			else
				in1_u16 = (UINT16*)(in1_addr + p_parm->in_ofs_channel_ofs*ch_idx);
		} else {
			if (elt1_sign)
				in1_8 = (INT8*)(in1_addr + p_parm->in_ofs_channel_ofs*ch_idx);
			else
				in1_u8 = (UINT8*)(in1_addr + p_parm->in_ofs_channel_ofs*ch_idx);
		}
		
		if (out_bit == 16) {
			if (out_sign)
				out_16 = (INT16*)(out_addr + p_parm->out_ofs_channel_ofs*ch_idx);
			else 
				out_u16 = (UINT16*)(out_addr + p_parm->out_ofs_channel_ofs*ch_idx);
		} else {
			if (out_sign)
				out_8 = (INT8*)(out_addr + p_parm->out_ofs_channel_ofs*ch_idx);
			else
				out_u8 = (UINT8*)(out_addr + p_parm->out_ofs_channel_ofs*ch_idx);
		}

		for (pixel_idx = 0; pixel_idx < img_size; pixel_idx++) {
			//int tmp_shf = (p_isf_parm[1] > 0)?p_isf_parm[1]:(-1*p_isf_parm[1]);
			//int tmp_shf_dir = (p_isf_parm[1] > 0)?0:1;
			INT64 tmp_in0_val = 0;
			INT64 tmp_in1_val = 0;
			if (elt0_bit == 16) {
				if (elt0_sign)
					tmp_in0_val = (INT64)in0_16[pixel_idx];
				else
					tmp_in0_val = (INT64)in0_u16[pixel_idx];
			} else {
				if (elt0_sign)
					tmp_in0_val = (INT64)in0_8[pixel_idx];
				else
					tmp_in0_val = (INT64)in0_u8[pixel_idx];
			}
			
			if (elt1_bit == 16) {
				if (elt1_sign)
					tmp_in1_val = (INT64)in1_16[pixel_idx];
				else
					tmp_in1_val = (INT64)in1_u16[pixel_idx];
			} else {
				if (elt1_sign)
					tmp_in1_val = (INT64)in1_8[pixel_idx];
				else
					tmp_in1_val = (INT64)in1_u8[pixel_idx];
			}
			/*tmp_in0_val = custnn_r_scale_shift(tmp_in0_val, tmp_shf_dir, tmp_shf, p_isf_parm[0], 32, in_sign);
			tmp_shf = (p_isf_parm[3] > 0)?p_isf_parm[3]:(-1*p_isf_parm[3]);
			tmp_shf_dir = (p_isf_parm[3] > 0)?0:1;
			tmp_in1_val = custnn_r_scale_shift(tmp_in1_val, tmp_shf_dir, tmp_shf, p_isf_parm[2], 32, in_sign);*/
		
			
			val0 = (INT64)(tmp_in0_val * (INT64)p_parm->elt_coeff0);
            val1 = (INT64)(tmp_in1_val * (INT64)p_parm->elt_coeff1);
			
			if (p_parm->elt_shift0 < 0) {
				val0 = custnn_int64_leftShift(val0, (unsigned char)((-1)*p_parm->elt_shift0));
			} else {
				val0 = custnn_int64_rightShift(val0, (unsigned char)(p_parm->elt_shift0));
			}
			
			if (p_parm->elt_shift1 < 0) {
				val1 = custnn_int64_leftShift(val1, (unsigned char)((-1)*p_parm->elt_shift1));
			} else {
				val1 = custnn_int64_rightShift(val1, (unsigned char)(p_parm->elt_shift1));
			}

            val0 = custnn_int64_clamp(val0, elt_in_dnb, elt_in_upb);
            val1 = custnn_int64_clamp(val1, elt_in_dnb, elt_in_upb);
			
			if (p_parm->elt_mode == 0) {
                //add
                oval = val0 + val1;
                oval = custnn_int64_rightShift(oval, elt_out_shift);
            } else if (p_parm->elt_mode == 1) {
                //multiply
                val0 = custnn_int64_clamp(val0, elt0_lob, elt0_upb);
                val1 = custnn_int64_clamp(val1, elt1_lob, elt1_upb);
                oval = val0 * val1;
                oval = custnn_int64_rightShift(oval, elt_out_shift);
            } else {
                //max
                oval = (val0 > val1) ? val0 : val1;
                oval = custnn_int64_rightShift(oval, elt_out_shift);
            }
			
			oval = custnn_int64_clamp(oval, elt_out_dnb, elt_out_upb);
			
			//TODO: scale shift with ofs
			/*tmp_shf = (p_osf_parm[1] > 0)?p_osf_parm[1]:(-1*p_osf_parm[1]);
			tmp_shf_dir = (p_osf_parm[1] > 0)?0:1;
			oval = custnn_r_scale_shift(oval, tmp_shf_dir, tmp_shf, p_osf_parm[0], out_bit, outSign);*/

			Sout = (int)custnn_int64_clamp((INT64)oval, elt_olob, elt_oupb);
			
			if (outBitDepth){
				if (out_sign)
					out_16[pixel_idx] = (INT16)Sout;
				else
					out_u16[pixel_idx] = (UINT16)Sout;
            } else {
				if (out_sign)
					out_8[pixel_idx] = (INT8)Sout;
				else
					out_u8[pixel_idx] = (UINT8)Sout;
            }
		}
	}
	
	//hd_common_mem_flush_cache((void *)out_addr, p_parm->out_ofs_batch_ofs*p_parm->batch_num);
	
	return HD_OK;
}


