/**
    Ctl SIE Layer, Buffer ctrl

    @file       ctl_sie_buf_int.h
    @ingroup    mIIPL_Ctrl
    @note       None

    Copyright   Novatek Microelectronics Corp. 2012.  All rights reserved.
*/


#ifndef _CTL_SIE_BUF_INT_H
#define _CTL_SIE_BUF_INT_H

#include "ctl_sie_utility_int.h"

#define KFLOW_SIE_BUF_FLAG 0x5a5a5a5a

/**********************************/
/*  ctl_sie_buf message queue     */
/**********************************/
#define CTL_SIE_BUF_MSG_STS_FREE    0x00000000
#define CTL_SIE_BUF_MSG_STS_LOCK    0x00000001

#define CTL_SIE_BUF_MSG_IGNORE      0x00000000
#define CTL_SIE_BUF_MSG_PUSH        0x00000001  //[0]: buf_io_fp, [1]: raw header address, [2]: reserve
#define CTL_SIE_BUF_MSG_UNLOCK      0x00000002  //[0]: buf_io_fp, [1]: raw header address, [2]: reserve
#define CTL_SIE_BUF_MSG_LOCK        0x00000004  //[0]: buf_io_fp, [1]: raw header address, [2]: reserve

#define CTL_SIE_BUF_CONV2_SIZE(w, h) (((w & 0x0000ffff) << 16) | (h & 0x0000ffff))

typedef enum {
	CTL_SIE_HEAD_ITEM_SIGN = 0,     ///< signature = MAKEFOURCC('V','F','R','M')
	CTL_SIE_HEAD_ITEM_FC,           ///< frame count
	CTL_SIE_HEAD_ITEM_TS,           ///< time stamp
	CTL_SIE_HEAD_ITEM_MAIN_OUT_IMG, ///< main image out
	CTL_SIE_HEAD_ITEM_MAIN_OUT_ADDR,///< main image out addr(ch0) or CCIR out addr(ch0/ch1)
	CTL_SIE_HEAD_ITEM_CH1_ADDR,     ///< ch1 addr, CA
	CTL_SIE_HEAD_ITEM_CH2_ADDR,     ///< ch2 addr, LA out
	CTL_SIE_HEAD_ITEM_DEST_CROP,    ///< dest. crop win
	CTL_SIE_HEAD_ITEM_DIRECT_CTL,   ///< direct flow ctl info(pub_buf_id and pub_addr)
	CTL_SIE_HEAD_ITEM_MAX,
	ENUM_DUMMY4WORD(CTL_SIE_HEAD_ITEM)
} CTL_SIE_HEAD_ITEM;

typedef struct {
	UINT32 cmd;
	UINT32 param[5];
	UINT32 rev[2];
	CTL_SIE_LIST_HEAD list;
} CTL_SIE_BUF_MSG_EVENT;

INT32 ctl_sie_buf_msg_snd(UINT32 cmd, UINT32 p1, UINT32 p2, UINT32 p3);
INT32 ctl_sie_buf_msg_rcv(UINT32 *cmd, UINT32 *p1, UINT32 *p2, UINT32 *p3);
INT32 ctl_sie_buf_msg_flush(void);
INT32 ctl_sie_buf_erase_queue(UINT32 handle);
INT32 ctl_sie_buf_msg_reset_queue(void);
UINT32 ctl_sie_buf_get_free_queue_num(void);

/**********************************/
/*  ctl_sie_buf task ctrl         */
/**********************************/
INT32 ctl_sie_buf_open_tsk(void);
INT32 ctl_sie_buf_close_tsk(void);
INT32 ctl_sie_buf_set_resume(BOOL b_flush_evt);
INT32 ctl_sie_buf_set_pause(BOOL b_wait_end, BOOL b_flush_evt);
INT32 ctl_sie_buf_wait_pause_end(void);


/**********************************/
/*  ctl_sie_buf                   */
/**********************************/
void ctl_sie_buf_update_out_addr(CTL_SIE_HDL *ctrl_hdl, CTL_SIE_HEAD_IDX head_idx);
void ctl_sie_buf_update_out_size(CTL_SIE_HDL *ctrl_hdl);
void ctl_sie_buf_io_cfg(UINT32 id, CTL_SIE_BUF_IO_CFG io_cfg, UINT32 req_size, UINT32 sie_header_addr);
/**********************************/
/*  header ctl                    */
/**********************************/
INT32 ctl_sie_header_create(CTL_SIE_HDL *ctrl_hdl, CTL_SIE_HEAD_IDX head_idx);
void ctl_sie_header_set_cur_head(CTL_SIE_HDL *ctrl_hdl);
void ctl_sie_header_set(CTL_SIE_HEAD_ITEM set_item, CTL_SIE_HDL *ctrl_hdl, CTL_SIE_HEAD_IDX head_idx);
#endif // _CTL_SIE_BUF_INT_H
