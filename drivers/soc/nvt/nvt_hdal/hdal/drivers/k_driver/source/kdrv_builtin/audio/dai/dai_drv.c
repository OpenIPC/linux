#include <linux/wait.h>
#include <linux/param.h>
#include <linux/sched.h>
#include <linux/interrupt.h>
#include <linux/uaccess.h>
#include <linux/clk.h>

#include "kwrap/type.h"
#include "dai_drv.h"
#include "dai_ioctl.h"
#include "dai_dbg.h"
#include "dai_int.h"
#include "dai.h"
#include "dai_platform.h"

/*===========================================================================*/
/* Function declaration                                                      */
/*===========================================================================*/
int nvt_dai_drv_wait_cmd_complete(PDAI_INFO pmodule_info);
int nvt_dai_drv_ioctl(unsigned char uc_if, DAI_INFO *pmodule_info, unsigned int cmd, unsigned long arg);
irqreturn_t nvt_dai_drv_do_tasklet(int irq, void *devid);
irqreturn_t nvt_dai_drv_isr(int irq, void *devid);
/*===========================================================================*/
/* Define                                                                    */
/*===========================================================================*/
typedef irqreturn_t (*irq_handler_t)(int, void *);

/*===========================================================================*/
/* Global variable                                                           */
/*===========================================================================*/
int event_flag = 0;
UINT32 _DAI_REG_BASE_ADDR[DAI_REG_NUM];

/*===========================================================================*/
/* Function define                                                           */
/*===========================================================================*/
int nvt_dai_drv_open(PDAI_INFO pmodule_info, unsigned char uc_if)
{
	nvt_dbg(IND, "%d\n", uc_if);

	/* Add HW Moduel initial operation here when the device file opened*/

	return 0;
}


int nvt_dai_drv_release(PDAI_INFO pmodule_info, unsigned char uc_if)
{
	nvt_dbg(IND, "%d\n", uc_if);

	/* Add HW Moduel release operation here when device file closed */

	return 0;
}

int nvt_dai_drv_init(DAI_INFO *pmodule_info)
{
	int ret = 0;
	unsigned char ucloop;
	struct clk *dai_clk;

	for (ucloop = 0 ; ucloop < DAI_REG_NUM; ucloop++) {
		_DAI_REG_BASE_ADDR[ucloop] = (UINT32)pmodule_info->io_addr[ucloop];
		nvt_dbg(IND, "DAI[%d]-Addr = 0x%08X\n", ucloop, _DAI_REG_BASE_ADDR[ucloop]);
	}
#if 0
	/* register IRQ here*/
	if (request_irq(pmodule_info->iinterrupt_id[0], nvt_dai_drv_isr, IRQF_TRIGGER_HIGH, "DAI_INT", pmodule_info)) {
		nvt_dbg(ERR, "failed to register an IRQ Int:%d\n", pmodule_info->iinterrupt_id[0]);
		ret = -ENODEV;
		goto FAIL_FREE_IRQ;
	}

	tasklet_init(&pmodule_info->dai_tasklet[0], nvt_dai_drv_do_tasklet, (unsigned long)pmodule_info);
#else
	ret = request_threaded_irq(pmodule_info->iinterrupt_id[0], nvt_dai_drv_isr, nvt_dai_drv_do_tasklet,
			IRQF_TRIGGER_HIGH|IRQF_SHARED, "DAI_INT", pmodule_info);

	if (ret < 0) {
		nvt_dbg(ERR, "DAI request_irq error (%d)\n",ret);
		goto FAIL_FREE_IRQ;
	}
#endif
	/* Add HW Module initialization here when driver loaded */

	dai_clk = clk_get(NULL, "f0630000.dai");
	if (IS_ERR(dai_clk)) {
		nvt_dbg(ERR, "failed to get dai clk\n");
		ret = -ENODEV;
		goto FAIL_FREE_IRQ;
	}
	// reset controller
	clk_prepare(dai_clk);
	clk_put(dai_clk);


	return ret;

FAIL_FREE_IRQ:

	free_irq(pmodule_info->iinterrupt_id[0], pmodule_info);

	/* Add error handler here */

	return ret;
}

int nvt_dai_drv_remove(DAI_INFO *pmodule_info)
{

	//Free IRQ
	free_irq(pmodule_info->iinterrupt_id[0], pmodule_info);

	/* Add HW Moduel release operation here*/

	return 0;
}

int nvt_dai_drv_suspend(DAI_INFO *pmodule_info)
{
	nvt_dbg(IND, "\n");

	/* Add suspend operation here*/

	return 0;
}

int nvt_dai_drv_resume(DAI_INFO *pmodule_info)
{
	nvt_dbg(IND, "\n");
	/* Add resume operation here*/

	return 0;
}

int nvt_dai_drv_ioctl(unsigned char uc_if, DAI_INFO *pmodule_info, unsigned int ui_cmd, unsigned long ul_arg)
{
	REG_INFO reg_info;
	int ret = 0;


	nvt_dbg(IND, "IF-%d cmd:%x\n", uc_if, ui_cmd);



	switch (ui_cmd) {
	case DAI_IOC_START:
			/*call someone to start operation*/
		break;

	case DAI_IOC_STOP:
			/*call someone to stop operation*/
		break;

	case DAI_IOC_READ_REG:
		ret = copy_from_user(&reg_info, (void __user *)ul_arg, sizeof(REG_INFO));
		if (!ret) {
			reg_info.reg_value = READ_REG(pmodule_info->io_addr[uc_if] + reg_info.reg_addr);
			ret = copy_to_user((void __user *)ul_arg, &reg_info, sizeof(REG_INFO));
		}
		break;

	case DAI_IOC_WRITE_REG:
		ret = copy_from_user(&reg_info, (void __user *)ul_arg, sizeof(REG_INFO));
		if (!ret)
			WRITE_REG(reg_info.reg_value, pmodule_info->io_addr[uc_if] + reg_info.reg_addr);
		break;
#if 0
	case DAI_IOC_READ_REG_LIST:
		ret = copy_from_user(&reg_info_list, (void __user *)ul_arg, sizeof(REG_INFO_LIST));
		if (!ret) {
			cnt = reg_info_list.count;
			for (loop = 0; loop < cnt; loop++)
				reg_info_list.reglist[loop].reg_value = READ_REG(pmodule_info->io_addr[uc_if] + reg_info_list.reglist[loop].reg_addr);
			ret = copy_to_user((void __user *)ul_arg, &reg_info_list, sizeof(REG_INFO_LIST));
		}
		break;
	case DAI_IOC_WRITE_REG_LIST:
		ret = copy_from_user(&reg_info_list, (void __user *)ul_arg, sizeof(REG_INFO_LIST));
		if (!ret) {
			cnt = reg_info_list.count;
			for (loop = 0 ; loop < cnt; loop++)
				WRITE_REG(reg_info_list.reglist[loop].reg_value, pmodule_info->io_addr[uc_if] + reg_info_list.reglist[loop].reg_addr);
		}
		break;
#endif
		/* Add other operations here */
	}

	return ret;
}

irqreturn_t nvt_dai_drv_isr(int irq, void *devid)
{
	//PDAI_INFO pmodule_info = (PDAI_INFO)devid;

	dai_isr();
	//tasklet_schedule(&pmodule_info->dai_tasklet[0]);

	return IRQ_WAKE_THREAD;
}

int nvt_dai_drv_wait_cmd_complete(PDAI_INFO pmodule_info)
{
	wait_for_completion(&pmodule_info->dai_completion);
	return 0;
}

int nvt_dai_drv_write_reg(PDAI_INFO pmodule_info, unsigned long addr, unsigned long value)
{
	WRITE_REG(value, pmodule_info->io_addr[0] + addr);
	return 0;
}

int nvt_dai_drv_read_reg(PDAI_INFO pmodule_info, unsigned long addr)
{
	return READ_REG(pmodule_info->io_addr[0] + addr);
}

irqreturn_t nvt_dai_drv_do_tasklet(int irq, void *devid)
{
	nvt_dbg(IND, "\n");

	dai_tasklet();
	return IRQ_HANDLED;
}
