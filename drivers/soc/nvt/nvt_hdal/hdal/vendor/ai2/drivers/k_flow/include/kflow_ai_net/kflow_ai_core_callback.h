/**
	@brief Source file of kflow_ai_net.

	@file kflow_ai_core.h

	@ingroup kflow_ai_net

	@note Nothing.

	Copyright Novatek Microelectronics Corp. 2018.  All rights reserved.
*/

/*-----------------------------------------------------------------------------*/
/* Include Files                                                               */
/*-----------------------------------------------------------------------------*/

#ifndef _KFLOW_AI_NET_CORE_CALLBACK_H_
#define _KFLOW_AI_NET_CORE_CALLBACK_H_

#include "kwrap/type.h"
#include "kwrap/platform.h"
#include "kwrap/task.h"
#include "kwrap/spinlock.h"
#include "kwrap/semaphore.h"
#include "kwrap/flag.h"
#include "kwrap/error_no.h"

#include "kflow_ai_net/kflow_ai_core.h"

extern int kflow_ai_core_cb_init(UINT32 proc_id);
extern int kflow_ai_core_cb_exit(UINT32 proc_id);
extern void kflow_ai_job_cb(UINT32 proc_id, KFLOW_AI_JOB* p_job);
extern KFLOW_AI_JOB* kflow_ai_core_cb_wait(UINT32 proc_id);
extern void kflow_ai_core_cb_sig(UINT32 proc_id, KFLOW_AI_JOB* p_job);
extern int kflow_ai_core_cb_init_cb(VOID);
extern int kflow_ai_core_cb_uninit_cb(VOID);

#endif //_KFLOW_AI_NET_CORE_CALLBACK_H_
