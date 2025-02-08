// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2005, Intec Automation Inc.
 * Copyright (C) 2014, Freescale Semiconductor, Inc.
 */

#include <linux/mtd/spi-nor.h>

#include "core.h"

#ifdef CONFIG_ARCH_XMEDIA
static int
xmc_post_bfpt_fixups(struct spi_nor *nor,
			    const struct sfdp_parameter_header *bfpt_header,
			    const struct sfdp_bfpt *bfpt,
			    struct spi_nor_flash_parameter *params)
{
	struct spi_nor_read_command read =       SNOR_OP_READ(0, 0, SPINOR_OP_READ, SNOR_PROTO_1_1_1);
	struct spi_nor_read_command read_fast =  SNOR_OP_READ(0, 8, SPINOR_OP_READ_FAST, SNOR_PROTO_1_1_1);
	struct spi_nor_read_command read_1_1_2 = SNOR_OP_READ(0, 8, SPINOR_OP_READ_1_1_2, SNOR_PROTO_1_1_2);
	struct spi_nor_read_command read_1_2_2 = SNOR_OP_READ(0, 8, SPINOR_OP_READ_1_2_2, SNOR_PROTO_1_2_2);
	struct spi_nor_read_command read_1_1_4 = SNOR_OP_READ(0, 8, SPINOR_OP_READ_1_1_4, SNOR_PROTO_1_1_4);
	struct spi_nor_read_command read_1_4_4 = SNOR_OP_READ(0, 24, SPINOR_OP_READ_1_4_4,SNOR_PROTO_1_4_4);
	struct spi_nor_pp_command PP =       SNOR_OP_PROGRAMS(SPINOR_OP_PP, SNOR_PROTO_1_1_1);
	struct spi_nor_pp_command PP_1_1_4 = SNOR_OP_PROGRAMS(SPINOR_OP_PP_1_1_4, SNOR_PROTO_1_1_4);

	params->hwcaps.mask = SNOR_RD_MODES | SNOR_WR_MODES;
	memcpy(&params->reads[SNOR_CMD_READ], &read, sizeof(read));
	memcpy(&params->reads[SNOR_CMD_READ_FAST], &read_fast, sizeof(read_fast));
	memcpy(&params->reads[SNOR_CMD_READ_1_1_2], &read_1_1_2, sizeof(read_1_1_2));
	memcpy(&params->reads[SNOR_CMD_READ_1_2_2], &read_1_2_2, sizeof(read_1_2_2));
	memcpy(&params->reads[SNOR_CMD_READ_1_1_4], &read_1_1_4, sizeof(read_1_1_4));
	memcpy(&params->reads[SNOR_CMD_READ_1_4_4], &read_1_4_4, sizeof(read_1_4_4));

	memcpy(&params->page_programs[SNOR_CMD_PP], &PP, sizeof(PP));
	memcpy(&params->page_programs[SNOR_CMD_PP_1_1_4], &PP_1_1_4, sizeof(PP_1_1_4));

	return 0;
}

static struct spi_nor_fixups g_xmc_fixups = {
	.post_bfpt = xmc_post_bfpt_fixups,
};
#endif

static const struct flash_info xmc_parts[] = {
#ifdef CONFIG_ARCH_XMEDIA
	/* XMC */
	{ "xm25qh64a",	  INFO(0x207017, 0, 64 * 1024,	 128, SPI_NOR_QUAD_READ)
			.fixups = &g_xmc_fixups, CLK_MHZ_2X(104) },
	{ "xm25qh64b",	  INFO(0x206017, 0, 64 * 1024,	 128, SPI_NOR_QUAD_READ)
			.fixups = &g_xmc_fixups, CLK_MHZ_2X(104) },
	{ "xm25qh128a",   INFO(0x207018, 0, 64 * 1024,	 256, SPI_NOR_QUAD_READ)
			.fixups = &g_xmc_fixups, CLK_MHZ_2X(104) },
	{ "xm25qh128b",   INFO(0x206018, 0, 64 * 1024,	 256, SPI_NOR_QUAD_READ)
			.fixups = &g_xmc_fixups, CLK_MHZ_2X(104) },
	{ "xm25qh64chiq", INFO(0x204017, 0, 64 * 1024,	 128, SPI_NOR_QUAD_READ)
			.fixups = &g_xmc_fixups, CLK_MHZ_2X(133) },
	{ "xm25qh128chiq",INFO(0x204018, 0, 64 * 1024,	 256, SPI_NOR_QUAD_READ)
			.fixups = &g_xmc_fixups, CLK_MHZ_2X(133) },
#else
/* XMC (Wuhan Xinxin Semiconductor Manufacturing Corp.) */
	{ "xm25qh64a", INFO(0x207017, 0, 64 * 1024, 128,
				SECT_4K | SPI_NOR_DUAL_READ | SPI_NOR_QUAD_READ) },
	{ "xm25qh128a", INFO(0x207018, 0, 64 * 1024, 256,
				SECT_4K | SPI_NOR_DUAL_READ | SPI_NOR_QUAD_READ) },
	{ "xm25qh64b", INFO(0x206017, 0, 64 * 1024, 128, SPI_NOR_QUAD_READ) },
	{ "xm25qh128b", INFO(0x206018, 0, 64 * 1024, 256, SPI_NOR_QUAD_READ) },
	{ "xm25qh64chiq", INFO(0x204017, 0, 64 * 1024, 128, SPI_NOR_QUAD_READ) },
	{ "xm25qh128chiq", INFO(0x204018, 0, 64 * 1024, 256, SPI_NOR_QUAD_READ) },
#endif /* CONFIG_ARCH_XMEDIA */
};

const struct spi_nor_manufacturer spi_nor_xmc = {
	.name = "xmc",
	.parts = xmc_parts,
	.nparts = ARRAY_SIZE(xmc_parts),
};
