#ifndef __SIE_DBG_H_
#define __SIE_DBG_H_

#include "kwrap/debug.h"

#define KDRV_SIE_E_OK  			(0)
#define KDRV_SIE_E_ID 			(-100)	//illegal sie handle
#define KDRV_SIE_E_SYS 			(-200)	//sysem error
#define KDRV_SIE_E_HDL 			(-300)	//null handle
#define KDRV_SIE_E_NULL_FP 		(-400)	//null fp
#define KDRV_SIE_E_PAR 			(-500)	//parameter error
#define KDRV_SIE_E_NOSPT 		(-600)	//no support function
#define KDRV_SIE_E_NODEV 		(-700)	//no device found
#define KDRV_SIE_E_NOMEM 		(-800)	//no memory
#define KDRV_SIE_E_STATE 		(-900)	//illegal state

typedef enum _KDRV_SIE_DBG_LVL {
	KDRV_SIE_DBG_LVL_NONE = 0,
	KDRV_SIE_DBG_LVL_ERR,
	KDRV_SIE_DBG_LVL_WRN,
	KDRV_SIE_DBG_LVL_IND,
	KDRV_SIE_DBG_LVL_TRC,
	KDRV_SIE_DBG_LVL_MAX,
	ENUM_DUMMY4WORD(KDRV_SIE_DBG_LVL)
} KDRV_SIE_DBG_LVL;

extern KDRV_SIE_DBG_LVL kdrv_sie_dbg_rt_lvl;
#define kdrv_sie_dbg_dump(fmt, args...)   DBG_DUMP(fmt, ##args);
#define kdrv_sie_dbg_err(fmt, args...)    { if (kdrv_sie_dbg_rt_lvl >= KDRV_SIE_DBG_LVL_ERR) { DBG_ERR(fmt, ##args);  }}
#define kdrv_sie_dbg_wrn(fmt, args...)    { if (kdrv_sie_dbg_rt_lvl >= KDRV_SIE_DBG_LVL_WRN) { DBG_WRN(fmt, ##args);  }}
#define kdrv_sie_dbg_ind(fmt, args...)    { if (kdrv_sie_dbg_rt_lvl >= KDRV_SIE_DBG_LVL_IND) { DBG_DUMP(fmt, ##args);  }}
#define kdrv_sie_dbg_trc(fmt, args...)    { if (kdrv_sie_dbg_rt_lvl >= KDRV_SIE_DBG_LVL_TRC) { DBG_DUMP(fmt, ##args);  }}

#endif
