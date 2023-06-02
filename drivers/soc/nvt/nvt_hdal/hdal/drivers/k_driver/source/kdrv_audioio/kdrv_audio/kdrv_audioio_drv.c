#include <linux/wait.h>
#include <linux/param.h>
#include <linux/sched.h>
#include <linux/interrupt.h>
#include <linux/uaccess.h>
#include <linux/clk.h>

#include "kdrv_audioio_drv.h"
#include "kdrv_audioio_dbg.h"

/*===========================================================================*/
/* Function declaration                                                      */
/*===========================================================================*/

/*===========================================================================*/
/* Define                                                                    */
/*===========================================================================*/

/*===========================================================================*/
/* Global variable                                                           */
/*===========================================================================*/

/*===========================================================================*/
/* Function define                                                           */
/*===========================================================================*/




int nvt_kdrv_audioio_drv_init(AUDLIB_MODULE_INFO *pmodule_info)
{
	/* Add HW Module initialization here when driver loaded */

	return 0;
}

int nvt_kdrv_audioio_drv_remove(AUDLIB_MODULE_INFO *pmodule_info)
{
	/* Add HW Moduel release operation here*/

	return 0;
}

