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

#ifdef CONFIG_MTD_PARTITION_FROM_DTS
/*****************************************************************************/
static inline int mtd_has_partitions(void) { return 1; }
#else

/*****************************************************************************/
#define MAX_MTD_PARTITIONS         (32)

struct partition_entry {
	char name[16];
	unsigned long long start;
	unsigned long long length;
	unsigned int flags;
};

struct partition_info {
	int parts_num;
	struct partition_entry entry[MAX_MTD_PARTITIONS];
	struct mtd_partition parts[MAX_MTD_PARTITIONS];
};

static struct partition_info ptn_info = {0};

static int __init parse_nand_partitions(const struct tag *tag)
{
	int i;

	if (tag->hdr.size <= 2) {
		PR_BUG("tag->hdr.size <= 2\n");
		return 0;
	}
	ptn_info.parts_num = (tag->hdr.size - 2)
		/ (sizeof(struct partition_entry)/sizeof(int));
	memcpy(ptn_info.entry,
		&tag->u,
		ptn_info.parts_num * sizeof(struct partition_entry));

	for (i = 0; i < ptn_info.parts_num; i++) {
		ptn_info.parts[i].name   = ptn_info.entry[i].name;
		ptn_info.parts[i].size   = (ptn_info.entry[i].length);
		ptn_info.parts[i].offset = (ptn_info.entry[i].start);
		ptn_info.parts[i].mask_flags = 0;
		ptn_info.parts[i].ecclayout  = 0;
	}

	return 0;
}

/* turn to ascii is "HiNp" */
__tagtable(0x48694E70, parse_nand_partitions);
/*****************************************************************************/
static int hinfc_os_add_paratitions(struct hinfc_host *host)
{
	int ix;
	int nr_parts = 0;
	struct mtd_partition *parts = NULL;
	int ret;

#ifdef CONFIG_MTD_CMDLINE_PARTS
	static const char * const part_probes[] = {"cmdlinepart", NULL, };

	nr_parts = parse_mtd_partitions(host->mtd, part_probes, &parts, 0);
#endif

	if (!nr_parts) {
		nr_parts = ptn_info.parts_num;
		parts    = ptn_info.parts;
	}

	if (nr_parts <= 0)
		return 0;

	for (ix = 0; ix < nr_parts; ix++) {
		DBG_MSG("partitions[%d] = {.name = %s, .offset = 0x%.8x,",
			ix, parts[ix].name,
			(unsigned int)parts[ix].offset);
		DBG_MSG(".size = 0x%08x (%uKiB) }\n",
			(unsigned int)parts[ix].size,
			(unsigned int)parts[ix].size/1024);
	}

	host->add_partition = 1;

	ret = mtd_device_register(host->mtd, parts, nr_parts);

	kfree(parts);
	parts = NULL;

	return (1 == ret) ? -ENODEV : 0;
}
/*****************************************************************************/
#endif /* CONFIG_MTD_PARTITION_FROM_DTS */

static unsigned int  nand_otp_len;
static unsigned char nand_otp[128] = {0};

/* Get NAND parameter table. */
static int __init parse_nand_param(const struct tag *tag)
{
	if (tag->hdr.size <= 2)
		return 0;

	nand_otp_len = ((tag->hdr.size << 2) - sizeof(struct tag_header));

	if (nand_otp_len > sizeof(nand_otp)) {
		PR_BUG("tag->hdr.size <= 2\n");
		return 0;
	}
	memcpy(nand_otp, &tag->u, nand_otp_len);
	return 0;
}
/* 0x48694E77 equal to fastoot ATAG_NAND_PARAM */
__tagtable(0x48694E77, parse_nand_param);
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
#ifdef CONFIG_MTD_PARTITION_FROM_DTS
	struct mtd_partition *parts = NULL;
	int nr_parts = 0;
#endif

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
	mtd->priv  = chip;
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
	chip->options    = NAND_NO_AUTOINCR
			| NAND_NEED_READRDY
			| NAND_BROKEN_XD
			| NAND_SKIP_BBTSCAN;
	chip->ecc.layout = NULL;
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

	if (nand_otp_len) {
		PR_MSG("Copy Nand read retry parameter from boot,");
		PR_MSG(" parameter length %d.\n", nand_otp_len);
		memcpy(host->rr_data, nand_otp, nand_otp_len);
	}

	if (nand_scan(mtd, CONFIG_HINFC610_MAX_CHIP)) {
		result = -ENXIO;
		goto fail;
	}

#ifdef CONFIG_MTD_PARTITION_FROM_DTS
	if (mtd_has_partitions()) {
		static char const *part_probes[] = {
			"cmdlinepart",
			NULL,
		};

		nr_parts = parse_mtd_partitions(host->mtd,
				part_probes, &parts, 0);
		PR_MSG("parse mtd partitions: %d\n", nr_parts);
		if (nr_parts > 0)
			host->add_partition = 1;
	}

	result = mtd_device_register(host->mtd, parts, nr_parts);
	if (result) {
		kfree(parts);
		parts = NULL;
	}
	return (1 == result) ? -ENODEV : 0;
#else
	result = hinfc_os_add_paratitions(host);
	if (host->add_partition)
		return result;

	if (!add_mtd_device(host->mtd))
		return 0;
#endif
	result = -ENODEV;
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
	{ .compatible = "hisilicon,hinfc610-nand" },
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
