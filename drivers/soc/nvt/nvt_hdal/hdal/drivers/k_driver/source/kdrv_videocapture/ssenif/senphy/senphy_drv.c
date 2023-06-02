#include <linux/wait.h>
#include <linux/param.h>
#include <linux/sched.h>
#include <linux/interrupt.h>
#include <linux/uaccess.h>
#include <linux/clk.h>

#include "kwrap/type.h"
#include "senphy_drv.h"
#include "senphy_ioctl.h"
#include "senphy_dbg.h"
#include "../senphy.h"
//#include "senphy_int.h"
#include <kdrv_builtin/kdrv_builtin.h>


/*===========================================================================*/
/* Function declaration                                                      */
/*===========================================================================*/
int nvt_senphy_drv_wait_cmd_complete(PSENPHY_MODULE_INFO pmodule_info);
int nvt_senphy_drv_ioctl(unsigned char uc_if, SENPHY_MODULE_INFO *pmodule_info, unsigned int cmd, unsigned long arg);
void nvt_senphy_drv_do_tasklet(unsigned long data);
irqreturn_t nvt_senphy_drv_isr(int irq, void *devid);
/*===========================================================================*/
/* Define                                                                    */
/*===========================================================================*/
typedef irqreturn_t (*irq_handler_t)(int, void *);

/*===========================================================================*/
/* Global variable                                                           */
/*===========================================================================*/
UINT32 _SENPHY_REG_BASE_ADDR[MODULE_REG_NUM];

/*===========================================================================*/
/* Function define                                                           */
/*===========================================================================*/
int nvt_senphy_drv_open(PSENPHY_MODULE_INFO pmodule_info, unsigned char uc_if)
{
	nvt_dbg(IND, "%d\n", uc_if);

	senphy_api("%s\n", __func__);

	/* Add HW Moduel initial operation here when the device file opened*/

	return 0;
}


int nvt_senphy_drv_release(PSENPHY_MODULE_INFO pmodule_info, unsigned char uc_if)
{
	nvt_dbg(IND, "%d\n", uc_if);

	senphy_api("%s\n", __func__);

	/* Add HW Moduel release operation here when device file closed */

	return 0;
}

int nvt_senphy_drv_init(SENPHY_MODULE_INFO *pmodule_info)
{
	int ret = 0;
	unsigned char ucloop;
	int fastboot = kdrv_builtin_is_fastboot();

	senphy_api("%s\n", __func__);

	for (ucloop = 0 ; ucloop < MODULE_REG_NUM; ucloop++) {
		_SENPHY_REG_BASE_ADDR[ucloop] = (UINT32)pmodule_info->io_addr[ucloop];
		nvt_dbg(IND, "SENPHY[%d]-Addr = 0x%08X\n", ucloop, _SENPHY_REG_BASE_ADDR[ucloop]);
	}

	if (!fastboot) {
		senphy_init();
	}

	return ret;
}

int nvt_senphy_drv_remove(SENPHY_MODULE_INFO *pmodule_info)
{
	senphy_api("%s\n", __func__);

	/* Add HW Moduel release operation here*/

	return 0;
}

int nvt_senphy_drv_suspend(SENPHY_MODULE_INFO *pmodule_info)
{
	nvt_dbg(IND, "\n");

	senphy_api("%s\n", __func__);

	/* Add suspend operation here*/

	return 0;
}

int nvt_senphy_drv_resume(SENPHY_MODULE_INFO *pmodule_info)
{
	nvt_dbg(IND, "\n");

	senphy_api("%s\n", __func__);

	/* Add resume operation here*/

	return 0;
}

int nvt_senphy_drv_ioctl(unsigned char uc_if, SENPHY_MODULE_INFO *pmodule_info, unsigned int ui_cmd, unsigned long ul_arg)
{
	REG_INFO reg_info;
	//REG_INFO_LIST reg_info_list;
	//int loop;
	int ret = 0;

	nvt_dbg(IND, "IF-%d cmd:%x\n", uc_if, ui_cmd);

	senphy_api("%s IF-%d cmd:%x\n", __func__, uc_if, ui_cmd);

	switch (ui_cmd) {
	case SENPHY_IOC_START:
		/*call someone to start operation*/
		break;

	case SENPHY_IOC_STOP:
		/*call someone to stop operation*/
		break;

	case SENPHY_IOC_READ_REG:
		ret = copy_from_user(&reg_info, (void __user *)ul_arg, sizeof(REG_INFO));
		if (!ret) {
			reg_info.ui_value = READ_REG(pmodule_info->io_addr[uc_if] + reg_info.ui_addr);
			ret = copy_to_user((void __user *)ul_arg, &reg_info, sizeof(REG_INFO));
		}
		break;

	case SENPHY_IOC_WRITE_REG:
		ret = copy_from_user(&reg_info, (void __user *)ul_arg, sizeof(REG_INFO));
		if (!ret) {
			WRITE_REG(reg_info.ui_value, pmodule_info->io_addr[uc_if] + reg_info.ui_addr);
		}
		break;
/*
	case SENPHY_IOC_READ_REG_LIST:
		ret = copy_from_user(&reg_info_list, (void __user *)ul_arg, sizeof(REG_INFO_LIST));
		if (!ret) {
			for (loop = 0 ; loop < reg_info_list.ui_count; loop++) {
				reg_info_list.reg_list[loop].ui_value = READ_REG(pmodule_info->io_addr[uc_if] + reg_info_list.reg_list[loop].ui_addr);
			}

			ret = copy_to_user((void __user *)ul_arg, &reg_info_list, sizeof(REG_INFO_LIST));
		}
		break;
	case SENPHY_IOC_WRITE_REG_LIST:
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

irqreturn_t nvt_senphy_drv_isr(int irq, void *devid)
{
	//PSENPHY_MODULE_INFO pmodule_info = (PSENPHY_MODULE_INFO)devid;

	senphy_api("%s irq=%d\n", __func__, irq);

	/* simple triggle and response mechanism*/
	//complete(&pmodule_info->senphy_completion);


	/*  Tasklet for bottom half mechanism */
	//tasklet_schedule(&pmodule_info->senphy_tasklet);

	return IRQ_HANDLED;
}

int nvt_senphy_drv_wait_cmd_complete(PSENPHY_MODULE_INFO pmodule_info)
{
	senphy_api("%s\n", __func__);

	//wait_for_completion(&pmodule_info->senphy_completion);

	return 0;
}

int nvt_senphy_drv_write_reg(PSENPHY_MODULE_INFO pmodule_info, unsigned long addr, unsigned long value)
{
	int idx;

	idx = (addr>>16)&0xF;

	if (pmodule_info->io_addr[idx]) {
		WRITE_REG(value, pmodule_info->io_addr[idx] + (addr&0xFFFF));
		senphy_api("SENPHY%d_W_REG[0x%03X]=0x%08X\n", idx, (unsigned int)(addr&0xFFFF), (unsigned int)value);
	}

	return 0;
}

int nvt_senphy_drv_read_reg(PSENPHY_MODULE_INFO pmodule_info, unsigned long addr)
{
	int ret = 0, idx;

	idx = (addr>>16)&0xF;

	if (pmodule_info->io_addr[idx]) {
		ret = READ_REG(pmodule_info->io_addr[idx] + (addr&0xFFFF));
		//senphy_api("SENPHY%d_R_REG[0x%03X]=0x%08X\n", idx, (addr&0xFFFF), ret);
	}

	return ret;
}

void nvt_senphy_drv_do_tasklet(unsigned long data)
{
	//PSENPHY_MODULE_INFO pmodule_info = (PSENPHY_MODULE_INFO)data;

	nvt_dbg(IND, "\n");

	senphy_api("%s\n", __func__);

	/* do something you want*/
	//complete(&pmodule_info->senphy_completion);
}
