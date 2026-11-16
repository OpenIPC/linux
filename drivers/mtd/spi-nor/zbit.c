// SPDX-License-Identifier: GPL-2.0
/*
 * Zbit Semiconductor SPI NOR flash parts
 *
 * Copyright (C) 2026 Difan Zhang
 */

#include <linux/mtd/spi-nor.h>

#include "core.h"

static void zbit_nor_default_init(struct spi_nor *nor)
{
	nor->params->quad_enable = spi_nor_sr2_bit1_quad_enable;
}

static const struct spi_nor_fixups zbit_nor_fixups = {
	.default_init = zbit_nor_default_init,
};

static const struct flash_info zbit_nor_parts[] = {
	{
		.id = SNOR_ID(0x5e, 0x40, 0x17),
		.name = "zb25vq64",
		.size = SZ_8M,
		.no_sfdp_flags = SECT_4K | SPI_NOR_DUAL_READ |
				 SPI_NOR_QUAD_READ | SPI_NOR_HAS_LOCK,
	}, {
		.id = SNOR_ID(0x5e, 0x40, 0x18),
		.name = "zb25vq128",
		.size = SZ_16M,
		.no_sfdp_flags = SECT_4K | SPI_NOR_DUAL_READ |
				 SPI_NOR_QUAD_READ | SPI_NOR_HAS_LOCK,
	},
};

const struct spi_nor_manufacturer spi_nor_zbit = {
	.name = "zbit",
	.parts = zbit_nor_parts,
	.nparts = ARRAY_SIZE(zbit_nor_parts),
	.fixups = &zbit_nor_fixups,
};
