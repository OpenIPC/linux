/**
	@brief Header file of debug definition of vendor net flow sample.

	@file net_flow_sample.h

	@ingroup net_flow_sample

	@note Nothing.

	Copyright Novatek Microelectronics Corp. 2018.  All rights reserved.
*/
#ifndef _KFLOW_AI_NET_DBG_H_
#define _KFLOW_AI_NET_DBG_H_

#define THIS_DBGLVL         NVT_DBG_WRN
#define __MODULE__          kflow_ai_net
#define __DBGLVL__          8 // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
#define __DBGFLT__          "*" // *=All, [mark]=CustomClass
#if defined (__UITRON)
#include "DebugModule.h"
#else
#include "kwrap/debug.h"
#endif
extern unsigned int kflow_ai_net_debug_level;


#define PROF                DISABLE
#if PROF
	static struct timeval tstart, tend;
	#define PROF_START()    do_gettimeofday(&tstart);
	#define PROF_END(msg)   do_gettimeofday(&tend);     \
			printk("%s time (us): %lu\r\n", msg,        \
					(tend.tv_sec - tstart.tv_sec) * 1000000 + (tend.tv_usec - tstart.tv_usec));
#else
	#define PROF_START()
	#define PROF_END(msg)
#endif


#endif  /* _KFLOW_AI_NET_DBG_H_ */
