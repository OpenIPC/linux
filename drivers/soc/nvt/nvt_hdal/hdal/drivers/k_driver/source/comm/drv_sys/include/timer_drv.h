#ifndef __MODULE_DRV_H__
#define __MODULE_DRV_H__
#if !defined __FREERTOS
#include <linux/io.h>
#include <linux/spinlock.h>
#include <linux/semaphore.h>
#include <linux/interrupt.h>
#include <linux/completion.h>
#include <linux/clk.h>

#define MODULE_IRQ_NUM          1
#define MODULE_REG_NUM          1
#define MODULE_CLK_NUM          1

#define WRITE_REG(VALUE, ADDR)  iowrite32(VALUE, ADDR)
#define READ_REG(ADDR)          ioread32(ADDR)

typedef struct module_info {
	//struct completion xxx_completion;
	//struct semaphore xxx_sem;
	struct clk *pclk[MODULE_CLK_NUM];
	struct tasklet_struct tasklet;
	void __iomem *io_addr[MODULE_REG_NUM];
	int iinterrupt_id[MODULE_IRQ_NUM];
	wait_queue_head_t wait_queue;
	//spinlock_t xxx_spinlock;
	struct list_head stTimerList;
} MODULE_INFO, *PMODULE_INFO;

int nvt_timer_drv_open(PMODULE_INFO pmodule_info, unsigned char if_id);
int nvt_timer_drv_release(PMODULE_INFO pmodule_info, unsigned char if_id);
int nvt_timer_drv_init(PMODULE_INFO pmodule_info);
int nvt_timer_drv_remove(PMODULE_INFO pmodule_info);
int nvt_timer_drv_suspend(MODULE_INFO *pmodule_info);
int nvt_timer_drv_resume(MODULE_INFO *pmodule_info);
int nvt_timer_drv_ioctl(unsigned char if_id, MODULE_INFO *pmodule_info, unsigned int cmd, unsigned long argc);
int nvt_timer_drv_ioctl_init(void);
int nvt_get_silence_timer(void);
void nvt_set_silence_timer(int timer_number);
void nvt_clear_silence_timer(void);
int nvt_get_losing_event_count(int timer_number);
void nvt_set_losing_event_count(int timer_number);

//int nvt_timer_drv_write_reg(PMODULE_INFO pmodule_info, unsigned long addr, unsigned long value);
//int nvt_timer_drv_read_reg(PMODULE_INFO pmodule_info, unsigned long addr);
#else
#if defined(_BSP_NA51000_)
#include "rtos_na51000/interrupt.h"
#else
#include "rtos_na51089/interrupt.h"
#endif
int nvt_timer_drv_init(void);
int nvt_timer_drv_remove(void);
#endif

#endif

