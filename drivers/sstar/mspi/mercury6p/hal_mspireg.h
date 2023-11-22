/*
 * hal_mspireg.h- Sigmastar
 *
 * Copyright (c) [2019~2020] SigmaStar Technology.
 *
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License version 2 for more details.
 *
 */

#ifndef __MHAL_MSPI_REG_H__
#define __MHAL_MSPI_REG_H__

#define MUX_SPI0 0
#define MUX_SPI1 1

#define EJTAG_MODE 0x0F
#define MSPI0_MODE 0x0C // bit0~bit1
#define MSPI1_MODE 0x0C // bit4~bit5

#define MSPI_WRITE_BUF_OFFSET 0x40
#define MSPI_READ_BUF_OFFSET  0x44
#define MSPI_WBF_SIZE_OFFSET  0x48
#define MSPI_RBF_SIZE_OFFSET  0x48
// read/ write buffer size
#define MSPI_RWSIZE_MASK      0xFF
#define MSPI_RSIZE_BIT_OFFSET 0x8
#define MAX_READ_BUF_SIZE     0x8
#define MAX_WRITE_BUF_SIZE    0x8
// CLK config
#define MSPI_CTRL_OFFSET             0x49
#define MSPI_CLK_CLOCK_OFFSET        0x49
#define MSPI_CLK_CLOCK_BIT_OFFSET    0x08
#define MSPI_CLK_CLOCK_MASK          0xFF
#define MSPI_CLK_PHASE_MASK          0x40
#define MSPI_3WIREMODE_MASK          0x10
#define MSPI_CLK_PHASE_BIT_OFFSET    0x06
#define MSPI_CLK_POLARITY_MASK       0x80
#define MSPI_CLK_POLARITY_BIT_OFFSET 0x07
#define MSPI_CLK_PHASE_MAX           0x1
#define MSPI_CLK_POLARITY_MAX        0x1
#define MSPI_CLK_CLOCK_MAX           0x7
// DC config
#define MSPI_DC_MASK            0xFF
#define MSPI_DC_BIT_OFFSET      0x08
#define MSPI_DC_TR_START_OFFSET 0x4A
#define MSPI_DC_TRSTART_MAX     0xFF
#define MSPI_DC_TR_END_OFFSET   0x4A
#define MSPI_DC_TREND_MAX       0xFF
#define MSPI_DC_TB_OFFSET       0x4B
#define MSPI_DC_TB_MAX          0xFF
#define MSPI_DC_TRW_OFFSET      0x4B
#define MSPI_DC_TRW_MAX         0xFF
// Frame Config
#define MSPI_FRAME_WBIT_OFFSET  0x4C
#define MSPI_FRAME_RBIT_OFFSET  0x4E
#define MSPI_FRAME_BIT_MAX      0x07
#define MSPI_FRAME_BIT_MASK     0x07
#define MSPI_FRAME_BIT_FIELD    0x03
#define MSPI_LSB_FIRST_OFFSET   0x34
#define MSPI_TRIGGER_OFFSET     0x5A
#define MSPI_DONE_OFFSET        0x5B
#define MSPI_DONE_CLEAR_OFFSET  0x5C
#define MSPI_CHIP_SELECT_OFFSET 0x5F

#define MSPI_CLK_NOT_GATED_OFFSET 0x76
#define MSPI_CLK_NOT_GATED_MASK   0x100

#define MSPI_FULL_DEPLUX_OFFSET 0x78

// chip select bit map
#define MSPI_CHIP_SELECT_MAX 0x07
// control bit
#define MSPI_DONE_FLAG  0x01
#define MSPI_TRIGGER    0x01
#define MSPI_CLEAR_DONE 0x01
#define MSPI_INT_ENABLE 0x04
#define MSPI_RESET      0x02
#define MSPI_ENABLE     0x01

// spi dma
#define MSPI_DMA_DATA_LENGTH_L 0x30
#define MSPI_DMA_DATA_LENGTH_H 0x31
#define MSPI_DMA_ENABLE        0x32
#define MSPI_DMA_RW_MODE       0x33
#define MSPI_DMA_WRITE         0x00
#define MSPI_DMA_READ          0x01

#define MSPI_CLK_DIV_VAL              \
    {                                 \
        2, 4, 8, 16, 32, 64, 128, 256 \
    }

#endif
