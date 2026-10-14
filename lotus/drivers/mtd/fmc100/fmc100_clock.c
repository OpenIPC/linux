/*
 * Copyright (c) LOTUS. All rights reserved.
 */

#include "../lotus_flash.h"

struct nand_chip_clock g_fmc100_nand_rwlatch[] = {
	/* id_len == 0, means default clock, don't remove. */
	{
		.id = {0x00},
		.id_len = 0,
		.value = 0x457,
	}
};

struct spinand_chip_xfer g_fmc100_spinand_chip_xfer[] = {
	{"AFS4GQ4UAGWC4", {0xc2, 0xd4}, 2, 0x44, 50},
	{"AFS2GQ4UADWC2", {0xc1, 0x52}, 2, 0x44, 50},
	{"AFS1GQ4UACWC2", {0xc1, 0x51}, 2, 0x44, 50},
	{"AFS1GQ4UAAWC2", {0xc8, 0x31}, 2, 0x44, 50},
	{"GD5F1GM7UEYIG", {0xc8, 0x91}, 2, 0x44, 133},
	{"GD5F2GQ4UAYIG", {0xC8, 0xF2}, 2, 0x44, 50},
	{"MX35UF2G14AC",  {0xC2, 0xA0}, 2, 0x44, 24},
	{"TC58CVG1S3H",   {0x98, 0xCB}, 2, 0x14, 24},
	{"W25N02KVZEIR",  {0xEF, 0xAA, 0x22}, 3, 0x44, 104},
	{"W25N01KVZEIR",  {0xEF, 0xAE, 0x21}, 3, 0x44, 104},
	{"W25N01GVZEIR",  {0xEF, 0xAA, 0x21}, 3, 0x44, 104},
	{"DS35Q1GA-IB",   {0xE5, 0x71}, 2, 0x44, 104},

	/* id_len == 0, means default value, don't remove. */
	{NULL, {0x00}, 0, 0x44, 133},
};

struct spinor_chip_xfer lotus_fmc100_spinor_chip_xfer[] = {
	{ "MX25L8006E", {0xc2, 0x20, 0x14}, 3,
		.read = SPIFLASH_XFER(SPINOR_OP_READ_QUAD, 1, 0, 80), },

	{ "MX25L6406E", {0xc2, 0x20, 0x17}, 3,
		.read = SPIFLASH_XFER(SPINOR_OP_READ_QUAD, 1, 0, 133),
		.write =  SPIFLASH_XFER(SPINOR_OP_PP_QUAD, 0, 256, 133),},

	{ "MX25L128xx", {0xc2, 0x20, 0x18}, 3,
		.read = SPIFLASH_XFER(SPINOR_OP_READ_QUAD, 1, 0, 80),
		.write =  SPIFLASH_XFER(SPINOR_OP_PP_QUAD, 0, 256, 104),},

	{ "MX25L25645G", {0xc2, 0x20, 0x19}, 3,
		.read = SPIFLASH_XFER(SPINOR_OP_READ_QUAD_4B, 1, 0, 80),
		.write = SPIFLASH_XFER(SPINOR_OP_PP_QUAD_4B, 0, 256, 120),
		.erase = SPIFLASH_XFER(SPINOR_OP_SE_64K_4B, 0, SZ_64K, 120)},

	{ "W25Q64FV", {0xef, 0x40, 0x17}, 3,
		.read = SPIFLASH_XFER(SPINOR_OP_READ_QUAD, 1, 0, 100),
		.write =  SPIFLASH_XFER(SPINOR_OP_PP_QUAD, 0, 256, 100),},

	{ "W25Q128XV", {0xef, 0x40, 0x18}, 3,
		.read = SPIFLASH_XFER(SPINOR_OP_READ_QUAD, 1, 0, 104),
		.write =  SPIFLASH_XFER(SPINOR_OP_PP_QUAD, 0, 256, 104),},

	{ "W25Q256FV", {0xef, 0x40, 0x19}, 3,
		.read = SPIFLASH_XFER(SPINOR_OP_READ_QUAD_4B, 1, 0, 80),
		.write =  SPIFLASH_XFER(SPINOR_OP_PP_QUAD_4B, 0, 256, 80),
		.erase =  SPIFLASH_XFER(SPINOR_OP_SE_64K_4B, 0, SZ_64K, 80)},

	{ "W25Q512NW", {0xef, 0x80, 0x20}, 3,
		.read = SPIFLASH_XFER(SPINOR_OP_READ_QUAD, 1, 0, 133),
		.write =  SPIFLASH_XFER(SPINOR_OP_PP_QUAD, 0, 256, 133),},

	{ "GD25Q32", {0xC8, 0x40, 0x16}, 3,
		.read = SPIFLASH_XFER(SPINOR_OP_READ_QUAD, 1, 0, 80),
		.write =  SPIFLASH_XFER(SPINOR_OP_PP_QUAD, 0, 256, 80),},

	{ "GD25Q64", {0xC8, 0x40, 0x17}, 3,
		.read = SPIFLASH_XFER(SPINOR_OP_READ_QUAD, 1, 0, 80),
		.write =  SPIFLASH_XFER(SPINOR_OP_PP_QUAD, 0, 256, 80),},

	{ "GD25Q128", {0xC8, 0x40, 0x18}, 3,
		.read = SPIFLASH_XFER(SPINOR_OP_READ_QUAD, 1, 0, 104),
		.write =  SPIFLASH_XFER(SPINOR_OP_PP_QUAD, 0, 256, 80),},

	{ "GD25Q128F", {0xC8, 0x43, 0x18}, 3,
		.read = SPIFLASH_XFER(SPINOR_OP_READ_QUAD, 1, 0, 80),
		.write =  SPIFLASH_XFER(SPINOR_OP_PP_QUAD, 0, 256, 80),},

	{ "GD25Q256E", {0xC8, 0x40, 0x19}, 3,
		.read = SPIFLASH_XFER(SPINOR_OP_READ_QUAD_4B, 1, 0, 80),
		.write =  SPIFLASH_XFER(SPINOR_OP_PP_QUAD_4B, 0, 256, 80),
		.erase =  SPIFLASH_XFER(SPINOR_OP_SE_64K_4B, 0, SZ_64K, 80)},

	{ "EN25QH128A", {0x1c, 0x70, 0x18}, 3,
		.read = SPIFLASH_XFER(SPINOR_OP_READ_DUAL, 1, 0, 104),
		.write =  SPIFLASH_XFER(SPINOR_OP_PP, 0, 256, 104),},

	{ "EN25XQ128A", {0x1c, 0x71, 0x18}, 3,
		.read = SPIFLASH_XFER(SPINOR_OP_READ_QUAD, 1, 0, 104),
		.write =  SPIFLASH_XFER(SPINOR_OP_PP_QUAD, 0, 256, 104),},

	{ "XM25QH64C/D", {0x20, 0x40, 0x17}, 3,
		.read = SPIFLASH_XFER(SPINOR_OP_READ_QUAD, 1, 0, 133),
		.write =  SPIFLASH_XFER(SPINOR_OP_PP_QUAD, 0, 256, 133),},

	{ "XM25QH128C/D", {0x20, 0x40, 0x18}, 3,
		.read = SPIFLASH_XFER(SPINOR_OP_READ_QUAD, 1, 0, 133),
		.write =  SPIFLASH_XFER(SPINOR_OP_PP_QUAD, 0, 256, 133),},

	{ "ZB25VQ64A", {0x5e, 0x40, 0x17}, 3,
		.read = SPIFLASH_XFER(SPINOR_OP_READ_QUAD, 1, 0, 104),
		.write =  SPIFLASH_XFER(SPINOR_OP_PP_QUAD, 0, 256, 104),},

	{ "ZB25VQ128A", {0x5e, 0x40, 0x18}, 3,
		.read = SPIFLASH_XFER(SPINOR_OP_READ_QUAD, 1, 0, 104),
		.write =  SPIFLASH_XFER(SPINOR_OP_PP_QUAD, 0, 256, 104),},

	{ "FM25Q64", {0xa1, 0x40, 0x17}, 3,
		.read = SPIFLASH_XFER(SPINOR_OP_READ_QUAD, 1, 0, 104),
		.write =  SPIFLASH_XFER(SPINOR_OP_PP_QUAD, 0, 256, 104),},

	{ "FM25Q128A", {0xa1, 0x40, 0x18}, 3,
		.read = SPIFLASH_XFER(SPINOR_OP_READ_QUAD, 1, 0, 104),
		.write =  SPIFLASH_XFER(SPINOR_OP_PP_QUAD, 0, 256, 104),},

	{ "GM25Q64ASIG", {0x1c, 0x40, 0x17}, 3,
		.read = SPIFLASH_XFER(SPINOR_OP_READ_QUAD, 1, 0, 80),
		.write =  SPIFLASH_XFER(SPINOR_OP_PP_QUAD, 0, 256, 80),},

	{ "GM25Q128ASIG", {0x1c, 0x40, 0x18}, 3,
		.read = SPIFLASH_XFER(SPINOR_OP_READ_QUAD, 1, 0, 80),
		.write =  SPIFLASH_XFER(SPINOR_OP_PP_QUAD, 0, 256, 80),},

	{ "NM25Q64EVB", {0x52, 0x22, 0x17}, 3,
		.read = SPIFLASH_XFER(SPINOR_OP_READ_QUAD, 1, 0, 80),
		.write =  SPIFLASH_XFER(SPINOR_OP_PP_QUAD, 0, 256, 80),},

	{ "NM25Q128EVB", {0x52, 0x21, 0x18}, 3,
		.read = SPIFLASH_XFER(SPINOR_OP_READ_QUAD, 1, 0, 50),
		.write =  SPIFLASH_XFER(SPINOR_OP_PP_QUAD, 0, 256, 50),},

	{ "XT25F64BSS", {0x0B, 0x40, 0x17}, 3,
		.read = SPIFLASH_XFER(SPINOR_OP_READ_QUAD, 1, 0, 72),
		.write =  SPIFLASH_XFER(SPINOR_OP_PP_QUAD, 0, 256, 80),},

	{ "XT25F128BSS", {0x0B, 0x40, 0x18}, 3,
		.read = SPIFLASH_XFER(SPINOR_OP_READ_QUAD, 1, 0, 72),
		.write =  SPIFLASH_XFER(SPINOR_OP_PP_QUAD, 0, 256, 80),},

	{ "P25Q64SH", {0x85, 0x60, 0x17}, 3,
		.read = SPIFLASH_XFER(SPINOR_OP_READ_QUAD_ADDR, 1, 0, 80),
		.write =  SPIFLASH_XFER(SPINOR_OP_PP_QUAD, 0, 256, 80),},

	{ "P25Q128H", {0x85, 0x60, 0x18}, 3,
		.read = SPIFLASH_XFER(SPINOR_OP_READ_QUAD, 1, 0, 104),
		.write =  SPIFLASH_XFER(SPINOR_OP_PP_QUAD, 0, 256, 104),},

	{ "PY25Q64HA", {0x85, 0x20, 0x17}, 3,
		.read = SPIFLASH_XFER(SPINOR_OP_READ_QUAD, 1, 0, 104),
		.write =  SPIFLASH_XFER(SPINOR_OP_PP_QUAD, 0, 256, 104),},

	{ "PY25Q128HA", {0x85, 0x20, 0x18}, 3,
		.read = SPIFLASH_XFER(SPINOR_OP_READ_QUAD, 1, 0, 104),
		.write =  SPIFLASH_XFER(SPINOR_OP_PP_QUAD, 0, 256, 104),},

	/* id_len == 0, means default value, don't remove. */
	{ NULL, {0}, 0, NULL }
};
