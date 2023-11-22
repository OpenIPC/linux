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

int g_hd_link0[4] = {0};
int g_hd_link1[4] = {0};

// extern struct _sstar_phy_adjust phy_adj0;

// extern u32 u32_gen1_gen2_vpp[];
// extern u32 u32_gen3_de_emphasis[];

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

// static u32 sstar_sata_wait_reg(u32 reg_addr, u32 mask, u32 val, unsigned long interval, unsigned long timeout)
static u32 sstar_sata_wait_reg(void *reg_addr, u32 mask, u32 val, unsigned long interval, unsigned long timeout)
{
    u32           temp;
    unsigned long timeout_vale = 0;

    temp = readl(reg_addr);

    while ((temp & mask) == val)
    {
        msleep(interval);
        timeout_vale += interval;
        if (timeout_vale > timeout)
            break;
        temp = readl(reg_addr);
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
#if 0
    printk("%s , mmio = 0x%p \n", __func__, mmio);
    printk("%s , BASE_REG_SATA0_SNPS_PA = 0x%X , IO_ADDRESS(BASE_REG_SATA0_SNPS_PA) = 0x%lX \n", __func__,
           BASE_REG_SATA0_SNPS_PA, IO_ADDRESS(BASE_REG_SATA0_SNPS_PA));
    printk("%s , BASE_REG_SATA1_SNPS_PA = 0x%X , IO_ADDRESS(BASE_REG_SATA1_SNPS_PA) = 0x%lX \n", __func__,
           BASE_REG_SATA1_SNPS_PA, IO_ADDRESS(BASE_REG_SATA1_SNPS_PA));
#endif

    if ((unsigned long)mmio == IO_ADDRESS(BASE_REG_SATA0_SNPS_PA))
    {
        hd_link = g_hd_link0;
    }
    else if ((unsigned long)mmio == IO_ADDRESS(BASE_REG_SATA1_SNPS_PA))
    {
        // pr_err("%s , i am sata2\n", __func__);
        hd_link = g_hd_link1;
    }
    else
    {
        printk("%s , fail hd_link = %d\n", __func__, ret);
        return ret;
    }

    //    priv->res_ahci_port0 = ( void *) (0x01F346B00 + 0xFFFFFF80DE000000);   //(void *) (0x1F346900);
    //    priv->res_ahci_misc = ( void *) (0x01F288200+ 0xFFFFFF80DE000000);  //(void *) (0x1F287400);

    status = readl((volatile void *)(mmio + PORT0_SSTS));

    // printk("%s , 1st enable OOB status = 0x%X \n", __func__, status);

    for (i = 0; i < counter; ++i)
    {
        // address = SATA_MAC + PORT0_CMD , ex: devmem 0x1F345118 32 0x00000000
        writel(0x00000000, (volatile void *)(mmio + PORT0_CMD));
        // address = SATA_MAC + PORT0_SCTL , ex: devmem 0x1F34512C 32 0x00000031
        writel(0x00000001, (volatile void *)(mmio + PORT0_SCTL));
        ndelay(1000);

        // devmem ( SATA_AHCI + offset PORT0_SCTL ) 32 0x00000030
        writel(0x00000000, (volatile void *)(mmio + PORT0_SCTL));
        msleep(500);

        // check hd link status
        status = readl((volatile void *)(mmio + PORT0_SSTS));

        // printk("%s , status = 0x%X \n", __func__, status);

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

    // printk("%s ,hd_link = %d\n", __func__, ret);

    return ret;
}

void ss_sata_misc_init(void *mmio, unsigned long port_base, unsigned long misc_base, int n_ports)
{
    //    void __iomem *port_base = mmio + 0x100UL;    //   1A3480<<1  = 346900
    //    void __iomem *misc_base = mmio -(0x5FA00UL<<1); //  143A00<<1  = 287400  , 1A3400-143A00 = 5FA00

    //    MHal_SATA_Clock_Config((unsigned long)misc_base, (unsigned long)port_base, TRUE);
    MHal_SATA_Clock_Config(misc_base, port_base, TRUE);
}

void ss_sata_phy_init(void *mmio, int phy_mode, int n_ports)
{
    // u32 hba_base = (u32)mmio; //  1A3400<<1
    unsigned long port_base = (unsigned long)(mmio + 0x100); //   1A3480<<1
                                                             //    u16 u16Temp;
    // unsigned long port_base = BASE_REG_SATA0_SNPS_PORT0_PA; //   0x1A3480<<1
    unsigned long GHC_PHY_D    = BASE_REG_SATA0_PHYD_PA;    // 0x143B00
    unsigned long GHC_PHY_ANA  = BASE_REG_SATA0_PHYA_PA;    //  0x143C00
    unsigned long GHC_PHY2_ANA = BASE_REG_SATA0_PHYA_2_PA;  //  0x143D00
    unsigned long GHC_GP_CTRL  = BASE_REG_SATA0_GP_CTRL_PA; //  0x144500
    unsigned long MISC_PA      = BASE_REG_SATA0_MISC_PA;    //  0x143A00
    u16           u16Temp      = 0;

    if ((n_ports < 1) || (n_ports > 4))
        pr_err("ERROR: PORT num you set is WRONG!!!\n");

    if ((phy_mode < 0) || (phy_mode > 2))
    {
        pr_err("%s ERROR: phy_mode set to default!\n", __func__);
        phy_mode = 2;
    }

    // if(port_base == SATA_GHC_1_P0_ADDRESS_START)
    if ((unsigned long)mmio == IO_ADDRESS(BASE_REG_SATA1_SNPS_PA))
    {
        port_base    = BASE_REG_SATA1_SNPS_PORT0_PA; //   0x1A3480<<1
        GHC_PHY_D    = BASE_REG_SATA1_PHYD_PA;       // 0x144200
        GHC_PHY_ANA  = BASE_REG_SATA1_PHYA_PA;       //  0x144300
        GHC_PHY2_ANA = BASE_REG_SATA1_PHYA_2_PA;     //  0x144400
        GHC_GP_CTRL  = BASE_REG_SATA1_GP_CTRL_PA;    //  0x144700
        MISC_PA      = BASE_REG_SATA1_MISC_PA;
        // pr_err("%s , i am sata2\n", __func__);
    }
#if 0
    pr_err("%s GHC_PHY_D = 0x%lX \n", __func__, GHC_PHY_D);
    pr_err("%s GHC_PHY_ANA = 0x%lX \n", __func__, GHC_PHY_ANA);
    pr_err("%s GHC_PHY2_ANA = 0x%lX \n", __func__, GHC_PHY2_ANA);
    pr_err("%s GHC_GP_CTRL  = 0x%lX \n", __func__, GHC_GP_CTRL);
    pr_err("%s GHC_GP_CTRL  = 0x%lX \n", __func__, (GHC_GP_CTRL + REG_ID_21));
    pr_err("%s GHC_GP_CTRL  = 0x%lX \n", __func__, IO_ADDRESS(GHC_GP_CTRL + REG_ID_21));
#endif

    OUTREG16((MISC_PA + REG_ID_00), 0x0000);

    // printk("sata phy init  A\n");
    //*** Bank 0x1445 h0021 => 0x0
    OUTREG16((GHC_GP_CTRL + REG_ID_21), 0x0000);

    //*** Bank 0x143C h0020 => 0x0100
    OUTREG16((GHC_PHY_ANA + REG_ID_20), 0x0100);

    //*** bringup script setting , Bank 0x143C h0030 => 0x1008 , and need to apply efuse  SATA_RXPLL_ICTRL_CDR trim
    // setting
    u16Temp = INREG16(GHC_PHY_ANA + REG_ID_30);
    u16Temp = (u16Temp & ~0x0F) | 0x000E;
    u16Temp = u16Temp | 0x1000;
    OUTREG16((GHC_PHY_ANA + REG_ID_30), u16Temp);

    OUTREG16((GHC_PHY_ANA + REG_ID_33), 0x0500); //*** Bank 0x143C h0033 => 0x0500
    // OUTREG16((GHC_PHY_ANA + REG_ID_05), 0x0002); //*** Bank 0x143C h0005 => 0x0002

    // gen3 no need
    if (phy_mode == 1) // gen2
        OUTREG16((GHC_PHY_ANA + REG_ID_56), 0x0078);
    else if (phy_mode == 0) // gen1
        OUTREG16((GHC_PHY_ANA + REG_ID_56), 0x03C0);

    OUTREG16((GHC_PHY_ANA + REG_ID_60), 0x0002); //*** Bank 0x143C h0060 => 0x0002
    OUTREG16((GHC_PHY_ANA + REG_ID_70), 0x0062); //*** Bank 0x143C h0070 => 0x0062
    OUTREG16((GHC_PHY_ANA + REG_ID_3E), 0x8000); //*** Bank 0x143C h003E => 0x8000
    OUTREG16((GHC_PHY_ANA + REG_ID_04), 0x0002); //*** Bank 0x143C h0004 => 0x0002
    OUTREG16((GHC_PHY_D + REG_ID_26), 0x1905);   //*** Bank 0x143B h0026 => 0x1905
    OUTREG16((GHC_PHY_D + REG_ID_3E), 0xB659);   //*** Bank 0x143B h003E => 0xB659
    OUTREG16((GHC_PHY_D + REG_ID_40), 0xD819);   //*** Bank 0x143B h0040 => 0xD819
    OUTREG16((GHC_PHY_D + REG_ID_61), 0x4000);   //*** Bank 0x143B h0061 => 0x4000
    OUTREG16((GHC_PHY_D + REG_ID_64), 0x0044);   //*** Bank 0x143B h0064 => 0x0044
    OUTREG16((GHC_PHY_D + REG_ID_0B), 0x0010);   //*** Bank 0x143B h000B => 0x0010
    OUTREG16((GHC_PHY_D + REG_ID_2C), 0x4812);   //*** Bank 0x143B h002C => 0x4812
    OUTREG16((GHC_PHY_D + REG_ID_0E), 0x8904);   //*** Bank 0x143B h000E => 0x8904
    OUTREG16((GHC_PHY_D + REG_ID_2A), 0x0100);   //*** Bank 0x143B h002A => 0x0100
    OUTREG16((GHC_PHY_D + REG_ID_50), 0x0300);   //*** Bank 0x143B h0050 => 0x8300
    OUTREG16((GHC_PHY_D + REG_ID_46), 0x2802);   //*** Bank 0x143B h0046 => 0x2802

    // #chiyun suggestion

    OUTREG16((GHC_PHY_D + REG_ID_0A), 0x4402); //*** Bank 0x143B h000A => 0x4402 # 143B h0A[6:2] = 5'b00000
    OUTREG16((GHC_PHY_D + REG_ID_28), 0x1018); //*** Bank 0x143B h0028 => 0x1018 # 143B h28[5:0] = 6'b011000
    OUTREG16((GHC_PHY_D + REG_ID_3F), 0x1060); //*** Bank 0x143B h003F => 0x1060 # 143B h3F[7:2] = 6'b011000
    OUTREG16((GHC_PHY_D + REG_ID_41), 0x147C); //*** Bank 0x143B h0041 => 0x147C # 143B h41[13:8] = 6'b011110
    //    OUTREG16((GHC_PHY_D + REG_ID_12), 0x4200); //*** Bank 0x143B h0012 => 0x4200 # 143B h12[12] = 1'b0
    OUTREG16((GHC_PHY_D + REG_ID_12), 0x4230); //*** Bank 0x143B h0012 => 0x4200 # 143B h12[12] = 1'b0

    //*** Bank 0x143C h0040 => 0xDD2F # 143C h41[7:0],h40[15:0] = 24'h24 dd2f
    OUTREG16((GHC_PHY_ANA + REG_ID_40), 0xDD2F);
    //*** Bank 0x143C h0041 => 0x0024 # 143C h41[7:0],h40[15:0] = 24'h24 dd2f
    OUTREG16((GHC_PHY_ANA + REG_ID_41), 0x0024);

    OUTREG16((GHC_PHY_ANA + REG_ID_44), 0x0000); //*** Bank 0x143C h0044 => 0x0000
    udelay(100);
    OUTREG16((GHC_PHY_ANA + REG_ID_44), 0x0001); //*** Bank 0x143C h0044 => 0x0001
    udelay(100);

    OUTREG16((GHC_PHY_ANA + REG_ID_21), 0x0010); //*** Bank 0x143C h0021 => 0x0010 # 143C h21[7:0] = 8'b00010000

    //*** Bank 0x143C h0010 => 0x0000 # 143C h11[15:0],10[15:0] = 32'h2a00 0000
    OUTREG16((GHC_PHY_ANA + REG_ID_10), 0x0000);
    //*** Bank 0x143C h0011 => 0x2A00 # 143C h11[15:0],10[15:0] = 32'h2a00 0000
    OUTREG16((GHC_PHY_ANA + REG_ID_11), 0x2A00);
    OUTREG16((GHC_PHY_ANA + REG_ID_31), 0x0005); //*** Bank 0x143C h0031 => 0x0005 # 143C h31[7:0] = 8'd5
    OUTREG16((GHC_PHY_ANA + REG_ID_54), 0xB132); //*** Bank 0x143C h0054 => 0xB132 # 143C h54[15] = 1'b1
    OUTREG16((GHC_PHY_ANA + REG_ID_58), 0x0100); //*** Bank 0x143C h0058 => 0x0100 # 143C h58[8] =1'b1
    // OUTREG16((GHC_PHY2_ANA + REG_ID_09), 0x0002); //*** Bank 0x143D h0009 => 0x0002 # 143D h09[4:0] = 5'b00010
    OUTREG16((GHC_PHY2_ANA + REG_ID_09), 0x0002); //*** Bank 0x143D h0009 => 0x0002 # 143D h09[4:0] = 5'b00010
    // OUTREG16((GHC_PHY_ANA + REG_ID_30), 0x100E);  //*** Bank 0x143C h0030 => 0x100E   # 143C h30[3:1] = 3'b111
    OUTREG16((GHC_PHY2_ANA + REG_ID_1C), 0x000F); //*** Bank 0x143D h001C => 0x000F  # 143D h1C[3:0] = 4'b1111
    OUTREG16((GHC_PHY2_ANA + REG_ID_1B), 0x0100); //*** Bank 0x143D h001B => 0x0100  # 143D h1B[8] = 1'b1
    OUTREG16((GHC_PHY2_ANA + REG_ID_21), 0x000F); //*** Bank 0x143D h0021 => 0x000F # 143D h21[3:0] = 4'b1111
    OUTREG16((GHC_PHY2_ANA + REG_ID_15), 0x0020); //*** Bank 0x143D h0015 => 0x0020 # 143D h15[8:4] = 5'b00010
    OUTREG16((GHC_PHY2_ANA + REG_ID_1A), 0x0030); //*** Bank 0x143D h001A => 0x0030 # 143D h1A[8:4] = 5'b00011
    OUTREG16((GHC_PHY2_ANA + REG_ID_08), 0x0000); //*** Bank 0x143D h0008 => 0x0000 # 143D h08[6:4] = 3'b010
    OUTREG16((GHC_PHY2_ANA + REG_ID_09), 0x0001); //*** Bank 0x143D
    OUTREG16((GHC_PHY2_ANA + REG_ID_06), 0x6060); //*** Bank 0x143D h0006 => 0x6060
    //    OUTREG16((GHC_PHY2_ANA + REG_ID_05), 0x0210); //*** Bank 0x143D h0005 => 0x0210
    OUTREG16((GHC_PHY2_ANA + REG_ID_05), 0x0200); //*** Bank 0x143D h0005 => 0x0210
    OUTREG16((GHC_PHY_ANA + REG_ID_30), 0x100E);  //***
    OUTREG16((GHC_PHY2_ANA + REG_ID_17), 0x0180); //*** Bank 0x143D h0017 => 0x0180

    OUTREG16((GHC_PHY_D + REG_ID_42), 0x5542);    //*** Bank 0x143B h0042 => 0x5542
    OUTREG16((GHC_PHY2_ANA + REG_ID_2B), 0x0006); //*** Bank 0x143B h002B => 0x0006
    OUTREG16((GHC_PHY2_ANA + REG_ID_0D), 0x101C); //*** Bank 0x143D h000D => 0x101C
    OUTREG16((GHC_PHY2_ANA + REG_ID_0E), 0x1010); //*** Bank 0x143D h000E => 0x1010
    OUTREG16((GHC_PHY2_ANA + REG_ID_0F), 0x0170); //*** Bank 0x143D h000F => 0x0170
    OUTREG16((GHC_PHY2_ANA + REG_ID_11), 0x7000); //*** Bank 0x143D h0011 => 0x7000 # 143D h11[15:12] = 4'b0111

    // # 143B h2F[13:12] = 2'b00
    // # 143B h2F[15:14] = 2'b00
    // #
    //#chiyun suggestion end
#if 0
    //*** Bank 0x143C h0054 => bit 15 = 1
    u16Temp = INREG16(GHC_PHY_ANA + REG_ID_54);
    u16Temp |= (0x1 << 15);
    OUTREG16((GHC_PHY_ANA + REG_ID_54), u16Temp);
    u16Temp = INREG16(GHC_PHY_ANA + REG_ID_54);

    //*** Bank 0x143C h0058 => bit 8 = 1
    u16Temp = INREG16(GHC_PHY_ANA + REG_ID_58);
    u16Temp |= (0x1 << 8);
    OUTREG16((GHC_PHY_ANA + REG_ID_58), u16Temp);
    u16Temp = INREG16(GHC_PHY_ANA + REG_ID_58);
#endif
    //*** Bank 0x143C h0005 => 0x0001
    OUTREG16((GHC_PHY_ANA + REG_ID_05), phy_mode);
}

// int ss_sata_init(void __iomem *mmio , int phy_mode, int port_num)
int ss_sata_init(struct sstar_ahci_priv *priv, int phy_mode, int port_num)
{
    u32           i;
    u32           u32Temp  = 0;
    unsigned long hba_base = 0, port_base = 0, misc_base = 0;
    void *        mmio = priv->res_ahci_port0 - 0x100;

    priv->mmio = mmio;

    hba_base  = (unsigned long)mmio;             //  1A3400<<1
    port_base = (unsigned long)(mmio + 0x100UL); //  1A3480<<1 + 0x100
    misc_base = (unsigned long)priv->res_ahci_misc;

    printk("sstar sata HW settings start!!!\n");
    ss_sata_misc_init(mmio, port_base, misc_base, port_num);
    ss_sata_phy_init(mmio, phy_mode, port_num);

    // AHCI init
    writew(HOST_RESET, (volatile void *)hba_base + (HOST_CTL));

    u32Temp = sstar_sata_wait_reg(HOST_CTL + mmio, HOST_RESET, HOST_RESET, 1, 500);
    if (u32Temp & HOST_RESET)
    {
        printk("SATA host reset fail!\n");
        return -1;
    }

    // Turn on AHCI_EN
    u32Temp = readl((void *)HOST_CTL + hba_base);
    if (u32Temp & HOST_AHCI_EN)
    {
        MHal_SATA_Setup_Port_Implement(misc_base, port_base, hba_base);
        goto SS_HOST_AHCI_EN_DONE;
    }

    // Try AHCI_EN Trurn on for a few time
    for (i = 0; i < 5; i++)
    {
        u32Temp |= HOST_AHCI_EN;
        writel(u32Temp, (void *)HOST_CTL + hba_base);
        u32Temp = readl((void *)HOST_CTL + hba_base);
        if (u32Temp & HOST_AHCI_EN)
            break;
        msleep(10);
    }

    MHal_SATA_Setup_Port_Implement(misc_base, port_base, hba_base);

SS_HOST_AHCI_EN_DONE:
    printk("sstar sata HW settings done!!!\n");
    return 0;
}
// EXPORT_SYMBOL(ss_sata_init);

void ss_sata_applySetting(int portNum, struct ahci_host_priv *hpriv)
{
    struct sstar_ahci_priv *priv = hpriv->plat_data;

    priv->bFirstOOB = 1;
    priv->hd_link   = __ss_ahci_store_hd_link(priv->res_ahci_port0 - 0x100);
    ss_sata_applyRxEq(portNum, priv);

    // pr_info("[%s] , priv->res_ahci_port0 =0x%p \n", __func__ , priv->res_ahci_port0 );   // 0xfd346900
    // pr_info("[%s] , priv->res_ahci_misc  =0x%p \n", __func__ , priv->res_ahci_misc );     // 0xfd287400
}

void hal_sata_set_testGen(int portNum, int gen)
{
    u16           u16Temp;
    unsigned long GHC_PHY_D = BASE_REG_SATA0_PHYD_PA; // 0x143B00

    if (portNum == 1)
    {
        GHC_PHY_D = BASE_REG_SATA1_PHYD_PA; // 0x144200
        // pr_err("%s , i am sata2\n", __func__);
    }

    //*** Bank 0x143B h002A => bit3_2
    u16Temp = INREG16(GHC_PHY_D + REG_ID_2A);
    // printk("%s , pre Bank 0x143B 0x2A = 0x%X\n", __func__ , u16Temp);
    u16Temp &= ~(0x3 << 2);
    u16Temp |= (gen << 2);
    OUTREG16((GHC_PHY_D + REG_ID_2A), u16Temp);
    u16Temp = INREG16(GHC_PHY_D + REG_ID_2A);
    // printk("%s , after Bank 0x143B 0x2A = 0x%X\n", __func__ , u16Temp);
}

void hal_sata_set_db_level(int portNum, int db, int level)
{
    int offset40_11_6  = 0;
    int offset40_15_12 = 0;
    int offset41_1_0   = 0;
    int offset45_8_6   = 0;
    u16 u16Temp;

    int table_offset40_11_6[3][DE_EMPHASIS_LEVEL_MAX] = {
        {0x13, 0x17, 0x1C, 0x20, 0x25, 0x29, 0x2E, 0x33},
        {0x12, 0x16, 0x1A, 0x1F, 0x23, 0x27, 0x2C, 0x30},
        {0x11, 0x14, 0x19, 0x1D, 0x21, 0x26, 0x2A, 0x2E},
    };

    int table_offset40_15_12[3][DE_EMPHASIS_LEVEL_MAX] = {
        {0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A},
        {0x05, 0x06, 0x08, 0x09, 0x0A, 0x0B, 0x0D, 0x0E},
        {0x07, 0x08, 0x0A, 0x0B, 0x0D, 0x0F, 0x01, 0x03},
    };

    int table_offset41_1_0[3][DE_EMPHASIS_LEVEL_MAX] = {
        {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
        {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
        {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01},
    };

    int table_offset45_8_6[3][DE_EMPHASIS_LEVEL_MAX] = {
        {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x07},
        {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x07},
        {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 07},
    };

    unsigned long GHC_PHY_D    = BASE_REG_SATA0_PHYD_PA;    // 0x143B00
    unsigned long GHC_PHY_ANA  = BASE_REG_SATA0_PHYA_PA;    //  0x143C00
    unsigned long GHC_PHY2_ANA = BASE_REG_SATA0_PHYA_2_PA;  //  0x143D00
    unsigned long GHC_GP_CTRL  = BASE_REG_SATA0_GP_CTRL_PA; //  0x144500
    unsigned long MISC_PA      = BASE_REG_SATA0_MISC_PA;

    if (portNum == 1)
    {
        GHC_PHY_D    = BASE_REG_SATA1_PHYD_PA;    // 0x144200
        GHC_PHY_ANA  = BASE_REG_SATA1_PHYA_PA;    //  0x144300
        GHC_PHY2_ANA = BASE_REG_SATA1_PHYA_2_PA;  //  0x144400
        GHC_GP_CTRL  = BASE_REG_SATA1_GP_CTRL_PA; //  0x144700
        MISC_PA      = BASE_REG_SATA1_MISC_PA;
        // pr_err("%s , i am sata2\n", __func__);
    }

    if ((db < 0) || (db >= DE_EMPHASIS_MAX_DB))
    {
        pr_err("%s , bad db = 0x%X\n", __func__, db);
        return;
    }

    if ((level < 0) || (db >= DE_EMPHASIS_LEVEL_MAX))
    {
        pr_err("%s , bad level = 0x%X\n", __func__, level);
        return;
    }

    offset40_11_6  = table_offset40_11_6[db][level];
    offset40_15_12 = table_offset40_15_12[db][level];
    offset41_1_0   = table_offset41_1_0[db][level];
    offset45_8_6   = table_offset45_8_6[db][level];

    //*** Bank 0x143B h0040 => bit11_6
    u16Temp = INREG16(GHC_PHY_D + REG_ID_40);
    // printk("%s , pre Bank 0x143B 0x40 = 0x%X\n", __func__ , u16Temp);
    u16Temp &= ~(0X0FC0);
    u16Temp |= (offset40_11_6 << 6);
    OUTREG16((GHC_PHY_D + REG_ID_40), u16Temp);
    u16Temp = INREG16(GHC_PHY_D + REG_ID_40);
    // printk("%s , after Bank 0x143B 0x40 = 0x%X\n", __func__ , u16Temp);

    //*** Bank 0x143B h0040 => bit15_12
    u16Temp = INREG16(GHC_PHY_D + REG_ID_40);
    // printk("%s , pre Bank 0x143B 0x40 = 0x%X\n", __func__ , u16Temp);
    u16Temp &= ~(0X0F000);
    u16Temp |= (offset40_15_12 << 12);
    OUTREG16((GHC_PHY_D + REG_ID_40), u16Temp);
    u16Temp = INREG16(GHC_PHY_D + REG_ID_40);
    // printk("%s , after Bank 0x143B 0x40 = 0x%X\n", __func__ , u16Temp);

    //*** Bank 0x143B h0041 => bit1_0
    u16Temp = INREG16(GHC_PHY_D + REG_ID_41);
    // printk("%s , pre Bank 0x143B 0x41 = 0x%X\n", __func__ , u16Temp);
    u16Temp &= ~(0X03);
    u16Temp |= (offset41_1_0 << 0);
    OUTREG16((GHC_PHY_D + REG_ID_41), u16Temp);
    u16Temp = INREG16(GHC_PHY_D + REG_ID_41);
    // printk("%s , after Bank 0x143B 0x41 = 0x%X\n", __func__ , u16Temp);

    //*** Bank 0x143B h0045 => bit8_6
    u16Temp = INREG16(GHC_PHY_D + REG_ID_45);
    // printk("%s , pre Bank 0x143B 0x45 = 0x%X\n", __func__ , u16Temp);
    u16Temp &= ~(0X01C0);
    u16Temp |= (offset45_8_6 << 6);
    OUTREG16((GHC_PHY_D + REG_ID_45), u16Temp);
    u16Temp = INREG16(GHC_PHY_D + REG_ID_45);
    // printk("%s , after Bank 0x143B 0x45 = 0x%X\n", __func__ , u16Temp);
}

void hal_sata_set_VppLevel(int portNum, int gen, int level)
{
    u16 u16Temp;

    int offset26_13_8 = 0;
    int offset3E_5_0  = 0;
    int offset40_5_0  = 0;
    int offset45_2_0  = 0;
    int offset45_5_3  = 0;
    int offset45_8_6  = 0;

    unsigned long GHC_PHY_D = BASE_REG_SATA0_PHYD_PA; // 0x143B00
    // unsigned long GHC_PHY_ANA  = BASE_REG_SATA0_PHYA_PA;    //  0x143C00
    // unsigned long GHC_PHY2_ANA = BASE_REG_SATA0_PHYA_2_PA;  //  0x143D00
    // unsigned long GHC_GP_CTRL  = BASE_REG_SATA0_GP_CTRL_PA; //  0x144500
    // unsigned long MISC_PA      = BASE_REG_SATA0_MISC_PA;

    int table_offset26_13_8[DE_EMPHASIS_LEVEL_MAX] = {0x14, 0x19, 0x1E, 0x23, 0x28, 0x00, 0x00, 0x00};
    int table_offset45_2_0[DE_EMPHASIS_LEVEL_MAX]  = {0x01, 0x02, 0x03, 0x04, 0x05, 0x00, 0x00, 0x00};

    int table_offset3E_5_0[DE_EMPHASIS_LEVEL_MAX] = {0x14, 0x19, 0x1E, 0x23, 0x28, 0x2D, 0x00, 0x00};
    int table_offset45_5_3[DE_EMPHASIS_LEVEL_MAX] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x00, 0x00};

    int table_offset40_5_0[DE_EMPHASIS_LEVEL_MAX] = {0x14, 0x19, 0x1E, 0x23, 0x28, 0x2D, 0x32, 0x37};
    int table_offset45_8_6[DE_EMPHASIS_LEVEL_MAX] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x7};

    if (portNum == 1)
    {
        GHC_PHY_D = BASE_REG_SATA1_PHYD_PA; // 0x144200
        // GHC_PHY_ANA  = BASE_REG_SATA1_PHYA_PA;    //  0x144300
        // GHC_PHY2_ANA = BASE_REG_SATA1_PHYA_2_PA;  //  0x144400
        // GHC_GP_CTRL  = BASE_REG_SATA1_GP_CTRL_PA; //  0x144700
        // MISC_PA      = BASE_REG_SATA1_MISC_PA;
        //  pr_err("%s , i am sata2\n", __func__);
    }

    if ((gen == SATA_GEN1) || (gen == SATA_GEN2) || (gen == SATA_GEN3))
    {
        //*** Bank 0x143B h00A => bit1 = 0x0
        u16Temp = INREG16(GHC_PHY_D + REG_ID_0A);
        // printk("%s , pre Bank 0x143B 0x0A = 0x%X\n", __func__ , u16Temp);
        u16Temp &= ~(0x1 << 1);
        OUTREG16((GHC_PHY_D + REG_ID_0A), u16Temp);
        u16Temp = INREG16(GHC_PHY_D + REG_ID_0A);
        // printk("%s , after Bank 0x143B 0x0A = 0x%X\n", __func__ , u16Temp);

        //*** Bank 0x143B h002F => bit15_14 = 0x01
        u16Temp = INREG16(GHC_PHY_D + REG_ID_2F);
        // printk("%s , pre Bank 0x143B 0x2F = 0x%X\n", __func__ , u16Temp);
        u16Temp &= ~(0x3 << 14);
        u16Temp |= (0x2 << 14);
        OUTREG16((GHC_PHY_D + REG_ID_2F), u16Temp);
        u16Temp = INREG16(GHC_PHY_D + REG_ID_2F);
        // printk("%s , after Bank 0x143B 0x2F = 0x%X\n", __func__ , u16Temp);
    }

    if ((gen == SATA_GEN1) && (level < 0 || level > DE_EMPHASIS_LEVEL_0p8))
    {
        pr_err("%s , bad gen = 0x%0X level = 0x%X\n", __func__, gen, level);
        return;
    }

    if ((gen == SATA_GEN2) && (level < 0 || level > DE_EMPHASIS_LEVEL_0p9))
    {
        pr_err("%s , bad gen = 0x%0X level = 0x%X\n", __func__, gen, level);
        return;
    }

    if ((gen == SATA_GEN3) && (level < 0 || level > DE_EMPHASIS_LEVEL_1p1))
    {
        pr_err("%s , bad gen = 0x%0X level = 0x%X\n", __func__, gen, level);
        return;
    }

    if (gen == SATA_GEN1)
    {
        offset26_13_8 = table_offset26_13_8[level];
        offset45_2_0  = table_offset45_2_0[level];

        //*** Bank 0x143B h0026 => bit13_8
        u16Temp = INREG16(GHC_PHY_D + REG_ID_26);
        // printk("%s , pre Bank 0x143B 0x26 = 0x%X\n", __func__ , u16Temp);
        u16Temp &= ~(0X03F00);
        u16Temp |= (offset26_13_8 << 8);
        OUTREG16((GHC_PHY_D + REG_ID_26), u16Temp);
        u16Temp = INREG16(GHC_PHY_D + REG_ID_26);
        // printk("%s , after Bank 0x143B 0x26 = 0x%X\n", __func__ , u16Temp);

        //*** Bank 0x143B h0045 => bit2_0
        u16Temp = INREG16(GHC_PHY_D + REG_ID_45);
        // printk("%s , pre Bank 0x143B 0x45 = 0x%X\n", __func__ , u16Temp);
        u16Temp &= ~(0X07);
        u16Temp |= (offset45_2_0 << 0);
        OUTREG16((GHC_PHY_D + REG_ID_45), u16Temp);
        u16Temp = INREG16(GHC_PHY_D + REG_ID_45);
        // printk("%s , after Bank 0x143B 0x45 = 0x%X\n", __func__ , u16Temp);
    }
    else if (gen == SATA_GEN2)
    {
        offset3E_5_0 = table_offset3E_5_0[level];
        offset45_5_3 = table_offset45_5_3[level];

        //*** Bank 0x143B h003E => bit5_0
        u16Temp = INREG16(GHC_PHY_D + REG_ID_3E);
        // printk("%s , pre Bank 0x143B 0x3E = 0x%X\n", __func__ , u16Temp);
        u16Temp &= ~(0X03F);
        u16Temp |= (offset3E_5_0 << 0);
        OUTREG16((GHC_PHY_D + REG_ID_3E), u16Temp);
        u16Temp = INREG16(GHC_PHY_D + REG_ID_3E);
        // printk("%s , after Bank 0x143B 0x3E = 0x%X\n", __func__ , u16Temp);

        //*** Bank 0x143B h0045 => bit5_3
        u16Temp = INREG16(GHC_PHY_D + REG_ID_45);
        // printk("%s , pre Bank 0x143B 0x45 = 0x%X\n", __func__ , u16Temp);
        u16Temp &= ~(0X038);
        u16Temp |= (offset45_5_3 << 3);
        OUTREG16((GHC_PHY_D + REG_ID_45), u16Temp);
        u16Temp = INREG16(GHC_PHY_D + REG_ID_45);
        // printk("%s , after Bank 0x143B 0x45 = 0x%X\n", __func__ , u16Temp);
    }
    else if (gen == SATA_GEN3)
    {
        offset40_5_0 = table_offset40_5_0[level];
        offset45_8_6 = table_offset45_8_6[level];

        //*** Bank 0x143B h0040 => bit5_0
        // printk("%s , pre Bank 0x143B 0x40 = 0x%X\n", __func__ , u16Temp);
        u16Temp = INREG16(GHC_PHY_D + REG_ID_40);
        u16Temp &= ~(0X03F);
        u16Temp |= (offset40_5_0 << 0);
        OUTREG16((GHC_PHY_D + REG_ID_40), u16Temp);
        u16Temp = INREG16(GHC_PHY_D + REG_ID_40);
        // printk("%s , after Bank 0x143B 0x40 = 0x%X\n", __func__ , u16Temp);

        //*** Bank 0x143B h0045 => bit8_6
        u16Temp = INREG16(GHC_PHY_D + REG_ID_45);
        // printk("%s , pre Bank 0x143B 0x45 = 0x%X\n", __func__ , u16Temp);
        u16Temp &= ~(0X01C0);
        u16Temp |= (offset45_8_6 << 6);
        OUTREG16((GHC_PHY_D + REG_ID_45), u16Temp);
        u16Temp = INREG16(GHC_PHY_D + REG_ID_45);
        // printk("%s , after Bank 0x143B 0x45 = 0x%X\n", __func__ , u16Temp);
    }
    else
    {
        return;
    }
}

void hal_sata_set_cts_pattern(int portNum, struct _sstar_phy_adjust *phy_adj)
{
    u16 u16Temp;

    int offset7C_15_0 = 0;
    int offset7D_15_0 = 0;
    int offset46_15_0 = 0;
    int offset21_15_0 = 0;

    unsigned long GHC_PHY_D    = BASE_REG_SATA0_PHYD_PA;    // 0x143B00
    unsigned long GHC_PHY_ANA  = BASE_REG_SATA0_PHYA_PA;    //  0x143C00
    unsigned long GHC_PHY2_ANA = BASE_REG_SATA0_PHYA_2_PA;  //  0x143D00
    unsigned long GHC_GP_CTRL  = BASE_REG_SATA0_GP_CTRL_PA; //  0x144500
    unsigned long MISC_PA      = BASE_REG_SATA0_MISC_PA;

    if (portNum == 1)
    {
        GHC_PHY_D    = BASE_REG_SATA1_PHYD_PA;    // 0x144200
        GHC_PHY_ANA  = BASE_REG_SATA1_PHYA_PA;    //  0x144300
        GHC_PHY2_ANA = BASE_REG_SATA1_PHYA_2_PA;  //  0x144400
        GHC_GP_CTRL  = BASE_REG_SATA1_GP_CTRL_PA; //  0x144700
        MISC_PA      = BASE_REG_SATA1_MISC_PA;
        // pr_err("%s , i am sata2\n", __func__);
    }

    if (phy_adj->test_gen == SATA_GEN1)
    {
        if (phy_adj->cts_pattern == SATA_CTS_PATTERN_HFTP)
        {
            offset7C_15_0 = 0x5555;
            offset7D_15_0 = 0x15;
            offset46_15_0 = 0x2802;
            offset21_15_0 = 0x0205;
        }
        else if (phy_adj->cts_pattern == SATA_CTS_PATTERN_MFTP)
        {
            offset7C_15_0 = 0x3333;
            offset7D_15_0 = 0x13;
            offset46_15_0 = 0x2802;
            offset21_15_0 = 0x0205;
        }
        else if (phy_adj->cts_pattern == SATA_CTS_PATTERN_LFTP)
        {
            offset7C_15_0 = 0x8E1C;
            offset7D_15_0 = 0x17;
            offset46_15_0 = 0x2802;
            offset21_15_0 = 0x0205;
        }
        else if (phy_adj->cts_pattern == SATA_CTS_PATTERN_LBP)
        {
            offset7C_15_0 = 0x2C2C;
            offset7D_15_0 = 0x00;
            offset46_15_0 = 0x6802;
            offset21_15_0 = 0x0205;
        }
        else if (phy_adj->cts_pattern == SATA_CTS_PATTERN_SSOP)
        {
            offset7C_15_0 = 0x32B3;
            offset7D_15_0 = 0x15;
            offset46_15_0 = 0x2802;
            offset21_15_0 = 0x0205;
        }
    }
    else if (phy_adj->test_gen == SATA_GEN2)
    {
        if (phy_adj->cts_pattern == SATA_CTS_PATTERN_HFTP)
        {
            offset7C_15_0 = 0x5555;
            offset7D_15_0 = 0x15;
            offset46_15_0 = 0x2802;
            offset21_15_0 = 0x0105;
        }
        else if (phy_adj->cts_pattern == SATA_CTS_PATTERN_MFTP)
        {
            offset7C_15_0 = 0x5555;
            offset7D_15_0 = 0x15;
            offset46_15_0 = 0x2802;
            offset21_15_0 = 0x0205;
        }
        else if (phy_adj->cts_pattern == SATA_CTS_PATTERN_LFTP)
        {
            offset7C_15_0 = 0x8E1C;
            offset7D_15_0 = 0x17;
            offset46_15_0 = 0x2802;
            offset21_15_0 = 0x0105;
        }
        else if (phy_adj->cts_pattern == SATA_CTS_PATTERN_LBP)
        {
            offset7C_15_0 = 0x2C2C;
            offset7D_15_0 = 0x00;
            offset46_15_0 = 0x6802;
            offset21_15_0 = 0x0105;
        }
        else if (phy_adj->cts_pattern == SATA_CTS_PATTERN_SSOP)
        {
            offset7C_15_0 = 0x32B3;
            offset7D_15_0 = 0x15;
            offset46_15_0 = 0x2802;
            offset21_15_0 = 0x0105;
        }
    }
    else if (phy_adj->test_gen == SATA_GEN3)
    {
        if (phy_adj->cts_pattern == SATA_CTS_PATTERN_HFTP)
        {
            offset7C_15_0 = 0x5555;
            offset7D_15_0 = 0x15;
            offset46_15_0 = 0x2802;
            offset21_15_0 = 0x0005;
        }
        else if (phy_adj->cts_pattern == SATA_CTS_PATTERN_MFTP)
        {
            offset7C_15_0 = 0x3333;
            offset7D_15_0 = 0x13;
            offset46_15_0 = 0x2802;
            offset21_15_0 = 0x0005;
        }
        else if (phy_adj->cts_pattern == SATA_CTS_PATTERN_LFTP)
        {
            offset7C_15_0 = 0x8E1C;
            offset7D_15_0 = 0x17;
            offset46_15_0 = 0x2802;
            offset21_15_0 = 0x0005;
        }
        else if (phy_adj->cts_pattern == SATA_CTS_PATTERN_LBP)
        {
            offset7C_15_0 = 0x2C2C;
            offset7D_15_0 = 0x00;
            offset46_15_0 = 0x6802;
            offset21_15_0 = 0x0005;
        }
        else if (phy_adj->cts_pattern == SATA_CTS_PATTERN_SSOP)
        {
            offset7C_15_0 = 0x32B3;
            offset7D_15_0 = 0x15;
            offset46_15_0 = 0x2802;
            offset21_15_0 = 0x0005;
        }
    }

    //*** Bank 0x143B h0A => bit8 = 0x01
    u16Temp = INREG16(GHC_PHY_D + REG_ID_0A);
    // printk("%s , pre Bank 0x143B 0x0A = 0x%X\n", __func__ , u16Temp);
    u16Temp &= ~(0x1 << 8);
    u16Temp |= (0x1 << 8);
    OUTREG16((GHC_PHY_D + REG_ID_0A), u16Temp);
    u16Temp = INREG16(GHC_PHY_D + REG_ID_0A);
    // printk("%s , after Bank 0x143B 0x0A = 0x%X\n", __func__ , u16Temp);

    //*** Bank 0x143B h29 => bit1 = 0x01
    u16Temp = INREG16(GHC_PHY_D + REG_ID_29);
    // printk("%s , pre Bank 0x143B 0x29 = 0x%X\n", __func__ , u16Temp);
    u16Temp &= ~(0x1 << 1);
    u16Temp |= (0x1 << 1);
    OUTREG16((GHC_PHY_D + REG_ID_29), u16Temp);
    u16Temp = INREG16(GHC_PHY_D + REG_ID_29);
    // printk("%s , after Bank 0x143B 0x29 = 0x%X\n", __func__ , u16Temp);

    //*** Bank 0x143B h0B => bit0 = 0x01
    u16Temp = INREG16(GHC_PHY_D + REG_ID_0B);
    // printk("%s , pre Bank 0x143B 0x0B = 0x%X\n", __func__ , u16Temp);
    u16Temp &= ~(0x1 << 0);
    u16Temp |= (0x1 << 0);
    OUTREG16((GHC_PHY_D + REG_ID_0B), u16Temp);
    u16Temp = INREG16(GHC_PHY_D + REG_ID_0B);
    // printk("%s , after Bank 0x143B 0x0B = 0x%X\n", __func__ , u16Temp);

    //*** Bank 0x143C h007C => bit15_0
    u16Temp = INREG16(GHC_PHY_ANA + REG_ID_7C);
    // printk("%s , pre Bank 0x143C 0x7C = 0x%X\n", __func__ , u16Temp);
    OUTREG16((GHC_PHY_ANA + REG_ID_7C), offset7C_15_0);
    u16Temp = INREG16(GHC_PHY_ANA + REG_ID_7C);
    // printk("%s , after Bank 0x143C 0x7C = 0x%X\n", __func__ , u16Temp);

    //*** Bank 0x143C h007D => bit15_0
    u16Temp = INREG16(GHC_PHY_ANA + REG_ID_7D);
    // printk("%s , pre Bank 0x143C 0x7D = 0x%X\n", __func__ , u16Temp);
    OUTREG16((GHC_PHY_ANA + REG_ID_7D), offset7D_15_0);
    u16Temp = INREG16(GHC_PHY_ANA + REG_ID_7D);
    // printk("%s , after Bank 0x143C 0x7D = 0x%X\n", __func__ , u16Temp);

    //*** Bank 0x143B h0046 => bit15_0
    u16Temp = INREG16(GHC_PHY_D + REG_ID_46);
    // printk("%s , pre Bank 0x143B 0x46 = 0x%X\n", __func__ , u16Temp);
    OUTREG16((GHC_PHY_D + REG_ID_46), offset46_15_0);
    u16Temp = INREG16(GHC_PHY_D + REG_ID_46);
    // printk("%s , after Bank 0x143B 0x46 = 0x%X\n", __func__ , u16Temp);

    //*** Bank 0x143C h0021 => bit15_0
    u16Temp = INREG16(GHC_PHY_ANA + REG_ID_21);
    // printk("%s , pre Bank 0x143C 0x21 = 0x%X\n", __func__ , u16Temp);
    OUTREG16((GHC_PHY_ANA + REG_ID_21), offset21_15_0);
    u16Temp = INREG16(GHC_PHY_ANA + REG_ID_21);
    // printk("%s , after Bank 0x143C 0x21 = 0x%X\n", __func__ , u16Temp);
}

void hal_sata_set_de_emphasis(int portNum, struct _sstar_phy_adjust *phy_adj)
{
    u16 u16Temp;

    unsigned long GHC_PHY_D    = BASE_REG_SATA0_PHYD_PA;    // 0x143B00
    unsigned long GHC_PHY_ANA  = BASE_REG_SATA0_PHYA_PA;    //  0x143C00
    unsigned long GHC_PHY2_ANA = BASE_REG_SATA0_PHYA_2_PA;  //  0x143D00
    unsigned long GHC_GP_CTRL  = BASE_REG_SATA0_GP_CTRL_PA; //  0x144500
    unsigned long MISC_PA      = BASE_REG_SATA0_MISC_PA;

    if (portNum == 1)
    {
        GHC_PHY_D    = BASE_REG_SATA1_PHYD_PA;    // 0x144200
        GHC_PHY_ANA  = BASE_REG_SATA1_PHYA_PA;    //  0x144300
        GHC_PHY2_ANA = BASE_REG_SATA1_PHYA_2_PA;  //  0x144400
        GHC_GP_CTRL  = BASE_REG_SATA1_GP_CTRL_PA; //  0x144700
        MISC_PA      = BASE_REG_SATA1_MISC_PA;
        // pr_err("%s , i am sata2\n", __func__);
    }

    //*** Bank 0x143B h00A => bit1 = 0x0

    u16Temp = INREG16(GHC_PHY_D + REG_ID_0A);
    // printk("%s , pre Bank 0x143B 0x0A = 0x%X\n", __func__ , u16Temp);
    u16Temp &= ~(0x1 << 1);
    OUTREG16((GHC_PHY_D + REG_ID_0A), u16Temp);
    u16Temp = INREG16(GHC_PHY_D + REG_ID_0A);
    // printk("%s , after Bank 0x143B 0x0A = 0x%X\n", __func__ , u16Temp);

    if (phy_adj->dmhs.enable == 1)
    {
        //*** Bank 0x143B h002F => bit15_14 = 0x01
        u16Temp = INREG16(GHC_PHY_D + REG_ID_2F);
        // printk("%s , pre Bank 0x143B 0x2F = 0x%X\n", __func__ , u16Temp);
        u16Temp &= ~(0x3 << 14);
        u16Temp |= (0x1 << 14);
        OUTREG16((GHC_PHY_D + REG_ID_2F), u16Temp);
        u16Temp = INREG16(GHC_PHY_D + REG_ID_2F);
        // printk("%s , after Bank 0x143B 0x2F = 0x%X\n", __func__ , u16Temp);

        hal_sata_set_testGen(portNum, SATA_GEN3); // 2 = gen3
        // printk("%s , enable de-emphasis force change to test Gen3\n", __func__);
        // printk("%s , if cts_pattern active , please set cts_pattern again\n", __func__);
        phy_adj->test_gen = SATA_GEN3; // 2 = gen3
        hal_sata_set_db_level(portNum, phy_adj->dmhs.db, phy_adj->dmhs.level);
    }
    else
    {
        //*** Bank 0x143B h002F => bit15_14 = 0x01
        u16Temp = INREG16(GHC_PHY_D + REG_ID_2F);
        // printk("%s , pre Bank 0x143B 0x2F = 0x%X\n", __func__ , u16Temp);
        u16Temp &= ~(0x3 << 14);
        u16Temp |= (0x2 << 14);
        OUTREG16((GHC_PHY_D + REG_ID_2F), u16Temp);
        u16Temp = INREG16(GHC_PHY_D + REG_ID_2F);
        // printk("%s , after Bank 0x143B 0x2F = 0x%X\n", __func__ , u16Temp);
        hal_sata_set_testGen(portNum, phy_adj->test_gen);
        hal_sata_set_VppLevel(portNum, phy_adj->test_gen, phy_adj->dmhs.level);
    }
}
// struct _sstar_phy_adjust phy_adj0;

// void ss_sata_applyDmhs(int portNum, u32 *u32_gen1_gen2_vpp, u32 *u32_gen3_de_emphasis,
//                        struct _sstar_phy_adjust *phy_adj)
void ss_sata_applyDmhs(int portNum, struct sstar_ahci_priv *priv, struct _sstar_phy_adjust *phy_adj)

{
    if (priv->tx_gen1_gen2_vpp[0] != 0xFF
        && priv->tx_gen1_gen2_vpp[0] <= DE_EMPHASIS_LEVEL_0p8) // Gen1 Vppdiff 0.4/0.5/0.6/0.7/0.8
    {
        hal_sata_set_VppLevel(portNum, SATA_GEN1, priv->tx_gen1_gen2_vpp[0]);
    }

    if (priv->tx_gen1_gen2_vpp[1] != 0xFF
        && priv->tx_gen1_gen2_vpp[1] <= DE_EMPHASIS_LEVEL_0p9) // Gen2 Vppdiff 0.4/0.5/0.6/0.7/0.8/0.9
    {
        hal_sata_set_VppLevel(portNum, SATA_GEN2, priv->tx_gen1_gen2_vpp[1]);
    }

    if (priv->tx_gen3_de_emphasis[0] == 0)
    {
        if (priv->tx_gen3_de_emphasis[2] != 0xFF
            && priv->tx_gen3_de_emphasis[2] <= DE_EMPHASIS_LEVEL_MAX) // Gen3 Vppdiff 0.4/0.5/0.6/0.7/0.8/0.9/1.0/1.1
        {
            hal_sata_set_VppLevel(portNum, SATA_GEN3, priv->tx_gen3_de_emphasis[2]);
        }
    }
    else if (priv->tx_gen3_de_emphasis[0] == 1)
    {
        if (priv->tx_gen3_de_emphasis[1] > DE_EMPHASIS_MAX_DB) // -1.5db/-2.5db/-3.5db
        {
            pr_err("%s wrong tx_gen3_de_emphasis[1]=0x%X\n", __func__, priv->tx_gen3_de_emphasis[1]);
            return;
        }

        if (priv->tx_gen3_de_emphasis[2] > DE_EMPHASIS_LEVEL_MAX) // vpp diff 0.4/0.5/0.6/0.7/0.8/0.9/1.0/1.1
        {
            pr_err("%s wrong tx_gen3_de_emphasis[2]=0x%X\n", __func__, priv->tx_gen3_de_emphasis[2]);
            return;
        }

        phy_adj->dmhs.enable = priv->tx_gen3_de_emphasis[0];
        phy_adj->dmhs.db     = priv->tx_gen3_de_emphasis[1];
        phy_adj->dmhs.level  = priv->tx_gen3_de_emphasis[2];

        hal_sata_set_de_emphasis(portNum, phy_adj);
        phy_adj->new_de_emphasis = 1;
    }
}

void ss_sata_applyRxEq(int portNum, struct sstar_ahci_priv *priv)
{
    u16 u16Temp;

    unsigned long GHC_PHY_D    = BASE_REG_SATA0_PHYD_PA;   // 0x143B00
    unsigned long GHC_PHY2_ANA = BASE_REG_SATA0_PHYA_2_PA; //  0x143D00

    if (portNum == 1)
    {
        GHC_PHY_D    = BASE_REG_SATA1_PHYD_PA;   // 0x144200
        GHC_PHY2_ANA = BASE_REG_SATA1_PHYA_2_PA; //  0x144400
        //  pr_err("%s , i am sata2\n", __func__);
    }
#if 0
    if (priv->rx_eq_cs[0] != 0xFF && priv->rx_eq_cs[0] < 0x10)
    { // CS Gen1  //*** Bank 0x143B h0036 , bit11~8
        u16Temp = INREG16(GHC_PHY_D + REG_ID_36);
        // printk("%s , pre Bank 0x143B 0x36 = 0x%X\n", __func__, u16Temp);
        u16Temp &= ~(0X0F00);
        u16Temp |= (priv->rx_eq_cs[0] << 8);
        OUTREG16((GHC_PHY_D + REG_ID_36), u16Temp);
        u16Temp = INREG16(GHC_PHY_D + REG_ID_36);
        // printk("%s , after Bank 0x143B 0x36 = 0x%X\n", __func__, u16Temp);
    }

    if (priv->rx_eq_cs[1] != 0xFF && priv->rx_eq_cs[1] < 0x10)
    { // CS Gen2  //*** Bank 0x143B h0042 , bit3~0
        u16Temp = INREG16(GHC_PHY_D + REG_ID_42);
        // printk("%s , pre Bank 0x143B 0x42 = 0x%X\n", __func__, u16Temp);
        u16Temp &= ~(0X000F);
        u16Temp |= (priv->rx_eq_cs[1] << 0);
        OUTREG16((GHC_PHY_D + REG_ID_42), u16Temp);
        u16Temp = INREG16(GHC_PHY_D + REG_ID_42);
        // printk("%s , after Bank 0x143B 0x42 = 0x%X\n", __func__, u16Temp);
    }

    if (priv->rx_eq_cs[2] != 0xFF && priv->rx_eq_cs[2] < 0x10)
    { // CS Gen3  //*** Bank 0x143B h0042 , bit7~4

        u16Temp = INREG16(GHC_PHY_D + REG_ID_42);
        // printk("%s , pre Bank 0x143B 0x42 = 0x%X\n", __func__, u16Temp);
        u16Temp &= ~(0X00F0);
        u16Temp |= (priv->rx_eq_cs[2] << 4);
        OUTREG16((GHC_PHY_D + REG_ID_42), u16Temp);
        u16Temp = INREG16(GHC_PHY_D + REG_ID_42);
        // printk("%s , after Bank 0x143B 0x42 = 0x%X\n", __func__, u16Temp);
    }
#else
    if (priv->rx_eq_ctle != 0xFF && priv->rx_eq_ctle < 0x1F)
    { // CTLE , *** Bank 0x143D h002B , bit4~0

        u16Temp = INREG16(GHC_PHY2_ANA + REG_ID_2B);
        // printk("%s , pre Bank 0x143D 0x2B = 0x%X\n", __func__, u16Temp);
        u16Temp &= ~(0X001F);
        u16Temp |= (priv->rx_eq_ctle);
        OUTREG16((GHC_PHY2_ANA + REG_ID_2B), u16Temp);
        u16Temp = INREG16(GHC_PHY2_ANA + REG_ID_2B);
        // printk("%s , after Bank 0x143D 0x2B = 0x%X\n", __func__, u16Temp);
    }
#endif

    if (priv->rx_eq_rs1[0] != 0xFF && priv->rx_eq_rs1[1] < 4)
    { // RS1 Gen1  //*** Bank 0x143B h0034 , bit11~10

        u16Temp = INREG16(GHC_PHY_D + REG_ID_34);
        // printk("%s , pre Bank 0x143B 0x34 = 0x%X\n", __func__, u16Temp);
        u16Temp &= ~(0X0C00);
        u16Temp |= (priv->rx_eq_rs1[0] << 10);
        OUTREG16((GHC_PHY_D + REG_ID_34), u16Temp);
        u16Temp = INREG16(GHC_PHY_D + REG_ID_34);
        // printk("%s , after Bank 0x143B 0x34 = 0x%X\n", __func__, u16Temp);
    }

    if (priv->rx_eq_rs1[1] != 0xFF && priv->rx_eq_rs1[1] < 4)
    { // RS1 Gen2  //*** Bank 0x143B h0042 , bit9~8

        u16Temp = INREG16(GHC_PHY_D + REG_ID_42);
        // printk("%s , pre Bank 0x143B 0x42 = 0x%X\n", __func__, u16Temp);
        u16Temp &= ~(0X0300);
        u16Temp |= (priv->rx_eq_rs1[1] << 8);
        OUTREG16((GHC_PHY_D + REG_ID_42), u16Temp);
        u16Temp = INREG16(GHC_PHY_D + REG_ID_42);
        // printk("%s , after Bank 0x143B 0x42 = 0x%X\n", __func__, u16Temp);
    }

    if (priv->rx_eq_rs1[2] != 0xFF && priv->rx_eq_rs1[2] < 4)
    { // RS1 Gen3  //*** Bank 0x143B h0042 , bit10~11

        u16Temp = INREG16(GHC_PHY_D + REG_ID_42);
        // printk("%s , pre Bank 0x143B 0x42 = 0x%X\n", __func__, u16Temp);
        u16Temp &= ~(0X0C00);
        u16Temp |= (priv->rx_eq_rs1[2] << 10);
        OUTREG16((GHC_PHY_D + REG_ID_42), u16Temp);
        u16Temp = INREG16(GHC_PHY_D + REG_ID_42);
        // printk("%s , after Bank 0x143B 0x42 = 0x%X\n", __func__, u16Temp);
    }

    if (priv->rx_eq_rs2[0] != 0xFF && priv->rx_eq_rs2[0] < 4)
    { // RS2 Gen1  //*** Bank 0x143B h0034 , bit13~12

        u16Temp = INREG16(GHC_PHY_D + REG_ID_34);
        // printk("%s , pre Bank 0x143B 0x34 = 0x%X\n", __func__, u16Temp);
        u16Temp &= ~(0X3000);
        u16Temp |= (priv->rx_eq_rs2[0] << 12);
        OUTREG16((GHC_PHY_D + REG_ID_34), u16Temp);
        u16Temp = INREG16(GHC_PHY_D + REG_ID_34);
        // printk("%s , after Bank 0x143B 0x34 = 0x%X\n", __func__, u16Temp);
    }

    if (priv->rx_eq_rs2[1] != 0xFF && priv->rx_eq_rs2[1] < 4)
    { // RS2 Gen2  //*** Bank 0x143B h0042 , bit13~12

        u16Temp = INREG16(GHC_PHY_D + REG_ID_42);
        // printk("%s , pre Bank 0x143B 0x42 = 0x%X\n", __func__, u16Temp);
        u16Temp &= ~(0X3000);
        u16Temp |= (priv->rx_eq_rs2[1] << 12);
        OUTREG16((GHC_PHY_D + REG_ID_42), u16Temp);
        u16Temp = INREG16(GHC_PHY_D + REG_ID_42);
        // printk("%s , after Bank 0x143B 0x42 = 0x%X\n", __func__, u16Temp);
    }

    if (priv->rx_eq_rs2[2] != 0xFF && priv->rx_eq_rs2[2] < 4)
    { // RS2 Gen3  //*** Bank 0x143B h0042 , bit15~14

        u16Temp = INREG16(GHC_PHY_D + REG_ID_42);
        // printk("%s , pre Bank 0x143B 0x42 = 0x%X\n", __func__, u16Temp);
        u16Temp &= ~(0XC000);
        u16Temp |= (priv->rx_eq_rs2[2] << 14);
        OUTREG16((GHC_PHY_D + REG_ID_42), u16Temp);
        u16Temp = INREG16(GHC_PHY_D + REG_ID_42);
        // printk("%s , after Bank 0x143B 0x42 = 0x%X\n", __func__, u16Temp);
    }
}
