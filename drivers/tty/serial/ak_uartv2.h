/*
 * AKXX uartv2 driver
 *
 * Copyright (C) 2023 Anyka(Guangzhou) Microelectronics Technology Co., Ltd.
 *
 * Author: JianXiong Yang <yang_jianxiong@anyka.oa>
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
 */

#ifndef __AK_UART_H_
#define __AK_UART_H_

/* Anyka AKxx UART */
#define PORT_AK   101

/* UART port definitions */
#define NR_PORTS        4
#define AK_SERIAL_NAME  "ttySAK"

#define AK_UART0_TXBUF_BASE (AK_VA_L2MEM + 0x1000)
#define AK_UART0_RXBUF_BASE (AK_VA_L2MEM + 0x1080)

#define AK_UART1_TXBUF_BASE (AK_VA_L2MEM + 0x1100)
#define AK_UART1_RXBUF_BASE (AK_VA_L2MEM + 0x1180)

#define AK_UART2_TXBUF_BASE (AK_VA_L2MEM + 0x1200)
#define AK_UART2_RXBUF_BASE (AK_VA_L2MEM + 0x1280)

#define AK_UART3_TXBUF_BASE (AK_VA_L2MEM + 0x1300)
#define AK_UART3_RXBUF_BASE (AK_VA_L2MEM + 0x1380)
#define AK_UART_REG_OFFSET  0x100

#define AK_UART0_BASE           (AK_VA_UART + 0x0000)
#define AK_UART1_BASE           (AK_VA_UART + 0x8000)

#define AK_UART0_PA_BASE        (AK_PA_UART + 0x0000)
#define AK_UART1_PA_BASE        (AK_PA_UART + 0x8000)


/* Hardware register offsets and field definitions */

/*************** Uart register offsets ***************/
#define UART_CFG                    (0x00)
#define UART_CFG_RX_THRESHOLD       (0x04)
#define UART_CFG_TX_THRESHOLD       (0x08)
#define UART_TX_RX_DATA             (0x0c)
#define UART_MUTIL_COM              (0x10)
#define UART_BAUD_DIV               (0x14)
#define UART_STATUS                 (0x18)
#define UART_ALTER_FUNC_CTRL        (0x1c)
#define UART_INT_CTRL               (0x20)
#define UART_TURN_TIME              (0x24)
#define UART_RS485_CTRL             (0x28)
#define UART_FLOW_CTRL              (0x2c)
#define UART_RECEIVE_TIMEOUT        (0x30)
#define UART_PRESCARE_CNT           (0x34)
#define UART_FIFO_STATUS            (0x38)

/*************** field definitions ***************/
//CFG_UART(0x00)
#define CFG_TX_CLR                  (1 << 29)
#define CFG_RX_CLR                  (1 << 28)
#define CFG_TX_EN                   (1 << 27)
#define CFG_RX_EN                   (1 << 26)
#define CFG_UART_EN                 (1 << 25)
#define CFG_IDLE_TIME               (16)//[24:16]
#define CFG_IDLE_TIME_MASK          (0xff << 16)
#define CFG_PARITY_TYPE             (12)//[13:12]
#define CFG_PARITY_EN               (1 << 11)
#define CFG_UART_MODE_MASK          (0x3 << 9)
#define NORMAL_MODE                 (0x00 << 9)
#define LOCAL_LOOPBACK_MODE         (0x01 << 9)
#define LAUNCH_LOOPBACK_MODE        (0x02 << 9)               
#define DIRECT_LOOPBACK             (0x03 << 9)
#define CFG_DATA_SEQ                (8)
#define CFG_RXD_SEL                 (7)
#define CFG_TXD_SEL                 (6)
#define CFG_TX_STOP_BIT             (4)//[5:4]
#define CFG_RX_STOP_BIT             (2)//[3:2]
#define CFG_DATA_BIT                (0)

#define TX_STOP_BIT_MASK            (0x3 << CFG_TX_STOP_BIT)
#define TX_STOP_0_POINT_5_BIT       (0x00 << CFG_TX_STOP_BIT)//0.5bit
#define TX_STOP_1BIT                (0x01 << CFG_TX_STOP_BIT)//1bit(default)
#define TX_STOP_1_POINT_5_BIT       (0x02 << CFG_TX_STOP_BIT)//1.5bit
#define TX_STOP_2BIT                (0x03 << CFG_TX_STOP_BIT)//2bit

#define RX_STOP_BIT_MASK            (0x3 << CFG_RX_STOP_BIT)
#define RX_STOP_0_POINT_5_BIT       (0x00 << CFG_RX_STOP_BIT)//0.5bit
#define RX_STOP_1BIT                (0x01 << CFG_RX_STOP_BIT)//1bit(default)
#define RX_STOP_1_POINT_5_BIT       (0x02 << CFG_RX_STOP_BIT)//1.5bit
#define RX_STOP_2BIT                (0x03 << CFG_RX_STOP_BIT)//2bit

#define DATA_5BIT                   (0x00 << CFG_DATA_BIT)//5bit
#define DATA_6BIT                   (0x01 << CFG_DATA_BIT)//6bit
#define DATA_7BIT                   (0x02 << CFG_DATA_BIT)//7bit
#define DATA_8BIT                   (0x03 << CFG_DATA_BIT)//8bit(default)

//CFG_RX_THRESHOLD(0x04)
#define RX_TH_EN                    (1UL << 31)
#define RX_TH_CNT                   (16)//[27:16]
#define RX_TH_CFG                   (0)//[11:0]
#define RX_TH_CFG_MASK              (0xfff << 0)

//CFG_TX_THRESHOLD(0x08)
#define TX_TH_EN                    (1UL << 31)
#define TX_TH_CNT                   (16)//[27:16]
#define TX_TH_CFG                   (0)//[11:0]
#define TX_TH_CFG_MASK              (0xfff << 0)

//TX_RX_DATA(0x0c)
#define TX_SHIFT_DAT                (24)//[31:24]
#define TX_DAT                      (16)//[23:16]
#define RX_SHIFT_DAT                (8)//[15:8]
#define RX_DAT                      (0)//[7:0]

//MUTIL_COM(0x10)
#define CFG_MULTI_COM_EN            (1UL << 31)
#define CFG_SEND_ADDR               (30)
#define CFG_MULTI_TYPE              (29)
#define CFG_TX_ADDR                 (16)//[23:16]
#define CFG_RX_ADDR                 (0)//[7:0]

//BAUD_DIV(0x14)
#define CFG_INTERGER                (16)//[31:16]
#define CFG_INTERGER_MASK           (0xffff << CFG_INTERGER)
#define CFG_FRACTION                (0)
#define CFG_FRACTION_MASK           (0xf << CFG_FRACTION)

//STATUS(0x18)
#define STA_TX_BUSY                 (19)
#define STA_CTS_CHANGE              (18)//W1C
#define STA_TX_END                  (17)//W1C
#define STA_TX_FIFO_OVER_ERR        (16)//W1C
#define STA_RX_FIFO_OVER_ERR        (15)//W1C
#define STA_RX_NOISE_ERR            (14)//W1C
#define STA_RX_FRAME_ERR            (13)//W1C
#define STA_RX_PAR_ERR              (12)//W1C
#define STA_RX_START_ERR            (11)//W1C
#define STA_RX_TIMEOUT              (10)//W1C
#define STA_BREAK_ERR               (9)//W1C
#define STA_RX_TH_INT               (8)//W1C
#define STA_TX_TH_INT               (7)//W1C
#define STA_WMEM_NRDY               (6)//W1C
#define STA_RX_FIFO_FULL            (5)//W1C
#define STA_RX_EMPTY                (4)//W1C
#define STA_RX_FIFO_THRESHOLD       (3)//W1C
#define STA_TX_FIFO_FULL            (2)
#define STA_TX_FIFO_EMPTY           (1)
#define STA_TX_FIFO_THRESHOLD       (0)

#define TX_BUSY                 (1 << 19)
#define CTS_CHANGE              (1 << 18)//W1C
#define TX_END                  (1 << 17)//W1C
#define TX_FIFO_OVER_ERR        (1 << 16)//W1C
#define RX_FIFO_OVER_ERR        (1 << 15)//W1C
#define RX_NOISE_ERR            (1 << 14)//W1C
#define RX_FRAME_ERR            (1 << 13)//W1C
#define RX_PAR_ERR              (1 << 12)//W1C
#define RX_START_ERR            (1 << 11)//W1C
#define RX_TIMEOUT              (1 << 10)//W1C
#define BREAK_ERR               (1 << 9)//W1C
#define RX_TH_INT               (1 << 8)//W1C
#define TX_TH_INT               (1 << 7)//W1C
#define WMEM_NRDY               (1 << 6)//W1C
#define RX_FIFO_FULL            (1 << 5)//W1C
#define RX_EMPTY                (1 << 4)//W1C
#define RX_FIFO_THRESHOLD       (1 << 3)//W1C
#define TX_FIFO_FULL            (1 << 2)
#define TX_FIFO_EMPTY           (1 << 1)
#define TX_FIFO_THRESHOLD       (1 << 0)

//ALTER_FUNC_CTRL(0x1c)
#define CFG_SIR_EN                  (1 << 31)
#define CFG_SIR_LP                  (30)
#define CFG_RECE_DAT                (28)
#define CFG_RX_INT_SEL              (26)//[27:26]
#define CFG_TX_INT_SEL              (24)//[25:24]
#define CFG_DMA_RX                  (17)
#define CFG_DMA_TX                  (16)

#define CFG_RX_INT_SEL_MASK         (0x3 << CFG_RX_INT_SEL)
#define RX_FIFO_1_8_FULL            (0x00 << CFG_RX_INT_SEL)// >= 1/8full
#define RX_FIFO_1_4_FULL            (0x01 << CFG_RX_INT_SEL)// >= 1/4full
#define RX_FIFO_1_2_FULL            (0x02 << CFG_RX_INT_SEL)// >= 1/2full(default)
#define RX_FIFO_7_8_FULL            (0x03 << CFG_RX_INT_SEL)// >= 7/8full

#define CFG_TX_INT_SEL_MASK         (0x3 << CFG_TX_INT_SEL)
#define TX_FIFO_1_8_FULL            (0x00 << CFG_TX_INT_SEL)// <= 1/8full
#define TX_FIFO_1_4_FULL            (0x01 << CFG_TX_INT_SEL)// <= 1/4full
#define TX_FIFO_1_2_FULL            (0x02 << CFG_TX_INT_SEL)// <= 1/2full(default)
#define TX_FIFO_7_8_FULL            (0x03 << CFG_TX_INT_SEL)// <= 7/8full

#define CFG_STOP_FIND               (20)
#define CFG_SAMPLE_LOC              (19)
#define CFG_HIGH_PERFORM            (18)
#define CFG_DMA_RX_EN               (17)
#define CFG_DMA_TX_EN               (16)
#define CFG_WORK_TYPE               (11)//[12:11]
#define CFG_HDUPLEX_EN              (10)
#define CFG_SMART_NACK              (9)
#define CFG_SMART_EN                (8)
#define CFG_SYNC_CPHA               (2)
#define CFG_SYNC_CPOL               (1)
#define CFG_SYNC_EN                 (0)

//INT_CTRL_EN(0x20)
#define CFG_CTS_INT_EN               (1 << 18)
#define CFG_TX_END_EN               (1 << 17)
#define CFG_TX_OVERRUN_EN           (1 << 16)
#define CFG_RX_OVERRUN_EN           (1 << 15)
#define CFG_NF_EN                   (1 << 14)
#define CFG_FE_EN                   (1 << 13)
#define CFG_PE_EN                   (1 << 12)
#define CFG_RE_START_EN             (1 << 11)
#define CFG_TIMEOUT_EN              (1 << 10)
#define CFG_BREAK_EN                (1 << 9)
#define CFG_RX_TH_EN                (1 << 8)
#define CFG_TX_TH_EN                (1 << 7)
#define CFG_DMA_TIMEOUT_EN          (1 << 6)
#define CFG_RX_FULL_EN              (1 << 5)
#define CFG_RX_EMPTY_EN             (1 << 4)
#define CFG_RX_SEL_EN               (1 << 3)
#define CFG_TX_FULL_EN              (1 << 2)
#define CFG_TX_EMPTY_EN             (1 << 1)
#define CFG_TX_SEL_EN               (1 << 0)

//TURN_TIME(0x24)
#define CFG_RE_TO_DE                (16)//[31:16]
#define CFG_DE_TO_RE                (0)//[15:0]

//RS485_CTRL(0x28)
#define CFG_RS485_EN                (1 << 31)
#define DE_INVALID_TIME             (16)//[23:16]
#define DE_VALID_TIME               (0)//[7:0]

//FLOW_CTRL(0x2c)
#define CFG_RTS                     (4)
#define CFG_CTS_SEL                 (3)
#define CFG_RTS_SEL                 (2)
#define CFG_CTS_EN                  (1 << 1)
#define CFG_RTS_EN                  (1 << 0)

//RECEIVE_TIMEOUT(0x30)
#define CFG_RX_TIMEOUT_EN           (1UL << 31)
#define CFG_WR_REQ_THRESHOLD        (16)//[27:16]
#define CFG_RX_TIMEOUT              (0)//[15:0]

//PRESCARE_CNT(0x34)
#define CFG_PRESCARE_DIV            (0)//[7:0]


#define rL2_CONBUF8_15          (AK_VA_L2CTRL + 0x8C) 

//Stop Bit Timeout Configuration Register

#define UARTN_BIT_CFG(value)                ((value) & 0x1ff)
#define UARTN_TIME_OUT_CFG(value)           (((value) >> 16) & 0x0ffff)

#define TX_STATUS           (1)
#define RX_STATUS           (0)
#define DISABLE             (0)
#define ENABLE              (1)

#define UART_RX_FIFO_SIZE   128

#endif  /* end __AK_UART_H_ */
