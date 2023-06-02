/*
 *	uvc_gadget.c  --  USB Video Class Gadget driver
 *
 *	Copyright (C) 2009-2010
 *	    Laurent Pinchart (laurent.pinchart@ideasonboard.com)
 *
 *	This program is free software; you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation; either version 2 of the License, or
 *	(at your option) any later version.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/errno.h>
#include <linux/usb/ch9.h>
#include <linux/usb/gadget.h>
#include <linux/usb/video.h>

#include "uvc.h"
#include "uvc_format.h"


#ifdef UVC_FASTBOOT

#define YUY2_MAX_W              1280
#define YUY2_MAX_H            720

static const struct uvc_frame_info uvc_frames_nv12[] = {
	{ 640, 368, {333333, 400000, 0 }, },
	{ 640, 480, {333333, 400000, 0 }, },
	{ 1024, 576, {333333, 400000, 0 }, },
	{ YUY2_MAX_W, YUY2_MAX_H, {333333, 400000, 0 }, },
	{ 0, 0, { 0, }, },
};

static const struct uvc_frame_info uvc_frames_yuy2[] = {
	{ 640, 368, {333333, 400000, 0 }, },
	{ 640, 480, {333333, 400000, 0 }, },
	{ 1024, 576, {333333, 400000, 0 }, },
	{ YUY2_MAX_W, YUY2_MAX_H, {333333, 400000, 0 }, },
	{ 0, 0, { 0, }, },
};

static const struct uvc_frame_info uvc_frames_mjpeg[] = {
	{ 640, 480, {333333, 400000, 0 }, },
	{ 1280, 720, {333333, 400000, 0 }, },
	{ 1920, 1080, {333333, 400000, 0 }, },
	{ 1920, 1088, {333333, 400000, 0 }, },
	{ 2560, 1440, {333333, 400000, 0 }, },
	{ 0, 0, { 0, }, },
};

static const struct uvc_frame_info uvc_frames_h264[] = {
	{ 640, 480, {333333, 400000, 0 }, },
	{ 1280, 720, {333333, 400000, 0 }, },
	{ 1920, 1080, {333333, 400000, 0 }, },
	{ 2560, 1440, {333333, 400000, 0 }, },
	{ 0, 0, { 0, }, },
};

static const struct uvc_format_info uvc_formats[] = {
	{ V4L2_PIX_FMT_NV12, uvc_frames_nv12, ARRAY_SIZE(uvc_frames_nv12)},
	{ V4L2_PIX_FMT_YUY2, uvc_frames_yuy2, ARRAY_SIZE(uvc_frames_yuy2)},
	{ V4L2_PIX_FMT_MJPEG, uvc_frames_mjpeg, ARRAY_SIZE(uvc_frames_mjpeg)},
	{ V4L2_PIX_FMT_H264, uvc_frames_h264, ARRAY_SIZE(uvc_frames_h264)},
};
#endif

#define YUV_NV12    0
#define YUV_YUY2    1

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

DECLARE_UVC_INPUT_HEADER_DESCRIPTOR(1, 1);
DECLARE_UVC_INPUT_HEADER_DESCRIPTOR(1, 5);

struct uvc_fmt_array_data {
	struct UVC_INPUT_HEADER_DESCRIPTOR(1, 5) uvc_input_header;
	struct uvc_format_uncompressed uvc_format_nv12;
	struct uvc_format_uncompressed uvc_format_yuy2;
	struct uvc_format_mjpeg uvc_format_mjpg;
	struct uvc_format_frameBased uvc_format_h264;
	struct uvc_format_frameBased uvc_format_hevc;

	struct uvc_frm_yuv_info yuv_frames[MAX_FRAME_DESCRIPTORS_NUM];
	struct uvc_frm_yuv_info yuy2_frames[MAX_FRAME_DESCRIPTORS_NUM];
	struct uvc_frm_mjpg_info mjpg_frames[MAX_FRAME_DESCRIPTORS_NUM];
	struct uvc_frm_h264_info h264_frames[MAX_FRAME_DESCRIPTORS_NUM];
	struct uvc_frm_h264_info hevc_frames[MAX_FRAME_DESCRIPTORS_NUM];
	struct uvc_descriptor_header *uvc_streaming_data[MAX_FRAME_DESCRIPTORS_NUM*4];

	int fmt_num;
};

static int uvc_stream_idx;
static int uvc_fmt_idx = 1;
static struct uvc_fmt_array_data *pCurFmtArray;
static struct uvc_fmt_array_data uvc_fmt_array[2];



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
	// .bmaControls[1][0]	= 4,
};

static /*const*/ struct uvc_format_uncompressed uvc_format_nv12 = {
	.bLength		= UVC_DT_FORMAT_UNCOMPRESSED_SIZE,
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VS_FORMAT_UNCOMPRESSED,
	.bFormatIndex		= 1,
	.bNumFrameDescriptors	= 3,
	.guidFormat		= { 'N',  'V',  '1',  '2',
	0x00, 0x00, 0x10, 0x00, 0x80, 0x00, 0x00,
	0xaa, 0x00, 0x38, 0x9b, 0x71},
	.bBitsPerPixel      = 12,
	.bDefaultFrameIndex	= 1,
	.bAspectRatioX		= 0,
	.bAspectRatioY		= 0,
	.bmInterfaceFlags	= 0,
	.bCopyProtect		= 0,
};

static /*const*/ struct uvc_format_uncompressed uvc_format_yuy2 = {
	.bLength		= UVC_DT_FORMAT_UNCOMPRESSED_SIZE,
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VS_FORMAT_UNCOMPRESSED,
	.bFormatIndex		= 1,
	.bNumFrameDescriptors	= 3,
	.guidFormat		= { 'Y', 'U',  'Y', '2', 0x00, 0x00, 0x10, 0x00,
					0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71},
	.bBitsPerPixel      = 16,
	.bDefaultFrameIndex	= 1,
	.bAspectRatioX		= 0,
	.bAspectRatioY		= 0,
	.bmInterfaceFlags	= 0,
	.bCopyProtect		= 0,
};

static const struct uvc_format_mjpeg uvc_format_mjpg = {
	.bLength		= UVC_DT_FORMAT_MJPEG_SIZE,
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VS_FORMAT_MJPEG,
	.bFormatIndex		= 2,
	.bNumFrameDescriptors	= 2,
	.bmFlags		= 0,
	.bDefaultFrameIndex	= 1,
	.bAspectRatioX		= 0,
	.bAspectRatioY		= 0,
	.bmInterfaceFlags	= 0,
	.bCopyProtect		= 0,
};

static const struct uvc_color_matching_descriptor uvc_color_matching = {
	.bLength		= UVC_DT_COLOR_MATCHING_SIZE,
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VS_COLORFORMAT,
	.bColorPrimaries	= 1,
	.bTransferCharacteristics	= 1,
	.bMatrixCoefficients	= 4,
};

static const struct uvc_format_frameBased uvc_format_h264 = {
	.bLength		= UVC_DT_FORMAT_FRAMEBASED_SIZE,
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType = UVC_VS_FORMAT_FRAME_BASED,
	.bFormatIndex		= 1,
	.bNumFrameDescriptors	= 1,
	.guidFormat			= { 'H',  '2',  '6',  '4',
	0x00, 0x00, 0x10, 0x00, 0x80, 0x00, 0x00, 0xaa,
	0x00, 0x38, 0x9b, 0x71},
	.bBitsPerPixel		= 16,
	.bDefaultFrameIndex	= 1,
	.bAspectRatioX		= 0,
	.bAspectRatioY		= 0,
	.bmInterfaceFlags	= 0,
	.bCopyProtect		= 0,
	.bVariableSize		= 1,
};

static const struct uvc_format_frameBased uvc_format_hevc = {
	.bLength        = UVC_DT_FORMAT_FRAMEBASED_SIZE,
	.bDescriptorType    = USB_DT_CS_INTERFACE,
	.bDescriptorSubType = UVC_VS_FORMAT_FRAME_BASED,
	.bFormatIndex        = 1,
	.bNumFrameDescriptors    = 1,
	.guidFormat = {
		 'H',  '2',  '6',  '5', 0x00, 0x00, 0x10, 0x00,
			0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71},
	.bBitsPerPixel        = 16,
	.bDefaultFrameIndex    = 1,
	.bAspectRatioX        = 0,
	.bAspectRatioY        = 0,
	.bmInterfaceFlags    = 0,
	.bCopyProtect        = 0,
	.bVariableSize        = 1,
};

static int get_frame_array_num(const struct uvc_frame_info *pFrms)
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

static int get_frame_intervals_num(const struct uvc_frame_info *pFrms)
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

static void uvc_stream_append_data(void *data)
{
	pCurFmtArray->uvc_streaming_data[uvc_stream_idx++] =
					(struct uvc_descriptor_header *)data;
}

static void deal_frms_array(unsigned int fcc, const struct uvc_frame_info *pFrms)
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
	case V4L2_PIX_FMT_YUY2:
		uvc_stream_append_data(&pCurFmtArray->uvc_format_yuy2);
		pCurFmtArray->uvc_format_yuy2.bFormatIndex = uvc_fmt_idx++;
		pCurFmtArray->uvc_format_yuy2.bNumFrameDescriptors = frm_num;
		for (i = 0; i < frm_num; i++) {
			gen_yuv_frame_data(&pCurFmtArray->yuy2_frames[i], i+1, pFrmData);
			uvc_stream_append_data(&pCurFmtArray->yuy2_frames[i]);
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
	case V4L2_PIX_FMT_H265:
		uvc_stream_append_data(&pCurFmtArray->uvc_format_hevc);
		pCurFmtArray->uvc_format_hevc.bFormatIndex = uvc_fmt_idx++;
		pCurFmtArray->uvc_format_hevc.bNumFrameDescriptors = frm_num;
		for (i = 0; i < frm_num; i++) {
			gen_h264_frame_data(&pCurFmtArray->hevc_frames[i], i+1, pFrmData);
			uvc_stream_append_data(&pCurFmtArray->hevc_frames[i]);
			pFrmData++;
		}
		break;
	}
}

static void gen_uvc_fmt_array(int id, struct uvc_format_info *fmt, int fmt_num)
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
	memcpy(&pCurFmtArray->uvc_format_yuy2, &uvc_format_yuy2,
			sizeof(uvc_format_yuy2));
	memcpy(&pCurFmtArray->uvc_format_mjpg, &uvc_format_mjpg,
			sizeof(uvc_format_mjpg));
	memcpy(&pCurFmtArray->uvc_format_h264, &uvc_format_h264,
			sizeof(uvc_format_h264));
	memcpy(&pCurFmtArray->uvc_format_hevc, &uvc_format_hevc,
			sizeof(uvc_format_hevc));

	uvc_stream_append_data((void *)&pCurFmtArray->uvc_input_header);
	pCurFmtArray->uvc_input_header.bNumFormats = fmt_num;
	pCurFmtArray->uvc_input_header.bLength = UVC_DT_INPUT_HEADER_SIZE(1, fmt_num);


	for (i = 0; i < fmt_num; i++) {
		deal_frms_array(pFmtData->fcc, pFmtData->frames);
		pFmtData++;
	}

	uvc_stream_append_data((void *)&uvc_color_matching);
	uvc_stream_append_data(NULL);
}

static void gen_stream_descriptor_array(struct uvc_fmt_array_info *pFai)
{
	gen_uvc_fmt_array(0, pFai->pFmts, pFai->fmt_num);
}

void change_usb_support_fmt(struct uvc_device *uvc, void *arg)
{
	struct usb_function *f;
	struct uvc_fmt_array_info fai;
	struct uvc_format_info *fmt, *m_fmt;
	struct uvc_frame_info *frames;
	int ret, i;

	if (arg != NULL) {
		ret = copy_from_user(&fai, arg, sizeof(struct uvc_fmt_array_info));
		fmt = kmalloc(sizeof(struct uvc_format_info) * fai.fmt_num, GFP_KERNEL);
		ret = copy_from_user(fmt, fai.pFmts, sizeof(struct uvc_format_info) * fai.fmt_num);
		fai.pFmts = fmt;
		m_fmt = fmt;

		for (i = 0; i < fai.fmt_num; i++) {
			frames = kmalloc(sizeof(struct uvc_frame_info) * fmt->frame_num, GFP_KERNEL);
			ret = copy_from_user(frames, fmt->frames,
				sizeof(struct uvc_frame_info) * fmt->frame_num);
			fmt->frames = frames;
			fmt++;
		}

		gen_stream_descriptor_array(&fai);
		uvc->desc.fs_streaming =
			(const struct uvc_descriptor_header * const *)uvc_fmt_array[0].uvc_streaming_data;
		uvc->desc.hs_streaming =
			(const struct uvc_descriptor_header * const *)uvc_fmt_array[0].uvc_streaming_data;
		uvc->desc.ss_streaming =
			(const struct uvc_descriptor_header * const *)uvc_fmt_array[0].uvc_streaming_data;
	} else
		pr_err("[UVC] VIDIOC_RESET_DESC arg is NULL!\n");
	f = &uvc->func;
	kfree(f->fs_descriptors);
	/* Copy descriptors. */
	f->fs_descriptors = uvc_copy_descriptors(uvc, USB_SPEED_FULL);
	if (IS_ERR(f->fs_descriptors)) {
		ret = PTR_ERR(f->fs_descriptors);
		f->fs_descriptors = NULL;
		return;
	}
	if (gadget_is_dualspeed(f->config->cdev->gadget)) {
		kfree(f->hs_descriptors);
		f->hs_descriptors = uvc_copy_descriptors(uvc, USB_SPEED_HIGH);
		if (IS_ERR(f->hs_descriptors)) {
			ret = PTR_ERR(f->hs_descriptors);
			f->hs_descriptors = NULL;
			return;
		}
	}
	if (gadget_is_superspeed(f->config->cdev->gadget)) {
		kfree(f->ss_descriptors);
		f->ss_descriptors = uvc_copy_descriptors(uvc, USB_SPEED_SUPER);
		if (IS_ERR(f->ss_descriptors)) {
			ret = PTR_ERR(f->ss_descriptors);
			f->ss_descriptors = NULL;
			return;
		}
	}
	for (i = 0; i < fai.fmt_num; i++) {
		kfree(m_fmt->frames);
		m_fmt++;
	}
	kfree(fai.pFmts);
}

#ifdef UVC_FASTBOOT
void change_usb_support_fmt_fast(struct uvc_device *uvc)
{
	struct usb_function *f;
	struct uvc_fmt_array_info fai;
	int ret;

	fai.fmt_num = ARRAY_SIZE(uvc_formats);
	fai.pFmts = (struct uvc_format_info *)uvc_formats;

	gen_stream_descriptor_array(&fai);
	uvc->desc.fs_streaming =
		(const struct uvc_descriptor_header * const *)uvc_fmt_array[0].uvc_streaming_data;
	uvc->desc.hs_streaming =
		(const struct uvc_descriptor_header * const *)uvc_fmt_array[0].uvc_streaming_data;
	uvc->desc.ss_streaming =
		(const struct uvc_descriptor_header * const *)uvc_fmt_array[0].uvc_streaming_data;

	f = &uvc->func;
	kfree(f->fs_descriptors);
	/* Copy descriptors. */
	f->fs_descriptors = uvc_copy_descriptors(uvc, USB_SPEED_FULL);
	if (IS_ERR(f->fs_descriptors)) {
		ret = PTR_ERR(f->fs_descriptors);
		f->fs_descriptors = NULL;
		return;
	}
	if (gadget_is_dualspeed(f->config->cdev->gadget)) {
		kfree(f->hs_descriptors);
		f->hs_descriptors = uvc_copy_descriptors(uvc, USB_SPEED_HIGH);
		if (IS_ERR(f->hs_descriptors)) {
			ret = PTR_ERR(f->hs_descriptors);
			f->hs_descriptors = NULL;
			return;
		}
	}
	if (gadget_is_superspeed(f->config->cdev->gadget)) {
		kfree(f->ss_descriptors);
		f->ss_descriptors = uvc_copy_descriptors(uvc, USB_SPEED_SUPER);
		if (IS_ERR(f->ss_descriptors)) {
			ret = PTR_ERR(f->ss_descriptors);
			f->ss_descriptors = NULL;
			return;
		}
	}
}

int usb_fmt_to_usr(struct uvc_device *uvc, void *arg)
{
	struct uvc_fmt_array_info fai;
	struct uvc_format_info *fmt, *o_fmt, *m_fmt;
	int ret, i;

	if (arg != NULL) {
		ret = copy_from_user(&fai, arg,
			sizeof(struct uvc_fmt_array_info));
		fmt = kmalloc(sizeof(struct uvc_format_info) * fai.fmt_num,
			GFP_KERNEL);
		ret = copy_from_user(fmt, fai.pFmts,
			sizeof(struct uvc_format_info) * fai.fmt_num);
		o_fmt = fmt;

		m_fmt = fai.pFmts;
		fai.fmt_num = ARRAY_SIZE(uvc_formats);

		for (i = 0; i < fai.fmt_num; i++) {
			fmt->fcc = uvc_formats[i].fcc;
			fmt->frame_num = uvc_formats[i].frame_num;
			ret = copy_to_user((void *)m_fmt, fmt,
					sizeof(struct uvc_format_info));
			ret = copy_to_user((void *)fmt->frames,
				uvc_formats[i].frames,
				sizeof(struct uvc_frame_info) *
				uvc_formats[i].frame_num);

			fmt++;
			m_fmt++;
		}
	}
	kfree(o_fmt);

	return 0;

}
#endif
