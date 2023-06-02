#ifndef __MODULE_DBG_H_
#define __MODULE_DBG_H_

#define __MODULE__	ctl_ise
#define __DBGLVL__	8 // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
#include "kwrap/debug.h"
#include "kwrap/perf.h"
#include "comm/hwclock.h"

extern unsigned int ctl_ise_debug_level;

#endif



