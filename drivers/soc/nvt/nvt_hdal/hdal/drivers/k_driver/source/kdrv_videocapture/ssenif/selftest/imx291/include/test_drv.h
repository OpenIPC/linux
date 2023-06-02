#ifndef __MODULE_DRV_H__
#define __MODULE_DRV_H__
#include <linux/io.h>
#include <linux/spinlock.h>
#include <linux/semaphore.h>
#include <linux/interrupt.h>
#include <linux/completion.h>
#include <linux/clk.h>


typedef struct ssenif_module_info {
	int useless;
} TEST_MODULE_INFO, *PTEST_MODULE_INFO;

int nvt_test_drv_init(PTEST_MODULE_INFO pmodule_info);
int nvt_test_drv_remove(PTEST_MODULE_INFO pmodule_info);

#endif

