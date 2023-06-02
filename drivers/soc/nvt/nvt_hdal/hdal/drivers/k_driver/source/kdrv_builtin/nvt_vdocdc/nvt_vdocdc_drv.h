#ifndef __NVT_VDOCDC_DRV_H_
#define __NVT_VDOCDC_DRV_H_

#if defined(__LINUX)
#include <linux/io.h>
#include <linux/spinlock.h>
#include <linux/semaphore.h>
#include <linux/interrupt.h>
#include <linux/completion.h>
#include <linux/clk.h>

#define MODULE_REG_NUM		1	
#define MODULE_IRQ_NUM		1
#define MODULE_CLK_NUM		1

typedef struct module_info {
	struct completion comp;
	struct clk* pclk[MODULE_CLK_NUM];
	struct semaphore sem;
	struct tasklet_struct tasklet;
	
	void __iomem* io_addr[MODULE_REG_NUM];
	int iinterrupt_id[MODULE_IRQ_NUM];
	
	wait_queue_head_t wait_queue;
	spinlock_t        spinlock;
}MODULE_INFO;

int nvt_vdocdc_drv_init(MODULE_INFO *p_module_info);
int nvt_vdocdc_drv_remove(MODULE_INFO *p_module_info);

#elif defined(__FREERTOS)
int nvt_vdocdc_drv_init(void);
int nvt_vdocdc_drv_remove(void);
#endif

#endif // __KDRV_VDOCDC_DRV_H_