#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/i2c.h>
#include <linux/clk.h>
#include <linux/errno.h>
#include <linux/sched.h>
#include <linux/err.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/io.h>
#include <linux/slab.h>
#include <mach/i2c.h>
//#define FH_I2C_DEBUG

#ifdef FH_I2C_DEBUG
#define PRINT_DBG(fmt, args...)  printk(fmt, ## args)
#else
#define PRINT_DBG(fmt, args...)  do { } while (0)
#endif

/*
 * Registers offset
 */
#define DW_IC_CON		0x0
#define DW_IC_TAR		0x4
#define DW_IC_DATA_CMD		0x10
#define DW_IC_SS_SCL_HCNT	0x14
#define DW_IC_SS_SCL_LCNT	0x18
#define DW_IC_FS_SCL_HCNT	0x1c
#define DW_IC_FS_SCL_LCNT	0x20
#define DW_IC_INTR_STAT		0x2c
#define DW_IC_INTR_MASK		0x30
#define DW_IC_RAW_INTR_STAT	0x34
#define DW_IC_RX_TL		0x38
#define DW_IC_TX_TL		0x3c
#define DW_IC_CLR_INTR		0x40
#define DW_IC_CLR_RX_UNDER	0x44
#define DW_IC_CLR_RX_OVER	0x48
#define DW_IC_CLR_TX_OVER	0x4c
#define DW_IC_CLR_RD_REQ	0x50
#define DW_IC_CLR_TX_ABRT	0x54
#define DW_IC_CLR_RX_DONE	0x58
#define DW_IC_CLR_ACTIVITY	0x5c
#define DW_IC_CLR_STOP_DET	0x60
#define DW_IC_CLR_START_DET	0x64
#define DW_IC_CLR_GEN_CALL	0x68
#define DW_IC_ENABLE		0x6c
#define DW_IC_STATUS		0x70
#define DW_IC_TXFLR		0x74
#define DW_IC_RXFLR		0x78
#define DW_IC_COMP_PARAM_1	0xf4
#define DW_IC_TX_ABRT_SOURCE	0x80

#define DW_IC_CON_MASTER		0x1
#define DW_IC_CON_SPEED_STD		0x2
#define DW_IC_CON_SPEED_FAST		0x4
#define DW_IC_CON_10BITADDR_MASTER	0x10
#define DW_IC_CON_RESTART_EN		0x20
#define DW_IC_CON_SLAVE_DISABLE		0x40

#define DW_IC_INTR_RX_UNDER	0x001
#define DW_IC_INTR_RX_OVER	0x002
#define DW_IC_INTR_RX_FULL	0x004
#define DW_IC_INTR_TX_OVER	0x008
#define DW_IC_INTR_TX_EMPTY	0x010
#define DW_IC_INTR_RD_REQ	0x020
#define DW_IC_INTR_TX_ABRT	0x040
#define DW_IC_INTR_RX_DONE	0x080
#define DW_IC_INTR_ACTIVITY	0x100
#define DW_IC_INTR_STOP_DET	0x200
#define DW_IC_INTR_START_DET	0x400
#define DW_IC_INTR_GEN_CALL	0x800

#define DW_IC_INTR_DEFAULT_MASK		(DW_IC_INTR_RX_FULL | \
					 DW_IC_INTR_TX_EMPTY | \
					 DW_IC_INTR_TX_ABRT | \
					 DW_IC_INTR_STOP_DET)

#define DW_IC_STATUS_ACTIVITY	0x1
#define DW_IC_STATUS_MASTER_ACTIVITY   0x20

#define DW_IC_ERR_TX_ABRT	0x1

/*
 * status codes
 */
#define STATUS_IDLE			0x0
#define STATUS_WRITE_IN_PROGRESS	0x1
#define STATUS_READ_IN_PROGRESS		0x2

#define TIMEOUT			20 /* ms */

/*
 * hardware abort codes from the DW_IC_TX_ABRT_SOURCE register
 *
 * only expected abort codes are listed here
 * refer to the datasheet for the full list
 */
#define ABRT_7B_ADDR_NOACK	0
#define ABRT_10ADDR1_NOACK	1
#define ABRT_10ADDR2_NOACK	2
#define ABRT_TXDATA_NOACK	3
#define ABRT_GCALL_NOACK	4
#define ABRT_GCALL_READ		5
#define ABRT_SBYTE_ACKDET	7
#define ABRT_SBYTE_NORSTRT	9
#define ABRT_10B_RD_NORSTRT	10
#define ABRT_MASTER_DIS		11
#define ARB_LOST		12

#define DW_IC_TX_ABRT_7B_ADDR_NOACK	(1UL << ABRT_7B_ADDR_NOACK)
#define DW_IC_TX_ABRT_10ADDR1_NOACK	(1UL << ABRT_10ADDR1_NOACK)
#define DW_IC_TX_ABRT_10ADDR2_NOACK	(1UL << ABRT_10ADDR2_NOACK)
#define DW_IC_TX_ABRT_TXDATA_NOACK	(1UL << ABRT_TXDATA_NOACK)
#define DW_IC_TX_ABRT_GCALL_NOACK	(1UL << ABRT_GCALL_NOACK)
#define DW_IC_TX_ABRT_GCALL_READ	(1UL << ABRT_GCALL_READ)
#define DW_IC_TX_ABRT_SBYTE_ACKDET	(1UL << ABRT_SBYTE_ACKDET)
#define DW_IC_TX_ABRT_SBYTE_NORSTRT	(1UL << ABRT_SBYTE_NORSTRT)
#define DW_IC_TX_ABRT_10B_RD_NORSTRT	(1UL << ABRT_10B_RD_NORSTRT)
#define DW_IC_TX_ABRT_MASTER_DIS	(1UL << ABRT_MASTER_DIS)
#define DW_IC_TX_ARB_LOST		(1UL << ARB_LOST)

#define DW_IC_TX_ABRT_NOACK		(DW_IC_TX_ABRT_7B_ADDR_NOACK | \
					 DW_IC_TX_ABRT_10ADDR1_NOACK | \
					 DW_IC_TX_ABRT_10ADDR2_NOACK | \
					 DW_IC_TX_ABRT_TXDATA_NOACK | \
					 DW_IC_TX_ABRT_GCALL_NOACK)

static char *abort_sources[] = {
	[ABRT_7B_ADDR_NOACK] =
		"slave address not acknowledged (7bit mode)",
	[ABRT_10ADDR1_NOACK] =
		"first address byte not acknowledged (10bit mode)",
	[ABRT_10ADDR2_NOACK] =
		"second address byte not acknowledged (10bit mode)",
	[ABRT_TXDATA_NOACK] =
		"data not acknowledged",
	[ABRT_GCALL_NOACK] =
		"no acknowledgement for a general call",
	[ABRT_GCALL_READ] =
		"read after general call",
	[ABRT_SBYTE_ACKDET] =
		"start byte acknowledged",
	[ABRT_SBYTE_NORSTRT] =
		"trying to send start byte when restart is disabled",
	[ABRT_10B_RD_NORSTRT] =
		"trying to read when restart is disabled (10bit mode)",
	[ABRT_MASTER_DIS] =
		"trying to use disabled adapter",
	[ARB_LOST] =
		"lost arbitration",
};

/**
 * struct fh_i2c_dev - private i2c-designware data
 * @dev: driver model device node
 * @base: IO registers pointer
 * @cmd_complete: tx completion indicator
 * @lock: protect this struct and IO registers
 * @clk: input reference clock
 * @cmd_err: run time hadware error code
 * @msgs: points to an array of messages currently being transferred
 * @msgs_num: the number of elements in msgs
 * @msg_write_idx: the element index of the current tx message in the msgs
 *	array
 * @tx_buf_len: the length of the current tx buffer
 * @tx_buf: the current tx buffer
 * @msg_read_idx: the element index of the current rx message in the msgs
 *	array
 * @rx_buf_len: the length of the current rx buffer
 * @rx_buf: the current rx buffer
 * @msg_err: error status of the current transfer
 * @status: i2c master status, one of STATUS_*
 * @abort_source: copy of the TX_ABRT_SOURCE register
 * @irq: interrupt number for the i2c master
 * @adapter: i2c subsystem adapter node
 * @tx_fifo_depth: depth of the hardware tx fifo
 * @rx_fifo_depth: depth of the hardware rx fifo
 */
struct fh_i2c_dev {
	struct device		*dev;
	void __iomem		*base;
	struct completion	cmd_complete;
	struct mutex		lock;
	struct clk		*clk;
	int			cmd_err;
	struct i2c_msg		*msgs;
	int			msgs_num;
	int			msg_write_idx;
	u32			tx_buf_len;
	u8			*tx_buf;
	int			msg_read_idx;
	u32			rx_buf_len;
	u8			*rx_buf;
	int			msg_err;
	unsigned int		status;
	u32			abort_source;
	int			irq;
	struct i2c_adapter	adapter;
	unsigned int		tx_fifo_depth;
	unsigned int		rx_fifo_depth;
};


static int i2c_fh_wait_master_not_active(struct fh_i2c_dev *dev)
{
    int timeout = 200;  //2000 us

    while (I2c_IsActiveMst( dev->base))
    {
        if (timeout <= 0)
        {
            dev_warn(dev->dev, "timeout waiting for master not active\n");
            return -ETIMEDOUT;
        }
        timeout--;
        udelay(10);
    }

    return 0;
}

static u32
i2c_fh_scl_hcnt(u32 ic_clk, u32 tSYMBOL, u32 tf, int cond, int offset)
{
	/*
	 * DesignWare I2C core doesn't seem to have solid strategy to meet
	 * the tHD;STA timing spec.  Configuring _HCNT based on tHIGH spec
	 * will result in violation of the tHD;STA spec.
	 */
	if (cond)
		/*
		 * Conditional expression:
		 *
		 *   IC_[FS]S_SCL_HCNT + (1+4+3) >= IC_CLK * tHIGH
		 *
		 * This is based on the DW manuals, and represents an ideal
		 * configuration.  The resulting I2C bus speed will be
		 * faster than any of the others.
		 *
		 * If your hardware is free from tHD;STA issue, try this one.
		 */
		return (ic_clk * tSYMBOL + 5000) / 10000 - 8 + offset;
	else
		/*
		 * Conditional expression:
		 *
		 *   IC_[FS]S_SCL_HCNT + 3 >= IC_CLK * (tHD;STA + tf)
		 *
		 * This is just experimental rule; the tHD;STA period turned
		 * out to be proportinal to (_HCNT + 3).  With this setting,
		 * we could meet both tHIGH and tHD;STA timing specs.
		 *
		 * If unsure, you'd better to take this alternative.
		 *
		 * The reason why we need to take into account "tf" here,
		 * is the same as described in i2c_fh_scl_lcnt().
		 */
		return (ic_clk * (tSYMBOL + tf) + 5000) / 10000 - 3 + offset;
}

static u32 i2c_fh_scl_lcnt(u32 ic_clk, u32 tLOW, u32 tf, int offset)
{
	/*
	 * Conditional expression:
	 *
	 *   IC_[FS]S_SCL_LCNT + 1 >= IC_CLK * (tLOW + tf)
	 *
	 * DW I2C core starts counting the SCL CNTs for the LOW period
	 * of the SCL clock (tLOW) as soon as it pulls the SCL line.
	 * In order to meet the tLOW timing spec, we need to take into
	 * account the fall time of SCL signal (tf).  Default tf value
	 * should be 0.3 us, for safety.
	 */
	return ((ic_clk * (tLOW + tf) + 5000) / 10000) - 1 + offset;
}

/**
 * i2c_fh_init() - initialize the designware i2c master hardware
 * @dev: device private data
 *
 * This functions configures and enables the I2C master.
 * This function is called during I2C init function, and in case of timeout at
 * run time.
 */
static void i2c_fh_init(struct fh_i2c_dev *dev)
{
	u32 input_clock_khz = clk_get_rate(dev->clk) / 1000;
	u32 ic_con, hcnt, lcnt;

	/* Disable the adapter */
	i2c_fh_wait_master_not_active(dev);
	I2c_DisEnable((unsigned int)dev->base);

	/* set standard and fast speed deviders for high/low periods */

	/* Standard-mode */
	hcnt = i2c_fh_scl_hcnt(input_clock_khz,
				40,	/* tHD;STA = tHIGH = 4.0 us */
				3,	/* tf = 0.3 us */
				0,	/* 0: DW default, 1: Ideal */
				0);	/* No offset */
	lcnt = i2c_fh_scl_lcnt(input_clock_khz,
				47,	/* tLOW = 4.7 us */
				3,	/* tf = 0.3 us */
				0);	/* No offset */
	I2c_SetSsHcnt( dev->base ,hcnt);
	I2c_SetSsLcnt( dev->base ,lcnt);
	pr_info("\tClock: %dkhz, Standard-mode HCNT:LCNT = %d:%d\n", input_clock_khz, hcnt, lcnt);

	/* Fast-mode */
	hcnt = i2c_fh_scl_hcnt(input_clock_khz,
				6,	/* tHD;STA = tHIGH = 0.6 us */
				3,	/* tf = 0.3 us */
				0,	/* 0: DW default, 1: Ideal */
				0);	/* No offset */
	lcnt = i2c_fh_scl_lcnt(input_clock_khz,
				13,	/* tLOW = 1.3 us */
				3,	/* tf = 0.3 us */
				0);	/* No offset */
	I2c_SetFsHcnt( dev->base ,hcnt);
	I2c_SetFsLcnt( dev->base ,lcnt);
	//dev_dbg(dev->dev, "Fast-mode HCNT:LCNT = %d:%d\n", hcnt, lcnt);

	/* Configure Tx/Rx FIFO threshold levels */

	I2c_SetTxRxTl(dev->base  ,dev->tx_fifo_depth - 1,0);
	/* configure the i2c master */
	ic_con = DW_IC_CON_MASTER | DW_IC_CON_SLAVE_DISABLE |
		/*DW_IC_CON_RESTART_EN |*/ DW_IC_CON_SPEED_FAST;/// DW_IC_CON_SPEED_STD;
	I2c_SetCon(dev->base,ic_con);

}

/*
 * Waiting for bus not busy
 */
static int i2c_fh_wait_bus_not_busy(struct fh_i2c_dev *dev)
{
	int timeout = TIMEOUT;

	while (I2c_IsActiveMst( dev->base)) {
		if (timeout <= 0) {
			dev_warn(dev->dev, "timeout waiting for bus ready\n");
			return -ETIMEDOUT;
		}
		timeout--;
		msleep(1);
	}

	return 0;
}

static void i2c_fh_xfer_init(struct fh_i2c_dev *dev)
{
	struct i2c_msg *msgs = dev->msgs;
	u32 ic_con;

	/* Disable the adapter */
	i2c_fh_wait_master_not_active(dev);
	I2c_DisEnable((unsigned int)dev->base);

	/* set the slave (target) address */
	I2c_SetDeviceId(dev->base,msgs[dev->msg_write_idx].addr);

	/* if the slave address is ten bit address, enable 10BITADDR */
	ic_con = I2c_GetCon(dev->base);
	if (msgs[dev->msg_write_idx].flags & I2C_M_TEN)
		ic_con |= DW_IC_CON_10BITADDR_MASTER;
	else
		ic_con &= ~DW_IC_CON_10BITADDR_MASTER;
	I2c_SetCon(dev->base,ic_con);

	/* Enable the adapter */
	I2c_Enable(dev->base);

	/* Enable interrupts */
	I2c_SetIntrMask(dev->base,DW_IC_INTR_DEFAULT_MASK);

}

/*
 * Initiate (and continue) low level master read/write transaction.
 * This function is only called from i2c_fh_isr, and pumping i2c_msg
 * messages into the tx buffer.  Even if the size of i2c_msg data is
 * longer than the size of the tx buffer, it handles everything.
 */
static void
i2c_fh_xfer_msg(struct fh_i2c_dev *dev)
{
	struct i2c_msg *msgs = dev->msgs;
	u32 intr_mask, cmd;
	int tx_limit, rx_limit;
	u32 addr = msgs[dev->msg_write_idx].addr;
	u32 buf_len = dev->tx_buf_len;
	u8 *buf = dev->tx_buf;;

	PRINT_DBG("i2c_fh_xfer_msg start, dev->msgs_num: %d\n", dev->msgs_num);

	intr_mask = DW_IC_INTR_DEFAULT_MASK;

	for (; dev->msg_write_idx < dev->msgs_num; dev->msg_write_idx++)
	{
		/*
		 * if target address has changed, we need to
		 * reprogram the target address in the i2c
		 * adapter when we are done with this transfer
		 */
		if (msgs[dev->msg_write_idx].addr != addr) {
			dev_err(dev->dev,
				"%s: invalid target address\n", __func__);
			dev->msg_err = -EINVAL;
			break;
		}

		if (msgs[dev->msg_write_idx].len == 0) {
			dev_err(dev->dev,
				"%s: invalid message length\n", __func__);
			dev->msg_err = -EINVAL;
			break;
		}

		if (!(dev->status & STATUS_WRITE_IN_PROGRESS))
		{
			/* new i2c_msg */
			buf = msgs[dev->msg_write_idx].buf;
			buf_len = msgs[dev->msg_write_idx].len;

			PRINT_DBG("new msg: len: %d, buf: 0x%x\n", buf_len, buf[0]);
		}

		tx_limit = dev->tx_fifo_depth - I2c_GetTxTl(dev->base );
		rx_limit = dev->rx_fifo_depth - I2c_GetRxTl(dev->base );

		while (buf_len > 0 && tx_limit > 0 && rx_limit > 0)
		{
			if (msgs[dev->msg_write_idx].flags & I2C_M_RD)
			{
				cmd = 0x100;
				rx_limit--;
			}
			else
			{
				cmd = *buf++;
			}

			tx_limit--; buf_len--;

			if(!buf_len)
				cmd |= 0x200;

			I2c_Write(dev->base, cmd);
		}
		PRINT_DBG("\n");

		dev->tx_buf = buf;
		dev->tx_buf_len = buf_len;

		if (buf_len > 0)
		{
			/* more bytes to be written */
			dev->status |= STATUS_WRITE_IN_PROGRESS;
			break;
		}
		else
		{
			dev->status &= ~STATUS_WRITE_IN_PROGRESS;
		}
	}

	/*
	 * If i2c_msg index search is completed, we don't need TX_EMPTY
	 * interrupt any more.
	 */

	if (dev->msg_write_idx == dev->msgs_num)
		intr_mask &= ~DW_IC_INTR_TX_EMPTY;

	if (dev->msg_err)
		intr_mask = 0;

	I2c_SetIntrMask(dev->base,intr_mask);

}

static void
i2c_fh_read(struct fh_i2c_dev *dev)
{
	struct i2c_msg *msgs = dev->msgs;
	int rx_valid;

	for (; dev->msg_read_idx < dev->msgs_num; dev->msg_read_idx++)
	{
		u32 len;
		u8 *buf;

		if (!(msgs[dev->msg_read_idx].flags & I2C_M_RD))
			continue;

		if (!(dev->status & STATUS_READ_IN_PROGRESS))
		{
			len = msgs[dev->msg_read_idx].len;
			buf = msgs[dev->msg_read_idx].buf;
		}
		else
		{
			PRINT_DBG("STATUS_READ_IN_PROGRESS\n");
			len = dev->rx_buf_len;
			buf = dev->rx_buf;
		}

		rx_valid = I2c_GetRxFLR(dev->base);

		if(rx_valid == 0)
		{
			PRINT_DBG("rx_valid == 0\n");
		}

		for (; len > 0 && rx_valid > 0; len--, rx_valid--)
		{
			*buf++ = I2c_Read(dev->base);
		}

		PRINT_DBG("i2c_fh_read, len: %d, buf[0]: 0x%x\n", msgs[dev->msg_read_idx].len, msgs[dev->msg_read_idx].buf[0]);

		if (len > 0)
		{
			PRINT_DBG("len > 0\n");
			dev->status |= STATUS_READ_IN_PROGRESS;
			dev->rx_buf_len = len;
			dev->rx_buf = buf;
			return;
		} else
			dev->status &= ~STATUS_READ_IN_PROGRESS;
	}
}

static int i2c_fh_handle_tx_abort(struct fh_i2c_dev *dev)
{
	unsigned long abort_source = dev->abort_source;
	int i;

	if (abort_source & DW_IC_TX_ABRT_NOACK) {
		for_each_set_bit(i, &abort_source, ARRAY_SIZE(abort_sources))
		{
			PRINT_DBG(
				"%s: %s\n", __func__, abort_sources[i]);
		}
		return -EREMOTEIO;
	}

	for_each_set_bit(i, &abort_source, ARRAY_SIZE(abort_sources))
		dev_err(dev->dev, "%s: %s\n", __func__, abort_sources[i]);

	if (abort_source & DW_IC_TX_ARB_LOST)
		return -EAGAIN;
	else if (abort_source & DW_IC_TX_ABRT_GCALL_READ)
		return -EINVAL; /* wrong msgs[] data */
	else
		return -EIO;
}

/*
 * Prepare controller for a transaction and call i2c_fh_xfer_msg
 */
static int
i2c_fh_xfer(struct i2c_adapter *adap, struct i2c_msg *msgs, int num)
{
	struct fh_i2c_dev *dev = i2c_get_adapdata(adap);
	int ret;

	PRINT_DBG("-------i2c, %s: msgs: %d\n", __func__, num);

	mutex_lock(&dev->lock);

	INIT_COMPLETION(dev->cmd_complete);
	dev->msgs = msgs;
	dev->msgs_num = num;
	dev->cmd_err = 0;
	dev->msg_write_idx = 0;
	dev->msg_read_idx = 0;
	dev->msg_err = 0;
	dev->status = STATUS_IDLE;
	dev->abort_source = 0;

	ret = i2c_fh_wait_bus_not_busy(dev);
	if (ret < 0)
	{
		goto done;
	}

	/* start the transfers */
	i2c_fh_xfer_init(dev);

	/* wait for tx to complete */
	ret = wait_for_completion_interruptible_timeout(&dev->cmd_complete, HZ);
	if (ret == 0) {
		dev_err(dev->dev, "controller timed out\n");
		i2c_fh_init(dev);
		ret = -ETIMEDOUT;
		goto done;
	} else if (ret < 0)
		goto done;

	if (dev->msg_err)
	{
		PRINT_DBG("dev->msg_err\n");
		ret = dev->msg_err;
		goto done;
	}

	/* no error */
	if (likely(!dev->cmd_err)) {
		/* Disable the adapter */
	    i2c_fh_wait_master_not_active(dev);
	    I2c_DisEnable(dev->base);
		ret = num;
		goto done;
	}

	/* We have an error */
	if (dev->cmd_err == DW_IC_ERR_TX_ABRT)
	{
		PRINT_DBG("dev->cmd_err == DW_IC_ERR_TX_ABRT\n");
		ret = i2c_fh_handle_tx_abort(dev);
		goto done;
	}

	ret = -EIO;

done:
	PRINT_DBG("buf: 0x%x\n", dev->msgs[num - 1].buf[0]);
	mutex_unlock(&dev->lock);

	return ret;
}

static u32 i2c_fh_func(struct i2c_adapter *adap)
{
	return	I2C_FUNC_I2C |
		I2C_FUNC_SMBUS_BYTE |
		I2C_FUNC_SMBUS_BYTE_DATA |
		I2C_FUNC_SMBUS_WORD_DATA |
		I2C_FUNC_SMBUS_I2C_BLOCK;
}

static u32 i2c_fh_read_clear_intrbits(struct fh_i2c_dev *dev)
{
	u32 stat;

	/*
	 * The IC_INTR_STAT register just indicates "enabled" interrupts.
	 * Ths unmasked raw version of interrupt status bits are available
	 * in the IC_RAW_INTR_STAT register.
	 *
	 * That is,
	 *   stat = readl(IC_INTR_STAT);
	 * equals to,
	 *   stat = readl(IC_RAW_INTR_STAT) & readl(IC_INTR_MASK);
	 *
	 * The raw version might be useful for debugging purposes.
	 */
	stat = readl(dev->base + DW_IC_INTR_STAT);

	/*
	 * Do not use the IC_CLR_INTR register to clear interrupts, or
	 * you'll miss some interrupts, triggered during the period from
	 * readl(IC_INTR_STAT) to readl(IC_CLR_INTR).
	 *
	 * Instead, use the separately-prepared IC_CLR_* registers.
	 */
	if (stat & DW_IC_INTR_RX_UNDER)
		I2c_ClrIntr(dev->base,DW_IC_CLR_RX_UNDER);
	if (stat & DW_IC_INTR_RX_OVER)
		I2c_ClrIntr(dev->base , DW_IC_CLR_RX_OVER);
	if (stat & DW_IC_INTR_TX_OVER)
		I2c_ClrIntr(dev->base , DW_IC_CLR_TX_OVER);
	if (stat & DW_IC_INTR_RD_REQ)
		I2c_ClrIntr(dev->base , DW_IC_CLR_RD_REQ);
	if (stat & DW_IC_INTR_TX_ABRT) {
		/*
		 * The IC_TX_ABRT_SOURCE register is cleared whenever
		 * the IC_CLR_TX_ABRT is read.  Preserve it beforehand.
		 */
		dev->abort_source = readl(dev->base + DW_IC_TX_ABRT_SOURCE);
		I2c_ClrIntr(dev->base , DW_IC_CLR_TX_ABRT);
	}
	if (stat & DW_IC_INTR_RX_DONE)
		I2c_ClrIntr(dev->base ,DW_IC_CLR_RX_DONE);
	if (stat & DW_IC_INTR_ACTIVITY)
		I2c_ClrIntr(dev->base ,DW_IC_CLR_ACTIVITY);
	if (stat & DW_IC_INTR_STOP_DET)
		I2c_ClrIntr(dev->base , DW_IC_CLR_STOP_DET);
	if (stat & DW_IC_INTR_START_DET)
		I2c_ClrIntr(dev->base , DW_IC_CLR_START_DET);
	if (stat & DW_IC_INTR_GEN_CALL)
		I2c_ClrIntr(dev->base , DW_IC_CLR_GEN_CALL);

	return stat;
}

/*
 * Interrupt service routine. This gets called whenever an I2C interrupt
 * occurs.
 */
static irqreturn_t i2c_fh_isr(int this_irq, void *dev_id)
{
	struct fh_i2c_dev *dev = dev_id;
	u32 stat;

	stat = i2c_fh_read_clear_intrbits(dev);
	PRINT_DBG("-----------i2c, %s: stat=0x%x\n", __func__, stat);

	if (stat & DW_IC_INTR_TX_ABRT)
	{
		PRINT_DBG("DW_IC_INTR_TX_ABRT\n");
		dev->cmd_err |= DW_IC_ERR_TX_ABRT;
		dev->status = STATUS_IDLE;

		/*
		 * Anytime TX_ABRT is set, the contents of the tx/rx
		 * buffers are flushed.  Make sure to skip them.
		 */
		I2c_SetIntrMask( dev->base,DW_IC_INTR_NONE);
		goto tx_aborted;
	}

	if (stat & DW_IC_INTR_RX_FULL)
	{
		PRINT_DBG("i2c_fh_read\n");
		i2c_fh_read(dev);
	}

	if (stat & DW_IC_INTR_TX_EMPTY)
	{
		PRINT_DBG("i2c_fh_xfer_msg\n");
		i2c_fh_xfer_msg(dev);
	}

	/*
	 * No need to modify or disable the interrupt mask here.
	 * i2c_fh_xfer_msg() will take care of it according to
	 * the current transmit status.
	 */

tx_aborted:
	if ((stat & (DW_IC_INTR_TX_ABRT | DW_IC_INTR_STOP_DET)) || dev->msg_err)
		complete(&dev->cmd_complete);

	return IRQ_HANDLED;
}

static struct i2c_algorithm i2c_fh_algo =
{
	.master_xfer	= i2c_fh_xfer,
	.functionality	= i2c_fh_func,
};

static int __devinit fh_i2c_probe(struct platform_device *pdev)
{
	struct fh_i2c_dev *dev;
	struct i2c_adapter *adap;
	struct resource *mem, *ioarea;
	int irq, r;

	pr_info("I2C driver:\n\tplatform registration... ");

	/* NOTE: driver uses the static register mapping */
	mem = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!mem)
	{
		dev_err(&pdev->dev, "no mem resource?\n");
		return -EINVAL;
	}

	irq = platform_get_irq(pdev, 0);
	if (irq < 0)
	{
		dev_err(&pdev->dev, "no irq resource?\n");
		return irq; /* -ENXIO */
	}

	ioarea = request_mem_region(mem->start, resource_size(mem),
			pdev->name);
	if (!ioarea)
	{
		dev_err(&pdev->dev, "I2C region already claimed\n");
		return -EBUSY;
	}

	dev = kzalloc(sizeof(struct fh_i2c_dev), GFP_KERNEL);
	if (!dev)
	{
		r = -ENOMEM;
		goto err_release_region;
	}

	init_completion(&dev->cmd_complete);
	mutex_init(&dev->lock);
	dev->dev = get_device(&pdev->dev);
	dev->irq = irq;
	platform_set_drvdata(pdev, dev);

	if(pdev->id)
		dev->clk = clk_get(NULL, "i2c1_clk");
	else
		dev->clk = clk_get(NULL, "i2c0_clk");


	if (IS_ERR(dev->clk))
	{
		r = -ENODEV;
		goto err_free_mem;
	}
	clk_enable(dev->clk);

	dev->base = ioremap(mem->start, resource_size(mem));
	if (dev->base == NULL)
	{
		dev_err(&pdev->dev, "failure mapping io resources\n");
		r = -ENOMEM;
		goto err_unuse_clocks;
	}
	{
		dev->tx_fifo_depth = I2c_GetTxFifoDepth(dev->base);
		dev->rx_fifo_depth = I2c_GetRxFifoDepth(dev->base);
	}
	i2c_fh_init(dev);

	pr_info("\ttx fifo depth: %d, rx fifo depth: %d\n", dev->tx_fifo_depth, dev->rx_fifo_depth);

	I2c_SetIntrMask( dev->base,DW_IC_INTR_NONE); /* disable IRQ */
	r = request_irq(dev->irq, i2c_fh_isr, IRQF_DISABLED, pdev->name, dev);
	if (r)
	{
		dev_err(&pdev->dev, "failure requesting irq %i\n", dev->irq);
		goto err_iounmap;
	}

	adap = &dev->adapter;
	i2c_set_adapdata(adap, dev);
	adap->owner = THIS_MODULE;
	adap->class = I2C_CLASS_HWMON;
	strlcpy(adap->name, "FH I2C adapter",
			sizeof(adap->name));
	adap->algo = &i2c_fh_algo;
	adap->dev.parent = &pdev->dev;

	adap->nr = pdev->id;
	r = i2c_add_numbered_adapter(adap);
	if (r) {
		dev_err(&pdev->dev, "failure adding adapter\n");
		goto err_free_irq;
	}

	pr_info("\t%s - (dev. name: %s - id: %d, IRQ #%d\n"
		   "\t\tIO base addr: 0x%p)\n", "I2C", pdev->name,
		   pdev->id, dev->irq, dev->base);

	return 0;

err_free_irq:
	free_irq(dev->irq, dev);
err_iounmap:
	iounmap(dev->base);
err_unuse_clocks:
	clk_disable(dev->clk);
	clk_put(dev->clk);
	dev->clk = NULL;
err_free_mem:
	platform_set_drvdata(pdev, NULL);
	put_device(&pdev->dev);
	kfree(dev);
err_release_region:
	release_mem_region(mem->start, resource_size(mem));

	return r;
}

static int __devexit fh_i2c_remove(struct platform_device *pdev)
{
	struct fh_i2c_dev *dev = platform_get_drvdata(pdev);
	struct resource *mem;

	platform_set_drvdata(pdev, NULL);
	i2c_del_adapter(&dev->adapter);
	put_device(&pdev->dev);

	clk_disable(dev->clk);
	clk_put(dev->clk);
	dev->clk = NULL;
	i2c_fh_wait_master_not_active(dev);
	writel(0, dev->base + DW_IC_ENABLE);
	free_irq(dev->irq, dev);
	kfree(dev);

	mem = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	release_mem_region(mem->start, resource_size(mem));
	return 0;
}

static struct platform_driver fh_i2c_driver =
{
	.remove		= __devexit_p(fh_i2c_remove),
	.driver		=
	{
		.name	= "fh_i2c",
		.owner	= THIS_MODULE,
	},
};

static int __init fh_i2c_init_driver(void)
{
	return platform_driver_probe(&fh_i2c_driver, fh_i2c_probe);
}
module_init(fh_i2c_init_driver);

static void __exit fh_i2c_exit_driver(void)
{
	platform_driver_unregister(&fh_i2c_driver);
}
module_exit(fh_i2c_exit_driver);

MODULE_AUTHOR("QIN");
MODULE_ALIAS("platform:fh");
MODULE_DESCRIPTION("FH I2C bus adapter");
MODULE_LICENSE("GPL");
