#include <linux/wait.h>
#include <linux/param.h>
#include <linux/sched.h>
#include <linux/interrupt.h>
#include <linux/uaccess.h>
#include <linux/clk.h>

#include "ssenif_drv.h"
#include "ssenif_ioctl.h"
#include "ssenif_dbg.h"
#include "kwrap/type.h"



/*===========================================================================*/
/* Function declaration                                                      */
/*===========================================================================*/

/*===========================================================================*/
/* Define                                                                    */
/*===========================================================================*/

/*===========================================================================*/
/* Global variable                                                           */
/*===========================================================================*/
UINT32 _SLVSEC_REG_BASE_ADDR[SLVSEC_REG_NUM];
PSSENIF_MODULE_INFO pssenif_mod_info;

/*===========================================================================*/
/* Function define                                                           */
/*===========================================================================*/
int nvt_ssenif_drv_open(PSSENIF_MODULE_INFO pmodule_info, unsigned char uc_if)
{
	nvt_dbg(IND, "%d\n", uc_if);

	ssenif_api("%s\n", __func__);

	/* Add HW Moduel initial operation here when the device file opened*/

	return 0;
}

int nvt_ssenif_drv_release(PSSENIF_MODULE_INFO pmodule_info, unsigned char uc_if)
{
	nvt_dbg(IND, "%d\n", uc_if);

	ssenif_api("%s\n", __func__);

	/* Add HW Moduel release operation here when device file closed */

	return 0;
}


int nvt_ssenif_drv_init(SSENIF_MODULE_INFO *pmodule_info)
{
	ssenif_api("%s\n", __func__);

	pssenif_mod_info = pmodule_info;

	return 0;
}

int nvt_ssenif_drv_remove(SSENIF_MODULE_INFO *pmodule_info)
{
	ssenif_api("%s\n", __func__);

	/* Add HW Moduel release operation here*/
	pssenif_mod_info = NULL;

	return 0;
}


int nvt_ssenif_drv_suspend(SSENIF_MODULE_INFO *pmodule_info)
{
	nvt_dbg(IND, "\n");

	ssenif_api("%s\n", __func__);

	/* Add suspend operation here*/








	return 0;
}

int nvt_ssenif_drv_resume(SSENIF_MODULE_INFO *pmodule_info)
{
	nvt_dbg(IND, "\n");

	ssenif_api("%s\n", __func__);

	/* Add resume operation here*/

	return 0;
}

int nvt_ssenif_drv_ioctl(unsigned char uc_if, SSENIF_MODULE_INFO *pmodule_info, unsigned int ui_cmd, unsigned long ul_arg)
{
	//REG_INFO reg_info;
	//REG_INFO_LIST reg_info_list;
	//int i_loop;
	int i_ret = 0;

	nvt_dbg(IND, "IF-%d cmd:%x\n", uc_if, ui_cmd);

	ssenif_api("%s IF-%d cmd:%x\n", __func__, uc_if, ui_cmd);

	switch (ui_cmd) {
	case SSENIF_IOC_START:
		/*call someone to start operation*/
		break;

	case SSENIF_IOC_STOP:
		/*call someone to stop operation*/
		break;
/*
	case SSENIF_IOC_READ_REG:
		i_ret = copy_from_user(&reg_info, (void __user *)ul_arg, sizeof(REG_INFO));
		if (!i_ret) {
			reg_info.ui_value = READ_REG(pmodule_info->slvsec_io_addr[uc_if] + reg_info.ui_addr);
			i_ret = copy_to_user((void __user *)ul_arg, &reg_info, sizeof(REG_INFO));
		}
		break;

	case SSENIF_IOC_WRITE_REG:
		i_ret = copy_from_user(&reg_info, (void __user *)ul_arg, sizeof(REG_INFO));
		if (!i_ret) {
			WRITE_REG(reg_info.ui_value, pmodule_info->slvsec_io_addr[uc_if] + reg_info.ui_addr);
		}
		break;
	case SSENIF_IOC_READ_REG_LIST:
		i_ret = copy_from_user(&reg_info_list, (void __user *)ul_arg, sizeof(REG_INFO_LIST));
		if (!i_ret) {
			for (i_loop = 0 ; i_loop < reg_info_list.ui_count; i_loop++) {
				reg_info_list.reg_list[i_loop].ui_value = READ_REG(pmodule_info->slvsec_io_addr[uc_if] + reg_info_list.reg_list[i_loop].ui_addr);
			}

			i_ret = copy_to_user((void __user *)ul_arg, &reg_info_list, sizeof(REG_INFO_LIST));
		}
		break;
	case SSENIF_IOC_WRITE_REG_LIST:
		i_ret = copy_from_user(&reg_info_list, (void __user *)ul_arg, sizeof(REG_INFO_LIST));
		if (!i_ret)
			for (i_loop = 0 ; i_loop < reg_info_list.ui_count ; i_loop++) {
				WRITE_REG(reg_info_list.reg_list[i_loop].ui_value, pmodule_info->slvsec_io_addr[uc_if] + reg_info_list.reg_list[i_loop].ui_addr);
			}
		break;
*/
		/* Add other operations here */
	}

	return i_ret;
}







