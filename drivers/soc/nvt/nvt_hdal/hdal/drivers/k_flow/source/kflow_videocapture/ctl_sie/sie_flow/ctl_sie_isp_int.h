/**
    SIE Ctrl Layer, Algorithm function control

    @file       ctl_sie_isp_int.h
    @ingroup    mISIE_Ctrl
    @note       None

    Copyright   Novatek Microelectronics Corp. 2012.  All rights reserved.
*/

#include "ctl_sie_utility_int.h"
#include "kflow_videocapture/ctl_sie_isp.h"

#ifndef _CTL_SIE_ISP_INT_H
#define _CTL_SIE_ISP_INT_H

// funtion type for sie
typedef INT32(*CTL_SIE_ISP_SET_FP)(CTL_SIE_ID, void *);
typedef INT32(*CTL_SIE_ISP_GET_FP)(CTL_SIE_ID, void *);

#define CTL_SIE_ISP_HANDLE_NAME_MAX_LENGTH  (32)
#define CTL_SIE_ISP_HANDLE_STATUS_FREE      (0)
#define CTL_SIE_ISP_HANDLE_STATUS_USED      (1)

typedef struct {
	CHAR name[CTL_SIE_ISP_HANDLE_NAME_MAX_LENGTH];
	ISP_EVENT_FP fp;
	ISP_EVENT evt;
	UINT32 status;
	CTL_SIE_ISP_CB_MSG cb_msg;
} CTL_SIE_ISP_HANDLE;

typedef struct {
	UINT32 id;
	CTL_SIE_HEADER_INFO sie_head_info;
} CTL_SIE_ISP_GET_IMG_INFO;

typedef enum {
	CTL_SIE_IQ_FUNC_NONE        = 0x00000000, ///< none
	CTL_SIE_IQ_FUNC_OB_BYPASS   = 0x00000001, ///<
	CTL_SIE_IQ_FUNC_CA          = 0x00000002, ///<
	CTL_SIE_IQ_FUNC_LA          = 0x00000004, ///<
	CTL_SIE_IQ_FUNC_CG          = 0x00000008, ///<
	CTL_SIE_IQ_FUNC_DG          = 0x00000010, ///<
	CTL_SIE_IQ_FUNC_DPC         = 0x00000020, ///<
	CTL_SIE_IQ_FUNC_ECS         = 0x00000040, ///<
	CTL_SIE_IQ_FUNC_COMPAND     = 0x00000080, ///<
	CTL_SIE_IQ_FUNC_MD          = 0x00000100, ///<
} CTL_SIE_ISP_IQ_FUNC;

typedef struct {
	BOOL skip_cb_en;
	CTL_SIE_ISP_IQ_FUNC iq_func_off;
} CTL_SIE_ISP_IQ_DBG_INFO;

void ctl_sie_isp_init(void);
UINT32 ctl_sie_isp_get_sts(void);
INT32 ctl_sie_isp_update_sta_out(CTL_SIE_ID id);
INT32 ctl_sie_isp_event_cb(CTL_SIE_ID id, UINT32 sts, UINT64 fc, void *param);
BOOL ctl_sie_isp_int_pull_img_out(UINT32 id, UINT32 sie_header_addr);
void ctl_sie_isp_set_skip(UINT32 id, BOOL skip_en, UINT32 iq_func_off);
CTL_SIE_ISP_HANDLE *ctl_sie_isp_get_hdl(void);

#endif //_CTL_SIE_ISP_INT_H
