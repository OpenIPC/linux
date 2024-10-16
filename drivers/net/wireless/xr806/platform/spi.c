/*
 * xr806/spi.c
 *
 * Copyright (c) 2022
 * Allwinner Technology Co., Ltd. <www.allwinner.com>
 * laumy <liumingyuan@allwinner.com>
 *
 * hardware interface spi implementation for Xr806 drivers
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 */

#include <linux/spi/spi.h>
#include <linux/skbuff.h>
#include <linux/interrupt.h>
#include <linux/device.h>
#include <linux/of.h>
#include <linux/gpio.h>
#include <linux/of_gpio.h>

#include "hwio.h"
#include "debug.h"

struct xradio_spi_priv {
	int gpio_rw;
	int gpio_wakeup;
	rx_ind_func rx_ind;
	struct device_node *nd;
	struct spi_device *spi_dev;
	void *xr_priv;
};

static struct xradio_spi_priv spi_priv;

static int xradio_gpio_init(void);
static int xradio_gpio_deinit(void);
static int xradio_spi_read_rx_gpio(void);
static int xradio_spi_read_rw_gpio(void);
static int xradio_reg_rx_ind(rx_ind_func func);
static int xradio_spi_init(void *priv);
static int xradio_spi_read(u8 *buff, u32 len);
static int xradio_spi_write(u8 *buff, u16 len);
static int xradio_spi_deinit(void *priv);

static struct xradio_bus_ops spi_ops = {
	.gpio_init = xradio_gpio_init,
	.gpio_deinit = xradio_gpio_deinit,
	.reg_rx_ind = xradio_reg_rx_ind,
	.read_rx_gpio = xradio_spi_read_rx_gpio,
	.read_rw_gpio = xradio_spi_read_rw_gpio,
	.init = xradio_spi_init,
	.write = xradio_spi_write,
	.read = xradio_spi_read,
	.deinit = xradio_spi_deinit,
};

static const struct of_device_id wlan_gpio_ids[] = {
	{.compatible = "xradio,wlan"},
	{},
};

static int xradio_gpio_init(void)
{
	spi_priv.nd = of_find_matching_node(NULL, wlan_gpio_ids);
	if (!spi_priv.nd) {
		spi_printk(XRADIO_DBG_ERROR, "matching spi node failed\n");
		return -1;
	}

	spi_priv.gpio_rw = of_get_named_gpio(spi_priv.nd, "wlan_data_rd", 0);

	if (spi_priv.gpio_rw < 0) {
		spi_printk(XRADIO_DBG_ERROR, "spi gpio tranc get failed\n");
		return -1;
	}

	if (gpio_request(spi_priv.gpio_rw, "data_rd")) {
		spi_printk(XRADIO_DBG_ERROR, "spi gpio spi tranc request failed:%d\n",
			   spi_priv.gpio_rw);
		return -1;
	}

	gpio_direction_input(spi_priv.gpio_rw);

	spi_priv.gpio_wakeup = of_get_named_gpio(spi_priv.nd, "wlan_data_irq", 0);

	if (spi_priv.gpio_wakeup < 0) {
		spi_printk(XRADIO_DBG_ERROR, "spi gpio wakeup get failed\n");
		gpio_free(spi_priv.gpio_rw);
		return -1;
	}

	if (gpio_request(spi_priv.gpio_wakeup, "data_irq")) {
		spi_printk(XRADIO_DBG_ERROR, "spi gpio wakeup request failed:%d\n",
			   spi_priv.gpio_wakeup);
		gpio_free(spi_priv.gpio_rw);
		return -1;
	}

	gpio_direction_input(spi_priv.gpio_wakeup);

	spi_printk(XRADIO_DBG_ALWY, "wlan gpio rw:%d, irq:%d\n", spi_priv.gpio_rw,
		   spi_priv.gpio_wakeup);

	return 0;
}

static irqreturn_t xradio_plat_interrupt_handler(int irq, void *dev)
{
	if (spi_priv.rx_ind)
		spi_priv.rx_ind(spi_priv.xr_priv);
	return IRQ_HANDLED;
}

static int xradio_reg_rx_ind(rx_ind_func func)
{
	int status;
	int gpio_irq;

	spi_printk(XRADIO_DBG_ALWY, "Register irq callback:%d\n", spi_priv.gpio_wakeup);

	gpio_irq = gpio_to_irq(spi_priv.gpio_wakeup);

	spi_priv.rx_ind = func;

	status = request_irq(gpio_irq, xradio_plat_interrupt_handler,
			     IRQF_TRIGGER_LOW | IRQF_NO_SUSPEND, "xradio_irq", &spi_priv);

	if (status) {
		spi_printk(XRADIO_DBG_ERROR, "Failed to request IRQ for spi wakeup:%d, %d\n",
			   gpio_irq, status);
		gpio_free(spi_priv.gpio_rw);
		gpio_free(spi_priv.gpio_wakeup);
		return -1;
	}

	return status;
}

static int xradio_spi_read_rx_gpio(void)
{
	return gpio_get_value(spi_priv.gpio_wakeup);
}

static int xradio_spi_read_rw_gpio(void)
{
	return gpio_get_value(spi_priv.gpio_rw);
}

static int xradio_gpio_deinit(void)
{
	int gpio_irq;

	spi_printk(XRADIO_DBG_MSG, "wlan gpio tranc:%d, wakeup:%d\n", spi_priv.gpio_rw,
		   spi_priv.gpio_wakeup);
	spi_printk(XRADIO_DBG_ALWY, "xradio rxrd gpio deinit.\n");

	gpio_irq = gpio_to_irq(spi_priv.gpio_wakeup);

	disable_irq(gpio_irq);

	free_irq(gpio_irq, &spi_priv);

	gpio_free(spi_priv.gpio_wakeup);

	gpio_free(spi_priv.gpio_rw);

	return 0;
}

static int xradio_spi_dev_probe(struct spi_device *spi)
{
	spi_printk(XRADIO_DBG_ALWY, "spi probe, setup spi.\n");

	spi_priv.spi_dev = spi;

	spi->max_speed_hz = (50 * 1000 * 1000);

	spi->mode = SPI_MODE_0;

	spi_setup(spi);

	return 0;
}

static int xradio_spi_dev_remove(struct spi_device *spi)
{
	spi_printk(XRADIO_DBG_ALWY, "spi remove.\n");

	spi_priv.spi_dev = NULL;
	return 0;
}

static const struct of_device_id spidev_dt_ids[] = {
	{.compatible = "xradio,xr806"},
	{},
};

static struct spi_driver xr_spi_driver = {
	.driver = {
			.name = "xr_spidev",
			.of_match_table = of_match_ptr(spidev_dt_ids),
		},
	.probe = xradio_spi_dev_probe,
	.remove = xradio_spi_dev_remove,
};

static int xradio_spi_init(void *priv)
{
	int status;

	spi_priv.xr_priv = priv;

	spi_printk(XRADIO_DBG_ALWY, "Spi register driver.\n");

	status = spi_register_driver(&xr_spi_driver);

	if (status < 0)
		spi_printk(XRADIO_DBG_ERROR, "Register spi driver failed.\n ");

	return status;
}

static int xradio_spi_deinit(void *priv)
{
	spi_printk(XRADIO_DBG_ALWY, "Spi unregister driver.\n");

	spi_unregister_driver(&xr_spi_driver);

	spi_priv.xr_priv = NULL;

	memset(&spi_priv, 0, sizeof(spi_priv));

	return 0;
}

static int xradio_spi_read(u8 *buff, u32 len)
{
	struct spi_message m;
	struct spi_transfer t = {
		.rx_buf = buff,
		.len = len,
	};
	int status;

	spi_message_init(&m);
	spi_message_add_tail(&t, &m);

	status = spi_sync(spi_priv.spi_dev, &m);

	if (!status) {
		status = m.actual_length;
		spi_printk(XRADIO_DBG_MSG, "Spi read actual length:%d\n", m.actual_length);
	}
	return status;
}

static int xradio_spi_write(u8 *buff, u16 len)
{
	struct spi_transfer t = {
		.tx_buf = buff,
		.len = len,
	};
	struct spi_message m;
	int status;

	spi_message_init(&m);
	spi_message_add_tail(&t, &m);
	status = spi_sync(spi_priv.spi_dev, &m);

	if (status)
		spi_printk(XRADIO_DBG_ERROR, "Spi write error:%d\n", status);

	return status;
}

struct xradio_bus_ops *xradio_get_bus_ops(void)
{
	return &spi_ops;
}
