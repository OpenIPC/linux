#ifndef _AE_DBG_H_
#define _AE_DBG_H_

#define THIS_DBGLVL         6  //NVT_DBG_MSG
#define __MODULE__          ae
#define __DBGLVL__          THIS_DBGLVL
#define __DBGFLT__          "*" // *=All, [mark]=CustomClass

#include "kwrap/debug.h"

#include "ae_alg.h"

#define AE_DBG_NONE             0x00000000
#define AE_DBG_ERR_MSG          0x00000001
#define AE_DBG_SYNC				0x00000002
#define AE_DBG_CFG				0x00000004
#define AE_DBG_INFO				0x00000100
#define AE_DBG_OVEREXP			0x00000200
#define AE_DBG_HDR				0x00000400
#define AE_DBG_SMARTIR			0x00000800
#define AE_DBG_ROI				0x00001000
#define AE_DBG_LA_ROI			0x00002000
#define AE_DBG_BOUND	        0x00004000
#define AE_DBG_LOCK				0x00008000
#define AE_DBG_STATUS			0x00100000
#define AE_DBG_CAPTURE			0x01000000
#define AE_DBG_PERFORMANCE		0x10000000

typedef enum _AE_PROC_CMD_TYPE {
	AE_PROC_CMD_NONE = 0,
	AE_PROC_CMD_DBG,
	AE_PROC_CMD_MODE,
	AE_PROC_CMD_TARGET_Y,
	AE_PROC_CMD_ISOCOEF,
	AE_PROC_CMD_CONVERGE_TH,
	AE_PROC_CMD_SPEED,
	AE_PROC_CMD_GAIN_BOUND,
	AE_PROC_CMD_EXPT_BOUND,
	AE_PROC_CMD_MAE,
	AE_PROC_CMD_MTG,
	AE_PROC_CMD_POWER_FREQ,
	AE_PROC_CMD_FLICKER_MODE,
	AE_PROC_CMD_ALC,
	AE_PROC_CMD_OVEREXP,
	AE_PROC_CMD_DUMP_CURVE,
	AE_PROC_CMD_UI,
	AE_PROC_CMD_PRI_EXPT,
	AE_PROC_CMD_PRI_ISO,
	AE_PROC_CMD_PRI_APERTURE,
	AE_PROC_CMD_ROI,
	AE_PROC_CMD_ROI_WEIGHTING,
	AE_PROC_CMD_FREEZE,
	AE_PROC_CMD_SLOW_SHOOT,
	AE_PROC_CMD_PDCTRL,
	AE_PROC_CMD_RATE,
	AE_PROC_CMD_SMART_IR_WEIGHT,
	AE_PROC_CMD_PERF_CMD,
	AE_PROC_CMD_TEST,
	AE_PROC_CMD_CURVE_TRACKING,
	AE_PROC_CMD_NUM
} AE_PROC_CMD_TYPE;

typedef struct _AE_PROC_CMD_OBJ {
	AE_PROC_CMD_TYPE cmd;
	UINT32 dbg_cmd;
	UINT32 dbg_cyc;
	AE_MODE mode;
	UINT32 target_y;
	UINT32 isocoef;
	AE_BOUNDARY converge_th;
	UINT32 speed;
	AE_BOUNDARY gain_bound;
	AE_BOUNDARY expt_bound;
	UINT32 mae_expt;
	UINT32 mae_gain;
	UINT32 mae_aperture;
	UINT64 mae_totalgain;
	UINT32 power_freq;
	UINT32 flicker_mode;
	UINT32 alc_en;
	UINT32 overexp_en;
	UINT32 overexp_thr;
	UINT32 curve_type;
	UINT32 ui_cmd;
	UINT32 ui_value;
	UINT32 priority_mode;
	UINT32 priority_expt;
	UINT32 priority_iso;
	UINT32 priority_aperture;
	UINT32 roi_start_x;
	UINT32 roi_start_y;
	UINT32 roi_end_x;
	UINT32 roi_end_y;
	UINT32 roi_option;
	UINT32 roi_center_w;
	UINT32 roi_neighbor_w0;
	UINT32 roi_neighbor_w1;
	UINT32 freeze_en;
	UINT32 freeze_cyc;
	UINT32 freeze_thr;
	UINT32 slowshoot_en;
	UINT32 slowshoot_range;
	UINT32 slowshoot_thr;
	UINT32 p_factor[AEALG_SHOOT_MAXCNT];
	UINT32 d_factor[AEALG_SHOOT_MAXCNT];
	UINT32 rate;
	AE_SMART_IR_WEIGHT smart_ir_weight;
	UINT32 perf_cmd;
	UINT32 test;
	UINT32 ct_period;
	UINT32 ct_interval;
	AE_BOUNDARY ct_range; 
	UINT32 ct_tg_unit;
} AE_PROC_CMD_OBJ;

#define PRINT_AE(type, fmt, args...)  {if (type) DBG_DUMP(fmt, ## args); }
#define PRINT_AE_VAR(type, var)      {if (type) DBG_DUMP("%s %s = %d\r\n", __func__, #var, var); }
#define PRINT_AE_ARR(type, arr, len) {       \
	if (type) {                              \
		do {                                 \
			UINT32 i;                        \
			DBG_DUMP("%s %s = { ", __func__, #arr);    \
			for (i = 0; i < len; i++)        \
				DBG_DUMP("%d, ", arr[i]);    \
			DBG_DUMP("}\r\n");               \
		} while (0);                         \
	};                                       \
}

#define PRINT_AE_ERR(type, fmt, args...) {if (type) DBG_ERR(fmt, ## args); if (ae_dbg_check_err_msg(type)) DBG_ERR(fmt, ## args); }
#if defined(__KERNEL__)
#define PRINT_AE_INFO(sfile, fmt, args...) {seq_printf(sfile, fmt, ## args); }
#else
#define PRINT_AE_INFO(sfile, fmt, args...) {DBG_DUMP(fmt, ## args); }
#endif

extern AE_PROC_CMD_OBJ *ae_flow_get_proc_cmd(UINT32 id);

extern UINT32 ae_dbg_get_dbg_mode(AE_ID id);
extern void ae_dbg_set_dbg_mode(AE_ID id, UINT32 cmd);
extern void ae_dbg_clr_dbg_mode(AE_ID id, UINT32 cmd);
extern BOOL ae_dbg_check_err_msg(BOOL show_dbg_msg);
extern void ae_dbg_clr_err_msg(void);
extern UINT32 ae_dbg_get_err_msg(void);

#endif
