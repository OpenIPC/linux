/*
 * Copyright (c) HiSilicon (Shanghai) Technologies Co., Ltd. 2020-2023. All rights reserved.
 */

#include <linux/mtd/spi-nor.h>

#include "core.h"

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

static void macronix_default_init(struct spi_nor *nor)
{
	nor->params->quad_enable = spi_nor_sr1_bit6_quad_enable;
	nor->params->set_4byte_addr_mode = spi_nor_set_4byte_addr_mode;
}

static const struct spi_nor_fixups macronix_fixups = {
	.default_init = macronix_default_init,
};

static void xm25q128a_default_init(struct spi_nor *nor)
{
	nor->params->quad_enable = NULL;
}

static struct spi_nor_fixups xm25q128a_fixups = {
	.default_init = xm25q128a_default_init,
};

static const struct flash_info general_parts[] = {
	/* XTX */
	{ "xt25f128b", 	INFO(0x0b4018, 0, 64 * 1024,  256,
					SPI_NOR_QUAD_READ) PARAMS(xtx), CLK_MHZ_2X(70) },
	{ "xt25f64b", 	INFO(0x0b4017, 0, 64 * 1024,  128,
					SPI_NOR_QUAD_READ) PARAMS(xtx), CLK_MHZ_2X(70) },
	{ "xt25f32b", 	INFO(0x0b4016, 0, 64 * 1024,  256,
					SPI_NOR_QUAD_READ) PARAMS(xtx), CLK_MHZ_2X(100) },
	/* FM */
	{ "fm25q64", 	INFO(0xa14017, 0, 64 * 1024,  128,
					SPI_NOR_QUAD_READ) PARAMS(spansion), CLK_MHZ_2X(80) },
	{ "fm25q64a", 	INFO(0xa14018, 0, 64 * 1024,  256,
					SPI_NOR_QUAD_READ) PARAMS(spansion), CLK_MHZ_2X(80) },
	/* EON */
	{ "en25qh64a",  INFO(0x1c7017, 0, 64 * 1024,  128, SPI_NOR_QUAD_READ)
					PARAMS(eon), CLK_MHZ_2X(104) .fixups = &xm25q128a_fixups},
	/* GD */
	{ "gd25lq16c", INFO(0xc86015, 0, 64 * 1024, 32,
					SECT_4K | SPI_NOR_QUAD_READ) PARAMS(gd), CLK_MHZ_2X(104) },
	{ "gd25lq32", 	INFO(0xc86016, 0, 64 * 1024, 64,
					SECT_4K | SPI_NOR_DUAL_READ | SPI_NOR_QUAD_READ |
					SPI_NOR_HAS_LOCK | SPI_NOR_HAS_TB) },
	{ "gd25lq64",   INFO(0xc86017, 0, 64 * 1024, 128,
					SECT_4K | SPI_NOR_QUAD_READ) PARAMS(gd), CLK_MHZ_2X(133) },
	{ "gd25lq128",  INFO(0xc86018, 0, 64 * 1024, 256,
					SECT_4K | SPI_NOR_QUAD_READ) PARAMS(gd), CLK_MHZ_2X(133) },
	{ "gd25q256", 	INFO(0xc84019, 0, 64 * 1024, 512,
					SECT_4K | SPI_NOR_DUAL_READ | SPI_NOR_QUAD_READ |
					SPI_NOR_4B_OPCODES | SPI_NOR_HAS_LOCK |
					SPI_NOR_HAS_TB | SPI_NOR_TB_SR_BIT6)
					.fixups = &gd25q256_fixups },
	{ "gd25q16c", 	INFO(0xc84015, 0, 64 * 1024, 32,
					SECT_4K | SPI_NOR_QUAD_READ) PARAMS(gd), CLK_MHZ_2X(120) },
	{ "gd25q32", 	INFO(0xc84016, 0, 64 * 1024,  64,
					SECT_4K | SPI_NOR_QUAD_READ) PARAMS(gd), CLK_MHZ_2X(120) },
	{ "gd25q64", 	INFO(0xc84017, 0, 64 * 1024, 128,
					SECT_4K | SPI_NOR_QUAD_READ) PARAMS(gd), CLK_MHZ_2X(120) },
	{ "gd25q128/gd25q127", 	INFO(0xc84018, 0, 64 * 1024, 256,
					SECT_4K | SPI_NOR_QUAD_READ) PARAMS(gd), CLK_MHZ_2X(80) },
	/* GigaDevice 1.8V */
	{ "gd25lq64", 	INFO(0xc86017, 0, 64 * 1024, 128,
					SECT_4K | SPI_NOR_QUAD_READ) PARAMS(gd), CLK_MHZ_2X(133) },
	{ "gd25lq128", 	INFO(0xc86018, 0, 64 * 1024, 256,
					SECT_4K | SPI_NOR_QUAD_READ) PARAMS(gd), CLK_MHZ_2X(133) },
	/* macronix */
	{ "mx25l6436f", INFO(0xc22017, 0, 64 * 1024, 128,
					SPI_NOR_QUAD_READ) PARAMS(mxic), CLK_MHZ_2X(133)
					.fixups = &macronix_fixups },
	{ "mx25l12835f", INFO(0xc22018, 0, 64 * 1024, 256,
					SPI_NOR_QUAD_READ) PARAMS(mxic), CLK_MHZ_2X(84)
					.fixups = &macronix_fixups },
	{ "mx25l25635f", INFO(0xc22019, 0, 64 * 1024, 512,
					SPI_NOR_QUAD_READ | SPI_NOR_4B_OPCODES) PARAMS(mxic), CLK_MHZ_2X(84)
					.fixups = &macronix_fixups},
	{ "mx25l1606e", INFO(0xc22015, 0, 64 * 1024,  32,
					SECT_4K | SPI_NOR_DUAL_READ) CLK_MHZ_2X(80)
					.fixups = &macronix_fixups },
	{ "mx25v1635f", INFO(0xc22315, 0, 64 * 1024, 32,
					SPI_NOR_QUAD_READ) PARAMS(mxic), CLK_MHZ_2X(80)
					.fixups = &macronix_fixups},
	/* Macronix/MXIC 1.8V */
	{ "mx25u6435f", INFO(0xc22537, 0, 64 * 1024, 128,
					SECT_4K | SPI_NOR_QUAD_READ) PARAMS(mxic), CLK_MHZ_2X(84)
					.fixups = &macronix_fixups},
	{ "mx25u12835f/mx25u12832f", INFO(0xc22538, 0, 64 * 1024, 256,
					SPI_NOR_QUAD_READ) PARAMS(mxic), CLK_MHZ_2X(84)
					.fixups = &macronix_fixups},
	{ "mx66l51235l/mx25l51245g", INFO(0xc2201a, 0, 64 * 1024, 1024,
					SPI_NOR_QUAD_READ) PARAMS(mxic), CLK_MHZ_2X(133)
					.fixups = &macronix_fixups},
	{ "mx25u51245g", INFO(0xc2253a, 0, 64 * 1024, 1024, SPI_NOR_QUAD_READ | SPI_NOR_4B_OPCODES)
					PARAMS(mxic), CLK_MHZ_2X(166) .fixups = &xm25q128a_fixups},
	/* Winbond */
	{ "w25q32", 	INFO(0xef4016, 0, 64 * 1024,  64,
					SECT_4K | SPI_NOR_QUAD_READ) PARAMS(winbond), CLK_MHZ_2X(80) },
	{ "w25q64fv(spi)/w25q64jv_iq", INFO(0xef4017, 0, 64 * 1024, 128,
					SECT_4K | SPI_NOR_QUAD_READ) PARAMS(winbond), CLK_MHZ_2X(80) },
	{ "w25q128(b/f)v", INFO(0xef4018, 0, 64 * 1024, 256,
					SECT_4K | SPI_NOR_QUAD_READ) PARAMS(winbond), CLK_MHZ_2X(104) },
	{ "w25q128jv_im", INFO(0xef7018, 0, 64 * 1024, 256,
					SECT_4K | SPI_NOR_QUAD_READ) PARAMS(winbond), CLK_MHZ_2X(80) },
	{ "w25q256", 	INFO(0xef4019, 0, 64 * 1024, 512,
					SECT_4K | SPI_NOR_QUAD_READ | SPI_NOR_4B_OPCODES)
					PARAMS(winbond), CLK_MHZ_2X(80) },
	{ "w25q01jvzeiq", INFO(0xef4021, 0, 64 * 1024, 2048,
					SECT_4K | SPI_NOR_QUAD_READ | SPI_NOR_4B_OPCODES)
					PARAMS(winbond), CLK_MHZ_2X(90) },
	/* Winbond 1.8V */
	{ "w25q32fw", 	INFO(0xef6016, 0, 64 * 1024,  64,
					SECT_4K | SPI_NOR_DUAL_READ | SPI_NOR_QUAD_READ |
					SPI_NOR_HAS_LOCK | SPI_NOR_HAS_TB) PARAMS(winbond), CLK_MHZ_2X(80) },
	{ "w25q64dw", 	INFO(0xef6017, 0, 64 * 1024, 128,
					SECT_4K | SPI_NOR_DUAL_READ | SPI_NOR_QUAD_READ |
					SPI_NOR_HAS_LOCK | SPI_NOR_HAS_TB) PARAMS(winbond), CLK_MHZ_2X(80) },
	{ "w25q128fw", 	INFO(0xef6018, 0, 64 * 1024, 256,
					SECT_4K | SPI_NOR_DUAL_READ | SPI_NOR_QUAD_READ |
					SPI_NOR_HAS_LOCK | SPI_NOR_HAS_TB) PARAMS(winbond), CLK_MHZ_2X(80) },
	/* xmc */
	{ "xm25qh64a",  INFO(0x207017, 0, 64 * 1024,   128, SPI_NOR_QUAD_READ)
					PARAMS(xmc), CLK_MHZ_2X(104) .fixups = &xm25q128a_fixups},
	{ "xm25qh64b",  INFO(0x206017, 0, 64 * 1024,   128, SPI_NOR_QUAD_READ)
					PARAMS(xmc), CLK_MHZ_2X(104) .fixups = &xm25q128a_fixups},
	{ "xm25qh128a", INFO(0x207018, 0, 64 * 1024,   256, SPI_NOR_QUAD_READ)
					PARAMS(xmc), CLK_MHZ_2X(104) .fixups = &xm25q128a_fixups},
	{ "xm25qh128b", INFO(0x206018, 0, 64 * 1024,   256, SPI_NOR_QUAD_READ)
					PARAMS(xmc), CLK_MHZ_2X(104) .fixups = &xm25q128a_fixups},
	{ "xm25qh64c",  INFO(0x204017, 0, 64 * 1024,   128, SPI_NOR_QUAD_READ)
					PARAMS(xmc), CLK_MHZ_2X(104) },
	{ "xm25qh128c", INFO(0x204018, 0, 64 * 1024,   256, SPI_NOR_QUAD_READ)
					PARAMS(xmc), CLK_MHZ_2X(104) },
	{ "by25q64", INFO(0x684017, 0, 64 * 1024,  128, 0) PARAMS(spansion), CLK_MHZ_2X(50) },
	/* Zbit */
	{ "zb25vq64d",  INFO(0x5e4017, 0, 64 * 1024, 128,
					SECT_4K | SPI_NOR_QUAD_READ) PARAMS(gd), CLK_MHZ_2X(104) },
	{ "zb25vq128d", INFO(0x5e4018, 0, 64 * 1024, 256,
					SECT_4K | SPI_NOR_QUAD_READ) PARAMS(gd), CLK_MHZ_2X(104) },
};

static void spinor_default_init(struct spi_nor *nor)
{
}

static const struct spi_nor_fixups general_fixups = {
	.default_init = spinor_default_init,
};

const struct spi_nor_manufacturer spi_nor_general = {
	.name = "general",
	.parts = general_parts,
	.nparts = ARRAY_SIZE(general_parts),
	.fixups = &general_fixups,
};
