/*
 * Faraday FTIIC010 I2C Controller
 *
 * (C) Copyright 2010 Faraday Technology
 * Po-Yu Chuang <ratbert@faraday-tech.com>
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
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/platform_device.h>
#include <linux/i2c.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/io.h>
#include <linux/version.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/uaccess.h>
#include <linux/ratelimit.h>
#include <linux/clk.h>
#include "i2c-ftiic010.h"

#if defined(CONFIG_PLATFORM_NA51039)
#include <mach/nvt_pcie.h>
#define MAX_CHIPS   4
#endif

#ifdef CONFIG_OF
#include <linux/of_irq.h>
#endif

#define NVT_I2C_VERSION     "0.0.1"

#define MAX_RETRY	1000
#define SCL_SPEED	(100 * 1000)
#define TIMEOUT     (HZ/10)	/* 100 ms */

#define GSR	    0xF
#define TSR	    0x27

struct ftiic010 {
	struct resource *res;
	void __iomem *base;
	struct clk          *clk;
	int irq;
#if defined(CONFIG_PLATFORM_NA51039)
	int ep_no;
#endif
	struct i2c_adapter adapter;
	int ack;
	int	nack;	//debug only
	wait_queue_head_t waitq;
	/* Default I2C bus speed in HZ */
	int default_bus_speed;
	/* Semaphore lock for dynamically adjusting the bus speed */
	struct semaphore sem_lock;
#ifdef CONFIG_I2C_INTERRUPT_MODE
	/* Keep record if TX/RX error occurs */
	int err;
#endif
	int	hdmi_i2c;	/* Is a HDMI device? */
};

/* Keep a record of all I2C platform device instances */
#if defined(CONFIG_PLATFORM_NA51039)
static struct platform_device *i2c_pdev_array[I2C_FTI2C010_COUNT*MAX_CHIPS] = {
	[0 ... (I2C_FTI2C010_COUNT*MAX_CHIPS - 1)] = NULL
};
#else
static struct platform_device *i2c_pdev_array[I2C_FTI2C010_COUNT] = {
	[0 ... (I2C_FTI2C010_COUNT - 1)] = NULL
};
#endif

#ifdef CONFIG_OF
/* Count the I2C host number descripted in device tree script */
static int i2c_of_probe_count;
#endif

/* Add a proc node for adjusting default bus speed */
static struct proc_dir_entry *ftiic010_proc_root;
static struct proc_dir_entry *ftiic010_proc_bus_speed;

/******************************************************************************
 * internal functions
 *****************************************************************************/
static void ftiic010_reset(struct ftiic010 *ftiic010)
{
	int cr = FTIIC010_CR_I2C_RST;

	iowrite32(cr, ftiic010->base + FTIIC010_OFFSET_CR);

	/* Wait until reset bit cleared by hw */
	while (ioread32(ftiic010->base + FTIIC010_OFFSET_CR) &
	       FTIIC010_CR_I2C_RST)
		;
}

void ftiic010_set_clock_speed(struct ftiic010 *ftiic010, int hz)
{
	int cdr, gsr;
	u32 clk_rate;

	if (unlikely(hz < 50 * 1000)) {
		dev_err(&ftiic010->adapter.dev,
			"Speed smaller than 50 KHz, set %d hz fail\n", hz);
		return;
	}

	if (unlikely(hz > 400 * 1000)) {
		dev_err(&ftiic010->adapter.dev,
			"Speed greater than 400 KHz, set %d hz fail\n", hz);
		return;
	}

	/* Read bit field [13:10] -> bit mask: 0x3C00 */
	gsr = (ioread32(ftiic010->base + FTIIC010_OFFSET_TGSR) & 0x3C00) >> 10;
	clk_rate = clk_get_rate(ftiic010->clk);
	cdr = (clk_rate / hz - gsr) / 2 - 2;
	cdr &= FTIIC010_CDR_MASK;

	dev_dbg(&ftiic010->adapter.dev, "  [CDR] = %08x\n", cdr);
	iowrite32(cdr, ftiic010->base + FTIIC010_OFFSET_CDR);
}
EXPORT_SYMBOL(ftiic010_set_clock_speed);

static void ftiic010_set_tgsr(struct ftiic010 *ftiic010, int tsr, int gsr)
{
	int tgsr;

	tgsr = FTIIC010_TGSR_TSR(tsr);
	tgsr |= FTIIC010_TGSR_GSR(gsr);

	dev_dbg(&ftiic010->adapter.dev, "  [TGSR] = %08x\n", tgsr);
	iowrite32(tgsr, ftiic010->base + FTIIC010_OFFSET_TGSR);
}

static void ftiic010_hw_init(struct ftiic010 *ftiic010)
{
	int cr, sda, i;

	/* Reset FTIIC010 first for arbitration lost */
	ftiic010_reset(ftiic010);
	ftiic010_set_tgsr(ftiic010, TSR, GSR);
	ftiic010_set_clock_speed(ftiic010, ftiic010->default_bus_speed);
	dev_dbg(&ftiic010->adapter.dev, "Hardware init!\n");
	/* Wait some time to make sure I2C host is ready */
	udelay(100);

	cr = ioread32(ftiic010->base + FTIIC010_OFFSET_CR);

	/* Device I2C hangs detection & recovery */
	for (i = 0; i < 9; i++) {
		sda =
		    ioread32(ftiic010->base +
			     FTIIC010_OFFSET_BMR) & FTIIC010_BMR_SDA_IN;
		if (sda == 0) {
			iowrite32(cr | FTIIC010_CR_SCL_LOW,
				  ftiic010->base + FTIIC010_OFFSET_CR);
			udelay(50);
			iowrite32((cr & ~(FTIIC010_CR_SCL_LOW)),
				  ftiic010->base + FTIIC010_OFFSET_CR);
			udelay(50);
		} else {
			break;
		}
	}

	if (i != 0)
		printk("Output extra %d SCL clocks to release device hang!\n", i);

	/* Issue a stop condition */
	iowrite32(cr | FTIIC010_CR_SCL_LOW,
		  ftiic010->base + FTIIC010_OFFSET_CR);
	udelay(10);
	iowrite32(cr | FTIIC010_CR_SDA_LOW,
		  ftiic010->base + FTIIC010_OFFSET_CR);
	udelay(10);
	iowrite32((cr & ~(FTIIC010_CR_SCL_LOW)),
		  ftiic010->base + FTIIC010_OFFSET_CR);
	udelay(10);
	iowrite32((cr & ~(FTIIC010_CR_SDA_LOW)),
		  ftiic010->base + FTIIC010_OFFSET_CR);
	dev_dbg(&ftiic010->adapter.dev, "Issue a stop condition!\n");

	/* Reset FTIIC010 again */
	ftiic010_reset(ftiic010);
	ftiic010_set_tgsr(ftiic010, TSR, GSR);
	ftiic010_set_clock_speed(ftiic010, ftiic010->default_bus_speed);
	/* Wait some time to make sure I2C host is ready */
	udelay(100);
}

int ftiic010_get_default_bus_speed(int bus_id)
{
	struct platform_device *pdev;
	struct ftiic010 *ftiic010;
	int hz;

#if defined(CONFIG_PLATFORM_NA51039)
	if ((bus_id >= (I2C_FTI2C010_COUNT*MAX_CHIPS)) || (i2c_pdev_array[bus_id] == NULL))
		return -1;
#else
	if ((bus_id >= I2C_FTI2C010_COUNT) || (i2c_pdev_array[bus_id] == NULL))
		return -1;
#endif

	pdev = i2c_pdev_array[bus_id];
	ftiic010 = platform_get_drvdata(pdev);

	down(&ftiic010->sem_lock);

	hz = ftiic010->default_bus_speed;

	up(&ftiic010->sem_lock);

	return hz;
}

int ftiic010_set_default_bus_speed(int bus_id, int hz)
{
	struct platform_device *pdev;
	struct ftiic010 *ftiic010;

#if defined(CONFIG_PLATFORM_NA51039)
	if ((bus_id >= (I2C_FTI2C010_COUNT*MAX_CHIPS)) || (i2c_pdev_array[bus_id] == NULL))
		return -1;
#else
	if ((bus_id >= I2C_FTI2C010_COUNT) || (i2c_pdev_array[bus_id] == NULL))
		return -1;
#endif

	/* Valid bus speed value: from 50 to 400 KHz */
	if ((hz < 50000) || (hz > 400000))
		return -1;

	pdev = i2c_pdev_array[bus_id];
	ftiic010 = platform_get_drvdata(pdev);

	down(&ftiic010->sem_lock);

	ftiic010->default_bus_speed = hz;
	ftiic010_hw_init(ftiic010);

	up(&ftiic010->sem_lock);

	return 0;
}

static inline void ftiic010_set_cr(struct ftiic010 *ftiic010, int start,
				   int stop, int nak)
{
	unsigned int cr;

	cr = FTIIC010_CR_I2C_EN | FTIIC010_CR_SCL_EN | FTIIC010_CR_TB_EN
#ifdef CONFIG_I2C_INTERRUPT_MODE
	    | FTIIC010_CR_DTI_EN | FTIIC010_CR_DRI_EN
#endif
	    | FTIIC010_CR_BERRI_EN | FTIIC010_CR_ALI_EN;

	if (start)
		cr |= FTIIC010_CR_START;

	if (stop)
		cr |= FTIIC010_CR_STOP;

	if (nak)
		cr |= FTIIC010_CR_NAK;

	iowrite32(cr, ftiic010->base + FTIIC010_OFFSET_CR);
}

static int ftiic010_tx_byte(struct ftiic010 *ftiic010, __u8 data, int start,
			    int stop)
{
#ifdef CONFIG_I2C_INTERRUPT_MODE
	int sr;
#else
	int i = 0;
	unsigned int status;
#endif

	iowrite32(data, ftiic010->base + FTIIC010_OFFSET_DR);

	ftiic010->ack = 0;
	ftiic010->nack = 0;
#ifdef CONFIG_I2C_INTERRUPT_MODE
	ftiic010->err = 0;
#endif
	ftiic010_set_cr(ftiic010, start, stop, 0);

#ifdef CONFIG_I2C_INTERRUPT_MODE
	wait_event_timeout(ftiic010->waitq, ftiic010->ack, TIMEOUT);

	if (unlikely(!ftiic010->ack)) {
		sr = ioread32(ftiic010->base + FTIIC010_OFFSET_SR);
		if ((sr & FTIIC010_SR_DT) && (ftiic010->err == 0)) {
			printk_ratelimited(KERN_ERR
					   "[I2C] CPU is too busy to process I2C TX interrupts!\n");
			#ifdef CONFIG_PLATFORM_NA51039
			///< Clear status, write 1 to clear
			iowrite32(sr, ftiic010->base + FTIIC010_OFFSET_SR);
			#endif

			return 0;
		}
		if (ftiic010->hdmi_i2c) {
			/* NAK is allowed for edid */
			return 0;
		}
		/* general i2c */
		dev_err(&ftiic010->adapter.dev, "I2C TX data 0x%x timeout! sr = 0x%x\n", data, sr);

		return -EIO;
	}

	return 0;
#else
	for (i = 0; i < MAX_RETRY; i++) {
		status = ioread32(ftiic010->base + FTIIC010_OFFSET_SR);
		if (status & FTIIC010_SR_DR) {
			///< Clear status, write 1 to clear
			iowrite32(status, ftiic010->base + FTIIC010_OFFSET_SR);
			return 0;
		}

		udelay(1);
	}
	return -EIO;
#endif
}

static int ftiic010_rx_byte(struct ftiic010 *ftiic010, int stop, int nak)
{
#ifdef CONFIG_I2C_INTERRUPT_MODE
	int sr;
#else
	int i = 0;
	unsigned int status;
#endif

	ftiic010->ack = 0;
	ftiic010_set_cr(ftiic010, 0, stop, nak);


#ifdef CONFIG_I2C_INTERRUPT_MODE
	wait_event_timeout(ftiic010->waitq, ftiic010->ack, TIMEOUT);

	if (unlikely(!ftiic010->ack)) {
		sr = ioread32(ftiic010->base + FTIIC010_OFFSET_SR);
		if (sr & FTIIC010_SR_DR && !(sr & FTIIC010_SR_AL)) {
			printk_ratelimited(KERN_ERR
					   "[I2C] CPU is too busy to process I2C RX interrupts!\n");
			#ifdef CONFIG_PLATFORM_NA51039
			///< Clear status, write 1 to clear
			iowrite32(sr, ftiic010->base + FTIIC010_OFFSET_SR);
			#endif

			return ioread32(ftiic010->base +
					FTIIC010_OFFSET_DR) & FTIIC010_DR_MASK;
		}
		dev_err(&ftiic010->adapter.dev, "I2C RX timeout!\n");
		return -EIO;
	}

	return ioread32(ftiic010->base + FTIIC010_OFFSET_DR) & FTIIC010_DR_MASK;
#else
	for (i = 0; i < MAX_RETRY; i++) {
		status = ioread32(ftiic010->base + FTIIC010_OFFSET_SR);
		if (status & FTIIC010_SR_DR) {
			///< Clear status, write 1 to clear
			iowrite32(status, ftiic010->base + FTIIC010_OFFSET_SR);

			return ioread32(ftiic010->base + FTIIC010_OFFSET_DR) & FTIIC010_DR_MASK;
		}
		udelay(1);
	}

	dev_err(&ftiic010->adapter.dev, "I2C: Failed to receive!\n");
	return -EIO;
#endif
}

static int ftiic010_tx_msg(struct ftiic010 *ftiic010,
			   struct i2c_msg *msg, int last)
{
	__u8 data;
	int i, ret, stop;

	data = (msg->addr & 0x7f) << 1 | 0;	/* write */
	ret = ftiic010_tx_byte(ftiic010, data, 1, 0);
	if (unlikely(ret < 0)) {
		pr_err("ftiic010_tx_msg addr 0x%.2x fail (%d)\r\n", (int)msg->addr, ret);
		return ret;
	}

	for (i = 0; i < msg->len; i++) {
		stop = 0;

		if (last && (i + 1 == msg->len))
			stop = 1;

		ret = ftiic010_tx_byte(ftiic010, msg->buf[i], 0, stop);
		if (unlikely(ret < 0)) {
			pr_err("ftiic010_tx_msg byte 0x%.2x fail (%d)\r\n", (int)msg->addr, ret);
			return ret;
		}
	}

	return 0;
}

static int ftiic010_rx_msg(struct ftiic010 *ftiic010,
			   struct i2c_msg *msg, int last)
{
	__u8 data;
	int i, ret, nak, stop;

	data = (msg->addr & 0x7f) << 1 | 1;	/* read */
	ret = ftiic010_tx_byte(ftiic010, data, 1, 0);
	if (unlikely(ret < 0)) {
		pr_err("ftiic010_rx_msg addr 0x%.2x fail (%d)\r\n", (int)msg->addr, ret);
		return ret;
	}

	for (i = 0; i < msg->len; i++) {
		nak = 0;
		stop = 0;

		if (i + 1 == msg->len) {
			if (last)
				stop = 1;
			nak = 1;
		}

		ret = ftiic010_rx_byte(ftiic010, stop, nak);
		if (unlikely(ret < 0)) {
			pr_err("ftiic010_rx_msg addr 0x%.2x fail (%d)\r\n", (int)msg->addr, ret);
			return ret;
		}

		msg->buf[i] = ret;
	}

	return 0;
}

static int ftiic010_do_msg(struct ftiic010 *ftiic010,
			   struct i2c_msg *msg, int last)
{
	if (msg->flags & I2C_M_RD)
		return ftiic010_rx_msg(ftiic010, msg, last);
	else
		return ftiic010_tx_msg(ftiic010, msg, last);
}

/******************************************************************************
 * interrupt handler
 *****************************************************************************/
#ifdef CONFIG_I2C_INTERRUPT_MODE
static irqreturn_t ftiic010_interrupt(int irq, void *dev_id)
{
	struct ftiic010 *ftiic010 = dev_id;
	struct i2c_adapter *adapter = &ftiic010->adapter;
	unsigned int sr;
	#ifndef CONFIG_PLATFORM_NA51039
	unsigned int cr;
	#endif

	sr = ioread32(ftiic010->base + FTIIC010_OFFSET_SR);

	#ifndef CONFIG_PLATFORM_NA51039
	if (sr & FTIIC010_SR_DT) {
		dev_dbg(&adapter->dev, "data transmitted\n");
		if (!(sr & FTIIC010_SR_BERR) && !(sr & FTIIC010_SR_AL)) {
			ftiic010->ack = 1;
			wake_up(&ftiic010->waitq);
		}
	}
	#endif

	if (sr & FTIIC010_SR_DR) {
		dev_dbg(&adapter->dev, "data received\n");
		if (!(sr & FTIIC010_SR_AL)) {
			ftiic010->ack = 1;
			wake_up(&ftiic010->waitq);
		}
	}

	#ifdef CONFIG_PLATFORM_NA51039
	/* I'm transmiter but receives NAK */
	if ((!(sr & FTIIC010_SR_RW)) && (sr & FTIIC010_SR_BERR)) {	/* Device NAK */
	#else
	if (sr & FTIIC010_SR_BERR) { ///< Device NAK
		///< Disable DT interrupt
		cr = ioread32(ftiic010->base + FTIIC010_OFFSET_CR);
		cr &= ~(FTIIC010_CR_DTI_EN);
		iowrite32(cr, ftiic010->base + FTIIC010_OFFSET_CR);
	#endif
		if (ftiic010->hdmi_i2c) {	/* for edid spec, NAK can be ignored */
			if (sr & FTIIC010_SR_BERR)
				ftiic010->nack = 1;	/* only the case: When the cable is not connected, nak interrupt only raise when start issued */
			ftiic010->ack = 1;
		} else {
			ftiic010->err = 1;
			dev_err(&adapter->dev, "NAK!\n");
		}
	}

	if (sr & FTIIC010_SR_AL)
		dev_err(&adapter->dev, "arbitration lost!\n");

	#ifdef CONFIG_PLATFORM_NA51039
	///< Clear status, write 1 to clear
	iowrite32(sr, ftiic010->base + FTIIC010_OFFSET_SR);
	#endif

	return IRQ_HANDLED;
}
#endif

/******************************************************************************
 * struct i2c_algorithm functions
 *****************************************************************************/
static int ftiic010_master_xfer(struct i2c_adapter *adapter,
				struct i2c_msg *msgs, int num)
{
	struct ftiic010 *ftiic010 = i2c_get_adapdata(adapter);
	int i, ret, last = 0;

#ifdef CONFIG_I2C_INTERRUPT_MODE
	if (irqs_disabled()) {
		printk_ratelimited(KERN_ERR
				   "Please don't disable interrupt during I2C transfers (in I2C interrupt mode)!\n");
		return -EIO;
	}
#endif

	down(&ftiic010->sem_lock);

	for (i = 0; i < num; i++) {

		if (i == num - 1)
			last = 1;

		ret = ftiic010_do_msg(ftiic010, &msgs[i], last);
		if (unlikely(ret < 0)) {
			ftiic010_hw_init(ftiic010);
			up(&ftiic010->sem_lock);
			return ret;
		}
	}

	up(&ftiic010->sem_lock);
	return num;
}

static u32 ftiic010_functionality(struct i2c_adapter *adapter)
{
	return I2C_FUNC_I2C | I2C_FUNC_SMBUS_EMUL;
}

static struct i2c_algorithm ftiic010_algorithm = {
	.master_xfer = ftiic010_master_xfer,
	.functionality = ftiic010_functionality,
};

#ifdef CONFIG_OF
static int ftiic010_i2c_of_probe(struct platform_device *pdev,
				 struct ftiic010 *i2c)
{
	struct device_node *np = pdev->dev.of_node;
	u32 clock_frequency;
	u32 irq;

	if (of_property_read_u32(np, "clock-frequency", &clock_frequency)) {
		dev_err(&pdev->dev, "Missing parameter 'clock-frequency'\n");
		clock_frequency = SCL_SPEED;
	}
	i2c->default_bus_speed = clock_frequency;

#if defined(CONFIG_PLATFORM_NA51039)
	if (i2c->ep_no != -1) {
		irq = 0;
		of_property_read_u32(np, "ep_irqs", &irq);
	}
	else
#endif
	irq = irq_of_parse_and_map(np, 0);
	if (irq == 0) {
		dev_err(&pdev->dev,
			"Missing required parameter 'IRQ number'\n");
		return -ENODEV;
	}
	i2c->irq = irq;

#if defined(CONFIG_PLATFORM_NA51039)
	if (i2c_of_probe_count < (I2C_FTI2C010_COUNT*MAX_CHIPS)) {
#else
	if (i2c_of_probe_count < I2C_FTI2C010_COUNT) {
#endif
		pdev->id = i2c_of_probe_count;
		i2c_of_probe_count++;
	} else
		dev_err(&pdev->dev,
			"Only %d FTIIC010 hosts are available, please check the device tree script!\n",
#if defined(CONFIG_PLATFORM_NA51039)
			(I2C_FTI2C010_COUNT*MAX_CHIPS));
#else
			I2C_FTI2C010_COUNT);
#endif
	return 0;
}
#else
#define ftiic010_i2c_of_probe(pdev, i2c) -ENODEV
#endif

/******************************************************************************
 * proc node functions
 *****************************************************************************/
/*
	proc fuction - bus speed
*/
static int ftiic010_proc_bus_speed_show(struct seq_file *sfile, void *v)
{
	int i, hz;

	seq_puts(sfile, "\nCommands to set the I2C bus speed: ");
	seq_printf(sfile,
		   "\necho [bus id] [speed in KHz, 50~400 KHz] > /proc/ftiic010/bus_speed");
	seq_printf(sfile,
		   "\ne.g. \"echo 0 50 > /proc/ftiic010/bus_speed\" -> set bus speed of I2C#0 to 50 KHz.");
	seq_printf(sfile,
		   "\n-------------------------------------------------------------------------------\n");
	seq_puts(sfile, "Current I2C bus default speed:\n");

#if defined(CONFIG_PLATFORM_NA51039)
	for (i = 0; i < (I2C_FTI2C010_COUNT*MAX_CHIPS); i++) {
#else
	for (i = 0; i < I2C_FTI2C010_COUNT; i++) {
#endif
		hz = ftiic010_get_default_bus_speed(i);
		if (hz != -1)
			seq_printf(sfile, "I2C#%d: %d (KHz)\n", i, hz / 1000);
	}
	return 0;
}

static ssize_t ftiic010_proc_bus_speed_write(struct file *file,
					     const char __user *buffer,
					     size_t count, loff_t *ppos)
{
	int ret, bus_id, speed;
	char value_str[32] = { '\0' };

	if (copy_from_user(value_str, buffer, count))
		return -EFAULT;

	ret = sscanf(value_str, "%d %d\n", &bus_id, &speed);

	ret = ftiic010_set_default_bus_speed(bus_id, speed * 1000);
	if (ret == 0) {
		printk_ratelimited(KERN_NOTICE
				   "Set the bus speed of I2C#%d to %d KHz!\n",
				   bus_id, speed);
	}

	return count;
}

static int ftiic010_proc_bus_speed_open(struct inode *inode, struct file *file)
{
	return single_open(file, ftiic010_proc_bus_speed_show, PDE_DATA(inode));
}

static const struct file_operations ftiic010_proc_bus_speed_ops = {
	.owner = THIS_MODULE,
	.open = ftiic010_proc_bus_speed_open,
	.write = ftiic010_proc_bus_speed_write,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

/*
	proc fuction - common
*/
static void ftiic010_proc_remove(void)
{
	if (ftiic010_proc_root) {
		if (ftiic010_proc_bus_speed)
			remove_proc_entry("bus_speed", ftiic010_proc_root);
	}
	remove_proc_entry("ftiic010", NULL);
	ftiic010_proc_root = NULL;
}

static int ftiic010_proc_init(void)
{
	int ret = 0;

	/* root */
	ftiic010_proc_root = proc_mkdir("ftiic010", NULL);
	if (!ftiic010_proc_root) {
		pr_err("[I2C] create proc node 'ftiic010' failed!\n");
		ret = -EINVAL;
		goto end;
	}

	/* bus_speed */
	ftiic010_proc_bus_speed =
	    proc_create_data("bus_speed", S_IRUGO | S_IXUGO, ftiic010_proc_root,
			     &ftiic010_proc_bus_speed_ops, NULL);
	if (!ftiic010_proc_bus_speed) {
		pr_err("[I2C] create proc node 'ftiic010/bus_speed' failed!\n");
		ret = -EINVAL;
		goto err;
	}

end:
	return ret;

err:
	ftiic010_proc_remove();
	return ret;
}

/******************************************************************************
 * struct platform_driver functions
 *****************************************************************************/
static int ftiic010_probe(struct platform_device *pdev)
{
	struct ftiic010 *ftiic010;
	struct resource *res;
	int irq;
	int ret;
	char name[32];
#ifdef CONFIG_OF
    const char *status;
     int statlen;
	u32 *pID;
#if defined(CONFIG_PLATFORM_NA51039)
	u32 *pEP;
	int EPno=-1;
#endif
#endif

#ifdef CONFIG_OF
    status = (char *)of_get_property(pdev->dev.of_node, "status", &statlen);
    if (status) {
	    if (statlen > 0) {
			if (!strcmp(status, "disabled"))
				return -ENXIO;
		}
	}

#if defined(CONFIG_PLATFORM_NA51039)
	pEP = (u32 *)of_get_property(pdev->dev.of_node, "ep", NULL);
	if (pEP) {
		EPno = __be32_to_cpu(*pEP);
		if (EPno==0 && !nvt_pcie_downstream_active(PCIE_EP0))
			return -ENXIO;
		else if (EPno==1 && !nvt_pcie_downstream_active(PCIE_EP1))
			return -ENXIO;
		else if (EPno==2 && !nvt_pcie_downstream_active(PCIE_EP2))
			return -ENXIO;
	}
#endif
#endif

	/*
	 * This function will be called several times
	 * and pass different pdev structure
	 */
	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res)
		return -ENXIO;

#if defined(CONFIG_PLATFORM_NA51039)
	switch(EPno) {
	case 0:
		res->start = nvt_pcie_downstream_addr(PCIE_EP0, res->start);
		res->end = nvt_pcie_downstream_addr(PCIE_EP0, res->end);
		break;
	case 1:
		res->start = nvt_pcie_downstream_addr(PCIE_EP1, res->start);
		res->end = nvt_pcie_downstream_addr(PCIE_EP1, res->end);
		break;
	case 2:
		res->start = nvt_pcie_downstream_addr(PCIE_EP2, res->start);
		res->end = nvt_pcie_downstream_addr(PCIE_EP2, res->end);
		break;
	}
#endif

	ftiic010 = kzalloc(sizeof(*ftiic010), GFP_KERNEL);
	if (!ftiic010) {
		ret = -ENOMEM;
		dev_err(&pdev->dev, "Could not allocate private data\n");
		goto err_alloc;
	}

#if defined(CONFIG_PLATFORM_NA51039)
	ftiic010->ep_no = EPno;
#endif

	init_waitqueue_head(&ftiic010->waitq);

	/* Mark the region is occupied */
	ftiic010->res = request_mem_region(res->start,
					   res->end - res->start,
					   dev_name(&pdev->dev));
	if (ftiic010->res == NULL) {
		dev_err(&pdev->dev, "Could not reserve memory region\n");
		ret = -ENOMEM;
		goto err_req_mem;
	}

	ftiic010->base = ioremap(res->start, res->end - res->start);
	if (ftiic010->base == NULL) {
		dev_err(&pdev->dev, "Failed to ioremap\n");
		ret = -ENOMEM;
		goto err_ioremap;
	}

	/* initialize i2c adapter */
	ftiic010->adapter.owner = THIS_MODULE;
	ftiic010->adapter.algo = &ftiic010_algorithm;
	ftiic010->adapter.timeout = 1;
	ftiic010->adapter.dev.parent = &pdev->dev;
#ifdef CONFIG_OF
	ftiic010->adapter.dev.of_node = pdev->dev.of_node;
	if (ftiic010_i2c_of_probe(pdev, ftiic010) != 0)
		ftiic010->default_bus_speed = SCL_SPEED;
	irq = ftiic010->irq;

	pID = (u32 *)of_get_property(pdev->dev.of_node, "id", NULL);
	if (pID)
		pdev->id = __be32_to_cpu(*pID);

	if ((u32 *)of_get_property(pdev->dev.of_node, "hdmi", NULL))
		ftiic010->hdmi_i2c = 1;

#ifdef CONFIG_PLATFORM_NA51039 /* temp sol */
	if (pdev->id == 5)
		ftiic010->hdmi_i2c = 1;
#endif

#else
	ftiic010->default_bus_speed = SCL_SPEED;
	irq = platform_get_irq(pdev, 0);
#endif
	snprintf(name, 32, "ftiic010 adapter#%d", pdev->id);
	strcpy(ftiic010->adapter.name, name);

	if (i2c_pdev_array[pdev->id] == NULL)
		i2c_pdev_array[pdev->id] = pdev;

	i2c_set_adapdata(&ftiic010->adapter, ftiic010);

#ifdef CONFIG_I2C_INTERRUPT_MODE
	ret =
	    request_irq(irq, ftiic010_interrupt, IRQF_SHARED, pdev->name,
			ftiic010);
	if (ret) {
		dev_err(&pdev->dev, "Failed to request irq %d\n", irq);
		goto err_req_irq;
	}

	ftiic010->irq = irq;
#endif

	/* Create the proc node for adjusting default bus speed */
	if (ftiic010_proc_root == NULL) {
		ret = ftiic010_proc_init();
		if (ret < 0)
			ftiic010_proc_remove();
	}

#ifdef CONFIG_I2C_INTERRUPT_MODE
	dev_info(&pdev->dev, "irq %d, mapped at %p\n", irq, ftiic010->base);
#endif

	pr_info("NVT I2C%d Driver Version: %s(hdmi:%s)\n", pdev->id, NVT_I2C_VERSION, ftiic010->hdmi_i2c ? "yes":"no");
	sema_init(&ftiic010->sem_lock, 1);

	platform_set_drvdata(pdev, ftiic010);

	/* config clk */
	ftiic010->clk = devm_clk_get(&pdev->dev, dev_name(&pdev->dev));
	if (IS_ERR(ftiic010->clk)) {
		dev_err(&pdev->dev, "can't find clock %s\n", dev_name(&pdev->dev));
		ftiic010->clk = NULL;
	}
	else {
		clk_prepare(ftiic010->clk);
		clk_enable(ftiic010->clk);
	}

	ftiic010_hw_init(ftiic010);

//#define I2C_DYNAMIC_BUS_NUM
#ifndef I2C_DYNAMIC_BUS_NUM
	/* "pdev->id" was defined in platform.c */
	ftiic010->adapter.nr = pdev->id;
	ret = i2c_add_numbered_adapter(&ftiic010->adapter);
#else
	ret = i2c_add_adapter(&ftiic010->adapter);
#endif
	if (ret) {
		dev_err(&pdev->dev, "Failed to add i2c adapter\n");
		goto err_add_adapter;
	}

	return 0;

err_add_adapter:
#ifdef CONFIG_I2C_INTERRUPT_MODE
	free_irq(ftiic010->irq, ftiic010);
err_req_irq:
#endif
	iounmap(ftiic010->base);
err_ioremap:
	release_resource(ftiic010->res);
err_req_mem:
	kfree(ftiic010);
err_alloc:
	return ret;
};

static int ftiic010_remove(struct platform_device *pdev)
{
	struct ftiic010 *ftiic010 = platform_get_drvdata(pdev);

	platform_set_drvdata(pdev, NULL);
	i2c_del_adapter(&ftiic010->adapter);
#ifdef CONFIG_I2C_INTERRUPT_MODE
	free_irq(ftiic010->irq, ftiic010);
#endif

	clk_disable_unprepare(ftiic010->clk);
	ftiic010->clk = NULL;

	iounmap(ftiic010->base);
	release_resource(ftiic010->res);
	kfree(ftiic010);
	return 0;
};

#ifdef CONFIG_OF
static const struct of_device_id ftiic_of_ids[] = {
	{.compatible = "nvt,ftiic010"},
	{},
};

MODULE_DEVICE_TABLE(of, ftiic_of_ids);
#else
#define ftiic_of_ids     NULL
#endif

static struct platform_driver ftiic010_driver = {
	.probe = ftiic010_probe,
	.remove = ftiic010_remove,
	.driver = {
		   .name = "nvt,ftiic010",
		   .owner = THIS_MODULE,
		   .of_match_table = of_match_ptr(ftiic_of_ids),
		   },
};

/******************************************************************************
 * initialization / finalization
 *****************************************************************************/
static int __init ftiic010_init(void)
{
	return platform_driver_register(&ftiic010_driver);
}

static void __exit ftiic010_exit(void)
{
	platform_driver_unregister(&ftiic010_driver);
}

module_init(ftiic010_init);
module_exit(ftiic010_exit);

MODULE_AUTHOR("Po-Yu Chuang <ratbert@faraday-tech.com>");
MODULE_DESCRIPTION("FTIIC010 I2C bus adapter");
MODULE_LICENSE("GPL");
MODULE_VERSION(NVT_I2C_VERSION);
