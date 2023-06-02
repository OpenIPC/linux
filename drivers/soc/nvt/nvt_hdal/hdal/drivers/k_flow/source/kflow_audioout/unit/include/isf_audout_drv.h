#ifndef __ISF_AUDOUT_DRV_H__
#define __ISF_AUDOUT_DRV_H__

#define MODULE_IRQ_NUM          0
#define MODULE_REG_NUM          1
#define MODULE_CLK_NUM          0

typedef struct _ISF_AUDOUT_INFO {
	int reversed;
} ISF_AUDOUT_INFO, *PISF_AUDOUT_INFO;

int isf_audout_drv_open(PISF_AUDOUT_INFO pmodule_info, unsigned char if_id);
int isf_audout_drv_release(PISF_AUDOUT_INFO pmodule_info, unsigned char if_id);
int isf_audout_drv_init(PISF_AUDOUT_INFO pmodule_info);
int isf_audout_drv_remove(PISF_AUDOUT_INFO pmodule_info);
int isf_audout_drv_suspend(ISF_AUDOUT_INFO *pmodule_info);
int isf_audout_drv_resume(ISF_AUDOUT_INFO *pmodule_info);
int isf_audout_drv_ioctl(unsigned char if_id, ISF_AUDOUT_INFO *pmodule_info, unsigned int cmd, unsigned long argc);
#endif

