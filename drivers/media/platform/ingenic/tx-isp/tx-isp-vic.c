// SPDX-License-Identifier: GPL-2.0
/*
 * Ingenic T23 Video Ingest Controller
 * Copyright (C) 2017-2023 Ingenic Semiconductor Co., Ltd.
 * Copyright (C) 2026 Difan Zhang
 */

#include <linux/iopoll.h>

#include "tx-isp-device.h"

struct tx_isp_vic_device {
	struct tx_isp_subdev sd;
	struct platform_device *pdev;
	struct mutex lock;
	unsigned int width;
	unsigned int height;
	unsigned int port;
	bool configured;
	bool running;
};

static struct tx_isp_vic_device *g_vic_dev;

int tx_isp_vic_configure(struct tx_isp_subdev *sd, unsigned int width,
			 unsigned int height, unsigned int port)
{
	struct tx_isp_vic_device *vic = sd ? sd->dev_priv : NULL;
	int ret = 0;

	if (!vic || !width || width > 1920 || !height || height > 1080 || port > 1)
		return -EINVAL;
	mutex_lock(&vic->lock);
	if (vic->running)
		ret = -EBUSY;
	else {
		vic->width = width;
		vic->height = height;
		vic->port = port;
		vic->configured = true;
	}
	mutex_unlock(&vic->lock);
	return ret;
}

int tx_isp_vic_start(struct tx_isp_subdev *sd)
{
	struct tx_isp_vic_device *vic = sd ? sd->dev_priv : NULL;
	u32 value;
	int ret = 0;

	if (!vic || !sd->base)
		return -EINVAL;
	mutex_lock(&vic->lock);
	if (!vic->configured) {
		ret = -EINVAL;
		goto unlock;
	}
	if (vic->running)
		goto unlock;

	/* The selected sensor supplies the input clock during reset. */
	writel(vic->port ? VIC_PORT_SEL_PORT1 : VIC_PORT_SEL_PORT0,
	       sd->base + VIC_PORT_SEL);
	writel(0, sd->base + VIC_AUTO_SWITCH_CTRL);
	writel(VIC_FRM_CYCLE_DEFAULT, sd->base + VIC_FRM_CYCLE);
	writel(DIV_ROUND_UP(vic->width * 10, 32), sd->base + VIC_LINE_STRIDE);
	writel(VIC_CONF_DEFAULT, sd->base + VIC_CONF_REG);
	writel(VIC_INPUT_DEFAULT, sd->base + VIC_INPUT_REG);
	writel((vic->width << 16) | vic->height, sd->base + VIC_IMAGE_SIZE);
	writel(VIC_CSI_CONF_DEFAULT, sd->base + VIC_CSI_CONF);
	writel(vic->width << 16, sd->base + VIC_CSI_SIZE);
	writel(0, sd->base + VIC_CSI_SIZE1);
	writel(0, sd->base + VIC_CSI_SIZE2);
	writel(0, sd->base + VIC_CSI_SIZE3);
	writel(VIC_FIFO_TH_1_DEFAULT, sd->base + VIC_FIFO_TH_1);
	writel(VIC_FIFO_TH_0_DEFAULT, sd->base + VIC_FIFO_TH_0);
	writel(VIC_FIFO_TH_2_DEFAULT, sd->base + VIC_FIFO_TH_2);
	writel(VIC_CTRL_RESET_BIT1, sd->base + VIC_CONTROL_REG);
	writel(VIC_CTRL_RESET_BIT2, sd->base + VIC_CONTROL_REG);
	writel(0, sd->base + VIC_FRM_CTRL);
	/*
	 * A frame-sync slave's clock starts after the master's first exposure
	 * and the slave's delay, up to a frame each: 109 ms at a 3000-line
	 * frame with a 2994-line master exposure.
	 */
	ret = readl_poll_timeout(sd->base + VIC_CONTROL_REG, value, !value,
				 100, 500000);
	if (ret) {
		dev_err(&vic->pdev->dev, "VIC reset timed out: %#x\n", value);
		goto unlock;
	}
	writel(0, sd->base + VIC_DVP_TIMING0);
	writel(0, sd->base + VIC_DVP_TIMING1);
	if (g_tx_isp_dev->processed) {
		unsigned long rate = clk_get_rate(g_tx_isp_dev->cgu_isp);

		/* SC1A4T uses a 100 us hardware camera-switch delay. */
		writel(VIC_AUTO_SWITCH_EN | VIC_AUTO_SWITCH_ACTIVE | VIC_AUTO_SWITCH_PORT0,
		       sd->base + VIC_AUTO_SWITCH_CTRL);
		writel((rate / 1000000) * 100,
		       sd->base + VIC_AUTO_SWITCH_TIME);
	}
	writel(VIC_ENABLE_DEFAULT, sd->base + VIC_ENABLE_REG);
	writel(U32_MAX, sd->base + VIC_CLEAR_REG_0);
	writel(U32_MAX, sd->base + VIC_CLEAR_REG_1);
	writel(~(VIC_INT0_FRAME_START | VIC_INT0_VER_ERR_CH2 |
		 VIC_INT0_DMA_SYFIFO_OVF | VIC_INT0_CONTROL_LIMIT_ERR |
		 VIC_INT0_MIPI_CH0_HCOMP_ERR | VIC_INT0_MIPI_CH2_HCOMP_ERR),
	       sd->base + VIC_MASK_REG_0);
	writel(~(VIC_INT1_DMA_CH0_FRAME_DONE |
		 VIC_INT1_DMA_ARB_TRANS_DONE_OVF |
		 VIC_INT1_DMA_CHID_OVF),
	       sd->base + VIC_MASK_REG_1);
	if (!g_tx_isp_dev->processed)
		writel(VIC_CTRL_ENABLE, sd->base + VIC_CONTROL_REG);
	vic->running = true;
unlock:
	mutex_unlock(&vic->lock);
	return ret;
}

void tx_isp_vic_capture_trigger(struct tx_isp_subdev *sd, bool enable)
{
	writel(enable ? VIC_CTRL_ENABLE : 0, sd->base + VIC_CONTROL_REG);
	readl(sd->base + VIC_CONTROL_REG);
}

int tx_isp_vic_stop(struct tx_isp_subdev *sd)
{
	struct tx_isp_vic_device *vic = sd ? sd->dev_priv : NULL;

	if (!vic || !sd->base)
		return -EINVAL;
	mutex_lock(&vic->lock);
	if (vic->running) {
		tx_isp_vic_dma_stop(sd);
		writel(U32_MAX, sd->base + VIC_MASK_REG_0);
		writel(U32_MAX, sd->base + VIC_MASK_REG_1);
		writel(0, sd->base + VIC_CONTROL_REG);
		vic->running = false;
	}
	mutex_unlock(&vic->lock);
	return 0;
}

void tx_isp_vic_dma_arm(struct tx_isp_subdev *sd, dma_addr_t address)
{
	struct tx_isp_vic_device *vic = sd->dev_priv;
	unsigned int bank;

	writel(VIC_DMA_RESET_TRIG, sd->base + VIC_DMA_RESET);
	writel((vic->width << 16) | vic->height,
	       sd->base + VIC_DMA_RESOLUTION);
	writel(vic->width * 2, sd->base + VIC_DMA_STRIDE);
	writel(vic->width * 2, sd->base + VIC_DMA_STRIDE1);
	for (bank = 0; bank < 5; bank++)
		writel(address, sd->base + VIC_DMA_BANK(bank));
	/* One frame per arm; unqueued userspace buffers are never DMA targets. */
	writel(VIC_DMA_CTRL_ENABLE | VIC_DMA_CTRL_AUTO | VIC_DMA_CTRL_FMT_RAW10,
	       sd->base + VIC_DMA_CONTROL);
	writel(VIC_CTRL_ENABLE, sd->base + VIC_CONTROL_REG);
}

void tx_isp_vic_select_port(struct tx_isp_subdev *sd, unsigned int port)
{
	struct tx_isp_vic_device *vic = sd->dev_priv;

	/* Change the mux only while DMA is stopped. */
	vic->port = port;
	writel(port ? VIC_PORT_SEL_PORT1 : VIC_PORT_SEL_PORT0,
	       sd->base + VIC_PORT_SEL);
	readl(sd->base + VIC_PORT_SEL);
}

void tx_isp_vic_dma_stop(struct tx_isp_subdev *sd)
{
	writel(0, sd->base + VIC_DMA_CONTROL);
	readl(sd->base + VIC_DMA_CONTROL);
}

void tx_isp_vic_dma_resume(struct tx_isp_subdev *sd)
{
	writel(readl(sd->base + VIC_DMA_CONTROL) | VIC_DMA_CTRL_ENABLE,
	       sd->base + VIC_DMA_CONTROL);
}

irqreturn_t isp_vic_interrupt_service_routine(struct tx_isp_subdev *sd,
					      bool *handled)
{
	struct tx_isp_vic_device *vic = sd ? sd->dev_priv : NULL;
	u32 mask, mask1, pending, pending1, errors, errors1;

	if (!vic || !sd->base)
		return IRQ_NONE;
	mask = readl(sd->base + VIC_MASK_REG_0);
	mask1 = readl(sd->base + VIC_MASK_REG_1);
	pending = readl(sd->base + VIC_STATUS_REG_0) & ~mask;
	pending1 = readl(sd->base + VIC_STATUS_REG_1) & ~mask1;
	if (!pending && !pending1)
		return IRQ_NONE;
	writel(pending, sd->base + VIC_CLEAR_REG_0);
	writel(pending1, sd->base + VIC_CLEAR_REG_1);

	errors = pending & GENMASK(31, 9);
	errors1 = pending1 & GENMASK(3, 2);
	if (errors || errors1)
		dev_err_ratelimited(&vic->pdev->dev,
				    "VIC error status: %#x/%#x\n", errors,
				    errors1);
	if ((pending & VIC_INT0_FRAME_START) &&
	    !(pending1 & VIC_INT1_DMA_CH0_FRAME_DONE) && !errors && !errors1 &&
	    (readl(sd->base + VIC_PORT_STATUS) & 1) == vic->port)
		t23_isp_video_frame_event(dev_get_drvdata(vic->pdev->dev.parent));
	if ((pending1 & VIC_INT1_DMA_CH0_FRAME_DONE) || errors || errors1) {
		struct tx_isp_device *isp = dev_get_drvdata(vic->pdev->dev.parent);
		bool bad_address = (pending1 & VIC_INT1_DMA_CH0_FRAME_DONE) &&
			readl(sd->base + VIC_DMA_CURRENT) != isp->dma_address;
		bool bad_port = (pending1 & VIC_INT1_DMA_CH0_FRAME_DONE) &&
			(readl(sd->base + VIC_PORT_STATUS) & 1) != vic->port;

		tx_isp_vic_dma_stop(sd);
		if (isp->processed) {
			if (errors || errors1)
				t23_isp_video_processed_event(isp, false, false, true);
		} else {
			t23_isp_video_dma_done(isp, errors || errors1 ||
					      bad_address || bad_port);
		}
	}
	if (handled)
		*handled = true;
	return IRQ_HANDLED;
}

static struct tx_isp_subdev_ops vic_subdev_ops = {
	.interrupt_service_routine = isp_vic_interrupt_service_routine,
};

int tx_isp_vic_probe(struct platform_device *pdev)
{
	struct tx_isp_vic_device *vic;
	int ret;

	vic = kzalloc(sizeof(*vic), GFP_KERNEL);
	if (!vic)
		return -ENOMEM;
	vic->pdev = pdev;
	mutex_init(&vic->lock);
	ret = tx_isp_subdev_init(pdev, &vic->sd, &vic_subdev_ops);
	if (ret) {
		kfree(vic);
		return ret;
	}
	vic->sd.dev_priv = vic;
	g_vic_dev = vic;
	return 0;
}

int tx_isp_vic_remove(struct platform_device *pdev)
{
	struct tx_isp_vic_device *vic = g_vic_dev;

	if (vic) {
		tx_isp_vic_stop(&vic->sd);
		tx_isp_subdev_deinit(&vic->sd);
		kfree(vic);
		g_vic_dev = NULL;
	}
	return 0;
}
