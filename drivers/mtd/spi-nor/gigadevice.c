// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2005, Intec Automation Inc.
 * Copyright (C) 2014, Freescale Semiconductor, Inc.
 */

#include <linux/mtd/spi-nor.h>

#include "core.h"

#ifdef CONFIG_ARCH_XMEDIA
static int
gd_post_bfpt_fixups(struct spi_nor *nor,
			    const struct sfdp_parameter_header *bfpt_header,
			    const struct sfdp_bfpt *bfpt,
			    struct spi_nor_flash_parameter *params)
{
	struct spi_nor_read_command read =       SNOR_OP_READ(0, 0, SPINOR_OP_READ, SNOR_PROTO_1_1_1);
	struct spi_nor_read_command read_fast =  SNOR_OP_READ(0, 8, SPINOR_OP_READ_FAST, SNOR_PROTO_1_1_1);
	struct spi_nor_read_command read_1_1_2 = SNOR_OP_READ(0, 8, SPINOR_OP_READ_1_1_2, SNOR_PROTO_1_1_2);
	struct spi_nor_read_command read_1_2_2 = SNOR_OP_READ(8, 0, SPINOR_OP_READ_1_2_2, SNOR_PROTO_1_2_2);
	struct spi_nor_read_command read_1_1_4 = SNOR_OP_READ(0, 8, SPINOR_OP_READ_1_1_4, SNOR_PROTO_1_1_4);
	struct spi_nor_read_command read_1_4_4 = SNOR_OP_READ(8, 16, SPINOR_OP_READ_1_4_4,SNOR_PROTO_1_4_4);
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

	params->quad_enable		 = gd_quad_enable;

	return 0;
}

static struct spi_nor_fixups g_gd_fixups = {
	.post_bfpt = gd_post_bfpt_fixups,
};
#else
static void gd25q256_default_init(struct spi_nor *nor)
{
	/*
	 * Some manufacturer like GigaDevice may use different
	 * bit to set QE on different memories, so the MFR can't
	 * indicate the quad_enable method for this case, we need
	 * to set it in the default_init fixup hook.
	 */
	nor->params->quad_enable = spi_nor_sr1_bit6_quad_enable;
}

static struct spi_nor_fixups gd25q256_fixups = {
	.default_init = gd25q256_default_init,
};
#endif

static const struct flash_info gigadevice_parts[] = {
#ifdef CONFIG_ARCH_XMEDIA
	{ "gd25q16c", INFO(0xc84015, 0, 64 * 1024, 32,
			SECT_4K | SPI_NOR_QUAD_READ) .fixups = &g_gd_fixups, CLK_MHZ_2X(120) },
	{ "gd25q32", INFO(0xc84016, 0, 64 * 1024,  64,
			SECT_4K | SPI_NOR_QUAD_READ) .fixups = &g_gd_fixups, CLK_MHZ_2X(120) },
	{ "gd25q64", INFO(0xc84017, 0, 64 * 1024, 128,
			SECT_4K | SPI_NOR_QUAD_READ) .fixups = &g_gd_fixups, CLK_MHZ_2X(120) },
	{ "gd25q128/gd25q127", INFO(0xc84018, 0, 64 * 1024, 256,
			SECT_4K | SPI_NOR_QUAD_READ) .fixups = &g_gd_fixups, CLK_MHZ_2X(80) },
	/* GigaDevice 1.8V */
	{ "gd25lq16c", INFO(0xc86015, 0, 64 * 1024, 32,
			SECT_4K | SPI_NOR_QUAD_READ) .fixups = &g_gd_fixups, CLK_MHZ_2X(104) },
	{ "gd25lq64", INFO(0xc86017, 0, 64 * 1024, 128,
			SECT_4K | SPI_NOR_QUAD_READ) .fixups = &g_gd_fixups, CLK_MHZ_2X(133) },
	{ "gd25lq128", INFO(0xc86018, 0, 64 * 1024, 256,
			SECT_4K | SPI_NOR_QUAD_READ) .fixups = &g_gd_fixups, CLK_MHZ_2X(133) },
	{ "gd25q256", INFO(0xc84019, 0, 64 * 1024, 512,
			SECT_4K | SPI_NOR_DUAL_READ | SPI_NOR_QUAD_READ |
			SPI_NOR_4B_OPCODES | SPI_NOR_HAS_LOCK |
			SPI_NOR_HAS_TB | SPI_NOR_TB_SR_BIT6)
			.fixups = &g_gd_fixups, CLK_MHZ_2X(133) },
#else
	{ "gd25q16", INFO(0xc84015, 0, 64 * 1024,  32,
			SECT_4K | SPI_NOR_DUAL_READ | SPI_NOR_QUAD_READ |
			SPI_NOR_HAS_LOCK | SPI_NOR_HAS_TB) },
	{ "gd25q32", INFO(0xc84016, 0, 64 * 1024,  64,
			SECT_4K | SPI_NOR_DUAL_READ | SPI_NOR_QUAD_READ |
			SPI_NOR_HAS_LOCK | SPI_NOR_HAS_TB) },
	{ "gd25q64", INFO(0xc84017, 0, 64 * 1024, 128,
			SECT_4K | SPI_NOR_DUAL_READ | SPI_NOR_QUAD_READ |
			SPI_NOR_HAS_LOCK | SPI_NOR_HAS_TB) },
	{ "gd25lq16c", INFO(0xc86015, 0, 64 * 1024, 32,
			SECT_4K | SPI_NOR_DUAL_READ | SPI_NOR_QUAD_READ |
			SPI_NOR_HAS_LOCK | SPI_NOR_HAS_TB) },
	{ "gd25lq64c", INFO(0xc86017, 0, 64 * 1024, 128,
			SECT_4K | SPI_NOR_DUAL_READ | SPI_NOR_QUAD_READ |
			SPI_NOR_HAS_LOCK | SPI_NOR_HAS_TB) },
	{ "gd25lq128d", INFO(0xc86018, 0, 64 * 1024, 256,
			SECT_4K | SPI_NOR_DUAL_READ | SPI_NOR_QUAD_READ |
			SPI_NOR_HAS_LOCK | SPI_NOR_HAS_TB) },
	{ "gd25q128", INFO(0xc84018, 0, 64 * 1024, 256,
			SECT_4K | SPI_NOR_DUAL_READ | SPI_NOR_QUAD_READ |
			SPI_NOR_HAS_LOCK | SPI_NOR_HAS_TB) },
	{ "gd25q256", INFO(0xc84019, 0, 64 * 1024, 512,
		    SECT_4K | SPI_NOR_DUAL_READ | SPI_NOR_QUAD_READ |
		    SPI_NOR_4B_OPCODES | SPI_NOR_HAS_LOCK |
		    SPI_NOR_HAS_TB | SPI_NOR_TB_SR_BIT6)
		    .fixups = &gd25q256_fixups },
#endif
	{ "gd25lq32", INFO(0xc86016, 0, 64 * 1024, 64,
		    SECT_4K | SPI_NOR_DUAL_READ | SPI_NOR_QUAD_READ |
		    SPI_NOR_HAS_LOCK | SPI_NOR_HAS_TB) },
};

const struct spi_nor_manufacturer spi_nor_gigadevice = {
	.name = "gigadevice",
	.parts = gigadevice_parts,
	.nparts = ARRAY_SIZE(gigadevice_parts),
};
