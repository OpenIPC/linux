#ifndef AK_ISP_LIFECYCLE_H
#define AK_ISP_LIFECYCLE_H

struct device;

int ak_isp_acquire(int (*set_style)(void *context, unsigned int style),
		   void *context);
int ak_isp_prepare(struct device *dev);
void ak_isp_release(void);
/* Caller holds capture mutex and quiesces IRQ/3A; failure requires reopen. */
int ak_isp_apply_tuning_style(unsigned int style);
int ak_isp_get_awb_auto(int *enabled);
int ak_isp_set_awb_auto(int enabled);

#endif
