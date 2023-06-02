#ifndef _NVT_HDAL_NEON_TEST_
#define _NVT_HDAL_NEON_TEST_

#include <kwrap/type.h>
#include <asm/neon.h>


typedef enum {
	FLOAT_ADD,
	FLOAT_SUB,
	FLOAT_MUL,
	FLOAT_MAX,
	FLOAT_MIN,

	ENUM_DUMMY4WORD(FLOAT_OPERATION_TYPE)
} FLOAT_OPERATION_TYPE;

typedef enum {
	FLOAT_EQUAL,
	FLOAT_GREATER,
	FLOAT_LESS,

	ENUM_DUMMY4WORD(FLOAT_COMPARISON_TYPE)
} FLOAT_COMPARISON_TYPE;


void affine_neon_array_operation(FLOAT_OPERATION_TYPE type, float *s, float *c, float *y);
void affine_neon_array_comparison(FLOAT_COMPARISON_TYPE type, float *s, float *c, UINT32 *y);
void affine_neon_float_to_uint(float *s, UINT32 *y);
void affine_neon_uint_to_float(UINT32 *s, float *y);
void affine_neon_float_to_int(float *s, INT32 *y);
void affine_neon_int_to_float(INT32 *s, float *y);



#endif /* _NVT_HDAL_NEON_TEST_ */
