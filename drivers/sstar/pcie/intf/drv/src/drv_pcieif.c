/*
 * drv_pcieif.c- Sigmastar
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
#include "drv_pcieif.h"
#include "hal_pcieif.h"

struct ss_pcieif
{
    void __iomem *  dbi; /* dbi register address */
    void __iomem *  phy; /* phy register address */
    void __iomem *  mac; /* mac register address */
    CamOsSpinlock_t lock;
} ss_pcieif;

static struct ss_plat_pcieif
{
    struct ss_pcieif *intf; /* for each controller */
    u32               num;  /* number of controller */
} m_plat_pcieif = {
    .intf = NULL,
    .num  = 0,
};

u8 sstar_pcieif_num_of_intf(void)
{
    return m_plat_pcieif.num;
}
EXPORT_SYMBOL(sstar_pcieif_num_of_intf);

u16 sstar_pcieif_readw_phy(u8 id, u32 reg)
{
    struct ss_pcieif *intf = &m_plat_pcieif.intf[id];

    return readw(intf->phy + reg);
}
EXPORT_SYMBOL(sstar_pcieif_readw_phy);

void sstar_pcieif_writew_phy(u8 id, u32 reg, u16 val)
{
    struct ss_pcieif *intf = &m_plat_pcieif.intf[id];

    writew(val, intf->phy + reg);
}
EXPORT_SYMBOL(sstar_pcieif_writew_phy);

u16 sstar_pcieif_readw_mac(u8 id, u32 reg)
{
    struct ss_pcieif *intf = &m_plat_pcieif.intf[id];

    return readw(intf->mac + reg);
}
EXPORT_SYMBOL(sstar_pcieif_readw_mac);

void sstar_pcieif_writew_mac(u8 id, u32 reg, u16 val)
{
    struct ss_pcieif *intf = &m_plat_pcieif.intf[id];

    writew(val, intf->mac + reg);
}
EXPORT_SYMBOL(sstar_pcieif_writew_mac);

u32 sstar_pcieif_readl_dbi(u8 id, u32 reg)
{
    u32               val;
    u32               addr;
    struct ss_pcieif *intf = &m_plat_pcieif.intf[id];

    CamOsSpinLockIrqSave(&intf->lock);
    addr = hal_prog_dbi_addr(intf->mac, reg);
    val  = readl(intf->dbi + addr);
    hal_rst_dbi_addr(intf->mac);
    CamOsSpinUnlockIrqRestore(&intf->lock);

    return val;
}
EXPORT_SYMBOL(sstar_pcieif_readl_dbi);

u16 sstar_pcieif_readw_dbi(u8 id, u32 reg)
{
    u16 val;
    u32 addr;

    struct ss_pcieif *intf = &m_plat_pcieif.intf[id];

    CamOsSpinLockIrqSave(&intf->lock);
    addr = hal_prog_dbi_addr(intf->mac, reg);
    val  = readw(intf->dbi + addr);
    hal_rst_dbi_addr(intf->mac);
    CamOsSpinUnlockIrqRestore(&intf->lock);

    return val;
}
EXPORT_SYMBOL(sstar_pcieif_readw_dbi);

u8 sstar_pcieif_readb_dbi(u8 id, u32 reg)
{
    u8                val;
    u32               addr;
    struct ss_pcieif *intf = &m_plat_pcieif.intf[id];

    CamOsSpinLockIrqSave(&intf->lock);
    addr = hal_prog_dbi_addr(intf->mac, reg);
    val  = readb(intf->dbi + addr);
    hal_rst_dbi_addr(intf->mac);
    CamOsSpinUnlockIrqRestore(&intf->lock);

    return val;
}
EXPORT_SYMBOL(sstar_pcieif_readb_dbi);

void sstar_pcieif_writel_dbi(u8 id, u32 reg, u32 val)
{
    u32               addr;
    struct ss_pcieif *intf = &m_plat_pcieif.intf[id];

    CamOsSpinLockIrqSave(&intf->lock);
    addr = hal_prog_dbi_addr(intf->mac, reg);
    writel(val, intf->dbi + addr);
    readl(intf->dbi + addr); // dummy read to make sure x32 write has token effect
    hal_rst_dbi_addr(intf->mac);
    CamOsSpinUnlockIrqRestore(&intf->lock);
}
EXPORT_SYMBOL(sstar_pcieif_writel_dbi);

void sstar_pcieif_writew_dbi(u8 id, u32 reg, u16 val)
{
    u32               addr;
    struct ss_pcieif *intf = &m_plat_pcieif.intf[id];

    CamOsSpinLockIrqSave(&intf->lock);
    addr = hal_prog_dbi_addr(intf->mac, reg);
    writew(val, intf->dbi + addr);
    readw(intf->dbi + addr); // dummy read to make sure x32 write has token effect
    hal_rst_dbi_addr(intf->mac);
    CamOsSpinUnlockIrqRestore(&intf->lock);
}
EXPORT_SYMBOL(sstar_pcieif_writew_dbi);

void sstar_pcieif_writeb_dbi(u8 id, u32 reg, u8 val)
{
    u32               addr;
    struct ss_pcieif *intf = &m_plat_pcieif.intf[id];

    CamOsSpinLockIrqSave(&intf->lock);
    addr = hal_prog_dbi_addr(intf->mac, reg);
    writeb(val, intf->dbi + addr);
    readb(intf->dbi + addr); // dummy read to make sure x32 write has token effect
    hal_rst_dbi_addr(intf->mac);
    CamOsSpinUnlockIrqRestore(&intf->lock);
}
EXPORT_SYMBOL(sstar_pcieif_writeb_dbi);

void sstar_pcieif_start_link(u8 id)
{
    sstar_pcieif_writew_mac(id, REG_PCIE_MAC_CTRL, 0x0010);
}
EXPORT_SYMBOL(sstar_pcieif_start_link);

void sstar_pcieif_set_mode(u8 id, enum ss_pcieif_mode mode)
{
    if (mode == PCIE_IF_MODE_RC)
    {
        sstar_pcieif_writew_mac(id, REG_PCIE_DEVICE_TYPE, PCIE_DEVICE_RC);
        sstar_pcieif_writew_mac(id, REG_PCIE_IRQ2_MASK, ~(IRQ2_INTR_MSI));
    }
    else if (mode == PCIE_IF_MODE_EP)
    {
        sstar_pcieif_writew_mac(id, REG_PCIE_DEVICE_TYPE, PCIE_DEVICE_EP);
        sstar_pcieif_writew_mac(id, REG_PCIE_IRQ2_MASK, 0xFFFF);
    }
    sstar_pcieif_writew_phy(id, REG_SLV_READ_BYPASS, 0x0100); // slv_read_bypass
    sstar_pcieif_writew_mac(id, REG_SLV_WSTRB_MASK, 0xFFFF);
    sstar_pcieif_writew_phy(id, REG_PHYVREG_BPS, 1); // Bypass to 1.8V
}
EXPORT_SYMBOL(sstar_pcieif_set_mode);

void sstar_pcieif_clr_memwr_hit_intr_sta(u8 id)
{
    sstar_pcieif_writew_mac(id, REG_PCIE_IRQ2_CLR, IRQ2_INTR_ADDR_MATCH);
    sstar_pcieif_writew_mac(id, REG_PCIE_IRQ2_CLR, 0);
}
EXPORT_SYMBOL(sstar_pcieif_clr_memwr_hit_intr_sta);

bool sstar_pcieif_memwr_hit_intr_sta(u8 id)
{
    u16 status, mask;

    status = sstar_pcieif_readw_mac(id, REG_PCIE_IRQ2_STAT);
    mask   = sstar_pcieif_readw_mac(id, REG_PCIE_IRQ2_MASK);
    status &= ~(mask);
    return (status & IRQ2_INTR_ADDR_MATCH) ? true : false;
}
EXPORT_SYMBOL(sstar_pcieif_memwr_hit_intr_sta);

void sstar_pcieif_enable_memwr_hit_intr(u8 id, phys_addr_t addr)
{
    /* program memory write match address */
    sstar_pcieif_writew_mac(id, REG_WR_MATCH_ADDR0, addr & 0xFFFF);
    sstar_pcieif_writew_mac(id, REG_WR_MATCH_ADDR1, (addr >> 16) & 0xFFFF);
    sstar_pcieif_writew_mac(id, REG_WR_MATCH_ADDR2, upper_32_bits(addr) & 0x000F);
    /* clear wr_addr_match_int status */
    sstar_pcieif_clr_memwr_hit_intr_sta(id);
    /* unmask wr_addr_match_int */
    sstar_pcieif_writew_mac(id, REG_PCIE_IRQ2_MASK,
                            sstar_pcieif_readw_mac(id, REG_PCIE_IRQ2_MASK) & ~(IRQ2_INTR_ADDR_MATCH));
}
EXPORT_SYMBOL(sstar_pcieif_enable_memwr_hit_intr);

void sstar_pcieif_internalclk_en(u8 id, u8 ssc_enable)
{
    hal_internalclk_en(id, ssc_enable);
}
EXPORT_SYMBOL(sstar_pcieif_internalclk_en);

#ifdef CONFIG_SSTAR_FPGA_LPBK
static void _cr_data_in(u8 id, u16 val)
{
    sstar_pcieif_writew_phy(id, 0x48 * 2, 0x0000); // reg_cr_write = 0
    sstar_pcieif_writew_phy(id, 0x48 * 2, 0x0000); // reg_cr_write = 0
    sstar_pcieif_writew_phy(id, 0x48 * 2, 0x0000); // reg_cr_write = 0
    sstar_pcieif_writew_phy(id, 0x44 * 2, val);    // reg_cr_data_in
    sstar_pcieif_writew_phy(id, 0x48 * 2, 0x0001); // reg_cr_write = 1
    sstar_pcieif_writew_phy(id, 0x48 * 2, 0x0001); // reg_cr_write = 1
    sstar_pcieif_writew_phy(id, 0x48 * 2, 0x0001); // reg_cr_write = 1
}

void sstar_pcieif_loopback_link(u8 rc, u8 ep)
{
    sstar_pcieif_writew_mac(rc, REG_PCIE_MAC_CTRL, 0x0010); // PCIE0 reg_app_ltssm_enable
    sstar_pcieif_writew_mac(ep, REG_PCIE_MAC_CTRL, 0x0010); // PCIE1 reg_app_ltssm_enable

    _cr_data_in(rc, 0x0002);
    // Waiting for pcie0 smlh_ltssm_state changed to DETECT_ACT (0x01) state
    while ((sstar_pcieif_readw_phy(rc, 0x4A * 2) & 0x3F) != 0x01)
        ; // reg_cr_data_out

    _cr_data_in(rc, 0x0003);
    _cr_data_in(ep, 0x0002);
    // Waiting for pcie1 smlh_ltssm_state changed to DETECT_ACT (0x01) state
    while ((sstar_pcieif_readw_phy(ep, 0x4A * 2) & 0x3F) != 0x01)
        ; // reg_cr_data_out

    _cr_data_in(ep, 0x0003);
    while ((sstar_pcieif_readw_phy(rc, 0x4A * 2) & 0x3F) != 0x01)
        ; // reg_cr_data_out

    _cr_data_in(rc, 0x006F);
    _cr_data_in(rc, 0x406F);
    _cr_data_in(rc, 0x006F);
    // Waiting for pcie0 smlh_ltssm_state changed to POLL_ACTIVE (0x02) state
    while ((sstar_pcieif_readw_phy(rc, 0x4A * 2) & 0x3F) != 0x02)
        ; // reg_cr_data_out

    _cr_data_in(ep, 0x006F);
    _cr_data_in(ep, 0x406F);
    _cr_data_in(ep, 0x006F);
    // Waiting for pcie1 smlh_ltssm_state changed to POLL_ACTIVE (0x02) state
    while ((sstar_pcieif_readw_phy(rc, 0x4A * 2) & 0x3F) != 0x02)
        ; // reg_cr_data_out

    _cr_data_in(rc, 0x0003);
    _cr_data_in(rc, 0x4003);
    _cr_data_in(rc, 0x0003);
    _cr_data_in(ep, 0x0003);
    _cr_data_in(ep, 0x4003);
    _cr_data_in(ep, 0x0003);
    _cr_data_in(ep, 0x0303);
    _cr_data_in(rc, 0x0303);
    while ((sstar_pcieif_readw_phy(rc, 0x4A * 2) & 0x3F) != 0x0E)
        ; // reg_cr_data_out
    while ((sstar_pcieif_readw_phy(ep, 0x4A * 2) & 0x3F) != 0x0E)
        ; // reg_cr_data_out

    _cr_data_in(rc, 0x0393);
    _cr_data_in(rc, 0x0003);
    _cr_data_in(ep, 0x0393);
    _cr_data_in(ep, 0x0003);
    _cr_data_in(rc, 0x4003);
    while ((sstar_pcieif_readw_phy(rc, 0x4A * 2) & 0x3F) != 0x0D)
        ; // reg_cr_data_out
    _cr_data_in(rc, 0x0003);
    _cr_data_in(ep, 0x4003);
    while ((sstar_pcieif_readw_phy(ep, 0x4A * 2) & 0x3F) != 0x0D)
        ; // reg_cr_data_out

    _cr_data_in(ep, 0x0303);
    _cr_data_in(rc, 0x0303);
    while ((sstar_pcieif_readw_phy(rc, 0x4A * 2) & 0x3F) != 0x11)
        ; // reg_cr_data_out
    while ((sstar_pcieif_readw_phy(ep, 0x4A * 2) & 0x3F) != 0x11)
        ; // reg_cr_data_out
}
EXPORT_SYMBOL(sstar_pcieif_loopback_link);
#endif

static int ss_pcieif_probe(struct platform_device *pdev)
{
    u8                     i;
    struct resource *      res; /* Resource from DT */
    char                   reg_name[10];
    struct ss_plat_pcieif *plat = &m_plat_pcieif;
    struct device *        dev  = &pdev->dev;
    struct ss_pcieif *     intf;

    if (of_property_read_u32(dev->of_node, "num-controller", &plat->num))
    {
        dev_err(dev, "no num-controller property in DT\r\n");
        return -EINVAL;
    }
    dev_err(dev, "ss_pcieif_probe: num of controller %d\r\n", plat->num);

    plat->intf = devm_kzalloc(dev, sizeof(struct ss_pcieif) * plat->num, GFP_KERNEL);
    if (!plat->intf)
        return -ENOMEM;

    for (i = 0; i < plat->num; i++)
    {
        intf = &plat->intf[i];

        /* obtain reg address of dbi from DT */
        snprintf(reg_name, sizeof(reg_name), "pcie%d_dbi", i);
        res = platform_get_resource_byname(pdev, IORESOURCE_MEM, reg_name);
        if (!res)
        {
            dev_err(dev, "no %s reg in DT\r\n", reg_name);
            return -EINVAL;
        }
        intf->dbi = (void __iomem *)IO_ADDRESS(res->start);

        /* obtain reg address of phy from DT */
        snprintf(reg_name, sizeof(reg_name), "pcie%d_phy", i);
        res = platform_get_resource_byname(pdev, IORESOURCE_MEM, reg_name);
        if (!res)
        {
            dev_err(dev, "no %s reg in DT\r\n", reg_name);
            return -EINVAL;
        }
        intf->phy = (void __iomem *)IO_ADDRESS(res->start);

        /* obtain reg address of mac from DT */
        snprintf(reg_name, sizeof(reg_name), "pcie%d_mac", i);
        res = platform_get_resource_byname(pdev, IORESOURCE_MEM, reg_name);
        if (!res)
        {
            dev_err(dev, "no %s reg in DT\r\n", reg_name);
            return -EINVAL;
        }
        intf->mac = (void __iomem *)IO_ADDRESS(res->start);
        dev_dbg(dev, "id[%d] dbi 0x%px, phy 0x%px, mac 0x%px\r\n", i, intf->dbi, intf->phy, intf->mac);

        CamOsSpinInit(&intf->lock);
    }

    return 0;
}

static const struct of_device_id ss_pcieif_of_match[] = {
    {.compatible = "sstar,pcie-if"},
    {},
};

static struct platform_driver ss_pcieif_driver = {
    .driver =
        {
            .name           = "pcieif",
            .owner          = THIS_MODULE,
            .of_match_table = ss_pcieif_of_match,
        },
    .probe = ss_pcieif_probe,
};

static int __init ss_pcieif_init(void)
{
    return platform_driver_register(&ss_pcieif_driver);
}

subsys_initcall(ss_pcieif_init);
