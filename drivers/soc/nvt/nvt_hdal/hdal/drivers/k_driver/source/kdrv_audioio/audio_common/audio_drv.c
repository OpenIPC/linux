#include <linux/wait.h>
#include <linux/param.h>
#include <linux/sched.h>
#include <linux/interrupt.h>
#include <linux/uaccess.h>
#include <linux/clk.h>
#include "audio_drv.h"
#include "audio_ioctl.h"
#include "audio_dbg.h"

/*===========================================================================*/
/* Function declaration                                                      */
/*===========================================================================*/
int nvt_audio_drv_wait_cmd_complete(PMODULE_INFO pmodule_info);
int nvt_audio_drv_ioctl(unsigned char ucIF, MODULE_INFO* pmodule_info, unsigned int cmd, unsigned long arg);
void nvt_audio_drv_do_tasklet(unsigned long data);
irqreturn_t nvt_audio_drv_isr(int irq, void *devid);
/*===========================================================================*/
/* Define                                                                    */
/*===========================================================================*/
typedef irqreturn_t (*irq_handler_t)(int, void *);

/*===========================================================================*/
/* Global variable                                                           */
/*===========================================================================*/
int iEventFlag = 0;

/*===========================================================================*/
/* Function define                                                           */
/*===========================================================================*/
int nvt_audio_drv_open(PMODULE_INFO pmodule_info, unsigned char ucIF)
{
	nvt_dbg(IND, "%d\n", ucIF);

	/* Add HW Moduel initial operation here when the device file opened*/

	return 0;
}


int nvt_audio_drv_release(PMODULE_INFO pmodule_info, unsigned char ucIF)
{
	nvt_dbg(IND, "%d\n", ucIF);

	/* Add HW Moduel release operation here when device file closed */

	return 0;
}

int nvt_audio_drv_init(MODULE_INFO* pmodule_info)
{
	int iRet = 0;
#if 0
	init_waitqueue_head(&pmodule_info->xxx_wait_queue);
	spin_lock_init(&pmodule_info->xxx_spinlock);
	sema_init(&pmodule_info->xxx_sem, 1);
	init_completion(&pmodule_info->xxx_completion);
	tasklet_init(&pmodule_info->xxx_tasklet, nvt_xxx_drv_do_tasklet, (unsigned long)pmodule_info);
#endif
	/* initial clock here */



	/* register IRQ here*/
#if 0
	if(request_irq(pmodule_info->iinterrupt_id[0], nvt_xxx_drv_isr, IRQF_TRIGGER_RISING, "XXX_INT", pmodule_info)) {
		nvt_dbg(ERR, "failed to register an IRQ Int:%d\n", pmodule_info->iinterrupt_id[0]);
		iRet = -ENODEV;
		goto FAIL_FREE_IRQ;
	}
#endif

	/* Add HW Module initialization here when driver loaded */

	return iRet;
/*
FAIL_FREE_IRQ:

	free_irq(pmodule_info->iinterrupt_id[0], pmodule_info);

	// Add error handler here

	return iRet;*/
}

int nvt_audio_drv_remove(MODULE_INFO* pmodule_info)
{

	//Free IRQ
	//free_irq(pmodule_info->iinterrupt_id[0], pmodule_info);

	/* Add HW Moduel release operation here*/

	return 0;
}

int nvt_audio_drv_suspend(MODULE_INFO* pmodule_info)
{
	nvt_dbg(IND, "\n");

	/* Add suspend operation here*/

	return 0;
}

int nvt_audio_drv_resume(MODULE_INFO* pmodule_info)
{
	nvt_dbg(IND, "\n");
	/* Add resume operation here*/

	return 0;
}

int nvt_audio_drv_ioctl(unsigned char ucIF, MODULE_INFO* pmodule_info, unsigned int uiCmd, unsigned long ulArg)
{
#if 0
	REG_INFO reg_info;
	REG_INFO_LIST reg_info_list;
	int iLoop;
#endif
	int iRet = 0;

	nvt_dbg(IND, "IF-%d cmd:%x\n", ucIF, uiCmd);



	switch(uiCmd) {
		case AUDIO_IOC_START:
			/*call someone to start operation*/
			break;

		case AUDIO_IOC_STOP:
			/*call someone to stop operation*/
			break;
#if 0
		case XXX_IOC_READ_REG:
			iRet = copy_from_user(&reg_info, (void __user *)ulArg, sizeof(REG_INFO));
	        if(!iRet) {
		        reg_info.uiValue = READ_REG(pmodule_info->io_addr[ucIF] + reg_info.uiAddr);
        	 	iRet = copy_to_user((void __user *)ulArg, &reg_info, sizeof(REG_INFO));
			}
			break;

		case XXX_IOC_WRITE_REG:
			iRet = copy_from_user(&reg_info, (void __user *)ulArg, sizeof(REG_INFO));
	        if(!iRet)
				WRITE_REG(reg_info.uiValue, pmodule_info->io_addr[ucIF] + reg_info.uiAddr);
			break;

		case XXX_IOC_READ_REG_LIST:
			iRet = copy_from_user(&reg_info_list, (void __user *)ulArg, sizeof(REG_INFO_LIST));
	        if(!iRet) {
				for(iLoop = 0 ; iLoop < reg_info_list.uiCount; iLoop++)
			        reg_info_list.RegList[iLoop].uiValue = READ_REG(pmodule_info->io_addr[ucIF] + reg_info_list.RegList[iLoop].uiAddr);

        	 	iRet = copy_to_user((void __user *)ulArg, &reg_info_list, sizeof(REG_INFO_LIST));
			}
			break;
		case XXX_IOC_WRITE_REG_LIST:
			iRet = copy_from_user(&reg_info_list, (void __user *)ulArg, sizeof(REG_INFO_LIST));
	        if(!iRet)
				for(iLoop = 0 ; iLoop < reg_info_list.uiCount ; iLoop++)
					WRITE_REG(reg_info_list.RegList[iLoop].uiValue, pmodule_info->io_addr[ucIF] + reg_info_list.RegList[iLoop].uiAddr);
			break;
#endif

		/* Add other operations here */
	}

	return iRet;
}

irqreturn_t nvt_audio_drv_isr(int irq, void *devid)
{
	//PMODULE_INFO pmodule_info = (PMODULE_INFO)devid;

	/* simple triggle and response mechanism*/
	//complete(&pmodule_info->aduio_completion);


	/*  Tasklet for bottom half mechanism */
	//tasklet_schedule(&pmodule_info->audio_tasklet);

    return IRQ_HANDLED;
}

int nvt_audio_drv_wait_cmd_complete(PMODULE_INFO pmodule_info)
{
	//wait_for_completion(&pmodule_info->audio_completion);
	return 0;
}

int nvt_audio_drv_write_reg(PMODULE_INFO pmodule_info, unsigned long addr, unsigned long value)
{
//	WRITE_REG(value, pmodule_info->io_addr[0] + addr);
	return 0;
}

int nvt_audio_drv_read_reg(PMODULE_INFO pmodule_info, unsigned long addr)
{
//	return READ_REG(pmodule_info->io_addr[0] + addr);
    return 0;
}

void nvt_audio_drv_do_tasklet(unsigned long data)
{
	//PMODULE_INFO pmodule_info = (PMODULE_INFO)data;
	nvt_dbg(IND, "\n");

	/* do something you want*/
	//complete(&pmodule_info->audio_completion);
}
