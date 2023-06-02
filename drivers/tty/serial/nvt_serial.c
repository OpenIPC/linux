// SPDX-License-Identifier: GPL-2.0
/*
 * NVT UART
 *
 * Driver for NVT Soc
 *
 * @file  nvt_serial.c
 *
 * Copyright Novatek Microelectronics Corp. 2020. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#if defined(CONFIG_SERIAL_NVT_CONSOLE) && defined(CONFIG_MAGIC_SYSRQ)
#define SUPPORT_SYSRQ
#endif

#include <linux/atomic.h>
#include <linux/hrtimer.h>
#include <linux/module.h>
#include <linux/io.h>
#include <linux/ioport.h>
#include <linux/irq.h>
#include <linux/init.h>
#include <linux/console.h>
#include <linux/tty.h>
#include <linux/tty_flip.h>
#include <linux/serial_core.h>
#include <linux/serial.h>
#include <linux/clk.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/of_irq.h>
#include <linux/dma-mapping.h>
#include <linux/clk-provider.h>

#include <plat/nvt_serial.h>
#include <plat/top.h>

#define DRIVER_VERSION		"2.0.3"

#define DRIVER_NAME		"nvt_serial"

#define RX_DMA_ADDR_ALIGN	16

static int global_line = 0;

static int __serial_rx_size = -1;
module_param_named(serial_rx_size, __serial_rx_size, int, 0600);

static int __serial_tx_dma_en_debug = -1;
module_param_named(serial_tx_dma_en_debug, __serial_tx_dma_en_debug, int, 0600);

static int __serial_rx_dma_en_debug = -1;
module_param_named(serial_rx_dma_en_debug, __serial_rx_dma_en_debug, int, 0600);

static int __serial_debug = 0;
module_param_named(serial_debug, __serial_debug, int, 0600);

static int __serial_debug_console = 0;
module_param_named(serial_debug_console, __serial_debug_console, int, 0600);

static int __serial_flow_debug = 0;
module_param_named(serial_flow_debug, __serial_flow_debug, int, 0600);

#define serial_dbg(fmt, ...) do { \
	if (port->line != 0 || __serial_debug_console) { \
		if (__serial_debug) \
			pr_info("nvt_serial%d: "fmt, port->line, ##__VA_ARGS__); \
		else \
			pr_debug("nvt_serial%d: "fmt, port->line,##__VA_ARGS__); \
	} \
} while (0)

#define serial_err(fmt, ...) do { \
	pr_err("nvt_serial%d: "fmt, port->line, ##__VA_ARGS__); \
} while (0)

#define serial_flow_dbg(fmt, ...) do { \
	if (port->line != 0 || __serial_debug_console) { \
		if (__serial_flow_debug) \
			pr_info("nvt_serial%d: %s\n"fmt, port->line, __func__, ##__VA_ARGS__); \
	} \
} while (0)

#define UART_TO_NVT(uart_port)	((struct nvt_port *) uart_port)

static void nvt_enable_rx_dma(struct work_struct *work)
{
	struct nvt_port *nvt_port = container_of(work, struct nvt_port, rx_dma_work);
	struct uart_port *port = &nvt_port->uart;

	serial_flow_dbg();

	if (__serial_rx_size > 0)
		nvt_port->rx_size = __serial_rx_size;
	else
		nvt_port->rx_size = UART_XMIT_SIZE;

	nvt_write(port, UART_RX_DMA_ADDR_REG, nvt_port->rx_dma_addr);
	nvt_write(port, UART_RX_DMA_SIZE_REG, nvt_port->rx_size);
	nvt_masked_write(port, UART_DMA_INT_CTRL_REG, RX_DMA_DONE_INTEN_BIT, RX_DMA_DONE_INTEN_BIT);
	nvt_masked_write(port, UART_DMA_INT_CTRL_REG, RX_DMA_ERR_INTEN_BIT, RX_DMA_ERR_INTEN_BIT);
	nvt_masked_write(port, UART_RX_DMA_CTRL_REG, RX_DMA_EN_BIT, RX_DMA_EN_BIT);
}

static void nvt_enable_tx_dma(struct uart_port *port)
{
	struct nvt_port *nvt_port = UART_TO_NVT(port);
	struct circ_buf *xmit = &port->state->xmit;

	serial_flow_dbg();

	nvt_port->tx_size = 0;

	while (!uart_circ_empty(xmit)) {
		*(nvt_port->tx_virt_addr + nvt_port->tx_size) = xmit->buf[xmit->tail];
		port->icount.tx++;
		serial_dbg("head(%d) tail(%d) cnt_tx(%d), tx_virt_addr[%d](0x%x)\n", xmit->head, xmit->tail, port->icount.tx, nvt_port->tx_size, *(nvt_port->tx_virt_addr + nvt_port->tx_size));
		xmit->tail = (xmit->tail + 1) & (UART_XMIT_SIZE - 1);
		nvt_port->tx_size++;
	}

	if (nvt_port->tx_size > 0) {
		nvt_write(port, UART_TX_DMA_ADDR_REG, nvt_port->tx_dma_addr);
		nvt_write(port, UART_TX_DMA_SIZE_REG, nvt_port->tx_size);
		nvt_masked_write(port, UART_DMA_INT_CTRL_REG, TX_DMA_DONE_INTEN_BIT, TX_DMA_DONE_INTEN_BIT);
		nvt_masked_write(port, UART_TX_DMA_CTRL_REG, TX_DMA_EN_BIT, TX_DMA_EN_BIT);
	}
}

static void nvt_handle_rx_dma(struct uart_port *port, unsigned int lsr_r)
{
	struct nvt_port *nvt_port = UART_TO_NVT(port);
	struct tty_port *tport = &port->state->port;
	unsigned char ch;
	char flag;
	unsigned int idx, dma_transfered;

	serial_flow_dbg();

	dma_transfered = (nvt_read(port, UART_RX_CUR_ADDR_REG) & RX_CUR_ADDRESS_MASK) - nvt_port->rx_dma_addr;

	for (idx = 0; idx < dma_transfered; idx++) {
		ch = *(nvt_port->rx_virt_addr + idx);

		flag = TTY_NORMAL;
		port->icount.rx++;

		serial_dbg("start(0x%x) end(0x%x) cnt_rx(%d), rx_virt_addr[%d](0x%x)\n",
			(unsigned int)nvt_port->rx_virt_addr, (unsigned int)nvt_port->rx_virt_addr + dma_transfered, port->icount.rx, idx, ch);

		lsr_r |= nvt_port->lsr_break_flag;
		nvt_port->lsr_break_flag = 0;

		if (lsr_r & BREAK_INT_BIT) {
			port->icount.brk++;
			if (uart_handle_break(port))
				continue;
		} else if (lsr_r & PARITY_ERR_BIT)
			port->icount.parity++;
		else if (lsr_r & FRAMING_ERR_BIT)
			port->icount.frame++;

		if (lsr_r & OVERRUN_ERR_BIT)
			port->icount.overrun++;

		/*
		 * Mask off conditions which should be ignored.
		 */
		lsr_r &= port->read_status_mask;

		if (lsr_r & BREAK_INT_BIT)
			flag = TTY_BREAK;
		else if (lsr_r & PARITY_ERR_BIT)
			flag = TTY_PARITY;
		else if (lsr_r & FRAMING_ERR_BIT)
			flag = TTY_FRAME;

		if (!(uart_handle_sysrq_char(port, ch)))
			uart_insert_char(port, lsr_r, OVERRUN_ERR_BIT, ch, flag);

		lsr_r = nvt_read(port, UART_LSR_REG);
	}

	/*
	 * Drop the lock here since it might end up calling
	 * uart_start(), which takes the lock.
	 */
	spin_unlock(&port->lock);
	tty_flip_buffer_push(tport);
	spin_lock(&port->lock);
}

static void nvt_flush_rx(struct uart_port *port)
{
	struct nvt_port *nvt_port = UART_TO_NVT(port);
	struct tty_port *tport = &port->state->port;
	unsigned char ch;
	char flag;
	unsigned int lsr_r;

	serial_flow_dbg();

	while ((lsr_r = nvt_read(port, UART_LSR_REG)) & DATA_READY_BIT) {
		ch = nvt_read(port, UART_RBR_REG);

		flag = TTY_NORMAL;
		port->icount.rx++;

		serial_dbg("LSR(0x%x) cnt_rx(%d), read ch(0x%x) from RBR\n", lsr_r, port->icount.rx, ch);

		lsr_r |= nvt_port->lsr_break_flag;
		nvt_port->lsr_break_flag = 0;

		if (lsr_r & BREAK_INT_BIT) {
			port->icount.brk++;
			if (uart_handle_break(port))
				continue;
		} else if (lsr_r & PARITY_ERR_BIT)
			port->icount.parity++;
		else if (lsr_r & FRAMING_ERR_BIT)
			port->icount.frame++;

		if (lsr_r & OVERRUN_ERR_BIT)
			port->icount.overrun++;

		/*
		 * Mask off conditions which should be ignored.
		 */
		lsr_r &= port->read_status_mask;

		if (lsr_r & BREAK_INT_BIT)
			flag = TTY_BREAK;
		else if (lsr_r & PARITY_ERR_BIT)
			flag = TTY_PARITY;
		else if (lsr_r & FRAMING_ERR_BIT)
			flag = TTY_FRAME;

		if (!(uart_handle_sysrq_char(port, ch)))
			uart_insert_char(port, lsr_r, OVERRUN_ERR_BIT, ch, flag);
	}

	/*
	 * Drop the lock here since it might end up calling
	 * uart_start(), which takes the lock.
	 */
	spin_unlock(&port->lock);
	tty_flip_buffer_push(tport);
	spin_lock(&port->lock);
}

static void nvt_stop_rx(struct uart_port *port)
{
	struct nvt_port *nvt_port = UART_TO_NVT(port);

	serial_flow_dbg();

	if (nvt_port->rx_dma_en && port->line != 0) {
		nvt_masked_write(port, UART_DMA_INT_CTRL_REG, RX_DMA_DONE_INTEN_BIT, 0);
		nvt_masked_write(port, UART_DMA_INT_CTRL_REG, RX_DMA_ERR_INTEN_BIT, 0);

		if (nvt_read(port, UART_RX_DMA_CTRL_REG) & RX_DMA_EN_BIT) {
			nvt_handle_rx_dma(port, nvt_read(port, UART_LSR_REG));
		}

		nvt_masked_write(port, UART_RX_DMA_CTRL_REG, RX_DMA_EN_BIT, 0);
		while (nvt_read(port, UART_RX_DMA_CTRL_REG) & RX_DMA_EN_BIT) {
			;
		}

		nvt_flush_rx(port);
	}

	nvt_masked_write(port, UART_IER_REG, RLS_INTEN_BIT | RDA_INTEN_BIT, 0);
}

static void nvt_stop_tx(struct uart_port *port)
{
	struct nvt_port *nvt_port = UART_TO_NVT(port);

	serial_flow_dbg();

	if (nvt_port->tx_dma_en && port->line != 0) {
		nvt_masked_write(port, UART_DMA_INT_CTRL_REG, TX_DMA_DONE_INTEN_BIT, 0);
		nvt_masked_write(port, UART_TX_DMA_CTRL_REG, TX_DMA_EN_BIT, 0);
		while (nvt_read(port, UART_TX_DMA_CTRL_REG) & TX_DMA_EN_BIT) {
			;
		}
	}

	nvt_masked_write(port, UART_IER_REG, THR_EMPTY_INTEN_BIT, 0);
	while (nvt_read(port, UART_IER_REG) & THR_EMPTY_INTEN_BIT) {
		;
	}
}

#define UART_LSR_BRK_ERROR_BITS    0x1E /* BI, FE, PE, OE bits */
#define LSR_SAVE_FLAGS             UART_LSR_BRK_ERROR_BITS
static void nvt_handle_tx(struct uart_port *port);
static void nvt_start_tx(struct uart_port *port)
{
	struct nvt_port *nvt_port = UART_TO_NVT(port);
	unsigned char lsr;

	serial_flow_dbg();

	/*
	 * We can't control the number of start_tx in one startup cycle,
	 * which is determined by the number of SyS_write calls.
	 *
	 * If handle_tx exhausted the xmit buffer, it needs to be returned
	 * when xmit buffer is empty to prevent start_tx from hanging without
	 * data.
	 */
	if (uart_circ_empty(&port->state->xmit) || uart_tx_stopped(port)) {
		nvt_stop_tx(port);
		serial_dbg("uart_circ_empty or uart_tx_stopped, so return\n");
		return;
	}

	if (nvt_port->tx_dma_en && port->line != 0) {
		while (nvt_read(port, UART_TX_DMA_CTRL_REG) & TX_DMA_EN_BIT) {
			serial_dbg("wait TX_DMA_EN_BIT auto clear\n");
			;
		}

		nvt_enable_tx_dma(port);
	} else {
		nvt_masked_write(port, UART_IER_REG, THR_EMPTY_INTEN_BIT, THR_EMPTY_INTEN_BIT);
		while (!(nvt_read(port, UART_IER_REG) & THR_EMPTY_INTEN_BIT)) {
			;
		}

		lsr = nvt_read(port, UART_LSR_REG);
		nvt_port->lsr_saved_flags |= lsr & LSR_SAVE_FLAGS;
		if (lsr & THR_EMPTY_BIT) {
			nvt_handle_tx(port);
		}
	}
}

static unsigned int nvt_tx_empty(struct uart_port *port)
{
	struct nvt_port *nvt_port = UART_TO_NVT(port);
	unsigned int lsr_r;
	unsigned long flags;

	serial_flow_dbg();

	spin_lock_irqsave(&port->lock, flags);
	lsr_r = nvt_read(port, UART_LSR_REG);
	nvt_port->lsr_saved_flags |= lsr_r & LSR_SAVE_FLAGS;
	spin_unlock_irqrestore(&port->lock, flags);

	return (lsr_r & BOTH_EMPTY_BIT) ? TIOCSER_TEMT : 0;
}

static void nvt_handle_rx(struct uart_port *port, unsigned int lsr_r)
{
	struct nvt_port *nvt_port = UART_TO_NVT(port);
	struct tty_port *tport = &port->state->port;
	unsigned char ch;
	char flag;

	serial_flow_dbg();

	do {
		if (likely(lsr_r & DATA_READY_BIT))
			ch = nvt_read(port, UART_RBR_REG);
		else
			ch = 0;

		flag = TTY_NORMAL;
		port->icount.rx++;

		serial_dbg("LSR(0x%x) IER(0x%x) cnt_rx(%d), read ch(0x%x) from RBR\n", lsr_r, nvt_read(port, UART_IER_REG), port->icount.rx, ch);

		lsr_r |= nvt_port->lsr_saved_flags;
		nvt_port->lsr_saved_flags = 0;

		lsr_r |= nvt_port->lsr_break_flag;
		nvt_port->lsr_break_flag = 0;

		if (lsr_r & BREAK_INT_BIT) {
			port->icount.brk++;
			if (uart_handle_break(port))
				continue;
		} else if (lsr_r & PARITY_ERR_BIT)
			port->icount.parity++;
		else if (lsr_r & FRAMING_ERR_BIT)
			port->icount.frame++;

		if (lsr_r & OVERRUN_ERR_BIT)
			port->icount.overrun++;

		/*
		 * Mask off conditions which should be ignored.
		 */
		lsr_r &= port->read_status_mask;

		if (lsr_r & BREAK_INT_BIT)
			flag = TTY_BREAK;
		else if (lsr_r & PARITY_ERR_BIT)
			flag = TTY_PARITY;
		else if (lsr_r & FRAMING_ERR_BIT)
			flag = TTY_FRAME;

		if (!(uart_handle_sysrq_char(port, ch)))
			uart_insert_char(port, lsr_r, OVERRUN_ERR_BIT, ch, flag);
	} while ((lsr_r = nvt_read(port, UART_LSR_REG)) & DATA_READY_BIT);

	/*
	 * Drop the lock here since it might end up calling
	 * uart_start(), which takes the lock.
	 */
	spin_unlock(&port->lock);
	tty_flip_buffer_push(tport);
	spin_lock(&port->lock);
}

static void nvt_handle_tx(struct uart_port *port)
{
	struct nvt_port *nvt_port = UART_TO_NVT(port);
	struct circ_buf *xmit = &port->state->xmit;
	unsigned char ch;
	int count;

	serial_flow_dbg();

	/* For software flow control, xon resume transmission, xoff pause transmission */
	if (port->x_char) {
		nvt_write(port, UART_THR_REG, port->x_char);
		port->icount.tx++;
		port->x_char = 0;
		return;
	}

	/* If there isn't anything more to transmit, or the uart is now stopped, disable the uart and exit */
	if (uart_circ_empty(xmit) || uart_tx_stopped(port)) {
		nvt_stop_tx(port);
		return;
	}

	/* Drain the buffer by size of tx_loadsz in one cycle */
	count = nvt_port->tx_loadsz;
	do {
		ch = xmit->buf[xmit->tail];
		nvt_write(port, UART_THR_REG, ch);
		port->icount.tx++;
		serial_dbg("head(%d) tail(%d) cnt_tx(%d), write ch(0x%x) to THR\n", xmit->head, xmit->tail, port->icount.tx, ch);
		xmit->tail = (xmit->tail + 1) & (UART_XMIT_SIZE - 1);
		if (uart_circ_empty(xmit))
			break;
	} while (--count > 0);

	/*
	 * If num chars in xmit buffer are too few, ask tty layer for more.
	 * By Hard ISR to schedule processing in software interrupt part.
	 */
	if (uart_circ_chars_pending(xmit) < WAKEUP_CHARS) {
		spin_unlock(&port->lock);
		uart_write_wakeup(port);
		spin_lock(&port->lock);
	}

	if (uart_circ_empty(xmit)) {
		nvt_stop_tx(port);
	}
}

static irqreturn_t nvt_irq(int irq, void *dev_id)
{
	struct uart_port *port = dev_id;
	struct nvt_port *nvt_port = UART_TO_NVT(port);
	unsigned int lsr_r, ier_r, iir_r, dma_sts, dma_int;
	unsigned long flags;

	spin_lock_irqsave(&port->lock, flags);

	lsr_r = nvt_read(port, UART_LSR_REG);  /* error bits are cleared after reading */
	ier_r = nvt_read(port, UART_IER_REG);
	iir_r = nvt_read(port, UART_IIR_REG);  /* clear interrupt status to avoid irq storm */

	if ((nvt_port->tx_dma_en || nvt_port->rx_dma_en) && port->line != 0) {
		dma_sts = nvt_read(port, UART_DMA_INT_STS_REG);
		dma_int = nvt_read(port, UART_DMA_INT_CTRL_REG);
		nvt_write(port, UART_DMA_INT_STS_REG, dma_sts);  /* clear dma interrupt status */
		serial_dbg("irq stage LSR(0x%x) IER(0x%x) IIR(0x%x) DMA_STS(0x%x) DMA_INT(0x%x)\n", lsr_r, ier_r, iir_r, dma_sts, dma_int);
	} else {
		serial_dbg("irq stage LSR(0x%x) IER(0x%x) IIR(0x%x)\n", lsr_r, ier_r, iir_r);
	}

	/* Handle UART rx */
	if ((lsr_r & DATA_READY_BIT) && (ier_r & RDA_INTEN_BIT)) {
		nvt_handle_rx(port, lsr_r);
	} else if (nvt_port->rx_dma_en && port->line != 0) {
		if ((dma_sts & RX_DMA_DONE_BIT) && (dma_int & RX_DMA_DONE_INTEN_BIT)) {
			nvt_masked_write(port, UART_DMA_INT_CTRL_REG, RX_DMA_DONE_INTEN_BIT, 0);
			nvt_masked_write(port, UART_DMA_INT_CTRL_REG, RX_DMA_ERR_INTEN_BIT, 0);
			while (nvt_read(port, UART_RX_DMA_CTRL_REG) & RX_DMA_EN_BIT) {
				serial_dbg("wait RX_DMA_EN_BIT auto clear\n");
				;
			}

			nvt_handle_rx_dma(port, lsr_r);

			schedule_work(&nvt_port->rx_dma_work);
		} else if ((dma_sts & RX_DMA_ERR_BIT) && (dma_int & RX_DMA_ERR_INTEN_BIT)) {
			nvt_masked_write(port, UART_DMA_INT_CTRL_REG, RX_DMA_DONE_INTEN_BIT, 0);
			nvt_masked_write(port, UART_DMA_INT_CTRL_REG, RX_DMA_ERR_INTEN_BIT, 0);
			nvt_masked_write(port, UART_RX_DMA_CTRL_REG, RX_DMA_EN_BIT, 0);
			while (nvt_read(port, UART_RX_DMA_CTRL_REG) & RX_DMA_EN_BIT) {
				;
			}

			nvt_handle_rx_dma(port, lsr_r);

			if (dma_sts & DMA_OVERRUN_ERR_BIT)
				serial_err("dma overrun error!\r\n");

			if (dma_sts & DMA_PARITY_ERR_BIT)
				serial_err("dma parity error!\r\n");

			if (dma_sts & DMA_FRAMING_ERR_BIT)
				serial_dbg("dma framing error!\r\n");

			if (dma_sts & DMA_BREAK_ERR_BIT)
				serial_dbg("dma break interrupt!\r\n");

			schedule_work(&nvt_port->rx_dma_work);
		}
	}

	/* Handle UART tx */
	if ((lsr_r & THR_EMPTY_BIT) && (ier_r & THR_EMPTY_INTEN_BIT)) {
		nvt_handle_tx(port);
	} else if (nvt_port->tx_dma_en && port->line != 0) {
		if ((dma_sts & TX_DMA_DONE_BIT) && (dma_int & TX_DMA_DONE_INTEN_BIT)) {
			nvt_masked_write(port, UART_DMA_INT_CTRL_REG, TX_DMA_DONE_INTEN_BIT, 0);
		}
	}

	/* Print UART error */
	if (lsr_r & FIFO_DATA_ERR_BIT) {
		if (lsr_r & OVERRUN_ERR_BIT)
			serial_err("overrun error!\r\n");

		if (lsr_r & PARITY_ERR_BIT)
			serial_err("parity error!\r\n");

		if (lsr_r & FRAMING_ERR_BIT)
			serial_dbg("framing error!\r\n");

		if (lsr_r & BREAK_INT_BIT)
			serial_dbg("break interrupt!\r\n");
	}

	/* Clear RBR when IIR stuck in CRT status */
	if (((iir_r & UART_IIR_INT_ID_MASK) == _UART_IIR_INT_CRT) && ((lsr_r & DATA_READY_BIT) == 0))
		nvt_read(port, UART_RBR_REG);

	spin_unlock_irqrestore(&port->lock, flags);

	return IRQ_HANDLED;
}

static unsigned int nvt_get_mctrl(struct uart_port *port)
{
	serial_flow_dbg();

	/*
	 * Pretend we have a Modem status reg and following bits are
	 *  always set, to satify the serial core state machine
	 *  (DSR) Data Set Ready
	 *  (CTS) Clear To Send
	 *  (CAR) Carrier Detect
	 */
	return TIOCM_DSR | TIOCM_CTS | TIOCM_CAR;
}

static void nvt_set_mctrl(struct uart_port *port, unsigned int mctrl)
{
	serial_flow_dbg();

	/* MCR not present */
}

static void nvt_break_ctl(struct uart_port *port, int break_ctl)
{
	unsigned long flags;

	serial_flow_dbg();

	spin_lock_irqsave(&port->lock, flags);

	if (break_ctl)
		nvt_masked_write(port, UART_LCR_REG, SET_BREAK_BIT, SET_BREAK_BIT);
	else
		nvt_masked_write(port, UART_LCR_REG, SET_BREAK_BIT, 0);

	spin_unlock_irqrestore(&port->lock, flags);
}

static int nvt_startup(struct uart_port *port)
{
	struct nvt_port *nvt_port = UART_TO_NVT(port);
	int ret;

	serial_flow_dbg();

	snprintf(nvt_port->name, sizeof(nvt_port->name),
		 "nvt_serial%d", port->line);

	ret = request_irq(port->irq, nvt_irq, IRQF_TRIGGER_HIGH,
			  nvt_port->name, port);
	if (unlikely(ret))
		return ret;

	/*
	 * Clear the interrupt registers again for luck, and clear the
	 * saved flags to avoid getting false values from polling
	 * routines or the previous session.
	 */
	nvt_read(port, UART_LSR_REG);
	nvt_read(port, UART_THR_REG);
	nvt_read(port, UART_IIR_REG);
	nvt_port->lsr_saved_flags = 0;
	nvt_port->lsr_break_flag = 0;

#if defined(NVT_FR) && defined(CONFIG_SERIAL_NVT_DMA)
	/* Set DMA feature */
	nvt_port->tx_dma_en = 1;
	nvt_port->rx_dma_en = 1;
#endif

	/* A debug node to enable/disable DMA feature */
	if (__serial_tx_dma_en_debug == 1)
		nvt_port->tx_dma_en = 1;
	else if (__serial_tx_dma_en_debug == 0)
		nvt_port->tx_dma_en = 0;

	if (__serial_rx_dma_en_debug == 1)
		nvt_port->rx_dma_en = 1;
	else if (__serial_rx_dma_en_debug == 0)
		nvt_port->rx_dma_en = 0;

	/* Allocate DMA */
	if (nvt_port->tx_dma_en && port->line != 0) {
		nvt_port->tx_virt_addr = dma_alloc_coherent(port->dev, UART_XMIT_SIZE, &nvt_port->tx_dma_addr, GFP_KERNEL);
		if (!nvt_port->tx_virt_addr) {
			serial_err("allocate DMA failed, using PIO\n");
			nvt_port->tx_dma_en = 0;
		} else {
			serial_dbg("allocate tx_virt_addr(0x%x) tx_dma_addr(0x%x)\n", (unsigned int)nvt_port->tx_virt_addr, nvt_port->tx_dma_addr);
		}
	}

	if (nvt_port->rx_dma_en && port->line != 0) {
		nvt_port->rx_virt_addr = dma_alloc_coherent(port->dev, UART_XMIT_SIZE, &nvt_port->rx_dma_addr, GFP_KERNEL);
		if (!nvt_port->rx_virt_addr) {
			serial_err("allocate DMA failed, using PIO\n");
			nvt_port->rx_dma_en = 0;
		} else if (!IS_ALIGNED(nvt_port->rx_dma_addr, (dma_addr_t)RX_DMA_ADDR_ALIGN) && (nvt_get_chip_id() == CHIP_NA51055 || nvt_get_chip_id() == CHIP_NA51084)) {
			serial_err("rx_dma_addr(0x%x) failed to align %d bytes, using PIO\n", nvt_port->rx_dma_addr, RX_DMA_ADDR_ALIGN);
			nvt_port->rx_dma_en = 0;
		} else {
			serial_dbg("allocate rx_virt_addr(0x%x) rx_dma_addr(0x%x)\n", (unsigned int)nvt_port->rx_virt_addr, nvt_port->rx_dma_addr);
		}
	}

	return 0;
}

static void nvt_shutdown(struct uart_port *port)
{
	struct nvt_port *nvt_port = UART_TO_NVT(port);
	unsigned long flags;

	serial_flow_dbg();

	spin_lock_irqsave(&port->lock, flags);
	nvt_write(port, UART_IER_REG, 0);  /* disable interrupts to avoid hanging in start_tx */
	spin_unlock_irqrestore(&port->lock, flags);

	free_irq(port->irq, port);

	/* Free DMA */
	if ((nvt_port->tx_dma_en || nvt_port->rx_dma_en) && port->line != 0) {
		if (nvt_port->tx_virt_addr) {
			serial_dbg("free tx_virt_addr(0x%x) tx_dma_addr(0x%x)\n", (unsigned int)nvt_port->tx_virt_addr, nvt_port->tx_dma_addr);
			dma_free_coherent(port->dev, UART_XMIT_SIZE, nvt_port->tx_virt_addr, nvt_port->tx_dma_addr);
			nvt_port->tx_virt_addr = NULL;
		}

		if (nvt_port->rx_virt_addr) {
			serial_dbg("free rx_virt_addr(0x%x) rx_dma_addr(0x%x)\n", (unsigned int)nvt_port->rx_virt_addr, nvt_port->rx_dma_addr);
			dma_free_coherent(port->dev, UART_XMIT_SIZE, nvt_port->rx_virt_addr, nvt_port->rx_dma_addr);
			nvt_port->rx_virt_addr = NULL;
		}

		nvt_write(port, UART_DMA_INT_CTRL_REG, 0);
		nvt_write(port, UART_TX_DMA_CTRL_REG, 0);
		nvt_write(port, UART_RX_DMA_CTRL_REG, 0);
	}
}

static void nvt_set_baud_rate(struct uart_port *port, unsigned int baud)
{
	unsigned int PSR, DLR;

	PSR = 0x01;
	DLR = port->uartclk / 16 / PSR / baud;

	if (DLR > UART_DLR_MAX) {
		DLR = UART_DLR_MAX;
		serial_err("baud out of MAX range error!\n");
	}
	if (DLR < UART_DLR_MIN) {
		DLR = UART_DLR_MIN;
		serial_err("baud out of MIN range error!\n");
	}

	nvt_write(port, UART_DLL_REG, DLR & DLL_MASK);
	nvt_masked_write(port, UART_DLM_REG, DLM_MASK, DLR >> 8);
	nvt_write(port, UART_PSR_REG, PSR & PSR_MASK);
}

static void nvt_set_termios(struct uart_port *port, struct ktermios *termios,
			    struct ktermios *old)
{
	struct nvt_port *nvt_port = UART_TO_NVT(port);
	unsigned long flags;
	unsigned int lcr_val, baud;

	serial_flow_dbg();

	spin_lock_irqsave(&port->lock, flags);

	/* Set baud rate and divisor */
	if (nvt_port->baud) {
		if (tty_termios_baud_rate(termios))
			tty_termios_encode_baud_rate(termios, nvt_port->baud, nvt_port->baud);
		nvt_port->baud = 0;
	}
	baud = uart_get_baud_rate(port, termios, old, 0, port->uartclk / 16);

	nvt_masked_write(port, UART_LCR_REG, DLAB_BIT, DLAB_BIT);  /* set DLAB bit to access divisor */
	nvt_set_baud_rate(port, baud);
	if (tty_termios_baud_rate(termios))
		tty_termios_encode_baud_rate(termios, baud, baud);

	/* Set length */
	switch (termios->c_cflag & CSIZE) {
	case CS5:
		lcr_val |= _WL_L5;
		break;
	case CS6:
		lcr_val |= _WL_L6;
		break;
	case CS7:
		lcr_val |= _WL_L7;
		break;
	case CS8:
	default:
		lcr_val |= _WL_L8;
		break;
	}

	/* Set stop bits */
	if (termios->c_cflag & CSTOPB)
		lcr_val |= STOP_BIT;

	/* Set parity */
	if (termios->c_cflag & PARENB) {
		if (termios->c_cflag & CMSPAR) {
			if (termios->c_cflag & PARODD)
				lcr_val |= _UART_PARITY_ONE << _UART_LCR_PARITY_SHIFT;
			else
				lcr_val |= _UART_PARITY_ZERO << _UART_LCR_PARITY_SHIFT;
		} else {
			if (termios->c_cflag & PARODD)
				lcr_val |= _UART_PARITY_ODD << _UART_LCR_PARITY_SHIFT;
			else
				lcr_val |= _UART_PARITY_EVEN << _UART_LCR_PARITY_SHIFT;
		}
	}
	nvt_write(port, UART_LCR_REG, lcr_val);  /* clear DLAB bit */

	/* Set rx fifo trigger level */
	nvt_write(port, UART_FCR_REG, FIFO_EN_BIT | (nvt_port->rx_trig_level << _UART_FCR_RX_TRIGGER_SHIFT));

	/* Set hardware flow control */
	if (nvt_port->hw_flowctrl) {
		termios->c_cflag |= CRTSCTS;
		nvt_port->hw_flowctrl = 0;
	}

#ifdef NVT_FR
	if (termios->c_cflag & CRTSCTS) {
		nvt_masked_write(port, UART_MCR_REG, HW_FLOW_CTRL_BIT, HW_FLOW_CTRL_BIT);
	} else {
		nvt_masked_write(port, UART_MCR_REG, HW_FLOW_CTRL_BIT, 0);
	}
#else
	if (termios->c_cflag & CRTSCTS) {
		nvt_masked_write(port, UART_IER_REG, HW_FLOW_CTRL_BIT, HW_FLOW_CTRL_BIT);
	} else {
		nvt_masked_write(port, UART_IER_REG, HW_FLOW_CTRL_BIT, 0);
	}
#endif

#ifdef NVT_FR
	/* Set rs485 feature */
	if (nvt_port->rs485_en) {
		nvt_write(port, UART_RS485_REG, ENABLE_BIT
			| (nvt_port->rs485_delay_before_send << _UART_RS485_SETUP_TIME_SHIFT)
			| (nvt_port->rs485_delay_after_send << _UART_RS485_HOLD_TIME_SHIFT));
	}
#endif

	if (nvt_port->rx_dma_en && port->line != 0) {
		schedule_work(&nvt_port->rx_dma_work);
	} else {
		/* Enanle receive interrupts */
		nvt_write(port, UART_IER_REG, RLS_INTEN_BIT | RDA_INTEN_BIT);
	}

	/* Configure status bits to ignore based on termio flags */
	port->read_status_mask = OVERRUN_ERR_BIT;
	if (termios->c_iflag & INPCK)
		port->read_status_mask |= FRAMING_ERR_BIT | PARITY_ERR_BIT;
	if (termios->c_iflag & (IGNBRK | BRKINT | PARMRK))
		port->read_status_mask |= BREAK_INT_BIT;

	uart_update_timeout(port, termios->c_cflag, baud);

	serial_dbg("baud(%d) hw_flowctrl(%d) tx_dma(%d) rx_dma(%d)\n", baud, ((termios->c_cflag & CRTSCTS) ? 1 : 0), nvt_port->tx_dma_en, nvt_port->rx_dma_en);

	spin_unlock_irqrestore(&port->lock, flags);
}

static const char *nvt_type(struct uart_port *port)
{
	return "NVT";
}

static void nvt_release_port(struct uart_port *port)
{
	serial_flow_dbg();
}

static int nvt_request_port(struct uart_port *port)
{
	serial_flow_dbg();

	return 0;
}

static void nvt_config_port(struct uart_port *port, int flags)
{
	if (flags & UART_CONFIG_TYPE)
		port->type = PORT_16550A;
}

static int nvt_verify_port(struct uart_port *port, struct serial_struct *ser)
{
	serial_flow_dbg();

	if (unlikely(ser->type != PORT_UNKNOWN && ser->type != PORT_16550A))
		return -EINVAL;
	if (unlikely(port->irq != ser->irq))
		return -EINVAL;
	return 0;
}

static inline void wait_for_xmitr(struct uart_port *port, int bits)
{
	struct nvt_port *nvt_port = UART_TO_NVT(port);
	unsigned int status, tmout = 10000;

	/* Wait up to 10ms for the character(s) to be sent */
	for (;;) {
		status = nvt_read(port, UART_LSR_REG);

		nvt_port->lsr_saved_flags |= status & LSR_SAVE_FLAGS;

		if (status & BREAK_INT_BIT)
			nvt_port->lsr_break_flag = BREAK_INT_BIT;

		if ((status & bits) == bits)
			break;
		if (--tmout == 0)
			break;
		udelay(1);
	}
}

#ifdef CONFIG_CONSOLE_POLL
static int nvt_poll_get_char(struct uart_port *port)
{
	unsigned char ch;

	while (!(nvt_read(port, UART_LSR_REG) & DATA_READY_BIT))
		cpu_relax();

	ch = nvt_read(port, UART_RBR_REG);

	return ch;
}

static void nvt_poll_put_char(struct uart_port *port, unsigned char ch)
{
	unsigned int ier;

	/* First save the IER then disable the interrupts */
	ier = nvt_read(port, UART_IER_REG);
	nvt_write(port, UART_IER_REG, 0);
	wait_for_xmitr(port, BOTH_EMPTY_BIT);

	/* Send the character out */
	nvt_write(port, UART_THR_REG, ch);

	/* Finally, wait for transmitter to become empty and restore the IER */
	wait_for_xmitr(port, BOTH_EMPTY_BIT);
	nvt_write(port, UART_IER_REG, ier);
}
#endif

static struct uart_ops nvt_uart_pops = {
	.stop_rx = nvt_stop_rx,
	.stop_tx = nvt_stop_tx,
	.start_tx = nvt_start_tx,
	.tx_empty = nvt_tx_empty,
	.get_mctrl = nvt_get_mctrl,
	.set_mctrl = nvt_set_mctrl,
	.break_ctl = nvt_break_ctl,
	.startup = nvt_startup,
	.shutdown = nvt_shutdown,
	.set_termios = nvt_set_termios,
	.type = nvt_type,
	.release_port = nvt_release_port,
	.request_port = nvt_request_port,
	.config_port = nvt_config_port,
	.verify_port = nvt_verify_port,
#ifdef CONFIG_CONSOLE_POLL
	.poll_get_char	= nvt_poll_get_char,
	.poll_put_char	= nvt_poll_put_char,
#endif
};

static struct nvt_port nvt_uart_ports[] = {
#ifdef CONFIG_SERIAL_NVT_CONSOLE
	{
		.uart = {
			.line = 0,
			.ops = &nvt_uart_pops,
			.flags = UPF_BOOT_AUTOCONF,
		/**********************************************************************
		 *  Following items are default values and will be overwritten by dts *
		 **********************************************************************/
			.uartclk = 24000000,
			.iotype = UPIO_MEM32,
			.fifosize = 64,
			.regshift = 2,
		},
		.baud = 115200, /* set into termios structure when first used, and can be modified through user interface such as stty */
		.tx_loadsz = 64, /* maximum number of characters tx can send in one handle_tx cycle, usually following fifosize */
		.hw_flowctrl = 0, /* default hardware flow control is on or off, also we can use stty crtscts/-crtscts to turn on/off */
		.rx_trig_level = 3,
	},
#endif
	{
		.uart = {
			.line = 1,
			.ops = &nvt_uart_pops,
			.flags = UPF_BOOT_AUTOCONF,
		/**********************************************************************
		 *  Following items are default values and will be overwritten by dts *
		 **********************************************************************/
			.uartclk = 48000000,
			.iotype = UPIO_MEM32,
			.fifosize = 64,
			.regshift = 2,
		},
		.baud = 115200,
		.tx_loadsz = 64,
		.hw_flowctrl = 0,
		.rx_trig_level = 3,
	},
	{
		.uart = {
			.line = 2,
			.ops = &nvt_uart_pops,
			.flags = UPF_BOOT_AUTOCONF,
		/**********************************************************************
		 *  Following items are default values and will be overwritten by dts *
		 **********************************************************************/
			.uartclk = 48000000,
			.iotype = UPIO_MEM32,
			.fifosize = 64,
			.regshift = 2,
		},
		.baud = 0,
		.tx_loadsz = 64,
		.hw_flowctrl = 0,
		.rx_trig_level = 3,
	},
	{
		.uart = {
			.line = 3,
			.ops = &nvt_uart_pops,
			.flags = UPF_BOOT_AUTOCONF,
		/**********************************************************************
		 *  Following items are default values and will be overwritten by dts *
		 **********************************************************************/
			.uartclk = 48000000,
			.iotype = UPIO_MEM32,
			.fifosize = 64,
			.regshift = 2,
		},
		.baud = 0,
		.tx_loadsz = 64,
		.hw_flowctrl = 0,
		.rx_trig_level = 3,
	},
	{
		.uart = {
			.line = 4,
			.ops = &nvt_uart_pops,
			.flags = UPF_BOOT_AUTOCONF,
		/**********************************************************************
		 *  Following items are default values and will be overwritten by dts *
		 **********************************************************************/
			.uartclk = 48000000,
			.iotype = UPIO_MEM32,
			.fifosize = 64,
			.regshift = 2,
		},
		.baud = 0,
		.tx_loadsz = 64,
		.hw_flowctrl = 0,
		.rx_trig_level = 3,
	},
	{
		.uart = {
			.line = 5,
			.ops = &nvt_uart_pops,
			.flags = UPF_BOOT_AUTOCONF,
		/**********************************************************************
		 *  Following items are default values and will be overwritten by dts *
		 **********************************************************************/
			.uartclk = 48000000,
			.iotype = UPIO_MEM32,
			.fifosize = 64,
			.regshift = 2,
		},
		.baud = 0,
		.tx_loadsz = 64,
		.hw_flowctrl = 0,
		.rx_trig_level = 3,
	},
};

static int UART_NR;

static inline struct uart_port *get_port_from_line(unsigned int line)
{
#ifdef CONFIG_SERIAL_NVT_CONSOLE
	return &nvt_uart_ports[line].uart;
#else
	return &nvt_uart_ports[line-1].uart;
#endif
}

#ifdef CONFIG_SERIAL_NVT_CONSOLE
static void nvt_console_putchar(struct uart_port *port, int ch)
{
	wait_for_xmitr(port, THR_EMPTY_BIT);
	nvt_write(port, UART_THR_REG, ch);
}

static void nvt_console_write(struct console *co, const char *s, unsigned int count)
{
	struct uart_port *port = get_port_from_line(co->index);
	unsigned long flags;
	unsigned int ier;
	int locked = 1;

	if (port->sysrq)
		locked = 0;
	else if (oops_in_progress)
		locked = spin_trylock_irqsave(&port->lock, flags);
	else
		spin_lock_irqsave(&port->lock, flags);

	/* First save the IER then disable the interrupts */
	ier = nvt_read(port, UART_IER_REG);
	nvt_write(port, UART_IER_REG, 0);

	uart_console_write(port, s, count, nvt_console_putchar);

	/* Finally, wait for transmitter to become empty and restore the IER */
	wait_for_xmitr(port, BOTH_EMPTY_BIT);
	nvt_write(port, UART_IER_REG, ier);

	if (locked)
		spin_unlock_irqrestore(&port->lock, flags);
}

static int __init nvt_console_setup(struct console *co, char *options)
{
	struct uart_port *port;
	int baud = 115200;
	int bits = 8;
	int parity = 'n';
	int flow = 'n';

	if (unlikely(co->index >= UART_NR || co->index < 0))
		return -ENXIO;

	/*
	 * The uart port backing the console (e.g. ttyS0) might not have been
	 * init yet. If so, defer the console setup to after the port.
	 */
	port = get_port_from_line(co->index);
	if (unlikely(!port->membase))
		return -ENXIO;

	if (options)
		uart_parse_options(options, &baud, &parity, &bits, &flow);

	pr_info("nvt_serial: console setup on port #%d\n", port->line);

	/*
	 * Serial core will call port->ops->set_termios( )
	 * which will set the baud reg.
	 */
	return uart_set_options(port, co, baud, parity, bits, flow);
}

static void nvt_serial_early_write(struct console *con, const char *s, unsigned n)
{
	struct earlycon_device *dev = con->data;

	uart_console_write(&dev->port, s, n, nvt_console_putchar);
}

static int __init nvt_serial_early_console_setup(struct earlycon_device *device, const char *opt)
{
	if (!device->port.membase)
		return -ENODEV;

	device->con->write = nvt_serial_early_write;
	return 0;
}
EARLYCON_DECLARE(nvt_serial, nvt_serial_early_console_setup);
OF_EARLYCON_DECLARE(nvt_serial, "nvt.nvt_uart", nvt_serial_early_console_setup);

static struct uart_driver nvt_uart_driver;

static struct console nvt_console = {
	.name = "ttyS",
	.write = nvt_console_write,
	.device = uart_console_device,
	.setup = nvt_console_setup,
	.flags = CON_PRINTBUFFER,
	.index = -1,
	.data = &nvt_uart_driver,
};

#define NVT_CONSOLE	(&nvt_console)
#else
#define NVT_CONSOLE	NULL
#endif

static struct uart_driver nvt_uart_driver = {
	.owner = THIS_MODULE,
	.driver_name = DRIVER_NAME,
	.dev_name = "ttyS",
	.cons = NVT_CONSOLE,
};

static int nvt_serial_probe(struct platform_device *pdev)
{
	struct nvt_port *nvt_port;
	struct uart_port *port;
	u32 prop, prop_array[2] = {0};
	int line, irq;
	struct resource *resource;

	if (of_property_read_u32(pdev->dev.of_node, "uart_id", &prop) == 0) {
		line = prop;
	} else {
		dev_dbg(&pdev->dev, "get uart_id failed, use default uart_id(%d)\n", global_line);
		line = global_line;
		global_line++;
	}

	if (line >= UART_NR) {
		dev_err(&pdev->dev, "unsupported uart line(%d)\n", line);
		return -ENXIO;
	}

	port = get_port_from_line(line);
	port->dev = &pdev->dev;
	nvt_port = UART_TO_NVT(port);

	spin_lock_init(&nvt_port->write_lock);

	INIT_WORK(&nvt_port->rx_dma_work, nvt_enable_rx_dma);

	resource = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (unlikely(!resource)) {
		dev_err(&pdev->dev, "get address failed\n");
		return -ENXIO;
	}
	port->mapbase = resource->start;

	port->membase = devm_ioremap(port->dev, resource->start, resource_size(resource));
	if (!port->membase) {
		dev_err(&pdev->dev, "get ioremap failed\n");
		return -EBUSY;
	}

	irq = irq_of_parse_and_map(pdev->dev.of_node, 0);
	if (unlikely(irq < 0)) {
		dev_err(&pdev->dev, "get irq failed\n");
		return -ENXIO;
	}
	port->irq = irq;

	if (line == 0) {
		if (nvt_get_chip_id() == CHIP_NA51055) {
			port->fifosize = 16;
			nvt_port->tx_loadsz = 16;
		}
	}

	if (of_property_read_u32(pdev->dev.of_node, "reg-shift", &prop) == 0)
		port->regshift = prop;

	if (of_property_read_u32(pdev->dev.of_node, "reg-io-width", &prop) == 0) {
		switch (prop) {
		case 1:
			port->iotype = UPIO_MEM;
			break;
		case 4:
			port->iotype = of_device_is_big_endian(pdev->dev.of_node) ?
				       UPIO_MEM32BE : UPIO_MEM32;
			break;
		default:
			dev_warn(&pdev->dev, "unsupported reg-io-width(%d)\n", prop);
		}
	}

	if (of_find_property(pdev->dev.of_node, "no-loopback-test", NULL))
		port->flags |= UPF_SKIP_TEST;

	if (of_property_read_u32(pdev->dev.of_node, "clock-frequency", &prop) == 0) {
		port->uartclk = prop;
	} else {
		dev_dbg(&pdev->dev, "get clock-frequency failed, use default uartclk(%d)\n", port->uartclk);
	}

	nvt_port->clk = clk_get(&pdev->dev, dev_name(&pdev->dev));
	if (IS_ERR(nvt_port->clk)) {
		dev_err(&pdev->dev, "clk %s not found\n", dev_name(&pdev->dev));
		return PTR_ERR(nvt_port->clk);
	} else {
		if (!__clk_is_enabled(nvt_port->clk))
			clk_prepare_enable(nvt_port->clk);

		clk_set_rate(nvt_port->clk, port->uartclk);
	}

	if (of_property_read_u32(pdev->dev.of_node, "baud", &prop) == 0)
		nvt_port->baud = prop;

	if (of_property_read_u32(pdev->dev.of_node, "rx_trig_level", &prop) == 0)
		nvt_port->rx_trig_level = prop;

	if (of_property_read_u32(pdev->dev.of_node, "hw_flowctrl", &prop) == 0)
		nvt_port->hw_flowctrl = prop;

	if (of_property_read_u32(pdev->dev.of_node, "rs485_en", &prop) == 0) {
		nvt_port->rs485_en = prop;

		if (of_property_read_u32_array(pdev->dev.of_node, "rs485_delay", prop_array, 2) == 0) {
			nvt_port->rs485_delay_before_send = prop_array[0];
			nvt_port->rs485_delay_after_send = prop_array[1];
		}
	}

	serial_dbg("driver probed\n");

	platform_set_drvdata(pdev, port);

	return uart_add_one_port(&nvt_uart_driver, port);
}

static int nvt_serial_remove(struct platform_device *pdev)
{
	struct uart_port *port = platform_get_drvdata(pdev);

	uart_remove_one_port(&nvt_uart_driver, port);

	return 0;
}

static const struct of_device_id nvt_match_table[] = {
	{ .compatible = "nvt.nvt_uart" },
	{ .compatible = "ns16550a" },
	{},
};

static struct platform_driver nvt_serial_platform_driver = {
	.probe		= nvt_serial_probe,
	.remove		= nvt_serial_remove,
	.driver		= {
		.name	= "nvt_serial",
		.of_match_table = nvt_match_table,
	},
};

static int __init nvt_serial_init(void)
{
	int ret;

	if (nvt_get_chip_id() == CHIP_NA51084) {
		UART_NR = 6;
	} else {
		UART_NR = 3;
	}

	nvt_uart_driver.nr = UART_NR;

	ret = uart_register_driver(&nvt_uart_driver);
	if (unlikely(ret))
		return ret;

	ret = platform_driver_register(&nvt_serial_platform_driver);
	if (unlikely(ret)) {
		uart_unregister_driver(&nvt_uart_driver);
		return ret;
	}

	pr_info("nvt_serial: driver initialized\n");

	return ret;
}

static void __exit nvt_serial_exit(void)
{
	platform_driver_unregister(&nvt_serial_platform_driver);
	uart_unregister_driver(&nvt_uart_driver);
}

module_init(nvt_serial_init);
module_exit(nvt_serial_exit);

MODULE_AUTHOR("Shawn Chou <shawn_chou@novatek.com.tw>");
MODULE_DESCRIPTION("Driver for NVT Soc");
MODULE_VERSION(DRIVER_VERSION);
MODULE_LICENSE("GPL");
