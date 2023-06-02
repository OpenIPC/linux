#ifndef __ISF_AUDENC_DRV_H
#define __ISF_AUDENC_DRV_H

#define MODULE_IRQ_NUM          0
#define MODULE_REG_NUM          1
#define MODULE_CLK_NUM          0

typedef struct _ISF_AUDENC_INFO {
	int reversed;
} ISF_AUDENC_INFO, *PISF_AUDENC_INFO;

int isf_audenc_drv_open(ISF_AUDENC_INFO *pmodule_info, unsigned char minor);
int isf_audenc_drv_release(ISF_AUDENC_INFO *pmodule_info, unsigned char minor);
int isf_audenc_drv_init(ISF_AUDENC_INFO *pmodule_info);
int isf_audenc_drv_remove(ISF_AUDENC_INFO *pmodule_info);
int isf_audenc_drv_suspend(ISF_AUDENC_INFO *pmodule_info);
int isf_audenc_drv_resume(ISF_AUDENC_INFO *pmodule_info);
int isf_audenc_drv_ioctl(unsigned char minor, ISF_AUDENC_INFO *pmodule_info, unsigned int cmd_id, unsigned long arg);
#endif

