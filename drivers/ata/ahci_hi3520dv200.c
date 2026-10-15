// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * AHCI glue for the HiSilicon Hi3520DV200 on-chip SATA controller.
 *
 * The controller is a standard AHCI block, but its two-port PHY needs to be
 * clocked, released from reset and programmed before the AHCI core sees a
 * link. The sequence, register values and the per-port bug-bypass setting
 * follow the vendor 3.0.8 BSP (drivers/ata/ahci_sys.c and
 * ahci_sys_hi3520d_defconfig.c, CONFIG_HI_SATA, 1.5 Gbps mode).
 */

#include <linux/ahci_platform.h>
#include <linux/delay.h>
#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/libata.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/platform_device.h>

#include "ahci.h"

#define DRV_NAME "ahci-hi3520dv200"

/* PERI_CRG45 (0x200300b4): SATA clocks and resets */
#define CRG_PHY_RST		BIT(0)
#define CRG_HRST		BIT(1)
#define CRG_RST			BIT(2)
#define CRG_TX0_RST		BIT(3)
#define CRG_TX1_RST		BIT(4)
#define CRG_RX0_RST		BIT(5)
#define CRG_RX1_RST		BIT(6)
#define CRG_ALIVE_RST		BIT(7)
#define CRG_PORT1_CKEN		BIT(8)
#define CRG_PORT0_CKEN		BIT(9)
#define CRG_CKEN		BIT(10)
#define CRG_CLK_SEL		BIT(11)
#define CRG_CLK_ALIVE		BIT(12)
#define CRG_CORE_RESETS		(CRG_RST | CRG_TX0_RST | CRG_TX1_RST | \
				 CRG_RX0_RST | CRG_RX1_RST | CRG_ALIVE_RST)

/* Vendor registers in the AHCI block */
#define PHY0_CTLL		0x54
#define PHY0_CTLH		0x58
#define PHY1_CTLL		0x60
#define PHY1_CTLH		0x64
#define PHY_CTLH_DIS_CLK	BIT(12)
#define OOB_CTL			0x6c
#define PORT_BASE(i)		(0x100 + (i) * 0x80)
#define PORT_BUG_BYPASS		0x44
#define PORT_PHYCTL		0x74

/* 1.5 Gbps PHY settings from the vendor BSP */
#define PHY_CTLL_1G5		0x880ec788
#define PHY_CTLH		0x00002521
#define OOB_CTL_VAL		0x84060c15
/*
 * Data-transmit-control, record, PM-reset and fetch bypasses plus the
 * Seagate R_ERR workaround (bits 13..9), which the vendor sets on every port
 * "due to bugs of sata controller".
 */
#define PORT_BUG_BYPASS_VAL	0x3f24
/*
 * Port PHY control. HiSilicon's Hi3520D reference value is 0x0e276756; the
 * vendor firmware on XiongMai boards writes 0x0e262709 (the Hi3531/Hi3521
 * value), without which their link keeps resetting. Boards may override it
 * with "hisilicon,phy-config".
 */
#define PORT_PHYCTL_DEFAULT	0x0e276756
#define NR_PORTS		2

struct hi3520dv200_sata {
	void __iomem *crg;
	u32 phy_config;
};

static void crg_update(void __iomem *crg, u32 clear, u32 set)
{
	writel((readl(crg) & ~clear) | set, crg);
}

static void hi3520dv200_sata_phy_init(struct ahci_host_priv *hpriv)
{
	struct hi3520dv200_sata *priv = hpriv->plat_data;
	void __iomem *mmio = hpriv->mmio;
	void __iomem *crg = priv->crg;
	int i;

	/* clocks on, internal PHY clock, per-port bug bypasses */
	crg_update(crg, 0, CRG_CKEN | CRG_PORT0_CKEN | CRG_PORT1_CKEN);
	crg_update(crg, CRG_CLK_SEL | CRG_CLK_ALIVE, 0);
	for (i = 0; i < NR_PORTS; i++)
		writel(PORT_BUG_BYPASS_VAL, mmio + PORT_BASE(i) + PORT_BUG_BYPASS);

	/* bus interface, then PHY out of reset */
	crg_update(crg, CRG_HRST, 0);
	msleep(20);
	crg_update(crg, CRG_PHY_RST, 0);
	msleep(20);

	/* program both PHYs with their clocks gated */
	writel(readl(mmio + PHY0_CTLH) | PHY_CTLH_DIS_CLK, mmio + PHY0_CTLH);
	writel(readl(mmio + PHY1_CTLH) | PHY_CTLH_DIS_CLK, mmio + PHY1_CTLH);
	writel(PHY_CTLL_1G5, mmio + PHY0_CTLL);
	writel(PHY_CTLH, mmio + PHY0_CTLH);
	writel(PHY_CTLL_1G5, mmio + PHY1_CTLL);
	writel(PHY_CTLH, mmio + PHY1_CTLH);
	writel(OOB_CTL_VAL, mmio + OOB_CTL);
	for (i = 0; i < NR_PORTS; i++)
		writel(priv->phy_config, mmio + PORT_BASE(i) + PORT_PHYCTL);

	/* pulse the PHY reset so the settings take, then release the core */
	crg_update(crg, 0, CRG_PHY_RST);
	msleep(20);
	crg_update(crg, CRG_PHY_RST, 0);
	msleep(20);
	crg_update(crg, CRG_CORE_RESETS, 0);
	msleep(20);
}

static const struct ata_port_info ahci_hi3520dv200_port_info = {
	.flags		= AHCI_FLAG_COMMON,
	.pio_mask	= ATA_PIO4,
	.udma_mask	= ATA_UDMA6,
	.port_ops	= &ahci_platform_ops,
};

static const struct scsi_host_template ahci_hi3520dv200_sht = {
	AHCI_SHT(DRV_NAME),
};

static int ahci_hi3520dv200_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct hi3520dv200_sata *priv;
	struct ahci_host_priv *hpriv;
	int ret;

	priv = devm_kzalloc(dev, sizeof(*priv), GFP_KERNEL);
	if (!priv)
		return -ENOMEM;

	priv->crg = devm_platform_ioremap_resource(pdev, 1);
	if (IS_ERR(priv->crg))
		return PTR_ERR(priv->crg);

	if (of_property_read_u32(dev->of_node, "hisilicon,phy-config",
				 &priv->phy_config))
		priv->phy_config = PORT_PHYCTL_DEFAULT;

	hpriv = ahci_platform_get_resources(pdev, 0);
	if (IS_ERR(hpriv))
		return PTR_ERR(hpriv);
	hpriv->plat_data = priv;

	ret = ahci_platform_enable_resources(hpriv);
	if (ret)
		return ret;

	hi3520dv200_sata_phy_init(hpriv);

	ret = ahci_platform_init_host(pdev, hpriv, &ahci_hi3520dv200_port_info,
				      &ahci_hi3520dv200_sht);
	if (ret) {
		ahci_platform_disable_resources(hpriv);
		return ret;
	}

	dev_info(dev, "PHY config %#010x\n", priv->phy_config);
	return 0;
}

static const struct of_device_id ahci_hi3520dv200_of_match[] = {
	{ .compatible = "hisilicon,hi3520dv200-ahci" },
	{}
};
MODULE_DEVICE_TABLE(of, ahci_hi3520dv200_of_match);

static struct platform_driver ahci_hi3520dv200_driver = {
	.probe = ahci_hi3520dv200_probe,
	.remove = ata_platform_remove_one,
	.driver = {
		.name = DRV_NAME,
		.of_match_table = ahci_hi3520dv200_of_match,
	},
};
module_platform_driver(ahci_hi3520dv200_driver);

MODULE_DESCRIPTION("HiSilicon Hi3520DV200 AHCI SATA driver");
MODULE_LICENSE("GPL");
