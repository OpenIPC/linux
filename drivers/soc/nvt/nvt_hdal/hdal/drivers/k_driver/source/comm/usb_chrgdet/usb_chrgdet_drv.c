#include <linux/wait.h>
#include <linux/param.h>
#include <linux/sched.h>
#include <linux/interrupt.h>
#include <linux/uaccess.h>
#include <linux/clk.h>

#include "kwrap/type.h"
#include "usb_chrgdet_drv.h"
#include "usb_chrgdet_ioctl.h"
#include "usb_chrgdet_dbg.h"
//#include "../usb_chrgdet.h"
//#include "usb_chrgdet_int.h"

/*===========================================================================*/
/* Function declaration                                                      */
/*===========================================================================*/
int nvt_usb_chrgdet_drv_ioctl(unsigned char uc_if, USB_CHRGDET_MODULE_INFO *pmodule_info, unsigned int cmd, unsigned long arg);
/*===========================================================================*/
/* Define                                                                    */
/*===========================================================================*/
typedef irqreturn_t (*irq_handler_t)(int, void *);

/*===========================================================================*/
/* Global variable                                                           */
/*===========================================================================*/
UINT32 _USB_CHRGDET_REG_BASE_ADDR[MODULE_REG_NUM];

/*===========================================================================*/
/* Function define                                                           */
/*===========================================================================*/
int nvt_usb_chrgdet_drv_open(PUSB_CHRGDET_MODULE_INFO pmodule_info, unsigned char uc_if)
{
	nvt_dbg(IND, "%d\n", uc_if);

	usb_chrgdet_api("%s\n", __func__);

	/* Add HW Moduel initial operation here when the device file opened*/

	return 0;
}


int nvt_usb_chrgdet_drv_release(PUSB_CHRGDET_MODULE_INFO pmodule_info, unsigned char uc_if)
{
	nvt_dbg(IND, "%d\n", uc_if);

	usb_chrgdet_api("%s\n", __func__);

	/* Add HW Moduel release operation here when device file closed */

	return 0;
}

int nvt_usb_chrgdet_drv_init(USB_CHRGDET_MODULE_INFO *pmodule_info)
{
	int ret = 0;
	unsigned char ucloop;

	usb_chrgdet_api("%s\n", __func__);

	for (ucloop = 0 ; ucloop < MODULE_REG_NUM; ucloop++) {
		_USB_CHRGDET_REG_BASE_ADDR[ucloop] = (UINT32)pmodule_info->io_addr[ucloop];
		usb_chrgdet_api("USB_CHRGDET[%d]-Addr = 0x%08X\n", ucloop, _USB_CHRGDET_REG_BASE_ADDR[ucloop]);
	}

	//usb_chrgdet_init();

	return ret;
}

int nvt_usb_chrgdet_drv_remove(USB_CHRGDET_MODULE_INFO *pmodule_info)
{
	usb_chrgdet_api("%s\n", __func__);

	/* Add HW Moduel release operation here*/

	return 0;
}

#if 0
int nvt_usb_chrgdet_drv_suspend(USB_CHRGDET_MODULE_INFO *pmodule_info)
{
	nvt_dbg(IND, "\n");

	usb_chrgdet_api("%s\n", __func__);

	/* Add suspend operation here*/

	return 0;
}

int nvt_usb_chrgdet_drv_resume(USB_CHRGDET_MODULE_INFO *pmodule_info)
{
	nvt_dbg(IND, "\n");

	usb_chrgdet_api("%s\n", __func__);

	/* Add resume operation here*/

	return 0;
}
#endif

int nvt_usb_chrgdet_drv_ioctl(unsigned char uc_if, USB_CHRGDET_MODULE_INFO *pmodule_info, unsigned int ui_cmd, unsigned long ul_arg)
{
	REG_INFO reg_info;
	//REG_INFO_LIST reg_info_list;
	//int loop;
	int ret = 0;

	nvt_dbg(IND, "IF-%d cmd:%x\n", uc_if, ui_cmd);

	usb_chrgdet_api("%s IF-%d cmd:%x\n", __func__, uc_if, ui_cmd);

	switch (ui_cmd) {
	case USB_CHRGDET_IOC_START:
		/*call someone to start operation*/
		break;

	case USB_CHRGDET_IOC_STOP:
		/*call someone to stop operation*/
		break;

	case USB_CHRGDET_IOC_READ_REG:
		ret = copy_from_user(&reg_info, (void __user *)ul_arg, sizeof(REG_INFO));
		if (!ret) {
			reg_info.ui_value = READ_REG(pmodule_info->io_addr[uc_if] + reg_info.ui_addr);
			ret = copy_to_user((void __user *)ul_arg, &reg_info, sizeof(REG_INFO));
		}
		break;

	case USB_CHRGDET_IOC_WRITE_REG:
		ret = copy_from_user(&reg_info, (void __user *)ul_arg, sizeof(REG_INFO));
		if (!ret) {
			WRITE_REG(reg_info.ui_value, pmodule_info->io_addr[uc_if] + reg_info.ui_addr);
		}
		break;

		/* Add other operations here */
	}

	return ret;
}


