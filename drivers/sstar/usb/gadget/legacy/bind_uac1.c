/*
 * bind_uac1.c- Sigmastar
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

#include "u_uac1.h"
#include "bind_uac1.h"

/* Playback(USB-IN) Endpoint Size */
static int p_mpsize = UAC1_OUT_EP_MAX_PACKET_SIZE;
module_param(p_mpsize, uint, S_IRUGO);
MODULE_PARM_DESC(p_mpsize, "Playback Endpoint Max Packet Size");

/* Playback(USB-IN) Default Stereo - Fl/Fr */
static int p_chmask = UAC1_DEF_PCHMASK;
module_param(p_chmask, uint, S_IRUGO);
MODULE_PARM_DESC(p_chmask, "Playback Channel Mask");

/* Playback Default 48 KHz */
static int p_srate = UAC1_DEF_PSRATE;
module_param(p_srate, uint, S_IRUGO);
MODULE_PARM_DESC(p_srate, "Playback Sampling Rate");

/* Playback Default 16bits/sample */
static int p_ssize = UAC1_DEF_PSSIZE;
module_param(p_ssize, uint, S_IRUGO);
MODULE_PARM_DESC(p_ssize, "Playback Sample Size(bytes)");

/* Capture(USB-OUT) Endpoint Size */
static int c_mpsize = UAC1_OUT_EP_MAX_PACKET_SIZE;
module_param(c_mpsize, uint, S_IRUGO);
MODULE_PARM_DESC(c_mpsize, "Capture Endpoint Max Packet Size");

/* Capture(USB-OUT) Default Stereo - Fl/Fr */
static int c_chmask = UAC1_DEF_CCHMASK;
module_param(c_chmask, uint, S_IRUGO);
MODULE_PARM_DESC(c_chmask, "Capture Channel Mask");

/* Capture Default 48 KHz */
static int c_srate = UAC1_DEF_CSRATE;
module_param(c_srate, uint, S_IRUGO);
MODULE_PARM_DESC(c_srate, "Capture Sampling Rate");

/* Capture Default 16bits/sample */
static int c_ssize = UAC1_DEF_CSSIZE;
module_param(c_ssize, uint, S_IRUGO);
MODULE_PARM_DESC(c_ssize, "Capture Sample Size(bytes)");

static int uac_function_enable = 0;
module_param(uac_function_enable, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(uac_function_enable,
		 "Audio Play Mode, 0: Disable UAC Function, "
		 "1: Speaker Only, 2: Microphone Only, 3: Speaker & Microphone");

static struct usb_function_instance *fi_uac1;
static struct usb_function *f_uac1;

int audio_bind_config(struct usb_configuration *c)
{
	int status;

	if (!uac_function_enable)
		return 0;

	f_uac1 = usb_get_function(fi_uac1);
	if (IS_ERR(f_uac1)) {
		status = PTR_ERR(f_uac1);
		return status;
	}

	status = usb_add_function(c, f_uac1);
	if (status < 0) {
		usb_put_function(f_uac1);
		return status;
	}

	return 0;
}

int audio_bind(struct usb_composite_dev *cdev)
{
	struct f_uac1_opts *uac1_opts;

	if (!uac_function_enable)
		return 0;

	fi_uac1 = usb_get_function_instance("uac1");
	if (IS_ERR(fi_uac1))
		return PTR_ERR(fi_uac1);

	uac1_opts = container_of(fi_uac1, struct f_uac1_opts, func_inst);

	if (2 == uac_function_enable || 3 == uac_function_enable)
		uac1_opts->p_chmask = p_chmask;
	else
		uac1_opts->p_chmask = 0;

	uac1_opts->p_mpsize = p_mpsize;
	uac1_opts->p_srate = p_srate;
	uac1_opts->p_ssize = p_ssize;

	if (1 == uac_function_enable || 3 == uac_function_enable)
		uac1_opts->c_chmask = c_chmask;
	else
		uac1_opts->c_chmask = 0;

	uac1_opts->c_mpsize = c_mpsize;
	uac1_opts->c_srate = c_srate;
	uac1_opts->c_ssize = c_ssize;
	uac1_opts->req_number = UAC1_DEF_REQ_NUM;

	return 0;
}

int audio_unbind(struct usb_composite_dev *cdev)
{
	if (!uac_function_enable)
		return 0;

	if (!IS_ERR_OR_NULL(f_uac1)) {
		usb_put_function(f_uac1);
		f_uac1 = NULL;
	}

	if (!IS_ERR_OR_NULL(fi_uac1)) {
		usb_put_function_instance(fi_uac1);
		fi_uac1 = NULL;
	}

	return 0;
}

MODULE_AUTHOR("Laurent Pinchart");
MODULE_DESCRIPTION("Webcam Video Gadget");
MODULE_LICENSE("GPL");
