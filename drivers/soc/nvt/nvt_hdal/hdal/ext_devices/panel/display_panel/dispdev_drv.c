#include <linux/wait.h>
#include <linux/param.h>
#include <linux/sched.h>
#include <linux/interrupt.h>
#include <linux/uaccess.h>
#include <linux/clk.h>
#include "dispdev_drv.h"
#include "dispdev_ioctl.h"
#include "dispdev_dbg.h"

/*===========================================================================*/
/* Function declaration                                                      */
/*===========================================================================*/
int nvt_dispdev_drv_wait_cmd_complete(PMODULE_INFO pmodule_info);
int nvt_dispdev_drv_ioctl(unsigned char uc_if, MODULE_INFO *pmodule_info, unsigned int cmd, unsigned long arg);
/*===========================================================================*/
/* Define                                                                    */
/*===========================================================================*/
typedef irqreturn_t (*irq_handler_t)(int, void *);

/*===========================================================================*/
/* Global variable                                                           */
/*===========================================================================*/
int i_even_flag = 0;

/*===========================================================================*/
/* Function define                                                           */
/*===========================================================================*/
int nvt_dispdev_drv_open(PMODULE_INFO pmodule_info, unsigned char uc_if)
{
	nvt_dbg(IND, "%d\n", uc_if);

	/* Add HW Moduel initial operation here when the device file opened*/

	return 0;
}


int nvt_dispdev_drv_release(PMODULE_INFO pmodule_info, unsigned char uc_if)
{
	nvt_dbg(IND, "%d\n", uc_if);

	/* Add HW Moduel release operation here when device file closed */

	return 0;
}

int nvt_dispdev_drv_init(MODULE_INFO *pmodule_info)
{
	int i_ret = 0;

	/* Add HW Module initialization here when driver loaded */

	return i_ret;

}

int nvt_dispdev_drv_remove(MODULE_INFO *pmodule_info)
{

	/* Add HW Moduel release operation here*/

	return 0;
}

int nvt_dispdev_drv_suspend(MODULE_INFO *pmodule_info)
{
	nvt_dbg(IND, "\n");

	/* Add suspend operation here*/

	return 0;
}

int nvt_dispdev_drv_resume(MODULE_INFO *pmodule_info)
{
	nvt_dbg(IND, "\n");
	/* Add resume operation here*/

	return 0;
}

int nvt_dispdev_drv_ioctl(unsigned char uc_if, MODULE_INFO *pmodule_info, unsigned int ui_cmd, unsigned long ui_arg)
{
	//REG_INFO reg_info;
	//REG_INFO_LIST reg_info_list;
	//int i_loop;
	int i_ret = 0;

	nvt_dbg(IND, "IF-%d cmd:%x\n", uc_if, ui_cmd);



	switch (ui_cmd) {
	case DISPDEV_IOC_START:
		/*call someone to start operation*/
		break;

	case DISPDEV_IOC_STOP:
		/*call someone to stop operation*/
		break;
#if 0
	case DISPDEV_IOC_READ_REG:
		i_ret = copy_from_user(&reg_info, (void __user *)ui_arg, sizeof(REG_INFO));
		if (!i_ret) {
			//reg_info.ui_value = READ_REG(pmodule_info->io_addr[uc_if] + reg_info.ui_addr);
			i_ret = copy_to_user((void __user *)ui_arg, &reg_info, sizeof(REG_INFO));
		}
		break;

	case DISPDEV_IOC_WRITE_REG:
		i_ret = copy_from_user(&reg_info, (void __user *)ui_arg, sizeof(REG_INFO));
		if (!i_ret)
			//WRITE_REG(reg_info.ui_value, pmodule_info->io_addr[uc_if] + reg_info.ui_addr);
		{
			break;
		}

	case DISPDEV_IOC_READ_REG_LIST:
		i_ret = copy_from_user(&reg_info_list, (void __user *)ui_arg, sizeof(REG_INFO_LIST));
		if (!i_ret) {
			for (i_loop = 0 ; i_loop < reg_info_list.ui_count; i_loop++)
				//reg_info_list.reg_list[i_loop].ui_value = READ_REG(pmodule_info->io_addr[uc_if] + reg_info_list.reg_list[i_loop].ui_addr);

			{
				i_ret = copy_to_user((void __user *)ui_arg, &reg_info_list, sizeof(REG_INFO_LIST));
			}
		}
		break;
	case DISPDEV_IOC_WRITE_REG_LIST:
		i_ret = copy_from_user(&reg_info_list, (void __user *)ui_arg, sizeof(REG_INFO_LIST));
		if (!i_ret)
			for (i_loop = 0 ; i_loop < reg_info_list.ui_count ; i_loop++)
				//WRITE_REG(reg_info_list.reg_list[i_loop].ui_value, pmodule_info->io_addr[uc_if] + reg_info_list.reg_list[i_loop].ui_addr);
			{
				break;
			}
#endif
		/* Add other operations here */
	}

	return i_ret;
}

int nvt_dispdev_drv_wait_cmd_complete(PMODULE_INFO pmodule_info)
{
	return 0;
}

int nvt_dispdev_drv_write_reg(PMODULE_INFO pmodule_info, unsigned long addr, unsigned long value)
{
	return 0;
}

int nvt_dispdev_drv_read_reg(PMODULE_INFO pmodule_info, unsigned long addr)
{
	return 0;
}
