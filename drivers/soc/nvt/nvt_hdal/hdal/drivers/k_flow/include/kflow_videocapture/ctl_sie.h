/**
    SIE CTRL Layer

    @file       sie_ctrl.h
    @ingroup    mILibIPH
    @note

    Copyright   Novatek Microelectronics Corp. 2012.  All rights reserved.
*/
#ifndef _CTL_SIE_H_
#define _CTL_SIE_H_

#include "kwrap/type.h"
#include "kflow_videocapture/ctl_sie_utility.h"
#include "kflow_videocapture/ctl_sie_spt.h"

typedef enum {
	CTL_SIE_CBEVT_BUFIO         = 0 | CTL_SIE_EVENT_ISR_TAG,    //sts ref. to CTL_SIE_BUF_IO_CFG
	CTL_SIE_CBEVT_DIRECT        = 1 | CTL_SIE_EVENT_ISR_TAG,    //sie direct to ipp cb event
	CTL_SIE_CBEVT_ENG_SIE_ISR   = 2 | CTL_SIE_EVENT_ISR_TAG,    //sts ref. to CTL_SIE_INTE
	CTL_SIE_CBEVT_MAX = 3,
} CTL_SIE_CBEVT_TYPE;

/**
    SIE In/Out buffer config
*/
typedef enum _CTL_SIE_BUF_IO_CFG {
	CTL_SIE_BUF_IO_NEW      = 0x00000001,
	CTL_SIE_BUF_IO_PUSH     = 0x00000002,
	CTL_SIE_BUF_IO_LOCK     = 0x00000004,
	CTL_SIE_BUF_IO_UNLOCK   = 0x00000008,
	CTL_SIE_BUF_IO_ALL      = 0xffffffff,
	ENUM_DUMMY4WORD(CTL_SIE_BUF_IO_CFG)
} CTL_SIE_BUF_IO_CFG;

/**
    SIE Direct CB Cfg
*/
typedef enum _CTL_SIE_DIRECT_CFG {
	CTL_SIE_DIRECT_TRIG_START       = 0x00000001,   //trigger ipp start
	CTL_SIE_DIRECT_TRIG_STOP        = 0x00000002,   //trigger ipp stop
	CTL_SIE_DIRECT_FRM_CFG_START    = 0x00000004,   //trigger ipp when bp3(center of crop height)
	CTL_SIE_DIRECT_PUSH_RDY_BUF     = 0x00000008,   //push ready buffer
	CTL_SIE_DIRECT_DROP_BUF         = 0x00000010,   //drop public buffer
	CTL_SIE_DIRECT_SKIP_CFG         = 0x00000020,   //skip cfg next frame
	CTL_SIE_DIRECT_CFG_ALL          = 0xffffffff,
	ENUM_DUMMY4WORD(CTL_SIE_DIRECT_CFG)
} CTL_SIE_DIRECT_CFG;

typedef struct {
	CTL_SIE_CBEVT_TYPE cbevt;
	CTL_SIE_EVENT_FP fp;
	UINT32 sts;                 //wait event status
} CTL_SIE_REG_CB_INFO;

/**
    vdo_frm reserved info
    resv1: direct mode get raw flag
    reserved[0]: for proc cmd out buf wp using (ipp_dis_wp_en << 31 | sie_wp_en << 30 | set_idx << 4 | ddr_id)
    reserved[1]:
    reserved[2]: public buf id(direct mode only)
    reserved[3]: public buf addr(direct mode only)
    reserved[4]: ctl_sie_id(direct mode only)
    reserved[5]: direct: ring buffer length, dram: dest crop construction of (x << 16 | y)
    reserved[6]: dest crop construction of (w << 16 | h)
    reserved[7]: encode rate
*/
typedef struct {
	UINT32 buf_id;      ///for public buffer control
	UINT32 buf_addr;
	VDO_FRAME vdo_frm;
} CTL_SIE_HEADER_INFO;

void kflow_ctl_sie_init(void);
void kflow_ctl_sie_uninit(void);
UINT32 ctl_sie_buf_query(UINT32 num);
INT32 ctl_sie_init(UINT32 buf_addr, UINT32 buf_size);
INT32 ctl_sie_uninit(void);
UINT32 ctl_sie_open(void *open_cfg);    // CTL_SIE_OPEN_CFG
INT32 ctl_sie_close(UINT32 hdl);
INT32 ctl_sie_set(UINT32 hdl, CTL_SIE_ITEM item, void *data);
INT32 ctl_sie_get(UINT32 hdl, CTL_SIE_ITEM item, void *data);
INT32 ctl_sie_suspend(UINT32 hdl, void *data);
INT32 ctl_sie_resume(UINT32 hdl, void *data);
void ctl_sie_spt(CTL_SIE_ID id, CTL_SIE_SPT_ITEM item, void *spt);

#endif //_CTL_SIE_H_