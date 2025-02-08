/*
 * Copyright (c) XMEDIA. All rights reserved.
 */

#ifndef FLASH_H
#define FLASH_H

#include <linux/types.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/nand.h>
#include <linux/mtd/rawnand.h>

/* SPI-NAND opcodes */
#define SPINAND_OP_RESET       0xff /* Device Reset */
#define SPINAND_OP_RDID        0x9f /* Read ID */
#define SPINAND_OP_RDSR        0x0f /* Read status register */
#define SPINAND_OP_WRSR        0x1f /* Write status register */
#define SPINAND_OP_WREN        0x06 /* Write enable */
#define SPINAND_OP_WRDI        0x04 /* Write disable */
#define SPINAND_OP_BE          0xd8 /* Block erase */
#define SPINAND_OP_PLD         0x02 /* Program Data Load, 1 bit per clock. */
#define SPINAND_OP_PLD_4       0x32 /* Program Data Load, 4 bit per clock. */
#define SPINAND_OP_RPLD        0x84 /* Random Program Data Load, 1 bit per clock. */
#define SPINAND_OP_RPLD_4      0x34 /* Random Program Data Load, 4 bit per clock. */
#define SPINAND_OP_PE          0x10 /* Program Execute */
#define SPINAND_OP_PGRD        0x13 /* Page Read */

#define SPINAND_OP_READ        0x03 /* Read Data From Nand Cache, 1 bit per clock. */
#define SPINAND_OP_READ_2      0x3b /* Read Data From Nand Cache, 2 bit per clock. */
#define SPINAND_OP_READ_4      0x6b /* Read Data From Nand Cache, 4 bit per clock. */

enum spinand_read_mode {
	SPINAND_NORMAL = 0,
	SPINAND_FAST,
	SPINAND_DUAL,
	SPINAND_QUAD,
};

#define SPINAND_STATUS_OIP       0x01
#define SPINAND_STATUS_E_FAIL    0x04
#define SPINAND_STATUS_P_FAIL    0x08

/* feature register */
#define SPINAND_STATUS_BUF       0x08
#define SPINAND_STATUS_ECC_EN    0x10
#define SPINAND_STATUS_EQ        0x01

/* protection register */
#define SPINAND_BLOCK_PROT_GIGA         0x38
#define SPINAND_BLOCK_PROT_WINBOND      0x78

#define SPINOR_OP_WREN         0x06 /* Write enable */
#define SPINOR_OP_WRDI         0x04 /* Write disable */

#define SPINOR_OP_READ         0x03 /* Read data bytes (low frequency) */
#define SPINOR_OP_READ_4B      0x13 /* Read data bytes (low frequency) */
#define SPINOR_OP_READ_FAST    0x0b /* Read data bytes (high frequency) */
#define SPINOR_OP_READ_FAST_4B 0x0C /* Read data bytes (high frequency) */
#define SPINOR_OP_READ_DUAL    0x3b /* Read data bytes (Dual SPI) */
#define SPINOR_OP_READ_DUAL_4B 0x3C /* Read data bytes (Dual SPI) with 4 byte address */
#define SPINOR_OP_READ_QUAD    0x6b /* Read data bytes (Quad SPI) */
#define SPINOR_OP_READ_QUAD_4B 0x6C /* Read data bytes (Quad SPI) with 4 byte address  */
#define SPINOR_OP_PP           0x02 /* Page program (up to 256 bytes) */
#define SPINOR_OP_PP_4B        0x12 /* Page Programming with 4 byte address */
#define SPINOR_OP_PP_QUAD      0x32 /* Quad Page program (up to 256 bytes) */
#define SPINOR_OP_PP_QUAD_4B   0x3e /* Quad Page program (up to 256 bytes) with 4 byte address */

#define SPINOR_OP_RDID         0x9f /* Read JEDEC ID */

#define SPINOR_OP_EN4B         0xb7 /* Enter 4-byte mode */
#define SPINOR_OP_EX4B         0xe9 /* Exit 4-byte mode */

#define SPINOR_OP_SE           0xD8 /* 64KB Sector Erase */
#define SPINOR_OP_SE_4K        0x20 /* 4KB sector Erase */
#define SPINOR_OP_SE_4K_4B     0x21 /* 4KB sector Erase with 4 byte address */
#define SPINOR_OP_SE_32K       0x52 /* 32KB sector Erase */
#define SPINOR_OP_SE_32K_4B    0x5c /* 32KB sector Erase with 4 byte address */
#define SPINOR_OP_SE_64K       0xD8 /* 64KB sector Erase */
#define SPINOR_OP_SE_64K_4B    0xDC /* 64KB sector Erase with 4 byte address */
#define SPINOR_OP_SE_128K      0xD8 /* 128KB sector Erase */
#define SPINOR_OP_SE_256K      0xD8 /* 256KB sector Erase */
#define SPINOR_OP_BE           0xC7 /* chip erase */

#define SPINOR_OP_RDSR         0x05 /* Read status register */
#define SPINOR_OP_RDSR2        0x35 /* Read status register-2 */
#define SPINOR_OP_WRSR         0x01 /* Write status register*/

#define SPINOR_STATUS_WIP      0x01 /* write operation in process */
#define SPINOR_STATUS_EQ       0x40 /* Quad Enable MXIC */

#define SPINOR_STATUS2_EQ      0x02 /* Quad Enable, SPANSION */

/* add by xmedia */
#define NAND_RR_NONE                   0x0
#define NAND_RR_HYNIX_BG_BDIE          0x1
#define NAND_RR_HYNIX_BG_CDIE          0x2
#define NAND_RR_HYNIX_CG_ADIE          0x3
#define NAND_RR_MICRON                 0x4
#define NAND_RR_SAMSUNG                0x5
#define NAND_RR_TOSHIBA_V2012          0x6
#define NAND_RR_TOSHIBA_V2013          0x7

#define _400KHz          (400000)
#define _12MHz           (12000000)
#define _12dot5MHz       (12500000)
#define _24MHz           (24000000)
#define _25MHz           (25000000)
#define _15MHz           (15000000)
#define _37dot5MHz       (37500000)
#define _50MHz           (50000000)
#define _62dot5MHz       (62500000)
#define _75MHz           (75000000)
#define _86dot4MHz       (86400000)
#define _100MHz         (100000000)
#define _125MHz         (125000000)
#define _150MHz         (150000000)
#define _166dot5MHz     (166500000)
#define _168dot75MHz    (168750000)
#define _187MHz         (187000000)
#define _196MHz         (196000000)
#define _200MHz         (200000000)
#define _250MHz         (250000000)
#define _300MHz         (300000000)
#define _337dot5MHz     (377500000)
#define _400MHz         (400000000)

struct spiflash_xfer {
	u8  cmd;
	u8  dummy;
	u32 size;
	int clock;
};

#define SPIFLASH_XFER(_cmd, _dummy, _size, _clock) \
	((void *)&(struct spiflash_xfer) {       \
		.cmd = (_cmd),                   \
		.dummy = (_dummy),               \
		.size = (_size),                 \
		.clock = (_clock),               \
})

#define FLASH_ID_LEN        8

struct spinor_chip_xfer {
	const char *name;
	u8 id[FLASH_ID_LEN];
	int id_len;
	struct spiflash_xfer *read;
	struct spiflash_xfer *write;
	struct spiflash_xfer *erase;
};

struct spinand_chip_xfer {
	const char *name;
	u8 id[FLASH_ID_LEN];
	int id_len;
	int datapin;
	int clock;
};

struct nand_chip_clock {
	const char *name;
	u8 id[FLASH_ID_LEN];
	int id_len;
	u32 value;
};

struct cmdfunc_data {
	int column;
	int page_addr;
	u32 command;
	int offset;
};

struct nand_ctrl_info {
	int pagesize;
	struct {
		int strength; /* max correct bit per section, unit 1K */
		int step;     /* ecc section size, default 1K */
	} ecc;
	struct {
		int sz_data; /* number of data bytes per section */
		int sz_ecc;  /* number of ecc bytes per section */
	} section;
	int nr_ecc_sect; /* number of ecc section. */
	int oobsize;
	int oob_offset;  /* offset when only read oob area */
	struct mtd_ooblayout_ops *ooblayout;
	int threshold;   /* the ecc strength threshold */
	void (*dump)(void *host, u8 ecc[], int *max_bitsflip);
};

struct mtd_func {
	int (*_erase) (struct mtd_info *mtd, struct erase_info *instr);
	int (*_read) (struct mtd_info *mtd, loff_t from, size_t len,
		size_t *retlen, u_char *buf);
	int (*_write) (struct mtd_info *mtd, loff_t to, size_t len,
		size_t *retlen, const u_char *buf);
	int (*_panic_write) (struct mtd_info *mtd, loff_t to, size_t len,
		size_t *retlen, const u_char *buf);
	int (*_read_oob) (struct mtd_info *mtd, loff_t from,
	struct mtd_oob_ops *ops);
	int (*_write_oob) (struct mtd_info *mtd, loff_t to,
	struct mtd_oob_ops *ops);
	int (*_block_isbad) (struct mtd_info *mtd, loff_t ofs);
	int (*_block_markbad) (struct mtd_info *mtd, loff_t ofs);
};

struct flash_regop_info {
	void *priv;
	u16 cmd;
	int nr_cmd;
	u32 addrl;
	u32 addrh;
	int addr_cycle;
	u8 *buf;
	unsigned int sz_buf;
	int dummy;
	bool wait_ready; /* only for nand, not for spi-nand */
};

struct flash_regop {
	void *priv;
	int (*write_reg)(struct flash_regop_info *info);
	int (*read_reg)(struct flash_regop_info *info);
	int (*read_sr)(struct flash_regop *regop);
	int (*wait_device_ready)(struct flash_regop *regop);
};

struct flash_regop_intf {
	char name[8];
	int sz_buf;
	struct flash_regop *regop;

	void *args;
	int (*open)(void *args);
	int (*release)(void *args);

	void *priv;
};

int regop_intf_create(struct flash_regop_intf *intf);

void regop_intf_destory(struct flash_regop_intf *intf);

struct spinand_driver {
	u8 id[FLASH_ID_LEN];
	int id_len;

	/* read spi-nand feature or status */
	int (*get_feature)(struct flash_regop *regop, int addr);

	/* set spi-nand feature or status */
	int (*set_feature)(struct flash_regop *regop, int addr, u8 value);

	/*
	 * disable spi-nand chip internal ecc
	 * set some special register for some special nand chip.
	 */
	void (*chip_prepare)(struct flash_regop *regop);
};

struct spinor_driver {
	/* spi-nor quad pin transmit enable. */
	int (*quad_enable)(struct flash_regop *regop);

	/* spi-nor quad pin transmit disable */
	int (*quad_disable)(struct flash_regop *regop);

	/* set spi-nor write enable */
	int (*write_enable)(struct flash_regop *regop);

	/* spi-nor entry 4 address mode, default is 3 address mode */
	int (*entry_4addr)(struct flash_regop *regop);

	/* spi-nor exist 4 address mode, default is 3 address mode */
	int (*exit_4addr)(struct flash_regop *regop);
};

struct spinor_flash_dev {
	const char *name;
	u8 id[FLASH_ID_LEN];
	int id_len;
	int chipsize;
	int erasesize;
};

struct nand_read_retry_info {
	struct flash_regop *drv;
	u8 *buf;     /* for hynix otp data */
	int nr_buf;
};

struct nand_read_retry {
	int type;
	int count;
	int (*setup)(struct nand_read_retry_info *info, int retry_mode);
	int (*entry)(struct nand_read_retry_info *info);
	int (*exit)(struct nand_read_retry_info *info);
};

u32 nand_get_clock(struct nand_chip_clock *clks, u8 *id, int id_len);

struct spinand_driver *spinand_get_drvier(u8 *id, int id_len);

struct nand_read_retry *nand_get_read_retry(int read_retry_type);

struct nand_ctrl_info *nand_get_ctrl_info(struct nand_ctrl_info *info,
	int pagesize, int ecc_strength);

struct spinor_flash_dev *spinor_match_id(u8 *id, int id_len);

struct spinor_driver *spinor_get_driver(u8 *id, int id_len);

struct spinor_chip_xfer *spinor_get_xfer(struct spinor_chip_xfer *xfer,
					 u8 *id, int id_len);

struct spinand_chip_xfer *spinand_get_xfer(struct spinand_chip_xfer *xfer,
					u8 *id, int id_len);

extern struct nand_flash_dev nand_flash_ids_xm[];
extern struct nand_flash_dev spinand_flash_ids_xm[];

int flash_regop_info_dump(struct flash_regop_info *info, char *buf, int sz_buf);

#endif /* XMEDIA_FLASH_H */
