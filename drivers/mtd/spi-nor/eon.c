// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2005, Intec Automation Inc.
 * Copyright (C) 2014, Freescale Semiconductor, Inc.
 */

#include <linux/mtd/spi-nor.h>

#include "core.h"

#ifdef CONFIG_ARCH_XMEDIA
static int
eon_post_bfpt_fixups(struct spi_nor *nor,
			    const struct sfdp_parameter_header *bfpt_header,
			    const struct sfdp_bfpt *bfpt,
			    struct spi_nor_flash_parameter *params)
{
	struct spi_nor_read_command read =       SNOR_OP_READ(0, 0, SPINOR_OP_READ, SNOR_PROTO_1_1_1);
	struct spi_nor_read_command read_fast =  SNOR_OP_READ(0, 8, SPINOR_OP_READ_FAST,  SNOR_PROTO_1_1_1);
	struct spi_nor_read_command read_1_1_2 = SNOR_OP_READ(0, 8, SPINOR_OP_READ_1_1_2, SNOR_PROTO_1_1_2);
	struct spi_nor_read_command read_1_2_2 = SNOR_OP_READ(8, 0, SPINOR_OP_READ_1_2_2, SNOR_PROTO_1_2_2);
	struct spi_nor_read_command read_1_1_4 = SNOR_OP_READ(0, 8, SPINOR_OP_READ_1_1_4, SNOR_PROTO_1_1_4);
	struct spi_nor_read_command read_1_4_4 = SNOR_OP_READ(8, 16, SPINOR_OP_READ_1_4_4,SNOR_PROTO_1_4_4);
	struct spi_nor_pp_command PP =       SNOR_OP_PROGRAMS(SPINOR_OP_PP,SNOR_PROTO_1_1_1);

	params->hwcaps.mask = SNOR_EON_RD_MODES | SNOR_EON_WR_MODES;
	memcpy(&params->reads[SNOR_CMD_READ], &read, sizeof(read));
	memcpy(&params->reads[SNOR_CMD_READ_FAST], &read_fast, sizeof(read_fast));
	memcpy(&params->reads[SNOR_CMD_READ_1_1_2], &read_1_1_2, sizeof(read_1_1_2));
	memcpy(&params->reads[SNOR_CMD_READ_1_2_2], &read_1_2_2, sizeof(read_1_2_2));
	memcpy(&params->reads[SNOR_CMD_READ_1_1_4], &read_1_1_4, sizeof(read_1_1_4));
	memcpy(&params->reads[SNOR_CMD_READ_1_4_4], &read_1_4_4, sizeof(read_1_4_4));

	memcpy(&params->page_programs[SNOR_CMD_PP], &PP, sizeof(PP));

	return 0;
}

static struct spi_nor_fixups g_eon_fixups = {
	.post_bfpt = eon_post_bfpt_fixups,
};
#endif

static const struct flash_info eon_parts[] = {
	/* EON -- en25xxx */
	{ "en25f32",    INFO(0x1c3116, 0, 64 * 1024,   64, SECT_4K) },
	{ "en25p32",    INFO(0x1c2016, 0, 64 * 1024,   64, 0) },
	{ "en25p64",    INFO(0x1c2017, 0, 64 * 1024,  128, 0) },
#ifdef CONFIG_ARCH_XMEDIA
	{ "en25q32b",	INFO(0x1c3016, 0, 64 * 1024,   64, SPI_NOR_QUAD_READ)
			     .fixups = &g_eon_fixups, CLK_MHZ_2X(104) },
	{ "en25q64",	INFO(0x1c3017, 0, 64 * 1024,  128, SECT_4K | SPI_NOR_QUAD_READ)
			     .fixups = &g_eon_fixups, CLK_MHZ_2X(104) },
	{ "en25qh64a",	  INFO(0x1c7017, 0, 64 * 1024,	128, SPI_NOR_QUAD_READ)
			     .fixups = &g_eon_fixups, CLK_MHZ_2X(104) },
	{ "en25q128",   INFO(0x1c3018, 0, 64 * 1024,  256, SPI_NOR_QUAD_READ)
			     .fixups = &g_eon_fixups, CLK_MHZ_2X(104) },
	{ "en25xq128a", INFO(0x1c7118, 0, 64 * 1024,  256, SPI_NOR_QUAD_READ)
			     .fixups = &g_eon_fixups, CLK_MHZ_2X(104) },
	{ "en25qh128a", INFO(0x1c7018, 0, 64 * 1024,  256, SPI_NOR_QUAD_READ)
			     .fixups = &g_eon_fixups, CLK_MHZ_2X(104) },
#else
	{ "en25q32b",	INFO(0x1c3016, 0, 64 * 1024,   64, 0) },
	{ "en25q64",	INFO(0x1c3017, 0, 64 * 1024,  128, SECT_4K) },
	{ "en25xq128a", INFO(0x1c7118, 0, 64 * 1024,  256, 0) },
#endif
	{ "en25q80a",   INFO(0x1c3014, 0, 64 * 1024,   16,
			     SECT_4K | SPI_NOR_DUAL_READ) },
	{ "en25qh16",   INFO(0x1c7015, 0, 64 * 1024,   32,
			     SECT_4K | SPI_NOR_DUAL_READ) },
	{ "en25qh32",   INFO(0x1c7016, 0, 64 * 1024,   64, 0) },
	{ "en25qh64",   INFO(0x1c7017, 0, 64 * 1024,  128,
			     SECT_4K | SPI_NOR_DUAL_READ) },
	{ "en25qh128",  INFO(0x1c7018, 0, 64 * 1024,  256, 0) },
	{ "en25qh256",  INFO(0x1c7019, 0, 64 * 1024,  512, 0) },
	{ "en25s64",	INFO(0x1c3817, 0, 64 * 1024,  128, SECT_4K) },
	{ "en25q128",   INFO(0x1c3018, 0, 64 * 1024,  256, 0) },
};

const struct spi_nor_manufacturer spi_nor_eon = {
	.name = "eon",
	.parts = eon_parts,
	.nparts = ARRAY_SIZE(eon_parts),
};
