#ifndef _AE_COMMON_H_
#define _AE_COMMON_H_

#include "kwrap/type.h"
#include "isp_builtin.h"

#define AE_WIN_X		8
#define AE_WIN_Y		8
#define AE_WIN_NUM		(AE_WIN_X * AE_WIN_Y)

#define LA_WIN_X		32
#define LA_WIN_Y		32
#define LA_WIN_NUM		(LA_WIN_X * LA_WIN_Y)

#define AE_MIN(a, b)                    ((a < b) ? a : b)
#define AE_MAX(a, b)                    ((a > b) ? a : b)
#define AE_ABS(x, y)                    ((x > y) ? (x - y) : (y - x))
#define AE_CLAMP(in, lb, ub)            (((in) <= (lb)) ? (lb) : (((in) >= (ub)) ? (ub) : (in)))
#define AE_OP_BIT(in, src_bit, des_bit) ((in * ((1 << des_bit) - 1) + (1 <<  (src_bit - 1))) / ((1 << src_bit) - 1))

#define AE_PARAM_BIT                    8
#define AE_GAMMA_BIT                    10
#define AE_RAW_BIT                      12
#define AE_Y8_MAX                       ((1 << AE_PARAM_BIT) - 1)
#define AE_Y10_MAX                      ((1 << AE_GAMMA_BIT) - 1)
#define AE_RAW_MAX                      ((1 << AE_RAW_BIT) - 1)

#define AEALG_LV_ACCURACY_BASE      1000000
#define AEALG_EV_ACCURACY_BASE      AEALG_LV_ACCURACY_BASE

//=============================================================================
// struct & enum definition
//=============================================================================

//=============================================================================
// extern functions
//=============================================================================
extern UINT64 ae_comm_uint64_div(UINT64 numerator, UINT64 denominator);
extern UINT32 ae_comm_op_log2(UINT64 Devider, UINT64 Devident);
extern UINT64 ae_comm_totalgain2ev(UINT64 total_gain, UINT32 iso_coef);
extern UINT32 ae_comm_ev2lv(UINT64 EVValue);
extern UINT64 ae_comm_cal_ev(UINT64 exptime, UINT64 isogain, UINT32 iso_coef);

#endif
