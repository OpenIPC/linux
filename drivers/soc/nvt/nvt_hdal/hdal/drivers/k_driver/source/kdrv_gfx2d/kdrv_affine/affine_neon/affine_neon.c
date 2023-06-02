#include <linux/version.h>
#include <linux/types.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/device.h>
#include <arm_neon.h>
#include <kdrv_type.h>
#include "affine_neon.h"

static int __init affine_neon_module_initial(void)
{
	pr_info("%s: successfully\n", __func__);
	return 0;
}

void affine_neon_array_operation(FLOAT_OPERATION_TYPE type, float *s, float *c, float *y)
{
	float32x4_t v_s, v_c, v_y = {0,0,0,0};
	v_s = vld1q_f32(s);
	v_c = vld1q_f32(c);
	
	switch (type) {
	case FLOAT_ADD:
		v_y = vaddq_f32(v_s, v_c);
		break;
	case FLOAT_SUB:
		v_y = vsubq_f32(v_s, v_c);
		break;
	case FLOAT_MUL:
		v_y = vmulq_f32(v_s, v_c);
		break;
	case FLOAT_MAX:
		v_y = vmaxq_f32(v_s, v_c);
		break;
	case FLOAT_MIN:
		v_y = vminq_f32(v_s, v_c);
		break;

	default:
		break;
	}

	vst1q_f32(y, v_y);
	
	return;
}
EXPORT_SYMBOL(affine_neon_array_operation);

void affine_neon_array_comparison(FLOAT_COMPARISON_TYPE type, float *s, float *c, UINT32 *y)
{
	float32x4_t v_s, v_c;
	uint32x4_t v_y = {0,0,0,0};
	v_s = vld1q_f32(s);
	v_c = vld1q_f32(c);
	
	switch (type) {
	case FLOAT_EQUAL:
		v_y = vceqq_f32(v_s, v_c); 
		break;
	case FLOAT_GREATER:
		v_y = vcgtq_f32(v_s, v_c);
		break;
	case FLOAT_LESS:
		v_y = vcltq_f32(v_s, v_c);
		break;

	default:
		break;
	}

	vst1q_u32(y, v_y);
	
	return;
}
EXPORT_SYMBOL(affine_neon_array_comparison);

void affine_neon_float_to_uint(float *s, UINT32 *y)
{
	float32x4_t v_s;
	uint32x4_t v_y;

	v_s = vld1q_f32(s);
	
	v_y = vcvtq_u32_f32(v_s);

	vst1q_u32(y, v_y);
	
	return;
}
EXPORT_SYMBOL(affine_neon_float_to_uint);

void affine_neon_uint_to_float(UINT32 *s, float *y)
{
	uint32x4_t v_s;
	float32x4_t v_y;

	v_s = vld1q_u32(s);
	
	v_y = vcvtq_f32_u32(v_s);

	vst1q_f32(y, v_y);
	
	return;
}
EXPORT_SYMBOL(affine_neon_uint_to_float);

void affine_neon_float_to_int(float *s, INT32 *y)
{
	float32x4_t v_s;
	int32x4_t v_y;

	v_s = vld1q_f32(s);
	
	v_y = vcvtq_s32_f32(v_s);

	vst1q_s32(y, v_y);
	
	return;
}
EXPORT_SYMBOL(affine_neon_float_to_int);

void affine_neon_int_to_float(INT32 *s, float *y)
{
	int32x4_t v_s;
	float32x4_t v_y;

	v_s = vld1q_s32(s);
	
	v_y = vcvtq_f32_s32(v_s);

	vst1q_f32(y, v_y);
	
	return;
}
EXPORT_SYMBOL(affine_neon_int_to_float);


static void __exit affine_neon_module_exit(void)
{
	pr_info("%s: exit end\n", __func__);
}

module_init(affine_neon_module_initial);
module_exit(affine_neon_module_exit);

MODULE_AUTHOR("Novatek Corp.");
MODULE_LICENSE("GPL");
