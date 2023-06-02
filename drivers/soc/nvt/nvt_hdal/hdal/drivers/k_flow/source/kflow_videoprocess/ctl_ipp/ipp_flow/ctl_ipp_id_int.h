/**
	IPL Ctrl Layer

    .

    @file       ctl_ipp_id_int.h
    @ingroup    mISYSAlg
    @note       Nothing (or anything need to be mentioned).

    Copyright   Novatek Microelectronics Corp. 2011.  All rights reserved.
*/
#ifndef _CTL_IPP_ID_INT_H_
#define _CTL_IPP_ID_INT_H_

#include "kwrap/type.h"
#include "kwrap/flag.h"
#include "kwrap/task.h"

extern void ctl_ipp_install_id(void);
extern void ctl_ipp_uninstall_id(void);

/* define task priority/stack size */
#define CTL_IPP_TSK_PRI           3
#define CTL_IPP_TSK_STKSIZE       4096

#define CTL_IPP_BUF_TSK_PRI       3
#define CTL_IPP_BUF_TSK_STKSIZE   4096

#define CTL_IPP_ISE_TSK_PRI       3
#define CTL_IPP_ISE_TSK_STKSIZE   4096

#define CTL_IPP_ISP_TSK_PRI       3
#define CTL_IPP_ISP_TSK_STKSIZE   4096

#define CTL_IPP_HANDLE_FREE_FLAG_START		(1)

/**********************************************************/
/*              ctl_ipl handle process end flag shift	  */
/**********************************************************/
#define CTL_IPP_RPO_END_USE_BIT 2
#define CTL_IPP_RPO_END_TIMEOUT_SHIFT 0
#define CTL_IPP_RPO_END_SHIFT 1

/**********************************************************/
/*              ctl_ipl task flag pattern	              */
/**********************************************************/
#define CTL_IPP_TASK_PAUSE             FLGPTN_BIT(0)
#define CTL_IPP_TASK_PAUSE_END         FLGPTN_BIT(1)
#define CTL_IPP_TASK_RESUME            FLGPTN_BIT(2)
#define CTL_IPP_TASK_RESUME_END        FLGPTN_BIT(3)
#define CTL_IPP_TASK_IDLE              FLGPTN_BIT(4)
#define CTL_IPP_TASK_RESTART           FLGPTN_BIT(5)
#define CTL_IPP_TASK_FLUSH             FLGPTN_BIT(6)
#define CTL_IPP_TASK_RES               FLGPTN_BIT(7)
#define CTL_IPP_TASK_CHK               FLGPTN_BIT(8)

#define CTL_IPP_TASK_TRIG			  FLGPTN_BIT(9)
#define CTL_IPP_TASK_TRIG_END		  FLGPTN_BIT(10)

#define CTL_IPP_QUEUE_PROC			  FLGPTN_BIT(11)
#define CTL_IPP_TASK_EXIT_END		  FLGPTN_BIT(12)

#define CTL_IPP_TASK_CHK_JOBNUM		  FLGPTN_BIT(13)

#define CTL_IPP_DIR_PAUSE_END		  FLGPTN_BIT(14) 	/* direct only */
#define CTL_IPP_DIR_TIMEOUT		  	  FLGPTN_BIT(15)	/* direct only */

/* protect all task pause/resume */
#define CTL_IPP_TASK_LOCK			FLGPTN_BIT(16)

#define CTL_IPP_PROC_INIT       (CTL_IPP_TASK_PAUSE_END|CTL_IPP_TASK_TRIG_END|CTL_IPP_TASK_LOCK)

/**********************************************************/
/*              ctl_ipl handle flag pattern	              */
/**********************************************************/
#define CTL_IPP_HANDLE_POOL_LOCK FLGPTN_BIT(0)

/**********************************************************/
/*              ctl_ipl buf task flag pattern	          */
/**********************************************************/
#define CTL_IPP_BUF_TASK_PAUSE             FLGPTN_BIT(0)
#define CTL_IPP_BUF_TASK_PAUSE_END         FLGPTN_BIT(1)
#define CTL_IPP_BUF_TASK_RESUME            FLGPTN_BIT(2)
#define CTL_IPP_BUF_TASK_RESUME_END        FLGPTN_BIT(3)
#define CTL_IPP_BUF_TASK_IDLE              FLGPTN_BIT(4)
#define CTL_IPP_BUF_TASK_RESTART           FLGPTN_BIT(5)
#define CTL_IPP_BUF_TASK_FLUSH             FLGPTN_BIT(6)
#define CTL_IPP_BUF_TASK_RES               FLGPTN_BIT(7)
#define CTL_IPP_BUF_TASK_CHK               FLGPTN_BIT(8)

#define CTL_IPP_BUF_TASK_TRIG			  FLGPTN_BIT(9)
#define CTL_IPP_BUF_TASK_TRIG_END		  FLGPTN_BIT(10)

#define CTL_IPP_BUF_QUEUE_PROC			  FLGPTN_BIT(11)
#define CTL_IPP_BUF_TASK_EXIT_END         FLGPTN_BIT(12)


#define CTL_IPP_BUF_PROC_INIT       (CTL_IPP_BUF_TASK_PAUSE_END)

/**********************************************************/
/*              ctl_ipl ise task flag pattern	          */
/**********************************************************/
#define CTL_IPP_ISE_TASK_PAUSE             FLGPTN_BIT(0)
#define CTL_IPP_ISE_TASK_PAUSE_END         FLGPTN_BIT(1)
#define CTL_IPP_ISE_TASK_RESUME            FLGPTN_BIT(2)
#define CTL_IPP_ISE_TASK_RESUME_END        FLGPTN_BIT(3)
#define CTL_IPP_ISE_TASK_IDLE              FLGPTN_BIT(4)
#define CTL_IPP_ISE_TASK_RESTART           FLGPTN_BIT(5)
#define CTL_IPP_ISE_TASK_FLUSH             FLGPTN_BIT(6)
#define CTL_IPP_ISE_TASK_RES               FLGPTN_BIT(7)
#define CTL_IPP_ISE_TASK_CHK               FLGPTN_BIT(8)

#define CTL_IPP_ISE_TASK_TRIG			  FLGPTN_BIT(9)
#define CTL_IPP_ISE_TASK_TRIG_END		  FLGPTN_BIT(10)

#define CTL_IPP_ISE_QUEUE_PROC			  FLGPTN_BIT(11)
#define CTL_IPP_ISE_TASK_EXIT_END         FLGPTN_BIT(12)


#define CTL_IPP_ISE_PROC_INIT       (CTL_IPP_ISE_TASK_PAUSE_END|CTL_IPP_ISE_TASK_TRIG_END)

/**********************************************************/
/*              ctl_ipl isp flag pattern                  */
/**********************************************************/
#define CTL_IPP_ISP_TASK_PAUSE             FLGPTN_BIT(0)
#define CTL_IPP_ISP_TASK_PAUSE_END         FLGPTN_BIT(1)
#define CTL_IPP_ISP_TASK_RESUME            FLGPTN_BIT(2)
#define CTL_IPP_ISP_TASK_RESUME_END        FLGPTN_BIT(3)
#define CTL_IPP_ISP_TASK_IDLE              FLGPTN_BIT(4)
#define CTL_IPP_ISP_TASK_RESTART           FLGPTN_BIT(5)
#define CTL_IPP_ISP_TASK_FLUSH             FLGPTN_BIT(6)
#define CTL_IPP_ISP_TASK_RES               FLGPTN_BIT(7)
#define CTL_IPP_ISP_TASK_CHK               FLGPTN_BIT(8)

#define CTL_IPP_ISP_TASK_TRIG			  FLGPTN_BIT(9)
#define CTL_IPP_ISP_TASK_TRIG_END		  FLGPTN_BIT(10)

#define CTL_IPP_ISP_QUEUE_PROC			  FLGPTN_BIT(11)

#define CTL_IPP_ISP_CFG_LOCK			FLGPTN_BIT(12)
#define CTL_IPP_ISP_GET_YUV_END			FLGPTN_BIT(13)
#define CTL_IPP_ISP_GET_YUV_READY		FLGPTN_BIT(14)
#define CTL_IPP_ISP_GET_YUV_TIMEOUT		FLGPTN_BIT(15)
#define CTL_IPP_ISP_GET_3DNR_STA_READY	FLGPTN_BIT(16)

#define CTL_IPP_ISP_TASK_EXIT_END         FLGPTN_BIT(17)

#define CTL_IPP_ISP_PROC_INIT	(CTL_IPP_ISP_TASK_PAUSE_END | CTL_IPP_ISP_TASK_TRIG_END | CTL_IPP_ISP_CFG_LOCK | CTL_IPP_ISP_GET_YUV_END | CTL_IPP_ISP_GET_3DNR_STA_READY)

/*************************************************/
extern THREAD_DECLARE(ctl_ipp_tsk, p1);
extern THREAD_DECLARE(ctl_ipp_buf_tsk, p1);
extern THREAD_DECLARE(ctl_ipp_ise_tsk, p1);
extern THREAD_DECLARE(ctl_ipp_isp_tsk, p1);

extern THREAD_HANDLE g_ctl_ipp_tsk_id;
extern ID g_ctl_ipp_flg_id;

extern THREAD_HANDLE g_ctl_ipp_buf_tsk_id;
extern ID g_ctl_ipp_buf_flg_id;

extern THREAD_HANDLE g_ctl_ipp_ise_tsk_id;
extern ID g_ctl_ipp_ise_flg_id;

extern THREAD_HANDLE g_ctl_ipp_isp_tsk_id;
extern ID g_ctl_ipp_isp_flg_id;

#endif //_CTL_IPP_ID_INT_H_


