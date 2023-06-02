/*
 * f_uac1.c -- USB Audio Class 1.0 Function (using u_audio API)
 *
 * Copyright (C) 2016 Ruslan Bilovol <ruslan.bilovol@gmail.com>
 *
 * This driver doesn't expect any real Audio codec to be present
 * on the device - the audio streams are simply sinked to and
 * sourced from a virtual ALSA sound card created.
 *
 * This file is based on f_uac1.c which is
 *   Copyright (C) 2008 Bryan Wu <cooloney@kernel.org>
 *   Copyright (C) 2008 Analog Devices, Inc
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <linux/usb/audio.h>
#include <linux/module.h>

#include <media/v4l2-dev.h>
#include <media/v4l2-event.h>
#include "uac_ex.h"
#include "uac_v4l2_ex.h"
#include "u_uac1.h"

struct f_uac1 {
	struct uac_device uac_dev;
	u8 ac_intf, as_in_intf, as_out_intf;
	u8 ac_alt, as_in_alt, as_out_alt;	/* needed for get_alt() */
};

struct uac1_format_type_ii_discrete_descriptor {
    __u8 bLength;
    __u8 bDescriptorType;
    __u8 bDescriptorSubtype;
    __u8 bFormatType;
    __le16 wMaxBitRate;
    __le16 wSamplesPerFrame;
    __u8 bSamFreqType;
    __u8 tSamFreq[4][3];
} __attribute__((packed));

struct uac1_format_type_ii_ac3_descriptor {
    __u8 bLength;
    __u8 bDescriptorType;
    __u8 bDescriptorSubtype;
    __le16 wFormatTag;
    __u8 bmBSID[4];
    __u8 bmAC3Features;
} __attribute__((packed));

static inline struct f_uac1 *func_to_uac1(struct usb_function *f)
{
	return container_of(f, struct f_uac1, uac_dev.func);
}

/*
 * DESCRIPTORS ... most are static, but strings and full
 * configuration descriptors are built on demand.
 */

/*
 * We have three interfaces - one AudioControl and two AudioStreaming
 *
 * The driver implements a simple UAC_1 topology.
 * USB-OUT -> IT_1 -> OT_2 -> ALSA_Capture
 * ALSA_Playback -> IT_3 -> OT_4 -> USB-IN
 */
#define F_AUDIO_AC_INTERFACE		0
#define F_AUDIO_AS_OUT_INTERFACE	1
#define F_AUDIO_AS_IN_INTERFACE		2
/* Number of streaming interfaces */
#define F_AUDIO_NUM_INTERFACES		1

static struct usb_interface_assoc_descriptor uac_iad = {
	.bLength		= sizeof(uac_iad),
	.bDescriptorType	= USB_DT_INTERFACE_ASSOCIATION,
	.bFirstInterface	= 0,
	.bInterfaceCount	= 2,
	.bFunctionClass		= USB_CLASS_AUDIO,
	.bFunctionSubClass	= 0x00,
	.bFunctionProtocol	= 0x00,
	.iFunction		= 0,
};

/* B.3.1  Standard AC Interface Descriptor */
static struct usb_interface_descriptor ac_interface_desc = {
	.bLength =		USB_DT_INTERFACE_SIZE,
	.bDescriptorType =	USB_DT_INTERFACE,
	.bNumEndpoints =	0,
	.bInterfaceClass =	USB_CLASS_AUDIO,
	.bInterfaceSubClass =	USB_SUBCLASS_AUDIOCONTROL,
};

/*
 * The number of AudioStreaming and MIDIStreaming interfaces
 * in the Audio Interface Collection
 */
DECLARE_UAC_AC_HEADER_DESCRIPTOR(1);

#define UAC_DT_AC_HEADER_LENGTH	UAC_DT_AC_HEADER_SIZE(F_AUDIO_NUM_INTERFACES)

/* 2 input terminals and 2 output terminals */
#define UAC_DT_TOTAL_LENGTH (UAC_DT_AC_HEADER_LENGTH \
	+ 1*UAC_DT_INPUT_TERMINAL_SIZE + 1*UAC_DT_OUTPUT_TERMINAL_SIZE)

/* B.3.2  Class-Specific AC Interface Descriptor */
static struct uac1_ac_header_descriptor_1 ac_header_desc = {
	.bLength =		UAC_DT_AC_HEADER_LENGTH,
	.bDescriptorType =	USB_DT_CS_INTERFACE,
	.bDescriptorSubtype =	UAC_HEADER,
	.bcdADC =		cpu_to_le16(0x0100),
	.wTotalLength =		cpu_to_le16(UAC_DT_TOTAL_LENGTH),
	.bInCollection =	F_AUDIO_NUM_INTERFACES,
	.baInterfaceNr = {
	/* Interface number of the AudioStream interfaces */
		[0] =		1,
	}
};

#define USB_OUT_IT_ID	1
static struct uac_input_terminal_descriptor usb_out_it_desc = {
	.bLength =		UAC_DT_INPUT_TERMINAL_SIZE,
	.bDescriptorType =	USB_DT_CS_INTERFACE,
	.bDescriptorSubtype =	UAC_INPUT_TERMINAL,
	.bTerminalID =		USB_OUT_IT_ID,
	.wTerminalType =	cpu_to_le16(UAC_TERMINAL_STREAMING),
	.bAssocTerminal =	0,
	.wChannelConfig =	cpu_to_le16(0x3),
};

#define IO_OUT_OT_ID	2
static struct uac1_output_terminal_descriptor io_out_ot_desc = {
	.bLength		= UAC_DT_OUTPUT_TERMINAL_SIZE,
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubtype	= UAC_OUTPUT_TERMINAL,
	.bTerminalID		= IO_OUT_OT_ID,
	.wTerminalType		= cpu_to_le16(UAC_OUTPUT_TERMINAL_SPEAKER),
	.bAssocTerminal		= 0,
	.bSourceID		= USB_OUT_IT_ID,
};

#define IO_IN_IT_ID	3
static struct uac_input_terminal_descriptor io_in_it_desc = {
	.bLength		= UAC_DT_INPUT_TERMINAL_SIZE,
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubtype	= UAC_INPUT_TERMINAL,
	.bTerminalID		= IO_IN_IT_ID,
	.wTerminalType		= cpu_to_le16(UAC_INPUT_TERMINAL_MICROPHONE),
	.bAssocTerminal		= 0,
	.wChannelConfig		= cpu_to_le16(0x3),
};

#define USB_IN_OT_ID	4
static struct uac1_output_terminal_descriptor usb_in_ot_desc = {
	.bLength =		UAC_DT_OUTPUT_TERMINAL_SIZE,
	.bDescriptorType =	USB_DT_CS_INTERFACE,
	.bDescriptorSubtype =	UAC_OUTPUT_TERMINAL,
	.bTerminalID =		USB_IN_OT_ID,
	.wTerminalType =	cpu_to_le16(UAC_TERMINAL_STREAMING),
	.bAssocTerminal =	0,
	.bSourceID =		IO_IN_IT_ID,
};

/* B.4.1  Standard AS Interface Descriptor */
static struct usb_interface_descriptor as_out_interface_alt_0_desc = {
	.bLength =		USB_DT_INTERFACE_SIZE,
	.bDescriptorType =	USB_DT_INTERFACE,
	.bAlternateSetting =	0,
	.bNumEndpoints =	0,
	.bInterfaceClass =	USB_CLASS_AUDIO,
	.bInterfaceSubClass =	USB_SUBCLASS_AUDIOSTREAMING,
};

static struct usb_interface_descriptor as_out_interface_alt_1_desc = {
	.bLength =		USB_DT_INTERFACE_SIZE,
	.bDescriptorType =	USB_DT_INTERFACE,
	.bAlternateSetting =	1,
	.bNumEndpoints =	1,
	.bInterfaceClass =	USB_CLASS_AUDIO,
	.bInterfaceSubClass =	USB_SUBCLASS_AUDIOSTREAMING,
};

static struct usb_interface_descriptor as_in_interface_alt_0_desc = {
	.bLength =		USB_DT_INTERFACE_SIZE,
	.bDescriptorType =	USB_DT_INTERFACE,
	.bAlternateSetting =	0,
	.bNumEndpoints =	0,
	.bInterfaceClass =	USB_CLASS_AUDIO,
	.bInterfaceSubClass =	USB_SUBCLASS_AUDIOSTREAMING,
};

static struct usb_interface_descriptor as_in_interface_alt_1_desc = {
	.bLength =		USB_DT_INTERFACE_SIZE,
	.bDescriptorType =	USB_DT_INTERFACE,
	.bAlternateSetting =	1,
	.bNumEndpoints =	1,
	.bInterfaceClass =	USB_CLASS_AUDIO,
	.bInterfaceSubClass =	USB_SUBCLASS_AUDIOSTREAMING,
};

static struct usb_interface_descriptor as_in_interface_alt_2_desc = {
    .bLength =        USB_DT_INTERFACE_SIZE,
    .bDescriptorType =    USB_DT_INTERFACE,
    .bAlternateSetting =    2,
    .bNumEndpoints =    1,
    .bInterfaceClass =    USB_CLASS_AUDIO,
    .bInterfaceSubClass =    USB_SUBCLASS_AUDIOSTREAMING,
};

/* B.4.2  Class-Specific AS Interface Descriptor */
static struct uac1_as_header_descriptor as_in_header_desc = {
	.bLength =		UAC_DT_AS_HEADER_SIZE,
	.bDescriptorType =	USB_DT_CS_INTERFACE,
	.bDescriptorSubtype =	UAC_AS_GENERAL,
	.bTerminalLink =	USB_IN_OT_ID,
	.bDelay =		1,
	.wFormatTag =		cpu_to_le16(UAC_FORMAT_TYPE_I_PCM),
};

/* B.4.2  Class-Specific AS Interface Descriptor */
static struct uac1_as_header_descriptor as_in_header_2_desc = {
    .bLength =        UAC_DT_AS_HEADER_SIZE,
    .bDescriptorType =    USB_DT_CS_INTERFACE,
    .bDescriptorSubtype =    UAC_AS_GENERAL,
    .bTerminalLink =    USB_IN_OT_ID,
    .bDelay =        1,
    .wFormatTag =        cpu_to_le16(UAC_FORMAT_TYPE_II_AC3),
};

static struct uac1_format_type_ii_discrete_descriptor as_in_type_ii_ac3_desc = {
    .bLength =        sizeof(as_in_type_ii_ac3_desc),
    .bDescriptorType =    USB_DT_CS_INTERFACE,
    .bDescriptorSubtype =    UAC_FORMAT_TYPE,
    .bFormatType =        UAC_FORMAT_TYPE_II,
    .wMaxBitRate =    2,        /*FIXME: how to set MaxBitRate value ?? */
    .wSamplesPerFrame =    16,   /*FIXME: how to set SamplesPerFrame??*/
    .bSamFreqType =        4,
};

static struct uac1_format_type_ii_ac3_descriptor as_in_type_ii_ac3_spec_desc = {
    .bLength =        sizeof(as_in_type_ii_ac3_spec_desc),
    .bDescriptorType =    USB_DT_CS_INTERFACE,
    .bDescriptorSubtype =    UAC_FORMAT_SPECIFIC,
    .wFormatTag =        cpu_to_le16(UAC_FORMAT_TYPE_II_AC3),
};

DECLARE_UAC_FORMAT_TYPE_I_DISCRETE_DESC(4);

static struct uac_format_type_i_discrete_descriptor_4 as_out_type_i_desc = {
    .bLength =        UAC_FORMAT_TYPE_I_DISCRETE_DESC_SIZE(4),
	.bDescriptorType =	USB_DT_CS_INTERFACE,
	.bDescriptorSubtype =	UAC_FORMAT_TYPE,
	.bFormatType =		UAC_FORMAT_TYPE_I,
	.bSubframeSize =	2,
	.bBitResolution =	16,
	.bSamFreqType =        4,
};

/* Standard ISO OUT Endpoint Descriptor */
static struct usb_endpoint_descriptor as_out_ep_desc  = {
	.bLength =		USB_DT_ENDPOINT_AUDIO_SIZE,
	.bDescriptorType =	USB_DT_ENDPOINT,
	.bEndpointAddress =	USB_DIR_OUT,
	.bmAttributes =		USB_ENDPOINT_SYNC_ADAPTIVE
				| USB_ENDPOINT_XFER_ISOC,
	.wMaxPacketSize	=	cpu_to_le16(UAC1_OUT_EP_MAX_PACKET_SIZE),
	.bInterval =		4,
};

static struct uac_format_type_i_discrete_descriptor_4 as_in_type_i_desc = {
    .bLength =        UAC_FORMAT_TYPE_I_DISCRETE_DESC_SIZE(4),
	.bDescriptorType =	USB_DT_CS_INTERFACE,
	.bDescriptorSubtype =	UAC_FORMAT_TYPE,
	.bFormatType =		UAC_FORMAT_TYPE_I,
	.bSubframeSize =	2,
	.bBitResolution =	16,
    .bSamFreqType =        4,
};

/* Standard ISO OUT Endpoint Descriptor */
static struct usb_endpoint_descriptor as_in_ep_desc  = {
	.bLength =		USB_DT_ENDPOINT_AUDIO_SIZE,
	.bDescriptorType =	USB_DT_ENDPOINT,
	.bEndpointAddress =	USB_DIR_IN,
	.bmAttributes =		USB_ENDPOINT_SYNC_ASYNC
				| USB_ENDPOINT_XFER_ISOC,
	.wMaxPacketSize	=	cpu_to_le16(UAC1_OUT_EP_MAX_PACKET_SIZE),
	.bInterval =		4,
};

/* Class-specific AS ISO OUT Endpoint Descriptor */
static struct uac_iso_endpoint_descriptor as_iso_in_desc = {
	.bLength =		UAC_ISO_ENDPOINT_DESC_SIZE,
	.bDescriptorType =	USB_DT_CS_ENDPOINT,
	.bDescriptorSubtype =	UAC_EP_GENERAL,
	.bmAttributes =		1,
	.bLockDelayUnits =	0,
	.wLockDelay =		0,
};

static struct usb_descriptor_header *f_audio_desc[] = {
	(struct usb_descriptor_header *)&uac_iad,
	(struct usb_descriptor_header *)&ac_interface_desc,
	(struct usb_descriptor_header *)&ac_header_desc,
	(struct usb_descriptor_header *)&io_in_it_desc,
	(struct usb_descriptor_header *)&usb_in_ot_desc,
	(struct usb_descriptor_header *)&as_in_interface_alt_0_desc,
	(struct usb_descriptor_header *)&as_in_interface_alt_1_desc,
	(struct usb_descriptor_header *)&as_in_header_desc,
	(struct usb_descriptor_header *)&as_in_type_i_desc,
	(struct usb_descriptor_header *)&as_in_ep_desc,
	(struct usb_descriptor_header *)&as_iso_in_desc,
    (struct usb_descriptor_header *)&as_in_interface_alt_2_desc,
    (struct usb_descriptor_header *)&as_in_header_2_desc,
    (struct usb_descriptor_header *)&as_in_type_ii_ac3_desc,
    (struct usb_descriptor_header *)&as_in_type_ii_ac3_spec_desc,
    (struct usb_descriptor_header *)&as_in_ep_desc,
    (struct usb_descriptor_header *)&as_iso_in_desc,
	NULL,
};

enum {
	STR_AC_IF,
	STR_USB_OUT_IT,
	STR_USB_OUT_IT_CH_NAMES,
	STR_IO_OUT_OT,
	STR_IO_IN_IT,
	STR_IO_IN_IT_CH_NAMES,
	STR_USB_IN_OT,
	STR_AS_OUT_IF_ALT0,
	STR_AS_OUT_IF_ALT1,
	STR_AS_IN_IF_ALT0,
	STR_AS_IN_IF_ALT1,
    STR_AS_IN_IF_ALT2,
};

static struct usb_string strings_uac1[] = {
	[STR_AC_IF].s = "Hisilicon HD Audio",
	[STR_USB_OUT_IT].s = "Playback Input terminal",
	[STR_USB_OUT_IT_CH_NAMES].s = "Playback Channels",
	[STR_IO_OUT_OT].s = "Playback Output terminal",
	[STR_IO_IN_IT].s = "Capture Input terminal",
	[STR_IO_IN_IT_CH_NAMES].s = "Capture Channels",
	[STR_USB_IN_OT].s = "Capture Output terminal",
	[STR_AS_OUT_IF_ALT0].s = "Playback Inactive",
	[STR_AS_OUT_IF_ALT1].s = "Playback Active",
	[STR_AS_IN_IF_ALT0].s = "Capture Inactive",
	[STR_AS_IN_IF_ALT1].s = "Capture Active",
    [STR_AS_IN_IF_ALT2].s = "Capture Active",
	{ },
};

static struct usb_gadget_strings str_uac1 = {
	.language = 0x0409,	/* en-us */
	.strings = strings_uac1,
};

static struct usb_gadget_strings *uac1_strings[] = {
	&str_uac1,
	NULL,
};

/*
 * This function is an ALSA sound card following USB Audio Class Spec 1.0.
 */

static int audio_set_endpoint_req(struct usb_function *f,
		const struct usb_ctrlrequest *ctrl)
{
	//struct usb_composite_dev *cdev = f->config->cdev;
	int			value = -EOPNOTSUPP;
	u16			ep = le16_to_cpu(ctrl->wIndex);
	u16			len = le16_to_cpu(ctrl->wLength);
	u16			w_value = le16_to_cpu(ctrl->wValue);

	printk(KERN_EMERG  "%s:bRequest 0x%x, w_value 0x%04x, len %d, endpoint %d\n",
			__func__, ctrl->bRequest, w_value, len, ep);


    if ((w_value >>8) != UAC_EP_CS_ATTR_SAMPLE_RATE) {
        printk("!!!!! value = 0x%x\n", (w_value >>8));
        return value;
	}

	switch (ctrl->bRequest) {
	case UAC_SET_CUR:
		value = len;
		break;

	case UAC_SET_MIN:
		break;

	case UAC_SET_MAX:
		break;

	case UAC_SET_RES:
		break;

	case UAC_SET_MEM:
		break;

	default:
		break;
	}

	return value;
}

static int audio_get_endpoint_req(struct usb_function *f,
		const struct usb_ctrlrequest *ctrl)
{
	struct usb_composite_dev *cdev = f->config->cdev;
	struct f_uac1 *uac1 = func_to_uac1(f);
	struct usb_request    *req = uac1->uac_dev.control_req;
	struct uac_device *uac = req->context;
	int value = -EOPNOTSUPP;
	u8 ep = ((le16_to_cpu(ctrl->wIndex) >> 8) & 0xFF);
	u16 len = le16_to_cpu(ctrl->wLength);
	u16 w_value = le16_to_cpu(ctrl->wValue);

	printk(KERN_EMERG  "%s:bRequest 0x%x, w_value 0x%04x, len %d, endpoint %d\n",
			__func__, ctrl->bRequest, w_value, len, ep);

	switch (ctrl->bRequest) {
	case UAC_GET_CUR:
		if ((w_value >>8) == UAC_EP_CS_ATTR_SAMPLE_RATE) {
			memcpy(req->buf, &uac->params.p_srate, 3);
		}
		value = len;
		break;
	case UAC_GET_MIN:
	case UAC_GET_MAX:
	case UAC_GET_RES:
		value = len;
		break;
	case UAC_GET_MEM:
		break;
	default:
		break;
	}

	return value;
}

static int
f_audio_setup(struct usb_function *f, const struct usb_ctrlrequest *ctrl)
{
	struct usb_composite_dev *cdev = f->config->cdev;
    struct f_uac1 *uac1 = func_to_uac1(f);
    struct usb_request    *req = uac1->uac_dev.control_req;
	int			value = -EOPNOTSUPP;
	u16			w_index = le16_to_cpu(ctrl->wIndex);
	u16			w_value = le16_to_cpu(ctrl->wValue);
	u16			w_length = le16_to_cpu(ctrl->wLength);

	printk(KERN_EMERG "%s:\n", __func__);

	/* composite driver infrastructure handles everything; interface
	 * activation uses set_alt().
	 */
	switch (ctrl->bRequestType) {
	case USB_DIR_OUT | USB_TYPE_CLASS | USB_RECIP_ENDPOINT:
        {
            value = audio_set_endpoint_req(f, ctrl);
	/* respond with data transfer or status phase? */
	if (value >= 0) {
		printk(KERN_EMERG  "audio req%02x.%02x v%04x i%04x l%d\n",
			ctrl->bRequestType, ctrl->bRequest,
			w_value, w_index, w_length);
		req->zero = 0;
		req->length = value;
		value = usb_ep_queue(cdev->gadget->ep0, req, GFP_ATOMIC);
		if (value < 0)
			ERROR(cdev, "audio response on err %d\n", value);
            }
        }
        break;

    case USB_DIR_IN | USB_TYPE_CLASS | USB_RECIP_ENDPOINT:
        value = audio_get_endpoint_req(f, ctrl);
		if (value >= 0) {
			printk(KERN_EMERG  "audio req%02x.%02x v%04x i%04x l%d\n",
				ctrl->bRequestType, ctrl->bRequest,
				w_value, w_index, w_length);
				req->zero = 0;
				req->length = value;
				value = usb_ep_queue(cdev->gadget->ep0, req, GFP_ATOMIC);
				if (value < 0)
					ERROR(cdev, "audio response on err %d\n", value);
          }
        break;

    default:
        printk(KERN_EMERG   "invalid control req%02x.%02x v%04x i%04x l%d\n",
            ctrl->bRequestType, ctrl->bRequest,
            w_value, w_index, w_length);
    }

	/* device either stalls (value < 0) or reports success */
	return value;
}

static int f_audio_set_alt(struct usb_function *f, unsigned intf, unsigned alt)
{
	struct f_uac1 *uac1 = func_to_uac1(f);
	int ret = 0;

	printk(KERN_EMERG "%s:intf:%d alt=%d\n", __func__, intf, alt);

	/* No i/f has more than 2 alt settings */
	if (alt > 1) {
		printk(KERN_EMERG  "%s:%d Error!\n", __func__, __LINE__);
		return -EINVAL;
	}

	if (intf == uac1->ac_intf) {
		/* Control I/f has only 1 AltSetting - 0 */
		if (alt) {
			printk(KERN_EMERG  "%s:%d Error!\n", __func__, __LINE__);
			return -EINVAL;
		}

		/*FIXME: notify userpace uac has been connected*/
		{
			struct uac_device *uac_dev = &uac1->uac_dev;
			struct v4l2_event v4l2_event;

			memset(&v4l2_event, 0, sizeof(v4l2_event));
			v4l2_event.type = UAC_EVENT_CONNECT;
			v4l2_event_queue(uac_dev->vdev, &v4l2_event);

			printk(KERN_EMERG "%s: trigger connect\n", __func__);
		}

		return 0;
	}

	if (intf == uac1->as_out_intf) {
		uac1->as_out_alt = alt;

		if (alt) {
		} else {
		}
	} else if (intf == uac1->as_in_intf) {
		uac1->as_in_alt = alt;

			if (alt) {
				/*FIXME: notify userpace to start audio streaming*/
				{
					struct uac_device *uac_dev = &uac1->uac_dev;
					struct v4l2_event v4l2_event;

					memset(&v4l2_event, 0, sizeof(v4l2_event));
					v4l2_event.type = UAC_EVENT_STREAMON;
					v4l2_event_queue(uac_dev->vdev, &v4l2_event);

					printk(KERN_EMERG "%s: trigger UAC_EVENT_STREAMON\n", __func__);
				}

				ret = uac_device_start_playback(&uac1->uac_dev);
			} else {
				/*FIXME: notify userpace to start audio streaming*/
				{
					struct uac_device *uac_dev = &uac1->uac_dev;
					struct v4l2_event v4l2_event;

					memset(&v4l2_event, 0, sizeof(v4l2_event));
					v4l2_event.type = UAC_EVENT_STREAMOFF;
					v4l2_event_queue(uac_dev->vdev, &v4l2_event);

					printk(KERN_EMERG "%s: trigger UAC_EVENT_STREAMOFF\n", __func__);
				}

				uac_device_stop_playback(&uac1->uac_dev);
			}
	} else {
		printk(KERN_EMERG  "%s:%d Error!\n", __func__, __LINE__);
		return -EINVAL;
	}

	return ret;
}

static int f_audio_get_alt(struct usb_function *f, unsigned intf)
{
	struct f_uac1 *uac1 = func_to_uac1(f);

	printk(KERN_EMERG "%s:\n", __func__);

	if (intf == uac1->ac_intf) {
		printk(KERN_EMERG "%s:ac_intf = %d\n", __func__, uac1->ac_intf);
		return uac1->ac_alt;
	}
	else if (intf == uac1->as_out_intf) {
		printk(KERN_EMERG "%s:as_out_intf = %d\n", __func__, uac1->as_out_intf);
		return uac1->as_out_alt;
	}
	else if (intf == uac1->as_in_intf) {
		printk(KERN_EMERG "%s:as_in_intf = %d\n", __func__, uac1->as_in_intf);
		return uac1->as_in_alt;
	}
	else
		printk(KERN_EMERG  "%s:%d Invalid Interface %d!\n",
			__func__, __LINE__, intf);

	return -EINVAL;
}


static void f_audio_disable(struct usb_function *f)
{
	struct f_uac1 *uac1 = func_to_uac1(f);

	uac1->as_out_alt = 0;
	uac1->as_in_alt = 0;


	/*FIXME: notify userpace to disconnect*/
	{
		struct uac_device *uac_dev = &uac1->uac_dev;
		struct v4l2_event v4l2_event;

		memset(&v4l2_event, 0, sizeof(v4l2_event));
		v4l2_event.type = UAC_EVENT_DISCONNECT;
		v4l2_event_queue(uac_dev->vdev, &v4l2_event);
	}
}

/*-------------------------------------------------------------------------*/

static int
__uac_register_video(struct usb_composite_dev *dev, struct uac_device *uac_dev)
{
	struct usb_composite_dev *cdev = dev;
	struct video_device *video;

	/* TODO reference counting. */
	video = video_device_alloc();
	if (video == NULL)
		return -ENOMEM;

	video->v4l2_dev = &uac_dev->v4l2_dev;
	video->fops = &uac_v4l2_fops;
	video->ioctl_ops = &uac_v4l2_ioctl_ops;
	video->release = video_device_release;
	video->vfl_dir = VFL_DIR_TX;
	strlcpy(video->name, cdev->gadget->name, sizeof(video->name));

	uac_dev->vdev = video;
	video_set_drvdata(video, uac_dev);

	return video_register_device(video, VFL_TYPE_GRABBER, -1);
}

static void
__uac_function_ep0_complete(struct usb_ep *ep, struct usb_request *req)
{
    struct uac_device *uac = req->context;
    struct v4l2_event v4l2_event;
	struct uac_event *uac_event = (void *)&v4l2_event.u.data;

	/*FIXME: notify userpace to set audio resolution */
	if (req->actual == 3)
	{
		int rate = ((unsigned char*)req->buf)[0] | (((unsigned char*)req->buf)[1] << 8) | (((unsigned char*)req->buf)[2] << 16);

		uac->params.p_srate = rate;
		uac->p_pktsize = (rate / 1000) * uac->params.p_ssize * num_channels(uac->params.p_chmask);

		memset(&v4l2_event, 0, sizeof(v4l2_event));
		v4l2_event.type = UAC_EVENT_DATA;
		uac_event->data.length = req->actual;
		memcpy(&uac_event->data.data, req->buf, req->actual);
		v4l2_event_queue(uac->vdev, &v4l2_event);

		printk("%s:rate=%d srate = %d pkt_size=%d\n",__func__, rate, uac->params.p_srate, uac->p_pktsize);
	}
}

static void __print_params(struct uac_params *params)
{
	printk(KERN_EMERG "pamrams: p_chmask=%d p_srate=%d(hz) p_ssize=%d \
			req_number=%d\n",
			params->p_chmask,
			params->p_srate,
			params->p_ssize,
			params->req_number
		);
}

/* audio function driver setup/binding */
static int f_audio_bind(struct usb_configuration *c, struct usb_function *f)
{
	struct usb_composite_dev	*cdev = c->cdev;
	struct usb_gadget		*gadget = cdev->gadget;
	struct f_uac1			*uac1 = func_to_uac1(f);
	struct uac_device *uac_dev = &uac1->uac_dev;
	struct f_uac1_opts		*audio_opts;
	struct usb_ep			*ep = NULL;
	struct usb_string		*us;
	u8				*sam_freq;
	int				rate;
	int				status;

	audio_opts = container_of(f->fi, struct f_uac1_opts, func_inst);

	us = usb_gstrings_attach(cdev, uac1_strings, ARRAY_SIZE(strings_uac1));
	if (IS_ERR(us))
		return PTR_ERR(us);

	uac_iad.iFunction = us[STR_AC_IF].id;
	ac_interface_desc.iInterface = us[STR_AC_IF].id;
	usb_out_it_desc.iTerminal = us[STR_USB_OUT_IT].id;
	usb_out_it_desc.iChannelNames = us[STR_USB_OUT_IT_CH_NAMES].id;
	io_out_ot_desc.iTerminal = us[STR_IO_OUT_OT].id;
	as_out_interface_alt_0_desc.iInterface = us[STR_AS_OUT_IF_ALT0].id;
	as_out_interface_alt_1_desc.iInterface = us[STR_AS_OUT_IF_ALT1].id;
	io_in_it_desc.iTerminal = us[STR_IO_IN_IT].id;
	io_in_it_desc.iChannelNames = us[STR_IO_IN_IT_CH_NAMES].id;
	usb_in_ot_desc.iTerminal = us[STR_USB_IN_OT].id;
	as_in_interface_alt_0_desc.iInterface = us[STR_AS_IN_IF_ALT0].id;
	as_in_interface_alt_1_desc.iInterface = us[STR_AS_IN_IF_ALT1].id;
    as_in_interface_alt_2_desc.iInterface = us[STR_AS_IN_IF_ALT2].id;

	/* Set channel numbers */
	usb_out_it_desc.bNrChannels = num_channels(audio_opts->c_chmask);
	usb_out_it_desc.wChannelConfig = cpu_to_le16(audio_opts->c_chmask);
	as_out_type_i_desc.bNrChannels = num_channels(audio_opts->c_chmask);
	as_out_type_i_desc.bSubframeSize = audio_opts->c_ssize;
	as_out_type_i_desc.bBitResolution = audio_opts->c_ssize * 8;
	io_in_it_desc.bNrChannels = num_channels(audio_opts->p_chmask);
	io_in_it_desc.wChannelConfig = cpu_to_le16(audio_opts->p_chmask);
	as_in_type_i_desc.bNrChannels = num_channels(audio_opts->p_chmask);
	as_in_type_i_desc.bSubframeSize = audio_opts->p_ssize;
	as_in_type_i_desc.bBitResolution = audio_opts->p_ssize * 8;
    as_out_ep_desc.wMaxPacketSize = 48*audio_opts->c_ssize * num_channels(audio_opts->c_chmask);
	/*FIXME: max_resolution * sample * channnel*/
    as_in_ep_desc.wMaxPacketSize = 48*audio_opts->p_ssize * num_channels(audio_opts->p_chmask);


	/* Set sample rates */
	/*rate = audio_opts->c_srate;
	  sam_freq = as_out_type_i_desc.tSamFreq[0];
	  memcpy(sam_freq, &rate, 3);
	  rate = audio_opts->p_srate;
	  sam_freq = as_in_type_i_desc.tSamFreq[0];
	  memcpy(sam_freq, &rate, 3);
	  */
	rate = 8000;
	sam_freq = as_out_type_i_desc.tSamFreq[0];
	memcpy(sam_freq, &rate, 3);
	sam_freq = as_in_type_i_desc.tSamFreq[0];
	memcpy(sam_freq, &rate, 3);
	sam_freq = as_in_type_ii_ac3_desc.tSamFreq[0];
	memcpy(sam_freq, &rate, 3);

	rate = 16000;
	sam_freq = as_out_type_i_desc.tSamFreq[1];
	memcpy(sam_freq, &rate, 3);
	sam_freq = as_in_type_i_desc.tSamFreq[1];
	memcpy(sam_freq, &rate, 3);
	sam_freq = as_in_type_ii_ac3_desc.tSamFreq[1];
	memcpy(sam_freq, &rate, 3);

	rate = 32000;
	sam_freq = as_out_type_i_desc.tSamFreq[2];
	memcpy(sam_freq, &rate, 3);
	sam_freq = as_in_type_i_desc.tSamFreq[2];
	memcpy(sam_freq, &rate, 3);
	sam_freq = as_in_type_ii_ac3_desc.tSamFreq[2];
	memcpy(sam_freq, &rate, 3);

	rate = 48000;
	sam_freq = as_out_type_i_desc.tSamFreq[3];
	memcpy(sam_freq, &rate, 3);
	sam_freq = as_in_type_i_desc.tSamFreq[3];
	memcpy(sam_freq, &rate, 3);
	sam_freq = as_in_type_ii_ac3_desc.tSamFreq[3];
	memcpy(sam_freq, &rate, 3);

	/* allocate instance-specific interface IDs, and patch descriptors */
	status = usb_interface_id(c, f);
	if (status < 0) {
		printk(KERN_EMERG "%s:%d\n", __func__, __LINE__);
		goto fail;
	}

	uac_iad.bFirstInterface = status;
	ac_interface_desc.bInterfaceNumber = status;
	uac1->ac_intf = status;
	uac1->ac_alt = 0;
	printk(KERN_EMERG "ac_intf:%d\n", status);

	status = usb_interface_id(c, f);
	if (status < 0) {
		printk(KERN_EMERG "%s:%d\n", __func__, __LINE__);
		goto fail;
	}

	as_in_interface_alt_0_desc.bInterfaceNumber = status;
	as_in_interface_alt_1_desc.bInterfaceNumber = status;
	as_in_interface_alt_2_desc.bInterfaceNumber = status;

	ac_header_desc.baInterfaceNr[0] = status;

	uac1->as_in_intf = status;
	uac1->as_in_alt = 0;

	printk(KERN_EMERG "as_in_intf:%d\n", status);

	uac_dev->gadget = gadget;

	status = -ENODEV;

	/* allocate instance-specific endpoints */
	ep = usb_ep_autoconfig(cdev->gadget, &as_out_ep_desc);
	if (!ep) {
		printk(KERN_EMERG "%s:%d\n", __func__,__LINE__);
		goto fail;
	}

	uac_dev->out_ep = ep;
	uac_dev->out_ep->desc = &as_out_ep_desc;

	ep = usb_ep_autoconfig(cdev->gadget, &as_in_ep_desc);
	if (!ep) {
		printk(KERN_EMERG "%s:%d\n", __func__,__LINE__);
		goto fail;
	}
	uac_dev->in_ep = ep;
	uac_dev->in_ep->desc = &as_in_ep_desc;

	/* copy descriptors, and track endpoint copies */
	status = usb_assign_descriptors(f, f_audio_desc, f_audio_desc, NULL);
	if (status) {
		printk(KERN_EMERG "%s:%d\n", __func__,__LINE__);
		goto fail;
	}

	uac_dev->out_ep_maxpsize = le16_to_cpu(as_out_ep_desc.wMaxPacketSize);
	uac_dev->in_ep_maxpsize = le16_to_cpu(as_in_ep_desc.wMaxPacketSize);
	uac_dev->params.c_chmask = audio_opts->c_chmask;
	uac_dev->params.c_srate = audio_opts->c_srate;
	uac_dev->params.c_ssize = audio_opts->c_ssize;
	uac_dev->params.p_chmask = audio_opts->p_chmask;
	uac_dev->params.p_srate = audio_opts->p_srate;
	uac_dev->params.p_ssize = audio_opts->p_ssize;
	uac_dev->params.req_number = audio_opts->req_number;

	__print_params(&uac_dev->params);

	status = uac_device_setup(uac_dev);
	if (status)
		goto err_card_register;

    /* Preallocate control endpoint request. */
    uac_dev->control_req = usb_ep_alloc_request(cdev->gadget->ep0, GFP_KERNEL);
    uac_dev->control_buf = kmalloc(64, GFP_KERNEL);
    if (uac_dev->control_req == NULL || uac_dev->control_buf == NULL) {
        status = -ENOMEM;
        goto error;
    }

    uac_dev->control_req->buf = uac_dev->control_buf;
    uac_dev->control_req->complete = __uac_function_ep0_complete;
    uac_dev->control_req->context = uac_dev;

	/*FIXME: add v4l2 interface*/
	{
		struct uac_device *uac_dev = &uac1->uac_dev;

		if (uac_dev) {
			if (v4l2_device_register(&cdev->gadget->dev, &(uac_dev->v4l2_dev))) {
				printk(KERN_INFO "v4l2_device_register failed\n");
				goto error;
			}

			/* Initialise queue buffer. */
			status = uac_queue_init(&(uac_dev->queue));
			if (status < 0)
				goto error;

			/* Register a V4L2 device. */
			status = __uac_register_video(cdev, uac_dev);
			if (status < 0) {
				printk(KERN_INFO "Unable to register video device\n");
				goto error;
			}
		}
	}

	return 0;

error:
	if (&uac1->uac_dev) {
		v4l2_device_unregister(&(uac1->uac_dev.v4l2_dev));
		if ((uac1->uac_dev.vdev))
			video_device_release((uac1->uac_dev.vdev));
    }

	if (&uac1->uac_dev) {
		usb_ep_free_request(cdev->gadget->ep0, uac_dev->control_req);
		kfree(uac_dev->control_buf);
	}

err_card_register:
	usb_free_all_descriptors(f);
fail:
	return status;
}

/*-------------------------------------------------------------------------*/

static inline struct f_uac1_opts *to_f_uac1_opts(struct config_item *item)
{
	return container_of(to_config_group(item), struct f_uac1_opts,
			func_inst.group);
}

static void f_audio_free_inst(struct usb_function_instance *f)
{
	struct f_uac1_opts *opts;

	opts = container_of(f, struct f_uac1_opts, func_inst);
	kfree(opts);
}

static struct usb_function_instance *f_audio_alloc_inst(void)
{
	struct f_uac1_opts *opts;

	opts = kzalloc(sizeof(*opts), GFP_KERNEL);
	if (!opts)
		return ERR_PTR(-ENOMEM);

	mutex_init(&opts->lock);
	opts->func_inst.free_func_inst = f_audio_free_inst;

	opts->c_chmask = UAC1_DEF_CCHMASK;
	opts->c_srate = UAC1_DEF_CSRATE;
	opts->c_ssize = UAC1_DEF_CSSIZE;
	opts->p_chmask = UAC1_DEF_PCHMASK;
	opts->p_srate = UAC1_DEF_PSRATE;
	opts->p_ssize = UAC1_DEF_PSSIZE;
	opts->req_number = UAC1_DEF_REQ_NUM;
	return &opts->func_inst;
}

static void f_audio_free(struct usb_function *f)
{
	struct f_uac1_opts *opts;
	struct f_uac1  *uac1 = func_to_uac1(f);

	opts = container_of(f->fi, struct f_uac1_opts, func_inst);
	mutex_lock(&opts->lock);
	--opts->refcnt;
	mutex_unlock(&opts->lock);

	kfree(uac1);
}

static void f_audio_unbind(struct usb_configuration *c, struct usb_function *f)
{
	struct f_uac1  *uac1 = func_to_uac1(f);
	struct uac_device *uac_dev = &uac1->uac_dev;
    struct usb_composite_dev *cdev = c->cdev;

	uac_device_cleanup(uac_dev);
	usb_free_all_descriptors(f);

	uac_dev->gadget = NULL;

	if (uac_dev) { /* release v4l2 device*/
		video_unregister_device(uac_dev->vdev);
		v4l2_device_unregister(&(uac_dev->v4l2_dev));

        usb_ep_free_request(cdev->gadget->ep0, uac_dev->control_req);
        kfree(uac_dev->control_buf);
	}
}

static struct usb_function *f_audio_alloc(struct usb_function_instance *fi)
{
	struct f_uac1 *uac1;
	struct f_uac1_opts *opts;

	/* allocate and initialize one new instance */
	uac1 = kzalloc(sizeof(*uac1), GFP_KERNEL);
	if (!uac1)
		return ERR_PTR(-ENOMEM);

	opts = container_of(fi, struct f_uac1_opts, func_inst);
	mutex_lock(&opts->lock);
	++opts->refcnt;
	mutex_unlock(&opts->lock);

	uac1->uac_dev.func.name = "uac1_func";
	uac1->uac_dev.func.bind = f_audio_bind;
	uac1->uac_dev.func.unbind = f_audio_unbind;
	uac1->uac_dev.func.set_alt = f_audio_set_alt;
	uac1->uac_dev.func.get_alt = f_audio_get_alt;
	uac1->uac_dev.func.setup = f_audio_setup;
	uac1->uac_dev.func.disable = f_audio_disable;
	uac1->uac_dev.func.free_func = f_audio_free;

	return &uac1->uac_dev.func;
}

DECLARE_USB_FUNCTION_INIT(uac1, f_audio_alloc_inst, f_audio_alloc);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Ruslan Bilovol");
