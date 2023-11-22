/*
 * mdrv_sata_common.c- Sigmastar
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

#if 0
int ahci_port_phy_fsm_reset(struct ata_link *link) // prototype in mdrv_sata_host_ahci.h
{
    u16                     u16Temp = 0;
    struct ata_host *       host    = link->ap->host;
    struct ahci_host_priv * hpriv   = host->private_data;
    struct sstar_ahci_priv *priv    = hpriv->plat_data;

    uintptr_t mmio = (uintptr_t)hpriv->mmio;

    if ((priv->phy_mode == 0) || (priv->phy_mode == 1))
    {
        // printk("%s , mmio = 0x%p \n", __func__ , priv->mmio  );

        //*** Bank 0x1527 h0050 bit15 => 0x0
        u16Temp = readw((volatile void *)(mmio - SSTAR_SATA_PHY_OFFSET + REG_SATA_OFFSET_PHYA_REG_50));
        u16Temp = u16Temp & (~(0x01 << 15));
        writew(u16Temp, (volatile void *)(mmio - SSTAR_SATA_PHY_OFFSET + REG_SATA_OFFSET_PHYA_REG_50));

        ndelay(1000);

        //*** Bank 0x1527 h0050 bit15 => 0x1
        u16Temp = readw((volatile void *)(mmio - SSTAR_SATA_PHY_OFFSET + REG_SATA_OFFSET_PHYA_REG_50));
        u16Temp = u16Temp | (0x01 << 15);
        writew(u16Temp, (volatile void *)(mmio - SSTAR_SATA_PHY_OFFSET + REG_SATA_OFFSET_PHYA_REG_50));
    }

    return 0;
}
EXPORT_SYMBOL(ahci_port_phy_fsm_reset);
#endif

int ahci_port_apply_hd_link(struct ata_link *link)
{
    struct ata_host *       host  = link->ap->host;
    struct ahci_host_priv * hpriv = host->private_data;
    struct sstar_ahci_priv *priv  = NULL;

    u32       sstatus, scontrol, tmp;
    const int count = 5;
    int       i     = 0;

    if (hpriv->plat_data != NULL)
    {
        priv = hpriv->plat_data;
    }
    else
    {
        // pr_err("%s , i am pcie\n", __func__);
        return 0;
    }

    sata_scr_read(link, SCR_CONTROL, &scontrol);
    scontrol = (scontrol & 0x0f0) | 0x300;
    ata_msleep(link->ap, 1);
    sata_scr_write_flush(link, SCR_CONTROL, scontrol);

    ata_msleep(link->ap, 2);
    if (sata_scr_read(link, SCR_STATUS, &sstatus))
        return -1;

    tmp = (sstatus >> 4) & 0xf;
    // printk("%s , sstatus = 0x%X , hd_link = 0x%X \n", __func__, sstatus, priv->hd_link);

    if (tmp == priv->hd_link)
        return 0;

    for (i = 0; i < count; i++)
    {
        /* issue phy wake/reset */
        sata_scr_read(link, SCR_CONTROL, &scontrol);
        scontrol = (scontrol & 0x0f0) | 0x301;
        sata_scr_write_flush(link, SCR_CONTROL, scontrol);

        ata_msleep(link->ap, 1);

        scontrol = (scontrol & 0x0f0) | 0x300;
        sata_scr_write(link, SCR_CONTROL, scontrol);

        ata_msleep(link->ap, 2);
        sata_scr_read(link, SCR_STATUS, &sstatus);
        tmp = (sstatus >> 4) & 0xf;
        if (tmp == priv->hd_link)
        {
            printk("%s ,retry = %d success \n", __func__, i);
            return 0;
        }
    }

    printk("%s ,retry = %d fail \n", __func__, i);

    return -1;
}
EXPORT_SYMBOL(ahci_port_apply_hd_link);

void ss_sata_ssc_enable(struct ata_link *link, bool enable, unsigned int spd)
{
    struct ata_host *       host  = link->ap->host;
    struct ahci_host_priv * hpriv = host->private_data;
    struct sstar_ahci_priv *priv  = NULL;
    void __iomem *          mmio  = NULL;

    unsigned long GHC_PHY_ANA = BASE_REG_SATA0_PHYA_PA; //  0x143C00

    if (hpriv->plat_data != NULL)
    {
        priv = hpriv->plat_data;
        mmio = priv->mmio;
    }
    else
    {
        // pr_err("%s , i am pcie\n", __func__);
        return;
    }

    if (priv->ssc_mode != 1)
        return;

    // pr_err("%s, enable = %d\n", __func__, enable);

    if ((unsigned long)mmio == IO_ADDRESS(BASE_REG_SATA1_SNPS_PA))
    {
        GHC_PHY_ANA = BASE_REG_SATA1_PHYA_PA; //  0x144300
        // pr_err("%s , i am sata2\n", __func__);
    }

    if (enable)
    {
        //*** Bank 0x143C h0044 => 0x0000
        OUTREG16((GHC_PHY_ANA + REG_ID_44), 0x0000);
        udelay(10);

        if ((spd == 0) || (spd == 1))
        {
            //*** Bank 0x143C h0040 => 0x1906
            OUTREG16((GHC_PHY_ANA + REG_ID_40), 0x1906);
        }
        else
        {
            //*** Bank 0x143C h0040 => 0x1800
            OUTREG16((GHC_PHY_ANA + REG_ID_40), 0x1800);
        }

        udelay(10);

        //*** Bank 0x143C h0041 => 0x0017
        OUTREG16((GHC_PHY_ANA + REG_ID_41), 0x0017);
        udelay(10);

        //*** Bank 0x143C h0044 => 0x0001
        OUTREG16((GHC_PHY_ANA + REG_ID_44), 0x0001);
        udelay(10);

        //*** Bank 0x143C h0043 => 0x04C2
        OUTREG16((GHC_PHY_ANA + REG_ID_43), 0x04C2);
        udelay(10);

        //*** Bank 0x143C h0044 => 0x0110
        OUTREG16((GHC_PHY_ANA + REG_ID_44), 0x0110);
        udelay(10);

        //*** Bank 0x143C h0042 => 0x0002
        OUTREG16((GHC_PHY_ANA + REG_ID_42), 0x0002);
        udelay(10);
    }
    else
    {
        //*** Bank 0x143C h0044 => 0x0000
        OUTREG16((GHC_PHY_ANA + REG_ID_44), 0x0000);
        udelay(10);

        //*** Bank 0x143C h0040 => 0x0A3D
        OUTREG16((GHC_PHY_ANA + REG_ID_40), 0x0A3D);
        udelay(10);

        //*** Bank 0x143C h0041 => 0x0017
        OUTREG16((GHC_PHY_ANA + REG_ID_41), 0x0017);
        udelay(10);

        //*** Bank 0x143C h0044 => 0x0001
        OUTREG16((GHC_PHY_ANA + REG_ID_44), 0x0001);
        udelay(10);

        //*** Bank 0x143C h0043 => 0x04C2
        OUTREG16((GHC_PHY_ANA + REG_ID_43), 0x04C2);
        udelay(10);

        //*** Bank 0x143C h0044 => 0x0100
        OUTREG16((GHC_PHY_ANA + REG_ID_44), 0x0100);
        udelay(10);

        //*** Bank 0x143C h0042 => 0x0000
        OUTREG16((GHC_PHY_ANA + REG_ID_42), 0x0000);
        udelay(10);
    }
    udelay(100);
}
EXPORT_SYMBOL_GPL(ss_sata_ssc_enable);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("sstar Semiconductor");
MODULE_DESCRIPTION("SSTAR AHCI SATA COMMON FUNTION");
