/**
    Ctl SIE Layer

    .

    @file       ctl_sie_id_int.h
    @ingroup    mISYSAlg
    @note       Nothing (or anything need to be mentioned).

    Copyright   Novatek Microelectronics Corp. 2011.  All rights reserved.
*/
#ifndef _CTL_SIE_ID_INT_H_
#define _CTL_SIE_ID_INT_H_

#if defined (__LINUX)
#include <linux/string.h>
#endif
#include "kwrap/flag.h"
#include "kwrap/task.h"
#include "ctl_sie_int.h"
#include "ctl_sie_utility_int.h"

/* define task priority/stack size */
#define CTL_SIE_BUF_TSK_PRI       2
#define CTL_SIE_BUF_TSK_STKSIZE   1024

#define CTL_SIE_ISP_TSK_PRI       3
#define CTL_SIE_ISP_TSK_STKSIZE   1024

// ==================================================================
// ctl sie flag
// ==================================================================
#define CTL_SIE_FLG_LOCK            FLGPTN_BIT(0)
#define CTL_SIE_FLG_END             FLGPTN_BIT(1)
#define CTL_SIE_FLG_CNT             FLGPTN_BIT(2)
#define CTL_SIE_FLG_VD              FLGPTN_BIT(3)
#define CTL_SIE_FLG_TIMEOUT         FLGPTN_BIT(4)
#define CTL_SIE_FLG_CH_SEN_MODE     FLGPTN_BIT(5)
#define CTL_SIE_FLG_KDRV_END        FLGPTN_BIT(6)
#define CTL_SIE_FLG_ISR_PRC_END     FLGPTN_BIT(7)
#define CTL_SIE_FLG_VD_DBG          FLGPTN_BIT(8)
#define CTL_SIE_FLG_CLOSE           FLGPTN_BIT(9)

#define CTL_SIE_FLG_INIT        (CTL_SIE_FLG_LOCK|CTL_SIE_FLG_END|CTL_SIE_FLG_CNT|CTL_SIE_FLG_ISR_PRC_END)

/**********************************************************/
/*              ctl_sie buf task flag pattern             */
/**********************************************************/
#define CTL_SIE_BUF_TASK_PAUSE          FLGPTN_BIT(0)
#define CTL_SIE_BUF_TASK_PAUSE_END      FLGPTN_BIT(1)
#define CTL_SIE_BUF_TASK_RESUME         FLGPTN_BIT(2)
#define CTL_SIE_BUF_TASK_RESUME_END     FLGPTN_BIT(3)
#define CTL_SIE_BUF_TASK_IDLE           FLGPTN_BIT(4)
#define CTL_SIE_BUF_TASK_RESTART        FLGPTN_BIT(5)
#define CTL_SIE_BUF_TASK_FLUSH          FLGPTN_BIT(6)
#define CTL_SIE_BUF_TASK_RES            FLGPTN_BIT(7)
#define CTL_SIE_BUF_TASK_CHK            FLGPTN_BIT(8)
#define CTL_SIE_BUF_QUEUE_PROC          FLGPTN_BIT(9)
#define CTL_SIE_BUF_EXIT_END            FLGPTN_BIT(10)
#define CTL_SIE_BUF_PROC_INIT       (CTL_SIE_BUF_TASK_PAUSE_END)

/**********************************************************/
/*              ctl_sie isp flag pattern                 */
/**********************************************************/
#define CTL_SIE_ISP_TASK_PAUSE          FLGPTN_BIT(0)
#define CTL_SIE_ISP_TASK_PAUSE_END      FLGPTN_BIT(1)
#define CTL_SIE_ISP_TASK_RESUME         FLGPTN_BIT(2)
#define CTL_SIE_ISP_TASK_RESUME_END     FLGPTN_BIT(3)
#define CTL_SIE_ISP_TASK_IDLE           FLGPTN_BIT(4)
#define CTL_SIE_ISP_TASK_RESTART        FLGPTN_BIT(5)
#define CTL_SIE_ISP_TASK_FLUSH          FLGPTN_BIT(6)
#define CTL_SIE_ISP_TASK_RES            FLGPTN_BIT(7)
#define CTL_SIE_ISP_TASK_CHK            FLGPTN_BIT(8)
#define CTL_SIE_ISP_TASK_EXIT_END       FLGPTN_BIT(9)
#define CTL_SIE_ISP_QUEUE_PROC          FLGPTN_BIT(10)
#define CTL_SIE_ISP_CFG_LOCK            FLGPTN_BIT(11)
#define CTL_SIE_ISP_GET_IMG_END         FLGPTN_BIT(12)
#define CTL_SIE_ISP_GET_IMG_READY       FLGPTN_BIT(13)

#define CTL_SIE_ISP_PROC_INIT   (CTL_SIE_ISP_CFG_LOCK | CTL_SIE_ISP_GET_IMG_END|CTL_SIE_ISP_TASK_PAUSE_END)

/*************************************************/
extern THREAD_DECLARE(ctl_sie_buf_tsk, p1);
extern THREAD_DECLARE(ctl_sie_isp_tsk, p1);

extern THREAD_HANDLE g_ctl_sie_buf_tsk_id;
extern ID g_ctl_sie_buf_flg_id;

extern ID g_ctl_sie_flg_id[CTL_SIE_MAX_SUPPORT_ID];
extern void ctl_sie_install_id(void);
extern void ctl_sie_uninstall_id(void);

extern THREAD_HANDLE g_ctl_sie_isp_tsk_id;
extern ID g_ctl_sie_isp_flg_id;
extern void ctl_sie_isp_install_id(void);
extern void ctl_sie_isp_uninstall_id(void);

extern ID ctl_sie_get_flag_id(CTL_SIE_ID id);
extern ID ctl_sie_get_isp_flag_id(void);
extern ID ctl_sie_get_buf_flag_id(void);

#endif //_CTL_SIE_ID_INT_H_
