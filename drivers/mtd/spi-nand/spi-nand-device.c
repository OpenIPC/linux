/*
 * Copyright (c) 2009-2014 Micron Technology, Inc.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/module.h>
#include <linux/delay.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/partitions.h>
#include <linux/mtd/nand.h>
#include <linux/spi/spi.h>
#include <linux/spi/flash.h>
#include <linux/mtd/spi-nand.h>
#include "spi-nand-ids.h"

#ifdef SPINAND_BBT_DEBUG
#define fh_dev_debug	dev_err
#else
#define fh_dev_debug(...)
#endif

static int spi_nand_read_id(struct spi_nand_chip *chip, u8 *buf)
{
	struct spi_device *spi = chip->spi;
	struct spi_nand_cmd cmd = { 0 };

	cmd.cmd = SPINAND_CMD_READ_ID;
	cmd.n_rx = SPINAND_MAX_ID_LEN;
	cmd.rx_buf = buf;

	return spi_nand_send_cmd(spi, &cmd);
}

static void spi_nand_ecc_status(struct spi_nand_chip *chip, unsigned int status,
				      unsigned int *corrected, unsigned int *ecc_error)
{
	unsigned int ecc_status = (status >> SPI_NAND_ECC_SHIFT) &
				  chip->ecc_mask;

	*ecc_error = (ecc_status >= chip->ecc_uncorr);
	if (*ecc_error == 0)
		*corrected = ecc_status;
}

static void spi_nand_mt29f_ecc_status(unsigned int status,
				      unsigned int *corrected, unsigned int *ecc_error)
{
	unsigned int ecc_status = (status >> SPI_NAND_MT29F_ECC_SHIFT) &
				  SPI_NAND_MT29F_ECC_MASK;

	*ecc_error = (ecc_status == SPI_NAND_MT29F_ECC_UNCORR);
	if (*ecc_error == 0)
		*corrected = ecc_status;
}

static void spi_nand_gd5f_ecc_status(unsigned int status,
				     unsigned int *corrected, unsigned int *ecc_error)
{
	unsigned int ecc_status = (status >> SPI_NAND_GD5F_ECC_SHIFT) &
				  SPI_NAND_GD5F_ECC_MASK;

	*ecc_error = (ecc_status == SPI_NAND_GD5F_ECC_UNCORR);
	/*TODO fix corrected bits*/
	if (*ecc_error == 0)
		*corrected = ecc_status;
}

/*static int spi_nand_manufacture_init(struct spi_nand_chip *chip)
{
	switch (chip->mfr_id) {
	case SPINAND_MFR_MICRON:
		chip->get_ecc_status = spi_nand_mt29f_ecc_status;

		if (chip->page_spare_size == 64)
			chip->ecclayout = &micron_ecc_layout_64;

		chip->bbt_options |= NAND_BBT_NO_OOB;
		break;
	case SPINAND_MFR_GIGADEVICE:
		chip->get_ecc_status = spi_nand_gd5f_ecc_status;
		chip->read_cache = spi_nand_read_from_cache_snor_protocol;
		chip->ecc_strength_ds = 8;
		chip->ecc_step_ds = chip->page_size >> 2;
		if (chip->page_spare_size == 128)
			chip->ecclayout = &gd5f_ecc_layout_128;
		else if (chip->page_spare_size == 256)
			chip->ecclayout = &gd5f_ecc_layout_256;

		break;
	default:
		break;
	}

	return 0;
}*/

static int spi_nand_device_probe(struct spi_device *spi)
{
	struct spi_nand_chip *chip;
	enum spi_nand_device_variant variant;
	struct mtd_info *mtd;
	/*	struct mtd_part_parser_data ppdata;*/
	struct mtd_partition *parts = NULL;
	int nr_parts = 0;
	int ret, i;
	struct flash_platform_data	*data;

	fh_dev_debug(&spi->dev, "%s with spi%d:%d \n", __func__, spi->master->bus_num, spi->chip_select);

	data = spi->dev.platform_data;
	chip = kzalloc(sizeof(struct spi_nand_chip), GFP_KERNEL);
	if (!chip) {
		ret = -ENOMEM;
		goto err1;
	}
	chip->spi = spi;

	mtd = kzalloc(sizeof(struct mtd_info), GFP_KERNEL);
	if (!mtd) {
		ret = -ENOMEM;
		goto err2;
	}
	mtd->priv = chip;
	chip->mtd = mtd;
	spi_set_drvdata(spi, chip);
	/*
	 * read ID command format might be different for different manufactory
	 * such as, Micron SPI NAND need extra one dummy byte after perform
	 * read ID command but Giga device don't need.
	 *
	 * So, specify manufactory of device in device tree is obligatory
	 */
/*	variant = spi_get_device_id(spi)->driver_data;
	switch (variant) {
	case SPI_NAND_MT29F:
		chip->read_id = spi_nand_mt29f_read_id;
		break;
	case SPI_NAND_GD5F:
		chip->read_id = spi_nand_gd5f_read_id;
		break;
	default:
		dev_err(&spi->dev, "unknown device, id %d\n", variant);
		ret = -ENODEV;
		goto err3;
	}*/

	chip->read_id = spi_nand_read_id;
	ret = spi_nand_scan_ident(mtd);
	if (ret){
		ret = -ENODEV;
		goto err3;
	}

/*	spi_nand_manufacture_init(chip);*/
	chip->get_ecc_status = spi_nand_ecc_status;

	ret = spi_nand_scan_tail(mtd);
	if (ret) {
		fh_dev_debug(&spi->dev, "goto err4 %s\n", __func__);
		goto err4;
	}

	/* partitions should match sector boundaries; and it may be good to
	 * use readonly partitions for writeprotected sectors (BP2..BP0).
	 */
	mtd->name = "spi0.0";
	if (mtd_has_cmdlinepart()) {
		static const char *part_probes[] = { "cmdlinepart", NULL, };

		nr_parts = parse_mtd_partitions(mtd, part_probes, &parts, 0);
	}

	if (nr_parts <= 0 && data && data->parts) {
		parts = data->parts;
		nr_parts = data->nr_parts;
	}

	if (nr_parts > 0) {
		for (i = 0; i < nr_parts; i++) {
			DEBUG(MTD_DEBUG_LEVEL2,
			      "partitions[%d] = " "{.name = %s, .offset = 0x%llx, "
			      ".size = 0x%llx (%lldKiB) }\n",
			      i, parts[i].name, (long long)parts[i].offset,
			      (long long)parts[i].size,
			      (long long)(parts[i].size >> 10));
		}
	}

	fh_dev_debug(&spi->dev, " mtd_device_register %s\n", __func__);
	ret = mtd_device_register(mtd, parts, nr_parts);
	if (!ret)
		return 0;

	fh_dev_debug(&spi->dev, " spi_nand_scan_tail_release %s\n", __func__);
	spi_nand_scan_tail_release(mtd);
	fh_dev_debug(&spi->dev, "Leave %s\n", __func__);
err4:
	spi_nand_scan_ident_release(mtd);
err3:
	kfree(mtd);
err2:
	kfree(chip);
err1:
	return ret;
}

int spi_nand_device_remove(struct spi_device *spi)
{
	struct spi_nand_chip *chip = spi_get_drvdata(spi);
	struct mtd_info *mtd = chip->mtd;

	spi_nand_release(mtd);
	kfree(mtd);
	kfree(chip);

	return 0;
}

const struct spi_device_id spi_nand_id_table[] = {
	{ "spi-nand", SPI_NAND_GENERIC},
	{ "mt29f", SPI_NAND_MT29F },
	{ "gd5f", SPI_NAND_GD5F },
	{ },
};
MODULE_DEVICE_TABLE(spi, spi_nand_id_table);

/**
 * module_spi_driver() - Helper macro for registering a SPI driver
 * @__spi_driver: spi_driver struct
 *
 * Helper macro for SPI drivers which do not do anything special in module
 * init/exit. This eliminates a lot of boilerplate. Each module may only
 * use this macro once, and calling it replaces module_init() and module_exit()
 */
#define module_spi_driver(__spi_driver) \
	module_driver(__spi_driver, spi_register_driver, \
			spi_unregister_driver)

static struct spi_driver spi_nand_device_driver = {
	.driver = {
		.name	= "spi-nand",
		.bus	= &spi_bus_type,
		.owner	= THIS_MODULE,
	},
	.id_table = spi_nand_id_table,
	.probe	= spi_nand_device_probe,
	.remove	= spi_nand_device_remove,
};

static int __init spi_nand_init(void)
{
	return spi_register_driver(&spi_nand_device_driver);
}

static void __exit spi_nand_exit(void)
{
	spi_unregister_driver(&spi_nand_device_driver);
}

module_init(spi_nand_init);
module_exit(spi_nand_exit);

MODULE_DESCRIPTION("SPI NAND device");
MODULE_AUTHOR("Peter Pan<peterpandong at micron.com>");
MODULE_AUTHOR("Ezequiel Garcia <ezequiel.garcia at imgtec.com>");
MODULE_LICENSE("GPL v2");

