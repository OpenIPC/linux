/**
	@brief Source file of cpu util.

	@file vendor_ai_cpu_util.c

	@ingroup vendor_ai_cpu_util

	@note Nothing.

	Copyright Novatek Microelectronics Corp. 2019.  All rights reserved.
*/

/*-----------------------------------------------------------------------------*/
/* Include Files                                                               */
/*-----------------------------------------------------------------------------*/
#include "hd_type.h"
#include "vendor_ai.h"
#include "vendor_ai_util.h"
#include "vendor_ai_cpu_custnn.h"         // NN_CUSTOMER
#include "vendor_ai_cpu_custnn_sample.h"
#include "nvtnn/nvtnn_lib.h"
#include <math.h>
#include <arm_neon.h>

/*-----------------------------------------------------------------------------*/
/* Macro Constant Definitions                                                  */
/*-----------------------------------------------------------------------------*/


/*-----------------------------------------------------------------------------*/
/* Interface Functions                                                         */
/*-----------------------------------------------------------------------------*/
NN_AI_IO_TYPE custnn_get_type(NN_FMT format)
{
	NN_AI_IO_TYPE ret = NN_AI_IO_INT8;
	UINT32 bitdepth = format.sign_bits + format.int_bits + format.frac_bits;

	if (format.sign_bits) {
		if (bitdepth == 16) {
			ret = NN_AI_IO_INT16;
		} else {
			ret = NN_AI_IO_INT8;
		}
	} else {
		if (bitdepth == 16) {
			ret = NN_AI_IO_UINT16;
		} else {
			ret = NN_AI_IO_UINT8;
		}
	}

	return ret;
}

INT64 custnn_int64_rightShift(INT64 val, unsigned char shiftBits)
{
    INT64 oVal = 0;
    INT64 sign = (val < 0) ? -1 : 1;
    INT64 absVal = ( val >= 0 ) ? val : -val;
    INT64 lbVal = ((INT64)(-((UINT64)(0x1)<<63)));
	INT64 ubVal = (INT64)(((UINT64)(0x1)<<63)-1);

    if (shiftBits>63) {
        printf("custnn: right shift 64 might overflow: val = %d, shiftBits = %d\r\n", (unsigned int)val, (unsigned int)shiftBits);
		return oVal;
	}

    if (shiftBits > 0) {
        absVal += ((INT64)1<<(shiftBits-1));     // rounding
        oVal = (absVal>>shiftBits) * sign;
    } else {
        oVal = val;
    }

    if (oVal > ubVal) {
        oVal = ubVal;
    } else if(oVal < lbVal) {
        oVal = lbVal;
    }

    return oVal;
}

INT64 custnn_int64_leftShift(INT64 val, unsigned char shiftBits)
{
    INT64 oVal = 0;
    INT64 sign = (val < 0) ? -1 : 1;
    INT64 absVal = ( val >= 0 ) ? val : -val;
    INT64 lbVal = ((INT64)(-((UINT64)(0x1)<<63)));
	INT64 ubVal = (INT64)(((UINT64)(0x1)<<63)-1);

    if (shiftBits>31 ) {
        printf("custnn: Rounding might overflow: val = %d, shiftBits = %d\r\n", (unsigned int)val, (unsigned int)shiftBits);
    }

    if (shiftBits > 0) {
        oVal = (absVal<<shiftBits) * sign;
    } else {
        oVal = val;
    }

    if (oVal > ubVal) {
        oVal = ubVal;
    } else if(oVal < lbVal) {
        oVal = lbVal;
    }

    return (INT64)oVal;
}
/*
static INT64 custnn_rightShf(INT64 val, unsigned char shiftBits)
{
    INT64 oVal = 0;
    INT64 sign = (val < 0) ? -1 : 1;
    INT64 absVal = ( val >= 0 ) ? val : -val;
    INT64 lbVal = (-((INT64)(1)<<31));
	INT64 ubVal = (((INT64)(1)<<31)-1);

    if(shiftBits>63){
        printf("cusynn: right shift might overflow: val = %d, shiftBits = %d\r\n", (int)val, (int)shiftBits);
    }

    if(shiftBits > 0){
        absVal += ((INT64)1<<(shiftBits-1));
        oVal = (absVal>>shiftBits) * sign;
    }else{
        // FFFFFFFFFFE00000 * FFFFFFFFFFFFFC00 = 0000000080000000
        // (INT32)0000000080000000(positive) -> 80000000(negative)
        oVal = val;
    }

    if(oVal > ubVal){
        oVal = ubVal;
    }else if(oVal < lbVal){
        oVal = lbVal;
    }

    return (int)oVal;
}

static INT64 custnn_leftShift(INT64 val, unsigned char shiftBits)
{
    INT64 oVal  = 0;
    INT64 lbVal = (-((INT64)(1)<<31));
	INT64 ubVal = (((INT64)(1)<<31)-1);

    if(shiftBits>63){
        printf("custnn: left shift might overflow: val = %d, shiftBits = %d\r\n", (int)val, (int)shiftBits);
    }

    oVal = val * (INT64)(1<<shiftBits);
    if(oVal > ubVal){
        oVal = ubVal;
    }else if(oVal < lbVal){
        oVal = lbVal;
    }

    return (int)oVal;
}
*/

INT64 custnn_int64_clamp(INT64 prx, INT64 lb, INT64 ub)
{
	/* avoid integer overflow or underflow */
	if (prx < lb) {
		return (INT64)lb;
	} else if (prx > ub) {
		return (INT64)ub;
	} else {
		return (INT64)prx;
	}

}

INT32 custnn_r_scale_shift(INT64 ival, int ShiftDir, int Shift, int Scale, int bitdepth, int sign)
{
    int out, rbit, sbitupb;
    INT64 clamp_upb, clamp_dnb;
    INT64 oval=0, scval=0, sfval=0, rval;
    const int maxbit = 64;

    if (sign) {
        clamp_upb   = (((INT64)1)<<(bitdepth-1))-(INT64)1;
        clamp_dnb   = -(((INT64)1)<<(bitdepth-1));
    } else {
        clamp_upb   = (((INT64)1)<<(bitdepth))-(INT64)1;
        clamp_dnb   = 0;
    }

    //Scale
    scval = ival * (INT64)Scale;

    //check remaining bit
    if (scval<0) {
        rval = -1*scval;
    } else {
        rval = scval;
    }
    rbit = 0;
    while (rval) {
        rval = (rval>>1);
        rbit++;
    }
    sbitupb = maxbit-rbit-1;

	//Shift
	if (ShiftDir) {
	    // left shift

        // To prevent leftshift > 64-bit
        if (Shift>sbitupb) {
            Shift = sbitupb;
        }

		sfval = custnn_int64_leftShift(scval, Shift);
	} else {
        // right shift
		sfval = custnn_int64_rightShift(scval, Shift);
	}

    oval = custnn_int64_clamp(sfval, (INT64)clamp_dnb, (INT64)clamp_upb);
    out = (int)oval;

    return out;
}

HD_RESULT custnn_cpu_scaleshift_process(UINT32 in_addr, UINT32 out_addr, UINT32 buf_size, UINT32 io_type, INT32 scale, INT32 shift)
{
	UINT32 i = 0;
	int in_sign = ((io_type & 0x1) > 0)?0:1;
	int in_bitdepth = ((io_type >> 1) > 0)?16:8;
	int tmp_shf = (shift > 0)?shift:(-1*shift);
	int tmp_shf_dir = (shift > 0)?0:1;
	INT16*  in_16   = NULL;
	INT8*   in_8    = NULL;
	UINT16* in_u16  = NULL;
	UINT8*  in_u8   = NULL;
	INT16*  out_16  = NULL;
	INT8*   out_8   = NULL;
	UINT16* out_u16 = NULL;
	UINT8*  out_u8  = NULL;
	
	if (in_addr == 0 || out_addr == 0) {
		printf("custnn: input addr is NULL\n");
		return -1;
	}
	
	in_16   = (INT16*)in_addr;
	in_8    = (INT8*)in_addr;
	in_u16  = (UINT16*)in_addr;
	in_u8   = (UINT8*)in_addr;
	out_16  = (INT16*)out_addr;
	out_8   = (INT8*)out_addr;
	out_u16 = (UINT16*)out_addr;
	out_u8  = (UINT8*)out_addr;

	for (i = 0; i < buf_size*8/in_bitdepth; i++) {
		if (io_type == 0) {
			out_8[i]   = (INT8)custnn_r_scale_shift(in_8[i], tmp_shf_dir, tmp_shf, scale, in_bitdepth, in_sign);
		} else if (io_type == 1) {
			out_u8[i]  = (UINT8)custnn_r_scale_shift(in_u8[i], tmp_shf_dir, tmp_shf, scale, in_bitdepth, in_sign);
		} else if (io_type == 2) {
			out_16[i]  = (INT16)custnn_r_scale_shift(in_16[i], tmp_shf_dir, tmp_shf, scale, in_bitdepth, in_sign);
		} else if (io_type == 3) {
			out_u16[i] = (UINT16)custnn_r_scale_shift(in_u16[i], tmp_shf_dir, tmp_shf, scale, in_bitdepth, in_sign);
		} else {
			printf("custnn: unknown type\n");
			return -1;
		}
	}
	
	return HD_OK;
}

VOID change_fix16_to_float(NN_LAYER_OUTPUT_INFO* layer_info, float* layer_out, INT16* layer_int,FLOAT scale_ratio)
{
#define AI_TRANS_NUM_LANE 4
#define AI_TRANS_NUM_VEC  4
#define AI_TRANS_BATCH    (AI_TRANS_NUM_LANE * AI_TRANS_NUM_VEC)

	int dim = layer_info->out_width * layer_info->out_height * layer_info->out_channel * layer_info->out_batch;
	int frac_num = layer_info->out_frac_bit_num, num, other, i;
	float frac_scale;

	INT16 *p_in = layer_int;
	float *p_out = layer_out;
	int16x4x4_t vint_layerin_s16;
	int32x4x4_t vint_layerin_s32;
	float32x4x4_t vint_layerin_f32;
	float32x4x4_t vlayerout;

	num = ALIGN_FLOOR(dim, AI_TRANS_BATCH);

	if (frac_num>=0) {
		frac_scale = 1.0 / (float)(1 << frac_num) * scale_ratio;
	} else {
		frac_scale = (float)(1 << (-frac_num)) * scale_ratio;
	}

	for (i = 0; i < num; i += AI_TRANS_BATCH) {
		vint_layerin_s16 = vld4_s16(p_in);
		vint_layerin_s32.val[0] = vmovl_s16(vint_layerin_s16.val[0]);
		vint_layerin_s32.val[1] = vmovl_s16(vint_layerin_s16.val[1]);
		vint_layerin_s32.val[2] = vmovl_s16(vint_layerin_s16.val[2]);
		vint_layerin_s32.val[3] = vmovl_s16(vint_layerin_s16.val[3]);
		vint_layerin_f32.val[0] = vcvtq_f32_s32(vint_layerin_s32.val[0]);
		vint_layerin_f32.val[1] = vcvtq_f32_s32(vint_layerin_s32.val[1]);
		vint_layerin_f32.val[2] = vcvtq_f32_s32(vint_layerin_s32.val[2]);
		vint_layerin_f32.val[3] = vcvtq_f32_s32(vint_layerin_s32.val[3]);
		vlayerout.val[0] = vmulq_n_f32(vint_layerin_f32.val[0], frac_scale);
		vlayerout.val[1] = vmulq_n_f32(vint_layerin_f32.val[1], frac_scale);
		vlayerout.val[2] = vmulq_n_f32(vint_layerin_f32.val[2], frac_scale);
		vlayerout.val[3] = vmulq_n_f32(vint_layerin_f32.val[3], frac_scale);
		vst4q_f32(p_out, vlayerout);

		p_in += AI_TRANS_BATCH;
		p_out += AI_TRANS_BATCH;
	}

	other = dim - num;
	for (i = 0; i < other; i ++) {
		*p_out = *p_in * frac_scale;
		p_out++;
		p_in++;
	}
}


VOID change_fix8_to_float(NN_LAYER_OUTPUT_INFO* layer_info, float* layer_out, INT8* layer_int,FLOAT scale_ratio)
{
    int dim = layer_info->out_width * layer_info->out_height * layer_info->out_channel * layer_info->out_batch;
    int frac_num = layer_info->out_frac_bit_num, i;
    float frac_scale;

    if (frac_num >= 0) {
		frac_scale = 1.0 / (float)(1 << frac_num) * scale_ratio;
    } else {
		frac_scale = (float)(1 << (-frac_num)) * scale_ratio;
    }

    for (i = 0; i < dim; i++) {
		layer_out[i] = layer_int[i] * frac_scale;
    }
}

HD_RESULT vendor_ai_cpu_util_float2fixed(FLOAT *in_data, FLOAT in_scale_ratio, VOID *out_data, HD_VIDEO_PXLFMT out_fmt, INT32 data_size)
{
	INT32 sign_bits = HD_VIDEO_PXLFMT_SIGN(out_fmt);
	INT32 bits = HD_VIDEO_PXLFMT_BITS(out_fmt);
	INT32 frac_bits = HD_VIDEO_PXLFMT_FRAC(out_fmt);
	UINT32 ai_pxlfmt = (HD_VIDEO_PXLFMT_CLASS_AI << 28) |
						((sign_bits & 0xf) << 24) |
						((bits & 0xff) << 16);

	switch (ai_pxlfmt) {
		case HD_VIDEO_PXLFMT_AI_SINT8:
			{
				floattoint8_neon(in_data, out_data, frac_bits, data_size, in_scale_ratio);
			}
			break;
		case HD_VIDEO_PXLFMT_AI_UINT8:
			{
				floattouint8_neon(in_data, out_data, frac_bits, data_size, in_scale_ratio);
			}
			break;
		case HD_VIDEO_PXLFMT_AI_SINT16:
			{
				floattoint16_neon(in_data, out_data, frac_bits, data_size, in_scale_ratio);
			}
			break;
		case HD_VIDEO_PXLFMT_AI_UINT16:
			{
				floattouint16_neon(in_data, out_data, frac_bits, data_size, in_scale_ratio);
			}
			break;
		default:
			printf("vendor_ai_cpu_util_float2fixed: Not support for this output format(%#x)", out_fmt);
			break;
	}
	return HD_OK;
}

HD_RESULT vendor_ai_cpu_util_fixed2float(VOID *in_data, HD_VIDEO_PXLFMT in_fmt, FLOAT *out_data, FLOAT out_scale_ratio, INT32 data_size)
{
	INT32 sign_bits = HD_VIDEO_PXLFMT_SIGN(in_fmt);
	INT32 bits = HD_VIDEO_PXLFMT_BITS(in_fmt);
	INT32 frac_bits = HD_VIDEO_PXLFMT_FRAC(in_fmt);
	UINT32 ai_pxlfmt = (HD_VIDEO_PXLFMT_CLASS_AI << 28) |
						((sign_bits & 0xf) << 24) |
						((bits & 0xff) << 16);

	switch (ai_pxlfmt) {
		case HD_VIDEO_PXLFMT_AI_SINT8:
			{
				int8tofloat_neon(in_data, out_data, frac_bits, data_size, out_scale_ratio);
			}
			break;
		case HD_VIDEO_PXLFMT_AI_UINT8:
			{
				uint8tofloat_neon(in_data, out_data, frac_bits, data_size, out_scale_ratio);
			}
			break;
		case HD_VIDEO_PXLFMT_AI_SINT16:
			{
				int16tofloat_neon(in_data, out_data, frac_bits, data_size, out_scale_ratio);
			}
			break;
		case HD_VIDEO_PXLFMT_AI_UINT16:
			{
				uint16tofloat_neon(in_data, out_data, frac_bits, data_size, out_scale_ratio);
			}
			break;
		default:
			printf("vendor_ai_cpu_util_fixed2float: Not support for this input format(%#x)", in_fmt);
			break;
	}
	return HD_OK;
}

