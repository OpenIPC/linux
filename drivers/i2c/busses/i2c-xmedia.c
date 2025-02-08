/*
 * Copyright (c) XMEDIA. All rights reserved.
 */
#include <linux/slab.h>
#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/i2c.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/dma-mapping.h>
#include <asm/string.h>

#if defined(CONFIG_EDMAC)
#include <linux/edmac.h>
#endif

/*
 * I2C Registers offsets
 */
#define XMEDIA_I2C_GLB		0x0
#define XMEDIA_I2C_SCL_H		0x4
#define XMEDIA_I2C_SCL_L		0x8
#define XMEDIA_I2C_DATA1		0x10
#define XMEDIA_I2C_TXF		0x20
#define XMEDIA_I2C_RXF		0x24
#define XMEDIA_I2C_CMD_BASE	0x30
#define XMEDIA_I2C_LOOP1		0xb0
#define XMEDIA_I2C_DST1		0xb4
#define XMEDIA_I2C_LOOP2     0xb8
#define XMEDIA_I2C_DST2      0xbc
#define XMEDIA_I2C_TX_WATER	0xc8
#define XMEDIA_I2C_RX_WATER	0xcc
#define XMEDIA_I2C_CTRL1		0xd0
#define XMEDIA_I2C_CTRL2		0xd4
#define XMEDIA_I2C_STAT		0xd8
#define XMEDIA_I2C_INTR_RAW	0xe0
#define XMEDIA_I2C_INTR_EN	0xe4
#define XMEDIA_I2C_INTR_STAT	0xe8

/*
 * I2C Global Config Register -- XMEDIA_I2C_GLB
 */
#define GLB_EN_MASK		BIT(0)
#define GLB_SDA_HOLD_MASK	GENMASK(23, 8)
#define GLB_SDA_HOLD_SHIFT	(8)
#define should_copy_to_continuous_mem(addr) true

/*
 * I2C Timing CMD Register -- XMEDIA_I2C_CMD_BASE + n * 4 (n = 0, 1, 2, ... 31)
 */
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
#define CMD_JMP2    0x19
#define CMD_UP_TXF	0x1d
#define CMD_TX_RS	0x1e
#define CMD_TX_P	0x1f

/*
 * I2C Control Register 1 -- XMEDIA_I2C_CTRL1
 */
#define CTRL1_CMD_START_MASK	BIT(0)
#define CTRL1_DMA_OP_MASK	(0x3 << 8)
#define CTRL1_DMA_R		(0x3 << 8)
#define CTRL1_DMA_W		(0x2 << 8)

/*
 * I2C Status Register -- XMEDIA_I2C_STAT
 */
#define STAT_RXF_NOE_MASK	BIT(16) /* RX FIFO not empty flag */
#define STAT_TXF_NOF_MASK	BIT(19) /* TX FIFO not full flag */

/*
 * I2C Interrupt status and mask Register --
 * XMEDIA_I2C_INTR_RAW, XMEDIA_I2C_STAT, XMEDIA_I2C_INTR_STAT
 */
#define INTR_ABORT_MASK		(BIT(0) | BIT(11))
#define INTR_RX_MASK		BIT(2)
#define INTR_TX_MASK		BIT(4)
#define INTR_CMD_DONE_MASK	BIT(12)
#define INTR_USE_MASK		(INTR_ABORT_MASK \
				|INTR_RX_MASK \
				| INTR_TX_MASK \
				| INTR_CMD_DONE_MASK)
#define INTR_ALL_MASK		GENMASK(31, 0)

#define I2C_DEFAULT_FREQUENCY	100000
#define I2C_TXF_DEPTH		64
#define I2C_RXF_DEPTH		64
#define I2C_TXF_WATER		32
#define I2C_RXF_WATER		32
#define I2C_WAIT_TIMEOUT	0x400
#define I2C_IRQ_TIMEOUT		(msecs_to_jiffies(1000))

struct xmedia_i2c_dev {
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

	spinlock_t		lock;
	int			status;
};
static inline void xmedia_i2c_disable(const struct xmedia_i2c_dev *i2c);
static inline void xmedia_i2c_cfg_irq(const struct xmedia_i2c_dev *i2c,
				     unsigned int flag);
static inline unsigned int xmedia_i2c_clr_irq(const struct xmedia_i2c_dev *i2c);
static inline void xmedia_i2c_enable(const struct xmedia_i2c_dev *i2c);

#define CHECK_SDA_IN_SHIFT     (16)
#define GPIO_MODE_SHIFT        (8)
#define FORCE_SCL_OEN_SHIFT    (4)
#define FORCE_SDA_OEN_SHIFT    (0)

static void xmedia_i2c_rescue(const struct xmedia_i2c_dev *i2c)
{
	unsigned int val;
	unsigned int time_cnt;
	int index;

	xmedia_i2c_disable(i2c);
	xmedia_i2c_cfg_irq(i2c, 0);
	xmedia_i2c_clr_irq(i2c);

	val = (0x1 << GPIO_MODE_SHIFT) | (0x1 << FORCE_SCL_OEN_SHIFT) |
	      (0x1 << FORCE_SDA_OEN_SHIFT);
	writel(val, i2c->base + XMEDIA_I2C_CTRL2);

	time_cnt = 0;
	do {
		for (index = 0; index < 9; index++) {
			val = (0x1 << GPIO_MODE_SHIFT) | 0x1;
			writel(val, i2c->base + XMEDIA_I2C_CTRL2);

			udelay(5);

			val = (0x1 << GPIO_MODE_SHIFT) | (0x1 << FORCE_SCL_OEN_SHIFT) |
			      (0x1 << FORCE_SDA_OEN_SHIFT);
			writel(val, i2c->base + XMEDIA_I2C_CTRL2);

			udelay(5);
		}

		time_cnt++;
		if (time_cnt > I2C_WAIT_TIMEOUT) {
			dev_err(i2c->dev, "wait Timeout!\n");
			goto disable_rescue;
		}

		val = readl(i2c->base + XMEDIA_I2C_CTRL2);
	} while (!(val & (0x1 << CHECK_SDA_IN_SHIFT)));

	val = (0x1 << GPIO_MODE_SHIFT) | (0x1 << FORCE_SCL_OEN_SHIFT) |
	      (0x1 << FORCE_SDA_OEN_SHIFT);
	writel(val, i2c->base + XMEDIA_I2C_CTRL2);

	val = (0x1 << GPIO_MODE_SHIFT) | (0x1 << FORCE_SCL_OEN_SHIFT);
	writel(val, i2c->base + XMEDIA_I2C_CTRL2);

	udelay(10);

	val = (0x1 << GPIO_MODE_SHIFT) | (0x1 << FORCE_SCL_OEN_SHIFT) |
	      (0x1 << FORCE_SDA_OEN_SHIFT);
	writel(val, i2c->base + XMEDIA_I2C_CTRL2);

disable_rescue:
	val = (0x1 << FORCE_SCL_OEN_SHIFT) | 0x1;
	writel(val, i2c->base + XMEDIA_I2C_CTRL2);
}

static inline void xmedia_i2c_disable(const struct xmedia_i2c_dev *i2c)
{
	unsigned int val;

	val = readl(i2c->base + XMEDIA_I2C_GLB);
	val &= ~GLB_EN_MASK;
	writel(val, i2c->base + XMEDIA_I2C_GLB);
}

static inline void xmedia_i2c_enable(const struct xmedia_i2c_dev *i2c)
{
	unsigned int val;

	val = readl(i2c->base + XMEDIA_I2C_GLB);
	val |= GLB_EN_MASK;
	writel(val, i2c->base + XMEDIA_I2C_GLB);
}

static inline void xmedia_i2c_cfg_irq(const struct xmedia_i2c_dev *i2c,
				     unsigned int flag)
{
	writel(flag, i2c->base + XMEDIA_I2C_INTR_EN);
}

static inline void xmedia_i2c_disable_irq(const struct xmedia_i2c_dev *i2c,
		unsigned int flag)
{
	unsigned int val;

	val = readl(i2c->base + XMEDIA_I2C_INTR_EN);
	val &= ~flag;
	writel(val, i2c->base + XMEDIA_I2C_INTR_EN);
}

static inline unsigned int xmedia_i2c_clr_irq(const struct xmedia_i2c_dev *i2c)
{
	unsigned int val;

	val = readl(i2c->base + XMEDIA_I2C_INTR_STAT);
	writel(INTR_ALL_MASK, i2c->base + XMEDIA_I2C_INTR_RAW);

	return val;
}

static inline void xmedia_i2c_cmdreg_set(const struct xmedia_i2c_dev *i2c,
					unsigned int cmd, unsigned int *offset)
{
	dev_dbg(i2c->dev, "i2c reg: offset=0x%x, cmd=0x%x...\n",
		*offset * 4, cmd);
	writel(cmd, i2c->base + XMEDIA_I2C_CMD_BASE + *offset * 4);
	(*offset)++;
}

/*
 * config i2c slave addr
 */
static inline void xmedia_i2c_set_addr(const struct xmedia_i2c_dev *i2c)
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

	writel(addr, i2c->base + XMEDIA_I2C_DATA1);
}

/*
 * Start command sequence
 */
static inline void xmedia_i2c_start_cmd(const struct xmedia_i2c_dev *i2c)
{
	unsigned int val;

	val = readl(i2c->base + XMEDIA_I2C_CTRL1);
	val |= CTRL1_CMD_START_MASK;
	writel(val, i2c->base + XMEDIA_I2C_CTRL1);
}

static int xmedia_i2c_wait_rx_noempty(const struct xmedia_i2c_dev *i2c)
{
	unsigned int time_cnt = 0;
	unsigned int val;

	do {
		val = readl(i2c->base + XMEDIA_I2C_STAT);
		if (val & STAT_RXF_NOE_MASK)
			return 0;

		udelay(50);
	} while (time_cnt++ < I2C_WAIT_TIMEOUT);

	xmedia_i2c_rescue(i2c);

	dev_err(i2c->dev, "wait rx no empty timeout, RIS: 0x%x, SR: 0x%x\n",
		readl(i2c->base + XMEDIA_I2C_INTR_RAW), val);
	return -EIO;
}

static int xmedia_i2c_wait_tx_nofull(const struct xmedia_i2c_dev *i2c)
{
	unsigned int time_cnt = 0;
	unsigned int val;

	do {
		val = readl(i2c->base + XMEDIA_I2C_STAT);
		if (val & STAT_TXF_NOF_MASK)
			return 0;

		udelay(50);
	} while (time_cnt++ < I2C_WAIT_TIMEOUT);

	xmedia_i2c_rescue(i2c);

	dev_err(i2c->dev, "wait rx no empty timeout, RIS: 0x%x, SR: 0x%x\n",
		readl(i2c->base + XMEDIA_I2C_INTR_RAW), val);
	return -EIO;
}

static int xmedia_i2c_wait_idle(const struct xmedia_i2c_dev *i2c)
{
	unsigned int time_cnt = 0;
	unsigned int val;

	do {
		val = readl(i2c->base + XMEDIA_I2C_INTR_RAW);
		if (val & (INTR_ABORT_MASK)) {
			dev_err(i2c->dev, "wait idle abort!, RIS: 0x%x\n",
				val);
			return -EIO;
		}

		if (val & INTR_CMD_DONE_MASK)
			return 0;

		udelay(50);
	} while (time_cnt++ < I2C_WAIT_TIMEOUT);

	xmedia_i2c_rescue(i2c);

	dev_err(i2c->dev, "wait idle timeout, RIS: 0x%x, SR: 0x%x\n",
		val, readl(i2c->base + XMEDIA_I2C_STAT));

	return -EIO;
}

static void xmedia_i2c_set_freq(struct xmedia_i2c_dev *i2c)
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
		pr_err("xmedia_i2c_set_freq:freq can't be zero!");
		return;
	}

	if (freq <= 100000) {
		/* in normal mode               F_scl: freq
		   i2c_scl_hcnt = (F_i2c / F_scl) * 0.5
		   i2c_scl_hcnt = (F_i2c / F_scl) * 0.5
		 */
		val = clk_rate / (freq * 2);
		writel(val, i2c->base + XMEDIA_I2C_SCL_H);
		writel(val, i2c->base + XMEDIA_I2C_SCL_L);
	} else {
		/* in fast mode         F_scl: freq
		   i2c_scl_hcnt = (F_i2c / F_scl) * 0.36
		   i2c_scl_hcnt = (F_i2c / F_scl) * 0.64
		 */
		val = ((clk_rate / 100) * 36) / freq;
		writel(val, i2c->base + XMEDIA_I2C_SCL_H);
		val = ((clk_rate / 100) * 64) / freq;
		writel(val, i2c->base + XMEDIA_I2C_SCL_L);
	}

	val = readl(i2c->base + XMEDIA_I2C_GLB);
	val &= ~GLB_SDA_HOLD_MASK;
	val |= ((0xa << GLB_SDA_HOLD_SHIFT) & GLB_SDA_HOLD_MASK);
	writel(val, i2c->base + XMEDIA_I2C_GLB);
}

/*
 * set i2c controller TX and RX FIFO water
 */
static inline void xmedia_i2c_set_water(const struct xmedia_i2c_dev *i2c)
{
	writel(I2C_TXF_WATER, i2c->base + XMEDIA_I2C_TX_WATER);
	writel(I2C_RXF_WATER, i2c->base + XMEDIA_I2C_RX_WATER);
}

/*
 * initialise the controller, set i2c bus interface freq
 */
static void xmedia_i2c_hw_init(struct xmedia_i2c_dev *i2c)
{
	xmedia_i2c_disable(i2c);
	xmedia_i2c_disable_irq(i2c, INTR_ALL_MASK);
	xmedia_i2c_set_freq(i2c);
	xmedia_i2c_set_water(i2c);
}

/*
 * xmedia_i2c_cfg_cmd - config i2c controller command sequence
 *
 * After all the timing command is configured,
 * and then start the command, you can i2c communication,
 * and then only need to read and write i2c fifo.
 */
static void xmedia_i2c_cfg_cmd(const struct xmedia_i2c_dev *i2c)
{
	struct i2c_msg *msg = i2c->msg;
	int offset = 0;

	if (i2c->msg_idx == 0)
		xmedia_i2c_cmdreg_set(i2c, CMD_TX_S, &offset);
	else
		xmedia_i2c_cmdreg_set(i2c, CMD_TX_RS, &offset);

	if (msg->flags & I2C_M_TEN) {
		if (i2c->msg_idx == 0) {
			xmedia_i2c_cmdreg_set(i2c, CMD_TX_D1_2, &offset);
			xmedia_i2c_cmdreg_set(i2c, CMD_TX_D1_1, &offset);
		} else {
			xmedia_i2c_cmdreg_set(i2c, CMD_TX_D1_2, &offset);
		}
	} else {
		xmedia_i2c_cmdreg_set(i2c, CMD_TX_D1_1, &offset);
	}

	if (msg->flags & I2C_M_IGNORE_NAK)
		xmedia_i2c_cmdreg_set(i2c, CMD_IGN_ACK, &offset);
	else
		xmedia_i2c_cmdreg_set(i2c, CMD_RX_ACK, &offset);

	if (msg->flags & I2C_M_RD) {
		if (msg->len >= 2) {
			writel(offset, i2c->base + XMEDIA_I2C_DST1);
			writel(msg->len - 2, i2c->base + XMEDIA_I2C_LOOP1);
			xmedia_i2c_cmdreg_set(i2c, CMD_RX_FIFO, &offset);
			xmedia_i2c_cmdreg_set(i2c, CMD_TX_ACK, &offset);
			xmedia_i2c_cmdreg_set(i2c, CMD_JMP1, &offset);
		}
		xmedia_i2c_cmdreg_set(i2c, CMD_RX_FIFO, &offset);
		xmedia_i2c_cmdreg_set(i2c, CMD_TX_NACK, &offset);
	} else {
		writel(offset, i2c->base + XMEDIA_I2C_DST1);
		writel(msg->len - 1, i2c->base + XMEDIA_I2C_LOOP1);
		xmedia_i2c_cmdreg_set(i2c, CMD_UP_TXF, &offset);
		xmedia_i2c_cmdreg_set(i2c, CMD_TX_FIFO, &offset);

		if (msg->flags & I2C_M_IGNORE_NAK)
			xmedia_i2c_cmdreg_set(i2c, CMD_IGN_ACK, &offset);
		else
			xmedia_i2c_cmdreg_set(i2c, CMD_RX_ACK, &offset);

		xmedia_i2c_cmdreg_set(i2c, CMD_JMP1, &offset);
	}

	if ((i2c->msg_idx == (i2c->msg_num - 1)) || (msg->flags & I2C_M_STOP)) {
		dev_dbg(i2c->dev, "run to %s %d...TX STOP\n",
			__func__, __LINE__);
		xmedia_i2c_cmdreg_set(i2c, CMD_TX_P, &offset);
	}

	xmedia_i2c_cmdreg_set(i2c, CMD_EXIT, &offset);
}

static void xmedia_i2c_cfg_cmd_mul_reg(struct xmedia_i2c_dev *i2c,unsigned int reg_data_width)
{
	struct i2c_msg *msg = i2c->msg;
	int offset = 0;
	int i;

	if (i2c->msg_idx == 0) {
		xmedia_i2c_cmdreg_set(i2c, CMD_TX_S, &offset);
	} else {
		xmedia_i2c_cmdreg_set(i2c, CMD_TX_RS, &offset);
	}

	if (msg->flags & I2C_M_TEN) {
		if (i2c->msg_idx == 0) {
			xmedia_i2c_cmdreg_set(i2c, CMD_TX_D1_2, &offset);
			xmedia_i2c_cmdreg_set(i2c, CMD_TX_D1_1, &offset);
		} else {
			xmedia_i2c_cmdreg_set(i2c, CMD_TX_D1_2, &offset);
		}
	} else {
		xmedia_i2c_cmdreg_set(i2c, CMD_TX_D1_1, &offset);
	}

	if (msg->flags & I2C_M_IGNORE_NAK) {
		xmedia_i2c_cmdreg_set(i2c, CMD_IGN_ACK, &offset);
	} else {
		xmedia_i2c_cmdreg_set(i2c, CMD_RX_ACK, &offset);
	}

	if (msg->flags & I2C_M_RD) {
		if (msg->len >= 2) {
			writel(offset, i2c->base + XMEDIA_I2C_DST1);
			writel(msg->len - 2, i2c->base + XMEDIA_I2C_LOOP1);
			xmedia_i2c_cmdreg_set(i2c, CMD_RX_FIFO, &offset);
			xmedia_i2c_cmdreg_set(i2c, CMD_TX_ACK, &offset);
			xmedia_i2c_cmdreg_set(i2c, CMD_JMP1, &offset);
		}
		xmedia_i2c_cmdreg_set(i2c, CMD_RX_FIFO, &offset);
		xmedia_i2c_cmdreg_set(i2c, CMD_TX_NACK, &offset);
	} else {
		for(i = 0; i < reg_data_width - 1; i++){
			xmedia_i2c_cmdreg_set(i2c, CMD_UP_TXF, &offset);
			xmedia_i2c_cmdreg_set(i2c, CMD_TX_FIFO, &offset);
			xmedia_i2c_cmdreg_set(i2c, CMD_RX_ACK, &offset);
		}
		xmedia_i2c_cmdreg_set(i2c, CMD_UP_TXF, &offset);
		xmedia_i2c_cmdreg_set(i2c, CMD_TX_FIFO, &offset);
		xmedia_i2c_cmdreg_set(i2c, CMD_IGN_ACK, &offset);
	}

	xmedia_i2c_cmdreg_set(i2c, CMD_TX_P, &offset);
	if(((msg->len / reg_data_width) - 1) > 0){
		writel(0, i2c->base + XMEDIA_I2C_DST2);
		writel((msg->len / reg_data_width) - 1, i2c->base + XMEDIA_I2C_LOOP2);
		xmedia_i2c_cmdreg_set(i2c, CMD_JMP2, &offset);
	}
	xmedia_i2c_cmdreg_set(i2c, CMD_EXIT, &offset);
}

static inline void check_i2c_send_complete(struct xmedia_i2c_dev *i2c)
{
	unsigned int val;
	val = readl(i2c->base + XMEDIA_I2C_GLB);
	if(val & GLB_EN_MASK){
		xmedia_i2c_wait_idle(i2c);
		xmedia_i2c_disable(i2c);
	}
}

#if defined(CONFIG_XMEDIA_DMAC) || defined(CONFIG_EDMAC)
int dma_to_i2c(unsigned long src, unsigned int dst, unsigned int length)
{
	int chan;

	chan = do_dma_m2p(src, dst, length);
	if (chan == -1)
		pr_err("dma_to_i2c error\n");

	return chan;
}

int i2c_to_dma(unsigned int src, unsigned long dst,
	       unsigned int length)
{
	int chan;

	chan = do_dma_p2m(dst, src, length);
	if (chan == -1)
		pr_err("dma_p2m error...\n");

	return chan;
}

static int xmedia_i2c_do_dma_write(struct xmedia_i2c_dev *i2c,
				  unsigned long dma_dst_addr)
{
	int chan, val;
	int status = 0;
	struct i2c_msg *msg = i2c->msg;

	check_i2c_send_complete(i2c);
	xmedia_i2c_set_freq(i2c);
	writel(0x1, i2c->base + XMEDIA_I2C_TX_WATER);
	xmedia_i2c_enable(i2c);
	xmedia_i2c_clr_irq(i2c);
	xmedia_i2c_set_addr(i2c);
	xmedia_i2c_cfg_cmd(i2c);

	/*  transmit DATA from DMAC to I2C in DMA mode */
	chan = dma_to_i2c(dma_dst_addr, (i2c->phybase + XMEDIA_I2C_TXF),
			  msg->len);
	if (chan == -1) {
		status = -1;
		goto fail_0;
	}

	val = readl(i2c->base + XMEDIA_I2C_CTRL1);
	val &= ~CTRL1_DMA_OP_MASK;
	val |= CTRL1_DMA_W | CTRL1_CMD_START_MASK;
	writel(val, i2c->base + XMEDIA_I2C_CTRL1);

	if (dmac_wait(chan) != DMAC_CHN_SUCCESS) {
		status = -1;
		goto fail_1;
	}

	status = xmedia_i2c_wait_idle(i2c);

fail_1:
	dmac_channel_free((unsigned int)chan);
fail_0:
	xmedia_i2c_disable(i2c);

	return status;
}

static int xmedia_i2c_do_dma_write_mul_reg(struct xmedia_i2c_dev *i2c,
				  unsigned long dma_dst_addr, unsigned int reg_data_width)
{
	int chan, val, status = 0;
	struct i2c_msg *msg = i2c->msg;


	check_i2c_send_complete(i2c);
	xmedia_i2c_set_freq(i2c);
	writel(0x1, i2c->base + XMEDIA_I2C_TX_WATER);
	xmedia_i2c_enable(i2c);
	xmedia_i2c_clr_irq(i2c);
	xmedia_i2c_set_addr(i2c);
	xmedia_i2c_cfg_cmd_mul_reg(i2c, reg_data_width);

	/*  transmit DATA from DMAC to I2C in DMA mode */
	chan = dma_to_i2c(dma_dst_addr, (i2c->phybase + XMEDIA_I2C_TXF),
			  msg->len);
	if (chan == -1) {
		status = -1;
		goto fail_0;
	}

	val = readl(i2c->base + XMEDIA_I2C_CTRL1);
	val &= ~CTRL1_DMA_OP_MASK;
	val |= CTRL1_DMA_W | CTRL1_CMD_START_MASK;
	writel(val, i2c->base + XMEDIA_I2C_CTRL1);

fail_0:
	return status;
}

static int xmedia_i2c_do_dma_read(struct xmedia_i2c_dev *i2c,
				 unsigned long dma_dst_addr)
{
	int val, chan;
	int status = 0;
	struct i2c_msg *msg = i2c->msg;

	check_i2c_send_complete(i2c);
	xmedia_i2c_set_freq(i2c);
	writel(0x0, i2c->base + XMEDIA_I2C_RX_WATER);
	xmedia_i2c_enable(i2c);
	xmedia_i2c_clr_irq(i2c);
	xmedia_i2c_set_addr(i2c);
	xmedia_i2c_cfg_cmd(i2c);

	/* transmit DATA from I2C to DMAC in DMA mode */
	chan = i2c_to_dma((i2c->phybase + XMEDIA_I2C_RXF),
			  dma_dst_addr, msg->len);
	if (chan == -1) {
		status = -1;
		goto fail_0;
	}

	val = readl(i2c->base + XMEDIA_I2C_CTRL1);
	val &= ~CTRL1_DMA_OP_MASK;
	val |= CTRL1_CMD_START_MASK | CTRL1_DMA_R;
	writel(val, i2c->base + XMEDIA_I2C_CTRL1);

	if (dmac_wait(chan) != DMAC_CHN_SUCCESS) {
		status = -1;
		goto fail_1;
	}

	status = xmedia_i2c_wait_idle(i2c);

fail_1:
	dmac_channel_free((unsigned int)chan);
fail_0:
	xmedia_i2c_disable(i2c);

	return status;
}

static int copy_to_continuous_mem(struct xmedia_i2c_dev *i2c)
{
	if (should_copy_to_continuous_mem(i2c->msg->buf)) {
		i2c->msg->highmem_buf = i2c->msg->buf;
		i2c->msg->buf = kmalloc(i2c->msg->len, GFP_KERNEL | __GFP_ATOMIC);
		if (i2c->msg->buf == NULL) {
			i2c->msg->buf = i2c->msg->highmem_buf;
			dev_err(i2c->dev, "Allocate continuous memory fail.\n");
			return -EINVAL;
		}
	} else {
		i2c->msg->highmem_buf = NULL;
	}
	return 0;
}

static int xmedia_i2c_dma_xfer_one_msg(struct xmedia_i2c_dev *i2c)
{
	unsigned int status;
	struct i2c_msg *msg = i2c->msg;
	dma_addr_t dma_dst_addr;

	dev_dbg(i2c->dev, "[%s,%d]msg->flags=0x%x, len=0x%x\n",
		__func__, __LINE__, msg->flags, msg->len);

	if (copy_to_continuous_mem(i2c))
		return -EINVAL;

	if (msg->flags & I2C_M_RD) {
		mb();
		dma_dst_addr = dma_map_single(i2c->dev, msg->buf,
					      msg->len, DMA_FROM_DEVICE);
		status = dma_mapping_error(i2c->dev, dma_dst_addr);
		if (status) {
			dev_err(i2c->dev, "DMA mapping failed\n");
			goto out;
		}

		status = xmedia_i2c_do_dma_read(i2c, dma_dst_addr);

		dma_unmap_single(i2c->dev, dma_dst_addr, msg->len, DMA_FROM_DEVICE);
		mb();
		if (i2c->msg->highmem_buf)
			memcpy(msg->highmem_buf, msg->buf, msg->len);
	} else {
		if (i2c->msg->highmem_buf)
			memcpy(msg->buf, msg->highmem_buf, msg->len);
		mb();
		dma_dst_addr = dma_map_single(i2c->dev, msg->buf,
					      msg->len, DMA_TO_DEVICE);
		status = dma_mapping_error(i2c->dev, dma_dst_addr);
		if (status) {
			dev_err(i2c->dev, "DMA mapping failed\n");
			goto out;
		}

		status = xmedia_i2c_do_dma_write(i2c, dma_dst_addr);
		dma_unmap_single(i2c->dev, dma_dst_addr, msg->len, DMA_TO_DEVICE);
		mb();
	}

out:
	if (i2c->msg->highmem_buf) {
		kfree(msg->buf);
		msg->buf = msg->highmem_buf;
		msg->highmem_buf = NULL;
	}

	if (!status) {
		status = xmedia_i2c_wait_idle(i2c);
		xmedia_i2c_disable(i2c);
	}

	return status;
}

static int xmedia_i2c_dma_xfer_one_msg_mul_reg(struct xmedia_i2c_dev *i2c, unsigned int reg_data_width)
{
	unsigned int status;
	struct i2c_msg *msg = i2c->msg;
	dma_addr_t dma_dst_addr;


	dev_dbg(i2c->dev, "[%s,%d]msg->flags=0x%x, len=0x%x\n",
		__func__, __LINE__, msg->flags, msg->len);

	if (copy_to_continuous_mem(i2c))
		return -EINVAL;

	if (msg->flags & I2C_M_RD) {
		mb();
		dma_dst_addr = dma_map_single(i2c->dev, msg->buf,
					      msg->len, DMA_FROM_DEVICE);
		status = dma_mapping_error(i2c->dev, dma_dst_addr);
		if (status) {
			dev_err(i2c->dev, "DMA mapping failed\n");
			goto out;
		}

		status = xmedia_i2c_do_dma_read(i2c, dma_dst_addr);

		dma_unmap_single(i2c->dev, dma_dst_addr, msg->len, DMA_FROM_DEVICE);
		mb();
		if (i2c->msg->highmem_buf)
			memcpy(msg->highmem_buf, msg->buf, msg->len);
	} else {
		if (i2c->msg->highmem_buf)
			memcpy(msg->buf, msg->highmem_buf, msg->len);
		mb();
		dma_dst_addr = dma_map_single(i2c->dev, msg->buf,
					      msg->len, DMA_TO_DEVICE);
		status = dma_mapping_error(i2c->dev, dma_dst_addr);
		if (status) {
			dev_err(i2c->dev, "DMA mapping failed\n");
			goto out;
		}

		status = xmedia_i2c_do_dma_write_mul_reg(i2c, dma_dst_addr, reg_data_width);
		dma_unmap_single(i2c->dev, dma_dst_addr, msg->len, DMA_TO_DEVICE);
		mb();
	}

out:
	if (i2c->msg->highmem_buf) {
		kfree(msg->buf);
		msg->buf = msg->highmem_buf;
		msg->highmem_buf = NULL;
	}
	return status;
}
#endif
static int xmedia_i2c_polling_xfer_one_msg(struct xmedia_i2c_dev *i2c)
{
	int status;
	unsigned int val;
	struct i2c_msg *msg = i2c->msg;

	dev_dbg(i2c->dev, "[%s,%d]msg->flags=0x%x, len=0x%x\n",
		__func__, __LINE__, msg->flags, msg->len);

	check_i2c_send_complete(i2c);
	xmedia_i2c_enable(i2c);
	xmedia_i2c_clr_irq(i2c);
	xmedia_i2c_set_addr(i2c);
	xmedia_i2c_cfg_cmd(i2c);
	xmedia_i2c_start_cmd(i2c);

	i2c->msg_buf_ptr = 0;

	if (msg->flags & I2C_M_RD) {
		while (i2c->msg_buf_ptr < msg->len) {
			status = xmedia_i2c_wait_rx_noempty(i2c);
			if (status)
				goto end;

			val = readl(i2c->base + XMEDIA_I2C_RXF);
			msg->buf[i2c->msg_buf_ptr] = val;
			i2c->msg_buf_ptr++;
		}
	} else {
		while (i2c->msg_buf_ptr < msg->len) {
			status = xmedia_i2c_wait_tx_nofull(i2c);
			if (status)
				goto end;

			val = msg->buf[i2c->msg_buf_ptr];
			writel(val, i2c->base + XMEDIA_I2C_TXF);
			i2c->msg_buf_ptr++;
		}
	}

	status = xmedia_i2c_wait_idle(i2c);
end:
	xmedia_i2c_disable(i2c);

	return status;
}

static int xmedia_i2c_polling_xfer_one_msg_mul_reg(struct xmedia_i2c_dev *i2c, unsigned int reg_data_width)
{
	int status;
	unsigned int val;
	struct i2c_msg *msg = i2c->msg;

	dev_dbg(i2c->dev, "[%s,%d]msg->flags=0x%x, len=0x%x\n",
		__func__, __LINE__, msg->flags, msg->len);

	check_i2c_send_complete(i2c);
	xmedia_i2c_enable(i2c);
	xmedia_i2c_clr_irq(i2c);
	xmedia_i2c_set_addr(i2c);
	xmedia_i2c_cfg_cmd_mul_reg(i2c, reg_data_width);
	xmedia_i2c_start_cmd(i2c);

	i2c->msg_buf_ptr = 0;

	if (msg->flags & I2C_M_RD) {
		while (i2c->msg_buf_ptr < msg->len) {
			status = xmedia_i2c_wait_rx_noempty(i2c);
			if (status) {
				goto end;
			}

			val = readl(i2c->base + XMEDIA_I2C_RXF);
			msg->buf[i2c->msg_buf_ptr] = val;
			i2c->msg_buf_ptr++;

		}
	} else {
		while (i2c->msg_buf_ptr < msg->len) {
			status = xmedia_i2c_wait_tx_nofull(i2c);
			if (status) {
				goto end;
			}

			val = msg->buf[i2c->msg_buf_ptr];
			writel(val, i2c->base + XMEDIA_I2C_TXF);
			i2c->msg_buf_ptr++;
		}
	}

end:
	return status;
}

static irqreturn_t xmedia_i2c_isr(int irq, void *dev_id)
{
	struct xmedia_i2c_dev *i2c = dev_id;
	unsigned int irq_status;
	struct i2c_msg *msg = i2c->msg;

	spin_lock(&i2c->lock);

	irq_status = xmedia_i2c_clr_irq(i2c);
	dev_dbg(i2c->dev, "%s RIS:  0x%x\n", __func__, irq_status);

	if (!irq_status) {
		dev_dbg(i2c->dev, "no irq\n");
		goto end;
	}

	if (irq_status & INTR_ABORT_MASK) {
		dev_err(i2c->dev, "irq handle abort, RIS: 0x%x\n",
			irq_status);
		i2c->status = -EIO;
		xmedia_i2c_disable_irq(i2c, INTR_ALL_MASK);

		complete(&i2c->msg_complete);
		goto end;
	}

	if (msg->flags & I2C_M_RD) {
		while ((readl(i2c->base + XMEDIA_I2C_STAT) & STAT_RXF_NOE_MASK)
				&& (i2c->msg_buf_ptr < msg->len)) {
			msg->buf[i2c->msg_buf_ptr] =
				readl(i2c->base + XMEDIA_I2C_RXF);
			i2c->msg_buf_ptr++;
		}
	} else {
		while ((readl(i2c->base + XMEDIA_I2C_STAT) & STAT_TXF_NOF_MASK)
				&& (i2c->msg_buf_ptr < msg->len)) {
			writel(msg->buf[i2c->msg_buf_ptr],
			       i2c->base + XMEDIA_I2C_TXF);
			i2c->msg_buf_ptr++;
		}
	}

	if (i2c->msg_buf_ptr >= msg->len)
		xmedia_i2c_disable_irq(i2c, INTR_TX_MASK | INTR_RX_MASK);

	if (irq_status & INTR_CMD_DONE_MASK) {
		dev_dbg(i2c->dev, "cmd done\n");
		i2c->status =  0;
		xmedia_i2c_disable_irq(i2c, INTR_ALL_MASK);

		complete(&i2c->msg_complete);
	}

end:
	spin_unlock(&i2c->lock);

	return IRQ_HANDLED;
}

static int xmedia_i2c_interrupt_xfer_one_msg(struct xmedia_i2c_dev *i2c)
{
	int status;
	struct i2c_msg *msg = i2c->msg;
	unsigned long timeout;
	unsigned long flags;

	dev_dbg(i2c->dev, "[%s,%d]msg->flags=0x%x, len=0x%x\n",
		__func__, __LINE__, msg->flags, msg->len);

	reinit_completion(&i2c->msg_complete);
	i2c->msg_buf_ptr = 0;
	i2c->status = -EIO;

	spin_lock_irqsave(&i2c->lock, flags);
	check_i2c_send_complete(i2c);
	xmedia_i2c_enable(i2c);
	xmedia_i2c_clr_irq(i2c);
	if (msg->flags & I2C_M_RD)
		xmedia_i2c_cfg_irq(i2c, INTR_USE_MASK & ~INTR_TX_MASK);
	else
		xmedia_i2c_cfg_irq(i2c, INTR_USE_MASK & ~INTR_RX_MASK);

	xmedia_i2c_set_addr(i2c);
	xmedia_i2c_cfg_cmd(i2c);
	xmedia_i2c_start_cmd(i2c);
	spin_unlock_irqrestore(&i2c->lock, flags);

	timeout = wait_for_completion_timeout(&i2c->msg_complete,
					      I2C_IRQ_TIMEOUT);

	spin_lock_irqsave(&i2c->lock, flags);
	if (timeout == 0) {
		xmedia_i2c_disable_irq(i2c, INTR_ALL_MASK);
		status = -EIO;
		dev_err(i2c->dev, "%s timeout\n",
			msg->flags & I2C_M_RD ? "rx" : "tx");
	} else {
		status = i2c->status;
	}

	xmedia_i2c_disable(i2c);

	spin_unlock_irqrestore(&i2c->lock, flags);
	return status;
}

/*
 * Master transfer function
 */
static int xmedia_i2c_xfer(struct i2c_adapter *adap,
			  struct i2c_msg *msgs, int num)
{
	struct xmedia_i2c_dev *i2c = i2c_get_adapdata(adap);
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
#if defined(CONFIG_XMEDIA_DMAC) || defined(CONFIG_EDMAC)
		if ((i2c->msg->len >= CONFIG_DMA_MSG_MIN_LEN) &&
				(i2c->msg->len <= CONFIG_DMA_MSG_MAX_LEN)) {
			status = xmedia_i2c_dma_xfer_one_msg(i2c);
			if (status)
				break;
		} else if (i2c->irq >= 0) {
#else
		if (i2c->irq >= 0) {
#endif
			spin_unlock_irqrestore(&i2c->lock, flags);
			status = xmedia_i2c_interrupt_xfer_one_msg(i2c);
			spin_lock_irqsave(&i2c->lock, flags);
			if (status)
				break;
		} else {
			status = xmedia_i2c_polling_xfer_one_msg(i2c);
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

/* xmedia_i2c_break_polling_xfer
 *
 * I2c polling independent branch, Shielding interrupt interface
 */
static int xmedia_i2c_break_polling_xfer(const struct i2c_adapter *adap,
					struct i2c_msg *msgs, int num)
{
	struct xmedia_i2c_dev *i2c = i2c_get_adapdata(adap);
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
#if defined(CONFIG_XMEDIA_DMAC) || defined(CONFIG_EDMAC)
		if ((i2c->msg->len >= CONFIG_DMA_MSG_MIN_LEN) &&
				(i2c->msg->len <= CONFIG_DMA_MSG_MAX_LEN)) {
			status = xmedia_i2c_dma_xfer_one_msg(i2c);
			if (status)
				break;
		}
#else
		status = xmedia_i2c_polling_xfer_one_msg(i2c);
		if (status)
			break;
#endif
		i2c->msg++;
		i2c->msg_idx++;
	}
	if (!status || i2c->msg_idx > 0)
		status = i2c->msg_idx;
	spin_unlock_irqrestore(&i2c->lock, flags);
	return status;
}

static int xmedia_i2c_mul_reg_xfer(struct i2c_adapter *adap,
				 struct i2c_msg *msgs, int num, unsigned int reg_data_width)
{
	struct xmedia_i2c_dev *i2c = i2c_get_adapdata(adap);
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
		if ((i2c->msg->len >= CONFIG_DMA_MSG_MIN_LEN) && (i2c->msg->len <= CONFIG_DMA_MSG_MAX_LEN) && (i2c->msg->flags & I2C_M_DMA)) {
#if defined(CONFIG_EDMAC) && defined(CONFIG_EDMAC_INTERRUPT)
			status = xmedia_i2c_dma_xfer_one_msg_mul_reg(i2c, reg_data_width);
#endif
			if (status) {
				break;
			}
		} else {
			status = xmedia_i2c_polling_xfer_one_msg_mul_reg(i2c, reg_data_width);
			if (status) {
				break;
			}
		}
		i2c->msg++;
		i2c->msg_idx++;
	}
	if (!status || i2c->msg_idx > 0) {
		status = i2c->msg_idx;
	}
	spin_unlock_irqrestore(&i2c->lock, flags);
	return status;
}
/* I2C READ *
 * xmedia_i2c_master_recv - issue a single I2C message in master receive mode
 * @client: Handle to slave device
 * @buf: Where to store data read from slave
 * @count: How many bytes to read, must be less than 64k since msg.len is u16
 *
 * Returns negative errno, or else the number of bytes read.
 */
int xmedia_i2c_master_recv(const struct i2c_client *client, char *buf,
		       int count)
{
	printk("Wrong interface call."
	       "xmedia_i2c_transfer is the only interface to i2c read!!!\n");

	return -EIO;
}
EXPORT_SYMBOL(xmedia_i2c_master_recv);

/* I2C WRITE *
 * xmedia_i2c_master_send - issue a single I2C message in master transmit mode
 * @client: Handle to slave device
 * @buf: Data that will be written to the slave
 * @count: How many bytes to write, must be less than 64k since msg.len is u16
 *
 * Returns negative errno, or else the number of bytes written.
 */
int xmedia_i2c_master_send(const struct i2c_client *client,
		       const char *buf, int count)
{
	struct i2c_adapter *adap = NULL;
	struct i2c_msg msg;
	int msgs_count;

	if ((client == NULL) || (buf == NULL) || (client->adapter == NULL) ||
			(count < 0)) {
		printk(KERN_ERR "invalid args\n");
		return -EINVAL;
	}

	if ((client->addr > 0x3ff) ||
			(((client->flags & I2C_M_TEN) == 0) && (client->addr > 0x7f))) {
		printk(KERN_ERR "dev address out of range\n");
		return -EINVAL;
	}

	adap = client->adapter;
	msg.addr = client->addr;
	msg.flags = client->flags;
	msg.len = count;

	msg.buf = (__u8 *)buf;

	msgs_count = xmedia_i2c_break_polling_xfer(adap, &msg, 1);

	return (msgs_count == 1) ? count : -EIO;
}
EXPORT_SYMBOL(xmedia_i2c_master_send);


int xmedia_i2c_master_send_mul_reg(const struct i2c_client *client,
		       const char *buf, unsigned int count, unsigned int reg_data_width)
{
	struct i2c_adapter *adap = client->adapter;
	struct i2c_msg msg;
	int msgs_count;

	if ((client->addr > 0x3ff)
	    || (((client->flags & I2C_M_TEN) == 0) && (client->addr > 0x7f))) {
		printk(KERN_ERR "dev address out of range\n");
		return -EINVAL;
	}

	msg.addr = client->addr;
	msg.flags = client->flags;
	msg.len = count;

	if ((!buf)||(count < 0)) {
		printk(KERN_ERR "buf == NULL || count < 0, Invalid argument!\n");
		return -EINVAL;
	}
	msg.buf = (__u8 *)buf;

	msgs_count = xmedia_i2c_mul_reg_xfer(adap, &msg, 1,reg_data_width);

	return (msgs_count == 1) ? count : -EIO;
}
EXPORT_SYMBOL(xmedia_i2c_master_send_mul_reg);
/**
 * xmedia_i2c_transfer - execute a single or combined I2C message
 * @adap: Handle to I2C bus
 * @msgs: One or more messages to execute before STOP is issued to
 *  terminate the operation; each message begins with a START.
 * @num: Number of messages to be executed.
 *
 * Returns negative errno, else the number of messages executed.
 *
 * Note that there is no requirement that each message be sent to
 * the same slave address, although that is the most common model.
 */
int xmedia_i2c_transfer(struct i2c_adapter *adap, struct i2c_msg *msgs,
		    int num)
{
	int msgs_count;

	if ((!adap) || (!msgs)) {
		printk(KERN_ERR "adap == NULL || msgs == NULL, Invalid argument!\n");
		return -EINVAL;
	}

	if ((msgs[0].addr > 0x3ff) ||
			(((msgs[0].flags & I2C_M_TEN) == 0) && (msgs[0].addr > 0x7f))) {
		printk(KERN_ERR "msgs[0] dev address out of range\n");
		return -EINVAL;
	}

	if ((msgs[1].addr > 0x3ff) ||
			(((msgs[1].flags & I2C_M_TEN) == 0) && (msgs[1].addr > 0x7f))) {
		printk(KERN_ERR "msgs[1] dev address out of range\n");
		return -EINVAL;
	}

	msgs_count = xmedia_i2c_xfer(adap, msgs, num);

	return msgs_count;
}
EXPORT_SYMBOL(xmedia_i2c_transfer);

static u32 xmedia_i2c_func(struct i2c_adapter *adap)
{
	return I2C_FUNC_I2C | I2C_FUNC_10BIT_ADDR |
	       I2C_FUNC_PROTOCOL_MANGLING |
	       I2C_FUNC_SMBUS_WORD_DATA |
	       I2C_FUNC_SMBUS_BYTE_DATA |
	       I2C_FUNC_SMBUS_BYTE |
	       I2C_FUNC_SMBUS_I2C_BLOCK;
}

static const struct i2c_algorithm xmedia_i2c_algo = {
	.master_xfer		= xmedia_i2c_xfer,
	.functionality		= xmedia_i2c_func,
};

static const struct i2c_vender_operations xmedia_i2c_vops = {
	.i2c_master_send_mul_reg = xmedia_i2c_master_send_mul_reg,
};

static int xmedia_i2c_probe(struct platform_device *pdev)
{
	int status;
	struct xmedia_i2c_dev *i2c;
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
	xmedia_i2c_hw_init(i2c);

	i2c->irq = platform_get_irq(pdev, 0);
	status = devm_request_irq(&pdev->dev, i2c->irq, xmedia_i2c_isr,
				  IRQF_SHARED, dev_name(&pdev->dev), i2c);
	if (status) {
		dev_dbg(i2c->dev, "falling back to polling mode");
		i2c->irq = -1;
	}

	adap = &i2c->adap;
	i2c_set_adapdata(adap, i2c);
	adap->owner = THIS_MODULE;
	strlcpy(adap->name, "xmedia-i2c", sizeof(adap->name));
	adap->dev.parent = &pdev->dev;
	adap->dev.of_node = pdev->dev.of_node;
	adap->algo = &xmedia_i2c_algo;

	/* Add the i2c adapter */
	status = i2c_add_adapter(adap);
	if (status) {
		dev_err(i2c->dev, "failed to add bus to i2c core\n");
		goto err_add_adapter;
	}

	i2c_operations_register(&xmedia_i2c_vops);
	dev_info(i2c->dev, "%s%d@%dhz registered\n",
		 adap->name, adap->nr, i2c->freq);

	return 0;

err_add_adapter:
	clk_disable_unprepare(i2c->clk);
	return status;
}

static int xmedia_i2c_remove(struct platform_device *pdev)
{
	struct xmedia_i2c_dev *i2c = platform_get_drvdata(pdev);
	i2c_operations_unregister();
	clk_disable_unprepare(i2c->clk);
	i2c_del_adapter(&i2c->adap);

	return 0;
}

#ifdef CONFIG_PM_SLEEP
static int xmedia_i2c_suspend(struct device *dev)
{
	struct xmedia_i2c_dev *i2c = dev_get_drvdata(dev);

	i2c_lock_bus(&i2c->adap, I2C_LOCK_ROOT_ADAPTER);
	clk_disable_unprepare(i2c->clk);
	i2c_unlock_bus(&i2c->adap, I2C_LOCK_ROOT_ADAPTER);

	return 0;
}

static int xmedia_i2c_resume(struct device *dev)
{
	struct xmedia_i2c_dev *i2c = dev_get_drvdata(dev);

	i2c_lock_bus(&i2c->adap, I2C_LOCK_ROOT_ADAPTER);
	clk_prepare_enable(i2c->clk);
	xmedia_i2c_hw_init(i2c);
	i2c_unlock_bus(&i2c->adap, I2C_LOCK_ROOT_ADAPTER);

	return 0;
}
#endif

static SIMPLE_DEV_PM_OPS(xmedia_i2c_dev_pm, xmedia_i2c_suspend,
			 xmedia_i2c_resume);

static const struct of_device_id xmedia_i2c_match[] = {
	{ .compatible = "xmedia,xmedia-i2c" },
	{},
};
MODULE_DEVICE_TABLE(of, xmedia_i2c_match);

static struct platform_driver xmedia_i2c_driver = {
	.driver		= {
		.name	= "xmedia-i2c",
		.of_match_table = xmedia_i2c_match,
		.pm	= &xmedia_i2c_dev_pm,
	},
	.probe		= xmedia_i2c_probe,
	.remove		= xmedia_i2c_remove,
};

module_platform_driver(xmedia_i2c_driver);

MODULE_AUTHOR("Xmedia");
MODULE_DESCRIPTION("XMEDIA IPC I2C Bus driver");
MODULE_LICENSE("GPL v2");
