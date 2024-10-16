/*
 * driver/input/touchscreen/ft_touch.c
 *
 * Copyright(c) 2019 allwinner Technology Corp.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
#include <linux/i2c.h>
#include <linux/input.h>
#include <linux/input/mt.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/ioport.h>
#include <linux/gpio.h>
#include <linux/pm_runtime.h>
#ifdef CONFIG_PM
#include <linux/pm.h>
#endif

#include "../init-input.h"

#define FTS_META_REGS		3
#define FTS_ONE_TCH_LEN		6
#define FTS_MAX_POINTS		16
#define POINT_READ_BUF		(FTS_META_REGS + FTS_ONE_TCH_LEN * FTS_MAX_POINTS)


#define FTS_REG_CHIP_ID		0xA3
#define FTS_REG_FW_VER		0xA6
#define FTS_REG_FW_VENDOR_ID	0xA8
#define FTS_REG_POINT_RATE	0x88


#define FTS_PRESS		0x7F
#define FTS_MAX_ID		0x0F
#define FTS_TOUCH_X_H_POS	3
#define FTS_TOUCH_X_L_POS	4
#define FTS_TOUCH_Y_H_POS	5
#define FTS_TOUCH_Y_L_POS	6
#define FTS_TOUCH_PRE_POS	7
#define FTS_TOUCH_AREA_POS	8
#define FTS_TOUCH_POINT_NUM	2
#define FTS_TOUCH_EVENT_POS	3
#define FTS_TOUCH_ID_POS	5

#define FTS_TOUCH_DOWN		0
#define FTS_TOUCH_UP		1
#define FTS_TOUCH_CONTACT	2

static struct ctp_config_info config_info = {
	.input_type = CTP_TYPE,
	.name = NULL,
	.int_number = 0,
};

struct ts_event {
	u16 au16_x[FTS_MAX_POINTS];	/* x coordinate */
	u16 au16_y[FTS_MAX_POINTS];	/* y coordinate */
	u16 pressure[FTS_MAX_POINTS];
	/* touch event: 0 -- down; 1-- up; 2 -- contact */
	u8 au8_touch_event[FTS_MAX_POINTS];
	u8 au8_finger_id[FTS_MAX_POINTS];	/* touch ID */
	u8 area[FTS_MAX_POINTS];
	u8 touch_point;
	u8 point_num;
};

struct ft6336_ts_data {
	int retry;
	int panel_type;
	uint8_t bad_data;
	char phys[32];
	struct i2c_client *client;
	struct input_dev *input_dev;
	uint8_t use_irq;
	uint8_t use_shutdown;
	uint32_t gpio_shutdown;
	uint32_t gpio_irq;
	uint32_t screen_width;
	uint32_t screen_height;
	bool is_suspended;
	struct ts_event		event;
	int touchs;
	struct hrtimer timer;
	struct work_struct  work;
	int (*power)(struct ft6336_ts_data *ts, int on);
};

static const char *f6x_ts_name = "ft6336";
static struct workqueue_struct *ft6x_wq;

enum {
	DEBUG_INIT = 1U << 0,
	DEBUG_SUSPEND = 1U << 1,
	DEBUG_INT_INFO = 1U << 2,
	DEBUG_X_Y_INFO = 1U << 3,
	DEBUG_KEY_INFO = 1U << 4,
	DEBUG_WAKEUP_INFO = 1U << 5,
	DEBUG_OTHERS_INFO = 1U << 6,
};

#define CTP_IRQ_MODE		(IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING)
#define CTP_NAME		"ft6336"
#define SCREEN_MAX_X		(screen_max_x)
#define SCREEN_MAX_Y		(screen_max_y)
#define PRESS_MAX		(255)

static u32 screen_max_x;
static u32 screen_max_y;
static u32 revert_x_flag;
static u32 revert_y_flag;
static u32 exchange_x_y_flag;

static __u32 twi_id;

/* Addresses to scan */
static const unsigned short normal_i2c[2] = {0x38, I2C_CLIENT_END};
static const int chip_id_value[3] = {0x13, 0x27, 0x28};
static uint8_t read_chip_value[3] = {0x0f, 0x7d, 0};

static struct i2c_client *i2c_connect_client;

static void ft6x_init_events(struct work_struct *work);
static void ft6336_resume_events(struct work_struct *work);
static struct workqueue_struct *ft6x_init_wq;
static struct workqueue_struct *ft6x_resume_wq;
static DECLARE_WORK(ft6x_init_work, ft6x_init_events);
static DECLARE_WORK(ft6336_resume_work, ft6336_resume_events);
static struct ft6336_ts_data *ts_init;

/*
 * Function:
 * Read data from the i2c slave device.
 * Input:
 *	client:i2c device.
 *	buf[0]:operate address.
 *	buf[1]~buf[len]:read data buffer.
 *	len:operate length.
 * Output:
 *	numbers of i2c_msgs to transfer
 */
static int i2c_read_bytes(struct i2c_client *client, uint8_t *buf, uint16_t len)
{

	struct i2c_msg msgs[2];
	int ret = -1;

	msgs[0].flags = !I2C_M_RD;
	msgs[0].addr = client->addr;
	msgs[0].len = 1;
	msgs[0].buf = buf;

	msgs[1].flags = I2C_M_RD;
	msgs[1].addr = client->addr;
	msgs[1].len = len;
	msgs[1].buf = buf;

	ret = i2c_transfer(client->adapter, msgs, 2);

	return ret;
}

/*
 * Function:
 *	write data to the i2c slave device.
 * Input:
 *	client:i2c device.
 *	buf[0]:operate address.
 *	buf[1]~buf[len]:write data buffer.
 *	len:operate length.
 * Output:
 *	numbers of i2c_msgs to transfer.
 */
static int i2c_write_bytes(struct i2c_client *client,
					uint8_t *data, uint16_t len)
{

	struct i2c_msg msg;
	int ret = -1;

	msg.flags = !I2C_M_RD;
	msg.addr = client->addr;
	msg.len = len;
	msg.buf = data;

	ret = i2c_transfer(client->adapter, &msg, 1);

	return ret;
}

/**
 * ctp_detect - Device detection callback for automatic device creation
 * return value:
 *                    = 0; success;
 *                    < 0; err
 */
static int ctp_detect(struct i2c_client *client, struct i2c_board_info *info)
{
	struct i2c_adapter *adapter = client->adapter;
	int  i = 0;

	if (!i2c_check_functionality(adapter, I2C_FUNC_SMBUS_BYTE_DATA)) {
		pr_info("======return=====\n");
		return -ENODEV;
	}
	if (twi_id == adapter->nr) {
		i2c_read_bytes(client, read_chip_value, 3);
		pr_debug("addr:0x%x,chip_id_value:0x%x 0x%x 0x%x\n",
				client->addr, read_chip_value[0],
				read_chip_value[1], read_chip_value[2]);
		while (chip_id_value[i++]) {
			if (read_chip_value[2] == chip_id_value[i - 1]) {
				strlcpy(info->type, CTP_NAME, I2C_NAME_SIZE);
				return 0;
			}
		}
		pr_err("%s:I2C connection might be something wrong !\n",
			__func__);
		return -ENODEV;
	} else
		return -ENODEV;
}

static ssize_t ft6336_enable_show(struct device *dev,
				  struct device_attribute *attr, char *buf)
{
	struct input_dev *input = to_input_dev(dev);
	struct i2c_client *client = input_get_drvdata(input);
	ssize_t size;

	size = sprintf(buf, "%d\n", !pm_runtime_suspended(&client->dev));
	return size;
}

static ssize_t ft6336_enable_store(struct device *dev,
				   struct device_attribute *attr,
				   const char *buf, size_t count)
{
	unsigned long data;
	int error;
	struct input_dev *input = to_input_dev(dev);
	struct i2c_client *client = input_get_drvdata(input);

	error = kstrtoul(buf, 10, &data);
	if (error)
		return error;
	if (data == 0)
		pm_runtime_put_sync(&client->dev);
	else if (data == 1)
		pm_runtime_get_sync(&client->dev);

	return count;
}

static DEVICE_ATTR(enable, 0644,
		   ft6336_enable_show, ft6336_enable_store);

static struct attribute *ft6336_att_als[] = {
	&dev_attr_enable.attr,
	NULL
};

static struct attribute_group ft6336_als_gr = {
	.attrs = ft6336_att_als
};

/**
 * ctp_print_info - sysconfig print function
 * return value:
 *
 */
static void ctp_print_info(struct ctp_config_info info, int debug_level)
{
	if (debug_level == DEBUG_INIT) {
		pr_debug("info.ctp_used:%d\n", info.ctp_used);
		pr_debug("info.twi_id:%d\n", info.twi_id);
		pr_debug("info.screen_max_x:%d\n", info.screen_max_x);
		pr_debug("info.screen_max_y:%d\n", info.screen_max_y);
		pr_debug("info.revert_x_flag:%d\n", info.revert_x_flag);
		pr_debug("info.revert_y_flag:%d\n", info.revert_y_flag);
		pr_debug("info.exchange_x_y_flag:%d\n", info.exchange_x_y_flag);
		pr_debug("info.irq_gpio_number:%d\n", info.irq_gpio.gpio);
		pr_debug("info.wakeup_gpio_number:%d\n", info.wakeup_gpio.gpio);
	}
}

/**
 * ctp_wakeup - function
 */
static int ctp_wakeup(int status, int ms)
{
	pr_debug("***CTP*** %s:status:%d, ms = %d\n", __func__, status, ms);

	if (status == 0) {
		char pin_name[32];
		__u32 config;

		/* wakeup_gpio set output */
		memset(pin_name, 0, sizeof(pin_name));
		sunxi_gpio_to_name(config_info.wakeup_gpio.gpio, pin_name);
		config = SUNXI_PINCFG_PACK(SUNXI_PINCFG_TYPE_FUNC, 1);
		pin_config_set(SUNXI_PINCTRL, pin_name, config);

		/* wakeup_gpio set pull */
		memset(pin_name, 0, sizeof(pin_name));
		sunxi_gpio_to_name(config_info.wakeup_gpio.gpio, pin_name);
		config = SUNXI_PINCFG_PACK(SUNXI_PINCFG_TYPE_PUD, 1);
		pin_config_set(SUNXI_PINCTRL, pin_name, config);

		/* irq_gpio set pull */
		memset(pin_name, 0, sizeof(pin_name));
		sunxi_gpio_to_name(config_info.irq_gpio.gpio, pin_name);
		config = SUNXI_PINCFG_PACK(SUNXI_PINCFG_TYPE_PUD, 1);
		pin_config_set(SUNXI_PINCTRL, pin_name, config);

		if (ms == 0)
			__gpio_set_value(config_info.wakeup_gpio.gpio, 0);
		else {
			__gpio_set_value(config_info.wakeup_gpio.gpio, 0);
			msleep(ms);
			__gpio_set_value(config_info.wakeup_gpio.gpio, 1);
		}
	}
	if (status == 1) {
		if (ms == 0)
			__gpio_set_value(config_info.wakeup_gpio.gpio, 1);
		else {
			__gpio_set_value(config_info.wakeup_gpio.gpio, 1);
			msleep(ms);
			__gpio_set_value(config_info.wakeup_gpio.gpio, 0);
		}
	}
	usleep_range(5000, 6000);

	return 0;
}

/*
 * Function:
 *	GTP initialize function.
 * Input:
 * ts:	i2c client private struct.
 * Output:
 *	Executive outcomes.1---succeed.
 */
static int ft6336_init_panel(struct ft6336_ts_data *ts)
{
	u8 buf[8] = {0};

	buf[0] = FTS_REG_POINT_RATE;
	i2c_write_bytes(ts->client, buf, 2);

	buf[0] = FTS_REG_POINT_RATE;
	i2c_read_bytes(ts->client, buf, 5);
	pr_debug(" ft report rate (addr:0x%x) is %dHz.\n",
				FTS_REG_POINT_RATE, buf[0] * 10);

	memset(buf, 0, sizeof(buf));
	buf[0] = FTS_REG_FW_VER;
	i2c_write_bytes(ts->client, buf, 2);

	buf[0] = FTS_REG_FW_VER;
	i2c_read_bytes(ts->client, buf, 5);
	pr_debug(" ft Firmware version (addr:0x%x) is 0x%x 0x%x 0x%x 0x%x\n",
			FTS_REG_FW_VER, buf[0], buf[1], buf[2], buf[3]);

	memset(buf, 0, sizeof(buf));
	buf[0] = FTS_REG_CHIP_ID;
	i2c_write_bytes(ts->client, buf, 2);

	buf[0] = FTS_REG_CHIP_ID;
	i2c_read_bytes(ts->client, buf, 5);
	pr_debug(" ft chip id is (addr:0x%x) is 0x%x\n",
						FTS_REG_CHIP_ID, buf[0]);

	return 1;
}

static s32 ft6336_ts_version(struct ft6336_ts_data *ts)

{
	u8 buf[8];

	buf[0] = FTS_REG_CHIP_ID;
	buf[1] = 0x2;
	i2c_read_bytes(ts->client, buf, 5);
	pr_debug("FTS_REG_CHIP_ID:%02x %02x %02x %02x %02x\n",
				buf[0], buf[1], buf[2], buf[3], buf[4]);

	return 1;
}

static void ft_ts_work_func(struct work_struct *work)
{
	static u8 buf[POINT_READ_BUF] = {0};
	s32 ret = -1;
	struct ft6336_ts_data *ts = NULL;
	struct ts_event *event = NULL;
	u8 pointid = FTS_MAX_ID;
	int uppoint = 0;
	int touchs = 0;
	int i = 0;

	pr_debug("===enter %s===\n", __func__);
	ts = container_of(work, struct ft6336_ts_data, work);
	event = &ts->event;

	memset(buf, 0, sizeof(buf));
	ret = i2c_read_bytes(ts->client, buf, POINT_READ_BUF);
	if (ret <= 0) {
		pr_err("%s:I2C read error!", __func__);
		goto exit_work_func;
	}

	memset(event, 0, sizeof(struct ts_event));

	event->point_num = buf[FTS_TOUCH_POINT_NUM] & 0x0F;
	event->touch_point = 0;
	for (i = 0; i < FTS_MAX_POINTS; i++) {
		pointid = (buf[FTS_TOUCH_ID_POS + FTS_ONE_TCH_LEN * i]) >> 4;
		if (pointid >= FTS_MAX_ID)
			break;

		event->touch_point++;

		event->au16_x[i] =
			(s16)(buf[FTS_TOUCH_X_H_POS + FTS_ONE_TCH_LEN * i] & 0x0F) <<
			8 | (s16) buf[FTS_TOUCH_X_L_POS + FTS_ONE_TCH_LEN * i];
		event->au16_y[i] =
			(s16)(buf[FTS_TOUCH_Y_H_POS + FTS_ONE_TCH_LEN * i] & 0x0F) <<
			8 | (s16) buf[FTS_TOUCH_Y_L_POS + FTS_ONE_TCH_LEN * i];
		event->au8_touch_event[i] =
			buf[FTS_TOUCH_EVENT_POS + FTS_ONE_TCH_LEN * i] >> 6;
		event->au8_finger_id[i] =
			(buf[FTS_TOUCH_ID_POS + FTS_ONE_TCH_LEN * i]) >> 4;
		event->area[i] =
			(buf[FTS_TOUCH_AREA_POS + FTS_ONE_TCH_LEN * i]) >> 4;
		event->pressure[i] =
			(s16) buf[FTS_TOUCH_PRE_POS + FTS_ONE_TCH_LEN * i];

		if (0 == event->area[i])
			event->area[i] = 0x09;

		if (0 == event->pressure[i])
			event->pressure[i] = 0x3f;

		if ((event->au8_touch_event[i] == FTS_TOUCH_DOWN
				|| event->au8_touch_event[i] == FTS_TOUCH_CONTACT)
			&& (event->point_num == 0)) {
			goto exit_work_func;
		}
	}

	for (i = 0; i < event->touch_point; i++) {
		input_mt_slot(ts->input_dev, event->au8_finger_id[i]);

		if (event->au8_touch_event[i] == FTS_TOUCH_DOWN
			|| event->au8_touch_event[i] == FTS_TOUCH_CONTACT) {
			input_mt_report_slot_state(ts->input_dev,
							MT_TOOL_FINGER, true);
			input_report_abs(ts->input_dev,
					ABS_MT_TOUCH_MAJOR, event->area[i]);
			input_report_abs(ts->input_dev, ABS_MT_TRACKING_ID, i);
			input_report_abs(ts->input_dev,
					ABS_MT_PRESSURE, event->pressure[i]);
			input_report_abs(ts->input_dev,
					ABS_MT_POSITION_X, event->au16_x[i]);
			input_report_abs(ts->input_dev,
					ABS_MT_POSITION_Y, event->au16_y[i]);
			touchs |= BIT(event->au8_finger_id[i]);
			ts->touchs |= BIT(event->au8_finger_id[i]);
		} else {
			uppoint++;
			input_mt_report_slot_state(ts->input_dev,
							MT_TOOL_FINGER, false);
			ts->touchs &= ~BIT(event->au8_finger_id[i]);
		}
	}

	if (unlikely(ts->touchs ^ touchs)) {
		for (i = 0; i < FTS_MAX_POINTS; i++) {
			if (BIT(i) & (ts->touchs ^ touchs)) {
				uppoint++;
				input_mt_slot(ts->input_dev, i);
				input_mt_report_slot_state(ts->input_dev,
							MT_TOOL_FINGER, false);
			}
		}
	}
	ts->touchs = touchs;

	if (event->touch_point == uppoint || event->point_num == 0)
		input_report_key(ts->input_dev, BTN_TOUCH, 0);
	else
		input_report_key(ts->input_dev,
					BTN_TOUCH, event->touch_point > 0);

	input_sync(ts->input_dev);

exit_work_func:
	enable_irq(gpio_to_irq(config_info.int_number));
	return;
}

static irqreturn_t ft6336_ts_irq_hanbler(int irq, void *dev_id)
{
	struct ft6336_ts_data *ts = (struct ft6336_ts_data *)dev_id;

	disable_irq_nosync(gpio_to_irq(config_info.int_number));

	pr_debug("==========------TS Interrupt-----============\n");
	queue_work(ft6x_wq, &ts->work);

	return IRQ_HANDLED;
}

static int ft6336_ts_power(struct ft6336_ts_data *ts, int on)
{
	s32 ret = -1;

	switch (on) {
	case 0:
		ret = 1;
		break;
	case 1:
		ctp_wakeup(0, 2);
		usleep_range(10000, 11000);
		ret = ft6336_init_panel(ts);
		if (ret != 1) {
			pr_err("init panel fail!\n");
			return -1;
		}
		usleep_range(10000, 11000);
		ret = 1;
		break;
	default:
		pr_err("%s: Cant't support this command.", f6x_ts_name);
		ret = -EINVAL;
		break;
	}

	return ret;
}

static void ft6336_resume_events(struct work_struct *work)
{
	int ret;

	if (ts_init->is_suspended == false) {
		ctp_wakeup(0, 2);

		usleep_range(10000, 11000);
	} else if (ts_init->power) {
		ret = ts_init->power(ts_init, 1);
		if (ret < 0)
			pr_err("%s power on failed\n", f6x_ts_name);
	}
	ret = input_set_int_enable(&(config_info.input_type), 1);
	ts_init->is_suspended = false;
	if (ret < 0)
		pr_err("%s irq enable failed\n", f6x_ts_name);
}

static int ft6336_ts_suspend(struct device *dev)
{
	int ret;
	struct ft6336_ts_data *ts = dev_get_drvdata(dev);

	if (pm_runtime_suspended(dev))
		return 0;

	if (ts->is_suspended == false) {
		flush_workqueue(ft6x_resume_wq);
		ret = input_set_int_enable(&(config_info.input_type), 0);
		if (ret < 0)
			pr_err("%s irq disable failed\n", f6x_ts_name);

		ret = cancel_work_sync(&ts->work);
		flush_workqueue(ft6x_wq);
		if (ts->power) {
			ret = ts->power(ts, 0);
			if (ret < 0)
				dev_err(dev, " power off failed\n");
		}
		ts->is_suspended = true;
	}
	return 0;
}

static int ft6336_ts_resume(struct device *dev)
{
	struct ft6336_ts_data *ts = dev_get_drvdata(dev);

	if (pm_runtime_suspended(dev))
		return 0;

	if (ts->is_suspended == true)
		queue_work(ft6x_resume_wq, &ft6336_resume_work);

	return 0;
}

static void ft6x_init_events(struct work_struct *work)
{
	int ret = 0;

	ret = ft6336_init_panel(ts_init);
	if (!ret)
		pr_err("init panel fail!\n");
	else
		pr_debug("init panel succeed!\n");

	config_info.dev = &(ts_init->input_dev->dev);
	ret = input_request_int(&(config_info.input_type),
				ft6336_ts_irq_hanbler, CTP_IRQ_MODE, ts_init);
	if (ret)
		pr_info("ft6336_probe: request irq failed\n");
}

static int ft6336_ts_probe(struct i2c_client *client,
			   const struct i2c_device_id *id)
{
	struct ft6336_ts_data *ts;
	int ret = 0;

	pr_debug("=============FT6336 Probe==================\n");

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		dev_err(&client->dev, "System need I2C function.\n");
		ret = -ENODEV;
		goto err_check_functionality_failed;
	}

	ts = kzalloc(sizeof(*ts), GFP_KERNEL);
	if (ts == NULL) {
		ret = -ENOMEM;
		goto err_alloc_data_failed;
	}

	i2c_connect_client = client;	 /* used by Guitar Updating */

	INIT_WORK(&ts->work, ft_ts_work_func);
	ts->client = client;
	i2c_set_clientdata(ts->client, ts);

	ts->input_dev = input_allocate_device();
	if (ts->input_dev == NULL) {
		ret = -ENOMEM;
		dev_dbg(&client->dev, "Failed to allocate input device\n");
		goto err_input_dev_alloc_failed;
	}

	ts->input_dev->evbit[0] = BIT_MASK(EV_KEY) | BIT_MASK(EV_ABS);
	set_bit(BTN_TOUCH, ts->input_dev->keybit);
	set_bit(INPUT_PROP_DIRECT, ts->input_dev->propbit);

	ts->input_dev->absbit[0] = BIT_MASK(ABS_MT_TRACKING_ID) |
					BIT_MASK(ABS_MT_TOUCH_MAJOR) |
					BIT_MASK(ABS_MT_WIDTH_MAJOR) |
					BIT_MASK(ABS_MT_POSITION_X) |
					BIT_MASK(ABS_MT_POSITION_Y);
	input_set_abs_params(ts->input_dev, ABS_MT_WIDTH_MAJOR, 0, 0x0f, 0, 0);
	input_set_abs_params(ts->input_dev, ABS_MT_TOUCH_MAJOR, 0, 0x0f, 0, 0);
	input_set_abs_params(ts->input_dev, ABS_MT_PRESSURE, 0, 255, 0, 0);
	input_set_abs_params(ts->input_dev, ABS_MT_POSITION_X, 0, SCREEN_MAX_X,
			     0, 0);
	input_set_abs_params(ts->input_dev, ABS_MT_POSITION_Y, 0, SCREEN_MAX_Y,
			     0, 0);
	input_set_abs_params(ts->input_dev, ABS_MT_TRACKING_ID, 0,
			     FTS_MAX_POINTS, 0, 0);

	sprintf(ts->phys, "input/ft6336-ts");
	ts->input_dev->name = CTP_NAME;
	ts->input_dev->phys = ts->phys;
	ts->input_dev->id.bustype = BUS_I2C;
	ts->input_dev->id.vendor = 0xDEAD;
	ts->input_dev->id.product = 0xBEEF;
	ts->input_dev->id.version = 0x1105;

	ts->is_suspended = false;

	ret = input_register_device(ts->input_dev);
	if (ret) {
		dev_err(&client->dev, "Unable to register %s input device\n",
			ts->input_dev->name);
		goto err_input_register_device_failed;
	}
	input_set_drvdata(ts->input_dev, client);
	ret = sysfs_create_group(&ts->input_dev->dev.kobj,
				 &ft6336_als_gr);
	if (ret < 0) {
		dev_err(&client->dev, "ft6336: sysfs_create_group err\n");
		goto err_input_register_device_failed;
	}

	ft6x_wq = create_singlethread_workqueue("ft_wq");
	if (!ft6x_wq) {
		pr_err("Creat %s workqueue failed.\n", f6x_ts_name);
		return -ENOMEM;
	}
	flush_workqueue(ft6x_wq);
	ts->power = ft6336_ts_power;

	ts_init = ts;

	ft6x_init_wq = create_singlethread_workqueue("ft6x_init");
	if (ft6x_init_wq == NULL) {
		pr_err("create ft6x_wq fail!\n");
		return -ENOMEM;
	}

	ft6x_resume_wq = create_singlethread_workqueue("ft6336_resume");
	if (ft6x_resume_wq == NULL) {
		pr_err("create ft6x_resume_wq fail!\n");
		return -ENOMEM;
	}

	queue_work(ft6x_init_wq, &ft6x_init_work);

	ft6336_ts_version(ts);

	pm_runtime_set_active(&client->dev);
	pm_runtime_get(&client->dev);
	pm_runtime_enable(&client->dev);

	pr_debug("========Probe Ok================\n");

	return 0;

err_input_register_device_failed:
	input_free_device(ts->input_dev);
err_input_dev_alloc_failed:
	i2c_set_clientdata(client, NULL);
err_alloc_data_failed:
err_check_functionality_failed:
	return ret;
}

static int ft6336_ts_remove(struct i2c_client *client)
{
	struct ft6336_ts_data *ts = i2c_get_clientdata(client);

	dev_notice(&client->dev, "The driver is removing...\n");

	input_free_int(&(config_info.input_type), ts_init);

	cancel_work_sync(&ft6x_init_work);
	cancel_work_sync(&ft6336_resume_work);
	flush_workqueue(ft6x_wq);
	if (ft6x_resume_wq)
		destroy_workqueue(ft6x_resume_wq);
	if (ft6x_init_wq)
		destroy_workqueue(ft6x_init_wq);
	if (ft6x_wq)
		destroy_workqueue(ft6x_wq);
	pm_runtime_disable(&client->dev);
	pm_runtime_set_suspended(&client->dev);
	sysfs_remove_group(&ts->input_dev->dev.kobj, &ft6336_als_gr);
	input_unregister_device(ts->input_dev);
	i2c_set_clientdata(ts->client, NULL);
	kfree(ts);

	return 0;
}

static const struct i2c_device_id ft6336_ts_id[] = {
	{CTP_NAME, 0},
	{},
};
MODULE_DEVICE_TABLE(i2c, ft6336_ts_id);

static const struct dev_pm_ops ft6336_pm_ops = {
	.suspend = ft6336_ts_suspend,
	.resume = ft6336_ts_resume,
};

#define FT6336_PM_OPS (&ft6336_pm_ops)

static struct i2c_driver ft6336_ts_driver = {
	.class = I2C_CLASS_HWMON,
	.probe		= ft6336_ts_probe,
	.remove		= ft6336_ts_remove,
	.id_table	= ft6336_ts_id,
	.driver = {
		.name	= CTP_NAME,
		.owner = THIS_MODULE,
		.pm = FT6336_PM_OPS,
	},
	.detect         = ctp_detect,
	.address_list	= normal_i2c,
};

static int ctp_get_system_config(void)
{
	ctp_print_info(config_info, DEBUG_INIT);

	twi_id = config_info.twi_id;
	screen_max_x = config_info.screen_max_x;
	screen_max_y = config_info.screen_max_y;
	revert_x_flag = config_info.revert_x_flag;
	revert_y_flag = config_info.revert_y_flag;
	exchange_x_y_flag = config_info.exchange_x_y_flag;

	if ((screen_max_x == 0) || (screen_max_y == 0)) {
		pr_err("%s:read config error!\n", __func__);
		return 0;
	}

	return 1;
}
static int __init ft_ts_init(void)
{
	int ret = -1;

	pr_debug("*********************************************\n");
	if (!input_sensor_startup(&(config_info.input_type))) {
		ret = input_sensor_init(&(config_info.input_type));
		if (ret != 0) {
			pr_err("%s:ctp_ops.input_sensor_init err.\n", __func__);
			goto init_err;
		}
		input_set_power_enable(&(config_info.input_type), 1);
	} else {
		pr_err("%s: input_ctp_startup err.\n", __func__);
		return 0;
	}
	if (config_info.ctp_used == 0) {
		pr_info("*** ctp_used set to 0 !\n");
		pr_info("*** if use ctp, please put the sys_config.fex ctp_used set to 1.\n");
		ret = 0;
		goto init_err;
	}
	if (!ctp_get_system_config()) {
		pr_err("%s:read config fail!\n", __func__);
		goto init_err;
	}
	ctp_wakeup(0, 2);

	ret = i2c_add_driver(&ft6336_ts_driver);
	if (ret)
		goto init_err;

	pr_debug("*********************************************\n");
	return ret;

init_err:
	input_set_power_enable(&(config_info.input_type), 0);
	return ret;
}

static void __exit ft_ts_exit(void)
{
	i2c_del_driver(&ft6336_ts_driver);
	input_sensor_free(&(config_info.input_type));
	input_set_power_enable(&(config_info.input_type), 0);
}

late_initcall(ft_ts_init);
module_exit(ft_ts_exit);
MODULE_DESCRIPTION("ft Touchscreen Driver");
MODULE_LICENSE("GPL v2");
