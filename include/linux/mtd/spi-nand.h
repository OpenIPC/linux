/*-
 *
 * Copyright (c) 2009-2014 Micron Technology, Inc.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * Peter Pan <peterpandong at micron.com>
 *
 * based on mt29f_spinand.h
 */
#ifndef __LINUX_MTD_SPI_NAND_H
#define __LINUX_MTD_SPI_NAND_H

#include <linux/wait.h>
#include <linux/spinlock.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/flashchip.h>


/*
 * Standard SPI-NAND flash commands
 */
#define SPINAND_CMD_READ			0x13
#define SPINAND_CMD_READ_RDM			0x03
#define SPINAND_CMD_PROG_LOAD			0x02
#define SPINAND_CMD_PROG_RDM			0x84
#define SPINAND_CMD_PROG			0x10
#define SPINAND_CMD_ERASE_BLK			0xd8
#define SPINAND_CMD_WR_ENABLE			0x06
#define SPINAND_CMD_WR_DISABLE			0x04
#define SPINAND_CMD_READ_ID			0x9f
#define SPINAND_CMD_RESET			0xff
#define SPINAND_CMD_READ_REG			0x0f
#define SPINAND_CMD_WRITE_REG			0x1f

#define SPINAND_CMD_READ_CACHE_X2		0x3b
#define SPINAND_CMD_READ_CACHE_X4		0x6b
#define SPINAND_CMD_READ_CACHE_DUAL		0xbb
#define SPINAND_CMD_READ_CACHE_QUAD		0xeb

#define SPINAND_CMD_PROG_LOAD_X4		0x32
#define SPINAND_CMD_PROG_RDM_X4			0xC4 /*or 34*/

/* feature registers */
#define REG_BLOCK_LOCK			0xa0
#define REG_OTP				0xb0
#define REG_STATUS			0xc0/* timing */

/* status */
#define STATUS_OIP_MASK			0x01
#define STATUS_READY			(0 << 0)
#define STATUS_BUSY			(1 << 0)

#define STATUS_E_FAIL_MASK		0x04
#define STATUS_E_FAIL			(1 << 2)

#define STATUS_P_FAIL_MASK		0x08
#define STATUS_P_FAIL			(1 << 3)

/*OTP register defines*/
#define OTP_ECC_MASK			0X10
#define OTP_ECC_ENABLE			(1 << 4)
#define OTP_ENABLE			(1 << 6)
#define OTP_LOCK			(1 << 7)

/* block lock */
#define BL_ALL_LOCKED      0x38
#define BL_1_2_LOCKED      0x30
#define BL_1_4_LOCKED      0x28
#define BL_1_8_LOCKED      0x20
#define BL_1_16_LOCKED     0x18
#define BL_1_32_LOCKED     0x10
#define BL_1_64_LOCKED     0x08
#define BL_ALL_UNLOCKED    0

#define SPI_NAND_ECC_SHIFT		4

#define SPI_NAND_MT29F_ECC_MASK		3
#define SPI_NAND_MT29F_ECC_CORRECTED	1
#define SPI_NAND_MT29F_ECC_UNCORR	2
#define SPI_NAND_MT29F_ECC_RESERVED	3
#define SPI_NAND_MT29F_ECC_SHIFT	4

#define SPI_NAND_GD5F_ECC_MASK		7
#define SPI_NAND_GD5F_ECC_UNCORR	7
#define SPI_NAND_GD5F_ECC_SHIFT		4

struct spi_nand_onfi_params {
	/* rev info and features block */
	/* 'O' 'N' 'F' 'I'  */
	u8		sig[4];				/*0-3*/
	__le16		revision;			/*4-5*/
	__le16		features;			/*6-7*/
	__le16		opt_cmd;			/*8-9*/
	u8		reserved0[22];			/*10-31*/

	/* manufacturer information block */
	char		manufacturer[12];		/*32-43*/
	char		model[20];			/*44-63*/
	u8		mfr_id;				/*64*/
	__le16		date_code;			/*65-66*/
	u8		reserved1[13];			/*67-79*/

	/* memory organization block */
	__le32		byte_per_page;			/*80-83*/
	__le16		spare_bytes_per_page;		/*84*85*/
	__le32		data_bytes_per_ppage;		/*86-89*/
	__le16		spare_bytes_per_ppage;		/*90-91*/
	__le32		pages_per_block;		/*92-95*/
	__le32		blocks_per_lun;			/*96-99*/
	u8		lun_count;			/*100*/
	u8		addr_cycles;			/*101*/
	u8		bits_per_cell;			/*102*/
	__le16		bb_per_lun;			/*103-104*/
	__le16		block_endurance;		/*105-106*/
	u8		guaranteed_good_blocks;		/*107*/
	__le16		guaranteed_block_endurance;	/*108-109*/
	u8		programs_per_page;		/*110*/
	u8		ppage_attr;			/*111*/
	u8		ecc_bits;			/*112*/
	u8		interleaved_bits;		/*113*/
	u8		interleaved_ops;		/*114*/
	u8		reserved2[13];			/*115-127*/

	/* electrical parameter block */
	u8		io_pin_capacitance_max;		/*128*/
	__le16		timing_mode;			/*129-130*/
	__le16		program_cache_timing_mode;	/*131-132*/
	__le16		t_prog;				/*133-134*/
	__le16		t_bers;				/*135-136*/
	__le16		t_r;				/*137-138*/
	__le16		t_ccs;				/*139-140*/
	u8		reserved3[23];			/*141-163*/

	/* vendor */
	__le16		vendor_specific_revision;	/*164-165*/
	u8		vendor_specific[88];		/*166-253*/

	__le16		crc;				/*254-255*/
} __packed;

#define ONFI_CRC_BASE	0x4F4E

#define SPINAND_MAX_ID_LEN		4

/**
 * struct spi_nand_chip - SPI-NAND Private Flash Chip Data
 * @chip_lock:		[INTERN] protection lock
 * @name:		name of the chip
 * @wq:			[INTERN] wait queue to sleep on if a SPI-NAND operation
 *			is in progress used instead of the per chip wait queue
 *			when a hw controller is available.
 * @mfr_id:		[BOARDSPECIFIC] manufacture id
 * @dev_id:		[BOARDSPECIFIC] device id
 * @state:		[INTERN] the current state of the SPI-NAND device
 * @spi:		[INTERN] point to spi device structure
 * @mtd:		[INTERN] point to MTD device structure
 * @reset:		[REPLACEABLE] function to reset the device
 * @read_id:		[REPLACEABLE] read manufacture id and device id
 * @load_page:		[REPLACEABLE] load page from NAND to cache
 * @read_cache:		[REPLACEABLE] read data from cache
 * @store_cache:	[REPLACEABLE] write data to cache
 * @write_page:		[REPLACEABLE] program NAND with cache data
 * @erase_block:	[REPLACEABLE] erase a given block
 * @waitfunc:		[REPLACEABLE] wait for ready.
 * @write_enable:	[REPLACEABLE] set write enable latch
 * @get_ecc_status:	[REPLACEABLE] get ecc and bitflip status
 * @enable_ecc:		[REPLACEABLE] enable on-die ecc
 * @disable_ecc:	[REPLACEABLE] disable on-die ecc
 * @buf:		[INTERN] buffer for read/write
 * @oobbuf:		[INTERN] buffer for read/write oob
 * @pagebuf:		[INTERN] holds the pagenumber which is currently in
 *			data_buf.
 * @pagebuf_bitflips:	[INTERN] holds the bitflip count for the page which is
 *			currently in data_buf.
 * @size:		[INTERN] the size of chip
 * @block_size:		[INTERN] the size of eraseblock
 * @page_size:		[INTERN] the size of page
 * @page_spare_size:	[INTERN] the size of page oob size
 * @block_shift:	[INTERN] number of address bits in a eraseblock
 * @page_shift:		[INTERN] number of address bits in a page (column
 *			address bits).
 * @pagemask:		[INTERN] page number mask = number of (pages / chip) - 1
 * @options:		[BOARDSPECIFIC] various chip options. They can partly
 *			be set to inform nand_scan about special functionality.
 * @ecc_strength_ds:	[INTERN] ECC correctability from the datasheet.
 *			Minimum amount of bit errors per @ecc_step_ds guaranteed
 *			to be correctable. If unknown, set to zero.
 * @ecc_step_ds:	[INTERN] ECC step required by the @ecc_strength_ds,
 *                      also from the datasheet. It is the recommended ECC step
 *			size, if known; if unknown, set to zero.
 * @ecc_mask:
 * @ecc_uncorr:
 * @bits_per_cell:	[INTERN] number of bits per cell. i.e., 1 means SLC.
 * @ecclayout:		[BOARDSPECIFIC] ECC layout control structure
 *			See the defines for further explanation.
 * @bbt_options:	[INTERN] bad block specific options. All options used
 *			here must come from bbm.h. By default, these options
 *			will be copied to the appropriate nand_bbt_descr's.
 * @bbt:		[INTERN] bad block table pointer
 * @badblockpos:	[INTERN] position of the bad block marker in the oob
 *			area.
 * @bbt_td:		[REPLACEABLE] bad block table descriptor for flash
 *			lookup.
 * @bbt_md:		[REPLACEABLE] bad block table mirror descriptor
 * @badblock_pattern:	[REPLACEABLE] bad block scan pattern used for initial
 *			bad block scan.
 * @onfi_params:	[INTERN] holds the ONFI page parameter when ONFI is
 *			supported, 0 otherwise.
 */
struct spi_nand_chip {
	spinlock_t	chip_lock;
	char		*name;
	wait_queue_head_t wq;
	u8		dev_id_len;
	u8		dev_id[SPINAND_MAX_ID_LEN];
	flstate_t	state;
	struct spi_device	*spi;
	struct mtd_info	*mtd;

	int (*reset)(struct spi_nand_chip *chip);
	int (*read_id)(struct spi_nand_chip *chip, u8 *id);
	int (*load_page)(struct spi_nand_chip *chip, unsigned int page_addr);
	int (*read_cache)(struct spi_nand_chip *chip, unsigned int page_addr,
		unsigned int page_offset,	size_t length, u8 *read_buf);
	int (*store_cache)(struct spi_nand_chip *chip, unsigned int page_addr,
		unsigned int page_offset,	size_t length, u8 *write_buf);
	int (*write_page)(struct spi_nand_chip *chip, unsigned int page_addr);
	int (*erase_block)(struct spi_nand_chip *chip, u32 page_addr);
	int (*waitfunc)(struct spi_nand_chip *chip, u8 *status);
	int (*write_enable)(struct spi_nand_chip *chip);
	void (*get_ecc_status)(struct spi_nand_chip *chip, unsigned int status,
						unsigned int *corrected,
						unsigned int *ecc_errors);
	int (*enable_ecc)(struct spi_nand_chip *chip);
	int (*disable_ecc)(struct spi_nand_chip *chip);
	int (*block_bad)(struct mtd_info *mtd, loff_t ofs, int getchip);

	u8		*buf;
	u8		*oobbuf;
	int		pagebuf;
	u32		pagebuf_bitflips;
	u64		size;
	u32		block_size;
	u16		page_size;
	u16		page_spare_size;
	u8		block_shift;
	u8		page_shift;
	u16		page_mask;
	u32		options;
	u16		ecc_strength_ds;
	u16		ecc_step_ds;
	u8		ecc_mask;
	u8		ecc_uncorr;
	u8		bits_per_cell;
	struct nand_ecclayout *ecclayout;
	u32		bbt_options;
	u8		*bbt;
	int		badblockpos;
	struct nand_bbt_descr *bbt_td;
	struct nand_bbt_descr *bbt_md;
	struct nand_bbt_descr *badblock_pattern;
	struct spi_nand_onfi_params	 onfi_params;
};


struct spi_nand_id_info{
#define SPI_NAND_ID_NO_DUMMY  (0xff)
	u8 id_addr;
	u8 id_len;
};

struct spi_nand_flash {
	char		*name;
	struct spi_nand_id_info id_info;
	u8		dev_id[SPINAND_MAX_ID_LEN];
	u32		page_size;
	u32		page_spare_size;
	u32		pages_per_blk;
	u32		blks_per_chip;
	u32		options;
	u8		ecc_mask;
	u8		ecc_uncorr;
	struct nand_ecclayout *ecc_layout;
};

struct spi_nand_cmd {
	u8		cmd;
	u32		n_addr;		/* Number of address */
	u8		addr[3];	/* Reg Offset */
	u32		n_tx;		/* Number of tx bytes */
	u8		*tx_buf;	/* Tx buf */
	u8		tx_nbits;
	u32		n_rx;		/* Number of rx bytes */
	u8		*rx_buf;	/* Rx buf */
	u8		rx_nbits;
};

#define SPI_NAND_INFO(nm, mid, did, pagesz, sparesz, pg_per_blk,\
	blk_per_chip, opts)				\
	{ .name = (nm), .mfr_id = (mid), .dev_id = (did),\
	.page_size = (pagesz), .page_spare_size = (sparesz),\
	.pages_per_blk = (pg_per_blk), .blks_per_chip = (blk_per_chip),\
	.options = (opts) }

#define SPINAND_NEED_PLANE_SELECT	(1 << 0)

#define SPINAND_MFR_MICRON		0x2C
#define SPINAND_MFR_GIGADEVICE	0xC8

int spi_nand_send_cmd(struct spi_device *spi, struct spi_nand_cmd *cmd);
int spi_nand_read_from_cache(struct spi_nand_chip *chip,
		u32 page_addr, u32 column, size_t len, u8 *rbuf);
int spi_nand_read_from_cache_snor_protocol(struct spi_nand_chip *chip,
		u32 page_addr, u32 column, size_t len, u8 *rbuf);
int spi_nand_scan_ident(struct mtd_info *mtd);
int spi_nand_scan_tail(struct mtd_info *mtd);
int spi_nand_scan_ident_release(struct mtd_info *mtd);
int spi_nand_scan_tail_release(struct mtd_info *mtd);
int spi_nand_release(struct mtd_info *mtd);
int __spi_nand_erase(struct mtd_info *mtd, struct erase_info *einfo,
		int allowbbt);
int spi_nand_isbad_bbt(struct mtd_info *mtd, loff_t offs, int allowbbt);
int spi_nand_default_bbt(struct mtd_info *mtd);
int spi_nand_markbad_bbt(struct mtd_info *mtd, loff_t offs);
#endif /* __LINUX_MTD_SPI_NAND_H */

