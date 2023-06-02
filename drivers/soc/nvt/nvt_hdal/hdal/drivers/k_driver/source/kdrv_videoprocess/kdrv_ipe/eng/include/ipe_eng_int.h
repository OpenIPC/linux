

#ifndef _IPE_ENG_INT_H_
#define _IPE_ENG_INT_H_

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


#include "kwrap/type.h"


#include "ipe_eng_int_reg.h"




//#define DEFAULT_HN      672//336//528 //88
//#define IPE_MAXHSTRP    (DEFAULT_HN)//336 //528 //88
//#define IPE_MINHSTRP    4  // 660 can be 4

#define IPE_MAXVSTRP    8191
#define IPE_MINVSTRP    16
#define IPE_VLINE    8
#define IPE_ENG_STRIP_MAX_SIZE  (2688)



#define IPE_ALIGN_FLOOR(value, base)  ((value) & ~((base)-1))                   ///< Align Floor
#define IPE_ALIGN_ROUND(value, base)  IPE_ALIGN_FLOOR((value) + ((base)/2), base)   ///< Align Round
#define IPE_ALIGN_CEIL(value, base)   IPE_ALIGN_FLOOR((value) + ((base)-1), base)   ///< Align Ceil

#define IPE_ALIGN_ROUND_64(a)       IPE_ALIGN_ROUND(a, 64)  ///< Round Off to 64
#define IPE_ALIGN_ROUND_32(a)       IPE_ALIGN_ROUND(a, 32)  ///< Round Off to 32
#define IPE_ALIGN_ROUND_16(a)       IPE_ALIGN_ROUND(a, 16)  ///< Round Off to 16
#define IPE_ALIGN_ROUND_8(a)        IPE_ALIGN_ROUND(a, 8)   ///< Round Off to 8
#define IPE_ALIGN_ROUND_4(a)        IPE_ALIGN_ROUND(a, 4)   ///< Round Off to 4

#define IPE_ALIGN_CEIL_64(a)        IPE_ALIGN_CEIL(a, 64)   ///< Round Up to 64
#define IPE_ALIGN_CEIL_32(a)        IPE_ALIGN_CEIL(a, 32)   ///< Round Up to 32
#define IPE_ALIGN_CEIL_16(a)        IPE_ALIGN_CEIL(a, 16)   ///< Round Up to 16
#define IPE_ALIGN_CEIL_8(a)         IPE_ALIGN_CEIL(a, 8)    ///< Round Up to 8
#define IPE_ALIGN_CEIL_4(a)         IPE_ALIGN_CEIL(a, 4)    ///< Round Up to 4

#define IPE_ALIGN_FLOOR_64(a)       IPE_ALIGN_FLOOR(a, 64)  ///< Round down to 64
#define IPE_ALIGN_FLOOR_32(a)       IPE_ALIGN_FLOOR(a, 32)  ///< Round down to 32
#define IPE_ALIGN_FLOOR_16(a)       IPE_ALIGN_FLOOR(a, 16)  ///< Round down to 16
#define IPE_ALIGN_FLOOR_8(a)        IPE_ALIGN_FLOOR(a, 8)   ///< Round down to 8
#define IPE_ALIGN_FLOOR_4(a)        IPE_ALIGN_FLOOR(a, 4)   ///< Round down to 4



typedef enum {
	IPE_STATUS_IDLE,
	IPE_STATUS_READY,
	IPE_STATUS_RUN,
	IPE_STATUS_PAUSE
} IPE_STATUS;

typedef enum {
	IPE_OP_OPEN,
	IPE_OP_CLOSE,
	IPE_OP_SETMODE,
	IPE_OP_START,
	IPE_OP_PAUSE,
	IPE_OP_HWRESET,
	IPE_OP_CHGSIZE,
	IPE_OP_CHGPARAM,
	IPE_OP_CHGINOUT,
	IPE_OP_LOADLUT,
	IPE_OP_READLUT,
	IPE_OP_CHGPARAMALL
} IPE_OPERATION;

/*
    IPE update state machine
*/
typedef enum {
	NOTUPDATE   = 0,
	UPDATE      = 1,
	ENUM_DUMMY4WORD(IPESTATUSUPDATE)
} IPESTATUSUPDATE;




extern volatile NT98560_IPE_REGISTER_STRUCT *ipeg;
extern volatile UINT8 *p_ipe_reg_chg_flag;

extern UINT16 ipe_int_to_2comp(INT16 val, UINT16 bits);
extern INT16 ipe_2comp_to_int(UINT16 val, UINT16 bits);


#ifdef __cplusplus
} // extern "C"
#endif


#endif

