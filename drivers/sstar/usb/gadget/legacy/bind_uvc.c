/*
 * bind_uvc.c- Sigmastar
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
#include <linux/usb/video.h>

#include "u_uvc.h"
#include "bind_uvc.h"

/* module parameters specific to the Video streaming endpoint */
static unsigned int nr_name;
static char *streaming_name[MAX_STREAM_SUPPORT] = {
	DEFAULT_STREAM_NAME, DEFAULT_STREAM_NAME, DEFAULT_STREAM_NAME,
	DEFAULT_STREAM_NAME, DEFAULT_STREAM_NAME, DEFAULT_STREAM_NAME
};
module_param_array(streaming_name, charp, &nr_name, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(streaming_name, "Uvc Stream Name");

static unsigned int nr_interval;
static unsigned int streaming_interval[MAX_STREAM_SUPPORT] = {
	1, 1, 1, 1, 1, 1
};
module_param_array(streaming_interval, uint, &nr_interval, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(streaming_interval, "1 - 16");

static unsigned int nr_maxpacket;
static unsigned int streaming_maxpacket[MAX_STREAM_SUPPORT] = {
	1024 * 3, 1024 * 3, 1024 * 3, 1024 * 3, 1024 * 3, 1024 * 3
};
module_param_array(streaming_maxpacket, uint, &nr_maxpacket, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(streaming_maxpacket,
		 "ISOC: 1 - 1023 (FS), 1 - 3072 (hs/ss) / "
		 "BULK: 1 - 64 (FS), 1 - 512 (HS), 1 - 1024 (SS)");

static unsigned int nr_maxburst;
static unsigned int streaming_maxburst[MAX_STREAM_SUPPORT] = { 13, 13, 13,
							       13, 0,  0 };
module_param_array(streaming_maxburst, uint, &nr_maxburst, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(streaming_maxburst, "0 - 15 (ss only)");

static bool bulk_streaming_ep = 0;
module_param(bulk_streaming_ep, bool, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(bulk_streaming_ep, "0 (Use ISOC video streaming ep) / "
				    "1 (Use BULK video streaming ep)");

static int uvc_function_enable = 1;
module_param(uvc_function_enable, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(uvc_function_enable, "UVC Function Enable,"
				      "0: Disable UVC, 1: Enable UVC");

static struct usb_function_instance **fi_uvc;
static struct usb_function **f_uvc;

#define UVC_HEADER_DESCRIPTOR_EX(num) UVC_HEADER_DESCRIPTOR(num)

DECLARE_UVC_HEADER_DESCRIPTOR(MULTI_STREAM_NUM);

static struct UVC_HEADER_DESCRIPTOR_EX(MULTI_STREAM_NUM) uvc_control_header = {
	.bLength = UVC_DT_HEADER_SIZE(MULTI_STREAM_NUM),
	.bDescriptorType = USB_DT_CS_INTERFACE,
	.bDescriptorSubType = UVC_VC_HEADER,
#if (USB_VIDEO_CLASS_VERSION == 0x150)
	.bcdUVC = cpu_to_le16(0x0150),
#else
	.bcdUVC = cpu_to_le16(0x0100),
#endif
	.wTotalLength = 0, /* dynamic */
	.dwClockFrequency = cpu_to_le32(48000000),
	.bInCollection = 0, /* dynamic */
	.baInterfaceNr[0] = 0, /* dynamic */
};

static struct uvc_camera_terminal_descriptor uvc_camera_terminal = {
	.bLength = UVC_DT_CAMERA_TERMINAL_SIZE(3),
	.bDescriptorType = USB_DT_CS_INTERFACE,
	.bDescriptorSubType = UVC_VC_INPUT_TERMINAL,
	.bTerminalID = UVC_IT_ID,
	.wTerminalType = cpu_to_le16(0x0201),
	.bAssocTerminal = 0,
	.iTerminal = 0,
	.wObjectiveFocalLengthMin = cpu_to_le16(0),
	.wObjectiveFocalLengthMax = cpu_to_le16(0),
	.wOcularFocalLength = cpu_to_le16(0),
	.bControlSize = 3,
	.bmControls[0] = 1,
	.bmControls[1] = 0,
#if (USB_VIDEO_CLASS_VERSION == 0x150) && defined(CONFIG_WINDOWS_HELLO_SUPPORT)
	.bmControls[2] = 0x20,
#else
	.bmControls[2] = 0,
#endif
};

static struct UVC_EXTENSION_UNIT_DESCRIPTOR(1, 2) uvc_extension_unit1 = {
	.bLength = UVC_DT_EXTENSION_UNIT_SIZE(1, 2),
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
	.bLength = UVC_DT_EXTENSION_UNIT_SIZE(1, 2),
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

#ifdef CONFIG_WINDOWS_HELLO_SUPPORT
static struct UVC_EXTENSION_UNIT_DESCRIPTOR(1, 2) uvc_extension_unit3 = {
	.bLength = UVC_DT_EXTENSION_UNIT_SIZE(1, 2),
	.bDescriptorType = USB_DT_CS_INTERFACE,
	.bDescriptorSubType = UVC_VC_EXTENSION_UNIT,
	.bUnitID = UVC_EU3_ID,
	.guidExtensionCode = UVC_EU3_GUID,
	.bNumControls = 0x02,
	.bNrInPins = 0x01,
	.baSourceID[0] = UVC_EU2_ID,
	.bControlSize = 0x02,
	.bmControls[0] = 0x20,
	.bmControls[1] = 0x01,
	.iExtension = 0x00,
};
#endif

static struct uvc_processing_unit_descriptor uvc_processing = {
	.bLength = UVC_DT_PROCESSING_UNIT_SIZE(2),
	.bDescriptorType = USB_DT_CS_INTERFACE,
	.bDescriptorSubType = UVC_VC_PROCESSING_UNIT,
	.bUnitID = UVC_PU_ID,
	.bSourceID = UVC_IT_ID,
	.wMaxMultiplier = cpu_to_le16(16 * 1024),
	.bControlSize = 2,
	.bmControls[0] = 1,
	.bmControls[1] = 0,
#if (USB_VIDEO_CLASS_VERSION == 0x150)
	.bmControls[2] = 0,
#endif
	.iProcessing = 0,
};

static struct uvc_output_terminal_descriptor
	uvc_output_terminal[MAX_STREAM_SUPPORT] = { {
		.bLength = UVC_DT_OUTPUT_TERMINAL_SIZE,
		.bDescriptorType = USB_DT_CS_INTERFACE,
		.bDescriptorSubType = UVC_VC_OUTPUT_TERMINAL,
		.bTerminalID = UVC_OT_ID,
		.wTerminalType = cpu_to_le16(0x0101),
		.bAssocTerminal = 0,
#ifdef CONFIG_WINDOWS_HELLO_SUPPORT
		.bSourceID = UVC_EU3_ID,
#else
		.bSourceID = UVC_EU2_ID,
#endif
		.iTerminal = 0,
	} };

DECLARE_UVC_INPUT_HEADER_DESCRIPTOR(1, 5);
DECLARE_UVC_FRAME_UNCOMPRESSED(3);
DECLARE_UVC_FRAME_FRAMEBASE(3);
DECLARE_UVC_STILL_IMAGE_FRAME_DESCRIPTOR(1, 1);
#if (USB_VIDEO_CLASS_VERSION == 0x150)
DECLARE_UVC_FRAME_H264(3);
#endif

#define SUPPORT_MJPG
#ifdef CONFIG_WINDOWS_HELLO_SUPPORT
#define SUPPORT_IR
#else
#define SUPPORT_YUY2
#define SUPPORT_NV12
#define SUPPORT_H264
#define SUPPORT_H265
#endif

static struct UVC_INPUT_HEADER_DESCRIPTOR(1, 5)
	uvc_input_header[MAX_STREAM_SUPPORT] = { {
		.bLength = UVC_DT_INPUT_HEADER_SIZE(1, 5),
		.bDescriptorType = USB_DT_CS_INTERFACE,
		.bDescriptorSubType = UVC_VS_INPUT_HEADER,
		.bNumFormats = 5,
		.wTotalLength = 0, /* dynamic */
		.bEndpointAddress = 0, /* dynamic */
		.bmInfo = 0,
		.bTerminalLink = UVC_OT_ID,
		.bStillCaptureMethod = 0x02,
		.bTriggerSupport = 0,
		.bTriggerUsage = 0,
		.bControlSize = 1,
		.bmaControls[0][0] = 0,
		.bmaControls[1][0] = 4,
		.bmaControls[2][0] = 4,
		.bmaControls[3][0] = 4,
	} };

#ifdef SUPPORT_YUY2
static struct uvc_format_uncompressed uvc_format_yuy2 = {
	.bLength = UVC_DT_FORMAT_UNCOMPRESSED_SIZE,
	.bDescriptorType = USB_DT_CS_INTERFACE,
	.bDescriptorSubType = UVC_VS_FORMAT_UNCOMPRESSED,
	.bFormatIndex = 1,
	.bNumFrameDescriptors = 6,
	.guidFormat = { 'Y', 'U', 'Y', '2', 0x00, 0x00, 0x10, 0x00, 0x80, 0x00,
			0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71 },
	.bBitsPerPixel = 16,
	.bDefaultFrameIndex = 1,
	.bAspectRatioX = 0,
	.bAspectRatioY = 0,
	.bmInterfaceFlags = 0,
	.bCopyProtect = 0,
};

static struct UVC_FRAME_UNCOMPRESSED(3) uvc_frame_yuy2_240p = {
	.bLength = UVC_DT_FRAME_UNCOMPRESSED_SIZE(3),
	.bDescriptorType = USB_DT_CS_INTERFACE,
	.bDescriptorSubType = UVC_VS_FRAME_UNCOMPRESSED,
	.bFrameIndex = 1,
	.bmCapabilities = 0,
	.wWidth = cpu_to_le16(320),
	.wHeight = cpu_to_le16(240),
	.dwMinBitRate = cpu_to_le32(320 * 240 * 2.0 * 8 * 10),
	.dwMaxBitRate = cpu_to_le32(320 * 240 * 2.0 * 8 * 30),
	.dwMaxVideoFrameBufferSize = cpu_to_le32(320 * 240 * 2.0),
	.dwDefaultFrameInterval = cpu_to_le32(333333),
	.bFrameIntervalType = 3,
	.dwFrameInterval[0] = cpu_to_le32(333333),
	.dwFrameInterval[1] = cpu_to_le32(666666),
	.dwFrameInterval[2] = cpu_to_le32(1000000),
};
static struct UVC_FRAME_UNCOMPRESSED(3) uvc_frame_yuy2_480p = {
	.bLength = UVC_DT_FRAME_UNCOMPRESSED_SIZE(3),
	.bDescriptorType = USB_DT_CS_INTERFACE,
	.bDescriptorSubType = UVC_VS_FRAME_UNCOMPRESSED,
	.bFrameIndex = 2,
	.bmCapabilities = 0,
	.wWidth = cpu_to_le16(640),
	.wHeight = cpu_to_le16(480),
	.dwMinBitRate = cpu_to_le32(640 * 480 * 2.0 * 8 * 10),
	.dwMaxBitRate = cpu_to_le32(640 * 480 * 2.0 * 8 * 30),
	.dwMaxVideoFrameBufferSize = cpu_to_le32(640 * 480 * 2.0),
	.dwDefaultFrameInterval = cpu_to_le32(333333),
	.bFrameIntervalType = 3,
	.dwFrameInterval[0] = cpu_to_le32(333333),
	.dwFrameInterval[1] = cpu_to_le32(666666),
	.dwFrameInterval[2] = cpu_to_le32(1000000),
};
static struct UVC_FRAME_UNCOMPRESSED(3) uvc_frame_yuy2_720p = {
	.bLength = UVC_DT_FRAME_UNCOMPRESSED_SIZE(3),
	.bDescriptorType = USB_DT_CS_INTERFACE,
	.bDescriptorSubType = UVC_VS_FRAME_UNCOMPRESSED,
	.bFrameIndex = 3,
	.bmCapabilities = 0,
	.wWidth = cpu_to_le16(1280),
	.wHeight = cpu_to_le16(720),
	.dwMinBitRate = cpu_to_le32(1280 * 720 * 2.0 * 8 * 10),
	.dwMaxBitRate = cpu_to_le32(1280 * 720 * 2.0 * 8 * 30),
	.dwMaxVideoFrameBufferSize = cpu_to_le32(1280 * 720 * 2.0),
	.dwDefaultFrameInterval = cpu_to_le32(333333),
	.bFrameIntervalType = 3,
	.dwFrameInterval[0] = cpu_to_le32(333333),
	.dwFrameInterval[1] = cpu_to_le32(666666),
	.dwFrameInterval[2] = cpu_to_le32(1000000),
};
static struct UVC_FRAME_UNCOMPRESSED(3) uvc_frame_yuy2_1080p = {
	.bLength = UVC_DT_FRAME_UNCOMPRESSED_SIZE(3),
	.bDescriptorType = USB_DT_CS_INTERFACE,
	.bDescriptorSubType = UVC_VS_FRAME_UNCOMPRESSED,
	.bFrameIndex = 4,
	.bmCapabilities = 0,
	.wWidth = cpu_to_le16(1920),
	.wHeight = cpu_to_le16(1080),
	.dwMinBitRate = cpu_to_le32(1920 * 1080 * 2.0 * 8 * 10),
	.dwMaxBitRate = cpu_to_le32(1920 * 1080 * 2.0 * 8 * 30),
	.dwMaxVideoFrameBufferSize = cpu_to_le32(1920 * 1080 * 2.0),
	.dwDefaultFrameInterval = cpu_to_le32(333333),
	.bFrameIntervalType = 3,
	.dwFrameInterval[0] = cpu_to_le32(333333),
	.dwFrameInterval[1] = cpu_to_le32(666666),
	.dwFrameInterval[2] = cpu_to_le32(1000000),
};
static struct UVC_FRAME_UNCOMPRESSED(3) uvc_frame_yuy2_2kp = {
	.bLength = UVC_DT_FRAME_UNCOMPRESSED_SIZE(3),
	.bDescriptorType = USB_DT_CS_INTERFACE,
	.bDescriptorSubType = UVC_VS_FRAME_UNCOMPRESSED,
	.bFrameIndex = 5,
	.bmCapabilities = 0,
	.wWidth = cpu_to_le16(2560),
	.wHeight = cpu_to_le16(1440),
	.dwMinBitRate = cpu_to_le32(2560 * 1440 * 2.0 * 8 * 10),
	.dwMaxBitRate = cpu_to_le32(2560 * 1440 * 2.0 * 8 * 10),
	.dwMaxVideoFrameBufferSize = cpu_to_le32(2560 * 1440 * 2.0),
	.dwDefaultFrameInterval = cpu_to_le32(333333),
	.bFrameIntervalType = 3,
	.dwFrameInterval[0] = cpu_to_le32(333333),
	.dwFrameInterval[1] = cpu_to_le32(666666),
	.dwFrameInterval[2] = cpu_to_le32(1000000),
};
static struct UVC_FRAME_UNCOMPRESSED(3) uvc_frame_yuy2_4kp = {
	.bLength = UVC_DT_FRAME_UNCOMPRESSED_SIZE(3),
	.bDescriptorType = USB_DT_CS_INTERFACE,
	.bDescriptorSubType = UVC_VS_FRAME_UNCOMPRESSED,
	.bFrameIndex = 6,
	.bmCapabilities = 0,
	.wWidth = cpu_to_le16(3840),
	.wHeight = cpu_to_le16(2160),
	.dwMinBitRate = cpu_to_le32(3840 * 2160 * 2.0 * 8 * 10),
	.dwMaxBitRate = cpu_to_le32(3840 * 2160 * 2.0 * 8 * 10),
	.dwMaxVideoFrameBufferSize = cpu_to_le32(3840 * 2160 * 2.0),
	.dwDefaultFrameInterval = cpu_to_le32(666666),
	.bFrameIntervalType = 3,
	.dwFrameInterval[0] = cpu_to_le32(333333),
	.dwFrameInterval[1] = cpu_to_le32(666666),
	.dwFrameInterval[2] = cpu_to_le32(1000000),
};

static struct UVC_STILL_IMAGE_FRAME_DESCRIPTOR(1,
					       1) uvc_frame_yuy2_still_image = {
	.bLength = UVC_DT_STILL_IMAGE_FRAME_SIZE(1, 1),
	.bDescriptorType = USB_DT_CS_INTERFACE,
	.bDescriptorSubtype = UVC_VS_STILL_IMAGE_FRAME,
	.bEndpointAddress = 0, //use method 2, should be set to zero
	.bNumImageSizePatterns = 1,
	.wWidthwHeight[0] = { cpu_to_le16(1920), cpu_to_le16(1080) },
	.bNumCompressionPattern = 1,
	.bCompression[0] = 3,
};

#define UVC_DESCRIPTOR_HEADERS_OF_YUY2_FRAME                                   \
	(struct uvc_descriptor_header *)&uvc_format_yuy2,                      \
		(struct uvc_descriptor_header *)&uvc_frame_yuy2_240p,          \
		(struct uvc_descriptor_header *)&uvc_frame_yuy2_480p,          \
		(struct uvc_descriptor_header *)&uvc_frame_yuy2_720p,          \
		(struct uvc_descriptor_header *)&uvc_frame_yuy2_1080p,         \
		(struct uvc_descriptor_header *)&uvc_frame_yuy2_2kp,           \
		(struct uvc_descriptor_header *)&uvc_frame_yuy2_4kp,           \
		(struct uvc_descriptor_header *)&uvc_frame_yuy2_still_image,   \
		(struct uvc_descriptor_header *)&uvc_color_matching,
#else
#define UVC_DESCRIPTOR_HEADERS_OF_YUY2_FRAME
#endif
#ifdef SUPPORT_NV12
static struct uvc_format_uncompressed uvc_format_nv12 = {
	.bLength = UVC_DT_FORMAT_UNCOMPRESSED_SIZE,
	.bDescriptorType = USB_DT_CS_INTERFACE,
	.bDescriptorSubType = UVC_VS_FORMAT_UNCOMPRESSED,
	.bFormatIndex = 2,
	.bNumFrameDescriptors = 6,
	.guidFormat = { 'N', 'V', '1', '2', 0x00, 0x00, 0x10, 0x00, 0x80, 0x00,
			0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71 },
	.bBitsPerPixel = 12,
	.bDefaultFrameIndex = 1,
	.bAspectRatioX = 0,
	.bAspectRatioY = 0,
	.bmInterfaceFlags = 0,
	.bCopyProtect = 0,
};
static struct UVC_FRAME_UNCOMPRESSED(3) uvc_frame_nv12_240p = {
	.bLength = UVC_DT_FRAME_UNCOMPRESSED_SIZE(3),
	.bDescriptorType = USB_DT_CS_INTERFACE,
	.bDescriptorSubType = UVC_VS_FRAME_UNCOMPRESSED,
	.bFrameIndex = 1,
	.bmCapabilities = 0,
	.wWidth = cpu_to_le16(320),
	.wHeight = cpu_to_le16(240),
	.dwMinBitRate = cpu_to_le32(320 * 240 * 1.5 * 8 * 10),
	.dwMaxBitRate = cpu_to_le32(320 * 240 * 1.5 * 8 * 30),
	.dwMaxVideoFrameBufferSize = cpu_to_le32(320 * 240 * 1.5),
	.dwDefaultFrameInterval = cpu_to_le32(333333),
	.bFrameIntervalType = 3,
	.dwFrameInterval[0] = cpu_to_le32(333333),
	.dwFrameInterval[1] = cpu_to_le32(666666),
	.dwFrameInterval[2] = cpu_to_le32(1000000),
};
static struct UVC_FRAME_UNCOMPRESSED(3) uvc_frame_nv12_480p = {
	.bLength = UVC_DT_FRAME_UNCOMPRESSED_SIZE(3),
	.bDescriptorType = USB_DT_CS_INTERFACE,
	.bDescriptorSubType = UVC_VS_FRAME_UNCOMPRESSED,
	.bFrameIndex = 2,
	.bmCapabilities = 0,
	.wWidth = cpu_to_le16(640),
	.wHeight = cpu_to_le16(480),
	.dwMinBitRate = cpu_to_le32(640 * 480 * 1.5 * 8 * 10),
	.dwMaxBitRate = cpu_to_le32(640 * 480 * 1.5 * 8 * 30),
	.dwMaxVideoFrameBufferSize = cpu_to_le32(640 * 480 * 1.5),
	.dwDefaultFrameInterval = cpu_to_le32(333333),
	.bFrameIntervalType = 3,
	.dwFrameInterval[0] = cpu_to_le32(333333),
	.dwFrameInterval[1] = cpu_to_le32(666666),
	.dwFrameInterval[2] = cpu_to_le32(1000000),
};
static struct UVC_FRAME_UNCOMPRESSED(3) uvc_frame_nv12_720p = {
	.bLength = UVC_DT_FRAME_UNCOMPRESSED_SIZE(3),
	.bDescriptorType = USB_DT_CS_INTERFACE,
	.bDescriptorSubType = UVC_VS_FRAME_UNCOMPRESSED,
	.bFrameIndex = 3,
	.bmCapabilities = 0,
	.wWidth = cpu_to_le16(1280),
	.wHeight = cpu_to_le16(720),
	.dwMinBitRate = cpu_to_le32(1280 * 720 * 1.5 * 8 * 10),
	.dwMaxBitRate = cpu_to_le32(1280 * 720 * 1.5 * 8 * 30),
	.dwMaxVideoFrameBufferSize = cpu_to_le32(1280 * 720 * 1.5),
	.dwDefaultFrameInterval = cpu_to_le32(333333),
	.bFrameIntervalType = 3,
	.dwFrameInterval[0] = cpu_to_le32(333333),
	.dwFrameInterval[1] = cpu_to_le32(666666),
	.dwFrameInterval[2] = cpu_to_le32(1000000),
};
static struct UVC_FRAME_UNCOMPRESSED(3) uvc_frame_nv12_1080p = {
	.bLength = UVC_DT_FRAME_UNCOMPRESSED_SIZE(3),
	.bDescriptorType = USB_DT_CS_INTERFACE,
	.bDescriptorSubType = UVC_VS_FRAME_UNCOMPRESSED,
	.bFrameIndex = 4,
	.bmCapabilities = 0,
	.wWidth = cpu_to_le16(1920),
	.wHeight = cpu_to_le16(1080),
	.dwMinBitRate = cpu_to_le32(1920 * 1080 * 1.5 * 8 * 10),
	.dwMaxBitRate = cpu_to_le32(1920 * 1080 * 1.5 * 8 * 30),
	.dwMaxVideoFrameBufferSize = cpu_to_le32(1920 * 1080 * 1.5),
	.dwDefaultFrameInterval = cpu_to_le32(333333),
	.bFrameIntervalType = 3,
	.dwFrameInterval[0] = cpu_to_le32(333333),
	.dwFrameInterval[1] = cpu_to_le32(666666),
	.dwFrameInterval[2] = cpu_to_le32(1000000),
};
static struct UVC_FRAME_UNCOMPRESSED(3) uvc_frame_nv12_2kp = {
	.bLength = UVC_DT_FRAME_UNCOMPRESSED_SIZE(3),
	.bDescriptorType = USB_DT_CS_INTERFACE,
	.bDescriptorSubType = UVC_VS_FRAME_UNCOMPRESSED,
	.bFrameIndex = 5,
	.bmCapabilities = 0,
	.wWidth = cpu_to_le16(2560),
	.wHeight = cpu_to_le16(1440),
	.dwMinBitRate = cpu_to_le32(2560 * 1440 * 1.5 * 8 * 10),
	.dwMaxBitRate = cpu_to_le32(2560 * 1440 * 1.5 * 8 * 10),
	.dwMaxVideoFrameBufferSize = cpu_to_le32(2560 * 1440 * 1.5),
	.dwDefaultFrameInterval = cpu_to_le32(333333),
	.bFrameIntervalType = 3,
	.dwFrameInterval[0] = cpu_to_le32(333333),
	.dwFrameInterval[1] = cpu_to_le32(666666),
	.dwFrameInterval[2] = cpu_to_le32(1000000),
};
static struct UVC_FRAME_UNCOMPRESSED(3) uvc_frame_nv12_4kp = {
	.bLength = UVC_DT_FRAME_UNCOMPRESSED_SIZE(3),
	.bDescriptorType = USB_DT_CS_INTERFACE,
	.bDescriptorSubType = UVC_VS_FRAME_UNCOMPRESSED,
	.bFrameIndex = 6,
	.bmCapabilities = 0,
	.wWidth = cpu_to_le16(3840),
	.wHeight = cpu_to_le16(2160),
	.dwMinBitRate = cpu_to_le32(3840 * 2160 * 1.5 * 8 * 10),
	.dwMaxBitRate = cpu_to_le32(3840 * 2160 * 1.5 * 8 * 10),
	.dwMaxVideoFrameBufferSize = cpu_to_le32(3840 * 2160 * 1.5),
	.dwDefaultFrameInterval = cpu_to_le32(666666),
	.bFrameIntervalType = 3,
	.dwFrameInterval[0] = cpu_to_le32(333333),
	.dwFrameInterval[1] = cpu_to_le32(666666),
	.dwFrameInterval[2] = cpu_to_le32(1000000),
};

static struct UVC_STILL_IMAGE_FRAME_DESCRIPTOR(1,
					       1) uvc_frame_nv12_still_image = {
	.bLength = UVC_DT_STILL_IMAGE_FRAME_SIZE(1, 1),
	.bDescriptorType = USB_DT_CS_INTERFACE,
	.bDescriptorSubtype = UVC_VS_STILL_IMAGE_FRAME,
	.bEndpointAddress = 0, //use method 2, should be set to zero
	.bNumImageSizePatterns = 1,
	.wWidthwHeight[0] = { cpu_to_le16(1920), cpu_to_le16(1080) },
	.bNumCompressionPattern = 1,
	.bCompression[0] = 3,
};

#define UVC_DESCRIPTOR_HEADERS_OF_NV12_FRAME                                   \
	(struct uvc_descriptor_header *)&uvc_format_nv12,                      \
		(struct uvc_descriptor_header *)&uvc_frame_nv12_240p,          \
		(struct uvc_descriptor_header *)&uvc_frame_nv12_480p,          \
		(struct uvc_descriptor_header *)&uvc_frame_nv12_720p,          \
		(struct uvc_descriptor_header *)&uvc_frame_nv12_1080p,         \
		(struct uvc_descriptor_header *)&uvc_frame_nv12_2kp,           \
		(struct uvc_descriptor_header *)&uvc_frame_nv12_4kp,           \
		(struct uvc_descriptor_header *)&uvc_frame_nv12_still_image,   \
		(struct uvc_descriptor_header *)&uvc_color_matching,
#else
#define UVC_DESCRIPTOR_HEADERS_OF_NV12_FRAME
#endif

#ifdef SUPPORT_MJPG
static struct uvc_format_mjpeg uvc_format_mjpg = {
	.bLength = UVC_DT_FORMAT_MJPEG_SIZE,
	.bDescriptorType = USB_DT_CS_INTERFACE,
	.bDescriptorSubType = UVC_VS_FORMAT_MJPEG,
	.bFormatIndex = 3,
	.bNumFrameDescriptors = 6,
	.bmFlags = 0,
	.bDefaultFrameIndex = 1,
	.bAspectRatioX = 0,
	.bAspectRatioY = 0,
	.bmInterfaceFlags = 0,
	.bCopyProtect = 0,
};

DECLARE_UVC_FRAME_MJPEG(3);

static struct UVC_FRAME_MJPEG(3) uvc_frame_mjpg_240p = {
	.bLength = UVC_DT_FRAME_MJPEG_SIZE(3),
	.bDescriptorType = USB_DT_CS_INTERFACE,
	.bDescriptorSubType = UVC_VS_FRAME_MJPEG,
	.bFrameIndex = 1,
	.bmCapabilities = 0,
	.wWidth = cpu_to_le16(320),
	.wHeight = cpu_to_le16(240),
	.dwMinBitRate = cpu_to_le32(320 * 240 * 2 * 8 * 10),
	.dwMaxBitRate = cpu_to_le32(320 * 240 * 2 * 8 * 30),
	.dwMaxVideoFrameBufferSize = cpu_to_le32(320 * 240 * 2),
	.dwDefaultFrameInterval = cpu_to_le32(333333),
	.bFrameIntervalType = 3,
	.dwFrameInterval[0] = cpu_to_le32(333333),
	.dwFrameInterval[1] = cpu_to_le32(666666),
	.dwFrameInterval[2] = cpu_to_le32(1000000),
};
static struct UVC_FRAME_MJPEG(3) uvc_frame_mjpg_480p = {
	.bLength = UVC_DT_FRAME_MJPEG_SIZE(3),
	.bDescriptorType = USB_DT_CS_INTERFACE,
	.bDescriptorSubType = UVC_VS_FRAME_MJPEG,
	.bFrameIndex = 2,
	.bmCapabilities = 0,
	.wWidth = cpu_to_le16(640),
	.wHeight = cpu_to_le16(480),
	.dwMinBitRate = cpu_to_le32(640 * 480 * 2 * 8 * 10),
	.dwMaxBitRate = cpu_to_le32(640 * 480 * 2 * 8 * 30),
	.dwMaxVideoFrameBufferSize = cpu_to_le32(640 * 480 * 2),
	.dwDefaultFrameInterval = cpu_to_le32(333333),
	.bFrameIntervalType = 3,
	.dwFrameInterval[0] = cpu_to_le32(333333),
	.dwFrameInterval[1] = cpu_to_le32(666666),
	.dwFrameInterval[2] = cpu_to_le32(1000000),
};
static struct UVC_FRAME_MJPEG(3) uvc_frame_mjpg_720p = {
	.bLength = UVC_DT_FRAME_MJPEG_SIZE(3),
	.bDescriptorType = USB_DT_CS_INTERFACE,
	.bDescriptorSubType = UVC_VS_FRAME_MJPEG,
	.bFrameIndex = 3,
	.bmCapabilities = 0,
	.wWidth = cpu_to_le16(1280),
	.wHeight = cpu_to_le16(720),
	.dwMinBitRate = cpu_to_le32(1280 * 720 * 2 * 8 * 10),
	.dwMaxBitRate = cpu_to_le32(1280 * 720 * 2 * 8 * 30),
	.dwMaxVideoFrameBufferSize = cpu_to_le32(1280 * 720 * 2),
	.dwDefaultFrameInterval = cpu_to_le32(333333),
	.bFrameIntervalType = 3,
	.dwFrameInterval[0] = cpu_to_le32(333333),
	.dwFrameInterval[1] = cpu_to_le32(666666),
	.dwFrameInterval[2] = cpu_to_le32(1000000),
};
static struct UVC_FRAME_MJPEG(3) uvc_frame_mjpg_1080p = {
	.bLength = UVC_DT_FRAME_MJPEG_SIZE(3),
	.bDescriptorType = USB_DT_CS_INTERFACE,
	.bDescriptorSubType = UVC_VS_FRAME_MJPEG,
	.bFrameIndex = 4,
	.bmCapabilities = 0,
	.wWidth = cpu_to_le16(1920),
	.wHeight = cpu_to_le16(1080),
	.dwMinBitRate = cpu_to_le32(1920 * 1080 * 2 * 8 * 10),
	.dwMaxBitRate = cpu_to_le32(1920 * 1080 * 2 * 8 * 30),
	.dwMaxVideoFrameBufferSize = cpu_to_le32(1920 * 1080 * 2),
	.dwDefaultFrameInterval = cpu_to_le32(333333),
	.bFrameIntervalType = 3,
	.dwFrameInterval[0] = cpu_to_le32(333333),
	.dwFrameInterval[1] = cpu_to_le32(666666),
	.dwFrameInterval[2] = cpu_to_le32(1000000),
};

static struct UVC_FRAME_MJPEG(3) uvc_frame_mjpg_2kp = {
	.bLength = UVC_DT_FRAME_MJPEG_SIZE(3),
	.bDescriptorType = USB_DT_CS_INTERFACE,
	.bDescriptorSubType = UVC_VS_FRAME_MJPEG,
	.bFrameIndex = 5,
	.bmCapabilities = 0,
	.wWidth = cpu_to_le16(2560),
	.wHeight = cpu_to_le16(1440),
	.dwMinBitRate = cpu_to_le32(2560 * 1440 * 2 * 8 * 10),
	.dwMaxBitRate = cpu_to_le32(2560 * 1440 * 2 * 8 * 10), //overflow
	.dwMaxVideoFrameBufferSize = cpu_to_le32(2560 * 1440 * 2),
	.dwDefaultFrameInterval = cpu_to_le32(333333),
	.bFrameIntervalType = 3,
	.dwFrameInterval[0] = cpu_to_le32(333333),
	.dwFrameInterval[1] = cpu_to_le32(666666),
	.dwFrameInterval[2] = cpu_to_le32(1000000),
};
static struct UVC_FRAME_MJPEG(3) uvc_frame_mjpg_4kp = {
	.bLength = UVC_DT_FRAME_MJPEG_SIZE(3),
	.bDescriptorType = USB_DT_CS_INTERFACE,
	.bDescriptorSubType = UVC_VS_FRAME_MJPEG,
	.bFrameIndex = 6,
	.bmCapabilities = 0,
	.wWidth = cpu_to_le16(3840),
	.wHeight = cpu_to_le16(2160),
	.dwMinBitRate = cpu_to_le32(3840 * 2160 * 2 * 8 * 10),
	.dwMaxBitRate = cpu_to_le32(3840 * 2160 * 2 * 8 * 10), //overflow
	.dwMaxVideoFrameBufferSize = cpu_to_le32(3840 * 2160 * 2),
	.dwDefaultFrameInterval = cpu_to_le32(333333),
	.bFrameIntervalType = 3,
	.dwFrameInterval[0] = cpu_to_le32(333333),
	.dwFrameInterval[1] = cpu_to_le32(666666),
	.dwFrameInterval[2] = cpu_to_le32(1000000),
};

static struct UVC_STILL_IMAGE_FRAME_DESCRIPTOR(1,
					       1) uvc_frame_mjpg_still_image = {
	.bLength = UVC_DT_STILL_IMAGE_FRAME_SIZE(1, 1),
	.bDescriptorType = USB_DT_CS_INTERFACE,
	.bDescriptorSubtype = UVC_VS_STILL_IMAGE_FRAME,
	.bEndpointAddress = 0, //use method 2, should be set to zero
	.bNumImageSizePatterns = 1,
	.wWidthwHeight[0] = { cpu_to_le16(1920), cpu_to_le16(1080) },
	.bNumCompressionPattern = 1,
	.bCompression[0] = 3,
};

#define UVC_DESCRIPTOR_HEADERS_OF_MJPG_FRAME                                   \
	(struct uvc_descriptor_header *)&uvc_format_mjpg,                      \
		(struct uvc_descriptor_header *)&uvc_frame_mjpg_240p,          \
		(struct uvc_descriptor_header *)&uvc_frame_mjpg_480p,          \
		(struct uvc_descriptor_header *)&uvc_frame_mjpg_720p,          \
		(struct uvc_descriptor_header *)&uvc_frame_mjpg_1080p,         \
		(struct uvc_descriptor_header *)&uvc_frame_mjpg_2kp,           \
		(struct uvc_descriptor_header *)&uvc_frame_mjpg_4kp,           \
		(struct uvc_descriptor_header *)&uvc_frame_mjpg_still_image,   \
		(struct uvc_descriptor_header *)&uvc_color_matching,
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
	.bLength = UVC_DT_FORMAT_FRAMEBASE_SIZE,
	.bDescriptorType = USB_DT_CS_INTERFACE,
	.bDescriptorSubType = UVC_VS_FORMAT_FRAME_BASED,
	.bFormatIndex = 4,
	.bNumFrameDescriptors = 6,
	.guidFormat = { 'H', '2', '6', '4', 0x00, 0x00, 0x10, 0x00, 0x80, 0x00,
			0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71 },
	.bBitsPerPixel = 16,
	.bDefaultFrameIndex = 1,
	.bAspectRatioX = 0,
	.bAspectRatioY = 0,
	.bmInterfaceFlags = 0,
	.bCopyProtect = 0,
	.bVariableSize = 1
};

static struct UVC_FRAME_FRAMEBASE(3) uvc_frame_h264_240p = {
	.bLength = UVC_DT_FRAME_MJPEG_SIZE(3),
	.bDescriptorType = USB_DT_CS_INTERFACE,
	.bDescriptorSubType = UVC_VS_FRAME_FRAME_BASED,
	.bFrameIndex = 1,
	.bmCapabilities = 0,
	.wWidth = cpu_to_le16(320),
	.wHeight = cpu_to_le16(240),
	.dwMinBitRate = cpu_to_le32(320 * 240 * 2 * 8 * 10),
	.dwMaxBitRate = cpu_to_le32(320 * 240 * 2 * 8 * 30),
	.dwDefaultFrameInterval = cpu_to_le32(333333),
	.bFrameIntervalType = 3,
	.dwFrameInterval[0] = cpu_to_le32(333333),
	.dwFrameInterval[1] = cpu_to_le32(666666),
	.dwFrameInterval[2] = cpu_to_le32(1000000),
};
static struct UVC_FRAME_FRAMEBASE(3) uvc_frame_h264_480p = {
	.bLength = UVC_DT_FRAME_MJPEG_SIZE(3),
	.bDescriptorType = USB_DT_CS_INTERFACE,
	.bDescriptorSubType = UVC_VS_FRAME_FRAME_BASED,
	.bFrameIndex = 2,
	.bmCapabilities = 0,
	.wWidth = cpu_to_le16(640),
	.wHeight = cpu_to_le16(480),
	.dwMinBitRate = cpu_to_le32(640 * 480 * 2 * 8 * 10),
	.dwMaxBitRate = cpu_to_le32(640 * 480 * 2 * 8 * 30),
	.dwDefaultFrameInterval = cpu_to_le32(333333),
	.bFrameIntervalType = 3,
	.dwFrameInterval[0] = cpu_to_le32(333333),
	.dwFrameInterval[1] = cpu_to_le32(666666),
	.dwFrameInterval[2] = cpu_to_le32(1000000),
};
static struct UVC_FRAME_FRAMEBASE(3) uvc_frame_h264_720p = {
	.bLength = UVC_DT_FRAME_MJPEG_SIZE(3),
	.bDescriptorType = USB_DT_CS_INTERFACE,
	.bDescriptorSubType = UVC_VS_FRAME_FRAME_BASED,
	.bFrameIndex = 3,
	.bmCapabilities = 0,
	.wWidth = cpu_to_le16(1280),
	.wHeight = cpu_to_le16(720),
	.dwMinBitRate = cpu_to_le32(1280 * 720 * 2 * 8 * 10),
	.dwMaxBitRate = cpu_to_le32(1280 * 720 * 2 * 8 * 30),
	.dwDefaultFrameInterval = cpu_to_le32(333333),
	.bFrameIntervalType = 3,
	.dwFrameInterval[0] = cpu_to_le32(333333),
	.dwFrameInterval[1] = cpu_to_le32(666666),
	.dwFrameInterval[2] = cpu_to_le32(1000000),
};
static struct UVC_FRAME_FRAMEBASE(3) uvc_frame_h264_1080p = {
	.bLength = UVC_DT_FRAME_MJPEG_SIZE(3),
	.bDescriptorType = USB_DT_CS_INTERFACE,
	.bDescriptorSubType = UVC_VS_FRAME_FRAME_BASED,
	.bFrameIndex = 4,
	.bmCapabilities = 0,
	.wWidth = cpu_to_le16(1920),
	.wHeight = cpu_to_le16(1080),
	.dwMinBitRate = cpu_to_le32(1920 * 1080 * 2 * 8 * 10),
	.dwMaxBitRate = cpu_to_le32(1920 * 1080 * 2 * 8 * 30),
	.dwDefaultFrameInterval = cpu_to_le32(333333),
	.bFrameIntervalType = 3,
	.dwFrameInterval[0] = cpu_to_le32(333333),
	.dwFrameInterval[1] = cpu_to_le32(666666),
	.dwFrameInterval[2] = cpu_to_le32(1000000),
};
static struct UVC_FRAME_FRAMEBASE(3) uvc_frame_h264_2kp = {
	.bLength = UVC_DT_FRAME_MJPEG_SIZE(3),
	.bDescriptorType = USB_DT_CS_INTERFACE,
	.bDescriptorSubType = UVC_VS_FRAME_FRAME_BASED,
	.bFrameIndex = 5,
	.bmCapabilities = 0,
	.wWidth = cpu_to_le16(2560),
	.wHeight = cpu_to_le16(1440),
	.dwMinBitRate = cpu_to_le32(2560 * 1440 * 2 * 8 * 10),
	.dwMaxBitRate = cpu_to_le32(2560 * 1440 * 2 * 8 * 10),
	.dwDefaultFrameInterval = cpu_to_le32(333333),
	.bFrameIntervalType = 3,
	.dwFrameInterval[0] = cpu_to_le32(333333),
	.dwFrameInterval[1] = cpu_to_le32(666666),
	.dwFrameInterval[2] = cpu_to_le32(1000000),
};
static struct UVC_FRAME_FRAMEBASE(3) uvc_frame_h264_4kp = {
	.bLength = UVC_DT_FRAME_MJPEG_SIZE(3),
	.bDescriptorType = USB_DT_CS_INTERFACE,
	.bDescriptorSubType = UVC_VS_FRAME_FRAME_BASED,
	.bFrameIndex = 6,
	.bmCapabilities = 0,
	.wWidth = cpu_to_le16(3840),
	.wHeight = cpu_to_le16(2160),
	.dwMinBitRate = cpu_to_le32(3840 * 2160 * 2 * 8 * 10),
	.dwMaxBitRate = cpu_to_le32(3840 * 2160 * 2 * 8 * 10),
	.dwDefaultFrameInterval = cpu_to_le32(333333),
	.bFrameIntervalType = 3,
	.dwFrameInterval[0] = cpu_to_le32(333333),
	.dwFrameInterval[1] = cpu_to_le32(666666),
	.dwFrameInterval[2] = cpu_to_le32(1000000),
};
#endif

static struct UVC_STILL_IMAGE_FRAME_DESCRIPTOR(1,
					       1) uvc_frame_h264_still_image = {
	.bLength = UVC_DT_STILL_IMAGE_FRAME_SIZE(1, 1),
	.bDescriptorType = USB_DT_CS_INTERFACE,
	.bDescriptorSubtype = UVC_VS_STILL_IMAGE_FRAME,
	.bEndpointAddress = 0, //use method 2, should be set to zero
	.bNumImageSizePatterns = 1,
	.wWidthwHeight[0] = { cpu_to_le16(1920), cpu_to_le16(1080) },
	.bNumCompressionPattern = 1,
	.bCompression[0] = 3,
};

#define UVC_DESCRIPTOR_HEADERS_OF_H264_FRAME                                   \
	(struct uvc_descriptor_header *)&uvc_format_h264,                      \
		(struct uvc_descriptor_header *)&uvc_frame_h264_240p,          \
		(struct uvc_descriptor_header *)&uvc_frame_h264_480p,          \
		(struct uvc_descriptor_header *)&uvc_frame_h264_720p,          \
		(struct uvc_descriptor_header *)&uvc_frame_h264_1080p,         \
		(struct uvc_descriptor_header *)&uvc_frame_h264_2kp,           \
		(struct uvc_descriptor_header *)&uvc_frame_h264_4kp,           \
		(struct uvc_descriptor_header *)&uvc_frame_h264_still_image,   \
		(struct uvc_descriptor_header *)&uvc_color_matching,
#else
#define UVC_DESCRIPTOR_HEADERS_OF_H264_FRAME
#endif

#ifdef SUPPORT_H265
static struct uvc_format_framebase uvc_format_h265 = {
	.bLength = UVC_DT_FORMAT_FRAMEBASE_SIZE,
	.bDescriptorType = USB_DT_CS_INTERFACE,
	.bDescriptorSubType = UVC_VS_FORMAT_FRAME_BASED,
	.bFormatIndex = 5,
	.bNumFrameDescriptors = 6,
	.guidFormat = { 'H', '2', '6', '5', 0x00, 0x00, 0x10, 0x00, 0x80, 0x00,
			0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71 },
	.bBitsPerPixel = 16,
	.bDefaultFrameIndex = 1,
	.bAspectRatioX = 0,
	.bAspectRatioY = 0,
	.bmInterfaceFlags = 0,
	.bCopyProtect = 0,
	.bVariableSize = 1
};
static struct UVC_FRAME_FRAMEBASE(3) uvc_frame_h265_240p = {
	.bLength = UVC_DT_FRAME_MJPEG_SIZE(3),
	.bDescriptorType = USB_DT_CS_INTERFACE,
	.bDescriptorSubType = UVC_VS_FRAME_FRAME_BASED,
	.bFrameIndex = 1,
	.bmCapabilities = 0,
	.wWidth = cpu_to_le16(320),
	.wHeight = cpu_to_le16(240),
	.dwMinBitRate = cpu_to_le32(320 * 240 * 2 * 8 * 10),
	.dwMaxBitRate = cpu_to_le32(320 * 240 * 2 * 8 * 30),
	.dwDefaultFrameInterval = cpu_to_le32(333333),
	.bFrameIntervalType = 3,
	.dwFrameInterval[0] = cpu_to_le32(333333),
	.dwFrameInterval[1] = cpu_to_le32(666666),
	.dwFrameInterval[2] = cpu_to_le32(1000000),
};
static struct UVC_FRAME_FRAMEBASE(3) uvc_frame_h265_480p = {
	.bLength = UVC_DT_FRAME_MJPEG_SIZE(3),
	.bDescriptorType = USB_DT_CS_INTERFACE,
	.bDescriptorSubType = UVC_VS_FRAME_FRAME_BASED,
	.bFrameIndex = 2,
	.bmCapabilities = 0,
	.wWidth = cpu_to_le16(640),
	.wHeight = cpu_to_le16(480),
	.dwMinBitRate = cpu_to_le32(640 * 480 * 2 * 8 * 10),
	.dwMaxBitRate = cpu_to_le32(640 * 480 * 2 * 8 * 30),
	.dwDefaultFrameInterval = cpu_to_le32(333333),
	.bFrameIntervalType = 3,
	.dwFrameInterval[0] = cpu_to_le32(333333),
	.dwFrameInterval[1] = cpu_to_le32(666666),
	.dwFrameInterval[2] = cpu_to_le32(1000000),
};
static struct UVC_FRAME_FRAMEBASE(3) uvc_frame_h265_720p = {
	.bLength = UVC_DT_FRAME_MJPEG_SIZE(3),
	.bDescriptorType = USB_DT_CS_INTERFACE,
	.bDescriptorSubType = UVC_VS_FRAME_FRAME_BASED,
	.bFrameIndex = 3,
	.bmCapabilities = 0,
	.wWidth = cpu_to_le16(1280),
	.wHeight = cpu_to_le16(720),
	.dwMinBitRate = cpu_to_le32(1280 * 720 * 2 * 8 * 10),
	.dwMaxBitRate = cpu_to_le32(1280 * 720 * 2 * 8 * 30),
	.dwDefaultFrameInterval = cpu_to_le32(333333),
	.bFrameIntervalType = 3,
	.dwFrameInterval[0] = cpu_to_le32(333333),
	.dwFrameInterval[1] = cpu_to_le32(666666),
	.dwFrameInterval[2] = cpu_to_le32(1000000),
};
static struct UVC_FRAME_FRAMEBASE(3) uvc_frame_h265_1080p = {
	.bLength = UVC_DT_FRAME_MJPEG_SIZE(3),
	.bDescriptorType = USB_DT_CS_INTERFACE,
	.bDescriptorSubType = UVC_VS_FRAME_FRAME_BASED,
	.bFrameIndex = 4,
	.bmCapabilities = 0,
	.wWidth = cpu_to_le16(1920),
	.wHeight = cpu_to_le16(1080),
	.dwMinBitRate = cpu_to_le32(1920 * 1080 * 2 * 8 * 10),
	.dwMaxBitRate = cpu_to_le32(1920 * 1080 * 2 * 8 * 30),
	.dwDefaultFrameInterval = cpu_to_le32(333333),
	.bFrameIntervalType = 3,
	.dwFrameInterval[0] = cpu_to_le32(333333),
	.dwFrameInterval[1] = cpu_to_le32(666666),
	.dwFrameInterval[2] = cpu_to_le32(1000000),
};
static struct UVC_FRAME_FRAMEBASE(3) uvc_frame_h265_2kp = {
	.bLength = UVC_DT_FRAME_MJPEG_SIZE(3),
	.bDescriptorType = USB_DT_CS_INTERFACE,
	.bDescriptorSubType = UVC_VS_FRAME_FRAME_BASED,
	.bFrameIndex = 5,
	.bmCapabilities = 0,
	.wWidth = cpu_to_le16(2560),
	.wHeight = cpu_to_le16(1440),
	.dwMinBitRate = cpu_to_le32(2560 * 1440 * 2 * 8 * 10),
	.dwMaxBitRate = cpu_to_le32(2560 * 1440 * 2 * 8 * 10),
	.dwDefaultFrameInterval = cpu_to_le32(333333),
	.bFrameIntervalType = 3,
	.dwFrameInterval[0] = cpu_to_le32(333333),
	.dwFrameInterval[1] = cpu_to_le32(666666),
	.dwFrameInterval[2] = cpu_to_le32(1000000),
};
static struct UVC_FRAME_FRAMEBASE(3) uvc_frame_h265_4kp = {
	.bLength = UVC_DT_FRAME_MJPEG_SIZE(3),
	.bDescriptorType = USB_DT_CS_INTERFACE,
	.bDescriptorSubType = UVC_VS_FRAME_FRAME_BASED,
	.bFrameIndex = 6,
	.bmCapabilities = 0,
	.wWidth = cpu_to_le16(3840),
	.wHeight = cpu_to_le16(2160),
	.dwMinBitRate = cpu_to_le32(3840 * 2160 * 2 * 8 * 10),
	.dwMaxBitRate = cpu_to_le32(3840 * 2160 * 2 * 8 * 10),
	.dwDefaultFrameInterval = cpu_to_le32(333333),
	.bFrameIntervalType = 3,
	.dwFrameInterval[0] = cpu_to_le32(333333),
	.dwFrameInterval[1] = cpu_to_le32(666666),
	.dwFrameInterval[2] = cpu_to_le32(1000000),
};

static struct UVC_STILL_IMAGE_FRAME_DESCRIPTOR(1,
					       1) uvc_frame_h265_still_image = {
	.bLength = UVC_DT_STILL_IMAGE_FRAME_SIZE(1, 1),
	.bDescriptorType = USB_DT_CS_INTERFACE,
	.bDescriptorSubtype = UVC_VS_STILL_IMAGE_FRAME,
	.bEndpointAddress = 0, //use method 2, should be set to zero
	.bNumImageSizePatterns = 1,
	.wWidthwHeight[0] = { cpu_to_le16(1920), cpu_to_le16(1080) },
	.bNumCompressionPattern = 1,
	.bCompression[0] = 3,
};

#define UVC_DESCRIPTOR_HEADERS_OF_H265_FRAME                                   \
	(struct uvc_descriptor_header *)&uvc_format_h265,                      \
		(struct uvc_descriptor_header *)&uvc_frame_h265_240p,          \
		(struct uvc_descriptor_header *)&uvc_frame_h265_480p,          \
		(struct uvc_descriptor_header *)&uvc_frame_h265_720p,          \
		(struct uvc_descriptor_header *)&uvc_frame_h265_1080p,         \
		(struct uvc_descriptor_header *)&uvc_frame_h265_2kp,           \
		(struct uvc_descriptor_header *)&uvc_frame_h265_4kp,           \
		(struct uvc_descriptor_header *)&uvc_frame_h265_still_image,   \
		(struct uvc_descriptor_header *)&uvc_color_matching,
#else
#define UVC_DESCRIPTOR_HEADERS_OF_H265_FRAME
#endif

#ifdef SUPPORT_IR
static struct uvc_format_uncompressed uvc_format_ir = {
	.bLength = UVC_DT_FORMAT_UNCOMPRESSED_SIZE,
	.bDescriptorType = USB_DT_CS_INTERFACE,
	.bDescriptorSubType = UVC_VS_FORMAT_UNCOMPRESSED,
	.bFormatIndex = 1,
	.bNumFrameDescriptors = 1,
	.guidFormat = { 0x32, 0x00, 0x00, 0x00, 0x02, 0x00, 0x10, 0x00, 0x80,
			0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71 },
	.bBitsPerPixel = 8,
	.bDefaultFrameIndex = 1,
	.bAspectRatioX = 0,
	.bAspectRatioY = 0,
	.bmInterfaceFlags = 0,
	.bCopyProtect = 0,
};

static struct UVC_FRAME_UNCOMPRESSED(3) uvc_frame_ir_320p = {
	.bLength = UVC_DT_FRAME_UNCOMPRESSED_SIZE(3),
	.bDescriptorType = USB_DT_CS_INTERFACE,
	.bDescriptorSubType = UVC_VS_FRAME_UNCOMPRESSED,
	.bFrameIndex = 1,
	.bmCapabilities = 0,
	.wWidth = cpu_to_le16(320),
	.wHeight = cpu_to_le16(320),
	.dwMinBitRate = cpu_to_le32(320 * 320 * 1.0 * 8 * 10),
	.dwMaxBitRate = cpu_to_le32(320 * 320 * 1.0 * 8 * 30),
	.dwMaxVideoFrameBufferSize = cpu_to_le32(320 * 320 * 1.0),
	.dwDefaultFrameInterval = cpu_to_le32(333333),
	.bFrameIntervalType = 3,
	.dwFrameInterval[0] = cpu_to_le32(333333),
	.dwFrameInterval[1] = cpu_to_le32(666666),
	.dwFrameInterval[2] = cpu_to_le32(1000000),
};

#define UVC_DESCRIPTOR_HEADERS_OF_IR_FRAME                                     \
	(struct uvc_descriptor_header *)&uvc_format_ir,                        \
		(struct uvc_descriptor_header *)&uvc_frame_ir_320p,            \
		(struct uvc_descriptor_header *)&uvc_color_matching,
#else
#define UVC_DESCRIPTOR_HEADERS_OF_IR_FRAME
#endif

static struct uvc_color_matching_descriptor uvc_color_matching = {
	.bLength = UVC_DT_COLOR_MATCHING_SIZE,
	.bDescriptorType = USB_DT_CS_INTERFACE,
	.bDescriptorSubType = UVC_VS_COLORFORMAT,
	.bColorPrimaries = 1,
	.bTransferCharacteristics = 1,
	.bMatrixCoefficients = 4,
};

#ifdef CONFIG_WINDOWS_HELLO_SUPPORT
#define MAX_CONTROL_SIZE 11
#define BASE_CONTROL_SIZE 7
#else
#define MAX_CONTROL_SIZE 10
#define BASE_CONTROL_SIZE 6
#endif
static struct uvc_descriptor_header **uvc_fs_control_cls;
static struct uvc_descriptor_header **uvc_ss_control_cls;

#define MAX_FORMAT_COUNT 50
static struct uvc_descriptor_header **uvc_fs_streaming_cls;
static struct uvc_descriptor_header **uvc_hs_streaming_cls;
static struct uvc_descriptor_header **uvc_ss_streaming_cls;
static struct uvc_descriptor_header
	*uvc_streaming_cls_std[MAX_STREAM_SUPPORT][MAX_FORMAT_COUNT] = {
		{
			(struct uvc_descriptor_header *)&uvc_input_header[0],
			UVC_DESCRIPTOR_HEADERS_OF_YUY2_FRAME
				UVC_DESCRIPTOR_HEADERS_OF_NV12_FRAME
					UVC_DESCRIPTOR_HEADERS_OF_MJPG_FRAME
						UVC_DESCRIPTOR_HEADERS_OF_H264_FRAME
							UVC_DESCRIPTOR_HEADERS_OF_H265_FRAME
								NULL,
		},
		{
			(struct uvc_descriptor_header *)&uvc_input_header[1],
#ifdef CONFIG_WINDOWS_HELLO_SUPPORT
			UVC_DESCRIPTOR_HEADERS_OF_IR_FRAME
#else
			UVC_DESCRIPTOR_HEADERS_OF_YUY2_FRAME
				UVC_DESCRIPTOR_HEADERS_OF_NV12_FRAME
					UVC_DESCRIPTOR_HEADERS_OF_MJPG_FRAME
						UVC_DESCRIPTOR_HEADERS_OF_H264_FRAME
							UVC_DESCRIPTOR_HEADERS_OF_H265_FRAME
#endif
				NULL,
		},
		{
			(struct uvc_descriptor_header *)&uvc_input_header[2],
			UVC_DESCRIPTOR_HEADERS_OF_YUY2_FRAME
				UVC_DESCRIPTOR_HEADERS_OF_NV12_FRAME
					UVC_DESCRIPTOR_HEADERS_OF_MJPG_FRAME
						UVC_DESCRIPTOR_HEADERS_OF_H264_FRAME
							UVC_DESCRIPTOR_HEADERS_OF_H265_FRAME
								NULL,
		},
		{
			(struct uvc_descriptor_header *)&uvc_input_header[3],
			UVC_DESCRIPTOR_HEADERS_OF_YUY2_FRAME
				UVC_DESCRIPTOR_HEADERS_OF_NV12_FRAME
					UVC_DESCRIPTOR_HEADERS_OF_MJPG_FRAME
						UVC_DESCRIPTOR_HEADERS_OF_H264_FRAME
							UVC_DESCRIPTOR_HEADERS_OF_H265_FRAME
								NULL,
		},
		{
			(struct uvc_descriptor_header *)&uvc_input_header[4],
			UVC_DESCRIPTOR_HEADERS_OF_YUY2_FRAME
				UVC_DESCRIPTOR_HEADERS_OF_NV12_FRAME
					UVC_DESCRIPTOR_HEADERS_OF_MJPG_FRAME
						UVC_DESCRIPTOR_HEADERS_OF_H264_FRAME
							UVC_DESCRIPTOR_HEADERS_OF_H265_FRAME
								NULL,
		},
		{
			(struct uvc_descriptor_header *)&uvc_input_header[5],
			UVC_DESCRIPTOR_HEADERS_OF_YUY2_FRAME
				UVC_DESCRIPTOR_HEADERS_OF_NV12_FRAME
					UVC_DESCRIPTOR_HEADERS_OF_MJPG_FRAME
						UVC_DESCRIPTOR_HEADERS_OF_H264_FRAME
							UVC_DESCRIPTOR_HEADERS_OF_H265_FRAME
								NULL,
		}
	};

int video_bind_config(struct usb_configuration *c)
{
	int status = 0, i;

	if (!uvc_function_enable)
		return 0;

	f_uvc = kzalloc(uvc_function_enable * sizeof(*f_uvc), GFP_KERNEL);

	for (i = 0; i < uvc_function_enable; i++) {
		f_uvc[i] = usb_get_function(fi_uvc[i]);
		if (IS_ERR(f_uvc[i]))
			return PTR_ERR(f_uvc[i]);

		status = usb_add_function(c, f_uvc[i]);
		if (status < 0) {
			usb_put_function(f_uvc[i]);
			break;
		}
	}

	return status;
}

int video_bind(struct usb_composite_dev *cdev)
{
	int i, j;
	struct f_uvc_opts *uvc_opts;
	struct uvc_descriptor_header **fs_control_cls = NULL;
	struct uvc_descriptor_header **ss_control_cls = NULL;
	struct uvc_descriptor_header **fs_streaming_cls = NULL;
	struct uvc_descriptor_header **hs_streaming_cls = NULL;
	struct uvc_descriptor_header **ss_streaming_cls = NULL;

	if (!uvc_function_enable)
		return 0;

	if (MULTI_STREAM_NUM > MAX_STREAM_SUPPORT) {
		BUG();
	}
#ifdef CONFIG_SS_GADGET_UVC_MULTI_STREAM
	uvc_function_enable = 1;
#endif

	if (!uvc_fs_control_cls) {
		uvc_fs_control_cls = kzalloc(
			sizeof(void *) * uvc_function_enable * MAX_CONTROL_SIZE,
			GFP_KERNEL);
		uvc_ss_control_cls = kzalloc(
			sizeof(void *) * uvc_function_enable * MAX_CONTROL_SIZE,
			GFP_KERNEL);
	}

	if (!uvc_fs_streaming_cls) {
#ifdef CONFIG_SS_GADGET_UVC_MULTI_STREAM
		uvc_fs_streaming_cls = kzalloc(
			sizeof(void *) * MAX_STREAM_SUPPORT * MAX_FORMAT_COUNT,
			GFP_KERNEL);
		uvc_hs_streaming_cls = kzalloc(
			sizeof(void *) * MAX_STREAM_SUPPORT * MAX_FORMAT_COUNT,
			GFP_KERNEL);
		uvc_ss_streaming_cls = kzalloc(
			sizeof(void *) * MAX_STREAM_SUPPORT * MAX_FORMAT_COUNT,
			GFP_KERNEL);
#else
		uvc_fs_streaming_cls = kzalloc(
			sizeof(void *) * uvc_function_enable * MAX_FORMAT_COUNT,
			GFP_KERNEL);
		uvc_hs_streaming_cls = kzalloc(
			sizeof(void *) * uvc_function_enable * MAX_FORMAT_COUNT,
			GFP_KERNEL);
		uvc_ss_streaming_cls = kzalloc(
			sizeof(void *) * uvc_function_enable * MAX_FORMAT_COUNT,
			GFP_KERNEL);
#endif
	}
	fi_uvc = kzalloc(uvc_function_enable * sizeof(*fi_uvc), GFP_KERNEL);
	for (i = 0; i < uvc_function_enable; i++) {
		uvc_opts = NULL;

		fi_uvc[i] = usb_get_function_instance("uvc");
		if (IS_ERR(fi_uvc[i]))
			return PTR_ERR(fi_uvc[i]);

		uvc_opts =
			container_of(fi_uvc[i], struct f_uvc_opts, func_inst);
		if (IS_ERR(uvc_opts))
			return PTR_ERR(uvc_opts);

#ifdef CONFIG_SS_GADGET_UVC_MULTI_STREAM
		for (j = 0; j < MULTI_STREAM_NUM; j++) {
			uvc_opts->streaming_interval[j] = streaming_interval[j];
			uvc_opts->streaming_maxpacket[j] =
				streaming_maxpacket[j];
			uvc_opts->streaming_maxburst[j] = streaming_maxburst[j];
			uvc_opts->streaming_name[j] = streaming_name[j];
		}
#else
		uvc_opts->streaming_interval = streaming_interval[i];
		uvc_opts->streaming_maxpacket = streaming_maxpacket[i];
		uvc_opts->streaming_maxburst = streaming_maxburst[i];
		strlcpy(uvc_opts->streaming_name, streaming_name[i], 32);
#endif
		uvc_opts->bulk_streaming_ep = bulk_streaming_ep;

		/*
		 * controls:
		 */
		fs_control_cls = &uvc_fs_control_cls[i * MAX_CONTROL_SIZE];
		ss_control_cls = &uvc_ss_control_cls[i * MAX_CONTROL_SIZE];

		for (j = 0; j < MAX_STREAM_SUPPORT; j++) {
			memcpy(&uvc_output_terminal[j], &uvc_output_terminal[0],
			       sizeof(uvc_output_terminal[0]));
			uvc_output_terminal[j].bTerminalID = UVC_OT_ID + j;
		}

#ifdef CONFIG_WINDOWS_HELLO_SUPPORT
		{
			fs_control_cls[0] = (struct uvc_descriptor_header
						     *)&uvc_control_header;
			fs_control_cls[1] = (struct uvc_descriptor_header
						     *)&uvc_camera_terminal;
			fs_control_cls[2] =
				(struct uvc_descriptor_header *)&uvc_processing;
			fs_control_cls[3] = (struct uvc_descriptor_header
						     *)&uvc_extension_unit1;
			fs_control_cls[4] = (struct uvc_descriptor_header
						     *)&uvc_extension_unit2;
			fs_control_cls[5] = (struct uvc_descriptor_header
						     *)&uvc_extension_unit3;
			fs_control_cls[6] = (struct uvc_descriptor_header
						     *)&uvc_output_terminal[i];
			for (j = 1; j < MULTI_STREAM_NUM; j++) {
				fs_control_cls[6 + j] =
					(struct uvc_descriptor_header
						 *)&uvc_output_terminal[j];
			}
			fs_control_cls[6 + MULTI_STREAM_NUM] = NULL;

			ss_control_cls[0] = (struct uvc_descriptor_header
						     *)&uvc_control_header;
			ss_control_cls[1] = (struct uvc_descriptor_header
						     *)&uvc_camera_terminal;
			ss_control_cls[2] =
				(struct uvc_descriptor_header *)&uvc_processing;
			ss_control_cls[3] = (struct uvc_descriptor_header
						     *)&uvc_extension_unit1;
			ss_control_cls[4] = (struct uvc_descriptor_header
						     *)&uvc_extension_unit2;
			ss_control_cls[5] = (struct uvc_descriptor_header
						     *)&uvc_extension_unit3;
			ss_control_cls[6] = (struct uvc_descriptor_header
						     *)&uvc_output_terminal[i];
			for (j = 1; j < MULTI_STREAM_NUM; j++) {
				ss_control_cls[6 + j] =
					(struct uvc_descriptor_header
						 *)&uvc_output_terminal[j];
			}
			ss_control_cls[6 + MULTI_STREAM_NUM] = NULL;
		}
#else
		{
			fs_control_cls[0] = (struct uvc_descriptor_header
						     *)&uvc_control_header;
			fs_control_cls[1] = (struct uvc_descriptor_header
						     *)&uvc_camera_terminal;
			fs_control_cls[2] =
				(struct uvc_descriptor_header *)&uvc_processing;
			fs_control_cls[3] = (struct uvc_descriptor_header
						     *)&uvc_extension_unit1;
			fs_control_cls[4] = (struct uvc_descriptor_header
						     *)&uvc_extension_unit2;
			fs_control_cls[5] = (struct uvc_descriptor_header
						     *)&uvc_output_terminal[i];
			for (j = 1; j < MULTI_STREAM_NUM; j++) {
				fs_control_cls[5 + j] =
					(struct uvc_descriptor_header
						 *)&uvc_output_terminal[j];
			}
			fs_control_cls[5 + MULTI_STREAM_NUM] = NULL;

			ss_control_cls[0] = (struct uvc_descriptor_header
						     *)&uvc_control_header;
			ss_control_cls[1] = (struct uvc_descriptor_header
						     *)&uvc_camera_terminal;
			ss_control_cls[2] =
				(struct uvc_descriptor_header *)&uvc_processing;
			ss_control_cls[3] = (struct uvc_descriptor_header
						     *)&uvc_extension_unit1;
			ss_control_cls[4] = (struct uvc_descriptor_header
						     *)&uvc_extension_unit2;
			ss_control_cls[5] = (struct uvc_descriptor_header
						     *)&uvc_output_terminal[i];
			for (j = 1; j < MULTI_STREAM_NUM; j++) {
				ss_control_cls[5 + j] =
					(struct uvc_descriptor_header
						 *)&uvc_output_terminal[j];
			}
			ss_control_cls[5 + MULTI_STREAM_NUM] = NULL;
		}
#endif

		uvc_opts->fs_control =
			(const struct uvc_descriptor_header *const *)
				fs_control_cls;
		uvc_opts->ss_control =
			(const struct uvc_descriptor_header *const *)
				ss_control_cls;

		/*
		 * Formats:
		 */
		fs_streaming_cls = &uvc_fs_streaming_cls[i * MAX_FORMAT_COUNT];
		hs_streaming_cls = &uvc_hs_streaming_cls[i * MAX_FORMAT_COUNT];
		ss_streaming_cls = &uvc_ss_streaming_cls[i * MAX_FORMAT_COUNT];

#ifdef CONFIG_SS_GADGET_UVC_MULTI_STREAM
		for (j = 0; j < MAX_STREAM_SUPPORT; j++) {
			memcpy(&uvc_input_header[j], &uvc_input_header[0],
			       sizeof(uvc_input_header[j]));
			uvc_input_header[j].bTerminalLink = UVC_OT_ID + j;
#ifdef CONFIG_WINDOWS_HELLO_SUPPORT
			uvc_input_header[0].bNumFormats = 1;
			uvc_input_header[0].bLength =
				UVC_DT_INPUT_HEADER_SIZE(1, 1);

			uvc_input_header[1].bNumFormats = 1;
			uvc_input_header[1].bLength =
				UVC_DT_INPUT_HEADER_SIZE(1, 1);
#endif

			memcpy(fs_streaming_cls + j * MAX_FORMAT_COUNT,
			       uvc_streaming_cls_std[j],
			       sizeof(uvc_streaming_cls_std[j]));
			memcpy(hs_streaming_cls + j * MAX_FORMAT_COUNT,
			       uvc_streaming_cls_std[j],
			       sizeof(uvc_streaming_cls_std[j]));
			memcpy(ss_streaming_cls + j * MAX_FORMAT_COUNT,
			       uvc_streaming_cls_std[j],
			       sizeof(uvc_streaming_cls_std[j]));
		}
#else
		{
			memcpy(&uvc_input_header[i], &uvc_input_header[0],
			       sizeof(uvc_input_header[i]));
			uvc_input_header[i].bTerminalLink = UVC_OT_ID + i;
#ifdef CONFIG_WINDOWS_HELLO_SUPPORT
			uvc_input_header[0].bNumFormats = 1;
			uvc_input_header[0].bLength =
				UVC_DT_INPUT_HEADER_SIZE(1, 1);

			uvc_input_header[1].bNumFormats = 1;
			uvc_input_header[1].bLength =
				UVC_DT_INPUT_HEADER_SIZE(1, 1);
#endif

			memcpy(fs_streaming_cls, uvc_streaming_cls_std[i],
			       sizeof(uvc_streaming_cls_std[i]));
			memcpy(hs_streaming_cls, uvc_streaming_cls_std[i],
			       sizeof(uvc_streaming_cls_std[i]));
			memcpy(ss_streaming_cls, uvc_streaming_cls_std[i],
			       sizeof(uvc_streaming_cls_std[i]));
		}
#endif

		fs_streaming_cls[0] =
			(struct uvc_descriptor_header *)&uvc_input_header[i];
		hs_streaming_cls[0] =
			(struct uvc_descriptor_header *)&uvc_input_header[i];
		ss_streaming_cls[0] =
			(struct uvc_descriptor_header *)&uvc_input_header[i];

		uvc_opts->fs_streaming =
			(const struct uvc_descriptor_header *const *)
				fs_streaming_cls;
		uvc_opts->hs_streaming =
			(const struct uvc_descriptor_header *const *)
				hs_streaming_cls;
		uvc_opts->ss_streaming =
			(const struct uvc_descriptor_header *const *)
				ss_streaming_cls;
	}

	return 0;
}

int video_unbind(struct usb_composite_dev *cdev)
{
	int i;

	if (!uvc_function_enable)
		return 0;

	for (i = 0; i < uvc_function_enable; i++) {
		if (!IS_ERR_OR_NULL(f_uvc[i]))
			usb_put_function(f_uvc[i]);

		if (!IS_ERR_OR_NULL(fi_uvc[i]))
			usb_put_function_instance(fi_uvc[i]);
	}

	if (!IS_ERR_OR_NULL(f_uvc)) {
		kfree(f_uvc);
		f_uvc = NULL;
	}

	if (!IS_ERR_OR_NULL(fi_uvc)) {
		kfree(fi_uvc);
		fi_uvc = NULL;
	}

	if (uvc_fs_streaming_cls) {
		kfree(uvc_fs_streaming_cls);
		kfree(uvc_hs_streaming_cls);
		kfree(uvc_ss_streaming_cls);
		uvc_fs_streaming_cls = NULL;
		uvc_hs_streaming_cls = NULL;
		uvc_ss_streaming_cls = NULL;
	}

	if (uvc_fs_control_cls) {
		kfree(uvc_fs_control_cls);
		kfree(uvc_ss_control_cls);
		uvc_fs_control_cls = NULL;
		uvc_ss_control_cls = NULL;
	}

	return 0;
}

MODULE_AUTHOR("Laurent Pinchart");
MODULE_DESCRIPTION("Webcam Video Gadget");
MODULE_LICENSE("GPL");
