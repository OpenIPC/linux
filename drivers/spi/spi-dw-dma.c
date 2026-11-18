/**********************************************************************************
 *
 * Copyright (c) 2019-2020 Beijing AXera Technology Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Beijing AXera Technology Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Beijing AXera Technology Co., Ltd.
 *
 **********************************************************************************/

#include <linux/dma-mapping.h>
#include <linux/dmaengine.h>
#include <linux/interrupt.h>
#include <linux/slab.h>
#include <linux/spi/spi.h>
#include <linux/types.h>
#include <linux/of_device.h>
#include "spi-dw.h"
#include <asm/cacheflush.h>
#ifdef CONFIG_AXERA_DMA_PER
#include <linux/soc/axera/ax_boardinfo.h>
#include "../dma/axera-dma-per/axera-dma-per.h"
#endif

#define RX_BUSY		0
#define TX_BUSY		1

static ulong spi_dma_tx_start_cnt = 0;
static ulong spi_dma_rx_start_cnt = 0;
static ulong spi_dma_tx_done_cnt = 0;
static ulong spi_dma_rx_done_cnt = 0;

#ifdef CONFIG_AXERA_DMA_PER
#define AX_SPI_DMA_REG_DUMP
#ifdef AX_SPI_DMA_REG_DUMP
static int tx_submit = 0;
static int rx_submit = 0;
extern u32 ax_get_chip_type(void);
static int ax_dma_per_lli_addr_invalid(phys_addr_t lli_paddr)
{
	phys_addr_t ddr_start = 0x40000000;
	phys_addr_t ddr_end;

	if ((AX630C_CHIP != ax_get_chip_type()) && (AX631_CHIP != ax_get_chip_type())) {
		ddr_end = 0x4FFFFFFF;
		#ifdef CONFIG_PHYS_ADDR_T_64BIT
		printk("620Q mem whole space [0x%llX:0x%llX]\n", ddr_start, ddr_end);
		#else
		printk("620Q mem whole space [0x%X:0x%X]\n", ddr_start, ddr_end);
		#endif
	}
	else {
		ddr_end = 0xFFFFFFFF;
		#ifdef CONFIG_PHYS_ADDR_T_64BIT
		printk("630C mem whole space [0x%llX:0x%llX]\n", ddr_start, ddr_end);
		#else
		printk("630C mem whole space [0x%X:0x%X]\n", ddr_start, ddr_end);
		#endif
	}

	if ((lli_paddr >= ddr_start) && (lli_paddr < ddr_end))
		return 0;
	else
		return 1;
}

static void ax_dma_per_reg_dump(struct dma_chan * dchan, struct dw_spi *dws)
{
	struct axi_dma_chan *chan = dchan_to_axi_dma_chan(dchan);
	u64 lli_paddr = 0;
	void __iomem * lli_vaddr = NULL;
	int row;

	dev_err(&dws->master->dev, "\r\n==================== DMA REG DUMP ====================\n");
	axi_chan_status_print();
	dev_err(&dws->master->dev, "spi tx start %ld times, completed %ld times\n", spi_dma_tx_start_cnt, spi_dma_tx_done_cnt);
	dev_err(&dws->master->dev, "spi rx start %ld times, completed %ld times\n", spi_dma_rx_start_cnt, spi_dma_rx_done_cnt);
	for (row = 0; row < 0x30; row++) {
		#ifdef CONFIG_PHYS_ADDR_T_64BIT
		dev_err(&dws->master->dev, "%08llX ++ %04X: %08X %08X %08X %08X\n", chan->chip->paddr, row * 0x10,
			__raw_readl(chan->chip->regs + row * 0x10), __raw_readl(chan->chip->regs + row * 0x10 + 0x4),
			__raw_readl(chan->chip->regs + row * 0x10 + 0x8), __raw_readl(chan->chip->regs + row * 0x10 + 0xC));
		#else
		dev_err(&dws->master->dev, "%08X ++ %04X: %08X %08X %08X %08X\n", chan->chip->paddr, row * 0x10,
			__raw_readl(chan->chip->regs + row * 0x10), __raw_readl(chan->chip->regs + row * 0x10 + 0x4),
			__raw_readl(chan->chip->regs + row * 0x10 + 0x8), __raw_readl(chan->chip->regs + row * 0x10 + 0xC));
		#endif
	}
	for (row = 0x4; row < 0x6; row++) {
		#ifdef CONFIG_PHYS_ADDR_T_64BIT
		dev_err(&dws->master->dev, "%08llX ++ %04X: %08X %08X %08X %08X\n", chan->chip->req_paddr, row * 0x10,
			__raw_readl(chan->chip->req_regs + row * 0x10), __raw_readl(chan->chip->req_regs + row * 0x10 + 0x4),
			__raw_readl(chan->chip->req_regs + row * 0x10 + 0x8), __raw_readl(chan->chip->req_regs + row * 0x10 + 0xC));
		#else
		dev_err(&dws->master->dev, "%08X ++ %04X: %08X %08X %08X %08X\n", chan->chip->req_paddr, row * 0x10,
			__raw_readl(chan->chip->req_regs + row * 0x10), __raw_readl(chan->chip->req_regs + row * 0x10 + 0x4),
			__raw_readl(chan->chip->req_regs + row * 0x10 + 0x8), __raw_readl(chan->chip->req_regs + row * 0x10 + 0xC));
		#endif
	}

	lli_paddr = (u64)__raw_readl(chan->chip->regs + 0x20 + chan->id * 8);
	lli_paddr = (lli_paddr << 32) | __raw_readl(chan->chip->regs + 0x24 + chan->id * 8);
	if (ax_dma_per_lli_addr_invalid(lli_paddr)) {
		dev_err(&dws->master->dev, "%s: LLI PHY 0x%llX invalid\n", __FUNCTION__, lli_paddr);
		return;
	}
	lli_vaddr = (void __iomem *)phys_to_virt(lli_paddr);
	dev_err(&dws->master->dev, "== DMA CHAN %d LLI PHY %llX, VIR %lX ==\n", chan->id, lli_paddr, (long)lli_vaddr);
	if (NULL == lli_vaddr)
		return;

	for (row = 0; row < 2; row++) {
		dev_err(&dws->master->dev, "%08llX ++ %04X: %08X %08X %08X %08X\n", lli_paddr, row * 0x10,
			__raw_readl(lli_vaddr + row * 0x10), __raw_readl(lli_vaddr + row * 0x10 + 0x4),
			__raw_readl(lli_vaddr + row * 0x10 + 0x8), __raw_readl(lli_vaddr + row * 0x10 + 0xC));
	}
}

static void dw_spi_reg_dump(struct dw_spi *dws)
{
	int row;

	dev_err(&dws->master->dev, "==================== SPI REG DUMP ====================\n");
	for (row = 0; row < 0x10; row++) {
		if (6 == row)
			row = 0xf;
		dev_err(&dws->master->dev, "%08lX ++ %04X: %08X %08X %08X %08X\n", dws->paddr, row * 0x10,
			__raw_readl(dws->regs + row * 0x10), __raw_readl(dws->regs + row * 0x10 + 0x4),
			__raw_readl(dws->regs + row * 0x10 + 0x8), __raw_readl(dws->regs + row * 0x10 + 0xC));
	}
	dev_err(&dws->master->dev, "======================================================\n");
}
#endif
#endif

static int dw_spi_dma_init(struct dw_spi *dws)
{
	dma_cap_mask_t mask;
	dma_cap_zero(mask);
	dma_cap_set(DMA_SLAVE, mask);
	/* 1. Init rx channel */
	dws->rxchan = dma_request_slave_channel_compat(mask,
						       NULL, NULL,
						       &dws->master->dev, "rx");
	if (!dws->rxchan)
		goto err_exit;
	dws->master->dma_rx = dws->rxchan;

	/* 2. Init tx channel */
	dws->txchan = dma_request_slave_channel_compat(mask,
						       NULL, NULL,
						       &dws->master->dev, "tx");
	if (!dws->txchan)
		goto free_rxchan;
	dws->master->dma_tx = dws->txchan;

	of_dma_configure(&dws->master->dev, dws->master->dev.of_node, true);
	dws->dma_inited = 1;
	return 0;

free_rxchan:
	dma_release_channel(dws->rxchan);
err_exit:
	return -EBUSY;
}

static void dw_spi_dma_exit(struct dw_spi *dws)
{
	if (!dws->dma_inited)
		return;
	if (dws->txchan) {
		dmaengine_terminate_sync(dws->txchan);
		dma_release_channel(dws->txchan);
	}

	if (dws->rxchan) {
		dmaengine_terminate_sync(dws->rxchan);
		dma_release_channel(dws->rxchan);
	}
}

static irqreturn_t dma_transfer(struct dw_spi *dws)
{
	u16 irq_status = dw_readl(dws, DW_SPI_ISR);
	if (!irq_status)
		return IRQ_NONE;
#ifdef AX_SPI_DMA_REG_DUMP
	else {
		u32 rxflr, txflr;

		rxflr = dw_readl(dws, DW_SPI_RXFLR);
		txflr = dw_readl(dws, DW_SPI_TXFLR);
		dev_err(&dws->master->dev, "RXFIFO entries=0x%X, TXFIFO entries=0x%X\n", rxflr, txflr);
		if (rx_submit)
			ax_dma_per_reg_dump(dws->rxchan, dws);
		if (tx_submit)
			ax_dma_per_reg_dump(dws->txchan, dws);
		dw_spi_reg_dump(dws);
	}
#endif
	dw_readl(dws, DW_SPI_ICR);
	spi_reset_chip(dws);

	dev_err(&dws->master->dev, "%s: FIFO overrun/underrun\n", __func__);
	dws->master->cur_msg->status = -EIO;
	spi_finalize_current_transfer(dws->master);
	return IRQ_HANDLED;
}

static bool dw_spi_can_dma(struct spi_controller *master,
		struct spi_device *spi, struct spi_transfer *xfer)
{
	struct dw_spi *dws = spi_controller_get_devdata(master);
	if (!dws->dma_inited)
		return false;
	return xfer->len > dws->fifo_len;
}

static enum dma_slave_buswidth convert_dma_width(u32 dma_width) {
	if (dma_width == 1)
		return DMA_SLAVE_BUSWIDTH_1_BYTE;
	else if (dma_width == 2)
		return DMA_SLAVE_BUSWIDTH_2_BYTES;
	else if (dma_width == 4)
		return DMA_SLAVE_BUSWIDTH_4_BYTES;

	return DMA_SLAVE_BUSWIDTH_UNDEFINED;
}

static int dw_spi_dma_wait(struct dw_spi *dws, unsigned int len, u32 speed)
{
	unsigned long long ms;

	ms = len * MSEC_PER_SEC * BITS_PER_BYTE;
	do_div(ms, speed);
	ms += ms + 200;

	if (ms > UINT_MAX)
		ms = UINT_MAX;
	ms = wait_for_completion_timeout(&dws->master->xfer_completion,
					 msecs_to_jiffies(ms));
	if (ms == 0) {
		dev_err(&dws->master->cur_msg->spi->dev,
			"DMA transaction timed out\n");
#ifdef AX_SPI_DMA_REG_DUMP
		if (rx_submit)
			ax_dma_per_reg_dump(dws->rxchan, dws);
		if (tx_submit)
			ax_dma_per_reg_dump(dws->txchan, dws);
		dw_spi_reg_dump(dws);
#endif
		return -ETIMEDOUT;
	}
	return 0;
}

/*
 * dws->dma_chan_busy is set before the dma transfer starts, callback for tx
 * channel will clear a corresponding bit.
 */
static void dw_spi_dma_tx_done(void *arg)
{
	struct dw_spi *dws = arg;
	spi_dma_tx_done_cnt++;
	clear_bit(TX_BUSY, &dws->dma_chan_busy);
	if (test_bit(RX_BUSY, &dws->dma_chan_busy))
		return;
	spi_finalize_current_transfer(dws->master);
}

static struct dma_async_tx_descriptor *dw_spi_dma_prepare_tx(struct dw_spi *dws,
		struct spi_transfer *xfer)
{
	struct dma_slave_config txconf;
	struct dma_async_tx_descriptor *txdesc;
	u8 dma_endian = 0;
	if (!xfer->tx_buf)
		return NULL;

	txconf.direction = DMA_MEM_TO_DEV;
	txconf.dst_addr = dws->dma_addr;
	txconf.dst_maxburst = 0x10;//16 data item
	txconf.src_addr_width = DMA_SLAVE_BUSWIDTH_4_BYTES;
	txconf.dst_addr_width = convert_dma_width(dws->dma_width);
	txconf.device_fc = false;
	switch (convert_dma_width(dws->dma_width)) {
	case DMA_SLAVE_BUSWIDTH_4_BYTES:
		dma_endian = 0x1;
		break;

	case DMA_SLAVE_BUSWIDTH_2_BYTES:
		/* dev_warn(&dws->master->cur_msg->spi->dev, "SPITX & AXDMA width is %d bytes mode\n", DMA_SLAVE_BUSWIDTH_2_BYTES);*/
		dma_endian = 0x2;
		break;

	case DMA_SLAVE_BUSWIDTH_1_BYTE:
	default:
		/* dev_warn(&dws->master->cur_msg->spi->dev, "SPITX & AXDMA width is %d byte mode\n", DMA_SLAVE_BUSWIDTH_1_BYTE);*/
		dma_endian = 0x0;
		break;
	}
	txconf.slave_id = (dma_endian << 30);

	dmaengine_slave_config(dws->txchan, &txconf);

	txdesc = dmaengine_prep_slave_sg(dws->txchan,
				xfer->tx_sg.sgl,
				xfer->tx_sg.nents,
				DMA_MEM_TO_DEV,
				DMA_PREP_INTERRUPT | DMA_CTRL_ACK);
	if (!txdesc)
		return NULL;

	txdesc->callback = dw_spi_dma_tx_done;
	txdesc->callback_param = dws;

	return txdesc;
}

/*
 * dws->dma_chan_busy is set before the dma transfer starts, callback for rx
 * channel will clear a corresponding bit.
 */
static void dw_spi_dma_rx_done(void *arg)
{
	struct dw_spi *dws = arg;
	spi_dma_rx_done_cnt++;
	clear_bit(RX_BUSY, &dws->dma_chan_busy);
	if (test_bit(TX_BUSY, &dws->dma_chan_busy))
		return;
	spi_finalize_current_transfer(dws->master);
}

static struct dma_async_tx_descriptor *dw_spi_dma_prepare_rx(struct dw_spi *dws,
		struct spi_transfer *xfer)
{
	struct dma_slave_config rxconf;
	struct dma_async_tx_descriptor *rxdesc;
	u8 dma_endian = 0;
	if (!xfer->rx_buf)
		return NULL;

	rxconf.direction = DMA_DEV_TO_MEM;
	rxconf.src_addr = dws->dma_addr;
	rxconf.src_maxburst = 0x10;//16 data item
	rxconf.dst_addr_width = DMA_SLAVE_BUSWIDTH_4_BYTES;
	rxconf.src_addr_width = convert_dma_width(dws->dma_width);
	rxconf.device_fc = false;
	switch (convert_dma_width(dws->dma_width)) {
	case DMA_SLAVE_BUSWIDTH_4_BYTES:
		dma_endian = 0x1;
		break;

	case DMA_SLAVE_BUSWIDTH_2_BYTES:
		/* dev_warn(&dws->master->cur_msg->spi->dev, "SPIRX & AXDMA width is %d bytes mode\n", DMA_SLAVE_BUSWIDTH_2_BYTES);*/
		dma_endian = 0x2;
		break;

	case DMA_SLAVE_BUSWIDTH_1_BYTE:
	default:
		/* dev_warn(&dws->master->cur_msg->spi->dev, "SPIRX & AXDMA width is %d byte mode\n", DMA_SLAVE_BUSWIDTH_1_BYTE);*/
		dma_endian = 0x0;
		break;
	}
	rxconf.slave_id = (dma_endian << 30);

	dmaengine_slave_config(dws->rxchan, &rxconf);

	rxdesc = dmaengine_prep_slave_sg(dws->rxchan,
				xfer->rx_sg.sgl,
				xfer->rx_sg.nents,
				DMA_DEV_TO_MEM,
				DMA_PREP_INTERRUPT | DMA_CTRL_ACK);
	if (!rxdesc)
		return NULL;

	rxdesc->callback = dw_spi_dma_rx_done;
	rxdesc->callback_param = dws;

	return rxdesc;
}

static int dw_spi_dma_setup(struct dw_spi *dws, struct spi_transfer *xfer)
{
	u16 imr, dma_ctrl;
	if (!xfer->tx_buf && !xfer->rx_buf)
		return -EINVAL;
	dw_writel(dws, DW_SPI_DMARDLR, 0xf);
	dw_writel(dws, DW_SPI_DMATDLR, 0x10);

	if (xfer->tx_buf)
		dma_ctrl |= SPI_DMA_TDMAE;
	if (xfer->rx_buf)
		dma_ctrl |= SPI_DMA_RDMAE;
	dw_writel(dws, DW_SPI_DMACR, dma_ctrl);

	/* Set the interrupt mask */
	imr = SPI_INT_TXOI;
	if (xfer->rx_buf)
		imr |= SPI_INT_RXUI | SPI_INT_RXOI;
	spi_umask_intr(dws, imr);

	reinit_completion(&dws->master->xfer_completion);
	dws->transfer_handler = dma_transfer;
	return 0;
}

static int dw_spi_dma_transfer(struct dw_spi *dws, struct spi_transfer *xfer)
{
	int ret, val;
	struct dma_async_tx_descriptor *txdesc, *rxdesc;

	/* Prepare the TX dma transfer */
	txdesc = dw_spi_dma_prepare_tx(dws, xfer);
	/* Prepare the RX dma transfer */
	rxdesc = dw_spi_dma_prepare_rx(dws, xfer);

	if (rxdesc) {
		dmaengine_submit(rxdesc);
		set_bit(RX_BUSY, &dws->dma_chan_busy);
#ifdef AX_SPI_DMA_REG_DUMP
		rx_submit = 1;
		spi_dma_rx_start_cnt++;
#endif
		/* rx must be started before tx due to spi instinct */
		dma_async_issue_pending(dws->rxchan);
		if (!xfer->tx_buf) {
			dw_writel(dws, DW_SPI_DR, 0xffffffff);
		}
	}
	if (txdesc) {
		dmaengine_submit(txdesc);
		set_bit(TX_BUSY, &dws->dma_chan_busy);
#ifdef AX_SPI_DMA_REG_DUMP
		tx_submit = 1;
		spi_dma_tx_start_cnt++;
#endif
		dma_async_issue_pending(dws->txchan);
	}
	ret = dw_spi_dma_wait(dws, xfer->len, dws->current_freq);
	if (ret)
		return ret;

	/* waiting spi fifo empt and idle. */
	if (txdesc) {
		if (readl_poll_timeout(dws->regs + DW_SPI_SR, val,
					(val & SR_TF_EMPT) && !(val & SR_BUSY), 0,
					1000 * 1000)) {
			printk("wait TX SR_TF_EMPT or SR_BUSY timeout\n");
		}
	}
	/* If xfer->rx_sg.sgl->length is not 64-byte aligned, the cacheline invalidate operation will flush
	 * this cacheline to the DDR, resulting in overwriting the valid data.
	 * Solution: Align xfer->rx_sg.sgl->length up the length of the cacheline (64B), then set the invalidate flag bit, and consider the cacheline to be invalid.
	 */
	if (rxdesc)
		dma_sync_single_for_cpu(&dws->master->dev, ALIGN_DOWN(xfer->rx_sg.sgl->dma_address + xfer->rx_sg.sgl->length, cache_line_size()), cache_line_size(), DMA_FROM_DEVICE);

	dw_writel(dws, DW_SPI_SSIENR, 0);
	dw_writel(dws, DW_SPI_DMACR, 0);
	dw_writel(dws, DW_SPI_SSIENR, 1);
#ifdef AX_SPI_DMA_REG_DUMP
	tx_submit = 0;
	rx_submit = 0;
#endif
	return 0;
}

static void dw_spi_dma_stop(struct dw_spi *dws)
{
	if (test_bit(TX_BUSY, &dws->dma_chan_busy)) {
		dmaengine_terminate_sync(dws->txchan);
		clear_bit(TX_BUSY, &dws->dma_chan_busy);
	}
	if (test_bit(RX_BUSY, &dws->dma_chan_busy)) {
		dmaengine_terminate_sync(dws->rxchan);
		clear_bit(RX_BUSY, &dws->dma_chan_busy);
	}
}

static const struct dw_spi_dma_ops spi_dma_ops = {
	.dma_init	= dw_spi_dma_init,
	.dma_exit	= dw_spi_dma_exit,
	.dma_setup	= dw_spi_dma_setup,
	.can_dma	= dw_spi_can_dma,
	.dma_transfer	= dw_spi_dma_transfer,
	.dma_stop	= dw_spi_dma_stop,
};

int dw_apb_spi_dma_register(struct dw_spi *dws)
{
	dws->dma_ops = &spi_dma_ops;
	return 0;
}
EXPORT_SYMBOL_GPL(dw_apb_spi_dma_register);
