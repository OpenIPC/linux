#ifndef __ISF_VDOCAP_DBG_H
#define __ISF_VDOCAP_DBG_H

#define THIS_DBGLVL         NVT_DBG_WRN
#define __MODULE__          isf_vdocap
#define __DBGLVL__          8 // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
#define __DBGFLT__          "*" // *=All, [mark]=CustomClass
#include "kwrap/debug.h"

#define VCAP_TRACE(opclass, fmtstr, args...) p_thisunit->p_base->do_trace(p_thisunit, opclass, fmtstr, ##args)

extern unsigned int isf_vdocap_debug_level;

#endif
