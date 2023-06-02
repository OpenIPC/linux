/*
 * Copyright (c) 2016 Fullhan, Inc.
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


#ifndef DRIVERS_MTD_SPI_NAND_SPI_NAND_IDS_H_
#define DRIVERS_MTD_SPI_NAND_SPI_NAND_IDS_H_

enum spi_nand_device_variant {
	SPI_NAND_GENERIC,
	SPI_NAND_MT29F,
	SPI_NAND_GD5F,
};


bool spi_nand_scan_id_table(struct spi_nand_chip *chip, u8 *id);

#endif /* DRIVERS_MTD_SPI_NAND_SPI_NAND_IDS_H_ */
