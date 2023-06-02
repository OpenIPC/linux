/*
 * arch/arm/plat-ambarella/include/plat/dma.h
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

#ifndef __PLAT_AMBARELLA_DMA_H__
#define __PLAT_AMBARELLA_DMA_H__

/* ==========================================================================*/
#if (CHIP_REV == A5S) || (CHIP_REV == A7L)
#define NUM_DMA_CHANNELS 		4
#elif (CHIP_REV == S2)
#define NUM_DMA_CHANNELS 		5
#else
#define NUM_DMA_CHANNELS 		8
#endif

#define DMA_OFFSET			0x5000
#define DMA_BASE			(AHB_BASE + DMA_OFFSET)
#define DMA_REG(x)			(DMA_BASE + (x))

#define FDMA_OFFSET			0x12000
#define FDMA_BASE			(AHB_BASE + FDMA_OFFSET)
#define FDMA_REG(x)			(FDMA_BASE + (x))

/* ==========================================================================*/

#define INVALID_DMA_CHAN		0xFF

#if (CHIP_REV == S2L)
#define NOR_SPI_TX_DMA_CHAN		0 /* share with SSI0 */
#define NOR_SPI_RX_DMA_CHAN		1 /* share with SSI0 */
#define SSI1_TX_DMA_CHAN		2
#define SSI1_RX_DMA_CHAN		3
#define UART_TX_DMA_CHAN		4 /* share with SSIS0 */
#define UART_RX_DMA_CHAN		5 /* share with SSIS0 */
#define I2S_RX_DMA_CHAN			6
#define I2S_TX_DMA_CHAN			7
/* non-existed dma channel */
#define MS_AHB_SSI_TX_DMA_CHAN		INVALID_DMA_CHAN
#define SPDIF_AHB_SSI_DMA_CHAN		INVALID_DMA_CHAN
#define SLIM_TX_DMA_CHAN		INVALID_DMA_CHAN
#define SLIM_RX_DMA_CHAN		INVALID_DMA_CHAN

#elif (CHIP_REV == S3) || (CHIP_REV == S3L)
#define NOR_SPI_TX_DMA_CHAN		0
#define NOR_SPI_RX_DMA_CHAN		1
#define SSI1_TX_DMA_CHAN		2
#define SSI1_RX_DMA_CHAN		3
#define UART_TX_DMA_CHAN		4
#define UART_RX_DMA_CHAN		5
#define I2S_RX_DMA_CHAN			6
#define I2S_TX_DMA_CHAN			7

#elif (CHIP_REV == S2E)
#define NOR_SPI_TX_DMA_CHAN		0
#define I2S_RX_DMA_CHAN			1
#define I2S_TX_DMA_CHAN			2
#define I2S1_RX_DMA_CHAN		3
#define I2S1_TX_DMA_CHAN		4
#define UART_RX_DMA_CHAN		5
#define UART_TX_DMA_CHAN		6
#define NOR_SPI_RX_DMA_CHAN		7
/* non-existed dma channel */
#define SSI1_TX_DMA_CHAN		INVALID_DMA_CHAN
#define SSI1_RX_DMA_CHAN		INVALID_DMA_CHAN

#else
#define I2S_RX_DMA_CHAN			1
#define I2S_TX_DMA_CHAN			2
#define MS_AHB_SSI_TX_DMA_CHAN		3
#define SPDIF_AHB_SSI_DMA_CHAN		4
/* non-existed dma channel */
#define NOR_SPI_TX_DMA_CHAN		INVALID_DMA_CHAN
#define NOR_SPI_RX_DMA_CHAN		INVALID_DMA_CHAN
#define SSI1_TX_DMA_CHAN		INVALID_DMA_CHAN
#define SSI1_RX_DMA_CHAN		INVALID_DMA_CHAN
#define UART_TX_DMA_CHAN		INVALID_DMA_CHAN
#define UART_RX_DMA_CHAN		INVALID_DMA_CHAN
#define SLIM_TX_DMA_CHAN		INVALID_DMA_CHAN
#define SLIM_RX_DMA_CHAN		INVALID_DMA_CHAN

#endif

/* ==========================================================================*/
#define DMA_CHAN_CTR_REG(x)		DMA_REG((0x300 + ((x) << 4)))
#define DMA_CHAN_SRC_REG(x)		DMA_REG((0x304 + ((x) << 4)))
#define DMA_CHAN_DST_REG(x)		DMA_REG((0x308 + ((x) << 4)))
#define DMA_CHAN_STA_REG(x)		DMA_REG((0x30c + ((x) << 4)))
#define DMA_CHAN_DA_REG(x)		DMA_REG((0x380 + ((x) << 2)))

/* ==========================================================================*/

#define FDMA_CHAN_CTR_REG(x)		FDMA_REG((0x300 + ((x) << 4)))
#define FDMA_CHAN_SRC_REG(x)		FDMA_REG((0x304 + ((x) << 4)))
#define FDMA_CHAN_DST_REG(x)		FDMA_REG((0x308 + ((x) << 4)))
#define FDMA_CHAN_STA_REG(x)		FDMA_REG((0x30c + ((x) << 4)))
#define FDMA_CHAN_DA_REG(x)		FDMA_REG((0x380 + ((x) << 2)))
#define FDMA_CHAN_DSM_CTR_REG(x)	FDMA_REG((0x3a0 + ((x) << 4)))

#define FDMA_CHAN_SPR_CNT_REG(x)	FDMA_REG((0x200 + ((x) << 4)))
#define FDMA_CHAN_SPR_SRC_REG(x)	FDMA_REG((0x204 + ((x) << 4)))
#define FDMA_CHAN_SPR_DST_REG(x)	FDMA_REG((0x208 + ((x) << 4)))
#define FDMA_CHAN_SPR_STA_REG(x)	FDMA_REG((0x20c + ((x) << 4)))
#define FDMA_CHAN_SPR_DA_REG(x)		FDMA_REG((0x280 + ((x) << 2)))

/* ==========================================================================*/

#define DMA_INT_OFFSET			0x3f0
#define DMA_PAUSE_SET_OFFSET		0x3f4
#define DMA_PAUSE_CLR_OFFSET		0x3f8
#define DMA_EARLY_END_OFFSET		0x3fc

/* ==========================================================================*/

/* DMA_CHANX_CTR_REG */
#define DMA_CHANX_CTR_EN		0x80000000
#define DMA_CHANX_CTR_D			0x40000000
#define DMA_CHANX_CTR_WM		0x20000000
#define DMA_CHANX_CTR_RM		0x10000000
#define DMA_CHANX_CTR_NI		0x08000000
#define DMA_CHANX_CTR_BLK_1024B		0x07000000
#define DMA_CHANX_CTR_BLK_512B		0x06000000
#define DMA_CHANX_CTR_BLK_256B		0x05000000
#define DMA_CHANX_CTR_BLK_128B		0x04000000
#define DMA_CHANX_CTR_BLK_64B		0x03000000
#define DMA_CHANX_CTR_BLK_32B		0x02000000
#define DMA_CHANX_CTR_BLK_16B		0x01000000
#define DMA_CHANX_CTR_BLK_8B		0x00000000
#define DMA_CHANX_CTR_TS_8B		0x00C00000
#define DMA_CHANX_CTR_TS_4B		0x00800000
#define DMA_CHANX_CTR_TS_2B		0x00400000
#define DMA_CHANX_CTR_TS_1B		0x00000000

/* DMA descriptor bit fields */
#define DMA_DESC_EOC			0x01000000
#define DMA_DESC_WM			0x00800000
#define DMA_DESC_RM			0x00400000
#define DMA_DESC_NI			0x00200000
#define DMA_DESC_TS_8B			0x00180000
#define DMA_DESC_TS_4B			0x00100000
#define DMA_DESC_TS_2B			0x00080000
#define DMA_DESC_TS_1B			0x00000000
#define DMA_DESC_BLK_1024B		0x00070000
#define DMA_DESC_BLK_512B		0x00060000
#define DMA_DESC_BLK_256B		0x00050000
#define DMA_DESC_BLK_128B		0x00040000
#define DMA_DESC_BLK_64B		0x00030000
#define DMA_DESC_BLK_32B		0x00020000
#define DMA_DESC_BLK_16B		0x00010000
#define DMA_DESC_BLK_8B			0x00000000
#define DMA_DESC_ID			0x00000004
#define DMA_DESC_IE			0x00000002
#define DMA_DESC_ST			0x00000001

/* DMA_CHANX_STA_REG */
#define DMA_CHANX_STA_DM		0x80000000
#define DMA_CHANX_STA_OE		0x40000000
#define DMA_CHANX_STA_DA		0x20000000
#define DMA_CHANX_STA_DD		0x10000000
#define DMA_CHANX_STA_OD		0x08000000
#define DMA_CHANX_STA_ME		0x04000000
#define DMA_CHANX_STA_BE		0x02000000
#define DMA_CHANX_STA_RWE		0x01000000
#define DMA_CHANX_STA_AE		0x00800000
#define DMA_CHANX_STA_DN		0x00400000

/* DMA_INT_REG */
#define DMA_INT_CHAN(x)			(0x1 << (x))

#define DMA_INT_CHAN4			0x00000010
#define DMA_INT_CHAN3			0x00000008
#define DMA_INT_CHAN2			0x00000004
#define DMA_INT_CHAN1			0x00000002
#define DMA_INT_CHAN0			0x00000001

/* DMA_DUAL_SPACE_MODE_REG */
#define DMA_DSM_EN                      0x80000000
#define DMA_DSM_MAJP_2KB                0x00000090
#define DMA_DSM_SPJP_64B                0x00000004
#define DMA_DSM_SPJP_128B               0x00000005

#if (CHIP_REV == S3) || (CHIP_REV == S2E) || (CHIP_REV == S3L)
#define DMA_NODC_MN_BURST_SIZE	(DMA_CHANX_CTR_BLK_512B | DMA_CHANX_CTR_TS_8B)
#define DMA_NODC_SP_BURST_SIZE	(DMA_CHANX_CTR_BLK_16B | DMA_CHANX_CTR_TS_8B)
#define DMA_DESC_MN_BURST_SIZE	(DMA_DESC_BLK_512B | DMA_DESC_TS_8B)
#define DMA_DESC_SP_BURST_SIZE	(DMA_DESC_BLK_16B | DMA_DESC_TS_8B)
#define DMA_NODC_MN_BURST_SIZE8	(DMA_CHANX_CTR_BLK_512B | DMA_CHANX_CTR_TS_8B)
#define FIO_MN_BURST_SIZE	(FIO_DMACTR_BLK_512B | FIO_DMACTR_TS8B)
#define FIO_SP_BURST_SIZE	(FIO_DMACTR_BLK_16B | FIO_DMACTR_TS8B)
#define FIO_MN_BURST_SIZE8	(FIO_DMACTR_BLK_512B | FIO_DMACTR_TS8B)
#else
#define DMA_NODC_MN_BURST_SIZE	(DMA_CHANX_CTR_BLK_512B | DMA_CHANX_CTR_TS_4B)
#define DMA_NODC_SP_BURST_SIZE	(DMA_CHANX_CTR_BLK_16B | DMA_CHANX_CTR_TS_4B)
#define DMA_DESC_MN_BURST_SIZE	(DMA_DESC_BLK_512B | DMA_DESC_TS_4B)
#define DMA_DESC_SP_BURST_SIZE	(DMA_DESC_BLK_16B | DMA_DESC_TS_4B)
#define DMA_NODC_MN_BURST_SIZE8	(DMA_CHANX_CTR_BLK_512B | DMA_CHANX_CTR_TS_8B)
#define FIO_MN_BURST_SIZE	(FIO_DMACTR_BLK_512B | FIO_DMACTR_TS4B)
#define FIO_SP_BURST_SIZE	(FIO_DMACTR_BLK_16B | FIO_DMACTR_TS4B)
#define FIO_MN_BURST_SIZE8	(FIO_DMACTR_BLK_512B | FIO_DMACTR_TS8B)
#endif

/* ==========================================================================*/
#ifndef __ASSEMBLER__

extern int ambarella_dma_channel_id(void *chan);

#endif /* __ASSEMBLER__ */

/* ==========================================================================*/

#endif /* __PLAT_AMBARELLA_DMA_H__ */

