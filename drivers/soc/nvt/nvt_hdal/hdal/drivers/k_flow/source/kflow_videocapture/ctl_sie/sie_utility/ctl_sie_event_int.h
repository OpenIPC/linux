#ifndef _CTL_SIE_EVENT_INT_H_
#define _CTL_SIE_EVENT_INT_H_

/**
    ctl_sie_event_int.h


    @file       ctl_sie_event_int.h
    @ingroup    mISYSAlg
    @note       Nothing (or anything need to be mentioned).

    Copyright   Novatek Microelectronics Corp. 2011.  All rights reserved.
*/
#include "kflow_videocapture/ctl_sie_event.h"
#include "ctl_sie_utility_int.h"

#define SIE_EVENT_ROOT_MAX		8
#define SIE_EVENT_ENTRY_MAX		6

#define SIE_EVENT_CTRL_MASK 0xffff0000
#define SIE_EVENT_NAME_MAX 10

#define SIE_FREE_FLAG_PAGE_SIZE			32//sizeof(FLGPTN)
#define SIE_FREE_FLAG_START				(SIE_EVENT_ROOT_MAX + 1)

#define SIE_EVENT_FLAG_MAX			2
#define SIE_EVENT_FREE_ENTRY_ITEM_MAX_NUM ((SIE_EVENT_FLAG_MAX * SIE_FREE_FLAG_PAGE_SIZE) - SIE_FREE_FLAG_START)

#define SIE_EVENT_FREE_ENTRY_ITEM_NUM (SIE_EVENT_ROOT_MAX * SIE_EVENT_ENTRY_MAX)
#if (SIE_EVENT_FREE_ENTRY_ITEM_NUM > SIE_EVENT_FREE_ENTRY_ITEM_MAX_NUM)
#error "SIE_EVENT_POOL_FREE_NUM must be <= SIE_EVENT_FREE_ENTRY_ITEM_MAX_NUM"
#endif

#define SIE_EVENT_STS_FREE			0x00000100
#define SIE_EVENT_STS_CTL_MASK		0xffff0000

#define SIE_EVENT_EVENT_CTL_MASK	SIE_EVENT_STS_CTL_MASK

typedef struct {
	CHAR name[SIE_EVENT_NAME_MAX];
	UINT32 enter_ts;
	CTL_SIE_EVENT_FP fp;
	UINT32 exit_ts;
	UINT32 sts;
	CTL_SIE_LIST_HEAD list;
} SIE_EVENT_ENTRY_ITEM;

typedef struct {
	CHAR name[SIE_EVENT_NAME_MAX];
	UINT32 sts;
	CTL_SIE_LIST_HEAD root_list;
	UINT32 root_lock;
	CTL_SIE_LIST_HEAD entry_list[SIE_EVENT_ENTRY_MAX];
	UINT32 entry_lock[SIE_EVENT_ENTRY_MAX];
} SIE_EVENT_ROOT_ITEM;

typedef struct {
	UINT32 flag;
	UINT32 ptn;
} SIE_EVENT_LOCK_INFO;

/**
	sample:
	open -> register -> proc -> proc -> proc -> proc -> proc -> close
	open -> register -> proc -> register -> proc_isr -> proc -> proc -> close

*/
/**
     sie_event_reset

     @note clear all status
*/
extern void sie_event_reset(void);

/**
     sie_event_open

     @note open a process handle

     @param[in] name      prcess name

     @return Description of data returned.
		- @b 0:	open fail
*/
extern UINT32 sie_event_open(CHAR *name);

/**
     sie_event_close

     @note close a process handle & flush all register function.

     @param[in] handle

     @return Description of data returned.
		- @b SIE_EVENT_OK: success
		- @b SIE_EVENT_NG: fail
*/
extern UINT32 sie_event_close(UINT32 handle);

/**
     sie_event_register

     @note register callback function

     @param[in] handle
     @param[in] event: event id
     @param[in] fp: callback event function pointer
     @param[in] name: event name

     @return Description of data returned.
		- @b SIE_EVENT_OK: success
		- @b SIE_EVENT_NG: fail
*/
extern UINT32 sie_event_register(UINT32 handle, UINT32 event, CTL_SIE_EVENT_FP fp, CHAR *name);

/**
     sie_event_unregister

     @note unregister callback function

     @param[in] handle
     @param[in] event: event id
     @param[in] fp: callback event function pointer

     @return Description of data returned.
		- @b SIE_EVENT_OK: success
		- @b SIE_EVENT_NG: fail
*/
extern UINT32 sie_event_unregister(UINT32 handle, UINT32 event, CTL_SIE_EVENT_FP fp);

/**
     sie_event_proc

     @note trigger event

     @param[in] handle
     @param[in] event: event id
     @param[in] p_in: input data structure
     @param[out] p_out: output data structure

     @return Description of data returned.
		- @b SIE_EVENT_OK: success
		- @b SIE_EVENT_NG: fail
*/
extern INT32 sie_event_proc(UINT32 handle, UINT32 event, void *p_in, void *p_out);

/**
     sie_event_proc_isr

     @note trigger isr event

     @param[in] handle
     @param[in] event: event id
     @param[in] p_in: input data structure
     @param[out] p_out: output data structure

     @return Description of data returned.
		- @b SIE_EVENT_OK: success
		- @b SIE_EVENT_NG: fail
*/
extern INT32 sie_event_proc_isr(UINT32 handle, UINT32 event, void *p_in, void *p_out);

/**
     sie_event_proc_isr

     @note trigger isr event

     @param[in] handle
     @param[in] event: event id
     @param[in] p_in: input data structure
     @param[out] p_out: output data structure

     @return Description of data returned.
		- @b SIE_EVENT_OK: success
		- @b SIE_EVENT_NG: fail
*/
extern void sie_event_dump(int (*dump)(const char *fmt, ...));
#endif //_CTL_SIE_EVENT_INT_H_
