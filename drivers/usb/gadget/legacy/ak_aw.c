#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/usb/composite.h>

#include "ak_audio.h"
#include "ak_webcam.h"

#define DRIVER_DESC		"Linux USB Audio Webcam Gadget"
/* Thanks to Linux Foundation for donating this product ID. */
#define AUDIO_VENDOR_NUM		0x1d6b	/* Linux Foundation */
#define AUDIO_PRODUCT_NUM		0x0101	/* Linux-USB Audio Gadget */

USB_GADGET_COMPOSITE_OPTIONS();

static struct usb_device_descriptor device_desc = {
	.bLength =		sizeof device_desc,
	.bDescriptorType =	USB_DT_DEVICE,

	.bcdUSB =		cpu_to_le16(0x200),

// #ifdef CONFIG_GADGET_AK_UAC1
// 	.bDeviceClass =     USB_CLASS_PER_INTERFACE,
// 	.bDeviceSubClass =	0,
// 	.bDeviceProtocol =	0,
// #else
	.bDeviceClass =		USB_CLASS_MISC,
	.bDeviceSubClass =	0x02,
	.bDeviceProtocol =	0x01,
// #endif

	/* Vendor and product id defaults change according to what configs
	 * we support.  (As does bNumConfigurations.)  These values can
	 * also be overridden by module parameters.
	 */
	.idVendor =		cpu_to_le16(AUDIO_VENDOR_NUM),
	.idProduct =		cpu_to_le16(AUDIO_PRODUCT_NUM),
	/* .bcdDevice = f(hardware) */
	/* .iManufacturer = DYNAMIC */
	/* .iProduct = DYNAMIC */
	/* NO SERIAL NUMBER */
	.bNumConfigurations =	1,
};

/* string IDs are assigned dynamically */
static struct usb_string strings_dev[] = {
	[USB_GADGET_MANUFACTURER_IDX].s = "ANYKA",
	[USB_GADGET_PRODUCT_IDX].s = DRIVER_DESC,
	[USB_GADGET_SERIAL_IDX].s = "",
	{  } /* end of list */
};

static struct usb_gadget_strings stringtab_dev = {
	.language = 0x0409,	/* en-us */
	.strings = strings_dev,
};

static struct usb_gadget_strings *audio_strings[] = {
	&stringtab_dev,
	NULL,
};

static struct usb_configuration audio_config_driver = {
	.label			= DRIVER_DESC,
	.bConfigurationValue	= 1,
	/* .iConfiguration = DYNAMIC */
	.bmAttributes		= USB_CONFIG_ATT_SELFPOWER,
    .MaxPower = 500,
};

int audio_webcam_do_config(struct usb_configuration *c)
{
	//int status;
	webcam_config_bind(c);
	audio_do_config(c);
	return 0;
}

static int audio_webcam_bind(struct usb_composite_dev *cdev)
{
    int ret;

	//pr_info("start audio bind\n");
    ret = audio_bind(cdev);
    if (ret){
        pr_err("audio bind err\n");
    }
	//pr_info("start webcam bind\n");
    ret = webcam_bind(cdev);
    if (ret){
        pr_err("webcam bind err\n");
    }

	//printk("%s --> usb_add_config, config=0x%px\n", __func__,&audio_config_driver);
	ret = usb_add_config(cdev, &audio_config_driver, audio_webcam_do_config);
	if (ret < 0){
    	pr_info("usb add config failed\n");
		// goto fail_otg_desc;
	}

	usb_composite_overwrite_options(cdev, &coverwrite);
	return 0;
}

static int audio_webcam_unbind(struct usb_composite_dev *cdev)
{
    int ret;

    ret = audio_unbind(cdev);
    ret = webcam_unbind(cdev);
	return 0;
}

static struct usb_composite_driver audio_webcam_driver = {
	.name		= "g_audio_webcam",
	.dev		= &device_desc,
	.strings	= audio_strings,
	.max_speed	= USB_SPEED_HIGH,
	.bind		= audio_webcam_bind,
	.unbind		= audio_webcam_unbind,
};

module_usb_composite_driver(audio_webcam_driver);

MODULE_DESCRIPTION(DRIVER_DESC);
MODULE_AUTHOR("Anyka");
MODULE_LICENSE("GPL");

