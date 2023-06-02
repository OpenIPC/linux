#ifndef __JPG_DRV_H__
#define __JPG_DRV_H__


#ifdef __KERNEL__
#include <linux/io.h>
#include <linux/spinlock.h>
#include <linux/semaphore.h>
#include <linux/interrupt.h>
#include <linux/completion.h>
#include <linux/clk.h>
#endif
#include "jpeg_int.h"

#define MODULE_IRQ_NUM          1
#define MODULE_REG_NUM          1
#define MODULE_CLK_NUM          1

#define WRITE_REG(value, addr) iowrite32(value, addr)
#define READ_REG(addr) ioread32(addr)



//#define NVT_XXX_DBG printk
//#define NVT_XXX_DBG(_fmt, args...)

typedef struct jpg_module_info {
	struct completion jpg_completion;
	struct semaphore jpg_sem;
	struct clk *pclk[MODULE_CLK_NUM];
	struct tasklet_struct jpg_tasklet;
	void __iomem *io_addr[MODULE_REG_NUM];
	int iinterrupt_id[MODULE_IRQ_NUM];
	wait_queue_head_t jpg_wait_queue;
	spinlock_t jpg_spinlock;
} JPG_MODULE_INFO, *PJPG_MODULE_INFO;


extern JPG_MODULE_INFO *jpeg_module_info;

int nvt_jpg_drv_open(PJPG_MODULE_INFO pmodule_info, unsigned char interface);
int nvt_jpg_drv_release(PJPG_MODULE_INFO pmodule_info, unsigned char interface);
int nvt_jpg_drv_init(PJPG_MODULE_INFO pmodule_info);
int nvt_jpg_drv_remove(PJPG_MODULE_INFO pmodule_info);
int nvt_jpg_drv_suspend(JPG_MODULE_INFO *pmodule_info);
int nvt_jpg_drv_resume(JPG_MODULE_INFO *pmodule_info);
int nvt_jpg_drv_regist_irq(JPG_MODULE_INFO *pmodule_info);
int nvt_jpg_drv_ioctl(unsigned char interface, JPG_MODULE_INFO *pmodule_info, unsigned int cmd, unsigned long arg);



#endif

