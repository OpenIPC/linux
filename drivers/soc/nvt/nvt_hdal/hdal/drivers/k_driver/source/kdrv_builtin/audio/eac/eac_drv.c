#include <linux/wait.h>
#include <linux/param.h>
#include <linux/sched.h>
#include <linux/interrupt.h>
#include <linux/uaccess.h>
#include <linux/clk.h>

#include "kwrap/type.h"
#include "eac_drv.h"
#include "eac_ioctl.h"
#include "eac_dbg.h"
#include "eac.h"
#include "eac_platform.h"
//#include "eac_int.h"

/*===========================================================================*/
/* Function declaration                                                      */
/*===========================================================================*/
int nvt_eac_drv_wait_cmd_complete(PEAC_MODULE_INFO pmodule_info);
int nvt_eac_drv_ioctl(unsigned char uc_if, EAC_MODULE_INFO *pmodule_info, unsigned int cmd, unsigned long arg);
/*===========================================================================*/
/* Define                                                                    */
/*===========================================================================*/
typedef irqreturn_t (*irq_handler_t)(int, void *);

/*===========================================================================*/
/* Global variable                                                           */
/*===========================================================================*/
static PEAC_MODULE_INFO peac_mod_info;


UINT32 _EAC_REG_BASE_ADDR[EAC_REG_NUM];

/*===========================================================================*/
/* Function define                                                           */
/*===========================================================================*/
int nvt_eac_drv_open(PEAC_MODULE_INFO pmodule_info, unsigned char uc_if)
{
	nvt_dbg(IND, "%d\n", uc_if);

	/* Add HW Moduel initial operation here when the device file opened*/

	return 0;
}


int nvt_eac_drv_release(PEAC_MODULE_INFO pmodule_info, unsigned char uc_if)
{
	nvt_dbg(IND, "%d\n", uc_if);

	/* Add HW Moduel release operation here when device file closed */

	return 0;
}

int nvt_eac_drv_init(EAC_MODULE_INFO *pmodule_info)
{
	int ret = 0;
	unsigned char ucloop;
	struct clk *eac_clk;

	eac_api("%s\n", __func__);

	for (ucloop = 0 ; ucloop < EAC_REG_NUM; ucloop++) {
		_EAC_REG_BASE_ADDR[ucloop] = (UINT32)pmodule_info->io_addr[ucloop];
		nvt_dbg(IND, "EAC[%d]-Addr = 0x%08X\n", ucloop, _EAC_REG_BASE_ADDR[ucloop]);
	}

	//eac_create_resource();
	peac_mod_info = pmodule_info;

	eac_clk = clk_get(NULL, "f0640000.eac");
	if (IS_ERR(eac_clk)) {
		nvt_dbg(ERR, "failed to get eac clk\n");
		return -ENODEV;
	}
	clk_prepare(eac_clk);
	clk_put(eac_clk);

	eac_clk = clk_get(NULL, "f0640000.eacadc");
	if (IS_ERR(eac_clk)) {
		nvt_dbg(ERR, "failed to get eac-ad clk\n");
		return -ENODEV;
	}
	clk_prepare(eac_clk);
	clk_put(eac_clk);

	eac_clk = clk_get(NULL, "f0640000.eacdac");
	if (IS_ERR(eac_clk)) {
		nvt_dbg(ERR, "failed to get eac-da clk\n");
		return -ENODEV;
	}
	clk_prepare(eac_clk);
	clk_put(eac_clk);

	return ret;
}

int nvt_eac_drv_remove(EAC_MODULE_INFO *pmodule_info)
{
	eac_api("%s\n", __func__);

	/* Add HW Moduel release operation here*/
	peac_mod_info = NULL;

	return 0;
}

int nvt_eac_drv_suspend(EAC_MODULE_INFO *pmodule_info)
{
	nvt_dbg(IND, "\n");

	/* Add suspend operation here*/

	return 0;
}

int nvt_eac_drv_resume(EAC_MODULE_INFO *pmodule_info)
{
	nvt_dbg(IND, "\n");

	/* Add resume operation here*/

	return 0;
}

int nvt_eac_drv_ioctl(unsigned char uc_if, EAC_MODULE_INFO *pmodule_info, unsigned int ui_cmd, unsigned long ul_arg)
{
	REG_INFO reg_info;
	int ret = 0;

	nvt_dbg(IND, "IF-%d cmd:%x\n", uc_if, ui_cmd);

	switch (ui_cmd) {
	case EAC_IOC_START:
		/*call someone to start operation*/
		break;

	case EAC_IOC_STOP:
		/*call someone to stop operation*/
		break;

	case EAC_IOC_READ_REG:
		ret = copy_from_user(&reg_info, (void __user *)ul_arg, sizeof(REG_INFO));
		if (!ret) {
			reg_info.reg_value = READ_REG(pmodule_info->io_addr[uc_if] + reg_info.reg_addr);
			ret = copy_to_user((void __user *)ul_arg, &reg_info, sizeof(REG_INFO));
		}
		break;

	case EAC_IOC_WRITE_REG:
		ret = copy_from_user(&reg_info, (void __user *)ul_arg, sizeof(REG_INFO));
		if (!ret) {
			WRITE_REG(reg_info.reg_value, pmodule_info->io_addr[uc_if] + reg_info.reg_addr);
		}
		break;
#if 0
	case EAC_IOC_READ_REG_LIST:
		ret = copy_from_user(&reg_info_list, (void __user *)ul_arg, sizeof(REG_INFO_LIST));
		if (!ret) {
			cnt = reg_info_list.count;
			for (loop = 0 ; loop < cnt; loop++) {
				reg_info_list.reglist[loop].reg_value = READ_REG(pmodule_info->io_addr[uc_if] + reg_info_list.reglist[loop].reg_addr);
			}

			ret = copy_to_user((void __user *)ul_arg, &reg_info_list, sizeof(REG_INFO_LIST));
		}
		break;
	case EAC_IOC_WRITE_REG_LIST:
		ret = copy_from_user(&reg_info_list, (void __user *)ul_arg, sizeof(REG_INFO_LIST));
		if (!ret) {
			cnt = reg_info_list.count;
			for (loop = 0 ; loop < cnt ; loop++) {
				WRITE_REG(reg_info_list.reglist[loop].reg_value, pmodule_info->io_addr[uc_if] + reg_info_list.reglist[loop].reg_addr);
			}
		}
		break;
#endif
		/* Add other operations here */
	}

	return ret;
}

int nvt_eac_drv_write_reg(PEAC_MODULE_INFO pmodule_info, unsigned long addr, unsigned long value)
{
	int idx;

	idx = (addr>>16)&0xF;

	if (pmodule_info->io_addr[idx]) {
		WRITE_REG(value, pmodule_info->io_addr[idx] + (addr&0xFFFF));
		eac_api("EAC%d_W_REG[0x%03X]=0x%08X\n", idx, (unsigned int)(addr&0xFFFF), (unsigned int)value);
	}

	return 0;
}

int nvt_eac_drv_read_reg(PEAC_MODULE_INFO pmodule_info, unsigned long addr)
{
	int ret = 0, idx;

	idx = (addr>>16)&0xF;

	if (pmodule_info->io_addr[idx]) {
		ret = READ_REG(pmodule_info->io_addr[idx] + (addr&0xFFFF));
		//eac_api("EAC%d_R_REG[0x%03X]=0x%08X\n", idx, (addr&0xFFFF), ret);
	}

	return ret;
}

