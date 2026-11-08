#ifndef __ak_sensor_common_h__
#define __ak_sensor_common_h__

#include <linux/init.h>
#include <linux/module.h>
#include <mach-anyka/ispdrv_interface.h>

#define ak_sensor_print printk

struct ak_sensor_i2c_data {
	unsigned char u8DevAddr;
	unsigned int u32RegAddr;
	unsigned int u32RegAddrByteNum;
	unsigned int u32Data;
	unsigned int u32DataByteNum;
	unsigned int reserved[2];
};

struct ak_sensor_parameter {
    u32 sensor_ucode;
    unsigned int sensor_id;
    unsigned int sensor_mclk;
    unsigned int sensor_io_interface;
    unsigned int mipi_mbps;
    unsigned int mipi_lane;
    unsigned int pclk_hz;
};

int ak_sensor_read_register(struct ak_sensor_i2c_data *p_i2c_data);
int ak_sensor_write_register(const struct ak_sensor_i2c_data *p_i2c_data);

int ak_sensor_set_pin_as_gpio(const int pin);
int ak_sensor_set_pin_dir(const int pin, const int is_output);
int ak_sensor_set_pin_level(const int pin, const int level);

int ak_sensor_mdelay(const int msec);

#define AK_SENSOR_MODULE(callback, name) \
static int name##_module_init(void) \
{ \
	return ispdrv_register_sensor(&callback); \
} \
static void name##_module_exit(void) \
{ \
} \
module_init(name##_module_init); \
module_exit(name##_module_exit); \
MODULE_LICENSE("GPL"); \
MODULE_AUTHOR("ye_guohong <ye_guohong@anyka.oa>"); \
MODULE_DESCRIPTION("Driver for Sensor Interface");

/*
 * As AK_SENSOR_MODULE(), and registers @name ("sc1345") as the sensor's
 * OpenIPC name: the ISP driver loads /etc/sensor/<name>.conf for it.
 */
#define AK_SENSOR_MODULE_NAMED(callback, name) \
static int name##_module_init(void) \
{ \
	int ret = ispdrv_register_sensor_name(&callback, #name); \
	return ret ? ret : ispdrv_register_sensor(&callback); \
} \
static void name##_module_exit(void) \
{ \
} \
module_init(name##_module_init); \
module_exit(name##_module_exit); \
MODULE_LICENSE("GPL");

#endif
