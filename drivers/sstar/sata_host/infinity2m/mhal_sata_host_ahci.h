/*
* mhal_sata_host_ahci.h - Sigmastar
*
* Copyright (C) 2018 Sigmastar Technology Corp.
*
* Author: edie.chen <edie.chen@sigmastar.com.tw>
*
* This software is licensed under the terms of the GNU General Public
* License version 2, as published by the Free Software Foundation, and
* may be copied, distributed, and modified under those terms.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
*/

#ifndef _MHAL_SATA_HOST_AHCI_H
#define _MHAL_SATA_HOST_AHCI_H

void ss_sata_misc_init(void *mmio, int phy_mode, int n_ports);
void ss_sata_phy_init(void __iomem *mmio, int phy_mode, int n_port);
#endif
