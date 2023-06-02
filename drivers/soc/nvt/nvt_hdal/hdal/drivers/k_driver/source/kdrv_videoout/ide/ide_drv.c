#include <linux/wait.h>
#include <linux/param.h>
#include <linux/sched.h>
#include <linux/interrupt.h>
#include <linux/uaccess.h>
#include <linux/clk.h>
#include "ide_drv.h"
#include "ide_ioctl.h"
#include "ide_dbg.h"
#include "ide2_int.h"

/*===========================================================================*/
/* Function declaration                                                      */
/*===========================================================================*/
int nvt_ide_drv_wait_cmd_complete(PMODULE_INFO pmodule_info);
int nvt_ide_drv_ioctl(unsigned char uc_if, MODULE_INFO *pmodule_info, unsigned int cmd, unsigned long arg);
void nvt_ide_drv_do_tasklet(unsigned long data);
void nvt_ide_drv_do_tasklet2(unsigned long data);
irqreturn_t nvt_ide_drv_isr(int irq, void *dev_id);
irqreturn_t nvt_ide2_drv_isr(int irq, void *dev_id);
/*===========================================================================*/
/* Define                                                                    */
/*===========================================================================*/
typedef irqreturn_t (*irq_handler_t)(int, void *);

/*===========================================================================*/
/* Global variable                                                           */
/*===========================================================================*/
int i_event_flag = 0;

/*===========================================================================*/
/* Function define                                                           */
/*===========================================================================*/
int nvt_ide_drv_open(PMODULE_INFO pmodule_info, unsigned char uc_if)
{
	nvt_dbg(IND, "%d\n", uc_if);

	/* Add HW Moduel initial operation here when the device file opened*/

	return 0;
}


int nvt_ide_drv_release(PMODULE_INFO pmodule_info, unsigned char uc_if)
{
	nvt_dbg(IND, "%d\n", uc_if);

	/* Add HW Moduel release operation here when device file closed */

	return 0;
}

int nvt_ide_drv_init(MODULE_INFO *pmodule_info)
{
	int i_ret = 0;

	init_waitqueue_head(&pmodule_info->ide_wait_queue);
	spin_lock_init(&pmodule_info->ide_spinlock);
	sema_init(&pmodule_info->ide_sem, 1);
	init_completion(&pmodule_info->ide_completion);
	tasklet_init(&pmodule_info->ide_tasklet[0], nvt_ide_drv_do_tasklet, (unsigned long)pmodule_info);

	/* initial clock here */



	/* register IRQ here*/
	if (request_irq(pmodule_info->iinterrupt_id[0], nvt_ide_drv_isr, IRQF_TRIGGER_HIGH, "IDE_INT", pmodule_info)) {
		nvt_dbg(ERR, "failed to register an IRQ Int:%d\n", pmodule_info->iinterrupt_id[0]);
		i_ret = -ENODEV;
		goto FAIL_FREE_IRQ;
	}


	/* Add HW Module initialization here when driver loaded */

	ide_platform_create_resource(pmodule_info);

	return i_ret;

FAIL_FREE_IRQ:

	free_irq(pmodule_info->iinterrupt_id[0], pmodule_info);

	/* Add error handler here */

	return i_ret;
}

int nvt_ide_drv_remove(MODULE_INFO *pmodule_info)
{

	//Free IRQ
	free_irq(pmodule_info->iinterrupt_id[0], pmodule_info);

	ide_platform_release_resource();

	/* Add HW Moduel release operation here*/

	return 0;
}

int nvt_ide_drv_suspend(MODULE_INFO *pmodule_info)
{
	nvt_dbg(IND, "\n");

	/* Add suspend operation here*/

	return 0;
}

int nvt_ide_drv_resume(MODULE_INFO *pmodule_info)
{
	nvt_dbg(IND, "\n");
	/* Add resume operation here*/

	return 0;
}

int nvt_ide_drv_ioctl(unsigned char uc_if, MODULE_INFO *pmodule_info, unsigned int ui_cmd, unsigned long ul_arg)
{
	//REG_INFO reg_info;
	//REG_INFO_LIST reg_info_list;
	//int i_loop;
	int i_ret = 0;

	nvt_dbg(IND, "IF-%d cmd:%x\n", uc_if, ui_cmd);



	switch (ui_cmd) {
	case IDE_IOC_START:
		/*call someone to start operation*/
		break;

	case IDE_IOC_STOP:
		/*call someone to stop operation*/
		break;
#if 0
	case IDE_IOC_READ_REG:
		i_ret = copy_from_user(&reg_info, (void __user *)ul_arg, sizeof(REG_INFO));
		if (!i_ret) {
			reg_info.ui_value = READ_REG(pmodule_info->io_addr[uc_if] + reg_info.ui_addr);
			i_ret = copy_to_user((void __user *)ul_arg, &reg_info, sizeof(REG_INFO));
		}
		break;

	case IDE_IOC_WRITE_REG:
		i_ret = copy_from_user(&reg_info, (void __user *)ul_arg, sizeof(REG_INFO));
		if (!i_ret) {
			WRITE_REG(reg_info.ui_value, pmodule_info->io_addr[uc_if] + reg_info.ui_addr);
		}
		break;

	case IDE_IOC_READ_REG_LIST:
		i_ret = copy_from_user(&reg_info_list, (void __user *)ul_arg, sizeof(REG_INFO_LIST));
		if (!i_ret) {
			for (i_loop = 0 ; i_loop < reg_info_list.ui_count; i_loop++) {
				reg_info_list.reg_list[i_loop].ui_value = READ_REG(pmodule_info->io_addr[uc_if] + reg_info_list.reg_list[i_loop].ui_addr);
			}

			i_ret = copy_to_user((void __user *)ul_arg, &reg_info_list, sizeof(REG_INFO_LIST));
		}
		break;
	case IDE_IOC_WRITE_REG_LIST:
		i_ret = copy_from_user(&reg_info_list, (void __user *)ul_arg, sizeof(REG_INFO_LIST));
		if (!i_ret)
			for (i_loop = 0 ; i_loop < reg_info_list.ui_count ; i_loop++) {
				WRITE_REG(reg_info_list.reg_list[i_loop].ui_value, pmodule_info->io_addr[uc_if] + reg_info_list.reg_list[i_loop].ui_addr);
			}
		break;
#endif
		/* Add other operations here */
	}

	return i_ret;
}

irqreturn_t nvt_ide_drv_isr(int irq, void *dev_id)
{
	//PMODULE_INFO pmodule_info = (PMODULE_INFO)dev_id;

	/* simple triggle and response mechanism*/
	//complete(&pmodule_info->ide_completion);


	/*  Tasklet for bottom half mechanism */
	//tasklet_schedule(&pmodule_info->ide_tasklet);
	ide_isr();


	return IRQ_HANDLED;
}

int nvt_ide_drv_wait_cmd_complete(PMODULE_INFO pmodule_info)
{
	wait_for_completion(&pmodule_info->ide_completion);
	return 0;
}

int nvt_ide_drv_write_reg(PMODULE_INFO pmodule_info, unsigned long addr, unsigned long value)
{
	WRITE_REG(value, pmodule_info->io_addr[0] + addr);
	return 0;
}

int nvt_ide_drv_read_reg(PMODULE_INFO pmodule_info, unsigned long addr)
{
	return READ_REG(pmodule_info->io_addr[0] + addr);
}

void nvt_ide_drv_dump(PMODULE_INFO pmodule_info, unsigned int id)
{
	//idec_dump_info(id);
}

void nvt_ide_drv_do_tasklet(unsigned long data)
{
	ide_platform_ist(IDE_ID_1, data);
}
