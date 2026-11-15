// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * AHCI SATA platform driver
 *
 * Copyright 2004-2005  Red Hat, Inc.
 *   Jeff Garzik <jgarzik@pobox.com>
 * Copyright 2010  MontaVista Software, LLC.
 *   Anton Vorontsov <avorontsov@ru.mvista.com>
 */

#include <linux/kernel.h>
#include <linux/mod_devicetable.h>
#include <linux/module.h>
#include <linux/pm.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/property.h>
#include <linux/libata.h>
#include <linux/phy/phy.h>
#include <linux/ahci_platform.h>
#include <linux/pci_ids.h>
#include "ahci.h"

#define DRV_NAME "ahci"

static const struct ata_port_info ahci_port_info = {
	.flags		= AHCI_FLAG_COMMON,
	.pio_mask	= ATA_PIO4,
	.udma_mask	= ATA_UDMA6,
	.port_ops	= &ahci_platform_ops,
};

static const struct ata_port_info ahci_port_info_nolpm = {
	.flags		= AHCI_FLAG_COMMON | ATA_FLAG_NO_LPM,
	.pio_mask	= ATA_PIO4,
	.udma_mask	= ATA_UDMA6,
	.port_ops	= &ahci_platform_ops,
};

/*
 * The PHY of the Hi3520D V100 ignores the speed limit field of SControl; its
 * link speed is selected through phy_set_speed(), in Mbit/s, which has to
 * happen before the hard reset that applies libata's speed limit. libata keeps
 * sata_spd_limit non-zero (UINT_MAX without a limit), so the speed is never 0;
 * the clamp keeps the table index valid regardless.
 */
static int ahci_hi3520dv100_hardreset(struct ata_link *link,
				      unsigned int *class,
				      unsigned long deadline)
{
	static const int gen_speed_mbps[] = { 1500, 3000, 6000 };
	struct ata_port *ap = link->ap;
	struct ahci_host_priv *hpriv = ap->host->private_data;
	int gen = clamp_t(int, fls(link->sata_spd_limit), 1,
			  ARRAY_SIZE(gen_speed_mbps));
	bool online;
	int ret;

	if (ata_is_host_link(link) && hpriv->phys &&
	    ap->port_no < hpriv->nports) {
		ret = phy_set_speed(hpriv->phys[ap->port_no],
				    gen_speed_mbps[gen - 1]);
		if (ret)
			ata_link_warn(link,
				      "PHY did not take the %d Mbit/s speed limit (%d)\n",
				      gen_speed_mbps[gen - 1], ret);
	}

	return ahci_do_hardreset(link, class, deadline, &online);
}

static struct ata_port_operations ahci_hi3520dv100_ops = {
	.inherits	= &ahci_platform_ops,
	.reset.hardreset = ahci_hi3520dv100_hardreset,
};

static const struct ata_port_info ahci_hi3520dv100_port_info = {
	.flags		= AHCI_FLAG_COMMON,
	.pio_mask	= ATA_PIO4,
	.udma_mask	= ATA_UDMA6,
	.port_ops	= &ahci_hi3520dv100_ops,
};

static const struct scsi_host_template ahci_platform_sht = {
	AHCI_SHT(DRV_NAME),
};

static int ahci_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct ahci_host_priv *hpriv;
	const struct ata_port_info *port;
	int rc;

	hpriv = ahci_platform_get_resources(pdev,
					    AHCI_PLATFORM_GET_RESETS);
	if (IS_ERR(hpriv))
		return PTR_ERR(hpriv);

	rc = ahci_platform_enable_resources(hpriv);
	if (rc)
		return rc;

	if (device_is_compatible(dev, "hisilicon,hisi-ahci"))
		hpriv->flags |= AHCI_HFLAG_NO_FBS | AHCI_HFLAG_NO_NCQ;

	port = device_get_match_data(dev);
	if (!port)
		port = &ahci_port_info;

	rc = ahci_platform_init_host(pdev, hpriv, port,
				     &ahci_platform_sht);
	if (rc)
		goto disable_resources;

	return 0;
disable_resources:
	ahci_platform_disable_resources(hpriv);
	return rc;
}

static SIMPLE_DEV_PM_OPS(ahci_pm_ops, ahci_platform_suspend,
			 ahci_platform_resume);

static const struct of_device_id ahci_of_match[] = {
	{ .compatible = "generic-ahci", },
	/* Keep the following compatibles for device tree compatibility */
	{ .compatible = "ibm,476gtr-ahci", },
	{ .compatible = "hisilicon,hisi-ahci", },
	{ .compatible = "hisilicon,hi3520dv100-ahci",
	  .data = &ahci_hi3520dv100_port_info },
	{ .compatible = "cavium,octeon-7130-ahci", },
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, ahci_of_match);

static const struct acpi_device_id ahci_acpi_match[] = {
	{ "APMC0D33", (unsigned long)&ahci_port_info_nolpm },
	{ ACPI_DEVICE_CLASS(PCI_CLASS_STORAGE_SATA_AHCI, 0xffffff) },
	{},
};
MODULE_DEVICE_TABLE(acpi, ahci_acpi_match);

static struct platform_driver ahci_driver = {
	.probe = ahci_probe,
	.remove = ata_platform_remove_one,
	.shutdown = ahci_platform_shutdown,
	.driver = {
		.name = DRV_NAME,
		.of_match_table = ahci_of_match,
		.acpi_match_table = ahci_acpi_match,
		.pm = &ahci_pm_ops,
	},
};
module_platform_driver(ahci_driver);

MODULE_DESCRIPTION("AHCI SATA platform driver");
MODULE_AUTHOR("Anton Vorontsov <avorontsov@ru.mvista.com>");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:ahci");
