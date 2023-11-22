/*
 * sstar-lib.h- Sigmastar
 *
 * Copyright (c) [2019~2020] SigmaStar Technology.
 *
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License version 2 for more details.
 *
 */

#ifndef _SSTAR_USB_LIB_H_
#define _SSTAR_USB_LIB_H_

struct sstar_efuse
{
    uintptr_t efuse_base_addr;
    u32       reg_set_addr; // register offset for set_addr
    u32       reg_read;     // register offset for issue_read
    u32       reg_data;     // register offset for data
    u16       bank_addr;
    u16       issue_read;
};

extern u32 sstar_efuse_read(struct sstar_efuse *efuse);
extern u16 sstar_efuse_rterm(void);

#endif /* _SSTAR_USB_LIB_H_ */
