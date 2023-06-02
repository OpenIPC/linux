/*
 * arch/arm/plat-ambarella/include/plat/nand.h
 *
 * Author: Cao Rongrong <rrcao@ambarella.com>
 *
 * Copyright (C) 2004-2009, Ambarella, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 */

#ifndef __PLAT_AMBARELLA_NAND_H__
#define __PLAT_AMBARELLA_NAND_H__

/* ==========================================================================*/

#if (CHIP_REV == S2E) || (CHIP_REV == S3L)
#define NAND_ECC_RPT_NUM_SUPPORT	1
#else
#define NAND_ECC_RPT_NUM_SUPPORT	0
#endif

/* ==========================================================================*/
#define FLASH_CTR_OFFSET		0x120
#define FLASH_CMD_OFFSET		0x124
#define FLASH_TIM0_OFFSET		0x128
#define FLASH_TIM1_OFFSET		0x12c
#define FLASH_TIM2_OFFSET		0x130
#define FLASH_TIM3_OFFSET		0x134
#define FLASH_TIM4_OFFSET		0x138
#define FLASH_TIM5_OFFSET		0x13c
#define FLASH_STA_OFFSET		0x140
#define FLASH_ID_OFFSET			0x144
#define FLASH_CFI_OFFSET		0x148
#define FLASH_LEN_OFFSET		0x14c
#define FLASH_INT_OFFSET		0x150
#define FLASH_EX_CTR_OFFSET		0x15c
#define FLASH_EX_ID_OFFSET		0x160
/* followings are for customer command, start from S3L */
#define FLASH_TIM6_OFFSET		0x164
#define FLASH_CC_OFFSET			0x170
#define FLASH_CC_WORD_OFFSET		0x174
#define FLASH_CC_DATA0_OFFSET		0x180
#define FLASH_CC_DATA1_OFFSET		0x184
#define FLASH_CC_DATA2_OFFSET		0x188
#define FLASH_CC_DATA3_OFFSET		0x18c
#define FLASH_CC_DATA4_OFFSET		0x190
#define FLASH_CC_DATA5_OFFSET		0x194
#define FLASH_CC_DATA6_OFFSET		0x198
#define FLASH_CC_DATA7_OFFSET		0x19c

#define NAND_CTR_REG			FIO_REG(0x120)
#define NAND_CMD_REG			FIO_REG(0x124)
#define NAND_TIM0_REG			FIO_REG(0x128)
#define NAND_TIM1_REG			FIO_REG(0x12c)
#define NAND_TIM2_REG			FIO_REG(0x130)
#define NAND_TIM3_REG			FIO_REG(0x134)
#define NAND_TIM4_REG			FIO_REG(0x138)
#define NAND_TIM5_REG			FIO_REG(0x13c)
#define NAND_STA_REG			FIO_REG(0x140)
#define NAND_ID_REG			FIO_REG(0x144)
#define NAND_COPY_ADDR_REG		FIO_REG(0x148)
#define NAND_LEN_REG			FIO_REG(0x14c)
#define NAND_INT_REG			FIO_REG(0x150)
#define NAND_EXT_CTR_REG		FIO_REG(0x15c)
#define NAND_EXT_ID5_REG		FIO_REG(0x160)
/* followings are for customer command, start from S3L */
#define NAND_TIM6_REG			FIO_REG(0x164)
#define NAND_CC_REG			FIO_REG(0x170)
#define NAND_CC_WORD_REG		FIO_REG(0x174)
#define NAND_CC_DATA0_REG		FIO_REG(0x180)
#define NAND_CC_DATA1_REG		FIO_REG(0x184)
#define NAND_CC_DATA2_REG		FIO_REG(0x188)
#define NAND_CC_DATA3_REG		FIO_REG(0x18c)
#define NAND_CC_DATA4_REG		FIO_REG(0x190)
#define NAND_CC_DATA5_REG		FIO_REG(0x194)
#define NAND_CC_DATA6_REG		FIO_REG(0x198)
#define NAND_CC_DATA7_REG		FIO_REG(0x19c)

#define NAND_READ_CMDWORD_REG		FIO_REG(0x154)
#define NAND_PROG_CMDWORD_REG		FIO_REG(0x158)
#define NAND_CMDWORD1(x)		((x) << 16)
#define NAND_CMDWORD2(x)		((x) & 0xff)

#define NAND_CPS1_ADDR_REG		FIO_REG(0x154)
#define NAND_CPD1_ADDR_REG		FIO_REG(0x158)
#define NAND_CPS2_ADDR_REG		FIO_REG(0x15c)
#define NAND_CPD2_ADDR_REG		FIO_REG(0x160)
#define NAND_CPS3_ADDR_REG		FIO_REG(0x164)
#define NAND_CPD3_ADDR_REG		FIO_REG(0x168)
#define NAND_NBR_SPA_REG		FIO_REG(0x16c)

#define NAND_CTR_A(x)			((x) << 28)
#define NAND_CTR_SA			0x08000000
#define NAND_CTR_SE			0x04000000
#define NAND_CTR_C2			0x02000000
#define NAND_CTR_P3			0x01000000
#define NAND_CTR_I4			0x00800000
#define NAND_CTR_RC			0x00400000
#define NAND_CTR_CC			0x00200000
#define NAND_CTR_CE			0x00100000
#define NAND_CTR_EC_MAIN		0x00080000
#define NAND_CTR_EC_SPARE		0x00040000
#define NAND_CTR_EG_MAIN		0x00020000
#define NAND_CTR_EG_SPARE		0x00010000
#define NAND_CTR_WP			0x00000200
#define NAND_CTR_IE			0x00000100
#define NAND_CTR_XS			0x00000080
#define NAND_CTR_SZ_8G			0x00000070
#define NAND_CTR_SZ_4G			0x00000060
#define NAND_CTR_SZ_2G			0x00000050
#define NAND_CTR_SZ_1G			0x00000040
#define NAND_CTR_SZ_512M		0x00000030
#define NAND_CTR_SZ_256M		0x00000020
#define NAND_CTR_SZ_128M		0x00000010
#define NAND_CTR_SZ_64M			0x00000000
#define NAND_CTR_4BANK			0x00000008
#define NAND_CTR_2BANK			0x00000004
#define NAND_CTR_1BANK			0x00000000
#define NAND_CTR_WD_64BIT		0x00000003
#define NAND_CTR_WD_32BIT		0x00000002
#define NAND_CTR_WD_16BIT		0x00000001
#define NAND_CTR_WD_8BIT		0x00000000

#define NAND_CTR_INTLVE			0x80000000
#define NAND_CTR_SPRBURST		0x40000000
#define NAND_CFG_STAT_ENB		0x00002000
#define NAND_CTR_2INTL			0x00001000
#define NAND_CTR_K9			0x00000800

#define NAND_CTR_WAS			0x00000400

#define NAND_AMB_CMD_ADDR(x)		((x) << 4)
#define NAND_AMB_CMD_NOP		0x0
#define NAND_AMB_CMD_DMA		0x1
#define NAND_AMB_CMD_RESET		0x2
#define NAND_AMB_CMD_NOP2		0x3
#define NAND_AMB_CMD_NOP3		0x4
#define NAND_AMB_CMD_NOP4		0x5
#define NAND_AMB_CMD_NOP5		0x6
#define NAND_AMB_CMD_COPYBACK		0x7
#define NAND_AMB_CMD_NOP6		0x8
#define NAND_AMB_CMD_ERASE		0x9
#define NAND_AMB_CMD_READID		0xa
#define NAND_AMB_CMD_NOP7		0xb
#define NAND_AMB_CMD_READSTATUS		0xc
#define NAND_AMB_CMD_NOP8		0xd
#define NAND_AMB_CMD_READ		0xe
#define NAND_AMB_CMD_PROGRAM		0xf

/* NAND_TIM0_REG (NAND mode) */
#define NAND_TIM0_TCLS(x)		((x) << 24)
#define NAND_TIM0_TALS(x)		((x) << 16)
#define NAND_TIM0_TCS(x)		((x) << 8)
#define NAND_TIM0_TDS(x)		(x)

/* NAND_TIM1_REG (NAND mode) */
#define NAND_TIM1_TCLH(x)		((x) << 24)
#define NAND_TIM1_TALH(x)		((x) << 16)
#define NAND_TIM1_TCH(x)		((x) << 8)
#define NAND_TIM1_TDH(x)		(x)

/* NAND_TIM2_REG (NAND mode) */
#define NAND_TIM2_TWP(x)		((x) << 24)
#define NAND_TIM2_TWH(x)		((x) << 16)
#define NAND_TIM2_TWB(x)		((x) << 8)
#define NAND_TIM2_TRR(x)		(x)

/* NAND_TIM3_REG (NAND mode) */
#define NAND_TIM3_TRP(x)		((x) << 24)
#define NAND_TIM3_TREH(x)		((x) << 16)
#define NAND_TIM3_TRB(x)		((x) << 8)
#define NAND_TIM3_TCEH(x)		(x)

/* NAND_TIM4_REG (NAND mode) */
#define NAND_TIM4_TRDELAY(x)		((x) << 24)
#define NAND_TIM4_TCLR(x)		((x) << 16)
#define NAND_TIM4_TWHR(x)		((x) << 8)
#define NAND_TIM4_TIR(x)		(x)

/* NAND_TIM5_REG (NAND mode) */
#define NAND_TIM5_TWW(x)		((x) << 16)
#define NAND_TIM5_TRHZ(x)		((x) << 8)
#define NAND_TIM5_TAR(x)		(x)

/* NAND_INT_REG (NAND mode) */
#define NAND_INT_DI			0x1

/* NAND_EXT_CTR_REG */
#define NAND_EXT_CTR_I5			0x00800000
#define NAND_EXT_CTR_SP_2X		0x00000001

/* ==========================================================================*/
#define EC_MDSD		0	/* check main disable and spare disable */
#define EC_MDSE		1	/* check main disable and spare enable */
#define EC_MESD		2	/* check main enable and spare disable */
#define EC_MESE		3	/* check main enable and spare enable */

#define EG_MDSD		0	/* generate main disable and spare disable */
#define EG_MDSE		1	/* generate main disable and spare enable */
#define EG_MESD		2	/* generate main enable and spare disable */
#define EG_MESE		3	/* generate main enable and spare enable */

/**
 * Define for xdhost and nand_host
 */
#define MAIN_ONLY		0
#define SPARE_ONLY		1
#define MAIN_ECC		2
#define SPARE_ECC		3
#define SPARE_ONLY_BURST	4
#define SPARE_ECC_BURST		5

/* ECC use bch-x bytes */
#define NAND_ECC_BCH6_BYTES	40	/* 10Bytes/512Bytes, so 40B/2048B */
#define NAND_ECC_BCH8_BYTES	52	/* 13Bytes/512Bytes, so 52B/2048B */


#define FIO_DMA_CHAN		0
#define FDMA_CTR_OFFSET		(0x300 + ((FIO_DMA_CHAN) << 4))
#define FDMA_SRC_OFFSET		(0x304 + ((FIO_DMA_CHAN) << 4))
#define FDMA_DST_OFFSET		(0x308 + ((FIO_DMA_CHAN) << 4))
#define FDMA_STA_OFFSET		(0x30c + ((FIO_DMA_CHAN) << 4))
#define FDMA_DA_OFFSET		(0x380 + ((FIO_DMA_CHAN) << 2))
#define FDMA_INT_OFFSET		(0x3f0)

#define FDMA_SPR_CNT_OFFSET	(0x200 + ((FIO_DMA_CHAN) << 4))
#define FDMA_SPR_SRC_OFFSET	(0x204 + ((FIO_DMA_CHAN) << 4))
#define FDMA_SPR_DST_OFFSET	(0x208 + ((FIO_DMA_CHAN) << 4))
#define FDMA_SPR_STA_OFFSET	(0x20c + ((FIO_DMA_CHAN) << 4))
#define FDMA_SPR_DA_OFFSET	(0x280 + ((FIO_DMA_CHAN) << 2))
#define FDMA_DSM_CTR_OFFSET	(0x3a0 + ((FIO_DMA_CHAN) << 2))

#endif

