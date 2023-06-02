/*
 * usb-serial driver for Quatech USB 2 devices
 *
 * Copyright (C) 2012 Bill Pemberton (wfp5p@virginia.edu)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation.
 *
 *
 *  These devices all have only 1 bulk in and 1 bulk out that is shared
 *  for all serial ports.
 *
 */

#include <asm/unaligned.h>
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/tty.h>
#include <linux/tty_driver.h>
#include <linux/tty_flip.h>
#include <linux/module.h>
#include <linux/serial.h>
#include <linux/usb.h>
#include <linux/usb/serial.h>
#include <linux/serial_reg.h>
#include <linux/uaccess.h>

#define DRIVER_DESC "u1960 USB to Serial Driver"

static const struct usb_device_id id_table[] = {
	{USB_DEVICE(0x1e89,0x1a20)},
	{}			/* Terminating entry */
};

MODULE_DEVICE_TABLE(usb, id_table);

static struct usb_serial_driver u1960_device = {
	.driver = {
		.owner = THIS_MODULE,
		.name = "U1960",
	},
	.description	     = DRIVER_DESC,
	.id_table	     = id_table,
	.num_ports =		1,
};

static struct usb_serial_driver *const serial_drivers[] = {
	&u1960_device, NULL
};

module_usb_serial_driver(serial_drivers, id_table);

MODULE_DESCRIPTION(DRIVER_DESC);
MODULE_LICENSE("GPL");
