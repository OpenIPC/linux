#include <linux/wait.h>
#include <linux/param.h>
#include <linux/sched.h>
#include <linux/interrupt.h>
#include <linux/uaccess.h>
#include <linux/clk.h>

#include "kwrap/type.h"
#include "lvds_drv.h"
#include "lvds_ioctl.h"
#include "lvds_dbg.h"
#include "../lvds.h"
#include "lvds_int.h"

/*===========================================================================*/
/* Function declaration                                                      */
/*===========================================================================*/
int nvt_lvds_drv_wait_cmd_complete(PLVDS_MODULE_INFO pmodule_info);
int nvt_lvds_drv_ioctl(unsigned char uc_if, LVDS_MODULE_INFO *pmodule_info, unsigned int cmd, unsigned long arg);
#if (LVDS_INT_MODE == LVDS_INT_MODE_TASKLET)
void nvt_lvds_drv_do_tasklet(unsigned long data);
void nvt_lvds2_drv_do_tasklet(unsigned long data);
void nvt_lvds3_drv_do_tasklet(unsigned long data);
void nvt_lvds4_drv_do_tasklet(unsigned long data);
void nvt_lvds5_drv_do_tasklet(unsigned long data);
void nvt_lvds6_drv_do_tasklet(unsigned long data);
void nvt_lvds7_drv_do_tasklet(unsigned long data);
void nvt_lvds8_drv_do_tasklet(unsigned long data);
#endif
irqreturn_t nvt_lvds_drv_isr(int irq, void *devid);
/*===========================================================================*/
/* Define                                                                    */
/*===========================================================================*/
typedef irqreturn_t (*irq_handler_t)(int, void *);

/*===========================================================================*/
/* Global variable                                                           */
/*===========================================================================*/
PLVDS_MODULE_INFO plvds_mod_info;


UINT32 _LVDS_REG_BASE_ADDR[MODULE_REG_NUM];

/*===========================================================================*/
/* Function define                                                           */
/*===========================================================================*/
int nvt_lvds_drv_open(PLVDS_MODULE_INFO pmodule_info, unsigned char uc_if)
{
	nvt_dbg(IND, "%d\n", uc_if);

	/* Add HW Moduel initial operation here when the device file opened*/

	return 0;
}


int nvt_lvds_drv_release(PLVDS_MODULE_INFO pmodule_info, unsigned char uc_if)
{
	nvt_dbg(IND, "%d\n", uc_if);

	/* Add HW Moduel release operation here when device file closed */

	return 0;
}

int nvt_lvds_drv_init(LVDS_MODULE_INFO *pmodule_info)
{
	int ret = 0;
	unsigned char ucloop;

	lvds_api("%s\n", __func__);

#if (LVDS_INT_MODE == LVDS_INT_MODE_TASKLET)
#if (MODULE_IRQ_NUM >= 1)
	tasklet_init(&pmodule_info->lvds_tasklet[0], nvt_lvds_drv_do_tasklet, (unsigned long)pmodule_info);
#endif
#if (MODULE_IRQ_NUM >= 2)
	tasklet_init(&pmodule_info->lvds_tasklet[1], nvt_lvds2_drv_do_tasklet, (unsigned long)pmodule_info);
#endif
#endif

	/* initial clock here */
	for (ucloop = 0 ; ucloop < MODULE_CLK_NUM; ucloop++) {
		if (!(IS_ERR(pmodule_info->pclk[ucloop]))) {

			clk_prepare(pmodule_info->pclk[ucloop]);

			/* enable clock first. modify this later */
			//clk_enable(pmodule_info->pclk[ucloop]);
		}
	}

	/* register IRQ here*/
	for (ucloop = 0 ; ucloop < MODULE_IRQ_NUM; ucloop++) {
#if (LVDS_INT_MODE == LVDS_INT_MODE_TASKLET)
		if (request_irq(pmodule_info->iinterrupt_id[ucloop], nvt_lvds_drv_isr, IRQF_SHARED, "LVDS_INT", pmodule_info)) {
#else
		if (request_irq(pmodule_info->iinterrupt_id[ucloop], nvt_lvds_drv_isr, IRQF_SHARED | IRQF_TRIGGER_HIGH, "LVDS_INT", pmodule_info)) {
#endif
			nvt_dbg(ERR, "failed to register an IRQ Int:%d\n", pmodule_info->iinterrupt_id[ucloop]);
			ret = -ENODEV;
			goto FAIL_FREE_IRQ;
		}
	}

	for (ucloop = 0 ; ucloop < MODULE_REG_NUM; ucloop++) {
		_LVDS_REG_BASE_ADDR[ucloop] = (UINT32)pmodule_info->io_addr[ucloop];
		nvt_dbg(IND, "LVDS[%d]-Addr = 0x%08X\n", ucloop, _LVDS_REG_BASE_ADDR[ucloop]);
	}

	lvds_create_resource();
	lvds2_create_resource();
	plvds_mod_info = pmodule_info;

	return ret;

FAIL_FREE_IRQ:

	free_irq(pmodule_info->iinterrupt_id[ucloop], pmodule_info);

	/* Add error handler here */

	return ret;
}

int nvt_lvds_drv_remove(LVDS_MODULE_INFO *pmodule_info)
{
	unsigned char ucloop;

	lvds_api("%s\n", __func__);

	for (ucloop = 0 ; ucloop < MODULE_IRQ_NUM; ucloop++) {
		/* Free IRQ */
		free_irq(pmodule_info->iinterrupt_id[ucloop], pmodule_info);

		//if (!(IS_ERR(pmodule_info->pclk[ucloop]))) {
			/* disable clock */
			//clk_disable(pmodule_info->pclk[ucloop]);
		//}

	}

	/* Add HW Moduel release operation here*/
	plvds_mod_info = NULL;
	lvds_release_resource();
	lvds2_release_resource();

	return 0;
}

int nvt_lvds_drv_suspend(LVDS_MODULE_INFO *pmodule_info)
{
	nvt_dbg(IND, "\n");

	/* Add suspend operation here*/

	return 0;
}

int nvt_lvds_drv_resume(LVDS_MODULE_INFO *pmodule_info)
{
	nvt_dbg(IND, "\n");

	/* Add resume operation here*/

	return 0;
}

int nvt_lvds_drv_ioctl(unsigned char uc_if, LVDS_MODULE_INFO *pmodule_info, unsigned int ui_cmd, unsigned long ul_arg)
{
	REG_INFO reg_info;
	//REG_INFO_LIST reg_info_list;
	//int loop;
	int ret = 0;

	nvt_dbg(IND, "IF-%d cmd:%x\n", uc_if, ui_cmd);

	switch (ui_cmd) {
	case LVDS_IOC_START:
		/*call someone to start operation*/
		break;

	case LVDS_IOC_STOP:
		/*call someone to stop operation*/
		break;

	case LVDS_IOC_READ_REG:
		ret = copy_from_user(&reg_info, (void __user *)ul_arg, sizeof(REG_INFO));
		if (!ret) {
			reg_info.ui_value = READ_REG(pmodule_info->io_addr[uc_if] + reg_info.ui_addr);
			ret = copy_to_user((void __user *)ul_arg, &reg_info, sizeof(REG_INFO));
		}
		break;

	case LVDS_IOC_WRITE_REG:
		ret = copy_from_user(&reg_info, (void __user *)ul_arg, sizeof(REG_INFO));
		if (!ret) {
			WRITE_REG(reg_info.ui_value, pmodule_info->io_addr[uc_if] + reg_info.ui_addr);
		}
		break;
/*
	case LVDS_IOC_READ_REG_LIST:
		ret = copy_from_user(&reg_info_list, (void __user *)ul_arg, sizeof(REG_INFO_LIST));
		if (!ret) {
			for (loop = 0 ; loop < reg_info_list.ui_count; loop++) {
				reg_info_list.reg_list[loop].ui_value = READ_REG(pmodule_info->io_addr[uc_if] + reg_info_list.reg_list[loop].ui_addr);
			}

			ret = copy_to_user((void __user *)ul_arg, &reg_info_list, sizeof(REG_INFO_LIST));
		}
		break;
	case LVDS_IOC_WRITE_REG_LIST:
		ret = copy_from_user(&reg_info_list, (void __user *)ul_arg, sizeof(REG_INFO_LIST));
		if (!ret)
			for (loop = 0 ; loop < reg_info_list.ui_count ; loop++) {
				WRITE_REG(reg_info_list.reg_list[loop].ui_value, pmodule_info->io_addr[uc_if] + reg_info_list.reg_list[loop].ui_addr);
			}
		break;
*/
		/* Add other operations here */
	}

	return ret;
}

int nvt_lvds_drv_write_reg(PLVDS_MODULE_INFO pmodule_info, unsigned long addr, unsigned long value)
{
	int idx;

	idx = (addr>>16)&0xF;

	if (pmodule_info->io_addr[idx]) {
		WRITE_REG(value, pmodule_info->io_addr[idx] + (addr&0xFFFF));
		lvds_api("LVDS%d_W_REG[0x%03X]=0x%08X\n", idx, (unsigned int)(addr&0xFFFF), (unsigned int)value);
	}

	return 0;
}

int nvt_lvds_drv_read_reg(PLVDS_MODULE_INFO pmodule_info, unsigned long addr)
{
	int ret = 0, idx;

	idx = (addr>>16)&0xF;

	if (pmodule_info->io_addr[idx]) {
		ret = READ_REG(pmodule_info->io_addr[idx] + (addr&0xFFFF));
		//lvds_api("LVDS%d_R_REG[0x%03X]=0x%08X\n", idx, (addr&0xFFFF), Ret);
	}

	return ret;
}


irqreturn_t nvt_lvds_drv_isr(int irq, void *devid)
{
	PLVDS_MODULE_INFO pmodule_info = (PLVDS_MODULE_INFO)devid;

	if (pmodule_info == plvds_mod_info) {

		/* lvds_api("%s irq=%d\n", __func__, irq); */

		if (irq == pmodule_info->iinterrupt_id[0]) {
			if (lvds_isr_check()) {
#if (LVDS_INT_MODE == LVDS_INT_MODE_TASKLET)
				tasklet_schedule(&pmodule_info->lvds_tasklet[0]);
#else
				lvds_isr();
#endif
				return IRQ_HANDLED;
			} else {
				return IRQ_NONE;
			}
		}

		if (irq == pmodule_info->iinterrupt_id[1]) {
			if (lvds2_isr_check()) {
#if (LVDS_INT_MODE == LVDS_INT_MODE_TASKLET)
				tasklet_schedule(&pmodule_info->lvds_tasklet[1]);
#else
				lvds2_isr();
#endif
				return IRQ_HANDLED;
			} else {
				return IRQ_NONE;
			}
		}
	}

	return IRQ_NONE;
}

#if (LVDS_INT_MODE == LVDS_INT_MODE_TASKLET)
void nvt_lvds_drv_do_tasklet(unsigned long data)
{
	nvt_dbg(IND, "\n");

	while (lvds_isr_check()) {
		lvds_isr();
	}
}

void nvt_lvds2_drv_do_tasklet(unsigned long data)
{
	nvt_dbg(IND, "\n");

	while (lvds2_isr_check()) {
		lvds2_isr();
	}
}


#endif

