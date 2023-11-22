/*
 * hal_pcieif.c- Sigmastar
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

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/err.h>
#include "cam_os_wrapper.h"
#include "ms_platform.h"
#include "hal_pcieif.h"
#include "registers.h"

extern void sstar_pcieif_writew_phy(u8 id, u32 reg, u16 val);

/* DBI address bus layout
 *
 * Due the 32-bit data type length, reg addr bit[21:20] are used as CDM/ELBI & CS2 select bits
 *
 * In the design:
 * type | 32 | 31 | 30-20 | 19 | 18-2 | 1 | 0 |
 *  CMD |  0 | CS2|       |  0 |      | 0 | 0 |
 *  ATU |  1 |  1 |       |  0 | addr | 0 | 0 |
 *  DMA |  1 |  1 |       |  1 | addr | 0 | 0 |
 *
 *
 * reg addr layout:
 * type | 31-22 | 21 | 20 | 19 | 18-2 | 1 | 0 |
 *  CMD |       |  0 | CS2|  0 |      | 0 | 0 |
 *  ATU |       |  1 |  1 |  0 | addr | 0 | 0 |
 *  DMA |       |  1 |  1 |  1 | addr | 0 | 0 |
 */
u32 hal_prog_dbi_addr(void __iomem *mac_base, u32 reg)
{
    u32 bits = 0;

    bits = reg >> 12;
    if (bits)
    {
        // bit[31:12] are not zeros
        // here is a trick, bit[21:20] is actually for bit[32:31]
        bits = (bits & 0xFFCFF) | ((reg & 0x300000) >> 1);
        writew(bits & 0x1FF, mac_base + REG_DBI_ADDR_20to12);
        writew((bits >> 9) & 0xFFF, mac_base + REG_DBI_ADDR_32to21);
    }
    return reg & 0xFFF; // dbi addr bit[11:0]
}

void hal_rst_dbi_addr(void __iomem *mac_base)
{
    writew(0, mac_base + REG_DBI_ADDR_20to12);
    writew(0, mac_base + REG_DBI_ADDR_32to21);
}

void hal_internalclk_en(u8 id, u8 ssc_enable)
{
    // u16 u16regdat = 0;

    // upll1 for pcie//
    //  xtal_atop wriu 0x00111b12 0x00
    OUTREGMSK16(BASE_REG_XTAL_ATOP_PA + REG_ID_09, 0, 0x00FF);

    //+++wriu 0x00103003 0x00
    OUTREGMSK16(BASE_REG_MPLL_PA + REG_ID_01, 0, 0xFF00);

    if (id == 0)
    {
        // wriu 0x00141f46 0x32
        OUTREGMSK16(BASE_REG_UPLL1_PA + REG_ID_23, 0x32, 0x00FF);

        // wriu 0x00141f4e 0xbc
        OUTREGMSK16(BASE_REG_UPLL1_PA + REG_ID_27, 0xbc, 0x00FF);

        // wriu 0x00141f00 0x00
        OUTREGMSK16(BASE_REG_UPLL1_PA + REG_ID_00, 0x00, 0x00FF);

        // wriu 0x00141f40 0x20
        OUTREGMSK16(BASE_REG_UPLL1_PA + REG_ID_20, 0x20, 0x00FF);

        // wriu 0x00141f4a 0xaf
        OUTREGMSK16(BASE_REG_UPLL1_PA + REG_ID_25, 0xaf, 0x00FF);

        // wriu 0x00141f4c 0x80
        OUTREGMSK16(BASE_REG_UPLL1_PA + REG_ID_26, 0x80, 0x00FF);

        //+++wriu 0x00141f4d 0x81
        OUTREGMSK16(BASE_REG_UPLL1_PA + REG_ID_26, 0x8100, 0xFF00);

        //+++wriu 0x00141f43 0x01
        OUTREGMSK16(BASE_REG_UPLL1_PA + REG_ID_21, 0x0100, 0xFF00);

        // SATA
        OUTREG16(BASE_REG_SATA_MAC_PA + REG_ID_00, 0);
        // OUTREG16(BASE_REG_CLKGEN_PA + REG_ID_6E, 0);
        // OUTREG16(BASE_REG_CLKGEN_PA + REG_ID_6C, 0);
        // OUTREG16(BASE_REG_CLKGEN_PA + REG_ID_46, 0);

        CLRREG16(BASE_REG_SATA_MAC2_PA + REG_ID_00, BIT(12));
        SETREG16(BASE_REG_SATA_PHY2_PA + REG_ID_14, BIT(0));
        SETREG16(BASE_REG_SATA_PHY2_PA + REG_ID_14, BIT(12));
        SETREG16(BASE_REG_SATA_PHY2_PA + REG_ID_15, BIT(0) | BIT(1));
        SETREG16(BASE_REG_SATA_PHY2_PA + REG_ID_16, BIT(10) | BIT(11));

        // u16regdat = INREG16(0x1f000000 + (0x143d * 0x200) + 0x31 * 4);
        // printk("chk1=0x%x \r\n", u16regdat);
        SETREG16(BASE_REG_SATA_PHY2_PA + REG_ID_31, BIT(1) | BIT(2));
        // u16regdat = INREG16(0x1f000000 + (0x143d * 0x200) + 0x31 * 4);
        // printk("chk2=0x%x \r\n", u16regdat);

        if (ssc_enable)
        {
            // UPLL1_SSC-3.txt
            OUTREG16(BASE_REG_UPLL1_PA + REG_ID_0B, 0x0000);
            OUTREG16(BASE_REG_UPLL1_PA + REG_ID_0C, 0x0090);
            OUTREG16(BASE_REG_UPLL1_PA + REG_ID_0E, 0x00C1);
            OUTREG16(BASE_REG_UPLL1_PA + REG_ID_0F, 0x0003);
            OUTREG16(BASE_REG_UPLL1_PA + REG_ID_0D, 0x000E);
            OUTREG16(BASE_REG_UPLL1_PA + REG_ID_24, 0x0000);
        }
    }

    if (id == 1)
    {
        // PCIE1
        sstar_pcieif_writew_phy(id, REG_PHY_REF_USEPAD, 0x0000); // Select ref_pad_clk_p/m

        // aupll for pcie//
        // wriu 0x00111b12 0x00
        //+++wriu 0x00103003 0x00
        // wriu 0x00141d4e 0xbc
        OUTREGMSK16(BASE_REG_AUPLL_PA + REG_ID_27, 0xbc, 0x00FF);

        // wriu 0x00141d46 0x19
        OUTREGMSK16(BASE_REG_AUPLL_PA + REG_ID_23, 0x19, 0x00FF);

        /// wriu 0x00141d00 0x00
        OUTREGMSK16(BASE_REG_AUPLL_PA + REG_ID_00, 0x00, 0x00FF);

        // wriu 0x00141d40 0x20
        OUTREGMSK16(BASE_REG_AUPLL_PA + REG_ID_20, 0x20, 0x00FF);

        // wriu 0x00141d4a 0xa7
        OUTREGMSK16(BASE_REG_AUPLL_PA + REG_ID_25, 0xa7, 0x00FF);

        // wriu 0x00141d4c 0x80
        OUTREGMSK16(BASE_REG_AUPLL_PA + REG_ID_26, 0x80, 0x00FF);

        //+++wriu 0x00141d4d 0x81
        OUTREGMSK16(BASE_REG_AUPLL_PA + REG_ID_26, 0x8100, 0xFF00);

        if (ssc_enable)
        {
            // AUPLL_SSC-3.txt
            OUTREG16(BASE_REG_AUPLL_PA + REG_ID_0B, 0x0000);
            OUTREG16(BASE_REG_AUPLL_PA + REG_ID_0C, 0x0090);
            OUTREG16(BASE_REG_AUPLL_PA + REG_ID_0E, 0x00C1);
            OUTREG16(BASE_REG_AUPLL_PA + REG_ID_0F, 0x0003);
            OUTREG16(BASE_REG_AUPLL_PA + REG_ID_0D, 0x000E);
            OUTREG16(BASE_REG_AUPLL_PA + REG_ID_24, 0x0000);
        }
    }
}
