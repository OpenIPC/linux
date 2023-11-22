/*
 * mhal_gmac.c- Sigmastar
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

#include <linux/clk.h>
#include <linux/kernel.h>
#include <linux/mfd/syscon.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/of_net.h>
#include <linux/phy.h>
#include <linux/platform_device.h>
#include <linux/stmmac.h>
#include <linux/ctype.h>
#include <linux/string.h>
#include "dwmac4.h"
#include "registers.h"
#include "stmmac_platform.h"
#include "sstar_gmac.h"

enum
{
    GMAC_MDC,
    GMAC_MDIO,
    GMAC_RGMII_TX_CLK,
    GMAC_RGMII_TX_D0,
    GMAC_RGMII_TX_D1,
    GMAC_RGMII_TX_D2,
    GMAC_RGMII_TX_D3,
    GMAC_RGMII_TX_CTL,
    GMAC_MAX_IO
};

char *Gmac_IOName[GMAC_BANK_NUM] = {
    "MDC", "MDIO", "RGMII_TX_CLK", "RGMII_TX_D0", "RGMII_TX_D1", "RGMII_TX_D2", "RGMII_TX_D3", "RGMII_TX_CTL",
};

//#define GMAC_FPGA             (1)
#define GAMC_DRIVING_UPDATEALL 0xff
#define GMAC_MAX_GEAR          4
#define GMAC_DRIVING_REG_MASK  0x180
#define GMAC_DRIVING_REG_SHIFT 7

extern void *GBase[GMAC_NUM];
extern u32   GBank[GMAC_NUM][GMAC_BANK_NUM];

u8 GMAC_Driving_Setting[GMAC_NUM][GMAC_MAX_IO] = {{
                                                      1, // RGMII0_MDC
                                                      1, // RGMII0_MDIO
                                                      1, // RGMII0_TX_CLK
                                                      1, // RGMII0_TX_D0
                                                      1, // RGMII0_TX_D1
                                                      1, // RGMII0_TX_D2
                                                      1, // RGMII0_TX_D3
                                                      1  // RGMII0_TX_CTL
                                                  },
                                                  {
                                                      1, // RGMII1_MDC
                                                      1, // RGMII1_MDIO
                                                      1, // RGMII1_TX_CLK
                                                      1, // RGMII1_TX_D0
                                                      1, // RGMII1_TX_D1
                                                      1, // RGMII1_TX_D2
                                                      1, // RGMII1_TX_D3
                                                      1  // RGMII1_TX_CTL
                                                  }};

u8 GMAC_Driving_Offset[GMAC_NUM][GMAC_MAX_IO] = {{0x73, 0x72, 0x6C, 0x6E, 0x6F, 0x70, 0x71, 0x6D},
                                                 {0x64, 0x63, 0x5D, 0x5F, 0x60, 0x61, 0x62, 0x5E}};

/*
 * Usage of proc for GMAC
 * 1.cat /proc/gmac_0/driving
 * 2.echo io_idx gear > /proc/gmac_0/driving
 *
 * ex :
 * input:
 * echo 0 8 > /proc/gmac_0/driving
 * output on console:
 * MDC driving = 8
 *
 * Driving Gear Mapping :
 * 3.3V mode (typical)
 * Gear (DS3, DS2, DS1, DS0) =
 * 0 -> (0, 0, 0, 0) -> 4.4mA
 * 1 -> (0, 0, 0, 1) -> 6.5mA
 * 2 -> (0, 0, 1, 0) -> 8.7mA
 * 3 -> (0, 0, 1, 1) -> 10.9mA
 * 4 -> (0, 1, 0, 0) -> 13.0mA
 * 5 -> (0, 1, 0, 1) -> 15.2mA
 * 6 -> (0, 1, 1, 0) -> 17.4mA
 * 7 -> (0, 1, 1, 1) -> 19.5mA
 * 8 -> (1, 0, 0, 0) -> 21.7mA
 * 9 -> (1, 0, 0, 1) -> 23.9mA
 * 10 -> (1, 0, 1, 0) -> 26.0mA
 * 11 -> (1, 0, 1, 1) -> 28.2mA
 * 12 -> (1, 1, 0, 0) -> 30.3mA
 * 13 -> (1, 1, 0, 1) -> 32.5mA
 * 14 -> (1, 1, 1, 0) -> 34.6mA
 * 15 -> (1, 1, 1, 1) -> 36.8mA
 * */

static void mhal_gmac_update_driving(u8 gmacId, u8 io_idx)
{
    if (io_idx == GAMC_DRIVING_UPDATEALL)
    { // Update All IO
        int i = 0;

        for (i = 0; i < GMAC_MAX_IO; i++)
        {
            OUTREGMSK16(GET_REG_ADDR(BASE_REG_PAD_GPIO_PA, GMAC_Driving_Offset[gmacId][i]),
                        (u16)(GMAC_Driving_Setting[gmacId][i] << GMAC_DRIVING_REG_SHIFT), GMAC_DRIVING_REG_MASK);
        }
    }
    else
    { // Update one IO by io_idx
        OUTREGMSK16(GET_REG_ADDR(BASE_REG_PAD_GPIO_PA, GMAC_Driving_Offset[gmacId][io_idx]),
                    (u16)(GMAC_Driving_Setting[gmacId][io_idx] << GMAC_DRIVING_REG_SHIFT), GMAC_DRIVING_REG_MASK);
    }
}

static int proc_gmac_driving_show(struct seq_file *m, void *v)
{
    int                 i;
    struct sstar_dwmac *dwmac        = m->private;
    u8 *                pDrivingGear = GMAC_Driving_Setting[dwmac->id];

    if (pDrivingGear == GMAC_Driving_Setting[0])
        printk("gmac0 driving:\r\n");
    else
        printk("gmac1 driving:\r\n");

    for (i = 0; i < GMAC_MAX_IO; i++)
    {
        printk("%s = %u", Gmac_IOName[i], *pDrivingGear);
        pDrivingGear++;
    }
    printk("\r\n");
    return 0;
}

static int proc_gmac_driving_open(struct inode *inode, struct file *file)
{
    return single_open(file, proc_gmac_driving_show, PDE_DATA(inode));
}

static ssize_t proc_gmac_driving_write(struct file *file, const char __user *buf, size_t len, loff_t *pos)
{
    u8                  usr_gear     = 0xFF;
    u8                  io_idx       = 0xFF;
    char *              ptr          = NULL;
    char                strbuf[128]  = {0};
    struct sstar_dwmac *dwmac        = ((struct seq_file *)file->private_data)->private;
    u8 *                pDrivingGear = GMAC_Driving_Setting[dwmac->id];

    if (len > sizeof(strbuf) - 1)
    {
        printk(KERN_ERR "command len is to long!\n");
        return len;
    }

    if (copy_from_user(strbuf, buf, len))
    {
        return -EFAULT;
    }

    strbuf[len] = 0;
    io_idx      = simple_strtol(strbuf, NULL, 10);

    if (io_idx >= GMAC_MAX_IO)
    {
        return -EFAULT;
    }

    ptr = strbuf;
    ptr = strchr(strbuf, ' ');
    while (isspace(*ptr))
        ptr++; // skip space
    usr_gear = simple_strtol(ptr, NULL, 10);

    if (usr_gear >= GMAC_MAX_GEAR)
    {
        return -EFAULT;
    }

    *(pDrivingGear + io_idx) = usr_gear;

    if (pDrivingGear == GMAC_Driving_Setting[0])
        mhal_gmac_update_driving(GMAC0, io_idx);
    else
        mhal_gmac_update_driving(GMAC1, io_idx);

    printk("%s driving = %u\n", Gmac_IOName[io_idx], usr_gear);

    return len;
}

static const struct proc_ops proc_gmac_driving_ops = {.proc_open    = proc_gmac_driving_open,
                                                      .proc_read    = seq_read,
                                                      .proc_write   = proc_gmac_driving_write,
                                                      .proc_lseek   = seq_lseek,
                                                      .proc_release = single_release};

static int proc_gmac_clk_inv_show(struct seq_file *m, void *v)
{
    struct sstar_dwmac *dwmac = m->private;

    if (dwmac->id == GMAC0)
    {
        printk("Clk invert = %u\r\n", (INREG16(GET_REG_ADDR(BASE_REG_GMAC0_PA, 0x02)) >> 7) & 0x01);
    }
    else
    {
        printk("Clk invert = %u\r\n", (INREG16(GET_REG_ADDR(BASE_REG_GMAC1_PA, 0x02)) >> 7) & 0x01);
    }

    return 0;
}

static int proc_gmac_clk_inv_open(struct inode *inode, struct file *file)
{
    return single_open(file, proc_gmac_clk_inv_show, PDE_DATA(inode));
}

static ssize_t proc_gmac_clk_inv_write(struct file *file, const char __user *buf, size_t len, loff_t *pos)
{
    u8                  inv         = 0;
    char                strbuf[128] = {0};
    struct sstar_dwmac *dwmac       = ((struct seq_file *)file->private_data)->private;

    if (len > sizeof(strbuf) - 1)
    {
        printk(KERN_ERR "command len is to long!\n");
        return len;
    }

    if (copy_from_user(strbuf, buf, len))
    {
        return -EFAULT;
    }

    strbuf[len] = 0;
    inv         = simple_strtol(strbuf, NULL, 10);

    if (dwmac->id == GMAC0)
    {
        OUTREGMSK16(GET_REG_ADDR(BASE_REG_GMAC0_PA, 0x02), inv << 7, 0x0080);
    }
    else
    {
        OUTREGMSK16(GET_REG_ADDR(BASE_REG_GMAC1_PA, 0x02), inv << 7, 0x0080);
    }

    return len;
}

static const struct proc_ops proc_gmac_clk_inv_ops = {.proc_open    = proc_gmac_clk_inv_open,
                                                      .proc_read    = seq_read,
                                                      .proc_write   = proc_gmac_clk_inv_write,
                                                      .proc_lseek   = seq_lseek,
                                                      .proc_release = single_release};

static int proc_gmac_txc_show(struct seq_file *m, void *v)
{ // BIT11~BIT12 : TXC Phase(B11->degree 90)(B10->degree 270)(B00->degree 0)(B01->degree 180)
    struct sstar_dwmac *dwmac = m->private;
    u32                 gmac_bank, val;

    if (dwmac->id == GMAC0)
    {
        gmac_bank = BASE_REG_GMAC0_PA;
    }
    else
    {
        gmac_bank = BASE_REG_GMAC1_PA;
    }

    val = INREG16(GET_REG_ADDR(gmac_bank, 0x31));
    val = (val >> 11) & 0x03;

    if (val == 0)
    {
        printk("tx degree 0\n");
    }
    else if (val == 1)
    {
        printk("tx degree 180\n");
    }
    else if (val == 2)
    {
        printk("tx degree 270\n");
    }
    else if (val == 3)
    {
        printk("tx degree 90\n");
    }

    return 0;
}

static int proc_gmac_txc_open(struct inode *inode, struct file *file)
{
    return single_open(file, proc_gmac_txc_show, PDE_DATA(inode));
}

static ssize_t proc_gmac_txc_write(struct file *file, const char __user *buf, size_t len, loff_t *pos)
{
    u32                 txc         = 0;
    char                strbuf[128] = {0};
    struct sstar_dwmac *dwmac       = ((struct seq_file *)file->private_data)->private;
    u32                 gmac_bank;

    if (len > sizeof(strbuf) - 1)
    {
        printk(KERN_ERR "command len is to long!\n");
        return len;
    }

    if (copy_from_user(strbuf, buf, len))
    {
        return -EFAULT;
    }

    strbuf[len] = 0;
    txc         = simple_strtol(strbuf, NULL, 10);

    if (dwmac->id == GMAC0)
    {
        gmac_bank = BASE_REG_GMAC0_PA;
    }
    else
    {
        gmac_bank = BASE_REG_GMAC1_PA;
    }

    if (txc == 0)
    {
        OUTREGMSK16(GET_REG_ADDR(gmac_bank, 0x31), 0x0000, (BIT11 | BIT12));
        printk("tx degree 0\n");
    }
    else if (txc == 90)
    {
        OUTREGMSK16(GET_REG_ADDR(gmac_bank, 0x31), 0x1800, (BIT11 | BIT12));
        printk("\ntx degree 90\n");
    }
    else if (txc == 180)
    {
        OUTREGMSK16(GET_REG_ADDR(gmac_bank, 0x31), 0x0800, (BIT11 | BIT12));
        printk("\ntx degree 180\n");
    }
    else if (txc == 270)
    {
        OUTREGMSK16(GET_REG_ADDR(gmac_bank, 0x31), 0x1000, (BIT11 | BIT12));
        printk("\ntx degree 270\n");
    }

    return len;
}

static const struct proc_ops proc_gmac_txc_ops = {.proc_open    = proc_gmac_txc_open,
                                                  .proc_read    = seq_read,
                                                  .proc_write   = proc_gmac_txc_write,
                                                  .proc_lseek   = seq_lseek,
                                                  .proc_release = single_release};

static int proc_gmac_rxc_show(struct seq_file *m, void *v)
{ // BIT8~BIT9 : RXC latch timing(B01->falling ,B10->raising (for 100Mbps only))
    struct sstar_dwmac *dwmac = m->private;
    u32                 gmac_bank, val;

    if (dwmac->id == GMAC0)
    {
        gmac_bank = BASE_REG_GMAC0_PA;
    }
    else
    {
        gmac_bank = BASE_REG_GMAC1_PA;
    }

    val = INREG16(GET_REG_ADDR(gmac_bank, 0x31));
    val = (val >> 8) & 0x03;

    if (val == 0)
    {
        printk("rxc none\n");
    }
    else if (val == 1)
    {
        printk("rxc falling\n");
    }
    else if (val == 2)
    {
        printk("rxc raising\n");
    }

    return 0;
}

static int proc_gmac_rxc_open(struct inode *inode, struct file *file)
{
    return single_open(file, proc_gmac_rxc_show, PDE_DATA(inode));
}

static ssize_t proc_gmac_rxc_write(struct file *file, const char __user *buf, size_t len, loff_t *pos)
{
    char                strbuf[128] = {0};
    struct sstar_dwmac *dwmac       = ((struct seq_file *)file->private_data)->private;
    u32                 gmac_bank;

    if (len > sizeof(strbuf) - 1)
    {
        printk(KERN_ERR "command len is to long!\n");
        return len;
    }

    if (copy_from_user(strbuf, buf, len))
    {
        return -EFAULT;
    }

    strbuf[len] = 0;

    if (dwmac->id == GMAC0)
    {
        gmac_bank = BASE_REG_GMAC0_PA;
    }
    else
    {
        gmac_bank = BASE_REG_GMAC1_PA;
    }

    if (strcmp(strbuf, "falling\n") == 0)
    {
        OUTREGMSK16(GET_REG_ADDR(gmac_bank, 0x31), 0x0100, (BIT8 | BIT9));
        printk("\nrxc falling\n");
    }
    else if (strcmp(strbuf, "raising\n") == 0)
    {
        OUTREGMSK16(GET_REG_ADDR(gmac_bank, 0x31), 0x0200, (BIT8 | BIT9));
        printk("\nrxc raising\n");
    }
    else if (strcmp(strbuf, "none\n") == 0)
    {
        OUTREGMSK16(GET_REG_ADDR(gmac_bank, 0x31), 0x0000, (BIT8 | BIT9));
        printk("\nrxc none\n");
    }

    return len;
}

static const struct proc_ops proc_gmac_rxc_ops = {.proc_open    = proc_gmac_rxc_open,
                                                  .proc_read    = seq_read,
                                                  .proc_write   = proc_gmac_rxc_write,
                                                  .proc_lseek   = seq_lseek,
                                                  .proc_release = single_release};

void mhal_gmac_disable_padmux(void)
{
    OUTREGMSK16(GET_REG_ADDR(BASE_REG_PADTOP_PA, 0x41), 0x0000, 0x0003);
}

static void mhal_gmac_gpio_reset(struct sstar_dwmac *dwmac)
{
    u32 bank   = BASE_REG_PAD_GPIO_PA;
    u32 offset = 0x65;

    if (dwmac->reset_io == 0xFF)
        return;

    if (dwmac->id == GMAC0)
    {
#ifdef GMAC_FPGA
        OUTREGMSK16(GET_REG_ADDR(BASE_REG_GMAC0_PA, 0x31), 0x0000, 0x000F);
        mdelay(1);
        OUTREGMSK16(GET_REG_ADDR(BASE_REG_GMAC0_PA, 0x31), 0x0002, 0x000F);
#else
        if (dwmac->reset_io == 1)
        {
            // GPIO 138
            bank   = BASE_REG_PAD_GPIO2_PA;
            offset = 0x0A;
        }
        else
        {
            // GPIO 101
            bank   = BASE_REG_PAD_GPIO_PA;
            offset = 0x65;
        }
#endif
    }
    else if (dwmac->id == GMAC1)
    {
#ifdef GMAC_FPGA
        OUTREGMSK16(GET_REG_ADDR(BASE_REG_GMAC1_PA, 0x31), 0x0000, 0x000F);
        mdelay(1);
        OUTREGMSK16(GET_REG_ADDR(BASE_REG_GMAC1_PA, 0x31), 0x0002, 0x000F);
#else
        if (dwmac->reset_io == 1)
        {
            // GPIO 86
            bank   = BASE_REG_PAD_GPIO_PA;
            offset = 0x56;
        }
        else
        {
            // GPIO 85
            bank   = BASE_REG_PAD_GPIO_PA;
            offset = 0x55;
        }
#endif
    }

    // Output Low
    OUTREGMSK16(GET_REG_ADDR(bank, offset), 0x0008, 0x000F);
    mdelay(20);
    // Output High
    OUTREGMSK16(GET_REG_ADDR(bank, offset), 0x000A, 0x000F);
}

static void mhal_gmac_mclk_setting(struct sstar_dwmac *dwmac)
{
    if (dwmac->id == GMAC0)
    {
        if (dwmac->mclk_freq == 0)
        {
            // do nothing
        }
        else
        {
            if (dwmac->mclk_refmode == 1)
            {
                // GPIO138 : disable GPIO mode ,enable output
                OUTREG8(GET_REG_ADDR8(BASE_REG_PAD_GPIO2_PA, 0x14), 0x0a50);
                // padtop reg_gphy0_ref_mode
                OUTREGMSK16(GET_REG_ADDR(BASE_REG_PADTOP_PA, 0x38), 0x0001, BIT1 | BIT0);
            }
            else if (dwmac->mclk_refmode == 2)
            {
                // GPIO101 : disable GPIO mode ,enable output
                OUTREG8(GET_REG_ADDR8(BASE_REG_PAD_GPIO_PA, 0xCA), 0x0a50);
                // padtop reg_gphy0_ref_mode
                OUTREGMSK16(GET_REG_ADDR(BASE_REG_PADTOP_PA, 0x38), 0x0002, BIT1 | BIT0);
            }
            else
            {
                // default is 1
                //  GPIO138 : disable GPIO mode ,enable output
                OUTREG8(GET_REG_ADDR8(BASE_REG_PAD_GPIO2_PA, 0x14), 0x0a50);
                // padtop reg_gphy0_ref_mode
                OUTREGMSK16(GET_REG_ADDR(BASE_REG_PADTOP_PA, 0x38), 0x0001, BIT1 | BIT0);
            }
        }
    }
    else if (dwmac->id == GMAC1)
    {
        if (dwmac->mclk_freq == 0)
        {
            // do nothing
        }
        else
        {
            if (dwmac->mclk_refmode == 1)
            {
                // GPIO86 : disable GPIO mode ,enable output
                OUTREG8(GET_REG_ADDR8(BASE_REG_PAD_GPIO_PA, 0xAC), 0x0a50);
                // padtop reg_gphy0_ref_mode
                OUTREGMSK16(GET_REG_ADDR(BASE_REG_PADTOP_PA, 0x38), 0x0010, BIT4 | BIT5);
            }
            else if (dwmac->mclk_refmode == 2)
            {
                // GPIO85 : disable GPIO mode ,enable output
                OUTREG8(GET_REG_ADDR8(BASE_REG_PAD_GPIO_PA, 0xAA), 0x0a50);
                // padtop reg_gphy0_ref_mode
                OUTREGMSK16(GET_REG_ADDR(BASE_REG_PADTOP_PA, 0x38), 0x0020, BIT4 | BIT5);
            }
            else
            {
                // default is 1
                //  GPIO86 : disable GPIO mode ,enable output
                OUTREG8(GET_REG_ADDR8(BASE_REG_PAD_GPIO_PA, 0xAC), 0x0a50);
                // padtop reg_gphy0_ref_mode
                OUTREGMSK16(GET_REG_ADDR(BASE_REG_PADTOP_PA, 0x38), 0x0010, BIT4 | BIT5);
            }
        }
    }
}

void mhal_gmac_probe(struct sstar_dwmac *dwmac)
{
    // turn on gmacpll; wait around 200us for PLL to be stable
    // swch 4
    // wriu   0x00103a02 0x00
    // wriu   0x00103a03 0x00
    OUTREG8(GET_REG_ADDR8(BASE_REG_GMACPLL_PA, 0x02), 0);
    OUTREG8(GET_REG_ADDR8(BASE_REG_GMACPLL_PA, 0x03), 0);

    // OUTREG16(0x1f207402, 0x0000);
    //  turn on d2s & vco_en
    // wriu   0x00103a2c 0xbe
    OUTREG8(GET_REG_ADDR8(BASE_REG_GMACPLL_PA, 0x2c), 0xbe);

    // new add for U02
    // wait 1
    // wriu 0x00103a15 0x40
    mdelay(1);
    OUTREG8(GET_REG_ADDR8(BASE_REG_GMACPLL_PA, 0x15), 0x40);

    mhal_gmac_mclk_setting(dwmac);
    mhal_gmac_update_driving(dwmac->id, GAMC_DRIVING_UPDATEALL);

    if (dwmac->id == GMAC1)
    {
        // disable long packet protection for gmac1
        OUTREG16(GET_REG_ADDR(BASE_REG_GMAC1_PA, 0x28), INREG16(GET_REG_ADDR(BASE_REG_GMAC1_PA, 0x28)) & 0x7FFF);

        // padtop 103c -> RGMII 1.8V
        // OUTREG16(GET_REG_ADDR(BASE_REG_PADTOP_PA,0x04), (INREG16(GET_REG_ADDR(BASE_REG_PADTOP_PA,0x04)) | 0x02));

        printk("[%s][%d] SS GMAC1 Setting : Interface=%u\r\n", __func__, __LINE__, dwmac->interface);

        if (dwmac->interface == PHY_INTERFACE_MODE_MII)
        {
            OUTREG16(GET_REG_ADDR(BASE_REG_GMAC1_PA, 0x01), 0x8000);
        }
        else if (dwmac->interface == PHY_INTERFACE_MODE_RMII)
        {
            // synopsys x32 bank (10M : xA003,100M : xE003, 1000M : ?)
            // OUTREG32(GET_REG_ADDR(BASE_REG_X32_GMAC1_PA,0x00),0xA000);
            // MAC via RMII
            OUTREG16(GET_REG_ADDR(BASE_REG_GMAC1_PA, 0x01), 0x8004);
            // padtop 103c
            OUTREGMSK16(GET_REG_ADDR(BASE_REG_PADTOP_PA, 0x41), BIT1, BIT1);
            // chiptop 101e
            OUTREGMSK16(GET_REG_ADDR(BASE_REG_CHIPTOP_PA, 0x50), 0x0000, 0xFF00);
        }
        else if (dwmac->interface == PHY_INTERFACE_MODE_RGMII)
        {
            // synopsys x32 bank (10M : xA003,100M : xE003, 1000M : ?)
            // OUTREG32(GET_REG_ADDR(BASE_REG_X32_GMAC1_PA,0x00),0xA000);
            // MAC via RGMII
            OUTREG16(GET_REG_ADDR(BASE_REG_GMAC1_PA, 0x01), 0x0001);
            // padtop 103c
            OUTREGMSK16(GET_REG_ADDR(BASE_REG_PADTOP_PA, 0x41), BIT1, BIT1);
            // chiptop 101e
            OUTREGMSK16(GET_REG_ADDR(BASE_REG_CHIPTOP_PA, 0x50), 0x0000, 0xFF00);
        }

        mhal_gmac_gpio_reset(dwmac);

        printk("[%s][%d] INREG16(0x1f2A2204)=0x%x\r\n", __func__, __LINE__,
               INREG16(GET_REG_ADDR(BASE_REG_GMAC1_PA, 0x01)));

        // 1512 10 bit1 block reg for interrupt entry gmac1
        OUTREG16(GET_REG_ADDR(BASE_REG_NET_GP_CTRL_PA, 0x10),
                 INREG16(GET_REG_ADDR(BASE_REG_NET_GP_CTRL_PA, 0x10)) & (~BIT(1)));
        printk("[%s][%d] INREG16(0x1f2a2440)=%x\r\n", __func__, __LINE__,
               INREG16(GET_REG_ADDR(BASE_REG_NET_GP_CTRL_PA, 0x10)));
    }
    else
    {
        // disable long packet protection for gmac0
        OUTREG16(GET_REG_ADDR(BASE_REG_GMAC0_PA, 0x28), INREG16(GET_REG_ADDR(BASE_REG_GMAC0_PA, 0x28)) & 0x7FFF);

        // padtop 103c -> RGMII 1.8V
        // OUTREG16(GET_REG_ADDR(BASE_REG_PADTOP_PA,0x04), (INREG16(GET_REG_ADDR(BASE_REG_PADTOP_PA,0x04)) | 0x80));

        printk("[%s][%d] SS GMAC0 Setting : Interface=%u\r\n", __func__, __LINE__, dwmac->interface);
        if (dwmac->interface == PHY_INTERFACE_MODE_MII)
        {
            OUTREG16(GET_REG_ADDR(BASE_REG_GMAC0_PA, 0x01), 0x0000);
        }
        else if (dwmac->interface == PHY_INTERFACE_MODE_RMII)
        {
            // synopsys x32 bank (10M : xA003,100M : xE003, 1000M : ?)
            // OUTREG32(GET_REG_ADDR(BASE_REG_X32_GMAC0_PA,0x00),0xA000);
            // MAC via RMII
            OUTREG16(GET_REG_ADDR(BASE_REG_GMAC0_PA, 0x01), 0x8004);
            // padtop 103c
            OUTREGMSK16(GET_REG_ADDR(BASE_REG_PADTOP_PA, 0x41), BIT0, BIT0);
            // chiptop 101e
            OUTREGMSK16(GET_REG_ADDR(BASE_REG_CHIPTOP_PA, 0x50), 0x0000, 0xFF00);
        }
        else if (dwmac->interface == PHY_INTERFACE_MODE_RGMII)
        {
            // synopsys x32 bank (10M : xA003,100M : xE003, 1000M : ?)
            // OUTREG32(GET_REG_ADDR(BASE_REG_X32_GMAC0_PA,0x00),0xA000);
            // MAC via RGMII
            OUTREG16(GET_REG_ADDR(BASE_REG_GMAC0_PA, 0x01), 0x0001);
            // padtop 103c
            OUTREGMSK16(GET_REG_ADDR(BASE_REG_PADTOP_PA, 0x41), BIT0, BIT0);
            // chiptop 101e All padin
            OUTREGMSK16(GET_REG_ADDR(BASE_REG_CHIPTOP_PA, 0x50), 0x0000, 0xFF00);
        }

        mhal_gmac_gpio_reset(dwmac);

        printk("[%s][%d] INREG16(0x1f2A2004)=0x%x\r\n", __func__, __LINE__,
               INREG16(GET_REG_ADDR(BASE_REG_GMAC0_PA, 0x01)));
        // 1512 10 bit0 block reg for interrupt entry gmac0
        OUTREG16(GET_REG_ADDR(BASE_REG_NET_GP_CTRL_PA, 0x10),
                 INREG16(GET_REG_ADDR(BASE_REG_NET_GP_CTRL_PA, 0x10)) & (~BIT(0)));
        printk("[%s][%d] INREG16(0x1f2a2440)=%x\r\n", __func__, __LINE__,
               INREG16(GET_REG_ADDR(BASE_REG_NET_GP_CTRL_PA, 0x10)));
    }

    // 1017 h6f bit11 clear to 0 for interrupt entry 2 , we got two entry of interrupt
    printk("[%s][%d] INREG16(0x1f202fbc)=%x\r\n", __func__, __LINE__,
           INREG16(GET_REG_ADDR(BASE_REG_INTR_CTRL1_1_PA, 0x6F)));
    OUTREG16(GET_REG_ADDR(BASE_REG_INTR_CTRL1_1_PA, 0x6F),
             INREG16(GET_REG_ADDR(BASE_REG_INTR_CTRL1_1_PA, 0x6F)) & (~BIT(11)));
    printk("[%s][%d] INREG16(0x1f202fbc)=%x\r\n", __func__, __LINE__,
           INREG16(GET_REG_ADDR(BASE_REG_INTR_CTRL1_1_PA, 0x6F)));

    proc_create_data("driving", 0, dwmac->gmac_root_dir, &proc_gmac_driving_ops, dwmac);
    proc_create_data("clk_inv", 0, dwmac->gmac_root_dir, &proc_gmac_clk_inv_ops, dwmac);
    proc_create_data("txc", 0, dwmac->gmac_root_dir, &proc_gmac_txc_ops, dwmac);
    proc_create_data("rxc", 0, dwmac->gmac_root_dir, &proc_gmac_rxc_ops, dwmac);
}

#define GMAC_CALB_MSK     (BIT(2) | BIT(3))
#define GMAC_TX_PHASE_MSK (BIT(11) | BIT(12))

void mhal_gmac_tx_clk_pad_sel(struct sstar_dwmac *dwmac, unsigned int speed)
{
    u16 val = 0;

    if (dwmac->id == GMAC1)
    {
        if (dwmac->interface == PHY_INTERFACE_MODE_RMII)
        {
            switch (speed)
            {
                case SPEED_1000:
                    // without overwrite bit 2,3
                    OUTREGMSK16(GET_REG_ADDR(BASE_REG_GMAC1_PA, 0x31), 0x0000, ~GMAC_SEL_MSK);
                    break;
                case SPEED_100:
                    OUTREGMSK16(GET_REG_ADDR(BASE_REG_GMAC1_PA, 0x31), 0x2200, ~GMAC_SEL_MSK);
                    break;
                case SPEED_10:
                    OUTREGMSK16(GET_REG_ADDR(BASE_REG_GMAC1_PA, 0x31), 0x2200, ~GMAC_SEL_MSK);
                    break;
            }
        }
        else if (dwmac->interface == PHY_INTERFACE_MODE_RGMII)
        {
            switch (speed)
            {
                case SPEED_1000:
                    OUTREGMSK16(GET_REG_ADDR(BASE_REG_GMAC1_PA, 0x31), 0x0000, ~GMAC_SEL_MSK);
                    break;
                case SPEED_100:
                    // rx use negedge, without overwrite bit 2,3
                    OUTREGMSK16(GET_REG_ADDR(BASE_REG_GMAC1_PA, 0x31), 0x140, ~GMAC_SEL_MSK);
#if (DYN_PHASE_CALB == 0)
                    if (NEED_CALB)
                    {
                        // reference bit 2,3 to calibrate tx clk phase
                        val = (INREG16(GET_REG_ADDR(BASE_REG_GMAC1_PA, 0x31)) & GMAC_CALB_MSK) >> 2;
                        // only update bit 11.12
                        OUTREGMSK16(GET_REG_ADDR(BASE_REG_GMAC1_PA, 0x31), val << 11, GMAC_TX_PHASE_MSK);
                    }
#endif
                    break;
                case SPEED_10:
                    OUTREGMSK16(GET_REG_ADDR(BASE_REG_GMAC1_PA, 0x31), 0x0280, ~GMAC_SEL_MSK);
                    break;
            }
#if (DYN_PHASE_CALB)
            if (NEED_CALB)
            {
                // reference bit 2,3 to calibrate tx clk phase
                val = (INREG16(GET_REG_ADDR(BASE_REG_GMAC1_PA, 0x31)) & GMAC_CALB_MSK) >> 2;
                // only update bit 11.12
                OUTREGMSK16(GET_REG_ADDR(BASE_REG_GMAC1_PA, 0x31), val << 11, GMAC_TX_PHASE_MSK);
            }
#endif
        }
    }
    else
    {
        if (dwmac->interface == PHY_INTERFACE_MODE_RMII)
        {
            switch (speed)
            {
                case SPEED_1000:
                    OUTREGMSK16(GET_REG_ADDR(BASE_REG_GMAC0_PA, 0x31), 0x0000, ~GMAC_SEL_MSK);
                    break;
                case SPEED_100:
                    OUTREGMSK16(GET_REG_ADDR(BASE_REG_GMAC0_PA, 0x31), 0x2200, ~GMAC_SEL_MSK);
                    break;
                case SPEED_10:
                    OUTREGMSK16(GET_REG_ADDR(BASE_REG_GMAC0_PA, 0x31), 0x2200, ~GMAC_SEL_MSK);
                    break;
            }
        }
        else if (dwmac->interface == PHY_INTERFACE_MODE_RGMII)
        {
            switch (speed)
            {
                case SPEED_1000:
                    OUTREGMSK16(GET_REG_ADDR(BASE_REG_GMAC0_PA, 0x31), 0x0000, ~GMAC_SEL_MSK);
                    break;
                case SPEED_100:
                    // rx use negedge, without overwrite bit 2,3
                    OUTREGMSK16(GET_REG_ADDR(BASE_REG_GMAC0_PA, 0x31), 0x140, ~GMAC_SEL_MSK);
#if (DYN_PHASE_CALB == 0)
                    if (NEED_CALB)
                    {
                        // reference bit 2,3 to calibrate tx clk phase
                        val = (INREG16(GET_REG_ADDR(BASE_REG_GMAC0_PA, 0x31)) & GMAC_CALB_MSK) >> 2;
                        // only update bit 11.12
                        OUTREGMSK16(GET_REG_ADDR(BASE_REG_GMAC0_PA, 0x31), val << 11, GMAC_TX_PHASE_MSK);
                    }
#endif
                    break;
                case SPEED_10:
                    OUTREGMSK16(GET_REG_ADDR(BASE_REG_GMAC0_PA, 0x31), 0x0280, ~GMAC_SEL_MSK);
                    break;
            }
#if (DYN_PHASE_CALB)
            if (NEED_CALB)
            {
                // reference bit 2,3 to calibrate tx clk phase
                val = (INREG16(GET_REG_ADDR(BASE_REG_GMAC0_PA, 0x31)) & GMAC_CALB_MSK) >> 2;
                // only update bit 11.12
                OUTREGMSK16(GET_REG_ADDR(BASE_REG_GMAC0_PA, 0x31), val << 11, GMAC_TX_PHASE_MSK);
            }
#endif
        }
    }
}

u32 dwmac_to_sstar_rebase(void __iomem *addr, int id)
{
    u32           bank = 0, axi_offset = 0, offset = 0;
    u32           bank_offset;
    unsigned long rebase_addr;

    offset     = addr - GBase[id];
    axi_offset = offset >> 8;

    switch (axi_offset)
    {
        case 0x00:
        case 0x01:
            axi_offset = 0x00;
            bank       = GBank[id][0];
            break;
        case 0x02:
        case 0x03:
            axi_offset = 0x02;
            bank       = GBank[id][1];
            break;
        case 0x07:
        case 0x08:
            axi_offset = 0x07;
            bank       = GBank[id][2];
            break;
        case 0x09:
        case 0x0a:
            axi_offset = 0x09;
            bank       = GBank[id][3];
            break;
        case 0x0b:
        case 0x0c:
            axi_offset = 0x0b;
            bank       = GBank[id][4];
            break;
        case 0x0d:
        case 0x0e:
            axi_offset = 0x0d;
            bank       = GBank[id][5];
            break;
        case 0x0f:
        case 0x10:
            axi_offset = 0x0f;
            bank       = GBank[id][6];
            break;
        case 0x11:
        case 0x12:
            axi_offset = 0x11;
            bank       = GBank[id][7];
            break;
    };

    bank_offset = offset - (axi_offset << 8);
    rebase_addr = rebase_based + (bank << 9) + bank_offset;

    rbs_wrte_dbg("[RBS] input write addr: 0x%px\r\n", addr);
    rbs_wrte_dbg("[RBS] rebse write addr= 0x%lx \r\n", rebase_addr);
    rbs_read_dbg("[RBS] input read addr: 0x%px\r\n", addr);
    rbs_read_dbg("[RBS] rebse read addr= 0x%lx \r\n", rebase_addr);

    return rebase_addr;
}

#ifdef CONFIG_SSTAR_SNPS_GMAC_CODING_OPTIMIZE
u32 mhal_axi_2_sstar_tbl[4][32];

void mhal_gmac_axi_tbl(int gmacId, int *bank, int nbank)
{
    u32 *tbl  = &mhal_axi_2_sstar_tbl[gmacId][0];
    u32  base = (u32)rebase_based;

    memset(tbl, 0xff, sizeof(mhal_axi_2_sstar_tbl[0]));
    tbl[0] = base + (bank[0] << 9) + 0x000;
    tbl[1] = base + (bank[0] << 9) + 0x100;

    tbl[2] = base + (bank[1] << 9) + 0x000;
    tbl[3] = base + (bank[1] << 9) + 0x100;

    tbl[7] = base + (bank[2] << 9) + 0x000;
    tbl[8] = base + (bank[2] << 9) + 0x100;

    tbl[9]  = base + (bank[3] << 9) + 0x000;
    tbl[10] = base + (bank[3] << 9) + 0x100;

    tbl[11] = base + (bank[4] << 9) + 0x000;
    tbl[12] = base + (bank[4] << 9) + 0x100;

    tbl[13] = base + (bank[5] << 9) + 0x000;
    tbl[14] = base + (bank[5] << 9) + 0x100;

    tbl[15] = base + (bank[6] << 9) + 0x000;
    tbl[16] = base + (bank[6] << 9) + 0x100;

    tbl[17] = base + (bank[7] << 9) + 0x000;
    tbl[18] = base + (bank[7] << 9) + 0x100;
}

int sstar_gmac_int_owner(int gmacId)
{
    return INREG16(GET_REG_ADDR(BASE_REG_NET_GP_CTRL_PA, 0x12)) & (1 << gmacId);
}
#else
u32 prior_to_rebase(void __iomem *addr)
{
    if (GBase[GMAC1] == 0)
        return dwmac_to_sstar_rebase(addr, GMAC0);
    else
    {
        if (addr >= GBase[GMAC1])
            return dwmac_to_sstar_rebase(addr, GMAC1);
        else if (addr >= GBase[GMAC0] && GBase[GMAC0] != 0)
            return dwmac_to_sstar_rebase(addr, GMAC0);
        else
        {
            printk("[%s] Error \r\n", __func__);
            return -1;
        }
    }
    return 0;
}
#endif

#if (DYN_PHASE_CALB)
void lp_send_pkt(struct sstar_dwmac *dwmac, char *p, int size)
{
    struct sk_buff *   skb = NULL;
    struct sk_buff *   trailer;
    struct net_device *dev;

    dev = dev_get_drvdata(dwmac->dev);

    skb = alloc_skb(size + ETH_HLEN + 2, GFP_ATOMIC);
    memcpy(skb_put(skb, size), p, size);
    skb_reset_network_header(skb);

    skb->dev      = dev;
    skb->pkt_type = PACKET_OUTGOING;
    skb_cow_data(skb, 1, &trailer);

    dev->netdev_ops->ndo_start_xmit(skb, dev);
    // dev_queue_xmit(skb);
}

int dyn_loopback_test(struct sstar_dwmac *dwmac, u16 phase)
{
    char                gmac_calb_pass = 0;
    unsigned long       old_rxpkt_cnt, old_rxpkt_bytes;
    int                 i;
    u32                 reg, txcoalbk;
    struct net_device * ndev = dev_get_drvdata(dwmac->dev);
    struct stmmac_priv *priv = netdev_priv(ndev);

    unsigned char packet_fill[] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x08, 0x06,
                                   0x00, 0x01, 0x08, 0x00, 0x06, 0x04, 0x00, 0x01, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05,
                                   0xc0, 0xa8, 0x01, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xc0, 0xa8, 0x01, 0x0f};

    if (dwmac->id == GMAC1)
    {
        reg = GET_REG_ADDR(BASE_REG_GMAC1_PA, 0x31);
    }
    else
    {
        reg = GET_REG_ADDR(BASE_REG_GMAC0_PA, 0x31);
    }

    // only update bit 11.12
    OUTREGMSK16(reg, phase, GMAC_TX_PHASE_MSK);
    // printk("TX Phase : 0x%08x \r\n", INREG16(reg));

    old_rxpkt_cnt   = priv->xstats.rx_pkt_n;
    old_rxpkt_bytes = priv->dev->stats.rx_bytes;
    txcoalbk        = priv->tx_coal_frames;

    priv->tx_coal_frames = 0;
    lp_send_pkt(dwmac, packet_fill, sizeof(packet_fill));
    priv->tx_coal_frames = txcoalbk;

    for (i = 0; i < 6; i++)
    {
        if (old_rxpkt_cnt + 1 == priv->xstats.rx_pkt_n && old_rxpkt_bytes + 60 == priv->dev->stats.rx_bytes)
        {
            gmac_calb_pass = 1;
            break;
        }
        mdelay(1 * CONFIG_SSTAR_GMAC_DELAY_SCALE);
    }

    if (gmac_calb_pass == 1)
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

void mhal_gmac_dyncalibrat(struct stmmac_priv *priv)
{
#define WAIT_LINK_UP_TIME  5000 // 5s
#define WAIT_LINK_UP_DELAY 100  // 100 ms

    struct plat_stmmacenet_data *plat_dat;
    struct sstar_dwmac *         dwmac;
    struct phy_device *          phydev;
    u16                          txc_phase = TXC_PHASE_0;
    int                          ret;
    u16                          val;
    u16                          reg0_bk;
    u32                          reg;
    u16                          phy_speed       = 0x2000;
    u32                          wait_linkup_cnt = WAIT_LINK_UP_TIME / WAIT_LINK_UP_DELAY;

    plat_dat = priv->plat;
    dwmac    = plat_dat->bsp_priv;
    phydev   = priv->dev->phydev;

    if (dwmac->interface != PHY_INTERFACE_MODE_RGMII)
    {
        return;
    }

    if (dwmac->id == GMAC1)
    {
        reg = GET_REG_ADDR(BASE_REG_GMAC1_PA, 0x31);
    }
    else
    {
        reg = GET_REG_ADDR(BASE_REG_GMAC0_PA, 0x31);
    }

    val = (INREG16(reg) & GMAC_RESV_SPEED_MSK) >> 14;
    // Check if we need calibration for current speed
    switch (priv->speed)
    {
        case SPEED_1000:
            if (val == 1)
                return;
            phy_speed = 0x40;
            break;
        case SPEED_100:
            if (val == 2)
                return;
            phy_speed = 0x2000;
            break;
        case SPEED_10:
            if (val == 3)
                return;
            phy_speed = 0x0;
            break;
    }

    // printk("Start mhal_gmac_dyncalibrat, phydev->phy_id = %x , mdio.addr = %u\n", phydev->phy_id, phydev->mdio.addr);

    ret = mdiobus_read(priv->mii, phydev->mdio.addr, MII_BMCR);
    if (ret < 0)
        printk("%s %d err=0x%x\n", __FUNCTION__, __LINE__, ret);

    reg0_bk = ret;

    mutex_lock(&phydev->lock);

    // Raising loopback bit
    ret = mdiobus_write(priv->mii, phydev->mdio.addr, MII_BMCR, phy_speed | BMCR_LOOPBACK);
    if (ret < 0)
        printk("%s %d err=0x%x\n", __FUNCTION__, __LINE__, ret);

    // mdelay(20);//Need Test
    mdelay(100 * CONFIG_SSTAR_GMAC_DELAY_SCALE);

    while (1)
    {
        if (dyn_loopback_test(dwmac, TXC_PHASE_0))
        {
            txc_phase = TXC_PHASE_0;
            break;
        }
        if (dyn_loopback_test(dwmac, TXC_PHASE_180))
        {
            txc_phase = TXC_PHASE_180;
            break;
        }
        if (dyn_loopback_test(dwmac, TXC_PHASE_90))
        {
            txc_phase = TXC_PHASE_90;
            break;
        }
        if (dyn_loopback_test(dwmac, TXC_PHASE_270))
        {
            txc_phase = TXC_PHASE_270;
            break;
        }
        printk("\r\n\r\n");
        //  Clear loopback bit
        ret = mdiobus_write(priv->mii, phydev->mdio.addr, MII_BMCR, reg0_bk);
        mutex_unlock(&phydev->lock);
        return;
    }

    OUTREGMSK16(reg, txc_phase >> 9, GMAC_CALB_MSK);

    switch (priv->speed)
    {
        case SPEED_1000:
            OUTREGMSK16(reg, 1 << 14, GMAC_RESV_SPEED_MSK);
            break;
        case SPEED_100:
            OUTREGMSK16(reg, 2 << 14, GMAC_RESV_SPEED_MSK);
            break;
        case SPEED_10:
            OUTREGMSK16(reg, 3 << 14, GMAC_RESV_SPEED_MSK);
            break;
    }

    //  Clear loopback bit
    ret = mdiobus_write(priv->mii, phydev->mdio.addr, MII_BMCR, reg0_bk);
    if (ret < 0)
        printk("%s %d err=0x%x\n", __FUNCTION__, __LINE__, ret);

    mhal_gmac_tx_clk_pad_sel(dwmac, priv->speed);

    while (wait_linkup_cnt)
    {
        ret = mdiobus_read(priv->mii, phydev->mdio.addr, MII_BMSR);
        if (ret < 0)
            printk("%s %d err=0x%x\n", __FUNCTION__, __LINE__, ret);
        if (ret & BMSR_LSTATUS)
            break;
        msleep(WAIT_LINK_UP_DELAY * CONFIG_SSTAR_GMAC_DELAY_SCALE);
        wait_linkup_cnt--;
    }

    mutex_unlock(&phydev->lock);
    // printk("loppback done! state = %u, remain time = %u ms\n", phydev->state, wait_linkup_cnt * WAIT_LINK_UP_DELAY);
}
#endif
