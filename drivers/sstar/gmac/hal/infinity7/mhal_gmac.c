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
#define GMAC_MAX_GEAR          16
#define GMAC_DRIVING_REG_MASK  0x780
#define GMAC_DRIVING_REG_SHIFT 7

extern void *GBase[GMAC_NUM];
extern u32   GBank[GMAC_NUM][GMAC_BANK_NUM];

u8 GMAC_Driving_Setting[GMAC_NUM][GMAC_MAX_IO] = {{
                                                      7, // RGMII0_MDC
                                                      7, // RGMII0_MDIO
                                                      7, // RGMII0_TX_CLK
                                                      7, // RGMII0_TX_D0
                                                      7, // RGMII0_TX_D1
                                                      7, // RGMII0_TX_D2
                                                      7, // RGMII0_TX_D3
                                                      7  // RGMII0_TX_CTL
                                                  },
                                                  {
                                                      7, // RGMII1_MDC
                                                      7, // RGMII1_MDIO
                                                      7, // RGMII1_TX_CLK
                                                      7, // RGMII1_TX_D0
                                                      7, // RGMII1_TX_D1
                                                      7, // RGMII1_TX_D2
                                                      7, // RGMII1_TX_D3
                                                      7  // RGMII1_TX_CTL
                                                  }};

u8 GMAC_Driving_Offset[GMAC_NUM][GMAC_MAX_IO] = {{0x68, 0x69, 0x5A, 0x5B, 0x5C, 0x5D, 0x5E, 0x5F},
                                                 {0x78, 0x79, 0x6A, 0x6B, 0x6C, 0x6D, 0x6E, 0x6F}};

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
            OUTREGMSK16(GET_REG_ADDR(BASE_REG_PAD_GPIO2_PA, GMAC_Driving_Offset[gmacId][i]),
                        GMAC_Driving_Setting[gmacId][i] << GMAC_DRIVING_REG_SHIFT, GMAC_DRIVING_REG_MASK);
        }
    }
    else
    { // Update one IO by io_idx
        OUTREGMSK16(GET_REG_ADDR(BASE_REG_PAD_GPIO2_PA, GMAC_Driving_Offset[gmacId][io_idx]),
                    GMAC_Driving_Setting[gmacId][io_idx] << GMAC_DRIVING_REG_SHIFT, GMAC_DRIVING_REG_MASK);
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

void mhal_gmac_disable_padmux(void)
{
    OUTREGMSK16(GET_REG_ADDR(BASE_REG_PADTOP_PA, 0x41), 0x0000, 0x0003);
}

static void mhal_gmac_gpio_reset(struct sstar_dwmac *dwmac)
{
    u32 bank   = BASE_REG_PAD_GPIO2_PA;
    u32 offset = 0x66;

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
            // GPIO 231
            bank   = BASE_REG_PAD_GPIO2_PA;
            offset = 0x67;
            OUTREGMSK16(GET_REG_ADDR(BASE_REG_PADTOP_PA, 0x42), 0x2000, 0x2000);
        }
        else
        {
            // GPIO 230
            bank   = BASE_REG_PAD_GPIO2_PA;
            offset = 0x66;
            OUTREGMSK16(GET_REG_ADDR(BASE_REG_PADTOP_PA, 0x42), 0x1000, 0x1000);
        }
#endif
    }
    else // if(dwmac->id == GMAC1)
    {
#ifdef GMAC_FPGA
        OUTREGMSK16(GET_REG_ADDR(BASE_REG_GMAC1_PA, 0x31), 0x0000, 0x000F);
        mdelay(1);
        OUTREGMSK16(GET_REG_ADDR(BASE_REG_GMAC1_PA, 0x31), 0x0002, 0x000F);
#else
        if (dwmac->reset_io == 1)
        {
            // GPIO 247
            bank   = BASE_REG_PAD_GPIO2_PA;
            offset = 0x77;
            OUTREGMSK16(GET_REG_ADDR(BASE_REG_PADTOP_PA, 0x43), 0x2000, 0x2000);
        }
        else
        {
            // GPIO 246
            bank   = BASE_REG_PAD_GPIO2_PA;
            offset = 0x76;
            OUTREGMSK16(GET_REG_ADDR(BASE_REG_PADTOP_PA, 0x43), 0x1000, 0x1000);
        }
#endif
    }

    // Output Low
    OUTREGMSK16(GET_REG_ADDR(bank, offset), 0x0008, 0x000F);
    mdelay(20);
    // Output High
    OUTREGMSK16(GET_REG_ADDR(bank, offset), 0x000A, 0x000F);
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

    mhal_gmac_update_driving(dwmac->id, GAMC_DRIVING_UPDATEALL);

    if (dwmac->id == GMAC1)
    {
        // padtop 103c -> RGMII 1.8V
        // OUTREG16(GET_REG_ADDR(BASE_REG_PADTOP_PA,0x04), (INREG16(GET_REG_ADDR(BASE_REG_PADTOP_PA,0x04)) | 0x02));

        printk("[%s][%d] SS GMAC1 Setting : Interface=%u\r\n", __func__, __LINE__, dwmac->interface);

        if (dwmac->interface == PHY_INTERFACE_MODE_MII)
        {
            OUTREG16(GET_REG_ADDR(BASE_REG_GMAC1_PA, 0x01), 0x0000);
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
            // RX clock (REFCLK/2) invert : Using falling edge of RX 25MHz clock to pack 4bit data for DWMAC
            OUTREGMSK16(GET_REG_ADDR(BASE_REG_GMAC1_PA, 0x02), BIT7, BIT7);
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
            // RX clock (REFCLK/2) invert : Using falling edge of RX 25MHz clock to pack 4bit data for DWMAC
            OUTREGMSK16(GET_REG_ADDR(BASE_REG_GMAC0_PA, 0x02), BIT7, BIT7);
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
}

void mhal_gmac_tx_clk_pad_sel(struct sstar_dwmac *dwmac, unsigned int speed)
{
    // no need
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
