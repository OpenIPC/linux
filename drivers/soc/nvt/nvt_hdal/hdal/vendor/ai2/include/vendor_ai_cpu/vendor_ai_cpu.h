/**
	@brief Header file of extend engine CPU.

	@file vendor_ai_cpu.h

	@ingroup vendor_ai_cpu

	@note Nothing.

	Copyright Novatek Microelectronics Corp. 2019.  All rights reserved.
*/
#ifndef _VENDOR_AI_CPU_H_
#define _VENDOR_AI_CPU_H_

#include "vendor_ai_plugin.h"
#include "vendor_ai_net/nn_net.h"

/********************************************************************
	EXTERN VARIABLES & FUNCTION PROTOTYPES DECLARATIONS
********************************************************************/
extern void* vendor_ai_cpu1_get_engine(void);

extern HD_RESULT vendor_ai_cpu_thread_init(UINT32 proc_id);
extern HD_RESULT vendor_ai_cpu_thread_exit(UINT32 proc_id);
extern HD_RESULT vendor_ai_cpu_thread_reg_cb(VENDOR_AI_ENG_CB fp);
extern HD_RESULT vendor_ai_cpu_thread_init_task(VOID);
extern HD_RESULT vendor_ai_cpu_thread_uninit_task(VOID);

#endif  /* _VENDOR_AI_CPU_H_ */
