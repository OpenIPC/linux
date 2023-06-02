/*
 * Novatek I2C adapter driver.
 *
 * Copyright (C) 2015 Novatek MicroElectronics Corp.
 *
 * Updated by Howard Chang Aug 2015
 *
 * ----------------------------------------------------------------------------
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
 * ----------------------------------------------------------------------------
 *
 */
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
#include <linux/cpufreq.h>
#include <linux/gpio.h>
#include <linux/semaphore.h>
#include <linux/dma-mapping.h>

#include "plat/i2c_reg.h"
#define DRV_VERSION         "1.00.013"
#define NVTIM_I2C_TIMEOUT (msecs_to_jiffies(1000))

static struct semaphore i2c_sem;
#define loc_cpu()       down(&i2c_sem);
#define unl_cpu()       up(&i2c_sem);


struct nvtim_i2c_clk_info {
	u16 gsr;
	u16 tsr;
	u16 clkl;
	u16 clkh;
};

struct nvtim_i2c_dev {
	struct device       *dev;
	void __iomem        *base;
	struct completion   cmd_complete;
	struct clk          *clk;
	u8                  *buf;
	size_t              buf_len;
	u8                  addr;
	int                 irq;
	int                 stop;
	int                 flags;
	u32                 current_bytes;
	u32                 remain_bytes;
	int                 berr;
	int                 bal;
	int                 bscltimeout;
	u8                  vd_src;
	u32                 busfree_interval;
	struct i2c_adapter  adapter;
	struct nvtim_i2c_platform_data *pdata;
};

/* default platform data to use if not supplied in the platform_device */
static struct nvtim_i2c_platform_data nvtim_i2c_platform_data_default = {
	.bus_freq               = I2C_BUS_CLOCK_100KHZ,
	.bus_delay              = 0,
	.gsr                    = I2C_DEFAULT_GSR,
	.tsr                    = 1,    //I2C_DEFAULT_TSR, // set 1 for backward compatible
	.clkl                   = 0xee, // 100KHz
	.clkh                   = 0xec, // 100KHz
	.rtytimes               = 0,
	.upd_bus_freq_num       = 0,
	.upd_timeout_ms_num     = 0,
	.upd_rtytimes_num       = 0,
	.err_hdl				= 0,
};

static inline void nvtim_i2c_write_reg(struct nvtim_i2c_dev *i2c_dev,
									   int reg, u32 val)
{
	__raw_writel(val, i2c_dev->base + reg);
}

static inline u32 nvtim_i2c_read_reg(struct nvtim_i2c_dev *i2c_dev, int reg)
{
	return __raw_readl(i2c_dev->base + reg);
}

static u32 nvtim_i2c_get_busclock(u32 *gsr, u32 *clkh, u32 *clkl)
{
	return (I2C_SOURCE_CLOCK / (*clkl + *clkh + *gsr + 4));

}

static void nvtim_transceive_bytes(struct nvtim_i2c_dev *dev)
{
	dev->current_bytes = (dev->remain_bytes > I2C_BYTE_CNT_4) \
						 ? I2C_BYTE_CNT_4 : dev->remain_bytes;

	dev->remain_bytes = (dev->remain_bytes > I2C_BYTE_CNT_4) \
						? (dev->remain_bytes - I2C_BYTE_CNT_4) : 0;
}

static void i2c_get_clk_info(struct nvtim_i2c_dev *dev, struct nvtim_i2c_clk_info *clk_info)
{
	if (clk_info == NULL) {
		dev_err(dev->dev, "clk_info NULL\n");
	}

	clk_info->gsr   = (u16)dev->pdata->gsr;
	clk_info->tsr   = (u16)dev->pdata->tsr;
	clk_info->clkl  = (u16)dev->pdata->clkl;
	clk_info->clkh  = (u16)dev->pdata->clkh;
}

static void i2c_set_clk_info(struct nvtim_i2c_dev *dev, struct nvtim_i2c_clk_info clk_info)
{
	dev->pdata->gsr     = (unsigned int)clk_info.gsr;
	dev->pdata->tsr     = (unsigned int)clk_info.tsr;
	dev->pdata->clkl    = (unsigned int)clk_info.clkl;
	dev->pdata->clkh    = (unsigned int)clk_info.clkh;
}

/*
    [input]
    dev
    input_clock
    nvtim_i2c_clk_info.gsr
    nvtim_i2c_clk_info.tsr

    [output]
    nvtim_i2c_clk_info.tsr
    nvtim_i2c_clk_info.clkl
    nvtim_i2c_clk_info.clkh
*/
static void nvtim_calc_clk_dividers(struct nvtim_i2c_dev *dev, u32 input_clock, struct nvtim_i2c_clk_info *clk_info)
{
	u32 gsr, tsr;
	u32 clkh, min_clkh;
	u32 clkl, min_clkl;

	if ((input_clock < I2C_BUS_CLOCK_50KHZ) || \
		(input_clock > I2C_BUS_CLOCK_1MHZ)) {
		dev_err(dev->dev, "invalid value %d\n", input_clock);
	}

	gsr = (u32)clk_info->gsr;
	clkl = clkh = ((I2C_SOURCE_CLOCK / input_clock) - gsr) >> 1;
	while (nvtim_i2c_get_busclock(&gsr, &clkl, &clkh) > input_clock) {
		clkl++;
	}

	/*Standard mode (bus clock <= 100 KHz)*/
	if (input_clock <= I2C_BUS_CLOCK_100KHZ) {
		/*if Clock low period must >= 4.7 us,*/
		/*clock high period must >= 4 us*/
		min_clkl = (u32)(((float)I2C_SOURCE_CLOCK / (float)1000000000) \
						 * (float)4700) + 1;
		min_clkh = (u32)(((float)I2C_SOURCE_CLOCK / (float)1000000000) \
						 * (float)4000) + 1;
	} else if (input_clock <= I2C_BUS_CLOCK_400KHZ) {
		/*Fast mode (bus clock <= 400 KHz)*/
		/*if Clock low period must >= 1.3 us,*/
		/*clock high period must >= 0.6 us*/
		min_clkl = (u32)(((float)I2C_SOURCE_CLOCK / (float)1000000000) \
						 * (float)1300) + 1;
		min_clkh = (u32)(((float)I2C_SOURCE_CLOCK / (float)1000000000) \
						 * (float)600) + 1;
	} else {
		/*Fast mode plus (bus clock <= 1 MHz)*/
		/*if Clock low period must >= 0.5 us,*/
		/*clock high period must >= 0.26 us*/
		min_clkl = (u32)(((float)I2C_SOURCE_CLOCK / (float)1000000000) \
						 * (float)500) + 1;
		min_clkh = (u32)(((float)I2C_SOURCE_CLOCK / (float)1000000000) \
						 * (float)260) + 1;
	}

	/*Find correct clock low period*/
	if (clkl < min_clkl) {
		clkh -= (min_clkl - clkl);
		clkl  = min_clkl;
	}

	if (clkl > I2C_CLKLOW_MAX) {
		clkl = I2C_CLKLOW_MAX;
	}

	/*Find correct clock high period*/
	if (clkh < min_clkh) {
		clkh = min_clkh;
	}

	if (clkh > I2C_CLKHIGH_MAX) {
		clkh = I2C_CLKHIGH_MAX;
	}

	/*Adjust to real register settings*/
	clkl -= 1;
	if (clkl < I2C_CLKLOW_MIN) {
		clkl = I2C_CLKLOW_MIN;
	}

	clkh -= 3;
	if (clkh < I2C_CLKHIGH_MIN) {
		clkh = I2C_CLKHIGH_MIN;
	}

	while (nvtim_i2c_get_busclock(&gsr, &clkl, &clkh) > input_clock) {
		clkl++;
	}

	/*Fix TSR as 0x1. About 0.1us.*/
	tsr = clk_info->tsr;


	/*Clock low counter must > (4 + GSR + TSR)*/
	if (tsr >= (clkl - 4 - gsr)) {
		tsr = clkl - 5 - gsr;
	}

	/*TSR: 1 ~ 1023*/
	if (tsr < I2C_TSR_MIN) {
		tsr = I2C_TSR_MIN;
	} else if (tsr > I2C_TSR_MAX) {
		tsr = I2C_TSR_MAX;
	}

	clk_info->tsr   = (u16)tsr;
	clk_info->clkl  = (u16)clkl;
	clk_info->clkh  = (u16)clkh;

	dev_dbg(dev->dev, "input_clock = %d (gsr %d tsr %d clkh %d clkl %d)\n"
			, input_clock, gsr, tsr, clkh, clkl);

}

/* error handle - bus clear (send 9 clock on SCL) */
static int i2c_nvtim_busclr(struct nvtim_i2c_dev *dev)
{
	nvtim_i2c_write_reg(dev, I2C_CTRL_REG, nvtim_i2c_read_reg(dev, I2C_CTRL_REG) | (1 << I2C_CTRL_BUS_CLR_EN));

	do {
		udelay(1);
	} while ((nvtim_i2c_read_reg(dev, I2C_CTRL_REG) & (1 << I2C_CTRL_BUS_CLR_EN)) == (1 << I2C_CTRL_BUS_CLR_EN));

	dev_err(dev->dev, "%s\n", __func__);

	return 0;
}

/* error handle - module reset */
static int i2c_nvtim_rst_module(struct nvtim_i2c_dev *dev)
{
#define reg_num 0x40
#define reg_sz reg_num >> 2

	int i, reg_val[reg_sz] = {0};

	/* read ori register, for I2C_RSTN(DO_RESET) */
	for (i = 0; i < reg_sz; i++) {
		reg_val[i] = nvtim_i2c_read_reg(dev, i << 2);
//		dev_err(dev->dev, "0x%.2x=0x%.8x\n", (unsigned int)(i << 2), (unsigned int)reg_val[i]);
	}

	clk_disable_unprepare(dev->clk);

    clk_prepare(dev->clk);
    clk_enable(dev->clk);

	/* write ori register, for I2C_RSTN(DO_RESET) */
	for (i = 0; i < reg_sz; i++) {
		nvtim_i2c_write_reg(dev, i << 2, reg_val[i]);
//		dev_err(dev->dev, "0x%.2x=0x%.8x\n", (unsigned int)(i << 2), (unsigned int)reg_val[i]);
	}

	dev_err(dev->dev, "%s: staus 0x%.8x\n", __func__, nvtim_i2c_read_reg(dev, I2C_STS_REG));

	return 0;
}

/*This function configures I2C init conditions*/
static int i2c_nvtim_init(struct nvtim_i2c_dev *dev)
{
	struct nvtim_i2c_clk_info clk_info = {0};

	/*put I2C into reset*/
	nvtim_i2c_write_reg(dev, I2C_CTRL_REG, I2C_CTRL_REG_DEFAULT);

	/*compute clock dividers & Set default timeout */
	i2c_get_clk_info(dev, &clk_info);
	nvtim_calc_clk_dividers(dev, dev->pdata->bus_freq, &clk_info);
	i2c_set_clk_info(dev, clk_info);
	nvtim_i2c_write_reg(dev, I2C_BUSCLK_REG, clk_info.clkl | (clk_info.clkh << I2C_BUSCLK_HIGH_COUNTER));
	nvtim_i2c_write_reg(dev, I2C_TIMING_REG, clk_info.tsr | (clk_info.gsr << I2C_TIMING_GSR) | (I2C_DEFAULT_SCL_TIMEOUT << I2C_TIMING_SCLTIMEOUT));

	/*Set busfree time*/
	nvtim_i2c_write_reg(dev, I2C_BUSFREE_REG, dev->busfree_interval);
	/*clear interrupts sts*/
	nvtim_i2c_write_reg(dev, I2C_STS_REG, I2C_ALL_BIT);
	/*Enable interrupts*/
	nvtim_i2c_write_reg(dev, I2C_CTRL_REG, I2C_INTR_ALL);

	/*
		I2C error handle
	*/
	// NVT_I2C_AUTO_BUCLEAR (must send befor reset i2c module)
	if ((dev->pdata->err_hdl & NVT_I2C_AUTO_BUCLEAR_BIT) == NVT_I2C_AUTO_BUCLEAR_BIT) {
		i2c_nvtim_busclr(dev);
	}

	// NVT_I2C_AUTO_RSTMDL
	if ((dev->pdata->err_hdl & NVT_I2C_AUTO_RSTMDL_BIT) == NVT_I2C_AUTO_RSTMDL_BIT) {
		i2c_nvtim_rst_module(dev);
	}

	return 0;
}

/*Waiting for bus not busy*/
static int i2c_nvtim_wait_bus_not_busy(struct nvtim_i2c_dev *dev,
									   char allow_sleep)
{
	u32 timeout = I2C_POLLING_TIMEOUT;
	u32 busfree, busbusy;

	do {
		timeout--;
		busbusy = nvtim_i2c_read_reg(dev, I2C_STS_REG);
		busfree = busbusy >> I2C_STS_BUSFREE;
		busbusy &= BIT(I2C_STS_BUSBUSY);

		if (allow_sleep) {
			schedule_timeout(1);
		}
	} while (((busfree == 0) || (busbusy == 1)) && timeout);

	if (timeout) {
		return 0;
	} else {
		return -EBUSY;
	}
}

static void i2c_nvtim_handle_nack(struct nvtim_i2c_dev *dev)
{
	u32  timeout, reg;

	/*Configure controller to send STOP condition*/
	nvtim_i2c_write_reg(dev, I2C_CONFIG_REG, ~I2C_ALL_BIT | \
						BIT(I2C_CONFIG_STOP_GEN0) | BIT(I2C_CONFIG_PIO_DATASIZE));

	nvtim_i2c_write_reg(dev, I2C_CTRL_REG, \
						nvtim_i2c_read_reg(dev, I2C_CTRL_REG) | BIT(I2C_CTRL_TB_EN));

	/*Wait for STOP condition sent, controller back to idle*/
	timeout = I2C_POLLING_TIMEOUT;
	do {
		timeout--;
		reg = nvtim_i2c_read_reg(dev, I2C_STS_REG);
		reg = (reg & (1 << I2C_STS_BUSY));
		reg = reg >> I2C_STS_BUSY;
	} while ((reg == 1) && timeout);

	if (timeout == 0)
		dev_err(dev->dev, "Receive NACK, send STOP, \
			wait for ready timeout!\r\n");

}

static void nvtim_set_data(struct nvtim_i2c_dev *dev)
{
	u32 byte_shift, reg, config_reg;

	config_reg = ~I2C_ALL_BIT;
	reg = I2C_DATA_REG_DEFAULT;

	if (dev->buf_len != 0) {
		config_reg |= (dev->current_bytes << I2C_CONFIG_PIO_DATASIZE);
		byte_shift = 0;
		if ((dev->remain_bytes + dev->current_bytes) == (dev->buf_len + 1)) {
			reg = (dev->addr << 1);
			config_reg |= BIT(I2C_START_GEN_SHIFT);
			byte_shift++;
		}

		for (; byte_shift < dev->current_bytes; byte_shift++) {
			reg |= (*dev->buf & I2C_DATA_MASK) << (I2C_DATA_SHIFT * byte_shift);
			dev->buf++;
		}

		/*Set the stop byte, note that stop byte count from 0*/
		if ((dev->remain_bytes == 0) && (dev->stop) && (dev->buf_len != 0)) {
			config_reg |= BIT(I2C_STOP_GEN_SHIFT + (dev->current_bytes - 1));
		}
	} else {  /* support i2cdetect (1 byte addr scan) */
		config_reg |= ((dev->current_bytes + 1) << I2C_CONFIG_PIO_DATASIZE);
		reg = (dev->addr << 1);
		config_reg |= BIT(I2C_START_GEN_SHIFT);

		reg |= ((0 & I2C_DATA_MASK) << I2C_DATA_SHIFT);
		config_reg |= BIT(I2C_STOP_GEN_SHIFT + 1);
	}

	/*Config controller*/
	nvtim_i2c_write_reg(dev, I2C_CONFIG_REG, config_reg);

	/*Write data*/
	nvtim_i2c_write_reg(dev, I2C_DATA_REG, reg);

	/*Control bits*/
	nvtim_i2c_write_reg(dev, I2C_CTRL_REG, \
						nvtim_i2c_read_reg(dev, I2C_CTRL_REG) | BIT(I2C_CTRL_TB_EN));
}

static void nvtim_set_rxconfig(struct nvtim_i2c_dev *dev)
{
	u32 config_reg, reg;

	/*Set default value*/
	config_reg = ~I2C_ALL_BIT;

	/*Set data size*/
	config_reg |= dev->current_bytes << I2C_CONFIG_PIO_DATASIZE;

	if ((dev->remain_bytes + dev->current_bytes) == (dev->buf_len + 1)) {
		reg = (dev->addr << 1) | I2C_READ_BIT;
		config_reg |= BIT(I2C_START_GEN_SHIFT);
		/*Write data*/
		nvtim_i2c_write_reg(dev, I2C_DATA_REG, reg);
	}

	/*Set the stop byte, note that stop byte count from 0*/
	if ((dev->remain_bytes == 0) && (dev->stop)) {
		config_reg |= BIT(I2C_STOP_GEN_SHIFT + (dev->current_bytes - 1)) \
					  | BIT(I2C_NACK_GEN_SHIFT + (dev->current_bytes - 1));
	}

	/*Config controller*/
	nvtim_i2c_write_reg(dev, I2C_CONFIG_REG, config_reg);

	/*Control bits*/
	nvtim_i2c_write_reg(dev, I2C_CTRL_REG, \
						nvtim_i2c_read_reg(dev, I2C_CTRL_REG) | BIT(I2C_CTRL_TB_EN));
}

static void nvtim_get_rxdata(struct nvtim_i2c_dev *dev)
{
	u32 byte_shift, reg;

	byte_shift = 0;
	if ((dev->remain_bytes + dev->current_bytes) == (dev->buf_len + 1)) {
		byte_shift++;
	}
	for (; byte_shift < dev->current_bytes; byte_shift++) {
		reg = nvtim_i2c_read_reg(dev, I2C_DATA_REG);
		*dev->buf = (reg >> (I2C_DATA_SHIFT * byte_shift)) & I2C_DATA_MASK;
		dev->buf++;
	}
}

static void
nvt_i2c_config(struct i2c_adapter *adap, struct i2c_msg msgs[], int num)
{
	struct nvtim_i2c_dev *dev = i2c_get_adapdata(adap);

	if (msgs[0].flags & NVT_I2C_VD_SRC) {
		dev->vd_src = msgs[0].buf[0];
		//printk("%s dev->vd_src = %d\n", __func__, dev->vd_src);
	}

	if (msgs[0].flags & NVT_I2C_BUSFREE_VAL) {
		dev->busfree_interval = msgs[0].buf[0] | (msgs[0].buf[1] << 8) | (msgs[0].buf[2] << 16);
		nvtim_i2c_write_reg(dev, I2C_BUSFREE_REG, dev->busfree_interval);
		//printk("%s dev->busfree_interval = 0x%x\n", __func__, dev->busfree_interval);
	}

	/* NVT_I2C_SET_CFG_FLG */
	if (msgs[0].flags == NVT_I2C_SET_CFG_FLG) {
		/* check msg len */
		if (msgs[0].len < 1) {
			dev_err(dev->dev, "%s: len %d error\n", __func__, msgs[0].len);
			return;
		}
		if ((msgs[0].buf[0] == NVT_I2C_AUTO_RSTMDL) || (msgs[0].buf[0] == NVT_I2C_AUTO_BUCLEAR)) {
			if (msgs[0].len < 2) {
				dev_err(dev->dev, "%s: len %d error\n", __func__, msgs[0].len);
				return;
			}
		}
		/* parse buffer data and set cfg */
		if (msgs[0].buf[0] == NVT_I2C_AUTO_RSTMDL) {
			if (msgs[0].buf[1] == 0) {
				dev->pdata->err_hdl &= ~(NVT_I2C_AUTO_RSTMDL_BIT);
			} else {
				dev->pdata->err_hdl |= NVT_I2C_AUTO_RSTMDL_BIT;
			}
		}

		if (msgs[0].buf[0] == NVT_I2C_AUTO_BUCLEAR) {
			if (msgs[0].buf[1] == 0) {
				dev->pdata->err_hdl &= ~(NVT_I2C_AUTO_BUCLEAR_BIT);
			} else {
				dev->pdata->err_hdl |= NVT_I2C_AUTO_BUCLEAR_BIT;
			}
		}
	}
}

static int
i2c_nvtim_xfer_msg_vd(struct i2c_adapter *adap, struct i2c_msg msgs[], int num)
{
	struct nvtim_i2c_dev *dev = i2c_get_adapdata(adap);
	int i, ret = 0, length = 0;
	char *pbuf, *phead;
	dma_addr_t dma;

	dev->berr  = 0;
	dev->bal   = 0;
	dev->bscltimeout = 0;
	reinit_completion(&dev->cmd_complete);

	for (i = 0; i < num; i++) {
		if (msgs[0].len != msgs[i].len) {
			printk(KERN_ALERT "failed to send different size cmd for vd sync\n");
			return num;
		}
		length += msgs[i].len;
	}

	phead = pbuf = kmalloc(length + num, GFP_KERNEL | __GFP_ZERO);

	length = 0;
	for (i = 0; i < num; i++) {
		*(pbuf + length) = msgs[i].addr << 1;
		length++;
		memcpy(pbuf + length, msgs[i].buf, msgs[i].len);
		length += msgs[i].len;
	}
	dma = dma_map_single(dev->dev, pbuf, length, DMA_TO_DEVICE);
	if (dma_mapping_error(dev->dev, dma)) {
		printk(KERN_ALERT "failed to do the dma map\n");
		kfree(pbuf);
		return - ENOMEM;
	}

	nvtim_i2c_write_reg(dev, I2C_CONFIG_REG, BIT(I2C_CONFIG_ACCESSMODE));

	nvtim_i2c_write_reg(dev, I2C_CTRL_REG, \
						nvtim_i2c_read_reg(dev, I2C_CTRL_REG) | ((dev->vd_src - 1) << I2C_CTRL_DMA_VD_SRC) | BIT(I2C_CTRL_DMA_VD_SYNC));

	nvtim_i2c_write_reg(dev, I2C_CTRL_REG, \
						nvtim_i2c_read_reg(dev, I2C_CTRL_REG) | BIT(I2C_CTRL_DMAED_INT_EN) | BIT(I2C_CTRL_DMA_RCWRITE));

	// write start addr
	nvtim_i2c_write_reg(dev, I2C_DMA_START_ADDR_REG, __pa(pbuf));

	nvtim_i2c_write_reg(dev, I2C_DMA_SIZE_REG, length);
	nvtim_i2c_write_reg(dev, I2C_DMA_DB, (msgs[0].len + 1) | (1 << 24));

	/*Control bits*/
	nvtim_i2c_write_reg(dev, I2C_CTRL_REG, \
						nvtim_i2c_read_reg(dev, I2C_CTRL_REG) | BIT(I2C_CTRL_DMA_DIR) | BIT(I2C_CTRL_DMA_EN));

	ret = wait_for_completion_timeout(&dev->cmd_complete, dev->adapter.timeout);

	//restore setting
	nvtim_i2c_write_reg(dev, I2C_CONFIG_REG, 0x0);

	nvtim_i2c_write_reg(dev, I2C_CTRL_REG, \
						nvtim_i2c_read_reg(dev, I2C_CTRL_REG) & ~(BIT(I2C_CTRL_DMAED_INT_EN) | BIT(I2C_CTRL_DMA_RCWRITE) | BIT(I2C_CTRL_DMA_VD_SYNC)));

	if (dev->berr) {
		dev_err(dev->dev, "%s(%d) NACK!\n", __func__, __LINE__);
		i2c_nvtim_handle_nack(dev);
		i2c_nvtim_init(dev);
		kfree(pbuf);
		return -EREMOTEIO;
	}

	if (dev->bal) {
		dev_err(dev->dev, "%s(%d) arbitration lost!\n", __func__, __LINE__);
		i2c_nvtim_init(dev);
		kfree(pbuf);
		return -EREMOTEIO;
	}

	if (dev->bscltimeout) {
		dev_err(dev->dev, "%s(%d) scl low timeout !\n", __func__, __LINE__);
		i2c_nvtim_init(dev);
		kfree(pbuf);
		return -EREMOTEIO;
	}

	kfree(pbuf);

	if (!ret) {
		dev_err(dev->dev, "i2c transfer timed out(%d)\n", __LINE__);
		i2c_nvtim_init(dev);
		dev->buf_len = 0;
		return -ETIMEDOUT;
	}

	if (ret < 0) {
		return ret;
	} else {
		return num;
	}

}

/*
 * Low level master read/write transaction. This function is called
 * from i2c_nvtim_xfer.
 */
static int
i2c_nvtim_xfer_msg(struct i2c_adapter *adap, struct i2c_msg *msg, int stop)
{
	struct nvtim_i2c_dev *dev = i2c_get_adapdata(adap);
	struct nvtim_i2c_platform_data *pdata = dev->pdata;
	int ret;

	/* Introduce a delay, required for some boards (e.g nvtim EVM) */
	if (pdata->bus_delay) {
		udelay(pdata->bus_delay);
	}

	dev->addr    = msg->addr;
	dev->buf     = msg->buf;
	dev->buf_len = msg->len;
	dev->flags   = msg->flags;
	dev->berr    = 0;
	dev->bal     = 0;
	dev->bscltimeout = 0;
	dev->stop    = stop;

	reinit_completion(&dev->cmd_complete);

	dev->remain_bytes = dev->buf_len + 1;
	do {
		nvtim_transceive_bytes(dev);

		if (msg->flags & I2C_M_RD) {
			nvtim_set_rxconfig(dev);
		} else {
			nvtim_set_data(dev);
		}

		ret = wait_for_completion_timeout(&dev->cmd_complete, dev->adapter.timeout);


		if (dev->berr) {
			dev_err(dev->dev, "%s(%d) NACK!\n",__func__,__LINE__);
			i2c_nvtim_handle_nack(dev);
			i2c_nvtim_init(dev);
			return -EREMOTEIO;
		}

		if (dev->bal) {
			dev_err(dev->dev, "%s(%d) arbitration lost!\n", __func__, __LINE__);
			i2c_nvtim_init(dev);
			return -EREMOTEIO;
		}

		if (dev->bscltimeout) {
			dev_err(dev->dev, "%s(%d) scl low timeout !\n", __func__, __LINE__);
			i2c_nvtim_init(dev);
			return -EREMOTEIO;
		}
	} while (dev->remain_bytes > 0);

	/*Disable Controller*/
	nvtim_i2c_write_reg(dev, I2C_CTRL_REG, \
						nvtim_i2c_read_reg(dev, I2C_CTRL_REG) & ~BIT(I2C_CTRL_TB_EN));

	if (!ret) {
		dev_err(dev->dev, "i2c transfer timed out(%d) jiffies(%d)\n", __LINE__, dev->adapter.timeout);
		i2c_nvtim_init(dev);
		dev->buf_len = 0;
		return -ETIMEDOUT;
	}

	if (ret < 0) {
		return ret;
	} else {
		return msg->len;
	}

	dev_err(dev->dev, "i2c transfer failed\n");

	return -EIO;
}

static int i2c_upd_freq_dtsi(struct nvtim_i2c_dev *dev, __u16 slv_addr)
{
	int i, is_upd = 0;

	if (dev->pdata == NULL) {
		dev_err(dev->dev, "%s: pdata NULL\n", __func__);
		return is_upd;
	}

	if (dev->pdata->upd_bus_freq_num == 0) {
		return is_upd;
	}

	for (i = 0; i < dev->pdata->upd_bus_freq_num; i++) {
		if (dev->pdata->upd_bus_freq_slvaddr[i] == slv_addr) {
			if (dev->pdata->upd_freq_hz[i] != dev->pdata->bus_freq) {
				// update bus frequency by slv_addr
				nvtim_i2c_write_reg(dev, I2C_BUSCLK_REG, dev->pdata->upd_freq_clkl[i] | (dev->pdata->upd_freq_clkh[i] << I2C_BUSCLK_HIGH_COUNTER));
				nvtim_i2c_write_reg(dev, I2C_TIMING_REG, dev->pdata->upd_freq_tsr[i] | (dev->pdata->upd_freq_gsr[i] << I2C_TIMING_GSR) | (I2C_DEFAULT_SCL_TIMEOUT << I2C_TIMING_SCLTIMEOUT));
				is_upd = 1;
			}
			break;
		}
	}
	return is_upd;
}

static void i2c_upd_freq_dft(struct nvtim_i2c_dev *dev)
{
	if (dev->pdata == NULL) {
		dev_err(dev->dev, "%s: pdata NULL\n", __func__);
		return;
	}
	nvtim_i2c_write_reg(dev, I2C_BUSCLK_REG, dev->pdata->clkl | (dev->pdata->clkh << I2C_BUSCLK_HIGH_COUNTER));
	nvtim_i2c_write_reg(dev, I2C_TIMING_REG, dev->pdata->tsr | (dev->pdata->gsr << I2C_TIMING_GSR) | (I2C_DEFAULT_SCL_TIMEOUT << I2C_TIMING_SCLTIMEOUT));
}

static int i2c_upd_timeout_dtsi(struct nvtim_i2c_dev *dev, __u16 slv_addr)
{
	int i, is_upd = 0;

	if (dev->pdata == NULL) {
		dev_err(dev->dev, "%s: pdata NULL\n", __func__);
		return is_upd;
	}

	if (dev->pdata->upd_timeout_ms_num == 0) {
		return is_upd;
	}

	for (i = 0; i < dev->pdata->upd_timeout_ms_num; i++) {
		if (dev->pdata->upd_timeout_slvaddr[i] == slv_addr) {
			if (dev->pdata->upd_timeout_ms[i] != dev->adapter.timeout) {
				// update timeout by slv_addr
				dev->adapter.timeout = msecs_to_jiffies(dev->pdata->upd_timeout_ms[i]);
				is_upd = 1;
			}
			break;
		}
	}
	return is_upd;
}

static void i2c_upd_timeout_dft(struct nvtim_i2c_dev *dev)
{
	dev->adapter.timeout = NVTIM_I2C_TIMEOUT;
}


static int i2c_upd_rtytimes_dtsi(struct nvtim_i2c_dev *dev, __u16 slv_addr)
{
	int i, is_upd = 0;

	if (dev->pdata == NULL) {
		dev_err(dev->dev, "%s: pdata NULL\n", __func__);
		return is_upd;
	}

	if (dev->pdata->upd_rtytimes_num == 0) {
		return is_upd;
	}

	for (i = 0; i < dev->pdata->upd_rtytimes_num; i++) {
		if (dev->pdata->upd_rtytimes_slvaddr[i] == slv_addr) {
			if (dev->pdata->upd_rtytimes[i] != dev->pdata->rtytimes) {
				// update retry times by slv_addr
				dev->pdata->rtytimes = dev->pdata->upd_rtytimes[i];
				is_upd = 1;
			}
			break;
		}
	}
	return is_upd;
}

static void i2c_upd_rtytimes_dft(struct nvtim_i2c_dev *dev)
{
	if (dev->pdata == NULL) {
		dev_err(dev->dev, "%s: pdata NULL\n", __func__);
		return;
	}
	dev->pdata->rtytimes = 0;
}

/*
 * Prepare controller for a transaction and call i2c_nvtim_xfer_msg
 */
static int
i2c_nvtim_xfer(struct i2c_adapter *adap, struct i2c_msg msgs[], int num)
{
	struct nvtim_i2c_dev *dev = i2c_get_adapdata(adap);
	int i, is_upd[3], ret;
	int rtytimes;

	ret = i2c_nvtim_wait_bus_not_busy(dev, 1);
	if (ret < 0) {
		dev_warn(dev->dev, "timeout waiting for bus ready\n");
		i2c_nvtim_init(dev);
		return ret;
	}

	if ((num == 1) && (msgs[0].flags & NVT_I2C_CONFIG_MASK)) {
		loc_cpu();
		nvt_i2c_config(adap, msgs, num);
		unl_cpu();
		return num;
	}

	if ((num == 1) && (msgs[0].flags == NVT_I2C_SET_CFG_FLG)) {
		loc_cpu();
		nvt_i2c_config(adap, msgs, num);
		unl_cpu();
		return num;
	}

	if (msgs[0].flags & NVT_I2C_VD_SEND) {
		loc_cpu();
		ret = i2c_nvtim_xfer_msg_vd(adap, msgs, num);
		unl_cpu();
		if (ret < 0) {
			return ret;
		}
		return num;
	}

	for (i = 0; i < num; i++) {
		loc_cpu();

		is_upd[0] = i2c_upd_freq_dtsi(dev, msgs[i].addr); // set frequency to dtsi setting
		is_upd[1] = i2c_upd_timeout_dtsi(dev, msgs[i].addr); // set timeout to dtsi setting
		is_upd[2] = i2c_upd_rtytimes_dtsi(dev, msgs[i].addr); // set retry times to dtsi setting
		rtytimes = (int)dev->pdata->rtytimes;
retry:
		ret = i2c_nvtim_xfer_msg(adap, &msgs[i], (i == (num - 1)));
		if ((ret != msgs[i].len) && (--rtytimes > 0)) { // transfer fail
			dev_err(dev->dev, "%s addr 0x%.2x ret %d rtytimes remain %d \n", __func__, msgs[i].addr, ret, rtytimes);
			is_upd[0] = i2c_upd_freq_dtsi(dev, msgs[i].addr); // set frequency to dtsi setting, after i2c_nvtim_init
			goto retry;
		}

		if (is_upd[0]) {
			i2c_upd_freq_dft(dev); // set frequency to default setting
		}
		if (is_upd[1]) {
			i2c_upd_timeout_dft(dev); // set timeout to default setting
		}
		if (is_upd[2]) {
			i2c_upd_rtytimes_dft(dev); // set retry times to default setting
		}

		unl_cpu();
		dev_dbg(dev->dev, "%s [%d/%d] ret: %d\n", __func__, i + 1, num, ret);
		if (ret < 0) {
			return ret;
		}
	}

	return num;
}

static u32 i2c_nvtim_func(struct i2c_adapter *adap)
{
	return I2C_FUNC_I2C | I2C_FUNC_SMBUS_EMUL;
}

/*
 * Interrupt service routine. This gets called whenever an I2C interrupt
 * occurs.
 */
static irqreturn_t i2c_nvtim_isr(int this_irq, void *dev_id)
{
	struct nvtim_i2c_dev *dev = dev_id;
	u32 sts_reg;
	sts_reg = nvtim_i2c_read_reg(dev, I2C_STS_REG);

	/*Handle enabled interrupt status*/
	sts_reg &= (nvtim_i2c_read_reg(dev, I2C_CTRL_REG) & I2C_CTRL_INTEN_MASK);

	if (sts_reg != 0) {
		/*Clear interrupt status*/
		nvtim_i2c_write_reg(dev, I2C_STS_REG, sts_reg);

		if (sts_reg & BIT(I2C_STS_DR)) {
			nvtim_get_rxdata(dev);
		}

		if (sts_reg & (BIT(I2C_STS_DT) | BIT(I2C_STS_DR))) {
			complete(&dev->cmd_complete);
		}

		if (sts_reg & BIT(I2C_STS_DMAED)) {
			complete(&dev->cmd_complete);
		}

		if (sts_reg & BIT(I2C_STS_BERR)) {
			dev->berr = 1;
			complete(&dev->cmd_complete);
		}

		if (sts_reg & BIT(I2C_STS_AL)) {
			dev->bal = 1;
			complete(&dev->cmd_complete);
		}

		if (sts_reg & BIT(I2C_STS_SCLTIMEOUT)) {
			dev->bscltimeout = 1;
			complete(&dev->cmd_complete);
		}
	}

	return IRQ_HANDLED;
}

static struct i2c_algorithm i2c_nvtim_algo = {
	.master_xfer    = i2c_nvtim_xfer,
	.functionality  = i2c_nvtim_func,
};

static int dtsi_i2c_id = -1;
#ifdef CONFIG_OF

static int nvtim_i2c_of_probe(struct platform_device *pdev, struct nvtim_i2c_dev *i2c)
{
	struct device_node *np = pdev->dev.of_node;
	u32 clock_frequency, gsr, tsr, user_data_u32;
	int rt, cnt, i;
	unsigned int *user_data = NULL;
	struct nvtim_i2c_clk_info clk_info = {0};


	if (of_property_read_u32(np, "clock-frequency", &clock_frequency)) {
		dev_err(&pdev->dev, "Missing required parameter 'clock-frequency'\n");
		return -ENODEV;
	}
	(i2c->pdata)->bus_freq = clock_frequency;

	/* gsr [option]*/
	if ((rt = of_property_read_u32(np, "gsr", &gsr)) == 0) {
		(i2c->pdata)->gsr = gsr;
	} else {
		(i2c->pdata)->gsr = I2C_DEFAULT_GSR;
	}

	/* tsr [option]*/
	if ((rt = of_property_read_u32(np, "tsr", &tsr)) == 0) {
		(i2c->pdata)->tsr = tsr;
	} else {
		(i2c->pdata)->tsr = 1; // I2C_DEFAULT_TSR; // set 1 for backward compatible
	}

	/* id [option]*/
	if ((rt = of_property_read_u32(np, "id", &dtsi_i2c_id)) == 0) {
		// used dtsi id
	} else {
		dtsi_i2c_id = -1;
	}

	/* auto_rst [option]*/
	if((rt = of_property_read_u32(np, "auto_rst", &user_data_u32)) == 0) {
		if (user_data_u32 == 0) {
			i2c->pdata->err_hdl &= ~(NVT_I2C_AUTO_RSTMDL_BIT);
		} else {
			i2c->pdata->err_hdl |= NVT_I2C_AUTO_RSTMDL_BIT;
		}
	}

	/* auto_busclear [option]*/
	if((rt = of_property_read_u32(np, "auto_busclear", &user_data_u32)) == 0) {
		if (user_data_u32 == 0) {
			i2c->pdata->err_hdl &= ~(NVT_I2C_AUTO_BUCLEAR_BIT);
		} else {
			i2c->pdata->err_hdl |= NVT_I2C_AUTO_BUCLEAR_BIT;
		}
	}

	/*
	    upd_freq_hz [option]
	    dtsi sample :
	        &i2c1 {
	            upd-clock-frequency = <0x1a 100000>, <0x34 200000>;
	        };
	    description :
	        user_data[0] : device 1 slv_addr
	        user_data[1] : device 1 frequency
	        user_data[2] : device 2 slv_addr
	        user_data[3] : device 2 frequency
	        ...

	*/
	cnt = of_property_count_u32_elems(np, "upd-clock-frequency");
	if (cnt > 0) {
		user_data = kzalloc(sizeof(unsigned int) * cnt, GFP_KERNEL);
		if ((rt = of_property_read_u32_array(np, "upd-clock-frequency", user_data, cnt)) == 0) {
			i2c->pdata->upd_bus_freq_num = cnt / 2;
			i2c->pdata->upd_bus_freq_slvaddr    = kzalloc(sizeof(unsigned int) * i2c->pdata->upd_bus_freq_num, GFP_KERNEL);
			i2c->pdata->upd_freq_hz             = kzalloc(sizeof(unsigned int) * i2c->pdata->upd_bus_freq_num, GFP_KERNEL);
			i2c->pdata->upd_freq_gsr            = kzalloc(sizeof(unsigned int) * i2c->pdata->upd_bus_freq_num, GFP_KERNEL);
			i2c->pdata->upd_freq_tsr            = kzalloc(sizeof(unsigned int) * i2c->pdata->upd_bus_freq_num, GFP_KERNEL);
			i2c->pdata->upd_freq_clkl           = kzalloc(sizeof(unsigned int) * i2c->pdata->upd_bus_freq_num, GFP_KERNEL);
			i2c->pdata->upd_freq_clkh           = kzalloc(sizeof(unsigned int) * i2c->pdata->upd_bus_freq_num, GFP_KERNEL);

			// calc_clk_dividers (must calc after set tsr/gsr to platform_data)
			for (i = 0; i < i2c->pdata->upd_bus_freq_num; i++) {
				dev_dbg(&pdev->dev, "%s : upd-clock-frequency 0x%x=%d\r\n", __func__, (unsigned int)user_data[i * 2], user_data[i * 2 + 1]);

				i2c->pdata->upd_bus_freq_slvaddr[i]     = (user_data[i * 2] & 0x7f);
				i2c->pdata->upd_freq_hz[i]              = user_data[i * 2 + 1];

				i2c_get_clk_info(i2c, &clk_info);
				nvtim_calc_clk_dividers(i2c, i2c->pdata->upd_freq_hz[i], &clk_info);

				i2c->pdata->upd_freq_gsr[i]             = clk_info.gsr;
				i2c->pdata->upd_freq_tsr[i]             = clk_info.tsr;
				i2c->pdata->upd_freq_clkl[i]            = clk_info.clkl;
				i2c->pdata->upd_freq_clkh[i]            = clk_info.clkh;
			}
		}
		if (user_data) {
			kfree(user_data);
		}
	}

	/*
	    upd_timeout_ms [option]
	    dtsi sample :
	        &i2c1 {
	            upd-timeout-ms = <0x2b 1000>, <0x3c 100000>;
	        };
	    description :
	        user_data[0] : device 1 slv_addr
	        user_data[1] : device 1 timeout
	        user_data[2] : device 2 slv_addr
	        user_data[3] : device 2 timeout
	        ...

	*/
	cnt = of_property_count_u32_elems(np, "upd-timeout-ms");
	if (cnt > 0) {
		user_data = kzalloc(sizeof(unsigned int) * cnt, GFP_KERNEL);
		if ((rt = of_property_read_u32_array(np, "upd-timeout-ms", user_data, cnt)) == 0) {
			i2c->pdata->upd_timeout_ms_num = cnt / 2;
			i2c->pdata->upd_timeout_slvaddr     = kzalloc(sizeof(unsigned int) * i2c->pdata->upd_timeout_ms_num, GFP_KERNEL);
			i2c->pdata->upd_timeout_ms          = kzalloc(sizeof(unsigned int) * i2c->pdata->upd_timeout_ms_num, GFP_KERNEL);
			for (i = 0; i < i2c->pdata->upd_timeout_ms_num; i++) {
				dev_dbg(&pdev->dev, "%s : upd-timeout-ms 0x%x=%d\r\n", __func__, (unsigned int)user_data[i * 2], user_data[i * 2 + 1]);
				i2c->pdata->upd_timeout_slvaddr[i]  = (user_data[i * 2] & 0x7f);
				i2c->pdata->upd_timeout_ms[i]       = user_data[i * 2 + 1];
			}
		}
		if (user_data) {
			kfree(user_data);
		}
	}

	/*
	    upd_rty_times [option]
	    dtsi sample :
	        &i2c1 {
	            upd-retry-times = <0x11 3>, <0x22 5>;
	        };
	    description :
	        user_data[0] : device 1 slv_addr
	        user_data[1] : device 1 retry times
	        user_data[2] : device 2 slv_addr
	        user_data[3] : device 2 retry times
	        ...

	*/
	cnt = of_property_count_u32_elems(np, "upd-retry-times");
	if (cnt > 0) {
		user_data = kzalloc(sizeof(unsigned int) * cnt, GFP_KERNEL);
		if ((rt = of_property_read_u32_array(np, "upd-retry-times", user_data, cnt)) == 0) {
			i2c->pdata->upd_rtytimes_num = cnt / 2;
			i2c->pdata->upd_rtytimes_slvaddr    = kzalloc(sizeof(unsigned int) * i2c->pdata->upd_rtytimes_num, GFP_KERNEL);
			i2c->pdata->upd_rtytimes            = kzalloc(sizeof(unsigned int) * i2c->pdata->upd_rtytimes_num, GFP_KERNEL);
			for (i = 0; i < i2c->pdata->upd_rtytimes_num; i++) {
				dev_dbg(&pdev->dev, "%s : upd-retry-times 0x%x=%d\r\n", __func__, (unsigned int)user_data[i * 2], user_data[i * 2 + 1]);
				i2c->pdata->upd_rtytimes_slvaddr[i] = (user_data[i * 2] & 0x7f);
				i2c->pdata->upd_rtytimes[i]         = user_data[i * 2 + 1];
			}
		}
		if (user_data) {
			kfree(user_data);
		}
	}
	return 0;
}
static int nvtim_i2c_of_remove(struct platform_device *pdev, struct nvtim_i2c_dev *i2c)
{
	if (i2c->pdata->upd_bus_freq_slvaddr) {
		kfree(i2c->pdata->upd_bus_freq_slvaddr);
	}
	if (i2c->pdata->upd_freq_hz) {
		kfree(i2c->pdata->upd_freq_hz);
	}
	if (i2c->pdata->upd_freq_gsr) {
		kfree(i2c->pdata->upd_freq_gsr);
	}
	if (i2c->pdata->upd_freq_tsr) {
		kfree(i2c->pdata->upd_freq_tsr);
	}
	if (i2c->pdata->upd_freq_clkl) {
		kfree(i2c->pdata->upd_freq_clkl);
	}
	if (i2c->pdata->upd_freq_clkh) {
		kfree(i2c->pdata->upd_freq_clkh);
	}
	if (i2c->pdata->upd_timeout_slvaddr) {
		kfree(i2c->pdata->upd_timeout_slvaddr);
	}
	if (i2c->pdata->upd_timeout_ms) {
		kfree(i2c->pdata->upd_timeout_ms);
	}
	if (i2c->pdata->upd_rtytimes_slvaddr) {
		kfree(i2c->pdata->upd_rtytimes_slvaddr);
	}
	if (i2c->pdata->upd_rtytimes) {
		kfree(i2c->pdata->upd_rtytimes);
	}

	return 0;
}
#else
#define nvtim_i2c_of_probe(pdev, i2c) -ENODEV
#define nvtim_i2c_of_remove(pdev, i2c) -ENODEV
#endif

static int nvtim_i2c_probe(struct platform_device *pdev)
{
	struct nvtim_i2c_dev *dev;
	struct i2c_adapter *adap;
	struct resource *mem, *irq;
	int r;

	/* NOTE: driver uses the static register mapping */
	mem = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!mem) {
		dev_err(&pdev->dev, "no mem resource?\n");
		return -ENODEV;
	}

	irq = platform_get_resource(pdev, IORESOURCE_IRQ, 0);
	if (!irq) {
		dev_err(&pdev->dev, "no irq resource?\n");
		return -ENODEV;
	}

	dev = devm_kzalloc(&pdev->dev, sizeof(struct nvtim_i2c_dev),
					   GFP_KERNEL);
	if (!dev) {
		dev_err(&pdev->dev, "Memory allocation failed\n");
		return -ENOMEM;
	}

	init_completion(&dev->cmd_complete);
	sema_init(&i2c_sem, 1);

	dev->dev   = &pdev->dev;
	dev->irq   = irq->start;
	dev->pdata = dev->dev->platform_data;
	dev->busfree_interval = 0xE2;
	platform_set_drvdata(pdev, dev);

	if (!dev->pdata) {
		dev->pdata = devm_kzalloc(&pdev->dev, sizeof(struct nvtim_i2c_platform_data), GFP_KERNEL);
		if (!dev->pdata) {
			return -ENOMEM;
		}

		if (nvtim_i2c_of_probe(pdev, dev) != 0) {
			memcpy(dev->pdata, &nvtim_i2c_platform_data_default, sizeof(struct nvtim_i2c_platform_data));
		}
	}

	dev->clk = devm_clk_get(&pdev->dev, dev_name(&pdev->dev));
	if (IS_ERR(dev->clk)) {
		dev_err(&pdev->dev, "can't find clock %s\n", dev_name(&pdev->dev));
		dev->clk = NULL;
	} else {
		clk_prepare(dev->clk);
		clk_enable(dev->clk);
	}

	dev->base = devm_ioremap_resource(&pdev->dev, mem);
	if (IS_ERR(dev->base)) {
		r = PTR_ERR(dev->base);
		goto err_unuse_clocks;
	}

	i2c_nvtim_init(dev);

	r = devm_request_irq(&pdev->dev, dev->irq, i2c_nvtim_isr, 0,
						 pdev->name, dev);
	if (r) {
		dev_err(&pdev->dev, "failure requesting irq %i\n", dev->irq);
		goto err_unuse_clocks;
	}

	adap = &dev->adapter;
	i2c_set_adapdata(adap, dev);
	adap->owner = THIS_MODULE;
	adap->class = I2C_CLASS_HWMON;
	strlcpy(adap->name, "nvtim I2C adapter", sizeof(adap->name));
	adap->algo = &i2c_nvtim_algo;
	adap->dev.parent = &pdev->dev;
	adap->timeout = NVTIM_I2C_TIMEOUT;


	if (dtsi_i2c_id == -1) {
		adap->nr = pdev->id;
	} else {
		adap->nr = dtsi_i2c_id;
	}
#ifdef CONFIG_OF
	adap->dev.of_node = pdev->dev.of_node;
#endif

	r = i2c_add_numbered_adapter(adap);

	if (r) {
		dev_err(&pdev->dev, "failure adding adapter\n");
		goto err_unuse_clocks;
	}

	dev_info(&pdev->dev, "Register %s successfully ver:%s\n", __func__, DRV_VERSION);

	return 0;

err_unuse_clocks:
	clk_disable_unprepare(dev->clk);
	dev->clk = NULL;
	return r;
}

static int nvtim_i2c_remove(struct platform_device *pdev)
{
	struct nvtim_i2c_dev *dev = platform_get_drvdata(pdev);

	nvtim_i2c_of_remove(pdev, dev);

	i2c_del_adapter(&dev->adapter);

	clk_disable_unprepare(dev->clk);
	dev->clk = NULL;

	nvtim_i2c_write_reg(dev, I2C_CTRL_REG, 0);

	return 0;
}

#ifdef CONFIG_PM
static int nvtim_i2c_suspend(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct nvtim_i2c_dev *i2c_dev = platform_get_drvdata(pdev);

	clk_disable_unprepare(i2c_dev->clk);

	return 0;
}

static int nvtim_i2c_resume(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct nvtim_i2c_dev *i2c_dev = platform_get_drvdata(pdev);

	clk_prepare_enable(i2c_dev->clk);

	return 0;
}

static const struct dev_pm_ops nvtim_i2c_pm = {
	.suspend        = nvtim_i2c_suspend,
	.resume         = nvtim_i2c_resume,
};

#define nvtim_i2c_pm_ops (&nvtim_i2c_pm)
#else
#define nvtim_i2c_pm_ops NULL
#endif

#ifdef CONFIG_OF
static const struct of_device_id nvtim_i2c_ids[] = {
	{ .compatible = "nvt,nvt_i2c" },
	{},
};
MODULE_DEVICE_TABLE(of, nvtim_i2c_ids);
#else
#define nvtim_i2c_ids     NULL
#endif

static struct platform_driver nvtim_i2c_driver = {
	.probe      = nvtim_i2c_probe,
	.remove     = nvtim_i2c_remove,
	.driver     = {
		.name           = "nvt_i2c",
		.owner          = THIS_MODULE,
		.pm             = nvtim_i2c_pm_ops,
		.of_match_table = of_match_ptr(nvtim_i2c_ids),
	},
};

/* I2C may be needed to bring up other drivers */
static int __init nvtim_i2c_init_driver(void)
{
	return platform_driver_register(&nvtim_i2c_driver);
}
subsys_initcall(nvtim_i2c_init_driver);

static void __exit nvtim_i2c_exit_driver(void)
{
	platform_driver_unregister(&nvtim_i2c_driver);
}
module_exit(nvtim_i2c_exit_driver);

MODULE_AUTHOR("Novatek");
MODULE_VERSION(DRV_VERSION);
MODULE_DESCRIPTION("NT96680 I2C bus adapter");
MODULE_LICENSE("GPL");
