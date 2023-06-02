/*
 * Copyright (c) 2016 Fullhan, Inc.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/mtd/spi-nand.h>
#include <linux/mtd/bbm.h>

/*static struct spi_nand_flash spi_nand_table[] = {
	SPI_NAND_INFO("MT29F2G01AAAED", 0x2C, 0X22, 2048, 64, 64, 2048,
	SPINAND_NEED_PLANE_SELECT),
	SPI_NAND_INFO("MT29F4G01AAADD", 0x2C, 0X32, 2048, 64, 64, 4096,
	SPINAND_NEED_PLANE_SELECT),
	SPI_NAND_INFO("GD5F 512MiB 1.8V", 0xC8, 0XA4, 4096, 256, 64, 2048,
	0),
	SPI_NAND_INFO("GD5F 512MiB 3.3V", 0xC8, 0XB4, 4096, 256, 64, 2048,
	0),
	SPI_NAND_INFO("GD5F 256MiB 3.3V", 0xC8, 0XB2, 2048, 128, 64, 2048,
	0),
	SPI_NAND_INFO("GD5F 128MiB 3.3V", 0xC8, 0XB1, 2048, 128, 64, 1024,
	0),
	SPI_NAND_INFO("W25N01GV", 0xEF, 0XAA21, 2048, 64, 64, 1024,
	0),
	{.name = NULL},
};*/

/**
*  Default OOB area specification layout
*/
static struct nand_ecclayout ecc_layout_64 = {
	.eccbytes = 32,
	.eccpos = {
		8, 9, 10, 11, 12, 13, 14, 15,
		24, 25, 26, 27, 28, 29, 30, 21,
		40, 41, 42, 43, 44, 45, 46, 47,
		56, 57, 58, 59, 60, 61, 62, 63
	},
	.oobavail = 30,
	.oobfree = {
		{
			.offset = 2,
			.length = 6
		}, {
			.offset = 16,
			.length = 8
		}, {
			.offset = 32,
			.length = 8
		}, {
			.offset = 48,
			.length = 8
		},
	}
};

static struct nand_ecclayout gd5f_ecc_layout_256 = {
	.eccbytes = 128,
	.eccpos = {
		128, 129, 130, 131, 132, 133, 134, 135,
		136, 137, 138, 139, 140, 141, 142, 143,
		144, 145, 146, 147, 148, 149, 150, 151,
		152, 153, 154, 155, 156, 157, 158, 159,
		160, 161, 162, 163, 164, 165, 166, 167,
		168, 169, 170, 171, 172, 173, 174, 175,
		176, 177, 178, 179, 180, 181, 182, 183,
		184, 185, 186, 187, 188, 189, 190, 191,
		192, 193, 194, 195, 196, 197, 198, 199,
		200, 201, 202, 203, 204, 205, 206, 207,
		208, 209, 210, 211, 212, 213, 214, 215,
		216, 217, 218, 219, 220, 221, 222, 223,
		224, 225, 226, 227, 228, 229, 230, 231,
		232, 233, 234, 235, 236, 237, 238, 239,
		240, 241, 242, 243, 244, 245, 246, 247,
		248, 249, 250, 251, 252, 253, 254, 255
	},
	.oobavail = 127,
	.oobfree = { {1, 127} }
};

static struct nand_ecclayout gd5f_ecc_layout_128 = {
	.eccbytes = 64,
	.eccpos = {
		64, 65, 66, 67, 68, 69, 70, 72,
		72, 73, 74, 75, 76, 77, 78, 79,
		80, 81, 82, 83, 84, 85, 86, 87,
		88, 89, 90, 91, 92, 93, 94, 95,
		96, 97, 98, 99, 100, 101, 102, 103,
		104, 105, 106, 107, 108, 109, 110, 111,
		112, 113, 114, 115, 116, 117, 118, 119,
		120, 121, 122, 123, 124, 125, 126, 127,
	},
	.oobavail = 62,
	.oobfree = { {2, 63} }
};

static struct nand_ecclayout pn26_ecc_layout_128 = {
	.eccbytes = 52,
	.eccpos = {
		6, 7, 8, 9, 10, 11, 12, 13, 14,15,16,17,18,
		21,22,23,24,25,26,27,28,29,30,31,32,33,
		36,37,38,39,40,41,42,43,44,45,46,47,48,
		51,52,53,54,55,56,57,58,59,60,61,62,63
	},
	.oobavail = 72,
	.oobfree = {
			{
				.offset = 4,
				.length = 2
			}, {
				.offset = 19,
				.length = 2
			}, {
				.offset = 34,
				.length = 2
			}, {
				.offset = 49,
				.length = 2
			},
			{
				.offset = 64,
				.length = 64
			},
		}
};

static struct nand_ecclayout default_ecc_layout = {
	.eccbytes = 64,
	.oobavail = 28,
	.oobfree = { { 2, 30 } }
};

static struct nand_ecclayout mx35_ecc_layout_64 = {
	.eccbytes = 0,
	.oobavail = 62,
	.oobfree = { {2, 62} }
};

static struct nand_ecclayout mt29f_ecc_layout_128 = {
	.eccbytes = 64,
	.oobavail = 60,
	.oobfree = { {4, 60} }
};
static struct nand_ecclayout tc58_ecc_layout_64 = {
	.eccbytes = 0,
	.oobavail = 60,
	.oobfree = { {4, 60} }
};

static struct nand_ecclayout ds35_ecc_layout_64 = {
	.eccbytes = 16,
	.oobavail = 32,
	.oobfree = { 
	{
		.offset = 8,
		.length = 8
	}, {
		.offset = 24,
		.length = 8
	}, {
		.offset = 40,
		.length = 8
	}, {
		.offset = 56,
		.length = 8
	},
	}
};

static struct nand_ecclayout fs35_ecc_layout_64 = {
	.eccbytes = 0,
	.oobavail = 62,
	.oobfree = { {2, 62} }
};

static struct nand_ecclayout xt_ecc_layout_128 = {
	.eccbytes = 52,
	.eccpos = {
		64, 65, 66, 67, 68, 69, 70, 72,
		72, 73, 74, 75, 76, 77, 78, 79,
		80, 81, 82, 83, 84, 85, 86, 87,
		88, 89, 90, 91, 92, 93, 94, 95,
		96, 97, 98, 99, 100, 101, 102, 103,
		104, 105, 106, 107, 108, 109, 110, 111,
		112, 113, 114, 115,
	},
	.oobavail = 74,
	.oobfree = { {2, 62}, {116, 12} }
};

static struct nand_ecclayout en_ecc_layout_64 = {
	.eccbytes = 52,
	.eccpos = {
		3, 4, 5, 6, 7, 8, 9, 10,
		11, 12, 13, 14, 15, 19, 20, 21,
		22, 23, 24, 25, 26, 27, 28, 29,
		30, 31, 35, 36, 37, 38, 39, 40,
		41, 42, 43, 44, 45, 46, 47, 51,
		52, 53, 54, 55, 56, 57, 58, 59,
		60, 61, 62, 63,
	},
	.oobavail = 10,
	.oobfree = {
		{
			.offset = 2,
			.length = 1
		}, {
			.offset = 16,
			.length = 3
		}, {
			.offset = 32,
			.length = 3
		}, {
			.offset = 48,
			.length = 3
		},
	}
};

static struct spi_nand_flash spi_nand_table[] = {
	{
		.name = "W25N01GV",
		.id_info = {
			.id_addr = 0,
			.id_len = 3,
		},
		.dev_id = {0xEF, 0xAA, 0x21},
		.page_size = 2048,
		.page_spare_size = 64,
		.pages_per_blk = 64,
		.blks_per_chip = 1024,
		.options = 0,
		.ecc_mask = 3,
		.ecc_uncorr = 2,
		.ecc_layout = &ecc_layout_64,
		.qe_addr = 0xa0,
		.qe_flag = 0x0,
		.qe_mask = 1<<1,
		.multi_wire_command_length = 4,
	},
	{
		.name = "MX35LF1GE4AB",
		.id_info = {
			.id_addr = 0,
			.id_len = 2,
		},
		.dev_id = {0xC2, 0x12},
		.page_size = 2048,
		.page_spare_size = 64,
		.pages_per_blk = 64,
		.blks_per_chip = 1024,
		.options = 0,
		.ecc_mask = 3,
		.ecc_uncorr = 2,
		.ecc_layout = &mx35_ecc_layout_64,
		.qe_addr = 0xb0,
		.qe_flag = 0x1,
		.qe_mask = 1<<0,
		.multi_wire_command_length = 4,
	},
	{
		.name = "MX35LF2GE4AB",
		.id_info = {
			.id_addr = 0,
			.id_len = 2,
		},
		.dev_id = {0xC2, 0x22},
		.page_size = 2048,
		.page_spare_size = 64,
		.pages_per_blk = 64,
		.blks_per_chip = 2048,
		.options = SPINAND_NEED_PLANE_SELECT,
		.ecc_mask = 3,
		.ecc_uncorr = 2,
		.ecc_layout = &mx35_ecc_layout_64,
		.qe_addr = 0xb0,
		.qe_flag = 0x1,
		.qe_mask = 1<<0,
		.multi_wire_command_length = 4,
	},
	{
		.name = "GD5F1GQ4UC",
		.id_info = {
			.id_addr = SPI_NAND_ID_NO_DUMMY,
			.id_len = 3,
		},
		.dev_id = {0xC8, 0xB1, 0x48},
		.page_size = 2048,
		.page_spare_size = 128,
		.pages_per_blk = 64,
		.blks_per_chip = 1024,
		.options = FIRST_DUMMY_BYTE,
		.ecc_mask = 7,
		.ecc_uncorr = 7,
		.ecc_layout = &gd5f_ecc_layout_128,
		.qe_addr = 0xb0,
		.qe_flag = 0x1,
		.qe_mask = 1<<0,
		.multi_wire_command_length = 5,
	},
	{
		.name = "GD5F2GQ4UC",
		.id_info = {
			.id_addr = SPI_NAND_ID_NO_DUMMY,
			.id_len = 3,
		},
		.dev_id = {0xC8, 0xB2, 0x48},
		.page_size = 2048,
		.page_spare_size = 128,
		.pages_per_blk = 64,
		.blks_per_chip = 1024,
		.options = FIRST_DUMMY_BYTE,
		.ecc_mask = 7,
		.ecc_uncorr = 7,
		.ecc_layout = &gd5f_ecc_layout_128,
		.qe_addr = 0xb0,
		.qe_flag = 0x1,
		.qe_mask = 1<<0,
		.multi_wire_command_length = 5,
	},
	{
		.name = "GD5F2GQ4UB",
		.id_info = {
			.id_addr = 0,
			.id_len = 2,
		},
		.dev_id = {0xC8, 0xD2},
		.page_size = 2048,
		.page_spare_size = 128,
		.pages_per_blk = 64,
		.blks_per_chip = 2048,
		.options = 0,
		.ecc_mask = 3,
		.ecc_uncorr = 2,
		.ecc_layout = &gd5f_ecc_layout_128,
		.qe_addr = 0xb0,
		.qe_flag = 0x1,
		.qe_mask = 1<<0,
		.multi_wire_command_length = 4,
	},
	{
		.name = "GD5F1GQ4UB",
		.id_info = {
			.id_addr = 0,
			.id_len = 2,
		},
		.dev_id = {0xC8, 0xD1},
		.page_size = 2048,
		.page_spare_size = 128,
		.pages_per_blk = 64,
		.blks_per_chip = 1024,
		.options = 0,
		.ecc_mask = 3,
		.ecc_uncorr = 2,
		.ecc_layout = &gd5f_ecc_layout_128,
		.qe_addr = 0xb0,
		.qe_flag = 0x1,
		.qe_mask = 1<<0,
		.multi_wire_command_length = 4,
	},
	{
		.name = "PN26G01A",
		.id_info = {
			.id_addr = 0x0,
			.id_len = 2,
		},
		.dev_id = {0xA1, 0xE1},
		.page_size = 2048,
		.page_spare_size = 128,
		.pages_per_blk = 64,
		.blks_per_chip = 1024,
		.options = 0,
		.ecc_mask = 3,
		.ecc_uncorr = 2,
		.ecc_layout = &pn26_ecc_layout_128,
		.qe_addr = 0xb0,
		.qe_flag = 0x1,
		.qe_mask = 1<<0,
		.multi_wire_command_length = 4,
		},
	    /*MT29F1G01*/
		{
		.name = "MT29F1G01",
		.id_info = {
			.id_addr = 0,
			.id_len = 2,
		},
		.dev_id = {0x2c, 0x14},
		.page_size = 2048,
		.page_spare_size = 128,
		.pages_per_blk = 64,
		.blks_per_chip = 1024,
		.options = 0,
		.ecc_mask = 3,
		.ecc_uncorr = 2,
		.ecc_layout = &mt29f_ecc_layout_128,
		.qe_addr =  0xa0,
		.qe_mask  =  1<<1,
		.qe_flag =  0,
		.multi_wire_command_length = 4,
		},
		/*TC58CVG0S3H*/
		{
		.name = "TC58CVG0S3H",
		.id_info = {
			.id_addr = 0,
			.id_len = 2,
		},
		.dev_id = {0x98, 0xc2},
		.page_size = 2048,
		.page_spare_size = 64,
		.pages_per_blk = 64,
		.blks_per_chip = 1024,
		.options = 0,
		.ecc_mask = 3,
		.ecc_uncorr = 2,
		.ecc_layout = &tc58_ecc_layout_64,
		.qe_addr =  0xa0,
		.qe_mask  =  1<<1,
		.qe_flag =  0,
		.multi_wire_command_length = 4,
		},
		/*DS35X1GA*/
		{
		.name = "DS35X1GA",
		.id_info = {
			.id_addr = 0,
			.id_len = 2,
		},
		.dev_id = {0xe5, 0x71},
		.page_size = 2048,
		.page_spare_size = 64,
		.pages_per_blk = 64,
		.blks_per_chip = 1024,
		.options = 0,
		.ecc_mask = 3,
		.ecc_uncorr = 2,
		.ecc_layout = &ds35_ecc_layout_64,
		.qe_addr =  0xb0,
		.qe_mask  =  1<<0,
		.qe_flag =  1,
		.multi_wire_command_length = 4,
		},
		{
		.name = "FS35ND01G-S1Y2",
		.id_info = {
		    .id_addr = 0,
		    .id_len = 3,
		},
		.dev_id = {0xcd, 0xea, 0x11},
		.page_size = 2048,
		.page_spare_size = 64,
		.pages_per_blk = 64,
		.blks_per_chip = 1024,
		.options = 0,
		.ecc_mask = 3,
		.ecc_uncorr = 2,
		.ecc_layout = &fs35_ecc_layout_64,
		.qe_addr =  0xb0,
		.qe_mask  =  1<<0,
		.qe_flag =  1,
		.multi_wire_command_length = 4,
		},
		{
		.name = "TC58CVGS3HRAIJ",
		.id_info = {
		    .id_addr = 0,
		    .id_len = 3,
		},
		.dev_id = {0x98, 0xe2, 0x40},
		.page_size = 2048,
		.page_spare_size = 64,
		.pages_per_blk = 64,
		.blks_per_chip = 1024,
		.options = 0,
		.ecc_mask = 3,
		.ecc_uncorr = 2,
		.ecc_layout = &fs35_ecc_layout_64,
		.multi_wire_command_length = 4,
		.bitflip_threshold = 3,
		},
		{
		.name = "FM25S01A",
		.id_info = {
		    .id_addr = 0,
		    .id_len = 2,
		},
		.dev_id = {0xa1, 0xe4},
		.page_size = 2048,
		.page_spare_size = 64,
		.pages_per_blk = 64,
		.blks_per_chip = 1024,
		.options = 0,
		.ecc_mask = 3,
		.ecc_uncorr = 2,
		.ecc_layout = &fs35_ecc_layout_64,
		.multi_wire_command_length = 4,
		},
		{
		.name = "XT26G11C",
		.id_info = {
			.id_addr = 0,
			.id_len = 2,
		},
		.dev_id = {0x0b, 0x15},
		.page_size = 2048,
		.page_spare_size = 128,
		.pages_per_blk = 64,
		.blks_per_chip = 1024,
		.options = 0,
		.ecc_mask = 3,
		.ecc_uncorr = 2,
		.ecc_layout = &xt_ecc_layout_128,
		.qe_addr =  0xb0,
		.qe_mask  =  1<<0,
		.qe_flag =  1,
		.multi_wire_command_length = 4,
		},
		{
		.name = "FS35SQA001G",
		.id_info = {
			.id_addr = 0,
			.id_len = 3,
		},
		.dev_id = {0xcd, 0x71, 0x71},
		.page_size = 2048,
		.page_spare_size = 64,
		.pages_per_blk = 64,
		.blks_per_chip = 1024,
		.options = 0,
		.ecc_mask = 3,
		.ecc_uncorr = 2,
		.ecc_layout = &fs35_ecc_layout_64,
		.qe_addr =  0xb0,
		.qe_mask  =  1<<0,
		.qe_flag =  1,
		.multi_wire_command_length = 4,
		},
		{
		.name = "ENADSW1G",
		.id_info = {
			.id_addr = 0,
			.id_len = 2,
		},
		.dev_id = {0x2c, 0xc1},
		.page_size = 2048,
		.page_spare_size = 64,
		.pages_per_blk = 64,
		.blks_per_chip = 1024,
		.options = 0,
		.ecc_mask = 3,
		.ecc_uncorr = 2,
		.ecc_layout = &en_ecc_layout_64,
		.qe_addr =  0xb0,
		.qe_mask  =  1<<0,
		.qe_flag =  1,
		.multi_wire_command_length = 4,
		.bbt_options = NAND_BBT_NO_OOB,
		},
		{
		.name = "XT26G01C",
		.id_info = {
			.id_addr = 0,
			.id_len = 2,
		},
		.dev_id = {0x0b, 0x11},
		.page_size = 2048,
		.page_spare_size = 128,
		.pages_per_blk = 64,
		.blks_per_chip = 1024,
		.options = 0,
		.ecc_mask = 15,
		.ecc_uncorr = 15,
		.ecc_layout = &xt_ecc_layout_128,
		.qe_addr =  0xb0,
		.qe_mask  =  1<<0,
		.qe_flag =  1,
		.multi_wire_command_length = 4,
		.bitflip_threshold = 4,
		},
};

/**
 * spi_nand_scan_id_table - [INTERN] scan chip info in id table
 * @chip: SPI-NAND device structure
 * @id: point to manufacture id and device id
 */
bool spi_nand_scan_id_table(struct spi_nand_chip *chip, u8 *id)
{
	int i, j = 0;
	struct spi_nand_flash *type = spi_nand_table;
	int m=0;

	for (m=0; m<ARRAY_SIZE(spi_nand_table); m++,type++) {
//		if (id[0] == type->mfr_id && id[1] == type->dev_id) {
		for (j=0, i = 0; j < type->id_info.id_len;j++,i++ ) {
			if (id[i] != type->dev_id[j])
				break;
		}
		if (j == type->id_info.id_len) {
			chip->name = type->name;
			chip->size = type->page_size * type->pages_per_blk
				     * type->blks_per_chip;
			chip->block_size = type->page_size
					   * type->pages_per_blk;
			chip->page_size = type->page_size;
			chip->page_spare_size = type->page_spare_size;
			chip->block_shift = ilog2(chip->block_size);
			chip->page_shift = ilog2(chip->page_size);
			chip->page_mask = chip->page_size - 1;
			chip->options = type->options;
			if (!type->ecc_layout)
				chip->ecclayout = &default_ecc_layout;
			else
				chip->ecclayout = type->ecc_layout;
			chip->dev_id_len = type->id_info.id_len;
			chip->ecc_uncorr = type->ecc_uncorr;
			chip->ecc_mask = type->ecc_mask;
			memcpy(chip->dev_id, type->dev_id, chip->dev_id_len);
			chip->qe_addr = type->qe_addr;
			chip->qe_flag = type->qe_flag;
			chip->qe_mask = type->qe_mask;
			chip->multi_wire_command_length = 
				type->multi_wire_command_length;
			chip->bbt_options = type->bbt_options;
			chip->bitflip_threshold = type->bitflip_threshold;
			return true;
		}
	}
	return false;
}
