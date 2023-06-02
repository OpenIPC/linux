#if defined(CONFIG_SERIAL_FH_CONSOLE) && defined(CONFIG_MAGIC_SYSRQ)
#define SUPPORT_SYSRQ
#endif

#include <linux/module.h>
#include <linux/ioport.h>
#include <linux/io.h>
#include <linux/platform_device.h>
#include <linux/init.h>
#include <linux/sysrq.h>
#include <linux/console.h>
#include <linux/tty.h>
#include <linux/tty_flip.h>
#include <linux/serial_core.h>
#include <linux/serial.h>
#include <linux/delay.h>
#include <linux/clk.h>
#include <linux/cpufreq.h>
#include <linux/moduleparam.h>
#include <linux/ratelimit.h>
#include <linux/serial_reg.h>
#include <linux/nmi.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/types.h>
#include <linux/interrupt.h>
#include <linux/err.h>
#include <linux/sched.h>
#include <linux/timex.h>
#include <asm/irq.h>
#include <asm/mach/irq.h>
#include "fh_serial.h"
#include <linux/completion.h>
#include <linux/dmaengine.h>
#include <linux/dma-mapping.h>
#include <mach/pmu.h>
#include <linux/scatterlist.h>
#include <linux/syslog.h>
#include <mach/fh_dmac_regs.h>
#include <mach/chip.h>
/*********************************
 *
 * fh  private
 *
 *********************************/
#define     REG_UART_RBR		(0x0000)
#define     REG_UART_THR		(0x0000)
#define     REG_UART_DLL		(0x0000)
#define     REG_UART_DLH		(0x0004)
#define     REG_UART_IER		(0x0004)
#define     REG_UART_IIR		(0x0008)
#define     REG_UART_FCR		(0x0008)
#define     REG_UART_LCR		(0x000c)
#define     REG_UART_MCR		(0x0010)
#define     REG_UART_LSR		(0x0014)
#define     REG_UART_MSR		(0x0018)
#define     REG_UART_SCR		(0x001c)
#define     REG_UART_FAR		(0x0070)
#define     REG_UART_TFR		(0x0074)
#define     REG_UART_RFW		(0x0078)
#define     REG_UART_USR		(0x007c)
#define     REG_UART_TFL		(0x0080)
#define     REG_UART_RFL		(0x0084)
#define     REG_UART_SRR		(0x0088)
#define     REG_UART_SFE		(0x0098)
#define     REG_UART_SRT		(0x009c)
#define     REG_UART_STET		(0x00a0)
#define     REG_UART_HTX		(0x00a4)
#define     REG_UART_DMASA		(0x00a8)
#define     REG_UART_CPR		(0x00f4)
#define     REG_UART_UCV		(0x00f8)
#define     REG_UART_CTR		(0x00fc)

#define	DBGLINE()		printk(KERN_DEBUG \
				"file: %s\tfunc:%s\tline:%d\n",\
				__FILE__, __func__, __LINE__)
#define FH_SERIAL_NAME					"ttyS"
#define FH_DRIVE_NAME					"ttyS"
#define FH_DEV_NAME						"ttyS"

#define UART_DMA_TRANSFER_LEN   (8)

#define UART_READ_RX_DW_FIFO_OK			0
#define UART_READ_RX_DW_FIFO_TIME_OUT	0xcc
#define MAP_SIZE						0x80000

#ifdef CONFIG_SERIAL_FH_CONSOLE
static struct console fh_serial_console;
#define FH_SERIAL_CONSOLE		 (&fh_serial_console)
#else
#define FH_SERIAL_CONSOLE NULL
#endif

#define tx_enabled(port)		((port)->unused[0])
#define rx_enabled(port)		((port)->unused[1])
#define FH_TYPE					(99)
#define BOTH_EMPTY				(UART_LSR_TEMT | UART_LSR_THRE)
#define fh_dev_to_port(__dev)	(struct uart_port *)dev_get_drvdata(__dev)


#define fh_uart_readl(addr) \
	__raw_readl(addr)
#define fh_uart_writel(addr, val) \
	__raw_writel((val), addr)

#define fh_uart_readw(addr) \
	__raw_readw(addr)
#define fh_uart_writew(addr, val) \
	__raw_writew((val), addr)

/******************************************************************************
 * Function prototype section
 * add prototypes for all functions called by this file,execepting those
 * declared in header file
 *****************************************************************************/
static void fh_uart_pm(struct uart_port *port, unsigned int level,
			      unsigned int old);
static void fh_uart_stop_tx(struct uart_port *port);
static void fh_uart_start_tx(struct uart_port *port);
static void fh_uart_stop_rx(struct uart_port *port);
static void fh_uart_start_rx(struct uart_port *port);
static void fh_uart_enable_ms(struct uart_port *port);
static unsigned int fh_uart_tx_empty(struct uart_port *port);
static unsigned int fh_uart_get_mctrl(struct uart_port *port);
static void fh_uart_set_mctrl(struct uart_port *port, unsigned int mctrl);
static void fh_uart_break_ctl(struct uart_port *port, int break_state);
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
static int fh_uart_set_wake(struct uart_port *, unsigned int state);

struct fh_uart_port *to_fh_uart_port(struct uart_port *port);
struct fh_uart_port *info_to_fh_uart_port(fh_uart_info *info);
static bool fh_uart_dma_chan_filter(struct dma_chan *chan, void *param);
static void fh_serial_dma_tx_char_done(void *arg);
static void fh_serial_dma_rx_callback(void *arg);
static void fh_serial_dma_rx_char(void *arg);
static int uart_dma_set_rx_para(struct fh_uart_port *fh_uart_dma, int xfer_len,
						void (*call_back)(void *arg));
static int uart_dma_set_tx_para(struct fh_uart_port *fh_uart_dma, int xfer_len,
						void (*call_back)(void *arg));
static void fh_serial_dma_tx_char(struct fh_uart_port *uart_dma);
/*****************************************************************************
 * Global variables section - Local
 * define global variables(will be refered only in this file) here,
 * static keyword should be used to limit scope of local variable to this file
 * e.g.
 *	static uint8_t ufoo;
 *****************************************************************************/
static struct uart_ops fh_serial_ops = {
	.pm		= fh_uart_pm,
	.tx_empty	= fh_uart_tx_empty,
	.get_mctrl	= fh_uart_get_mctrl,
	.set_mctrl	= fh_uart_set_mctrl,
	.stop_tx	= fh_uart_stop_tx,
	.start_tx	= fh_uart_start_tx,
	.stop_rx	= fh_uart_stop_rx,
	.enable_ms	= fh_uart_enable_ms,
	.break_ctl	= fh_uart_break_ctl,
	.startup	= fh_serial_startup,
	.shutdown	= fh_serial_shutdown,
	.set_termios	= fh_serial_set_termios,
	.type		= fh_serial_type,
	.release_port	= fh_serial_release_port,
	.request_port	= fh_serial_request_port,
	.config_port	= fh_serial_config_port,
	.set_wake	= fh_uart_set_wake,
	.verify_port	= NULL,
};

static struct uart_driver fh_uart_drv = {
	.owner			= THIS_MODULE,
	.driver_name	= FH_DRIVE_NAME,
	.nr				= FH_UART_NUMBER,
	.cons			= FH_SERIAL_CONSOLE,
	.dev_name		= FH_DEV_NAME,
	.major			= 4,
	.minor			= 64,
};

#if (defined(CONFIG_ARCH_FH8810) || defined(CONFIG_ARCH_WUDANG))
#define UART_PORT0_ISR      ISR_NUMBER1
#define UART_PORT1_ISR      ISR_NUMBER0
#define UART_PORT0_BASE     UART1_REG_BASE
#define UART_PORT1_BASE     UART0_REG_BASE
#define UART_PORT0(x)       VUART1(x)
#define UART_PORT1(x)       VUART0(x)
#else
#define UART_PORT0_ISR      ISR_NUMBER0
#define UART_PORT0(x)       VUART0(x)
#define UART_PORT0_BASE     UART0_REG_BASE
#if FH_UART_NUMBER >= 2
#define UART_PORT1_ISR      ISR_NUMBER1
#define UART_PORT1_BASE     UART1_REG_BASE
#define UART_PORT1(x)       VUART1(x)
#endif
#if FH_UART_NUMBER >= 3
#define UART_PORT2_ISR      ISR_NUMBER2
#define UART_PORT2_BASE     UART2_REG_BASE
#define UART_PORT2(x)       VUART2(x)
#endif
#if FH_UART_NUMBER >= 4
#define UART_PORT3_ISR      ISR_NUMBER3
#define UART_PORT3_BASE     UART3_REG_BASE
#define UART_PORT3(x)       VUART3(x + 0x100)
#endif
#endif

static  struct fh_uart_port  fh_own_ports[FH_UART_NUMBER] = {
	[0] = {
		.port = {
			.lock		= __SPIN_LOCK_UNLOCKED(fh_own_ports[0].port.lock),
			.iotype		= UPIO_MEM,
			.irq		= UART_PORT0_ISR,
			.uartclk	= UART_CLOCK_FREQ,
			.fifosize	= UART0_FIFO_SIZE,
			.ops		= &fh_serial_ops,
			.flags		= UPF_BOOT_AUTOCONF,
			.line		= 0,
			.mapbase = UART_PORT0_BASE,
			.membase = (unsigned char __iomem *)UART_PORT0(UART_PORT0_BASE),
		},
		.fh_info = {
			.name = "FH UART0",
			.irq_num = UART_PORT0_ISR,
			.base_add = (unsigned char __iomem *)UART_PORT0(UART_PORT0_BASE),
			.baudrate = BAUDRATE_115200,
			.line_ctrl = Uart_line_8n2,
			.fifo_ctrl = UART_INT_RXFIFO_DEPTH_QUARTER,
		}
	},
#if FH_UART_NUMBER >= 2
	[1] = {
		.port = {
			.lock		= __SPIN_LOCK_UNLOCKED(fh_own_ports[1].port.lock),
			.iotype		= UPIO_MEM,
			.irq		= UART_PORT1_ISR,
			.uartclk	= UART_CLOCK_FREQ,
			.fifosize	= UART1_FIFO_SIZE,
			.ops		= &fh_serial_ops,
			.flags		= UPF_BOOT_AUTOCONF,
			.line		= 1,
			.mapbase = UART_PORT1_BASE,
			.membase = (unsigned char __iomem *)UART_PORT1(UART_PORT1_BASE),

		},
		.fh_info = {
			.name = "FH UART1",
			.irq_num = UART_PORT1_ISR,
			.base_add = (unsigned char __iomem *)UART_PORT1(UART_PORT1_BASE),
			.baudrate = BAUDRATE_115200,
			.line_ctrl = Uart_line_8n2,
			.fifo_ctrl = UART_DMA_RXFIFO_DEPTH_HALF,
			.use_dma = 0,
		},
		.uart_dma = {
			.tx_hs_no = UART1_TX_HW_HANDSHAKE,
			.rx_hs_no = UART1_RX_HW_HANDSHAKE,
			.tx_dma_channel = UART1_DMA_TX_CHAN,
			.rx_dma_channel = UART1_DMA_RX_CHAN,
			.rx_xmit_len    = UART_DMA_TRANSFER_LEN,
		}
	},
#endif
#if FH_UART_NUMBER >= 3
	[2] = {
		.port = {
			.lock		= __SPIN_LOCK_UNLOCKED(fh_own_ports[2].port.lock),
			.iotype		= UPIO_MEM,
			.irq		= UART_PORT2_ISR,
			.uartclk	= UART_CLOCK_FREQ,
			.fifosize	= UART2_FIFO_SIZE,
			.ops		= &fh_serial_ops,
			.flags		= UPF_BOOT_AUTOCONF,
			.line		= 2,
			.mapbase = UART_PORT2_BASE,
			.membase = (unsigned char __iomem *)UART_PORT2(UART_PORT2_BASE),

		},
		.fh_info = {
			.name = "FH UART2",
			.irq_num = UART_PORT2_ISR,
			.base_add = (unsigned char __iomem *)UART_PORT2(UART_PORT2_BASE),
			.baudrate = BAUDRATE_115200,
			.line_ctrl = Uart_line_8n2,
			.fifo_ctrl = UART_INT_RXFIFO_DEPTH_QUARTER,
		}
	},
#endif
#if FH_UART_NUMBER >= 4
	[3] = {
		.port = {
			.lock		= __SPIN_LOCK_UNLOCKED(fh_own_ports[3].port.lock),
			.iotype		= UPIO_MEM,
			.irq		= UART_PORT3_ISR,
			.uartclk	= UART_CLOCK_FREQ,
			.fifosize	= UART3_FIFO_SIZE,
			.ops		= &fh_serial_ops,
			.flags		= UPF_BOOT_AUTOCONF,
			.line		= 3,
			.mapbase = UART_PORT3_BASE,
			.membase = (unsigned char __iomem *)UART_PORT3(UART_PORT3_BASE),

		},
		.fh_info = {
			.name = "FH UART3",
			.irq_num = UART_PORT3_ISR,
			.base_add = (unsigned char __iomem *)UART_PORT3(UART_PORT3_BASE),
			.baudrate = BAUDRATE_115200,
			.line_ctrl = Uart_line_8n2,
			.fifo_ctrl = UART_INT_RXFIFO_DEPTH_QUARTER,
		}
	},
#endif
};

struct fh_uart_port *to_fh_uart_port(struct uart_port *port)
{
	return container_of(port, struct fh_uart_port, port);

}

struct fh_uart_port *info_to_fh_uart_port(fh_uart_info *info)
{
	return container_of(info, struct fh_uart_port, fh_info);

}

s32 Uart_Disable_Irq(fh_uart_info *desc, uart_irq_e interrupts)
{
	u32 ret;
	u32 base = (u32)desc->base_add;

	ret = fh_uart_readl(base + REG_UART_IER);
	ret &= ~interrupts;
	fh_uart_writel(base+REG_UART_IER, ret);

	return UART_CONFIG_OK;
}

s32 Uart_Enable_Irq(fh_uart_info *desc, uart_irq_e interrupts)
{
	u32 ret;
	u32 base = (u32)desc->base_add;

	ret = fh_uart_readl(base);
	ret |= interrupts;
	fh_uart_writel(base + REG_UART_IER, ret);

	return UART_CONFIG_OK;

}

s32 Uart_Fifo_Config(fh_uart_info *desc)
{
	u32 ret;
	u32 base = (u32)desc->base_add;

	fh_uart_writel(base + REG_UART_FCR, desc->fifo_ctrl);
	ret = fh_uart_readl(base + REG_UART_IIR);

	if (ret & UART_FIFO_IS_ENABLE)
		return UART_CONFIG_FIFO_OK;
	else
		return UART_CONFIG_FIFO_ERROR;
}

s32 Uart_Read_Control_Status(fh_uart_info *desc)
{
	u32 base = (u32)desc->base_add;
	return fh_uart_readl(base + REG_UART_USR);
}

s32 Uart_Set_Line_Control(fh_uart_info *desc)
{
	u32 ret;
	u32 base = (u32)desc->base_add;

	ret = Uart_Read_Control_Status(desc);
	if (ret & UART_STATUS_BUSY)
		return UART_IS_BUSY;

	fh_uart_writel(base + REG_UART_LCR, desc->line_ctrl);
	return UART_CONFIG_LINE_OK;
}

s32 Uart_Read_Line_Status(fh_uart_info *desc)
{
	u32 base = (u32)desc->base_add;
	return fh_uart_readl(base + REG_UART_LSR);
}

s32 Uart_Set_Clock_Divisor(fh_uart_info *desc)
{
	u32 low, high, ret;
	u32 base = (u32)desc->base_add;

	low = desc->baudrate & 0x00ff;
	high = (desc->baudrate & 0xff00) >> 8;

	ret = Uart_Read_Control_Status(desc);
	if (ret & UART_STATUS_BUSY)
		return UART_IS_BUSY;

	ret = fh_uart_readl(base + REG_UART_LCR);
	/* if DLAB not set */
	if (!(ret & UART_LCR_DLAB_POS)) {
		ret |= UART_LCR_DLAB_POS;
		fh_uart_writel(base + REG_UART_LCR, ret);
	}
	fh_uart_writel(base + REG_UART_DLL, low);
	fh_uart_writel(base + REG_UART_DLH, high);

	/* clear DLAB */
	ret = ret & 0x7f;
	fh_uart_writel(base + REG_UART_LCR, ret);

	return UART_CONFIG_DIVISOR_OK;
}

s32 Uart_Read_iir(fh_uart_info *desc)
{
	u32 base = (u32)desc->base_add;
	return fh_uart_readl(base + REG_UART_IIR);
}

static bool fh_uart_dma_chan_filter(struct dma_chan *chan, void *param)
{
	int dma_channel = *(int *) param;
	bool ret = false;
	if (chan->chan_id == dma_channel)
		ret = true;
	return ret;
}
static void fh_serial_dma_tx_char_done(void *arg)
{
	struct fh_uart_port *uart_dma = (struct fh_uart_port *)arg;
	uart_dma->port.icount.tx += uart_dma->uart_dma.tx_count;
	uart_dma->uart_dma.tx_done = 1;
}
static void fh_serial_dma_rx_char(void *arg)
{
	struct fh_uart_port *uart_dma = (struct fh_uart_port *)arg;
	struct uart_port *port = &uart_dma->port;
	struct tty_struct *tty = uart_dma->port.state->port.tty;
	unsigned int uerstat;
	unsigned int flag;
	unsigned int xmit_len;
	unsigned int i;
	xmit_len = uart_dma->uart_dma.rx_xmit_len;
	uart_dma->port.icount.rx += xmit_len;
	uerstat = Uart_Read_Line_Status(&uart_dma->fh_info);
	flag = TTY_NORMAL;
	if (unlikely(uerstat & UART_LINE_STATUS_RFE)) {
		printk(KERN_INFO "rxerr: port rxs=0x%08x\n", uerstat);
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
	for (i = uart_dma->uart_dma.rx_rd_ptr; \
		i < uart_dma->uart_dma.rx_rd_ptr + xmit_len; i++) {
		if (!uart_handle_sysrq_char(&uart_dma->port, \
			uart_dma->uart_dma.rx_dumy_buff[i])) {
			uart_insert_char(&uart_dma->port, uerstat, \
				UART_LINE_STATUS_OE,\
				uart_dma->uart_dma.rx_dumy_buff[i], flag);
		}
	}
dma_ignore_char:
	tty_flip_buffer_push(tty);
}
static int uart_dma_set_rx_para(struct fh_uart_port *fh_uart_dma,
					int xfer_len,
					void (*call_back)(void *arg))
{
	struct fh_dma_extra ext_para;
	struct dma_slave_config *rx_config;
	struct dma_chan *rxchan;
	struct scatterlist *p_sca_list;
	unsigned int sg_size = 1;
	memset(&fh_uart_dma->uart_dma.dma_rx.cfg,
			 0,
			sizeof(struct dma_slave_config));
	memset(&ext_para, 0, sizeof(struct fh_dma_extra));
	p_sca_list = &fh_uart_dma->uart_dma.dma_rx.sgl[0];
	rxchan = fh_uart_dma->uart_dma.dma_rx.chan;
	rx_config = &fh_uart_dma->uart_dma.dma_rx.cfg;
	rx_config->src_addr = fh_uart_dma->uart_dma.paddr + REG_UART_RBR;
	rx_config->dst_addr_width = DMA_SLAVE_BUSWIDTH_1_BYTE;
	rx_config->src_addr_width = DMA_SLAVE_BUSWIDTH_1_BYTE;
	rx_config->slave_id = fh_uart_dma->uart_dma.rx_hs_no;
	rx_config->src_maxburst = 8;
	rx_config->dst_maxburst = 8;
	rx_config->direction = DMA_DEV_TO_MEM;
	rx_config->device_fc = 0;
	ext_para.sinc = FH_DMA_SLAVE_FIX;
	p_sca_list->dma_address = fh_uart_dma->uart_dma.rx_dma_add \
		+ fh_uart_dma->uart_dma.rx_wr_ptr;
	p_sca_list->length = min(xfer_len, 128);
	ext_para.dinc = FH_DMA_SLAVE_INC;
	dmaengine_slave_config(rxchan, rx_config);
	fh_uart_dma->uart_dma.dma_rx.desc =
		rxchan->device->device_prep_slave_sg(rxchan,
		&fh_uart_dma->uart_dma.dma_rx.sgl[0], sg_size, DMA_DEV_TO_MEM,
		DMA_PREP_INTERRUPT | DMA_COMPL_SKIP_DEST_UNMAP,
		&ext_para);
	fh_uart_dma->uart_dma.dma_rx.actual_sgl_size = sg_size;
	fh_uart_dma->uart_dma.dma_rx.desc->callback = fh_serial_dma_rx_callback;
	fh_uart_dma->uart_dma.dma_rx.desc->callback_param = fh_uart_dma;
	return 0;
}
static void fh_serial_dma_rx_callback(void *arg)
{
	struct fh_uart_port *port = (struct fh_uart_port *)arg;
	port->uart_dma.rx_wr_ptr =
		(port->uart_dma.rx_wr_ptr + port->uart_dma.rx_xmit_len) & (128 - 1);
	fh_serial_dma_rx_char(port);
	port->uart_dma.rx_rd_ptr = port->uart_dma.rx_wr_ptr;
	uart_dma_set_rx_para(port,
						 port->uart_dma.rx_xmit_len,
						 fh_serial_dma_rx_callback);
	port->uart_dma.dma_rx.desc->tx_submit(port->uart_dma.dma_rx.desc);
}
static int uart_dma_set_tx_para(struct fh_uart_port *fh_uart_dma,
						int xfer_len,
						void (*call_back)(void *arg))
{
	struct fh_dma_extra ext_para;
	struct dma_slave_config *tx_config;
	struct dma_chan *txchan;
	struct scatterlist *p_sca_list;
	unsigned int sg_size = 1;
	memset(&fh_uart_dma->uart_dma.dma_tx.cfg,
			 0,
			 sizeof(struct dma_slave_config));
	memset(&ext_para, 0, sizeof(struct fh_dma_extra));
	p_sca_list = &fh_uart_dma->uart_dma.dma_tx.sgl[0];
	txchan = fh_uart_dma->uart_dma.dma_tx.chan;
	tx_config = &fh_uart_dma->uart_dma.dma_tx.cfg;
	tx_config->dst_addr = fh_uart_dma->uart_dma.paddr + REG_UART_THR;
	tx_config->dst_addr_width = DMA_SLAVE_BUSWIDTH_1_BYTE;
	tx_config->src_addr_width = DMA_SLAVE_BUSWIDTH_1_BYTE;
	tx_config->slave_id = fh_uart_dma->uart_dma.tx_hs_no;
	tx_config->src_maxburst = 1;
	tx_config->dst_maxburst = 1;
	tx_config->direction = DMA_MEM_TO_DEV;
	tx_config->device_fc = 0;
	ext_para.sinc = FH_DMA_SLAVE_FIX;
	p_sca_list->dma_address = fh_uart_dma->uart_dma.tx_dma_add;
	p_sca_list->length = min(xfer_len, 128);
	ext_para.dinc = FH_DMA_SLAVE_INC;
	dmaengine_slave_config(txchan, tx_config);
	fh_uart_dma->uart_dma.dma_tx.desc =
		txchan->device->device_prep_slave_sg(txchan,
		&fh_uart_dma->uart_dma.dma_tx.sgl[0], sg_size, DMA_MEM_TO_DEV,
		DMA_PREP_INTERRUPT | DMA_COMPL_SKIP_DEST_UNMAP,
		&ext_para);
	fh_uart_dma->uart_dma.dma_tx.actual_sgl_size = sg_size;
	fh_uart_dma->uart_dma.dma_tx.desc->callback = fh_serial_dma_tx_char_done;
	fh_uart_dma->uart_dma.dma_tx.desc->callback_param = fh_uart_dma;
	return 0;
}
static void fh_serial_dma_tx_char(struct fh_uart_port *uart_dma)
{
	unsigned int xfer_len;
	struct circ_buf *xmit = &uart_dma->port.state->xmit;
	if (uart_circ_empty(xmit) || uart_tx_stopped(&uart_dma->port)) {
		uart_dma->uart_dma.tx_done = 1;
		uart_dma->uart_dma.tx_done = 0;
		return;
	}
	if (uart_dma->uart_dma.tx_done) {
		xfer_len =
			uart_circ_chars_pending(xmit); /* Check xfer length */
		if (xfer_len > 128)
			xfer_len = 128;
		if (xmit->tail < xmit->head) {
			memcpy(uart_dma->uart_dma.tx_dumy_buff,
				 &xmit->buf[xmit->tail],
				 xfer_len);
		} else {
			int first = UART_XMIT_SIZE - xmit->tail;
			int second = xmit->head;
			memcpy(uart_dma->uart_dma.tx_dumy_buff,
				 &xmit->buf[xmit->tail],
				 first);
			if (second)
				memcpy(&uart_dma->uart_dma.tx_dumy_buff[first],
					 &xmit->buf[0],
					 second);
		}
		xmit->tail = (xmit->tail + xfer_len) & (UART_XMIT_SIZE - 1);
		uart_dma->uart_dma.tx_count = xfer_len;
		uart_dma_set_tx_para(
						uart_dma,
						xfer_len,
						fh_serial_dma_tx_char_done);
		uart_dma->uart_dma.dma_tx.desc->tx_submit(uart_dma->uart_dma.dma_tx.desc);
		uart_dma->uart_dma.tx_done = 0;
	}
}
void fh_uart_rx_dma_start(struct fh_uart_port *port)
{
	uart_dma_set_rx_para(port,
						port->uart_dma.rx_xmit_len,
						fh_serial_dma_rx_callback);
	port->uart_dma.dma_rx.desc->tx_submit(port->uart_dma.dma_rx.desc);
}

s32 Uart_Init(fh_uart_info *desc)
{

	u32 base = (u32)desc->base_add;
	struct fh_uart_port *port = info_to_fh_uart_port(desc);
	u8 test_init_status = 0;

	/* reset fifo */
	fh_uart_writel(base + REG_UART_FCR, 6);
	test_init_status |= Uart_Set_Clock_Divisor(desc);
	test_init_status |= Uart_Set_Line_Control(desc);
	test_init_status |= Uart_Fifo_Config(desc);
	if (test_init_status != 0)
		return test_init_status;

	Uart_Disable_Irq(desc, UART_INT_ALL);
	if (!port->fh_info.use_dma)
		fh_uart_start_rx(&port->port);
	else {
		if (!port->uart_dma.HasInit) {
			port->uart_dma.HasInit = 1;
			fh_uart_rx_dma_start(port);
		}
	}
	return 0;
}



/*********************************
 *
 *
 * FH  CONSOLE
 *
 *
 *********************************/
#ifdef CONFIG_SERIAL_FH_CONSOLE
static struct uart_port *cons_uart;

static void
fh_serial_console_putchar(struct uart_port *port, int ch)
{
	u32 ret;
	struct fh_uart_port *myown_port = to_fh_uart_port(port);

	do {
		ret = (u8)Uart_Read_Control_Status(&myown_port->fh_info);
	}
	/* wait txfifo is full
	 * 0 means full.
	 * 1 means not full
	 */
	while (!(ret & UART_STATUS_TFNF))
		;

	fh_uart_writel(myown_port->fh_info.base_add + REG_UART_THR, ch);
}

static void
fh_serial_console_write(struct console *co, const char *s,
				unsigned int count)
{
	struct fh_uart_port *myown_port  = &fh_own_ports[co->index];
	uart_console_write(&myown_port->port,
					s,
					count,
					fh_serial_console_putchar);
}


static int __init
fh_serial_console_setup(struct console *co,
						char *options)
{
	struct uart_port *port;
	int baud = 115200;
	int bits = 8;
	int parity = 'n';
	int flow = 'n';
	/* is this a valid port */
	if ((co->index == -1) || (co->index >= FH_UART_NUMBER))
		pr_err("ERROR: co->index invaild: %d\n", co->index);

	port = &fh_own_ports[co->index].port;

	/* is the port configured? */
	if (port->mapbase == 0x0)
		pr_err("ERROR: port->mapbase == 0x0\n");

	cons_uart = port;
	/*
	 * Check whether an invalid uart number has been specified, and
	 * if so, search for the first available port that does have
	 * console support.
	 */
	if (options)
		uart_parse_options(options, &baud, &parity, &bits, &flow);
	uart_set_options(port, co, baud, parity, bits, flow);

	/* must true for setup ok, see printk.c line:1463 */
	return 1;
}


int fh_serial_initconsole(void)
{
	fh_serial_console.data = &fh_uart_drv;
	register_console(&fh_serial_console);
	return 0;
}
console_initcall(fh_serial_initconsole);

static struct console fh_serial_console = {
	.name		= FH_SERIAL_NAME,
	.device		= uart_console_device,
	.flags		= CON_PRINTBUFFER | CON_ANYTIME | CON_ENABLED,
	.index		= -1,
	.write		= fh_serial_console_write,
	.setup		= fh_serial_console_setup,
	.data		= &fh_uart_drv,
};

#endif /* CONFIG_SERIAL_FH_CONSOLE */

static void fh_uart_stop_tx(struct uart_port *port)
{
	/* close tx isr */
	struct fh_uart_port *myown_port = to_fh_uart_port(port);
	struct circ_buf *xmit = &myown_port->port.state->xmit;
	u32 base = (u32)myown_port->fh_info.base_add;
	if (!myown_port->fh_info.use_dma)
		fh_uart_writel(base + REG_UART_IER, 0x01);
	else{
		xmit->tail =
		(xmit->tail + myown_port->uart_dma.tx_count) & (UART_XMIT_SIZE - 1);
		myown_port->port.icount.tx += myown_port->uart_dma.tx_count;
		myown_port->uart_dma.tx_count = 0;
		myown_port->uart_dma.tx_done = 1;
	}
	tx_enabled(port) = 0;
	fh_uart_writel(base + REG_UART_IER, 0x01);
}

static void fh_uart_start_tx(struct uart_port *port)
{
	/* open tx isr */
	struct fh_uart_port *myown_port = to_fh_uart_port(port);

	if (!myown_port->fh_info.use_dma) {
		u32 base = (u32)myown_port->fh_info.base_add;
		fh_uart_writel(base + REG_UART_IER, 0x03);
		tx_enabled(port) = 1;
	} else {
		if (myown_port->uart_dma.tx_done) {
			fh_serial_dma_tx_char(myown_port);
			tx_enabled(port) = 1;
		}
	}
}

static void fh_uart_stop_rx(struct uart_port *port)
{
	struct fh_uart_port *myown_port = to_fh_uart_port(port);
	rx_enabled(port) = 0;
	Uart_Disable_Irq(&myown_port->fh_info, UART_INT_ERBFI_POS);
}

static void fh_uart_start_rx(struct uart_port *port)
{
	struct fh_uart_port *myown_port = to_fh_uart_port(port);
	rx_enabled(port) = 1;
	Uart_Enable_Irq(&myown_port->fh_info, UART_INT_ERBFI_POS);
}

static void fh_uart_pm(struct uart_port *port, unsigned int level,
			      unsigned int old)
{

}

static int fh_uart_set_wake(struct uart_port *port, unsigned int state)
{
	return 0;
}


static void fh_uart_enable_ms(struct uart_port *port)
{

}


static unsigned int fh_uart_tx_empty(struct uart_port *port)
{
	struct fh_uart_port *myown_port = to_fh_uart_port(port);
	/*
	 * 1 means empty
	 * 0:means no empty
	 */
	int ret = 1;
	int ret_status;

	ret_status = (u8)Uart_Read_Control_Status(&myown_port->fh_info);
	if (ret_status & UART_STATUS_TFE)
		ret = 1;
	else
		ret = 0;
	return ret;
}


static unsigned int fh_uart_get_mctrl(struct uart_port *port)
{
	return 0;
}


static void fh_uart_set_mctrl(struct uart_port *port, unsigned int mctrl)
{

}

static void fh_uart_break_ctl(struct uart_port *port, int break_state)
{

}

static irqreturn_t
fh_uart_rx_chars(int irq, void *dev_id)
{
	struct fh_uart_port *myown_port  = dev_id;
	struct uart_port *port = &myown_port->port;
	struct tty_struct *tty = port->state->port.tty;
	unsigned int ch = 0;
	unsigned int flag;
	unsigned int uerstat;
	int max_count = 64;
	int ret_status;

	while (max_count-- > 0) {
		/* check if rx fifo is empty */
		ret_status = (u8)Uart_Read_Control_Status(&myown_port->fh_info);
		if (!(ret_status & UART_STATUS_RFNE))
			break;
		/* read error in the rx process */
		uerstat = Uart_Read_Line_Status(&myown_port->fh_info);
		/* read  data in the rxfifo */
		if (uerstat & UART_LINE_STATUS_DR)
			ch = fh_uart_readl(myown_port->fh_info.base_add + REG_UART_RBR);
		/* insert the character into the buffer */
		flag = TTY_NORMAL;
		port->icount.rx++;
		/* if at least one error in rx process */
		if (unlikely(uerstat & UART_LINE_STATUS_RFE)) {
			pr_err("rxerr: port ch=0x%02x, rxs=0x%08x\n",
				ch, uerstat);
			/* check for break */
			if (uerstat & UART_LINE_STATUS_BI) {
				port->icount.brk++;
				if (uart_handle_break(port))
					goto ignore_char;
			}

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
		}

		if (uart_handle_sysrq_char(port, ch))
			goto ignore_char;

		uart_insert_char(port, uerstat, UART_LINE_STATUS_OE,
				 ch, flag);

 ignore_char:
		continue;
	}
	tty_flip_buffer_push(tty);
	return IRQ_HANDLED;
}



static irqreturn_t
fh_uart_tx_chars(int irq, void *dev_id)
{
	struct fh_uart_port* myown_port  = dev_id;
	struct uart_port *port = &myown_port->port;
	struct circ_buf *xmit = &port->state->xmit;
	int count = 256;
	int ret_status;

	/* if there isn't anything more to transmit, or the uart is now
	 * stopped, disable the uart and exit
	*/
	if (uart_circ_empty(xmit) || uart_tx_stopped(port)) {
		fh_uart_stop_tx(port);
		goto out;
	}
	/* try and drain the buffer... */
	while (!uart_circ_empty(xmit) && count-- > 0) {

		/*
		 *  check the tx fifo full?
		 * full then break
		 */
		ret_status = (u8)Uart_Read_Control_Status(&myown_port->fh_info);
		if(!(ret_status & UART_STATUS_TFNF))
			break;
		/* write data to the hw fifo */
		fh_uart_writel(myown_port->fh_info.base_add + REG_UART_THR, \
				xmit->buf[xmit->tail]);
		xmit->tail = (xmit->tail + 1) & (UART_XMIT_SIZE - 1);
		port->icount.tx++;
	}

	if (uart_circ_chars_pending(xmit) < WAKEUP_CHARS)
		uart_write_wakeup(port);

	if (uart_circ_empty(xmit))
		fh_uart_stop_tx(port);
out:
	return IRQ_HANDLED;
}

static irqreturn_t
fh_uart_isr(int irq, void *dev_id)
{
	irqreturn_t ret_isr;
	struct fh_uart_port* myown_port  = dev_id;
	int ret_iir;

	/* check if the tx empty isr */
	ret_iir = Uart_Read_iir(&myown_port->fh_info);
	if (ret_iir == 0x06)
		pr_err("uart overrun\n");

	if ((ret_iir & 0x04)||(ret_iir & 0x0c))
		ret_isr = fh_uart_rx_chars(irq,dev_id);

	if (ret_iir & 0x02)
		ret_isr = fh_uart_tx_chars(irq,dev_id);
	else
		ret_isr = IRQ_HANDLED;

	return ret_isr;
}

static void fh_serial_shutdown(struct uart_port *port)
{
	struct fh_uart_port* myown_port = to_fh_uart_port(port);

	Uart_Disable_Irq(&myown_port->fh_info,UART_INT_ALL);
	fh_uart_writel( myown_port->fh_info.base_add + REG_UART_FCR, 6);
	free_irq(myown_port->fh_info.irq_num, myown_port);
	tx_enabled(port) = 0;
	rx_enabled(port) = 0;
}

static int fh_serial_startup(struct uart_port *port)
{
	struct fh_uart_port* myown_port = to_fh_uart_port(port);
	int ret;
	int status;

	do {
		status = Uart_Read_Line_Status(&myown_port->fh_info);

	} while ((status & BOTH_EMPTY) != BOTH_EMPTY);

	Uart_Init(&myown_port->fh_info);
	if ((ret = request_irq(myown_port->fh_info.irq_num,
							fh_uart_isr,
							0,
							FH_DEV_NAME,
							(void*)myown_port))) {
		pr_err("cannot get irq %d\n", myown_port->fh_info.irq_num);
		return ret;
	}

	enable_irq_wake(myown_port->fh_info.irq_num);

	return 0;
}

static void fh_serial_set_termios(struct uart_port *port,
				       struct ktermios *termios,
				       struct ktermios *old)
{
	struct fh_uart_port* myown_port = to_fh_uart_port(port);

	unsigned long flags;
	unsigned int baud, quot;
	unsigned int line_data = 0,status;

	do {
		status = Uart_Read_Line_Status(&myown_port->fh_info);

	} while ((status & BOTH_EMPTY) != BOTH_EMPTY);

	switch (termios->c_cflag & CSIZE) {
	case CS5:
		line_data |= UART_LCR_WLEN5;
		break;
	case CS6:
		line_data |= UART_LCR_WLEN6;
		break;
	case CS7:
		line_data |= UART_LCR_WLEN7;
		break;
	case CS8:
		line_data |= UART_LCR_WLEN8;
		break;
	default:
		line_data |= UART_LCR_WLEN8;
		break;
	}
	/* stop bits */
	if (termios->c_cflag & CSTOPB)
		line_data |= UART_LCR_STOP;

	if (termios->c_cflag & PARENB) {
		line_data |= UART_LCR_PARITY;

		if (!(termios->c_cflag & PARODD))
			line_data |= UART_LCR_EPAR;
	}
	/*
	 * baud cal.
	 * baud is the uart will be out.
	 * the quot is the div
	 */
	baud = uart_get_baud_rate(port, termios, old, 0, port->uartclk/16);
	quot = uart_get_divisor(port, baud);

	do{
		status = Uart_Read_Line_Status(&myown_port->fh_info);
	} while ((status & BOTH_EMPTY) != BOTH_EMPTY);

	uart_update_timeout(port, termios->c_cflag, baud);
	spin_lock_irqsave(&myown_port->port.lock, flags);

	myown_port->port.read_status_mask =
			UART_LSR_OE | UART_LSR_THRE | UART_LSR_DR;
	if (termios->c_iflag & INPCK)
		myown_port->port.read_status_mask |= UART_LSR_FE | UART_LSR_PE;
	if (termios->c_iflag & (BRKINT | PARMRK))
		myown_port->port.read_status_mask |= UART_LSR_BI;

	/*
	 * Characters to ignore
	 */
	myown_port->port.ignore_status_mask = 0;
	if (termios->c_iflag & IGNPAR)
		myown_port->port.ignore_status_mask |= UART_LSR_PE | UART_LSR_FE;
	if (termios->c_iflag & IGNBRK) {
		myown_port->port.ignore_status_mask |= UART_LSR_BI;
		/*
		 * If we're ignoring parity and break indicators,
		 * ignore overruns too (for real raw support).
		 */
		if (termios->c_iflag & IGNPAR)
			myown_port->port.ignore_status_mask |= UART_LSR_OE;
	}

	/*
	 * ignore all characters if CREAD is not set
	 */
	if ((termios->c_cflag & CREAD) == 0)
		myown_port->port.ignore_status_mask |= UART_LSR_DR;


	myown_port->fh_info.line_ctrl = (uart_line_e)line_data;
	myown_port->fh_info.baudrate = quot;
	myown_port->fh_info.fifo_ctrl = UART_INT_RXFIFO_DEPTH_QUARTER;
	Uart_Init(&myown_port->fh_info);
	spin_unlock_irqrestore(&myown_port->port.lock, flags);
}



static const char *fh_serial_type(struct uart_port *port)
{
	return FH_SERIAL_NAME;
}

static void fh_serial_release_port(struct uart_port *port)
{
	release_mem_region(port->mapbase, MAP_SIZE);
}

static int fh_serial_request_port(struct uart_port *port)
{
	struct fh_uart_port* myown_port = to_fh_uart_port(port);
	const char* name = myown_port->fh_info.name;
	return request_mem_region(port->mapbase, MAP_SIZE, name) ? 0 : -EBUSY;
}


static void fh_serial_config_port(struct uart_port *port, int flags)
{
	if (flags & UART_CONFIG_TYPE) {
		fh_serial_request_port(port);
		port->type = FH_TYPE;
	}
}

static int fh_serial_init_port(struct fh_uart_port *myown_port,

				    struct platform_device *platdev)
{
	struct uart_port *port = &myown_port->port;
	struct resource *res;

	if (platdev == NULL)
		return -ENODEV;


	myown_port->fh_info.dev = platdev;
	/* setup info for port */
	port->dev	= &platdev->dev;

	/* sort our the physical and virtual addresses for each UART */
	res = platform_get_resource(platdev, IORESOURCE_MEM, 0);
	if (res == NULL) {
		printk(KERN_ERR "failed to find memory resource for uart\n");
		return -EINVAL;
	}

	myown_port->uart_dma.paddr = res->start;
	myown_port->fh_info.baudrate = BAUDRATE_115200;
	if (myown_port->fh_info.use_dma)
		myown_port->fh_info.fifo_ctrl = 0x8f;
	else
		myown_port->fh_info.fifo_ctrl = UART_INT_RXFIFO_DEPTH_QUARTER;
	myown_port->fh_info.line_ctrl = Uart_line_8n2;

	Uart_Init(&myown_port->fh_info);
	return 0;
}

static inline int fh_serial_cpufreq_register(struct fh_uart_port* myown_port)
{
	return 0;
}

static ssize_t fh_serial_show_clksrc(struct device *dev,
					  struct device_attribute *attr,
					  char *buf)
{
	return snprintf(buf, PAGE_SIZE, "* %s\n", FH_SERIAL_NAME);
}

static DEVICE_ATTR(clock_source, S_IRUGO, fh_serial_show_clksrc, NULL);

#ifdef CONFIG_PM
static int fh_serial_suspend(struct platform_device *pdev,
				pm_message_t state)
{
	struct uart_port *port = platform_get_drvdata(pdev);

	uart_suspend_port(&fh_uart_drv, port);

	return 0;
}

static int fh_serial_resume(struct platform_device *pdev)
{
	struct uart_port *port = platform_get_drvdata(pdev);
	int may_wakeup;

	may_wakeup = device_may_wakeup(&pdev->dev);

	uart_resume_port(&fh_uart_drv, port);
	device_set_wakeup_enable(&pdev->dev, may_wakeup);

	return 0;
}
#endif

#ifdef CONFIG_MAGIC_SYSRQ
static unsigned int SysrqParseHex(char *buff)
{
	unsigned int data;

	data = (unsigned int)simple_strtoul(buff, NULL, 16);
	return data;
}

static int SysrqGetSingle(void)
{
	struct fh_uart_port* myown_port;
	unsigned int ch;

	myown_port = &fh_own_ports[0];
	while (1) {
		if ((fh_uart_readl(myown_port->fh_info.base_add + REG_UART_LSR) & 0x01) == 0x01) {
			ch = fh_uart_readl(myown_port->fh_info.base_add + REG_UART_RBR) & 0xff;
			return ch;
		}
	}
}

static int SysrqGetValue(void)
{
	unsigned char internal_buffer[11] = {0};
	unsigned char loop = 0;
	unsigned char data;
	unsigned int value;
	struct fh_uart_port *myown_port;

	myown_port = &fh_own_ports[0];
	while (1) {
		if ((fh_uart_readl(myown_port->fh_info.base_add + REG_UART_LSR) & 0x01) == 0x01) {
			data = fh_uart_readl(myown_port->fh_info.base_add + REG_UART_RBR) & 0xff;
			/*Get Next Param*/
			if (data == ' ' || data == '\r' || data == '\n') {
				internal_buffer[loop] = '\0';
				value = SysrqParseHex(internal_buffer);
				return value;
			} else {
				internal_buffer[loop] = data;
				loop++;
				if (loop >= sizeof(internal_buffer)) {
					printk(KERN_INFO "Address or length Error!\n");
					return 0;
				}
			}
		}
	}
}

static int SysrqGetDirection(void)
{
	unsigned int ch;

	ch = SysrqGetSingle();
	if (ch == 'w' || ch == 'W')
		return 1;
	else if (ch == 'r' || ch == 'R')
		return 0;
	else
		return -1;
}

static int SysrqGetMode(void)
{
	unsigned int ch;

	ch = SysrqGetSingle();
	if (ch == 'v' || ch == 'V')
		return 1;
	else if (ch == 'p' || ch == 'P')
		return 0;
	else
		return -1;
}

static void sysrq_handle_fh_mem(int key)
{
	unsigned int ch;
	unsigned int mode;
	unsigned int direction;
	unsigned int vir_address;
	unsigned int phy_address;
	unsigned int len;
	unsigned int start = 0;

	direction = SysrqGetDirection();
	if (direction == 1) {
		printk(KERN_INFO "Write Mode(p or v)\n");
		mode = SysrqGetMode();
		if (mode == 0) {
			printk(KERN_INFO \
				"Physical Address Write:\n");
			phy_address = SysrqGetValue();
			len = SysrqGetValue();
			printk(KERN_INFO \
			"Start Physical Address : 0x%x\n", phy_address);
			printk(KERN_INFO "Write Data : 0x%x\n", len);
			vir_address = __phys_to_virt(phy_address);
			writel(len, vir_address);
			printk(KERN_INFO \
			"Address:0x%x at 0x%x\n", vir_address, len);
		} else if (mode == 1) {
			printk(KERN_INFO \
				"Virtual Address Write:\n");
			vir_address = SysrqGetValue();
			len = SysrqGetValue();
			writel(len, vir_address);
			printk(KERN_INFO \
				"Address:0x%x at 0x%x\n", vir_address, len);
		} else {
			printk(KERN_INFO "Select Wrong Mode!\n");
			return;
		}
	} else if (direction == 0) {
		printk(KERN_INFO "Read Mode(p or v)\n");
		mode = SysrqGetMode();
		if (mode == 0) {
			printk(KERN_INFO \
				"Physical Address Read:\n");
			phy_address = SysrqGetValue();
			len = SysrqGetValue();
			printk(KERN_INFO \
				"Start Physical Address : 0x%x\n", phy_address);
			printk(KERN_INFO \
				"Read Length : 0x%x\n", len);
			vir_address = __phys_to_virt(phy_address);
			for (; start < len; start++) {
				if (start % 8 == 0)
					printk(KERN_INFO "\n%02x:",
					(vir_address + start * 4) & 0xff);
				ch = readl((vir_address + start * 4));
				printk("%08x   ", ch);
			}
		} else if (mode == 1) {
			printk(KERN_INFO \
				"Virtual Address Read:\n");
			vir_address = SysrqGetValue();
			len = SysrqGetValue();
			printk(KERN_INFO \
				"Read Length : 0x%x\n", len);
			for (; start < len; start++) {
				if (start % 8 == 0)
					printk(KERN_INFO "\n%02x:",
					(vir_address + start * 4) & 0xff);
				ch = readl((vir_address + start * 4));
				printk("%08x  ", ch);
			}
		} else {
			printk(KERN_INFO "Select Wrong Mode!\n");
			return;
		}
	} else {
		printk(KERN_INFO "Select Wrong Direction!\n");
		return;
	}

}

static void fh_show_syslog(char *log_buf, int len)
{
	for (; len; len--) {
		if (*log_buf == 0x0a) {
			fh_serial_console_putchar(&fh_own_ports[0].port, '\r');
			fh_serial_console_putchar(&fh_own_ports[0].port, '\n');
			log_buf++;
		}
		fh_serial_console_putchar(&fh_own_ports[0].port, *log_buf);
		log_buf++;
	}
}


static void sysrq_handle_fh_dmesg(int key)
{
	int len;
	char *log_buf = NULL;

	/* Just Dummy */
	len =  16 * 1024;
	len = do_syslog(SYSLOG_ACTION_SIZE_UNREAD, \
				log_buf, len, SYSLOG_FROM_CALL);
	log_buf = kmalloc(len, GFP_KERNEL);
	if (log_buf == NULL) {
		pr_info("Get Syslog Buffer failed!\n");
		return;
	}
	memset((void *)log_buf, 0x0, len);
	do_syslog(SYSLOG_ACTION_READ, log_buf, len, SYSLOG_FROM_CALL);
	fh_show_syslog(log_buf, len);
	kfree(log_buf);
}


static struct sysrq_key_op sysrq_fh_debug_mem_op = {
	.handler     = sysrq_handle_fh_mem,
	.help_msg    = "Fullhan Debug Dump Mem(G)",
	.action_msg  = "Fullhan Dump Mem(w or r)",
	.enable_mask = SYSRQ_ENABLE_DUMP,
};

static struct sysrq_key_op sysrq_fh_dmesg_op = {
	.handler     = sysrq_handle_fh_dmesg,
	.help_msg    = "Fullhan DMSG(x)",
	.action_msg  = "Fullhan DMSG",
	.enable_mask = SYSRQ_ENABLE_DUMP,
};


#endif

int fh_serial_probe(struct platform_device *dev)
{
	int ret = 0;
	struct fh_uart_port *myown_port;
	int filter_no;
	dma_cap_mask_t mask;
	struct clk *uart_clk;
	char clk_name[16];

	if (dev->id > (sizeof(fh_own_ports)/sizeof(struct fh_uart_port)))
		goto probe_err;

	sprintf(clk_name, "uart%d_clk", dev->id);
	uart_clk = clk_get(NULL, clk_name);
	if (IS_ERR(uart_clk))
		return PTR_ERR(uart_clk);
	ret = clk_enable(uart_clk);
	if (ret)
		goto probe_err;

	myown_port = &fh_own_ports[dev->id];
	if (myown_port->fh_info.use_dma) {
		filter_no = myown_port->uart_dma.tx_dma_channel;
		dma_cap_zero(mask);
		dma_cap_set(DMA_SLAVE, mask);
		myown_port->uart_dma.dma_tx.chan = dma_request_channel(mask,
			fh_uart_dma_chan_filter, &filter_no);
		if (!myown_port->uart_dma.dma_tx.chan)
			goto probe_err;
		filter_no = myown_port->uart_dma.rx_dma_channel;
		dma_cap_zero(mask);
		dma_cap_set(DMA_SLAVE, mask);
		myown_port->uart_dma.dma_rx.chan = dma_request_channel(mask,
			fh_uart_dma_chan_filter, &filter_no);
		if (!myown_port->uart_dma.dma_rx.chan) {
			dma_release_channel(myown_port->uart_dma.dma_tx.chan);
			myown_port->fh_info.use_dma = 0;
		goto probe_err;
		}
		myown_port->uart_dma.tx_dma_add = dma_map_single(
			myown_port->uart_dma.dma_tx.chan->dev->device.parent,
			(void *) myown_port->uart_dma.tx_dumy_buff,
			sizeof(myown_port->uart_dma.tx_dumy_buff),
			DMA_TO_DEVICE);
		myown_port->uart_dma.rx_dma_add = dma_map_single(
			myown_port->uart_dma.dma_rx.chan->dev->device.parent,
			(void *) myown_port->uart_dma.rx_dumy_buff,
			sizeof(myown_port->uart_dma.rx_dumy_buff),
			DMA_TO_DEVICE);
		myown_port->uart_dma.HasInit   = 0;
		myown_port->uart_dma.tx_done   = 1;
		myown_port->uart_dma.tx_count  = 0;
		myown_port->uart_dma.rx_wr_ptr = 0;
		myown_port->uart_dma.rx_rd_ptr = 0;
	}

	ret = fh_serial_init_port(myown_port, dev);
	if (ret < 0)
		goto probe_err;

	ret = uart_add_one_port(&fh_uart_drv, &myown_port->port);
	if (ret != 0)
		printk(KERN_ERR "%s: failed to add one port.\n", __func__);

	platform_set_drvdata(dev, &myown_port->port);
	ret = device_create_file(&dev->dev, &dev_attr_clock_source);

	if (ret < 0)
		printk(KERN_ERR "%s: failed to add clksrc attr.\n", __func__);

	ret = fh_serial_cpufreq_register(myown_port);
	if (ret < 0)
		dev_err(&dev->dev, "failed to add cpufreq notifier\n");
#ifdef CONFIG_MAGIC_SYSRQ
	register_sysrq_key('g', &sysrq_fh_debug_mem_op);
	register_sysrq_key('x', &sysrq_fh_dmesg_op);
#endif
	printk(KERN_DEBUG "fh serial probe done\n");
	return 0;

 probe_err:
	printk(KERN_ERR "%s: fh serial probe error.\n", __func__);
	return ret;
}
EXPORT_SYMBOL_GPL(fh_serial_probe);

static inline void fh_serial_cpufreq_deregister(struct fh_uart_port* myown_port)
{

}


int __devexit fh_serial_remove(struct platform_device *dev)
{
	struct uart_port *port = fh_dev_to_port(&dev->dev);
	struct fh_uart_port *myown_port = to_fh_uart_port(port);
	struct device *dev_rx =
		myown_port->uart_dma.dma_tx.chan->dev->device.parent;
	struct device *dev_tx =
		myown_port->uart_dma.dma_rx.chan->dev->device.parent;

	if (port) {
		if (myown_port->fh_info.use_dma) {
			dma_release_channel(myown_port->uart_dma.dma_tx.chan);
			dma_release_channel(myown_port->uart_dma.dma_rx.chan);
			dma_unmap_single(dev_tx,
				(dma_addr_t)myown_port->uart_dma.tx_dumy_buff,
				sizeof(myown_port->uart_dma.tx_dumy_buff),
				DMA_TO_DEVICE);
			dma_unmap_single(dev_rx,
				(dma_addr_t)myown_port->uart_dma.rx_dumy_buff,
				sizeof(myown_port->uart_dma.rx_dumy_buff),
				DMA_TO_DEVICE);
		}
		fh_serial_cpufreq_deregister(myown_port);
		device_remove_file(&dev->dev, &dev_attr_clock_source);
		uart_remove_one_port(&fh_uart_drv, port);
	}

	return 0;
}
EXPORT_SYMBOL_GPL(fh_serial_remove);

int fh_serial_init(struct platform_driver *drv)
{
	return platform_driver_register(drv);
}
EXPORT_SYMBOL_GPL(fh_serial_init);

static int __init fh_serial_modinit(void)
{
	int ret;
	ret = uart_register_driver(&fh_uart_drv);
	if (ret < 0) {
		printk(KERN_ERR "failed to register UART driver\n");
		return -1;
	}
	return 0;
}

static void __exit fh_serial_modexit(void)
{
	uart_unregister_driver(&fh_uart_drv);
}


static int _fh_serial_probe(struct platform_device *dev)
{
	return fh_serial_probe(dev);
}

static struct platform_driver fh_serial_driver = {
	.probe		= _fh_serial_probe,
	.remove		= __devexit_p(fh_serial_remove),
#ifdef CONFIG_PM
	.suspend	= fh_serial_suspend,
	.resume		= fh_serial_resume,
#endif
	.driver		= {
		.name	= FH_SERIAL_NAME,
		.owner	= THIS_MODULE,
	},
};

static int __init _fh_serial_init(void)
{
	return fh_serial_init(&fh_serial_driver);
}

static void __exit _fh_serial_exit(void)
{
	platform_driver_unregister(&fh_serial_driver);
}

module_init(_fh_serial_init);
module_exit(_fh_serial_exit);

module_init(fh_serial_modinit);
module_exit(fh_serial_modexit);
