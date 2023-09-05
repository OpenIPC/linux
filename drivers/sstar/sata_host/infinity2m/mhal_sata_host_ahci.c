/*
* mhal_sata_host_ahci.c - Sigmastar
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

#include <linux/kernel.h>
#include <asm/io.h>
#include <linux/delay.h>
#include "ms_platform.h"
#include "mdrv_types.h"
#include "mhal_sata_host.h"
#include "mhal_sata_host_ahci.h"

#if 0
static void ss_sata_clk_enable(void)
{
    // Enable Clock, bank is chip dependent
    writew(0x000c, (volatile void *)SSTAR_RIU_BASE + (0x100b64 << 1)); // [0] gate
    // [1] clk invert
    // [4:2] 0:240M, 1:216M, [3:clk_miu]
}
#endif
#if 0
static void ss_sata_clk_disable(void)
{
    MHal_SATA_Clock_Config(SATA_MISC_0_ADDRESS_START, SATA_GHC_0_P0_ADDRESS_START, FALSE);
}
#endif

void ss_sata_misc_init(void *mmio, int phy_mode, int n_ports)
{
    void __iomem *port_base = mmio + 0x80; //1A2840<<2
    void __iomem *misc_base = mmio - 0xA0600; //152500<<2
    u32 u32Temp;

    MHal_SATA_Clock_Config((phys_addr_t)misc_base, (phys_addr_t)port_base, TRUE);

#if 1
    u32Temp = readl((volatile void *)port_base + 0x2c);
    u32Temp = u32Temp & (~E_PORT_SPEED_MASK);
    if(phy_mode == 0)
    {
        u32Temp = u32Temp | E_PORT_SPEED_GEN1;
    }
    else if(phy_mode == 1)
    {
        u32Temp = u32Temp | E_PORT_SPEED_GEN2;
    }
    else if(phy_mode == 2)
    {
        u32Temp = u32Temp | E_PORT_SPEED_GEN3;
    }
    else
    {
        u32Temp = u32Temp | MHal_SATA_get_max_speed();
    }
    writel(u32Temp, (volatile void *)port_base + 0x2c);
    u32Temp = readl((volatile void *)port_base + 0x2c);

    printk("MAC SATA SPEED= 0x%x\n", u32Temp);
#endif
}

void ss_sata_phy_init(void *mmio, int phy_mode, int n_ports)
{
    phys_addr_t hba_base = (phys_addr_t)mmio; //1A2800<<1
    phys_addr_t port_base = (phys_addr_t)(mmio + 0x80); //1A2840<<1
    u32 GHC_PHY_ANA = 0x0;

    if ((n_ports < 1) || (n_ports > 4))
        pr_err("ERROR: PORT num you set is WRONG!!!\n");

    if(port_base == SATA_GHC_0_P0_ADDRESS_START)
    {
        GHC_PHY_ANA = SATA_GHC_0_PHY_ANA;//0x152700
    }

    //printk("sata phy init  A\n");

    //*** Bank 0x1527 h0005 => 0x00:Gen1, 0x01:Gen2, 0x10:Gen3
    switch (phy_mode)
    {
        case 0:
            writew(0x00, (volatile void *) GHC_PHY_ANA + (0x05 << 1));
            break;
        case 1:
            writew(0x01, (volatile void *) GHC_PHY_ANA + (0x05 << 1));
            break;
        case 2:
        default:
            writew(0x10, (volatile void *) GHC_PHY_ANA + (0x05 << 1));
            break;
    }

    writew(0x0001, (volatile void *) hba_base + SS_HOST_CTL); // reset HBA

}
