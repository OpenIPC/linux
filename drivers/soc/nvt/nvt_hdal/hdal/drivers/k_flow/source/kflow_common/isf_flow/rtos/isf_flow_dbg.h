#ifndef __ISF_FLOW_DBG_H_
#define __ISF_FLOW_DBG_H_

#define THIS_DBGLVL         NVT_DBG_WRN
#define __MODULE__          isf_flow
#define __DBGLVL__          8 // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
#define __DBGFLT__          "*" // *=All, [mark]=CustomClass
#if defined (__UITRON) || defined(__ECOS)  || defined (__FREERTOS)
#include "kwrap/debug.h"
#else
#include "kwrap/debug.h"
#endif
extern unsigned int isf_flow_debug_level;

#endif

