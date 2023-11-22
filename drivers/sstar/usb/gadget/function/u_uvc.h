/*
 * u_uvc.h- Sigmastar
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

/* SPDX-License-Identifier: GPL-2.0 */
/*
 * u_uvc.h
 *
 * Utility definitions for the uvc function
 *
 * Copyright (c) 2013-2014 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com
 *
 * Author: Andrzej Pietrasiewicz <andrzejtp2010@gmail.com>
 */

#ifndef U_UVC_H
#define U_UVC_H

#include <linux/mutex.h>
#include <linux/usb/composite.h>
#include <linux/usb/video.h>

/* --------------------------------------------------------------------------
 * Customer Define Start
 */
#include "uvc_ait_xu.h"

#ifdef USE_AIT_XU
#define UVC_EU1_GUID UVC_AIT_EU1_GUID
#define UVC_EU2_GUID UVC_CUS_EU2_GUID
#define UVC_EU3_GUID UVC_MS_EU3_GUID
#else
#define UVC_EU1_GUID UVC_AIT_EU1_GUID
#define UVC_EU2_GUID UVC_CUS_EU2_GUID
#define UVC_EU3_GUID UVC_MS_EU3_GUID
#endif
// termail link:
//	 UVC_IT_ID -> UVC_PU_ID -> UVC_EU1_ID -> UVC_EU2_ID -> UVC_OT_ID;
//	 UVC_IT_ID -> UVC_PU_ID -> UVC_EU1_ID -> UVC_EU2_ID -> UVC_EU3_ID -> UVC_OT_ID;

#define UVC_EU1_ID (0x6) //for Isp use
#define UVC_EU2_ID (0x2) //for Customer to use
#define UVC_EU3_ID (0x4) //for Microsoft to use
#define UVC_PU_ID (0x3)
#define UVC_IT_ID (0x1)
#define UVC_OT_ID (0x7)

DECLARE_UVC_EXTENSION_UNIT_DESCRIPTOR(1, 2);
/* --------------------------------------------------------------------------
 * Customer Define End
 */

#ifndef CONFIG_MULTI_STREAM_FUNC_NUM
#define CONFIG_MULTI_STREAM_FUNC_NUM 1
#endif

#define MAX_STREAM_SUPPORT 6
#define DEFAULT_STREAM_NAME "UVC Camera"
#define MULTI_STREAM_NUM CONFIG_MULTI_STREAM_FUNC_NUM

#if (MULTI_STREAM_NUM > 1)
#define CONFIG_SS_GADGET_UVC_MULTI_STREAM
#endif

#define fi_to_f_uvc_opts(f) container_of(f, struct f_uvc_opts, func_inst)
#ifndef CONFIG_SS_GADGET_UVC_MULTI_STREAM
#define video_to_uvc(f) container_of(f, struct uvc_device, video)
#else
#define video_to_stream(f) container_of(f, struct uvc_streaming, video)
#define video_to_uvc(f) (video_to_stream(f))->dev
#endif

struct f_uvc_opts {
	struct usb_function_instance func_inst;
#ifndef CONFIG_SS_GADGET_UVC_MULTI_STREAM
	unsigned int streaming_interval;
	unsigned int streaming_maxpacket;
	unsigned int streaming_maxburst;
	char *streaming_name;
#else
	unsigned int streaming_interval[MAX_STREAM_SUPPORT];
	unsigned int streaming_maxpacket[MAX_STREAM_SUPPORT];
	unsigned int streaming_maxburst[MAX_STREAM_SUPPORT];
	char *streaming_name[MAX_STREAM_SUPPORT];
#endif
	bool bulk_streaming_ep;

	unsigned int control_interface;
	unsigned int streaming_interface;

	/*
	 * Control descriptors array pointers for full-/high-speed and
	 * super-speed. They point by default to the uvc_fs_control_cls and
	 * uvc_ss_control_cls arrays respectively. Legacy gadgets must
	 * override them in their gadget bind callback.
	 */
	const struct uvc_descriptor_header *const *fs_control;
	const struct uvc_descriptor_header *const *ss_control;

	/*
	 * Streaming descriptors array pointers for full-speed, high-speed and
	 * super-speed. They will point to the uvc_[fhs]s_streaming_cls arrays
	 * for configfs-based gadgets. Legacy gadgets must initialize them in
	 * their gadget bind callback.
	 */
	const struct uvc_descriptor_header *const *fs_streaming;
	const struct uvc_descriptor_header *const *hs_streaming;
	const struct uvc_descriptor_header *const *ss_streaming;

	/* Default control descriptors for configfs-based gadgets. */
	struct uvc_camera_terminal_descriptor uvc_camera_terminal;
	struct uvc_processing_unit_descriptor uvc_processing;
	struct UVC_EXTENSION_UNIT_DESCRIPTOR(1, 2) uvc_extension1;
	struct UVC_EXTENSION_UNIT_DESCRIPTOR(1, 2) uvc_extension2;
	struct uvc_output_terminal_descriptor uvc_output_terminal;
	struct uvc_color_matching_descriptor uvc_color_matching;

	/*
	 * Control descriptors pointers arrays for full-/high-speed and
	 * super-speed. The first element is a configurable control header
	 * descriptor, the other elements point to the fixed default control
	 * descriptors. Used by configfs only, must not be touched by legacy
	 * gadgets.
	 */
	struct uvc_descriptor_header *uvc_fs_control_cls[7];
	struct uvc_descriptor_header *uvc_ss_control_cls[7];

	/*
	 * Streaming descriptors for full-speed, high-speed and super-speed.
	 * Used by configfs only, must not be touched by legacy gadgets. The
	 * arrays are allocated at runtime as the number of descriptors isn't
	 * known in advance.
	 */
	struct uvc_descriptor_header **uvc_fs_streaming_cls;
	struct uvc_descriptor_header **uvc_hs_streaming_cls;
	struct uvc_descriptor_header **uvc_ss_streaming_cls;

	/*
	 * Read/write access to configfs attributes is handled by configfs.
	 *
	 * This lock protects the descriptors from concurrent access by
	 * read/write and symlink creation/removal.
	 */
	struct mutex lock;
	int refcnt;
};

#endif /* U_UVC_H */
