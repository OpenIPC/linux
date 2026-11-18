#include <linux/delay.h>
#include <linux/device.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/serial_8250.h>
#include <linux/serial_reg.h>
#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/of_platform.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/acpi.h>
#include <linux/reset.h>
#include <linux/pm_runtime.h>

#include <asm/byteorder.h>

#include "8250_axlib.h"

#define PERIPH_GLOBAL_BASE		0x4870000
#define PERIPH_GLOBAL_LEN		0x10000
#define PERIPH_UART_MODE_OFFSET		0x38
#define PERIPH_UART_RS485_ENABLED	0x3

#define AX_RS485_ENABLED		0x17
#define AX_RS485_DISABLED		0x0
#define DE_TO_RE_AND_RE_TO_DE		0x30003
#define DE_ENABLED			1
#define RE_ENABLED			1

#define TCR_OFFSET			0xac
#define DE_EN_OFFSET			0xb0
#define RE_EN_OFFSET			0xb4
#define TAT_OFFSET			0xbc

/* Offsets for the DesignWare specific registers */
#define AX_UART_USR	0x1f /* UART Status Register */

/* DesignWare specific register fields */
#define AX_UART_MCR_SIRE		BIT(6)

#define PERI_SYS_BASE			0x4870000
#define PERI_SYS_BASE_LEN		0x100
#define CLK_MUX_0_SET			0xA8	/*clk source set,bit17-18,00 24m,01,50m,10 156m,11 208m*/
#define CLK_MUX_0_CLR			0xAC
#define CLK_SOURCE_BIT			GENMASK(18, 17)
#define CLK_EB_2_SET			0xC0	/*clk set,bit5 - 10*/
#define CLK_EB_2_CLR			0xC4
#define CLK_BIT(x)			BIT(5 + x)
#define CLK_EB_3_SET			0xC8	/*pclk set,bit13 - 18*/
#define CLK_EB_3_CLR			0xCC
#define PCLK_BIT(x)			BIT(13 + x)

void __iomem *uart_clk_reg;
static int source_set_flag;
struct ax8250_data {
	struct ax8250_port_data	data;

	u8			usr_reg;
	int			msr_mask_on;
	int			msr_mask_off;
	struct notifier_block	clk_notifier;
	struct work_struct	clk_work;
	struct reset_control	*rst;
	struct reset_control	*prst;
	unsigned int		skip_autocfg:1;
	unsigned int		uart_16550_compatible:1;
	int			uart_clk_id;
};

static inline struct ax8250_data *to_ax8250_data(struct ax8250_port_data *data)
{
	return container_of(data, struct ax8250_data, data);
}

static inline struct ax8250_data *clk_to_ax8250_data(struct notifier_block *nb)
{
	return container_of(nb, struct ax8250_data, clk_notifier);
}

static inline struct ax8250_data *work_to_ax8250_data(struct work_struct *work)
{
	return container_of(work, struct ax8250_data, clk_work);
}

static inline int ax8250_modify_msr(struct uart_port *p, int offset, int value)
{
	struct ax8250_data *d = to_ax8250_data(p->private_data);

	/* Override any modem control signals if needed */
	if (offset == UART_MSR) {
		value |= d->msr_mask_on;
		value &= ~d->msr_mask_off;
	}

	return value;
}

static void ax8250_force_idle(struct uart_port *p)
{
	struct uart_8250_port *up = up_to_u8250p(p);

	serial8250_clear_and_reinit_fifos(up);
	(void)p->serial_in(p, UART_RX);
}

static void ax8250_check_lcr(struct uart_port *p, int value)
{
	void __iomem *offset = p->membase + (UART_LCR << p->regshift);
	int tries = 1000;

	/* Make sure LCR write wasn't ignored */
	while (tries--) {
		unsigned int lcr = p->serial_in(p, UART_LCR);

		if ((value & ~UART_LCR_SPAR) == (lcr & ~UART_LCR_SPAR))
			return;

		ax8250_force_idle(p);

#ifdef CONFIG_64BIT
		if (p->type == PORT_OCTEON)
			__raw_writeq(value & 0xff, offset);
		else
#endif
		if (p->iotype == UPIO_MEM32)
			writel(value, offset);
		else if (p->iotype == UPIO_MEM32BE)
			iowrite32be(value, offset);
		else
			writeb(value, offset);
	}
	/*
	 * FIXME: this deadlocks if port->lock is already held
	 * dev_err(p->dev, "Couldn't set LCR to %d\n", value);
	 */
}

static void ax8250_serial_out(struct uart_port *p, int offset, int value)
{
	struct ax8250_data *d = to_ax8250_data(p->private_data);

	writeb(value, p->membase + (offset << p->regshift));

	if (offset == UART_LCR && !d->uart_16550_compatible)
		ax8250_check_lcr(p, value);
}

static unsigned int ax8250_serial_in(struct uart_port *p, int offset)
{
	unsigned int value = readb(p->membase + (offset << p->regshift));

	return ax8250_modify_msr(p, offset, value);
}

static void ax8250_serial_out32(struct uart_port *p, int offset, int value)
{
	struct ax8250_data *d = to_ax8250_data(p->private_data);

	writel(value, p->membase + (offset << p->regshift));

	if (offset == UART_LCR && !d->uart_16550_compatible)
		ax8250_check_lcr(p, value);
}

static unsigned int ax8250_serial_in32(struct uart_port *p, int offset)
{
	unsigned int value = readl(p->membase + (offset << p->regshift));

	return ax8250_modify_msr(p, offset, value);
}

static int ax8250_handle_irq(struct uart_port *p)
{
	struct uart_8250_port *up = up_to_u8250p(p);
	struct ax8250_data *d = to_ax8250_data(p->private_data);
	unsigned int iir = p->serial_in(p, UART_IIR);
	unsigned int status;
	unsigned long flags;

	/*
	 * There are ways to get Designware-based UARTs into a state where
	 * they are asserting UART_IIR_RX_TIMEOUT but there is no actual
	 * data available.  If we see such a case then we'll do a bogus
	 * read.  If we don't do this then the "RX TIMEOUT" interrupt will
	 * fire forever.
	 *
	 * This problem has only been observed so far when not in DMA mode
	 * so we limit the workaround only to non-DMA mode.
	 */
	if (!up->dma && ((iir & 0x3f) == UART_IIR_RX_TIMEOUT)) {
		spin_lock_irqsave(&p->lock, flags);
		status = p->serial_in(p, UART_LSR);

		if (!(status & (UART_LSR_DR | UART_LSR_BI)))
			(void) p->serial_in(p, UART_RX);

		spin_unlock_irqrestore(&p->lock, flags);
	}

	if (serial8250_handle_irq(p, iir))
		return 1;

	if ((iir & UART_IIR_BUSY) == UART_IIR_BUSY) {
		/* Clear the USR */
		(void)p->serial_in(p, d->usr_reg);

		return 1;
	}

	return 0;
}

static void
ax8250_do_pm(struct uart_port *port, unsigned int state, unsigned int old)
{
	if (!state)
		pm_runtime_get_sync(port->dev);

	serial8250_do_pm(port, state, old);

	if (state)
		pm_runtime_put_sync_suspend(port->dev);
}

static void ax8250_set_termios(struct uart_port *p, struct ktermios *termios,
			       struct ktermios *old)
{
	p->status &= ~UPSTAT_AUTOCTS;
	if (termios->c_cflag & CRTSCTS)
		p->status |= UPSTAT_AUTOCTS;

	serial8250_do_set_termios(p, termios, old);
}

static void ax8250_set_ldisc(struct uart_port *p, struct ktermios *termios)
{
	struct uart_8250_port *up = up_to_u8250p(p);
	unsigned int mcr = p->serial_in(p, UART_MCR);

	if (up->capabilities & UART_CAP_IRDA) {
		if (termios->c_line == N_IRDA)
			mcr |= AX_UART_MCR_SIRE;
		else
			mcr &= ~AX_UART_MCR_SIRE;

		p->serial_out(p, UART_MCR, mcr);
	}
	serial8250_do_set_ldisc(p, termios);
}

/*
 * ax8250_fallback_dma_filter will prevent the UART from getting just any free
 * channel on platforms that have DMA engines, but don't have any channels
 * assigned to the UART.
 *
 * REVISIT: This is a work around for limitation in the DMA Engine API. Once the
 * core problem is fixed, this function is no longer needed.
 */
static bool ax8250_fallback_dma_filter(struct dma_chan *chan, void *param)
{
	return false;
}

static void ax8250_quirks(struct uart_port *p, struct ax8250_data *data)
{
	if (p->dev->of_node) {
		struct device_node *np = p->dev->of_node;
		int id;

		/* get index of serial line, if found in DT aliases */
		id = of_alias_get_id(np, "serial");
		if (id >= 0)
			p->line = id;
	}
}

static int ax8250_rs485_config(struct uart_port *port,
				struct serial_rs485 *rs485)
{
	bool is_rs485 = !!(rs485->flags & SER_RS485_ENABLED);
	void __iomem *p = port->membase;

	if (is_rs485)
		writel(AX_RS485_ENABLED, p + TCR_OFFSET);
	else
		writel(AX_RS485_DISABLED, p + TCR_OFFSET);


	if (is_rs485){
		writel(DE_TO_RE_AND_RE_TO_DE, p + TAT_OFFSET);

		writel(DE_ENABLED, p + DE_EN_OFFSET);
		writel(RE_ENABLED, p + RE_EN_OFFSET);
	}
	memcpy(&port->rs485 , rs485, sizeof(struct serial_rs485));

	return 0;
}

static void ax_uart_clk(int ax_clk_id, bool on)
{
	if (on) {
		writel(PCLK_BIT(ax_clk_id), uart_clk_reg + CLK_EB_3_SET);
		writel(CLK_BIT(ax_clk_id), uart_clk_reg + CLK_EB_2_SET);
	} else {
		writel(PCLK_BIT(ax_clk_id), uart_clk_reg + CLK_EB_3_CLR);
		writel(CLK_BIT(ax_clk_id), uart_clk_reg + CLK_EB_2_CLR);
	}
}

static int ax8250_probe(struct platform_device *pdev)
{
	struct uart_8250_port uart = {}, *up = &uart;
	struct resource *regs = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	struct uart_port *p = &up->port;
	struct device *dev = &pdev->dev;
	struct ax8250_data *data;
	void __iomem *reg;
	int irq;
	int err, clk_id;
	u32 val, rs485_config_val, value;

	err = device_property_read_u32(dev, "ax_rs485_config", &rs485_config_val);
	if (!err) {
		rs485_config_val = (((rs485_config_val + 1) * 2) - 2);
		reg = ioremap(PERIPH_GLOBAL_BASE, PERIPH_GLOBAL_LEN);
		value = readl(reg + PERIPH_UART_MODE_OFFSET);
		value |= (PERIPH_UART_RS485_ENABLED << rs485_config_val);
		writel(value, reg + PERIPH_UART_MODE_OFFSET);
		iounmap(reg);
	}

	if (!regs) {
		dev_err(dev, "no registers defined\n");
		return -EINVAL;
	}

	irq = platform_get_irq(pdev, 0);
	if (irq < 0)
		return irq;

	spin_lock_init(&p->lock);
	p->mapbase	= regs->start;
	p->irq		= irq;
	p->handle_irq	= ax8250_handle_irq;
	p->pm		= ax8250_do_pm;
	p->type		= PORT_8250;
	p->flags	= UPF_SHARE_IRQ | UPF_FIXED_PORT;
	p->dev		= dev;
	p->iotype	= UPIO_MEM;
	p->serial_in	= ax8250_serial_in;
	p->serial_out	= ax8250_serial_out;
	p->set_ldisc	= ax8250_set_ldisc;
	p->rs485_config = ax8250_rs485_config,
	p->set_termios	= ax8250_set_termios;

	p->membase = devm_ioremap(dev, regs->start, resource_size(regs));
	if (!p->membase)
		return -ENOMEM;

	data = devm_kzalloc(dev, sizeof(*data), GFP_KERNEL);
	if (!data)
		return -ENOMEM;

	data->data.dma.fn = ax8250_fallback_dma_filter;
	data->usr_reg = AX_UART_USR;
	p->private_data = &data->data;

	data->uart_16550_compatible = device_property_read_bool(dev,
						"axera,uart-16550-compatible");

	err = device_property_read_u32(dev, "reg-shift", &val);
	if (!err)
		p->regshift = val;

	err = device_property_read_u32(dev, "reg-io-width", &val);
	if (!err && val == 4) {
		p->iotype = UPIO_MEM32;
		p->serial_in = ax8250_serial_in32;
		p->serial_out = ax8250_serial_out32;
	}

	/* Always ask for fixed clock rate from a property. */
	device_property_read_u32(dev, "clock-frequency", &p->uartclk);

	/* Optional interface clock */
	device_property_read_u32(&pdev->dev, "ax_clk_id", &clk_id);
	data->uart_clk_id = clk_id;
	if (source_set_flag == 0) {
		uart_clk_reg = ioremap(PERI_SYS_BASE, PERI_SYS_BASE_LEN);
		writel(CLK_SOURCE_BIT, uart_clk_reg + CLK_MUX_0_SET);
		source_set_flag = 1;
	}
	/* If no clock rate is defined, fail. */
	if (!p->uartclk) {
		dev_err(dev, "clock rate not defined\n");
		err = -EINVAL;
		return -1;
	}

	data->rst = devm_reset_control_get_optional_exclusive(dev, NULL);
	if (IS_ERR(data->rst)) {
		err = PTR_ERR(data->rst);
		return -1;
	}
	reset_control_deassert(data->rst);

	data->prst = devm_reset_control_get_optional_exclusive(dev, "preset");
	if (IS_ERR(data->prst)) {
		err = PTR_ERR(data->prst);
		goto err_reset;
	}
	reset_control_deassert(data->prst);
	ax_uart_clk(clk_id, true);
	ax8250_quirks(p, data);

	/* If the Busy Functionality is not implemented, don't handle it */
	if (data->uart_16550_compatible)
		p->handle_irq = NULL;

	if (!data->skip_autocfg)
		ax8250_setup_port(p);

	/* If we have a valid fifosize, try hooking up DMA */
	if (p->fifosize) {
		data->data.dma.rxconf.src_maxburst = 16;
		data->data.dma.txconf.dst_maxburst = 16;
		up->dma = &data->data.dma;
	}

	data->data.line = serial8250_register_8250_port(up);
	if (data->data.line < 0) {
		err = data->data.line;
		goto err_preset;
	}

	platform_set_drvdata(pdev, data);

	pm_runtime_set_active(dev);
	pm_runtime_enable(dev);

	return 0;

err_preset:
	reset_control_assert(data->prst);

err_reset:
	reset_control_assert(data->rst);

	return err;
}

static int ax8250_remove(struct platform_device *pdev)
{
	struct ax8250_data *data = platform_get_drvdata(pdev);
	struct device *dev = &pdev->dev;

	pm_runtime_get_sync(dev);


	serial8250_unregister_port(data->data.line);

	reset_control_assert(data->prst);

	reset_control_assert(data->rst);

	ax_uart_clk(data->uart_clk_id, false);

	pm_runtime_disable(dev);
	pm_runtime_put_noidle(dev);

	return 0;
}

#ifdef CONFIG_PM_SLEEP
static int ax8250_suspend(struct device *dev)
{
	struct ax8250_data *data = dev_get_drvdata(dev);

	serial8250_suspend_port(data->data.line);

	return 0;
}

static int ax8250_resume(struct device *dev)
{
	struct ax8250_data *data = dev_get_drvdata(dev);

	serial8250_resume_port(data->data.line);

	return 0;
}
#endif /* CONFIG_PM_SLEEP */

#ifdef CONFIG_PM
static int ax8250_runtime_suspend(struct device *dev)
{
	struct ax8250_data *data = dev_get_drvdata(dev);

	ax_uart_clk(data->uart_clk_id, false);

	return 0;
}

static int ax8250_runtime_resume(struct device *dev)
{
	struct ax8250_data *data = dev_get_drvdata(dev);

	ax_uart_clk(data->uart_clk_id, true);

	return 0;
}
#endif

static const struct dev_pm_ops ax8250_pm_ops = {
	SET_SYSTEM_SLEEP_PM_OPS(ax8250_suspend, ax8250_resume)
	SET_RUNTIME_PM_OPS(ax8250_runtime_suspend, ax8250_runtime_resume, NULL)
};

static const struct of_device_id ax8250_of_match[] = {
	{ .compatible = "axera,ax-apb-uart" },
	{ /* Sentinel */ }
};
MODULE_DEVICE_TABLE(of, ax8250_of_match);

static struct platform_driver ax8250_platform_driver = {
	.driver = {
		.name		= "ax-apb-uart",
		.pm		= &ax8250_pm_ops,
		.of_match_table	= ax8250_of_match,
	},
	.probe			= ax8250_probe,
	.remove			= ax8250_remove,
};

module_platform_driver(ax8250_platform_driver);

MODULE_AUTHOR("Axera");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Axera 8250 serial port driver");
MODULE_ALIAS("platform:ax-apb-uart");
