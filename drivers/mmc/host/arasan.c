/*
 * Arasan MMC/SD/SDIO driver
 *
 *  This is the driver for the Arasan MMC/SD/SDIO host controller
 *  integrated in the STMicroelectronics platforms
 *
 * Author: Giuseppe Cavallaro <peppe.cavallaro@xxxxxx>
 * Copyright (C) 2010 STMicroelectronics Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/io.h>
#include <linux/platform_device.h>
#include <linux/mbus.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/slab.h>
#include <linux/dma-mapping.h>
#include <linux/scatterlist.h>
#include <linux/irq.h>
#include <linux/highmem.h>
#include <linux/sched.h>
#include <linux/mmc/host.h>
#include <asm/sizes.h>
#include <asm/unaligned.h>
#include <mach/hardware.h>
#include <linux/mmc/arasan_plat.h>
#include "arasan.h"

/* To enable more debug information. */
//#define ARASAN_DEBUG
//#define DEBUG

#ifdef ARASAN_DEBUG
#define DBG(fmt, args...)  pr_info(fmt, ## args)
#else
#define DBG(fmt, args...)  do { } while (0)
#endif

static int maxfreq = ARASAN_CLOCKRATE_MAX;
//module_param(maxfreq, int, S_IRUGO);
//MODULE_PARM_DESC(maxfreq, "Maximum card clock frequency (default 25MHz)");

static unsigned int adma = 1;
//module_param(adma, int, S_IRUGO);
//MODULE_PARM_DESC(adma, "Disable/Enable the Advanced DMA mode");

static unsigned int led;
//module_param(led, int, S_IRUGO | S_IWUSR);
//MODULE_PARM_DESC(led, "Enable|Disable LED");

//static unsigned int pio;
//module_param(pio, int, S_IRUGO);
//MODULE_PARM_DESC(pio, "PIO mode (no DMA)");

struct arasan_cap {
	unsigned int timer_freq;
	unsigned int timer_unit;
	unsigned int base_clk_sd;
	unsigned int max_blk_len;
	unsigned int adma2;
	unsigned int high_speed;
	unsigned int sdma;
	unsigned int suspend;
	unsigned int voltage33;
	unsigned int voltage30;
	unsigned int voltage18;
	unsigned int int_mode;
	unsigned int spi;
	unsigned int spi_block;
};

struct arasan_host {
	void __iomem *base;
	struct mmc_request *mrq;
	unsigned int intr_en;
	u8 ctrl;
	unsigned int sg_frags;
	struct scatterlist **sg;
	struct timer_list timer;
	struct mmc_host *mmc;
	struct device *dev;
	struct resource *res;
	int irq;
	struct arasan_cap cap;
	u8 vdd;
	unsigned int freq;
	unsigned int status;
	unsigned int adma;
	unsigned int use_pio;
	u16 pio_blksz;
	u32 pio_blocks;
	u32 *pio_blkbuf;
	spinlock_t lock;
	struct tasklet_struct card_tasklet;
	u8 *adma_desc;
	dma_addr_t adma_addr;

	unsigned int need_poll;
	unsigned int need_detect;
	unsigned int card_irq;
	unsigned int auto_cmd12;
	unsigned int sdio_4bit_data;
};

static inline void arsan_sw_reset(struct arasan_host *host, unsigned int flag)
{
	/* After completing the reset, wait the HC clears these bits */
	if (likely(flag == reset_all)) {
		writeb(ARSAN_RESET_ALL, host->base + ARASAN_SW_RESET);
		do { } while ((readb(host->base + ARASAN_SW_RESET)) &
			 ARSAN_RESET_ALL);
	} else if (flag == reset_cmd_line) {
		writeb(ARSAN_RESET_CMD_LINE, host->base + ARASAN_SW_RESET);
		do { } while ((readb(host->base + ARASAN_SW_RESET)) &
			 ARSAN_RESET_CMD_LINE);

	} else if (flag == reset_dat_line) {
		writeb(ARSAN_RESET_DAT_LINE, host->base + ARASAN_SW_RESET);
		do { } while ((readb(host->base + ARASAN_SW_RESET)) &
			 ARSAN_RESET_DAT_LINE);
	}
}

static inline void arsan_hc_version(struct arasan_host *host)
{
	u16 version;

	version = readw(host->base + ARASAN_HOST_VERSION);
	DBG("Arasan MMC/SDIO:\n\tHC Vendor Version Number: %d\n",
		 (version >> 8));
	DBG("\tHC SPEC Version Number: %d\n", (version & 0x00ff));
}

static void arasan_capabilities(struct arasan_host *host)
{
	unsigned int cap;
	unsigned int max_blk_len;

	cap = readl(host->base + ARASAN_CAPABILITIES);

	DBG("\tArasan capabilities: 0x%x\n", cap);

	host->cap.timer_freq = cap & 0x3f;
	host->cap.timer_unit = (cap >> 7) & 0x1;

	DBG("\tTimeout Clock Freq: %d %s\n", host->cap.timer_freq,
		 host->cap.timer_unit ? "MHz" : "KHz");

	host->cap.base_clk_sd = (cap >> 8) & 0x3f;
	DBG("\tBase Clock Freq for SD: %d MHz\n", host->cap.base_clk_sd);

	max_blk_len = (cap >> 16) & 0x3;
	switch (max_blk_len) {
	case 0:
		host->cap.max_blk_len = 512;
		break;
	case 1:
		host->cap.max_blk_len = 1024;
		break;
	case 2:
		host->cap.max_blk_len = 2048;
		break;
	case 3:
		host->cap.max_blk_len = 4096;
		break;
	default:
		break;
	}
	DBG("\tMax Block size: %d bytes\n", host->cap.max_blk_len);
	//printk("\tMax Block size: %d bytes\n", host->cap.max_blk_len);

	host->cap.adma2 = (cap >> 19) & 0x1;
	host->cap.high_speed = (cap >> 21) & 0x1;
	host->cap.sdma = (cap >> 22) & 0x1;

	DBG("\tadma2 %s, high speed %s, sdma %s\n",
		 host->cap.adma2 ? "Yes" : "Not",
		 host->cap.high_speed ? "Yes" : "Not",
		 host->cap.sdma ? "Yes" : "Not");

	host->cap.suspend = (cap >> 23) & 0x1;
	DBG("\tsuspend/resume %s suported\n",
		 host->cap.adma2 ? "is" : "Not");

	/* Disable adma user option if cap not supported. */
	if (!host->cap.adma2)
		adma = 0;

	host->cap.voltage33 = (cap >> 24) & 0x1;
	host->cap.voltage30 = (cap >> 25) & 0x1;
	host->cap.voltage18 = (cap >> 26) & 0x1;
	host->cap.int_mode = (cap >> 27) & 0x1;
	host->cap.spi = (cap >> 29) & 0x1; /* 是否支持spi模式 */
	host->cap.spi_block = (cap >> 30) & 0x1;

	if (host->cap.voltage33)
		DBG("\t3.3V voltage suported\n");
	if (host->cap.voltage30)
		DBG("\t3.0V voltage suported\n");
	if (host->cap.voltage18)
		DBG("\t1.8V voltage suported\n");

	if (host->cap.int_mode)
		DBG("\tInterrupt Mode supported\n");
	if (host->cap.spi)
		DBG("\tSPI Mode supported\n");
	if (host->cap.spi_block)
		DBG("\tSPI Block Mode supported\n");
}

static void arasan_ctrl_led(struct arasan_host *host, unsigned int flag)
{
	//printk(KERN_EMERG"arasan_ctrl_led.\n");
	if (led) {
		u8 ctrl_reg = readb(host->base + ARASAN_HOST_CTRL);

		//printk(KERN_EMERG"flag:%d\n", flag);
		if (flag)
			ctrl_reg |= ARASAN_HOST_CTRL_LED;
		else
			ctrl_reg &= ~ARASAN_HOST_CTRL_LED;

		host->ctrl = ctrl_reg;
		writeb(host->ctrl, host->base + ARASAN_HOST_CTRL);
	}
}

static inline void arasan_set_interrupts(struct arasan_host *host)
{
	host->intr_en = ARASAN_IRQ_DEFAULT_MASK;
	writel(host->intr_en, host->base + ARASAN_NORMAL_INT_STATUS_EN);
	writel(host->intr_en, host->base + ARASAN_NORMAL_INT_SIGN_EN);

}

static inline void arasan_clear_interrupts(struct arasan_host *host)
{
	writel(0, host->base + ARASAN_NORMAL_INT_STATUS_EN);
	writel(0, host->base + ARASAN_ERR_INT_STATUS_EN);
	writel(0, host->base + ARASAN_NORMAL_INT_SIGN_EN);
}

static void arasan_power_set(struct arasan_host *host, unsigned int pwr, u8 vdd)
{
	u8 pwr_reg;

	pwr_reg = readb(host->base + ARASAN_PWR_CTRL);

	host->vdd = (1 << vdd);

	if (pwr) {
		pwr_reg &= 0xf1;

		if ((host->vdd & MMC_VDD_165_195) && host->cap.voltage18)
			pwr_reg |= ARASAN_PWR_BUS_VOLTAGE_18;
		else if ((host->vdd & MMC_VDD_29_30) && host->cap.voltage30)
			pwr_reg |= ARASAN_PWR_BUS_VOLTAGE_30;
		else if ((host->vdd & MMC_VDD_32_33) && host->cap.voltage33)
			pwr_reg |= ARASAN_PWR_BUS_VOLTAGE_33;

		//pwr_reg |= ARASAN_PWR_CTRL_UP;
		/* 注意：电路设计和外围电路设计相反 */
		pwr_reg &= ~ARASAN_PWR_CTRL_UP;
	} else
		//pwr_reg &= ~ARASAN_PWR_CTRL_UP;
		pwr_reg |= ARASAN_PWR_CTRL_UP;

	DBG("%s: pwr_reg 0x%x, host->vdd = 0x%x\n", __func__, pwr_reg,
	    host->vdd);
	//printk(KERN_EMERG"pwr_reg:%2x\n", pwr_reg);
	writeb(pwr_reg, host->base + ARASAN_PWR_CTRL);
}

static int arasan_test_card(struct arasan_host *host)
{
	unsigned int ret = 0;
	u32 present = readl(host->base + ARASAN_PRESENT_STATE);
	
	if (!host->need_detect)
		goto out;
	if (likely(!(present & ARASAN_PRESENT_STATE_CARD_PRESENT))) {
		ret = -1;
	}

out:
#ifdef ARASAN_DEBUG
	if (present & ARASAN_PRESENT_STATE_CARD_STABLE)
		pr_info("\tcard stable...");
	if (!(present & ARASAN_PRESENT_STATE_WR_EN))
		pr_info("\tcard Write protected...");
	if (present & ARASAN_PRESENT_STATE_BUFFER_RD_EN)
		pr_info("\tPIO Read Enable...");
	if (present & ARASAN_PRESENT_STATE_BUFFER_WR_EN)
		pr_info("\tPIO Write Enable...");
	if (present & ARASAN_PRESENT_STATE_RD_ACTIVE)
		pr_info("\tRead Xfer data...");
	if (present & ARASAN_PRESENT_STATE_WR_ACTIVE)
		pr_info("\tWrite Xfer data...");
	if (present & ARASAN_PRESENT_STATE_DAT_ACTIVE)
		pr_info("\tDAT line active...");
#endif
	return ret;
}
static void arasan_set_clock(struct arasan_host *host, unsigned int freq)
{
	u16 clock = 0;
	unsigned long flags;

	/* 协商阶段400K-低速模式 */
	spin_lock_irqsave(&host->lock, flags);

	if ((host->freq != freq) && (freq)) {
		u16 divisor;

		/* Ensure clock is off before making any changes */
		//writew(clock, host->base + ARASAN_CLOCK_CTRL);

		/* core checks if this is a good freq < max_freq */
		host->freq = freq;

		printk("%s:\n\tnew freq %d", __func__, host->freq);

		/* Work out divisor for specified clock frequency */
		for (divisor = 1; divisor <= 256; divisor *= 2)
			/* Find first divisor producing a frequency less
			 * than or equal to MHz */
			if ((maxfreq / divisor) <= freq)
				break;

		DBG("\tdivisor %d", divisor);
	//	printk("\tdivisor %d\n", divisor);
		/* Set the clock divisor and enable the internal clock */
		clock = divisor << (ARASAN_CLOCK_CTRL_SDCLK_SHIFT);
		//clock = 0 << (ARASAN_CLOCK_CTRL_SDCLK_SHIFT);
		clock &= ARASAN_CLOCK_CTRL_SDCLK_MASK;
		clock |= ARASAN_CLOCK_CTRL_ICLK_ENABLE;
		writew(clock, host->base + ARASAN_CLOCK_CTRL);
	//	printk("\tread divisor %x\n", readw(host->base + ARASAN_CLOCK_CTRL));

		/* Busy wait for the clock to become stable */
		do { } while (((readw(host->base + ARASAN_CLOCK_CTRL)) &
			  ARASAN_CLOCK_CTRL_ICLK_STABLE) == 0);

		/* Enable the SD clock */
		clock |= ARASAN_CLOCK_CTRL_SDCLK_ENABLE;
		writew(clock, host->base + ARASAN_CLOCK_CTRL);

		DBG("\tclk ctrl reg. [0x%x]\n",
		    (unsigned int)readw(host->base + ARASAN_CLOCK_CTRL));
	}

	spin_unlock_irqrestore(&host->lock, flags);
}

/* Read the response from the card */
static void arasan_get_resp(struct mmc_command *cmd, struct arasan_host *host)
{
	unsigned int i;
	unsigned int resp[4];

	for (i = 0; i < 4; i++)
		resp[i] = readl(host->base + ARASAN_RSP(i));

	if (cmd->flags & MMC_RSP_136) {
		cmd->resp[3] = (resp[0] << 8);
		cmd->resp[2] = (resp[0] >> 24) | (resp[1] << 8);
		cmd->resp[1] = (resp[1] >> 24) | (resp[2] << 8);
		cmd->resp[0] = (resp[2] >> 24) | (resp[3] << 8);
	} else {
		cmd->resp[0] = resp[0];
		cmd->resp[1] = resp[1];
	}
	
	/* 数据出错之后，response寄存器当中仍然会有接收到响应数据 */
	//printk("resp[0]:%x\n",resp[0]);

	DBG("%s: resp length %s\n-(CMD%u):\n %08x %08x %08x %08x\n"
	    "-RAW reg:\n %08x %08x %08x %08x\n",
	    __func__, (cmd->flags & MMC_RSP_136) ? "136" : "48", cmd->opcode,
	    cmd->resp[0], cmd->resp[1], cmd->resp[2], cmd->resp[3],
	    resp[0], resp[1], resp[2], resp[3]);
}

int blocks;
static void arasan_read_block_pio(struct arasan_host *host)
{
	unsigned long flags;
	u16 blksz;

	DBG("\tPIO reading\n");

	local_irq_save(flags);

	for (blksz = host->pio_blksz; blksz > 0; blksz -= 4) {

		*host->pio_blkbuf =
		    readl(host->base + ARASAN_BUFF);
		host->pio_blkbuf++;
	}

	local_irq_restore(flags);
}

static void arasan_write_block_pio(struct arasan_host *host)
{
	unsigned long flags;
	u16 blksz;

	DBG("\tPIO writing\n");
	//printk("\tPIO writing\n");
	local_irq_save(flags);

	//printk("host->pio_blksize:%d\n", host->pio_blksz);
	for (blksz = host->pio_blksz; blksz > 0; blksz -= 4) {
		//writel(0x00,host->base + ARASAN_BUFF);

		writel(*host->pio_blkbuf,
		       host->base + ARASAN_BUFF);
		host->pio_blkbuf++;
	}

	local_irq_restore(flags);
}

static void arasan_data_pio(struct arasan_host *host)
{
	static int i = 0;
	static int j = 0;
	int sg_length = 0;
	static int size = 0;
	static int size1 = 0;
	if (host->pio_blocks == 0)
		return;


	if (host->status == STATE_DATA_READ) {
		host->pio_blkbuf = sg_virt(host->sg[i])+size;
		sg_length = sg_dma_len(host->sg[i]);
		//printk("rd-%d, %d, %p, %d\n", host->sg_frags, i,  host->pio_blkbuf, sg_length);

		while (readl(host->base + ARASAN_PRESENT_STATE) &
			     ARASAN_PRESENT_STATE_BUFFER_RD_EN) {

			arasan_read_block_pio(host);

			size += host->pio_blksz;
			//printk("size:%d\n", size);
			if (size == sg_length) {
				/* 下个sg */
				i++;
				size = 0;

				if (i==host->sg_frags) {
					//printk("hello world.\n");
					host->pio_blocks--;
					if (unlikely(host->pio_blocks != 0)) {
						printk("SD-MMC fatal error.\n");
					}
					i = 0;
					/* 数据全部操作完 */
					if (host->sg)
						kfree(host->sg);
					break;
				}
				host->pio_blkbuf = sg_virt(host->sg[i]);
				sg_length = sg_dma_len(host->sg[i]);
				//printk("rd-%d, %d, %p, %d\n", host->sg_frags, i,  host->pio_blkbuf, sg_length);
			}

			host->pio_blocks--;
			if (host->pio_blocks == 0)
				break;
		}
		//printk("rd-sg_frags:%d, cur:%d\n", host->sg_frags, i);

	} else {
		host->pio_blkbuf = sg_virt(host->sg[j])+size1;
		sg_length = sg_dma_len(host->sg[j]);
		//printk("wr-%d, %d, %p, %d\n", host->sg_frags, j,  host->pio_blkbuf, sg_length);
		while (readl(host->base + ARASAN_PRESENT_STATE) &
			     ARASAN_PRESENT_STATE_BUFFER_WR_EN) {

			arasan_write_block_pio(host);

			size1 += host->pio_blksz;
			if (size1 == sg_length) {
				/* 下个sg */
				j++;
				size1 = 0;

				if (j==host->sg_frags) {
					//printk("hello world.\n");
					host->pio_blocks--;
					if (unlikely(host->pio_blocks != 0)) {
						printk("SD-MMC fatal error.\n");
					}
					j = 0;

					/* 数据全部操作完 */
					if (host->sg)
						kfree(host->sg);
					break;
				}
				host->pio_blkbuf = sg_virt(host->sg[j]);
				sg_length = sg_dma_len(host->sg[j]);
				//printk("dwr-%d, %d, %p, %d, %d\n", host->sg_frags, j,  host->pio_blkbuf, sg_length, size1);
			}

			host->pio_blocks--;
			if (host->pio_blocks == 0)
				break;
		}
		//printk("wr-%d, %d, %p, %d, %d\n", host->sg_frags, j,  host->pio_blkbuf, sg_length, size1);
		//printk("wr-sg_frags:%d, cur:%d\n", host->sg_frags, j);
	}

	DBG("\tPIO transfer complete.\n");
}

static void arasan_start_cmd(struct arasan_host *host, struct mmc_command *cmd)
{
	u16 cmdreg = 0;

	/* Command Request */
	cmdreg = ARASAN_CMD_INDEX(cmd->opcode);
	DBG("%s: cmd type %04x,  CMD%d\n", __func__,
	    mmc_resp_type(cmd), cmd->opcode);

	if (cmd->flags & MMC_RSP_BUSY) {
		cmdreg |= ARASAN_CMD_RSP_48BUSY;
		DBG("\tResponse length 48 check Busy.\n");
	} else if (cmd->flags & MMC_RSP_136) {
		cmdreg |= ARASAN_CMD_RSP_136;
		DBG("\tResponse length 136\n");
	} else if (cmd->flags & MMC_RSP_PRESENT) {
		cmdreg |= ARASAN_CMD_RSP_48;
		DBG("\tResponse length 48\n");
	} else {
		cmdreg |= ARASAN_CMD_RSP_NONE;
		DBG("\tNo Response\n");
	}

	if (cmd->flags & MMC_RSP_CRC) {
		cmdreg |= ARASAN_CMD_CHECK_CMDCRC;
		DBG("\tCheck the CRC field in the response\n");
	}
	if (cmd->flags & MMC_RSP_OPCODE) {
		cmdreg |= ARASAN_CMD_INDX_CHECK;
		DBG("\tCheck the Index field in the response\n");
	}

	/* Wait until the CMD line is not in use */
	do { } while ((readl(host->base + ARASAN_PRESENT_STATE)) &
		 ARASAN_PRESENT_STATE_CMD_INHIBIT);

	/* Set the argument register */
	writel(cmd->arg, host->base + ARASAN_ARG);

	/* Data present and must be transferred */
	if (likely(host->mrq->data)) {
		cmdreg |= ARASAN_CMD_DATA_PRESENT;
		if (cmd->flags & MMC_RSP_BUSY)
			/* Wait for data inhibit */
			do { } while ((readl(host->base +
					ARASAN_PRESENT_STATE)) &
				 ARASAN_PRESENT_STATE_DAT_INHIBIT);
	}

	/* Write the Command */
	writew(cmdreg, host->base + ARASAN_CMD);

	DBG("\tcmd: 0x%x cmd reg: 0x%x - cmd->arg 0x%x, reg 0x%x\n",
	    cmdreg, readw(host->base + ARASAN_CMD), cmd->arg,
	    readl(host->base + ARASAN_ARG));
}

#ifdef ARASAN_DEBUG
static void arasan_adma_error(struct arasan_host *host)
{
	u8 status = readb(host->base + ARASAN_ADMA_ERR_STATUS);

	if (status & ARASAN_ADMA_ERROR_LENGTH)
		pr_err("-ADMA Length Mismatch Error...");

	if (status & ARASAN_ADMA_ERROR_ST_TFR)
		pr_err("-Transfer Data Error desc: ");
	else if (status & ARASAN_ADMA_ERROR_ST_FDS)
		pr_err("-Fetch Data Error desc: ");
	else if (status & ARASAN_ADMA_ERROR_ST_STOP)
		pr_err("-Stop DMA Data Error desc: ");

	pr_err("0x%x", readl(host->base + ARASAN_ADMA_ADDRESS));
}

static void arasan_adma_dump_desc(u8 *desc)
{
	__le32 *dma;
	__le16 *len;
	u8 attr;

	pr_info("\tDescriptors:");

	while (1) {
		dma = (__le32 *) (desc + 4);
		len = (__le16 *) (desc + 2);
		attr = *desc;

		pr_info("\t\t%p: Buff 0x%08x, len %d, Attr 0x%02x\n",
			desc, le32_to_cpu(*dma), le16_to_cpu(*len), attr);

		desc += 8;

		if (attr & 2)	/* END of descriptor */
			break;
	}
}
#else
static void arasan_adma_error(struct arasan_host *host)
{
}

static void arasan_adma_dump_desc(u8 *desc)
{
}
#endif

static int arasan_init_sg(struct arasan_host *host)
{

	host->adma_desc = kmalloc((ARASAN_DMA_DESC_NUM * 2 + 1) * 4, \
				  GFP_KERNEL);

	if (unlikely(host->adma_desc == NULL))
		return -ENOMEM;

	return 0;
}

static void arasan_adma_table_pre(struct arasan_host *host,
				  struct mmc_data *data)
{
	int direction, i;
	u8 *desc;
	struct scatterlist *sg;
	int len;
	dma_addr_t addr;

	if (host->status == STATE_DATA_READ)
		direction = DMA_FROM_DEVICE;
	else
		direction = DMA_TO_DEVICE;

	DBG("\t%s: sg entries %d\n", __func__, data->sg_len);

	/* 
	 * 得到scatterlist当中有多少个内在块需要传输
	 * 实际上sg_frags在正常的情况下的值 = data->sg_len
	 * -这个函数的实际目的是将虚拟地址映射成物理地址
	 * 这里和主调函数当中作的动作是一样的，所以这里可以屏蔽掉
	 */
	/*
	host->sg_frags = dma_map_sg(mmc_dev(host->mmc), data->sg,
				    data->sg_len, direction);
	*/
	desc = host->adma_desc;

	//printk("sg_frags:%d\n", host->sg_frags);
	for_each_sg(data->sg, sg, host->sg_frags, i) {
		addr = sg_dma_address(sg); /* 每一个scatter指定的DMA传输的地址 */
		len = sg_dma_len(sg);	   
		//printk("len:%d\n", len);
		//printk("addr:%x\n", addr);

		DBG("\t\tFrag %d: addr 0x%x, len %d\n", i, addr, len);

		/* Preparing the descriptor */
		desc[7] = (addr >> 24) & 0xff;
		desc[6] = (addr >> 16) & 0xff;
		desc[5] = (addr >> 8) & 0xff;
		desc[4] = (addr >> 0) & 0xff;

		desc[3] = (len >> 8) & 0xff;
		desc[2] = (len >> 0) & 0xff;

		desc[1] = 0x00;
		desc[0] = 0x21;

		desc += 8;
	}
	//printk("i=%d\n", i);
	/* 返回到最后一个descriptor,并将该descriptor设置为最后一个descriptor,
	 * 让sdio知道什么时候停止取descriptor 
	 */
	desc -= 8;
	desc[0] = 0x23;

	arasan_adma_dump_desc(host->adma_desc);

	/* 将kmalloc获得的descriptors的虚拟地址转换成物理地址 */
	host->adma_addr = dma_map_single(mmc_dev(host->mmc),
					 host->adma_desc,
					 (ARASAN_DMA_DESC_NUM * 2 + 1) * 4,
					 DMA_TO_DEVICE);

	writel(host->adma_addr, host->base + ARASAN_ADMA_ADDRESS);
}

static void arasan_adma_table_post(struct arasan_host *host,
				   struct mmc_data *data)
{
	int direction;

	if (host->status == STATE_DATA_READ)
		direction = DMA_FROM_DEVICE;
	else
		direction = DMA_TO_DEVICE;

	DBG("\t%s\n", __func__);

	dma_unmap_single(mmc_dev(host->mmc), host->adma_addr,
			 (ARASAN_DMA_DESC_NUM * 2 + 1) * 4, DMA_TO_DEVICE);

	dma_unmap_sg(mmc_dev(host->mmc), data->sg, data->sg_len, direction);
}

static int arasan_setup_data(struct arasan_host *host)
{
	u16 blksz=0;
	u16 xfer = 0;
	struct mmc_data *data = host->mrq->data;

	DBG("%s:\n\t%s mode, data dir: %s; Buff=0x%08x,"
	    "blocks=%d, blksz=%d\n", __func__, host->use_pio ? "PIO" : "DMA",
	    (data->flags & MMC_DATA_READ) ? "read" : "write",
	    (unsigned int)sg_virt(data->sg), data->blocks, data->blksz);

	/* Transfer Direction */
	if (data->flags & MMC_DATA_READ) {
		xfer |= ARASAN_XFER_DATA_DIR;
		host->status = STATE_DATA_READ;
	} else {
		xfer &= ~ARASAN_XFER_DATA_DIR;
		host->status = STATE_DATA_WRITE;
	}

	xfer |= ARASAN_XFER_BLK_COUNT_EN;

	if (data->blocks > 1) {
		if (host->auto_cmd12) // sd2.0
			xfer |= ARASAN_XFER_MULTI_BLK | ARASAN_XFER_AUTOCMD12;
		else // sdio2.0
			xfer |= ARASAN_XFER_MULTI_BLK;
	}

	//printk("blksz:%d\n", data->blksz);
	/* Set the block size register */
	//blksz = ARASAN_BLOCK_SIZE_SDMA_512KB;
	blksz |= (data->blksz & ARASAN_BLOCK_SIZE_TRANSFER);
	blksz |= (data->blksz & 0x1000) ? ARASAN_BLOCK_SIZE_SDMA_8KB : 0;

	//printk("blksz:%x\n", blksz);
	writew(blksz, host->base + ARASAN_BLK_SIZE);

	/* Set the block count register */
	writew(data->blocks, host->base + ARASAN_BLK_COUNT);
	//printk("blocks:%d\n", data->blocks);

	/* PIO mode is used when 'pio' var is set by the user or no
	 * sdma is available from HC caps. */
	if (unlikely(host->use_pio || (host->cap.sdma == 0))) {
		int i = 0;
		struct scatterlist *_sg;
		//int len;

		_sg = NULL;
		host->sg = NULL;
		host->sg_frags = dma_map_sg(mmc_dev(host->mmc), data->sg,
                                       data->sg_len,
                                       (host->status & STATE_DATA_READ) ?
                                       DMA_FROM_DEVICE : DMA_TO_DEVICE);

		host->sg = (struct scatterlist **)kmalloc(sizeof(struct scatterlist*)*host->sg_frags,GFP_KERNEL);
		if (unlikely(host->sg==NULL)) {
			printk("SD-MMC kmalloc failed.\n");
			return -ENOMEM;
		}

		//printk("sg_frags:%d\n", host->sg_frags);
		for_each_sg(data->sg, _sg, host->sg_frags, i) { 
			host->sg[i] = _sg;
			//printk("sg_len:%d\n",sg_dma_len(_sg));
		}

		/* PIO的数据传输在中断当中完成 */
		//printk("blksz:%d\n", blksz);
		//printk("blocks:%d\n", data->blocks);
		host->pio_blksz = data->blksz;
		host->pio_blocks = data->blocks;
		//host->pio_blkbuf = sg_virt(data->sg);
	} else {
		dma_addr_t phys_addr;

		/* Enable DMA */
		xfer |= ARASAN_XFER_DMA_EN;

		/* Scatter list init */
		host->sg_frags = dma_map_sg(mmc_dev(host->mmc), data->sg,
					    data->sg_len,
					    (host->status & STATE_DATA_READ) ?
					    DMA_FROM_DEVICE : DMA_TO_DEVICE);

		phys_addr = sg_dma_address(data->sg);

		if (likely(host->adma)) {
			/* Set the host control register dma bits for adma
			 * if supported and enabled by user. */
			host->ctrl |= ARASAN_HOST_CTRL_ADMA2_32;

			/* Prepare ADMA table */
			arasan_adma_table_pre(host, data);
		} else {
			/* SDMA Mode selected (default mode) */
			host->ctrl &= ~ARASAN_HOST_CTRL_ADMA2_64;

			writel((unsigned int)phys_addr,
			       host->base + ARASAN_SDMA_SYS_ADDR);
		}
		writeb(host->ctrl, host->base + ARASAN_HOST_CTRL);

	}
	/* Set the data transfer mode register */
	writew(xfer, host->base + ARASAN_XFER_MODE);

	DBG("\tHC Reg [xfer 0x%x] [blksz 0x%x] [blkcount 0x%x] [CRTL 0x%x]\n",
	    readw(host->base + ARASAN_XFER_MODE),
	    readw(host->base + ARASAN_BLK_SIZE),
	    readw(host->base + ARASAN_BLK_COUNT),
	    readb(host->base + ARASAN_HOST_CTRL));

	return 0;
}

static void arasan_finish_data(struct arasan_host *host)
{
	struct mmc_data *data = host->mrq->data;

	DBG("\t%s\n", __func__);

	if (unlikely(host->pio_blkbuf)) {
		host->pio_blksz = 0;
		host->pio_blocks = 0;
		host->pio_blkbuf = NULL;
	} else {
		if (likely(host->adma)) {
			arasan_adma_table_post(host, data);
		} else {
			dma_unmap_sg(mmc_dev(host->mmc), data->sg,
				     host->sg_frags,
				     (host->status & STATE_DATA_READ) ?
				     DMA_FROM_DEVICE : DMA_TO_DEVICE);
		}
	}

	data->bytes_xfered = data->blocks * data->blksz;
	host->status = STATE_CMD;
}

static int arasan_finish_cmd(unsigned int err_status, unsigned int status,
			     unsigned int opcode)
{
	int ret = 0;

	if (unlikely(err_status)) {
		if (err_status & ARASAN_CMD_TIMEOUT) {
			//printk("sdio cmd_timeout...\n");
			ret = -ETIMEDOUT;
		}
		if (err_status & ARASAN_CMD_CRC_ERROR) {
			printk("sdio cmd_crc_error...\n");
			ret = -EILSEQ;
		}
		if (err_status & ARASAN_CMD_END_BIT_ERROR) {
			printk("sdio cmd_end_bit_error...\n");
			ret = -EILSEQ;
		}
		if (err_status & ARASAN_CMD_INDEX_ERROR) {
			printk("sdio cmd_index_error...\n");
			ret = -EILSEQ;
		}
	}
	if (likely(status & ARASAN_N_CMD_COMPLETE))
		DBG("\tCommand (CMD%u) Completed irq...\n", opcode);

	return ret;
}

//#define ARASAN_IRQ_DEFAULT_MASK		0x02ff00fb
/* Enable/Disable Normal and Error interrupts */
static void aranan_enable_sdio_irq(struct mmc_host *mmc, int enable)
{
	unsigned long flags;
	struct arasan_host *host = mmc_priv(mmc);
	static unsigned int arasan_irq_mask;

	DBG("%s: %s CARD_IRQ\n", __func__, enable ? "enable" : "disable");
	//printk("%s: %s CARD_IRQ\n", __func__, enable ? "enable" : "disable");
	
	arasan_irq_mask = host->card_irq ?ARASAN_IRQ_DEFAULT_MASK: 
						ARASAN_IRQ_DEFAULT_MASK_NOCARDIRQ;
	spin_lock_irqsave(&host->lock, flags);
	if (enable)
		host->intr_en = arasan_irq_mask;
	else 
		host->intr_en = 0;

	writel(host->intr_en, host->base + ARASAN_NORMAL_INT_STATUS_EN);
	writel(host->intr_en, host->base + ARASAN_NORMAL_INT_SIGN_EN);
	spin_unlock_irqrestore(&host->lock, flags);
}

static void arasan_timeout_timer(unsigned long data)
{
	struct arasan_host *host = (struct arasan_host *)data;
	struct mmc_request *mrq;
	unsigned long flags;

	spin_lock_irqsave(&host->lock, flags);

	//printk("timeout.\n");
	//printk("host->status:%d\n", host->status);
	if ((host->mrq) && ((host->status==STATE_CMD) ||
				(host->status==STATE_DATA_READ) || 
				(host->status==STATE_DATA_WRITE))) {
		mrq = host->mrq;

		DBG("%s: Timeout waiting for hardware interrupt.\n",
			 mmc_hostname(host->mmc));

		writel(0xffffffff, host->base + ARASAN_NORMAL_INT_STATUS);


		if (mrq->data) {
			arasan_finish_data(host);
			arsan_sw_reset(host, reset_dat_line);
			mrq->data->error = -ETIMEDOUT;
		}
		if (likely(mrq->cmd)) {
			mrq->cmd->error = -ETIMEDOUT;
			arsan_sw_reset(host, reset_cmd_line);
			arasan_get_resp(mrq->cmd, host);
		}
		arasan_ctrl_led(host, 0);
		host->mrq = NULL;
		mmc_request_done(host->mmc, mrq);
	}
	spin_unlock_irqrestore(&host->lock, flags);
}

/* Process requests from the MMC layer */
static void arasan_request(struct mmc_host *mmc, struct mmc_request *mrq)
{
	/* 进入到这个函数时，mrq当中有用的就只剩下cmd和data */
	struct arasan_host *host = mmc_priv(mmc);
	struct mmc_command *cmd = mrq->cmd;
	unsigned long flags;

	BUG_ON(host->mrq != NULL);

	
	spin_lock_irqsave(&host->lock, flags);

	DBG(">>> araran_request:\n");
	/* Check that there is a card in the slot */
	if (unlikely(arasan_test_card(host) < 0)) {
		DBG("%s: Error: No card present...\n", mmc_hostname(host->mmc));

		mrq->cmd->error = -ENOMEDIUM;
		mmc_request_done(mmc, mrq); /* 来自core.c，通知上层操作已经完成 */
		spin_unlock_irqrestore(&host->lock, flags);
		return;
	}
	//printk(KERN_EMERG"The card is present.\n");

	host->mrq = mrq;

	host->status = STATE_CMD;
	if (likely(mrq->data))  {
		//printk("1\n");
		//mdelay(10000);
		arasan_setup_data(host);
	}


	/* Turn-on/off the LED when send/complete a cmd */
	arasan_ctrl_led(host, 1);

	//printk("start_cmd.\n");
	//mdelay(5000);
	arasan_start_cmd(host, cmd);

	mod_timer(&host->timer, jiffies + 5 * HZ);


	DBG("<<< araran_request done!\n");
	//printk("<<< araran_request done!\n");
	spin_unlock_irqrestore(&host->lock, flags);
}

static int arasan_get_ro(struct mmc_host *mmc)
{
	struct arasan_host *host = mmc_priv(mmc);

	u32 ro = readl(host->base + ARASAN_PRESENT_STATE);
	if (!(ro & ARASAN_PRESENT_STATE_WR_EN))
		return 1;

	return 0;
}

/* I/O bus settings (MMC clock/power ...) */
static void arasan_set_ios(struct mmc_host *mmc, struct mmc_ios *ios)
{
	struct arasan_host *host = mmc_priv(mmc);
	u8 ctrl_reg = readb(host->base + ARASAN_HOST_CTRL);

	DBG("%s: pwr %d, clk %d, vdd %d, bus_width %d, timing %d\n",
	    __func__, ios->power_mode, ios->clock, ios->vdd, ios->bus_width,
	    ios->timing);
	//printk(KERN_EMERG"clk:%d\n", ios->clock);

	/* Set the power supply mode */
	if (ios->power_mode == MMC_POWER_OFF) {
		arasan_power_set(host, 0, ios->vdd);
	}
	else {
		//printk("powerup.\n");
		//mdelay(5000);
		arasan_power_set(host, 1, ios->vdd);
		//printk("after powerup.\n");
		//mdelay(5000);
	}

	/* Timing (high speed supported?) */
	if ((ios->timing == MMC_TIMING_MMC_HS ||
	     ios->timing == MMC_TIMING_SD_HS) && host->cap.high_speed)
		/*
		 * 本调试板在高速模式下同样只能用下降沿输出
		 */
	{
		//ctrl_reg |= ARASAN_HOST_CTRL_HIGH_SPEED;
	}
    ctrl_reg |= ARASAN_HOST_CTRL_HIGH_SPEED;
	/* Clear the current bus width configuration */
	ctrl_reg &= ~ARASAN_HOST_CTRL_SD_MASK;

	/* Set SD bus bit mode */
	switch (ios->bus_width) {
	case MMC_BUS_WIDTH_8:
		ctrl_reg |= ARASAN_HOST_CTRL_SD8;
		break;
	case MMC_BUS_WIDTH_4:
		ctrl_reg |= ARASAN_HOST_CTRL_SD;
		break;
	/* added by me, 不能插拔识别卡的问题 */
	case MMC_BUS_WIDTH_1:
		ctrl_reg &= ~ARASAN_HOST_CTRL_SD;
		break;
	}

	/* Default to maximum timeout */
	writeb(0x0e, host->base + ARASAN_TIMEOUT_CTRL);

	/* Disable Card Interrupt in Host in case we change
	 * the Bus Width. */
	aranan_enable_sdio_irq(host->mmc, 0);

	host->ctrl = ctrl_reg;
	writeb(host->ctrl, host->base + ARASAN_HOST_CTRL);

	aranan_enable_sdio_irq(host->mmc, 1);

	/* Set clock */
	arasan_set_clock(host, ios->clock);
	//printk(KERN_EMERG"clk set done.\n");
	//mdelay(5000);
}

/* Tasklet for Card-detection */
static void arasan_tasklet_card(unsigned long data)
{
	unsigned long flags;
	struct arasan_host *host = (struct arasan_host *)data;

	spin_lock_irqsave(&host->lock, flags);

	if (likely((readl(host->base + ARASAN_PRESENT_STATE) &
		    ARASAN_PRESENT_STATE_CARD_PRESENT))) {
		if (host->mrq) {
		//	printk("card_detection.\n");
			pr_err("%s: Card removed during transfer!\n",
			       mmc_hostname(host->mmc));
			/* Reset cmd and dat lines */
			arsan_sw_reset(host, reset_cmd_line);
			arsan_sw_reset(host, reset_dat_line);

			if (likely(host->mrq->cmd)) {
				struct mmc_request *mrq = host->mrq;
				mrq->cmd->error = -ENOMEDIUM;
				host->mrq = NULL;
				mmc_request_done(host->mmc, mrq);
			}
		}
	}

	spin_unlock_irqrestore(&host->lock, flags);

	if (likely(host->mmc))
		mmc_detect_change(host->mmc, msecs_to_jiffies(200));
}

static void arasan_setup_hc(struct arasan_host *host);
static irqreturn_t arasan_irq(int irq, void *dev)
{
	struct arasan_host *host = dev;
	unsigned int status, err_status, handled = 0;
	unsigned short int_en;
	struct mmc_command *cmd = NULL;
	struct mmc_data *data = NULL;

	/* host->lock已经有加锁机制，所以这个变量不用担心互斥的产生 */
	//static int get_card_irq = 0;

	spin_lock(&host->lock);

	/* Interrupt Status */
	status = readl(host->base + ARASAN_NORMAL_INT_STATUS);
	err_status = (status >> 16) & 0xffff;

	/* 清中断状态寄存器 */
	DBG("\tclear status and exit...\n");
	writel(status, host->base + ARASAN_NORMAL_INT_STATUS);

	if (likely(status & ARASAN_N_CARD_IRQ)) {
		/* 进入这个中断时不能直接退出，因为这个接收到这个中断
		 * 时，还可能接收到数据完成中断
		 */
		/*
		 * mmc_signal_sdio_irq(host->mmc);
		 * 这让人崩溃，这个函数里面将所有的中断关闭掉，在这期间是会有
		 * 其它的中断的(这是由于实现不同造成的), 所以这里用下面的方法
		 * 取代，只关闭sdio中断
		 */
		int_en = readl(host->base+ARASAN_NORMAL_INT_STATUS_EN);
		int_en &= ~(0x1<<8);

		writel(int_en, host->base + ARASAN_NORMAL_INT_STATUS_EN);
		/*
		 * 替代 wake_up_prcess,因为我们对外设由电源开关，而在
		 * 本分代码的逻辑上是先打开中断，然后在打开开关，这就造成
		 * 在打开中断时，dat1为低，造成中断，但此时，sdio_irq_thread
		 * 还未被赋值
		 */
		host->mmc->sdio_irq_pending = true;
		if (host->mmc && host->mmc->sdio_irq_thread)
			wake_up_process(host->mmc->sdio_irq_thread);

	}

#if 0
	if ((status&ARASAN_N_CARD_REMOVAL) ||
			(status&ARASAN_N_CARD_INS)) {
		/* 防止中断抖动 */
		int_en = readw(host->base + ARASAN_NORMAL_INT_SIGN_EN);
		//printk("int_en:%x\n", int_en);
		int_en = (int_en & (~(0x3<<6)));
		//printk("int_en:%x\n", int_en);
		writew(int_en, host->base+ARASAN_NORMAL_INT_SIGN_EN);
	}
#endif


	DBG("%s: Normal IRQ status  0x%x, Error status 0x%x\n",
	    __func__, status & 0xffff, err_status);

	//printk("z\n");
	/*
	printk(KERN_EMERG"%s: Normal IRQ status  0x%x, Error status 0x%x\n",
	    __func__, status & 0xffff, err_status);
	*/
	//printk("arasan_irq.\n");
	//

	if ((!host->need_poll) &&
		    ((status & ARASAN_N_CARD_INS) || 
		     (status & ARASAN_N_CARD_REMOVAL))) {
			tasklet_schedule(&host->card_tasklet);
	}

	if (unlikely(!host->mrq)) {
		goto out;
	}
	

	cmd = host->mrq->cmd;
	data = host->mrq->data;

	cmd->error = 0;
	/* Check for any CMD interrupts */
	if (likely(status & ARASAN_INT_CMD_MASK)) {
		//printk("response to cmd.\n");
		cmd->error = arasan_finish_cmd(err_status, status, cmd->opcode);
		if (cmd->error)
			arsan_sw_reset(host, reset_cmd_line);

		if ((host->status == STATE_CMD) || cmd->error) {
			arasan_get_resp(cmd, host);

			handled = 1;
		}
	}

	/* Check for any data interrupts */
	if (likely((status & ARASAN_INT_DATA_MASK)) && data) {
		data->error = 0;
		if (unlikely(err_status)) {
			//printk("error.\n");
			if (err_status & ARASAN_DATA_TIMEOUT_ERROR) {
				DBG("\tdata_timeout_error...\n");
				data->error = -ETIMEDOUT;
			}
			if (err_status & ARASAN_DATA_CRC_ERROR) {
				DBG("\tdata_crc_error...\n");
				//printk("\tdata_crc_error...\n");
				data->error = -EILSEQ;
				
			}
			if (err_status & ARASAN_DATA_END_ERROR) {
				DBG("\tdata_end_error...\n");
				data->error = -EILSEQ;
			}
			if (err_status & ARASAN_AUTO_CMD12_ERROR) {
				unsigned int err_cmd12 =
				    readw(host->base + ARASAN_CMD12_ERR_STATUS);

				DBG("\tc12err 0x%04x\n", err_cmd12);

				if (err_cmd12 & ARASAN_AUTOCMD12_ERR_NOTEXE)
					data->stop->error = -ENOEXEC;

				if ((err_cmd12 & ARASAN_AUTOCMD12_ERR_TIMEOUT)
				    && !(err_cmd12 & ARASAN_AUTOCMD12_ERR_CRC))
					/* Timeout Error */
					data->stop->error = -ETIMEDOUT;
				else if (!(err_cmd12 &
					   ARASAN_AUTOCMD12_ERR_TIMEOUT)
					 && (err_cmd12 &
					     ARASAN_AUTOCMD12_ERR_CRC))
					/* CRC Error */
					data->stop->error = -EILSEQ;
				else if ((err_cmd12 &
					  ARASAN_AUTOCMD12_ERR_TIMEOUT)
					 && (err_cmd12 &
					     ARASAN_AUTOCMD12_ERR_CRC))
					DBG("\tCMD line Conflict\n");
			}
			arsan_sw_reset(host, reset_dat_line);
			handled = 1;
		} else {
			if (likely(((status & ARASAN_N_BUFF_READ) ||
				    status & ARASAN_N_BUFF_WRITE))) {
				DBG("\tData R/W interrupts...\n");
				//printk("\tData R/W interrupts...\n");
				arasan_data_pio(host); /* 使用pio的方式进行数据传输
							  必须在中断当中手动将数据
							  全部写入到buff当中去
							  */
			}

			if (likely(status & ARASAN_N_DMA_IRQ))
				DBG("\tDMA interrupts...\n");

			if (likely(status & ARASAN_N_TRANS_COMPLETE)) {
				//printk("done.\n");
				DBG("\tData XFER completed interrupts...\n");
				arasan_finish_data(host);
				if (data->stop) {
					u32 opcode = data->stop->opcode;
					data->stop->error =
					    arasan_finish_cmd(err_status,
							      status, opcode);
					arasan_get_resp(data->stop, host);
				}
				handled = 1;
			}

		}
	}
	if (err_status & ARASAN_ADMA_ERROR) {
		//printk("3.\n");
		DBG("\tADMA Error...\n");
		arasan_adma_error(host);
		cmd->error = -EIO;
	}
	if (err_status & ARASAN_CURRENT_LIMIT_ERROR) {
		//printk("4.\n");
		DBG("\tPower Fail...\n");
		cmd->error = -EIO;
	}

	if (likely(host->mrq && handled)) {
		//printk("5.\n");
		struct mmc_request *mrq = host->mrq;

		arasan_ctrl_led(host, 0);

		del_timer(&host->timer);

		host->mrq = NULL;
		//DBG("\tcalling mmc_request_done...\n");
		mmc_request_done(host->mmc, mrq);
	}
out:

#if 0
	if ((status&ARASAN_N_CARD_REMOVAL) ||
			(status&ARASAN_N_CARD_INS)) {
		int_en = (int_en | (0x3<<6));
		//printk("int_en:%x\n", int_en);
		writel(int_en, host->base + ARASAN_NORMAL_INT_STATUS_EN);
		writew(int_en, host->base+ARASAN_NORMAL_INT_SIGN_EN);
	}
#endif

	spin_unlock(&host->lock);

	return IRQ_HANDLED;
}

static void arasan_setup_hc(struct arasan_host *host)
{
	/* Clear all the interrupts before resetting */
	arasan_clear_interrupts(host);

	/* Reset All and get the HC version */
	arsan_sw_reset(host, reset_all);

	/* Print HC version and SPEC */
	arsan_hc_version(host);

	/* Set capabilities and print theri info */
	arasan_capabilities(host);

	//printk("before arasan_set_interrupt.\n");
	//mdelay(10000);

	/* Enable interrupts */
	//arasan_set_interrupts(host);

	//printk("arasan_set_interrupt.\n");
	//mdelay(10000);
}

static const struct mmc_host_ops arasan_ops = {
	.request = arasan_request,
	.get_ro = arasan_get_ro, /* 是否写保护 */
	.set_ios = arasan_set_ios,
	.enable_sdio_irq = aranan_enable_sdio_irq,
};

static int arasan_probe(struct platform_device *pdev)
{
	struct mmc_host *mmc = NULL;
	struct arasan_host *host = NULL;
	const struct arasan_platform_data *arasan_data;
	struct resource *r, *r1;
	int ret, irq;


	//printk(KERN_EMERG"platform_get_resource.\n");
	r = platform_get_resource(pdev, IORESOURCE_MEM, 0);

	r1 = platform_get_resource(pdev, IORESOURCE_IRQ, 0);

	//printk(KERN_EMERG"platform_get_irq_byname.\n");
	//irq = platform_get_irq_byname(pdev, "sdio1_irq");
	irq = r1->start;

	//printk(KERN_EMERG"irq:%d\n", irq);
	//printk(KERN_EMERG"start:%x\n", r->start);
	arasan_data = pdev->dev.platform_data;

	if (!r || irq < 0 || !arasan_data)
		return -ENXIO;

	/* wifi pdn 引脚 */
	if (arasan_data->p_powerup != NULL) {
		arasan_data->p_powerup();
	} else {
		//printk("needn't powerup.\n");
	}


	//printk(KERN_EMERG"request_mem_region.\n");
	r = request_mem_region(r->start, resource_size(r), pdev->name);
	if (!r) {
		pr_err("%s: ERROR: memory allocation failed\n", __func__);
		return -EBUSY;
		goto out;
	}
	//printk(KERN_EMERG"mmc_alloc_host.\n");
	/* Allocate the mmc_host with private data size */
	mmc = mmc_alloc_host(sizeof(struct arasan_host), &pdev->dev);
	if (!mmc) {
		pr_err("%s: ERROR: mmc_alloc_host failed\n", __func__);
		ret = -ENOMEM;
		goto out;
	}

	/* Verify resource from the platform */
	//printk(KERN_EMERG"arasan_claim_host.\n");
	ret = arasan_claim_resource(pdev);
	if (ret < 0)
		goto out;

	host = mmc_priv(mmc);
	host->mmc = mmc;
	host->dev = &pdev->dev;
	host->res = r;

	/* card_irq
	 * sd卡不接收card_irq
	 */
	host->card_irq = arasan_data->card_irq;

	/* auto_cmd12
	 * 区别sdio2.0和sd2.0的多块传输
	 * sdio2.0-cmd52
	 */
	host->auto_cmd12 = arasan_data->auto_cmd12;

	/* use_pio
	 * 使用主机读取buf，还是DMA完成数据的传输 
	 */
	host->use_pio = arasan_data->use_pio;
	//printk("host->use_pio=%d\n", host->use_pio);

	/*
	 * need_detect 
	 * wifi和sd卡使用同一个驱动，sd卡需要进行探测，而wifi不需要
	 * 这里影响每次发送请求时是否判断卡的存在
	 */
	host->need_detect = arasan_data->need_detect;
	//printk("host->need_detect=%d\n", host->need_detect);
	
	host->sdio_4bit_data = arasan_data->sdio_4bit_data;

	/* 
	 * need_poll 表示是否由子系统来完成卡的探测 ,
	 * 这里从表面上看起来其实是一样的，因为arasan_tasklet_card也是启动
	 * host->detect工作队列
	 */
	host->need_poll = arasan_data->need_poll;
	if (host->need_poll) {
		mmc->caps |= MMC_CAP_NEEDS_POLL;
		DBG("\tHC needs polling to detect the card...");
	} else
		/* no set the MMC_CAP_NEEDS_POLL in cap */
		tasklet_init(&host->card_tasklet, arasan_tasklet_card,
			     (unsigned long)host);

	//printk(KERN_EMERG"ioremap.\n");
	//printk(KERN_EMERG"r->start:%x\n", r->start);
	host->base = ioremap(r->start, resource_size(r));
	if (!host->base) {
		pr_err("%s: ERROR: memory mapping failed\n", __func__);
		ret = -ENOMEM;
		goto out;
	}
	//printk(KERN_EMERG"host->base:%x\n", host->base);

	//printk(KERN_EMERG"request_irq.\n");
	ret =
	    request_irq(irq, arasan_irq, IRQF_SHARED, ARASAN_DRIVER_NAME, host);
	if (ret) {
		pr_err("%s: cannot assign irq %d\n", __func__, irq);
		goto out;
	} else
		host->irq = irq;

	spin_lock_init(&host->lock);

	/* Setup the Host Controller according to its capabilities */
	//printk(KERN_EMERG"arasan_setup_hc.\n");
	arasan_setup_hc(host);

	mmc->ops = &arasan_ops;

	/* 这些支持的电压值会被用做探测过程中协商host与sd卡的电压 */
	if (host->cap.voltage33)
		mmc->ocr_avail |= MMC_VDD_32_33 | MMC_VDD_33_34;
	if (host->cap.voltage30)
		mmc->ocr_avail |= MMC_VDD_29_30;
	if (host->cap.voltage18)
		mmc->ocr_avail |= MMC_VDD_165_195;

	mmc->caps = MMC_CAP_SDIO_IRQ; /* sdio 使用 */

	/*
	 * 在探测时不使用spi模式
	 */
#if 0
	 if (host->cap.spi)
	 	mmc->caps |= MMC_CAP_SPI;
#endif
	/* mmc子系统会根据这个值来确认是否设置host的4bits/8bits总线宽度 */
    if (host->sdio_4bit_data) {
	    mmc->caps |= MMC_CAP_4_BIT_DATA | MMC_CAP_8_BIT_DATA;
    }

	/* mmc子系统会根据这个值来确认是否设置host的高速模式 */
    if (!host->card_irq) {
	    if (host->cap.high_speed)
		    mmc->caps |= MMC_CAP_MMC_HIGHSPEED | MMC_CAP_SD_HIGHSPEED;
    }

	/* 在子系统当中没神马用 */
	host->freq = host->cap.timer_freq * 1000000;
	//host->use_pio = pio;
	
	/* 在子系统当中没多大用，只是填充一个字段，上层想要知道这个值时，能够有一种方法获取 */
	mmc->f_max = maxfreq;
	mmc->f_min = mmc->f_max / 256;

	/*
	 * Maximum block size. This is specified in the capabilities register.
	 */
	mmc->max_blk_size = host->cap.max_blk_len;
	/* 确认一次最多传送多少块 */
	mmc->max_blk_count = 65535;

	/* 
	 * 1表示接口一次只能完成一段数据的传输，另外的值表示能够完成多次传输 
	 * 根据max_seq_size的定义和影响，可以看出max_segs*max_seg_size最大为max_seq_size
	 * 8*65536  = 524288, 那为什么这里的值，我却设置为16啦？因为不是每个segment都能达
	 * 到65536,根据实验发现它们不是所有的bio segment都能顺利和合并成65536，而这是因为
	 * 这些segment可能在物理上并不连续，所以要设置为大于8,在最坏情况下,每个页框都不能
	 * 合并，那么这个值应该为 524288/4096 = 128
	 *
	 * 另外这个值的设置也和一次request的最大长度有关，虽然没有得到确切的理论依据，但
	 * 根据试验结果来看，如果这个值设置过小,request的最大长度就不能达到下面设置的max_req_size
	 *
	 * 这里对性能有性能有一定的提升，读可提升10%-%15,写也提升可达10%左右
	 *
	 */
	mmc->max_segs = 128;
	//mmc->max_segs = 1;

	/* 一个segment即一次单位DMA传输，即表示一次单位DMA传输能传输多少数据,通常这个值为65535 */
	mmc->max_seg_size = 65536;

	/* 
	 * 和这个参数相关的参数包括:max_hw_sector, max_sectors
	 * max_hw_sector则来自于max_seg_size/512,max_sector来自于max_hw_sectors和
	 * BLK_DEF_MAX_SECTORS(1024)中较小的一个.
	 *
	 * 对于上层而言，它关心的参数是max_sector，它代表在通用块层一次最大能完成多少传输，可以看出这个值最大
	 * 最大为1024,所以这个值设置成大于1024*512以上的值没有意义，为了增大吞吐量，这个值设为最大1024*512
	 */
	//mmc->max_req_size = mmc->max_blk_count*mmc->max_blk_size;
	mmc->max_req_size = 524288;

	/* Passing the "pio" option, we force the driver to not
	 * use any DMA engines. */
	if (unlikely(host->use_pio)) {
		adma = 0;
		DBG("\tPIO mode\n");
		printk("\tPIO mode\n");
	} else {
		if (likely(adma)) {
			/* Turn-on the ADMA if supported by the HW
			 * or Fall back to SDMA in case of failures */
			DBG("\tADMA mode\n");
			//printk("\tADMA mode\n");
			//printk(KERN_EMERG"arasan_init_sg.\n");
			ret = arasan_init_sg(host);
			if (unlikely(ret)) {
				pr_warning("\tSG init failed (disable ADMA)\n");
				adma = 0;
			} else {
				/* Set the Maximum number of segments
				 * becasue we can do scatter/gathering in ADMA
				 * mode. */
				//mmc->max_hw_segs = 128;
			}
		} else
			DBG("\tSDMA mode\n");
	}
	host->adma = adma;

	//printk("before add host.\n");
	//mdelay(10000);

	platform_set_drvdata(pdev, mmc);
	ret = mmc_add_host(mmc);
	if (ret)
		goto out;

	/* 
	 * 定义一个定时器，超时时间在mod_timer当中定义
	 * */
	//printk(KERN_EMERG"setup_timer.\n");
	setup_timer(&host->timer, arasan_timeout_timer, (unsigned long)host);

	pr_info("%s: driver initialized... IRQ: %d, Base addr 0x%x\n",
		mmc_hostname(mmc), irq, (unsigned int)host->base);

//#ifdef ARASAN_DEBUG
	led = 1;
//#endif
	return 0;
out:
	if (host) {
		if (host->irq)
			free_irq(host->irq, host);
		if (host->base)
			iounmap(host->base);
	}
	if (r)
		release_resource(r);
	if (mmc)
		mmc_free_host(mmc);

	return ret;
}

static int arasan_remove(struct platform_device *pdev)
{
	struct mmc_host *mmc = platform_get_drvdata(pdev);

	if (mmc) {
		struct arasan_host *host = mmc_priv(mmc);

		arasan_clear_interrupts(host);
		free_irq(host->irq, host);
		if (!host->need_poll) {
			tasklet_kill(&host->card_tasklet);
		}
		mmc_remove_host(mmc);
		arasan_power_set(host, 0, -1);
		iounmap(host->base);
		if (likely(host->adma))
			kfree(host->adma_desc);
		release_resource(host->res);
		mmc_free_host(mmc);
	}
	platform_set_drvdata(pdev, NULL);
	return 0;
}

static struct platform_driver arasan_driver = {
	.probe  = arasan_probe,
	.remove = arasan_remove,
	.driver = {
		.name = ARASAN_DRIVER_NAME,
		.owner = THIS_MODULE,
	},
};


static int __init arasan_init(void)
{
	//printk(KERN_EMERG"arasan_init.\n");
	//platform_device_register(&arasan1_device);
	//printk(KERN_EMERG"platform_river_register.\n");
	return platform_driver_register(&arasan_driver);
}

static void __exit arasan_exit(void)
{
	//platform_device_unregister(&arasan1_device);
	platform_driver_unregister(&arasan_driver);
}

#if 0
static int __init arasan_cmdline_opt(char *str)
{
	char *opt;

	if (!str || !*str)
		return -EINVAL;

	while ((opt = strsep(&str, ",")) != NULL) {
		if (!strncmp(opt, "maxfreq:", 8))
			strict_strtoul(opt + 8, 0, (unsigned long *)&maxfreq);
		else if (!strncmp(opt, "adma:", 5))
			strict_strtoul(opt + 5, 0, (unsigned long *)&adma);
		else if (!strncmp(opt, "led:", 4))
			strict_strtoul(opt + 4, 0, (unsigned long *)&led);
		else if (!strncmp(opt, "pio:", 4))
			strict_strtoul(opt + 4, 0, (unsigned long *)&pio);
	}
	return 0;
}

__setup("arasanmmc=", arasan_cmdline_opt);
#endif

module_init(arasan_init);
module_exit(arasan_exit);

MODULE_AUTHOR("Giuseppe Cavallaro <peppe.cavallaro@xxxxxx>");
MODULE_DESCRIPTION("Arasan MMC/SD/SDIO Host Controller driver");
MODULE_LICENSE("GPL");
