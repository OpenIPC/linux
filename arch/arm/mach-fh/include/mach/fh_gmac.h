#ifndef __FH_GMAC_PLATFORM_DATA
#define __FH_GMAC_PLATFORM_DATA

#include <linux/platform_device.h>

enum {
	gmac_phyt_reg_basic_ctrl = 0,
	gmac_phyt_reg_basic_status = 1,
	gmac_phyt_reg_phy_id1 = 2,
	gmac_phyt_reg_phy_id2 = 3,
	gmac_phyt_rtl8201_rmii_mode = 16,
	gmac_phyt_ti83848_rmii_mode = 17,
	gmac_phyt_rtl8201_power_saving = 24,
	gmac_phyt_rtl8201_page_select = 31,
	gmac_phyt_ip101g_page_select = 20
};

enum {
	gmac_speed_10m,
	gmac_speed_100m
};

struct fh_gmac_platform_data {
	int interface;
	int phyid;
	void (*early_init)(struct fh_gmac_platform_data *plat_data);
	void (*plat_init)(struct fh_gmac_platform_data *plat_data);
	void (*set_rmii_speed)(int speed);
	void (*phy_reset)(void);
};

#endif

