/**
	@brief Header file of debug definition of vendor net flow sample.

	@file kflow_dsp_dbg.h

	@ingroup net_flow_sample

	@note Nothing.

	Copyright Novatek Microelectronics Corp. 2018.  All rights reserved.
*/
#ifndef _KFLOW_DSP_DBG_H_
#define _KFLOW_DSP_DBG_H_

#define THIS_DBGLVL         NVT_DBG_WRN
#define __MODULE__          kflow_dsp
#define __DBGLVL__          8 // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
#define __DBGFLT__          "*" // *=All, [mark]=CustomClass
#if defined (__UITRON)
#include "DebugModule.h"
#else
#include "kwrap/debug.h"
#endif
extern unsigned int kflow_dsp_debug_level;

#endif  /* _KFLOW_DSP_DBG_H_ */
