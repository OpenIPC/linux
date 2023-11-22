/*
 * mhal_sata_host.c- Sigmastar
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
#include <linux/clk.h>
#include <linux/clk-provider.h>
#include <linux/version.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <registers.h>
#include <mhal_sata_host.h>
#include "ms_platform.h"

#if 0
#define sata_reg_write16(val, addr)                                     \
    {                                                                   \
        (*((volatile unsigned short *)(addr))) = (unsigned short)(val); \
    }
#define sata_reg_write8(val, addr)                                    \
    {                                                                 \
        (*((volatile unsigned char *)(addr))) = (unsigned char)(val); \
    }
#endif
#define GET_REG8_ADDR(x, y)  (x + (y)*2)
#define GET_REG16_ADDR(x, y) (x + (y)*4)
#define RIU_BASE_ADDR        0xFD000000

#define BASE_REG_OTP3_PA GET_BASE_ADDR_BY_BANK(BASE_REG_RIU_PA, 0x102300)

void _MHal_SATA_Trim_init(unsigned long port_base)
{
    u16 u16Temp   = 0;
    u16 u16Otp_48 = 0, u16Otp_49 = 0, u16Otp_4A = 0;
    int portId = 0;
    // unsigned long port_base = BASE_REG_SATA0_SNPS_PORT0_PA; //   0x1A3480<<1
    unsigned long GHC_PHY_D    = BASE_REG_SATA0_PHYD_PA;   // 0x143B00
    unsigned long GHC_PHY2_ANA = BASE_REG_SATA0_PHYA_2_PA; //  0x143D00

    if (port_base == IO_ADDRESS(BASE_REG_SATA1_SNPS_PORT0_PA))
    {
        GHC_PHY_D    = BASE_REG_SATA1_PHYD_PA;   // 0x143B00
        GHC_PHY2_ANA = BASE_REG_SATA1_PHYA_2_PA; //  0x143D00
        portId       = 1;
        // pr_err("%s , i am sata2\n", __func__);
    }

    u16Otp_48 = INREG16(BASE_REG_OTP3_PA + REG_ID_48);
    u16Otp_49 = INREG16(BASE_REG_OTP3_PA + REG_ID_49);
    u16Otp_4A = INREG16(BASE_REG_OTP3_PA + REG_ID_4A);

    printk("%s ,u16Otp_48 = 0x%X, u16Otp_49 = 0x%X , u16Otp_4A = 0x%X\n", __func__, u16Otp_48, u16Otp_49, u16Otp_4A);

    if (((port_base == IO_ADDRESS(BASE_REG_SATA0_SNPS_PORT0_PA)) && (u16Otp_49 & BIT0))
        || ((port_base == IO_ADDRESS(BASE_REG_SATA1_SNPS_PORT0_PA)) && (u16Otp_4A & BIT1)))
    {
        printk("SATA0_TX_R50 trim = 0x%04X\n", (INREG16(GHC_PHY_D + REG_ID_2A) >> 7) & 0x1F);
        printk("SATA0_RX_R50 trim = 0x%04X\n", (INREG16(GHC_PHY_D + REG_ID_3C) >> 4) & 0x1F);
        printk("SATA0_TX_R50_IBIAS trim = 0x%04X\n", (INREG16(GHC_PHY_D + REG_ID_50) >> 10) & 0x3F);
#if 0
        printk("SATA0_RXPLL_3G trim = 0x%04X\n", INREG16(GHC_PHY2_ANA + REG_ID_55) & 0x3FF);
        printk("SATA0_RXPLL_1P5G trim_56 = 0x%04X , trim_57 = 0x%04X \n", INREG16(GHC_PHY2_ANA + REG_ID_56) & 0x3FF,
               INREG16(GHC_PHY2_ANA + REG_ID_57) & 0x3FF);
        printk("SATA0_TXPLL trim_45 = 0x%04X , trim_46 = 0x%04X , trim_47 = 0x%04X\n",
               INREG16(GHC_PHY2_ANA + REG_ID_45) & 0x3FF, INREG16(GHC_PHY2_ANA + REG_ID_46) & 0x3FF,
               INREG16(GHC_PHY2_ANA + REG_ID_47) & 0x3FF);
#endif
    }
    else
    {
        pr_err("%s ,sata%d use default trim\n", __func__, portId);

        // SATA_TX_R50 trim
        u16Temp = INREG16(GHC_PHY_D + REG_ID_0B);
        u16Temp |= 0x0010;
        OUTREG16((GHC_PHY_D + REG_ID_0B), u16Temp);

        u16Temp = INREG16(GHC_PHY_D + REG_ID_2A);
        u16Temp &= ~(0x0F80);
        u16Temp |= (0x0010 << 7);
        OUTREG16((GHC_PHY_D + REG_ID_2A), u16Temp);
        printk("SATA_TX_R50 trim = 0x%04X\n", (INREG16(GHC_PHY_D + REG_ID_2A) >> 7) & 0x1F);

        // SATA_RX_R50 trim
        u16Temp = INREG16(GHC_PHY_D + REG_ID_0E);
        u16Temp |= 0x0004;
        OUTREG16((GHC_PHY_D + REG_ID_0E), u16Temp);

        u16Temp = INREG16(GHC_PHY_D + REG_ID_3C);
        u16Temp &= ~(0x01F0);
        u16Temp |= (0x0010 << 4);
        OUTREG16((GHC_PHY_D + REG_ID_3C), u16Temp);
        printk("SATA_RX_R50 trim = 0x%04X\n", (INREG16(GHC_PHY_D + REG_ID_3C) >> 4) & 0x1F);

        // SATA_TX_R50_IBIAS trim
        u16Temp = INREG16(GHC_PHY_D + REG_ID_46);
        u16Temp |= (0x01 << 13);
        OUTREG16((GHC_PHY_D + REG_ID_46), u16Temp);

        u16Temp = INREG16(GHC_PHY_D + REG_ID_50);
        u16Temp &= ~(0xFC00);
        OUTREG16((GHC_PHY_D + REG_ID_50), u16Temp);
        printk("SATA_TX_R50_IBIAS trim = 0x%04X\n", (INREG16(GHC_PHY_D + REG_ID_50) >> 10) & 0x3F);
#if 0
        printk("SATA_RXPLL_3G trim = 0x%04X\n", INREG16(GHC_PHY2_ANA + REG_ID_55) & 0x3FF);
        printk("SATA_RXPLL_1P5G trim_56 = 0x%04X , trim_57 = 0x%04X \n", INREG16(GHC_PHY2_ANA + REG_ID_56) & 0x3FF,
               INREG16(GHC_PHY2_ANA + REG_ID_57) & 0x3FF);
        printk("SATA_TXPLL trim_45 = 0x%04X , trim_46 = 0x%04X , trim_47 = 0x%04X\n",
               INREG16(GHC_PHY2_ANA + REG_ID_45) & 0x3FF, INREG16(GHC_PHY2_ANA + REG_ID_46) & 0x3FF,
               INREG16(GHC_PHY2_ANA + REG_ID_47) & 0x3FF);
#endif
    }
}

int _MHal_SATA_Power_On(int portId)
{
#ifdef CONFIG_CAM_CLK
    u32                  u32clknum = 0;
    u32                  SataClk;
    struct device_node * dev_node      = NULL;
    void *               pvSataclk     = NULL;
    u32                  SataParentCnt = 1;
    CAMCLK_Set_Attribute stSetCfg;
    CAMCLK_Get_Attribute stGetCfg;

    if (portId == 0)
        dev_node = of_find_compatible_node(NULL, NULL, SSTAR_SATA_DTS_NAME);
    if (portId == 1)
        dev_node = of_find_compatible_node(NULL, NULL, SSTAR_SATA1_DTS_NAME);

    // printk("%s , 6+0\n", __func__);
    if (!dev_node)
        return -ENODEV;

    if (of_find_property(dev_node, "camclk", &SataParentCnt))
    {
        SataParentCnt /= sizeof(int);
        // printk( "[%s] Number : %d\n", __func__, num_parents);
        if (SataParentCnt < 0)
        {
            printk("[%s] Fail to get parent count! Error Number : %d\n", __func__, SataParentCnt);
            return -ENODEV;
        }
        for (u32clknum = 0; u32clknum < SataParentCnt; u32clknum++)
        {
            SataClk = 0;
            of_property_read_u32_index(dev_node, "camclk", u32clknum, &(SataClk));
            if (!SataClk)
            {
                printk(KERN_DEBUG "[%s] Fail to get clk!\n", __func__);
            }
            else
            {
                CamClkRegister("Sata", SataClk, &(pvSataclk));
                CamClkAttrGet(pvSataclk, &stGetCfg);
                CAMCLK_SETPARENT(stSetCfg, stGetCfg.u32Parent[0]);
                CamClkAttrSet(pvSataclk, &stSetCfg);
                CamClkSetOnOff(pvSataclk, 1);
                CamClkUnregister(pvSataclk);
            }
        }
    }
    else
    {
        printk("[%s] W/O Camclk \n", __func__);
        return -ENODEV;
    }
    return 0;
#else
    struct device_node *dev_node = NULL;
    struct clk **       sata_clks;
    struct clk *        clk_parent;
    int                 num_parents, i;

#if CONFIG_OF

    if (portId == 0)
        dev_node = of_find_compatible_node(NULL, NULL, SSTAR_SATA_DTS_NAME);
    if (portId == 1)
        dev_node = of_find_compatible_node(NULL, NULL, SSTAR_SATA1_DTS_NAME);
    // printk("%s , 6+0\n", __func__);
    if (!dev_node)
        return -ENODEV;

    num_parents = of_clk_get_parent_count(dev_node);
    if (num_parents > 0)
    {
        sata_clks = kzalloc((sizeof(struct clk *) * num_parents), GFP_KERNEL);
        if (sata_clks == NULL)
        {
            printk("[SATA%d]kzalloc failed!\n", portId);
            return -ENOMEM;
        }

        // enable all clk
        for (i = 0; i < num_parents; i++)
        {
            sata_clks[i] = of_clk_get(dev_node, i);
            if (IS_ERR(sata_clks[i]))
            {
                printk("Fail to get SATA%d clk!\n", portId);
                clk_put(sata_clks[i]);
                kfree(sata_clks);
                return -ENXIO;
            }

            /* Get parent clock */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 7, 0)
            clk_parent = clk_hw_get_parent_by_index(__clk_get_hw(sata_clks[i]), 0)->clk;
#else
            clk_parent = clk_get_parent_by_index(sata_clks[i], 0);
#endif
            if (IS_ERR(clk_parent))
            {
                printk("[SATA%d]can't get parent clock\n", portId);
                clk_put(sata_clks[i]);
                kfree(sata_clks);
                return -ENXIO;
            }
            /* Set clock parent */
            clk_set_parent(sata_clks[i], clk_parent);
            clk_prepare_enable(sata_clks[i]);
            clk_put(sata_clks[i]);
        }
        kfree(sata_clks);
    }

    // printk("%s , 6+1\n", __func__);
    return 0;
#else
    return -1;
#endif
#endif
}

// void MHal_SATA_Clock_Config(u32 misc_base, u32 port_base, bool enable)
void MHal_SATA_Clock_Config(unsigned long misc_base, unsigned long port_base, bool enable)
{
    u16           u16Temp;
    unsigned long MISC_PA = BASE_REG_SATA0_MISC_PA;
    int           portId  = 0;

    // Enable MAC Clock, bank is chip dependent
    // [0] phy clk gated
    // [1] phy clk invert
    // [3:2] 0: 108M
    // [8] clk gated
    // [9] clk invert
    // [11:10] 0: 432M
    // printk("%s , line = %d , enable = %d \n", __func__, __LINE__, enable);

    if (misc_base == IO_ADDRESS(BASE_REG_SATA1_MISC_PA))
    {
        MISC_PA = BASE_REG_SATA1_MISC_PA;
        portId  = 1;
        // pr_err("%s , i am sata2\n", __func__);
    }

    if (enable)
    {
        //*** Bank 0x143A h0000 => clear bit 12
        u16Temp = INREG16(MISC_PA + REG_ID_00);
        u16Temp &= ~(0x1000); // reg_sata_swrst_all
        OUTREG16((MISC_PA + REG_ID_00), u16Temp);

        if (_MHal_SATA_Power_On(portId))
        {
            printk("[SATA%d] Can't control clock with dtb, set registers directly!\n", portId);
            if (portId == 0)
            {
                OUTREG16(BASE_REG_CLKGEN_PA + REG_ID_6E, 0x0000); //*** Bank 0x1038 h006E => 0x0000
                OUTREG16(BASE_REG_CLKGEN_PA + REG_ID_6C, 0x0000); //*** Bank 0x1038 h006C => 0x0000
                OUTREG16(BASE_REG_CLKGEN_PA + REG_ID_46, 0x0000); //*** Bank 0x1038 h0046 => 0x0000
            }

            if (portId == 1)
            {
                // Turn On SATA2 clock
                OUTREG16(BASE_REG_SATA2_CLKGEN_PA + REG_ID_52, 0x0000); //*** Bank 0x103F h0052 => 0x0000
                OUTREG16(BASE_REG_SATA2_CLKGEN_PA + REG_ID_53, 0x0000); //*** Bank 0x103F h0053 => 0x0000
                OUTREG16(BASE_REG_SATA2_CLKGEN_PA + REG_ID_54, 0x0000); //*** Bank 0x103F h0054 => 0x0000
                OUTREG16(BASE_REG_SATA2_CLKGEN_PA + REG_ID_55, 0x0000); //*** Bank 0x103F h0055 => 0x0000
                OUTREG16(BASE_REG_SATA2_CLKGEN_PA + REG_ID_56, 0x0000); //*** Bank 0x103F h0056 => 0x0000
                // pr_err("%s , i am sata2\n", __func__);
            }
        }

        /*This function must run after "Bank 0x143A h0000 => clear bit 12" and clock enable  */
        _MHal_SATA_Trim_init(port_base);
    }
    else
    {
        u16Temp = INREG16(MISC_PA + REG_ID_00);
        u16Temp |= 0x1000; // reg_sata_swrst_all
        OUTREG16((MISC_PA + REG_ID_00), u16Temp);
    }

    printk("[SATA%d] Clock : %s\n", portId, (enable ? "ON" : "OFF"));
}

u32 MHal_SATA_get_max_speed(void)
{
    return E_PORT_SPEED_GEN3;
}
// EXPORT_SYMBOL_GPL(MHal_SATA_get_max_speed);

// void MHal_SATA_Setup_Port_Implement(u32 misc_base, u32 port_base, u32 hba_base)
void MHal_SATA_Setup_Port_Implement(unsigned long misc_base, unsigned long port_base, unsigned long hba_base)
{
//    void *mmio = priv->res_ahci_port0-0x100 ;
#if 0
    pr_err("%s , 6+0\n" , __func__);
//    writew(0x0000, (volatile void *)SATA_REG_ADDR(SSTAR_MAIL_BOX, REG_ID_00));
//    writew(0x0000, (volatile void *)SSTAR_RIU_BASE + (SSTAR_MAIL_BOX << 1));
    OUTREG16((BASE_REG_MAILBOX_PA+REG_ID_00), 0x0000);

    udelay(100);
//    writew(0x1111, (volatile void *)SATA_REG_ADDR(SSTAR_MAIL_BOX, REG_ID_00));
//    writew(0x1111, (volatile void *)SSTAR_RIU_BASE + (SSTAR_MAIL_BOX << 1));
    OUTREG16((BASE_REG_MAILBOX_PA+REG_ID_00), 0x1111);
#endif

    //  Init FW to trigger controller
    writel(0x00000000, (volatile void *)(hba_base) + (SS_HOST_CAP));

    // Port Implement
    writel(0x00000001, (volatile void *)(hba_base) + (SS_HOST_PORTS_IMPL));
    writel(0x00000000, (volatile void *)(port_base) + (SS_PORT_CMD));
}
// EXPORT_SYMBOL_GPL(MHal_SATA_Setup_Port_Implement);

#ifndef CONFIG_SS_SATA_AHCI_PLATFORM_HOST
void MHal_SATA_HW_Inital(u32 misc_base, u32 port_base, u32 hba_base)
{
    // u16 u16Temp;
    u32 GHC_PHY_ANA = 0x0, u32Temp;
    int phy_mode;
    // printk( "%s , 6+%d\n" , __func__ , __LINE__);

    if (port_base == BASE_REG_SATA0_SNPS_PORT0_PA)
    {
        GHC_PHY_ANA = SATA_GHC_0_PHY_ANA; // 0x143C00
    }

    MHal_SATA_Clock_Config(misc_base, port_base, 1);
    // u16Temp = readw((volatile void *)SSTAR_RIU_BASE + (REG_CKG_SATA_FCLK << 1));
    // printk("SATA_CLK_REG = 0x%x\n", u16Temp);

    printk("RIU_BASE = 0x%x, GHC_PHY_ANA = 0x%x\n", SSTAR_RIU_BASE, GHC_PHY_ANA);
    // RIU_BASE = xfd000000, misc_base : xfd287400 , hba_base : xfd346800 , port_base : xfd346900 , GHC_PHY : 143B00

    // printk("check sata speed !!!!!!\n");
    u32Temp = readl((volatile void *)port_base + 0x2c);
    u32Temp = u32Temp & (~E_PORT_SPEED_MASK);
    u32Temp = u32Temp | MHal_SATA_get_max_speed();
    writel(u32Temp, (volatile void *)port_base + 0x2c);
    u32Temp = readl((volatile void *)port_base + 0x2c);
    printk("MAC SATA SPEED= 0x%x\n", u32Temp);

    if ((u32Temp & E_PORT_SPEED_MASK) == E_PORT_SPEED_GEN1)
        phy_mode = 0;
    else if ((u32Temp & E_PORT_SPEED_MASK) == E_PORT_SPEED_GEN2)
        phy_mode = 1;
    else
        phy_mode = 2;

    // printk("sata hal PHY init !!!!!!\n");
    //*** Bank 0x143C h0005 => 0x00:Gen1, 0x01:Gen2, 0x10:Gen3
    switch (phy_mode)
    {
        case 0:
            writew(0x00, (volatile void *)GHC_PHY_ANA + (0x05 << 1));
            break;
        case 1:
            writew(0x01, (volatile void *)GHC_PHY_ANA + (0x05 << 1));
            break;
        case 2:
        default:
            writew(0x10, (volatile void *)GHC_PHY_ANA + (0x05 << 1));
            break;
    }

    writel(0x00000001, (volatile void *)hba_base + SS_HOST_CTL); // reset HBA
}
// EXPORT_SYMBOL_GPL(MHal_SATA_HW_Inital);

#endif
