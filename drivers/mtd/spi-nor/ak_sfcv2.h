#ifndef _AK_SFCV2_H
#define _AK_SFCV2_H

//registers
#define SFC_REG_BASE_ADDR             (0x20120000)
#define SFC_REG_XIP_CFG(i)            (0x40 + (0x10 * i))
#define SFC_REG_XIP_SIZE(i)           (0x48 + (0x10 * i))
#define SFC_REG_XIP_ADDR_OFFSET(i)    (0x4c + (0x10 * i))
#define SFC_REG_FIFO_STATUS           (0xc0)
#define SFC_REG_STATUS                (0xd0)
#define SFC_REG_INT_MASK              (0xd4)
#define SFC_REG_WORK_MODE             (0x100)
#define SFC_REG_CS_TIMING             (0x104)
#define SFC_REG_BAUD_RATE             (0x108) //此寄存器的相邻写操作应间隔1us及以上时间
#define SFC_REG_WAIT_PHASE            (0x10c)
#define SFC_REG_XIP_TIMEOUT           (0x110)
#define SFC_REG_RXDS_CFG              (0x11c)
#define SFC_REG_DMA_CFG0              (0x120)
#define SFC_REG_DMA_CFG1              (0x124)
#define SFC_REG_TRAN_ENABLE           (0x180)
#define SFC_REG_FIFO_DATA             (0x184)
#define SFC_REG_DMA_ADDR              (0x188)
#define SFC_REG_DMA_LEN               (0x190)
#define SFC_REG_TRAN_ABORT            (0x19c)
#define SFC_REG_INST(i)               (0x200 + (0x40 * i))
#define SFC_REG_ADDR(i)               (0x204 + (0x40 * i))
#define SFC_REG_MODE(i)               (0x208 + (0x40 * i))
#define SFC_REG_DATA(i)               (0x20c + (0x40 * i))
#define SFC_REG_MATCH_MASK(i)         (0x218 + (0x40 * i))
#define SFC_REG_MATCH_DATA(i)         (0x21c + (0x40 * i))
#define SFC_REG_INTERFACE(i)          (0x220 + (0x40 * i))
#define SFC_REG_LENGTH(i)             (0x224 + (0x40 * i))
#define SFC_REG_INTERVAL(i)           (0x22c + (0x40 * i))

#define SFC_TXFIFO_SIZE               (8)

/****************** field definitions *******************/
/* fifo threshold and status (0xc0) */
#define STA_RXFIFO_CNT                     (24)//[31:24]
#define STA_TXFIFO_CNT                     (16)//[23:16]
#define CFG_RX_FIFO_TH                     (8)//[15:8]
#define CFG_TXFIFO_TH                      (0)//[7:0]

/* sfc status (0xd0) */
#define STA_SPI_ABORT_HAPPEN                (9)//w1c
#define STA_MDLU_LOCK_MODE                  (8)
#define STA_MDLU_LOCK_ERR                   (7)
#define STA_MDLU_INIT_DONE                  (6)
#define STA_SFC_TRANS_IDLE                  (5)
#define STA_SFC_TRANS_DONE                  (4)//w1C
#define STA_RXFIFO_FULL                     (3)
#define STA_TXFIFO_EMPTY                    (2)
#define STA_XIP_ADDR_ERR                    (1)//w1c
#define STA_XIP_MODE_ERR                    (0)//w1c

#define STA_SPI_ABORT_HAPPEN_INT            (1 << 9)//w1c
#define STA_MDLU_INIT_DONE_INT              (1 << 6)
#define STA_SFC_TRANS_DONE_INT              (1 << 4)//w1C
#define STA_RXFIFO_FULL_INT                 (1 << 3)
#define STA_TXFIFO_EMPTY_INT                (1 << 2)
#define STA_XIP_ADDR_ERR_INT                (1 << 1)//w1c
#define STA_XIP_MODE_ERR_INT                (1 << 0)//w1c

/* sfc interrupt enable (0xd4) */
#define STA_SPI_ABORT_HAPPEN_INT_EN         (1<<9)
#define STA_MDLU_INIT_DONE_INIT_EN          (1<<5)
#define STA_SFC_TRANS_DONE_INT_EN           (1<<4)
#define STA_RXFIFOCNT_FULL_INT_EN           (1<<3)
#define STA_TXFIFO_EMPTY_INT_EN             (1<<2)
#define STA_XIP_ADDR_ERR_INT_EN             (1<<1)
#define STA_XIP_MODE_ERR_INT_EN             (1<<0)

/* SFC work mode config (0x100) */
#define CFG_DUAL_FLASH_EN                   (6)
#define CFG_HYPERBUS_FLASH                  (5)//spiflash or hyperflash
#define CFG_SFC_FIFO_CLEAR                  (4)
#define CFG_SFC_WORK_MODE                   (2)//[3:2] swc or xip mode
#define RET_XIP_IDLE                        (1)
#define CFG_XIP_HOLD                        (0)

#define SWC_MODE                            (0)
#define XIP_MODE                            (1)
#define SPI_FLASH                           (0)
#define HYPER_FLASH                         (1)

/* spi cs timing con (0x104) */
#define CFG_SPI_CS_TIMING                   (0)//[7:0]

/* spi baud rate con (0x108) */
#define CFG_SPI_TIME_MODE                   (31)//mode 0 or mode 3
#define CFG_SPI_SCLK_SAME                   (30)
#define CFG_SPI_BAUD_RATE                   (0)//[9:0]

#define SPI_MODE_0                          (0)
#define SPI_MODE_1                          (1)
#define SPI_SCLK_SAME                       (0)
#define SPI_SCLK_UNSAME                     (1)

/* spi wait phase con (0x10c) */
#define CFG_WAIT_CYCLE_LEN                  (0)//[7:0]
#define CFG_XIP_TIMEOUT_EN                  (31)
#define CFG_XIP_TIMEOUT_TH                  (0)//[11:0]

/* RXDS con0 (0x114) */
#define CFG_MDLU_CLOSE                      (31)
#define CFG_MDLU_RESET                      (30)
#define RET_MDLU_LOCK_WORD                  (20)//[27:20]
#define CFG_MDLU_WAIT                       (16)//[19:16]
#define CFG_MDLU_STEP                       (8)//[15:8]
#define CFG_MDLU_INIT                       (0)//[7:0]

/* RXDS con1 (0x118) */
#define CFG_MDLU_LOCK_FREQ                  (30)//[31:30]
#define CFG_MDLU_LOCK_TIME                  (0)//[29:0]

/* RXDS con2 (0x11c) */
#define CFG_RX_SAMP_DELAY                   (28)//[31:28]
#define CFG_SDLU_UPDATE_EN                  (10)
#define CFG_RX_SDLU_DELAY                   (0)//[7:0]

/* DMA interface con0 (0x120) */
#define CFG_DMA_AXI_AWQOS                   (20)//[23:20]
#define CFG_DMA_AXI_ARQOS                   (16)//[19:16]
#define CFG_DMA_AXI_AWID                    (8)//[11:8]
#define CFG_DMA_AXI_ARID                    (0)//[3:0]

/* DMA interface con1 (0x124) */
#define STA_DMA_CLEAN_URD                   (17)
#define STA_DMA_CLEAN_DWR                   (16)
#define CFG_AXI_OSTMAX_AR                   (12)//[15:12]
#define CFG_AXI_OSTMAX_AW                   (8)//[11:8]
#define CFG_DMA_OSTEN_URD                   (7)
#define CFG_DMA_OSTEN_DWR                   (6)
#define CFG_DMA_BOUNDARY_URD                (3)//[5:3]
#define CFG_DMA_BOUNDARY_DWR                (0)//[2:0]

#define BURST_BOUNDARY_4KB                  (0x0)
#define BURST_BOUNDARY_2KB                  (0x1)
#define BURST_BOUNDARY_1KB                  (0x2)
#define BURST_BOUNDARY_512B                 (0x3)
#define BURST_BOUNDARY_256Byte              (0x4)
#define BURST_BOUNDARY_128Byte              (0x5)
#define BURST_BOUNDARY_64Byte               (0x6)
#define BURST_BOUNDARY_32Byte               (0x7)

/* spi transfer enable con (0x180) */
#define CFG_SPI_TRANS_EN                    (1<<31)
#define CFG_SFC_DMA_EN                      (1<<30)
#define CFG_SFC_DMA_DIR                     (29)
#define DMA_UPLOAD                          (1 << 29)
#define DMA_DOWNLOAD                        (0 << 29)
#define CFG_SPI_SW_ABORT                    (28)
#define CFG_SPI_ABORT_MODE                  (27)
#define RET_SPI_TRANS_IDX                   (4)//[7:4]
#define CFG_SPI_CMD_NUM                     (0)//[3:0]

/* spi transfer interface con (0x220+0x40*i) */
#define CFG_SPI_TIMER_ABORT_EN              (29)
#define CFG_SPI_DATA_SEL                    (28)
#define CFG_SPI_CMD_TYPE                    (27)
#define CFG_POLLING_MATCH_MODE              (26)
#define CFG_SPI_CS_SEL                      (20)//[21:20]
#define CFG_SPI_CRYPTO_EN                   (16)
#define CFG_SPI_RXDS_EN                     (15)
#define CFG_SPI_TRANS_DIR                   (14)
#define CFG_SPI_TRANS_DDR_D                 (13)
#define CFG_SPI_TRANS_DDR_AMW               (12)
#define CFG_SPI_TRANS_DDR_I                 (11)//[13:11]
#define CFG_SPI_TRANS_PHASE_D               (10)
#define CFG_SPI_TRANS_PHASE_W               (9)
#define CFG_SPI_TRANS_PHASE_M               (8)
#define CFG_SPI_TRANS_PHASE_A               (7)
#define CFG_SPI_TRANS_PHASE_I               (6)//[10:6]
#define CFG_SPI_TRANS_SIZE_D                (4)//[5:4]
#define CFG_SPI_TRANS_SIZE_AMW              (2)//[3:2]
#define CFG_SPI_TRANS_SIZE_I                (0)//[1:0]

#define SPI_TRANS_DATA_FIFO                 (0 << 28)
#define SPI_TRANS_TXD_RXD                   (1 << 28)
#define NOT_POLLING_CMD                     (0 << 27)
#define POLLING_CMD                         (1 << 27)
#define FLASH_0                             (0 << 20)
#define FLASH_1                             (1 << 20)
#define SPI_CRYPTO_EN                       (1 << 16)
#define SPI_RXDS_EN                         (1 << 15)
#define READ_SPI_FLASH                      (0 << 14)
#define WRITE_SPI_FLASH                     (1 << 14)
#define INST_PHASE                          (1 << 6)
#define ADDR_PHASE                          (1 << 7)
#define MODE_PHASE                          (1 << 8)
#define WAIT_PHASE                          (1 << 9)
#define DATA_PHASE                          (1 << 10)

/* spi transfer length con (0x224+0x40*i) */
#define INST_PHASE_LEN                      (0)//[1:0]
#define ADDR_PHASE_LEN                      (2)//[3:2]
#define MODE_PHASE_LEN                      (4)//[5:4]
#define DATA_PHASE_LEN                      (6)//[31:6]

#endif
