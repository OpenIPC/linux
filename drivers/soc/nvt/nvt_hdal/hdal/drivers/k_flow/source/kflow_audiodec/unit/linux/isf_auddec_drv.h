#ifndef __ISF_AUDDEC_DRV_H
#define __ISF_AUDDEC_DRV_H

#define MODULE_IRQ_NUM          0
#define MODULE_REG_NUM          1
#define MODULE_CLK_NUM          0

typedef struct _ISF_AUDDEC_INFO {
	int reversed;
} ISF_AUDDEC_INFO, *PISF_AUDDEC_INFO;

int isf_auddec_drv_open(ISF_AUDDEC_INFO *pmodule_info, unsigned char minor);
int isf_auddec_drv_release(ISF_AUDDEC_INFO *pmodule_info, unsigned char minor);
int isf_auddec_drv_init(ISF_AUDDEC_INFO *pmodule_info);
int isf_auddec_drv_remove(ISF_AUDDEC_INFO *pmodule_info);
int isf_auddec_drv_suspend(ISF_AUDDEC_INFO *pmodule_info);
int isf_auddec_drv_resume(ISF_AUDDEC_INFO *pmodule_info);
int isf_auddec_drv_ioctl(unsigned char minor, ISF_AUDDEC_INFO *pmodule_info, unsigned int cmd_id, unsigned long arg);
#endif

