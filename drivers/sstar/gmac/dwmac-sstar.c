/*
 * dwmac-sstar.c- Sigmastar
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
#include <linux/pm_wakeirq.h>
#include <linux/regmap.h>
#include <linux/slab.h>
#include <linux/stmmac.h>
#include <linux/notifier.h>
#include <linux/syscalls.h>
#include "dwmac4.h"
#include "dwmac4_sstar_dma.h"
#include "stmmac_platform.h"
#include "sstar_gmac.h"

#define SYSCFG_MCU_ETH_MASK   BIT(23)
#define SYSCFG_MP1_ETH_MASK   GENMASK(23, 16)
#define SYSCFG_PMCCLRR_OFFSET 0x40

#define SYSCFG_PMCR_ETH_CLK_SEL     BIT(16)
#define SYSCFG_PMCR_ETH_REF_CLK_SEL BIT(17)

/* CLOCK feed to PHY*/
#define ETH_CK_F_2_5M 2500000
#define ETH_CK_F_25M  25000000
#define ETH_CK_F_125M 125000000

#define SYSCFG_PMCR_ETH_SEL_MII   BIT(20)
#define SYSCFG_PMCR_ETH_SEL_RGMII BIT(21)
#define SYSCFG_PMCR_ETH_SEL_RMII  BIT(23)
#define SYSCFG_PMCR_ETH_SEL_GMII  0
#define SYSCFG_MCU_ETH_SEL_MII    0
#define SYSCFG_MCU_ETH_SEL_RMII   1

void *GBase[GMAC_NUM]                = {0};
u32   GBank[GMAC_NUM][GMAC_BANK_NUM] = {0};

extern void dwmac_sstar_drv_proc(struct sstar_dwmac *dwmac);
#ifdef CONFIG_SSTAR_SNPS_GMAC_TEST
extern void dwmac_sstar_test(struct sstar_dwmac *dwmac);
#endif
extern const struct stmmac_desc_ops dwmac4_desc_sstar_ops;
extern const struct stmmac_dma_ops  dwmac410_dma_sstar_ops;

#ifndef CONFIG_SSTAR_SNPS_GMAC_CODING_OPTIMIZE
u8 sstar_gmac_iomem2id(void __iomem *ioaddr)
{
    u8 GmacId = 0;

    if (GBase[GMAC1] && (uintptr_t)ioaddr >= (uintptr_t)GBase[GMAC1]) // GMAC1
    {
        GmacId = GMAC1;
    }
    else if (GBase[GMAC0] && (uintptr_t)ioaddr >= (uintptr_t)GBase[GMAC0]) // GMAC0
    {
        GmacId = GMAC0;
    }

    return GmacId;
}
#endif
void sstar_gmac_disable_and_clean_int(void *par)
{
    struct stmmac_priv *priv = par;
    u32                 chan, maxq;

    maxq = priv->plat->rx_queues_to_use;

    if (maxq < priv->plat->tx_queues_to_use)
        maxq = priv->plat->tx_queues_to_use;

    for (chan = 0; chan < maxq; chan++)
    {
        writel(0, priv->ioaddr + DMA_CHAN_INTR_ENA(chan));
        writel(0xFFFFFFFF, priv->ioaddr + DMA_CHAN_STATUS(chan));
    }
}

static u32 dwmac4_sstar_get_num_vlan(void __iomem *ioaddr)
{
    u32 val, num_vlan;

    val = readl(ioaddr + GMAC_HW_FEATURE3);
    switch (val & GMAC_HW_FEAT_NRVF)
    {
        case 0:
            num_vlan = 1;
            break;
        case 1:
            num_vlan = 4;
            break;
        case 2:
            num_vlan = 8;
            break;
        case 3:
            num_vlan = 16;
            break;
        case 4:
            num_vlan = 24;
            break;
        case 5:
            num_vlan = 32;
            break;
        default:
            num_vlan = 1;
    }

    return num_vlan;
}

static struct mac_device_info *sstar_dwmac_setup(void *pdata)
{
    struct mac_device_info *mac; // = priv->hw;
    struct stmmac_priv *    priv = (struct stmmac_priv *)pdata;

    mac = devm_kzalloc(priv->device, sizeof(*mac), GFP_KERNEL);

    if (!mac)
        return NULL;

    priv->dev->priv_flags |= IFF_UNICAST_FLT;
    mac->pcsr                   = priv->ioaddr;
    mac->multicast_filter_bins  = priv->plat->multicast_filter_bins;
    mac->unicast_filter_entries = priv->plat->unicast_filter_entries;
    mac->mcast_bits_log2        = 0;

    if (mac->multicast_filter_bins)
        mac->mcast_bits_log2 = ilog2(mac->multicast_filter_bins);

    mac->link.duplex       = GMAC_CONFIG_DM;
    mac->link.speed10      = GMAC_CONFIG_PS;
    mac->link.speed100     = GMAC_CONFIG_FES | GMAC_CONFIG_PS;
    mac->link.speed1000    = 0;
    mac->link.speed_mask   = GMAC_CONFIG_FES | GMAC_CONFIG_PS;
    mac->mii.addr          = GMAC_MDIO_ADDR;
    mac->mii.data          = GMAC_MDIO_DATA;
    mac->mii.addr_shift    = 21;
    mac->mii.addr_mask     = GENMASK(25, 21);
    mac->mii.reg_shift     = 16;
    mac->mii.reg_mask      = GENMASK(20, 16);
    mac->mii.clk_csr_shift = 8;
    mac->mii.clk_csr_mask  = GENMASK(11, 8);
    mac->num_vlan          = dwmac4_sstar_get_num_vlan(priv->ioaddr);
    mac->desc              = &dwmac4_desc_sstar_ops;
    mac->dma               = &dwmac410_dma_sstar_ops;

    return mac;
}

static void sstar_dwmac_fix_speed(void *priv, unsigned int speed)
{
    struct sstar_dwmac *dwmac = priv;
    int                 err;
    unsigned long       rate;

    switch (speed)
    {
        case SPEED_1000:
            rate = ETH_CK_F_125M;
            break;
        case SPEED_100:
            rate = ETH_CK_F_25M;
            break;
        case SPEED_10:
            rate = ETH_CK_F_2_5M;
            break;
        default:
            dev_err(dwmac->dev, "invalid speed %u\n", speed);
            return;
    }

    err = clk_set_rate(dwmac->gmac_clkgen, rate);
    if (err < 0)
        dev_err(dwmac->dev, "failed to set tx rate %lu\n", rate);

    // printk("Change clk = 0x%x\n", INREG16(0x1f207ED4));

    mhal_gmac_tx_clk_pad_sel(dwmac, speed);
}

static int sstar_dwmac_init(struct plat_stmmacenet_data *plat_dat)
{
    struct sstar_dwmac *dwmac = plat_dat->bsp_priv;
    int                 ret;
    int                 err;

    ret = clk_prepare_enable(dwmac->gmac_clkgen);
    if (ret)
        return ret;
    // printk("Enable clk = 0x%x\n", INREG16(0x1f207ED4));

    if (dwmac->mclk_freq)
    {
        ret = clk_prepare_enable(dwmac->gmac_mclk);
        if (ret)
        {
            dev_err(dwmac->dev, "failed to open mclk \n");
            return ret;
        }

        err = clk_set_rate(dwmac->gmac_mclk, dwmac->mclk_freq * 1000000);
        if (err < 0)
            dev_err(dwmac->dev, "failed to set mclk rate %u MHz\n", dwmac->mclk_freq);
    }
    return ret;
}

static void sstar_dwmac_clk_disable(struct sstar_dwmac *dwmac)
{
    clk_disable_unprepare(dwmac->gmac_clkgen);
    clk_disable_unprepare(dwmac->gmac_mclk);
}

static int sstar_dwmac_parse_data(struct sstar_dwmac *dwmac, struct device *dev, struct stmmac_resources *stmmac_res)
{
    dwmac->gmac_clkgen = devm_clk_get(dev, "gmac-clk");
    if (IS_ERR(dwmac->gmac_clkgen))
    {
        dev_err(dev, "No gmac clock provided...\n");
        return PTR_ERR(dwmac->gmac_clkgen);
    }

    dwmac->gmac_mclk = devm_clk_get(dev, "mclk");
    if (IS_ERR(dwmac->gmac_mclk))
    {
        dev_err(dev, "No gmac mclock provided...\n");
    }

    return 0;
}

static int sstar_probe_config_dt(struct sstar_dwmac *dwmac)
{
    char proc_dir_str[32];

    if (of_property_read_u32(dwmac->np, "gmac-id", &dwmac->id) < 0)
    {
        dev_err(dwmac->dev, "missing sstar gmac id property\n");
        return -EINVAL;
    }

    GBase[dwmac->id] = dwmac->base_addr;

    device_property_read_u32_array(dwmac->dev, "gmac-bank", dwmac->bank, 8);
    memcpy((void *)GBank[dwmac->id], (void *)dwmac->bank, sizeof(u32) * 8);
    dwmac->reset_io = 0xFF;

    of_property_read_u8(dwmac->np, "disable-txfc", &dwmac->disable_txfc);
    of_property_read_u8(dwmac->np, "mclk", &dwmac->mclk_freq);
    of_property_read_u8(dwmac->np, "mclk-refmode", &dwmac->mclk_refmode);
    of_property_read_u8(dwmac->np, "reset-io", &dwmac->reset_io);

    snprintf(proc_dir_str, sizeof(proc_dir_str), "gmac_%u", dwmac->id);
    dwmac->gmac_root_dir = proc_mkdir(proc_dir_str, NULL);
    dwmac_sstar_drv_proc(dwmac);
#ifdef CONFIG_SSTAR_SNPS_GMAC_TEST
    dwmac_sstar_test(dwmac);
#endif
    mhal_gmac_probe(dwmac);

    return 0;
}

static int gmac_notify_panic(struct notifier_block *self, unsigned long unused1, void *ptr)
{
    // printk("gmac_notify_panic!\n");
    mhal_gmac_disable_padmux();
    return 0;
}

static struct notifier_block gmac_panic_exit_notifier = {
    .notifier_call = gmac_notify_panic, .next = NULL, .priority = 1};

static int gmac_add_notifier(void)
{
    atomic_notifier_chain_register(&panic_notifier_list, &gmac_panic_exit_notifier);
    return 0;
}
__initcall(gmac_add_notifier);

static int sstar_dwmac_probe(struct platform_device *pdev)
{
    struct stmmac_priv *         priv;
    struct plat_stmmacenet_data *plat_dat;
    struct stmmac_resources      stmmac_res;
    struct sstar_dwmac *         dwmac;
    int                          ret;

    ret = stmmac_get_platform_resources(pdev, &stmmac_res);
    if (ret)
        return ret;

    plat_dat = stmmac_probe_config_dt(pdev, &stmmac_res.mac);
    if (IS_ERR(plat_dat))
        return PTR_ERR(plat_dat);

    dwmac = devm_kzalloc(&pdev->dev, sizeof(*dwmac), GFP_KERNEL);
    if (!dwmac)
    {
        ret = -ENOMEM;
        goto err_remove_config_dt;
    }

    dwmac->dev       = &pdev->dev;
    dwmac->np        = pdev->dev.of_node;
    dwmac->interface = plat_dat->interface;
    dwmac->base_addr = stmmac_res.addr;

    sstar_probe_config_dt(dwmac);
#ifdef CONFIG_SSTAR_SNPS_GMAC_CODING_OPTIMIZE
    dwmac->base_addr = (void *)(((unsigned long)dwmac->base_addr) | dwmac->id);
    stmmac_res.addr  = (void *)(((unsigned long)stmmac_res.addr) | dwmac->id);
    mhal_gmac_axi_tbl(dwmac->id, dwmac->bank, GMAC_BANK_NUM);
#endif

#ifdef CONFIG_SSTAR_SNPS_GMAC_STORM_PROTECT
    dwmac->bcast_prct_enable = 1;
    dwmac->bcast_prct_on     = 0;
    dwmac->bcast_thresh      = 200;
    dwmac->mcast_prct_enable = 1;
    dwmac->mcast_prct_on     = 0;
    dwmac->mcast_thresh      = 950; // reserve 10Mbs bandwidth
    dwmac->storm_ms_timer    = 1000;
#endif

#if 0
	data = of_device_get_match_data(&pdev->dev);
	if (!data) {
		dev_err(&pdev->dev, "no of match data provided\n");
		ret = -EINVAL;
		goto err_remove_config_dt;
	}

	dwmac->ops = data;

#endif
    dwmac->dev = &pdev->dev;
    ret        = sstar_dwmac_parse_data(dwmac, &pdev->dev, &stmmac_res);
    if (ret)
    {
        dev_err(&pdev->dev, "Unable to parse OF data\n");
        goto err_remove_config_dt;
    }

    plat_dat->bsp_priv      = dwmac;
    plat_dat->setup         = sstar_dwmac_setup;
    plat_dat->fix_mac_speed = sstar_dwmac_fix_speed;
    plat_dat->clk_csr       = 7; // MDC freq = MIU clock/324
    plat_dat->bus_id        = dwmac->id;
    ret                     = sstar_dwmac_init(plat_dat);
    if (ret)
        goto err_remove_config_dt;

    ret = stmmac_dvr_probe(&pdev->dev, plat_dat, &stmmac_res);
    if (ret)
        goto err_clk_disable;

    priv = netdev_priv(dev_get_drvdata(dwmac->dev));

    if (dwmac->disable_txfc)
        priv->flow_ctrl = FLOW_RX;

    return 0;

err_clk_disable:
    sstar_dwmac_clk_disable(dwmac);
err_remove_config_dt:
    stmmac_remove_config_dt(pdev, plat_dat);

    return ret;
}

static int sstar_dwmac_remove(struct platform_device *pdev)
{
    struct net_device * ndev  = platform_get_drvdata(pdev);
    struct stmmac_priv *priv  = netdev_priv(ndev);
    int                 ret   = stmmac_dvr_remove(&pdev->dev);
    struct sstar_dwmac *dwmac = priv->plat->bsp_priv;
    struct resource *   res   = platform_get_resource(pdev, IORESOURCE_MEM, 0);

    sstar_dwmac_clk_disable(priv->plat->bsp_priv);

    if (dwmac->irq_pwr_wakeup >= 0)
    {
        dev_pm_clear_wake_irq(&pdev->dev);
        device_init_wakeup(&pdev->dev, false);
    }
#ifdef CONFIG_SSTAR_SNPS_GMAC_CODING_OPTIMIZE
    devm_iounmap(&pdev->dev, (void __iomem *)(((unsigned long)dwmac->base_addr) & ~0x3));
#else
    devm_iounmap(&pdev->dev, dwmac->base_addr);
#endif
    devm_release_mem_region(&pdev->dev, res->start, resource_size(res));
    GBase[dwmac->id] = 0;

    return ret;
}
#if 0
static int stm32mp1_suspend(struct sstar_dwmac *dwmac)
{
	int ret = 0;

	ret = clk_prepare_enable(dwmac->clk_ethstp);
	if (ret)
		return ret;

	clk_disable_unprepare(dwmac->clk_tx);
	clk_disable_unprepare(dwmac->syscfg_clk);
	if (dwmac->enable_eth_ck)
		clk_disable_unprepare(dwmac->clk_eth_ck);

	return ret;
}

static void stm32mp1_resume(struct sstar_dwmac *dwmac)
{
	clk_disable_unprepare(dwmac->clk_ethstp);
}

static int stm32mcu_suspend(struct sstar_dwmac *dwmac)
{
	clk_disable_unprepare(dwmac->clk_tx);
	clk_disable_unprepare(dwmac->clk_rx);

	return 0;
}
#endif
#ifdef CONFIG_PM_SLEEP
static int sstar_dwmac_suspend(struct device *dev)
{
    struct net_device * ndev  = dev_get_drvdata(dev);
    struct stmmac_priv *priv  = netdev_priv(ndev);
    struct sstar_dwmac *dwmac = priv->plat->bsp_priv;

    int ret;

    ret = stmmac_suspend(dev);

    if (dwmac->ops->suspend)
        ret = dwmac->ops->suspend(dwmac);

    return ret;
}

static int sstar_dwmac_resume(struct device *dev)
{
    struct net_device * ndev  = dev_get_drvdata(dev);
    struct stmmac_priv *priv  = netdev_priv(ndev);
    struct sstar_dwmac *dwmac = priv->plat->bsp_priv;
    int                 ret;

    if (dwmac->ops->resume)
        dwmac->ops->resume(dwmac);
#if 0
	ret = sstar_dwmac_init(priv->plat);
	if (ret)
		return ret;
#endif
    ret = stmmac_resume(dev);

    return ret;
}
#endif /* CONFIG_PM_SLEEP */

static SIMPLE_DEV_PM_OPS(sstar_dwmac_pm_ops, sstar_dwmac_suspend, sstar_dwmac_resume);

static const struct of_device_id sstar_dwmac_match[] = {{.compatible = "sstar,gmac"}, {}};
MODULE_DEVICE_TABLE(of, sstar_dwmac_match);

struct platform_driver sstar_dwmac_driver = {
    .probe  = sstar_dwmac_probe,
    .remove = sstar_dwmac_remove,
    .driver =
        {
            .name           = "sstar-gmac",
            .pm             = &sstar_dwmac_pm_ops,
            .of_match_table = sstar_dwmac_match,
        },
};
module_platform_driver(sstar_dwmac_driver);

MODULE_AUTHOR("Matt/Titan");
MODULE_DESCRIPTION("Sstar DWMAC Specific Glue layer");
MODULE_LICENSE("GPL v2");
