/*
 * sstar_gadget.c- Sigmastar
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

#include <linux/usb/composite.h>

USB_GADGET_COMPOSITE_OPTIONS();

/*-------------------------------------------------------------------------*/

#ifdef CONFIG_USB_SSTAR_UVC
#include "bind_uvc.h"
#endif

#ifdef CONFIG_USB_SSTAR_UAC1
#include "bind_uac1.h"
#endif

#ifdef CONFIG_USB_SSTAR_RNDIS
#include "bind_rndis.h"
#endif

#ifdef CONFIG_USB_SSTAR_DFU
#include "bind_dfu.h"
#endif

/* --------------------------------------------------------------------------
 * Device descriptor
 */

#define WEBCAM_VENDOR_ID 0x1d6b /* Linux Foundation */
#define WEBCAM_PRODUCT_ID 0x0102 /* Webcam A/V gadget */
#define WEBCAM_DEVICE_BCD 0x0010 /* 0.10 */

static char webcam_vendor_label[] = "Linux Foundation";
static char webcam_product_label[] = "Webcam gadget";
static char webcam_config_label[] = "Video";

/* string IDs are assigned dynamically */

#define STRING_DESCRIPTION_IDX USB_GADGET_FIRST_AVAIL_IDX

static struct usb_string webcam_strings[] = {
	[USB_GADGET_MANUFACTURER_IDX].s = webcam_vendor_label,
	[USB_GADGET_PRODUCT_IDX].s = webcam_product_label,
	[USB_GADGET_SERIAL_IDX].s = "",
	[STRING_DESCRIPTION_IDX].s = webcam_config_label,
	{}
};

static struct usb_gadget_strings webcam_stringtab = {
	.language = 0x0409, /* en-us */
	.strings = webcam_strings,
};

static struct usb_gadget_strings *webcam_device_strings[] = {
	&webcam_stringtab,
	NULL,
};

static struct usb_device_descriptor webcam_device_descriptor = {
	.bLength = USB_DT_DEVICE_SIZE,
	.bDescriptorType = USB_DT_DEVICE,
	/* .bcdUSB = DYNAMIC */
	.bDeviceClass = USB_CLASS_MISC,
	.bDeviceSubClass = 0x02,
	.bDeviceProtocol = 0x01,
	.bMaxPacketSize0 = 0, /* dynamic */
	.idVendor = cpu_to_le16(WEBCAM_VENDOR_ID),
	.idProduct = cpu_to_le16(WEBCAM_PRODUCT_ID),
	.bcdDevice = cpu_to_le16(WEBCAM_DEVICE_BCD),
	.iManufacturer = 0, /* dynamic */
	.iProduct = 0, /* dynamic */
	.iSerialNumber = 0, /* dynamic */
	.bNumConfigurations = 0, /* dynamic */
};

/* --------------------------------------------------------------------------
 * USB configuration
 */

static struct usb_configuration webcam_config_driver = {
	.label = webcam_config_label,
	.bConfigurationValue = 1,
	.iConfiguration = 0, /* dynamic */
	.bmAttributes = USB_CONFIG_ATT_SELFPOWER,
	.MaxPower = CONFIG_USB_GADGET_VBUS_DRAW,
};

static int webcam_bind_config(struct usb_configuration *c)
{
	int ret = 0;

#ifdef CONFIG_USB_SSTAR_RNDIS
	ret = rndis_bind_config(c);
	if (ret < 0)
		return ret;
#endif

#ifdef CONFIG_USB_SSTAR_UVC
	ret = video_bind_config(c);
	if (ret < 0)
		return ret;
#endif

#ifdef CONFIG_USB_SSTAR_UAC1
	ret = audio_bind_config(c);
	if (ret < 0)
		return ret;
#endif

#ifdef CONFIG_USB_SSTAR_DFU
	ret = dfu_bind_config(c);
	if (ret < 0)
		return ret;
#endif

	return ret;
}

static int webcam_bind(struct usb_composite_dev *cdev)
{
	int ret;
	/* Allocate string descriptor numbers ... note that string contents
	 * can be overridden by the composite_dev glue.
	 */
	ret = usb_string_ids_tab(cdev, webcam_strings);
	if (ret < 0)
		return ret;
	webcam_device_descriptor.iManufacturer =
		webcam_strings[USB_GADGET_MANUFACTURER_IDX].id;
	webcam_device_descriptor.iProduct =
		webcam_strings[USB_GADGET_PRODUCT_IDX].id;
	webcam_config_driver.iConfiguration =
		webcam_strings[STRING_DESCRIPTION_IDX].id;

#ifdef CONFIG_USB_SSTAR_RNDIS
	ret = rndis_bind(cdev);
	if (ret < 0) {
		goto error;
	}
#endif

#ifdef CONFIG_USB_SSTAR_UVC
	ret = video_bind(cdev);
	if (ret < 0)
		goto error;
#endif

#ifdef CONFIG_USB_SSTAR_UAC1
	ret = audio_bind(cdev);
	if (ret < 0)
		goto error;
#endif

#ifdef CONFIG_USB_SSTAR_DFU
	ret = dfu_bind(cdev);
	if (ret < 0)
		goto error;
#endif

#ifdef CONFIG_WINDOWS_HELLO_SUPPORT
	cdev->gadget->lpm_capable = true;
	cdev->use_os_string = true;
	cdev->b_vendor_code = 0x01;
	cdev->os_desc_config = &webcam_config_driver;
#endif

	/* Register our configuration. */
	if ((ret = usb_add_config(cdev, &webcam_config_driver,
				  webcam_bind_config)) < 0) {
		goto error;
	}

	usb_composite_overwrite_options(cdev, &coverwrite);
	INFO(cdev, "Webcam Video Gadget\n");
	return 0;

error:
#ifdef CONFIG_USB_SSTAR_DFU
	dfu_unbind(cdev);
#endif

#ifdef CONFIG_USB_SSTAR_UAC1
	audio_unbind(cdev);
#endif

#ifdef CONFIG_USB_SSTAR_UVC
	video_unbind(cdev);
#endif

#ifdef CONFIG_USB_SSTAR_RNDIS
	rndis_unbind(cdev);
#endif
	return ret;
}
static int webcam_unbind(struct usb_composite_dev *cdev)
{
	int ret = 0;

#ifdef CONFIG_USB_SSTAR_RNDIS
	ret = rndis_unbind(cdev);
#endif

#ifdef CONFIG_USB_SSTAR_UVC
	ret = video_unbind(cdev);
#endif

#ifdef CONFIG_USB_SSTAR_UAC1
	ret = audio_unbind(cdev);
#endif

#ifdef CONFIG_USB_SSTAR_DFU
	ret = dfu_unbind(cdev);
#endif

	return ret;
}

/* --------------------------------------------------------------------------
 * Driver
 */

static struct usb_composite_driver webcam_driver = {
	.name = "g_webcam",
	.dev = &webcam_device_descriptor,
	.strings = webcam_device_strings,
	.max_speed = USB_SPEED_SUPER,
	.bind = webcam_bind,
	.unbind = webcam_unbind,
};

module_usb_composite_driver(webcam_driver);

MODULE_AUTHOR("Laurent Pinchart");
MODULE_DESCRIPTION("Webcam Video Gadget");
MODULE_LICENSE("GPL");
