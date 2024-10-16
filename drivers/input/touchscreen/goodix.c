/*
 *  Driver for Goodix Touchscreens
 *
 *  Copyright (c) 2014 Red Hat Inc.
 *
 *  This code is based on gt9xx.c authored by andrew@goodix.com:
 *
 *  2010 - 2012 Goodix Technology.
 */

/*
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; version 2 of the License.
 */

#include <linux/kernel.h>
#include <linux/dmi.h>
#include <linux/i2c.h>
#include <linux/input.h>
#include <linux/input/mt.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/slab.h>
#include <linux/acpi.h>
#include <linux/of.h>
#include <asm/unaligned.h>
#include <linux/gpio/consumer.h>
#include "gt5688_720x1280_cfg.h"

#define DBG_INFO(format, args...) (printk("[GOODIX 9XX INFO] LINE:%04d-->%s:"format, __LINE__, __func__, ##args))
#define DBG_ERR(format, args...) (printk("[GOODIX 9XX ERR] LINE:%04d-->%s:"format, __LINE__, __func__, ##args))

struct goodix_ts_data {
	struct i2c_client *client;
	struct input_dev *input_dev;
	struct gpio_desc *reset_gpio;
	struct gpio_desc *int_gpio;
	int abs_x_max;
	int abs_y_max;
	int offset_x;
	int offset_y;
	int invert_x;
	int invert_y;
	int swap_xy;
	unsigned int max_touch_num;
	unsigned int int_trigger_type;
	bool rotated_screen;
	int is_down;
};

#define GOODIX_MAX_HEIGHT		4096
#define GOODIX_MAX_WIDTH		4096
#define GOODIX_INT_TRIGGER		1
#define GOODIX_CONTACT_SIZE		8
#define GOODIX_MAX_CONTACTS		10

#define GOODIX_CONFIG_MAX_LENGTH	240

/* Register defines */
#define GOODIX_READ_COOR_ADDR		0x814E
#define GOODIX_REG_CONFIG_DATA		0x8047
#define GOODIX_REG_ID			0x8140
#define MAX_CONTACTS		1

//#define RESOLUTION_LOC		1
#define MAX_CONTACTS_LOC	5
//#define TRIGGER_LOC		6

static const unsigned long goodix_irq_flags[] = {
	IRQ_TYPE_EDGE_RISING,
	IRQ_TYPE_EDGE_FALLING,
	IRQ_TYPE_LEVEL_LOW,
	IRQ_TYPE_LEVEL_HIGH,
};

/*
 * Those tablets have their coordinates origin at the bottom right
 * of the tablet, as if rotated 180 degrees
 */
static const struct dmi_system_id rotated_screen[] = {
#if defined(CONFIG_DMI) && defined(CONFIG_X86)
	{
		.ident = "WinBook TW100",
		.matches = {
			DMI_MATCH(DMI_SYS_VENDOR, "WinBook"),
			DMI_MATCH(DMI_PRODUCT_NAME, "TW100")
		}
	},
	{
		.ident = "WinBook TW700",
		.matches = {
			DMI_MATCH(DMI_SYS_VENDOR, "WinBook"),
			DMI_MATCH(DMI_PRODUCT_NAME, "TW700")
		},
	},
#endif
	{}
};


static unsigned char config[GTP_CONFIG_MAX_LENGTH + GTP_ADDR_LENGTH]
= {GTP_REG_CONFIG_DATA >> 8, GTP_REG_CONFIG_DATA & 0xff};

static void goodix_reset(struct goodix_ts_data *ts)
{
	if (!ts->reset_gpio)
		return;

	/*
	 *
	 * address: 0xba/0xbb
	 *
	 * INT:  _____________________
	 *
	 * RESET: _____        ______
	 *             \______/
	 *
	 */
	DBG_INFO("reset the tp !!!!!!\n");
	gpiod_direction_output(ts->int_gpio, 0);
	gpiod_direction_output(ts->reset_gpio, 1);

	udelay(100);

	gpiod_set_value(ts->reset_gpio, 0);
	msleep(10);
	gpiod_set_value(ts->reset_gpio, 1);
	msleep(60);
	gpiod_set_value(ts->int_gpio, 0);

	gpiod_direction_input(ts->int_gpio);

}

/**
 * goodix_i2c_read - read data from a register of the i2c slave device.
 *
 * @client: i2c device.
 * @reg: the register to read from.
 * @buf: raw write data buffer.
 * @len: length of the buffer to write
 */
static int goodix_i2c_read(struct i2c_client *client,
		u16 reg, u8 *buf, int len)
{
	struct i2c_msg msgs[2];
	u16 wbuf = cpu_to_be16(reg);
	int ret;

	msgs[0].flags = 0;
	msgs[0].addr  = client->addr;
	msgs[0].len   = 2;
	msgs[0].buf   = (u8 *)&wbuf;

	msgs[1].flags = I2C_M_RD;
	msgs[1].addr  = client->addr;
	msgs[1].len   = len;
	msgs[1].buf   = buf;

	ret = i2c_transfer(client->adapter, msgs, 2);

	return ret < 0 ? ret : (ret != ARRAY_SIZE(msgs) ? -EIO : 0);
}

static int goodix_i2c_write(struct i2c_client *client,
		u16 reg, u8 *buf, int len)
{
	struct i2c_msg msgs[1];
	/* u16 wbuf = cpu_to_be16(reg); */
	int ret;

	msgs[0].flags = !I2C_M_RD;
	msgs[0].addr  = client->addr;
	msgs[0].len   = len;
	msgs[0].buf   = (u8 *)&buf;

	ret = i2c_transfer(client->adapter, msgs, 1);
	DBG_INFO("ret:%d\n", ret);
	return ret < 0 ? ret : (ret != ARRAY_SIZE(msgs) ? -EIO : 0);
}

static int goodix_ts_read_input_report(struct goodix_ts_data *ts, u8 *data)
{
	int touch_num;
	int error;

	error = goodix_i2c_read(ts->client, GOODIX_READ_COOR_ADDR, data,
			GOODIX_CONTACT_SIZE + 1);
	if (error) {
		dev_err(&ts->client->dev, "I2C transfer error: %d\n", error);
		return error;
	}

	if (!(data[0] & 0x80))
		return -EAGAIN;

	touch_num = data[0] & 0x0f;
	if (touch_num > ts->max_touch_num)
		return -EPROTO;

	if (touch_num > 1) {
		data += 1 + GOODIX_CONTACT_SIZE;
		error = goodix_i2c_read(ts->client,
				GOODIX_READ_COOR_ADDR +
				1 + GOODIX_CONTACT_SIZE,
				data,
				GOODIX_CONTACT_SIZE * (touch_num - 1));
		if (error)
			return error;
	}

	return touch_num;
}

static void goodix_ts_report_touch(struct goodix_ts_data *ts, u8 *coor_data)
{
	/* int id = coor_data[0] & 0x0F; */
	int input_x = get_unaligned_le16(&coor_data[1]);
	int input_y = get_unaligned_le16(&coor_data[3]);
	int input_w = get_unaligned_le16(&coor_data[5]);

	//DBG_INFO("input_x:%d, input_y:%d, input_w:%d, id:%d.\n", input_x, input_y, input_w, id);

	if (ts->rotated_screen) {
		input_x = ts->abs_x_max - input_x;
		input_y = ts->abs_y_max - input_y;
	} else {

		if (ts->invert_x)
			input_x = ts->abs_x_max - input_x;

		if (ts->invert_y)
			input_y = ts->abs_y_max - input_y;
	}

	if (ts->offset_x)
		input_x += ts->offset_x;

	if (ts->offset_y)
		input_y += ts->offset_y;

#if 0
	input_mt_slot(ts->input_dev, id);
	input_mt_report_slot_state(ts->input_dev, MT_TOOL_FINGER, true);

	if (ts->swap_xy) {
		input_report_abs(ts->input_dev, ABS_MT_POSITION_X, input_y);
		input_report_abs(ts->input_dev, ABS_MT_POSITION_Y, input_x);
	} else {
		input_report_abs(ts->input_dev, ABS_MT_POSITION_X, input_x);
		input_report_abs(ts->input_dev, ABS_MT_POSITION_Y, input_y);
	}
	DBG_INFO("input_x:%d, input_y:%d\n", input_x, input_y);
	input_report_abs(ts->input_dev, ABS_MT_TOUCH_MAJOR, input_w);
	input_report_abs(ts->input_dev, ABS_MT_WIDTH_MAJOR, input_w);
#else
	input_report_abs(ts->input_dev, ABS_MT_TRACKING_ID, 0);
	input_report_abs(ts->input_dev, ABS_MT_TOUCH_MAJOR, input_w);
	input_report_abs(ts->input_dev, ABS_MT_POSITION_X, input_x);
	input_report_abs(ts->input_dev, ABS_MT_POSITION_Y, input_y);
	input_report_abs(ts->input_dev, ABS_MT_WIDTH_MAJOR, 1);
	input_report_key(ts->input_dev, BTN_TOUCH, 1);
	ts->is_down = 1;
	//DBG_INFO("down!\n");
#endif
}

/**
 * goodix_process_events - Process incoming events
 *
 * @ts: our goodix_ts_data pointer
 *
 * Called when the IRQ is triggered. Read the current device state, and push
 * the input events to the user space.
 */
static void goodix_process_events(struct goodix_ts_data *ts)
{
	u8  point_data[1 + GOODIX_CONTACT_SIZE * GOODIX_MAX_CONTACTS];
	int touch_num;
	int i;

	touch_num = goodix_ts_read_input_report(ts, point_data);
	if (touch_num < 0)
		return;

	//DBG_INFO("touch_num:%d\n", touch_num);
	for (i = 0; i < touch_num; i++)
		goodix_ts_report_touch(ts,
				&point_data[1 + GOODIX_CONTACT_SIZE * i]);
#if 0
	input_mt_sync_frame(ts->input_dev);
#else
	if (touch_num == 0 && ts->is_down) {
		DBG_INFO("up!\n");
		input_report_abs(ts->input_dev, ABS_MT_TOUCH_MAJOR, 0);
		input_report_abs(ts->input_dev, ABS_MT_WIDTH_MAJOR, 0);
		input_report_key(ts->input_dev, BTN_TOUCH, 0);
		ts->is_down = 0;
	}
#endif
	input_sync(ts->input_dev);
}

/**
 * goodix_ts_irq_handler - The IRQ handler
 *
 * @irq: interrupt number.
 * @dev_id: private data pointer.
 */
static irqreturn_t goodix_ts_irq_handler(int irq, void *dev_id)
{
	static const u8 end_cmd[] = {
		GOODIX_READ_COOR_ADDR >> 8,
		GOODIX_READ_COOR_ADDR & 0xff,
		0
	};
	struct goodix_ts_data *ts = dev_id;

	goodix_process_events(ts);

	if (i2c_master_send(ts->client, end_cmd, sizeof(end_cmd)) < 0)
		dev_err(&ts->client->dev, "I2C write end_cmd error\n");

	return IRQ_HANDLED;
}

/**
 * goodix_read_config - Read the embedded configuration of the panel
 *
 * @ts: our goodix_ts_data pointer
 *
 * Must be called during probe
 */
static void goodix_read_config(struct goodix_ts_data *ts)
{

	ts->abs_x_max = GOODIX_MAX_WIDTH;
	ts->abs_y_max = GOODIX_MAX_HEIGHT;
	ts->int_trigger_type = GOODIX_INT_TRIGGER;
	ts->max_touch_num = GOODIX_MAX_CONTACTS;

	if (device_property_read_u32(&ts->client->dev, "touchscreen-size-x",
				&ts->abs_x_max) ||
			device_property_read_u32(&ts->client->dev, "touchscreen-size-y",
				&ts->abs_y_max)) {
		dev_err(&ts->client->dev,
				"touchscreen-size-x and/or -y missing, \
				will use config in default\n");
	}

	if (device_property_read_u32(&ts->client->dev,
				"touchscreen-inverted-x", &ts->invert_x))
		ts->invert_x = 0;
	if (device_property_read_u32(&ts->client->dev,
				"touchscreen-offset-y", &ts->invert_y))
		ts->invert_y = 0;

	if (device_property_read_u32(&ts->client->dev,
				"touchscreen-offset-x", &ts->offset_x))
		ts->offset_x = 0;
	if (device_property_read_u32(&ts->client->dev,
				"touchscreen-offset-y", &ts->offset_y))
		ts->offset_y = 0;

	if (device_property_read_u32(&ts->client->dev,
				"touchscreen-swapped-x-y", &ts->swap_xy))
		ts->swap_xy = 0;

	ts->rotated_screen = dmi_check_system(rotated_screen);
	if (ts->rotated_screen)
		dev_dbg(&ts->client->dev,
				"Applying '180 degrees rotated screen' quirk\n");

	DBG_INFO("abs_x_max:%d, abs_y_max:%d, ts->invert_x:%d, ts->invert_y:%d, ts->swap_xy:%d, ts->rotated_screen:%d\n",
			ts->abs_x_max, ts->abs_y_max, ts->invert_x, ts->invert_y, ts->swap_xy, ts->rotated_screen);

#if 0
	ts->invert_x = 0;
	ts->invert_y = 0;
	ts->swap_xy  = 1;
	ts->abs_x_max = 720;
	ts->abs_y_max = 1280;

	DBG_INFO("abs_x_max:%d, abs_y_max:%d, ts->invert_x:%d, ts->invert_y:%d, ts->swap_xy:%d, ts->rotated_screen:%d\n",
			ts->abs_x_max, ts->abs_y_max, ts->invert_x, ts->invert_y, ts->swap_xy, ts->rotated_screen);
#endif
}

/**
 * goodix_read_version - Read goodix touchscreen version
 *
 * @client: the i2c client
 * @version: output buffer containing the version on success
 * @id: output buffer containing the id on success
 */
static int goodix_read_version(struct i2c_client *client, u16 *version, u16 *id)
{
	int error;
	u8 buf[6];
	char id_str[5];

	error = goodix_i2c_read(client, GOODIX_REG_ID, buf, sizeof(buf));
	if (error) {
		dev_err(&client->dev, "read version failed: %d\n", error);
		return error;
	}

	memcpy(id_str, buf, 4);
	id_str[4] = 0;
	if (kstrtou16(id_str, 10, id))
		*id = 0x1001;

	*version = get_unaligned_le16(&buf[4]);

	dev_info(&client->dev, "ID %d, version: %04x\n", *id, *version);

	return 0;
}

/**
 * goodix_i2c_test - I2C test function to check if the device answers.
 *
 * @client: the i2c client
 */

/* static int goodix_i2c_test(struct i2c_client *client)
 * {
 *         int retry = 0;
 *         int error;
 *         u8 test;
 *
 *         while (retry++ < 2) {
 *                 error = goodix_i2c_read(client, GOODIX_REG_CONFIG_DATA,
 *                                 &test, 1);
 *                 if (!error)
 *                         return 0;
 *
 *                 dev_err(&client->dev, "i2c test failed attempt %d: %d\n",
 *                                 retry, error);
 *                 msleep(20);
 *         }
 *
 *         return error;
 * } */

static int goodix_update_firmware(struct goodix_ts_data *ts)
{
	u8 cfg_array[] = CTP_CFG_GROUP1;
	int error = 0;
	/* int index; */
	u8 opr_buf[16];
	int len = sizeof(cfg_array) / sizeof(cfg_array[0]);
	u8 check_sum;
	int i;

	DBG_INFO("cfg_array:size:%d, cfg_array[0]:0x%x, len:%d\n", sizeof(cfg_array), cfg_array[0], len);
	error = goodix_i2c_read(ts->client, GOODIX_REG_CONFIG_DATA, opr_buf, 1);
	DBG_INFO("error:%d, opr_buf:0x%x\n", error, opr_buf[0]);
	if (error) {
		DBG_ERR("read GOODIX_REG_CONFIG_DATA:0x%x fail!!\n", GOODIX_REG_CONFIG_DATA);
		return -1;
	}
	memset(&config[GTP_ADDR_LENGTH], 0, GTP_CONFIG_MAX_LENGTH);
	memcpy(&config[GTP_ADDR_LENGTH], cfg_array, len);

	check_sum = 0;
	for (i = GTP_ADDR_LENGTH; i < len; i++) {
		check_sum += config[i];
	}
	config[len] = (~check_sum) + 1;

	goodix_i2c_write(ts->client, 0, config, GTP_CONFIG_MAX_LENGTH + GTP_ADDR_LENGTH);
#if 0
	cfg_array[0] = opr_buf[0];
	error = goodix_i2c_read(ts->client, RESOLUTION_LOC, opr_buf, 4);
	for (index = 0; index < 4; ++index) {
		DBG_INFO("RESOLUTION_LOC:0x%x, error:%d, opr_buf[%d]:0x%x\n", RESOLUTION_LOC, error, index, opr_buf[index]);
	}
	error = goodix_i2c_read(ts->client, TRIGGER_LOC, opr_buf, 1);
	DBG_INFO("TRIGGER_LOC:0x%x, error:%d, opr_buf:0x%x\n", TRIGGER_LOC, error, opr_buf[0]);

	cfg_array[RESOLUTION_LOC]     = (u8)ts->abs_x_max;
	cfg_array[RESOLUTION_LOC + 1] = (u8)(ts->abs_x_max >> 8);
	cfg_array[RESOLUTION_LOC + 2] = (u8)ts->abs_y_max;
	cfg_array[RESOLUTION_LOC + 3] = (u8)(ts->abs_y_max >> 8);
#endif
	return 0;
}

/**
 * goodix_request_input_dev - Allocate, populate and register the input device
 *
 * @ts: our goodix_ts_data pointer
 * @version: device firmware version
 * @id: device ID
 *
 * Must be called during probe
 */
static int goodix_request_input_dev(struct goodix_ts_data *ts, u16 version,
		u16 id)
{
	int error;

	ts->input_dev = devm_input_allocate_device(&ts->client->dev);
	if (!ts->input_dev) {
		dev_err(&ts->client->dev, "Failed to allocate input device.");
		return -ENOMEM;
	}


	if (ts->swap_xy) {
		input_set_abs_params(ts->input_dev, ABS_MT_POSITION_X,
				0, ts->abs_y_max, 0, 0);
		input_set_abs_params(ts->input_dev, ABS_MT_POSITION_Y,
				0, ts->abs_x_max, 0, 0);
	} else {

		input_set_abs_params(ts->input_dev, ABS_MT_POSITION_X,
				0, ts->abs_x_max, 0, 0);
		input_set_abs_params(ts->input_dev, ABS_MT_POSITION_Y,
				0, ts->abs_y_max, 0, 0);
	}

	input_set_abs_params(ts->input_dev, ABS_MT_WIDTH_MAJOR, 0, 255, 0, 0);
	input_set_abs_params(ts->input_dev, ABS_MT_TOUCH_MAJOR, 0, 255, 0, 0);

#if 0
	input_mt_init_slots(ts->input_dev, ts->max_touch_num,
			INPUT_MT_DIRECT | INPUT_MT_DROP_UNUSED);
#else
	input_set_abs_params(ts->input_dev, ABS_MT_TRACKING_ID, 0,
			(MAX_CONTACTS+1), 0, 0);
	set_bit(EV_ABS, ts->input_dev->evbit);
	set_bit(EV_KEY, ts->input_dev->evbit);
	__set_bit(INPUT_PROP_DIRECT, ts->input_dev->propbit);
	ts->input_dev->keybit[BIT_WORD(BTN_TOUCH)] = BIT_MASK(BTN_TOUCH);

#endif
	ts->input_dev->name = "Goodix Capacitive TouchScreen";
	ts->input_dev->phys = "input/ts";
	ts->input_dev->id.bustype = BUS_I2C;
	ts->input_dev->id.vendor = 0x0416;
	ts->input_dev->id.product = id;
	ts->input_dev->id.version = version;

	error = input_register_device(ts->input_dev);
	if (error) {
		dev_err(&ts->client->dev,
				"Failed to register input device: %d", error);
		return error;
	}

	return 0;
}

static int goodix_ts_probe(struct i2c_client *client,
		const struct i2c_device_id *id)
{
	struct goodix_ts_data *ts;
	struct device *dev = &client->dev;
	unsigned long irq_flags;
	int error;
	u16 version_info, id_info;
	dev_dbg(&client->dev, "I2C Address: 0x%02x\n", client->addr);

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		dev_err(&client->dev, "I2C check functionality failed.\n");
		return -ENXIO;
	}

	ts = devm_kzalloc(&client->dev, sizeof(*ts), GFP_KERNEL);
	if (!ts)
		return -ENOMEM;

	ts->client = client;
	i2c_set_clientdata(client, ts);

	ts->int_gpio = devm_gpiod_get_optional(&client->dev, "int", GPIOD_OUT_LOW);
	ts->reset_gpio = devm_gpiod_get_optional(&client->dev, "reset",
			GPIOD_OUT_LOW);
	if (IS_ERR(ts->reset_gpio)) {
		error = PTR_ERR(ts->reset_gpio);
		if (error != -EPROBE_DEFER)
			dev_err(dev, "error getting reset gpio: %d\n", error);
		return error;
	}

	goodix_reset(ts);

	if (!client->irq && ts->int_gpio) {
		client->irq = gpiod_to_irq(ts->int_gpio);
	}


	if (!client->irq)
		dev_warn(dev, "irq is missing, it will entry poll mode\n");

#if  0
	error = goodix_i2c_test(client);
	if (error) {
		dev_err(&client->dev, "I2C communication failure: %d\n", error);
		return error;
	}
#endif

	error = goodix_read_version(client, &version_info, &id_info);
	if (error) {
		dev_err(&client->dev, "Read version failed.\n");
		return error;
	}
	DBG_INFO("\n");
	goodix_read_config(ts);
	DBG_INFO("\n");
	goodix_update_firmware(ts);
	DBG_INFO("\n");

	error = goodix_request_input_dev(ts, version_info, id_info);
	if (error)
		return error;

	irq_flags = goodix_irq_flags[ts->int_trigger_type] | IRQF_ONESHOT;
	error = devm_request_threaded_irq(&ts->client->dev, client->irq,
			NULL, goodix_ts_irq_handler,
			irq_flags, client->name, ts);
	if (error) {
		dev_err(&client->dev, "request IRQ failed: %d\n", error);
		return error;
	}

	return 0;
}

static int goodix_pm_suspend(struct device *dev)
{
	struct goodix_ts_data *ts =
		(struct goodix_ts_data *)dev_get_drvdata(dev);

	disable_irq(ts->client->irq);

	/*
	 * TODO: add regulator ops.
	 */

	return 0;
}

static int goodix_pm_resume(struct device *dev)
{
	struct goodix_ts_data *ts =
		(struct goodix_ts_data *)dev_get_drvdata(dev);

	enable_irq(ts->client->irq);

	return 0;
}

static const struct i2c_device_id goodix_ts_id[] = {
	{ "GDIX1001:00", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, goodix_ts_id);

#ifdef CONFIG_ACPI
static const struct acpi_device_id goodix_acpi_match[] = {
	{ "GDIX1001", 0 },
	{ "GDIX1002", 0 },
	{ }
};
MODULE_DEVICE_TABLE(acpi, goodix_acpi_match);
#endif

#ifdef CONFIG_OF
static const struct of_device_id goodix_of_match[] = {
	{ .compatible = "goodix,gt911" },
	{ .compatible = "goodix,gt9110" },
	{ .compatible = "goodix,gt912" },
	{ .compatible = "goodix,gt927" },
	{ .compatible = "goodix,gt9271" },
	{ .compatible = "goodix,gt928" },
	{ .compatible = "goodix,gt967" },
	{ }
};
MODULE_DEVICE_TABLE(of, goodix_of_match);
#endif

static const struct dev_pm_ops goodix_pm_ops = {
	.suspend = goodix_pm_suspend,
	.resume = goodix_pm_resume,
};

static struct i2c_driver goodix_ts_driver = {
	.probe = goodix_ts_probe,
	.id_table = goodix_ts_id,
	.driver = {
		.name = "Goodix-TS",
		.acpi_match_table = ACPI_PTR(goodix_acpi_match),
		.of_match_table = of_match_ptr(goodix_of_match),
#if defined(CONFIG_PM)
		.pm = &goodix_pm_ops,
#endif
	},
};
module_i2c_driver(goodix_ts_driver);

MODULE_AUTHOR("Benjamin Tissoires <benjamin.tissoires@gmail.com>");
MODULE_AUTHOR("Bastien Nocera <hadess@hadess.net>");
MODULE_DESCRIPTION("Goodix touchscreen driver");
MODULE_LICENSE("GPL v2");

