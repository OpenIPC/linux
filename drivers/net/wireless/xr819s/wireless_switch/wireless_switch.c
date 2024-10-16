/*
 * XR819S wireless switch
 *
 * Copyright (c) 2013
 * Xradio Technology Co., Ltd. <www.xradiotech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/sched.h>
#include <net/sock.h>
#include <linux/proc_fs.h>
#include <linux/gpio.h>
#include <linux/sunxi-gpio.h>
#include <linux/of_gpio.h>
#include <linux/platform_device.h>
#include <linux/irq.h>
#include <linux/param.h>
#include <linux/rfkill.h>
struct sunxi_wireless_platdata {
	unsigned int wk_pin;
	struct rfkill *rfkill;
	struct platform_device *pdev;
};


MODULE_LICENSE("GPL");
MODULE_AUTHOR("ANC");
MODULE_DESCRIPTION("wireless-switch");

#define WIRELESS_MODULE_VERSION "1.0"

#define MAX_WRITE_COUNT 1

#define BT_STATE_OFF      false
#define BT_STATE_ON       true
#define WLAN_STATE_OFF    false
#define WLAN_STATE_ON     true

#define INPUT_OFFSET    '0'

#define WIRELESS_SWITCH_ERROR(fmt, args...) printk(KERN_ERR"BT SWITCH DEBUG:" fmt, ##args)
#define WIRELESS_SWITCH_DEBUG(fmt, args...) printk(KERN_DEBUG"BT SWITCH DEBUG:" fmt, ##args)

extern void sunxi_bluetooth_set_power(bool on_off);
extern void sunxi_wlan_set_power(bool on);

static bool bt_state;
static bool wlan_state;
static struct sunxi_wireless_platdata *wireless;

void wlan_reg_enable(void)
{
	sunxi_bluetooth_set_power(true);
}
EXPORT_SYMBOL_GPL(wlan_reg_enable);

void wlan_set_reset_pin(bool state)
{
	sunxi_wlan_set_power(state);

	if (state == WLAN_STATE_OFF && bt_state == BT_STATE_OFF)
		sunxi_bluetooth_set_power(state);

	wlan_state = state;
}
EXPORT_SYMBOL_GPL(wlan_set_reset_pin);

static void bt_set_reset_pin(bool state)
{
	sunxi_bluetooth_set_power(state);
}

static void bt_set_wakeup_pin(bool state)
{
	gpio_set_value(wireless->wk_pin, state);
	WIRELESS_SWITCH_DEBUG("%s state:%d\n", __func__, state);
}

static void bt_set_state(bool state)
{
	WIRELESS_SWITCH_DEBUG("%s\n", __func__);

	if (state == BT_STATE_ON) {
		if (wlan_state == WLAN_STATE_OFF) {
			bt_set_reset_pin(state);
			bt_set_wakeup_pin(state);
			WIRELESS_SWITCH_DEBUG("wlan off, bt would be turn on\n");
		} else {
			bt_set_wakeup_pin(!state);
			udelay(200);
			bt_set_wakeup_pin(state);
			WIRELESS_SWITCH_DEBUG("wlan on, bt would be turn on\n");
		}
	} else if (state == BT_STATE_OFF) {
		if (wlan_state == WLAN_STATE_OFF) {
			bt_set_reset_pin(state);
			bt_set_wakeup_pin(state);
			WIRELESS_SWITCH_DEBUG("wlan off, bt would be turn off\n");
		}
	}

	bt_state = state;
}

static int sunxi_wireless_set_block(void *data, bool blocked)
{
	blocked = !blocked;

	WIRELESS_SWITCH_DEBUG("blocked is %d\n", blocked);
	if (blocked == BT_STATE_ON) {
		bt_set_state(blocked);
		WIRELESS_SWITCH_DEBUG("bt would turn on, bt_state %d\n", blocked);
	} else if (blocked == BT_STATE_OFF) {
		bt_set_state(blocked);
		WIRELESS_SWITCH_DEBUG("bt would turn off, bt_state %d\n", blocked);
	} else {
		WIRELESS_SWITCH_DEBUG("error state, bt_state %d\n", blocked);
	}
	return 0;
}

static const struct rfkill_ops sunxi_wireless_rfkill_ops = {
	.set_block = sunxi_wireless_set_block,
};

static int sw_rfkill_reg(void)
{
	int ret = -1;
	struct device *dev = &wireless->pdev->dev;

	wireless->rfkill = rfkill_alloc("sunxi-wireless", dev, RFKILL_TYPE_BLUETOOTH,
				    &sunxi_wireless_rfkill_ops, wireless);
	if (wireless->rfkill == NULL) {
		WIRELESS_SWITCH_ERROR("rfkill alloc failed\n");
		return ret;
	}

	rfkill_set_states(wireless->rfkill, true, false);

	ret = rfkill_register(wireless->rfkill);

	return ret;
}

static void sw_rfkill_dereg(void)
{
	rfkill_destroy(wireless->rfkill);
	wireless->rfkill = NULL;
}

static int sw_platform_init(void)
{
	struct device_node *np = NULL;
	struct gpio_config config;

	//kzalloc memory for wireless data struct
	wireless = kzalloc(sizeof(*wireless), GFP_KERNEL);
	if (wireless == NULL) {
		WIRELESS_SWITCH_ERROR("kzalloc failed\n");
		return -1;
	}

	wireless->pdev = platform_device_alloc("sunxi-wireless", 0);
	if (!wireless->pdev) {
		WIRELESS_SWITCH_ERROR("device created failed\n");
		return -1;
	}

	if (platform_device_add(wireless->pdev)) {
		kfree(wireless->pdev);
		wireless->pdev = NULL;
		WIRELESS_SWITCH_ERROR("add device failed\n");
		return -1;
	}

	np = of_find_node_by_type(NULL, "btlpm");

	wireless->wk_pin = of_get_named_gpio_flags(np, "bt_wake", 0, (enum of_gpio_flags *)&config);
	WIRELESS_SWITCH_DEBUG("bt_wake is %d\n", wireless->wk_pin);

	if (!gpio_is_valid(wireless->wk_pin)) {
		WIRELESS_SWITCH_ERROR("get gpio bt_wakeup_pin failed\n");
		return -1;
	}

	gpio_direction_output(wireless->wk_pin, config.data);

	gpio_set_value(wireless->wk_pin, 0);

	WIRELESS_SWITCH_DEBUG("init wake up pin finish\n");

	return 0;
}

static void sw_platform_deinit(void)
{
	platform_device_del(wireless->pdev);
	WIRELESS_SWITCH_DEBUG("deinit platform finish %p\n", wireless->pdev);

	platform_device_put(wireless->pdev);
	wireless->pdev = NULL;

	kzfree(wireless);
	wireless = NULL;
}

static void sw_pin_init(void)
{
	bt_set_wakeup_pin(BT_STATE_OFF);
	wlan_set_reset_pin(WLAN_STATE_OFF);
	bt_set_reset_pin(BT_STATE_OFF);
}

static int __init wireless_switch_init(void)
{
	WIRELESS_SWITCH_DEBUG("init wireless switch! version %s\n", WIRELESS_MODULE_VERSION);

	if (sw_platform_init() < 0) {
		WIRELESS_SWITCH_DEBUG("init bt wakeup pin failed!\n");
		return -1;
	}
	WIRELESS_SWITCH_DEBUG("init platform success!\n");

	sw_pin_init();
	WIRELESS_SWITCH_DEBUG("init switch pin success!\n");

	if (sw_rfkill_reg() < 0) {
		WIRELESS_SWITCH_ERROR("init rfkill failed!\n");
		return -1;
	}
	WIRELESS_SWITCH_DEBUG("init wireless switch rfkill success!\n");

	WIRELESS_SWITCH_DEBUG("init wireless switch driver finish!\n");

	return 0;
}

static void __exit wireless_switch_exit(void)
{
	sw_pin_init();

	sw_rfkill_dereg();

	sw_platform_deinit();

	WIRELESS_SWITCH_DEBUG("wireless switch driver exit!\n");
}

module_init(wireless_switch_init);
module_exit(wireless_switch_exit);
