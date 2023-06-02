// SPDX-License-Identifier: GPL-2.0
/*
 * NVT UART header for NA51055/NA51084 platform
 *
 * This file is the internal header file that defines the variables,
 * structures and function prototypes of UART module.
 *
 * @file  nvt_serial.h
 *
 * Copyright Novatek Microelectronics Corp. 2020. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef _NVT_SERIAL_H
#define _NVT_SERIAL_H

#define NVT_FR                           1               /* A flag to distinguish between different platforms */
#define UART_MCR_AFC                     0x40            /* Enable NVT auto-RTS/CTS */

/************************
 *    UART Registers    *
 ************************/
/*
 * 0x00  Receive Buffer Register (RBR), when DLAB=0 read
 *       Transmit Holding Register (THR), when DLAB=0 write
 *       Baud Rate Divisor Latch LSB (DLL), when DLAB=1
 */
#define UART_RBR_REG                     0x00            /* Receive data port */

#define UART_THR_REG                     0x00            /* Transmit data port */

#define UART_DLL_REG                     0x00
#define DLL_MASK                         GENMASK(7, 0)   /* Baud rate Divisor Latch Least significant byte */

#define UART_DLR_MAX                     0xFFFF          /* Divisor Latch Register Max value */
#define UART_DLR_MIN                     0x0001          /* Divisor Latch Register Min value */

/*
 * 0x04 Interrupt Enable Register (IER), when DLAB=0
 *      Baud Rate Divisor Latch MSB (DLM), when DLAB=1
 */
#define UART_IER_REG                     0x04
#define RDA_INTEN_BIT                    BIT(0)          /* Received Data Available Interrupt enable */
#define THR_EMPTY_INTEN_BIT              BIT(1)          /* Transmitter Holding Register Empty Interrupt enable */
#define RLS_INTEN_BIT                    BIT(2)          /* Receiver Line Status Interrupt enable */

#define UART_DLM_REG                     0x04
#define DLM_MASK                         GENMASK(7, 0)   /* Baud rate Divisor Latch Most significant byte */

/*
 * 0x08 Interrupt ID Register (IIR), when DLAB=0 read
 *      FIFO Control Register (FCR), when DLAB=0 write
 *      Prescaler Register (PSR), when DLAB=1
 */
#define UART_IIR_REG                     0x08
#define UART_IIR_INT_ID_MASK             GENMASK(3, 0)   /* Interrupt ID mask */
#define _UART_IIR_INT_NONE               0x1             /* None */
#define _UART_IIR_INT_RLS                0x6             /* Receiver Line Status */
#define _UART_IIR_INT_RDR                0x4             /* Received Data Ready */
#define _UART_IIR_INT_CRT                0xC             /* Character Reception Timeout */
#define _UART_IIR_INT_THRE               0x2             /* Transmitter Holding Register Empty */
#define FIFO_MODE_ENABLED_BIT            BIT(7)          /* This bit is set when FCR[0] is set as 1 */

#define UART_FCR_REG                     0x08
#define FIFO_EN_BIT                      BIT(0)          /* FIFO enable (Do not change this bit when TX_DMA_EN or RX_DMA_EN bit is set) */
#define RX_FIFO_RESET_BIT                BIT(1)          /* Rx FIFO Reset */
#define TX_FIFO_RESET_BIT                BIT(2)          /* Tx FIFO Reset */

#define RXFIFO_TRGL_MASK                 GENMASK(7, 6)   /* Rx trigger level for data ready interrupt */
#define _UART_FCR_RX_TRIGGER_SHIFT       6
#define _UART_FCR_RX_TRIGGER_LEVEL_0     0x0             /* character =  1(16 FIFO) and  1 (64 FIFO) */
#define _UART_FCR_RX_TRIGGER_LEVEL_1     0x1             /* character =  4(16 FIFO) and 16 (64 FIFO) */
#define _UART_FCR_RX_TRIGGER_LEVEL_2     0x2             /* character =  8(16 FIFO) and 48 (64 FIFO) */
#define _UART_FCR_RX_TRIGGER_LEVEL_3     0x3             /* character = 14(16 FIFO) and 56 (64 FIFO) */

#define UART_PSR_REG                     0x08
#define PSR_MASK                         GENMASK(4, 0)   /* Prescaler Register */

/* 0x0C Line Control Register (LCR) */
#define UART_LCR_REG                     0x0C
#define _WL_L5                           0x0             /* Char Length 5 */
#define _WL_L6                           0x1             /* Char Length 6 */
#define _WL_L7                           0x2             /* Char Length 7 */
#define _WL_L8                           0x3             /* Char Length 8 */
#define STOP_BIT                         BIT(2)          /* Stop Bits */
#define _UART_LCR_PARITY_SHIFT           3               /* LCR PARITY SHIFT BIT */
#define _UART_PARITY_NONE                0x0             /* No parity checking */
#define _UART_PARITY_ODD                 0x1             /* Odd parity */
#define _UART_PARITY_EVEN                0x3             /* Even parity */
#define _UART_PARITY_ONE                 0x5             /* One parity */
#define _UART_PARITY_ZERO                0x7             /* Zero parity */
#define SET_BREAK_BIT                    BIT(6)          /* Causes a break condition to be transmitted (Only Tx) */
#define DLAB_BIT                         BIT(7)          /* Divisor Latch Access Bit (DLAB) */

/*
 * 0x14 Line Status Register (LSR), when read
 *      Testing Register (TST), when write
 */
#define UART_LSR_REG                     0x14
#define DATA_READY_BIT                   BIT(0)          /* Data ready */
#define OVERRUN_ERR_BIT                  BIT(1)          /* Overrun error (PIO Only) */
#define PARITY_ERR_BIT                   BIT(2)          /* Parity error */
#define FRAMING_ERR_BIT                  BIT(3)          /* Framing error */
#define BREAK_INT_BIT                    BIT(4)          /* Break interrupt */
#define THR_EMPTY_BIT                    BIT(5)          /* THR empty */
#define TRANSMITTER_EMPTY_BIT            BIT(6)          /* Transmitter empty */
#define BOTH_EMPTY_BIT                   (THR_EMPTY_BIT | TRANSMITTER_EMPTY_BIT)
#define FIFO_DATA_ERR_BIT                BIT(7)          /* FIFO data error */

#define UART_TST_REG                     0x14
#define TEST_PAR_ERR_BIT                 BIT(0)          /* Generates incorrect parity during UART transmission */
#define TEST_FRM_ERR_BIT                 BIT(1)          /* Generates a logic 0 STOP bit during UART transmission */
#define TEST_BAUDGEN_BIT                 BIT(2)          /* Just to improve baud rate generator toggle rate */

/* 0x1C Scratch Pad Register (SPR) */
#define UART_SPR_REG                     0x1C
#define USER_DATA_MASK                   GENMASK(7, 0)   /* Used by programmer to hold data temporarily */

/*************************
 *    UART2 Registers    *
 *************************/
/* 0x10 MODEN Control Register */
#define UART_MCR_REG                     0x10
#define LOOP_BIT                         BIT(4)
#define HW_FLOW_CTRL_BIT                 BIT(6)          /* Hardware Flow Control */

/* 0x20 RS485 Register */
#define UART_RS485_REG                   0x20
#define ENABLE_BIT                       BIT(0)          /* RS485 mode is enable, RTS signal will be operated as DE signal */
#define SETUP_TIME_MASK                  GENMASK(15, 4)  /* Setup time between DE rising and first Tx signal */
#define _UART_RS485_SETUP_TIME_SHIFT     4
#define HOLD_TIME_MASK                   GENMASK(31, 20) /* Setup time between DE falling and first Tx signal */
#define _UART_RS485_HOLD_TIME_SHIFT      20

/* 0x80 Tx DMA Starting Address Register */
#define UART_TX_DMA_ADDR_REG             0x80
#define TX_DMA_ADDRESS_MASK              GENMASK(30, 0)

/* 0x84 Tx DMA Starting SIZE Register */
#define UART_TX_DMA_SIZE_REG             0x84
#define TX_DMA_SIZE_MASK                 GENMASK(19, 0)

/* 0x88, Tx DMA Current Address Register */
#define UART_TX_CUR_ADDR_REG             0x88
#define TX_CUR_ADDRESS_MASK              GENMASK(30, 0)

/* 0x8C, Tx DMA Control Register */
#define UART_TX_DMA_CTRL_REG             0x8C
#define TX_DMA_EN_BIT                    BIT(0)

/* 0x90, Rx DMA Starting Address Register */
#define UART_RX_DMA_ADDR_REG             0x90
#define RX_DMA_ADDRESS_MASK              GENMASK(30, 0)

/* 0x94, Rx DMA Starting SIZE Register */
#define UART_RX_DMA_SIZE_REG             0x94
#define RX_DMA_SIZE_MASK                 GENMASK(19, 0)

/* 0x98, Rx DMA Current Address Register */
#define UART_RX_CUR_ADDR_REG             0x98
#define RX_CUR_ADDRESS_MASK              GENMASK(30, 0)

/* 0x9C, Rx DMA Control Register */
#define UART_RX_DMA_CTRL_REG             0x9C
#define RX_DMA_EN_BIT                    BIT(0)

/* 0xA0, DMA Interrupt Control Register */
#define UART_DMA_INT_CTRL_REG            0xA0
#define TX_DMA_DONE_INTEN_BIT            BIT(0)
#define RX_DMA_DONE_INTEN_BIT            BIT(8)
#define RX_DMA_ERR_INTEN_BIT             BIT(9)

/* 0xA4, DMA Interrupt Status Register */
#define UART_DMA_INT_STS_REG             0xA4
#define TX_DMA_DONE_BIT                  BIT(0)
#define RX_DMA_DONE_BIT                  BIT(8)
#define RX_DMA_ERR_BIT                   BIT(9)
#define DMA_OVERRUN_ERR_BIT              BIT(16)
#define DMA_PARITY_ERR_BIT               BIT(17)
#define DMA_FRAMING_ERR_BIT              BIT(18)
#define DMA_BREAK_ERR_BIT                BIT(19)

struct nvt_port {
	struct uart_port    uart;
	char                name[16];
	struct spinlock     write_lock;
	struct clk          *clk;
	unsigned int        baud;
	unsigned int        tx_loadsz;                /* transmit fifo load size */
	unsigned int        hw_flowctrl;              /* hardware flow contol */
	unsigned int        rx_trig_level;            /* rx fifo trigger level */
	unsigned int        rs485_en;                 /* rs485 feature */
	unsigned int        rs485_delay_before_send;
	unsigned int        rs485_delay_after_send;
	/*
	 * Some bits in registers are cleared on a read, so they must
	 * be saved whenever the register is read, but the bits will not
	 * be immediately processed.
	 */
	unsigned int        lsr_break_flag;
	unsigned char       lsr_saved_flags;

	/* DMA feature */
	unsigned int        tx_dma_en;
	unsigned int        rx_dma_en;
	unsigned char       *tx_virt_addr;
	unsigned char       *rx_virt_addr;
	dma_addr_t          tx_dma_addr;
	dma_addr_t          rx_dma_addr;
	unsigned int        tx_size;
	unsigned int        rx_size;
	struct work_struct  rx_dma_work;
};

static inline void nvt_write(struct uart_port *port, unsigned int off, unsigned int val)
{
	struct nvt_port *nvt_port = container_of(port, struct nvt_port, uart);
	unsigned long flags = 0;

	spin_lock_irqsave(&nvt_port->write_lock, flags);

	writel_relaxed(val, port->membase + off);

	spin_unlock_irqrestore(&nvt_port->write_lock, flags);
}

static inline unsigned int nvt_read(struct uart_port *port, unsigned int off)
{
	return readl_relaxed(port->membase + off);
}

static inline void nvt_masked_write(struct uart_port *port, unsigned int off, unsigned int mask, unsigned int val)
{
	struct nvt_port *nvt_port = container_of(port, struct nvt_port, uart);
	unsigned long flags = 0;
	unsigned int tmp = nvt_read(port, off);  /* be careful that some registers are cleared after reading, such as RBR */

	spin_lock_irqsave(&nvt_port->write_lock, flags);

	tmp &= ~mask;
	tmp |= val & mask;
	writel_relaxed(tmp, port->membase + off);

	spin_unlock_irqrestore(&nvt_port->write_lock, flags);
}

#endif
