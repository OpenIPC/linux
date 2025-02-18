/*
 * Copyright (c) XMEDIA. All rights reserved.
 */

#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/dma-mapping.h>
#include <linux/mtd/nand.h>
#include <linux/mtd/partitions.h>
#include <linux/of_platform.h>
#include <asm/setup.h>
#include <linux/mtd/rawnand.h>

#include "../../mtdcore.h"
#include "xmedia_fmc100.h"

static int xmedia_fmc100_spi_nand_pre_probe(struct nand_chip *chip)
{
	uint8_t nand_maf_id;
	struct xmedia_fmc_host *host = chip->priv;

	/* Reset the chip first */
	host->send_cmd_reset(host);
	udelay(1000);

	/* Check the ID */
	host->offset = 0;
	memset((unsigned char *)(chip->legacy.IO_ADDR_R), 0, 0x10);
	host->send_cmd_readid(host);
	nand_maf_id = fmc_readb(chip->legacy.IO_ADDR_R);

	if (nand_maf_id == 0x00 || nand_maf_id == 0xff) {
		printk("Cannot found a valid SPI Nand Device\n");
		return 1;
	}

	return 0;
}

static int xmedia_fmc_nand_scan(struct mtd_info *mtd)
{
	int result = 0;
	unsigned char cs;
	unsigned char chip_num = CONFIG_SPI_NAND_MAX_CHIP_NUM;
	struct nand_chip *chip = mtd_to_nand(mtd);
	struct xmedia_fmc_host *host = chip->priv;

	for (cs = 0; chip_num && (cs < FMC_MAX_CHIP_NUM); cs++) {
		if (fmc_cs_user[cs]) {
			FMC_PR(BT_DBG, "\t\t*-Current CS(%d) is occupied.\n",
			       cs);
			continue;
		}

		host->cmd_op.cs = cs;

		if (xmedia_fmc100_spi_nand_pre_probe(chip)) {
			return -ENODEV;
		}

		FMC_PR(BT_DBG, "\t\t*-Scan SPI nand flash on CS: %d\n", cs);
		if (nand_scan_with_ids(chip, chip_num, NULL)) {
			continue;
		}
		chip_num--;
	}

	if (chip_num == CONFIG_SPI_NAND_MAX_CHIP_NUM) {
		result = -ENXIO;
	} else {
		result = 0;
	}

	return result;
}

static int xmedia_spi_nand_probe(struct platform_device *pltdev)
{
	int len;
	int result = 0;
	struct xmedia_fmc_host *host = NULL;
	struct nand_chip *chip = NULL;
	struct mtd_info *mtd = NULL;
	struct device *dev = &pltdev->dev;
	struct device_node *np = NULL;
	struct bsp_fmc *fmc = dev_get_drvdata(dev->parent);

	FMC_PR(BT_DBG, "\t*-Start SPI Nand flash driver probe\n");

	if (!fmc) {
		dev_err(dev, "get mfd fmc devices failed\n");
		return -ENXIO;
	}

	len = sizeof(struct xmedia_fmc_host) + sizeof(struct nand_chip) +
	      sizeof(struct mtd_info);
	host = devm_kzalloc(dev, len, GFP_KERNEL);
	if (!host) {
		return -ENOMEM;
	}
	memset((char *)host, 0, len);

	platform_set_drvdata(pltdev, host);
	host->dev = &pltdev->dev;

	host->chip = chip = (struct nand_chip *)&host[1];
	host->mtd  = mtd  = nand_to_mtd(chip);

	host->regbase = fmc->regbase;
	host->iobase = fmc->iobase;
	host->clk = fmc->clk;
	host->lock = &fmc->lock;
	host->buffer = fmc->buffer;
	host->dma_buffer = fmc->dma_buffer;

	memset((char *)host->iobase, 0xff, fmc->dma_len);
	chip->legacy.IO_ADDR_R = chip->legacy.IO_ADDR_W = host->iobase;

	chip->priv = host;

	/* Set system clock */
	result = clk_prepare_enable(host->clk);
	if (result) {
		printk("\nclk prepare enable failed!");
		goto fail;
	}

	xmedia_fmc100_spi_nand_init(chip);

	np = of_get_next_available_child(dev->of_node, NULL);
	if (np == NULL) {
		printk("\nof_get_next_available_child failed!");
		goto fail;
	}
	mtd->name = np->name;
	mtd->type = MTD_NANDFLASH;
	mtd->priv = chip;
	mtd->owner = THIS_MODULE;

	result = of_property_read_u32(np, "spi-max-frequency", &host->clkrate);
	if (result) {
		printk("\nget fmc clkrate failed");
		goto fail;
	}

	result = xmedia_fmc_nand_scan(mtd);
	if (result) {
		FMC_PR(BT_DBG, "\t|-Scan SPI Nand failed.\n");
		goto fail;
	}

	result = mtd_device_register(mtd, NULL, 0);
	if (!result) {
		FMC_PR(BT_DBG, "\t*-End driver probe !!\n");
		return 0;
	}

	result = -ENODEV;
	nand_cleanup(mtd_to_nand(mtd));
fail:
	clk_disable_unprepare(host->clk);
	DB_MSG("Error: driver probe, result: %d\n", result);
	return result;
}

static int xmedia_spi_nand_remove(struct platform_device *pltdev)
{
	struct xmedia_fmc_host *host = platform_get_drvdata(pltdev);
	int ret;

	if (host) {
		if (host->clk)
			clk_disable_unprepare(host->clk);
		if (host->mtd) {
			ret = mtd_device_unregister(host->mtd);
			WARN_ON(ret);
			nand_cleanup(mtd_to_nand(host->mtd));
		}
	}

	return 0;
}

#ifdef CONFIG_PM
static int xmedia_fmc100_os_suspend(struct platform_device *pltdev,
			       pm_message_t state)
{
	struct xmedia_fmc_host *host = platform_get_drvdata(pltdev);

	if (host && host->suspend) {
		return (host->suspend)(pltdev, state);
	}

	return 0;
}

static int xmedia_fmc100_os_resume(struct platform_device *pltdev)
{
	struct xmedia_fmc_host *host = platform_get_drvdata(pltdev);

	if (host && host->resume) {
		return (host->resume)(pltdev);
	}

	return 0;
}
#endif /* End of CONFIG_PM */

static const struct of_device_id xmedia_spi_nand_dt_ids[] = {
	{ .compatible = "xmedia,fmc-spi-nand" },
	{ } /* sentinel */
};
MODULE_DEVICE_TABLE(of, xmedia_spi_nand_dt_ids);

static struct platform_driver xmedia_spi_nand_driver = {
	.driver = {
		.name   = "xmedia_spi_nand",
		.of_match_table = xmedia_spi_nand_dt_ids,
	},
	.probe  = xmedia_spi_nand_probe,
	.remove = xmedia_spi_nand_remove,
#ifdef CONFIG_PM
	.suspend    = xmedia_fmc100_os_suspend,
	.resume     = xmedia_fmc100_os_resume,
#endif
};
module_platform_driver(xmedia_spi_nand_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Xmedia");
MODULE_DESCRIPTION("Xmedia Flash Memory Controller V100 SPI Nand Driver");
