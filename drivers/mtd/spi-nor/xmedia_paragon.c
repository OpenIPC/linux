#ifdef CONFIG_ARCH_XMEDIA
#include <linux/mtd/spi-nor.h>

#include "core.h"

static int
puya_post_bfpt_fixups(struct spi_nor *nor,
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

	params->quad_enable		 = puya_quad_enable;

	return 0;
}

static struct spi_nor_fixups g_puya_fixups = {
	.post_bfpt = puya_post_bfpt_fixups,
};

static int
xtx_post_bfpt_fixups(struct spi_nor *nor,
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

	params->quad_enable		 = xtx_quad_enable;

	return 0;
}

static struct spi_nor_fixups g_xtx_fixups = {
	.post_bfpt = xtx_post_bfpt_fixups,
};

static int
paragon_post_bfpt_fixups(struct spi_nor *nor,
			    const struct sfdp_parameter_header *bfpt_header,
			    const struct sfdp_bfpt *bfpt,
			    struct spi_nor_flash_parameter *params)
{
	struct spi_nor_read_command read =       SNOR_OP_READ(0, 0, SPINOR_OP_READ, SNOR_PROTO_1_1_1);
	struct spi_nor_read_command read_fast =  SNOR_OP_READ(0, 8, SPINOR_OP_READ_FAST,	SNOR_PROTO_1_1_1);
	struct spi_nor_read_command read_1_1_2 = SNOR_OP_READ(0, 8, SPINOR_OP_READ_1_1_2,	SNOR_PROTO_1_1_2);
	struct spi_nor_read_command read_1_2_2 = SNOR_OP_READ(8, 0, SPINOR_OP_READ_1_2_2,	SNOR_PROTO_1_2_2);
	struct spi_nor_read_command read_1_1_4 = SNOR_OP_READ(0, 8, SPINOR_OP_READ_1_1_4,	SNOR_PROTO_1_1_4);
	struct spi_nor_read_command read_1_4_4 = SNOR_OP_READ(8, 16, SPINOR_OP_READ_1_4_4, SNOR_PROTO_1_4_4);
	struct spi_nor_pp_command PP =       SNOR_OP_PROGRAMS(SPINOR_OP_PP,SNOR_PROTO_1_1_1);

	params->hwcaps.mask = SNOR_RD_MODES | SNOR_PARAGON_WR_MODES;
	memcpy(&params->reads[SNOR_CMD_READ], &read, sizeof(read));
	memcpy(&params->reads[SNOR_CMD_READ_FAST], &read_fast, sizeof(read_fast));
	memcpy(&params->reads[SNOR_CMD_READ_1_1_2], &read_1_1_2, sizeof(read_1_1_2));
	memcpy(&params->reads[SNOR_CMD_READ_1_2_2], &read_1_2_2, sizeof(read_1_2_2));
	memcpy(&params->reads[SNOR_CMD_READ_1_1_4], &read_1_1_4, sizeof(read_1_1_4));
	memcpy(&params->reads[SNOR_CMD_READ_1_4_4], &read_1_4_4, sizeof(read_1_4_4));

	memcpy(&params->page_programs[SNOR_CMD_PP], &PP, sizeof(PP));

	params->quad_enable		 = spansion_quad_enable;

	return 0;
}

static struct spi_nor_fixups g_paragon_fixups = {
	.post_bfpt = paragon_post_bfpt_fixups,
};

static int
nm_post_bfpt_fixups(struct spi_nor *nor,
			    const struct sfdp_parameter_header *bfpt_header,
			    const struct sfdp_bfpt *bfpt,
			    struct spi_nor_flash_parameter *params)
{
	struct spi_nor_read_command read =       SNOR_OP_READ(0, 0, SPINOR_OP_READ, SNOR_PROTO_1_1_1);
	struct spi_nor_read_command read_fast =  SNOR_OP_READ(0, 8, SPINOR_OP_READ_FAST,	SNOR_PROTO_1_1_1);
	struct spi_nor_read_command read_1_1_2 = SNOR_OP_READ(0, 8, SPINOR_OP_READ_1_1_2,	SNOR_PROTO_1_1_2);
	struct spi_nor_read_command read_1_2_2 = SNOR_OP_READ(8, 0, SPINOR_OP_READ_1_2_2,	SNOR_PROTO_1_2_2);
	struct spi_nor_read_command read_1_1_4 = SNOR_OP_READ(0, 8, SPINOR_OP_READ_1_1_4,	SNOR_PROTO_1_1_4);
	struct spi_nor_read_command read_1_4_4 = SNOR_OP_READ(8, 16, SPINOR_OP_READ_1_4_4, SNOR_PROTO_1_4_4);
	struct spi_nor_pp_command PP =       SNOR_OP_PROGRAMS(SPINOR_OP_PP,SNOR_PROTO_1_1_1);

	params->hwcaps.mask = SNOR_RD_MODES | SNOR_WR_MODES;
	memcpy(&params->reads[SNOR_CMD_READ], &read, sizeof(read));
	memcpy(&params->reads[SNOR_CMD_READ_FAST], &read_fast, sizeof(read_fast));
	memcpy(&params->reads[SNOR_CMD_READ_1_1_2], &read_1_1_2, sizeof(read_1_1_2));
	memcpy(&params->reads[SNOR_CMD_READ_1_2_2], &read_1_2_2, sizeof(read_1_2_2));
	memcpy(&params->reads[SNOR_CMD_READ_1_1_4], &read_1_1_4, sizeof(read_1_1_4));
	memcpy(&params->reads[SNOR_CMD_READ_1_4_4], &read_1_4_4, sizeof(read_1_4_4));

	memcpy(&params->page_programs[SNOR_CMD_PP], &PP, sizeof(PP));

	params->quad_enable		 = nm_quad_enable;

	return 0;
}

static struct spi_nor_fixups g_nm_fixups = {
	.post_bfpt = nm_post_bfpt_fixups,
};

static const struct flash_info paragon_parts[] = {
	/* Paragon 3.3V */
	{ "pn25f16s", INFO(0xe04015, 0, 64 * 1024,	32, SPI_NOR_QUAD_READ)
		.fixups = &g_paragon_fixups, CLK_MHZ_2X(80) },
	{ "pn25f32s", INFO(0xe04016, 0, 64 * 1024,	64, SPI_NOR_QUAD_READ)
		.fixups = &g_paragon_fixups, CLK_MHZ_2X(80) },

	/* XTX */
	{ "xt25f128b", INFO(0x0b4018, 0, 64 * 1024,  256,
		SPI_NOR_QUAD_READ) .fixups = &g_xtx_fixups, CLK_MHZ_2X(70) },
	{ "xt25f64b", INFO(0x0b4017, 0, 64 * 1024,	128,
		SPI_NOR_QUAD_READ) .fixups = &g_xtx_fixups, CLK_MHZ_2X(70) },
	{ "xt25f32bssigu-s", INFO(0x0b4016, 0, 64 * 1024,  64,
		SPI_NOR_QUAD_READ) .fixups = &g_xtx_fixups, CLK_MHZ_2X(108) },
	{ "xt25f16bssigu", INFO(0x0b4015, 0, 64 * 1024,  32,
		SPI_NOR_QUAD_READ) .fixups = &g_xtx_fixups, CLK_MHZ_2X(120) },

	/* puya 3.3V */
	{ "p25q64sh", INFO(0x856017, 0, 64 * 1024, 128,
		SPI_NOR_QUAD_READ) .fixups = &g_puya_fixups, CLK_MHZ_2X(80) },
	{ "p25q128h", INFO(0x856018, 0, 64 * 1024, 256,
		SPI_NOR_QUAD_READ) .fixups = &g_puya_fixups, CLK_MHZ_2X(104) },
	{ "py25q128ha",INFO(0x852018, 0, 64 * 1024, 256,
		SPI_NOR_QUAD_READ) .fixups = &g_puya_fixups, CLK_MHZ_2X(104) },

	/* nm 3.3V */
	{ "nm25q64evb",  INFO(0x522217, 0, 64 * 1024, 128,
		SPI_NOR_QUAD_READ) .fixups = &g_nm_fixups, CLK_MHZ_2X(50) },
	{ "nm25q128evb", INFO(0x522218, 0, 64 * 1024, 256,
		SPI_NOR_QUAD_READ) .fixups = &g_nm_fixups, CLK_MHZ_2X(50) },
};

const struct spi_nor_manufacturer xmedia_spi_nor_paragon = {
	.name = "xmedia_paragon",
	.parts = paragon_parts,
	.nparts = ARRAY_SIZE(paragon_parts),
};
#endif
