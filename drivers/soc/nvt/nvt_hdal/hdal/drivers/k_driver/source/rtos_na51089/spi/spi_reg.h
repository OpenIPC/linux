#ifndef _SPI_REG_H
#define _SPI_REG_H

//#include "DrvCommon.h"
#include <rcw_macro.h>

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

/*
    SPI_LATCHDELAY_ENUM
*/
typedef enum {
	SPI_LATCHDELAY_NORMAL,              //< Normal latch
	SPI_LATCHDELAY_0P5T,                //< Delay latch 0.5T
	SPI_LATCHDELAY_1T,                  //< Delay latch 1T
	SPI_LATCHDELAY_1P5T,                //< Delay latch 1.5T

	ENUM_DUMMY4WORD(SPI_LATCHDELAY_ENUM)
} SPI_LATCHDELAY_ENUM;

/*
    SPI_PKT_LEN_ENUM
*/
typedef enum {
	SPI_PKT_LEN_ENUM_1BYTE,             //< 1 byte
	SPI_PKT_LEN_ENUM_2BYTES,            //< 2 byte

	ENUM_DUMMY4WORD(SPI_PKT_LEN_ENUM)
} SPI_PKT_LEN_ENUM;

/*
    SPI_PKT_CNT_ENUM
*/
typedef enum {
	SPI_PKT_CNT_ENUM_1PKT,              //< 1 packet
	SPI_PKT_CNT_ENUM_2PKT,              //< 2 packet
	SPI_PKT_CNT_ENUM_3PKT,              //< 3 packet (reserved)
	SPI_PKT_CNT_ENUM_4PKT,              //< 4 packet

	ENUM_DUMMY4WORD(SPI_PKT_CNT_ENUM)
} SPI_PKT_CNT_ENUM;

/*
    SPI_GYRO_MODE_ENUM
*/
typedef enum {
	SPI_GYRO_MODE_ENUM_NONE,            //< none
	SPI_GYRO_MODE_ENUM_SIETRIG,         //< SIE VD trigger mode
	SPI_GYRO_MODE_ENUM_ONESHOT,         //< f/w trigger one-shot mode
	SPI_GYRO_MODE_ENUM_FREERUN,         //< f/w trigger free run mode

	ENUM_DUMMY4WORD(SPI_GYRO_MODE_ENUM)
} SPI_GYRO_MODE_ENUM;

/*
    SPI_CS_ACT_LVL_ENUM
*/
typedef enum {
	SPI_CS_ACT_LVL_LOW,                 //< low active
	SPI_CS_ACT_LVL_HIGH,                //< high active

	ENUM_DUMMY4WORD(SPI_CS_ACT_LVL_ENUM)
} SPI_CS_ACT_LVL_ENUM;

#define SPI_CTRL_REG_OFS    0x00
REGDEF_BEGIN(SPI_CTRL_REG)      /* SPI Control Register */
REGDEF_BIT(spi_en, 1)       /* S/W must to take care this bit when disable SPI */
REGDEF_BIT(spics_value, 1)  /* Force the value of SPICS ,0:Force to low, 1:Force to high*/
REGDEF_BIT(spi_dma_en, 1)   /* Start DMA mode */
REGDEF_BIT(spi_dma_dis, 1)  /* Abort DMA mode */
REGDEF_BIT(spi_rdsts_en, 1) /* Start read flash status */
REGDEF_BIT(spi_rdsts_dis, 1) /* Abort read flash status */
REGDEF_BIT(spi_gyro_en, 1)  /* Start GYRO mode */
REGDEF_BIT(spi_gyro_dis, 1) /* Abort GYRO mode */
REGDEF_BIT(spi_gyro_update, 1) /* Update GYRO settings */
REGDEF_BIT(, 7)
REGDEF_BIT(spi_dma_sts, 1)  /* DMA mode status */
REGDEF_BIT(spi_rdsts_sts, 1) /* Flash mode status */
REGDEF_BIT(spi_gyro_sts, 1) /* Gyro mode status */
REGDEF_BIT(, 13)
REGDEF_END(SPI_CTRL_REG)        /* SPI Control Register end */

#define SPI_IO_REG_OFS      0x04
REGDEF_BEGIN(SPI_IO_REG)        /* SPI IO Register */
REGDEF_BIT(spi_gyro_cs_pol, 1)  /* CS polarity in Gyro mode */
REGDEF_BIT(, 1)
REGDEF_BIT(spi_cpha, 1)     /* SPI_CPHA Setting clock phase */
REGDEF_BIT(spi_cpol, 1)     /* SPI_CPOL Setting clock polarity. */
REGDEF_BIT(, 4)
REGDEF_BIT(spi_bus_width, 2) /* SPI bus width */
REGDEF_BIT(, 2)
REGDEF_BIT(spi_io_out_en, 1) /* 0:SPI IO pins to input ,1:SPI IO pins to output */
REGDEF_BIT(spi_io_order, 1) /* setup bit order of IO0 IO1 IO2 IO3 */
REGDEF_BIT(, 2)
REGDEF_BIT(spi_auto_io_out_en, 1)   /* Set data pin to output mode when data is required to transmit */
REGDEF_BIT(, 3)
REGDEF_BIT(spi_rdy_pol, 1)  /* Polarity of SPI_RDY */
REGDEF_BIT(, 11)
REGDEF_END(SPI_IO_REG)          /* SPI IO Register end */

#define SPI_CONFIG_REG_OFS  0x08
REGDEF_BEGIN(SPI_CONFIG_REG)    /* SPI Configuration Register */
REGDEF_BIT(spi_pkt_lsb_mode, 1) /* LSb mode of each packet */
REGDEF_BIT(, 1)
REGDEF_BIT(spi_pktlen, 1)   /* spi_pktlen. 0: 1 byte, 1: 2 bytes */
REGDEF_BIT(, 1)
REGDEF_BIT(spi_pkt_cnt, 2)  /* packet count. 0: 1 pkt, 1: 2 pkt, , 2: 3 pkt, 3: 4 pkt */
REGDEF_BIT(spi_pkt_burst_handshake_en, 1)   /* Enable handshake for each packet burst */
REGDEF_BIT(, 5)
REGDEF_BIT(spi_gyro_mode, 2) /* 0: sie auto trig, 1: one-shot , 2: free run */
REGDEF_BIT(, 2)
REGDEF_BIT(spi_pkt_burst_pre_cond, 1)   /* Packet burst pre-condition */
REGDEF_BIT(, 1)
REGDEF_BIT(spi_pkt_burst_post_cond, 1)  /* Packet burst post-condition */
REGDEF_BIT(, 13)
REGDEF_END(SPI_CONFIG_REG)      /* SPI Configuration Register end */

#define SPI_TIMING_REG_OFS  0x0C
REGDEF_BEGIN(SPI_TIMING_REG)    /* SPI Timing Register */
REGDEF_BIT(spi_cs_dly, 8)   /* delay count from SPI_CS asserted to SPI_CLK toggle */
REGDEF_BIT(, 8)
REGDEF_BIT(spi_post_cond_dly, 13)   /* Packet burst post condition delay */
REGDEF_BIT(, 3)
REGDEF_END(SPI_TIMING_REG)      /* SPI Timing Register end  */

#define SPI_FLASH_CTRL_REG_OFS  0x10
REGDEF_BEGIN(SPI_FLASH_CTRL_REG)/* SPI Flash Control Register */
REGDEF_BIT(spi_rdysts_bit, 3) /* Flash ready bit position */
REGDEF_BIT(, 5)
REGDEF_BIT(spi_rdysts_val, 1) /* Flash ready value */
REGDEF_BIT(, 7)
REGDEF_BIT(spi_rdsts_cmd, 8) /* Command byte to get status of serial flash */
REGDEF_BIT(, 8)
REGDEF_END(SPI_FLASH_CTRL_REG)  /* SPI Flash Control Register end */

#define SPI_DLY_CHAIN_REG_OFS   0x14
REGDEF_BEGIN(SPI_DLY_CHAIN_REG) /* SPI Delay Chain Register */
REGDEF_BIT(dly_clk_sel, 6)  /* delay how many delay cell to sample data */
REGDEF_BIT(, 2)
REGDEF_BIT(dly_clk_polarity, 1)
REGDEF_BIT(, 1)
REGDEF_BIT(dly_clk_src, 1)
REGDEF_BIT(, 1)
REGDEF_BIT(latch_data_src, 1)
REGDEF_BIT(, 1)
REGDEF_BIT(dly_clk_edge, 1)
REGDEF_BIT(, 1)
REGDEF_BIT(latch_clk_shift, 2)
REGDEF_BIT(, 2)
REGDEF_BIT(latch_clk_edge, 1)
REGDEF_BIT(, 11)
REGDEF_END(SPI_DLY_CHAIN_REG)   /* SPI Delay Chain Register end  */

#define SPI_STATUS_REG_OFS  0x18
REGDEF_BEGIN(SPI_STATUS_REG)    /* SPI Status Register */
REGDEF_BIT(spi_tdr_empty, 1) /* Transmit data register empty.  */
REGDEF_BIT(spi_rdr_full, 1) /* Receive data register full.  */
REGDEF_BIT(dma_abort_sts, 1)
REGDEF_BIT(spi_dmaed, 1)    /* 0: DMA not transfer/receive end , 1: DMA transfer/receive end */
REGDEF_BIT(spi_rdstsed, 1)  /*0: SPI is reading status from serial flash or idle , 1: read SPI status done */
REGDEF_BIT(gyro_trs_rdy_sts, 1)
REGDEF_BIT(gyro_overrun_sts, 1)
REGDEF_BIT(all_gyrotrs_done_sts, 1)
REGDEF_BIT(gyro_seq_err_sts, 1)
REGDEF_BIT(gyro_trs_timeout_sts, 1)
REGDEF_BIT(, 22)
REGDEF_END(SPI_STATUS_REG)      /* SPI Status Register end */

#define SPI_INTEN_REG_OFS   0x1C
REGDEF_BEGIN(SPI_INTEN_REG)     /* SPI Status Register */
REGDEF_BIT(spi_tdr_empty, 1) /* Transmit data register empty.  */
REGDEF_BIT(spi_rdr_full, 1) /* Receive data register full.  */
REGDEF_BIT(dma_abort_en, 1)
REGDEF_BIT(spi_dmaed_en, 1) /* 0: DMA not transfer/receive end , 1: DMA transfer/receive end */
REGDEF_BIT(spi_rdstsed_en, 1)   /*0: SPI is reading status from serial flash or idle , 1: read SPI status done */
REGDEF_BIT(gyro_trs_rdy_en, 1)
REGDEF_BIT(gyro_overrun_en, 1)
REGDEF_BIT(all_gyrotrs_done_en, 1)
REGDEF_BIT(gyro_seq_err_en, 1)
REGDEF_BIT(gyro_trs_timeout_en, 1)
REGDEF_BIT(, 22)
REGDEF_END(SPI_INTEN_REG)       /* SPI Status Register end */

#define SPI_RDR_REG_OFS     0x20
REGDEF_BEGIN(SPI_RDR_REG)       /* SPI Receive Data Register */
REGDEF_BIT(rdr, 32)         /* Receive data register */
REGDEF_END(SPI_RDR_REG)         /* SPI Receive Data Register end  */

#define SPI_TDR_REG_OFS     0x24
REGDEF_BEGIN(SPI_TDR_REG)       /* SPI Transmit Data Register */
REGDEF_BIT(tdr, 32)         /* Transmit data register */
REGDEF_END(SPI_TDR_REG)         /* SPI Transmit Data Register end  */

/* 0x28, 0x2C: reserved */

#define SPI_DMA_CTRL_REG_OFS    0x30
REGDEF_BEGIN(SPI_DMA_CTRL_REG)  /* SPI DMA Control Register */
REGDEF_BIT(dma_dir, 1)      /* 0: SPI2DMA, 1: DMA2SPI */
REGDEF_BIT(, 31)
REGDEF_END(SPI_DMA_CTRL_REG)    /* SPI DMA Control Register end  */

#define SPI_DMA_BUFSIZE_REG_OFS 0x34
REGDEF_BEGIN(SPI_DMA_BUFSIZE_REG)   /* SPI DMA Control Register */
REGDEF_BIT(spi_dma_bufsize, 24) /* DMA buffer size register in byte alignment */
REGDEF_BIT(, 8)
REGDEF_END(SPI_DMA_BUFSIZE_REG) /* SPI DMA Control Register end  */

#define SPI_DMA_STARTADDR_REG_OFS   0x38
REGDEF_BEGIN(SPI_DMA_STARTADDR_REG) /* SPI DMA Control Register */
REGDEF_BIT(spi_dma_start_addr, 32) /* DMA DRAM starting address */
REGDEF_END(SPI_DMA_STARTADDR_REG)   /* SPI DMA Control Register end  */

/* 0x3C: reserved */

#define SPI_GYROSEN_CONFIG_REG_OFS      0x40
REGDEF_BEGIN(SPI_GYROSEN_CONFIG_REG)    /* SPI Gyro sensor Configuration Register */
REGDEF_BIT(trscnt, 5)               /* gyro transfer count per trigger */
REGDEF_BIT(rsv, 3)
REGDEF_BIT(trslen, 2)               /* gyro transfer length: how many OPs */
REGDEF_BIT(rsv2, 2)
REGDEF_BIT(gyro_vdsrc, 2)
REGDEF_BIT(, 2)
REGDEF_BIT(len_op0, 3)              /* OP0 byte length */
REGDEF_BIT(rsv3, 1)
REGDEF_BIT(len_op1, 3)              /* OP1 byte length */
REGDEF_BIT(rsv4, 1)
REGDEF_BIT(len_op2, 3)              /* OP2 byte length */
REGDEF_BIT(rsv5, 1)
REGDEF_BIT(len_op3, 3)              /* OP3 byte length */
REGDEF_BIT(rsv6, 1)
REGDEF_END(SPI_GYROSEN_CONFIG_REG)          /* SPI Gyro sensor Configuration Register end  */

#define SPI_GYROSEN_OP_INTERVAL_REG_OFS 0x44
REGDEF_BEGIN(SPI_GYROSEN_OP_INTERVAL_REG)   /* SPI Gyro sensor OP interval Register */
REGDEF_BIT(op_interval, 21)
REGDEF_BIT(rsv, 11)
REGDEF_END(SPI_GYROSEN_OP_INTERVAL_REG)     /* SPI Gyro sensor OP interval Register  */

#define SPI_GYROSEN_TRS_INTERVAL_REG_OFS    0x48
REGDEF_BEGIN(SPI_GYROSEN_TRS_INTERVAL_REG)  /* SPI Gyro sensor Transfer interval Register */
REGDEF_BIT(trs_interval, 21)
REGDEF_BIT(rsv, 11)
REGDEF_END(SPI_GYROSEN_TRS_INTERVAL_REG)    /* SPI Gyro sensor Transfer interval Register  */

#define SPI_GYROSEN_FIFO_STS_REG_OFS    0x4C
REGDEF_BEGIN(SPI_GYROSEN_FIFO_STS_REG)      /* SPI Gyro sensor Transfer interval Register */
REGDEF_BIT(valid_entry, 6)
REGDEF_BIT(rsv, 26)
REGDEF_END(SPI_GYROSEN_FIFO_STS_REG)        /* SPI Gyro sensor Transfer interval Register  */

#define SPI_GYROSEN_TRS_STS_REG_OFS     0x50
REGDEF_BEGIN(SPI_GYROSEN_TRS_STS_REG)       /* SPI Gyro Transfer Status Register */
REGDEF_BIT(trsSts_0, 1)
REGDEF_BIT(trsSts_1, 1)
REGDEF_BIT(trsSts_2, 1)
REGDEF_BIT(trsSts_3, 1)
REGDEF_BIT(trsSts_4, 1)
REGDEF_BIT(trsSts_5, 1)
REGDEF_BIT(trsSts_6, 1)
REGDEF_BIT(trsSts_7, 1)
REGDEF_BIT(trsSts_8, 1)
REGDEF_BIT(trsSts_9, 1)
REGDEF_BIT(trsSts_10, 1)
REGDEF_BIT(trsSts_11, 1)
REGDEF_BIT(trsSts_12, 1)
REGDEF_BIT(trsSts_13, 1)
REGDEF_BIT(trsSts_14, 1)
REGDEF_BIT(trsSts_15, 1)
REGDEF_BIT(trsSts_16, 1)
REGDEF_BIT(trsSts_17, 1)
REGDEF_BIT(trsSts_18, 1)
REGDEF_BIT(trsSts_19, 1)
REGDEF_BIT(trsSts_20, 1)
REGDEF_BIT(trsSts_21, 1)
REGDEF_BIT(trsSts_22, 1)
REGDEF_BIT(trsSts_23, 1)
REGDEF_BIT(trsSts_24, 1)
REGDEF_BIT(trsSts_25, 1)
REGDEF_BIT(trsSts_26, 1)
REGDEF_BIT(trsSts_27, 1)
REGDEF_BIT(trsSts_28, 1)
REGDEF_BIT(trsSts_29, 1)
REGDEF_BIT(trsSts_30, 1)
REGDEF_BIT(trsSts_31, 1)
REGDEF_END(SPI_GYROSEN_TRS_STS_REG)         /* SPI Gyro Transfer Status Register */

#define SPI_GYROSEN_TRS_INTEN_REG_OFS        0x54
REGDEF_BEGIN(SPI_GYROSEN_TRS_INTEN_REG)      /* SPI Gyro Transfer Interrupt Enable Register */
REGDEF_BIT(trsSts_0, 1)
REGDEF_BIT(trsSts_1, 1)
REGDEF_BIT(trsSts_2, 1)
REGDEF_BIT(trsSts_3, 1)
REGDEF_BIT(trsSts_4, 1)
REGDEF_BIT(trsSts_5, 1)
REGDEF_BIT(trsSts_6, 1)
REGDEF_BIT(trsSts_7, 1)
REGDEF_BIT(trsSts_8, 1)
REGDEF_BIT(trsSts_9, 1)
REGDEF_BIT(trsSts_10, 1)
REGDEF_BIT(trsSts_11, 1)
REGDEF_BIT(trsSts_12, 1)
REGDEF_BIT(trsSts_13, 1)
REGDEF_BIT(trsSts_14, 1)
REGDEF_BIT(trsSts_15, 1)
REGDEF_BIT(trsSts_16, 1)
REGDEF_BIT(trsSts_17, 1)
REGDEF_BIT(trsSts_18, 1)
REGDEF_BIT(trsSts_19, 1)
REGDEF_BIT(trsSts_20, 1)
REGDEF_BIT(trsSts_21, 1)
REGDEF_BIT(trsSts_22, 1)
REGDEF_BIT(trsSts_23, 1)
REGDEF_BIT(trsSts_24, 1)
REGDEF_BIT(trsSts_25, 1)
REGDEF_BIT(trsSts_26, 1)
REGDEF_BIT(trsSts_27, 1)
REGDEF_BIT(trsSts_28, 1)
REGDEF_BIT(trsSts_29, 1)
REGDEF_BIT(trsSts_30, 1)
REGDEF_BIT(trsSts_31, 1)
REGDEF_END(SPI_GYROSEN_TRS_INTEN_REG)        /* SPI Gyro Transfer Interrupt Enable Register */

#define SPI_GYROSEN_COUNTER_REG_OFS         0x58
REGDEF_BEGIN(SPI_GYROSEN_COUNTER_REG)       /* SPI Gyro sensor Transfer and OP Counter Register */
REGDEF_BIT(transfer_counter, 5)
REGDEF_BIT(rsv, 3)
REGDEF_BIT(op_counter, 2)
REGDEF_BIT(rsv2, 22)
REGDEF_END(SPI_GYROSEN_COUNTER_REG)         /* SPI Gyro sensor Transfer and OP Counter Register */

#define SPI_GYROSEN_RX_DATA_REG_OFS         0x5C
REGDEF_BEGIN(SPI_GYROSEN_RX_DATA_REG)       /* SPI Gyro sensor Receive Data Register */
REGDEF_BIT(rxdata, 32)
REGDEF_END(SPI_GYROSEN_RX_DATA_REG)         /* SPI Gyro sensor Receive Data Register */

#define SPI_GYROSEN_TX_DATA1_REG_OFS        0x60
REGDEF_BEGIN(SPI_GYROSEN_TX_DATA1_REG)      /* SPI Gyro sensor Transfer Data Register 1 */
REGDEF_BIT(op0_lsb, 32)
REGDEF_END(SPI_GYROSEN_TX_DATA1_REG)        /* SPI Gyro sensor Transfer Data Register 1 */

#define SPI_GYROSEN_TX_DATA2_REG_OFS        0x64
REGDEF_BEGIN(SPI_GYROSEN_TX_DATA2_REG)      /* SPI Gyro sensor Transfer Data Register 2 */
REGDEF_BIT(op0_msb, 32)
REGDEF_END(SPI_GYROSEN_TX_DATA2_REG)        /* SPI Gyro sensor Transfer Data Register 2 */

/* 0x68, 0x6C: reserved */

#define SPI_GYROSEN_TX_DATA3_REG_OFS        0x70
REGDEF_BEGIN(SPI_GYROSEN_TX_DATA3_REG)      /* SPI Gyro sensor Transfer Data Register 3 */
REGDEF_BIT(op1_lsb, 32)
REGDEF_END(SPI_GYROSEN_TX_DATA3_REG)        /* SPI Gyro sensor Transfer Data Register 3 */

#define SPI_GYROSEN_TX_DATA4_REG_OFS        0x74
REGDEF_BEGIN(SPI_GYROSEN_TX_DATA4_REG)      /* SPI Gyro sensor Transfer Data Register 4 */
REGDEF_BIT(op1_msb, 32)
REGDEF_END(SPI_GYROSEN_TX_DATA4_REG)        /* SPI Gyro sensor Transfer Data Register 4 */

/* 0x78, 0x7C: reserved */

#define SPI_GYROSEN_TX_DATA5_REG_OFS        0x80
REGDEF_BEGIN(SPI_GYROSEN_TX_DATA5_REG)      /* SPI Gyro sensor Transfer Data Register 5 */
REGDEF_BIT(op2_lsb, 32)
REGDEF_END(SPI_GYROSEN_TX_DATA5_REG)        /* SPI Gyro sensor Transfer Data Register 5 */

#define SPI_GYROSEN_TX_DATA6_REG_OFS        0x84
REGDEF_BEGIN(SPI_GYROSEN_TX_DATA6_REG)      /* SPI Gyro sensor Transfer Data Register 6 */
REGDEF_BIT(op2_msb, 32)
REGDEF_END(SPI_GYROSEN_TX_DATA6_REG)        /* SPI Gyro sensor Transfer Data Register 6 */

/* 0x88, 0x8C: reserved */

#define SPI_GYROSEN_TX_DATA7_REG_OFS        0x90
REGDEF_BEGIN(SPI_GYROSEN_TX_DATA7_REG)      /* SPI Gyro sensor Transfer Data Register 7 */
REGDEF_BIT(op3_lsb, 32)
REGDEF_END(SPI_GYROSEN_TX_DATA7_REG)        /* SPI Gyro sensor Transfer Data Register 7 */

#define SPI_GYROSEN_TX_DATA8_REG_OFS        0x94
REGDEF_BEGIN(SPI_GYROSEN_TX_DATA8_REG)      /* SPI Gyro sensor Transfer Data Register 8 */
REGDEF_BIT(op3_msb, 32)
REGDEF_END(SPI_GYROSEN_TX_DATA8_REG)        /* SPI Gyro sensor Transfer Data Register 8 */


#endif
