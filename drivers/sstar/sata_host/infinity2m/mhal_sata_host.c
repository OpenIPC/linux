/*
* mhal_sata_host.c - Sigmastar
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
#include <mhal_sata_host.h>
#include <linux/delay.h>
#include "ms_platform.h"

#define sata_reg_write16(val, addr) { (*((volatile unsigned short*)(addr))) = (unsigned short)(val); }
#define sata_reg_write8(val, addr) { (*((volatile unsigned char*)(addr))) = (unsigned char)(val);}
#define GET_REG8_ADDR(x, y)     (x+(y)*2)
#define GET_REG16_ADDR(x, y)    (x+(y)*4)
#define RIU_BASE_ADDR 0xFD000000

void MHal_SATA_Clock_Config(phys_addr_t misc_base, phys_addr_t port_base, bool enable)
{
    u16 u16Temp;

    // Enable MAC Clock, bank is chip dependent
    // [0] phy clk gated
    // [1] phy clk invert
    // [3:2] 0: 108M
    // [8] clk gated
    // [9] clk invert
    // [11:10] 0: 432M

    if(enable)
    {
        if(port_base == SATA_GHC_0_P0_ADDRESS_START)
        {
            //*** Bank 0x000E h003D => clear bit 0~5
            u16Temp = readw((volatile void *)SSTAR_RIU_BASE + (REG_PD_XTAL_HV << 1));
            u16Temp &= ~(SATA_HV_CRYSTAL_CLK_MASK);
            writew(u16Temp, (volatile void *)SSTAR_RIU_BASE + (REG_PD_XTAL_HV << 1));

            //*** Bank 0x1038 h0046 => 0x0000
            u16Temp = readw((volatile void *)SSTAR_RIU_BASE + (REG_CKG_SATA_FCLK << 1));
            u16Temp &= ~(CKG_SATA_FCLK_MASK | CKG_SATA_FCLK_GATED | CKG_SATA_FCLK_PHY_MASK | CKG_SATA_FCLK_PHY_GATED);
            writew(u16Temp, (volatile void *)SSTAR_RIU_BASE + (REG_CKG_SATA_FCLK << 1));

            //*** Bank 0x1527 h0044 => set bit 0
            u16Temp = readw((volatile void *)SSTAR_RIU_BASE + (REG_SATA_PHY_SYNTH_SLD << 1));
            u16Temp &= ~0x1;
            writew(u16Temp, (volatile void *)SSTAR_RIU_BASE + (REG_SATA_PHY_SYNTH_SLD << 1));
            u16Temp |= 0x1;
            writew(u16Temp, (volatile void *)SSTAR_RIU_BASE + (REG_SATA_PHY_SYNTH_SLD << 1));

            //*** Bank 0x1525 h0000 => set bit 12
            u16Temp = readw((volatile void *)misc_base + (0x00 < 1));
            u16Temp |= 0x1000; // reg_sata_swrst_all
            writew(u16Temp, (volatile void *)misc_base + (0x00 < 1));
        }
    }
    else
    {
        if(port_base == SATA_GHC_0_P0_ADDRESS_START)
        {
            //*** Bank 0x000E h003D => set bit 0~5
            u16Temp = readw((volatile void *)SSTAR_RIU_BASE + (REG_PD_XTAL_HV << 1));
            u16Temp |= SATA_HV_CRYSTAL_CLK_MASK;
            writew(u16Temp, (volatile void *)SSTAR_RIU_BASE + (REG_PD_XTAL_HV << 1));

            //*** Bank 0x1038 h0046 => 0x0101
            u16Temp = readw((volatile void *)SSTAR_RIU_BASE + (REG_CKG_SATA_FCLK << 1));
            u16Temp &= ~(CKG_SATA_FCLK_MASK | CKG_SATA_FCLK_GATED | CKG_SATA_FCLK_PHY_MASK | CKG_SATA_FCLK_PHY_GATED);
            u16Temp |= (CKG_SATA_FCLK_GATED | CKG_SATA_FCLK_PHY_GATED);
            writew(u16Temp, (volatile void *)SSTAR_RIU_BASE + (REG_CKG_SATA_FCLK << 1));

            //*** Bank 0x1527 h0044 => clear bit 0
            u16Temp = readw((volatile void *)SSTAR_RIU_BASE + (REG_SATA_PHY_SYNTH_SLD << 1));
            u16Temp &= ~0x1;
            writew(u16Temp, (volatile void *)SSTAR_RIU_BASE + (REG_SATA_PHY_SYNTH_SLD << 1));

            //*** Bank 0x1525 h0000 => clear bit 12
            u16Temp = readw((volatile void *)misc_base + (0x00 < 1));
            u16Temp &= ~(0x1000); // reg_sata_swrst_all
            writew(u16Temp, (volatile void *)misc_base + (0x00 < 1));
        }
    }

    if(port_base == SATA_GHC_0_P0_ADDRESS_START)
        printk("[SATA0] Clock : %s\n", (enable ? "ON" : "OFF"));
}
//EXPORT_SYMBOL_GPL(MHal_SATA_Clock_Config);

u32 MHal_SATA_get_max_speed(void)
{
    return E_PORT_SPEED_GEN3;
}
//EXPORT_SYMBOL_GPL(MHal_SATA_get_max_speed);

void MHal_SATA_Setup_Port_Implement(phys_addr_t misc_base, phys_addr_t port_base, phys_addr_t hba_base)
{
    //  Init FW to trigger controller
    writel(0x00000000, (volatile void *)hba_base + (SS_HOST_CAP));

    // Port Implement
    writel(0x00000001, (volatile void *)hba_base + (SS_HOST_PORTS_IMPL));
    writel(0x00000000, (volatile void *)port_base + (SS_PORT_CMD));
}
//EXPORT_SYMBOL_GPL(MHal_SATA_Setup_Port_Implement);

#ifndef CONFIG_SSTAR_SATA_AHCI_PLATFORM_HOST
void MHal_SATA_HW_Inital(phys_addr_t misc_base, phys_addr_t port_base, phys_addr_t hba_base)
{
    //u16 u16Temp;
    u32 GHC_PHY_ANA = 0x0, u32Temp;
    int phy_mode;

    if(port_base == SATA_GHC_0_P0_ADDRESS_START)
    {
        GHC_PHY_ANA = SATA_GHC_0_PHY_ANA;//0x152700
    }

    MHal_SATA_Clock_Config(misc_base, port_base, 1);
    //u16Temp = readw((volatile void *)SSTAR_RIU_BASE + (REG_CKG_SATA_FCLK << 1));
    //printk("SATA_CLK_REG = 0x%x\n", u16Temp);

    printk("RIU_BASE = 0x%x, GHC_PHY_ANA = 0x%x\n", SSTAR_RIU_BASE, GHC_PHY_ANA);
    //RIU_BASE = xfd000000, misc_base : xfd205a00 , hba_base : xfd345000 , port_base : xfd345080 , GHC_PHY : 152600

    //printk("check sata speed !!!!!!\n");
    u32Temp = readl((volatile void *)port_base + 0x2c);
    u32Temp = u32Temp & (~E_PORT_SPEED_MASK);
    u32Temp = u32Temp | MHal_SATA_get_max_speed();
    writel(u32Temp, (volatile void *)port_base + 0x2c);
    u32Temp = readl((volatile void *)port_base + 0x2c);
    printk("MAC SATA SPEED= 0x%x\n", u32Temp);

    if((u32Temp & E_PORT_SPEED_MASK) == E_PORT_SPEED_GEN1)
        phy_mode = 0;
    else if((u32Temp & E_PORT_SPEED_MASK) == E_PORT_SPEED_GEN2)
        phy_mode = 1;
    else
        phy_mode = 2;

    //printk("sata hal PHY init !!!!!!\n");
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

    writel(0x00000001, (volatile void *) hba_base + SS_HOST_CTL); // reset HBA

}
//EXPORT_SYMBOL_GPL(MHal_SATA_HW_Inital);

phys_addr_t MHal_SATA_bus_address(phys_addr_t phy_address)
{
    //phys_addr_t bus_address;

    //printk("phy addr = 0x%llx\n", phy_address);
    if (phy_address >= MIU_INTERVAL_SATA)
    {
        //printk("select MIU1, bus addr = 0x%8.8x\n", phy_address + 0x20000000);
        //return phy_address + MIU_INTERVAL_SATA;
        return phy_address + 0x20000000;
    }
    else
    {
        //printk("select MIU0, bus addr = 0x%8.8x\n", phy_address - 0x20000000);
        //return phy_address - MIU_INTERVAL_SATA;
        return phy_address - 0x20000000;
    }
}
//EXPORT_SYMBOL_GPL(MHal_SATA_bus_address);
#endif
