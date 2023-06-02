/**
	@brief Header file of definition of kflow ai builtin api.

	@file kflow_ai_builtin_api.h

	@ingroup kflow_ai_builtin

	@note Nothing.

	Copyright Novatek Microelectronics Corp. 2020.  All rights reserved.
*/
#ifndef _KFLOW_AI_BUILTIN_API_H_
#define _KFLOW_AI_BUILTIN_API_H_

/********************************************************************
	INCLUDE FILES
********************************************************************/
#ifdef __KERNEL__
#include <linux/ioctl.h>
#endif
#include "kwrap/ioctl.h"
#include "kdrv_ai/kdrv_ai.h"
#include "kdrv_ipp_builtin.h"
#include "pdcnn_api.h"

typedef struct _KFLOW_AI_BUILTIN_MEM_PARM {
	UINT32 pa;
	UINT32 va;
	UINT32 size;
} KFLOW_AI_BUILTIN_MEM_PARM;

typedef struct _KFLOW_AI_BUILTIN_MAP_MEM_PARM {
	KFLOW_AI_BUILTIN_MEM_PARM kerl_parm;
	KFLOW_AI_BUILTIN_MEM_PARM user_model;
	KFLOW_AI_BUILTIN_MEM_PARM pd_buff;
	KFLOW_AI_BUILTIN_MEM_PARM rslt_buff;
	KFLOW_AI_BUILTIN_MEM_PARM io_buff;
} KFLOW_AI_BUILTIN_MAP_MEM_PARM;

typedef struct {
	INT32 out_num;
	PD_MEM_PARM out_result;
	UINT32 timestamp;
} KFLOW_AI_BUILTIN_RSLT_INFO, *PVDOENC_BUILTIN_RSLT_INFO;

typedef struct {
	/* ai internal buffer info */
	UINT32 blk_addr;
	UINT32 blk_size;
} KFLOW_AI_BUILTIN_INIT_INFO;

extern ER kflow_ai_builtin_init(KFLOW_AI_BUILTIN_INIT_INFO *info);
extern ER kflow_ai_builtin_close(void);
extern ER kflow_ai_builtin_exit(void);
extern ER kflow_ai_builtin_proc_net(KFLOW_AI_BUILTIN_MAP_MEM_PARM *p_mem, UINT32 net_id);
extern BOOL kflow_ai_builtin_get_rslt(UINT32 proc_id, KFLOW_AI_BUILTIN_RSLT_INFO *builtin_rslt_info);
extern UINT32 kflow_ai_builtin_how_many_in_rsltq(UINT32 proc_id);
extern void kflow_ai_builtin_free_rslt_que_mem(UINT32 proc_id);
extern ER kflow_ai_builtin_get_builtin_mem(KFLOW_AI_BUILTIN_MAP_MEM_PARM *p_mem);

#endif  /* _KFLOW_AI_BUILTIN_API_H_ */
