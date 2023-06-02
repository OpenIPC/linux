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

#include <mach/io.h>
#include "pcie_hi3519av100.h"

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

/*
 * ret:
 */
static int __arch_pcie_info_setup(struct pcie_info *info, int *controllers_nr)
{
    unsigned int pcie_mem_size;
    unsigned int pcie_cfg_size;
    unsigned int pcie_dbi_base;
    unsigned int pcie_ep_conf_base;
    unsigned int pcie_contrl;
    int err;

    /* Get pcie deice memory size */
    err = of_property_read_u32(g_of_node, "dev_mem_size", &pcie_mem_size);
    if (err) {
        pcie_error("No dev_mem_size found!");
        return -EINVAL;
    }

    /* Get pcie config space size*/
    err = of_property_read_u32(g_of_node, "dev_conf_size", &pcie_cfg_size);
    if (err) {
        pcie_error("No dev_conf_size founcd!");
        return -EINVAL;
    }

    /* Get pcie dib base address */
    err = of_property_read_u32(g_of_node, "pcie_dbi_base", &pcie_dbi_base);
    if (err) {
        pcie_error("No pcie_dbi_base found!");
        return -EINVAL;
    }

    /* Get pcie device config base address */
    err = of_property_read_u32(g_of_node, "ep_conf_base", &pcie_ep_conf_base);
    if (err) {
        pcie_error("No ep_conf_base found!");
        return -EINVAL;
    }

    if ((pcie_mem_size > __128MB__) || (pcie_cfg_size > __128MB__)) {
        pcie_error(
            "Invalid parameter: pcie mem size[0x%x], pcie cfg size[0x%x]!",
            pcie_mem_size, pcie_cfg_size);
        return -EINVAL;
    }

    err = of_property_read_u32(g_of_node, "pcie_controller", &pcie_contrl);
    if (err) {
        pcie_error("No pcie_controller found!");
        return -EINVAL;
    }

    info->controller = pcie_contrl;

    /* RC configuration space */
    info->conf_base_addr = (unsigned int)ioremap_nocache(pcie_dbi_base,
                           __8KB__);
    if (!info->conf_base_addr) {
        pcie_error("Address mapping for RC dbi failed!");
        return -EIO;
    }

    /* Configuration space for all EPs */
    info->base_addr = (unsigned int)ioremap_nocache(pcie_ep_conf_base,
                      pcie_cfg_size);
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

    /* set pcie to gen 1*/
#ifdef PCIE_GEN1_ENABLE
    writel(0x1, dbi_base + 0x8BC);
    val = readl(dbi_base + 0x7C);
    val = ((val >> 4) << 4) | 0x1;
    writel(val, dbi_base + 0x7C);
#endif

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
