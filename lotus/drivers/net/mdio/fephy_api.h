/*
 * Copyright (c) LOTUS. All rights reserved.
 */

#ifndef __LOTUS_FEPHY_API_H__
#define __LOTUS_FEPHY_API_H__
struct lotus_fephy_private {
	unsigned int fephy_init_state;
	unsigned int compat_state;
	unsigned int (*get_fw_state)(void);
	void (*mask_fw_state)(void);
	void (*fephy_update)(void);
};

void __fephy_set_private_date(struct lotus_fephy_private *xm_data);
#endif
