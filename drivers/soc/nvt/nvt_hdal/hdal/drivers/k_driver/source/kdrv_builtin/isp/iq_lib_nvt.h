#ifndef _IQ_LIB_NVT_H_
#define _IQ_LIB_NVT_H_

#include "kwrap/type.h"

//=============================================================================
// struct & enum definition
//=============================================================================
#define IQ_MAX(a, b)         (((INT32)(a) > (INT32)(b)) ? (INT32)(a) : (INT32)(b))
#define IQ_MIN(a, b)         (((INT32)(a) < (INT32)(b)) ? (INT32)(a) : (INT32)(b))
#define IQ_CLAMP(in, lb, ub) (((INT32)(in) <= (INT32)(lb)) ? (INT32)(lb) : (((INT32)(in) >= (INT32)(ub)) ? (INT32)(ub) : (INT32)(in)))

//=============================================================================
// extern functions
//=============================================================================
extern UINT32 iq_lib_calc_wdr_nvt(UINT32 id);

#endif

