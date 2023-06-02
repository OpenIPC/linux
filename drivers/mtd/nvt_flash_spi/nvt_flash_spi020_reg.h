/*
    @file	nand_reg.h
    @ingroup	mICardSMC

    @brief	Nand flash / Smartmedia Card driver register definition

    @note

    @version	V1.00.000
    @author	Cliff Lin
    @date	2009/05/19

    Copyright	Novatek Microelectronics Corp. 2009.  All rights reserved.

*/

#ifndef _NAND_REG_H
#define _NAND_REG_H

#include <mach/rcw_macro.h>
#include <mach/nvt_type.h>

/*
    SPI_LATCHDELAY_ENUM
*/
typedef enum {
	ADDR_LEN_NONE,                      //< No flash address state
	ADDR_LEN_1BYTE,                     //< 1 byte
	ADDR_LEN_2BYTES,                    //< 2 bytes
	ADDR_LEN_3BYTES,                    //< 3 bytes
	ADDR_LEN_4BYTES,                    //< 4 bytes

	ENUM_DUMMY4WORD(SPI_LATCHDELAY_ENUM)
} ADDR_LEN_ENUM;

/*
    RD_STS_MODE_ENUM
*/
typedef enum {
	RD_STS_MODE_BY_HW,
	RD_STS_MODE_BY_SW,

	ENUM_DUMMY4WORD(RD_STS_MODE_ENUM)
} RD_STS_MODE_ENUM;

#define DUMMY_CYCLES_MAX            (32)

#define MAX_FIFO_DEPTH              (32)    // unit: byte

#define CMD_QUEUE_1ST_REG_OFS       0x00
union CMD_QUEUE_1ST_REG {
	uint32_t reg;
	struct {
	unsigned int FLASH_ADDR:32;
	} bit;
};

#define CMD_QUEUE_2ND_REG_OFS       0x04
union CMD_QUEUE_2ND_REG {
	uint32_t reg;
	struct {
	unsigned int ADDR_LEN:3;
	unsigned int reserved0:13;
	unsigned int DUMMY_CYCLES:8;
	unsigned int INSTRUCTION_LEN:2;
	unsigned int reserved1:2;
	unsigned int CONTI_MODE_EN:1;
	unsigned int reserved2:3;
	} bit;
};

#define CMD_QUEUE_3RD_REG_OFS       0x08
union CMD_QUEUE_3RD_REG {
	uint32_t reg;
	struct {
	unsigned int DATA_COUNTER:32;
	} bit;
};

#define CMD_QUEUE_4TH_REG_OFS       0x0C
union CMD_QUEUE_4TH_REG {
	uint32_t reg;
	struct {
	unsigned int reserved0:1;
	unsigned int WRITE_EN:1;
	unsigned int RD_STS_EN:1;
	unsigned int RD_STS_MODE:1;
	unsigned int DTR_MODE:1;
	unsigned int SPI_OPERATE_MODE:3;
	unsigned int CS_SEL:2;
	unsigned int reserved1:6;
	unsigned int CONTI_MODE_CODE:8;
	unsigned int INSTRUCTION_CODE:8;
	} bit;
};

#define FLASH_CONTROL_REG_OFS       0x10
union FLASH_CONTROL_REG {
	uint32_t reg;
	struct {
	unsigned int CLK_DIV:2;
	unsigned int reserved0:2;
	unsigned int SPI_MODE:1;
	unsigned int reserved1:3;
	unsigned int ABORT:1;
	unsigned int reserved2:7;
	unsigned int RD_STS_LOC:3;
	unsigned int reserved3:1;
	unsigned int DRAM_PORT_SEL:1;
	unsigned int reserved4:11;
	} bit;
};

#define FLASH_AC_TIMING_REG_OFS     0x14
union FLASH_AC_TIMING_REG {
	uint32_t reg;
	struct {
	unsigned int CS_DELAY:4;
	unsigned int reserved0:28;
	} bit;
};

#define FLASH_STATUS_REG_OFS        0x18
union FLASH_STATUS_REG {
	uint32_t reg;
	struct {
	unsigned int TX_FIFO_EMPTY:1;
	unsigned int RX_FIFO_FULL:1;
	unsigned int reserved0:30;
	} bit;
};

#define INT_CTRL_REG_OFS            0x20
union INT_CTRL_REG {
	uint32_t reg;
	struct {
	unsigned int DMA_EN:1;
	unsigned int INT_EN:1;
	unsigned int reserved0:6;
	unsigned int TXFIFO_THOD:2;
	unsigned int reserved1:2;
	unsigned int RXFIFO_THOD:2;
	unsigned int reserved2:18;
	} bit;
};

#define INT_STS_REG_OFS             0x24
union INT_STS_REG {
	uint32_t reg;
	struct {
	unsigned int COMPLETE_STS:1;
	unsigned int reserved0:31;
	} bit;
};

#define FLASH_READ_STS_REG_OFS      0x28
union FLASH_READ_STS_REG {
	uint32_t reg;
	struct {
	unsigned int READ_STATUS:8;
	unsigned int reserved0:24;
	} bit;
};

#define DLY_CHAIN_REG_OFS           0x60
union DLY_CHAIN_REG {
	uint32_t reg;
	struct {
	unsigned int CLK_IN_DLY:4;
	unsigned int reserved0:4;
	unsigned int DATA_IN_DLY:4;
	unsigned int reserved1:20;
	} bit;
};

#define CHKSUM_CTRL_REG_OFS         0x64
union CHKSUM_CTRL_REG {
	uint32_t reg;
	struct {
	unsigned int WFIFO_CHKSUM_EN:1;
	unsigned int reserved0:3;
	unsigned int WFIFO_CHKSUM_CLR:1;
	unsigned int reserved1:3;
	unsigned int RFIFO_CHKSUM_EN:1;
	unsigned int reserved2:3;
	unsigned int RFIFO_CHKSUM_CLR:1;
	unsigned int reserved3:19;
	} bit;
};

#define WFIFO_CHKSUM_REG_OFS        0x68
union WFIFO_CHKSUM_REG {
	uint32_t reg;
	struct {
	unsigned int CHKSUM:32;
	} bit;
};

#define RFIFO_CHKSUM_REG_OFS        0x6C
union RFIFO_CHKSUM_REG {
	uint32_t reg;
	struct {
	unsigned int CHKSUM:32;
	} bit;
};

#define FLASH_DATA_PORT_REG_OFS     0x100
union FLASH_DATA_PORT_REG {
	uint32_t reg;
	struct {
	unsigned int DATA:32;
	} bit;
};
#endif

