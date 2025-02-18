// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2005, Intec Automation Inc.
 * Copyright (C) 2014, Freescale Semiconductor, Inc.
 */

#include <linux/mtd/spi-nor.h>

#include "core.h"

#ifdef CONFIG_ARCH_XMEDIA
static int
esmt_post_bfpt_fixups(struct spi_nor *nor,
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
	struct spi_nor_pp_command PP_1_1_4 = SNOR_OP_PROGRAMS(SPINOR_OP_PP_1_1_4,SNOR_PROTO_1_1_4);

	params->hwcaps.mask = SNOR_RD_MODES | SNOR_WR_MODES;
	memcpy(&params->reads[SNOR_CMD_READ], &read, sizeof(read));
	memcpy(&params->reads[SNOR_CMD_READ_FAST], &read_fast, sizeof(read_fast));
	memcpy(&params->reads[SNOR_CMD_READ_1_1_2], &read_1_1_2, sizeof(read_1_1_2));
	memcpy(&params->reads[SNOR_CMD_READ_1_2_2], &read_1_2_2, sizeof(read_1_2_2));
	memcpy(&params->reads[SNOR_CMD_READ_1_1_4], &read_1_1_4, sizeof(read_1_1_4));
	memcpy(&params->reads[SNOR_CMD_READ_1_4_4], &read_1_4_4, sizeof(read_1_4_4));

	memcpy(&params->page_programs[SNOR_CMD_PP], &PP, sizeof(PP));
	memcpy(&params->page_programs[SNOR_CMD_PP_1_1_4], &PP_1_1_4, sizeof(PP_1_1_4));

	params->quad_enable		 = macronix_quad_enable;

	return 0;
}

static struct spi_nor_fixups g_esmt_fixups = {
	.post_bfpt = esmt_post_bfpt_fixups,
};
#endif

static const struct flash_info esmt_parts[] = {
	/* ESMT */
	{ "f25l32pa", INFO(0x8c2016, 0, 64 * 1024, 64,
			   SECT_4K | SPI_NOR_HAS_LOCK) },
	{ "f25l32qa", INFO(0x8c4116, 0, 64 * 1024, 64,
			   SECT_4K | SPI_NOR_HAS_LOCK) },
#ifdef CONFIG_ARCH_XMEDIA
	{ "f25l64qa", INFO(0x8c4117, 0, 64 * 1024, 128, SPI_NOR_QUAD_READ)
			   .fixups = &g_esmt_fixups, CLK_MHZ_2X(84) },
#else
	{ "f25l64qa", INFO(0x8c4117, 0, 64 * 1024, 128,
			   SECT_4K | SPI_NOR_HAS_LOCK) },
#endif
};

const struct spi_nor_manufacturer spi_nor_esmt = {
	.name = "esmt",
	.parts = esmt_parts,
	.nparts = ARRAY_SIZE(esmt_parts),
};
