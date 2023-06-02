#ifndef _IPL_EVENT_INT_H_
#define _IPL_EVENT_INT_H_

/**
    ipl_event_int.h


    @file       ipl_event_int.h
    @ingroup    mISYSAlg
    @note       Nothing (or anything need to be mentioned).

    Copyright   Novatek Microelectronics Corp. 2011.  All rights reserved.
*/
#include "kwrap/type.h"
#include "kflow_videoprocess/ipp_event.h"
#include "ctl_ipp_util_int.h"

#define IPP_EVENT_ROOT_MAX		16
#define IPP_EVENT_ENTRY_MAX		10

#define IPP_EVENT_CTRL_MASK 0xffff0000
#define IPP_EVENT_NAME_MAX 10

#define IPP_FREE_FLAG_START				(1)

#define IPP_EVENT_STS_FREE			0x00000100
#define IPP_EVENT_STS_CTL_MASK		0xffff0000

#define IPL_EVENT_EVENT_CTL_MASK	IPP_EVENT_STS_CTL_MASK

typedef struct {
	CHAR name[IPP_EVENT_NAME_MAX];
	UINT32 enter_ts;
	IPP_EVENT_FP fp;
	UINT32 exit_ts;
	UINT32 sts;
	CTL_IPP_LIST_HEAD list;
} IPP_EVENT_ENTRY_ITEM;

typedef struct {
	CHAR name[IPP_EVENT_NAME_MAX];
	UINT32 sts;
	CTL_IPP_LIST_HEAD root_list;
	UINT32 root_lock;
	CTL_IPP_LIST_HEAD entry_list[IPP_EVENT_ENTRY_MAX];
	UINT32 entry_lock[IPP_EVENT_ENTRY_MAX];
} IPP_EVENT_ROOT_ITEM;

typedef struct {
	UINT32 flag;
	UINT32 ptn;
} IPP_EVENT_LOCK_INFO;

/**
	sample:
	open -> register -> proc -> proc -> proc -> proc -> proc -> close
	open -> register -> proc -> register -> proc_isr -> proc -> proc -> close

*/
/**
     ipp_event_init

     @note alloc buffer, clear all status
*/
extern UINT32 ipp_event_init(UINT32 num, UINT32 buf_addr, UINT32 is_query);

/**
	ipp event uninit

     @note release buffer
*/
extern void ipp_event_uninit(void);

/**
     ipp_event_open

     @note open a process handle

     @param[in] name      prcess name

     @return Description of data returned.
		- @b 0: open fail
*/
extern UINT32 ipp_event_open(CHAR *name);

/**
     ipp_event_close

     @note close a process handle & flush all register function.

     @param[in] handle

     @return Description of data returned.
		- @b IPP_EVENT_OK: success
		- @b IPP_EVENT_NG: fail
*/
extern UINT32 ipp_event_close(UINT32 handle);

/**
     ipp_event_register

     @note register callback function

     @param[in] handle
     @param[in] event: event id
     @param[in] fp: callback event function pointer
     @param[in] name: event name

     @return Description of data returned.
		- @b IPP_EVENT_OK: success
		- @b IPP_EVENT_NG: fail
*/
extern UINT32 ipp_event_register(UINT32 handle, UINT32 event, IPP_EVENT_FP fp, CHAR *name);

/**
     ipl_event_unregister

     @note unregister callback function

     @param[in] handle
     @param[in] event: event id
     @param[in] fp: callback event function pointer

     @return Description of data returned.
		- @b IPP_EVENT_OK: success
		- @b IPP_EVENT_NG: fail
*/
extern UINT32 ipl_event_unregister(UINT32 handle, UINT32 event, IPP_EVENT_FP fp);

/**
     ipp_event_proc

     @note trigger event

     @param[in] handle
     @param[in] event: event id
     @param[in] p_in: input data structure
     @param[out] p_out: output data structure

     @return Description of data returned.
		- @b IPP_EVENT_OK: success
		- @b IPP_EVENT_NG: fail
*/
extern INT32 ipp_event_proc(UINT32 handle, UINT32 event, void *p_in, void *p_out);

/**
     ipp_event_proc_isr

     @note trigger isr event

     @param[in] handle
     @param[in] event: event id
     @param[in] p_in: input data structure
     @param[out] p_out: output data structure

     @return Description of data returned.
		- @b IPP_EVENT_OK: success
		- @b IPP_EVENT_NG: fail
*/
extern INT32 ipp_event_proc_isr(UINT32 handle, UINT32 event, void *p_in, void *p_out);

/**
     ipp_event_proc_isr

     @note trigger isr event

     @param[in] handle
     @param[in] event: event id
     @param[in] p_in: input data structure
     @param[out] p_out: output data structure

     @return Description of data returned.
		- @b IPP_EVENT_OK: success
		- @b IPP_EVENT_NG: fail
*/
extern void ipp_event_dump(void);
extern void ipp_event_dump_proc_time(int (*dump)(const char *fmt, ...));
#endif //_IPL_EVENT_INT_H_
