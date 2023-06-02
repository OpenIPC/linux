#include <linux/wait.h>
#include <linux/param.h>
#include <linux/sched.h>
#include <linux/interrupt.h>
#include <linux/uaccess.h>
#include <linux/clk.h>
#include <linux/slab.h>

#include "usb2dev_int.h"
#include "kwrap/type.h"
#include "usb2dev_drv.h"
#include "usb2dev_ioctl.h"
#include "usb2dev_dbg.h"
#include "usb2dev.h"
#include "emu_usbtest.h"

/*===========================================================================*/
/* Function declaration                                                      */
/*===========================================================================*/
int nvt_usb2dev_drv_ioctl(unsigned char uc_if, USB2DEV_MODULE_INFO *pmodule_info, unsigned int cmd, unsigned long arg);
irqreturn_t nvt_usb2dev_drv_isr(int irq, void *devid);

/*===========================================================================*/
/* Define                                                                    */
/*===========================================================================*/
typedef irqreturn_t (*irq_handler_t)(int, void *);

/*===========================================================================*/
/* Global variable                                                           */
/*===========================================================================*/
UINT32 _USB2DEV_REG_BASE_ADDR[MODULE_REG_NUM];


extern USB_MNG gUSBManage;

static USB_DEVICE_DESC *usbioc_dev_desc = NULL;
static USB_DEVICE_DESC *usbioc_devquali_desc = NULL;
static UINT8           *usbioc_cfg_desc_hs = NULL;
static UINT8           *usbioc_cfg_desc_hs_oth = NULL;
static UINT8           *usbioc_cfg_desc_fs = NULL;
static UINT8           *usbioc_cfg_desc_fs_oth = NULL;

static UINT8           *usbioc_str_desc0 = NULL;
static UINT8           *usbioc_str_desc1 = NULL;
static UINT8           *usbioc_str_desc2 = NULL;
static UINT8           *usbioc_str_desc3 = NULL;
static UINT8           *usbioc_str_desc4 = NULL;
static UINT8           *usbioc_str_desc5 = NULL;
static UINT8           *usbioc_str_desc6 = NULL;
static UINT8           *usbioc_str_desc7 = NULL;

static UINT8           *usbioc_cx_data_buf = NULL;

/*===========================================================================*/
/* Function define                                                           */
/*===========================================================================*/
int nvt_usb2dev_drv_open(PUSB2DEV_MODULE_INFO pmodule_info, unsigned char uc_if)
{
	nvt_dbg(IND, "%d\n", uc_if);

	usb2dev_api("%s\n", __func__);

	/* Add HW Moduel initial operation here when the device file opened*/

	return 0;
}


int nvt_usb2dev_drv_release(PUSB2DEV_MODULE_INFO pmodule_info, unsigned char uc_if)
{
	nvt_dbg(IND, "%d\n", uc_if);

	usb2dev_api("%s\n", __func__);

	/* Add HW Moduel release operation here when device file closed */

	return 0;
}

int nvt_usb2dev_drv_init(USB2DEV_MODULE_INFO *pmodule_info)
{
	int ret = 0;
	unsigned char ucloop;

	usb2dev_api("%s\n", __func__);

	for (ucloop = 0 ; ucloop < MODULE_REG_NUM; ucloop++) {
		_USB2DEV_REG_BASE_ADDR[ucloop] = (UINT32)pmodule_info->io_addr[ucloop];
		usb2dev_api("USB2DEV[%d]-Addr = 0x%08X\n", ucloop, _USB2DEV_REG_BASE_ADDR[ucloop]);
	}

	/* register IRQ here*/
	for (ucloop = 0 ; ucloop < MODULE_IRQ_NUM; ucloop++) {
		if (request_irq(pmodule_info->iinterrupt_id[ucloop], nvt_usb2dev_drv_isr, IRQF_TRIGGER_HIGH, "USB_INT", pmodule_info)) {
			nvt_dbg(ERR, "failed to register an IRQ Int:%d\n", pmodule_info->iinterrupt_id[ucloop]);
			ret = -ENODEV;
			goto FAIL_FREE_IRQ;
		} else {
			usb2dev_api("irq request ok.\n");
		}
	}

	usb2dev_power_on_init(TRUE);

	#if _USB_KERN_SELFTEST
	// To using embeed usbtest, Add emu_usbtest.o to MakeFile first.
	// And also set _USB_KERN_SELFTEST to 1
	emu_usb_test_open(2);
	#endif

	return ret;

FAIL_FREE_IRQ:

	free_irq(pmodule_info->iinterrupt_id[ucloop], pmodule_info);

	/* Add error handler here */

	return ret;

}

int nvt_usb2dev_drv_remove(USB2DEV_MODULE_INFO *pmodule_info)
{
	unsigned char ucloop;

	usb2dev_api("%s\n", __func__);

	/* Add HW Moduel release operation here*/

	for (ucloop = 0 ; ucloop < MODULE_IRQ_NUM; ucloop++) {
		/* Free IRQ */
		free_irq(pmodule_info->iinterrupt_id[ucloop], pmodule_info);
	}

	usb2dev_uninit();

	return 0;
}

#if 0
int nvt_usb2dev_drv_suspend(USB2DEV_MODULE_INFO *pmodule_info)
{
	nvt_dbg(IND, "\n");

	usb2dev_api("%s\n", __func__);

	/* Add suspend operation here*/

	return 0;
}

int nvt_usb2dev_drv_resume(USB2DEV_MODULE_INFO *pmodule_info)
{
	nvt_dbg(IND, "\n");

	usb2dev_api("%s\n", __func__);

	/* Add resume operation here*/

	return 0;
}
#endif


#if 1
static USB2DEV_EVENT usbioc_event;

static void usb2dev_ioc_open_needed_fifo(void)
{
	FLGPTN uiFlag = 0;

	//usb2dev_api("usb2dev_ioc_open_needed_fifo\r\n");

	// Like the sem lock
	vos_flag_wait_interruptible(&uiFlag, FLG_ID_USBTSK, FLGUSB_USER_EVENT_DONE, TWF_ORW|TWF_CLR);

	// fill event exported here
	usbioc_event.evt_type = USB2DEV_EVT_TYPE_OPEN_NEEDED_FIFO_CB;

	set_flg(FLG_ID_USBTSK, FLGUSB_USER_EVENT_SET);
	vos_flag_wait_interruptible(&uiFlag, FLG_ID_USBTSK, FLGUSB_USER_EVENT_DONE|FLGUSB_CTLTSK_STOP|FLGUSB_SETUPTSK_STOP, TWF_ORW);

}
static void usb2dev_ioc_event_cb(UINT32 event)
{
	FLGPTN uiFlag = 0;

	//usb2dev_api("usb2dev_ioc_event_cb %d\r\n", event);

	// Like the sem lock
	vos_flag_wait_interruptible(&uiFlag, FLG_ID_USBTSK, FLGUSB_USER_EVENT_DONE, TWF_ORW|TWF_CLR);

	// fill event exported here
	usbioc_event.evt_type = USB2DEV_EVT_TYPE_USBEVENT_CB;
	usbioc_event.event = event;

	set_flg(FLG_ID_USBTSK, FLGUSB_USER_EVENT_SET);
	vos_flag_wait_interruptible(&uiFlag, FLG_ID_USBTSK, FLGUSB_USER_EVENT_DONE|FLGUSB_CTLTSK_STOP|FLGUSB_SETUPTSK_STOP, TWF_ORW);

}

static void usb2dev_ioc_suspend_cb(void)
{
	FLGPTN uiFlag = 0;

	//usb2dev_api("usb2dev_ioc_suspend_cb\r\n");

	// Like the sem lock
	vos_flag_wait_interruptible(&uiFlag, FLG_ID_USBTSK, FLGUSB_USER_EVENT_DONE, TWF_ORW|TWF_CLR);

	// fill event exported here
	usbioc_event.evt_type = USB2DEV_EVT_TYPE_SUSPEND_CB;


	set_flg(FLG_ID_USBTSK, FLGUSB_USER_EVENT_SET);
	vos_flag_wait_interruptible(&uiFlag, FLG_ID_USBTSK, FLGUSB_USER_EVENT_DONE|FLGUSB_CTLTSK_STOP|FLGUSB_SETUPTSK_STOP, TWF_ORW);
}

void usb2dev_ioc_charging_cb(UINT32 event)
{
	FLGPTN uiFlag = 0;

	//usb2dev_api("usb2dev_ioc_charging_cb %d\r\n", event);

	// Like the sem lock
	vos_flag_wait_interruptible(&uiFlag, FLG_ID_USBTSK, FLGUSB_USER_EVENT_DONE, TWF_ORW|TWF_CLR);

	// fill event exported here
	usbioc_event.evt_type = USB2DEV_EVT_TYPE_CHARGING_CB;
	usbioc_event.event = event;

	set_flg(FLG_ID_USBTSK, FLGUSB_USER_EVENT_SET);
	vos_flag_wait_interruptible(&uiFlag, FLG_ID_USBTSK, FLGUSB_USER_EVENT_DONE|FLGUSB_CTLTSK_STOP|FLGUSB_SETUPTSK_STOP, TWF_ORW);

}

void usb2dev_ioc_cx_vendor_cb(UINT32 event)
{
	FLGPTN uiFlag = 0;

	//usb2dev_api("usb2dev_ioc_cx_vendor_cb %d\r\n", event);

	// Like the sem lock
	vos_flag_wait_interruptible(&uiFlag, FLG_ID_USBTSK, FLGUSB_USER_EVENT_DONE, TWF_ORW|TWF_CLR);

	// fill event exported here
	usbioc_event.evt_type = USB2DEV_EVT_TYPE_CX_VENDOR;
	usbioc_event.event = event;
	memcpy(usbioc_event.device_request, &usb2dev_control_data.device_request, 8);

	set_flg(FLG_ID_USBTSK, FLGUSB_USER_EVENT_SET);
	vos_flag_wait_interruptible(&uiFlag, FLG_ID_USBTSK, FLGUSB_USER_EVENT_DONE|FLGUSB_CTLTSK_STOP|FLGUSB_SETUPTSK_STOP, TWF_ORW);

}

void usb2dev_ioc_cx_class_cb(void)
{
	FLGPTN uiFlag = 0;

	//usb2dev_api("usb2dev_ioc_cx_class_cb\r\n");

	// Like the sem lock
	vos_flag_wait_interruptible(&uiFlag, FLG_ID_USBTSK, FLGUSB_USER_EVENT_DONE, TWF_ORW|TWF_CLR);

	// fill event exported here
	usbioc_event.evt_type = USB2DEV_EVT_TYPE_CX_CLASS;
	usbioc_event.event = 0;
	memcpy(usbioc_event.device_request, &usb2dev_control_data.device_request, 8);

	set_flg(FLG_ID_USBTSK, FLGUSB_USER_EVENT_SET);
	vos_flag_wait_interruptible(&uiFlag, FLG_ID_USBTSK, FLGUSB_USER_EVENT_DONE|FLGUSB_CTLTSK_STOP|FLGUSB_SETUPTSK_STOP, TWF_ORW);

}

void usb2dev_ioc_set_interface_cb(UINT32 event)
{
	FLGPTN uiFlag = 0;

	//usb2dev_api("usb2dev_ioc_set_interface_cb %d\r\n", event);

	// Like the sem lock
	vos_flag_wait_interruptible(&uiFlag, FLG_ID_USBTSK, FLGUSB_USER_EVENT_DONE, TWF_ORW|TWF_CLR);

	// fill event exported here
	usbioc_event.evt_type = USB2DEV_EVT_TYPE_SET_INTF_CB;
	usbioc_event.event = event;
	memcpy(usbioc_event.device_request, &usb2dev_control_data.device_request, 8);

	set_flg(FLG_ID_USBTSK, FLGUSB_USER_EVENT_SET);
	vos_flag_wait_interruptible(&uiFlag, FLG_ID_USBTSK, FLGUSB_USER_EVENT_DONE|FLGUSB_CTLTSK_STOP|FLGUSB_SETUPTSK_STOP, TWF_ORW);
}

void usb2dev_ioc_std_unknown_cb(UINT32 event)
{
	FLGPTN uiFlag = 0;

	//usb2dev_api("usb2dev_ioc_std_unknown_cb %d\r\n", event);

	// Like the sem lock
	vos_flag_wait_interruptible(&uiFlag, FLG_ID_USBTSK, FLGUSB_USER_EVENT_DONE, TWF_ORW|TWF_CLR);

	// fill event exported here
	usbioc_event.evt_type = USB2DEV_EVT_TYPE_STDUNKNOWN_CB;
	usbioc_event.event = event;
	memcpy(usbioc_event.device_request, &usb2dev_control_data.device_request, 8);

	set_flg(FLG_ID_USBTSK, FLGUSB_USER_EVENT_SET);
	vos_flag_wait_interruptible(&uiFlag, FLG_ID_USBTSK, FLGUSB_USER_EVENT_DONE|FLGUSB_CTLTSK_STOP|FLGUSB_SETUPTSK_STOP, TWF_ORW);

}

#endif

int nvt_usb2dev_drv_ioctl(unsigned char uc_if, USB2DEV_MODULE_INFO *pmodule_info, unsigned int ui_cmd, unsigned long ul_arg)
{
	REG_INFO 		reg_info;
	int 			ret = 0;
	USB2DEV_DESC 	*pdesc;
	UINT32			value;

	//usb2dev_api("%s IF-%d cmd:%x\n", __func__, uc_if, ui_cmd);
	pdesc = kmalloc(sizeof(USB2DEV_DESC), GFP_KERNEL);

	switch (ui_cmd) {
	case USB2DEV_IOC_OPEN: {

		gUSBManage.fp_open_needed_fifo = usb2dev_ioc_open_needed_fifo;
		gUSBManage.fp_event_callback = usb2dev_ioc_event_cb;
		gUSBManage.fp_usb_suspend = usb2dev_ioc_suspend_cb;

		usb2dev_set_callback(USB_CALLBACK_CHARGING_EVENT, 		(USB_GENERIC_CB)usb2dev_ioc_charging_cb);
		usb2dev_set_callback(USB_CALLBACK_CX_VENDOR_REQUEST,	(USB_GENERIC_CB)usb2dev_ioc_cx_vendor_cb);
		usb2dev_set_callback(USB_CALLBACK_CX_CLASS_REQUEST,		(USB_GENERIC_CB)usb2dev_ioc_cx_class_cb);
		usb2dev_set_callback(USB_CALLBACK_SET_INTERFACE,		(USB_GENERIC_CB)usb2dev_ioc_set_interface_cb);
		//usb2dev_set_callback(USB_CALLBACK_READ_QUEUE_DONE,		(USB_GENERIC_CB)usb2dev_ioc_read_queue_done_cb);
		usb2dev_set_callback(USB_CALLBACK_STD_UNKNOWN_REQ,		(USB_GENERIC_CB)usb2dev_ioc_std_unknown_cb);

		/* PreSet flag to make using like as sem */
		set_flg(FLG_ID_USBTSK, FLGUSB_USER_EVENT_DONE);

		if (usb2dev_open() != E_OK) {
			DBG_ERR("USB2DEV_IOC_OPEN error\r\n");
		}
	} break;

	case USB2DEV_IOC_CLOSE: {

		usb2dev_close();

		if(usbioc_cx_data_buf) {
			kfree(usbioc_cx_data_buf);
			usbioc_cx_data_buf = NULL;
		}

		if(usbioc_dev_desc) {
			kfree(usbioc_dev_desc);
			usbioc_dev_desc = NULL;
		}

		if(usbioc_devquali_desc) {
			kfree(usbioc_devquali_desc);
			usbioc_devquali_desc = NULL;
		}

		if(usbioc_cfg_desc_hs) {
			kfree(usbioc_cfg_desc_hs);
			usbioc_cfg_desc_hs = NULL;
		}

		if(usbioc_cfg_desc_hs_oth) {
			kfree(usbioc_cfg_desc_hs_oth);
			usbioc_cfg_desc_hs_oth = NULL;
		}

		if(usbioc_cfg_desc_fs) {
			kfree(usbioc_cfg_desc_fs);
			usbioc_cfg_desc_fs = NULL;
		}

		if(usbioc_cfg_desc_fs_oth) {
			kfree(usbioc_cfg_desc_fs_oth);
			usbioc_cfg_desc_fs_oth = NULL;
		}

		if(usbioc_str_desc0) {
			kfree(usbioc_str_desc0);
			usbioc_str_desc0 = NULL;
		}
		if(usbioc_str_desc1) {
			kfree(usbioc_str_desc1);
			usbioc_str_desc1 = NULL;
		}
		if(usbioc_str_desc2) {
			kfree(usbioc_str_desc2);
			usbioc_str_desc2 = NULL;
		}
		if(usbioc_str_desc3) {
			kfree(usbioc_str_desc3);
			usbioc_str_desc3 = NULL;
		}
		if(usbioc_str_desc4) {
			kfree(usbioc_str_desc4);
			usbioc_str_desc4 = NULL;
		}
		if(usbioc_str_desc5) {
			kfree(usbioc_str_desc5);
			usbioc_str_desc5 = NULL;
		}
		if(usbioc_str_desc6) {
			kfree(usbioc_str_desc6);
			usbioc_str_desc6 = NULL;
		}
		if(usbioc_str_desc7) {
			kfree(usbioc_str_desc7);
			usbioc_str_desc7 = NULL;
		}


		//{
		//	UINT32 i;
		//	for(i=0; i<USB_EP_MAX - 1; i++)
		//		gEPAssignedMap[i] = USB_FIFO_NOT_USE;
		//}

	} break;

	case USB2DEV_IOC_SET_DEVICE_DESC: {
		ret = copy_from_user(pdesc, (void __user *)ul_arg, sizeof(USB2DEV_DESC));
		if (!ret) {
			usb2dev_api("dev desc (%d) 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X\r\n"
				, pdesc->ui_length, pdesc->desc_data[0], pdesc->desc_data[1], pdesc->desc_data[2], pdesc->desc_data[3], pdesc->desc_data[4], pdesc->desc_data[5], pdesc->desc_data[6], pdesc->desc_data[7]);

			if(usbioc_dev_desc == NULL) {
				usbioc_dev_desc = kmalloc(18, GFP_KERNEL);
			}

			if (pdesc->ui_length <=  18)
				memcpy(usbioc_dev_desc, pdesc->desc_data, pdesc->ui_length);

			gUSBManage.p_dev_desc = usbioc_dev_desc;
		}
	} break;

	case USB2DEV_IOC_SET_CFG_HS_DESC: {
		ret = copy_from_user(pdesc, (void __user *)ul_arg, sizeof(USB2DEV_DESC));
		if (!ret) {
			usb2dev_api("cfg hs desc (%d) 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X\r\n"
				, pdesc->ui_length, pdesc->desc_data[0], pdesc->desc_data[1], pdesc->desc_data[2], pdesc->desc_data[3], pdesc->desc_data[4], pdesc->desc_data[5], pdesc->desc_data[6], pdesc->desc_data[7]);

			if(usbioc_cfg_desc_hs == NULL) {
				usbioc_cfg_desc_hs = kmalloc(USB_DESC_MAX_SIZE, GFP_KERNEL);
			}

			if (pdesc->ui_length <=  USB_DESC_MAX_SIZE)
				memcpy(usbioc_cfg_desc_hs, pdesc->desc_data, pdesc->ui_length);

			gUSBManage.p_config_desc_hs = (USB_CONFIG_DESC *)usbioc_cfg_desc_hs;
		}
	} break;

	case USB2DEV_IOC_SET_CFG_HS_OTHER_DESC: {
		ret = copy_from_user(pdesc, (void __user *)ul_arg, sizeof(USB2DEV_DESC));
		if (!ret) {
			usb2dev_api("cfg hs desc oth (%d) 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X\r\n"
				, pdesc->ui_length, pdesc->desc_data[0], pdesc->desc_data[1], pdesc->desc_data[2], pdesc->desc_data[3], pdesc->desc_data[4], pdesc->desc_data[5], pdesc->desc_data[6], pdesc->desc_data[7]);

			if(usbioc_cfg_desc_hs_oth == NULL) {
				usbioc_cfg_desc_hs_oth = kmalloc(USB_DESC_MAX_SIZE, GFP_KERNEL);
			}

			if (pdesc->ui_length <=  USB_DESC_MAX_SIZE)
				memcpy(usbioc_cfg_desc_hs_oth, pdesc->desc_data, pdesc->ui_length);

			gUSBManage.p_config_desc_hs_other = (USB_CONFIG_DESC *)usbioc_cfg_desc_hs_oth;
		}
	}break;

	case USB2DEV_IOC_SET_CFG_FS_DESC: {
		ret = copy_from_user(pdesc, (void __user *)ul_arg, sizeof(USB2DEV_DESC));
		if (!ret) {
			usb2dev_api("cfg fs desc (%d) 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X\r\n"
				, pdesc->ui_length, pdesc->desc_data[0], pdesc->desc_data[1], pdesc->desc_data[2], pdesc->desc_data[3], pdesc->desc_data[4], pdesc->desc_data[5], pdesc->desc_data[6], pdesc->desc_data[7]);

			if(usbioc_cfg_desc_fs == NULL) {
				usbioc_cfg_desc_fs = kmalloc(USB_DESC_MAX_SIZE, GFP_KERNEL);
			}

			if (pdesc->ui_length <=  USB_DESC_MAX_SIZE)
				memcpy(usbioc_cfg_desc_fs, pdesc->desc_data, pdesc->ui_length);

			gUSBManage.p_config_desc_fs = (USB_CONFIG_DESC *)usbioc_cfg_desc_fs;
		}
	} break;

	case USB2DEV_IOC_SET_CFG_FS_OTHER_DESC: {
		ret = copy_from_user(pdesc, (void __user *)ul_arg, sizeof(USB2DEV_DESC));
		if (!ret) {
			usb2dev_api("cfg hs desc oth (%d) 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X\r\n"
				, pdesc->ui_length, pdesc->desc_data[0], pdesc->desc_data[1], pdesc->desc_data[2], pdesc->desc_data[3], pdesc->desc_data[4], pdesc->desc_data[5], pdesc->desc_data[6], pdesc->desc_data[7]);

			if(usbioc_cfg_desc_fs_oth == NULL) {
				usbioc_cfg_desc_fs_oth = kmalloc(USB_DESC_MAX_SIZE, GFP_KERNEL);
			}

			if (pdesc->ui_length <=  USB_DESC_MAX_SIZE)
				memcpy(usbioc_cfg_desc_fs_oth, pdesc->desc_data, pdesc->ui_length);

			gUSBManage.p_config_desc_fs_other = (USB_CONFIG_DESC *)usbioc_cfg_desc_fs_oth;
		}
	} break;

	case USB2DEV_IOC_SET_DEV_QUALI_DESC: {
		ret = copy_from_user(pdesc, (void __user *)ul_arg, sizeof(USB2DEV_DESC));
		if (!ret) {
			usb2dev_api("dev qua desc (%d) 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X\r\n"
				, pdesc->ui_length, pdesc->desc_data[0], pdesc->desc_data[1], pdesc->desc_data[2], pdesc->desc_data[3], pdesc->desc_data[4], pdesc->desc_data[5], pdesc->desc_data[6], pdesc->desc_data[7]);

			if(usbioc_devquali_desc == NULL) {
				usbioc_devquali_desc = kmalloc(18, GFP_KERNEL);
			}

			if (pdesc->ui_length <=  18)
				memcpy(usbioc_devquali_desc, pdesc->desc_data, pdesc->ui_length);

			gUSBManage.p_dev_quali_desc = (USB_DEVICE_DESC *)usbioc_devquali_desc;
		}
	} break;

	case USB2DEV_IOC_SET_STRING0_DESC: {
		ret = copy_from_user(pdesc, (void __user *)ul_arg, sizeof(USB2DEV_DESC));
		if (!ret) {
			usb2dev_api("str-0 desc (%d) 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X\r\n"
				, pdesc->ui_length, pdesc->desc_data[0], pdesc->desc_data[1], pdesc->desc_data[2], pdesc->desc_data[3], pdesc->desc_data[4], pdesc->desc_data[5], pdesc->desc_data[6], pdesc->desc_data[7]);

			if(usbioc_str_desc0 == NULL) {
				usbioc_str_desc0 = kmalloc(512, GFP_KERNEL);
			}

			if (pdesc->ui_length <=  512) {
				memcpy(usbioc_str_desc0, pdesc->desc_data, pdesc->ui_length);
				gUSBManage.p_string_desc[0] = (USB_STRING_DESC *)usbioc_str_desc0;
			} else {
				DBG_ERR("str0 size exceed 512\r\n");
			}
		}
	} break;
	case USB2DEV_IOC_SET_STRING1_DESC: {
		ret = copy_from_user(pdesc, (void __user *)ul_arg, sizeof(USB2DEV_DESC));
		if (!ret) {
			usb2dev_api("str-1 desc (%d) 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X\r\n"
				, pdesc->ui_length, pdesc->desc_data[0], pdesc->desc_data[1], pdesc->desc_data[2], pdesc->desc_data[3], pdesc->desc_data[4], pdesc->desc_data[5], pdesc->desc_data[6], pdesc->desc_data[7]);

			if(usbioc_str_desc1 == NULL) {
				usbioc_str_desc1 = kmalloc(512, GFP_KERNEL);
			}

			if (pdesc->ui_length <=  512) {
				memcpy(usbioc_str_desc1, pdesc->desc_data, pdesc->ui_length);
				gUSBManage.p_string_desc[1] = (USB_STRING_DESC *)usbioc_str_desc1;
			} else {
				DBG_ERR("str1 size exceed 512\r\n");
			}
		}
	} break;
	case USB2DEV_IOC_SET_STRING2_DESC: {
		ret = copy_from_user(pdesc, (void __user *)ul_arg, sizeof(USB2DEV_DESC));
		if (!ret) {
			usb2dev_api("str-2 desc (%d) 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X\r\n"
				, pdesc->ui_length, pdesc->desc_data[0], pdesc->desc_data[1], pdesc->desc_data[2], pdesc->desc_data[3], pdesc->desc_data[4], pdesc->desc_data[5], pdesc->desc_data[6], pdesc->desc_data[7]);

			if(usbioc_str_desc2 == NULL) {
				usbioc_str_desc2 = kmalloc(512, GFP_KERNEL);
			}

			if (pdesc->ui_length <=  512) {
				memcpy(usbioc_str_desc2, pdesc->desc_data, pdesc->ui_length);
				gUSBManage.p_string_desc[2] = (USB_STRING_DESC *)usbioc_str_desc2;
			} else {
				DBG_ERR("str2 size exceed 512\r\n");
			}
		}
	} break;
	case USB2DEV_IOC_SET_STRING3_DESC: {
		ret = copy_from_user(pdesc, (void __user *)ul_arg, sizeof(USB2DEV_DESC));
		if (!ret) {
			usb2dev_api("str-3 desc (%d) 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X\r\n"
				, pdesc->ui_length, pdesc->desc_data[0], pdesc->desc_data[1], pdesc->desc_data[2], pdesc->desc_data[3], pdesc->desc_data[4], pdesc->desc_data[5], pdesc->desc_data[6], pdesc->desc_data[7]);

			if(usbioc_str_desc3 == NULL) {
				usbioc_str_desc3 = kmalloc(512, GFP_KERNEL);
			}

			if (pdesc->ui_length <=  512) {
				memcpy(usbioc_str_desc3, pdesc->desc_data, pdesc->ui_length);
				gUSBManage.p_string_desc[3] = (USB_STRING_DESC *)usbioc_str_desc3;
			} else {
				DBG_ERR("str3 size exceed 512\r\n");
			}
		}
	} break;
	case USB2DEV_IOC_SET_STRING4_DESC: {
		ret = copy_from_user(pdesc, (void __user *)ul_arg, sizeof(USB2DEV_DESC));
		if (!ret) {
			usb2dev_api("str-4 desc (%d) 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X\r\n"
				, pdesc->ui_length, pdesc->desc_data[0], pdesc->desc_data[1], pdesc->desc_data[2], pdesc->desc_data[3], pdesc->desc_data[4], pdesc->desc_data[5], pdesc->desc_data[6], pdesc->desc_data[7]);

			if(usbioc_str_desc4 == NULL) {
				usbioc_str_desc4 = kmalloc(512, GFP_KERNEL);
			}

			if (pdesc->ui_length <=  512) {
				memcpy(usbioc_str_desc4, pdesc->desc_data, pdesc->ui_length);
				gUSBManage.p_string_desc[4] = (USB_STRING_DESC *)usbioc_str_desc4;
			} else {
				DBG_ERR("str4 size exceed 512\r\n");
			}
		}
	} break;
	case USB2DEV_IOC_SET_STRING5_DESC: {
		ret = copy_from_user(pdesc, (void __user *)ul_arg, sizeof(USB2DEV_DESC));
		if (!ret) {
			usb2dev_api("str-5 desc (%d) 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X\r\n"
				, pdesc->ui_length, pdesc->desc_data[0], pdesc->desc_data[1], pdesc->desc_data[2], pdesc->desc_data[3], pdesc->desc_data[4], pdesc->desc_data[5], pdesc->desc_data[6], pdesc->desc_data[7]);

			if(usbioc_str_desc5 == NULL) {
				usbioc_str_desc5 = kmalloc(512, GFP_KERNEL);
			}

			if (pdesc->ui_length <=  512) {
				memcpy(usbioc_str_desc5, pdesc->desc_data, pdesc->ui_length);
				gUSBManage.p_string_desc[5] = (USB_STRING_DESC *)usbioc_str_desc5;
			} else {
				DBG_ERR("str5 size exceed 512\r\n");
			}
		}
	} break;
	case USB2DEV_IOC_SET_STRING6_DESC: {
		ret = copy_from_user(pdesc, (void __user *)ul_arg, sizeof(USB2DEV_DESC));
		if (!ret) {
			usb2dev_api("str-6 desc (%d) 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X\r\n"
				, pdesc->ui_length, pdesc->desc_data[0], pdesc->desc_data[1], pdesc->desc_data[2], pdesc->desc_data[3], pdesc->desc_data[4], pdesc->desc_data[5], pdesc->desc_data[6], pdesc->desc_data[7]);

			if(usbioc_str_desc6 == NULL) {
				usbioc_str_desc6 = kmalloc(512, GFP_KERNEL);
			}

			if (pdesc->ui_length <=  512) {
				memcpy(usbioc_str_desc6, pdesc->desc_data, pdesc->ui_length);
				gUSBManage.p_string_desc[6] = (USB_STRING_DESC *)usbioc_str_desc6;
			} else {
				DBG_ERR("str6 size exceed 512\r\n");
			}
		}
	} break;
	case USB2DEV_IOC_SET_STRING7_DESC: {
		ret = copy_from_user(pdesc, (void __user *)ul_arg, sizeof(USB2DEV_DESC));
		if (!ret) {
			usb2dev_api("str-7 desc (%d) 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X\r\n"
				, pdesc->ui_length, pdesc->desc_data[0], pdesc->desc_data[1], pdesc->desc_data[2], pdesc->desc_data[3], pdesc->desc_data[4], pdesc->desc_data[5], pdesc->desc_data[6], pdesc->desc_data[7]);

			if(usbioc_str_desc7 == NULL) {
				usbioc_str_desc7 = kmalloc(512, GFP_KERNEL);
			}

			if (pdesc->ui_length <=  512) {
				memcpy(usbioc_str_desc7, pdesc->desc_data, pdesc->ui_length);
				gUSBManage.p_string_desc[7] = (USB_STRING_DESC *)usbioc_str_desc7;
			} else {
				DBG_ERR("str7 size exceed 512\r\n");
			}
		}
	} break;


	case USB2DEV_IOC_SET_EPCFG_HS: {
		ret = copy_from_user(&gUSBManage.ep_config_hs, (void __user *)ul_arg, sizeof(USB2DEV_EPCFG));
		if (!ret) {
			usb2dev_api("en=%d blksz=%d bkno=%d\r\n",gUSBManage.ep_config_hs[0].enable,gUSBManage.ep_config_hs[0].blk_size,gUSBManage.ep_config_hs[0].blk_num);
			usb2dev_api("en=%d blksz=%d bkno=%d\r\n",gUSBManage.ep_config_hs[1].enable,gUSBManage.ep_config_hs[1].blk_size,gUSBManage.ep_config_hs[1].blk_num);
			usb2dev_api("en=%d blksz=%d bkno=%d\r\n",gUSBManage.ep_config_hs[2].enable,gUSBManage.ep_config_hs[2].blk_size,gUSBManage.ep_config_hs[2].blk_num);
			usb2dev_api("en=%d blksz=%d bkno=%d\r\n",gUSBManage.ep_config_hs[3].enable,gUSBManage.ep_config_hs[3].blk_size,gUSBManage.ep_config_hs[3].blk_num);
		}
	} break;
	case USB2DEV_IOC_SET_EPCFG_FS: {
		ret = copy_from_user(&gUSBManage.ep_config_fs, (void __user *)ul_arg, sizeof(USB2DEV_EPCFG));
		if (!ret) {
			usb2dev_api("en=%d blksz=%d bkno=%d\r\n",gUSBManage.ep_config_fs[0].enable,gUSBManage.ep_config_fs[0].blk_size,gUSBManage.ep_config_fs[0].blk_num);
			usb2dev_api("en=%d blksz=%d bkno=%d\r\n",gUSBManage.ep_config_fs[1].enable,gUSBManage.ep_config_fs[1].blk_size,gUSBManage.ep_config_fs[1].blk_num);
			usb2dev_api("en=%d blksz=%d bkno=%d\r\n",gUSBManage.ep_config_fs[2].enable,gUSBManage.ep_config_fs[2].blk_size,gUSBManage.ep_config_fs[2].blk_num);
			usb2dev_api("en=%d blksz=%d bkno=%d\r\n",gUSBManage.ep_config_fs[3].enable,gUSBManage.ep_config_fs[3].blk_size,gUSBManage.ep_config_fs[3].blk_num);
		}
	} break;

	case USB2DEV_IOC_SET_CFG_NUMBER: {
		ret = copy_from_user(&value, (void __user *)ul_arg, sizeof(UINT32));
		if (!ret) {
			usb2dev_api("Cfg No =%d\r\n", value);
			gUSBManage.num_of_configurations = (UINT16)value;
		}
	} break;

	case USB2DEV_IOC_SET_STRING_NUMBER: {
		ret = copy_from_user(&value, (void __user *)ul_arg, sizeof(UINT32));
		if (!ret) {
			usb2dev_api("Str No =%d\r\n", value);
			gUSBManage.num_of_strings = (UINT16)value;
		}
	} break;



	case USB2DEV_IOC_GET_EVENT: {
		FLGPTN uiFlag;

		vos_flag_wait_interruptible(&uiFlag, FLG_ID_USBTSK, FLGUSB_USER_EVENT_SET, TWF_ORW|TWF_CLR);
		ret = copy_to_user((void __user *)ul_arg, &usbioc_event, sizeof(USB2DEV_EVENT));
		if (ret != 0)
			DBG_ERR("copy_to_user\n");

	} break;

	case USB2DEV_IOC_SET_EVENT_DONE: {
		set_flg(FLG_ID_USBTSK, FLGUSB_USER_EVENT_DONE);
	} break;



	case USB2DEV_IOC_CX_RET_DATA: {
		USB2DEV_CX_RET_DATA ret_data;

		ret = copy_from_user(&ret_data, (void __user *)ul_arg, sizeof(USB2DEV_CX_RET_DATA));

		if (!ret) {

			if(usbioc_cx_data_buf) {
				kfree(usbioc_cx_data_buf);
				usbioc_cx_data_buf = NULL;
			}

			if (ret_data.ui_length < 20480) {
				usbioc_cx_data_buf = (UINT8 *)kmalloc(ret_data.ui_length, GFP_KERNEL);

				if(usbioc_cx_data_buf != NULL) {
					ret = copy_from_user(usbioc_cx_data_buf, (void __user *)ret_data.p_data, ret_data.ui_length);
					if (!ret) {

						usb2dev_control_data.p_data = usbioc_cx_data_buf;
						usb2dev_control_data.w_length = ret_data.ui_length;

						usb2dev_reture_setup_data();
					}
				} else {
					DBG_ERR("No mem\n");
				}
			} else {
					DBG_ERR("request larger than 20KB buf\n");
			}

		}

	} break;


	case USB2DEV_IOC_SET_CX_DONE: {
		usb2dev_set_ep0_done();
	} break;

	case USB2DEV_IOC_SET_UNMASK_EPINT: {
		ret = copy_from_user(&value, (void __user *)ul_arg, sizeof(UINT32));
		if (!ret) {
			usb2dev_unmask_ep_interrupt((USB_EP) value);
		}
	} break;
	case USB2DEV_IOC_SET_MASK_EPINT: {
		ret = copy_from_user(&value, (void __user *)ul_arg, sizeof(UINT32));
		if (!ret) {
			usb2dev_mask_ep_interrupt((USB_EP) value);
		}
	} break;

	case USB2DEV_IOC_SET_EPCFG: {
		USB2DEV_SETEPCFG ep_cfg;

		ret = copy_from_user(&ep_cfg, (void __user *)ul_arg, sizeof(USB2DEV_SETEPCFG));
		if (!ret) {
			if (ep_cfg.epn < USB_EP_MAX)
				usb2dev_set_ep_config(ep_cfg.epn, ep_cfg.cfgid, ep_cfg.cfg_value);
		}
	} break;

	case USB2DEV_IOC_SET_CFG: {
		USB2DEV_SETCFG _cfg;

		ret = copy_from_user(&_cfg, (void __user *)ul_arg, sizeof(USB2DEV_SETCFG));
		if (!ret) {
				usb2dev_set_config(_cfg.cfgid, _cfg.cfg_value);
		}

	} break;

	case USB2DEV_IOC_GET_CONTROL_STATE: {
		value = (UINT32) usb2dev_get_controller_state();
		ret = copy_to_user((void __user *)ul_arg, &value, sizeof(UINT32));
		if (ret != 0)
			DBG_ERR("copy_to_user\n");

	} break;

	case USB2DEV_IOC_SET_EP_R: {
		USB2DEV_DATA_RW data_rw;
		UINT32 dma_len = 0;

		ret = copy_from_user(&data_rw, (void __user *)ul_arg, sizeof(USB2DEV_DATA_RW));
		if (!ret) {

			ret = copy_from_user(&dma_len, (void __user *)data_rw.dma_length, sizeof(UINT32));

			if (!ret)
				usb2dev_set_ep_read(data_rw.epn, data_rw.buffer, &dma_len);
		}
	} break;
	case USB2DEV_IOC_SET_EP_W: {
		USB2DEV_DATA_RW data_rw;
		UINT32 dma_len = 0;

		ret = copy_from_user(&data_rw, (void __user *)ul_arg, sizeof(USB2DEV_DATA_RW));
		if (!ret) {

			ret = copy_from_user(&dma_len, (void __user *)data_rw.dma_length, sizeof(UINT32));

			if (!ret)
				usb2dev_set_ep_write(data_rw.epn, data_rw.buffer, &dma_len);
		}
	} break;
	case USB2DEV_IOC_READ_EP: {
		USB2DEV_DATA_RW data_rw;
		UINT32 dma_len = 0;


		ret = copy_from_user(&data_rw, (void __user *)ul_arg, sizeof(USB2DEV_DATA_RW));
		if (!ret) {

			ret = copy_from_user(&dma_len, (void __user *)data_rw.dma_length, sizeof(UINT32));

			if (!ret) {

				if (data_rw.epn == USB_EP0) {
					memset(pmodule_info->ctrl_buff, 0x0, 64);

					if(dma_len <= 64) {
						usb2dev_read_endpoint(data_rw.epn, pmodule_info->ctrl_buff, &dma_len);
						ret = copy_to_user((void __user *)data_rw.buffer, pmodule_info->ctrl_buff, dma_len);
						if (ret != 0)
							DBG_ERR("copy_to_user\n");

					} else {
						DBG_ERR("ep0 r sz err %d\n",dma_len);
					}
				} else {
					usb2dev_read_endpoint(data_rw.epn, data_rw.buffer, &dma_len);
				}
				ret = copy_to_user((void __user *)data_rw.dma_length, &dma_len, sizeof(UINT32));
				if (ret != 0)
					DBG_ERR("copy_to_user\n");

			}
		}
	} break;
	case USB2DEV_IOC_READ_EP_TMOT: {
		USB2DEV_DATA_RW data_rw;
		UINT32 dma_len = 0;


		ret = copy_from_user(&data_rw, (void __user *)ul_arg, sizeof(USB2DEV_DATA_RW));
		if (!ret) {

			ret = copy_from_user(&dma_len, (void __user *)data_rw.dma_length, sizeof(UINT32));

			if (!ret) {

				if (data_rw.epn != USB_EP0) {
					usb2dev_read_endpoint_timeout(data_rw.epn, data_rw.buffer, &dma_len, data_rw.timeout_ms);
				}

				ret = copy_to_user((void __user *)data_rw.dma_length, &dma_len, sizeof(UINT32));
				if (ret != 0)
					DBG_ERR("copy_to_user\n");

			}
		}
	} break;

	case USB2DEV_IOC_WRITE_EP: {
		USB2DEV_DATA_RW data_rw;
		UINT32 dma_len = 0;

		ret = copy_from_user(&data_rw, (void __user *)ul_arg, sizeof(USB2DEV_DATA_RW));
		if (!ret) {

			ret = copy_from_user(&dma_len, (void __user *)data_rw.dma_length, sizeof(UINT32));

			if (!ret) {

				if (data_rw.epn == USB_EP0) {
					memset(pmodule_info->ctrl_buff, 0x0, 64);

					if(dma_len <= 64) {
						ret = copy_from_user(pmodule_info->ctrl_buff, (void __user *)data_rw.buffer,  dma_len);
						if (ret != 0)
							DBG_ERR("copy_from_user\n");

						usb2dev_write_endpoint(data_rw.epn, pmodule_info->ctrl_buff, &dma_len);

					} else {
						DBG_ERR("ep0 r sz err %d\n",dma_len);
					}
				} else {
					usb2dev_write_endpoint(data_rw.epn, data_rw.buffer, &dma_len);
				}
				ret = copy_to_user((void __user *)data_rw.dma_length, &dma_len, sizeof(UINT32));
				if (ret != 0)
					DBG_ERR("copy_to_user\n");

			}
		}
	} break;
	case USB2DEV_IOC_WRITE_EP_TMOT: {
		USB2DEV_DATA_RW data_rw;
		UINT32 dma_len = 0;

		ret = copy_from_user(&data_rw, (void __user *)ul_arg, sizeof(USB2DEV_DATA_RW));
		if (!ret) {

			ret = copy_from_user(&dma_len, (void __user *)data_rw.dma_length, sizeof(UINT32));

			if (!ret) {

				if (data_rw.epn != USB_EP0) {
					usb2dev_write_endpoint_timeout(data_rw.epn, data_rw.buffer, &dma_len, data_rw.timeout_ms);
				}
				ret = copy_to_user((void __user *)data_rw.dma_length, &dma_len, sizeof(UINT32));
				if (ret != 0)
					DBG_ERR("copy_to_user\n");

			}
		}
	} break;

	case USB2DEV_IOC_ABORT_EP: {
		ret = copy_from_user(&value, (void __user *)ul_arg, sizeof(UINT32));
		if (!ret) {
			if (value < USB_EP_MAX)
				usb2dev_abort_endpoint((USB_EP) value);
		}
	} break;

	case USB2DEV_IOC_GET_BYTECNT: {
		ret = copy_from_user(&value, (void __user *)ul_arg, sizeof(UINT32));
		if (!ret) {
			value = usb2dev_get_ep_bytecount((USB_EP) value);
			ret = copy_to_user((void __user *)ul_arg, &value, sizeof(UINT32));
			if (ret != 0)
				DBG_ERR("copy_to_user\n");

		}
	} break;

	case USB2DEV_IOC_STALL_EP: {
		ret = copy_from_user(&value, (void __user *)ul_arg, sizeof(UINT32));
		if (!ret) {
			usb2dev_set_ep_stall((USB_EP) value);
		}

	} break;

	case USB2DEV_IOC_TX0_BYTE: {
		ret = copy_from_user(&value, (void __user *)ul_arg, sizeof(UINT32));
		if (!ret) {
			usb2dev_set_tx0byte((USB_EP) value);
		}
	} break;

	case USB2DEV_IOC_CHK_EPBUSY: {
		ret = copy_from_user(&value, (void __user *)ul_arg, sizeof(UINT32));
		if (!ret) {
			value = usb2dev_check_ep_busy((USB_EP) value);
			ret = copy_to_user((void __user *)ul_arg, &value, sizeof(UINT32));
			if (ret != 0)
				DBG_ERR("copy_to_user\n");

		}
	} break;

	case USB2DEV_IOC_CLEAR_EPFIFO: {
		ret = copy_from_user(&value, (void __user *)ul_arg, sizeof(UINT32));
		if (!ret) {
			usb2dev_clear_ep_fifo((USB_EP) value);
		}
	} break;

	case USB2DEV_IOC_GET_SOF: {
		value = usb2dev_get_sof_number();
		ret = copy_to_user((void __user *)ul_arg, &value, sizeof(UINT32));
		if (ret != 0)
			DBG_ERR("copy_to_user\n");

	} break;

	case USB2DEV_IOC_STATE_CHG: {
		value = usb2dev_state_change();
		ret = copy_to_user((void __user *)ul_arg, &value, sizeof(UINT32));
		if (ret != 0)
			DBG_ERR("copy_to_user\n");

	} break;

	case USB2DEV_IOC_CHK_CHARGER: {
		ret = copy_from_user(&value, (void __user *)ul_arg, sizeof(UINT32));
		if (!ret) {
			value = usb2dev_check_charger(value);
			ret = copy_to_user((void __user *)ul_arg, &value, sizeof(UINT32));
			if (ret != 0)
				DBG_ERR("copy_to_user\n");

		}
	} break;

	case USB2DEV_IOC_IS_HIGHSPEED: {
		value = usb2dev_is_highspeed_enabled();
		ret = copy_to_user((void __user *)ul_arg, &value, sizeof(UINT32));
		if (ret != 0)
			DBG_ERR("copy_to_user\n");
	} break;

	case USB2DEV_IOC_READ_REG:
		ret = copy_from_user(&reg_info, (void __user *)ul_arg, sizeof(REG_INFO));
		if (!ret) {
			reg_info.ui_value = READ_REG(pmodule_info->io_addr[uc_if] + reg_info.ui_addr);
			ret = copy_to_user((void __user *)ul_arg, &reg_info, sizeof(REG_INFO));
			if (ret != 0)
				DBG_ERR("copy_to_user\n");
		}
		break;

	case USB2DEV_IOC_WRITE_REG:
		ret = copy_from_user(&reg_info, (void __user *)ul_arg, sizeof(REG_INFO));
		if (!ret) {
			WRITE_REG(reg_info.ui_value, pmodule_info->io_addr[uc_if] + reg_info.ui_addr);
		}
		break;

	case USB2DEV_IOC_CHK_CXOUT:
		value = usb2dev_get_config(USB_CONFIG_ID_CHECK_CXOUT);
		ret = copy_to_user((void __user *)ul_arg, &value, sizeof(UINT32));
		if (ret != 0)
			DBG_ERR("copy_to_user\n");
		break;

	default:
		break;


		/* Add other operations here */
	}

	kfree(pdesc);
	return ret;
}


extern irqreturn_t usb_isr(int irq, void *devid);

irqreturn_t nvt_usb2dev_drv_isr(int irq, void *devid)
{
	return usb_isr(irq, devid);
}



