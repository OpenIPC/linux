/*
 * Copyright (c) LOTUS. All rights reserved.
 */
#include <linux/slab.h>
#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/i2c.h>
#include <linux/lotus/i2c.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/dma-mapping.h>
#include <asm/string.h>
#ifdef CONFIG_EDMAC
#include <linux/dmaengine.h>
#include <linux/dma-mapping.h>
#endif

/* I2C Registers offsets */
#define LOTUS_I2C_GLB		0x0
#define LOTUS_I2C_SCL_H		0x20
#define LOTUS_I2C_SCL_L		0x24
#define LOTUS_I2C_DATA1		0x28
#define LOTUS_I2C_TXF		0x10
#define LOTUS_I2C_RXF		0x14
#define LOTUS_I2C_CMD_BASE	0x40
#define LOTUS_I2C_LOOP1		0xc0
#define LOTUS_I2C_DST1		0xc4
#define LOTUS_I2C_LOOP2     	0xc8
#define LOTUS_I2C_DST2      	0xcc
#define LOTUS_I2C_TX_WATER	0x18
#define LOTUS_I2C_RX_WATER	0x1c
#define LOTUS_I2C_CTRL1		0x04
#define LOTUS_I2C_CTRL2		0x08
#define LOTUS_I2C_STAT		0x0c
#define LOTUS_I2C_INTR_RAW	0x30
#define LOTUS_I2C_INTR_EN	0x34
#define LOTUS_I2C_INTR_STAT	0x38

/* I2C Global Config Register -- LOTUS_I2C_GLB */
#define GLB_EN_MASK		BIT(0)
#define GLB_SDA_HOLD_MASK	GENMASK(19, 4)
#define GLB_SDA_HOLD_SHIFT	(4)
#define should_copy_to_continuous_mem(addr) true

/* I2C Timing CMD Register -- LOTUS_I2C_CMD_BASE + n * 4 (n = 0, 1, 2, ... 31) */
#define CMD_EXIT	0x0
#define CMD_TX_S	0x1
#define CMD_TX_D1_2	0x4
#define CMD_TX_D1_1	0x5
#define CMD_TX_FIFO	0x9
#define CMD_RX_FIFO	0x12
#define CMD_RX_ACK	0x13
#define CMD_IGN_ACK	0x15
#define CMD_TX_ACK	0x16
#define CMD_TX_NACK	0x17
#define CMD_JMP1	0x18
#define CMD_JMP2    	0x19
#define CMD_UP_TXF	0x1d
#define CMD_TX_RS	0x1e
#define CMD_TX_P	0x1f

/* I2C Control Register 1 -- LOTUS_I2C_CTRL1 */
#define CTRL1_CMD_START_MASK	BIT(0)
#define CTRL1_DMA_OP_MASK	(0x3 << 1)
#define CTRL1_DMA_R		(0x3 << 1)
#define CTRL1_DMA_W		(0x2 << 1)

/* I2C Status Register -- LOTUS_I2C_STAT */
#define STAT_RXF_NOE_MASK	BIT(0) /* RX FIFO not empty flag */
#define STAT_TXF_NOF_MASK	BIT(3) /* TX FIFO not full flag */

/*
 * I2C Interrupt status and mask Register --
 * LOTUS_I2C_INTR_STAT
 */
#define INTR_ABORT_MASK		(BIT(0) | BIT(3))
#define INTR_RX_MASK		BIT(5)
#define INTR_TX_MASK		BIT(6)
#define INTR_CMD_DONE_MASK	BIT(4)
#define INTR_USE_MASK		(INTR_ABORT_MASK \
				|INTR_RX_MASK \
				| INTR_TX_MASK \
				| INTR_CMD_DONE_MASK)
#define INTR_ALL_MASK		GENMASK(31, 0)

/*
 * I2C Interrupt raw status and mask Register --
 * LOTUS_I2C_INTR_RAW
 */
#define INTR_RAW_TX_MASK   BIT(6)
#define INTR_RAW_ALL_MASK		GENMASK(6, 0)

/*
 * I2C Interrupt EN status and mask Register --
 * LOTUS_I2C_INTR_EN
 */
#define INTR_EN_TX_MASK   BIT(6)
#define INTR_EN_ALL_MASK		GENMASK(6, 0)


#define I2C_DEFAULT_FREQUENCY	100000
#define I2C_TXF_DEPTH		64
#define I2C_RXF_DEPTH		64
#define I2C_TXF_WATER		32
#define I2C_RXF_WATER		0
#define I2C_WAIT_TIMEOUT	0x400
#define I2C_IRQ_TIMEOUT		(msecs_to_jiffies(1000))
#ifdef CONFIG_EDMAC
#define I2C_DMA_TIMEOUT		(msecs_to_jiffies(2000))
#endif

struct lotus_i2c_dev {
	struct device *dev;
	struct i2c_adapter adap;
	resource_size_t phybase;
	void __iomem *base;
	struct clk *clk;
	int irq;

	unsigned int		freq;
	struct i2c_msg *msg;
	unsigned int		msg_num;
	unsigned int		msg_idx;
	unsigned int		msg_buf_ptr;
	struct completion	msg_complete;
#ifdef CONFIG_EDMAC
	struct completion	dma_complete;
	struct dma_chan *tx_dma_chan;
	struct dma_chan *rx_dma_chan;
	unsigned int dma_buf_size;
	dma_addr_t dma_phys;
	void *dma_buf;
#endif
	spinlock_t		lock;
	int			status;
};

static inline void lotus_i2c_disable(const struct lotus_i2c_dev *i2c);
static inline void lotus_i2c_cfg_irq(const struct lotus_i2c_dev *i2c,
				     unsigned int flag);
static inline unsigned int lotus_i2c_clr_irq(const struct lotus_i2c_dev *i2c);
static inline void lotus_i2c_enable(const struct lotus_i2c_dev *i2c);

#define CHECK_SDA_IN_SHIFT     (5)
#define GPIO_MODE_SHIFT        (0)
#define FORCE_SCL_OEN_SHIFT    (1)
#define FORCE_SDA_OEN_SHIFT    (2)

static void lotus_i2c_rescue(const struct lotus_i2c_dev *i2c)
{
	unsigned int val;
	unsigned int time_cnt;
	int index;

	lotus_i2c_disable(i2c);
	lotus_i2c_cfg_irq(i2c, 0);
	lotus_i2c_clr_irq(i2c);

	val = (0x1 << GPIO_MODE_SHIFT) | (0x1 << FORCE_SCL_OEN_SHIFT) |
	      (0x1 << FORCE_SDA_OEN_SHIFT);
	writel(val, i2c->base + LOTUS_I2C_CTRL2);

	time_cnt = 0;
	do {
		for (index = 0; index < 9; index++) {
			val = (0x1 << GPIO_MODE_SHIFT) | (0x1 << FORCE_SDA_OEN_SHIFT);
			writel(val, i2c->base + LOTUS_I2C_CTRL2);
			udelay(5);

			val = (0x1 << GPIO_MODE_SHIFT) | (0x1 << FORCE_SCL_OEN_SHIFT) |
			      (0x1 << FORCE_SDA_OEN_SHIFT);
			writel(val, i2c->base + LOTUS_I2C_CTRL2);
			udelay(5);
		}

		time_cnt++;
		if (time_cnt > I2C_WAIT_TIMEOUT) {
			dev_err(i2c->dev, "wait Timeout!\n");
			goto disable_rescue;
		}
		val = readl(i2c->base + LOTUS_I2C_CTRL2);
	} while (!(val & (0x1 << CHECK_SDA_IN_SHIFT)));

	val = (0x1 << GPIO_MODE_SHIFT) | (0x1 << FORCE_SCL_OEN_SHIFT) |
	      (0x1 << FORCE_SDA_OEN_SHIFT);
	writel(val, i2c->base + LOTUS_I2C_CTRL2);

	val = (0x1 << GPIO_MODE_SHIFT) | (0x1 << FORCE_SCL_OEN_SHIFT);
	writel(val, i2c->base + LOTUS_I2C_CTRL2);

	udelay(10);

	val = (0x1 << GPIO_MODE_SHIFT) | (0x1 << FORCE_SCL_OEN_SHIFT) |
	      (0x1 << FORCE_SDA_OEN_SHIFT);
	writel(val, i2c->base + LOTUS_I2C_CTRL2);

disable_rescue:
	val = (0x1 << FORCE_SCL_OEN_SHIFT) | (0x1 << FORCE_SDA_OEN_SHIFT);
	writel(val, i2c->base + LOTUS_I2C_CTRL2);
}

static inline void lotus_i2c_disable(const struct lotus_i2c_dev *i2c)
{
	unsigned int val;

	val = readl(i2c->base + LOTUS_I2C_GLB);
	val &= ~GLB_EN_MASK;
	writel(val, i2c->base + LOTUS_I2C_GLB);
}

static inline void lotus_i2c_enable(const struct lotus_i2c_dev *i2c)
{
	unsigned int val;

	val = readl(i2c->base + LOTUS_I2C_GLB);
	val |= GLB_EN_MASK;
	writel(val, i2c->base + LOTUS_I2C_GLB);
}

static inline void lotus_i2c_cfg_irq(const struct lotus_i2c_dev *i2c,
				     unsigned int flag)
{
	writel(flag, i2c->base + LOTUS_I2C_INTR_EN);
}

static inline void lotus_i2c_irq_modify_changed(const struct lotus_i2c_dev *i2c,
						 unsigned int mask, unsigned int set)
{
	unsigned int val;

	val = readl(i2c->base + LOTUS_I2C_INTR_EN);
	val &= ~mask;
	val |= set;
	writel(val, i2c->base + LOTUS_I2C_INTR_EN);
}

static inline void lotus_i2c_disable_irq(const struct lotus_i2c_dev *i2c,
		unsigned int flag)
{
	unsigned int val;

	val = readl(i2c->base + LOTUS_I2C_INTR_EN);
	val &= ~flag;
	writel(val, i2c->base + LOTUS_I2C_INTR_EN);
}

static inline unsigned int lotus_i2c_clr_irq(const struct lotus_i2c_dev *i2c)
{
	unsigned int val;

	val = readl(i2c->base + LOTUS_I2C_INTR_STAT);
	writel(INTR_RAW_ALL_MASK, i2c->base + LOTUS_I2C_INTR_RAW);

	return val;
}

static inline void lotus_i2c_cmdreg_set(const struct lotus_i2c_dev *i2c,
					unsigned int cmd, unsigned int *offset)
{
	writel(cmd, i2c->base + LOTUS_I2C_CMD_BASE + *offset * 4);
	(*offset)++;
}

/* config i2c slave addr */
static inline void lotus_i2c_set_addr(const struct lotus_i2c_dev *i2c)
{
	struct i2c_msg *msg = i2c->msg;
	u16 addr;

	if (msg->flags & I2C_M_TEN) {
		/* First byte is 11110XX0 where XX is upper 2 bits */
		addr = ((msg->addr & 0x300) << 1) | 0xf000;
		if (msg->flags & I2C_M_RD)
			addr |= 1 << 8;

		/* Second byte is the remaining 8 bits */
		addr |= msg->addr & 0xff;
	} else {
		addr = (msg->addr & 0x7f) << 1;
		if (msg->flags & I2C_M_RD)
			addr |= 1;
	}

	writel(addr, i2c->base + LOTUS_I2C_DATA1);
}

/* Start command sequence */
static inline void lotus_i2c_start_cmd(const struct lotus_i2c_dev *i2c)
{
	unsigned int val;

	val = readl(i2c->base + LOTUS_I2C_CTRL1);
	val |= CTRL1_CMD_START_MASK;
	writel(val, i2c->base + LOTUS_I2C_CTRL1);
}

static int lotus_i2c_wait_rx_noempty(const struct lotus_i2c_dev *i2c)
{
	unsigned int time_cnt = 0;
	unsigned int val;

	do {
		val = readl(i2c->base + LOTUS_I2C_STAT);
		if (val & STAT_RXF_NOE_MASK)
			return 0;

		udelay(50);
	} while (time_cnt++ < I2C_WAIT_TIMEOUT);

	lotus_i2c_rescue(i2c);

	dev_err(i2c->dev, "wait rx no empty timeout, RIS: 0x%x, SR: 0x%x\n",
		readl(i2c->base + LOTUS_I2C_INTR_RAW), val);
	return -EIO;
}

static int lotus_i2c_wait_tx_nofull(const struct lotus_i2c_dev *i2c)
{
	unsigned int time_cnt = 0;
	unsigned int val;

	do {
		val = readl(i2c->base + LOTUS_I2C_STAT);
		if (val & STAT_TXF_NOF_MASK)
			return 0;

		udelay(50);
	} while (time_cnt++ < I2C_WAIT_TIMEOUT);

	lotus_i2c_rescue(i2c);

	dev_err(i2c->dev, "wait rx no empty timeout, RIS: 0x%x, SR: 0x%x\n",
		readl(i2c->base + LOTUS_I2C_INTR_RAW), val);
	return -EIO;
}

static int lotus_i2c_wait_idle(const struct lotus_i2c_dev *i2c)
{
	unsigned int time_cnt = 0;
	unsigned int val;

	do {
		val = readl(i2c->base + LOTUS_I2C_INTR_RAW);
		if (val & (INTR_ABORT_MASK)) {
			dev_err(i2c->dev, "wait idle abort!, RIS: 0x%x\n",
				val);
			return -EIO;
		}

		if (val & INTR_CMD_DONE_MASK)
			return 0;

		udelay(50);
	} while (time_cnt++ < I2C_WAIT_TIMEOUT);

	lotus_i2c_rescue(i2c);

	dev_err(i2c->dev, "wait idle timeout, RIS: 0x%x, SR: 0x%x\n",
		val, readl(i2c->base + LOTUS_I2C_STAT));

	return -EIO;
}

static void lotus_i2c_set_freq(struct lotus_i2c_dev *i2c)
{
	unsigned int max_freq, freq;
	unsigned int clk_rate;
	unsigned int val;

	freq = i2c->freq;
	clk_rate = clk_get_rate(i2c->clk);
	max_freq = clk_rate >> 1;

	if (freq > max_freq) {
		i2c->freq = max_freq;
		freq = i2c->freq;
	}

	if (!freq) {
		dev_err(i2c->dev, "freq can't be zero!\n");
		return;
	}

	if (freq <= 100000) {
		/* in normal mode               F_scl: freq
		   i2c_scl_hcnt = (F_i2c / F_scl) * 0.5
		   i2c_scl_hcnt = (F_i2c / F_scl) * 0.5
		 */
		val = clk_rate / (freq * 2);
		writel(val, i2c->base + LOTUS_I2C_SCL_H);
		writel(val, i2c->base + LOTUS_I2C_SCL_L);
	} else {
		/* in fast mode         F_scl: freq
		   i2c_scl_hcnt = (F_i2c / F_scl) * 0.36
		   i2c_scl_hcnt = (F_i2c / F_scl) * 0.64
		 */
		val = ((clk_rate / 100) * 36) / freq;
		writel(val, i2c->base + LOTUS_I2C_SCL_H);
		val = ((clk_rate / 100) * 64) / freq;
		writel(val, i2c->base + LOTUS_I2C_SCL_L);
	}

	val = readl(i2c->base + LOTUS_I2C_GLB);
	val &= ~GLB_SDA_HOLD_MASK;
	val |= ((0xa << GLB_SDA_HOLD_SHIFT) & GLB_SDA_HOLD_MASK);
	writel(val, i2c->base + LOTUS_I2C_GLB);
}

/* set i2c controller TX and RX FIFO water */
static inline void lotus_i2c_set_water(const struct lotus_i2c_dev *i2c)
{
	writel(I2C_TXF_WATER, i2c->base + LOTUS_I2C_TX_WATER);
	writel(I2C_RXF_WATER, i2c->base + LOTUS_I2C_RX_WATER);
}

/* initialise the controller, set i2c bus interface freq */
static void lotus_i2c_hw_init(struct lotus_i2c_dev *i2c)
{
	lotus_i2c_disable(i2c);
	lotus_i2c_disable_irq(i2c, INTR_ALL_MASK);
	lotus_i2c_set_freq(i2c);
	lotus_i2c_set_water(i2c);
}

/*
 * lotus_i2c_cfg_cmd - config i2c controller command sequence
 *
 * After all the timing command is configured,
 * and then start the command, you can i2c communication,
 * and then only need to read and write i2c fifo.
 */
static void lotus_i2c_cfg_cmd(const struct lotus_i2c_dev *i2c)
{
	struct i2c_msg *msg = i2c->msg;
	int offset = 0;

	if (i2c->msg_idx == 0)
		lotus_i2c_cmdreg_set(i2c, CMD_TX_S, &offset);
	else
		lotus_i2c_cmdreg_set(i2c, CMD_TX_RS, &offset);

	if (msg->flags & I2C_M_TEN) {
		if (i2c->msg_idx == 0) {
			lotus_i2c_cmdreg_set(i2c, CMD_TX_D1_2, &offset);
			lotus_i2c_cmdreg_set(i2c, CMD_RX_ACK, &offset);
			lotus_i2c_cmdreg_set(i2c, CMD_TX_D1_1, &offset);
		} else {
			lotus_i2c_cmdreg_set(i2c, CMD_TX_D1_2, &offset);
		}
	} else {
		lotus_i2c_cmdreg_set(i2c, CMD_TX_D1_1, &offset);
	}

	if (msg->flags & I2C_M_IGNORE_NAK)
		lotus_i2c_cmdreg_set(i2c, CMD_IGN_ACK, &offset);
	else
		lotus_i2c_cmdreg_set(i2c, CMD_RX_ACK, &offset);

	if (msg->flags & I2C_M_RD) {
		if (msg->len >= 2) {
			writel(offset, i2c->base + LOTUS_I2C_DST1);
			writel(msg->len - 2, i2c->base + LOTUS_I2C_LOOP1);
			lotus_i2c_cmdreg_set(i2c, CMD_RX_FIFO, &offset);
			lotus_i2c_cmdreg_set(i2c, CMD_TX_ACK, &offset);
			lotus_i2c_cmdreg_set(i2c, CMD_JMP1, &offset);
		}
		lotus_i2c_cmdreg_set(i2c, CMD_RX_FIFO, &offset);
		lotus_i2c_cmdreg_set(i2c, CMD_TX_NACK, &offset);
	} else {
		writel(offset, i2c->base + LOTUS_I2C_DST1);
		writel(msg->len - 1, i2c->base + LOTUS_I2C_LOOP1);
		lotus_i2c_cmdreg_set(i2c, CMD_UP_TXF, &offset);
		lotus_i2c_cmdreg_set(i2c, CMD_TX_FIFO, &offset);

		if (msg->flags & I2C_M_IGNORE_NAK)
			lotus_i2c_cmdreg_set(i2c, CMD_IGN_ACK, &offset);
		else
			lotus_i2c_cmdreg_set(i2c, CMD_RX_ACK, &offset);

		lotus_i2c_cmdreg_set(i2c, CMD_JMP1, &offset);
	}

	if ((i2c->msg_idx == (i2c->msg_num - 1)) || (msg->flags & I2C_M_STOP)) {
		dev_dbg(i2c->dev, "run to %s %d...TX STOP\n",
			__func__, __LINE__);
		lotus_i2c_cmdreg_set(i2c, CMD_TX_P, &offset);
	}

	lotus_i2c_cmdreg_set(i2c, CMD_EXIT, &offset);
}

static inline void check_i2c_send_complete(struct lotus_i2c_dev *i2c)
{
	unsigned int val;
	val = readl(i2c->base + LOTUS_I2C_GLB);
	if (val & GLB_EN_MASK) {
		lotus_i2c_wait_idle(i2c);
		lotus_i2c_disable(i2c);
	}
}

#ifdef CONFIG_EDMAC
static void lotus_i2c_dma_complete(void *args)
{
	struct lotus_i2c_dev *i2c_dev = args;

	i2c_dev->status = 0;
	complete(&i2c_dev->dma_complete);
}

int dma_to_i2c(struct lotus_i2c_dev *i2c, dma_addr_t dst,
	       unsigned int length)
{
	struct dma_async_tx_descriptor *dma_desc;

	dev_dbg(i2c->dev, "starting DMA for length: %zu\n", length);

	reinit_completion(&i2c->dma_complete);

	dma_desc = dmaengine_prep_slave_single(i2c->tx_dma_chan, dst,
						   length, DMA_MEM_TO_DEV, DMA_PREP_INTERRUPT |
						   DMA_CTRL_ACK);
	if (!dma_desc) {
		dev_err(i2c->dev, "failed to get TX DMA descriptor\n");
		return -EINVAL;
	}

	dma_desc->callback = lotus_i2c_dma_complete;
	dma_desc->callback_param = i2c;

	dmaengine_submit(dma_desc);
	dma_async_issue_pending(i2c->tx_dma_chan);

	return 0;
}

int i2c_to_dma(struct lotus_i2c_dev *i2c, dma_addr_t dst, unsigned int length)
{
	struct dma_async_tx_descriptor *dma_desc;

	dev_dbg(i2c->dev, "starting DMA for length: %zu\n", length);

	reinit_completion(&i2c->dma_complete);

	dma_desc = dmaengine_prep_slave_single(i2c->rx_dma_chan, dst,
						   length, DMA_DEV_TO_MEM, DMA_PREP_INTERRUPT |
						   DMA_CTRL_ACK);
	if (!dma_desc) {
		dev_err(i2c->dev, "failed to get RX DMA descriptor\n");
		return -EINVAL;
	}

	dma_desc->callback = lotus_i2c_dma_complete;
	dma_desc->callback_param = i2c;

	dmaengine_submit(dma_desc);
	dma_async_issue_pending(i2c->rx_dma_chan);

	return 0;
}

static int lotus_i2c_do_dma_write(struct lotus_i2c_dev *i2c, dma_addr_t dma_dst_addr)
{
	unsigned long timeout = 1;
	int chan, val;
	int status = 0;
	struct i2c_msg *msg = i2c->msg;

	check_i2c_send_complete(i2c);
	lotus_i2c_set_freq(i2c);
	writel(0x1, i2c->base + LOTUS_I2C_TX_WATER);
	lotus_i2c_enable(i2c);
	lotus_i2c_clr_irq(i2c);
	lotus_i2c_set_addr(i2c);
	lotus_i2c_cfg_cmd(i2c);

	/*  transmit DATA from DMAC to I2C in DMA mode */
	chan = dma_to_i2c(i2c, dma_dst_addr, msg->len);
	if (chan == -1) {
		status = -1;
		goto fail_0;
	}

	val = readl(i2c->base + LOTUS_I2C_CTRL1);
	val &= ~CTRL1_DMA_OP_MASK;
	val |= CTRL1_DMA_W | CTRL1_CMD_START_MASK;
	writel(val, i2c->base + LOTUS_I2C_CTRL1);

	timeout = wait_for_completion_timeout(&i2c->dma_complete, I2C_DMA_TIMEOUT);

	if (timeout == 0) {
		lotus_i2c_disable_irq(i2c, INTR_ALL_MASK);
		status = -EIO;
		dev_err(i2c->dev, "%s timeout\n", msg->flags & I2C_M_RD ? "rx" : "tx");
	} else {
		status = i2c->status;
	}

	status = lotus_i2c_wait_idle(i2c);

fail_0:
	lotus_i2c_disable(i2c);

	return status;
}

static int lotus_i2c_do_dma_read(struct lotus_i2c_dev *i2c, dma_addr_t dma_dst_addr)
{
	unsigned long timeout = 1;
	int val, chan;
	int status = 0;
	struct i2c_msg *msg = i2c->msg;

	check_i2c_send_complete(i2c);
	lotus_i2c_set_freq(i2c);
	writel(0x0, i2c->base + LOTUS_I2C_RX_WATER);
	lotus_i2c_enable(i2c);
	lotus_i2c_clr_irq(i2c);
	lotus_i2c_set_addr(i2c);
	lotus_i2c_cfg_cmd(i2c);

	/* transmit DATA from I2C to DMAC in DMA mode */
	chan = i2c_to_dma(i2c, dma_dst_addr, msg->len);
	if (chan == -1) {
		status = -1;
		goto fail_0;
	}

	val = readl(i2c->base + LOTUS_I2C_CTRL1);
	val &= ~CTRL1_DMA_OP_MASK;
	val |= CTRL1_CMD_START_MASK | CTRL1_DMA_R;
	writel(val, i2c->base + LOTUS_I2C_CTRL1);

	timeout = wait_for_completion_timeout(&i2c->dma_complete,
						  I2C_DMA_TIMEOUT);

	if (timeout == 0) {
		lotus_i2c_disable_irq(i2c, INTR_ALL_MASK);
		status = -EIO;
		dev_err(i2c->dev, "%s timeout\n", msg->flags & I2C_M_RD ? "rx" : "tx");
	} else {
		status = i2c->status;
	}

	status = lotus_i2c_wait_idle(i2c);

fail_0:
	lotus_i2c_disable(i2c);

	return status;
}

static int lotus_i2c_dma_xfer_one_msg(struct lotus_i2c_dev *i2c)
{
	unsigned int status;
	struct i2c_msg *msg = i2c->msg;
	dma_addr_t dma_dst_addr;

	if (msg->flags & I2C_M_RD) {
		dma_dst_addr = dma_map_single(i2c->dev, msg->buf,
					      msg->len, DMA_FROM_DEVICE);
		status = dma_mapping_error(i2c->dev, dma_dst_addr);
		if (status) {
			dev_err(i2c->dev, "DMA mapping failed\n");
			return status;
		}

		status = lotus_i2c_do_dma_read(i2c, dma_dst_addr);
		dma_unmap_single(i2c->dev, dma_dst_addr, msg->len, DMA_FROM_DEVICE);
	} else {
		dma_dst_addr = dma_map_single(i2c->dev, msg->buf,
					      msg->len, DMA_TO_DEVICE);
		status = dma_mapping_error(i2c->dev, dma_dst_addr);
		if (status) {
			dev_err(i2c->dev, "DMA mapping failed\n");
			return status;
		}

		status = lotus_i2c_do_dma_write(i2c, dma_dst_addr);
		dma_unmap_single(i2c->dev, dma_dst_addr, msg->len, DMA_TO_DEVICE);
	}

	return status;
}

static void lotus_i2c_release_dma(struct lotus_i2c_dev *i2c_dev)
{
	if (i2c_dev->dma_buf) {
		dma_free_coherent(i2c_dev->dev, i2c_dev->dma_buf_size,
				  i2c_dev->dma_buf, i2c_dev->dma_phys);
		i2c_dev->dma_buf = NULL;
	}

	if (i2c_dev->tx_dma_chan) {
		dma_release_channel(i2c_dev->tx_dma_chan);
		i2c_dev->tx_dma_chan = NULL;
	}

	if (i2c_dev->rx_dma_chan) {
		dma_release_channel(i2c_dev->rx_dma_chan);
		i2c_dev->rx_dma_chan = NULL;
	}
}

static int lotus_i2c_dma_probe(struct lotus_i2c_dev *i2c_dev)
{
	struct dma_chan *chan;
	dma_addr_t dma_phys;
	struct dma_slave_config rx_conf;
	struct dma_slave_config tx_conf;
	u32 *dma_buf;
	int err;
	int ret;

	chan = dma_request_chan(i2c_dev->dev, "rx");
	if (IS_ERR(chan)) {
		err = PTR_ERR(chan);
		goto err_out;
	}

	i2c_dev->rx_dma_chan = chan;

	rx_conf.src_addr = i2c_dev->phybase + LOTUS_I2C_RXF;
	rx_conf.src_addr_width = DMA_SLAVE_BUSWIDTH_1_BYTE;
	rx_conf.src_maxburst = 1;
	rx_conf.direction = DMA_DEV_TO_MEM;
	ret = dmaengine_slave_config(i2c_dev->rx_dma_chan, &rx_conf);
	if (ret < 0) {
		dev_err(i2c_dev->dev, "can't configure tx channel (%d)\n", ret);
		goto err_out;
	}

	chan = dma_request_chan(i2c_dev->dev, "tx");
	if (IS_ERR(chan)) {
		err = PTR_ERR(chan);
		goto err_out;
	}

	i2c_dev->tx_dma_chan = chan;

	tx_conf.dst_addr = i2c_dev->phybase + LOTUS_I2C_TXF;
	tx_conf.dst_addr_width = DMA_SLAVE_BUSWIDTH_1_BYTE;
	tx_conf.dst_maxburst = 1;
	tx_conf.direction = DMA_MEM_TO_DEV;
	ret = dmaengine_slave_config(i2c_dev->tx_dma_chan, &tx_conf);
	if (ret < 0) {
		dev_err(i2c_dev->dev, "can't configure tx channel (%d)\n", ret);
		goto err_out;
	}

	dma_buf = dma_alloc_coherent(i2c_dev->dev, CONFIG_DMA_MSG_MAX_LEN,
				     &dma_phys, GFP_KERNEL | __GFP_NOWARN);
	if (!dma_buf) {
		dev_err(i2c_dev->dev, "failed to allocate DMA buffer\n");
		err = -ENOMEM;
		goto err_out;
	}

	i2c_dev->dma_buf = dma_buf;
	i2c_dev->dma_phys = dma_phys;

	init_completion(&i2c_dev->dma_complete);

	return 0;

err_out:
	lotus_i2c_release_dma(i2c_dev);
	if (err != -EPROBE_DEFER) {
		dev_err(i2c_dev->dev, "cannot use DMA: %d\n", err);
		dev_err(i2c_dev->dev, "falling back to PIO\n");
		return 0;
	}

	return err;
}
#endif

static int lotus_i2c_polling_xfer_one_msg(struct lotus_i2c_dev *i2c)
{
	int status;
	unsigned int val;
	struct i2c_msg *msg = i2c->msg;

	check_i2c_send_complete(i2c);
	lotus_i2c_enable(i2c);
	lotus_i2c_clr_irq(i2c);
	lotus_i2c_set_addr(i2c);
	lotus_i2c_cfg_cmd(i2c);
	lotus_i2c_start_cmd(i2c);

	i2c->msg_buf_ptr = 0;
	if (msg->flags & I2C_M_RD) {
		while (i2c->msg_buf_ptr < msg->len) {
			status = lotus_i2c_wait_rx_noempty(i2c);
			if (status)
				goto end;

			val = readl(i2c->base + LOTUS_I2C_RXF);
			msg->buf[i2c->msg_buf_ptr] = val;
			i2c->msg_buf_ptr++;
		}
	} else {
		while (i2c->msg_buf_ptr < msg->len) {
			status = lotus_i2c_wait_tx_nofull(i2c);
			if (status)
				goto end;

			val = msg->buf[i2c->msg_buf_ptr];
			writel(val, i2c->base + LOTUS_I2C_TXF);
			i2c->msg_buf_ptr++;
		}
	}

	status = lotus_i2c_wait_idle(i2c);
end:
	lotus_i2c_disable(i2c);

	return status;
}

static irqreturn_t lotus_i2c_isr(int irq, void *dev_id)
{
	struct lotus_i2c_dev *i2c = dev_id;
	unsigned int irq_status;
	struct i2c_msg *msg = i2c->msg;

	spin_lock(&i2c->lock);

	irq_status = lotus_i2c_clr_irq(i2c);
	if (irq_status & INTR_ABORT_MASK) {
		dev_err(i2c->dev, "irq handle abort, RIS: 0x%x\n", irq_status);
		i2c->status = -EIO;
		lotus_i2c_disable_irq(i2c, INTR_ALL_MASK);
		complete(&i2c->msg_complete);
		goto end;
	}

	if (irq_status & INTR_RX_MASK) {
		while ((readl(i2c->base + LOTUS_I2C_STAT) & STAT_RXF_NOE_MASK)
				&& (i2c->msg_buf_ptr < msg->len)) {
			msg->buf[i2c->msg_buf_ptr] =
				readl(i2c->base + LOTUS_I2C_RXF);
			i2c->msg_buf_ptr++;
		}
	} else {
		if ((irq_status == 0) && ((readl(i2c->base + LOTUS_I2C_INTR_RAW)) & INTR_RAW_TX_MASK) && \
			((readl(i2c->base + LOTUS_I2C_INTR_EN)) & INTR_EN_TX_MASK)) {
			while ((readl(i2c->base + LOTUS_I2C_STAT) & STAT_TXF_NOF_MASK)
					&& (i2c->msg_buf_ptr < msg->len)) {
				writel(msg->buf[i2c->msg_buf_ptr],
				       i2c->base + LOTUS_I2C_TXF);
				i2c->msg_buf_ptr++;
			}
		}
	}

	if (i2c->msg_buf_ptr == msg->len) {
		lotus_i2c_irq_modify_changed(i2c, INTR_ALL_MASK, INTR_CMD_DONE_MASK);
	}

	if (irq_status & INTR_CMD_DONE_MASK) {
		dev_dbg(i2c->dev, "cmd done\n");
		i2c->status =  0;
		lotus_i2c_disable_irq(i2c, INTR_ALL_MASK);

		complete(&i2c->msg_complete);
	}

end:
	spin_unlock(&i2c->lock);

	return IRQ_HANDLED;
}

static int lotus_i2c_interrupt_xfer_one_msg(struct lotus_i2c_dev *i2c)
{
	int status;
	struct i2c_msg *msg = i2c->msg;
	unsigned long timeout;
	unsigned long flags;

	reinit_completion(&i2c->msg_complete);
	i2c->msg_buf_ptr = 0;
	i2c->status = -EIO;

	spin_lock_irqsave(&i2c->lock, flags);
	check_i2c_send_complete(i2c);
	lotus_i2c_enable(i2c);
	lotus_i2c_clr_irq(i2c);

	if (msg->flags & I2C_M_RD) {
		lotus_i2c_cfg_irq(i2c, INTR_USE_MASK & ~INTR_TX_MASK);
	} else {
		lotus_i2c_cfg_irq(i2c, INTR_USE_MASK & ~(INTR_RX_MASK | INTR_CMD_DONE_MASK));
	}
	lotus_i2c_set_addr(i2c);
	lotus_i2c_cfg_cmd(i2c);
	lotus_i2c_start_cmd(i2c);
	spin_unlock_irqrestore(&i2c->lock, flags);

	timeout = wait_for_completion_timeout(&i2c->msg_complete,
					      I2C_IRQ_TIMEOUT);

	spin_lock_irqsave(&i2c->lock, flags);
	if (timeout == 0) {
		lotus_i2c_disable_irq(i2c, INTR_ALL_MASK);
		status = -EIO;
		dev_err(i2c->dev, "%s timeout\n", msg->flags & I2C_M_RD ? "rx" : "tx");
	} else {
		status = i2c->status;
	}

	lotus_i2c_disable(i2c);
	spin_unlock_irqrestore(&i2c->lock, flags);

	return status;
}

/* Master transfer function */
static int lotus_i2c_xfer_common(struct i2c_adapter *adap, struct i2c_msg *msgs, int num)
{
	struct lotus_i2c_dev *i2c = i2c_get_adapdata(adap);
	int status = -EINVAL;
	unsigned long flags;

	if (!msgs || (num <= 0)) {
		dev_err(i2c->dev, "msgs == NULL || num <= 0, Invalid argument!\n");
		return -EINVAL;
	}

	spin_lock_irqsave(&i2c->lock, flags);

	i2c->msg = msgs;
	i2c->msg_num = num;
	i2c->msg_idx = 0;

	while (i2c->msg_idx < i2c->msg_num) {
		if (i2c->irq >= 0) {
			spin_unlock_irqrestore(&i2c->lock, flags);
#ifdef CONFIG_EDMAC
			if ((i2c->msg->len >= CONFIG_DMA_MSG_MIN_LEN) &&
					(i2c->msg->len <= CONFIG_DMA_MSG_MAX_LEN) && i2c->rx_dma_chan && i2c->tx_dma_chan) {
				status = lotus_i2c_dma_xfer_one_msg(i2c);
			} else
#endif
			{
				if (i2c->msg->flags & I2C_M_RD) {
					status = lotus_i2c_interrupt_xfer_one_msg(i2c);
				} else {
					status = lotus_i2c_polling_xfer_one_msg(i2c);
				}
			}
			spin_lock_irqsave(&i2c->lock, flags);
			if (status)
				break;
		} else {
			status = lotus_i2c_polling_xfer_one_msg(i2c);
			if (status)
				break;
		}
		i2c->msg++;
		i2c->msg_idx++;
	}

	if (!status || i2c->msg_idx > 0)
		status = i2c->msg_idx;

	spin_unlock_irqrestore(&i2c->lock, flags);

	return status;
}

/*
 * lotus_i2c_break_polling_xfer
 * I2c polling independent branch, Shielding interrupt interface
 */
static int lotus_i2c_break_polling_xfer(struct i2c_adapter *adap,
					struct i2c_msg *msgs, int num)
{
	struct lotus_i2c_dev *i2c = i2c_get_adapdata(adap);
	int status = -EINVAL;
	unsigned long flags;

	if (!msgs || (num <= 0)) {
		dev_err(i2c->dev, "msgs == NULL || num <= 0, Invalid argument!\n");
		return -EINVAL;
	}

	spin_lock_irqsave(&i2c->lock, flags);
	i2c->msg = msgs;
	i2c->msg_num = num;
	i2c->msg_idx = 0;
	while (i2c->msg_idx < i2c->msg_num) {
		status = lotus_i2c_polling_xfer_one_msg(i2c);
		if (status)
			break;

		i2c->msg++;
		i2c->msg_idx++;
	}
	if (!status || i2c->msg_idx > 0)
		status = i2c->msg_idx;
	spin_unlock_irqrestore(&i2c->lock, flags);

	return status;
}

static int lotus_i2c_xfer(struct i2c_adapter *adap, struct i2c_msg *msgs, int num)
{
	int status = -EINVAL;

	if (in_interrupt())
		status = lotus_i2c_break_polling_xfer(adap, msgs, num);
	else
		status = lotus_i2c_xfer_common(adap, msgs, num);

	return status;
}

static u32 lotus_i2c_func(struct i2c_adapter *adap)
{
	return I2C_FUNC_I2C | I2C_FUNC_10BIT_ADDR |
	       I2C_FUNC_PROTOCOL_MANGLING |
	       I2C_FUNC_SMBUS_WORD_DATA |
	       I2C_FUNC_SMBUS_BYTE_DATA |
	       I2C_FUNC_SMBUS_BYTE |
	       I2C_FUNC_SMBUS_I2C_BLOCK;
}

static const struct i2c_algorithm lotus_i2c_algo = {
	.master_xfer		= lotus_i2c_xfer,
	.functionality		= lotus_i2c_func,
};

static int lotus_i2c_probe(struct platform_device *pdev)
{
	int status;
	struct lotus_i2c_dev *i2c;
	struct i2c_adapter *adap = NULL;
	struct resource *res = NULL;

	i2c = devm_kzalloc(&pdev->dev, sizeof(*i2c), GFP_KERNEL);
	if (!i2c)
		return -ENOMEM;

	platform_set_drvdata(pdev, i2c);
	i2c->dev = &pdev->dev;
	spin_lock_init(&i2c->lock);
	init_completion(&i2c->msg_complete);

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res) {
		dev_err(i2c->dev, "Invalid mem resource./n");
		return -ENODEV;
	}

	i2c->phybase = res->start;
	i2c->base = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(i2c->base)) {
		dev_err(i2c->dev, "cannot ioremap resource\n");
		return -ENOMEM;
	}

	i2c->clk = devm_clk_get(&pdev->dev, NULL);
	if (IS_ERR(i2c->clk)) {
		dev_err(i2c->dev, "cannot get clock\n");
		return -ENOENT;
	}
	clk_prepare_enable(i2c->clk);

	if (of_property_read_u32(pdev->dev.of_node, "clock-frequency",
				 &i2c->freq)) {
		dev_warn(i2c->dev, "setting default clock-frequency@%dHz\n",
			 I2C_DEFAULT_FREQUENCY);
		i2c->freq = I2C_DEFAULT_FREQUENCY;
	}

	/* i2c controller initialization, disable interrupt */
	lotus_i2c_hw_init(i2c);

	i2c->irq = platform_get_irq(pdev, 0);
	status = devm_request_irq(&pdev->dev, i2c->irq, lotus_i2c_isr,
				  IRQF_SHARED, dev_name(&pdev->dev), i2c);
	if (status) {
		dev_dbg(i2c->dev, "falling back to polling mode");
		i2c->irq = -1;
	}

	adap = &i2c->adap;
	i2c_set_adapdata(adap, i2c);
	adap->owner = THIS_MODULE;
	strlcpy(adap->name, "lotus-i2c", sizeof(adap->name));
	adap->dev.parent = &pdev->dev;
	adap->dev.of_node = pdev->dev.of_node;
	adap->algo = &lotus_i2c_algo;

	/* Add the i2c adapter */
	status = i2c_add_adapter(adap);
	if (status) {
		dev_err(i2c->dev, "failed to add bus to i2c core\n");
		goto err_add_adapter;
	}
#ifdef CONFIG_EDMAC
	/* Get DMA channels, try autoconfiguration first */
	status = lotus_i2c_dma_probe(i2c);
	if (status == -EPROBE_DEFER) {
		dev_dbg(i2c->dev, "deferring probe to get DMA channel\n");
		goto err_add_adapter;
	}
#endif
	dev_info(i2c->dev, "%s%d@%dhz registered\n",
		 adap->name, adap->nr, i2c->freq);

	return 0;

err_add_adapter:
	clk_disable_unprepare(i2c->clk);
	return status;
}

static int lotus_i2c_remove(struct platform_device *pdev)
{
	struct lotus_i2c_dev *i2c = platform_get_drvdata(pdev);
	if (i2c == NULL) {
		dev_err(i2c->dev, "i2c remove err!!!\n");
		return 0;
	}
	clk_disable_unprepare(i2c->clk);
	i2c_del_adapter(&i2c->adap);
#ifdef CONFIG_EDMAC
	lotus_i2c_release_dma(i2c);
#endif
	return 0;
}

#ifdef CONFIG_PM_SLEEP
static int lotus_i2c_suspend(struct device *dev)
{
	struct lotus_i2c_dev *i2c = dev_get_drvdata(dev);

	i2c_lock_bus(&i2c->adap, I2C_LOCK_ROOT_ADAPTER);
	clk_disable_unprepare(i2c->clk);
	i2c_unlock_bus(&i2c->adap, I2C_LOCK_ROOT_ADAPTER);

	return 0;
}

static int lotus_i2c_resume(struct device *dev)
{
	struct lotus_i2c_dev *i2c = dev_get_drvdata(dev);

	i2c_lock_bus(&i2c->adap, I2C_LOCK_ROOT_ADAPTER);
	clk_prepare_enable(i2c->clk);
	lotus_i2c_hw_init(i2c);
	i2c_unlock_bus(&i2c->adap, I2C_LOCK_ROOT_ADAPTER);

	return 0;
}
#endif

static SIMPLE_DEV_PM_OPS(lotus_i2c_dev_pm, lotus_i2c_suspend,
			 lotus_i2c_resume);

static const struct of_device_id lotus_i2c_match[] = {
	{ .compatible = "lotus,lotus-i2c" },
	{},
};
MODULE_DEVICE_TABLE(of, lotus_i2c_match);

static struct platform_driver lotus_i2c_driver = {
	.driver		= {
		.name	= "lotus-i2c",
		.of_match_table = lotus_i2c_match,
		.pm	= &lotus_i2c_dev_pm,
	},
	.probe		= lotus_i2c_probe,
	.remove		= lotus_i2c_remove,
};

module_platform_driver(lotus_i2c_driver);

MODULE_AUTHOR("Lotus");
MODULE_DESCRIPTION("LOTUS IPC I2C Bus driver");
MODULE_LICENSE("GPL v2");
