#ifndef __ISF_FLOW_DBG_H_
#define __ISF_FLOW_DBG_H_

#define THIS_DBGLVL         NVT_DBG_WRN
#define __MODULE__          isf_flow
#define __DBGLVL__          THIS_DBGLVL
#define __DBGFLT__          "*" // *=All, [mark]=CustomClass
#if defined (__UITRON)
#include "DebugModule.h"
#else
#include "kwrap/debug.h"
#endif
extern unsigned int isf_flow_debug_level;

#endif

