/*
 * I2C slave mode SUNXI simulator
 *
 * Copyright (C) 2014 by Wolfram Sang, Sang Engineering <wsa@sang-engineering.com>
 * Copyright (C) 2014 by Renesas Electronics Corporation
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; version 2 of the License.
 *
 * Because most IP blocks can only detect one I2C slave address anyhow, this
 * driver does not support simulating SUNXI types which take more than one
 * address. It is prepared to simulate bigger SUNXIs with an internal 16 bit
 * pointer, yet implementation is deferred until the need actually arises.
 */

#include <linux/i2c.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/sysfs.h>

struct sunxi_data {
	struct bin_attribute bin;
	bool first_write;
	spinlock_t buffer_lock;
	u8 buffer_idx;
	u8 buffer[256];
};

static int i2c_slave_sunxi_slave_cb(struct i2c_client *client,
				     enum i2c_slave_event event, u8 *val)
{
	struct sunxi_data *sunxi = i2c_get_clientdata(client);

	switch (event) {
	case I2C_SLAVE_WRITE_RECEIVED:
		if (sunxi->first_write) {
			sunxi->buffer_idx = *val;
			sunxi->first_write = false;
		} else {
			spin_lock(&sunxi->buffer_lock);
			sunxi->buffer[sunxi->buffer_idx++] = *val;
			spin_unlock(&sunxi->buffer_lock);
		}
		break;

	case I2C_SLAVE_READ_PROCESSED:
		/* The previous byte made it to the bus, get next one */
		sunxi->buffer_idx++;
		/* fallthrough */
	case I2C_SLAVE_READ_REQUESTED:
		spin_lock(&sunxi->buffer_lock);
		*val = sunxi->buffer[sunxi->buffer_idx];
		spin_unlock(&sunxi->buffer_lock);
		/*
		 * Do not increment buffer_idx here, because we don't know if
		 * this byte will be actually used. Read Linux I2C slave docs
		 * for details.
		 */
		break;

	case I2C_SLAVE_STOP:
	case I2C_SLAVE_WRITE_REQUESTED:
		sunxi->first_write = true;
		break;

	default:
		break;
	}

	return 0;
}

static ssize_t i2c_slave_sunxi_bin_read(struct file *filp, struct kobject *kobj,
		struct bin_attribute *attr, char *buf, loff_t off, size_t count)
{
	struct sunxi_data *sunxi;
	unsigned long flags;

	sunxi = dev_get_drvdata(container_of(kobj, struct device, kobj));

	spin_lock_irqsave(&sunxi->buffer_lock, flags);
	memcpy(buf, &sunxi->buffer[off], count);
	spin_unlock_irqrestore(&sunxi->buffer_lock, flags);

	return count;
}

static ssize_t i2c_slave_sunxi_bin_write(struct file *filp, struct kobject *kobj,
		struct bin_attribute *attr, char *buf, loff_t off, size_t count)
{
	struct sunxi_data *sunxi;
	unsigned long flags;

	sunxi = dev_get_drvdata(container_of(kobj, struct device, kobj));

	spin_lock_irqsave(&sunxi->buffer_lock, flags);
	memcpy(&sunxi->buffer[off], buf, count);
	spin_unlock_irqrestore(&sunxi->buffer_lock, flags);

	return count;
}

static int i2c_slave_sunxi_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	struct sunxi_data *sunxi;
	int ret;
	unsigned size = id->driver_data;

	printk("i2c slave sunxi probe\n");
	sunxi = devm_kzalloc(&client->dev, sizeof(struct sunxi_data) + size, GFP_KERNEL);
	if (!sunxi)
		return -ENOMEM;

	sunxi->first_write = true;
	spin_lock_init(&sunxi->buffer_lock);
	i2c_set_clientdata(client, sunxi);

	sysfs_bin_attr_init(&sunxi->bin);
	sunxi->bin.attr.name = "slave-sunxi";
	sunxi->bin.attr.mode = S_IRUSR | S_IWUSR;
	sunxi->bin.read = i2c_slave_sunxi_bin_read;
	sunxi->bin.write = i2c_slave_sunxi_bin_write;
	sunxi->bin.size = size;

	ret = sysfs_create_bin_file(&client->dev.kobj, &sunxi->bin);
	if (ret)
		return ret;

	ret = i2c_slave_register(client, i2c_slave_sunxi_slave_cb);
	if (ret) {
		sysfs_remove_bin_file(&client->dev.kobj, &sunxi->bin);
		return ret;
	}

	return 0;
};

static int i2c_slave_sunxi_remove(struct i2c_client *client)
{
	struct sunxi_data *sunxi = i2c_get_clientdata(client);

	i2c_slave_unregister(client);
	sysfs_remove_bin_file(&client->dev.kobj, &sunxi->bin);

	return 0;
}

static const struct i2c_device_id i2c_slave_sunxi_id[] = {
	{ "slave-sunxi", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, i2c_slave_sunxi_id);

static struct i2c_driver i2c_slave_sunxi_driver = {
	.driver = {
		.name = "i2c-slave-sunxi",
	},
	.probe = i2c_slave_sunxi_probe,
	.remove = i2c_slave_sunxi_remove,
	.id_table = i2c_slave_sunxi_id,
};
module_i2c_driver(i2c_slave_sunxi_driver);

MODULE_AUTHOR("Wolfram Sang <wsa@sang-engineering.com>");
MODULE_DESCRIPTION("I2C slave mode SUNXI simulator");
MODULE_LICENSE("GPL v2");
