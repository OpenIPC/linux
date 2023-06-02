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
#ifndef _KFLOW_AI_CORE_H_
#define _KFLOW_AI_CORE_H_

#include "kwrap/type.h"
#include "kwrap/error_no.h"

#include "kflow_ai_net/kflow_ai_net_list.h" //for LIST_HEAD
#include "kflow_ai_net/kflow_ai_core_task.h" //for KFLOW_CORE_TASK_FUNC


#define KFLOW_AI_DBG_CORE		0x00000000	//dump current jobs in wait queue, ready queue and run of all engines
#define KFLOW_AI_DBG_BIND		0x00000001	//dump net graph before net proc
#define KFLOW_AI_DBG_SCHD		0x00000002	//dump schedule log while proc each job
#define KFLOW_AI_DBG_CTX		0x00000004	//dump context after proc each job
#define KFLOW_AI_DBG_OBUF		0x00000008	//dump buffer after proc each job

#define KFLOW_AI_DBG_TIME		0x00000100	//dump time after proc
#define KFLOW_AI_DBG_TIMELINE	0x00000200 	//dump timeline after proc


typedef struct _KFLOW_AI_BIND KFLOW_AI_BIND;
typedef struct _KFLOW_AI_JOB KFLOW_AI_JOB;
typedef struct _KFLOW_AI_NET KFLOW_AI_NET;
typedef struct _KFLOW_AI_CHANNEL_CTX KFLOW_AI_CHANNEL_CTX;
typedef struct _KFLOW_AI_ENGINE_CTX KFLOW_AI_ENGINE_CTX;

typedef struct _KFLOW_AI_BIND {
	LIST_HEAD list; //belong to graph's freelist or job's parent-list or job's child-list
	struct _KFLOW_AI_JOB *p_job;
} KFLOW_AI_BIND;

typedef struct _KFLOW_AI_JOB {
	UINT32 proc_id;
	UINT32 job_id;
	//runtime state
	LIST_HEAD list;
	//graph input
	INT32 parent_cnt;
	LIST_HEAD parent_list;
	//graph output
	INT32 child_cnt;
	LIST_HEAD child_list;
	//graph node info
	void* p_op_info; // operation info
	void* p_io_info; // input/output info
	//runtime state
	UINT32 state; //0 = done, 1 = wait, 2 = ready, 3 = run;
	INT32 exec_cnt;
	INT32 wait_cnt; //after ready: current waiting resource (def: parent_cnt)
	UINT32 engine_op;
	UINT32 engine_id;
	UINT32 schd_parm; //for FIFO: (none), for FAIR: (channel_id), for CAPACITY: (QoS bandwidth)
	INT32 wait_ms; ///<-1: blocking, 0: non-blocking, >0: non-blocking + timeout
	struct _KFLOW_AI_ENGINE_CTX* p_eng; //assigned engine
	struct _KFLOW_AI_CHANNEL_CTX* p_ch; //assigned channel
	UINT32 ts_wait_begin; // hwclock_get_longcounter();
	UINT32 ts_wait_end;
	UINT32 ts_ready_begin;
	UINT32 ts_ready_end;
	UINT32 ts_exec_begin;
	UINT32 ts_exec_end;
	UINT32 ts_exec_predict;
	UINT32 ts_exec_actual;
	UINT32 debug_func;
	//runtime func
	KFLOW_CORE_TASK_FUNC deferred;
} KFLOW_AI_JOB;

typedef struct _KFLOW_AI_JOB_CMD {
	UINT32 proc_id;
	UINT32 job_id;
} KFLOW_AI_JOB_CMD;

typedef struct _KFLOW_AI_NET {
	UINT32 proc_id;
	UINT32 max_job_cnt;
	UINT32 job_cnt;
	UINT32* map;
	UINT32 map_id;
	struct _KFLOW_AI_JOB* job;
	LIST_HEAD free_job_list;
	UINT32 bind_cnt;
	struct _KFLOW_AI_BIND* bind;
	LIST_HEAD free_bind_list;
	UINT32 src_cnt;
	UINT32 dest_cnt;
	UINT32 total_cnt;
	UINT32 src_wait_cnt;
	UINT32 dest_wait_cnt;
	UINT32 exec_cnt;
	UINT32 isr_trig_cnt;
	UINT32 debug_func;
	UINT32 flow_mask;
	UINT32 proc_rec; //00:isr trigger, 01:task trigger
	UINT32 ts_proc_begin; // hwclock_get_longcounter();
	UINT32 ts_proc_end;
	UINT32 blk_id;
	UINT32 blk_sz;
	UINT32 pool;
	void* addr;
	INT32 rv; //0=ok, -1=FAILED, -2=TIMEOUT, -3=ABORT
} KFLOW_AI_NET;

typedef struct _KFLOW_AI_CHANNEL_CTX {
	char* name;
	LIST_HEAD* ready_queue;
	KFLOW_AI_JOB* run_job;
	UINT32 state;  //0: init, 1: open, 2: start, 3: error
	INT32 wait_ms; ///< last time
#if 1 //for LOAD_BALANCE
	UINT32 ts_ready_load;
	UINT32 ts_prev_end; //before idle, last job end
	UINT32 ts_this_begin; //after busy, first job begin
#endif
	void (*open)(struct _KFLOW_AI_CHANNEL_CTX* p_ch);
	void (*close)(struct _KFLOW_AI_CHANNEL_CTX* p_ch);
	void (*trigger)(struct _KFLOW_AI_CHANNEL_CTX* p_ch, KFLOW_AI_JOB* p_job);
	void (*onfinish)(struct _KFLOW_AI_CHANNEL_CTX* p_ch, KFLOW_AI_JOB* p_job, UINT32 cycle);
	void (*reset)(struct _KFLOW_AI_CHANNEL_CTX* p_ch);
	void (*reset2)(struct _KFLOW_AI_CHANNEL_CTX* p_ch);
	void (*debug)(struct _KFLOW_AI_CHANNEL_CTX* p_ch, KFLOW_AI_JOB* p_job, UINT32 info);
} KFLOW_AI_CHANNEL_CTX;

typedef struct _KFLOW_AI_ENGINE_CTX {
	char* name;
	LIST_HEAD* wait_queue;
	UINT32 state;  //0: n/a, 1: init
	UINT32 channel_max; //user config max count
	UINT32 channel_count;
	KFLOW_AI_CHANNEL_CTX** p_ch;
	UINT32 attr; //0: fast, 1: slow
	KFLOW_AI_ENGINE_CTX* (*geteng)(UINT32 engine_id);
	void (*init)(struct _KFLOW_AI_ENGINE_CTX* p_eng);
	void (*uninit)(struct _KFLOW_AI_ENGINE_CTX* p_eng);
	UINT32 (*getcfg)(struct _KFLOW_AI_ENGINE_CTX* p_eng, KFLOW_AI_JOB* p_job, UINT32 cfg);
} KFLOW_AI_ENGINE_CTX;

//init/uninit
extern void kflow_ai_core_reset_engine(void);
extern void kflow_ai_core_add_engine(UINT32 engine_id, KFLOW_AI_ENGINE_CTX* p_eng);
extern UINT32 kflow_ai_core_get_engine_cnt(void);
extern KFLOW_AI_ENGINE_CTX* kflow_ai_core_get_engine(UINT32 engine_id);
extern UINT32 kflow_ai_core_get_channel_cnt(KFLOW_AI_ENGINE_CTX* p_eng);
extern KFLOW_AI_CHANNEL_CTX* kflow_ai_core_get_channel(KFLOW_AI_ENGINE_CTX* p_eng, UINT32 channel_id);
extern void kflow_ai_core_reset(void);
extern ER kflow_ai_core_init(void);
extern ER kflow_ai_core_uninit(void);
extern void kflow_ai_core_cfgschd(UINT32 schd);
extern void kflow_ai_core_cfgchk(UINT32 chk_interval);

//net
extern KFLOW_AI_NET* kflow_ai_core_net(UINT32 proc_id);
extern void kflow_ai_net_create(KFLOW_AI_NET* p_net, UINT32 max_job_cnt, UINT32 job_cnt, UINT32 bind_cnt, UINT32 ddr_id);
extern void kflow_ai_net_destory(KFLOW_AI_NET* p_net);
extern KFLOW_AI_JOB* kflow_ai_net_add_job(KFLOW_AI_NET* p_net, UINT32 job_id);
extern KFLOW_AI_JOB* kflow_ai_net_job(KFLOW_AI_NET* p_net, UINT32 job_id);
extern UINT32 kflow_ai_net_job_id(KFLOW_AI_NET* p_net, KFLOW_AI_JOB* p_job);
extern void kflow_ai_net_dump(KFLOW_AI_NET* p_net, UINT32 info);
extern void kflow_ai_net_debug(UINT32 proc_id, UINT32 func);
extern void kflow_ai_net_flow(KFLOW_AI_NET* p_net, UINT32 mask);

//dump
extern void kflow_ai_engine_dump(KFLOW_AI_ENGINE_CTX* p_eng, UINT32 info);
extern void kflow_ai_core_dump(void);
extern void kflow_ai_reg_dump(void);
//clr/set job
extern void kflow_ai_core_clr_job(KFLOW_AI_NET* p_net, KFLOW_AI_JOB* p_job);
//extern void kflow_ai_core_set_job(KFLOW_AI_NET* p_net, KFLOW_AI_JOB* p_job, UINT32 engine_id, UINT32 engine_op, void* p_op_info, void* p_io_info, INT32 wait_ms);
extern void kflow_ai_core_set_job(KFLOW_AI_NET* p_net, KFLOW_AI_JOB* p_job, void* p_op_info, void* p_io_info, INT32 wait_ms);
//clr bind
extern void kflow_ai_core_clr_bind(KFLOW_AI_NET* p_net, KFLOW_AI_BIND* p_bind);
//bind/unbind job
extern void kflow_ai_core_bind_job(KFLOW_AI_NET* p_net, KFLOW_AI_JOB* p_job, KFLOW_AI_JOB* p_next_job);
extern void kflow_ai_core_sum_job(KFLOW_AI_NET* p_net, UINT32* src_count, UINT32* dest_count);
//push job (cannot modify graph)
extern void kflow_ai_core_push_begin(KFLOW_AI_NET* p_net);
extern void kflow_ai_core_push_end(KFLOW_AI_NET* p_net);
extern void kflow_ai_core_push_job(KFLOW_AI_NET* p_net, KFLOW_AI_JOB* p_job);
extern INT32 kflow_ai_core_lock_job(KFLOW_AI_NET* p_net, KFLOW_AI_JOB* p_job);
extern INT32 kflow_ai_core_unlock_job(KFLOW_AI_NET* p_net, KFLOW_AI_JOB* p_job);
//pull job
extern void kflow_ai_core_pull_begin(KFLOW_AI_NET* p_net);
extern void kflow_ai_core_pull_end(KFLOW_AI_NET* p_net);
extern void kflow_ai_core_pull_ready(KFLOW_AI_NET* p_net, KFLOW_AI_JOB* p_job);
extern INT32 kflow_ai_core_pull_job(KFLOW_AI_NET* p_net, KFLOW_AI_JOB** p_job);

//consume job (cannot modify graph)
extern void kflow_ai_core_run_job(KFLOW_AI_CHANNEL_CTX* p_ch, KFLOW_AI_JOB* p_job);
extern void kflow_ai_core_trig_job(KFLOW_AI_CHANNEL_CTX* p_ch, KFLOW_AI_JOB* p_job);
extern void kflow_ai_core_onfinish_job(KFLOW_AI_CHANNEL_CTX* p_ch, KFLOW_AI_JOB* p_job, UINT32 cycle);

extern void kflow_ai_core_dump_job(UINT32 info, UINT32 act, KFLOW_AI_JOB* p_job);




#endif //_KFLOW_AI_CORE_H_
