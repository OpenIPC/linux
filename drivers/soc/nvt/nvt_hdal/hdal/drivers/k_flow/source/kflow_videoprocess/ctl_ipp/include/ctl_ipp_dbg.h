#ifndef __MODULE_DBG_H_
#define __MODULE_DBG_H_

#define __MODULE__	ctl_ipp
#define __DBGLVL__	8 // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
#include "kwrap/debug.h"
#include "kwrap/perf.h"

/* for debug.h */
extern unsigned int ctl_ipp_debug_level;

/* err rate contorl */
extern UINT32 g_ctl_ipp_dbg_rate;

/**
	debug log
*/
#define CTL_IPP_DBG_ERR(fmt, args...)	DBG_ERR(fmt, ##args)
#define CTL_IPP_DBG_WRN(fmt, args...)	DBG_WRN(fmt, ##args)
#define CTL_IPP_DBG_IND(fmt, args...)	DBG_IND(fmt, ##args)
#define CTL_IPP_DBG_TRC(fmt, args...)	DBG_MSG(fmt, ##args)
#define CTL_IPP_DBG_DUMP(fmt, args...)	DBG_DUMP("%s:" fmt, __func__, ##args);

#define CTL_IPP_DBG_ERR_RATE(cnt, fmt, args...) {if ((cnt % g_ctl_ipp_dbg_rate) == 0) {DBG_ERR(fmt, ##args);}}

#endif


