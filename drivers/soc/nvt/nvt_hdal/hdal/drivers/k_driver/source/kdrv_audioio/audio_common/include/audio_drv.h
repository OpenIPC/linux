#ifndef __AUDIO_DRV_H__
#define __AUDIO_DRV_H__
#include <linux/io.h>
#include <linux/spinlock.h>
#include <linux/semaphore.h>
#include <linux/interrupt.h>
#include <linux/completion.h>
#include <linux/clk.h>

typedef struct module_info {

} MODULE_INFO, *PMODULE_INFO;

int nvt_audio_drv_open(PMODULE_INFO pmodule_info, unsigned char ucIF);
int nvt_audio_drv_release(PMODULE_INFO pmodule_info, unsigned char ucIF);
int nvt_audio_drv_init(PMODULE_INFO pmodule_info);
int nvt_audio_drv_remove(PMODULE_INFO pmodule_info);
int nvt_audio_drv_suspend(MODULE_INFO* pmodule_info);
int nvt_audio_drv_resume(MODULE_INFO* pmodule_info);
int nvt_audio_drv_ioctl(unsigned char ucIF, MODULE_INFO* pmodule_info, unsigned int uiCmd, unsigned long ulArg);
int nvt_audio_drv_write_reg(PMODULE_INFO pmodule_info, unsigned long addr, unsigned long value);
int nvt_audio_drv_read_reg(PMODULE_INFO pmodule_info, unsigned long addr);
#endif

