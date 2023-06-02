/*
    ISE module driver

    NT98520 ISE internal header file.

    @file       kdrv_ise_tsk_int.h
    @ingroup    mIIPPISE
    @note       Nothing

    Copyright   Novatek Microelectronics Corp. 2019.  All rights reserved.
*/
#ifndef _KDRV_ISE_INT_TSK_H_
#define _KDRV_ISE_INT_TSK_H_

#include "kwrap/nvt_type.h"
#include "kwrap/flag.h"
#include "kdrv_ise_int.h"

#define KDRV_ISE_TSK_PRI (4) /* SIE(2), IPP(3), ise task process time < 100us, set to 4 prevent heavy-used case(AI) affect ipp/codec */

/* common flag bit for both proc task and cb task */
#define KDRV_ISE_TSK_PAUSE		FLGPTN_BIT(0)
#define KDRV_ISE_TSK_PAUSE_END	FLGPTN_BIT(1)
#define KDRV_ISE_TSK_RESUME		FLGPTN_BIT(2)
#define KDRV_ISE_TSK_RESUME_END	FLGPTN_BIT(3)
#define KDRV_ISE_TSK_TRIGGER	FLGPTN_BIT(4)
#define KDRV_ISE_TSK_EXIT		FLGPTN_BIT(5)
#define KDRV_ISE_TSK_EXIT_END	FLGPTN_BIT(6)
#define KDRV_ISE_TSK_CHK		FLGPTN_BIT(7)

/* cb task */
#define KDRV_ISE_TSK_JOB_FAIL		FLGPTN_BIT(30)
#define KDRV_ISE_TSK_JOB_DONE		FLGPTN_BIT(31)
#if 0
#define KDRV_ISE_TSK_JOB_DONE_OFS	(24)
#define KDRV_ISE_TSK_JOB_DONE_1 FLGPTN_BIT(KDRV_ISE_TSK_JOB_DONE_OFS + 0)
#define KDRV_ISE_TSK_JOB_DONE_2 FLGPTN_BIT(KDRV_ISE_TSK_JOB_DONE_OFS + 1)
#define KDRV_ISE_TSK_JOB_DONE_3 FLGPTN_BIT(KDRV_ISE_TSK_JOB_DONE_OFS + 2)
#define KDRV_ISE_TSK_JOB_DONE_4 FLGPTN_BIT(KDRV_ISE_TSK_JOB_DONE_OFS + 3)
#define KDRV_ISE_TSK_JOB_DONE_5 FLGPTN_BIT(KDRV_ISE_TSK_JOB_DONE_OFS + 4)
#define KDRV_ISE_TSK_JOB_DONE_6 FLGPTN_BIT(KDRV_ISE_TSK_JOB_DONE_OFS + 5)
#define KDRV_ISE_TSK_JOB_DONE_7 FLGPTN_BIT(KDRV_ISE_TSK_JOB_DONE_OFS + 6)
#define KDRV_ISE_TSK_JOB_DONE_8 FLGPTN_BIT(KDRV_ISE_TSK_JOB_DONE_OFS + 7)
#endif

/* pool flag bit */
#define KDRV_ISE_POOL_LL_BLK_EXIST	FLGPTN_BIT(0)
#define KDRV_ISE_POOL_CFG_BLK_EXIST	FLGPTN_BIT(1)
#define KDRV_ISE_POOL_JOB_BLK_EXIST	FLGPTN_BIT(2)

void kdrv_ise_int_trig_proc_tsk(KDRV_ISE_HANDLE *p_hdl);
INT32 kdrv_ise_int_resume_proc_tsk(KDRV_ISE_HANDLE *p_hdl, INT32 wait_end);
INT32 kdrv_ise_int_pause_proc_tsk(KDRV_ISE_HANDLE *p_hdl, INT32 wait_end);
INT32 kdrv_ise_int_close_proc_tsk(KDRV_ISE_HANDLE *p_hdl, INT32 wait_end);

void kdrv_ise_int_trig_cb_tsk(KDRV_ISE_HANDLE *p_hdl);
INT32 kdrv_ise_int_resume_cb_tsk(KDRV_ISE_HANDLE *p_hdl, INT32 wait_end);
INT32 kdrv_ise_int_pause_cb_tsk(KDRV_ISE_HANDLE *p_hdl, INT32 wait_end);
INT32 kdrv_ise_int_close_cb_tsk(KDRV_ISE_HANDLE *p_hdl, INT32 wait_end);

#endif /* _KDRV_ISE_TSK_INT_H_ */
