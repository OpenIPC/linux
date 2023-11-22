/*
 * bind_rndis.c- Sigmastar
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

#include "u_ether.h"
#include "u_gether.h"
#include "u_rndis.h"
#include "rndis.h"
#include "bind_rndis.h"

USB_ETHERNET_MODULE_PARAMETERS();

static int rndis_function_enable = 0;
module_param(rndis_function_enable, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(rndis_function_enable, "RNDIS Function Enable");

static struct usb_function_instance *fi_rndis;
static struct usb_function *f_rndis;

int rndis_bind_config(struct usb_configuration *c)
{
	int status = 0;

	if (!rndis_function_enable)
		return 0;

	f_rndis = usb_get_function(fi_rndis);
	if (IS_ERR(f_rndis))
		return PTR_ERR(f_rndis);

	status = usb_add_function(c, f_rndis);
	if (status < 0)
		usb_put_function(f_rndis);

	return status;
}

int rndis_bind(struct usb_composite_dev *cdev)
{
	struct f_rndis_opts *rndis_opts = NULL;
	struct net_device *net;
	int status;

	if (!rndis_function_enable)
		return 0;

	fi_rndis = usb_get_function_instance("rndis");
	if (IS_ERR(fi_rndis)) {
		status = PTR_ERR(fi_rndis);
		goto fail;
	}

	/* set up main config label and device descriptor */
	rndis_opts = container_of(fi_rndis, struct f_rndis_opts, func_inst);

	net = rndis_opts->net;

	gether_set_qmult(net, qmult);
	if (!gether_set_host_addr(net, host_addr))
		pr_info("using host ethernet address: %s", host_addr);
	if (!gether_set_dev_addr(net, dev_addr))
		pr_info("using self ethernet address: %s", dev_addr);

	fi_rndis = usb_get_function_instance("rndis");
	if (IS_ERR(fi_rndis)) {
		status = PTR_ERR(fi_rndis);
		goto fail;
	}

	return 0;

fail:
	fi_rndis = NULL;
	return status;
}

int rndis_unbind(struct usb_composite_dev *cdev)
{
	if (!rndis_function_enable)
		return 0;

	if (!IS_ERR_OR_NULL(f_rndis)) {
		usb_put_function(f_rndis);
		f_rndis = NULL;
	}

	if (!IS_ERR_OR_NULL(fi_rndis)) {
		usb_put_function_instance(fi_rndis);
		fi_rndis = NULL;
	}

	return 0;
}

MODULE_AUTHOR("Laurent Pinchart");
MODULE_DESCRIPTION("Webcam Video Gadget");
MODULE_LICENSE("GPL");
