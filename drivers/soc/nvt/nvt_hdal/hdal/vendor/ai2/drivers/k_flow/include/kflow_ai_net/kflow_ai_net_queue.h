/**
	@brief Source file of vendor net flow sample.

	@file net_flow_sample.c

	@ingroup net_flow_sample

	@note Nothing.

	Copyright Novatek Microelectronics Corp. 2018.  All rights reserved.
*/

/*-----------------------------------------------------------------------------*/
/* Include Files                                                               */
/*-----------------------------------------------------------------------------*/
#include "kwrap/type.h"
#include "kwrap/error_no.h"

/*-----------------------------------------------------------------------------*/
/* Macro Constant Definitions                                                  */
/*-----------------------------------------------------------------------------*/
/*-----------------------------------------------------------------------------*/
/* Local Global Variables                                                      */
/*-----------------------------------------------------------------------------*/
/*-----------------------------------------------------------------------------*/
/* Local Functions                                                             */
/*-----------------------------------------------------------------------------*/

typedef struct _AI_PROC_QUEUE {
	UINT32 		sign; ///< signature
	UINT32		proc_id;
	UINT32		en;
	UINT32		data;
	void*     	p_sem;
	void*     	p_sem_q;
} AI_PROC_QUEUE, *PAI_PROC_QUEUE;

extern ER nvt_ai_proc_queue_open(PAI_PROC_QUEUE p_queue, UINT32 proc_id, void* p_sem, void* p_sem_q);
extern ER nvt_ai_proc_queue_put(PAI_PROC_QUEUE p_queue, UINT32 data);
extern ER nvt_ai_proc_queue_cancel(PAI_PROC_QUEUE p_queue);
extern ER nvt_ai_proc_queue_get(PAI_PROC_QUEUE p_queue, UINT32* p_data, INT32 wait_ms);
extern ER nvt_ai_proc_queue_close(PAI_PROC_QUEUE p_queue);

