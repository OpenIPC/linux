
#ifndef _IFE_INT_H_
#define _IFE_INT_H_

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __KERNEL__
#include "kwrap/type.h"
#include <mach/rcw_macro.h>
#include "kwrap/error_no.h"
#elif defined(__FREERTOS)
#include "kwrap/type.h"
#include "rcw_macro.h"
#include "kwrap/error_no.h"
#else
#endif

#include "ife_eng_int_reg.h"


extern volatile NT98560_IFE_REG_STRUCT *ifeg;
extern volatile UINT8 *p_ife_reg_chg_flag;


#ifdef __cplusplus
} // extern "C"
#endif

#endif  // #ifndef _IFE2_INT_H_


