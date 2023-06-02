/**
    IPL Ctrl Layer, Flow Task ctrl

    @file       ctl_ipp_flow_task_int.h
    @ingroup    mIIPL_Ctrl
    @note       None

    Copyright   Novatek Microelectronics Corp. 2012.  All rights reserved.
*/


#ifndef _CTL_IPP_FLOW_TASK_INT_H
#define _CTL_IPP_FLOW_TASK_INT_H

#include "kwrap/type.h"
#include "ctl_ipp_util_int.h"

/**********************************/
/*	ctl_ipp_ise message queue	  */
/**********************************/
#define CTL_IPP_ISE_MSG_STS_FREE    0x00000000
#define CTL_IPP_ISE_MSG_STS_LOCK    0x00000001

#define CTL_IPP_ISE_MSG_IGNORE      0x00000000
#define CTL_IPP_ISE_MSG_PROCESS     0x00000001	//[0]: EVENT [1]: ctrl_info address, [2]: reserve
#define CTL_IPP_ISE_MSG_DROP		0x00000002  //[0]: EVENT [1]: ctrl_info address, [2]: bufio_stop

typedef enum {
	CTL_IPP_ISE_PM = 0,
	CTL_IPP_ISE_MAX,
} CTL_IPP_ISE_MSG;

typedef struct {
	UINT32 hdl_addr;
	USIZE size;
	UINT32 lofs;
	UINT32 addr;
	UINT32 pixel_blk;
} CTL_IPP_ISE_OUT_INFO;

typedef struct {
	UINT32 cmd;
	UINT32 param[5];
	UINT32 rev[2];
	CTL_IPP_LIST_HEAD list;
} CTL_IPP_ISE_MSG_EVENT;

ER ctl_ipp_ise_msg_snd(UINT32 cmd, UINT32 p1, UINT32 p2, UINT32 p3);
ER ctl_ipp_ise_msg_rcv(UINT32 *cmd, UINT32 *p1, UINT32 *p2, UINT32 *p3);
ER ctl_ipp_ise_msg_flush(void);
ER ctl_ipp_ise_erase_queue(UINT32 handle);
ER ctl_ipp_ise_msg_reset_queue(void);
ER ctl_ipp_ise_msg_snd_chkdrop(UINT32 msg, UINT32 addr);
UINT32 ctl_ipp_ise_get_free_queue_num(void);

/**********************************/
/*	ctl_ipp_ise task ctrl		  */
/**********************************/
ER ctl_ipp_ise_open_tsk(void);
ER ctl_ipp_ise_close_tsk(void);
ER ctl_ipp_ise_set_resume(BOOL b_flush_evt);
ER ctl_ipp_ise_set_pause(BOOL b_wait_end, BOOL b_flush_evt);
ER ctl_ipp_ise_wait_pause_end(void);
ER ctl_ipp_ise_wait_process_end(void);
void ctl_ipp_ise_process_oneshot(UINT32 msg, UINT32 ctrl_info_addr, CTL_IPP_ISE_OUT_INFO *p_ise_out);
void ctl_ipp_ise_process(UINT32 msg, UINT32 ctrl_info_addr, UINT32 rev);
void ctl_ipp_ise_drop(UINT32 msg, UINT32 ctrl_info_addr, UINT32 rev);
void ctl_ipp_ise_cal_pm_size(CTL_IPP_PM_PXL_BLK pixel_blk, UINT32 *size_h, UINT32 *size_v);
ER ctl_ipp_ise_get_last_rdy(UINT32 msg, CTL_IPP_ISE_OUT_INFO *p_last_rdy);
void ctl_ipp_ise_dumpinfo(int (*dump)(const char *fmt, ...));
UINT32 ctl_ipp_ise_pool_init(UINT32 num, UINT32 buf_addr, UINT32 is_query);
void ctl_ipp_ise_pool_free(void);

#endif // _CTL_IPP_FLOW_TASK_INT_H
