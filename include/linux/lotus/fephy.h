/*
 * Copyright (c) LOTUS. All rights reserved.
 */

#ifndef __FEPHY_H__
#define __FEPHY_H__

int femac_phy_read(unsigned int regnum);

int femac_phy_write(unsigned int regnum, unsigned short val);

unsigned int fephy_get_fw_state(void);

void fephy_mask_fw_state(void);

void fephy_update(void);

unsigned int fephy_get_compat_state(void);

unsigned int fephy_init_state(void);

#endif /* End of #ifndef __FEPHY_H */
