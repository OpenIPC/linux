/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __EDMAC_H__
#define __EDMAC_H__

#define MAX_TRANSFER_BYTES  0x400000

/* reg offset */
#define DMA_INT_STATUS			(0x00)
#define DMA_TC_STATUS				(0x04)
#define DMA_TC_CLEAR				(0x08)
#define DMA_ERR_STATUS			(0x0C)
#define DMA_ERR_CLEAR				(0x10)
#define DMA_RAW_TC_STATUS			(0x14)
#define DMA_RAW_ERR_STATUS		(0x18)
#define DMA_EN_CHAN				(0x1c)
#define DMA_SYNC					(0x20)
#define DMA_VER					(0x34)
#define DMA_FEA					(0x38)


#define DMA_Cx_BASE(x)			((0x100 + (x * 0x20)))
/* The Faraday FTDMAC030 derivative shuffles the registers around */
#define DMA_CH_CTL			(0x00)
#define DMA_CH_CFG			(0x04)
#define DMA_CH_SRC_ADDR			(0x08)
#define DMA_CH_DST_ADDR			(0x0C)
#define DMA_CH_LLP			(0x10)
#define DMA_CH_SIZE			(0x14)
#define DMA_CH_STRIDE			(0x18)

#define DMA_ENDIAN               0x4c
#define DMA_WRITE_ONLY           0x50

#define DMA_WIDTH_8BIT              (0x0)
#define DMA_WIDTH_16BIT             (0x1)
#define DMA_WIDTH_32BIT             (0x2)
#define DMA_WIDTH_64BIT             (0x3)
#define DMA_WIDTH_128BIT            (0x4)


/* bit mapping of main configuration status register(CSR 0x24) */
#define DMA_CSR_M1ENDIAN         0x00000004
#define DMA_CSR_M0ENDIAN         0x00000002
#define DMA_CSR_DMACEN           0x00000001


#define DMA_CSR_TC_MSK           0x10000000 //bit28


#define DMA_CSR_SRCCTRL_Fix      0x100000
#define DMA_CSR_DSTCTRL_Fix      0x040000
#define DMA_CSR_WDT_EN           0x020000
#define DMA_CSR_CH_EN            0x010000 //bit16
#define DMA_CSR_EXP_EN           0x8000
#define DMA_CSR_2D_EN            0x4000
#define DMA_CSR_WEVENT_EN        0x2000
#define DMA_CSR_SEVENT_EN        0x1000
#define DMA_CSR_WSYNC            0x0100
/*END  bit mapping of channel control register 0x100 */

/* bit mapping of channel control register 0x104 */
#define DMA_CSR_UNALIGN          0x80000000
#define DMA_CSR_WRITE_ONLY       0x40000000
#define DMA_CSR_CHPRI_HI         0x10000000
/* hardware feature register*/
#define DMA_FEATURE_UNALIGN      0x08

/*END  bit mapping of channel control register 0x104 */
#define DMA_CSR_CHPRJ_HIGHEST    0x00C00000
#define DMA_CSR_CHPRJ_2ND        0x00800000
#define DMA_CSR_CHPRJ_3RD        0x00400000
#define DMA_CSR_PRTO3            0x00200000
#define DMA_CSR_PRTO2            0x00100000
#define DMA_CSR_PRTO1            0x00080000

#define DMA_CSR_ABT              0x00008000

#define DMA_CSR_MODE_NORMAL      0x00000000
#define DMA_CSR_MODE_HANDSHAKE   0x00000080

#define DMA_CSR_SRC_INCREMENT    0x00000000
#define DMA_CSR_SRC_DECREMENT    0x00000020
#define DMA_CSR_SRC_FIX          0x00000040

#define DMA_CSR_DST_INCREMENT    0x00000000
#define DMA_CSR_DST_DECREMENT    0x00000008
#define DMA_CSR_DST_FIX          0x00000010

#define DMA_CSR_SRC_SEL          0x00000004
#define DMA_CSR_DST_SEL          0x00000002
#define DMA_CSR_CH_ENABLE        0x00000001

#define DMA_CSR_DMA_FF_TH        0x07000000
#define DMA_CSR_CHPR1            0x00C00000
#define DMA_CSR_SRC_SIZE         0x00070000
#define DMA_CSR_SRC_WIDTH        0x00003800
#define DMA_CSR_DST_WIDTH        0x00000700
#define DMA_CSR_SRCAD_CTL        0x00000060
#define DMA_CSR_DSTAD_CTL        0x00000018


#define DMA_CFG_INT_ABT_MSK      0x00000004
#define DMA_CFG_INT_ERR_MSK      0x00000002
#define DMA_CFG_INT_TC_MSK       0x00000001
#define DMA_LLP_DMA_FF_TH        0xE0000000

#define DMA_LLP_TC_MSK           0x10000000

#define DMA_LLP_SRC_WIDTH_8      0x00000000
#define DMA_LLP_SRC_WIDTH_16     0x02000000
#define DMA_LLP_SRC_WIDTH_32     0x04000000
#define DMA_LLP_SRC_WIDTH_64     0x06000000

#define DMA_LLP_DST_WIDTH_8      0x00000000
#define DMA_LLP_DST_WIDTH_16     0x00400000
#define DMA_LLP_DST_WIDTH_32     0x00800000
#define DMA_LLP_DST_WIDTH_64     0x00C00000

#define DMA_LLP_SRC_INCREMENT    0x00000000
#define DMA_LLP_SRC_DECREMENT    0x00100000
#define DMA_LLP_SRC_FIX          0x00200000

#define DMA_LLP_DST_INCREMENT    0x00000000
#define DMA_LLP_DST_DECREMENT    0x00040000
#define DMA_LLP_DST_FIX          0x00080000

#define DMA_LLP_SRC_SEL          0x00020000
#define DMA_LLP_DST_SEL          0x00010000


#define AHBDMA_Channel0             0
#define AHBDMA_Channel1             1
#define AHBDMA_Channel2             2
#define AHBDMA_Channel3             3
#define AHBDMA_Channel4             4
#define AHBDMA_Channel5             5
#define AHBDMA_Channel6             6
#define AHBDMA_Channel7             7

#define AHBDMA_SrcWidth_Byte        0
#define AHBDMA_SrcWidth_Word        1
#define AHBDMA_SrcWidth_DWord       2
#define AHBDMA_SrcWidth_DDWord      3


#define AHBDMA_DstWidth_Byte        0
#define AHBDMA_DstWidth_Word        1
#define AHBDMA_DstWidth_DWord       2
#define AHBDMA_DstWidth_DDWord      3


#define AHBDMA_Burst1               0
#define AHBDMA_Burst2               1
#define AHBDMA_Burst4               2
#define AHBDMA_Burst8               3
#define AHBDMA_Burst16              4
#define AHBDMA_Burst32              5
#define AHBDMA_Burst64              6
#define AHBDMA_Burst128             7

#define AHBDMA_NormalMode           0
#define AHBDMA_HwHandShakeMode      1

#define AHBDMA_SrcInc               0
#define AHBDMA_SrcFix               2

#define AHBDMA_DstInc               0
#define AHBDMA_DstFix               2

#define AHBDMA_PriorityLow          0
#define AHBDMA_PriorityHigh         1

#define AHBDMA_MAX_LLDSIZE          0x100

#define DMA_CH_CFG_INT_ABT_MASK		BIT(2)
#define DMA_CH_CFG_INT_ERR_MASK		BIT(1)
#define DMA_CH_CFG_INT_TC_MASK		BIT(0)


#endif
