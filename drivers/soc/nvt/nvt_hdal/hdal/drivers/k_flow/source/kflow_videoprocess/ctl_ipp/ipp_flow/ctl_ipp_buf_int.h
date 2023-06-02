/**
    IPL Ctrl Layer, Buffer ctrl

    @file       ctl_ipp_buf_int.h
    @ingroup    mIIPL_Ctrl
    @note       None

    Copyright   Novatek Microelectronics Corp. 2012.  All rights reserved.
*/


#ifndef _CTL_IPP_BUF_INT_H
#define _CTL_IPP_BUF_INT_H

#include "kwrap/type.h"
#include "ctl_ipp_util_int.h"

#define CTL_IPP_BUF_NO_USE  0xffffffff

/* for single out check,
	this tag will be assign to the top of the buffer
	if this tag is not overwrite by engine, the buffer should not be pushed
*/
#define CTL_IPP_BUF_CHK_TAG MAKEFOURCC('N', 'O', 'U', 'T')

/**********************************/
/*	ctl_ipp_buf message queue	  */
/**********************************/
#define CTL_IPP_BUF_MSG_STS_FREE    0x00000000
#define CTL_IPP_BUF_MSG_STS_LOCK    0x00000001

#define CTL_IPP_BUF_MSG_IGNORE      0x00000000
#define CTL_IPP_BUF_MSG_PUSH		0x00000001	//[0]: buf_io_fp, [1]: ctrl_info address, [2]: reserve
#define CTL_IPP_BUF_MSG_RELEASE		0x00000002  //[0]: buf_io_fp, [1]: ctrl_info address, [2]: bufio_stop
#define CTL_IPP_BUF_MSG_PUSH_LOWDLY	0x00000003	//[0]: buf_io_fp, [1]: ctrl_info address, [2]: reserve, add to the head of list
#define CTL_IPP_BUF_MSG_PUSH_SLICE	0x00000004	//[0]: buf_io_fp, [1]: ctrl_info address, [2]: slice_idx

typedef struct {
	UINT32 cmd;
	UINT32 param[5];
	UINT32 rev[2];
	CTL_IPP_LIST_HEAD list;
} CTL_IPP_BUF_MSG_EVENT;

typedef enum {
	CTRL_IPP_BUF_ITEM_LCA = 0,
	CTRL_IPP_BUF_ITEM_MO,
	CTRL_IPP_BUF_ITEM_MV,
	CTRL_IPP_BUF_ITEM_DFG,
	CTRL_IPP_BUF_ITEM_PM,
	CTRL_IPP_BUF_ITEM_YOUT,
	CTRL_IPP_BUF_ITEM_MS_ROI,
	CTRL_IPP_BUF_ITEM_MS,
	CTRL_IPP_BUF_ITEM_3DNR_STA,
	CTRL_IPP_BUF_ITEM_WDR,
	CTRL_IPP_BUF_ITEM_VA,
	CTRL_IPP_BUF_ITEM_DUMMY_UV,
	CTRL_IPP_BUF_ITEM_FCVG,
	CTRL_IPP_BUF_ITEM_MAX,
} CTRL_IPP_BUF_ITEM;

typedef enum {
	CTL_IPP_BUF_LCA_1 = 0,
	CTL_IPP_BUF_LCA_2,
	CTL_IPP_BUF_LCA_3,
	CTL_IPP_BUF_LCA_MAX,

	CTL_IPP_BUF_MO_1,
	CTL_IPP_BUF_MO_2,
	CTL_IPP_BUF_MO_3,
	CTL_IPP_BUF_MO_MAX,

	CTL_IPP_BUF_MV_1,
	CTL_IPP_BUF_MV_2,
	CTL_IPP_BUF_MV_3,
	CTL_IPP_BUF_MV_MAX,

	CTL_IPP_BUF_DFG_1,
	CTL_IPP_BUF_DFG_2,
	CTL_IPP_BUF_DFG_3,
	CTL_IPP_BUF_DFG_MAX,

	CTL_IPP_BUF_PM_1,
	CTL_IPP_BUF_PM_2,
	CTL_IPP_BUF_PM_3,
	CTL_IPP_BUF_PM_MAX,

	CTL_IPP_BUF_YOUT_1,
	CTL_IPP_BUF_YOUT_2,
	CTL_IPP_BUF_YOUT_3,
	CTL_IPP_BUF_YOUT_4,
	CTL_IPP_BUF_YOUT_MAX,

	CTL_IPP_BUF_MS_ROI_1,
	CTL_IPP_BUF_MS_ROI_2,
	CTL_IPP_BUF_MS_ROI_3,
	CTL_IPP_BUF_MS_ROI_MAX,

	CTL_IPP_BUF_MS_1,
	CTL_IPP_BUF_MS_2,
	CTL_IPP_BUF_MS_3,
	CTL_IPP_BUF_MS_MAX,

	CTL_IPP_BUF_3DNR_STA_1,
	CTL_IPP_BUF_3DNR_STA_2,
	CTL_IPP_BUF_3DNR_STA_3,
	CTL_IPP_BUF_3DNR_STA_MAX,

	CTL_IPP_BUF_WDR_1,
	CTL_IPP_BUF_WDR_2,
	CTL_IPP_BUF_WDR_3,
	CTL_IPP_BUF_WDR_MAX,

	CTL_IPP_BUF_VA_1,
	CTL_IPP_BUF_VA_2,
	CTL_IPP_BUF_VA_3,
	CTL_IPP_BUF_VA_MAX,

	CTL_IPP_BUF_DUMMY_UV_1,
	CTL_IPP_BUF_DUMMY_UV_2,
	CTL_IPP_BUF_DUMMY_UV_3,
	CTL_IPP_BUF_DUMMY_UV_MAX,

	CTL_IPP_BUF_FCVG_1,
	CTL_IPP_BUF_FCVG_2,
	CTL_IPP_BUF_FCVG_3,
	CTL_IPP_BUF_FCVG_MAX,

	CTL_IPP_DBG_BUF_MAX_CNT,
} CTL_IPP_DBG_PRI_BUF_IDX;

ER ctl_ipp_buf_msg_snd(UINT32 cmd, UINT32 p1, UINT32 p2, UINT32 p3);
ER ctl_ipp_buf_msg_rcv(UINT32 *cmd, UINT32 *p1, UINT32 *p2, UINT32 *p3);
ER ctl_ipp_buf_msg_flush(void);
ER ctl_ipp_buf_erase_queue(UINT32 handle);
ER ctl_ipp_buf_erase_path_in_queue(UINT32 handle, UINT32 pid);
ER ctl_ipp_buf_msg_reset_queue(void);
UINT32 ctl_ipp_buf_get_free_queue_num(void);
void ctl_ipp_buf_msg_dump(int (*dump)(const char *fmt, ...));

/**********************************/
/*	ctl_ipp_buf task ctrl		  */
/**********************************/
ER ctl_ipp_buf_open_tsk(void);
ER ctl_ipp_buf_close_tsk(void);
ER ctl_ipp_buf_set_resume(BOOL b_flush_evt);
ER ctl_ipp_buf_set_pause(BOOL b_wait_end, BOOL b_flush_evt);
ER ctl_ipp_buf_wait_pause_end(void);
void ctl_ipp_buf_push(UINT32 bufio_fp_addr, UINT32 ctrl_info_addr, UINT32 rev);
void ctl_ipp_buf_push_low_delay(UINT32 bufio_fp_addr, UINT32 ctrl_info_addr, UINT32 rev);
void ctl_ipp_buf_push_slice(UINT32 bufio_fp_addr, UINT32 ctrl_info_addr, UINT32 slice_idx);
void ctl_ipp_buf_release(UINT32 bufio_fp_addr, UINT32 ctrl_info_addr, UINT32 bufio_stop, INT32 err_msg);
void ctl_ipp_buf_release_path(UINT32 bufio_fp_addr, UINT32 ctrl_info_addr, UINT32 bufio_stop, UINT32 pid, INT32 err_msg);
void ctl_ipp_buf_iostop_path(UINT32 bufio_fp_addr, UINT32 hdl, UINT32 pid);
void ctl_ipp_buf_iostart_path(UINT32 bufio_fp_addr, UINT32 hdl, UINT32 pid);
void ctl_ipp_buf_frm_end(UINT32 bufio_fp_addr, UINT32 ctrl_info_addr, UINT32 rev);
void ctl_ipp_buf_frm_start(UINT32 bufio_fp_addr, UINT32 ctrl_info_addr, UINT32 rev);

/**********************************/
/*	ctl_ipp_buf					  */
/**********************************/

typedef struct {
	USIZE src_img_size;
	USIZE md_size;
	UINT32 md_lofs;
	UINT32 reserved[3];
} CTL_IPP_BUF_MD_HEADER;

/**
	Get buffer from bufio_cb
*/
void ctl_ipp_buf_alloc(CTL_IPP_HANDLE *ctrl_hdl, CTL_IPP_INFO_LIST_ITEM *ctrl_info);

/**
	Get private buffer
*/
void ctl_ipp_buf_pri_alloc(CTL_IPP_HANDLE *ctrl_hdl, CTL_IPP_INFO_LIST_ITEM *ctrl_info);

/**
	Get private buffer(for common task)
*/
UINT32 ctl_ipp_buf_task_alloc(CTRL_IPP_BUF_ITEM item, UINT32 ctrl_hdl, UINT32 out_addr, USIZE out);

/**
	search private buffer
*/
UINT32 ctl_ipp_buf_pri_search(CTL_IPP_HANDLE *ctrl_hdl, UINT32 last_rdy_addr, UINT32 buf_item, UINT32 *addr);

/**
	Iint private buffer information
*/
void ctl_ipp_buf_pri_init(UINT32 buf_info);

/**
	buf io fp wrapper
*/
void ctl_ipp_buf_fp_wrapper(UINT32 hdl, UINT32 bufio_fp_addr, CTL_IPP_BUF_IO_CFG cfg, CTL_IPP_OUT_BUF_INFO *p_buf);

#endif // _CTL_IPP_BUF_INT_H
