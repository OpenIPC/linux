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

#define ONE_WIRE_SUPPORT		(1<<0)
#define DUAL_WIRE_SUPPORT		(1<<1)
#define QUAD_WIRE_SUPPORT		(1<<2)
#define MULTI_WIRE_SUPPORT		(1<<8)

static int spi_nand_read_id(struct spi_nand_chip *chip, u8 *buf)
{
	struct spi_device *spi = chip->spi;
	struct spi_nand_cmd cmd = { 0 };

	cmd.cmd = SPINAND_CMD_READ_ID;

	if (chip->read_id_dummy != SPI_NAND_ID_NO_DUMMY) {
		cmd.n_addr = 1;
		cmd.addr[0] = 0x00;
	}

	cmd.n_rx = SPINAND_MAX_ID_LEN;
	cmd.rx_buf = buf;

	return spi_nand_send_cmd(spi, &cmd);
}

static void spi_nand_ecc_status(struct spi_nand_chip *chip, unsigned int status,
				      unsigned int *corrected, unsigned int *ecc_error)
{
	unsigned int ecc_status = (status >> SPI_NAND_ECC_SHIFT) &
				  chip->ecc_mask;

	*ecc_error = (ecc_status == chip->ecc_uncorr);
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
	struct mtd_info *mtd;
	/*	struct mtd_part_parser_data ppdata;*/
	int ret;
	struct flash_platform_data	*data;

	fh_dev_debug(&spi->dev, "%s with spi%d:%d \n", __func__, 
		spi->master->bus_num, spi->chip_select);
 
	data = dev_get_platdata(&spi->dev);
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
	dev_set_drvdata(&spi->dev, chip);
	/*spi_set_drvdata(spi, chip);*/
	mtd->dev.parent = &spi->dev;
	chip->read_id = spi_nand_read_id;
	ret = spi_nand_scan_ident(mtd);
	if (ret) {
		ret = -ENODEV;
		goto err3;
	}

/*	spi_nand_manufacture_init(chip);*/
	chip->get_ecc_status = spi_nand_ecc_status;
	mtd->dev.of_node = spi->dev.of_node;
	mtd->name = dev_name(mtd->dev.parent);
	ret = spi_nand_scan_tail(mtd);
	if (ret) {
		fh_dev_debug(&spi->dev, "goto err4 %s\n", __func__);
		goto err4;
	}

	fh_dev_debug(&spi->dev, " mtd_device_register %s\n", __func__);
	ret = mtd_device_register(mtd, data ? data->parts : NULL,
				   data ? data->nr_parts : 0);
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
void spi_nand_shutdown(struct spi_device *spi)
{
	struct spi_nand_cmd cmd = {0};

	/*  before reset delay 1ms */
	udelay(1000);
	cmd.cmd = SPINAND_CMD_RESET;
	if (spi_nand_send_cmd(spi, &cmd) < 0)
		pr_err("spi_nand reset failed!\n");

	/* elapse 1ms before issuing any other command */
	udelay(1000);

}


static const struct of_device_id spinand_dt[] = {
	{ .compatible = "fh spi-nand", },
	{}
};
MODULE_DEVICE_TABLE(of, spinand_dt);

/*
 * Device name structure description
 */
static struct spi_driver spinand_driver = {
	.driver = {
		.name		= "spi-nand",
		.of_match_table	= spinand_dt,
	},
	.probe		= spi_nand_device_probe,
	.remove		= spi_nand_device_remove,
	.shutdown = spi_nand_shutdown,
};

module_spi_driver(spinand_driver);

MODULE_DESCRIPTION("Fullhan SPI NAND device driver");
MODULE_AUTHOR("Peter Pan<peterpandong at micron.com>");
MODULE_AUTHOR("Ezequiel Garcia <ezequiel.garcia at imgtec.com>");
MODULE_LICENSE("GPL v2");

