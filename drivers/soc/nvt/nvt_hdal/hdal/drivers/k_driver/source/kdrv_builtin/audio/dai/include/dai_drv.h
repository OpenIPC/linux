#ifndef __DAI_DRV_H__
#define __DAI_DRV_H__
#include <linux/io.h>
#include <linux/spinlock.h>
#include <linux/semaphore.h>
#include <linux/interrupt.h>
#include <linux/completion.h>
#include <linux/clk.h>

#define MODULE_IRQ_NUM          1
#define MODULE_REG_NUM          1
#define MODULE_CLK_NUM          1


#define WRITE_REG(value, address)  iowrite32(value, address)
#define READ_REG(address)          ioread32(address)

typedef struct dai_info {
	struct completion dai_completion;
	struct semaphore dai_sem;
	struct clk *pclk[MODULE_CLK_NUM];
	struct tasklet_struct dai_tasklet[MODULE_IRQ_NUM];
	void __iomem *io_addr[MODULE_REG_NUM];
	int iinterrupt_id[MODULE_IRQ_NUM];
	wait_queue_head_t dai_wait_queue;
	spinlock_t dai_spinlock;
} DAI_INFO, *PDAI_INFO;

int nvt_dai_drv_open(PDAI_INFO pmodule_info, unsigned char uc_if);
int nvt_dai_drv_release(PDAI_INFO pmodule_info, unsigned char uc_if);
int nvt_dai_drv_init(PDAI_INFO pmodule_info);
int nvt_dai_drv_remove(PDAI_INFO pmodule_info);
int nvt_dai_drv_suspend(DAI_INFO *pmodule_info);
int nvt_dai_drv_resume(DAI_INFO *pmodule_info);
int nvt_dai_drv_ioctl(unsigned char uc_if, DAI_INFO *pmodule_info, unsigned int cmd, unsigned long arg);
int nvt_dai_drv_write_reg(PDAI_INFO pmodule_info, unsigned long addr, unsigned long value);
int nvt_dai_drv_read_reg(PDAI_INFO pmodule_info, unsigned long addr);
#endif

