#ifndef __FH_WDT_PLAT_H__
#define __FH_WDT_PLAT_H__

struct fh_wdt_platform_data {
	int mode;
#define MODE_CONTINUOUS		0
#define MODE_DISCRETE		1
};

#endif

