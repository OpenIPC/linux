/*
 * Copyright (c) XMEDIA. All rights reserved.
 */

#include "../xmedia_flash.h"

struct nand_chip_clock g_fmc100_nand_rwlatch[] = {
	/* id_len == 0, means default clock, don't remove. */
	{
		.id 	= {0x00},
		.id_len = 0,
		.value	= 0x457,
	}
};

struct spinand_chip_xfer g_fmc100_spinand_chip_xfer[] = {
	{"AFS4GQ4UAGWC4", {0xc2, 0xd4}, 2, 4, _50MHz},
	{"AFS2GQ4UADWC2", {0xc1, 0x52}, 2, 4, _50MHz},
	{"AFS1GQ4UACWC2", {0xc1, 0x51}, 2, 4, _50MHz},
	{"AFS1GQ4UAAWC2", {0xc8, 0x31}, 2, 4, _50MHz},
	{"GD5F2GQ4UAYIG", {0xC8, 0xF2}, 2, 4, _50MHz},
	{"MX35UF2G14AC",  {0xC2, 0xA0}, 2, 4, _24MHz},

	/* id_len == 0, means default value, don't remove. */
	{NULL, {0x00}, 0, 4, _100MHz},
};

struct spinor_chip_xfer xmedia_fmc100_spinor_chip_xfer[] = {
	{ "MX25L8006E", {0xc2, 0x20, 0x14}, 3,
		.read = SPIFLASH_XFER(SPINOR_OP_READ_QUAD, 1, 0, 80), },

	{ "MX25L6406E", {0xc2, 0x20, 0x17}, 3,
		/* .read = SPINOR_XFER(SPINOR_OP_READ_QUAD, 1, 0, 0), */ },

	{ "GD25Q128", {0xC8, 0x40, 0x18}, 3,
		.read = SPIFLASH_XFER(SPINOR_OP_READ_QUAD, 1, 0, 80),
		.write =  SPIFLASH_XFER(SPINOR_OP_PP_QUAD, 0, 256, 80),},

	{ "GD25Q64", {0xC8, 0x40, 0x17}, 3,
		.read = SPIFLASH_XFER(SPINOR_OP_READ_QUAD, 1, 0, 80),
		.write =  SPIFLASH_XFER(SPINOR_OP_PP_QUAD, 0, 256, 80),},

	{ "GD25Q32", {0xC8, 0x40, 0x16}, 3,
		.read = SPIFLASH_XFER(SPINOR_OP_READ_QUAD, 1, 0, 80),
		.write =  SPIFLASH_XFER(SPINOR_OP_PP_QUAD, 0, 256, 80),},

	/* id_len == 0, means default value, don't remove. */
	{ NULL, {0}, 0, NULL }
};
