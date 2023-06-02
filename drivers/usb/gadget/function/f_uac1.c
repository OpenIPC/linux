// SPDX-License-Identifier: GPL-2.0+
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
 */

#include <linux/usb/audio.h>
#include <linux/module.h>

#include "u_audio.h"
#include "u_uac1.h"


struct f_uac1 {
	struct g_audio g_audio;
	u8 ac_intf, as_in_intf, as_out_intf;
	u8 ac_alt, as_in_alt, as_out_alt;	/* needed for get_alt() */

	/* Control Set command */
	struct work_struct cmd_work;
	struct list_head cs;
	u8 set_cmd, ready_cmd;
	int ready_value;
	struct usb_audio_control *set_con, *ready_con;
};

static struct f_uac1 *g_f_uac1 = NULL;
static int generic_set_cmd(struct usb_audio_control *con, u8 cmd, int value);
static int generic_get_cmd(struct usb_audio_control *con, u8 cmd);

static inline struct f_uac1 *func_to_uac1(struct usb_function *f)
{
	return container_of(f, struct f_uac1, g_audio.func);
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
#define F_AUDIO_NUM_INTERFACES		2

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
#define UAC_DT_AC_HEADER_LENGTH_1	UAC_DT_AC_HEADER_SIZE(1)
/* 1 input terminal, 1 output terminal */
#define UAC_DT_TOTAL_LENGTH_1 (UAC_DT_AC_HEADER_LENGTH_1 \
	+ UAC_DT_INPUT_TERMINAL_SIZE + UAC_DT_FEATURE_UNIT_SIZE(0) \
	+ UAC_DT_OUTPUT_TERMINAL_SIZE)
/* B.3.2  Class-Specific AC Interface Descriptor */
static struct uac1_ac_header_descriptor_1 ac_header_desc_1 = {
	.bLength =		UAC_DT_AC_HEADER_LENGTH_1,
	.bDescriptorType =	USB_DT_CS_INTERFACE,
	.bDescriptorSubtype =	UAC_HEADER,
	.bcdADC =		cpu_to_le16(0x0100),
	.wTotalLength =		cpu_to_le16(UAC_DT_TOTAL_LENGTH_1),
	.bInCollection =	1,
	.baInterfaceNr = {
	/* Interface number of the first AudioStream interface */
	/*
		[0] =		F_AUDIO_AS_OUT_INTERFACE / F_AUDIO_AS_IN_INTERFACE,
	 */
	}
};

DECLARE_UAC_AC_HEADER_DESCRIPTOR(2);
#define UAC_DT_AC_HEADER_LENGTH_2	UAC_DT_AC_HEADER_SIZE(2)
/* 2 input terminal, 2 output terminal */
#define UAC_DT_TOTAL_LENGTH_2 (UAC_DT_AC_HEADER_LENGTH_2 \
	+ 2*UAC_DT_INPUT_TERMINAL_SIZE + UAC_DT_FEATURE_UNIT_SIZE(0) \
	+ 2*UAC_DT_OUTPUT_TERMINAL_SIZE)
/* B.3.2  Class-Specific AC Interface Descriptor */
static struct uac1_ac_header_descriptor_2 ac_header_desc_2 = {
	.bLength =		UAC_DT_AC_HEADER_LENGTH_2,
	.bDescriptorType =	USB_DT_CS_INTERFACE,
	.bDescriptorSubtype =	UAC_HEADER,
	.bcdADC =		cpu_to_le16(0x0100),
	.wTotalLength =		cpu_to_le16(UAC_DT_TOTAL_LENGTH_2),
	.bInCollection =	2,
	.baInterfaceNr = {
	/* Interface number of the first AudioStream interface */
	/*
		[0] =		F_AUDIO_AS_OUT_INTERFACE,
		[1] =		F_AUDIO_AS_IN_INTERFACE,
	 */
	}
};


#define USB_OUT_IT_ID	1
#define IO_OUT_OT_ID	2
#define IO_IN_IT_ID	3
#define USB_IN_OT_ID	4
#define USB_IN_FU_ID	5

static struct uac_input_terminal_descriptor usb_out_it_desc = {
	.bLength =		UAC_DT_INPUT_TERMINAL_SIZE,
	.bDescriptorType =	USB_DT_CS_INTERFACE,
	.bDescriptorSubtype =	UAC_INPUT_TERMINAL,
	.bTerminalID =		USB_OUT_IT_ID,
	.wTerminalType =	cpu_to_le16(UAC_TERMINAL_STREAMING),
	.bAssocTerminal =	IO_OUT_OT_ID,
	.wChannelConfig =	cpu_to_le16(0x3),
};

static struct uac1_output_terminal_descriptor io_out_ot_desc = {
	.bLength		= UAC_DT_OUTPUT_TERMINAL_SIZE,
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubtype	= UAC_OUTPUT_TERMINAL,
	.bTerminalID		= IO_OUT_OT_ID,
	.wTerminalType		= cpu_to_le16(UAC_OUTPUT_TERMINAL_SPEAKER),
	.bAssocTerminal		= USB_OUT_IT_ID,
	.bSourceID		= USB_OUT_IT_ID,
};

static struct usb_audio_control playback_volume_control = {
	.list = LIST_HEAD_INIT(playback_volume_control.list),
	.name = "Playback Volume Control",
	.type = UAC_FU_VOLUME,
	/* Todo: add real Volume control code */
	.set = generic_set_cmd,
	.get = generic_get_cmd,
};

static struct usb_audio_control_selector playback_fu_controls = {
	.list = LIST_HEAD_INIT(playback_fu_controls.list),
	.name = "Playback Function Unit Controls",
};

static struct uac_input_terminal_descriptor io_in_it_desc = {
	.bLength		= UAC_DT_INPUT_TERMINAL_SIZE,
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubtype	= UAC_INPUT_TERMINAL,
	.bTerminalID		= IO_IN_IT_ID,
	.wTerminalType		= cpu_to_le16(UAC_INPUT_TERMINAL_MICROPHONE),
	.bAssocTerminal		= 0,
	.wChannelConfig		= cpu_to_le16(0x3),
};

DECLARE_UAC_FEATURE_UNIT_DESCRIPTOR(0);

static struct uac_feature_unit_descriptor_0 usb_capture_fu_desc = {
	.bLength        = UAC_DT_FEATURE_UNIT_SIZE(0),
	.bDescriptorType    = USB_DT_CS_INTERFACE,
	.bDescriptorSubtype = UAC_FEATURE_UNIT,
	.bUnitID        = USB_IN_FU_ID,
	.bSourceID      = IO_IN_IT_ID,
	.bControlSize       = 1,
	.bmaControls[0]     = (UAC_FU_VOLUME),
};

static struct uac1_output_terminal_descriptor usb_in_ot_desc = {
	.bLength =		UAC_DT_OUTPUT_TERMINAL_SIZE,
	.bDescriptorType =	USB_DT_CS_INTERFACE,
	.bDescriptorSubtype =	UAC_OUTPUT_TERMINAL,
	.bTerminalID =		USB_IN_OT_ID,
	.wTerminalType =	cpu_to_le16(UAC_TERMINAL_STREAMING),
	.bAssocTerminal =	0,
	.bSourceID =		USB_IN_FU_ID,
};

static struct usb_audio_control capture_volume_control = {
	.list = LIST_HEAD_INIT(capture_volume_control.list),
	.name = "Capture Volume Control",
	.type = UAC_FU_VOLUME,
	/* Todo: add real Volume control code */
	.set = generic_set_cmd,
	.get = generic_get_cmd,
};

static struct usb_audio_control_selector capture_fu_controls = {
	.list = LIST_HEAD_INIT(capture_fu_controls.list),
	.id   = USB_IN_FU_ID,
	.name = "Capture Mute & Volume Control",
	.desc = (struct usb_descriptor_header *)&usb_in_ot_desc,
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

/* B.4.2  Class-Specific AS Interface Descriptor */
static struct uac1_as_header_descriptor as_out_header_desc = {
	.bLength =		UAC_DT_AS_HEADER_SIZE,
	.bDescriptorType =	USB_DT_CS_INTERFACE,
	.bDescriptorSubtype =	UAC_AS_GENERAL,
	.bTerminalLink =	USB_OUT_IT_ID,
	.bDelay =		1,
	.wFormatTag =		cpu_to_le16(UAC_FORMAT_TYPE_I_PCM),
};

static struct uac1_as_header_descriptor as_in_header_desc = {
	.bLength =		UAC_DT_AS_HEADER_SIZE,
	.bDescriptorType =	USB_DT_CS_INTERFACE,
	.bDescriptorSubtype =	UAC_AS_GENERAL,
	.bTerminalLink =	USB_IN_OT_ID,
	.bDelay =		1,
	.wFormatTag =		cpu_to_le16(UAC_FORMAT_TYPE_I_PCM),
};

DECLARE_UAC_FORMAT_TYPE_I_DISCRETE_DESC(1);

static struct uac_format_type_i_discrete_descriptor_1 as_out_type_i_desc = {
	.bLength =		UAC_FORMAT_TYPE_I_DISCRETE_DESC_SIZE(1),
	.bDescriptorType =	USB_DT_CS_INTERFACE,
	.bDescriptorSubtype =	UAC_FORMAT_TYPE,
	.bFormatType =		UAC_FORMAT_TYPE_I,
	.bSubframeSize =	2,
	.bBitResolution =	16,
	.bSamFreqType =		1,
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

static struct usb_ss_ep_comp_descriptor ss_as_out_ep_comp_desc = {
	.bLength =		USB_DT_SS_EP_COMP_SIZE,
	.bDescriptorType =	USB_DT_SS_ENDPOINT_COMP,
};

/* Class-specific AS ISO OUT Endpoint Descriptor */
static struct uac_iso_endpoint_descriptor as_iso_out_desc = {
	.bLength =		UAC_ISO_ENDPOINT_DESC_SIZE,
	.bDescriptorType =	USB_DT_CS_ENDPOINT,
	.bDescriptorSubtype =	UAC_EP_GENERAL,
	.bmAttributes =		1,
	.bLockDelayUnits =	1,
	.wLockDelay =		cpu_to_le16(1),
};

static struct uac_format_type_i_discrete_descriptor_1 as_in_type_i_desc = {
	.bLength =		UAC_FORMAT_TYPE_I_DISCRETE_DESC_SIZE(1),
	.bDescriptorType =	USB_DT_CS_INTERFACE,
	.bDescriptorSubtype =	UAC_FORMAT_TYPE,
	.bFormatType =		UAC_FORMAT_TYPE_I,
	.bSubframeSize =	2,
	.bBitResolution =	16,
	.bSamFreqType =		1,
};

/* Standard ISO IN Endpoint Descriptor */
static struct usb_endpoint_descriptor as_in_ep_desc  = {
	.bLength =		USB_DT_ENDPOINT_AUDIO_SIZE,
	.bDescriptorType =	USB_DT_ENDPOINT,
	.bEndpointAddress =	USB_DIR_IN,
	.bmAttributes =		USB_ENDPOINT_SYNC_ASYNC
				| USB_ENDPOINT_XFER_ISOC,
	.wMaxPacketSize	=	cpu_to_le16(UAC1_OUT_EP_MAX_PACKET_SIZE),
	.bInterval =		4,
};

static struct usb_ss_ep_comp_descriptor ss_as_in_ep_comp_desc = {
	.bLength =		USB_DT_SS_EP_COMP_SIZE,
	.bDescriptorType =	USB_DT_SS_ENDPOINT_COMP,
};

/* Class-specific AS ISO IN Endpoint Descriptor */
static struct uac_iso_endpoint_descriptor as_iso_in_desc = {
	.bLength =		UAC_ISO_ENDPOINT_DESC_SIZE,
	.bDescriptorType =	USB_DT_CS_ENDPOINT,
	.bDescriptorSubtype =	UAC_EP_GENERAL,
	.bmAttributes =		1,
	.bLockDelayUnits =	0,
	.wLockDelay =		0,
};

static struct usb_interface_assoc_descriptor
audio_iad_descriptor = {
	.bLength		   =	sizeof(audio_iad_descriptor),
	.bDescriptorType   =	USB_DT_INTERFACE_ASSOCIATION,
	.bFirstInterface   =	0, /* updated at bind */
	.bInterfaceCount   =	3,
	.bFunctionClass    =	USB_CLASS_AUDIO,
	.bFunctionSubClass =	0,
	.bFunctionProtocol =	UAC_VERSION_1,
};

/* super-speed */
static struct usb_descriptor_header *f_audio_ss_desc_0[] = {
	(struct usb_descriptor_header *)&audio_iad_descriptor,
	(struct usb_descriptor_header *)&ac_interface_desc,
	(struct usb_descriptor_header *)&ac_header_desc_1,

	(struct usb_descriptor_header *)&usb_out_it_desc,
	(struct usb_descriptor_header *)&io_out_ot_desc,

	(struct usb_descriptor_header *)&as_out_interface_alt_0_desc,
	(struct usb_descriptor_header *)&as_out_interface_alt_1_desc,
	(struct usb_descriptor_header *)&as_out_header_desc,

	(struct usb_descriptor_header *)&as_out_type_i_desc,

	(struct usb_descriptor_header *)&as_out_ep_desc,
	(struct usb_descriptor_header *)&ss_as_out_ep_comp_desc,
	(struct usb_descriptor_header *)&as_iso_out_desc,
	NULL,
};

 static struct usb_descriptor_header *f_audio_ss_desc_1[] = {
	(struct usb_descriptor_header *)&audio_iad_descriptor,
	(struct usb_descriptor_header *)&ac_interface_desc,
	(struct usb_descriptor_header *)&ac_header_desc_1,

	(struct usb_descriptor_header *)&io_in_it_desc,
	(struct usb_descriptor_header *)&usb_capture_fu_desc,
	(struct usb_descriptor_header *)&usb_in_ot_desc,

	(struct usb_descriptor_header *)&as_in_interface_alt_0_desc,
	(struct usb_descriptor_header *)&as_in_interface_alt_1_desc,
	(struct usb_descriptor_header *)&as_in_header_desc,

	(struct usb_descriptor_header *)&as_in_type_i_desc,

	(struct usb_descriptor_header *)&as_in_ep_desc,
	(struct usb_descriptor_header *)&ss_as_in_ep_comp_desc,
	(struct usb_descriptor_header *)&as_iso_in_desc,
	NULL,
};

static struct usb_descriptor_header *f_audio_ss_desc_2[] = {
	(struct usb_descriptor_header *)&audio_iad_descriptor,
	(struct usb_descriptor_header *)&ac_interface_desc,
	(struct usb_descriptor_header *)&ac_header_desc_2,

	(struct usb_descriptor_header *)&usb_out_it_desc,
	(struct usb_descriptor_header *)&io_out_ot_desc,
	(struct usb_descriptor_header *)&io_in_it_desc,
	(struct usb_descriptor_header *)&usb_capture_fu_desc,
	(struct usb_descriptor_header *)&usb_in_ot_desc,

	(struct usb_descriptor_header *)&as_out_interface_alt_0_desc,
	(struct usb_descriptor_header *)&as_out_interface_alt_1_desc,
	(struct usb_descriptor_header *)&as_out_header_desc,

	(struct usb_descriptor_header *)&as_out_type_i_desc,

	(struct usb_descriptor_header *)&as_out_ep_desc,
	(struct usb_descriptor_header *)&ss_as_out_ep_comp_desc,
	(struct usb_descriptor_header *)&as_iso_out_desc,

	(struct usb_descriptor_header *)&as_in_interface_alt_0_desc,
	(struct usb_descriptor_header *)&as_in_interface_alt_1_desc,
	(struct usb_descriptor_header *)&as_in_header_desc,

	(struct usb_descriptor_header *)&as_in_type_i_desc,

	(struct usb_descriptor_header *)&as_in_ep_desc,
	(struct usb_descriptor_header *)&ss_as_in_ep_comp_desc,
	(struct usb_descriptor_header *)&as_iso_in_desc,
	NULL,
};

/* high-speed */
static struct usb_descriptor_header *f_audio_desc_0[] = {
	(struct usb_descriptor_header *)&audio_iad_descriptor,
	(struct usb_descriptor_header *)&ac_interface_desc,
	(struct usb_descriptor_header *)&ac_header_desc_1,

	(struct usb_descriptor_header *)&usb_out_it_desc,
	(struct usb_descriptor_header *)&io_out_ot_desc,

	(struct usb_descriptor_header *)&as_out_interface_alt_0_desc,
	(struct usb_descriptor_header *)&as_out_interface_alt_1_desc,
	(struct usb_descriptor_header *)&as_out_header_desc,

	(struct usb_descriptor_header *)&as_out_type_i_desc,

	(struct usb_descriptor_header *)&as_out_ep_desc,
	(struct usb_descriptor_header *)&as_iso_out_desc,
	NULL,
};

 static struct usb_descriptor_header *f_audio_desc_1[] = {
	(struct usb_descriptor_header *)&audio_iad_descriptor,
	(struct usb_descriptor_header *)&ac_interface_desc,
	(struct usb_descriptor_header *)&ac_header_desc_1,

	(struct usb_descriptor_header *)&io_in_it_desc,
	(struct usb_descriptor_header *)&usb_capture_fu_desc,
	(struct usb_descriptor_header *)&usb_in_ot_desc,

	(struct usb_descriptor_header *)&as_in_interface_alt_0_desc,
	(struct usb_descriptor_header *)&as_in_interface_alt_1_desc,
	(struct usb_descriptor_header *)&as_in_header_desc,

	(struct usb_descriptor_header *)&as_in_type_i_desc,

	(struct usb_descriptor_header *)&as_in_ep_desc,
	(struct usb_descriptor_header *)&as_iso_in_desc,
	NULL,
};

static struct usb_descriptor_header *f_audio_desc_2[] = {
	(struct usb_descriptor_header *)&audio_iad_descriptor,
	(struct usb_descriptor_header *)&ac_interface_desc,
	(struct usb_descriptor_header *)&ac_header_desc_2,

	(struct usb_descriptor_header *)&usb_out_it_desc,
	(struct usb_descriptor_header *)&io_out_ot_desc,
	(struct usb_descriptor_header *)&io_in_it_desc,
	(struct usb_descriptor_header *)&usb_capture_fu_desc,
	(struct usb_descriptor_header *)&usb_in_ot_desc,

	(struct usb_descriptor_header *)&as_out_interface_alt_0_desc,
	(struct usb_descriptor_header *)&as_out_interface_alt_1_desc,
	(struct usb_descriptor_header *)&as_out_header_desc,

	(struct usb_descriptor_header *)&as_out_type_i_desc,

	(struct usb_descriptor_header *)&as_out_ep_desc,
	(struct usb_descriptor_header *)&as_iso_out_desc,

	(struct usb_descriptor_header *)&as_in_interface_alt_0_desc,
	(struct usb_descriptor_header *)&as_in_interface_alt_1_desc,
	(struct usb_descriptor_header *)&as_in_header_desc,

	(struct usb_descriptor_header *)&as_in_type_i_desc,

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
};

static struct usb_string strings_uac1[] = {
	[STR_AC_IF].s = "AC Interface",
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

static void f_audio_complete(struct usb_ep *ep, struct usb_request *req)
{
	struct f_uac1 *uac1 = req->context;
	int status = req->status;
	u32 data = 0;

	switch (status) {
	case 0:				/* normal completion? */
		if (uac1->set_con) {
			memcpy(&data, req->buf, req->length);
			uac1->set_con->set(uac1->set_con, uac1->set_cmd,
					le16_to_cpu(data));
			uac1->set_con = NULL;
		}

		break;
	case -ESHUTDOWN:
	default:
		break;
	}
}

static int audio_set_intf_req(struct usb_function *f,
		const struct usb_ctrlrequest *ctrl)
{
	struct f_uac1		*uac1 = func_to_uac1(f);
	struct usb_composite_dev *cdev = f->config->cdev;
	struct usb_request	*req = cdev->req;
	u8			id = ((le16_to_cpu(ctrl->wIndex) >> 8) & 0xFF);
	u16			len = le16_to_cpu(ctrl->wLength);
	u16			w_value = le16_to_cpu(ctrl->wValue);
	u8			con_sel = (w_value >> 8) & 0xFF;
	u8			cmd = (ctrl->bRequest & 0x0F);
	struct usb_audio_control_selector *cs;
	struct usb_audio_control *con;

	DBG(cdev, "bRequest 0x%x, w_value 0x%04x, len %d, entity %d\n",
			ctrl->bRequest, w_value, len, id);

	list_for_each_entry(cs, &uac1->cs, list) {
		if (cs->id == id) {
			list_for_each_entry(con, &cs->control, list) {
				if (con->type == con_sel) {
					uac1->set_con = con;
					break;
				}
			}
			break;
		}
	}

	uac1->set_cmd = cmd;
	req->context = uac1;
	req->complete = f_audio_complete;

	return len;
}

static int audio_get_intf_req(struct usb_function *f,
		const struct usb_ctrlrequest *ctrl)
{
	struct f_uac1	*uac1 = func_to_uac1(f);
	struct usb_composite_dev *cdev = f->config->cdev;
	struct usb_request	*req = cdev->req;
	int			value = -EOPNOTSUPP;
	u8			id = ((le16_to_cpu(ctrl->wIndex) >> 8) & 0xFF);
	u16			len = le16_to_cpu(ctrl->wLength);
	u16			w_value = le16_to_cpu(ctrl->wValue);
	u8			con_sel = (w_value >> 8) & 0xFF;
	u8			cmd = (ctrl->bRequest & 0x0F);
	struct usb_audio_control_selector *cs;
	struct usb_audio_control *con;

	DBG(cdev, "bRequest 0x%x, w_value 0x%04x, len %d, entity %d\n",
			ctrl->bRequest, w_value, len, id);

	list_for_each_entry(cs, &uac1->cs, list) {
		if (cs->id == id) {
			list_for_each_entry(con, &cs->control, list) {
				if (con->type == con_sel && con->get) {
					value = con->get(con, cmd);
					break;
				}
			}
			break;
		}
	}

	req->context = uac1;
	req->complete = f_audio_complete;
	len = min_t(size_t, sizeof(value), len);
	memcpy(req->buf, &value, len);

	return len;
}

static int audio_set_endpoint_req(struct usb_function *f,
		const struct usb_ctrlrequest *ctrl)
{
	struct usb_composite_dev *cdev = f->config->cdev;
	int			value = -EOPNOTSUPP;
	u16			ep = le16_to_cpu(ctrl->wIndex);
	u16			len = le16_to_cpu(ctrl->wLength);
	u16			w_value = le16_to_cpu(ctrl->wValue);

	DBG(cdev, "bRequest 0x%x, w_value 0x%04x, len %d, endpoint %d\n",
			ctrl->bRequest, w_value, len, ep);

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
	int value = -EOPNOTSUPP;
	u8 ep = ((le16_to_cpu(ctrl->wIndex) >> 8) & 0xFF);
	u16 len = le16_to_cpu(ctrl->wLength);
	u16 w_value = le16_to_cpu(ctrl->wValue);

	DBG(cdev, "bRequest 0x%x, w_value 0x%04x, len %d, endpoint %d\n",
			ctrl->bRequest, w_value, len, ep);

	switch (ctrl->bRequest) {
	case UAC_GET_CUR:
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
	struct usb_request	*req = cdev->req;
	int			value = -EOPNOTSUPP;
	u16			w_index = le16_to_cpu(ctrl->wIndex);
	u16			w_value = le16_to_cpu(ctrl->wValue);
	u16			w_length = le16_to_cpu(ctrl->wLength);

	/* composite driver infrastructure handles everything; interface
	 * activation uses set_alt().
	 */
	switch (ctrl->bRequestType) {
	case USB_DIR_OUT | USB_TYPE_CLASS | USB_RECIP_ENDPOINT:
		value = audio_set_endpoint_req(f, ctrl);
		break;

	case USB_DIR_IN | USB_TYPE_CLASS | USB_RECIP_ENDPOINT:
		value = audio_get_endpoint_req(f, ctrl);
		break;

	case USB_DIR_OUT | USB_TYPE_CLASS | USB_RECIP_INTERFACE:
		value = audio_set_intf_req(f, ctrl);
		break;

	case USB_DIR_IN | USB_TYPE_CLASS | USB_RECIP_INTERFACE:
		value = audio_get_intf_req(f, ctrl);
		break;

	default:
		ERROR(cdev, "invalid control req%02x.%02x v%04x i%04x l%d\n",
			ctrl->bRequestType, ctrl->bRequest,
			w_value, w_index, w_length);
	}

	/* respond with data transfer or status phase? */
	if (value >= 0) {
		DBG(cdev, "audio req%02x.%02x v%04x i%04x l%d\n",
			ctrl->bRequestType, ctrl->bRequest,
			w_value, w_index, w_length);
		req->zero = 0;
		req->length = value;
		value = usb_ep_queue(cdev->gadget->ep0, req, GFP_ATOMIC);
		if (value < 0)
			ERROR(cdev, "audio response on err %d\n", value);
	}

	/* device either stalls (value < 0) or reports success */
	return value;
}

static int f_audio_set_alt(struct usb_function *f, unsigned intf, unsigned alt)
{
	struct usb_composite_dev *cdev = f->config->cdev;
	struct usb_gadget *gadget = cdev->gadget;
	struct device *dev = &gadget->dev;
	struct f_uac1 *uac1 = func_to_uac1(f);
	int ret = 0;

	/* No i/f has more than 2 alt settings */
	if (alt > 1) {
		dev_err(dev, "%s:%d Error!\n", __func__, __LINE__);
		return -EINVAL;
	}

	if (intf == uac1->ac_intf) {
		/* Control I/f has only 1 AltSetting - 0 */
		if (alt) {
			dev_err(dev, "%s:%d Error!\n", __func__, __LINE__);
			return -EINVAL;
		}
		return 0;
	}

	if (intf == uac1->as_out_intf) {
		uac1->as_out_alt = alt;

		if (alt)
			ret = u_audio_start_capture(&uac1->g_audio);
		else
			u_audio_stop_capture(&uac1->g_audio);
	} else if (intf == uac1->as_in_intf) {
		uac1->as_in_alt = alt;

		if (alt)
			ret = u_audio_start_playback(&uac1->g_audio);
		else
			u_audio_stop_playback(&uac1->g_audio);
	} else {
		dev_err(dev, "%s:%d Error!\n", __func__, __LINE__);
		return -EINVAL;
	}

	return ret;
}

static int f_audio_get_alt(struct usb_function *f, unsigned intf)
{
	struct usb_composite_dev *cdev = f->config->cdev;
	struct usb_gadget *gadget = cdev->gadget;
	struct device *dev = &gadget->dev;
	struct f_uac1 *uac1 = func_to_uac1(f);

	if (intf == uac1->ac_intf)
		return uac1->ac_alt;
	else if (intf == uac1->as_out_intf)
		return uac1->as_out_alt;
	else if (intf == uac1->as_in_intf)
		return uac1->as_in_alt;
	else
		dev_err(dev, "%s:%d Invalid Interface %d!\n",
			__func__, __LINE__, intf);

	return -EINVAL;
}


static void f_audio_disable(struct usb_function *f)
{
	struct f_uac1 *uac1 = func_to_uac1(f);

	uac1->as_out_alt = 0;
	uac1->as_in_alt = 0;

	u_audio_stop_capture(&uac1->g_audio);
}

/*-------------------------------------------------------------------------*/

/* audio function driver setup/binding */
static int f_audio_bind(struct usb_configuration *c, struct usb_function *f)
{
	static struct usb_descriptor_header **f_audio_desc = NULL;
	static struct usb_descriptor_header **f_audio_ss_desc = NULL;	//super-speed
	struct usb_composite_dev	*cdev = c->cdev;
	struct usb_gadget		*gadget = cdev->gadget;
	struct f_uac1			*uac1 = func_to_uac1(f);
	struct g_audio			*audio = func_to_g_audio(f);
	struct f_uac1_opts		*audio_opts;
	struct usb_ep			*ep = NULL;
	struct usb_string		*us;
	u8				*sam_freq;
	int				rate;
	int				status;

	audio_opts = container_of(f->fi, struct f_uac1_opts, func_inst);

	/* Sanity check the streaming endpoint module parameters.
	 */
	audio_opts->c_mpsize = audio_opts->c_mpsize > 1024?1024:audio_opts->c_mpsize;
	audio_opts->p_mpsize = audio_opts->p_mpsize > 1024?1024:audio_opts->p_mpsize;

	as_out_ep_desc.wMaxPacketSize = cpu_to_le16(audio_opts->c_mpsize);
	as_in_ep_desc.wMaxPacketSize = cpu_to_le16(audio_opts->p_mpsize);

	/* super-speed */
  	if(gadget_is_superspeed(gadget))
	{
		ss_as_out_ep_comp_desc.bmAttributes = cpu_to_le16(0);
		ss_as_out_ep_comp_desc.bMaxBurst = cpu_to_le16(0);
		ss_as_out_ep_comp_desc.wBytesPerInterval = cpu_to_le16(as_out_ep_desc.wMaxPacketSize);
		ss_as_in_ep_comp_desc.bmAttributes = cpu_to_le16(0);
		ss_as_in_ep_comp_desc.bMaxBurst = cpu_to_le16(0);
		ss_as_in_ep_comp_desc.wBytesPerInterval = cpu_to_le16(as_in_ep_desc.wMaxPacketSize);
	}

	us = usb_gstrings_attach(cdev, uac1_strings, ARRAY_SIZE(strings_uac1));
	if (IS_ERR(us))
		return PTR_ERR(us);
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

	/* Set sample rates */
	rate = audio_opts->c_srate;
	sam_freq = as_out_type_i_desc.tSamFreq[0];
	memcpy(sam_freq, &rate, 3);
	rate = audio_opts->p_srate;
	sam_freq = as_in_type_i_desc.tSamFreq[0];
	memcpy(sam_freq, &rate, 3);

	/* allocate instance-specific interface IDs, and patch descriptors */
	status = usb_interface_id(c, f);
	if (status < 0)
		goto fail;
	ac_interface_desc.bInterfaceNumber = status;
	audio_iad_descriptor.bFirstInterface = status;
	uac1->ac_intf = status;
	uac1->ac_alt = 0;

	if(ENABLE_SPEAKER == audio_opts->audio_play_mode)
	{
		status = usb_interface_id(c, f);
		if (status < 0)
			goto fail;
		as_out_interface_alt_0_desc.bInterfaceNumber = status;
		as_out_interface_alt_1_desc.bInterfaceNumber = status;
		ac_header_desc_1.baInterfaceNr[0] = status;
		uac1->as_out_intf = status;
		uac1->as_out_alt = 0;
	}

	if(ENABLE_MICROPHONE == audio_opts->audio_play_mode)
	{
		status = usb_interface_id(c, f);
		if (status < 0)
			goto fail;
		as_in_interface_alt_0_desc.bInterfaceNumber = status;
		as_in_interface_alt_1_desc.bInterfaceNumber = status;
		ac_header_desc_1.baInterfaceNr[0] = status;
		uac1->as_in_intf = status;
		uac1->as_in_alt = 0;
	}

	if(ENABLE_MIC_AND_SPK == audio_opts->audio_play_mode)
	{
		status = usb_interface_id(c, f);
		if (status < 0)
			goto fail;
		as_out_interface_alt_0_desc.bInterfaceNumber = status;
		as_out_interface_alt_1_desc.bInterfaceNumber = status;
		ac_header_desc_2.baInterfaceNr[0] = status;
		uac1->as_out_intf = status;
		uac1->as_out_alt = 0;

		status = usb_interface_id(c, f);
		if (status < 0)
			goto fail;
		as_in_interface_alt_0_desc.bInterfaceNumber = status;
		as_in_interface_alt_1_desc.bInterfaceNumber = status;
		ac_header_desc_2.baInterfaceNr[1] = status;
		uac1->as_in_intf = status;
		uac1->as_in_alt = 0;
	}

	audio->gadget = gadget;

	status = -ENODEV;

	if(gadget_is_superspeed(gadget))
	{
		/* allocate instance-specific endpoints */
		if(ENABLE_SPEAKER == audio_opts->audio_play_mode ||
		  ENABLE_MIC_AND_SPK == audio_opts->audio_play_mode )
		{
			ep = usb_ep_autoconfig_ss(cdev->gadget, &as_out_ep_desc, &ss_as_in_ep_comp_desc);
			if (!ep)
				goto fail;
			audio->out_ep = ep;
			audio->out_ep->desc = &as_out_ep_desc;
			audio->out_ep->comp_desc = &ss_as_out_ep_comp_desc;
		}

		if(ENABLE_MICROPHONE == audio_opts->audio_play_mode ||
		   ENABLE_MIC_AND_SPK == audio_opts->audio_play_mode )
		{
			ep = usb_ep_autoconfig_ss(cdev->gadget, &as_in_ep_desc, &ss_as_in_ep_comp_desc);
			if (!ep)
				goto fail;
			audio->in_ep = ep;
			audio->in_ep->desc = &as_in_ep_desc;
			audio->in_ep->comp_desc = &ss_as_in_ep_comp_desc;
		}
	}
	else
	{
		/* allocate instance-specific endpoints */
		if(ENABLE_SPEAKER == audio_opts->audio_play_mode ||
		  ENABLE_MIC_AND_SPK == audio_opts->audio_play_mode )
		{
			ep = usb_ep_autoconfig(cdev->gadget, &as_out_ep_desc);
			if (!ep)
				goto fail;
			audio->out_ep = ep;
			audio->out_ep->desc = &as_out_ep_desc;
		}

		if(ENABLE_MICROPHONE == audio_opts->audio_play_mode ||
		   ENABLE_MIC_AND_SPK == audio_opts->audio_play_mode )
		{
			ep = usb_ep_autoconfig(cdev->gadget, &as_in_ep_desc);
			if (!ep)
				goto fail;
			audio->in_ep = ep;
			audio->in_ep->desc = &as_in_ep_desc;
		}
	}


	if(ENABLE_MICROPHONE == audio_opts->audio_play_mode)
	{
		audio_iad_descriptor.bInterfaceCount = 2;

		if(gadget_is_superspeed(gadget))
			f_audio_ss_desc = f_audio_ss_desc_1;

		f_audio_desc = f_audio_desc_1;

	}
	else if(ENABLE_MIC_AND_SPK == audio_opts->audio_play_mode)
	{
		audio_iad_descriptor.bInterfaceCount = 3;

		if(gadget_is_superspeed(gadget))
			f_audio_ss_desc = f_audio_ss_desc_2;

		f_audio_desc = f_audio_desc_2;

	}
	else if (ENABLE_SPEAKER == audio_opts->audio_play_mode)
	{
		audio_iad_descriptor.bInterfaceCount = 2;

		if(gadget_is_superspeed(gadget))
			f_audio_ss_desc = f_audio_ss_desc_0;

		f_audio_desc = f_audio_desc_0;

	}

	/* copy descriptors, and track endpoint copies */
	status = usb_assign_descriptors(f, f_audio_desc, f_audio_desc, f_audio_ss_desc,
					NULL);
	if (status)
		goto fail;

	audio->out_ep_maxpsize = le16_to_cpu(as_out_ep_desc.wMaxPacketSize);
	audio->in_ep_maxpsize = le16_to_cpu(as_in_ep_desc.wMaxPacketSize);

	audio->params.c_chmask = audio_opts->c_chmask;
	audio->params.c_srate = audio_opts->c_srate;
	audio->params.c_ssize = audio_opts->c_ssize;
	audio->params.p_chmask = audio_opts->p_chmask;
	audio->params.p_srate = audio_opts->p_srate;
	audio->params.p_ssize = audio_opts->p_ssize;
	audio->params.req_number = audio_opts->req_number;

	status = g_audio_setup(audio, "UAC1_PCM", "UAC1_Gadget");
	if (status)
		goto err_card_register;

	return 0;

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

static void f_uac1_attr_release(struct config_item *item)
{
	struct f_uac1_opts *opts = to_f_uac1_opts(item);

	usb_put_function_instance(&opts->func_inst);
}

static struct configfs_item_operations f_uac1_item_ops = {
	.release	= f_uac1_attr_release,
};

#define UAC1_ATTRIBUTE(name)						\
static ssize_t f_uac1_opts_##name##_show(				\
					  struct config_item *item,	\
					  char *page)			\
{									\
	struct f_uac1_opts *opts = to_f_uac1_opts(item);		\
	int result;							\
									\
	mutex_lock(&opts->lock);					\
	result = sprintf(page, "%u\n", opts->name);			\
	mutex_unlock(&opts->lock);					\
									\
	return result;							\
}									\
									\
static ssize_t f_uac1_opts_##name##_store(				\
					  struct config_item *item,	\
					  const char *page, size_t len)	\
{									\
	struct f_uac1_opts *opts = to_f_uac1_opts(item);		\
	int ret;							\
	u32 num;							\
									\
	mutex_lock(&opts->lock);					\
	if (opts->refcnt) {						\
		ret = -EBUSY;						\
		goto end;						\
	}								\
									\
	ret = kstrtou32(page, 0, &num);					\
	if (ret)							\
		goto end;						\
									\
	opts->name = num;						\
	ret = len;							\
									\
end:									\
	mutex_unlock(&opts->lock);					\
	return ret;							\
}									\
									\
CONFIGFS_ATTR(f_uac1_opts_, name)

UAC1_ATTRIBUTE(c_mpsize);
UAC1_ATTRIBUTE(c_chmask);
UAC1_ATTRIBUTE(c_srate);
UAC1_ATTRIBUTE(c_ssize);
UAC1_ATTRIBUTE(p_mpsize);
UAC1_ATTRIBUTE(p_chmask);
UAC1_ATTRIBUTE(p_srate);
UAC1_ATTRIBUTE(p_ssize);
UAC1_ATTRIBUTE(req_number);

static struct configfs_attribute *f_uac1_attrs[] = {
	&f_uac1_opts_attr_c_mpsize,
	&f_uac1_opts_attr_c_chmask,
	&f_uac1_opts_attr_c_srate,
	&f_uac1_opts_attr_c_ssize,
	&f_uac1_opts_attr_p_mpsize,
	&f_uac1_opts_attr_p_chmask,
	&f_uac1_opts_attr_p_srate,
	&f_uac1_opts_attr_p_ssize,
	&f_uac1_opts_attr_req_number,
	NULL,
};

static struct config_item_type f_uac1_func_type = {
	.ct_item_ops	= &f_uac1_item_ops,
	.ct_attrs	= f_uac1_attrs,
	.ct_owner	= THIS_MODULE,
};

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

	config_group_init_type_name(&opts->func_inst.group, "",
				    &f_uac1_func_type);

	opts->c_mpsize = UAC1_OUT_EP_MAX_PACKET_SIZE;
	opts->c_chmask = UAC1_DEF_CCHMASK;
	opts->c_srate = UAC1_DEF_CSRATE;
	opts->c_ssize = UAC1_DEF_CSSIZE;
	opts->p_mpsize = UAC1_OUT_EP_MAX_PACKET_SIZE;
	opts->p_chmask = UAC1_DEF_PCHMASK;
	opts->p_srate = UAC1_DEF_PSRATE;
	opts->p_ssize = UAC1_DEF_PSSIZE;
	opts->req_number = UAC1_DEF_REQ_NUM;

	return &opts->func_inst;
}

static void f_audio_free(struct usb_function *f)
{
	struct g_audio *audio;
	struct f_uac1_opts *opts;

	struct f_uac1 *uac1 = func_to_uac1(f);
	if (g_f_uac1 !=uac1)
		return;

	audio = func_to_g_audio(f);
	opts = container_of(f->fi, struct f_uac1_opts, func_inst);
	kfree(audio);

	uac1 = NULL;

	mutex_lock(&opts->lock);
	--opts->refcnt;
	mutex_unlock(&opts->lock);
}

static void f_audio_unbind(struct usb_configuration *c, struct usb_function *f)
{
	struct g_audio *audio = func_to_g_audio(f);

	g_audio_cleanup(audio);
	usb_free_all_descriptors(f);

	audio->gadget = NULL;
}

static void mixer_cmd_work(struct work_struct *data)
{
	struct f_uac1 *uac1 = g_f_uac1;
	struct usb_audio_control *con = uac1->ready_con;
	int value = uac1->ready_value;

	if (!con)
		return;

	switch (con->type)
	{
		case UAC_FU_VOLUME: // actual db / CAPTURE_VOLUME_STEP
			uac1->g_audio.volume = UAC_VOLUME_ATTR_TO_DB(value);
			g_audio_notify(&uac1->g_audio);
			break;
		default:
			break;
	}

	uac1->ready_con = NULL;
}

static int generic_set_cmd(struct usb_audio_control *con, u8 cmd, int value)
{
	struct f_uac1 *uac1 = g_f_uac1;
	if (!uac1)
		return -EINVAL;

	if (uac1->ready_con)
		return -EINVAL;

	uac1->ready_con = con;
	uac1->ready_cmd = cmd;
	uac1->ready_value = value;

	con->data[cmd] = value;
	schedule_work(&uac1->cmd_work);
	return 0;
}

static int generic_get_cmd(struct usb_audio_control *con, u8 cmd)
{
	return con->data[cmd];
}

static int control_selector_init(struct f_uac1 *uac1)
{
	INIT_LIST_HEAD(&uac1->cs);
	list_add(&capture_fu_controls.list,&uac1->cs);
	list_add(&playback_fu_controls.list,&uac1->cs);

	INIT_LIST_HEAD(&capture_fu_controls.control);
	list_add(&capture_volume_control.list,
			 &capture_fu_controls.control);

	INIT_LIST_HEAD(&playback_fu_controls.control);
	list_add(&playback_volume_control.list,
		 &playback_fu_controls.control);

	capture_volume_control.data[UAC__CUR] = DB_TO_UAC_VOLUME_ATTR(CAPTURE_VOLUME_CUR);
	capture_volume_control.data[UAC__MIN] = DB_TO_UAC_VOLUME_ATTR(CAPTURE_VOLUME_MIN);
	capture_volume_control.data[UAC__MAX] = DB_TO_UAC_VOLUME_ATTR(CAPTURE_VOLUME_MAX);
	capture_volume_control.data[UAC__RES] = DB_TO_UAC_VOLUME_ATTR(CAPTURE_VOLUME_STEP);
	playback_volume_control.data[UAC__CUR] = 0xffc0;
	playback_volume_control.data[UAC__MIN] = 0xe3a0;
	playback_volume_control.data[UAC__MAX] = 0xfff0;
	playback_volume_control.data[UAC__RES] = 0x0030;

	return 0;
}

static struct usb_function *f_audio_alloc(struct usb_function_instance *fi)
{
	struct f_uac1 *uac1;
	struct f_uac1_opts *opts;

	if (g_f_uac1)
		return ERR_PTR(-EAGAIN);

	/* allocate and initialize one new instance */
	uac1 = kzalloc(sizeof(*uac1), GFP_KERNEL);
	if (!uac1)
		return ERR_PTR(-ENOMEM);

	opts = container_of(fi, struct f_uac1_opts, func_inst);
	mutex_lock(&opts->lock);
	++opts->refcnt;
	mutex_unlock(&opts->lock);

	uac1->g_audio.func.name = "uac1_func";
	uac1->g_audio.func.bind = f_audio_bind;
	uac1->g_audio.func.unbind = f_audio_unbind;
	uac1->g_audio.func.set_alt = f_audio_set_alt;
	uac1->g_audio.func.get_alt = f_audio_get_alt;
	uac1->g_audio.func.setup = f_audio_setup;
	uac1->g_audio.func.disable = f_audio_disable;
	uac1->g_audio.func.free_func = f_audio_free;

	control_selector_init(uac1);
	INIT_WORK(&uac1->cmd_work, mixer_cmd_work);
	g_f_uac1 = uac1;

	return &uac1->g_audio.func;
}

DECLARE_USB_FUNCTION_INIT(uac1, f_audio_alloc_inst, f_audio_alloc);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Ruslan Bilovol");
