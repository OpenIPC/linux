// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2023-2024 AXERA
 */

#include <linux/gpio/driver.h>
#include <linux/i2c.h>
#include <linux/interrupt.h>
#include <linux/module.h>
#include <linux/of_irq.h>
#include <linux/seq_file.h>
#include <linux/slab.h>

#define INPUT_PORT_REGISTER0			0x00   /* input reg:IO00-IO07 */
#define INPUT_PORT_REGISTER1			0x01   /* input reg:IO10-IO17 */
#define OUTPUT_PORT_REGISTER0			0x02   /* output reg:IO00-IO07 */
#define OUTPUT_PORT_REGISTER1			0x03   /* output reg:IO10-IO17 */
#define POLARITY_INVERSION_PORT_REGISTER0	0x04   /* polarity reg:IO00-IO07 */
#define POLARITY_INVERSION_PORT_REGISTER1	0x05   /* polarity reg:IO10-IO17 */
#define CONFIG_PORT_REGISTER0			0x06   /* config reg:IO00-IO07 */
#define CONFIG_PORT_REGISTER1			0x07   /* config reg:IO10-IO17 */

struct ax_ext_gpio {
	struct i2c_client *client;
	struct gpio_chip gpio;
	struct mutex i2c_lock;
};

static int ax_read(struct ax_ext_gpio *ax_gpio, unsigned offset, uint8_t *value)
{
	int err;

	err = i2c_smbus_read_byte_data(ax_gpio->client, offset);
	if (err < 0) {
		dev_err(ax_gpio->gpio.parent, "%s failed: %d\n",
			"i2c_smbus_read_byte_data()", err);
		return err;
	}

	*value = err;
	return 0;
}

static int ax_write(struct ax_ext_gpio *ax_gpio, unsigned offset, uint8_t value)
{
	int err;

	err = i2c_smbus_write_byte_data(ax_gpio->client, offset, value);
	if (err < 0) {
		dev_err(ax_gpio->gpio.parent, "%s failed: %d\n",
			"i2c_smbus_write_byte_data()", err);
		return err;
	}

	return 0;
}
static int ax_port_judge(unsigned offset)
{
	if (offset > 7)
		return 1;
	else
		return 0;
}

static int ax_gpio_config_input_or_output(struct gpio_chip *chip, unsigned offset, bool dir)
{
	struct ax_ext_gpio *ax_gpio = gpiochip_get_data(chip);
	int err;
	u8 out_value, in_value;
	/************************config output 0/input 1****************************/
	if (dir) {
		if (ax_port_judge(offset)) {
			err = ax_read(ax_gpio, CONFIG_PORT_REGISTER1, &in_value);
			if (err < 0)
				return err;
			in_value |= BIT(offset - 8);
			ax_write(ax_gpio, CONFIG_PORT_REGISTER1, in_value);
		} else {
			err = ax_read(ax_gpio, CONFIG_PORT_REGISTER0, &in_value);
			if (err < 0)
				return err;
			in_value |= BIT(offset);
			ax_write(ax_gpio, CONFIG_PORT_REGISTER0, in_value);
		}
	} else {
		if (ax_port_judge(offset)) {
			err = ax_read(ax_gpio, CONFIG_PORT_REGISTER1, &out_value);
			if (err < 0)
				return err;
			out_value &= (~BIT(offset - 8));
			ax_write(ax_gpio, CONFIG_PORT_REGISTER1, out_value);
		} else {
			err = ax_read(ax_gpio, CONFIG_PORT_REGISTER0, &out_value);
			if (err < 0)
				return err;
			out_value &= (~BIT(offset));
			ax_write(ax_gpio, CONFIG_PORT_REGISTER0, out_value);
		}
	}
	return 0;
}
static int ax_gpio_get(struct gpio_chip *chip, unsigned offset)
{
	struct ax_ext_gpio *ax_gpio = gpiochip_get_data(chip);
	u8 value;
	int err;

	if (ax_port_judge(offset)) {
		err = ax_read(ax_gpio, CONFIG_PORT_REGISTER1, &value);
		if (err < 0)
			return err;
		value &= BIT(offset - 8);
		if (value) {
			err = ax_read(ax_gpio, OUTPUT_PORT_REGISTER1, &value);
			if (err < 0)
				return err;
			return (value & BIT(offset - 8)) ? 1 : 0;
		} else {
			err = ax_read(ax_gpio, INPUT_PORT_REGISTER1, &value);
			if (err < 0)
				return err;
			return (value & BIT(offset - 8)) ? 1 : 0;
		}
	} else {
		err = ax_read(ax_gpio, CONFIG_PORT_REGISTER0, &value);
		if (err < 0)
			return err;
		value &= BIT(offset);
		if (value) {
			err = ax_read(ax_gpio, OUTPUT_PORT_REGISTER0, &value);
			if (err < 0)
				return err;
			return (value & BIT(offset)) ? 1 : 0;
		} else {
			err = ax_read(ax_gpio, INPUT_PORT_REGISTER0, &value);
			if (err < 0)
				return err;
			return (value & BIT(offset)) ? 1 : 0;
		}
	}

	return -1;
}

static void __ax_gpio_set(struct ax_ext_gpio *ax_gpio, unsigned offset, int value)
{
	int err;
	u8 out_val;

	err = ax_gpio_config_input_or_output(&ax_gpio->gpio, offset, 0);
	if (err)
		printk("ax_gpio_config_input_or_output fail\n");
	if (ax_port_judge(offset)) {
		err = ax_read(ax_gpio, OUTPUT_PORT_REGISTER1, &out_val);
		if (err < 0)
			return;
		if (value)
			out_val |= BIT(offset - 8);
		else
			out_val &= (~BIT(offset - 8));
		ax_write(ax_gpio, OUTPUT_PORT_REGISTER1, out_val);
	} else {
		err = ax_read(ax_gpio, OUTPUT_PORT_REGISTER0, &out_val);
		if (err < 0)
			return;
		if (value)
			out_val |= BIT(offset);
		else
			out_val &= (~BIT(offset));
		ax_write(ax_gpio, OUTPUT_PORT_REGISTER0, out_val);
	}
}

static void ax_gpio_set(struct gpio_chip *chip, unsigned offset, int value)
{
	struct ax_ext_gpio *ax_gpio = gpiochip_get_data(chip);

	mutex_lock(&ax_gpio->i2c_lock);
	__ax_gpio_set(ax_gpio, offset, value);
	mutex_unlock(&ax_gpio->i2c_lock);
}

static int ax_gpio_direction_input(struct gpio_chip *chip, unsigned offset)
{
	struct ax_ext_gpio *ax_gpio = gpiochip_get_data(chip);
	int err;

	mutex_lock(&ax_gpio->i2c_lock);

	err = ax_gpio_config_input_or_output(chip, offset, 1);
	if (err < 0)
		goto out;

	err = 0;

out:
	mutex_unlock(&ax_gpio->i2c_lock);
	return err;
}

static int ax_gpio_direction_output(struct gpio_chip *chip, unsigned offset,
				      int value)
{
	struct ax_ext_gpio *ax_gpio = gpiochip_get_data(chip);

	mutex_lock(&ax_gpio->i2c_lock);
	__ax_gpio_set(ax_gpio, offset, value);
	mutex_unlock(&ax_gpio->i2c_lock);
	return 0;
}


static int ax_gpio_setup(struct ax_ext_gpio *ax_gpio, unsigned int num_gpios)
{
	struct gpio_chip *chip = &ax_gpio->gpio;
	int err;

	chip->direction_input = ax_gpio_direction_input;
	chip->direction_output = ax_gpio_direction_output;
	chip->get = ax_gpio_get;
	chip->set = ax_gpio_set;
	chip->can_sleep = false;

	chip->base = 128;
	chip->ngpio = num_gpios;
	chip->label = ax_gpio->client->name;
	chip->parent = &ax_gpio->client->dev;
	chip->of_node = chip->parent->of_node;
	chip->owner = THIS_MODULE;


	err = devm_gpiochip_add_data(&ax_gpio->client->dev, chip, ax_gpio);
	if (err)
		return err;

	return 0;
}

static int ax_i2c_probe(struct i2c_client *client,
				    const struct i2c_device_id *id)
{
	struct device_node *np = client->dev.of_node;
	struct ax_ext_gpio *ax_gpio;
	u32 num_gpios;
	int err;

	err = of_property_read_u32(np, "nr-gpios", &num_gpios);
	if (err < 0)
		return err;

	ax_gpio = devm_kzalloc(&client->dev, sizeof(*ax_gpio), GFP_KERNEL);
	if (!ax_gpio)
		return -ENOMEM;

	mutex_init(&ax_gpio->i2c_lock);
	ax_gpio->client = client;

	err = ax_gpio_setup(ax_gpio, num_gpios);
	if (err)
		return err;

	i2c_set_clientdata(client, ax_gpio);

	return 0;
}

static const struct i2c_device_id ax_i2c_id[] = {
	{ "gpio-ax" },
	{ },
};
MODULE_DEVICE_TABLE(i2c, ax_i2c_id);

static const struct of_device_id ax_of_match[] = {
	{ .compatible = "axera,gpio-ext", },
	{ },
};
MODULE_DEVICE_TABLE(of, ax_of_match);

static struct i2c_driver ax_i2c_driver = {
	.driver = {
		.name = "gpio-ax",
		.of_match_table = ax_of_match,
	},
	.probe = ax_i2c_probe,
	.id_table = ax_i2c_id,
};
module_i2c_driver(ax_i2c_driver);

MODULE_DESCRIPTION("axera GPIO expander");
MODULE_AUTHOR("AXERA");
MODULE_LICENSE("GPL");
