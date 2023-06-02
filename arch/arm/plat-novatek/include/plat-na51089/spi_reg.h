
/*
	Register offset and bit definition for SPI module

	Register offset and bit definition for SPI module.

	@file       spi_reg.h
	@ingroup    
	@note

	Copyright   Novatek Microelectronics Corp. 2018.  All rights reserved
	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License version 2 as
	published by the Free Software Foundation.
*/
#ifndef _SPI_REG_H
#define _SPI_REG_H

#include <mach/rcw_macro.h>

#define SPI_GYRO_OP_INTERVAL_MAX        (0x1FFFFF)
#define SPI_GYRO_TRANSFER_INTERVAL_MAX  (0x1FFFFF)

#define SPI_GYRO_TRANSFER_MIN           (1)
#define SPI_GYRO_TRANSFER_MAX           (32)

#define SPI_GYRO_TRSLEN_MIN             (1)
#define SPI_GYRO_TRSLEN_MAX             (4)

#define SPI_GYRO_OPLEN_MIN              (1)
#define SPI_GYRO_OPLEN_MAX              (8)

#define SPI_GYRO_FIFO_THRESHOLD         (24)
#define SPI_GYRO_FIFO_DEPTH             (32)    // Max depth = 32 OP

#ifndef ENUM_DUMMY4WORD
#define ENUM_DUMMY4WORD(m)
#endif

/*
	SPI_LATCHDELAY_ENUM
*/
typedef enum
{
	SPI_LATCHDELAY_NORMAL,              //< Normal latch
	SPI_LATCHDELAY_1T,                  //< Delay latch 1T
	SPI_LATCHDELAY_2T,                  //< Delay latch 2T

	ENUM_DUMMY4WORD(SPI_LATCHDELAY_ENUM)
} SPI_LATCHDELAY_ENUM;

/*
	SPI_PKT_LEN_ENUM
*/
typedef enum
{
	SPI_PKT_LEN_ENUM_1BYTE,             //< 1 byte
	SPI_PKT_LEN_ENUM_2BYTES,            //< 2 byte

	ENUM_DUMMY4WORD(SPI_PKT_LEN_ENUM)
} SPI_PKT_LEN_ENUM;

/*
	SPI_PKT_CNT_ENUM
*/
typedef enum
{
	SPI_PKT_CNT_ENUM_1PKT,              //< 1 packet
	SPI_PKT_CNT_ENUM_2PKT,              //< 2 packet
	SPI_PKT_CNT_ENUM_3PKT,              //< 3 packet (reserved)
	SPI_PKT_CNT_ENUM_4PKT,              //< 4 packet

	ENUM_DUMMY4WORD(SPI_PKT_CNT_ENUM)
} SPI_PKT_CNT_ENUM;

/*
	SPI_GYRO_MODE_ENUM
*/
typedef enum
{
	SPI_GYRO_MODE_ENUM_SIETRIG,         //< SIE VD trigger mode
	SPI_GYRO_MODE_ENUM_ONESHOT,         //< f/w trigger one-shot mode
	SPI_GYRO_MODE_ENUM_FREERUN,         //< f/w trigger free run mode

	ENUM_DUMMY4WORD(SPI_GYRO_MODE_ENUM)
} SPI_GYRO_MODE_ENUM;

/*
	SPI_CS_ACT_LVL_ENUM
*/
typedef enum
{
	SPI_CS_ACT_LVL_LOW,                 //< low active
	SPI_CS_ACT_LVL_HIGH,                //< high active

	ENUM_DUMMY4WORD(SPI_CS_ACT_LVL_ENUM)
} SPI_CS_ACT_LVL_ENUM;

#define SPI_CTRL_REG_OFS	0x00
union SPI_CTRL_REG {		     /* SPI Control Register */
	uint32_t reg;
	struct {
	unsigned int spi_en:1;		/* S/W must to take care this bit when disable SPI */
	unsigned int spics_value:1;	/* Force the value of SPICS ,0:Force to low, 1:Force to high*/
	unsigned int spi_dma_en:1;	/* Start DMA mode */
	unsigned int spi_dma_dis:1;	/* Abort DMA mode */
	unsigned int spi_rdsts_en:1;	/* Start read flash status */
	unsigned int spi_rdsts_dis:1;	/* Abort read flash status */
	unsigned int spi_gyro_en:1;	/* Start GYRO mode */
	unsigned int spi_gyro_dis:1;	/* Abort GYRO mode */
	unsigned int reserved0:8;
	unsigned int spi_dma_sts:1;	/* DMA mode status */
	unsigned int spi_rdsts_sts:1;	/* Flash mode status */
	unsigned int spi_gyro_sts:1;	/* Gyro mode status */
	unsigned int reserved1:13;
	} bit;
};

#define SPI_IO_REG_OFS	0x04
union SPI_IO_REG {			/* SPI IO Register */
	uint32_t reg;
	struct {
	unsigned int spi_gyro_cs_pol:1;	/* CS polarity in Gyro mode */
	unsigned int reserved0:1;
	unsigned int spi_cpha:1;	/* SPI_CPHA Setting clock phase */
	unsigned int spi_cpol:1;	/* SPI_CPOL Setting clock polarity. */
	unsigned int reserved1:4;
	unsigned int spi_bus_width:2;	/* SPI bus width */
	unsigned int reserved2:2;
	unsigned int spi_io_out_en:1;	/* 0:SPI IO pins to input ,1:SPI IO pins to output */
	unsigned int spi_io_order:1;	/* setup bit order of IO0 IO1 IO2 IO3 */
	unsigned int reserved3:2;
	unsigned int spi_auto_io_out_en:1;	/* Set data pin to output mode when data is required to transmit */
	unsigned int reserved4:3;
	unsigned int spi_rdy_pol:1;	/* Polarity of SPI_RDY */
	unsigned int reserved5:11;
	} bit;
};

#define SPI_CONFIG_REG_OFS	0x08
union SPI_CONFIG_REG {   		/* SPI Configuration Register */
	uint32_t reg;
	struct {
	unsigned int spi_pkt_lsb_mode:1;/* LSb mode of each packet */
	unsigned int reserved0:1;
	unsigned int spi_pktlen:1;	/* spi_pktlen. 0: 1 byte, 1: 2 bytes */
	unsigned int reserved1:1;
	unsigned int spi_pkt_cnt:2;	/* packet count. 0: 1 pkt, 1: 2 pkt, , 2: 3 pkt, 3: 4 pkt */
	unsigned int spi_pkt_burst_handshake_en:1;	/* Enable handshake for each packet burst */
	unsigned int reserved2:5;
	unsigned int spi_gyro_mode:2;	/* 0: sie auto trig, 1: one-shot , 2: free run */
	unsigned int reserved3:2;
	unsigned int spi_pkt_burst_pre_cond:1;	/* Packet burst pre-condition */
	unsigned int reserved4:1;
	unsigned int spi_pkt_burst_post_cond:1;	/* Packet burst post-condition */
	unsigned int reserved5:13;
	} bit;
};

#define SPI_TIMING_REG_OFS	0x0C
union SPI_TIMING_REG {		/* SPI Timing Register */
	uint32_t reg;
	struct {
	unsigned int spi_cs_dly:8;	/* delay count from SPI_CS asserted to SPI_CLK toggle */
	unsigned int reserved0:8;
	unsigned int spi_post_cond_dly:13;	/* Packet burst post condition delay */
	unsigned int reserved1:3;
	} bit;
};

#define SPI_FLASH_CTRL_REG_OFS	0x10
union SPI_FLASH_CTRL_REG {		/* SPI Flash Control Register */
	uint32_t reg;
	struct {
	unsigned int spi_rdysts_bit:3;	/* Flash ready bit position */
	unsigned int reserved0:5;
	unsigned int spi_rdysts_val:1;	/* Flash ready value */
	unsigned int reserved1:7;
	unsigned int spi_rdsts_cmd:8;	/* Command byte to get status of serial flash */
	unsigned int reserved2:8;
	} bit;
};

#define SPI_DLY_CHAIN_REG_OFS	0x14
union SPI_DLY_CHAIN_REG {		/* SPI Delay Chain Register */
	uint32_t reg;
	struct {
	unsigned int dly_clk_sel:6;	/* delay how many delay cell to sample data */
	unsigned int reserved0:2;
	unsigned int dly_clk_polarity:1;
	unsigned int reserved1:1;
	unsigned int dly_clk_src:1;
	unsigned int reserved2:1;
	unsigned int latch_data_src:1;
	unsigned int reserved3:1;
	unsigned int dly_clk_edge:1;
	unsigned int reserved4:1;
	unsigned int latch_clk_shift:2;
	unsigned int reserved5:2;
	unsigned int latch_clk_edge:1;
	unsigned int reserved6:11;
	} bit;
};

#define SPI_STATUS_REG_OFS	0x18
union SPI_STATUS_REG {			/* SPI Status Register */
	uint32_t reg;
	struct {
	unsigned int spi_tdr_empty:1;	/* Transmit data register empty. */
	unsigned int spi_rdr_full:1;	/* Receive data register full. */
	unsigned int dma_abort_sts:1;
	unsigned int spi_dmaed:1;	/* 0: DMA not transfer/receive end , 1: DMA transfer/receive end */
	unsigned int spi_rdstsed:1;	/*0: SPI is reading status from serial flash or idle , 1: read SPI status done */
	unsigned int gyro_trs_rdy_sts:1;
	unsigned int gyro_overrun_sts:1;
	unsigned int all_gyrotrs_done_sts:1;
	unsigned int gyro_seq_err_sts:1;
	unsigned int gyro_trs_timeout_sts:1;
	unsigned int reserved0:22;
	} bit;
};

#define SPI_INTEN_REG_OFS	0x1C
union SPI_INTEN_REG {			/* SPI Status Register */
	uint32_t reg;
	struct {
	unsigned int spi_tdr_empty:1;	/* Transmit data register empty.  */
	unsigned int spi_rdr_full:1;	/* Receive data register full.  */
	unsigned int dma_abort_en:1;
	unsigned int spi_dmaed_en:1;	/* 0: DMA not transfer/receive end , 1: DMA transfer/receive end */
	unsigned int spi_rdstsed_en:1;	/* 0: SPI is reading status from serial flash or idle , 1: read SPI status done */
	unsigned int gyro_trs_rdy_en:1;
	unsigned int gyro_overrun_en:1;
	unsigned int all_gyrotrs_done_en:1;
	unsigned int gyro_seq_err_en:1;
	unsigned int gyro_trs_timeout_en:1;
	unsigned int reserved0:22;
	} bit;
};

#define SPI_RDR_REG_OFS	0x20
union SPI_RDR_REG {		/* SPI Receive Data Register */
	uint32_t reg;
	struct {
	unsigned int rdr:32;	/* Receive data register */
	} bit;
};

#define SPI_TDR_REG_OFS	0x24
union SPI_TDR_REG {		/* SPI Transmit Data Register */
	uint32_t reg;
	struct {
	unsigned int tdr:32;	/* Transmit data register */
	} bit;
};

/* 0x28, 0x2C: reserved */

#define SPI_DMA_CTRL_REG_OFS	0x30
union SPI_DMA_CTRL_REG {	/* SPI DMA Control Register */
	uint32_t reg;
	struct {
	unsigned int dma_dir:1;	/* 0: SPI2DMA, 1: DMA2SPI */
	unsigned int reserved0:31;
	} bit;
};

#define SPI_DMA_BUFSIZE_REG_OFS	0x34
union SPI_DMA_BUFSIZE_REG {			/* SPI DMA Control Register */
	uint32_t reg;
	struct {
	unsigned int spi_dma_bufsize:24;	/* DMA buffer size register in byte alignment */
	unsigned int reserved0:8;
	} bit;
};

#define SPI_DMA_STARTADDR_REG_OFS	0x38
union SPI_DMA_STARTADDR_REG {			/* SPI DMA Control Register */
	uint32_t reg;
	struct {
	unsigned int spi_dma_start_addr:30;	/* DMA DRAM starting address */
	unsigned int reserved0:2;
	} bit;
};

/* 0x3C: reserved */

#define SPI_GYROSEN_CONFIG_REG_OFS      0x40
union SPI_GYROSEN_CONFIG_REG {		/* SPI Gyro sensor Configuration Register */
	uint32_t reg;
	struct {
	unsigned int trscnt:5;		/* gyro transfer count per trigger */
	unsigned int reserved0:3;
	unsigned int trslen:2;		/* gyro transfer length: how many OPs */
	unsigned int reserved1:6;
	unsigned int len_op0:3;		/* OP0 byte length */
	unsigned int reserved2:1;
	unsigned int len_op1:3;		/* OP1 byte length */
	unsigned int reserved3:1;
	unsigned int len_op2:3;		/* OP2 byte length */
	unsigned int reserved4:1;
	unsigned int len_op3:3;		/* OP3 byte length */
	unsigned int reserved5:1;
	} bit;
};

#define SPI_GYROSEN_OP_INTERVAL_REG_OFS	0x44
union SPI_GYROSEN_OP_INTERVAL_REG {	/* SPI Gyro sensor OP interval Register */
	uint32_t reg;
	struct {
	unsigned int op_interval:21;
	unsigned int reserved0:11;
	} bit;
};

#define SPI_GYROSEN_TRS_INTERVAL_REG_OFS    0x48
union SPI_GYROSEN_TRS_INTERVAL_REG {	/* SPI Gyro sensor Transfer interval Register */
	uint32_t reg;
	struct {
	unsigned int trs_interval:21;
	unsigned int reserved0:11;
	} bit;
};

#define SPI_GYROSEN_FIFO_STS_REG_OFS    0x4C
union SPI_GYROSEN_FIFO_STS_REG {	/* SPI Gyro sensor Transfer interval Register */
	uint32_t reg;
	struct {
	unsigned int valid_entry:6;
	unsigned int reserved0:26;
	} bit;
};

#define SPI_GYROSEN_TRS_STS_REG_OFS     0x50
union SPI_GYROSEN_TRS_STS_REG {	/* SPI Gyro Transfer Status Register */
	uint32_t reg;
	struct {
	unsigned int trsSts_0:1;
	unsigned int trsSts_1:1;
	unsigned int trsSts_2:1;
	unsigned int trsSts_3:1;
	unsigned int trsSts_4:1;
	unsigned int trsSts_5:1;
	unsigned int trsSts_6:1;
	unsigned int trsSts_7:1;
	unsigned int trsSts_8:1;
	unsigned int trsSts_9:1;
	unsigned int trsSts_10:1;
	unsigned int trsSts_11:1;
	unsigned int trsSts_12:1;
	unsigned int trsSts_13:1;
	unsigned int trsSts_14:1;
	unsigned int trsSts_15:1;
	unsigned int trsSts_16:1;
	unsigned int trsSts_17:1;
	unsigned int trsSts_18:1;
	unsigned int trsSts_19:1;
	unsigned int trsSts_20:1;
	unsigned int trsSts_21:1;
	unsigned int trsSts_22:1;
	unsigned int trsSts_23:1;
	unsigned int trsSts_24:1;
	unsigned int trsSts_25:1;
	unsigned int trsSts_26:1;
	unsigned int trsSts_27:1;
	unsigned int trsSts_28:1;
	unsigned int trsSts_29:1;
	unsigned int trsSts_30:1;
	unsigned int trsSts_31:1;
	} bit;
};

#define SPI_GYROSEN_TRS_INTEN_REG_OFS	0x54
union SPI_GYROSEN_TRS_INTEN_REG {	/* SPI Gyro Transfer Interrupt Enable Register */
	uint32_t reg;
	struct {
	unsigned int trsSts_0:1;
	unsigned int trsSts_1:1;
	unsigned int trsSts_2:1;
	unsigned int trsSts_3:1;
	unsigned int trsSts_4:1;
	unsigned int trsSts_5:1;
	unsigned int trsSts_6:1;
	unsigned int trsSts_7:1;
	unsigned int trsSts_8:1;
	unsigned int trsSts_9:1;
	unsigned int trsSts_10:1;
	unsigned int trsSts_11:1;
	unsigned int trsSts_12:1;
	unsigned int trsSts_13:1;
	unsigned int trsSts_14:1;
	unsigned int trsSts_15:1;
	unsigned int trsSts_16:1;
	unsigned int trsSts_17:1;
	unsigned int trsSts_18:1;
	unsigned int trsSts_19:1;
	unsigned int trsSts_20:1;
	unsigned int trsSts_21:1;
	unsigned int trsSts_22:1;
	unsigned int trsSts_23:1;
	unsigned int trsSts_24:1;
	unsigned int trsSts_25:1;
	unsigned int trsSts_26:1;
	unsigned int trsSts_27:1;
	unsigned int trsSts_28:1;
	unsigned int trsSts_29:1;
	unsigned int trsSts_30:1;
	unsigned int trsSts_31:1;
	} bit;
};

#define SPI_GYROSEN_COUNTER_REG_OFS	0x58
union SPI_GYROSEN_COUNTER_REG {		/* SPI Gyro sensor Transfer and OP Counter Register */
	uint32_t reg;
	struct {
	unsigned int transfer_counter:5;
	unsigned int reserved0:3;
	unsigned int op_counter:2;
	unsigned int reserved1:22;
	} bit;
};

#define SPI_GYROSEN_RX_DATA_REG_OFS	0x5C
union SPI_GYROSEN_RX_DATA_REG {		/* SPI Gyro sensor Receive Data Register */
	uint32_t reg;
	struct {
	unsigned int rxdata:32;
	} bit;
};

#define SPI_GYROSEN_TX_DATA1_REG_OFS	0x60
union SPI_GYROSEN_TX_DATA1_REG {	/* SPI Gyro sensor Transfer Data Register 1 */
	uint32_t reg;
	struct {
	unsigned int op0_lsb:32;
	} bit;
};

#define SPI_GYROSEN_TX_DATA2_REG_OFS	0x64
union SPI_GYROSEN_TX_DATA2_REG {	/* SPI Gyro sensor Transfer Data Register 2 */
	uint32_t reg;
	struct {
	unsigned int op0_msb:32;
	} bit;
};

/* 0x68, 0x6C: reserved */

#define SPI_GYROSEN_TX_DATA3_REG_OFS	0x70
union SPI_GYROSEN_TX_DATA3_REG {	/* SPI Gyro sensor Transfer Data Register 3 */
	uint32_t reg;
	struct {
	unsigned int op1_lsb:32;
	} bit;
};

#define SPI_GYROSEN_TX_DATA4_REG_OFS	0x74
union SPI_GYROSEN_TX_DATA4_REG {	/* SPI Gyro sensor Transfer Data Register 4 */
	uint32_t reg;
	struct {
	unsigned int op1_msb:32;
	} bit;
};

/* 0x78, 0x7C: reserved */

#define SPI_GYROSEN_TX_DATA5_REG_OFS	0x80
union SPI_GYROSEN_TX_DATA5_REG {	/* SPI Gyro sensor Transfer Data Register 5 */
	uint32_t reg;
	struct {
	unsigned int op2_lsb:32;
	} bit;
};

#define SPI_GYROSEN_TX_DATA6_REG_OFS	0x84
union SPI_GYROSEN_TX_DATA6_REG {	/* SPI Gyro sensor Transfer Data Register 6 */
	uint32_t reg;
	struct {
	unsigned int op2_msb:32;
	} bit;
};

/* 0x88, 0x8C: reserved */

#define SPI_GYROSEN_TX_DATA7_REG_OFS	0x90
union SPI_GYROSEN_TX_DATA7_REG {	/* SPI Gyro sensor Transfer Data Register 7 */
	uint32_t reg;
	struct {
	unsigned int op3_lsb:32;
	} bit;
};

#define SPI_GYROSEN_TX_DATA8_REG_OFS	0x94
union SPI_GYROSEN_TX_DATA8_REG {	/* SPI Gyro sensor Transfer Data Register 8 */
	uint32_t reg;
	struct {
	unsigned int op3_msb:32;
	} bit;
};
#endif
