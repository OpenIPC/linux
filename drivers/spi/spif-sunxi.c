/*
 * SUNXI SPIF Controller Driver
 *
 * Copyright (c) 2021-2028 Allwinnertech Co., Ltd.
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * 2021.6.24 liuyu <liuyu@allwinnertech.com>
 *    creat thie file and support sun8i of Allwinner.
 */

//#define DEBUG
#include <linux/init.h>
#include <linux/module.h>
#include <linux/spinlock.h>
#include <linux/workqueue.h>
#include <linux/interrupt.h>
#include <linux/errno.h>
#include <linux/err.h>
#include <linux/clk.h>
#include <linux/pinctrl/consumer.h>
#include <linux/gpio.h>
#include <linux/platform_device.h>
#include <linux/dmapool.h>
#include <linux/sched.h>
#include <linux/kthread.h>
#include <linux/regulator/consumer.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/regulator/consumer.h>
#include <asm/uaccess.h>
#include <asm/cacheflush.h>
#include <asm/io.h>
#include <linux/clk/sunxi.h>
#include <linux/spi/spi.h>
#include <linux/spi/spi_bitbang.h>
#include "spif-sunxi.h"

#define SUNXI_SPIF_DEV_NAME				"sunxi_spif"
#define XFER_TIMEOUT					5000
#define SPIF_DEFAULT_SPEED_HZ			10000000	/*300M/2/(14+1) = 10M*/

static void sunxi_spif_dump_reg(struct sunxi_spif *sspi, u32 offset, u32 len)
{
#ifdef DEBUG
	u32 i;
	u8 buf[64], cnt = 0;

	for (i = 0; i < len; i = i + REG_INTERVAL) {
		if (i%HEXADECIMAL == 0)
			cnt += sprintf(buf + cnt, "0x%08x: ",
					(u32)(sspi->base_addr_phy  + offset + i));

		cnt += sprintf(buf + cnt, "%08x ",
				readl(sspi->base_addr + offset + i));

		if (i%HEXADECIMAL == REG_CL) {
			dev_dbg(&sspi->pdev->dev, "%s\n", buf);
			cnt = 0;
		}
	}
#endif
}

#ifdef DEBUG
void sunxi_spif_dump_data(struct sunxi_spif *sspi, u8 *buf, u32 len)
{
	u32 i, cnt = 0;
	u8 *tmp;

	tmp = kzalloc(len, GFP_KERNEL);
	if (!tmp)
		return;

	for (i = 0; i < len; i++) {
		if (i%HEXADECIMAL == 0)
			cnt += sprintf(tmp + cnt, "0x%08x: ", i);

		cnt += sprintf(tmp + cnt, "%02x ", buf[i]);

		if ((i%HEXADECIMAL == REG_END) || (i == (len - 1))) {
			dev_dbg(&sspi->pdev->dev, "%s\n", tmp);
			cnt = 0;
		}
	}

	kfree(tmp);
}
#endif

static void sunxi_spif_save_register(struct sunxi_spif *sspi)
{
	int i;
	for (i = 0; i < 24; i++)
		sspi->spif_register[i] = readl(sspi->base_addr + 0x04 * i);
}

static void sunxi_spif_restore_register(struct sunxi_spif *sspi)
{
	int i;
	for (i = 0; i < 24; i++)
		writel(sspi->base_addr + 0x04 * i, sspi->spif_register[i]);
}

static int sunxi_spif_regulator_request(struct sunxi_spif *sspi)
{
	struct regulator *regu = NULL;

	/* Consider "n*" as nocare. Support "none", "nocare", "null", "" etc. */
	if ((sspi->regulator_id[0] == 'n') || (sspi->regulator_id[0] == 0))
		return 0;

	regu = devm_regulator_get(NULL, sspi->regulator_id);
	if (IS_ERR(regu)) {
		dev_err(&sspi->pdev->dev, "get regulator %s failed!\n", sspi->regulator_id);
		sspi->regulator = NULL;
		return -EINVAL;
	}

	sspi->regulator = regu;

	return 0;
}

static void sunxi_spif_regulator_release(struct sunxi_spif *sspi)
{
	if (!sspi->regulator)
		return;

	regulator_put(sspi->regulator);
	sspi->regulator = NULL;
}

static int sunxi_spif_regulator_enable(struct sunxi_spif *sspi)
{
#ifndef CONFIG_EVB_PLATFORM
	// fpga don't support
	if (!sspi->regulator)
		return -EINVAL;

	if (regulator_enable(sspi->regulator)) {
		dev_err(&sspi->pdev->dev, "enable regulator %s failed!\n", sspi->regulator_id);
		return -EINVAL;
	}

	if (!sspi->regulator)
#endif
	return 0;
}

static void sunxi_spif_regulator_disable(struct sunxi_spif *sspi)
{
#ifndef CONFIG_EVB_PLATFORM
	if (!sspi->regulator)
		return;

	regulator_disable(sspi->regulator);
#endif
}

static int sunxi_spif_select_gpio_state(struct sunxi_spif *sspi, char *name)
{
	struct pinctrl_state *pctrl_state = NULL;
	int ret = 0;

	pctrl_state = pinctrl_lookup_state(sspi->pctrl, name);
	if (IS_ERR(pctrl_state)) {
		dev_err(&sspi->pdev->dev, "spif pinctrl_lookup_state(%s)\n", name);
		return PTR_ERR(pctrl_state);
	}

	ret = pinctrl_select_state(sspi->pctrl, pctrl_state);
	if (ret) {
		dev_err(&sspi->pdev->dev, "spif pinctrl_select_state(%s) failed\n", name);
		return ret;
	}

	return 0;
}

static int sunxi_spif_pinctrl_init(struct sunxi_spif *sspi)
{
	return sunxi_spif_select_gpio_state(sspi, PINCTRL_STATE_DEFAULT);
}

static int sunxi_spif_pinctrl_exit(struct sunxi_spif *sspi)
{
	/* susend will use this function to set pin sleep
	 * driver remove will use this function to, and then devm related functions
	 * will auto recover resource
	 */
	return sunxi_spif_select_gpio_state(sspi, PINCTRL_STATE_SLEEP);
}

static void spif_set_clk(u32 spif_clk, u32 mode_clk, struct sunxi_spif *sspi)
{
	clk_set_rate(sspi->mclk, spif_clk);
	if (clk_get_rate(sspi->mclk) != spif_clk) {
		clk_set_rate(sspi->mclk, mode_clk);
		dev_err(&sspi->pdev->dev,
				"set spf clock %d failed, use clk:%d\n", spif_clk, mode_clk);
	} else {
		dev_dbg(&sspi->pdev->dev, "set spif clock %d success\n", spif_clk);
	}
}

static int sunxi_spif_clk_init(struct sunxi_spif *sspi)
{
	int ret = 0;

	ret = clk_prepare_enable(sspi->pclk);
	if (ret) {
		dev_err(&sspi->pdev->dev, "Couldn't enable AHB clock\n");
		goto err0;
	}

	ret = clk_prepare_enable(sspi->mclk);
	if (ret) {
		dev_err(&sspi->pdev->dev, "Couldn't enable module clock\n");
		goto err1;
	}

	spif_set_clk(sspi->speed_hz, clk_get_rate(sspi->mclk), sspi);

#if 0
	/* linux-4.9 need't do reset and assert working(ccmu do that) */
	ret = reset_control_reset(sspi->rstc);
	if (ret) {
		dev_err(dev, "Couldn't deassert the device from reset\n");
		goto err2;
	}
#endif

	return ret;

err2:
	clk_disable_unprepare(sspi->mclk);

err1:
	clk_disable_unprepare(sspi->pclk);

err0:
	return ret;
}

static void sunxi_spif_clk_exit(struct sunxi_spif *sspi)
{
	clk_disable_unprepare(sspi->mclk);

	clk_disable_unprepare(sspi->pclk);
}

static void sunxi_spif_inside_clock_init(struct sunxi_spif *sspi)
{
	u32 reg_val = readl(sspi->base_addr + SPIF_TCG);

	if (sspi->working_mode & DOUBLE_EDGE_TRIGGER)
		reg_val |= CLK_SPIF_SRC_SEL;
	else
		reg_val &= ~CLK_SPIF_SRC_SEL;

	reg_val &= ~CLK_SCK_SRC_SEL;
	reg_val &= ~CLK_SCKOUT_SRC_SEL;

	writel(reg_val, sspi->base_addr + SPIF_TCG);
}

static void sunxi_spif_inside_clock_exit(struct sunxi_spif *sspi)
{
	u32 reg_val = readl(sspi->base_addr + SPIF_TCG);

	reg_val &= ~CLK_SPIF_SRC_SEL;
	reg_val &= ~CLK_SCK_SRC_SEL;
	reg_val &= ~CLK_SCKOUT_SRC_SEL;

	writel(reg_val, sspi->base_addr + SPIF_TCG);
}

static void sunxi_spif_fifo_reset(struct sunxi_spif *sspi)
{
	u32 reg_val = readl(sspi->base_addr + SPIF_GCA);
	reg_val |= SPIF_CDC_WF_RST;
	reg_val |= SPIF_CDC_RF_RST;

	if (sspi->working_mode & DQS)
		reg_val |= SPIF_DQS_RF_SRST;

	writel(reg_val, sspi->base_addr + SPIF_GCA);
}

static void sunxi_spif_fifo_init(struct sunxi_spif *sspi)
{
	u32 reg_val;

	sunxi_spif_fifo_reset(sspi);

	/* set fifo water level */
	reg_val = readl(sspi->base_addr + SPIF_CFT);

	reg_val &= ~(0xff << SPIF_RF_EMPTY_TRIG_LEVEL);
	reg_val |= (0x10 << SPIF_RF_EMPTY_TRIG_LEVEL);

	/*rf_fifo should less than 104*/
	reg_val &= ~(0xff << SPIF_RF_FULL_TRIG_LEVEL);
	reg_val |= (0x64 << SPIF_RF_FULL_TRIG_LEVEL);

	reg_val &= ~(0xff << SPIF_WF_EMPTY_TRIG_LEVEL);
	reg_val |= (0x10 << SPIF_WF_EMPTY_TRIG_LEVEL);

	/*ef_fifo should less than 104*/
	reg_val &= ~(0xff << SPIF_WF_FULL_TRIG_LEVEL);
	reg_val |= (0x64 << SPIF_WF_FULL_TRIG_LEVEL);

	writel(reg_val, sspi->base_addr + SPIF_CFT);

	/* dqs mode fifo init */
	if (sspi->working_mode & DQS) {
		/* set fifo water level */
		reg_val = readl(sspi->base_addr + SPIF_QFT);
		reg_val |= 0x0 << SPIF_DQS_EMPTY_TRIG_LEVEL;
		reg_val |= 0xff << SPIF_DQS_FULL_TRIG_LEVEL;
		writel(reg_val, sspi->base_addr + SPIF_QFT);
	}
}

/*
 * cnt1 : the digital delay clk cnt
 * cnt2 : the analog delay clk cnt, only when DQS-mode can be set
  */
static void
sunxi_spif_set_sckr_delay(struct sunxi_spif *sspi, int cnt1, int cnt2)
{
	u32 reg_val = readl(sspi->base_addr + SPIF_TCG);
	reg_val |= cnt1 << DIGITAL_SCKR_DELAY_CFG;
	writel(reg_val, sspi->base_addr + SPIF_TCG);

	if (sspi->working_mode & DQS) {
		/* digital delay + analog delay */
		reg_val |= SCKR_DLY_MODE_SEL;

		reg_val |= ANALOG_SAMP_DL_SW_EN_RX;

		reg_val &= ~(0x3f);
		reg_val |= cnt2 << ANALOG_SAMP_DL_SW_VALUE_RX;

		writel(reg_val, sspi->base_addr + SPIF_TCG);
	} else {
		dev_info(&sspi->pdev->dev, "only DQS mode support analog delay\n");
	}

}

static void sunxi_spif_set_cs_delay(struct sunxi_spif *sspi)
{
	u32 reg_val = readl(sspi->base_addr + SPIF_CSD);

	reg_val &= ~(0xff << SPIF_CSSOT);
	reg_val |= SPIF_CSSOT_DEFAULT << SPIF_CSSOT;

	reg_val &= ~(0xff << SPIF_CSEOT);
	reg_val |= SPIF_CSEOT_DEFAULT << SPIF_CSEOT;

	reg_val &= ~(0xff << SPIF_CSDA);
	reg_val |= SPIF_CSDA_DEFAULT << SPIF_CSDA;

	writel(reg_val, sspi->base_addr + SPIF_CSD);
}

/*
 * choose the spif controller channel paramter source
 * true : set dma mode , AHB_M set spif controller paramter
 * false : set cpu mode , AHB_S set spif controller paramter
 */
static void
sunxi_spif_channel_setup(struct sunxi_spif *sspi, bool mode)
{
	u32 reg_val;

	reg_val = readl(sspi->base_addr + SPIF_GCR);
	if (mode)
		reg_val |= SPIF_CFG_MODE;
	else
		reg_val &= ~SPIF_CFG_MODE;

	writel(reg_val, sspi->base_addr + SPIF_GCR);
}

static void sunxi_spif_set_working_mode(struct sunxi_spif *spi)
{
	struct sunxi_spif *sspi = spi_master_get_devdata(spi->master);
	u32 reg_val;

	/* support the standard/dual/quad mode spi */
	reg_val = readl(sspi->base_addr + SPIF_BATC);
	reg_val &= ~0x3U;
	writel(reg_val, sspi->base_addr + SPIF_BATC);

	/*
	 * prefetch-mode use cpu to set spif-controller
	 * normal-mode use dma descriptor4~7 to set spif-controller
	 */
	if (sspi->working_mode == PREFETCH_READ_MODE) {
		u32 reg_val = readl(sspi->base_addr + SPIF_GCR);
		bool CPU_MODE = false;

		reg_val |= SPIF_PMODE_EN;
		/* set addr map mode */
		reg_val |= PREFETCH_ADDR_MAP_MODE;
		writel(reg_val, sspi->base_addr + SPIF_GCR);

		/* spi parameter config from cpu*/
		sunxi_spif_channel_setup(sspi, CPU_MODE);
	} else {
		bool DMA_MODE = true;
#if 1
		sunxi_spif_channel_setup(sspi, DMA_MODE);
		//reg_val |= SPIF_NMODE_EN;
#else
		sunxi_spif_channel_setup(sspi, false);
#endif
	}
}

static void sunxi_spif_enable_irq(struct sunxi_spif *sspi, u32 bitmap)
{
	u32 reg_val = readl(sspi->base_addr + SPIF_IER);

	reg_val &= 0x0;
	reg_val |= bitmap;

	writel(reg_val, sspi->base_addr + SPIF_IER);
}

static void sunxi_spif_disable_irq(struct sunxi_spif *sspi, u32 bitmap)
{
	u32 reg_val = readl(sspi->base_addr + SPIF_IER);
	reg_val &= ~bitmap;
	writel(reg_val, sspi->base_addr + SPIF_IER);
}

static inline u32 sunxi_spif_query_irq_pending(struct sunxi_spif *sspi)
{
	u32 STA_MASK = SPIF_INT_STA_TC_EN | SPIF_INT_STA_ERR_EN;
	return (STA_MASK & readl(sspi->base_addr + SPIF_ISR));
}

static void sunxi_spif_clear_irq_pending(struct sunxi_spif *sspi, u32 bitmap)
{
	u32 reg_val = readl(sspi->base_addr + SPIF_ISR);
	reg_val |= bitmap;
	writel(reg_val, sspi->base_addr + SPIF_ISR);
}

static void sunxi_spif_soft_reset(struct sunxi_spif *sspi)
{
	u32 reg_val = readl(sspi->base_addr + SPIF_GCA);
	reg_val |= SPIF_RESET;
	writel(reg_val, sspi->base_addr + SPIF_GCA);
}

static irqreturn_t sunxi_spif_handler(int irq, void *dev_id)
{
	struct sunxi_spif *sspi = (struct sunxi_spif *)dev_id;
	u32 irq_sta;
	unsigned long flags = 0;

	spin_lock_irqsave(&sspi->lock, flags);

	irq_sta = readl(sspi->base_addr + SPIF_ISR);
	dev_dbg(&sspi->pdev->dev, "irq is coming, and status is %x", irq_sta);

	if (irq_sta & SPIF_INT_STA_TC) {
		dev_dbg(&sspi->pdev->dev, "SPI TC comes\n");
		/*wakup uplayer, by the sem */
		complete(&sspi->done);
		writel(irq_sta, sspi->base_addr + SPIF_ISR);
		spin_unlock_irqrestore(&sspi->lock, flags);
		return IRQ_HANDLED;
	} else if (irq_sta & SPIF_INT_STA_ERR) { /* master mode:err */
		dev_err(&sspi->pdev->dev, " SPI ERR %#x comes\n", irq_sta);
		sspi->result = -1;
		complete(&sspi->done);
		writel(irq_sta, sspi->base_addr + SPIF_ISR);
		spin_unlock_irqrestore(&sspi->lock, flags);
		return IRQ_HANDLED;
	}

	writel(irq_sta, sspi->base_addr + SPIF_ISR);
	spin_unlock_irqrestore(&sspi->lock, flags);

	return IRQ_NONE;
}

static void sunxi_spif_set_clock_mode(struct spi_device *spi)
{
	struct sunxi_spif *sspi = spi_master_get_devdata(spi->master);
	u32 reg_val = readl(sspi->base_addr + SPIF_GCR);

	/*1. POL */
	if (spi->mode & SPI_CPOL)
		reg_val |= SPIF_TC_POL;
	else
		reg_val &= ~SPIF_TC_POL; /*default POL = 0 */

	/*2. PHA */
	if (spi->mode & SPIF_TC_POL)
		reg_val |= SPIF_TC_PHA;
	else
		reg_val &= ~SPIF_TC_PHA; /*default PHA = 0 */

	writel(reg_val, sspi->base_addr + SPIF_GCR);
}

static void sunxi_spif_set_addr_mode(struct spi_device *spi)
{
	struct sunxi_spif *sspi = spi_master_get_devdata(spi->master);
	u32 reg_val = readl(sspi->base_addr + SPIF_GCR);

	if (spi->mode & SPI_LSB_FIRST) {
		reg_val |= SPIF_TX_CFG_FBS;
		writel(reg_val, sspi->base_addr + SPIF_GCR);
	}
}
/*
* spif controller config chip select
* spif cs can only control by controller self
*/
static u32 sunxi_spif_ss_select(struct spi_device *spi)
{
	struct sunxi_spif *sspi = spi_master_get_devdata(spi->master);
	u32 reg_val = readl(sspi->base_addr + SPIF_GCR);

	if (spi->chip_select < 4) {
		reg_val &= ~(SPIF_CS_MASK << SPIF_CS_SEL) ;/* SS-chip select, clear two bits */
		reg_val |= spi->chip_select << SPIF_CS_SEL;/* set chip select */
		writel(reg_val, sspi->base_addr + SPIF_GCR);
		dev_info(&sspi->pdev->dev, "use ss : %d\n", spi->chip_select);
		return 0;
	} else {
		dev_err(&sspi->pdev->dev, "cs set fail! cs = %d\n", spi->chip_select);
		return -EINVAL;
	}
}

static void sunxi_spif_start_dma_xfer(struct sunxi_spif *sspi)
{
	u32 reg_val = readl(sspi->base_addr + SPIF_DMC);

	/* the dma descriptor default len is 8*4=32 bit */
	reg_val &= ~(0xff << SPIF_DMA_DESCRIPTOR_LEN);
	reg_val |= (0x20 << SPIF_DMA_DESCRIPTOR_LEN);

	/* start transfer, this bit will be quickly clear to 0 after set to 1 */
	reg_val |= SPIF_CFG_DMA_START;

	writel(reg_val, sspi->base_addr + SPIF_DMC);
}

static int
sunxi_spif_prefetch_xfer(struct sunxi_spif *sspi, struct spi_transfer *t)
{
	dev_err(&sspi->pdev->dev, "now don't support\n");

	//sunxi_spif_enable_irq(sspi, (SPIF_PREFETCH_READ_EN | SPIF_INT_STA_ERR_EN));

	return 0;
}

static int
sunxi_spif_normal_xfer(struct sunxi_spif *sspi, struct spi_transfer *t)
{
	struct sunxi_spif_transfer_t *sspi_t;
	unsigned long flags;

	spin_lock_irqsave(&sspi->lock, flags);
	sunxi_spif_enable_irq(sspi, (SPIF_DMA_TRANS_DONE_EN | SPIF_INT_STA_ERR_EN));
	memset(sspi->dma_desc, 0, sizeof(struct dma_descriptor)); /* clear the dma descriptor */

	if (t->tx_buf) {
		sspi_t = (struct sunxi_spif_transfer_t *)t->tx_buf;
	} else if (t->rx_buf) {
		sspi_t = (struct sunxi_spif_transfer_t *)t->rx_buf;
	} else {
		dev_err(&sspi->pdev->dev, "spi_transfer rx and rx is null\n");
		return -EINVAL;
	}

	sspi->dma_desc->dma_cfg0 |= SPIF_DMA_FINISH_FLAG;

	if (t->rx_buf)
		sspi->dma_desc->dma_cfg0 |= SPIF_DMA_TX;
	else
		sspi->dma_desc->dma_cfg0 &= ~SPIF_DMA_TX;

	/* use dma set spif controller config */
	sspi->dma_desc->spif_cfg3 |= SPIF_NORMAL_EN;

	sspi->dma_desc->dma_cfg0 &= ~(0x7 << SPIF_HBURST_TYPE);
	sspi->dma_desc->dma_cfg0 |= 0x3 << SPIF_HBURST_TYPE; /* default value :16 */

	/* set the one time trans len */
	if (sspi_t->data_cnt >= 64)
		sspi->dma_desc->dma_cfg1 |= 0x3 << SPIF_DMA_BLK_LEN;
	else if (sspi_t->data_cnt >= 32)
		sspi->dma_desc->dma_cfg1 |= 0x2 << SPIF_DMA_BLK_LEN;
	else if (sspi_t->data_cnt >= 16)
		sspi->dma_desc->dma_cfg1 |= 0x1 << SPIF_DMA_BLK_LEN;
	else
		sspi->dma_desc->dma_cfg1 |= 0x0 << SPIF_DMA_BLK_LEN;
	sspi->dma_desc->dma_cfg1 |= sspi_t->data_cnt << SPIF_DMA_DATA_LEN;

	/* set the data addr to be send, must be phys addr */
	sspi->dma_desc->data_buf = sspi_t->data_buf;
	//sspi->dma_desc->data_buf |= 0x50100040 << SPIF_DMA_BUFFER_SADDR; /* for fpga test */

	/* config cmd */
	if (sspi_t->cmd) {
		sspi->dma_desc->spif_cfg0 |= SPIF_COMMAND_TRANS_EN;
		sspi->dma_desc->spif_cfg2 |= sspi_t->cmd << SPIF_CMD_OPCODE;
		sspi->dma_desc->spif_cfg2 |= sspi_t->cmd_type << SPIF_CMD_TRANS_TYPE;
	}

	/* config addr */
	if (sspi_t->addr) {
		sspi->dma_desc->spif_cfg0 |= SPIF_ADDR_TRANS_EN;
		sspi->dma_desc->spif_cfg1 |= sspi_t->addr << SPIF_ADDR_OPCODE;
		sspi->dma_desc->spif_cfg2 |= sspi_t->addr_type << SPIF_ADDR_TRANS_TYPE;
		if (sspi_t->addr_bit == 3)
			sspi->dma_desc->spif_cfg3 &= ~SPIF_ADDR_32;
		else if (sspi_t->addr_bit == 4)
			sspi->dma_desc->spif_cfg3 |= SPIF_ADDR_32;
		else
			dev_err(&sspi->pdev->dev, "addr size only can be set 24 or 32\n");
	}

	/* config  mode*/
	if (sspi_t->mode) {
		sspi->dma_desc->spif_cfg0 |= SPIF_MODE_TRANS_EN;
		sspi->dma_desc->spif_cfg2 |= sspi_t->mode << SPIF_MODE_OPCODE;
		sspi->dma_desc->spif_cfg2 |= sspi_t->mode_type << SPIF_MODE_TRANS_TYPE;
	}

	/* config dummy */
	if (sspi_t->dummy_cnt) {
		sspi->dma_desc->spif_cfg0 |= SPIF_DUMMY_TRANS_EN;
		sspi->dma_desc->spif_cfg3 |= sspi_t->dummy_cnt << SPIF_DUMMY_TRANS_NUM;
	}

	/* config data */
	if (sspi_t->data_buf) {
		sspi->dma_desc->spif_cfg0 |= SPIF_TX_DATA_EN;
		sspi->dma_desc->spif_cfg2 |= sspi_t->data_type << SPIF_DATA_TRANS_TYPE;
		sspi->dma_desc->spif_cfg3 |= sspi_t->data_cnt << SPIF_DATA_TRANS_NUM; /* the total trans len */
	}

	dev_dbg(&sspi->pdev->dev, "dma descriptor addr is %x\n"
			"the dma descriptor is :\n"
			"desc-0 = 0x%08x, desc-1 = 0x%08x\n"
			"desc-2 = 0x%08x, desc-3 = 0x%08x\n"
			"desc-4 = 0x%08x, desc-5 = 0x%08x\n"
			"desc-6 = 0x%08x, desc-7 = 0x%08x\n",
			sspi->desc_phys,
			sspi->dma_desc->dma_cfg0, sspi->dma_desc->dma_cfg1,
			sspi->dma_desc->data_buf, sspi->dma_desc->next_desc,
			sspi->dma_desc->spif_cfg0, sspi->dma_desc->spif_cfg1,
			sspi->dma_desc->spif_cfg2, sspi->dma_desc->spif_cfg3);

	/* set the first dma descriptor start address */
	writel(sspi->desc_phys, sspi->base_addr + SPIF_DSC);
	sunxi_spif_start_dma_xfer(sspi);

	spin_unlock_irqrestore(&sspi->lock, flags);
	return 0;
}

/*
 * setup the spif controller according to the characteristics of the transmission
 * return 0:succeed, < 0:failed.
 */
static int sunxi_spif_xfer_setup(struct spi_device *spi, struct spi_transfer *t)
{
	struct sunxi_spif *sspi = spi_master_get_devdata(spi->master);

	/* every spi-device can adjust the speed of spif bus clock frequency */
	u32 speed_hz = (t && t->speed_hz) ? t->speed_hz : spi->max_speed_hz;
	if (speed_hz < sspi->speed_hz)
		spif_set_clk(speed_hz, clk_get_rate(sspi->mclk), sspi);

	sunxi_spif_set_clock_mode(spi);

	sunxi_spif_set_addr_mode(spi);

	sunxi_spif_ss_select(spi);

	return 0;
}

static int sunxi_spif_xfer(struct spi_device *spi, struct spi_transfer *t)
{
	struct sunxi_spif *sspi = spi_master_get_devdata(spi->master);
	u32 err;

	if (sspi->working_mode & PREFETCH_READ_MODE) {
		err = sunxi_spif_prefetch_xfer(sspi, t);
		if (err) {
			dev_err(&sspi->pdev->dev, "prefetch mode xfer failed\n");
			return err;
		}
	} else {
		err = sunxi_spif_normal_xfer(sspi, t);
		if (err) {
			dev_err(&sspi->pdev->dev, "prefetch mode xfer failed\n");
			return err;
		}
	}

	return 0;
}

/*
 * the interface to connect spi framework
 * wait for done completion in this function, wakup in the irq hanlder
 */
static int sunxi_spif_transfer_one(struct spi_master *master, struct spi_device *spi,
			    struct spi_transfer *t)
{
	struct sunxi_spif *sspi = spi_master_get_devdata(spi->master);
	unsigned char *tx_buf = (unsigned char *)t->tx_buf;
	unsigned char *rx_buf = (unsigned char *)t->rx_buf;
	unsigned long timeout;
	int err;

	if ((!t->tx_buf && !t->rx_buf) || !t->len)
		return -EINVAL;

	dev_info(&sspi->pdev->dev, "begin transfer, txbuf %p, rxbuf %p, len %d\n",
			tx_buf, rx_buf, t->len);

	err = sunxi_spif_xfer_setup(spi, t);
	if (err)
		return -EINVAL;

	dev_dbg(&sspi->pdev->dev, "before xfer the register value is following :\n");
	sunxi_spif_dump_reg(sspi, 0, 0x60);

	sunxi_spif_xfer(spi, t);

	/* wait for xfer complete in the isr. */
	timeout = wait_for_completion_timeout(&sspi->done,
			msecs_to_jiffies(XFER_TIMEOUT));
	if (timeout == 0) {
		dev_err(&sspi->pdev->dev, "xfer timeout\n");
		err = -EINVAL;
	} else if (sspi->result < 0) {
		/* after transfer error, must reset the clk and fifo */
		sunxi_spif_soft_reset(sspi);
		sunxi_spif_fifo_reset(sspi);
		dev_err(&sspi->pdev->dev, "xfer failed...\n");
		err = -EINVAL;
	}

	dev_dbg(&sspi->pdev->dev, "after xfer the register value is following :\n");
	sunxi_spif_dump_reg(sspi, 0, 0x60);

	sunxi_spif_disable_irq(sspi, (SPIF_DMA_TRANS_DONE_EN | SPIF_INT_STA_ERR_EN));

	return err;
}

/* sunxi_spif_hw_init : config the spif controller's public configration
 * return 0 on success, reutrn err num on failed
 */
static int sunxi_spif_hw_init(struct sunxi_spif *sspi)
{
	int err;

	err = sunxi_spif_regulator_enable(sspi);
	if (err)
		goto err0;

	err = sunxi_spif_pinctrl_init(sspi);
	if (err)
		goto err1;

	err = sunxi_spif_clk_init(sspi);
	if (err)
		goto err2;

	sunxi_spif_soft_reset(sspi);

	sunxi_spif_inside_clock_init(sspi);

	sunxi_spif_fifo_init(sspi);

	sunxi_spif_set_sckr_delay(sspi, 0, 0x20);

	/* set the dedault vaule */
	sunxi_spif_set_cs_delay(sspi);

	sunxi_spif_set_working_mode(sspi);

	return 0;

err2:
	sunxi_spif_pinctrl_exit(sspi);
err1:
	sunxi_spif_regulator_disable(sspi);
err0:
	return err;
}

static int sunxi_spif_hw_deinit(struct sunxi_spif *sspi)
{
	sunxi_spif_clk_exit(sspi);
	sunxi_spif_pinctrl_exit(sspi);
	sunxi_spif_regulator_disable(sspi);

	return 0;
}

static int sunxi_spif_resource_get(struct sunxi_spif *sspi)
{
	struct device_node *np = sspi->pdev->dev.of_node;
	struct platform_device *pdev = sspi->pdev;
	struct resource *mem_res;
	int ret = 0;

	pdev->id = of_alias_get_id(np, "spi");
	if (pdev->id < 0) {
		dev_err(&pdev->dev, "failed to get alias id\n");
		return pdev->id;
	}
	snprintf(sspi->dev_name, sizeof(sspi->dev_name), SUNXI_SPIF_DEV_NAME"%d", pdev->id);

	mem_res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (IS_ERR_OR_NULL(mem_res)) {
		dev_err(&sspi->pdev->dev, "resource get failed\n");
		return -EINVAL;
	}
	sspi->base_addr_phy = mem_res->start;
	sspi->base_addr = devm_ioremap_resource(&pdev->dev, mem_res);
	if (IS_ERR_OR_NULL(sspi->base_addr)) {
		dev_err(&sspi->pdev->dev, "spif unable to ioremap\n");
		return PTR_RET(sspi->base_addr);
	}

	sspi->pclk = devm_clk_get(&sspi->pdev->dev, "pclk");
	if (IS_ERR(sspi->pclk)) {
		dev_err(&sspi->pdev->dev, "spif unable to acquire parent clock\n");
		return PTR_RET(sspi->pclk);
	}

	sspi->mclk = devm_clk_get(&sspi->pdev->dev, "mclk");
	if (IS_ERR(sspi->mclk)) {
		dev_err(&sspi->pdev->dev, "spif unable to acquire mode clock\n");
		return PTR_RET(sspi->mclk);
	}

	sspi->irq = platform_get_irq(sspi->pdev, 0);
	if (sspi->irq < 0) {
		dev_err(&sspi->pdev->dev, "get irq failed\n");
		return sspi->irq;
	}

	sspi->pctrl = devm_pinctrl_get(&sspi->pdev->dev);
	if (IS_ERR(sspi->pctrl)) {
		dev_err(&sspi->pdev->dev, "pin get failed!\n");
		return PTR_ERR(sspi->pctrl);
	} else {
		sunxi_spif_select_gpio_state(sspi, PINCTRL_STATE_DEFAULT);
	}

	/* linux-4.9 need't do reset and assert working(ccmu do that) */
/*
	sspi->rstc = devm_reset_control_get(&sspi->pdev->dev, NULL);
	if (IS_ERR_OR_NULL(sspi->rstc)) {
		dev_err(&sspi->pdev->dev, "Couldn't get reset controller\n");
		return PTR_ERR(sspi->rstc);
	}
*/

	ret = sunxi_spif_regulator_request(sspi);
	if (ret) {
		dev_err(&pdev->dev, "request regulator failed\n");
		return ret;
	}

	ret = of_property_read_u32(np, "clock-frequency", &sspi->speed_hz);
	if (ret) {
		sspi->speed_hz = SPIF_DEFAULT_SPEED_HZ;
		dev_err(&pdev->dev, "get spif controller working speed_hz failed,\
				set default spped_hz %d\n", SPIF_DEFAULT_SPEED_HZ);
	}

	/* AW SPIF controller self working mode */
	if (of_property_read_bool(np, "dtr_mode_enabled")) {
		dev_dbg(&sspi->pdev->dev, "double edge trigger mode enabled");
		sspi->working_mode |= DOUBLE_EDGE_TRIGGER;
	}
	if (of_property_read_bool(np, "prefetch_read_mode_enabled")) {
		dev_info(&sspi->pdev->dev, "prefetch read mode enabled");
		sspi->working_mode |= PREFETCH_READ_MODE;
	}
	if (of_property_read_bool(np, "dqs_mode_enabled")) {
		dev_dbg(&sspi->pdev->dev, "DQS mode enabled");
		sspi->working_mode |= DQS;
	}

	return 0;
}

static void sunxi_spif_resource_put(struct sunxi_spif *sspi)
{
	sunxi_spif_regulator_release(sspi);
}

static struct sunxi_spif_data sun8iw21_spif_data = {
	.max_speed_hz = 150000000, /* 150M */
	.min_speed_hz = 2500000,	/* 2.5M */
	/* SPIF don't support customize gpio as cs */
	.cs_num = 4,		/* the total num of spi chip select */
};

static const struct of_device_id sunxi_spif_dt_ids[] = {
	{.compatible = "allwinner,sun8i-spif", .data = &sun8iw21_spif_data},
	{ /* sentinel */ },
};
MODULE_DEVICE_TABLE(of, sunxi_spif_dt_ids);

static int sunxi_spif_probe(struct platform_device *pdev)
{
	struct spi_master *master;
	struct sunxi_spif *sspi;
	const struct of_device_id *of_id;
	int err = 0;

	master = spi_alloc_master(&pdev->dev, sizeof(struct sunxi_spif));
	if (!master) {
		dev_err(&pdev->dev, "Unable to allocate SPI Master\n");
		return PTR_ERR(master);
	}

	platform_set_drvdata(pdev, master);
	sspi = spi_master_get_devdata(master);

	of_id = of_match_device(sunxi_spif_dt_ids, &pdev->dev);
	if (!of_id) {
		dev_err(&pdev->dev, "of_match_device() failed\n");
		return PTR_ERR(of_id);
	}

	sspi->pool = dmam_pool_create(dev_name(&pdev->dev), &pdev->dev,
		sizeof(struct dma_descriptor), 4, 0);
	if (!sspi->pool) {
		dev_err(&pdev->dev, "No memory for descriptors dma pool\n");
		err = ENOMEM;
		goto err0;
	}
	sspi->dma_desc = dma_pool_zalloc(sspi->pool, GFP_NOWAIT, &sspi->desc_phys);
	if (!sspi->dma_desc) {
		dev_err(&pdev->dev, "Failed to alloc dma descriptor memory\n");
		err = PTR_ERR(sspi->dma_desc);
		goto err1;
	}
	dev_dbg(&pdev->dev, "dma descriptor phys addr is %x\n", sspi->desc_phys);

	sspi->data		= (struct sunxi_spif_data *)(of_id->data);
	sspi->master	= master;
	sspi->pdev		= pdev;

	err = sunxi_spif_resource_get(sspi);
	if (err)
		goto err2;

	master->dev.of_node			= pdev->dev.of_node;
	master->bus_num				= pdev->id;
	master->max_speed_hz		= sspi->data->max_speed_hz;
	master->transfer_one		= sunxi_spif_transfer_one;
	master->num_chipselect		= sspi->data->cs_num;
	/* the spif controller support mode is as flow */
	master->mode_bits = SPI_CPOL | SPI_CPHA | SPI_LSB_FIRST | SPI_TX_DUAL
						| SPI_TX_QUAD | SPI_RX_DUAL | SPI_RX_QUAD;
	master->bits_per_word_mask	= SPI_BPW_MASK(8);

	err = sunxi_spif_hw_init(sspi);
	if (err)
		goto err3;

	err = devm_spi_register_master(&pdev->dev, master);
	if (err) {
		dev_err(&pdev->dev, "cannot register spif master\n");
		goto err4;
	}

	err = devm_request_irq(&pdev->dev, sspi->irq, sunxi_spif_handler, 0,
			sspi->dev_name, sspi);
	if (err) {
		dev_err(&pdev->dev, " Cannot request irq %d\n", sspi->irq);
		goto err4;
	}

	spin_lock_init(&sspi->lock);
	init_completion(&sspi->done);

	dev_info(&pdev->dev, "spif probe success!\n");
	return 0;

err4:
	sunxi_spif_hw_deinit(sspi);

err3:
	sunxi_spif_resource_put(sspi);

err2:
	dma_pool_free(sspi->pool, sspi->dma_desc, sspi->desc_phys);

err1:
	dmam_pool_destroy(sspi->pool);

err0:
	platform_set_drvdata(pdev, NULL);
	spi_master_put(master);
	return err;
}

static int sunxi_spif_remove(struct platform_device *pdev)
{
	struct spi_master *master = spi_master_get(platform_get_drvdata(pdev));
	struct sunxi_spif *sspi = spi_master_get_devdata(master);

	sunxi_spif_hw_deinit(sspi);
	sunxi_spif_resource_put(sspi);
	dma_pool_free(sspi->pool, sspi->dma_desc, sspi->desc_phys);
	dmam_pool_destroy(sspi->pool);
	spi_master_put(master);
	platform_set_drvdata(pdev, NULL);

	return 0;
}

#ifdef CONFIG_PM
static int sunxi_spif_suspend(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct spi_master *master = spi_master_get(platform_get_drvdata(pdev));
	struct sunxi_spif *sspi = spi_master_get_devdata(master);
	int ret;

	/* spi framework function, to stop transfer queue */
	ret = spi_master_suspend(master);
	if (ret) {
		dev_err(&pdev->dev, "spi master suspend error\n");
		return ret;
	}

	sunxi_spif_save_register(sspi);

	sunxi_spif_hw_deinit(sspi);

	dev_info(&pdev->dev, "[spi-flash%d] suspend finish\n", master->bus_num);

	return 0;
}

static int sunxi_spif_resume(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct spi_master *master = spi_master_get(platform_get_drvdata(pdev));
	struct sunxi_spi  *sspi = spi_master_get_devdata(master);
	int ret;

	ret = sunxi_spif_hw_init(sspi);
	if (ret) {
		dev_err(&pdev->dev, "spi master resume error\n");
		return ret;
	}

	sunxi_spif_restore_register(sspi);

	ret = spi_master_resume(master);
	if (ret) {
		dev_err(&pdev->dev, "spi master resume error\n");
		return ret;
	}

	dev_info(&pdev->dev, "[spi-flash%d] resume finish\n", master->bus_num);

	return ret;
}

static const struct dev_pm_ops sunxi_spif_dev_pm_ops = {
	.suspend = sunxi_spif_suspend,
	.resume  = sunxi_spif_resume,
};

#define SUNXI_SPIF_DEV_PM_OPS (&sunxi_spif_dev_pm_ops)
#else
#define SUNXI_SPIF_DEV_PM_OPS NULL
#endif /* CONFIG_PM */

static struct platform_driver sunxi_spif_driver = {
	.probe   = sunxi_spif_probe,
	.remove  = sunxi_spif_remove,
	.driver = {
		.name	= SUNXI_SPIF_DEV_NAME,
		.owner	= THIS_MODULE,
		.pm		= SUNXI_SPIF_DEV_PM_OPS,
		.of_match_table = sunxi_spif_dt_ids,
	},
};
module_platform_driver(sunxi_spif_driver);

MODULE_AUTHOR("liuyu@allwinnertech.com");
MODULE_DESCRIPTION("SUNXI SPIF BUS Driver");
MODULE_ALIAS("platform:"SUNXI_SPIF_DEV_NAME);
MODULE_LICENSE("GPL v2");
MODULE_VERSION("v1.0.0");
