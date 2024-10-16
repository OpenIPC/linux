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
#include <linux/mtd/mtd.h>

/*
 * Manufacturer IDs
 *
 * The first byte returned from the flash after sending opcode SPINOR_OP_RDID.
 * Sometimes these are the same as CFI IDs, but sometimes they aren't.
 */
#define SNOR_MFR_ATMEL		CFI_MFR_ATMEL
#define SNOR_MFR_GIGADEVICE	0xc8
#define SNOR_MFR_INTEL		CFI_MFR_INTEL
#define SNOR_MFR_ST		CFI_MFR_ST /* ST Micro <--> Micron */
#define SNOR_MFR_MICRON		CFI_MFR_MICRON /* ST Micro <--> Micron */
#define SNOR_MFR_MACRONIX	CFI_MFR_MACRONIX
#define SNOR_MFR_SPANSION	CFI_MFR_AMD
#define SNOR_MFR_SST		CFI_MFR_SST
#define SNOR_MFR_ESMT		CFI_MFR_EON
#define SNOR_MFR_GFX		CFI_MFR_EON
#define SNOR_MFR_WINBOND	0xef /* Also used by some Spansion */
#define SNOR_MFR_ADESTO		0x1f /* Also used by some Spansion0 */
#define SNOR_MFR_XMC		0x20
#define SNOR_MFR_XTX		0x0b
#define SNOR_MFR_PUYA		0x85
#define SNOR_MFR_ZETTA		0xba
#define SNOR_MFR_BOYA		0x68
#define SNOR_MFR_EON		CFI_MFR_EON
#define SNOR_MFR_FM             0xa1
#define SNOR_MFR_MXIC	 0xc2

/*
 * GFX Device IDs
 *
 * Manufacturer ID is equal to EON,so define Device id to distinguish.
 */
#define GM_64A_ID  	0x4017
#define GM_128A_ID	 0x4018

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
#define SPINOR_OP_WRSR		0x01	/* Write status register 1 byte */
#define SPINOR_OP_RDSR2		0x35	/* Read status register 2 */
#define SPINOR_OP_WRSR2		0x31	/* Write status register 2 */
#define SPINOR_OP_RDSR3		0x15	/* Read status register 3 */
#define SPINOR_OP_WRSR3		0x11	/* Write status register 3 */
#define SPINOR_OP_READ		0x03	/* Read data bytes (low frequency) */
#define SPINOR_OP_READ_FAST	0x0b	/* Read data bytes (high frequency) */
#define SPINOR_OP_READ_1_1_2	0x3b	/* Read data bytes (Dual Output SPI) */
#define SPINOR_OP_READ_1_2_2	0xbb	/* Read data bytes (Dual I/O SPI) */
#define SPINOR_OP_READ_1_1_4	0x6b	/* Read data bytes (Quad Output SPI) */
#define SPINOR_OP_READ_1_4_4	0xeb	/* Read data bytes (Quad I/O SPI) */
#define SPINOR_OP_READ_1_1_8	0x8b	/* Read data bytes (Octal Output SPI) */
#define SPINOR_OP_READ_1_8_8	0xcb	/* Read data bytes (Octal I/O SPI) */
#define SPINOR_OP_PP		0x02	/* Page program (up to 256 bytes) */
#define SPINOR_OP_PP_1_1_4	0x32	/* Quad page program */
#define SPINOR_OP_PP_1_4_4	0x38	/* Quad page program */
#define SPINOR_OP_PP_1_1_8	0x82	/* Octal page program */
#define SPINOR_OP_PP_1_8_8	0xc2	/* Octal page program */
#define SPINOR_OP_BE_4K		0x20	/* Erase 4KiB block */
#define SPINOR_OP_BE_4K_PMC	0xd7	/* Erase 4KiB block on PMC chips */
#define SPINOR_OP_BE_32K	0x52	/* Erase 32KiB block */
#define SPINOR_OP_CHIP_ERASE	0xc7	/* Erase whole flash chip */
#define SPINOR_OP_SE		0xd8	/* Sector erase (usually 64KiB) */
#define SPINOR_OP_RDID		0x9f	/* Read JEDEC ID */
#define SPINOR_OP_RDCR		0x35	/* Read configuration register */
#define SPINOR_OP_RDFSR		0x70	/* Read flag status register */
#define SPINOR_OP_DIESEL	0xc2	/* Software Die Select */
#define SPINOR_OP_RESTEN	0x66	/* Reset enable */
#define SPINOR_OP_RESET  	0x99	/* Reset device */

/* 4-byte address opcodes - used on Spansion and some Macronix flashes. */
#define SPINOR_OP_READ4		0x13	/* Read data bytes (low frequency) */
#define SPINOR_OP_READ4_FAST	0x0c	/* Read data bytes (high frequency) */
#define SPINOR_OP_READ4_1_1_2	0x3c	/* Read data bytes (Dual SPI) */
#define SPINOR_OP_READ4_1_2_2	0xbc	/* Read data bytes (Dual I/O SPI) */
#define SPINOR_OP_READ4_1_1_4	0x6c	/* Read data bytes (Quad SPI) */
#define SPINOR_OP_READ4_1_4_4	0xec	/* Read data bytes (Quad I/O SPI) */
#define SPINOR_OP_PP_4B			0x12	/* Page program (up to 256 bytes) */
#define SPINOR_OP_PP_1_1_4_4B	0x34	/* Quad page program */
#define SPINOR_OP_PP_1_4_4_4B	0x3e	/* Quad page program */
#define SPINOR_OP_BE_4K_4B		0x21	/* Erase 4KiB block */
#define SPINOR_OP_BE_32K_4B		0x5c	/* Erase 32KiB block */
#define SPINOR_OP_SE_4B		0xdc	/* Sector erase (usually 64KiB) */

/* Double Transfer Rate opcodes - defined in JEDEC JESD216B. */
#define SPINOR_OP_READ_1_1_1_DTR	0x0d
#define SPINOR_OP_READ_1_2_2_DTR	0xbd
#define SPINOR_OP_READ_1_4_4_DTR	0xed

#define SPINOR_OP_READ_1_1_1_DTR_4B	0x0e
#define SPINOR_OP_READ_1_2_2_DTR_4B	0xbe
#define SPINOR_OP_READ_1_4_4_DTR_4B	0xee

/* Used for SST flashes only. */
#define SPINOR_OP_BP		0x02	/* Byte program */
#define SPINOR_OP_WRDI		0x04	/* Write disable */
#define SPINOR_OP_AAI_WP	0xad	/* Auto address increment word program */

/* Used for Macronix and Winbond flashes. */
#define SPINOR_OP_EN4B		0xb7	/* Enter 4-byte mode */
#define SPINOR_OP_EX4B		0xe9	/* Exit 4-byte mode */

/* Used for Spansion flashes only. */
#define SPINOR_OP_BRWR		0x17	/* Bank register write */

/* Used for Micron flashes only. */
#define SPINOR_OP_RD_EVCR      0x65    /* Read EVCR register */
#define SPINOR_OP_WD_EVCR      0x61    /* Write EVCR register */

/* Used for individual lock*/
#define SPINOR_OP_IBLK          0x36    /* Lock individual block */
#define SPINOR_OP_UIBLK         0x39    /* Unlock individual block */
#define SPINOR_OP_RDBLK         0x3d    /* Read block lock */
#define SPINOR_OP_GBLK          0x7e    /* Lock global block */
#define SPINOR_OP_UGBLK         0x98    /* Unlock global block */
#define SR_WPS_EN_WINBOND	BIT(2)
#define SR_WPS_EN_FM		BIT(3)
#define SR_WPS_EN_XTX		BIT(4)

/* Used for MXIC individual lock*/
#define SPINOR_OP_MXICWBLK	 0xe1    /* Lock MXIC DPB individual block */
#define SPINOR_OP_MXICRBLK	 0xe0    /* Unlock MXIC DPB individual block */

/* Used for MXIC nor flash*/
#define SPINOR_OP_USPB	 0xe4    /* Unused SPB lock */
#define SPINOR_OP_RDSCUR	 0x2b    /* read security regisetr */
#define SR_WPSEL	 BIT(7)    /* WPSEL bit */

/* Used for secutity rigister */
#define SR_OP_PROGRAM		 0x42    /* common sr write op */
#define SR_OP_ERASE		 0x44    /* common sr erase op */
#define SR_OP_READ		 0x48    /* common sr read op */
#define LB_OFS		 2      /* LB bit offset */


/* use for EON flashes*/
#define SPINOR_OP_WREN_VSR      0x50    /* Write enable for Volatile Status Register */
#define SPINOR_OP_RDCR_EON	0x09	/* Read EON configuration register */
#define SPINOR_OP_EXIT_OTP	0x04	/* exit otp mode*/
#define SPINOR_OP_ENTER_OTP	0x3a	/* enter otp mode*/
#define SR_OTP_WXDIS_EN_EON	BIT(6)  /* status register WXDIS*/

/* Status Register bits. */
#define SR_WIP			BIT(0)	/* Write in progress */
#define SR_WEL			BIT(1)	/* Write enable latch */
/* meaning of other SR_* bits may differ between vendors */
#define SR_BP0			BIT(2)	/* Block protect 0 */
#define SR_BP1			BIT(3)	/* Block protect 1 */
#define SR_BP2			BIT(4)	/* Block protect 2 */
#define SR_BP3			BIT(5)	/* Block protect 3 */
#define SR_BP4			BIT(6)	/* Block protect 4 */
#define SR_TB			BIT(5)	/* Top/Bottom protect */
#define SR_SRWD			BIT(7)	/* SR write protect */

#define SR_QUAD_EN_MX		BIT(6)	/* Macronix Quad I/O */
#define CR_TB_MX		BIT(3)  /* Macronix Top/Bottom protect */
#define SCUR_WPSEL_MX           BIT(7)  /* Macronix WPSEL bit */
#define OTP_SR_TB_EON		BIT(3)	/* Eon Top/Bottom protect */
#define SR2_CMP_GD		BIT(6)  /* Gigadevice CMP bit */

/* Enhanced Volatile Configuration Register bits */
#define EVCR_QUAD_EN_MICRON	BIT(7)	/* Micron Quad I/O */

/* Flag Status Register bits */
#define FSR_READY		BIT(7)
#define CR_QUAD_EN_GD		BIT(1)  /* Gd Quad I/O */
#define CR_QUAD_EN_EON		BIT(1)  /* Gd Quad I/O */

/* Configuration Register bits. */
#define CR_QUAD_EN_SPAN		BIT(1)	/* Spansion Quad I/O */

/*For Spinor Debugfs*/
struct spinordbg_data {
	char param[32];
	char value[32];
	char status[512];
};

enum read_mode {
	SPI_NOR_NORMAL = 0,
	SPI_NOR_FAST,
	SPI_NOR_DUAL,
	SPI_NOR_QUAD,
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
	SNOR_F_NO_OP_CHIP_ERASE	= BIT(2),
	SNOR_F_READY_XSR_RDY	= BIT(3),
	SNOR_F_USE_CLSR		= BIT(4),
	SNOR_F_BROKEN_RESET	= BIT(5),
	SNOR_F_4B_OPCODES	= BIT(6),
	SNOR_F_HAS_4BAIT	= BIT(7),
	SNOR_F_HAS_LOCK		= BIT(8),
	SNOR_F_INDIVIDUAL_LOCK	= BIT(9),
	SNOR_F_HAS_LOCK_HANDLE	= BIT(10),
};

/**
 * struct flash_info - Forward declaration of a structure used internally by
 *		       spi_nor_scan()
 */
struct flash_info;

/**
 * struct spi_nor - Structure for defining a the SPI NOR layer
 * @mtd:		point to a mtd_info structure
 * @lock:		the lock for the read/write/erase/lock/unlock operations
 * @dev:		point to a spi device, or a spi nor controller device.
 * @page_size:		the page size of the SPI NOR
 * @addr_width:		number of address bytes
 * @erase_opcode:	the opcode for erasing a sector
 * @read_opcode:	the read opcode
 * @read_dummy:		the dummy needed by the read operation
 * @program_opcode:	the program opcode
 * @flash_read:		the mode of the read
 * @sst_write_second:	used by the SST write operation
 * @flags:		flag options for the current SPI-NOR (SNOR_F_*)
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
	const struct flash_info	*info;
	u32			page_size;
	u8			addr_width;
	u32			n_banks;
	u8			erase_opcode;
	u8			read_opcode;
	u8			read_dummy;
	u8			program_opcode;
	enum read_mode		flash_read;
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
	int (*flash_select_bank)(struct spi_nor *nor, loff_t ofs, u32 ids);

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
 * @mode:	the read mode supported by the driver
 *
 * The drivers can use this fuction to scan the SPI NOR.
 * In the scanning, it will try to get all the necessary information to
 * fill the mtd_info{} and the spi_nor{}.
 *
 * The chip type name can be provided through the @name parameter.
 *
 * Return: 0 for success, others for failure.
 */
int spi_nor_scan(struct spi_nor *nor, const char *name, enum read_mode mode);
struct spi_nor *get_spinor(void);

/**
 * spi_nor_restore_addr_mode() - restore the status of SPI NOR
 * @nor:	the spi_nor structure
 */

struct nor_protection {
    unsigned int boundary; /* protected addr [0, boundary) */
    int bp:8;
    int flag:24;
#define SET_TB BIT(0)
#define SET_CMP BIT(1)
};

void spi_nor_restore(struct spi_nor *nor);
int spinor_debug_init(void);

#ifdef CONFIG_SPI_FLASH_SR
/* Support the operation of safety register */
int security_regiser_read_data(struct mtd_info *mtd,
					loff_t addr, loff_t len, u_char *buf);
int security_regiser_write_data(struct mtd_info *mtd,
					loff_t addr, loff_t len, u_char *buf);
int security_regiser_is_locked(struct mtd_info *mtd, u8 sr_num);
int security_register_lock(struct mtd_info *mtd, u8 sr_num);
#endif

#endif
