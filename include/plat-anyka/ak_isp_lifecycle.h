#ifndef AK_ISP_LIFECYCLE_H
#define AK_ISP_LIFECYCLE_H

struct device;

int ak_isp_acquire(int (*set_style)(void *context, unsigned int style),
		   void *context);
int ak_isp_prepare(struct device *dev);
void ak_isp_release(void);
/* Wait out style switches still running a callback; see ak_isp_char.c. */
void ak_isp_sync_style(void);
/* Caller holds capture mutex and quiesces IRQ/3A; failure requires reopen. */
int ak_isp_apply_tuning_style(unsigned int style);
int ak_isp_get_awb_auto(int *enabled);
int ak_isp_set_awb_auto(int enabled);
/*
 * The ISP's output orientation, mirror (@hflip) and flip (@vflip). Each is 0
 * or 1 to set that axis, or negative to leave it; on success both hold what
 * the ISP now has. Takes the lock the control ioctls take, so a concurrent
 * AK_ISP_SET_MISC_ATTR cannot interleave with its read-modify-write.
 */
int ak_isp_orientation(int *hflip, int *vflip);

#endif
