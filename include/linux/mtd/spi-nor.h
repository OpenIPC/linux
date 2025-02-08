/*
 * Copyright (C) 2014 Freescale Semiconductor, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef __LINUX_MTD_SPI_NOR_H
#define __LINUX_MTD_SPI_NOR_H

#include <linux/bitops.h>
#include <linux/mtd/cfi.h>

/*
 * Manufacturer IDs
 *
 * The first byte returned from the flash after sending opcode SPINOR_OP_RDID.
 * Sometimes these are the same as CFI IDs, but sometimes they aren't.
 */
#define SNOR_MFR_ATMEL		CFI_MFR_ATMEL
#define SNOR_MFR_INTEL		CFI_MFR_INTEL
#define SNOR_MFR_MICRON		CFI_MFR_ST /* ST Micro <--> Micron */
#define SNOR_MFR_MACRONIX	CFI_MFR_MACRONIX
#define SNOR_MFR_SPANSION	CFI_MFR_AMD
#define SNOR_MFR_SST		CFI_MFR_SST
#define SNOR_MFR_EON		CFI_MFR_EON
#define SNOR_MFR_WINBOND	0xef
#define SNOR_MFR_ESMT		0x8c
#define SNOR_MFR_GD			0xc8
#define SNOR_MFR_XTX        0x0b
#define SNOR_MFR_PUYA       0x85
#define SNOR_MFR_ISSI		0x9d

/* Flash set the RESET# from */
#define SPI_NOR_SR_RST_MASK	BIT(7)
#define SPI_NOR_GET_RST(val)	(((val) & SPI_NOR_SR_RST_MASK) >> 7)
#define SPI_NOR_SET_RST(val)    ((val) | SPI_NOR_SR_RST_MASK)

/* Flash block protect */
#ifdef CONFIG_XMEDIA_SPI_BLOCK_PROTECT
#define _2M				(0x200000UL)
#define _4M				(0x400000UL)
#define _8M				(0x800000UL)
#define _16M				(0x1000000UL)
#define _32M				(0x2000000UL)

#define BP_NUM_3                        3
#define BP_NUM_4                        4

#define DEBUG_SPI_NOR_BP	0

#define SPI_NOR_SR_SRWD_SHIFT	7
#define SPI_NOR_SR_SRWD_MASK	(1 << SPI_NOR_SR_SRWD_SHIFT)

#define SPI_NOR_SR_BP0_SHIFT    2
#define SPI_NOR_SR_BP_WIDTH_4   0xf
#define SPI_NOR_SR_BP_MASK_4    (SPI_NOR_SR_BP_WIDTH_4 << SPI_NOR_SR_BP0_SHIFT)

#define SPI_NOR_SR_BP_WIDTH_3   0x7
#define SPI_NOR_SR_BP_MASK_3    (SPI_NOR_SR_BP_WIDTH_3 << SPI_NOR_SR_BP0_SHIFT)

#define SPI_NOR_SR_TB_SHIFT	3
#define SPI_NOR_SR_TB_MASK	(1 << SPI_NOR_SR_TB_SHIFT)

#define LOCK_LEVEL_MAX(bp_num)	(((0x01) << bp_num) - 1)

#endif /* CONFIG_SPI_BLOCK_PROTECT */

/*
 * Note on opcode nomenclature: some opcodes have a format like
 * SPINOR_OP_FUNCTION{4,}_x_y_z. The numbers x, y, and z stand for the number
 * of I/O lines used for the opcode, address, and data (respectively). The
 * FUNCTION has an optional suffix of '4', to represent an opcode which
 * requires a 4-byte (32-bit) address.
 */

/* Flash opcodes. */
#define SPINOR_OP_WREN		0x06	/* Write enable */
#define SPINOR_OP_RDSR		0x05	/* Read status register */
#define SPINOR_OP_RDSR2		0x35    /* Read Status Register-2 */
#define SPINOR_OP_RDSR3		0x15    /* Read Status Register-3 */
#define SPINOR_OP_WRSR		0x01	/* Write status register 1 byte */
#define SPINOR_OP_WRSR2		0x31    /* Write Status Register-2 1 byte*/
#define SPINOR_OP_WRSR3		0x11    /* Write Status Register-3 1 byte*/
#define SPINOR_OP_READ		0x03	/* Read data bytes (low frequency) */
#define SPINOR_OP_READ_FAST	0x0b	/* Read data bytes (high frequency) */
#define SPINOR_OP_READ_1_1_2	0x3b	/* Read data bytes (Dual Output SPI) */
#define SPINOR_OP_READ_1_2_2	0xbb	/* Read data bytes (Dual I/O SPI) */
#define SPINOR_OP_READ_1_1_4	0x6b	/* Read data bytes (Quad Output SPI) */
#define SPINOR_OP_READ_1_4_4	0xeb	/* Read data bytes (Quad I/O SPI) */
#define SPINOR_OP_PP		0x02	/* Page program (up to 256 bytes) */
#define SPINOR_OP_PP_1_1_4	0x32	/* Quad page program */
#define SPINOR_OP_PP_1_4_4	0x38	/* Quad page program */
#define SPINOR_OP_BE_4K		0x20	/* Erase 4KiB block */
#define SPINOR_OP_BE_4K_PMC	0xd7	/* Erase 4KiB block on PMC chips */
#define SPINOR_OP_BE_32K	0x52	/* Erase 32KiB block */
#define SPINOR_OP_CHIP_ERASE	0xc7	/* Erase whole flash chip */
#define SPINOR_OP_SE		0xd8	/* Sector erase (usually 64KiB) */
#define SPINOR_OP_RDID		0x9f	/* Read JEDEC ID */
#define SPINOR_OP_RDSFDP	0x5a	/* Read SFDP */
#define SPINOR_OP_RDCR		0x35	/* Read configuration register */
#define SPINOR_OP_RDFSR		0x70	/* Read flag status register */

/* 4-byte address opcodes - used on Spansion and some Macronix flashes. */
#define SPINOR_OP_READ_4B	0x13	/* Read data bytes (low frequency) */
#define SPINOR_OP_READ_FAST_4B	0x0c	/* Read data bytes (high frequency) */
#define SPINOR_OP_READ_1_1_2_4B	0x3c	/* Read data bytes (Dual Output SPI) */
#define SPINOR_OP_READ_1_2_2_4B	0xbc	/* Read data bytes (Dual I/O SPI) */
#define SPINOR_OP_READ_1_1_4_4B	0x6c	/* Read data bytes (Quad Output SPI) */
#define SPINOR_OP_READ_1_4_4_4B	0xec	/* Read data bytes (Quad I/O SPI) */
#define SPINOR_OP_PP_4B		0x12	/* Page program (up to 256 bytes) */
#define SPINOR_OP_PP_1_1_4_4B	0x34	/* Quad page program */
#define SPINOR_OP_PP_1_4_4_4B	0x3e	/* Quad page program */
#define SPINOR_OP_BE_4K_4B	0x21	/* Erase 4KiB block */
#define SPINOR_OP_BE_32K_4B	0x5c	/* Erase 32KiB block */
#define SPINOR_OP_SE_4B		0xdc	/* Sector erase (usually 64KiB) */

/* Used for SST flashes only. */
#define SPINOR_OP_BP		0x02	/* Byte program */
#define SPINOR_OP_WRDI		0x04	/* Write disable */
#define SPINOR_OP_AAI_WP	0xad	/* Auto address increment word program */

/* Used for Macronix and Winbond flashes. */
#define SPINOR_OP_EN4B		0xb7	/* Enter 4-byte mode */
#define SPINOR_OP_EX4B		0xe9	/* Exit 4-byte mode */

/* Used for Spansion flashes only. */
#define SPINOR_OP_BRRD		0x16	/* Bank register write */
#define SPINOR_OP_BRWR		0x17	/* Bank register write */

/* Used for GigaDevice flashes only. */
#define SPINOR_OP_WRCR		0x31	/* Config register write */

/* Used for Micron flashes only. */
#define SPINOR_OP_RD_EVCR      0x65    /* Read EVCR register */
#define SPINOR_OP_WD_EVCR      0x61    /* Write EVCR register */

/* Software reset code */
#define SPINOR_ENABLE_RESET	0x66	/* Enable reset */
#define SPINOR_OP_RESET		0x99	/* Reset */

/* Status Register bits. */
#define SR_WIP			BIT(0)	/* Write in progress */
#define SR_WEL			BIT(1)	/* Write enable latch */
/* meaning of other SR_* bits may differ between vendors */
#define SR_BP0			BIT(2)	/* Block protect 0 */
#define SR_BP1			BIT(3)	/* Block protect 1 */
#define SR_BP2			BIT(4)	/* Block protect 2 */
#define SR_TB			BIT(5)	/* Top/Bottom protect */
#define SR_SRWD			BIT(7)	/* SR write protect */

#define SR_QUAD_EN_MX		BIT(6)	/* Macronix Quad I/O */
#define CR_DUMMY_CYCLE          (0x03 << 6) /* Macronix dummy cycle bits */
/* Enhanced Volatile Configuration Register bits */
#define EVCR_DUAL_EN_MICRON	BIT(6)	/* Micron Dual I/O */
#define EVCR_QUAD_EN_MICRON	BIT(7)	/* Micron Quad I/O */

/* Flag Status Register bits */
#define FSR_READY		BIT(7)

/* Configuration Register bits. */
#define CR_QUAD_EN_SPAN		BIT(1)	/* Spansion Quad I/O */
#define CR_QUAD_EN_NM		BIT(2)	/* Nor-mem Quad I/O */
#define QUAD_EN_ISSI		BIT(6)
/* Status Register bits. */
#define SR_QUAD_EN_XTX      BIT(1)

/* Supported modes */
enum spi_nor_mode_index {
	/* Sorted by ascending priority order */
	SNOR_MIDX_SLOW = 0,
	SNOR_MIDX_1_1_1,
	SNOR_MIDX_1_1_2,
	SNOR_MIDX_1_2_2,
	SNOR_MIDX_1_1_4,
	SNOR_MIDX_1_4_4,

	SNOR_MIDX_MAX
};

#define SNOR_MODE_SLOW		BIT(SNOR_MIDX_SLOW)
#define SNOR_MODE_1_1_1		BIT(SNOR_MIDX_1_1_1)
#define SNOR_MODE_1_1_2		BIT(SNOR_MIDX_1_1_2)
#define SNOR_MODE_1_2_2		BIT(SNOR_MIDX_1_2_2)
#define SNOR_MODE_1_1_4		BIT(SNOR_MIDX_1_1_4)
#define SNOR_MODE_1_4_4		BIT(SNOR_MIDX_1_4_4)

struct spi_nor_modes {
	u32	rd_modes;	/* supported SPI modes for (Fast) Read */
	u32	wr_modes;	/* supported SPI modes for Page Program */
};

struct spi_nor_read_op {
	u8	num_mode_clocks;
	u8	num_wait_states;
	u8	opcode;
};

#define SNOR_OP_READ(_num_mode_clocks, _num_wait_states, _opcode) \
	{							  \
		.num_mode_clocks = _num_mode_clocks,		  \
		.num_wait_states = _num_wait_states,		  \
		.opcode = _opcode,				  \
	}

struct spi_nor_erase_type {
	u8	size;	/* specifies 'N' so erase size = 2^N */
	u8	opcode;
};

#define SNOR_OP_ERASE(_size, _opcode) { .size = _size, .opcode = _opcode }
#define SNOR_OP_ERASE_64K(_opcode) SNOR_OP_ERASE(0x10, _opcode)
#define SNOR_OP_ERASE_32K(_opcode) SNOR_OP_ERASE(0x0f, _opcode)
#define SNOR_OP_ERASE_4K(_opcode)  SNOR_OP_ERASE(0x0c, _opcode)

struct spi_nor;

#define SNOR_MAX_ERASE_TYPES	4

struct spi_nor_basic_flash_parameter {
	/* Fast Read settings */
	u32				rd_modes;
	struct spi_nor_read_op		reads[SNOR_MIDX_MAX];

	/* Page Program settings */
	u32				wr_modes;
	u8				page_programs[SNOR_MIDX_MAX];

	/* Sector Erase settings */
	struct spi_nor_erase_type	erase_types[SNOR_MAX_ERASE_TYPES];

	int (*enable_quad_io)(struct spi_nor *nor);
};

#define SNOR_PROTO_CODE_OFF	8
#define SNOR_PROTO_CODE_MASK	GENMASK(11, 8)
#define SNOR_PROTO_CODE_TO_PROTO(code) \
	(((code) << SNOR_PROTO_CODE_OFF) & SNOR_PROTO_CODE_MASK)
#define SNOR_PROTO_CODE_FROM_PROTO(proto) \
	((((u32)(proto)) & SNOR_PROTO_CODE_MASK) >> SNOR_PROTO_CODE_OFF)

#define SNOR_PROTO_ADDR_OFF	4
#define SNOR_PROTO_ADDR_MASK	GENMASK(7, 4)
#define SNOR_PROTO_ADDR_TO_PROTO(addr) \
	(((addr) << SNOR_PROTO_ADDR_OFF) & SNOR_PROTO_ADDR_MASK)
#define SNOR_PROTO_ADDR_FROM_PROTO(proto) \
	((((u32)(proto)) & SNOR_PROTO_ADDR_MASK) >> SNOR_PROTO_ADDR_OFF)

#define SNOR_PROTO_DATA_OFF	0
#define SNOR_PROTO_DATA_MASK	GENMASK(3, 0)
#define SNOR_PROTO_DATA_TO_PROTO(data) \
	(((data) << SNOR_PROTO_DATA_OFF) & SNOR_PROTO_DATA_MASK)
#define SNOR_PROTO_DATA_FROM_PROTO(proto) \
	((((u32)(proto)) & SNOR_PROTO_DATA_MASK) >> SNOR_PROTO_DATA_OFF)

#define SNOR_PROTO(code, addr, data)	  \
	(SNOR_PROTO_CODE_TO_PROTO(code) |   \
	 SNOR_PROTO_ADDR_TO_PROTO(addr) | \
	 SNOR_PROTO_DATA_TO_PROTO(data))

enum spi_nor_protocol {
	SNOR_PROTO_1_1_1 = SNOR_PROTO(1, 1, 1),	/* SPI */
	SNOR_PROTO_1_1_2 = SNOR_PROTO(1, 1, 2),	/* Dual Output */
	SNOR_PROTO_1_2_2 = SNOR_PROTO(1, 2, 2),	/* Dual IO */
	SNOR_PROTO_1_1_4 = SNOR_PROTO(1, 1, 4),	/* Quad Output */
	SNOR_PROTO_1_4_4 = SNOR_PROTO(1, 4, 4),	/* Quad IO */
};

#define SPI_NOR_MAX_CMD_SIZE	8
enum spi_nor_ops {
	SPI_NOR_OPS_READ = 0,
	SPI_NOR_OPS_WRITE,
	SPI_NOR_OPS_ERASE,
	SPI_NOR_OPS_LOCK,
	SPI_NOR_OPS_UNLOCK,
};

enum spi_nor_option_flags {
	SNOR_F_USE_FSR		= BIT(0),
	SNOR_F_HAS_SR_TB	= BIT(1),
};

struct mtd_info;

/**
 * struct spi_nor - Structure for defining a the SPI NOR layer
 * @mtd:		point to a mtd_info structure
 * @lock:		the lock for the read/write/erase/lock/unlock operations
 * @dev:		point to a spi device, or a spi nor controller device.
 * @flash_node:		point to a device node describing this flash instance.
 * @page_size:		the page size of the SPI NOR
 * @addr_width:		number of address bytes
 * @erase_opcode:	the opcode for erasing a sector
 * @read_opcode:	the read opcode
 * @read_dummy:		the dummy needed by the read operation
 * @program_opcode:	the program opcode
 * @sst_write_second:	used by the SST write operation
 * @flags:		flag options for the current SPI-NOR (SNOR_F_*)
 * @erase_proto:	the SPI protocol used by erase operations
 * @read_proto:		the SPI protocol used by read operations
 * @write_proto:	the SPI protocol used by write operations
 * @reg_proto		the SPI protocol used by read_reg/write_reg operations
 * @cmd_buf:		used by the write_reg
 * @prepare:		[OPTIONAL] do some preparations for the
 *			read/write/erase/lock/unlock operations
 * @unprepare:		[OPTIONAL] do some post work after the
 *			read/write/erase/lock/unlock operations
 * @read_reg:		[DRIVER-SPECIFIC] read out the register
 * @write_reg:		[DRIVER-SPECIFIC] write data to the register
 * @read:		[DRIVER-SPECIFIC] read data from the SPI NOR
 * @write:		[DRIVER-SPECIFIC] write data to the SPI NOR
 * @erase:		[DRIVER-SPECIFIC] erase a sector of the SPI NOR
 *			at the offset @offs; if not provided by the driver,
 *			spi-nor will send the erase opcode via write_reg()
 * @flash_lock:		[FLASH-SPECIFIC] lock a region of the SPI NOR
 * @flash_unlock:	[FLASH-SPECIFIC] unlock a region of the SPI NOR
 * @flash_is_locked:	[FLASH-SPECIFIC] check if a region of the SPI NOR is
 *			completely locked
 * @priv:		the private data
 */
struct spi_nor {
	struct mtd_info		mtd;
	struct mutex		lock;
	struct device		*dev;
	struct device_node	*flash_node;
	u32			page_size;
	u8			addr_width;
	u8			erase_opcode;
	u8			read_opcode;
	u8			read_dummy;
	u8			program_opcode;
	enum spi_nor_protocol	erase_proto;
	enum spi_nor_protocol	read_proto;
	enum spi_nor_protocol	write_proto;
	bool			sst_write_second;
	u32			flags;
	u8			cmd_buf[SPI_NOR_MAX_CMD_SIZE];

	int (*prepare)(struct spi_nor *nor, enum spi_nor_ops ops);
	void (*unprepare)(struct spi_nor *nor, enum spi_nor_ops ops);
	int (*read_reg)(struct spi_nor *nor, u8 opcode, u8 *buf, int len);
	int (*write_reg)(struct spi_nor *nor, u8 opcode, u8 *buf, int len);

	ssize_t (*read)(struct spi_nor *nor, loff_t from,
			size_t len, u_char *read_buf);
	ssize_t (*write)(struct spi_nor *nor, loff_t to,
			size_t len, const u_char *write_buf);
	int (*erase)(struct spi_nor *nor, loff_t offs);

	int (*flash_lock)(struct spi_nor *nor, loff_t ofs, uint64_t len);
	int (*flash_unlock)(struct spi_nor *nor, loff_t ofs, uint64_t len);
	int (*flash_is_locked)(struct spi_nor *nor, loff_t ofs, uint64_t len);

#ifdef CONFIG_XMEDIA_SPI_BLOCK_PROTECT
	unsigned int end_addr;
	unsigned int lock_level_max;
	unsigned char level;
#endif
	u32 clkrate;
	void *priv;
};

static inline void spi_nor_set_flash_node(struct spi_nor *nor,
					  struct device_node *np)
{
	mtd_set_of_node(&nor->mtd, np);
}

static inline struct device_node *spi_nor_get_flash_node(struct spi_nor *nor)
{
	return mtd_get_of_node(&nor->mtd);
}

/**
 * spi_nor_scan() - scan the SPI NOR
 * @nor:	the spi_nor structure
 * @name:	the chip type name
 * @modes:	the SPI modes supported by the controller driver
 *
 * The drivers can use this fuction to scan the SPI NOR.
 * In the scanning, it will try to get all the necessary information to
 * fill the mtd_info{} and the spi_nor{}.
 *
 * The chip type name can be provided through the @name parameter.
 *
 * Return: 0 for success, others for failure.
 */
int spi_nor_scan(struct spi_nor *nor, const char *name,
		 struct spi_nor_modes *modes);
void spi_nor_driver_shutdown(struct spi_nor *nor);
#ifdef CONFIG_PM
int spi_nor_suspend(struct spi_nor *nor, pm_message_t state);
int spi_nor_resume(struct spi_nor *nor);
#endif

#endif
