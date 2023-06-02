#ifndef __PWM_DRV_H__
#define __PWM_DRV_H__
#include <linux/io.h>
#include <linux/spinlock.h>
#include <linux/semaphore.h>
#include <linux/interrupt.h>
#include <linux/completion.h>
#include <linux/clk.h>

#define MODULE_IRQ_NUM          1
#define MODULE_REG_NUM          1
#define MODULE_CLK_NUM          15

#define WRITE_REG(VALUE, ADDR)  iowrite32(VALUE, ADDR)
#define READ_REG(ADDR)          ioread32(ADDR)

typedef struct module_info {
	struct completion pwm_completion;
	struct semaphore pwm_sem;
	struct clk *pclk[MODULE_CLK_NUM];
	struct tasklet_struct pwm_tasklet;
	void __iomem *io_addr[MODULE_REG_NUM];
	int iinterrupt_id[MODULE_IRQ_NUM];
	wait_queue_head_t pwm_wait_queue;
	spinlock_t pwm_spinlock;
} MODULE_INFO, *PMODULE_INFO;

int nvt_pwm_drv_open(PMODULE_INFO pmodule_info, unsigned char if_id);
int nvt_pwm_drv_release(PMODULE_INFO pmodule_info, unsigned char if_id);
int nvt_pwm_drv_init(PMODULE_INFO pmodule_info);
int nvt_pwm_drv_remove(PMODULE_INFO pmodule_info);
int nvt_pwm_drv_suspend(MODULE_INFO *pmodule_info);
int nvt_pwm_drv_resume(MODULE_INFO *pmodule_info);
int nvt_pwm_drv_ioctl(unsigned char if_id, MODULE_INFO *pmodule_info, unsigned int cmd, unsigned long argc);
int nvt_pwm_drv_write_reg(PMODULE_INFO pmodule_info, unsigned long addr, unsigned long value);
int nvt_pwm_drv_read_reg(PMODULE_INFO pmodule_info, unsigned long addr);
void nvt_pwm_drv_open_pwm(unsigned long channel, unsigned long base_period, unsigned long rising, unsigned long falling, unsigned long clk_div, unsigned long inv, unsigned long cycle, unsigned long reload);
void nvt_pwm_drv_close_pwm(unsigned long channel);
void nvt_pwm_drv_open_mstep(unsigned long channel, unsigned long dir, unsigned long phase_type, unsigned long step_phase, unsigned long threshold_en, unsigned long threshold, unsigned long clk_div, unsigned long cycle);
void nvt_pwm_drv_close_mstep(unsigned long channel);
#endif

