/**
    CTL SIE Layer

    @file       ctl_sie_isp_task_int.h
    @ingroup    mIIPL_Ctrl
    @note       None

    Copyright   Novatek Microelectronics Corp. 2012.  All rights reserved.
*/


#ifndef _CTL_SIE_ISP_TASK_INT_H
#define _CTL_SIE_ISP_TASK_INT_H

#include "kwrap/list.h"
#include "ctl_sie_dbg.h"

/**********************************/
/*  ctl_sie_isp_message queue     */
/**********************************/
#define CTL_SIE_ISP_MSG_STS_FREE        0x00000000
#define CTL_SIE_ISP_MSG_STS_LOCK        0x00000001

#define CTL_SIE_ISP_MSG_IGNORE          0x00000000
#define CTL_SIE_ISP_MSG_CBEVT_ISP       0x00000001

typedef struct vos_list_head CTL_SIE_LIST_HEAD;
INT32 ctl_sie_isp_cb_proc(UINT32 id, UINT32 evt, UINT32 cb_fp, UINT32 frame_cnt);

typedef struct {
	UINT32 cmd;
	UINT32 param[5];
	UINT32 rev[2];
	CTL_SIE_LIST_HEAD list;
} CTL_SIE_ISP_MSG_EVENT;

INT32 ctl_sie_isp_msg_snd(UINT32 cmd, UINT32 p1, UINT32 p2, UINT32 p3, UINT32 p4);
INT32 ctl_sie_isp_msg_rcv(UINT32 *cmd, UINT32 *p1, UINT32 *p2, UINT32 *p3, UINT32 *p4);
INT32 ctl_sie_isp_msg_flush(void);
INT32 ctl_sie_isp_erase_queue(UINT32 handle);
INT32 ctl_sie_isp_msg_reset_queue(void);
UINT32 ctl_sie_isp_get_free_queue_num(void);

/**********************************/
/*  ctl_sie_isp task ctrl         */
/**********************************/
INT32 ctl_sie_isp_open_tsk(void);
INT32 ctl_sie_isp_close_tsk(void);
INT32 ctl_sie_isp_task_set_resume(BOOL b_flush_evt);
INT32 ctl_sie_isp_task_set_pause(BOOL b_wait_end, BOOL b_flush_evt);
INT32 ctl_sie_isp_task_wait_pause_end(void);

#endif //_CTL_SIE_ISP_TASK_INT_H