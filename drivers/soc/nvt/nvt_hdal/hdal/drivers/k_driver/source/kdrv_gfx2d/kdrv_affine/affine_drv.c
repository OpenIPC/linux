#include <linux/wait.h>
#include <linux/param.h>
#include <linux/sched.h>
#include <linux/interrupt.h>
#include <linux/uaccess.h>
#include <linux/clk.h>
#include "affine_drv.h"
#include "affine_ioctl.h"
#include "affine_dbg.h"
#include "affine_platform.h"

/*===========================================================================*/
/* Function declaration                                                      */
/*===========================================================================*/
extern void affine_platform_init(MODULE_INFO *pmodule_info);

int nvt_affine_drv_ioctl(unsigned char if_id, MODULE_INFO *pmodule_info, unsigned int cmd, unsigned long arg);
void nvt_affine_drv_do_tasklet(unsigned long data);
irqreturn_t nvt_affine_drv_isr(int irq, void *devid);
/*===========================================================================*/
/* Define                                                                    */
/*===========================================================================*/
typedef irqreturn_t (*irq_handler_t)(int, void *);

/*===========================================================================*/
/* Global variable                                                           */
/*===========================================================================*/
//int iEventFlag = 0;

/*===========================================================================*/
/* Function define                                                           */
/*===========================================================================*/
int nvt_affine_drv_open(PMODULE_INFO pmodule_info, unsigned char if_id)
{
	nvt_dbg(IND, "%d\n", if_id);

	/* Add HW Moduel initial operation here when the device file opened*/

	return 0;
}


int nvt_affine_drv_release(PMODULE_INFO pmodule_info, unsigned char if_id)
{
	nvt_dbg(IND, "%d\n", if_id);

	/* Add HW Moduel release operation here when device file closed */

	return 0;
}

int nvt_affine_drv_init(MODULE_INFO *pmodule_info)
{
	int err = 0;
	struct clk *pclk;

	tasklet_init(&pmodule_info->affine_tasklet[0],
			nvt_affine_drv_do_tasklet,
			(unsigned long)pmodule_info);

	/* allocate affine resource here */
	affine_platform_init(pmodule_info);

	pclk = clk_get(NULL, "fix240m");
	if (IS_ERR(pclk)) {
		printk("%s: get source 240 fail\r\n", __func__);
	}

	clk_set_parent(pmodule_info->pclk[0], pclk);

	clk_prepare(pmodule_info->pclk[0]);

	clk_put(pclk);

	/* register IRQ here*/
	if (request_irq(pmodule_info->iinterrupt_id[0], nvt_affine_drv_isr, IRQF_TRIGGER_HIGH, "AFFINE_INT", pmodule_info)) {
		nvt_dbg(ERR, "failed to register an IRQ Int:%d\n", pmodule_info->iinterrupt_id[0]);
		err = -ENODEV;
		goto FAIL_FREE_IRQ;
	}

	printk("%s: done\n", __func__);

	return err;

FAIL_FREE_IRQ:
	free_irq(pmodule_info->iinterrupt_id[0], pmodule_info);

	/* Add error handler here */

	return err;
}

int nvt_affine_drv_remove(MODULE_INFO *pmodule_info)
{
	//Free IRQ
	free_irq(pmodule_info->iinterrupt_id[0], pmodule_info);

	/* Add HW Moduel release operation here*/

	/* release OS resources */
	affine_platform_uninit();

	return 0;
}

int nvt_affine_drv_suspend(MODULE_INFO *pmodule_info)
{
	nvt_dbg(IND, "\n");

	/* Add suspend operation here*/

	return 0;
}

int nvt_affine_drv_resume(MODULE_INFO *pmodule_info)
{
	nvt_dbg(IND, "\n");
	/* Add resume operation here*/

	return 0;
}

int nvt_affine_drv_ioctl(unsigned char if_id, MODULE_INFO *pmodule_info, unsigned int cmd, unsigned long argc)
{
//	REG_INFO reg_info;
//	REG_INFO_LIST reg_info_list;
//	int loop_count;
	int err = 0;

	nvt_dbg(IND, "IF-%d cmd:%x\n", if_id, cmd);



	switch (cmd) {
	case AFFINE_IOC_START:
		/*call someone to start operation*/
		break;

	case AFFINE_IOC_STOP:
		/*call someone to stop operation*/
		break;

#if 0
	case AFFINE_IOC_READ_REG:
		err = copy_from_user(&reg_info, (void __user *)argc, sizeof(REG_INFO));
		if (!err) {
			reg_info.reg_value = READ_REG(pmodule_info->io_addr[if_id] + reg_info.reg_addr);
			err = copy_to_user((void __user *)argc, &reg_info, sizeof(REG_INFO));
		}
		break;

	case AFFINE_IOC_WRITE_REG:
		err = copy_from_user(&reg_info, (void __user *)argc, sizeof(REG_INFO));
		if (!err)
			WRITE_REG(reg_info.reg_value, pmodule_info->io_addr[if_id] + reg_info.reg_addr);
		break;

	case AFFINE_IOC_READ_REG_LIST:
		err = copy_from_user(&reg_info_list, (void __user *)argc, sizeof(REG_INFO_LIST));
		if (!err) {
			for (loop_count = 0 ; loop_count < reg_info_list.reg_cnt; loop_count++)
				reg_info_list.reg_list[loop_count].reg_value = READ_REG(pmodule_info->io_addr[if_id] + reg_info_list.reg_list[loop_count].reg_addr);

			err = copy_to_user((void __user *)argc, &reg_info_list, sizeof(REG_INFO_LIST));
		}
		break;
	case AFFINE_IOC_WRITE_REG_LIST:
		err = copy_from_user(&reg_info_list, (void __user *)argc, sizeof(REG_INFO_LIST));
		if (!err)
			for (loop_count = 0 ; loop_count < reg_info_list.reg_cnt ; loop_count++)
				WRITE_REG(reg_info_list.reg_list[loop_count].reg_value, pmodule_info->io_addr[if_id] + reg_info_list.reg_list[loop_count].reg_addr);
		break;
#endif
	/* Add other operations here */
	}

	return err;
}

irqreturn_t nvt_affine_drv_isr(int irq, void *devid)
{
	affine_isr();
	return IRQ_HANDLED;
}

#if 0
int nvt_affine_drv_write_reg(PMODULE_INFO pmodule_info, unsigned long addr, unsigned long value)
{
	WRITE_REG(value, pmodule_info->io_addr[0] + addr);
	return 0;
}
#endif

#if 0
int nvt_affine_drv_read_reg(PMODULE_INFO pmodule_info, unsigned long addr)
{
	return READ_REG(pmodule_info->io_addr[0] + addr);
}
#endif

void nvt_affine_drv_do_tasklet(unsigned long data)
{
	affine_platform_ist(AFFINE_ID_1, data);
}
