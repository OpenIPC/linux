#ifndef _IQ_BUILTIN_H_
#define _IQ_BUILTIN_H_

#include "kwrap/type.h"
#include "siep_lib.h"
#include "isp_builtin.h"
#include "iq_param.h"

//=============================================================================
// version
//=============================================================================
#define IQ_BUILTIN_VERSION 0x01000500

//=============================================================================
// define
//=============================================================================
#define SIE_BASE_ADDR  0xF0C00000
#define SIE2_BASE_ADDR 0xF0D20000
#define SIE3_BASE_ADDR 0xF0D30000
#define SIE4_BASE_ADDR 0xF0D40000
#define SIE5_BASE_ADDR 0xF0D80000
#define IFE_BASE_ADDR  0xF0C70000
#define DCE_BASE_ADDR  0xF0C20000
#define IPE_BASE_ADDR  0xF0C30000
#define IME_BASE_ADDR  0xF0C40000
#define SIE_SIZE       0x8B0
#define IFE_SIZE       0x754
#define DCE_SIZE       0x624
#define IPE_SIZE       0x8B0
#define IME_SIZE       0xAC0

#define GAMMA_BUFFER_SIZE 208*4  // 208, 64 aligned
#define YCURVE_BUFFER_SIZE 80*4  // 80, 64 aligned
#define LUT2D_BUFFER_SIZE 4288*4 // 65*65, 64 aligned
#define LUT2D_TABLE_SIZE  65*65
#define DPC_BUFFER_SIZE   4096*4  // 4095, 64 aligned
#define DPC_TABLE_SIZE    4096  // 4095, 64 aligned
#define ECS_BUFFER_SIZE   65*65*4  // 65*65, 64 aligned
#define ECS_TABLE_SIZE    65*65

#define IQ_EDGE_GAMMA_GAMMA_LEN 65

#define TMNR_PRE_FILTER_STRENGTH_TAB       4
#define TMNR_NR_LUMA_THRESHOLD_TAB         4
#define TMNR_NR_LUMA_LUT_TAB               8

#define IQ_CLAMP(in, lb, ub) (((INT32)(in) <= (INT32)(lb)) ? (INT32)(lb) : (((INT32)(in) >= (INT32)(ub)) ? (INT32)(ub) : (INT32)(in)))

//=============================================================================
// struct & enum definition
//=============================================================================
typedef enum _FRONT_FACTOR_ {
	FRONT_FACTOR_VALUE = 0,
	FRONT_FACTOR_START,
	FRONT_FACTOR_END,
	FRONT_FACTOR_NUM
} FRONT_FACTOR;

typedef struct _IQ_BUILTIN_FRONT_FACTOR_ {
	UINT32 smooth_thr;
	UINT32 rth_nlm;
	UINT32 rth_nlm_lut;
	UINT32 clamp_th;
	UINT32 c_con;
	UINT32 overshoot;
} IQ_BUILTIN_FRONT_FACTOR;

typedef struct _IQ_BUILTIN_FRONT_DTSI_ {
	UINT32 smooth_thr;
	UINT32 iso_w[IQ_GAIN_ID_MAX_NUM];
	UINT32 rth_nlm;
	UINT32 rth_nlm_lut;
	UINT32 clamp_th;
	UINT32 c_con;
	UINT32 overshoot;
} IQ_BUILTIN_FRONT_DTSI;

typedef enum _BUILTIN_IQ_ITEM {
	BUILTIN_IQ_ITEM_ENC_PARAM,
	BUILTIN_IQ_ITEM_MAX,
	ENUM_DUMMY4WORD(BUILTIN_IQ_ITEM)
} BUILTIN_IQ_ITEM;

typedef struct _ISOMAP {
	unsigned int  iso;
	unsigned int  index;
} ISOMAP;

//=============================================================================
// extern functions
//=============================================================================
extern BOOL iq_builtin_get_direct_mode(void);
extern void iq_builtin_get_histo(UINT32 id, ISP_BUILTIN_HISTO_RSLT *histo_rslt);
extern void iq_builtin_get_win_info(UINT32 id, SIE_CA_WIN_INFO *sie_ca_win_info, SIE_LA_WIN_INFO *sie_la_win_info);
extern void iq_builtin_set_ca_crop(UINT32 id, BOOL enable);
extern void iq_builtin_get_param(UINT32 id, UINT32 *param);
extern INT32 iq_builtin_init(UINT32 id);
extern INT32 iq_builtin_trig(UINT32 id, ISP_BUILTIN_IQ_TRIG_MSG msg);

#endif

