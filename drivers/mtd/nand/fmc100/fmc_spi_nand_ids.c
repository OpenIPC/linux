/*
 * Copyright (c) HiSilicon (Shanghai) Technologies Co., Ltd. 2020-2023. All rights reserved.
 */

#include <asm/setup.h>
#include <linux/types.h>
#include <linux/io.h>
#include <linux/sched.h>
#include <linux/printk.h>
#include <linux/platform_device.h>
#include <linux/mtd/partitions.h>
#include <linux/mfd/bsp_fmc.h>
#include <linux/uaccess.h>

#include <linux/securec.h>
#include "fmc100.h"

set_read_std(1, INFINITE, 24);

set_read_fast(1, INFINITE, 80);
set_read_fast(1, INFINITE, 100);
set_read_fast(1, INFINITE, 104);
set_read_fast(1, INFINITE, 108);
set_read_fast(1, INFINITE, 120);
set_read_fast(1, INFINITE, 133);

set_read_dual(1, INFINITE, 80);
set_read_dual(1, INFINITE, 100);
set_read_dual(1, INFINITE, 104);
set_read_dual(1, INFINITE, 108);
set_read_dual(1, INFINITE, 120);
set_read_dual(1, INFINITE, 133);

set_read_dual_addr(1, INFINITE, 40);
set_read_dual_addr(1, INFINITE, 80);
set_read_dual_addr(2, INFINITE, 80);
set_read_dual_addr(1, INFINITE, 100);
set_read_dual_addr(1, INFINITE, 104);
set_read_dual_addr(1, INFINITE, 108);
set_read_dual_addr(1, INFINITE, 120);
set_read_dual_addr(2, INFINITE, 104);

set_read_quad(1, INFINITE, 80);
set_read_quad(1, INFINITE, 100);
set_read_quad(1, INFINITE, 104);
set_read_quad(1, INFINITE, 108);
set_read_quad(1, INFINITE, 120);
set_read_quad(1, INFINITE, 133);

set_read_quad_addr(2, INFINITE, 40);
set_read_quad_addr(1, INFINITE, 80);
set_read_quad_addr(2, INFINITE, 80);
set_read_quad_addr(4, INFINITE, 80);
set_read_quad_addr(1, INFINITE, 100);
set_read_quad_addr(1, INFINITE, 104);
set_read_quad_addr(2, INFINITE, 104);
set_read_quad_addr(1, INFINITE, 108);
set_read_quad_addr(1, INFINITE, 120);
set_read_quad_addr(4, INFINITE, 104);

set_write_std(0, 256, 24);
set_write_std(0, 256, 75);
set_write_std(0, 256, 80);
set_write_std(0, 256, 100);
set_write_std(0, 256, 104);
set_write_std(0, 256, 133);

set_write_quad(0, 256, 80);
set_write_quad(0, 256, 100);
set_write_quad(0, 256, 104);
set_write_quad(0, 256, 108);
set_write_quad(0, 256, 120);
set_write_quad(0, 256, 133);

set_erase_sector_128k(0, _128K, 24);
set_erase_sector_128k(0, _128K, 75);
set_erase_sector_128k(0, _128K, 80);
set_erase_sector_128k(0, _128K, 104);
set_erase_sector_128k(0, _128K, 133);

set_erase_sector_256k(0, _256K, 24);
set_erase_sector_256k(0, _256K, 75);
set_erase_sector_256k(0, _256K, 80);
set_erase_sector_256k(0, _256K, 100);
set_erase_sector_256k(0, _256K, 104);
set_erase_sector_256k(0, _256K, 133);

static struct spi_drv spi_driver_general = {
	.wait_ready = spi_general_wait_ready,
	.write_enable = spi_general_write_enable,
	.qe_enable = spi_general_qe_enable,
};

/* some spi nand flash default QUAD enable, needn't to set qe enable */
static struct spi_drv spi_driver_no_qe = {
	.wait_ready = spi_general_wait_ready,
	.write_enable = spi_general_write_enable,
};

#define SPI_NAND_ID_TAB_VER     "2.7"

/******************************************************************************
 * We do not guarantee the compatibility of the following device models in the
 * table.Device compatibility is based solely on the list of compatible devices
 * in the release package.
 ******************************************************************************/
static struct spi_nand_info fmc_spi_nand_flash_table[] = {
	/* Micron MT29F1G01ABA 1GBit */
	/* name     id  id_len  chipsize(Bytes) erasesize  pagesize oobsize(Bytes) */
	{
		.name      = "MT29F1G01ABA",
		.id        = {0x2C, 0x14},
		.id_len    = 2,
		.chipsize  = _128M,
		.erasesize = _128K,
		.pagesize  = _2K,
		.oobsize   = 128,
		.badblock_pos = BBP_FIRST_PAGE,
		.read      = {
			/* dummy clock:1 byte, read size:INFINITE bytes,
			 * clock frq:24MHz */
			&read_std(1, INFINITE, 24), /* 24MHz */
			&read_fast(1, INFINITE, 80), /* 80MHz */
			&read_dual(1, INFINITE, 80), /* 80MHz */
			&read_dual_addr(1, INFINITE, 80), /* 80MHz */
			&read_quad(1, INFINITE, 80), /* 80MHz */
			&read_quad_addr(2, INFINITE, 80), /* 80MHz */
			0
		},
		.write     = {
			/* dummy clock:0 byte, write size:256 bytes,
			 * clock frq:80MHz */
			&write_std(0, 256, 80), /* 80MHz 256 bytes */
			&write_quad(0, 256, 80), /* 80MHz 256 bytes */
			0
		},
		.erase     = {
			/* dummy clock:0 byte, write size:128KB,
			 * clock frq:80MHz */
			&erase_sector_128k(0, _128K, 80), /* 80MHz */
			0
		},
		.driver    = &spi_driver_no_qe,
	},

	/* Micron MT29F1G01ABB 1GBit 1.8V */
	{
		.name      = "MT29F1G01ABB",
		.id        = {0x2C, 0x15},
		.id_len    = 2,
		.chipsize  = _128M,
		.erasesize = _128K,
		.pagesize  = _2K,
		.oobsize   = 128,
		.badblock_pos = BBP_FIRST_PAGE,
		.read      = {
			&read_std(1, INFINITE, 24),
			&read_fast(1, INFINITE, 80),
			&read_dual(1, INFINITE, 80),
			&read_dual_addr(1, INFINITE, 80),
			&read_quad(1, INFINITE, 80),
			&read_quad_addr(2, INFINITE, 80),
			0
		},
		.write     = {
			&write_std(0, 256, 80),
			&write_quad(0, 256, 80),
			0
		},
		.erase     = {
			&erase_sector_128k(0, _128K, 80),
			0
		},
		.driver    = &spi_driver_no_qe,
	},

	/* Micron MT29F2G01ABA 2GBit */
	{
		.name      = "MT29F2G01ABA",
		.id        = {0x2C, 0x24},
		.id_len    = 2,
		.chipsize  = _256M,
		.erasesize = _128K,
		.pagesize  = _2K,
		.oobsize   = 128,
		.badblock_pos = BBP_FIRST_PAGE,
		.read      = {
			&read_std(1, INFINITE, 24),
			&read_fast(1, INFINITE, 108),
			&read_dual(1, INFINITE, 108),
			&read_dual_addr(1, INFINITE, 108),
			&read_quad(1, INFINITE, 108),
			&read_quad_addr(2, INFINITE, 104),
			0
		},
		.write     = {
			&write_std(0, 256, 80),
			&write_quad(0, 256, 108),
			0
		},
		.erase     = {
			&erase_sector_128k(0, _128K, 80),
			0
		},
		.driver    = &spi_driver_no_qe,
	},

	/* Micron MT29F2G01ABB 2GBit 1.8V */
	{
		.name      = "MT29F2G01ABB",
		.id        = {0x2C, 0x25},
		.id_len    = 2,
		.chipsize  = _256M,
		.erasesize = _128K,
		.pagesize  = _2K,
		.oobsize   = 128,
		.badblock_pos = BBP_FIRST_PAGE,
		.read      = {
			&read_std(1, INFINITE, 24),
			&read_fast(1, INFINITE, 80),
			&read_dual(1, INFINITE, 80),
			&read_dual_addr(1, INFINITE, 80),
			&read_quad(1, INFINITE, 80),
			&read_quad_addr(2, INFINITE, 80),
			0
		},
		.write     = {
			&write_std(0, 256, 80),
			&write_quad(0, 256, 80),
			0
		},
		.erase     = {
			&erase_sector_128k(0, _128K, 80),
			0
		},
		.driver    = &spi_driver_no_qe,
	},

	/* Micron MT29F4G01ADAG 4GBit 3.3V */
	{
		.name      = "MT29F4G01ADAG",
		.id        = {0x2C, 0x36},
		.id_len    = 2,
		.chipsize  = _512M,
		.erasesize = _128K,
		.pagesize  = _2K,
		.oobsize   = 128,
		.badblock_pos = BBP_FIRST_PAGE,
		.read      = {
			&read_std(1, INFINITE, 24),
			&read_fast(1, INFINITE, 108),
			&read_dual(1, INFINITE, 108),
			&read_dual_addr(1, INFINITE, 108),
			&read_quad(1, INFINITE, 108),
			&read_quad_addr(2, INFINITE, 104),
			0
		},
		.write     = {
			&write_std(0, 256, 80),
			&write_quad(0, 256, 108),
			0
		},
		.erase     = {
			&erase_sector_128k(0, _128K, 80),
			0
		},
		.driver    = &spi_driver_no_qe,
	},

	/* ESMT F50L512M41A 512Mbit */
	{
		.name      = "F50L512M41A",
		.id        = {0xC8, 0x20},
		.id_len    = 2,
		.chipsize  = _64M,
		.erasesize = _128K,
		.pagesize  = _2K,
		.oobsize   = 64,
		.badblock_pos = BBP_FIRST_PAGE,
		.read      = {
			&read_std(1, INFINITE, 24),
			&read_fast(1, INFINITE, 104),
			&read_dual(1, INFINITE, 104),
			&read_quad(1, INFINITE, 104),
			0
		},
		.write     = {
			&write_std(0, 256, 24),
			&write_quad(0, 256, 104),
			0
		},
		.erase     = {
			&erase_sector_128k(0, _128K, 24),
			0
		},
		.driver    = &spi_driver_no_qe,
	},

	/* ESMT F50L1G41A 1Gbit */
	{
		.name      = "F50L1G41A",
		.id        = {0xC8, 0x21},
		.id_len    = 2,
		.chipsize  = _128M,
		.erasesize = _128K,
		.pagesize  = _2K,
		.oobsize   = 64,
		.badblock_pos = BBP_FIRST_PAGE,
		.read      = {
			&read_std(1, INFINITE, 24),
			&read_fast(1, INFINITE, 104),
			&read_dual(1, INFINITE, 104),
			&read_quad(1, INFINITE, 104),
			0
		},
		.write     = {
			&write_std(0, 256, 24),
			&write_quad(0, 256, 104),
			0
		},
		.erase     = {
			&erase_sector_128k(0, _128K, 24),
			0
		},
		.driver    = &spi_driver_no_qe,
	},

	/* GD 3.3v GD5F1GQ5UEYIGY/GD5F1GQ5UEYIGR 1Gbit */
	{
		.name      = "GD5F1GQ5UEYIGY",
		.id        = {0xc8, 0x51},
		.id_len    = 2,
		.chipsize  = _128M,
		.erasesize = _128K,
		.pagesize  = _2K,
		.oobsize   = 128,
		.badblock_pos = BBP_FIRST_PAGE,
		.read      = {
			&read_std(1, INFINITE, 24),
			&read_fast(1, INFINITE, 133),
			&read_dual(1, INFINITE, 133),
			&read_quad(1, INFINITE, 133),
			0
		},
		.write     = {
			&write_std(0, 256, 133),
			&write_quad(0, 256, 133),
			0
		},
		.erase     = {
			&erase_sector_128k(0, _128K, 133),
			0
		},
		.driver    = &spi_driver_general,
	},

	/* ESMT F50L1G41LB-104YG2ME 1Gbit */
	{
		.name      = "F50L1G41LB-104YG2ME",
		.id        = {0xC8, 0x01, 0X7F},
		.id_len    = 3,
		.chipsize  = _128M,
		.erasesize = _128K,
		.pagesize  = _2K,
		.oobsize   = 64,
		.badblock_pos = BBP_FIRST_PAGE,
		.read      = {
			&read_std(1, INFINITE, 24),
			&read_fast(1, INFINITE, 104),
			&read_dual(1, INFINITE, 104),
			&read_quad(1, INFINITE, 104),
			0
		},
		.write     = {
			&write_std(0, 256, 104),
			&write_quad(0, 256, 104),
			0
		},
		.erase     = {
			&erase_sector_128k(0, _128K, 104),
			0
		},
		.driver    = &spi_driver_no_qe,
	},

	/* GD 3.3v GD5F1GQ4UAYIG 1Gbit */
	{
		.name      = "GD5F1GQ4UAYIG",
		.id        = {0xc8, 0xf1},
		.id_len    = 2,
		.chipsize  = _128M,
		.erasesize = _128K,
		.pagesize  = _2K,
		.oobsize   = 64,
		.badblock_pos = BBP_FIRST_PAGE,
		.read      = {
			&read_std(1, INFINITE, 24),
			&read_fast(1, INFINITE, 120),
			&read_dual(1, INFINITE, 120),
			&read_dual_addr(1, INFINITE, 120),
			&read_quad(1, INFINITE, 120),
			&read_quad_addr(1, INFINITE, 120),
			0
		},
		.write     = {
			&write_std(0, 256, 24),
			&write_quad(0, 256, 120),
			0
		},
		.erase     = {
			&erase_sector_128k(0, _128K, 24),
			0
		},
		.driver    = &spi_driver_general,
	},

	/* GD 1.8v GD5F1GQ5REYIG 1Gbit */
	{
		.name      = "GD5F1GQ5REYIG",
		.id        = {0xc8, 0x41},
		.id_len    = 2,
		.chipsize  = _128M,
		.erasesize = _128K,
		.pagesize  = _2K,
		.oobsize   = 128,
		.badblock_pos = BBP_FIRST_PAGE,
		.read      = {
			&read_std(1, INFINITE, 24),
			&read_fast(1, INFINITE, 104),
			&read_dual(1, INFINITE, 104),
			&read_quad(1, INFINITE, 104),
			0
		},
		.write     = {
			&write_std(0, 256, 24),
			&write_quad(0, 256, 104),
			0
		},
		.erase     = {
			&erase_sector_128k(0, _128K, 104),
			0
		},
		.driver    = &spi_driver_general,
	},

	/* GD 3.3v GD5F1GQ4UEYIHY 1Gbit */
	{
		.name      = "GD5F1GQ4UEYIHY",
		.id        = {0xc8, 0xd9},
		.id_len    = 2,
		.chipsize  = _128M,
		.erasesize = _128K,
		.pagesize  = _2K,
		.oobsize   = 64,
		.badblock_pos = BBP_FIRST_PAGE,
		.read      = {
			&read_std(1, INFINITE, 24),
			&read_fast(1, INFINITE, 120),
			&read_dual(1, INFINITE, 120),
			&read_dual_addr(1, INFINITE, 120),
			&read_quad(1, INFINITE, 120),
			&read_quad_addr(1, INFINITE, 120),
			0
		},
		.write     = {
			&write_std(0, 256, 104),
			&write_quad(0, 256, 120),
			0
		},
		.erase     = {
			&erase_sector_128k(0, _128K, 104),
			0
		},
		.driver    = &spi_driver_general,
	},

	/* GD 1.8v GD5F1GQ4RB9IG 1Gbit */
	{
		.name      = "GD5F1GQ4RB9IG",
		.id        = {0xc8, 0xc1},
		.id_len    = 2,
		.chipsize  = _128M,
		.erasesize = _128K,
		.pagesize  = _2K,
		.oobsize   = 128,
		.badblock_pos = BBP_FIRST_PAGE,
		.read      = {
			&read_std(1, INFINITE, 24),
			&read_fast(1, INFINITE, 120),
			&read_dual(1, INFINITE, 120),
			&read_dual_addr(1, INFINITE, 120),
			&read_quad(1, INFINITE, 120),
			&read_quad_addr(1, INFINITE, 120),
			0
		},
		.write     = {
			&write_std(0, 256, 24),
			&write_quad(0, 256, 120),
			0
		},
		.erase     = {
			&erase_sector_128k(0, _128K, 24),
			0
		},
		.driver    = &spi_driver_general,
	},

	/* GD 3.3v GD5F1GQ4UBYIG 1Gbit */
	{
		.name      = "GD5F1GQ4UBYIG",
		.id        = {0xc8, 0xd1},
		.id_len    = 2,
		.chipsize  = _128M,
		.erasesize = _128K,
		.pagesize  = _2K,
		.oobsize   = 128,
		.badblock_pos = BBP_FIRST_PAGE,
		.read      = {
			&read_std(1, INFINITE, 24),
			&read_fast(1, INFINITE, 120),
			&read_dual(1, INFINITE, 120),
			&read_dual_addr(1, INFINITE, 120),
			&read_quad(1, INFINITE, 120),
			&read_quad_addr(1, INFINITE, 120),
			0
		},
		.write     = {
			&write_std(0, 256, 24),
			&write_quad(0, 256, 120),
			0
		},
		.erase     = {
			&erase_sector_128k(0, _128K, 24),
			0
		},
		.driver    = &spi_driver_general,
	},

	/* GD 3.3v GD5F2GQ4UAYIG 2Gbit */
	{
		.name      = "GD5F2GQ4UAYIG",
		.id        = {0xc8, 0xf2},
		.id_len    = 2,
		.chipsize  = _256M,
		.erasesize = _128K,
		.pagesize  = _2K,
		.oobsize   = 64,
		.badblock_pos = BBP_FIRST_PAGE,
		.read      = {
			&read_std(1, INFINITE, 24),
			&read_fast(1, INFINITE, 120),
			&read_dual(1, INFINITE, 120),
			&read_dual_addr(1, INFINITE, 120),
			&read_quad(1, INFINITE, 120),
			&read_quad_addr(1, INFINITE, 120),
			0
		},
		.write     = {
			&write_std(0, 256, 24),
			&write_quad(0, 256, 120),
			0
		},
		.erase     = {
			&erase_sector_128k(0, _128K, 24),
			0
		},
		.driver    = &spi_driver_general,
	},

	/* GD 3.3v GD5F2GQ4U9IGR/BYIG 2Gbit */
	{
		.name      = "GD5F2GQ4U9IGR/BYIG",
		.id        = {0xc8, 0xd2},
		.id_len    = 2,
		.chipsize  = _256M,
		.erasesize = _128K,
		.pagesize  = _2K,
		.oobsize   = 128,
		.badblock_pos = BBP_FIRST_PAGE,
		.read      = {
			&read_std(1, INFINITE, 24),
			&read_fast(1, INFINITE, 120),
			&read_dual(1, INFINITE, 120),
			&read_dual_addr(1, INFINITE, 120),
			&read_quad(1, INFINITE, 120),
			&read_quad_addr(1, INFINITE, 120),
			0
		},
		.write     = {
			&write_std(0, 256, 24),
			&write_quad(0, 256, 120),
			0
		},
		.erase     = {
			&erase_sector_128k(0, _128K, 24),
			0
		},
		.driver    = &spi_driver_general,
	},

	/* GD 3.3v GD5F2GQ5UEYIG 2Gbit */
	{
		.name      = "GD5F2GQ5UEYIG",
		.id        = {0xc8, 0x52},
		.id_len    = 2,
		.chipsize  = _256M,
		.erasesize = _128K,
		.pagesize  = _2K,
		.oobsize   = 128,
		.badblock_pos = BBP_FIRST_PAGE,
		.read      = {
			&read_std(1, INFINITE, 24),
			&read_fast(1, INFINITE, 104),
			&read_dual(1, INFINITE, 104),
			&read_dual_addr(2, INFINITE, 104),
			&read_quad(1, INFINITE, 104),
			&read_quad_addr(4, INFINITE, 104),
			0
		},
		.write     = {
			&write_std(0, 256, 104),
			&write_quad(0, 256, 120),
			0
		},
		.erase     = {
			&erase_sector_128k(0, _128K, 104),
			0
		},
		.driver    = &spi_driver_general,
	},

	/* GD 1.8v GD5F2GQ5REYIG 2Gbit */
	{
		.name      = "GD5F2GQ5REYIG",
		.id        = {0xc8, 0x42},
		.id_len    = 2,
		.chipsize  = _256M,
		.erasesize = _128K,
		.pagesize  = _2K,
		.oobsize   = 128,
		.badblock_pos = BBP_FIRST_PAGE,
		.read      = {
			&read_std(1, INFINITE, 24),
			&read_fast(1, INFINITE, 104),
			&read_dual(1, INFINITE, 104),
			&read_dual_addr(2, INFINITE, 104),
			&read_quad(1, INFINITE, 104),
			&read_quad_addr(4, INFINITE, 104),
			0
		},
		.write     = {
			&write_std(0, 256, 104),
			&write_quad(0, 256, 104),
			0
		},
		.erase     = {
			&erase_sector_128k(0, _128K, 104),
			0
		},
		.driver    = &spi_driver_general,
	},

	/* GD 3.3v GD5F4GQ4UAYIG 4Gbit */
	{
		.name      = "GD5F4GQ4UAYIG",
		.id        = {0xc8, 0xf4},
		.id_len    = 2,
		.chipsize  = _512M,
		.erasesize = _128K,
		.pagesize  = _2K,
		.oobsize   = 64,
		.badblock_pos = BBP_FIRST_PAGE,
		.read      = {
			&read_std(1, INFINITE, 24),
			&read_fast(1, INFINITE, 120),
			&read_dual(1, INFINITE, 120),
			&read_dual_addr(1, INFINITE, 120),
			&read_quad(1, INFINITE, 120),
			&read_quad_addr(1, INFINITE, 120),
			0
		},
		.write     = {
			&write_std(0, 256, 24),
			&write_quad(0, 256, 120),
			0
		},
		.erase     = {
			&erase_sector_128k(0, _128K, 24),
			0
		},
		.driver    = &spi_driver_general,
	},

	/* GD 3.3v GD5F4GQ4UBYIG 4Gbit */
	{
		.name      = "GD5F4GQ4UBYIG",
		.id        = {0xc8, 0xd4},
		.id_len    = 2,
		.chipsize  = _512M,
		.erasesize = _256K,
		.pagesize  = _4K,
		.oobsize   = 256,
		.badblock_pos = BBP_FIRST_PAGE,
		.read      = {
			&read_std(1, INFINITE, 24),
			&read_fast(1, INFINITE, 120),
			&read_dual(1, INFINITE, 120),
			&read_dual_addr(1, INFINITE, 120),
			&read_quad(1, INFINITE, 120),
			&read_quad_addr(1, INFINITE, 120),
			0
		},
		.write     = {
			&write_std(0, 256, 24),
			&write_quad(0, 256, 120),
			0
		},
		.erase     = {
			&erase_sector_256k(0, _256K, 24),
			0
		},
		.driver    = &spi_driver_general,
	},

	/* GD 3.3v GD5F4GQ6UEYIG 4Gbit */
	{
		.name      = "GD5F4GQ6UEYIG",
		.id        = {0xc8, 0x55},
		.id_len    = 2,
		.chipsize  = _512M,
		.erasesize = _128K,
		.pagesize  = _2K,
		.oobsize   = 128,
		.badblock_pos = BBP_FIRST_PAGE,
		.read      = {
			&read_std(1, INFINITE, 24), /* 24MHz */
			&read_fast(1, INFINITE, 104),  /* 104MHz */
			&read_dual(1, INFINITE, 104),  /* 104MHz */
			&read_dual_addr(2, INFINITE, 104),  /* 104MHz */
			&read_quad(1, INFINITE, 104),  /* 104MHz */
			&read_quad_addr(4, INFINITE, 104),  /* 104MHz */
			0
		},
		.write     = {
			&write_std(0, 256, 24),  /* 24MHz */
			&write_quad(0, 256, 104),  /* 104MHz */
			0
		},
		.erase     = {
			&erase_sector_128k(0, _128K, 104),  /* 104MHz */
			0
		},
		.driver    = &spi_driver_general,
	},

	/* GD 1.8V GD5F1GQ4RB9IGR 1Gbit */
	{
		.name      = "GD5F1GQ4RB9IGR",
		.id        = {0xc8, 0xc1},
		.id_len    = 2,
		.chipsize  = _128M,
		.erasesize = _128K,
		.pagesize  = _2K,
		.oobsize   = 128,
		.badblock_pos = BBP_FIRST_PAGE,
		.read      = {
			&read_std(1, INFINITE, 24),
			&read_fast(1, INFINITE, 104),
			&read_dual(1, INFINITE, 104),
			&read_dual_addr(1, INFINITE, 104),
			&read_quad(1, INFINITE, 104),
			&read_quad_addr(1, INFINITE, 104),
			0
		},
		.write     = {
			&write_std(0, 256, 24),
			&write_quad(0, 256, 104),
			0
		},
		.erase     = {
			&erase_sector_128k(0, _128K, 24),
			0
		},
		.driver    = &spi_driver_general,
	},

	/* GD 1.8V GD5F2GQ4RB9IGR 2Gbit */
	{
		.name      = "GD5F2GQ4RB9IGR",
		.id        = {0xc8, 0xc2},
		.id_len    = 2,
		.chipsize  = _256M,
		.erasesize = _128K,
		.pagesize  = _2K,
		.oobsize   = 128,
		.badblock_pos = BBP_FIRST_PAGE,
		.read      = {
			&read_std(1, INFINITE, 24),
			&read_fast(1, INFINITE, 104),
			&read_dual(1, INFINITE, 104),
			&read_dual_addr(1, INFINITE, 104),
			&read_quad(1, INFINITE, 104),
			&read_quad_addr(1, INFINITE, 104),
			0
		},
		.write     = {
			&write_std(0, 256, 24),
			&write_quad(0, 256, 104),
			0
		},
		.erase     = {
			&erase_sector_128k(0, _128K, 24),
			0
		},
		.driver    = &spi_driver_general,
	},
	/* GD 1.8V 5F4GQ6RE9IG 4Gbit */
	{
		.name      = "GD5F4GQ6RE9IG",
		.id        = {0xc8, 0x45},
		.id_len    = 2,
		.chipsize  = _512M,
		.erasesize = _128K,
		.pagesize  = _2K,
		.oobsize   = 128,
		.badblock_pos = BBP_FIRST_PAGE,
		.read      = {
			&read_std(1, INFINITE, 24),
			&read_fast(1, INFINITE, 80),
			&read_dual(1, INFINITE, 80),
			&read_dual_addr(2, INFINITE, 80),
			&read_quad(1, INFINITE, 80),
			&read_quad_addr(4, INFINITE, 80),
			0
		},
		.write     = {
			&write_std(0, 256, 80),
			&write_quad(0, 256, 80),
			0
		},
		.erase     = {
			&erase_sector_128k(0, _128K, 80),
			0
		},
		.driver    = &spi_driver_general,
	},
	/* GD 1.8V GD5F4GQ4RAYIG 4Gbit */
	{
		.name      = "GD5F4GQ4RAYIG",
		.id        = {0xc8, 0xe4},
		.id_len    = 2,
		.chipsize  = _512M,
		.erasesize = _256K,
		.pagesize  = _4K,
		.oobsize   = 256,
		.badblock_pos = BBP_FIRST_PAGE,
		.read      = {
			&read_std(1, INFINITE, 24),
			&read_fast(1, INFINITE, 104),
			&read_dual(1, INFINITE, 104),
			&read_dual_addr(1, INFINITE, 104),
			&read_quad(1, INFINITE, 104),
			&read_quad_addr(1, INFINITE, 104),
			0
		},
		.write     = {
			&write_std(0, 256, 24),
			&write_quad(0, 256, 104),
			0
		},
		.erase     = {
			&erase_sector_256k(0, _256K, 24),
			0
		},
		.driver    = &spi_driver_general,
	},

	/* Winbond 1.8V W25N02JWZEIF 2Gbit */
	{
		.name      = "W25N02JWZEIF",
		.id        = {0xef, 0xbf, 0x22},
		.id_len    = 3,
		.chipsize  = _256M,
		.erasesize = _128K,
		.pagesize  = _2K,
		.oobsize   = 64,
		.badblock_pos = BBP_FIRST_PAGE,
		.read      = {
			&read_std(1, INFINITE, 24),
			&read_fast(1, INFINITE, 120),
			&read_dual(1, INFINITE, 120),
			&read_dual_addr(1, INFINITE, 120),
			&read_quad(1, INFINITE, 120),
			&read_quad_addr(2, INFINITE, 104),
			0
		},
		.write     = {
			&write_std(0, 256, 104),
			&write_quad(0, 256, 104),
			0
		},
		.erase     = {
			&erase_sector_128k(0, _128K, 104),
			0
		},
		.driver  = &spi_driver_general,
	},

	/* GD 1.8V 5F4GQ4RBYIG 4Gbit */
	{
		.name      = "5F4GQ4RBYIG",
		.id        = {0xc8, 0xc4},
		.id_len    = 2,
		.chipsize  = _512M,
		.erasesize = _256K,
		.pagesize  = _4K,
		.oobsize   = 256,
		.badblock_pos = BBP_FIRST_PAGE,
		.read      = {
			&read_std(1, INFINITE, 24),
			&read_fast(1, INFINITE, 120),
			&read_dual(1, INFINITE, 120),
			&read_dual_addr(1, INFINITE, 120),
			&read_quad(1, INFINITE, 120),
			&read_quad_addr(1, INFINITE, 120),
			0
		},
		.write     = {
			&write_std(0, 256, 24),
			&write_quad(0, 256, 120),
			0
		},
		.erase     = {
			&erase_sector_256k(0, _256K, 24),
			0
		},
		.driver    = &spi_driver_general,
	},

	/* Winbond W25N01GV 1Gbit 3.3V */
	{
		.name      = "W25N01GV",
		.id        = {0xef, 0xaa, 0x21},
		.id_len    = 3,
		.chipsize  = _128M,
		.erasesize = _128K,
		.pagesize  = _2K,
		.oobsize   = 64,
		.badblock_pos = BBP_FIRST_PAGE,
		.read      = {
			&read_std(1, INFINITE, 24),
			&read_fast(1, INFINITE, 104),
			&read_dual(1, INFINITE, 104),
			&read_dual_addr(1, INFINITE, 104),
			&read_quad(1, INFINITE, 104),
			&read_quad_addr(2, INFINITE, 104),
			0
		},
		.write     = {
			&write_std(0, 256, 24),
			&write_quad(0, 256, 104),
			0
		},
		.erase     = {
			&erase_sector_128k(0, _128K, 24),
			0
		},
		.driver    = &spi_driver_no_qe,
	},

	/* Winbond 3.3V W25N04KVZEIR 4Gbit: the W25N02KV with twice the blocks,
	 * as OpenIPC U-Boot's table has it (4096 x 128 KiB, 2 KiB pages, 128 B
	 * OOB). The Taobao Hi3519DV500 kit boots from it. */
	{
		.name      = "W25N04KVZEIR",
		.id        = {0xef, 0xaa, 0x23},
		.id_len    = 3,
		.chipsize  = _512M,
		.erasesize = _128K,
		.pagesize  = _2K,
		.oobsize   = 128,
		.badblock_pos = BBP_FIRST_PAGE,
		.read      = {
			&read_std(1, INFINITE, 24),
			&read_fast(1, INFINITE, 104),
			&read_dual(1, INFINITE, 104),
			&read_dual_addr(1, INFINITE, 104),
			&read_quad(1, INFINITE, 104),
			&read_quad_addr(2, INFINITE, 104),
			0
		},
		.write     = {
			&write_std(0, 256, 104),
			&write_quad(0, 256, 104),
			0
		},
		.erase     = {
			&erase_sector_128k(0, _128K, 104),
			0
		},
		.driver    = &spi_driver_general,
	},

	/* Winbond W25N01GWZEIG 1Gbit 1.8V */
	{
		.name      = "W25N01GWZEIG",
		.id        = {0xef, 0xba, 0x21},
		.id_len    = 3,
		.chipsize  = _128M,
		.erasesize = _128K,
		.pagesize  = _2K,
		.oobsize   = 64,
		.badblock_pos = BBP_FIRST_PAGE,
		.read      = {
			&read_std(1, INFINITE, 24),
			&read_fast(1, INFINITE, 104),
			&read_dual(1, INFINITE, 104),
			&read_dual_addr(1, INFINITE, 104),
			&read_quad(1, INFINITE, 104),
			&read_quad_addr(2, INFINITE, 104),
			0
		},
		.write     = {
			&write_std(0, 256, 24),
			&write_quad(0, 256, 80),
			0
		},
		.erase     = {
			&erase_sector_128k(0, _128K, 24),
			0
		},
		.driver    = &spi_driver_no_qe,
	},

	/* ATO ATO25D1GA 1Gbit */
	{
		.name      = "ATO25D1GA",
		.id        = {0x9b, 0x12},
		.id_len    = 2,
		.chipsize  = _128M,
		.erasesize = _128K,
		.pagesize  = _2K,
		.oobsize   = 64,
		.badblock_pos = BBP_FIRST_PAGE,
		.read      = {
			&read_std(1, INFINITE, 24),
			&read_fast(1, INFINITE, 104),
			&read_quad(1, INFINITE, 104),
			0
		},
		.write     = {
			&write_std(0, 256, 24),
			&write_quad(0, 256, 104),
			0
		},
		.erase     = {
			&erase_sector_128k(0, _128K, 24),
			0
		},
		.driver    = &spi_driver_general,
	},

	/* MXIC MX35LF1GE4AB 1Gbit */
	{
		.name      = "MX35LF1GE4AB",
		.id        = {0xc2, 0x12},
		.id_len    = 2,
		.chipsize  = _128M,
		.erasesize = _128K,
		.pagesize  = _2K,
		.oobsize   = 64,
		.badblock_pos = BBP_FIRST_PAGE,
		.read      = {
			&read_std(1, INFINITE, 24),
			&read_fast(1, INFINITE, 104),
			&read_quad(1, INFINITE, 104),
			0
		},
		.write     = {
			&write_std(0, 256, 24),
			&write_quad(0, 256, 104),
			0
		},
		.erase     = {
			&erase_sector_128k(0, _128K, 24),
			0
		},
		.driver    = &spi_driver_general,
	},

	/* MXIC MX35UF1G14AC 1Gbit 1.8V */
	{
		.name      = "MX35UF1G14AC",
		.id        = {0xc2, 0x90},
		.id_len    = 2,
		.chipsize  = _128M,
		.erasesize = _128K,
		.pagesize  = _2K,
		.oobsize   = 64,
		.badblock_pos = BBP_FIRST_PAGE,
		.read      = {
			&read_std(1, INFINITE, 24),
			&read_fast(1, INFINITE, 104),
			&read_quad(1, INFINITE, 104),
			0
		},
		.write     = {
			&write_std(0, 256, 24),
			&write_quad(0, 256, 104),
			0
		},
		.erase     = {
			&erase_sector_128k(0, _128K, 104),
			0
		},
		.driver    = &spi_driver_general,
	},

	/* MXIC MX35LF2GE4AB 2Gbit SOP-16Pin */
	{
		.name      = "MX35LF2GE4AB",
		.id        = {0xc2, 0x22},
		.id_len    = 2,
		.chipsize  = _256M,
		.erasesize = _128K,
		.pagesize  = _2K,
		.oobsize   = 64,
		.badblock_pos = BBP_FIRST_PAGE,
		.read      = {
			&read_std(1, INFINITE, 24),
			&read_fast(1, INFINITE, 104),
			&read_quad(1, INFINITE, 104),
			0
		},
		.write     = {
			&write_std(0, 256, 24),
			&write_quad(0, 256, 104),
			0
		},
		.erase     = {
			&erase_sector_128k(0, _128K, 24),
			0
		},
		.driver    = &spi_driver_general,
	},

	/* MXIC MX35LF2G14AC 2GBit */
	{
		.name      = "MX35LF2G14AC",
		.id        = {0xc2, 0x20},
		.id_len    = 2,
		.chipsize  = _256M,
		.erasesize = _128K,
		.pagesize  = _2K,
		.oobsize   = 64,
		.badblock_pos = BBP_FIRST_PAGE,
		.read      = {
			&read_std(1, INFINITE, 24),
			&read_fast(1, INFINITE, 104),
			&read_quad(1, INFINITE, 104),
			0
		},
		.write     = {
			&write_std(0, 256, 24),
			&write_quad(0, 256, 104),
			0
		},
		.erase     = {
			&erase_sector_128k(0, _128K, 24),
			0
		},
		.driver    = &spi_driver_general,
	},

	/* MXIC MX35UF2G14AC 2Gbit 1.8V */
	{
		.name      = "MX35UF2G14AC",
		.id        = {0xc2, 0xa0},
		.id_len    = 2,
		.chipsize  = _256M,
		.erasesize = _128K,
		.pagesize  = _2K,
		.oobsize   = 64,
		.badblock_pos = BBP_FIRST_PAGE,
		.read      = {
			&read_std(1, INFINITE, 24),
			&read_fast(1, INFINITE, 104),
			&read_quad(1, INFINITE, 104),
			0
		},
		.write     = {
			&write_std(0, 256, 24),
			&write_quad(0, 256, 104),
			0
		},
		.erase     = {
			&erase_sector_128k(0, _128K, 104),
			0
		},
		.driver    = &spi_driver_general,
	},

	/* Paragon PN26G01A 1Gbit */
	{
		.name      = "PN26G01A",
		.id        = {0xa1, 0xe1},
		.id_len    = 2,
		.chipsize  = _128M,
		.erasesize = _128K,
		.pagesize  = _2K,
		.oobsize   = 128,
		.badblock_pos = BBP_FIRST_PAGE,
		.read      = {
			&read_std(1, INFINITE, 24),
			&read_fast(1, INFINITE, 108),
			&read_dual(1, INFINITE, 108),
			&read_dual_addr(1, INFINITE, 108),
			&read_quad(1, INFINITE, 108),
			&read_quad_addr(1, INFINITE, 108),
			0
		},
		.write     = {
			&write_std(0, 256, 24),
			&write_quad(0, 256, 108),
			0
		},
		.erase     = {
			&erase_sector_128k(0, _128K, 24),
			0
		},
		.driver    = &spi_driver_general,
	},

	/* Paragon PN26G02A 2Gbit */
	{
		.name      = "PN26G02A",
		.id        = {0xa1, 0xe2},
		.id_len    = 2,
		.chipsize  = _256M,
		.erasesize = _128K,
		.pagesize  = _2K,
		.oobsize   = 128,
		.badblock_pos = BBP_FIRST_PAGE,
		.read      = {
			&read_std(1, INFINITE, 24),
			&read_fast(1, INFINITE, 108),
			&read_dual(1, INFINITE, 108),
			&read_dual_addr(1, INFINITE, 108),
			&read_quad(1, INFINITE, 108),
			&read_quad_addr(1, INFINITE, 108),
			0
		},
		.write     = {
			&write_std(0, 256, 24),
			&write_quad(0, 256, 108),
			0
		},
		.erase     = {
			&erase_sector_128k(0, _128K, 24),
			0
		},
		.driver    = &spi_driver_general,
	},

	/* All-flash AFS1GQ4UAC 1Gbit */
	{
		.name      = "AFS1GQ4UAC",
		.id        = {0xc1, 0x51},
		.id_len    = 2,
		.chipsize  = _128M,
		.erasesize = _128K,
		.pagesize  = _2K,
		.oobsize   = 128,
		.badblock_pos = BBP_FIRST_PAGE,
		.read      = {
			&read_std(1, INFINITE, 24),
			&read_fast(1, INFINITE, 80),
			&read_dual(1, INFINITE, 80),
			&read_dual_addr(1, INFINITE, 80),
			&read_quad(1, INFINITE, 80),
			&read_quad_addr(1, INFINITE, 80),
			0
		},
		.write     = {
			&write_std(0, 256, 24),
			&write_quad(0, 256, 80),
			0
		},
		.erase     = {
			&erase_sector_128k(0, _128K, 24),
			0
		},
		.driver    = &spi_driver_general,
	},

	/* All-flash AFS2GQ4UAD 2Gbit */
	{
		.name      = "AFS2GQ4UAD",
		.id        = {0xc1, 0x52},
		.id_len    = 2,
		.chipsize  = _256M,
		.erasesize = _128K,
		.pagesize  = _2K,
		.oobsize   = 128,
		.badblock_pos = BBP_FIRST_PAGE,
		.read      = {
			&read_std(1, INFINITE, 24),
			&read_fast(1, INFINITE, 80),
			&read_dual(1, INFINITE, 80),
			&read_dual_addr(1, INFINITE, 80),
			&read_quad(1, INFINITE, 80),
			&read_quad_addr(1, INFINITE, 80),
			0
		},
		.write     = {
			&write_std(0, 256, 24),
			&write_quad(0, 256, 80),
			0
		},
		.erase     = {
			&erase_sector_128k(0, _128K, 24),
			0
		},
		.driver    = &spi_driver_general,
	},

	/* TOSHIBA TC58CVG0S3H 1Gbit */
	{
		.name      = "TC58CVG0S3H",
		.id        = {0x98, 0xc2},
		.id_len    = 2,
		.chipsize  = _128M,
		.erasesize = _128K,
		.pagesize  = _2K,
		.oobsize   = 128,
		.badblock_pos = BBP_FIRST_PAGE,
		.read      = {
			&read_std(1, INFINITE, 24),
			&read_fast(1, INFINITE, 104),
			&read_dual(1, INFINITE, 104),
			&read_quad(1, INFINITE, 104),
			0
		},
		.write     = {
			&write_std(0, 256, 104),
			0
		},
		.erase     = {
			&erase_sector_128k(0, _128K, 104),
			0
		},
		.driver    = &spi_driver_no_qe,
	},

	{
		.name      = "TC58CVG0S3HRAIJ",
		.id        = {0x98, 0xe2, 0x40},
		.id_len    = 3,
		.chipsize  = _128M,
		.erasesize = _128K,
		.pagesize  = _2K,
		.oobsize   = 128,
		.badblock_pos = BBP_FIRST_PAGE,
		.read      = {
			&read_std(1, INFINITE, 24),
			&read_fast(1, INFINITE, 133),
			&read_dual(1, INFINITE, 133),
			&read_quad(1, INFINITE, 133),
			0
		},
		.write     = {
			&write_std(0, 256, 133),
			&write_quad(0, 256, 133),
			0
		},
		.erase     = {
			&erase_sector_128k(0, _128K, 133),
			0
		},
		.driver    = &spi_driver_no_qe,
	},
	/* TOSHIBA TC58CYG0S3H 1.8V 1Gbit */
	{
		.name      = "TC58CYG0S3H",
		.id        = {0x98, 0xb2},
		.id_len    = 2,
		.chipsize  = _128M,
		.erasesize = _128K,
		.pagesize  = _2K,
		.oobsize   = 128,
		.badblock_pos = BBP_FIRST_PAGE,
		.read      = {
			&read_std(1, INFINITE, 24),
			&read_fast(1, INFINITE, 104),
			&read_dual(1, INFINITE, 104),
			&read_quad(1, INFINITE, 104),
			0
		},
		.write     = {
			&write_std(0, 256, 104),
			0
		},
		.erase     = {
			&erase_sector_128k(0, _128K, 104),
			0
		},
		.driver    = &spi_driver_no_qe,
	},

	/* TOSHIBA TC58CYG0S3HRAIJ 1.8V 1Gbit */
	{
		.name      = "TC58CYG0S3HRAIJ",
		.id        = {0x98, 0xD2, 0x40},
		.id_len    = 3,
		.chipsize  = _128M,
		.erasesize = _128K,
		.pagesize  = _2K,
		.oobsize   = 128,
		.badblock_pos = BBP_FIRST_PAGE,
		.read      = {
			&read_std(1, INFINITE, 24),
			&read_fast(1, INFINITE, 133),
			&read_dual(1, INFINITE, 133),
			&read_quad(1, INFINITE, 133),
			0
		},
		.write     = {
			&write_std(0, 256, 133),
			&write_quad(0, 256, 133),
			0
		},
		.erase     = {
			&erase_sector_128k(0, _128K, 133),
			0
		},
		.driver    = &spi_driver_no_qe,
	},

	/* TOSHIBA TC58CVG1S3H 2Gbit */
	{
		.name      = "TC58CVG1S3H",
		.id        = {0x98, 0xcb},
		.id_len    = 2,
		.chipsize  = _256M,
		.erasesize = _128K,
		.pagesize  = _2K,
		.oobsize   = 128,
		.badblock_pos = BBP_FIRST_PAGE,
		.read      = {
			&read_std(1, INFINITE, 24),
			&read_fast(1, INFINITE, 104),
			&read_dual(1, INFINITE, 104),
			&read_quad(1, INFINITE, 104),
			0
		},
		.write     = {
			&write_std(0, 256, 104),
			0
		},
		.erase     = {
			&erase_sector_128k(0, _128K, 104),
			0
		},
		.driver    = &spi_driver_no_qe,
	},

	/* TOSHIBA TC58CVG1S3HRAIJ 2Gbit */
	{
		.name      = "TC58CVG1S3HRAIJ",
		.id        = {0x98, 0xeb, 0x40},
		.id_len    = 3,
		.chipsize  = _256M,
		.erasesize = _128K,
		.pagesize  = _2K,
		.oobsize   = 128,
		.badblock_pos = BBP_FIRST_PAGE,
		.read      = {
			&read_std(1, INFINITE, 24),
			&read_fast(1, INFINITE, 133),
			&read_dual(1, INFINITE, 133),
			&read_quad(1, INFINITE, 133),
			0
		},
		.write     = {
			&write_std(0, 256, 133),
			&write_quad(0, 256, 133),
			0
		},
		.erase     = {
			&erase_sector_128k(0, _128K, 133),
			0
		},
		.driver    = &spi_driver_no_qe,
	},

	/* TOSHIBA TC58CYG1S3H 1.8V 2Gbit */
	{
		.name      = "TC58CYG1S3H",
		.id        = {0x98, 0xbb},
		.id_len    = 2,
		.chipsize  = _256M,
		.erasesize = _128K,
		.pagesize  = _2K,
		.oobsize   = 128,
		.badblock_pos = BBP_FIRST_PAGE,
		.read      = {
			&read_std(1, INFINITE, 24),
			&read_fast(1, INFINITE, 104),
			&read_dual(1, INFINITE, 104),
			&read_quad(1, INFINITE, 104),
			0
		},
		.write     = {
			&write_std(0, 256, 75),
			0
		},
		.erase     = {
			&erase_sector_128k(0, _128K, 75),
			0
		},
		.driver    = &spi_driver_no_qe,
	},

	/* TOSHIBA TC58CYG1S3HRAIJ 1.8V 2Gbit */
	{
		.name      = "TC58CYG1S3HRAIJ",
		.id        = {0x98, 0xdb, 0x40},
		.id_len    = 3,
		.chipsize  = _256M,
		.erasesize = _128K,
		.pagesize  = _2K,
		.oobsize   = 128,
		.badblock_pos = BBP_FIRST_PAGE,
		.read      = {
			&read_std(1, INFINITE, 24),
			&read_fast(1, INFINITE, 133),
			&read_dual(1, INFINITE, 133),
			&read_quad(1, INFINITE, 133),
			0
		},
		.write     = {
			&write_std(0, 256, 133),
			&write_quad(0, 256, 133),
			0
		},
		.erase     = {
			&erase_sector_128k(0, _128K, 133),
			0
		},
		.driver    = &spi_driver_no_qe,
	},

	/* TOSHIBA TC58CVG2S0H 4Gbit */
	{
		.name      = "TC58CVG2S0H",
		.id        = {0x98, 0xcd},
		.id_len    = 2,
		.chipsize  = _512M,
		.erasesize = _256K,
		.pagesize  = _4K,
		.oobsize   = 256,
		.badblock_pos = BBP_FIRST_PAGE,
		.read      = {
			&read_std(1, INFINITE, 24),
			&read_fast(1, INFINITE, 104),
			&read_dual(1, INFINITE, 104),
			&read_quad(1, INFINITE, 104),
			0
		},
		.write     = {
			&write_std(0, 256, 104),
			0
		},
		.erase     = {
			&erase_sector_256k(0, _256K, 104),
			0
		},
		.driver    = &spi_driver_no_qe,
	},

	/* TOSHIBA TC58CVG2S0HRAIJ 4Gbit */
	{
		.name      = "TC58CVG2S0HRAIJ",
		.id        = {0x98, 0xed, 0x51},
		.id_len    = 3,
		.chipsize  = _512M,
		.erasesize = _256K,
		.pagesize  = _4K,
		.oobsize   = 256,
		.badblock_pos = BBP_FIRST_PAGE,
		.read      = {
			&read_std(1, INFINITE, 24),
			&read_fast(1, INFINITE, 133),
			&read_dual(1, INFINITE, 133),
			&read_quad(1, INFINITE, 133),
			0
		},
		.write     = {
			&write_std(0, 256, 133),
			&write_quad(0, 256, 133),
			0
		},
		.erase     = {
			&erase_sector_256k(0, _256K, 133),
			0
		},
		.driver    = &spi_driver_no_qe,
	},

	/* TOSHIBA TC58CYG2S0H 1.8V 4Gbit */
	{
		.name      = "TC58CYG2S0H",
		.id        = {0x98, 0xbd},
		.id_len    = 2,
		.chipsize  = _512M,
		.erasesize = _256K,
		.pagesize  = _4K,
		.oobsize   = 256,
		.badblock_pos = BBP_FIRST_PAGE,
		.read      = {
			&read_std(1, INFINITE, 24),
			&read_fast(1, INFINITE, 104),
			&read_dual(1, INFINITE, 104),
			&read_quad(1, INFINITE, 104),
			0
		},
		.write     = {
			&write_std(0, 256, 75),
			0
		},
		.erase     = {
			&erase_sector_256k(0, _256K, 75),
			0
		},
		.driver    = &spi_driver_no_qe,
	},

	/* KIOXIA TH58CYG2S0HRAIJ 1.8V 4Gbit */
	{
		.name      = "TH58CYG2S0HRAIJ",
		.id        = {0x98, 0xdd, 0x51},
		.id_len    = 3,
		.chipsize  = _512M,
		.erasesize = _256K,
		.pagesize  = _4K,
		.oobsize   = 256,
		.badblock_pos = BBP_FIRST_PAGE,
		.read      = {
			&read_std(1, INFINITE, 24), /* 24MHz */
			&read_fast(1, INFINITE, 133), /* 133MHz */
			&read_dual(1, INFINITE, 133), /* 133MHz */
			&read_quad(1, INFINITE, 133), /* 133MHz */
			0
		},
		.write     = {
			&write_std(0, 256, 133), /* 133MHz */
			&write_quad(0, 256, 133), /* 133MHz */
			0
		},
		.erase     = {
			&erase_sector_256k(0, _256K, 133), /* 133MHz */
			0
		},
		.driver    = &spi_driver_no_qe,
	},

	/* KIOXIA TH58CYG3S0H 1.8V 8Gbit */
	{
		.name      = "TH58CYG3S0H",
		.id        = {0x98, 0xd4, 0x51},
		.id_len    = 3,
		.chipsize  = _1G,
		.erasesize = _256K,
		.pagesize  = _4K,
		.oobsize   = 256,
		.badblock_pos = BBP_FIRST_PAGE,
		.read      = {
			&read_std(1, INFINITE, 24), /* 24MHz */
			&read_fast(1, INFINITE, 133), /* 133MHz */
			&read_dual(1, INFINITE, 133), /* 133MHz */
			&read_quad(1, INFINITE, 133), /* 133MHz */
			0
		},
		.write     = {
			&write_std(0, 256, 133), /* 133MHz */
			&write_quad(0, 256, 133), /* 133MHz */
			0
		},
		.erase     = {
			&erase_sector_256k(0, _256K, 133), /* 133MHz */
			0
		},
		.driver    = &spi_driver_no_qe,
	},

	/* HeYangTek HYF1GQ4UAACAE 1Gbit */
	{
		.name      = "HYF1GQ4UAACAE",
		.id        = {0xc9, 0x51},
		.id_len    = 2,
		.chipsize  = _128M,
		.erasesize = _128K,
		.pagesize  = _2K,
		.oobsize   = 128,
		.badblock_pos = BBP_FIRST_PAGE,
		.read      = {
			&read_std(1, INFINITE, 24),
			&read_fast(1, INFINITE, 80),
			&read_dual(1, INFINITE, 80),
			&read_dual_addr(1, INFINITE, 80),
			&read_quad(1, INFINITE, 80),
			&read_quad_addr(1, INFINITE, 80),
			0
		},
		.write     = {
			&write_std(0, 256, 80),
			&write_quad(0, 256, 80),
			0
		},
		.erase     = {
			&erase_sector_128k(0, _128K, 80),
			0
		},
		.driver    = &spi_driver_general,
	},

	/* HeYangTek HYF2GQ4UAACAE 2Gbit */
	{
		.name      = "HYF2GQ4UAACAE",
		.id        = {0xc9, 0x52},
		.id_len    = 2,
		.chipsize  = _256M,
		.erasesize = _128K,
		.pagesize  = _2K,
		.oobsize   = 128,
		.badblock_pos = BBP_FIRST_PAGE,
		.read      = {
			&read_std(1, INFINITE, 24),
			&read_fast(1, INFINITE, 80),
			&read_dual(1, INFINITE, 80),
			&read_dual_addr(1, INFINITE, 80),
			&read_quad(1, INFINITE, 80),
			&read_quad_addr(1, INFINITE, 80),
			0
		},
		.write     = {
			&write_std(0, 256, 80),
			&write_quad(0, 256, 80),
			0
		},
		.erase     = {
			&erase_sector_128k(0, _128K, 80),
			0
		},
		.driver    = &spi_driver_general,
	},

	/* HeYangTek HYF4GQ4UAACBE 4Gbit */
	{
		.name      = "HYF4GQ4UAACBE",
		.id        = {0xc9, 0xd4},
		.id_len    = 2,
		.chipsize  = _512M,
		.erasesize = _256K,
		.pagesize  = _4K,
		.oobsize   = 256,
		.badblock_pos = BBP_FIRST_PAGE,
		.read      = {
			&read_std(1, INFINITE, 24),
			&read_fast(1, INFINITE, 80),
			&read_dual(1, INFINITE, 80),
			&read_dual_addr(1, INFINITE, 80),
			&read_quad(1, INFINITE, 80),
			&read_quad_addr(1, INFINITE, 80),
			0
		},
		.write     = {
			&write_std(0, 256, 80),
			&write_quad(0, 256, 80),
			0
		},
		.erase     = {
			&erase_sector_256k(0, _256K, 80),
			0
		},
		.driver    = &spi_driver_general,
	},

	/* Dosilicon 3.3V DS35Q1GA-IB 1Gbit */
	{
		.name      = "DS35Q1GA-IB",
		.id        = {0xe5, 0x71},
		.id_len    = 2,
		.chipsize  = _128M,
		.erasesize = _128K,
		.pagesize  = _2K,
		.oobsize   = 64,
		.badblock_pos = BBP_FIRST_PAGE,
		.read      = {
			&read_std(1, INFINITE, 24),
			&read_fast(1, INFINITE, 104),
			&read_dual(1, INFINITE, 104),
			&read_quad(1, INFINITE, 104),
			0
		},
		.write     = {
			&write_std(0, 256, 80),
			&write_quad(0, 256, 104),
			0
		},
		.erase     = {
			&erase_sector_128k(0, _128K, 104),
			0
		},
		.driver    = &spi_driver_general,
	},

	/* XTX 3.3V XT26G01B 1Gbit */
	{
		.name      = "XT26G01B",
		.id        = {0x0B, 0xF1},
		.id_len    = 2,
		.chipsize  = _128M,
		.erasesize = _128K,
		.pagesize  = _2K,
		.oobsize   = 64,
		.badblock_pos = BBP_FIRST_PAGE,
		.read      = {
			&read_std(1, INFINITE, 24),
			&read_fast(1, INFINITE, 80),
			&read_dual(1, INFINITE, 80),
			&read_dual_addr(1, INFINITE, 80),
			&read_quad(1, INFINITE, 80),
			&read_quad_addr(1, INFINITE, 80),
			0
		},
		.write     = {
			&write_std(0, 256, 80),
			&write_quad(0, 256, 80),
			0
		},
		.erase     = {
			&erase_sector_128k(0, _128K, 80),
			0
		},
		.driver    = &spi_driver_general,
	},

	/* Etron 1.8V EM78F044VCA-H 8Gbit */
	{
		.name      = "EM78F044VCA-H",
		.id        = {0xD5, 0x8D},
		.id_len    = 2,
		.chipsize  = _512M * 2,
		.erasesize = _256K,
		.pagesize  = _4K,
		.oobsize   = 256,
		.badblock_pos = BBP_FIRST_PAGE,
		.read      = {
			&read_std(1, INFINITE, 24),
			&read_fast(1, INFINITE, 100),
			&read_dual(1, INFINITE, 100),
			&read_dual_addr(1, INFINITE, 100),
			&read_quad(1, INFINITE, 100),
			&read_quad_addr(1, INFINITE, 100),
			0
		},
		.write     = {
			&write_std(0, 256, 100),
			&write_quad(0, 256, 100),
			0
		},
		.erase     = {
			&erase_sector_256k(0, _256K, 100),
			0
		},
		.driver    = &spi_driver_general,
	},

	/* Etron 1.8V EM78E044VCA-H 4Gbit */
	{
		.name      = "EM78E044VCA-H",
		.id        = {0xD5, 0x8C},
		.id_len    = 2,
		.chipsize  = _512M,
		.erasesize = _256K,
		.pagesize  = _4K,
		.oobsize   = 256,
		.badblock_pos = BBP_FIRST_PAGE,
		.read      = {
			&read_std(1, INFINITE, 24),
			&read_fast(1, INFINITE, 100),
			&read_dual(1, INFINITE, 100),
			&read_dual_addr(1, INFINITE, 100),
			&read_quad(1, INFINITE, 100),
			&read_quad_addr(1, INFINITE, 100),
			0
		},
		.write     = {
			&write_std(0, 256, 100),
			&write_quad(0, 256, 100),
			0
		},
		.erase     = {
			&erase_sector_256k(0, _256K, 100),
			0
		},
		.driver    = &spi_driver_general,
	},

	/* Etron 1.8V EM78D044VCF-H 2Gbit */
	{
		.name      = "EM78D044VCF-H",
		.id        = {0xd5, 0x81},
		.id_len    = 2,
		.chipsize  = _256M,
		.erasesize = _128K,
		.pagesize  = _2K,
		.oobsize   = 64,
		.badblock_pos = BBP_FIRST_PAGE,
		.read      = {
			&read_std(1, INFINITE, 24),
			&read_fast(1, INFINITE, 104),
			&read_dual(1, INFINITE, 104),
			&read_dual_addr(1, INFINITE, 104),
			&read_quad(1, INFINITE, 104),
			&read_quad_addr(1, INFINITE, 104),
			0
		},
		.write     = {
			&write_std(0, 256, 80),
			&write_quad(0, 256, 104),
			0
		},
		.erase     = {
			&erase_sector_128k(0, _128K, 104),
			0
		},
		.driver    = &spi_driver_general,
	},

	/* Etron 3.3V EM73C044VCC-H 1Gbit */
	{
		.name      = "EM73C044VCC-H",
		.id        = {0xd5, 0x22},
		.id_len    = 2,
		.chipsize  = _128M,
		.erasesize = _128K,
		.pagesize  = _2K,
		.oobsize   = 64,
		.badblock_pos = BBP_FIRST_PAGE,
		.read      = {
			&read_std(1, INFINITE, 24),
			&read_fast(1, INFINITE, 120),
			&read_dual(1, INFINITE, 120),
			&read_dual_addr(1, INFINITE, 120),
			&read_quad(1, INFINITE, 120),
			&read_quad_addr(1, INFINITE, 120),
			0
		},
		.write     = {
			&write_std(0, 256, 104),
			&write_quad(0, 256, 120),
			0
		},
		.erase     = {
			&erase_sector_128k(0, _128K, 104),
			0
		},
		.driver    = &spi_driver_general,
	},

	/* Micron MT29F4G01ABBFDWB 4GBit 1.8V */
	{
		.name      = "MT29F4G01ABBFDWB",
		.id        = {0x2C, 0x35},
		.id_len    = 2,
		.chipsize  = _512M,
		.erasesize = _256K,
		.pagesize  = _4K,
		.oobsize   = 256,
		.badblock_pos = BBP_FIRST_PAGE,
		.read      = {
			&read_std(1, INFINITE, 24),
			&read_fast(1, INFINITE, 80),
			&read_dual(1, INFINITE, 80),
			&read_quad(1, INFINITE, 80),
			0
		},
		.write     = {
			&write_std(0, 256, 80),
			&write_quad(0, 256, 80),
			0
		},
		.erase     = {
			&erase_sector_256k(0, _256K, 80),
			0
		},
		.driver    = &spi_driver_no_qe,
	},

	/* Dosilicon 3.3V DS35Q2GA-IB 1Gb */
	{
		.name      = "DS35Q2GA-IB",
		.id        = {0xe5, 0x72},
		.id_len    = 2,
		.chipsize  = _256M,
		.erasesize = _128K,
		.pagesize  = _2K,
		.oobsize   = 64,
		.badblock_pos = BBP_FIRST_PAGE,
		.read      = {
			&read_std(1, INFINITE, 24),
			&read_fast(1, INFINITE, 104),
			&read_dual(1, INFINITE, 104),
			&read_quad(1, INFINITE, 104),
			0
		},
		.write     = {
			&write_std(0, 256, 80),
			&write_quad(0, 256, 104),
			0
		},
		.erase     = {
			&erase_sector_128k(0, _128K, 104),
			0
		},
		.driver    = &spi_driver_general,
	},

	/* FM 3.3v FM25S01-DND-A-G  1Gbit */
	{
		.name      = "FM25S01-DND-A-G",
		.id        = {0xa1, 0xa1},
		.id_len    = 2,
		.chipsize  = _128M,
		.erasesize = _128K,
		.pagesize  = _2K,
		.oobsize   = 128,
		.badblock_pos = BBP_FIRST_PAGE,
		.read      = {
			&read_std(1, INFINITE, 24), /* 24MHz */
			&read_fast(1, INFINITE, 104), /* 104MHz */
			&read_dual(1, INFINITE, 104), /* 104MHz */
			&read_dual_addr(1, INFINITE, 40), /* 40MHz */
			&read_quad(1, INFINITE, 104), /* 104MHz */
			&read_quad_addr(2, INFINITE, 40), /* 40MHz */
			0
		},
		.write     = {
			&write_std(0, 256, 104), /* 104MHz */
			&write_quad(0, 256, 104),  /* 104MHz */
			0
		},
		.erase     = {
			&erase_sector_128k(0, _128K, 104), /* 104MHz */
			0
		},
		.driver    = &spi_driver_no_qe,
	},

	/* FM 3.3v FM25S01A 1Gbit */
	{
		.name      = "FM25S01A",
		.id        = {0xa1, 0xe4},
		.id_len    = 2,
		.chipsize  = _128M,
		.erasesize = _128K,
		.pagesize  = _2K,
		.oobsize   = 64,
		.badblock_pos = BBP_FIRST_PAGE,
		.read      = {
			&read_std(1, INFINITE, 24), /* 104MHz */
			&read_fast(1, INFINITE, 104), /* 104MHz */
			&read_dual(1, INFINITE, 104), /* 104MHz */
			&read_dual_addr(1, INFINITE, 40), /* 40MHz */
			&read_quad(1, INFINITE, 104), /* 104MHz */
			&read_quad_addr(2, INFINITE, 40), /* 40MHz */
			0
		},
		.write     = {
			&write_std(0, 256, 104), /* 104MHz */
			&write_quad(0, 256, 104),  /* 104MHz */
			0
		},
		.erase     = {
			&erase_sector_128k(0, _128K, 104), /* 104MHz */
			0
		},
		.driver    = &spi_driver_no_qe,
	},

	{   .id_len    = 0, },
};


static void fmc100_spi_nand_search_rw(struct spi_nand_info *spiinfo,
					struct spi_op *spiop_rw, u_int iftype, u_int max_dummy, int rw_type)
{
	int ix = 0;
	struct spi_op **spiop, **fitspiop;
	int ret;

	for (fitspiop = spiop = ((rw_type == RW_OP_WRITE) ? spiinfo->write : spiinfo->read);
	     (*spiop) && ix < MAX_SPI_OP; spiop++, ix++) {
		if (((((unsigned int)(*spiop)->iftype) & iftype) != 0)
		    && ((*spiop)->dummy <= max_dummy)
		    && (*fitspiop)->iftype < (*spiop)->iftype)
			fitspiop = spiop;
	}
	ret = memcpy_s(spiop_rw, sizeof(struct spi_op), (*fitspiop),
		sizeof(struct spi_op));
	if (ret)
		printk("%s: memcpy_s failed\n", __func__);
}


static void fmc100_spi_nand_get_erase(const struct spi_nand_info *spiinfo,
					struct spi_op *spiop_erase)
{
	int ix;
	int ret;

	spiop_erase->size = 0;
	for (ix = 0; ix < MAX_SPI_OP; ix++) {
		if (spiinfo->erase[ix] == NULL)
			break;

		if (spiinfo->erasesize == spiinfo->erase[ix]->size) {
			ret = memcpy_s(&spiop_erase[ix], sizeof(struct spi_op),
				spiinfo->erase[ix], sizeof(struct spi_op));
			if (ret)
				printk("%s:memcpy_s failed\n", __func__);
			break;
		}
	}
}


static void fmc100_map_spi_op(struct fmc_spi *spi)
{
	unsigned char ix;
	const int iftype_read[] = {
		SPI_IF_READ_STD,    IF_TYPE_STD,
		SPI_IF_READ_FAST,   IF_TYPE_STD,
		SPI_IF_READ_DUAL,   IF_TYPE_DUAL,
		SPI_IF_READ_DUAL_ADDR,  IF_TYPE_DIO,
		SPI_IF_READ_QUAD,   IF_TYPE_QUAD,
		SPI_IF_READ_QUAD_ADDR,  IF_TYPE_QIO,
		0,          0,
	};
	const int iftype_write[] = {
		SPI_IF_WRITE_STD,   IF_TYPE_STD,
		SPI_IF_WRITE_QUAD,  IF_TYPE_QUAD,
		0,          0,
	};
	const char *if_str[] = {"STD", "DUAL", "DIO", "QUAD", "QIO"};

	fmc_pr(BT_DBG, "\t||*-Start Get SPI operation iftype\n");

	for (ix = 0; iftype_write[ix]; ix += 2) { /* 2 is row1 of iftype_write[] */
		if (spi->write->iftype == iftype_write[ix]) {
			spi->write->iftype = iftype_write[ix + 1];
			break;
		}
	}
	fmc_pr(BT_DBG, "\t|||-Get best write iftype: %s \n",
	       if_str[spi->write->iftype]);

	for (ix = 0; iftype_read[ix]; ix += 2) { /* 2 is row1 of iftype_read[] */
		if (spi->read->iftype == iftype_read[ix]) {
			spi->read->iftype = iftype_read[ix + 1];
			break;
		}
	}
	fmc_pr(BT_DBG, "\t|||-Get best read iftype: %s \n",
	       if_str[spi->read->iftype]);

	spi->erase->iftype = IF_TYPE_STD;
	fmc_pr(BT_DBG, "\t|||-Get best erase iftype: %s \n",
	       if_str[spi->erase->iftype]);

	fmc_pr(BT_DBG, "\t||*-End Get SPI operation iftype \n");
}

static void fmc100_spi_nand_op_cmd_init(struct spi_nand_info *spi_dev,
		struct fmc_spi *spi)
{
	fmc100_spi_nand_search_rw(spi_dev, spi->read,
				    FMC_SPI_NAND_SUPPORT_READ,
				    FMC_SPI_NAND_SUPPORT_MAX_DUMMY, RW_OP_READ);
	fmc_pr(BT_DBG, "\t||-Save spi->read op cmd:%#x\n", spi->read->cmd);

	fmc100_spi_nand_search_rw(spi_dev, spi->write,
				    FMC_SPI_NAND_SUPPORT_WRITE,
				    FMC_SPI_NAND_SUPPORT_MAX_DUMMY, RW_OP_WRITE);
	fmc_pr(BT_DBG, "\t||-Save spi->write op cmd:%#x\n", spi->write->cmd);

	fmc100_spi_nand_get_erase(spi_dev, spi->erase);
	fmc_pr(BT_DBG, "\t||-Save spi->erase op cmd:%#x\n", spi->erase->cmd);

	fmc100_map_spi_op(spi);
}


static int fmc100_spi_nand_dis_wr_protect(struct fmc_spi *spi, u_char *reg)
{
	int ret;

	ret = spi_nand_feature_op(spi, GET_OP, PROTECT_ADDR, reg);
	if (ret)
		return ret;
	fmc_pr(BT_DBG, "\t||-Get protect status[%#x]: %#x\n", PROTECT_ADDR,
	       *reg);
	if (any_bp_enable(*reg)) {
		*reg &= ~ALL_BP_MASK;
		ret = spi_nand_feature_op(spi, SET_OP, PROTECT_ADDR, reg);
		if (ret)
			return ret;
		fmc_pr(BT_DBG, "\t||-Set [%#x]FT %#x\n", PROTECT_ADDR, *reg);

		spi->driver->wait_ready(spi);

		ret = spi_nand_feature_op(spi, GET_OP, PROTECT_ADDR, reg);
		if (ret)
			return ret;
		fmc_pr(BT_DBG, "\t||-Check BP disable result: %#x\n", *reg);
		if (any_bp_enable(*reg))
			db_msg("Error: Write protection disable failed!\n");
	}
	return ret;
}

static int fmc100_spi_nand_dis_chip_inner_ecc(struct fmc_spi *spi, u_char *reg)
{
	int ret;

	ret = spi_nand_feature_op(spi, GET_OP, FEATURE_ADDR, reg);
	if (ret)
		return ret;
	fmc_pr(BT_DBG, "\t||-Get feature status[%#x]: %#x\n", FEATURE_ADDR,
	       *reg);
	if (*reg & FEATURE_ECC_ENABLE) {
		*reg &= ~FEATURE_ECC_ENABLE;
		ret = spi_nand_feature_op(spi, SET_OP, FEATURE_ADDR, reg);
		if (ret)
			return ret;
		fmc_pr(BT_DBG, "\t||-Set [%#x]FT: %#x\n", FEATURE_ADDR, *reg);

		spi->driver->wait_ready(spi);

		ret = spi_nand_feature_op(spi, GET_OP, FEATURE_ADDR, reg);
		if (ret)
			return ret;
		fmc_pr(BT_DBG, "\t||-Check internal ECC disable result: %#x\n",
		       *reg);
		if (*reg & FEATURE_ECC_ENABLE)
			db_msg("Error: Chip internal ECC disable failed!\n");
	}
	return ret;
}

static void fmc100_spi_ids_probe(struct mtd_info *mtd,
				   struct spi_nand_info *spi_dev)
{
	u_char reg;
	int ret;
	struct nand_chip *chip = NULL;
	struct fmc_host *host = NULL;
	struct fmc_spi *spi = NULL;

	if (mtd == NULL || spi_dev == NULL) {
		db_msg("Error: mtd or spi_dev is NULL!\n");
		return;
	}
	chip = mtd_to_nand(mtd);
	if (chip == NULL || chip->priv == NULL) {
		db_msg("Error: chip is NULL!\n");
		return;
	}
	host = chip->priv;
	if (host->spi == NULL) {
		db_msg("Error: host->spi is NULL!\n");
		return;
	}
	spi = host->spi;
	fmc_pr(BT_DBG, "\t|*-Start match SPI operation & chip init\n");

	spi->host = host;
	spi->name = spi_dev->name;
	spi->driver = spi_dev->driver;
	if (!spi->driver) {
		db_msg("Error: host->driver is NULL!\n");
		return;
	}

	fmc100_spi_nand_op_cmd_init(spi_dev, spi);

	if (spi->driver->qe_enable)
		spi->driver->qe_enable(spi);

	/* Disable write protection */
	ret = fmc100_spi_nand_dis_wr_protect(spi, &reg);
	if (ret)
		return;

	/* Disable chip internal ECC */
	ret = fmc100_spi_nand_dis_chip_inner_ecc(spi, &reg);
	if (ret)
		return;

	fmc_cs_user[host->cmd_op.cs]++;

	fmc_pr(BT_DBG, "\t|*-End match SPI operation & chip init\n");
}

static struct nand_flash_dev spi_nand_dev;

static struct nand_flash_dev *spi_nand_get_flash_info(struct mtd_info *mtd,
		unsigned char *id)
{
	unsigned char ix;
	int len;
	char buffer[BUFF_LEN];
	struct nand_chip *chip = mtd_to_nand(mtd);
	struct fmc_host *host = chip->priv;
	struct spi_nand_info *spi_dev = fmc_spi_nand_flash_table;
	struct nand_flash_dev *type = &spi_nand_dev;
	int ret;

	fmc_pr(BT_DBG, "\t*-Start find SPI Nand flash\n");

	len = sprintf_s(buffer, BUFF_LEN, "SPI Nand(cs %d) ID: %#x %#x",
		      host->cmd_op.cs, id[0], id[1]);
	if (len < 0)
		printk("%s, line: %d, sprintf_s failed\n", __func__, __LINE__);

	for (; spi_dev->id_len; spi_dev++) {
		if (memcmp(id, spi_dev->id, spi_dev->id_len))
			continue;

		for (ix = 2; ix < spi_dev->id_len; ix++) { /* star from id[2] */
			if ((spi_dev->id_len <= MAX_SPI_NAND_ID_LEN)) {
				len += sprintf_s(buffer + len, BUFF_LEN - len, " %#x",
					id[ix]);
				if (len < 0)
					printk("%s,line: %d, sprintf_s failed\n",
						__func__, __LINE__);
			}
		}
		pr_info("%s\n", buffer);

		fmc_pr(BT_DBG, "\t||-CS(%d) found SPI Nand: %s\n",
		       host->cmd_op.cs, spi_dev->name);

		type->name = spi_dev->name;
		ret = memcpy_s(type->id, MAX_SPI_NAND_ID_LEN, spi_dev->id,
			spi_dev->id_len);
		if (ret) {
			printk("%s: memcpy_s failed\n", __func__);
			return NULL;
		}
		type->pagesize = spi_dev->pagesize;
		type->chipsize = (unsigned int)(spi_dev->chipsize >>
			20); /* 1M unit shift right 20 bit */
		type->erasesize = spi_dev->erasesize;
		type->id_len = spi_dev->id_len;
		type->oobsize = spi_dev->oobsize;
		fmc_pr(BT_DBG, "\t|-Save struct nand_flash_dev info\n");

		mtd->oobsize = spi_dev->oobsize;
		mtd->erasesize = spi_dev->erasesize;
		mtd->writesize = spi_dev->pagesize;

		chip->base.memorg.pagesize = spi_dev->pagesize;
		chip->base.memorg.pages_per_eraseblock = spi_dev->erasesize / spi_dev->pagesize;
		chip->base.memorg.eraseblocks_per_lun = spi_dev->chipsize / spi_dev->erasesize;
		chip->base.memorg.oobsize = spi_dev->oobsize;

		fmc100_spi_ids_probe(mtd, spi_dev);

		fmc_pr(BT_DBG, "\t*-Found SPI nand: %s\n", spi_dev->name);

		return type;
	}

	fmc_pr(BT_DBG, "\t*-Not found SPI nand flash, %s\n", buffer);

	return NULL;
}


void fmc_spi_nand_ids_register(void)
{
	pr_info("SPI Nand ID Table Version %s\n", SPI_NAND_ID_TAB_VER);
	get_spi_nand_flash_type_hook = spi_nand_get_flash_info;
}

#ifdef CONFIG_PM


static int fmc100_spi_nand_dis_wp(struct fmc_spi *spi, u_char *reg)
{
	int ret;

	ret = spi_nand_feature_op(spi, GET_OP, PROTECT_ADDR, reg);
	if (ret)
		return ret;
	fmc_pr(PM_DBG, "\t|-Get protect status[%#x]: %#x\n", PROTECT_ADDR,
	       *reg);
	if (any_bp_enable(*reg)) {
		*reg &= ~ALL_BP_MASK;
		ret = spi_nand_feature_op(spi, SET_OP, PROTECT_ADDR, reg);
		if (ret)
			return ret;
		fmc_pr(PM_DBG, "\t|-Set [%#x]FT %#x\n", PROTECT_ADDR, *reg);

		spi->driver->wait_ready(spi);

		ret = spi_nand_feature_op(spi, GET_OP, PROTECT_ADDR, reg);
		if (ret)
			return ret;
		fmc_pr(PM_DBG, "\t|-Check BP disable result: %#x\n", *reg);
		if (any_bp_enable(*reg))
			db_msg("Error: Write protection disable failed!\n");
	}
	return ret;
}

void fmc100_spi_nand_config(struct fmc_host *host)
{
	u_char reg;
	int ret;
	struct fmc_spi *spi = NULL;
	static const char *str[] = {"STD", "DUAL", "DIO", "QUAD", "QIO"};

	if ((host == NULL) || (host->spi == NULL)) {
		db_msg("Error: host or host->spi is NULL!\n");
		return;
	}
	spi = host->spi;
	/* judge whether support QUAD read/write or not, set it if yes */
	fmc_pr(PM_DBG, "\t|-SPI read iftype: %s write iftype: %s\n",
	       str[spi->read->iftype], str[spi->write->iftype]);

	if (spi->driver->qe_enable)
		spi->driver->qe_enable(spi);

	/* Disable write protection */
	ret = fmc100_spi_nand_dis_wp(spi, &reg);
	if (ret)
		return;
	/* Disable chip internal ECC */
	ret = spi_nand_feature_op(spi, GET_OP, FEATURE_ADDR, &reg);
	if (ret)
		return;
	fmc_pr(PM_DBG, "\t|-Get feature status[%#x]: %#x\n", FEATURE_ADDR,
	       reg);
	if (reg & FEATURE_ECC_ENABLE) {
		reg &= ~FEATURE_ECC_ENABLE;
		ret = spi_nand_feature_op(spi, SET_OP, FEATURE_ADDR, &reg);
		if (ret)
			return;
		fmc_pr(PM_DBG, "\t|-Set [%#x]FT: %#x\n", FEATURE_ADDR, reg);

		spi->driver->wait_ready(spi);

		ret = spi_nand_feature_op(spi, GET_OP, FEATURE_ADDR, &reg);
		if (ret)
			return;
		fmc_pr(PM_DBG, "\t|-Check internal ECC disable result: %#x\n",
		       reg);
		if (reg & FEATURE_ECC_ENABLE)
			db_msg("Error: Chip internal ECC disable failed!\n");
	}
}

#endif /* CONFIG_PM */
