#include <linux/wait.h>
#include <linux/param.h>
#include <linux/sched.h>
#include <linux/interrupt.h>
#include <linux/uaccess.h>
#include <linux/clk.h>
#include "ctl_sen_drv.h"
#include "ctl_sen_ioctl.h"
#include "ctl_sen_dbg.h"
#include "kflow_videocapture/ctl_sen_config.h"

/*===========================================================================*/
/* Function declaration                                                      */
/*===========================================================================*/
int nvt_ctl_sen_drv_wait_cmd_complete(PCTL_SEN_MODULE_INFO pctl_sen_module_info);
int nvt_ctl_sen_drv_ioctl(unsigned char uc_if, CTL_SEN_MODULE_INFO *pctl_sen_module_info, unsigned int cmd, unsigned long arg);
void nvt_ctl_sen_drv_do_tasklet(unsigned long data);
#if 0
irqreturn_t nvt_ctl_sen_drv_isr(int irq, void *devid);
#endif
/*===========================================================================*/
/* Define                                                                    */
/*===========================================================================*/
#if 0
typedef irqreturn_t (*irq_handler_t)(int, void *);
#endif

/*===========================================================================*/
/* Global variable                                                           */
/*===========================================================================*/
int i_event_flag = 0;

/*===========================================================================*/
/* Function define                                                           */
/*===========================================================================*/
int nvt_ctl_sen_drv_open(PCTL_SEN_MODULE_INFO pctl_sen_module_info, unsigned char uc_if)
{
	/* Add HW Moduel initial operation here when the device file opened*/

	return 0;
}


int nvt_ctl_sen_drv_release(PCTL_SEN_MODULE_INFO pctl_sen_module_info, unsigned char uc_if)
{
	/* Add HW Moduel release operation here when device file closed */
	return 0;
}

int nvt_ctl_sen_drv_init(CTL_SEN_MODULE_INFO *pctl_sen_module_info)
{
	return 0;
}

