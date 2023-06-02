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
				__FILE__, __FUNCTION__, __LINE__)
#define FH_SERIAL_NAME					"ttyS"
#define FH_DRIVE_NAME					"ttyS"
#define FH_DEV_NAME						"ttyS"


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
#define UART_PORT1_ISR      ISR_NUMBER1
#define UART_PORT2_ISR      ISR_NUMBER2
#define UART_PORT0_BASE     UART0_REG_BASE
#define UART_PORT1_BASE     UART1_REG_BASE
#define UART_PORT2_BASE     UART2_REG_BASE
#define UART_PORT0(x)       VUART0(x)
#define UART_PORT1(x)       VUART1(x)
#define UART_PORT2(x)       VUART2(x)
#endif

static  struct fh_uart_port  fh_own_ports[FH_UART_NUMBER] = {
	[0] = {
		.port = {
			.lock		= __SPIN_LOCK_UNLOCKED(fh_own_ports[0].port.lock),
			.iotype		= UPIO_MEM,
			.irq		= UART_PORT0_ISR,
			.uartclk	= 30000000,
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
	[1] = {
		.port = {
			.lock		= __SPIN_LOCK_UNLOCKED(fh_own_ports[1].port.lock),
			.iotype		= UPIO_MEM,
			.irq		= UART_PORT1_ISR,
			.uartclk	= 30000000,
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
			.fifo_ctrl = UART_INT_RXFIFO_DEPTH_QUARTER,
		}
	},
#ifdef CONFIG_ARCH_FH8830
	[2] = {
		.port = {
			.lock		= __SPIN_LOCK_UNLOCKED(fh_own_ports[2].port.lock),
			.iotype		= UPIO_MEM,
			.irq		= UART_PORT2_ISR,
			.uartclk	= 30000000,
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
	if (test_init_status != 0) {
		return test_init_status;
	}
	Uart_Disable_Irq(desc, UART_INT_ALL);
	fh_uart_start_rx(&port->port);
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
	while(!(ret & UART_STATUS_TFNF));

	fh_uart_writel(myown_port->fh_info.base_add + REG_UART_THR,ch);
}

static void
fh_serial_console_write(struct console *co, const char *s,
			     unsigned int count)
{
	struct fh_uart_port* myown_port  = &fh_own_ports[co->index];
	uart_console_write(&myown_port->port, s, count, fh_serial_console_putchar);
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
	if (co->index == -1 || co->index >= FH_UART_NUMBER)
	    printk("ERROR: co->index invaild: %d\n", co->index);

	port = &fh_own_ports[co->index].port;

	/* is the port configured? */
	if (port->mapbase == 0x0)
	    printk("ERROR: port->mapbase == 0x0\n");

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
	struct fh_uart_port* myown_port = to_fh_uart_port(port);
	u32 base = (u32)myown_port->fh_info.base_add;
	tx_enabled(port) = 0;
	fh_uart_writel(base + REG_UART_IER, 0x01);
}

static void fh_uart_start_tx(struct uart_port *port)
{
	/* open tx isr */
	struct fh_uart_port* myown_port = to_fh_uart_port(port);
	u32 base = (u32)myown_port->fh_info.base_add;
	fh_uart_writel(base + REG_UART_IER, 0x03);
	tx_enabled(port) = 1;
}

static void fh_uart_stop_rx(struct uart_port *port)
{
	struct fh_uart_port* myown_port = to_fh_uart_port(port);
	rx_enabled(port) = 0;
	Uart_Disable_Irq(&myown_port->fh_info,UART_INT_ERBFI_POS);
}

static void fh_uart_start_rx(struct uart_port *port)
{
	struct fh_uart_port* myown_port = to_fh_uart_port(port);
	rx_enabled(port) = 1;
	Uart_Enable_Irq(&myown_port->fh_info,UART_INT_ERBFI_POS);
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
	struct fh_uart_port* myown_port = to_fh_uart_port(port);
	/*
	 * 1 means empty
	 * 0:means no empty
	 */
	int ret = 1;
	int ret_status;

	ret_status = (u8)Uart_Read_Control_Status(&myown_port->fh_info);
	if(ret_status & UART_STATUS_TFE)
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
	struct fh_uart_port* myown_port  = dev_id;
	struct uart_port *port = &myown_port->port;
	struct tty_struct *tty = port->state->port.tty;
	unsigned int ch;
	unsigned int flag;
	unsigned int uerstat;
	int max_count = 64;
	int ret_status;

	while (max_count-- > 0) {
		/* check if rx fifo is empty */
		ret_status = (u8)Uart_Read_Control_Status(&myown_port->fh_info);
		if(!(ret_status & UART_STATUS_RFNE))
			break;
		/* read error in the rx process */
		uerstat = Uart_Read_Line_Status(&myown_port->fh_info);
		/* read  data in the rxfifo */
		if(uerstat & UART_LINE_STATUS_DR)
			ch = fh_uart_readl(myown_port->fh_info.base_add + REG_UART_RBR);
		/* insert the character into the buffer */
		flag = TTY_NORMAL;
		port->icount.rx++;
		/* if at least one error in rx process */
		if (unlikely(uerstat & UART_LINE_STATUS_RFE)) {
			printk("rxerr: port ch=0x%02x, rxs=0x%08x\n",
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
		fh_uart_writel(myown_port->fh_info.base_add + REG_UART_THR,xmit->buf[xmit->tail]);
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
fh_uart_isr(int irq, void *dev_id){
	irqreturn_t ret_isr;
	struct fh_uart_port* myown_port  = dev_id;
	int ret_iir;

	/* check if the tx empty isr */
	ret_iir = Uart_Read_iir(&myown_port->fh_info);
	if (ret_iir == 0x06){
		printk("uart overrun\n");
	}

	if((ret_iir & 0x04)||(ret_iir & 0x0c)){
		ret_isr = fh_uart_rx_chars(irq,dev_id);
	}

	if(ret_iir & 0x02){
		ret_isr = fh_uart_tx_chars(irq,dev_id);
	}
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
	if ((ret = request_irq(myown_port->fh_info.irq_num, fh_uart_isr, 0, FH_DEV_NAME, (void*)myown_port)))
	{
		printk("cannot get irq %d\n", myown_port->fh_info.irq_num);
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

	if (termios->c_cflag & PARENB){
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

	myown_port->port.read_status_mask = UART_LSR_OE | UART_LSR_THRE | UART_LSR_DR;
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

static int fh_serial_init_port(struct fh_uart_port* myown_port,

				    struct platform_device *platdev)
{
	struct uart_port *port = &myown_port->port;
	struct resource *res;

	if(platdev == NULL)
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

	myown_port->fh_info.baudrate = BAUDRATE_115200;
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

int fh_serial_probe(struct platform_device *dev)
{
	int ret = 0;
	struct fh_uart_port* myown_port;
	if(dev->id > (sizeof(fh_own_ports)/sizeof(struct fh_uart_port)))
		goto probe_err;

	myown_port = &fh_own_ports[dev->id];
	ret = fh_serial_init_port(myown_port,dev);
	if (ret < 0)
		goto probe_err;

	ret = uart_add_one_port(&fh_uart_drv, &myown_port->port);
	if(ret != 0)
		printk(KERN_ERR "%s: failed to add one port.\n", __func__);

	platform_set_drvdata(dev, &myown_port->port);
	ret = device_create_file(&dev->dev, &dev_attr_clock_source);

	if (ret < 0)
		printk(KERN_ERR "%s: failed to add clksrc attr.\n", __func__);

	ret = fh_serial_cpufreq_register(myown_port);
	if (ret < 0)
		dev_err(&dev->dev, "failed to add cpufreq notifier\n");

	printk(KERN_DEBUG "fh serial probe done\n");
	return 0;

 probe_err:
	printk(KERN_ERR "%s: fh serial probe error.\n",__func__);
	return ret;
}

EXPORT_SYMBOL_GPL(fh_serial_probe);

static inline void fh_serial_cpufreq_deregister(struct fh_uart_port* myown_port)
{

}


int __devexit fh_serial_remove(struct platform_device *dev)
{
	struct uart_port *port = fh_dev_to_port(&dev->dev);
	struct fh_uart_port* myown_port = to_fh_uart_port(port);

	if (port) {
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
