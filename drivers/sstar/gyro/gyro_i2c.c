/*
 * gyro_i2c.c - Sigmastar
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
#include <linux/i2c.h>
#include <linux/kernel.h>
#include "gyro.h"
#include "gyro_core.h"

static struct gyro_dev *tmp_gyro_dev;

static int gyro_i2c_read_regs(struct gyro_dev *dev, u8 reg, void *val, int len)
{
    struct i2c_client *client = container_of(dev->transfer_dev, struct i2c_client, dev);
    struct i2c_msg     msgs[2];
    int                ret;

    msgs[0].addr  = client->addr;
    msgs[0].flags = 0;
    msgs[0].buf   = &reg;
    msgs[0].len   = 1;

    msgs[1].addr  = client->addr;
    msgs[1].flags = I2C_M_RD;
    msgs[1].buf   = val;
    msgs[1].len   = len;

    ret = i2c_transfer(client->adapter, msgs, 2);

    return ret < 0 ? ret : (ret != ARRAY_SIZE(msgs) ? -EIO : 0);
}

static int gyro_i2c_read_reg(struct gyro_dev *dev, u8 reg, u8 *val)
{
    return gyro_i2c_read_regs(dev, reg, val, 1);
}

static int gyro_i2c_write_reg(struct gyro_dev *dev, u8 reg, u8 val)
{
    struct i2c_client *client = container_of(dev->transfer_dev, struct i2c_client, dev);
    struct i2c_msg     msg;
    int                ret;

    u8 buf[2] = {0};
    buf[0]    = reg;
    buf[1]    = val;

    msg.addr  = client->addr;
    msg.flags = 0;
    msg.buf   = buf;
    msg.len   = 2;
    ret       = i2c_transfer(client->adapter, &msg, 1);

    return ret < 0 ? ret : (ret != 1 ? -EIO : 0);
}

static struct gyro_reg_ops i2c_reg_ops = {
    .read_regs = gyro_i2c_read_regs,
    .read_reg  = gyro_i2c_read_reg,
    .write_reg = gyro_i2c_write_reg,
};

static int gyro_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
    tmp_gyro_dev->transfer_dev = &client->dev;
    GYRO_DBG("gyro i2c probe");
    return 0;
}

static int gyro_i2c_remove(struct i2c_client *client)
{
    tmp_gyro_dev->transfer_dev = NULL;
    GYRO_DBG("gyro i2c remove");
    return 0;
}

static const struct i2c_device_id gyro_i2c_id[] = {
    {"gyro", 0},
    {},
};
MODULE_DEVICE_TABLE(i2c, gyro_i2c_id);

static const struct of_device_id gyro_i2c_of_match[] = {
    {.compatible = "sstar,gyro"},
    {},
};
MODULE_DEVICE_TABLE(of, gyro_i2c_of_match);

static struct i2c_driver gyro_i2c_driver = {
    .probe  = gyro_i2c_probe,
    .remove = gyro_i2c_remove,
    .driver =
        {
            .owner          = THIS_MODULE,
            .name           = "gyro",
            .of_match_table = of_match_ptr(gyro_i2c_of_match),
        },
    .id_table = gyro_i2c_id,
};

int gyro_transfer_init(struct gyro_dev *dev)
{
    int ret      = 0;
    tmp_gyro_dev = dev;

    ret = i2c_add_driver(&gyro_i2c_driver);
    if (0 != ret)
    {
        GYRO_ERR("Add i2c driver error.");
        goto err_i2c_add_driver;
    }

    dev->reg_ops = &i2c_reg_ops;

    GYRO_DBG("Gyro i2c init");
    return 0;

err_i2c_add_driver:
    return ret;
}

void gyro_transfer_deinit(struct gyro_dev *dev)
{
    dev->reg_ops = NULL;
    i2c_del_driver(&gyro_i2c_driver);
    tmp_gyro_dev = NULL;
    GYRO_DBG("Gyro i2c deinit");
}

MODULE_LICENSE("GPL");
