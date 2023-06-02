#ifndef __FH_WDT_PLATFORM_DATA
#define __FH_WDT_PLATFORM_DATA

#include <linux/platform_device.h>

struct fh_wdt_platform_data {
	void (*resume)(void);
	void (*pause)(void);
};

#endif

