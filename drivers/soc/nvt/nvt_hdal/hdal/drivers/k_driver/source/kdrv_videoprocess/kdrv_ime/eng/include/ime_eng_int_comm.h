#ifndef _IME_ENG_INT_COMM_H_
#define _IME_ENG_INT_COMM_H_

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



#define IME_GET_API_EN    0


#define IME_ALIGN_FLOOR(value, base)  ((value) & ~((base)-1))                   ///< Align Floor
#define IME_ALIGN_ROUND(value, base)  IME_ALIGN_FLOOR((value) + ((base)/2), base)   ///< Align Round
#define IME_ALIGN_CEIL(value, base)   IME_ALIGN_FLOOR((value) + ((base)-1), base)   ///< Align Ceil

#define IME_ALIGN_ROUND_64(a)       IME_ALIGN_ROUND(a, 64)  ///< Round Off to 64
#define IME_ALIGN_ROUND_32(a)       IME_ALIGN_ROUND(a, 32)  ///< Round Off to 32
#define IME_ALIGN_ROUND_16(a)       IME_ALIGN_ROUND(a, 16)  ///< Round Off to 16
#define IME_ALIGN_ROUND_8(a)        IME_ALIGN_ROUND(a, 8)   ///< Round Off to 8
#define IME_ALIGN_ROUND_4(a)        IME_ALIGN_ROUND(a, 4)   ///< Round Off to 4

#define IME_ALIGN_CEIL_64(a)        IME_ALIGN_CEIL(a, 64)   ///< Round Up to 64
#define IME_ALIGN_CEIL_32(a)        IME_ALIGN_CEIL(a, 32)   ///< Round Up to 32
#define IME_ALIGN_CEIL_16(a)        IME_ALIGN_CEIL(a, 16)   ///< Round Up to 16
#define IME_ALIGN_CEIL_8(a)         IME_ALIGN_CEIL(a, 8)    ///< Round Up to 8
#define IME_ALIGN_CEIL_4(a)         IME_ALIGN_CEIL(a, 4)    ///< Round Up to 4

#define IME_ALIGN_FLOOR_64(a)       IME_ALIGN_FLOOR(a, 64)  ///< Round down to 64
#define IME_ALIGN_FLOOR_32(a)       IME_ALIGN_FLOOR(a, 32)  ///< Round down to 32
#define IME_ALIGN_FLOOR_16(a)       IME_ALIGN_FLOOR(a, 16)  ///< Round down to 16
#define IME_ALIGN_FLOOR_8(a)        IME_ALIGN_FLOOR(a, 8)   ///< Round down to 8
#define IME_ALIGN_FLOOR_4(a)        IME_ALIGN_FLOOR(a, 4)   ///< Round down to 4



/**
    IME enum - function enalbe/disable
*/
typedef enum _IME_FUNC_EN {
	IME_FUNC_DISABLE = 0,   ///< function disable
	IME_FUNC_ENABLE  = 1,  ///< function enable
	ENUM_DUMMY4WORD(IME_FUNC_EN)
} IME_FUNC_EN;




#ifdef __cplusplus
}
#endif

#endif

