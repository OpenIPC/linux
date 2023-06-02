#include <linux/wait.h>
#include <linux/param.h>
#include <linux/sched.h>
#include <linux/interrupt.h>
#include <linux/uaccess.h>
#include <linux/clk.h>
#include "kwrap/type.h"
#include "tge_lib.h"
#include "tge_int.h"
#include "tge_platform.h"
#include "kdrv_tge_int_drv.h"
#include "kdrv_tge_int_ioctl.h"
#include "kdrv_tge_int_dbg.h"


/*===========================================================================*/
/* Function declaration                                                      */
/*===========================================================================*/
int nvt_tge_drv_wait_cmd_complete(PTGE_MODULE_INFO pmodule_info);
int nvt_tge_drv_ioctl(unsigned char ucIF, TGE_MODULE_INFO* pmodule_info, unsigned int cmd, unsigned long arg);
void nvt_tge_drv_do_tasklet(unsigned long data);
irqreturn_t nvt_tge_drv_isr(int irq, void *devid);
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
int nvt_tge_drv_open(PTGE_MODULE_INFO pmodule_info, unsigned char ucIF)
{
	nvt_dbg(IND, "%d\n", ucIF);

	/* Add HW Moduel initial operation here when the device file opened*/

	return 0;
}


int nvt_tge_drv_release(PTGE_MODULE_INFO pmodule_info, unsigned char ucIF)
{
	nvt_dbg(IND, "%d\n", ucIF);

	/* Add HW Moduel release operation here when device file closed */

	return 0;
}

int nvt_tge_drv_init(TGE_MODULE_INFO* pmodule_info)
{
	int iRet = 0;

	init_waitqueue_head(&pmodule_info->tge_wait_queue);
	// coverity[side_effect_free]
	spin_lock_init(&pmodule_info->tge_spinlock);
	sema_init(&pmodule_info->tge_sem, 1);

	tge_platform_create_resource(pmodule_info);
	tge_platform_prepare_clk();

	/* initial clock here */
	//clk_prepare(pmodule_info->pclk[0]);
	//clk_enable(pmodule_info->pclk[0]);
	//clk_set_rate(pmodule_info->pclk[0], 240000000);

	/* register IRQ here*/
	if(request_irq(pmodule_info->iinterrupt_id[0], nvt_tge_drv_isr, IRQF_TRIGGER_HIGH, "TGE_INT", pmodule_info)) {
		nvt_dbg(ERR, "failed to register an IRQ Int:%d\n", pmodule_info->iinterrupt_id[0]);
		iRet = -ENODEV;
		goto FAIL_FREE_IRQ;
	}

	/* Add HW Module initialization here when driver loaded */

	return iRet;

FAIL_FREE_IRQ:

	free_irq(pmodule_info->iinterrupt_id[0], pmodule_info);

	/* Add error handler here */

	return iRet;
}
int nvt_tge_drv_remove(TGE_MODULE_INFO* pmodule_info)
{

	//Free IRQ
	free_irq(pmodule_info->iinterrupt_id[0], pmodule_info);

	/* Add HW Moduel release operation here*/
	tge_platform_release_resource();

	return 0;
}

int nvt_tge_drv_suspend(TGE_MODULE_INFO* pmodule_info)
{
	nvt_dbg(IND, "\n");

	/* Add suspend operation here*/

	return 0;
}

int nvt_tge_drv_resume(TGE_MODULE_INFO* pmodule_info)
{
	nvt_dbg(IND, "\n");
	/* Add resume operation here*/

	return 0;
}

int nvt_tge_drv_ioctl(unsigned char ucIF, TGE_MODULE_INFO* pmodule_info, unsigned int uiCmd, unsigned long ulArg)
{
	REG_INFO reg_info;
	REG_INFO_LIST reg_info_list;
	int iLoop;
	int iRet = 0;

	nvt_dbg(IND, "IF-%d cmd:%x\n", ucIF, uiCmd);

	switch(uiCmd) {
		case TGE_IOC_START:
			/*call someone to start operation*/
			break;

		case TGE_IOC_STOP:
			/*call someone to stop operation*/
			break;

		case TGE_IOC_READ_REG:
			iRet = copy_from_user(&reg_info, (void __user *)ulArg, sizeof(REG_INFO));
	        if(!iRet) {
		        reg_info.uiValue = READ_REG(pmodule_info->io_addr[ucIF] + reg_info.uiAddr);
        	 	iRet = copy_to_user((void __user *)ulArg, &reg_info, sizeof(REG_INFO));
			}
			break;

		case TGE_IOC_WRITE_REG:
			iRet = copy_from_user(&reg_info, (void __user *)ulArg, sizeof(REG_INFO));
	        if(!iRet)
				WRITE_REG(reg_info.uiValue, pmodule_info->io_addr[ucIF] + reg_info.uiAddr);
			break;

		case TGE_IOC_READ_REG_LIST:
			iRet = copy_from_user(&reg_info_list, (void __user *)ulArg, sizeof(REG_INFO_LIST));
	        if(!iRet) {
				if (reg_info_list.uiCount <= MODULE_REG_LIST_NUM) {
					for(iLoop = 0 ; iLoop < reg_info_list.uiCount; iLoop++) {
				        reg_info_list.RegList[iLoop].uiValue = READ_REG(pmodule_info->io_addr[ucIF] + reg_info_list.RegList[iLoop].uiAddr);
					}
				} else {
					DBG_ERR("Loop bound error!\r\n");
				}
        	 	iRet = copy_to_user((void __user *)ulArg, &reg_info_list, sizeof(REG_INFO_LIST));
			}
			break;
		case TGE_IOC_WRITE_REG_LIST:
			iRet = copy_from_user(&reg_info_list, (void __user *)ulArg, sizeof(REG_INFO_LIST));
	        if(!iRet) {
				if (reg_info_list.uiCount <= MODULE_REG_LIST_NUM) {
					for(iLoop = 0 ; iLoop < reg_info_list.uiCount ; iLoop++) {
						WRITE_REG(reg_info_list.RegList[iLoop].uiValue, pmodule_info->io_addr[ucIF] + reg_info_list.RegList[iLoop].uiAddr);
					}
				} else {
					DBG_ERR("Loop bound error!\r\n");
				}
	        }
			break;

		/* Add other operations here */
	}

	return iRet;
}

irqreturn_t nvt_tge_drv_isr(int irq, void *devid)
{
	tge_isr();
    return IRQ_HANDLED;
}

int nvt_tge_drv_write_reg(PTGE_MODULE_INFO pmodule_info, unsigned long addr, unsigned long value)
{
	WRITE_REG(value, pmodule_info->io_addr[0] + addr);
	return 0;
}

int nvt_tge_drv_read_reg(PTGE_MODULE_INFO pmodule_info, unsigned long addr)
{
	return READ_REG(pmodule_info->io_addr[0] + addr);
}


