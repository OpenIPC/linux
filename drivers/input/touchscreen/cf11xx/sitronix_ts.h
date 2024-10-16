/*
 * drivers/input/touchscreen/sitronix_i2c_touch.h
 *
 * Touchscreen driver for Sitronix
 *
 * Copyright (C) 2011 Sitronix Technology Co., Ltd.
 *	Rudy Huang <rudy_huang@sitronix.com.tw>
 */
/*
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 */

#ifndef __SITRONIX_I2C_TOUCH_h
#define __SITRONIX_I2C_TOUCH_h

#include <linux/kernel.h>
#include <linux/hrtimer.h>
#include <linux/i2c.h>
#include <linux/input.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/i2c.h>
#include <linux/proc_fs.h>
#include <linux/string.h>
#include <asm/uaccess.h>
#include <linux/vmalloc.h>
#include <linux/interrupt.h>
#include <linux/io.h>

#ifdef CONFIG_HAS_EARLYSUSPEND
#include <linux/earlysuspend.h>
#endif

#include <linux/device.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/errno.h>
#include <linux/platform_device.h>
#include <linux/async.h>
#include <linux/ioport.h>
#include <asm/irq.h>
#include <asm/delay.h>
#include <linux/irq.h>
#include <linux/gpio.h>

#include <linux/ioctl.h> /* needed for the _IOW etc stuff used later */
#include <linux/cdev.h>

#include "sitronix_ts_custom_func.h"

#define ST_UPGRADE_FIRMWARE
#define ST_DEVICE_NODE
#define STX_POWER_SUPPLY_EN

/*******************************************************************************
* MT SLOT feature is implmented in linux kernel 2.6.38 and later.\
  Make sure that version of your linux kernel before using this feature.
*******************************************************************************/
#define SITRONIX_SUPPORT_MT_SLOT

#ifdef SITRONIX_SUPPORT_MT_SLOT
#include <linux/input/mt.h>
#endif // SITRONIX_SUPPORT_MT_SLOT

//#define SITRONIX_I2C_COMBINED_MESSAGE
#ifndef SITRONIX_I2C_COMBINED_MESSAGE
#define SITRONIX_I2C_SINGLE_MESSAGE
#endif

#define st_u8 u8
#define st_char char
#define st_msleep msleep
#define st_usleep udelay
#define st_int int
#define st_u16 u16
#define st_u32 u32

//#define ST_DEBUG_EN

#ifdef ST_DEBUG_EN
#define STX_DEBUG(fmt, args...) printk("[SITRONIX]" fmt "\n", ##args)
#define STX_FUNC_ENTER() printk("[SITRONIX]%s: Enter\n", __func__)
#define STX_FUNC_EXIT() printk("[SITRONIX]%s: Exit(%d)\n", __func__, __LINE__)
#else /* #if ST_DEBUG_EN*/
#define STX_DEBUG(fmt, args...)
#define STX_FUNC_ENTER()
#define STX_FUNC_EXIT()
#endif

#define STX_INFO(fmt, args...) printk("[SITRONIX][Info]" fmt "\n", ##args)
#define STX_ERROR(fmt, args...) printk("[SITRONIX][Error]" fmt "\n", ##args)

struct sitronix_ts_device_info
{
	uint8_t chip_id;
	uint8_t fw_version[2];
	uint8_t fw_revision[32];
	uint8_t max_touches;
	uint16_t x_res;
	uint16_t y_res;
	uint8_t rx_chs;
	uint8_t tx_chs;
	uint8_t Num_X;
	uint8_t Num_Y;
}; //hfst 1

struct sitronix_ts_platform_data
{
	int irq_gpio;
	u32 irq_gpio_flags;
	int reset_gpio;
	u32 reset_gpio_flags;
	char *name;
	u32 button_map[4];
	u8 num_button;
};

struct sitronix_ts_data
{
	struct i2c_client *client;
	struct input_dev *input_dev;
	struct input_dev *keyevent_input;
	struct sitronix_ts_device_info ts_dev_info;
	struct sitronix_ts_platform_data *host_if;
	struct work_struct work;
#if defined(CONFIG_FB)
	struct notifier_block fb_notif;
#elif defined(CONFIG_HAS_EARLYSUSPEND)
	struct early_suspend early_suspend;
#endif
	int suspend_state;
	spinlock_t irq_lock;
	int irq_is_enable;
	int irq_flags;
	struct regulator *vdd_ana;
	struct regulator *vcc_i2c;
	struct mutex dev_mutex;
	int rawTestResult;
	bool glove_mode;
	bool cases_mode;
	bool is_upgrading;
	bool is_testing;
	int fsmart_wakeup;
	int irq_registered;
    int fapp_in;
};

typedef enum
{
	FIRMWARE_VERSION,
	STATUS_REG,
	DEVICE_CONTROL_REG,
	TIMEOUT_TO_IDLE_REG,
	XY_RESOLUTION_HIGH,
	X_RESOLUTION_LOW,
	Y_RESOLUTION_LOW,
	DEVICE_CONTROL_REG2 = 0x09,
	FIRMWARE_REVISION_3 = 0x0C,
	FIRMWARE_REVISION_2,
	FIRMWARE_REVISION_1,
	FIRMWARE_REVISION_0,
	FINGERS,
	KEYS_REG,
	XY0_COORD_H,
	X0_COORD_L,
	Y0_COORD_L,
	I2C_PROTOCOL = 0x3E,
	MAX_NUM_TOUCHES,
	DATA_0_HIGH,
	DATA_0_LOW,
	MISC_Info = 0xF0,
	MISC_CONTROL = 0xF1,
	SMART_WAKE_UP_REG = 0xF2,
	CHIP_ID = 0xF4,
	XY_CHS = 0xF5,
	PAGE_REG = 0xff,
} RegisterOffset;

typedef enum
{
	XY_COORD_H,
	X_COORD_L,
	Y_COORD_L,
	PIXEL_DATA_LENGTH_B,
	PIXEL_DATA_LENGTH_A,
} PIXEL_DATA_FORMAT;

#define X_RES_H_SHFT 4
#define X_RES_H_BMSK 0xf
#define Y_RES_H_SHFT 0
#define Y_RES_H_BMSK 0xf
//#define FINGERS_SHFT 0
//#define FINGERS_BMSK 0xf
//#define X_COORD_VALID_SHFT 7
//#define X_COORD_VALID_BMSK 0x1
//#define X_COORD_H_SHFT 4
//#define X_COORD_H_BMSK 0x7
//#define Y_COORD_H_SHFT 0
//#define Y_COORD_H_BMSK 0x7

struct sitronix_sensor_key_t
{
	unsigned int code;
};

extern struct sitronix_ts_data stx_gpts;

/*st_rawtest*/
extern int st_testraw_invoke(void);

/*st_gesture*/
extern int sitronix_swk_disable(void);
extern int sitronix_swk_enable(void);
extern int sitronix_swk_func(struct input_dev *dev);
extern bool sitronix_cases_mode_check(int x, int y);
extern int st_gesture_init(void);

/*st_monitor*/
extern int sitronix_monitor_start(void);
extern int sitronix_monitor_stop(void);
extern int sitronix_monitor_delay(void);

/*st_sysfs*/
extern int st_create_sysfs(struct i2c_client *client);
extern int st_remove_sysfs(struct i2c_client *client);
extern int st_dev_node_init(void);
extern void st_dev_node_exit(void);

/*st_upgrade_fw*/
extern int st_upgrade_fw(void);
extern int st_upgrade_fw_handler(void *unused);
extern int stx_force_upgrade_fw(char *path); ///////hfst
extern int stx_clr_crash_msg(void);

/*sitronix_ts*/
extern void st_reset_ic(void);
extern void stx_irq_enable(void);
extern void stx_irq_disable(void);

/*st_utility*/
extern void st_power_down(struct sitronix_ts_data *ts);
extern void st_power_up(struct sitronix_ts_data *ts);
extern int st_get_status(struct sitronix_ts_data *ts);
extern int st_print_version(struct sitronix_ts_data *ts);
extern int st_enter_glove_mode(struct sitronix_ts_data *ts);
extern int st_leave_glove_mode(struct sitronix_ts_data *ts);
extern int st_get_dev_status(struct i2c_client *client, uint8_t *err_code, uint8_t *dev_status);
extern int st_get_touch_info(struct sitronix_ts_data *ts);
extern int st_irq_off(void);
extern int st_irq_on(void);

/*st_i2c*/
extern s32 stx_i2c_write(struct i2c_client *client, u8 *buf, int len);
extern int stx_i2c_read(struct i2c_client *client, u8 *buffer, int len, u8 addr);
extern int stx_i2c_read_direct(st_u8 *rxbuf, int len);
extern int stx_i2c_read_bytes(st_u8 addr, st_u8 *rxbuf, int len);
extern int stx_i2c_write_bytes(st_u8 *txbuf, int len);
extern int stx_cmdio_read(int type, int address, unsigned char *buf, int len);
extern int stx_cmdio_write(int type, int address, unsigned char *buf, int len);

#endif // __SITRONIX_I2C_TOUCH_h
