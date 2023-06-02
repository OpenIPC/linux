/*
 * Copyright (c) 2017-2018 HiSilicon Technologies Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef CONFIG_ARM64
#include <mach/io.h>
#endif
#include <mach/platform.h>
#include "pcie_hi3559av100.h"

static void *dbi_base;
static int __arch_pcie_info_setup(struct pcie_info *info, int *controllers_nr);
static int __arch_pcie_sys_init(struct pcie_info *info);
static void __arch_pcie_info_release(struct pcie_info *info);

struct pcie_iatu iatu_table[] = {
    {
        .viewport   = 0,
        .region_ctrl_1  = 0x00000004,
        .region_ctrl_2  = 0x90000000,
        .lbar           = PCIE_EP_CONF_BASE + (1 << 20),
        .ubar           = 0x0,
        .lar            = PCIE_EP_CONF_BASE + (2 << 20) - 1,
        .ltar           = 0x01000000,
        .utar           = 0x00000000,
    },
    {
        .viewport       = 1,
        .region_ctrl_1  = 0x00000005,
        .region_ctrl_2  = 0x90000000,
        .lbar           = PCIE_EP_CONF_BASE + (2 << 20),
        .ubar           = 0x0,
        .lar            = PCIE_EP_CONF_BASE + (__128MB__ - 1),
        .ltar           = 0x02000000,
        .utar           = 0x00000000,
    },
};

static void __arch_config_iatu_tbl(struct pcie_info *info,
                                   struct pci_sys_data *sys)
{
    int i;
    void __iomem *config_base = (void __iomem *)info->conf_base_addr;
    struct pcie_iatu *ptable = iatu_table;
    int table_size = ARRAY_SIZE(iatu_table);

    for (i = 0; i < table_size; i++) {
        writel((ptable + i)->viewport, config_base + 0x900);
        writel((ptable + i)->lbar, config_base + 0x90c);
        writel((ptable + i)->ubar, config_base + 0x910);
        writel((ptable + i)->lar,  config_base + 0x914);
        writel((ptable + i)->ltar, config_base + 0x918);
        writel((ptable + i)->utar, config_base + 0x91c);
        writel((ptable + i)->region_ctrl_1, config_base + 0x904);
        writel((ptable + i)->region_ctrl_2, config_base + 0x908);
    }

}

static inline int __arch_check_pcie_link(struct pcie_info *info)
{
    int val;

    val = readl(dbi_base + PCIE_SYS_STATE0);
    return ((val & (1 << PCIE_XMLH_LINK_UP))
            && (val & (1 << PCIE_RDLH_LINK_UP))) ? 1 : 0;
}

static int  __arch_get_port_nr(void)
{
    unsigned int val, mode;
    int nr;
    void *pcie_sys_stat;
    unsigned int sys_ctrl_base;

    /* Get sys ctrl  base address */
    of_property_read_u32(g_of_node, "sys_ctrl_base", &sys_ctrl_base);

    pcie_sys_stat = ioremap_nocache(sys_ctrl_base + REG_SC_STAT, sizeof(int));
    if (!pcie_sys_stat) {
        pr_err("ioremap pcie sys status register failed!\n");
        return 0;
    }

    val = readl(pcie_sys_stat);
    mode = (val >> 12) & 0x3;
    switch (mode) {
        case 0x1:
            nr = 1;
            break;

        case 0x0:
            nr = 2;
            break;

        default:
            nr = 0;
            break;
    }

    iounmap(pcie_sys_stat);

    return nr;
}

/*
 * ret:
 */
static int __arch_pcie_info_setup(struct pcie_info *info, int *controllers_nr)
{
    unsigned int mem_size;
    unsigned int cfg_size;
    int nr;

    /* Get pcie deice memory size */
    of_property_read_u32(g_of_node, "dev_mem_size", &mem_size);

    /* Get pcie config space size*/
    of_property_read_u32(g_of_node, "dev_conf_size", &cfg_size);

    nr = __arch_get_port_nr();
    if (!nr) {
        pr_err("Pcie port number: 0\n");
        *controllers_nr = 0;
        return -EINVAL;
    }

    if ((mem_size > __256MB__) || (cfg_size > __256MB__)) {
        pcie_error(
            "Invalid parameter: pcie mem size[0x%x], pcie cfg size[0x%x]!",
            mem_size, cfg_size);
        return -EINVAL;
    }

    info->controller = 0;

    /* RC configuration space */
    info->conf_base_addr = (unsigned long)ioremap_nocache(PCIE_DBI_BASE,
                           __8KB__);
    if (!info->conf_base_addr) {
        pcie_error("Address mapping for RC dbi failed!");
        return -EIO;
    }

    /* Configuration space for all EPs */
    info->base_addr = (unsigned long)ioremap_nocache(PCIE_EP_CONF_BASE,
                      cfg_size);
    if (!info->base_addr) {
        iounmap((void *)info->conf_base_addr);
        pcie_error("Address mapping for EPs cfg failed!");
        return -EIO;
    }

    return 0;

}

static void __arch_pcie_info_release(struct pcie_info *info)
{
    if (info->base_addr) {
        iounmap((void *)info->base_addr);
    }

    if (info->conf_base_addr) {
        iounmap((void *)info->conf_base_addr);
    }
}

static int __arch_pcie_sys_init(struct pcie_info *info)
{
    unsigned int val;
    void *crg_base = (void *)ioremap_nocache(PERI_CRG_BASE, __8KB__);
    void *misc_base = (void *)ioremap_nocache(MISC_CTRL_BASE, __4KB__);
    void *sys_base = (void *)ioremap_nocache(SYS_CTRL_BASE, __4KB__);

    dbi_base = (void *)info->conf_base_addr;

    /*
     * Disable PCIE
     */
    val = readl(dbi_base + PCIE_SYS_CTRL7);
    val &= (~(1 << PCIE_APP_LTSSM_ENBALE));
    writel(val, dbi_base + PCIE_SYS_CTRL7);

    /*
     * Reset
     */
    val = readl(crg_base + PERI_CRG99);
    val |= (1 << PCIE_X2_SRST_REQ);
    writel(val, crg_base + PERI_CRG99);

    /*
     * Retreat from the reset state
     */
    udelay(500);
    val = readl(crg_base + PERI_CRG99);
    val &= ~(1 << PCIE_X2_SRST_REQ);
    writel(val, crg_base + PERI_CRG99);
    mdelay(10);

    val = readl(sys_base + SYS_SATA);
    if((val & (0x3 << PCIE_MODE)) == 0) {
        /*X2 select phy reset from crg*/
        val = readl(crg_base + PERI_CRG98);
        val |= (0x1 << phy1_srs_req_sel) | (0x1 << phy0_srs_req_sel);
        writel(val, crg_base + PERI_CRG98);
        mdelay(10);

        /*X2 reset phy reset*/
        val = readl(crg_base + PERI_CRG98);
        val |= ((0x1 << phy1_srs_req) | (0x1 << phy0_srs_req));
        writel(val, crg_base + PERI_CRG98);
        udelay(500);

        /*X2 release phy reset*/
        val = readl(crg_base + PERI_CRG98);
        val &= ((~(0x1 << phy1_srs_req)) & (~(0x1 << phy0_srs_req)));
        writel(val, crg_base + PERI_CRG98);

        /*
         * X2 seperate_rate=1
         */
        writel(0x90f, misc_base + MISC_CTRL5);
        writel(0x94f, misc_base + MISC_CTRL5);
        writel(0x90f, misc_base + MISC_CTRL5);
        writel(0x0, misc_base + MISC_CTRL5);
        writel(0x92f, misc_base + MISC_CTRL5);
        writel(0x96f, misc_base + MISC_CTRL5);
        writel(0x92f, misc_base + MISC_CTRL5);
        writel(0x0, misc_base + MISC_CTRL5);
        mdelay(10);

        /*
         * X2 split_cp_dis
         */
        writel(0xd11, misc_base + MISC_CTRL5);
        writel(0xd51, misc_base + MISC_CTRL5);
        writel(0xd11, misc_base + MISC_CTRL5);
        writel(0x0, misc_base + MISC_CTRL5);
        writel(0xd31, misc_base + MISC_CTRL5);
        writel(0xd71, misc_base + MISC_CTRL5);
        writel(0xd31, misc_base + MISC_CTRL5);
        writel(0x0, misc_base + MISC_CTRL5);
        mdelay(10);
    } else {

        /*X1 select phy reset from crg*/
        val = readl(crg_base + PERI_CRG98);
        val |= (0x1 << phy0_srs_req_sel);
        writel(val, crg_base + PERI_CRG98);
        mdelay(10);

        /*X1 reset phy reset*/
        val = readl(crg_base + PERI_CRG98);
        val |= (0x1 << phy0_srs_req);
        writel(val, crg_base + PERI_CRG98);
        udelay(500);

        /*X1 release phy reset*/
        val = readl(crg_base + PERI_CRG98);
        val &= ~(0x1 << phy0_srs_req);
        writel(val, crg_base + PERI_CRG98);

        /*
         * X1 seperate_rate=1
         */
        writel(0x90f, misc_base + MISC_CTRL5);
        writel(0x94f, misc_base + MISC_CTRL5);
        writel(0x90f, misc_base + MISC_CTRL5);
        writel(0x0, misc_base + MISC_CTRL5);
        mdelay(10);

        /*
         * X1 split_cp_dis
         */
        writel(0xd11, misc_base + MISC_CTRL5);
        writel(0xd51, misc_base + MISC_CTRL5);
        writel(0xd11, misc_base + MISC_CTRL5);
        writel(0x0, misc_base + MISC_CTRL5);
        mdelay(10);

    };

    /*
     * PCIE RC work mode
     */
    val = readl(dbi_base + PCIE_SYS_CTRL0);
    val &= (~(0xf << PCIE_DEVICE_TYPE));
    val |= (PCIE_WM_RC << PCIE_DEVICE_TYPE);
    writel(val, dbi_base + PCIE_SYS_CTRL0);

    /*
     * Enable clk
     */
    val = readl(crg_base + PERI_CRG99);
    val |= ((1 << PCIE_X2_BUS_CKEN)
            | (1 << PCIE_X2_SYS_CKEN)
            | (1 << PCIE_X2_PIPE_CKEN)
            | (1 << PCIE_X2_AUX_CKEN));
    writel(val, crg_base + PERI_CRG99);

    mdelay(10);

    /*
     *  * Set PCIe support the identification Board card
     */
    val = readl(dbi_base + PCI_CARD);
    val |= (1 << 3);
    writel(val, dbi_base + PCI_CARD);
    mdelay(10);

    /*
     * Set PCIE controller class code to be PCI-PCI bridge device
     */
    val = readl(dbi_base + PCI_CLASS_REVISION);
    val &= ~(0xffffff00);
    val |= (0x60400 << 8);
    writel(val, dbi_base + PCI_CLASS_REVISION);
    udelay(1000);

    /*
     * Enable controller
     */
    val = readl(dbi_base + PCIE_SYS_CTRL7);
    val |= (1 << PCIE_APP_LTSSM_ENBALE);
    writel(val, dbi_base + PCIE_SYS_CTRL7);
    udelay(1000);

    val = readl(dbi_base + PCI_COMMAND);
    val |= 7;
    writel(val, dbi_base + PCI_COMMAND);

#ifdef CONFIG_ENABLE_PCIE_1
    /* set pcie to gen 1*/
    writel(0x1, dbi_base + 0x8BC);
    val = readl(dbi_base + 0x7C);
    val = ((val >> 4) << 4) | 0x1;
    writel(val, dbi_base + 0x7C);
#endif

    iounmap(misc_base);
    iounmap(crg_base);
    return 0;
}

static void __arch_pcie_sys_exit(void)
{
    unsigned int val;
    void *crg_base = (void *)ioremap_nocache(PERI_CRG_BASE, __8KB__);

    /*
     * Disable PCIE
     */
    val = readl(dbi_base + PCIE_SYS_CTRL7);
    val &= (~(1 << PCIE_APP_LTSSM_ENBALE));
    writel(val, dbi_base + PCIE_SYS_CTRL7);

    /*
     * Reset
     */
    val = readl(crg_base + PERI_CRG99);
    val |= (1 << PCIE_X2_SRST_REQ);
    writel(val, crg_base + PERI_CRG99);

    udelay(1000);

    /*
     * Disable clk
     */
    val = readl(crg_base + PERI_CRG99);
    val &= (~(1 << PCIE_X2_AUX_CKEN));
    val &= (~(1 << PCIE_X2_PIPE_CKEN));
    val &= (~(1 << PCIE_X2_SYS_CKEN));
    val &= (~(1 << PCIE_X2_BUS_CKEN));
    writel(val, crg_base + PERI_CRG99);

    iounmap(crg_base);

    udelay(1000);
}
