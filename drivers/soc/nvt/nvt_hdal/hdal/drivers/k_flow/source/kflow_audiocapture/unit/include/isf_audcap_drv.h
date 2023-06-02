#ifndef __ISF_AUDCAP_DRV_H__
#define __ISF_AUDCAP_DRV_H__

#define MODULE_IRQ_NUM          0
#define MODULE_REG_NUM          1
#define MODULE_CLK_NUM          0

typedef struct _ISF_AUDCAP_INFO {
	int reversed;
} ISF_AUDCAP_INFO, *PISF_AUDCAP_INFO;

int isf_audcap_drv_open(PISF_AUDCAP_INFO pmodule_info, unsigned char if_id);
int isf_audcap_drv_release(PISF_AUDCAP_INFO pmodule_info, unsigned char if_id);
int isf_audcap_drv_init(PISF_AUDCAP_INFO pmodule_info);
int isf_audcap_drv_remove(PISF_AUDCAP_INFO pmodule_info);
int isf_audcap_drv_suspend(ISF_AUDCAP_INFO *pmodule_info);
int isf_audcap_drv_resume(ISF_AUDCAP_INFO *pmodule_info);
int isf_audcap_drv_ioctl(unsigned char if_id, ISF_AUDCAP_INFO *pmodule_info, unsigned int cmd, unsigned long argc);
#endif

