#ifndef __FH_GMAC_PLAT_H__
#define __FH_GMAC_PLAT_H__

struct fh_gmac_platform_data {
	u32 phy_reset_pin;
	u32 speed_switch_pmu_reg;
	u32 speed_switch_bit_pos;
	u32 speed_switch_100M_val;
};
#endif