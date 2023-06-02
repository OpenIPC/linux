/**
    NVT mmc function
    Register settings
    @file       na51055_mmcreg.h
    @ingroup
    @note
    Copyright   Novatek Microelectronics Corp. 2019.  All rights reserved.

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2 as
    published by the Free Software Foundation.
*/

#ifndef _SDIO_REG_H
#define _SDIO_REG_H

#include <mach/rcw_macro.h>

#define SDIO_SCATTER_DMA            1
#define NEW_UNION                   1
#define SDIO_CMD_REG_OFS            0x00
#if NEW_UNION
union SDIO_CMD_REG {
	uint32_t reg;
	struct {
	unsigned int CMD_IDX:6;
	unsigned int NEED_RSP:1;
	unsigned int LONG_RSP:1;
	unsigned int RSP_TIMEOUT_TYPE:1;
	unsigned int CMD_EN:1;
	unsigned int SDC_RST:1;
	unsigned int ENABLE_SDIO_INT_DETECT:1;
	unsigned int ENABLE_VOL_SWITCH_DET:1;
	unsigned int ENABLE_EMMC_BOOT:1;
	unsigned int EMMC_BOOT_ACK:1;
	unsigned int EMMC_BOOT_MODE:1;
	unsigned int EMMC_BOOT_CLK:1;
	unsigned int Reserved0:15;
	} bit;
};
#else
REGDEF_BEGIN(SDIO_CMD_REG)
	REGDEF_BIT(CMD_IDX, 6)
	REGDEF_BIT(NEED_RSP, 1)
	REGDEF_BIT(LONG_RSP, 1)
	REGDEF_BIT(RSP_TIMEOUT_TYPE, 1)
	REGDEF_BIT(CMD_EN, 1)
	REGDEF_BIT(SDC_RST, 1)
	REGDEF_BIT(ENABLE_SDIO_INT_DETECT, 1)
	REGDEF_BIT(ENABLE_VOL_SWITCH_DET,  1)
	REGDEF_BIT(ENABLE_EMMC_BOOT,  1)
	REGDEF_BIT(EMMC_BOOT_ACK,  1)
	REGDEF_BIT(EMMC_BOOT_MODE,  1)
	REGDEF_BIT(EMMC_BOOT_CLK,  1)
	REGDEF_BIT(Reserved0, 15)
REGDEF_END(SDIO_CMD_REG)
#endif

#define SDIO_ARGU_REG_OFS           0x04
#if NEW_UNION
union SDIO_ARGU_REG {
	uint32_t reg;
	struct {
	unsigned int ARGUMENT:32;
	} bit;
};
#else
REGDEF_BEGIN(SDIO_ARGU_REG)
	REGDEF_BIT(ARGUMENT, 32)
REGDEF_END(SDIO_ARGU_REG)
#endif

#define SDIO_RSP0_REG_OFS           0x08
#if NEW_UNION
union SDIO_RSP0_REG {
	uint32_t reg;
	struct {
	unsigned int RSP:32;
	} bit;
};
#else
REGDEF_BEGIN(SDIO_RSP0_REG)
	REGDEF_BIT(RSP, 32)
REGDEF_END(SDIO_RSP0_REG)
#endif

#define SDIO_RSP1_REG_OFS           0x0C
#if NEW_UNION
union SDIO_RSP1_REG {
	uint32_t reg;
	struct {
	unsigned int RSP_3RD_MSB:32;
	} bit;
};
#else
REGDEF_BEGIN(SDIO_RSP1_REG)
	REGDEF_BIT(RSP_3RD_MSB, 32)
REGDEF_END(SDIO_RSP1_REG)
#endif

#define SDIO_RSP2_REG_OFS           0x10
#if NEW_UNION
union SDIO_RSP2_REG {
	uint32_t reg;
	struct {
	unsigned int RSP_2ND_MSB:32;
	} bit;
};
#else
REGDEF_BEGIN(SDIO_RSP2_REG)
	REGDEF_BIT(RSP_2ND_MSB, 32)
REGDEF_END(SDIO_RSP2_REG)
#endif

#define SDIO_RSP3_REG_OFS           0x14
#if NEW_UNION
union SDIO_RSP3_REG {
	uint32_t reg;
	struct {
	unsigned int RSP_1ST_MSB:32;
	} bit;
};
#else
REGDEF_BEGIN(SDIO_RSP3_REG)
	REGDEF_BIT(RSP_1ST_MSB, 32)
REGDEF_END(SDIO_RSP3_REG)
#endif


#define SDIO_RSP_CMD_REG_OFS        0x18
#if NEW_UNION
union SDIO_RSP_CMD_REG {
	uint32_t reg;
	struct {
	unsigned int RSP_CMD_IDX:6;
	unsigned int Reserved0:26;
	} bit;
};
#else
REGDEF_BEGIN(SDIO_RSP_CMD_REG)
	REGDEF_BIT(RSP_CMD_IDX, 6)
	REGDEF_BIT(Reserved0,  26)
REGDEF_END(SDIO_RSP_CMD_REG)
#endif

#define SDIO_DATA_CTRL_REG_OFS      0x1C
#if NEW_UNION
union SDIO_DATA_CTRL_REG {
	uint32_t reg;
	struct {
	unsigned int Reserved0:6;
	unsigned int DATA_EN:1;
	unsigned int READ_WAIT_EN:1;
	unsigned int SUSPEND_DATA:1;
	unsigned int DIS_SDIO_INT_PERIOD:1;
	unsigned int Reserved1:3;
	unsigned int WAIT_VOL_SWITCH_EN:1;
	unsigned int Reserved2:2;
	unsigned int BLK_SIZE:16;
	} bit;
};
#else
REGDEF_BEGIN(SDIO_DATA_CTRL_REG)
	REGDEF_BIT(Reserved0, 6)
	REGDEF_BIT(DATA_EN, 1)
	REGDEF_BIT(READ_WAIT_EN, 1)
	REGDEF_BIT(SUSPEND_DATA, 1)
	REGDEF_BIT(DIS_SDIO_INT_PERIOD, 1)
	REGDEF_BIT(Reserved1, 3)
	REGDEF_BIT(WAIT_VOL_SWITCH_EN, 1)
	REGDEF_BIT(Reserved2, 2)
	REGDEF_BIT(BLK_SIZE, 16)
REGDEF_END(SDIO_DATA_CTRL_REG)
#endif

#define SDIO_DATA_TIMER_REG_OFS     0x20
#if NEW_UNION
union SDIO_DATA_TIMER_REG {
	uint32_t reg;
	struct {
	unsigned int TIMEOUT:32;
	} bit;
};
#else
REGDEF_BEGIN(SDIO_DATA_TIMER_REG)
	REGDEF_BIT(TIMEOUT, 32)
REGDEF_END(SDIO_DATA_TIMER_REG)
#endif

#define SDIO_VOL_SWITCH_TIMER_REG_OFS     0x24
#if NEW_UNION
union SDIO_VOL_SWITCH_TIMER_REG {
	uint32_t reg;
	struct {
	unsigned int VOL_TIMEOUT:12;
	unsigned int Reserved1:20;
	} bit;
};
#else
REGDEF_BEGIN(SDIO_VOL_SWITCH_TIMER_REG)
	REGDEF_BIT(VOL_TIMEOUT, 12)
	REGDEF_BIT(Reserved1, 20)
REGDEF_END(SDIO_VOL_SWITCH_TIMER_REG)
#endif


#define SDIO_STATUS_REG_OFS  0x28
#if NEW_UNION
union SDIO_STATUS_REG {
	uint32_t reg;
	struct {
	unsigned int RSP_CRC_FAIL:1;
	unsigned int DATA_CRC_FAIL:1;
	unsigned int RSP_TIMEOUT:1;
	unsigned int DATA_TIMEOUT:1;
	unsigned int RSP_CRC_OK:1;
	unsigned int DATA_CRC_OK:1;
	unsigned int CMD_SENT:1;
	unsigned int DATA_END:1;
	unsigned int SDIO_INT:1;
	unsigned int READ_WAIT:1;
	unsigned int CARD_BUSY2READY:1;
	unsigned int VOL_SWITCH_END:1;
	unsigned int VOL_SWITCH_TIMEOUT:1;
	unsigned int RSP_VOL_SWITCH_FAIL:1;
	unsigned int VOL_SWITCH_GLITCH:1;
	unsigned int EMMC_BOOT_ACK_RECEIVE:1;
	unsigned int EMMC_BOOT_ACK_TIMEOUT:1;
	unsigned int EMMC_BOOT_END:1;
	unsigned int EMMC_BOOT_ACK_ERROR:1;
	unsigned int DMA_ERROR:1;
	unsigned int Reserved0:12;
	} bit;
};
#else
	REGDEF_BEGIN(SDIO_STATUS_REG)
	REGDEF_BIT(RSP_CRC_FAIL, 1)
	REGDEF_BIT(DATA_CRC_FAIL, 1)
	REGDEF_BIT(RSP_TIMEOUT, 1)
	REGDEF_BIT(DATA_TIMEOUT, 1)
	REGDEF_BIT(RSP_CRC_OK, 1)
	REGDEF_BIT(DATA_CRC_OK, 1)
	REGDEF_BIT(CMD_SENT, 1)
	REGDEF_BIT(DATA_END, 1)
	REGDEF_BIT(SDIO_INT, 1)
	REGDEF_BIT(READ_WAIT, 1)
	REGDEF_BIT(CARD_BUSY2READY, 1)
	REGDEF_BIT(VOL_SWITCH_END, 1)
	REGDEF_BIT(VOL_SWITCH_TIMEOUT, 1)
	REGDEF_BIT(RSP_VOL_SWITCH_FAIL, 1)
	REGDEF_BIT(VOL_SWITCH_GLITCH, 1)
	REGDEF_BIT(EMMC_BOOT_ACK_RECEIVE, 1)
	REGDEF_BIT(EMMC_BOOT_ACK_TIMEOUT, 1)
	REGDEF_BIT(EMMC_BOOT_END, 1)
	REGDEF_BIT(EMMC_BOOT_ACK_ERROR, 1)
	REGDEF_BIT(Reserved0, 13)
REGDEF_END(SDIO_STATUS_REG)
#endif

#define SDIO_INT_MASK_REG_OFS       0x30
#if NEW_UNION
union SDIO_INT_MASK_REG {
	uint32_t reg;
	struct {
	unsigned int RSP_CRC_FAIL_INT_EN:1;
	unsigned int DATA_CRC_FAIL_INT_EN:1;
	unsigned int RSP_TIMEOUT_INT_EN:1;
	unsigned int DATA_TIMEOUT_INT_EN:1;
	unsigned int RSP_CRC_OK_INT_EN:1;
	unsigned int DATA_CRC_OK_INT_EN:1;
	unsigned int CMD_SENT_INT_EN:1;
	unsigned int DATA_END_INT_EN:1;
	unsigned int SDIO_INT_INT_EN:1;
	unsigned int READ_WAIT_INT_EN:1;
	unsigned int CARD_BUSY2READY_INT_EN:1;
	unsigned int VOL_SWITCH_END_INT_EN:1;
	unsigned int VOL_SWITCH_TIMEOUT_INT_EN:1;
	unsigned int RSP_VOL_SWITCH_FAIL_INT_EN:1;
	unsigned int VOL_SWITCH_GLITCH_INT_EN:1;
	unsigned int EMMC_BOOT_ACK_RECEIVE_INT_EN:1;
	unsigned int EMMC_BOOT_ACK_TIMEOUT_INT_EN:1;
	unsigned int EMMC_BOOT_END_INT_EN:1;
	unsigned int EMMC_BOOT_ACK_ERROR_INT_EN:1;
	unsigned int Reserved0:13;
	} bit;
};
#else
REGDEF_BEGIN(SDIO_INT_MASK_REG)
	REGDEF_BIT(RSP_CRC_FAIL_INT_EN, 1)
	REGDEF_BIT(DATA_CRC_FAIL_INT_EN, 1)
	REGDEF_BIT(RSP_TIMEOUT_INT_EN, 1)
	REGDEF_BIT(DATA_TIMEOUT_INT_EN, 1)
	REGDEF_BIT(RSP_CRC_OK_INT_EN, 1)
	REGDEF_BIT(DATA_CRC_OK_INT_EN, 1)
	REGDEF_BIT(CMD_SENT_INT_EN, 1)
	REGDEF_BIT(DATA_END_INT_EN, 1)
	REGDEF_BIT(SDIO_INT_INT_EN, 1)
	REGDEF_BIT(READ_WAIT_INT_EN, 1)
	REGDEF_BIT(CARD_BUSY2READY_INT_EN, 1)
	REGDEF_BIT(VOL_SWITCH_END_INT_EN, 1)
	REGDEF_BIT(VOL_SWITCH_TIMEOUT_INT_EN, 1)
	REGDEF_BIT(RSP_VOL_SWITCH_FAIL_INT_EN, 1)
	REGDEF_BIT(VOL_SWITCH_GLITCH_INT_EN, 1)
	REGDEF_BIT(EMMC_BOOT_ACK_RECEIVE_INT_EN, 1)
	REGDEF_BIT(EMMC_BOOT_ACK_TIMEOUT_INT_EN, 1)
	REGDEF_BIT(EMMC_BOOT_END_INT_EN, 1)
	REGDEF_BIT(EMMC_BOOT_ACK_ERROR_INT_EN, 1)
	REGDEF_BIT(Reserved0, 13)
REGDEF_END(SDIO_INT_MASK_REG)
#endif

#define SDIO_CLOCK_CTRL_REG_OFS     0x38
#if NEW_UNION
union SDIO_CLOCK_CTRL_REG {
	uint32_t reg;
	struct {
	unsigned int REV0:8;
	unsigned int CLK_AUTOGATE_HIGH:1;
	unsigned int REV1:1;
	unsigned int CLK_DIS:1;
	unsigned int REV2:21;
	} bit;
};
#else
REGDEF_BEGIN(SDIO_CLOCK_CTRL_REG)
	REGDEF_BIT(REV0,     8)
	REGDEF_BIT(CLK_AUTOGATE_HIGH, 1)
	REGDEF_BIT(CLK_SD,   1)
	REGDEF_BIT(CLK_DIS,  1)
	REGDEF_BIT(CLK_DDR,  1)
	REGDEF_BIT(DLY_ACT,  1)
	REGDEF_BIT(DLY_TYPE, 1)
	REGDEF_BIT(REV1,     2)
	REGDEF_BIT(DLY_POLARITY, 1)
	REGDEF_BIT(REV2,     3)
	REGDEF_BIT(SMP_EDGE_SEL, 1)
	REGDEF_BIT(CLK_SD_CMD, 1)
	REGDEF_BIT(SMP_EDGE_SEL_CMD, 1)
	REGDEF_BIT(REV3,     1)
	REGDEF_BIT(DEL_SEL,  6)
	REGDEF_BIT(REV4,     2)
REGDEF_END(SDIO_CLOCK_CTRL_REG)
#endif

#define SDIO_BUS_WIDTH_REG_OFS      0x3C
#if NEW_UNION
union SDIO_BUS_WIDTH_REG {
	uint32_t reg;
	struct {
	unsigned int BUS_WIDTH:2;
	unsigned int Reserved0:30;
	} bit;
};
#else
REGDEF_BEGIN(SDIO_BUS_WIDTH_REG)
	REGDEF_BIT(BUS_WIDTH, 2)
	REGDEF_BIT(Reserved0, 30)
REGDEF_END(SDIO_BUS_WIDTH_REG)
#endif

#define SDIO_BUS_STATUS_REG_OFS     0x40
#if NEW_UNION
union SDIO_BUS_STATUS_REG {
	uint32_t reg;
	struct {
	unsigned int CARD_BUSY:1;
	unsigned int BUS_STS_CMD:1;
	unsigned int Reserved0:6;
	unsigned int BUS_STS_D0:1;
	unsigned int BUS_STS_D1:1;
	unsigned int BUS_STS_D2:1;
	unsigned int BUS_STS_D3:1;
	unsigned int BUS_STS_D4:1;
	unsigned int BUS_STS_D5:1;
	unsigned int BUS_STS_D6:1;
	unsigned int BUS_STS_D7:1;
	unsigned int Reserved1:16;
	} bit;
};
#else
REGDEF_BEGIN(SDIO_BUS_STATUS_REG)
	REGDEF_BIT(CARD_BUSY, 1)
	REGDEF_BIT(BUS_STS_CMD, 1)
	REGDEF_BIT(Reserved0, 6)
	REGDEF_BIT(BUS_STS_D0, 1)
	REGDEF_BIT(BUS_STS_D1, 1)
	REGDEF_BIT(BUS_STS_D2, 1)
	REGDEF_BIT(BUS_STS_D3, 1)
	REGDEF_BIT(BUS_STS_D4, 1)
	REGDEF_BIT(BUS_STS_D5, 1)
	REGDEF_BIT(BUS_STS_D6, 1)
	REGDEF_BIT(BUS_STS_D7, 1)
	REGDEF_BIT(Reserved1, 16)
REGDEF_END(SDIO_BUS_STATUS_REG)
#endif

#define SDIO_CLOCK_CTRL2_REG_OFS    0x44
#if NEW_UNION
union SDIO_CLOCK_CTRL2_REG {
	uint32_t reg;
	struct {
	unsigned int OUTDLY_SEL:6;
	unsigned int Reserved0:10;
	unsigned int INDLY_SEL:6;
	unsigned int Reserved1:9;
	} bit;
};
#else
REGDEF_BEGIN(SDIO_CLOCK_CTRL2_REG)
	REGDEF_BIT(OUTDLY_SEL, 5)
	REGDEF_BIT(Reserved0, 27)
REGDEF_END(SDIO_CLOCK_CTRL2_REG)
#endif

#define SDIO_EMMC_BOOT_ACK_TIMER_REG_OFS    0x48
#if NEW_UNION
union SDIO_EMMC_BOOT_ACK_TIMER_REG {
	uint32_t reg;
	struct {
	unsigned int TIMEOUT:24;
	unsigned int Reserved0:8;
	} bit;
};
#else
REGDEF_BEGIN(SDIO_EMMC_BOOT_ACK_TIMER_REG)
	REGDEF_BIT(TIMEOUT, 24)
	REGDEF_BIT(Reserved0, 8)
REGDEF_END(SDIO_EMMC_BOOT_ACK_TIMER_REG)
#endif

#define SDIO_PHY_REG_OFS    0x4C
#if NEW_UNION
union SDIO_PHY_REG {
	uint32_t reg;
	struct {
	unsigned int PHY_SW_RST:1;
	unsigned int Reserved0:3;
	unsigned int BLK_FIFO_EN:1;
	unsigned int Reserved1:27;
	} bit;
};
#else
REGDEF_BEGIN(SDIO_PHY_REG)
    REGDEF_BIT(PHY_SW_RST,  1)
    REGDEF_BIT(           , 3)
    REGDEF_BIT(BLK_FIFO_EN, 1)
    REGDEF_BIT(           , 27)
REGDEF_END(SDIO_PHY_REG)
#endif

#define SDIO_DLY0_REG_OFS           0x50
#if NEW_UNION
union SDIO_DLY0_REG {
	uint32_t reg;
	struct {
	unsigned int Reserved0:12;
	unsigned int SAMPLE_CLK_EDGE:1;
	unsigned int SRC_CLK_SEL:1;
	unsigned int Reserved1:18;
	} bit;
};
#else
REGDEF_BEGIN(SDIO_DLY0_REG)
    REGDEF_BIT(DLY_PHASE_SEL,   4)
    REGDEF_BIT(DLY_SEL,         3)
    REGDEF_BIT(                ,1)
    REGDEF_BIT(DET_SEL,         4)
    REGDEF_BIT(SAMPLE_CLK_INV,  1)
    REGDEF_BIT(SRC_CLK_SEL,     1)
    REGDEF_BIT(PAD_CLK_SEL,     1)
    REGDEF_BIT(                ,5)
    REGDEF_BIT(DATA_DLY_SEL,    5)
    REGDEF_BIT(                ,3)
    REGDEF_BIT(OUT_CLK_DLY_SEL, 4)
REGDEF_END(SDIO_DLY0_REG)
#endif

#define SDIO_DLY1_REG_OFS           0x54
#if NEW_UNION
union SDIO_DLY1_REG {
	uint32_t reg;
	struct {
	unsigned int Reserved0:28;
	unsigned int DATA_READ_DLY:2;
	unsigned int Reserved1:2;
	} bit;
};
#else
REGDEF_BEGIN(SDIO_DLY1_REG)
    REGDEF_BIT(DATA_DSW_SEL,   27)
    REGDEF_BIT(                ,1)
    REGDEF_BIT(DATA_READ_DLY,   2)
    REGDEF_BIT(DET_READ_DLY,    2)
REGDEF_END(SDIO_DLY1_REG)
#endif

#define SDIO_DLY2_REG_OFS           0x58
#if NEW_UNION
union SDIO_DLY2_REG {
	uint32_t reg;
	struct {
	unsigned int DET_DATA_OUT:16;
	unsigned int Reserved0:16;
	} bit;
};
#else
REGDEF_BEGIN(SDIO_DLY2_REG)
    REGDEF_BIT(DET_DATA_OUT,   16)
    REGDEF_BIT(               ,16)
REGDEF_END(SDIO_DLY2_REG)
#endif

#define SDIO_DLY3_REG_OFS           0x5C
#if NEW_UNION
union SDIO_DLY3_REG {
	uint32_t reg;
	struct {
	unsigned int DET_CLR:1;
	unsigned int Reserved0:31;
	} bit;
};
#else
REGDEF_BEGIN(SDIO_DLY3_REG)
    REGDEF_BIT(DET_CLR,   1)
    REGDEF_BIT(          ,31)
REGDEF_END(SDIO_DLY3_REG)
#endif

#define SDIO_DLY4_REG_OFS           0x60
#if NEW_UNION
union SDIO_DLY4_REG {
	uint32_t reg;
	struct {
	unsigned int PHASE_COMP_EN:16;
	unsigned int Reserved0:16;
	} bit;
};
#else
REGDEF_BEGIN(SDIO_DLY4_REG)
    REGDEF_BIT(PHASE_COMP_EN,   16)
    REGDEF_BIT(          ,16)
REGDEF_END(SDIO_DLY4_REG)
#endif

#define SDIO_DATA_PORT_REG_OFS  0x100
#if NEW_UNION
union SDIO_DATA_PORT_REG {
	uint32_t reg;
	struct {
	unsigned int FIFO:32;
	} bit;
};
#else
REGDEF_BEGIN(SDIO_DATA_PORT_REG)
	REGDEF_BIT(FIFO, 32)
REGDEF_END(SDIO_DATA_PORT_REG)
#endif

#define SDIO_DATA_LENGTH_REG_OFS  0x104
#if NEW_UNION
union SDIO_DATA_LENGTH_REG {
	uint32_t reg;
	struct {
	unsigned int LENGTH:26;
	unsigned int REV0:6;
	} bit;
};
#else
REGDEF_BEGIN(SDIO_DATA_LENGTH_REG)
	REGDEF_BIT(LENGTH, 26)
	REGDEF_BIT(REV0,   6)
REGDEF_END(SDIO_DATA_LENGTH_REG)
#endif

#define SDIO_FIFO_STATUS_REG_OFS  0x108
#if NEW_UNION
union SDIO_FIFO_STATUS_REG {
	uint32_t reg;
	struct {
	unsigned int FIFO_CNT:6;
	unsigned int Reserved0:2;
	unsigned int FIFO_EMPTY:1;
	unsigned int FIFO_FULL:1;
	unsigned int Reserved1:22;
	} bit;
};
#else
REGDEF_BEGIN(SDIO_FIFO_STATUS_REG)
	REGDEF_BIT(FIFO_CNT, 6)
	REGDEF_BIT(Reserved0, 2)
	REGDEF_BIT(FIFO_EMPTY, 1)
	REGDEF_BIT(FIFO_FULL, 1)
	REGDEF_BIT(Reserved1, 22)
REGDEF_END(SDIO_FIFO_STATUS_REG)
#endif

#define SDIO_FIFO_CONTROL_REG_OFS  0x10C
#if NEW_UNION
union SDIO_FIFO_CONTROL_REG {
	uint32_t reg;
	struct {
	unsigned int FIFO_EN:1;
	unsigned int FIFO_MODE:1;
	unsigned int FIFO_DIR:1;
	unsigned int Reserved0:29;
	} bit;
};
#else
REGDEF_BEGIN(SDIO_FIFO_CONTROL_REG)
	REGDEF_BIT(FIFO_EN, 1)
	REGDEF_BIT(FIFO_MODE, 1)
	REGDEF_BIT(FIFO_DIR, 1)
	REGDEF_BIT(Reserved0, 29)
REGDEF_END(SDIO_FIFO_CONTROL_REG)
#endif

#define SDIO_FIFO_SWITCH_REG_OFS   0x1B0
union SDIO_FIFO_SWITCH_REG {
	uint32_t reg;
	struct {
	unsigned int Reserved0:4;
	unsigned int FIFO_SWITCH_DLY:1;
	unsigned int Reserved1:27;
	} bit;
};

#if (SDIO_SCATTER_DMA)
#define SDIO_DMA_DES_START_ADDR_REG_OFS  0x110
#if NEW_UNION
union SDIO_DMA_DES_START_ADDR_REG {
	uint32_t reg;
	struct {
	unsigned int DES_ADDR:31;
	unsigned int Reserved0:1;
	} bit;
};
#else
REGDEF_BEGIN(SDIO_DMA_DES_START_ADDR_REG)
	REGDEF_BIT(DES_ADDR, 31)
	REGDEF_BIT(Reserved0, 1)
REGDEF_END(SDIO_DMA_DES_START_ADDR_REG)
#endif

#define SDIO_DMA_DES_CURR_ADDR_REG_OFS  0x114
#if NEW_UNION
union SDIO_DMA_DES_CURR_ADDR_REG {
	uint32_t reg;
	struct {
	unsigned int DRAM_ADDR:31;
	unsigned int Reserved0:1;
	} bit;
};
#else
REGDEF_BEGIN(SDIO_DMA_DES_CURR_ADDR_REG)
	REGDEF_BIT(DRAM_ADDR, 31)
	REGDEF_BIT(Reserved0, 1)
REGDEF_END(SDIO_DMA_DES_CURR_ADDR_REG)
#endif

#define SDIO_DMA_ERROR_STS_REG_OFS  0x118
#if NEW_UNION
union SDIO_DMA_ERROR_STS_REG {
	uint32_t reg;
	struct {
	unsigned int DMA_ERR_STS:2;
	unsigned int DMA_LEN_MISMATCH:1;
	unsigned int REV0:29;
	} bit;
};
#else
REGDEF_BEGIN(SDIO_DMA_ERROR_STS_REG)
	REGDEF_BIT(DMA_ERR_STS,      2)
	REGDEF_BIT(DMA_LEN_MISMATCH, 1)
	REGDEF_BIT(REV0,             29)
REGDEF_END(SDIO_DMA_ERROR_STS_REG)
#endif

#define SDIO_DES_LINE_REG_OFS       0x00
#if NEW_UNION
union SDIO_DES_LINE_REG {
	uint32_t reg;
	struct {
	unsigned int DMA_DES_VALID:1;
	unsigned int DMA_DES_END:1;
	unsigned int REV0:2;
	unsigned int DMA_DES_ACT:2;
	unsigned int REV1:26;
	} bit;
};
#else
REGDEF_BEGIN(SDIO_DES_LINE_REG)
	REGDEF_BIT(DMA_DES_VALID,    1)
	REGDEF_BIT(DMA_DES_END,      1)
	REGDEF_BIT(REV0,             2)
	REGDEF_BIT(DMA_DES_ACT,      2)
	REGDEF_BIT(REV1,             26)
REGDEF_END(SDIO_DES_LINE_REG)
#endif

#define SDIO_DES_LINE1_REG_OFS      0x04
#if NEW_UNION
union SDIO_DES_LINE1_REG {
	uint32_t reg;
	struct {
	unsigned int DMA_DES_DATA_LEN:26;
	unsigned int REV0:6;
	} bit;
};
#else
REGDEF_BEGIN(SDIO_DES_LINE1_REG)
	REGDEF_BIT(DMA_DES_DATA_LEN, 26)
	REGDEF_BIT(REV0,             6)
REGDEF_END(SDIO_DES_LINE1_REG)
#endif

#define SDIO_DES_LINE2_REG_OFS      0x08
#if NEW_UNION
union SDIO_DES_LINE2_REG {
	uint32_t reg;
	struct {
	unsigned int DMA_DES_DATA_ADDR:32;
	} bit;
};
#else
REGDEF_BEGIN(SDIO_DES_LINE2_REG)
	REGDEF_BIT(DMA_DES_DATA_ADDR, 32)
REGDEF_END(SDIO_DES_LINE2_REG)
#endif


#else
#define SDIO_DMA_START_ADDR_REG_OFS  0x110
#if NEW_UNION
union SDIO_DMA_START_ADDR_REG {
	uint32_t reg;
	struct {
	unsigned int DRAM_ADDR:31;
	unsigned int Reserved0:1;
	} bit;
};
#else
REGDEF_BEGIN(SDIO_DMA_START_ADDR_REG)
	REGDEF_BIT(DRAM_ADDR, 31)
	REGDEF_BIT(Reserved0, 1)
REGDEF_END(SDIO_DMA_START_ADDR_REG)
#endif

#define SDIO_DMA_CURR_ADDR_REG_OFS   0x114
#if NEW_UNION
union SDIO_DMA_CURR_ADDR_REG {
	uint32_t reg;
	struct {
	unsigned int DRAM_ADDR:31;
	unsigned int Reserved0:1;
	} bit;
};
#else
REGDEF_BEGIN(SDIO_DMA_CURR_ADDR_REG)
	REGDEF_BIT(DRAM_ADDR, 31)
	REGDEF_BIT(Reserved0, 1)
REGDEF_END(SDIO_DMA_CURR_ADDR_REG)
#endif

#endif

#endif
