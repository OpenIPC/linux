/*
* Copyright (C) 2016 Ambarella, Inc.
*
* This software is licensed under the terms of the GNU General Public
* License version 2, as published by the Free Software Foundation, and
* may be copied, distributed, and modified under those terms.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*/

#include <linux/module.h>
#include <linux/delay.h>
#include <linux/of_i2c.h>
#include <linux/iio/iio.h>
#include "inv_mpu_iio.h"

int inv_mpu_i2cmst_read_data(struct inv_mpu6050_state *st,
		u16 addr, u8 command, int size, union i2c_smbus_data *data)
{
	unsigned int d, num;
	void *buf;
	int result;

	result = inv_mpu6050_set_power_itg(st, true);
	if (result)
		return result;

	switch (size) {
	case I2C_SMBUS_BYTE_DATA:
		num = 1;
		buf = &data->byte;
		break;
	case I2C_SMBUS_WORD_DATA:
		num = 2;
		buf = &data->word;
		break;
	case I2C_SMBUS_I2C_BLOCK_DATA:
		num = data->block[0];
		buf = &data->block[1];
		break;
	default:
		return -EINVAL;
	}

	d = INV_MPU6050_BIT_I2C_SLV0_EN | num;
	result = regmap_write(st->map, INV_MPU6050_REG_I2C_SLV0_CTRL, d);
	if (result)
		return result;

	result = regmap_write(st->map, INV_MPU6050_REG_I2C_SLV0_REG, command);
	if (result)
		return result;

	d = INV_MPU6050_BIT_I2C_SLV0_RNW | addr;
	result = regmap_write(st->map, INV_MPU6050_REG_I2C_SLV0_ADDR, d);
	if (result)
		return result;

	d = 1000 / st->chip_config.fifo_rate;
	msleep(d + d / 2);

	result = regmap_bulk_read(st->map, INV_MPU6050_REG_EXT_SENS_DATA, buf, num);
	if (result)
		return result;

	result = inv_mpu6050_set_power_itg(st, false);
	if (result)
		return result;

	return 0;
}

int inv_mpu_i2cmst_write_data(struct inv_mpu6050_state *st,
		u16 addr, u8 command, int size, union i2c_smbus_data *data)
{
	unsigned int d;
	int result;

	if (size != I2C_SMBUS_BYTE_DATA) {
		dev_err(&st->adap->dev, "funky write size %d\n", size);
		return -EINVAL;
	}

	result = inv_mpu6050_set_power_itg(st, true);
	if (result)
		return result;

	d = INV_MPU6050_BIT_I2C_SLV0_EN | 1; /* only one byte allowed to write */
	result = regmap_write(st->map, INV_MPU6050_REG_I2C_SLV0_CTRL, d);
	if (result)
		return result;

	result = regmap_write(st->map, INV_MPU6050_REG_I2C_SLV0_REG, command);
	if (result)
		return result;

	result = regmap_write(st->map, INV_MPU6050_REG_I2C_SLV0_DO, data->byte);
	if (result)
		return result;

	result = regmap_write(st->map, INV_MPU6050_REG_I2C_SLV0_ADDR, addr);
	if (result)
		return result;

	d = 1000 / st->chip_config.fifo_rate;
	msleep(d + d / 2);

	result = inv_mpu6050_set_power_itg(st, false);
	if (result)
		return result;

	return 0;
}

static int inv_mpu_i2cmst_xfer(struct i2c_adapter *adap, u16 addr,
		unsigned short flags, char rw, u8 command,
		int size, union i2c_smbus_data *data)
{
	struct inv_mpu6050_state *st = i2c_get_adapdata(adap);
	int ret = -EINVAL;

	if (rw == I2C_SMBUS_WRITE)
		ret = inv_mpu_i2cmst_write_data(st, addr, command, size, data);
	else if (rw == I2C_SMBUS_READ)
		ret = inv_mpu_i2cmst_read_data(st, addr, command, size, data);

	return ret;
}

static u32 inv_mpu_i2cmst_func(struct i2c_adapter *adap)
{
	return I2C_FUNC_SMBUS_BYTE_DATA | I2C_FUNC_SMBUS_READ_I2C_BLOCK;
}

static const struct i2c_algorithm inv_mpu_i2cmst_algo = {
	.smbus_xfer	= inv_mpu_i2cmst_xfer,
	.functionality	= inv_mpu_i2cmst_func,
};

int inv_mpu_i2cmst_probe(struct iio_dev *indio_dev)
{
	struct inv_mpu6050_state *st = iio_priv(indio_dev);
	struct i2c_adapter *adap;
	int ret;

	adap = devm_kzalloc(&indio_dev->dev, sizeof(*adap), GFP_KERNEL);
	if (!adap)
		return -ENOMEM;

	adap->owner = THIS_MODULE;
	adap->class = I2C_CLASS_HWMON;
	strlcpy(adap->name, "INV MPU I2CMST adapter", sizeof(adap->name));
	adap->algo = &inv_mpu_i2cmst_algo;
	adap->dev.parent = &indio_dev->dev;
	adap->dev.of_node = indio_dev->dev.of_node;

	i2c_set_adapdata(adap, st);
	st->adap = adap;

	ret = i2c_add_adapter(adap);
	if (ret < 0) {
		dev_err(&indio_dev->dev, "failed to add I2C master: %d\n", ret);
		return ret;
	}

	of_i2c_register_devices(adap);

	return 0;
}

void inv_mpu_i2cmst_remove(struct iio_dev *indio_dev)
{
	struct inv_mpu6050_state *st = iio_priv(indio_dev);
	i2c_del_adapter(st->adap);
}

MODULE_AUTHOR("Cao Rongrong <rrcao@ambarella.com>");
MODULE_DESCRIPTION("Invensense MPU9250 I2C Master driver");
MODULE_LICENSE("GPL");
