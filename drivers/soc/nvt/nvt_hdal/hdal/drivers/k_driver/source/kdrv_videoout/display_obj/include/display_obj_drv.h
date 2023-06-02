#ifndef __DISPOBJ_DRV_H__
#define __DISPOBJ_DRV_H__
#include <linux/io.h>
#include <linux/spinlock.h>
#include <linux/semaphore.h>
#include <linux/interrupt.h>
#include <linux/completion.h>
#include <linux/clk.h>

#define MODULE_REG_NUM          1

#define WRITE_REG(value, addr)  iowrite32(value, addr)
#define READ_REG(addr)          ioread32(addr)

typedef struct module_info {
	struct completion dispobj_completion;
	struct semaphore dispobj_sem;
	wait_queue_head_t dispobj_wait_queue;
	spinlock_t dispobj_spinlock;
} MODULE_INFO, *PMODULE_INFO;

int nvt_dispobj_drv_open(PMODULE_INFO pmodule_info, unsigned char uc_if);
int nvt_dispobj_drv_release(PMODULE_INFO pmodule_info, unsigned char uc_if);
int nvt_dispobj_drv_init(PMODULE_INFO pmodule_info);
int nvt_dispobj_drv_remove(PMODULE_INFO pmodule_info);
int nvt_dispobj_drv_suspend(MODULE_INFO *pmodule_info);
int nvt_dispobj_drv_resume(MODULE_INFO *pmodule_info);
int nvt_dispobj_drv_ioctl(unsigned char uc_if, MODULE_INFO *pmodule_info, unsigned int ui_cmd, unsigned long ui_arg);
int nvt_dispobj_drv_write_reg(PMODULE_INFO pmodule_info, unsigned long addr, unsigned long value);
int nvt_dispobj_drv_read_reg(PMODULE_INFO pmodule_info, unsigned long addr);
int nvt_dispobj_drv_lcd_open(PMODULE_INFO pmodule_info, unsigned int open);
int nvt_dispobj_drv_tv_open(PMODULE_INFO pmodule_info, unsigned int open);
int nvt_dispobj_drv_fill_background(PMODULE_INFO pmodule_info, unsigned int color);
int nvt_dispobj_drv_fill_video(PMODULE_INFO pmodule_info, unsigned int format);
int nvt_dispobj_drv_fill_video2(PMODULE_INFO pmodule_info, unsigned int format);
int nvt_dispobj_drv_fill_osd(PMODULE_INFO pmodule_info, unsigned int format);
int nvt_dispobj_drv_v1_scaling(PMODULE_INFO pmodule_info, unsigned int w, unsigned int h, unsigned int window);
int nvt_dispobj_drv_v2_scaling(PMODULE_INFO pmodule_info, unsigned int w, unsigned int h, unsigned int window);
int nvt_dispobj_drv_osd_scaling(PMODULE_INFO pmodule_info, unsigned int w, unsigned int h, unsigned int window);
#endif

