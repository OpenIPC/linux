/*
 * drivers/tty/serial/ambarella_uart.c
 *
 * Author: Anthony Ginger <hfjiang@ambarella.com>
 *
 * Copyright (C) 2004-2011, Ambarella, Inc.
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
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 */

#if defined(CONFIG_SERIAL_AMBARELLA_CONSOLE) && defined(CONFIG_MAGIC_SYSRQ)
#define SUPPORT_SYSRQ
#endif

#include <linux/module.h>
#include <linux/io.h>
#include <linux/of.h>
#include <linux/of_platform.h>
#include <linux/of_address.h>
#include <linux/clk.h>
#include <linux/syscore_ops.h>
#include <linux/console.h>
#include <linux/sysrq.h>
#include <linux/serial_reg.h>
#include <linux/tty.h>
#include <linux/tty_flip.h>
#include <linux/serial.h>
#include <linux/serial_core.h>
#include <linux/pinctrl/consumer.h>
#include <linux/delay.h>
#include <mach/hardware.h>
#include <plat/clk.h>
#include <plat/uart.h>
#include <linux/dmaengine.h>
#include <linux/dma-mapping.h>
#include <linux/dmapool.h>
#include <plat/dma.h>
#include <linux/slab.h>

#define AMBA_UART_RX_DMA_BUFFER_SIZE 4096
#define AMBA_UART_MIN_DMA			16
#define AMBA_UART_LSR_ANY			(UART_LSR_OE | UART_LSR_BI |UART_LSR_PE | UART_LSR_FE)

#define AMBARELLA_UART_RESET_FLAG		0 /* bit 0 */

struct ambarella_uart_port {
	struct uart_port port;
	struct clk *uart_pll;
	unsigned long flags;
	u32 msr_used : 1;
	u32 tx_fifo_fix : 1;
	u32 less_reg : 1;
	u32 txdma_used:1;
	u32 rxdma_used:1;
	u32 mcr;
	u32 c_cflag;
	/* following are for dma transfer */
	struct dma_chan *rx_dma_chan;
	struct dma_chan *tx_dma_chan;
	unsigned char *rx_dma_buf_virt;
	unsigned char *tx_dma_buf_virt;
	dma_addr_t rx_dma_buf_phys;
	dma_addr_t tx_dma_buf_phys;
	dma_addr_t buf_phys_a;
	dma_addr_t buf_phys_b;
	unsigned char *buf_virt_a;
	unsigned char *buf_virt_b;
	bool use_buf_b;
	struct dma_async_tx_descriptor *tx_dma_desc;
	struct dma_async_tx_descriptor *rx_dma_desc;
	dma_cookie_t tx_cookie;
	dma_cookie_t rx_cookie;
	int tx_bytes_requested;
	int rx_bytes_requested;
	int tx_in_progress;
};

static struct ambarella_uart_port ambarella_port[UART_INSTANCES];

static void __serial_ambarella_stop_tx(struct uart_port *port, u32 tx_fifo_fix)
{
	u32 ier, iir;

	if (tx_fifo_fix) {
		ier = amba_readl(port->membase + UART_IE_OFFSET);
		if ((ier & UART_IE_PTIME) != UART_IE_PTIME)
			amba_writel(port->membase + UART_IE_OFFSET,
					ier | (UART_IE_PTIME | UART_IE_ETBEI));

		iir = amba_readl(port->membase + UART_II_OFFSET);
		amba_writel(port->membase + UART_IE_OFFSET,
			ier & ~(UART_IE_PTIME | UART_IE_ETBEI));
		(void)(iir);
	} else {
		amba_clrbitsl(port->membase + UART_IE_OFFSET, UART_IE_ETBEI);
	}
}

static u32 __serial_ambarella_read_ms(struct uart_port *port, u32 tx_fifo_fix)
{
	u32 ier, ms;

	if (tx_fifo_fix) {
		ier = amba_readl(port->membase + UART_IE_OFFSET);
		if ((ier & UART_IE_EDSSI) != UART_IE_EDSSI)
			amba_writel(port->membase + UART_IE_OFFSET,
					ier | UART_IE_EDSSI);

		ms = amba_readl(port->membase + UART_MS_OFFSET);
		if ((ier & UART_IE_EDSSI) != UART_IE_EDSSI)
			amba_writel(port->membase + UART_IE_OFFSET, ier);
	} else {
		ms = amba_readl(port->membase + UART_MS_OFFSET);
	}

	return ms;
}

static void __serial_ambarella_enable_ms(struct uart_port *port)
{
	amba_setbitsl(port->membase + UART_IE_OFFSET, UART_IE_EDSSI);
}

static void __serial_ambarella_disable_ms(struct uart_port *port)
{
	amba_clrbitsl(port->membase + UART_IE_OFFSET, UART_IE_EDSSI);
}

static inline void wait_for_tx(struct uart_port *port)
{
	u32 ls;

	ls = amba_readl(port->membase + UART_LS_OFFSET);
	while ((ls & UART_LS_TEMT) != UART_LS_TEMT) {
		cpu_relax();
		ls = amba_readl(port->membase + UART_LS_OFFSET);
	}
}

static inline void wait_for_rx(struct uart_port *port)
{
	u32 ls;

	ls = amba_readl(port->membase + UART_LS_OFFSET);
	while ((ls & UART_LS_DR) != UART_LS_DR) {
		cpu_relax();
		ls = amba_readl(port->membase + UART_LS_OFFSET);
	}
}

static inline int tx_fifo_is_full(struct uart_port *port)
{
	return !(amba_readl(port->membase + UART_US_OFFSET) & UART_US_TFNF);
}

/* ==========================================================================*/
static void serial_ambarella_hw_setup(struct uart_port *port)
{
	struct ambarella_uart_port *amb_port;

	amb_port = (struct ambarella_uart_port *)(port->private_data);

	if (!test_and_set_bit(AMBARELLA_UART_RESET_FLAG, &amb_port->flags)) {
		clk_set_rate(amb_port->uart_pll, ambarella_clk_get_ref_freq());
		port->uartclk = clk_get_rate(amb_port->uart_pll);
		/* reset the whole UART only once */
		amba_writel(port->membase + UART_SRR_OFFSET, 0x01);
		mdelay(1);
		amba_writel(port->membase + UART_SRR_OFFSET, 0x00);
	}

	amba_writel(port->membase + UART_IE_OFFSET,
		DEFAULT_AMBARELLA_UART_IER | UART_IE_PTIME);
	amba_writel(port->membase + UART_FC_OFFSET,
		UART_FC_FIFOE | UART_FC_RX_2_TO_FULL |
		UART_FC_TX_EMPTY | UART_FC_XMITR | UART_FC_RCVRR);
	amba_writel(port->membase + UART_IE_OFFSET,
		DEFAULT_AMBARELLA_UART_IER);

	if (amb_port->txdma_used || amb_port->rxdma_used) {
		/* we must use 14bytes as trigger, because if we use 8bytes, FIFO may
		* be empty due to dma burst size is also set to 8bytes before dma complete,
		* in this case, if FIFO is empty, but no more data is received, timeout irq
		* can't be trigged after, we don't have chance to push the data that dma has
		* transfered any more.
		*/
		amba_writel(port->membase + UART_FC_OFFSET,
			UART_FC_FIFOE | UART_FC_RX_2_TO_FULL |
			UART_FC_TX_EMPTY | UART_FC_XMITR |
			UART_FC_RCVRR |UART_FCR_DMA_SELECT);
		amba_writel(port->membase + UART_DMAE_OFFSET,
			(amb_port->txdma_used << 1) | amb_port->rxdma_used);
	}
}

static inline void serial_ambarella_receive_chars(struct uart_port *port,
	u32 tmo)
{
	u32 ch, flag, ls;
	int max_count;

	ls = amba_readl(port->membase + UART_LS_OFFSET);
	max_count = port->fifosize;

	do {
		flag = TTY_NORMAL;
		if (unlikely(ls & (UART_LS_BI | UART_LS_PE |
					UART_LS_FE | UART_LS_OE))) {
			if (ls & UART_LS_BI) {
				ls &= ~(UART_LS_FE | UART_LS_PE);
				port->icount.brk++;

				if (uart_handle_break(port))
					goto ignore_char;
			}
			if (ls & UART_LS_FE)
				port->icount.frame++;
			if (ls & UART_LS_PE)
				port->icount.parity++;
			if (ls & UART_LS_OE)
				port->icount.overrun++;

			ls &= port->read_status_mask;

			if (ls & UART_LS_BI)
				flag = TTY_BREAK;
			else if (ls & UART_LS_FE)
				flag = TTY_FRAME;
			else if (ls & UART_LS_PE)
				flag = TTY_PARITY;
			else if (ls & UART_LS_OE)
				flag = TTY_OVERRUN;

			if (ls & UART_LS_OE) {
				printk(KERN_DEBUG "%s: OVERFLOW\n", __func__);
			}
		}

		if (likely(ls & UART_LS_DR)) {
			ch = amba_readl(port->membase + UART_RB_OFFSET);
			port->icount.rx++;
			tmo = 0;

			if (uart_handle_sysrq_char(port, ch))
				goto ignore_char;

			uart_insert_char(port, ls, UART_LS_OE, ch, flag);
		} else {
			if (tmo) {
				ch = amba_readl(port->membase + UART_RB_OFFSET);
				/* printk(KERN_DEBUG "False TMO get %d\n", ch); */
			}
		}

ignore_char:
		ls = amba_readl(port->membase + UART_LS_OFFSET);
	} while ((ls & UART_LS_DR) && (max_count-- > 0));

	spin_unlock(&port->lock);
	tty_flip_buffer_push(&port->state->port);
	spin_lock(&port->lock);
}

static void serial_ambarella_transmit_chars(struct uart_port *port)
{
	struct circ_buf *xmit = &port->state->xmit;
	struct ambarella_uart_port *amb_port;
	int count;

	amb_port = (struct ambarella_uart_port *)(port->private_data);

	if (port->x_char) {
		amba_writel(port->membase + UART_TH_OFFSET, port->x_char);
		port->icount.tx++;
		port->x_char = 0;
		return;
	}

	if (uart_tx_stopped(port) || uart_circ_empty(xmit)) {
		__serial_ambarella_stop_tx(port, amb_port->tx_fifo_fix);
		return;
	}

	count = port->fifosize;
	while (count-- > 0) {
		if (!amb_port->less_reg && tx_fifo_is_full(port))
			break;
		amba_writel(port->membase + UART_TH_OFFSET, xmit->buf[xmit->tail]);
		xmit->tail = (xmit->tail + 1) & (UART_XMIT_SIZE - 1);
		port->icount.tx++;
		if (uart_circ_empty(xmit))
			break;
	}

	if (uart_circ_chars_pending(xmit) < WAKEUP_CHARS)
		uart_write_wakeup(port);
	if (uart_circ_empty(xmit))
		__serial_ambarella_stop_tx(port, amb_port->tx_fifo_fix);
}

static inline void serial_ambarella_check_modem_status(struct uart_port *port)
{
	struct ambarella_uart_port *amb_port;
	u32 ms;

	amb_port = (struct ambarella_uart_port *)(port->private_data);

	if (amb_port->msr_used) {
		ms = __serial_ambarella_read_ms(port, amb_port->tx_fifo_fix);

		if (ms & UART_MS_RI)
			port->icount.rng++;
		if (ms & UART_MS_DSR)
			port->icount.dsr++;
		if (ms & UART_MS_DCTS)
			uart_handle_cts_change(port, (ms & UART_MS_CTS));
		if (ms & UART_MS_DDCD)
			uart_handle_dcd_change(port, (ms & UART_MS_DCD));

		wake_up_interruptible(&port->state->port.delta_msr_wait);
	}
}

static char serial_ambarella_decode_rx_error(struct ambarella_uart_port *amb_port,
			unsigned long lsr)
{
	char flag = TTY_NORMAL;

	if (unlikely(lsr & AMBA_UART_LSR_ANY)) {
		if (lsr & UART_LSR_OE) {
			/* Overrrun error */
			flag |= TTY_OVERRUN;
			amb_port->port.icount.overrun++;
			dev_err(amb_port->port.dev, "Got overrun errors\n");
		} else if (lsr & UART_LSR_PE) {
			/* Parity error */
			flag |= TTY_PARITY;
			amb_port->port.icount.parity++;
			dev_err(amb_port->port.dev, "Got Parity errors\n");
		} else if (lsr & UART_LSR_FE) {
			flag |= TTY_FRAME;
			amb_port->port.icount.frame++;
			dev_err(amb_port->port.dev, "Got frame errors\n");
		} else if (lsr & UART_LSR_BI) {
			dev_err(amb_port->port.dev, "Got Break\n");
			amb_port->port.icount.brk++;
			/* If FIFO read error without any data, reset Rx FIFO */
			if (!(lsr & UART_LSR_DR) && (lsr & UART_LSR_FIFOE))
				amba_writel(amb_port->port.membase + UART_SRR_OFFSET,
					UART_FCR_CLEAR_RCVR);
		}
	}
	return flag;
}

static void serial_ambarella_handle_rx_pio(struct ambarella_uart_port *amb_port,
		struct tty_port *tty)
{
	do {
		char flag = TTY_NORMAL;
		unsigned long lsr = 0;
		unsigned char ch;

		lsr = amba_readl(amb_port->port.membase + UART_LS_OFFSET);
		if (!(lsr & UART_LS_DR))
			break;

		flag = serial_ambarella_decode_rx_error(amb_port, lsr);
		ch = (unsigned char) amba_readl(amb_port->port.membase + UART_RB_OFFSET);
		amb_port->port.icount.rx++;

		if (!uart_handle_sysrq_char(&amb_port->port, ch) && tty)
			tty_insert_flip_char(tty, ch, flag);
	} while (1);

	return;
}

static void serial_ambarella_copy_rx_to_tty(struct ambarella_uart_port *amb_port,
		struct tty_port *tty, int count);
static int serial_ambarella_start_rx_dma(struct ambarella_uart_port *amb_port);

static void serial_ambarella_dma_rx_irq(struct ambarella_uart_port *amb_port)
{
	struct tty_port *port = &amb_port->port.state->port;
	struct tty_struct *tty = tty_port_tty_get(&amb_port->port.state->port);
	struct dma_tx_state state;
	size_t pending;

	dmaengine_tx_status(amb_port->rx_dma_chan, amb_port->rx_cookie, &state);
	dmaengine_terminate_all(amb_port->rx_dma_chan);

	pending = amb_port->rx_bytes_requested - state.residue;
	BUG_ON(pending > AMBA_UART_RX_DMA_BUFFER_SIZE);

	/* switch the rx buffer */
	amb_port->use_buf_b = !amb_port->use_buf_b;

	serial_ambarella_start_rx_dma(amb_port);

	serial_ambarella_copy_rx_to_tty(amb_port, port, pending);

	serial_ambarella_handle_rx_pio(amb_port, port);

	if (tty) {
		tty_flip_buffer_push(port);
		tty_kref_put(tty);
	}
}

static void serial_ambarella_start_tx(struct uart_port *port);

static irqreturn_t serial_ambarella_irq(int irq, void *dev_id)
{
	struct uart_port *port = dev_id;
	int rval = IRQ_HANDLED;
	u32 ii;
	unsigned long flags;

	struct ambarella_uart_port *amb_port;
	amb_port = (struct ambarella_uart_port *)(port->private_data);

	spin_lock_irqsave(&port->lock, flags);

	ii = amba_readl(port->membase + UART_II_OFFSET);
	switch (ii & 0x0F) {
	case UART_II_MODEM_STATUS_CHANGED:
		serial_ambarella_check_modem_status(port);
		break;

	case UART_II_THR_EMPTY:
		if (amb_port->txdma_used)
			serial_ambarella_start_tx(port);
		else
			serial_ambarella_transmit_chars(port);
		break;

	case UART_II_RCV_STATUS:
	case UART_II_RCV_DATA_AVAIL:
		serial_ambarella_receive_chars(port, 0);
		break;
	case UART_II_CHAR_TIMEOUT:
		if (amb_port->rxdma_used){
			struct ambarella_uart_port *amb_port;
			amb_port = (struct ambarella_uart_port *)(port->private_data);
			serial_ambarella_dma_rx_irq(amb_port);
		} else {
			serial_ambarella_receive_chars(port, 1);
		}
		break;

	case UART_II_NO_INT_PENDING:
		break;
	default:
		printk(KERN_DEBUG "%s: 0x%x\n", __func__, ii);
		break;
	}

	spin_unlock_irqrestore(&port->lock, flags);

	return rval;
}

/* ==========================================================================*/
static void serial_ambarella_enable_ms(struct uart_port *port)
{
	__serial_ambarella_enable_ms(port);
}

static void serial_ambarella_start_next_tx(struct ambarella_uart_port *amb_port);

static void serial_ambarella_tx_dma_complete(void *args)
{
	struct ambarella_uart_port *amb_port = args;
	struct circ_buf *xmit = &amb_port->port.state->xmit;
	struct dma_tx_state state;
	unsigned long flags;
	int count;

	dmaengine_tx_status(amb_port->tx_dma_chan, amb_port->tx_cookie, &state);
	count = amb_port->tx_bytes_requested - state.residue;
	spin_lock_irqsave(&amb_port->port.lock, flags);
	xmit->tail = (xmit->tail + count) & (UART_XMIT_SIZE - 1);
	amb_port->tx_in_progress = 0;
	amb_port->port.icount.tx += count;
	if (uart_circ_chars_pending(xmit) < WAKEUP_CHARS)
		uart_write_wakeup(&amb_port->port);
	serial_ambarella_start_next_tx(amb_port);
	spin_unlock_irqrestore(&amb_port->port.lock, flags);
}

static int serial_ambarella_start_tx_dma(struct ambarella_uart_port *amb_port,
		unsigned long count)
{
	struct circ_buf *xmit = &amb_port->port.state->xmit;
	dma_addr_t tx_phys_addr;
	int tx_bytes;

	dma_sync_single_for_device(amb_port->port.dev, amb_port->tx_dma_buf_phys,
				UART_XMIT_SIZE, DMA_TO_DEVICE);

	tx_bytes = count & ~(0xF);
	tx_phys_addr = amb_port->tx_dma_buf_phys + xmit->tail;
	amb_port->tx_dma_desc = dmaengine_prep_slave_single(amb_port->tx_dma_chan,
				tx_phys_addr, tx_bytes, DMA_MEM_TO_DEV,
				DMA_PREP_INTERRUPT | DMA_COMPL_SKIP_SRC_UNMAP |
				DMA_COMPL_SKIP_DEST_UNMAP | DMA_CTRL_ACK);
	if (!amb_port->tx_dma_desc) {
		dev_err(amb_port->port.dev, "Not able to get desc for Tx\n");
		return -EIO;
	}

	amb_port->tx_dma_desc->callback = serial_ambarella_tx_dma_complete;
	amb_port->tx_dma_desc->callback_param = amb_port;
	amb_port->tx_bytes_requested = tx_bytes;
	amb_port->tx_in_progress = 1;
	amb_port->tx_cookie = dmaengine_submit(amb_port->tx_dma_desc);
	dma_async_issue_pending(amb_port->tx_dma_chan);

	return 0;
}

static void serial_ambarella_start_next_tx(struct ambarella_uart_port *amb_port)
{
	struct circ_buf *xmit = &amb_port->port.state->xmit;
	struct uart_port *port;
	unsigned long tail, count;

	port = &amb_port->port;

	tail = (unsigned long)&xmit->buf[xmit->tail];
	count = CIRC_CNT_TO_END(xmit->head, xmit->tail, UART_XMIT_SIZE);
	if (!count)
		return;

	wait_for_tx(port);
	if (count < AMBA_UART_MIN_DMA)
		serial_ambarella_transmit_chars(port);
	else
		serial_ambarella_start_tx_dma(amb_port, count);
}

static void serial_ambarella_start_tx(struct uart_port *port)
{
	struct ambarella_uart_port *amb_port;
	struct tty_struct *tty;

	amb_port = (struct ambarella_uart_port *)(port->private_data);
	tty = tty_port_tty_get(&amb_port->port.state->port);

	amba_setbitsl(port->membase + UART_IE_OFFSET, UART_IE_ETBEI);

	if (amb_port->txdma_used) {
		if (tty->hw_stopped ||amb_port->tx_in_progress)
			return;
		serial_ambarella_start_next_tx(amb_port);
	} else {
		/* if FIFO status register is not provided, we have no idea about
	 	* the Tx FIFO is full or not, so we need to wait for the Tx Empty
		 * Interrupt comming, then we can start to transfer data. */
		if (amb_port->less_reg)
			return;

		serial_ambarella_transmit_chars(port);
	}
}

static void serial_ambarella_copy_rx_to_tty(struct ambarella_uart_port *amb_port,
		struct tty_port *tty, int count)
{
	dma_addr_t old_buf_phys;
	unsigned char *old_buf_virt;

	amb_port->port.icount.rx += count;
	if (!tty) {
		dev_err(amb_port->port.dev, "No tty port\n");
		return;
	}

	/* use old buffer to copy data to tty, and use new buffer to receive */
	if (amb_port->use_buf_b) {
		old_buf_phys = amb_port->buf_phys_a;
		old_buf_virt = amb_port->buf_virt_a;
	} else {
		old_buf_phys = amb_port->buf_phys_b;
		old_buf_virt = amb_port->buf_virt_b;
	}

	dma_sync_single_for_cpu(amb_port->port.dev, old_buf_phys,
		AMBA_UART_RX_DMA_BUFFER_SIZE, DMA_FROM_DEVICE);

	tty_insert_flip_string(tty, old_buf_virt, count);
}

static void serial_ambarella_rx_dma_complete(void *args)
{
	struct ambarella_uart_port *amb_port = args;
	struct tty_struct *tty = tty_port_tty_get(&amb_port->port.state->port);
	struct uart_port *u = &amb_port->port;
	struct tty_port *port = &u->state->port;
	int count = amb_port->rx_bytes_requested;
	unsigned long flags;

	/* switch the rx buffer */
	amb_port->use_buf_b = !amb_port->use_buf_b;
	serial_ambarella_start_rx_dma(amb_port);

	serial_ambarella_copy_rx_to_tty(amb_port, port, count);

	spin_lock_irqsave(&u->lock, flags);
	if (tty) {
		tty_flip_buffer_push(port);
		tty_kref_put(tty);
	}
	spin_unlock_irqrestore(&u->lock, flags);
}

static int serial_ambarella_start_rx_dma(struct ambarella_uart_port *amb_port)
{
	unsigned int count = AMBA_UART_RX_DMA_BUFFER_SIZE;

	if (amb_port->use_buf_b) {
		amb_port->rx_dma_buf_virt = amb_port->buf_virt_b;
		amb_port->rx_dma_buf_phys = amb_port->buf_phys_b;
	} else {
		amb_port->rx_dma_buf_virt = amb_port->buf_virt_a;
		amb_port->rx_dma_buf_phys = amb_port->buf_phys_a;
	}

	amb_port->rx_dma_desc = dmaengine_prep_slave_single(amb_port->rx_dma_chan,
				amb_port->rx_dma_buf_phys, count, DMA_DEV_TO_MEM,
				DMA_PREP_INTERRUPT | DMA_CTRL_ACK| DMA_COMPL_SKIP_SRC_UNMAP |
				DMA_COMPL_SKIP_DEST_UNMAP);

	if (!amb_port->rx_dma_desc) {
		dev_err(amb_port->port.dev, "Not able to get desc for Rx\n");
		return -EIO;
	}

	amb_port->rx_dma_desc->callback = serial_ambarella_rx_dma_complete;
	amb_port->rx_dma_desc->callback_param = amb_port;
	dma_sync_single_for_device(amb_port->port.dev, amb_port->rx_dma_buf_phys,
				count, DMA_TO_DEVICE);
	amb_port->rx_bytes_requested = count;
	amb_port->rx_cookie = dmaengine_submit(amb_port->rx_dma_desc);
	dma_async_issue_pending(amb_port->rx_dma_chan);

	return 0;
}

static void serial_ambarella_stop_tx(struct uart_port *port)
{
	struct ambarella_uart_port *amb_port;

	amb_port = (struct ambarella_uart_port *)(port->private_data);

	__serial_ambarella_stop_tx(port, amb_port->tx_fifo_fix);
}

static void serial_ambarella_stop_rx(struct uart_port *port)
{
	amba_clrbitsl(port->membase + UART_IE_OFFSET, UART_IE_ERBFI);
}

static unsigned int serial_ambarella_tx_empty(struct uart_port *port)
{
	unsigned int lsr;
	unsigned long flags;

	spin_lock_irqsave(&port->lock, flags);
	lsr = amba_readl(port->membase + UART_LS_OFFSET);
	spin_unlock_irqrestore(&port->lock, flags);

	return ((lsr & (UART_LS_TEMT | UART_LS_THRE)) ==
		(UART_LS_TEMT | UART_LS_THRE)) ? TIOCSER_TEMT : 0;
}

static unsigned int serial_ambarella_get_mctrl(struct uart_port *port)
{
	struct ambarella_uart_port *amb_port;
	u32 ms, mctrl = 0;

	amb_port = (struct ambarella_uart_port *)(port->private_data);

	if (amb_port->msr_used) {
		ms = __serial_ambarella_read_ms(port, amb_port->tx_fifo_fix);

		if (ms & UART_MS_CTS)
			mctrl |= TIOCM_CTS;
		if (ms & UART_MS_DSR)
			mctrl |= TIOCM_DSR;
		if (ms & UART_MS_RI)
			mctrl |= TIOCM_RI;
		if (ms & UART_MS_DCD)
			mctrl |= TIOCM_CD;
	}

	return mctrl;
}

static void serial_ambarella_set_mctrl(struct uart_port *port,
	unsigned int mctrl)
{
	struct ambarella_uart_port *amb_port;
	u32 mcr, mcr_new = 0;

	amb_port = (struct ambarella_uart_port *)(port->private_data);

	if (amb_port->msr_used) {
		mcr = amba_readl(port->membase + UART_MC_OFFSET);

		if (mctrl & TIOCM_DTR)
			mcr_new |= UART_MC_DTR;
		if (mctrl & TIOCM_RTS)
			mcr_new |= UART_MC_RTS;
		if (mctrl & TIOCM_OUT1)
			mcr_new |= UART_MC_OUT1;
		if (mctrl & TIOCM_OUT2)
			mcr_new |= UART_MC_OUT2;
		if (mctrl & TIOCM_LOOP)
			mcr_new |= UART_MC_LB;

		mcr_new |= amb_port->mcr;
		if (mcr_new != mcr) {
			if ((mcr & UART_MC_AFCE) == UART_MC_AFCE) {
				mcr &= ~UART_MC_AFCE;
				amba_writel(port->membase + UART_MC_OFFSET,
					mcr);
			}
			amba_writel(port->membase + UART_MC_OFFSET, mcr_new);
		}
	}
}

static void serial_ambarella_break_ctl(struct uart_port *port, int break_state)
{
	unsigned long flags;

	spin_lock_irqsave(&port->lock, flags);
	if (break_state != 0)
		amba_setbitsl(port->membase + UART_LC_OFFSET, UART_LC_BRK);
	else
		amba_clrbitsl(port->membase + UART_LC_OFFSET, UART_LC_BRK);
	spin_unlock_irqrestore(&port->lock, flags);
}

static bool serial_ambarella_dma_filter(struct dma_chan *chan, void *fparam)
{
	int chan_id;
	bool ret = false;

	chan_id = *(int *)fparam;

	if (ambarella_dma_channel_id(chan) == chan_id)
		ret = true;

	return ret;
}

static int serial_ambarella_dma_channel_allocate(struct ambarella_uart_port *amb_port,
			bool dma_to_memory)
{
	struct dma_chan *dma_chan;
	struct dma_slave_config dma_sconfig;
	dma_addr_t dma_phys;
	dma_cap_mask_t mask;
	unsigned char *dma_buf;
	int ret, chan_id;

	if (dma_to_memory)
		chan_id = UART_RX_DMA_CHAN;
	else
		chan_id = UART_TX_DMA_CHAN;

	dma_cap_zero(mask);
	dma_cap_set(DMA_SLAVE, mask);
	dma_chan = dma_request_channel(mask, serial_ambarella_dma_filter, &chan_id);
	if (!dma_chan) {
		dev_err(amb_port->port.dev,
			"Dma channel is not available, will try later\n");
		return -EPROBE_DEFER;
	}

	if (dma_to_memory) {
		/* use double buffer to handle rx */
		dma_buf = kmalloc(AMBA_UART_RX_DMA_BUFFER_SIZE, GFP_KERNEL);
		dma_phys = virt_to_phys(dma_buf);
		if (!dma_buf) {
			dev_err(amb_port->port.dev,
				"Not able to allocate the dma buffer a\n");
			dma_release_channel(dma_chan);
			return -ENOMEM;
		}

		amb_port->buf_virt_a = dma_buf;
		amb_port->buf_phys_a = dma_phys;

		/* buffer b */
		dma_buf = kmalloc(AMBA_UART_RX_DMA_BUFFER_SIZE, GFP_KERNEL);
		dma_phys = virt_to_phys(dma_buf);
		if (!dma_buf) {
			dev_err(amb_port->port.dev,
				"Not able to allocate the dma buffer b\n");
			dma_release_channel(dma_chan);
			return -ENOMEM;
		}

		amb_port->buf_virt_b = dma_buf;
		amb_port->buf_phys_b = dma_phys;

	} else {
		dma_phys = dma_map_single(amb_port->port.dev,
			amb_port->port.state->xmit.buf, UART_XMIT_SIZE,
			DMA_TO_DEVICE);
		dma_buf = amb_port->port.state->xmit.buf;
	}

	if (dma_to_memory) {
		dma_sconfig.src_addr = amb_port->port.mapbase + UART_DMAF_OFFSET;
		dma_sconfig.src_addr_width = DMA_SLAVE_BUSWIDTH_1_BYTE;
		dma_sconfig.src_maxburst = 8;
		dma_sconfig.direction = DMA_DEV_TO_MEM;
	} else {
		dma_sconfig.dst_addr = amb_port->port.mapbase + UART_DMAF_OFFSET;
		dma_sconfig.dst_addr_width = DMA_SLAVE_BUSWIDTH_1_BYTE;
		dma_sconfig.dst_maxburst = 16;
		dma_sconfig.direction = DMA_MEM_TO_DEV;
	}

	ret = dmaengine_slave_config(dma_chan, &dma_sconfig);
	if (ret < 0) {
		dev_err(amb_port->port.dev,
			"Dma slave config failed, err = %d\n", ret);
		goto scrub;
	}

	if (dma_to_memory) {
		amb_port->rx_dma_chan = dma_chan;
		amb_port->rx_dma_buf_virt = amb_port->buf_virt_a;
		amb_port->rx_dma_buf_phys = amb_port->buf_phys_a;
	} else {
		amb_port->tx_dma_chan = dma_chan;
		amb_port->tx_dma_buf_virt = dma_buf;
		amb_port->tx_dma_buf_phys = dma_phys;
	}
	return 0;

scrub:
	dma_release_channel(dma_chan);
	return ret;
}

static int serial_ambarella_startup(struct uart_port *port)
{
	int rval = 0;
	struct ambarella_uart_port *amb_port;

	amb_port = (struct ambarella_uart_port *)(port->private_data);

	serial_ambarella_hw_setup(port);

	rval = request_irq(port->irq, serial_ambarella_irq,
		IRQF_TRIGGER_HIGH, dev_name(port->dev), port);

	if (amb_port->txdma_used) {
		rval = serial_ambarella_dma_channel_allocate(amb_port, false);
		if (rval < 0) {
			dev_err(amb_port->port.dev, "Tx Dma allocation failed, err = %d\n", rval);
			return -EBUSY;
		}
	}

	if (amb_port->rxdma_used) {
		amb_port->use_buf_b = false;

		rval = serial_ambarella_dma_channel_allocate(amb_port, true);
		if (rval < 0) {
			dev_err(amb_port->port.dev, "Rx Dma allocation failed, err = %d\n", rval);
			return -EBUSY;
		}

		rval = serial_ambarella_start_rx_dma(amb_port);
		if (rval < 0) {
			dev_err(amb_port->port.dev, "Not able to start Rx DMA\n");
			return rval;
		}
	}

	return rval;
}

static void serial_ambarella_hw_deinit(struct ambarella_uart_port *amb_port)
{
	struct uart_port *port = &amb_port->port;

	/* Disable interrupts */
	amba_writel(port->membase + UART_IE_OFFSET, 0);
	/* Reset the Rx and Tx FIFOs */
	amba_writel(port->membase + UART_SRR_OFFSET,
		UART_FCR_CLEAR_XMIT| UART_FCR_CLEAR_RCVR);
}

static void serial_ambarella_dma_channel_free(struct ambarella_uart_port *amb_port,
		bool dma_to_memory)
{
	struct dma_chan *dma_chan;

	if (dma_to_memory) {
		kfree(amb_port->buf_virt_a);
		kfree(amb_port->buf_virt_b);

		dma_chan = amb_port->rx_dma_chan;
		amb_port->rx_dma_chan = NULL;
		amb_port->rx_dma_buf_phys = 0;
		amb_port->rx_dma_buf_virt = NULL;
		amb_port->buf_phys_a = 0;
		amb_port->buf_virt_a = NULL;
		amb_port->buf_phys_b = 0;
		amb_port->buf_virt_b = NULL;
	} else {
		dma_unmap_single(amb_port->port.dev, amb_port->tx_dma_buf_phys,
			UART_XMIT_SIZE, DMA_TO_DEVICE);
		dma_chan = amb_port->tx_dma_chan;
		amb_port->tx_dma_chan = NULL;
		amb_port->tx_dma_buf_phys = 0;
		amb_port->tx_dma_buf_virt = NULL;
	}
	dma_release_channel(dma_chan);
}

static void serial_ambarella_shutdown(struct uart_port *port)
{
	struct ambarella_uart_port *amb_port;
	unsigned long flags;

	amb_port = (struct ambarella_uart_port *)(port->private_data);

	spin_lock_irqsave(&port->lock, flags);

	if (amb_port->txdma_used || amb_port->rxdma_used) {
		serial_ambarella_hw_deinit(amb_port);
		spin_unlock_irqrestore(&port->lock, flags);

		if (amb_port->txdma_used) {
			dmaengine_terminate_all(amb_port->tx_dma_chan);
			serial_ambarella_dma_channel_free(amb_port, false);
			amb_port->tx_in_progress = 0;
		}
		if (amb_port->rxdma_used) {
			dmaengine_terminate_all(amb_port->rx_dma_chan);
			serial_ambarella_dma_channel_free(amb_port, true);
		}

		spin_lock_irqsave(&port->lock, flags);
	}

	amba_clrbitsl(port->membase + UART_LC_OFFSET, UART_LC_BRK);
	spin_unlock_irqrestore(&port->lock, flags);

	free_irq(port->irq, port);
}

static void serial_ambarella_set_termios(struct uart_port *port,
	struct ktermios *termios, struct ktermios *old)
{
	struct ambarella_uart_port *amb_port;
	unsigned int baud, quot;
	u32 lc = 0x0;

	amb_port = (struct ambarella_uart_port *)(port->private_data);
	amb_port->c_cflag = termios->c_cflag;

	port->uartclk = clk_get_rate(amb_port->uart_pll);
	switch (termios->c_cflag & CSIZE) {
	case CS5:
		lc |= UART_LC_CLS_5_BITS;
		break;
	case CS6:
		lc |= UART_LC_CLS_6_BITS;
		break;
	case CS7:
		lc |= UART_LC_CLS_7_BITS;
		break;
	case CS8:
	default:
		lc |= UART_LC_CLS_8_BITS;
		break;
	}

	if (termios->c_cflag & CSTOPB)
		lc |= UART_LC_STOP_2BIT;
	else
		lc |= UART_LC_STOP_1BIT;

	if (termios->c_cflag & PARENB) {
		if (termios->c_cflag & PARODD)
			lc |= (UART_LC_PEN | UART_LC_ODD_PARITY);
		else
			lc |= (UART_LC_PEN | UART_LC_EVEN_PARITY);
	}

	baud = uart_get_baud_rate(port, termios, old, 0, port->uartclk / 16);
	quot = uart_get_divisor(port, baud);

	disable_irq(port->irq);
	uart_update_timeout(port, termios->c_cflag, baud);

	port->read_status_mask = UART_LSR_OE | UART_LSR_THRE | UART_LSR_DR;
	if (termios->c_iflag & INPCK)
		port->read_status_mask |= UART_LSR_FE | UART_LSR_PE;
	if (termios->c_iflag & (BRKINT | PARMRK))
		port->read_status_mask |= UART_LSR_BI;

	port->ignore_status_mask = 0;
	if (termios->c_iflag & IGNPAR)
		port->ignore_status_mask |= UART_LSR_PE | UART_LSR_FE;
	if (termios->c_iflag & IGNBRK) {
		port->ignore_status_mask |= UART_LSR_BI;
		if (termios->c_iflag & IGNPAR)
			port->ignore_status_mask |= UART_LSR_OE;
	}
	if ((termios->c_cflag & CREAD) == 0)
		port->ignore_status_mask |= UART_LSR_DR;

	if ((termios->c_cflag & CRTSCTS) == 0)
		amb_port->mcr &= ~UART_MC_AFCE;
	else
		amb_port->mcr |= UART_MC_AFCE;

	amba_writel(port->membase + UART_LC_OFFSET, UART_LC_DLAB);
	amba_writel(port->membase + UART_DLL_OFFSET, quot & 0xff);
	amba_writel(port->membase + UART_DLH_OFFSET, (quot >> 8) & 0xff);
	amba_writel(port->membase + UART_LC_OFFSET, lc);
	if (UART_ENABLE_MS(port, termios->c_cflag))
		__serial_ambarella_enable_ms(port);
	else
		__serial_ambarella_disable_ms(port);
	serial_ambarella_set_mctrl(port, port->mctrl);

	enable_irq(port->irq);
}

static void serial_ambarella_pm(struct uart_port *port,
	unsigned int state, unsigned int oldstate)
{
}

static void serial_ambarella_release_port(struct uart_port *port)
{
}

static int serial_ambarella_request_port(struct uart_port *port)
{
	return 0;
}

static void serial_ambarella_config_port(struct uart_port *port, int flags)
{
}

static int serial_ambarella_verify_port(struct uart_port *port,
					struct serial_struct *ser)
{
	int rval = 0;

	if (ser->type != PORT_UNKNOWN && ser->type != PORT_UART00)
		rval = -EINVAL;
	if (port->irq != ser->irq)
		rval = -EINVAL;
	if (ser->io_type != SERIAL_IO_MEM)
		rval = -EINVAL;

	return rval;
}

static const char *serial_ambarella_type(struct uart_port *port)
{
	return "ambuart";
}

#ifdef CONFIG_CONSOLE_POLL
static void serial_ambarella_poll_put_char(struct uart_port *port,
	unsigned char chr)
{
	struct ambarella_uart_port *amb_port;

	amb_port = (struct ambarella_uart_port *)(port->private_data);
	if (!port->suspended) {
		wait_for_tx(port);
		amba_writel(port->membase + UART_TH_OFFSET, chr);
	}
}

static int serial_ambarella_poll_get_char(struct uart_port *port)
{
	struct ambarella_uart_port *amb_port;

	amb_port = (struct ambarella_uart_port *)(port->private_data);
	if (!port->suspended) {
		wait_for_rx(port);
		return amba_readl(port->membase + UART_RB_OFFSET);
	}
	return 0;
}
#endif

struct uart_ops serial_ambarella_pops = {
	.tx_empty	= serial_ambarella_tx_empty,
	.set_mctrl	= serial_ambarella_set_mctrl,
	.get_mctrl	= serial_ambarella_get_mctrl,
	.stop_tx	= serial_ambarella_stop_tx,
	.start_tx	= serial_ambarella_start_tx,
	.stop_rx	= serial_ambarella_stop_rx,
	.enable_ms	= serial_ambarella_enable_ms,
	.break_ctl	= serial_ambarella_break_ctl,
	.startup	= serial_ambarella_startup,
	.shutdown	= serial_ambarella_shutdown,
	.set_termios	= serial_ambarella_set_termios,
	.pm		= serial_ambarella_pm,
	.type		= serial_ambarella_type,
	.release_port	= serial_ambarella_release_port,
	.request_port	= serial_ambarella_request_port,
	.config_port	= serial_ambarella_config_port,
	.verify_port	= serial_ambarella_verify_port,
#ifdef CONFIG_CONSOLE_POLL
	.poll_put_char	= serial_ambarella_poll_put_char,
	.poll_get_char	= serial_ambarella_poll_get_char,
#endif
};

/* ==========================================================================*/
#if defined(CONFIG_SERIAL_AMBARELLA_CONSOLE)

static struct uart_driver serial_ambarella_reg;
static void ambarella_uart_of_enumerate(void);

static void serial_ambarella_console_putchar(struct uart_port *port, int ch)
{
	wait_for_tx(port);
	amba_writel(port->membase + UART_TH_OFFSET, ch);
}

static void serial_ambarella_console_write(struct console *co,
	const char *s, unsigned int count)
{
	struct uart_port *port;
	int locked = 1;
	unsigned long flags;

	port = &ambarella_port[co->index].port;

	if (!port->suspended) {
		local_irq_save(flags);
		if (port->sysrq) {
			locked = 0;
		} else if (oops_in_progress) {
			locked = spin_trylock(&port->lock);
		} else {
			spin_lock(&port->lock);
			locked = 1;
		}

		uart_console_write(port, s, count,
			serial_ambarella_console_putchar);
		wait_for_tx(port);

		if (locked)
			spin_unlock(&port->lock);
		local_irq_restore(flags);
	}
}

static int __init serial_ambarella_console_setup(struct console *co,
	char *options)
{
	struct ambarella_uart_port *amb_port;
	struct uart_port *port;
	int baud = 115200, bits = 8, parity = 'n', flow = 'n';

	if (co->index < 0 || co->index >= serial_ambarella_reg.nr)
		co->index = 0;

	amb_port = &ambarella_port[co->index];
	amb_port->uart_pll = clk_get(NULL, "gclk_uart");

	port = &ambarella_port[co->index].port;
	if (!port->membase) {
		pr_err("No device available for console\n");
		return -ENODEV;
	}

	port->ops = &serial_ambarella_pops;
	port->private_data = &ambarella_port[co->index];
	port->line = co->index;

	serial_ambarella_hw_setup(port);

	if (options)
		uart_parse_options(options, &baud, &parity, &bits, &flow);

	return uart_set_options(port, co, baud, parity, bits, flow);
}

static struct console serial_ambarella_console = {
	.name		= "ttyS",
	.write		= serial_ambarella_console_write,
	.device		= uart_console_device,
	.setup		= serial_ambarella_console_setup,
	.flags		= CON_PRINTBUFFER | CON_ANYTIME,
	.index		= -1,
	.data		= &serial_ambarella_reg,
};

#ifdef CONFIG_PM
/* when no_console_suspend is specified in cmdline, Linux thought the console
 * would never suspend. But actually we may put system into STR and power off
 * the SoC, so the console must be initialized when system exit STR.
 *
 * NOTE: We MUST ensure that no information will be output via UART until
 * syscore_resume() is completed, otherwise the UART may be in deadloop.
 */
static void serial_ambarella_console_resume(void)
{
	struct console *co = &serial_ambarella_console;
	struct ambarella_uart_port *amb_port;
	struct uart_port *port;
	struct ktermios termios;

	if (console_suspend_enabled)
		return;

	if (co->index < 0 || co->index >= serial_ambarella_reg.nr)
		return;

	amb_port = &ambarella_port[co->index];

	port = &ambarella_port[co->index].port;
	if (!port->membase)
		return;

	clear_bit(AMBARELLA_UART_RESET_FLAG, &amb_port->flags);
	serial_ambarella_hw_setup(port);

	memset(&termios, 0, sizeof(struct ktermios));
	termios.c_cflag = amb_port->c_cflag;
	port->ops->set_termios(port, &termios, NULL);
}

static struct syscore_ops ambarella_console_syscore_ops = {
	.resume	 = serial_ambarella_console_resume,
};
#endif

static int __init serial_ambarella_console_init(void)
{
	struct device_node *np;
	const char *name;
	int id;

	ambarella_uart_of_enumerate();

	name = of_get_property(of_chosen, "linux,stdout-path", NULL);
	if (name == NULL)
		return -ENODEV;

	np = of_find_node_by_path(name);
	if (!np)
		return -ENODEV;

	id = of_alias_get_id(np, "serial");
	if (id < 0 || id >= serial_ambarella_reg.nr)
		return -ENODEV;

	ambarella_port[id].port.membase = of_iomap(np, 0);

	register_console(&serial_ambarella_console);

#ifdef CONFIG_PM
	register_syscore_ops(&ambarella_console_syscore_ops);
#endif

	return 0;
}

console_initcall(serial_ambarella_console_init);

#define AMBARELLA_CONSOLE	&serial_ambarella_console
#else
#define AMBARELLA_CONSOLE	NULL
#endif

/* ==========================================================================*/
static struct uart_driver serial_ambarella_reg = {
	.owner		= THIS_MODULE,
	.driver_name	= "ambarella-uart",
	.dev_name	= "ttyS",
	.major		= TTY_MAJOR,
	.minor		= 64,
	.nr		= 0,
	.cons		= AMBARELLA_CONSOLE,
};

static int serial_ambarella_probe(struct platform_device *pdev)
{
	struct ambarella_uart_port *amb_port;
	struct resource *mem;
	struct pinctrl *pinctrl;
	int irq, id, rval = 0;

	mem = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!mem) {
		dev_err(&pdev->dev, "no mem resource!\n");
		return -ENODEV;
	}

	irq = platform_get_irq(pdev, 0);
	if (irq < 0) {
		dev_err(&pdev->dev, "no irq resource!\n");
		return -ENODEV;
	}

	id = of_alias_get_id(pdev->dev.of_node, "serial");
	if (id < 0 || id >= serial_ambarella_reg.nr) {
		dev_err(&pdev->dev, "Invalid uart ID %d!\n", id);
		return -ENXIO;
	}

	pinctrl = devm_pinctrl_get_select_default(&pdev->dev);
	if (IS_ERR(pinctrl)) {
		dev_err(&pdev->dev, "Failed to request pinctrl\n");
		return PTR_ERR(pinctrl);
	}

	amb_port = &ambarella_port[id];

	amb_port->uart_pll = clk_get(NULL, "gclk_uart");
	if (IS_ERR(amb_port->uart_pll)) {
		dev_err(&pdev->dev, "Get uart clk failed!\n");
		return PTR_ERR(amb_port->uart_pll);
	}

	amb_port->mcr = DEFAULT_AMBARELLA_UART_MCR;
	/* check if using modem status register,  available for non-uart0. */
	if (of_find_property(pdev->dev.of_node, "amb,msr-used", NULL))
		amb_port->msr_used = 1;
	else
		amb_port->msr_used = 0;
	/* check if workaround for tx fifo is needed */
	if (of_find_property(pdev->dev.of_node, "amb,tx-fifo-fix", NULL))
		amb_port->tx_fifo_fix = 1;
	else
		amb_port->tx_fifo_fix = 0;
	/* check if registers like FIFO status register are NOT provided. */
	if (of_find_property(pdev->dev.of_node, "amb,less-reg", NULL))
		amb_port->less_reg = 1;
	else
		amb_port->less_reg = 0;
	/* check if using tx dma */
	if (of_find_property(pdev->dev.of_node, "amb,txdma-used", NULL)) {
		amb_port->txdma_used = 1;
		dev_info(&pdev->dev,"Serial[%d] use txdma\n", id);
	}
	else
		amb_port->txdma_used = 0;
	/* check if using rx dma */
	if (of_find_property(pdev->dev.of_node, "amb,rxdma-used", NULL)) {
		amb_port->rxdma_used = 1;
		dev_info(&pdev->dev,"Serial[%d] use rxdma\n", id);
	} else {
		amb_port->rxdma_used = 0;
	}

	amb_port->port.dev = &pdev->dev;
	amb_port->port.type = PORT_UART00;
	amb_port->port.iotype = UPIO_MEM;
	amb_port->port.fifosize = UART_FIFO_SIZE;
	amb_port->port.uartclk = clk_get_rate(amb_port->uart_pll);
	amb_port->port.ops = &serial_ambarella_pops;
	amb_port->port.private_data = amb_port;
	amb_port->port.irq = irq;
	amb_port->port.line = id;
	amb_port->port.mapbase = mem->start;
	amb_port->port.membase =
		devm_ioremap(&pdev->dev, mem->start, resource_size(mem));
	if (!amb_port->port.membase) {
		dev_err(&pdev->dev, "can't ioremap UART\n");
		return -ENOMEM;
	}

	rval = uart_add_one_port(&serial_ambarella_reg, &amb_port->port);
	if (rval < 0) {
		dev_err(&pdev->dev, "failed to add port: %d, %d!\n", id, rval);
	}

	platform_set_drvdata(pdev, amb_port);

	return rval;
}

static int serial_ambarella_remove(struct platform_device *pdev)
{
	struct ambarella_uart_port *amb_port;
	int rval = 0;

	amb_port = platform_get_drvdata(pdev);
	rval = uart_remove_one_port(&serial_ambarella_reg, &amb_port->port);
	if (rval < 0)
		dev_err(&pdev->dev, "uart_remove_one_port fail %d!\n",	rval);

	dev_notice(&pdev->dev,
		"Remove Ambarella Media Processor UART.\n");

	return rval;
}

#ifdef CONFIG_PM
static int serial_ambarella_suspend(struct platform_device *pdev,
	pm_message_t state)
{
	struct ambarella_uart_port *amb_port;
	int rval = 0;

	amb_port = platform_get_drvdata(pdev);
	rval = uart_suspend_port(&serial_ambarella_reg, &amb_port->port);

	dev_dbg(&pdev->dev, "%s exit with %d @ %d\n",
		__func__, rval, state.event);

	return rval;
}

static int serial_ambarella_resume(struct platform_device *pdev)
{
	struct ambarella_uart_port *amb_port;
	int rval = 0;

	amb_port = platform_get_drvdata(pdev);

	clear_bit(AMBARELLA_UART_RESET_FLAG, &amb_port->flags);
	serial_ambarella_hw_setup(&amb_port->port);

	rval = uart_resume_port(&serial_ambarella_reg, &amb_port->port);

	dev_dbg(&pdev->dev, "%s exit with %d\n", __func__, rval);

	return rval;
}
#endif

static const struct of_device_id ambarella_serial_of_match[] = {
	{ .compatible = "ambarella,uart" },
	{},
};
MODULE_DEVICE_TABLE(of, ambarella_serial_of_match);

static void ambarella_uart_of_enumerate(void)
{
	struct device_node *np;
	int nr = 0;

	if (serial_ambarella_reg.nr <= 0) {
		for_each_matching_node(np, ambarella_serial_of_match) {
			nr++;
		}

		if (nr > UART_INSTANCES)
			nr = UART_INSTANCES;

		serial_ambarella_reg.nr = nr;
	}
}


static struct platform_driver serial_ambarella_driver = {
	.probe		= serial_ambarella_probe,
	.remove		= serial_ambarella_remove,
#ifdef CONFIG_PM
	.suspend	= serial_ambarella_suspend,
	.resume		= serial_ambarella_resume,
#endif
	.driver		= {
		.name	= "ambarella-uart",
		.of_match_table = ambarella_serial_of_match,
	},
};

int __init serial_ambarella_init(void)
{
	int rval;

	ambarella_uart_of_enumerate();

	rval = uart_register_driver(&serial_ambarella_reg);
	if (rval < 0)
		return rval;

	rval = platform_driver_register(&serial_ambarella_driver);
	if (rval < 0)
		uart_unregister_driver(&serial_ambarella_reg);

	return rval;
}

void __exit serial_ambarella_exit(void)
{
	platform_driver_unregister(&serial_ambarella_driver);
	uart_unregister_driver(&serial_ambarella_reg);
}

module_init(serial_ambarella_init);
module_exit(serial_ambarella_exit);

MODULE_AUTHOR("Anthony Ginger");
MODULE_DESCRIPTION("Ambarella Media Processor UART driver");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:ambarella-uart");
