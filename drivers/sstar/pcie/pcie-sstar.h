/*
 * pcie-sstar.h - Sigmastar
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

#ifndef _PCIE_SSTAR_H_
#define _PCIE_SSTAR_H_

struct dw_plat_pcie
{
    struct dw_pcie*          pci;
    enum dw_pcie_device_mode mode;
    u8                       id; /* controller id */
};

#define PCIE_GET_DBI2_REG_OFFSET (0x1 << 20)

#endif /* _PCIE_SSTAR_H_ */