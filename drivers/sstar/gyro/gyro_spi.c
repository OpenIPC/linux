/*
 * gyro_spi.c - Sigmastar
 *
 * Copyright (c) [2019~2020] SigmaStar Technology.
 *
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License version 2 for more details.
 *
 */
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/printk.h>
#include <linux/spi/spi.h>
#include <linux/of.h>
#include <linux/gpio.h>
#include "gyro_core.h"
#include "gyro.h"

int gyro_spi_read_regs(struct gyro_dev *dev, u8 reg, void *val, int len)
{
    struct spi_device * spi = container_of(dev->transfer_dev, struct spi_device, dev);
    struct spi_message  msg;
    u8                  tx_reg = 0x80 | reg;
    struct spi_transfer t[]    = {{.rx_buf = NULL, .tx_buf = &tx_reg, .len = 1},
                               {.rx_buf = val, .tx_buf = NULL, .len = len}};
    int                 ret;
    spi_message_init(&msg);
    spi_message_add_tail(&t[0], &msg);
    spi_message_add_tail(&t[1], &msg);
    ret = spi_sync(spi, &msg);
    return ret;
}

int gyro_spi_read_reg(struct gyro_dev *dev, u8 reg, u8 *val)
{
    return gyro_spi_read_regs(dev, reg, val, 1);
}

int gyro_spi_write_reg(struct gyro_dev *dev, u8 reg, u8 val)
{
    struct spi_device * spi = container_of(dev->transfer_dev, struct spi_device, dev);
    struct spi_message  msg;
    struct spi_transfer t[] = {{.rx_buf = NULL, .tx_buf = &reg, .len = 1}, {.rx_buf = NULL, .tx_buf = &val, .len = 1}};
    int                 ret;
    spi_message_init(&msg);
    spi_message_add_tail(&t[0], &msg);
    spi_message_add_tail(&t[1], &msg);
    ret = spi_sync(spi, &msg);
    return ret;
}

static struct gyro_reg_ops spi_reg_ops = {
    .read_regs = gyro_spi_read_regs,
    .read_reg  = gyro_spi_read_reg,
    .write_reg = gyro_spi_write_reg,
};

static int gyro_spi_probe(struct spi_device *spi)
{
    int ret = 0;
    GYRO_DBG("gyro_spi probe");
    ret = gyro_cdev_probe();
    if (0 != ret)
    {
        return ret;
    }
    return gyro_probe(&spi->dev, &spi_reg_ops);
}

static int gyro_spi_remove(struct spi_device *spi)
{
    GYRO_DBG("gyro_spi remove");
    gyro_remove();
    return gyro_cdev_remove();
}

static const struct spi_device_id gyro_spi_id[] = {
    {"gyro", 0},
    {},
};
MODULE_DEVICE_TABLE(spi, gyro_spi_id);

static const struct of_device_id gyro_spi_of_match[] = {
    {.compatible = "sstar,gyro_spi"},
    {},
};
MODULE_DEVICE_TABLE(of, gyro_spi_of_match);

static struct spi_driver gyro_spi_driver = {
    .probe  = gyro_spi_probe,
    .remove = gyro_spi_remove,
    .driver =
        {
            .owner          = THIS_MODULE,
            .name           = "gyro_spi",
            .of_match_table = of_match_ptr(gyro_spi_of_match),
        },
    .id_table = gyro_spi_id,
};

int gyro_spi_init(void)
{
    int ret = 0;
    ret     = spi_register_driver(&gyro_spi_driver);
    if (0 != ret)
    {
        GYRO_ERR("Add i2c driver error");
        goto err_i2c_add_driver;
    }
    GYRO_DBG("Gyro spi init");
    return 0;

err_i2c_add_driver:
    return ret;
}

void gyro_spi_exit(void)
{
    spi_unregister_driver(&gyro_spi_driver);
    GYRO_DBG("Gyro spi exit");
}

MODULE_LICENSE("GPL");
