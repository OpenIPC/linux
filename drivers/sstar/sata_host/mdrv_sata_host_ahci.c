/*
* mdrv_sata_host_ahci.h - Sigmastar
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
#include <linux/delay.h>
#include <linux/spinlock.h>
#include <linux/module.h>
#include <linux/io.h>
#include <linux/pm.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/ahci_platform.h>
#include <irqs.h>
#include "ahci.h"
#include "mhal_sata_host.h"
#include "mdrv_sata_host_ahci.h"
#include "mhal_sata_host_ahci.h"
//#include "mhal_sata_host_ahci.c"

#define SW_OOB_MODE 0

static int n_ports = 1;//config sata ports //TBD
static int phy_mode = 2;//config sata mode //TBD

#if 0
    #ifdef MODULE//TBD
        module_param(phy_config, uint, 0600);
        MODULE_PARM_DESC(phy_config, "sata phy config");//(default:0x0e180000)
        module_param(n_ports, uint, 0600);
        MODULE_PARM_DESC(n_ports, "sata port number");//(default:2)
        module_param(mode_3g, uint, 0600);
        MODULE_PARM_DESC(phy_mode, "sata phy mode ");//(0:1.5G;1:3G(default);2:6G)
    #endif
#endif
module_param(phy_mode, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(phy_mode, "\nSATA Max Speed:\n"
                 " [0] 1.5 Gbps\n"
                 " [1] 3.0 Gbps(default)\n"
                 " [2] 6.0 Gbps");

extern int ahci_scr_read(struct ata_link *link, unsigned int sc_reg, u32 *val);
extern int ahci_scr_write(struct ata_link *link, unsigned int sc_reg, u32 val);
extern unsigned ahci_scr_offset(struct ata_port *ap, unsigned int sc_reg);


static u32 sstar_sata_wait_reg(phys_addr_t reg_addr, u32 mask, u32 val, unsigned long interval, unsigned long timeout)
{
    u32 temp;
    unsigned long timeout_vale = 0;

    temp = readl((void *)reg_addr);

    while((temp & mask) == val)
    {
        msleep(interval);
        timeout_vale += interval;
        if (timeout_vale > timeout)
            break;
        temp = readl((void *)reg_addr);
    }
    return temp;
}

#if (SW_OOB_MODE == 1)
static void sstar_sata_sw_oob_mode1(void)
{
    u16 u16Temp, u16IrqRetry, u16Irq2Retry=2;

    // Enable all SATA interrupt
    writew(0x0000, (volatile void *)SSTAR_RIU_BASE + (0x15273A << 1));

SW_OOB_MODE1_STAGE0:
    // Clear all SATA interrupt
    writew(0xFFFF, (volatile void *)SSTAR_RIU_BASE + (0x15273C << 1));
    writew(0x0000, (volatile void *)SSTAR_RIU_BASE + (0x15273C << 1));

    // Force reg_sata_rxpll_pd_cdr = 1
    u16Temp = readw((volatile void *)SSTAR_RIU_BASE + (0x152762 << 1));
    u16Temp |= 0x400; // [10]: reg_sata_rxpll_pd_cdr
    writew(u16Temp, (volatile void *)SSTAR_RIU_BASE + (0x152762 << 1));

    udelay(22);

    for (u16IrqRetry = 0; u16IrqRetry < 5; u16IrqRetry++)
    {
        // Check if IRQ1 is yes
        if (readw((volatile void *)SSTAR_RIU_BASE + (0x15273E << 1)) & INT_SATA_PHY_RXPLL_FREQ_LOCK_FLAG)
            break;

        udelay(2);
    }

    // Force reg_sata_rxpll_pd_cdr = 0
    u16Temp = readw((volatile void *)SSTAR_RIU_BASE + (0x152762 << 1));
    u16Temp &= ~(0x400); // [10]: reg_sata_rxpll_pd_cdr
    writew(u16Temp, (volatile void *)SSTAR_RIU_BASE + (0x152762 << 1));

    udelay(3);

    u16Irq2Retry--;
    // Check if IRQ2 no
    if ((readw((volatile void *)SSTAR_RIU_BASE + (0x15273E << 1)) & INT_SATA_PHY_RXPLL_FREQ_UNLOCK_FLAG) || u16Irq2Retry == 0)
        goto SW_OOB_MODE1_STAGE0;

    udelay(2);

    for (u16IrqRetry = 0; u16IrqRetry < 2; u16IrqRetry++)
    {
        // Check if IRQ3 is yes
        if (readw((volatile void *)SSTAR_RIU_BASE + (0x15273E << 1)) & INT_SATA_PHY_RX_DATA_VLD_PRE_0)
            break;

        udelay(1);
    }

    // Data Ready

    // Disable all SATA interrupt
    writew(0xFFFF, (volatile void *)SSTAR_RIU_BASE + (0x15273A << 1));
}
#elif (SW_OOB_MODE == 2)
static void sstar_sata_sw_oob_mode2(void)
{
    u16 u16Temp, u16Irq3Retry=2;

    //Enable all SATA interrupt
    writew(0x0000, (volatile void *)SSTAR_RIU_BASE + (0x15273A << 1));

SW_OOB_MODE2_STAGE0:
    // Clear all SATA interrupt
    writew(0xFFFF, (volatile void *)SSTAR_RIU_BASE + (0x15273C << 1));
    writew(0x0000, (volatile void *)SSTAR_RIU_BASE + (0x15273C << 1));

    // Force reg_sata_rxpll_pd_cdr = 1
    u16Temp = readw((volatile void *)SSTAR_RIU_BASE + (0x152762 << 1));
    u16Temp |= 0x400; // [10]: reg_sata_rxpll_pd_cdr
    writew(u16Temp, (volatile void *)SSTAR_RIU_BASE + (0x152762 << 1));

    udelay(22);

    // Force reg_sata_rxpll_pd_cdr = 0
    u16Temp = readw((volatile void *)SSTAR_RIU_BASE + (0x152762 << 1));
    u16Temp &= ~(0x400); // [10]: reg_sata_rxpll_pd_cdr
    writew(u16Temp, (volatile void *)SSTAR_RIU_BASE + (0x152762 << 1));

    udelay(3);

    u16Irq3Retry--;
    // Check if IRQ3 is yes
    if ((readw((volatile void *)SSTAR_RIU_BASE + (0x15273E << 1)) & INT_SATA_PHY_RX_DATA_VLD_PRE_0) || u16Irq3Retry == 0)
        break;

    // Data Ready

    // Disable all SATA interrupt
    writew(0xFFFF, (volatile void *)SSTAR_RIU_BASE + (0x15273A << 1));
}
#endif

int ss_sata_init(struct device *dev, void __iomem *mmio)
{
    u32 i;
    u32 u32Temp = 0;
    phys_addr_t hba_base = (phys_addr_t)mmio; //1A2800<<1
    phys_addr_t port_base = (phys_addr_t)(mmio + 0x80); //1A2840<<1
    phys_addr_t misc_base = (phys_addr_t)(mmio - 0xA0600); //152500<<1
    int port_num;

    port_num = n_ports;

    //printk("sstar sata HW settings start!!!\n");
    ss_sata_misc_init(mmio, phy_mode, port_num);
    ss_sata_phy_init(mmio, phy_mode, port_num);

    writew(HOST_RESET, (volatile void *)mmio + (HOST_CTL));

    #if (SW_OOB_MODE == 1)
    sstar_sata_sw_oob_mode1();
    #elif (SW_OOB_MODE == 2)
    sstar_sata_sw_oob_mode2();
    #else
    u32Temp = sstar_sata_wait_reg(HOST_CTL + (phys_addr_t)mmio, HOST_RESET, HOST_RESET, 1, 500);
    if (u32Temp & HOST_RESET)
        return -1;
    #endif

    // Turn on AHCI_EN
    u32Temp = readl((void *)HOST_CTL + (phys_addr_t)hba_base);
    if (u32Temp & HOST_AHCI_EN)
    {
        MHal_SATA_Setup_Port_Implement((phys_addr_t)misc_base, (phys_addr_t)port_base, (phys_addr_t)hba_base);
        return 0;
    }

    // Try AHCI_EN Trurn on for a few time
    for (i = 0; i < 5; i++)
    {
        u32Temp |= HOST_AHCI_EN;
        writel(u32Temp, (void *)HOST_CTL + (phys_addr_t)hba_base);
        u32Temp = readl((void *)HOST_CTL + (phys_addr_t)hba_base);
        if (u32Temp & HOST_AHCI_EN)
            break;
        msleep(10);
    }

    MHal_SATA_Setup_Port_Implement((phys_addr_t)misc_base, (phys_addr_t)port_base, (phys_addr_t)hba_base);

    printk("sstar sata HW settings done!!!\n");
    return 0;
}
//EXPORT_SYMBOL(ss_sata_init);

void ss_sata_exit(struct device *dev)
{
    struct ata_host *host = dev_get_drvdata(dev);
    struct ahci_host_priv *hpriv = host->private_data;
    phys_addr_t port_base = (phys_addr_t)(hpriv->mmio + 0x80);   //(0x1A2840 << 1)
    phys_addr_t misc_base = (phys_addr_t)(hpriv->mmio - 0xA0600);//(0x152500 << 1)

    pr_info("[%s] port_base = 0x%x\n", __func__, port_base);
    MHal_SATA_Clock_Config(misc_base, port_base, FALSE);
}
//EXPORT_SYMBOL(ss_sata_exit);


static int ss_sata_suspend(struct device *dev)
{
    pr_info("[%s]\n", __func__);
    ss_sata_exit(dev);

    return 0;
}

static int ss_sata_resume(struct device *dev)
{
    struct ata_host *host = dev_get_drvdata(dev);
    struct ahci_host_priv *hpriv = host->private_data;

    pr_info("[%s]\n", __func__);
    ss_sata_init(dev, hpriv->mmio);

    return 0;
}

struct ahci_platform_data ss_ahci_platdata =
{
    .init    = ss_sata_init,
    .exit    = ss_sata_exit,
    .suspend = ss_sata_suspend,
    .resume  = ss_sata_resume,
};

static struct resource ss_sata_ahci_resources[] =
{
    [0] = {
        .start = SATA_GHC_0_ADDRESS_START,
        .end   = SATA_GHC_0_ADDRESS_END,
        .flags = IORESOURCE_MEM,
    },
    [1] = {
        .start = SATA_GHC_0_P0_ADDRESS_START,
        .end   = SATA_GHC_0_P0_ADDRESS_END,
        .flags = IORESOURCE_MEM,
    },
    [2] = {
        .start = SATA_MISC_0_ADDRESS_START,
        .end   = SATA_MISC_0_ADDRESS_END,
        .flags = IORESOURCE_MEM,
    },
    [3] = {
        .start = INT_IRQ_SATA,
        .end   = INT_IRQ_SATA,
        .flags = IORESOURCE_IRQ,
    },
};

#if defined(CONFIG_ARM64)
    static u64 ahci_dmamask = ~(u64)0;
#else
    static u64 ahci_dmamask = ~(u32)0;
#endif

static void ss_satav100_ahci_platdev_release(struct device *dev)
{
    struct ata_host *host = dev_get_drvdata(dev);
    struct ahci_host_priv *hpriv = host->private_data;
    phys_addr_t port_base = (phys_addr_t)(hpriv->mmio + 0x80); //SATA_GHC_0_P0_ADDRESS_START
    phys_addr_t misc_base = (phys_addr_t)(hpriv->mmio - 0xA0600);//(0x152500 << 1)

    pr_info("[%s] port_base = 0x%x\n", __func__, port_base);
    MHal_SATA_Clock_Config(misc_base, port_base, FALSE);
    return;
}

static struct platform_device ss_sata_ahci_device =
{
    .name           = "ahci",
    .id = 0,
    .dev = {
        .platform_data          = &ss_ahci_platdata,
        .dma_mask               = &ahci_dmamask,
#if defined(CONFIG_ARM64)
        .coherent_dma_mask = 0xffffffffffffffff,
#else
        .coherent_dma_mask = 0xffffffff,
#endif
        .release                = ss_satav100_ahci_platdev_release,
    },
    .num_resources  = ARRAY_SIZE(ss_sata_ahci_resources),
    .resource       = ss_sata_ahci_resources,
};

static int __init ss_ahci_init(void)
{
    int ret = 0;

    pr_info("[%s]\n", __func__);

    ret = platform_device_register(&ss_sata_ahci_device);
    if (ret)
    {
        pr_err("[%s %d] sstar sata platform device register is failed!!!\n",
               __func__, __LINE__);
        return ret;
    }

    return ret;
}

static void __exit ss_ahci_exit(void)
{
    pr_info("[%s]\n", __func__);

    platform_device_unregister(&ss_sata_ahci_device);
    return;
}
module_init(ss_ahci_init);
module_exit(ss_ahci_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("sstar Semiconductor");
MODULE_DESCRIPTION("sstar SATA controller low level driver");
MODULE_VERSION("1.00");
