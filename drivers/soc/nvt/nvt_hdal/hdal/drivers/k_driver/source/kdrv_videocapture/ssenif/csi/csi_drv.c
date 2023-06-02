#include <linux/wait.h>
#include <linux/param.h>
#include <linux/sched.h>
#include <linux/interrupt.h>
#include <linux/uaccess.h>
#include <linux/clk.h>
#include "csi_drv.h"
#include "csi_ioctl.h"
#include "csi_dbg.h"
#include "../csi.h"

/*===========================================================================*/
/* Function declaration                                                      */
/*===========================================================================*/
int nvt_csi_drv_wait_cmd_complete(PMODULE_INFO pmodule_info);
int nvt_csi_drv_ioctl(unsigned char uc_if, MODULE_INFO *pmodule_info, unsigned int ui_cmd, unsigned long ul_arg);
void nvt_csi_drv_do_tasklet(unsigned long data);
irqreturn_t nvt_csi_drv_isr(int irq, void *devid);
/*===========================================================================*/
/* Define                                                                    */
/*===========================================================================*/
typedef irqreturn_t (*irq_handler_t)(int, void *);

/*===========================================================================*/
/* Global variable                                                           */
/*===========================================================================*/
int g_event_flag = 0;
PCSIOBJ csiobj[MODULE_REG_NUM];
PMODULE_INFO pcsi_mod_info;

/*===========================================================================*/
/* Function define                                                           */
/*===========================================================================*/
int nvt_csi_drv_open(PMODULE_INFO pmodule_info, unsigned char uc_if)
{
	nvt_dbg(IND, "%d\n", uc_if);

	/* Add HW Moduel initial operation here when the device file opened*/

	return 0;
}


int nvt_csi_drv_release(PMODULE_INFO pmodule_info, unsigned char uc_if)
{
	nvt_dbg(IND, "%d\n", uc_if);

	/* Add HW Moduel release operation here when device file closed */

	return 0;
}

int nvt_csi_drv_init(MODULE_INFO *pmodule_info/*, struct platform_device *pdev, unsigned char uc_if*/)
{
	int i_ret = 0;
	unsigned char ucloop;
	struct clk *parent_clk;

	init_waitqueue_head(&pmodule_info->csi_wait_queue);
	spin_lock_init(&pmodule_info->csi_spinlock);
	sema_init(&pmodule_info->csi_sem, 1);
	init_completion(&pmodule_info->csi_completion);
	tasklet_init(&pmodule_info->csi_tasklet, nvt_csi_drv_do_tasklet, (unsigned long)pmodule_info);

	/* initial clock here */
	for (ucloop = 0 ; ucloop < MODULE_CLK_NUM; ucloop++) {
		clk_prepare(pmodule_info->pclk[ucloop]);

		if (pmodule_info->fast_boot != 1) {
#if defined(CONFIG_NVT_FPGA_EMULATION)
			parent_clk = clk_get(NULL, "fix120m");
#else
			parent_clk = clk_get(NULL, "fix60m");
#endif
			if (!IS_ERR(parent_clk)) {
				clk_set_parent(pmodule_info->pclk[ucloop], parent_clk);
				clk_put(parent_clk);
			} else {
				nvt_dbg(ERR, "Get fix60m ERR\n");
			}
		}
	}

	/* Add HW Module initialization here when driver loaded */
	csi_create_resource();
	csi_set_base_addr((UINT32)pmodule_info->io_addr[0]);

	csi2_create_resource();
	csi2_set_base_addr((UINT32)pmodule_info->io_addr[1]);

	if (pmodule_info->fast_boot == 1) {
		csi_set_fastboot();
		csi2_set_fastboot();
	}

	/* register IRQ here*/
	for (ucloop = 0 ; ucloop < MODULE_IRQ_NUM; ucloop++) {
		if (request_irq(pmodule_info->iinterrupt_id[ucloop], nvt_csi_drv_isr, IRQF_SHARED | IRQF_TRIGGER_HIGH, "CSI_INT"/*pdev->name*/, pmodule_info)) {
			nvt_dbg(ERR, "failed to register an IRQ Int:%d\n", pmodule_info->iinterrupt_id[ucloop]);
			i_ret = -ENODEV;
			goto FAIL_FREE_IRQ;
		}
	}

	pcsi_mod_info = pmodule_info;

	return i_ret;

FAIL_FREE_IRQ:

	free_irq(pmodule_info->iinterrupt_id[ucloop], pmodule_info);

	/* Add error handler here */

	return i_ret;
}

int nvt_csi_drv_remove(MODULE_INFO *pmodule_info)
{
	unsigned char ucloop;

	nvt_dbg(IND, "\n");

	//Free IRQ
	for (ucloop = 0 ; ucloop < MODULE_IRQ_NUM; ucloop++) {
		/* Free IRQ */
		free_irq(pmodule_info->iinterrupt_id[ucloop], pmodule_info);

		//if (!(IS_ERR(pmodule_info->pclk[ucloop]))) {
			/* disable clock */
			//clk_disable(pmodule_info->pclk[ucloop]);
		//}
	}

	/* Add HW Moduel release operation here*/
	csi_release_resource();
	csi2_release_resource();

	return 0;
}

int nvt_csi_drv_suspend(MODULE_INFO *pmodule_info)
{
	nvt_dbg(IND, "\n");

	/* Add suspend operation here*/

	return 0;
}

int nvt_csi_drv_resume(MODULE_INFO *pmodule_info)
{
	nvt_dbg(IND, "\n");
	/* Add resume operation here*/

	return 0;
}

int nvt_csi_drv_ioctl(unsigned char uc_if, MODULE_INFO *pmodule_info, unsigned int ui_cmd, unsigned long ul_arg)
{
	REG_INFO reg_info;
	//REG_INFO_LIST reg_info_list;
	//int i_loop;
	int i_ret = 0;

	nvt_dbg(IND, "IF-%d cmd:%x\n", uc_if, ui_cmd);



	switch (ui_cmd) {
	case CSI_IOC_START:
		/*call someone to start operation*/
		break;

	case CSI_IOC_STOP:
		/*call someone to stop operation*/
		break;

	case CSI_IOC_READ_REG:
		i_ret = copy_from_user(&reg_info, (void __user *)ul_arg, sizeof(REG_INFO));
		if (!i_ret) {
			reg_info.value = READ_REG(pmodule_info->io_addr[uc_if] + reg_info.addr);
			i_ret = copy_to_user((void __user *)ul_arg, &reg_info, sizeof(REG_INFO));
		}
		break;

	case CSI_IOC_WRITE_REG:
		i_ret = copy_from_user(&reg_info, (void __user *)ul_arg, sizeof(REG_INFO));
		if (!i_ret) {
			WRITE_REG(reg_info.value, pmodule_info->io_addr[uc_if] + reg_info.addr);
		}
		break;

#if 0
	case CSI_IOC_READ_REG_LIST:
		i_ret = copy_from_user(&reg_info_list, (void __user *)ul_arg, sizeof(REG_INFO_LIST));
		if (!i_ret) {
			for (i_loop = 0 ; i_loop < reg_info_list.count; i_loop++) {
				reg_info_list.reg_list[i_loop].value = READ_REG(pmodule_info->io_addr[uc_if] + reg_info_list.reg_list[i_loop].addr);
			}

			i_ret = copy_to_user((void __user *)ul_arg, &reg_info_list, sizeof(REG_INFO_LIST));
		}
		break;
	case CSI_IOC_WRITE_REG_LIST:
		i_ret = copy_from_user(&reg_info_list, (void __user *)ul_arg, sizeof(REG_INFO_LIST));
		if (!i_ret)
			for (i_loop = 0 ; i_loop < reg_info_list.count ; i_loop++) {
				WRITE_REG(reg_info_list.reg_list[i_loop].value, pmodule_info->io_addr[uc_if] + reg_info_list.reg_list[i_loop].addr);
			}
		break;
#endif
		/* Add other operations here */
	}

	return i_ret;
}

irqreturn_t nvt_csi_drv_isr(int irq, void *devid)
{
#if 0
	PMODULE_INFO pmodule_info = (PMODULE_INFO)devid;

	/* csi and lvds are share interrupts, if it is not csi interrupt, it must return IRQ_NONE */
	/*if (pmodule_info->io_addr[0] != 0xF0280000)
	    return IRQ_NONE;*/

	/* simple triggle and response mechanism*/
	complete(&pmodule_info->csi_completion);


	/*  Tasklet for bottom half mechanism */
	tasklet_schedule(&pmodule_info->csi_tasklet);
#endif
	int ret;
	PMODULE_INFO pmodule_info = (PMODULE_INFO)devid;

	//ret = IRQ_NONE;
	if (pmodule_info->signature != CSI_SIGNATURE) {
		//nvt_dbg(IND, "not CSI interrupt !\r\n");
		ret = IRQ_NONE;
	} else {
		if (irq == pmodule_info->iinterrupt_id[0]) {
			//nvt_dbg(IND, "irq = %d\r\n", irq);
			if (csi_isr_check()) {
				csi_isr();
				ret = IRQ_HANDLED;
			} else {
				ret = IRQ_NONE;
			}
		} else if (irq == pmodule_info->iinterrupt_id[1]) {
			//nvt_dbg(IND, "irq2 = %d\r\n", irq);
			if (csi2_isr_check()) {
				csi2_isr();
				ret = IRQ_HANDLED;
			} else {
				ret = IRQ_NONE;
			}
		} else {
			nvt_dbg(IND, "irqnum = %d\r\n", irq);
			ret = IRQ_NONE;
		}
	}
	return ret;
}

int nvt_csi_drv_wait_cmd_complete(PMODULE_INFO pmodule_info)
{
	wait_for_completion(&pmodule_info->csi_completion);
	return 0;
}

int nvt_csi_drv_write_reg(PMODULE_INFO pmodule_info, unsigned long addr, unsigned long value)
{
	WRITE_REG(value, pmodule_info->io_addr[0] + addr);
	return 0;
}

int nvt_csi_drv_read_reg(PMODULE_INFO pmodule_info, unsigned long addr)
{
	return READ_REG(pmodule_info->io_addr[0] + addr);
}

void nvt_csi_drv_do_tasklet(unsigned long data)
{
	PMODULE_INFO pmodule_info = (PMODULE_INFO)data;

	nvt_dbg(IND, "\n");

	/* do something you want*/
	complete(&pmodule_info->csi_completion);
}
