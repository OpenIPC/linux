#if defined __LINUX
#include <linux/wait.h>
#include <linux/param.h>
#include <linux/sched.h>
#include <linux/interrupt.h>
#include <linux/uaccess.h>
#include <linux/clk.h>
#endif
#include "ctl_sie_drv.h"
#include "ctl_sie_dbg.h"
#include "kdf_sie_int.h"
#include "kflow_videocapture/ctl_sie.h"
#include "ctl_sie_event_id_int.h"
#include "ctl_sie_id_int.h"
#include "ctl_sie_buf_int.h"
#include "ctl_sie_isp_int.h"
#include "ctl_sie_isp_task_int.h"

/*===========================================================================*/
/* Function declaration                                                      */
/*===========================================================================*/

/*===========================================================================*/
/* Define                                                                    */
/*===========================================================================*/
typedef irqreturn_t (*irq_handler_t)(int, void *);

/*===========================================================================*/
/* Global variable                                                           */
/*===========================================================================*/

/*===========================================================================*/
/* Function define                                                           */
/*===========================================================================*/
int nvt_ctl_sie_drv_release(PMODULE_INFO pmodule_info, unsigned char uc_if)
{
	kflow_ctl_sie_uninit();
	return 0;
}

int nvt_ctl_sie_drv_init(MODULE_INFO *pmodule_info)
{
	kflow_ctl_sie_init();
	return 0;
}

int nvt_ctl_sie_drv_remove(MODULE_INFO *pmodule_info)
{
	return 0;
}

int nvt_ctl_sie_drv_suspend(MODULE_INFO *pmodule_info)
{
	ctl_sie_dbg_ind("\n");

	/* Add suspend operation here*/

	return 0;
}

int nvt_ctl_sie_drv_resume(MODULE_INFO *pmodule_info)
{
	ctl_sie_dbg_ind("\n");
	/* Add resume operation here*/

	return 0;
}
