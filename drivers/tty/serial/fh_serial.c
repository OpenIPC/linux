/*
 *  linux/drivers/tty/serial/fh_of_serial.c
 *
 *  Copyright (C) 2017 Fullhan Ltd.
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#if defined(CONFIG_MAGIC_SYSRQ)
#define SUPPORT_SYSRQ
#endif
#include <linux/module.h>
#include <linux/tty.h>
#include <linux/tty_flip.h>
#include <linux/ioport.h>
#include <linux/init.h>
#include <linux/serial.h>
#include <linux/console.h>
#include <linux/sysrq.h>
#include <linux/kthread.h>
#include <linux/device.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/of_platform.h>
#include <linux/of_irq.h>
#include <linux/of_address.h>
#include <linux/platform_device.h>
#include <linux/io.h>
#include <linux/slab.h>
#include <linux/mm.h>
#include <linux/serial_core.h>
#include <linux/serial_reg.h>
#include <asm/irq.h>
#include <linux/clk.h>
#include <mach/pmu.h>

#include <linux/completion.h>
#include <linux/dmaengine.h>
#include <linux/dma-mapping.h>
#include <linux/scatterlist.h>
#include <linux/fh_dmac.h>

#include <mach/fh_uart_plat.h>

#include "fh_serial.h"

#define REG_UART_RBR		(0x0000)
#define REG_UART_THR		(0x0000)
#define REG_UART_DLL		(0x0000)
#define REG_UART_DLH		(0x0004)
#define REG_UART_IER		(0x0004)
#define REG_UART_IIR		(0x0008)
#define REG_UART_FCR		(0x0008)
#define REG_UART_LCR		(0x000c)
#define REG_UART_MCR		(0x0010)
#define REG_UART_LSR		(0x0014)
#define REG_UART_MSR		(0x0018)
#define REG_UART_SCR		(0x001c)
#define REG_UART_FAR		(0x0070)
#define REG_UART_TFR		(0x0074)
#define REG_UART_RFW		(0x0078)
#define REG_UART_USR		(0x007c)
#define REG_UART_TFL		(0x0080)
#define REG_UART_RFL		(0x0084)
#define REG_UART_SRR		(0x0088)
#define REG_UART_SFE		(0x0098)
#define REG_UART_SRT		(0x009c)
#define REG_UART_STET		(0x00a0)
#define REG_UART_HTX		(0x00a4)
#define REG_UART_DMASA		(0x00a8)
#define REG_UART_DLF		(0x00C0)
#define REG_UART_CPR		(0x00f4)
#define REG_UART_UCV		(0x00f8)
#define REG_UART_CTR		(0x00fc)

#define FH_SERIAL_NAME		"fh-serial"
#define FH_DRIVE_NAME		"ttyS" /* for tty */
#define FH_DEV_NAME			"ttyS" /*for earlyprintk*/

#define UART_READ_RX_DW_FIFO_OK				0
#define UART_READ_RX_DW_FIFO_TIME_OUT		0xcc
#define MAP_SIZE							0x100

static struct console fh_serial_console;
#define FH_SERIAL_CONSOLE		(&fh_serial_console)

#define tx_enabled(port)		((port)->unused[0])
#define rx_enabled(port)		((port)->unused[1])

#define UART_FIFO_BOTH_EMPTY	(UART_LSR_TEMT | UART_LSR_THRE)

#define UART_BAUD	BAUDRATE_115200
#define UART_FIFO	UART_INT_RXFIFO_DEPTH_QUARTER
#define UART_LINE	Uart_line_8n2
#define UART_DEFAULT_BAUD	115200
/* denominator = (2 ^ DLF_SIZE) when FRACTIONAL_BAUD_DIVISOR Enabled */
#define DLF_SIZE	4

#define FH_UART_NUMBER 3

#define fh_uart_readl(port, offset) \
	readl(port->membase + offset)
#define fh_uart_writel(port, offset, val) \
	writel((val), port->membase + offset)

static void fh_serial_stop_tx(struct uart_port *port);
static void fh_serial_start_tx(struct uart_port *port);
static void fh_serial_stop_rx(struct uart_port *port);
static void fh_serial_start_rx(struct uart_port *port);
static unsigned int fh_serial_tx_empty(struct uart_port *port);
static unsigned int fh_serial_get_mctrl(struct uart_port *port);
static void fh_serial_set_mctrl(struct uart_port *port, unsigned int mctrl);
static void fh_serial_break_ctl(struct uart_port *port, int break_state);
static irqreturn_t fh_uart_rx_chars(int irq, void *dev_id);
static irqreturn_t fh_uart_tx_chars(int irq, void *dev_id);
static irqreturn_t fh_uart_isr(int irq, void *dev_id);
static void fh_serial_shutdown(struct uart_port *port);
static int fh_serial_startup(struct uart_port *port);
static void fh_serial_set_termios(struct uart_port *port,
				  struct ktermios *termios,
				  struct ktermios *old);
static const char *fh_serial_type(struct uart_port *port);
static void fh_serial_release_port(struct uart_port *port);
static int fh_serial_request_port(struct uart_port *port);
static void fh_serial_config_port(struct uart_port *port, int flags);
static int fh_serial_verify_port(struct uart_port *port,
				   struct serial_struct *ser);
#ifdef CONFIG_USE_OF
static int fh_serial_of_configure(void);
#endif
static int fh_serial_configure(struct uart_port *port, struct platform_device *op);

static bool fh_uart_dma_chan_filter(struct dma_chan *chan, void *param);
static void fh_serial_dma_rx_callback(void *arg);
static void fh_serial_dma_rx_char(void *arg);
static int uart_dma_set_rx_para(struct uart_port *port, int xfer_len,
	void (*call_back)(void *arg));
#ifdef CONFIG_UART_TX_DMA
static void fh_serial_dma_tx_char_done(void *arg);
static int uart_dma_set_tx_para(struct uart_port *port, int xfer_len,
	void (*call_back)(void *arg));
static void fh_serial_dma_tx_char(struct uart_port *port);
#endif

static struct uart_ops fh_serial_ops = {
	.tx_empty		= fh_serial_tx_empty,
	.get_mctrl		= fh_serial_get_mctrl,
	.set_mctrl		= fh_serial_set_mctrl,
	.stop_tx		= fh_serial_stop_tx,
	.start_tx		= fh_serial_start_tx,
	.stop_rx		= fh_serial_stop_rx,
	.break_ctl		= fh_serial_break_ctl,
	.startup		= fh_serial_startup,
	.shutdown		= fh_serial_shutdown,
	.set_termios	= fh_serial_set_termios,
	.type			= fh_serial_type,
	.release_port	= fh_serial_release_port,
	.request_port	= fh_serial_request_port,
	.config_port	= fh_serial_config_port,
	.verify_port	= fh_serial_verify_port,
};

static struct uart_driver fh_serial_driver = {
	.owner			= THIS_MODULE,
	.driver_name	= FH_DRIVE_NAME,
	.nr				= FH_UART_NUMBER,
	.cons			= FH_SERIAL_CONSOLE,
	.dev_name		= FH_DEV_NAME,
	.major			= TTY_MAJOR,
	.minor			= 64,
};

#ifdef CONFIG_USE_OF
struct uart_port fh_serial_ports[FH_UART_NUMBER];
#else
extern struct uart_port fh_serial_ports[FH_UART_NUMBER];
#endif

static struct fh_uart_dma *uart_dma[FH_UART_NUMBER];

static inline int Uart_Disable_Irq(struct uart_port *port, uart_irq_e interrupts)
{
	unsigned int ret;

	ret = fh_uart_readl(port, REG_UART_IER);
	ret &= ~interrupts;
	fh_uart_writel(port, REG_UART_IER, ret);

	return 0;
}

static inline int Uart_Enable_Irq(struct uart_port *port, uart_irq_e interrupts)
{
	unsigned int ret;

	ret = fh_uart_readl(port, REG_UART_IER);
	ret |= interrupts;
	fh_uart_writel(port, REG_UART_IER, ret);

	return 0;
}

static inline int Uart_Read_Control_Status(struct uart_port *port)
{
	return fh_uart_readl(port, REG_UART_USR);
}

static inline int Uart_Check_Busy(struct uart_port *port)
{
	int ret;

	ret = Uart_Read_Control_Status(port);
	if (ret & UART_STATUS_BUSY) {
		printk(KERN_ERR "%s: uart is busy, ret: %d\n",
				__func__, ret);
		return UART_IS_BUSY;
	}

	return 0;
}

static inline int Uart_Fifo_Config(struct uart_port *port,
		unsigned int fifo_ctrl)
{
	unsigned int ret;

	fh_uart_writel(port, REG_UART_FCR, fifo_ctrl);
	ret = fh_uart_readl(port, REG_UART_IIR);

	if (ret & UART_FIFO_IS_ENABLE)
		return UART_CONFIG_FIFO_OK;
	else
		return UART_CONFIG_FIFO_ERROR;
}

static inline int Uart_Set_Line_Control(struct uart_port *port,
		unsigned int line_ctrl)
{
	unsigned int ret;

	ret = Uart_Check_Busy(port);
	if (ret)
		return UART_IS_BUSY;

	fh_uart_writel(port, REG_UART_LCR, line_ctrl);
	return UART_CONFIG_LINE_OK;
}

static inline int Uart_Read_Line_Status(struct uart_port *port)
{
	return fh_uart_readl(port, REG_UART_LSR);
}

void Uart_Flush_Fifo(struct uart_port *port)
{
	fh_uart_writel(port, REG_UART_FCR,
			UART_FCR_CLEAR_RCVR | UART_FCR_CLEAR_XMIT);
}

static inline int Uart_Set_Clock_Divisor(struct uart_port *port, unsigned int baud)
{
	u32 low, high;
	u32 ret;

	ret = 0;
	low = baud & 0x00ff;
	high = (baud & 0xff00) >> 8;

	ret = Uart_Check_Busy(port);
	if (ret)
		return UART_IS_BUSY;

	ret = fh_uart_readl(port, REG_UART_LCR);
	// if DLAB not set
	if (!(ret & UART_LCR_DLAB_POS)) {
		ret |= UART_LCR_DLAB_POS;
		fh_uart_writel(port, REG_UART_LCR, ret);
	}
	/* fh_uart_writel(port, REG_UART_DLF, fraction_divide); */
	fh_uart_writel(port, REG_UART_DLL, low);
	fh_uart_writel(port, REG_UART_DLH, high);

	/* clear DLAB */
	ret = ret & 0x7f;
	fh_uart_writel(port, REG_UART_LCR, ret);

	return 0;
}

static inline int Uart_Read_iir(struct uart_port *port)
{
	return fh_uart_readl(port, REG_UART_IIR);
}

static bool fh_uart_dma_chan_filter(struct dma_chan *chan, void *param)
{
	int dma_channel = *(int *) param;
	bool ret = false;

	if (chan->chan_id == dma_channel)
		ret = true;
	return ret;
}

#ifdef CONFIG_UART_TX_DMA
static void fh_serial_dma_tx_char_done(void *arg)
{
	struct uart_port *uart_port  = (struct uart_port *)arg;
	struct fh_uart_dma *dma_info = uart_dma[uart_port->line];

	uart_port->icount.tx += dma_info->tx_count;
	dma_info->tx_done = 1;
}
#endif

static void fh_serial_dma_rx_char(void *arg)
{
	struct uart_port *port = (struct uart_port *)arg;
	struct fh_uart_dma *dma_info = uart_dma[port->line];
	unsigned int uerstat;
	unsigned int flag;
	unsigned int xmit_len;
	unsigned int i;

	xmit_len = dma_info->rx_xmit_len;
	port->icount.rx += xmit_len;
	uerstat = Uart_Read_Line_Status(port);
	flag = TTY_NORMAL;

	if (unlikely(uerstat & UART_LINE_STATUS_RFE)) {
		dev_dbg(port->dev, "rxerr: port rxs=0x%08x\n", uerstat);
		if (uerstat & UART_LINE_STATUS_BI) {
			port->icount.brk++;
			if (uart_handle_break(port))
				goto dma_ignore_char;
		}
	}
	if (uerstat & UART_LINE_STATUS_PE)
		port->icount.parity++;
	if (uerstat & UART_LINE_STATUS_BI)
		port->icount.frame++;
	if (uerstat & UART_LINE_STATUS_OE)
		port->icount.overrun++;
	uerstat &= port->read_status_mask;
	if (uerstat & UART_LINE_STATUS_BI)
		flag = TTY_BREAK;
	else if (uerstat & UART_LINE_STATUS_PE)
		flag = TTY_PARITY;
	else if (uerstat & (UART_LINE_STATUS_FE |
			UART_LINE_STATUS_OE))
		flag = TTY_FRAME;
	for (i = 0;
		i < xmit_len; i++) {
		if (!uart_handle_sysrq_char(port, dma_info->rx_dumy_buff[i])) {
			uart_insert_char(port, uerstat, UART_LINE_STATUS_OE,\
				dma_info->rx_dumy_buff[i], flag);
		}
	}

dma_ignore_char:
	tty_flip_buffer_push(&port->state->port);
}

static int uart_dma_set_rx_para(struct uart_port *port,
					int xfer_len,
					void (*call_back)(void *arg))
{
	struct fh_dma_extra ext_para;
	struct dma_slave_config *rx_config;
	struct dma_chan *rxchan;
	struct scatterlist *p_sca_list;
	unsigned int sg_size = 1;
	struct dma_async_tx_descriptor *p_desc;
	struct fh_uart_dma *dma_info = uart_dma[port->line];

	p_desc = 0;
	memset(&dma_info->dma_rx.cfg,
			0,
			sizeof(struct dma_slave_config));
	memset(&ext_para, 0, sizeof(struct fh_dma_extra));
	p_sca_list = &dma_info->dma_rx.sgl[0];
	rxchan = dma_info->dma_rx.chan;
	rx_config = &dma_info->dma_rx.cfg;
	rx_config->src_addr = dma_info->paddr + REG_UART_RBR;
	rx_config->dst_addr_width = DMA_SLAVE_BUSWIDTH_1_BYTE;
	rx_config->src_addr_width = DMA_SLAVE_BUSWIDTH_1_BYTE;
	rx_config->slave_id = dma_info->rx_hs_no;
	rx_config->src_maxburst = 16;
	rx_config->dst_maxburst = 16;
	rx_config->direction = DMA_DEV_TO_MEM;
	rx_config->device_fc = 0;
	ext_para.sinc = FH_DMA_SLAVE_FIX;
	p_sca_list->dma_address = dma_info->rx_dma_add;
	p_sca_list->length = min(xfer_len, 128);
	ext_para.dinc = FH_DMA_SLAVE_INC;
	dmaengine_slave_config(rxchan, rx_config);
	p_desc =
		rxchan->device->device_prep_slave_sg(rxchan,
		&dma_info->dma_rx.sgl[0], sg_size, DMA_DEV_TO_MEM,
		DMA_PREP_INTERRUPT,
		&ext_para);

	dma_info->dma_rx.actual_sgl_size = sg_size;
	p_desc->callback = fh_serial_dma_rx_callback;
	p_desc->callback_param = port;
	dma_info->dma_rx.desc = p_desc;

	return 0;
}

void uart_unmap_dma_rx_sg(struct uart_port *port)
{
	struct dma_chan *rxchan;
	struct scatterlist *p_sca_list;
	struct fh_uart_dma *dma_info = uart_dma[port->line];

	p_sca_list = &dma_info->dma_rx.sgl[0];
	rxchan = dma_info->dma_rx.chan;

	dma_unmap_single(rxchan->dev->device.parent,
		p_sca_list->dma_address,
		1,
		DMA_FROM_DEVICE);
}


static void fh_serial_dma_rx_callback(void *arg)
{
	struct uart_port *port = (struct uart_port *)arg;
	struct fh_uart_dma *dma_info = uart_dma[port->line];

	uart_unmap_dma_rx_sg(port);
	fh_serial_dma_rx_char(port);

	complete(&dma_info->rx_cmp);
}

#ifdef CONFIG_UART_TX_DMA
static int uart_dma_set_tx_para(struct uart_port *port,
						int xfer_len,
						void (*call_back)(void *arg))
{
	struct fh_dma_extra ext_para;
	struct dma_slave_config *tx_config;
	struct dma_chan *txchan;
	struct scatterlist *p_sca_list;
	unsigned int sg_size = 1;
	struct fh_uart_dma *dma_info = uart_dma[port->line];

	memset(&dma_info->dma_tx.cfg,
			0,
			sizeof(struct dma_slave_config));
	memset(&ext_para, 0, sizeof(struct fh_dma_extra));
	p_sca_list = &dma_info->dma_tx.sgl[0];
	txchan = dma_info->dma_tx.chan;
	tx_config = &dma_info->dma_tx.cfg;
	tx_config->dst_addr = dma_info->paddr + REG_UART_THR;
	tx_config->dst_addr_width = DMA_SLAVE_BUSWIDTH_1_BYTE;
	tx_config->src_addr_width = DMA_SLAVE_BUSWIDTH_1_BYTE;
	tx_config->slave_id = dma_info->tx_hs_no;
	tx_config->src_maxburst = 1;
	tx_config->dst_maxburst = 1;
	tx_config->direction = DMA_MEM_TO_DEV;
	tx_config->device_fc = 0;
	ext_para.sinc = FH_DMA_SLAVE_FIX;
	p_sca_list->dma_address = dma_map_single(
			dma_info->dma_rx.chan->dev->device.parent,
			(void *)dma_info->rx_dumy_buff,
			sizeof(dma_info->rx_dumy_buff),
			DMA_TO_DEVICE);
	p_sca_list->length = min(xfer_len, 128);
	ext_para.dinc = FH_DMA_SLAVE_INC;
	dmaengine_slave_config(txchan, tx_config);
	dma_info->dma_tx.desc =
		txchan->device->device_prep_slave_sg(txchan,
		&dma_info->dma_tx.sgl[0], sg_size, DMA_MEM_TO_DEV,
		DMA_PREP_INTERRUPT | DMA_CTRL_ACK,
		&ext_para);
	dma_info->dma_tx.actual_sgl_size = sg_size;
	dma_info->dma_tx.desc->callback = fh_serial_dma_tx_char_done;
	dma_info->dma_tx.desc->callback_param = port;

	return 0;
}
#endif

#ifdef CONFIG_UART_TX_DMA
static void fh_serial_dma_tx_char(struct uart_port *port)
{
	unsigned int xfer_len;
	struct fh_uart_dma *dma_info = uart_dma[port->line];
	struct circ_buf *xmit = &port->state->xmit;

	if (uart_circ_empty(xmit) || uart_tx_stopped(port)) {
		dma_info->tx_done = 1;
		dma_info->tx_done = 0;
		return;
	}

	if (dma_info->tx_done) {
		xfer_len =
			uart_circ_chars_pending(xmit); /* Check xfer length */
		if (xfer_len > 128)
			xfer_len = 128;
		if (xmit->tail < xmit->head) {
			memcpy(dma_info->tx_dumy_buff,
					&xmit->buf[xmit->tail],
					xfer_len);
		} else {
			int first = UART_XMIT_SIZE - xmit->tail;
			int second = xmit->head;

			memcpy(dma_info->tx_dumy_buff,
				 &xmit->buf[xmit->tail],
				 first);
			if (second)
				memcpy(&dma_info->tx_dumy_buff[first],
					 &xmit->buf[0],
					 second);
		}
		xmit->tail = (xmit->tail + xfer_len) & (UART_XMIT_SIZE - 1);
		dma_info->tx_count = xfer_len;
		uart_dma_set_tx_para(
						port,
						xfer_len,
						fh_serial_dma_tx_char_done);
		dma_info->dma_tx.desc->tx_submit(dma_info->dma_tx.desc);
		dma_info->tx_done = 0;
	}
}
#endif

int uart_rx_thread(void *data)
{
	struct uart_port *port = (struct uart_port *)data;
	struct fh_uart_dma *dma_info = uart_dma[port->line];

	while (1) {
		uart_dma_set_rx_para(port,
			dma_info->rx_xmit_len,
			fh_serial_dma_rx_callback);
		dma_info->dma_rx.desc->tx_submit(dma_info->dma_rx.desc);

		wait_for_completion(&dma_info->rx_cmp);
	}
}

int fh_uart_rx_dma_start(struct uart_port *port)
{
	struct task_struct *uart_rx_kthread = NULL;
	int err = 0;
	char thread_name[16] = {0};

	sprintf(thread_name, "uart%d_rx", port->line);

	uart_rx_kthread = kthread_create(uart_rx_thread,
					port,
					thread_name);

	if (IS_ERR(uart_rx_kthread)) {
		pr_err("Create UART DMA RX Thread Failed!\n");
		err = PTR_ERR(uart_rx_kthread);
		uart_rx_kthread = NULL;

		return err;
	}

	wake_up_process(uart_rx_kthread);

	return err;
}


static inline int Uart_Init(struct uart_port *port)
{
	int ret = 0;
	unsigned int quot;

	Uart_Flush_Fifo(port);
	quot = ((port->uartclk / UART_DEFAULT_BAUD) + 8) / 16;
	ret |= Uart_Set_Clock_Divisor(port, quot);
	ret |= Uart_Set_Line_Control(port, UART_LINE);
	if (uart_dma[port->line])
		ret |= Uart_Fifo_Config(port, UART_INT_RXFIFO_DEPTH_QUARTER_DMA);
	else
		ret |= Uart_Fifo_Config(port, UART_FIFO);
	Uart_Disable_Irq(port, UART_INT_ALL);
	if (uart_dma[port->line] && !uart_dma[port->line]->inited) {
		uart_dma[port->line]->inited = 1;
		ret |= fh_uart_rx_dma_start(port);
	} else
		fh_serial_start_rx(port);


	return ret;
}

static irqreturn_t
fh_uart_rx_chars(int irq, void *dev_id)
{
	struct uart_port *port = dev_id;
	unsigned int ch = 0;
	unsigned int flag;
	unsigned int uerstat;
	int max_count = 64;
	int ret_status;

	while (max_count-- > 0) {
		//check if rx fifo is empty
		ret_status = Uart_Read_Control_Status(port);
		//0 means empty.	 1 means not empty
		if (!(ret_status & UART_STATUS_RFNE))
			break;
		//read error in the rx process
		uerstat = Uart_Read_Line_Status(port);
		//read  data in the rxfifo
		if (uerstat & UART_LINE_STATUS_DR)
			ch = fh_uart_readl(port, REG_UART_RBR);
		/* insert the character into the buffer */
		flag = TTY_NORMAL;
		port->icount.rx++;
		//if at least one error in rx process
		if (unlikely(uerstat & UART_LINE_STATUS_RFE)) {
			dev_dbg(port->dev, "rxerr: port ch=0x%02x, rxs=0x%08x\n",
				ch, uerstat);
			/* check for break */
			if (uerstat & UART_LINE_STATUS_BI) {
				//dbg("break!\n");
				port->icount.brk++;
				if (uart_handle_break(port))
					goto ignore_char;
			}

			if (uerstat & UART_LINE_STATUS_BI)
				port->icount.frame++;
			if (uerstat & UART_LINE_STATUS_OE)
				port->icount.overrun++;

			//check the mask
			uerstat &= port->read_status_mask;

			if (uerstat & UART_LINE_STATUS_BI)
				flag = TTY_BREAK;
			else if (uerstat & UART_LINE_STATUS_PE)
				flag = TTY_PARITY;
			else if (uerstat & (UART_LINE_STATUS_FE |
					    UART_LINE_STATUS_OE))
				flag = TTY_FRAME;
		}

		if (uart_handle_sysrq_char(port, ch))
			goto ignore_char;

		uart_insert_char(port, uerstat, UART_LINE_STATUS_OE,
				ch, flag);

ignore_char:
		continue;
	}
	tty_flip_buffer_push(&port->state->port);
	return IRQ_HANDLED;
}



static irqreturn_t
fh_uart_tx_chars(int irq, void *dev_id)
{
	struct uart_port *port = dev_id;
	struct circ_buf *xmit = &port->state->xmit;
	int count = 256;
	int ret_status;

	/* if there isn't anything more to transmit, or the uart is now
	 * stopped, disable the uart and exit
	 */
	if (uart_circ_empty(xmit) || uart_tx_stopped(port)) {
		fh_serial_stop_tx(port);
		goto out;
	}
	/* try and drain the buffer... */
	while (!uart_circ_empty(xmit) && count-- > 0) {

		//check the tx fifo full?    full then break
		ret_status = Uart_Read_Control_Status(port);
		if (!(ret_status & UART_STATUS_TFNF)) //0 means full.	 1 means not full
			break;
		//write data to the hw fifo
		fh_uart_writel(port, REG_UART_THR, xmit->buf[xmit->tail]);
		xmit->tail = (xmit->tail + 1) & (UART_XMIT_SIZE - 1);
		port->icount.tx++;
	}

	if (uart_circ_chars_pending(xmit) < WAKEUP_CHARS)
		uart_write_wakeup(port);

	if (uart_circ_empty(xmit))
		fh_serial_stop_tx(port);
out:
	return IRQ_HANDLED;
}

static irqreturn_t
fh_uart_isr(int irq, void *dev_id)
{
	irqreturn_t ret_isr;
	struct uart_port *port = dev_id;
	int ret_iir;
	int control_status;
	int line_status;

	ret_iir = Uart_Read_iir(port);

	ret_iir = ret_iir & 0xf;

	if (ret_iir == 0xc) {
		control_status = Uart_Read_Control_Status(port);

		if (!(control_status & UART_STATUS_RFNE)) {
			line_status = Uart_Read_Line_Status(port);

			if (!(line_status & UART_LINE_STATUS_DR)) {
				fh_uart_readl(port, REG_UART_RBR);
				return IRQ_HANDLED;
			}
		}
	}

	if ((ret_iir & 0x04))
		ret_isr = fh_uart_rx_chars(irq, dev_id);

	if (ret_iir & 0x02)
		ret_isr = fh_uart_tx_chars(irq, dev_id);
	else
		ret_isr = IRQ_HANDLED;

	return ret_isr;
}

static void fh_serial_stop_tx(struct uart_port *port)
{
	if (uart_dma[port->line]) {
		struct circ_buf *xmit = &port->state->xmit;
		struct fh_uart_dma *dma_info = uart_dma[port->line];

		xmit->tail =
		(xmit->tail + dma_info->tx_count) & (UART_XMIT_SIZE - 1);
		port->icount.tx += dma_info->tx_count;
		dma_info->tx_count = 0;
		dma_info->tx_done = 1;
	}

	Uart_Disable_Irq(port, UART_INT_ETBEI_POS);
}

static void fh_serial_start_tx(struct uart_port *port)
{
#ifdef CONFIG_UART_TX_DMA
	if (uart_dma[port->line]) {
		struct fh_uart_dma *dma_info = uart_dma[port->line];

		if (dma_info->tx_done)
			fh_serial_dma_tx_char(port);
	} else
#endif
		Uart_Enable_Irq(port, UART_INT_ETBEI_POS);
}

static void fh_serial_stop_rx(struct uart_port *port)
{
	Uart_Disable_Irq(port, UART_INT_ERBFI_POS);
}

static void fh_serial_start_rx(struct uart_port *port)
{
	Uart_Enable_Irq(port, UART_INT_ERBFI_POS);
}

static unsigned int fh_serial_tx_empty(struct uart_port *port)
{
	/*//1 means empty   0:means no empty*/
	int ret;
	int ret_status;

	ret_status = Uart_Read_Control_Status(port);
	if (ret_status & UART_STATUS_TFE)
		ret = 1;
	else
		ret = 0;
	return ret;
}

static unsigned int fh_serial_get_mctrl(struct uart_port *port)
{
	/* The FH UART handles flow control in hardware */
	return TIOCM_CAR | TIOCM_DSR | TIOCM_CTS;
}


static void fh_serial_set_mctrl(struct uart_port *port, unsigned int mctrl)
{
	/* The FH UART handles flow control in hardware */
}

static void fh_serial_break_ctl(struct uart_port *port, int break_state)
{
	/* We don't support sending break */
}

static void fh_serial_shutdown(struct uart_port *port)
{
	Uart_Disable_Irq(port, UART_INT_ALL);
	Uart_Flush_Fifo(port);
	free_irq(port->irq, port);
}

static int fh_serial_startup(struct uart_port *port)
{
	int ret;
	int status;
	char *isr_name;

	do {
		status = Uart_Read_Line_Status(port);
	} while ((status & UART_FIFO_BOTH_EMPTY) != UART_FIFO_BOTH_EMPTY);

	Uart_Init(port);

	isr_name = kzalloc(16, GFP_KERNEL);
	sprintf(isr_name, "ttyS-%d", port->line);

	ret = request_irq(port->irq, fh_uart_isr, 0,
		isr_name, port);
	if (ret) {
		printk(KERN_ERR "request serial irq %d failed.\n", port->irq);
		return ret;
	}

	enable_irq_wake(port->irq);

	return 0;
}

static void fh_serial_set_termios(struct uart_port *port,
				  struct ktermios *termios,
				  struct ktermios *old)
{
	unsigned long flags;
	unsigned int baud, quot;
	unsigned int line_data = 0, status;

	do {
		status = Uart_Read_Line_Status(port);

	} while ((status & UART_FIFO_BOTH_EMPTY) != UART_FIFO_BOTH_EMPTY);
	//data bit
	switch (termios->c_cflag & CSIZE) {
	case CS5:
		line_data |= UART_LCR_WLEN5;			//5bit
		break;
	case CS6:
		line_data |= UART_LCR_WLEN6;			//6bit
		break;
	case CS7:
		line_data |= UART_LCR_WLEN7;			//7bit
		break;
	case CS8:
		line_data |= UART_LCR_WLEN8;			//8bit
		break;
	default:
		line_data |= UART_LCR_WLEN8;
		break;
	}
	/* stop bits */
	if (termios->c_cflag & CSTOPB)
		line_data |= UART_LCR_STOP;			//0:1bit 1:2bit
	//parity enable?
	if (termios->c_cflag & PARENB) {
		line_data |= UART_LCR_PARITY;
		//even or odd parity
		if (!(termios->c_cflag & PARODD))
			line_data |= UART_LCR_EPAR;
	}
	//baud cal.   baud is the uart will be out.		the quot is the div
	baud = uart_get_baud_rate(port, termios, old, 0, port->uartclk / 16);
	quot = uart_get_divisor(port, baud);

	do {
		status = Uart_Read_Line_Status(port);
	} while ((status & UART_FIFO_BOTH_EMPTY) != UART_FIFO_BOTH_EMPTY);

	uart_update_timeout(port, termios->c_cflag, baud);
	spin_lock_irqsave(&port->lock, flags);

	port->read_status_mask = UART_LSR_OE | UART_LSR_THRE | UART_LSR_DR;
	if (termios->c_iflag & INPCK)
		port->read_status_mask |= UART_LSR_FE | UART_LSR_PE;
	if (termios->c_iflag & (BRKINT | PARMRK))
		port->read_status_mask |= UART_LSR_BI;

	/*
	 * Characters to ignore
	 */
	port->ignore_status_mask = 0;
	if (termios->c_iflag & IGNPAR)
		port->ignore_status_mask |= UART_LSR_PE | UART_LSR_FE;
	if (termios->c_iflag & IGNBRK) {
		port->ignore_status_mask |= UART_LSR_BI;
		/*
		 * If we're ignoring parity and break indicators,
		 * ignore overruns too (for real raw support).
		 */
		if (termios->c_iflag & IGNPAR)
			port->ignore_status_mask |= UART_LSR_OE;
	}

	/*
	 * ignore all characters if CREAD is not set
	 */
	if ((termios->c_cflag & CREAD) == 0)
		port->ignore_status_mask |= UART_LSR_DR;

	Uart_Flush_Fifo(port);
	Uart_Set_Clock_Divisor(port, quot);
	Uart_Set_Line_Control(port, line_data);
	Uart_Disable_Irq(port, UART_INT_ALL);

	if (!uart_dma[port->line]) {
		Uart_Fifo_Config(port, UART_FIFO);
		fh_serial_start_rx(port);
	} else
		Uart_Fifo_Config(port, UART_INT_RXFIFO_DEPTH_QUARTER_DMA);

	spin_unlock_irqrestore(&port->lock, flags);
}



static const char *fh_serial_type(struct uart_port *port)
{
	return  (port->type == PORT_FH) ? FH_DRIVE_NAME : NULL;
}

static void fh_serial_release_port(struct uart_port *port)
{
	release_mem_region(port->mapbase, MAP_SIZE);
}

static int fh_serial_request_port(struct uart_port *port)
{
	return request_mem_region(port->mapbase, MAP_SIZE, FH_SERIAL_NAME) ? 0 : -EBUSY;
}


static void fh_serial_config_port(struct uart_port *port, int flags)
{
	if (flags & UART_CONFIG_TYPE) {
		fh_serial_request_port(port);
		port->type = PORT_FH;
	}
}

static int fh_serial_verify_port(struct uart_port *port,
			       struct serial_struct *ser)
{
	int ret = 0;

	if (ser->type != PORT_UNKNOWN && ser->type != PORT_FH)
		ret = -EINVAL;
	if (ser->irq < 0 || ser->irq >= NR_IRQS)
		ret = -EINVAL;
	if (ser->baud_base < 2400)
		ret = -EINVAL;
	return ret;
}

/*********************************
 *
 *FH Memory Debug
 *
 ********************************/
#ifdef CONFIG_MAGIC_SYSRQ

static void SysrqShowCharacter(char *character)
{
	struct uart_port *port;

	port = &fh_serial_ports[0];

	fh_uart_writel(port, REG_UART_THR, *character);
}

static unsigned int SysrqParseHex(char *buff)
{
	unsigned int data;

	data = (unsigned int)simple_strtoul(buff, NULL, 16);
	return data;
}

static void SysrqGetCharacter(char *buff)
{
	struct uart_port *port;
	char ch;

	port = &fh_serial_ports[0];
	while (1) {
		if ((fh_uart_readl(port, REG_UART_LSR) & 0x01) == 0x01) {
			ch = fh_uart_readl(port, REG_UART_RBR) & 0xff;
			SysrqShowCharacter(&ch);
			if (ch == ' ' || ch == '\r' || ch == '\n')
				break;
			else
				*buff++ = ch;
		}
	}
}

static int SysrqGetDirection(void)
{
	char ch;

	SysrqGetCharacter(&ch);
	if (ch == 'w' || ch == 'W')
		return 1;
	else if (ch == 'r' || ch == 'R')
		return 0;
	else
		return -1;
}

static int SysrqGetMode(void)
{
	char ch;

	SysrqGetCharacter(&ch);
	if (ch == 'v' || ch == 'V')
		return 1;
	else if (ch == 'p' || ch == 'P')
		return 0;
	else
		return -1;
}

static void SysWritePhyMemory(int phyaddress, int value)
{
	int virtualaddress;

	pr_info("Physical Address Write:0x%08x->0x%08x", value, phyaddress);
	virtualaddress = __phys_to_virt(phyaddress);
	writel(value, (void __iomem *)virtualaddress);
}

static void SysWriteVirtualMemory(int virtualaddress, int value)
{
	pr_info("Virtual Address Write:0x%08x->0x%08x", value, virtualaddress);

	writel(value, (void __iomem *)virtualaddress);
}

static void SysWriteMemory(void)
{
	int mode;
	int address;
	int value;
	char internal_buffer[11] = {0};

	mode = SysrqGetMode();
	switch (mode) {
	case 0:
		SysrqGetCharacter(internal_buffer);
		address = SysrqParseHex(internal_buffer);
		memset(internal_buffer, 0, sizeof(internal_buffer));
		SysrqGetCharacter(internal_buffer);
		value = SysrqParseHex(internal_buffer);
		SysWritePhyMemory(address, value);
		break;
	case 1:
		SysrqGetCharacter(internal_buffer);
		address = SysrqParseHex(internal_buffer);
		memset(internal_buffer, 0, sizeof(internal_buffer));
		SysrqGetCharacter(internal_buffer);
		value = SysrqParseHex(internal_buffer);
		SysWriteVirtualMemory(address, value);
		break;
	default:
		pr_info("Select Wrong Mode\n");
		break;
	}
}

static void SysReadPhyMemory(int phyaddress, int length)
{
	int start;
	int virtualaddress;
	unsigned int ch;

	pr_info("Physical Address Read From:0x%08x(0x%x)\n", phyaddress, length);
	virtualaddress =  __phys_to_virt(phyaddress);
	for (start = 0; start < length; start++) {
		if (start % 8 == 0)
			pr_info("Address:%02x:\n", (virtualaddress + start * 4) & 0xff);
		ch = readl((void __iomem *)(virtualaddress + start * 4));
		pr_info("0x%08x  \n", ch);
	}
}

static void SysReadVirtualMemory(int virtualaddress, int length)
{
	int start;
	unsigned int ch;

	pr_info("Virtual Address Read From:0x%08x(0x%x)\n", virtualaddress, length);
	for (start = 0; start < length; start++) {
		if (start % 8 == 0)
			pr_info("Address:%08x:\n", (virtualaddress + start * 4));
		ch = readl((void __iomem *)(virtualaddress + start * 4));
		pr_info("0x%08x  \n", ch);
	}
}


static void SysReadMemory(void)
{
	int mode;
	int address;
	int length;
	char internal_buffer[11] = {0};

	mode = SysrqGetMode();
	switch (mode) {
	case 0:
		SysrqGetCharacter(internal_buffer);
		address = SysrqParseHex(internal_buffer);
		memset(internal_buffer, 0, sizeof(internal_buffer));
		SysrqGetCharacter(internal_buffer);
		length = SysrqParseHex(internal_buffer);
		SysReadPhyMemory(address, length);
		break;
	case 1:
		SysrqGetCharacter(internal_buffer);
		address = SysrqParseHex(internal_buffer);
		memset(internal_buffer, 0, sizeof(internal_buffer));
		SysrqGetCharacter(internal_buffer);
		length = SysrqParseHex(internal_buffer);
		SysReadVirtualMemory(address, length);
		break;
	default:
		pr_info("Select Wrong Mode\n");
		break;
	}
}

static void SysHandleFHMemory(int key)
{
	int direction;

	direction = SysrqGetDirection();
	switch (direction) {
	case 0:
		SysReadMemory();
		break;
	case 1:
		SysWriteMemory();
		break;
	default:
		pr_info("Select Wrong Direction\n");
		break;
	}
}

#endif

/*********************************
 *
 *
 * FH  CONSOLE
 *
 *
 *********************************/
static void
fh_serial_console_putchar(struct uart_port *port, int ch)
{
	unsigned int ret;

	do {
		ret = Uart_Read_Control_Status(port);
	}
	//wait txfifo is full
	//0 means full.	 1 means not full
	while (!(ret & UART_STATUS_TFNF))
		;

	fh_uart_writel(port, REG_UART_THR, ch);
}

static void
fh_serial_console_write(struct console *co, const char *s,
			unsigned int count)
{
	struct uart_port *port;

	if (co->index < 0)
		return;

	port = &fh_serial_ports[co->index];
	uart_console_write(port, s, count, fh_serial_console_putchar);
}


static int __init
fh_serial_console_setup(struct console *co, char *options)
{
	struct uart_port *port;
	int baud = 115200;
	int bits = 8;
	int parity = 'n';
	int flow = 'n';
	/* is this a valid port */
	if (co->index < 0 || co->index >= FH_UART_NUMBER) {
		pr_err("ERROR: co->index invaild: %d\n", co->index);
		return -EINVAL;
	}

	port = &fh_serial_ports[co->index];
	/* Console First Initialize */
#ifdef CONFIG_USE_OF
	fh_serial_of_configure();
#else
	port->ops = &fh_serial_ops;
#endif

	/* is the port configured? */
	if (port->mapbase == 0x0)
		pr_err("ERROR: port->mapbase == 0x0\n");

	/*
	 * Check whether an invalid uart number has been specified, and
	 * if so, search for the first available port that does have
	 * console support.
	 */
	if (options)
		uart_parse_options(options, &baud, &parity, &bits, &flow);
	uart_set_options(port, co, baud, parity, bits, flow);

	// must true for setup ok, see printk.c line:1463
	return 1;
}

#ifdef CONFIG_MAGIC_SYSRQ
static struct sysrq_key_op sysrq_fh_debug_mem_op = {
	.handler     = SysHandleFHMemory,
	.help_msg    = "Fullhan Debug Dump Mem(G)",
	.action_msg  = "Fullhan Dump Mem(w or r)",
	.enable_mask = SYSRQ_ENABLE_DUMP,
};
#endif

static int fh_serial_probe(struct platform_device *op)
{
	struct uart_port *port = NULL;
	struct clk *uart_clk   = NULL;
	char clk_name[16] = {0};
	int ret, id;

#ifdef CONFIG_USE_OF
	id = of_alias_get_id(op->dev.of_node, "ttyS");
#else
	id = op->id;
#endif
	port = &fh_serial_ports[id];
	ret = fh_serial_configure(port, op);
	if (ret < 0) {
		pr_info("fh serial driver probe fail.\n");
		return -EIO;
	}

	sprintf(clk_name, "uart%d_clk", id);
	uart_clk = clk_get(NULL, clk_name);
	if (!IS_ERR(uart_clk)) {
		ret = clk_prepare_enable(uart_clk);
		if (ret)
			return ret;
	}

	port->dev = &op->dev;

	uart_add_one_port(&fh_serial_driver, (struct uart_port *)port);

#ifdef CONFIG_MAGIC_SYSRQ
	register_sysrq_key('g', &sysrq_fh_debug_mem_op);
#endif
	pr_info("fh serial at 0x%x, irq %d\n",
			(resource_size_t) port->mapbase, port->irq);
	return 0;
}

static int fh_serial_remove(struct platform_device *op)
{
	/* This will never be called */
	struct uart_port *port = dev_get_drvdata(&op->dev);

	if (port)
		uart_remove_one_port(&fh_serial_driver, port);

	return 0;
}

#ifdef CONFIG_USE_OF
static struct of_device_id fh_serial_match[] = {
	{
		.compatible = "fh,fh-serial",
	},
	{},
};
#endif

static struct platform_driver fh_serial_of_driver = {
	.probe = fh_serial_probe,
	.remove = fh_serial_remove,
	.driver = {
		.owner = THIS_MODULE,
		.name = FH_DRIVE_NAME,
#ifdef CONFIG_USE_OF
		.of_match_table = fh_serial_match,
#endif
	},
};

#ifndef CONFIG_USE_OF
static struct early_platform_driver early_fh_uart_driver __initdata = {
	.class_str = "fh-console",
	.pdrv = &fh_serial_of_driver,
	.requested_id = EARLY_PLATFORM_ID_UNSET,
};
#endif

static int __init fh_serial_initconsole(void)
{
#ifndef CONFIG_USE_OF
	int ret = 0;

	ret = early_platform_driver_register(&early_fh_uart_driver, FH_DRIVE_NAME);

	ret = early_platform_driver_probe("fh-console", FH_UART_NUMBER, 0);
#endif
	register_console(&fh_serial_console);
	return 0;
}
console_initcall(fh_serial_initconsole);


static struct console fh_serial_console = {
	.name		= FH_DEV_NAME,
	.device		= uart_console_device,
	.flags		= CON_PRINTBUFFER | CON_ANYTIME | CON_ENABLED, //CON_ENABLED for ttys1 for 81
	.index		= -1,
	.write		= fh_serial_console_write,
	.setup		= fh_serial_console_setup,
	.data		= &fh_serial_driver,
};

#ifdef CONFIG_USE_OF
static int fh_serial_of_configure(void)
{
	struct device_node *np;
	int line;

	for_each_matching_node(np, fh_serial_match) {
		unsigned int irq, freq_hz, fifo_size, regs;
		struct uart_port *port;

		irq  = irq_of_parse_and_map(np, 0);
		line = of_alias_get_id(np, "ttyS");

		of_property_read_u32(np, "reg", &regs);
		of_property_read_u32(np, "clock-frequency", &freq_hz);
		of_property_read_u32(np, "fifo_size", &fifo_size);

		port = &fh_serial_ports[line];
		port->mapbase = regs;
		port->membase = ioremap(regs, MAP_SIZE);
		port->irq = irq;
		port->iotype = UPIO_MEM;
		port->ops = &fh_serial_ops;
		port->flags = UPF_BOOT_AUTOCONF;
		port->line = line;
		port->uartclk = freq_hz;
		port->fifosize = fifo_size;

		/* We support maximum UART_NR uarts ... */
		if (line == FH_UART_NUMBER)
			break;
	}

	return line ? 0 : -ENODEV;
}
#endif

static int fh_serial_configure(struct uart_port *port, struct platform_device *op)
{
#ifdef CONFIG_USE_OF
	const struct of_device_id *match;
	struct device_node *np = op->dev.of_node;
	unsigned int irq, freq_hz, fifo_size, regs;

	match = of_match_device(fh_serial_match, &op->dev);
	if (!match) {
		pr_info("Invalid uart device mismatch.\n");
		return -ENODEV;
	}

	irq = irq_of_parse_and_map(np, 0);
	of_property_read_u32(np, "reg", &regs);
	of_property_read_u32(np, "clock-frequency", &freq_hz);
	of_property_read_u32(np, "fifo-size", &fifo_size);

	port->mapbase = regs;
	port->membase = of_iomap(np, 0);
	port->irq = irq;
	port->iotype = UPIO_MEM;
	port->ops = &fh_serial_ops;
	port->flags = UPF_BOOT_AUTOCONF;
	port->line = of_alias_get_id(op->dev.of_node, "ttyS");
	port->uartclk = freq_hz;
	port->fifosize = fifo_size;

	return of_alias_get_id(op->dev.of_node, "ttyS");
#else
	struct fh_platform_uart *plat_data = dev_get_platdata(&op->dev);
	int filter_no;
	dma_cap_mask_t mask;

	if (op->id < 0 || op->id >= FH_UART_NUMBER) {
		pr_err("Invalid uart platform device id.\n");
		return -ENOENT;
	}

	port->mapbase = plat_data->mapbase;
	port->membase = ioremap(plat_data->mapbase, SZ_4K);
	port->irq = irq_create_mapping(NULL, plat_data->irq);
	port->iotype = UPIO_MEM;
	port->ops = &fh_serial_ops;
	port->flags = UPF_BOOT_AUTOCONF;
	port->line = op->id;
	port->uartclk = plat_data->uartclk;
	port->fifosize = plat_data->fifo_size;

	if (plat_data->use_dma) {
		struct fh_uart_dma *dma_info = plat_data->dma_info;

		dma_info->paddr = plat_data->mapbase;

#ifdef CONFIG_UART_TX_DMA
		filter_no = dma_info->tx_dma_channel;
		dma_cap_zero(mask);
		dma_cap_set(DMA_SLAVE, mask);
		dma_info->dma_tx.chan = dma_request_channel(mask,
			fh_uart_dma_chan_filter, &filter_no);
		if (!dma_info->dma_tx.chan) {
			op->id = -1;
			goto out;
		}
#endif

		filter_no = dma_info->rx_dma_channel;
		dma_cap_zero(mask);
		dma_cap_set(DMA_SLAVE, mask);
		dma_info->dma_rx.chan = dma_request_channel(mask,
			fh_uart_dma_chan_filter, &filter_no);
		if (!dma_info->dma_rx.chan) {
			dma_release_channel(dma_info->dma_tx.chan);
			op->id = -1;
			goto out;
		}

#ifdef CONFIG_UART_TX_DMA
		dma_info->tx_dma_add = dma_map_single(
			dma_info->dma_tx.chan->dev->device.parent,
			(void *)dma_info->tx_dumy_buff,
			sizeof(dma_info->tx_dumy_buff),
			DMA_TO_DEVICE);
#endif
		dma_info->rx_dma_add = dma_map_single(
			dma_info->dma_rx.chan->dev->device.parent,
			(void *)dma_info->rx_dumy_buff,
			sizeof(dma_info->rx_dumy_buff),
			DMA_TO_DEVICE);

		dma_info->tx_done   = 1;
		dma_info->tx_count  = 0;
		init_completion(&dma_info->rx_cmp);

		uart_dma[op->id] = dma_info;
	}

out:
	return op->id;
#endif
}

static int __init fh_serial_init(void)
{
	int ret;
	/* Find all UARTS in device the tree and initialize their ports */

	pr_info("Serial: fh serial driver\n");

	ret = uart_register_driver(&fh_serial_driver);

	if (ret) {
		pr_err("%s: uart_register_driver failed (%i)\n",
				__func__, ret);
		return ret;
	}

	ret = platform_driver_register(&fh_serial_of_driver);
	if (ret) {
		pr_err("%s: platform_driver_register failed (%i)\n",
				__func__, ret);
		uart_unregister_driver(&fh_serial_driver);
		return ret;
	}

	return ret;
}

static void __exit fh_serial_exit(void)
{
	int i;

	for (i = 0; i < FH_UART_NUMBER; i++)
		uart_remove_one_port(&fh_serial_driver,
					&fh_serial_ports[i]);

	uart_unregister_driver(&fh_serial_driver);
	platform_driver_unregister(&fh_serial_of_driver);
}

module_init(fh_serial_init);
module_exit(fh_serial_exit);

MODULE_AUTHOR("Fullhan Software Team");
MODULE_DESCRIPTION("Fullhan Serial device driver");
MODULE_VERSION("2.0");
MODULE_LICENSE("GPL");
