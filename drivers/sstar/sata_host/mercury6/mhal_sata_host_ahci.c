/*
 * mhal_sata_host_ahci.c- Sigmastar
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

#include <linux/kernel.h>
#include <asm/io.h>
#include <linux/delay.h>
#include "ms_platform.h"
#include "ahci.h"
#include "mhal_sata_host.h"
#include "mhal_sata_host_ahci.h"
#include "mdrv_sata_host_ahci.h"

//#define USE_MIPSPLL

int                 g_hd_link0[4] = {0};
extern unsigned int phy_setting;

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

static u32 sstar_sata_wait_reg(u32 reg_addr, u32 mask, u32 val, unsigned long interval, unsigned long timeout)
{
    u32           temp;
    unsigned long timeout_vale = 0;

    temp = readl((void *)reg_addr);

    while ((temp & mask) == val)
    {
        msleep(interval);
        timeout_vale += interval;
        if (timeout_vale > timeout)
            break;
        temp = readl((void *)reg_addr);
    }
    return temp;
}

int __ss_ahci_store_hd_link(void __iomem *mmio)
{
    const int counter = 5;
    int       i       = 0;
    int       status  = 0;
    int       link    = 0;
    int *     hd_link;
    int       max = -1, ret = 0;

    // printk("%s , mmio = 0x%p \n", __func__ , mmio  );
    hd_link = g_hd_link0;

    for (i = 0; i < counter; ++i)
    {
        // address = SATA_MAC + PORT0_CMD , ex: devmem 0x1F345118 32 0x00000000
        writel(0x00000000, (volatile void *)(mmio + PORT0_CMD));
        // address = SATA_MAC + PORT0_SCTL , ex: devmem 0x1F34512C 32 0x00000031
        writel(0x00000031, (volatile void *)(mmio + PORT0_SCTL));
        ndelay(1000);

        // devmem ( SATA_AHCI + offset PORT0_SCTL ) 32 0x00000030
        writel(0x00000030, (volatile void *)(mmio + PORT0_SCTL));
        msleep(500);
        // check hd link status
        status = readl((volatile void *)(mmio + PORT0_SSTS));

        if (((status & 0x00000100) == 0) || ((status & 0x0F) != 0x03))
        {
            hd_link[0]++;
            continue;
        }

        link = (status & 0x0F0) >> 4;
        if (link > 3)
        {
            hd_link[0]++;
            printk("%s , strange status\n", __func__);
        }
        else
        {
            hd_link[link]++;
        }
    }

    // check max link
    for (i = 0; i < 4; i++)
    {
        if (hd_link[i] > max)
            max = hd_link[i];
    }

    for (i = 0; i < 4; i++)
    {
        if (hd_link[i] == max)
            ret = i;
    }

    // printk("%s ,hd_link = %d\n", __func__ , ret);

    return ret;
}

void ss_sata_misc_init(void *mmio, int n_ports)
{
    void __iomem *port_base = mmio + 0x100;          //   1A3480<<1  = 346900
    void __iomem *misc_base = mmio - (0x5FA00 << 1); //  143A00<<1  = 287400  , 1A3400-143A00 = 5FA00

    MHal_SATA_Clock_Config((u32)misc_base, (u32)port_base, TRUE);
}

void ss_sata_phy_init(void *mmio, int phy_mode, int n_ports)
{
    // u32 hba_base = (u32)mmio; //  1A3400<<1
    u32 port_base   = (u32)(mmio + 0x100); //   1A3480<<1
    u32 GHC_PHY_ANA = 0x0;
    u16 u16Temp;

    if ((n_ports < 1) || (n_ports > 4))
        pr_err("ERROR: PORT num you set is WRONG!!!\n");

    if ((phy_mode < 0) || (phy_mode > 2))
    {
        pr_err("%s ERROR: phy_mode set to default!\n", __func__);
        phy_mode = 2;
    }

    if (port_base == SATA_GHC_0_P0_ADDRESS_START)
    {
        GHC_PHY_ANA = SATA_GHC_0_PHY_ANA; //  0x143C00
    }

    // printk("sata phy init  A\n");
    //*** Bank 0x1524 h0003 => 0x0
    writew(0x00000, (volatile void *)SSTAR_RIU_BASE + (REG_USB3_SATA_PHYA_REG_03 << 1)); // M6 new register

    //*** Bank 0x143C h0020 => 0x0100
    writew(0x0100, (volatile void *)SSTAR_RIU_BASE + (REG_SATA_PHY_REG_20 << 1));

    //*** bringup script setting , Bank 0x143C h0030 => 0x1008 , and need to apply efuse  SATA_RXPLL_ICTRL_CDR trim
    // setting
    u16Temp = readw((volatile void *)SSTAR_RIU_BASE + (REG_SATA_PHY_REG_30 << 1));
    u16Temp = (u16Temp & ~0x0F) | 0x0008;
    u16Temp = u16Temp | 0x1000;
    writew(u16Temp, (volatile void *)SSTAR_RIU_BASE + (REG_SATA_PHY_REG_30 << 1));

    //*** Bank 0x143C h0033 => 0x0500
    writew(0x0500, (volatile void *)SSTAR_RIU_BASE + (REG_SATA_PHY_REG_33 << 1));

    //*** Bank 0x143C h0060 => 0x0002
    writew(0x0002, (volatile void *)SSTAR_RIU_BASE + (REG_SATA_PHY_TXPLL_DET_SW << 1));

    //*** Bank 0x143C h0070 => 0x0062
    writew(0x0062, (volatile void *)SSTAR_RIU_BASE + (REG_SATA_PHY_REG_70 << 1));

    //*** Bank 0x143C h003E => 0x8000
    writew(0x8000, (volatile void *)SSTAR_RIU_BASE + (REG_SATA_PHY_REG_3E << 1));

#ifdef USE_MIPSPLL
    //*** Bank 0x1032 h0011 => enable bit11
    u16Temp = readw((volatile void *)SSTAR_RIU_BASE + ((0x103200 + (0x11 << 1)) << 1));
    u16Temp |= 0x0800;
    printk("Bank 0x1032 0x11 = 0x%x\n", u16Temp);
    writew(u16Temp, (volatile void *)SSTAR_RIU_BASE + ((0x103200 + (0x11 << 1)) << 1));

    //*** Bank 0x143C h0004 => 0x0000
    writew(0x0000, (volatile void *)SSTAR_RIU_BASE + (REG_SATA_PHY_CLK_PMALIVE_SEL << 1));
#else
    //*** Bank 0x143C h0004 => 0x0002
    writew(0x0002, (volatile void *)SSTAR_RIU_BASE + (REG_SATA_PHY_CLK_PMALIVE_SEL << 1));
#endif

    //*** Bank 0x143C h0044 => set bit 0
    u16Temp = readw((volatile void *)SSTAR_RIU_BASE + (REG_SATA_PHY_SYNTH_SLD << 1));
    u16Temp |= 0x1;
    writew(u16Temp, (volatile void *)SSTAR_RIU_BASE + (REG_SATA_PHY_SYNTH_SLD << 1));
    u16Temp &= ~0x1;
    writew(u16Temp, (volatile void *)SSTAR_RIU_BASE + (REG_SATA_PHY_SYNTH_SLD << 1));

    //*** Bank 0x143B h000A => 0x440A
    writew(0x440A, (volatile void *)SSTAR_RIU_BASE + (REG_SATA_PHYD_REG_0A << 1));

    //*** Bank 0x143B h0026 => 0x1905
    writew(0x1905, (volatile void *)SSTAR_RIU_BASE + (REG_SATA_PHYD_REG_26 << 1));

    //*** Bank 0x143B h003E => 0xB659
    writew(0xB659, (volatile void *)SSTAR_RIU_BASE + (REG_SATA_PHYD_REG_3E << 1));

    //*** Bank 0x143B h0040 => 0xD819
    writew(0xD819, (volatile void *)SSTAR_RIU_BASE + (REG_SATA_PHYD_REG_40 << 1));

    //*** Bank 0x143B h0061 => 0x4000
    writew(0x4000, (volatile void *)SSTAR_RIU_BASE + (REG_SATA_PHYD_REG_61 << 1));

    //*** Bank 0x143B h0064 => 0x0044
    writew(0x0044, (volatile void *)SSTAR_RIU_BASE + (REG_SATA_PHYD_REG_64 << 1));

    //  riu_w 0x143B 0x12 0x4200
    if (phy_setting == 0)
        writew(0x4200, (volatile void *)SSTAR_RIU_BASE + (REG_SATA_PHYD_REG_12 << 1));
    else
        writew(phy_setting, (volatile void *)SSTAR_RIU_BASE + (REG_SATA_PHYD_REG_12 << 1));

    //*** Bank 0x143C h0054 => bit 15 = 1
    u16Temp = readw((volatile void *)SSTAR_RIU_BASE + (REG_SATA_PHYA_REG_54 << 1));
    //  printk("%s , pre Bank 0x143C 0x54 = 0x%X\n", __func__ , u16Temp);
    u16Temp |= (0x1 << 15);
    writew(u16Temp, (volatile void *)SSTAR_RIU_BASE + (REG_SATA_PHYA_REG_54 << 1));
    u16Temp = readw((volatile void *)SSTAR_RIU_BASE + (REG_SATA_PHYA_REG_54 << 1));
    //  printk("%s , after Bank 0x143C 0x54 = 0x%X\n", __func__ , u16Temp);

    //*** Bank 0x143C h0058 => bit 8 = 1
    u16Temp = readw((volatile void *)SSTAR_RIU_BASE + (REG_SATA_PHYA_REG_58 << 1));
    // printk("%s , pre Bank 0x143C 0x58 = 0x%X\n", __func__ , u16Temp);
    u16Temp |= (0x1 << 8);
    writew(u16Temp, (volatile void *)SSTAR_RIU_BASE + (REG_SATA_PHYA_REG_58 << 1));
    u16Temp = readw((volatile void *)SSTAR_RIU_BASE + (REG_SATA_PHYA_REG_58 << 1));
    // printk("%s , after Bank 0x143C 0x58 = 0x%X\n", __func__ , u16Temp);

    //*** Bank 0x143C h0005 => 0x0001
    writew(phy_mode, (volatile void *)SSTAR_RIU_BASE + ((GHC_PHY_ANA + (0x5 << 1)) << 1));
}

// int ss_sata_init(void __iomem *mmio , int phy_mode, int port_num)
int ss_sata_init(struct sstar_ahci_priv *priv, int phy_mode, int port_num)
{
    u32   i;
    u32   u32Temp = 0, hba_base = 0, port_base = 0, misc_base = 0;
    void *mmio = priv->res_ahci_port0 - 0x100;

    priv->mmio = mmio;

    hba_base  = (u32)mmio;                    //  1A3400<<1
    port_base = (u32)(mmio + 0x100);          //  1A3480<<1 + 0x100
    misc_base = (u32)(mmio - (0x5FA00 << 1)); //  143A00<<1  = 287400  , 1A3400-143A00 = 5FA00

    // printk( "%s , line = %d\n" , __func__ , __LINE__);
    // pr_info("[%s] , hba_base  =0x%X \n", __func__ , hba_base );   //  0xFD346800
    // pr_info("[%s] , port_base =0x%X \n", __func__ , port_base );   //  0xFD346900
    // pr_info("[%s] , misc_base =0x%X \n", __func__ , misc_base );   // 0xFD287400

    // printk("sstar sata HW settings start!!!\n");
    ss_sata_misc_init(mmio, port_num);
    ss_sata_phy_init(mmio, phy_mode, port_num);
    // printk( "%s , line = %d\n" , __func__ , __LINE__);

    // AHCI init
    writew(HOST_RESET, (volatile void *)hba_base + (HOST_CTL));

    u32Temp = sstar_sata_wait_reg(HOST_CTL + (u32)mmio, HOST_RESET, HOST_RESET, 1, 500);
    if (u32Temp & HOST_RESET)
    {
        printk("SATA host reset fail!\n");
        return -1;
    }

    // Turn on AHCI_EN
    u32Temp = readl((void *)HOST_CTL + (u32)hba_base);
    if (u32Temp & HOST_AHCI_EN)
    {
        MHal_SATA_Setup_Port_Implement((u32)misc_base, (u32)port_base, (u32)hba_base);
        goto SS_HOST_AHCI_EN_DONE;
    }

    // Try AHCI_EN Trurn on for a few time
    for (i = 0; i < 5; i++)
    {
        u32Temp |= HOST_AHCI_EN;
        writel(u32Temp, (void *)HOST_CTL + (u32)hba_base);
        u32Temp = readl((void *)HOST_CTL + (u32)hba_base);
        if (u32Temp & HOST_AHCI_EN)
            break;
        msleep(10);
    }

    MHal_SATA_Setup_Port_Implement((u32)misc_base, (u32)port_base, (u32)hba_base);

SS_HOST_AHCI_EN_DONE:
    printk("sstar sata HW settings done!!!\n");
    return 0;
}
// EXPORT_SYMBOL(ss_sata_init);

void ss_sata_applySetting(struct ahci_host_priv *hpriv)
{
    struct sstar_ahci_priv *priv = hpriv->plat_data;

    priv->bFirstOOB = 1;
    priv->hd_link   = __ss_ahci_store_hd_link(priv->res_ahci_port0 - 0x100);

    // pr_info("[%s] , priv->res_ahci_port0 =0x%p \n", __func__ , priv->res_ahci_port0 );   // 0xfd346900
    // pr_info("[%s] , priv->res_ahci_misc  =0x%p \n", __func__ , priv->res_ahci_misc );     // 0xfd287400
}
