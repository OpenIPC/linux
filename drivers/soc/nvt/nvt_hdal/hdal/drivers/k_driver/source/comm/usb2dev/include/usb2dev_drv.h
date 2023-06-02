#ifndef __USB_CHRGDET_DRV_H__
#define __USB_CHRGDET_DRV_H__
#include <linux/io.h>
#include <linux/spinlock.h>
#include <linux/semaphore.h>
#include <linux/interrupt.h>
#include <linux/completion.h>
#include <linux/clk.h>

#define MODULE_IRQ_NUM          1
#define MODULE_REG_NUM          1
#define MODULE_CLK_NUM          0

#define WRITE_REG(value, addr)  iowrite32(value, addr)
#define READ_REG(addr)          ioread32(addr)

typedef struct usb2dev_module_info {
	void __iomem *io_addr[MODULE_REG_NUM];
	int iinterrupt_id[MODULE_IRQ_NUM];
	unsigned char *ctrl_buff;
} USB2DEV_MODULE_INFO, *PUSB2DEV_MODULE_INFO;

int nvt_usb2dev_drv_open(PUSB2DEV_MODULE_INFO pmodule_info, unsigned char uc_if);
int nvt_usb2dev_drv_release(PUSB2DEV_MODULE_INFO pmodule_info, unsigned char uc_if);
int nvt_usb2dev_drv_init(PUSB2DEV_MODULE_INFO pmodule_info);
int nvt_usb2dev_drv_remove(PUSB2DEV_MODULE_INFO pmodule_info);
//int nvt_usb2dev_drv_suspend(USB2DEV_MODULE_INFO *pmodule_info);
//int nvt_usb2dev_drv_resume(USB2DEV_MODULE_INFO *pmodule_info);
int nvt_usb2dev_drv_ioctl(unsigned char uc_if, USB2DEV_MODULE_INFO *pmodule_info, unsigned int ui_cmd, unsigned long ul_arg);
#endif

