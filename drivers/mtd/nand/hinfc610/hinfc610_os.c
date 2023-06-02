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
#include "hinfc610_os.h"
#include "hinfc610.h"

/*****************************************************************************/

static int hinfc610_nand_pre_probe(struct nand_chip *chip)
{
	uint8_t nand_maf_id;
	struct hinfc_host *host = chip->priv;

	/* Reset the chip first */
	host->send_cmd_reset(host, 0);

	/* Check the ID */
	host->offset = 0;
	memset((unsigned char *)(chip->IO_ADDR_R), 0, 0x10);
	host->send_cmd_readid(host);
	nand_maf_id = readb(chip->IO_ADDR_R);

	if (nand_maf_id == 0x00 || nand_maf_id == 0xff) {
		PR_BUG("\nCannot found a valid Nand Device\n");
		return 1;
	}

	return 0;
}
/*****************************************************************************/

static int hinfc610_os_probe(struct platform_device *pltdev)
{
	int size;
	int result = 0;
	struct hinfc_host *host;
	struct nand_chip *chip;
	struct mtd_info *mtd;
	struct resource *rs_reg, *rs_io = NULL;
	struct device *dev = &pltdev->dev;
	struct device_node *np = NULL;

	size = sizeof(struct hinfc_host) + sizeof(struct nand_chip)
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
	host->mtd  = mtd  = (struct mtd_info *)&chip[1];

	host->clk = devm_clk_get(dev, NULL);
	if (IS_ERR(host->clk))
		return PTR_ERR(host->clk);
	/* enable and set system clock */
	clk_prepare_enable(host->clk);

	rs_reg = platform_get_resource_byname(pltdev, IORESOURCE_MEM,
				"control");
	host->iobase = devm_ioremap_resource(dev, rs_reg);
	if (IS_ERR(host->iobase)) {
		PR_BUG("Error: Can't get resource for reg address.\n");
		result = -EIO;
		goto fail;
	}

	np = of_get_next_available_child(dev->of_node, NULL);

	mtd->type = MTD_NANDFLASH;
	mtd = nand_to_mtd(chip);
	mtd->flags = MTD_CAP_NANDFLASH;
	mtd->owner = THIS_MODULE;
	mtd->name = np->name;

	rs_io = platform_get_resource_byname(pltdev, IORESOURCE_MEM,
					"memory");
	chip->IO_ADDR_R = chip->IO_ADDR_W = devm_ioremap_resource(dev, rs_io);
	if (IS_ERR(chip->IO_ADDR_R)) {
		PR_BUG("Error: Can't get resource for buffer address.\n");
		result = -EIO;
		goto fail;
	}

	host->buffer = dma_alloc_coherent(host->dev,
		(NAND_MAX_PAGESIZE + NAND_MAX_OOBSIZE),
		&host->dma_buffer, GFP_KERNEL);
	if (!host->buffer) {
		PR_BUG("Can't malloc memory for NAND driver.");
		result = -EIO;
		goto fail;
	}

	chip->priv        = host;
	host->chip        = chip;
	chip->cmd_ctrl    = hinfc610_cmd_ctrl;
	chip->dev_ready   = hinfc610_dev_ready;
	chip->select_chip = hinfc610_select_chip;
	chip->read_byte   = hinfc610_read_byte;
	chip->read_word   = hinfc610_read_word;
	chip->write_buf   = hinfc610_write_buf;
	chip->read_buf    = hinfc610_read_buf;

	chip->chip_delay = HINFC610_CHIP_DELAY;
	chip->options    = NAND_NEED_READRDY
			| NAND_BROKEN_XD
			| NAND_SKIP_BBTSCAN;
	chip->ecc.mode   = NAND_ECC_NONE;

	if (hinfc610_nand_init(host, chip)) {
		PR_BUG("failed to allocate device buffer.\n");
		result = -EIO;
		goto fail;
	}

	if (hinfc610_nand_pre_probe(chip)) {
		result = -EXDEV;
		goto fail;
	}

	if (nand_scan(mtd, CONFIG_HINFC610_MAX_CHIP)) {
		result = -ENXIO;
		goto fail;
	}

	result = mtd_device_register(mtd, NULL, 0);
	if (result)
		goto fail;

	return result;

fail:
	if (host->buffer) {
		dma_free_coherent(host->dev,
			(NAND_MAX_PAGESIZE + NAND_MAX_OOBSIZE),
			host->buffer,
			host->dma_buffer);
		host->buffer = NULL;
	}
	nand_release(host->mtd);
	kfree(host);
	platform_set_drvdata(pltdev, NULL);

	return result;
}
/*****************************************************************************/

static int hinfc610_os_remove(struct platform_device *pltdev)
{
	struct hinfc_host *host = platform_get_drvdata(pltdev);

	clk_disable_unprepare(host->clk);

	nand_release(host->mtd);

	dma_free_coherent(host->dev,
		(NAND_MAX_PAGESIZE + NAND_MAX_OOBSIZE),
		host->buffer,
		host->dma_buffer);
	kfree(host);
	platform_set_drvdata(pltdev, NULL);

	return 0;
}
/*****************************************************************************/
#ifdef CONFIG_PM
static int hinfc610_os_suspend(struct platform_device *pltdev,
			       pm_message_t state)
{
	struct hinfc_host *host = platform_get_drvdata(pltdev);

	while ((hinfc_read(host, HINFC610_STATUS) & 0x1) == 0x0)
		;

	while ((hinfc_read(host, HINFC610_DMA_CTRL))
		& HINFC610_DMA_CTRL_DMA_START)
		_cond_resched();

	clk_disable_unprepare(host->clk);

	return 0;
}
/*****************************************************************************/

static int hinfc610_os_resume(struct platform_device *pltdev)
{
	int cs;
	struct hinfc_host *host = platform_get_drvdata(pltdev);
	struct nand_chip *chip = host->chip;

	clk_prepare_enable(host->clk);
	for (cs = 0; cs < chip->numchips; cs++)
		host->send_cmd_reset(host, cs);
	hinfc_write(host,
		SET_HINFC610_PWIDTH(CONFIG_HINFC610_W_LATCH,
			CONFIG_HINFC610_R_LATCH, CONFIG_HINFC610_RW_LATCH),
		HINFC610_PWIDTH);

	return 0;
}
#endif /* CONFIG_PM */
/*****************************************************************************/
static const struct of_device_id hisi_nand_dt_ids[] = {
	{ .compatible = "hisilicon,hisi-parallel-nand" },
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, hisi_nand_dt_ids);

static struct platform_driver hisi_nand_driver = {
	.driver = {
		.name	= "hisi-nand",
		.of_match_table = hisi_nand_dt_ids,
	},
	.probe	= hinfc610_os_probe,
	.remove = hinfc610_os_remove,
#ifdef CONFIG_PM
	.suspend	= hinfc610_os_suspend,
	.resume		= hinfc610_os_resume,
#endif
};
module_platform_driver(hisi_nand_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("BVT_BSP");
MODULE_DESCRIPTION("Hisilicon Flash Memory Controller NFC610 Nand Driver");
