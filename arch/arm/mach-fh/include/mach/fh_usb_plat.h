#ifndef __FH_USB_PLAT_H__
#define __FH_USB_PLAT_H__

struct fh_usb_platform_data {
	char *dr_mode;
	unsigned int vbus_pwren;
};

#endif
