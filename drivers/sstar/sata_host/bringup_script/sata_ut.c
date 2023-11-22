/*
 * sata_ut.c- Sigmastar
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

//==============================================================================
//
//                              INCLUDE FILES
//
//==============================================================================

#include <linux/pfn.h>
#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/kthread.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/vmalloc.h> /* seems do not need this */
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/time.h>
#include <linux/module.h>
#include <asm/uaccess.h>
#include <linux/fs.h>
#include <asm/io.h>
#include <asm/string.h>
#include <linux/cdev.h>
#include <linux/platform_device.h>
#include <linux/poll.h>
#include <linux/irqreturn.h>
#include <linux/interrupt.h>
#include <linux/file.h>
#include <ms_platform.h>

//#include "mhal_common.h"
//#include "mhal_miu.h"
//#include "mdrv_miu.h"
//#include "mhal_cmdq.h"
//#include "ms_msys.h"
//#include "cam_os_wrapper.h"

//#include "ms_platform.h"
#include "ahci.h"

#include "registers.h"

//#include "mhal_sata_host.h"
//#include "mhal_sata_host_ahci.h"
//#include "mdrv_sata_host_ahci.h"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("SIGMASTAR");
MODULE_DESCRIPTION("SATA UT driver");

//==============================================================================
//
//                              MACRO DEFINE
//
//==============================================================================
#define SOC_01 0
#define SOC_02 1

//==============================================================================
//
//                              STRUCTURE
//
//==============================================================================

//==============================================================================
//
//                              GLOBAL VARIABLES
//
//==============================================================================

static unsigned int port = 0;
module_param(port, uint, 0);

static unsigned int phy_mode = 0;
module_param(phy_mode, uint, 0);

//==============================================================================
//
//                              FUNCTIONS
//
//==============================================================================

//#define ARCH_MERCURY6

//#define ARCH_INFINITY7
#define ARCH_MERCURY6P

#ifdef ARCH_MERCURY6
#define MAX_SATA_PORT 1
#else
#define MAX_SATA_PORT 2 // i7 / m6p
#endif

#define SATA_GEN1    0
#define SATA_GEN2    1
#define SATA_GEN3    2
#define MAX_SATA_GEN SATA_GEN3

#ifdef ARCH_MERCURY6
#define BASE_REG_SATA0_SNPS_PA_UT       GET_BASE_ADDR_BY_BANK(BASE_REG_RIU_PA, 0x1A3400)
#define BASE_REG_SATA0_SNPS_PORT0_PA_UT GET_BASE_ADDR_BY_BANK(BASE_REG_RIU_PA, 0x1A3480)
#define BASE_REG_SATA0_MISC_PA_UT       GET_BASE_ADDR_BY_BANK(BASE_REG_RIU_PA, 0x143A00)
#define BASE_REG_SATA0_PHYD_PA_UT       GET_BASE_ADDR_BY_BANK(BASE_REG_RIU_PA, 0x143B00)
#define BASE_REG_SATA0_PHYA_PA_UT       GET_BASE_ADDR_BY_BANK(BASE_REG_RIU_PA, 0x143C00)
#define BASE_REG_SATA0_GP_CTRL_PA_UT    GET_BASE_ADDR_BY_BANK(BASE_REG_RIU_PA, 0x144500)

#define BASE_REG_SATA1_SNPS_PA_UT       GET_BASE_ADDR_BY_BANK(BASE_REG_RIU_PA, 0x1A3500)
#define BASE_REG_SATA1_SNPS_PORT0_PA_UT GET_BASE_ADDR_BY_BANK(BASE_REG_RIU_PA, 0x1A3580)
#define BASE_REG_SATA1_MISC_PA_UT       GET_BASE_ADDR_BY_BANK(BASE_REG_RIU_PA, 0x144100)
#define BASE_REG_SATA1_PHYD_PA_UT       GET_BASE_ADDR_BY_BANK(BASE_REG_RIU_PA, 0x144200)
#define BASE_REG_SATA1_PHYA_PA_UT       GET_BASE_ADDR_BY_BANK(BASE_REG_RIU_PA, 0x144300)
#define BASE_REG_SATA1_GP_CTRL_PA_UT    GET_BASE_ADDR_BY_BANK(BASE_REG_RIU_PA, 0x144700)

#elif defined(ARCH_INFINITY7)
#define BASE_REG_SATA0_SNPS_PA_UT       GET_BASE_ADDR_BY_BANK(BASE_REG_RIU_PA, 0x1A3400)
#define BASE_REG_SATA0_SNPS_PORT0_PA_UT GET_BASE_ADDR_BY_BANK(BASE_REG_RIU_PA, 0x1A3480)
#define BASE_REG_SATA0_MISC_PA_UT       GET_BASE_ADDR_BY_BANK(BASE_REG_RIU_PA, 0x143A00)
#define BASE_REG_SATA0_PHYD_PA_UT       GET_BASE_ADDR_BY_BANK(BASE_REG_RIU_PA, 0x143B00)
#define BASE_REG_SATA0_PHYA_PA_UT       GET_BASE_ADDR_BY_BANK(BASE_REG_RIU_PA, 0x143C00)
#define BASE_REG_SATA0_PHYA_2_PA_UT     GET_BASE_ADDR_BY_BANK(BASE_REG_RIU_PA, 0x143D00)
#define BASE_REG_SATA0_GP_CTRL_PA_UT    GET_BASE_ADDR_BY_BANK(BASE_REG_RIU_PA, 0x144500)

#define BASE_REG_SATA1_SNPS_PA_UT       GET_BASE_ADDR_BY_BANK(BASE_REG_RIU_PA, 0x1A3500)
#define BASE_REG_SATA1_SNPS_PORT0_PA_UT GET_BASE_ADDR_BY_BANK(BASE_REG_RIU_PA, 0x1A3580)
#define BASE_REG_SATA1_MISC_PA_UT       GET_BASE_ADDR_BY_BANK(BASE_REG_RIU_PA, 0x144100)
#define BASE_REG_SATA1_PHYD_PA_UT       GET_BASE_ADDR_BY_BANK(BASE_REG_RIU_PA, 0x144200)
#define BASE_REG_SATA1_PHYA_PA_UT       GET_BASE_ADDR_BY_BANK(BASE_REG_RIU_PA, 0x144300)
#define BASE_REG_SATA1_PHYA_2_PA_UT     GET_BASE_ADDR_BY_BANK(BASE_REG_RIU_PA, 0x143400)
#define BASE_REG_SATA1_GP_CTRL_PA_UT    GET_BASE_ADDR_BY_BANK(BASE_REG_RIU_PA, 0x144700)

#elif defined(ARCH_MERCURY6P)
#define BASE_REG_SATA0_SNPS_PA_UT       GET_BASE_ADDR_BY_BANK(BASE_REG_RIU_PA, 0x1A3400)
#define BASE_REG_SATA0_SNPS_PORT0_PA_UT GET_BASE_ADDR_BY_BANK(BASE_REG_RIU_PA, 0x1A3480)
#define BASE_REG_SATA0_MISC_PA_UT       GET_BASE_ADDR_BY_BANK(BASE_REG_RIU_PA, 0x143A00)
#define BASE_REG_SATA0_PHYD_PA_UT       GET_BASE_ADDR_BY_BANK(BASE_REG_RIU_PA, 0x143B00)
#define BASE_REG_SATA0_PHYA_PA_UT       GET_BASE_ADDR_BY_BANK(BASE_REG_RIU_PA, 0x143C00)
#define BASE_REG_SATA0_PHYA_2_PA_UT     GET_BASE_ADDR_BY_BANK(BASE_REG_RIU_PA, 0x143D00)
#define BASE_REG_SATA0_GP_CTRL_PA_UT    GET_BASE_ADDR_BY_BANK(BASE_REG_RIU_PA, 0x144500)

#define BASE_REG_SATA1_SNPS_PA_UT       GET_BASE_ADDR_BY_BANK(BASE_REG_RIU_PA, 0x1A3500)
#define BASE_REG_SATA1_SNPS_PORT0_PA_UT GET_BASE_ADDR_BY_BANK(BASE_REG_RIU_PA, 0x1A3580)
#define BASE_REG_SATA1_MISC_PA_UT       GET_BASE_ADDR_BY_BANK(BASE_REG_RIU_PA, 0x144100)
#define BASE_REG_SATA1_PHYD_PA_UT       GET_BASE_ADDR_BY_BANK(BASE_REG_RIU_PA, 0x144200)
#define BASE_REG_SATA1_PHYA_PA_UT       GET_BASE_ADDR_BY_BANK(BASE_REG_RIU_PA, 0x144300)
#define BASE_REG_SATA1_PHYA_2_PA_UT     GET_BASE_ADDR_BY_BANK(BASE_REG_RIU_PA, 0x144400)
#define BASE_REG_SATA1_GP_CTRL_PA_UT    GET_BASE_ADDR_BY_BANK(BASE_REG_RIU_PA, 0x144700)
#endif

void sata_bringup(unsigned int u32_sata_port, unsigned int u32_phy_mode)
{
    //    unsigned long port_base = (unsigned long)(BASE_REG_SATA0_SNPS_PA_UT + 0x100); //   0x1A3480<<1
    unsigned long port_base   = BASE_REG_SATA0_SNPS_PORT0_PA_UT; //   0x1A3480<<1
    unsigned long GHC_PHY_D   = BASE_REG_SATA0_PHYD_PA_UT;       // 0x143B00
    unsigned long GHC_PHY_ANA = BASE_REG_SATA0_PHYA_PA_UT;       //  0x143C00
    //    unsigned long GHC_PHY2_ANA = SATA_GHC_0_PHY_ANA2;  //  0x143D00
    unsigned long GHC_GP_CTRL = BASE_REG_SATA0_GP_CTRL_PA_UT; //  0x144500
    unsigned long MISC_PA     = BASE_REG_SATA0_MISC_PA_UT;
    // void  *mmio = ( void *)(IO_ADDRESS(BASE_REG_SATA0_SNPS_PA_UT));
    unsigned long mmio = BASE_REG_SATA0_SNPS_PA_UT;
    int           i = 0, reg_value = 0;

    if (u32_sata_port == 1)
    {
        // mmio = (void *)(IO_ADDRESS(BASE_REG_SATA1_SNPS_PA_UT));
        mmio        = BASE_REG_SATA1_SNPS_PA_UT;
        port_base   = BASE_REG_SATA1_SNPS_PORT0_PA_UT; //   0x1A3480<<1
        GHC_PHY_D   = BASE_REG_SATA1_PHYD_PA_UT;       // 0x144200
        GHC_PHY_ANA = BASE_REG_SATA1_PHYA_PA_UT;       //  0x144300
                                                       //        GHC_PHY2_ANA = SATA_GHC_1_PHY_ANA2;  //  0x144400
        GHC_GP_CTRL = BASE_REG_SATA1_GP_CTRL_PA_UT;    //  0x144700
        MISC_PA     = BASE_REG_SATA1_MISC_PA_UT;
        pr_err("%s , i am sata2\n", __func__);
    }

    pr_err("%s, sata_port = 0x%X , phy_mode = 0x%X \n", __func__, u32_sata_port, u32_phy_mode);

    pr_err("%s, mmio = 0x%lX\n", __func__, mmio);
    pr_err("%s, port_base = 0x%lX\n", __func__, port_base);
    pr_err("%s, MISC_PA = 0x%lX\n", __func__, MISC_PA);

    pr_err("%s, GHC_PHY_D = 0x%lX\n", __func__, GHC_PHY_D);
    pr_err("%s, GHC_PHY_ANA = 0x%lX\n", __func__, GHC_PHY_ANA);
    pr_err("%s, GHC_GP_CTRL = 0x%lX\n", __func__, GHC_GP_CTRL);

#if 0
    ss_sata_misc_init(mmio, sata_port);
    ss_sata_phy_init(mmio, phyMode,sata_port);
#endif

    OUTREG16(BASE_REG_MAILBOX_PA + REG_ID_00, 0x1111);
    pr_err("6+0 , riu_w 0x1004 0x00 0x1111 \r\n");

    // Release SATA MAC/PHY SW resetn
    // riu_w 0x143A 0x00 0x0000
    OUTREG16((MISC_PA + REG_ID_00), 0x0000);
    pr_err("%s, 0x%lX = 0x%X\n", __func__, IO_ADDRESS(MISC_PA + REG_ID_00), INREG16(MISC_PA + REG_ID_00));

    // Turn On SATA clock
    // riu_w 0x1038 0x6E 0x0000
    // riu_w 0x1038 0x6C 0x0000
    // riu_w 0x1038 0x46 0x0000

    OUTREG16((BASE_REG_CLKGEN_PA + REG_ID_6E), 0x0000);
    pr_err("%s, 0x%lX = 0x%X\n", __func__, IO_ADDRESS(BASE_REG_CLKGEN_PA + REG_ID_6E),
           INREG16(BASE_REG_CLKGEN_PA + REG_ID_6E));

    OUTREG16((BASE_REG_CLKGEN_PA + REG_ID_6C), 0x0000);
    pr_err("%s, 0x%lX = 0x%X\n", __func__, IO_ADDRESS(BASE_REG_CLKGEN_PA + REG_ID_6C),
           INREG16(BASE_REG_CLKGEN_PA + REG_ID_6C));

    OUTREG16((BASE_REG_CLKGEN_PA + REG_ID_46), 0x0000);
    pr_err("%s, 0x%lX = 0x%X\n", __func__, IO_ADDRESS(BASE_REG_CLKGEN_PA + REG_ID_46),
           INREG16(BASE_REG_CLKGEN_PA + REG_ID_46));

    OUTREG16(BASE_REG_MAILBOX_PA + REG_ID_00, 0x1111);
    pr_err("6+0 , riu_w 0x1004 0x00 0x1111 \r\n");

// Start OOB Script
#if 0
    devmem 0x1F346804 32 0x00000001
    devmem 0x1F346800 32 0x00000000
    devmem 0x1F34680C 32 0x00000001
    devmem 0x1F34692C 32 0x00000001
    devmem 0x1F34692C 32 0x00000000
    devmem 0x1F346900 32 0x02100000
    devmem 0x1F346908 32 0x02000000
    devmem 0x1F346918 32 0x00000016
#endif

    OUTREG32((mmio + 0x04), 0x00000001);
    pr_err("%s, 0x%lX = 0x%X\n", __func__, IO_ADDRESS(mmio + 0x04), INREG32(mmio + 0x04));

    OUTREG32((mmio + 0x00), 0x00000000);
    pr_err("%s, 0x%lX = 0x%X\n", __func__, IO_ADDRESS(mmio + 0x00), INREG32(mmio + 0x00));

    OUTREG32((mmio + 0x0C), 0x00000001);
    pr_err("%s, 0x%lX = 0x%X\n", __func__, IO_ADDRESS(mmio + 0x0C), INREG32(mmio + 0x0C));

    if (u32_phy_mode == SATA_GEN3)
    {
        //        devmem 0x1F34692C 32 0x00000001
        //        devmem 0x1F34692C 32 0x00000000
        OUTREG32((port_base + 0x2C), 0x00000001);
        pr_err("%s, 0x%lX = 0x%X\n", __func__, IO_ADDRESS(port_base + 0x2C), INREG32((port_base + 0x2C)));

        OUTREG32((port_base + 0x2C), 0x00000000);
        pr_err("%s, 0x%lX = 0x%X\n", __func__, IO_ADDRESS(port_base + 0x2C), INREG32((port_base + 0x2C)));
    }
    else if (u32_phy_mode == SATA_GEN2)
    {
        OUTREG32((port_base + 0x2C), 0x00000021);
        pr_err("%s, 0x%lX = 0x%X\n", __func__, IO_ADDRESS(port_base + 0x2C), INREG32((port_base + 0x2C)));

        OUTREG32((port_base + 0x2C), 0x00000020);
        pr_err("%s, 0x%lX = 0x%X\n", __func__, IO_ADDRESS(port_base + 0x2C), INREG32((port_base + 0x2C)));
    }
    else if (u32_phy_mode == SATA_GEN1)
    {
        OUTREG32((port_base + 0x2C), 0x00000011);
        pr_err("%s, 0x%lX = 0x%X\n", __func__, IO_ADDRESS(port_base + 0x2C), INREG32((port_base + 0x2C)));

        OUTREG32((port_base + 0x2C), 0x00000010);
        pr_err("%s, 0x%lX = 0x%X\n", __func__, IO_ADDRESS(port_base + 0x2C), INREG32((port_base + 0x2C)));
    }

    OUTREG32((port_base + 0x00), 0x02100000);
    pr_err("%s, 0x%lX = 0x%X\n", __func__, IO_ADDRESS(port_base + 0x00), INREG32((port_base + 0x00)));

    OUTREG32((port_base + 0x08), 0x02000000);
    pr_err("%s, 0x%lX = 0x%X\n", __func__, IO_ADDRESS(port_base + 0x08), INREG32((port_base + 0x08)));

    OUTREG32((port_base + 0x18), 0x00000016);
    pr_err("%s, 0x%lX = 0x%X\n", __func__, IO_ADDRESS(port_base + 0x18), INREG32((port_base + 0x18)));
    // 0
    pr_err("%s, 0x%lX = 0x%X\n", __func__, IO_ADDRESS(port_base + 0x28), INREG32((port_base + 0x28)));

    msleep(200);
    pr_err("%s, 0x%lX = 0x%X\n", __func__, IO_ADDRESS(port_base + 0x28), INREG32((port_base + 0x28)));

    msleep(200);
    pr_err("%s, 0x%lX = 0x%X\n", __func__, IO_ADDRESS(port_base + 0x28), INREG32((port_base + 0x28)));

    msleep(200);
    pr_err("%s, 0x%lX = 0x%X\n", __func__, IO_ADDRESS(port_base + 0x28), INREG32((port_base + 0x28)));

    msleep(200);
    pr_err("%s, 0x%lX = 0x%X\n", __func__, IO_ADDRESS(port_base + 0x28), INREG32((port_base + 0x28)));

    // 5

    for (i = 0; i < 20; ++i)
    {
        reg_value = INREG32(port_base + 0x28);

        pr_err("%s, %dth Round  0x%lX = 0x%X\n", __func__, i, IO_ADDRESS(port_base + 0x28), reg_value);

        if (((reg_value & 0x0F) == 0x3)
            && ((reg_value & 0x0F00) == 0x0100)) // ahci_status reg value = 0x113 , 0x123 , 0x133
        {
            pr_err("%s, sata connection pass \n", __func__);
            break;
        }
        msleep(300);
    }
}

static int __init SATA_ut_init(void)
{
    printk("[KUT] %s\r\n", __func__);

    if (port >= MAX_SATA_PORT)
    {
        printk("[%s] wrong port = %d \r\n", __func__, port);
        goto fail;
    }

    if (phy_mode > MAX_SATA_GEN)
    {
        printk("[%s] wrong phy_mode = %d \r\n", __func__, phy_mode);
        goto fail;
    }

    sata_bringup(port, phy_mode);

fail:

    return 0;
}

static void __exit sata_ut_exit(void)
{
    printk("[KUT] SATA UT exit\r\n");
    //    MmuUtRemove();
    return;
}

module_init(SATA_ut_init);
module_exit(sata_ut_exit);
