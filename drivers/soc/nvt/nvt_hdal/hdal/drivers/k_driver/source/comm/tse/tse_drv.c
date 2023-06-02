#include <linux/wait.h>
#include <linux/param.h>
#include <linux/sched.h>
#include <linux/interrupt.h>
#include <linux/uaccess.h>
#include <linux/clk.h>
#include "tse_drv.h"
#include "tse_dbg.h"
#include "tse_platform_int.h"

/*===========================================================================*/
/* Function declaration                                                      */
/*===========================================================================*/
int nvt_tse_drv_ioctl(unsigned char if_id, MODULE_INFO *pmodule_info, unsigned int cmd, unsigned long arg);
irqreturn_t nvt_tse_drv_isr(int irq, void *devid);
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
int nvt_tse_drv_open(PMODULE_INFO pmodule_info, unsigned char if_id)
{
	DBG_IND("%d\n", if_id);

	/* Add HW Moduel initial operation here when the device file opened*/

	return 0;
}


int nvt_tse_drv_release(PMODULE_INFO pmodule_info, unsigned char if_id)
{
	DBG_IND("%d\n", if_id);

	/* Add HW Moduel release operation here when device file closed */

	return 0;
}

int nvt_tse_drv_init(MODULE_INFO *pmodule_info)
{
	int err = 0;

	DBG_IND("\n");

	/* initial clock here */
	clk_prepare(pmodule_info->pclk[0]);

	/* register IRQ here*/
	if (request_irq(pmodule_info->interrupt_id[0], nvt_tse_drv_isr, IRQF_TRIGGER_HIGH, "TSE_INT", pmodule_info)) {
		DBG_ERR("failed to register an IRQ Int:%d\n", pmodule_info->interrupt_id[0]);
		err = -ENODEV;
		goto FAIL_FREE_IRQ;
	}

	/* Add HW Module initialization here when driver loaded */
	tse_platform_set_resource(pmodule_info);

	return err;

FAIL_FREE_IRQ:

	free_irq(pmodule_info->interrupt_id[0], pmodule_info);

	/* Add error handler here */
	return err;
}

int nvt_tse_drv_remove(MODULE_INFO *pmodule_info)
{
	DBG_IND("\n");

	//Free IRQ
	free_irq(pmodule_info->interrupt_id[0], pmodule_info);

	/* Add HW Moduel release operation here*/
	tse_platform_release_resource();

	/* uninitial clock here */
	clk_unprepare(pmodule_info->pclk[0]);

	return 0;
}

int nvt_tse_drv_suspend(MODULE_INFO *pmodule_info)
{
	DBG_IND("\n");

	/* Add suspend operation here*/

	return 0;
}

int nvt_tse_drv_resume(MODULE_INFO *pmodule_info)
{
	DBG_IND("\n");
	/* Add resume operation here*/

	return 0;
}

irqreturn_t nvt_tse_drv_isr(int irq, void *devid)
{
	tse_platform_isr();
	return IRQ_HANDLED;
}
