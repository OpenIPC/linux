// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2005, Intec Automation Inc.
 * Copyright (C) 2014, Freescale Semiconductor, Inc.
 */

#include <linux/mtd/spi-nor.h>

#include "core.h"

#ifdef CONFIG_ARCH_XMEDIA
static int
mxic_post_bfpt_fixups(struct spi_nor *nor,
			    const struct sfdp_parameter_header *bfpt_header,
			    const struct sfdp_bfpt *bfpt,
			    struct spi_nor_flash_parameter *params)
{
	struct spi_nor_read_command read =       SNOR_OP_READ(0, 0, SPINOR_OP_READ, SNOR_PROTO_1_1_1);
	struct spi_nor_read_command read_fast =  SNOR_OP_READ(0, 8, SPINOR_OP_READ_FAST, SNOR_PROTO_1_1_1);
	struct spi_nor_read_command read_1_1_2 = SNOR_OP_READ(0, 8, SPINOR_OP_READ_1_1_2, SNOR_PROTO_1_1_2);
	struct spi_nor_read_command read_1_2_2 = SNOR_OP_READ(0, 8, SPINOR_OP_READ_1_2_2, SNOR_PROTO_1_2_2);
	struct spi_nor_read_command read_1_1_4 = SNOR_OP_READ(0, 8, SPINOR_OP_READ_1_1_4, SNOR_PROTO_1_1_4);
	struct spi_nor_read_command read_1_4_4 = SNOR_OP_READ(8, 16, SPINOR_OP_READ_1_4_4,SNOR_PROTO_1_4_4);

	struct spi_nor_pp_command PP =       SNOR_OP_PROGRAMS(SPINOR_OP_PP, SNOR_PROTO_1_1_1);
	struct spi_nor_pp_command PP_1_4_4 = SNOR_OP_PROGRAMS(SPINOR_OP_PP_1_4_4, SNOR_PROTO_1_4_4);

	params->hwcaps.mask = SNOR_RD_MODES | SNOR_MXIC_WR_MODES;
	memcpy(&params->reads[SNOR_CMD_READ], &read, sizeof(read));
	memcpy(&params->reads[SNOR_CMD_READ_FAST], &read_fast, sizeof(read_fast));
	memcpy(&params->reads[SNOR_CMD_READ_1_1_2], &read_1_1_2, sizeof(read_1_1_2));
	memcpy(&params->reads[SNOR_CMD_READ_1_2_2], &read_1_2_2, sizeof(read_1_2_2));
	memcpy(&params->reads[SNOR_CMD_READ_1_1_4], &read_1_1_4, sizeof(read_1_1_4));
	memcpy(&params->reads[SNOR_CMD_READ_1_4_4], &read_1_4_4, sizeof(read_1_4_4));

	memcpy(&params->page_programs[SNOR_CMD_PP], &PP, sizeof(PP));
	memcpy(&params->page_programs[SNOR_CMD_PP_1_4_4], &PP_1_4_4, sizeof(PP_1_4_4));

	params->quad_enable		 = macronix_quad_enable;

	return 0;
}

static struct spi_nor_fixups g_mxic_fixups = {
	.post_bfpt = mxic_post_bfpt_fixups,
};
#else
static int
mx25l25635_post_bfpt_fixups(struct spi_nor *nor,
			    const struct sfdp_parameter_header *bfpt_header,
			    const struct sfdp_bfpt *bfpt,
			    struct spi_nor_flash_parameter *params)
{
	/*
	 * MX25L25635F supports 4B opcodes but MX25L25635E does not.
	 * Unfortunately, Macronix has re-used the same JEDEC ID for both
	 * variants which prevents us from defining a new entry in the parts
	 * table.
	 * We need a way to differentiate MX25L25635E and MX25L25635F, and it
	 * seems that the F version advertises support for Fast Read 4-4-4 in
	 * its BFPT table.
	 */
	if (bfpt->dwords[BFPT_DWORD(5)] & BFPT_DWORD5_FAST_READ_4_4_4)
		nor->flags |= SNOR_F_4B_OPCODES;

	return 0;
}

static struct spi_nor_fixups mx25l25635_fixups = {
	.post_bfpt = mx25l25635_post_bfpt_fixups,
};
#endif

static const struct flash_info macronix_parts[] = {
	/* Macronix */
	{ "mx25l512e",   INFO(0xc22010, 0, 64 * 1024,   1, SECT_4K) },
	{ "mx25l2005a",  INFO(0xc22012, 0, 64 * 1024,   4, SECT_4K) },
	{ "mx25l4005a",  INFO(0xc22013, 0, 64 * 1024,   8, SECT_4K) },
	{ "mx25l8005",   INFO(0xc22014, 0, 64 * 1024,  16, 0) },
	{ "mx25l3205d",  INFO(0xc22016, 0, 64 * 1024,  64, SECT_4K) },
	{ "mx25l3255e",  INFO(0xc29e16, 0, 64 * 1024,  64, SECT_4K) },
	{ "mx25u2033e",  INFO(0xc22532, 0, 64 * 1024,   4, SECT_4K) },
	{ "mx25u3235f",  INFO(0xc22536, 0, 64 * 1024,  64,
			      SECT_4K | SPI_NOR_DUAL_READ |
			      SPI_NOR_QUAD_READ) },
	{ "mx25u4035",   INFO(0xc22533, 0, 64 * 1024,   8, SECT_4K) },
	{ "mx25u8035",   INFO(0xc22534, 0, 64 * 1024,  16, SECT_4K) },
	{ "mx25l12855e", INFO(0xc22618, 0, 64 * 1024, 256, 0) },
	{ "mx25r1635f",  INFO(0xc22815, 0, 64 * 1024,  32,
			      SECT_4K | SPI_NOR_DUAL_READ |
			      SPI_NOR_QUAD_READ) },
	{ "mx25r3235f",  INFO(0xc22816, 0, 64 * 1024,  64,
			      SECT_4K | SPI_NOR_DUAL_READ |
			      SPI_NOR_QUAD_READ) },
	/* Macronix/MXIC Wide Voltage Range 1.65~3.6V */
	{ "mx25u25635f", INFO(0xc22539, 0, 64 * 1024, 512,
			      SECT_4K | SPI_NOR_4B_OPCODES) },
	{ "mx25v8035f",  INFO(0xc22314, 0, 64 * 1024,  16,
			      SECT_4K | SPI_NOR_DUAL_READ |
			      SPI_NOR_QUAD_READ) },
	{ "mx25l25655e", INFO(0xc22619, 0, 64 * 1024, 512, 0) },
	{ "mx66l1g45g",  INFO(0xc2201b, 0, 64 * 1024, 2048,
			      SECT_4K | SPI_NOR_DUAL_READ |
			      SPI_NOR_QUAD_READ) },
	{ "mx66l1g55g",  INFO(0xc2261b, 0, 64 * 1024, 2048,
			      SPI_NOR_QUAD_READ) },
#ifdef CONFIG_ARCH_XMEDIA
	{ "mx25l6436f",  INFO(0xc22017, 0, 64 * 1024, 128,
			SPI_NOR_QUAD_READ) .fixups = &g_mxic_fixups, CLK_MHZ_2X(133) },
	{ "mx25l12835f", INFO(0xc22018, 0, 64 * 1024, 256,
			SPI_NOR_QUAD_READ) .fixups = &g_mxic_fixups, CLK_MHZ_2X(84) },
	{ "mx25l25635f", INFO(0xc22019, 0, 64 * 1024, 512,
			SPI_NOR_QUAD_READ | SPI_NOR_4B_OPCODES) .fixups = &g_mxic_fixups, CLK_MHZ_2X(84) },
	{ "mx25l1606e",  INFO(0xc22015, 0, 64 * 1024,  32, SECT_4K
			| SPI_NOR_DUAL_READ) CLK_MHZ_2X(80) },
	{ "mx25v1635f",  INFO(0xc22315, 0, 64 * 1024, 32 ,
			SPI_NOR_QUAD_READ) .fixups = &g_mxic_fixups, CLK_MHZ_2X(80) },
	/* Macronix/MXIC Wide Voltage Range 1.65~3.6V */
	{ "mx25r6435f",  INFO(0xc22817, 0, 64 * 1024, 128,
			SPI_NOR_DUAL_READ | SPI_NOR_QUAD_READ) CLK_MHZ_2X(80) },
	/* Macronix/MXIC 1.8V */
	{ "mx25u1633f",  INFO(0xc22535, 0, 64 * 1024, 32,
			SPI_NOR_QUAD_READ) .fixups = &g_mxic_fixups, CLK_MHZ_2X(80) },
	{ "mx25u6435f",  INFO(0xc22537, 0, 64 * 1024, 128,
			SECT_4K | SPI_NOR_QUAD_READ) .fixups = &g_mxic_fixups, CLK_MHZ_2X(84) },
	{ "mx25u12835f/mx25u12832f", INFO(0xc22538, 0, 64 * 1024, 256,
			SPI_NOR_QUAD_READ) .fixups = &g_mxic_fixups, CLK_MHZ_2X(84) },
	{ "mx25u51245g", INFO(0xc2253a, 0, 64 * 1024, 1024,
			SPI_NOR_QUAD_READ | SPI_NOR_4B_OPCODES) .fixups = &g_mxic_fixups, CLK_MHZ_2X(166) },
	{ "mx66l51235l/mx25l51245g", INFO(0xc2201a, 0, 64 * 1024, 1024,
			SPI_NOR_QUAD_READ) .fixups = &g_mxic_fixups, CLK_MHZ_2X(133)},
	{ "mx66u1g45gm", INFO(0xc2253b, 0, 64 * 1024, 2048,
			SPI_NOR_QUAD_READ | SPI_NOR_4B_OPCODES) .fixups = &g_mxic_fixups, CLK_MHZ_2X(133) },
#else
	{ "mx25l6405d",  INFO(0xc22017, 0, 64 * 1024, 128, SECT_4K) },
	{ "mx25l1606e",  INFO(0xc22015, 0, 64 * 1024,  32, SECT_4K) },
	{ "mx25u6435f",  INFO(0xc22537, 0, 64 * 1024, 128, SECT_4K) },
	{ "mx25l12805d", INFO(0xc22018, 0, 64 * 1024, 256, SECT_4K) },
	{ "mx25v1635f",  INFO(0xc22315, 0, 64 * 1024, 32 ,
				  SPI_NOR_QUAD_READ) },
	{ "mx25r6435f",  INFO(0xc22817, 0, 64 * 1024, 128,
			      SPI_NOR_DUAL_READ | SPI_NOR_QUAD_READ) },
	{ "mx25u12835f", INFO(0xc22538, 0, 64 * 1024, 256,
			      SECT_4K | SPI_NOR_DUAL_READ |
			      SPI_NOR_QUAD_READ) },
	{ "mx25l25635e", INFO(0xc22019, 0, 64 * 1024, 512,
			      SPI_NOR_DUAL_READ | SPI_NOR_QUAD_READ)
		.fixups = &mx25l25635_fixups },
	{ "mx25u51245g", INFO(0xc2253a, 0, 64 * 1024, 1024,
			      SECT_4K | SPI_NOR_DUAL_READ |
			      SPI_NOR_QUAD_READ | SPI_NOR_4B_OPCODES) },
	{ "mx66l51235l", INFO(0xc2201a, 0, 64 * 1024, 1024,
			      SPI_NOR_DUAL_READ | SPI_NOR_QUAD_READ |
			      SPI_NOR_4B_OPCODES) },
	/* Macronix/MXIC 1.8V */
	{ "mx25u1633f",  INFO(0xc22535, 0, 64 * 1024, 32,
				  SPI_NOR_QUAD_READ) },
	{ "mx66u1g45gm", INFO(0xc2253b, 0, 64 * 1024, 2048,
				  SPI_NOR_QUAD_READ | SPI_NOR_4B_OPCODES) },
#endif /* CONFIG_ARCH_XMEDIA */
	{ "mx66u2g45g",	 INFO(0xc2253c, 0, 64 * 1024, 4096,
			      SECT_4K | SPI_NOR_DUAL_READ |
			      SPI_NOR_QUAD_READ | SPI_NOR_4B_OPCODES) },
};

static void macronix_default_init(struct spi_nor *nor)
{
	nor->params->quad_enable = spi_nor_sr1_bit6_quad_enable;
	nor->params->set_4byte_addr_mode = spi_nor_set_4byte_addr_mode;
}

static const struct spi_nor_fixups macronix_fixups = {
	.default_init = macronix_default_init,
};

const struct spi_nor_manufacturer spi_nor_macronix = {
	.name = "macronix",
	.parts = macronix_parts,
	.nparts = ARRAY_SIZE(macronix_parts),
	.fixups = &macronix_fixups,
};
