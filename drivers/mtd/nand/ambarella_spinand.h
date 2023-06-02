/**
 * ambarella_spinand.h
 *
 * History:
 *    2015/10/26 - [Ken He] created file
 *
 * Copyright 2014-2018 Ambarella Inc.  All Rights Reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef __LINUX_AMBARELLA_SPINAND_H__
#define __LINUX_AMBARELLA_SPINAND_H__

#include <linux/init.h>
#include <linux/io.h>
#include <linux/err.h>
#include <linux/errno.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/interrupt.h>
#include <linux/mutex.h>
#include <linux/math64.h>
#include <linux/slab.h>
#include <linux/sched.h>
#include <linux/mod_devicetable.h>

#include <linux/mtd/mtd.h>
#include <linux/mtd/nand.h>
#include <linux/mtd/partitions.h>
#include <linux/of_platform.h>

#include <linux/clk.h>
#include <linux/dma-mapping.h>
#include <linux/dmaengine.h>
#include <plat/dma.h>
#include <plat/rct.h>

#include <plat/ptb.h>
#include <linux/spi/spi.h>
#include <linux/spi/flash.h>
#include <linux/delay.h>

#define REGPREP(reg,mask,shift,value) (reg=((~mask)&reg)|(value << shift))
#define REGDUMP(reg,mask,shift) ((reg&mask)>>shift)

#define REG00_DATALEN_SHIFT   0   /*[21:0]    data access length*/
#define REG00_DATALEN_MASK    0x003fffff
#define REG00_DUMMYLEN_SHIFT  22  /*[26:22]    dummy cycle length*/
#define REG00_DUMMYLEN_MASK   0x07c00000
#define REG00_ADDRLEN_SHIFT   27  /*[29:27]    address length*/
#define REG00_ADDRLEN_MASK    0x38000000
#define REG00_CMDLEN_SHIFT    30  /*[31:30]    command length*/
#define    REG00_CMDLEN_MASK  0xc0000000

#define REG04_READEN_SHIFT   0 /*[0]    data part read mode*/
#define REG04_READEN_MASK    0x00000001
#define REG04_WRITEEN_SHIFT  1 /*[1]    data part write mode*/
#define REG04_WRITEEN_MASK   0x00000002
#define REG04_RXLANE_SHIFT   9 /*[9] rx lane config*/
#define REG04_RXLANE_MASK    0x00000200
#define REG04_DATALANE_SHIFT 10 /*[11:10] number of data lane*/
#define REG04_DATALANE_MASK  0x00000c00
#define REG04_ADDRLANE_SHIFT 12 /*[13:12] number of addr lane*/
#define REG04_ADDRLANE_MASK  0x00003000
#define REG04_CMDLANE_SHIFT  14 /*[15:14] number of command lane */
#define REG04_CMDLANE_MASK   0x0000c000
#define REG04_LSBFRT_SHIFT   24 /*[24]    0-msb first 1-lsb first*/
#define REG04_LSBFRT_MASK    0x01000000
#define REG04_DATADTR_SHIFT  28 /*[28]    data double transfer rate*/
#define REG04_DATADTR_MASK   0x10000000
#define REG04_DUMMYDTR_SHIFT 29 /*[29]    dummy double transfer rate*/
#define REG04_DUMMYDTR_MASK  0x20000000
#define REG04_ADDRDTR_SHIFT  30 /*[30] address double transfer rate*/
#define REG04_ADDRDTR_MASK   0x40000000
#define REG04_CMDDTR_SHIFT   31/*[31] command dtr,only reg_clk divider>2*/
#define REG04_CMDDTR_MASK    0x80000000

#define REG08_RXSPLDELAY_SHIFT   0 /*[4:0]    adjust rx sampling data phase*/
#define REG08_RXSPLDELAY_MASK    0x0000001f
#define REG08_CLKDIV_SHIFT       10 /*[17:10]    divide reference clock*/
#define REG08_CLKDIV_MASK        0x0003fc00
#define REG08_CHIPSEL_SHIFT      18 /*[25:18]    cen for multiple device 0 for select and vice versa*/
#define REG08_CHIPSEL_MASK       0x03fc0000
#define REG08_HOLDSWITCH_SHIFT   26 /*[26]0 for hold switching on switch one ref clock cycle before negative edge*/
#define REG08_HOLDSWITCH_MASK    0x04000000
#define REG08_SPICLKPOL_SHIFT    27 /*[27]    clock will remain 1 or 0 in standby mode*/
#define REG08_SPICLKPOL_MASK     0x08000000
#define REG08_HOLDPIN_SHIFT      28 /*[30:28]    for flow control purpose*/
#define REG08_HOLDPIN_MASK       0x70000000
#define REG08_FLOWCON_SHIFT      31 /*[31]    flow control enable*/
#define REG08_FLOWCON_MASK       0x80000000

#define REG0C_CMD0_SHIFT 0 /*[7:0]    command for SPI  device*/
#define REG0C_CMD0_MASK  0x000000ff
#define REG0C_CMD1_SHIFT 8 /*[15:8]*/
#define REG0C_CMD1_MASK  0x0000ff00
#define REG0C_CMD2_SHIFT 16 /*[23:16]*/
#define REG0C_CMD2_MASK  0x00ff0000

#define REG18_RXDMAEN_SHIFT 0 /*[0]    rx dma enable*/
#define REG18_RXDMAEN_MASK  0x00000001
#define REG18_TXDMAEN_SHIFT 1 /*[1]    tx dma enable*/
#define REG18_TXDMAEN_MASK  0x00000002

#define REG1C_TXFIFOLV_SHIFT  0 /*[8:0]  tx fifo threshold level*/
#define REG1C_TXFIFOLV_MASK   0x000000ff

#define REG20_RXFIFOLV_SHIFT  0
#define REG20_RXFIFOLV_MASK   0x000000ff

#define REG24_TXFIFOLV_SHIFT  0
#define REG24_TXFIFOLV_MASK   0x000000ff

#define REG28_RXFIFOLV_SHIFT 0
#define REG28_RXFIFOLV_MASK  0x000000ff

#define REG2C_TXFIFONOTFULL_SHIFT 1 /*[1]*/
#define REG2C_TXFIFONOTFULL_MASK  0x00000002
#define REG2C_TXFIFOEMP_SHIFT     2 /*[2] tx fifo empty*/
#define REG2C_TXFIFOEMP_MASK      0x00000004
#define REG2C_RXFIFONOTEMP_SHIFT  3 /*[3] */
#define REG2C_RXFIFONOTEMP_MASK   0x00000008
#define REG2C_RXFIFOFULL_SHIFT    4 /*[4] rx fifo full*/
#define REG2C_RXFIFOFULL_MASK     0x00000010

#define REG30_TXEMP_SHIFT     0/*[0] tx fifo almost empty*/
#define REG30_TXEMP_MASK      0x00000001
#define REG30_TXOVER_SHIFT    1 /*[1]*/
#define REG30_TXOVER_MASK     0x00000002
#define REG30_RXUNDER_SHIFT   2/*[2]*/
#define REG30_RXUNDER_MASK    0x00000004
#define REG30_RXOVER_SHIFT    3/*[3]*/
#define REG30_RXOVER_MASK     0x00000008
#define REG30_RXFULL_SHIFT    4 /*[4]*/
#define REG30_RXFULL_MASK     0x00000010
#define REG30_DATAREACH_SHIFT 5 /*[5]*/
#define REG30_DATAREACH_MASK  0x00000020
#define REG30_TXUNDER_SHIFT   6/*[6]*/
#define REG30_TXUNDER_MASK    0x00000040

#define REG38_TXEMPTYINTR_SHIFT     0 /*[0]  tx almost empty*/
#define REG38_TXEMPTYINTR_MASK      0x00000001
#define REG38_TXOVERFLOWINTR_SHIFT  1 /*[1] */
#define REG38_TXOVERFLOWINTR_MASK   0x00000002
#define REG38_RXUNDERFLOWINTR_SHIFT 2 /*[2]*/
#define REG38_RXUNDERFLOWINTR_MASK  0x00000004
#define REG38_RXOVERFLOWINTR_SHIFT  3 /*[3]*/
#define REG38_RXOVERFLOWINTR_MASK   0x00000008
#define REG38_RXFULLINTR_SHIFT      4 /*[4] rx fifo almost full*/
#define REG38_RXFULLINTR_MASK        0x00000010
#define REG38_DATALENREACHINTR_SHIFT 5 /*[5] transaction done interrupt*/
#define REG38_DATALENREACHINTR_MASK  0x00000020
#define REG38_TXUNDERFLOWINTR_SHIFT  6 /*[6] */
#define REG38_TXUNDERFLOWINTR_MASK   0x00000040

#define REG40_TXFIFORESET_SHIFT  0 /*[0] software reset the tx fifo*/
#define REG40_TXFIFORESET_MASK   0x00000001

#define REG44_RXFIFORESET_SHIFT  0 /*[0]  software reset the rx fifo*/
#define REG44_RXFIFORESET_MASK   0x00000001

#define REG50_STRTRX_SHIFT 0 /*[0]    start tx or rx*/
#define REG50_STRTRX_MASK  0x00000001

#define HAS_IMG_PARTS       15
#define TOTAL_FW_PARTS        (HAS_IMG_PARTS + HAS_NO_IMG_PARTS)

#define REG00 0x00
#define REG04 0x04
#define REG08 0x08
#define REG0C 0x0c
#define REG10 0x10
#define REG14 0x14
#define REG18 0x18
#define REG1C 0x1c
#define REG20 0x20
#define REG24 0x24
#define REG28 0x28
#define REG2C 0x2c
#define REG30 0x30
#define REG34 0x34
#define REG38 0x38
#define REG3C 0x3C
#define REG40 0x40
#define REG44 0x44
#define REG50 0x50
#define REG100 0x100
#define REG200 0x200
#define DMA_CONTROLLER_OFFSET (0xe0005000)

#if defined(CONFIG_SPI_NOR_CHIP_0)
#define SPINOR_DEV 0
#elif defined(CONFIG_SPI_NOR_CHIP_1)
#define SPINOR_DEV 1
#elif defined(CONFIG_SPI_NOR_CHIP_2)
#define SPINOR_DEV 2
#elif defined(CONFIG_SPI_NOR_CHIP_3)
#define SPINOR_DEV 3
#else
#define SPINOR_DEV 0
#endif

struct amb_spinand {
    struct device                *dev;
    unsigned char __iomem        *regbase;
    unsigned char __iomem        *dmaregbase;
    dma_addr_t                   dmaaddr;
    u8                           *dmabuf;
    int (*write_enable)(struct amb_spinand *flash);
    int (*wait_till_ready)(struct amb_spinand *flash);
    struct mutex                 lock;
    struct mtd_info              mtd;
    u16                          page_size;
    u16                          addr_width;
    u8                           erase_opcode;
    u8                           *command;
    u8                           dummy;
    u32                          addr;
	u32							 clk;
    bool                         fast_read;
	u32							 jedec_id;
	struct nand_chip			chip;
	void						*priv;
};
//the buffer size must align to 32 and smaller than the max size of DMA
#define AMBA_SPINOR_DMA_BUFF_SIZE    4096

/* SPI NAND Command Set Definitions */
enum {
	SPI_NAND_WRITE_ENABLE			= 0x06,
	SPI_NAND_WRITE_DISABLE			= 0x04,
	SPI_NAND_GET_FEATURE_INS		= 0x0F,
	SPI_NAND_SET_FEATURE			= 0x1F,
	SPI_NAND_PAGE_READ_INS			= 0x13,
	SPI_NAND_READ_CACHE_INS			= 0x03,
	SPI_NAND_FAST_READ_CACHE_INS		= 0x0B,
	SPI_NAND_READ_CACHE_X2_INS		= 0x3B,
	SPI_NAND_READ_CACHE_X4_INS		= 0x6B,
	SPI_NAND_READ_CACHE_DUAL_IO_INS		= 0xBB,
	SPI_NAND_READ_CACHE_QUAD_IO_INS		= 0xEB,
	SPI_NAND_READ_ID			= 0x9F,
	SPI_NAND_PROGRAM_LOAD_INS		= 0x02,
	SPI_NAND_PROGRAM_LOAD4_INS		= 0x32,
	SPI_NAND_PROGRAM_EXEC_INS		= 0x10,
	SPI_NAND_PROGRAM_LOAD_RANDOM_INS	= 0x84,
	SPI_NAND_PROGRAM_LOAD_RANDOM4_INS	= 0xC4,
	SPI_NAND_BLOCK_ERASE_INS		= 0xD8,
	SPI_NAND_RESET				= 0xFF
};

/* Feature registers */
enum feature_register {
	SPI_NAND_PROTECTION_REG_ADDR	= 0xA0,
	SPI_NAND_FEATURE_EN_REG_ADDR	= 0xB0,
	SPI_NAND_STATUS_REG_ADDR	= 0xC0,
	SPI_NAND_DS_REG_ADDR		= 0xD0,
};
/*
 * SR7 - reserved
 * SR6 - ECC status 2
 * SR5 - ECC status 1
 * SR4 - ECC status 0
 *
 * ECCS provides ECC status as follows:
 * 000b = No bit errors were detected during the previous read algorithm.
 * 001b = bit error was detected and corrected, error bit number < 3.
 * 010b = bit error was detected and corrected, error bit number = 4.
 * 011b = bit error was detected and corrected, error bit number = 5.
 * 100b = bit error was detected and corrected, error bit number = 6.
 * 101b = bit error was detected and corrected, error bit number = 7.
 * 110b = bit error was detected and corrected, error bit number = 8.
 * 111b = bit error was detected and not corrected.
 *
 * SR3 - P_Fail Program fail
 * SR2 - E_Fail Erase fail
 * SR1 - WEL - Write enable latch
 * SR0 - OIP - Operation in progress
 */
enum {
	SPI_NAND_ECCS2	= 0x40,
	SPI_NAND_ECCS1	= 0x20,
	SPI_NAND_ECCS0	= 0x10,
	SPI_NAND_PF	= 0x08,
	SPI_NAND_EF	= 0x04,
	SPI_NAND_WEL	= 0x02,
	SPI_NAND_OIP	= 0x01
};

enum {
	SPI_NAND_ECC_NO_ERRORS = 0x00,
	SPI_NAND_ECC_UNABLE_TO_CORRECT =
		SPI_NAND_ECCS0 | SPI_NAND_ECCS1 | SPI_NAND_ECCS2
};

/*
 * Feature enable register description: SPI_NAND_FEATURE_EN_REG_ADDR
 *
 * FR7 - OTP protect
 * FR6 - OTP enable
 * FR5 - reserved
 * FR4 - ECC enable
 * FR3 - reserved
 * FR2 - reserved
 * FR1 - reserved
 * FR0 - Quad operation enable
 */
enum {
	SPI_NAND_QUAD_EN	= 0x01,
	SPI_NAND_ECC_EN		= 0x10,
	SPI_NAND_OTP_EN		= 0x40,
	SPI_NAND_OTP_PRT	= 0x80
};

/*
 * Protection register description: SPI_NAND_PROTECTION_REG_ADDR
 *
 * BL7 - BRWD
 * BL6 - reserved
 * BL5 - Block protect 2
 * BL4 - Block protect 1
 * BL3 - Block protect 0
 * BL2 - INV
 * BL1 - CMP
 * BL0 - reserved
 */
enum {
	SPI_NAND_BRWD	= 0x80,
	SPI_NAND_BP2	= 0x20,
	SPI_NAND_BP1	= 0x10,
	SPI_NAND_BP0	= 0x08,
	SPI_NAND_INV	= 0x04,
	SPI_NAND_CMP	= 0x02
};

enum protected_rows {
	/* All unlocked : 000xx */
	SPI_NAND_PROTECTED_ALL_UNLOCKED	= 0x00,
	SPI_NAND_UPPER_1_64_LOCKED	= 0x04,
	SPI_NAND_LOWER_63_64_LOCKED	= 0x05,
	SPI_NAND_LOWER_1_64_LOCKED	= 0x06,
	SPI_NAND_UPPER_63_64_LOCKED	= 0x07,
	SPI_NAND_UPPER_1_32_LOCKED	= 0x08,
	SPI_NAND_LOWER_31_32_LOCKED	= 0x09,
	SPI_NAND_LOWER_1_32_LOCKED	= 0x0A,
	SPI_NAND_UPPER_31_32_LOCKED	= 0x0B,
	SPI_NAND_UPPER_1_16_LOCKED	= 0x0C,
	SPI_NAND_LOWER_15_16_LOCKED	= 0x0D,
	SPI_NAND_LOWER_1_16_LOCKED	= 0x0E,
	SPI_NAND_UPPER_15_16_LOCKED	= 0x0F,
	SPI_NAND_UPPER_1_8_LOCKED	= 0x10,
	SPI_NAND_LOWER_7_8_LOCKED	= 0x11,
	SPI_NAND_LOWER_1_8_LOCKED	= 0x12,
	SPI_NAND_UPPER_7_8_LOCKED	= 0x13,
	SPI_NAND_UPPER_1_4_LOCKED	= 0x14,
	SPI_NAND_LOWER_3_4_LOCKED	= 0x15,
	SPI_NAND_LOWER_1_4_LOCKED	= 0x16,
	SPI_NAND_UPPER_3_4_LOCKED	= 0x17,
	SPI_NAND_UPPER_1_2_LOCKED	= 0x18,
	SPI_NAND_BLOCK_0_LOCKED		= 0x19,
	SPI_NAND_LOWER_1_2_LOCKED	= 0x1A,
	SPI_NAND_BLOCK_0_LOCKED1	= 0x1B,
	/* All locked (default) : 111xx */
	SPI_NAND_PROTECTED_ALL_LOCKED	= 0x1C,

};

struct spinand_info{
	struct	nand_ecclayout *ecclayout;
	void	*priv;
};

struct spinand_state{
	u32 col; /* offset in page */
	u32 row; /* page number */
	int	buf_ptr;
};

#endif /* __LINUX_AMBARELLA_SPINAND_H__ */
