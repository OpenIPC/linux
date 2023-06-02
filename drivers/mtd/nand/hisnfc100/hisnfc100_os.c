/*
 * Copyright (c) 2016 HiSilicon Technologies Co., Ltd.
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include <linux/of_platform.h>
#include "hisnfc100_os.h"
#include "hisnfc100_spi_ids.h"
#include "hisnfc100.h"

/*****************************************************************************/
static int hisnfc100_os_probe(struct platform_device *pltdev)
{
	int size, result = 0;
	struct hisnfc_host *host;
	struct nand_chip *chip;
	struct mtd_info *mtd;
	struct resource *res1, *res2 = NULL;
	struct device *dev = &pltdev->dev;
	struct device_node *np = NULL;

	size = sizeof(struct hisnfc_host) + sizeof(struct nand_chip)
		+ sizeof(struct mtd_info);
	host = kmalloc(size, GFP_KERNEL);
	if (!host) {
		PR_BUG("failed to allocate device structure.\n");
		return -ENOMEM;
	}
	memset((char *)host, 0, size);
	platform_set_drvdata(pltdev, host);

	host->dev  = dev;
	host->chip = chip = (struct nand_chip *)&host[1];
	host->mtd  = mtd  = nand_to_mtd(chip);;

	host->clk = devm_clk_get(dev, NULL);
	if (IS_ERR(host->clk))
		return PTR_ERR(host->clk);
	/* enable and set system clock */
	clk_prepare_enable(host->clk);

	res1 = platform_get_resource_byname(pltdev, IORESOURCE_MEM, "control");
	host->regbase = devm_ioremap_resource(dev, res1);
	if (IS_ERR(host->iobase)) {
		PR_BUG("Error: Can't get resource for reg address.\n");
		result = -EIO;
		goto fail;
	}

	res2 = platform_get_resource_byname(pltdev, IORESOURCE_MEM, "memory");
	host->iobase = devm_ioremap_resource(dev, res2);
	if (IS_ERR(host->iobase)) {
		PR_BUG("Error: Can't get resource for buffer address.\n");
		result = -EIO;
		goto fail;
	}

	memset((char *)host->iobase, 0xff, HISNFC100_BUFFER_BASE_ADDRESS_LEN);
	chip->IO_ADDR_R = chip->IO_ADDR_W = host->iobase;

	host->buffer = dma_alloc_coherent(host->dev, HISNFC100_BUFFER_LEN,
						&host->dma_buffer, GFP_KERNEL);
	if (!host->buffer) {
		PR_BUG("Can't malloc memory for SPI Nand driver.");
		result = -ENOMEM;
		goto fail;
	}
	memset(host->buffer, 0xff, HISNFC100_BUFFER_LEN);

	np = of_get_next_available_child(dev->of_node, NULL);
	mtd->priv  = chip;
	mtd->owner = THIS_MODULE;
	mtd->type = MTD_NANDFLASH;
	mtd->name = np->name;

	result = hisnfc100_host_init(host);
	if (result)
		return result;

	chip->priv = host;
	hisnfc100_nand_init(chip);

	spi_nand_ids_register();
	hinfc_param_adjust = hisnfc100_ecc_probe;

	if (nand_scan(mtd, CONFIG_HISNFC100_MAX_CHIP)) {
		result = -ENXIO;
		goto fail;
	}

	result = mtd_device_register(mtd, NULL, 0);
	if (result)
		goto fail;

	return result;

fail:
	if (host->buffer) {
		dma_free_coherent(host->dev, HISNFC100_BUFFER_LEN,
					host->buffer, host->dma_buffer);
		host->buffer = NULL;
	}
	nand_release(host->mtd);
	kfree(host);
	platform_set_drvdata(pltdev, NULL);

	return result;
}
/*****************************************************************************/

static int hisnfc100_os_remove(struct platform_device *pltdev)
{
	struct hisnfc_host *host = platform_get_drvdata(pltdev);

	clk_disable_unprepare(host->clk);

	nand_release(host->mtd);
	dma_free_coherent(host->dev,
				(SPI_NAND_MAX_PAGESIZE + SPI_NAND_MAX_OOBSIZE),
				host->buffer, host->dma_buffer);
	kfree(host);
	platform_set_drvdata(pltdev, NULL);

	return 0;
}
/*****************************************************************************/
#ifdef CONFIG_PM
/*****************************************************************************/
static int hisnfc100_os_suspend(struct platform_device *pltdev,
			       pm_message_t state)
{
	return 0;
}

/*****************************************************************************/
static int hisnfc100_os_resume(struct platform_device *pltdev)
{
	return 0;
}
#endif /* CONFIG_PM */

/*****************************************************************************/
static const struct of_device_id hisi_spi_nand_dt_ids[] = {
	{ .compatible = "hisilicon,hisi-spi-nand" },
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, hisi_spi_nand_dt_ids);

static struct platform_driver hisi_nand_driver = {
	.driver = {
		.name	= "hisi-nand",
		.of_match_table = hisi_spi_nand_dt_ids,
	},
	.probe	= hisnfc100_os_probe,
	.remove = hisnfc100_os_remove,
#ifdef CONFIG_PM
	.suspend	= hisnfc100_os_suspend,
	.resume		= hisnfc100_os_resume,
#endif
};
module_platform_driver(hisi_nand_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("BVT_BSP");
MODULE_DESCRIPTION("Hisilicon snfc Device Driver, Version 100");

