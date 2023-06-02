/*
 * External Flash table
 *
 * Copyright (C) Novatek Inc. 2017
 *
 */
#include <linux/mtd/mtd.h>

/* data structure to maintain flash ids from different vendors */
struct flash_device {
	char *name;
	u8 erase_cmd;
	u32 device_id;
	u32 pagesize;
	unsigned long sectorsize;
	unsigned long size_in_bytes;
	u8 quad_mode;
	u8 read_mode;
};

#define FLASH_ID(n, es, id, psize, ssize, size, qpp, rd_mode)	\
{				\
	.name = n,		\
	.erase_cmd = es,	\
	.device_id = id,	\
	.pagesize = psize,	\
	.sectorsize = ssize,	\
	.size_in_bytes = size,	\
	.quad_mode = qpp,	\
	.read_mode = rd_mode    \
}


/*
 * quad_mode :
 *           1 bit  : 0
 *           4 bits : WR_QPP
 *
 * read_mode :
 *           SPI_NOR_NORMAL_READ
 *           SPI_NOR_DUAL_READ
 *           SPI_NOR_QUAD_READ
 *
 * Example:
 *       Read: 4 bits, Write: 1 bit
 *       FLASH_ID("mx 25l12835f"  , 0xd8, 0x001820C2, 0x100, 0x1000 , 0x1000000, 0, SPI_NOR_QUAD_READ),
 *
 *       Read: 2 bits, Write: 1 bit
 *       FLASH_ID("mac 25l6405"   , 0xd8, 0x001720C2, 0x100, 0x10000, 0x800000, 0, SPI_NOR_DUAL_READ),
 *
 *       Read: 4 bits, Write: 4 bit
 *       FLASH_ID("gd W25Q127C"   , 0xd8, 0x001840C8, 0x100, 0x10000,  0x1000000, WR_QPP, SPI_NOR_QUAD_READ),
*/
#ifdef CONFIG_MTD_EXTERNAL_FLASH_TABLE
static struct flash_device flash_devices[] = {
};
#endif