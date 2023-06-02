/*
 *	webcam.c -- USB webcam gadget driver
 *
 *	Copyright (C) 2009-2010
 *	    Laurent Pinchart (laurent.pinchart@ideasonboard.com)
 *
 *	This program is free software; you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation; either version 2 of the License, or
 *	(at your option) any later version.
 *
 */




#ifdef TEST_AUDIO
#include "audio.c"
#else

#define ENABLE_UVC
#define ENABLE_UAC

#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/usb/video.h>

#include "f_uvc.h"

/*
 * Kbuild is not very cooperative with respect to linking separately
 * compiled library objects into one module.  So for now we won't use
 * separate compilation ... ensuring init/exit sections work to shrink
 * the runtime footprint, and giving us at least some parts of what
 * a "gcc --combine ... part1.c part2.c part3.c ... " build would.
 */
#include "composite.c"
#include "usbstring.c"
#include "config.c"
#include "epautoconf.c"

#include "uvc_queue.c"
#include "uvc_video.c"
#include "uvc_v4l2.c"
#include "f_uvc.c"
#include "f_uac.c"
#include "audio_poll.c"
#include "uvc.h"
/* --------------------------------------------------------------------------
 * Device descriptor
 */

#define YUV_NV12    0
#define YUV_YUY2    1


#ifdef ENABLE_UVC

#define WEBCAM_VENDOR_ID		0x1d6b	/* Linux Foundation */
#define WEBCAM_PRODUCT_ID		0x0102	/* Webcam A/V gadget */

#else

#define WEBCAM_VENDOR_ID		0x3d9c	/* Linux Foundation */
#define WEBCAM_PRODUCT_ID		0x0806	/* Webcam A/V gadget */

#endif

#define WEBCAM_DEVICE_BCD		0x0010	/* 0.10 */

static const char webcam_vendor_label[] = "FULLHAN webcam";
static const char webcam_product_label[] = "Webcam gadget";
static const char webcam_config_label[] = "Video";

/* string IDs are assigned dynamically */

#define STRING_MANUFACTURER_IDX		0
#define STRING_PRODUCT_IDX		1
#define STRING_DESCRIPTION_IDX		2

/* identification number of Unit or Terminal */
#define UVC_INTERFACE_ID			0
#define UVC_CAMERAL_TERMINAL_ID		1
#define UVC_PROCESSING_UNIT_ID		2
#define UVC_H264_EXTENSION_UNIT_ID	3
#define UVC_OUTPUT_TERMINAL_ID		4
#define UVC_OUTPUT_TERMINAL2_ID		5



static struct usb_string webcam_strings[] = {
	[STRING_MANUFACTURER_IDX].s = webcam_vendor_label,
	[STRING_PRODUCT_IDX].s = webcam_product_label,
	[STRING_DESCRIPTION_IDX].s = webcam_config_label,
	{  }
};

static struct usb_gadget_strings webcam_stringtab = {
	.language = 0x0409,	/* en-us */
	.strings = webcam_strings,
};

static struct usb_gadget_strings *webcam_device_strings[] = {
	&webcam_stringtab,
	NULL,
};

static struct usb_device_descriptor webcam_device_descriptor = {
	.bLength		= USB_DT_DEVICE_SIZE,
	.bDescriptorType	= USB_DT_DEVICE,
	.bcdUSB			= cpu_to_le16(0x0200),
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
	.bNumConfigurations	= 0, /* dynamic */
};

DECLARE_UVC_HEADER_DESCRIPTOR(2);

static  struct UVC_HEADER_DESCRIPTOR(2) uvc_control_header = {
	.bLength		= UVC_DT_HEADER_SIZE(1),
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VC_HEADER,
	.bcdUVC			= cpu_to_le16(0x0100),
	.wTotalLength		= 0, /* dynamic */
	.dwClockFrequency	= cpu_to_le32(48000000),
	.bInCollection		= 0, /* dynamic */
	.baInterfaceNr[0]	= 0, /* dynamic */
};

static const struct uvc_camera_terminal_descriptor uvc_camera_terminal = {
	.bLength		= UVC_DT_CAMERA_TERMINAL_SIZE(3),
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VC_INPUT_TERMINAL,
	.bTerminalID		= UVC_CAMERAL_TERMINAL_ID,
	.wTerminalType		= cpu_to_le16(0x0201),
	.bAssocTerminal		= 0,
	.iTerminal		= 0,
	.wObjectiveFocalLengthMin	= cpu_to_le16(0),
	.wObjectiveFocalLengthMax	= cpu_to_le16(0),
	.wOcularFocalLength		= cpu_to_le16(0),
	.bControlSize		= 3,
	.bmControls[0]		= 0x0e, /* 0xff*/
	.bmControls[1]		= 0x00, /* 0xff*/
	.bmControls[2]		= 0x07,
};

static const struct uvc_processing_unit_descriptor uvc_processing = {
	.bLength		= UVC_DT_PROCESSING_UNIT_SIZE(2),
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VC_PROCESSING_UNIT,
	.bUnitID		= UVC_PROCESSING_UNIT_ID,
	.bSourceID		= UVC_CAMERAL_TERMINAL_ID,
	.wMaxMultiplier		= cpu_to_le16(16*1024),
	.bControlSize		= 2,
	.bmControls[0]		= 0x0b, /*0xff*/
	.bmControls[1]		= 0x00, /*0xff*/
	.iProcessing		= 0,
};


DECLARE_UVC_EXTENSION_UNIT_DESCRIPTOR(1, 18);

static const struct UVC_EXTENSION_UNIT_DESCRIPTOR(1, 18) h264_extension_unit = {
	.bLength		= UVC_DT_EXTENSION_UNIT_SIZE(1, 18),
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VC_EXTENSION_UNIT,
	.bUnitID		= UVC_H264_EXTENSION_UNIT_ID,
	.guidExtensionCode = { 0x41, 0x76, 0x9e, 0xa2, 0x04,\
	0xde, 0xe3, 0x47, 0x8b, 0x2b, 0xf4, 0x34, 0x1a,\
	0xff, 0x00, 0x3b},
	.bNumControls = 17,
	.bNrInPins = 1,
	.baSourceID[0]		= UVC_PROCESSING_UNIT_ID,
	.bControlSize		= 18,
	.bmControls[0]		= 0xff,
	.bmControls[1]		= 0xff,
	.bmControls[2]		= 0xff,
	.bmControls[3]		= 0xff,
	.bmControls[4]		= 0xff,
	.bmControls[5]		= 0xff,
	.bmControls[6]		= 0xff,
	.bmControls[7]		= 0xff,
	.bmControls[8]		= 0xff,
	.bmControls[9]		= 0xff,
	.bmControls[10]		= 0xff,
	.bmControls[11]		= 0xff,
	.bmControls[12]		= 0xff,
	.bmControls[13]		= 0xff,
	.bmControls[14]		= 0xff,
	.bmControls[15]		= 0xff,
	.bmControls[16]		= 0xff,
	.iExtension		= 0,
};


static const struct uvc_output_terminal_descriptor uvc_output_terminal = {
	.bLength		= UVC_DT_OUTPUT_TERMINAL_SIZE,
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VC_OUTPUT_TERMINAL,
	.bTerminalID		= UVC_OUTPUT_TERMINAL_ID,
	.wTerminalType		= cpu_to_le16(0x0101),
	.bAssocTerminal		= 0,
	.bSourceID		= UVC_H264_EXTENSION_UNIT_ID,
	.iTerminal		= 0,
};



static const struct uvc_output_terminal_descriptor uvc_output_terminal2 = {
	.bLength		= UVC_DT_OUTPUT_TERMINAL_SIZE,
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VC_OUTPUT_TERMINAL,
	.bTerminalID		= UVC_OUTPUT_TERMINAL2_ID,
	.wTerminalType		= cpu_to_le16(0x0101),
	.bAssocTerminal		= 0,
	.bSourceID		= UVC_H264_EXTENSION_UNIT_ID,
	.iTerminal		= 0,
};




DECLARE_UVC_INPUT_HEADER_DESCRIPTOR(1, 1);

static const struct UVC_INPUT_HEADER_DESCRIPTOR(1, 1) uvc_input_header = {
	.bLength		= UVC_DT_INPUT_HEADER_SIZE(1, 1),
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VS_INPUT_HEADER,
	.bNumFormats		= 1,
	.wTotalLength		= 0, /* dynamic */
	.bEndpointAddress	= 0, /* dynamic */
	.bmInfo			= 0,
	.bTerminalLink		= UVC_OUTPUT_TERMINAL_ID,
	.bStillCaptureMethod	= 0,
	.bTriggerSupport	= 0,
	.bTriggerUsage		= 0,
	.bControlSize		= 1,
	.bmaControls[0][0]	= 0,
	/*.bmaControls[1][0]	= 4,*/
	/*.bmaControls[2][0]	= 4,*/
};

static /*const*/ struct uvc_format_uncompressed uvc_format_nv12 = {
	.bLength		= UVC_DT_FORMAT_UNCOMPRESSED_SIZE,
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VS_FORMAT_UNCOMPRESSED,
	.bFormatIndex		= 1,
	.bNumFrameDescriptors	= 3,
	.guidFormat		= { 'N',  'V',  '1',  '2',\
	0x00, 0x00, 0x10, 0x00, 0x80, 0x00, 0x00,\
	0xaa, 0x00, 0x38, 0x9b, 0x71},
	.bBitsPerPixel      = 12,
	.bDefaultFrameIndex	= 1,
	.bAspectRatioX		= 0,
	.bAspectRatioY		= 0,
	.bmInterfaceFlags	= 0,
	.bCopyProtect		= 0,
};

DECLARE_UVC_FRAME_UNCOMPRESSED(3);
DECLARE_UVC_FRAME_UNCOMPRESSED(5);

static const struct UVC_FRAME_UNCOMPRESSED(5) uvc_frame_nv12_360p = {
	.bLength		= UVC_DT_FRAME_UNCOMPRESSED_SIZE(5),
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VS_FRAME_UNCOMPRESSED,
	.bFrameIndex		= 1,
	.bmCapabilities		= 0,
	.wWidth			= cpu_to_le16(640),
	.wHeight		= cpu_to_le16(368),
	.dwMinBitRate		= cpu_to_le32(18432000),
	.dwMaxBitRate		= cpu_to_le32(55296000),
	.dwMaxVideoFrameBufferSize	= cpu_to_le32(471040),
	.dwDefaultFrameInterval	= cpu_to_le32(333333),
	.bFrameIntervalType	= 5,
	.dwFrameInterval[0]	= cpu_to_le32(166666), /*60fps*/
	.dwFrameInterval[1]	= cpu_to_le32(333333), /*30fps*/
	.dwFrameInterval[2]	= cpu_to_le32(666666), /*15fps*/
	.dwFrameInterval[3]	= cpu_to_le32(1000000), /*10fps*/
	.dwFrameInterval[4]	= cpu_to_le32(5000000), /*2fps*/
};

static const struct UVC_FRAME_UNCOMPRESSED(5) uvc_frame_nv12_720p = {
	.bLength		= UVC_DT_FRAME_UNCOMPRESSED_SIZE(5),
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VS_FRAME_UNCOMPRESSED,
	.bFrameIndex		= 2,
	.bmCapabilities		= 0,
	.wWidth			= cpu_to_le16(1280),
	.wHeight		= cpu_to_le16(720),
	.dwMinBitRate		= cpu_to_le32(29491200),
	.dwMaxBitRate		= cpu_to_le32(29491200),
	.dwMaxVideoFrameBufferSize	= cpu_to_le32(1843200),
	.dwDefaultFrameInterval	= cpu_to_le32(333333),
	.bFrameIntervalType	= 5,
	.dwFrameInterval[0]	= cpu_to_le32(166666), /*60fps*/
	.dwFrameInterval[1]	= cpu_to_le32(333333), /*30fps*/
	.dwFrameInterval[2]	= cpu_to_le32(666666), /*15fps*/
	.dwFrameInterval[3]	= cpu_to_le32(1000000), /*10fps*/
	.dwFrameInterval[4]	= cpu_to_le32(5000000), /*2fps*/
};

static const struct UVC_FRAME_UNCOMPRESSED(3) uvc_frame_nv12_1080p = {
	.bLength		= UVC_DT_FRAME_UNCOMPRESSED_SIZE(3),
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VS_FRAME_UNCOMPRESSED,
	.bFrameIndex		= 3,
	.bmCapabilities		= 0,
	.wWidth			= cpu_to_le16(1920),
	.wHeight		= cpu_to_le16(1088),
	.dwMinBitRate		= cpu_to_le32(29491200),
	.dwMaxBitRate		= cpu_to_le32(29491200),
	.dwMaxVideoFrameBufferSize	= cpu_to_le32(1843200),
	.dwDefaultFrameInterval	= cpu_to_le32(666666),
	.bFrameIntervalType	= 3,
	.dwFrameInterval[0]	= cpu_to_le32(666666), /*15fps*/
	.dwFrameInterval[1]	= cpu_to_le32(1000000), /*10fps*/
	.dwFrameInterval[2]	= cpu_to_le32(5000000), /*2fps*/
};

static /*const*/ struct uvc_format_mjpeg uvc_format_mjpg = {
	.bLength		= UVC_DT_FORMAT_MJPEG_SIZE,
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VS_FORMAT_MJPEG,
	.bFormatIndex		= 1,
	.bNumFrameDescriptors	= 3,
	.bmFlags		= 0,
	.bDefaultFrameIndex	= 1,
	.bAspectRatioX		= 0,
	.bAspectRatioY		= 0,
	.bmInterfaceFlags	= 0,
	.bCopyProtect		= 0,
};


static const struct uvc_format_mjpeg uvc_format_mjpg_fm1 = {
	.bLength		= UVC_DT_FORMAT_MJPEG_SIZE,
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VS_FORMAT_MJPEG,
	.bFormatIndex		= 1,
	.bNumFrameDescriptors	= 1,
	.bmFlags		= 0,
	.bDefaultFrameIndex	= 1,
	.bAspectRatioX		= 0,
	.bAspectRatioY		= 0,
	.bmInterfaceFlags	= 0,
	.bCopyProtect		= 0,
};

DECLARE_UVC_FRAME_MJPEG(5);

static const struct UVC_FRAME_MJPEG(5) uvc_frame_mjpg_360p = {
	.bLength		= UVC_DT_FRAME_MJPEG_SIZE(5),
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VS_FRAME_MJPEG,
	.bFrameIndex		= 1,
	.bmCapabilities		= 0,
	.wWidth			= cpu_to_le16(640),
	.wHeight		= cpu_to_le16(368),
	.dwMinBitRate		= cpu_to_le32(18432000),
	.dwMaxBitRate		= cpu_to_le32(55296000),
	.dwMaxVideoFrameBufferSize	= cpu_to_le32(460800),
	.dwDefaultFrameInterval	= cpu_to_le32(333333),
	.bFrameIntervalType	= 5,
	.dwFrameInterval[0]	= cpu_to_le32(166666), /*60fps*/
	.dwFrameInterval[1]	= cpu_to_le32(333333), /*30fps*/
	.dwFrameInterval[2]	= cpu_to_le32(666666), /*15fps*/
	.dwFrameInterval[3]	= cpu_to_le32(1000000), /*10fps*/
	.dwFrameInterval[4]	= cpu_to_le32(5000000), /*2fps*/
};

static const struct UVC_FRAME_MJPEG(5) uvc_frame_mjpg_720p = {
	.bLength		= UVC_DT_FRAME_MJPEG_SIZE(5),
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VS_FRAME_MJPEG,
	.bFrameIndex		= 2,
	.bmCapabilities		= 0,
	.wWidth			= cpu_to_le16(1280),
	.wHeight		= cpu_to_le16(720),
	.dwMinBitRate		= cpu_to_le32(29491200),
	.dwMaxBitRate		= cpu_to_le32(29491200),
	.dwMaxVideoFrameBufferSize	= cpu_to_le32(1843200),
	.dwDefaultFrameInterval	= cpu_to_le32(333333),
	.bFrameIntervalType	= 5,
	.dwFrameInterval[0]	= cpu_to_le32(166666), /*60fps*/
	.dwFrameInterval[1]	= cpu_to_le32(333333), /*30fps*/
	.dwFrameInterval[2]	= cpu_to_le32(666666), /*15fps*/
	.dwFrameInterval[3]	= cpu_to_le32(1000000), /*10fps*/
	.dwFrameInterval[4]	= cpu_to_le32(5000000), /*2fps*/
};

static const struct UVC_FRAME_MJPEG(5) uvc_frame_mjpg_1080p = {
	.bLength		= UVC_DT_FRAME_MJPEG_SIZE(5),
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VS_FRAME_MJPEG,
	.bFrameIndex		= 3,
	.bmCapabilities		= 0,
	.wWidth			= cpu_to_le16(1920),
	.wHeight		= cpu_to_le16(1088),
	.dwMinBitRate		= cpu_to_le32(29491200),
	.dwMaxBitRate		= cpu_to_le32(29491200),
	.dwMaxVideoFrameBufferSize	= cpu_to_le32(4147200),
	.dwDefaultFrameInterval	= cpu_to_le32(333333),
	.bFrameIntervalType	= 5,
	.dwFrameInterval[0]	= cpu_to_le32(166666), /*60fps*/
	.dwFrameInterval[1]	= cpu_to_le32(333333), /*30fps*/
	.dwFrameInterval[2]	= cpu_to_le32(666666), /*15fps*/
	.dwFrameInterval[3]	= cpu_to_le32(1000000), /*10fps*/
	.dwFrameInterval[4]	= cpu_to_le32(5000000), /*2fps*/
};

static /*const*/ struct uvc_format_frameBased uvc_format_h264 = {
	.bLength		= UVC_DT_FORMAT_FRAMEBASED_SIZE,
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType = UVC_VS_FORMAT_FRAME_BASED,
	.bFormatIndex		= 1,
	.bNumFrameDescriptors	= 1,
	.guidFormat			= { 'H',  '2',  '6',  '4',\
	0x00, 0x00, 0x10, 0x00, 0x80, 0x00, 0x00, 0xaa,\
	0x00, 0x38, 0x9b, 0x71},
	.bBitsPerPixel		= 0,
	.bDefaultFrameIndex	= 1,
	.bAspectRatioX		= 0,
	.bAspectRatioY		= 0,
	.bmInterfaceFlags	= 0,
	.bCopyProtect		= 0,
	.bVariableSize		= 1,
};

DECLARE_UVC_FRAME_FRAMEBASED(5);

static const struct UVC_FRAME_FRAMEBASED(5) uvc_frame_h264_360p = {
	.bLength		= UVC_DT_FRAME_FRAMEBASED_SIZE(5),
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType = UVC_VS_FRAME_FRAME_BASED,
	.bFrameIndex		= 1,
	.bmCapabilities		= 0,
	.wWidth				= cpu_to_le16(640),
	.wHeight			= cpu_to_le16(368),
	.dwMinBitRate		= cpu_to_le32(18432000),
	.dwMaxBitRate		= cpu_to_le32(55296000),
	.dwDefaultFrameInterval	= cpu_to_le32(333333),
	.bFrameIntervalType = 5,
	.dwBytesPerLine		= 0,
	/*when bFrameIntervalType = 0 which means continuous frame interval
	//.dwMinFrameInterval	= 0,
	//.dwMaxFrameInterval	= 0,
	//.dwFrameIntervalStep= 0,*/
	.dwFrameInterval[0]	= cpu_to_le32(166666), /*60fps*/
	.dwFrameInterval[1]	= cpu_to_le32(333333), /*30fps*/
	.dwFrameInterval[2]	= cpu_to_le32(666666), /*15fps*/
	.dwFrameInterval[3]	= cpu_to_le32(1000000), /*10fps*/
	.dwFrameInterval[4]	= cpu_to_le32(5000000), /*2fps*/
};

static const struct UVC_FRAME_FRAMEBASED(5) uvc_frame_h264_720p = {
	.bLength		= UVC_DT_FRAME_FRAMEBASED_SIZE(5),
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType = UVC_VS_FRAME_FRAME_BASED,
	.bFrameIndex		= 2,
	.bmCapabilities		= 0,
	.wWidth				= cpu_to_le16(1280),
	.wHeight			= cpu_to_le16(720),
	.dwMinBitRate		= cpu_to_le32(18432000),
	.dwMaxBitRate		= cpu_to_le32(55296000),
	.dwDefaultFrameInterval	= cpu_to_le32(333333),
	.bFrameIntervalType = 5,
	.dwBytesPerLine		= 0,
	/*when bFrameIntervalType = 0 which means continuous frame interval
	//.dwMinFrameInterval	= 0,
	//.dwMaxFrameInterval	= 0,
	//.dwFrameIntervalStep= 0,*/
	.dwFrameInterval[0]	= cpu_to_le32(166666), /*60fps*/
	.dwFrameInterval[1]	= cpu_to_le32(333333), /*30fps*/
	.dwFrameInterval[2]	= cpu_to_le32(666666), /*15fps*/
	.dwFrameInterval[3]	= cpu_to_le32(1000000), /*10fps*/
	.dwFrameInterval[4]	= cpu_to_le32(5000000), /*2fps*/
};

static const struct UVC_FRAME_FRAMEBASED(5) uvc_frame_h264_1080p = {
	.bLength		= UVC_DT_FRAME_FRAMEBASED_SIZE(5),
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType = UVC_VS_FRAME_FRAME_BASED,
	.bFrameIndex		= 3,
	.bmCapabilities		= 0,
	.wWidth				= cpu_to_le16(1920),
	.wHeight			= cpu_to_le16(1088),
	.dwMinBitRate		= cpu_to_le32(18432000),
	.dwMaxBitRate		= cpu_to_le32(55296000),
	.dwDefaultFrameInterval	= cpu_to_le32(333333),
	.bFrameIntervalType = 5,
	.dwBytesPerLine		= 0,
	/*when bFrameIntervalType = 0 which means continuous frame interval
	//.dwMinFrameInterval	= 0,
	//.dwMaxFrameInterval	= 0,
	//.dwFrameIntervalStep= 0,*/
	.dwFrameInterval[0]	= cpu_to_le32(166666), /*60fps*/
	.dwFrameInterval[1]	= cpu_to_le32(333333), /*30fps*/
	.dwFrameInterval[2]	= cpu_to_le32(666666), /*15fps*/
	.dwFrameInterval[3]	= cpu_to_le32(1000000), /*10fps*/
	.dwFrameInterval[4]	= cpu_to_le32(5000000), /*2fps*/
};
#if 0
static const struct uvc_still_image_descriptor uvc_still_image = {
	.bLength		= UVC_DT_STILL_IMAGE_SIZE,
	.bDescriptorType	= 0x24,
	.bDescriptorSubType	= 0x03,
	.bEndpointAddress	= 0,
	.bNumImageSizePatterns	= 1,
	.wSizePatterns[0]	= 640,
	.wSizePatterns[1]   = 480,
	.bNumCompressionPattern = 0,
};
#endif
static const struct uvc_color_matching_descriptor uvc_color_matching = {
	.bLength		= UVC_DT_COLOR_MATCHING_SIZE,
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VS_COLORFORMAT,
	.bColorPrimaries	= 1,
	.bTransferCharacteristics	= 1,
	.bMatrixCoefficients	= 4,
};

static const struct uvc_descriptor_header * const uvc_control_cls[] = {
	(const struct uvc_descriptor_header *) &uvc_control_header,
	(const struct uvc_descriptor_header *) &uvc_camera_terminal,
	(const struct uvc_descriptor_header *) &uvc_processing,
	(const struct uvc_descriptor_header *) &h264_extension_unit,
	(const struct uvc_descriptor_header *) &uvc_output_terminal,
#ifdef UVC_DOUBLE_STREAM
	(const struct uvc_descriptor_header *) &uvc_output_terminal2,
#endif
	NULL,
};

static const struct uvc_descriptor_header * const uvc_fs_streaming_cls[] = {
	(const struct uvc_descriptor_header *) &uvc_input_header,
	/*(const struct uvc_descriptor_header *) &uvc_format_nv12,
	//(const struct uvc_descriptor_header *) &uvc_frame_nv12_360p,
	//(const struct uvc_descriptor_header *) &uvc_frame_nv12_720p,
	//(const struct uvc_descriptor_header *) &uvc_frame_nv12_1080p,*/
	(const struct uvc_descriptor_header *) &uvc_format_mjpg,
	(const struct uvc_descriptor_header *) &uvc_frame_mjpg_360p,
	(const struct uvc_descriptor_header *) &uvc_frame_mjpg_720p,
	(const struct uvc_descriptor_header *) &uvc_frame_mjpg_1080p,
	/*(const struct uvc_descriptor_header *) &uvc_format_h264,
	//(const struct uvc_descriptor_header *) &uvc_frame_h264_360p,
	//(const struct uvc_descriptor_header *) &uvc_frame_h264_720p,
	//(const struct uvc_descriptor_header *) &uvc_frame_h264_1080p,*/
	(const struct uvc_descriptor_header *) &uvc_color_matching,
	NULL,
};

static const struct uvc_descriptor_header * const uvc_hs_streaming_cls[] = {
	(const struct uvc_descriptor_header *) &uvc_input_header,
	/*(const struct uvc_descriptor_header *) &uvc_format_nv12,
	//(const struct uvc_descriptor_header *) &uvc_frame_nv12_360p,
	//(const struct uvc_descriptor_header *) &uvc_frame_nv12_720p,
	//(const struct uvc_descriptor_header *) &uvc_frame_nv12_1080p,
	//(const struct uvc_descriptor_header *) &uvc_format_mjpg,
	//(const struct uvc_descriptor_header *) &uvc_frame_mjpg_360p,
	//(const struct uvc_descriptor_header *) &uvc_frame_mjpg_720p,
	//(const struct uvc_descriptor_header *) &uvc_frame_mjpg_1080p,*/
	(const struct uvc_descriptor_header *) &uvc_format_h264,
	(const struct uvc_descriptor_header *) &uvc_frame_h264_360p,
	/*(const struct uvc_descriptor_header *) &uvc_frame_h264_720p,
	//(const struct uvc_descriptor_header *) &uvc_frame_h264_1080p,*/
	(const struct uvc_descriptor_header *) &uvc_color_matching,
	NULL,
};

#define MAX_FRAME_DESCRIPTORS_NUM   10
#define MAX_FRAME_INTERVAL_NUM      5
struct uvc_frm_mjpg_info {
	__u8  bLength;
	__u8  bDescriptorType;
	__u8  bDescriptorSubType;
	__u8  bFrameIndex;
	__u8  bmCapabilities;
	__u16 wWidth;
	__u16 wHeight;
	__u32 dwMinBitRate;
	__u32 dwMaxBitRate;
	__u32 dwMaxVideoFrameBufferSize;
	__u32 dwDefaultFrameInterval;
	__u8  bFrameIntervalType;
	__u32 dwFrameInterval[MAX_FRAME_INTERVAL_NUM];
} __packed;


struct uvc_frm_yuv_info {
	__u8  bLength;
	__u8  bDescriptorType;
	__u8  bDescriptorSubType;
	__u8  bFrameIndex;
	__u8  bmCapabilities;
	__u16 wWidth;
	__u16 wHeight;
	__u32 dwMinBitRate;
	__u32 dwMaxBitRate;
	__u32 dwMaxVideoFrameBufferSize;
	__u32 dwDefaultFrameInterval;
	__u8  bFrameIntervalType;
	__u32 dwFrameInterval[MAX_FRAME_INTERVAL_NUM];
} __packed;



struct uvc_frm_h264_info {
	__u8  bLength;
	__u8  bDescriptorType;
	__u8  bDescriptorSubType;
	__u8  bFrameIndex;
	__u8  bmCapabilities;
	__u16 wWidth;
	__u16 wHeight;
	__u32 dwMinBitRate;
	__u32 dwMaxBitRate;
	__u32 dwDefaultFrameInterval;
	__u8  bFrameIntervalType;
	__u32 dwBytesPerLine;
	__u32 dwFrameInterval[MAX_FRAME_INTERVAL_NUM];
} __packed;


struct uvc_frame_info {
	unsigned int width;
	unsigned int height;
	unsigned int intervals[8];
};

struct uvc_format_info {
	unsigned int fcc;
	const struct uvc_frame_info *frames;
};


struct uvc_fmt_array_info {
	unsigned int yuv_type;

	unsigned int fmt_num;
	struct uvc_format_info *pFmts;

	unsigned int fmt2_num;
	struct uvc_format_info *pFmts2;

};




struct uvc_fmt_array_data {
	struct UVC_INPUT_HEADER_DESCRIPTOR(1, 1) uvc_input_header;
	struct uvc_format_uncompressed uvc_format_nv12;
	struct uvc_format_mjpeg uvc_format_mjpg;
	struct uvc_format_frameBased uvc_format_h264;

	struct uvc_frm_yuv_info yuv_frames[MAX_FRAME_DESCRIPTORS_NUM];
	struct uvc_frm_mjpg_info mjpg_frames[MAX_FRAME_DESCRIPTORS_NUM];
	struct uvc_frm_h264_info h264_frames[MAX_FRAME_DESCRIPTORS_NUM];
	struct uvc_descriptor_header *uvc_streaming_data[MAX_FRAME_DESCRIPTORS_NUM*4];

	int fmt_num;
};

static struct uvc_fmt_array_data uvc_fmt_array[2];
static struct uvc_fmt_array_data *pCurFmtArray;


static int uvc_stream_idx;
static int uvc_fmt_idx = 1;


int get_frame_array_num(const struct uvc_frame_info *pFrms)
{
	const struct uvc_frame_info *pFrmData = pFrms;
	int num = 0;
	while (1) {
		if (pFrmData->width == 0)
			break;
		num++;
		pFrmData++;
	}

	if (num > MAX_FRAME_DESCRIPTORS_NUM)
		num = MAX_FRAME_DESCRIPTORS_NUM;

	return num;
}


int get_frame_intervals_num(const struct uvc_frame_info *pFrms)
{
	const struct uvc_frame_info *pFrmData = pFrms;
	int num = 0;

	while (1) {
		if (pFrmData->intervals[num] == 0)
			break;
		num++;
	}

	if (num > MAX_FRAME_INTERVAL_NUM)
		num = MAX_FRAME_INTERVAL_NUM;

	return num;
}


static void
gen_yuv_frame_data(struct uvc_frm_yuv_info *pYuv,
					int idx,
					const struct uvc_frame_info *pFrm)
{
	int i;
	int num = get_frame_intervals_num(pFrm);

	pYuv->bLength		        = UVC_DT_FRAME_UNCOMPRESSED_SIZE(num);
	pYuv->bDescriptorType	    = USB_DT_CS_INTERFACE;
	pYuv->bDescriptorSubType	= UVC_VS_FRAME_UNCOMPRESSED;
	pYuv->bFrameIndex		    = idx;
	pYuv->bmCapabilities		= 0;
	pYuv->wWidth			    = cpu_to_le16(pFrm->width);
	pYuv->wHeight		        = cpu_to_le16(pFrm->height);
	pYuv->dwMinBitRate		    = cpu_to_le32(18432000);
	pYuv->dwMaxBitRate		    = cpu_to_le32(55296000);
	pYuv->dwMaxVideoFrameBufferSize	= cpu_to_le32(471040);
	pYuv->dwDefaultFrameInterval	= cpu_to_le32(pFrm->intervals[0]);

	pYuv->bFrameIntervalType	= num;
	for (i = 0; i < num; i++)
		pYuv->dwFrameInterval[i]	=
				cpu_to_le32(pFrm->intervals[i]);

}


static void
gen_mjpg_frame_data(struct uvc_frm_mjpg_info *pMjpg,
					int idx,
					const struct uvc_frame_info *pFrm)
{
	int i;
	int num = get_frame_intervals_num(pFrm);

	pMjpg->bLength		                = UVC_DT_FRAME_MJPEG_SIZE(num);
	pMjpg->bDescriptorType	            = USB_DT_CS_INTERFACE;
	pMjpg->bDescriptorSubType	        = UVC_VS_FRAME_MJPEG;
	pMjpg->bFrameIndex		            = idx;
	pMjpg->bmCapabilities		        = 0;
	pMjpg->wWidth			            = cpu_to_le16(pFrm->width);
	pMjpg->wHeight		                = cpu_to_le16(pFrm->height);
	pMjpg->dwMinBitRate		            = cpu_to_le32(18432000);
	pMjpg->dwMaxBitRate		            = cpu_to_le32(55296000);
	pMjpg->dwMaxVideoFrameBufferSize	= cpu_to_le32(460800);
	pMjpg->dwDefaultFrameInterval	    = cpu_to_le32(pFrm->intervals[0]);


	pMjpg->bFrameIntervalType	= num;
	for (i = 0; i < num; i++)
		pMjpg->dwFrameInterval[i]	=
					cpu_to_le32(pFrm->intervals[i]);

}

static void gen_h264_frame_data(struct uvc_frm_h264_info *pH264,
								int idx,
					const struct uvc_frame_info *pFrm)
{
	int i;
	int num = get_frame_intervals_num(pFrm);

	pH264->bLength		        = UVC_DT_FRAME_FRAMEBASED_SIZE(num);
	pH264->bDescriptorType	    = USB_DT_CS_INTERFACE;
	pH264->bDescriptorSubType   = UVC_VS_FRAME_FRAME_BASED;
	pH264->bFrameIndex		    = idx;
	pH264->bmCapabilities		= 0;
	pH264->wWidth				= cpu_to_le16(pFrm->width);
	pH264->wHeight			    = cpu_to_le16(pFrm->height);
	pH264->dwMinBitRate		    = cpu_to_le32(18432000);
	pH264->dwMaxBitRate		    = cpu_to_le32(55296000);
	pH264->dwDefaultFrameInterval	= cpu_to_le32(pFrm->intervals[0]);
	pH264->dwBytesPerLine		= 0;

	pH264->bFrameIntervalType	= num;
	for (i = 0; i < num; i++)
		pH264->dwFrameInterval[i] = cpu_to_le32(pFrm->intervals[i]);

}


void uvc_stream_append_data(void *data)
{
	pCurFmtArray->uvc_streaming_data[uvc_stream_idx++] =
					(struct uvc_descriptor_header *)data;
}

void deal_frms_array(unsigned int fcc, const struct uvc_frame_info *pFrms)
{

	int frm_num = get_frame_array_num(pFrms);
	const struct uvc_frame_info *pFrmData = pFrms;
	int i;

	if (frm_num <= 0)
		return;


	switch (fcc) {
	case V4L2_PIX_FMT_NV12:
		uvc_stream_append_data(&pCurFmtArray->uvc_format_nv12);
		pCurFmtArray->uvc_format_nv12.bFormatIndex = uvc_fmt_idx++;
		pCurFmtArray->uvc_format_nv12.bNumFrameDescriptors = frm_num;
		for (i = 0; i < frm_num; i++) {
			gen_yuv_frame_data(&pCurFmtArray->yuv_frames[i],
				i+1, pFrmData);
			uvc_stream_append_data(&pCurFmtArray->yuv_frames[i]);
			pFrmData++;
		}
		break;

	case V4L2_PIX_FMT_MJPEG:
		uvc_stream_append_data(&pCurFmtArray->uvc_format_mjpg);
		pCurFmtArray->uvc_format_mjpg.bFormatIndex = uvc_fmt_idx++;
		pCurFmtArray->uvc_format_mjpg.bNumFrameDescriptors = frm_num;
		for (i = 0; i < frm_num; i++) {
			gen_mjpg_frame_data(&pCurFmtArray->mjpg_frames[i],
				i+1, pFrmData);
			uvc_stream_append_data(&pCurFmtArray->mjpg_frames[i]);
			pFrmData++;
		}
		break;

	case V4L2_PIX_FMT_H264:
		uvc_stream_append_data(&pCurFmtArray->uvc_format_h264);
		pCurFmtArray->uvc_format_h264.bFormatIndex = uvc_fmt_idx++;
		pCurFmtArray->uvc_format_h264.bNumFrameDescriptors = frm_num;
		for (i = 0; i < frm_num; i++) {
			gen_h264_frame_data(&pCurFmtArray->h264_frames[i],
				i+1, pFrmData);
			uvc_stream_append_data(&pCurFmtArray->h264_frames[i]);
			pFrmData++;
		}
		break;
	}
}


/*void set_yuv_bpp(int bpp);*/
static __u8  nv12Format[16] = { 'N',  'V',  '1',  '2', 0x00, 0x00, 0x10, 0x00,
	0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71};

static __u8  yuy2Format[16] = { 'Y',  'U',  'Y',  '2', 0x00, 0x00, 0x10, 0x00,
	0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71};



void gen_uvc_fmt_array(int id, struct uvc_format_info *fmt, int fmt_num)
{
	struct uvc_format_info *pFmtData = fmt;
	unsigned int i;

	uvc_stream_idx = 0;
	uvc_fmt_idx = 1;

	pCurFmtArray = &uvc_fmt_array[id&0x01];
	pCurFmtArray->fmt_num = fmt_num;
	memcpy(&pCurFmtArray->uvc_input_header, &uvc_input_header,
			sizeof(uvc_input_header));
	memcpy(&pCurFmtArray->uvc_format_nv12, &uvc_format_nv12,
			sizeof(uvc_format_nv12));
	memcpy(&pCurFmtArray->uvc_format_mjpg, &uvc_format_mjpg,
			sizeof(uvc_format_mjpg));
	memcpy(&pCurFmtArray->uvc_format_h264, &uvc_format_h264,
			sizeof(uvc_format_h264));

	uvc_stream_append_data((void *)&pCurFmtArray->uvc_input_header);


	for (i = 0; i < fmt_num; i++) {
		deal_frms_array(pFmtData->fcc, pFmtData->frames);
		pFmtData++;
	}
	/*uvc_stream_append_data((void*)&uvc_still_image);*/
	uvc_stream_append_data((void *)&uvc_color_matching);
	uvc_stream_append_data(NULL);
}

void gen_stream_descriptor_array(struct uvc_fmt_array_info *pFai)
{
	switch (pFai->yuv_type)	{
	case YUV_YUY2:
		set_yuv_bpp(16);
		memcpy(uvc_format_nv12.guidFormat, yuy2Format,
				sizeof(yuy2Format));

		uvc_format_nv12.bBitsPerPixel      = 16;
		break;

	case YUV_NV12:
	default:
		set_yuv_bpp(12);
		memcpy(uvc_format_nv12.guidFormat,
								nv12Format,
						sizeof(nv12Format));
		uvc_format_nv12.bBitsPerPixel      = 12;
		break;
	}

	gen_uvc_fmt_array(0, pFai->pFmts, pFai->fmt_num);
	gen_uvc_fmt_array(1, pFai->pFmts2, pFai->fmt2_num);
}

static struct uvc_descriptor_header *uvc_streaming_test[] = {
	(struct uvc_descriptor_header *) &uvc_input_header,
	/*(const struct uvc_descriptor_header *) &uvc_format_mjpg_fm1,
	(const struct uvc_descriptor_header *) &uvc_frame_mjpg_360p,*/
	(struct uvc_descriptor_header *) &uvc_format_h264,
	(struct uvc_descriptor_header *) &uvc_frame_h264_360p,
	(struct uvc_descriptor_header *) &uvc_color_matching,
	NULL,
};


void change_usb_support_fmt(struct uvc_device *uvc, void *arg)
{
	struct usb_function *f;
	if (NULL != arg) {
		gen_stream_descriptor_array(arg);
		uvc->comm->desc.fs_streaming =
			uvc_fmt_array[0].uvc_streaming_data;
		uvc->comm->desc.hs_streaming =
			uvc_fmt_array[0].uvc_streaming_data;

#ifdef UVC_DOUBLE_STREAM

		if (uvc_fmt_array[1].fmt_num > 0) {
			uvc->comm->desc.fs_streaming2 =
				uvc_fmt_array[1].uvc_streaming_data;
			uvc->comm->desc.hs_streaming2 =
				uvc_fmt_array[1].uvc_streaming_data;
			uvc_control_header.bLength = UVC_DT_HEADER_SIZE(2);
		} else {
			uvc->comm->desc.fs_streaming2 = NULL;
			uvc->comm->desc.hs_streaming2 = NULL;
		}
#else

		uvc->comm->desc.fs_streaming2 = NULL;
		uvc->comm->desc.hs_streaming2 = NULL;


#endif
	} else {
		return;
		uvc->comm->desc.fs_streaming = uvc_streaming_test;
		uvc->comm->desc.hs_streaming = uvc_streaming_test;

		uvc->comm->desc.fs_streaming2 = uvc_streaming_test;
		uvc->comm->desc.hs_streaming2 = uvc_streaming_test;
	}


	f = &uvc->comm->func;


	kfree(f->descriptors);
	kfree(f->hs_descriptors);

	/* Copy descriptors. */
	f->descriptors = uvc_copy_descriptors(uvc, USB_SPEED_FULL);

	f->hs_descriptors = uvc_copy_descriptors(uvc, USB_SPEED_HIGH);
}

/* --------------------------------------------------------------------------
 * USB configuration
 */


/*extern int uac_bind_config(struct usb_configuration *c);*/

static int __init
webcam_config_bind(struct usb_configuration *c)
{
	int ret;
#ifdef ENABLE_UVC
	ret = uvc_bind_config(c, uvc_control_cls, uvc_fs_streaming_cls,
				   uvc_hs_streaming_cls);
#endif

#ifdef ENABLE_UAC
	ret = uac_bind_config(c);
#endif
	return ret;
}

static struct usb_configuration webcam_config_driver = {
	.label			= webcam_config_label,
	.bConfigurationValue	= 1,
	.iConfiguration		= 0, /* dynamic */
	.bmAttributes		= USB_CONFIG_ATT_SELFPOWER,
	.bMaxPower		= CONFIG_USB_GADGET_VBUS_DRAW / 2,
};

static int /* __init_or_exit */
webcam_unbind(struct usb_composite_dev *cdev)
{
	return 0;
}

static int __init
webcam_bind(struct usb_composite_dev *cdev)
{
	int ret;

	/* Allocate string descriptor numbers ... note that string contents
	 * can be overridden by the composite_dev glue.
	 */
	ret = usb_string_id(cdev);
	if (ret < 0)
		goto error;
	webcam_strings[STRING_MANUFACTURER_IDX].id = ret;
	webcam_device_descriptor.iManufacturer = ret;
	ret = usb_string_id(cdev);
	if (ret < 0)
		goto error;
	webcam_strings[STRING_PRODUCT_IDX].id = ret;
	webcam_device_descriptor.iProduct = ret;
	ret = usb_string_id(cdev);
	if (ret < 0)
		goto error;
	webcam_strings[STRING_DESCRIPTION_IDX].id = ret;
	webcam_config_driver.iConfiguration = ret;

	/* Register our configuration. */
	ret = usb_add_config(cdev, &webcam_config_driver, \
	webcam_config_bind);
	if (ret < 0)
		goto error;

	INFO(cdev, "Webcam Video Gadget\n");
	return 0;

error:
	webcam_unbind(cdev);
	return ret;
}

/* --------------------------------------------------------------------------
 * Driver
 */

static struct usb_composite_driver webcam_driver = {
	.name		= "g_webcam",
	.dev		= &webcam_device_descriptor,
	.strings	= webcam_device_strings,
	.unbind		= webcam_unbind,
};

static int __init
webcam_init(void)
{
	return usb_composite_probe(&webcam_driver, webcam_bind);
}

static void __exit
webcam_cleanup(void)
{
	usb_composite_unregister(&webcam_driver);
}

module_init(webcam_init);
module_exit(webcam_cleanup);

MODULE_AUTHOR("Laurent Pinchart");
MODULE_DESCRIPTION("Webcam Video Gadget");
MODULE_LICENSE("GPL");
MODULE_VERSION("0.1.0");

#endif

