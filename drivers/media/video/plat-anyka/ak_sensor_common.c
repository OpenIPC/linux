#include <plat-anyka/ak_sensor.h>
#include <plat-anyka/ak_sensor_i2c.h>
#include <mach/gpio.h>
#include <linux/delay.h>
#include "ak_sensor_common.h"

int ak_sensor_read_register(struct ak_sensor_i2c_data *p_i2c_data)
{
	return sensor_read_register((T_SENSOR_I2C_DATA_S *)p_i2c_data);
}
EXPORT_SYMBOL_GPL(ak_sensor_read_register);

int ak_sensor_write_register(const struct ak_sensor_i2c_data *p_i2c_data)
{
	return sensor_write_register((T_SENSOR_I2C_DATA_S *)p_i2c_data);
}
EXPORT_SYMBOL_GPL(ak_sensor_write_register);

int ak_sensor_set_pin_as_gpio(const int pin)
{
	unsigned int tmp;

	tmp = (unsigned int)pin;
	return ak_setpin_as_gpio(tmp);
}
EXPORT_SYMBOL_GPL(ak_sensor_set_pin_as_gpio);

int ak_sensor_set_pin_dir(const int pin, const int is_output)
{
	unsigned int tmp;

	tmp = (unsigned int)pin;
	if (is_output)
		return ak_gpio_cfgpin(tmp, AK_GPIO_DIR_OUTPUT);
	else
		return ak_gpio_cfgpin(tmp, AK_GPIO_DIR_INPUT);
}
EXPORT_SYMBOL_GPL(ak_sensor_set_pin_dir);

int ak_sensor_set_pin_level(const int pin, const int level)
{
	unsigned int tmp;

	tmp = (unsigned int)pin;
	return ak_gpio_setpin(tmp, level ? 1 : 0);    
}
EXPORT_SYMBOL_GPL(ak_sensor_set_pin_level);

int ak_sensor_set_pin_pull(const int pin, const int is_pullup, const int is_enable)
{
	unsigned int tmp;

	tmp = (unsigned int)pin;
	if (is_pullup)
		return ak_gpio_pullup(tmp, is_enable);
	else
		return ak_gpio_pulldown(tmp, is_enable);
}
EXPORT_SYMBOL_GPL(ak_sensor_set_pin_pull);

int ak_sensor_mdelay(const int msec)
{
	mdelay(msec);
	return 0;
}
EXPORT_SYMBOL_GPL(ak_sensor_mdelay);
