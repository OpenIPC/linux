#ifndef _UVC_FORMAT_H_
#define _UVC_FORMAT_H_

#ifdef __KERNEL__

#include "uvc.h"

extern void change_usb_support_fmt(struct uvc_device *uvc, void *arg);
#ifdef UVC_FASTBOOT
extern int usb_fmt_to_usr(struct uvc_device *uvc, void *arg);
extern void change_usb_support_fmt_fast(struct uvc_device *uvc);
#endif

#endif /* __KERNEL__ */

#endif /* _UVC_FORMAT_H_ */
