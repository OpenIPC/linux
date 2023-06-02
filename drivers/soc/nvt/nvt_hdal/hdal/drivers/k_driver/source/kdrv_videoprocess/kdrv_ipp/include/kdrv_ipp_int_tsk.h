/*
    IPP module driver

    NT98520 IPP internal header file.

    @file       kdrv_ipp_int_tsk.h
    @ingroup    mIIPP
    @note       Nothing

    Copyright   Novatek Microelectronics Corp. 2019.  All rights reserved.
*/
#ifndef _KDRV_IPP_INT_TSK_H_
#define _KDRV_IPP_INT_TSK_H_

#include "kwrap/nvt_type.h"
#include "kdrv_ipp_int.h"

#define KDRV_IPP_TSK_PRIORITY (3)

/* common flag bit for both proc task and cb task */
#define KDRV_IPP_TSK_PAUSE		FLGPTN_BIT(0)
#define KDRV_IPP_TSK_PAUSE_END	FLGPTN_BIT(1)
#define KDRV_IPP_TSK_RESUME		FLGPTN_BIT(2)
#define KDRV_IPP_TSK_RESUME_END	FLGPTN_BIT(3)
#define KDRV_IPP_TSK_TRIGGER	FLGPTN_BIT(4)
#define KDRV_IPP_TSK_EXIT		FLGPTN_BIT(5)
#define KDRV_IPP_TSK_EXIT_END	FLGPTN_BIT(6)
#define KDRV_IPP_TSK_CHK		FLGPTN_BIT(7)
#define KDRV_IPP_TSK_OOPS		FLGPTN_BIT(31)

/* cb task */
#if 0
#define KDRV_IPP_TSK_JOB_DONE_OFS	(24)
#define KDRV_IPP_TSK_JOB_DONE_1 FLGPTN_BIT(KDRV_IPP_TSK_JOB_DONE_OFS + 0)
#define KDRV_IPP_TSK_JOB_DONE_2 FLGPTN_BIT(KDRV_IPP_TSK_JOB_DONE_OFS + 1)
#define KDRV_IPP_TSK_JOB_DONE_3 FLGPTN_BIT(KDRV_IPP_TSK_JOB_DONE_OFS + 2)
#define KDRV_IPP_TSK_JOB_DONE_4 FLGPTN_BIT(KDRV_IPP_TSK_JOB_DONE_OFS + 3)
#define KDRV_IPP_TSK_JOB_DONE_5 FLGPTN_BIT(KDRV_IPP_TSK_JOB_DONE_OFS + 4)
#define KDRV_IPP_TSK_JOB_DONE_6 FLGPTN_BIT(KDRV_IPP_TSK_JOB_DONE_OFS + 5)
#define KDRV_IPP_TSK_JOB_DONE_7 FLGPTN_BIT(KDRV_IPP_TSK_JOB_DONE_OFS + 6)
#define KDRV_IPP_TSK_JOB_DONE_8 FLGPTN_BIT(KDRV_IPP_TSK_JOB_DONE_OFS + 7)
#endif

/* pool flag bit */
#define KDRV_IPP_POOL_LL_BLK_EXIST	FLGPTN_BIT(0)
#define KDRV_IPP_POOL_CFG_BLK_EXIST	FLGPTN_BIT(1)
#define KDRV_IPP_POOL_JOB_BLK_EXIST	FLGPTN_BIT(2)

/* ipp eng status bit */
#define KDRV_IPP_ENG_IFE_IDLE		FLGPTN_BIT(KDRV_IPP_ENG_IFE)
#define KDRV_IPP_ENG_DCE_IDLE		FLGPTN_BIT(KDRV_IPP_ENG_DCE)
#define KDRV_IPP_ENG_IPE_IDLE		FLGPTN_BIT(KDRV_IPP_ENG_IPE)
#define KDRV_IPP_ENG_IME_IDLE		FLGPTN_BIT(KDRV_IPP_ENG_IME)
#define KDRV_IPP_ENG_STS_DEFAULT	(KDRV_IPP_ENG_IFE_IDLE | KDRV_IPP_ENG_DCE_IDLE | KDRV_IPP_ENG_IPE_IDLE | KDRV_IPP_ENG_IME_IDLE)

INT32 kdrv_ipp_ife_isr(UINT32 dev_id, UINT32 msg, void *in , void *out);
INT32 kdrv_ipp_dce_isr(UINT32 dev_id, UINT32 msg, void *in , void *out);
INT32 kdrv_ipp_ipe_isr(UINT32 dev_id, UINT32 msg, void *in , void *out);
INT32 kdrv_ipp_ime_isr(UINT32 dev_id, UINT32 msg, void *in , void *out);

void kdrv_ipp_int_set_eng_status(KDRV_IPP_HANDLE *p_hdl, UINT32 sts);
void kdrv_ipp_int_clr_eng_status(KDRV_IPP_HANDLE *p_hdl, UINT32 sts);
INT32 kdrv_ipp_int_wait_eng_status(KDRV_IPP_HANDLE *p_hdl, UINT32 sts, UINT32 timeout, UINT32 b_clr);

void kdrv_ipp_int_trig_proc_tsk(KDRV_IPP_HANDLE *p_hdl);
INT32 kdrv_ipp_int_resume_proc_tsk(KDRV_IPP_HANDLE *p_hdl, INT32 wait_end);
INT32 kdrv_ipp_int_pause_proc_tsk(KDRV_IPP_HANDLE *p_hdl, INT32 wait_end);
INT32 kdrv_ipp_int_close_proc_tsk(KDRV_IPP_HANDLE *p_hdl, INT32 wait_end);

void kdrv_ipp_int_trig_cb_tsk(KDRV_IPP_HANDLE *p_hdl);
INT32 kdrv_ipp_int_resume_cb_tsk(KDRV_IPP_HANDLE *p_hdl, INT32 wait_end);
INT32 kdrv_ipp_int_pause_cb_tsk(KDRV_IPP_HANDLE *p_hdl, INT32 wait_end);
INT32 kdrv_ipp_int_close_cb_tsk(KDRV_IPP_HANDLE *p_hdl, INT32 wait_end);

#endif /* _KDRV_IPP_TSK_INT_H_ */

