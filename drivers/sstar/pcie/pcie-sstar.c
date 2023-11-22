/*
 * pcie-sstar.c - Sigmastar
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
#include <linux/delay.h>
//#include <linux/gpio.h>
#include <linux/interrupt.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/of_device.h>
#include <linux/of_gpio.h>
#include <linux/of_fdt.h>
#include <linux/pci.h>
#include <linux/platform_device.h>
#include <linux/resource.h>
//#include <linux/signal.h>
#include <linux/types.h>
#include <linux/of_clk.h>

#include "pcie-designware.h"
#include "ms_platform.h"
#include "drv_pcieif.h"
#include "pcie-sstar.h"

struct dw_plat_pcie_of_data
{
    enum dw_pcie_device_mode mode;
};

enum dw_pcie_device_mode gpcie0_mode = DW_PCIE_UNKNOWN_TYPE;
enum dw_pcie_device_mode gpcie1_mode = DW_PCIE_UNKNOWN_TYPE;

static bool use_internal_clk_pcie0 = false;
static bool use_internal_clk_pcie1 = false;
static bool pcie0_ssc_enable       = false;
static bool pcie1_ssc_enable       = false;

static const struct of_device_id dw_plat_pcie_of_match[];

static bool gpcie0_boot = false;
static bool gpcie1_boot = false;

static void dw_plat_set_num_vectors(struct pcie_port *pp)
{
    pp->num_vectors = MAX_MSI_IRQS;
}

static int sstar_pcie_start_link(struct dw_pcie *pci)
{
    struct dw_plat_pcie *plat = dev_get_drvdata(pci->dev);

    sstar_pcieif_start_link(plat->id);
    return 0;
}

static u64 sstar_pcie_cpu_addr_fixup(struct dw_pcie *pci, u64 cpu_addr)
{
    struct dw_plat_pcie *plat = dev_get_drvdata(pci->dev);
    struct pcie_port *   pp   = &pci->pp;
    struct dw_pcie_ep *  ep   = &pci->ep;

    switch (plat->mode)
    {
        case DW_PCIE_RC_TYPE:
            return cpu_addr - pp->cfg0_base;
        case DW_PCIE_EP_TYPE:
            return cpu_addr - ep->phys_base;
        default:
            dev_err(pci->dev, "UNKNOWN device type\n");
    }
    return cpu_addr;
}

static u32 sstar_pcie_read_dbi(struct dw_pcie *pci, void __iomem *base, u32 reg, size_t size)
{
    struct dw_plat_pcie *plat = dev_get_drvdata(pci->dev);

    if ((uintptr_t)reg & (size - 1))
    {
        dev_err(pci->dev, "sstar_pcie_read_dbi address failed\n");
        return 0;
    }

    if (((u64)base) == ((u64)(pci->atu_base)))
        reg |= DEFAULT_DBI_ATU_OFFSET;
    else if (((u64)base) == ((u64)(pci->dbi_base2)))
        reg |= PCIE_GET_DBI2_REG_OFFSET;

    switch (size)
    {
        case 4:
            return sstar_pcieif_readl_dbi(plat->id, reg);
        case 2:
            return sstar_pcieif_readw_dbi(plat->id, reg);
        case 1:
            return sstar_pcieif_readb_dbi(plat->id, reg);
        default:
            dev_err(pci->dev, "sstar_pcie_read_dbi size invalid\n");
            return 0;
    }

    return 0;
}

static void sstar_pcie_write_dbi(struct dw_pcie *pci, void __iomem *base, u32 reg, size_t size, u32 val)
{
    struct dw_plat_pcie *plat = dev_get_drvdata(pci->dev);

    if ((uintptr_t)reg & (size - 1))
    {
        dev_err(pci->dev, "sstar_pcie_write_dbi address failed\n");
        return;
    }

    if (((u64)base) == ((u64)(pci->atu_base)))
        reg |= DEFAULT_DBI_ATU_OFFSET;
    else if (((u64)base) == ((u64)(pci->dbi_base2)))
        reg |= PCIE_GET_DBI2_REG_OFFSET;

    switch (size)
    {
        case 4:
            return sstar_pcieif_writel_dbi(plat->id, reg, val);
        case 2:
            return sstar_pcieif_writew_dbi(plat->id, reg, val);
        case 1:
            return sstar_pcieif_writeb_dbi(plat->id, reg, val);
        default:
            dev_err(pci->dev, "sstar_pcie_write_dbi size invalid\n");
    }
}
/*
static void sstar_pcie_write_dbi2(struct dw_pcie *pci, void __iomem *base,
                u32 reg, size_t size, u32 val)
{
    struct dw_plat_pcie *plat = dev_get_drvdata(pci->dev);
    u32 offset = PCIE_GET_DBI2_REG_OFFSET();
    reg += offset;
    if ((uintptr_t)reg & (size - 1)) {
        dev_err(pci->dev, "sstar_pcie_write_dbi address failed\n");
        return;
    }

    switch(size) {
    case 4:
        return ss_pcieif_writel_dbi(plat->id, reg, val);
    case 2:
        return ss_pcieif_writew_dbi(plat->id, reg, val);
    case 1:
        return ss_pcieif_writeb_dbi(plat->id, reg, val);
    default:
        dev_err(pci->dev, "sstar_pcie_write_dbi size invalid\n");
    }
}
*/
static const struct dw_pcie_ops dw_pcie_ops = {
    .cpu_addr_fixup = sstar_pcie_cpu_addr_fixup,
    .start_link     = sstar_pcie_start_link,
    .read_dbi       = sstar_pcie_read_dbi,
    .write_dbi      = sstar_pcie_write_dbi,
    .write_dbi2     = sstar_pcie_write_dbi,
};

static int sstar_host_rd_own_conf(struct pcie_port *pp, int where, int size, u32 *val)
{
    struct dw_pcie *pci = to_dw_pcie_from_pp(pp);

    *val = sstar_pcie_read_dbi(pci, pci->dbi_base, where, size);
    return 0;
}

static int sstar_host_wr_own_conf(struct pcie_port *pp, int where, int size, u32 val)
{
    struct dw_pcie *pci = to_dw_pcie_from_pp(pp);

    sstar_pcie_write_dbi(pci, pci->dbi_base, where, size, val);
    return 0;
}

#ifdef CONFIG_SSTAR_FPGA_LPBK
static void _link_up(struct dw_pcie *pci)
{
    struct dw_plat_pcie *plat = dev_get_drvdata(pci->dev);
    u8                   ep;

    dev_info(pci->dev, "_link_up...\r\n");

    ep = (plat->id == 0) ? 1 : 0;
    sstar_pcieif_loopback_link(plat->id, ep);
}
#endif

static void dw_sstar_pcie_clkinit(struct device *pdev, bool use_internal_clk)
{
    struct device_node *dev_node = (struct device_node *)(pdev->of_node);
    struct clk **       pcie_clks;
    int                 num_parents, i, clks_mandatory = 0;

#if CONFIG_OF
    num_parents = of_clk_get_parent_count(dev_node);
    if (!use_internal_clk)
    {
        if (of_property_read_u32(pdev->of_node, "num_clks_mandatory", &clks_mandatory))
            dev_dbg(pdev, "no num_clks_mandatory in dts\r\n");
        else
            num_parents = clks_mandatory;
    }

    if (num_parents > 0)
    {
        pcie_clks = kzalloc((sizeof(struct clk *) * num_parents), GFP_KERNEL);
        if (pcie_clks == NULL)
        {
            printk("[PCIE]kzalloc failed!\n");
            return;
        }

        // enable all clk
        for (i = 0; i < num_parents; i++)
        {
            pcie_clks[i] = of_clk_get(dev_node, i);
            if (IS_ERR(pcie_clks[i]))
            {
                printk("Fail to get PCIE clk!\n");
                clk_put(pcie_clks[i]);
                kfree(pcie_clks);
                return;
            }
            else
            {
                /* Set clock parent */
                clk_prepare_enable(pcie_clks[i]);
                clk_put(pcie_clks[i]);
            }
        }
        kfree(pcie_clks);
    }
#endif
    return;
}

static int dw_plat_pcie_host_init(struct pcie_port *pp)
{
    struct dw_pcie *pci = to_dw_pcie_from_pp(pp);

#ifdef CONFIG_SSTAR_FPGA_LPBK
    _link_up(pci);
#endif
    dw_pcie_setup_rc(pp);

#ifndef CONFIG_SSTAR_FPGA_LPBK
    sstar_pcie_start_link(pci);
#endif

    dw_pcie_wait_for_link(pci);

    // if (IS_ENABLED(CONFIG_PCI_MSI))
    dw_pcie_msi_init(pp);

    return 0;
}

static const struct dw_pcie_host_ops dw_plat_pcie_host_ops = {
    .host_init       = dw_plat_pcie_host_init,
    .set_num_vectors = dw_plat_set_num_vectors,
    .rd_own_conf     = sstar_host_rd_own_conf,
    .wr_own_conf     = sstar_host_wr_own_conf,
};

static void dw_plat_pcie_ep_init(struct dw_pcie_ep *ep)
{
    struct dw_pcie *pci = to_dw_pcie_from_ep(ep);
    enum pci_barno  bar;

    // always using unroll iatu
    pci->iatu_unroll_enabled = 1;

    for (bar = 0; bar <= PCI_STD_NUM_BARS; bar++)
        dw_pcie_ep_reset_bar(pci, bar);
}

static int dw_plat_pcie_ep_raise_irq(struct dw_pcie_ep *ep, u8 func_no, enum pci_epc_irq_type type, u16 interrupt_num)
{
    struct dw_pcie *pci = to_dw_pcie_from_ep(ep);

    switch (type)
    {
        case PCI_EPC_IRQ_LEGACY:
            return dw_pcie_ep_raise_legacy_irq(ep, func_no);
        case PCI_EPC_IRQ_MSI:
            return dw_pcie_ep_raise_msi_irq(ep, func_no, interrupt_num);
        case PCI_EPC_IRQ_MSIX:
            return dw_pcie_ep_raise_msix_irq(ep, func_no, interrupt_num);
        default:
            dev_err(pci->dev, "UNKNOWN IRQ type\n");
    }

    return 0;
}

static const struct pci_epc_features dw_plat_pcie_epc_features = {
    .linkup_notifier = false,
    .msi_capable     = true,
    .msix_capable    = true,
    .bar_fixed_64bit = 0,
    .bar_fixed_size  = {SZ_1M, SZ_64K, SZ_1M, SZ_64K, SZ_256, SZ_64K},
    .align           = SZ_64K,
};

static const struct pci_epc_features *dw_plat_pcie_get_features(struct dw_pcie_ep *ep)
{
    return &dw_plat_pcie_epc_features;
}

static const struct dw_pcie_ep_ops pcie_ep_ops = {
    .ep_init      = dw_plat_pcie_ep_init,
    .raise_irq    = dw_plat_pcie_ep_raise_irq,
    .get_features = dw_plat_pcie_get_features,
};

static int dw_plat_add_pcie_port(struct dw_plat_pcie *dw_plat_pcie, struct platform_device *pdev)
{
    struct dw_pcie *  pci = dw_plat_pcie->pci;
    struct pcie_port *pp  = &pci->pp;
    struct device *   dev = &pdev->dev;
    int               ret;

    pp->irq = platform_get_irq(pdev, 1);
    if (pp->irq < 0)
        return pp->irq;
    // devm_request_irq later?

    if (IS_ENABLED(CONFIG_PCI_MSI))
    {
        pp->msi_irq = platform_get_irq(pdev, 0);
        if (pp->msi_irq < 0)
            return pp->msi_irq;
    }

    pp->ops = &dw_plat_pcie_host_ops;

    sstar_pcieif_set_mode(dw_plat_pcie->id, PCIE_IF_MODE_RC);
    // set vendorID & deviceID
    sstar_pcieif_writel_dbi(dw_plat_pcie->id, PCI_VENDOR_ID, 0x47AE19E5);
    sstar_pcieif_writel_dbi(dw_plat_pcie->id, PCIE_LINK_WIDTH_SPEED_CONTROL,
                            PORT_LOGIC_AUTO_LANE_FLIP | PORT_LOGIC_LINK_WIDTH_2_LANES
                                | PORT_LOGIC_FAST_TRAIN_SEQ(0x2C));

    ret = dw_pcie_host_init(pp);
    if (ret)
    {
        dev_err(dev, "Failed to initialize host\n");
        return ret;
    }

    return 0;
}

static int dw_plat_add_pcie_ep(struct dw_plat_pcie *dw_plat_pcie, struct platform_device *pdev)
{
    int                ret;
    struct dw_pcie_ep *ep;
    struct resource *  res;
    struct device *    dev = &pdev->dev;
    struct dw_pcie *   pci = dw_plat_pcie->pci;

    ep      = &pci->ep;
    ep->ops = &pcie_ep_ops;

    res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "dbi2");
    if (!res)
    {
        // dts doesn't specify the dbi2 base, dbi2 access is handled by the hooked dbi_read / dbi_write ops
        pci->dbi_base2 = pci->dbi_base + PCIE_GET_DBI2_REG_OFFSET;
    }
    else
    {
        pci->dbi_base2 = devm_ioremap_resource(dev, res);
        if (IS_ERR(pci->dbi_base2))
            return PTR_ERR(pci->dbi_base2);
    }

    res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "addr_space");
    if (!res)
        return -EINVAL;

    ep->irq       = platform_get_irq_byname(pdev, "mac");
    ep->phys_base = res->start;
    ep->addr_size = resource_size(res);
    ep->page_size = SZ_64K;
    dev_info(dev, "addr_base = 0x%llx, size = 0x%lx, irq = %d\r\n", ep->phys_base, ep->addr_size, ep->irq);

    sstar_pcieif_set_mode(dw_plat_pcie->id, PCIE_IF_MODE_EP);
    sstar_pcieif_writel_dbi(dw_plat_pcie->id, PCI_VENDOR_ID, 0x7B2A1D0F); // FAKE vendor id of AMAZON as EP
    sstar_pcieif_writel_dbi(dw_plat_pcie->id, PCIE_LINK_WIDTH_SPEED_CONTROL,
                            PORT_LOGIC_SPEED_CHANGE | PORT_LOGIC_AUTO_LANE_FLIP | PORT_LOGIC_LINK_WIDTH_2_LANES
                                | PORT_LOGIC_FAST_TRAIN_SEQ(0x2C));
    sstar_pcieif_writel_dbi(dw_plat_pcie->id, PORT_LOGIC_TRGT_MAP_CTRL,
                            sstar_pcieif_readl_dbi(dw_plat_pcie->id, PORT_LOGIC_TRGT_MAP_CTRL) | 0x3F);

    ret = dw_pcie_ep_init(ep);
    if (ret)
    {
        dev_err(dev, "Failed to initialize endpoint\n");
        return ret;
    }

    if (IS_ENABLED(CONFIG_PCI_EPF_VHOST))
    {
        struct pci_epc *epc;
        struct pci_epf *epf;
        char            name[64];

        snprintf(name, sizeof(name), "pci-epf-vhost%d", dw_plat_pcie->id);
        epc          = ep->epc;
        epf          = pci_epf_create(name);
        epf->func_no = 0;

        ret = pci_epc_add_epf(epc, epf);
        if (ret)
            goto err_add_epf;

        ret = pci_epf_bind(epf);
        if (ret)
            goto err_epf_bind;

        ret = pci_epc_start(epc);
        if (ret)
            goto err_epc_start;

        return 0;
    err_epc_start:
        pci_epf_unbind(epf);
    err_epf_bind:
        pci_epc_remove_epf(epc, epf);
    err_add_epf:
        return ret;
    }

    return ret;
}

static int dw_plat_pcie_probe(struct platform_device *pdev)
{
    struct device *                    dev = &pdev->dev;
    struct dw_plat_pcie *              dw_plat_pcie;
    struct dw_pcie *                   pci;
    struct resource *                  res; /* Resource from DT */
    int                                ret;
    const struct of_device_id *        match;
    const struct dw_plat_pcie_of_data *data;
    enum dw_pcie_device_mode           mode;
    u32                                id;
    u32                                use_internal_clk = 0;

    // printk("\r\n@ %s_%d \r\n", __func__, __LINE__);
    // printk("From Bootargs :use_internal_clk_pcie0_%d \r\n", use_internal_clk_pcie0);
    // printk("From Bootargs :use_internal_clk_pcie1_%d \r\n", use_internal_clk_pcie1);
    // printk("From Bootargs :pcie0_ssc_enable_%d \r\n", pcie0_ssc_enable);
    // printk("From Bootargs :pcie1_ssc_enable_%d \r\n", pcie1_ssc_enable);

    match = of_match_device(dw_plat_pcie_of_match, dev);
    if (!match)
        return -EINVAL;

    data = (struct dw_plat_pcie_of_data *)match->data;
    mode = (enum dw_pcie_device_mode)data->mode;

    dw_plat_pcie = devm_kzalloc(dev, sizeof(*dw_plat_pcie), GFP_KERNEL);
    if (!dw_plat_pcie)
        return -ENOMEM;

    pci = devm_kzalloc(dev, sizeof(*pci), GFP_KERNEL);
    if (!pci)
        return -ENOMEM;

    pci->dev = dev;
    pci->ops = &dw_pcie_ops;

    dw_plat_pcie->pci = pci;
    if (of_property_read_u32(dev->of_node, "portid", &id))
    {
        dev_err(dev, "must have portid property in device tree\r\n");
        return -EINVAL;
    }
    dw_plat_pcie->id = id;
    if (mode == DW_PCIE_UNKNOWN_TYPE)
    {
        if (id == 0)
            dw_plat_pcie->mode = gpcie0_mode;
        else if (id == 1)
            dw_plat_pcie->mode = gpcie1_mode;
    }
    else
    {
        dw_plat_pcie->mode = mode;
    }

    if ((dw_plat_pcie->mode == DW_PCIE_EP_TYPE) && ((id == 0 && gpcie0_boot) || (id == 1 && gpcie1_boot)))
    {
        if (!of_property_read_bool(dev->of_node, "ep,dis_set_bar_quirk"))
        {
            struct property *prop;
            int              ret;

            prop       = kzalloc(sizeof(*prop), GFP_KERNEL);
            prop->name = "ep,dis_set_bar_quirk";
            ret        = of_add_property(dev->of_node, prop);
            if (ret)
            {
                dev_err(&pdev->dev, "add dis_set_bar_quirk property failed\n");
                kfree(prop);
                return ret;
            }
        }
    }

    if (IS_ENABLED(CONFIG_ARM64))
    {
        if (dma_set_mask(&pdev->dev, DMA_BIT_MASK(64)) || dma_set_coherent_mask(&pdev->dev, DMA_BIT_MASK(64)))
        {
            dev_err(&pdev->dev, "consistent DMA mask 64 set failed\n");
            return -EOPNOTSUPP;
        }
    }

    res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "dbi");
    if (!res)
        res = platform_get_resource(pdev, IORESOURCE_MEM, 0);

    pci->dbi_base = devm_ioremap_resource(dev, res);
    if (IS_ERR(pci->dbi_base))
        return PTR_ERR(pci->dbi_base);
    dev_info(dev, "dbi_base = 0x%px\r\n", pci->dbi_base);

    platform_set_drvdata(pdev, dw_plat_pcie);

    if (((id == 0) && use_internal_clk_pcie0) || ((id == 1) && use_internal_clk_pcie1))
    {
        use_internal_clk = 1; // bootargs for higer priority
    }
    else
    { // dtsi for lower priority
        if (of_property_read_u32(dev->of_node, "use_internal_clk", &use_internal_clk))
            dev_dbg(dev, "not using internal-pll for pcie%d\r\n", id);
    }

    dw_sstar_pcie_clkinit(dev, use_internal_clk);

    // ref 100M_Clock reg-setting
    if (use_internal_clk)
    {
        if (id == 0)
            sstar_pcieif_internalclk_en(id, pcie0_ssc_enable);
        else if (id == 1)
            sstar_pcieif_internalclk_en(id, pcie1_ssc_enable);
    }

    switch (dw_plat_pcie->mode)
    {
        case DW_PCIE_RC_TYPE:
            if (!IS_ENABLED(CONFIG_SSTAR_PCIE_HOST))
                return -ENODEV;

            ret = dw_plat_add_pcie_port(dw_plat_pcie, pdev);
            if (ret < 0)
                return ret;
            break;
        case DW_PCIE_EP_TYPE:
            if (!IS_ENABLED(CONFIG_SSTAR_PCIE_EP))
                return -ENODEV;

            ret = dw_plat_add_pcie_ep(dw_plat_pcie, pdev);
            if (ret < 0)
                return ret;
            break;
        default:
            dev_err(dev, "INVALID device type %d\n", dw_plat_pcie->mode);
    }

    return 0;
}

static void __init pcie_epc_reserve_memory(u8 id)
{
    u64 base, size;

    if (id == 0)
    {
        base = 0x1000200000;
        size = 0x00100000;
    }
    else if (id == 1)
    {
        base = 0x1000300000;
        size = 0x00100000;
    }
    else
    {
        return;
    }
    early_init_dt_reserve_memory_arch(base, size, false);
}

static void __init parse_pcie_mode(u8 id, char *str)
{
    while (str)
    {
        char *k = strchr(str, ',');
        if (k)
            *k++ = 0;
        if (*str)
        {
            if (!strncmp(str, "rc", 2))
            {
                if (id == 0)
                    gpcie0_mode = DW_PCIE_RC_TYPE;
                else if (id == 1)
                    gpcie1_mode = DW_PCIE_RC_TYPE;
            }
            else if (!strncmp(str, "ep", 2))
            {
                if (id == 0)
                    gpcie0_mode = DW_PCIE_EP_TYPE;
                else if (id == 1)
                    gpcie1_mode = DW_PCIE_EP_TYPE;
                pcie_epc_reserve_memory(id);
            }
            else if (!strncmp(str, "boot", 4))
            {
                if (id == 0)
                    gpcie0_boot = true;
                else if (id == 1)
                    gpcie1_boot = true;
            }
            else if (!strncmp(str, "int", 3)) // internal pll
            {
                if (id == 0)
                    use_internal_clk_pcie0 = true;
                else if (id == 1)
                    use_internal_clk_pcie1 = true;
            }
            else if (!strncmp(str, "ssc", 3))
            {
                if (id == 0)
                {
                    use_internal_clk_pcie0 = true;
                    pcie0_ssc_enable       = true;
                }
                else if (id == 1)
                {
                    use_internal_clk_pcie1 = true;
                    pcie1_ssc_enable       = true;
                }
            }
            else
            {
                pr_err("pcie%d: Unknown option `%s'\n", id, str);
            }
        }
        str = k;
    }
}

static int __init pcie0_setup(char *str)
{
    parse_pcie_mode(0, str);
    return 0;
}
early_param("pcie0", pcie0_setup);

static int __init pcie1_setup(char *str)
{
    parse_pcie_mode(1, str);
    return 0;
}
early_param("pcie1", pcie1_setup);

static const struct dw_plat_pcie_of_data dw_plat_pcie_rc_of_data = {
    .mode = DW_PCIE_RC_TYPE,
};

static const struct dw_plat_pcie_of_data dw_plat_pcie_ep_of_data = {
    .mode = DW_PCIE_EP_TYPE,
};

static const struct dw_plat_pcie_of_data dw_plat_pcie_earlyparam_of_data = {
    .mode = DW_PCIE_UNKNOWN_TYPE,
};

static const struct of_device_id dw_plat_pcie_of_match[] = {
    {
        .compatible = "snps,dw-pcie",
        .data       = &dw_plat_pcie_rc_of_data,
    },
    {
        .compatible = "snps,dw-pcie-ep",
        .data       = &dw_plat_pcie_ep_of_data,
    },
    {
        .compatible = "snps,dw-pcie-earlyparam",
        .data       = &dw_plat_pcie_earlyparam_of_data,
    },
    {},
};

static struct platform_driver dw_plat_pcie_driver = {
    .driver =
        {
            .name                = "dw-pcie",
            .of_match_table      = dw_plat_pcie_of_match,
            .suppress_bind_attrs = true,
        },
    .probe = dw_plat_pcie_probe,
};
builtin_platform_driver(dw_plat_pcie_driver);
