#ifndef __MODULE_DRV_H__
#define __MODULE_DRV_H__
#include <linux/io.h>
#include <linux/spinlock.h>
#include <linux/semaphore.h>
#include <linux/interrupt.h>
#include <linux/completion.h>
#include <linux/clk.h>


typedef struct audlib_module_info {
	int useless;
} AUDLIB_MODULE_INFO, *PAUDLIB_MODULE_INFO;

int nvt_audlib_agc_drv_init(PAUDLIB_MODULE_INFO pmodule_info);
int nvt_audlib_agc_drv_remove(PAUDLIB_MODULE_INFO pmodule_info);

#endif

