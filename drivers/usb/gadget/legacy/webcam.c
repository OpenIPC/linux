/*
 *	webcam.c -- USB webcam gadget driver
 *
 *	Copyright (C) 2009-2010
 *		Laurent Pinchart (laurent.pinchart@ideasonboard.com)
 *
 *	This program is free software; you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation; either version 2 of the License, or
 *	(at your option) any later version.
 */

#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/module.h>
#include <linux/usb/video.h>
#include <linux/usb/composite.h>

USB_GADGET_COMPOSITE_OPTIONS();
/*-------------------------------------------------------------------------*/

#if defined(CONFIG_USB_WEBCAM_UVC)
#include "u_uvc.h"
#include "uvc_ait_xu.h"

/* module parameters specific to the Video streaming endpoint */
static unsigned int nr_name;
static char *streaming_name[MAX_STREAM_SUPPORT] =
			{DEFAULT_STREAM_NAME, DEFAULT_STREAM_NAME, DEFAULT_STREAM_NAME, DEFAULT_STREAM_NAME};
module_param_array(streaming_name, charp, &nr_name, S_IRUGO|S_IWUSR);
MODULE_PARM_DESC(streaming_name, "Uvc Stream Name");

static unsigned int nr_interval;
static unsigned int streaming_interval[MAX_STREAM_SUPPORT] = {1, 1, 1, 1};
module_param_array(streaming_interval, uint, &nr_interval, S_IRUGO|S_IWUSR);
MODULE_PARM_DESC(streaming_interval, "1 - 16");

static unsigned int nr_maxpacket;
static unsigned int streaming_maxpacket[MAX_STREAM_SUPPORT] = {1024 * 3, 1024 * 3, 30192, 30064};
module_param_array(streaming_maxpacket, uint, &nr_maxpacket, S_IRUGO|S_IWUSR);
MODULE_PARM_DESC(streaming_maxpacket, "ISOC: 1 - 1023 (FS), 1 - 3072 (hs/ss) / "
		"BULK: 1 - 64 (FS), 1 - 512 (HS), 1 - 1024 (SS)");

static unsigned int nr_maxburst;
static unsigned int streaming_maxburst[MAX_STREAM_SUPPORT] = {0, 0, 0, 0};
module_param_array(streaming_maxburst, uint, &nr_maxburst, S_IRUGO|S_IWUSR);
MODULE_PARM_DESC(streaming_maxburst, "0 - 15 (ss only)");

static bool bulk_streaming_ep = 0;
module_param(bulk_streaming_ep, bool, S_IRUGO|S_IWUSR);
MODULE_PARM_DESC(bulk_streaming_ep, "0 (Use ISOC video streaming ep) / "
		"1 (Use BULK video streaming ep)");

static int uvc_function_enable	= 1;
module_param(uvc_function_enable, int, S_IRUGO|S_IWUSR);
MODULE_PARM_DESC(uvc_function_enable, "UVC Function Enable,"
				"0: Disable UVC, 1: Enable UVC");
static unsigned int trace;
module_param(trace, uint, S_IRUGO|S_IWUSR);
MODULE_PARM_DESC(trace, "Trace level bitmask");
#endif

#if defined(CONFIG_USB_WEBCAM_UAC)
#ifndef CONFIG_USB_WEBCAM_UAC1_LEGACY
#include "u_uac1.h"

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

static int uac_function_enable	= 0;
module_param(uac_function_enable, int, S_IRUGO|S_IWUSR);
MODULE_PARM_DESC(uac_function_enable, "Audio Play Mode, 0: Disable UAC Function, "
					  "1: Speaker Only, 2: Microphone Only, 3: Speaker & Microphone");
#else /* CONFIG_USB_WEBCAM_UAC1_LEGACY */
#include "u_uac1_legacy.h"

static char *fn_play = FILE_PCM_PLAYBACK;
module_param(fn_play, charp, S_IRUGO);
MODULE_PARM_DESC(fn_play, "Playback PCM device file name");

static char *fn_cap = FILE_PCM_CAPTURE;
module_param(fn_cap, charp, S_IRUGO);
MODULE_PARM_DESC(fn_cap, "Capture PCM device file name");

static char *fn_cntl = FILE_CONTROL;
module_param(fn_cntl, charp, S_IRUGO);
MODULE_PARM_DESC(fn_cntl, "Control device file name");

static int playback_channel_count = UAC1_PLAYBACK_CHANNEL_COUNT;
module_param(playback_channel_count, int, S_IRUGO|S_IWUSR);
MODULE_PARM_DESC(playback_channel_count, "Speaker Channel Counts");

static int capture_channel_count = UAC1_CAPTURE_CHANNEL_COUNT;
module_param(capture_channel_count, int, S_IRUGO|S_IWUSR);
MODULE_PARM_DESC(capture_channel_count, "Microphone Channel Counts");

static int playback_sample_rate = UAC1_PLAYBACK_SAMPLE_RATE;
module_param(playback_sample_rate, int, S_IRUGO|S_IWUSR);
MODULE_PARM_DESC(playback_sample_rate, "Speaker Sample Rate");

static int capture_sample_rate = UAC1_CAPTURE_SAMPLE_RATE;
module_param(capture_sample_rate, int, S_IRUGO|S_IWUSR);
MODULE_PARM_DESC(capture_sample_rate, "Microphone Sample Rate");

static int out_req_buf_size = UAC1_OUT_EP_MAX_PACKET_SIZE;
module_param(out_req_buf_size, int, S_IRUGO|S_IWUSR);
MODULE_PARM_DESC(out_req_buf_size, "ISO OUT endpoint request buffer size");

static int in_req_buf_size = UAC1_IN_EP_MAX_PACKET_SIZE;
module_param(in_req_buf_size, int, S_IRUGO|S_IWUSR);
MODULE_PARM_DESC(in_req_buf_size, "ISO IN endpoint request buffer size");

static int out_req_count = UAC1_OUT_REQ_COUNT;
module_param(out_req_count, int, S_IRUGO|S_IWUSR);
MODULE_PARM_DESC(out_req_count, "ISO OUT endpoint request count");

static int in_req_count = UAC1_IN_REQ_COUNT;
module_param(in_req_count, int, S_IRUGO|S_IWUSR);
MODULE_PARM_DESC(in_req_count, "ISO IN endpoint request count");

static int audio_playback_buf_size = UAC1_AUDIO_PLAYBACK_BUF_SIZE;
module_param(audio_playback_buf_size, int, S_IRUGO|S_IWUSR);
MODULE_PARM_DESC(audio_playback_buf_size, "Audio playback buffer size");

static int audio_capture_period_size = UAC1_AUDIO_PTN_PER_FRAME_SIZE;
module_param(audio_capture_period_size, int, S_IRUGO|S_IWUSR);
MODULE_PARM_DESC(audio_capture_period_size, "Audio capture PtNumPerFrm size");

static int audio_capture_buf_size = UAC1_AUDIO_CAPTURE_BUF_SIZE;
module_param(audio_capture_buf_size, int, S_IRUGO|S_IWUSR);
MODULE_PARM_DESC(audio_capture_buf_size, "Audio capture buffer size");

static int uac_function_enable	= 0;
module_param(uac_function_enable, int, S_IRUGO|S_IWUSR);
MODULE_PARM_DESC(uac_function_enable, "Audio Play Mode, 0: Disable UAC Function, "
					  "1: Speaker Only, 2: Microphone Only, 3: Speaker & Microphone");
#endif
#endif

#if defined(CONFIG_USB_WEBCAM_RNDIS)
#include "u_ether.h"
#include "u_gether.h"
#include "u_rndis.h"
#include "rndis.h"

USB_ETHERNET_MODULE_PARAMETERS();

static int rndis_function_enable	= 0;
module_param(rndis_function_enable, int, S_IRUGO|S_IWUSR);
MODULE_PARM_DESC(rndis_function_enable, "RNDIS Function Enable");
#endif

#if defined(CONFIG_USB_WEBCAM_DFU)
#include "f_dfu.h"

static int dfu_function_enable	= 0;
module_param(dfu_function_enable, int, S_IRUGO|S_IWUSR);
MODULE_PARM_DESC(dfu_function_enable, "0: Disable 1: Enable");
#endif

/* --------------------------------------------------------------------------
 * Customer Define
 */
#ifdef USE_AIT_XU
#define UVC_EU1_GUID UVC_AIT_EU1_GUID
#define UVC_EU2_GUID UVC_CUS_EU2_GUID
#else
#define UVC_EU1_GUID UVC_AIT_EU1_GUID
#define UVC_EU2_GUID UVC_CUS_EU2_GUID
#endif
// termail link:
//	 UVC_IT_ID -> UVC_PU_ID -> UVC_EU1_ID -> UVC_EU2_ID -> UVC_OT_ID;

#define UVC_EU1_ID	(0x6) //for Isp use
#define UVC_EU2_ID	(0x2) //for customer to use
#define UVC_PU_ID	(0x3)
#define UVC_IT_ID	(0x1)
#define UVC_OT_ID	(0x7)

/* --------------------------------------------------------------------------
 * Device descriptor
 */
#define WEBCAM_VENDOR_ID		0x1d6b	/* Linux Foundation */
#define WEBCAM_PRODUCT_ID		0x0102	/* Webcam A/V gadget */
#define WEBCAM_DEVICE_BCD		0x0010	/* 0.10 */

static char webcam_vendor_label[] = "Linux Foundation";
static char webcam_product_label[] = "Webcam gadget";
static char webcam_config_label[] = "Video";

/* string IDs are assigned dynamically */

#define STRING_DESCRIPTION_IDX		USB_GADGET_FIRST_AVAIL_IDX

static struct usb_string webcam_strings[] = {
	[USB_GADGET_MANUFACTURER_IDX].s = webcam_vendor_label,
	[USB_GADGET_PRODUCT_IDX].s = webcam_product_label,
	[USB_GADGET_SERIAL_IDX].s = "",
	[STRING_DESCRIPTION_IDX].s = webcam_config_label,
	{  }
};

static struct usb_gadget_strings webcam_stringtab = {
	.language = 0x0409, /* en-us */
	.strings = webcam_strings,
};

static struct usb_gadget_strings *webcam_device_strings[] = {
	&webcam_stringtab,
	NULL,
};

#if defined(CONFIG_USB_WEBCAM_UVC)
static struct usb_function_instance **fi_uvc;
static struct usb_function **f_uvc;
#endif

#if defined(CONFIG_USB_WEBCAM_UAC)
static struct usb_function_instance *fi_uac1;
static struct usb_function *f_uac1;
#endif

#if defined(CONFIG_USB_WEBCAM_RNDIS) // need to be the first interface
static struct usb_function_instance *fi_rndis;
static struct usb_function *f_rndis;
#endif

#if defined(CONFIG_USB_WEBCAM_DFU)
static struct usb_function_instance *fi_dfu;
static struct usb_function *f_dfu;
#endif

static struct usb_device_descriptor webcam_device_descriptor = {
	.bLength		= USB_DT_DEVICE_SIZE,
	.bDescriptorType	= USB_DT_DEVICE,
	/* .bcdUSB = DYNAMIC */
	.bDeviceClass		= USB_CLASS_MISC,
	.bDeviceSubClass	= 0x02,
	.bDeviceProtocol	= 0x01,
	.bMaxPacketSize0	= 0, /* dynamic */
	.idVendor		= cpu_to_le16(WEBCAM_VENDOR_ID),
	.idProduct		= cpu_to_le16(WEBCAM_PRODUCT_ID),
	.bcdDevice		= cpu_to_le16(WEBCAM_DEVICE_BCD),
	.iManufacturer		= 0, /* dynamic */
	.iProduct		= 0, /* dynamic */
	.iSerialNumber		= 0, /* dynamic */
	.bNumConfigurations = 0, /* dynamic */
};

#if defined(CONFIG_USB_WEBCAM_UVC)
#define UVC_HEADER_DESCRIPTOR_EX(num) UVC_HEADER_DESCRIPTOR(num)

DECLARE_UVC_HEADER_DESCRIPTOR(MULTI_STREAM_NUM);

static struct UVC_HEADER_DESCRIPTOR_EX(MULTI_STREAM_NUM) uvc_control_header = {
	.bLength		= UVC_DT_HEADER_SIZE(MULTI_STREAM_NUM),
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType = UVC_VC_HEADER,
#if (USB_VIDEO_CLASS_VERSION==0x150)
	.bcdUVC			= cpu_to_le16(0x0150),
#else
	.bcdUVC			= cpu_to_le16(0x0100),
#endif
	.wTotalLength		= 0, /* dynamic */
	.dwClockFrequency	= cpu_to_le32(48000000),
	.bInCollection		= 0, /* dynamic */
	.baInterfaceNr[0]	= 0, /* dynamic */
};
#endif

static struct uvc_camera_terminal_descriptor uvc_camera_terminal = {
	.bLength		= UVC_DT_CAMERA_TERMINAL_SIZE(3),
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType = UVC_VC_INPUT_TERMINAL,
	.bTerminalID		= UVC_IT_ID,
	.wTerminalType		= cpu_to_le16(0x0201),
	.bAssocTerminal		= 0,
	.iTerminal		= 0,
	.wObjectiveFocalLengthMin	= cpu_to_le16(0),
	.wObjectiveFocalLengthMax	= cpu_to_le16(0),
	.wOcularFocalLength		= cpu_to_le16(0),
	.bControlSize		= 3,
	.bmControls[0]		= 2,
	.bmControls[1]		= 0,
	.bmControls[2]		= 0,
};

DECLARE_UVC_EXTENSION_UNIT_DESCRIPTOR(1, 2);

static struct UVC_EXTENSION_UNIT_DESCRIPTOR(1,2) uvc_extension_unit1 = {
	.bLength = UVC_DT_EXTENSION_UNIT_SIZE(1,2),
	.bDescriptorType = USB_DT_CS_INTERFACE,
	.bDescriptorSubType = UVC_VC_EXTENSION_UNIT,
	.bUnitID = UVC_EU1_ID,
	.guidExtensionCode = UVC_EU1_GUID,
	.bNumControls = 0x0E,
	.bNrInPins = 0x01,
	.baSourceID[0] = UVC_PU_ID,
	.bControlSize = 0x02,
	.bmControls[0] = 0xFF,
	.bmControls[1] = 0x6F,
	.iExtension = 0x00,
};

static struct UVC_EXTENSION_UNIT_DESCRIPTOR(1, 2) uvc_extension_unit2 = {
	.bLength = UVC_DT_EXTENSION_UNIT_SIZE(1,2),
	.bDescriptorType = USB_DT_CS_INTERFACE,
	.bDescriptorSubType = UVC_VC_EXTENSION_UNIT,
	.bUnitID = UVC_EU2_ID,
	.guidExtensionCode = UVC_EU2_GUID,
	.bNumControls = 0x06,
	.bNrInPins = 0x01,
	.baSourceID[0] = UVC_EU1_ID,
	.bControlSize = 0x02,
	.bmControls[0] = 0x3F,
	.bmControls[1] = 0x00,
	.iExtension = 0x00,
};

static struct uvc_processing_unit_descriptor uvc_processing = {
	.bLength		= UVC_DT_PROCESSING_UNIT_SIZE(2),
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType = UVC_VC_PROCESSING_UNIT,
	.bUnitID		= UVC_PU_ID,
	.bSourceID		= UVC_IT_ID,
	.wMaxMultiplier		= cpu_to_le16(16*1024),
	.bControlSize		= 2,
	.bmControls[0]		= 1,
	.bmControls[1]		= 0,
#if (USB_VIDEO_CLASS_VERSION==0x150)
	.bmControls[2]		= 0,
#endif
	.iProcessing		= 0,
};

static struct uvc_output_terminal_descriptor uvc_output_terminal[MAX_STREAM_SUPPORT] = {{
	.bLength		= UVC_DT_OUTPUT_TERMINAL_SIZE,
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType = UVC_VC_OUTPUT_TERMINAL,
	.bTerminalID		= UVC_OT_ID,
	.wTerminalType		= cpu_to_le16(0x0101),
	.bAssocTerminal		= 0,
	.bSourceID		= UVC_EU2_ID,
	.iTerminal		= 0,
}};

DECLARE_UVC_INPUT_HEADER_DESCRIPTOR(1, 5);
DECLARE_UVC_FRAME_UNCOMPRESSED(3);
DECLARE_UVC_FRAME_FRAMEBASE(3);
#if (USB_VIDEO_CLASS_VERSION==0x150)
DECLARE_UVC_FRAME_H264(3);
#endif

#define SUPPORT_YUY2
#define SUPPORT_NV12
#define SUPPORT_MJPG
#define SUPPORT_H264
#define SUPPORT_H265

static struct UVC_INPUT_HEADER_DESCRIPTOR(1, 5) uvc_input_header[MAX_STREAM_SUPPORT] = {{
	.bLength		= UVC_DT_INPUT_HEADER_SIZE(1, 5),
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType = UVC_VS_INPUT_HEADER,
	.bNumFormats		= 5,
	.wTotalLength		= 0, /* dynamic */
	.bEndpointAddress	= 0, /* dynamic */
	.bmInfo			= 0,
	.bTerminalLink		= UVC_OT_ID,
	.bStillCaptureMethod	= 0,
	.bTriggerSupport	= 0,
	.bTriggerUsage		= 0,
	.bControlSize		= 1,
	.bmaControls[0][0]	= 0,
	.bmaControls[1][0]	= 4,
	.bmaControls[2][0]	= 4,
	.bmaControls[3][0]	= 4,
}};

#ifdef SUPPORT_YUY2
static struct uvc_format_uncompressed uvc_format_yuy2 = {
	.bLength		= UVC_DT_FORMAT_UNCOMPRESSED_SIZE,
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType = UVC_VS_FORMAT_UNCOMPRESSED,
	.bFormatIndex		= 1,
	.bNumFrameDescriptors	= 4,
	.guidFormat		=
		{ 'Y',	'U',  'Y',	'2', 0x00, 0x00, 0x10, 0x00,
		 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71},
	.bBitsPerPixel		= 16,
	.bDefaultFrameIndex = 1,
	.bAspectRatioX		= 0,
	.bAspectRatioY		= 0,
	.bmInterfaceFlags	= 0,
	.bCopyProtect		= 0,
};


static struct UVC_FRAME_UNCOMPRESSED(3) uvc_frame_yuy2_240p = {
	.bLength		= UVC_DT_FRAME_UNCOMPRESSED_SIZE(3),
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType = UVC_VS_FRAME_UNCOMPRESSED,
	.bFrameIndex		= 1,
	.bmCapabilities		= 0,
	.wWidth			= cpu_to_le16(320),
	.wHeight		= cpu_to_le16(240),
	.dwMinBitRate		= cpu_to_le32(320*240*2.0*8*10),
	.dwMaxBitRate		= cpu_to_le32(320*240*2.0*8*30),
	.dwMaxVideoFrameBufferSize	= cpu_to_le32(320*240*2.0),
	.dwDefaultFrameInterval		= cpu_to_le32(333333),
	.bFrameIntervalType		= 3,
	.dwFrameInterval[0]		= cpu_to_le32(333333),
	.dwFrameInterval[1]		= cpu_to_le32(666666),
	.dwFrameInterval[2]		= cpu_to_le32(1000000),
};
static struct UVC_FRAME_UNCOMPRESSED(3) uvc_frame_yuy2_480p = {
	.bLength		= UVC_DT_FRAME_UNCOMPRESSED_SIZE(3),
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType = UVC_VS_FRAME_UNCOMPRESSED,
	.bFrameIndex		= 2,
	.bmCapabilities		= 0,
	.wWidth			= cpu_to_le16(640),
	.wHeight		= cpu_to_le16(480),
	.dwMinBitRate		= cpu_to_le32(640*480*2.0*8*10),
	.dwMaxBitRate		= cpu_to_le32(640*480*2.0*8*30),
	.dwMaxVideoFrameBufferSize	= cpu_to_le32(640*480*2.0),
	.dwDefaultFrameInterval		= cpu_to_le32(333333),
	.bFrameIntervalType = 3,
	.dwFrameInterval[0] = cpu_to_le32(333333),
	.dwFrameInterval[1] = cpu_to_le32(666666),
	.dwFrameInterval[2] = cpu_to_le32(1000000),
};
static struct UVC_FRAME_UNCOMPRESSED(3) uvc_frame_yuy2_720p = {
	.bLength		= UVC_DT_FRAME_UNCOMPRESSED_SIZE(3),
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType = UVC_VS_FRAME_UNCOMPRESSED,
	.bFrameIndex		= 3,
	.bmCapabilities		= 0,
	.wWidth			= cpu_to_le16(1280),
	.wHeight		= cpu_to_le16(720),
	.dwMinBitRate		= cpu_to_le32(1280*720*2.0*8*10),
	.dwMaxBitRate		= cpu_to_le32(1280*720*2.0*8*30),
	.dwMaxVideoFrameBufferSize	= cpu_to_le32(1280*720*2.0),
	.dwDefaultFrameInterval = cpu_to_le32(333333),
	.bFrameIntervalType = 3,
	.dwFrameInterval[0] = cpu_to_le32(333333),
	.dwFrameInterval[1] = cpu_to_le32(666666),
	.dwFrameInterval[2] = cpu_to_le32(1000000),
};
static struct UVC_FRAME_UNCOMPRESSED(3) uvc_frame_yuy2_1080p = {
	.bLength		= UVC_DT_FRAME_UNCOMPRESSED_SIZE(3),
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType = UVC_VS_FRAME_UNCOMPRESSED,
	.bFrameIndex		= 4,
	.bmCapabilities		= 0,
	.wWidth			= cpu_to_le16(1920),
	.wHeight		= cpu_to_le16(1080),
	.dwMinBitRate		= cpu_to_le32(1920*1080*2.0*8*10),
	.dwMaxBitRate		= cpu_to_le32(1920*1080*2.0*8*30),
	.dwMaxVideoFrameBufferSize	= cpu_to_le32(1920*1080*2.0),
	.dwDefaultFrameInterval		= cpu_to_le32(333333),
	.bFrameIntervalType = 3,
	.dwFrameInterval[0] = cpu_to_le32(333333),
	.dwFrameInterval[1] = cpu_to_le32(666666),
	.dwFrameInterval[2] = cpu_to_le32(1000000),
};
#define UVC_DESCRIPTOR_HEADERS_OF_YUY2_FRAME \
	(struct uvc_descriptor_header *) &uvc_format_yuy2, \
	(struct uvc_descriptor_header *) &uvc_frame_yuy2_240p, \
	(struct uvc_descriptor_header *) &uvc_frame_yuy2_480p, \
	(struct uvc_descriptor_header *) &uvc_frame_yuy2_720p, \
	(struct uvc_descriptor_header *) &uvc_frame_yuy2_1080p,
#else
#define UVC_DESCRIPTOR_HEADERS_OF_YUY2_FRAME
#endif
#ifdef SUPPORT_NV12
static struct uvc_format_uncompressed uvc_format_nv12 = {
	.bLength		= UVC_DT_FORMAT_UNCOMPRESSED_SIZE,
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType = UVC_VS_FORMAT_UNCOMPRESSED,
	.bFormatIndex		= 2,
	.bNumFrameDescriptors	= 4,
	.guidFormat		=
		{ 'N',	'V',  '1',	'2', 0x00, 0x00, 0x10, 0x00,
		0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71},
	.bBitsPerPixel		= 12,
	.bDefaultFrameIndex = 1,
	.bAspectRatioX		= 0,
	.bAspectRatioY		= 0,
	.bmInterfaceFlags	= 0,
	.bCopyProtect		= 0,
};
static struct UVC_FRAME_UNCOMPRESSED(3) uvc_frame_nv12_240p = {
	.bLength		= UVC_DT_FRAME_UNCOMPRESSED_SIZE(3),
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType = UVC_VS_FRAME_UNCOMPRESSED,
	.bFrameIndex		= 1,
	.bmCapabilities		= 0,
	.wWidth			= cpu_to_le16(320),
	.wHeight		= cpu_to_le16(240),
	.dwMinBitRate		= cpu_to_le32(320*240*1.5*8*10),
	.dwMaxBitRate		= cpu_to_le32(320*240*1.5*8*30),
	.dwMaxVideoFrameBufferSize	= cpu_to_le32(320*240*1.5),
	.dwDefaultFrameInterval		= cpu_to_le32(333333),
	.bFrameIntervalType = 3,
	.dwFrameInterval[0] = cpu_to_le32(333333),
	.dwFrameInterval[1] = cpu_to_le32(666666),
	.dwFrameInterval[2] = cpu_to_le32(1000000),
};
static struct UVC_FRAME_UNCOMPRESSED(3) uvc_frame_nv12_480p = {
	.bLength		= UVC_DT_FRAME_UNCOMPRESSED_SIZE(3),
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType = UVC_VS_FRAME_UNCOMPRESSED,
	.bFrameIndex		= 2,
	.bmCapabilities		= 0,
	.wWidth			= cpu_to_le16(640),
	.wHeight		= cpu_to_le16(480),
	.dwMinBitRate		= cpu_to_le32(640*480*1.5*8*10),
	.dwMaxBitRate		= cpu_to_le32(640*480*1.5*8*30),
	.dwMaxVideoFrameBufferSize	= cpu_to_le32(640*480*1.5),
	.dwDefaultFrameInterval		= cpu_to_le32(333333),
	.bFrameIntervalType = 3,
	.dwFrameInterval[0] = cpu_to_le32(333333),
	.dwFrameInterval[1] = cpu_to_le32(666666),
	.dwFrameInterval[2] = cpu_to_le32(1000000),
};
static struct UVC_FRAME_UNCOMPRESSED(3) uvc_frame_nv12_720p = {
	.bLength		= UVC_DT_FRAME_UNCOMPRESSED_SIZE(3),
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType = UVC_VS_FRAME_UNCOMPRESSED,
	.bFrameIndex		= 3,
	.bmCapabilities		= 0,
	.wWidth			= cpu_to_le16(1280),
	.wHeight		= cpu_to_le16(720),
	.dwMinBitRate		= cpu_to_le32(1280*720*1.5*8*10),
	.dwMaxBitRate		= cpu_to_le32(1280*720*1.5*8*30),
	.dwMaxVideoFrameBufferSize	= cpu_to_le32(1280*720*1.5),
	.dwDefaultFrameInterval		= cpu_to_le32(333333),
	.bFrameIntervalType = 3,
	.dwFrameInterval[0] = cpu_to_le32(333333),
	.dwFrameInterval[1] = cpu_to_le32(666666),
	.dwFrameInterval[2] = cpu_to_le32(1000000),
};
static struct UVC_FRAME_UNCOMPRESSED(3) uvc_frame_nv12_1080p = {
	.bLength		= UVC_DT_FRAME_UNCOMPRESSED_SIZE(3),
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType = UVC_VS_FRAME_UNCOMPRESSED,
	.bFrameIndex		= 4,
	.bmCapabilities		= 0,
	.wWidth			= cpu_to_le16(1920),
	.wHeight		= cpu_to_le16(1080),
	.dwMinBitRate		= cpu_to_le32(1920*1080*1.5*8*10),
	.dwMaxBitRate		= cpu_to_le32(1920*1080*1.5*8*30),
	.dwMaxVideoFrameBufferSize	= cpu_to_le32(1920*1080*1.5),
	.dwDefaultFrameInterval		= cpu_to_le32(333333),
	.bFrameIntervalType = 3,
	.dwFrameInterval[0] = cpu_to_le32(333333),
	.dwFrameInterval[1] = cpu_to_le32(666666),
	.dwFrameInterval[2] = cpu_to_le32(1000000),
};
#define UVC_DESCRIPTOR_HEADERS_OF_NV12_FRAME \
	(struct uvc_descriptor_header *) &uvc_format_nv12, \
	(struct uvc_descriptor_header *) &uvc_frame_nv12_240p, \
	(struct uvc_descriptor_header *) &uvc_frame_nv12_480p, \
	(struct uvc_descriptor_header *) &uvc_frame_nv12_720p, \
	(struct uvc_descriptor_header *) &uvc_frame_nv12_1080p,
#else
#define UVC_DESCRIPTOR_HEADERS_OF_NV12_FRAME
#endif

#ifdef SUPPORT_MJPG
static struct uvc_format_mjpeg uvc_format_mjpg = {
	.bLength		= UVC_DT_FORMAT_MJPEG_SIZE,
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType = UVC_VS_FORMAT_MJPEG,
	.bFormatIndex		= 3,
	.bNumFrameDescriptors	= 6,
	.bmFlags		= 0,
	.bDefaultFrameIndex = 1,
	.bAspectRatioX		= 0,
	.bAspectRatioY		= 0,
	.bmInterfaceFlags	= 0,
	.bCopyProtect		= 0,
};

DECLARE_UVC_FRAME_MJPEG(3);

static struct UVC_FRAME_MJPEG(3) uvc_frame_mjpg_240p = {
	.bLength		= UVC_DT_FRAME_MJPEG_SIZE(3),
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType = UVC_VS_FRAME_MJPEG,
	.bFrameIndex		= 1,
	.bmCapabilities		= 0,
	.wWidth			= cpu_to_le16(320),
	.wHeight		= cpu_to_le16(240),
	.dwMinBitRate		= cpu_to_le32(320*240*2*8*10),
	.dwMaxBitRate		= cpu_to_le32(320*240*2*8*30),
	.dwMaxVideoFrameBufferSize	= cpu_to_le32(320*240*2),
	.dwDefaultFrameInterval		= cpu_to_le32(333333),
	.bFrameIntervalType = 3,
	.dwFrameInterval[0] = cpu_to_le32(333333),
	.dwFrameInterval[1] = cpu_to_le32(666666),
	.dwFrameInterval[2] = cpu_to_le32(1000000),
};
static struct UVC_FRAME_MJPEG(3) uvc_frame_mjpg_480p = {
	.bLength		= UVC_DT_FRAME_MJPEG_SIZE(3),
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType = UVC_VS_FRAME_MJPEG,
	.bFrameIndex		= 2,
	.bmCapabilities		= 0,
	.wWidth			= cpu_to_le16(640),
	.wHeight		= cpu_to_le16(480),
	.dwMinBitRate		= cpu_to_le32(640*480*2*8*10),
	.dwMaxBitRate		= cpu_to_le32(640*480*2*8*30),
	.dwMaxVideoFrameBufferSize	= cpu_to_le32(640*480*2),
	.dwDefaultFrameInterval		= cpu_to_le32(333333),
	.bFrameIntervalType = 3,
	.dwFrameInterval[0] = cpu_to_le32(333333),
	.dwFrameInterval[1] = cpu_to_le32(666666),
	.dwFrameInterval[2] = cpu_to_le32(1000000),
};
static struct UVC_FRAME_MJPEG(3) uvc_frame_mjpg_720p = {
	.bLength		= UVC_DT_FRAME_MJPEG_SIZE(3),
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType = UVC_VS_FRAME_MJPEG,
	.bFrameIndex		= 3,
	.bmCapabilities		= 0,
	.wWidth			= cpu_to_le16(1280),
	.wHeight		= cpu_to_le16(720),
	.dwMinBitRate		= cpu_to_le32(1280*720*2*8*10),
	.dwMaxBitRate		= cpu_to_le32(1280*720*2*8*30),
	.dwMaxVideoFrameBufferSize	= cpu_to_le32(1280*720*2),
	.dwDefaultFrameInterval		= cpu_to_le32(333333),
	.bFrameIntervalType = 3,
	.dwFrameInterval[0] = cpu_to_le32(333333),
	.dwFrameInterval[1] = cpu_to_le32(666666),
	.dwFrameInterval[2] = cpu_to_le32(1000000),
};
static struct UVC_FRAME_MJPEG(3) uvc_frame_mjpg_1080p = {
	.bLength		= UVC_DT_FRAME_MJPEG_SIZE(3),
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType = UVC_VS_FRAME_MJPEG,
	.bFrameIndex		= 4,
	.bmCapabilities		= 0,
	.wWidth			= cpu_to_le16(1920),
	.wHeight		= cpu_to_le16(1080),
	.dwMinBitRate		= cpu_to_le32(1920*1080*2*8*10),
	.dwMaxBitRate		= cpu_to_le32(1920*1080*2*8*30),
	.dwMaxVideoFrameBufferSize	= cpu_to_le32(1920*1080*2),
	.dwDefaultFrameInterval = cpu_to_le32(333333),
	.bFrameIntervalType = 3,
	.dwFrameInterval[0] = cpu_to_le32(333333),
	.dwFrameInterval[1] = cpu_to_le32(666666),
	.dwFrameInterval[2] = cpu_to_le32(1000000),
};

static struct UVC_FRAME_MJPEG(3) uvc_frame_mjpg_2kp = {
	.bLength		= UVC_DT_FRAME_MJPEG_SIZE(3),
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType = UVC_VS_FRAME_MJPEG,
	.bFrameIndex		= 5,
	.bmCapabilities		= 0,
	.wWidth			= cpu_to_le16(2560),
	.wHeight		= cpu_to_le16(1440),
	.dwMinBitRate		= cpu_to_le32(2560*1440*2*8*10),
	.dwMaxBitRate		= cpu_to_le32(2560*1440*2*8*10), //overflow
	.dwMaxVideoFrameBufferSize	= cpu_to_le32(2560*1440*2),
	.dwDefaultFrameInterval = cpu_to_le32(333333),
	.bFrameIntervalType = 3,
	.dwFrameInterval[0] = cpu_to_le32(333333),
	.dwFrameInterval[1] = cpu_to_le32(666666),
	.dwFrameInterval[2] = cpu_to_le32(1000000),
};
static struct UVC_FRAME_MJPEG(3) uvc_frame_mjpg_4kp = {
	.bLength		= UVC_DT_FRAME_MJPEG_SIZE(3),
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType = UVC_VS_FRAME_MJPEG,
	.bFrameIndex		= 6,
	.bmCapabilities		= 0,
	.wWidth			= cpu_to_le16(3840),
	.wHeight		= cpu_to_le16(2160),
	.dwMinBitRate		= cpu_to_le32(3840*2160*2*8*10),
	.dwMaxBitRate		= cpu_to_le32(3840*2160*2*8*10), //overflow
	.dwMaxVideoFrameBufferSize	= cpu_to_le32(3840*2160*2),
	.dwDefaultFrameInterval = cpu_to_le32(333333),
	.bFrameIntervalType = 3,
	.dwFrameInterval[0] = cpu_to_le32(333333),
	.dwFrameInterval[1] = cpu_to_le32(666666),
	.dwFrameInterval[2] = cpu_to_le32(1000000),
};
#define UVC_DESCRIPTOR_HEADERS_OF_MJPG_FRAME \
	(struct uvc_descriptor_header *) &uvc_format_mjpg, \
	(struct uvc_descriptor_header *) &uvc_frame_mjpg_240p,	\
	(struct uvc_descriptor_header *) &uvc_frame_mjpg_480p,	\
	(struct uvc_descriptor_header *) &uvc_frame_mjpg_720p,	\
	(struct uvc_descriptor_header *) &uvc_frame_mjpg_1080p, \
	(struct uvc_descriptor_header *) &uvc_frame_mjpg_2kp,	\
	(struct uvc_descriptor_header *) &uvc_frame_mjpg_4kp,
#else
#define UVC_DESCRIPTOR_HEADERS_OF_MJPG_FRAME
#endif

#ifdef SUPPORT_H264
#if 0
static struct uvc_format_h264 uvc_format_h264 = {
	.bLength				= UVC_DT_FORMAT_H264_SIZE,
	.bDescriptorType			= USB_DT_CS_INTERFACE,
	.bDescriptorSubType			= UVC_VS_FORMAT_H264,
	.bFormatIndex				= 3,
	.bNumFrameDescriptors		= 1,
	.bDefaultFrameIndex			= 1,
	.bMaxCodecConfigDelay			= 0x4,
	.bmSupportedSliceModes			= 0,
	.bmSupportedSyncFrameTypes		= 0x76,
	.bResolutionScaling				= 0,
	.Reserved1						= 0,
	.bmSupportedRateControlModes	= 0x3F,
	.wMaxMBperSecOneResNoScalability	= cpu_to_le16(972),
	.wMaxMBperSecTwoResNoScalability	= 0,
	.wMaxMBperSecThreeResNoScalability	= 0,
	.wMaxMBperSecFourResNoScalability	= 0,
	.wMaxMBperSecOneResTemporalScalability	= cpu_to_le16(972),
	.wMaxMBperSecTwoResTemporalScalability	= 0,
	.wMaxMBperSecThreeResTemporalScalability	= 0,
	.wMaxMBperSecFourResTemporalScalability		= 0,
	.wMaxMBperSecOneResTemporalQualityScalability	= cpu_to_le16(972),
	.wMaxMBperSecTwoResTemporalQualityScalability	= 0,
	.wMaxMBperSecThreeResTemporalQualityScalability	= 0,
	.wMaxMBperSecFourResTemporalQualityScalability	= 0,
	.wMaxMBperSecOneResTemporalSpatialScalability	= 0,
	.wMaxMBperSecTwoResTemporalSpatialScalability	= 0,
	.wMaxMBperSecThreeResTemporalSpatialScalability	= 0,
	.wMaxMBperSecFourResTemporalSpatialScalability	= 0,
	.wMaxMBperSecOneResFullScalability		= 0,
	.wMaxMBperSecTwoResFullScalability		= 0,
	.wMaxMBperSecThreeResFullScalability	= 0,
	.wMaxMBperSecFourResFullScalability		= 0,
};

static struct UVC_FRAME_H264(3) uvc_frame_h264_240p = {
	.bLength		= UVC_DT_FRAME_H264_SIZE(3),
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VS_FRAME_H264,
	.bFrameIndex		= 1,
	.wWidth			= cpu_to_le16(320),
	.wHeight		= cpu_to_le16(240),
	.wSARwidth		= 1,
	.wSARheight		= 1,
	.wProfile		= 0x6400,
	.bLevelIDC		= 0x33,
	.bmSupportedUsages	= 0x70003,
	.wConstrainedToolset	= cpu_to_le16(0),
	.bmCapabilities		= 0x47,
	.bmSVCCapabilities	= 0x4,
	.bmMVCCapabilities	= 0,
	.dwMinBitRate		= cpu_to_le32(320*240*2*8*10),
	.dwMaxBitRate		= cpu_to_le32(320*240*2*8*30),
	.dwDefaultFrameInterval	= cpu_to_le32(333333),
	.bNumFrameIntervals	= 3,
	.dwFrameInterval[0]	= cpu_to_le32(333333),
	.dwFrameInterval[1]	= cpu_to_le32(666666),
	.dwFrameInterval[2]	= cpu_to_le32(1000000),
};

static struct UVC_FRAME_H264(3) uvc_frame_h264_480p;
static struct UVC_FRAME_H264(3) uvc_frame_h264_720p;
static struct UVC_FRAME_FRAMEBASE(3) uvc_frame_h264_1080p;
static struct UVC_FRAME_FRAMEBASE(3) uvc_frame_h264_2kp;
static struct UVC_FRAME_FRAMEBASE(3) uvc_frame_h264_4kp;
#else
static struct uvc_format_framebase uvc_format_h264 = {
	.bLength		= UVC_DT_FORMAT_FRAMEBASE_SIZE,
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType = UVC_VS_FORMAT_FRAME_BASED,
	.bFormatIndex		= 4,
	.bNumFrameDescriptors	= 6,
	.guidFormat		=
		{ 'H',	'2',  '6',	'4', 0x00, 0x00, 0x10, 0x00,
		 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71},
	.bBitsPerPixel = 16,
	.bDefaultFrameIndex = 1,
	.bAspectRatioX		= 0,
	.bAspectRatioY		= 0,
	.bmInterfaceFlags	= 0,
	.bCopyProtect		= 0,
	.bVariableSize = 1
};

static struct UVC_FRAME_FRAMEBASE(3) uvc_frame_h264_240p = {
	.bLength		= UVC_DT_FRAME_MJPEG_SIZE(3),
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType = UVC_VS_FRAME_FRAME_BASED,
	.bFrameIndex		= 1,
	.bmCapabilities		= 0,
	.wWidth			= cpu_to_le16(320),
	.wHeight		= cpu_to_le16(240),
	.dwMinBitRate		= cpu_to_le32(320*240*2*8*10),
	.dwMaxBitRate		= cpu_to_le32(320*240*2*8*30),
	.dwDefaultFrameInterval = cpu_to_le32(333333),
	.bFrameIntervalType = 3,
	.dwFrameInterval[0] = cpu_to_le32(333333),
	.dwFrameInterval[1] = cpu_to_le32(666666),
	.dwFrameInterval[2] = cpu_to_le32(1000000),
};
static struct UVC_FRAME_FRAMEBASE(3) uvc_frame_h264_480p = {
	.bLength		= UVC_DT_FRAME_MJPEG_SIZE(3),
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType = UVC_VS_FRAME_FRAME_BASED,
	.bFrameIndex		= 2,
	.bmCapabilities		= 0,
	.wWidth			= cpu_to_le16(640),
	.wHeight		= cpu_to_le16(480),
	.dwMinBitRate		= cpu_to_le32(640*480*2*8*10),
	.dwMaxBitRate		= cpu_to_le32(640*480*2*8*30),
	.dwDefaultFrameInterval = cpu_to_le32(333333),
	.bFrameIntervalType = 3,
	.dwFrameInterval[0] = cpu_to_le32(333333),
	.dwFrameInterval[1] = cpu_to_le32(666666),
	.dwFrameInterval[2] = cpu_to_le32(1000000),
};
static struct UVC_FRAME_FRAMEBASE(3) uvc_frame_h264_720p = {
	.bLength		= UVC_DT_FRAME_MJPEG_SIZE(3),
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType = UVC_VS_FRAME_FRAME_BASED,
	.bFrameIndex		= 3,
	.bmCapabilities		= 0,
	.wWidth			= cpu_to_le16(1280),
	.wHeight		= cpu_to_le16(720),
	.dwMinBitRate		= cpu_to_le32(1280*720*2*8*10),
	.dwMaxBitRate		= cpu_to_le32(1280*720*2*8*30),
	.dwDefaultFrameInterval = cpu_to_le32(333333),
	.bFrameIntervalType = 3,
	.dwFrameInterval[0] = cpu_to_le32(333333),
	.dwFrameInterval[1] = cpu_to_le32(666666),
	.dwFrameInterval[2] = cpu_to_le32(1000000),
};
static struct UVC_FRAME_FRAMEBASE(3) uvc_frame_h264_1080p = {
	.bLength		= UVC_DT_FRAME_MJPEG_SIZE(3),
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType = UVC_VS_FRAME_FRAME_BASED,
	.bFrameIndex		= 4,
	.bmCapabilities		= 0,
	.wWidth			= cpu_to_le16(1920),
	.wHeight		= cpu_to_le16(1080),
	.dwMinBitRate		= cpu_to_le32(1920*1080*2*8*10),
	.dwMaxBitRate		= cpu_to_le32(1920*1080*2*8*30),
	.dwDefaultFrameInterval = cpu_to_le32(333333),
	.bFrameIntervalType = 3,
	.dwFrameInterval[0] = cpu_to_le32(333333),
	.dwFrameInterval[1] = cpu_to_le32(666666),
	.dwFrameInterval[2] = cpu_to_le32(1000000),
};
static struct UVC_FRAME_FRAMEBASE(3) uvc_frame_h264_2kp = {
	.bLength		= UVC_DT_FRAME_MJPEG_SIZE(3),
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType = UVC_VS_FRAME_FRAME_BASED,
	.bFrameIndex		= 5,
	.bmCapabilities		= 0,
	.wWidth			= cpu_to_le16(2560),
	.wHeight		= cpu_to_le16(1440),
	.dwMinBitRate		= cpu_to_le32(2560*1440*2*8*10),
	.dwMaxBitRate		= cpu_to_le32(2560*1440*2*8*10),
	.dwDefaultFrameInterval = cpu_to_le32(333333),
	.bFrameIntervalType = 3,
	.dwFrameInterval[0] = cpu_to_le32(333333),
	.dwFrameInterval[1] = cpu_to_le32(666666),
	.dwFrameInterval[2] = cpu_to_le32(1000000),
};
static struct UVC_FRAME_FRAMEBASE(3) uvc_frame_h264_4kp = {
	.bLength		= UVC_DT_FRAME_MJPEG_SIZE(3),
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType = UVC_VS_FRAME_FRAME_BASED,
	.bFrameIndex		= 6,
	.bmCapabilities		= 0,
	.wWidth			= cpu_to_le16(3840),
	.wHeight		= cpu_to_le16(2160),
	.dwMinBitRate		= cpu_to_le32(3840*2160*2*8*10),
	.dwMaxBitRate		= cpu_to_le32(3840*2160*2*8*10),
	.dwDefaultFrameInterval = cpu_to_le32(333333),
	.bFrameIntervalType = 3,
	.dwFrameInterval[0] = cpu_to_le32(333333),
	.dwFrameInterval[1] = cpu_to_le32(666666),
	.dwFrameInterval[2] = cpu_to_le32(1000000),
};
#endif
#define UVC_DESCRIPTOR_HEADERS_OF_H264_FRAME \
	(struct uvc_descriptor_header *) &uvc_format_h264,	  \
	(struct uvc_descriptor_header *) &uvc_frame_h264_240p,	\
	(struct uvc_descriptor_header *) &uvc_frame_h264_480p,	\
	(struct uvc_descriptor_header *) &uvc_frame_h264_720p,	\
	(struct uvc_descriptor_header *) &uvc_frame_h264_1080p, \
	(struct uvc_descriptor_header *) &uvc_frame_h264_2kp,	\
	(struct uvc_descriptor_header *) &uvc_frame_h264_4kp,
#else
#define UVC_DESCRIPTOR_HEADERS_OF_H264_FRAME
#endif

#ifdef SUPPORT_H265
static struct uvc_format_framebase uvc_format_h265 = {
	.bLength		= UVC_DT_FORMAT_FRAMEBASE_SIZE,
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType = UVC_VS_FORMAT_FRAME_BASED,
	.bFormatIndex		= 5,
	.bNumFrameDescriptors	= 6,
	.guidFormat		=
		{ 'H',	'2',  '6',	'5', 0x00, 0x00, 0x10, 0x00,
		 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71},
	.bBitsPerPixel = 16,
	.bDefaultFrameIndex = 1,
	.bAspectRatioX		= 0,
	.bAspectRatioY		= 0,
	.bmInterfaceFlags	= 0,
	.bCopyProtect		= 0,
	.bVariableSize = 1
};
static struct UVC_FRAME_FRAMEBASE(3) uvc_frame_h265_240p = {
	.bLength		= UVC_DT_FRAME_MJPEG_SIZE(3),
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType = UVC_VS_FRAME_FRAME_BASED,
	.bFrameIndex		= 1,
	.bmCapabilities		= 0,
	.wWidth			= cpu_to_le16(320),
	.wHeight		= cpu_to_le16(240),
	.dwMinBitRate		= cpu_to_le32(320*240*2*8*10),
	.dwMaxBitRate		= cpu_to_le32(320*240*2*8*30),
	.dwDefaultFrameInterval = cpu_to_le32(333333),
	.bFrameIntervalType = 3,
	.dwFrameInterval[0] = cpu_to_le32(333333),
	.dwFrameInterval[1] = cpu_to_le32(666666),
	.dwFrameInterval[2] = cpu_to_le32(1000000),
};
static struct UVC_FRAME_FRAMEBASE(3) uvc_frame_h265_480p = {
	.bLength		= UVC_DT_FRAME_MJPEG_SIZE(3),
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType = UVC_VS_FRAME_FRAME_BASED,
	.bFrameIndex		= 2,
	.bmCapabilities		= 0,
	.wWidth			= cpu_to_le16(640),
	.wHeight		= cpu_to_le16(480),
	.dwMinBitRate		= cpu_to_le32(640*480*2*8*10),
	.dwMaxBitRate		= cpu_to_le32(640*480*2*8*30),
	.dwDefaultFrameInterval = cpu_to_le32(333333),
	.bFrameIntervalType = 3,
	.dwFrameInterval[0] = cpu_to_le32(333333),
	.dwFrameInterval[1] = cpu_to_le32(666666),
	.dwFrameInterval[2] = cpu_to_le32(1000000),
};
static struct UVC_FRAME_FRAMEBASE(3) uvc_frame_h265_720p = {
	.bLength		= UVC_DT_FRAME_MJPEG_SIZE(3),
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType = UVC_VS_FRAME_FRAME_BASED,
	.bFrameIndex		= 3,
	.bmCapabilities		= 0,
	.wWidth			= cpu_to_le16(1280),
	.wHeight		= cpu_to_le16(720),
	.dwMinBitRate		= cpu_to_le32(1280*720*2*8*10),
	.dwMaxBitRate		= cpu_to_le32(1280*720*2*8*30),
	.dwDefaultFrameInterval = cpu_to_le32(333333),
	.bFrameIntervalType = 3,
	.dwFrameInterval[0] = cpu_to_le32(333333),
	.dwFrameInterval[1] = cpu_to_le32(666666),
	.dwFrameInterval[2] = cpu_to_le32(1000000),
};
static struct UVC_FRAME_FRAMEBASE(3) uvc_frame_h265_1080p = {
	.bLength		= UVC_DT_FRAME_MJPEG_SIZE(3),
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType = UVC_VS_FRAME_FRAME_BASED,
	.bFrameIndex		= 4,
	.bmCapabilities		= 0,
	.wWidth			= cpu_to_le16(1920),
	.wHeight		= cpu_to_le16(1080),
	.dwMinBitRate		= cpu_to_le32(1920*1080*2*8*10),
	.dwMaxBitRate		= cpu_to_le32(1920*1080*2*8*30),
	.dwDefaultFrameInterval = cpu_to_le32(333333),
	.bFrameIntervalType = 3,
	.dwFrameInterval[0] = cpu_to_le32(333333),
	.dwFrameInterval[1] = cpu_to_le32(666666),
	.dwFrameInterval[2] = cpu_to_le32(1000000),
};
static struct UVC_FRAME_FRAMEBASE(3) uvc_frame_h265_2kp = {
	.bLength		= UVC_DT_FRAME_MJPEG_SIZE(3),
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType = UVC_VS_FRAME_FRAME_BASED,
	.bFrameIndex		= 5,
	.bmCapabilities		= 0,
	.wWidth			= cpu_to_le16(2560),
	.wHeight		= cpu_to_le16(1440),
	.dwMinBitRate		= cpu_to_le32(2560*1440*2*8*10),
	.dwMaxBitRate		= cpu_to_le32(2560*1440*2*8*10),
	.dwDefaultFrameInterval = cpu_to_le32(333333),
	.bFrameIntervalType = 3,
	.dwFrameInterval[0] = cpu_to_le32(333333),
	.dwFrameInterval[1] = cpu_to_le32(666666),
	.dwFrameInterval[2] = cpu_to_le32(1000000),
};
static struct UVC_FRAME_FRAMEBASE(3) uvc_frame_h265_4kp = {
	.bLength		= UVC_DT_FRAME_MJPEG_SIZE(3),
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType = UVC_VS_FRAME_FRAME_BASED,
	.bFrameIndex		= 6,
	.bmCapabilities		= 0,
	.wWidth			= cpu_to_le16(3840),
	.wHeight		= cpu_to_le16(2160),
	.dwMinBitRate		= cpu_to_le32(3840*2160*2*8*10),
	.dwMaxBitRate		= cpu_to_le32(3840*2160*2*8*10),
	.dwDefaultFrameInterval = cpu_to_le32(333333),
	.bFrameIntervalType = 3,
	.dwFrameInterval[0] = cpu_to_le32(333333),
	.dwFrameInterval[1] = cpu_to_le32(666666),
	.dwFrameInterval[2] = cpu_to_le32(1000000),
};
#define UVC_DESCRIPTOR_HEADERS_OF_H265_FRAME \
	(struct uvc_descriptor_header *) &uvc_format_h265,	 \
	(struct uvc_descriptor_header *) &uvc_frame_h265_240p, \
	(struct uvc_descriptor_header *) &uvc_frame_h265_480p, \
	(struct uvc_descriptor_header *) &uvc_frame_h265_720p, \
	(struct uvc_descriptor_header *) &uvc_frame_h265_1080p,\
	(struct uvc_descriptor_header *) &uvc_frame_h265_2kp,  \
	(struct uvc_descriptor_header *) &uvc_frame_h265_4kp,  \
	(struct uvc_descriptor_header *) &uvc_color_matching,
#else
#define UVC_DESCRIPTOR_HEADERS_OF_H265_FRAME
#endif

static struct uvc_color_matching_descriptor uvc_color_matching = {
	.bLength		= UVC_DT_COLOR_MATCHING_SIZE,
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType = UVC_VS_COLORFORMAT,
	.bColorPrimaries	= 1,
	.bTransferCharacteristics	= 1,
	.bMatrixCoefficients	= 4,
};

#define MAX_CONTROL_SIZE 10
#define BASE_CONTROL_SIZE 6
static struct uvc_descriptor_header ** uvc_fs_control_cls;
static struct uvc_descriptor_header ** uvc_ss_control_cls;

#define MAX_FORMAT_COUNT 50
static struct uvc_descriptor_header ** uvc_fs_streaming_cls;
static struct uvc_descriptor_header ** uvc_hs_streaming_cls;
static struct uvc_descriptor_header ** uvc_ss_streaming_cls;
static struct uvc_descriptor_header * uvc_streaming_cls_std[] = {
	(struct uvc_descriptor_header *) &uvc_input_header[0],
	UVC_DESCRIPTOR_HEADERS_OF_YUY2_FRAME
	UVC_DESCRIPTOR_HEADERS_OF_NV12_FRAME
	UVC_DESCRIPTOR_HEADERS_OF_MJPG_FRAME
	UVC_DESCRIPTOR_HEADERS_OF_H264_FRAME
	UVC_DESCRIPTOR_HEADERS_OF_H265_FRAME
	NULL,
};
/* --------------------------------------------------------------------------
 * USB configuration
 */
#if defined(CONFIG_USB_WEBCAM_UVC)
static int video_bind_config(struct usb_configuration *c)
{
	int status = 0, i;

	f_uvc = kzalloc(uvc_function_enable * sizeof(*f_uvc), GFP_KERNEL);
	for (i = 0;i < uvc_function_enable;i++)
	{
		f_uvc[i] = usb_get_function(fi_uvc[i]);
		if (IS_ERR(f_uvc[i]))
			return PTR_ERR(f_uvc[i]);

		status = usb_add_function(c, f_uvc[i]);
		if (status < 0)
		{
			usb_put_function(f_uvc[i]);
			break;
		}
	}
	return status;
}


static int
video_unbind(struct usb_composite_dev *cdev)
{
	int i;

	for (i = 0;i < uvc_function_enable;i++)
	{
		if (!IS_ERR_OR_NULL(f_uvc[i]))
		{
			usb_put_function(f_uvc[i]);
		}
		if (!IS_ERR_OR_NULL(fi_uvc[i]))
		{
			usb_put_function_instance(fi_uvc[i]);
		}
	}
	if (!IS_ERR_OR_NULL(f_uvc))
	{
		kfree(f_uvc);
		f_uvc = NULL;
	}
	if (!IS_ERR_OR_NULL(fi_uvc))
	{
		kfree(fi_uvc);
		fi_uvc = NULL;
	}
	if (uvc_fs_streaming_cls)
	{
		kfree(uvc_fs_streaming_cls);
		kfree(uvc_hs_streaming_cls);
		kfree(uvc_ss_streaming_cls);
		uvc_fs_streaming_cls = NULL;
		uvc_hs_streaming_cls = NULL;
		uvc_ss_streaming_cls = NULL;
	}
	if (uvc_fs_control_cls)
	{
		kfree(uvc_fs_control_cls);
		kfree(uvc_ss_control_cls);
		uvc_fs_control_cls = NULL;
		uvc_ss_control_cls = NULL;
	}
	return 0;
}

static int __init video_bind(struct usb_composite_dev *cdev)
{
	int i,j;
	struct f_uvc_opts *uvc_opts;
	struct uvc_descriptor_header ** fs_control_cls = NULL;
	struct uvc_descriptor_header ** ss_control_cls = NULL;
	struct uvc_descriptor_header ** fs_streaming_cls = NULL;
	struct uvc_descriptor_header ** hs_streaming_cls = NULL;
	struct uvc_descriptor_header ** ss_streaming_cls = NULL;

	if (MULTI_STREAM_NUM > MAX_STREAM_SUPPORT)
	{
		BUG();
	}
#ifdef CONFIG_SS_GADGET_UVC_MULTI_STREAM
	uvc_function_enable = 1;
#endif

	if (!uvc_fs_control_cls)
	{
		uvc_fs_control_cls =
			kzalloc(sizeof(void*) * uvc_function_enable * MAX_CONTROL_SIZE, GFP_KERNEL);
		uvc_ss_control_cls =
			kzalloc(sizeof(void*) * uvc_function_enable * MAX_CONTROL_SIZE, GFP_KERNEL);
	}

	if (!uvc_fs_streaming_cls)
	{
		uvc_fs_streaming_cls =
			kzalloc(sizeof(void*) * uvc_function_enable * MAX_FORMAT_COUNT, GFP_KERNEL);
		uvc_hs_streaming_cls =
			kzalloc(sizeof(void*) * uvc_function_enable * MAX_FORMAT_COUNT, GFP_KERNEL);
		uvc_ss_streaming_cls =
			kzalloc(sizeof(void*) * uvc_function_enable * MAX_FORMAT_COUNT, GFP_KERNEL);
	}
	fi_uvc = kzalloc(uvc_function_enable * sizeof(*fi_uvc), GFP_KERNEL);
	for (i=0; i < uvc_function_enable; i++)
	{
		uvc_opts = NULL;

		fi_uvc[i] = usb_get_function_instance("uvc");
		if (IS_ERR(fi_uvc[i]))
			return PTR_ERR(fi_uvc[i]);

		uvc_opts = container_of(fi_uvc[i], struct f_uvc_opts, func_inst);
		if(IS_ERR(uvc_opts))
			return PTR_ERR(uvc_opts);

#ifdef CONFIG_SS_GADGET_UVC_MULTI_STREAM
		for (j=0; j < MULTI_STREAM_NUM; j++)
		{
			uvc_opts->streaming_interval[j] = streaming_interval[j];
			uvc_opts->streaming_maxpacket[j] = streaming_maxpacket[j];
			uvc_opts->streaming_maxburst[j] = streaming_maxburst[j];
			uvc_opts->streaming_name[j] = streaming_name[j];
		}
#else
		uvc_opts->streaming_interval = streaming_interval[i];
		uvc_opts->streaming_maxpacket = streaming_maxpacket[i];
		uvc_opts->streaming_maxburst = streaming_maxburst[i];
		uvc_opts->streaming_name = streaming_name[i];
#endif
		uvc_opts->bulk_streaming_ep   = bulk_streaming_ep;

		/*
		 * controls:
		 */
		fs_control_cls = &uvc_fs_control_cls[i * MAX_CONTROL_SIZE];
		ss_control_cls = &uvc_ss_control_cls[i * MAX_CONTROL_SIZE];

		for (j=0;j < MAX_STREAM_SUPPORT;j++)
		{
			memcpy(&uvc_output_terminal[j], &uvc_output_terminal[0], sizeof(uvc_output_terminal[0]));
			uvc_output_terminal[j].bTerminalID = UVC_OT_ID + j;
		}

		{
			fs_control_cls[0] = (struct uvc_descriptor_header *) &uvc_control_header;
			fs_control_cls[1] = (struct uvc_descriptor_header *) &uvc_camera_terminal;
			fs_control_cls[2] = (struct uvc_descriptor_header *) &uvc_processing;
			fs_control_cls[3] = (struct uvc_descriptor_header *) &uvc_extension_unit1;
			fs_control_cls[4] = (struct uvc_descriptor_header *) &uvc_extension_unit2;
			fs_control_cls[5] = (struct uvc_descriptor_header *) &uvc_output_terminal[i];
			for (j=1; j < MULTI_STREAM_NUM; j++)
			{
				fs_control_cls[5+j] = (struct uvc_descriptor_header *) &uvc_output_terminal[j];
			}
			fs_control_cls[5+MULTI_STREAM_NUM] = NULL;

			ss_control_cls[0] = (struct uvc_descriptor_header *) &uvc_control_header;
			ss_control_cls[1] = (struct uvc_descriptor_header *) &uvc_camera_terminal;
			ss_control_cls[2] = (struct uvc_descriptor_header *) &uvc_processing;
			ss_control_cls[3] = (struct uvc_descriptor_header *) &uvc_extension_unit1;
			ss_control_cls[4] = (struct uvc_descriptor_header *) &uvc_extension_unit2;
			ss_control_cls[5] = (struct uvc_descriptor_header *) &uvc_output_terminal[i];
			for (j=1;j < MULTI_STREAM_NUM;j++)
			{
				ss_control_cls[5+j] = (struct uvc_descriptor_header *) &uvc_output_terminal[j];
			}
			ss_control_cls[5+MULTI_STREAM_NUM] = NULL;
		}

		uvc_opts->fs_control =
				(const struct uvc_descriptor_header * const *)fs_control_cls;
		uvc_opts->ss_control =
				 (const struct uvc_descriptor_header * const *)ss_control_cls;

		/*
		 * Formats:
		 */
		fs_streaming_cls = &uvc_fs_streaming_cls[i * MAX_FORMAT_COUNT];
		hs_streaming_cls = &uvc_hs_streaming_cls[i * MAX_FORMAT_COUNT];
		ss_streaming_cls = &uvc_ss_streaming_cls[i * MAX_FORMAT_COUNT];
		memcpy(&uvc_input_header[i], &uvc_input_header[0], sizeof(uvc_input_header[i]));
		uvc_input_header[i].bTerminalLink = UVC_OT_ID + i;

		{
			memcpy(fs_streaming_cls, uvc_streaming_cls_std, sizeof(uvc_streaming_cls_std));
			memcpy(hs_streaming_cls, uvc_streaming_cls_std, sizeof(uvc_streaming_cls_std));
			memcpy(ss_streaming_cls, uvc_streaming_cls_std, sizeof(uvc_streaming_cls_std));
		}
		fs_streaming_cls[0] =
				(struct uvc_descriptor_header *) &uvc_input_header[i];
		hs_streaming_cls[0] =
				(struct uvc_descriptor_header *) &uvc_input_header[i];
		ss_streaming_cls[0] =
				(struct uvc_descriptor_header *) &uvc_input_header[i];

		uvc_opts->fs_streaming =
				(const struct uvc_descriptor_header * const *)fs_streaming_cls;
		uvc_opts->hs_streaming =
				(const struct uvc_descriptor_header * const *)hs_streaming_cls;
		uvc_opts->ss_streaming =
				(const struct uvc_descriptor_header * const *)ss_streaming_cls;
	}
	uvc_set_trace_param(trace);
	return 0;
}
#endif

#if defined(CONFIG_USB_WEBCAM_DFU)
static int __init dfu_bind_config(struct usb_configuration *c)
{
	int status;
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
static int __init dfu_bind(struct usb_composite_dev *cdev)
{
	struct f_dfu_opts	*dfu_opts;

	fi_dfu = usb_get_function_instance("dfu");
	if (IS_ERR(fi_dfu))
		return PTR_ERR(fi_dfu);

	dfu_opts = container_of(fi_dfu, struct f_dfu_opts, func_inst);
	if(IS_ERR(dfu_opts))
		return PTR_ERR(dfu_opts);

	return 0;
}
static int __exit dfu_unbind(struct usb_composite_dev *cdev)
{
	if (!IS_ERR_OR_NULL(f_dfu))
	{
		usb_put_function(f_dfu);
		f_dfu = NULL;
	}
	if (!IS_ERR_OR_NULL(fi_dfu))
	{
		usb_put_function_instance(fi_dfu);
		fi_dfu = NULL;
	}
	return 0;
}
#endif

#if defined(CONFIG_USB_WEBCAM_RNDIS)
static int __init rndis_bind_config(struct usb_configuration *c)
{
	int status = 0;

	f_rndis = usb_get_function(fi_rndis);
	if (IS_ERR(f_rndis))
		return PTR_ERR(f_rndis);

	status = usb_add_function(c, f_rndis);
	if (status < 0)
		usb_put_function(f_rndis);

	return status;
}

static int __init rndis_bind(struct usb_composite_dev *cdev)
{
	struct f_rndis_opts	*rndis_opts = NULL;
	struct net_device	*net;
	int			status;

	fi_rndis = usb_get_function_instance("rndis");
	if (IS_ERR(fi_rndis)) {
		status = PTR_ERR(fi_rndis);
		goto fail;
	}

	/* set up main config label and device descriptor */
	rndis_opts = container_of(fi_rndis, struct f_rndis_opts,
				 func_inst);

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

static int __exit rndis_unbind(struct usb_composite_dev *cdev)
{
	if (!IS_ERR_OR_NULL(f_rndis))
	{
		usb_put_function(f_rndis);
		f_rndis = NULL;
	}
	if (!IS_ERR_OR_NULL(fi_rndis))
	{
		usb_put_function_instance(fi_rndis);
		fi_rndis = NULL;
	}

	return 0;
}
#endif

#if defined(CONFIG_USB_WEBCAM_UAC)
static int __init audio_bind_config(struct usb_configuration *c)
{
	int status;
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
static int __init audio_bind(struct usb_composite_dev *cdev)
{
#ifndef CONFIG_USB_WEBCAM_UAC1_LEGACY
	struct f_uac1_opts	*uac1_opts;
	fi_uac1 = usb_get_function_instance("uac1");
#else
	struct f_uac1_legacy_opts	*uac1_opts;
	fi_uac1 = usb_get_function_instance("uac1_legacy");
#endif
	if (IS_ERR(fi_uac1))
		return PTR_ERR(fi_uac1);

#ifndef CONFIG_USB_WEBCAM_UAC1_LEGACY
	uac1_opts = container_of(fi_uac1, struct f_uac1_opts, func_inst);

	if(ENABLE_MICROPHONE== uac_function_enable||ENABLE_MIC_AND_SPK==uac_function_enable)
		uac1_opts->p_chmask = p_chmask;
	else
		uac1_opts->p_chmask = 0;

	uac1_opts->p_mpsize = p_mpsize;
	uac1_opts->p_srate = p_srate;
	uac1_opts->p_ssize = p_ssize;


	if(ENABLE_SPEAKER== uac_function_enable||ENABLE_MIC_AND_SPK==uac_function_enable)
		uac1_opts->c_chmask = c_chmask;
	else
		uac1_opts->c_chmask = 0;

	uac1_opts->c_mpsize = c_mpsize;
	uac1_opts->c_srate = c_srate;
	uac1_opts->c_ssize = c_ssize;
	uac1_opts->req_number = UAC1_DEF_REQ_NUM;
	uac1_opts->audio_play_mode = uac_function_enable;
#else /* CONFIG_USB_WEBCAM_UAC1_LEGACY */
	uac1_opts = container_of(fi_uac1, struct f_uac1_legacy_opts, func_inst);
	if(IS_ERR(uac1_opts))
		return PTR_ERR(uac1_opts);

	uac1_opts->fn_play = fn_play;
	uac1_opts->fn_cap = fn_cap;
	uac1_opts->fn_cntl = fn_cntl;
	uac1_opts->playback_channel_count = playback_channel_count;
	uac1_opts->playback_sample_rate = playback_sample_rate;
	uac1_opts->capture_channel_count = capture_channel_count;
	uac1_opts->capture_sample_rate = capture_sample_rate;
	uac1_opts->out_req_buf_size = out_req_buf_size;
	uac1_opts->out_req_count = out_req_count;
	uac1_opts->audio_playback_buf_size = audio_playback_buf_size;
	uac1_opts->in_req_buf_size = in_req_buf_size;
	uac1_opts->in_req_count = in_req_count;
	uac1_opts->audio_capture_period_size = audio_capture_period_size;
	uac1_opts->audio_capture_buf_size = audio_capture_buf_size;
	uac1_opts->audio_play_mode = uac_function_enable;
#endif
	return 0;
}
static int __exit audio_unbind(struct usb_composite_dev *cdev)
{
	if (!IS_ERR_OR_NULL(f_uac1))
	{
		usb_put_function(f_uac1);
		f_uac1 = NULL;
	}
	if (!IS_ERR_OR_NULL(fi_uac1))
	{
		usb_put_function_instance(fi_uac1);
		fi_uac1 = NULL;
	}
	return 0;
}
#endif

static struct usb_configuration webcam_config_driver = {
	.label			= webcam_config_label,
	.bConfigurationValue	= 1,
	.iConfiguration		= 0, /* dynamic */
	.bmAttributes		= USB_CONFIG_ATT_SELFPOWER,
	.MaxPower		= CONFIG_USB_GADGET_VBUS_DRAW,
};

static int __init webcam_bind_config(struct usb_configuration *c)
{
	int ret = 0;

#if defined(CONFIG_USB_WEBCAM_RNDIS)
	if(rndis_function_enable)
	{
		ret = rndis_bind_config(c);
		if(ret < 0)
			return ret;
	}
#endif

#if defined(CONFIG_USB_WEBCAM_UVC)
	if(uvc_function_enable)
	{
		ret = video_bind_config(c);
		if(ret < 0)
			return ret;
	}
#endif

#if defined(CONFIG_USB_WEBCAM_UAC)
	if(uac_function_enable)
	{
		ret = audio_bind_config(c);
		if(ret < 0)
			return ret;
	}
#endif

#if defined(CONFIG_USB_WEBCAM_DFU)
	if(dfu_function_enable)
	{
		ret = dfu_bind_config(c);
		if(ret < 0)
			return ret;
	}
#endif

	return ret;
}
static int __init webcam_bind(struct usb_composite_dev *cdev)
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

#if defined(CONFIG_USB_WEBCAM_RNDIS)
	if(rndis_function_enable)
	{
		ret = rndis_bind(cdev);
		if(ret < 0)
		{
			goto error;
		}
	}
#endif

#if defined(CONFIG_USB_WEBCAM_UVC)
	if(uvc_function_enable)
	{
		ret = video_bind(cdev);
		if(ret < 0)
			goto error;
	}
#endif

#if defined(CONFIG_USB_WEBCAM_UAC)
	if(uac_function_enable)
	{
		ret = audio_bind(cdev);
		if(ret < 0)
			goto error;
	}
#endif

#if defined(CONFIG_USB_WEBCAM_DFU)
	if(dfu_function_enable)
	{
		ret = dfu_bind(cdev);
		if(ret < 0)
			goto error;
	}
#endif

	/* Register our configuration. */
	if ((ret = usb_add_config(cdev, &webcam_config_driver,
					webcam_bind_config)) < 0)
	{
		goto error;
	}

	usb_composite_overwrite_options(cdev, &coverwrite);
	INFO(cdev, "Webcam Video Gadget\n");
	return 0;

error:
#if defined(CONFIG_USB_WEBCAM_DFU)
	dfu_unbind(cdev);
#endif

#if defined(CONFIG_USB_WEBCAM_UAC)
	audio_unbind(cdev);
#endif

#if defined(CONFIG_USB_WEBCAM_UVC)
	video_unbind(cdev);
#endif

#if defined(CONFIG_USB_WEBCAM_RNDIS)
	rndis_unbind(cdev);
#endif
	return ret;
}
static int
webcam_unbind(struct usb_composite_dev *cdev)
{
	int ret = 0;

#if defined(CONFIG_USB_WEBCAM_RNDIS)
	if(rndis_function_enable)
		ret = rndis_unbind(cdev);
#endif

#if defined(CONFIG_USB_WEBCAM_UVC)
	if(uvc_function_enable)
		ret = video_unbind(cdev);
#endif

#if defined(CONFIG_USB_WEBCAM_UAC)
	if(uac_function_enable)
		ret = audio_unbind(cdev);
#endif

#if defined(CONFIG_USB_WEBCAM_DFU)
	if(dfu_function_enable)
		ret = dfu_unbind(cdev);
#endif
	return ret;
}
/* --------------------------------------------------------------------------
 * Driver
 */

static struct usb_composite_driver webcam_driver = {
	.name		= "g_webcam",
	.dev		= &webcam_device_descriptor,
	.strings	= webcam_device_strings,
	.max_speed	= USB_SPEED_SUPER,
	.bind		= webcam_bind,
	.unbind		= webcam_unbind,
};

module_usb_composite_driver(webcam_driver);

MODULE_AUTHOR("Laurent Pinchart");
MODULE_DESCRIPTION("Webcam Video Gadget");
MODULE_LICENSE("GPL");
MODULE_VERSION("0.1.0");

