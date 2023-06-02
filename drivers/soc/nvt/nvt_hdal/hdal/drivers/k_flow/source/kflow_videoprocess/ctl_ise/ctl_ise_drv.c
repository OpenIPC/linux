#if defined(__LINUX)
#include <linux/wait.h>
#include <linux/param.h>
#include <linux/sched.h>
#include <linux/interrupt.h>
#include <linux/uaccess.h>
#include <linux/clk.h>
#elif defined(__FREERTOS)
#endif
#include "ctl_ise_drv.h"
#include "ctl_ise_dbg.h"

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
int nvt_ctl_ise_drv_release(PMODULE_INFO pmodule_info, unsigned char uc_if)
{
	return 0;
}

int nvt_ctl_ise_drv_init(MODULE_INFO *pmodule_info)
{
	return 0;
}

int nvt_ctl_ise_drv_remove(MODULE_INFO *pmodule_info)
{
	return 0;
}

int nvt_ctl_ise_drv_suspend(MODULE_INFO *pmodule_info)
{
	/* Add suspend operation here*/

	return 0;
}

int nvt_ctl_ise_drv_resume(MODULE_INFO *pmodule_info)
{
	/* Add resume operation here*/

	return 0;
}
