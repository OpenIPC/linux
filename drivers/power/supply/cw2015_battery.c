/*
 * CellWise CW2015 battery driver
 *
 * Copyright (C) 2015 Intel Corporation
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/power_supply.h>
#include <linux/regmap.h>
#include <linux/types.h>
#include <linux/gpio/consumer.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/gpio.h>

#include <linux/acpi.h>
#include <linux/of.h>
#include <linux/of_gpio.h>

#ifdef CONFIG_ARCH_SUNXI
#include <linux/sunxi-gpio.h>
#endif

#define CW2015_REG_VERSION		0x00
#define CW2015_REG_VCELL0		0x02
#define CW2015_REG_VCELL1		0x03
#define CW2015_REG_SOC0			0x04
#define CW2015_REG_SOC1			0x05
#define CW2015_REG_RRT_ALRT0		0x06
#define CW2015_REG_RRT_ALRT1		0x07
#define CW2015_REG_CONFIG		0x08
#define CW2015_REG_MODE			0x0A

#define CW2015_MODE_SLEEP		(6)
#define CW2015_MODE_QSTRT		(4)
#define CW2015_MODE_POR			(0)


#define CW2015_MANUFACTURER		"CellWise"

struct cw2015_device {
	struct i2c_client *client;
	struct device *dev;
	struct power_supply *charger;
	struct delayed_work bat_monitor;

	struct regmap *rmap;

	u32 alrt_threshold;
	u32 emerg_threshold;
	bool warning_on;

	struct mutex lock; /* protect state data */
};

static bool cw2015_is_volatile_reg(struct device *dev, unsigned int reg)
{
	switch (reg) {
	case CW2015_REG_VERSION:
	case CW2015_REG_VCELL0:
	case CW2015_REG_VCELL1:
	case CW2015_REG_SOC0:
	case CW2015_REG_SOC1:
		return false;

	default:
		return true;
	}
}

static const struct regmap_config cw2015_regmap_config = {
	.reg_bits = 8,
	.val_bits = 8,

	.max_register = CW2015_REG_MODE,
	.cache_type = REGCACHE_NONE,

	.volatile_reg = cw2015_is_volatile_reg,
};

static irqreturn_t cw2015_irq_handler_thread(int irq, void *private)
{
	int ret;
	struct cw2015_device *cw = private;
	unsigned int val;

	ret = regmap_read(cw->rmap, CW2015_REG_RRT_ALRT0, &val);
	if (val & 0x80)
		regmap_update_bits(cw->rmap, CW2015_REG_RRT_ALRT0, 0x80, 0x00);

	power_supply_changed(cw->charger);

	return IRQ_HANDLED;
}

static ssize_t cw2015_show_alrt_threshold(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	struct power_supply *psy = dev_get_drvdata(dev);
	struct cw2015_device *cw = power_supply_get_drvdata(psy);

	return scnprintf(buf, PAGE_SIZE, "%u\n", cw->alrt_threshold);
}

static DEVICE_ATTR(alrt_threshold, 0444, cw2015_show_alrt_threshold, NULL);

static struct attribute *cw2015_battery_attr[] = {
	&dev_attr_alrt_threshold.attr,
	NULL,
};

static const struct attribute_group cw2015_attr_group = {
	.attrs = cw2015_battery_attr,
};

static unsigned int c2015_get_bat_voltage(struct cw2015_device *cw)
{
	unsigned char vol[2];
	unsigned int voltage;

	regmap_bulk_read(cw->rmap, CW2015_REG_VCELL0, vol, 2);
	/*
	 * 14bit ADC, voltage resolution is 305uV
	 */
	voltage = (((vol[0] & 0x3f) << 8) + vol[1]) * 305 / 1000;
	/*pr_info("h8-0x%x l8-0x%x, voltage=%u\n", vol[0], vol[1], voltage);*/

	return voltage;
}

static unsigned int c2015_get_bat_rest(struct cw2015_device *cw)
{
	unsigned int rest;

	regmap_read(cw->rmap, CW2015_REG_SOC0, &rest);

	return rest;
}

static int cw2015_power_supply_get_property(struct power_supply *psy,
					     enum power_supply_property psp,
					     union power_supply_propval *val)
{
	struct cw2015_device *cw = power_supply_get_drvdata(psy);

	switch (psp) {
	case POWER_SUPPLY_PROP_MANUFACTURER:
		val->strval = CW2015_MANUFACTURER;
		break;

	case POWER_SUPPLY_PROP_MODEL_NAME:
		val->strval = "cw2015";
		break;
	case POWER_SUPPLY_PROP_VOLTAGE_NOW:
		val->intval = c2015_get_bat_voltage(cw);
		break;
	case POWER_SUPPLY_PROP_CAPACITY:
		val->intval = c2015_get_bat_rest(cw);
		break;
	case POWER_SUPPLY_PROP_CAPACITY_ALERT_MAX:
		val->intval = cw->alrt_threshold;
		break;
	case POWER_SUPPLY_PROP_CAPACITY_ALERT_MIN:
		val->intval = cw->emerg_threshold;
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int cw2015_power_supply_property_is_writeable(struct power_supply *psy,
					enum power_supply_property psp)
{
	return false;
}

static enum power_supply_property cw2015_power_supply_props[] = {
	POWER_SUPPLY_PROP_MANUFACTURER,
	POWER_SUPPLY_PROP_MODEL_NAME,
	POWER_SUPPLY_PROP_VOLTAGE_NOW,
	POWER_SUPPLY_PROP_CAPACITY,
	POWER_SUPPLY_PROP_CAPACITY_ALERT_MIN,
	POWER_SUPPLY_PROP_CAPACITY_ALERT_MAX,
};

static const struct power_supply_desc cw2015_power_supply_desc = {
	.name = "battery",
	.type = POWER_SUPPLY_TYPE_BATTERY,
	.properties = cw2015_power_supply_props,
	.num_properties = ARRAY_SIZE(cw2015_power_supply_props),
	.get_property = cw2015_power_supply_get_property,
	.property_is_writeable = cw2015_power_supply_property_is_writeable,
};

static char *cw2015_charger_supplied_to[] = {
	"main-battery",
};

struct power_supply_config cw2015_psy_cfg = {
	.supplied_to = cw2015_charger_supplied_to,
	.num_supplicants = ARRAY_SIZE(cw2015_charger_supplied_to),
};

static int cw2015_power_supply_init(struct cw2015_device *cw)
{
	cw2015_psy_cfg.drv_data = cw,
	cw->charger = devm_power_supply_register(cw->dev,
						 &cw2015_power_supply_desc,
						 &cw2015_psy_cfg);

	return PTR_ERR_OR_ZERO(cw->charger);
}

#ifdef CONFIG_MISC_AXP_LEDS
extern int axp_leds_control(unsigned int idx, unsigned int duty_ns);
static void cw2015_bat_monitor(struct work_struct *work)
{
	struct cw2015_device *cw =
		container_of(work, struct cw2015_device, bat_monitor.work);
	unsigned int rest;
	unsigned int duty_ns;

	power_supply_changed(cw->charger);
	rest = c2015_get_bat_rest(cw);
	if (rest <= cw->emerg_threshold) {
		/* duty:200ms, period:400ms */
		duty_ns = 200000000;
		axp_leds_control(0, duty_ns);
		cw->warning_on = true;
	} else if (rest <= cw->alrt_threshold) {
		/* duty:500ms, period:1000ms */
		duty_ns = 500000000;
		axp_leds_control(0, duty_ns);
		cw->warning_on = true;
	} else if (cw->warning_on) {
		axp_leds_control(0, 0);
		cw->warning_on = false;
	}

	schedule_delayed_work(&cw->bat_monitor, msecs_to_jiffies(10*1000));
}
#else
static void cw2015_bat_monitor(struct work_struct *work)
{
	struct cw2015_device *cw =
		container_of(work, struct cw2015_device, bat_monitor.work);

	power_supply_changed(cw->charger);

	schedule_delayed_work(&cw->bat_monitor, msecs_to_jiffies(10*1000));
}
#endif

static int cw2015_fw_probe(struct cw2015_device *cw)
{
	int ret;
	u32 property;
	struct device_node *np = cw->dev->of_node;
	int gpio;
#ifdef CONFIG_ARCH_SUNXI
	struct gpio_config flags;
#else
	enum of_gpio_flags flags;
#endif

	gpio = of_get_named_gpio_flags(np, "cw,irq-gpio", 0,
					(enum of_gpio_flags *)&flags);
	if (gpio > 0) {
#ifdef CONFIG_ARCH_SUNXI
		char pin_name[32];
		__u32 config;

		/* wakeup_gpio set pull */
		memset(pin_name, 0, sizeof(pin_name));
		sunxi_gpio_to_name(gpio, pin_name);
		config = SUNXI_PINCFG_PACK(SUNXI_PINCFG_TYPE_PUD, 1);
		if (gpio < SUNXI_PL_BASE)
			pin_config_set(SUNXI_PINCTRL, pin_name, config);
		else
			pin_config_set(SUNXI_R_PINCTRL, pin_name, config);
#endif
		cw->client->irq = gpio_to_irq(gpio);
	}

	ret = device_property_read_u32(cw->dev, "cw,alrt-threshold",
				       &property);
	if (ret < 0)
		return ret;
	cw->alrt_threshold = property;

	ret = device_property_read_u32(cw->dev, "cw,emerg-threshold",
				       &property);
	if (ret < 0)
		property = 0;
	cw->emerg_threshold = property;

	return 0;
}

static int cw2015_hw_init(struct cw2015_device *cw)
{
	unsigned int mode, value;

	regmap_read(cw->rmap, CW2015_REG_MODE, &mode);
	mode &= (~(0x3 << CW2015_MODE_SLEEP));
	mode |= (0x3 << CW2015_MODE_QSTRT);
	regmap_write(cw->rmap, CW2015_REG_MODE, mode);

	regmap_read(cw->rmap, CW2015_REG_CONFIG, &value);
	regmap_update_bits(cw->rmap, CW2015_REG_CONFIG,
				0xf8, (cw->alrt_threshold << 3));

	return 0;
}

static int cw2015_probe(struct i2c_client *client,
			 const struct i2c_device_id *id)
{
	struct i2c_adapter *adapter = to_i2c_adapter(client->dev.parent);
	struct device *dev = &client->dev;
	struct cw2015_device *cw;
	int ret = 0;

	if (!i2c_check_functionality(adapter, I2C_FUNC_SMBUS_BYTE_DATA)) {
		dev_err(dev, "No support for SMBUS_BYTE_DATA\n");
		return -ENODEV;
	}

	cw = devm_kzalloc(dev, sizeof(*cw), GFP_KERNEL);
	if (!cw)
		return -ENOMEM;

	cw->client = client;
	cw->dev = dev;

	mutex_init(&cw->lock);

	cw->rmap = devm_regmap_init_i2c(client, &cw2015_regmap_config);
	if (IS_ERR(cw->rmap)) {
		dev_err(dev, "failed to allocate register map\n");
		return PTR_ERR(cw->rmap);
	}

	i2c_set_clientdata(client, cw);

	ret = cw2015_fw_probe(cw);
	if (ret < 0) {
		dev_err(dev, "Cannot read device properties.\n");
		return ret;
	}

	ret = cw2015_hw_init(cw);
	if (ret < 0) {
		dev_err(dev, "hw init failed.\n");
		return ret;
	}

	ret = cw2015_power_supply_init(cw);
	if (ret < 0) {
		dev_err(dev, "Failed to register power supply\n");
		return ret;
	}

	if (client->irq > 0) {
		ret = devm_request_threaded_irq(dev, client->irq, NULL,
						cw2015_irq_handler_thread,
						IRQF_TRIGGER_FALLING |
						IRQF_ONESHOT | IRQF_SHARED,
						"cw2015", cw);
		if (ret) {
			dev_err(dev, "request IRQ #%d failed\n", client->irq);
			return ret;
		}
	}

	INIT_DELAYED_WORK(&cw->bat_monitor, cw2015_bat_monitor);
	schedule_delayed_work(&cw->bat_monitor, msecs_to_jiffies(2 * 1000));

	ret = sysfs_create_group(&cw->charger->dev.kobj, &cw2015_attr_group);
	if (ret < 0) {
		dev_err(dev, "Can't create sysfs entries\n");
		return ret;
	}

	return 0;
}

static int cw2015_remove(struct i2c_client *client)
{
	struct cw2015_device *cw = i2c_get_clientdata(client);


	sysfs_remove_group(&cw->charger->dev.kobj, &cw2015_attr_group);

	return 0;
}

#ifdef CONFIG_PM_SLEEP
static int cw2015_suspend(struct device *dev)
{
	struct cw2015_device *cw = dev_get_drvdata(dev);
	unsigned int mode;

	regmap_read(cw->rmap, CW2015_REG_MODE, &mode);
	mode &= (~(0x3 << CW2015_MODE_QSTRT));
	mode |= (0x3 << CW2015_MODE_SLEEP);
	regmap_write(cw->rmap, CW2015_REG_MODE, mode);

	return 0;
}

static int cw2015_resume(struct device *dev)
{
	struct cw2015_device *cw = dev_get_drvdata(dev);
	unsigned int mode;

	regmap_read(cw->rmap, CW2015_REG_MODE, &mode);
	mode &= (~(0x3 << CW2015_MODE_SLEEP));
	mode |= (0x3 << CW2015_MODE_QSTRT);
	regmap_write(cw->rmap, CW2015_REG_MODE, mode);

	/* signal userspace, maybe state changed while suspended */
	power_supply_changed(cw->charger);

	return 0;
}
#endif

static const struct dev_pm_ops cw2015_pm = {
	SET_SYSTEM_SLEEP_PM_OPS(cw2015_suspend, cw2015_resume)
};

static const struct i2c_device_id cw2015_i2c_ids[] = {
	{ "cw2015", 0 },
	{},
};
MODULE_DEVICE_TABLE(i2c, cw2015_i2c_ids);

static const struct of_device_id cw2015_of_match[] = {
	{ .compatible = "cellwise,cw2015", },
	{ },
};
MODULE_DEVICE_TABLE(of, cw2015_of_match);

static struct i2c_driver cw2015_driver = {
	.driver = {
		.name = "cw2015-battery",
		.of_match_table = of_match_ptr(cw2015_of_match),
		.pm = &cw2015_pm,
	},
	.probe = cw2015_probe,
	.remove = cw2015_remove,
	.id_table = cw2015_i2c_ids,
};
module_i2c_driver(cw2015_driver);

MODULE_AUTHOR("ForeverCai <caiyongheng@allwinnertech.com>");
MODULE_DESCRIPTION("cw2015 charger driver");
MODULE_LICENSE("GPL");
