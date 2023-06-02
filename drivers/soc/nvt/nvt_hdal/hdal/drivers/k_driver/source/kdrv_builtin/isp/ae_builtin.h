#ifndef _AE_BUILTIN_H_
#define _AE_BUILTIN_H_

#include "kwrap/type.h"
#include "isp_builtin.h"


//=============================================================================
// struct & enum definition
//=============================================================================
typedef struct _AE_BUILTIN_INFO {
	UINT32 expt_min, expt_max;
	UINT32 gain_min, gain_max;
	UINT32 isp_gain_th;
	UINT64 total_gain_min, total_gain_max;
} AE_BUILTIN_INFO;

typedef struct _AE_BUILTIN_SHDR {
	UINT32 enable;
	UINT32 le_id;
	UINT32 se_id;
	UINT32 frm_num;
	UINT32 mask;
	UINT32 ev_ratio;
	UINT32 expt_max;
} AE_BUILTIN_SHDR;

typedef struct _AE_BUILTIN_FRONT_DTSI_ {
	UINT32 enable;
	UINT32 sen_expt_min;
	UINT32 sen_expt_max;
} AE_BUILTIN_FRONT_DTSI;

/*
typedef enum _AE_ID {
	AE_ID_1 = 0,                    ///< ae id 1
	AE_ID_2,                        ///< ae id 2
	AE_ID_3,                        ///< ae id 3
	AE_ID_4,                        ///< ae id 4
	AE_ID_5,                        ///< ae id 5
	AE_ID_MAX_NUM,
	ENUM_DUMMY4WORD(AE_ID)
} AE_ID;
*/
//=============================================================================
// extern functions
//=============================================================================
extern INT32 ae_builtin_init(UINT32 id);
extern INT32 ae_builtin_trig(UINT32 id, ISP_BUILTIN_AE_TRIG_MSG msg);

#endif
