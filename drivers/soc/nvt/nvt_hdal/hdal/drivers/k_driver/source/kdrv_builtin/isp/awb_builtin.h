#ifndef _AWB_BUILTIN_H_
#define _AWB_BUILTIN_H_

#include "kwrap/type.h"
#include "isp_builtin.h"

//=============================================================================
// struct & enum definition
//=============================================================================
#define ISP_AWB_CT_NUM              6

/**
	 AWB th.

	 @note
*/
typedef struct _AWB_BUILTIN_TH {
	INT32 y_l;
	INT32 y_u;
	INT32 r2g_l;
	INT32 r2g_u;
	INT32 b2g_l;
	INT32 b2g_u;
} AWB_BUILTIN_TH;

/**
	AWB CT table
*/
typedef struct _AWB_BUILTIN_CT_INFO {
	UINT32 temperature[ISP_AWB_CT_NUM];
	UINT32 r_gain[ISP_AWB_CT_NUM];
	UINT32 g_gain[ISP_AWB_CT_NUM];
	UINT32 b_gain[ISP_AWB_CT_NUM];
} AWB_BUILTIN_CT_INFO;

/**
	AWB Status
*/
typedef enum _AWB_BUILTIN_STS {
	AWBALG_STATUS_INIT          = 0,
	AWBALG_STATUS_FREEZE,
	AWBALG_STATUS_CONVERGE,
	AWBALG_STATUS_END           = 0xffffffff,
	ENUM_DUMMY4WORD(AWB_STS)
} AWB_BUILTIN_STS;

/**
	AWB info
*/
typedef struct _AWB_BUILTIN_INFO {
	ISP_BUILTIN_CGAIN awb_gain;
	UINT32 cur_ct;
	AWB_BUILTIN_STS status;
	UINT32 awb_app_mode;
} AWB_BUILTIN_INFO;

typedef struct _AWB_BUILTIN_FRONT_DTSI_ {
	UINT32 enable;
	ISP_BUILTIN_CGAIN color_gain;
} AWB_BUILTIN_FRONT_DTSI;

//=============================================================================
// extern functions
//=============================================================================
extern INT32 awb_builtin_init(UINT32 id);
extern INT32 awb_builtin_trig(UINT32 id, ISP_BUILTIN_AWB_TRIG_MSG msg);

#endif

