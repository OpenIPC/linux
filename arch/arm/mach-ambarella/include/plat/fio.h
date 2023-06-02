/*
 * arch/arm/plat-ambarella/include/plat/fio.h
 *
 * Author: Anthony Ginger <hfjiang@ambarella.com>
 *
 * Copyright (C) 2004-2010, Ambarella, Inc.
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

#ifndef __PLAT_AMBARELLA_COMMON_FIO_H__
#define __PLAT_AMBARELLA_COMMON_FIO_H__

/* ==========================================================================*/
#if (CHIP_REV == A5S)
#define	FIO_USE_2X_FREQ			1
#else
#define	FIO_USE_2X_FREQ			0
#endif

#if (CHIP_REV == A5S)
#define	NAND_READ_ID5			0
#else
#define	NAND_READ_ID5			1
#endif

#if (CHIP_REV == A5S) || (CHIP_REV == S2)
#define	FIO_INDEPENDENT_SD		0
#else
#define	FIO_INDEPENDENT_SD		1
#endif

/* For BCH mode */
#if (CHIP_REV == S3L)
#define FIO_SUPPORT_SKIP_BLANK_ECC	1
#else
#define FIO_SUPPORT_SKIP_BLANK_ECC	0
#endif

/* ==========================================================================*/
#define FIO_FIFO_OFFSET			0x0000
#define FIO_OFFSET			0x1000
#if (CHIP_REV == S2E)
#define FIO_4K_OFFSET			0x1e000
#else
#define FIO_4K_OFFSET			0x30000
#endif

#define FIO_4K_PHYS_BASE		(AHB_PHYS_BASE + FIO_4K_OFFSET)

#define FIO_FIFO_BASE			(AHB_BASE + FIO_FIFO_OFFSET)
#define FIO_BASE			(AHB_BASE + FIO_OFFSET)
#define FIO_4K_BASE			(AHB_BASE + FIO_4K_OFFSET)
#define FIO_REG(x)			(FIO_BASE + (x))
#define FIO_4K_REG(x)			(FIO_4K_BASE + (x))

/* ==========================================================================*/
#define FIO_CTR_OFFSET			0x000
#define FIO_STA_OFFSET			0x004
#define FIO_DMACTR_OFFSET		0x080
#define FIO_DMAADR_OFFSET		0x084
#define FIO_DMASTA_OFFSET		0x08c
#define FIO_DSM_CTR_OFFSET              0x0a0
#define FIO_ECC_RPT_STA_OFFSET          0x0a4

#define FIO_CTR_REG			FIO_REG(0x000)
#define FIO_STA_REG			FIO_REG(0x004)
#define FIO_DMACTR_REG			FIO_REG(0x080)
#define FIO_DMAADR_REG			FIO_REG(0x084)
#define FIO_DMASTA_REG			FIO_REG(0x08c)
#define FIO_DSM_CTR_REG			FIO_REG(0x0a0)
#define FIO_ECC_RPT_STA_REG		FIO_REG(0x0a4)

/* FIO_CTR_REG */
#define FIO_CTR_DA			0x00020000
#define FIO_CTR_DR			0x00010000
#define FIO_CTR_SX			0x00000100
#if (FIO_SUPPORT_SKIP_BLANK_ECC == 1)
#define FIO_CTR_SKIP_BLANK		0x00000080
#else
#define FIO_CTR_SKIP_BLANK		0x00000000
#endif
#define FIO_CTR_ECC_8BIT		0x00000060
#define FIO_CTR_ECC_6BIT		0x00000040
#define FIO_CTR_RS			0x00000010
#define FIO_CTR_SE			0x00000008
#define FIO_CTR_CO			0x00000004
#define FIO_CTR_RR			0x00000002
#define FIO_CTR_XD			0x00000001

/* FIO_STA_REG */
#define FIO_STA_SI			0x00000008
#define FIO_STA_CI			0x00000004
#define FIO_STA_XI			0x00000002
#define FIO_STA_FI			0x00000001

/* FIO_DMACTR_REG */
#define FIO_DMACTR_EN			0x80000000
#define FIO_DMACTR_RM			0x40000000
#define FIO_DMACTR_SD			0x30000000
#define FIO_DMACTR_CF			0x20000000
#define FIO_DMACTR_XD			0x10000000
#define FIO_DMACTR_FL			0x00000000
#define FIO_DMACTR_BLK_32768B		0x0c000000
#define FIO_DMACTR_BLK_16384B		0x0b000000
#define FIO_DMACTR_BLK_8192B		0x0a000000
#define FIO_DMACTR_BLK_4096B		0x09000000
#define FIO_DMACTR_BLK_2048B		0x08000000
#define FIO_DMACTR_BLK_1024B		0x07000000
#define FIO_DMACTR_BLK_256B		0x05000000
#define FIO_DMACTR_BLK_512B		0x06000000
#define FIO_DMACTR_BLK_128B		0x04000000
#define FIO_DMACTR_BLK_64B		0x03000000
#define FIO_DMACTR_BLK_32B		0x02000000
#define FIO_DMACTR_BLK_16B		0x01000000
#define FIO_DMACTR_BLK_8B		0x00000000
#define FIO_DMACTR_TS8B			0x00c00000
#define FIO_DMACTR_TS4B			0x00800000
#define FIO_DMACTR_TS2B			0x00400000
#define FIO_DMACTR_TS1B			0x00000000

/* FIO_DMASTA_REG */
#define FIO_DMASTA_RE			0x04000000
#define FIO_DMASTA_AE			0x02000000
#define FIO_DMASTA_DN			0x01000000

/* FIO_DSM_CTR_REG */
#define FIO_DSM_EN			0x80000000
#define FIO_DSM_MAJP_2KB		0x00000090
#define FIO_DSM_SPJP_64B		0x00000004
#define FIO_DSM_SPJP_128B		0x00000005

/* FIO_ECC_RPT_REG */
#define FIO_ECC_RPT_ERR			0x80000000
#define FIO_ECC_RPT_FAIL		0x40000000

/* ==========================================================================*/
#define SELECT_FIO_FREE		(0x00000000)
#define SELECT_FIO_FL		(0x00000001)
#define SELECT_FIO_XD		(0x00000002)
#define SELECT_FIO_CF		(0x00000004)
#define SELECT_FIO_SD		(0x00000008)
#define SELECT_FIO_SDIO		(0x00000010)

#define FIO_OP_NOT_DONE_ER	(-1)	/* operation(xfer) not done error */
#define FIO_READ_ER		(-2)	/* uncorrected ECC error */
#define	FIO_ADDR_ER		(-3)	/* address unaligned error */

/* ==========================================================================*/
#ifndef __ASSEMBLER__

/* ==========================================================================*/

#if (FIO_INDEPENDENT_SD == 1)
static inline void fio_select_lock(int module) { }
static inline void fio_unlock(int module) { }
static inline void fio_amb_sd0_set_int(u32 mask, u32 on){ }
static inline void fio_amb_sdio0_set_int(u32 mask, u32 on){ }
#else
extern void fio_select_lock(int module);
extern void fio_unlock(int module);
extern void fio_amb_sd0_set_int(u32 mask, u32 on);
extern void fio_amb_sdio0_set_int(u32 mask, u32 on);
#endif

extern int ambarella_init_fio(void);
extern void ambarella_fio_rct_reset(void);

extern void *ambarella_fio_push(void *func, u32 size);

#endif /* __ASSEMBLER__ */
/* ==========================================================================*/

#endif /* __PLAT_AMBARELLA_COMMON_FIO_H__ */

