/*
 * SPI interface for Faraday SPI020
 *
 * Copyright (c) 2011 BingJiun Luo <bjluo@faraday-tech.com>
 * Copyright (c) 2009 Po-Yu Chuang <ratbert@faraday-tech.com>
 *
 * Copyright (c) 2006 Ben Dooks
 * Copyright (c) 2006 Simtec Electronics
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
*/

#include <linux/version.h>
#include <linux/module.h>
#include <linux/spinlock.h>
#include <linux/platform_device.h>
#include <linux/dma-mapping.h>
#include <linux/interrupt.h>
#include <linux/workqueue.h>
#include <linux/delay.h>
#include <linux/sched.h>
#include <linux/spi/spi.h>
#include <mach/ftpmu010.h>
#include <mach/platform/board.h>
#include <mach/fmem.h>
#include "ftspi020.h"
#include <asm/io.h>

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,27))
#include <mach/hardware.h>
#else
#include <asm/hardware.h>
#endif

//#define CONFIG_FTSPI020_USE_AHBDMA//???

#if defined(CONFIG_FTSPI020_USE_AHBDMA) || defined(CONFIG_FTSPI020_USE_AXIDMA)
#include <linux/dma-mapping.h>
#include <linux/dmaengine.h>

#ifdef CONFIG_FTSPI020_USE_AHBDMA
#include <mach/ftdmac020.h>
#else
#include <mach/ftdmac030.h>
#endif

#ifdef CONFIG_PLATFORM_GM8210
#ifdef CONFIG_FTSPI020_USE_AHBDMA
#define AHBMASTER_R_SRC   FTDMA020_AHBMASTER_0
#define AHBMASTER_R_DST   FTDMA020_AHBMASTER_0
#define AHBMASTER_W_SRC   FTDMA020_AHBMASTER_0
#define AHBMASTER_W_DST   FTDMA020_AHBMASTER_0
#define SPI020_REQ  8
#else
#define SPI020_REQ  1
#endif
#endif

#if defined(CONFIG_PLATFORM_GM8139) || defined(CONFIG_PLATFORM_GM8287) || defined(CONFIG_PLATFORM_GM8136) || defined(CONFIG_PLATFORM_GM8220)
#define AHBMASTER_R_SRC   FTDMA020_AHBMASTER_0
#define AHBMASTER_R_DST   FTDMA020_AHBMASTER_1
#define AHBMASTER_W_SRC   FTDMA020_AHBMASTER_1
#define AHBMASTER_W_DST   FTDMA020_AHBMASTER_0
#define SPI020_REQ  8
#endif

static void ftspi020_dma_callback(void *param);
static unsigned int trigger_flag = 0;
static wait_queue_head_t spi020_queue;
#endif

static int spi020_fd;

/******************************************************************************
 * SPI020 definitions
 *****************************************************************************/
#define	FTSPI020_REG_CMD0		0x00	/* Flash address */
#define	FTSPI020_REG_CMD1		0x04
#define	FTSPI020_REG_CMD2		0x08
#define	FTSPI020_REG_CMD3		0x0c
#define	FTSPI020_REG_CTRL		0x10	/* Control */
#define	FTSPI020_REG_AC_TIME		0x14
#define	FTSPI020_REG_STS		0x18	/* Status */
#define	FTSPI020_REG_ICR		0x20	/* Interrupt Enable */
#define	FTSPI020_REG_ISR		0x24	/* Interrupt Status */
#define	FTSPI020_REG_READ_STS		0x28
#define FTSPI020_REG_REVISION		0x50
#define FTSPI020_REG_FEATURE		0x54
#define FTSPI020_REG_DATA_PORT		0x100


/*
 * Control Register offset 0x10
 */
#define	FTSPI020_CTRL_READY_LOC_MASK	~(0x7 << 16)
#define	FTSPI020_CTRL_READY_LOC(x)	(((x) & 0x7) << 16)

#define	FTSPI020_CTRL_ABORT		(1 << 8)

#define	FTSPI020_CTRL_CLK_MODE_MASK	~(1 << 4)
#define	FTSPI020_CTRL_CLK_MODE_0	(0 << 4)
#define	FTSPI020_CTRL_CLK_MODE_3	(1 << 4)

#define	FTSPI020_CTRL_CLK_DIVIDER_MASK	~(3 << 0)
#define	FTSPI020_CTRL_CLK_DIVIDER_2	(0 << 0)
#define FTSPI020_CTRL_CLK_DIVIDER_4	(1 << 0)
#define FTSPI020_CTRL_CLK_DIVIDER_6	(2 << 0)
#define FTSPI020_CTRL_CLK_DIVIDER_8	(3 << 0)

/*
 * Status Register offset 0x18
 */
#define	FTSPI020_STS_RFR	(1 << 1) /* RX FIFO ready */
#define	FTSPI020_STS_TFR	(1 << 0) /* TX FIFO ready */

/*
 * Interrupt Control Register
 */
#define	FTSPI020_ICR_RFTH(x)	(((x) & 0x3) << 12)	/* RX FIFO threshold interrupt */
#define	FTSPI020_ICR_TFTH(x)	(((x) & 0x3) << 8)	/* TX FIFO threshold interrupt */
#define	FTSPI020_ICR_INT	(1 << 1)		/* INT enable */
#define	FTSPI020_ICR_DMA	(1 << 0)		/* DMA handshake enable */

/*
 * Interrupt Status Register
 */
#define	FTSPI020_ISR_CMD_CMPL	(1 << 0)	/* Command complete interrupt  */

/*
 * Feature Register
 */
#define FTSPI020_FEATURE_CLK_MODE(reg)		(((reg) >> 25) & 0x1)
#define FTSPI020_FEATURE_DTR_MODE(reg)		(((reg) >> 24) & 0x1)
#define FTSPI020_FEATURE_CMDQ_DEPTH(reg)	(((reg) >> 16) & 0xff)
#define FTSPI020_FEATURE_RXFIFO_DEPTH(reg)	(((reg) >>  8) & 0xff)
#define FTSPI020_FEATURE_TXFIFO_DEPTH(reg)	(((reg) >>  0) & 0xff)

static int g_cmd_complete;

/******************************************************************************
 * spi_master (controller) private data
 *****************************************************************************/
struct ftspi020_ctrl {
	spinlock_t		lock;

	void __iomem		*base;
	int			irq;
	int			rxfifo_depth;
	int			txfifo_depth;

	struct spi_master	*master;

	struct workqueue_struct	*workqueue;
	struct work_struct	work;

	wait_queue_head_t	waitq;
	struct list_head	message_queue;

#if defined(CONFIG_FTSPI020_USE_AHBDMA) || defined(CONFIG_FTSPI020_USE_AXIDMA)
    struct dma_chan     *dma_chan;
    dma_cap_mask_t      cap_mask;
#ifdef CONFIG_FTSPI020_USE_AHBDMA
    struct ftdmac020_dma_slave  dma_slave_config;
#else
    struct ftdmac030_dma_slave  dma_slave_config;
#endif
    dma_cookie_t        cookie;
    dma_addr_t 		    mem_dmaaddr;
    dma_addr_t 		    spi_dmaaddr;
    unsigned char       *mem_dmabuf;
    unsigned char       *sg_dmabuf;
#endif
};

/******************************************************************************
 * internal functions for FTSPI020
 *****************************************************************************/
#ifdef CONFIG_PLATFORM_GM8210
static pmuReg_t  pmu_reg[] = {
    /* off, bitmask,   lockbit,    init val,   init mask */
    {0x5C, (0x1 << 24), (0x1 << 24), (0x1 << 24), (0x1 << 24)},   /* pinMux with GPIO */
#ifdef CONFIG_FTSPI020_USE_AXIDMA
    {0xA4, (0x1 << 26), (0x1 << 26), (0x1 << 26), (0x1 << 26)},   /* DMA ack selection */
#endif
    {0xB4, (0x1 << 9), (0x1 << 9), (0x0 << 9), (0x1 << 9)},   /* AHB clock gate */
};
#endif
#ifdef CONFIG_PLATFORM_GM8139
static pmuReg_t  pmu_reg[] = {
    /* off, bitmask,   lockbit,    init val,   init mask */
    {0x54, (0x3F << 26), 0, (0x15 << 26), (0x3F << 26)},   /* pinMux with NAND, don't lock */
    {0x58, (0x3 << 0), 0, (0x1 << 0), (0x3 << 0)},   /* pinMux with NAND, don't lock */
    {0xB4, (0x1 << 15), (0x1 << 15), (0x0 << 15), (0x1 << 15)},   /* AHB clock gate */
};
#endif
#ifdef CONFIG_PLATFORM_GM8136
static pmuReg_t  pmu_reg[] = {
    /* off, bitmask,   lockbit,    init val,   init mask */
    {0xB4, (0x1 << 15), (0x1 << 15), (0x0 << 15), (0x1 << 15)},   /* AHB clock gate */
};
#endif
#ifdef CONFIG_PLATFORM_GM8287
static pmuReg_t  pmu_reg[] = {
    /* off, bitmask,   lockbit,    init val,   init mask */
    {0x5C, (0x3 << 20), (0x3 << 20), (0x0 << 26), (0x3 << 20)},   /* pinMux */
    {0xB4, (0x1 << 9), (0x1 << 9), (0x0 << 9), (0x1 << 9)},   /* AHB clock gate */
};
#endif
#ifdef CONFIG_PLATFORM_GM8220
static pmuReg_t  pmu_reg[] = {
    /* off, bitmask,   lockbit,    init val,   init mask */
    {0x1A4, (0xFFFF << 14), (0xFFFF << 14), (0x5555 << 14), (0xFFFF << 14)},   /* pinMux */
    //{0x1A4, (0xFFFF << 14), (0xFFFF << 14), (0xAAAA << 14), (0xFFFF << 14)},   /* pinMux, 4-bit mode */
    {0x68, (0x1 << 17), (0x1 << 17), (0x0 << 17), (0x1 << 17)},   /* AHB clock gate */
};
#endif

static pmuRegInfo_t	pmu_reg_info = {
    "SPI020",
    ARRAY_SIZE(pmu_reg),
    ATTR_TYPE_NONE,
    pmu_reg
};

#if defined(CONFIG_FTSPI020_USE_AHBDMA) || defined(CONFIG_FTSPI020_USE_AXIDMA)

#define FTSPI020_DMA_BUF_SIZE	(4 * 1024)

static void ftspi020_dma_enable(void __iomem * base)
{
	unsigned int	tmp = inl(base + FTSPI020_REG_ICR);

	tmp |= FTSPI020_ICR_DMA;
	outl(tmp, base + FTSPI020_REG_ICR);
	printk(KERN_INFO "FTSPI020 enable DMA handshake 0x%x\n", inl(base + FTSPI020_REG_ICR));
}

static int ftspi020_dma_wait(void)
{
    int rc = 0;

    rc = wait_event_timeout(spi020_queue, trigger_flag == 1, 15 * HZ);
    if (rc == 0) {
    	printk(KERN_ERR "spi020 dma queue wake up timeout signal arrived\n");
    	return -1;
    }

    trigger_flag = 0;
    return 0;
}

#else

static void ftspi020_write_word(void __iomem * base, const void *data,
				int wsize)
{
	if (data) {
		switch (wsize) {
		case 1:
			outb(*(const u8 *)data, base + FTSPI020_REG_DATA_PORT);
			break;

		case 2:
			outw(*(const u16 *)data, base + FTSPI020_REG_DATA_PORT);
			break;

		default:
			outl(*(const u32 *)data, base + FTSPI020_REG_DATA_PORT);
			break;
		}
	}

}

static void ftspi020_read_word(void __iomem * base, void *buf, int wsize)
{
	if (buf) {
		switch (wsize) {
		case 1:
			*(u8 *) buf = inb(base + FTSPI020_REG_DATA_PORT);
			break;

		case 2:
			*(u16 *) buf = inw(base + FTSPI020_REG_DATA_PORT);
			break;

		default:
			*(u32 *) buf = inl(base + FTSPI020_REG_DATA_PORT);
			break;
		}
	}
}

static int ftspi020_txfifo_ready(void __iomem * base)
{
	return inl(base + FTSPI020_REG_STS) & FTSPI020_STS_TFR;
}

static int ftspi020_rxfifo_ready(void __iomem * base)
{
	return inl(base + FTSPI020_REG_STS) & FTSPI020_STS_RFR;
}

#endif

static unsigned int ftspi020_read_feature(void __iomem *base)
{
	return inl(base + FTSPI020_REG_FEATURE);
}

static int ftspi020_txfifo_depth(void __iomem * base)
{
	return  FTSPI020_FEATURE_TXFIFO_DEPTH(ftspi020_read_feature(base));
}

static int ftspi020_rxfifo_depth(void __iomem * base)
{
	return  FTSPI020_FEATURE_RXFIFO_DEPTH(ftspi020_read_feature(base));
}

static unsigned char ftspi020_read_status(void __iomem * base)
{
	return inb(base + FTSPI020_REG_READ_STS);
}

static void ftspi020_reset_hw(void __iomem * base)
{
	unsigned int	tmp = inl(base + FTSPI020_REG_CTRL);

	tmp |= FTSPI020_CTRL_ABORT;
	outl(tmp, base + FTSPI020_REG_CTRL);
}

static void ftspi020_int_enable(void __iomem * base)
{
	unsigned int	tmp = inl(base + FTSPI020_REG_ICR);

	tmp |= FTSPI020_ICR_INT;
	outl(tmp, base + FTSPI020_REG_ICR);
}

static void ftspi020_operate_mode(void __iomem * base, char mode)
{
	unsigned int	tmp = inl(base + FTSPI020_REG_CTRL);

	tmp &= FTSPI020_CTRL_CLK_MODE_MASK;
	tmp |= mode;
	outl(tmp, base + FTSPI020_REG_CTRL);
}

static void ftspi020_busy_location(void __iomem * base, char loc)
{
	unsigned int	tmp = inl(base + FTSPI020_REG_CTRL);

	tmp &= FTSPI020_CTRL_READY_LOC_MASK;
	tmp |= FTSPI020_CTRL_READY_LOC(loc);
	outl(tmp, base + FTSPI020_REG_CTRL);
}

static void ftspi020_clk_divider(void __iomem * base, char div)
{
	unsigned int	tmp = inl(base + FTSPI020_REG_CTRL);

	tmp &= FTSPI020_CTRL_CLK_DIVIDER_MASK;
	tmp |= div;
	outl(tmp, base + FTSPI020_REG_CTRL);
}

/******************************************************************************
 * workqueue
 *****************************************************************************/
#if defined(CONFIG_FTSPI020_USE_AHBDMA) || defined(CONFIG_FTSPI020_USE_AXIDMA)

static int setup_dma(struct ftspi020_ctrl *ctrl, int direct)
{
    struct dma_slave_config *common;

    ctrl->dma_slave_config.id = -1;
    ctrl->dma_slave_config.handshake = SPI020_REQ;//enable

    common = &ctrl->dma_slave_config.common;

#ifdef CONFIG_FTSPI020_USE_AHBDMA
    ctrl->dma_slave_config.src_size = FTDMAC020_BURST_SZ_4;//64x4=256

    if(direct == DMA_DEV_TO_MEM){
        ctrl->dma_slave_config.src_sel = AHBMASTER_R_SRC;
        ctrl->dma_slave_config.dst_sel = AHBMASTER_R_DST;
    }else{
        ctrl->dma_slave_config.src_sel = AHBMASTER_W_SRC;
        ctrl->dma_slave_config.dst_sel = AHBMASTER_W_DST;
    }
#else
    common->dst_maxburst = 4;
    common->src_maxburst = 4;
#endif

    if(direct == DMA_MEM_TO_DEV){
     	common->src_addr = ctrl->mem_dmaaddr;
    	common->dst_addr = ctrl->spi_dmaaddr;
    }else{
     	common->src_addr = ctrl->spi_dmaaddr;
    	common->dst_addr = ctrl->mem_dmaaddr;
    }

    /* SPI kernel maybe send len = 2011, so can't div 4 */
    common->dst_addr_width = 1;
    common->src_addr_width = 1;

    common->direction = direct;

    return dmaengine_slave_config(ctrl->dma_chan, common);//step 2
}

static int spi020_dma_start(struct ftspi020_ctrl *ctrl, size_t len, int direct)
{
	int ret;
	enum dma_ctrl_flags flags;
	struct dma_async_tx_descriptor *desc;

	ret = setup_dma(ctrl, direct);
	if (ret)
	    return ret;

	flags = DMA_PREP_INTERRUPT | DMA_CTRL_ACK | DMA_COMPL_SKIP_SRC_UNMAP | DMA_COMPL_SKIP_DEST_UNMAP;

	desc = dmaengine_prep_slave_single(ctrl->dma_chan, (void *)ctrl->sg_dmabuf, len, direct, flags);//step 3

	if (!desc)
	    return ret;

	desc->callback = ftspi020_dma_callback;
        desc->callback_param = &ctrl;
        ctrl->cookie = dmaengine_submit(desc);	//step 4
	dma_async_issue_pending(ctrl->dma_chan);//step 5

	return 0;
}
#endif

static int _ftspi020_ctrl_work_transfer(struct ftspi020_ctrl *ctrl,
				       struct spi_device *spi,
				       struct spi_transfer *t, int wsize)
{
	int		len;
	const void	*tx_buf;
	void		*rx_buf;
	struct ftspi020_cmd *cmd;
	int		ret = 0;

	cmd = (struct ftspi020_cmd *) t->tx_buf;

	len = cmd->data_cnt;

	/* Send command: Transfer OP CODE and address(if available)  */
	if (cmd->flags & FTSPI020_XFER_CMD_STATE) {
		int cmdq1, cmdq3;

		g_cmd_complete = 0;

		outl(cmd->spi_addr, ctrl->base + FTSPI020_REG_CMD0);

		cmdq1 = (cmd->conti_read_mode_en | cmd->ins_len | cmd->dum_2nd_cyc | cmd->addr_len);
		outl(cmdq1, ctrl->base + FTSPI020_REG_CMD1);

		outl(cmd->data_cnt, ctrl->base + FTSPI020_REG_CMD2);

		cmdq3 = (cmd->ins_code | cmd->conti_read_mode_code | FTSPI020_CMD3_CE(spi->chip_select) | cmd->spi_mode |
			cmd->dtr_mode | cmd->read_status | cmd->read_status_en | cmd->write_en |
			FTSPI020_CMD3_CMD_COMPL_INTR);
		outl(cmdq3, ctrl->base + FTSPI020_REG_CMD3);

	}

	if (cmd->flags & FTSPI020_XFER_DATA_STATE) {

		tx_buf = cmd->tx_buf;
		rx_buf = cmd->rx_buf;

		/* Special handling for Read Status */
		if ((cmd->read_status_en == FTSPI020_CMD3_RD_STS_EN) &&
		    (cmd->read_status == FTSPI020_CMD3_STS_SW_READ)) {

			*(u8 *) rx_buf = ftspi020_read_status(ctrl->base);
			len = 0;
		}

		while (len > 0) {
			int access_byte;

			if (tx_buf) {
#if defined(CONFIG_FTSPI020_USE_AHBDMA) || defined(CONFIG_FTSPI020_USE_AXIDMA)

				access_byte = min_t(int, len, FTSPI020_DMA_BUF_SIZE);

				memcpy(ctrl->mem_dmabuf, tx_buf, access_byte);

                ret = spi020_dma_start(ctrl, access_byte, DMA_MEM_TO_DEV);
                if (ret < 0) {
                    printk(KERN_ERR "spi020 dma write fail\n");
                    goto out;
                }
                ftspi020_dma_wait();

				tx_buf += access_byte;
				len -= access_byte;
#else
				access_byte = min_t(int, len, ctrl->txfifo_depth);
				len -= access_byte;

				while (!ftspi020_txfifo_ready(ctrl->base));

				while (access_byte) {
					ftspi020_write_word(ctrl->base, tx_buf, wsize);

					tx_buf += wsize;
					access_byte -= wsize;
				}
#endif
			} else if (rx_buf) {
#if defined(CONFIG_FTSPI020_USE_AHBDMA) || defined(CONFIG_FTSPI020_USE_AXIDMA)

				access_byte = min_t(int, len, FTSPI020_DMA_BUF_SIZE);

                ret = spi020_dma_start(ctrl, access_byte, DMA_DEV_TO_MEM);
                if (ret < 0) {
                    printk(KERN_ERR "spi020 dma read fail\n");
                    goto out;
                }

                ftspi020_dma_wait();

				memcpy(rx_buf, ctrl->mem_dmabuf, access_byte);
				rx_buf += access_byte;
				len -= access_byte;
#else
				while (!ftspi020_rxfifo_ready(ctrl->base));

				access_byte = min_t(int, len, ctrl->rxfifo_depth);
				len -= access_byte;

				while (access_byte) {
					ftspi020_read_word(ctrl->base, rx_buf, wsize);

					rx_buf += wsize;
					access_byte -= wsize;
				}
#endif
			}
		}

		ret = cmd->data_cnt - len;
	}

	/* Wait for Command complete interrupt */
	if (cmd->flags & FTSPI020_XFER_CHECK_CMD_COMPLETE) {
		/* wait until command complete interrupt */
		len = wait_event_timeout(ctrl->waitq, g_cmd_complete, 15 * HZ);
		if (len == 0) {
    	    printk(KERN_ERR "spi020 complete queue wake up timeout signal arrived\n");
    	    return -1;
        }
	}
#if defined(CONFIG_FTSPI020_USE_AHBDMA) || defined(CONFIG_FTSPI020_USE_AXIDMA)
out:
#endif
	return ret;
}

static int ftspi020_ctrl_work_transfer(struct ftspi020_ctrl *ctrl,
				      struct spi_device *spi,
				      struct spi_transfer *t)
{
	unsigned int	bpw;
	unsigned int	wsize;	/* word size */
	int		ret;

	dev_dbg(&spi->dev, "%s(%p)\n", __func__, t);

	bpw = t->bits_per_word ? t->bits_per_word : spi->bits_per_word;

	if (bpw == 0 || bpw > 32)
		return -EINVAL;

	if (bpw <= 8)
		wsize = 1;
	else if (bpw <= 16)
		wsize = 2;
	else
		wsize = 4;

	/* XXX we should check t->speed_hz */
	ret = _ftspi020_ctrl_work_transfer(ctrl, spi, t, wsize);

	if (t->delay_usecs)
		udelay(t->delay_usecs);

	return ret;
}

static void ftspi020_ctrl_work_message(struct ftspi020_ctrl *ctrl,
				      struct spi_message *m)
{
	struct spi_device	*spi = m->spi;
	struct spi_transfer	*t;

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,24))
	dev_dbg(&ctrl->master->dev, "%s(%p)\n", __func__, m);
#else
	dev_dbg(&ctrl->master->cdev.dev, "%s(%p)\n", __func__, m);
#endif

	m->status		= 0;
	m->actual_length	= 0;

	list_for_each_entry(t, &m->transfers, transfer_list) {
		int	ret;

		if ((ret =
		     ftspi020_ctrl_work_transfer(ctrl, spi, t)) < 0) {
			m->status = ret;
			break;
		}

		m->actual_length += ret;
	}

	m->complete(m->context);
}

static void ftspi020_ctrl_work(struct work_struct *work)
{
	struct ftspi020_ctrl	*ctrl;
	unsigned long		flags;

	ctrl = container_of(work, struct ftspi020_ctrl, work);

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,24))
	dev_dbg(&ctrl->master->dev, "%s(%p)\n", __func__, work);
#else
	dev_dbg(ctrl->master->cdev.dev, "%s(%p)\n", __func__, work);
#endif

	spin_lock_irqsave(&ctrl->lock, flags);

	while (!list_empty(&ctrl->message_queue)) {
		struct spi_message	*m;

		m = container_of(ctrl->message_queue.next,
				 struct spi_message, queue);
		list_del_init(&m->queue);

		spin_unlock_irqrestore(&ctrl->lock, flags);
		ftspi020_ctrl_work_message(ctrl, m);
		spin_lock_irqsave(&ctrl->lock, flags);
	}

	spin_unlock_irqrestore(&ctrl->lock, flags);
}

/******************************************************************************
 * interrupt handler
 *****************************************************************************/
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,19))
static irqreturn_t ftspi020_ctrl_interrupt(int irq, void *dev_id)
#else
static irqreturn_t ftspi020_ctrl_interrupt(int irq, void *dev_id,
					  struct pt_regs *regs)
#endif
{
	struct spi_master	*master = dev_id;
	struct ftspi020_ctrl	*ctrl;
	u32			isr;

	ctrl = spi_master_get_devdata(master);

	isr = inl(ctrl->base + FTSPI020_REG_ISR);
	outl(isr, ctrl->base + FTSPI020_REG_ISR);

	if (isr & FTSPI020_ISR_CMD_CMPL) {
		g_cmd_complete = 1;

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,24))
		dev_dbg(&master->dev, "Command Complete !\n");
#else
		dev_dbg(master->cdev.dev, "Command Complete!\n");
#endif
	}
	wake_up(&ctrl->waitq);
	return IRQ_HANDLED;
}


/******************************************************************************
 * struct spi_master functions
 *****************************************************************************/

static int ftspi020_ctrl_master_setup_mode(struct ftspi020_ctrl *ctrl,
					  u8 mode)
{
	u8 val;

	if (mode ==  SPI_MODE_0)
		val = FTSPI020_CTRL_CLK_MODE_0;
	else if (mode == SPI_MODE_3)
		val = FTSPI020_CTRL_CLK_MODE_3;
	else {
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,24))
		dev_err(&ctrl->master->dev,
			 "setup: unsupported mode bits %x\n", mode);
#else
		dev_err(ctrl->master->cdev.dev,
			 "setup: unsupported mode bits %x\n", mode);
#endif
		return -EINVAL;
	}

	ftspi020_operate_mode(ctrl->base, val);

	return 0;
}

static int ftspi020_ctrl_master_setup(struct spi_device *spi)
{
	struct ftspi020_ctrl	*ctrl;
	unsigned int		bpw = spi->bits_per_word;
	int			CLK, CLK_DIV, ret, mod;

	ctrl = spi_master_get_devdata(spi->master);

	if (spi->chip_select > spi->master->num_chipselect) {
		dev_info(&spi->dev,
			 "setup: invalid chipselect %u (%u defined)\n",
			 spi->chip_select, spi->master->num_chipselect);
		return -EINVAL;
	}

	/* check bits per word */

	bpw = bpw ? bpw : 32;

	if (bpw == 0 || bpw > 32) {
		dev_info(&spi->dev, "setup: invalid bpw%u (1 to 32)\n", bpw);
		return -EINVAL;
	}

	spi->bits_per_word = bpw;

	/* check mode */

	if ((ret = ftspi020_ctrl_master_setup_mode(ctrl, spi->mode)) < 0)
		return ret;

	dev_info(&spi->dev, "setup: bpw %u mode %d\n", bpw, spi->mode);

	/* check speed */
	if (!spi->max_speed_hz) {
		dev_err(&spi->dev, "setup: max speed not specified\n");
		return -EINVAL;
	}

	/* XXX we should calculate this from actual clock and max_speed_hz */
#if defined(CONFIG_PLATFORM_GM8139) || defined(CONFIG_PLATFORM_GM8136)
#ifdef CONFIG_PLATFORM_GM8139
    if(ftpmu010_read_reg(0x28) & (1 << 13))
        CLK = ftpmu010_get_attr(ATTR_TYPE_PLL2) / 4;
    else
        CLK = ftpmu010_get_attr(ATTR_TYPE_PLL1) / 2;
#endif
#ifdef CONFIG_PLATFORM_GM8136
    if(ftpmu010_read_reg(0x28) & (1 << 10))
        CLK = ftpmu010_get_attr(ATTR_TYPE_PLL1);
    else
        CLK = ftpmu010_get_attr(ATTR_TYPE_PLL2);
#endif
    CLK /= ((ftpmu010_read_reg(0x6C) & 0x7) + 1);

#else 	
	CLK = ftpmu010_get_attr(ATTR_TYPE_AHB);
#endif	
	//printk("CLK = %d,%d\n",CLK,spi->max_speed_hz);

    CLK_DIV = CLK;
    mod = do_div(CLK_DIV, spi->max_speed_hz);
    
    if(mod)
        CLK_DIV++;
    
    //printk("CLK_DIV = %d, speed_hz = %d, mod = %d\n",CLK_DIV, spi->max_speed_hz, mod);
    if(CLK_DIV > 8){
        printk(KERN_ERR "SPI clock too quick\n");
        CLK_DIV = FTSPI020_CTRL_CLK_DIVIDER_8;
    }
    else if(CLK_DIV > 6) {
        CLK_DIV = FTSPI020_CTRL_CLK_DIVIDER_8;
        CLK /= 8;
    } else if(CLK_DIV > 4) {
        CLK_DIV = FTSPI020_CTRL_CLK_DIVIDER_6;
        CLK /= 6;
    } else if(CLK_DIV > 2) {
        CLK_DIV = FTSPI020_CTRL_CLK_DIVIDER_4;
        CLK /= 4;
    } else {
        CLK_DIV = FTSPI020_CTRL_CLK_DIVIDER_2;
        CLK /= 2;
    }

    printk(KERN_NOTICE "CLK div field set %d, clock = %dHz\n", CLK_DIV, CLK);
	ftspi020_clk_divider(ctrl->base, CLK_DIV);

	/* Maybe require to clear FIFO */
#if 0
	ftspi020_reset_hw(ctrl->base);
#endif

	return 0;
}

static int ftspi020_ctrl_master_send_message(struct spi_device *spi,
					    struct spi_message *m)
{
	struct ftspi020_ctrl	*ctrl;
	struct spi_transfer	*t;
	unsigned long		flags;

	ctrl = spi_master_get_devdata(spi->master);

	/* sanity check */
	list_for_each_entry(t, &m->transfers, transfer_list) {
		dev_dbg(&spi->dev,
		        "  xfer %p: len %u tx %p/%08x rx %p/%08x\n",
			t, t->len, t->tx_buf, t->tx_dma, t->rx_buf, t->rx_dma);

		if (!t->tx_buf) {
			dev_err(&spi->dev, "missing important tx buf\n");
			return -EINVAL;
		}
		if (!t->rx_buf && t->len) {
			dev_err(&spi->dev, "missing rx tx buf\n");
			return -EINVAL;
		}
	}

	m->status		= -EINPROGRESS;
	m->actual_length	= 0;

	/* transfer */

	spin_lock_irqsave(&ctrl->lock, flags);

	list_add_tail(&m->queue, &ctrl->message_queue);
	queue_work(ctrl->workqueue, &ctrl->work);

	spin_unlock_irqrestore(&ctrl->lock, flags);

	return 0;

}

static int ftspi020_ctrl_master_transfer(struct spi_device *spi,
					struct spi_message *m)
{
	dev_dbg(&spi->dev, "new message %p submitted for %s\n",
		m, dev_name(&spi->dev));

	if (unlikely(list_empty(&m->transfers))) {
		dev_err(&spi->dev, "empty spi_message\n");
		return -EINVAL;
	}

	return ftspi020_ctrl_master_send_message(spi, m);
}

static void ftspi020_ctrl_master_cleanup(struct spi_device *spi)
{
	dev_info(&spi->dev, "cleanup\n");
	if (!spi->controller_state)
		return;
}

/******************************************************************************
 * struct platform_driver functions
 *****************************************************************************/
static int ftspi020_probe(struct platform_device *pdev)
{
	struct resource		*res;
	int			irq;
	void __iomem		*base;
	int			ret = 0;
	struct spi_master	*master;
	struct ftspi020_ctrl	*ctrl;

	if ((res = platform_get_resource(pdev, IORESOURCE_MEM, 0)) == 0) {
		return -ENXIO;
	}

	if ((irq = platform_get_irq(pdev, 0)) < 0) {
		return irq;
	}
    
	/* setup spi core */

	if ((master =
	     spi_alloc_master(&pdev->dev, sizeof *ctrl)) == NULL) {
		ret = -ENOMEM;
		goto err_dealloc;
	}

        //master->mode_bits = 0;
	master->bus_num		= pdev->id;
	master->num_chipselect	= 4;
	master->setup		= ftspi020_ctrl_master_setup;
	master->transfer	= ftspi020_ctrl_master_transfer;
	master->cleanup		= ftspi020_ctrl_master_cleanup;
	platform_set_drvdata(pdev, master);

	/* setup master private data */

	ctrl = spi_master_get_devdata(master);

	spin_lock_init(&ctrl->lock);
	INIT_LIST_HEAD(&ctrl->message_queue);
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,20))
	INIT_WORK(&ctrl->work, ftspi020_ctrl_work);
#else
	INIT_WORK(&ctrl->work, ftspi020_ctrl_work, &ctrl->work);
#endif
	init_waitqueue_head(&ctrl->waitq);
#if defined(CONFIG_FTSPI020_USE_AHBDMA) || defined(CONFIG_FTSPI020_USE_AXIDMA)
    ctrl->spi_dmaaddr = res->start + 0x100; //data register
#endif
	if ((base =
	     ioremap_nocache(pdev->resource[0].start,pdev->resource[0].end - pdev->resource[0].start + 1)) == NULL) {
		ret = -ENOMEM;
		goto err_dealloc;
	}

	if ((ret =
	     request_irq(irq, ftspi020_ctrl_interrupt, 0, dev_name(&pdev->dev),
			 master))) {
		goto err_unmap;
	}

	ctrl->irq		= irq;
	ctrl->base		= base;
	ctrl->master		= master;
	ctrl->rxfifo_depth	= ftspi020_rxfifo_depth(base);
	ctrl->txfifo_depth	= ftspi020_txfifo_depth(base);

	if ((ctrl->workqueue =
	     create_singlethread_workqueue(dev_name(&pdev->dev))) == NULL) {
		goto err_free_irq;
	}

	/* Initialize the hardware */

	ftspi020_reset_hw(base);
    ftspi020_int_enable(base);
	ftspi020_busy_location(base, 0);

	/* Initialize DMA engine */

#if defined(CONFIG_FTSPI020_USE_AHBDMA) || defined(CONFIG_FTSPI020_USE_AXIDMA)
#ifdef CONFIG_FTSPI020_USE_AHBDMA
    printk(KERN_INFO "SPI020 uses AHB DMA mode\n");
#else
    ftpmu010_write_reg(spi020_fd, 0xA4, (0x0 << 26), (0x1 << 26));
    printk(KERN_INFO "SPI020 uses AXI DMA mode\n");
#endif

	ftspi020_dma_enable(base);

    dma_cap_set(DMA_SLAVE, ctrl->cap_mask);
#ifdef CONFIG_FTSPI020_USE_AHBDMA
    {
    	struct ftdmac020_dma_slave slave;
    	memset(&slave, 0, sizeof(slave));
    	ctrl->dma_chan = dma_request_channel(ctrl->cap_mask, ftdmac020_chan_filter, (void *)&slave);//step 1
    }
#else
    {
    	struct ftdmac030_dma_slave slave;
   		memset(&slave, 0, sizeof(slave));
   		ctrl->dma_chan = dma_request_channel(ctrl->cap_mask, ftdmac030_chan_filter, (void *)&slave);//step 1
    }
#endif
    if (!ctrl->dma_chan){
        dev_err(&pdev->dev, "SPI020 DMA channel allocation failed\n");
        ret = -ENODEV;
        goto out_free_chan;
    }
    printk(KERN_INFO "SPI020 gets DMA channel %d\n", ctrl->dma_chan->chan_id);

    ctrl->mem_dmabuf = dma_alloc_coherent(&pdev->dev, FTSPI020_DMA_BUF_SIZE, &ctrl->mem_dmaaddr, GFP_KERNEL);

    if (!ctrl->mem_dmabuf) {
        ret = -ENOMEM;
        goto out_free_dma;
    }
    ctrl->sg_dmabuf = dma_to_virt(&pdev->dev, ctrl->mem_dmaaddr);

    //printk("sg mem pa = 0x%x, va = 0x%x\n", (u32)ctrl->mem_dmaaddr, (u32)ctrl->sg_dmabuf);
#endif

	/* go! */

	dev_info(&pdev->dev, "Faraday FTSPI020 Controller at 0x%08x(0x%08x) irq %d.\n",
		 (unsigned int)res->start, (unsigned int)ctrl->base, irq);

	if ((ret = spi_register_master(master)) != 0) {
		dev_err(&pdev->dev, "register master failed\n");
		goto err_destroy_workqueue;
	}

	printk(KERN_INFO "Probe FTSPI020 SPI Controller at 0x%08x (irq %d)\n", (unsigned int)res->start, irq);
	return 0;

err_destroy_workqueue:
#if defined(CONFIG_FTSPI020_USE_AHBDMA) || defined(CONFIG_FTSPI020_USE_AXIDMA)
    if(ctrl->mem_dmabuf)
        dma_free_coherent(&pdev->dev, FTSPI020_DMA_BUF_SIZE, ctrl->mem_dmabuf, ctrl->mem_dmaaddr);
  out_free_dma:
    if(ctrl->dma_chan)
    	dma_release_channel(ctrl->dma_chan);
  out_free_chan:
#endif
	destroy_workqueue(ctrl->workqueue);
err_free_irq:
	free_irq(irq, master);

err_unmap:
	iounmap(base);
err_dealloc:
	spi_master_put(master);
	return ret;
}

static int ftspi020_remove(struct platform_device *pdev)
{
	struct spi_master	*master;
	struct ftspi020_ctrl	*ctrl;
	struct spi_message	*m;


	master		= platform_get_drvdata(pdev);
	ctrl	= spi_master_get_devdata(master);

	/* Terminate remaining queued transfers */
	list_for_each_entry(m, &ctrl->message_queue, queue) {
		m->status = -ESHUTDOWN;
		m->complete(m->context);
	}

	destroy_workqueue(ctrl->workqueue);
	ftpmu010_deregister_reg(spi020_fd);

	free_irq(ctrl->irq, master);
	iounmap(ctrl->base);

	spi_unregister_master(master);
	dev_info(&pdev->dev, "FTSPI020 unregistered\n");
#if defined(CONFIG_FTSPI020_USE_AHBDMA) || defined(CONFIG_FTSPI020_USE_AXIDMA)
    if(ctrl->mem_dmabuf)
        dma_free_coherent(&pdev->dev, FTSPI020_DMA_BUF_SIZE, ctrl->mem_dmabuf, ctrl->mem_dmaaddr);
    if(ctrl->dma_chan)
    	dma_release_channel(ctrl->dma_chan);
#endif
	return 0;
}

#define ftspi020_suspend NULL
#define ftspi020_resume NULL

static struct platform_driver ftspi020_driver = {
	.probe		= ftspi020_probe,
	.remove		= ftspi020_remove,
	.suspend	= ftspi020_suspend,
	.resume		= ftspi020_resume,
	.driver		= {
		.name = "ftspi020",
		.owner = THIS_MODULE,
	},
};

/******************************************************************************
 * initialization / finalization
 *****************************************************************************/
static int __init ftspi020_init(void)
{
	int	ret = 0;

#ifdef CONFIG_PLATFORM_GM8210
	fmem_pci_id_t pci_id;
	fmem_cpu_id_t cpu_id;
	fmem_get_identifier(&pci_id, &cpu_id);
	if((cpu_id != FMEM_CPU_FA726) || (pci_id != FMEM_PCI_HOST))
		return 0;
#endif

	printk(KERN_INFO "SPI020 init\n");

    /* check if the system is running SPI system
     */
    if (platform_check_flash_type() >= 0) {
        printk(KERN_INFO "Not for SPI NOR pin mux\n");
        return 0;
    }

#if defined(CONFIG_FTSPI020_USE_AHBDMA) || defined(CONFIG_FTSPI020_USE_AXIDMA)
    init_waitqueue_head(&spi020_queue);
#endif

   /* Register PMU and turn on gate clock
    */
   spi020_fd = ftpmu010_register_reg(&pmu_reg_info);
   if (spi020_fd < 0)
       printk(KERN_ERR "SPI020: register PMU fail");

	ret = platform_driver_register(&ftspi020_driver);

	if (ret)
		printk(KERN_ERR "register platform driver failed(%d)\n", ret);

	return ret;
}

static void __exit ftspi020_exit(void)
{
	printk(KERN_DEBUG "%s()\n", __func__);

    /* check if the system is running SPI system
     */
    if (platform_check_flash_type() >= 0)
        return;

	platform_driver_unregister(&ftspi020_driver);
}

#if defined(CONFIG_FTSPI020_USE_AHBDMA) || defined(CONFIG_FTSPI020_USE_AXIDMA)

void ftspi020_dma_callback(void *param)
{
    //printk("%s\n", __func__);

    trigger_flag = 1;
    wake_up(&spi020_queue);

    return;
}
#endif

module_init(ftspi020_init);
module_exit(ftspi020_exit);

MODULE_AUTHOR("BingJiun Luo <bjluo@faraday-tech.com>");
MODULE_DESCRIPTION("FTSPI020 SPI Flash Controller Driver");
MODULE_LICENSE("GPL");
