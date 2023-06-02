#ifndef __FH_USB
#define __FH_USB

#include <linux/platform_device.h>


struct fh_usb_platform_data {

	void (*utmi_rst)(void);
	void (*phy_rst)(void);
	void (*hcd_resume)(void);
	void (*power_on)(void);
	unsigned int grxfsiz_pwron_val;
	unsigned int gnptxfsiz_pwron_val;
};

#endif

