/*
 * bind_dfu.c- Sigmastar
 *
 * Copyright (c) [2019~2020] SigmaStar Technology.
 *
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License version 2 for more details.
 *
 */

// SPDX-License-Identifier: GPL-2.0+
/*
 *	webcam.c -- USB webcam gadget driver
 *
 *	Copyright (C) 2009-2010
 *	    Laurent Pinchart (laurent.pinchart@ideasonboard.com)
 */

#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/module.h>

#include "f_dfu.h"
#include "bind_dfu.h"

static int dfu_function_enable = 0;
module_param(dfu_function_enable, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(dfu_function_enable, "0: Disable 1: Enable");

static struct usb_function_instance *fi_dfu;
static struct usb_function *f_dfu;

int dfu_bind_config(struct usb_configuration *c)
{
	int status;

	if (!dfu_function_enable)
		return 0;

	f_dfu = usb_get_function(fi_dfu);
	if (IS_ERR(f_dfu)) {
		status = PTR_ERR(f_dfu);
		return status;
	}

	status = usb_add_function(c, f_dfu);
	if (status < 0) {
		usb_put_function(f_dfu);
		return status;
	}

	return 0;
}

int dfu_bind(struct usb_composite_dev *cdev)
{
	struct f_dfu_opts *dfu_opts;

	if (!dfu_function_enable)
		return 0;

	fi_dfu = usb_get_function_instance("dfu");
	if (IS_ERR(fi_dfu))
		return PTR_ERR(fi_dfu);

	dfu_opts = container_of(fi_dfu, struct f_dfu_opts, func_inst);
	if (IS_ERR(dfu_opts))
		return PTR_ERR(dfu_opts);

	return 0;
}

int dfu_unbind(struct usb_composite_dev *cdev)
{
	if (!dfu_function_enable)
		return 0;

	if (!IS_ERR_OR_NULL(f_dfu)) {
		usb_put_function(f_dfu);
		f_dfu = NULL;
	}

	if (!IS_ERR_OR_NULL(fi_dfu)) {
		usb_put_function_instance(fi_dfu);
		fi_dfu = NULL;
	}

	return 0;
}

MODULE_AUTHOR("Laurent Pinchart");
MODULE_DESCRIPTION("Webcam Video Gadget");
MODULE_LICENSE("GPL");
